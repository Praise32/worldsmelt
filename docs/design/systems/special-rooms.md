---
id: gd-system-special-rooms
title: Special Rooms
domain: design
status: approved
authority: canonical
owner: design
summary: "Dettaglio dei cinque archetipi speciali (DEC-010, esteso da DEC-051): fusione, segreta a due livelli (DEC-025), arena di sfida, scambio ad alto rischio — unico luogo per patti a costo salute (DEC-026), con offerta e prezzo generati dentro un budget di equità (DEC-044) — e stanza a tempo nei piani avanzati (DEC-051) — sottoinsieme dichiarato della tassonomia di rooms-and-floor-generation.md. La stanza di fusione (WP4, 30/07), la stanza a tempo (WP5, 30/07) e l'arena di sfida incontrata nel piano (WP6, 30/07) sono i primi tre dei cinque archetipi con un RoomKind fisico nel motore."
last_reviewed: 2026-07-30
last_verified_commit: 06b9b16
topics: [stanze-speciali, fusione, scambio-alto-rischio, arena-di-sfida, stanza-a-tempo, WP4, WP5, WP6, ROOM_FUSION, ROOM_TIMED, ROOM_ARENA]
related: []
supersedes: []
source_files: [src/world/world.c, src/core/game_types.h, src/gameplay/combat.c, src/render/game_renderer.c]
---

# Special Rooms

## Intento per il giocatore

Le stanze speciali offrono decisioni fuori dal combattimento standard: rischio, scambio, scoperta o sfida opzionale, con un segnale chiaro che le distingue dalle stanze standard (dove la scoperta non è parte del design).

## Condizioni di ingresso

La tassonomia completa dei tipi di stanza (standard + speciali) è definita in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md); questo documento non la ridefinisce. Qui si dettagliano i quattro archetipi speciali dichiarati da DEC-010 — stanza di fusione, stanza segreta, arena di sfida, scambio ad alto rischio — più il quinto archetipo aggiunto da DEC-051: la stanza a tempo.

**Nota sul negozio:** il negozio è un tipo di stanza **standard** (definito in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), non uno dei quattro archetipi speciali qui descritti. Il negozio ha prezzi base fissi per fascia di rarità più un'offerta speciale generata per negozio (DEC-026): il dettaglio economico vive in [rewards-and-economy.md](./rewards-and-economy.md), non ripetuto qui. Lo scambio ad alto rischio — in-game **Pourhouse**, «Casa della Colata» (DEC-136) — è un archetipo diverso dal negozio: offre scambi rischiosi o non convenzionali, con presentazione e regole originali — mai nominato o presentato con riferimenti a giochi esistenti. **Confine netto (DEC-026):** i "patti" a costo salute (cedere salute in cambio di un guadagno) non esistono nel negozio; restano esclusivi dello scambio ad alto rischio.

## I cinque archetipi

### Stanza di fusione

Ospita la meccanica-firma di fusione esplicita tra due oggetti. Il dettaglio della meccanica (input, catalizzatore di fusione, risultato generato) è descritto in [item-fusion.md](./item-fusion.md); questo documento non lo ripete, colloca solo la stanza nella tassonomia e ne descrive l'accesso.

### Stanza segreta

Stanza non indicata direttamente sulla mappa, a **due livelli** (DEC-025): "normale", con
indizi visivi leggibili (crepe, anomalie del tema) apribile con lo strumento di breccia; e
"super-segreta", senza indizi, trovabile solo con oggetti/Innesti rivelatori o intuizione
estrema. Il dettaglio dei due livelli è descritto in
[secrets-and-obstacles.md](./secrets-and-obstacles.md); questo documento non lo ripete,
colloca solo l'archetipo nella tassonomia.

### Arena di sfida

Stanza opzionale con combattimento più impegnativo in cambio di ricompensa maggiore. È anche accessibile in versione "best-of" dal Piano 0, usando contenuti già validati delle run passate (DEC-004): vedi [floor-zero.md](./floor-zero.md) per il dettaglio di questo accesso alternativo; questo documento descrive solo la versione incontrata durante il piano.

**Stato (WP6, 30/07):** la versione **incontrata nel piano** ha ora un `RoomKind`
fisico nel motore (`ROOM_ARENA`) — vedi "Stato di implementazione: l'arena di sfida
nel piano" sotto. L'accesso "best-of" dal Piano 0 resta **non implementato** e dipende
dal museo delle creazioni descritto in [floor-zero.md](./floor-zero.md): le due versioni
sono distinte, e questo lavoro non ne anticipa nulla.

### Scambio ad alto rischio

Stanza che propone uno scambio non convenzionale (es. cedere una risorsa, salute o una parte della build per un guadagno maggiore ma incerto), ri-tematizzata in modo originale. È l'**unico** archetipo dove sono ammessi scambi a costo salute (DEC-026): il negozio non li offre mai. Nome e presentazione precisi restano da definire in fase di contenuto, ma la funzione — rischio dichiarato in cambio di un guadagno superiore alla media — è fissata da DEC-010.

### Stanza a tempo (DEC-051)

Stanza fissa dei **piani avanzati**: se il giocatore la raggiunge entro una soglia di tempo,
ottiene una ricompensa aggiuntiva. Coerente con il timer di run sempre visibile nell'HUD
(DEC-051, vedi [HUD](../ui/hud.md)): il gioco si dichiara esplicitamente una corsa, e questo
archetipo rende quella dichiarazione parte del level design nei piani avanzati. Il dettaglio
della ricompensa vive in [Rewards and Economy](./rewards-and-economy.md) come fonte unica;
questo documento colloca solo l'archetipo nella tassonomia e ne descrive l'accesso.

**Estensione della tassonomia (DEC-051):** questo è un quinto archetipo speciale, aggiunto
da DEC-051 ai quattro originali di DEC-010.

#### Puntata generata dentro un budget di equità (DEC-044)

Nella stanza di scambio l'IA genera **sia l'offerta sia il prezzo** dentro un **budget di
equità**: non sono coppie fisse curate, ma una puntata composta per l'occasione. Il prezzo
può essere:

- salute, immediata o massima (riduzione del tetto, non solo del valore corrente);
- un oggetto o Innesto posseduto dal giocatore;
- valuta principale;
- perfino un catalizzatore di fusione.

Ogni scambio proposto è diverso dagli altri. L'**equità** tra offerta e prezzo è garantita
dal budget di equità, non da una tabella fissa; la validazione della coppia offerta/prezzo
segue le regole generali di
[Generated Content Validation](./generated-content-validation.md), non riformulate qui. Il
negozio resta invariato (DEC-026): questa generazione è esclusiva della stanza di scambio ad
alto rischio.

## Input/azioni

Il giocatore decide se entrare (quando l'ingresso è opzionale o a costo) e compie l'azione specifica dell'archetipo: fondere due oggetti, cercare un varco nascosto, affrontare la sfida, accettare o rifiutare lo scambio.

## Risultato

Ogni stanza speciale produce un esito dichiarato (oggetto fuso, accesso a una ricompensa nascosta, ricompensa da sfida superata, esito dello scambio) proporzionato al rischio o costo pagato (vedi [rewards-and-economy.md](./rewards-and-economy.md) per il principio generale di proporzione rischio/ricompensa).

## Feedback

- segnale visivo che distingue una stanza speciale da una stanza standard prima dell'ingresso, dove la scoperta non è parte del design (fusione, arena, scambio);
- per la stanza segreta, l'assenza di segnale diretto è intenzionale (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md));
- conferma esplicita prima di un'azione irreversibile (fusione, scambio ad alto rischio).

## Interazioni

- con la tassonomia generale delle stanze ([rooms-and-floor-generation.md](./rooms-and-floor-generation.md));
- con la meccanica di fusione ([item-fusion.md](./item-fusion.md)) e il catalizzatore di fusione ([health-and-resources.md](./health-and-resources.md), [items-pools-and-rarity.md](./items-pools-and-rarity.md));
- con il Piano 0 per l'accesso "best-of" all'arena di sfida ([floor-zero.md](./floor-zero.md));
- con l'economia per lo scambio ad alto rischio ([rewards-and-economy.md](./rewards-and-economy.md));
- con la regola di validazione generale per la coppia offerta/prezzo generata ([generated-content-validation.md](./generated-content-validation.md), DEC-044);
- con la ricompensa della stanza a tempo ([rewards-and-economy.md](./rewards-and-economy.md), DEC-051) e con il timer sempre visibile ([hud.md](../ui/hud.md)).

## Stato di implementazione: la stanza di fusione (WP4, 2026-07-30)

Primo dei quattro archetipi speciali di questo documento ad avere un `RoomKind`
fisico nel motore (`ROOM_FUSION`, `src/core/game_types.h`), oltre alla
meccanica-firma stessa (già implementata, vedi [Item Fusion](item-fusion.md),
"Stato di implementazione").

- **Piazzamento**: `WorldPlaceSpecialRoom` (`src/world/world.c`) — lo stesso
  algoritmo di tesoro/negozio: un tentativo per piano, taglia 1x1, mai
  adiacente alla stanza boss (DEC-182), deterministico dal seed del piano. Non
  garantito: su una griglia satura il piano resta senza stanza di fusione per
  quel giro. Misurato su 120 piani generati (5 piani × 24 semi, `--rooms-test`):
  piazzata in 119 casi su 120.
- **Crogiolo interagibile**: un `Pickup` di kind dedicato (`PICKUP_FUSION_ALTAR`)
  al centro della stanza, ri-materializzato a ogni ingresso come l'arredo di
  ogni altra stanza. A differenza di ogni altro pickup non si consuma mai
  (`CombatPickup`, `src/gameplay/combat.c`): toccarlo scrive
  `Game.fusionRoomTriggered`, consumato da `UpdateApp` (`src/app/app.c`) che
  apre `BuildScreen` — lo stesso schermo, già pronto alla fusione, che TAB e la
  voce "Build e sinergie" del PauseMenu aprono da sempre. Stesso schema di
  "blocco che si scioglie allontanandosi" del piedistallo degli attivi
  (DEC-117), così il crogiolo resta ritoccabile per tutta la permanenza nella
  stanza invece che un varco a uso singolo.
- **Scenario 4 rispettato senza codice dedicato**: la stanza non sa nulla dei
  requisiti di fusione (due oggetti idonei, un catalizzatore) — apre sempre
  `BuildScreen`, che mostra `FusionStatusText` quando l'azione non è ancora
  disponibile. Nessun ramo speciale nella stanza stessa.
- **L'accesso globale RESTA** (TAB da Gameplay, voce dal PauseMenu): rete di
  sicurezza esplicita di questa demo — una run non deve mai dipendere dal
  trovare fisicamente questa stanza per poter fondere, coerente col non
  piazzamento garantito sopra. `item-fusion.md` aggiorna la sua riga "Dove si
  innesca" di conseguenza: entrambi i percorsi convivono, non si è sostituito
  l'uno con l'altro.
- **Segnale visivo (DEC-058)**: colore dedicato in `RoomMapColor` (violetto,
  distinto dall'ambra del crogiolo del Piano 0) e icona `"F"` in `DrawRoomIcon`
  (`src/render/game_renderer.c`) — DOPO essere entrati, la stanza si distingue
  anche senza colore (icona sulla sua cella di stato sulla minimappa). PRIMA
  di entrare no: `DrawMinimap` disegna `DrawRoomIcon` solo per `room->visited`
  (stesso pattern preesistente di tesoro/negozio/boss, "un pizzico di
  scoperta, come in Isaac"), quindi una cella nota-ma-non-visitata si
  distingue oggi solo dalla tinta smorzata del suo colore — un limite reale
  rispetto a DEC-058, non solo di questa stanza. Registrato come gap esplicito
  in `docs/engineering/known-issues.md`, voce 12.
- **Sprite del crogiolo**: `assets/art/props/crogiolo` (corsia arte, prodotto
  30/07) è nel dataset curato con vocabolario di tag dedicato — `"spento"` /
  `"attivo"` — e `DrawPickup` seleziona sempre `"attivo"` (il crogiolo è
  sempre utilizzabile, Scenario 4). Se dovesse mancare (rigenerazione,
  dataset parziale) si ripiega su `assets/art/props/piedistallo`, che usa un
  vocabolario diverso — `"vuoto"` / `"pieno"` — e sceglie sempre `"pieno"`; se
  anche quello manca, forma geometrica di riserva (basamento + fiamma). Mai
  un pickup invisibile.
- **Test**: `RoomsTestFusionInteraction` dentro `GameRoomsTest`
  (`src/tests/game_tests.c`, `--rooms-test`/`make test`) — unicità e non
  adiacenza al boss su 120 piani generati, più la catena intera
  tocco-crogiolo → `fusionRoomTriggered` → `UpdateApp` apre `APP_BUILD_SCREEN`
  → si sblocca allontanandosi.

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Frequenza per piano** | Un tentativo per piano, non garantito (stesso algoritmo di tesoro/negozio). La domanda "Frequenza esatta di ciascun archetipo per piano" (sotto, "Domande aperte residue") resta aperta per gli ALTRI tre archetipi (segreta, arena, scambio), che non esistono ancora nel motore. | `WorldGenerateFloorMap`, `src/world/world.c` |
| **Taglia della stanza** | Sempre 1x1 (stesso default di tesoro/negozio, DEC-170): una fusione è una decisione, non serve spazio di combattimento. | `WorldPlaceSpecialRoom` |
| **Interazione** | Tocco automatico (overlap col crogiolo), non un tasto dedicato — stesso pattern dei piedistalli esistenti (DEC-117), coerente con "conferma esplicita solo per l'azione irreversibile" (la fusione vera, dentro BuildScreen), non per l'ingresso alla schermata. | `CombatPickup`, `src/gameplay/combat.c` |
| **Valuta di completamento** | Nessuna: la stanza non ha una condizione di "ripulita" a cui agganciarla (a differenza di combattimento/tesoro/negozio/boss). | `WorldAwardRoomCompletionCurrency`, `src/world/world.c` |

## Stato di implementazione: la stanza a tempo (WP5, 2026-07-30)

Secondo dei cinque archetipi speciali di questo documento ad avere un
`RoomKind` fisico nel motore (`ROOM_TIMED`, `src/core/game_types.h`), in coda
dopo `ROOM_FUSION` (WP4).

- **Esclusiva dei piani avanzati**: `WorldGenerateFloorMap` prova il
  piazzamento SOLO quando `game->floor >= 3` — stesso confine già scelto per
  l'escalation del tileset (DEC-024) e il passaggio dei boss a due fasi
  (DEC-028/106, [open-questions.md](../governance/open-questions.md) voce 23).
  Nei piani 1-2 non compare mai: non è solo un default di frequenza, è parte
  della decisione stessa (DEC-051, "esclusiva dei piani avanzati").
- **Piazzamento**: `WorldPlaceSpecialRoom` (`src/world/world.c`) — lo stesso
  algoritmo di tesoro/negozio/fusione: un tentativo per piano, taglia 1x1, mai
  adiacente alla stanza boss (DEC-182), deterministico dal seed del piano. Non
  garantito. Il giro di prova genera 120 piani (5 piani × 24 semi,
  `--rooms-test`), ma solo i piani 3-5 (72 = 3 × 24) sono candidati — i piani
  1-2 (48 casi) non tentano nemmeno il piazzamento: **piazzata in 40 casi su
  72 tentativi** (era 69/72 prima del WP6: l'arena di sfida si piazza prima
  delle speciali 1x1 e le toglie qualche cella libera — vedi "Stato di
  implementazione: l'arena di sfida nel piano" sotto, dove il prezzo è
  dichiarato e misurato).
- **Soglia di tempo**: misurata da `Game.floorEntryElapsedSeconds` (il valore
  di `runElapsedSeconds` catturato da `WorldStartFloor` all'ingresso nel
  piano, MAI dall'inizio della run) — `WorldTimedRoomThresholdSeconds`
  (`src/world/world.c`) restituisce `40s + 6s × Game.floorCellCount`, dove
  `floorCellCount` è la taglia VERA del piano appena generato (celle totali:
  partenza + combattimento + boss + speciali 1x1), non il bersaglio
  pre-estrazione. Un piano più grande dà più tempo, proporzionalmente alla
  sua taglia effettiva. DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile
  DEC-019): i due numeri (40, 6) restano da confermare col playtest — vedi
  `governance/open-questions.md`, voce 3.
- **Esito deciso una sola volta**: al primo ingresso (`WorldSpawnRoomContents`,
  `firstVisit`), si confronta il tempo trascorso dall'ingresso nel piano con
  la soglia sopra. Il risultato si scrive su `RoomState.rewardTaken` — stesso
  campo che tesoro/negozio usano per "il premio è già stato preso", qui
  riletto come "il premio di questa stanza è stato assegnato" — e resta fisso
  per il resto della run: rientrare più tardi non ricalcola né revoca
  l'esito.
- **Ricompensa**: `WorldAwardRoomCompletionCurrency(game, ROOM_TIMED)` — SOLO
  se raggiunta in tempo, mai altrimenti (coerente con
  [rewards-and-economy.md](./rewards-and-economy.md), "nessun bonus" oltre
  soglia significa anche nessuna valuta DEC-167 per quell'ingresso). Importo
  di default: 6 Ingots (proporzionato al rischio — deviare rotta in un piano
  avanzato per arrivarci in tempo — più di tesoro/negozio, meno del boss).
- **Oltre soglia: MAI bloccante**. La stanza a tempo non ha porte bloccate
  (`GameRoomIsLocked` non la considera mai: non è né `ROOM_COMBAT` né
  `ROOM_BOSS`) e resta attraversabile con o senza ricompensa, esattamente
  come il caso limite del documento richiede.
- **Segnale visivo prima di entrare (DEC-058)**: colore dedicato in
  `RoomMapColor` (ciano/acqua, distinto da ogni altro tipo) e icona `"!"` in
  `DrawRoomIcon` — stesso limite pre-visita di `ROOM_FUSION` (l'icona compare
  solo a stanza `visited`, vedi `known-issues.md` voce 12: non è una garanzia
  nuova, eredita quella già registrata per la fusione).
- **Indicazione leggibile dell'esito dentro la stanza (senza solo colore)**:
  un `Pickup` decorativo dedicato (`PICKUP_TIMED_MARKER`, mai consumato, mai
  un tasto — pura segnaletica) al centro della stanza, ri-materializzato a
  ogni ingresso come il crogiolo della fusione. Due canali NON-colore
  insieme: (1) un'etichetta testuale sempre disegnata — "IN TEMPO" o
  "SCADUTO" — indipendente dal caricamento dello sprite; (2) lo sprite
  dedicato (`assets/art/props/clessidra`, tag `attiva`/`scaduta`) o, se
  manca, una clessidra disegnata a forma geometrica nello stesso
  colore-per-stato. Il primo ingresso mostra anche un messaggio con i secondi
  esatti impiegati e la soglia — esempio **illustrativo** (non un caso
  osservato, vedi la fascia reale misurata sotto):
  `"raggiunta in tempo (38s/148s)"` / `"soglia scaduta (190s/148s)"` (148s è
  un valore che la formula `40 + 6×celle` produce davvero, con 18 celle); un
  rientro successivo mostra solo l'esito già fissato, senza numeri che
  confonderebbero (il tempo trascorso nel frattempo non è più quello
  rilevante). La soglia vera dipende dalla taglia del piano
  (`WorldTimedRoomThresholdSeconds` sotto): sui piani 3-5 dei semi di test
  varia in **148-178s** (celle vere 18-23), mai un valore fisso come 82s
  dell'esempio precedente di questa sezione (corretto il 30/07 dopo verifica
  sui semi reali; la fascia è salita da 136-172s / 16-22 celle dopo il WP6, che
  aggiunge al piano le 2-4 celle dell'arena di sfida — la formula non è
  cambiata, è cambiata la taglia vera dei piani).
- **Test**: `RoomsTestTimedRoomInteraction` dentro `GameRoomsTest`
  (`src/tests/game_tests.c`, `--rooms-test`/`make test`) — unicità, non
  adiacenza al boss, mai prima del piano 3 su 120 piani generati, più la
  catena intera per lo stesso piano/seme: raggiunta a `elapsed=0` → ricompensa
  assegnata + clessidra "in tempo", e raggiunta oltre soglia (calcolata con
  la stessa funzione del motore) → nessuna ricompensa + clessidra "scaduta" +
  stanza mai bloccata (`GameRoomIsLocked` sempre falso in entrambi i casi).

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Piani ammessi** | Dal piano 3 in su, stesso confine dell'escalation del tileset e dei boss a due fasi. | `WORLD_TIMED_ROOM_MIN_FLOOR`, `src/world/world.h` |
| **Frequenza per piano** | Un tentativo per piano, non garantito (stesso algoritmo di tesoro/negozio/fusione). | `WorldGenerateFloorMap`, `src/world/world.c` |
| **Taglia della stanza** | Sempre 1x1 (stesso default degli altri tre speciali 1x1). | `WorldPlaceSpecialRoom` |
| **Soglia di tempo** | `40s + 6s × taglia vera del piano in celle`, misurata dall'ingresso nel piano. | `WorldTimedRoomThresholdSeconds`, `src/world/world.c` |
| **Ricompensa entro soglia** | 6 Ingots (tra tesoro/negozio e boss). | `WORLD_ROOM_CURRENCY_TIMED`, `src/world/world.h` |
| **Ricompensa oltre soglia** | Nessuna (né il bonus né la valuta DEC-167 di base). | `WorldSpawnRoomContents`, `src/world/world.c` |

## Stato di implementazione: l'arena di sfida nel piano (WP6, 2026-07-30)

Terzo dei cinque archetipi speciali di questo documento ad avere un `RoomKind`
fisico nel motore (`ROOM_ARENA`, `src/core/game_types.h`), in coda dopo
`ROOM_FUSION` (WP4) e `ROOM_TIMED` (WP5). Copre **solo** la versione incontrata
durante il piano: l'accesso "best-of" dal Piano 0 (DEC-004, Scenario 2) è
esplicitamente fuori da questo lavoro e resta descritto in
[floor-zero.md](./floor-zero.md).

- **Piazzamento proprio, non `WorldPlaceSpecialRoom`**: l'arena è l'unica delle
  stanze speciali del motore a **non** passare dall'algoritmo delle 1x1
  (tesoro/negozio/fusione/a tempo, che restano quattro chiamanti). Ha
  `WorldPlaceArenaRoom` (`src/world/world.c`), che prova le taglie **grandi per
  prime** — 2x2, poi le quattro forme a L, poi 1x2/2x1 — e **non scende mai
  sotto le due celle**: l'arena è combattimento, e una 1x1 stretta la
  mortificherebbe (nessuno spazio per schivare un'ondata maggiorata). Meglio
  nessuna arena su quel piano che un'arena che non si può giocare.
- **Sempre foglia del grafo, come la stanza boss (DEC-182)**: ogni incastro
  deve toccare **esattamente una** stanza esistente, e mai la stanza boss (le
  darebbe una seconda porta). È la forma **strutturale** — verificabile con un
  test, non dichiarata a parole — del caso limite di questo documento: l'arena
  non è mai un passaggio obbligato e non blocca il piano se ignorata. Senza
  questa garanzia, accettare la sfida (che chiude le porte) potrebbe tagliare
  il piano in due. Anche le quattro speciali 1x1 non si attaccano mai
  all'arena, per lo stesso motivo per cui non si attaccano al boss.
- **Solo dai piani 2+** (`WORLD_ARENA_ROOM_MIN_FLOOR`, `src/world/world.h`), un
  tentativo per piano, non garantito. Piazzata **PRIMA** delle quattro speciali
  1x1: è l'unica che ha bisogno di celle libere **contigue**, e una griglia 5x5
  già cresciuta si frammenta in fretta. Misurato su 120 piani generati (5 piani
  × 24 semi, `--rooms-test`; solo i piani 2-5, **96 tentativi**, sono
  candidati): **piazzata in 82 casi su 96**, mai 1x1 (taglie osservate: L 43,
  1x2 20, 2x1 17, 2x2 2 — la 2x2 resta rara per lo stesso motivo per cui è rara
  per il boss, più perimetro significa più occasioni di toccare due stanze).
  L'ordine ha un prezzo dichiarato e misurato sulle 1x1 già esistenti: la
  stanza di fusione scende da 119/120 a **101/120** e la stanza a tempo da
  69/72 a **40/72** (con l'arena piazzata per ultima i due valori resterebbero
  intatti, ma l'arena stessa scenderebbe a 17/96 — cioè quasi non esisterebbe).
  Entrambe restano frequenti e nessuna delle due è necessaria a completare un
  piano.
- **L'opzionalità è il cuore dell'archetipo: la sfida NON parte entrando.**
  Dentro c'è un segnale interagibile — un `Pickup` di kind
  `PICKUP_ARENA_ALTAR`, ri-materializzato a ogni ingresso come il crogiolo
  della fusione e la clessidra della stanza a tempo — e finché il giocatore non
  **conferma esplicitamente** la stanza è attraversabile come una stanza vuota:
  nessun nemico, porte mai bloccate, nessuna valuta di completamento (DEC-167:
  attraversare non è completare).
- **Conferma esplicita, mai per inerzia**: a differenza del crogiolo della
  fusione, **toccare** il segnale non fa partire nulla — camminarci sopra non è
  una conferma, e la sfida è irreversibile. Serve il **tasto di interazione**
  (`X`, `Game.interactQueued` → `WorldTryStartArenaChallenge`) premuto **a
  contatto** con il segnale; premuto altrove non fa niente. Il tasto è un
  DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019) come `E`/`G`/`F`/`C`:
  nessun documento fissa quale sia, e non può essere "conferma" (ENTER/SPAZIO,
  perché SPAZIO in Gameplay è già la bomba). Suono: `ui_confirm`, quello già
  esistente, nessun evento sonoro nuovo.
- **Sfida accettata**: porte chiuse fino alla fine (`GameRoomIsLocked`, lo
  stesso pattern di `ROOM_COMBAT`/`ROOM_BOSS`, nessuna regola nuova), ondata a
  **budget +50%** e nemici portati alla **fascia alta della loro banda di
  potenza** — cioè dove [enemies.md](./enemies.md) colloca il Veterano, mai
  fuori dalla banda dichiarata (DEC-019). Tutto deterministico dal seed: due
  run con lo stesso seme trovano la stessa arena e la stessa ondata.
- **Abbandono e morte**: non esistono. Da sfida accettata si esce solo
  vincendo; morire dentro è una morte normale — **permadeath, nessun retry**
  (`PHASE_GAME_OVER` come ovunque). Non c'è codice dedicato: è la conseguenza
  di non averne aggiunto.
- **Vittoria**: `WorldCheckRoomClear` la tratta come qualunque altra stanza che
  si ripulisce, con la propria condizione di completamento (DEC-167) —
  "**sfida accettata e vinta**", mai "attraversata". Ricompensa su tre canali,
  tutti maggiorati rispetto a una stanza di combattimento equivalente
  ([rewards-and-economy.md](./rewards-and-economy.md), Scenario 2): **8 Ingots**
  (il doppio di un combattimento, sotto il boss), **l'oggetto di rarità
  migliore fra i tre candidati del piano** (rarità minima alzata: mai
  un'estrazione che possa pagare una comune quando nel pool c'è una rara) e un
  **catalizzatore di fusione al 50%** — che chiude la terza delle tre fonti di
  Flux dichiarate da DEC-022 ("drop di boss o di arene di sfida, oppure un
  acquisto costoso nel negozio"), prima del WP6 presente nel motore solo per
  due terzi.
- **Segnale visivo (DEC-058)**: colore dedicato in `RoomMapColor` (blu acceso,
  l'unica tinta francamente blu della tavola) e icona `"A"` in `DrawRoomIcon`.
  Stesso limite pre-ingresso di `ROOM_FUSION`/`ROOM_TIMED`, ereditato e non
  nuovo: l'icona compare solo a stanza `visited`, vedi
  `docs/engineering/known-issues.md` voce 12. **Dentro** la stanza, invece, i
  tre stati sono leggibili **senza dipendere dal colore**: il segnale porta
  sempre un'etichetta testuale — `"SFIDA"` / `"IN CORSO"` / `"SUPERATA"` —
  scritta anche quando lo sprite non carica, oltre alla forma dedicata (due
  lame incrociate su un basamento) e al colore di stato.
- **Test**: il controllo `(q)` di `GameRoomsTest` più
  `RoomsTestArenaInteraction` (`src/tests/game_tests.c`, `--rooms-test`/`make
  test`): al più una arena per piano, mai prima del piano 2, mai adiacente al
  boss, mai sotto le due celle, **sempre di grado 1 nel grafo**, e una BFS che
  ignora l'arena raggiunge comunque ogni altra stanza del piano; poi il ciclo
  di vita completo su un'arena vera — non accettata (attraversabile, si esce
  davvero, non si completa né paga), toccata (non parte), tasto premuto lontano
  (non parte), accettata (porte chiuse, budget davvero maggiorato rispetto a
  una stanza di combattimento della stessa taglia/piano con la stessa
  estrazione, nemici sopra il tipo base ma dentro la banda), vinta (valuta
  dell'arena, oggetto di rarità migliore, segnale "superata" anche rientrando)
  e la controprova che una stanza di combattimento ripulita **non** paga mai la
  valuta dell'arena né lascia il suo oggetto.

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Piani ammessi** | Dal piano 2 in su (il piano 1 resta il primo contatto col mondo generato). Confine diverso da quello della stanza a tempo di proposito: lì il "dai piani avanzati" è parte di DEC-051, qui è solo frequenza. | `WORLD_ARENA_ROOM_MIN_FLOOR`, `src/world/world.h` |
| **Frequenza per piano** | Un tentativo per piano, non garantito (82 casi su 96 piani candidati). | `WorldGenerateFloorMap`, `src/world/world.c` |
| **Taglia della stanza** | Mai 1x1: si provano 2x2 → L → 1x2/2x1, e se nessuna entra il piano resta senza arena. | `WorldPlaceArenaRoom`, `src/world/world.c` |
| **Struttura** | Sempre foglia del grafo di adiacenza (grado 1), come la stanza boss (DEC-182). | `WorldPlaceArenaRoom` |
| **Innesco** | Segnale interagibile al centro + conferma col tasto `X` a contatto; il solo tocco non basta. | `PICKUP_ARENA_ALTAR`, `WorldTryStartArenaChallenge` |
| **Budget nemici** | ×1.5 rispetto alla stessa stanza come combattimento (dopo la scala per celle di DEC-170, prima della riduzione per ostacoli di DEC-043). | `WORLD_ARENA_BUDGET_MULTIPLIER`, `src/world/world.h` |
| **Grado dei nemici** | Tipi portati alla fascia ALTA della banda di potenza dichiarata (dove enemies.md colloca il Veterano), mai fuori banda. Senza tipi generati (manifest vecchio) l'arena sale di sola quantità. | `WorldArenaGradeUpEnemyType`, `src/world/world.c` |
| **Valuta alla vittoria** | 8 Ingots — il doppio di un combattimento, meno del boss. | `WORLD_ROOM_CURRENCY_ARENA`, `src/world/world.h` |
| **Oggetto alla vittoria** | Il migliore per rarità fra i tre candidati del piano (rarità minima alzata), non un'estrazione pesata. | `WorldSpawnRoomReward`, `src/world/world.c` |
| **Catalizzatore di fusione** | 50% (più del 35% del boss: l'arena è un rischio scelto). Chiude la terza fonte di DEC-022. | `WORLD_ARENA_FLUX_DROP_PERCENT`, `src/world/world.h` |
| **Arredo della stanza** | Nessun ostacolo di layout (come boss/tesoro/negozio): l'arena resta uno spazio libero, perché un'ondata maggiorata dev'essere schivabile. Conseguenza: la riduzione di budget per ostacoli di DEC-043 non tocca mai l'arena. | `WorldBuildObstacles`, `src/world/world.c` |
| **Abbandono/retry** | Non esistono: si esce solo vincendo, morire dentro è permadeath come ovunque. | — |

## Regola di originalità

I nomi, la presentazione e la logica precisa dei quattro archetipi devono essere originali. Gli archetipi sono funzioni di design, non contenuti da copiare da giochi esistenti (vedi `09-originality-guardrails.md`).

## Regole per contenuti generati

Ogni istanza di stanza speciale generata dichiara un'origine (curato | composto | variato | nuovo) e rispetta il contratto: accesso, costo, ricompensa, frequenza, segnale visivo, uscita, interazioni con risorse.

Per lo scambio ad alto rischio, la coppia offerta/prezzo generata (DEC-044) deve restare
dentro il budget di equità dichiarato: un prezzo sproporzionato rispetto all'offerta, o
un'offerta priva di un prezzo coerente, non supera la validazione e non viene mai proposta al
giocatore.

## Casi limite

- Una stanza di fusione generata senza almeno due oggetti fondibili posseduti dal giocatore: resta accessibile ma senza azione disponibile finché non sono soddisfatti i requisiti.
- Uno scambio ad alto rischio proposto quando il giocatore non ha nulla di cedibile: la stanza non deve bloccare il progresso, deve offrire un'uscita senza penalità.
- Una puntata generata (offerta/prezzo, DEC-044) risulta squilibrata rispetto al budget di equità: va respinta o rigenerata in validazione prima di essere proposta al giocatore.
- Il prezzo generato richiederebbe più salute massima di quella posseduta dal giocatore: il prezzo non deve mai superare risorse che il giocatore non ha, la generazione deve restare compatibile con lo stato corrente del giocatore.
- L'arena di sfida "best-of" nel Piano 0 richiede contenuti già validati che potrebbero non esistere ancora nelle prime run: va gestita con il fallback previsto in [floor-zero.md](./floor-zero.md).
- L'arena di sfida incontrata nel piano non deve **mai** essere un passaggio obbligato né bloccare il piano se il giocatore la ignora. **Stato (WP6, 30/07):** garantito per costruzione — l'arena è sempre una foglia del grafo di adiacenza (grado 1, come la stanza boss di DEC-182) e, finché la sfida non è accettata, è attraversabile come una stanza vuota; verificato dal controllo `(q)` di `GameRoomsTest` con una BFS che ignora l'arena e raggiunge comunque ogni altra stanza del piano.
- Il giocatore accetta la sfida dell'arena e non riesce a vincerla: non esiste abbandono né retry — si esce solo vincendo, e morire dentro è una morte normale della run (permadeath). È la conseguenza voluta di "rischio dichiarato in cambio di un guadagno superiore" (DEC-010): il rischio deve essere reale.
- Il giocatore raggiunge una stanza a tempo dopo la scadenza della soglia: non deve mai bloccare il progresso del piano; resta almeno accessibile come stanza ordinaria, anche senza il bonus a tempo (soglia esatta e comportamento di mancato rispetto da definire col playtest, vedi `governance/open-questions.md`).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni archetipo speciale ha un contenuto curato sufficiente a comparire senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non ridefinisce la tassonomia completa dei tipi di stanza (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).
- Non dettaglia la meccanica di fusione (vedi [item-fusion.md](./item-fusion.md)).
- Non risponde alla domanda su come si scoprono le stanze segrete (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)).
- Non dettaglia l'accesso "best-of" dal Piano 0 (vedi [floor-zero.md](./floor-zero.md)).

## Domande aperte residue

- ~~Nome e presentazione definitivi dello scambio ad alto rischio~~: risolto da DEC-136 — **Pourhouse** («Casa della Colata»), presentazione canonica nel glossario.
- Frequenza esatta di ciascun archetipo per piano. **Aggiornamento 30/07 (WP4/WP5):**
  per la stanza di fusione e per la stanza a tempo esiste ora un default
  proposto e implementato — un tentativo per piano, la stanza a tempo solo
  dal piano 3 — vedi "Stato di implementazione" sopra e
  `governance/open-questions.md` voci 30 e 32. **Aggiornamento 30/07 (WP6):**
  anche l'arena di sfida incontrata nel piano ha ora un default proposto e
  implementato — un tentativo per piano dal piano 2, vedi
  `governance/open-questions.md` voce 37; resta aperta per i due archetipi non
  ancora nel motore (segreta, scambio) e per l'accesso "best-of" dell'arena dal
  Piano 0.
- Valori esatti di soglia e ricompensa della stanza a tempo. **Aggiornamento
  30/07 (WP5):** esiste ora un default proposto e implementato — soglia
  `40s + 6s × celle del piano` dall'ingresso nel piano, ricompensa 6 Ingots
  solo entro soglia — vedi "Stato di implementazione" sopra,
  [rewards-and-economy.md](./rewards-and-economy.md) e
  `governance/open-questions.md` voce 3.
- Valori numerici esatti del budget di equità della puntata generata (DEC-044 fissa il principio, non i numeri).
- ~~Quali rivelatori esistono per le super-segrete~~: risolto da DEC-127 (Innesti sensore + oggetti rari), vedi `secrets-and-obstacles.md`.

## Scenari

### Scenario 1 — Negozio non è un archetipo speciale

Given un giocatore che entra in un negozio durante un piano
When cerca questa stanza nella tassonomia
Then la trova descritta come tipo standard in [rooms-and-floor-generation.md](./rooms-and-floor-generation.md), non tra i quattro archetipi speciali di questo documento

### Scenario 2 — Accesso "best-of" all'arena di sfida dal Piano 0

Given un giocatore nel Piano 0 con contenuti già validati disponibili dal museo delle creazioni
When sceglie di affrontare un'arena di sfida "best-of"
Then accede a un'arena costruita con contenuti "best-of" di run passate, distinta dall'arena di sfida incontrata durante un piano generato

### Scenario 3 — Scambio ad alto rischio senza nulla da cedere

Given un giocatore senza risorse o oggetti cedibili che entra in una stanza di scambio ad alto rischio
When valuta le opzioni disponibili
Then la stanza offre comunque un'uscita senza penalità, senza bloccare il progresso

### Scenario 4 — Stanza di fusione senza requisiti soddisfatti

Given un giocatore con un solo oggetto fondibile
When entra nella stanza di fusione
Then la stanza è visitabile ma l'azione di fusione resta non disponibile finché non possiede almeno due oggetti fondibili e il catalizzatore richiesto

### Scenario 5 — Negozio senza patti a costo salute

Given un giocatore in un negozio con l'offerta speciale generata per quel negozio
When osserva le opzioni di acquisto disponibili
Then nessuna di esse chiede di cedere salute in cambio di uno sconto o di un oggetto: quel tipo di patto esiste solo nella stanza di scambio ad alto rischio (DEC-026)

### Scenario 6 — Stanza segreta a due livelli

Given un giocatore che esplora un piano con una stanza segreta "normale" e una "super-segreta"
When cerca di individuarle
Then trova la "normale" tramite un indizio visivo leggibile apribile con lo strumento di breccia, mentre individua la "super-segreta" solo con un oggetto/Innesto rivelatore o intuizione estrema (DEC-025)

### Scenario 7 — Puntata generata in una stanza di scambio ad alto rischio

Given un giocatore che entra in una stanza di scambio ad alto rischio
When il gioco genera l'offerta e il prezzo della puntata
Then il prezzo appartiene a una delle categorie ammesse (salute immediata o massima, oggetto o Innesto posseduto, valuta principale, catalizzatore di fusione) ed è proporzionato all'offerta dentro il budget di equità (DEC-044)

### Scenario 8 — Ogni scambio è diverso

Given un giocatore che entra in due stanze di scambio ad alto rischio diverse nella stessa run
When confronta le due puntate proposte
Then offerta e prezzo delle due stanze sono diversi tra loro, perché ogni scambio è generato per l'occasione (DEC-044)

### Scenario 10 — Arena di sfida ignorata

Given un piano generato che contiene un'arena di sfida (`ROOM_ARENA`)
When il giocatore la attraversa senza confermare la sfida, o non ci entra affatto
Then la stanza non ha nemici e non blocca mai le porte, nessuna ricompensa e nessuna valuta di completamento vengono assegnate, e ogni altra stanza del piano resta raggiungibile senza passare da lì (l'arena è sempre una foglia del grafo)

### Scenario 11 — Arena di sfida accettata e superata

Given un giocatore che, dentro un'arena di sfida, si porta sul segnale e preme il tasto di interazione
When la sfida parte
Then le porte restano chiuse fino alla fine, i nemici arrivano con un budget maggiorato e in fascia alta della loro banda di potenza, e alla vittoria il giocatore riceve una ricompensa superiore a quella di una stanza di combattimento equivalente (valuta doppia, l'oggetto di rarità migliore del piano e una probabilità di catalizzatore di fusione, DEC-022)

### Scenario 9 — Stanza a tempo raggiunta in tempo

Given un giocatore che raggiunge una stanza a tempo entro la soglia richiesta in un piano avanzato
When entra nella stanza
Then riceve la ricompensa aggiuntiva descritta in [rewards-and-economy.md](./rewards-and-economy.md), coerente col timer di run sempre visibile nell'HUD (DEC-051)
