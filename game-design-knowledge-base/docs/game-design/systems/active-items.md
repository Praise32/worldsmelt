---
id: gd-system-active-items
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Oggetti attivabili volontariamente dal giocatore, una delle 4 categorie della tassonomia oggetti."
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

Un attivo entra in gioco quando viene raccolto e assegnato a uno slot attivo libero (o
sostituisce volontariamente l'occupante dello slot, se il gioco lo consente).

## Input/azioni

- input di attivazione dedicato (esplicito, non automatico);
- condizioni d'uso (es. richiede bersaglio, richiede stanza non in transizione, ecc.).

## Come si attivano e ricaricano

Ogni attivo dichiara uno tra:

- **cariche**: un numero finito di usi che si ricaricano con una fonte esterna (tempo,
  danno inflitto, stanze superate, a seconda del design specifico dell'oggetto);
- **cooldown**: un tempo di attesa fisso dopo l'uso, indipendente da azioni successive.

Il metodo di ricarica scelto è parte della fantasia dell'oggetto e va dichiarato
esplicitamente, non lasciato implicito.

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

## Domande aperte residue

- Numero massimo di slot attivi ottenibili in una run.
- Se e come il giocatore possa riordinare o scambiare volontariamente gli attivi tra
  slot.

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
