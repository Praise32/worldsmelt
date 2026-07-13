CC := gcc

RAYLIB_DIR := deps/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/build/raylib/libraylib.a

# Lua 5.5.0 (fase 3, sandbox script): compilata con la propria Makefile
# ufficiale (target "linux") in scripts/setup-deps.sh, come raylib/llama.cpp/
# stable-diffusion.cpp qui sotto. Statica, MIT: il binario del gioco la
# linka (vedi docs/LICENZE.md), ma NON linka mai llama.cpp/stable-diffusion.cpp/
# cJSON (vedi i commenti su GEN_LIBS/SPRITES_LIBS piu' sotto e AGENTS.md).
LUA_DIR := deps/lua-5.5.0
LUA_LIB := $(LUA_DIR)/src/liblua.a

CFLAGS := -std=c99 -Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
GAME_CFLAGS := $(CFLAGS) -Isrc -I$(RAYLIB_DIR)/src -I$(LUA_DIR)/src
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
GEN_EXTRA_SRC := src/core/game_math.c src/script/script_sandbox.c

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

.PHONY: all game gen sprites run run-gen run-gen-fast test test-gen test-sprites test-script test-llm clean

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
	$(TEST_RUNNER) ./$(GAME_BIN) --smoke-test
	$(TEST_RUNNER) ./$(GAME_BIN) --screenshot-test
	$(TEST_RUNNER) ./$(GAME_BIN) --gen-test
	$(TEST_RUNNER) ./$(GAME_BIN) --atlas-fallback-test
	$(TEST_RUNNER) ./$(GAME_BIN) --layer-test

test-gen: all
	bash scripts/test-gen.sh

test-sprites: sprites
	bash scripts/test-sprites.sh

test-script: game
	bash scripts/test-script.sh

test-llm: all
	bash scripts/test-llm.sh

clean:
	rm -rf bin
