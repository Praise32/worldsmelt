#!/usr/bin/env bash
# Scarica i modelli CANDIDATI per la suite di comparazione (sessione di
# decisione 22/07/2026, piano docs/plans/active/model-comparison.md).
# Separato da scripts/download-models.sh apposta: quello scarica SOLO i
# modelli di produzione (il 7B/1.5B Coder che il gioco usa di default);
# questo scarica il resto della lista curata (testo + immagine), usata solo
# da scripts/model-comparison.sh / scripts/image-comparison.sh e mai
# referenziata da tools/melting-gen o tools/melting-sprites.
#
# Uso: scripts/download-comparison-models.sh [--skip-extra-quants]
#   --skip-extra-quants: salta le quantizzazioni extra di TESTO (7B Q5/Q6,
#                         1.5B/3B Q8_0) e scarica solo i 4 modelli testo
#                         Q4_K_M nuovi + i 2 candidati immagine
#                         (~18.1 GB invece di ~35.2 GB -- i candidati
#                         immagine, ~8.5 GB, non sono mai saltati: sono solo
#                         2 e non hanno quantizzazioni extra da saltare).
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p models

SKIP_EXTRA=0
for arg in "$@"; do
  case "$arg" in
    --skip-extra-quants) SKIP_EXTRA=1 ;;
    *) echo "download-comparison-models.sh: opzione sconosciuta: $arg" >&2; exit 2 ;;
  esac
done

# -- 4 candidati nuovi (una quantizzazione Q4_K_M ciascuno) -----------------
# URL e sha256 verificati con `curl -sIL` + HuggingFace API (?blobs=true) il
# 23/07/2026 contro il tag llama.cpp pinnato b9979 (deps/llama.cpp, supporta
# gia' le architetture qwen3/gemma3/phi3 -- vedi src/llama-arch.cpp).
NAME_CODER3B="qwen2.5-coder-3b-instruct-q4_k_m.gguf"
URL_CODER3B="https://huggingface.co/Qwen/Qwen2.5-Coder-3B-Instruct-GGUF/resolve/main/qwen2.5-coder-3b-instruct-q4_k_m.gguf"
SHA_CODER3B="724fb256bec1ff062b2f65e4569e871ad2e95ab2a3989723d1769c54294730b7"

# ATTENZIONE licenza: a differenza del 7B/1.5B Coder (Apache 2.0), la scheda
# di Qwen2.5-Coder-3B-Instruct dichiara "license: other / qwen-research" (uso
# di ricerca, non commerciale) -- vedi
# https://huggingface.co/Qwen/Qwen2.5-Coder-3B-Instruct/blob/main/LICENSE.
# Va bene per QUESTA valutazione tecnica; se il 3B dovesse mai finire nel
# gioco spedito, la licenza va rivista con una decisione dedicata (non
# implicita in questo script).

# Repo Qwen ufficiale "Qwen/Qwen3-4B-Instruct-2507-GGUF" indicato nel task
# NON esiste (verificato: HTTP 401 sull'intero repo, non solo sul file) --
# uso il quant bartowski (Apache 2.0, stessa licenza del modello base),
# rinominato per coerenza con lo schema minuscolo-con-trattini degli altri
# file di models/.
NAME_QWEN3_4B="qwen3-4b-instruct-2507-q4_k_m.gguf"
URL_QWEN3_4B="https://huggingface.co/bartowski/Qwen_Qwen3-4B-Instruct-2507-GGUF/resolve/main/Qwen_Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
SHA_QWEN3_4B="2fde00ce69dd4899c70d020845e2638353015bba0fdf161b3eb965f2bca4464e"

# Repo bartowski indicato nel task ("Phi-4-mini-instruct-GGUF") non esiste
# con quel nome esatto: il repo vero e' "microsoft_Phi-4-mini-instruct-GGUF"
# (bartowski prefissa sempre con l'org del modello base). Licenza MIT.
NAME_PHI4_MINI="phi-4-mini-instruct-q4_k_m.gguf"
URL_PHI4_MINI="https://huggingface.co/bartowski/microsoft_Phi-4-mini-instruct-GGUF/resolve/main/microsoft_Phi-4-mini-instruct-Q4_K_M.gguf"
SHA_PHI4_MINI="01999f17c39cc3074afae5e9c539bc82d45f2dd7faa3917c66cbef76fce8c0c2"

# ggml-org (repo GGUF ufficiale del team llama.cpp/Google per gemma-3).
# Licenza Gemma (permissiva ma con restrizioni d'uso proprie, non OSI/Apache).
NAME_GEMMA3_4B="gemma-3-4b-it-q4_k_m.gguf"
URL_GEMMA3_4B="https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q4_K_M.gguf"
SHA_GEMMA3_4B="882e8d2db44dc554fb0ea5077cb7e4bc49e7342a1f0da57901c0802ea21a0863"

# -- quantizzazioni extra sui modelli gia' scaricati (stessa licenza Apache
#    2.0 dei rispettivi Q4_K_M gia' in models/, stesso repo) ---------------
NAME_CODER7B_Q5="qwen2.5-coder-7b-instruct-q5_k_m.gguf"
URL_CODER7B_Q5="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/qwen2.5-coder-7b-instruct-q5_k_m.gguf"
SHA_CODER7B_Q5="586844eac4d6d6321689f0192c8aa8e69cd8625974a5cc2d925b1a03366e4d16"

NAME_CODER7B_Q6="qwen2.5-coder-7b-instruct-q6_k.gguf"
URL_CODER7B_Q6="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/qwen2.5-coder-7b-instruct-q6_k.gguf"
SHA_CODER7B_Q6="46291ddea1bfb608fe63d9a1907eea6918bda87a7626593edc4bf97c5fd73f9d"

NAME_CODER15B_Q8="qwen2.5-coder-1.5b-instruct-q8_0.gguf"
URL_CODER15B_Q8="https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF/resolve/main/qwen2.5-coder-1.5b-instruct-q8_0.gguf"
SHA_CODER15B_Q8="507de59046601282ba768a9789900e6ccf60ed93ddf346730b7c68eb0715bc47"

NAME_CODER3B_Q8="qwen2.5-coder-3b-instruct-q8_0.gguf"
URL_CODER3B_Q8="https://huggingface.co/Qwen/Qwen2.5-Coder-3B-Instruct-GGUF/resolve/main/qwen2.5-coder-3b-instruct-q8_0.gguf"
SHA_CODER3B_Q8="f648c25dfd5a0870c4ad76724a745124ab5667ff97b664534fcbe46089b75ab8"

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

fetch "$NAME_CODER3B"   "$URL_CODER3B"   "$SHA_CODER3B"
fetch "$NAME_QWEN3_4B"  "$URL_QWEN3_4B"  "$SHA_QWEN3_4B"
fetch "$NAME_PHI4_MINI" "$URL_PHI4_MINI" "$SHA_PHI4_MINI"
fetch "$NAME_GEMMA3_4B" "$URL_GEMMA3_4B" "$SHA_GEMMA3_4B"

# -- candidati IMMAGINE (estensione della missione, sessione 22/07/2026) ----
# SD1.5 vanilla: il repo storico runwayml/stable-diffusion-v1-5 e' stato
# ritirato da HuggingFace -- questo e' il mirror ufficiale di continuazione
# (org "stable-diffusion-v1-5", non gated, stesso file esatto). Licenza
# CreativeML OpenRAIL-M (come il checkpoint pixel-art gia' in models/).
NAME_SD15_VANILLA="sd15-vanilla-pruned-emaonly.safetensors"
URL_SD15_VANILLA="https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors"
SHA_SD15_VANILLA="6ce0161689b3853acaa03779ec93eafe75a02f4ced659bee03f50797806fa2fa"

# Secondo fine-tune pixel-art SD1.5, GIA' citato come alternativa Apache 2.0
# nel commento di scripts/download-models.sh (models/README.md, sezione
# sprite): "SD_PixelArt_SpriteSheet_Generator (leggermente peggiore in resa)".
NAME_PIXELART_ALT="pixelart-spritesheet-generator-v1.ckpt"
URL_PIXELART_ALT="https://huggingface.co/Onodofthenorth/SD_PixelArt_SpriteSheet_Generator/resolve/main/PixelartSpritesheet_V.1.ckpt"
SHA_PIXELART_ALT="85524e631aa5572ae58e305773993620a6064a998383d94fbc4a4d1369f94736"

fetch "$NAME_SD15_VANILLA"  "$URL_SD15_VANILLA"  "$SHA_SD15_VANILLA"
fetch "$NAME_PIXELART_ALT"  "$URL_PIXELART_ALT"  "$SHA_PIXELART_ALT"

if [ "$SKIP_EXTRA" -eq 0 ]; then
  fetch "$NAME_CODER7B_Q5"  "$URL_CODER7B_Q5"  "$SHA_CODER7B_Q5"
  fetch "$NAME_CODER7B_Q6"  "$URL_CODER7B_Q6"  "$SHA_CODER7B_Q6"
  fetch "$NAME_CODER15B_Q8" "$URL_CODER15B_Q8" "$SHA_CODER15B_Q8"
  fetch "$NAME_CODER3B_Q8"  "$URL_CODER3B_Q8"  "$SHA_CODER3B_Q8"
fi

echo "Modelli candidati pronti in models/. Esegui 'make model-comparison' per la suite."
