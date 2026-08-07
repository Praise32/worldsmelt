#!/usr/bin/env python3
"""lora_dataset_mine — prova LoRA v1 RESEARCH-ONLY (mandato orchestratore 07/08/2026).

NON e' il dataset commerciale del gioco. E' una prova con provenienza dubbia
ESPLICITAMENTE autorizzata per questo esperimento soltanto (docs/ai-production/
regole-agenti-ml.md, divieto 3: "un asset 'research' non diventa 'commercial-clean'
per decisione estetica"): non distribuire, non promuovere a commerciale senza un
rifacimento pulito delle licenze. dataset-sources/ (sorgenti grezze) e
dataset/lora-v1-research/images/ (derivati) restano fuori da git (vedi .gitignore) —
solo ledger, report e pacchetto Kaggle entrano in cronologia, perche' sono la
provenienza, non i pixel.

REGOLA DI GRANA (reclamo del proprietario sul v0, dove grane diverse — icone 16px
e sprite 64px — finivano nella stessa cartella di training, insegnando due scale
di "pixel" incompatibili nello stesso batch): MAI grane diverse nella stessa
cartella. Quattro sorgenti, quattro grane, quattro cartelle:
  primary-128/          Limbicnation/pixel-art-character (grana fine, ricampionata
                         alla grana stimata per-immagine, canvas comune 128)
  secondary-dcss-32/     OGA Dungeon Crawl 32x32 (nativo 32, nessun ricampionamento)
  secondary-tinyhero-64/ TinyHero generator, sola direzione fronte (nativo 64)
  secondary-nouns-32/    pixel-art-nouns-2k (nativo 32 dentro un JPEG 320px)

Pipeline per immagine (ordine fisso, ogni scarto registrato — anche i sopravvissuti
passano tutti gli stessi gate):
  a. integrita'/formato -> RGBA
  b. GRANA: round-trip NEAREST downscale/upscale, MSE minima -> grana stimata
     (registrata SEMPRE, anche sugli scarti: e' un dato di censimento, non solo
     un gate)
  c. soggetto singolo: isolamento sfondo (trasparenza nativa se c'e' gia', altrimenti
     flood-fill dalla moda di cornice — RIUSA teacher_bench_post.flood_remove_background,
     non lo riscrive) -> 1 componente dominante, foreground 8-60%, niente bordo pesante,
     niente aspect estremi
  c2. soggetto singolo sul CANVAS FINALE (non sul frame sorgente): riempimento,
     occupazione della cornice, compattezza e "lastra" orizzontale in basso —
     e' qui che cadono scene/diorami/tilemap/cornici, che tutti i gate al punto c
     lasciano passare perche' sul frame sorgente sono UNA sola componente pulita
  d. qualita': n colori dopo quantizzazione leggera, contrasto e percentuale di
     soggetto impercettibile CONTRO CANVAS_BG_RGB (il fondo su cui lo sprite finisce
     davvero, non quello della sorgente), contenuto >=12px logici
  e. dedup percettivo (dHash 64bit) GLOBALE fra le quattro sorgenti
  f. NSFW (solo Limbicnation): parole chiave della caption (empiricamente un no-op
     su questa sorgente: 0/500) + frazione di incarnato misurata sui pixel del
     canvas — in dubbio, scarta (mandato). Gli id sospetti restano elencati in
     mining_report.json e in nsfw-review.jsonl: la decisione sul bucket e' del
     proprietario, non di questo script

Caption: mai un'asserzione che lo script non ha verificato. Limbicnation ha caption
sintetiche che sono PROMPT di generazione, non descrizioni (misurato: il generatore
non le ha seguite), quindi si buttano e la caption si costruisce dai pixel
(colori dominanti misurati, forma della bbox); DCSS usa il nome file reale;
TinyHero dichiara "front view" solo perche' la direzione tenuta e' verificata.

Dipendenze: stdlib + Pillow + requests (verificate presenti su questa macchina;
pyarrow/pandas/datasets NON ci sono — le due sorgenti HF si scaricano riga per riga
dalla API pubblica datasets-server, non da parquet).

Uscite in dataset/lora-v1-research/: le quattro cartelle di training + ledger.jsonl
(una riga per immagine tenuta), rejects.jsonl (una riga per SCARTO — le esclusioni
devono essere verificabili quanto le inclusioni), nsfw-review.jsonl (scarti
dell'euristica + tenute sopra la soglia di attenzione, per la decisione del
proprietario), mining_report.json, un contact sheet per bucket e il pacchetto Kaggle.

Uso:
  python3 scripts/lora_dataset_mine.py download            # le 4 sorgenti, riprendibile
  python3 scripts/lora_dataset_mine.py mine [--limit N]     # filtri + assemblaggio
  python3 scripts/lora_dataset_mine.py kaggle                # adatta il pacchetto Kaggle
  python3 scripts/lora_dataset_mine.py all                  # download + mine + kaggle
"""

import argparse
import colorsys
import datetime
import hashlib
import json
import random
import re
import shutil
import subprocess
import sys
import tarfile
import time
from collections import Counter, defaultdict
from pathlib import Path

from PIL import Image, ImageChops, ImageStat

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
# Riuso esplicito (mandato): "flood a moda di cornice" e le sue funzioni di
# supporto vengono da teacher_bench_post.py, non duplicate qui. Il resto del
# modulo (palette Fucina, scene mock, contact sheet giudice) non ci serve:
# questa e' una prova LoRA di ricerca, non il postproc del gioco.
import teacher_bench_post as tbp  # noqa: E402

SOURCES_DIR = REPO_ROOT / "dataset-sources"
OUT_DIR = REPO_ROOT / "dataset" / "lora-v1-research"

# ---------------------------------------------------------------------------
# Configurazione per sorgente: nome cartella dataset-sources/, grana nominale
# (None per Limbicnation: la' la grana si MISURA per immagine, non e' fissa),
# nome del bucket di uscita, licenza dichiarata (vedi mandato: provenienza
# dubbia autorizzata SOLO per questa prova).
# ---------------------------------------------------------------------------
NOMINAL_GRAIN = {"dcss-32": 32, "tinyhero-64": 64, "nouns-32": 32}
SECONDARY_BUCKET = {
    "dcss-32": "secondary-dcss-32",
    "tinyhero-64": "secondary-tinyhero-64",
    "nouns-32": "secondary-nouns-32",
}
PRIMARY_BUCKET = "primary-128"
PRIMARY_CANVAS = 128

LICENSE_INFO = {
    "limbicnation": {
        "license_id": "apache-2.0",
        "license_url": "https://www.apache.org/licenses/LICENSE-2.0",
        "dataset_url": "https://huggingface.co/datasets/Limbicnation/pixel-art-character",
        "author": "Limbicnation (HuggingFace)",
    },
    "dcss-32": {
        "license_id": "cc0-1.0",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "dataset_url": "https://opengameart.org/content/dungeon-crawl-32x32-tiles",
        "author": "Dungeon Crawl Stone Soup contributors (OpenGameArt)",
    },
    "tinyhero-64": {
        "license_id": "cc-by-sa-3.0",
        "license_url": "https://github.com/AgaMiko/pixel_character_generator/blob/master/cc-by-sa-3.0.txt",
        "license_alt": "gpl-3.0 (vedi gpl-3.0.txt nello stesso repo, doppia licenza dichiarata dall'autore)",
        "dataset_url": "https://github.com/AgaMiko/pixel_character_generator",
        "author": "AgaMiko (pixel_character_generator)",
    },
    "nouns-32": {
        "license_id": "cc0-de-facto",
        "license_url": "https://huggingface.co/datasets/jiovine/pixel-art-nouns-2k",
        "dataset_url": "https://huggingface.co/datasets/jiovine/pixel-art-nouns-2k",
        "author": "jiovine (arte Nouns DAO, CC0 di fatto — non una dichiarazione formale del dataset HF)",
    },
}

# ---------------------------------------------------------------------------
# Soglie della pipeline — ognuna misurata o giustificata nel commento, nessuna
# a caso (stesso principio di lora_dataset_build.py: le soglie separano un
# gruppo osservato dal successivo, non sono estetica).
# ---------------------------------------------------------------------------
GRAIN_CANDIDATES = (8, 16, 24, 32, 48, 64, 96, 128, 192, 256)
# Soglia assoluta per un PNG pulito (round-trip quasi lossless su griglia
# uniforme) + margine relativo sul minimo osservato: le sorgenti JPEG (Nouns)
# hanno rumore di compressione che non azzera MAI l'errore neppure alla grana
# vera, quindi la soglia si adatta al rumore di FONDO di quella immagine
# invece di un numero fisso che scarterebbe ogni JPEG.
GRAIN_MSE_ABS_FLOOR = 4.0
GRAIN_MSE_REL_SLACK = 1.35
PRIMARY_GRAIN_MIN = 96  # sotto: nessuna cartella di destinazione per Limbicnation (niente bucket secondario suo)

FOREGROUND_MIN_FRACTION = 0.08   # sotto: "quasi vuota" (mandato, letterale)
FOREGROUND_MAX_FRACTION = 0.60   # sopra: "quasi piena" (mandato, letterale)
DOMINANT_COMPONENT_MIN_SHARE = 0.85  # mandato, letterale
BORDER_CONTACT_MAX = 0.15   # frazione di cornice opaca oltre cui e' "contatto pesante":
                             # 0.0 sarebbe cieco (molte icone legittime toccano un
                             # angolo per un pixel), 1.0 non filtrerebbe niente —
                             # 0.15 lascia passare un bordo sfiorato, non un soggetto
                             # tagliato dalla cornice del frame sorgente.
ASPECT_RATIO_MAX = 6.0      # bbox lato-lungo/lato-corto oltre cui e' "tile/font/barra"
                             # (lance/frecce restano sotto: es. arco 24x77 = 3.2)

# Quantizzazione ADATTIVA (median-cut), non troncamento di bit: misurato su
# campioni reali, un posterize a bit fisso (anche a 5 bit/canale = 32 livelli)
# NON scende mai sotto le migliaia di colori su Limbicnation (mediana 1501) e
# Nouns (mediana 1053) — sono pixel-art "stile AI", con ombreggiatura morbida
# dentro ogni blocco, non palette piatte come DCSS/TinyHero (mediana 11-28
# anche SENZA quantizzare). Un tetto fisso a 256 dopo posterize scartava
# percio' il 100% di Limbicnation/Nouns nello smoke test: la quantizzazione
# leggera deve trovare la MIGLIOR tavolozza a N colori (median-cut), non
# troncare bit a caso — e' quello che rende il conteggio "sensato per pixel
# art" a prescindere dallo stile della sorgente.
QUALITY_TARGET_COLORS = 64
QUALITY_MIN_COLORS = 4       # sotto: soggetto piatto/monocolore, sospetto di render rotto
QUALITY_MAX_COLORS = QUALITY_TARGET_COLORS  # per costruzione (quantizza A questo tetto):
                                             # il vincolo reale e' il minimo, il massimo e'
                                             # una garanzia della quantizzazione stessa
MIN_CONTENT_LOGICAL_PX = 12  # mandato, letterale: lato lungo del contenuto in px LOGICI
CONTRAST_MIN_RATIO = 1.4     # WCAG-style, permissivo apposta: qui serve solo scartare
                             # il soggetto che si confonde col fondo, non leggibilita' da UI
IMPERCEPTIBLE_CONTRAST = 1.15  # sotto questo rapporto un pixel del soggetto e' indistinguibile
                                # dal canvas a occhio: e' il modo in cui una figura chiara passa
                                # il contrasto MEDIO (basta un contorno scuro a tirarlo su) ed
                                # entra comunque nel training come riquadro quasi vuoto
IMPERCEPTIBLE_MAX_FRACTION = 0.50  # oltre meta' del soggetto invisibile sul canvas: lo sprite
                                    # che il modello vedrebbe non e' quello che il ledger dichiara

# --- soggetto singolo sul CANVAS FINALE (bocciatura 07/08: ~29% di primary-128 erano
# scene/diorami/tilemap/cornici, e i gate del punto c non ne vedevano nessuno perche'
# sul frame sorgente sono UNA componente sola, pulita, dentro i limiti di foreground
# e di cornice).
#
# Tre segnali, misurati sul canvas logico e tarati su un insieme etichettato a mano
# di 35 immagini del bucket (9 scene/diorami — i 7 dell'elenco della bocciatura piu'
# limbicnation-00011 e -00036 trovati campionando — contro 26 sprite legittimi):
#   riempimento    scene che SONO il canvas: 00026 .90 / max degli sprite .52
#   cornice        la scena esce dai bordi: 00426 .19 / max degli sprite .15 (il margine
#                  piu' stretto dei quattro: e' l'unico segnale che prende la mappa
#                  dungeon, che per il resto sembra uno sprite grande)
#   bordo dritto   la firma dei diorami isometrici: piattaforme e pavimenti hanno
#                  spigoli RETTI e lunghi (pendenze 1:2, 1:1, 2:1) sul profilo inferiore —
#                  00213 .49, 00036 .49, 00002 .43, 00013 .38, 00165 .29 / max degli
#                  sprite .16 (una silhouette organica non produce un bordo dritto lungo)
#   simmetria alto/basso  cornici, stemmi e tilemap sono speculari rispetto alla
#                  mediana orizzontale: 00026 1.00, 00176 .88 / max degli sprite .68
#                  (un personaggio ha la testa in cima e i piedi in fondo)
#
# Le soglie stanno TUTTE nel vuoto fra i due gruppi, non sul valore del caso peggiore:
# le versioni piu' severe provate prima (tetto su riempimento+compattezza, o sulla
# "lastra" orizzontale in basso) prendevano anche lupi, gatti, robot e busti —
# misurato, non temuto: limbicnation-00009 (lupo), -00035 (gatto), -00003 (robot),
# -00006 (uomo seduto) cadevano con quelle. Cio' che resta fuori dalla rete e' scritto
# nel riepilogo con gli id, non nascosto.
CANVAS_FILL_MAX = 0.62            # oltre: il soggetto E' il canvas (scena/tilemap)
CANVAS_FILL_MIN = 0.06            # sotto: il canvas e' quasi vuoto. E' il gemello sul canvas
                                   # di FOREGROUND_MIN_FRACTION (che guarda il frame SORGENTE) e
                                   # serve perche' l'alpha di questa sorgente e' morbida: le
                                   # figure "fantasma" (chiarissime, contorno sottile) hanno
                                   # un'alpha piena, un contrasto medio alto — bastano pochi
                                   # pixel scuri a tirarlo su — e sul canvas si vedono per
                                   # l'1% (limbicnation-00280 .005, -00432 .012, -00160 .029).
                                   # Nel training sarebbero riquadri vuoti con una caption.
CANVAS_BORDER_OCC_MAX = 0.18      # cornice del canvas occupata: la scena esce dai bordi
CANVAS_STRAIGHT_EDGE_MAX = 0.25   # frazione della larghezza bbox coperta dal piu' lungo
                                   # tratto RETTO del profilo inferiore (piattaforma isometrica)
CANVAS_VSYM_MAX = 0.80            # sovrapposizione della maschera con se stessa ribaltata
                                   # in verticale (cornice/stemma/tilemap)
CANVAS_EDGE_SLOPES = (0.5, -0.5, 1.0, -1.0, 2.0, -2.0)
CANVAS_EDGE_TOLERANCE = 1.0       # scarto max in px dal segmento ideale: 1px assorbe la
                                   # scaletta della pixel art, 2px comincia ad accettare
                                   # profili organici come "dritti"

SKIN_FRACTION_MAX = 0.38     # frazione di pixel VISIBILI del soggetto nella gamma incarnato
                              # oltre cui si scarta (solo Limbicnation). Tarata sui nudi trovati
                              # dal giudice piu' quelli visti campionando: 00215 .47, 00128 .40,
                              # 00021 .55 contro personaggi vestiti a .0-.32. La gamma incarnato
                              # comprende per forza legno, sabbia e terra (stessa tinta): a questa
                              # soglia cadono anche soggetti di legno e la mappa dungeon 00426
                              # (.53). Sono falsi positivi accettati, non ignorati — "in dubbio
                              # scarta" (mandato) — e il motivo registrato dice esattamente cosa
                              # e' stato misurato, non "nudo".

CAPTION_COLOR_MIN_COVERAGE = 0.12   # un colore entra nella caption solo se copre almeno questo
                                     # del soggetto (le caption sintetiche bocciate nominavano
                                     # colori sotto l'8%)
CAPTION_AUDIT_MIN_COVERAGE = 0.08   # soglia dell'audit caption/contenuto nel report: stessa
                                     # usata dal giudice per misurare il difetto

DHASH_DUP_MAX_DIST = 6       # mandato, letterale

NSFW_KEYWORDS = (
    "nude", "naked", "nsfw", "sex", "sexual", "porn", "penis", "vagina",
    "vulva", "breast", "nipple", "genital", "erotic", "hentai", "lewd",
    "fetish", "explicit", "topless", "bdsm", "orgasm", "masturbat", "cum ",
    "gore", "dismember", "beheaded", "bestiality", "incest", "underage",
    "loli", "shota",
)

VAL_TARGET = 0.10
CANVAS_BG_RGB = (238, 236, 230)  # neutro chiaro piatto — NO alpha (SD1.5 non ha canale
                                  # alpha, mandato letterale); volutamente non bianco
                                  # puro per non creare un alone duro coi soggetti chiari
FINAL_PX = 512

CONTACT_SHEET_COLS = 10
CONTACT_SHEET_ROWS = 8
CONTACT_SHEET_CELL = 96
CONTACT_SHEET_SEED = 20260807

SPLIT_SEED = "lora-v1-research-20260807"

REPORT_SAMPLES_PER_REASON = 20


def log(msg):
    print(f"lora_dataset_mine: {msg}", flush=True)


def repo_rel(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(REPO_ROOT))
    except ValueError:
        return str(path.resolve())


def sha256_of_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# =============================================================================
# DOWNLOAD — quattro sorgenti, ognuna riprendibile (salta cio' che c'e' gia').
# Niente pyarrow/pandas/datasets su questa macchina (verificato): le due
# sorgenti HuggingFace si scaricano riga per riga dalla API pubblica
# datasets-server.huggingface.co (JSON coi campi + URL firmato dell'immagine),
# non da parquet.
# =============================================================================

def http_get_json(url, timeout=30, retries=6):
    """datasets-server.huggingface.co applica un rate limit per-minuto duro
    (verificato: 429 dopo ~19 pagine consecutive anche con retry brevi) — un
    backoff da 1.5s*tentativo non basta a farlo scadere. Sul 429 si onora
    Retry-After se presente, altrimenti backoff largo (10s*tentativo); sugli
    altri errori resta il backoff breve originale (rete instabile, non limite)."""
    import requests
    last_exc = None
    for attempt in range(retries):
        try:
            r = requests.get(url, timeout=timeout)
            r.raise_for_status()
            return r.json()
        except requests.exceptions.HTTPError as e:
            last_exc = e
            if e.response is not None and e.response.status_code == 429:
                wait = float(e.response.headers.get("Retry-After", 10 * (attempt + 1)))
                time.sleep(wait)
            else:
                time.sleep(1.5 * (attempt + 1))
        except Exception as e:  # noqa: BLE001 — rete instabile, si ritenta
            last_exc = e
            time.sleep(1.5 * (attempt + 1))
    raise last_exc


def http_download_file(url, dest: Path, timeout=60, retries=3):
    import requests
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    last_exc = None
    for attempt in range(retries):
        try:
            with requests.get(url, timeout=timeout, stream=True) as r:
                r.raise_for_status()
                with open(tmp, "wb") as f:
                    for chunk in r.iter_content(1 << 16):
                        f.write(chunk)
            tmp.rename(dest)
            return
        except Exception as e:  # noqa: BLE001
            last_exc = e
            time.sleep(1.5 * (attempt + 1))
    raise last_exc


def download_hf_dataset_rows(dataset_id, dest_dir: Path, image_ext, caption_field="text",
                              filename_field=None, page_size=100):
    """Scarica un dataset immagini di HuggingFace via l'API pubblica
    datasets-server (rows?dataset=...): nessuna libreria 'datasets'/pyarrow su
    questa macchina, e l'API rows e' l'unica via che non richiede ne' l'una ne'
    l'altra. Ogni riga porta un URL firmato TEMPORANEO dell'immagine (scade in
    ore): va seguito subito, mai salvato per dopo. Riprendibile per indice
    (row_idx): un file gia' presente con la sua caption non viene riscaricato."""
    dest_dir.mkdir(parents=True, exist_ok=True)
    info_path = dest_dir / "_source_info.json"
    base = "https://datasets-server.huggingface.co/rows"

    # Sorgente gia' completa da una corsa precedente: si esce PRIMA di toccare la
    # rete. Senza questo, rilanciare 'download' su una macchina offline (o con
    # datasets-server in rate limit) fallisce su una sorgente che e' gia' tutta
    # su disco — e il comando serve anche solo a riscrivere la provenienza.
    known_total = None
    if info_path.is_file():
        try:
            known_total = json.loads(info_path.read_text(encoding="utf-8")).get("num_rows_total")
        except json.JSONDecodeError:
            known_total = None
    if known_total and all((dest_dir / f"{i:05d}.{image_ext}").is_file()
                           and (dest_dir / f"{i:05d}.txt").is_file()
                           for i in range(known_total)):
        log(f"[{dataset_id}] {known_total} righe gia' complete su disco, salto (nessuna rete)")
        return

    first = http_get_json(f"{base}?dataset={dataset_id}&config=default&split=train&offset=0&length=1")
    total = first["num_rows_total"]
    log(f"[{dataset_id}] {total} righe dichiarate")

    n_downloaded = n_skipped = n_failed = 0
    for offset in range(0, total, page_size):
        # Precheck SENZA rete: row_idx coincide con l'offset di pagina (verificato
        # sull'API rows), quindi se ogni file atteso per questa pagina c'e' gia'
        # si salta la fetch JSON stessa — non solo il download immagine. Un
        # resume su un dataset gia' quasi completo (es. 1726/2000) altrimenti
        # brucia il budget di rate-limit di datasets-server.huggingface.co
        # rifetchando pagine intere di righe gia' presenti (osservato: due 429
        # consecutivi proprio in coda a un resume).
        page_len = min(page_size, total - offset)
        if all((dest_dir / f"{i:05d}.{image_ext}").is_file()
               and (dest_dir / f"{i:05d}.txt").is_file()
               for i in range(offset, offset + page_len)):
            n_skipped += page_len
            continue
        url = f"{base}?dataset={dataset_id}&config=default&split=train&offset={offset}&length={page_size}"
        data = http_get_json(url)
        for row in data["rows"]:
            idx = row["row_idx"]
            img_path = dest_dir / f"{idx:05d}.{image_ext}"
            cap_path = dest_dir / f"{idx:05d}.txt"
            if img_path.is_file() and cap_path.is_file():
                n_skipped += 1
                continue
            r = row["row"]
            img_src = r.get("image", {}).get("src")
            if not img_src:
                n_failed += 1
                log(f"  [{dataset_id}] riga {idx}: nessun campo image.src, salto")
                continue
            try:
                http_download_file(img_src, img_path)
            except Exception as e:  # noqa: BLE001 — una riga fallita non ferma le altre 1999
                n_failed += 1
                log(f"  [{dataset_id}] riga {idx}: download fallito ({e})")
                continue
            caption = (r.get(caption_field) or "").strip()
            cap_path.write_text(caption + "\n", encoding="utf-8")
            if filename_field and r.get(filename_field):
                (dest_dir / f"{idx:05d}.source_name.txt").write_text(
                    r[filename_field], encoding="utf-8")
            n_downloaded += 1
        if offset % (page_size * 5) == 0:
            log(f"  [{dataset_id}] {offset + len(data['rows'])}/{total}")

    info = {
        "dataset_id": dataset_id,
        "api": "datasets-server.huggingface.co/rows",
        "num_rows_total": total,
        "downloaded_at": datetime.date.today().isoformat(),
        "downloaded": n_downloaded, "skipped": n_skipped, "failed": n_failed,
    }
    info_path.write_text(json.dumps(info, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    log(f"[{dataset_id}] fatto: {n_downloaded} nuove, {n_skipped} gia' presenti, {n_failed} fallite")


def download_zip_source(name, url, dest_dir: Path, marker_name="extracted"):
    """curl -L riprendibile (mandato, letterale) + estrazione idempotente: se
    la cartella 'extracted/' esiste gia' e non e' vuota si salta tutto."""
    raw_dir = dest_dir / "raw"
    extracted_dir = dest_dir / marker_name
    raw_dir.mkdir(parents=True, exist_ok=True)
    zip_path = raw_dir / "download.zip"

    if extracted_dir.is_dir() and any(extracted_dir.iterdir()):
        log(f"[{name}] gia' estratto in {extracted_dir}, salto download+unzip")
        # Il salto non deve saltare anche la PROVENIENZA: senza questo, una
        # cartella estratta da una corsa precedente resta senza _source_info.json
        # e URL/data di scarico esistono solo nella memoria di chi l'ha lanciata.
        write_zip_source_info(name, url, dest_dir, zip_path)
        return

    if not zip_path.is_file():
        log(f"[{name}] scarico {url}")
        # -C - riprende un download parziale se un tentativo precedente si e'
        # interrotto a meta'; curl la ignora silenziosamente se il file non c'e'.
        cmd = ["curl", "-sS", "-L", "-C", "-", "--max-time", "300", "-o", str(zip_path), url]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(f"[{name}] curl fallito ({r.returncode}): {r.stderr[-500:]}")
    else:
        log(f"[{name}] zip gia' presente ({zip_path}), salto il download")

    log(f"[{name}] estraggo in {extracted_dir}")
    extracted_dir.mkdir(parents=True, exist_ok=True)
    with zipfile_open(zip_path) as zf:
        zf.extractall(extracted_dir)

    write_zip_source_info(name, url, dest_dir, zip_path)
    log(f"[{name}] fatto")


def write_zip_source_info(name, url, dest_dir: Path, zip_path: Path):
    """Provenienza della sorgente zip: URL, data e sha256 dell'archivio se c'e'
    ancora (una cartella gia' estratta puo' averlo perso: si registra comunque
    cio' che si sa, con il motivo per cui l'hash manca)."""
    info_path = dest_dir / "_source_info.json"
    existing = {}
    if info_path.is_file():
        try:
            existing = json.loads(info_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            existing = {}
    # L'archivio puo' avere un nome diverso da download.zip (scaricato a mano in
    # una sessione precedente): si cerca comunque uno zip in raw/, perche' e' quel
    # file — non il suo nome — la cosa da hashare.
    archive = zip_path if zip_path.is_file() else next(iter(sorted(zip_path.parent.glob("*.zip"))), None)
    info = {
        "source_name": name,
        "source_url": url,
        "downloaded_at": existing.get("downloaded_at", datetime.date.today().isoformat()),
        "recorded_at": datetime.date.today().isoformat(),
        "archive_path": repo_rel(archive) if archive else None,
        "zip_sha256": (sha256_of_file(archive) if archive
                       else existing.get("zip_sha256",
                                         "archivio non piu' presente in raw/: hash non ricalcolabile")),
        "extracted_dir": repo_rel(dest_dir / "extracted"),
    }
    info_path.write_text(json.dumps(info, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def zipfile_open(path):
    import zipfile
    return zipfile.ZipFile(path)


def download_all():
    download_hf_dataset_rows(
        "Limbicnation/pixel-art-character", SOURCES_DIR / "limbicnation",
        image_ext="png", caption_field="text", filename_field="file_name")
    download_zip_source(
        "dcss-32",
        "https://opengameart.org/sites/default/files/Dungeon%20Crawl%20Stone%20Soup%20Full_0.zip",
        SOURCES_DIR / "dcss-32")
    download_zip_source(
        "tinyhero-64",
        "https://github.com/AgaMiko/pixel_character_generator/raw/master/data.zip",
        SOURCES_DIR / "tinyhero-64")
    download_hf_dataset_rows(
        "jiovine/pixel-art-nouns-2k", SOURCES_DIR / "nouns-32",
        image_ext="jpg", caption_field="text", filename_field=None)


# =============================================================================
# CANDIDATI — un dict uniforme per immagine sorgente, qualunque sia la
# provenienza: path, source, id_base (usato per nome file + fallback subject),
# subject_base_id (raggruppamento per lo split 90/10), caption grezza,
# categoria/tema per la caption, licenza.
# =============================================================================

def clean_caption_prefix(text):
    """Toglie i prefissi rumorosi delle caption HF ('Game asset:', 'Asset:',
    'Sprite:'...) — il resto della frase resta intatto, e' l'unica parte
    davvero specifica dell'immagine."""
    t = text.strip()
    t = re.sub(r"^(game asset|asset|pixel art|sprite)\s*:\s*", "", t, flags=re.IGNORECASE)
    return t.strip()


def ensure_sprite_markers(caption, single_subject_verified):
    """Aggiunge 'game sprite'/'single subject' SOLO se assenti (mandato,
    letterale): non duplica se la caption HF li ha gia'. 'single subject' entra
    SOLO se i gate del soggetto singolo (frame sorgente + canvas finale) sono
    passati: nella versione bocciata veniva appeso a tutte le caption, comprese
    quelle di mappe e diorami — lo script scriveva nel training un'asserzione
    che non aveva verificato."""
    low = caption.lower()
    extra = []
    if "sprite" not in low:
        extra.append("game sprite")
    if single_subject_verified and "single subject" not in low:
        extra.append("single subject")
    if not extra:
        return caption
    return caption.rstrip(" .,") + ", " + ", ".join(extra)


def iter_limbicnation_candidates(src_dir: Path, limit=None):
    lic = LICENSE_INFO["limbicnation"]
    files = sorted(src_dir.glob("*.png"))
    if limit:
        files = files[:limit]
    for path in files:
        idx = path.stem
        cap_path = path.with_suffix(".txt")
        caption_raw = cap_path.read_text(encoding="utf-8").strip() if cap_path.is_file() else ""
        name_path = src_dir / f"{idx}.source_name.txt"
        file_name = name_path.read_text(encoding="utf-8").strip() if name_path.is_file() else None
        original_url = (f"{lic['dataset_url']}/resolve/main/{file_name}" if file_name
                         else lic["dataset_url"])
        yield {
            "source": "limbicnation", "path": path, "id_base": f"limbicnation-{idx}",
            "subject_base_id": f"limbicnation-{idx}", "caption_raw": caption_raw,
            "category": None, "theme": None, "original_url": original_url,
            "license": lic,
        }


# URL per-riga della sorgente Nouns: il dataset HF non espone un nome file per
# riga (verificato: le righe portano solo image.src, un URL FIRMATO che scade in
# ore — inutile come provenienza), quindi l'ancora stabile e' l'indice di riga
# sull'API pubblica, che chiunque puo' rifetchare per ottenere esattamente
# quella immagine. Senza questo tutte le righe Nouns del ledger puntavano allo
# stesso URL di dataset, cioe' a niente in particolare.
NOUNS_ROW_URL = ("https://datasets-server.huggingface.co/rows?dataset=jiovine%2F"
                 "pixel-art-nouns-2k&config=default&split=train&offset={idx}&length=1")


def iter_nouns_candidates(src_dir: Path, limit=None):
    lic = LICENSE_INFO["nouns-32"]
    files = sorted(src_dir.glob("*.jpg"))
    if limit:
        files = files[:limit]
    for path in files:
        idx = path.stem
        cap_path = path.with_suffix(".txt")
        caption_raw = cap_path.read_text(encoding="utf-8").strip() if cap_path.is_file() else ""
        yield {
            "source": "nouns-32", "path": path, "id_base": f"nouns-{idx}",
            "subject_base_id": f"nouns-{idx}", "caption_raw": caption_raw,
            "category": "character", "theme": "nouns-art",
            "original_url": NOUNS_ROW_URL.format(idx=int(idx)),
            "license": lic,
        }


# Direzione tenuta per TinyHero: le altre 3 sono quasi-duplicati posturali
# dello stesso personaggio (mandato, letterale — "le altre sono quasi-duplicati").
# "2" e' il FRONTE verificato a occhio (viso/occhi visibili, confrontando le
# quattro cartelle data/0..3 fianco a fianco): "0" e' invece il DORSO (nessun
# viso, solo nuca/spalle) — build_caption_tinyhero() sotto dichiara "front
# view" nella caption, quindi la direzione tenuta deve essere DAVVERO il
# fronte o la caption mente al training (esattamente il difetto trovato
# ispezionando il contact sheet: dorsi con didascalia "front view").
TINYHERO_KEEP_DIRECTION = "2"


def iter_tinyhero_candidates(src_dir: Path, limit=None):
    lic = LICENSE_INFO["tinyhero-64"]
    direction_dir = src_dir / "extracted" / "data" / TINYHERO_KEEP_DIRECTION
    files = sorted(direction_dir.glob("*.png"), key=lambda p: int(p.stem) if p.stem.isdigit() else p.stem)
    if limit:
        files = files[:limit]
    for path in files:
        num = path.stem
        yield {
            "source": "tinyhero-64", "path": path, "id_base": f"tinyhero-{int(num):04d}",
            "subject_base_id": f"tinyhero-{int(num):04d}", "caption_raw": None,
            "category": "character", "theme": "generated-character",
            "original_url": f"{lic['dataset_url']}/blob/master/data.zip#data/{TINYHERO_KEEP_DIRECTION}/{num}.png",
            "license": lic,
        }


DCSS_TOP_CATEGORY = {
    "monster": "enemy", "item": "item", "player": "character",
    "dungeon": "prop", "effect": "vfx", "misc": "prop",
    "gui": "icon", "emissaries": "character",
}

# Suffissi di variante/frame nel nome file DCSS: non fanno parte del soggetto
# (ghost_old / ghost_new sono lo STESSO mostro in due versioni grafiche,
# bardiche_5 e' la variante-colore #5 della stessa arma) — vanno via sia dalla
# caption (subject_phrase) sia dalla chiave di raggruppamento per lo split.
DCSS_VARIANT_TOKENS = {"old", "new"}


def dcss_strip_variant_tokens(stem):
    tokens = stem.split("_")
    while len(tokens) > 1 and (tokens[-1] in DCSS_VARIANT_TOKENS or tokens[-1].isdigit()):
        tokens.pop()
    return tokens or stem.split("_")


def iter_dcss_candidates(src_dir: Path, limit=None):
    lic = LICENSE_INFO["dcss-32"]
    root = src_dir / "extracted" / "Dungeon Crawl Stone Soup Full"
    files = sorted(root.rglob("*.png"))
    if limit:
        files = files[:limit]
    for path in files:
        rel = path.relative_to(root)
        parts = rel.parts  # es. ("monster", "undead", "ghost_old.png")
        top = parts[0]
        subfolder = parts[1] if len(parts) > 2 else None
        stem = path.stem
        variant_tokens = dcss_strip_variant_tokens(stem)
        id_slug = re.sub(r"[^a-z0-9]+", "-", "-".join(parts).lower().rsplit(".", 1)[0]).strip("-")
        subject_key = f"dcss-{top}-{subfolder or ''}-{'_'.join(variant_tokens)}"
        yield {
            "source": "dcss-32", "path": path, "id_base": f"dcss-{id_slug}",
            "subject_base_id": subject_key, "caption_raw": None,
            "category": DCSS_TOP_CATEGORY.get(top, "prop"),
            "theme": subfolder or top,
            "original_url": f"{lic['dataset_url']}#{'/'.join(parts)}",
            "license": lic, "name_tokens": variant_tokens,
        }


# Da dove viene la caption di ogni sorgente, dichiarato riga per riga nel ledger:
# senza questo campo "caption" e' una stringa senza garanzie, e le quattro
# sorgenti hanno garanzie molto diverse.
CAPTION_POLICY = {
    "limbicnation": ("content-derived: caption sorgente SCARTATA (prompt di generazione che il "
                     "generatore non ha seguito), testo costruito da colori e proporzioni misurati"),
    "nouns-32": "source-metadata: tratti dichiarati dal dataset (occhiali/testa/corpo)",
    "dcss-32": "file-name: nome reale dell'asset dentro il pack OGA",
    "tinyhero-64": "fixed-verified: testo fisso, 'front view' regge perche' la direzione tenuta e' verificata",
}

# Frazione di incarnato da cui in su un'immagine TENUTA finisce comunque nella
# lista di review: sotto SKIN_FRACTION_MAX (quindi non scartata) ma abbastanza
# alta da meritare un occhio umano prima di qualunque training.
NSFW_WATCH_SKIN_FRACTION = 0.22


def caption_color_defects(caption, color_coverage):
    """Ogni parola-colore della caption ricondotta al vocabolario misurabile e
    confrontata con la copertura REALE sul canvas. E' l'audit con cui il giudice
    ha bocciato le caption sintetiche (46,6% nominava un colore sotto l'8%):
    girarlo dentro lo script vuol dire che quel difetto, se torna, si vede nel
    report invece che a valle."""
    defects = []
    for word in re.findall(r"[a-z]+", caption.lower()):
        base = COLOR_SYNONYMS.get(word, word if word in COLOR_VOCABULARY else None)
        if base is None:
            continue
        cov = color_coverage.get(base, 0.0)
        if cov < CAPTION_AUDIT_MIN_COVERAGE:
            defects.append({"word": word, "color": base, "coverage": round(cov, 4)})
    return defects


def build_caption_dcss(cand):
    subject = " ".join(t for t in cand["name_tokens"]).replace("-", " ").strip()
    subject = subject.replace("_", " ")
    parts = ["pixel art game sprite", cand["category"], subject]
    if cand["theme"] and cand["theme"] != subject:
        parts.append(cand["theme"].replace("_", " "))
    parts += ["single subject", "plain background"]
    return ", ".join(p for p in parts if p)


def build_caption_tinyhero():
    return "pixel art game character, front view, single subject, plain background"


def build_caption_from_hf(caption_raw, single_subject_verified=True):
    cleaned = clean_caption_prefix(caption_raw) if caption_raw else "pixel art game sprite"
    return ensure_sprite_markers(cleaned, single_subject_verified)


# Le caption sintetiche di Limbicnation sono i PROMPT con cui il dataset e' stato
# generato, non descrizioni di cio' che e' uscito: misurato sul bucket primario,
# il 46,6% nominava un colore che copre meno dell'8% del soggetto, e a vista
# sbagliavano specie, oggetti e posa ("green griffin" su uno scheletro arancione,
# "berserker, yellow tones" su una tettoia di legno). Si buttano — restano nel
# ledger come caption_source_raw, che e' provenienza — e la caption si costruisce
# con cio' che si puo' CONTARE sui pixel del canvas: colori dominanti oltre il 12%
# e proporzione della bbox. Niente specie, niente oggetti, niente posa: attributi
# che questo script non e' in grado di garantire non entrano nel training.
def build_caption_from_content(item):
    stats = item["canvas_stats"]
    shape = item["canvas_shape"]
    # Due colori al massimo, e i cromatici prima dei neutri: in questo stile il
    # contorno nero supera il 12% quasi ovunque (misurato: 162/347 caption
    # cominciavano con "black"), quindi ordinare per sola copertura produce
    # caption vere ma tutte uguali. I neutri entrano solo se i cromatici non
    # bastano a fare due nomi — e restano comunque colori CONTATI, non aggettivi.
    qualified = [name for name, cov in stats["color_coverage"].items()
                 if cov >= CAPTION_COLOR_MIN_COVERAGE]
    chromatic = [n for n in qualified if n not in ("black", "white", "gray")]
    named = (chromatic + [n for n in qualified if n not in chromatic])[:2]
    bw, bh = shape["bbox"]
    if bh >= bw * 1.35:
        proportion = "tall"
    elif bw >= bh * 1.35:
        proportion = "wide"
    else:
        proportion = "compact"
    parts = ["pixel art game sprite", f"{proportion} subject"]
    if named:
        parts.append(" and ".join(named) + " tones")
    else:
        parts.append("multicolored")
    parts += ["single subject", "plain background"]
    caption = ", ".join(parts)
    return caption if item["single_subject_verified"] else caption.replace(", single subject", "")


# =============================================================================
# GRANA — round-trip NEAREST downscale/upscale (mandato, metodo letterale).
# Si misura sull'immagine RGB INTERA (sfondo compreso): per queste sorgenti lo
# sfondo condivide la stessa griglia-pixel del soggetto (e' lo stesso raster
# generato/esportato), quindi misurare sull'intera cornice e' PIU' robusto che
# misurare sulla sola silhouette ritagliata (piu' bordi, piu' segnale per il
# confronto), non un compromesso.
# ============================================================================
def estimate_grain(img_rgb):
    """Ritorna (grana_stimata, mse_alla_grana_scelta). La soglia di "errore
    quasi zero" e' adattiva (pavimento assoluto + margine sul minimo
    osservato): un PNG pulito ha un vero zero alla grana giusta, un JPEG
    (Nouns) no — un numero fisso avrebbe scartato ogni sorgente JPEG."""
    w, h = img_rgb.size
    long_side = max(w, h)
    candidates = [s for s in GRAIN_CANDIDATES if s <= long_side] or [long_side]

    mse_by_scale = {}
    for s in candidates:
        if w >= h:
            nw, nh = s, max(1, round(h * s / w))
        else:
            nh, nw = s, max(1, round(w * s / h))
        down = img_rgb.resize((nw, nh), Image.NEAREST)
        up = down.resize((w, h), Image.NEAREST)
        diff = ImageChops.difference(img_rgb, up)
        stat = ImageStat.Stat(diff)
        mse = sum(stat.sum2) / (w * h * len(stat.sum2))
        mse_by_scale[s] = mse

    min_mse = min(mse_by_scale.values())
    threshold = max(GRAIN_MSE_ABS_FLOOR, min_mse * GRAIN_MSE_REL_SLACK)
    for s in candidates:  # ordine ascendente: la PRIMA che supera la soglia e' la grana minima
        if mse_by_scale[s] <= threshold:
            return s, mse_by_scale[s]
    best = min(mse_by_scale, key=mse_by_scale.get)
    return best, mse_by_scale[best]


def resample_ratio_for(source, raw_max_dim, measured_grain):
    """La matematica di ricampionamento usa la grana MISURATA per Limbicnation
    (varia per immagine, e' l'unica sorgente dove ha senso misurarla) e la
    grana NOMINALE fissa per le altre tre (sono pack curati con una risoluzione
    nativa dichiarata: usare il valore per-immagine, rumoroso su contenuti
    semplici — vedi commento su GRAIN_CANDIDATES — romperebbe l'uniformita' di
    grana dentro la stessa cartella secondaria, la regola che questo script
    esiste apposta per rispettare)."""
    if source == "limbicnation":
        grain = measured_grain
    else:
        grain = NOMINAL_GRAIN[source]
    grain = max(1, grain)
    return raw_max_dim / grain, grain


# =============================================================================
# dHash 64bit (dedup percettivo globale) — composto su CANVAS_BG_RGB, non nero:
# un nero di fondo farebbe sembrare identici due soggetti scuri diversi.
# =============================================================================
def dhash64(rgba_crop, size=8):
    flat = Image.new("RGB", rgba_crop.size, CANVAS_BG_RGB)
    flat.paste(rgba_crop, (0, 0), rgba_crop)
    small = flat.convert("L").resize((size + 1, size), Image.BILINEAR)
    px = small.load()
    bits = 0
    for y in range(size):
        for x in range(size):
            bits = (bits << 1) | (1 if px[x, y] > px[x + 1, y] else 0)
    return bits


def hamming(a, b):
    return (a ^ b).bit_count()


# =============================================================================
# Filtro per immagine (passi b-d-f del mandato; c e' dentro flood/component).
# Ritorna sempre un dict con almeno {"status", "reason_short", ...}; se
# status=="ok" porta anche il ritaglio RGBA pronto per l'assemblaggio.
# =============================================================================
def reject(reason_short, reason, **extra):
    return {"status": "reject", "reason_short": reason_short, "reason": reason, **extra}


def process_candidate(cand):
    path = cand["path"]
    source = cand["source"]

    try:
        im = Image.open(path)
        im.load()
    except Exception as e:  # noqa: BLE001 — file corrotto/illeggibile, non deve fermare la corsa
        return reject("integrity", f"file illeggibile/corrotto: {e}")

    rgba = im.convert("RGBA")
    w, h = rgba.size
    if w < 4 or h < 4:
        return reject("integrity", f"immagine degenere {w}x{h}")

    # -- b. grana (SEMPRE misurata, anche sugli scarti successivi) --
    grain, grain_mse = estimate_grain(rgba.convert("RGB"))

    # -- c. isolamento del soggetto --
    alpha_lo, _alpha_hi = rgba.split()[-1].getextrema()
    if alpha_lo < 250:
        # trasparenza reale gia' presente nel file sorgente (OGA/TinyHero):
        # e' la maschera stessa, nessun flood-fill da fare o rischiare di rompere.
        isolated = rgba
        bg_removal_mode = "native-alpha"
    else:
        isolated, bg_info = tbp.flood_remove_background(rgba)
        bg_removal_mode = bg_info["bg_removal_mode"]
        if bg_removal_mode != "flood":
            return reject("no-flat-background",
                          f"impossibile isolare il soggetto (modo={bg_removal_mode}, "
                          f"dominanza cornice={bg_info['bg_border_dominance']:.2f})",
                          grain=grain)

    mask, mw, mh = tbp.alpha_mask(isolated)
    comps = tbp.connected_components(mask, mw, mh)
    if not comps:
        return reject("empty-foreground", "nessun pixel opaco dopo l'isolamento", grain=grain)

    total_fg = sum(c["size"] for c in comps)
    fg_fraction = total_fg / (mw * mh)
    if not (FOREGROUND_MIN_FRACTION <= fg_fraction <= FOREGROUND_MAX_FRACTION):
        return reject("foreground-fraction",
                      f"foreground {fg_fraction * 100:.1f}% fuori da "
                      f"[{FOREGROUND_MIN_FRACTION * 100:.0f}-{FOREGROUND_MAX_FRACTION * 100:.0f}]%",
                      grain=grain)

    main = max(comps, key=lambda c: c["size"])
    dominant_share = main["size"] / total_fg
    if dominant_share < DOMINANT_COMPONENT_MIN_SHARE:
        return reject("no-dominant-component",
                      f"componente principale {dominant_share * 100:.1f}% del foreground "
                      f"(< {DOMINANT_COMPONENT_MIN_SHARE * 100:.0f}%, {len(comps)} componenti)",
                      grain=grain)

    minx, miny, maxx, maxy = main["bbox"]
    bw, bh = maxx - minx + 1, maxy - miny + 1
    aspect = max(bw, bh) / max(1, min(bw, bh))
    if aspect > ASPECT_RATIO_MAX:
        return reject("extreme-aspect-ratio",
                      f"bbox {bw}x{bh} rapporto {aspect:.1f} > {ASPECT_RATIO_MAX} "
                      f"(tile/font/barra sospetti)", grain=grain)

    border_occ = tbp.border_occupancy(mask, mw, mh)
    if border_occ > BORDER_CONTACT_MAX:
        return reject("heavy-border-contact",
                      f"{border_occ * 100:.1f}% della cornice e' opaca (> {BORDER_CONTACT_MAX * 100:.0f}%)",
                      grain=grain)

    tbp.strip_non_main_components(isolated, comps, main)
    crop = isolated.crop((minx, miny, maxx + 1, maxy + 1))

    # -- d. qualita' --
    n_colors, _mean_fg_rgb, _opaque_px = foreground_colors_posterized(crop)
    if not (QUALITY_MIN_COLORS <= n_colors <= QUALITY_MAX_COLORS):
        return reject("color-count",
                      f"{n_colors} colori dopo quantizzazione leggera, fuori da "
                      f"[{QUALITY_MIN_COLORS}-{QUALITY_MAX_COLORS}]", grain=grain)

    ratio, effective_grain = resample_ratio_for(source, max(w, h), grain)
    logical_w = max(1, round(bw / ratio))
    logical_h = max(1, round(bh / ratio))
    if max(logical_w, logical_h) < MIN_CONTENT_LOGICAL_PX:
        return reject("content-too-small-logical",
                      f"contenuto {logical_w}x{logical_h}px logici < {MIN_CONTENT_LOGICAL_PX}",
                      grain=grain)

    # -- instradamento PRIMA dei gate di canvas: fill/lastra/contrasto vanno
    # misurati sul canvas della cartella di destinazione (128 per il primario,
    # la grana nativa per i secondari), non su un canvas ipotetico. --
    bucket, canvas_size = route_bucket(source, grain)
    if bucket is None:
        return reject("grain-below-primary-threshold",
                      f"grana {grain} < {PRIMARY_GRAIN_MIN} e nessun bucket secondario "
                      f"per '{source}'", grain=grain)

    canvas, canvas_mask, canvas_visible, logical_size = compose_logical_canvas(
        crop, ratio, canvas_size)
    shape = canvas_shape_metrics(canvas_visible, canvas_size)
    stats = canvas_pixel_stats(canvas, canvas_mask, canvas_visible, canvas_size)
    if shape is None or stats is None:
        return reject("subject-imperceptible",
                      "nessun pixel del soggetto si distingue dal canvas", grain=grain)

    # -- c2. soggetto singolo sul canvas finale (solo bucket primario) --
    # Le tre sorgenti secondarie sono pack curati a grana nativa dove lo sprite
    # riempie la propria cella per costruzione (un boulder DCSS o un busto Nouns
    # sono soggetti singoli legittimi che occupano quasi tutto il canvas 32): li'
    # il gate equivalente e' gia' quello sul frame sorgente (foreground 8-60% +
    # cornice), e applicare queste soglie svuoterebbe cartelle che il giudice ha
    # verificato sane. Il difetto misurato (~29% di scene/diorami) e' del solo
    # primario, dove il soggetto e' DISEGNATO dentro un frame 512 che puo'
    # contenere una scena intera.
    if bucket == PRIMARY_BUCKET:
        scene_reason = scene_rejection_reason(shape)
        if scene_reason:
            return reject("scene-not-single-subject", scene_reason, grain=grain)

    # -- d2. contrasto e leggibilita' CONTRO IL FONDO DEL CANVAS --
    if shape["fill"] < CANVAS_FILL_MIN:
        return reject("canvas-nearly-empty",
                      f"solo il {shape['fill'] * 100:.1f}% del canvas mostra qualcosa "
                      f"(< {CANVAS_FILL_MIN * 100:.0f}%): soggetto quasi invisibile sul fondo chiaro",
                      grain=grain)
    # Nessun gate per modo di isolamento: la sorgente primaria e' tutta
    # native-alpha, e legarlo al ramo flood (com'era) lo spegneva proprio dove
    # serviva. Due misure, non una: il contrasto MEDIO non vede la figura
    # chiarissima con un contorno scuro, la frazione impercettibile si'.
    if stats["contrast"] < CONTRAST_MIN_RATIO:
        return reject("low-contrast",
                      f"contrasto medio soggetto/canvas {stats['contrast']:.2f} < "
                      f"{CONTRAST_MIN_RATIO}", grain=grain)
    if stats["imperceptible_fraction"] > IMPERCEPTIBLE_MAX_FRACTION:
        return reject("subject-imperceptible",
                      f"{stats['imperceptible_fraction'] * 100:.0f}% dei pixel del soggetto "
                      f"sotto contrasto {IMPERCEPTIBLE_CONTRAST} col canvas", grain=grain)

    # -- f. NSFW (solo Limbicnation) --
    if source == "limbicnation":
        cap_low = (cand.get("caption_raw") or "").lower()
        hit = next((k for k in NSFW_KEYWORDS if k in cap_low), None)
        if hit:
            return reject("nsfw-keyword", f"parola chiave '{hit.strip()}' nella caption sorgente",
                          grain=grain)
        if stats["skin_fraction"] > SKIN_FRACTION_MAX:
            return reject("nsfw-skin-fraction",
                          f"{stats['skin_fraction'] * 100:.0f}% dei pixel visibili in gamma "
                          f"incarnato (> {SKIN_FRACTION_MAX * 100:.0f}%): la gamma comprende anche "
                          f"legno/sabbia/terra, quindi non e' una diagnosi di nudo — e' la regola "
                          f"'in dubbio scarta' del mandato", grain=grain)

    return {
        "status": "ok", "cand": cand, "crop": crop, "grain": grain, "grain_mse": grain_mse,
        "effective_grain": effective_grain, "resample_ratio": ratio,
        "bucket": bucket, "canvas_size": canvas_size,
        "bg_removal_mode": bg_removal_mode, "n_colors": n_colors,
        "logical_size": list(logical_size),
        "source_logical_size": [logical_w, logical_h],
        "fg_fraction": round(fg_fraction, 4),
        "canvas_shape": shape, "canvas_stats": stats,
        "single_subject_verified": True,
        # Da COSA e' sostenuta quell'asserzione su questa riga: i gate di canvas
        # girano solo sul bucket primario (vedi il commento al punto c2), e dire
        # "verificato" senza dire da cosa sarebbe di nuovo un'affermazione piu'
        # larga della verifica che l'ha prodotta.
        "single_subject_checks": (["source-frame", "canvas-scene-gates"]
                                  if bucket == PRIMARY_BUCKET else ["source-frame"]),
        "dhash": dhash64(crop),
    }


def scene_rejection_reason(shape):
    """I quattro modi in cui una scena si presenta sul canvas, dal piu' grossolano
    al piu' sottile. Nessun elenco di id: sono misure, ognuna tarata sullo stacco
    fra scene e sprite dell'insieme etichettato (vedi le costanti CANVAS_*)."""
    if shape["fill"] >= CANVAS_FILL_MAX:
        return (f"riempimento canvas {shape['fill'] * 100:.0f}% >= {CANVAS_FILL_MAX * 100:.0f}%: "
                f"il soggetto E' il canvas (scena/tilemap), non uno sprite dentro un canvas")
    if shape["border_occ"] >= CANVAS_BORDER_OCC_MAX:
        return (f"cornice del canvas occupata al {shape['border_occ'] * 100:.0f}% "
                f"(>= {CANVAS_BORDER_OCC_MAX * 100:.0f}%): il contenuto esce dai bordi come una scena")
    if shape["straight_edge"] >= CANVAS_STRAIGHT_EDGE_MAX:
        return (f"bordo inferiore dritto per il {shape['straight_edge'] * 100:.0f}% della larghezza "
                f"(>= {CANVAS_STRAIGHT_EDGE_MAX * 100:.0f}%): spigolo di piattaforma/pavimento "
                f"isometrico, non silhouette di un soggetto")
    if shape["vsym"] >= CANVAS_VSYM_MAX:
        return (f"simmetria alto/basso {shape['vsym'] * 100:.0f}% (>= {CANVAS_VSYM_MAX * 100:.0f}%): "
                f"cornice, stemma o campo di tile, non un soggetto con testa e piedi")
    return None


def foreground_colors_posterized(crop_rgba, target_colors=QUALITY_TARGET_COLORS):
    """Quantizzazione ADATTIVA median-cut (vedi commento sopra GRAIN_CANDIDATES):
    Pillow cerca la MIGLIOR tavolozza fino a target_colors per QUESTO ritaglio
    (non un troncamento di bit fisso, che su Limbicnation/Nouns non scende mai
    sotto le migliaia di colori). Il fondo viene appiattito su CANVAS_BG_RGB
    solo per dare a quantize() un'immagine RGB piena — il conteggio finale usa
    l'alpha ORIGINALE per contare solo gli indici di palette toccati dal
    foreground, mai dal fondo. Dither=NONE: con dithering attivo il rumore di
    ridistribuzione gonfierebbe artificialmente n_colors, vanificando il
    filtro. Stesso giro anche per il colore medio del foreground (contrasto),
    un solo passaggio pixel-per-pixel sul ritaglio (gia' piccolo, e' il bbox)."""
    w, h = crop_rgba.size
    flat = Image.new("RGB", (w, h), CANVAS_BG_RGB)
    flat.paste(crop_rgba, (0, 0), crop_rgba)
    quantized = flat.quantize(colors=target_colors, method=Image.Quantize.MEDIANCUT,
                               dither=Image.Dither.NONE).convert("RGB")

    a_px = crop_rgba.split()[-1].load()
    q_px = quantized.load()
    colors = set()
    rs = gs = bs = n = 0
    for y in range(h):
        for x in range(w):
            if a_px[x, y] <= 128:
                continue
            r, g, b = q_px[x, y]
            n += 1
            rs += r; gs += g; bs += b
            colors.add((r, g, b))
    mean_rgb = (rs // max(1, n), gs // max(1, n), bs // max(1, n))
    return len(colors), mean_rgb, n


def relative_luminance(rgb):
    def lin(c):
        c = c / 255.0
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (lin(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(rgb_a, rgb_b):
    la = relative_luminance(rgb_a) + 0.05
    lb = relative_luminance(rgb_b) + 0.05
    return max(la, lb) / min(la, lb)


# =============================================================================
# COLORE MISURATO — vocabolario grezzo (13 nomi) usato per DUE cose diverse ma
# simmetriche: costruire le caption di primary-128 dai pixel e verificare, nel
# report, che ogni parola-colore di OGNI caption corrisponda a pixel che
# esistono davvero. Simmetrico apposta: la caption dice solo cio' che l'audit
# sa ricontrollare. I confini di tinta sono i soliti settori HSV, l'unica
# distinzione non ovvia e' arancio scuro -> "brown" e arancio slavato -> "tan",
# senza le quali meta' della pixel art (legno, pelle, terra) finirebbe
# etichettata "orange".
# =============================================================================
COLOR_VOCABULARY = ("black", "white", "gray", "red", "orange", "brown", "tan",
                    "yellow", "olive", "green", "cyan", "blue", "purple", "magenta", "pink")

# Sinonimi che compaiono nelle caption delle sorgenti (prompt sintetici di
# Limbicnation, metadati Nouns, nomi file DCSS): l'audit li riconduce al nome
# base misurabile, altrimenti misurerebbe "crimson" contro una tavolozza che
# conosce solo "red" e segnalerebbe difetti inesistenti.
COLOR_SYNONYMS = {
    "crimson": "red", "scarlet": "red", "ruby": "red", "blood": "red",
    "golden": "yellow", "gold": "yellow", "amber": "yellow", "lemon": "yellow",
    "emerald": "green", "jade": "green", "lime": "green", "forest": "green",
    "teal": "cyan", "turquoise": "cyan", "aqua": "cyan", "cerulean": "blue",
    "azure": "blue", "navy": "blue", "sapphire": "blue", "cobalt": "blue",
    "violet": "purple", "lavender": "purple", "amethyst": "purple", "indigo": "purple",
    "fuchsia": "magenta", "rose": "pink", "peachy": "tan", "peach": "tan",
    "beige": "tan", "bege": "tan", "ivory": "white", "pearl": "white", "snow": "white",
    "charcoal": "black", "obsidian": "black", "ebony": "black", "onyx": "black",
    "silver": "gray", "ash": "gray", "slate": "gray", "steel": "gray", "iron": "gray",
    "copper": "brown", "bronze": "brown", "rust": "brown", "chocolate": "brown",
    "darkbrown": "brown", "redpinkish": "red", "grayscale": "gray", "grey": "gray",
}


def color_name(rgb):
    r, g, b = (c / 255.0 for c in rgb)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    if v < 0.16:
        return "black"
    if s < 0.14:
        return "white" if v >= 0.82 else ("gray" if v >= 0.42 else "black")
    hd = h * 360.0
    if hd < 15 or hd >= 340:
        return "pink" if (v >= 0.80 and s < 0.45) else "red"
    if hd < 42:
        if v < 0.62:
            return "brown"
        return "tan" if s < 0.38 else "orange"
    if hd < 70:
        return "olive" if v < 0.55 else "yellow"
    if hd < 165:
        return "green"
    if hd < 200:
        return "cyan"
    if hd < 258:
        return "blue"
    if hd < 292:
        return "purple"
    return "magenta"


# Gamma incarnato per l'euristica NSFW: settore arancio-rosato a saturazione
# media. Volutamente LARGA sulla luminosita' (incarnati scuri inclusi: il caso
# 00454 che il giudice ha trovato aveva caption "ebony"), e per questo da sola
# non decide niente — decide la FRAZIONE di soggetto che ci cade dentro.
def is_skin_tone(rgb):
    h, s, v = colorsys.rgb_to_hsv(*(c / 255.0 for c in rgb))
    return 0.014 <= h <= 0.115 and 0.15 <= s <= 0.72 and v >= 0.30


# =============================================================================
# ASSEMBLAGGIO — ricampiona il ritaglio ALLA GRANA (effettiva) -> lo centra in
# un canvas logico piatto (sfondo neutro, NO alpha: SD1.5 non ha canale alpha,
# mandato letterale) -> nearest upscale a 512.
# =============================================================================
def compose_logical_canvas(crop_rgba, resample_ratio, canvas_size):
    """Canvas LOGICO (prima dell'upscale a 512): l'immagine RGB piatta piu' la
    maschera del soggetto su quel canvas. E' lo stadio su cui girano i gate del
    punto c2, del contrasto e dell'euristica NSFW — misurare sul frame sorgente
    (com'era prima) significa misurare pixel che nel training non ci sono."""
    cw, ch = crop_rgba.size
    if resample_ratio > 1.0 + 1e-6:
        lw = max(1, round(cw / resample_ratio))
        lh = max(1, round(ch / resample_ratio))
        content = crop_rgba.resize((lw, lh), Image.NEAREST)
    else:
        content, lw, lh = crop_rgba, cw, ch

    # Raro (crop molto largo o grana sottostimata): un ulteriore downscale
    # NEAREST fa rientrare il contenuto nel canvas. Mai un crop qui — taglierebbe
    # il soggetto già isolato, l'esatto difetto che i filtri sopra escludono.
    if lw > canvas_size or lh > canvas_size:
        extra = max(lw / canvas_size, lh / canvas_size)
        lw = max(1, round(lw / extra))
        lh = max(1, round(lh / extra))
        content = content.resize((lw, lh), Image.NEAREST)

    flat = Image.new("RGB", (lw, lh), CANVAS_BG_RGB)
    flat.paste(content, (0, 0), content)
    canvas = Image.new("RGB", (canvas_size, canvas_size), CANVAS_BG_RGB)
    ox, oy = (canvas_size - lw) // 2, (canvas_size - lh) // 2
    canvas.paste(flat, (ox, oy))

    # DUE maschere, perche' servono a domande diverse:
    #   mask  = estensione del soggetto (alpha binarizzata) — risponde a "quanto
    #           del soggetto e' invisibile sul canvas?", che con la sola maschera
    #           dei pixel visibili sarebbe zero per costruzione;
    #   visible = i pixel che sul canvas si distinguono davvero dal fondo — e'
    #           la sagoma che il modello vede, quindi l'unica onesta per la
    #           geometria (riempimento, bordi, simmetria). Una figura chiarissima
    #           ha un'alpha piena e una sagoma visibile piena di buchi: misurare
    #           la forma sull'alpha significa misurare pixel che non ci sono.
    mask = bytearray(canvas_size * canvas_size)
    visible = bytearray(canvas_size * canvas_size)
    a_px = content.split()[-1].load()
    c_px = canvas.load()
    for y in range(lh):
        row = (oy + y) * canvas_size
        for x in range(lw):
            if a_px[x, y] > 128:
                mask[row + ox + x] = 1
                if contrast_ratio(c_px[ox + x, oy + y], CANVAS_BG_RGB) >= IMPERCEPTIBLE_CONTRAST:
                    visible[row + ox + x] = 1
    return canvas, mask, visible, (lw, lh)


def upscale_to_final(canvas, canvas_size):
    upscale = FINAL_PX // canvas_size
    return canvas.resize((canvas_size * upscale, canvas_size * upscale), Image.NEAREST)


def canvas_shape_metrics(mask, canvas_size):
    """Geometria del soggetto SUL CANVAS: riempimento, cornice occupata,
    compattezza, lunghezza del piu' lungo tratto RETTO del profilo inferiore e
    simmetria alto/basso. Gli ultimi due sono i segnali che distinguono una scena
    da uno sprite senza toccare gli sprite: un bordo dritto e lungo lo produce una
    piattaforma isometrica, non una silhouette organica; una simmetria verticale
    quasi perfetta la produce una cornice o un tilemap, non un personaggio."""
    fg = sum(mask)
    if fg == 0:
        return None
    rows = [mask[y * canvas_size:(y + 1) * canvas_size] for y in range(canvas_size)]
    ys = [y for y, r in enumerate(rows) if any(r)]
    xs = [x for x in range(canvas_size) if any(rows[y][x] for y in ys)]
    y0, y1, x0, x1 = ys[0], ys[-1], xs[0], xs[-1]
    bw, bh = x1 - x0 + 1, y1 - y0 + 1

    ring_w = max(1, round(canvas_size / 64))
    ring_total = canvas_size * canvas_size - (canvas_size - 2 * ring_w) ** 2
    ring_hit = sum(1 for y in range(canvas_size) for x in range(canvas_size)
                   if (x < ring_w or y < ring_w or x >= canvas_size - ring_w
                       or y >= canvas_size - ring_w) and mask[y * canvas_size + x])

    # Profilo inferiore (y piu' basso occupato per colonna) e piu' lungo tratto
    # che sta entro CANVAS_EDGE_TOLERANCE da un segmento a pendenza nota.
    bottom = {}
    for x in range(x0, x1 + 1):
        col = [y for y in range(y0, y1 + 1) if rows[y][x]]
        if col:
            bottom[x] = col[-1]
    cols = sorted(bottom)
    longest_edge = 0
    for slope in CANVAS_EDGE_SLOPES:
        for i, start in enumerate(cols):
            j = i
            while (j + 1 < len(cols) and cols[j + 1] == cols[j] + 1
                   and abs(bottom[cols[j + 1]] - (bottom[start] + slope * (cols[j + 1] - start)))
                   <= CANVAS_EDGE_TOLERANCE):
                j += 1
            longest_edge = max(longest_edge, cols[j] - start)

    inter = union = 0
    for y in range(y0, y1 + 1):
        mirrored = rows[y1 - (y - y0)]
        row = rows[y]
        for x in range(x0, x1 + 1):
            a, b = row[x], mirrored[x]
            inter += a and b
            union += a or b

    return {
        "fill": fg / (canvas_size * canvas_size),
        "border_occ": ring_hit / ring_total,
        "solidity": fg / (bw * bh),
        "straight_edge": longest_edge / max(1, bw),
        "vsym": inter / max(1, union),
        "bbox": (bw, bh),
    }


def canvas_pixel_stats(canvas_rgb, mask, visible, canvas_size):
    """Un solo passaggio sui pixel del soggetto sul canvas finale: colore medio,
    frazione impercettibile sul fondo, frazione di incarnato e copertura per nome
    di colore. Tutto cio' che le caption possono affermare esce da qui — e la
    copertura per colore si conta sui soli pixel VISIBILI, altrimenti una caption
    finirebbe per nominare il colore di pixel che nel training non si vedono."""
    px = canvas_rgb.load()
    counts = Counter()
    rs = gs = bs = n = imperceptible = skin = n_visible = 0
    for y in range(canvas_size):
        base = y * canvas_size
        for x in range(canvas_size):
            if not mask[base + x]:
                continue
            c = px[x, y]
            n += 1
            rs += c[0]; gs += c[1]; bs += c[2]
            if not visible[base + x]:
                imperceptible += 1
                continue
            n_visible += 1
            if is_skin_tone(c):
                skin += 1
            counts[color_name(c)] += 1
    if n == 0 or n_visible == 0:
        return None
    mean_rgb = (rs // n, gs // n, bs // n)
    return {
        "mean_rgb": mean_rgb,
        "contrast": contrast_ratio(mean_rgb, CANVAS_BG_RGB),
        "imperceptible_fraction": imperceptible / n,
        "skin_fraction": skin / n_visible,
        "color_coverage": {name: cnt / n_visible for name, cnt in counts.most_common()},
        "subject_px": n,
        "visible_px": n_visible,
    }


# =============================================================================
# DEDUP percettivo GLOBALE (dHash, distanza <=6) — prima vince, ordine
# deterministico per sorgente+id cosi' la corsa e' riproducibile bit per bit.
# =============================================================================
def global_dedup(survivors):
    survivors = sorted(survivors, key=lambda r: (r["cand"]["source"], r["cand"]["id_base"]))
    kept, dup_rejects = [], []
    kept_hashes = []  # lista di (hash, id) per il messaggio di scarto
    for item in survivors:
        h = item["dhash"]
        best_dist, best_id = 999, None
        for kh, kid in kept_hashes:
            d = hamming(h, kh)
            if d < best_dist:
                best_dist, best_id = d, kid
                if d == 0:
                    break
        if best_dist <= DHASH_DUP_MAX_DIST:
            r = reject("duplicate-perceptual",
                       f"dHash a distanza {best_dist} da '{best_id}' gia' tenuta", grain=item["grain"])
            r["cand"] = item["cand"]
            dup_rejects.append(r)
            continue
        kept_hashes.append((h, item["cand"]["id_base"]))
        kept.append(item)
    return kept, dup_rejects


# =============================================================================
# SPLIT 90/10 per soggetto (id base) — riempimento avido per hash, un gruppo
# entra in val solo se ci sta INTERO (stesso schema di lora_dataset_build.py).
# =============================================================================
def assign_splits(items, bucket_name):
    groups = defaultdict(list)
    for it in items:
        groups[it["cand"]["subject_base_id"]].append(it)
    target = max(1, round(len(items) * VAL_TARGET)) if items else 0
    ordered = sorted(groups.items(), key=lambda kv: hashlib.sha256(
        f"{SPLIT_SEED}|{bucket_name}|{kv[0]}".encode("utf-8")).hexdigest())
    in_val = 0
    for _key, members in ordered:
        if in_val + len(members) <= target:
            split = "val"
            in_val += len(members)
        else:
            split = "train"
        for it in members:
            it["split"] = split


# =============================================================================
# Contact sheet — griglia 10x8, campione casuale con seme fisso (riproducibile).
# =============================================================================
def build_contact_sheet(image_paths, cols=CONTACT_SHEET_COLS, rows=CONTACT_SHEET_ROWS,
                        cell=CONTACT_SHEET_CELL, seed=CONTACT_SHEET_SEED):
    rng = random.Random(seed)
    pool = list(image_paths)
    rng.shuffle(pool)
    sample = pool[:cols * rows]
    sheet = Image.new("RGB", (cols * cell, rows * cell), (30, 30, 34))
    for i, p in enumerate(sample):
        r, c = divmod(i, cols)
        with Image.open(p) as im:
            thumb = im.convert("RGB").resize((cell, cell), Image.NEAREST)
        sheet.paste(thumb, (c * cell, r * cell))
    return sheet


# =============================================================================
# Driver principale: candidati -> filtri -> dedup globale -> instradamento nei
# bucket -> assemblaggio+salvataggio -> split -> ledger + report + contact sheet.
# =============================================================================
def slugify(text):
    out = re.sub(r"[^a-z0-9\-_]+", "-", text.lower())
    while "--" in out:
        out = out.replace("--", "-")
    return out.strip("-")


def route_bucket(source, grain):
    """SOLO limbicnation puo' finire in primary-128, e SOLO se la sua grana
    misurata supera PRIMARY_GRAIN_MIN (altrimenti scartata: niente bucket
    secondario suo, mandato letterale). Le altre tre sorgenti vanno SEMPRE nel
    proprio bucket a grana nominale fissa, MAI instradate sulla grana misurata:
    il round-trip e' rumoroso sulle JPEG semplici (Nouns, compressione) e su
    contenuti piatti in generale — verificato in smoke test, misura fino a 256
    su sprite Nouns nativamente a 32px. Instradare per grana misurata invece
    che per sorgente rimescolerebbe grane diverse nella STESSA cartella,
    l'esatto difetto (reclamo su lora-v0) per cui questo script esiste."""
    if source == "limbicnation":
        if grain >= PRIMARY_GRAIN_MIN:
            return PRIMARY_BUCKET, PRIMARY_CANVAS
        return None, None
    if source in SECONDARY_BUCKET:
        return SECONDARY_BUCKET[source], NOMINAL_GRAIN[source]
    return None, None


def build_caption(item):
    cand = item["cand"]
    source = cand["source"]
    if source == "limbicnation":
        return build_caption_from_content(item)
    if source == "nouns-32":
        return build_caption_from_hf(cand["caption_raw"], item["single_subject_verified"])
    if source == "tinyhero-64":
        return build_caption_tinyhero()
    if source == "dcss-32":
        return build_caption_dcss(cand)
    raise ValueError(f"sorgente sconosciuta: {source}")


def run_mine(out_dir: Path, sources_dir: Path, limit_per_source=None):
    log("raccolgo candidati...")
    candidates = []
    candidates += list(iter_limbicnation_candidates(sources_dir / "limbicnation", limit_per_source))
    candidates += list(iter_dcss_candidates(sources_dir / "dcss-32", limit_per_source))
    candidates += list(iter_tinyhero_candidates(sources_dir / "tinyhero-64", limit_per_source))
    candidates += list(iter_nouns_candidates(sources_dir / "nouns-32", limit_per_source))
    log(f"{len(candidates)} candidati totali")

    by_source_total = Counter(c["source"] for c in candidates)

    survivors = []
    reject_rows = []
    reason_counter = Counter()
    reason_by_source = Counter()
    reason_samples = defaultdict(list)

    t0 = time.time()
    for i, cand in enumerate(candidates, start=1):
        result = process_candidate(cand)
        if result["status"] == "ok":
            survivors.append(result)
        else:
            reason_counter[result["reason_short"]] += 1
            reason_by_source[(result["reason_short"], cand["source"])] += 1
            row = {
                "source": cand["source"], "source_path": repo_rel(cand["path"]),
                "id_base": cand["id_base"],
                "reason_short": result["reason_short"], "reason": result["reason"],
                "grain": result.get("grain"),
            }
            reject_rows.append(row)
            if len(reason_samples[result["reason_short"]]) < REPORT_SAMPLES_PER_REASON:
                reason_samples[result["reason_short"]].append(row["source_path"])
        if i % 500 == 0:
            log(f"  {i}/{len(candidates)} processati ({time.time() - t0:.0f}s)")
    log(f"filtri per-immagine: {len(survivors)} sopravvissuti, {len(reject_rows)} scarti "
        f"({time.time() - t0:.0f}s)")

    kept, dup_rejects = global_dedup(survivors)
    for r in dup_rejects:
        reason_counter[r["reason_short"]] += 1
        reason_by_source[(r["reason_short"], r["cand"]["source"])] += 1
        row = {"source": r["cand"]["source"], "source_path": repo_rel(r["cand"]["path"]),
               "id_base": r["cand"]["id_base"],
               "reason_short": r["reason_short"], "reason": r["reason"], "grain": r.get("grain")}
        reject_rows.append(row)
        if len(reason_samples[r["reason_short"]]) < REPORT_SAMPLES_PER_REASON:
            reason_samples[r["reason_short"]].append(row["source_path"])
    log(f"dedup globale: {len(kept)} sopravvissuti dopo {len(dup_rejects)} duplicati")

    # L'instradamento e' gia' deciso dentro process_candidate (i gate di canvas
    # hanno bisogno del canvas di destinazione per girare): qui si raggruppa e
    # basta, non si decide piu' niente.
    buckets = defaultdict(list)
    for item in kept:
        buckets[item["bucket"]].append(item)

    out_dir.mkdir(parents=True, exist_ok=True)
    # Pulizia PRIMA di ripopolare: ogni bucket conosciuto (non solo quelli con
    # sopravvissuti in QUESTA corsa) parte da una cartella vuota. Senza questo,
    # una ri-mine dopo una modifica dei filtri/candidati (es. cambio direzione
    # TinyHero) lascia file ORFANI della corsa precedente — stesso schema id
    # (tinyhero-NNNN), contenuto diverso, NESSUNA riga nel ledger — mescolati
    # coi file nuovi. Verificato: 209 orfani (dorso, corsa precedente) rimasti
    # in secondary-tinyhero-64/ accanto ai 318 fronti di questa corsa, silenziosi
    # perche' condividono lo stesso schema-nome e quindi non danno errori di
    # scrittura, solo un dataset piu' grande di quanto il ledger dichiari.
    for stale_bucket in (PRIMARY_BUCKET, *SECONDARY_BUCKET.values()):
        stale_dir = out_dir / stale_bucket
        if stale_dir.is_dir():
            shutil.rmtree(stale_dir)
    ledger_rows = []
    # Per bucket, non globale: i bucket si scrivono in ordine alfabetico del
    # nome (dict di insertion order, ma il primo instradato e' sempre
    # 'secondary-dcss-32', il piu' numeroso) — un cap GLOBALE a 40 riempirebbe
    # l'intero campione con un solo bucket/percorso di caption (osservato: 40/40
    # da build_caption_dcss, zero esempi delle altre tre build_caption_*).
    # Il campione finale (round-robin fra i bucket, vedi sotto) deve invece
    # coprire tutti e quattro i percorsi di costruzione caption per essere
    # utile a verificarli dal solo mining_report.json.
    sample_captions_by_bucket = defaultdict(list)
    SAMPLE_CAPTIONS_PER_BUCKET = 10
    now = datetime.datetime.now().isoformat(timespec="seconds")
    caption_audit = defaultdict(lambda: {"captions": 0, "with_unsupported_color": 0, "examples": []})
    nsfw_watch = []

    for bucket_name, items in buckets.items():
        assign_splits(items, bucket_name)
        bucket_dir = out_dir / bucket_name
        bucket_dir.mkdir(parents=True, exist_ok=True)
        seen_ids = set()
        saved_paths = []
        for item in items:
            cand = item["cand"]
            base_id = slugify(cand["id_base"])
            img_id = base_id
            n = 2
            while img_id in seen_ids:
                img_id = f"{base_id}-{n}"
                n += 1
            seen_ids.add(img_id)

            # Ricomposto (non tenuto in memoria dai filtri): un resize NEAREST
            # per immagine costa niente, tenere 5800 canvas vivi fino a qui no.
            canvas, _mask, _visible, logical_size = compose_logical_canvas(
                item["crop"], item["resample_ratio"], item["canvas_size"])
            final_img = upscale_to_final(canvas, item["canvas_size"])
            img_path = bucket_dir / f"{img_id}.png"
            txt_path = bucket_dir / f"{img_id}.txt"
            caption = build_caption(item)
            final_img.save(img_path, "PNG")
            txt_path.write_text(caption + "\n", encoding="utf-8")
            saved_paths.append(img_path)
            if len(sample_captions_by_bucket[bucket_name]) < SAMPLE_CAPTIONS_PER_BUCKET:
                sample_captions_by_bucket[bucket_name].append(caption)

            audit = caption_audit[bucket_name]
            audit["captions"] += 1
            defects = caption_color_defects(caption, item["canvas_stats"]["color_coverage"])
            if defects:
                audit["with_unsupported_color"] += 1
                if len(audit["examples"]) < REPORT_SAMPLES_PER_REASON:
                    audit["examples"].append({"id": img_id, "caption": caption, "defects": defects})
            if (cand["source"] == "limbicnation"
                    and item["canvas_stats"]["skin_fraction"] >= NSFW_WATCH_SKIN_FRACTION):
                nsfw_watch.append({
                    "id": img_id, "status": "tenuta",
                    "image_path": repo_rel(img_path),
                    "skin_fraction": round(item["canvas_stats"]["skin_fraction"], 4),
                    "caption_source_raw": cand.get("caption_raw"),
                })

            lic = cand["license"]
            ledger_rows.append({
                "id": img_id,
                "bucket": bucket_name,
                "image_path": repo_rel(img_path),
                "caption_path": repo_rel(txt_path),
                "caption": caption,
                "caption_policy": CAPTION_POLICY[cand["source"]],
                # Provenienza della caption sorgente ANCHE quando viene buttata
                # (primary-128): serve a poter rifare l'analisi che l'ha bocciata.
                "caption_source_raw": cand.get("caption_raw"),
                "source": cand["source"],
                "url": cand["original_url"],
                "license_declared": lic["license_id"],
                "license_id": lic["license_id"],  # alias: compatibilita' col preflight di dataset/lora-v0/kaggle/train_lora_v0.py
                "license_url": lic["license_url"],
                "license_alt": lic.get("license_alt"),
                "license_snapshot_path": (
                    "nessuno snapshot scaricato (prova research-only): vedi license_url"),
                "author": lic["author"],
                "provenance": "research-only",
                "grain_estimated": item["grain"],
                "grain_mse": round(item["grain_mse"], 3),
                "effective_grain_used": item["effective_grain"],
                "canvas_size": item["canvas_size"],
                "logical_content_size": list(logical_size),
                "fg_fraction": item["fg_fraction"],
                "n_colors": item["n_colors"],
                "bg_removal_mode": item["bg_removal_mode"],
                # Misure sul canvas finale: sono quelle su cui girano i gate di
                # soggetto singolo, contrasto e NSFW — scritte per ogni riga cosi'
                # che chiunque possa ricontrollare le soglie senza rifare la corsa.
                "canvas_fill": round(item["canvas_shape"]["fill"], 4),
                "canvas_border_occ": round(item["canvas_shape"]["border_occ"], 4),
                "canvas_solidity": round(item["canvas_shape"]["solidity"], 4),
                "canvas_straight_edge": round(item["canvas_shape"]["straight_edge"], 4),
                "canvas_vsym": round(item["canvas_shape"]["vsym"], 4),
                "subject_contrast_vs_canvas": round(item["canvas_stats"]["contrast"], 3),
                "imperceptible_fraction": round(item["canvas_stats"]["imperceptible_fraction"], 4),
                "skin_fraction": round(item["canvas_stats"]["skin_fraction"], 4),
                "color_coverage": {k: round(v, 4) for k, v in
                                   list(item["canvas_stats"]["color_coverage"].items())[:5]},
                "single_subject_verified": item["single_subject_verified"],
                "single_subject_checks": item["single_subject_checks"],
                "transformations": [
                    "alpha_isolation:" + item["bg_removal_mode"],
                    f"resample_nearest_to_grain_{item['effective_grain']}",
                    f"center_in_canvas_{item['canvas_size']}_bg_{'-'.join(str(c) for c in CANVAS_BG_RGB)}",
                    f"nearest_upscale_{FINAL_PX}",
                ],
                "source_sha256": sha256_of_file(cand["path"]),
                "derived_sha256": sha256_of_file(img_path),
                "perceptual_hash": f"dhash64:{item['dhash']:016x}",
                "subject_key": cand["subject_base_id"],
                "split": item["split"],
                "added_at": now,
            })

        sheet = build_contact_sheet(saved_paths)
        sheet.save(out_dir / f"contact-sheet-{bucket_name}.png")
        log(f"[{bucket_name}] {len(items)} immagini, contact sheet salvato")

    ledger_rows.sort(key=lambda r: (r["bucket"], r["id"]))
    ledger_path = out_dir / "ledger.jsonl"
    with ledger_path.open("w", encoding="utf-8") as f:
        for row in ledger_rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    # rejects.jsonl come in dataset/lora-v0/: una riga per SCARTO, non 20 esempi
    # per motivo dentro il report. Senza questo file una decisione di esclusione
    # non e' ne' verificabile ne' riproducibile a posteriori — e' provenienza
    # tanto quanto il ledger delle immagini tenute.
    reject_rows.sort(key=lambda r: (r["source"], r["source_path"]))
    with (out_dir / "rejects.jsonl").open("w", encoding="utf-8") as f:
        for row in reject_rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    # Coda di review NSFW: gli scarti dell'euristica PIU' le immagini tenute con
    # incarnato sopra la soglia di attenzione. La decisione sul bucket e' del
    # proprietario (mandato + regole-agenti-ml.md): questo file esiste per
    # rendergliela possibile su id concreti, non per chiudere la questione.
    nsfw_rejected = [
        {"id": r.get("id_base"), "status": "scartata", "source_path": r["source_path"],
         "reason_short": r["reason_short"], "reason": r["reason"]}
        for r in reject_rows if r["reason_short"].startswith("nsfw-")
    ]
    nsfw_rows = nsfw_rejected + sorted(nsfw_watch, key=lambda r: -r["skin_fraction"])
    with (out_dir / "nsfw-review.jsonl").open("w", encoding="utf-8") as f:
        for row in nsfw_rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    # Round-robin fra i bucket (ordine alfabetico dei nomi, deterministico) cosi'
    # il campione finale copre tutti i percorsi di caption anche se un bucket
    # (dcss-32) e' molto piu' numeroso degli altri tre.
    sample_captions = []
    bucket_names_sorted = sorted(sample_captions_by_bucket)
    for round_idx in range(SAMPLE_CAPTIONS_PER_BUCKET):
        for bname in bucket_names_sorted:
            caps = sample_captions_by_bucket[bname]
            if round_idx < len(caps):
                sample_captions.append(caps[round_idx])

    report = build_mining_report(by_source_total, buckets, reason_counter, reason_by_source,
                                 reason_samples, len(candidates), len(ledger_rows), sample_captions,
                                 caption_audit, nsfw_rejected, nsfw_watch)
    report_path = out_dir / "mining_report.json"
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print_summary(report)
    return report


def build_mining_report(by_source_total, buckets, reason_counter, reason_by_source,
                        reason_samples, n_candidates, n_kept, sample_captions,
                        caption_audit, nsfw_rejected, nsfw_watch):
    by_bucket = {name: len(items) for name, items in buckets.items()}
    by_bucket_source = defaultdict(Counter)
    by_bucket_split = defaultdict(Counter)
    for name, items in buckets.items():
        for it in items:
            by_bucket_source[name][it["cand"]["source"]] += 1
            by_bucket_split[name][it["split"]] += 1

    reason_by_source_readable = {
        f"{reason}|{source}": n for (reason, source), n in reason_by_source.items()
    }

    return {
        "generated_at": datetime.datetime.now().isoformat(timespec="seconds"),
        "candidates_total": n_candidates,
        "candidates_by_source": dict(by_source_total),
        "kept_total": n_kept,
        "kept_by_bucket": by_bucket,
        "kept_by_bucket_by_source": {k: dict(v) for k, v in by_bucket_source.items()},
        "kept_by_bucket_by_split": {k: dict(v) for k, v in by_bucket_split.items()},
        "rejected_total": sum(reason_counter.values()),
        "rejected_by_reason": dict(reason_counter.most_common()),
        "rejected_by_reason_by_source": reason_by_source_readable,
        "reject_samples_by_reason": {k: v for k, v in reason_samples.items()},
        "rejects_full_list": "dataset/lora-v1-research/rejects.jsonl (una riga per scarto)",
        "sample_captions": sample_captions[:10],
        "caption_policy_by_source": CAPTION_POLICY,
        # Audit caption/contenuto: quante caption nominano un colore che sul canvas
        # copre meno di CAPTION_AUDIT_MIN_COVERAGE. Sul primario deve stare a zero
        # per costruzione (le caption vengono dai pixel); sugli altri bucket e' la
        # misura di quanto ci si puo' fidare dei metadati della sorgente.
        "caption_color_audit": {
            name: {**data,
                   "unsupported_ratio": round(data["with_unsupported_color"] / max(1, data["captions"]), 4),
                   "threshold": CAPTION_AUDIT_MIN_COVERAGE}
            for name, data in caption_audit.items()
        },
        "caption_color_audit_note": (
            "Zero su primary-128 e tinyhero e' per costruzione (caption dai pixel o testo fisso "
            "senza colori). Il numero alto di secondary-nouns-32 NON e' l'errore misurato sul "
            "vecchio primario: le caption Nouns nominano il colore degli OCCHIALI, che su un "
            "personaggio 32px copre pochi pixel per natura — l'audit non sa distinguere 'colore "
            "di un dettaglio piccolo' da 'colore che non c'e''. Su DCSS i colori sono dentro il "
            "nome reale dell'asset. Il controllo resta utile come sentinella: se un giorno il "
            "primario risale sopra zero, le caption hanno ricominciato ad affermare cose non viste."),
        "single_subject_gate": {
            "applies_to": PRIMARY_BUCKET,
            "why_primary_only": ("le tre sorgenti secondarie sono pack curati a grana nativa dove "
                                 "lo sprite riempie la propria cella per costruzione; li' il gate "
                                 "equivalente e' quello sul frame sorgente"),
            "thresholds": {
                "canvas_fill_max": CANVAS_FILL_MAX,
                "canvas_fill_min": CANVAS_FILL_MIN,
                "canvas_border_occ_max": CANVAS_BORDER_OCC_MAX,
                "canvas_straight_edge_max": CANVAS_STRAIGHT_EDGE_MAX,
                "canvas_vsym_max": CANVAS_VSYM_MAX,
            },
            "rejected": reason_counter.get("scene-not-single-subject", 0),
        },
        "nsfw_review": {
            "policy": ("euristica prudente (mandato: 'in dubbio scarta'): parole chiave sulla "
                       "caption sorgente + frazione di incarnato misurata sui pixel del canvas. "
                       "Il filtro a parole chiave e' empiricamente un no-op su Limbicnation "
                       "(0/500 caption contengono una keyword): il lavoro lo fa la misura sui pixel."),
            "keyword_rejected": sum(1 for r in nsfw_rejected if r["reason_short"] == "nsfw-keyword"),
            "skin_rejected": sum(1 for r in nsfw_rejected if r["reason_short"] == "nsfw-skin-fraction"),
            "skin_threshold": SKIN_FRACTION_MAX,
            "kept_above_watch_threshold": len(nsfw_watch),
            "watch_threshold": NSFW_WATCH_SKIN_FRACTION,
            "rejected_ids": [r["id"] for r in nsfw_rejected],
            "kept_ids_to_review": [r["id"] for r in nsfw_watch],
            "full_list": "dataset/lora-v1-research/nsfw-review.jsonl",
            "owner_decision": ("PENDENTE — l'euristica non e' una garanzia: nel campione casuale "
                               "del giudice il tasso di nudi era ~5% (limbicnation-00454, -00432, "
                               "-00113). Nessun training sul bucket primario prima di una decisione "
                               "esplicita del proprietario sugli id qui elencati."),
        },
    }


def print_summary(report):
    print("\n== dataset/lora-v1-research — riepilogo mining ==\n")
    print(f"Candidati totali: {report['candidates_total']} {report['candidates_by_source']}")
    print(f"Tenute: {report['kept_total']}  Scartate: {report['rejected_total']}")
    print("\n-- per bucket --")
    for bucket, n in report["kept_by_bucket"].items():
        by_src = report["kept_by_bucket_by_source"].get(bucket, {})
        by_split = report["kept_by_bucket_by_split"].get(bucket, {})
        print(f"  {bucket:26s} {n:5d}  fonti={by_src}  split={by_split}")
    print("\n-- scarti per motivo --")
    for reason, n in report["rejected_by_reason"].items():
        print(f"  {n:5d}  {reason}")
    print("\n-- 10 caption di esempio --")
    for c in report["sample_captions"]:
        print(f"  {c}")
    print("\n-- audit caption/contenuto (caption che nominano un colore sotto la soglia) --")
    for bucket, data in report["caption_color_audit"].items():
        print(f"  {bucket:26s} {data['with_unsupported_color']:5d}/{data['captions']:<5d} "
              f"({data['unsupported_ratio'] * 100:.1f}%)")
    nsfw = report["nsfw_review"]
    print("\n-- NSFW --")
    print(f"  scartate: {nsfw['keyword_rejected']} per parola chiave, {nsfw['skin_rejected']} per "
          f"incarnato > {nsfw['skin_threshold'] * 100:.0f}%")
    print(f"  tenute da rivedere (incarnato >= {nsfw['watch_threshold'] * 100:.0f}%): "
          f"{nsfw['kept_above_watch_threshold']} -> {nsfw['full_list']}")
    print(f"  {nsfw['owner_decision']}")


# =============================================================================
# CLI
# =============================================================================
def build_arg_parser():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("download", help="scarica le 4 sorgenti in dataset-sources/ (riprendibile)")

    p_mine = sub.add_parser("mine", help="filtri + assemblaggio in dataset/lora-v1-research/")
    p_mine.add_argument("--limit", type=int, default=None,
                        help="limita i candidati PER SORGENTE (debug/smoke test)")
    p_mine.add_argument("--out", default=str(OUT_DIR))
    p_mine.add_argument("--sources-dir", default=str(SOURCES_DIR))

    sub.add_parser("kaggle", help="adatta il pacchetto Kaggle (copia da dataset/lora-v0/kaggle/)")

    p_all = sub.add_parser("all", help="download + mine + kaggle")
    p_all.add_argument("--limit", type=int, default=None)

    return ap


def main():
    args = build_arg_parser().parse_args()
    if args.cmd == "download":
        download_all()
    elif args.cmd == "mine":
        run_mine(Path(args.out), Path(args.sources_dir), args.limit)
    elif args.cmd == "kaggle":
        build_kaggle_package(OUT_DIR)
    elif args.cmd == "all":
        download_all()
        run_mine(OUT_DIR, SOURCES_DIR, args.limit)
        build_kaggle_package(OUT_DIR)
    return 0


KAGGLE_README = """# Worldsmelt LoRA v1-research — pacchetto Kaggle (prova RESEARCH-ONLY)

Adattato il {today} da `dataset/lora-v0/kaggle/` (mandato orchestratore 07/08,
{sha_note}) per puntare a `dataset/lora-v1-research/primary-128/` — SOLO il
bucket a grana fine (Limbicnation, ricampionato a 128 logici), non i tre
bucket secondari. Le due basi e gli iperparametri rank-16 restano INVARIATI
(mandato, letterale): questo pacchetto cambia solo il dataset, non il metodo.

**Provenienza dubbia, autorizzata SOLO per questa prova** (docs/ai-production/
regole-agenti-ml.md, divieto 3: "un asset 'research' non diventa
'commercial-clean' per decisione estetica"). Conseguenza diretta, non un bug:

## Il preflight di questo pacchetto FALLISCE DI PROPOSITO

`train_lora_v0.py` (copiato INVARIATO, stessa logica di `dataset/lora-v0/kaggle/`)
verifica `license_id` contro una whitelist di produzione
(`cc0`/`cc0-1.0`/`own`/`commissioned`). Il ledger di questo pacchetto dichiara
`license_id: apache-2.0` (Limbicnation, vera licenza della sorgente) — FUORI
whitelist per costruzione. Il preflight quindi si ferma con `SystemExit`
PRIMA di toccare la GPU, anche se `run_policy.yaml: approved_gpu_run` fosse
`true`. **Non e' un difetto da correggere**: e' il cancello previsto dal
divieto 3, che impedisce di far scivolare una prova a provenienza dubbia
dentro un run che sembra "pronto" senza una decisione umana esplicita.

Per lanciare comunque questa prova (mai senza il proprietario): editare
consapevolmente `LICENSE_WHITELIST` in una copia locale di
`train_lora_v0.py` con una nota che spiega perche', MAI nel repo condiviso —
la promozione a "commercial-clean" richiede una verifica di licenza vera,
non un bypass di una riga.

## SECONDO cancello: decisione NSFW del proprietario (PENDENTE)

Il bucket primario viene da una sorgente che contiene nudo. La pipeline
scarta cio' che l'euristica sull'incarnato riconosce ({nsfw_rejected} immagini in
questa corsa) e lascia in lista di review le tenute sopra la soglia di
attenzione ({nsfw_watch} immagini, `nsfw-review.jsonl` — nel tar e in
`dataset/lora-v1-research/`). **Non e' una garanzia**: un campione casuale sul
pacchetto bocciato del 07/08 dava ~5% di nudi (limbicnation-00454, -00432,
-00113). `run_policy.yaml` porta percio' `owner_nsfw_decision: pending`:
finche' resta cosi', il pacchetto non si lancia — la decisione e' del
proprietario, presa sugli id elencati, non di chi ha scritto la pipeline.

## Cosa contiene

```text
kaggle/
├── run.py                                     # entrypoint del kernel (path aggiornati)
├── train_lora_v0.py                           # trainer INVARIATO (stesso nome file, stessa logica)
├── kernel-metadata.json                       # id/slug ancora REPLACE_WITH_KAGGLE_USERNAME
├── README.md                                  # questo file
├── worldsmelt-lora-v1-research-dataset.tar.gz  # dati (SOLO primary-128) + codice + nsfw-review.jsonl
├── requirements-ml.txt                        # invariato
├── run_policy.yaml                            # approved_gpu_run: false + research_only: true
├── eval-prompts-lora-v1-research.json          # stessi 20 prompt congelati, metadati aggiornati
└── configs/
    ├── lora-v1-research-dreamshaper8.yaml       # rank 16, iperparametri INVARIATI
    └── lora-v1-research-dreamshaper-pixelart.yaml
```

Hash del tar consegnato:

```text
sha256: {tar_sha}
```

**Niente e' stato lanciato su Kaggle** (stessa regola di lora-v0): questo
pacchetto e' pronto e documentato, non avviato.
"""


def build_kaggle_package(out_dir: Path):
    src_kaggle = REPO_ROOT / "dataset" / "lora-v0" / "kaggle"
    required = [
        "run.py", "train_lora_v0.py", "requirements-ml.txt", "run_policy.yaml",
        "kernel-metadata.json", "eval-prompts-lora-v0.json",
        "configs/lora-v0-dreamshaper8.yaml", "configs/lora-v0-dreamshaper-pixelart.yaml",
    ]
    missing = [r for r in required if not (src_kaggle / r).is_file()]
    if missing:
        dst = out_dir / "kaggle"
        dst.mkdir(parents=True, exist_ok=True)
        (dst / "README.md").write_text(
            "# Pacchetto Kaggle non generato\n\n"
            f"dataset/lora-v0/kaggle/ non e' completo (mancano: {missing}); "
            "adattare a mano seguendo dataset/lora-v0/kaggle/README.md come riferimento.\n",
            encoding="utf-8")
        log(f"AVVISO: dataset/lora-v0/kaggle/ incompleto ({missing}), scritto solo un README segnaposto")
        return

    primary_dir = out_dir / PRIMARY_BUCKET
    if not primary_dir.is_dir() or not any(primary_dir.glob("*.png")):
        log(f"AVVISO: {primary_dir} vuota o assente — esegui 'mine' prima di 'kaggle'")
        return

    dst = out_dir / "kaggle"
    (dst / "configs").mkdir(parents=True, exist_ok=True)

    def adapt_text(relpath, new_name=None):
        text = (src_kaggle / relpath).read_text(encoding="utf-8")
        text = text.replace("lora-v0", "lora-v1-research")  # solo i token con trattino
                                                              # (path/tar/config/experiment_id);
                                                              # "train_lora_v0.py" usa underscore,
                                                              # non lo tocca (verificato, vedi README)
        out_name = new_name or Path(relpath).name.replace("lora-v0", "lora-v1-research")
        (dst / out_name).write_text(text, encoding="utf-8")
        return dst / out_name

    adapt_text("run.py", "run.py")
    adapt_text("train_lora_v0.py", "train_lora_v0.py")  # nome file INVARIATO di proposito
    shutil.copy(src_kaggle / "requirements-ml.txt", dst / "requirements-ml.txt")

    nsfw = {}
    report_path = out_dir / "mining_report.json"
    if report_path.is_file():
        nsfw = json.loads(report_path.read_text(encoding="utf-8")).get("nsfw_review", {})

    policy_text = (src_kaggle / "run_policy.yaml").read_text(encoding="utf-8")
    policy_text += (
        "\n# Campo aggiunto per questa prova (mandato 07/08, regole-agenti-ml.md divieto 3):\n"
        "# ricorda perche' il preflight di train_lora_v0.py FALLISCE di proposito su questo\n"
        "# pacchetto (license_id fuori whitelist) — vedi kaggle/README.md.\n"
        "research_only: true\n"
        "\n# SECONDO cancello, indipendente dalle licenze: il bucket primario viene da una\n"
        "# sorgente con nudo. L'euristica sull'incarnato ha scartato quello che ha saputo\n"
        f"# riconoscere ({nsfw.get('skin_rejected', 'n/d')} immagini) e ha lasciato in lista di review\n"
        f"# {nsfw.get('kept_above_watch_threshold', 'n/d')} immagini tenute (nsfw-review.jsonl), ma non e' una\n"
        "# garanzia. Finche' questo campo resta 'pending' il pacchetto non va lanciato:\n"
        "# la decisione e' del proprietario, su id concreti, non di chi ha scritto la pipeline.\n"
        "owner_nsfw_decision: pending\n"
    )
    (dst / "run_policy.yaml").write_text(policy_text, encoding="utf-8")

    for cfg_name in ("lora-v0-dreamshaper8.yaml", "lora-v0-dreamshaper-pixelart.yaml"):
        adapt_text(f"configs/{cfg_name}", None)
        # adapt_text scrive in dst/, non in dst/configs/: sposta al posto giusto.
        wrong = dst / cfg_name.replace("lora-v0", "lora-v1-research")
        right = dst / "configs" / cfg_name.replace("lora-v0", "lora-v1-research")
        wrong.rename(right)

    eval_v0 = json.loads((src_kaggle / "eval-prompts-lora-v0.json").read_text(encoding="utf-8"))
    eval_v0["stage"] = "lora-v1-research"
    eval_v0["purpose"] = (
        "Suite di valutazione congelata (STESSI 20 prompt/2 seed di dataset/lora-v0/kaggle/, "
        "riusati apposta per confrontare le due prove) per i due run rank-16 di questo pacchetto "
        "RESEARCH-ONLY (dataset/lora-v1-research/kaggle/), dataset primary-128 (Limbicnation, "
        "provenienza dubbia autorizzata solo per questa prova). Non lanciata da questo pacchetto: "
        "l'esecuzione la fa l'orchestratore col proprietario, se e quando decide di superare il "
        "cancello del preflight (vedi kaggle/README.md).")
    if "notes" in eval_v0 and "character_category_gap" in eval_v0["notes"]:
        eval_v0["notes"]["character_category_gap"] = (
            "Nota v0 non riportata qui invariata: la composizione per categoria di "
            "primary-128 e' DIVERSA da lora-v0 (fonte Limbicnation, non i pack CC0 di "
            "oggetti/nemici) — vedi dataset/lora-v1-research/mining_report.json per i "
            "conteggi reali invece di assumerli.")
    (dst / "eval-prompts-lora-v1-research.json").write_text(
        json.dumps(eval_v0, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    kernel_meta = {
        "id": "REPLACE_WITH_KAGGLE_USERNAME/worldsmelt-lora-v1-research",
        "title": "Worldsmelt LoRA v1-research - rank16 (DreamShaper8 + DreamShaper PixelArt) [RESEARCH-ONLY]",
        "code_file": "run.py",
        "language": "python",
        "kernel_type": "script",
        "is_private": True,
        "enable_gpu": True,
        "enable_internet": True,
        "dataset_sources": [
            "REPLACE_WITH_KAGGLE_USERNAME/worldsmelt-lora-v1-research-dataset",
            "REPLACE_WITH_KAGGLE_USERNAME/worldsmelt-teacher-checkpoints",
        ],
        "competition_sources": [], "kernel_sources": [], "model_sources": [],
    }
    (dst / "kernel-metadata.json").write_text(
        json.dumps(kernel_meta, indent=2) + "\n", encoding="utf-8")

    # -- tar del pacchetto: SOLO primary-128 (mandato, letterale) --
    ledger_all = [json.loads(l) for l in (out_dir / "ledger.jsonl").read_text(encoding="utf-8").splitlines() if l.strip()]
    primary_rows = [r for r in ledger_all if r["bucket"] == PRIMARY_BUCKET]
    tar_path = dst / "worldsmelt-lora-v1-research-dataset.tar.gz"
    with tarfile.open(tar_path, "w:gz") as tf:
        for png in sorted(primary_dir.glob("*.png")):
            tf.add(png, arcname=f"images/{png.name}")
        for txt in sorted(primary_dir.glob("*.txt")):
            tf.add(txt, arcname=f"images/{txt.name}")
        ledger_tmp = dst / "_primary_ledger.jsonl"
        with ledger_tmp.open("w", encoding="utf-8") as f:
            for r in primary_rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        tf.add(ledger_tmp, arcname="ledger.jsonl")
        ledger_tmp.unlink()
        tf.add(dst / "eval-prompts-lora-v1-research.json", arcname="eval-prompts-lora-v1-research.json")
        review_path = out_dir / "nsfw-review.jsonl"
        if review_path.is_file():
            # Viaggia DENTRO il pacchetto: la lista di review non serve a niente
            # se resta nel repo mentre i dati sono su Kaggle.
            tf.add(review_path, arcname="nsfw-review.jsonl")
        tf.add(dst / "train_lora_v0.py", arcname="train_lora_v0.py")
        tf.add(dst / "requirements-ml.txt", arcname="requirements-ml.txt")
        tf.add(dst / "run_policy.yaml", arcname="run_policy.yaml")
        for cfg in sorted((dst / "configs").glob("*.yaml")):
            tf.add(cfg, arcname=f"configs/{cfg.name}")

    tar_sha = sha256_of_file(tar_path)
    readme = KAGGLE_README.format(
        today=datetime.date.today().isoformat(),
        sha_note="stesso schema del pacchetto lora-v0",
        tar_sha=tar_sha,
        nsfw_rejected=nsfw.get("skin_rejected", "n/d"),
        nsfw_watch=nsfw.get("kept_above_watch_threshold", "n/d"),
    )
    (dst / "README.md").write_text(readme, encoding="utf-8")
    log(f"pacchetto Kaggle -> {dst} ({len(primary_rows)} righe primary-128, tar sha256={tar_sha[:12]}...)")


if __name__ == "__main__":
    sys.exit(main())
