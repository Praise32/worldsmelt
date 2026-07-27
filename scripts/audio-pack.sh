#!/usr/bin/env bash
# Wrapper della produzione del pacchetto audio della demo (DEC-172).
# Genera assets/audio/ (musica/ambience/SFX in OGG 44.1 kHz + manifest.json)
# con scripts/audio-pack.py e i checkpoint locali di Stable Audio 3 Small.
# Stessi prerequisiti del benchmark (scripts/audio-benchmark.sh: venv
# ~/venvs/stable-audio con la ricetta documentata li', pesi scaricati a mano
# nei due models/stable-audio-3-small-{sfx,music}/) piu' ffmpeg con libvorbis
# per la codifica OGG.
#
# CPU-only per costruzione (come il benchmark): non tocca mai la VRAM, puo'
# girare in parallelo ai carichi GPU ma NON durante una misura di tempi
# pulita -- la CPU e' condivisa. Tempo atteso a regime: ~10 min per il
# pacchetto completo (musica a ~0.68x realtime, benchmark del 23/07/2026).
#
# Uso: scripts/audio-pack.sh [argomenti passati a audio-pack.py]
#      es. scripts/audio-pack.sh --only boss,ui_confirm
# Il venv si cambia con AUDIO_PACK_VENV=/percorso scripts/audio-pack.sh ...
set -uo pipefail
cd "$(dirname "$0")/.."

VENV="${AUDIO_PACK_VENV:-$HOME/venvs/stable-audio}"
PY="$VENV/bin/python3"

SFX_DIR="models/stable-audio-3-small-sfx"
MUSIC_DIR="models/stable-audio-3-small-music"

STAMP=$(date +%Y%m%d-%H%M%S)
OUT="logs/audio-pack/$STAMP"
mkdir -p "$OUT"

if [ ! -x "$PY" ]; then
  echo "audio-pack: venv assente ($VENV) -- lancia scripts/audio-benchmark.sh senza argomenti per la ricetta di installazione." >&2
  exit 1
fi

if ! "$PY" -c "import stable_audio_3, soundfile, numpy" >/dev/null 2>&1; then
  echo "audio-pack: dipendenze non importabili nel venv ($VENV) -- vedi scripts/audio-benchmark.sh per la ricetta." >&2
  exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "audio-pack: ffmpeg non trovato nel PATH (serve libvorbis per la codifica OGG)." >&2
  exit 1
fi

for d in "$SFX_DIR" "$MUSIC_DIR"; do
  if [ ! -f "$d/model.safetensors" ] || [ ! -f "$d/model_config.json" ] || [ ! -f "$d/t5gemma-b-b-ul2/model.safetensors" ]; then
    echo "audio-pack: pesi mancanti o incompleti in $d -- vedi scripts/audio-benchmark.sh per come scaricarli (repo HF gated, DEC-113)." >&2
    exit 1
  fi
done

echo "== audio-pack: genero il pacchetto demo (DEC-172) -> assets/audio/, wav e log in $OUT =="

RUN_LOG="$OUT/run.log"
"$PY" scripts/audio-pack.py --log-dir "$OUT" "$@" 2>&1 | tee "$RUN_LOG"
status=${PIPESTATUS[0]}

if [ "$status" -ne 0 ]; then
  echo "audio-pack: scripts/audio-pack.py e' uscito con codice $status -- vedi $RUN_LOG" >&2
  exit "$status"
fi

echo "== audio-pack: fatto, manifest in assets/audio/manifest.json, log in $RUN_LOG =="
