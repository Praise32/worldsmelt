#!/usr/bin/env bash
# Esperimento 0 (roadmap 16/07/2026, settimana 2; vedi
# roguelike-ai-appunti/06-training-hardware-costi.md, "Piano di
# addestramento SD" e docs/dataset/baseline-prompts.txt): baseline sprite a
# prompt e seed FISSI con la pipeline attuale, SENZA alcun training. Il
# risultato e' il metro di paragone per ogni Style/Item LoRA futura.
#
# Per ogni coppia tema/stile di docs/dataset/baseline-prompts.txt e per ogni
# seed fisso, scrive un mini-manifest temporaneo con SOLO floor1.theme= e
# floor1.style= -- i due unici campi che SpritesLoadManifest legge da
# current_run.txt (tools/melting-sprites/sprite_manifest.c) -- e lancia
# bin/melting-sprites su quel manifest. Ogni atlas risultante finisce in
# logs/sprite-baseline/<timestamp>/, con un index.txt che elenca tema, stile,
# seed e tempo di ogni atlas.
#
# Non compila nulla (nessun 'make'): usa bin/melting-sprites e i prompt di
# tools/melting-sprites/prompts/ cosi' come sono, ed esce con un errore
# chiaro se il binario o il modello mancano.
#
# Uso:      scripts/sprite-baseline.sh
# Variabili:
#   START_PAIR prima coppia da usare, 1-based (default: 1) -- per riprendere
#              una baseline interrotta o girare a tranche (una corsa intera
#              sono ~40 min di GPU: piu' di quanto un task in background possa
#              restare vivo, misurato stanotte). La numerazione NN degli atlas
#              resta quella ASSOLUTA della coppia nel file, cosi' le tranche
#              si ricompongono in una cartella sola senza collisioni.
#   MAX_PAIRS  quante coppie usare da START_PAIR in poi (default: tutte)
#   SEEDS      lista di seed separati da spazi (default: "5 17")
#   MODEL      checkpoint SD (default: quello di tools/melting-sprites/main.c)
#   OUT_DIR    cartella di uscita (default: logs/sprite-baseline/<timestamp>)
#              -- per accodare una tranche alla cartella di una precedente.
set -euo pipefail
cd "$(dirname "$0")/.."

SPR="bin/melting-sprites"
PROMPTS_FILE="docs/dataset/baseline-prompts.txt"
START_PAIR="${START_PAIR:-1}"
MAX_PAIRS="${MAX_PAIRS:-15}"
SEEDS="${SEEDS:-5 17}"
SECS_PER_ATLAS=85   # misurato nello spike (docs/SPRITES-SPIKE.md); solo una stima

# Stesso default di ParseArgs in tools/melting-sprites/main.c: si controlla
# QUI, una volta sola, invece di lasciar fallire ogni singola chiamata dopo
# aver gia' scritto meta' degli atlas (come scripts/test-llm.sh fa per il
# modello di melting-gen).
MODEL="${MODEL:-models/Public-Prompts-Pixel-Model.ckpt}"
[ -f "$MODEL" ] || { echo "Modello mancante: $MODEL — esegui scripts/download-models.sh"; exit 1; }
[ -f "$SPR" ] || { echo "Binario mancante: $SPR — questo script non compila nulla, eseguire prima 'make sprites'"; exit 1; }
[ -f "$PROMPTS_FILE" ] || { echo "Prompt di baseline mancanti: $PROMPTS_FILE"; exit 1; }

# Righe utili: niente commenti ('#') ne' righe vuote (vedi l'intestazione di
# baseline-prompts.txt).
mapfile -t PAIRS < <(grep -vE '^[[:space:]]*#' "$PROMPTS_FILE" | grep -vE '^[[:space:]]*$')
if [ "${#PAIRS[@]}" -eq 0 ]; then
  echo "Nessuna coppia tema|stile trovata in $PROMPTS_FILE"
  exit 1
fi
if [ "$START_PAIR" -lt 1 ] || [ "$START_PAIR" -gt "${#PAIRS[@]}" ]; then
  echo "START_PAIR=$START_PAIR fuori dall'intervallo 1..${#PAIRS[@]}"
  exit 1
fi
PAIRS=("${PAIRS[@]:$((START_PAIR - 1)):$MAX_PAIRS}")

read -r -a SEED_LIST <<< "$SEEDS"
if [ "${#SEED_LIST[@]}" -eq 0 ]; then
  echo "SEEDS vuoto: nessun seed su cui girare"
  exit 1
fi

N_PAIRS="${#PAIRS[@]}"
N_SEEDS="${#SEED_LIST[@]}"
N_ATLAS=$((N_PAIRS * N_SEEDS))
EST_SECS=$((N_ATLAS * SECS_PER_ATLAS))
EST_MIN=$((EST_SECS / 60))
echo "== sprite-baseline: $N_PAIRS coppie x $N_SEEDS seed (${SEED_LIST[*]}) = $N_ATLAS atlas =="
echo "== stima: ~${EST_MIN} min (~${SECS_PER_ATLAS}s/atlas, misurato nello spike) =="

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="${OUT_DIR:-logs/sprite-baseline/$STAMP}"
mkdir -p "$OUT"
INDEX="$OUT/index.txt"
[ -f "$INDEX" ] || echo "# atlas | tema | stile | seed | tempo(s)" > "$INDEX"

TMP_ROOT=$(mktemp -d)
trap 'rm -rf "$TMP_ROOT"' EXIT

# NN degli atlas = numero ASSOLUTO della coppia nel file dei prompt (non la
# posizione nella tranche): due tranche con START_PAIR diversi si accodano
# nella stessa OUT_DIR senza sovrascriversi.
i=$((START_PAIR - 1))
for pair in "${PAIRS[@]}"; do
  i=$((i + 1))
  pair="${pair%$'\r'}"          # tolleranza a un eventuale CRLF nel file
  theme="${pair%%|*}"
  style="${pair#*|}"
  nn=$(printf "%02d" "$i")

  for seed in "${SEED_LIST[@]}"; do
    manifestDir="$TMP_ROOT/pair-$nn-seed$seed"
    mkdir -p "$manifestDir"
    # Stesso formato "chiave=valore" per riga scritto da
    # tools/melting-gen/gen_manifest.c: SpritesLoadManifest legge solo
    # floor1.theme= e floor1.style=, quindi bastano queste due righe.
    printf 'floor1.theme=%s\nfloor1.style=%s\n' "$theme" "$style" > "$manifestDir/current_run.txt"

    echo "-- [$i/$N_PAIRS] seed $seed: $theme | $style --"
    t0=$SECONDS
    "$SPR" --out "$manifestDir" --model "$MODEL" --seed "$seed"
    elapsed=$((SECONDS - t0))

    destAtlas="$OUT/atlas-$nn-seed$seed.png"
    cp "$manifestDir/current_atlas.png" "$destAtlas"
    printf '%s | %s | %s | %s | %ss\n' "$destAtlas" "$theme" "$style" "$seed" "$elapsed" >> "$INDEX"
  done
done

echo "== fatto: $N_ATLAS atlas e indice in $OUT/index.txt =="
