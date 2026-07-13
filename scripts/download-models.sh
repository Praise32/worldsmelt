#!/usr/bin/env bash
# Scarica i modelli GGUF ufficiali Qwen (Apache 2.0) con verifica SHA256.
# Uso: scripts/download-models.sh [--light]   (--light: solo il modello 1.5B)
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p models

MODEL_7B="qwen2.5-coder-7b-instruct-q4_k_m.gguf"
URL_7B="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/$MODEL_7B"
SHA_7B="509287f78cb4d4cf6b3843734733b914b2c158e43e22a7f4bf5e963800894d3c"

MODEL_15B="qwen2.5-coder-1.5b-instruct-q4_k_m.gguf"
URL_15B="https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/$MODEL_15B"
SHA_15B="cc324af070c2ecbfd324a30884d2f951a7ff756aba85cb811a6ec436933bb046"

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
if [ "${1:-}" != "--light" ]; then
  fetch "$MODEL_7B" "$URL_7B" "$SHA_7B"
fi

cat > models/README.md <<'EOF'
# Modelli locali (mai committare)

- qwen2.5-coder-7b-instruct-q4_k_m.gguf  — Qwen/Qwen2.5-Coder-7B-Instruct-GGUF  (Apache 2.0)
- qwen2.5-coder-1.5b-instruct-q4_k_m.gguf — Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF (Apache 2.0)

Scaricati e verificati da scripts/download-models.sh.
EOF
echo "Modelli pronti."
