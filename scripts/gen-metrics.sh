#!/usr/bin/env bash
# Harness delle metriche di generazione (roadmap 16/07/2026, settimana 1).
# Lancia RUNS generazioni VERE su seed diversi (serve un modello in models/) e
# poi misura validita' del Lua e varieta' FRA run con scripts/gen_metrics.py.
# Ogni run col 7B costa ~2-3 minuti: il default (3 run) e' ~6-9 minuti.
#
# Uso: scripts/gen-metrics.sh [RUNS] [SEED_BASE]     (default: 3 run, base 4242)
# Variabili: MODEL (default: il 7B se c'e', altrimenti l'1.5B), NGL.
set -euo pipefail
cd "$(dirname "$0")/.."

RUNS="${1:-3}"
BASE="${2:-4242}"
NGL="${NGL:-99}"
MODEL="${MODEL:-}"
if [ -z "$MODEL" ]; then
  for m in models/qwen2.5-coder-7b-instruct-q4_k_m.gguf \
           models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf; do
    if [ -f "$m" ]; then MODEL="$m"; break; fi
  done
fi
[ -n "$MODEL" ] && [ -f "$MODEL" ] || {
  echo "Nessun modello in models/ — esegui scripts/download-models.sh"; exit 1; }

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/gen-metrics/$STAMP"
mkdir -p "$OUT"
echo "== gen-metrics: $RUNS run con $MODEL -> $OUT =="

for i in $(seq 0 $((RUNS - 1))); do
  seed=$((BASE + i*101))
  echo "-- run $((i + 1))/$RUNS (seed $seed)..."
  t0=$SECONDS
  bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$seed" \
    --out "$OUT/run-$seed" > "$OUT/run-$seed.log" 2>&1
  echo "   fatto in $((SECONDS - t0))s"
  cp "$OUT/run-$seed/current_run.json" "$OUT/manifest-$seed.json"
  # Il corpus della run appena finita e' il file piu' recente: lo si sposta
  # accanto al manifest cosi' la coppia (manifest, corpus) resta appaiata
  # per seed e logs/gen-corpus/ non si riempie di run di misura.
  newest="logs/gen-corpus/$(ls -t logs/gen-corpus 2>/dev/null | head -1)"
  [ -f "$newest" ] && mv "$newest" "$OUT/corpus-$seed.jsonl"
done

python3 scripts/gen_metrics.py "$OUT" | tee "$OUT/report.txt"
