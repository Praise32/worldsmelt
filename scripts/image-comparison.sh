#!/usr/bin/env bash
# Suite di comparazione dei modelli IMMAGINE candidati (estensione della
# missione, sessione 22/07/2026 -- piano docs/plans/active/model-comparison.md,
# sezione immagini). Generalizza scripts/sprite-baseline.sh (che resta
# INVARIATO: e' il metro di paragone "Esperimento 0" per le LoRA future, un
# solo modello) su PIU' modelli/checkpoint candidati, stesso meccanismo:
# per ognuno, le 15 coppie tema|stile CONGELATE di
# docs/ai-production/dataset/baseline-prompts.txt x N seed fissi, scritte in un
# mini-manifest current_run.txt (SOLO floor1.theme/floor1.style, gli unici
# campi che SpritesLoadManifest legge) e passate a bin/melting-sprites.
#
# REGOLA FERREA: mai due processi GPU insieme (6 GB VRAM di riferimento) --
# questo script NON va lanciato mentre gira scripts/model-comparison.sh (testo)
# o un'altra generazione GPU. Sequenziale anche fra modelli immagine diversi
# (un solo processo bin/melting-sprites alla volta, come sprite-baseline.sh).
#
# Uso: scripts/image-comparison.sh <nome>:<model>[:<lora-o-vuoto>] [<nome>:<model>[:<lora>] ...]
#   <lora> omesso o vuoto = usa il default di melting-sprites (LCM-LoRA SD1.5,
#   models/lcm-lora-sdv1-5.safetensors); passare "-" per disattivarla
#   esplicitamente (modelli non-SD1.5, es. SD3.5, dove la LoRA SD1.5 non e'
#   compatibile -- vedi DEC-113/regole-agenti-ml.md, "LoRA prima dei
#   checkpoint completi" vale per lo stile, non per la compatibilita' architetturale).
# Esempio:
#   scripts/image-comparison.sh \
#     pixel-baseline:models/Public-Prompts-Pixel-Model.ckpt \
#     sd15-vanilla:models/sd15-vanilla-pruned-emaonly.safetensors
# Variabili: MAX_PAIRS (default 15, tutte), SEEDS (default "5 17", stessi di
# sprite-baseline.sh), TAESD=1 per usare il VAE veloce approssimato.
set -uo pipefail   # NON -e: un modello che fallisce non deve fermare la suite
cd "$(dirname "$0")/.."

SPR="bin/melting-sprites"
PROMPTS_FILE="docs/ai-production/dataset/baseline-prompts.txt"
MAX_PAIRS="${MAX_PAIRS:-15}"
SEEDS="${SEEDS:-5 17}"
TAESD="${TAESD:-0}"

[ -x "$SPR" ] || { echo "image-comparison: $SPR non trovato (esegui 'make sprites' prima)" >&2; exit 1; }
[ -f "$PROMPTS_FILE" ] || { echo "image-comparison: prompt di baseline mancanti: $PROMPTS_FILE" >&2; exit 1; }
[ $# -gt 0 ] || { echo "image-comparison: nessun candidato passato (uso: nome:model[:lora] ...)" >&2; exit 1; }

mapfile -t PAIRS < <(grep -vE '^[[:space:]]*#' "$PROMPTS_FILE" | grep -vE '^[[:space:]]*$')
PAIRS=("${PAIRS[@]:0:$MAX_PAIRS}")
read -r -a SEED_LIST <<< "$SEEDS"
N_PAIRS="${#PAIRS[@]}"
N_SEEDS="${#SEED_LIST[@]}"

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/model-comparison/$STAMP/images"
mkdir -p "$OUT"

echo "== image-comparison: $# candidati, $N_PAIRS coppie x $N_SEEDS seed -> $OUT =="

TMP_ROOT=$(mktemp -d)
trap 'rm -rf "$TMP_ROOT"' EXIT

for spec in "$@"; do
  IFS=':' read -r name model lora <<< "$spec"
  mdir="$OUT/$name"
  mkdir -p "$mdir"
  if [ ! -f "$model" ]; then
    echo ""; echo "---- $name: modello mancante ($model), salto ----"
    echo "loadOk=0 note=modello_mancante" > "$mdir/meta.txt"
    continue
  fi
  sizeBytes=$(stat -c%s "$model")
  loraArgs=()
  loraNote="default (LCM-LoRA SD1.5)"
  if [ "${lora:-}" = "-" ]; then
    loraArgs=(--lora "")
    loraNote="disattivata"
  elif [ -n "${lora:-}" ]; then
    loraArgs=(--lora "$lora")
    loraNote="$lora"
  fi
  taesdArgs=()
  [ "$TAESD" = "1" ] && taesdArgs=(--taesd)

  echo ""
  echo "---- $name ($(( sizeBytes / 1024 / 1024 )) MiB, modello=$model, lora=$loraNote) ----"

  echo "-- controllo di caricamento + s/immagine a regime (--bench) --"
  benchOut=$(timeout 300 "$SPR" --bench --model "$model" "${loraArgs[@]}" "${taesdArgs[@]}" 2>&1)
  benchRc=$?
  printf '%s\n' "$benchOut" > "$mdir/bench.log"
  benchLine=$(printf '%s\n' "$benchOut" | grep '^bench:' || true)
  if [ "$benchRc" -ne 0 ] || [ -z "$benchLine" ]; then
    echo "   NON CARICA (rc=$benchRc): $(printf '%s\n' "$benchOut" | tail -3 | tr '\n' ' ')"
    { echo "loadOk=0 sizeBytes=$sizeBytes lora=$loraNote"; } > "$mdir/meta.txt"
    continue
  fi
  imgS=$(printf '%s\n' "$benchLine" | sed -n 's/.*img_s=\([0-9.]*\).*/\1/p')
  loadS=$(printf '%s\n' "$benchLine" | sed -n 's/.*load_s=\([0-9.]*\).*/\1/p')
  warmupS=$(printf '%s\n' "$benchLine" | sed -n 's/.*warmup_s=\([0-9.]*\).*/\1/p')
  echo "   carica: img/s a regime=$imgS (warmup=${warmupS}s), caricamento=${loadS}s"
  { echo "loadOk=1 sizeBytes=$sizeBytes lora=$loraNote imgS=$imgS loadS=$loadS warmupS=$warmupS"; } > "$mdir/meta.txt"

  echo "-- generazione atlas ($N_PAIRS coppie x $N_SEEDS seed = $((N_PAIRS * N_SEEDS)) atlas) --"
  INDEX="$mdir/index.txt"
  echo "# atlas | tema | stile | seed | tempo(s) | celle_scartate" > "$INDEX"
  i=0
  for pair in "${PAIRS[@]}"; do
    i=$((i + 1))
    pair="${pair%$'\r'}"
    theme="${pair%%|*}"
    style="${pair#*|}"
    nn=$(printf "%02d" "$i")
    for seed in "${SEED_LIST[@]}"; do
      manifestDir="$TMP_ROOT/$name-pair-$nn-seed$seed"
      mkdir -p "$manifestDir"
      printf 'floor1.theme=%s\nfloor1.style=%s\n' "$theme" "$style" > "$manifestDir/current_run.txt"

      echo "   [$i/$N_PAIRS seed $seed] $theme | $style"
      lcBefore=$(wc -l < logs/melting-sprites.log 2>/dev/null || echo 0)
      t0=$SECONDS
      if timeout 300 "$SPR" --out "$manifestDir" --model "$model" --seed "$seed" \
          "${loraArgs[@]}" "${taesdArgs[@]}" >> "$mdir/gen.log" 2>&1; then
        elapsed=$((SECONDS - t0))
        cp "$manifestDir/current_atlas.png" "$mdir/atlas-$nn-seed$seed.png"
        # "run: ... celle=N scartate=M ..." -- solo le righe aggiunte da
        # QUESTA invocazione (logs/melting-sprites.log e' condiviso e
        # persistente, stesso motivo dell'offset in
        # scripts/model-comparison.sh per il testo).
        rejLine=$(tail -n +"$((lcBefore + 1))" logs/melting-sprites.log 2>/dev/null | grep '^\[.*\] run: ' | tail -1)
        rejected=$(printf '%s\n' "$rejLine" | sed -n 's/.*scartate=\([0-9]*\).*/\1/p')
        printf '%s | %s | %s | %s | %ss | %s\n' "atlas-$nn-seed$seed.png" "$theme" "$style" "$seed" "$elapsed" "${rejected:-n/d}" >> "$INDEX"
      else
        elapsed=$((SECONDS - t0))
        echo "   FALLITO o timeout dopo ${elapsed}s"
        printf '%s | %s | %s | %s | FALLITO dopo %ss | -\n' "atlas-$nn-seed$seed.png" "$theme" "$style" "$seed" "$elapsed" >> "$INDEX"
      fi
    done
  done
done

echo ""
echo "== fatto: atlas e indici per modello sotto $OUT =="
