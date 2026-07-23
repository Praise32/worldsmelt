#!/usr/bin/env bash
# Suite di comparazione dei modelli candidati (decisione dell'utente, sessione
# 22/07/2026: lista curata + quantizzazioni; piano in
# docs/plans/active/model-comparison.md). NON fa parte di `make test` -- e'
# lunga (piu' modelli x piu' run vere) -- ha un target dedicato,
# `make model-comparison`. I benchmark GPU girano SEMPRE in sequenza, mai due
# modelli in VRAM insieme (stesso vincolo di scripts/benchmark.sh).
#
# Per ogni modello:
#   1. controllo di caricamento (--bench, rispetta la VRAM di riferimento a
#      n_ctx=8192 fisso, tools/melting-gen/melting_gen.h:GEN_LLM_SESSION_N_CTX);
#      un modello che non carica (architettura non supportata dal tag llama.cpp
#      pinnato, o non entra in VRAM) resta nel report ma non genera run vere;
#   2. RUNS generazioni vere su seed fissi (stessi di scripts/gen-metrics.sh:
#      base 4242, passo 101), ognuna scritta in <modello>/run-<seed>/ con
#      manifest e corpus copiati accanto come manifest-<seed>.json/
#      corpus-<seed>.jsonl -- lo STESSO schema che gen_metrics.py gia' sa
#      leggere (scripts/model_comparison_report.py lo riusa via import).
#
# Uso: scripts/model-comparison.sh [models/uno.gguf models/due.gguf ...]
#   Senza argomenti: TUTTI i .gguf in models/ (l'ordinamento e' alfabetico;
#   quelli che non caricano finiscono comunque nel report, vedi sopra --
#   e' cosi' che si soddisfa "default: tutti i .gguf che caricano" senza
#   dover indovinare in anticipo quali carichino).
# Variabili: RUNS (default 3), SEED_BASE (default 4242), NGL (default 99),
# RUN_TIMEOUT/BENCH_TIMEOUT in secondi (default 900/180).
set -uo pipefail   # NON -e: un modello che fallisce non deve fermare la suite
cd "$(dirname "$0")/.."

RUNS="${RUNS:-3}"
SEED_BASE="${SEED_BASE:-4242}"
NGL="${NGL:-99}"
RUN_TIMEOUT="${RUN_TIMEOUT:-900}"
BENCH_TIMEOUT="${BENCH_TIMEOUT:-180}"

GEN=bin/melting-gen
[ -x "$GEN" ] || { echo "model-comparison: $GEN non trovato (esegui 'make gen' prima)" >&2; exit 1; }

mkdir -p logs/model-comparison logs/gen-corpus
STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/model-comparison/$STAMP"
mkdir -p "$OUT"

MODELS=("$@")
if [ ${#MODELS[@]} -eq 0 ]; then
  while IFS= read -r -d '' f; do MODELS+=("$f"); done < <(find models -maxdepth 1 -name '*.gguf' -print0 | sort -z)
fi
[ ${#MODELS[@]} -gt 0 ] || { echo "model-comparison: nessun .gguf in models/ (scripts/download-models.sh / download-comparison-models.sh)" >&2; exit 1; }

echo "== model-comparison: ${#MODELS[@]} candidati, $RUNS run/modello -> $OUT =="
echo "   seed: $(seq 0 $((RUNS-1)) | while read -r i; do printf '%d ' $((SEED_BASE + i*101)); done)"

for model in "${MODELS[@]}"; do
  if [ ! -f "$model" ]; then
    echo ""; echo "---- $model: FILE MANCANTE, salto ----"
    continue
  fi
  name=$(basename "$model" .gguf)
  mdir="$OUT/$name"
  mkdir -p "$mdir"
  sizeBytes=$(stat -c%s "$model")
  echo ""
  echo "---- $name ($(( sizeBytes / 1024 / 1024 )) MiB) ----"

  echo "-- controllo di caricamento (--bench, timeout ${BENCH_TIMEOUT}s) --"
  benchOut=$(timeout "$BENCH_TIMEOUT" "$GEN" --bench --model "$model" --ngl "$NGL" 2>&1)
  benchRc=$?
  printf '%s\n' "$benchOut" > "$mdir/bench.log"
  benchLine=$(printf '%s\n' "$benchOut" | grep '^bench:' || true)
  if [ "$benchRc" -ne 0 ] || [ -z "$benchLine" ]; then
    echo "   NON CARICA (rc=$benchRc): $(printf '%s\n' "$benchOut" | tail -3 | tr '\n' ' ')"
    { echo "loadOk=0 sizeBytes=$sizeBytes"; } > "$mdir/meta.txt"
    continue
  fi
  benchTokS=$(printf '%s\n' "$benchLine" | sed -n 's/.*tok_s=\([0-9.]*\).*/\1/p')
  benchLoadS=$(printf '%s\n' "$benchLine" | sed -n 's/.*load_s=\([0-9.]*\).*/\1/p')
  echo "   carica: bench tok/s=$benchTokS, caricamento=${benchLoadS}s"
  { echo "loadOk=1 sizeBytes=$sizeBytes benchTokS=$benchTokS benchLoadS=$benchLoadS"; } > "$mdir/meta.txt"

  for i in $(seq 0 $((RUNS - 1))); do
    seed=$((SEED_BASE + i*101))
    echo "-- run $((i+1))/$RUNS seed=$seed (timeout ${RUN_TIMEOUT}s) --"
    lcBefore=$(wc -l < logs/melting-gen.log 2>/dev/null || echo 0)
    t0=$SECONDS
    if timeout "$RUN_TIMEOUT" "$GEN" --model "$model" --ngl "$NGL" --seed "$seed" \
        --out "$mdir/run-$seed" > "$mdir/run-$seed.log" 2>&1; then
      wallSecs=$((SECONDS - t0))
      echo "   fatto in ${wallSecs}s"
      echo "seed=$seed wallSecs=$wallSecs" >> "$mdir/timing.txt"
      [ -f "$mdir/run-$seed/current_run.json" ] && cp "$mdir/run-$seed/current_run.json" "$mdir/manifest-$seed.json"
      # Il file di corpus di QUESTA run porta il seed nel nome (gen_corpus.c,
      # ...-seed<SEED>-...): niente ambiguita' con corse precedenti sullo
      # stesso seed, si prende la piu' recente fra quelle che matchano.
      corpus=$(ls -t logs/gen-corpus/*-seed"${seed}"-*.jsonl 2>/dev/null | head -1)
      [ -n "$corpus" ] && mv "$corpus" "$mdir/corpus-$seed.jsonl"
    else
      rc=$?
      wallSecs=$((SECONDS - t0))
      echo "   FALLITO o timeout dopo ${wallSecs}s (rc=$rc) -- vedi $mdir/run-$seed.log"
      echo "seed=$seed wallSecs=$wallSecs failed=1" >> "$mdir/timing.txt"
    fi
    # Righe di logs/melting-gen.log aggiunte da QUESTA run (offset per riga,
    # non per contenuto: il file e' condiviso e persistente fra tutte le
    # generazioni mai fatte su questa macchina, un grep per nome modello
    # prenderebbe anche corse di mesi fa). Solo informativo (tok/s della
    # generazione vera, oltre al tok/s sintetico del bench).
    lcAfter=$(wc -l < logs/melting-gen.log 2>/dev/null || echo 0)
    if [ "$lcAfter" -gt "$lcBefore" ]; then
      tail -n +"$((lcBefore + 1))" logs/melting-gen.log > "$mdir/run-$seed.genlog"
    fi
  done
done

echo ""
echo "== genero report.md/report.csv =="
python3 scripts/model_comparison_report.py "$OUT" | tee "$OUT/report.txt"
echo ""
echo "Report completo: $OUT/report.md"
