#!/usr/bin/env bash
# Generazione reale con modello. Variabili: MODEL, NGL, SEED.
set -euo pipefail
cd "$(dirname "$0")/.."

# I test che aprono una finestra girano su display virtuale quando disponibile
# (vedi la variabile TEST_RUNNER nel Makefile): su una sessione Wayland bloccata
# il gioco resterebbe appeso al primo SwapBuffers.
XVFB_RUNTIME="$PWD/.xvfb-runtime"
if command -v xvfb-run >/dev/null 2>&1; then
  mkdir -p "$XVFB_RUNTIME" && chmod 700 "$XVFB_RUNTIME"
  GAME_RUN=(env -u WAYLAND_DISPLAY "XDG_RUNTIME_DIR=$XVFB_RUNTIME"
            xvfb-run -a -s "-screen 0 1920x1080x24 +extension GLX +render")
else
  GAME_RUN=()
fi

MODEL="${MODEL:-models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf}"
NGL="${NGL:-99}"
SEED="${SEED:-31337}"
[ -f "$MODEL" ] || { echo "Modello mancante: $MODEL — esegui scripts/download-models.sh"; exit 1; }

bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$SEED" --out generated
grep -q "^source=local:" generated/current_run.txt
grep -q "^floor5.item3.script=" generated/current_run.txt
# --manifest-test (fase 3a-L3) carica anche gli script Lua presenti nel
# manifest in una sandbox vera e asserisce che compilino: vedi
# src/tests/game_tests.c, GameManifestTest.
"${GAME_RUN[@]}" bin/melting_run_gpu --manifest-test
echo "--- ultima riga di log (tempi e tok/s) ---"
grep "^\[.*\] ok: model=" logs/melting-gen.log | tail -1

echo "--- riepilogo Lua (fase 3a-L3): quanti dei 15 oggetti hanno preso uno script funzionante ---"
grep "lua: riepilogo run" logs/melting-gen.log | tail -1

echo "--- script Lua scritti in generated/scripts/ ---"
ls -1 generated/scripts/*.lua 2>/dev/null || echo "(nessuno: tutti gli oggetti sono ripiegati sulla mini-VM)"

echo "TEST-LLM: OK"
