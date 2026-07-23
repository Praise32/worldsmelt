#!/usr/bin/env bash
# Wrapper del benchmark audio (DEC-109/DEC-113). SBLOCCATO il 23/07/2026 con
# Stable Audio 3 Small (due checkpoint, sfx+music, scaricati in
# models/stable-audio-3-small-{sfx,music}/ -- non versionati) e la libreria
# `stable-audio-3` (non su PyPI, installata da git). I due blocchi storici
# documentati in docs/plans/active/model-comparison.md (gate HF su
# stable-audio-open-small, incompatibilita' di stable-audio-tools con Python
# 3.14) non si applicano: modello diverso (gia' accettato/scaricato con
# token separato) e libreria diversa (nessun hard-pin incompatibile).
#
# CPU-only per costruzione (niente ROCm per torch sulla RX 5600 XT): puo'
# girare IN PARALLELO a scripts/model-comparison.sh (testo, GPU) o
# scripts/image-comparison.sh (immagini, GPU) senza violare la regola "mai
# due processi GPU insieme" -- non tocca mai la VRAM. NON lanciarlo pero'
# durante una MISURA di tok/s o img/s di cui serve il tempo pulito: la CPU e'
# condivisa, e un carico concorrente sposta quei numeri.
#
# Uso: scripts/audio-benchmark.sh [venv-dir]   (default: ~/venvs/stable-audio)
set -uo pipefail
cd "$(dirname "$0")/.."

VENV="${1:-$HOME/venvs/stable-audio}"
PY="$VENV/bin/python3"

SFX_DIR="models/stable-audio-3-small-sfx"
MUSIC_DIR="models/stable-audio-3-small-music"

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/model-comparison/audio-$STAMP"
mkdir -p "$OUT"

if [ ! -x "$PY" ]; then
  cat <<EOF
audio-benchmark: venv assente ($VENV).

Ricetta verificata il 23/07/2026 (Python di sistema 3.14, niente modulo pip
-- serve un interprete 3.10-3.12 per stable-audio-3, procurato con uv senza
sudo):
  curl -LsSf https://astral.sh/uv/install.sh | sh        # installa uv a livello utente in ~/.local/bin
  export PATH="\$HOME/.local/bin:\$PATH"
  uv venv "$VENV" --python 3.12                          # uv scarica da solo il CPython standalone

  # torch/torchaudio CPU pinnati alla versione ESATTA richiesta da stable-audio-3
  # (pyproject.toml del progetto: torch==2.7.1/torchaudio==2.7.1, con
  # [tool.uv.sources] che punta all'indice CUDA 12.6 su linux x86_64 -- va
  # scavalcato installando prima la build CPU alla stessa versione):
  uv pip install --python "$PY" torch==2.7.1 torchaudio==2.7.1 \\
      --index-url https://download.pytorch.org/whl/cpu

  # stable-audio-3 non e' su PyPI (solo su GitHub): --no-deps per non farsi
  # sovrascrivere torch con la build CUDA che il pyproject.toml richiederebbe
  # di default:
  uv pip install --python "$PY" "git+https://github.com/Stability-AI/stable-audio-3" --no-deps

  # le altre dipendenze runtime (torch/torchaudio esclusi, restano quelli CPU sopra):
  uv pip install --python "$PY" "einops>=0.8.2" "numpy>=2.2.6" "transformers>=5.8.0" \\
      "huggingface-hub>=1.7.1" "soundfile>=0.13.1" safetensors tokenizers tqdm rich \\
      typer shellingham pyyaml regex packaging
EOF
  exit 1
fi

if ! "$PY" -c "import stable_audio_3" >/dev/null 2>&1; then
  echo "audio-benchmark: stable_audio_3 non importabile nel venv ($VENV) -- rilancia $0 senza argomenti per la ricetta di installazione." >&2
  exit 1
fi

# I pesi vanno scaricati a mano (repo gated su HuggingFace, token utente con
# licenza Stability Community accettata -- DEC-113) in models/stable-audio-3-small-{sfx,music}/.
# Il benchmark li carica DAI FILE LOCALI (scripts/audio_benchmark.py,
# load_local_model): nessuna chiamata di rete a generazione, quindi qui basta
# verificare che i file esistano, non serve alcun HF_TOKEN per lanciare lo script.
for d in "$SFX_DIR" "$MUSIC_DIR"; do
  if [ ! -f "$d/model.safetensors" ] || [ ! -f "$d/model_config.json" ] || [ ! -f "$d/t5gemma-b-b-ul2/model.safetensors" ]; then
    cat <<EOF
audio-benchmark: pesi mancanti o incompleti in $d.

Attesi: model.safetensors, model_config.json, t5gemma-b-b-ul2/{model.safetensors,tokenizer.json,tokenizer.model,tokenizer_config.json,config.json,generation_config.json,special_tokens_map.json}.
I repo sono gated su HuggingFace (stabilityai/stable-audio-3-small-sfx e -music):
accetta la licenza sulla pagina del modello con un account HF, crea un token
in https://huggingface.co/settings/tokens, poi scarica i file elencati sopra
con quel token (curl -H "Authorization: Bearer \$TOKEN" o huggingface-cli
download) dentro $d/.
EOF
    exit 1
  fi
done

echo "== audio-benchmark: Stable Audio 3 Small (sfx 6 prompt x2 seed @4s, music 4 prompt x2 seed @20s) -> $OUT =="
echo "   caricamento dai file locali, nessun HF_TOKEN necessario per questa fase"

RUN_LOG="$OUT/run.log"
TIME_LOG="$OUT/time-v.log"
if command -v /usr/bin/time >/dev/null 2>&1; then
  /usr/bin/time -v "$PY" scripts/audio_benchmark.py "$OUT" 2>&1 | tee "$RUN_LOG"
  status=${PIPESTATUS[0]}
  # /usr/bin/time -v scrive il suo report su stderr, gia' catturato in RUN_LOG
  # insieme allo stdout dello script -- lo separiamo qui per comodita' di lettura.
  grep -E "Maximum resident set size|Elapsed \(wall clock\)|Percent of CPU" "$RUN_LOG" > "$TIME_LOG" || true
else
  "$PY" scripts/audio_benchmark.py "$OUT" 2>&1 | tee "$RUN_LOG"
  status=${PIPESTATUS[0]}
fi

if [ "$status" -ne 0 ]; then
  echo "audio-benchmark: scripts/audio_benchmark.py e' uscito con codice $status -- vedi $RUN_LOG" >&2
  exit "$status"
fi

# Appende al report.md generato da audio_benchmark.py le note d'ambiente che
# solo la shell puo' osservare: RAM di picco esterna (/usr/bin/time -v, se
# disponibile) e warning unici emersi nel log (torch/transformers/flash_attn
# mancante ecc.) -- il report resta un unico file da rivedere.
{
  echo ""
  echo "## Note ambiente"
  echo ""
  if [ -s "$TIME_LOG" ]; then
    echo "Misura esterna (/usr/bin/time -v, processo intero):"
    echo ""
    sed 's/^/- /' "$TIME_LOG"
    echo ""
  fi
  warnings=$(grep -iE "warning|deprecat" "$RUN_LOG" | sort -u)
  if [ -n "$warnings" ]; then
    echo "Warning unici osservati nel log (vedi $RUN_LOG per il contesto completo):"
    echo ""
    echo "$warnings" | sed 's/^/- /'
  else
    echo "Nessun warning osservato nel log."
  fi
} >> "$OUT/report.md"

echo "== audio-benchmark: fatto, vedi $OUT/report.md =="
