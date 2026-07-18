---
id: gd-system-rooms-floors
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Struttura dei piani (griglia fissa, numero e grandezza di stanze variabili, DEC-009) e tassonomia completa dei tipi di stanza (DEC-010, estesa a un quinto archetipo dalla stanza a tempo, DEC-051). Modificatori di stanza generati nei piani avanzati (DEC-024). Il budget di difficoltà della stanza è condiviso tra ostacoli e nemici (DEC-043). Il Piano 0 non è un piano generato: vedi floor-zero.md."
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
- Le stanze hanno **grandezze tutte diverse tra loro**, con una **grandezza minima garantita**.
- **Stato:** regola di design **approved**. **Implementato (M2):** il codice genera un numero di stanze variabile per piano e assegna a ogni stanza una taglia distinta entro un rettangolo massimo fisso (nessuna camera), rispettando una grandezza minima garantita — vedi il blocco "Default proposti" sotto per i valori scelti dall'implementazione.

### Default proposti dall'implementazione (stile DEC-019, da validare col playtest)

I valori esatti restano una domanda aperta di design (vedi `governance/open-questions.md`):
quelli sotto sono **default proposti dal codice**, non una decisione di design. Il rettangolo
massimo del canvas di gioco (960×640 logici) resta fisso; ogni stanza è un rettangolo più
piccolo o uguale, sempre centrato al suo interno.

- **Numero di stanze per piano:** `6 + numero_piano + estrazione(0..3)` dal seme della run
  (piano 1: 7..10 stanze; piano 5: 11..14), più fino a due stanze speciali (tesoro/negozio)
  quando trovano posto. Varia sia fra piani sia fra run con seed diverso.
- **Lattice delle taglie:** larghezze `{876, 812, 748, 684, 620, 556}` × altezze
  `{458, 418, 378, 338, 298}` px, quantizzate a passi di 8px (coerenza pixel-art). Le coppie
  si pescano senza ripetizione da un pool di 30 combinazioni mescolato col seme della run:
  nessuna stanza dello stesso piano condivide la stessa taglia.
- **Grandezza minima garantita:** 556×298 px (il valore più piccolo del lattice sopra).
- **Stanza boss:** sempre alla taglia massima (876×458, l'intero rettangolo del canvas).
- **Stanza di partenza:** riceve una taglia riservata "almeno mediana" del lattice (non la
  più piccola disponibile). Tesoro/negozio pescano liberamente dal resto del pool.

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
