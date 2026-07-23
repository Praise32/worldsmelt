---
id: gd-system-active-items
title: Active Items
domain: design
status: approved
authority: canonical
owner: design
summary: "Oggetti attivabili volontariamente dal giocatore, una delle 4 categorie della tassonomia oggetti. Ricarica a doppio canale di base — stanze completate ed energia droppata dai nemici — estensibile da oggetti che aggiungono ulteriori modi di ricarica (DEC-059)."
last_reviewed: 2026-07-22
last_verified_commit: 0ec60d0
topics: [oggetti, attivi, ricarica, slot, DEC-059, budget di potenza]
related: []
supersedes: []
source_files: []
---

# Active Items

Gli attivi sono una delle 4 categorie della tassonomia oggetti (attivo, passivo,
stat-up, Innesto). I campi obbligatori di un oggetto sono definiti in
[Items, Pools and Rarity](items-pools-and-rarity.md): questo documento non li
riformula, aggiunge solo le specificità del sottotipo attivo.

## Intento per il giocatore

Un attivo deve creare una decisione sul momento d'uso ("lo uso ora o lo tengo?"), non
essere un bonus automatico privo di scelta.

## Slot

Si parte con **1 slot attivo**; oggetti o eventi rari durante la run possono aggiungere
slot attivi aggiuntivi (vedi [Items, Pools and Rarity](items-pools-and-rarity.md#slot)).
Con più slot attivi il giocatore sceglie quale attivare in un dato momento, non tutti
insieme automaticamente.

## Condizioni di ingresso

Un attivo entra in gioco quando viene raccolto da un **piedistallo** e assegnato a uno
slot attivo libero; a slot pieni la raccolta è uno **scambio col piedistallo** (DEC-117,
sezione dedicata più sotto).

## Input/azioni

- input di attivazione dedicato (esplicito, non automatico);
- condizioni d'uso (es. richiede bersaglio, richiede stanza non in transizione, ecc.).

## Come si attivano e ricaricano

Ogni attivo dichiara uno tra:

- **cariche**: un numero finito di usi che si ricaricano con una fonte esterna (vedi
  "Ricarica a doppio canale" sotto);
- **cooldown**: un tempo di attesa fisso dopo l'uso, indipendente da azioni successive.

Il metodo di ricarica scelto è parte della fantasia dell'oggetto e va dichiarato
esplicitamente, non lasciato implicito.

### Ricarica a doppio canale (DEC-059)

Gli attivi a **cariche** si ricaricano attraverso **due canali di base**, sempre attivi:

1. **completando stanze**;
2. **raccogliendo energia droppata dai nemici**.

Il dosaggio esatto (quanta carica per stanza, quanta energia serve, quanto ne droppa un
nemico) fa parte del budget di potenza dell'oggetto e resta specifico di ciascun attivo,
non un valore unico globale.

Oltre ai due canali di base, alcuni oggetti possono **aggiungere** modi di ricarica
ulteriori (es. danno inflitto, tempo, un evento specifico): il sistema è estensibile per
design, anche da contenuti generati, sempre dentro la validazione descritta in
[Generated Content Validation](generated-content-validation.md) (rimando, non
riformulato qui).

## Risultato

L'effetto dichiarato si applica al momento dell'attivazione, rispettando il budget di
potenza e le eventuali interazioni con altri attivi.

## Feedback

- stato disponibile/in ricarica sempre visibile;
- conferma immediata dell'attivazione;
- comportamento chiaro e comunicato se l'oggetto non può essere usato in quel momento
  (condizione d'uso non soddisfatta, in ricarica, ecc.).

## Interazioni

- con altri oggetti attivi (se più slot attivi sono posseduti);
- con le sinergie implicite e la fusione esplicita, vedi [Synergies](synergies.md);
- persistenza tra stanze e piani: un attivo raccolto resta nella build per tutta la run,
  salvo effetti espliciti che lo rimuovano.

## Regole per contenuti generati

Un attivo generato deve comunque dichiarare input di attivazione, cariche o cooldown,
condizioni d'uso, effetto, feedback e comportamento di fallback prima di poter essere
`approvato-per-run` (vedi [Generated Content Validation](generated-content-validation.md)).
Il budget di potenza limita l'entità dell'effetto proposto dal modello.

## Casi limite

- Attivazione richiesta mentre l'oggetto è in ricarica: nessun effetto, feedback chiaro.
- Attivazione senza bersaglio o condizione valida: nessun effetto, feedback chiaro,
  nessuna perdita di carica se la condizione d'uso non è mai stata soddisfatta.
- Più slot attivi occupati da oggetti con effetti che si sovrappongono sulla stessa
  proprietà: si applicano le regole di priorità descritte in
  [Synergies](synergies.md#priorità).

## Fallback

Se un attivo generato non supera la validazione in tempo utile, si applica la regola
unica descritta in [Generated Content Validation](generated-content-validation.md): non
riformulata qui.

## Non-obiettivi

- Questo documento non definisce singoli effetti o numeri di bilanciamento specifici.
- Non ridefinisce i campi obbligatori dell'oggetto: vivono in
  [Items, Pools and Rarity](items-pools-and-rarity.md).

## Raccolta e scambio: i piedistalli (DEC-117)

Gli attivi offerti dal gioco (stanza del tesoro, negozio, drop) **fluttuano su un
piedistallo**. Raccogliere un attivo nuovo lo mette nell'inventario — visibile nella
sezione della UI dedicata agli attivi — e, se gli slot sono pieni, **l'attivo che
possedevi finisce sul piedistallo al suo posto** — con più slot pieni, quello
**attualmente selezionato per l'attivazione** (DEC-117): lo scambio è sempre reversibile
finché resti nella stanza, perché il vecchio attivo non sparisce, fluttua lì. Con uno slot
libero la raccolta riempie lo slot senza scambio. Gap di implementazione esplicito: il
piedistallo di scambio non è ancora implementato.

## Domande aperte residue

- Numero massimo di slot attivi ottenibili in una run.
- Dosaggio esatto della ricarica a doppio canale (quanta carica per stanza, quanta
  energia per nemico) — DEC-059 fissa solo i due canali di base, non i numeri.

## Scenari verificabili

### Scenario 1 — attivazione riuscita

Given un attivo con cariche disponibili e condizione d'uso soddisfatta,  
When il giocatore preme l'input di attivazione,  
Then l'effetto si applica, una carica viene consumata e il feedback conferma
l'attivazione.

### Scenario 2 — attivazione in ricarica

Given un attivo ancora in cooldown,  
When il giocatore preme l'input di attivazione,  
Then nessun effetto si applica e il feedback comunica lo stato "in ricarica" senza
ambiguità.

### Scenario 3 — secondo slot attivo sbloccato

Given un giocatore che ha ottenuto un evento raro che aggiunge uno slot attivo,  
When raccoglie un secondo oggetto attivo,  
Then entrambi gli attivi restano disponibili in slot separati e il giocatore può
attivarli indipendentemente.

### Scenario 4 — ricarica a doppio canale

Given un attivo a cariche con carica non piena,  
When il giocatore completa una stanza oppure raccoglie energia droppata da un nemico,  
Then la carica dell'attivo aumenta secondo il dosaggio dichiarato dall'oggetto, indipendentemente da quale dei due canali di base ha attivato la ricarica (DEC-059).
