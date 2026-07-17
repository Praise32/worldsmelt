#!/usr/bin/env bash
# Benchmark della macchina (piano strategico 16/07/2026, sezione tier): il
# tier va per THROUGHPUT MISURATO, mai per nome GPU -- la scheda n.1 su Steam
# e' una 4060 Laptop, ~2x piu' lenta della desktop omonima con lo stesso nome.
# Questo script orchestra i due passi --bench (tools/melting-gen/main.c,
# tools/melting-sprites/main.c) IN SEQUENZA, mai insieme: i due modelli non
# stanno insieme nella VRAM della scheda di riferimento (stesso vincolo di
# src/app/app.c, AppStartGeneration/AppStartSpritesGeneration). Scrive
# logs/benchmark.txt in formato chiave=valore, che src/app/app.c rilegge al
# prossimo avvio con --generate (AppReadBenchmarkPreset) per applicare da
# solo il preset --low-spec quando serve -- vedi il vincolo architetturale in
# AGENTS.md: il gioco NON linka MAI llama.cpp/sd.cpp, legge solo questo file.
#
# Uso:      scripts/benchmark.sh   (anche "make benchmark")
set -uo pipefail
cd "$(dirname "$0")/.."

mkdir -p logs bin

GEN=bin/melting-gen
SPR=bin/melting-sprites

if [ ! -x "$GEN" ]; then
  echo "benchmark: $GEN non trovato (esegui 'make gen' prima)" >&2
  exit 1
fi
if [ ! -x "$SPR" ]; then
  echo "benchmark: $SPR non trovato (esegui 'make sprites' prima)" >&2
  exit 1
fi

echo "== benchmark: testo (melting-gen --bench, modello di sempre) =="
genOut=$("$GEN" --bench 2>&1)
genLine=$(printf '%s\n' "$genOut" | grep '^bench:' || true)
tokS=""
if [ -n "$genLine" ]; then
  tokS=$(printf '%s\n' "$genLine" | sed -n 's/.*tok_s=\([0-9.]*\).*/\1/p')
  loadSGen=$(printf '%s\n' "$genLine" | sed -n 's/.*load_s=\([0-9.]*\).*/\1/p')
  echo "   tok/s=$tokS  caricamento=${loadSGen}s"
else
  echo "   nessun modello di testo disponibile (vedi scripts/download-models.sh)"
fi

echo "== benchmark: sprite (melting-sprites --bench, immagine 512px) =="
sprOut=$("$SPR" --bench 2>&1)
sprLine=$(printf '%s\n' "$sprOut" | grep '^bench:' || true)
imgS=""
if [ -n "$sprLine" ]; then
  imgS=$(printf '%s\n' "$sprLine" | sed -n 's/.*img_s=\([0-9.]*\).*/\1/p')
  loadSSpr=$(printf '%s\n' "$sprLine" | sed -n 's/.*load_s=\([0-9.]*\).*/\1/p')
  echo "   img/s=$imgS  caricamento=${loadSSpr}s"
else
  echo "   nessun modello Stable Diffusion disponibile (vedi scripts/download-models.sh)"
fi

# Soglie (piano strategico 16/07/2026, sezione tier), misurate non dedotte:
#   tokS >= 12 E imgS <= 8 -> full       (hardware alla pari/sopra la scheda di riferimento)
#   tokS >= 6              -> lowspec    (testo comunque utilizzabile)
#   sotto                  -> unsupported
# Se manca il modello SD (imgS vuoto) il ramo "full" non puo' mai scattare
# (richiede imgS<=8), quindi con tokS>=6 il verdetto ricade gia' da solo su
# lowspec -- esattamente "SD assente ma il testo regge" del piano: il gioco
# ha gia' il fallback geometrico per gli sprite (atlas BMP procedurale), non
# serve una regola a parte.
tier="unsupported"
if [ -n "$tokS" ] && awk -v t="$tokS" 'BEGIN{exit !(t>=12)}'; then
  if [ -n "$imgS" ] && awk -v i="$imgS" 'BEGIN{exit !(i<=8)}'; then
    tier="full"
  else
    tier="lowspec"
  fi
elif [ -n "$tokS" ] && awk -v t="$tokS" 'BEGIN{exit !(t>=6)}'; then
  tier="lowspec"
fi

{
  echo "benchSchema=1"
  echo "tokS=${tokS:-0}"
  echo "imgS=${imgS:-0}"
  echo "tier=$tier"
  echo "measuredAt=$(date +%s)"
} > logs/benchmark.txt.tmp
mv logs/benchmark.txt.tmp logs/benchmark.txt

echo ""
echo "== verdetto =="
case "$tier" in
  full)
    echo "tier=full: hardware alla pari o sopra la scheda di riferimento (5600 XT). Preset di default al prossimo --generate." ;;
  lowspec)
    echo "tier=lowspec: sotto la scheda di riferimento. Il prossimo --generate applichera' da solo il preset --low-spec (modello 1.5B + sprite 256px), a meno di passare --low-spec o --full-spec a mano." ;;
  unsupported)
    echo "tier=unsupported: throughput sotto ogni soglia utile. Si puo' comunque giocare (fallback procedurale, nessun blocco), ma la generazione IA sara' lenta o assente." ;;
esac
echo "Scritto logs/benchmark.txt"
