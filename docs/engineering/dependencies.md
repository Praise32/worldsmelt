---
id: eng-dependencies
title: Dipendenze vendorizzate e riferimenti locali
domain: engineering
status: implemented
authority: supporting
owner: engineering
summary: >-
  Librerie native in deps/ (versioni pinnate da scripts/setup-deps.sh) con
  ruolo e binario che le linka, piu' i repository locali di sola consultazione
  e i modelli GGUF/SD scaricati da scripts/download-models.sh.
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [dipendenze, deps, raylib, llama.cpp, lua, stable-diffusion.cpp, raygui, modelli]
related: [eng-architecture]
supersedes: []
source_files: [scripts/setup-deps.sh, scripts/download-models.sh, Makefile, src/render/raygui_impl.c]
---

# Dipendenze vendorizzate e riferimenti locali

Tutte le dipendenze native vivono in `deps/` (ignorata da git, ricreata da
`scripts/setup-deps.sh`, idempotente). Nessuna e' installata come pacchetto di
sistema oltre alle librerie apt elencate in cima allo script (toolchain,
X11/Wayland, Vulkan). Le versioni sotto sono quelle **pinnate nello script**,
non "ultima disponibile".

## Le quattro dipendenze in deps/

| Libreria | Versione pinnata | Come arriva | Build |
|---|---|---|---|
| raylib | tag `6.0` | `git clone --depth 1 --branch 6.0` | CMake statico, `PLATFORM=Desktop`, GLFW con X11+Wayland |
| llama.cpp | tag `b9979` | `git clone --depth 1 --branch b9979` | CMake statico, `GGML_VULKAN=ON`, niente server/tool/esempi; include anche `build/bin/test-gbnf-validator` (target esplicito, non in `all`) |
| stable-diffusion.cpp | tag `master-775-b5d8120` | `git clone --recursive --depth 1` (vendorizza il proprio ggml come sottomodulo) | CMake statico, `SD_VULKAN=ON` |
| Lua | `5.5.0` | tarball da lua.org, verificato via SHA256 fissato nello script | `make linux` (Makefile ufficiale Lua, target che NON abilita readline) |

Nota sull'integrita': i tag git (raylib/llama.cpp/stable-diffusion.cpp) si
fidano del canale HTTPS + repository GitHub ufficiale; solo il tarball Lua ha
un hash SHA256 verificato esplicitamente nello script, perche' lua.org non
pubblica un file di checksum affiancato.

### Chi linka cosa (Makefile)

Tre eseguibili, mai piu' di una libreria di inferenza/immagine per binario:

- **`bin/melting_run_gpu`** (il gioco, target `game`): `GAME_LIBS` = raylib
  statica + Lua statica + `-lGL -lm -lpthread -ldl -lrt -lX11`. **Mai**
  llama.cpp, stable-diffusion.cpp o cJSON (Makefile, commento sopra
  `LUA_DIR`; verificato in `GAME_LIBS`, riga 16).
- **`bin/melting-gen`** (`tools/melting-gen/`, target `gen`): `GEN_LIBS` =
  `libllama.a` + le librerie ggml del fork di llama.cpp (`libggml.a`,
  `libggml-vulkan.a`, `libggml-cpu.a`, `libggml-base.a`) + Lua statica (per
  compilare/validare gli script Lua generati, non per eseguire il gioco) +
  `-lvulkan -lgomp -lstdc++ -lpthread -lm -ldl`. cJSON e' **vendorizzato nei
  sorgenti** (`tools/melting-gen/vendor/cJSON.{c,h}`, v1.7.19 MIT, incluso via
  `wildcard` in `GEN_SRC`), non e' una libreria esterna in `deps/`.
- **`bin/melting-sprites`** (`tools/melting-sprites/`, target `sprites`):
  `SPRITES_LIBS` = `libstable-diffusion.a` + le librerie ggml del fork
  **leejet/ggml** vendorizzato da stable-diffusion.cpp (stessi nomi di file
  di llama.cpp ma binari diversi) + `-lvulkan -lgomp -lstdc++ -lpthread -lm
  -ldl`. Non linka raylib ne' llama.cpp.

**Perche' due eseguibili separati per l'inferenza** (`melting-gen` e
`melting-sprites`) invece di uno solo: llama.cpp e stable-diffusion.cpp
vendorizzano **due fork incompatibili di ggml** (quello di
`ggml-org/llama.cpp` e quello di `leejet/ggml` usato da stable-diffusion.cpp).
Linkarli nello stesso binario produrrebbe simboli duplicati: la separazione in
due processi esterni, mai avviati insieme, e' l'unica soluzione con questo
vincolo (commento in cima al blocco `SPRITES_*` del Makefile e in
`tools/melting-sprites/sprite_sd.c`).

### Raygui: caso a parte

`deps/raygui/raygui.h` (versione 4.5.0, libreria header-only) e' presente in
`deps/` ma **non e' clonata da `scripts/setup-deps.sh`**: lo script non la
menziona affatto, e' stata copiata a mano nel repository locale. E' pero'
gia' integrata nel gioco, non solo una candidatura futura:
`src/render/raygui_impl.c` e' l'unica unita' di traduzione che definisce
`RAYGUI_IMPLEMENTATION` (macro da definire in un solo `.c`, altrimenti i
simboli si duplicano al link); tutti gli altri file la includono senza quella
macro. Il Makefile aggiunge `-Ideps/raygui` a `GAME_CFLAGS`. Chi ricrea
`deps/` da zero seguendo solo `scripts/setup-deps.sh` **non** ottiene
`deps/raygui/`: va copiato a mano (o lo script va esteso) prima di compilare
il gioco.

## Repository locali di sola consultazione (non dipendenze di build)

Non sono in `deps/`, non vengono compilati ne' linkati; servono da
riferimento leggibile durante lo sviluppo:

- **Raygui** (sorgente upstream, `https://github.com/raysan5/raygui`) — la
  stessa libreria vendorizzata sopra; il repository upstream resta utile per
  consultare esempi/documentazione dei widget non presenti nel solo header.
- **IsaacDocs** (`https://wofsauge.github.io/IsaacDocs/rep/`) — riferimento
  concettuale per callback e pattern della sandbox Lua degli oggetti. Non e'
  codice da copiare ne' una specifica del nostro motore: il modello dati e le
  API Lua reali sono quelle di `tools/melting-gen/prompts/` e della sandbox in
  `src/`.
- **Generazione dungeon di Isaac** (articolo di BorisTheBrave) — algoritmi da
  reinterpretare per il modello dati di `src/world`, non da portare 1:1 (vedi
  anche il vincolo DEC-009 su stanze di taglia non uniforme, in conflitto con
  alcune proposte di riferimenti esterni — `docs/_meta/DOC-CONFLICTS.md`,
  DOC-CONFLICT-001).
- **gguf-tools** (`https://github.com/antirez/gguf-tools`) — solo ispezione e
  manipolazione di file GGUF (niente inferenza); utile come codice leggibile
  per capire il formato dei modelli scaricati sotto. L'inferenza vera e'
  sempre llama.cpp.

## Modelli (GGUF e Stable Diffusion), via `scripts/download-models.sh`

Scaricati in `models/` (mai committati), ognuno verificato con SHA256 fissato
nello script prima di considerarlo valido; `fetch()` salta il download se il
file e' gia' presente e l'hash combacia. Opzioni: `--light` (salta il 7B),
`--no-sprites` (salta i tre modelli Stable Diffusion, licenza diversa dai
modelli di testo).

Testo (usati da `melting-gen`, entrambi Apache 2.0):
- `qwen2.5-coder-1.5b-instruct-q4_k_m.gguf` — Qwen2.5-Coder-1.5B-Instruct-GGUF, scaricato sempre.
- `qwen2.5-coder-7b-instruct-q4_k_m.gguf` — Qwen2.5-Coder-7B-Instruct-GGUF, saltato con `--light`.

Sprite (usati da `melting-sprites`, saltati con `--no-sprites`):
- `Public-Prompts-Pixel-Model.ckpt` — PublicPrompts/All-In-One-Pixel-Model
  (CreativeML OpenRAIL-M: le immagini generate sono utilizzabili/vendibili,
  ma ridistribuire i PESI col gioco propagherebbe le restrizioni della
  licenza; l'alternativa Apache 2.0 e' SD_PixelArt_SpriteSheet_Generator,
  resa leggermente peggiore, non scaricata da questo script).
- `lcm-lora-sdv1-5.safetensors` — latent-consistency/lcm-lora-sdv1-5
  (openrail++, vedi `docs/ai-production/licenze.md`; NON Apache 2.0), salvato
  con questo nome specifico perche' stable-diffusion.cpp risolve le LoRA per
  nome file da `--lora-model-dir` (su HuggingFace il file si chiama
  `pytorch_lora_weights.safetensors`).
- `taesd.safetensors` — madebyollin/taesd (MIT), VAE approssimato veloce per
  il decoding.
