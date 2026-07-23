#!/usr/bin/env bash
# Test della sandbox Lua (src/script/script_sandbox.c). Vedi la spec:
# docs/engineering/specs/2026-07-13-lua-sandbox-design.md (sezioni 2,3,4,9).
#
# A differenza di test-gen.sh/test-sprites.sh, qui non serve xvfb-run: i due
# flag usati sotto (--script-sandbox-test, --script-determinism-test) girano
# PRIMA che il gioco apra una finestra (vedi src/app/app.c, come --gen-test),
# quindi nessuna delle due chiamate tocca raylib/GLX/X11.
set -euo pipefail
cd "$(dirname "$0")/.."

GAME=bin/melting_run_gpu

echo "-- guardia: mai luaL_loadbuffer/luaL_loadstring/luaL_dostring in src/ (bytecode non fidato, mai verificato da Lua) --"
if grep -rnE 'luaL_loadbuffer\(|luaL_loadstring\(|luaL_dostring\(' src/; then
  echo "FALLITO: src/ deve usare solo luaL_loadbufferx(...,\"t\")"; exit 1
fi

echo "-- il binario del gioco linka Lua ma non llama.cpp/stable-diffusion.cpp/cJSON --"
NM_OUT=$(mktemp)
trap 'rm -f "$NM_OUT"' EXIT
nm "$GAME" 2>/dev/null > "$NM_OUT"
grep -qiE 'lua_newstate|luaL_loadbufferx' "$NM_OUT"
! grep -qiE 'llama_model_load|llama_decode|new_sd_ctx|cJSON_Parse' "$NM_OUT"

echo "-- nove fughe, ciascuna eseguita per davvero (--script-sandbox-test) --"
# ScriptSandboxSelfTest (src/tests/script_sandbox_tests.c) esegue gli
# escape 1-8 e 10; ognuno gira lo snippet ostile vero attraverso l'API
# pubblica e verifica di essere stato fermato, non solo che una stringa sia
# assente da un file. Un timeout duro qui sotto e' l'ultima rete di
# sicurezza dello SCRIPT DI TEST: se una fuga sfuggisse davvero (bug nella
# sandbox), il binario girerebbe all'infinito invece di fallire con un
# messaggio, ed e' esattamente cosi' che e' stato scoperto e corretto il
# bug reale di questo modulo durante lo sviluppo (luaopen_base installava
# pcall nell'_ENV vero: "while true do pcall(f) end" girava per sempre).
SANDBOX_OUT=$(timeout -s KILL 30 "$GAME" --script-sandbox-test)
echo "$SANDBOX_OUT"
echo "$SANDBOX_OUT" | grep -q "^Lua sandbox test: ok$"

echo "-- determinismo (escape 9): stesso seed, DUE PROCESSI separati, stesso output --"
DET_A=$(timeout -s KILL 10 "$GAME" --script-determinism-test --seed 12345)
DET_B=$(timeout -s KILL 10 "$GAME" --script-determinism-test --seed 12345)
if [ "$DET_A" != "$DET_B" ]; then
  echo "FALLITO: stesso seed (12345), due processi, output diverso"
  echo "  processo A: $DET_A"
  echo "  processo B: $DET_B"
  exit 1
fi
echo "   pairs()+rng() osservati: $DET_A"

echo "-- seed diverso = sequenza diversa --"
DET_C=$(timeout -s KILL 10 "$GAME" --script-determinism-test --seed 999)
if [ "$DET_A" = "$DET_C" ]; then
  echo "FALLITO: semi diversi (12345 e 999) hanno prodotto lo stesso output"
  exit 1
fi

echo "-- log: ogni kill finisce in logs/script-sandbox.log con il suo motivo --"
[ -s logs/script-sandbox.log ]
grep -q "tetto di memoria superato" logs/script-sandbox.log
grep -q "budget di istruzioni superato\|errore a runtime" logs/script-sandbox.log

# ============================================================
# Fase 3a-L2: API di gioco a handle + callback degli oggetti +
# sistema delle cache (src/script/script_api.c, src/script/script_items.c).
# Vedi src/tests/script_items_tests.c per il dettaglio di ciascun test:
# non richiede raylib/xvfb per lo stesso motivo dei test sopra (gira PRIMA
# di InitWindow, vedi src/app/app.c).
# ============================================================
echo "-- API a handle + callback oggetti + sistema delle cache (--script-items-test) --"
ITEMS_OUT=$(timeout -s KILL 30 "$GAME" --script-items-test)
echo "$ITEMS_OUT"
echo "$ITEMS_OUT" | grep -q "^Script items test: ok$"

# ============================================================
# M6b-2 (DEC-037): il trait UNICO del personaggio generato
# (src/script/script_character.c) -- UNA sandbox dietro la STESSA facciata
# di ScriptItems* (combat.c continua a non vederla mai). Vedi
# src/tests/script_character_tests.c: lifecycle, ordine del ricalcolo
# (base -> trait -> oggetti, valori noti), sopravvivenza a un reset/una
# riselezione, fallimento di caricamento/compilazione -> inattivo. Stesso
# motivo dei due blocchi sopra: gira PRIMA di InitWindow.
# ============================================================
echo "-- trait del personaggio generato: lifecycle + ordine del ricalcolo + reset survival (--script-character-test) --"
CHARACTER_OUT=$(timeout -s KILL 30 "$GAME" --script-character-test)
echo "$CHARACTER_OUT"
echo "$CHARACTER_OUT" | grep -q "^Script character test: ok$"

echo "TEST-SCRIPT: OK"
