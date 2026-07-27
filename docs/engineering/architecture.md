---
id: eng-architecture
title: Architettura del codice
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Mappa verificata dei moduli C, dei processi esterni (melting-gen,
  melting-sprites) e dei confini di sicurezza (sandbox Lua, mini-VM di
  ripiego) del motore di Worldsmelt.
last_reviewed: 2026-07-27
last_verified_commit: 17204df
topics: [architettura, moduli, sandbox-lua, melting-gen, melting-sprites, build]
related: [meta-doc-code-drift]
supersedes: []
source_files: [src/main.c, src/app/app.c, src/core/game_types.h, src/world/world.c, src/world/room_camera.c, AGENTS.md, Makefile]
---

# Architettura del codice

Questo documento sostituisce `docs/archive/superseded/architecture-2026-07-13.md` (superato: descriveva la mini-VM
come comportamento predefinito e ignorava `src/script/`, vedi
`docs/_meta/DOC-CODE-DRIFT.md` DOC-CODE-DRIFT-001..007). Stato reale: **la sandbox Lua è
il percorso primario per il comportamento degli oggetti e del trait personaggio
(DEC-037); la mini-VM CSV resta la rete di sicurezza** quando la generazione/validazione
Lua fallisce.

`src/main.c` contiene solo il punto di ingresso e chiama `AppRun` (`src/app/app.c`).
`AppMode` (`src/core/game_types.h:177-187`) ha nove stati reali: `APP_MAIN_MENU`,
`APP_RUN_SETUP`, `APP_FLOOR_ZERO`, `APP_GAMEPLAY`, `APP_PAUSE_MENU`, `APP_OPTIONS`,
`APP_BUILD_SCREEN`, `APP_RUN_RESULTS`, `APP_EXIT_CONFIRM` — non esiste più uno stato
`APP_GENERATING` separato: `APP_FLOOR_ZERO` ospita la generazione come overlay bloccante.

## 1. Moduli di `src/`

- **`app`**: ciclo applicativo, finestra, opzioni da riga di comando, i nove stati sopra
  e l'orchestrazione dei due processi esterni (`gen.command`/`gen.spritesCommand`,
  `src/app/app.c:1124-1125`).
- **`assets`**: caricamento e rilascio delle risorse Raylib (atlas, font).
- **`content`**: manifest della run (`run_content.c`), catalogo (`run_catalog.c`),
  roster/proposta di personaggio (`character_roster.c`, `character_proposal.c`). Legge
  solo `generated/current_run.txt` e `generated/current_atlas.{bmp,png}`.
- **`core`**: `game_types.h` (costanti/enum/struct condivise), `game_math` (matematica,
  colori, RNG), più i moduli "biblioteca condivisa fra gioco e generatore" che non
  toccano raylib apposta, per poter essere ricompilati dentro `melting-gen`:
  `shot_type.c` (bilanciamento dei tipi di colpo), `enemy_type.c`, `room_layout.c`,
  `character_type.c` (bande/clamp del personaggio per-run, M6b-1/DEC-014).
- **`game`**: inizializzazione e orchestrazione del frame (`game.c`); `game_internal.h`
  è riservato alle collaborazioni interne fra i sotto-moduli di `game`/`gameplay`, non
  un contenitore generico.
- **`gameplay`**: `combat.c` (giocatore, nemici, colpi, bombe, pickup, uso degli attivi e
  sgancio degli Innesti), `entities.c` (creazione/pulizia entità), `item_traits.c`
  (conversione/descrizione tratti), `item_slots.c` (slot funzionali della tassonomia a 4
  categorie: quale oggetto occupa lo slot attivo/Innesto è **derivato** scorrendo
  `Player.items[]` per categoria, non memorizzato in indici — nessuna tabella parallela da
  tenere allineata alle rimozioni; qui vivono anche cariche, cooldown e i due canali di
  ricarica di DEC-059), `synergies.c` (le sinergie fra oggetti via `SynergySignal` su
  `Item.traits` + tipo di colpo attivo, nessun campo `Item.archetype`), `script_vm.c` (la
  mini-VM CSV a quattro operazioni: **rete di sicurezza**, non il percorso predefinito).
  `combat.c` non include mai `lua.h` né `script_sandbox.h`/`script_api.h`: chiama solo
  `ScriptItems*` (vedi §3) — comprese `ScriptItemsRemoveItem` (l'unica via per togliere un
  oggetto dall'inventario, perché deve distruggerne la sandbox) e `ScriptItemsOnUse`.
- **`gen`**: `gen_runner.{h,c}`, ciclo di vita del processo esterno di generazione
  (avvio, sondaggio del progresso, timeout, annullamento). Nessuna logica di gioco;
  guardia di piattaforma Linux qui dentro (AGENTS.md).
- **`render`**: rendering di scena e interfaccia, inclusa l'integrazione Raygui già
  avvenuta (`render/raygui_impl.c`, `RAYGUI_IMPLEMENTATION`, vendor `deps/raygui`
  4.5.0) — non in un `src/ui` separato, che non esiste ancora nel repo.
- **`script`**: sandbox Lua 5.5, percorso primario del comportamento generato. Vedi §3.
- **`tests`**: test interni da riga di comando, non solo portale/mini-VM:
  `game_tests.c`, `catalog_tests.c`, `script_sandbox_tests.c`,
  `script_items_tests.c`, `script_character_tests.c`.
- **`world`**: `world.c` (stanze multi-cella, forme, porte, transizioni, ricompense),
  `room_camera.{h,c}` (**DEC-170**: le due funzioni *pure* della telecamera — rettangolo di
  clamp e avvicinamento esponenziale — separate da `world.c` proprio perché testabili senza
  finestra, vedi `--rooms-test`), `floor_zero.c` (hub Piano 0). Dopo DEC-170 una stanza è
  una **maschera di celle** (`RoomState.cells`, fino a 4 celle contigue di un blocco 2x2) e
  lo stato mutabile vive nella sola cella di stato: `WorldRoomAt`/`WorldRoomAtMutable` sono
  l'unico accesso corretto, `game->rooms[y][x]` diretto risponde solo per
  `exists`/`doors[]`/`origin`/`cells`.

## 2. Flusso dei processi e confine di rete/AI

Il motore C del gioco è **indipendente da rete, chiavi API e modelli AI**: legge solo
file locali già validati in `generated/` (`generated/current_run.txt`,
`generated/current_atlas.{bmp,png}`, letti da `src/content/run_content.c:369,445-479`).
Il binario del gioco linka **Lua** (statico, `src/script/`) ma **mai** llama.cpp,
stable-diffusion.cpp o cJSON.

```text
src/main.c -> app/AppRun
    -> gen/GenRunner (con --generate: overlay dentro APP_FLOOR_ZERO)
        -> bin/melting-gen   (processo figlio separato, poi esce)
        -> bin/melting-sprites (processo figlio separato, poi esce)
    -> game/GameUpdate -> world / gameplay / content / assets / script
    -> render/RendererDrawApp
```

- **`tools/melting-gen`**: llama.cpp su backend Vulkan, grammatica GBNF, validatore
  (`gen_validate.c`) e fallback deterministico (`gen_fallback.c`). Scrive lo stesso
  manifest testuale del gioco. Da fase 3a-L3 compila anche
  `src/core/game_math.c` + `src/core/{shot_type,enemy_type,room_layout,character_type}.c`
  e `src/script/script_sandbox.c` (**la stessa sandbox del gioco**: stesso allowlist,
  stesso tetto di memoria, stesso budget di istruzioni) per fare il **dry-run** di ogni
  script Lua di oggetto/trait prima di scriverlo su disco (`gen_lua.c`), usando un'API
  di gioco finta (`GenLuaStubRegister`, nessun `Game*` reale, stessi nomi/arità di
  `script_api.c`). Fino a 2 ritenti con l'errore rimandato al modello; su fallimento
  l'oggetto resta sulla mini-VM (`script_vm.c`), mentre per il trait del personaggio
  (M6b-2/DEC-037) un fallimento non produce ripiego: la proposta di personaggio non
  viene scritta.
- **`tools/melting-sprites`**: post-processing + generazione via stable-diffusion.cpp.
  Non linka raylib né llama.cpp (llama.cpp e sd.cpp vendorizzano due ggml
  incompatibili): è un eseguibile separato. Genera 12 celle dal tema/stile del
  manifest; a modello assente/caricamento fallito esce senza scrivere l'atlas né
  toccare il manifest (mai un ripiego silenzioso fuori da `--dry-run`).
- I due strumenti **non girano mai insieme**: oltre al confine di link, è anche una
  necessità di VRAM (scheda di riferimento da 6 GB, i due modelli non ci stanno
  contemporaneamente); il gioco li alterna come processi figli e ognuno libera tutta
  la memoria all'uscita.

## 3. I tre livelli di `src/script` e il confine `ScriptItems*`

Tre prefissi distinti convivono in `src/script`/`src/gameplay` e non vanno confusi:

1. **`ScriptSandbox`** (`script_sandbox.{h,c}`) — il "vascello" Lua blindato: stato Lua
   indipendente per script (mai condiviso fra oggetti/nemici), allocatore col tetto di
   memoria, hook del budget di istruzioni (`SCRIPT_SANDBOX_LOAD_BUDGET`/`_FRAME_BUDGET`),
   `_ENV` costruito da un allowlist esplicito (niente `luaL_openlibs`). Compila testo
   con `luaL_loadbufferx(...,"t")`, mai bytecode (`make test` porta una guardia
   `grep` anti-`luaL_loadbuffer/loadstring/dostring` su tutto `src/`).
2. **`ScriptApi`** (`script_api.{h,c}`) — l'API di gioco a handle (indice+generazione,
   mai un puntatore grezzo) registrata in quell'`_ENV`, con ogni scrittura clampata
   agli stessi confini della mini-VM.
3. **`ScriptItems`** (`script_items.{h,c}`) — le callback degli oggetti
   (`on_evaluate`/`on_fire`/`on_hit`/`on_tick`, più `on_use` per i soli oggetti attivi:
   gira per il singolo oggetto che il giocatore ha usato, non per tutti quelli posseduti)
   e il sistema delle cache "alla Isaac":
   ricalcola sempre da zero da `Player.base*`, mai in place — da questa fase anche la
   maschera `Player.traits`, che prima era un OR accumulato al pickup e quindi non si
   spegneva più quando un oggetto viene rimosso. Dietro la stessa facciata,
   `script_character.{h,c}` (`ScriptCharacter*`, M6b-2/DEC-037) gestisce l'unica
   sandbox del trait del personaggio generato per la run (non un oggetto: niente slot
   d'inventario né layer visivi); `ScriptItems*` la pilota internamente
   (`ScriptItemsInit`/`Shutdown` per il ciclo di vita,
   `ScriptItemsOnFire/OnHit/OnTick`/`RecomputeStats` per le callback). Ordine del
   ricalcolo: `Player.base*` → `on_evaluate` del trait (se attivo) → oggetti → clamp.

**Confine per `src/gameplay`**: `combat.c` (e ogni chiamante fuori da `src/script/`)
chiama solo `ScriptItems*`, mai `lua.h`/`script_sandbox.h`/`script_api.h`/
`script_character.h` direttamente — quel confine appartiene a `src/script/`.

## 4. Regole di estensione (ancora valide)

- Una nuova funzione va nel modulo che possiede quella responsabilità.
- Una responsabilità nuova e consistente prende un **nuovo modulo**: cartella dedicata
  e, se serve un'API, coppia `.h`/`.c`.
- `game_internal.h` resta riservato alle collaborazioni interne fra `game`/`gameplay`:
  le API pubbliche restano nei rispettivi header.
- Una futura estensione dell'integrazione Raygui in un modulo `src/ui` separato dal
  rendering del mondo resta possibile ma non è ancora avvenuta: oggi vive dentro
  `src/render/raygui_impl.c`.
- Non ampliare l'allowlist di `_ENV` (`ScriptSandboxBuildEnv` in `script_sandbox.c`)
  senza una barriera di sicurezza corrispondente (memoria/istruzioni/determinismo) e
  un test in `src/tests/script_sandbox_tests.c` che esegua davvero lo scenario.
- Simboli globali: usare i prefissi di modulo (`Game`, `World`, `Combat`, `Entities`,
  `ScriptVm`, `ScriptSandbox`, `ScriptApi`, `ScriptItems`, `Renderer`, `Ui`,
  `GenRunner`; `Gen` dentro `tools/melting-gen`), mai nomi generici.

## 5. Build e test

Target principali del `Makefile` (`GAME_BIN=bin/melting_run_gpu`,
`GEN_BIN=bin/melting-gen`, `SPRITES_BIN=bin/melting-sprites`):

```bash
make game     # bin/melting_run_gpu (raylib + Lua statica, mai llama.cpp/sd.cpp)
make gen      # bin/melting-gen (llama.cpp Vulkan + Lua, per il dry-run)
make sprites  # bin/melting-sprites (stable-diffusion.cpp)
make all      # i tre insieme

make test         # script/portal/smoke/screenshot/gen-runner + guardia anti-bytecode
make test-gen     # scripts/test-gen.sh: determinismo fallback, grammatica, corpus rotti
make test-script  # scripts/test-script.sh: sandbox Lua, fughe note, determinismo, cache
make test-sprites # scripts/test-sprites.sh
make test-llm     # richiede un modello scaricato (scripts/download-models.sh)

make docs-index / docs-check / docs-audit  # indice/verifica/report di docs/
```

I test aprono una finestra raylib: su Wayland, se la sessione è bloccata o la finestra
non è visibile, il compositor smette di consegnare frame e il gioco resta appeso al
primo `SwapBuffers`. Per questo `make test`/`test-script`/`test-sprites` girano sotto
`xvfb-run` quando disponibile (schermo virtuale 24 bit — a 8 bit OpenGL non parte —
con `XDG_RUNTIME_DIR` dedicato e `WAYLAND_DISPLAY` disattivato, così GLFW non sceglie
comunque il socket Wayland): vedi la variabile `TEST_RUNNER` in cima al `Makefile`.
