#!/usr/bin/env bash
# runtime_bench_fusion — harness R3 (mandato 06/08): sulle config RUNTIME
# promosse da R2 (default S2 S3 -- parametro, vedi sotto), confronta TRE
# tecniche di fusione di coppie di asset gia' generati:
#   (a) spec-fusion : prompt "fusion of X and Y, ..." (scripts/
#                      visualspec_template.py:build_fusion_prompt), text2img
#                      puro -- nessuna immagine sorgente, solo il prompt.
#   (b) img2img      : composito dei due sprite pixel-64 (affiancati/
#                       sovrapposti, upscale nearest) come --init-img,
#                       --strength 0.55 -- sd-cli denoisa PARZIALMENTE da una
#                       geometria gia' fusa.
#   (c) controlnet   : maschera unione dei due alpha (stesso composito di
#                       (b), ridotta al contorno) come --control-image, via
#                       --control-net + control_v11p_sd15_scribble -- nessun
#                       --init-img, il controllo e' SOLO strutturale.
# Le tre tecniche condividono lo STESSO prompt/negative (quello di (a)): la
# variabile sotto esame e' il MECCANISMO di fusione, non il testo.
#
# PRECONDIZIONE (non verificata qui in automatico, solo con un controllo di
# esistenza file): scripts/runtime-bench.sh + `python3 scripts/
# teacher_bench_post.py --root artifacts/runtime-bench` devono essere gia'
# girati per OGNI config di questo script, cosi' pixel-64/<config>/
# <reqid>__spec.png esiste per gli id delle coppie -- (b)/(c) lo leggono
# come sorgente. Manca il file? La coppia/tecnica viene SALTATA con un
# fallimento registrato (vedi sotto), non e' fatale per il resto della corsa.
#
# 8 coppie FISSE (item 3 del task, commentate qui invece che in un file a
# parte: sono un dato dell'esperimento, non una configurazione che deve
# cambiare spesso): 4 item+item, 2 enemy+enemy, 2 enemy+item. Gli id
# assumono la numerazione standard "<domain>_01".."<domain>_10" del
# contratto batch.json (10 richieste/dominio, vedi scripts/runtime-bench.sh);
# override totale con FUSION_PAIRS ("idA:idB idA2:idB2 ..." separati da
# spazio) se il batch reale numera diversamente.
FUSION_PAIRS_DEFAULT=(
  "item_01:item_02" "item_03:item_04" "item_05:item_06" "item_07:item_08"   # 4x item+item
  "enemy_01:enemy_02" "enemy_03:enemy_04"                                    # 2x enemy+enemy
  "enemy_05:item_09" "enemy_06:item_10"                                      # 2x enemy+item
)

set -uo pipefail
cd "$(dirname "$0")/.."

OUT_ROOT="${OUT_ROOT:-artifacts/runtime-bench/fusion}"
RAW_DIR="$OUT_ROOT/raw-512"
MANIFEST_DIR="$OUT_ROOT/manifests"
FAILURES_DIR="$OUT_ROOT/failures"
LOG_DIR="logs/runtime-bench-fusion"
SOURCE_DIR="$OUT_ROOT/sources"   # composito img2img/controlnet, tenuto (non temporaneo): e' un dato dell'esperimento tanto quanto il raw finale

RUNTIME_BENCH_ROOT="${RUNTIME_BENCH_ROOT:-artifacts/runtime-bench}"   # dove runtime-bench.sh + teacher_bench_post.py hanno gia' scritto pixel-64/
BATCH_FILE="${BATCH_FILE:-generated/visualspecs/batch.json}"
SD_CLI="${SD_CLI:-deps/stable-diffusion.cpp/build/bin/sd-cli}"
LEDGER_FILE="${LEDGER_FILE:-artifacts/image-model-research/manifests/model-ledger.json}"
CONTROL_NET_MODEL="${CONTROL_NET_MODEL:-models/teacher-bench-2026-08/control_v11p_sd15_scribble_fp16.safetensors}"
COMPOSE_PY="scripts/runtime_bench_fusion_compose.py"
VISUALSPEC_TEMPLATE="scripts/visualspec_template.py"
WIDTH="${WIDTH:-512}"
HEIGHT="${HEIGHT:-512}"
IMG2IMG_STRENGTH="${IMG2IMG_STRENGTH:-0.55}"
DRY_RUN="${DRY_RUN:-0}"
# Seed FISSO e registrato nel manifest, non --seed -1. La prima versione
# usava il casuale, con l'argomento che "fra text2img, img2img e controlnet
# il seed non isola niente di confrontabile": vero, ma il costo e' che
# un'immagine di fusione che il proprietario approva non si puo' piu' ne'
# rigenerare ne' spiegare, e il manifest non registrava nemmeno il seed che
# sd-cli aveva risolto. Un seed fisso e' al peggio neutro sul confronto fra
# tecniche (nessuna delle tre e' avvantaggiata) ed e' l'unica forma che
# rende la corsa ripetibile. Default = il "seed" del batch, cosi' l'intera
# giornata di esperimenti (R2 + R3) condivide un solo numero da citare.
FUSION_SEED="${FUSION_SEED:-}"

DEFAULT_CONFIGS=(S2 S3)   # "le 2 config con piu' silhouette connesse in R2" (task): default in attesa del verdetto R2 vero, parametro esplicito

if [ "${1:-}" = "--list" ]; then
  echo "runtime_bench_fusion: tecniche fisse spec-fusion/img2img/controlnet; config di default: ${DEFAULT_CONFIGS[*]} (override: argomenti posizionali)"
  echo "coppie di default: ${FUSION_PAIRS_DEFAULT[*]} (override: FUSION_PAIRS=\"idA:idB ...\")"
  exit 0
fi

[ -f "$BATCH_FILE" ] || { echo "runtime_bench_fusion: batch mancante: $BATCH_FILE" >&2; exit 1; }
[ -f "$VISUALSPEC_TEMPLATE" ] || { echo "runtime_bench_fusion: manca $VISUALSPEC_TEMPLATE" >&2; exit 1; }
[ -f "$COMPOSE_PY" ] || { echo "runtime_bench_fusion: manca $COMPOSE_PY" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "runtime_bench_fusion: python3 richiesto" >&2; exit 1; }
if [ "$DRY_RUN" != "1" ]; then
  [ -x "$SD_CLI" ] || { echo "runtime_bench_fusion: sd-cli non trovato/eseguibile: $SD_CLI" >&2; exit 1; }
  # REGOLA FERREA GPU, stesso motivo/stessa forma di scripts/teacher-bench.sh e scripts/runtime-bench.sh.
  if pgrep 'sd-cli|melting-sprites|melting-gen' >/dev/null 2>&1; then
    echo "runtime_bench_fusion: un processo GPU (sd-cli/melting-sprites/melting-gen) e' gia' in esecuzione -- attendi e rilancia." >&2
    exit 1
  fi
fi

SD_CPP_COMMIT="unknown"
if [ -d deps/stable-diffusion.cpp/.git ]; then
  SD_CPP_COMMIT=$(git -C deps/stable-diffusion.cpp rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
fi

mkdir -p "$RAW_DIR" "$MANIFEST_DIR" "$FAILURES_DIR" "$LOG_DIR" "$SOURCE_DIR"

# -- Tabella config (COPIA di S1..S4, stesso motivo/stessa forma di
# scripts/runtime-bench.sh: source-are teacher-bench.sh eseguirebbe Stage A
# per intero. Tenere sincronizzata a mano con CONFIG_ROW li' e in
# scripts/runtime-bench.sh se cambia modello/LoRA/step.) --------------------
declare -A CONFIG_ROW
CONFIG_ROW[S1]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f6\x1f1.5\x1flcm\x1flcm\x1fbasepixel, \x1fmodels:lcm-lora-sdv1-5:1.0'
CONFIG_ROW[S2]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f8\x1f5\x1feuler_a\x1fkarras\x1fbasepixel, \x1fmodels/teacher-bench-2026-08:Hyper-SD15-8steps-CFG-lora:1.0'
CONFIG_ROW[S3]=$'models/teacher-bench-2026-08/anyloraCheckpoint_lcm.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f6\x1f1.5\x1flcm\x1flcm\x1fpixel_art, \x1f'
CONFIG_ROW[S4]=$'models/teacher-bench-2026-08/DreamShaper8_LCM.safetensors\x1f\x1f\x1f\x1f8\x1f2\x1flcm\x1flcm\x1f\x1f'

ledger_field() {
  python3 -c "
import json, sys
try:
    d = json.load(open(sys.argv[1])).get('entries', {}).get(sys.argv[2]) or {}
except FileNotFoundError:
    d = {}
v = d.get(sys.argv[3])
print('' if v is None else v, end='')
" "$LEDGER_FILE" "$1" "$2"
}

# -- Indice id -> (domain, spec_json) di TUTTO il batch -----------------------
# Stesso `mapfile -d ''` di scripts/runtime-bench.sh (via visualspec_template
# --batch), ma qui serve solo un LOOKUP per id arbitrario (le coppie, non
# tutte le richieste in ordine): popola due dict associativi bash invece di
# iterare in blocco.
declare -A REQ_DOMAIN REQ_SPEC_JSON
mapfile -d '' REQ_FIELDS < <(python3 "$VISUALSPEC_TEMPLATE" --batch "$BATCH_FILE")
if [ "${#REQ_FIELDS[@]}" -eq 0 ] || [ $(( ${#REQ_FIELDS[@]} % 6 )) -ne 0 ]; then
  echo "runtime_bench_fusion: $BATCH_FILE non contiene 'requests' valide" >&2
  exit 1
fi
for ((i = 0; i < ${#REQ_FIELDS[@]}; i += 6)); do
  REQ_DOMAIN["${REQ_FIELDS[i]}"]="${REQ_FIELDS[i+1]}"
  REQ_SPEC_JSON["${REQ_FIELDS[i]}"]="${REQ_FIELDS[i+2]}"
done

# Seed di default = quello del batch (vedi FUSION_SEED in testata). Letto qui
# e non prima perche' e' l'unico altro campo di batch.json che serve a questo
# script: tutto il resto passa da visualspec_template.py.
if [ -z "$FUSION_SEED" ]; then
  FUSION_SEED=$(python3 -c "
import json, sys
print(json.load(open(sys.argv[1])).get('seed', ''))" "$BATCH_FILE")
fi
case "$FUSION_SEED" in
  ''|*[!0-9]*) echo "runtime_bench_fusion: FUSION_SEED non numerico ('$FUSION_SEED') e nessun 'seed' valido in $BATCH_FILE" >&2; exit 1 ;;
esac

if [ -n "${FUSION_PAIRS:-}" ]; then
  read -ra REQUESTED_PAIRS <<< "$FUSION_PAIRS"
else
  REQUESTED_PAIRS=("${FUSION_PAIRS_DEFAULT[@]}")
fi

if [ $# -gt 0 ]; then
  REQUESTED_CONFIGS=("$@")
else
  REQUESTED_CONFIGS=("${DEFAULT_CONFIGS[@]}")
fi

fail_pair() {
  local cfgid="$1" pairid="$2" technique="$3" reason="$4"
  echo "runtime_bench_fusion:   SALTATO [$cfgid] $pairid ($technique): $reason" >&2
  mkdir -p "$FAILURES_DIR"
  printf 'config=%s pair=%s technique=%s reason=%s\n' "$cfgid" "$pairid" "$technique" "$reason" \
    > "$FAILURES_DIR/${cfgid}_${pairid}__${technique}.txt"
}

# -- Generazione di UNA immagine di fusione -----------------------------------
# 'extra_sd_args' (array, passato per nome con nameref -- bash 4.3+, gia' un
# requisito implicito del repo: gli altri harness usano mapfile -d '' e
# associative array introdotti nella stessa versione) porta i flag SPECIFICI
# della tecnica (--init-img/--strength per img2img, --control-net/
# --control-image per controlnet); spec-fusion non ne aggiunge nessuno.
run_fusion_one() {
  local cfgid="$1" pairid="$2" technique="$3" prompt_full="$4" negative="$5" \
        model="$6" steps="$7" cfg_scale="$8" sampler="$9"
  shift 9
  local scheduler="$1" extra_args_name="$2" source_note="$3"
  local -n extra_sd_args="$extra_args_name"

  local raw_dir="$RAW_DIR/$cfgid"
  local stem="${pairid}__${technique}"
  local raw_path="$raw_dir/${stem}.png"
  local manifest_path="$MANIFEST_DIR/${cfgid}_${stem}.json"

  if [ -f "$raw_path" ]; then
    echo "runtime_bench_fusion:   [$cfgid] $pairid ($technique) gia' presente, salto (resume)"
    return 0
  fi
  mkdir -p "$raw_dir"

  if [ "$DRY_RUN" = "1" ]; then
    printf 'DRY-RUN [%s] %s technique=%s :: %s -m %s -p %q -n %q -W %s -H %s --steps %s --cfg-scale %s --sampling-method %s --scheduler %s --seed %s %s -o %s\n' \
      "$cfgid" "$pairid" "$technique" "$SD_CLI" "$model" "$prompt_full" "$negative" \
      "$WIDTH" "$HEIGHT" "$steps" "$cfg_scale" "$sampler" "$scheduler" "$FUSION_SEED" "${extra_sd_args[*]:-}" "$raw_path"
    return 0
  fi

  local tmp_out="${raw_path}.tmp.png"
  local sdlog="$LOG_DIR/${cfgid}_${stem}.log"
  echo "runtime_bench_fusion:   [$cfgid] $pairid ($technique) ..."
  local t0 t1 latency_ms rc
  t0=$(date +%s%N)
  # --seed "$FUSION_SEED" (fisso e registrato nel manifest, vedi testata):
  # un'immagine di fusione approvata deve poter essere rigenerata.
  "$SD_CLI" -m "$model" -p "$prompt_full" -n "$negative" \
    -W "$WIDTH" -H "$HEIGHT" --steps "$steps" --cfg-scale "$cfg_scale" \
    --sampling-method "$sampler" --scheduler "$scheduler" --seed "$FUSION_SEED" \
    "${extra_sd_args[@]}" -o "$tmp_out" -v > "$sdlog" 2>&1
  rc=$?
  t1=$(date +%s%N)
  latency_ms=$(( (t1 - t0) / 1000000 ))

  local gen_ok=0 raw_rel=""
  if [ "$rc" -eq 0 ] && [ -s "$tmp_out" ]; then
    mv "$tmp_out" "$raw_path"
    gen_ok=1
    raw_rel="raw-512/${cfgid}/${stem}.png"
    echo "runtime_bench_fusion:     ok (${latency_ms} ms)"
  else
    rm -f "$tmp_out"
    echo "runtime_bench_fusion:     FALLITO (rc=$rc, ${latency_ms} ms) -- vedi $sdlog" >&2
    mkdir -p "$FAILURES_DIR"
    {
      echo "config=$cfgid pair=$pairid technique=$technique rc=$rc latency_ms=$latency_ms note=$source_note"
      echo "--- ultime righe log sd-cli ---"
      tail -n 20 "$sdlog" 2>/dev/null
    } > "$FAILURES_DIR/${cfgid}_${stem}.txt"
  fi

  local model_sha model_license
  model_sha=$(ledger_field "$model" file_hash)
  model_license=$(ledger_field "$model" license)

  MB_CONFIG="$cfgid" MB_PAIR="$pairid" MB_TECHNIQUE="$technique" MB_SOURCE_NOTE="$source_note" \
  MB_SEED_VAL="$FUSION_SEED" MB_STRENGTH="${IMG2IMG_STRENGTH}" \
  MB_MODEL_PATH="$model" MB_MODEL_SHA="$model_sha" MB_LICENSE_MODEL="$model_license" \
  MB_PROMPT_FULL="$prompt_full" MB_NEGATIVE="$negative" \
  MB_STEPS="$steps" MB_CFG_SCALE="$cfg_scale" MB_SAMPLER="$sampler" MB_SCHEDULER="$scheduler" \
  MB_WIDTH="$WIDTH" MB_HEIGHT="$HEIGHT" MB_SD_COMMIT="$SD_CPP_COMMIT" \
  MB_LATENCY_MS="$latency_ms" MB_GEN_OK="$gen_ok" MB_RAW_REL="$raw_rel" MB_MANIFEST_PATH="$manifest_path" \
  python3 <<'PY'
import json
import os
from datetime import datetime, timezone

record = {
    "config_id": os.environ["MB_CONFIG"],
    "pair_id": os.environ["MB_PAIR"],
    "technique": os.environ["MB_TECHNIQUE"],   # "spec-fusion" | "img2img" | "controlnet"
    "source_note": os.environ.get("MB_SOURCE_NOTE") or None,
    # seed: quello PASSATO a sd-cli, non uno risolto a posteriori dal log --
    # e' fisso per costruzione (vedi FUSION_SEED in testata), quindi il campo
    # e' esattamente cio' che serve per rigenerare l'immagine. "strength" ha
    # senso solo per img2img: negli altri due manifest resta null invece di
    # portare un numero che nessuno ha usato.
    "seed": int(os.environ["MB_SEED_VAL"]),
    "strength": float(os.environ["MB_STRENGTH"]) if os.environ["MB_TECHNIQUE"] == "img2img" else None,
    "model": {
        "path": os.environ["MB_MODEL_PATH"],
        "sha256": os.environ.get("MB_MODEL_SHA") or None,
        "license": os.environ.get("MB_LICENSE_MODEL", ""),
    },
    "prompt_full": os.environ["MB_PROMPT_FULL"],
    "negative_prompt": os.environ.get("MB_NEGATIVE", ""),
    "steps": int(os.environ["MB_STEPS"]),
    "cfg_scale": float(os.environ["MB_CFG_SCALE"]),
    "sampling_method": os.environ["MB_SAMPLER"],
    "scheduler": os.environ["MB_SCHEDULER"],
    "width": int(os.environ["MB_WIDTH"]),
    "height": int(os.environ["MB_HEIGHT"]),
    "sd_cpp_commit": os.environ.get("MB_SD_COMMIT", ""),
    "latency_ms": int(os.environ["MB_LATENCY_MS"]),
    "generation_ok": os.environ["MB_GEN_OK"] == "1",
    "generated_at": datetime.now(tz=timezone.utc).isoformat(),
    "raw_image": os.environ.get("MB_RAW_REL") or None,
}
out_path = os.environ["MB_MANIFEST_PATH"]
os.makedirs(os.path.dirname(out_path), exist_ok=True)
with open(out_path, "w") as f:
    json.dump(record, f, indent=2, ensure_ascii=False)
    f.write("\n")
PY
}

# -- Una coppia su UNA config: le tre tecniche -------------------------------
generate_pair() {
  local cfgid="$1" pair="$2"
  local id_a="${pair%%:*}" id_b="${pair##*:}"
  local domain_a="${REQ_DOMAIN[$id_a]:-}" domain_b="${REQ_DOMAIN[$id_b]:-}"
  local spec_a="${REQ_SPEC_JSON[$id_a]:-}" spec_b="${REQ_SPEC_JSON[$id_b]:-}"
  local pairid="${id_a}_x_${id_b}"

  if [ -z "$domain_a" ] || [ -z "$domain_b" ]; then
    fail_pair "$cfgid" "$pairid" "all" "id assente da $BATCH_FILE (id_a='$id_a' trovato=$([ -n "$domain_a" ] && echo si || echo no), id_b='$id_b' trovato=$([ -n "$domain_b" ] && echo si || echo no))"
    return 0
  fi

  local model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler default_trigger extra_lora
  IFS=$'\x1f' read -r model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler \
    default_trigger extra_lora <<< "${CONFIG_ROW[$cfgid]}"

  # Prompt/negative di fusione: UNA sola volta per coppia (condivisi dalle
  # tre tecniche, vedi testata del file) -- visualspec_template.py fa il
  # lavoro vero, qui si legge solo il suo output NUL-delimitato.
  local fusion_prompt fusion_negative
  IFS=$'\x1f' read -r -d '' fusion_prompt fusion_negative < <(python3 - "$spec_a" "$spec_b" "$domain_a" "$domain_b" <<'PY'
import json, sys
sys.path.insert(0, "scripts")
import visualspec_template as vt

spec_a = json.loads(sys.argv[1])
spec_b = json.loads(sys.argv[2])
domain_a, domain_b = sys.argv[3], sys.argv[4]
sys.stdout.write(vt.build_fusion_prompt(spec_a, spec_b))
sys.stdout.write("\x1f")
sys.stdout.write(vt.negative_for_domains([domain_a, domain_b]))
sys.stdout.write("\0")
PY
)
  local full_prompt="${default_trigger}${fusion_prompt}"
  if [ -n "$lora_name" ]; then
    full_prompt="${full_prompt}<lora:${lora_name}:${lora_weight}>"
  fi
  local lora_args=()
  [ -n "$lora_name" ] && lora_args+=(--lora-model-dir "$lora_dir")
  if [ -n "$extra_lora" ]; then
    local ex_dir ex_name ex_w
    IFS=':' read -r ex_dir ex_name ex_w <<< "$extra_lora"
    full_prompt="${full_prompt}<lora:${PWD}/${ex_dir}/${ex_name}.safetensors:${ex_w:-1.0}>"
    [ -z "$lora_name" ] && lora_args+=(--lora-model-dir "$ex_dir")
  fi

  # (a) spec-fusion: nessuna immagine sorgente, solo il prompt di fusione.
  run_fusion_one "$cfgid" "$pairid" "spec-fusion" "$full_prompt" "$fusion_negative" \
    "$model" "$steps" "$cfg_scale" "$sampler" "$scheduler" lora_args \
    "text2img puro, nessuna sorgente strutturale"

  # (b)/(c): serve la sorgente strutturale, gli sprite pixel-64 gia'
  # generati per QUESTA config (vedi PRECONDIZIONE in testata). Assente ->
  # entrambe le tecniche saltano per QUESTA coppia+config (non fatale).
  local px_a="$RUNTIME_BENCH_ROOT/pixel-64/$cfgid/${id_a}__spec.png"
  local px_b="$RUNTIME_BENCH_ROOT/pixel-64/$cfgid/${id_b}__spec.png"
  if [ ! -f "$px_a" ] || [ ! -f "$px_b" ]; then
    fail_pair "$cfgid" "$pairid" "img2img" "sorgente pixel-64 mancante ($px_a e/o $px_b -- rilancia runtime-bench.sh + teacher_bench_post.py --root $RUNTIME_BENCH_ROOT per $cfgid prima di questo script)"
    fail_pair "$cfgid" "$pairid" "controlnet" "sorgente pixel-64 mancante ($px_a e/o $px_b -- stesso motivo di img2img sopra)"
    return 0
  fi

  # (b) img2img: composito colore dei due sprite come --init-img.
  local img2img_src="$SOURCE_DIR/${cfgid}_${pairid}__img2img-src.png"
  if [ ! -f "$img2img_src" ]; then
    python3 "$COMPOSE_PY" img2img "$px_a" "$px_b" "$img2img_src" \
      || { fail_pair "$cfgid" "$pairid" "img2img" "composizione sorgente fallita (scripts/runtime_bench_fusion_compose.py)"; img2img_src=""; }
  fi
  if [ -n "$img2img_src" ]; then
    local img2img_args=(--init-img "$img2img_src" --strength "$IMG2IMG_STRENGTH" "${lora_args[@]}")
    run_fusion_one "$cfgid" "$pairid" "img2img" "$full_prompt" "$fusion_negative" \
      "$model" "$steps" "$cfg_scale" "$sampler" "$scheduler" img2img_args \
      "init-img=$img2img_src strength=$IMG2IMG_STRENGTH"
  fi

  # (c) controlnet: maschera-contorno unione dei due alpha come --control-image.
  # Verificato coi 6GB/lcm SOLO come "il comando parte" (--help espone i flag
  # reali, vedi testata del file): se sd-cli fallisce qui (OOM, combinazione
  # non supportata dal sampler few-step) il fallimento finisce comunque in
  # failures/ da run_fusion_one -- e' il DATO richiesto dal task, non va
  # nascosto ne' fatto fallire l'intera corsa.
  if [ ! -f "$CONTROL_NET_MODEL" ]; then
    fail_pair "$cfgid" "$pairid" "controlnet" "modello ControlNet mancante: $CONTROL_NET_MODEL"
  else
    local controlnet_src="$SOURCE_DIR/${cfgid}_${pairid}__controlnet-src.png"
    if [ ! -f "$controlnet_src" ]; then
      python3 "$COMPOSE_PY" controlnet "$px_a" "$px_b" "$controlnet_src" \
        || { fail_pair "$cfgid" "$pairid" "controlnet" "composizione sorgente fallita (scripts/runtime_bench_fusion_compose.py)"; controlnet_src=""; }
    fi
    if [ -n "$controlnet_src" ]; then
      local controlnet_args=(--control-net "$CONTROL_NET_MODEL" --control-image "$controlnet_src" "${lora_args[@]}")
      run_fusion_one "$cfgid" "$pairid" "controlnet" "$full_prompt" "$fusion_negative" \
        "$model" "$steps" "$cfg_scale" "$sampler" "$scheduler" controlnet_args \
        "control-net=$CONTROL_NET_MODEL control-image=$controlnet_src"
    fi
  fi
}

echo "== runtime_bench_fusion: config ${REQUESTED_CONFIGS[*]}, ${#REQUESTED_PAIRS[@]} coppie, 3 tecniche, seed $FUSION_SEED =="
for cfgid in "${REQUESTED_CONFIGS[@]}"; do
  if [ -z "${CONFIG_ROW[$cfgid]+x}" ]; then
    echo "runtime_bench_fusion: config sconosciuta: $cfgid (--list per l'elenco)" >&2
    continue
  fi
  echo "== runtime_bench_fusion: config $cfgid =="
  for pair in "${REQUESTED_PAIRS[@]}"; do
    generate_pair "$cfgid" "$pair"
  done
done
echo "== runtime_bench_fusion: fatto. raw in $RAW_DIR, sorgenti in $SOURCE_DIR, manifest in $MANIFEST_DIR, fallimenti (se presenti) in $FAILURES_DIR =="
