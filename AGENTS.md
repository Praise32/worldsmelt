# Istruzioni di progetto

## Piattaforme

- **Linux (principale):** build con `make`; dipendenze native in `deps/` via
  `scripts/setup-deps.sh` (raylib 6.0, llama.cpp b9979, Lua 5.5.0, tag
  fissati); modelli GGUF in `models/` via `scripts/download-models.sh`.
- **Windows (conservata):** gli script `.bat` storici con MinGW restano e non
  vanno rimossi. Il codice specifico Linux vive dietro guard di piattaforma
  (`src/gen/gen_runner.c`).

## Struttura obbligatoria

- Mantieni `src/main.c` come punto di ingresso minimo: deve chiamare soltanto `AppRun`.
- Inserisci costanti, enum e strutture dati condivise in `src/core/game_types.h`.
- Ogni nuova responsabilità deve avere una cartella dedicata sotto `src` e, quando serve un'API, una coppia `.h`/`.c`.
- Non aggiungere nuove funzioni di gameplay a `main.c`.
- Usa `src/game/game_internal.h` soltanto per collaborazioni interne tra moduli. Le API pubbliche restano nei rispettivi header.
- Evita simboli globali generici: usa i prefissi del modulo (`Game`, `World`, `Combat`, `Entities`, `ScriptVm`, `ScriptSandbox`, `ScriptApi`, `ScriptItems`, `Renderer`, `Ui`, `GenRunner`; `Gen` dentro `tools/melting-gen`). Tre prefissi distinti convivono in `src/script`/`src/gameplay` e NON vanno confusi: `ScriptVm` (la mini-VM CSV a quattro operazioni, `src/gameplay/script_vm.c`, la rete di sicurezza), `ScriptSandbox` (il "vascello" Lua blindato, `src/script/script_sandbox.c`: stato, allocatore, hook di istruzioni, `_ENV`), `ScriptApi` (le funzioni di gioco a handle esposte dentro quell'`_ENV`, `src/script/script_api.c`) e `ScriptItems` (le callback degli oggetti + il sistema delle cache, `src/script/script_items.c`, l'unico punto che `src/gameplay/combat.c` chiama).
- Mantieni il motore C indipendente da rete, chiavi API e modelli AI. Il runtime legge soltanto file locali già validati in `generated/`. Solo `bin/melting-gen` linka llama.cpp e cJSON; `bin/melting-sprites` linka stable-diffusion.cpp. Il binario del gioco linka Lua (statico, `src/script/`) ma nessuno dei tre. Da fase 3a-L3, `bin/melting-gen` linka Lua ANCHE lui (compila anche `src/core/game_math.c` e `src/script/script_sandbox.c`, la stessa sandbox del gioco, per validare gli script Lua degli oggetti PRIMA di scriverli su disco: vedi `tools/melting-gen/gen_lua.c`) — permesso esplicitamente ("melting-gen può linkare Lua e cJSON"), il gioco resta l'unico a non linkare mai llama.cpp/cJSON.

## Responsabilità dei moduli

- `src/app`: ciclo applicativo, finestra, gli stati canonici del design
  (`MainMenu`, `RunSetup`, `FloorZero`, `Gameplay`, `PauseMenu`, `Options`,
  `BuildScreen`, `RunResults`, `ExitConfirm` — vedi
  `docs/design/05-game-states-and-flow.md`).
- `src/assets`: caricamento e rilascio delle risorse Raylib.
- `src/content`: manifest e contenuti della run.
- `src/core`: tipi, costanti e funzioni matematiche condivise.
- `src/game`: orchestrazione dello stato di gioco.
- `src/gameplay`: entità, combattimento, oggetti e mini-VM. Chiama
  `ScriptItems*` (`src/script/script_items.h`) per le callback Lua degli
  oggetti (`on_evaluate`/`on_fire`/`on_hit`/`on_tick`) e per il sistema delle
  cache: non include mai `lua.h` ne' `script_sandbox.h`/`script_api.h`
  direttamente, quel confine è di `src/script/`.
- `src/gen`: ciclo di vita del processo melting-gen (avvio, progresso, timeout, annullamento). Nessuna logica di gioco.
- `src/render`: rendering del gioco e dell'interfaccia.
- `src/script`: sandbox Lua 5.5 per script non fidati (vedi
  `docs/superpowers/specs/2026-07-13-lua-sandbox-design.md`). Tre livelli:
  `script_sandbox.{h,c}` (il "vascello": stato Lua, allocatore col tetto di
  memoria, hook del budget di istruzioni, `_ENV` costruito da un allowlist,
  invariato dalla fase 3a-L1); `script_api.{h,c}` (l'API di gioco a handle —
  indice+generazione, mai un puntatore grezzo — registrata in quell'`_ENV`,
  con ogni scrittura clampata agli stessi confini della mini-VM);
  `script_items.{h,c}` (le quattro callback degli oggetti e il sistema delle
  cache "alla Isaac": ricalcola SEMPRE da zero da `Player.base*`, mai in
  place, cosi' un oggetto generato male non fa accumulare un modificatore e
  rimuoverlo è banale). `src/gameplay/combat.c` chiama solo `ScriptItems*`.
  M6b-2 (DEC-037) aggiunge un quarto file dietro la STESSA facciata:
  `script_character.{h,c}` (`ScriptCharacter*`), UNA sola sandbox per il
  trait UNICO del personaggio GENERATO per la run (non un oggetto: niente
  slot d'inventario, niente layer visivi). `ScriptItems*` la pilota
  internamente (`ScriptItemsInit`/`ScriptItemsShutdown` per il ciclo di
  vita, `ScriptItemsOnFire/OnHit/OnTick`/`ScriptItemsRecomputeStats` per le
  callback) — `combat.c` (e ogni altro chiamante fuori da `src/script/`)
  continua a non vedere mai `ScriptCharacter*` direttamente. Ordine del
  ricalcolo: `Player.base*` → `on_evaluate` del TRAIT (se attivo) → oggetti
  → clamp, sempre da zero come per gli oggetti.
- `src/tests`: test interni eseguibili da riga di comando.
- `src/world`: stanze, mappe, transizioni e ricompense.
- `tools/melting-gen`: generatore locale (llama.cpp Vulkan + grammatica GBNF + validatore + fallback deterministico). Scrive gli stessi file del sidecar Node. `gen_lua.c` (fase 3a-L3) genera e valida lo script Lua opzionale di ogni oggetto: prompt cheat-sheet + few-shot (`prompts/lua_system.txt`/`lua_user.txt`), nessuna grammatica (un Lua completo non si esprime in GBNF), dry-run nella sandbox vera con un'API di gioco finta (`GenLuaStubRegister`, nessun `Game*`), fino a 2 ritenti con l'errore rimandato al modello; su fallimento l'oggetto resta sulla mini-VM. M6b-2 (DEC-037) aggiunge lo stesso ciclo per il trait UNICO del personaggio generato (`GenLuaValidateCharacterTrait`/`GenLuaGenerateCharacterTrait`, template `prompts/lua_character_user.txt`): gate di dominio diverso (esattamente una fra le quattro callback, mai zero), e su fallimento NESSUN ripiego — l'intera proposta di personaggio non si scrive (carta assente, mai un personaggio-curato-di-riserva).

## Verifiche obbligatorie

Dopo modifiche al codice C, su Linux:

```bash
make test          # script/portal/smoke/screenshot/gen-runner (+ guardia anti-bytecode di src/script)
make test-gen      # determinismo fallback, coerenza grammatica, corpus JSON rotti
make test-script   # sandbox Lua: fughe note + determinismo cross-processo, API a handle, callback oggetti, sistema delle cache
```

Chi modifica `tools/melting-gen/run.gbnf`, i prompt o `gen_validate.c` riesegue
`make test-gen`, più `make test-llm` se un modello è scaricato. Chi tocca
`src/script/` riesegue `make test-script`. Su Windows valgono le verifiche
`.bat` storiche del README.

## Documentazione e sicurezza

- Conserva appunti, decisioni e guide in `docs/`; spec e piani in `docs/superpowers/`.
- Mantieni `README.md` alla radice per la pagina iniziale GitHub.
- Non versionare `.env`, chiavi API, modelli, `deps/`, binari, log o contenuti generati.
- Non introdurre Raygui o nuovi backend AI senza una funzione concreta, un
  confine di modulo e test pertinenti. La roadmap delle fasi è in
  `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`.
- La sandbox Lua vive in `src/script/` (design in
  `docs/superpowers/specs/2026-07-13-lua-sandbox-design.md`): non ampliarne
  l'allowlist di `_ENV` (`src/script/script_sandbox.c`,
  `ScriptSandboxBuildEnv`) senza una barriera di sicurezza corrispondente
  (memoria/istruzioni/determinismo) e un test in
  `src/tests/script_sandbox_tests.c` che esegua davvero lo scenario che si
  vuole permettere.
