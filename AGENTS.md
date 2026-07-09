# Istruzioni di progetto

## Struttura obbligatoria

- Mantieni `src/main.c` come punto di ingresso minimo: deve chiamare soltanto `AppRun`.
- Inserisci costanti, enum e strutture dati condivise in `src/core/game_types.h`.
- Ogni nuova responsabilità deve avere una cartella dedicata sotto `src` e, quando serve un'API, una coppia `.h`/`.c`.
- Non aggiungere nuove funzioni di gameplay a `main.c`.
- Usa `src/game/game_internal.h` soltanto per collaborazioni interne tra moduli. Le API pubbliche devono restare nei rispettivi header.
- Evita simboli globali generici: usa i prefissi del modulo, per esempio `Game`, `World`, `Combat`, `Entities`, `ScriptVm`, `Renderer`, `Ui`.
- Mantieni il motore C indipendente da rete, chiavi API e modelli AI. Il runtime legge soltanto file locali già validati.

## Responsabilità dei moduli

- `src/app`: ciclo applicativo, finestra e modalità menu/gioco/pausa.
- `src/assets`: caricamento e rilascio delle risorse Raylib.
- `src/content`: manifest e contenuti della run.
- `src/core`: tipi, costanti e funzioni matematiche condivise.
- `src/game`: orchestrazione dello stato di gioco.
- `src/gameplay`: entità, combattimento, oggetti e mini-VM.
- `src/render`: rendering del gioco e della UI attuale.
- `src/tests`: test interni eseguibili da riga di comando.
- `src/world`: stanze, mappe, transizioni e ricompense.

## Verifiche obbligatorie

Dopo modifiche al codice C:

```bat
build_gpu.bat
bin\melting_run_gpu.exe --script-test
bin\melting_run_gpu.exe --portal-test
bin\melting_run_gpu.exe --smoke-test
bin\melting_run_gpu.exe --screenshot-test
```

Usa prima il grafo `codebase-memory-mcp` per scoprire simboli e dipendenze. Reindicizza la repository dopo cambi strutturali.

## Documentazione e sicurezza

- Conserva appunti, decisioni e guide in `docs/`.
- Mantieni `README.md` alla radice per la pagina iniziale GitHub.
- Non versionare `.env`, chiavi API, modelli, binari, log o contenuti generati.
- Non introdurre Raygui, Lua o nuovi backend AI senza una funzione concreta, un confine di modulo e test pertinenti.
