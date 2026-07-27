---
id: aiprod-piano-integrazione-c
title: Piano di integrazione nel codice C
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Piano in 8 fasi (A-H) per portare multi-LoRA, SpriteBundle, registry dei rig, animator ed EnemySpec esteso nel motore C, con migrazione graduale dell'atlas.
last_reviewed: 2026-07-27
topics: [piano-implementazione, multi-lora, sprite-bundle, rig-registry, atlas-migration]
related: []
supersedes: []
source_files: [tools/melting-sprites/melting_sprites.h, tools/melting-sprites/sprite_sd.c, tools/melting-sprites/main.c, AGENTS.md, src/gen]
---
# Piano di integrazione nel codice C

## Fase A — Multi-LoRA

File:

- `tools/melting-sprites/melting_sprites.h`
- `tools/melting-sprites/sprite_sd.c`
- `tools/melting-sprites/main.c`
- test sprite

Proposta:

```c
#define MAX_SPRITE_LORAS 8

typedef struct SpriteLoraConfig {
    const char *path;
    float multiplier;
} SpriteLoraConfig;

typedef struct SpriteSdConfig {
    const char *modelPath;
    SpriteLoraConfig loras[MAX_SPRITE_LORAS];
    int loraCount;
    const char *taesdPath;
    int steps;
    float cfg;
    const char *outDir;
} SpriteSdConfig;
```

Context:

```c
struct SpriteSdCtx {
    sd_ctx_t *ctx;
    sd_lora_t loras[MAX_SPRITE_LORAS];
    int loraCount;
    int steps;
    float cfg;
};
```

CLI ripetibile:

```bash
--lora models/lcm.safetensors:1.0
--lora models/worldsmelt-style.safetensors:0.85
--lora models/worldsmelt-enemies.safetensors:0.70
```

Compatibilità: un solo `--lora` deve continuare a funzionare.

## Fase B — Generation Recipe

Creare:

```text
generated/generation_recipe.json
```

Contiene:

- modello;
- hash;
- LoRA e pesi;
- prompt;
- seed;
- step;
- CFG;
- dimensione;
- postprocess;
- pipeline version.

Non dipendere soltanto dai log.

## Fase C — SpriteBundle

Nuovo modulo:

```text
src/assets/sprite_bundle.h
src/assets/sprite_bundle.c
src/render/sprite_animator.h
src/render/sprite_animator.c
```

Tool:

```text
tools/melting-sprites/sprite_bundle.c
tools/melting-sprites/sprite_validate.c
```

Mantenere `current_atlas.png` per fallback globale.

## Fase D — Registry dei rig

Nuovo modulo:

```text
src/render/rig_registry.h
src/render/rig_registry.c
src/render/rig_biped.c
src/render/rig_blob.c
src/render/rig_chain.c
src/render/rig_flying.c
```

I rig non leggono prompt. Ricevono dati validati.

## Fase E — EnemySpec

Estendere il manifest generato dal modello di testo attivo (oggi Gemma-3-4B-IT Q4, DEC-140)
con:

- body_plan;
- rig;
- parts;
- required_assets;
- visual tags;
- fallback.

GBNF e validatore devono conoscere enum e limiti.

## Fase F — Animator

Aggiungere a player/nemici:

- stato;
- facing;
- elapsed;
- frame;
- finished;
- eventi.

Gli eventi di attacco restano nel gameplay.

## Fase G — Loader

Il loader prova:

1. SpriteBundle specifico;
2. cella atlas legacy;
3. forma geometrica.

Mai invisibilità.

## Fase H — Scheduling

`src/gen` deve orchestrare:

```text
melting-gen
-> validate manifest
-> melting-sprites
-> validate bundles
-> atomic publish
```

Il gioco non linka Stable Diffusion o llama.cpp, coerentemente con `AGENTS.md`.

## Test

Aggiungere:

- parse multi-LoRA;
- limite adattatori;
- file mancante;
- hash recipe;
- bundle incompleto;
- pivot fuori canvas;
- socket fuori canvas;
- hitbox invalida;
- clip zero frame;
- evento fuori range;
- fallback atlas;
- fallback geometrico;
- determinismo manifest;
- nessuna inferenza in gameplay.

## Migrazione atlas

Non rimuovere subito `AtlasSprite`.

Percorso:

1. atlas legacy resta per oggetti e fallback;
2. bundle per un blob;
3. bundle per un tentacolare;
4. bundle per un bipede;
5. spostare altri nemici;
6. decidere se mantenere atlas globale per UI/pickup.
