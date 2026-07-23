---
id: eng-adr-003
title: "ADR-003: script degli oggetti generati in Lua 5.5 sandboxato, niente DSL tipizzata"
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Gli script dei contenuti generati sono Lua 5.5 sandboxato (allowlist _ENV,
  tetto memoria, budget istruzioni, solo caricamento testuale), non una DSL
  tipizzata dedicata; la mini-VM CSV resta come fallback e melting-gen fa un
  dry-run nella stessa sandbox prima che il gioco veda lo script.
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [lua, sandbox, script, dsl, adr]
related: [eng-adr-002, eng-spec-lua-sandbox]
supersedes: []
source_files: [docs/engineering/specs/2026-07-13-lua-sandbox-design.md, src/script/script_sandbox.c, src/script/script_sandbox.h, tools/melting-gen/gen_lua.c, scripts/test-script.sh]
---

# ADR-003: script degli oggetti generati in Lua 5.5 sandboxato, niente DSL tipizzata

## Contesto

L'obiettivo dichiarato del progetto è che l'LLM generi comportamento vero per
oggetti, personaggi e (in fasi successive) nemici/boss, non solo nomi e numeri
da incastrare in mattoncini fissi. Fino alla fase 3, il gioco eseguiva
esclusivamente una **mini-VM CSV a quattro operazioni** (`burst`, `projectile`,
`area`, `heal`, due parametri numerici ciascuna): l'LLM poteva inventare il
*nome* di un oggetto ma non *cosa fa davvero*
(`docs/engineering/specs/2026-07-13-lua-sandbox-design.md`, §1).

Decisione presa il 16/07: invece di progettare una DSL tipizzata dedicata per
esprimere comportamenti generati, si è scelto **Lua 5.5 sandboxato** come
linguaggio, con il codice scritto da un modello 7B locale trattato come
**input non fidato** (§2 della spec).

## Decisione

- **Il linguaggio è Lua 5.5.0** (MIT, vendorizzato, `deps/lua-5.5.0/` — versione
  pinnata anche in ADR-001), non LuaJIT (ha FFI, una fuga in un colpo solo, e
  niente release stabili) e non Luau (C++, trascinerebbe un toolchain C++ dentro
  un gioco C99). Motivo decisivo: `lua_newstate` in 5.5 accetta un **seed** per
  l'hashing delle stringhe, condizione necessaria per il determinismo
  run-riproducibile richiesto dal gioco (§3 della spec).
- **NIENTE DSL tipizzata dedicata.** L'alternativa scartata era progettare un
  linguaggio minimale su misura per i contenuti generati; si è preferito un
  linguaggio general-purpose maturo, sandboxato con tre barriere invece di
  inventare e mantenere un parser/type-checker proprio.
- **Le tre barriere di sicurezza** (`docs/engineering/specs/2026-07-13-lua-sandbox-design.md`,
  §4, implementate in `src/script/script_sandbox.c`):
  1. **Allowlist `_ENV`**: lo stato Lua non usa mai `luaL_openlibs`; l'`_ENV` è
     costruito a mano con solo `math` (senza `random`, sostituito dalla RNG del
     gioco), `table` limitata, poche funzioni base (`ipairs`, `pairs`, `type`,
     `tonumber`, `tostring`, `select`, `error`, `assert`) più l'API di gioco a
     handle.
  2. **Tetto di memoria**: allocatore custom passato a `lua_newstate`
     (`SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP`, 1 MB indicativo in
     `src/script/script_sandbox.h`), con GC d'emergenza e fallimento pulito
     oltre soglia.
  3. **Budget di istruzioni**: hook `LUA_MASKCOUNT`, due livelli distinti in
     `src/script/script_sandbox.h` — `SCRIPT_SANDBOX_LOAD_BUDGET` (10^6,
     generoso per compilare/eseguire il corpo di primo livello) e
     `SCRIPT_SANDBOX_FRAME_BUDGET` (10^4, stretto per le callback chiamate 60
     volte al secondo).
- **Solo caricamento testuale, mai bytecode.** `ScriptSandboxLoad` (in
  `src/script/script_sandbox.c`) usa **esclusivamente**
  `luaL_loadbufferx(L, source, strlen(source), sb->name, "t")` — mai
  `luaL_loadbuffer`/`luaL_loadstring`/`luaL_dostring`, che di default accettano
  anche bytecode non verificato (`lundump.c` si fida ciecamente dell'header: un
  chunk binario malformato crasha l'interprete invece di fallire con un errore
  catturabile). `scripts/test-script.sh` applica questa regola come guardia
  automatica: fallisce se una delle tre funzioni vietate compare in `src/`.
- **`ScriptApi` espone handle, mai puntatori**: un identificatore intero
  (indice + contatore di generazione), validato a ogni chiamata C — mai un
  `lightuserdata` (puntatore grezzo, use-after-free possibile se lo script lo
  conserva dopo che l'entità è morta).
- **La mini-VM CSV resta come rete di sicurezza/fallback** (§9 della spec;
  formalizzato in DEC-037 del decision-log di design, 2026-07-18): se uno
  script non compila, sfora memoria o istruzioni, solleva un errore a runtime o
  chiama un handle non valido, viene **disabilitato in modo permanente per
  quella run**, l'entità torna al proprio comportamento di riserva (l'oggetto
  usa la mini-VM CSV a quattro operazioni, `src/gameplay/script_vm.c`) e
  l'evento finisce nel log. Il patto: nessuno script rotto produce un crash, al
  massimo un oggetto un po' scialbo. DEC-037 estende lo stesso schema (Lua
  generato/validato con le manopole parametriche come garanzia di
  bilanciamento) anche al trait del personaggio e ai tipi di colpo.
- **Dry-run nella stessa sandbox dentro `melting-gen` prima che il gioco veda lo
  script**: `tools/melting-gen/gen_lua.c` compila e collega **lo stesso file**
  `src/script/script_sandbox.c` del gioco (stessa allowlist, stesso tetto
  memoria, stesso budget istruzioni — garantito dal `Makefile`, target
  `GEN_EXTRA_SRC`), ma con un'API di gioco **finta** (`GenLuaStubRegister`,
  nessun `Game*` reale: le funzioni hanno stessi nomi/arità/politica di
  validazione handle di `src/script/script_api.c`, ma rispondono con dati
  plausibili fissi invece di leggere/scrivere un mondo vero). `GenLuaValidateLoad`
  compila lo script (`ScriptSandboxLoad`) e rileva quali callback definisce
  (`on_evaluate`, `on_fire`, `on_hit`, `on_tick`); il chiamante applica poi il
  proprio gate di dominio e il dry-run vero e proprio prima di scrivere lo
  script su disco. Su fallimento persistente (fino a 2 ritenti con l'errore
  rimandato al modello, per gli oggetti) l'oggetto resta sulla mini-VM; per il
  trait del personaggio (M6b-2, DEC-037) non c'è ripiego: la proposta di
  personaggio non si scrive affatto.

## Conseguenze

- Nessuna DSL proprietaria da progettare, documentare e far evolvere: il
  costo di manutenzione ricade su una sandbox attorno a un linguaggio esistente
  e ben documentato, non su un linguaggio nuovo.
- Ogni nuovo tipo di contenuto scriptabile (nemici/boss, fase 3b; stanze, fase
  3c) riusa le stesse tre barriere e lo stesso schema fallback, non ne inventa
  di proprie.
- Il determinismo ha due buchi noti e documentati (non patchati nel motore
  Lua, mitigati lato prompt): `tostring` su tabelle/funzioni espone
  l'indirizzo di memoria (ASLR-dipendente) e `pairs()` su chiavi di tipo
  riferimento (tabelle/funzioni come chiave) itera in un ordine anch'esso
  derivato dall'indirizzo. Il cheat-sheet dato al modello
  (`tools/melting-gen/prompts/lua_system.txt`) istruisce a non usarli; una
  correzione nel motore stesso resta da rivalutare se servisse davvero.
- `melting-gen` è l'unica eccezione al principio "il gioco è l'unico a linkare
  Lua" (ADR-002): linka Lua anch'esso, ma solo per il dry-run usa-e-getta prima
  di scrivere su disco, mai per eseguire script a runtime.

## Fonti

- `docs/engineering/specs/2026-07-13-lua-sandbox-design.md` (spec integrale:
  §1 problema, §2 non fidato, §3 scelta Lua/determinismo, §4 tre barriere, §5
  API a handle, §9 patto di sicurezza/fallback, §10 decomposizione 3a/3b/3c).
- `src/script/script_sandbox.c`, `src/script/script_sandbox.h` (implementazione
  delle tre barriere, budget, caricamento solo testuale).
- `tools/melting-gen/gen_lua.c` (dry-run con API finta, ritenti, gate di
  dominio per oggetti vs trait del personaggio).
- `scripts/test-script.sh` (guardia automatica contro
  `luaL_loadbuffer`/`luaL_loadstring`/`luaL_dostring`, nove fughe eseguite per
  davvero, test di determinismo a due processi).
- `docs/design/governance/decision-log.md`, DEC-037 (estensione dei
  comportamenti Lua a trait personaggio e tipi di colpo, ruolo di garanzia
  delle manopole parametriche).
- `AGENTS.md` (prefissi `ScriptVm`/`ScriptSandbox`/`ScriptApi`/`ScriptItems`,
  eccezione esplicita di link Lua per `melting-gen`).
