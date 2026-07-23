#!/usr/bin/env bash
# Scarica i modelli con verifica SHA256: testo (Qwen, GGUF) e sprite
# (Stable Diffusion, per melting-sprites).
# Uso: scripts/download-models.sh [--light] [--no-sprites]
#   --light:       solo il modello di testo 1.5B (salta il 7B)
#   --no-sprites:  salta i modelli Stable Diffusion (2,2 GB, licenza diversa
#                  dai modelli di testo: vedi la nota licenze qui sotto)
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p models

LIGHT=0
NO_SPRITES=0
for arg in "$@"; do
  case "$arg" in
    --light) LIGHT=1 ;;
    --no-sprites) NO_SPRITES=1 ;;
    *) echo "download-models.sh: opzione sconosciuta: $arg" >&2; exit 2 ;;
  esac
done

MODEL_7B="qwen2.5-coder-7b-instruct-q4_k_m.gguf"
URL_7B="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/$MODEL_7B"
SHA_7B="509287f78cb4d4cf6b3843734733b914b2c158e43e22a7f4bf5e963800894d3c"

MODEL_15B="qwen2.5-coder-1.5b-instruct-q4_k_m.gguf"
URL_15B="https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/$MODEL_15B"
SHA_15B="cc324af070c2ecbfd324a30884d2f951a7ff756aba85cb811a6ec436933bb046"

# Modello SD1.5 per pixel art (spike misurato in docs/ai-production/experiments/sprites-spike.md), trigger "pixelsprite".
MODEL_SD="Public-Prompts-Pixel-Model.ckpt"
URL_SD="https://huggingface.co/PublicPrompts/All-In-One-Pixel-Model/resolve/main/Public-Prompts-Pixel-Model.ckpt"
SHA_SD="d7fb6396ab39b73019f37040977e15ec76a2548372daa534f9ac44b61c3d9548"

# LoRA LCM (generazione in pochi passi). Il file su HuggingFace si chiama
# pytorch_lora_weights.safetensors: va salvato con questo nome, perche'
# stable-diffusion.cpp risolve le LoRA per nome file da --lora-model-dir.
MODEL_LCM="lcm-lora-sdv1-5.safetensors"
URL_LCM="https://huggingface.co/latent-consistency/lcm-lora-sdv1-5/resolve/main/pytorch_lora_weights.safetensors"
SHA_LCM="8f90d840e075ff588a58e22c6586e2ae9a6f7922996ee6649a7f01072333afe4"

# TAESD: VAE approssimato e veloce per il decoding di stable-diffusion.cpp.
MODEL_TAESD="taesd.safetensors"
URL_TAESD="https://huggingface.co/madebyollin/taesd/resolve/main/diffusion_pytorch_model.safetensors"
SHA_TAESD="db169d69145ec4ff064e49d99c95fa05d3eb04ee453de35824a6d0f325513549"

fetch() {
  local name="$1" url="$2" sha="$3"
  if [ -f "models/$name" ] && echo "$sha  models/$name" | sha256sum -c --status; then
    echo "$name: gia' presente e verificato"
    return
  fi
  echo "Scarico $name (riprendibile con Ctrl+C e rilancio)..."
  curl -L -C - --fail -o "models/$name" "$url"
  echo "$sha  models/$name" | sha256sum -c
}

fetch "$MODEL_15B" "$URL_15B" "$SHA_15B"
if [ "$LIGHT" -eq 0 ]; then
  fetch "$MODEL_7B" "$URL_7B" "$SHA_7B"
fi

if [ "$NO_SPRITES" -eq 0 ]; then
  fetch "$MODEL_SD" "$URL_SD" "$SHA_SD"
  fetch "$MODEL_LCM" "$URL_LCM" "$SHA_LCM"
  fetch "$MODEL_TAESD" "$URL_TAESD" "$SHA_TAESD"
fi

cat > models/README.md <<'EOF'
# Modelli locali (mai committare)

Testo (melting-gen):
- qwen2.5-coder-7b-instruct-q4_k_m.gguf  — Qwen/Qwen2.5-Coder-7B-Instruct-GGUF  (Apache 2.0)
- qwen2.5-coder-1.5b-instruct-q4_k_m.gguf — Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF (Apache 2.0)

Sprite (melting-sprites, fase 2 — scaricati a meno di --no-sprites):
- Public-Prompts-Pixel-Model.ckpt — PublicPrompts/All-In-One-Pixel-Model (CreativeML OpenRAIL-M)
  Le immagini generate sono tue e vendibili; se un giorno ridistribuisci i
  PESI col gioco devi propagare le restrizioni d'uso della licenza. In quel
  caso l'alternativa Apache 2.0 e' SD_PixelArt_SpriteSheet_Generator
  (leggermente peggiore in resa), non scaricata da questo script.
- lcm-lora-sdv1-5.safetensors — latent-consistency/lcm-lora-sdv1-5 (openrail++, vedi docs/ai-production/licenze.md; salvato con questo nome perche' stable-diffusion.cpp risolve le LoRA per nome file)
- taesd.safetensors — madebyollin/taesd (MIT)

Scaricati e verificati da scripts/download-models.sh.
EOF
echo "Modelli pronti."
