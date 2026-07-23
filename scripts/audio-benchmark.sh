#!/usr/bin/env bash
# Wrapper del benchmark audio (DEC-109/DEC-113, estensione della missione
# 22/07/2026). BLOCCATO su questa macchina al 23/07/2026 -- vedi
# docs/plans/active/model-comparison.md, sezione audio, per i due blocchi
# indipendenti (gate HuggingFace + incompatibilita' di dipendenze su Python
# 3.14) e i passi ESATTI per sbloccarli. Questo script controlla le
# precondizioni e da' un messaggio chiaro invece di un traceback quando
# mancano; gira scripts/audio_benchmark.py solo se tutto e' pronto.
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

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/model-comparison/$STAMP/audio/stable-audio-open-small"
mkdir -p "$OUT"

if [ ! -x "$PY" ]; then
  cat <<EOF
audio-benchmark: venv assente o incompleto ($VENV).

Passi per prepararlo (CPU, niente GPU coinvolta):
  python3 -m venv --without-pip "$VENV"
  curl -sL https://bootstrap.pypa.io/get-pip.py | "$PY" -
  "$VENV/bin/pip" install torch --index-url https://download.pytorch.org/whl/cpu

Poi 'stable-audio-tools' -- vedi il blocco sotto, NON e' garantito installarsi
pulito su Python 3.14 (misurato 23/07/2026, vedi
docs/plans/active/model-comparison.md sezione audio per il dettaglio).
EOF
  exit 1
fi

if ! "$PY" -c "import stable_audio_tools" >/dev/null 2>&1; then
  cat <<EOF
audio-benchmark: stable-audio-tools non importabile nel venv ($VENV).

'pip install stable-audio-tools' ha hard-pin su una quindicina di pacchetti
(pandas==2.0.2, pytorch_lightning==2.1.0, sentencepiece==0.1.99, wandb==0.15.4,
torchmetrics==0.11.4, encodec==0.1.1, ...) precedenti a Python 3.14 e senza
wheel precompilata: la build da sorgente di pandas 2.0.2 fallisce gia' al
primo pacchetto (ModuleNotFoundError: pkg_resources, il setuptools moderno
non lo include piu' di default nell'ambiente di build isolato di pip).

Sblocco pulito: creare questo venv con un interprete Python 3.10-3.12 (quello
con cui stable-audio-tools e' stato pubblicato), via pyenv o conda -- non
presente su questa macchina e non installato da questo script (cambio
d'ambiente piu' grande di quanto un benchmark meriti senza autorizzazione
esplicita, vedi docs/ai-production/regole-agenti-ml.md).
EOF
  exit 1
fi

# Il modello e' gated su HuggingFace (DEC-113: licenza Stability Community
# accettata a livello di progetto, ma l'ACCETTAZIONE va fatta dall'account
# HF dell'utente che scarica -- verificato 23/07/2026, HTTP 401 su
# stabilityai/stable-audio-open-small senza token):
#   1. https://huggingface.co/stabilityai/stable-audio-open-small -> accetta
#      la licenza con un account HuggingFace;
#   2. https://huggingface.co/settings/tokens -> crea un token (basta "read");
#   3. export HF_TOKEN=hf_... (o huggingface-cli login) PRIMA di lanciare
#      questo script -- get_pretrained_model lo legge da li' via huggingface_hub.
if [ -z "${HF_TOKEN:-}" ]; then
  echo "audio-benchmark: HF_TOKEN non impostato -- il modello e' gated, vedi i passi qui sopra nel sorgente dello script." >&2
  echo "                 Tento comunque (huggingface-cli login potrebbe aver salvato un token altrove)." >&2
fi

echo "== audio-benchmark: stable-audio-open-small, 8 prompt x 2 seed -> $OUT =="
"$PY" scripts/audio_benchmark.py "$OUT"
