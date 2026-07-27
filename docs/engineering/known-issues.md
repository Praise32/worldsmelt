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
last_reviewed: 2026-07-27
last_verified_commit: dab9140
topics: [difetti, limiti, test, rng, generazione, catalogo]
related: [eng-dependencies, meta-doc-code-drift, gd-system-run-manifest]
supersedes: []
source_files: [src/tests/game_tests.c, src/content/run_catalog.c, scripts/test-llm.sh, scripts/test-gen.sh, src/game/game.c, src/app/app.c, tools/melting-gen/gen_util.c, tools/melting-sprites/sprite_util.c, tools/melting-gen/gen_lua.h, tools/melting-gen/melting_gen.h, tools/melting-gen/gen_validate.c, docs/archive/legacy-notes/issue-notes.md]
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

**Sintomo**: `GameResetRun` inizializza l'RNG di gioco con l'orologio di
sistema, non con un seed derivato dal seed del run scelto/condiviso. I
contenuti generati (tema, layout, oggetti) sono deterministici dal seed
(`melting-gen` prende `--seed` esplicito), ma il **gameplay** (spawn, drop,
RNG di combattimento durante la run) no.

**Evidenza**: `src/game/game.c:99-103` — `void GameResetRun(Game *game)` con
`game->rng = (unsigned int)time(NULL) ^ 0x514AACu;`.

**Impatto**: blocca le gare asincrone eque fra giocatori sullo stesso seed
(DEC-016 multiplayer asincrono via classifiche, DEC-062, DEC-066): due run
con lo stesso seed di generazione possono comunque divergere nel gameplay
perche' l'RNG di run non e' derivato da quel seed.

**Stato**: backlog aperto, nessuna correzione applicata. **DEC-141** (25/07) fissa il fix
come prerequisito bloccante di qualunque gara Classificata a stesso seed: nessuna gara del
genere va abilitata finché questo RNG non deriva dal seed di run (vedi anche
`docs/design/systems/run-manifest-and-reproducibility.md` e
`docs/engineering/multiplayer-steam.md`).

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
