CC := gcc

RAYLIB_DIR := deps/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/build/raylib/libraylib.a

CFLAGS := -std=c99 -Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
GAME_CFLAGS := $(CFLAGS) -Isrc -I$(RAYLIB_DIR)/src
GAME_LIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

GAME_SRC := $(shell find src -name '*.c')
GAME_HDR := $(shell find src -name '*.h')
GAME_BIN := bin/melting_run_gpu

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

.PHONY: all game gen run test clean

all: game gen

game: $(GAME_BIN)

gen: $(GEN_BIN)

$(GAME_BIN): $(GAME_SRC) $(GAME_HDR)
	@mkdir -p bin logs generated
	$(CC) $(GAME_CFLAGS) $(GAME_SRC) $(GAME_LIBS) -o $@

$(GEN_BIN): $(GEN_SRC)
	@mkdir -p bin logs
	$(CC) $(GEN_CFLAGS) $(GEN_SRC) $(GEN_LIBS) -o $@

run: game
	./$(GAME_BIN)

test: game
	./$(GAME_BIN) --script-test
	./$(GAME_BIN) --portal-test
	./$(GAME_BIN) --smoke-test
	./$(GAME_BIN) --screenshot-test

clean:
	rm -rf bin
