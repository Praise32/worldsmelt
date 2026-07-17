#!/usr/bin/env python3
"""Corpus delle generazioni -> dataset per il futuro QLoRA (roguelike-ai-appunti/04).

Legge logs/gen-corpus/*.jsonl e le copie in logs/gen-metrics/*/corpus-*.jsonl
(o percorsi espliciti passati come argomenti) e ne estrae tre famiglie di
esempi in dataset/qlora/*.jsonl (creata da questo script, MAI versionata:
e' materiale derivato e rigenerabile dal corpus, non sorgente).

  a. manifest_pairs.jsonl -- (prompt, completion) per ogni JSON di run
     accettato (kind=manifest, ok=true). Il prompt NON e' nel corpus (troppo
     grosso, e comunque deterministico): si RICOSTRUISCE chiamando
     `bin/melting-gen --print-json-prompt --seed <attemptSeed>`, che si ferma
     prima di caricare qualunque modello (vedi main.c riga ~317). L'offset
     seed/attempt e' quello di RunJsonAttempts in main.c:
         for (attempt = 0; attempt < 2 ...)
             attemptSeed = args->seed + attempt*7919
             GenCorpusRecordJson(attempt + 1, ...)
     cioe' il ciclo e' 0-based ma il corpus registra l'attempt 1-based: per
     ricostruire il tentativo N del corpus serve attemptSeed = seed +
     (N-1)*7919. Verificato leggendo main.c, non per fiducia.

     Fedelta': il prompt ricostruito e' quello VERO solo se i file in
     tools/melting-gen/prompts/ non sono cambiati da quella generazione. Il
     corpus non registra promptsFnv (quello sta in generated/provenance.txt,
     fuori dal corpus, vedi gen_manifest.c) quindi non possiamo confrontarlo:
     usiamo un'euristica piu' debole ma verificabile, l'mtime. Un file corpus
     e' considerato "fresco" (quindi ricostruibile) solo se il suo mtime e'
     >= mtime piu' recente fra i file di tools/melting-gen/prompts/; gli
     altri vengono scartati con un conteggio esplicito nel riepilogo, MAI
     ricostruiti alla cieca.

  b. lua_valid.jsonl -- gli script Lua validati (kind=lua, outcome=ok),
     deduplicati per sha256 dello script: lo stesso script compare spesso
     su piu' run/oggetti diversi (pattern comuni tipo spawn_shot bounce) e
     contarlo una volta sola evita di sovrappesare il pattern piu' frequente
     nel training set.

  c. lua_repairs.jsonl -- le coppie errore->correzione: nello stesso file,
     per lo stesso (theme,item), un record con attempt N outcome
     invalid/decode-failed seguito dal record attempt N+1 outcome ok. Sono
     l'oro del rejection-fine-tuning (l'idea e' insegnare la CORREZIONE, non
     solo lo script buono). I record decode-failed non hanno "script" (la
     generazione e' fallita prima di poterlo estrarre): quelle coppie
     vengono scartate, non si puo' mostrare una correzione senza il "prima".

Sottocomandi:
  build (default)  Scrive i tre file sotto dataset/qlora/ e stampa il
                    riepilogo. Idempotente: sovrascrive sempre da zero.
  stats             Solo il riepilogo, non scrive nulla.

Solo libreria standard.
"""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BIN_PATH = REPO_ROOT / "bin" / "melting-gen"
PROMPTS_DIR = REPO_ROOT / "tools" / "melting-gen" / "prompts"
OUT_DIR = REPO_ROOT / "dataset" / "qlora"

ATTEMPT_SEED_STRIDE = 7919
BAD_LUA_OUTCOMES = {"invalid", "decode-failed"}


def relpath(p):
    try:
        return str(p.resolve().relative_to(REPO_ROOT))
    except ValueError:
        return str(p)


def discover_corpus_files(explicit):
    """Percorsi espliciti se dati, altrimenti tutti i corpus noti."""
    if explicit:
        return [Path(p) for p in explicit]
    files = sorted(REPO_ROOT.glob("logs/gen-corpus/*.jsonl"))
    files += sorted(REPO_ROOT.glob("logs/gen-metrics/*/corpus-*.jsonl"))
    return files


def load_records(path):
    """Righe JSON valide di un file corpus. Le righe rotte vengono segnalate
    e saltate: un corpus scritto durante un SIGTERM puo' avere l'ultima riga
    troncata (vedi gen_corpus.c, flush per riga ma nessuna garanzia sulla
    scrittura finale)."""
    recs = []
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as e:
        print(f"AVVISO: impossibile leggere {path}: {e}", file=sys.stderr)
        return recs
    for lineno, line in enumerate(text.splitlines(), start=1):
        line = line.strip()
        if not line:
            continue
        try:
            recs.append(json.loads(line))
        except json.JSONDecodeError as e:
            print(f"AVVISO: {path}:{lineno} non e' JSON valido, salto: {e}", file=sys.stderr)
    return recs


def freshest_prompt_mtime():
    mtimes = [p.stat().st_mtime for p in PROMPTS_DIR.glob("*") if p.is_file()]
    return max(mtimes) if mtimes else None


def reconstruct_prompt_cached(cache, seed):
    """--print-json-prompt non carica modelli (vedi main.c): sicuro da
    eseguire quante volte serve. Cache per attemptSeed, i duplicati sono
    frequenti (piu' run con lo stesso seed base)."""
    if seed in cache:
        return cache[seed]
    if not BIN_PATH.exists():
        cache[seed] = None
        return None
    try:
        result = subprocess.run(
            [str(BIN_PATH), "--print-json-prompt", "--seed", str(seed)],
            cwd=REPO_ROOT, capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError) as e:
        print(f"AVVISO: --print-json-prompt --seed {seed} fallito: {e}", file=sys.stderr)
        cache[seed] = None
        return None
    if result.returncode != 0:
        print(f"AVVISO: --print-json-prompt --seed {seed} exit={result.returncode}: "
              f"{result.stderr.strip()}", file=sys.stderr)
        cache[seed] = None
        return None
    cache[seed] = result.stdout
    return result.stdout


def build_manifest_pairs(files, stats):
    records = []
    stats["manifest_ok_total"] = 0
    stats["manifest_discarded_stale"] = 0
    stats["manifest_discarded_no_raw"] = 0
    stats["manifest_discarded_no_prompt"] = 0

    if not BIN_PATH.exists():
        print(f"AVVISO: {BIN_PATH} assente, nessun manifest_pairs ricostruibile "
              f"(serve per --print-json-prompt)", file=sys.stderr)
        return records

    threshold = freshest_prompt_mtime()
    prompt_cache = {}

    for f in files:
        try:
            file_mtime = f.stat().st_mtime
        except OSError:
            file_mtime = None
        fresh = threshold is None or (file_mtime is not None and file_mtime >= threshold)

        last_model = None
        for rec in load_records(f):
            kind = rec.get("kind")
            if kind == "session":
                last_model = rec.get("model")
                continue
            if kind != "manifest" or rec.get("ok") is not True:
                continue
            stats["manifest_ok_total"] += 1

            if not fresh:
                stats["manifest_discarded_stale"] += 1
                continue

            raw = rec.get("raw")
            if not raw:
                stats["manifest_discarded_no_raw"] += 1
                continue

            seed = rec.get("seed")
            attempt = rec.get("attempt", 1)
            attempt_seed = (seed + (attempt - 1) * ATTEMPT_SEED_STRIDE) & 0xFFFFFFFF

            prompt = reconstruct_prompt_cached(prompt_cache, attempt_seed)
            if not prompt:
                stats["manifest_discarded_no_prompt"] += 1
                continue

            records.append({
                "prompt": prompt,
                "completion": raw,
                "seed": seed,
                "model": last_model,
                "attempt": attempt,
                "source_file": relpath(f),
            })
    return records


def build_lua_valid(files, stats):
    records = []
    seen_hashes = set()
    stats["lua_ok_total"] = 0
    stats["lua_valid_duplicates"] = 0

    for f in files:
        for rec in load_records(f):
            if rec.get("kind") != "lua" or rec.get("outcome") != "ok":
                continue
            stats["lua_ok_total"] += 1
            script = rec.get("script") or ""
            h = hashlib.sha256(script.encode("utf-8")).hexdigest()
            if h in seen_hashes:
                stats["lua_valid_duplicates"] += 1
                continue
            seen_hashes.add(h)
            records.append({
                "theme": rec.get("theme", ""),
                "item": rec.get("item", ""),
                "statUp": rec.get("statUp", False),
                "script": script,
                "seed": rec.get("seed"),
            })
    return records


def build_lua_repairs(files, stats):
    records = []
    stats["lua_repairs_pairs_found"] = 0
    stats["lua_repairs_discarded_no_bad_script"] = 0

    for f in files:
        by_key = {}
        for rec in load_records(f):
            if rec.get("kind") != "lua":
                continue
            key = (rec.get("theme", ""), rec.get("item", ""))
            by_key.setdefault(key, []).append(rec)

        for (theme, item), group in by_key.items():
            group.sort(key=lambda r: r.get("attempt", 0))
            for i in range(len(group) - 1):
                cur, nxt = group[i], group[i + 1]
                if cur.get("outcome") not in BAD_LUA_OUTCOMES:
                    continue
                if nxt.get("outcome") != "ok":
                    continue
                if nxt.get("attempt", 0) != cur.get("attempt", 0) + 1:
                    continue
                stats["lua_repairs_pairs_found"] += 1
                bad_script = cur.get("script")
                if not bad_script:
                    stats["lua_repairs_discarded_no_bad_script"] += 1
                    continue
                records.append({
                    "theme": theme,
                    "item": item,
                    "statUp": nxt.get("statUp", cur.get("statUp", False)),
                    "bad_script": bad_script,
                    "error": cur.get("error", ""),
                    "good_script": nxt.get("script", ""),
                })
    return records


def write_jsonl(path, records):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")


def print_summary(files, manifest_pairs, lua_valid, lua_repairs, stats):
    print(f"== dataset QLoRA da {len(files)} file corpus ==\n")

    print("-- a. manifest_pairs (prompt/completion) --")
    print(f"  manifest ok=true trovati: {stats['manifest_ok_total']}")
    print(f"  scartati (corpus non fresco vs tools/melting-gen/prompts/): "
          f"{stats['manifest_discarded_stale']}")
    print(f"  scartati (senza raw nel record): {stats['manifest_discarded_no_raw']}")
    print(f"  scartati (--print-json-prompt fallito o binario assente): "
          f"{stats['manifest_discarded_no_prompt']}")
    print(f"  esempi scritti: {len(manifest_pairs)}")
    print()

    print("-- b. lua_valid (script deduplicati) --")
    print(f"  record outcome=ok trovati: {stats['lua_ok_total']}")
    print(f"  duplicati (stesso sha256 dello script): {stats['lua_valid_duplicates']}")
    print(f"  esempi scritti: {len(lua_valid)}")
    print()

    print("-- c. lua_repairs (errore -> correzione) --")
    print(f"  coppie invalid/decode-failed -> ok trovate: {stats['lua_repairs_pairs_found']}")
    print(f"  scartate (decode-failed senza script da mostrare): "
          f"{stats['lua_repairs_discarded_no_bad_script']}")
    print(f"  esempi scritti: {len(lua_repairs)}")
    print()


def run(args):
    files = discover_corpus_files(args.corpus_files)
    if not files:
        print("nessun file corpus trovato (ne' in logs/gen-corpus, ne' in "
              "logs/gen-metrics/*/corpus-*.jsonl, ne' negli argomenti)", file=sys.stderr)
        return 1

    stats = {}
    manifest_pairs = build_manifest_pairs(files, stats)
    lua_valid = build_lua_valid(files, stats)
    lua_repairs = build_lua_repairs(files, stats)

    if args.cmd == "build":
        write_jsonl(OUT_DIR / "manifest_pairs.jsonl", manifest_pairs)
        write_jsonl(OUT_DIR / "lua_valid.jsonl", lua_valid)
        write_jsonl(OUT_DIR / "lua_repairs.jsonl", lua_repairs)
        print(f"scritti in {relpath(OUT_DIR)}/\n")

    print_summary(files, manifest_pairs, lua_valid, lua_repairs, stats)
    return 0


USAGE = """uso: corpus_to_dataset.py [build|stats] [percorso-corpus ...]

  build (default)  scrive dataset/qlora/{manifest_pairs,lua_valid,lua_repairs}.jsonl + riepilogo
  stats             solo il riepilogo, non scrive nulla

Senza percorsi espliciti, scandisce logs/gen-corpus/*.jsonl e
logs/gen-metrics/*/corpus-*.jsonl. Il sottocomando e' opzionale: se il primo
argomento non e' "build" ne' "stats", tutti gli argomenti vengono trattati
come percorsi corpus e il comando resta "build"."""


def parse_args(argv):
    """Niente subparser di argparse apposta: un parser a sottocomandi
    tratterebbe un percorso corpus come primo argomento come un comando
    sconosciuto e uscirebbe con errore invece di capire che il comando e'
    implicito ("build"). Qui il sottocomando e' opzionale per costruzione."""
    if argv and argv[0] in ("-h", "--help"):
        print(USAGE)
        sys.exit(0)
    if argv and argv[0] in ("build", "stats"):
        return argparse.Namespace(cmd=argv[0], corpus_files=argv[1:])
    return argparse.Namespace(cmd="build", corpus_files=argv)


def main():
    ns = parse_args(sys.argv[1:])
    return run(ns)


if __name__ == "__main__":
    sys.exit(main())
