---
id: eng-known-issues
title: Registro dei difetti e limiti noti
domain: engineering
status: approved
authority: supporting
owner: engineering
summary: >-
  Difetti e limiti tecnici NOTI e verificati nel codice reale, con sintomo,
  evidenza (file:riga) e stato attuale; non e' un elenco di idee o backlog di
  design.
last_reviewed: 2026-07-30
last_verified_commit: 06b9b16
topics: [difetti, limiti, test, rng, generazione, catalogo, audio, DEC-008, Crust, DEC-043, WP3, ostacoli, persistenza, WP-INT, WP6, font, glyphs_ext, personaggi, arena-di-sfida]
related: [eng-dependencies, meta-doc-code-drift, gd-system-run-manifest]
supersedes: []
source_files: [src/tests/game_tests.c, src/content/run_catalog.c, scripts/test-llm.sh, scripts/test-gen.sh, src/game/game.c, src/app/app.c, tools/melting-gen/gen_util.c, tools/melting-sprites/sprite_util.c, tools/melting-gen/gen_lua.h, tools/melting-gen/melting_gen.h, tools/melting-gen/gen_validate.c, tools/melting-gen/gen_fallback.c, src/gameplay/item_pool.c, src/content/run_content.c, docs/archive/legacy-notes/issue-notes.md, src/audio/audio.c, src/tests/audio_tests.c, src/world/floor_zero.c, src/render/game_renderer.c, src/core/game_types.h, src/gameplay/combat.c, src/world/world.c, src/assets/art_atlas.c, src/assets/art_atlas.h, src/render/art_draw.c, src/tests/art_atlas_tests.c, src/content/character_roster.c]
---

# Registro dei difetti e limiti noti

Ogni voce: sintomo osservabile, evidenza nel codice, stato. Formato
INTENDED/DOCUMENTED-AS-IS/OBSERVED-AS-IS dove utile a distinguere design da
realta' tecnica (vedi `docs/_meta/DOCUMENT-STANDARDS.md` §3).

## 1 — `make test` (`--states-test`) puo' risultare rosso se `catalog/` contiene run locali

**Sintomo**: il test del Catalogo assume che, all'apertura, la prima
categoria (indice 0, "mondi") sia vuota: dopo aver navigato avanti e indietro
fra categorie fino a tornare alla categoria 0, invia un "giu'" e verifica che
il focus voce NON si sia spostato — corretto solo se quella categoria non ha
voci. Se il processo di test gira nella working directory del repository e
`catalog/` contiene gia' file di run reali (scritti giocando/testando in
precedenza), la categoria 0 puo' avere voci: il "giu'" sposta il focus e
l'assert fallisce.

**Evidenza**: `src/tests/game_tests.c:349` — `STATES_CHECK(ui.catalogItemFocus == 0, "su/giu' su una
categoria vuota ha spostato il focus voce")` fallisce quando la categoria 0 ("mondi") del catalogo
aggregato ha davvero delle voci; `src/content/run_catalog.c` leggeva/scriveva `catalog/` con percorso
relativo fisso, quindi dipendeva dalla cwd del processo di test e dal contenuto reale di `catalog/`
anziché da un fixture isolato.

**Stato**: RISOLTO (27/07, notte). Implementazione:
- **Isolamento bidimensionale** (letture E scritture) tramite `g_testCatalogPath`: sia le letture
(`RunCatalogAggregate`/`RunCatalogAggregateFromPath`) sia le scritture (`RunCatalogWriteRun`,
`NextProgressive`) usano `g_testCatalogPath` quando settato via `RunCatalogSetTestPath`, altrimenti
il default `"catalog/"`. `RunCatalogGetTestPath` (nuovo getter pubblico) espone lo stesso percorso ai
test che devono scrivere fixture direttamente nella directory isolata.
- **Test `--states-test`** (GameStatesTest): crea una directory temporanea vuota (portabile Linux/
Windows, vedi `CreateTempCatalogTestDir` nello stesso file), settata con `RunCatalogSetTestPath`. Le
LETTURE e le SCRITTURE di catalogo usano il percorso isolato. Il test ripulisce la directory
temporanea alla fine.
- **Test `--catalog-screen-test`** (GameCatalogScreenTest): come GameStatesTest, crea UNA directory
temporanea isolata (stessa `CreateTempCatalogTestDir`, copia privata in `catalog_tests.c`) condivisa
da tutti gli scenari. `CatalogScreenEmptyScenario` (che gira per primo) sfrutta la garanzia "vuota per
costruzione" della directory appena creata, senza piu' dover spostare/ripristinare `catalog/` reale.
`CatalogScreenPopulatedScenario` scrive due run sintetiche e un file corrotto via
`RunCatalogGetTestPath` nello stesso percorso isolato. `GameCatalogScreenTest` ripulisce la directory
temporanea alla fine (nessuno scenario tocca mai `catalog/` reale).
- **Controprova su disco della guardia `catalogWritesEnabled`** (`CatalogFileCount` in
`game_tests.c`): le tre `STATES_CHECK` che confrontano il conteggio file prima/dopo (abbandono da
ExitConfirm, `PHASE_WIN` sintetico, `PHASE_GAME_OVER` sintetico) devono ispezionare la STESSA
directory in cui `RunCatalogWriteRun` scriverebbe davvero in quel momento — che durante
`GameStatesTest` e' la directory temporanea isolata, non la vera `catalog/`. Un primo giro di questo
fix aveva lasciato `CatalogFileCount` hardcoded su `"catalog"`: le tre verifiche risultavano vere per
costruzione (la vera `catalog/` non viene mai toccata dal test isolato) indipendentemente dal fatto
che la guardia funzionasse o si fosse rotta — una regressione silenziosa, individuata in revisione e
corretta qui. `CatalogFileCount` ora legge `RunCatalogGetTestPath()` (fallback `"catalog"`), quindi
segue lo stesso percorso di `RunCatalogWriteRun` e la controprova resta significativa.

File modificati:
- `src/content/run_catalog.c`: RunCatalogWriteRun, NextProgressive, RunCatalogGetTestPath (getter per il percorso isolato usato nei test)
- `src/content/run_catalog.h`: commento aggiornato su RunCatalogSetTestPath (copre ora letture E scritture); ristabilito RunCatalogGetTestPath come getter pubblico
- `src/tests/game_tests.c`: CreateTempCatalogTestDir/RemoveTempCatalogTestDir (helper portabili Linux/Windows) + GameStatesTest li usa con RunCatalogSetTestPath per isolare il test
- `src/tests/catalog_tests.c`: stessa coppia di helper (copia privata) + GameCatalogScreenTest li usa con RunCatalogSetTestPath; CatalogScreenPopulatedScenario usa RunCatalogGetTestPath per scrivere il file corrotto nel percorso isolato; CatalogScreenEmptyScenario non tocca piu' catalog/ reale (no residui in catalog/ reale)

## 2 — `make test-llm` flaky (~25%) col modello 1.5B

**Sintomo**: la guardia anti-fotocopia richiede 5 tipi di colpo (`shotName`)
distinti su 5 piani generati; col modello 1.5B il campo puo' collassare su un
attrattore ricorrente (osservato: nomi tipo "Jolt"/"Jolt Bolt" ripetuti),
facendo fallire il controllo di varieta' anche quando il resto della run e'
valido. La causa a monte e' l'inferenza Vulkan, non deterministica a parita' di
seed sull'hardware di riferimento.

**Evidenza**: `scripts/test-llm.sh` — blocco "varieta': 5 piani diversi, 5
tipi di colpo diversi" (`distinctShots`, `shotCount -ne 5` o
`distinctShots -lt 5` fanno fallire il test); commento nello script sul bug
storico della finestra di repeat-penalty piu' corta di un piano di JSON, che
ha motivato la guardia.

**Stato**: noto, non fixato in modo deterministico. Il modello di riferimento
per l'inferenza reale resta il 1.5B (il 7B e' usato per misure di budget
token, non come target primario di `test-llm` di default).

## 3 — RNG di gameplay ancora seedato con `time(NULL)`

**Sintomo (storico)**: `GameResetRun` inizializzava l'RNG di gioco con l'orologio di
sistema, non con un seed derivato dal seed del run scelto/condiviso. I
contenuti generati (tema, layout, oggetti) sono deterministici dal seed
(`melting-gen` prende `--seed` esplicito), ma il **gameplay** (spawn, drop,
RNG di combattimento durante la run) no.

**Evidenza (storica)**: `src/game/game.c:99-103` — `void GameResetRun(Game *game)` con
`game->rng = (unsigned int)time(NULL) ^ 0x514AACu;`.

**Impatto**: blocca le gare asincrone eque fra giocatori sullo stesso seed
(DEC-016 multiplayer asincrono via classifiche, DEC-062, DEC-066): due run
con lo stesso seed di generazione possono comunque divergere nel gameplay
perche' l'RNG di run non e' derivato da quel seed.

**Stato**: RISOLTO (27/07, notte, DEC-141) il prerequisito TECNICO — la Classificata a
stesso seed resta comunque NON abilitata (vedi sotto). Implementazione:
- **`GameResetRunWithSeed(Game *game, unsigned int runSeed)`** (nuova, `src/game/game.c`)
  affianca `GameResetRun` invece di cambiarne la firma (nessuna API pubblica rotta):
  deriva `game->rng` da `runSeed` con un finalizzatore splitmix64 a costante di dominio
  propria (`GameplayRngSeedFromRunSeed`), e passa `runSeed` **grezzo** a `RunContentLoad`
  (il seed di fallback dei contenuti) — gameplay e generazione non condividono mai lo
  stesso stream pur partendo dallo stesso seed. `GameResetRun` resta la wrapper storica
  (nessun seed di run disponibile: avvio provvisorio di `AppRun`, binari `*Test`), invariata
  per ogni chiamante esistente, ma ora passa anche lei da `GameResetRunWithSeed` con un
  seed-orologio.
- **Il seed vero arriva da `gen->pendingGenSeed`** (il seed che `AppEnterFloorZero` ha
  gia' deciso per la run in `RunSetup`/reroll/`RunResults`, lo stesso passato a
  `melting-gen`): l'attraversamento del varco del Piano 0 (`src/app/app.c`, ramo
  `floorZeroExitCrossed`) chiama `GameResetRunWithSeed(game, gen->pendingGenSeed)` invece
  di `GameResetRun(game)`.
- **`Game.runSeed`** (nuovo campo, `src/core/game_types.h`) porta il seed della run
  corrente e sopravvive al reset rapido R esattamente come `characterChosenIndex`
  (capture/restore in `GameUpdate`, `src/game/game.c`): premere R piu' volte sulla stessa
  run produce sempre la stessa sequenza, non piu' una nuova ad ogni pressione.
- **Test**: `--rng-seed-test` (`GameRngSeedTest`, `src/tests/game_tests.c`, in `make test`)
  rigioca l'inizio di una run con lo stesso seed due volte e confronta i nemici spawnati
  nella prima stanza di combattimento (tipo/posizione/hp) dopo un passo di `GameUpdate`
  vero: sequenze identiche a parita' di seed, diverse fra seed diversi.
- **Resta backlog**: la validazione ESTESA (mappa/spawn/drop/combattimento confrontati
  fra DUE giocatori/processi diversi sullo stesso manifest condiviso, non solo due reset
  nello stesso processo) e l'abilitazione vera della Classificata a stesso seed — vedi
  `docs/design/systems/run-manifest-and-reproducibility.md` e
  `docs/engineering/multiplayer-steam.md`.

## 4 — `generated/gen_progress_lazy.txt` non viene mai scritto dai processi reali

**Sintomo**: il percorso esiste come costante/argomento ma nessun processo
reale (`melting-gen`, `melting-sprites`, il gioco) lo popola durante una
generazione vera.

**Evidenza**: `src/app/app.c:159` passa
`"generated/gen_progress_lazy.txt"` come percorso atteso, ma
`tools/melting-gen/gen_util.c` e `tools/melting-sprites/sprite_util.c` non
contengono un varco di scrittura corrispondente collegato a un
`--progress-path` reale in `melting-gen` (l'opzione non e' implementata lato
generatore). Confermato anche in `docs/_meta/DOC-CODE-DRIFT.md` (voce su
`HANDOFF.md`, riga 300).

**Stato**: nessuna UI di progresso va costruita sopra questo file finche' non
viene aggiunto un `--progress-path` reale a `melting-gen` che lo scriva
davvero durante la generazione.

## 5 — Margine del prompt Lua sotto il tetto `n_ctx`: storicamente stretto, oggi ampio

**Sintomo storico**: in una fase precedente (`n_ctx=4096`), il prompt Lua
condiviso (cheat-sheet + few-shot) arrivava a un solo token di margine dal
tetto della sessione — un hint di rarita' scritto come frase intera invece
che poche parole ha fatto sforare il budget e mandato in fallback OGNI
oggetto di una run reale (osservato girando `make test-llm` col 7B).

**Stato verificato oggi**: `GEN_LLM_SESSION_N_CTX` e' stato alzato a **8192**
(non piu' 4096) proprio in risposta a quel problema — vedi
`tools/melting-gen/melting_gen.h:98` e il commento storico alle righe 44-96
che ripercorre gli aumenti (4096 -> 6144 -> 8192). Il margine attuale, misurato
col vocabolario reale del 7B, e' ampio: il ceiling in byte del prompt Lua
(`GEN_LUA_PROMPT_BYTE_CEILING`, `tools/melting-gen/gen_lua.h:212`) lascia
circa 4097 token liberi sotto il tetto, contro l'unico token di margine che
lo stesso prompt avrebbe avuto a `n_ctx=4096` (commento in
`tools/melting-gen/gen_lua.h:194-208`). La guardia `--prompt-budget-check`
resta comunque attiva in `make test-gen`, che invoca `scripts/test-gen.sh`:
la guardia e' alle righe 714-721 di **quello script** (NON del Makefile, che
ha 201 righe) e fa fallire il test se il prompt piu' grande di oggi supera il
ceiling.

**Nota per chi tocca il cheat-sheet**: il margine e' ampio ora, ma ogni fase
che ha aggiunto contenuto generato (tipi di colpo, poi nemici/boss) lo ha
eroso in passato fino quasi ad azzerarlo; ampliare in modo sostanziale il
cheat-sheet resta un rischio concreto e richiede rivalutare il budget (di
nuovo alzare `n_ctx` o accorciare il prompt), non solo affidarsi al margine
attuale.

## 6 — Manca un validatore visuale dell'atlas sprite prima di accettarlo

**Sintomo**: gli spritesheet generati dall'IA possono essere esteticamente
validi ma non tagliabili in celle perfette (celle disallineate rispetto alla
griglia attesa), e possono avere uno sfondo non coerente con il chroma-key
runtime che rimuove lo sfondo quasi nero. Nessun controllo automatico rifiuta
un atlas mal tagliato prima che entri in gioco.

**Evidenza**: `docs/archive/legacy-notes/issue-notes.md` (era
`docs/archive/legacy-notes/issue-notes.md`), sezioni "Da verificare prima di aprire
issue" ("I PNG della Image API possono ancora avere celle non perfette... 
Possibile miglioramento: validatore visuale prima di accettare la PNG come
atlas giocabile" e "I PNG generati... possono avere sfondo opaco... chroma-key
configurabile o richiesta di background coerente") e sezione "Risolte in
questo progetto" (il chroma-key runtime attuale e il fallback `--local-atlas`
sono la mitigazione gia' in codice, non un validatore).

**Stato**: backlog di prodotto, non implementato. Il chroma-key runtime e il
prompt tecnico vincolato restano l'unica mitigazione oggi.

## 7 — Il modello 1.5B non garantisce "stesso mondo che evolve" nei piani 2-5

**Sintomo**: DEC-005 impone che il Piano 1 apra "alla lettera" nel mondo/tema
scelto dal giocatore (non "un tema simile"); i Piani 2-5 restano invece
quelli generati liberamente dal modello, senza garanzia di continuita' tonale
forte con il tema scelto (l'asse di escalation/degenerazione tematica di
DEC-024 e' un affinamento separato, non una garanzia di aderenza al mondo
iniziale).

**Evidenza**: `tools/melting-gen/gen_validate.c:538-554` — la normalizzazione
forza `out->floors[0].theme` al tema scelto (`chosen->name`) quando presente,
con il commento esplicito: "I piani 2-5 restano quelli del modello
(l'aderenza 'stesso mondo che evolve' e' backlog separato, non questa
garanzia)". Design: `docs/design/governance/decision-log.md` DEC-005 (riga
77, Piano 1 alla lettera) e DEC-024 (riga 275, degenerazione come escalation
leggibile del tema, non identita' di mondo).

**Stato**: il motore (C) garantisce solo il minimo per il Piano 1; la piena
coerenza "stesso mondo" sui piani successivi dipende dal modello e non e'
misurata sistematicamente. Da rivalutare col 7B (non ancora misurato in modo
strutturato su questo asse specifico).

## 8 — RISOLTO (2026-07-27) — Il pool curato minimo VERO di `melting-gen` non aveva la garanzia di copertura DEC-144

**Sintomo (storico)**: DEC-144 ("il pool curato minimo garantisce almeno un oggetto per
rarita'") era esplicitamente scoped al pool curato minimo di 20 oggetti di
DEC-087 (`docs/design/systems/generated-content-validation.md` riga ~226).
Quel pool e' quello che `tools/melting-gen/gen_fallback.c` scrive su disco (5
piani x 3 oggetti + 5 bossItem = 20): ogni oggetto tirava la propria rarita'
individualmente con `GenRollRarity` (`tools/melting-gen/gen_util.c`, pesi
DEC-019 `{55,30,12,3}`), senza alcuna garanzia di copertura -- una run
generata da questo tool poteva legittimamente non contenere alcun oggetto
leggendario (0,6 leggendari attesi su 20, per arrotondamento spesso 0).

**Risolto durante il task "melting-gen emette e valida le 4 categorie"**:
`GenFallbackRun` (`tools/melting-gen/gen_fallback.c`) ora precalcola la rarita' delle 15
posizioni normali (bossItem escluso, tira sempre dai pesi boss come prima) con
`GenRarityMinimumCounts` (garanzia di copertura, stessa forma di `ItemPoolMinimumCounts`
ma duplicata ad-hoc in `gen_util.c` per non trascinare raylib dentro melting-gen via
`core/game_types.h`) e le rimescola con un RNG dedicato (`GenShuffleInts`) fra tutte le
posizioni della run, esattamente come gia' faceva `GenerateFallbackContent` lato motore.
`GenNormalizeRun` eredita la stessa garanzia (la rarita' di ogni oggetto viene sempre dal
fallback precalcolato, mai da un tiro indipendente). Golden file di regressione
(`tests/melting-gen/golden-fallback-seed12345.txt`) rigenerato di conseguenza (nuovo
consumo di RNG, atteso).

**Verificato da** (`make test-gen`, tre asserzioni che riguardano proprio la garanzia che
chiude il difetto -- non il generico "ogni valore e' uno dei 4 livelli" della fase 3b, che
c'era gia' prima e sarebbe passato anche col difetto aperto):

1. copertura sul seme 12345 (`for r in common uncommon rare legendary` sulle 15 posizioni
   normali, gemello del ciclo sui kind);
2. la stessa copertura su altri 7 semi (1, 2, 3, 7, 42, 100, 31337) -- e' una garanzia per
   costruzione, non una probabilita', quindi deve reggere su ogni seme;
3. `--taxonomy-check` (ramo senza modello ne' RNG di `tools/melting-gen/main.c`), che chiama
   `GenRarityMinimumCounts` direttamente e ne confronta l'uscita con l'esempio **normativo**
   del documento (pool da 20, pesi standard -> `{11,6,2,1}`) e col floor delle 15 posizioni
   vere (`{9,4,1,1}`). Serve perche' nessun manifest esercita il pool da 20 di DEC-144:
   senza questo ramo la copia dell'algoritmo dentro `melting-gen` e quella del motore
   (`ItemPoolMinimumCounts`, verificata da `ItemPoolTestMinimumCounts` in `make test`)
   potrebbero divergere con tutte le suite verdi.

## 9 — Modulo audio (DEC-172): una lacuna residua (la seconda chiusa a metà da W8)

**Contesto**: `src/audio/audio.{h,c}` (nuovo, W4) copre la mappatura musica/stato,
il crossfade, il duck di `PauseMenu` e i dieci SFX a evento elencati nel task
(sparo, colpo a nemico, danno subito, pickup, porta, fusione, card di scoperta,
navigazione/conferma/annulla UI). Restano queste lacune note, deliberate per lo scope
del task, non un difetto di implementazione dentro quello scope:

1. **Famiglia sonora del Piano 0 (DEC-121) assente**: `docs/design/content/
   audio-and-feedback.md` chiede due segnali dedicati per "scelta del tema" e
   "generazione completata" del Piano 0 (`AppConfirmThemeChoice`/
   `AppOpenFloorZeroExit`, `src/app/app.c`). Il pacchetto pre-generato in
   `assets/audio/sfx/` non contiene tracce per questi due eventi (solo i dieci
   della lista sopra): nessun hook e' stato aggiunto per non riciclare un suono
   semanticamente scorrelato (es. `ui_confirm`) su un evento che il documento
   vuole riconoscibile come famiglia propria. Richiede due nuovi asset prima di
   poter chiudersi.
2. ~~**Nessuna voce di volume in `Options`**~~ — **CHIUSA (parte UI) da W8**.
   `APP_OPTIONS` ha ora tre righe-slider (`Volume generale`, `Musica`, `Effetti`)
   agganciate ad `AudioSetMasterVolume/MusicVolume/SfxVolume`: su/giù scelgono la
   riga, sinistra/destra cambiano il valore a passi di `OPTIONS_VOLUME_STEP` (10%,
   dieci caselle), ENTER esce solo dalla riga "Indietro", ESC esce sempre — parità
   tastiera/controller di DEC-057 senza codice dedicato. Il suono di navigazione fa
   da anteprima del volume appena scelto. Il valore si legge sia dalle caselle sia
   dalla percentuale scritta (DEC-058: nessuna informazione dal solo colore).
   `src/app/app.c` (case `APP_OPTIONS`), `DrawOptionsOverlay`/`DrawOptionsSliderRow`
   in `src/render/game_renderer.c`, costanti in `src/audio/audio.h`.
   **Cosa RESTA aperto**: (a) i volumi **non persistono** fra un avvio e l'altro — il
   gioco non ha un file di configurazione, e inventarne uno avrebbe voluto dire
   decidere da soli percorso, formato e migrazione, tre cose che
   `docs/design/ui/options-and-accessibility.md` non fissa; (b) passo, etichette e
   ordine sono un **default proposto** (stile DEC-019), non canone: quel documento
   elenca "audio" fra le categorie minime senza fissare slider né valori.
   Entrambi in `docs/design/governance/open-questions.md`.
   Le altre quattro categorie minime del documento (video, controlli, accessibilità,
   gameplay) restano da scrivere e W8 non le ha inventate.

**Verificato da**: `--audio-test` (`src/tests/audio_tests.c`, `GameAudioTest`) --
mappatura pura stato/piano/stanza-boss -> traccia, clamp dei volumi, ciclo di vita
init/shutdown ripetuto con e senza device audio reale e con/senza `Game`, mai un
crash. Gira sotto Xvfb senza alcun backend audio (ambiente di CI/sviluppo di
questo repo): `AudioIsDeviceReady()` false dopo `AudioInit()` e' lo scenario
REALE esercitato da `make test`, non solo un caso sintetico.

## 10 — Consumo del pacchetto artistico (W8): buchi di ASSET dichiarati, non mascherati

**Contesto**: `src/assets/art_atlas.{h,c}` + `src/render/art_draw.{h,c}` agganciano al
motore le 73 coppie spritesheet+manifest di `assets/art/`: personaggio, nemici, boss,
oggetti, colpi, prop, i 5 tileset dei temi e i quattro componenti di sistema
dell'interfaccia. Ogni buco qui sotto è un asset che **non esiste ancora**, non un
percorso di codice mancante: il motore lo cerca, non lo trova, e degrada al percorso
precedente (immagine curata → cella d'atlas → primitiva geometrica). Tutti da chiudere
con un giro artistico dedicato (**CP4**) e un giro di aggancio motore dedicato
(**WP-INT**, 30/07, voci 1/2/3 sotto): il pacchetto artistico da solo bastava per i
casi a puro `propKey` (già il caso di lingotto/Flux/crogiolo/clessidra in W8/WP4/WP5,
e ora anche di cuore/bomba/chiave), ma font esteso e selezione dello sheet di
personaggio hanno richiesto anche codice nuovo (parser/decoder UTF-8, mappa
indice→sheet) — non erano un semplice "punta a questa chiave", contrariamente a quanto
la frase precedente lasciava intendere quando l'unico gap era l'asset mancante.

1. **RISOLTO (2026-07-30, WP-INT) — Font `font-5px`: mancavano le accentate italiane e
   le parentesi tonde.** Le parentesi erano già disegnabili (estensione diretta della
   chiave `"glyphs"`, nessun codice nuovo servito). Le sei accentate italiane (`à è é ì
   ò ù`) vivono ora in una chiave separata del manifest, `"glyphs_ext"` (codepoint
   decimale → `{x,w}`, mai byte UTF-8 grezzi dentro `"glyphs"`: avrebbe rotto la
   garanzia "solo ASCII" dello scanner sequenziale, vedi `src/assets/art_atlas.h`
   righe 9-17). `ParseGlyphsExt`/`ArtSheetGlyphExt` (`src/assets/art_atlas.{h,c}`)
   leggono la chiave nuova; `ArtDrawText`/`ArtTextWidth` (`src/render/art_draw.c`)
   decodificano l'UTF-8 in ingresso (sequenze 1-2 byte, Latin-1 Supplement) con
   `ArtUtf8Decode` e piegano minuscola→maiuscola accentata con `ArtUpperCodepoint`
   prima del lookup — fattorizzati in un solo `ArtResolveGlyph` condiviso dalle due
   funzioni, per non duplicare due volte il percorso di risoluzione. **Resta fuori
   dal set** qualunque carattere oltre queste sei accentate (es. Ç/Ñ/Ü): avanza come
   uno spazio, stesso degrado di sempre per un glifo assente — margine per `ART_GLYPH_EXT_MAX`
   (16) se un domani servissero. I testi di gioco esistenti restano scritti SENZA
   accentate (limite storico del font, riscriverli è fuori scope: lo abilita, non lo
   impone, un giro contenuti separato). Verificato da `--art-atlas-test`
   (`src/tests/art_atlas_tests.c`): parsing di `glyphs_ext` da fixture e dal manifest
   reale, larghezza/risoluzione end-to-end di una stringa con accentata via
   `ArtTextWidth`, degrado invariato per un codepoint esteso ancora fuori set.
2. **RISOLTO (2026-07-30, WP-INT) — Un solo spritesheet di personaggio.** I tre
   personaggi della rosa base ora hanno ciascuno il proprio sheet pixel art
   (`character/fonditrice`/`ashblade`/`bulwark`, strutturalmente identici — walk 4
   direzioni, idle, hit, death, anchor `[16,28]`): il motore sceglie lo sheet
   dall'indice del personaggio scelto (`CharacterSheetKey`, `src/render/
   game_renderer.c`), stesso ordine di `content/character_roster.c`. Il personaggio
   generato per-run mostra `character/fonditrice` — **default proposto**, non canone
   (`docs/design/systems/characters.md`, "Default proposti dall'implementazione";
   `governance/open-questions.md` punto 36), perché non esiste ancora una pipeline
   che generi uno sheet dedicato per-run. Della palette del personaggio si conserva
   ancora solo l'**alfa** (il lampeggio di invulnerabilità): tingere uno sprite
   disegnato con un colore ne sporcherebbe la palette — questa parte NON è un gap,
   è la stessa scelta intenzionale di sempre, ora esplicita per tutti e tre gli
   sheet.
3. **RISOLTO (2026-07-30, WP-INT) — Cuore, bomba e chiave non avevano un prop a
   terra.** `assets/art/props` ha ora anche `pickup-cuore`/`pickup-bomba`/
   `pickup-chiave` (idle a 2 fotogrammi, anchor al piede, stesso vocabolario di
   `pickup-lingotto`/`pickup-flux`): tutte e cinque le raccolte disegnano ora il
   proprio prop a priorità più alta della cella d'atlas generata per-tema
   (`DrawPickup`, `src/render/game_renderer.c`). Energia (DEC-059) e uscita restano
   primitive **per decisione**, non per mancanza di asset (aggiungere una cella
   d'atlas invaliderebbe ogni atlas già generato) — invariato da questo lavoro.
4. **RISOLTO (2026-07-30, WP2) — Salute temporanea (DEC-008)**: `Player.tempHp`
   (`src/core/game_types.h`) è il secondo strato della salute stratificata; `CombatDamagePlayer`
   (`src/gameplay/combat.c`) lo consuma PRIMA della salute base, con l'eccedenza nello stesso
   evento, e la cura normale (`PICKUP_HEART`) non lo tocca mai. Il cap è un default proposto
   dall'implementazione (`PLAYER_TEMP_HP_CAP = 4`, non canone — vedi
   `docs/design/governance/open-questions.md` voce 28); la fonte scelta per la demo è il
   negozio (`PICKUP_CRUST`, `WorldShopStocksCrust` in `src/world/world.c`, stessa tecnica
   hash-based del Flux). L'icona `heart_temp` **si disegna** ora accanto ai cuori base
   (`DrawHudV3TempHearts`, `src/render/game_renderer.c`, layout V3), con un ripiego testuale
   `+N` nel cluster senza pacchetto artistico (`DrawHudVitals`, DEC-058: mai solo colore).
   Verificato da `--temp-health-test` (`GameTempHealthTest`, in `make test`).
5. **RISOLTO (2026-07-30) — Timer di run (DEC-051)**: `Game.runElapsedSeconds` accumula
   durante la run vera e non nell'esplorazione del Piano 0 (`game->inRealRun`, WP1: la
   sala d'attesa mette anch'essa `PHASE_PLAY` per essere giocabile, M1b, ma non è una run
   cronometrata) e `FloorZeroEnter` lo riazzera all'ingresso nell'hub, così la seconda
   visita non mostra congelato il tempo della run precedente. Si disegna centrato in alto
   nel layout V3 (`DrawHudCanvas`, formato `m:ss`), nel ripiego senza pacchetto artistico
   (`DrawHudRunStatus`, riga "Tempo: m:ss" nel cluster di progressione) e in `RunResults`
   (`DrawRunResultsOverlay`, coerente con DEC-056). Verificato da `--run-timer-test`
   (`GameRunTimerTest`, in `make test`), che copre anche il blocco durante
   `PHASE_GAME_OVER`/`PHASE_WIN` e l'azzeramento all'ingresso nel Piano 0.
6. **Le fasce di muro restano quelle decorative storiche** (34/12/14 px, non una fila
   intera di tile da 32): sono lo spessore che la resa 2.5D dichiara da sempre ed è
   ancorato al bordo REALE del campo di gioco. Allargarle a 32 px per lato avrebbe
   spostato la parete di ~20 px rispetto alla collisione, cioè avrebbe fatto camminare
   il giocatore dentro il muro. Il tile viene ritagliato alla fascia (il sorgente, non
   compresso): il muro laterale mostra i suoi primi 12 px. Se in futuro si vuole una
   parete "piena", va cambiato il campo di gioco, non solo il disegno.
7. **Tema generato → tileset per hash**. `Theme` non ha un identificatore: si prova lo
   SLUG del nome come nome di file (`"Lunar Forge"` → `tiles/lunar-forge`, il cammino
   dei 5 temi curati) e, per un tema inventato dal modello (`"Library of Radiation"`),
   si sceglie uno dei cinque per hash FNV-1a del nome — deterministico, quindi lo stesso
   mondo si veste sempre allo stesso modo. Non è un difetto risolvibile con più codice:
   un tileset per tema generato andrebbe GENERATO, che è la Style LoRA di DEC-148.

**Verificato da**: `--art-atlas-test` (`src/tests/art_atlas_tests.c`, in `make test`) --
parser dei tre sapori di manifest comprese le estensioni, manifest rotti/troncati,
animatore deterministico ai confini esatti dei fotogrammi, cache e voci negative,
risoluzione a priorità degli image-id, degrado con manifest rotto / PNG assente / chiave
con `..`. `--atlas-fallback-test` esercita di proposito il gradino più basso (pacchetto
artistico puntato su una cartella inesistente → primitive). Gli screenshot di
`--art-screens-screenshot-test` (`logs/worldsmelt-w8-*.png`) sono la verifica visiva.

## 11 — La persistenza dei distruttibili spaccati (WP3, DEC-043) non è ancora osservabile in gioco

**Sintomo**: `docs/design/systems/secrets-and-obstacles.md` ("Ostacoli generati a tema")
descrive lo stato "spaccato" di un ostacolo distruttibile come qualcosa che persiste "per
tutta la permanenza sul piano corrente, uscendo e rientrando nella stessa stanza". Il
motore registra davvero questo stato per cella/piano (`Game.destroyedObstacleMask`,
`CombatExplodeAt` con `breach=true` lo marca, `WorldBuildObstacles` non rimette sullo
scaffale un distruttibile già marcato) — ma non esiste, nel gioco vero, nessuna sequenza
in cui un giocatore possa uscire e rientrare in una stanza di combattimento ancora aperta
per osservarlo.

**Evidenza**: `src/world/world.c` — `WorldBuildObstacles` esce subito (nessun ostacolo
ricostruito, di NESSUNA famiglia) quando `room->cleared` è vero; `GameRoomIsLocked` tiene
bloccate le porte di una stanza `ROOM_COMBAT`/`ROOM_BOSS` finché non è `cleared`;
`WorldCheckRoomClear` marca `cleared = true` nello stesso istante in cui muore l'ultimo
nemico. Non esiste quindi una finestra osservabile in cui una stanza di combattimento sia
sia "già visitata con un distruttibile spaccato" sia "ancora aperta e rientrabile":
appena il giocatore può uscire e rientrare, la stanza è già `cleared` e non ha più nessun
ostacolo, di nessuna famiglia (comportamento del motore preesistente a WP3, non toccato da
questo lavoro).

**Stato**: infrastruttura implementata e testata direttamente (`GameObstaclesTest`, test
(a) in `src/tests/game_tests.c`, che chiama `WorldSpawnRoomContents` due volte sullo stesso
`Game` per esercitare il meccanismo senza passare da una transizione di stanza vera) in
vista delle stanze segrete di un lavoro successivo, che potranno rientrare più volte prima
di essere "ripulite" in quel senso. Non è un difetto da correggere in isolamento: o si fa
sopravvivere l'arredo di una stanza di combattimento alla sua ripulitura (cambio di
comportamento più ampio del solo WP3), oppure la si tratta come infrastruttura in attesa
del task delle stanze segrete. `docs/design/systems/secrets-and-obstacles.md` registra lo
stesso limite nella sua sezione "Default proposti dall'implementazione".

## 12 — Le celle note-ma-non-visitate della minimappa distinguono il tipo di stanza SOLO col colore (DEC-058)

**Sintomo**: `docs/design/systems/special-rooms.md` (WP4, "Stanza di fusione") chiede un
segnale visivo "prima di entrare" nella stanza, distinguibile senza colore (DEC-058, che
vieta di affidare un'informazione al solo colore). Nel motore l'icona di `DrawRoomIcon`
(`"T"` tesoro, `"$"` negozio, `"B"` boss, `"F"` fusione) compare SOLO sulla cella di stato
di una stanza già `visited`; una cella nota (adiacente a una visitata, quindi disegnata
sulla minimappa) ma non ancora visitata si distingue dalle altre unicamente per la tinta
smorzata del suo `RoomMapColor` — nessun canale non-colore.

**Evidenza**: `src/render/game_renderer.c`, `DrawMinimap` — `if (room->visited && room ==
&game->rooms[y][x]) DrawRoomIcon(...)`; il colore di base (`base = room->visited ?
RoomMapColor(...) : GameColorLerp(RoomMapColor(...), ..., 0.7f)`) è l'UNICO segnale sulle
celle non visitate.

**Stato**: preesistente a WP4 per tesoro/negozio/boss (il commento sopra `DrawMinimap`
dichiara la scelta come intenzionale: "un pizzico di scoperta, come in Isaac" — le icone
si sbloccano visitando, non prima). WP4 eredita lo stesso limite per la stanza di fusione
invece di introdurne uno nuovo: non è stato corretto perché la correzione naturale (mostrare
l'icona anche sulle celle note-ma-non-visitate) toglierebbe la stessa scoperta anche a
tesoro/negozio/boss, una scelta di design più ampia del solo WP4 e non richiesta da nessuna
DEC. Da decidere insieme al proprietario: o si accetta che la garanzia DEC-058 valga solo
DOPO l'ingresso (e si aggiorna DEC-058/special-rooms.md di conseguenza), o si introduce un
canale non-colore anche per le celle non visitate (es. un bordo o pattern distinto per
"stanza speciale nota", senza svelarne il tipo esatto).

**Aggiornamento WP5 (30/07)**: la stanza a tempo (`ROOM_TIMED`) eredita lo stesso limite
pre-ingresso — `"!"` in `DrawRoomIcon` compare solo su `room->visited`, come `"F"` per
`ROOM_FUSION`. A differenza del limite pre-ingresso, l'esito DENTRO la stanza (raggiunta in
tempo o no) NON dipende solo dal colore: `DrawPickup` scrive sempre un'etichetta testuale
("IN TEMPO"/"SCADUTO") accanto alla clessidra, indipendente dal caricamento dello sprite —
quel canale non-colore esiste già per lo stato interno alla stanza, il gap resta solo
sulla minimappa prima di entrarci.

**Aggiornamento WP6 (30/07)**: l'arena di sfida (`ROOM_ARENA`) eredita lo stesso limite
pre-ingresso — `"A"` in `DrawRoomIcon` compare solo su `room->visited`, come `"F"` e `"!"`.
Nessuna garanzia nuova viene dichiarata: prima di entrarci l'arena si distingue solo per
il suo colore dedicato (blu) sulla minimappa. DENTRO la stanza, invece, i tre stati della
sfida — disponibile / in corso / superata — NON dipendono mai dal solo colore: il segnale
(`PICKUP_ARENA_ALTAR`) porta sempre un'etichetta testuale (`"SFIDA"`/`"IN CORSO"`/
`"SUPERATA"`) scritta anche quando lo sprite non carica, oltre alla forma dedicata e al
colore di stato — stessa struttura a due canali già usata dalla clessidra della stanza a
tempo.
