# Istruzioni di progetto

## Piattaforme

- **Linux (principale):** build con `make`; dipendenze native in `deps/` via
  `scripts/setup-deps.sh` (raylib 6.0 e llama.cpp b9979, tag fissati); modelli
  GGUF in `models/` via `scripts/download-models.sh`.
- **Windows (conservata):** gli script `.bat` storici con MinGW restano e non
  vanno rimossi. Il codice specifico Linux vive dietro guard di piattaforma
  (`src/gen/gen_runner.c`).

## Struttura obbligatoria

- Mantieni `src/main.c` come punto di ingresso minimo: deve chiamare soltanto `AppRun`.
- Inserisci costanti, enum e strutture dati condivise in `src/core/game_types.h`.
- Ogni nuova responsabilità deve avere una cartella dedicata sotto `src` e, quando serve un'API, una coppia `.h`/`.c`.
- Non aggiungere nuove funzioni di gameplay a `main.c`.
- Usa `src/game/game_internal.h` soltanto per collaborazioni interne tra moduli. Le API pubbliche restano nei rispettivi header.
- Evita simboli globali generici: usa i prefissi del modulo (`Game`, `World`, `Combat`, `Entities`, `ScriptVm`, `Renderer`, `Ui`, `GenRunner`; `Gen` dentro `tools/melting-gen`).
- Mantieni il motore C indipendente da rete, chiavi API e modelli AI. Il runtime legge soltanto file locali già validati in `generated/`. Solo `bin/melting-gen` linka llama.cpp e cJSON; il binario del gioco no.

## Responsabilità dei moduli

- `src/app`: ciclo applicativo, finestra, modalità menu/gioco/pausa/generazione.
- `src/assets`: caricamento e rilascio delle risorse Raylib.
- `src/content`: manifest e contenuti della run.
- `src/core`: tipi, costanti e funzioni matematiche condivise.
- `src/game`: orchestrazione dello stato di gioco.
- `src/gameplay`: entità, combattimento, oggetti e mini-VM.
- `src/gen`: ciclo di vita del processo melting-gen (avvio, progresso, timeout, annullamento). Nessuna logica di gioco.
- `src/render`: rendering del gioco e dell'interfaccia.
- `src/tests`: test interni eseguibili da riga di comando.
- `src/world`: stanze, mappe, transizioni e ricompense.
- `tools/melting-gen`: generatore locale (llama.cpp Vulkan + grammatica GBNF + validatore + fallback deterministico). Scrive gli stessi file del sidecar Node.

## Verifiche obbligatorie

Dopo modifiche al codice C, su Linux:

```bash
make test        # script/portal/smoke/screenshot/gen-runner
make test-gen    # determinismo fallback, coerenza grammatica, corpus JSON rotti
```

Chi modifica `tools/melting-gen/run.gbnf`, i prompt o `gen_validate.c` riesegue
`make test-gen`, più `make test-llm` se un modello è scaricato. Su Windows
valgono le verifiche `.bat` storiche del README.

## Documentazione e sicurezza

- Conserva appunti, decisioni e guide in `docs/`; spec e piani in `docs/superpowers/`.
- Mantieni `README.md` alla radice per la pagina iniziale GitHub.
- Non versionare `.env`, chiavi API, modelli, `deps/`, binari, log o contenuti generati.
- Non introdurre Raygui, Lua o nuovi backend AI senza una funzione concreta, un
  confine di modulo e test pertinenti. La roadmap delle fasi è in
  `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`.
