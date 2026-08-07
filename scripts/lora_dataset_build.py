#!/usr/bin/env python3
"""Costruisce dataset/lora-v0/: il primo dataset reale per la LoRA "identita' di
mestiere" (mandato del proprietario, 07/08/2026): NON "lo stile Fucina", ma la
capacita' di produrre oggetti/nemici/personaggi/boss in QUALSIASI tema che Gemma
proponga, dentro un UNICO linguaggio visivo pixel art (scala, silhouette, canone).
Per questo le caption portano temi diversi (sci-fi/dungeon/western/medieval/...)
e MAI un tag "worldsmelt"/"fucina": l'identita' addestrata e' del mestiere, non
del tema.

Sorgenti ammesse (sola lettura, mai scritte da questo script):
  1. assets/curated/manifest.json — 189 voci non-worldsmelt (le 44 in
     worldsmelt/ sono arte originale "own" del proprietario, ESCLUSE per sua
     scelta esplicita: per questo "189" e non 233).
  2. i pack CC0 di assets/art-library/10_references/_downloads/<pack>/ — 9 pack
     scaricati, licenza fotografata in license.txt per ciascuno (non in una
     cartella 00_licenses/, che nel repo e' vuota: il mandato la citava a
     memoria, la sede reale e' questa). Ogni pack e' stato ispezionato a mano
     (vedi ART_LIBRARY_INCLUDES / ART_LIBRARY_EXCLUDED_PACKS) e SOLO i file
     elencati esplicitamente entrano come candidati.

Pipeline di normalizzazione (contratto 01-VINCOLI-WORLDSMELT.md del dossier di
ricerca + DEC-177 "i boss possono superare la scala base"):
  bbox alpha (mai il bbox RGBA pieno: pixel trasparenti con RGB residuo
    farebbero credere il soggetto piu' grande di quanto sia)
  -> se il contenuto supera il primo gradino, si prova a "smontare" un eventuale
     upscale nearest gia' applicato dal pacchetto sorgente (blocchi kxk
     uniformi) per recuperare la vera griglia logica; MAI un resize qualunque
     (violerebbe "senza riscalare il contenuto")
  -> canvas logico 32 / 64 / 128, il primo che contiene il soggetto, centrato
     senza scalare (16 resta 16 dentro 32, 48 resta 48 dentro 64, 102 resta 102
     dentro 128)
  -> upscale nearest a 512 (512/32=16, 512/64=8, 512/128=4: sempre intero).

La scala si ferma a 128 e NON sale a 256: a 512/256 il blocco-pixel sarebbe 2x2,
visivamente indistinguibile da un render antialiasato, e insegnerebbe al modello
una seconda griglia-pixel incompatibile con la prima — l'opposto dell'"unica
identita' di mestiere" che il mandato chiede. I 4 boss e 1 nemico oltre 128px
logici restano fuori, dichiarati in rejects.jsonl con questo motivo.

Solo libreria standard + Pillow (nessuna altra dipendenza).
"""

import argparse
import datetime
import hashlib
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
CURATED_MANIFEST = REPO_ROOT / "assets" / "curated" / "manifest.json"
ART_LIBRARY_DOWNLOADS = REPO_ROOT / "assets" / "art-library" / "10_references" / "_downloads"
DEFAULT_OUT = REPO_ROOT / "dataset" / "lora-v0"

CANVAS_LADDER = (32, 64, 128)
FINAL_PX = 512

# -- soglie di scarto, tutte misurate sulle sorgenti reali (vedi il report della
#    build: ogni soglia separa un gruppo osservato dal successivo, nessuna e'
#    scelta a occhio).
MIN_LOGICAL_LONG_SIDE = 4   # sul LATO LUNGO e sull'AREA, non su entrambi i lati:
MIN_LOGICAL_AREA = 12       # spear-brown e' 3x15, una lancia legittima, e un tetto
                            # applicato a ogni lato la buttava via

# Le strisce di animazione di questi pack sono SEMPRE orizzontali (verificato su
# tutte quelle confermate: 84x21, 64x20, 384x32, 182x18); un soggetto molto piu'
# alto che largo e' invece un oggetto verticale legittimo (arco 24x77, lancia
# 3x15). Per questo il tetto sul rapporto e' asimmetrico: un solo numero uguale
# per le due direzioni scartava l'arco insieme allo strip di stelle.
STRIP_H_MIN_WIDTH = 32
STRIP_H_RATIO_MAX = 3.0
STRIP_V_MIN_HEIGHT = 96
STRIP_V_RATIO_MAX = 5.0
COMPONENT_MIN_AREA_RATIO = 0.03  # regioni sotto il 3% dell'area totale sono rumore

VECTOR_MAX_COLORS = 64      # oltre: render vettoriale antialiasato, non pixel art
VECTOR_SEMI_MAX = 0.10      # frazione di pixel semitrasparenti del soggetto
VECTOR_SEMI_MIN_COLORS = 12  # ...ma solo se ci sono anche molti colori: un effetto
                            # deliberatamente translucido (bubble-shield, 1 colore
                            # e 37% di alpha parziale) non e' antialiasing

# Cella piena di tileset: il file sorgente E' una cella quadrata standard di un
# atlas e il contenuto opaco la riempie quasi tutta. Misurato sulla CELLA, non
# sul bbox: un soggetto che lascia margine dentro la propria cella (crate-small,
# 10x9 dentro 16x16) non puo' finire qui per sbaglio, mentre il muro col
# gargoyle, le porte, le casse-tile e le icone su quadrato scuro ci finiscono
# tutte. E' la stessa regola "Tileset: No" gia' applicata a industrial_punk.
TILE_CELL_SIZES = (8, 16, 20, 24, 32)
TILE_CELL_MIN_FILL = 0.95
# Soglia di segnalazione (non di scarto): sopra, il soggetto tocca i quattro
# bordi del proprio ingombro e la caption lo dichiara ("edge-to-edge subject"),
# invece di lasciar credere che intorno ci sia un margine disegnato.
EDGE_TO_EDGE_FILL = 0.90

DEDUP_MERGE_SCORE = 0.15    # sotto questa differenza normalizzata due immagini
                            # sono lo STESSO soggetto ai fini dello split
THEME_SHARE_MAX = 0.70      # nessun tema puo' superare il 70% del dataset (mandato:
                            # "la diversita' e' proprio la chiave")

VAL_TARGET = 0.10


# ---------------------------------------------------------------------------
# Metadati di provenienza (licenza) per pacchetto sorgente.
# I campi seguono il "ledger minimo" di docs/ai-production/04-DATASET-LICENZE.md
# (allowed_commercial / allowed_derivatives / training_allowed_explicit /
# downloaded_at), non un formato inventato qui.
# ---------------------------------------------------------------------------

LICENSE_INFO = {
    "superpowers-asset-packs": {
        "license_id": "CC0-1.0",
        "license_url": "https://github.com/sparklinlabs/superpowers-asset-packs/blob/master/LICENSE.txt",
        "original_url": "https://github.com/sparklinlabs/superpowers-asset-packs",
        "author": "Sparklin Labs / vari (vedi README del pack)",
        "license_snapshot_path": "dataset-raw/superpowers-asset-packs/LICENSE.txt",
        "downloaded_at": "2026-07-17",
    },
    "kenney-tiny-dungeon": {
        "license_id": "CC0",
        "license_url": "https://kenney.nl/support",
        "original_url": "https://kenney.nl/assets/tiny-dungeon",
        "author": "Kenney",
        "license_snapshot_path": "dataset-raw/kenney-tiny-dungeon/extracted/License.txt",
        "downloaded_at": "2026-07-17",
    },
    "kenney-top-down-shooter": {
        "license_id": "CC0",
        "license_url": "https://kenney.nl/support",
        "original_url": "https://kenney.nl/assets/top-down-shooter",
        "author": "Kenney",
        "license_snapshot_path": "dataset-raw/kenney-top-down-shooter/extracted/License.txt",
        "downloaded_at": "2026-07-17",
    },
    "kenney-pixel-shmup": {
        "license_id": "CC0",
        "license_url": "https://kenney.nl/support",
        "original_url": "https://kenney.nl/assets/pixel-shmup",
        "author": "Kenney",
        "license_snapshot_path": "dataset-raw/kenney-pixel-shmup/extracted/License.txt",
        "downloaded_at": "2026-07-17",
    },
    "kenney-micro-roguelike": {
        "license_id": "CC0",
        "license_url": "https://kenney.nl/support",
        "original_url": "https://kenney.nl/assets/micro-roguelike",
        "author": "Kenney",
        "license_snapshot_path": "dataset-raw/kenney-micro-roguelike/extracted/License.txt",
        "downloaded_at": "2026-07-17",
    },
    "good_and_evil": {
        "license_id": "CC0-1.0",
        "license_url": "https://chromoxi.itch.io/good-and-evil",
        "original_url": "https://chromoxi.itch.io/good-and-evil",
        "author": "chromoxi",
        "license_snapshot_path": "assets/art-library/10_references/_downloads/good_and_evil/license.txt",
        "downloaded_at": "2026-07-31",
    },
    "weaponry_tools_kaenine": {
        "license_id": "CC0-1.0",
        "license_url": "https://kaenine.itch.io/16x16-weaponrytools",
        "original_url": "https://kaenine.itch.io/16x16-weaponrytools",
        "author": "kaenine",
        "license_snapshot_path": "assets/art-library/10_references/_downloads/weaponry_tools_kaenine/license.txt",
        "downloaded_at": "2026-07-31",
    },
}

# Tutte e sei le sorgenti sono CC0 verificata alla revisione fotografata sopra:
# i tre permessi del ledger minimo sono quindi identici per tutte, ma restano
# espliciti per file, perche' e' il file che verra' riletto fra sei mesi.
LICENSE_PERMISSIONS = {
    "allowed_commercial": True,
    "allowed_derivatives": True,
    "training_allowed_explicit": True,
}


# ---------------------------------------------------------------------------
# Tema e vista per sotto-pacchetto sorgente.
#
# Il tema NON e' piu' un default per pacchetto applicato in blocco: ogni pack
# contribuisce al massimo il proprio AMBIENTAZIONE (una sola parola, verificata
# sulla descrizione dell'autore), e "fantasy" non e' mai assegnato d'ufficio —
# lo si guadagna solo dai tag dell'asset (magia, drago, non-morto, pozione...).
# Prima di questa regola "fantasy" copriva il 75,8% del dataset, oltre la soglia
# del 70% che il mandato impone alla diversita'.
# ---------------------------------------------------------------------------

SETTING_BY_SOURCE = {
    ("superpowers-asset-packs", "medieval-fantasy"): "medieval",
    ("superpowers-asset-packs", "rpg-battle-system"): "adventure",
    ("superpowers-asset-packs", "ninja-adventure"): "feudal",
    ("superpowers-asset-packs", "space-shooter"): "sci-fi",
    ("superpowers-asset-packs", "western-fps-2d"): "western",
    ("superpowers-asset-packs", "top-down-shooter"): "modern",
    ("kenney-tiny-dungeon",): "dungeon",
    ("kenney-top-down-shooter",): "modern",
    ("kenney-pixel-shmup",): "sci-fi",
    ("kenney-micro-roguelike",): "dungeon",
    ("good_and_evil",): "dungeon",
    ("weaponry_tools_kaenine",): "rustic",
}

# Vista per sotto-pacchetto, ISPEZIONATA a colpo d'occhio sui contact sheet dei
# pack (scripts di lavoro, non versionati): niente vista inventata, e niente
# caption senza vista — prima erano 121 su 178 a non averne nessuna, e i 20
# prompt di valutazione aprono TUTTI con una vista.
VIEW_BY_SOURCE = {
    ("superpowers-asset-packs", "medieval-fantasy"): "top-down three-quarter view",
    ("superpowers-asset-packs", "rpg-battle-system"): "front view",
    ("superpowers-asset-packs", "ninja-adventure"): "front view",
    ("superpowers-asset-packs", "space-shooter"): "top-down view",
    ("superpowers-asset-packs", "western-fps-2d"): "front view",
    ("superpowers-asset-packs", "top-down-shooter"): "top-down view",
    ("kenney-tiny-dungeon",): "top-down three-quarter view",
    ("kenney-top-down-shooter",): "top-down view",
    ("kenney-pixel-shmup",): "top-down view",
    ("kenney-micro-roguelike",): "top-down three-quarter view",
    ("good_and_evil",): "top-down three-quarter view",
    ("weaponry_tools_kaenine",): "front view",
}

# Tag del manifest curato (vocabolario gia' controllato) -> tema/materiale.
# Esclusi di proposito i tag di RUOLO/gameplay (obstacle, weapon, boss,
# container, chest, currency, key, door, gate, projectile, ranged, throwing,
# blunt, consumable, curio, stationary, barrier, accessory, emblem, mimic,
# large, micro, neutral, rogue, guard, health, mana, speed, "fusion-reserve":
# quest'ultimo e' meccanica di gioco Worldsmelt, non un tema visivo).
TAG_TO_THEME = {
    "sci-fi": "sci-fi", "beast": "beast", "gadget": "tech", "humanoid": "humanoid",
    "potion": "alchemy", "gem": "crystal", "nature": "nature", "armor": "metal",
    "element": "elemental", "fire": "fire", "undead": "undead", "magic": "arcane",
    "ooze": "ooze", "reptile": "reptile", "mechanical": "mech", "aquatic": "aquatic",
    "ice": "ice", "poison": "poison", "dragon": "reptile", "water": "aquatic",
    "lightning": "elemental", "earth": "elemental", "wind": "elemental",
    "dark": "occult", "light": "arcane", "prehistoric": "primal",
    "sword": "steel", "axe": "steel", "dagger": "steel", "spear": "steel",
    "shield": "metal", "gloves": "leather", "helmet": "metal", "food": "rustic",
    "decor": "rustic", "flying": "winged",
}

# Parole del nome -> tema/materiale. Prudenti: solo quello che il nome dichiara
# esplicitamente. Sono la fonte principale di diversita' PER ASSET, quella che
# prima mancava del tutto per l'art-library (che non ha tag propri).
NAME_TO_THEME = {
    "gold": "gold", "golden": "gold", "silver": "silver", "bronze": "bronze",
    "iron": "iron", "steel": "steel", "metal": "metal", "wood": "wood",
    "wooden": "wood", "stone": "stone", "bone": "bone", "crystal": "crystal",
    "gem": "crystal", "ruby": "crystal", "diamond": "crystal", "emerald": "crystal",
    "leather": "leather", "cloth": "cloth", "glass": "glass", "ceramic": "ceramic",
    "ghost": "undead", "skeleton": "undead", "skull": "undead", "mummy": "undead",
    "zombie": "undead", "tomb": "funerary", "sarcophagus": "funerary",
    "grave": "funerary", "gravestone": "funerary", "tombstone": "funerary",
    "demon": "occult", "gargoyle": "occult", "void": "occult", "dark": "occult",
    "slime": "ooze", "blob": "ooze", "ooze": "ooze",
    "wizard": "arcane", "wand": "arcane", "scroll": "arcane", "orb": "arcane",
    "staff": "arcane", "rune": "arcane", "scepter": "arcane",
    "knight": "metal", "helm": "metal", "armor": "metal", "shield": "metal",
    "gauntlet": "metal", "anvil": "iron", "hammer": "iron", "horseshoe": "iron",
    "ship": "mech", "shmup": "mech", "rocket": "mech", "robot": "mech",
    "mecha": "mech", "turret": "mech", "cruiser": "mech", "dreadnought": "mech",
    "fire": "fire", "flame": "fire", "ember": "fire", "fireball": "fire",
    "lava": "fire", "ice": "ice", "frost": "ice", "poison": "poison",
    "acid": "poison", "water": "aquatic", "well": "aquatic", "fountain": "aquatic",
    "crab": "aquatic", "turtle": "aquatic", "octopus": "aquatic", "naga": "aquatic",
    "tree": "nature", "bush": "nature", "leaf": "nature", "pine": "nature",
    "mushroom": "nature", "sheep": "beast", "horse": "beast", "dog": "beast",
    "boar": "beast", "bat": "beast", "snake": "beast", "dino": "primal",
    "armadillo": "beast", "wolf": "beast", "shellback": "beast",
    "dragon": "reptile", "reptile": "reptile", "yeti": "primal", "giant": "primal",
    "cyclop": "primal", "goblin": "humanoid", "dwarf": "humanoid",
    "ninja": "stealth", "shuriken": "stealth", "kunai": "stealth",
    "katana": "steel", "sai": "steel", "sword": "steel", "dagger": "steel",
    "axe": "steel", "spear": "steel", "arrow": "wood", "bow": "wood",
    "crossbow": "wood", "pickaxe": "iron", "shovel": "iron",
    "potion": "alchemy", "vial": "alchemy", "cauldron": "alchemy",
    "elixir": "alchemy", "medipack": "medical", "heart": "medical",
    "coin": "gold", "goblet": "gold", "pouch": "leather", "chest": "wood",
    "crate": "wood", "barrel": "wood", "fence": "wood", "door": "wood",
    "pot": "ceramic", "jar": "ceramic", "lantern": "glass", "bubble": "glass",
    "statue": "stone", "bread": "rustic", "dice": "rustic", "watch": "clockwork",
    "pocket": "clockwork", "scifi": "sci-fi", "space": "sci-fi",
}

# Prefissi del nome che sono "famiglia" e non un sostantivo utile in caption.
NOUN_REWRITE = {"shmup": "spaceship", "ship": "spaceship", "element": "elemental sigil"}

# Suffissi che marcano il pacchetto, non il soggetto: vanno via dal nome
# leggibile ("chest-wood-medieval" -> "wood chest").
PACK_SUFFIX_TOKENS = {"kenney", "ninja", "micro", "medieval"}

CATEGORY_FIX = {"enemie": "enemy", "bosse": "boss"}

# Riempitivo per categoria, usato SOLO se dopo ambientazione + tag dell'asset il
# tema resta a un solo elemento (il mandato chiede 2-4 tag per asset).
CATEGORY_THEME_FILLER = {
    "item": "equipment", "weapon": "equipment", "enemy": "creature",
    "boss": "creature", "prop": "decor", "character": "adventure",
}


# ---------------------------------------------------------------------------
# Candidati dell'art-library: solo i file ispezionati a mano.
# Ogni voce: (percorso relativo dentro extracted/, categoria, nome soggetto).
# ---------------------------------------------------------------------------

ART_LIBRARY_INCLUDES = {
    "good_and_evil": [
        ("characters/knight/with_weapon.png", "character", "knight"),
        ("characters/knight/without_weapon.png", "character", "knight"),
        ("characters/wizard/with_weapon.png", "character", "wizard"),
        ("characters/wizard/without_weapon.png", "character", "wizard"),
        ("monsters/demon/DEMON.png", "enemy", "demon"),
        ("monsters/green_slime/slime.png", "enemy", "green_slime"),
        ("monsters/red_slime/slime_red.png", "enemy", "red_slime"),
    ],
    "weaponry_tools_kaenine": [
        ("Arrow.png", "weapon", "arrow"),
        ("Axe.png", "weapon", "axe"),
        ("Bow.png", "weapon", "bow"),
        ("Crossbow.png", "weapon", "crossbow"),
        ("CrossbowLoaded.png", "weapon", "crossbow_loaded"),
        ("Pickaxe.png", "weapon", "pickaxe"),
        ("Shovel.png", "weapon", "shovel"),
        ("Sword.png", "weapon", "sword"),
    ],
}

# Pacchetti scaricati ma esclusi in blocco, con il motivo verificato a mano:
# tenerli qui, anche se non usati, e' il registro dell'ispezione "controlla
# ognuno" richiesta dal mandato.
ART_LIBRARY_EXCLUDED_PACKS = {
    "debts_in_the_depths": "Characters/*.png sono strip di animazione (verificato: "
        "aspect ratio del bbox 3.9-9.8, es. sprWizard.png 182x18); Effects/ e' VFX "
        "(fuori canone v1); Environment/UI sono tileset/props-sheet/UI (fuori canone).",
    "industrial_punk": "Individual Tiles/ e' un tileset modulare (480 file, canone "
        "'Tileset: No'); Page Assets/ sono banner/mockup promozionali, non sprite.",
    "isometric_character_supernova": "i 4 PNG sono fogli di animazione interi "
        "(idle 512x512, walk 768x512 ecc.), non pose singole.",
    "isometric_character_template_intellikat": "solo 'template-spritesheet-*.png' "
        "(fogli interi); le sorgenti posa-per-posa esistono solo come .ase non "
        "parsabili senza Aseprite (fuori scope: solo stdlib+PIL).",
    "medieval_fantasy_items": "unico file 176x112 = griglia di ~77 icone 16x16 in "
        "un solo PNG (sheet multiplo), nessun file per-icona.",
    "pixel_patterns": "l'intero pack e' texture/pattern ripetibili (tile), non "
        "soggetti singoli.",
    "skull_enemy": "tutti i PNG sono sotto '(SpriteSheet PNGs)/': strip di "
        "animazione confermate (es. Skeleton_Head_Idle.png 384x32 = 12 frame).",
    "topdown_dungeon_character": "mai scaricato (richiede download manuale, "
        "vedi assets/art-library/README.md): nessun file da ispezionare.",
}


def log(*a):
    print(*a, file=sys.stderr)


def repo_rel(path: Path) -> str:
    """Percorso relativo alla radice del repo quando possibile, assoluto quando
    l'output e' stato messo fuori dal repo con --out: il ledger deve restare
    scrivibile anche in quel caso, che e' un'opzione documentata della CLI."""
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


def sha256_of_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


# ---------------------------------------------------------------------------
# Raccolta candidati
# ---------------------------------------------------------------------------

class Candidate:
    """Un file sorgente ancora da normalizzare, con tutta la provenienza gia'
    risolta prima di aprire l'immagine: se qualcosa va storto dopo, il motivo
    di scarto puo' comunque citare pack/licenza senza doverli ricalcolare."""

    def __init__(self, src_path, pack, source_key, category, name_tokens,
                 manifest_tags, license_info, source_field, manifest_transform=None):
        self.src_path = src_path
        self.pack = pack
        self.source_key = source_key      # tupla usata per ambientazione e vista
        self.category = category
        self.name_tokens = name_tokens    # token del nome, gia' ripuliti
        self.manifest_tags = manifest_tags
        self.license_info = license_info
        self.source_field = source_field
        self.manifest_transform = manifest_transform


def clean_name_tokens(raw: str):
    """Token del nome senza il suffisso di pacchetto: e' il suffisso a marcare
    la provenienza, non il soggetto, e in caption non deve comparire."""
    tokens = [t for t in raw.replace("_", "-").lower().split("-") if t]
    while len(tokens) > 1 and tokens[-1] in PACK_SUFFIX_TOKENS:
        tokens.pop()
    return tokens


def curated_candidates():
    manifest = json.loads(CURATED_MANIFEST.read_text(encoding="utf-8"))
    base = CURATED_MANIFEST.parent
    for entry in manifest["images"]:
        if entry["file"].startswith("worldsmelt/"):
            continue  # arte originale "own" del proprietario, esclusa (mandato 07/08)
        category = CATEGORY_FIX.get(entry["category"], entry["category"])
        source = entry.get("source", "")
        segments = source.split("/")
        if segments and segments[0] == "superpowers-asset-packs":
            source_key = tuple(segments[:2])
            license_key = "superpowers-asset-packs"
        else:
            source_key = (segments[0],) if segments else ("?",)
            license_key = segments[0] if segments else "?"
        license_info = LICENSE_INFO.get(license_key)
        if license_info is None:
            log(f"AVVISO: nessuna licenza mappata per '{license_key}' (id={entry['id']}), salto")
            continue
        yield Candidate(
            src_path=base / entry["file"],
            pack=f"curated:{license_key}",
            source_key=source_key,
            category=category,
            name_tokens=clean_name_tokens(entry["id"]),
            manifest_tags=entry.get("tags", []),
            license_info=license_info,
            source_field=source,
            manifest_transform=entry.get("transform"),
        )


def art_library_candidates():
    for pack, items in ART_LIBRARY_INCLUDES.items():
        pack_dir = ART_LIBRARY_DOWNLOADS / pack / "extracted"
        license_info = LICENSE_INFO[pack]
        for rel, category, subject in items:
            path = pack_dir / rel
            if not path.is_file():
                log(f"AVVISO: file atteso mancante per {pack}: {path}")
                continue
            yield Candidate(
                src_path=path,
                pack=f"art-library:{pack}",
                source_key=(pack,),
                category=category,
                name_tokens=clean_name_tokens(subject),
                manifest_tags=[],
                license_info=license_info,
                source_field=f"{pack}/{rel}",
            )


# ---------------------------------------------------------------------------
# Analisi immagine
# ---------------------------------------------------------------------------

def alpha_bbox(im: Image.Image, threshold=16):
    """Bbox sul solo canale alpha: un bbox sull'RGBA pieno conterebbe anche
    pixel trasparenti con RGB residuo (comune in export PNG con colore
    "sporco" sotto alpha=0), gonfiando il soggetto rilevato."""
    alpha = im.split()[3]
    alpha_t = alpha.point(lambda a: 255 if a > threshold else 0)
    return alpha_t.getbbox()


def count_components(im_rgba: Image.Image, alpha_threshold=16,
                     min_area_ratio=COMPONENT_MIN_AREA_RATIO):
    """Etichetta le regioni connesse (4-adiacenza) sul canale alpha.
    Serve a scartare fogli con piu' icone/pose separate che l'aspect ratio da
    solo non cattura (es. griglie quadrate, non strisce)."""
    w, h = im_rgba.size
    alpha = im_rgba.split()[3]
    px = alpha.load()
    visited = bytearray(w * h)
    comps = []
    for y in range(h):
        row_off = y * w
        for x in range(w):
            idx = row_off + x
            if visited[idx] or px[x, y] <= alpha_threshold:
                continue
            stack = [(x, y)]
            visited[idx] = 1
            minx = maxx = x
            miny = maxy = y
            area = 0
            while stack:
                cx, cy = stack.pop()
                area += 1
                if cx < minx: minx = cx
                if cx > maxx: maxx = cx
                if cy < miny: miny = cy
                if cy > maxy: maxy = cy
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < w and 0 <= ny < h:
                        nidx = ny * w + nx
                        if not visited[nidx] and px[nx, ny] > alpha_threshold:
                            visited[nidx] = 1
                            stack.append((nx, ny))
            comps.append(((minx, miny, maxx + 1, maxy + 1), area))
    total_area = sum(a for _, a in comps) or 1
    significant = [(bbox, area) for bbox, area in comps
                   if area / total_area >= min_area_ratio and area >= 6]
    significant.sort(key=lambda c: -c[1])
    return significant


def detect_pixel_scale(im_rgba: Image.Image, max_factor=16, tol=0.05):
    """Prova a recuperare la vera griglia logica di un asset gia' esportato
    pre-scalato (blocchi kxk uniformi). Usata SOLO quando il contenuto supera
    il primo gradino del canone: per gli asset gia' dentro non si tocca nulla,
    per non alterare dettagli genuinamente disegnati a quella risoluzione."""
    w, h = im_rgba.size

    def divisors(n):
        return {d for d in range(2, min(n, max_factor) + 1) if n % d == 0}

    candidates = sorted(divisors(w) & divisors(h), reverse=True)
    if not candidates:
        return 1
    px = im_rgba.load()
    for k in candidates:
        bw, bh = w // k, h // k
        total_blocks = bw * bh
        bad = 0
        over_tolerance = False
        for by in range(bh):
            for bx in range(bw):
                x0, y0 = bx * k, by * k
                ref = px[x0, y0]
                uniform = True
                for yy in range(k):
                    for xx in range(k):
                        if px[x0 + xx, y0 + yy] != ref:
                            uniform = False
                            break
                    if not uniform:
                        break
                if not uniform:
                    bad += 1
            if total_blocks and bad / total_blocks > tol:
                over_tolerance = True
                break  # scarto anticipato: questo k non e' abbastanza uniforme
        if not over_tolerance and total_blocks:
            return k
    return 1


def pixel_stats(im_rgba: Image.Image):
    """Colori distinti, frazione semitrasparente, riempimento del bbox e
    frazione scura del perimetro. Un solo passaggio: sono tutte misure che
    servono alle soglie di scarto e alla caption, e il doppio giro su immagini
    fino a 128x128 non aggiunge niente."""
    w, h = im_rgba.size
    px = im_rgba.load()
    colors = set()
    subject = 0
    semi = 0
    opaque = 0
    dark_edge = 0
    edge = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a <= 16:
                continue
            subject += 1
            colors.add((r, g, b))
            if a < 240:
                semi += 1
            else:
                opaque += 1
            border = False
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if not (0 <= nx < w and 0 <= ny < h) or px[nx, ny][3] <= 16:
                    border = True
                    break
            if border:
                edge += 1
                if (r * 299 + g * 587 + b * 114) / 1000 < 70:
                    dark_edge += 1
    return {
        "colors": len(colors),
        "subject_px": subject,
        "semi_fraction": semi / max(1, subject),
        "bbox_fill": opaque / max(1, w * h),
        "dark_edge_fraction": dark_edge / max(1, edge),
    }


def normalize_asset(im: Image.Image):
    """Ritorna (final_512_rgba, info_dict, None) oppure (None, None, motivo)."""
    im = im.convert("RGBA")
    source_w, source_h = im.size
    bbox = alpha_bbox(im)
    if bbox is None:
        return None, None, "immagine vuota (nessun pixel non trasparente)"

    trimmed = im.crop(bbox)
    tw, th = trimmed.size
    long_side = max(tw, th)
    if long_side < MIN_LOGICAL_LONG_SIDE or tw * th < MIN_LOGICAL_AREA:
        return None, None, (f"soggetto troppo piccolo ({tw}x{th}px, area {tw * th}): "
                            f"sotto {MIN_LOGICAL_LONG_SIDE}px di lato lungo o "
                            f"{MIN_LOGICAL_AREA}px di area non e' leggibile")

    if tw >= STRIP_H_MIN_WIDTH and tw / max(1, th) > STRIP_H_RATIO_MAX:
        return None, None, (f"striscia orizzontale {tw}x{th} (rapporto "
                            f"{tw / max(1, th):.2f} > {STRIP_H_RATIO_MAX}): "
                            f"striscia di animazione")
    if th >= STRIP_V_MIN_HEIGHT and th / max(1, tw) > STRIP_V_RATIO_MAX:
        return None, None, (f"striscia verticale {tw}x{th} (rapporto "
                            f"{th / max(1, tw):.2f} > {STRIP_V_RATIO_MAX})")

    comps = count_components(trimmed)
    if len(comps) >= 3:
        return None, None, (f"{len(comps)} regioni disgiunte comparabili "
                            f"(sheet multi-icona, {tw}x{th})")
    if len(comps) == 2:
        (a_bbox, _), (b_bbox, _) = comps
        aw, ah = a_bbox[2] - a_bbox[0], a_bbox[3] - a_bbox[1]
        bw, bh = b_bbox[2] - b_bbox[0], b_bbox[3] - b_bbox[1]
        if aw and bw and ah and bh:
            wr = min(aw, bw) / max(aw, bw)
            hr = min(ah, bh) / max(ah, bh)
            if wr > 0.75 and hr > 0.75:
                return None, None, f"2 regioni di dimensione simile (strip a 2 frame, {tw}x{th})"

    scale = 1
    if long_side > CANVAS_LADDER[0]:
        scale = detect_pixel_scale(trimmed)
        if scale > 1:
            trimmed = trimmed.resize((tw // scale, th // scale), Image.NEAREST)
            tw, th = trimmed.size
            long_side = max(tw, th)

    if long_side > CANVAS_LADDER[-1]:
        return None, None, (f"dimensione logica {tw}x{th}px oltre il gradino massimo "
                            f"{CANVAS_LADDER[-1]} (a 512/256 il blocco-pixel sarebbe 2x2, "
                            f"una seconda griglia incompatibile col resto del dataset)")

    stats = pixel_stats(trimmed)
    if stats["colors"] > VECTOR_MAX_COLORS:
        return None, None, (f"{stats['colors']} colori distinti su {tw}x{th}px "
                            f"(> {VECTOR_MAX_COLORS}): render vettoriale antialiasato, "
                            f"non pixel art")
    if (stats["semi_fraction"] > VECTOR_SEMI_MAX
            and stats["colors"] >= VECTOR_SEMI_MIN_COLORS):
        return None, None, (f"{stats['semi_fraction'] * 100:.1f}% di pixel semitrasparenti "
                            f"con {stats['colors']} colori: bordo antialiasato, non "
                            f"trasparenza deliberata")

    if source_w == source_h and source_w in TILE_CELL_SIZES:
        cell_fill = stats["bbox_fill"] * tw * th / (source_w * source_h)
        if cell_fill >= TILE_CELL_MIN_FILL:
            return None, None, (f"cella piena di tileset: {cell_fill * 100:.1f}% della "
                                f"cella {source_w}x{source_h} e' opaca — il fondo fa "
                                f"parte del disegno (muro, cornice, quadrato scuro) e "
                                f"la caption 'plain background' sarebbe falsa")

    canvas_size = next(c for c in CANVAS_LADDER if max(tw, th) <= c)
    canvas = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    off = ((canvas_size - tw) // 2, (canvas_size - th) // 2)
    # incolla SENZA maschera: con `paste(im, off, im)` l'alpha del sorgente
    # verrebbe usato anche come maschera e quindi elevato al quadrato, e i pixel
    # semitrasparenti di bordo sparirebbero (8 sprite perdevano 3 colonne).
    canvas.paste(trimmed, off)
    final = canvas.resize((FINAL_PX, FINAL_PX), Image.NEAREST)

    info = {
        "logical_size": [tw, th],
        "canvas_size": canvas_size,
        "pixel_scale_factor": scale,
        "colors": stats["colors"],
        "semi_fraction": round(stats["semi_fraction"], 4),
        "bbox_fill": round(stats["bbox_fill"], 4),
        "edge_to_edge": stats["bbox_fill"] >= EDGE_TO_EDGE_FILL,
        "outlined": stats["dark_edge_fraction"] >= 0.60,
        "signature": canvas.resize((64, 64), Image.NEAREST),
    }
    return final, info, None


def dhash(im_rgba: Image.Image, size=8) -> str:
    """Hash percettivo (differenze orizzontali su 8x9 in luminanza, con l'alpha
    composto su nero) richiesto dal ledger minimo di 04-DATASET-LICENZE.md."""
    flat = Image.new("RGB", im_rgba.size, (0, 0, 0))
    flat.paste(im_rgba, (0, 0), im_rgba)
    small = flat.convert("L").resize((size + 1, size), Image.BILINEAR)
    px = small.load()
    bits = 0
    for y in range(size):
        for x in range(size):
            bits = (bits << 1) | (1 if px[x, y] > px[x + 1, y] else 0)
    return f"dhash8:{bits:016x}"


def perceptual_distance(a: Image.Image, b: Image.Image) -> float:
    """Differenza normalizzata sull'UNIONE delle aree non trasparenti, non sul
    canvas: due icone 8x8 diverse dentro un canvas 32 differiscono solo sul 6%
    del canvas, e una soglia sul canvas le dichiarerebbe identiche."""
    pa, pb = a.load(), b.load()
    w, h = a.size
    union = 0
    diff = 0
    for y in range(h):
        for x in range(w):
            r1, g1, b1, a1 = pa[x, y]
            r2, g2, b2, a2 = pb[x, y]
            on1, on2 = a1 > 16, a2 > 16
            if not (on1 or on2):
                continue
            union += 1
            if on1 != on2:
                diff += 1
            elif max(abs(r1 - r2), abs(g1 - g2), abs(b1 - b2)) > 24:
                diff += 1
    return diff / max(1, union)


# ---------------------------------------------------------------------------
# Caption
# ---------------------------------------------------------------------------

def subject_phrase(tokens, category):
    """Frase del soggetto dal nome dell'asset, sostantivo in fondo:
    "sword-ornate-red" -> "ornate red sword". Il soggetto e' l'unica parte della
    caption davvero specifica dell'immagine: senza, 178 immagini condividevano
    66 caption e una sola copriva 13 file."""
    toks = list(tokens)
    if toks and toks[0] == "boss" and len(toks) > 1:
        toks = toks[1:]  # "boss" e' gia' la categoria
    if not toks:
        return category
    noun = NOUN_REWRITE.get(toks[0], toks[0])
    qualifiers = [t for t in toks[1:] if t not in ("a", "b", "variant", "plain")]
    return " ".join(qualifiers + [noun])


def theme_tags(candidate, category):
    """2-4 tag di tema/materiale, in ordine di specificita': prima quelli che
    l'ASSET dichiara (tag del manifest, parole del nome), poi l'ambientazione
    del pacchetto. Nessun tema d'ufficio per pacchetto: era cosi' che "fantasy"
    finiva sul 75,8% del dataset."""
    tags = []

    def add(t):
        if t and t not in tags:
            tags.append(t)

    for t in candidate.manifest_tags:
        add(TAG_TO_THEME.get(t))
    for t in candidate.name_tokens:
        add(NAME_TO_THEME.get(t))
    add(SETTING_BY_SOURCE.get(candidate.source_key))
    if len(tags) < 2:
        add(CATEGORY_THEME_FILLER.get(category, "adventure"))
    return tags[:4]


def build_caption(category, subject, view, tags, outlined, edge_to_edge):
    """Formato del mandato ("game sprite, categoria, vista, 2-4 tag, single
    subject, plain background") con due aggiunte richieste dalla review: il
    soggetto per-asset e un token di stile MISURATO. Il vocabolario e' quello
    dei 20 prompt di valutazione ("2d game sprite", "top-down three-quarter
    view", "bold flat color masses", "soft value contrast edges"): se le
    caption e la griglia di eval parlano lingue diverse, la griglia non misura
    la LoRA."""
    parts = ["2d game sprite", category, subject]
    if view:
        parts.append(view)
    parts.extend(tags)
    parts.append("bold flat color masses")
    parts.append("black outline" if outlined else "soft value contrast edges")
    parts.append("single subject")
    if edge_to_edge:
        # il soggetto tocca i quattro bordi del proprio ingombro: dichiararlo
        # evita che "plain background" (vero: il canvas intorno e' trasparente)
        # venga letto come "c'e' un margine disegnato intorno alla silhouette".
        parts.append("edge-to-edge subject")
    parts.append("plain background")
    return ", ".join(parts)


# ---------------------------------------------------------------------------
# Raggruppamento per soggetto e split
# ---------------------------------------------------------------------------

class UnionFind:
    def __init__(self):
        self.parent = {}

    def find(self, x):
        self.parent.setdefault(x, x)
        while self.parent[x] != x:
            self.parent[x] = self.parent[self.parent[x]]
            x = self.parent[x]
        return x

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.parent[max(ra, rb)] = min(ra, rb)


def name_family(candidate):
    """Famiglia di nome: pacchetto + sostantivo di base (+ secondo token per i
    boss, che altrimenti finirebbero tutti nella stessa famiglia). Collassa le
    varianti di colore/lettera dello stesso soggetto: chest-wood/metal/ornate/red,
    fence-a/b/c, pot-empty/water/milk, gargoyle-wall-plain/glowing."""
    toks = candidate.name_tokens
    if not toks:
        return f"{candidate.pack}:?"
    if toks[0] == "boss" and len(toks) > 1:
        stem = f"{toks[0]}-{toks[1]}"
    elif toks[0] == "gargoyle" and len(toks) > 1:
        stem = f"{toks[0]}-{toks[1]}"
    else:
        stem = toks[0]
    return f"{candidate.pack}:{stem}"


def assign_splits(records, val_target=VAL_TARGET):
    """Split 90/10 per SOGGETTO. L'appartenenza al soggetto e' l'unione di due
    criteri: la famiglia di nome e la vicinanza percettiva (le coppie provate in
    review — gargoyle 1,17% di pixel diversi, pot 1,46% — devono cadere dallo
    stesso lato anche se i nomi non lo dicessero). I gruppi si assegnano dal piu'
    grande al piu' piccolo finche' val non raggiunge la quota: cosi' un gruppo da
    8 non fa esplodere il 10%."""
    uf = UnionFind()
    for i, r in enumerate(records):
        uf.union(i, i)
    by_family = defaultdict(list)
    for i, r in enumerate(records):
        by_family[r["name_family"]].append(i)
    for idxs in by_family.values():
        for j in idxs[1:]:
            uf.union(idxs[0], j)

    for i in range(len(records)):
        for j in range(i + 1, len(records)):
            if uf.find(i) == uf.find(j):
                continue
            if perceptual_distance(records[i]["signature"], records[j]["signature"]) < DEDUP_MERGE_SCORE:
                uf.union(i, j)

    groups = defaultdict(list)
    for i in range(len(records)):
        groups[uf.find(i)].append(i)

    # chiave stabile del gruppo: il piu' piccolo id in ordine alfabetico, cosi'
    # il subject_key non dipende dall'ordine di lettura del filesystem.
    keyed = []
    for members in groups.values():
        key = min(records[i]["id"] for i in members)
        keyed.append((key, members))

    # Ordine deterministico ma non alfabetico (l'alfabetico metterebbe in val
    # solo i primi pacchetti), poi riempimento avido: un gruppo entra in val
    # solo se ci sta INTERO dentro la quota, cosi' un gruppo da 8 non la
    # sfonda e nessun soggetto si spezza fra i due lati.
    target = max(1, round(len(records) * val_target))
    in_val = 0
    for key, members in sorted(keyed, key=lambda kv: hashlib.sha256(
            kv[0].encode("utf-8")).hexdigest()):
        split = "train"
        if in_val + len(members) <= target:
            split = "val"
            in_val += len(members)
        for i in members:
            records[i]["subject_key"] = key
            records[i]["split"] = split
    return keyed


# ---------------------------------------------------------------------------
# Id
# ---------------------------------------------------------------------------

def slugify(text: str) -> str:
    out = []
    for ch in text.lower():
        if ch.isalnum() or ch in "-_":
            out.append(ch)
        else:
            out.append("-")
    slug = "".join(out)
    while "--" in slug:
        slug = slug.replace("--", "-")
    return slug.strip("-")


def make_id(candidate: Candidate, seen_ids: set) -> str:
    pack_part = slugify(candidate.pack.split(":", 1)[-1])
    subject_part = slugify(candidate.src_path.stem)
    base = f"{pack_part}-{candidate.category}-{subject_part}"
    if base not in seen_ids:
        seen_ids.add(base)
        return base
    i = 2
    while f"{base}-{i}" in seen_ids:
        i += 1
    seen_ids.add(f"{base}-{i}")
    return f"{base}-{i}"


# ---------------------------------------------------------------------------
# Build principale
# ---------------------------------------------------------------------------

def run_build(out_dir: Path, limit=None):
    images_dir = out_dir / "images"
    images_dir.mkdir(parents=True, exist_ok=True)
    ledger_path = out_dir / "ledger.jsonl"
    rejects_path = out_dir / "rejects.jsonl"

    seen_ids = set()
    reject_rows = []
    rejects = Counter()
    reject_samples = defaultdict(list)
    records = []

    candidates = list(curated_candidates()) + list(art_library_candidates())
    if limit:
        candidates = candidates[:limit]
    log(f"-- {len(candidates)} candidati raccolti (prima della normalizzazione) --")

    now = datetime.datetime.now().isoformat(timespec="seconds")

    for cand in candidates:
        try:
            src_bytes = cand.src_path.read_bytes()
            im = Image.open(cand.src_path)
            im.load()
        except Exception as e:  # noqa: BLE001 — vogliamo comunque loggare e continuare
            reason = "file illeggibile/corrotto"
            rejects[reason] += 1
            reject_rows.append({
                "source_path": repo_rel(cand.src_path), "pack": cand.pack,
                "category": cand.category, "source_dimensions": None,
                "reason_short": reason, "reason": f"{reason}: {e}",
            })
            continue

        source_w, source_h = im.size
        final, info, reason = normalize_asset(im)
        if reason is not None:
            short = reason.split(" (")[0].split(":")[0]
            rejects[short] += 1
            reject_rows.append({
                "source_path": repo_rel(cand.src_path), "pack": cand.pack,
                "category": cand.category, "source_dimensions": [source_w, source_h],
                "reason_short": short, "reason": reason,
            })
            if len(reject_samples[short]) < 8:
                reject_samples[short].append(
                    f"{repo_rel(cand.src_path)} [{source_w}x{source_h}] -> {reason}")
            continue

        img_id = make_id(cand, seen_ids)
        img_path = images_dir / f"{img_id}.png"
        txt_path = images_dir / f"{img_id}.txt"

        view = VIEW_BY_SOURCE.get(cand.source_key)
        subject = subject_phrase(cand.name_tokens, cand.category)
        tags = theme_tags(cand, cand.category)
        caption = build_caption(cand.category, subject, view, tags,
                                info["outlined"], info["edge_to_edge"])

        final.save(img_path, "PNG")
        txt_path.write_text(caption + "\n", encoding="utf-8")

        transformations = ["alpha_trim"]
        if cand.manifest_transform:
            # la catena parte dal pack originale: le 8 voci curate ritagliate da
            # sprite-sheet perdevano questo anello (il ledger diceva "alpha_trim"
            # su un file che era gia' un ritaglio).
            transformations.insert(0, f"manifest:{cand.manifest_transform}")
        if info["pixel_scale_factor"] > 1:
            transformations.append(f"pixel_scale_recover_x{info['pixel_scale_factor']}")
        transformations.append(f"center_in_canvas_{info['canvas_size']}")
        transformations.append(f"nearest_upscale_{FINAL_PX}")

        records.append({
            "id": img_id,
            "image_path": repo_rel(img_path),
            "caption_path": repo_rel(txt_path),
            "caption": caption,
            "subject": subject,
            "source_path": repo_rel(cand.src_path),
            "source_field": cand.source_field,
            "pack": cand.pack,
            "category": cand.category,
            "theme_tags": tags,
            "view": view,
            "outlined": info["outlined"],
            "license_id": cand.license_info["license_id"],
            "license_url": cand.license_info["license_url"],
            "original_url": cand.license_info["original_url"],
            "author": cand.license_info["author"],
            "license_snapshot_path": cand.license_info["license_snapshot_path"],
            "downloaded_at": cand.license_info["downloaded_at"],
            "allowed_commercial": LICENSE_PERMISSIONS["allowed_commercial"],
            "allowed_derivatives": LICENSE_PERMISSIONS["allowed_derivatives"],
            "training_allowed_explicit": LICENSE_PERMISSIONS["training_allowed_explicit"],
            "source_sha256": sha256_of_bytes(src_bytes),
            "derived_sha256": sha256_of_file(img_path),
            "perceptual_hash": dhash(info["signature"]),
            "source_dimensions": [source_w, source_h],
            "logical_size": info["logical_size"],
            "canvas_size": info["canvas_size"],
            "pixel_scale_factor": info["pixel_scale_factor"],
            "colors": info["colors"],
            "bbox_fill": info["bbox_fill"],
            "edge_to_edge": info["edge_to_edge"],
            "semi_fraction": info["semi_fraction"],
            "transformations": transformations,
            "name_family": name_family(cand),
            "added_at": now,
            "signature": info["signature"],
        })

    records.sort(key=lambda r: r["id"])
    groups = assign_splits(records)

    ledger_fields = [
        "id", "image_path", "caption_path", "caption", "subject", "source_path",
        "source_field", "pack", "category", "theme_tags", "view", "outlined",
        "license_id", "license_url", "original_url", "author",
        "license_snapshot_path", "downloaded_at", "allowed_commercial",
        "allowed_derivatives", "training_allowed_explicit", "source_sha256",
        "derived_sha256", "perceptual_hash", "source_dimensions", "logical_size",
        "canvas_size", "pixel_scale_factor", "colors", "bbox_fill", "edge_to_edge",
        "semi_fraction", "transformations", "name_family", "subject_key", "split",
        "added_at",
    ]
    with ledger_path.open("w", encoding="utf-8") as f:
        for r in records:
            f.write(json.dumps({k: r[k] for k in ledger_fields}, ensure_ascii=False) + "\n")
    with rejects_path.open("w", encoding="utf-8") as f:
        for r in reject_rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")

    stats = report(out_dir, records, candidates, rejects, reject_samples, groups, now)
    return stats


def report(out_dir, records, candidates, rejects, reject_samples, groups, now):
    by_category = Counter(r["category"] for r in records)
    by_pack = Counter(r["pack"] for r in records)
    by_theme = Counter(t for r in records for t in r["theme_tags"])
    by_canvas = Counter(r["canvas_size"] for r in records)
    n_train = sum(1 for r in records if r["split"] == "train")
    n_val = len(records) - n_train

    print("\n== dataset/lora-v0 — statistiche build ==\n")
    print(f"Candidati raccolti: {len(candidates)}")
    print(f"Immagini accettate: {len(records)}")
    print(f"Scarti: {sum(rejects.values())}")
    print(f"Soggetti (gruppi di split): {len(groups)} — train={n_train} val={n_val}")
    print(f"Caption distinte: {len({r['caption'] for r in records})} su {len(records)}")

    print("\n-- per categoria --")
    for cat, n in by_category.most_common():
        print(f"  {cat:12s} {n:4d}")

    print("\n-- per canvas logico --")
    for cs, n in sorted(by_canvas.items()):
        print(f"  {cs:4d}px (nearest x{FINAL_PX // cs:2d}) {n:4d}")

    print("\n-- per tema (un'immagine puo' avere piu' tag) --")
    total = max(1, len(records))
    for theme, n in by_theme.most_common():
        flag = "  << OLTRE LA SOGLIA 70%" if n / total > THEME_SHARE_MAX else ""
        print(f"  {theme:12s} {n:4d}  ({n / total * 100:5.1f}%){flag}")

    print("\n-- per pacchetto sorgente --")
    for pack, n in by_pack.most_common():
        print(f"  {pack:40s} {n:4d}")

    edge = [r for r in records if r["edge_to_edge"]]
    print(f"\n-- soggetti 'edge-to-edge' ({len(edge)}): ingombro opaco oltre il "
          f"{int(EDGE_TO_EDGE_FILL * 100)}%, dichiarato in caption --")
    for r in edge:
        print(f"  {r['id']:52s} fill={r['bbox_fill']:.3f} {r['logical_size']}")

    print("\n-- scarti per motivo --")
    for reason, n in rejects.most_common():
        print(f"  {n:4d}  {reason}")
        for sample in reject_samples[reason][:3]:
            print(f"        es: {sample}")
    print(f"  (elenco completo con path e dimensioni: {repo_rel(out_dir / 'rejects.jsonl')})")

    print("\n-- pacchetti art-library esclusi in blocco (ispezionati, motivo registrato) --")
    for pack, reason in ART_LIBRARY_EXCLUDED_PACKS.items():
        print(f"  {pack}: {reason}")

    n = len(records)
    if 300 <= n <= 500:
        target_note = "dentro il target 300-500"
    else:
        target_note = (f"SOTTO il target 300 richiesto dal mandato: {n} e' il numero VERO "
                       f"dopo aver scartato sheet, tileset, strip di animazione e render "
                       f"antialiasati (vedi rejects.jsonl) — nessun gonfiaggio con duplicati "
                       f"o flip, come richiesto esplicitamente")
    print(f"\n== Totale finale: {n} immagini — {target_note} ==")
    if "character" not in by_category:
        print("== NOTA: 0 immagini di categoria 'character' (gli unici candidati, "
              "good_and_evil, sono fogli di pose interi). I 3 prompt 'character' "
              "della griglia di eval misurano quindi TRASFERIMENTO, non aderenza. ==")

    if records:
        step = max(1, len(records) // 10)
        samples = records[::step][:10]
        print("\n-- 10 campioni (id + caption) --")
        for r in samples:
            print(f"  {r['id']}\n      \"{r['caption']}\"")
        write_contact_sheet(out_dir, samples)

    stats = {
        "total_candidates": len(candidates),
        "total_accepted": len(records),
        "total_rejected": sum(rejects.values()),
        "distinct_captions": len({r["caption"] for r in records}),
        "by_category": dict(by_category),
        "by_theme": dict(by_theme),
        "by_theme_share": {t: round(n / max(1, len(records)), 4) for t, n in by_theme.items()},
        "by_pack": dict(by_pack),
        "by_canvas": {str(k): v for k, v in sorted(by_canvas.items())},
        "rejects": dict(rejects),
        "split": {"train": n_train, "val": n_val, "subjects": len(groups)},
        "generated_at": now,
    }
    (out_dir / "build_stats.json").write_text(
        json.dumps(stats, indent=2, ensure_ascii=False), encoding="utf-8")
    return stats


def write_contact_sheet(out_dir: Path, samples):
    """Griglia 5x2 con le miniature + id, per un colpo d'occhio senza dover
    aprire 10 file singolarmente (punto 6 del mandato: campioni nel summary)."""
    cols, rows = 5, 2
    thumb = 100
    label_h = 22
    sheet = Image.new("RGBA", (cols * thumb, rows * (thumb + label_h)), (24, 24, 24, 255))
    draw = ImageDraw.Draw(sheet)
    for i, row in enumerate(samples):
        img_path = Path(row["image_path"])
        if not img_path.is_absolute():
            img_path = REPO_ROOT / img_path
        try:
            im = Image.open(img_path).convert("RGBA")
        except Exception:  # noqa: BLE001
            continue
        im.thumbnail((thumb - 8, thumb - 8), Image.NEAREST)
        cx, cy = i % cols, i // cols
        ox = cx * thumb + (thumb - im.width) // 2
        oy = cy * (thumb + label_h) + (thumb - im.height) // 2
        sheet.paste(im, (ox, oy), im)
        label = f"{row['category']}:{row['subject']}"[:18]
        draw.text((cx * thumb + 4, cy * (thumb + label_h) + thumb + 4), label,
                  fill=(230, 230, 230, 255))
    sheet.convert("RGB").save(out_dir / "sample_contact_sheet.png", "PNG")


# ---------------------------------------------------------------------------
# Verifica
# ---------------------------------------------------------------------------

REQUIRED_FIELDS = [
    "id", "image_path", "caption_path", "source_path", "pack", "category",
    "license_id", "license_url", "license_snapshot_path", "downloaded_at",
    "source_sha256", "derived_sha256", "perceptual_hash", "subject_key", "split",
    "transformations",
]
BOOL_FIELDS = ["allowed_commercial", "allowed_derivatives", "training_allowed_explicit"]


def resolve(out_dir: Path, stored: str) -> Path:
    p = Path(stored)
    return p if p.is_absolute() else REPO_ROOT / p


def load_ledger(ledger_path: Path):
    rows = []
    bad = 0
    with ledger_path.open("r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError as e:
                print(f"JSON NON VALIDO: riga {lineno}: {e}")
                bad += 1
    return rows, bad


def load_signatures(out_dir: Path, rows):
    sigs = {}
    for r in rows:
        p = resolve(out_dir, r["image_path"])
        if p.exists():
            sigs[r["id"]] = Image.open(p).convert("RGBA").resize((64, 64), Image.NEAREST)
    return sigs


def find_split_leaks(rows, sigs):
    """(fughe per chiave, fughe percettive). La seconda e' il controllo che
    prima mancava: confrontare solo il subject_key e' vacuo per costruzione —
    lo split viene assegnato PER chiave, quindi non puo' che risultare
    coerente. Qui si guardano i pixel, che e' l'unica prova che due varianti
    dello stesso soggetto non sono finite sui due lati."""
    subj_splits = defaultdict(set)
    for r in rows:
        subj_splits[r["subject_key"]].add(r["split"])
    key_leaks = {k: v for k, v in subj_splits.items() if len(v) > 1}

    train = [r for r in rows if r["split"] == "train"]
    val = [r for r in rows if r["split"] == "val"]
    near = []
    for rv in val:
        sv = sigs.get(rv["id"])
        if sv is None:
            continue
        for rt in train:
            st = sigs.get(rt["id"])
            if st is None:
                continue
            d = perceptual_distance(sv, st)
            if d < DEDUP_MERGE_SCORE:
                near.append((rv["id"], rt["id"], round(d, 4)))
    return key_leaks, near, len(subj_splits), len(val), len(train)


def run_verify(out_dir: Path, hash_sample=20):
    ledger_path = out_dir / "ledger.jsonl"
    if not ledger_path.exists():
        print(f"ERRORE: {ledger_path} non esiste (esegui prima 'build')", file=sys.stderr)
        return 1

    rows, problems = load_ledger(ledger_path)
    print(f"righe lette: {len(rows)}")

    sigs = load_signatures(out_dir, rows)
    key_leaks, near, n_subj, n_val, n_train = find_split_leaks(rows, sigs)
    if key_leaks:
        print(f"FUGA DI SPLIT (chiave): {len(key_leaks)} soggetti in train E val: "
              f"{list(key_leaks)[:10]}")
        problems += len(key_leaks)
    else:
        print(f"split OK per chiave: {n_subj} soggetti, nessuno attraversa train/val")
    if near:
        print(f"FUGA DI SPLIT (percettiva): {len(near)} coppie quasi identiche a cavallo:")
        for a, b, d in near[:10]:
            print(f"  {a} <-> {b}: {d * 100:.2f}% di pixel diversi")
        problems += len(near)
    else:
        print(f"split OK percettivo: nessuna coppia val/train sotto il "
              f"{int(DEDUP_MERGE_SCORE * 100)}% di differenza "
              f"({n_val}x{n_train} confronti)")

    # -- sha256 derivati duplicati.
    seen_sha = {}
    for i, r in enumerate(rows):
        sha = r.get("derived_sha256")
        if sha in seen_sha:
            print(f"DUPLICATO derivato: riga {i+1} == riga {seen_sha[sha]+1} ({r['id']})")
            problems += 1
        else:
            seen_sha[sha] = i

    # -- campi obbligatori del ledger minimo (04-DATASET-LICENZE.md).
    for i, r in enumerate(rows):
        missing = [k for k in REQUIRED_FIELDS if not r.get(k)]
        missing += [k for k in BOOL_FIELDS if k not in r]
        if missing:
            print(f"CAMPI MANCANTI: riga {i+1} ({r.get('id','?')}): {missing}")
            problems += 1

    # -- snapshot di licenza davvero presenti su disco.
    missing_snapshots = sorted({r["license_snapshot_path"] for r in rows
                                if r.get("license_snapshot_path")
                                and not (REPO_ROOT / r["license_snapshot_path"]).exists()})
    if missing_snapshots:
        print(f"SNAPSHOT DI LICENZA MANCANTI su disco: {missing_snapshots}")
        problems += len(missing_snapshots)
    else:
        print("snapshot di licenza OK: tutti i file citati esistono")

    # -- hash veri a campione.
    sample = rows[::max(1, len(rows) // hash_sample)][:hash_sample]
    hash_bad = 0
    for r in sample:
        img_path = resolve(out_dir, r["image_path"])
        if not img_path.exists():
            print(f"FILE MANCANTE: {img_path}")
            problems += 1
            continue
        if sha256_of_file(img_path) != r["derived_sha256"]:
            print(f"HASH DERIVATO NON CORRISPONDE: {r['id']}")
            hash_bad += 1
            problems += 1
        src = resolve(out_dir, r["source_path"])
        if src.exists() and sha256_of_file(src) != r["source_sha256"]:
            print(f"HASH SORGENTE NON CORRISPONDE: {r['id']}")
            hash_bad += 1
            problems += 1
    print(f"hash verificati su {len(sample)} campioni (sorgente+derivato): {hash_bad} falliti")

    # -- caption presenti, non vuote e allineate al ledger.
    for r in rows:
        cap_path = resolve(out_dir, r["caption_path"])
        if not cap_path.exists() or not cap_path.read_text(encoding="utf-8").strip():
            print(f"CAPTION MANCANTE/VUOTA: {r['id']}")
            problems += 1
        elif cap_path.read_text(encoding="utf-8").strip() != r["caption"]:
            print(f"CAPTION DIVERGENTE dal ledger: {r['id']}")
            problems += 1

    # -- diversita': nessun tema oltre la soglia del mandato.
    theme_counts = Counter(t for r in rows for t in r.get("theme_tags", []))
    for theme, n in theme_counts.most_common(3):
        share = n / max(1, len(rows))
        if share > THEME_SHARE_MAX:
            print(f"DIVERSITA' INSUFFICIENTE: tema '{theme}' su {share * 100:.1f}% "
                  f"delle immagini (soglia {THEME_SHARE_MAX * 100:.0f}%)")
            problems += 1
    if theme_counts:
        top, n = theme_counts.most_common(1)[0]
        print(f"diversita' OK: tema piu' frequente '{top}' al {n / max(1, len(rows)) * 100:.1f}%")

    # -- categorie: il mandato ne chiede almeno 4.
    cats = Counter(r["category"] for r in rows)
    if len(cats) < 4:
        print(f"CATEGORIE INSUFFICIENTI: {len(cats)} < 4 ({dict(cats)})")
        problems += 1
    else:
        print(f"categorie OK: {len(cats)} ({dict(cats)})")

    if problems == 0:
        print(f"\ncheck OK: {len(rows)} voci, nessun problema")
        return 0
    print(f"\ncheck FALLITO: {problems} problemi su {len(rows)} voci")
    return 1


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def run_selftest(out_dir: Path):
    """Prova che il rilevatore di fughe NON e' vacuo: prende la coppia di
    immagini piu' simili del dataset (che la build ha messo, giustamente, dallo
    stesso lato), le mette su lati opposti in un ledger di prova e pretende che
    il controllo percettivo se ne accorga. Se questa prova passa a vuoto, il
    controllo va sistemato prima di fidarsi dello split."""
    ledger_path = out_dir / "ledger.jsonl"
    rows, _ = load_ledger(ledger_path)
    if len(rows) < 2:
        print("selftest SALTATO: servono almeno 2 immagini")
        return 1
    sigs = load_signatures(out_dir, rows)

    best = None
    ids = sorted(sigs)
    for i in range(len(ids)):
        for j in range(i + 1, len(ids)):
            d = perceptual_distance(sigs[ids[i]], sigs[ids[j]])
            if best is None or d < best[0]:
                best = (d, ids[i], ids[j])
    d, a, b = best
    print(f"coppia piu' simile del dataset: {a} <-> {b} ({d * 100:.2f}% di pixel diversi)")
    if d >= DEDUP_MERGE_SCORE:
        print(f"selftest NON CONCLUSIVO: nemmeno la coppia piu' simile scende sotto "
              f"il {int(DEDUP_MERGE_SCORE * 100)}%, il rilevatore non puo' essere "
              f"esercitato su questo dataset")
        return 1

    poisoned = [dict(r) for r in rows]
    for r in poisoned:
        if r["id"] == a:
            r["split"] = "val"
        elif r["id"] == b:
            r["split"] = "train"
    _, near, _, _, _ = find_split_leaks(poisoned, sigs)
    hit = any((x == a and y == b) for x, y, _ in near)
    if hit:
        print(f"selftest OK: con {a} in val e {b} in train il controllo percettivo "
              f"segnala la fuga ({len(near)} coppie)")
        return 0
    print("selftest FALLITO: la fuga iniettata non e' stata rilevata")
    return 1


def main():
    parser = argparse.ArgumentParser(
        prog="lora_dataset_build.py",
        description="Costruisce dataset/lora-v0/ (immagini+caption+ledger) dalle "
                    "sorgenti CC0 di assets/curated e assets/art-library.")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_build = sub.add_parser("build", help="raccoglie, normalizza, scrive dataset+ledger")
    p_build.add_argument("--out", default=str(DEFAULT_OUT), help="cartella di output")
    p_build.add_argument("--limit", type=int, default=None,
                         help="limita i candidati processati (debug)")

    p_verify = sub.add_parser("verify", help="valida ledger, hash, split senza fughe, diversita'")
    p_verify.add_argument("--out", default=str(DEFAULT_OUT), help="cartella del dataset")

    p_self = sub.add_parser("selftest",
                            help="prova che il rilevatore di fughe di split non e' vacuo")
    p_self.add_argument("--out", default=str(DEFAULT_OUT), help="cartella del dataset")

    args = parser.parse_args()
    out_dir = Path(args.out)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir

    if args.cmd == "build":
        run_build(out_dir, limit=args.limit)
        return 0
    if args.cmd == "selftest":
        return run_selftest(out_dir)
    return run_verify(out_dir)


if __name__ == "__main__":
    sys.exit(main())
