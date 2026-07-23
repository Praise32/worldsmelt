---
id: eng-adr-001
title: "ADR-001: backend GPU Vulkan obbligatorio, mai ROCm, mai flash-attention su RDNA1"
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  llama.cpp e stable-diffusion.cpp usano sempre il backend Vulkan (mai ROCm) sulla
  RX 5600 XT di riferimento, con flash-attention disattivata; versioni di
  raylib/llama.cpp/Lua pinnate in scripts/setup-deps.sh.
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [vulkan, rocm, gpu, rdna1, dipendenze, adr]
related: [eng-adr-002]
supersedes: []
source_files: [scripts/setup-deps.sh, Makefile]
---

# ADR-001: backend GPU Vulkan obbligatorio, mai ROCm, mai flash-attention su RDNA1

## Contesto

La macchina di riferimento del progetto è Ubuntu 26.04, Ryzen 5 3600, 15 GiB RAM,
**AMD RX 5600 XT 6 GB (RDNA1, gfx1010), driver Mesa RADV**. Sia `melting-gen` (LLM
testuale via llama.cpp) sia `melting-sprites` (Stable Diffusion via
stable-diffusion.cpp) devono girare su questa GPU entro un budget di attesa
accettato dall'utente di 1-2 minuti a inizio run.

La decisione è stata presa nel ciclo di design del 2026-07-13 (vedi
`docs/engineering/specs/2026-07-13-local-llm-linux-design.md`, §2 "Vincoli
hardware e decisioni verificate") e non era ancora stata promossa a ADR
(rilevato come `DOC-CONFLICT-038` in `docs/_meta/DOC-CONFLICTS.md`): un vincolo
duro, già in produzione, senza un documento canonico che lo registri e impedisca
a un futuro agente di reintrodurre ROCm o la flash-attention inseguendo
prestazioni.

## Decisione

- **Backend GPU: Vulkan, sempre; mai ROCm.** ROCm non ha mai supportato in modo
  affidabile `gfx1010` (RDNA1) e i workaround noti sono instabili. Il backend
  Vulkan di llama.cpp/stable-diffusion.cpp è maturo, gira sul driver Mesa RADV di
  serie (nessun pacchetto proprietario aggiuntivo) ed è pari o più veloce di ROCm
  su RDNA1 (fonte citata dalla spec: scoreboard Vulkan di llama.cpp, discussione
  ggml-org #10879).
- **Mai flash-attention su RDNA1**: la spec la documenta come causa di un crollo
  delle prestazioni sulla stessa architettura. Il progetto non la abilita in
  nessun punto della build; il default del backend Vulkan la gestisce già senza
  bisogno di flag espliciti.
- **Verificato nel codice**: `scripts/setup-deps.sh` compila sia llama.cpp
  (`cmake ... -DGGML_VULKAN=ON`) sia stable-diffusion.cpp (`cmake ...
  -DSD_VULKAN=ON`); nessun flag `ROCm`/`HIP`/flash-attention compare nello
  script. Il `Makefile` linka `-lvulkan` sia per `melting-gen` (`GEN_LIBS`) sia
  per `melting-sprites` (`SPRITES_LIBS`), e in entrambi i target compila
  `ggml-vulkan/libggml-vulkan.a`.
- **Versioni pinnate** (idempotenti, in `scripts/setup-deps.sh`):
  - `raylib` tag `6.0` (statica, X11+Wayland).
  - `llama.cpp` tag `b9979` (statica, backend Vulkan; build con
    `-DBUILD_SHARED_LIBS=OFF -DLLAMA_CURL=OFF`, target di test
    `test-gbnf-validator` incluso).
  - `stable-diffusion.cpp` tag `master-775-b5d8120` (statica, backend Vulkan;
    fork `leejet/ggml`, deliberatamente diverso da quello di llama.cpp — vedi
    ADR-002).
  - `Lua` `5.5.0` (statica, MIT, sorgente scaricata e verificata via
    `sha256sum` prima della build).
- Verifica pratica di fine setup: `scripts/setup-deps.sh` esegue
  `vulkaninfo --summary` e segnala esplicitamente se fallisce, così un driver
  Vulkan mancante o rotto emerge subito invece che al primo avvio del gioco.

## Conseguenze

- Nessun percorso di build ROCm/HIP esiste o va aggiunto: chi lavora su GPU AMD
  diverse (RDNA2/3 con supporto ROCm migliore) non deve reintrodurlo per questo
  progetto senza riaprire la decisione qui sopra.
- Flash-attention resta disattivata di default; se in futuro si valuta hardware
  diverso da RDNA1 (es. sviluppo su GPU cloud per training LoRA, mai per il
  runtime del gioco), la riabilitazione va valutata come nuova decisione, non
  come default.
- Le versioni pinnate cambiano solo con un aggiornamento esplicito di
  `scripts/setup-deps.sh` (tag Git per raylib/llama.cpp/stable-diffusion.cpp,
  `LUA_SHA256` per Lua): un bump di versione non documentato qui è una
  deviazione dal processo, non uno stato nuovo da assumere silenziosamente.
- I numeri di prestazioni indicativi (7B Q4_K_M al limite dei 6 GB di VRAM,
  ~40-50 tok/s attesi a pieno offload) restano nella spec storica citata sotto
  come contesto della decisione; i numeri misurati per davvero sono in
  `docs/BENCHMARKS.md` (generato da `make test-llm`/`make benchmark`).

## Fonti

- `docs/engineering/specs/2026-07-13-local-llm-linux-design.md`, §2 (vincoli
  hardware e motivazione Vulkan/ROCm/flash-attention).
- `scripts/setup-deps.sh` (flag di build `-DGGML_VULKAN=ON`/`-DSD_VULKAN=ON`,
  tag pinnati, verifica `vulkaninfo`).
- `Makefile` (link `-lvulkan` e `ggml-vulkan` in `GEN_LIBS`/`SPRITES_LIBS`).
- `docs/_meta/DOC-CONFLICTS.md`, `DOC-CONFLICT-038` (rilevazione del vincolo non
  ancora promosso ad ADR, origine di questo documento).
