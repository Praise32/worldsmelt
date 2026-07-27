---
id: aiprod-pipeline-sprite-animazioni
title: Pipeline sprite e animazioni
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Pipeline dalla EnemySpec allo SpriteBundle animato: guide/ControlNet, Pixel Art Fixer, validazione automatica, formato eventi/animazioni e struct raylib. Aseprite è ora anche il percorso di produzione diretta di sprite originali (HUD, personaggio, nemici, boss, oggetti, colpi, prop) a contratto spritesheet fisso, che serve insieme come asset di gioco e dataset LoRA (DEC-175).
last_reviewed: 2026-07-28
topics: [sprite-bundle, animazione, raylib, pixel-art-fixer, validazione, controlnet, aseprite, contratto-spritesheet, DEC-175]
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
