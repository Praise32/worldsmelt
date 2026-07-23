---
id: gd-system-passive-items
title: Passive Items
domain: design
status: approved
authority: canonical
owner: design
summary: "Oggetti che modificano permanentemente la build nella run, una delle 4 categorie della tassonomia oggetti."
last_reviewed: 2026-07-17
last_verified_commit: 0ec60d0
topics: [oggetti, passivi, stacking, build, sinergie]
related: []
supersedes: []
source_files: []
---

# Passive Items

I passivi sono una delle 4 categorie della tassonomia oggetti (attivo, passivo,
stat-up, Innesto). I campi obbligatori di un oggetto sono definiti in
[Items, Pools and Rarity](items-pools-and-rarity.md): questo documento non li
riformula, aggiunge solo le specificità del sottotipo passivo.

## Intento per il giocatore

Un passivo deve avere almeno un effetto distinguibile e descrivibile in una frase
semplice, riconoscibile nella build senza dover consultare formule.

## Slot

I passivi **non hanno limite di slot**: si accumulano tutti quelli raccolti nella run
(vedi [Items, Pools and Rarity](items-pools-and-rarity.md#slot)).

## Condizioni di ingresso

Un passivo entra in gioco nel momento in cui viene raccolto: non richiede alcuna azione
successiva del giocatore.

## Quando si applicano gli effetti

- L'effetto si applica **immediatamente** alla raccolta e resta attivo per tutta la
  durata della run, salvo effetti espliciti che lo rimuovano.
- Le proprietà modificate vengono ricalcolate ogni volta che la build cambia (nuovo
  passivo raccolto, sinergia formata o sciolta, fusione avvenuta): un passivo non
  "aspetta" un trigger del giocatore.

## Input/azioni

Nessun input diretto del giocatore: la sola azione è la raccolta.

## Risultato

Le proprietà dichiarate dal passivo vengono modificate in modo persistente, entro il
budget di potenza assegnato.

## Feedback

- trasformazione visiva coerente con l'effetto (vedi [Synergies](synergies.md) per la
  regola di composizione visiva quando più passivi/attivi interagiscono);
- l'effetto è elencato nella schermata build in una frase semplice.

## Stacking

Quando più copie o più passivi con effetto simile si accumulano, l'oggetto dichiara se
gli effetti si sommano, si moltiplicano o sono soggetti a un limite massimo. Questo è
un dettaglio del singolo oggetto, non un campo obbligatorio elencato in
[Items, Pools and Rarity](items-pools-and-rarity.md), ma va sempre dichiarato quando
pertinente.

## Interazioni

- con le sinergie implicite e la fusione esplicita, vedi [Synergies](synergies.md);
- con l'`incompatibilità` dichiarata nei campi obbligatori: un passivo incompatibile con
  un altro elemento della build non può coesistere con esso.

## Regole per contenuti generati

Un passivo generato deve dichiarare comunque proprietà modificate, tag di sinergia,
eventuale stacking, limiti, incompatibilità e valore nel budget di potenza prima di
poter essere `approvato-per-run` (vedi
[Generated Content Validation](generated-content-validation.md)).

## Casi limite

- Due passivi che modificano la stessa proprietà in direzioni opposte: si applicano le
  regole di priorità descritte in [Synergies](synergies.md#priorità).
- Passivo che supererebbe il limite di stacking dichiarato: l'eccesso non ha effetto
  aggiuntivo, ma resta comunque nella build (nessun oggetto viene "sprecato" o perso).

## Fallback

Se un passivo generato non supera la validazione in tempo utile, si applica la regola
unica descritta in [Generated Content Validation](generated-content-validation.md): non
riformulata qui.

## Non-obiettivi

- Questo documento non definisce singoli effetti o numeri di bilanciamento specifici.
- Non ridefinisce i campi obbligatori dell'oggetto: vivono in
  [Items, Pools and Rarity](items-pools-and-rarity.md).

## Domande aperte residue

- Limiti generali di stacking applicabili di default ai passivi generati (oltre a quelli
  dichiarati oggetto per oggetto).

## Scenari verificabili

### Scenario 1 — raccolta e applicazione immediata

Given un giocatore senza quel passivo nella build,  
When raccoglie l'oggetto,  
Then l'effetto si applica immediatamente e la schermata build lo elenca senza bisogno
di ulteriori azioni.

### Scenario 2 — stacking di due copie

Given un giocatore che possiede già una copia di un passivo con stacking dichiarato,  
When raccoglie una seconda copia dello stesso passivo,  
Then l'effetto si somma o si moltiplica secondo quanto dichiarato dall'oggetto, entro il
limite eventualmente indicato.

### Scenario 3 — conflitto tra due passivi sulla stessa proprietà

Given due passivi che modificano la stessa proprietà in direzioni opposte,  
When entrambi sono presenti nella build,  
Then il risultato segue le regole di priorità descritte in
[Synergies](synergies.md#priorità), non un ordine arbitrario di raccolta.
