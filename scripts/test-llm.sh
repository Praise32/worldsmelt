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
# Step C review (bug REALE, trovato con una generazione vera al seed 20260714):
# la finestra della penalita' sulle ripetizioni era piu' CORTA di un piano di JSON
# (256 token contro ~370), quindi ricopiare il piano precedente non costava nulla
# al modello -- e su alcuni seed produceva CINQUE PIANI FOTOCOPIA: stesso tema,
# stesso boss, stessi oggetti, stesso tipo di colpo. Nessun test se ne accorgeva:
# make test-gen non tocca il modello, e il manifest era formalmente PERFETTO.
# Questo e' il posto giusto per la guardia -- l'unico test che il modello vero lo
# usa davvero. Non giudica il gusto (non e' testabile), giudica la VARIETA', che e'
# la promessa minima di un generatore di contenuti.
echo "--- varieta': 5 piani diversi, 5 tipi di colpo diversi (guardia anti-fotocopia) ---"
distinctThemes=$(grep -E '^floor[0-9]\.theme=' generated/current_run.txt | sed 's/.*=//' | sort -u | wc -l)
if [ "$distinctThemes" -lt 5 ]; then
  echo "FALLITO: solo $distinctThemes temi distinti su 5 -- il modello sta ricopiando i piani"
  grep -E '^floor[0-9]\.theme=' generated/current_run.txt
  exit 1
fi
shotNames=$(grep -E '^floor[0-9]\.item[0-9]\.shotName=' generated/current_run.txt | sed 's/.*=//')
shotCount=$(echo "$shotNames" | sed '/^$/d' | wc -l)
distinctShots=$(echo "$shotNames" | sed '/^$/d' | sort -u | wc -l)
if [ "$shotCount" -ne 5 ]; then
  echo "FALLITO: $shotCount tipi di colpo nel manifest (atteso 1 per piano = 5)"; exit 1
fi
if [ "$distinctShots" -lt 5 ]; then
  echo "FALLITO: solo $distinctShots tipi di colpo distinti su 5 -- il modello li sta ricopiando"
  echo "$shotNames"
  exit 1
fi
echo "   temi distinti: $distinctThemes/5 | tipi di colpo distinti: $distinctShots/5"
echo "$shotNames" | sed 's/^/   colpo inventato: /'

echo "--- ultima riga di log (tempi e tok/s) ---"
grep "^\[.*\] ok: model=" logs/melting-gen.log | tail -1

echo "--- riepilogo Lua (fase 3a-L3): quanti dei 15 oggetti hanno preso uno script funzionante ---"
grep "lua: riepilogo run" logs/melting-gen.log | tail -1

echo "--- script Lua scritti in generated/scripts/ ---"
ls -1 generated/scripts/*.lua 2>/dev/null || echo "(nessuno: tutti gli oggetti sono ripiegati sulla mini-VM)"

echo "TEST-LLM: OK"
