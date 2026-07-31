---
id: gd-system-rooms-floors
title: Rooms and Floor Generation
domain: design
status: approved
authority: canonical
owner: design
summary: "Struttura dei piani (griglia fissa, numero e grandezza di stanze variabili, DEC-009) e tassonomia completa dei tipi di stanza (DEC-010, estesa a un quinto archetipo dalla stanza a tempo, DEC-051). Modificatori di stanza generati nei piani avanzati (DEC-024). Il budget di difficoltà della stanza è condiviso tra ostacoli e nemici (DEC-043). Le stanze hanno taglie multiple in classi discrete stile Isaac (1x1/1x2/2x1/2x2/L) con telecamera a zoom fisso nelle taglie maggiori (DEC-170), che supera parzialmente il modello di taglie continue di DEC-009; nelle forme a L la telecamera segue in continuo clampata all'intera stanza (DEC-180), non più alla cella corrente. Una sola porta per coppia di stanze adiacenti, nel segmento più centrale del confine condiviso (DEC-181). La stanza boss è sempre foglia del grafo di adiacenza del piano, mai un passaggio obbligato (DEC-182). Il Piano 0 non è un piano generato: vedi floor-zero.md."
last_reviewed: 2026-07-31
last_verified_commit: 4d7a410
topics: [stanze, piani, generazione, griglia, budget-difficoltà, taglie-multiple, telecamera, forma-a-L, DEC-170, DEC-180, DEC-181, DEC-182, porta-unica, boss-isolato, DEC-043, WP3, ostacoli, ROOM_FUSION, ROOM_TIMED, ROOM_ARENA, ROOM_POURHOUSE, ROOM_SECRET, WP5, WP6, WP7, WP8, DEC-025, DEC-191]
related: []
supersedes: []
source_files: [src/render/game_renderer.c, src/assets/art_atlas.h, src/render/art_draw.h, src/core/room_layout.h, src/world/world.c, src/tests/game_tests.c]
---

# Rooms and Floor Generation

## Intento per il giocatore

Ogni piano deve sembrare costruito apposta, con una progressione leggibile verso il boss, anche se stanze e layout sono generati o variati piano dopo piano.

## Condizioni di ingresso

- Un piano viene generato quando la run lo richiede: i piani 1-5. La vittoria al boss del piano 5 chiude la run (DEC-006, DEC-031, vedi [bosses.md](./bosses.md)); non esistono piani oltre il 5 in questa fase del gioco.
- Il **Piano 1** è generato con le stesse regole di tutti gli altri piani: nessun trattamento speciale rispetto ai piani 2-5.
- Il **Piano 0** NON è un piano generato: è la sala d'attesa/hub del gioco. Per i suoi dettagli vedi [floor-zero.md](./floor-zero.md); questo documento non li ripete.

## Struttura del piano (DEC-009)

- Il piano usa una **griglia fissa** come intelaiatura spaziale.
- Il **numero di stanze** per piano è **variabile**.
- Le stanze hanno **taglie multiple**, con una **grandezza minima garantita**. La forma
  esatta della variabilità dimensionale è fissata da **DEC-170** (27/07): classi discrete
  stile Isaac (1x1/1x2/2x1/2x2/L), non più il lattice di taglie continue "tutte diverse tra
  loro" descritto sotto — vedi [Taglie multiple e telecamera](#taglie-multiple-e-telecamera-dec-170).
  DEC-009 riceve una nota di supersessione parziale su questo punto nel decision-log: il
  principio di variabilità e il minimo garantito restano validi.
- **Stato:** regola di design **approved**. **Implementato (DEC-170, 27/07):** il codice
  genera stanze a classi discrete (1x1/1x2/2x1/2x2/L) su celle contigue della griglia, con
  telecamera a zoom fisso nelle taglie maggiori — vedi
  [Taglie multiple e telecamera](#taglie-multiple-e-telecamera-dec-170) e i default sotto.
  Il lattice di taglie in pixel della fase M2 **non esiste più nel codice**: il blocco
  "Default proposti dall'implementazione M2" resta solo come riferimento storico.

### Default proposti dall'implementazione M2 (stile DEC-019) — superato in parte da DEC-170

I valori esatti restano una domanda aperta di design (vedi `governance/open-questions.md`):
quelli sotto erano **default proposti dal codice** per la fase M2, non una decisione di
design, e restano qui come riferimento storico dei valori scelti allora. Il rettangolo
massimo del canvas di gioco (960×640 logici) restava fisso; ogni stanza era un rettangolo
più piccolo o uguale, sempre centrato al suo interno.

- **Numero di stanze per piano:** `6 + numero_piano + estrazione(0..3)` dal seme della run
  (piano 1: 7..10 stanze; piano 5: 11..14), più fino a due stanze speciali (tesoro/negozio)
  quando trovano posto. Varia sia fra piani sia fra run con seed diverso. (Non toccato da
  DEC-170: riguarda il numero di stanze, non la loro taglia.)
- ~~**Lattice delle taglie:** larghezze `{876, 812, 748, 684, 620, 556}` × altezze
  `{458, 418, 378, 338, 298}` px, quantizzate a passi di 8px (coerenza pixel-art). Le coppie
  si pescano senza ripetizione da un pool di 30 combinazioni mescolato col seme della run:
  nessuna stanza dello stesso piano condivide la stessa taglia.~~ — **superato da DEC-170**:
  il modello attuale è a classi discrete (1x1/1x2/2x1/2x2/L), e più stanze dello stesso
  piano possono condividere la stessa taglia.
- ~~**Grandezza minima garantita:** 556×298 px (il valore più piccolo del lattice sopra).~~
  — **superato da DEC-170**: il minimo garantito è ora la taglia **1x1**; le sue dimensioni
  esatte in pixel restano da fissare dall'implementazione.
- ~~**Stanza boss:** sempre alla taglia massima (876×458, l'intero rettangolo del canvas).
  **Stanza di partenza:** riceve una taglia riservata "almeno mediana" del lattice (non la
  più piccola disponibile). Tesoro/negozio pescano liberamente dal resto del pool.~~ —
  **superato da DEC-170**: quale taglia esatta ricevano la stanza boss e la stanza di
  partenza nel nuovo modello a classi discrete non è fissato da questa decisione, resta un
  dettaglio di implementazione.

## Taglie multiple e telecamera (DEC-170)

Le stanze hanno **taglie multiple in classi discrete**, stile Isaac. Ogni stanza occupa una
o più **celle contigue della griglia fissa del piano** (DEC-009):

- **1x1** — una cella: la taglia base e minima garantita.
- **1x2** e **2x1** — due celle in fila, rispettivamente orizzontale e verticale.
- **2x2** — un blocco di quattro celle.
- **forma a L** — tre celle contigue di un blocco 2x2, con un angolo mancante.

Più stanze dello stesso piano possono condividere la stessa taglia: la clausola precedente
di DEC-009 "grandezze tutte diverse tra loro" **non si applica più** in questa forma (la
variabilità dimensionale del piano resta un requisito valido, ma è di **classe**, non di
misura continua univoca per stanza). Le dimensioni esatte in pixel di una cella e quale
taglia ricevano la stanza boss o la stanza di partenza restano **dettagli di
implementazione**, non fissati da questa decisione.

### Telecamera

- **Stanza 1x1:** inquadrata **per intero**, con **camera fissa** — comportamento invariato
  rispetto al modello M2: l'intera stanza è sempre visibile, senza movimento di camera e
  senza zoom.
- **Taglie maggiori (1x2, 2x1, 2x2, L):** il giocatore cammina dentro uno spazio più ampio
  dello schermo; la **telecamera lo segue** a **zoom fisso** — **nessuno zoom dinamico** che
  si adatti alla taglia della stanza o all'azione — **clampata ai bordi della stanza**: non
  mostra mai area fuori dal rettangolo occupato dalla stanza, nemmeno quando il giocatore è
  a ridosso di un bordo. Questo vale **allo stesso modo per la forma a L** (DEC-180, 30/07):
  il clamp è sul rettangolo dell'**intera stanza** (il blocco 2x2), non su una singola
  cella — vedi il dettaglio nella sezione "Default proposti dall'implementazione" sotto.

Come i layout di ostacoli esistenti (`ROOM_LAYOUT_*`, vedi
[Secrets and Obstacles](./secrets-and-obstacles.md)) si applichino alle stanze multi-cella —
per singola cella o estesi sull'intera stanza — resta un **dettaglio di implementazione**,
non fissato da questa decisione. Nessun rimando aggiunto a `ui/hud.md`: quel documento non
parla di inquadratura/telecamera.

### Default proposti dall'implementazione (DEC-170, stile DEC-019)

DEC-170 fissa le classi di taglia e il comportamento della telecamera, non i numeri. Questi
sono **default proposti dall'implementazione del 27/07**, non decisioni di design: restano
da confermare (vedi `governance/open-questions.md`).

- **Dimensione di una cella:** il canvas logico di gioco di sempre, **876×458 px**
  (`ROOM_X/Y/W/H`). Una cella più la sua cornice di muro riempie esattamente lo schermo
  960×640: è ciò che rende la stanza **1x1 identica a prima di DEC-170**, telecamera ferma
  e cornice compresa, senza alcun caso speciale nel codice.
- **Grandezza minima garantita (DEC-009):** una cella, cioè 876×458 px.
- **Stanza di partenza: sempre 1x1.** Il primo schermo di un piano è anche quello che
  insegna a leggere lo spazio, e la 1x1 si vede per intero senza muovere la telecamera.
- **Stanza boss: 2x2 quando entra nella griglia** (arena dedicata, coerente con
  [bosses.md](./bosses.md)). Non è una garanzia: si piazza per ultima e la griglia può
  essere satura, nel qual caso scende di classe (L → 1x2/2x1 → 1x1). Misurato su 120 piani
  generati (5 piani × 24 semi): **2x2 in 110 casi su 120**.
- **Tesoro, negozio, stanza di fusione, stanza a tempo e Pourhouse: sempre 1x1.** Sono
  stanze da una funzione sola (una ricompensa, una vetrina, il crogiolo, la clessidra, il
  banco della colata), tutta visibile appena si entra — leggere una puntata e decidere non
  ha bisogno di spazio, ha bisogno che il banco sia sotto gli occhi. La stanza a tempo
  (WP5) è l'unica delle cinque esclusiva dei piani avanzati (dal piano 3); la Pourhouse
  (WP7) è l'unica il cui tentativo di piazzamento **non si fa a ogni piano** — dal piano 2
  e solo quando l'estrazione del piano lo concede, vedi sotto e
  [Special Rooms](./special-rooms.md).
- **Arena di sfida: mai 1x1** (WP6, [Special Rooms](./special-rooms.md)). È l'eccezione
  dichiarata alla riga sopra, e per il motivo opposto: l'arena è **combattimento**, non
  una funzione sola da leggere a colpo d'occhio, e una 1x1 stretta mortificherebbe
  un'ondata maggiorata. Ha un piazzamento suo (`WorldPlaceArenaRoom`) che prova
  **2x2 → L → 1x2/2x1** in quest'ordine e rinuncia del tutto se nessuna entra. È anche
  sempre una **foglia del grafo** come la stanza boss (DEC-182): è così che si garantisce
  strutturalmente il caso limite "l'arena non è mai un passaggio obbligato".
- **Distribuzione delle taglie delle altre stanze** (estrazione dall'RNG del piano, quindi
  deterministica dal seed): **1x1 55% · 1x2 15% · 2x1 15% · 2x2 8% · L 7%** (per la L
  l'orientamento dell'angolo mancante è a sua volta estratto fra i quattro). La 1x1 resta
  la maggioranza netta di proposito: se le taglie grandi diventassero la norma, il caso che
  DEC-170 vuole esplicitamente invariato — la stanza inquadrata per intero — diventerebbe
  l'eccezione. Se la forma estratta non entra nello spazio libero, si ripiega su una 1x1
  nella stessa cella (mai un turno saltato).
- **Quantità di piano:** il budget `6 + numero_piano + estrazione(0..3)` di DEC-009 ora conta
  **celle**, non stanze (una stanza ne occupa da 1 a 4), più la stanza boss e fino a **tre**
  stanze speciali 1x1 (tesoro, negozio e — dal WP4 — la stanza di fusione `ROOM_FUSION`), o
  fino a **quattro** nei piani 3+, dove si aggiunge — dal WP5, solo un tentativo, non
  garantito — la stanza a tempo `ROOM_TIMED` ([Special Rooms](./special-rooms.md), DEC-051).
  Dal WP6 si aggiungono, nei piani 2+, le **2-4 celle dell'arena di sfida** `ROOM_ARENA`
  (anch'essa un solo tentativo, non garantito): è l'unica delle stanze speciali che non è
  1x1, e si piazza **prima** delle quattro sopra perché è l'unica a chiedere celle libere
  contigue — il costo di quell'ordine sulle 1x1 è misurato e dichiarato in
  [Special Rooms](./special-rooms.md).
  Dal WP7 si aggiunge, nei piani 2+, la **quinta speciale 1x1**: la Pourhouse
  `ROOM_POURHOUSE` (scambio ad alto rischio, DEC-136). È l'unica il cui tentativo è
  **condizionato**: non si prova a ogni piano ma solo quando l'estrazione del piano lo
  concede (70%, default proposto), e si piazza **per ultima** dopo ogni altra stanza —
  scelta deliberata, così la sua estrazione non sposta il flusso RNG di nessun altro
  piazzamento e le misure del WP6 restano valide parola per parola. Il totale massimo di
  celle speciali 1x1 per piano sale quindi a **cinque** dal piano 3 in su (tesoro, negozio,
  fusione, a tempo, Pourhouse) e a **quattro** nei piani 2.
  Dal WP8 si aggiungono fino a **due celle 1x1 EXTRA**: le stanze **segrete** `ROOM_SECRET`
  (una normale, tentata a ogni piano dal piano 1; una super-segreta, dal piano 2 e solo a
  estrazione concessa — DEC-025, [Special Rooms](./special-rooms.md)). Sono celle **in
  più**, mai una sostituzione: si piazzano solo su celle libere e non tolgono mai il posto
  a una stanza già piazzata. A differenza di ogni altra stanza del piano NON entrano nella
  connettività — finché il varco murato non è sbrecciato non hanno alcuna porta — quindi
  la garanzia "dalla partenza si raggiunge ogni stanza" si misura **senza contarle**, ed è
  il modo strutturale di dire che il piano resta completabile ignorando i segreti.
  Si piazzano **dopo** boss, arena, tesoro e negozio e **prima** di fusione, stanza a tempo
  e Pourhouse: hanno il vincolo di posizione più stretto di tutte (serve una cella libera
  con **una sola** cella vicina), e il costo di quell'ordine sulle tre 1x1 che vengono dopo
  è misurato e dichiarato in [Special Rooms](./special-rooms.md).
  La superficie giocabile di un piano resta quindi quella di sempre; il
  **numero di stanze scende** (~5-10 più boss e speciali). È una conseguenza dichiarata di
  DEC-170, non un effetto collaterale.
- **Budget di difficoltà di una stanza multi-cella:** il budget nemici della stanza
  (`3 + numero_piano + estrazione(0..2)`, vedi [enemies.md](./enemies.md)) viene moltiplicato
  per la **radice quadrata** del numero di celle, non per il numero di celle: una 2x2 è
  quattro schermate di spazio e col budget di una sola resterebbe vuota, ma con quattro volte
  i nemici sarebbe quattro stanze appiccicate invece di una stanza grande. Il tetto di buon
  senso agli spawn (16) scala per cella, sempre sotto il tetto duro di motore.
- **Riduzione del budget per gli ostacoli della stanza (DEC-043, WP3):** dopo la
  moltiplicazione per la radice quadrata sopra, il budget nemici si riduce di un importo per
  ogni ostacolo ambientale generato nella stanza (qualunque famiglia — solido, distruttibile
  o pericolo — croce centrale esclusa per costruzione), mai sotto una soglia minima che
  garantisce almeno un nemico. I numeri esatti (costo per ostacolo, soglia minima) sono un
  default proposto dall'implementazione, fonte unica in
  [secrets-and-obstacles.md](./secrets-and-obstacles.md) ("Default proposti
  dall'implementazione"), non riformulati qui.
- **Layout di ostacoli: applicati PER CELLA** (`RoomLayoutBuild` una volta per cella, seme
  mescolato dalle coordinate assolute della cella). Due celle della stessa stanza non hanno
  lo stesso arredo, e ogni cella conserva la garanzia esistente della croce centrale libera:
  la porta al centro di ciascun lato *e* il passaggio verso la cella accanto restano sempre
  raggiungibili. Tetto invariato a 10 blocchi per cella.
- **Angolo mancante di una forma a L:** è **muro pieno** — il gioco lo tratta come un
  ostacolo solido (ferma giocatore, nemici e colpi con lo stesso codice degli ostacoli) e il
  renderer lo disegna come parete. Sull'altro lato può esserci una stanza diversa, e in quel
  caso la porta è normale.
- ~~**Telecamera nelle forme a L:** il clamp usa il rettangolo della **cella corrente**, non
  il riquadro della stanza. È l'unico modo di rispettare "non mostra mai area fuori dal
  rettangolo occupato" senza introdurre uno zoom dinamico (che DEC-170 vieta) o una
  maschera: il riquadro di una L contiene l'angolo mancante, e una telecamera libera dentro
  il riquadro lo mostrerebbe. Il salto di inquadratura al cambio di cella è assorbito da una
  **interpolazione breve** (avvicinamento esponenziale, ~0,25 s per il 95% dello scarto), la
  stessa che smorza l'inseguimento nelle altre taglie.~~ — **superato da DEC-180** (30/07,
  primo playtest): la telecamera nelle forme a L segue il giocatore **in continuo**,
  **clampata al rettangolo dell'intera stanza** (il blocco 2x2), esattamente come nelle
  altre taglie maggiori — nessun clamp per cella, nessuna interpolazione al cambio di
  cella. L'angolo mancante può entrare in inquadratura: da W8 il tileset lo rende come
  **muro/sfondo** (vedi [Stato di implementazione: la stanza vestita dal
  tileset](#stato-di-implementazione-la-stanza-vestita-dal-tileset-w8-2026-07-30) sotto),
  quindi il vincolo "non mostra mai area fuori dalla stanza" resta soddisfatto dal
  **rendering del vuoto**, non più dal clamp per cella.
- **Inseguimento:** la telecamera è **stato di simulazione**, aggiornata a passo fisso
  insieme al giocatore (non nel rendering), così a parità di passi simulati l'inquadratura è
  identica. All'ingresso in una stanza si aggancia subito all'inquadratura giusta: entrare
  non è mai una scivolata.

**Compatibilità con gli script Lua già generati:** i quattro getter `room_left()`,
`room_top()`, `room_right()`, `room_bottom()` esposti alla sandbox rispondono ora con il
riquadro della **stanza corrente** invece che con un rettangolo fisso. La firma non cambia,
gli script del catalogo già su disco continuano a funzionare e descrivono lo spazio vero;
ogni scrittura resta comunque clampata dentro la stanza dal motore.

## Porte e connettività del piano (DEC-181, DEC-182)

Due regole emerse dal primo playtest reale della demo (30/07), che si aggiungono alla
regola di generazione delle porte già fissata da DEC-170 (Scenario 12: la porta collega
sempre celle di **stanze diverse**, mai celle della stessa stanza) senza modificarla.

### Una porta per coppia di stanze (DEC-181)

Con le stanze multi-cella di DEC-170, due stanze adiacenti possono condividere **più di
una coppia di celle** sul proprio confine (es. due 2x2 affiancate lungo un lato intero, o
una 2x2 e una 1x2 a contatto su due celle). In questo caso la generazione apre
**esattamente una porta** per quella coppia di stanze — **mai** una porta per ogni coppia
di celle adiacenti, mai porte multiple affiancate sullo stesso confine. La porta si
colloca nel **segmento più centrale** del confine condiviso, scelto **deterministicamente
dal seed** del piano: stessa run, stesso seed, stessa posizione.

### La stanza boss è sempre foglia del grafo (DEC-182)

La stanza boss ha **sempre e solo una porta**: è una **foglia del grafo di adiacenza**
delle stanze del piano, mai un nodo di passaggio. La generazione deve garantire che,
**rimuovendo la stanza boss e la sua unica porta dal grafo**, tutte le altre stanze del
piano restino **reciprocamente raggiungibili** tramite un percorso alternativo — nessuna
stanza il cui unico accesso passi per forza dalla stanza boss. Questa è una proprietà
**strutturale** della generazione del piano, da **verificare con un test dedicato** sulla
generazione (connettività del grafo meno il nodo boss), non solo una regola dichiarata a
parole. Rimando da [Bosses](./bosses.md), che non ripete questa regola.

**Stato (30/07, implementato):** entrambe le regole sono implementate in
`src/world/world.c` e verificate da `--rooms-test`
(`src/tests/game_tests.c`, `GameRoomsTest`) su 120 piani generati (5 piani × 24 semi).

- **DEC-181** — `WorldLinkRooms` raggruppa i segmenti di confine per **coppia di stanze**
  (identificata dalla cella di stato risolta da `WorldRoomAt`, mai dall'origine grezza
  della stanza: due stanze diverse possono avere lo stesso valore numerico di origine
  quando la maschera di una di esse non include il bit (0,0), quindi l'origine da sola
  non è un identificativo univoco). Le stanze restano al massimo un blocco 2x2, quindi un
  confine condiviso copre al più due coppie di celle: con una sola coppia si apre quella,
  con due nessuna è "più centrale" dell'altra (confine di lunghezza pari) e la scelta fra
  le due usa un'estrazione dall'RNG del piano — deterministica dal seed. Test dedicato:
  (l) in `GameRoomsTest`, conta le porte aperte per ogni coppia di stanze adiacenti e
  pretende sempre esattamente 1.
- **DEC-182** — `WorldPlaceBossRoom` prova ogni taglia/posizione/orientamento solo se la
  forma tocca **esattamente una** stanza esistente distinta
  (`WorldShapeNeighborRoomCount`, anch'essa risolta per cella di stato, mai per origine
  grezza); il ripiego di griglia satura promuove a boss la stanza già piazzata di grado
  minimo (preferendo la più lontana con grado ≤1). Le stanze speciali 1x1 — tesoro, negozio,
  stanza di fusione, (dal piano 3, WP5) stanza a tempo e (dal piano 2, WP7) Pourhouse —
  piazzate **dopo** il boss, non si
  attaccano mai ad esso (`WorldPlaceSpecialRoom` scarta le celle candidate che toccano la
  stanza boss, per tutti e cinque i chiamanti) — altrimenti gli darebbero una seconda porta.
  **WP6:** la stessa regola vale ora per due stanze invece che per una — la stanza boss e
  l'**arena di sfida**, che deve restare foglia per lo stesso motivo (`ROOM_ARENA`,
  [Special Rooms](./special-rooms.md)): `WorldShapeTouchesLeafRoom` è il predicato unico
  che i chiamanti 1x1 e il piazzamento dell'arena condividono. L'arena non passa da
  `WorldPlaceSpecialRoom`: ha `WorldPlaceArenaRoom`, che
  oltre al vincolo di foglia prova le taglie grandi per prime e non scende sotto le due
  celle.
  **WP7:** `WorldPlaceSpecialRoom` passa da quattro a **cinque chiamanti** con la
  Pourhouse (`ROOM_POURHOUSE`), che eredita senza modifiche entrambi i vincoli — 1x1 e mai
  a contatto con una stanza che deve restare foglia. L'unica differenza è a monte, nel
  chiamante e non nella funzione: il suo tentativo è condizionato all'estrazione del piano
  (vedi «Quantità di piano» sopra).
  **WP8:** le stanze **segrete** (`ROOM_SECRET`) sono la **terza** categoria che deve
  restare foglia, insieme a boss e arena — `WorldShapeTouchesLeafRoom` le include, così
  nessuna stanza piazzata dopo di loro può attaccarsi al loro secondo lato e regalare una
  porta **normale** a un segreto. Non passano da `WorldPlaceSpecialRoom`: hanno
  `WorldPlaceSecretRoom`, che oltre al vincolo di foglia pretende **esattamente una** cella
  vicina esistente (un solo muro condiviso) e che quella vicina sia una stanza **normale**
  (partenza o combattimento). Sono anche l'unico piazzamento del generatore che legge uno
  **stream deterministico locale** derivato dal seed di run invece di `game->rng`: così
  l'aggiunta dell'archetipo non sposta di un bit le estrazioni di nessun altro
  piazzamento.
  Test dedicati: (m) grado della stanza boss sempre 1; (n) BFS dalla partenza che non entra
  mai in una cella della stanza boss raggiunge comunque tutte le altre stanze del piano;
  (q, WP6) gli stessi due controlli per l'arena di sfida; (r, WP7) unicità, taglia 1x1,
  piano minimo, non-adiacenza a boss e arena, e — controllo che fallirebbe se qualcuno
  rendesse il tentativo incondizionato — la Pourhouse **non** su tutti i piani candidati.
- **Effetto collaterale misurato:** il vincolo di foglia riduce la frequenza della classe
  2x2 per la stanza boss (una 2x2 ha più perimetro, quindi più occasioni di toccare due
  stanze diverse): da ~110/120 piani (pre-DEC-182) a **54/120** piani misurati dopo
  l'implementazione — resta comunque la classe più frequente fra quelle grandi, non più
  "quasi sempre".

## Tipi di stanza (tassonomia completa, DEC-010)

Questo documento è la fonte unica della tassonomia dei tipi di stanza. Tipi canonici:

- partenza;
- combattimento;
- tesoro;
- negozio;
- boss;

più quattro archetipi speciali:

- stanza di fusione;
- stanza segreta (`ROOM_SECRET` dal WP8, a **due livelli** — normale e super-segreta,
  DEC-025 — ed è l'ultimo dei cinque archetipi speciali a entrare nel motore);
- arena di sfida (`ROOM_ARENA` dal WP6 nella versione **incontrata nel piano**; l'accesso
  "best-of" dal Piano 0 resta descritto solo in [floor-zero.md](./floor-zero.md));
- scambio ad alto rischio — in-game **Pourhouse** (DEC-136), `ROOM_POURHOUSE` dal WP7;

più un quinto archetipo speciale, aggiunto da DEC-051:

- stanza a tempo (esclusiva dei piani avanzati).

Il dettaglio di ciascun archetipo speciale (accesso, costo, ricompensa, frequenza, segnale visivo) è descritto in [special-rooms.md](./special-rooms.md) come sottoinsieme dichiarato di questa tassonomia; questo documento non lo ridefinisce.

**Estensione DEC-051:** la stanza a tempo è una stanza fissa dei piani avanzati che dà una
ricompensa se raggiunta entro una soglia di tempo. Soglie e valori esatti restano da
playtest (vedi [Rewards and Economy](./rewards-and-economy.md) per il dettaglio della
ricompensa e [Special Rooms](./special-rooms.md) per il dettaglio dell'archetipo).
**Stato (WP5, 30/07):** ha ora un `RoomKind` fisico nel motore (`ROOM_TIMED`), esclusivo dei
piani 3+, con un default proposto e implementato per soglia e ricompensa — vedi "Stato di
implementazione" in [Special Rooms](./special-rooms.md).

## Input/azioni

Il giocatore si muove liberamente ed entra/esce dalle stanze attraverso i varchi generati dalla griglia; alcune stanze richiedono un'azione (sconfiggere nemici, pagare un costo, superare una sfida) per essere completate.

## Requisiti di una stanza

- ingressi e uscite validi;
- spazio di movimento;
- visibilità delle minacce;
- posizione sicura iniziale;
- condizioni di completamento;
- ricompensa;
- compatibilità con gli archetipi nemici (vedi [enemies.md](./enemies.md));
- assenza di blocchi impossibili;
- una sola porta per coppia di stanze adiacenti, anche quando il confine condiviso copre
  più coppie di celle (DEC-181);
- se la stanza è la stanza boss: una sola porta, foglia del grafo di adiacenza del piano,
  mai un passaggio obbligato per raggiungere un'altra stanza (DEC-182);
- rispetto della grandezza minima garantita (DEC-009).

## Risultato

Il completamento delle stanze necessarie apre l'uscita verso il boss del piano; la sconfitta del boss apre il piano successivo (vedi [bosses.md](./bosses.md)).

## Feedback

- segnale visivo di stanza non ancora esplorata/completata sulla mappa;
- distinzione visiva tra tipi di stanza standard e archetipi speciali;
- indicazione quando l'uscita verso il piano successivo è disponibile.

## Coerenza del piano

Ogni piano possiede:

- tema visivo;
- grammatica ambientale;
- budget di difficoltà;
- pool principali;
- almeno un elemento ricorrente;
- progressione interna verso il boss.

## Budget di difficoltà condiviso tra ostacoli e nemici (DEC-043)

Il **budget di difficoltà della stanza** (vedi "Coerenza del piano" sopra) copre insieme
gli ostacoli ambientali e i nemici della stanza: spendere budget in ostacoli riduce quanto
resta disponibile per i nemici, e viceversa. Non sono due budget separati e indipendenti.
Il dettaglio della generazione degli ostacoli a tema (forma, comportamento, garanzie di
giocabilità) è definito in
[Secrets and Obstacles](./secrets-and-obstacles.md) come fonte unica; questo documento
registra solo il vincolo di condivisione del budget con i nemici (vedi anche
[Enemies](./enemies.md) per il budget lato nemici).

## Modificatori di stanza nei piani avanzati (DEC-024)

Nei piani avanzati sono ammessi **modificatori di stanza generati** (es. una variazione di
regola o condizione applicata a una stanza esistente, coerente col grado crescente del
tema), sempre dentro le garanzie di giocabilità della validazione dei contenuti generati
(vedi [Generated Content Validation](./generated-content-validation.md), non riformulata
qui). Questo fa parte dell'asse "regole di stanza" dell'escalation leggibile del tema
descritta in [Difficulty and Progression](../07-difficulty-and-progression.md), fonte del
principio generale; questo documento non lo ripete.

## Stato di implementazione: la stanza vestita dal tileset (W8, 2026-07-30)

Le stanze non sono più colori piatti: `assets/art/tiles/<tema>.png` + il suo manifest
(contratto in `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md`) vestono pavimento,
cornice di muro, angoli, porte, angolo mancante della forma a L, ostacoli e il vuoto fuori
dalla stanza. Cinque cose da sapere, tutte in `src/render/game_renderer.c`
(`RoomTileset`/`DrawRoomTiled`/`DrawTiledArea`):

- **Come si scegle il tileset.** `Theme` (`src/core/game_types.h`) non ha un
  identificatore: porta un NOME testuale e nient'altro, perché il nome lo **inventa il
  modello** e i cinque temi della demo esistono solo come nomi nel contenuto di ripiego.
  Due gradini: (1) lo **slug** del nome è provato come nome di file (`"Lunar Forge"` →
  `tiles/lunar-forge`, il cammino esatto dei cinque temi curati, e valido per qualunque
  tema futuro a cui la sessione artistica dedichi un tileset omonimo, senza toccare il C);
  (2) per un tema generato (`"Library of Radiation"`) si sceglie uno dei cinque per **hash
  FNV-1a** del nome — deterministico, quindi lo stesso mondo si veste sempre allo stesso
  modo in ogni run e su ogni macchina. Un tileset per tema *generato* andrebbe generato:
  è la Style LoRA di DEC-148, non un problema risolvibile con più codice.
- **La variante di escalation (DEC-024, asse aspetto).** Il contratto emette tre ruoli col
  suffisso `_deg` (`floor_deg`/`wall_deg`/`void_deg`, "crepe di brace"); nessun documento
  fissava a quale piano scattano. **Canone (DEC-191, 31/07)**: dal **piano 3**, lo
  stesso confine della seconda traccia di gameplay e del passaggio dei boss a due fasi
  (DEC-028/106) — confermato dal proprietario: i tre assi dell'escalation coincidono di
  proposito sullo stesso confine.
- **Le porte hanno tre stati leggibili** — `aperta`, `chiusa`, `bloccata` — nello stesso
  vocabolario (italiano) dei ruoli del tileset e delle animazioni di `props/porta`: un solo
  vocabolario per il tile e per lo sprite. `bloccata` quando la stanza tiene chiuse le
  uscite finché non la si pulisce, `chiusa` quando la stanza non è ancora pulita, `aperta`
  altrimenti.
- **Gli ostacoli si vestono per FAMIGLIA di layout** (`obst_pillar`/`obst_corridor`/
  `obst_arena`/`obst_scatter`, uno per valore di `RoomForm`): una stanza a colonne si vede
  come colonne e una a strozzature come strozzature, non come lo stesso blocco quattro
  volte. Il campo di gioco (`ROOM_*`) e la collisione **non cambiano di un pixel**: è
  tutta resa, come per la resa 2.5D.
- **Cosa NON è cambiato, e perché.** Le fasce di muro restano quelle decorative storiche
  (34 px al fondo, 12 ai lati, 14 davanti), non una fila intera di tile da 32: quelle
  misure sono ancorate al bordo REALE del campo di gioco, e allargarle a 32 px per lato
  avrebbe spostato la parete di ~20 px rispetto alla collisione — cioè avrebbe fatto
  camminare il giocatore dentro il muro. Il tile viene **ritagliato** alla fascia (si
  ritaglia il rettangolo sorgente, non si comprime il tile: comprimere avrebbe deformato i
  pixel proprio sul bordo, dove l'occhio confronta il tile col suo vicino intero), quindi
  il muro laterale mostra i suoi primi 12 px — che è esattamente ciò che si vede di uno
  spessore visto di taglio. Analogamente `ROOM_W`×`ROOM_H` (876×458) non è multiplo di 32:
  l'ultima colonna e l'ultima riga di tile sono ritagliate allo stesso modo. Resta anche
  la sfumatura che scurisce il fondo della stanza: è la prospettiva atmosferica della resa
  2.5D, senza la quale un pavimento a tile piatto perde ogni profondità.
- **Ripiego integrale**: nessun tileset caricabile (checkout senza `assets/art/`, PNG
  corrotto) → `DrawRoomFlat`, i colori piatti del tema con la griglia in prospettiva, cioè
  esattamente la resa di prima di W8. Un solo punto di scelta (`DrawRoom`), mai i due
  percorsi mescolati.

## Interazioni

- con il tema di run scelto nel Piano 0 (vedi [floor-zero.md](./floor-zero.md)), che evolve/degenera piano dopo piano;
- con gli archetipi nemici (vedi [enemies.md](./enemies.md)) e il boss di fine piano (vedi [bosses.md](./bosses.md));
- con le stanze speciali (vedi [special-rooms.md](./special-rooms.md));
- con ostacoli e segreti (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)), inclusa la condivisione del budget di difficoltà della stanza tra ostacoli e nemici (DEC-043).

## Regole per contenuti generati

- Il layout di ogni stanza dichiara un'origine (curato | composto | variato | nuovo).
- La generazione deve rispettare la griglia fissa, il numero variabile di stanze, le grandezze diverse con minimo garantito, e i requisiti di stanza sopra elencati.
- Il Piano 1 può appoggiarsi a un pool più curato per garantire l'avvio immediato della run, pur non avendo trattamento speciale nella generazione (vedi [generated-content-validation.md](./generated-content-validation.md) per la regola di fallback).

## Casi limite

- Una stanza generata sotto la grandezza minima garantita: va respinta o ridimensionata prima di essere proposta.
- Un piano con numero di stanze generato troppo basso per contenere tutti i tipi richiesti (es. manca il negozio): la generazione va integrata o rifiutata secondo la validazione.
- Il piano successivo non è ancora pronto quando il giocatore raggiunge l'uscita: gestito come attesa nel Piano 0, non come blocco (vedi [floor-zero.md](./floor-zero.md) e [generated-content-validation.md](./generated-content-validation.md)).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni piano ha contenuti curati sufficienti a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non descrive il Piano 0 (vedi [floor-zero.md](./floor-zero.md)).
- Non dettaglia i cinque archetipi speciali, incluso il quinto aggiunto da DEC-051 (vedi [special-rooms.md](./special-rooms.md)).
- Non definisce il budget di leggibilità di nemici o attacchi (vedi [enemies.md](./enemies.md), [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Domande aperte residue

- Valore esatto della grandezza minima garantita (DEC-009 non specifica un numero).
- Intervallo esatto del numero variabile di stanze per piano.
- Regole esatte (soglie per piano, intensità) di degenerazione del tema piano dopo piano,
  oltre al principio dei quattro assi e all'ammissibilità di modificatori di stanza
  generati nei piani avanzati fissati da DEC-024 (vedi anche [bosses.md](./bosses.md)).
- Frequenza esatta e piani minimi in cui compare la stanza a tempo (DEC-051 fissa solo
  "piani avanzati", non il numero esatto). **Aggiornamento 30/07 (WP5):** esiste ora un
  default proposto e implementato — un tentativo per piano, solo dal piano 3
  (`WORLD_TIMED_ROOM_MIN_FLOOR`) — vedi "Stato di implementazione" in
  [Special Rooms](./special-rooms.md) e `governance/open-questions.md`, voce 32.
- Dimensioni esatte in pixel di una cella/taglia, quale taglia ricevano la stanza boss e la
  stanza di partenza, come i layout `ROOM_LAYOUT_*` si applichino alle stanze multi-cella e
  con quali percentuali si distribuiscano le classi di taglia (DEC-170 fissa le classi
  discrete 1x1/1x2/2x1/2x2/L e il comportamento della telecamera, non questi valori:
  dettagli di implementazione). **Default proposti dall'implementazione il 27/07** nella
  sezione omonima sopra, in attesa di conferma.
- ~~Se il clamp della telecamera nelle forme a L debba restare la **cella corrente**
  (scelta dell'implementazione: nessuno zoom dinamico, l'angolo mancante non si vede mai,
  ma il giocatore non vede la cella in cui sta per entrare finché non la attraversa) o se
  valga la pena di una regola più permissiva.~~ — **Chiusa (30/07, DEC-180):** il clamp è
  ora sul rettangolo dell'**intera stanza**, non sulla cella corrente; l'angolo mancante
  può entrare in inquadratura e il tileset lo rende come muro/sfondo (W8).
- ~~Il test dedicato che verifica la connettività del grafo delle stanze **meno il nodo
  boss** (DEC-182) non esiste ancora nel motore: resta un gap di implementazione da
  chiudere, non solo una regola di design.~~ — **Chiusa (testo non aggiornato):** il test
  esiste ed è il controllo `(n)` di `GameRoomsTest` (`src/tests/game_tests.c`), che gira
  in `make test` su 120 piani generati. Dal WP6 esiste anche il gemello per l'arena di
  sfida (controllo `(q)`: BFS che ignora l'arena e raggiunge comunque ogni altra stanza).

## Scenari

### Scenario 1 — Piano 1 senza trattamento speciale

Given l'avvio di una nuova run
When il Piano 1 viene generato
Then usa la stessa griglia fissa, lo stesso numero variabile di stanze e le stesse regole di grandezza degli altri piani, senza layout o contenuti riservati solo al Piano 1

### Scenario 2 — Stanza sotto la grandezza minima

Given una stanza generata con grandezza sotto il minimo garantito da DEC-009
When la validazione la verifica
Then la stanza è respinta o ridimensionata prima di poter comparire nel piano

### Scenario 3 — Piano senza negozio generato

Given un piano generato che non contiene un negozio tra i suoi tipi di stanza
When la validazione controlla la tassonomia richiesta dal piano
Then il piano viene integrato con un negozio (curato o generato) o rifiutato, secondo le regole di fallback

### Scenario 4 — Ingresso al Piano 0 vs Piano 1

Given un giocatore nel Piano 0
When sceglie di entrare nel piano successivo pronto
Then entra nel Piano 1 generato con le stesse regole descritte in questo documento, mentre il Piano 0 resta descritto solo in [floor-zero.md](./floor-zero.md)

### Scenario 5 — Modificatore di stanza generato in un piano avanzato

Given un piano avanzato (es. dal piano 3 in su) che genera un modificatore di stanza legato al tema
When il modificatore viene proposto per una stanza
Then deve superare la validazione delle garanzie di giocabilità prima di poter apparire nella run, secondo [generated-content-validation.md](./generated-content-validation.md)

### Scenario 6 — Budget di stanza condiviso tra ostacoli e nemici

Given una stanza generata con un budget di difficoltà dichiarato
When la generazione spende una parte consistente del budget in ostacoli ambientali a tema (DEC-043)
Then il budget restante per i nemici della stessa stanza si riduce di conseguenza, perché ostacoli e nemici condividono lo stesso budget di difficoltà

### Scenario 7 — Stanza a tempo nei piani avanzati

Given un piano avanzato generato (dal piano 3 in su)
When il piano include l'archetipo aggiuntivo "stanza a tempo" (DEC-051, `ROOM_TIMED`)
Then la stanza è raggiungibile entro una soglia di tempo (default proposto: `40s + 6s × celle del piano` dall'ingresso nel piano) per ottenere una ricompensa extra (default proposto: 6 Ingots), con soglia e valore esatti da confermare col playtest, secondo il dettaglio in [rewards-and-economy.md](./rewards-and-economy.md) e [special-rooms.md](./special-rooms.md)

### Scenario 8 — Stanza 1x1 con camera fissa

Given una stanza generata di taglia 1x1
When il giocatore vi entra
Then la stanza è inquadrata per intero con camera fissa, senza movimento di camera né zoom dinamico (DEC-170)

### Scenario 9 — Telecamera a zoom fisso in una stanza più grande

Given una stanza generata di taglia maggiore (1x2, 2x1, 2x2 o a L)
When il giocatore cammina dentro la stanza
Then la telecamera lo segue a zoom fisso, senza mai mostrare area fuori dal rettangolo occupato dalla stanza (clamp ai bordi) e senza variare lo zoom (DEC-170)

### Scenario 10 — Una stanza multi-cella è un solo spazio continuo

Given una stanza di taglia 2x2 (quattro celle della griglia)
When il giocatore passa dalla cella in cui è entrato a una cella adiacente della stessa stanza
Then non c'è transizione, né porta, né ricarica di contenuti: cammina dentro lo stesso spazio, e cambia solo l'inquadratura (DEC-170)

### Scenario 11 — Angolo mancante di una forma a L

Given una stanza a L (tre celle di un blocco 2x2)
When il giocatore cammina verso l'angolo mancante
Then viene fermato come da un muro; la telecamera — clampata al rettangolo dell'intera stanza (DEC-180) — può mostrarlo in inquadratura, ma il tileset lo rende come muro/sfondo, mai come area vuota fuori dalla stanza

### Scenario 12 — Porte fra celle di stanze diverse

Given due stanze adiacenti sul piano, di cui una multi-cella
When la generazione collega il piano
Then la porta collega le due CELLE adiacenti (una stanza 2x2 può avere porte su più lati esterni), mentre fra due celle della stessa stanza non esiste porta

### Scenario 13 — Una sola porta fra due stanze con confine ampio

Given due stanze adiacenti che condividono più di una coppia di celle sul proprio confine
When la generazione collega il piano
Then si apre esattamente una porta, nel segmento più centrale del confine condiviso, scelto deterministicamente dal seed del piano — mai porte multiple affiancate sullo stesso confine (DEC-181)

### Scenario 14 — La stanza boss non è mai un passaggio obbligato

Given un piano generato con la sua stanza boss, che ha una sola porta
When si rimuove la stanza boss e la sua porta dal grafo di adiacenza delle stanze
Then tutte le altre stanze del piano restano reciprocamente raggiungibili tramite un percorso alternativo, perché la stanza boss è sempre una foglia del grafo (DEC-182)
