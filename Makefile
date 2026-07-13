CC ?= gcc

RAYLIB_DIR := deps/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/build/raylib/libraylib.a

CFLAGS := -std=c99 -Wall -Wextra -O2 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
GAME_CFLAGS := $(CFLAGS) -Isrc -I$(RAYLIB_DIR)/src
GAME_LIBS := $(RAYLIB_LIB) -lGL -lm -lpthread -ldl -lrt -lX11

GAME_SRC := $(shell find src -name '*.c')
GAME_BIN := bin/melting_run_gpu

.PHONY: all game run test clean

all: game

game: $(GAME_BIN)

$(GAME_BIN): $(GAME_SRC)
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
