# Piano di implementazione: build Linux + LLM testuale locale

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Il gioco compila e gira su Ubuntu, e una nuova run viene generata interamente in locale da `melting-gen` (C99 + llama.cpp/Vulkan + Qwen2.5-Coder), con schermata di caricamento nel gioco e fallback garantito.

**Architecture:** Generatore come processo separato (`tools/melting-gen/`) che scrive gli stessi file del sidecar Node (`generated/current_run.{txt,json}`, `current_atlas.{bmp,json}`); il gioco (nuovo modulo `src/gen/`) lo lancia a inizio run, ne legge il progresso da file e carica il manifest a processo concluso. Output LLM vincolato da grammatica GBNF + validatore C (port delle normalizzazioni Node).

**Tech Stack:** C99, raylib 6.0 (statica, da sorgente), llama.cpp tag `b9979` (statica, `-DGGML_VULKAN=ON`), cJSON v1.7.19 (vendorato), Qwen2.5-Coder-Instruct GGUF Q4_K_M (7B default, 1.5B riserva), bash + Makefile.

**Spec:** `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`. Nota di copertura: oltre a `current_run.txt/.json` il piano porta in C anche l'atlas BMP procedurale e `current_atlas.json` (la spec chiede «parità di output» col Node, che li scrive sempre; senza atlas fresco il gioco userebbe un BMP stantio della run precedente).

## Global Constraints

- **Hardware di riferimento:** AMD RX **5600 XT 6GB** (RDNA1/gfx1010, confermata dall'utente — una vecchia nota diceva 5700 XT ed è sbagliata), Mesa RADV, Ubuntu 26.04 «resolute». Backend **Vulkan, mai ROCm; mai flash-attention**.
- **Password/root:** `scripts/setup-deps.sh` esegue `sudo apt-get install`. **Avvisare l'utente PRIMA di lanciare qualunque comando che chiede la password** (regola esplicita dell'utente). Nessun altro comando del piano richiede root.
- **Versioni fissate (verificate 2026-07-13):** raylib `6.0`, llama.cpp `b9979`, cJSON `v1.7.19`. Non aggiornare i tag senza test dedicato.
- C99, `-Wall -Wextra -O2`; prefissi modulo esistenti (`Game`, `World`, `Combat`, `ScriptVm`, `Renderer`); nuovi prefissi: `GenRunner` (src/gen), `Gen` (tools/melting-gen).
- Il binario del gioco NON linka llama.cpp/cJSON; solo `bin/melting-gen` li linka.
- Formato manifest **byte-compatibile** con `runToManifest()` di `llm/run_content.mjs` (parser: `src/content/run_content.c:134`).
- Non toccare: file `.bat`, `llm/*.mjs`, `docs/APPUNTI.md`, `docs/DESIGN_NOTES.md`, la mini-VM.
- Mai committare: `deps/`, `models/`, `generated/`, `logs/`, `bin/`.
- Commit in inglese stile conventional (`feat:`, `docs:`, ...); prosa dei documenti in italiano.
- Tutti i comandi si lanciano dalla radice della repo. I test del gioco aprono una finestra: servono sessione grafica (c'è: desktop dell'utente).

## Struttura dei file

```text
Makefile                            ← nuovo (Task 1, 2, 9)
scripts/setup-deps.sh               ← nuovo (Task 1, 2)
scripts/download-models.sh          ← nuovo (Task 3)
scripts/test-gen.sh                 ← nuovo (Task 4-6)
scripts/test-llm.sh                 ← nuovo (Task 7)
tools/melting-gen/melting_gen.h     ← nuovo: struct + API interne (Task 4)
tools/melting-gen/gen_util.c        ← rng, hsv→hex, file, progress, log (Task 4)
tools/melting-gen/gen_fallback.c    ← run deterministica con seed (Task 4)
tools/melting-gen/gen_manifest.c    ← GenRun → txt/json/atlas.json (Task 4)
tools/melting-gen/gen_atlas.c       ← atlas BMP procedurale (Task 4)
tools/melting-gen/gen_validate.c    ← cJSON → GenRun normalizzato (Task 6)
tools/melting-gen/gen_llm.c         ← inferenza llama.cpp (Task 7)
tools/melting-gen/main.c            ← CLI + orchestrazione (Task 2, 4, 6, 7)
tools/melting-gen/vendor/cJSON.{c,h}← vendorato v1.7.19 (Task 4)
tools/melting-gen/run.gbnf          ← grammatica GBNF (Task 5)
tools/melting-gen/prompts/system.txt e user.txt (Task 5)
tests/melting-gen/bad/*.json, unparseable.txt (Task 6)
tests/fake-gen.sh                   ← finto generatore per i test (Task 9)
src/gen/gen_runner.{h,c}            ← spawn/monitor processo (Task 9)
src/core/game_types.h               ← + APP_GENERATING, GenProgress (Task 9, 10)
src/tests/game_tests.{h,c}          ← + GameManifestTest, GenRunnerSelfTest (Task 4, 9)
src/app/app.c                       ← flag nuovi + stato APP_GENERATING (Task 4, 9, 10)
src/render/game_renderer.{h,c}      ← overlay di generazione (Task 10)
AGENTS.md, README.md, docs/README.md, docs/LOCAL_REFERENCES.md, docs/BENCHMARKS.md (Task 8, 11)
.gitignore                          ← + deps/ (Task 1)
```

---

### Task 1: Branch + build Linux del gioco esistente

**Files:**
- Create: `scripts/setup-deps.sh` (parte raylib), `Makefile`
- Modify: `.gitignore`

**Interfaces:**
- Produces: `make` → `bin/melting_run_gpu`; `make test` esegue i 4 test esistenti; `deps/raylib/build/raylib/libraylib.a` per il link.

- [ ] **Step 1: Crea il branch di lavoro**

```bash
cd ~/progetti/melting-run-gpu
git checkout -b linux-local-llm
```

- [ ] **Step 2: Aggiungi `deps/` a `.gitignore`**

In `.gitignore`, sotto la riga `models/` aggiungi:

```text
deps/
```

- [ ] **Step 3: Scrivi `scripts/setup-deps.sh` (per ora solo raylib)**

```bash
#!/usr/bin/env bash
# Prepara le dipendenze native in deps/ a versioni fissate. Idempotente.
set -euo pipefail
cd "$(dirname "$0")/.."

RAYLIB_TAG="6.0"

echo "== Pacchetti di sistema (chiede la password) =="
# In un terminale si usa sudo; senza TTY (es. esecuzione da agente) pkexec apre
# la finestra grafica di autenticazione.
SUDO="sudo"
[ -t 0 ] || SUDO="pkexec"
$SUDO apt-get update
$SUDO apt-get install -y build-essential cmake git \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev \
  libvulkan-dev glslc spirv-headers vulkan-tools

mkdir -p deps

if [ ! -f deps/raylib/build/raylib/libraylib.a ]; then
  echo "== raylib $RAYLIB_TAG (statica, X11+Wayland) =="
  [ -d deps/raylib ] || git clone --depth 1 --branch "$RAYLIB_TAG" https://github.com/raysan5/raylib.git deps/raylib
  cmake -S deps/raylib -B deps/raylib/build -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF -DPLATFORM=Desktop \
    -DGLFW_BUILD_WAYLAND=ON -DGLFW_BUILD_X11=ON
  cmake --build deps/raylib/build -j"$(nproc)"
fi

echo "Dipendenze pronte."
```

```bash
chmod +x scripts/setup-deps.sh
```

- [ ] **Step 4: AVVISA L'UTENTE della richiesta di password, poi esegui lo script**

Prima di lanciare, scrivi all'utente: «Sto per lanciare setup-deps.sh: apt chiederà la tua password». Poi:

```bash
scripts/setup-deps.sh
```

Expected: clone e build di raylib senza errori; `ls deps/raylib/build/raylib/libraylib.a` esiste.

- [ ] **Step 5: Scrivi il `Makefile`**

```make
CC := gcc

RAYLIB_DIR := deps/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/build/raylib/libraylib.a

CFLAGS := -std=c99 -Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
GAME_CFLAGS := $(CFLAGS) -Isrc -I$(RAYLIB_DIR)/src
GAME_LIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

GAME_SRC := $(shell find src -name '*.c')
GAME_HDR := $(shell find src -name '*.h')
GAME_BIN := bin/melting_run_gpu

.PHONY: all game run test clean

all: game

game: $(GAME_BIN)

$(GAME_BIN): $(GAME_SRC) $(GAME_HDR)
	@mkdir -p bin logs generated
	$(CC) $(GAME_CFLAGS) $(GAME_SRC) $(GAME_LIBS) -o $@

run: game
	./$(GAME_BIN)

test: game
	./$(GAME_BIN) --script-test
	./$(GAME_BIN) --portal-test
	./$(GAME_BIN) --smoke-test
	./$(GAME_BIN) --screenshot-test

clean:
	rm -rf bin
```

- [ ] **Step 6: Compila e sistema eventuali errori di piattaforma**

```bash
make
```

Expected: `bin/melting_run_gpu` prodotto. Il codice C è raylib puro senza header Windows, quindi non dovrebbero servire modifiche; se raylib 6.0 ha cambiato la firma di qualche funzione rispetto alla 5.x usata su Windows, correggi il punto d'uso consultando `deps/raylib/src/raylib.h` (NON modificare raylib).

- [ ] **Step 7: Esegui i test esistenti**

```bash
make test
```

Expected output (4 righe di esito, exit 0):
```text
Script sandbox test: ok
Portal test: ok
```
(più smoke e screenshot senza errori; lo screenshot finisce in `logs/melting-run-screen.png`).

- [ ] **Step 8: Verifica manuale rapida**

```bash
./bin/melting_run_gpu &
```
Expected: finestra fullscreen, menu «MELTING ISAAC LLM», INVIO avvia una run giocabile (contenuti fallback interni). Chiudi con Q.

- [ ] **Step 9: Commit**

```bash
git add .gitignore scripts/setup-deps.sh Makefile
git commit -m "feat: Linux build via Makefile and pinned raylib 6.0 in deps/"
```

---

### Task 2: llama.cpp in deps/ + scheletro di melting-gen (verifica del link)

**Files:**
- Modify: `scripts/setup-deps.sh`, `Makefile`
- Create: `tools/melting-gen/main.c` (provvisorio, solo `--version`)

**Interfaces:**
- Produces: `bin/melting-gen --version` stampa i backend ggml disponibili (deve includere Vulkan). Librerie statiche in `deps/llama.cpp/build/`. Target `make gen`.

- [ ] **Step 1: Aggiungi llama.cpp a `scripts/setup-deps.sh`**

Dopo il blocco raylib, prima di `echo "Dipendenze pronte."`, inserisci (i pacchetti Vulkan sono già installati dal Task 1):

```bash
LLAMA_TAG="b9979"
if [ ! -f deps/llama.cpp/build/src/libllama.a ]; then
  echo "== llama.cpp $LLAMA_TAG (statica, backend Vulkan) =="
  [ -d deps/llama.cpp ] || git clone --depth 1 --branch "$LLAMA_TAG" https://github.com/ggml-org/llama.cpp.git deps/llama.cpp
  cmake -S deps/llama.cpp -B deps/llama.cpp/build -DCMAKE_BUILD_TYPE=Release \
    -DGGML_VULKAN=ON -DBUILD_SHARED_LIBS=OFF -DLLAMA_BUILD_TESTS=ON \
    -DLLAMA_BUILD_EXAMPLES=OFF -DLLAMA_BUILD_TOOLS=OFF -DLLAMA_BUILD_APP=OFF \
    -DLLAMA_BUILD_SERVER=OFF -DLLAMA_CURL=OFF
  # -j4 e non $(nproc): con 12 job paralleli GCC 15 va in internal compiler error
  # su llama-sampler.cpp per pressione di memoria (15 GiB di RAM su questa macchina).
  cmake --build deps/llama.cpp/build -j4
  # test-gbnf-validator non fa parte del target 'all': va chiesto esplicitamente.
  cmake --build deps/llama.cpp/build --target test-gbnf-validator -j4
fi

echo "== Verifica Vulkan =="
vulkaninfo --summary | head -25 || echo "ATTENZIONE: vulkaninfo fallito, controlla i driver"
```

(`LLAMA_BUILD_TESTS=ON` serve per avere `build/bin/test-gbnf-validator`, usato dal Task 5. `LLAMA_BUILD_APP=OFF` e `LLAMA_BUILD_SERVER=OFF` sono obbligatori: b9979 ha un target «binario unificato» attivo di default che richiede la libreria `common` e non compila senza; a noi serve solo `libllama`, quindi si spegne.)

- [ ] **Step 2: AVVISA L'UTENTE (password) e riesegui lo script**

```bash
scripts/setup-deps.sh
```
Expected: build llama.cpp completata (10-20 min); esistono `deps/llama.cpp/build/src/libllama.a`, `deps/llama.cpp/build/ggml/src/libggml.a`, `.../libggml-base.a`, `.../libggml-cpu.a`, `.../ggml-vulkan/libggml-vulkan.a`, `deps/llama.cpp/build/bin/test-gbnf-validator`; `vulkaninfo --summary` mostra la Radeon RX 5600 XT (RADV NAVI10).

- [ ] **Step 3: Scrivi lo scheletro `tools/melting-gen/main.c`**

```c
#include "llama.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            llama_backend_init();
            printf("melting-gen (llama.cpp b9979)\n%s\n", llama_print_system_info());
            llama_backend_free();
            return 0;
        }
    }
    fprintf(stderr, "melting-gen: uso: --version (altre opzioni nei task successivi)\n");
    return 1;
}
```

- [ ] **Step 4: Aggiungi il target `gen` al Makefile**

Dopo il blocco `GAME_*`, aggiungi:

```make
LLAMA_DIR := deps/llama.cpp
LLAMA_BUILD := $(LLAMA_DIR)/build

GEN_SRC := $(wildcard tools/melting-gen/*.c) $(wildcard tools/melting-gen/vendor/*.c)
GEN_CFLAGS := $(CFLAGS) -Itools/melting-gen -Itools/melting-gen/vendor \
  -I$(LLAMA_DIR)/include -I$(LLAMA_DIR)/ggml/include
GEN_LIBS := $(LLAMA_BUILD)/src/libllama.a \
  $(LLAMA_BUILD)/ggml/src/libggml.a \
  $(LLAMA_BUILD)/ggml/src/ggml-vulkan/libggml-vulkan.a \
  $(LLAMA_BUILD)/ggml/src/libggml-cpu.a \
  $(LLAMA_BUILD)/ggml/src/libggml-base.a \
  -lvulkan -lgomp -lstdc++ -lpthread -lm -ldl
GEN_BIN := bin/melting-gen

gen: $(GEN_BIN)

$(GEN_BIN): $(GEN_SRC)
	@mkdir -p bin logs
	$(CC) $(GEN_CFLAGS) $(GEN_SRC) $(GEN_LIBS) -o $@
```

e cambia `all: game` in `all: game gen`, aggiungendo `gen` alla riga `.PHONY`.

Nota: se il link fallisce con simboli mancanti, verifica i `.a` reali con `find deps/llama.cpp/build -name '*.a'` — la build può produrre archivi aggiuntivi (es. BLAS) da accodare a `GEN_LIBS` nello stesso ordine (dipendenti prima).

- [ ] **Step 5: Compila e verifica il link**

```bash
make gen && ./bin/melting-gen --version
```
Expected: stampa `melting-gen (llama.cpp b9979)` seguita dalla riga di system info di ggml. Se tra i backend registrati non compare Vulkan, la build di llama.cpp non l'ha attivato: ricontrolla lo Step 1.

- [ ] **Step 6: Commit**

```bash
git add scripts/setup-deps.sh Makefile tools/melting-gen/main.c
git commit -m "feat: pinned llama.cpp b9979 Vulkan build and melting-gen link skeleton"
```

---

### Task 3: Script di download dei modelli

**Files:**
- Create: `scripts/download-models.sh`

**Interfaces:**
- Produces: `models/qwen2.5-coder-7b-instruct-q4_k_m.gguf` (4.683.073.536 byte) e `models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf` (1.117.320.768 byte), verificati SHA256. Percorsi usati come default dal Task 7.

- [ ] **Step 1: Scrivi `scripts/download-models.sh`**

```bash
#!/usr/bin/env bash
# Scarica i modelli GGUF ufficiali Qwen (Apache 2.0) con verifica SHA256.
# Uso: scripts/download-models.sh [--light]   (--light: solo il modello 1.5B)
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p models

MODEL_7B="qwen2.5-coder-7b-instruct-q4_k_m.gguf"
URL_7B="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/$MODEL_7B"
SHA_7B="509287f78cb4d4cf6b3843734733b914b2c158e43e22a7f4bf5e963800894d3c"

MODEL_15B="qwen2.5-coder-1.5b-instruct-q4_k_m.gguf"
URL_15B="https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/$MODEL_15B"
SHA_15B="cc324af070c2ecbfd324a30884d2f951a7ff756aba85cb811a6ec436933bb046"

fetch() {
  local name="$1" url="$2" sha="$3"
  if [ -f "models/$name" ] && echo "$sha  models/$name" | sha256sum -c --status; then
    echo "$name: gia' presente e verificato"
    return
  fi
  echo "Scarico $name (riprendibile con Ctrl+C e rilancio)..."
  curl -L -C - --fail -o "models/$name" "$url"
  echo "$sha  models/$name" | sha256sum -c
}

fetch "$MODEL_15B" "$URL_15B" "$SHA_15B"
if [ "${1:-}" != "--light" ]; then
  fetch "$MODEL_7B" "$URL_7B" "$SHA_7B"
fi

cat > models/README.md <<'EOF'
# Modelli locali (mai committare)

- qwen2.5-coder-7b-instruct-q4_k_m.gguf  — Qwen/Qwen2.5-Coder-7B-Instruct-GGUF  (Apache 2.0)
- qwen2.5-coder-1.5b-instruct-q4_k_m.gguf — Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF (Apache 2.0)

Scaricati e verificati da scripts/download-models.sh.
EOF
echo "Modelli pronti."
```

```bash
chmod +x scripts/download-models.sh
```

- [ ] **Step 2: Esegui il download (≈5,8 GB totali — parte lunga)**

```bash
scripts/download-models.sh
```
Expected: due righe `...: OK` da sha256sum e `Modelli pronti.`. Se la rete cade, rilanciare riprende dal punto interrotto (`curl -C -`).

- [ ] **Step 3: Commit (solo lo script)**

```bash
git add scripts/download-models.sh
git commit -m "feat: model download script with SHA256 verification"
```

---

### Task 4: melting-gen — fallback deterministico, manifest, atlas BMP

Il generatore impara a scrivere TUTTI i file di output (senza LLM): è il percorso `--fallback`, che resta per sempre la rete di sicurezza. Porta in C la logica di `llm/run_content.mjs` (`fallbackRun`, `runToManifest`, `writePlayableAtlas`).

**Files:**
- Create: `tools/melting-gen/melting_gen.h`, `gen_util.c`, `gen_fallback.c`, `gen_manifest.c`, `gen_atlas.c`, `vendor/cJSON.{c,h}`
- Modify: `tools/melting-gen/main.c`, `src/app/app.c:58-120`, `src/tests/game_tests.{h,c}`, `Makefile`
- Test: `scripts/test-gen.sh`

**Interfaces:**
- Consumes: tabelle/regole di `llm/run_content.mjs` (portate 1:1), parser manifest `src/content/run_content.c:134`.
- Produces per i task 5-10: `GenRun` (struct dati), `GenFallbackRun(GenRun*, unsigned seed)`, `GenWriteRunFiles(const GenRun*, const char *outDir)`, `GenWriteAtlasBmp(...)`, `GenWriteLlmJson(const GenRun*, const char *path)`, `GenProgressWrite(outDir, phase, percent, message)`, `GenLogLine(fmt, ...)`, `GenTraitRuleFor(trait)`, `GEN_SLOTS`, `GEN_TRAITS`, `GenReadFile(path)`. Fasi progresso (vocabolario fisso): `avvio`, `carico-modello`, `genero`, `valido`, `scrivo`, `fine`, `errore`. CLI: `melting-gen --fallback --seed N --out DIR [--emit-llm-json]`, exit 0 ok / 3 errore scrittura. Flag del gioco: `--manifest-test` (exit 0/5).

- [ ] **Step 1: Vendorizza cJSON v1.7.19**

```bash
mkdir -p tools/melting-gen/vendor
curl -L --fail -o tools/melting-gen/vendor/cJSON.c https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/cJSON.c
curl -L --fail -o tools/melting-gen/vendor/cJSON.h https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/cJSON.h
head -3 tools/melting-gen/vendor/cJSON.h
```
Expected: header con copyright MIT di Dave Gamble.

- [ ] **Step 2: Scrivi il test PRIMA — `scripts/test-gen.sh` (versione iniziale)**

```bash
#!/usr/bin/env bash
# Test di melting-gen senza modello LLM.
set -euo pipefail
cd "$(dirname "$0")/.."
GEN=bin/melting-gen
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

echo "-- determinismo fallback: stesso seed = stessi byte --"
"$GEN" --fallback --seed 12345 --out "$TMP/a"
"$GEN" --fallback --seed 12345 --out "$TMP/b"
cmp "$TMP/a/current_run.txt" "$TMP/b/current_run.txt"
cmp "$TMP/a/current_atlas.bmp" "$TMP/b/current_atlas.bmp"

echo "-- seed diverso = manifest diverso --"
"$GEN" --fallback --seed 999 --out "$TMP/c"
if cmp -s "$TMP/a/current_run.txt" "$TMP/c/current_run.txt"; then
  echo "FALLITO: seed diversi hanno prodotto lo stesso manifest"; exit 1
fi

echo "-- il manifest e' completo --"
grep -q "^floor5.item3.script=" "$TMP/a/current_run.txt"
grep -q "^atlas.path=" "$TMP/a/current_run.txt"

echo "-- il gioco carica il manifest generato --"
"$GEN" --fallback --seed 4242 --out generated
bin/melting_run_gpu --manifest-test

echo "TEST-GEN: OK"
```

```bash
chmod +x scripts/test-gen.sh
```

- [ ] **Step 3: Aggiungi `test-gen` al Makefile e verifica che fallisca**

Nel Makefile aggiungi a `.PHONY` i target `test-gen` e in fondo:

```make
test-gen: all
	bash scripts/test-gen.sh
```

```bash
make test-gen
```
Expected: FAIL — `melting-gen` non conosce ancora `--fallback` (exit 1 dallo scheletro del Task 2).

- [ ] **Step 4: Scrivi `tools/melting-gen/melting_gen.h`**

```c
#ifndef MELTING_GEN_H
#define MELTING_GEN_H

#include <stddef.h>

#define GEN_FLOORS 5
#define GEN_ITEMS 3
#define GEN_MAX_OPS 3

typedef struct GenScriptOp {
    char trigger[10];   /* "on_fire" | "on_hit" */
    char op[12];        /* "burst" | "projectile" | "area" | "heal" */
    double a;
    double b;
    char trait[10];     /* uno dei GEN_TRAITS oppure "none" */
} GenScriptOp;

typedef struct GenItem {
    char name[48];      /* stesso limite di Item.name in game_types.h */
    char slot[8];       /* uno dei GEN_SLOTS */
    char traits[2][10];
    int traitCount;     /* 1..2 */
    char color[8];      /* "#rrggbb" */
    GenScriptOp ops[GEN_MAX_OPS];
    int opCount;        /* 1..3 */
} GenItem;

typedef struct GenFloor {
    char theme[64];
    char style[48];
    char boss[64];
    char bg[8], floorColor[8], wall[8], accent[8], accent2[8], enemy[8], bossColor[8];
    GenItem items[GEN_ITEMS];
} GenFloor;

typedef struct GenRun {
    char source[96];
    unsigned int seed;
    GenFloor floors[GEN_FLOORS];
} GenRun;

typedef struct GenTraitRule {
    const char *trait;
    const char *trigger;
    const char *op;
    double a;
    double b;
} GenTraitRule;

/* gen_util.c */
unsigned int GenRngNext(unsigned int *state);
int GenRngRange(unsigned int *state, int min, int max);
void GenHsvToHex(double h, double s, double v, char out[8]);
int GenEnsureDir(const char *path);
char *GenReadFile(const char *path);   /* buffer malloc terminato da zero, NULL su errore */
void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message);
void GenLogLine(const char *fmt, ...);
extern const char *GEN_SLOTS[6];
extern const char *GEN_TRAITS[9];
const GenTraitRule *GenTraitRuleFor(const char *trait);   /* NULL se sconosciuto */

/* gen_fallback.c */
void GenFallbackRun(GenRun *run, unsigned int seed);

/* gen_manifest.c */
int GenWriteRunFiles(const GenRun *run, const char *outDir);
int GenWriteLlmJson(const GenRun *run, const char *path);

/* gen_atlas.c */
int GenWriteAtlasBmp(const GenRun *run, const char *outDir);

/* gen_validate.c (Task 6) */
struct cJSON;
void GenNormalizeRun(const struct cJSON *raw, unsigned int seed, GenRun *out);

#endif
```

- [ ] **Step 5: Scrivi `tools/melting-gen/gen_util.c`**

```c
#include "melting_gen.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

unsigned int GenRngNext(unsigned int *state)
{
    unsigned int s = *state ? *state : 0xA341316Cu;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    if (!s) s = 0xA341316Cu;
    *state = s;
    return s;
}

int GenRngRange(unsigned int *state, int min, int max)
{
    if (max <= min) return min;
    return min + (int)(GenRngNext(state) % (unsigned int)(max - min + 1));
}

void GenHsvToHex(double h, double s, double v, char out[8])
{
    h = fmod(fmod(h, 360.0) + 360.0, 360.0);
    double c = v*s;
    double m = fmod(h/60.0, 2.0) - 1.0;
    double x = c*(1.0 - (m < 0 ? -m : m));
    double base = v - c;
    double r = 0, g = 0, b = 0;
    if (h < 60)       { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else              { r = c; b = x; }
    snprintf(out, 8, "#%02x%02x%02x",
             (int)lround((r + base)*255.0),
             (int)lround((g + base)*255.0),
             (int)lround((b + base)*255.0));
}

int GenEnsureDir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

char *GenReadFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message)
{
    char tmp[512], fin[512];
    snprintf(fin, sizeof(fin), "%s/gen_progress.txt", outDir);
    snprintf(tmp, sizeof(tmp), "%s/gen_progress.tmp", outDir);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "%s|%d|%s\n", phase, percent, message);
    fclose(f);
    rename(tmp, fin);
}

void GenLogLine(const char *fmt, ...)
{
    GenEnsureDir("logs");
    FILE *f = fopen("logs/melting-gen.log", "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s] ", stamp);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}

/* Tabelle condivise: stesse regole di TRAIT_SCRIPT_RULES in llm/run_content.mjs. */
const char *GEN_SLOTS[6] = { "hat", "eyes", "hand", "back", "body", "aura" };
const char *GEN_TRAITS[9] = {
    "bounce", "homing", "explode", "split", "pierce", "rapid", "giant", "slow", "vamp"
};

static const GenTraitRule GEN_TRAIT_RULES[9] = {
    { "bounce",  "on_fire", "burst",       2, 0.25 },
    { "homing",  "on_hit",  "projectile",  2, 260  },
    { "explode", "on_hit",  "area",       58, 0.48 },
    { "split",   "on_fire", "burst",       3, 0.36 },
    { "pierce",  "on_hit",  "projectile",  1, 420  },
    { "rapid",   "on_fire", "burst",       2, 0.16 },
    { "giant",   "on_hit",  "area",       44, 0.34 },
    { "slow",    "on_hit",  "area",       54, 0.22 },
    { "vamp",    "on_hit",  "heal",       18, 1    },
};

const GenTraitRule *GenTraitRuleFor(const char *trait)
{
    for (int i = 0; i < 9; i++)
    {
        if (trait && strcmp(GEN_TRAIT_RULES[i].trait, trait) == 0) return &GEN_TRAIT_RULES[i];
    }
    return NULL;
}
```

- [ ] **Step 6: Scrivi `tools/melting-gen/gen_fallback.c`** (port di `fallbackRun`, parole italiane identiche al Node)

```c
#include "melting_gen.h"

#include <stdio.h>
#include <string.h>

static void FallbackScriptForTrait(const char *trait, unsigned int *rng, GenItem *item)
{
    GenScriptOp *op = &item->ops[0];
    item->opCount = 1;
    const GenTraitRule *rule = GenTraitRuleFor(trait);
    if (rule)
    {
        snprintf(op->trigger, sizeof(op->trigger), "%s", rule->trigger);
        snprintf(op->op, sizeof(op->op), "%s", rule->op);
        op->a = rule->a;
        op->b = rule->b;
        snprintf(op->trait, sizeof(op->trait), "%s", rule->trait);
        return;
    }
    snprintf(op->trigger, sizeof(op->trigger), "%s", GenRngRange(rng, 0, 1) ? "on_hit" : "on_fire");
    snprintf(op->op, sizeof(op->op), "projectile");
    op->a = 1;
    op->b = 300;
    snprintf(op->trait, sizeof(op->trait), "none");
}

void GenFallbackRun(GenRun *run, unsigned int seed)
{
    static const char *themeWords[] = { "Cantina", "Biblioteca", "Acquario", "Fucina", "Cattedrale", "Laboratorio", "Teatro" };
    static const char *weirdWords[] = { "Neon", "Muffita", "Lunare", "Radioattiva", "di Zucchero", "Elettrica", "di Carta" };
    static const char *styles[]     = { "pixel semplice", "toon scuro", "arcade secco", "inchiostro piatto", "low-fi fantasy" };
    static const char *itemNames[]  = { "Corona", "Occhiali", "Guanto", "Mantello", "Medaglia", "Cappello", "Aureola", "Spada" };

    memset(run, 0, sizeof(*run));
    unsigned int rng = seed ? seed : 0xA341316Cu;
    snprintf(run->source, sizeof(run->source), "fallback");
    run->seed = seed;

    for (int f = 0; f < GEN_FLOORS; f++)
    {
        GenFloor *floor = &run->floors[f];
        int h = GenRngRange(&rng, 0, 359);
        snprintf(floor->theme, sizeof(floor->theme), "%s %s",
                 themeWords[GenRngRange(&rng, 0, 6)], weirdWords[GenRngRange(&rng, 0, 6)]);
        snprintf(floor->style, sizeof(floor->style), "%s", styles[GenRngRange(&rng, 0, 4)]);
        if (f == GEN_FLOORS - 1) snprintf(floor->boss, sizeof(floor->boss), "Ultimo Custode");
        else snprintf(floor->boss, sizeof(floor->boss), "Custode %d", f + 1);
        GenHsvToHex(h, 0.32, 0.12, floor->bg);
        GenHsvToHex((h + 20)%360, 0.38, 0.22, floor->floorColor);
        GenHsvToHex((h + 52)%360, 0.55, 0.45, floor->wall);
        GenHsvToHex((h + 100)%360, 0.62, 0.86, floor->accent);
        GenHsvToHex((h + 172)%360, 0.70, 0.94, floor->accent2);
        GenHsvToHex((h + 235)%360, 0.58, 0.82, floor->enemy);
        GenHsvToHex((h + 300)%360, 0.75, 0.88, floor->bossColor);

        for (int j = 0; j < GEN_ITEMS; j++)
        {
            GenItem *item = &floor->items[j];
            const char *trait = GEN_TRAITS[GenRngRange(&rng, 0, 8)];
            snprintf(item->name, sizeof(item->name), "%s %s", itemNames[GenRngRange(&rng, 0, 7)], trait);
            snprintf(item->slot, sizeof(item->slot), "%s", GEN_SLOTS[GenRngRange(&rng, 0, 5)]);
            snprintf(item->traits[0], sizeof(item->traits[0]), "%s", trait);
            item->traitCount = 1;
            GenHsvToHex((h + 80 + j*53)%360, 0.75, 0.92, item->color);
            FallbackScriptForTrait(trait, &rng, item);
        }
    }
}
```

- [ ] **Step 7: Scrivi `tools/melting-gen/gen_manifest.c`** (manifest byte-compatibile con `runToManifest`, più JSON)

```c
#include "melting_gen.h"

#include "cJSON.h"

#include <stdio.h>
#include <string.h>

static void ScriptToText(const GenItem *item, char *out, size_t outSize)
{
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < item->opCount; i++)
    {
        const GenScriptOp *op = &item->ops[i];
        int n = snprintf(out + used, outSize - used, "%s%s:%s,%g,%g,%s",
                         i > 0 ? "|" : "", op->trigger, op->op, op->a, op->b, op->trait);
        if (n < 0 || (size_t)n >= outSize - used) break;
        used += (size_t)n;
    }
}

static void TraitsToText(const GenItem *item, char *out, size_t outSize)
{
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < item->traitCount; i++)
    {
        int n = snprintf(out + used, outSize - used, "%s%s", i > 0 ? "," : "", item->traits[i]);
        if (n < 0 || (size_t)n >= outSize - used) break;
        used += (size_t)n;
    }
}

static int WriteManifest(const GenRun *run, const char *outDir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/current_run.txt", outDir);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "# Generated by melting-gen\n");
    fprintf(f, "source=%s\n", run->source);
    fprintf(f, "seed=%u\n", run->seed);
    fprintf(f, "atlas.path=%s/current_atlas.bmp\n", outDir);
    for (int fl = 0; fl < GEN_FLOORS; fl++)
    {
        const GenFloor *floor = &run->floors[fl];
        int n = fl + 1;
        fprintf(f, "floor%d.theme=%s\n", n, floor->theme);
        fprintf(f, "floor%d.style=%s\n", n, floor->style);
        fprintf(f, "floor%d.boss=%s\n", n, floor->boss);
        fprintf(f, "floor%d.bg=%s\n", n, floor->bg);
        fprintf(f, "floor%d.floor=%s\n", n, floor->floorColor);
        fprintf(f, "floor%d.wall=%s\n", n, floor->wall);
        fprintf(f, "floor%d.accent=%s\n", n, floor->accent);
        fprintf(f, "floor%d.accent2=%s\n", n, floor->accent2);
        fprintf(f, "floor%d.enemy=%s\n", n, floor->enemy);
        fprintf(f, "floor%d.bossColor=%s\n", n, floor->bossColor);
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const GenItem *item = &floor->items[i];
            char text[256];
            fprintf(f, "floor%d.item%d.name=%s\n", n, i + 1, item->name);
            fprintf(f, "floor%d.item%d.slot=%s\n", n, i + 1, item->slot);
            TraitsToText(item, text, sizeof(text));
            fprintf(f, "floor%d.item%d.traits=%s\n", n, i + 1, text);
            fprintf(f, "floor%d.item%d.color=%s\n", n, i + 1, item->color);
            ScriptToText(item, text, sizeof(text));
            fprintf(f, "floor%d.item%d.script=%s\n", n, i + 1, text);
        }
    }
    fclose(f);
    return 0;
}

/* Ordine di inserimento = ordine chiavi di run.gbnf: la coppia writer/grammatica
   viene verificata da test-gen (Task 5). */
static cJSON *RunToJson(const GenRun *run)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *floors = cJSON_AddArrayToObject(root, "floors");
    for (int fl = 0; fl < GEN_FLOORS; fl++)
    {
        const GenFloor *floor = &run->floors[fl];
        cJSON *jf = cJSON_CreateObject();
        cJSON_AddStringToObject(jf, "theme", floor->theme);
        cJSON_AddStringToObject(jf, "style", floor->style);
        cJSON_AddStringToObject(jf, "boss", floor->boss);
        cJSON_AddStringToObject(jf, "bg", floor->bg);
        cJSON_AddStringToObject(jf, "floor", floor->floorColor);
        cJSON_AddStringToObject(jf, "wall", floor->wall);
        cJSON_AddStringToObject(jf, "accent", floor->accent);
        cJSON_AddStringToObject(jf, "accent2", floor->accent2);
        cJSON_AddStringToObject(jf, "enemy", floor->enemy);
        cJSON_AddStringToObject(jf, "bossColor", floor->bossColor);
        cJSON *items = cJSON_AddArrayToObject(jf, "items");
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const GenItem *item = &floor->items[i];
            cJSON *ji = cJSON_CreateObject();
            cJSON_AddStringToObject(ji, "name", item->name);
            cJSON_AddStringToObject(ji, "slot", item->slot);
            cJSON *traits = cJSON_AddArrayToObject(ji, "traits");
            for (int t = 0; t < item->traitCount; t++)
            {
                cJSON_AddItemToArray(traits, cJSON_CreateString(item->traits[t]));
            }
            cJSON_AddStringToObject(ji, "color", item->color);
            cJSON *script = cJSON_AddArrayToObject(ji, "script");
            for (int s = 0; s < item->opCount; s++)
            {
                const GenScriptOp *op = &item->ops[s];
                cJSON *jo = cJSON_CreateObject();
                cJSON_AddStringToObject(jo, "trigger", op->trigger);
                cJSON_AddStringToObject(jo, "op", op->op);
                cJSON_AddNumberToObject(jo, "a", op->a);
                cJSON_AddNumberToObject(jo, "b", op->b);
                cJSON_AddStringToObject(jo, "trait", op->trait);
                cJSON_AddItemToArray(script, jo);
            }
            cJSON_AddItemToArray(items, ji);
        }
        cJSON_AddItemToArray(floors, jf);
    }
    return root;
}

int GenWriteLlmJson(const GenRun *run, const char *path)
{
    cJSON *root = RunToJson(run);
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!text) return -1;
    FILE *f = fopen(path, "w");
    if (!f) { cJSON_free(text); return -1; }
    fputs(text, f);
    fclose(f);
    cJSON_free(text);
    return 0;
}

int GenWriteRunFiles(const GenRun *run, const char *outDir)
{
    if (GenEnsureDir(outDir) != 0) return -1;
    if (WriteManifest(run, outDir) != 0) return -1;

    char atlasPath[300];
    snprintf(atlasPath, sizeof(atlasPath), "%s/current_atlas.bmp", outDir);

    cJSON *root = RunToJson(run);
    cJSON_AddStringToObject(root, "source", run->source);
    cJSON_AddNumberToObject(root, "seed", run->seed);
    cJSON *atlas = cJSON_AddObjectToObject(root, "atlas");
    cJSON_AddStringToObject(atlas, "path", atlasPath);
    cJSON_AddNumberToObject(atlas, "cellSize", 128);
    cJSON_AddNumberToObject(atlas, "columns", 8);
    char *text = cJSON_Print(root);
    cJSON_Delete(root);
    if (!text) return -1;
    char path[512];
    snprintf(path, sizeof(path), "%s/current_run.json", outDir);
    FILE *f = fopen(path, "w");
    if (!f) { cJSON_free(text); return -1; }
    fprintf(f, "%s\n", text);
    fclose(f);
    cJSON_free(text);

    snprintf(path, sizeof(path), "%s/current_atlas.json", outDir);
    f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f,
        "{\n"
        "  \"path\": \"%s\",\n"
        "  \"width\": 1024,\n  \"height\": 1024,\n"
        "  \"cellSize\": 128,\n  \"columns\": 8,\n  \"rows\": 8,\n"
        "  \"sprites\": {\n"
        "    \"player\": [0, 0], \"enemy_chaser\": [1, 0], \"enemy_shooter\": [2, 0],\n"
        "    \"enemy_tank\": [3, 0], \"boss\": [4, 0], \"item\": [5, 0], \"heart\": [6, 0],\n"
        "    \"coin\": [7, 0], \"bomb\": [0, 1], \"key\": [1, 1], \"exit\": [2, 1], \"shot\": [3, 1]\n"
        "  }\n"
        "}\n",
        atlasPath);
    fclose(f);
    return 0;
}
```

- [ ] **Step 8: Scrivi `tools/melting-gen/gen_atlas.c`** (port 1:1 di `writePlayableAtlas` + helper di disegno di `llm/run_content.mjs:499-683`)

```c
#include "melting_gen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATLAS_W 1024

typedef struct Rgb { int r, g, b; } Rgb;

static Rgb HexToRgb(const char *hex)
{
    Rgb c = { 255, 255, 255 };
    if (hex && hex[0] == '#' && strlen(hex) >= 7)
    {
        unsigned int v = (unsigned int)strtoul(hex + 1, NULL, 16);
        c.r = (int)((v >> 16) & 0xFFu);
        c.g = (int)((v >> 8) & 0xFFu);
        c.b = (int)(v & 0xFFu);
    }
    return c;
}

static int ClampByte(double v) { return v < 0 ? 0 : (v > 255 ? 255 : (int)lround(v)); }

static Rgb Shade(Rgb c, double factor)
{
    return (Rgb){ ClampByte(c.r*factor), ClampByte(c.g*factor), ClampByte(c.b*factor) };
}

static void PutPixel(unsigned char *px, int x, int y, Rgb c)
{
    if (x < 0 || y < 0 || x >= ATLAS_W || y >= ATLAS_W) return;
    size_t idx = ((size_t)y*ATLAS_W + (size_t)x)*4;
    px[idx + 0] = (unsigned char)c.b;   /* BMP: ordine BGRA */
    px[idx + 1] = (unsigned char)c.g;
    px[idx + 2] = (unsigned char)c.r;
    px[idx + 3] = 255;
}

static void FillRect(unsigned char *px, int x, int y, int w, int h, Rgb c)
{
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++) PutPixel(px, xx, yy, c);
}

static void FillCircle(unsigned char *px, int cx, int cy, int radius, Rgb c)
{
    int r2 = radius*radius;
    for (int y = cy - radius; y <= cy + radius; y++)
        for (int x = cx - radius; x <= cx + radius; x++)
        {
            int dx = x - cx, dy = y - cy;
            if (dx*dx + dy*dy <= r2) PutPixel(px, x, y, c);
        }
}

static void DrawLineT(unsigned char *px, int x0, int y0, int x1, int y1, int thickness, Rgb c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int steps = dx > dy ? dx : dy;
    if (steps < 1) steps = 1;
    for (int i = 0; i <= steps; i++)
    {
        double t = (double)i/(double)steps;
        FillCircle(px, (int)lround(x0 + (x1 - x0)*t), (int)lround(y0 + (y1 - y0)*t), thickness, c);
    }
}

static void FillDiamond(unsigned char *px, int cx, int cy, int radius, Rgb c)
{
    for (int y = -radius; y <= radius; y++)
    {
        int span = radius - (y < 0 ? -y : y);
        FillRect(px, cx - span, cy + y, span*2 + 1, 1, c);
    }
}

static void DrawCell(unsigned char *px, int cell, const GenRun *run)
{
    int x = (cell%8)*128;
    int y = (cell/8)*128;
    int cx = x + 64;
    int cy = y + 64;
    const GenFloor *floor = &run->floors[cell%GEN_FLOORS];
    Rgb accent = HexToRgb(floor->accent);
    Rgb accent2 = HexToRgb(floor->accent2);
    Rgb enemy = HexToRgb(floor->enemy);
    Rgb boss = HexToRgb(floor->bossColor);
    Rgb wall = HexToRgb(floor->wall);
    Rgb dark = { 7, 9, 12 };
    Rgb line = Shade(wall, 1.35);

    if (cell == 0)
    {
        FillCircle(px, cx, cy - 28, 14, accent2);
        FillRect(px, cx - 5, cy - 14, 10, 42, accent2);
        DrawLineT(px, cx - 25, cy - 2, cx + 25, cy - 2, 3, accent2);
        DrawLineT(px, cx - 5, cy + 27, cx - 20, cy + 54, 4, accent2);
        DrawLineT(px, cx + 5, cy + 27, cx + 20, cy + 54, 4, accent2);
        FillCircle(px, cx - 5, cy - 30, 3, dark);
        FillCircle(px, cx + 6, cy - 30, 3, dark);
    }
    else if (cell >= 1 && cell <= 4)
    {
        Rgb body = cell == 4 ? boss : enemy;
        int r = cell == 4 ? 42 : (cell == 3 ? 32 : 27);
        FillCircle(px, cx, cy, r, body);
        if (cell == 2)
        {
            FillRect(px, cx + 10, cy - 7, 38, 14, body);
            FillCircle(px, cx + 47, cy, 8, accent);
        }
        if (cell == 3)
        {
            FillRect(px, cx - 34, cy + 21, 68, 12, Shade(body, 0.7));
            FillRect(px, cx - 38, cy - 5, 12, 26, Shade(body, 0.8));
            FillRect(px, cx + 26, cy - 5, 12, 26, Shade(body, 0.8));
        }
        if (cell == 4)
        {
            for (int i = 0; i < 6; i++)
            {
                double a = 3.14159265358979*2.0*i/6.0;
                DrawLineT(px, cx, cy, cx + (int)lround(cos(a)*55), cy + (int)lround(sin(a)*55), 5, Shade(body, 0.8));
            }
            FillCircle(px, cx, cy, 17, accent);
        }
        FillCircle(px, cx - 12, cy - 8, 5, dark);
        FillCircle(px, cx + 12, cy - 8, 5, dark);
    }
    else if (cell == 5)
    {
        FillDiamond(px, cx, cy, 36, accent);
        FillDiamond(px, cx, cy, 21, accent2);
        FillCircle(px, cx, cy, 8, dark);
    }
    else if (cell == 6)
    {
        FillCircle(px, cx - 14, cy - 8, 19, boss);
        FillCircle(px, cx + 14, cy - 8, 19, boss);
        FillDiamond(px, cx, cy + 14, 31, boss);
        FillCircle(px, cx, cy + 1, 10, accent2);
    }
    else if (cell == 7)
    {
        FillCircle(px, cx, cy, 32, accent);
        FillCircle(px, cx, cy, 22, accent2);
        FillRect(px, cx - 5, cy - 23, 10, 46, accent);
    }
    else if (cell == 8)
    {
        FillCircle(px, cx, cy + 8, 32, Shade(wall, 1.55));
        DrawLineT(px, cx + 13, cy - 20, cx + 28, cy - 43, 3, accent);
        FillCircle(px, cx + 30, cy - 46, 5, boss);
    }
    else if (cell == 9)
    {
        FillCircle(px, cx - 23, cy, 17, accent2);
        FillCircle(px, cx - 23, cy, 8, dark);
        FillRect(px, cx - 7, cy - 4, 50, 8, accent2);
        FillRect(px, cx + 24, cy + 4, 8, 14, accent2);
        FillRect(px, cx + 38, cy + 4, 8, 22, accent2);
    }
    else if (cell == 10)
    {
        FillCircle(px, cx, cy, 43, accent);
        FillCircle(px, cx, cy, 32, dark);
        FillCircle(px, cx, cy, 24, accent2);
        FillCircle(px, cx, cy, 15, dark);
    }
    else if (cell == 11)
    {
        DrawLineT(px, cx - 34, cy + 8, cx + 29, cy - 9, 5, accent);
        FillCircle(px, cx + 35, cy - 11, 12, accent2);
        FillCircle(px, cx + 47, cy - 14, 5, boss);
    }
    else
    {
        int variant = cell%8;
        if (variant < 2)
        {
            FillRect(px, cx - 28, cy + 16, 56, 14, wall);
            DrawLineT(px, cx, cy + 16, cx, cy - 28, 4, accent2);
            FillCircle(px, cx - 14, cy - 15, 12, enemy);
            FillCircle(px, cx + 17, cy - 22, 13, accent);
        }
        else if (variant < 4)
        {
            FillDiamond(px, cx, cy, 33, line);
            FillDiamond(px, cx, cy, 23, wall);
            FillCircle(px, cx, cy, 7, accent2);
        }
        else if (variant < 6)
        {
            FillRect(px, cx - 31, cy - 22, 62, 44, wall);
            FillRect(px, cx - 22, cy - 13, 44, 26, Shade(wall, 1.45));
            DrawLineT(px, cx - 26, cy + 28, cx + 26, cy + 28, 3, accent);
        }
        else
        {
            FillCircle(px, cx - 17, cy + 12, 20, line);
            FillCircle(px, cx + 18, cy + 5, 27, wall);
            FillCircle(px, cx + 23, cy - 2, 7, accent2);
        }
    }
}

static void WriteU16(unsigned char *p, unsigned int v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
static void WriteU32(unsigned char *p, unsigned int v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

int GenWriteAtlasBmp(const GenRun *run, const char *outDir)
{
    size_t pixelBytes = (size_t)ATLAS_W*ATLAS_W*4;
    unsigned char *px = calloc(pixelBytes, 1);
    if (!px) return -1;
    for (int cell = 0; cell < 64; cell++) DrawCell(px, cell, run);

    unsigned char header[54] = { 0 };
    header[0] = 'B'; header[1] = 'M';
    WriteU32(header + 2, (unsigned int)(54 + pixelBytes));
    WriteU32(header + 10, 54);
    WriteU32(header + 14, 40);
    WriteU32(header + 18, ATLAS_W);
    WriteU32(header + 22, (unsigned int)(-ATLAS_W));   /* altezza negativa: righe dall'alto */
    WriteU16(header + 26, 1);
    WriteU16(header + 28, 32);
    WriteU32(header + 34, (unsigned int)pixelBytes);

    char path[512];
    snprintf(path, sizeof(path), "%s/current_atlas.bmp", outDir);
    FILE *f = fopen(path, "wb");
    if (!f) { free(px); return -1; }
    int ok = fwrite(header, 1, 54, f) == 54 && fwrite(px, 1, pixelBytes, f) == pixelBytes;
    fclose(f);
    free(px);
    return ok ? 0 : -1;
}
```

- [ ] **Step 9: Riscrivi `tools/melting-gen/main.c` con la CLI del fallback**

```c
#include "melting_gen.h"

#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct GenArgs {
    int fallback;
    int emitLlmJson;
    unsigned int seed;
    const char *outDir;
} GenArgs;

static int ParseArgs(int argc, char **argv, GenArgs *args)
{
    args->fallback = 0;
    args->emitLlmJson = 0;
    args->seed = (unsigned int)time(NULL);
    args->outDir = "generated";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            llama_backend_init();
            printf("melting-gen (llama.cpp b9979)\n%s\n", llama_print_system_info());
            llama_backend_free();
            exit(0);
        }
        else if (strcmp(argv[i], "--fallback") == 0) args->fallback = 1;
        else if (strcmp(argv[i], "--emit-llm-json") == 0) args->emitLlmJson = 1;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) args->seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) args->outDir = argv[++i];
        else
        {
            fprintf(stderr, "melting-gen: opzione sconosciuta: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

static int WriteOutputs(const GenRun *run, const GenArgs *args)
{
    GenProgressWrite(args->outDir, "scrivo", 85, "scrivo manifest e atlas");
    if (GenWriteAtlasBmp(run, args->outDir) != 0 || GenWriteRunFiles(run, args->outDir) != 0)
    {
        GenProgressWrite(args->outDir, "errore", 100, "scrittura file fallita");
        return 3;
    }
    if (args->emitLlmJson)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/llm_sample.json", args->outDir);
        if (GenWriteLlmJson(run, path) != 0) return 3;
    }
    GenProgressWrite(args->outDir, "fine", 100, "manifest pronto");
    GenLogLine("source=%s seed=%u out=%s", run->source, run->seed, args->outDir);
    return 0;
}

int main(int argc, char **argv)
{
    GenArgs args;
    if (ParseArgs(argc, argv, &args) != 0) return 2;
    if (GenEnsureDir(args.outDir) != 0)
    {
        fprintf(stderr, "melting-gen: impossibile creare %s\n", args.outDir);
        return 3;
    }
    GenProgressWrite(args.outDir, "avvio", 0, "melting-gen avviato");

    if (!args.fallback)
    {
        fprintf(stderr, "melting-gen: la generazione LLM arriva con i task successivi; usa --fallback\n");
        GenProgressWrite(args.outDir, "errore", 100, "modalita' non disponibile");
        return 2;
    }

    GenRun run;
    GenFallbackRun(&run, args.seed);
    return WriteOutputs(&run, &args);
}
```

- [ ] **Step 10: Aggiungi `--manifest-test` al gioco**

In `src/tests/game_tests.h` aggiungi la dichiarazione:

```c
bool GameManifestTest(Game *game);
```

In `src/tests/game_tests.c` (in fondo, con `#include <string.h>` tra gli include):

```c
bool GameManifestTest(Game *game)
{
    if (!game->content.loaded) return false;
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        if (!game->content.floors[f].theme.name[0]) return false;
        for (int i = 0; i < 3; i++)
        {
            const Item *item = &game->content.floors[f].items[i];
            if (!item->name[0] || !strchr(item->script, ':')) return false;
        }
    }
    return true;
}
```

In `src/app/app.c`, nel blocco di parsing argomenti di `AppRun` (dopo `--script-test`), aggiungi:

```c
        if (strcmp(argv[i], "--manifest-test") == 0)
        {
            smokeTest = true;
            manifestTest = true;
        }
```

con `bool manifestTest = false;` accanto agli altri flag, e dopo il blocco `if (scriptTest) {...}` aggiungi:

```c
    if (manifestTest)
    {
        bool ok = GameManifestTest(&game);
        printf("Manifest test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 5;
    }
```

(`game_tests.h` è già incluso da `app.c`? Verifica: se no, aggiungi `#include "tests/game_tests.h"` tra gli include di `app.c`.)

- [ ] **Step 11: Compila ed esegui il test**

```bash
make && make test-gen
```
Expected: tutte le sezioni passano, output finale `TEST-GEN: OK`. Ispezione visiva facoltativa: `./bin/melting_run_gpu` ora mostra la run fallback con l'atlas BMP generato (sprite a forme colorate coerenti col tema).

- [ ] **Step 12: Esegui anche i test esistenti (regressione)**

```bash
make test
```
Expected: i 4 test passano come nel Task 1.

- [ ] **Step 13: Commit**

```bash
git add tools/melting-gen scripts/test-gen.sh src/tests src/app/app.c Makefile
git commit -m "feat: melting-gen fallback generator with manifest, JSON and BMP atlas parity"
```

---

### Task 5: Grammatica GBNF + prompt

La grammatica vincola il campionamento dell'LLM al JSON esatto dello schema. La sua correttezza è verificata con `test-gbnf-validator` (build llama.cpp) contro un campione emesso da `GenWriteLlmJson`: così writer C, grammatica e schema restano coerenti per costruzione, senza golden file mantenuti a mano.

**Files:**
- Create: `tools/melting-gen/run.gbnf`, `tools/melting-gen/prompts/system.txt`, `tools/melting-gen/prompts/user.txt`
- Modify: `scripts/test-gen.sh`

**Interfaces:**
- Consumes: `GenWriteLlmJson` (Task 4), `deps/llama.cpp/build/bin/test-gbnf-validator` (Task 2).
- Produces: `run.gbnf` con simbolo iniziale `root` (usato da `llama_sampler_init_grammar` nel Task 7); prompt con segnaposto `{SEED}` in `user.txt`.

- [ ] **Step 1: Aggiungi il test di coerenza a `scripts/test-gen.sh`** (prima della riga `echo "TEST-GEN: OK"`)

```bash
echo "-- coerenza writer C <-> grammatica GBNF --"
GBNF=deps/llama.cpp/build/bin/test-gbnf-validator
"$GEN" --fallback --seed 1 --out "$TMP/g" --emit-llm-json
"$GBNF" tools/melting-gen/run.gbnf "$TMP/g/llm_sample.json" | grep -q "is valid"

echo "-- la grammatica rifiuta un enum sbagliato --"
sed 's/"hat"/"hut"/; s/"eyes"/"eyez"/' "$TMP/g/llm_sample.json" > "$TMP/g/broken.json"
if "$GBNF" tools/melting-gen/run.gbnf "$TMP/g/broken.json" | grep -q "is valid"; then
  echo "FALLITO: la grammatica ha accettato uno slot inesistente"; exit 1
fi
```

- [ ] **Step 2: Esegui e verifica che fallisca** (`run.gbnf` non esiste)

```bash
make test-gen
```
Expected: FAIL sulla sezione «coerenza writer C <-> grammatica GBNF».

- [ ] **Step 3: Scrivi `tools/melting-gen/run.gbnf`**

```text
# JSON compatto della run: 5 piani, 3 oggetti per piano, 1-3 operazioni script.
# Ordine delle chiavi FISSO, identico a RunToJson() in gen_manifest.c.
root ::= "{\"floors\":[" floor "," floor "," floor "," floor "," floor "]}"

floor ::= "{\"theme\":" name ",\"style\":" name ",\"boss\":" name ",\"bg\":" color ",\"floor\":" color ",\"wall\":" color ",\"accent\":" color ",\"accent2\":" color ",\"enemy\":" color ",\"bossColor\":" color ",\"items\":[" item "," item "," item "]}"

item ::= "{\"name\":" name ",\"slot\":" slot ",\"traits\":[" trait ("," trait)? "],\"color\":" color ",\"script\":[" op ("," op)? ("," op)? "]}"

op ::= "{\"trigger\":" trigger ",\"op\":" opkind ",\"a\":" num ",\"b\":" num ",\"trait\":" optrait "}"

name ::= "\"" namechar{3,40} "\""
namechar ::= [A-Za-z0-9 .,'!-]
color ::= "\"#" hexd hexd hexd hexd hexd hexd "\""
hexd ::= [0-9a-fA-F]
slot ::= "\"hat\"" | "\"eyes\"" | "\"hand\"" | "\"back\"" | "\"body\"" | "\"aura\""
trait ::= "\"bounce\"" | "\"homing\"" | "\"explode\"" | "\"split\"" | "\"pierce\"" | "\"rapid\"" | "\"giant\"" | "\"slow\"" | "\"vamp\""
trigger ::= "\"on_fire\"" | "\"on_hit\""
opkind ::= "\"burst\"" | "\"projectile\"" | "\"area\"" | "\"heal\""
optrait ::= trait | "\"none\""
num ::= [0-9] [0-9]? [0-9]? ("." [0-9] [0-9]?)?
```

Nota: se `test-gbnf-validator` rifiutasse la sintassi `{3,40}` (repetizione limitata), sostituisci `namechar{3,40}` con `namechar namechar namechar namechar*` — funziona con qualunque versione, perde solo il limite superiore (che il validatore C comunque impone troncando a 47).

- [ ] **Step 4: Scrivi `tools/melting-gen/prompts/system.txt`**

```text
Sei un generatore di contenuti per un piccolo action roguelite top-down in pixel art.
Inventa contenuti originali e coerenti. Non copiare giochi o marchi esistenti.
Rispondi con UN SOLO oggetto JSON compatto, senza testo prima o dopo, nel formato esatto:
{"floors":[F1,F2,F3,F4,F5]}
Ogni piano F: {"theme":"...","style":"...","boss":"...","bg":"#rrggbb","floor":"#rrggbb","wall":"#rrggbb","accent":"#rrggbb","accent2":"#rrggbb","enemy":"#rrggbb","bossColor":"#rrggbb","items":[I1,I2,I3]}
- theme, style, boss: stringhe brevi (3-40 caratteri), in italiano, evocative, senza accenti.
- bg e floor: colori scuri. wall: tono medio. accent, accent2, enemy, bossColor: vividi e leggibili.
Ogni oggetto I: {"name":"...","slot":"...","traits":[...],"color":"#rrggbb","script":[...]}
- slot: uno tra hat, eyes, hand, back, body, aura.
- traits: 1 o 2 tra bounce, homing, explode, split, pierce, rapid, giant, slow, vamp.
- script: 1-3 operazioni. Coppie valide: on_fire usa SOLO burst; on_hit usa projectile, area oppure heal.
- Parametri: burst a=colpi(1-6) b=apertura(0.05-1.2); projectile a=colpi(1-6) b=velocita(120-720); area a=raggio(18-96) b=scalaDanno(0.05-1.15); heal a=probabilita(0-60) b=cura(1-2).
- Ogni operazione deve avere trait uguale a uno dei traits dell'oggetto (oppure none) e rafforzarlo.
- Evita script puramente cosmetici: ogni oggetto deve creare una piccola sinergia giocabile.
```

- [ ] **Step 5: Scrivi `tools/melting-gen/prompts/user.txt`**

```text
Seed della run: {SEED}. Genera i 5 piani con temi molto diversi tra loro e una progressione di difficolta'. Il boss del piano 5 deve essere il piu' minaccioso. Esempio del formato di UN singolo piano (non copiarne i contenuti, serve solo per il formato):
{"theme":"Cantina Neon","style":"pixel semplice","boss":"Custode Verde","bg":"#151020","floor":"#241c30","wall":"#4a3a5c","accent":"#e08840","accent2":"#8fd4c0","enemy":"#c05555","bossColor":"#d8a030","items":[{"name":"Corona Frantumante","slot":"hat","traits":["split"],"color":"#e0c060","script":[{"trigger":"on_fire","op":"burst","a":3,"b":0.36,"trait":"split"}]},{"name":"Occhiali Segugio","slot":"eyes","traits":["homing"],"color":"#70c0e0","script":[{"trigger":"on_hit","op":"projectile","a":2,"b":260,"trait":"homing"}]},{"name":"Mantello Vampiro","slot":"back","traits":["vamp"],"color":"#b05070","script":[{"trigger":"on_hit","op":"heal","a":18,"b":1,"trait":"vamp"}]}]}
```

- [ ] **Step 6: Esegui il test**

```bash
make test-gen
```
Expected: `TEST-GEN: OK`, incluse le due nuove sezioni (campione valido, enum sbagliato rifiutato).

- [ ] **Step 7: Commit**

```bash
git add tools/melting-gen/run.gbnf tools/melting-gen/prompts scripts/test-gen.sh
git commit -m "feat: GBNF grammar and prompts, writer/grammar coherence test"
```

---

### Task 6: Validatore/normalizzatore (port delle regole Node)

Porta in C `normalizeRun` e funzioni collegate di `llm/run_content.mjs:255-360`: la grammatica garantisce la forma, questo garantisce la semantica (coppie trigger/op valide, parametri clampati, trait coerenti). Testato con un corpus di JSON rotti committato.

**Files:**
- Create: `tools/melting-gen/gen_validate.c`, `tests/melting-gen/bad/wrong-op-pair.json`, `tests/melting-gen/bad/out-of-range.json`, `tests/melting-gen/bad/wrong-types.json`, `tests/melting-gen/bad/missing-floors.json`, `tests/melting-gen/unparseable.txt`
- Modify: `tools/melting-gen/main.c`, `scripts/test-gen.sh`

**Interfaces:**
- Consumes: `GenFallbackRun`, `GenTraitRuleFor`, cJSON.
- Produces: `GenNormalizeRun(const struct cJSON *raw, unsigned int seed, GenRun *out)` — non fallisce mai: ogni campo invalido viene sostituito dal corrispondente fallback con lo stesso seed. CLI: `melting-gen --from-json FILE` (exit 4 se il JSON non è parsabile, 0 altrimenti).

- [ ] **Step 1: Scrivi i file del corpus**

`tests/melting-gen/bad/wrong-op-pair.json` (coppia on_fire+projectile da correggere in burst; un solo piano/oggetto: il resto arriva dal fallback):

```json
{"floors":[{"theme":"Prova Coppie","style":"pixel semplice","boss":"Custode Uno","bg":"#101018","floor":"#202030","wall":"#404060","accent":"#e08840","accent2":"#80d0c0","enemy":"#c05050","bossColor":"#d0a030","items":[{"name":"Anello Sbagliato","slot":"hand","traits":["homing"],"color":"#a0c0e0","script":[{"trigger":"on_fire","op":"projectile","a":2,"b":260,"trait":"homing"}]}]}]}
```

`tests/melting-gen/bad/out-of-range.json` (parametri fuori scala da clampare):

```json
{"floors":[{"theme":"Fuori Scala","style":"toon scuro","boss":"Custode Esagerato","bg":"#0a0a0a","floor":"#1a1a2a","wall":"#3a3a5a","accent":"#ff0000","accent2":"#00ff00","enemy":"#0000ff","bossColor":"#ffff00","items":[{"name":"Bomba Enorme","slot":"hat","traits":["split","vamp"],"color":"#ffffff","script":[{"trigger":"on_fire","op":"burst","a":99,"b":9.9,"trait":"split"},{"trigger":"on_hit","op":"heal","a":999,"b":7,"trait":"vamp"}]}]}]}
```

`tests/melting-gen/bad/wrong-types.json` (tipi sbagliati, stringhe vuote, colori invalidi, trait inesistenti e duplicati):

```json
{"floors":[{"theme":42,"style":null,"boss":"","bg":"rosso","floor":"#12345","wall":"#gg0000","accent":true,"accent2":"#123456","enemy":"#abcdef","bossColor":[],"items":[{"name":"   ","slot":"head","traits":["fuoco","split","split"],"color":"nero","script":"non-un-array"}]}]}
```

`tests/melting-gen/bad/missing-floors.json`:

```json
{"floors":[]}
```

`tests/melting-gen/unparseable.txt`:

```text
questo non e' json {
```

- [ ] **Step 2: Aggiungi i test del corpus a `scripts/test-gen.sh`** (prima di `echo "TEST-GEN: OK"`)

```bash
echo "-- corpus JSON rotti: normalizzati senza crash, manifest completo --"
for f in tests/melting-gen/bad/*.json; do
  "$GEN" --from-json "$f" --seed 7 --out "$TMP/bad"
  grep -q "^floor5.item3.script=" "$TMP/bad/current_run.txt" || { echo "FALLITO su $f"; exit 1; }
done

echo "-- normalizzazioni puntuali --"
"$GEN" --from-json tests/melting-gen/bad/wrong-op-pair.json --seed 7 --out "$TMP/n1"
grep -q "^floor1.item1.script=on_fire:burst,2,1.2,homing$" "$TMP/n1/current_run.txt"
"$GEN" --from-json tests/melting-gen/bad/out-of-range.json --seed 7 --out "$TMP/n2"
grep -q "^floor1.item1.script=on_fire:burst,6,1.2,split|on_hit:heal,60,2,vamp$" "$TMP/n2/current_run.txt"

echo "-- JSON non parsabile -> exit 4 --"
set +e
"$GEN" --from-json tests/melting-gen/unparseable.txt --seed 7 --out "$TMP/x"
rc=$?
set -e
[ "$rc" -eq 4 ]
```

- [ ] **Step 3: Esegui e verifica che fallisca** (`--from-json` non esiste ancora)

```bash
make test-gen
```
Expected: FAIL — `melting-gen: opzione sconosciuta: --from-json`.

- [ ] **Step 4: Scrivi `tools/melting-gen/gen_validate.c`**

```c
#include "melting_gen.h"

#include "cJSON.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Stesse regole di llm/run_content.mjs (SCRIPT_BOUNDS, OP_TRAITS, SCRIPT_TRAIT_PRIORITY). */

static const char *SCRIPT_TRAIT_PRIORITY[9] = {
    "split", "bounce", "rapid", "homing", "pierce", "explode", "slow", "giant", "vamp"
};

typedef struct OpBounds {
    const char *op;
    double aMin, aMax, bMin, bMax;
    int roundA, roundB;
} OpBounds;

static const OpBounds OP_BOUNDS[4] = {
    { "burst",       1,  6, 0.05, 1.20, 1, 0 },
    { "projectile",  1,  6, 120,  720,  1, 0 },
    { "area",       18, 96, 0.05, 1.15, 0, 0 },
    { "heal",        0, 60, 1,    2,    1, 1 },
};

static const OpBounds *BoundsFor(const char *op)
{
    for (int i = 0; i < 4; i++) if (strcmp(OP_BOUNDS[i].op, op) == 0) return &OP_BOUNDS[i];
    return &OP_BOUNDS[1];
}

static int OpAllowsTrait(const char *op, const char *trait)
{
    static const char *burst[]      = { "split", "bounce", "rapid", "homing", "pierce", NULL };
    static const char *projectile[] = { "homing", "pierce", "bounce", "rapid", NULL };
    static const char *area[]       = { "explode", "slow", "giant", NULL };
    static const char *heal[]       = { "vamp", NULL };
    const char **list = NULL;
    if (strcmp(op, "burst") == 0) list = burst;
    else if (strcmp(op, "projectile") == 0) list = projectile;
    else if (strcmp(op, "area") == 0) list = area;
    else if (strcmp(op, "heal") == 0) list = heal;
    if (!list || !trait) return 0;
    for (int i = 0; list[i]; i++) if (strcmp(list[i], trait) == 0) return 1;
    return 0;
}

static int TraitIndex(const char *trait)
{
    for (int i = 0; i < 9; i++) if (trait && strcmp(GEN_TRAITS[i], trait) == 0) return i;
    return -1;
}

static int PriorityIndex(const char *trait)
{
    for (int i = 0; i < 9; i++) if (trait && strcmp(SCRIPT_TRAIT_PRIORITY[i], trait) == 0) return i;
    return 9;
}

static const char *JsonString(const cJSON *obj, const char *key)
{
    if (!obj) return NULL;
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    return (cJSON_IsString(v) && v->valuestring) ? v->valuestring : NULL;
}

static double JsonNumber(const cJSON *obj, const char *key, double fallback, int *ok)
{
    *ok = 0;
    if (!obj) return fallback;
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(v)) return fallback;
    *ok = 1;
    return v->valuedouble;
}

static void CopyText(char *dst, size_t dstSize, const char *src, const char *fallback)
{
    const char *use = fallback;
    if (src)
    {
        while (*src == ' ' || *src == '\t') src++;
        if (*src) use = src;
    }
    snprintf(dst, dstSize, "%s", use ? use : "");
    size_t len = strlen(dst);
    while (len > 0 && (dst[len - 1] == ' ' || dst[len - 1] == '\t')) dst[--len] = '\0';
}

static int IsHexColor(const char *text)
{
    if (!text || text[0] != '#' || strlen(text) != 7) return 0;
    for (int i = 1; i < 7; i++)
    {
        char c = text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return 0;
    }
    return 1;
}

static void CopyColor(char *dst, size_t dstSize, const char *src, const char *fallback)
{
    snprintf(dst, dstSize, "%s", IsHexColor(src) ? src : fallback);
}

static double ClampD(double v, double min, double max)
{
    return v < min ? min : (v > max ? max : v);
}

static void NormalizeTraits(const cJSON *rawTraits, const GenItem *fbItem, GenItem *out)
{
    out->traitCount = 0;
    if (cJSON_IsArray((cJSON *)rawTraits))
    {
        const cJSON *el = NULL;
        cJSON_ArrayForEach(el, (cJSON *)rawTraits)
        {
            if (out->traitCount >= 2) break;
            if (!cJSON_IsString(el) || TraitIndex(el->valuestring) < 0) continue;
            int dup = 0;
            for (int i = 0; i < out->traitCount; i++)
            {
                if (strcmp(out->traits[i], el->valuestring) == 0) dup = 1;
            }
            if (!dup) snprintf(out->traits[out->traitCount++], sizeof(out->traits[0]), "%s", el->valuestring);
        }
    }
    if (out->traitCount == 0)
    {
        for (int i = 0; i < fbItem->traitCount && i < 2; i++)
        {
            snprintf(out->traits[out->traitCount++], sizeof(out->traits[0]), "%s", fbItem->traits[i]);
        }
    }
    if (out->traitCount == 2 && PriorityIndex(out->traits[0]) > PriorityIndex(out->traits[1]))
    {
        char tmp[10];
        memcpy(tmp, out->traits[0], sizeof(tmp));
        memcpy(out->traits[0], out->traits[1], sizeof(tmp));
        memcpy(out->traits[1], tmp, sizeof(tmp));
    }
}

static const char *PickScriptTrait(const char *op, const GenItem *item, const char *want)
{
    if (want && OpAllowsTrait(op, want))
    {
        for (int i = 0; i < item->traitCount; i++)
        {
            if (strcmp(item->traits[i], want) == 0) return want;
        }
    }
    for (int i = 0; i < item->traitCount; i++)
    {
        if (OpAllowsTrait(op, item->traits[i])) return item->traits[i];
    }
    return "none";
}

static const char *PreferredOpForTraits(const GenItem *item)
{
    const GenTraitRule *rule = item->traitCount > 0 ? GenTraitRuleFor(item->traits[0]) : NULL;
    return rule ? rule->op : "projectile";
}

static void NormalizeScriptOp(const cJSON *rawOp, const GenItem *item, GenScriptOp *out)
{
    const char *rawTrigger = JsonString(rawOp, "trigger");
    const char *rawKind = JsonString(rawOp, "op");
    const char *trigger = (rawTrigger && strcmp(rawTrigger, "on_fire") == 0) ? "on_fire" : "on_hit";
    const char *kind = "projectile";
    if (rawKind && (strcmp(rawKind, "burst") == 0 || strcmp(rawKind, "projectile") == 0 ||
                    strcmp(rawKind, "area") == 0 || strcmp(rawKind, "heal") == 0)) kind = rawKind;

    if (strcmp(trigger, "on_fire") == 0) kind = "burst";
    else if (strcmp(kind, "burst") == 0) kind = "projectile";

    int anyCompatible = 0;
    for (int i = 0; i < item->traitCount; i++)
    {
        if (OpAllowsTrait(kind, item->traits[i])) anyCompatible = 1;
    }
    if (!anyCompatible) kind = PreferredOpForTraits(item);

    const char *trait = PickScriptTrait(kind, item, JsonString(rawOp, "trait"));
    const GenTraitRule *rule = GenTraitRuleFor(trait);
    double defA = rule ? rule->a : 1;
    double defB = rule ? rule->b : (strcmp(kind, "projectile") == 0 ? 280 : 0.35);
    const OpBounds *bounds = BoundsFor(kind);

    int okA = 0, okB = 0;
    double a = JsonNumber(rawOp, "a", defA, &okA);
    double b = JsonNumber(rawOp, "b", defB, &okB);
    a = ClampD(okA ? a : defA, bounds->aMin, bounds->aMax);
    b = ClampD(okB ? b : defB, bounds->bMin, bounds->bMax);
    if (bounds->roundA) a = floor(a + 0.5);
    if (bounds->roundB) b = floor(b + 0.5);

    snprintf(out->trigger, sizeof(out->trigger), "%s", strcmp(kind, "burst") == 0 ? "on_fire" : "on_hit");
    snprintf(out->op, sizeof(out->op), "%s", kind);
    out->a = a;
    out->b = b;
    snprintf(out->trait, sizeof(out->trait), "%s", trait);
}
```

Continua il file:

```c
static void NormalizeScript(const cJSON *rawScript, const GenItem *fbItem, GenItem *item)
{
    item->opCount = 0;
    if (cJSON_IsArray((cJSON *)rawScript))
    {
        const cJSON *el = NULL;
        cJSON_ArrayForEach(el, (cJSON *)rawScript)
        {
            if (item->opCount >= GEN_MAX_OPS) break;
            if (!cJSON_IsObject(el)) continue;
            NormalizeScriptOp(el, item, &item->ops[item->opCount]);
            item->opCount++;
        }
    }
    for (int t = 0; t < item->traitCount && item->opCount < GEN_MAX_OPS; t++)
    {
        int covered = 0;
        for (int s = 0; s < item->opCount; s++)
        {
            if (strcmp(item->ops[s].trait, item->traits[t]) == 0) covered = 1;
        }
        const GenTraitRule *rule = GenTraitRuleFor(item->traits[t]);
        if (covered || !rule) continue;
        GenScriptOp *op = &item->ops[item->opCount++];
        snprintf(op->trigger, sizeof(op->trigger), "%s", rule->trigger);
        snprintf(op->op, sizeof(op->op), "%s", rule->op);
        op->a = rule->a;
        op->b = rule->b;
        snprintf(op->trait, sizeof(op->trait), "%s", rule->trait);
    }
    if (item->opCount == 0)
    {
        memcpy(item->ops, fbItem->ops, sizeof(item->ops));
        item->opCount = fbItem->opCount;
    }
}

void GenNormalizeRun(const struct cJSON *rawRoot, unsigned int seed, GenRun *out)
{
    GenRun fb;
    GenFallbackRun(&fb, seed);
    memset(out, 0, sizeof(*out));
    snprintf(out->source, sizeof(out->source), "local");
    out->seed = seed;

    const cJSON *floors = rawRoot ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawRoot, "floors") : NULL;
    for (int f = 0; f < GEN_FLOORS; f++)
    {
        const cJSON *rawFloor = cJSON_IsArray((cJSON *)floors) ? cJSON_GetArrayItem((cJSON *)floors, f) : NULL;
        const GenFloor *fbFloor = &fb.floors[f];
        GenFloor *floor = &out->floors[f];
        CopyText(floor->theme, sizeof(floor->theme), JsonString(rawFloor, "theme"), fbFloor->theme);
        CopyText(floor->style, sizeof(floor->style), JsonString(rawFloor, "style"), fbFloor->style);
        CopyText(floor->boss, sizeof(floor->boss), JsonString(rawFloor, "boss"), fbFloor->boss);
        CopyColor(floor->bg, sizeof(floor->bg), JsonString(rawFloor, "bg"), fbFloor->bg);
        CopyColor(floor->floorColor, sizeof(floor->floorColor), JsonString(rawFloor, "floor"), fbFloor->floorColor);
        CopyColor(floor->wall, sizeof(floor->wall), JsonString(rawFloor, "wall"), fbFloor->wall);
        CopyColor(floor->accent, sizeof(floor->accent), JsonString(rawFloor, "accent"), fbFloor->accent);
        CopyColor(floor->accent2, sizeof(floor->accent2), JsonString(rawFloor, "accent2"), fbFloor->accent2);
        CopyColor(floor->enemy, sizeof(floor->enemy), JsonString(rawFloor, "enemy"), fbFloor->enemy);
        CopyColor(floor->bossColor, sizeof(floor->bossColor), JsonString(rawFloor, "bossColor"), fbFloor->bossColor);

        const cJSON *rawItems = rawFloor ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawFloor, "items") : NULL;
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            const cJSON *rawItem = cJSON_IsArray((cJSON *)rawItems) ? cJSON_GetArrayItem((cJSON *)rawItems, i) : NULL;
            const GenItem *fbItem = &fbFloor->items[i];
            GenItem *item = &floor->items[i];
            CopyText(item->name, sizeof(item->name), JsonString(rawItem, "name"), fbItem->name);
            const char *slot = JsonString(rawItem, "slot");
            int slotOk = 0;
            for (int s = 0; s < 6; s++)
            {
                if (slot && strcmp(GEN_SLOTS[s], slot) == 0) slotOk = 1;
            }
            snprintf(item->slot, sizeof(item->slot), "%s", slotOk ? slot : fbItem->slot);
            NormalizeTraits(rawItem ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawItem, "traits") : NULL, fbItem, item);
            CopyColor(item->color, sizeof(item->color), JsonString(rawItem, "color"), fbItem->color);
            NormalizeScript(rawItem ? cJSON_GetObjectItemCaseSensitive((cJSON *)rawItem, "script") : NULL, fbItem, item);
        }
    }
}
```

- [ ] **Step 5: Collega `--from-json` in `main.c`**

Aggiungi `#include "cJSON.h"` e `#include <stdlib.h>` (per `free`) agli include di `main.c`, il campo `const char *fromJson;` a `GenArgs` (inizializzato a `NULL` in `ParseArgs`), il ramo di parsing:

```c
        else if (strcmp(argv[i], "--from-json") == 0 && i + 1 < argc) args->fromJson = argv[++i];
```

e in `main`, al posto del blocco «generazione LLM non disponibile», PRIMA del ramo fallback:

```c
    if (args.fromJson)
    {
        char *text = GenReadFile(args.fromJson);
        cJSON *root = text ? cJSON_Parse(text) : NULL;
        free(text);
        if (!root)
        {
            fprintf(stderr, "melting-gen: JSON non parsabile: %s\n", args.fromJson);
            GenProgressWrite(args.outDir, "errore", 100, "JSON non parsabile");
            return 4;
        }
        GenRun run;
        GenProgressWrite(args.outDir, "valido", 60, "valido e normalizzo il JSON");
        GenNormalizeRun(root, args.seed, &run);
        cJSON_Delete(root);
        snprintf(run.source, sizeof(run.source), "from-json");
        return WriteOutputs(&run, &args);
    }
```

(il ramo `--fallback` resta; senza flag, l'errore «usa --fallback» resta fino al Task 7).

- [ ] **Step 6: Compila ed esegui**

```bash
make && make test-gen
```
Expected: `TEST-GEN: OK` con le sezioni corpus e normalizzazioni puntuali verdi. Se i grep puntuali falliscono, confronta il valore atteso col prodotto: sono i clamp di `SCRIPT_BOUNDS` (es. `b` di burst limitato a 1.2).

- [ ] **Step 7: Commit**

```bash
git add tools/melting-gen/gen_validate.c tools/melting-gen/main.c tests/melting-gen scripts/test-gen.sh
git commit -m "feat: C port of run normalization rules with broken-JSON corpus tests"
```

---

### Task 7: Inferenza locale (gen_llm.c) e test-llm

**Files:**
- Create: `tools/melting-gen/gen_llm.c`, `scripts/test-llm.sh`
- Modify: `tools/melting-gen/melting_gen.h`, `tools/melting-gen/main.c`, `tools/melting-gen/gen_util.c`, `Makefile`

**Interfaces:**
- Consumes: API C di llama.cpp b9979 (firme verificate su `include/llama.h` del tag), `run.gbnf`, `prompts/`, `GenNormalizeRun`.
- Produces: `GenLlmGenerate(const GenLlmConfig*, char *out, size_t outCap, double *loadSecs, double *genSecs, int *tokensOut)` → 0 ok / -1 errore. `GenFileExists(path)`. CLI: senza flag melting-gen ora genera con l'LLM (7B → ripiego 1.5B → ripiego `--fallback`), con `--model`, `--ngl`, `--temp`, `--n-predict`, `--prompts`, `--grammar`. `make test-llm` (variabili `MODEL`, `NGL`, `SEED`). Deviazione dichiarata dalla spec: il flag `--ctx` è sostituito dal calcolo automatico `n_ctx = token del prompt + n-predict` (più sicuro di un valore fisso; la spec §5 viene aggiornata implicitamente da questa scelta).
- Regole fisse: mai abilitare la flash-attention (RDNA1); non impostarla affatto — il default del backend Vulkan la gestisce da solo. La grammatica parte dal primo token generato; `llama_sampler_init_grammar` che ritorna `NULL` = grammatica rotta = errore, mai proseguire senza.

- [ ] **Step 1: Scrivi il test — `scripts/test-llm.sh`**

```bash
#!/usr/bin/env bash
# Generazione reale con modello. Variabili: MODEL, NGL, SEED.
set -euo pipefail
cd "$(dirname "$0")/.."
MODEL="${MODEL:-models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf}"
NGL="${NGL:-99}"
SEED="${SEED:-31337}"
[ -f "$MODEL" ] || { echo "Modello mancante: $MODEL — esegui scripts/download-models.sh"; exit 1; }

bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$SEED" --out generated
grep -q "^source=local:" generated/current_run.txt
grep -q "^floor5.item3.script=" generated/current_run.txt
bin/melting_run_gpu --manifest-test
echo "--- ultima riga di log (tempi e tok/s) ---"
tail -1 logs/melting-gen.log
echo "TEST-LLM: OK"
```

```bash
chmod +x scripts/test-llm.sh
```

Nel Makefile aggiungi `test-llm` a `.PHONY` e:

```make
test-llm: all
	bash scripts/test-llm.sh
```

```bash
make test-llm
```
Expected: FAIL — melting-gen senza `--fallback`/`--from-json` risponde ancora «usa --fallback».

- [ ] **Step 2: Estendi `melting_gen.h`** (dopo la sezione gen_validate)

```c
/* gen_llm.c */
typedef struct GenLlmConfig {
    const char *modelPath;
    int nGpuLayers;
    int nPredict;
    float temp;
    unsigned int seed;
    const char *outDir;
    const char *promptsDir;
    const char *grammarPath;
} GenLlmConfig;

int GenLlmGenerate(const GenLlmConfig *cfg, char *out, size_t outCap,
                   double *loadSecs, double *genSecs, int *tokensOut);
```

e nella sezione gen_util.c la dichiarazione `int GenFileExists(const char *path);`, con l'implementazione in `gen_util.c`:

```c
int GenFileExists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}
```

- [ ] **Step 3: Scrivi `tools/melting-gen/gen_llm.c`**

```c
#include "melting_gen.h"

#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

static bool LoadProgressCb(float progress, void *user)
{
    const char *outDir = user;
    int pct = (int)(progress*60.0f);   /* caricamento modello = 0..60% della barra */
    GenProgressWrite(outDir, "carico-modello", pct, "carico il modello (Vulkan)");
    return true;   /* false interromperebbe il caricamento */
}

/* Prompt ChatML di Qwen2.5: <|im_end|> subito dopo il contenuto, poi newline. */
static char *BuildPrompt(const GenLlmConfig *cfg)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/system.txt", cfg->promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/user.txt", cfg->promptsDir);
    char *user = GenReadFile(path);
    if (!sys || !user)
    {
        free(sys);
        free(user);
        return NULL;
    }

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", cfg->seed);
    size_t userCap = strlen(user) + sizeof(seedText) + 1;
    char *userFinal = malloc(userCap);
    if (!userFinal)
    {
        free(sys);
        free(user);
        return NULL;
    }
    const char *mark = strstr(user, "{SEED}");
    if (mark)
    {
        size_t head = (size_t)(mark - user);
        memcpy(userFinal, user, head);
        userFinal[head] = '\0';
        strncat(userFinal, seedText, userCap - strlen(userFinal) - 1);
        strncat(userFinal, mark + 6, userCap - strlen(userFinal) - 1);
    }
    else snprintf(userFinal, userCap, "%s", user);

    size_t total = strlen(sys) + strlen(userFinal) + 128;
    char *prompt = malloc(total);
    if (prompt)
    {
        snprintf(prompt, total,
                 "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
                 sys, userFinal);
    }
    free(sys);
    free(user);
    free(userFinal);
    return prompt;
}

int GenLlmGenerate(const GenLlmConfig *cfg, char *out, size_t outCap,
                   double *loadSecs, double *genSecs, int *tokensOut)
{
    out[0] = '\0';
    *loadSecs = 0;
    *genSecs = 0;
    *tokensOut = 0;

    char *grammar = GenReadFile(cfg->grammarPath);
    char *prompt = BuildPrompt(cfg);
    if (!grammar || !prompt)
    {
        GenLogLine("llm: grammatica o prompt mancanti (%s, %s)", cfg->grammarPath, cfg->promptsDir);
        free(grammar);
        free(prompt);
        return -1;
    }

    llama_backend_init();
    int rc = -1;
    struct llama_model *model = NULL;
    struct llama_context *ctx = NULL;
    struct llama_sampler *smpl = NULL;
    llama_token *tokens = NULL;

    double t0 = NowSeconds();
    struct llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = cfg->nGpuLayers;
    mparams.progress_callback = LoadProgressCb;
    mparams.progress_callback_user_data = (void *)cfg->outDir;
    model = llama_model_load_from_file(cfg->modelPath, mparams);
    if (!model)
    {
        GenLogLine("llm: caricamento fallito: %s (ngl=%d)", cfg->modelPath, cfg->nGpuLayers);
        goto cleanup;
    }
    *loadSecs = NowSeconds() - t0;

    {
        const struct llama_vocab *vocab = llama_model_get_vocab(model);

        int n_prompt = -llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), NULL, 0, true, true);
        if (n_prompt <= 0) goto cleanup;
        tokens = malloc(sizeof(llama_token)*(size_t)n_prompt);
        if (!tokens) goto cleanup;
        if (llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), tokens, n_prompt, true, true) < 0) goto cleanup;

        struct llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = (uint32_t)(n_prompt + cfg->nPredict);
        cparams.n_batch = (uint32_t)n_prompt;
        ctx = llama_init_from_model(model, cparams);
        if (!ctx)
        {
            GenLogLine("llm: creazione contesto fallita (n_ctx=%d)", n_prompt + cfg->nPredict);
            goto cleanup;
        }

        smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
        struct llama_sampler *gsmpl = llama_sampler_init_grammar(vocab, grammar, "root");
        if (!gsmpl)
        {
            GenLogLine("llm: grammatica GBNF non parsabile: %s", cfg->grammarPath);
            goto cleanup;
        }
        llama_sampler_chain_add(smpl, gsmpl);   /* la grammatica PRIMA del selettore */
        llama_sampler_chain_add(smpl, llama_sampler_init_temp(cfg->temp));
        llama_sampler_chain_add(smpl, llama_sampler_init_dist(cfg->seed));

        GenProgressWrite(cfg->outDir, "genero", 62, "genero i contenuti della run");
        t0 = NowSeconds();
        size_t used = 0;
        int generated = 0;
        struct llama_batch batch = llama_batch_get_one(tokens, n_prompt);
        llama_token newToken = 0;
        rc = 0;
        while (generated < cfg->nPredict)
        {
            if (llama_decode(ctx, batch) != 0)
            {
                GenLogLine("llm: llama_decode fallita al token %d", generated);
                rc = -1;
                break;
            }
            newToken = llama_sampler_sample(smpl, ctx, -1);
            if (llama_vocab_is_eog(vocab, newToken)) break;
            char piece[128];
            int n = llama_token_to_piece(vocab, newToken, piece, sizeof(piece), 0, true);
            if (n < 0 || used + (size_t)n + 1 >= outCap)
            {
                rc = -1;
                break;
            }
            memcpy(out + used, piece, (size_t)n);
            used += (size_t)n;
            out[used] = '\0';
            generated++;
            if (generated%32 == 0)
            {
                int pct = 62 + (int)(30.0*generated/cfg->nPredict);
                char msg[96];
                snprintf(msg, sizeof(msg), "genero i contenuti (%d token)", generated);
                GenProgressWrite(cfg->outDir, "genero", pct > 92 ? 92 : pct, msg);
            }
            batch = llama_batch_get_one(&newToken, 1);
        }
        *genSecs = NowSeconds() - t0;
        *tokensOut = generated;
    }

cleanup:
    if (smpl) llama_sampler_free(smpl);   /* libera l'intera catena, grammatica inclusa */
    if (ctx) llama_free(ctx);
    if (model) llama_model_free(model);
    llama_backend_free();
    free(tokens);
    free(grammar);
    free(prompt);
    return rc;
}
```

- [ ] **Step 4: Collega l'inferenza in `main.c`**

Estendi `GenArgs` e `ParseArgs` con i campi/flag (default tra parentesi): `model` (`models/qwen2.5-coder-7b-instruct-q4_k_m.gguf`), `modelFallback` (`models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf`), `ngl` (`99` — il Task 8 lo ricalibra), `temp` (`0.8f`), `nPredict` (`2048`), `promptsDir` (`tools/melting-gen/prompts`), `grammarPath` (`tools/melting-gen/run.gbnf`):

```c
        else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) args->model = argv[++i];
        else if (strcmp(argv[i], "--ngl") == 0 && i + 1 < argc) args->ngl = atoi(argv[++i]);
        else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) args->temp = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--n-predict") == 0 && i + 1 < argc) args->nPredict = atoi(argv[++i]);
        else if (strcmp(argv[i], "--prompts") == 0 && i + 1 < argc) args->promptsDir = argv[++i];
        else if (strcmp(argv[i], "--grammar") == 0 && i + 1 < argc) args->grammarPath = argv[++i];
```

Sostituisci il blocco «generazione LLM arriva con i task successivi» con il percorso completo (dopo il ramo `--from-json`, prima del fallback):

```c
    GenRun run;
    int haveRun = 0;
    if (!args.fallback)
    {
        const char *modelPath = NULL;
        if (GenFileExists(args.model)) modelPath = args.model;
        else if (GenFileExists(args.modelFallback))
        {
            modelPath = args.modelFallback;
            GenLogLine("modello principale assente, ripiego su %s", modelPath);
        }
        else GenLogLine("nessun modello in models/: uso il fallback deterministico");

        static char json[65536];
        for (int attempt = 0; modelPath && attempt < 3 && !haveRun; attempt++)
        {
            GenLlmConfig cfg = {
                .modelPath = modelPath,
                .nGpuLayers = args.ngl,
                .nPredict = args.nPredict,
                .temp = args.temp,
                .seed = args.seed + (unsigned int)attempt*7919u,
                .outDir = args.outDir,
                .promptsDir = args.promptsDir,
                .grammarPath = args.grammarPath,
            };
            double loadSecs = 0, genSecs = 0;
            int tokens = 0;
            if (GenLlmGenerate(&cfg, json, sizeof(json), &loadSecs, &genSecs, &tokens) != 0)
            {
                GenLogLine("tentativo %d: generazione fallita", attempt + 1);
                continue;
            }
            cJSON *root = cJSON_Parse(json);
            if (!root)
            {
                GenLogLine("tentativo %d: JSON troncato o non parsabile (%d token)", attempt + 1, tokens);
                continue;
            }
            GenProgressWrite(args.outDir, "valido", 94, "valido e normalizzo");
            GenNormalizeRun(root, args.seed, &run);
            cJSON_Delete(root);
            const char *base = strrchr(modelPath, '/');
            snprintf(run.source, sizeof(run.source), "local:%s", base ? base + 1 : modelPath);
            GenLogLine("ok: model=%s ngl=%d load=%.1fs gen=%.1fs token=%d (%.1f tok/s)",
                       modelPath, args.ngl, loadSecs, genSecs, tokens,
                       genSecs > 0 ? tokens/genSecs : 0.0);
            haveRun = 1;
        }
    }
    if (!haveRun) GenFallbackRun(&run, args.seed);
    return WriteOutputs(&run, &args);
```

(il vecchio ramo `if (args.fallback) { GenFallbackRun...; return WriteOutputs...; }` viene assorbito da questo flusso unico: con `--fallback`, `haveRun` resta 0.)

- [ ] **Step 5: Prima generazione reale (modello piccolo)**

```bash
make && make test-llm
```
Expected: `TEST-LLM: OK`; la riga di log mostra `model=...1.5b... load=~2-5s gen=... token=... (N tok/s)`. Con l'1.5B su GPU aspettati generazione in secondi. Se `llama_decode` fallisce o il caricamento va in errore di memoria, riprova con `NGL=20 make test-llm` e annota il comportamento per il Task 8.

- [ ] **Step 6: Verifica visiva**

```bash
./bin/melting_run_gpu &
```
Expected: pannello sinistro «Fonte: LLM cache» (il manifest è caricato), temi/nomi inventati dal modello nei 5 piani. Chiudi con Q.

- [ ] **Step 7: Commit**

```bash
git add tools/melting-gen scripts/test-llm.sh Makefile
git commit -m "feat: local grammar-constrained generation via llama.cpp Vulkan"
```

---

### Task 8: Benchmark sulla macchina reale e default definitivi

**Files:**
- Create: `docs/BENCHMARKS.md`
- Modify: `tools/melting-gen/main.c` (solo i valori di default)

**Interfaces:**
- Consumes: `make test-llm` con variabili `MODEL`/`NGL`/`SEED`; `logs/melting-gen.log`.
- Produces: default calibrati di `--ngl` e del modello in `main.c`; tabella misure in `docs/BENCHMARKS.md` (base dati per la fase 5 della roadmap).

- [ ] **Step 1: Misura il 1.5B e il 7B a vari offload**

```bash
MODEL=models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf NGL=99 SEED=42 make test-llm
MODEL=models/qwen2.5-coder-7b-instruct-q4_k_m.gguf  NGL=99 SEED=42 make test-llm
MODEL=models/qwen2.5-coder-7b-instruct-q4_k_m.gguf  NGL=28 SEED=42 make test-llm
MODEL=models/qwen2.5-coder-7b-instruct-q4_k_m.gguf  NGL=24 SEED=42 make test-llm
MODEL=models/qwen2.5-coder-7b-instruct-q4_k_m.gguf  NGL=20 SEED=42 make test-llm
```
Per ogni corsa annota la riga finale di `logs/melting-gen.log` (load, gen, tok/s). Il 7B a `NGL=99` su 6GB può fallire il caricamento o degradare: è un risultato utile, annotalo. Durante le corse tieni d'occhio la VRAM se vuoi (`GALLIUM_HUD` o `radeontop` se installato — facoltativo).

- [ ] **Step 2: Scrivi `docs/BENCHMARKS.md` coi numeri veri**

```markdown
# Benchmark melting-gen — macchina di riferimento

Macchina: Ryzen 5 3600, RX 5600 XT 6GB (RDNA1, Mesa RADV, Vulkan), Ubuntu 26.04.
Comando: `MODEL=... NGL=... SEED=42 make test-llm`. Una riga per corsa, copiata da logs/melting-gen.log.

| Modello | ngl | load (s) | gen (s) | token | tok/s | Esito |
|---|---|---|---|---|---|---|
| 1.5B Q4_K_M | 99 | (misura) | (misura) | (misura) | (misura) | (ok/errore) |
| 7B Q4_K_M | 99 | (misura) | (misura) | (misura) | (misura) | (ok/errore) |
| 7B Q4_K_M | 28 | (misura) | (misura) | (misura) | (misura) | (ok/errore) |
| 7B Q4_K_M | 24 | (misura) | (misura) | (misura) | (misura) | (ok/errore) |
| 7B Q4_K_M | 20 | (misura) | (misura) | (misura) | (misura) | (ok/errore) |

Default scelti in tools/melting-gen/main.c: modello = (scelto), ngl = (scelto).
Criterio: la corsa piu' veloce che completa in modo stabile entro il budget di
1-2 minuti (spec §2); a parita' di stabilita' vince la qualita' (7B > 1.5B).
```

Le celle `(misura)` vanno riempite con i numeri reali PRIMA del commit: un BENCHMARKS.md con segnaposto è un errore, non committarlo così.

- [ ] **Step 3: Fissa i default in `main.c`**

In base alla tabella: se il 7B completa stabilmente entro ~90s a un certo `ngl`, quello diventa il default (`args->ngl`); altrimenti il default `--model` diventa il percorso del 1.5B con `ngl=99`. Aggiorna i valori di default in `ParseArgs` e riesegui:

```bash
make test-llm
```
Expected: `TEST-LLM: OK` coi default, senza variabili d'ambiente.

- [ ] **Step 4: Commit**

```bash
git add docs/BENCHMARKS.md tools/melting-gen/main.c
git commit -m "feat: calibrate default model and gpu offload from measured benchmarks"
```

---

### Task 9: src/gen — gen_runner con finto generatore e --gen-test

**Files:**
- Create: `src/gen/gen_runner.h`, `src/gen/gen_runner.c`, `tests/fake-gen.sh`
- Modify: `src/core/game_types.h`, `src/tests/game_tests.{h,c}`, `src/app/app.c`, `Makefile`

**Interfaces:**
- Consumes: formato progresso `fase|percento|messaggio` (Task 4).
- Produces per il Task 10:

```c
typedef enum GenRunnerState { GEN_RUNNER_IDLE, GEN_RUNNER_RUNNING, GEN_RUNNER_SUCCEEDED, GEN_RUNNER_FAILED } GenRunnerState;
typedef struct GenRunner { GenRunnerState state; GenProgress progress; long pid; double startTime; double timeoutSec; char progressPath[256]; } GenRunner;
bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed, double timeoutSec, const char *progressPath);
void GenRunnerUpdate(GenRunner *runner);   /* da chiamare una volta a frame */
void GenRunnerCancel(GenRunner *runner);
```

  e `GenProgress { char phase[32]; int percent; char message[96]; }` in `game_types.h`. Flag del gioco: `--gen-test` (exit 0/6, senza finestra). Su Windows `GenRunnerStart` ritorna sempre `false` (stub dietro guard).

- [ ] **Step 1: Scrivi il finto generatore `tests/fake-gen.sh`**

```bash
#!/usr/bin/env bash
# Finto melting-gen per collaudare src/gen/gen_runner senza modelli.
# FAKE_GEN_MODE: ok (default) | fail | hang.  FAKE_GEN_OUT: cartella output.
out="${FAKE_GEN_OUT:-generated}"
mkdir -p "$out"
prog() {
  printf '%s|%s|%s\n' "$1" "$2" "$3" > "$out/gen_progress.tmp"
  mv "$out/gen_progress.tmp" "$out/gen_progress.txt"
}
case "${FAKE_GEN_MODE:-ok}" in
  hang)
    prog carico-modello 10 "finto caricamento infinito"
    sleep 30
    exit 0
    ;;
  fail)
    prog errore 100 "errore simulato"
    exit 3
    ;;
  *)
    prog carico-modello 30 "finto caricamento"
    sleep 0.2
    prog genero 70 "finta generazione"
    sleep 0.2
    prog fine 100 "finto manifest pronto"
    exit 0
    ;;
esac
```

```bash
chmod +x tests/fake-gen.sh
```

- [ ] **Step 2: Aggiungi `GenProgress` a `src/core/game_types.h`** (vicino a `UiLayout`)

```c
typedef struct GenProgress {
    char phase[32];
    int percent;
    char message[96];
} GenProgress;
```

- [ ] **Step 3: Scrivi il test — `GenRunnerSelfTest` + flag `--gen-test`**

In `src/tests/game_tests.h`:

```c
bool GenRunnerSelfTest(void);
```

In `src/tests/game_tests.c` (in fondo):

```c
#ifndef _WIN32
#include "gen/gen_runner.h"

#include <stdlib.h>
#include <time.h>

static bool GenRunnerWait(GenRunner *runner, double maxSeconds)
{
    for (int i = 0; i < (int)(maxSeconds*100.0); i++)
    {
        GenRunnerUpdate(runner);
        if (runner->state != GEN_RUNNER_RUNNING) return true;
        struct timespec ts = { 0, 10L*1000L*1000L };
        nanosleep(&ts, NULL);
    }
    return false;
}

bool GenRunnerSelfTest(void)
{
    const char *cmd = "tests/fake-gen.sh";
    GenRunner runner;
    setenv("FAKE_GEN_OUT", "generated", 1);

    setenv("FAKE_GEN_MODE", "ok", 1);
    if (!GenRunnerStart(&runner, cmd, 1, 10.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 10.0) || runner.state != GEN_RUNNER_SUCCEEDED) return false;
    if (runner.progress.percent != 100) return false;

    setenv("FAKE_GEN_MODE", "fail", 1);
    if (!GenRunnerStart(&runner, cmd, 2, 10.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 10.0) || runner.state != GEN_RUNNER_FAILED) return false;

    setenv("FAKE_GEN_MODE", "hang", 1);   /* timeout: 2s contro uno sleep 30 */
    if (!GenRunnerStart(&runner, cmd, 3, 2.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 8.0) || runner.state != GEN_RUNNER_FAILED) return false;

    setenv("FAKE_GEN_MODE", "hang", 1);   /* annullamento esplicito */
    if (!GenRunnerStart(&runner, cmd, 4, 30.0, "generated/gen_progress.txt")) return false;
    GenRunnerCancel(&runner);
    if (runner.state != GEN_RUNNER_FAILED) return false;

    return true;
}
#else
bool GenRunnerSelfTest(void)
{
    return true;   /* la generazione in-game non esiste su Windows */
}
#endif
```

In `src/app/app.c`: flag `bool genTest = false;`, parsing `if (strcmp(argv[i], "--gen-test") == 0) genTest = true;`, e SUBITO DOPO il ciclo di parsing (prima di `SetConfigFlags`/`InitWindow` — questo test non apre finestre):

```c
    if (genTest)
    {
        bool ok = GenRunnerSelfTest();
        printf("Gen runner test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 6;
    }
```

Nel Makefile aggiungi al target `test` la riga:

```make
	./$(GAME_BIN) --gen-test
```

- [ ] **Step 4: Verifica che il test fallisca (link)**

```bash
make test
```
Expected: FAIL a link time — `GenRunnerStart` non esiste ancora.

- [ ] **Step 5: Scrivi `src/gen/gen_runner.h`**

```c
#ifndef MELTING_RUN_GEN_RUNNER_H
#define MELTING_RUN_GEN_RUNNER_H

#include "core/game_types.h"

typedef enum GenRunnerState {
    GEN_RUNNER_IDLE,
    GEN_RUNNER_RUNNING,
    GEN_RUNNER_SUCCEEDED,
    GEN_RUNNER_FAILED
} GenRunnerState;

typedef struct GenRunner {
    GenRunnerState state;
    GenProgress progress;
    long pid;
    double startTime;
    double timeoutSec;
    char progressPath[256];
} GenRunner;

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath);
void GenRunnerUpdate(GenRunner *runner);
void GenRunnerCancel(GenRunner *runner);

#endif
```

- [ ] **Step 6: Scrivi `src/gen/gen_runner.c`**

```c
#include "gen/gen_runner.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath)
{
    (void)command; (void)seed; (void)timeoutSec; (void)progressPath;
    memset(runner, 0, sizeof(*runner));
    runner->state = GEN_RUNNER_FAILED;
    return false;   /* su Windows la generazione resta esterna (.bat), come oggi */
}

void GenRunnerUpdate(GenRunner *runner) { (void)runner; }
void GenRunnerCancel(GenRunner *runner) { (void)runner; }

#else

#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

static void ReadProgress(GenRunner *runner)
{
    FILE *f = fopen(runner->progressPath, "r");
    if (!f) return;
    char line[192];
    if (fgets(line, sizeof(line), f))
    {
        char phase[32] = { 0 };
        int percent = 0;
        char message[96] = { 0 };
        if (sscanf(line, "%31[^|]|%d|%95[^\n]", phase, &percent, message) >= 2)
        {
            snprintf(runner->progress.phase, sizeof(runner->progress.phase), "%s", phase);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            runner->progress.percent = percent;
            snprintf(runner->progress.message, sizeof(runner->progress.message), "%s", message);
        }
    }
    fclose(f);
}

bool GenRunnerStart(GenRunner *runner, const char *command, unsigned int seed,
                    double timeoutSec, const char *progressPath)
{
    memset(runner, 0, sizeof(*runner));
    snprintf(runner->progressPath, sizeof(runner->progressPath), "%s", progressPath);
    remove(progressPath);

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    pid_t pid = fork();
    if (pid < 0)
    {
        runner->state = GEN_RUNNER_FAILED;
        return false;
    }
    if (pid == 0)
    {
        execl(command, command, "--seed", seedText, (char *)NULL);
        _exit(127);
    }
    runner->pid = (long)pid;
    runner->state = GEN_RUNNER_RUNNING;
    runner->startTime = NowSeconds();
    runner->timeoutSec = timeoutSec;
    snprintf(runner->progress.phase, sizeof(runner->progress.phase), "avvio");
    snprintf(runner->progress.message, sizeof(runner->progress.message), "avvio del generatore");
    return true;
}

void GenRunnerUpdate(GenRunner *runner)
{
    if (runner->state != GEN_RUNNER_RUNNING) return;
    ReadProgress(runner);
    int status = 0;
    pid_t done = waitpid((pid_t)runner->pid, &status, WNOHANG);
    if (done == (pid_t)runner->pid)
    {
        runner->state = (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            ? GEN_RUNNER_SUCCEEDED : GEN_RUNNER_FAILED;
        return;
    }
    if (NowSeconds() - runner->startTime > runner->timeoutSec)
    {
        GenRunnerCancel(runner);
        snprintf(runner->progress.message, sizeof(runner->progress.message), "tempo scaduto");
    }
}

void GenRunnerCancel(GenRunner *runner)
{
    if (runner->state != GEN_RUNNER_RUNNING) return;
    kill((pid_t)runner->pid, SIGTERM);
    for (int i = 0; i < 20; i++)
    {
        if (waitpid((pid_t)runner->pid, NULL, WNOHANG) == (pid_t)runner->pid)
        {
            runner->state = GEN_RUNNER_FAILED;
            return;
        }
        struct timespec ts = { 0, 50L*1000L*1000L };
        nanosleep(&ts, NULL);
    }
    kill((pid_t)runner->pid, SIGKILL);
    waitpid((pid_t)runner->pid, NULL, 0);
    runner->state = GEN_RUNNER_FAILED;
}

#endif
```

- [ ] **Step 7: Esegui i test**

```bash
make test
```
Expected: i 4 test storici + `Gen runner test: ok`. La sezione hang impiega ~2-3 secondi (timeout).

- [ ] **Step 8: Commit**

```bash
git add src/gen src/core/game_types.h src/tests src/app/app.c tests/fake-gen.sh Makefile
git commit -m "feat: gen_runner process module with fake-generator self test"
```

---

### Task 10: Stato APP_GENERATING, overlay di caricamento, flag --generate

**Files:**
- Modify: `src/core/game_types.h` (enum `AppMode`), `src/app/app.c`, `src/render/game_renderer.h`, `src/render/game_renderer.c`

**Interfaces:**
- Consumes: `GenRunner*` (Task 9), `GameResetRun` (ricarica il manifest da `generated/current_run.txt`), `GameSetMessage` (`game_internal.h`).
- Produces: flag `--generate` (abilita la generazione in-game) e `--gen-cmd PATH` (default `bin/melting-gen`; con `tests/fake-gen.sh` si prova il flusso senza modelli). Nuova firma renderer: `void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress);` (ultimo parametro `NULL` fuori da APP_GENERATING). Timeout generazione: 180 secondi.
- Regola d'oro: `GameResetRun` azzera l'intero `Game` (incluso il messaggio a schermo) — chiamare SEMPRE prima `GameResetRun`, poi `GameSetMessage`.

- [ ] **Step 1: Aggiungi lo stato a `src/core/game_types.h`**

```c
typedef enum AppMode {
    APP_MENU,
    APP_PLAY,
    APP_PAUSE,
    APP_GENERATING
} AppMode;
```

- [ ] **Step 2: Estendi `src/app/app.c`**

Include aggiuntivi in testa: `#include "game/game_internal.h"`, `#include "gen/gen_runner.h"`, `#include <time.h>`. Sopra `UpdateApp` definisci:

```c
typedef struct AppGen {
    bool enabled;
    const char *command;
    GenRunner runner;
} AppGen;

static bool AppStartGeneration(AppGen *gen)
{
    unsigned int seed = (unsigned int)time(NULL);
    return GenRunnerStart(&gen->runner, gen->command, seed, 180.0, "generated/gen_progress.txt");
}
```

Cambia la firma di `UpdateApp` in:

```c
static bool UpdateApp(Game *game, AppMode *mode, UiLayout layout, float dt, AppGen *gen)
```

Nel ramo `APP_MENU`, sostituisci il blocco `KEY_ENTER || KEY_SPACE` con:

```c
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            if (gen->enabled && AppStartGeneration(gen)) *mode = APP_GENERATING;
            else
            {
                GameResetRun(game);
                if (gen->enabled) GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
                *mode = APP_PLAY;
            }
        }
```

Nel ramo `APP_PAUSE`, sostituisci il blocco `KEY_R` con:

```c
        if (IsKeyPressed(KEY_R))
        {
            if (gen->enabled && AppStartGeneration(gen)) *mode = APP_GENERATING;
            else
            {
                GameResetRun(game);
                *mode = APP_PLAY;
            }
        }
```

Dopo il ramo `APP_PAUSE` e prima della gestione di `APP_PLAY`, aggiungi il ramo nuovo:

```c
    if (*mode == APP_GENERATING)
    {
        GenRunnerUpdate(&gen->runner);
        if (IsKeyPressed(KEY_ESCAPE))
        {
            GenRunnerCancel(&gen->runner);
            *mode = APP_MENU;
            return false;
        }
        if (gen->runner.state == GEN_RUNNER_SUCCEEDED)
        {
            GameResetRun(game);
            *mode = APP_PLAY;
        }
        else if (gen->runner.state == GEN_RUNNER_FAILED)
        {
            GameResetRun(game);
            GameSetMessage(game, "Generazione fallita: uso i contenuti di riserva");
            *mode = APP_PLAY;
        }
        GameUpdateParticles(game, dt);
        return false;
    }
```

Nel ramo finale (`APP_PLAY`), PRIMA della chiamata a `GameUpdate`, intercetta la R quando la generazione è attiva (altrimenti la gestisce `GameUpdate` come oggi):

```c
    if (gen->enabled && IsKeyPressed(KEY_R))
    {
        if (AppStartGeneration(gen)) *mode = APP_GENERATING;
        else
        {
            GameResetRun(game);
            GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
        }
        return false;
    }
```

In `AppRun`: aggiungi il parsing dei flag e il contesto (prima del ciclo di parsing esistente):

```c
    AppGen gen = { 0 };
    gen.command = "bin/melting-gen";
```

e nel ciclo:

```c
        if (strcmp(argv[i], "--generate") == 0) gen.enabled = true;
        if (strcmp(argv[i], "--gen-cmd") == 0 && i + 1 < argc) gen.command = argv[++i];
```

Aggiorna le due chiamate nel loop principale:

```c
        if (UpdateApp(&game, &appMode, layout, dt, &gen)) break;
        RendererDrawApp(&game, gameCanvas, appMode, screenshotTest && !screenshotDone,
                        appMode == APP_GENERATING ? &gen.runner.progress : NULL);
```

- [ ] **Step 3: Overlay nel renderer**

In `src/render/game_renderer.h` cambia la firma:

```c
void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress);
```

In `src/render/game_renderer.c`, prima di `RendererDrawApp` aggiungi:

```c
static void DrawGeneratingOverlay(const Game *game, const GenProgress *progress)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, GameColorWithAlpha(BLACK, 200));
    Rectangle box = { sw*0.5f - 320.0f, sh*0.5f - 130.0f, 640.0f, 260.0f };
    DrawRectangleRec(box, (Color){ 20, 22, 29, 246 });
    DrawRectangleLinesEx(box, 2.0f, game->theme.accent2);
    const char *title = "GENERAZIONE RUN";
    DrawText(title, (int)(box.x + box.width*0.5f - MeasureText(title, 30)*0.5f), (int)box.y + 28, 30, RAYWHITE);

    const char *phase = progress ? progress->phase : "avvio";
    int percent = progress ? progress->percent : 0;
    DrawText(TextFormat("%s  %d%%", phase, percent), (int)box.x + 60, (int)box.y + 84, 18, game->theme.accent2);

    Rectangle bar = { box.x + 60.0f, box.y + 116.0f, box.width - 120.0f, 26.0f };
    DrawRectangleRec(bar, (Color){ 35, 38, 48, 255 });
    DrawRectangleRec((Rectangle){ bar.x, bar.y, bar.width*(float)percent/100.0f, bar.height }, game->theme.accent2);
    DrawRectangleLinesEx(bar, 2.0f, RAYWHITE);

    DrawText(progress ? progress->message : "", (int)box.x + 60, (int)box.y + 158, 16, RAYWHITE);
    DrawText("ESC annulla e torna al menu", (int)box.x + 60, (int)box.y + 206, 15, (Color){ 155, 163, 176, 255 });
}
```

e in fondo a `RendererDrawApp`, aggiorna firma e overlay:

```c
void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress)
{
    ...corpo invariato...
    if (mode == APP_MENU || mode == APP_PAUSE) DrawMenuOverlay(mode, game);
    if (mode == APP_GENERATING) DrawGeneratingOverlay(game, genProgress);
    ...screenshot e EndDrawing() invariati...
}
```

- [ ] **Step 4: Compila e regressione completa**

```bash
make && make test && make test-gen
```
Expected: tutto verde (il comportamento senza `--generate` deve essere IDENTICO a prima: stessi test, stesso menu).

- [ ] **Step 5: Prova end-to-end col finto generatore (nessun modello richiesto)**

```bash
./bin/melting_run_gpu --generate --gen-cmd tests/fake-gen.sh
```
Verifica a mano: INVIO dal menu → overlay «GENERAZIONE RUN» con barra che avanza (30% → 70% → 100%) → si entra in una run giocabile. Poi R durante il gioco → di nuovo overlay → nuova run. Poi `FAKE_GEN_MODE=fail ./bin/melting_run_gpu --generate --gen-cmd tests/fake-gen.sh` → INVIO → messaggio «Generazione fallita: uso i contenuti di riserva» e run comunque giocabile. Infine ESC durante l'overlay → torna al menu.

- [ ] **Step 6: Prova end-to-end col modello vero**

```bash
make run-gen
```
(nel Makefile `run-gen` è già `./$(GAME_BIN) --generate`). INVIO → barra reale (caricamento modello 0-60%, generazione 62-92%, scrittura) → run con contenuti nuovi. Verifica nel pannello sinistro «Fonte: LLM cache» e temi diversi a ogni R. Tempo atteso: entro il budget misurato nel Task 8.

- [ ] **Step 7: Commit**

```bash
git add src/core/game_types.h src/app/app.c src/render/game_renderer.h src/render/game_renderer.c
git commit -m "feat: in-game generation state with progress overlay and --generate flag"
```

---

### Task 11: Documentazione e AGENTS.md dual-platform

**Files:**
- Modify: `AGENTS.md` (riscrittura completa), `docs/LOCAL_REFERENCES.md` (riscrittura completa), `README.md` (nuova sezione), `docs/README.md` (righe indice)

- [ ] **Step 1: Riscrivi `AGENTS.md`** con questo contenuto completo:

```markdown
# Istruzioni di progetto

## Piattaforme

- **Linux (principale):** build con `make`; dipendenze native in `deps/` via
  `scripts/setup-deps.sh` (raylib 6.0 e llama.cpp b9979, tag fissati); modelli
  GGUF in `models/` via `scripts/download-models.sh`.
- **Windows (conservata):** gli script `.bat` storici con MinGW restano e non
  vanno rimossi. Il codice specifico Linux vive dietro guard di piattaforma
  (`src/gen/gen_runner.c`).

## Struttura obbligatoria

- Mantieni `src/main.c` come punto di ingresso minimo: deve chiamare soltanto `AppRun`.
- Inserisci costanti, enum e strutture dati condivise in `src/core/game_types.h`.
- Ogni nuova responsabilità deve avere una cartella dedicata sotto `src` e, quando serve un'API, una coppia `.h`/`.c`.
- Non aggiungere nuove funzioni di gameplay a `main.c`.
- Usa `src/game/game_internal.h` soltanto per collaborazioni interne tra moduli. Le API pubbliche restano nei rispettivi header.
- Evita simboli globali generici: usa i prefissi del modulo (`Game`, `World`, `Combat`, `Entities`, `ScriptVm`, `Renderer`, `Ui`, `GenRunner`; `Gen` dentro `tools/melting-gen`).
- Mantieni il motore C indipendente da rete, chiavi API e modelli AI. Il runtime legge soltanto file locali già validati in `generated/`. Solo `bin/melting-gen` linka llama.cpp e cJSON; il binario del gioco no.

## Responsabilità dei moduli

- `src/app`: ciclo applicativo, finestra, modalità menu/gioco/pausa/generazione.
- `src/assets`: caricamento e rilascio delle risorse Raylib.
- `src/content`: manifest e contenuti della run.
- `src/core`: tipi, costanti e funzioni matematiche condivise.
- `src/game`: orchestrazione dello stato di gioco.
- `src/gameplay`: entità, combattimento, oggetti e mini-VM.
- `src/gen`: ciclo di vita del processo melting-gen (avvio, progresso, timeout, annullamento). Nessuna logica di gioco.
- `src/render`: rendering del gioco e dell'interfaccia.
- `src/tests`: test interni eseguibili da riga di comando.
- `src/world`: stanze, mappe, transizioni e ricompense.
- `tools/melting-gen`: generatore locale (llama.cpp Vulkan + grammatica GBNF + validatore + fallback deterministico). Scrive gli stessi file del sidecar Node.

## Verifiche obbligatorie

Dopo modifiche al codice C, su Linux:

```bash
make test        # script/portal/smoke/screenshot/gen-runner
make test-gen    # determinismo fallback, coerenza grammatica, corpus JSON rotti
```

Chi modifica `tools/melting-gen/run.gbnf`, i prompt o `gen_validate.c` riesegue
`make test-gen`, più `make test-llm` se un modello è scaricato. Su Windows
valgono le verifiche `.bat` storiche del README.

## Documentazione e sicurezza

- Conserva appunti, decisioni e guide in `docs/`; spec e piani in `docs/superpowers/`.
- Mantieni `README.md` alla radice per la pagina iniziale GitHub.
- Non versionare `.env`, chiavi API, modelli, `deps/`, binari, log o contenuti generati.
- Non introdurre Raygui, Lua o nuovi backend AI senza una funzione concreta, un
  confine di modulo e test pertinenti. La roadmap delle fasi è in
  `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`.
```

(rispetto alla versione precedente: via i riferimenti a `codebase-memory-mcp`, che esisteva solo sulla macchina Windows.)

- [ ] **Step 2: Riscrivi `docs/LOCAL_REFERENCES.md`** con questo contenuto completo:

```markdown
# Repository e riferimenti locali

## Dentro la repo (`deps/`, ignorata da git, creata da scripts/setup-deps.sh)

- `deps/raylib` — raylib 6.0 a tag fissato, build statica linkata dal gioco.
- `deps/llama.cpp` — llama.cpp b9979 con backend Vulkan, linkata SOLO da
  `tools/melting-gen`. La build include `build/bin/test-gbnf-validator`,
  usato da `make test-gen` per la grammatica.

## Vendorate (committate nella repo)

- `tools/melting-gen/vendor/cJSON.{c,h}` — cJSON v1.7.19 (MIT).

## Riferimenti esterni (da consultare, non dipendenze)

- Raygui — https://github.com/raysan5/raygui — candidata per la fase 4 (UI).
  Se integrata: `RAYGUI_IMPLEMENTATION` in un solo `.c` in un futuro `src/ui`.
- IsaacDocs — https://wofsauge.github.io/IsaacDocs/rep/ — riferimento
  concettuale per callback e pattern della futura sandbox Lua (fase 3).
  Non è codice da copiare né una specifica del nostro motore.
- Generazione dungeon di Isaac — articolo di BorisTheBrave:
  https://www.boristhebrave.com/2020/09/12/dungeon-generation-in-binding-of-isaac/
  Algoritmi da reinterpretare per il modello dati di `src/world`.
- gguf-tools — https://github.com/antirez/gguf-tools — SOLO ispezione e
  manipolazione di file GGUF (niente inferenza); utile come codice leggibile
  per capire il formato. L'inferenza è llama.cpp.
```

- [ ] **Step 3: Aggiungi la sezione Linux al `README.md`** (subito dopo la sezione «Avvio rapido» esistente):

````markdown
## Avvio rapido su Linux

```bash
scripts/setup-deps.sh        # una tantum: apt + raylib + llama.cpp (chiede la password)
make                         # compila gioco + melting-gen
make run                     # gioca (manifest esistente o fallback interno)
scripts/download-models.sh   # scarica i modelli GGUF (~5,8 GB, riprendibile)
make run-gen                 # nuova run generata in locale: INVIO dal menu, R in gioco
make test && make test-gen   # test senza modello
make test-llm                # generazione reale + tempi (vedi docs/BENCHMARKS.md)
```

La generazione locale usa Qwen2.5-Coder (GGUF Q4_K_M, Apache 2.0) con llama.cpp
su backend Vulkan e una grammatica GBNF che rende impossibile un JSON
malformato; senza modelli scaricati il gioco ripiega sempre sul generatore
deterministico interno. Design e roadmap in `docs/superpowers/specs/`.
````

- [ ] **Step 4: Aggiorna l'indice `docs/README.md`** aggiungendo queste righe all'elenco esistente:

```markdown
- [superpowers/specs/2026-07-13-local-llm-linux-design.md](superpowers/specs/2026-07-13-local-llm-linux-design.md): spec del ciclo «build Linux + LLM locale».
- [superpowers/plans/2026-07-13-linux-local-llm.md](superpowers/plans/2026-07-13-linux-local-llm.md): piano di implementazione del ciclo.
- [BENCHMARKS.md](BENCHMARKS.md): tempi misurati di melting-gen sulla macchina di riferimento.
```

- [ ] **Step 5: Commit**

```bash
git add AGENTS.md docs/LOCAL_REFERENCES.md README.md docs/README.md
git commit -m "docs: dual-platform AGENTS.md, Linux quickstart, updated local references"
```

---

### Task 12: Verifica finale del ciclo

Criteri di successo della spec (§10), tutti in una passata.

- [ ] **Step 1: Build pulita**

```bash
make clean && make
```
Expected: gioco e melting-gen compilano da zero senza warning nuovi.

- [ ] **Step 2: Tutti i test**

```bash
make test && make test-gen && make test-llm
```
Expected: tutto verde.

- [ ] **Step 3: Percorso completo a mano**

```bash
make run-gen
```
INVIO → generazione reale entro il budget (~2 min max) → run giocabile con contenuti nuovi → R → seconda generazione → ESC durante la barra → menu. Poi:

```bash
nvidia-smi 2>/dev/null || true; ls -la generated/ logs/
```
Verifica che `melting-gen` non sia più in esecuzione a run avviata (`pgrep melting-gen` → vuoto: VRAM libera).

- [ ] **Step 4: Igiene della repo**

```bash
git status --short
```
Expected: vuoto (niente file generati/deps/modelli tracciati). Cronologia: `git log --oneline` mostra i commit dei task in ordine.

- [ ] **Step 5: Chiusura**

Usa la skill `superpowers:finishing-a-development-branch` per decidere insieme all'utente come integrare il branch `linux-local-llm` (merge in `main`, push, eventuale PR).

---

## Note per l'esecutore

- **Password:** solo `scripts/setup-deps.sh` la richiede (apt). Avvisare l'utente PRIMA, ogni volta.
- **Ordine dei task:** 1→12 rigorosamente; ogni task lascia la repo compilante e testata.
- **Se un'API di llama.cpp b9979 non corrisponde al piano** (campo/firma rinominati): la fonte di verità è `deps/llama.cpp/include/llama.h` del tag scaricato — adegua il punto d'uso, non cambiare tag.
- **Se un test del piano fallisce per un valore atteso sbagliato** (es. un clamp diverso): la fonte di verità è `llm/run_content.mjs` — il C deve replicare il Node, e il test si corregge solo se il Node fa davvero un'altra cosa.
- **Screenshot/smoke test** aprono finestre: servono sessione grafica; non lanciarli via ssh senza display.




