CC := gcc

RAYLIB_DIR := deps/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/build/raylib/libraylib.a

# Lua 5.5.0 (fase 3, sandbox script): compilata con la propria Makefile
# ufficiale (target "linux") in scripts/setup-deps.sh, come raylib/llama.cpp/
# stable-diffusion.cpp qui sotto. Statica, MIT: il binario del gioco la
# linka (vedi docs/ai-production/licenze.md), ma NON linka mai llama.cpp/stable-diffusion.cpp/
# cJSON (vedi i commenti su GEN_LIBS/SPRITES_LIBS piu' sotto e AGENTS.md).
LUA_DIR := deps/lua-5.5.0
LUA_LIB := $(LUA_DIR)/src/liblua.a

CFLAGS := -std=c99 -Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
GAME_CFLAGS := $(CFLAGS) -Isrc -I$(RAYLIB_DIR)/src -Ideps/raygui -I$(LUA_DIR)/src
GAME_LIBS := $(RAYLIB_LIB) $(LUA_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

GAME_SRC := $(shell find src -name '*.c')
GAME_HDR := $(shell find src -name '*.h')
GAME_BIN := bin/melting_run_gpu

LLAMA_DIR := deps/llama.cpp
LLAMA_BUILD := $(LLAMA_DIR)/build

# Fase 3a-L3 (script Lua degli oggetti, gen_lua.c): melting-gen compila
# anche src/core/game_math.c e src/script/script_sandbox.c per riusare LA
# STESSA sandbox del gioco (stesso allowlist, stesso tetto di memoria, stesso
# budget di istruzioni) per il dry-run di ogni script prima che il gioco lo
# veda mai. I due file non toccano raylib (solo i tipi Vector2/Color del suo
# header, mai una sua funzione: nessun link a libraylib.a qui), quindi
# bastano -Isrc e l'header di raylib per compilarli; l'API di gioco vera
# (src/script/script_api.c) resta FUORI apposta, sostituita da uno stub
# senza Game* in gen_lua.c (vedi il commento li'). Per questo melting-gen
# linka anche Lua (statica, come il gioco): AGENTS.md, "melting-gen puo'
# linkare Lua e cJSON".
# src/core/shot_type.c (step C): la definizione dei tipi di colpo e la loro
# funzione di bilanciamento (ShotTypeBalance) sono UNA SOLA, condivisa fra gioco
# e generatore. melting-gen la compila per bilanciare i tipi che il modello
# inventa gia' mentre scrive il manifest; il gioco la ricompila per la propria
# rete di sicurezza al caricamento. Non tocca raylib (shot_type.h non lo include
# nemmeno: e' l'unico header di src/core/ che ne fa a meno, proprio per poter
# vivere dentro melting-gen senza trascinarsi dietro il gioco).
# src/core/character_type.c (M6b-1, DEC-014 prima fetta): come shot_type.c,
# le bande e il clamp del personaggio alternativo per-run sono UNA SOLA
# definizione condivisa fra gioco e generatore (mai due copie da tenere
# sincronizzate a mano). Non tocca raylib (character_type.h non lo include).
GEN_EXTRA_SRC := src/core/game_math.c src/core/shot_type.c src/core/enemy_type.c src/core/room_layout.c src/core/character_type.c src/script/script_sandbox.c

GEN_SRC := $(wildcard tools/melting-gen/*.c) $(wildcard tools/melting-gen/vendor/*.c) $(GEN_EXTRA_SRC)
GEN_HDR := $(wildcard tools/melting-gen/*.h) $(wildcard tools/melting-gen/vendor/*.h)
GEN_CFLAGS := $(CFLAGS) -Itools/melting-gen -Itools/melting-gen/vendor \
  -Isrc -I$(RAYLIB_DIR)/src -I$(LUA_DIR)/src \
  -I$(LLAMA_DIR)/include -I$(LLAMA_DIR)/ggml/include
GEN_LIBS := $(LLAMA_BUILD)/src/libllama.a \
  $(LLAMA_BUILD)/ggml/src/libggml.a \
  $(LLAMA_BUILD)/ggml/src/ggml-vulkan/libggml-vulkan.a \
  $(LLAMA_BUILD)/ggml/src/libggml-cpu.a \
  $(LLAMA_BUILD)/ggml/src/libggml-base.a \
  $(LUA_LIB) \
  -lvulkan -lgomp -lstdc++ -lpthread -lm -ldl
GEN_BIN := bin/melting-gen

# melting-sprites: post-processing + generazione via stable-diffusion.cpp
# (fase 2, S3). Non linka raylib ne' llama.cpp: llama.cpp e stable-diffusion.cpp
# vendorizzano due ggml incompatibili (sd.cpp usa il fork leejet/ggml), quindi
# melting-sprites e melting-gen restano DUE ESEGUIBILI SEPARATI, mai linkati
# insieme (vedi anche il commento in cima a tools/melting-sprites/sprite_sd.c).
# E' anche quello che serve per la VRAM: i due modelli non stanno insieme nei
# 6 GB della scheda di riferimento, ma i due processi si alternano e ognuno
# libera tutto quando esce.
SD_DIR := deps/stable-diffusion.cpp
SD_BUILD := $(SD_DIR)/build

SPRITES_SRC := $(wildcard tools/melting-sprites/*.c) $(wildcard tools/melting-sprites/vendor/*.c)
SPRITES_HDR := $(wildcard tools/melting-sprites/*.h) $(wildcard tools/melting-sprites/vendor/*.h)
SPRITES_CFLAGS := $(CFLAGS) -Itools/melting-sprites -Itools/melting-sprites/vendor \
  -I$(SD_DIR)/include -I$(SD_DIR)/ggml/include
SPRITES_LIBS := $(SD_BUILD)/libstable-diffusion.a \
  $(SD_BUILD)/ggml/src/libggml.a \
  $(SD_BUILD)/ggml/src/ggml-vulkan/libggml-vulkan.a \
  $(SD_BUILD)/ggml/src/libggml-cpu.a \
  $(SD_BUILD)/ggml/src/libggml-base.a \
  -lvulkan -lgomp -lstdc++ -lpthread -lm -ldl
SPRITES_BIN := bin/melting-sprites

# I test aprono una finestra. Su Wayland, se la sessione e' bloccata o la finestra
# non e' visibile, il compositor smette di consegnare frame e il gioco resta appeso
# al primo SwapBuffers. I test girano quindi su un display X11 virtuale quando
# xvfb-run e' disponibile: schermo a 24 bit (a 8 bit OpenGL non parte) e
# XDG_RUNTIME_DIR vuoto, altrimenti GLFW sceglie comunque il socket Wayland.
XVFB := $(shell command -v xvfb-run 2>/dev/null)
XVFB_RUNTIME := $(CURDIR)/.xvfb-runtime
ifeq ($(XVFB),)
TEST_RUNNER :=
else
TEST_RUNNER := env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=$(XVFB_RUNTIME) \
  $(XVFB) -a -s "-screen 0 1920x1080x24 +extension GLX +render"
endif

.PHONY: all game gen sprites run run-demo run-gen run-gen-fast test test-gen test-sprites test-script test-llm gen-metrics sprite-baseline benchmark model-comparison image-comparison docs-check docs-index docs-audit clean

all: game gen sprites

game: $(GAME_BIN)

gen: $(GEN_BIN)

sprites: $(SPRITES_BIN)

$(GAME_BIN): $(GAME_SRC) $(GAME_HDR)
	@mkdir -p bin logs generated
	$(CC) $(GAME_CFLAGS) $(GAME_SRC) $(GAME_LIBS) -o $@

$(GEN_BIN): $(GEN_SRC) $(GEN_HDR)
	@mkdir -p bin logs
	$(CC) $(GEN_CFLAGS) $(GEN_SRC) $(GEN_LIBS) -o $@

$(SPRITES_BIN): $(SPRITES_SRC) $(SPRITES_HDR)
	@mkdir -p bin
	$(CC) $(SPRITES_CFLAGS) $(SPRITES_SRC) $(SPRITES_LIBS) -o $@

run: game
	./$(GAME_BIN)

# Demo curata, senza nessuna generazione AI. Il binario del gioco non contiene i
# modelli per costruzione: melting-gen (testo) e melting-sprites (immagini) sono
# processi separati e il gioco non linka mai llama.cpp/stable-diffusion.cpp/cJSON
# (AGENTS.md). Senza --generate il gioco non li avvia nemmeno (gen.enabled resta
# falso, vedi AppStartGeneration/AppStartLazyGeneration in src/app/app.c), quindi
# qui si gioca solo con i contenuti curati e il fallback interno deterministico:
# nessun download di modelli, nessuna GPU, avvio immediato.
#
# Perche' una cartella di lavoro separata invece di lanciare il binario qui: il
# gioco legge generated/ RELATIVA alla cartella corrente, e generated/ e' persistente
# (make clean non la tocca). Se una vecchia run generata ha lasciato li' i suoi
# artefatti, la "demo curata" mostrerebbe contenuti AI: in particolare un
# generated/current_atlas.png residuo viene ripreso anche quando il manifest non
# c'e' piu' -- PreferPngAtlasIfFresh (src/content/run_content.c) confronta le date
# e GetFileModTime di un file assente vale 0, quindi il PNG vince sempre -- e l'HUD
# etichetterebbe la demo "Sprite locali (SD)" invece di "Atlas fallback".
# Cancellare quegli artefatti non e' la risposta giusta (butterebbe via il lavoro di
# un run-gen, sprite compresi): il target lancia invece il gioco in una cartella
# usa-e-getta con generated/ VUOTA, e ci collega assets/ (contenuti curati, sola
# lettura) e catalog/ (cosi' le run della demo finiscono comunque nel catalogo vero).
DEMO_DIR := build/demo

run-demo: game
	@rm -rf $(DEMO_DIR)
	@mkdir -p $(DEMO_DIR)/generated $(DEMO_DIR)/logs catalog
	@ln -s $(CURDIR)/assets $(DEMO_DIR)/assets
	@ln -s $(CURDIR)/catalog $(DEMO_DIR)/catalog
	cd $(DEMO_DIR) && $(CURDIR)/$(GAME_BIN)

run-gen: all
	./$(GAME_BIN) --generate

# Solo il testo (melting-gen): utile per iterare senza pagare gli ~85s di
# melting-sprites ad ogni run. Stessa cosa di run-gen ma con --no-sprites.
run-gen-fast: all
	./$(GAME_BIN) --generate --no-sprites

test: game
	@echo "-- guardia: mai luaL_loadbuffer/luaL_loadstring/luaL_dostring in src/ (caricano bytecode non verificato di default, vedi sandbox Lua sezione 2) --"
	@if grep -rnE 'luaL_loadbuffer\(|luaL_loadstring\(|luaL_dostring\(' src/; then \
		echo "FALLITO: src/ deve usare solo luaL_loadbufferx(...,\"t\"), mai le varianti che di default accettano bytecode"; \
		exit 1; \
	fi
	@mkdir -p $(XVFB_RUNTIME) && chmod 700 $(XVFB_RUNTIME)
	$(TEST_RUNNER) ./$(GAME_BIN) --script-test
	$(TEST_RUNNER) ./$(GAME_BIN) --portal-test
	$(TEST_RUNNER) ./$(GAME_BIN) --states-test
	$(TEST_RUNNER) ./$(GAME_BIN) --floor-zero-test
	$(TEST_RUNNER) ./$(GAME_BIN) --rooms-test
	$(TEST_RUNNER) ./$(GAME_BIN) --mouse-hit-test
	$(TEST_RUNNER) ./$(GAME_BIN) --rng-seed-test
	$(TEST_RUNNER) ./$(GAME_BIN) --run-timer-test
	$(TEST_RUNNER) ./$(GAME_BIN) --temp-health-test
	$(TEST_RUNNER) ./$(GAME_BIN) --obstacles-test
	$(TEST_RUNNER) ./$(GAME_BIN) --item-pool-test
	$(TEST_RUNNER) ./$(GAME_BIN) --economy-test
	$(TEST_RUNNER) ./$(GAME_BIN) --trials-test
	$(TEST_RUNNER) ./$(GAME_BIN) --arena-hub-test
	$(TEST_RUNNER) ./$(GAME_BIN) --fusion-test
	$(TEST_RUNNER) ./$(GAME_BIN) --discovery-test
	$(TEST_RUNNER) ./$(GAME_BIN) --audio-test
	$(TEST_RUNNER) ./$(GAME_BIN) --curated-content-test
	$(TEST_RUNNER) ./$(GAME_BIN) --art-atlas-test
	$(TEST_RUNNER) ./$(GAME_BIN) --catalog-test
	$(TEST_RUNNER) ./$(GAME_BIN) --catalog-screen-test
	$(TEST_RUNNER) ./$(GAME_BIN) --smoke-test
	$(TEST_RUNNER) ./$(GAME_BIN) --screenshot-test
	$(TEST_RUNNER) ./$(GAME_BIN) --gen-test
	$(TEST_RUNNER) ./$(GAME_BIN) --layout-test
	$(TEST_RUNNER) ./$(GAME_BIN) --atlas-fallback-test
	$(TEST_RUNNER) ./$(GAME_BIN) --layer-test
	$(TEST_RUNNER) ./$(GAME_BIN) --exit-confirm-light-modal-test
	$(TEST_RUNNER) ./$(GAME_BIN) --run-setup-mode-line-test
	$(TEST_RUNNER) ./$(GAME_BIN) --shot-forms-screenshot-test

test-gen: all
	bash scripts/test-gen.sh

test-sprites: sprites
	bash scripts/test-sprites.sh

test-script: game
	bash scripts/test-script.sh

test-llm: all
	bash scripts/test-llm.sh

# Metriche di generazione (validita' Lua + varieta' fra run): ~4-5 min a run
# col modello di riferimento (gemma-3-4b-it, DEC-140), 3 run di default.
# Vedi scripts/gen-metrics.sh e gen_metrics.py.
gen-metrics: all
	bash scripts/gen-metrics.sh

# Baseline sprite "Esperimento 0" (roadmap 16/07/2026, settimana 2): 15
# coppie tema/stile fisse x 2 seed fissi con la pipeline ATTUALE, senza
# alcun training -- il metro di paragone per ogni Style/Item LoRA futura.
# Vedi scripts/sprite-baseline.sh e docs/ai-production/dataset/baseline-prompts.txt.
sprite-baseline: sprites
	bash scripts/sprite-baseline.sh

# Benchmark della macchina (strumento diagnostico manuale, DEC-110): esegue in
# sequenza melting-gen --bench e melting-sprites --bench (mai insieme: VRAM) e
# scrive logs/benchmark.txt come report per chi sviluppa/misura l'hardware.
# Da DEC-110 non esiste piu' nessun tier di qualita' automatico: il gioco NON
# rilegge mai questo file, non lo linka a nessuna scelta di modello o
# dimensione sprite. I requisiti minimi del gioco completo sono quelli per
# far girare i modelli di riferimento (gemma-3-4b-it + SD1.5, DEC-140); hardware
# migliore rende solo la generazione piu' veloce, mai diversa. Vedi scripts/benchmark.sh.
benchmark: gen sprites
	bash scripts/benchmark.sh

# Suite di comparazione dei modelli candidati (decisione 22/07/2026, piano in
# docs/plans/active/model-comparison.md): NON fa parte di `make test` -- e'
# lunga (piu' modelli x piu' run vere, tutte in sequenza per la VRAM, mai in
# parallelo). Scarica i candidati con scripts/download-comparison-models.sh
# PRIMA di lanciare questo target (non e' una dipendenza make: i download
# sono grossi e riprendibili, non vanno rifatti a ogni chiamata). Vedi
# scripts/model-comparison.sh per l'elenco di argomenti/variabili.
model-comparison: gen
	bash scripts/model-comparison.sh

# Stessa suite, dominio immagini (estensione 22/07/2026 della missione,
# stessa cartella di piano): richiede un elenco esplicito nome:modello[:lora]
# (nessun default "tutti i checkpoint in models/" -- a differenza del testo,
# non ogni .ckpt/.safetensors in models/ e' un candidato valido, es. la LoRA
# LCM o il VAE TAESD condividono la cartella). NON lanciare mentre
# `model-comparison` sta ancora usando la GPU (mai due modelli GPU insieme).
# Vedi scripts/image-comparison.sh per l'elenco di argomenti/variabili.
image-comparison: sprites
	bash scripts/image-comparison.sh \
	  pixel-baseline:models/Public-Prompts-Pixel-Model.ckpt \
	  sd15-vanilla:models/sd15-vanilla-pruned-emaonly.safetensors \
	  pixelart-alt:models/pixelart-spritesheet-generator-v1.ckpt

# Knowledge base (docs/): indice derivato, verifica vincolante e report.
# Vedi docs/_meta/DOCUMENT-STANDARDS.md.
docs-index:
	python3 scripts/docs/build_knowledge_index.py --index

docs-check:
	python3 scripts/docs/build_knowledge_index.py --check

docs-audit:
	python3 scripts/docs/build_knowledge_index.py --audit

clean:
	rm -rf bin $(DEMO_DIR)
