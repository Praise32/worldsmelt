#!/usr/bin/env bash
# runtime-bench — harness R2 del confronto RUNTIME (mandato 06/08: scegliere
# fra S1..S4 del bake-off, report docs/ai-production/experiments/
# teacher-bench-2026-08-06.md, E fra le due architetture di prompting che
# usera' il GIOCO in produzione -- "Gemma->VisualSpec JSON->template
# deterministico" contro "Gemma->prompt libero". Le due architetture NON
# sono un solo esperimento: e' un confronto APPAIATO, stessa richiesta
# attraverso ENTRAMBE, cosi' la differenza osservata e' l'architettura, non
# il soggetto.
#
# CONTRATTO D'INTERFACCIA (congelato dall'orchestratore, confine fra i due
# cantieri R1/R2): questo script legge SOLO generated/visualspecs/batch.json,
# mai scrive dentro tools/melting-gen. Schema atteso:
#   { "version":1, "seed":N, "model":"...", "requests":[
#       { "id":"<domain>_<nn>", "domain":"character|enemy|weapon|item|boss_part",
#         "spec":{"category":..., "subtype":..., "body_plan":..., "materials":[...],
#                 "distinctive_feature":..., "size_class":...},
#         "free_prompt":"..." }, ... ] }
# 10 richieste per dominio (50 totali), OGNUNA con SIA spec SIA free_prompt
# (la coppia e' il punto del confronto, vedi sopra). "seed" e' UNICO per
# l'intero batch (non una lista come nei contratti teacher-bench): tutte le
# immagini di questa corsa condividono lo stesso seed, la variabile sotto
# esame e' l'architettura di prompt/la config, non il rumore del seed.
#
# "free_prompt" E' UN PROMPT SD COMPLETO (precisazione del contratto, 06/08):
# vista + soggetto + resa + soggetto singolo + sfondo grigio piatto, scritto
# da Gemma in prosa. Non e' un dettaglio di forma: a valle del braccio "free"
# non esiste NIENTE che aggiunga la parte tecnica, mentre il braccio "spec" ha
# scripts/visualspec_template.py. Se il free_prompt fosse di solo soggetto,
# questo harness misurerebbe "con template tecnico vs senza" -- e per giunta
# con l'esito gia' deciso, visto che il postprocesso comune ai due bracci
# (flood-fill dello sfondo + rimappa di palette, scripts/teacher_bench_post.py)
# pretende lo sfondo grigio piatto che solo un braccio avrebbe chiesto.
# Chi impone il contratto e' tools/melting-gen (ValidateFreePrompt in
# gen_visualspec.c rifiuta un free_prompt che non dichiari vista, soggetto
# singolo e sfondo), non questo script: qui il campo si usa e basta.
#
# Per ogni richiesta e per ogni config (S1..S4 di default) si generano DUE
# immagini:
#   mode "spec" = trigger della config + template deterministico dello spec
#                 (scripts/visualspec_template.py:build_spec_prompt)
#   mode "free" = trigger della config + free_prompt di Gemma COSI' COM'E'
#                 (nessun suffisso: l'aderenza nuda al prompt libero e' il
#                 dato che si vuole misurare, aggiungere ganci qui
#                 falserebbe il confronto a favore del prompt libero)
# Negative: standard per DOMINIO, stile Track F (SENZA il blocco anti-outline
# di Track P/DEC-205 -- vedi scripts/visualspec_template.py), uguale per
# entrambi i modi: sd-cli richiede sempre un negative, e il negative non e'
# la variabile sotto esame quanto lo e' invece il prompt positivo.
#
# CONFIG_ROW (S1..S4) e' una copia DELIBERATA della tabella in
# scripts/teacher-bench.sh, non un source di quel file: teacher-bench.sh
# esegue Stage A per intero appena caricato (nessun guard 'if __name__ ==
# main' in bash, l'ultimo blocco dello script chiama ensure_ledger_entries()
# e generate_config() incondizionatamente) -- sourcing lo lancerebbe. La
# tabella va tenuta sincronizzata A MANO con quella riga S1..S4 di
# scripts/teacher-bench.sh se un domani cambiano modello/LoRA/step: e' il
# prezzo di restare un harness a file singolo invece di introdurre una
# libreria condivisa per quattro righe di config, non ancora giustificato.
# I campi "contract"/"subjects_filter" della tabella originale sono OMESSI
# qui: quel ruolo (quali soggetti generare) lo gioca gia' batch.json.
#
# Uso:
#   scripts/runtime-bench.sh [CONFIG_ID ...]      (default: S1 S2 S3 S4)
#   scripts/runtime-bench.sh --list                elenca le config note ed esce
# Variabili:
#   BATCH_FILE   default generated/visualspecs/batch.json (contratto R1). Per
#                i giri di PROVA (DRY_RUN, postproc su PNG sintetici) usare
#                sempre scripts/testdata/visualspec_batch_fixture.json: il
#                path di produzione lo riscrive `melting-gen --visualspecs` e
#                una fixture parcheggiata li' sparisce a meta' sessione senza
#                che nessuno se ne accorga (successo davvero il 06/08).
#   SD_CLI       default deps/stable-diffusion.cpp/build/bin/sd-cli
#   OUT_ROOT     default artifacts/runtime-bench
#   WIDTH/HEIGHT default 512 512 (stessa cartella raw-512/ dei teacher-bench)
#   DRY_RUN=1    stampa i comandi sd-cli pianificati senza invocare la GPU
#
# Resume/incrementalita': un'immagine con
# raw-512/<config>/<reqid>__<modo>.png gia' presente viene SALTATA (nessun
# controllo di drift qui, a differenza di teacher-bench.sh: batch.json e'
# un contratto CONGELATO per l'intera corsa di un giorno, non un file che
# accumula versioni nel tempo come i contratti teacher-bench -- se cambia,
# e' onere di chi rilancia svuotare raw-512/ a mano). Un fallimento di UNA
# immagine (sd-cli non-zero o file di output assente/vuoto) NON ferma la
# suite (niente 'set -e'): registrato in failures/, si continua.
set -uo pipefail
cd "$(dirname "$0")/.."

OUT_ROOT="${OUT_ROOT:-artifacts/runtime-bench}"
RAW_DIR="$OUT_ROOT/raw-512"
MANIFEST_DIR="$OUT_ROOT/manifests"
FAILURES_DIR="$OUT_ROOT/failures"
LOG_DIR="logs/runtime-bench"   # log grezzi sd-cli per immagine, gia' gitignored (vedi .gitignore, stessa regola di logs/teacher-bench)

BATCH_FILE="${BATCH_FILE:-generated/visualspecs/batch.json}"
SD_CLI="${SD_CLI:-deps/stable-diffusion.cpp/build/bin/sd-cli}"
# Ledger di provenienza (sha256/licenza) di scripts/teacher-bench.sh: questo
# script lo legge SOLO in lettura (mai lo scrive/aggiorna) -- i modelli/LoRA
# S1..S4 sono gia' scaricati e ledger-ati da un giro precedente di
# teacher-bench.sh; se manca, i campi sha256/license del manifest restano
# vuoti (onesto: "non verificato qui", non un dato inventato).
LEDGER_FILE="${LEDGER_FILE:-artifacts/image-model-research/manifests/model-ledger.json}"
WIDTH="${WIDTH:-512}"
HEIGHT="${HEIGHT:-512}"
DRY_RUN="${DRY_RUN:-0}"

VISUALSPEC_TEMPLATE="scripts/visualspec_template.py"
DEFAULT_CONFIGS=(S1 S2 S3 S4)

if [ "${1:-}" = "--list" ]; then
  echo "config note: S1 S2 S3 S4 (copia di CONFIG_ROW in scripts/teacher-bench.sh, asse velocita' -- default: ${DEFAULT_CONFIGS[*]})"
  exit 0
fi

[ -f "$BATCH_FILE" ] || { echo "runtime-bench: batch mancante: $BATCH_FILE (contratto R1, tools/melting-gen --visualspecs)" >&2; exit 1; }
[ -f "$VISUALSPEC_TEMPLATE" ] || { echo "runtime-bench: manca $VISUALSPEC_TEMPLATE" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "runtime-bench: python3 richiesto" >&2; exit 1; }
if [ "$DRY_RUN" != "1" ]; then
  [ -x "$SD_CLI" ] || { echo "runtime-bench: sd-cli non trovato/eseguibile: $SD_CLI (compila deps/stable-diffusion.cpp)" >&2; exit 1; }
  # REGOLA FERREA GPU (CLAUDE.md/mandato): mai due processi GPU insieme.
  # SENZA -f, stesso motivo documentato in scripts/teacher-bench.sh (pgrep -f
  # confronta la riga di comando intera e puo' auto-corrispondere allo
  # script che sta facendo il controllo in alcuni ambienti -- verificato in
  # sessione su quello script gemello).
  if pgrep 'sd-cli|melting-sprites|melting-gen' >/dev/null 2>&1; then
    echo "runtime-bench: un processo GPU (sd-cli/melting-sprites/melting-gen) e' gia' in esecuzione -- attendi e rilancia." >&2
    exit 1
  fi
fi

SD_CPP_COMMIT="unknown"
if [ -d deps/stable-diffusion.cpp/.git ]; then
  SD_CPP_COMMIT=$(git -C deps/stable-diffusion.cpp rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
fi
TIME_BIN_AVAILABLE=""
command -v /usr/bin/time >/dev/null 2>&1 && TIME_BIN_AVAILABLE=1

mkdir -p "$RAW_DIR" "$MANIFEST_DIR" "$FAILURES_DIR" "$LOG_DIR"

# -- Tabella delle config (copia di S1..S4, vedi testata) --------------------
# Campi (separatore \x1f, stesso motivo di scripts/teacher-bench.sh: i
# commenti italiani di questo repo sono pieni di apostrofi che dentro un
# $'...' ANSI-C andrebbero escappati uno per uno):
#   model_path \x1f lora_dir \x1f lora_name \x1f lora_weight \x1f steps \x1f
#   cfg_scale \x1f sampler \x1f scheduler \x1f default_trigger \x1f extra_lora
declare -A CONFIG_ROW
CONFIG_ROW[S1]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f6\x1f1.5\x1flcm\x1flcm\x1fbasepixel, \x1fmodels:lcm-lora-sdv1-5:1.0'
CONFIG_ROW[S2]=$'models/teacher-bench-2026-08/DreamShaper_8_pruned.safetensors\x1fmodels/teacher-bench-2026-08\x1fbasepixel-20\x1f0.6\x1f8\x1f5\x1feuler_a\x1fkarras\x1fbasepixel, \x1fmodels/teacher-bench-2026-08:Hyper-SD15-8steps-CFG-lora:1.0'
CONFIG_ROW[S3]=$'models/teacher-bench-2026-08/anyloraCheckpoint_lcm.safetensors\x1fmodels/teacher-bench-2026-08\x1f8bitdiffuser64-v4-PX64NOCAP_epoch_10\x1f1.0\x1f6\x1f1.5\x1flcm\x1flcm\x1fpixel_art, \x1f'
CONFIG_ROW[S4]=$'models/teacher-bench-2026-08/DreamShaper8_LCM.safetensors\x1f\x1f\x1f\x1f8\x1f2\x1flcm\x1flcm\x1f\x1f'

declare -A CONFIG_NOTE
CONFIG_NOTE[S1]="S1: DreamShaper 8 + Basepixel 0.6 + LCM-LoRA 1.0 (extra_lora), lcm, 6 step, CFG 1.5"
CONFIG_NOTE[S2]="S2: DreamShaper 8 + Basepixel 0.6 + Hyper-SD15 8-step-CFG-LoRA 1.0 (extra_lora), euler_a/karras, 8 step, CFG 5"
CONFIG_NOTE[S3]="S3: AnyLoRA-LCM (checkpoint gia' fuso) + 8bitdiffuser64 1.0, lcm, 6 step, CFG 1.5"
CONFIG_NOTE[S4]="S4: DreamShaper8_LCM da solo, lcm, 8 step, CFG 2"

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

# -- Caricamento di batch.json -------------------------------------------------
# BATCH_SEED/BATCH_MODEL: metadati di tutto il batch (item "seed"/"model" del
# contratto), letti UNA sola volta -- BATCH_MODEL e' solo informativo (quale
# Gemma ha scritto batch.json, finisce nel manifest per provenienza) e non
# influenza la generazione immagine.
IFS=$'\x1f' read -r BATCH_VERSION BATCH_SEED BATCH_MODEL < <(python3 -c "
import json, sys
d = json.load(open(sys.argv[1]))
sys.stdout.write(str(d.get('version','')) + '\x1f' + str(d.get('seed','')) + '\x1f' + str(d.get('model','')))
" "$BATCH_FILE")
[ -n "$BATCH_SEED" ] || { echo "runtime-bench: batch.json senza 'seed' valido: $BATCH_FILE" >&2; exit 1; }

# REQ_FIELDS: un record NUL-delimitato per richiesta (id, domain, spec_json,
# spec_prompt, negative, free_prompt) -- costruito da
# scripts/visualspec_template.py, che e' anche la SOLA fonte del template
# deterministico e dei negative per dominio (niente logica di prompt
# duplicata qui in bash).
mapfile -d '' REQ_FIELDS < <(python3 "$VISUALSPEC_TEMPLATE" --batch "$BATCH_FILE")
if [ "${#REQ_FIELDS[@]}" -eq 0 ] || [ $(( ${#REQ_FIELDS[@]} % 6 )) -ne 0 ]; then
  echo "runtime-bench: $BATCH_FILE non contiene 'requests' valide (o $VISUALSPEC_TEMPLATE ha fallito)" >&2
  exit 1
fi
N_REQUESTS=$(( ${#REQ_FIELDS[@]} / 6 ))

# -- Parsing argomenti --------------------------------------------------------
if [ $# -gt 0 ]; then
  REQUESTED_CONFIGS=("$@")
else
  REQUESTED_CONFIGS=("${DEFAULT_CONFIGS[@]}")
fi

# -- Generazione di una singola immagine --------------------------------------
run_one() {
  local cfgid="$1" mode="$2" reqid="$3" domain="$4" spec_json="$5" \
        positive_body="$6" negative="$7"
  local model="$8" lora_dir="$9" lora_name="${10}" lora_weight="${11}" \
        steps="${12}" cfg_scale="${13}" sampler="${14}" scheduler="${15}" \
        prefix="${16}" extra_lora="${17}"

  local raw_dir="$RAW_DIR/$cfgid"
  local stem="${reqid}__${mode}"
  local raw_path="$raw_dir/${stem}.png"
  # Manifest = "{config}_{stem}.json": stessa convenzione di
  # scripts/teacher-bench.sh ("{cfgid}_{sid}_{seed}.json"), cosi'
  # scripts/teacher_bench_post.py (che ricalcola questo stesso path da
  # raw_path.stem senza modifiche) trova SEMPRE il manifest giusto -- vedi
  # rows_from_manifest/process_one li', invariati per questo harness.
  local manifest_path="$MANIFEST_DIR/${cfgid}_${stem}.json"

  if [ -f "$raw_path" ]; then
    echo "runtime-bench:   [$cfgid] $reqid mode=$mode gia' presente, salto (resume)"
    return 0
  fi
  mkdir -p "$raw_dir"

  local lora_path="" extra_lora_dir="" extra_lora_name="" extra_lora_weight=""
  local full_prompt="${prefix}${positive_body}"
  local extra_args=()
  if [ -n "$lora_name" ]; then
    lora_path="$lora_dir/$lora_name.safetensors"
    full_prompt="${full_prompt}<lora:${lora_name}:${lora_weight}>"
    extra_args+=(--lora-model-dir "$(dirname "$lora_path")")
  fi
  # extra_lora ("dir:nome:peso"): stesso trucco path-assoluto di
  # scripts/teacher-bench.sh (sd-cli estrae <lora:...> solo se
  # --lora-model-dir non e' vuoto; un path assoluto bypassa quella cartella).
  if [ -n "$extra_lora" ]; then
    IFS=':' read -r extra_lora_dir extra_lora_name extra_lora_weight <<< "$extra_lora"
    extra_lora_weight="${extra_lora_weight:-1.0}"
    local extra_lora_rel="$extra_lora_dir/$extra_lora_name.safetensors"
    full_prompt="${full_prompt}<lora:${PWD}/${extra_lora_rel}:${extra_lora_weight}>"
    if [ -z "$lora_name" ]; then
      extra_args+=(--lora-model-dir "$(dirname "$extra_lora_rel")")
    fi
  fi

  if [ "$DRY_RUN" = "1" ]; then
    printf 'DRY-RUN [%s] %s mode=%s domain=%s :: %s -m %s -p %q -n %q -W %s -H %s --steps %s --cfg-scale %s --sampling-method %s --scheduler %s --seed %s %s -o %s\n' \
      "$cfgid" "$reqid" "$mode" "$domain" "$SD_CLI" "$model" "$full_prompt" "$negative" \
      "$WIDTH" "$HEIGHT" "$steps" "$cfg_scale" "$sampler" "$scheduler" "$BATCH_SEED" "${extra_args[*]:-}" "$raw_path"
    return 0
  fi

  local tmp_out="${raw_path}.tmp.png"
  local sdlog="$LOG_DIR/${cfgid}_${stem}.log"
  local timelog="$LOG_DIR/${cfgid}_${stem}.time.txt"

  echo "runtime-bench:   [$cfgid] $reqid mode=$mode ..."
  local t0 t1 latency_ms rc
  t0=$(date +%s%N)
  if [ -n "$TIME_BIN_AVAILABLE" ]; then
    /usr/bin/time -v -o "$timelog" "$SD_CLI" -m "$model" -p "$full_prompt" -n "$negative" \
      -W "$WIDTH" -H "$HEIGHT" --steps "$steps" --cfg-scale "$cfg_scale" \
      --sampling-method "$sampler" --scheduler "$scheduler" --seed "$BATCH_SEED" \
      "${extra_args[@]}" -o "$tmp_out" -v > "$sdlog" 2>&1
    rc=$?
  else
    "$SD_CLI" -m "$model" -p "$full_prompt" -n "$negative" \
      -W "$WIDTH" -H "$HEIGHT" --steps "$steps" --cfg-scale "$cfg_scale" \
      --sampling-method "$sampler" --scheduler "$scheduler" --seed "$BATCH_SEED" \
      "${extra_args[@]}" -o "$tmp_out" -v > "$sdlog" 2>&1
    rc=$?
  fi
  t1=$(date +%s%N)
  latency_ms=$(( (t1 - t0) / 1000000 ))

  local gen_ok=0 raw_rel=""
  if [ "$rc" -eq 0 ] && [ -s "$tmp_out" ]; then
    mv "$tmp_out" "$raw_path"
    gen_ok=1
    raw_rel="raw-512/${cfgid}/${stem}.png"
    echo "runtime-bench:     ok (${latency_ms} ms)"
  else
    rm -f "$tmp_out"
    echo "runtime-bench:     FALLITO (rc=$rc, ${latency_ms} ms) -- vedi $sdlog" >&2
    mkdir -p "$FAILURES_DIR"
    {
      echo "config=$cfgid request=$reqid mode=$mode domain=$domain rc=$rc latency_ms=$latency_ms"
      echo "--- ultime righe log sd-cli ---"
      tail -n 20 "$sdlog" 2>/dev/null
    } > "$FAILURES_DIR/${cfgid}_${stem}.txt"
  fi

  local rss_kb="" vram_note=""
  if [ -f "$timelog" ]; then
    rss_kb=$(grep -oE 'Maximum resident set size \(kbytes\): [0-9]+' "$timelog" 2>/dev/null | grep -oE '[0-9]+$' || true)
  fi
  vram_note=$(grep -oiE '[a-z_]*(vram|compute buffer|params? buffer)[a-z_ :]*[0-9]+(\.[0-9]+)? ?(MB|GB)' "$sdlog" 2>/dev/null | head -3 | tr '\n' ';' || true)

  local model_sha model_license lora_sha lora_license extra_lora_sha extra_lora_license
  model_sha=$(ledger_field "$model" file_hash)
  model_license=$(ledger_field "$model" license)
  lora_sha="" lora_license=""
  if [ -n "$lora_path" ]; then
    lora_sha=$(ledger_field "$lora_path" file_hash)
    lora_license=$(ledger_field "$lora_path" license)
  fi
  extra_lora_sha="" extra_lora_license=""
  if [ -n "$extra_lora" ]; then
    extra_lora_sha=$(ledger_field "$extra_lora_dir/$extra_lora_name.safetensors" file_hash)
    extra_lora_license=$(ledger_field "$extra_lora_dir/$extra_lora_name.safetensors" license)
  fi

  MB_CONFIG="$cfgid" MB_REQID="$reqid" MB_DOMAIN="$domain" MB_MODE="$mode" \
  MB_SPEC_JSON="$spec_json" MB_POSITIVE_BODY="$positive_body" \
  MB_SEED_VAL="$BATCH_SEED" MB_BATCH_VERSION="$BATCH_VERSION" MB_BATCH_MODEL="$BATCH_MODEL" MB_BATCH_FILE="$BATCH_FILE" \
  MB_MODEL_PATH="$model" MB_MODEL_SHA="$model_sha" MB_LICENSE_MODEL="$model_license" \
  MB_LORA_PATH="$lora_path" MB_LORA_SHA="$lora_sha" MB_LORA_WEIGHT="$lora_weight" \
  MB_LORA_TRIGGER="$prefix" MB_LICENSE_LORA="$lora_license" \
  MB_EXTRA_LORA_PATH="${extra_lora:+$extra_lora_dir/$extra_lora_name.safetensors}" MB_EXTRA_LORA_SHA="$extra_lora_sha" \
  MB_EXTRA_LORA_WEIGHT="$extra_lora_weight" MB_LICENSE_EXTRA_LORA="$extra_lora_license" \
  MB_PROMPT_FULL="$full_prompt" MB_NEGATIVE="$negative" \
  MB_STEPS="$steps" MB_CFG_SCALE="$cfg_scale" MB_SAMPLER="$sampler" MB_SCHEDULER="$scheduler" \
  MB_WIDTH="$WIDTH" MB_HEIGHT="$HEIGHT" MB_SD_COMMIT="$SD_CPP_COMMIT" \
  MB_LATENCY_MS="$latency_ms" MB_RSS_KB="$rss_kb" MB_VRAM_NOTE="$vram_note" \
  MB_GEN_OK="$gen_ok" MB_RAW_REL="$raw_rel" MB_MANIFEST_PATH="$manifest_path" \
  python3 <<'PY'
import json
import os
from datetime import datetime, timezone


def envf(key, cast=None):
    v = os.environ.get(key, "")
    if v == "":
        return None
    return cast(v) if cast else v


mode = os.environ["MB_MODE"]
reqid = os.environ["MB_REQID"]
record = {
    "config_id": os.environ["MB_CONFIG"],
    # "subject_id"/"category" con lo STESSO nome di campo dei manifest
    # teacher-bench (scripts/teacher-bench.sh run_one): scripts/
    # teacher_bench_review.py e teacher_bench_post.py leggono gia' quei nomi,
    # riusarli evita un secondo schema da tenere sincronizzato. DEVE pero'
    # includere il modo (== lo stem del raw PNG, "<reqid>__<mode>"), NON il
    # solo reqid: scripts/teacher_bench_post.py chiave le righe di
    # metrics.csv su (config, subject, seed, canvas_size), e "seed" qui e'
    # lo STESSO in tutto il batch (un solo "seed" nel contratto, non una
    # lista per soggetto come nei contratti teacher-bench) -- con subject_id
    # = solo reqid, il manifest "free" e quello "spec" della stessa richiesta
    # avrebbero chiave IDENTICA e uno sovrascriverebbe l'altro in metrics.csv
    # (bug verificato in sessione: "3 righe da 2 manifest" invece di 6).
    # "request_id" (sotto) resta il reqid PURO, per chi deve accoppiare
    # spec/free della stessa richiesta (scripts/teacher_bench_review.py).
    "subject_id": f"{reqid}__{mode}",
    "request_id": reqid,
    "category": os.environ["MB_DOMAIN"],
    "domain": os.environ["MB_DOMAIN"],   # colonna extra richiesta (item 2 R2/R3): esplicita, non dedotta da 'category'
    "mode": mode,                        # "spec" | "free" -- colonna extra richiesta
    "seed": int(os.environ["MB_SEED_VAL"]),
    "batch_version": envf("MB_BATCH_VERSION"),
    "batch_model": envf("MB_BATCH_MODEL"),   # provenienza: quale Gemma ha scritto batch.json (informativo)
    "batch_file": os.environ.get("MB_BATCH_FILE", ""),
    "model": {
        "path": os.environ["MB_MODEL_PATH"],
        "sha256": envf("MB_MODEL_SHA"),
        "license": os.environ.get("MB_LICENSE_MODEL", ""),
    },
    "lora": None,
    "generation_mode": "text-only",
    "prompt_full": os.environ["MB_PROMPT_FULL"],
    "negative_prompt": os.environ.get("MB_NEGATIVE", ""),
    "steps": int(os.environ["MB_STEPS"]),
    "cfg_scale": float(os.environ["MB_CFG_SCALE"]),
    "sampling_method": os.environ["MB_SAMPLER"],
    "scheduler": os.environ["MB_SCHEDULER"],
    "width": int(os.environ["MB_WIDTH"]),
    "height": int(os.environ["MB_HEIGHT"]),
    "sd_cpp_commit": os.environ.get("MB_SD_COMMIT", ""),
    "latency_ms": envf("MB_LATENCY_MS", int),
    "rss_kb": envf("MB_RSS_KB", int),
    "vram_note": envf("MB_VRAM_NOTE"),
    "generation_ok": os.environ["MB_GEN_OK"] == "1",
    "generated_at": datetime.now(tz=timezone.utc).isoformat(),
    "raw_image": os.environ.get("MB_RAW_REL") or None,
    "postproc": None,   # riempito da scripts/teacher_bench_post.py in un secondo momento
}
lora_path = os.environ.get("MB_LORA_PATH", "")
if lora_path:
    record["lora"] = {
        "path": lora_path,
        "sha256": envf("MB_LORA_SHA"),
        "weight": float(os.environ.get("MB_LORA_WEIGHT", "0") or 0),
        "trigger": os.environ.get("MB_LORA_TRIGGER") or None,
        "license": os.environ.get("MB_LICENSE_LORA", ""),
    }
record["extra_lora"] = None
extra_lora_path = os.environ.get("MB_EXTRA_LORA_PATH", "")
if extra_lora_path:
    record["extra_lora"] = {
        "path": extra_lora_path,
        "sha256": envf("MB_EXTRA_LORA_SHA"),
        "weight": float(os.environ.get("MB_EXTRA_LORA_WEIGHT", "0") or 0),
        "license": os.environ.get("MB_LICENSE_EXTRA_LORA", ""),
    }

# Lo "spec/prompt usato" (item 2 R2/R3): mode "spec" porta lo spec ORIGINALE
# (dict), mode "free" porta il free_prompt grezzo di Gemma -- MAI entrambi,
# cosi' chi legge un manifest capisce a colpo d'occhio quale architettura ha
# generato QUESTA immagine senza dover incrociare "mode" con due campi.
if mode == "spec":
    record["spec"] = json.loads(os.environ["MB_SPEC_JSON"])
    record["free_prompt"] = None
else:
    record["spec"] = None
    record["free_prompt"] = os.environ.get("MB_POSITIVE_BODY", "")

out_path = os.environ["MB_MANIFEST_PATH"]
os.makedirs(os.path.dirname(out_path), exist_ok=True)
with open(out_path, "w") as f:
    json.dump(record, f, indent=2, ensure_ascii=False)
    f.write("\n")
PY
}

# -- Generazione di una config intera (N richieste x 2 modi) -----------------
generate_config() {
  local cfgid="$1"
  if [ -z "${CONFIG_ROW[$cfgid]+x}" ]; then
    echo "runtime-bench: config sconosciuta: $cfgid (--list per l'elenco)" >&2
    return 1
  fi
  local model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler default_trigger extra_lora
  IFS=$'\x1f' read -r model lora_dir lora_name lora_weight steps cfg_scale sampler scheduler \
    default_trigger extra_lora <<< "${CONFIG_ROW[$cfgid]}"
  echo "== runtime-bench: $cfgid -- ${CONFIG_NOTE[$cfgid]:-} =="

  if [ ! -f "$model" ]; then
    echo "runtime-bench: [$cfgid] modello mancante ($model) -- salto l'intera config" >&2
    mkdir -p "$FAILURES_DIR"
    printf 'config=%s reason=model_missing path=%s\n' "$cfgid" "$model" > "$FAILURES_DIR/${cfgid}-model-missing.txt"
    return 0
  fi
  if [ -n "$lora_name" ] && [ ! -f "$lora_dir/$lora_name.safetensors" ]; then
    echo "runtime-bench: [$cfgid] LoRA mancante ($lora_dir/$lora_name.safetensors) -- salto l'intera config" >&2
    mkdir -p "$FAILURES_DIR"
    printf 'config=%s reason=lora_missing path=%s\n' "$cfgid" "$lora_dir/$lora_name.safetensors" > "$FAILURES_DIR/${cfgid}-lora-missing.txt"
    return 0
  fi
  if [ -n "$extra_lora" ]; then
    local ex_dir ex_name
    IFS=':' read -r ex_dir ex_name _ <<< "$extra_lora"
    if [ ! -f "$ex_dir/$ex_name.safetensors" ]; then
      echo "runtime-bench: [$cfgid] extra LoRA mancante ($ex_dir/$ex_name.safetensors) -- salto l'intera config" >&2
      mkdir -p "$FAILURES_DIR"
      printf 'config=%s reason=extra_lora_missing path=%s\n' "$cfgid" "$ex_dir/$ex_name.safetensors" > "$FAILURES_DIR/${cfgid}-extra-lora-missing.txt"
      return 0
    fi
  fi

  echo "runtime-bench:   $N_REQUESTS richieste x 2 modi (spec/free), seed=$BATCH_SEED (batch v${BATCH_VERSION:-?}, modello dichiarante ${BATCH_MODEL:-?})"

  local i reqid domain spec_json spec_prompt negative free_prompt
  for ((i = 0; i < ${#REQ_FIELDS[@]}; i += 6)); do
    reqid="${REQ_FIELDS[i]}"; domain="${REQ_FIELDS[i+1]}"; spec_json="${REQ_FIELDS[i+2]}"
    spec_prompt="${REQ_FIELDS[i+3]}"; negative="${REQ_FIELDS[i+4]}"; free_prompt="${REQ_FIELDS[i+5]}"

    run_one "$cfgid" "spec" "$reqid" "$domain" "$spec_json" "$spec_prompt" "$negative" \
      "$model" "$lora_dir" "$lora_name" "$lora_weight" "$steps" "$cfg_scale" "$sampler" "$scheduler" \
      "$default_trigger" "$extra_lora"
    run_one "$cfgid" "free" "$reqid" "$domain" "$spec_json" "$free_prompt" "$negative" \
      "$model" "$lora_dir" "$lora_name" "$lora_weight" "$steps" "$cfg_scale" "$sampler" "$scheduler" \
      "$default_trigger" "$extra_lora"
  done
}

echo "== runtime-bench: config richieste: ${REQUESTED_CONFIGS[*]} ($N_REQUESTS richieste x 2 modi da $BATCH_FILE) =="
for cfgid in "${REQUESTED_CONFIGS[@]}"; do
  generate_config "$cfgid"
done
echo "== runtime-bench: fatto. raw in $RAW_DIR, manifest in $MANIFEST_DIR, fallimenti (se presenti) in $FAILURES_DIR =="
