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

echo "Dipendenze pronte."
