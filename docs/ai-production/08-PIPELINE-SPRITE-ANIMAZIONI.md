---
id: aiprod-pipeline-sprite-animazioni
title: Pipeline sprite e animazioni
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Pipeline dalla EnemySpec allo SpriteBundle animato: guide/ControlNet, Pixel Art Fixer, validazione automatica, formato eventi/animazioni e struct raylib. Aseprite è ora anche il percorso di produzione diretta di sprite originali (HUD, personaggio, nemici, boss, oggetti, colpi, prop) a contratto spritesheet fisso, che serve insieme come asset di gioco e dataset LoRA (DEC-175).
last_reviewed: 2026-07-30
topics: [sprite-bundle, animazione, raylib, pixel-art-fixer, validazione, controlnet, aseprite, contratto-spritesheet, DEC-175, tileset-manifest, ui-9patch, font-pixel]
related: []
supersedes: []
source_files: [src/render, src/assets]
---
# Pipeline sprite e animazioni

## Vista del gioco

Descrizione consigliata per prompt e dataset:

```text
top-down three-quarter 2D game sprite
orthographic game view
visible top and front surfaces
single centered subject
flat ground shadow
transparent or flat removable background
```

Non affidarsi alla sola parola `2.5D`.

## Pipeline generale

```text
EnemySpec / CharacterSpec
        |
        v
reference canonica
        |
        +--> componenti per rig
        |
        +--> pose guide per frame chiave
        v
SD1.5 + Style LoRA + Role LoRA
        v
Pixel Art Fixer / postprocess interno
        v
alpha + palette + silhouette validation
        v
SpriteBundle
        v
animazione raylib
```

## Aseprite: produzione diretta di sprite originali (DEC-175)

Oltre alla pipeline SD1.5 + LoRA descritta sopra, il progetto produce **sprite originali
disegnati direttamente in Aseprite**: HUD, personaggio, nemici, boss, oggetti, colpi,
prop. Questi sprite sono **allo stesso tempo asset di gioco e dataset definitivo delle
LoRA** (DEC-148/168, dettaglio della struttura per famiglia in `03-PIANO-LORA.md`,
sezione "Struttura del dataset definitivo per famiglie"): non una produzione a parte, non
un pre-training-set da rigenerare poi con SD.

Toolchain (compilata e verificata il 27/07):

- **binario Aseprite locale** (build personale, EULA rispettata: nessuna ridistribuzione
  del binario);
- **server MCP** registrato in `.mcp.json` del repository (comando Python, invoca il
  binario locale via `ASEPRITE_PATH`).

Tutte le animazioni prodotte con Aseprite seguono lo stesso **contratto spritesheet a
formato fisso** dello `SpriteBundle` definito sotto (`## Formato SpriteBundle`): canvas,
pivot, hitbox, socket e clip di animazione con lo stesso schema JSON, indipendentemente
dal fatto che i frame vengano generati da SD1.5+LoRA o disegnati a mano in Aseprite. Il
motore raylib non distingue le due provenienze: consuma lo stesso `SpriteBundle` in
entrambi i casi.

Per il dataset: **un file caption per immagine** (stesso nome base, estensione `.txt`),
registrazione nel ledger (`docs/ai-production/dataset/ledger.jsonl`) come **`own`**
(sprite proprio, non CC0), organizzati per famiglia in `dataset/worldsmelt-style/`.

### Contratto concreto della demo (prodotto in `assets/art/`, consumato da W8)

Il profilo demo del contratto, emesso dagli script batch `scripts/cp*_*.lua` e già
committato per personaggio, nemici, colpi, oggetti, prop e UI:

- `assets/art/<categoria>/<id>.png`: striscia orizzontale a frame di taglia fissa,
  **una riga per animazione**; accanto `<id>.json`:
  `{"frame_w":N,"frame_h":N,"anchor":[x,y],"anims":{"walk":{"row":0,"frames":4,"fps":8,"loop":true},...}}`.
- Il personaggio usa le righe `walk_down/walk_up/walk_right/walk_left` (left
  pre-specchiata); i nemici da contatto aggiungono la riga **`attack`** (opzionale:
  la mappa `anims` è aperta, il motore ignora le anim che non conosce).
- Le **taglie** restano del motore (`sizeMul`/`radiusMul`): gli sprite esistono in
  **tier disegnati** (32px base, 48px «grande», 64+ boss; colpi 16px base, 24px
  «grande») e il motore sceglie il tier più vicino, ritoccando la hitbox a parte.

**Estensioni UI** (`assets/art/ui/`): `panel-9patch`/`slot-9patch` aggiungono al json
il campo `"slice":[l,t,r,b]` (bordi 9-patch in pixel); `font-5px.json` è una mappa
glifi `{"glyph_h":5,"baseline_y":1,"space_w":3,"letter_spacing":1,"glyphs":{"A":{"x":0,"w":3},...}}`
sul PNG a striscia; `icons.png` usa una riga-anim per icona (1 frame ciascuna).
`font-5px.json` accetta anche la chiave opzionale `"glyphs_ext"` (WP-INT), STESSA
profondità/struttura di `"glyphs"` ma con la chiave codepoint Unicode in base 10
invece del carattere ASCII singolo — serve ai caratteri che non stanno in un `char`
(oggi le sei maiuscole accentate italiane più comuni: À È É Ì Ò Ù, tutte fuori
dall'intervallo ASCII):
`{"glyphs_ext":{"192":{"x":200,"w":3},"200":{"x":204,"w":3},"201":{"x":208,"w":3},"204":{"x":212,"w":3},"210":{"x":216,"w":3},"217":{"x":220,"w":3}}}`.
`x`/`w` hanno lo stesso significato di un glifo normale (ascissa nella striscia,
larghezza in pixel); il ritaglio verticale resta `baseline_y`/`glyph_h`, condiviso
con tutto il resto dello sheet — nessun campo nuovo oltre a `x`/`w`. Un manifest
senza `"glyphs_ext"` resta valido (la chiave è opzionale, ricade sul set ASCII di
sempre).
`baseline_y` è la riga del PNG da cui i glifi cominciano (la striscia è alta
`glyph_h+2`, con una riga di guardia sopra): era già nel file reale ma mancava da questo
elenco — aggiunta a W8, dopo che il loader del motore ha dovuto leggerla per non disegnare
i glifi spostati di un pixel.

**Consumo lato motore** (W8): lo legge `src/assets/art_atlas.{h,c}` (`ArtAtlas*`) con uno
scanner sequenziale minimale — il binario del gioco non linka cJSON (AGENTS.md) — e lo
disegna `src/render/art_draw.{h,c}` (`ArtDraw*`). Due garanzie da rispettare quando questo
contratto si estende: (1) **ogni chiave sconosciuta viene saltata**, quindi una chiave nuova
non fa sparire lo sprite sui motori più vecchi (verificato da `--art-atlas-test`); (2) il
manifest deve restare **ASCII, senza escape, profondo due livelli** — è la condizione che
rende lecito uno scanner invece di una libreria. I nomi delle animazioni per famiglia e i
ruoli del tileset elencati qui sono quelli che il renderer cerca per nome: rinominarne uno
lo fa ricadere sul percorso precedente in silenzio (primitiva geometrica o colore piatto),
mai un errore visibile. Buchi di asset noti: `docs/engineering/known-issues.md` voce 10.

**Tileset ambiente** (`assets/art/tiles/<tema>.png` + `.json`, temi fallback della
demo): manifest `{"tile_w":32,"tile_h":32,"grid":[cols,rows],"tiles":{"<ruolo>":[col,riga]}}`.
Ruoli emessi per ogni tema: `floor`, `floor_var1..3`, `wall_n/e/s/w`,
`corner_nw/ne/se/sw` (esterni), `inner_nw/ne/se/sw` (interni), `l_block` (angolo
mancante della forma a L), `door_{n,e,s,w}_{aperta,chiusa,bloccata}`,
`obst_{pillar,corridor,arena,scatter}` (famiglie `ROOM_LAYOUT_*`), `void`, e la
variante di escalation DEC-024 `floor_deg`/`wall_deg`/`void_deg` (crepe di brace).

## Personaggi umanoidi

Generare:

- down/front;
- up/back;
- right/side;
- left tramite mirroring.

Clip iniziali:

- idle: 2–4 frame;
- walk: 4–6;
- attack: 3–5;
- hurt: effetto procedurale o 1–2;
- death: 4–6 o dissoluzione procedurale.

Non chiedere un intero foglio in una singola generazione. Usare una reference canonica e
generare ogni frame con guida strutturale.

## Guide

Ordine di complessità:

1. silhouette disegnata;
2. sprite grezzo;
3. edge/Canny;
4. pose map;
5. ControlNet;
6. ControlNet personalizzato soltanto se necessario.

## Pixel Art Fixer

Il codice MIT può essere inserito nella pipeline di build:

```text
output SD
-> rilevamento griglia
-> ricostruzione 1×
-> alpha
-> palette Worldsmelt
-> nearest-neighbor verso la dimensione di visualizzazione
```

La versione classica non corregge anatomia o coerenza temporale. La versione neurale del
sito è separata.

## Validazione automatica

Per ogni frame:

- dimensione;
- alpha;
- pixel opachi;
- nessun pixel key-risk;
- margine;
- bounding box;
- pivot;
- palette massima;
- differenza di area rispetto al frame precedente;
- centro di massa;
- socket dentro la silhouette;
- nessun elemento scollegato troppo piccolo.

Per una clip:

- drift del pivot;
- variazione di scala;
- palette comune;
- identità;
- loop;
- frame duplicati;
- telegraph prima dell'evento.

## Eventi di animazione

Le clip devono contenere eventi, non solo frame:

```json
{
  "attack": {
    "fps": 12,
    "loop": false,
    "frames": [0, 1, 2, 3],
    "events": {
      "1": ["telegraph"],
      "2": ["fire", "play_sound"],
      "3": ["recover"]
    }
  }
}
```

Il danno non deve dipendere dall'immagine: il motore emette l'evento.

## Formato SpriteBundle

```json
{
  "version": 1,
  "id": "marrow_oracle",
  "rig": "tentacle_chain_v1",
  "textures": {
    "body": "body.png",
    "segment": "segment.png",
    "tip": "tip.png"
  },
  "canvas": [128, 128],
  "pivot": [64, 102],
  "hitbox": [42, 62, 44, 30],
  "sockets": {
    "shot_origin": [64, 42]
  },
  "animations": {
    "idle": {
      "mode": "procedural",
      "profile": "float_soft"
    }
  }
}
```

## Raylib

Il runtime deve avere:

```c
typedef struct SpriteClip {
    int firstFrame;
    int frameCount;
    float fps;
    bool loop;
} SpriteClip;

typedef struct SpriteAnimator {
    int state;
    int frame;
    float elapsed;
    int facing;
    bool finished;
} SpriteAnimator;
```

Per i bundle modulari, `firstFrame` può essere sostituito da un array di regioni o texture.

## Player Worldsmelt

Strategia raccomandata:

- mantenere il rig/socket fisso nella vertical slice;
- applicare skin e componenti pixel-art;
- mantenere oggetti come layer;
- usare bob, recoil, squash/stretch e particelle;
- rimandare il personaggio interamente generato finché non esiste un sistema di socket
  validato.
