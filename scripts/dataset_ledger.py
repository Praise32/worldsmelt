#!/usr/bin/env python3
"""Registro di provenienza del dataset (roadmap 16/07/2026, settimana 2).

Mantiene docs/ai-production/dataset/ledger.jsonl: una riga JSON per file, coi campi del
"Registro obbligatorio" di roguelike-ai-appunti/05-dataset-e-licenze.md.
Vedi docs/ai-production/dataset/README.md per le regole d'oro (solo CC0 verificate o asset
propri, niente output Retro Diffusion senza permesso scritto, niente
scraping, dedup con sha256, split per pack/autore mai per frame).

Solo libreria standard: niente Pillow ne' altre dipendenze. Le dimensioni
delle immagini PNG si leggono a mano dal chunk IHDR (8 byte di firma + 4 di
lunghezza + 4 "IHDR" + width/height a 4 byte big-endian ciascuno): non serve
un decoder immagine completo solo per sapere quanto e' grande un file.

Sottocomandi:
  add <file-o-cartella> --source-url U --license L --license-url LU \
      --author A --role R [--notes N]
      Calcola sha256 (e dimensioni se PNG), aggiunge una riga per ogni file
      trovato (ricorsivo su cartelle). Idempotente: un file con sha256 gia'
      registrato viene segnalato e MAI riscritto.
  check
      Verifica il registro: sha256 duplicati, campi obbligatori mancanti,
      licenze fuori whitelist (CC0/own/commissioned).
  stats
      Conteggi per ruolo, licenza e fonte (dominio di original_url).
"""

import argparse
import datetime
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path
from urllib.parse import urlsplit

REPO_ROOT = Path(__file__).resolve().parent.parent
LEDGER_PATH = REPO_ROOT / "docs" / "ai-production" / "dataset" / "ledger.jsonl"

# Whitelist delle licenze accettate per il corpus principale (05, "Fonti da
# evitare" + "Ordine consigliato"): CC0 verificata, asset proprio, asset
# commissionato con permessi espliciti nel contratto. Tutto il resto (CC-BY,
# CC-BY-SA, CC-NC, CC-ND, "licenza non chiara", output Retro Diffusion senza
# permesso scritto...) va segnalato da `check`, non accettato in silenzio.
LICENSE_WHITELIST = {"cc0", "cc0-1.0", "own", "commissioned"}

# Campi che devono essere valorizzati per ogni riga perche' la provenienza
# sia dimostrabile (sottoinsieme del "Registro obbligatorio" di 05: gli altri
# campi -- perceptual_hash, archive_url, source_pack, animation_group,
# exclusions, transformations, caption, split, reviewer -- restano null
# finche' non vengono compilati a mano o da uno strumento futuro).
REQUIRED_FIELDS = [
    "sha256", "path", "original_url", "author",
    "license_id", "license_url", "role", "license_snapshot_date",
]


def sha256_of_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def png_dimensions(path):
    """Legge width/height dal chunk IHDR di un PNG. None se non e' un PNG
    valido o e' troppo corto: non e' un errore fatale, solo un campo vuoto."""
    try:
        with open(path, "rb") as f:
            header = f.read(8)
            if header != b"\x89PNG\r\n\x1a\n":
                return None
            f.read(4)  # lunghezza del chunk (13 per IHDR, non ci serve)
            chunk_type = f.read(4)
            if chunk_type != b"IHDR":
                return None
            data = f.read(8)
            if len(data) < 8:
                return None
            width, height = struct.unpack(">II", data)
            return {"width": width, "height": height}
    except OSError:
        return None


def relative_path(path):
    """Percorso relativo alla radice del repo quando possibile, altrimenti
    il percorso assoluto: il registro deve restare leggibile anche se
    qualcuno lo esegue da una sottocartella."""
    resolved = path.resolve()
    try:
        return str(resolved.relative_to(REPO_ROOT))
    except ValueError:
        return str(resolved)


def iter_files(target):
    p = Path(target)
    if p.is_dir():
        for f in sorted(p.rglob("*")):
            if f.is_file():
                yield f
    elif p.is_file():
        yield p
    else:
        raise FileNotFoundError(f"ne' file ne' cartella: {target}")


def load_ledger():
    entries = []
    if not LEDGER_PATH.exists():
        return entries
    with LEDGER_PATH.open("r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                entries.append(json.loads(line))
            except json.JSONDecodeError as e:
                print(f"AVVISO: riga {lineno} di {LEDGER_PATH} non e' JSON valido: {e}", file=sys.stderr)
    return entries


def append_entry(entry):
    LEDGER_PATH.parent.mkdir(parents=True, exist_ok=True)
    with LEDGER_PATH.open("a", encoding="utf-8") as f:
        f.write(json.dumps(entry, ensure_ascii=False) + "\n")


def cmd_add(args):
    entries = load_ledger()
    known_sha = {e["sha256"]: e for e in entries if e.get("sha256")}

    try:
        files = list(iter_files(args.target))
    except FileNotFoundError as e:
        print(f"errore: {e}", file=sys.stderr)
        return 2
    if not files:
        print(f"nessun file trovato in {args.target}", file=sys.stderr)
        return 1

    today = datetime.date.today().isoformat()
    now = datetime.datetime.now().isoformat(timespec="seconds")
    added, skipped = 0, 0

    for f in files:
        sha = sha256_of_file(f)
        if sha in known_sha:
            prev = known_sha[sha]
            print(f"AVVISO: {f} ha lo stesso sha256 gia' registrato per "
                  f"'{prev.get('path')}' (asset_id={prev.get('asset_id')}): salto (idempotente)",
                  file=sys.stderr)
            skipped += 1
            continue

        dims = png_dimensions(f) if f.suffix.lower() == ".png" else None
        entry = {
            "asset_id": f"{args.role}:{f.stem}:{sha[:12]}",
            "sha256": sha,
            "perceptual_hash": None,
            "original_url": args.source_url,
            "archive_url": None,
            "author": args.author,
            "license_id": args.license,
            "license_url": args.license_url,
            "license_snapshot_date": today,
            "source_pack": None,
            "role": args.role,
            "dimensions": dims,
            "animation_group": None,
            "exclusions": None,
            "transformations": None,
            "caption": None,
            "split": None,
            "reviewer": None,
            "notes": args.notes,
            "path": relative_path(f),
            "added_at": now,
        }
        append_entry(entry)
        known_sha[sha] = entry
        added += 1
        dims_txt = f", {dims['width']}x{dims['height']}" if dims else ""
        print(f"aggiunto: {entry['path']} (sha256={sha[:12]}...{dims_txt})")

    print(f"-- add: {added} aggiunti, {skipped} gia' presenti (stesso sha256) --")
    return 0


def cmd_check(args):
    del args
    entries = load_ledger()
    problems = 0

    # -- sha256 duplicati: add() li impedisce, ma il file puo' essere stato
    #    modificato a mano o unito con un altro registro.
    seen = {}
    for i, e in enumerate(entries):
        sha = e.get("sha256")
        if not sha:
            continue
        if sha in seen:
            print(f"DUPLICATO: riga {i + 1} ripete lo sha256 della riga {seen[sha] + 1} "
                  f"({e.get('path', '?')} == {entries[seen[sha]].get('path', '?')})")
            problems += 1
        else:
            seen[sha] = i

    # -- campi obbligatori mancanti (provenienza non dimostrabile senza).
    for i, e in enumerate(entries):
        missing = [k for k in REQUIRED_FIELDS if not e.get(k)]
        if missing:
            print(f"CAMPI MANCANTI: riga {i + 1} ({e.get('path', '?')}): {', '.join(missing)}")
            problems += 1

    # -- licenze fuori whitelist.
    for i, e in enumerate(entries):
        lic = (e.get("license_id") or "").strip().lower()
        if lic and lic not in LICENSE_WHITELIST:
            print(f"LICENZA NON IN WHITELIST: riga {i + 1} ({e.get('path', '?')}): "
                  f"'{e.get('license_id')}' (ammesse: {', '.join(sorted(LICENSE_WHITELIST))})")
            problems += 1

    if problems == 0:
        print(f"check OK: {len(entries)} voci, nessun problema")
        return 0
    print(f"check FALLITO: {problems} problemi su {len(entries)} voci")
    return 1


def cmd_stats(args):
    del args
    entries = load_ledger()
    if not entries:
        print(f"registro vuoto o assente ({LEDGER_PATH})")
        return 0

    def source_domain(e):
        url = e.get("original_url") or ""
        netloc = urlsplit(url).netloc
        return netloc or (url or "?")

    by_role = Counter(e.get("role") or "?" for e in entries)
    by_license = Counter(e.get("license_id") or "?" for e in entries)
    by_source = Counter(source_domain(e) for e in entries)

    print(f"== stats: {len(entries)} voci nel registro ({LEDGER_PATH}) ==\n")

    print("-- per ruolo --")
    for role, n in by_role.most_common():
        print(f"  {role:20s} {n:4d}")

    print("\n-- per licenza --")
    for lic, n in by_license.most_common():
        flag = "" if lic.lower() in LICENSE_WHITELIST else "  << fuori whitelist"
        print(f"  {lic:20s} {n:4d}{flag}")

    print("\n-- per fonte (dominio di original_url) --")
    for src, n in by_source.most_common():
        print(f"  {src:30s} {n:4d}")

    return 0


def build_parser():
    parser = argparse.ArgumentParser(
        prog="dataset_ledger.py",
        description="Registro di provenienza del dataset (docs/ai-production/dataset/ledger.jsonl). "
                     "Vedi docs/ai-production/dataset/README.md.")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_add = sub.add_parser("add", help="registra un file o una cartella (ricorsivo)")
    p_add.add_argument("target", help="file o cartella da registrare")
    p_add.add_argument("--source-url", required=True, dest="source_url", help="original_url")
    p_add.add_argument("--license", required=True, help="license_id (whitelist: CC0/own/commissioned)")
    p_add.add_argument("--license-url", required=True, dest="license_url", help="license_url")
    p_add.add_argument("--author", required=True, help="autore o pack di provenienza")
    p_add.add_argument("--role", required=True, help="style_core/item/projectile/vfx/player_mutation/...")
    p_add.add_argument("--notes", default=None, help="annotazione libera (opzionale)")
    p_add.set_defaults(func=cmd_add)

    p_check = sub.add_parser("check", help="verifica duplicati, campi obbligatori e licenze")
    p_check.set_defaults(func=cmd_check)

    p_stats = sub.add_parser("stats", help="conteggi per ruolo/licenza/fonte")
    p_stats.set_defaults(func=cmd_stats)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
