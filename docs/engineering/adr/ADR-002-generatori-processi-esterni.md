---
id: eng-adr-002
title: "ADR-002: i generatori girano come processi esterni, mai linkati nel gioco"
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Il binario del gioco non linka mai llama.cpp/stable-diffusion.cpp/cJSON;
  melting-gen e melting-sprites sono due eseguibili separati (ggml incompatibili,
  VRAM condivisa) e il gioco legge solo file validati in generated/.
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [architettura, processi, generated, vram, adr]
related: [eng-adr-001]
supersedes: []
source_files: [Makefile, scripts/test-script.sh, AGENTS.md, src/app/app.c]
---

# ADR-002: i generatori girano come processi esterni, mai linkati nel gioco

## Contesto

Il gioco genera contenuti di run (temi, boss, oggetti, script Lua, sprite) con
due modelli locali: un LLM testuale via llama.cpp (`melting-gen`) e Stable
Diffusion via stable-diffusion.cpp (`melting-sprites`). Sulla macchina di
riferimento (RX 5600 XT, 6 GB VRAM — vedi ADR-001) i due modelli **non entrano
insieme in VRAM**: il 7B testuale quantizzato è già al limite da solo (§2 della
spec `docs/engineering/specs/2026-07-13-local-llm-linux-design.md`). Serviva
un'architettura che permettesse ai due modelli di alternarsi senza tenere mai
entrambi i runtime caricati nello stesso processo.

## Decisione

- **I generatori sono processi esterni, mai codice linkato nel binario del
  gioco.** Il gioco (`bin/melting_run_gpu`) lancia `melting-gen`/
  `melting-sprites` come processi figli (vedi `src/app/app.c`, es.
  `AppRun`/`GenRunner`), aspetta che scrivano file in `generated/` e poi legge
  **solo quei file**, mai lo stato interno del generatore.
- **melting-gen e melting-sprites sono due eseguibili separati**, non un solo
  binario con entrambi i backend, per due motivi verificati nel `Makefile`:
  1. **ggml incompatibili**: llama.cpp e stable-diffusion.cpp vendorizzano due
     fork diversi di ggml (stable-diffusion.cpp usa il fork `leejet/ggml`,
     tag `master-775-b5d8120` — vedi ADR-001); linkarli insieme nello stesso
     binario non è un'opzione pulita.
  2. **VRAM da 6 GB**: i due modelli non coesistono in VRAM; due processi
     separati permettono di farli alternare, ognuno libera tutta la VRAM
     quando esce (nessuno stato residente fra una fase e l'altra).
- **Il binario del gioco non linka MAI llama.cpp, stable-diffusion.cpp o
  cJSON.** Verificato in due punti indipendenti:
  - `Makefile`: `GAME_LIBS` contiene solo `$(RAYLIB_LIB) $(LUA_LIB) -lGL -lm
    -lpthread -ldl -lrt -lX11` — nessuna libreria llama/ggml/sd/cJSON. Quelle
    librerie compaiono solo in `GEN_LIBS` (per `bin/melting-gen`) e
    `SPRITES_LIBS` (per `bin/melting-sprites`), target separati.
  - `scripts/test-script.sh` lo verifica **a runtime sul binario compilato**,
    non solo leggendo i sorgenti: esegue `nm bin/melting_run_gpu` e controlla
    che i simboli `lua_newstate`/`luaL_loadbufferx` **siano presenti** (il
    gioco linka Lua per la sandbox degli script, vedi ADR-003) ma che
    `llama_model_load`, `llama_decode`, `new_sd_ctx` e `cJSON_Parse` **siano
    assenti**. Se in futuro un cambiamento accidentale nel gioco iniziasse a
    trascinare dentro una di quelle librerie, questo test fallisce.
- **Il gioco legge solo file già validati in `generated/`.** La generazione
  (grammatica GBNF, validazione, dry-run Lua in sandbox — vedi ADR-003, la mini-
  VM CSV come fallback) avviene interamente dentro `melting-gen`/
  `melting-sprites` **prima** di scrivere il file finale; il motore C non fa
  mai parsing/validazione "in linea" dell'output grezzo di un modello, tratta
  `generated/` come l'unico confine di fiducia (`AGENTS.md`: "Il runtime legge
  soltanto file locali già validati in `generated/`").
- **`melting-gen` è l'unica eccezione controllata**: da fase 3a-L3 linka anche
  Lua (statico, come il gioco) per eseguire un dry-run degli script generati
  nella stessa sandbox del gioco prima di scriverli su disco (`GEN_EXTRA_SRC`
  nel `Makefile` include `src/script/script_sandbox.c`; vedi ADR-003).
  `AGENTS.md` registra questa eccezione esplicitamente ("permesso
  esplicitamente: 'melting-gen può linkare Lua e cJSON'"): resta comunque vero
  che è `melting-gen`, non il gioco, a linkare quelle librerie.

## Conseguenze

- Un bug o un crash dentro un generatore non può mai portarsi dietro il
  processo di gioco: sono processi separati con confine di sistema operativo,
  non solo moduli logici.
- Il gioco non ha requisiti di build/link verso llama.cpp o stable-diffusion.cpp:
  chi compila solo `make game` non deve mai costruire quelle dipendenze pesanti
  (Vulkan escluso, che serve comunque per la finestra raylib).
- Qualunque nuovo tipo di contenuto generato deve passare da un file scritto in
  `generated/` e validato dal generatore, mai da una chiamata diretta a una
  libreria di inferenza dentro il codice di gioco.
- `scripts/test-script.sh` è la rete di sicurezza automatica di questa
  decisione: va eseguito (`make test-script`, richiamato da `make test`) prima
  di ogni commit che tocca `src/` o il `Makefile`, così una violazione
  accidentale del confine non arriva su `main`.

## Fonti

- `Makefile` (`GAME_LIBS` vs `GEN_LIBS`/`SPRITES_LIBS`, `GEN_EXTRA_SRC`, commenti
  sulla doppia vendorizzazione di ggml e sui 6 GB di VRAM).
- `scripts/test-script.sh` (verifica `nm` sul binario del gioco, righe 19-24).
- `AGENTS.md` (regola "Mantieni il motore C indipendente da rete, chiavi API e
  modelli AI" e l'eccezione esplicita per `melting-gen`/Lua).
- `src/app/app.c` (lancio dei generatori come processi figli, lettura di
  `generated/current_run.json`/`generated/theme_proposals.json`/
  `generated/character_proposal.json`).
- `docs/engineering/specs/2026-07-13-local-llm-linux-design.md`, §2 (vincolo
  VRAM 6 GB che ha motivato l'architettura a processo separato).
