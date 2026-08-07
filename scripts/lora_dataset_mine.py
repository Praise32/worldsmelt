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

BUCKET B (opzione scelta dal proprietario, 07/08/2026, "grana 32 pura, categorie
coerenti"): un QUINTO bucket, 'primary-32B', indipendente dai quattro sopra e
dalla loro logica di instradamento (route_bucket() non lo vede, process_candidate()
non lo tocca — vedi process_candidate_bucket_b()). Tre sorgenti, tutte dichiarate
a grana 32 nativa, ma qui la grana e' un CANCELLO e non solo una misura: ogni
candidato la cui grana stimata da round-trip non e' ESATTAMENTE 32 viene
scartato con motivo, comprese le immagini gia' accettate in secondary-dcss-32
(li' la grana nominale bastava, qui no — 2 delle 1873 non superano il cancello
piu' severo, vedi mining_report.json:bucket_b):
  dcss-32        RIUSATA da secondary-dcss-32 (gia' minata: si copiano le 512px
                 gia' prodotte, non si rielaborano i sorgenti) se grain_estimated==32
  oga-creatures  https://opengameart.org/content/assorted-32x32-creatures (CC0,
                 AndHeGames) — la pagina offre uno SPRITESHEET 288x288 (9x9 celle
                 32px), non uno zip di file singoli come atteso a mandato: si
                 scarica il PNG e si affetta in tile, le celle vuote (padding)
                 si scartano prima ancora del mining
  curated-32     LOCALE: assets/curated/*, identificate via
                 dataset/lora-v0/ledger.jsonl (canvas_size==32 e source_path
                 sotto assets/curated/) ma RIPROCESSATE dal file ORIGINALE (non
                 dal derivato 512px di lora-v0): e' il ledger v0 che identifica
                 quali file candidare, mai la fonte dei pixel
Caption sempre "pixel art game sprite, <categoria>, <soggetto>, <tema se
noto>, single subject, plain background" — build_caption_dcss() e' gia' in
questo formato (riusata invariata), build_caption_oga()/build_caption_curated()
lo replicano. Per oga-creatures il posto del <soggetto> lo prende cio' che si
MISURA sul canvas (proporzione + colori dominanti, describe_canvas_content()):
la pagina offre un solo spritesheet e non esiste un nome per-creatura, e il nome
del file non e' un soggetto. Il token "single subject" e' su tutte le righe ma
poggia SOLO sul frame sorgente (single_subject_checks nel ledger, sezione
dedicata nel README del pacchetto Kaggle e voce 19 di known-issues.md): a grana
32 i gate di scena del bucket primario sono mal tarati, e applicarli male
sarebbe peggio che dichiarare il limite.
Prima dello split c'e' un cancello percettivo (enforce_split_perceptual_separation)
che usa la STESSA metrica del preflight del trainer: due varianti di colore dello
stesso asset hanno nomi diversi e finirebbero su lati opposti dello split.
Split 90/10 e ledger/rejects nello STESSO ledger.jsonl/rejects.jsonl
degli altri bucket (distinti dal campo "bucket"/"target_bucket"), non file
separati — mining_report.json porta le statistiche del bucket B sotto la
chiave "bucket_b", senza toccare le chiavi del run originale.

Uso:
  python3 scripts/lora_dataset_mine.py download            # le 4 sorgenti, riprendibile
  python3 scripts/lora_dataset_mine.py mine [--limit N]     # filtri + assemblaggio (4 bucket originali)
  python3 scripts/lora_dataset_mine.py kaggle                # adatta il pacchetto Kaggle (primary-128)
  python3 scripts/lora_dataset_mine.py all                  # download + mine + kaggle
  python3 scripts/lora_dataset_mine.py mine-b [--limit N]   # bucket B: richiede 'mine' gia' fatto
                                                              # (riusa secondary-dcss-32), scarica
                                                              # oga-creatures da solo, assembla
                                                              # primary-32B e ripunta il pacchetto
                                                              # Kaggle su di esso
"""

import argparse
import colorsys
import datetime
import hashlib
import inspect
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
NOMINAL_GRAIN = {"dcss-32": 32, "tinyhero-64": 64, "nouns-32": 32,
                 # Le due sorgenti nuove del bucket B (vedi sotto): dichiarate a
                 # grana 32 nativa come dcss-32, ma la' dove dcss-32 usa questo
                 # valore incondizionatamente (route_bucket(), mai un cancello),
                 # process_candidate_bucket_b() lo affianca a un cancello vero
                 # sulla grana MISURATA — NOMINAL_GRAIN resta la base per il
                 # ricampionamento (resample_ratio_for()), non il gate.
                 "oga-creatures": 32, "curated-32": 32}
SECONDARY_BUCKET = {
    "dcss-32": "secondary-dcss-32",
    "tinyhero-64": "secondary-tinyhero-64",
    "nouns-32": "secondary-nouns-32",
}
PRIMARY_BUCKET = "primary-128"
PRIMARY_CANVAS = 128

# --- Bucket B (opzione scelta dal proprietario 07/08, "grana 32 pura, categorie
# coerenti"): vedi il paragrafo dedicato in cima al file. Deliberatamente FUORI
# da SECONDARY_BUCKET/route_bucket(): quella tabella instrada i quattro bucket
# originali (intoccati, mandato letterale), il bucket B ha la propria pipeline
# (process_candidate_bucket_b(), run_mine_bucket_b()) cosi' i due mondi non
# possono influenzarsi a vicenda per una modifica futura all'uno o all'altro.
BUCKET_B = "primary-32B"
BUCKET_B_CANVAS = 32
BUCKET_B_SOURCES = ("dcss-32", "oga-creatures", "curated-32")

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
    # Verificata sulla pagina (07/08, non assunta): riquadro "License(s): CC0"
    # con link a creativecommons.org/publicdomain/zero/1.0/, autore AndHeGames.
    # Nessuno zip in pagina (a differenza del mandato, che ne assumeva uno):
    # solo due PNG, lo spritesheet nativo (creatures_3.png, 288x288, 9x9 celle
    # 32px) e un export 2x (creatures_3-export_1.png, 576x576) — si scarica e
    # affetta SOLO il nativo, l'export raddoppierebbe la grana misurata.
    "oga-creatures": {
        "license_id": "cc0-1.0",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "dataset_url": "https://opengameart.org/content/assorted-32x32-creatures",
        "author": "AndHeGames (OpenGameArt)",
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


# --- oga-creatures (bucket B): niente zip in pagina (verificato, vedi
# LICENSE_INFO["oga-creatures"]), solo uno spritesheet. Scarico + affettatura
# sono due passi idempotenti separati cosi' un rilancio non riscarica ne'
# riaffetta se non serve.
OGA_SHEET_URL = "https://opengameart.org/sites/default/files/creatures_3.png"
OGA_TILE = 32


def download_oga_creatures(dest_dir: Path):
    raw_dir = dest_dir / "raw"
    sheet_path = raw_dir / "creatures_3.png"
    if not sheet_path.is_file():
        log("[oga-creatures] scarico lo spritesheet")
        http_download_file(OGA_SHEET_URL, sheet_path)
    else:
        log("[oga-creatures] spritesheet gia' presente, salto il download")

    info_path = dest_dir / "_source_info.json"
    lic = LICENSE_INFO["oga-creatures"]
    info = {
        "source_name": "oga-creatures",
        "source_url": lic["dataset_url"],
        "sheet_url": OGA_SHEET_URL,
        "license_id": lic["license_id"], "license_url": lic["license_url"],
        "author": lic["author"],
        "note": ("pagina verificata 07/08: nessuno zip, solo due PNG (spritesheet "
                 "nativo + export 2x) — usato SOLO il nativo, vedi commento sopra "
                 "OGA_SHEET_URL"),
        "downloaded_at": datetime.date.today().isoformat(),
        "sheet_sha256": sha256_of_file(sheet_path),
    }
    info_path.write_text(json.dumps(info, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    slice_oga_sheet(dest_dir)


def slice_oga_sheet(dest_dir: Path, tile=OGA_TILE):
    """Affetta lo spritesheet in celle tile x tile, scartando quelle
    COMPLETAMENTE trasparenti (padding della griglia, non creature) PRIMA
    ancora che arrivino a process_candidate_bucket_b(): non sono scarti di
    qualita', sono celle vuote per costruzione — contarle come candidati
    gonfierebbe 'candidati totali' con cornici vuote che nessun filtro deve
    spiegare. Idempotente: se tiles/ esiste gia' e non e' vuota, non riaffetta."""
    tiles_dir = dest_dir / "tiles"
    if tiles_dir.is_dir() and any(tiles_dir.iterdir()):
        log(f"[oga-creatures] tile gia' presenti in {tiles_dir}, salto l'affettatura")
        return
    sheet_path = dest_dir / "raw" / "creatures_3.png"
    im = Image.open(sheet_path).convert("RGBA")
    w, h = im.size
    if w % tile or h % tile:
        raise RuntimeError(f"[oga-creatures] spritesheet {w}x{h} non e' multiplo di {tile}px")
    tiles_dir.mkdir(parents=True, exist_ok=True)
    n_saved = n_empty = 0
    for ty in range(h // tile):
        for tx in range(w // tile):
            cell = im.crop((tx * tile, ty * tile, tx * tile + tile, ty * tile + tile))
            if cell.split()[-1].getextrema()[1] == 0:  # alpha massima 0: cella vuota
                n_empty += 1
                continue
            cell.save(tiles_dir / f"creatures_3-r{ty:02d}-c{tx:02d}.png")
            n_saved += 1
    log(f"[oga-creatures] {n_saved} tile salvate, {n_empty} celle vuote scartate (padding)")


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
    # Bucket B (vedi paragrafo dedicato in cima al file): curated-32 non scarica
    # niente, legge assets/curated/ + dataset/lora-v0/ledger.jsonl gia' sul
    # disco — solo oga-creatures ha una rete da toccare.
    download_oga_creatures(SOURCES_DIR / "oga-creatures")


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


# --- Candidati del bucket B (oga-creatures, curated-32) — vedi paragrafo
# dedicato in cima al file. dcss-32 per il bucket B non ha un iter_* proprio:
# si riusano le righe gia' in secondary-dcss-32/ (run_mine_bucket_b()), niente
# ririlettura dei file sorgente.
def iter_oga_candidates(src_dir: Path, limit=None):
    lic = LICENSE_INFO["oga-creatures"]
    tiles_dir = src_dir / "tiles"
    if not tiles_dir.is_dir():
        slice_oga_sheet(src_dir)
    files = sorted(tiles_dir.glob("*.png"))
    if limit:
        files = files[:limit]
    for path in files:
        stem = path.stem  # es. "creatures_3-r02-c04"
        yield {
            "source": "oga-creatures", "path": path, "id_base": f"oga-{stem}",
            "subject_base_id": f"oga-{stem}", "caption_raw": None,
            # theme None e non "creatures_3": quello e' il nome del file dello
            # spritesheet, non un tema — vedi build_caption_oga().
            "category": "creature", "theme": None,
            "original_url": f"{lic['dataset_url']}#{stem}",
            "license": lic,
        }


def load_v0_curated32_rows():
    """Identifica i candidati curated-32 SOLO tramite dataset/lora-v0/ledger.jsonl
    (mandato, letterale: 'riusa ... per identificare le immagini'): canvas_size==32
    (il builder v0 le ha gia' normalizzate su quel gradino della scala 32/64/128)
    E source_path sotto assets/curated/ (gli 8 di assets/art-library/ restano
    fuori, non sono 'le immagini di assets/curated' del mandato). Il ledger v0
    NON e' la fonte dei pixel — solo l'indice: process_candidate_bucket_b()
    riparte SEMPRE dal file originale in source_path, mai dal derivato 512px
    di lora-v0/images/."""
    ledger_path = REPO_ROOT / "dataset" / "lora-v0" / "ledger.jsonl"
    rows = []
    with ledger_path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            if row.get("canvas_size") == 32 and row.get("source_path", "").startswith("assets/curated/"):
                rows.append(row)
    return rows


def iter_curated32_candidates(limit=None):
    rows = load_v0_curated32_rows()
    if limit:
        rows = rows[:limit]
    for row in rows:
        path = REPO_ROOT / row["source_path"]
        # name_family raggruppa le varianti note dello stesso soggetto (es. le
        # due colorazioni 'shmup-cruiser-gray-a/-b'): stesso ruolo di
        # subject_base_id per DCSS, evita che due quasi-duplicati finiscano uno
        # in train e uno in val. Il ledger v0 lo popola sempre (verificato); id
        # resta il fallback per righe che in futuro non lo avessero.
        subject_key = row.get("name_family") or f"curated-{row['id']}"
        yield {
            "source": "curated-32", "path": path, "id_base": f"curated-{row['id']}",
            "subject_base_id": subject_key, "caption_raw": None,
            "category": row.get("category"), "theme": (row.get("theme_tags") or [None])[0],
            "original_url": row.get("original_url"),
            "license": {
                # Licenza PER RIGA (curated-32 non ha un'unica sorgente): il
                # ledger v0 la porta gia' verificata pack per pack (author/
                # license_id/license_url), qui si abbassa solo license_id a
                # minuscolo per coerenza con le altre righe di questo ledger
                # (tutte lowercase: "cc0-1.0", non "CC0-1.0").
                "license_id": (row.get("license_id") or "").strip().lower(),
                "license_url": row.get("license_url"),
                "dataset_url": row.get("original_url"),
                "author": row.get("author"),
            },
            "v0_row": row,
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
    "oga-creatures": ("page-tag + content-derived: nessun nome per-creatura in pagina (uno "
                      "spritesheet, non file singoli come atteso a mandato), quindi categoria "
                      "'creature' dai tag DELLA PAGINA + proporzione e colori dominanti MISURATI "
                      "sul canvas (stessa macchina di limbicnation). Il nome del file "
                      "('creatures_3') non entra: e' un nome di file, non un soggetto"),
    "curated-32": "curated-manifest: categoria/soggetto/tema dal ledger dataset/lora-v0 (a sua volta da assets/curated/manifest.json)",
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


# Formato comune al bucket B (mandato, letterale): "pixel art game sprite,
# <categoria>, <soggetto>, <tema se noto>, single subject, plain background".
# build_caption_dcss() sopra e' gia' in questo formato (nessuna modifica, e'
# la stessa funzione per secondary-dcss-32 e per il riuso nel bucket B).
def build_caption_oga(item):
    """La pagina OGA offre UNO spritesheet: non esiste un nome per-creatura.
    La prima stesura ripiegava su un testo fisso che infilava 'creatures_3' —
    il nome del FILE — nella caption, con due difetti misurati: un token
    spazzatura insegnato al modello, e 45 caption su 45 byte-identiche, cioe'
    zero segnale per distinguere una creatura dall'altra. Qui resta la sola
    categoria che LA PAGINA dichiara ('creature', dai suoi tag) e si aggiunge
    cio' che si puo' contare sui pixel, la stessa macchina di Limbicnation e
    per lo stesso motivo: mai un'asserzione che lo script non ha verificato."""
    parts = ["pixel art game sprite", item["cand"]["category"]]
    parts += describe_canvas_content(item)
    parts += ["single subject", "plain background"]
    return ", ".join(p for p in parts if p)


def build_caption_curated(cand):
    row = cand["v0_row"]
    parts = ["pixel art game sprite", cand["category"] or "prop",
             row.get("subject") or cand["id_base"], cand["theme"],
             "single subject", "plain background"]
    return ", ".join(p for p in parts if p)


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
def describe_canvas_content(item):
    """I due soli attributi che questo script sa CONTARE sui pixel del canvas
    finale: proporzione della bbox e colori dominanti. Estratta da
    build_caption_from_content() (Limbicnation) perche' serve identica anche a
    oga-creatures, che di nome per-creatura non ne ha nessuno."""
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
    tones = (" and ".join(named) + " tones") if named else "multicolored"
    return [f"{proportion} subject", tones]


def build_caption_from_content(item):
    parts = (["pixel art game sprite"] + describe_canvas_content(item)
             + ["single subject", "plain background"])
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
# DISTANZA PERCETTIVA fra due firme 64px — UNICA definizione, condivisa fra
# questo script (cancello di split del bucket B) e il preflight del trainer
# Kaggle, dove viene INIETTATA da build_kaggle_package() prendendo il sorgente
# di queste stesse funzioni con inspect.getsource(). Due copie a mano
# divergerebbero senza che nessuno se ne accorga: qui non possono.
#
# Perche' esiste. Il trainer v0 (dataset/lora-v0/kaggle/train_lora_v0.py)
# definisce "acceso" un pixel con alpha > 16, corretto per il dataset v0, che
# ha il fondo TRASPARENTE. I bucket di lora-v1-research invece compongono su
# CANVAS_BG_RGB opaco (SD1.5 non ha canale alpha), quindi ogni pixel del canvas
# risulta acceso, l'unione e' 4096/4096 px e due sprite piccoli e DIVERSI
# differiscono solo per pochi punti percentuali: sul bucket B il preflight
# riportava 6324 "fughe di split percettive" tutte false. La correzione misura
# il fondo invece di assumerlo (modale dell'anello di bordo) e ricade sul solo
# test alpha quando il fondo e' davvero trasparente, cosi' la stessa funzione
# resta valida anche per il dataset v0.
# =============================================================================
PERCEPTUAL_BG_TOL = 24      # scostamento dal fondo oltre cui un pixel e' soggetto
PERCEPTUAL_COLOR_TOL = 24   # scostamento di colore oltre cui due pixel sono "diversi"
PERCEPTUAL_SELFCHECK_PAIRS = 64
_perceptual_selfchecked = 0


def background_key(im):
    """Colore di fondo MISURATO: il piu' frequente sull'anello di 1 px del
    bordo. Non e' una costante perche' la stessa funzione deve valere sia per
    il fondo piatto opaco dei bucket v1-research sia per il fondo trasparente
    del dataset v0 — e un valore cablato in due file diversi e' esattamente il
    tipo di assunzione che ha prodotto il difetto che questo blocco corregge.
    Il pareggio si rompe sul colore (chiave del max) per restare deterministico
    fra processi."""
    px = im.load()
    w, h = im.size
    counts = {}
    for x in range(w):
        for y in (0, h - 1):
            counts[px[x, y]] = counts.get(px[x, y], 0) + 1
    for y in range(1, h - 1):
        for x in (0, w - 1):
            counts[px[x, y]] = counts.get(px[x, y], 0) + 1
    return max(counts.items(), key=lambda kv: (kv[1], kv[0]))[0]


def is_foreground_pixel(px, bg):
    """DEFINIZIONE di "pixel di soggetto", usata dall'implementazione di
    riferimento; foreground_mask() ne e' la versione vettoriale."""
    if px[3] <= 16:
        return False
    if bg[3] <= 16:
        return True  # fondo trasparente (dataset v0): il solo alpha basta
    return max(abs(px[0] - bg[0]), abs(px[1] - bg[1]), abs(px[2] - bg[2])) > PERCEPTUAL_BG_TOL


def foreground_mask(im, bg=None):
    """Maschera 0/255 dei pixel di soggetto, in C dentro Pillow. Memoizzata
    sull'oggetto immagine: il preflight confronta ogni firma di validazione
    con TUTTE quelle di training, quindi senza cache la stessa maschera si
    ricalcolerebbe migliaia di volte (misurato: e' la differenza fra ~35 s e
    diversi minuti di CPU bruciati su una macchina a GPU pagata a ore)."""
    from PIL import Image as _Image, ImageChops as _ImageChops
    if bg is None:
        cached = getattr(im, "_ws_fg_mask", None)
        if cached is not None:
            return cached
        bg = background_key(im)
        remember = True
    else:
        remember = False
    r, g, b, a = im.split()
    opaque = a.point(lambda v: 255 if v > 16 else 0)
    if bg[3] <= 16:
        mask = opaque
    else:
        d = _ImageChops.lighter(
            _ImageChops.lighter(
                _ImageChops.difference(r, _Image.new("L", im.size, bg[0])),
                _ImageChops.difference(g, _Image.new("L", im.size, bg[1]))),
            _ImageChops.difference(b, _Image.new("L", im.size, bg[2])))
        mask = _ImageChops.multiply(d.point(lambda v: 255 if v > PERCEPTUAL_BG_TOL else 0), opaque)
    if remember:
        im._ws_fg_mask = mask
    return mask


def perceptual_distance_reference(a, b) -> float:
    """Implementazione a pixel, lenta e leggibile: e' la DEFINIZIONE della
    metrica (frazione di pixel diversi sull'unione delle aree di SOGGETTO, non
    sul canvas: due icone piccole e diverse dentro lo stesso canvas
    differiscono solo su una manciata di pixel del totale, e una soglia
    calcolata sul canvas le dichiarerebbe identiche). perceptual_distance()
    deve restituire lo stesso numero, e _perceptual_selfcheck() lo verifica su
    dati veri invece di dichiararlo."""
    pa, pb = a.load(), b.load()
    bga, bgb = background_key(a), background_key(b)
    w, h = a.size
    union = diff = 0
    for y in range(h):
        for x in range(w):
            p1, p2 = pa[x, y], pb[x, y]
            on1 = is_foreground_pixel(p1, bga)
            on2 = is_foreground_pixel(p2, bgb)
            if not (on1 or on2):
                continue
            union += 1
            if on1 != on2 or max(abs(p1[0] - p2[0]), abs(p1[1] - p2[1]),
                                 abs(p1[2] - p2[2])) > PERCEPTUAL_COLOR_TOL:
                diff += 1
    return diff / union if union else 0.0


def _perceptual_selfcheck(a, b, fast_value):
    """Le prime PERCEPTUAL_SELFCHECK_PAIRS coppie REALI del dataset in esame
    vengono ricalcolate anche con l'implementazione di riferimento: se le due
    divergessero, ci si ferma qui invece di dichiarare "preflight OK" con una
    metrica sbagliata. Costo misurato ~1,5 ms a coppia, cioe' meno di 0,1 s su
    un preflight che ne confronta centinaia di migliaia."""
    global _perceptual_selfchecked
    if _perceptual_selfchecked >= PERCEPTUAL_SELFCHECK_PAIRS:
        return
    _perceptual_selfchecked += 1
    ref = perceptual_distance_reference(a, b)
    if abs(ref - fast_value) > 1e-9:
        raise SystemExit(
            f"INCOERENZA nella distanza percettiva: la versione vettoriale dice {fast_value!r}, "
            f"quella di riferimento {ref!r}. Le due devono coincidere per definizione: "
            f"non proseguire con una metrica di cui non si sa quale sia quella giusta.")


def perceptual_distance(a, b) -> float:
    """Vedi perceptual_distance_reference(): stessa metrica, calcolata dentro
    Pillow. a e b sono firme RGBA della stessa dimensione."""
    from PIL import ImageChops as _ImageChops
    ma, mb = foreground_mask(a), foreground_mask(b)
    union = _ImageChops.lighter(ma, mb)
    u = sum(union.histogram()[128:])
    if u == 0:
        return 0.0  # due canvas interamente di fondo: identici per definizione
    ar, ag, ab = a.split()[:3]
    br, bg_, bb = b.split()[:3]
    color = _ImageChops.lighter(
        _ImageChops.lighter(_ImageChops.difference(ar, br), _ImageChops.difference(ag, bg_)),
        _ImageChops.difference(ab, bb)).point(
            lambda v: 255 if v > PERCEPTUAL_COLOR_TOL else 0)
    changed = _ImageChops.lighter(_ImageChops.difference(ma, mb),
                                  _ImageChops.multiply(color, union))
    value = sum(changed.histogram()[128:]) / u
    _perceptual_selfcheck(a, b, value)
    return value


# Ordine di iniezione nel trainer del bucket B: le dipendenze prima di chi le usa.
PERCEPTUAL_BLOCK_FUNCS = (background_key, is_foreground_pixel, foreground_mask,
                          perceptual_distance_reference, _perceptual_selfcheck,
                          perceptual_distance)


def perceptual_block_source() -> str:
    """Il blocco qui sopra come TESTO, per sostituire perceptual_distance() nella
    copia adattata del trainer. Le costanti si riscrivono dal valore corrente,
    non si copiano a mano."""
    head = (f"PERCEPTUAL_BG_TOL = {PERCEPTUAL_BG_TOL}\n"
            f"PERCEPTUAL_COLOR_TOL = {PERCEPTUAL_COLOR_TOL}\n"
            f"PERCEPTUAL_SELFCHECK_PAIRS = {PERCEPTUAL_SELFCHECK_PAIRS}\n"
            f"_perceptual_selfchecked = 0\n")
    body = "\n\n".join(inspect.getsource(fn).rstrip() for fn in PERCEPTUAL_BLOCK_FUNCS)
    return head + "\n\n" + body + "\n"


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


# =============================================================================
# BUCKET B — process_candidate_bucket_b() (opzione scelta dal proprietario
# 07/08, "grana 32 pura"). Deliberatamente una funzione A SE', non un
# parametro in piu' su process_candidate(): quella funzione e' il cuore dei
# quattro bucket esistenti (mandato: "NON toccare i bucket esistenti"), e
# infilarci un ramo condizionale in piu' per un quinto bucket sperimentale
# avrebbe messo a rischio di regressione esattamente cio' che non si puo'
# toccare. Le funzioni di basso livello (estimate_grain, tbp.*,
# foreground_colors_posterized, resample_ratio_for, compose_logical_canvas,
# canvas_shape_metrics, canvas_pixel_stats, dhash64) restano CONDIVISE — solo
# l'ORDINE dei cancelli e le due differenze sotto sono duplicati apposta.
#
# Due differenze deliberate rispetto a process_candidate():
#   1. la grana e' un CANCELLO, non solo una misura instradata per sorgente:
#      qualunque candidato la cui grana stimata da round-trip non sia
#      ESATTAMENTE BUCKET_B_CANVAS (32) viene scartato — anche i candidati
#      dcss-32 gia' accettati in secondary-dcss-32, dove la grana nominale
#      del pack bastava (vedi route_bucket()) e la misura non era un cancello;
#   2. nessun gate di scena (punto c2 di process_candidate, CANVAS_FILL_MAX
#      e affini): quel gate esiste SOLO per il canvas 512-logico di
#      Limbicnation (soggetti disegnati a mano dentro un frame che PUO'
#      contenere un diorama intero) — qui il canvas e' 32, la stessa grana
#      nativa del contenuto, quindi lo sprite riempie la propria cella per
#      costruzione come le sorgenti secondarie esistenti (stessa nota gia'
#      in route_bucket()). Nessun gate NSFW: nessuna delle tre sorgenti e'
#      Limbicnation.
# =============================================================================
def process_candidate_bucket_b(cand):
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

    # -- grana: CANCELLO (differenza 1 sopra) --
    grain, grain_mse = estimate_grain(rgba.convert("RGB"))
    if grain != BUCKET_B_CANVAS:
        return reject("grain-not-32-roundtrip",
                      f"grana stimata da round-trip = {grain} (mse={grain_mse:.2f} alla grana "
                      f"scelta) != {BUCKET_B_CANVAS}: il bucket B accetta solo grana 32 pura",
                      grain=grain)

    alpha_lo, _alpha_hi = rgba.split()[-1].getextrema()
    if alpha_lo < 250:
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

    canvas, canvas_mask, canvas_visible, logical_size = compose_logical_canvas(
        crop, ratio, BUCKET_B_CANVAS)
    shape = canvas_shape_metrics(canvas_visible, BUCKET_B_CANVAS)
    stats = canvas_pixel_stats(canvas, canvas_mask, canvas_visible, BUCKET_B_CANVAS)
    if shape is None or stats is None:
        return reject("subject-imperceptible",
                      "nessun pixel del soggetto si distingue dal canvas", grain=grain)

    # -- niente gate di scena qui (differenza 2 sopra) --

    if shape["fill"] < CANVAS_FILL_MIN:
        return reject("canvas-nearly-empty",
                      f"solo il {shape['fill'] * 100:.1f}% del canvas mostra qualcosa "
                      f"(< {CANVAS_FILL_MIN * 100:.0f}%): soggetto quasi invisibile sul fondo chiaro",
                      grain=grain)
    if stats["contrast"] < CONTRAST_MIN_RATIO:
        return reject("low-contrast",
                      f"contrasto medio soggetto/canvas {stats['contrast']:.2f} < "
                      f"{CONTRAST_MIN_RATIO}", grain=grain)
    if stats["imperceptible_fraction"] > IMPERCEPTIBLE_MAX_FRACTION:
        return reject("subject-imperceptible",
                      f"{stats['imperceptible_fraction'] * 100:.0f}% dei pixel del soggetto "
                      f"sotto contrasto {IMPERCEPTIBLE_CONTRAST} col canvas", grain=grain)

    return {
        "status": "ok", "kind": "processed", "cand": cand, "crop": crop, "grain": grain,
        "grain_mse": grain_mse,
        "effective_grain": effective_grain, "resample_ratio": ratio,
        "bucket": BUCKET_B, "canvas_size": BUCKET_B_CANVAS,
        "bg_removal_mode": bg_removal_mode, "n_colors": n_colors,
        "logical_size": list(logical_size),
        "source_logical_size": [logical_w, logical_h],
        "fg_fraction": round(fg_fraction, 4),
        "canvas_shape": shape, "canvas_stats": stats,
        "single_subject_verified": True,
        "single_subject_checks": ["source-frame"],  # niente gate di canvas per il bucket B
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
# CANCELLO DI SPLIT PERCETTIVO (solo bucket B) — assign_splits() separa per
# CHIAVE di soggetto, e la chiave e' un nome: 'stone_2_blue' e 'stone_2_green'
# sono lo stesso amuleto in due colori ma due chiavi diverse, quindi potevano
# finire una in val e una in train. Il preflight del trainer lo considera una
# fuga (giustamente: la validazione misurerebbe qualcosa che il modello ha gia'
# visto) e si ferma. Qui si chiude PRIMA, con la stessa identica metrica del
# preflight — non un'euristica che gli somiglia — cosi' il pacchetto consegnato
# non puo' fallire su questo cancello per costruzione.
#
# Il gruppo di soggetto che sfora si sposta INTERO in train: val puo' solo
# rimpicciolirsi, quindi il giro converge (e le coppie da ricontrollare al giro
# dopo sono solo quelle contro gli elementi appena spostati).
# =============================================================================
SPLIT_PERCEPTUAL_MAX = 0.15   # deve valere DEDUP_MERGE_SCORE del trainer (verificato a build)
SPLIT_PERCEPTUAL_SIG_PX = 64  # deve valere SIGNATURE_PX del trainer (verificato a build)
SPLIT_SEPARATION_MAX_ROUNDS = 6


def bucket_b_item_label(item):
    return (item["ledger_row"]["id"] if item["kind"] == "reused"
            else item["cand"]["id_base"])


def bucket_b_signature(item):
    """Firma 64px calcolata sull'immagine FINALE (512px), la stessa che il
    preflight aprira' dal tar: per le righe riusate si legge il PNG gia' sul
    disco, per le nuove si ricompone il canvas e lo si porta a 512 come fara'
    il salvataggio. Nessuna scorciatoia sul canvas 32px: sarebbe equivalente
    solo finche' i fattori di scala restano interi, cioe' un'assunzione in piu'
    da mantenere vera."""
    if item["kind"] == "reused":
        with Image.open(REPO_ROOT / item["ledger_row"]["image_path"]) as im:
            return im.convert("RGBA").resize(
                (SPLIT_PERCEPTUAL_SIG_PX, SPLIT_PERCEPTUAL_SIG_PX), Image.NEAREST)
    canvas, _mask, _visible, _logical = compose_logical_canvas(
        item["crop"], item["resample_ratio"], item["canvas_size"])
    return upscale_to_final(canvas, item["canvas_size"]).convert("RGBA").resize(
        (SPLIT_PERCEPTUAL_SIG_PX, SPLIT_PERCEPTUAL_SIG_PX), Image.NEAREST)


def enforce_split_perceptual_separation(kept, signatures):
    """Ritorna la lista degli spostamenti fatti (vuota se lo split era gia'
    pulito). `signatures` e' parallela a `kept`."""
    moves = []
    frontier = None  # None = primo giro, confronto completo val x train
    for round_idx in range(SPLIT_SEPARATION_MAX_ROUNDS):
        val_idx = [i for i, it in enumerate(kept) if it["split"] == "val"]
        against = [i for i, it in enumerate(kept) if it["split"] == "train"] \
            if frontier is None else frontier
        offenders = {}
        for vi in val_idx:
            for ti in against:
                d = perceptual_distance(signatures[vi], signatures[ti])
                if d < SPLIT_PERCEPTUAL_MAX:
                    offenders[vi] = (ti, d)
                    break
        if not offenders:
            return moves
        keys = {kept[vi]["cand"]["subject_base_id"] for vi in offenders}
        newly = [i for i, it in enumerate(kept)
                 if it["split"] == "val" and it["cand"]["subject_base_id"] in keys]
        for i in newly:
            kept[i]["split"] = "train"
        for vi in sorted(offenders):
            ti, d = offenders[vi]
            moves.append({
                "round": round_idx + 1,
                "moved_to_train": bucket_b_item_label(kept[vi]),
                "subject_key": kept[vi]["cand"]["subject_base_id"],
                "near_duplicate_of": bucket_b_item_label(kept[ti]),
                "distance": round(d, 4),
                "group_size": sum(1 for i in newly
                                  if kept[i]["cand"]["subject_base_id"]
                                  == kept[vi]["cand"]["subject_base_id"]),
            })
        log(f"  cancello di split, giro {round_idx + 1}: {len(offenders)} soggetti "
            f"quasi-duplicati spostati in train ({len(newly)} immagini)")
        frontier = newly
    raise SystemExit(
        f"cancello di split percettivo: nessuna convergenza in "
        f"{SPLIT_SEPARATION_MAX_ROUNDS} giri — va guardato a mano, non allargata la soglia.")


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
    if source == "oga-creatures":
        return build_caption_oga(item)
    if source == "curated-32":
        return build_caption_curated(cand)
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
# BUCKET B — driver (run_mine_bucket_b): dcss-32 RIUSATA da secondary-dcss-32
# (nessuna rilettura dei sorgenti), oga-creatures + curated-32 attraverso
# process_candidate_bucket_b(). Ledger/rejects sono gli STESSI file degli
# altri quattro bucket (righe distinte dal campo "bucket"/"target_bucket"),
# mining_report.json prende una chiave "bucket_b" in piu' senza toccare le
# altre. Vedi il paragrafo dedicato in cima al file per il perche'.
# =============================================================================
def load_ledger_rows(path: Path):
    if not path.is_file():
        return []
    rows = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    return rows


def parse_dhash(perceptual_hash: str) -> int:
    """'dhash64:869696178686a627' -> int, per riusare hamming()/global_dedup()
    su righe gia' scritte nel ledger senza riaprire l'immagine."""
    return int(perceptual_hash.split(":", 1)[1], 16)


def merge_into_mining_report(out_dir: Path, key: str, data):
    report_path = out_dir / "mining_report.json"
    report = {}
    if report_path.is_file():
        try:
            report = json.loads(report_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            report = {}
    report[key] = data
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def run_mine_bucket_b(out_dir: Path, sources_dir: Path, limit_per_source=None):
    log("bucket B ('grana 32 pura'): raccolgo candidati...")

    existing_ledger = load_ledger_rows(out_dir / "ledger.jsonl")
    dcss_rows = [r for r in existing_ledger if r["bucket"] == SECONDARY_BUCKET["dcss-32"]]
    if not dcss_rows:
        raise SystemExit(
            f"bucket B richiede secondary-dcss-32 gia' minato in {out_dir / 'ledger.jsonl'} "
            "(mandato: DCSS 'gia' minato, 1873') — lanciare prima 'mine'.")
    if limit_per_source:
        dcss_rows = dcss_rows[:limit_per_source]
    dcss_row_by_id = {r["id"]: r for r in dcss_rows}

    candidates = []
    candidates += list(iter_oga_candidates(sources_dir / "oga-creatures", limit_per_source))
    candidates += list(iter_curated32_candidates(limit_per_source))
    by_source_total = Counter(c["source"] for c in candidates)
    by_source_total["dcss-32"] = len(dcss_rows)
    log(f"{len(dcss_rows)} righe dcss-32 riusate da secondary-dcss-32 + "
        f"{len(candidates)} candidati nuovi (oga-creatures + curated-32)")

    reason_counter = Counter()
    reason_by_source = Counter()
    reason_samples = defaultdict(list)
    all_reject_rows = []

    def record_reject(source, source_path, id_base, reason_short, reason, grain):
        reason_counter[reason_short] += 1
        reason_by_source[(reason_short, source)] += 1
        row = {"source": source, "source_path": source_path, "id_base": id_base,
               "reason_short": reason_short, "reason": reason, "grain": grain,
               "target_bucket": BUCKET_B}
        all_reject_rows.append(row)
        if len(reason_samples[reason_short]) < REPORT_SAMPLES_PER_REASON:
            reason_samples[reason_short].append(source_path)

    # -- dcss-32: riuso, cancello di grana sulla misura GIA' registrata in
    # secondary-dcss-32 (stesso algoritmo, stesso file: nessuna riprocessione) --
    reused_items = []
    for row in dcss_rows:
        if row["grain_estimated"] != BUCKET_B_CANVAS:
            record_reject(
                "dcss-32", row["image_path"], row["id"], "grain-not-32-bucket-b",
                f"gia' accettata in secondary-dcss-32 (grana nominale 32, mai un cancello li'), "
                f"ma esclusa da {BUCKET_B}: grana stimata da round-trip = {row['grain_estimated']} "
                f"!= {BUCKET_B_CANVAS}", row["grain_estimated"])
            continue
        reused_items.append({
            "kind": "reused", "ledger_row": row, "dhash": parse_dhash(row["perceptual_hash"]),
            "grain": row["grain_estimated"],
            "cand": {"source": "dcss-32", "id_base": row["id"], "subject_base_id": row["subject_key"]},
        })

    # -- oga-creatures + curated-32: pipeline completa (isolamento/qualita'/contrasto) --
    processed_survivors = []
    t0 = time.time()
    for i, cand in enumerate(candidates, start=1):
        result = process_candidate_bucket_b(cand)
        if result["status"] == "ok":
            processed_survivors.append(result)
        else:
            record_reject(cand["source"], repo_rel(cand["path"]), cand["id_base"],
                          result["reason_short"], result["reason"], result.get("grain"))
        if i % 200 == 0:
            log(f"  {i}/{len(candidates)} processati ({time.time() - t0:.0f}s)")
    log(f"[oga-creatures+curated-32] {len(processed_survivors)}/{len(candidates)} sopravvissuti "
        f"ai filtri per-immagine ({time.time() - t0:.0f}s)")

    # -- dedup percettivo GLOBALE cross-sorgente (le tre del bucket B insieme,
    # non contro gli altri quattro bucket: sono esperimenti indipendenti) --
    kept, dup_rejects = global_dedup(reused_items + processed_survivors)
    for r in dup_rejects:
        cref = r["cand"]
        source_path = (repo_rel(cref["path"]) if "path" in cref
                       else dcss_row_by_id[cref["id_base"]]["image_path"])
        record_reject(cref["source"], source_path, cref["id_base"], r["reason_short"], r["reason"],
                      r.get("grain"))
    log(f"dedup globale: {len(kept)} sopravvissuti dopo {len(dup_rejects)} duplicati")

    assign_splits(kept, BUCKET_B)

    # Cancello di split percettivo: stessa metrica del preflight del trainer
    # (vedi enforce_split_perceptual_separation). Costa un giro completo
    # val x train sulle firme 64px, decine di secondi — il prezzo per non
    # consegnare un pacchetto che fallisce il proprio preflight.
    t_sig = time.time()
    signatures = [bucket_b_signature(it) for it in kept]
    log(f"firme 64px per il cancello di split: {len(signatures)} ({time.time() - t_sig:.0f}s)")
    t_sep = time.time()
    split_moves = enforce_split_perceptual_separation(kept, signatures)
    log(f"cancello di split percettivo: {len(split_moves)} spostamenti "
        f"({time.time() - t_sep:.0f}s)")

    out_dir.mkdir(parents=True, exist_ok=True)
    bucket_dir = out_dir / BUCKET_B
    if bucket_dir.is_dir():
        shutil.rmtree(bucket_dir)  # stessa pulizia-prima-di-ripopolare di run_mine(): niente orfani
    bucket_dir.mkdir(parents=True)

    now = datetime.datetime.now().isoformat(timespec="seconds")
    ledger_rows_b = []
    saved_paths = []
    seen_ids = set()
    SAMPLE_CAPTIONS_PER_SOURCE = 10
    sample_captions_by_source = defaultdict(list)
    captions_by_source = defaultdict(list)
    caption_audit = {"captions": 0, "with_unsupported_color": 0, "examples": []}

    for item in kept:
        cand = item["cand"]
        if item["kind"] == "reused":
            # Copia dei pixel gia' prodotti (bit-identici a secondary-dcss-32/):
            # nessuna ricomposizione, la riga del ledger e' quella originale con
            # bucket/percorsi/split/added_at aggiornati e un campo in piu' che
            # dichiara il riuso (mai silenzioso).
            row = dict(item["ledger_row"])
            img_id = row["id"]
            seen_ids.add(img_id)
            img_path = bucket_dir / f"{img_id}.png"
            txt_path = bucket_dir / f"{img_id}.txt"
            shutil.copy(REPO_ROOT / row["image_path"], img_path)
            shutil.copy(REPO_ROOT / row["caption_path"], txt_path)
            row["bucket"] = BUCKET_B
            row["image_path"] = repo_rel(img_path)
            row["caption_path"] = repo_rel(txt_path)
            row["split"] = item["split"]
            row["added_at"] = now
            row["reused_from"] = "secondary-dcss-32"
            ledger_rows_b.append(row)
            saved_paths.append(img_path)
            if len(sample_captions_by_source["dcss-32"]) < SAMPLE_CAPTIONS_PER_SOURCE:
                sample_captions_by_source["dcss-32"].append(row["caption"])
            continue

        base_id = slugify(cand["id_base"])
        img_id = base_id
        n = 2
        while img_id in seen_ids:
            img_id = f"{base_id}-{n}"
            n += 1
        seen_ids.add(img_id)

        canvas, _mask, _visible, logical_size = compose_logical_canvas(
            item["crop"], item["resample_ratio"], item["canvas_size"])
        final_img = upscale_to_final(canvas, item["canvas_size"])
        img_path = bucket_dir / f"{img_id}.png"
        txt_path = bucket_dir / f"{img_id}.txt"
        caption = build_caption(item)
        final_img.save(img_path, "PNG")
        txt_path.write_text(caption + "\n", encoding="utf-8")
        saved_paths.append(img_path)
        if len(sample_captions_by_source[cand["source"]]) < SAMPLE_CAPTIONS_PER_SOURCE:
            sample_captions_by_source[cand["source"]].append(caption)
        captions_by_source[cand["source"]].append(caption)

        caption_audit["captions"] += 1
        defects = caption_color_defects(caption, item["canvas_stats"]["color_coverage"])
        if defects:
            caption_audit["with_unsupported_color"] += 1
            if len(caption_audit["examples"]) < REPORT_SAMPLES_PER_REASON:
                caption_audit["examples"].append({"id": img_id, "caption": caption, "defects": defects})

        lic = cand["license"]
        ledger_rows_b.append({
            "id": img_id, "bucket": BUCKET_B,
            "image_path": repo_rel(img_path), "caption_path": repo_rel(txt_path),
            "caption": caption, "caption_policy": CAPTION_POLICY[cand["source"]],
            "caption_source_raw": cand.get("caption_raw"),
            "source": cand["source"], "url": cand["original_url"],
            "license_declared": lic["license_id"], "license_id": lic["license_id"],
            "license_url": lic["license_url"], "license_alt": lic.get("license_alt"),
            "license_snapshot_path": "nessuno snapshot scaricato (prova research-only): vedi license_url",
            "author": lic["author"], "provenance": "research-only",
            "grain_estimated": item["grain"], "grain_mse": round(item["grain_mse"], 3),
            "effective_grain_used": item["effective_grain"], "canvas_size": item["canvas_size"],
            "logical_content_size": list(logical_size), "fg_fraction": item["fg_fraction"],
            "n_colors": item["n_colors"], "bg_removal_mode": item["bg_removal_mode"],
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
            "source_sha256": sha256_of_file(cand["path"]), "derived_sha256": sha256_of_file(img_path),
            "perceptual_hash": f"dhash64:{item['dhash']:016x}", "subject_key": cand["subject_base_id"],
            "split": item["split"], "added_at": now,
        })

    sheet = build_contact_sheet(saved_paths)
    sheet.save(out_dir / f"contact-sheet-{BUCKET_B}.png")
    log(f"[{BUCKET_B}] {len(ledger_rows_b)} immagini, contact sheet salvato")

    # -- scrittura: STESSI ledger.jsonl/rejects.jsonl degli altri bucket
    # (mandato: 'ledger+rejects come gli altri bucket'), non file separati —
    # si tolgono le righe di un'eventuale corsa precedente di QUESTO bucket
    # (stessa pulizia-prima-di-ripopolare di run_mine(), idempotenza) e si
    # riscrive l'insieme, senza toccare le righe degli altri quattro bucket. --
    merged_ledger = [r for r in existing_ledger if r["bucket"] != BUCKET_B] + ledger_rows_b
    merged_ledger.sort(key=lambda r: (r["bucket"], r["id"]))
    with (out_dir / "ledger.jsonl").open("w", encoding="utf-8") as f:
        for row in merged_ledger:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    existing_rejects = load_ledger_rows(out_dir / "rejects.jsonl")
    merged_rejects = ([r for r in existing_rejects if r.get("target_bucket") != BUCKET_B]
                      + all_reject_rows)
    merged_rejects.sort(key=lambda r: (r["source"], r["source_path"]))
    with (out_dir / "rejects.jsonl").open("w", encoding="utf-8") as f:
        for row in merged_rejects:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    sample_captions = []
    for round_idx in range(SAMPLE_CAPTIONS_PER_SOURCE):
        for sname in sorted(sample_captions_by_source):
            caps = sample_captions_by_source[sname]
            if round_idx < len(caps):
                sample_captions.append(caps[round_idx])

    by_source_kept = Counter(r["source"] for r in ledger_rows_b)
    by_split_kept = Counter(r["split"] for r in ledger_rows_b)

    # Quanto costerebbe applicare i gate di scena del bucket primario a QUESTO
    # bucket: si misura, non si stima a parole. Serve a sostenere (o smentire)
    # la ragione per cui non girano — vedi "single_subject_claim" sotto.
    scene_gate_hits = Counter()
    for r in ledger_rows_b:
        shape = {"fill": r["canvas_fill"], "border_occ": r["canvas_border_occ"],
                 "straight_edge": r["canvas_straight_edge"], "vsym": r["canvas_vsym"]}
        if scene_rejection_reason(shape) is None:
            continue
        # Il PRIMO cancello che cede, nello stesso ordine di
        # scene_rejection_reason(): quale sia conta piu' del totale, perche' e'
        # quello che dice se il cancello e' tarato per questa grana o no.
        if shape["fill"] >= CANVAS_FILL_MAX:
            scene_gate_hits["canvas_fill"] += 1
        elif shape["border_occ"] >= CANVAS_BORDER_OCC_MAX:
            scene_gate_hits["canvas_border_occ"] += 1
        elif shape["straight_edge"] >= CANVAS_STRAIGHT_EDGE_MAX:
            scene_gate_hits["canvas_straight_edge"] += 1
        else:
            scene_gate_hits["canvas_vsym"] += 1
    scene_gate_total = sum(scene_gate_hits.values())
    scene_gate_simulation = {
        "would_reject": scene_gate_total,
        "of_rows": len(ledger_rows_b),
        "fraction": round(scene_gate_total / max(1, len(ledger_rows_b)), 4),
        "by_first_failing_gate": dict(scene_gate_hits.most_common()),
    }
    report_b = {
        "generated_at": now, "bucket": BUCKET_B,
        "grain_gate": {
            "required_grain": BUCKET_B_CANVAS,
            "note": ("round-trip: qualunque candidato con grana stimata != 32 e' scartato "
                     "(reason_short='grain-not-32-roundtrip' per oga/curated, "
                     "'grain-not-32-bucket-b' per dcss-32 gia' accettata altrove a grana nominale)"),
        },
        "candidates_by_source": dict(by_source_total),
        "kept_total": len(ledger_rows_b),
        "kept_by_source": dict(by_source_kept),
        "kept_by_split": dict(by_split_kept),
        "rejected_total": sum(reason_counter.values()),
        "rejected_by_reason": dict(reason_counter.most_common()),
        "rejected_by_reason_by_source": {f"{reason}|{source}": n
                                         for (reason, source), n in reason_by_source.items()},
        "reject_samples_by_reason": dict(reason_samples),
        "rejects_full_list": "dataset/lora-v1-research/rejects.jsonl (target_bucket='primary-32B')",
        "sample_captions": sample_captions[:10],
        "caption_policy_by_source": {s: CAPTION_POLICY[s] for s in BUCKET_B_SOURCES},
        # Quanto SEGNALE portano davvero le caption costruite qui: caption tutte
        # uguali sono caption che non insegnano niente, e la prima stesura ne
        # aveva 45 su 45 identiche per oga-creatures. Misurato, non promesso.
        "caption_distinctness": {
            src: {"captions": len(caps), "distinct": len(set(caps))}
            for src, caps in sorted(captions_by_source.items())
        },
        "caption_color_audit": {
            "captions": caption_audit["captions"],
            "with_unsupported_color": caption_audit["with_unsupported_color"],
            "unsupported_ratio": round(
                caption_audit["with_unsupported_color"] / max(1, caption_audit["captions"]), 4),
            "threshold": CAPTION_AUDIT_MIN_COVERAGE, "examples": caption_audit["examples"],
            "note": ("copre SOLO oga-creatures+curated-32 (caption costruite qui); le righe dcss-32 "
                     "riusano la caption gia' scritta e verificata dal run originale (build_caption_dcss, "
                     "invariata)."),
        },
        "split_perceptual_gate": {
            "metric": ("stessa funzione del preflight del trainer (perceptual_distance, iniettata "
                       "da build_kaggle_package): frazione di pixel diversi sull'unione delle aree "
                       "di soggetto, firme 64px"),
            "threshold": SPLIT_PERCEPTUAL_MAX,
            "moves": split_moves,
            "note": ("assign_splits separa per NOME di soggetto: due varianti di colore dello "
                     "stesso asset hanno nomi diversi e potevano finire su lati opposti. Questo "
                     "cancello le rimette insieme (il gruppo sfora -> tutto in train) prima che "
                     "il preflight se ne accorga su Kaggle."),
        },
        "single_subject_claim": {
            "asserted_in_captions": len(ledger_rows_b),
            "backed_by": "source-frame",
            "scene_gates_if_applied": scene_gate_simulation,
            "limit": ("TUTTE le caption del bucket B dicono 'single subject', ma la prova e' solo "
                      "il frame sorgente (una cella di spritesheet / un file del pack): i gate di "
                      "scena sul canvas finale NON girano su questo bucket. Esistono controesempi "
                      "reali ereditati da secondary-dcss-32 — es. "
                      "'dcss-gui-spells-necromancy-control-undead-old', che in una cella sola ha "
                      "spettro + fiamma + umanoide + cuore. Applicarli in blocco a grana 32 "
                      "scarterebbe la quota misurata in 'scene_gates_if_applied' (in gran parte "
                      "su straight_edge, mal tarato a questa grana): la scelta e' di NON "
                      "applicarli e di DICHIARARLO, non di applicarli male. Il campo per-riga "
                      "single_subject_checks dice la stessa cosa riga per riga."),
        },
    }
    merge_into_mining_report(out_dir, "bucket_b", report_b)

    print("\n== dataset/lora-v1-research — riepilogo mining BUCKET B (primary-32B) ==\n")
    print(f"Candidati per sorgente: {report_b['candidates_by_source']}")
    print(f"Tenute: {report_b['kept_total']}  Scartate: {report_b['rejected_total']}")
    print(f"  per sorgente: {report_b['kept_by_source']}  per split: {report_b['kept_by_split']}")
    print("\n-- scarti per motivo --")
    for reason, n in report_b["rejected_by_reason"].items():
        print(f"  {n:5d}  {reason}")
    print("\n-- campione caption --")
    for c in report_b["sample_captions"]:
        print(f"  {c}")
    aud = report_b["caption_color_audit"]
    print(f"\naudit caption/contenuto (oga+curated): {aud['with_unsupported_color']}/{aud['captions']} "
          f"({aud['unsupported_ratio'] * 100:.1f}%)")
    return report_b


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

    sub.add_parser("kaggle", help="adatta il pacchetto Kaggle (copia da dataset/lora-v0/kaggle/, primary-128)")

    p_all = sub.add_parser("all", help="download + mine + kaggle")
    p_all.add_argument("--limit", type=int, default=None)

    p_mine_b = sub.add_parser(
        "mine-b", help="bucket B ('grana 32 pura'): richiede 'mine' gia' fatto (riusa "
                       "secondary-dcss-32), scarica oga-creatures da solo, assembla primary-32B "
                       "e ripunta il pacchetto Kaggle su di esso")
    p_mine_b.add_argument("--limit", type=int, default=None,
                          help="limita i candidati PER SORGENTE (debug/smoke test)")
    p_mine_b.add_argument("--out", default=str(OUT_DIR))
    p_mine_b.add_argument("--sources-dir", default=str(SOURCES_DIR))
    p_mine_b.add_argument("--skip-download", action="store_true",
                          help="salta il download di oga-creatures (gia' fatto in precedenza)")

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
    elif args.cmd == "mine-b":
        out_dir, sources_dir = Path(args.out), Path(args.sources_dir)
        if not args.skip_download:
            download_oga_creatures(sources_dir / "oga-creatures")  # curated-32 non scarica niente
        run_mine_bucket_b(out_dir, sources_dir, args.limit)
        build_kaggle_package(out_dir, BUCKET_B)
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


KAGGLE_README_BUCKET_B = """# Worldsmelt LoRA v1-research — pacchetto Kaggle (BUCKET B: primary-32B)

Adattato il {today} da `dataset/lora-v0/kaggle/` per puntare a
`dataset/lora-v1-research/primary-32B/` — il bucket "grana 32 pura" scelto dal
proprietario (mandato 07/08): dcss-32 (riusata da secondary-dcss-32, gia'
minata) + oga-creatures + curated-32, TUTTE verificate a grana 32 tramite
round-trip (non solo dichiarate). Le due basi e gli iperparametri rank-16
restano INVARIATI (stessa regola di primary-128: questo pacchetto cambia solo
il dataset, non il metodo).

## Esito REALE del preflight di questo pacchetto

{preflight_verdict}

Il preflight e' stato **eseguito davvero** su questo pacchetto, non dedotto:
`build_kaggle_package()` lo lancia in-processo sul contenuto esatto che finisce
nel tar e incolla qui il suo esito. Se un giorno tornasse a fallire, questa
sezione lo direbbe — e' generata dall'esito, non scritta a mano.

Comando per rifarlo a mano, dopo aver estratto il tar (nessuna GPU richiesta):

```bash
python3 train_lora_v0.py --config configs/lora-v1-research-dreamshaper8.yaml --preflight-only
```

**Licenze (cancello 1)**: {license_whitelist_note}

**Fughe di split percettive (cancello 2)**: {perceptual_note}

**Cancello di split lato miner**: {split_gate_note}

Un preflight verde NON significa "pronto al lancio senza supervisione":
`run_policy.yaml` porta comunque `approved_gpu_run: false` (il proprietario lo
alza esplicitamente quando decide di lanciare) e `research_only: true` per lo
scope dichiarato dell'intero modulo `lora-v1-research` (una prova di confronto
fra bucket, non ancora il dataset commerciale del gioco — vedi il docstring in
cima a `scripts/lora_dataset_mine.py`). Nessun cancello NSFW: nessuna delle tre
sorgenti e' Limbicnation, l'unica su cui gira l'euristica.

## Limite dichiarato: "single subject" nelle caption

Tutte le {n_rows} caption di questo bucket contengono `single subject`, ma cio'
che e' stato verificato e' il **frame sorgente** (una cella di spritesheet, un
file del pack): i gate di scena sul canvas finale, che su `primary-128`
scartano diorami e tilemap, **non girano su questo bucket**. Il ledger lo dice
riga per riga (`single_subject_checks: ["source-frame"]`), e qui lo diciamo per
esteso perche' e' una riga che entra nel training.

Il limite e' reale, non teorico: `dcss-gui-spells-necromancy-control-undead-old`
mette in una sola cella spettro + fiamma + umanoide + cuore, e la sua caption
dice comunque `single subject`. Applicare i gate di scena in blocco a grana 32
scarterebbe {scene_gate_would_reject} righe su {n_rows} ({scene_gate_fraction}),
per primo cancello che cede: `{scene_gate_breakdown}` — cioe' quasi tutto su
`straight_edge`, che a questa grana e' mal tarato (un bordo inferiore dritto su
32 px logici e' la norma, non l'indizio di una piattaforma). La scelta e' di non
applicarli e di dichiararlo, non di applicarli male.

## Composizione ({n_rows} righe)

Per sorgente: `{by_source}`
Per split: `{by_split}`

Caption: `dcss-32` usa il nome reale dell'asset dentro il pack; `curated-32` il
manifest curato; `oga-creatures` non ha nomi per-creatura (e' un solo
spritesheet) e usa la categoria dichiarata dalla pagina piu' proporzione e
colori **misurati** sul canvas — mai il nome del file dello spritesheet.
Quanto quelle caption siano davvero distinte e' misurato, non promesso:
`{caption_distinctness}` (caption totali / distinte, per sorgente costruita
qui). Su `oga-creatures` la ripetizione residua e' il limite di cio' che si
puo' contare senza un nome: due creature diverse ma compatte e con gli stessi
due colori dominanti ricevono la stessa frase.

## Cosa contiene

```text
kaggle/
├── run.py                                     # entrypoint del kernel (path aggiornati)
├── train_lora_v0.py                           # trainer (whitelist licenze verificata, vedi sopra)
├── kernel-metadata.json                       # id/slug ancora REPLACE_WITH_KAGGLE_USERNAME
├── README.md                                  # questo file
├── worldsmelt-lora-v1-research-dataset.tar.gz  # dati (SOLO primary-32B) + codice
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

**Niente e' stato lanciato su Kaggle**: questo pacchetto e' pronto e
documentato, non avviato.
"""


LICENSE_WHITELIST_RE = re.compile(r"LICENSE_WHITELIST\s*=\s*\{([^}]*)\}")
# Blocco da sostituire nel trainer: dalla firma di perceptual_distance fino
# alla def successiva (preflight), che resta invariata.
PERCEPTUAL_DISTANCE_RE = re.compile(
    r"^def perceptual_distance\(a, b\) -> float:\n.*?(?=^def preflight\()",
    re.DOTALL | re.MULTILINE)
TRAINER_CONST_RE = {
    "DEDUP_MERGE_SCORE": re.compile(r"^DEDUP_MERGE_SCORE\s*=\s*([0-9.]+)", re.MULTILINE),
    "SIGNATURE_PX": re.compile(r"^SIGNATURE_PX\s*=\s*([0-9]+)", re.MULTILINE),
}


def patch_perceptual_distance(trainer_path: Path) -> str:
    """Sostituisce perceptual_distance() nella SOLA copia adattata del trainer
    (dst/train_lora_v0.py), mai in dataset/lora-v0/kaggle/train_lora_v0.py:
    quel pacchetto e' consegnato e il suo dataset ha davvero il fondo
    trasparente, quindi li' la versione originale e' corretta. Stesso
    meccanismo gia' usato per la whitelist licenze.

    Il testo iniettato NON e' una seconda stesura: e' il sorgente delle
    funzioni di questo file (perceptual_block_source), cosi' le due copie non
    possono divergere."""
    text = trainer_path.read_text(encoding="utf-8")

    # Prima di sostituire: le costanti su cui il cancello di split del miner si
    # e' basato devono essere davvero quelle del trainer, altrimenti il
    # pacchetto passerebbe il cancello locale e fallirebbe quello vero.
    for name, expected in (("DEDUP_MERGE_SCORE", SPLIT_PERCEPTUAL_MAX),
                           ("SIGNATURE_PX", SPLIT_PERCEPTUAL_SIG_PX)):
        m = TRAINER_CONST_RE[name].search(text)
        if not m or float(m.group(1)) != float(expected):
            raise SystemExit(
                f"{name} del trainer ({m.group(1) if m else 'assente'}) non coincide col valore "
                f"usato dal cancello di split di questo script ({expected}): allinearli prima di "
                f"consegnare un pacchetto, non dopo.")

    block = perceptual_block_source()
    new_text, n = PERCEPTUAL_DISTANCE_RE.subn(lambda _m: block + "\n\n", text)
    if n != 1:
        raise SystemExit(
            f"perceptual_distance() non trovata (o trovata {n} volte) in {trainer_path}: "
            "il trainer e' cambiato, la correzione va rifatta a mano invece di essere saltata.")
    trainer_path.write_text(new_text, encoding="utf-8")
    return ("CORRETTA: la versione v0 considera 'acceso' ogni pixel con alpha > 16, vero per il "
            "dataset v0 (fondo trasparente) ma NON per questo bucket, che compone su un fondo "
            "piatto opaco (SD1.5 non ha canale alpha): l'unione delle aree di soggetto diventava "
            "l'intero canvas (misurato: 4096/4096 px) e il preflight riportava 6324 fughe di "
            "split percettive tutte false. La copia di QUESTO pacchetto misura il fondo (modale "
            "dell'anello di bordo) e ricade sul test alpha quando il fondo e' davvero "
            "trasparente. Il pacchetto dataset/lora-v0/kaggle/ NON e' stato toccato: li' la "
            "versione originale e' corretta.")


def run_preflight_in_process(dst: Path, primary_dir: Path, bucket_rows: list) -> tuple[bool, str]:
    """Esegue il preflight VERO del trainer appena scritto sul contenuto esatto
    che finisce nel tar (stesso layout: <root>/images/<id>.png + ledger.jsonl),
    e ne riporta l'esito testuale. Serve a togliere dal README ogni frase sul
    preflight che non sia stata misurata: la versione precedente dichiarava
    "NON fallisce" mentre falliva con 6324 problemi."""
    import contextlib
    import importlib.util
    import io
    import tempfile

    spec = importlib.util.spec_from_file_location(
        "ws_trainer_preflight", dst / "train_lora_v0.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    captured = io.StringIO()
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "images").symlink_to(primary_dir)
        ledger_tmp = root / "ledger.jsonl"
        with ledger_tmp.open("w", encoding="utf-8") as f:
            for r in bucket_rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        try:
            with contextlib.redirect_stdout(captured):
                module.preflight(root, ledger_tmp, FINAL_PX)
        except SystemExit as e:
            return False, str(e)
        except Exception as e:  # noqa: BLE001
            return False, f"il preflight e' esploso invece di dare un verdetto: {e!r}"
    # Le parole sono quelle del trainer, non un riassunto di questo script.
    return True, captured.getvalue().strip()


def parse_license_whitelist(trainer_text: str) -> set:
    """Legge la whitelist DAL TESTO del trainer appena scritto (non un valore
    duplicato qui a mano, che driftrebbe silenziosamente se train_lora_v0.py
    cambia): usata solo per bucket B, vedi build_kaggle_package()."""
    m = LICENSE_WHITELIST_RE.search(trainer_text)
    if not m:
        raise RuntimeError("LICENSE_WHITELIST non trovata nel trainer copiato")
    return set(re.findall(r'"([^"]+)"', m.group(1)))


def build_kaggle_package(out_dir: Path, bucket_name: str = PRIMARY_BUCKET):
    src_kaggle = REPO_ROOT / "dataset" / "lora-v0" / "kaggle"
    is_bucket_b = bucket_name == BUCKET_B
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

    # 'primary_dir'/'bucket_rows' sotto: nome storico del bucket confezionato
    # (primary-128 di default, primary-32B quando is_bucket_b) — generalizzato
    # per parametro, NON per una seconda funzione: la logica di adattamento
    # (adapt_text, tar, kernel-metadata) e' identica per costruzione, cambia
    # solo QUALE bucket entra nel tar.
    primary_dir = out_dir / bucket_name
    if not primary_dir.is_dir() or not any(primary_dir.glob("*.png")):
        cmd_hint = "mine-b" if is_bucket_b else "mine"
        log(f"AVVISO: {primary_dir} vuota o assente — esegui '{cmd_hint}' prima di 'kaggle'")
        return

    ledger_path = out_dir / "ledger.jsonl"
    ledger_all = [json.loads(l) for l in ledger_path.read_text(encoding="utf-8").splitlines() if l.strip()]
    bucket_rows = [r for r in ledger_all if r["bucket"] == bucket_name]

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

    # -- whitelist licenze: SOLO per bucket B (mandato punto 3, "adegua SOLO la
    # whitelist alle licenze REALI del bucket B, documentando"). Per primary-128
    # NON si tocca niente: il fallimento del preflight su quel pacchetto e' il
    # cancello VOLUTO (vedi KAGGLE_README), non un difetto da correggere qui. --
    license_whitelist_note = None
    perceptual_note = None
    if is_bucket_b:
        trainer_path = dst / "train_lora_v0.py"
        perceptual_note = patch_perceptual_distance(trainer_path)
        log(f"[kaggle] perceptual_distance (bucket B): {perceptual_note}")
        current_whitelist = parse_license_whitelist(trainer_path.read_text(encoding="utf-8"))
        real_licenses = sorted({r["license_id"] for r in bucket_rows})
        missing_from_whitelist = sorted(set(real_licenses) - current_whitelist)
        if missing_from_whitelist:
            # Verificato, non assunto (vedi commento sopra estimate_grain() per
            # lo stesso principio altrove nel file): il mandato suggeriva "cc0/
            # cc-by", ma solo le licenze DAVVERO presenti entrano nella
            # whitelist — mai un allargamento piu' ampio di cio' che si e'
            # misurato. Tocca SOLO la copia adattata (dst/train_lora_v0.py),
            # mai dataset/lora-v0/kaggle/train_lora_v0.py.
            trainer_text = trainer_path.read_text(encoding="utf-8")
            new_set = current_whitelist | set(missing_from_whitelist)
            new_literal = "{" + ", ".join(f'"{s}"' for s in sorted(new_set)) + "}"
            comment = (f"  # ampliata per il bucket B (mandato 07/08): licenze REALI verificate "
                      f"sulle {len(bucket_rows)} righe di {bucket_name} = {real_licenses} — "
                      f"aggiunte {missing_from_whitelist}, nessun'altra")
            trainer_text = LICENSE_WHITELIST_RE.sub(
                f"LICENSE_WHITELIST = {new_literal}{comment}", trainer_text, count=1)
            trainer_path.write_text(trainer_text, encoding="utf-8")
            license_whitelist_note = (
                f"AMPLIATA: le licenze reali del bucket B ({real_licenses}) includevano "
                f"{missing_from_whitelist}, fuori dalla whitelist di produzione "
                f"({sorted(current_whitelist)}) — aggiunte SOLO quelle, verificate riga per "
                f"riga sul ledger, non un allargamento a 'cc0/cc-by' generico come suggerito "
                f"a mandato.")
        else:
            license_whitelist_note = (
                f"NESSUNA MODIFICA: le licenze reali del bucket B, verificate su tutte le "
                f"{len(bucket_rows)} righe di {bucket_name}, sono {real_licenses} — gia' "
                f"TUTTE dentro la whitelist di produzione esistente "
                f"({sorted(current_whitelist)}). Il mandato suggeriva di allargarla ad "
                f"accettare anche 'cc-by': verificato che non serve, nessuna riga del bucket "
                f"B dichiara una licenza cc-by (solo cc0/cc0-1.0) — allargarla comunque "
                f"sarebbe stato un'whitelist piu' larga di cio' che il bucket usa davvero.")
        log(f"[kaggle] whitelist licenze (bucket B): {license_whitelist_note}")

    nsfw = {}
    report_path = out_dir / "mining_report.json"
    if report_path.is_file():
        nsfw = json.loads(report_path.read_text(encoding="utf-8")).get("nsfw_review", {})

    if is_bucket_b:
        # Bucket B non ha ne' il cancello licenze (whitelist gia' verificata
        # sopra) ne' quello NSFW (nessuna delle tre sorgenti e' Limbicnation,
        # l'unica con la keyword/skin-fraction — un'ASSENZA dichiarata, non un
        # buco silenzioso). research_only resta true per lineage/scope
        # dell'intero modulo (docstring in cima al file: questa e' comunque
        # una prova di confronto, non il dataset commerciale del gioco), MA
        # con un motivo diverso da primary-128: qui non c'e' nessuna licenza
        # fuori whitelist a bloccare il preflight.
        policy_text = (src_kaggle / "run_policy.yaml").read_text(encoding="utf-8")
        policy_text += (
            "\n# Campo aggiunto per il bucket B (mandato 07/08, opzione 'grana 32 pura'):\n"
            "# a differenza di primary-128, QUI il preflight di train_lora_v0.py NON fallisce\n"
            "# per la whitelist licenze (verificato: dcss-32/oga-creatures/curated-32 sono\n"
            "# tutte cc0/cc0-1.0, gia' dentro la whitelist di produzione — vedi kaggle/README.md,\n"
            "# sezione 'Licenze verificate'). research_only resta true per lo scope dichiarato\n"
            "# dell'intero modulo lora-v1-research (prova di confronto fra bucket, non il\n"
            "# dataset commerciale del gioco), non per una licenza dubbia di QUESTO bucket.\n"
            "research_only: true\n"
            "\n# Nessun cancello NSFW per questo bucket: nessuna delle tre sorgenti "
            "(dcss-32/oga-creatures/curated-32) e' Limbicnation, l'unica su cui gira "
            "l'euristica su parole chiave/frazione di incarnato — assenza verificata, non "
            "un'omissione.\n"
            "owner_nsfw_decision: not_applicable\n"
        )
    else:
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
    if is_bucket_b:
        eval_v0["purpose"] = (
            "Suite di valutazione congelata (STESSI 20 prompt/2 seed di dataset/lora-v0/kaggle/, "
            "riusati apposta per restare comparabili) per i due run rank-16 di questo pacchetto "
            "(dataset/lora-v1-research/kaggle/), dataset BUCKET B (primary-32B: dcss-32 + "
            "oga-creatures + curated-32, grana 32 pura, licenze CC0/CC0-1.0 verificate). Non "
            "lanciata da questo pacchetto: l'esecuzione la fa l'orchestratore col proprietario "
            "(vedi kaggle/README.md).")
    else:
        eval_v0["purpose"] = (
            "Suite di valutazione congelata (STESSI 20 prompt/2 seed di dataset/lora-v0/kaggle/, "
            "riusati apposta per confrontare le due prove) per i due run rank-16 di questo pacchetto "
            "RESEARCH-ONLY (dataset/lora-v1-research/kaggle/), dataset primary-128 (Limbicnation, "
            "provenienza dubbia autorizzata solo per questa prova). Non lanciata da questo pacchetto: "
            "l'esecuzione la fa l'orchestratore col proprietario, se e quando decide di superare il "
            "cancello del preflight (vedi kaggle/README.md).")
    if "notes" in eval_v0 and "character_category_gap" in eval_v0["notes"]:
        if is_bucket_b:
            eval_v0["notes"]["character_category_gap"] = (
                "Nota v0 non riportata qui invariata: la composizione per categoria del bucket B "
                "e' DIVERSA da lora-v0 (dcss-32/oga-creatures/curated-32: enemy/item/prop/creature, "
                "non personaggi) — vedi dataset/lora-v1-research/mining_report.json (chiave "
                "'bucket_b') per i conteggi reali invece di assumerli.")
        else:
            eval_v0["notes"]["character_category_gap"] = (
                "Nota v0 non riportata qui invariata: la composizione per categoria di "
                "primary-128 e' DIVERSA da lora-v0 (fonte Limbicnation, non i pack CC0 di "
                "oggetti/nemici) — vedi dataset/lora-v1-research/mining_report.json per i "
                "conteggi reali invece di assumerli.")
    (dst / "eval-prompts-lora-v1-research.json").write_text(
        json.dumps(eval_v0, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    kernel_meta = {
        "id": "REPLACE_WITH_KAGGLE_USERNAME/worldsmelt-lora-v1-research",
        "title": ("Worldsmelt LoRA v1-research - rank16 (DreamShaper8 + DreamShaper PixelArt) "
                  + ("[bucket B: primary-32B]" if is_bucket_b else "[RESEARCH-ONLY]")),
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

    # -- tar del pacchetto: SOLO il bucket confezionato (primary-128 o
    # primary-32B a seconda di bucket_name) — nome file/percorsi INVARIATI
    # (run.py/configs li cercano per nome fisso, "config invariati" a mandato). --
    tar_path = dst / "worldsmelt-lora-v1-research-dataset.tar.gz"
    with tarfile.open(tar_path, "w:gz") as tf:
        for png in sorted(primary_dir.glob("*.png")):
            tf.add(png, arcname=f"images/{png.name}")
        for txt in sorted(primary_dir.glob("*.txt")):
            tf.add(txt, arcname=f"images/{txt.name}")
        ledger_tmp = dst / "_bucket_ledger.jsonl"
        with ledger_tmp.open("w", encoding="utf-8") as f:
            for r in bucket_rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        tf.add(ledger_tmp, arcname="ledger.jsonl")
        ledger_tmp.unlink()
        tf.add(dst / "eval-prompts-lora-v1-research.json", arcname="eval-prompts-lora-v1-research.json")
        if not is_bucket_b:
            review_path = out_dir / "nsfw-review.jsonl"
            if review_path.is_file():
                # Viaggia DENTRO il pacchetto: la lista di review non serve a niente
                # se resta nel repo mentre i dati sono su Kaggle. Solo primary-128:
                # il bucket B non ha righe NSFW (nessuna sorgente e' Limbicnation),
                # includere il file globale confonderebbe id di un altro bucket.
                tf.add(review_path, arcname="nsfw-review.jsonl")
        tf.add(dst / "train_lora_v0.py", arcname="train_lora_v0.py")
        tf.add(dst / "requirements-ml.txt", arcname="requirements-ml.txt")
        tf.add(dst / "run_policy.yaml", arcname="run_policy.yaml")
        for cfg in sorted((dst / "configs").glob("*.yaml")):
            tf.add(cfg, arcname=f"configs/{cfg.name}")

    tar_sha = sha256_of_file(tar_path)
    if is_bucket_b:
        by_source = Counter(r["source"] for r in bucket_rows)
        by_split = Counter(r["split"] for r in bucket_rows)
        report_path = out_dir / "mining_report.json"
        report_b = {}
        if report_path.is_file():
            report_b = json.loads(report_path.read_text(encoding="utf-8")).get("bucket_b", {})
        gate = report_b.get("single_subject_claim", {}).get("scene_gates_if_applied", {})
        moves = report_b.get("split_perceptual_gate", {}).get("moves", [])

        log("[kaggle] eseguo il preflight vero sul pacchetto (decine di secondi)...")
        t_pf = time.time()
        ok, verdict = run_preflight_in_process(dst, primary_dir, bucket_rows)
        log(f"[kaggle] preflight: {'OK' if ok else 'FALLITO'} ({time.time() - t_pf:.0f}s)")
        if ok:
            verdict_md = f"**Il preflight passa.** Esito testuale del trainer:\n\n```text\n{verdict}\n```"
        else:
            verdict_md = ("**Il preflight FALLISCE su questo pacchetto.** Non lanciarlo: "
                          "spenderebbe credito GPU per fermarsi subito. Esito testuale del "
                          f"trainer:\n\n```text\n{verdict[:1500]}\n```")
        readme = KAGGLE_README_BUCKET_B.format(
            today=datetime.date.today().isoformat(),
            tar_sha=tar_sha,
            n_rows=len(bucket_rows),
            by_source=dict(by_source),
            by_split=dict(by_split),
            license_whitelist_note=license_whitelist_note,
            perceptual_note=perceptual_note,
            preflight_verdict=verdict_md,
            split_gate_note=(
                f"gruppi di soggetto spostati da val a train prima di confezionare, perche' "
                f"quasi-duplicati percettivi di righe di training: {len(moves)} — "
                + "; ".join(f"{m['moved_to_train']} ~ {m['near_duplicate_of']} "
                            f"(differiscono per il {m['distance'] * 100:.1f}% dei pixel)"
                            for m in moves)
                if moves else
                "nessuno spostamento necessario: lo split per nome di soggetto era gia' pulito "
                "anche alla misura percettiva"),
            scene_gate_fraction=f"il {gate.get('fraction', 0) * 100:.1f}%",
            scene_gate_would_reject=gate.get("would_reject", "n/d"),
            scene_gate_breakdown=gate.get("by_first_failing_gate", {}),
            caption_distinctness=report_b.get("caption_distinctness", {}),
        )
    else:
        readme = KAGGLE_README.format(
            today=datetime.date.today().isoformat(),
            sha_note="stesso schema del pacchetto lora-v0",
            tar_sha=tar_sha,
            nsfw_rejected=nsfw.get("skin_rejected", "n/d"),
            nsfw_watch=nsfw.get("kept_above_watch_threshold", "n/d"),
        )
    (dst / "README.md").write_text(readme, encoding="utf-8")
    log(f"pacchetto Kaggle -> {dst} ({len(bucket_rows)} righe {bucket_name}, tar sha256={tar_sha[:12]}...)")


if __name__ == "__main__":
    sys.exit(main())
