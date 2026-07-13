#!/usr/bin/env bash
# Test di melting-gen senza modello LLM.
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
"${GAME_RUN[@]}" bin/melting_run_gpu --manifest-test

echo "-- coerenza writer C <-> grammatica GBNF --"
GBNF=deps/llama.cpp/build/bin/test-gbnf-validator
"$GEN" --fallback --seed 1 --out "$TMP/g" --emit-llm-json
"$GBNF" tools/melting-gen/run.gbnf "$TMP/g/llm_sample.json" | grep -q "is valid"

echo "-- la grammatica rifiuta un enum sbagliato --"
sed 's/"hat"/"hut"/; s/"eyes"/"eyez"/' "$TMP/g/llm_sample.json" > "$TMP/g/broken.json"
if "$GBNF" tools/melting-gen/run.gbnf "$TMP/g/broken.json" | grep -q "is valid"; then
  echo "FALLITO: la grammatica ha accettato uno slot inesistente"; exit 1
fi

echo "TEST-GEN: OK"
