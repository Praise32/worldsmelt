---
id: gd-system-special-rooms
title: Special Rooms
domain: design
status: approved
authority: canonical
owner: design
summary: "Dettaglio dei cinque archetipi speciali (DEC-010, esteso da DEC-051): fusione, segreta a due livelli (DEC-025), arena di sfida, scambio ad alto rischio — in-game Pourhouse (DEC-136), unico luogo per patti a costo salute (DEC-026), con offerta e prezzo generati dentro un budget di equità (DEC-044) — e stanza a tempo nei piani avanzati (DEC-051) — sottoinsieme dichiarato della tassonomia di rooms-and-floor-generation.md. Dal WP8 (30/07) TUTTI E CINQUE gli archetipi hanno un RoomKind fisico nel motore: la stanza segreta (ROOM_SECRET) è l'ultima ad arrivare, dopo fusione (WP4), stanza a tempo (WP5), arena di sfida (WP6) e Pourhouse (WP7)."
last_reviewed: 2026-07-30
last_verified_commit: 63753fc
topics: [stanze-speciali, WP15a, arena-best-of, Piano-0, DEC-004, fusione, scambio-alto-rischio, pourhouse, arena-di-sfida, stanza-a-tempo, stanza-segreta, WP4, WP5, WP6, WP7, WP8, ROOM_FUSION, ROOM_TIMED, ROOM_ARENA, ROOM_POURHOUSE, ROOM_SECRET, DEC-044, DEC-136, DEC-025]
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

**Stato (WP8, 30/07):** ha ora un `RoomKind` fisico nel motore (`ROOM_SECRET`), ed è il
**QUINTO e ultimo** dei cinque archetipi di questo documento ad averlo — da qui in poi
nessun archetipo speciale resta fuori dal motore. Vedi "Stato di implementazione: la
stanza segreta" sotto.

### Arena di sfida

Stanza opzionale con combattimento più impegnativo in cambio di ricompensa maggiore. È anche accessibile in versione "best-of" dal Piano 0, usando contenuti già validati delle run passate (DEC-004): vedi [floor-zero.md](./floor-zero.md) per il dettaglio di questo accesso alternativo; questo documento descrive solo la versione incontrata durante il piano.

**Stato (WP6, 30/07):** la versione **incontrata nel piano** ha un `RoomKind`
fisico nel motore (`ROOM_ARENA`) — vedi "Stato di implementazione: l'arena di sfida
nel piano" sotto.

**Stato (WP15a, 30/07):** anche l'accesso **"best-of" dal Piano 0** esiste ora nel motore,
ed è una cosa **diversa** dalla stanza del piano: non un `RoomKind`, ma una *simulazione*
dentro l'unica cella del crogiolo, a rischio zero e senza economia (DEC-092/093), aperta da
piazzole segnalate. Fonte unica del dettaglio:
[floor-zero.md](./floor-zero.md), "Stato di implementazione: le arene di sfida del Piano 0";
qui si registra solo che lo **Scenario 2** di questo documento non è più un'aspirazione.
Resta fuori la prova dal **museo** (DEC-040, riaffrontare un boss esposto): il museo non
esiste ancora nel motore, e un boss del catalogo entra nella simulazione come nemico
normale.

### Scambio ad alto rischio

Stanza che propone uno scambio non convenzionale (es. cedere una risorsa, salute o una parte della build per un guadagno maggiore ma incerto), ri-tematizzata in modo originale. È l'**unico** archetipo dove sono ammessi scambi a costo salute (DEC-026): il negozio non li offre mai. Nome e presentazione sono fissati da DEC-136 — in-game **Pourhouse**, «Casa della Colata», vedi il [glossario](../governance/glossary.md) — e la funzione, rischio dichiarato in cambio di un guadagno superiore alla media, è fissata da DEC-010.

**Stato (WP7, 30/07):** ha ora un `RoomKind` fisico nel motore (`ROOM_POURHOUSE`), con la
puntata di DEC-044 composta deterministicamente dal seed dentro un budget di equità
dichiarato — vedi "Stato di implementazione: la Pourhouse" sotto.

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

## Stato di implementazione: la Pourhouse (WP7, 2026-07-30)

**Quarto** dei cinque archetipi speciali di questo documento ad avere un `RoomKind`
fisico nel motore (`ROOM_POURHOUSE`, `src/core/game_types.h`), in coda dopo `ROOM_FUSION`
(WP4), `ROOM_TIMED` (WP5) e `ROOM_ARENA` (WP6). Resta fuori dal motore solo la **stanza
segreta**, che dipende dai rivelatori di DEC-127 e da
[secrets-and-obstacles.md](./secrets-and-obstacles.md).

- **Piazzamento: la QUINTA chiamante di `WorldPlaceSpecialRoom`** (`src/world/world.c`) —
  stesso algoritmo di tesoro/negozio/fusione/stanza a tempo: taglia 1x1, mai adiacente a
  una stanza che deve restare foglia (boss, DEC-182, e arena di sfida), deterministica dal
  seed del piano. Una differenza sola, ed è di design: **non si tenta a ogni piano**. La
  Pourhouse è un archetipo raro, non un servizio di piano come il negozio — dal piano 2 in
  su, e solo quando l'estrazione del piano lo concede
  (`WORLD_POURHOUSE_ROOM_CHANCE_PERCENT` = 70%). Si piazza **per ultima**, dopo ogni altra
  stanza: è ciò che permette alla sua estrazione di non spostare il flusso RNG di nessun
  altro piazzamento — i numeri misurati al WP6 per fusione (101/120), stanza a tempo
  (40/72) e arena (82/96) sono rimasti **identici**. Prezzo di quella scelta: la Pourhouse
  eredita la griglia più satura di tutte, quindi il 70% è la probabilità del *tentativo*,
  non del risultato — con l'estrazione forzata al 100% troverebbe posto nel 44% dei piani
  candidati. **Misurato su 120 piani generati** (5 piani × 24 semi, `--rooms-test`; solo i
  piani 2-5, 96 candidati): **piazzata in 27 casi su 96**, cioè circa un piano candidato su
  quattro, e circa il 73% delle run ne incontra almeno una.
- **La puntata si COMPONE, non si genera a runtime (DEC-044 + DEC-171).** Nella demo
  nessun modello gira mentre si gioca, quindi «l'IA genera sia l'offerta sia il prezzo» si
  realizza come **composizione deterministica** dal seed di run + piano + cella —  la
  stessa disciplina già usata per `FusionKey` e per le sinergie (DEC-161).
  `WorldComposePourhouseWager` (`src/world/pourhouse.c`) enumera le 55 coppie candidate
  (11 offerte × 5 categorie di prezzo) in un ordine derivato dal seed e restituisce la
  prima che è insieme **equa** e **pagabile**. Il modulo è nuovo e separato da `world.c`
  proprio perché la puntata è un sistema a sé: composizione, validazione, testi e
  applicazione atomica.
- **Il budget di equità è una tabella di valori equivalenti**, ancorata alla valuta
  principale (1 Ingot = 1 punto) e ai prezzi che il negozio già pratica: fonte unica della
  tabella e del suo perché in
  [rewards-and-economy.md](./rewards-and-economy.md#tabella-di-equivalenza-della-pourhouse-dec-044),
  non ripetuta qui. La regola è
  `|valore(offerta) − valore(prezzo)| ≤ max(4 punti, 20% dell'offerta)`: una coppia fuori
  tolleranza è **respinta e non viene mai proposta** al giocatore, esattamente come chiede
  la sezione «Regole per contenuti generati» sopra.
- **Categorie ammesse, nessuna in più.** Prezzo: salute immediata, salute **massima** (il
  tetto), un oggetto/Innesto posseduto, valuta principale, catalizzatore di fusione — le
  cinque di DEC-044. Offerta: valuta abbondante, strumenti di breccia/apertura, Crust,
  l'oggetto di rarità migliore fra i tre candidati del piano, Flux. Una puntata non baratta
  mai una risorsa **con sé stessa** (Ingots per Ingots, Flux per Flux, un oggetto per un
  oggetto di pari valore): sarebbe un giro a vuoto, non uno scambio.
- **Validazione contro lo stato del giocatore, sempre.** Il prezzo non può chiedere
  risorse che il giocatore non ha: mai più Ingots/Flux di quelli in tasca, mai un oggetto
  non posseduto, mai un prezzo in salute che lo ucciderebbe, e — caso limite esplicito del
  documento — **mai più salute massima di quella posseduta**, con il tetto che non scende
  comunque mai sotto **un cuore** (`POURHOUSE_MIN_BASE_MAX_HP`, 2 punti vita). Un'offerta
  che non potrebbe essere consegnata per intero (Crust oltre il proprio tetto, un oggetto
  con l'inventario pieno) viene scartata già in composizione: il banco non promette mai
  ciò che il motore non può mantenere.
- **Scenario 3 — niente da cedere: uscita libera.** Se nessuna delle 55 coppie è insieme
  equa e pagabile, la puntata resta `valid = false`: il banco lo **dice per esteso** («la
  colata è fredda»), la stanza non blocca mai le porte (`GameRoomIsLocked` non la considera
  mai) e il tasto di conferma non fa nulla. Non è un fallback né un errore: è uno stato
  previsto del contratto. Finché la puntata resta fredda, un ritorno successivo la
  ricompone — un giocatore che torna con qualcosa da versare trova il banco acceso.
- **Scenario 8 — due Pourhouse, due puntate diverse.** `Game.pourhouseLastSignature`
  ricorda la firma dell'ultima puntata composta nella **run** (non nel piano): la
  composizione fa una prima passata che scarta le coppie con quella firma e ricade sulla
  seconda solo se non esiste alternativa valida — così due Pourhouse successive propongono
  puntate diverse quando è possibile, senza mai lasciare muto il banco di chi possiede una
  cosa sola.
- **Flusso: leggere prima, confermare poi (DEC-058).** Dentro la stanza c'è un banco
  (`PICKUP_POURHOUSE_BANK`, ri-materializzato a ogni ingresso come il crogiolo della
  fusione) che scrive **offerta e prezzo per esteso su due righe** — `DAI: …` /
  `PRENDI: …` — sempre, anche quando lo sprite non carica; il testo è la fonte, non il
  colore. Accettare richiede il **tasto di interazione `X` a contatto** col banco, lo
  stesso pattern dell'arena (`Game.interactQueued` → `WorldTryAcceptPourhouseWager`):
  toccare il banco non accetta nulla, premere il tasto altrove nemmeno. Suoni: `ui_confirm`
  quando la colata si versa, `ui_cancel` quando la puntata non è più pagabile — entrambi
  già esistenti, nessun evento sonoro nuovo.
- **Applicazione ATOMICA.** All'accettazione si ricontrolla **tutto** prima di toccare
  qualunque cosa: prezzo ancora pagabile *e* offerta ancora consegnabile. Se una delle due
  cade (il giocatore ha speso il prezzo altrove, ha riempito l'inventario, ha sganciato
  l'oggetto richiesto) non si conclude nulla e **non si paga nulla** — mai mezza puntata.
  Un fallimento qui non è una penalità: il banco lo dice e la stanza resta com'era.
- **Rifiuto = uscire.** Non esiste un tasto «rifiuta»: si esce dalla porta, senza alcun
  costo, e la puntata **resta la stessa** se si torna. È un default proposto
  dall'implementazione (vedi la tavola sotto): bruciare l'occasione per aver esitato
  punirebbe l'esplorazione, e la conferma esplicita di DEC-058 serve a proteggere
  dall'azione irreversibile, non a trasformare l'indecisione in una.
- **Salute massima: il tetto vero.** Il prezzo in salute massima scrive su
  `Player.baseMaxHp` (`CombatReducePlayerMaxHp`, `src/gameplay/combat.c`) e forza subito il
  ricalcolo delle statistiche: `maxHp` è un valore *derivato* che il sistema delle cache
  ricalcola da zero a ogni passaggio, quindi ridurre solo quello sarebbe stato annullato al
  primo ricalcolo — il prezzo più rischioso dell'archetipo sarebbe risultato gratis. La
  riduzione è **permanente per la run** e verificata come tale dal test.
- **Il Crust non paga mai un prezzo di salute (DEC-008).** Un prezzo in salute immediata
  tocca solo `Player.hp`, mai `Player.tempHp`: la salute temporanea/protettiva è
  protezione, non valuta, e l'ordine di consumo di
  [health-and-resources.md](./health-and-resources.md) vale per il **danno subito**, non
  per un patto volontario. Il documento non lo diceva esplicitamente perché non prevedeva
  un modo di «spendere» salute: registrato qui come default proposto e verificato dal test.
- **Nessuna valuta di completamento (DEC-167).** La Pourhouse non ha una condizione di
  «ripulita»: accettare o rifiutare è uno **scambio**, non un completamento, e il guadagno
  è l'offerta stessa — già pagata col prezzo. Stessa scelta esplicita già fatta per la
  stanza di fusione.
- **Segnale visivo (DEC-058)**: colore dedicato in `RoomMapColor` (magenta caldo, l'unica
  tinta rosa della tavola) e icona `"P"` in `DrawRoomIcon`. Stesso limite pre-ingresso di
  `ROOM_FUSION`/`ROOM_TIMED`/`ROOM_ARENA`, ereditato e non nuovo (`known-issues.md`, voce
  12): l'icona compare solo a stanza `visited`. **Dentro** la stanza il colore non porta
  mai da solo l'informazione: l'etichetta del banco (`PUNTATA` / `VERSATA` / `FREDDA`) e le
  due righe del contratto sono sempre scritte.
- **Sprite**: nessun asset nuovo richiesto. Il banco riusa `assets/art/props/piedistallo`
  (vocabolario `vuoto`/`pieno`) e, se manca, ripiega su una forma geometrica dedicata —
  un crogiolo rovesciato che cola, silhouette non usata da nessun'altra raccolta. Un prop
  dedicato sarebbe un miglioramento, non un requisito: registrato in
  `docs/engineering/known-issues.md`.
- **Test**: il controllo `(r)` di `GameRoomsTest` più `RoomsTestPourhouseInteraction`
  (`src/tests/game_tests.c`, `--rooms-test`/`make test`): al più una Pourhouse per piano,
  sempre 1x1, mai prima del piano 2, mai adiacente a boss o arena, e — controllo che
  fallirebbe se qualcuno rendesse il tentativo incondizionato — **mai su tutti** i piani
  candidati; poi la puntata vera e propria: composizione deterministica (stesso seed e
  stesso stato → stessa puntata; 19 puntate composte, 15 distinte fra i semi; due Pourhouse
  della stessa run → firme diverse), prezzo dentro le risorse possedute su sei stati
  diversi del giocatore (compreso quello senza nulla → uscita libera e porte mai bloccate),
  budget di equità rispettato, accettazione atomica (valuta↔Crust, oggetto↔valuta) con le
  due prove di fallimento a metà (inventario pieno; risorsa sparita fra composizione e
  conferma), rifiuto senza penalità con la stessa puntata al ritorno, tetto mai sotto un
  cuore neanche con una richiesta assurda, e Crust mai intaccato da un prezzo di salute.

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Piani ammessi** | Dal piano 2 in su, come l'arena: il piano 1 resta il primo contatto col mondo generato, e prima di possedere qualcosa la risposta della Pourhouse sarebbe quasi sempre «la colata è fredda». | `WORLD_POURHOUSE_ROOM_MIN_FLOOR`, `src/world/world.h` |
| **Frequenza per piano** | **Non ogni piano**: un tentativo solo quando l'estrazione del piano lo concede (70%), e comunque non garantito. Misurato: 27 piani su 96 candidati. | `WORLD_POURHOUSE_ROOM_CHANCE_PERCENT`, `WorldGenerateFloorMap` |
| **Taglia della stanza** | Sempre 1x1, come le altre quattro speciali di `WorldPlaceSpecialRoom`: leggere una puntata e decidere non ha bisogno di spazio. | `WorldPlaceSpecialRoom` |
| **Ordine di piazzamento** | Per ultima, dopo ogni altra stanza: così la sua estrazione non sposta il flusso RNG degli altri piazzamenti e le misure del WP6 restano valide. | `WorldGenerateFloorMap` |
| **Budget di equità** | `\|offerta − prezzo\| ≤ max(4 punti, 20% dell'offerta)`, su una tabella di valori equivalenti ancorata a 1 Ingot = 1 punto. | `POURHOUSE_EQUITY_TOLERANCE_*`, `src/world/pourhouse.h`; tabella in [rewards-and-economy.md](./rewards-and-economy.md) |
| **Baratto con sé stessa** | Vietato: mai Ingots per Ingots, Flux per Flux, oggetto per oggetto di pari valore. | `PourhouseSameResource`, `src/world/pourhouse.c` |
| **Tetto minimo di salute** | Il prezzo in salute massima non porta mai il tetto sotto **un cuore** (2 punti vita). | `POURHOUSE_MIN_BASE_MAX_HP` |
| **Crust e prezzi di salute** | Il Crust non paga **mai** un prezzo di salute (DEC-008: è protezione, non valuta). L'ordine di consumo vale per il danno subito, non per un patto volontario. | `WorldTryAcceptPourhouseWager` |
| **Rifiuto** | Uscire dalla stanza. Nessun tasto dedicato, nessuna penalità, e la puntata **resta disponibile** per un ritorno successivo: si consuma solo accettandola. | `WorldPourhousePrepareRoom` |
| **Ricomposizione** | Una puntata valida non si ri-tira mai (una sola puntata per stanza per run). Una puntata **fredda** invece sì, a ogni ingresso: se il giocatore torna con qualcosa da versare, il banco si accende. | `WorldPourhousePrepareRoom` |
| **Puntate diverse nella stessa run** | La composizione scarta, in prima passata, la firma dell'ultima puntata della run; ricade su di essa solo se non esiste alternativa valida. | `Game.pourhouseLastSignature` |
| **Tasto di conferma** | `X` a contatto col banco, lo stesso dell'arena: per il giocatore è un solo gesto — «accetto ciò che questa stanza propone». | `Game.interactQueued`, `WorldTryAcceptPourhouseWager` |
| **Valuta di completamento** | Nessuna: uno scambio non è un completamento (DEC-167). | `WorldAwardRoomCompletionCurrency` |

## Stato di implementazione: la stanza segreta (WP8, 2026-07-30)

**QUINTO e ultimo** archetipo di questo documento ad avere un `RoomKind` fisico nel motore
(`ROOM_SECRET`, `src/core/game_types.h`), a **due livelli** (`RoomState.secretSuper`,
DEC-025). Il dettaglio dei **segreti** — indizio, strumento di breccia, geometria della
parete, super-segreta senza indizi — vive in
[secrets-and-obstacles.md](./secrets-and-obstacles.md), "Stato di implementazione: i due
livelli nel motore"; qui c'è solo ciò che riguarda l'archetipo come **stanza del piano**.

- **Piazzamento EXTRA, mai sostitutivo**: `WorldPlaceSecretRoom` (`src/world/world.c`) —
  una cella 1x1 **in più** su una cella **libera** della griglia, mai al posto di una
  stanza già piazzata. La cella deve toccare **esattamente UNA** cella esistente (un solo
  muro condiviso, quindi un solo indizio e un solo varco) e quella cella deve appartenere
  a una stanza **normale**: partenza o combattimento. Mai boss, mai arena — devono restare
  foglie (DEC-182 e "Casi limite" di questo documento) — e mai un'altra speciale: un
  segreto dietro la parete di una stanza tesoro avrebbe spostato il suo costo su una
  chiave, che con lo strumento di breccia non c'entra nulla.
- **La segreta è foglia per natura**: 1x1 con una sola vicina, e `WorldShapeTouchesLeafRoom`
  impedisce a chiunque venga dopo di attaccarsi al suo secondo lato — altrimenti si
  entrerebbe da una porta normale, senza sbrecciare niente.
- **Fuori dalla connettività del piano**: finché il varco è murato la stanza non ha
  **nessuna** porta (`WorldLinkRooms` non ne apre per una segreta sigillata), quindi il
  piano resta completabile ignorando i segreti del tutto. Il controllo (j) di
  `GameRoomsTest` misura infatti la connettività **senza contarle**.
- **Non compare sulla minimappa** finché non è aperta — non indicata "direttamente sulla
  mappa" come chiede questo documento, e nemmeno smorzata come una cella nota-ma-non
  visitata (che sarebbe già un indizio, e l'indizio per DEC-025 sta sulla **parete**).
  Il filtro è un predicato unico e puro, `WorldRoomHiddenOnMap` (`src/world/world.h`),
  applicato da `DrawMinimap`: dopo la breccia torna falso e la stanza si comporta come
  ogni altra, con colore dedicato in `RoomMapColor` (ottone) e icona `"S"` in
  `DrawRoomIcon` alla prima visita. È l'unico archetipo di questo documento **senza** il
  limite DEC-058 "prima di entrare" di `known-issues.md` voce 12 — non perché sia stato
  risolto, ma perché qui *non far vedere niente prima* è proprio il design.
- **Ordine di piazzamento e prezzo dichiarato**: le due segrete si piazzano dopo boss,
  arena, tesoro e negozio, e **prima** di fusione/stanza a tempo/Pourhouse. La segreta ha
  un vincolo di posizione molto più stretto delle 1x1 (le serve una cella con una sola
  vicina): piazzata per ultima trovava posto in 23 piani su 120 e la **super-segreta in 0
  su 96**, cioè un intero livello di DEC-025 non sarebbe mai esistito nel gioco vero.
  Il prezzo, misurato su `--rooms-test`: stanza di fusione da 101/120 a **95/120**, stanza
  a tempo da 40/72 a **30/72**, Pourhouse da 27/96 a **17/96**. Nessuna delle tre è
  necessaria a completare un piano. Stessa forma di compromesso già dichiarata dal WP6 per
  l'arena.
- **Condizione di completamento propria** (DEC-167): "trovata", cioè il **primo ingresso**
  — il varco era già stato aperto un istante prima, e aprirlo *è* il lavoro di questo
  archetipo. Le porte non si bloccano mai (`GameRoomIsLocked` non elenca `ROOM_SECRET`).
- **Test**: `RoomsTestSecretRooms` e il controllo (s) di `GameRoomsTest`
  (`src/tests/game_tests.c`, `--rooms-test`/`make test`) — su 120 piani generati:
  sempre 1x1, al più una per livello, sempre murata in generazione, sempre con una sola
  parete condivisa verso una stanza normale, mai sulla minimappa prima della breccia; più
  il ciclo di vita completo su una segreta vera (bomba lontana o di origine nemica →
  nessun varco; bomba sulla parete → varco su entrambi i lati; uscita e rientro → varco
  ancora aperto e valuta pagata una volta sola; oggetto sempre lo stesso).

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Frequenza, livello normale** | Un tentativo per piano dal piano 1, non garantito. Misurata: 36 piani su 120. | `WORLD_SECRET_ROOM_MIN_FLOOR`, `WorldGenerateFloorMap` |
| **Frequenza, super-segreta** | Dal piano 2 e **non a ogni piano**: tentativo solo a estrazione concessa (50%). Misurata: 13 su 96 candidati — più rara della normale, come DEC-025 richiede. | `WORLD_SECRET_SUPER_MIN_FLOOR`, `WORLD_SECRET_SUPER_CHANCE_PERCENT` |
| **Taglia della stanza** | Sempre 1x1, come tesoro/negozio/fusione/a tempo/Pourhouse: dentro c'è una ricompensa da raccogliere, non uno spazio da giocare. | `WorldPlaceSecretRoom` |
| **Vicina ammessa** | Solo partenza o combattimento. Le altre speciali sono escluse per leggibilità del costo, boss e arena perché devono restare foglie. | `WorldPlaceSecretRoom` |
| **Ricompensa** | L'oggetto di rarità migliore fra i tre del piano, **senza estrazione** (identico a ogni rientro), più 6 Ingots di "segreta trovata". La super-segreta aggiunge 1 catalizzatore di fusione versato subito. | `WorldSpawnRoomContents`, `WORLD_ROOM_CURRENCY_SECRET`, `WORLD_SECRET_SUPER_FLUX` |
| **Valuta di completamento** | 6 Ingots, uguali per i due livelli: la superiorità del livello 2 è il catalizzatore, non più valuta. | `WorldAwardRoomCompletionCurrency` |

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
- Uno scambio ad alto rischio proposto quando il giocatore non ha nulla di cedibile: la stanza non deve bloccare il progresso, deve offrire un'uscita senza penalità. **Stato (WP7, 30/07):** garantito — se nessuna delle coppie candidate è insieme equa e pagabile la puntata resta non valida, il banco lo dichiara per esteso, le porte non si bloccano mai e il tasto di conferma non fa nulla; una puntata «fredda» si ricompone a ogni ritorno, così chi torna con qualcosa da versare la trova accesa.
- Una puntata generata (offerta/prezzo, DEC-044) risulta squilibrata rispetto al budget di equità: va respinta o rigenerata in validazione prima di essere proposta al giocatore. **Stato (WP7, 30/07):** una coppia fuori tolleranza non viene mai proposta — la composizione scarta e prova la successiva fra le 55 candidate, e il test ricalcola il budget dalle stesse costanti per accorgersi di una tolleranza allargata di nascosto.
- Il prezzo generato richiederebbe più salute massima di quella posseduta dal giocatore: il prezzo non deve mai superare risorse che il giocatore non ha, la generazione deve restare compatibile con lo stato corrente del giocatore. **Stato (WP7, 30/07):** garantito per costruzione — la composizione valida ogni coppia contro lo stato del giocatore in quel momento, il tetto non scende mai sotto un cuore, e l'accettazione ricontrolla tutto una seconda volta (prezzo ancora pagabile *e* offerta ancora consegnabile) prima di toccare qualunque cosa: mai mezza puntata.
- Il giocatore accetta una puntata la cui offerta non entra più nell'inventario, o il cui prezzo non possiede più: non si conclude nulla e non si paga nulla. Rifiutare — cioè uscire dalla stanza — non costa mai niente e non consuma la puntata, che resta la stessa per un ritorno successivo.
- L'arena di sfida "best-of" nel Piano 0 richiede contenuti già validati che potrebbero non esistere ancora nelle prime run: va gestita con il fallback previsto in [floor-zero.md](./floor-zero.md). **Stato (WP15a, 30/07):** garantito e verificato — senza alcuna run passata registrata l'arena si semina dal contenuto curato già caricato e, in ultima istanza, dai tipi d'esempio del motore; il test `--arena-hub-test` esercita proprio il caso "catalogo vuoto" e pretende che la simulazione abbia comunque dei nemici.
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
  `governance/open-questions.md` voce 37. **Aggiornamento 30/07 (WP7):** anche
  lo scambio ad alto rischio (Pourhouse) ha ora un default proposto e
  implementato — dal piano 2 e **non a ogni piano**: un tentativo solo quando
  l'estrazione del piano lo concede (70%), misurato in 27 piani su 96
  candidati, sempre 1x1 — vedi `governance/open-questions.md` voce 41.
  **Aggiornamento 30/07 (WP8):** anche la stanza segreta ha ora un default
  proposto e implementato, diverso per i due livelli di DEC-025 — la
  **normale** si tenta a ogni piano dal piano 1 (misurata in 36 piani su 120),
  la **super-segreta** dal piano 2 e solo a estrazione concessa, 50% (misurata
  in 13 su 96 candidati) — vedi `governance/open-questions.md` voce 44. La
  domanda resta quindi aperta solo per l'accesso "best-of" dell'arena dal
  Piano 0. **Aggiornamento 30/07 (WP15a):** anche quello è ora nel motore, con la
  propria forma — tre piazzole fisse nel crogiolo, non un piazzamento
  probabilistico su una griglia: la "frequenza per piano" non ha significato
  per un'arena che vive nell'hub. Vedi `governance/open-questions.md`, voce 50,
  e [floor-zero.md](./floor-zero.md).
- Valori esatti di soglia e ricompensa della stanza a tempo. **Aggiornamento
  30/07 (WP5):** esiste ora un default proposto e implementato — soglia
  `40s + 6s × celle del piano` dall'ingresso nel piano, ricompensa 6 Ingots
  solo entro soglia — vedi "Stato di implementazione" sopra,
  [rewards-and-economy.md](./rewards-and-economy.md) e
  `governance/open-questions.md` voce 3.
- Valori numerici esatti del budget di equità della puntata generata (DEC-044 fissa il
  principio, non i numeri). **Aggiornamento 30/07 (WP7):** esistono ora una tabella di
  valori equivalenti e una tolleranza proposte e implementate — 1 Ingot = 1 punto, salute
  immediata 4, salute massima 14, Crust 12, strumento di breccia 4, strumento di apertura
  5, Flux 30, oggetti 8/16/28/45 come i prezzi fissi del negozio; tolleranza
  `max(4 punti, 20% dell'offerta)` — vedi "Stato di implementazione: la Pourhouse" sopra,
  la tabella in [rewards-and-economy.md](./rewards-and-economy.md) e
  `governance/open-questions.md`, voce 42. Restano default di implementazione, non canone.
- Cosa succede alla puntata quando il giocatore la rifiuta: resta disponibile per un
  ritorno successivo o si brucia? **Aggiornamento 30/07 (WP7):** default proposto e
  implementato — **resta disponibile**, il rifiuto è semplicemente uscire e non consuma
  nulla; solo l'accettazione chiude la stanza. Vedi `governance/open-questions.md`, voce 43.
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

*(WP15a, 30/07: realizzato — le piazzole del crogiolo aprono simulazioni popolate dai tipi
di nemico e di boss della migliore run registrata nel catalogo, con ripiego curato quando
il catalogo è vuoto. La selezione dal **museo** resta futura: il museo non è nel motore.)*

### Scenario 3 — Scambio ad alto rischio senza nulla da cedere

Given un giocatore senza risorse o oggetti cedibili che entra in una stanza di scambio ad alto rischio
When valuta le opzioni disponibili
Then la stanza offre comunque un'uscita senza penalità, senza bloccare il progresso, e il banco dichiara per esteso che non c'è nulla da versare (WP7: `Game.pourhouse.valid` falso, porte mai bloccate, tasto di conferma senza effetto)

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

### Scenario 12 — Puntata accettata: prezzo e offerta insieme, mai una sola delle due

Given un giocatore che, dentro una Pourhouse, legge sul banco offerta e prezzo scritti per esteso
When si porta sul banco e preme il tasto di interazione
Then il prezzo viene pagato **e** l'offerta consegnata nello stesso istante; se una delle due non fosse più possibile — inventario pieno, risorsa spesa altrove, oggetto non più posseduto — non accade nessuna delle due e il giocatore non perde nulla

### Scenario 13 — Puntata rifiutata

Given un giocatore che entra in una Pourhouse, legge la puntata e decide di non accettarla
When esce dalla stanza dalla porta
Then non paga alcun prezzo e non subisce alcuna penalità, e tornando nella stessa stanza ritrova **la stessa** puntata: il rifiuto non la consuma

### Scenario 14 — Prezzo in salute massima

Given un giocatore che accetta una puntata il cui prezzo è salute **massima**
When la colata viene versata
Then il tetto di salute base scende in modo permanente per il resto della run, non torna al primo ricalcolo delle statistiche, non scende mai sotto un cuore, e la salute temporanea/protettiva (Crust, DEC-008) resta intatta: il Crust è protezione, non valuta, e non paga mai un prezzo di salute

### Scenario 9 — Stanza a tempo raggiunta in tempo

Given un giocatore che raggiunge una stanza a tempo entro la soglia richiesta in un piano avanzato
When entra nella stanza
Then riceve la ricompensa aggiuntiva descritta in [rewards-and-economy.md](./rewards-and-economy.md), coerente col timer di run sempre visibile nell'HUD (DEC-051)
