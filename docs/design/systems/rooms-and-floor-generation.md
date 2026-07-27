---
id: gd-system-rooms-floors
title: Rooms and Floor Generation
domain: design
status: approved
authority: canonical
owner: design
summary: "Struttura dei piani (griglia fissa, numero e grandezza di stanze variabili, DEC-009) e tassonomia completa dei tipi di stanza (DEC-010, estesa a un quinto archetipo dalla stanza a tempo, DEC-051). Modificatori di stanza generati nei piani avanzati (DEC-024). Il budget di difficoltà della stanza è condiviso tra ostacoli e nemici (DEC-043). Le stanze hanno taglie multiple in classi discrete stile Isaac (1x1/1x2/2x1/2x2/L) con telecamera a zoom fisso nelle taglie maggiori (DEC-170), che supera parzialmente il modello di taglie continue di DEC-009. Il Piano 0 non è un piano generato: vedi floor-zero.md."
last_reviewed: 2026-07-27
last_verified_commit: 0ec60d0
topics: [stanze, piani, generazione, griglia, budget-difficoltà, taglie-multiple, telecamera, forma-a-L, DEC-170]
related: []
supersedes: []
source_files: []
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
- **Stato:** regola di design **approved**. **Implementato (M2, superato in parte da
  DEC-170):** il codice della fase M2 genera un numero di stanze variabile per piano e
  assegna a ogni stanza una taglia distinta entro un rettangolo massimo fisso (nessuna
  camera) — il blocco "Default proposti" sotto ne conserva i valori come riferimento
  storico. Il modello attuale (taglie a classi discrete + telecamera) è descritto nella
  sezione dedicata sotto e resta da implementare.

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
  a ridosso di un bordo.

Come i layout di ostacoli esistenti (`ROOM_LAYOUT_*`, vedi
[Secrets and Obstacles](./secrets-and-obstacles.md)) si applichino alle stanze multi-cella —
per singola cella o estesi sull'intera stanza — resta un **dettaglio di implementazione**,
non fissato da questa decisione. Nessun rimando aggiunto a `ui/hud.md`: quel documento non
parla di inquadratura/telecamera.

## Tipi di stanza (tassonomia completa, DEC-010)

Questo documento è la fonte unica della tassonomia dei tipi di stanza. Tipi canonici:

- partenza;
- combattimento;
- tesoro;
- negozio;
- boss;

più quattro archetipi speciali:

- stanza di fusione;
- stanza segreta;
- arena di sfida;
- scambio ad alto rischio;

più un quinto archetipo speciale, aggiunto da DEC-051:

- stanza a tempo (esclusiva dei piani avanzati).

Il dettaglio di ciascun archetipo speciale (accesso, costo, ricompensa, frequenza, segnale visivo) è descritto in [special-rooms.md](./special-rooms.md) come sottoinsieme dichiarato di questa tassonomia; questo documento non lo ridefinisce.

**Estensione DEC-051:** la stanza a tempo è una stanza fissa dei piani avanzati che dà una
ricompensa se raggiunta entro una soglia di tempo. Soglie e valori esatti restano da
playtest (vedi [Rewards and Economy](./rewards-and-economy.md) per il dettaglio della
ricompensa e [Special Rooms](./special-rooms.md) per il dettaglio dell'archetipo).

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
  "piani avanzati", non il numero esatto).
- Dimensioni esatte in pixel di una cella/taglia, quale taglia ricevano la stanza boss e la
  stanza di partenza, e come i layout `ROOM_LAYOUT_*` si applichino alle stanze multi-cella
  (DEC-170 fissa le classi discrete 1x1/1x2/2x1/2x2/L e il comportamento della telecamera,
  non questi valori: dettagli di implementazione).

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

Given un piano avanzato generato
When il piano include l'archetipo aggiuntivo "stanza a tempo" (DEC-051)
Then la stanza è raggiungibile entro una soglia di tempo per ottenere una ricompensa extra, con soglia e valore esatti da definire col playtest, secondo il dettaglio in [rewards-and-economy.md](./rewards-and-economy.md) e [special-rooms.md](./special-rooms.md)

### Scenario 8 — Stanza 1x1 con camera fissa

Given una stanza generata di taglia 1x1
When il giocatore vi entra
Then la stanza è inquadrata per intero con camera fissa, senza movimento di camera né zoom dinamico (DEC-170)

### Scenario 9 — Telecamera a zoom fisso in una stanza più grande

Given una stanza generata di taglia maggiore (1x2, 2x1, 2x2 o a L)
When il giocatore cammina dentro la stanza
Then la telecamera lo segue a zoom fisso, senza mai mostrare area fuori dal rettangolo occupato dalla stanza (clamp ai bordi) e senza variare lo zoom (DEC-170)
