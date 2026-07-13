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

# Golden file di regressione: blocca l'ordine di estrazione dall'RNG del
# fallback (hue -> per-item trait/nome/slot x3 -> theme/weird -> style),
# le liste di parole e gli arrotondamenti. Un cambio accidentale a uno di
# questi tre punti produce un run diverso a parita' di seed senza far
# fallire nessun altro controllo di questo script (che confronta solo
# C-contro-C), quindi qui confrontiamo l'output con un file di riferimento
# generato una volta e committato.
echo "-- golden file: seed 12345 = manifest di riferimento --"
"$GEN" --fallback --seed 12345 --out "$TMP/golden"
cmp "$TMP/golden/current_run.txt" tests/melting-gen/golden-fallback-seed12345.txt

echo "TEST-GEN: OK"
