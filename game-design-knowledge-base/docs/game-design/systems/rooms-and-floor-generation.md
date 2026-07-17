---
id: gd-system-rooms-floors
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Struttura dei piani (griglia fissa, numero e grandezza di stanze variabili, DEC-009) e tassonomia completa dei tipi di stanza (DEC-010). Il Piano 0 non è un piano generato: vedi floor-zero.md."
---

# Rooms and Floor Generation

## Intento per il giocatore

Ogni piano deve sembrare costruito apposta, con una progressione leggibile verso il boss, anche se stanze e layout sono generati o variati piano dopo piano.

## Condizioni di ingresso

- Un piano viene generato quando la run lo richiede: i piani 1-5, e gli eventuali piani extra oltre il 5 (vedi [bosses.md](./bosses.md)).
- Il **Piano 1** è generato con le stesse regole di tutti gli altri piani: nessun trattamento speciale rispetto ai piani 2-5.
- Il **Piano 0** NON è un piano generato: è la sala d'attesa/hub del gioco. Per i suoi dettagli vedi [floor-zero.md](./floor-zero.md); questo documento non li ripete.

## Struttura del piano (DEC-009)

- Il piano usa una **griglia fissa** come intelaiatura spaziale.
- Il **numero di stanze** per piano è **variabile**.
- Le stanze hanno **grandezze tutte diverse tra loro**, con una **grandezza minima garantita**.
- **Stato:** regola di design **approved**. **Gap noto rispetto al codice:** l'implementazione attuale genera stanze tutte uguali; è un requisito di design non ancora implementato — il codice dovrà adeguarsi a questa regola, non viceversa.

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
- scambio ad alto rischio.

Il dettaglio di ciascun archetipo speciale (accesso, costo, ricompensa, frequenza, segnale visivo) è descritto in [special-rooms.md](./special-rooms.md) come sottoinsieme dichiarato di questa tassonomia; questo documento non lo ridefinisce.

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

## Interazioni

- con il tema di run scelto nel Piano 0 (vedi [floor-zero.md](./floor-zero.md)), che evolve/degenera piano dopo piano;
- con gli archetipi nemici (vedi [enemies.md](./enemies.md)) e il boss di fine piano (vedi [bosses.md](./bosses.md));
- con le stanze speciali (vedi [special-rooms.md](./special-rooms.md));
- con ostacoli e segreti (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md)).

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
- Non dettaglia i quattro archetipi speciali (vedi [special-rooms.md](./special-rooms.md)).
- Non definisce il budget di leggibilità di nemici o attacchi (vedi [enemies.md](./enemies.md), [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Domande aperte residue

- Valore esatto della grandezza minima garantita (DEC-009 non specifica un numero).
- Intervallo esatto del numero variabile di stanze per piano.
- Regole esatte di degenerazione del tema piano dopo piano (vedi anche [bosses.md](./bosses.md) per i piani extra).

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
