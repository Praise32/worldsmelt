#!/usr/bin/env python3
"""Report della suite di comparazione modelli (docs/plans/active/model-comparison.md,
sessione di decisione 22/07/2026). Chiamato da scripts/model-comparison.sh a fine
suite, MAI a mano su una cartella incompleta (si aspetta lo scheletro scritto
dalla shell: meta.txt/timing.txt/manifest-*.json/corpus-*.jsonl per ogni
sottocartella modello).

Riusa i mattoni di gen_metrics.py (stessa cartella) invece di reinventarli:
load_runs/lua_stats/category_values/word_report leggono ESATTAMENTE lo stesso
schema di manifest+corpus che scrive melting-gen, qui e in scripts/gen-metrics.sh.
Quello che aggiunge questo file e' il livello SOPRA la singola run: confronto
FRA MODELLI (dimensione, tok/s dal bench, tempo di parete, tasso di fallback)
che gen_metrics.py non calcola (lui misura un modello alla volta).

Uso: python3 scripts/model_comparison_report.py <logs/model-comparison/STAMP>
Scrive <dir>/report.md e <dir>/report.csv, stampa un riassunto su stdout.
"""

import csv
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_metrics  # noqa: E402  (stessa cartella, vedi commento sopra)

GEN_FLOORS = 5  # tools/melting-gen/melting_gen.h -- guardia anti-fotocopia

# Stessa lista di test-llm.sh (DEC-052, contenuti inglese-first): parole-
# funzione italiane a CONFINE DI PAROLA sui campi generati dal modello.
# Duplicata qui per lo stesso motivo di gen_metrics.py:STOPWORDS -- nessun
# punto unico di verita' condiviso fra shell/Python/C in questo repo.
ITALIAN_WORD_RE = re.compile(r"\b(del|della|dei|degli|delle|di|e)\b", re.IGNORECASE)


def read_kv(path):
    """meta.txt/timing.txt: righe 'chiave=valore', spazi separano piu' coppie
    sulla stessa riga (timing.txt ne scrive piu' d'una per riga, una per seed)."""
    out = []
    if not path.exists():
        return out
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        d = {}
        for tok in line.split():
            if "=" in tok:
                k, v = tok.split("=", 1)
                d[k] = v
        if d:
            out.append(d)
    return out


def fnum(d, k, default=None):
    v = d.get(k)
    if v is None or v == "":
        return default
    try:
        return float(v)
    except ValueError:
        return default


def json_attempt_stats(corpus):
    """Dal corpus (kind='manifest', un record per tentativo JSON): esito al
    primo colpo e numero di tentativi usati sull'ULTIMO gruppo scritto (una
    run puo' avere piu' 'sessioni' se ripetuta, ma qui una sola run = un solo
    gruppo di tentativi consecutivi attempt=1,2,...)."""
    attempts = [r for r in corpus if r.get("kind") == "manifest"]
    attempts.sort(key=lambda r: r.get("attempt", 0))
    if not attempts:
        return None
    firstOk = bool(attempts[0].get("ok")) if attempts[0].get("attempt") == 1 else None
    usedAttempts = len(attempts)
    everOk = any(r.get("ok") for r in attempts)
    return {"firstOk": firstOk, "attempts": usedAttempts, "everOk": everOk}


def theme_adherence(manifest):
    """Euristica di sovrapposizione lessicale (NON semantica): quante delle
    stringhe generate dentro un piano (colpo/boss/stanza/nemici/oggetti)
    condividono ALMENO una parola-contenuto col tema di quel piano. E' un
    proxy grezzo -- un nome puo' aderire al tema senza condividerne una
    parola -- ma e' l'unico segnale automatizzabile senza un giudice LLM
    (fuori scope di questo harness); il campione testuale nel report resta
    il modo per giudicare a occhio i casi dubbi."""
    hits, total = 0, 0
    for f in manifest.get("floors", []):
        themeWords = {w for w in gen_metrics.norm(f.get("theme", "")).split()
                      if w not in gen_metrics.STOPWORDS and len(w) >= 3}
        if not themeWords:
            continue
        names = [
            (f.get("shot") or {}).get("name", ""),
            (f.get("bossType") or {}).get("name", ""),
            (f.get("room") or {}).get("name", ""),
        ]
        names += [e.get("name", "") for e in (f.get("enemies") or [])]
        names += [i.get("name", "") for i in (f.get("items") or [])]
        for nm in names:
            if not nm:
                continue
            total += 1
            nmWords = {w for w in gen_metrics.norm(nm).split()
                       if w not in gen_metrics.STOPWORDS and len(w) >= 3}
            if nmWords & themeWords:
                hits += 1
    return hits, total


def english_hits(manifest):
    """Guardia automatica (stesso pattern di scripts/test-llm.sh): quante
    stringhe generate contengono una parola-funzione italiana a confine di
    parola. 0 e' l'atteso; un numero positivo va guardato a mano (campione
    stampato a parte, il conteggio da solo non basta a giudicare la lingua)."""
    n = 0
    for f in manifest.get("floors", []):
        vals = [f.get("theme", ""), f.get("style", ""),
                (f.get("shot") or {}).get("name", ""),
                (f.get("bossType") or {}).get("name", ""),
                (f.get("room") or {}).get("name", "")]
        vals += [e.get("name", "") for e in (f.get("enemies") or [])]
        vals += [i.get("name", "") for i in (f.get("items") or [])]
        for v in vals:
            if v and ITALIAN_WORD_RE.search(v):
                n += 1
    return n


def photocopy_runs(model_runs):
    """Quante delle run di QUESTO modello hanno meno di GEN_FLOORS temi
    distinti fra i suoi piani -- la stessa guardia anti-fotocopia di
    scripts/test-llm.sh, qui applicata per contare i casi, non per fallire."""
    bad = 0
    for r in model_runs:
        themes = [gen_metrics.norm(f.get("theme", "")) for f in r["manifest"].get("floors", [])]
        if len(set(t for t in themes if t)) < GEN_FLOORS:
            bad += 1
    return bad


def variety_summary(model_runs):
    """Stesso calcolo di gen_metrics.py sezione 2 (uniq/dup/jaccard), ma
    condensato in un unico numero per la tabella comparativa: la media del
    jaccard fra coppie di run sulle 6 categorie (piu' basso = piu' varieta',
    0 = nessuna sovrapposizione, 1 = run fotocopia). Il dettaglio per
    categoria resta nel report esteso, non solo questo numero."""
    if len(model_runs) < 2:
        return None, {}
    per_run = [gen_metrics.category_values(r["manifest"]) for r in model_runs]
    cats = ("temi", "colpi", "nemici", "boss", "stanze", "oggetti")
    detail = {}
    jaccards = []
    for cat in cats:
        pairs, overlap = 0, 0.0
        for i in range(len(per_run)):
            for j in range(i + 1, len(per_run)):
                a, b = set(per_run[i][cat]), set(per_run[j][cat])
                if a or b:
                    overlap += len(a & b) / len(a | b)
                    pairs += 1
        jac = overlap / pairs if pairs else 0.0
        detail[cat] = jac
        jaccards.append(jac)
    return sum(jaccards) / len(jaccards) if jaccards else None, detail


def load_model(mdir):
    """Un blocco per sottocartella modello (nome = basename del .gguf senza
    estensione, scritto da scripts/model-comparison.sh)."""
    name = mdir.name
    meta_rows = read_kv(mdir / "meta.txt")
    meta = meta_rows[0] if meta_rows else {}
    loadOk = fnum(meta, "loadOk", 0) == 1
    sizeBytes = fnum(meta, "sizeBytes", 0) or 0
    entry = {
        "name": name,
        "sizeBytes": sizeBytes,
        "loadOk": loadOk,
        "benchTokS": fnum(meta, "benchTokS"),
        "benchLoadS": fnum(meta, "benchLoadS"),
        "note": "",
    }
    if not loadOk:
        # Ultime righe non vuote del log del bench: di solito bastano a capire
        # se e' un'architettura non supportata dal tag pinnato o un
        # esaurimento VRAM (vedi AGENTS.md sul llama.cpp pinnato b9979).
        benchLog = mdir / "bench.log"
        tail = ""
        if benchLog.exists():
            lines = [l for l in benchLog.read_text().splitlines() if l.strip()]
            tail = " | ".join(lines[-3:])
        entry["note"] = f"non carica: {tail}" if tail else "non carica (nessun dettaglio nel log)"
        return entry

    timing = read_kv(mdir / "timing.txt")
    wallSecsOk = [fnum(t, "wallSecs") for t in timing if t.get("failed") != "1" and fnum(t, "wallSecs") is not None]
    nFailedRuns = sum(1 for t in timing if t.get("failed") == "1")
    entry["runsAttempted"] = len(timing)
    entry["runsFailed"] = nFailedRuns
    entry["avgWallSecs"] = sum(wallSecsOk) / len(wallSecsOk) if wallSecsOk else None

    runs = gen_metrics.load_runs(mdir)
    entry["runsWithManifest"] = len(runs)
    model_runs = [r for r in runs if r["manifest"].get("source", "?").startswith("local:")]
    entry["runsFromModel"] = len(model_runs)
    entry["runsFellBackToProcedural"] = len(runs) - len(model_runs)

    # -- validita' JSON (primo colpo / mai riuscito) -------------------------
    firstOkCount, everOkCount, attemptsUsed = 0, 0, []
    for r in runs:
        st = json_attempt_stats(r["corpus"])
        if st is None:
            continue
        if st["firstOk"]:
            firstOkCount += 1
        if st["everOk"]:
            everOkCount += 1
        attemptsUsed.append(st["attempts"])
    entry["jsonRunsWithData"] = len(attemptsUsed)
    entry["jsonFirstOkPct"] = 100.0 * firstOkCount / len(attemptsUsed) if attemptsUsed else None
    entry["jsonEverOkPct"] = 100.0 * everOkCount / len(attemptsUsed) if attemptsUsed else None
    entry["jsonAvgAttempts"] = sum(attemptsUsed) / len(attemptsUsed) if attemptsUsed else None

    # -- validita' Lua (riusa gen_metrics.lua_stats, sommato su tutte le run) --
    luaTotal = Counter()
    for r in runs:
        luaTotal.update(gen_metrics.lua_stats(r["corpus"]))
    nObjs = luaTotal.pop("oggetti", 0)
    entry["luaObjects"] = nObjs
    if nObjs:
        entry["luaFirstShotPct"] = 100.0 * luaTotal.get("primo colpo", 0) / nObjs
        entry["luaOkOverallPct"] = 100.0 * (luaTotal.get("primo colpo", 0) + luaTotal.get("dopo retry", 0)) / nObjs
        entry["luaFallbackPct"] = 100.0 * luaTotal.get("ripiegato (mini-VM)", 0) / nObjs
    else:
        entry["luaFirstShotPct"] = entry["luaOkOverallPct"] = entry["luaFallbackPct"] = None

    # -- varieta' fra le run di QUESTO modello -------------------------------
    jacAvg, jacDetail = variety_summary(model_runs)
    entry["varietyJaccardAvg"] = jacAvg
    entry["varietyDetail"] = jacDetail

    # -- fotocopie / aderenza al tema / guardia inglese ----------------------
    entry["photocopyRuns"] = photocopy_runs(model_runs)
    themeHits, themeTotal = 0, 0
    englishHitsTotal = 0
    samples = []
    for r in model_runs:
        h, t = theme_adherence(r["manifest"])
        themeHits += h
        themeTotal += t
        englishHitsTotal += english_hits(r["manifest"])
        floors = r["manifest"].get("floors", [])
        if floors and len(samples) < 2:
            themes = " | ".join(f.get("theme", "?") for f in floors)
            samples.append(f"seed {r['seed']}: {themes}")
    entry["themeAdherencePct"] = 100.0 * themeHits / themeTotal if themeTotal else None
    entry["englishHits"] = englishHitsTotal
    entry["samples"] = samples

    return entry


def gib(nbytes):
    return nbytes / (1024 ** 3) if nbytes else 0.0


def fmt(v, spec="{:.1f}", suffix=""):
    return (spec.format(v) + suffix) if v is not None else "n/d"


# -- punteggio composito (documentato qui, non un magic number nascosto) ----
# Pesi scelti per riflettere le priorita' del piano (varieta' e Lua-primo-
# colpo sono il valore centrale del generatore, vedi memoria
# "shot-types-ai-generated" e AGENTS.md su ScriptItems): validita' Lua al
# primo colpo 35%, JSON al primo colpo 25%, varieta' (1-jaccard) 25%,
# aderenza al tema 15%. Tok/s e dimensione NON entrano nel punteggio di
# qualita' (sono l'asse "rapporto qualita'/dimensione" separato sotto) --
# mescolarli renderebbe il punteggio un indice di velocita' travestito da
# indice di qualita'.
def quality_score(e):
    if not e.get("loadOk") or e.get("runsFromModel", 0) == 0:
        return None
    parts = []
    if e.get("luaFirstShotPct") is not None:
        parts.append(0.35 * e["luaFirstShotPct"])
    if e.get("jsonFirstOkPct") is not None:
        parts.append(0.25 * e["jsonFirstOkPct"])
    if e.get("varietyJaccardAvg") is not None:
        parts.append(0.25 * 100.0 * (1.0 - e["varietyJaccardAvg"]))
    if e.get("themeAdherencePct") is not None:
        parts.append(0.15 * e["themeAdherencePct"])
    return sum(parts) if parts else None


def meets_acceptable(e):
    """Soglie del piano (docs/plans/active/model-comparison.md): Lua validi
    >=70%, JSON al primo colpo SEMPRE (nessun tentativo 2 e nessun ripiego
    procedurale nel campione), niente piani fotocopia."""
    if not e.get("loadOk") or e.get("runsFromModel", 0) == 0:
        return False
    lua_ok = (e.get("luaOkOverallPct") or 0) >= 70.0
    json_ok = (e.get("jsonFirstOkPct") == 100.0) and e.get("runsFellBackToProcedural", 1) == 0
    no_photocopy = e.get("photocopyRuns", 1) == 0
    return lua_ok and json_ok and no_photocopy


def main():
    if len(sys.argv) < 2:
        print("uso: model_comparison_report.py <logs/model-comparison/STAMP>", file=sys.stderr)
        return 1
    outdir = Path(sys.argv[1])
    modelDirs = sorted(p for p in outdir.iterdir() if p.is_dir())
    if not modelDirs:
        print(f"nessuna sottocartella modello in {outdir}", file=sys.stderr)
        return 1

    entries = [load_model(d) for d in modelDirs]
    for e in entries:
        e["qualityScore"] = quality_score(e)
        e["acceptable"] = meets_acceptable(e)

    loaded = [e for e in entries if e["loadOk"] and e.get("runsFromModel", 0) > 0]
    failed = [e for e in entries if not e["loadOk"]]

    best_overall = max(loaded, key=lambda e: e["qualityScore"] or -1, default=None)
    best_ratio = max(
        loaded,
        key=lambda e: (e["qualityScore"] or 0) / gib(e["sizeBytes"]) if gib(e["sizeBytes"]) > 0 else -1,
        default=None,
    )
    acceptable = [e for e in loaded if e["acceptable"]]
    smallest_acceptable = min(acceptable, key=lambda e: e["sizeBytes"], default=None)

    # -- CSV (la parte "ordinabile": ogni colonna e' un valore, non testo
    #    formattato, cosi' si riordina in un foglio di calcolo o con `sort -t,
    #    -k<N>` senza riparsare il markdown) ----------------------------------
    csvPath = outdir / "report.csv"
    csvCols = ["name", "sizeGiB", "loadOk", "benchTokS", "avgWallSecs",
               "jsonFirstOkPct", "luaFirstShotPct", "luaOkOverallPct",
               "varietyJaccardAvg", "themeAdherencePct", "englishHits",
               "photocopyRuns", "qualityScore", "acceptable"]
    with csvPath.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(csvCols)
        for e in entries:
            w.writerow([
                e["name"], f"{gib(e['sizeBytes']):.2f}", int(e["loadOk"]),
                e.get("benchTokS"), e.get("avgWallSecs"),
                e.get("jsonFirstOkPct"), e.get("luaFirstShotPct"), e.get("luaOkOverallPct"),
                e.get("varietyJaccardAvg"), e.get("themeAdherencePct"), e.get("englishHits"),
                e.get("photocopyRuns"), e.get("qualityScore"), e.get("acceptable"),
            ])

    # -- report.md ------------------------------------------------------------
    lines = []
    lines.append("<!-- GENERATED da scripts/model_comparison_report.py, non modificare a mano -->")
    lines.append("")
    lines.append("# Report suite di comparazione modelli")
    lines.append("")
    lines.append(f"Cartella: `{outdir}` -- vedi anche `report.csv` (stesse colonne, valori grezzi, "
                  "per riordinare in un foglio di calcolo).")
    lines.append("")
    lines.append("## Tabella comparativa")
    lines.append("")
    header = ["Modello", "Dim. (GiB)", "tok/s (bench)", "Tempo/run (s)", "JSON 1° colpo",
              "Lua 1° colpo", "Lua valido (tot.)", "Varietà (jaccard, ↓ meglio)",
              "Aderenza tema", "Guardia EN", "Fotocopie", "Punteggio", "Note"]
    lines.append("| " + " | ".join(header) + " |")
    lines.append("|" + "---|" * len(header))
    for e in sorted(entries, key=lambda e: (not e["loadOk"], -(e["qualityScore"] or -1))):
        if not e["loadOk"]:
            lines.append(f"| {e['name']} | {gib(e['sizeBytes']):.2f} | — | — | — | — | — | — | — | — | — | — | {e['note']} |")
            continue
        note = "OK"
        if e.get("runsFellBackToProcedural"):
            note = f"{e['runsFellBackToProcedural']}/{e['runsAttempted']} run in ripiego procedurale"
        if e.get("runsFailed"):
            note += f"; {e['runsFailed']} run fallite/timeout"
        lines.append("| " + " | ".join([
            e["name"],
            f"{gib(e['sizeBytes']):.2f}",
            fmt(e.get("benchTokS"), "{:.1f}"),
            fmt(e.get("avgWallSecs"), "{:.0f}"),
            fmt(e.get("jsonFirstOkPct"), "{:.0f}", "%"),
            fmt(e.get("luaFirstShotPct"), "{:.0f}", "%"),
            fmt(e.get("luaOkOverallPct"), "{:.0f}", "%"),
            fmt(e.get("varietyJaccardAvg"), "{:.2f}"),
            fmt(e.get("themeAdherencePct"), "{:.0f}", "%"),
            f"{e.get('englishHits', 0)} hit",
            f"{e.get('photocopyRuns', 0)}/{e.get('runsFromModel', 0)}",
            fmt(e.get("qualityScore"), "{:.1f}"),
            note,
        ]) + " |")
    lines.append("")

    lines.append("## Giudizio automatico")
    lines.append("")
    lines.append("Soglie (piano, `docs/plans/active/model-comparison.md`): Lua validi (primo colpo + dopo "
                  "retry) >= 70%, JSON valido al primo tentativo su TUTTE le run campionate (nessun ripiego "
                  "procedurale), zero run con piani fotocopia (< 5 temi distinti su 5 piani).")
    lines.append("")
    if best_overall:
        lines.append(f"- **Migliore complessivo**: `{best_overall['name']}` "
                      f"(punteggio {fmt(best_overall['qualityScore'], '{:.1f}')}/100).")
    else:
        lines.append("- **Migliore complessivo**: nessun modello ha prodotto run valide dal modello.")
    if best_ratio:
        r = (best_ratio["qualityScore"] or 0) / gib(best_ratio["sizeBytes"])
        lines.append(f"- **Migliore rapporto qualità/dimensione**: `{best_ratio['name']}` "
                      f"({r:.1f} punti/GiB).")
    if smallest_acceptable:
        lines.append(f"- **Più piccolo accettabile**: `{smallest_acceptable['name']}` "
                      f"({gib(smallest_acceptable['sizeBytes']):.2f} GiB, sopra tutte e tre le soglie).")
    else:
        lines.append("- **Più piccolo accettabile**: nessun modello supera tutte e tre le soglie nel campione misurato.")
    if failed:
        lines.append("")
        lines.append(f"- {len(failed)} modelli NON caricano col tag llama.cpp pinnato o non entrano in VRAM "
                      "(dettaglio nella tabella e nella sezione sotto), esclusi dal giudizio.")
    lines.append("")

    lines.append("## Dettaglio per modello")
    for e in entries:
        lines.append("")
        lines.append(f"### {e['name']}")
        lines.append("")
        lines.append(f"- Dimensione file: {gib(e['sizeBytes']):.2f} GiB")
        if not e["loadOk"]:
            lines.append(f"- **Non carica**: {e['note']}")
            continue
        lines.append(f"- Bench: tok/s={fmt(e.get('benchTokS'), '{:.2f}')}, "
                      f"caricamento={fmt(e.get('benchLoadS'), '{:.2f}')}s")
        lines.append(f"- Run campionate: {e.get('runsAttempted', 0)} "
                      f"({e.get('runsFromModel', 0)} dal modello, "
                      f"{e.get('runsFellBackToProcedural', 0)} in ripiego procedurale, "
                      f"{e.get('runsFailed', 0)} fallite/timeout)")
        lines.append(f"- Tempo medio per run completa: {fmt(e.get('avgWallSecs'), '{:.0f}')}s")
        lines.append(f"- JSON: primo colpo {fmt(e.get('jsonFirstOkPct'), '{:.0f}', '%')}, "
                      f"riuscito comunque {fmt(e.get('jsonEverOkPct'), '{:.0f}', '%')}, "
                      f"tentativi medi {fmt(e.get('jsonAvgAttempts'), '{:.2f}')}")
        lines.append(f"- Lua ({e.get('luaObjects', 0)} oggetti totali): primo colpo "
                      f"{fmt(e.get('luaFirstShotPct'), '{:.0f}', '%')}, "
                      f"valido comunque {fmt(e.get('luaOkOverallPct'), '{:.0f}', '%')}, "
                      f"ripiegato su mini-VM {fmt(e.get('luaFallbackPct'), '{:.0f}', '%')}")
        if e.get("varietyDetail"):
            detail = ", ".join(f"{k} {v:.2f}" for k, v in e["varietyDetail"].items())
            lines.append(f"- Varietà per categoria (jaccard medio fra run, ↓ meglio): {detail}")
        lines.append(f"- Aderenza al tema (euristica lessicale): {fmt(e.get('themeAdherencePct'), '{:.0f}', '%')}")
        lines.append(f"- Guardia inglese: {e.get('englishHits', 0)} parole-funzione italiane trovate "
                      "(0 atteso, DEC-052)")
        lines.append(f"- Fotocopie: {e.get('photocopyRuns', 0)}/{e.get('runsFromModel', 0)} run con < "
                      f"{GEN_FLOORS} temi distinti")
        if e.get("samples"):
            lines.append("- Campione temi (giudicare l'inglese a occhio):")
            for s in e["samples"]:
                lines.append(f"  - {s}")

    reportPath = outdir / "report.md"
    reportPath.write_text("\n".join(lines) + "\n")

    # -- riassunto su stdout (anche in report.txt via tee dal chiamante) -----
    print(f"== report scritto in {reportPath} (+ {csvPath}) ==")
    for e in sorted(entries, key=lambda e: (not e["loadOk"], -(e["qualityScore"] or -1))):
        if not e["loadOk"]:
            print(f"  {e['name']:40s} NON CARICA -- {e['note']}")
        else:
            print(f"  {e['name']:40s} score={fmt(e['qualityScore'], '{:.1f}')} "
                  f"lua1={fmt(e.get('luaFirstShotPct'), '{:.0f}')}% "
                  f"json1={fmt(e.get('jsonFirstOkPct'), '{:.0f}')}% "
                  f"tokS={fmt(e.get('benchTokS'), '{:.1f}')} "
                  f"acceptable={e['acceptable']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
