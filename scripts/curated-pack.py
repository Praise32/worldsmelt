#!/usr/bin/env python3
"""Costruisce assets/curated/ a partire dal corpus CC0 di dataset-raw/ (DEC-171).

Il contenuto curato della demo usa le immagini del dataset di training gia' registrato
nel ledger (docs/ai-production/dataset/ledger.jsonl: pacchetti Kenney +
superpowers-asset-packs, licenza CC0, vedi docs/ai-production/04-DATASET-LICENZE.md e
docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md). Questo script NON genera nuovi
asset: seleziona e copia file gia' esistenti e licenziati. L'unica trasformazione ammessa
e' il ritaglio (crop) del primo fotogramma di alcuni sprite-sheet di rpg-battle-system
(vedi CROPPED_FRAMES sotto) quando il pacchetto non fornisce gia' un singolo fotogramma:
e' un ritaglio meccanico di unread pixel gia' esistenti, non contenuto nuovo, ed e'
documentato nel manifest col campo "transform".

Uso:
    python3 scripts/curated-pack.py            # ricostruisce assets/curated/ da zero
    python3 scripts/curated-pack.py --dry-run   # mostra solo i conteggi, non scrive nulla

Rigenerare dopo una modifica alla SELECTION qui sotto: lo script e' idempotente, ripulisce
assets/curated/{items,enemies,bosses,props} prima di ricopiare.
"""

import argparse
import json
import shutil
import sys
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
DATASET_RAW = REPO_ROOT / "dataset-raw"
CURATED_DIR = REPO_ROOT / "assets" / "curated"

# ---------------------------------------------------------------------------
# Selezione dichiarata a mano (curation, non estrazione automatica): ogni riga
# e' (id, path-relativo-a-dataset-raw, tags). Il path relativo diventa anche il
# campo "source" del manifest (== "<pacchetto>/<file-origine>" richiesto dal
# task). "category" e' fissata dalla tabella in cui la riga compare.
# ---------------------------------------------------------------------------

ITEMS = [
    # -- rpg-battle-system/item: icone RPG pulite, stile coerente (24) --
    ("helm-teal-winged", "superpowers-asset-packs/rpg-battle-system/item/1.png", ["armor", "helmet"]),
    ("helm-horned-yellow", "superpowers-asset-packs/rpg-battle-system/item/4.png", ["armor", "helmet"]),
    ("gauntlet-yellow", "superpowers-asset-packs/rpg-battle-system/item/6.png", ["armor", "gloves"]),
    ("potion-red", "superpowers-asset-packs/rpg-battle-system/item/10.png", ["potion", "health"]),
    ("potion-green", "superpowers-asset-packs/rpg-battle-system/item/11.png", ["potion", "nature"]),
    ("potion-blue", "superpowers-asset-packs/rpg-battle-system/item/12.png", ["potion", "mana"]),
    ("potion-yellow", "superpowers-asset-packs/rpg-battle-system/item/13.png", ["potion", "speed"]),
    ("bread-loaf", "superpowers-asset-packs/rpg-battle-system/item/14.png", ["food", "consumable"]),
    ("lantern-blue", "superpowers-asset-packs/rpg-battle-system/item/17.png", ["gadget", "light"]),
    ("coin-disc-one", "superpowers-asset-packs/rpg-battle-system/item/19.png", ["currency"]),
    ("pouch-brown", "superpowers-asset-packs/rpg-battle-system/item/21.png", ["container", "treasure"]),
    ("goblet-gold", "superpowers-asset-packs/rpg-battle-system/item/26.png", ["treasure"]),
    ("shield-round-brown", "superpowers-asset-packs/rpg-battle-system/item/28.png", ["armor", "shield"]),
    ("dagger-brown", "superpowers-asset-packs/rpg-battle-system/item/31.png", ["weapon", "dagger"]),
    ("dagger-yellow", "superpowers-asset-packs/rpg-battle-system/item/33.png", ["weapon", "dagger"]),
    ("bow-wood", "superpowers-asset-packs/rpg-battle-system/item/34.png", ["weapon", "ranged"]),
    ("ring-purple-gem", "superpowers-asset-packs/rpg-battle-system/item/37.png", ["accessory", "ring"]),
    ("ring-gold-plain", "superpowers-asset-packs/rpg-battle-system/item/38.png", ["accessory", "ring"]),
    ("helm-knight-teal", "superpowers-asset-packs/rpg-battle-system/item/45.png", ["armor", "helmet"]),
    ("sword-ornate-red", "superpowers-asset-packs/rpg-battle-system/item/46.png", ["weapon", "sword"]),
    ("emblem-flame-gold", "superpowers-asset-packs/rpg-battle-system/item/49.png", ["gadget", "magic", "fire"]),
    ("gem-diamond-white", "superpowers-asset-packs/rpg-battle-system/item/57.png", ["gem", "treasure"]),
    ("hammer-gray", "superpowers-asset-packs/rpg-battle-system/item/70.png", ["weapon", "blunt"]),
    ("staff-gold-cross", "superpowers-asset-packs/rpg-battle-system/item/77.png", ["weapon", "magic"]),

    # -- medieval-fantasy/items (numerati) (6) --
    ("helm-red-orange", "superpowers-asset-packs/medieval-fantasy/items/2.png", ["armor", "helmet"]),
    ("shield-cream-cross", "superpowers-asset-packs/medieval-fantasy/items/5.png", ["armor", "shield"]),
    ("emblem-gold-red-gem", "superpowers-asset-packs/medieval-fantasy/items/7.png", ["accessory", "emblem"]),
    ("gem-blue-orb", "superpowers-asset-packs/medieval-fantasy/items/12.png", ["gem"]),
    ("orb-fire-yellow", "superpowers-asset-packs/medieval-fantasy/items/13.png", ["gadget", "magic", "fire"]),
    ("glove-red-claw", "superpowers-asset-packs/medieval-fantasy/items/19.png", ["armor", "gloves"]),

    # -- medieval-fantasy/items/gold-&-gem (6) --
    ("coin-gold-round", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/coin.png", ["currency"]),
    ("gem-blue-cut", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/gem-1.png", ["gem"]),
    ("gem-green-square", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/gem-2.png", ["gem"]),
    ("gem-red-chunk", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/gem-3.png", ["gem"]),
    ("gem-orange-cube", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/gem-4.png", ["gem"]),
    ("gold-pile", "superpowers-asset-packs/medieval-fantasy/items/gold-&-gem/gold.png", ["currency", "treasure"]),

    # -- medieval-fantasy/items/element: icone elementali astratte, usate anche
    #    come riserva per le fusioni (fusion-reserve, DEC-171) (8) --
    ("element-fire", "superpowers-asset-packs/medieval-fantasy/items/element/1.png", ["element", "fire", "fusion-reserve"]),
    ("element-water", "superpowers-asset-packs/medieval-fantasy/items/element/2.png", ["element", "water", "fusion-reserve"]),
    ("element-lightning", "superpowers-asset-packs/medieval-fantasy/items/element/3.png", ["element", "lightning", "fusion-reserve"]),
    ("element-earth", "superpowers-asset-packs/medieval-fantasy/items/element/4.png", ["element", "earth", "fusion-reserve"]),
    ("element-leaf", "superpowers-asset-packs/medieval-fantasy/items/element/5.png", ["element", "nature", "fusion-reserve"]),
    ("element-wind", "superpowers-asset-packs/medieval-fantasy/items/element/6.png", ["element", "wind", "fusion-reserve"]),
    ("element-light", "superpowers-asset-packs/medieval-fantasy/items/element/7.png", ["element", "light", "fusion-reserve"]),
    ("element-dark", "superpowers-asset-packs/medieval-fantasy/items/element/8.png", ["element", "dark", "fusion-reserve"]),

    # -- medieval-fantasy/items/weapon (5) --
    ("sword-brown-hilt", "superpowers-asset-packs/medieval-fantasy/items/weapon/1.png", ["weapon", "sword"]),
    ("sword-silver", "superpowers-asset-packs/medieval-fantasy/items/weapon/2.png", ["weapon", "sword"]),
    ("dagger-red-blade", "superpowers-asset-packs/medieval-fantasy/items/weapon/4.png", ["weapon", "dagger"]),
    ("axe-double-tan", "superpowers-asset-packs/medieval-fantasy/items/weapon/5.png", ["weapon", "axe"]),
    ("spear-brown", "superpowers-asset-packs/medieval-fantasy/items/weapon/9.png", ["weapon", "spear"]),

    # -- medieval-fantasy/items/projectile (2) --
    ("projectile-fireball-orange", "superpowers-asset-packs/medieval-fantasy/items/projectile/1.png", ["projectile", "fire"]),
    ("projectile-ice-shard", "superpowers-asset-packs/medieval-fantasy/items/projectile/2.png", ["projectile", "ice"]),

    # -- ninja-adventure/items (9) --
    ("potion-life-ninja", "superpowers-asset-packs/ninja-adventure/items/life-pot.png", ["potion", "health"]),
    ("medipack", "superpowers-asset-packs/ninja-adventure/items/medipack.png", ["gadget", "health"]),
    ("coin-gold-ninja", "superpowers-asset-packs/ninja-adventure/items/gold-coin.png", ["currency"]),
    ("key-gold-ninja", "superpowers-asset-packs/ninja-adventure/items/gold-key.png", ["key"]),
    ("scroll-fire", "superpowers-asset-packs/ninja-adventure/items/scroll-fire.png", ["gadget", "magic", "fire"]),
    ("heart-pickup", "superpowers-asset-packs/ninja-adventure/items/heart.png", ["consumable", "health"]),
    ("shuriken", "superpowers-asset-packs/ninja-adventure/items/shuriken.png", ["weapon", "throwing"]),
    ("kunai", "superpowers-asset-packs/ninja-adventure/items/kunai.png", ["weapon", "throwing"]),
    ("fireball-ninja", "superpowers-asset-packs/ninja-adventure/items/fireball.png", ["gadget", "magic", "fire"]),

    # -- ninja-adventure/weapons (3) --
    ("axe-ninja", "superpowers-asset-packs/ninja-adventure/weapons/axe.png", ["weapon", "axe"]),
    ("katana", "superpowers-asset-packs/ninja-adventure/weapons/katana.png", ["weapon", "sword"]),
    ("sai", "superpowers-asset-packs/ninja-adventure/weapons/sai.png", ["weapon", "dagger"]),

    # -- western-fps-2d/item (6) --
    ("dice-pair", "superpowers-asset-packs/western-fps-2d/item/dice.png", ["gadget", "curio"]),
    ("pocket-watch", "superpowers-asset-packs/western-fps-2d/item/pocket-watch.png", ["gadget", "curio"]),
    ("poison-vial", "superpowers-asset-packs/western-fps-2d/item/poison.png", ["potion", "poison"]),
    ("horseshoe", "superpowers-asset-packs/western-fps-2d/item/horseshoe.png", ["gadget", "curio"]),
    ("ruby-gem", "superpowers-asset-packs/western-fps-2d/item/rubis.png", ["gem"]),
    ("gold-bar", "superpowers-asset-packs/western-fps-2d/item/gold-lingo.png", ["currency", "treasure"]),

    # -- space-shooter/items (5, due astratti come riserva fusione) --
    ("gem-pink-star", "superpowers-asset-packs/space-shooter/items/gem-1.png", ["gem"]),
    ("gem-white-star", "superpowers-asset-packs/space-shooter/items/gem-2.png", ["gem"]),
    ("rocket-gadget", "superpowers-asset-packs/space-shooter/items/rocket-1.png", ["gadget", "sci-fi"]),
    ("bubble-shield", "superpowers-asset-packs/space-shooter/items/bubble.png", ["gadget", "sci-fi", "fusion-reserve"]),
    ("orb-red-ball", "superpowers-asset-packs/space-shooter/items/ball.png", ["gadget", "fusion-reserve"]),

    # -- kenney-tiny-dungeon/Tiles (7) --
    ("sword-silver-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0104.png", ["weapon", "sword"]),
    ("potion-green-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0114.png", ["potion", "nature"]),
    ("potion-red-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0115.png", ["potion", "health"]),
    ("potion-blue-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0116.png", ["potion", "mana"]),
    ("axe-double-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0118.png", ["weapon", "axe"]),
    ("gem-purple-scepter-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0129.png", ["gem", "accessory"]),
    ("wand-blue-crystal-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0130.png", ["weapon", "magic"]),

    # -- kenney-micro-roguelike/Tiles/Colored (4) --
    ("key-padlock-micro", "kenney-micro-roguelike/extracted/Tiles/Colored/tile_0039.png", ["key", "micro"]),
    ("coin-gold-micro", "kenney-micro-roguelike/extracted/Tiles/Colored/tile_0088.png", ["currency", "micro"]),
    ("heart-micro", "kenney-micro-roguelike/extracted/Tiles/Colored/tile_0100.png", ["consumable", "health", "micro"]),
    ("potion-gray-micro", "kenney-micro-roguelike/extracted/Tiles/Colored/tile_0135.png", ["potion", "micro"]),
]

ENEMIES = [
    # -- rpg-battle-system/monster (5; 4 ritagliati dal primo fotogramma, vedi CROPPED_FRAMES) --
    ("boar-tusked", "superpowers-asset-packs/rpg-battle-system/monster/boar.png", ["beast"]),
    ("bat-winged", "superpowers-asset-packs/rpg-battle-system/monster/bat/sprite-sheet-121x89.png", ["flying", "beast"]),
    ("dino-orange", "superpowers-asset-packs/rpg-battle-system/monster/dino/sprite-sheet-154x118.png", ["beast", "prehistoric"]),
    ("ghost-pale", "superpowers-asset-packs/rpg-battle-system/monster/ghost/sprite-sheet-112x93.png", ["undead", "flying"]),
    ("slime-teal", "superpowers-asset-packs/rpg-battle-system/monster/slime/sprite-sheet-141x107.png", ["ooze"]),

    # -- medieval-fantasy/monsters (5) --
    ("bat-dark-red", "superpowers-asset-packs/medieval-fantasy/monsters/bat.png", ["flying", "beast"]),
    ("goblin-green", "superpowers-asset-packs/medieval-fantasy/monsters/goblin.png", ["humanoid"]),
    ("skeleton-axe", "superpowers-asset-packs/medieval-fantasy/monsters/skeleton.png", ["undead"]),
    ("slime-blue", "superpowers-asset-packs/medieval-fantasy/monsters/slim.png", ["ooze"]),
    ("snake-teal-orange", "superpowers-asset-packs/medieval-fantasy/monsters/snake.png", ["beast", "reptile"]),

    # -- medieval-fantasy/animals (6, fauna a bassa minaccia) --
    ("turtle-green", "superpowers-asset-packs/medieval-fantasy/animals/1.png", ["beast", "neutral"]),
    ("sheep-white", "superpowers-asset-packs/medieval-fantasy/animals/2.png", ["beast", "neutral"]),
    ("armadillo-orange", "superpowers-asset-packs/medieval-fantasy/animals/3.png", ["beast", "neutral"]),
    ("dog-gray", "superpowers-asset-packs/medieval-fantasy/animals/4.png", ["beast", "neutral"]),
    ("horse-brown", "superpowers-asset-packs/medieval-fantasy/animals/5.png", ["beast", "neutral"]),
    ("boar-wolf-brown", "superpowers-asset-packs/medieval-fantasy/animals/6.png", ["beast", "neutral"]),

    # -- kenney-tiny-dungeon/Tiles (9 mostri identificati sulla griglia) --
    ("slime-green-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0108.png", ["ooze"]),
    ("dwarf-brown-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0109.png", ["humanoid"]),
    ("crab-red-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0110.png", ["beast"]),
    ("skeleton-white-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0111.png", ["undead"]),
    ("goblin-scarf-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0112.png", ["humanoid"]),
    ("ghost-pale-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0121.png", ["undead", "flying"]),
    ("mummy-brown-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0122.png", ["undead"]),
    ("shellback-brown-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0123.png", ["beast"]),
    ("shellback-gray-variant-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0124.png", ["beast"]),

    # -- kenney-tiny-dungeon/Tiles: rilievi murali (gargoyle) usati come guardiani fissi (2)
    #    (NON confondere con medieval-fantasy/background-elements/18-19, che sono alberi) --
    ("gargoyle-wall-plain-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0019.png", ["stationary", "guardian"]),
    ("gargoyle-wall-glowing-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0020.png", ["stationary", "guardian"]),

    # -- kenney-top-down-shooter/PNG (9 pose "stand" pulite, un frame ciascuna) --
    ("hitman-stand", "kenney-top-down-shooter/extracted/PNG/Hitman 1/hitman1_stand.png", ["humanoid", "rogue"]),
    ("man-blue-stand", "kenney-top-down-shooter/extracted/PNG/Man Blue/manBlue_stand.png", ["humanoid"]),
    ("man-brown-stand", "kenney-top-down-shooter/extracted/PNG/Man Brown/manBrown_stand.png", ["humanoid"]),
    ("man-old-stand", "kenney-top-down-shooter/extracted/PNG/Man Old/manOld_stand.png", ["humanoid"]),
    ("robot-stand", "kenney-top-down-shooter/extracted/PNG/Robot 1/robot1_stand.png", ["mechanical"]),
    ("soldier-stand", "kenney-top-down-shooter/extracted/PNG/Soldier 1/soldier1_stand.png", ["humanoid", "guard"]),
    ("survivor-stand", "kenney-top-down-shooter/extracted/PNG/Survivor 1/survivor1_stand.png", ["humanoid"]),
    ("woman-green-stand", "kenney-top-down-shooter/extracted/PNG/Woman Green/womanGreen_stand.png", ["humanoid"]),
    ("zombie-stand", "kenney-top-down-shooter/extracted/PNG/Zombie 1/zoimbie1_stand.png", ["undead"]),

    # -- superpowers-asset-packs/top-down-shooter: torretta ferma (1) --
    ("turret-base", "superpowers-asset-packs/top-down-shooter/characters/turret/base.png", ["mechanical", "stationary"]),

    # -- space-shooter/ships (6 su 10; il decimo va a bosses) --
    ("ship-crab-purple", "superpowers-asset-packs/space-shooter/ships/1.png", ["flying", "sci-fi"]),
    ("ship-green-fighter", "superpowers-asset-packs/space-shooter/ships/2.png", ["flying", "sci-fi"]),
    ("ship-bird-orange", "superpowers-asset-packs/space-shooter/ships/3.png", ["flying", "sci-fi"]),
    ("ship-green-scout", "superpowers-asset-packs/space-shooter/ships/4.png", ["flying", "sci-fi"]),
    ("ship-turtle-teal", "superpowers-asset-packs/space-shooter/ships/6.png", ["flying", "sci-fi"]),
    ("ship-octopus-green", "superpowers-asset-packs/space-shooter/ships/9.png", ["flying", "sci-fi"]),

    # -- kenney-pixel-shmup/Ships (6 su 24) --
    ("shmup-fighter-blue", "kenney-pixel-shmup/extracted/Ships/ship_0000.png", ["flying", "sci-fi"]),
    ("shmup-fighter-green", "kenney-pixel-shmup/extracted/Ships/ship_0002.png", ["flying", "sci-fi"]),
    ("shmup-fighter-red", "kenney-pixel-shmup/extracted/Ships/ship_0005.png", ["flying", "sci-fi"]),
    ("shmup-fighter-orange", "kenney-pixel-shmup/extracted/Ships/ship_0007.png", ["flying", "sci-fi"]),
    ("shmup-cruiser-gray-a", "kenney-pixel-shmup/extracted/Ships/ship_0012.png", ["flying", "sci-fi"]),
    ("shmup-cruiser-gray-b", "kenney-pixel-shmup/extracted/Ships/ship_0019.png", ["flying", "sci-fi"]),

    # Nota: le 4 "teste mostro" 8x8 di kenney-micro-roguelike/Tiles/Colored (indici 5, 8,
    # 11, 12) sono state scartate dopo verifica visiva a campione: ingrandite, si leggono
    # come macchie di colore a croce, non come creature riconoscibili. Vedi
    # logs/curated-pack/ e questions-night.md per il dettaglio dello scarto.
]

BOSSES = [
    ("boss-giant", "superpowers-asset-packs/rpg-battle-system/monster/giant.png", ["boss", "large"]),
    ("boss-mimic-chest", "superpowers-asset-packs/rpg-battle-system/monster/chest.png", ["boss", "mimic"]),
    ("boss-yeti", "superpowers-asset-packs/rpg-battle-system/monster/yeti.png", ["boss", "beast"]),
    ("boss-octopus-abyss", "superpowers-asset-packs/rpg-battle-system/monster/octopus.png", ["boss", "aquatic"]),
    ("boss-dragon-ember", "superpowers-asset-packs/rpg-battle-system/monster/dragon/sprite-sheet-2-258x209.png", ["boss", "dragon", "fire"]),
    ("boss-mushroom-titan", "superpowers-asset-packs/rpg-battle-system/monster/mushroom/sprite-sheet-192x163.png", ["boss", "nature"]),
    ("boss-reptile-warlord", "superpowers-asset-packs/rpg-battle-system/monster/reptile/sprite-sheet- 248x151.png", ["boss", "reptile"]),
    ("boss-naga", "superpowers-asset-packs/rpg-battle-system/monster/snake/sprite-sheet-147x94.png", ["boss", "reptile"]),
    ("boss-king-skeleton", "superpowers-asset-packs/medieval-fantasy/monsters/king skeleton.png", ["boss", "undead"]),
    ("boss-dragon-teal", "superpowers-asset-packs/medieval-fantasy/monsters/dragon.png", ["boss", "dragon"]),
    ("boss-cyclop-brute", "superpowers-asset-packs/medieval-fantasy/monsters/cyclop.png", ["boss", "humanoid"]),
    ("boss-leonard-bloom", "superpowers-asset-packs/medieval-fantasy/monsters/leonard.png", ["boss", "nature"]),
    ("boss-blob-elder", "kenney-tiny-dungeon/extracted/Tiles/tile_0120.png", ["boss", "ooze"]),
    ("boss-mecha-guard", "superpowers-asset-packs/top-down-shooter/characters/robot/example.png", ["boss", "mechanical"]),
    ("boss-void-dreadnought", "superpowers-asset-packs/space-shooter/ships/8.png", ["boss", "sci-fi", "flying"]),
]

PROPS = [
    # -- kenney-tiny-dungeon/Tiles (18) --
    ("barrel-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0066.png", ["obstacle", "container"]),
    ("crate-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0063.png", ["obstacle", "container"]),
    ("crate-small-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0073.png", ["obstacle", "container"]),
    ("anvil-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0064.png", ["obstacle", "decor"]),
    ("statue-bust-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0074.png", ["obstacle", "decor"]),
    ("sarcophagus-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0054.png", ["obstacle", "decor"]),
    ("sarcophagus-tall-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0055.png", ["obstacle", "decor"]),
    ("tomb-standing-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0065.png", ["obstacle", "decor"]),
    ("fence-a-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0067.png", ["obstacle", "barrier"]),
    ("fence-b-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0068.png", ["obstacle", "barrier"]),
    ("fence-c-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0075.png", ["obstacle", "barrier"]),
    ("chest-wood-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0089.png", ["obstacle", "chest"]),
    ("chest-metal-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0090.png", ["obstacle", "chest"]),
    ("chest-ornate-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0091.png", ["obstacle", "chest"]),
    ("chest-red-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0092.png", ["obstacle", "chest"]),
    ("vault-door-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0069.png", ["obstacle", "gate"]),
    ("door-brown-a-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0033.png", ["obstacle", "door"]),
    ("door-brown-b-kenney", "kenney-tiny-dungeon/extracted/Tiles/tile_0045.png", ["obstacle", "door"]),

    # -- medieval-fantasy/background-elements + top-level (11) --
    ("well-stone", "superpowers-asset-packs/medieval-fantasy/background-elements/11.png", ["obstacle", "decor"]),
    ("fountain-stone", "superpowers-asset-packs/medieval-fantasy/background-elements/12.png", ["obstacle", "decor"]),
    ("statue-gray", "superpowers-asset-packs/medieval-fantasy/background-elements/14.png", ["obstacle", "decor"]),
    ("gravestone-shrine", "superpowers-asset-packs/medieval-fantasy/background-elements/16.png", ["obstacle", "decor"]),
    ("cauldron-black", "superpowers-asset-packs/medieval-fantasy/background-elements/8.png", ["obstacle", "decor"]),
    ("tree-round-green", "superpowers-asset-packs/medieval-fantasy/background-elements/6.png", ["obstacle", "nature"]),
    ("tree-bush-green", "superpowers-asset-packs/medieval-fantasy/background-elements/18.png", ["obstacle", "nature"]),
    ("tree-dead-brown", "superpowers-asset-packs/medieval-fantasy/background-elements/19.png", ["obstacle", "nature"]),
    ("tree-pine-green", "superpowers-asset-packs/medieval-fantasy/background-elements/20.png", ["obstacle", "nature"]),
    ("barrel-medieval", "superpowers-asset-packs/medieval-fantasy/items/barrel.png", ["obstacle", "container"]),
    ("crate-medieval", "superpowers-asset-packs/medieval-fantasy/items/crate.png", ["obstacle", "container"]),

    # -- ninja-adventure/items: contenitori (6) --
    ("chest-big-ninja", "superpowers-asset-packs/ninja-adventure/items/big-treasure-chest.png", ["obstacle", "chest"]),
    ("chest-small-ninja", "superpowers-asset-packs/ninja-adventure/items/little-treasure-chest.png", ["obstacle", "chest"]),
    ("jar-ninja", "superpowers-asset-packs/ninja-adventure/items/jar.png", ["obstacle", "container"]),
    ("pot-empty-ninja", "superpowers-asset-packs/ninja-adventure/items/empty-pot.png", ["obstacle", "container"]),
    ("pot-water-ninja", "superpowers-asset-packs/ninja-adventure/items/water-pot.png", ["obstacle", "container"]),
    ("pot-milk-ninja", "superpowers-asset-packs/ninja-adventure/items/milk-pot.png", ["obstacle", "container"]),

    # -- space-shooter/items: casse sci-fi (2) --
    ("crate-scifi-a", "superpowers-asset-packs/space-shooter/items/crate-1.png", ["obstacle", "container", "sci-fi"]),
    ("crate-scifi-b", "superpowers-asset-packs/space-shooter/items/crate-2.png", ["obstacle", "container", "sci-fi"]),

    # -- kenney-micro-roguelike: lapide (1) --
    ("tombstone-micro", "kenney-micro-roguelike/extracted/Tiles/Colored/tile_0120.png", ["obstacle", "decor", "micro"]),

    # -- medieval-fantasy/items: forzieri chiusi (2) --
    ("chest-wood-medieval", "superpowers-asset-packs/medieval-fantasy/items/wood-chest-close.png", ["obstacle", "chest"]),
    ("chest-gold-medieval", "superpowers-asset-packs/medieval-fantasy/items/gold-chest-close.png", ["obstacle", "chest"]),
]

# Sprite-sheet dove il pacchetto non fornisce un singolo fotogramma: si ritaglia il primo
# fotogramma (in alto a sinistra), la posa "idle" per convenzione di questo pacchetto (lo
# stesso autore lo fa a mano per boar/chest/giant/octopus/yeti, gia' forniti come singoli).
# Formato: id -> (larghezza_frame, altezza_frame).
CROPPED_FRAMES = {
    "bat-winged": (121, 89),
    "dino-orange": (154, 118),
    "ghost-pale": (112, 93),
    "slime-teal": (141, 107),
    "boss-dragon-ember": (258, 209),
    "boss-mushroom-titan": (192, 163),
    "boss-reptile-warlord": (248, 151),
    "boss-naga": (147, 94),
}

CATEGORIES = {
    "items": ITEMS,
    "enemies": ENEMIES,
    "bosses": BOSSES,
    "props": PROPS,
}


def build(dry_run=False):
    manifest_images = []
    counts = {}
    total_bytes = 0

    for category, rows in CATEGORIES.items():
        out_dir = CURATED_DIR / category
        if not dry_run:
            if out_dir.exists():
                shutil.rmtree(out_dir)
            out_dir.mkdir(parents=True, exist_ok=True)

        seen_ids = set()
        for asset_id, rel_source, tags in rows:
            if asset_id in seen_ids:
                raise ValueError(f"id duplicato in categoria {category}: {asset_id}")
            seen_ids.add(asset_id)

            src_path = DATASET_RAW / rel_source
            if not src_path.is_file():
                raise FileNotFoundError(f"sorgente mancante: {src_path}")

            dest_path = out_dir / f"{asset_id}.png"
            transform = None

            if asset_id in CROPPED_FRAMES:
                w, h = CROPPED_FRAMES[asset_id]
                if not dry_run:
                    im = Image.open(src_path).convert("RGBA")
                    crop = im.crop((0, 0, w, h))
                    crop.save(dest_path)
                size = (w, h)
                transform = f"crop-first-frame:{w}x{h}"
            else:
                if not dry_run:
                    shutil.copyfile(src_path, dest_path)
                with Image.open(src_path) as im:
                    size = im.size

            file_size = dest_path.stat().st_size if (not dry_run and dest_path.exists()) else src_path.stat().st_size
            total_bytes += file_size

            entry = {
                "id": asset_id,
                "file": f"{category}/{asset_id}.png",
                "category": category[:-1] if category != "props" else "prop",
                "tags": tags,
                "size": list(size),
                "source": rel_source,
            }
            if transform:
                entry["transform"] = transform
            manifest_images.append(entry)

        counts[category] = len(rows)

    manifest = {"images": manifest_images}

    if not dry_run:
        CURATED_DIR.mkdir(parents=True, exist_ok=True)
        with open(CURATED_DIR / "manifest.json", "w", encoding="utf-8") as f:
            json.dump(manifest, f, indent=2, ensure_ascii=False)
            f.write("\n")

    return counts, total_bytes, manifest_images


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    counts, total_bytes, manifest_images = build(dry_run=args.dry_run)

    print("== curated-pack: conteggi ==")
    for cat, n in counts.items():
        print(f"  {cat:10s} {n}")
    print(f"  {'totale':10s} {sum(counts.values())}")
    print(f"  dimensione totale copie: {total_bytes / 1024:.1f} KiB ({total_bytes / (1024*1024):.2f} MiB)")
    if args.dry_run:
        print("(dry-run: nessun file scritto)")


if __name__ == "__main__":
    sys.exit(main())
