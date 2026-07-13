#!/usr/bin/env bash
# Prepara le dipendenze native in deps/ a versioni fissate. Idempotente.
set -euo pipefail
cd "$(dirname "$0")/.."

RAYLIB_TAG="6.0"

echo "== Pacchetti di sistema (chiede la password) =="
# In un terminale si usa sudo; senza TTY (es. esecuzione da agente) pkexec apre
# la finestra grafica di autenticazione.
SUDO="sudo"
[ -t 0 ] || SUDO="pkexec"
$SUDO apt-get update
$SUDO apt-get install -y build-essential cmake git \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
  libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev \
  libvulkan-dev glslc spirv-headers vulkan-tools

mkdir -p deps

if [ ! -f deps/raylib/build/raylib/libraylib.a ]; then
  echo "== raylib $RAYLIB_TAG (statica, X11+Wayland) =="
  [ -d deps/raylib ] || git clone --depth 1 --branch "$RAYLIB_TAG" https://github.com/raysan5/raylib.git deps/raylib
  cmake -S deps/raylib -B deps/raylib/build -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF -DPLATFORM=Desktop \
    -DGLFW_BUILD_WAYLAND=ON -DGLFW_BUILD_X11=ON
  cmake --build deps/raylib/build -j"$(nproc)"
fi

LLAMA_TAG="b9979"
if [ ! -f deps/llama.cpp/build/src/libllama.a ]; then
  echo "== llama.cpp $LLAMA_TAG (statica, backend Vulkan) =="
  [ -d deps/llama.cpp ] || git clone --depth 1 --branch "$LLAMA_TAG" https://github.com/ggml-org/llama.cpp.git deps/llama.cpp
  cmake -S deps/llama.cpp -B deps/llama.cpp/build -DCMAKE_BUILD_TYPE=Release \
    -DGGML_VULKAN=ON -DBUILD_SHARED_LIBS=OFF -DLLAMA_BUILD_TESTS=ON \
    -DLLAMA_BUILD_EXAMPLES=OFF -DLLAMA_BUILD_TOOLS=OFF -DLLAMA_BUILD_APP=OFF \
    -DLLAMA_BUILD_SERVER=OFF -DLLAMA_CURL=OFF
  # -j4 e non $(nproc): con 12 job paralleli GCC 15 va in internal compiler error
  # su llama-sampler.cpp per pressione di memoria (15 GiB di RAM su questa macchina).
  cmake --build deps/llama.cpp/build -j4
  # test-gbnf-validator non fa parte del target 'all': va chiesto esplicitamente.
  cmake --build deps/llama.cpp/build --target test-gbnf-validator -j4
fi

echo "== Verifica Vulkan =="
vulkaninfo --summary | head -25 || echo "ATTENZIONE: vulkaninfo fallito, controlla i driver"

echo "Dipendenze pronte."
