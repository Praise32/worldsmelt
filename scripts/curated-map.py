#!/usr/bin/env python3
"""Costruisce assets/curated-content/image-map.txt (W5b, DEC-153).

Layer di INDIREZIONE fra il content-id di una voce del pool curato
(assets/curated-content/{items,enemies,bosses}.txt, vedi
src/content/curated_catalog.h) e l'image-id del manifest immagini
(assets/curated/manifest.json, vedi src/content/curated_images.h). Il motore
(src/content/curated_image_map.h) risolve un'immagine SOLO passando da questo
file: mai un content-id usato direttamente come image-id.

Questo script fa SOLO auto-mapping banale per tag alla prima costruzione --
nessuna curation estetica, nessun giudizio visivo: per ogni content-id cerca,
fra le immagini della stessa categoria nel manifest NON ANCORA assegnate a un
altro content-id di questa stessa categoria, quella coi TAG piu' in comune
(campo opzionale "<prefix>N.tags=", lista separata da virgole, nel file di
contenuto -- ignorato dal motore, letto SOLO da questo script). A parita' di
punteggio (inclusi tutti zero, cioe' nessun tag in comune) vince la prima
immagine LIBERA della categoria nell'ordine del manifest: deterministico,
riproducibile, mai un giudizio estetico. Solo quando una categoria ha PIU'
content-id che immagini disponibili, le voci in eccesso tornano a ripescare
fra quelle gia' assegnate (un duplicato e' comunque meglio di nessuna
immagine) -- correzione round 0 (minore): senza questa de-duplicazione, ogni
content-id senza tag in comune con nessuna immagine finiva sempre sulla
STESSA prima immagine della categoria. Il risultato e' un punto di partenza
correggibile a mano in seguito (il file prodotto resta testo semplice).

Uso:
    python3 scripts/curated-map.py             # scrive assets/curated-content/image-map.txt
    python3 scripts/curated-map.py --dry-run    # mostra solo i conteggi/le scelte, non scrive nulla

Un file di contenuto assente (items.txt/enemies.txt/bosses.txt) o
assets/curated/manifest.json assente sono entrambi casi NORMALI (nessun
contenuto ancora scritto, o checkout senza il pacchetto immagini): lo script
si limita a saltare quella parte, mai un errore fatale.
"""

import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CONTENT_DIR = REPO_ROOT / "assets" / "curated-content"
IMAGES_MANIFEST = REPO_ROOT / "assets" / "curated" / "manifest.json"
OUT_PATH = CONTENT_DIR / "image-map.txt"

# (prefisso nel file di contenuto, nome file, categoria nel manifest immagini
# -- gli stessi tre nomi "enemie"/"bosse" storpiati di proposito, vedi il
# commento su CuratedImage.category in src/content/curated_images.h: sono i
# nomi ESATTI gia' scritti nel manifest, non ritoccati).
CATEGORIES = [
    ("item", "items.txt", "item"),
    ("enemy", "enemies.txt", "enemie"),
    ("boss", "bosses.txt", "bosse"),
]

RECORD_KEY_RE = re.compile(r"^([a-zA-Z]+)(\d+)\.([a-zA-Z]+)=(.*)$")


def parse_content_file(path: Path):
    """Legge un file <prefix>N.chiave=valore e ritorna una lista di record
    (dict per indice N, in ordine), ognuno con almeno 'id' (puo' essere vuoto
    se la voce non lo dichiara) e 'tags' (lista, vuota se non dichiarati).
    Stesso formato riga-chiave=valore letto da src/content/curated_catalog.c,
    ma qui interessano SOLO "id" e "tags" -- ogni altra chiave e' ignorata. """
    if not path.exists():
        return []
    records = {}
    text = path.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        m = RECORD_KEY_RE.match(line.strip())
        if not m:
            continue
        _prefix, index, field, value = m.groups()
        index = int(index)
        record = records.setdefault(index, {"id": "", "tags": []})
        if field == "id":
            record["id"] = value.strip()
        elif field == "tags":
            record["tags"] = [t.strip().lower() for t in value.split(",") if t.strip()]
    return [records[i] for i in sorted(records.keys())]


def load_images_by_category(manifest_path: Path):
    """Ritorna {categoria: [ (image_id, set(tags)), ... ]} nell'ordine esatto
    del manifest (l'ordine e' quello che decide il pareggio a punteggio zero).
    Manifest assente o malformato -> dizionario vuoto, mai un'eccezione che
    fermi lo script: e' il caso normale di un checkout senza il pacchetto
    immagini curate. """
    if not manifest_path.exists():
        return {}
    try:
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        print(f"curated-map: impossibile leggere {manifest_path} ({exc}), nessuna immagine disponibile", file=sys.stderr)
        return {}

    by_category = {}
    for entry in data.get("images", []):
        image_id = entry.get("id")
        category = entry.get("category")
        if not image_id or not category:
            continue
        tags = {str(t).strip().lower() for t in entry.get("tags", [])}
        by_category.setdefault(category, []).append((image_id, tags))
    return by_category


def best_match(content_tags, candidates, used_ids):
    """candidates: lista [(image_id, tags_set), ...] gia' filtrata per
    categoria. 'used_ids' e' l'insieme degli image-id gia' assegnati ad un
    ALTRO content-id di questa stessa categoria in questa stessa build
    (correzione round 0, minore): si cerca PRIMA fra i candidati LIBERI
    (non in 'used_ids'), e solo se non ne resta nessuno si ricade sull'intera
    lista (un duplicato e' comunque meglio di nessuna immagine, stessa
    filosofia di CuratedImagesPickUnused in src/content/curated_images.c
    quando una categoria e' esaurita). Ritorna l'image_id col punteggio (tag
    in comune) piu' alto; a parita' (incluso zero) vince il PRIMO della lista
    considerata -- max() di Python e' stabile sul primo massimo incontrato,
    che e' esattamente l'ordine del manifest. None se 'candidates' e' vuota."""
    if not candidates:
        return None
    content_tags = set(content_tags)
    unused = [(image_id, tags) for image_id, tags in candidates if image_id not in used_ids]
    pool = unused if unused else candidates
    scored = [(len(content_tags & tags), image_id) for image_id, tags in pool]
    best_score, best_id = max(scored, key=lambda pair: pair[0])
    return best_id, best_score


def build_mapping(dry_run: bool):
    images_by_category = load_images_by_category(IMAGES_MANIFEST)
    if not images_by_category:
        print(f"curated-map: {IMAGES_MANIFEST} assente o vuoto -- nessuna immagine da abbinare", file=sys.stderr)

    lines = []
    total_content = 0
    total_mapped = 0

    for prefix, filename, image_category in CATEGORIES:
        records = parse_content_file(CONTENT_DIR / filename)
        candidates = images_by_category.get(image_category, [])
        used_ids = set()   # per categoria: correzione round 0 (minore), vedi best_match sopra
        for record in records:
            content_id = record["id"]
            if not content_id:
                continue   # voce senza content-id dichiarato: nessuna indirezione possibile
            total_content += 1
            match = best_match(record["tags"], candidates, used_ids)
            if match is None:
                print(f"curated-map: nessuna immagine di categoria '{image_category}' per '{content_id}', nessuna riga scritta", file=sys.stderr)
                continue
            image_id, score = match
            used_ids.add(image_id)
            lines.append(f"{content_id} = {image_id}")
            total_mapped += 1
            print(f"curated-map: {content_id} -> {image_id} (tag in comune: {score})")

    print(f"curated-map: {total_mapped}/{total_content} content-id abbinati", file=sys.stderr)

    if dry_run:
        print("curated-map: --dry-run, nessun file scritto", file=sys.stderr)
        return

    CONTENT_DIR.mkdir(parents=True, exist_ok=True)
    header = (
        "# Generato da scripts/curated-map.py (auto-mapping per tag, nessuna curation\n"
        "# estetica). Modificabile a mano in seguito: righe '#' e vuote ignorate dal\n"
        "# motore (src/content/curated_image_map.h). Formato: <content-id> = <image-id>\n"
    )
    OUT_PATH.write_text(header + "\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    print(f"curated-map: scritto {OUT_PATH}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true", help="mostra solo conteggi/scelte, non scrive nulla")
    args = parser.parse_args()
    build_mapping(dry_run=args.dry_run)


if __name__ == "__main__":
    main()
