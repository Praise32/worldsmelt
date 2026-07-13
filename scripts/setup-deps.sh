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
  libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev xvfb \
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

SD_TAG="master-775-b5d8120"
if [ ! -f deps/stable-diffusion.cpp/build/libstable-diffusion.a ]; then
  echo "== stable-diffusion.cpp $SD_TAG (statica, backend Vulkan) =="
  # --recursive: sd.cpp vendorizza il proprio ggml come sottomodulo, ed e' un
  # fork (leejet/ggml) diverso da quello di llama.cpp: i due binari restano
  # comunque separati (vedi la spec di fase 2), non si linkano mai insieme.
  [ -d deps/stable-diffusion.cpp ] || git clone --recursive --depth 1 --branch "$SD_TAG" https://github.com/leejet/stable-diffusion.cpp.git deps/stable-diffusion.cpp
  cmake -S deps/stable-diffusion.cpp -B deps/stable-diffusion.cpp/build -DCMAKE_BUILD_TYPE=Release -DSD_VULKAN=ON
  # -j4 e non $(nproc): stesso motivo di llama.cpp qui sopra (GCC 15 va in
  # internal compiler error con 12 job paralleli su questa macchina).
  cmake --build deps/stable-diffusion.cpp/build -j4
fi

LUA_TAG="5.5.0"
# Hash del tarball scaricato una volta da lua.org e fissato qui: lua.org non
# pubblica un file .sha256 affiancato, quindi il controllo di integrita' e'
# contro il valore osservato, non contro una fonte terza firmata (idem per i
# tag git di raylib/llama.cpp/stable-diffusion.cpp qui sopra, che si fidano
# del canale HTTPS + del repository GitHub ufficiale).
LUA_SHA256="57ccc32bbbd005cab75bcc52444052535af691789dba2b9016d5c50640d68b3d"
if [ ! -f deps/lua-$LUA_TAG/src/liblua.a ]; then
  echo "== Lua $LUA_TAG (statica, MIT) =="
  if [ ! -d deps/lua-$LUA_TAG ]; then
    curl -sSL -o deps/lua-$LUA_TAG.tar.gz "https://www.lua.org/ftp/lua-$LUA_TAG.tar.gz"
    echo "$LUA_SHA256  deps/lua-$LUA_TAG.tar.gz" | sha256sum -c -
    tar -xzf deps/lua-$LUA_TAG.tar.gz -C deps
    rm -f deps/lua-$LUA_TAG.tar.gz
  fi
  # Target "linux" del Makefile ufficiale di Lua: a differenza di macOS/BSD
  # non abilita readline (SYSCFLAGS = solo "-DLUA_USE_LINUX"), quindi non
  # serve aggiungere libreadline alla lista di pacchetti apt sopra. Costruisce
  # anche i binari lua/luac standalone oltre a liblua.a: il gioco linka solo
  # la libreria statica, i due binari restano semplicemente inutilizzati.
  make -C deps/lua-$LUA_TAG linux
fi

echo "== Verifica Vulkan =="
vulkaninfo --summary | head -25 || echo "ATTENZIONE: vulkaninfo fallito, controlla i driver"

echo "Dipendenze pronte."
