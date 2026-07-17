---
id: gd-system-bosses
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Boss come culmine del piano; sconfiggere il boss del piano 5 chiude la run ufficiale (DEC-006), con proseguimento facoltativo in piani extra degenerati. Bande di potenza e pesi di rarità del pool boss restano default draft (DEC-019)."
---

# Bosses

## Intento per il giocatore

Il boss verifica abilità e decisioni sviluppate nel piano, senza dipendere da una singola build obbligatoria, e segna il ritmo dell'obiettivo di vittoria della run.

## Condizioni di ingresso

- Il giocatore raggiunge la stanza boss dopo aver completato le condizioni previste dal piano (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).
- Il boss del piano 5 è l'ultimo boss della run standard: la sua sconfitta chiude la run ufficiale (DEC-006).

## Input/azioni

Il giocatore usa la build corrente, movimento libero e sparo nelle quattro direzioni per superare fasi e pattern del boss (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Struttura minima

- identità e tema;
- arena;
- fasi;
- pattern principali;
- transizioni leggibili;
- contromisure;
- ricompensa;
- condizioni anti-stallo;
- fallback.

## Risultato (DEC-006)

- **Boss dei piani 1-4:** la sconfitta apre l'uscita verso il piano successivo.
- **Boss del piano 5:** la sconfitta chiude la run ufficiale, valida per classifiche e vittoria. Da qui il giocatore sceglie se:
  - fermarsi e concludere la run con il risultato ottenuto; oppure
  - proseguire in piani extra, in continuità con lo stesso tema ma sempre più degenerati, oltre il piano 5, finché non muore.
- **Morte, in qualunque piano incluso un piano extra:** salute a zero significa run persa (permadeath). L'ordine di consumo tra salute temporanea e salute base è definito in [health-and-resources.md](./health-and-resources.md); questo documento non lo ripete.
- I piani extra oltre il piano 5 non contano ai fini della vittoria ufficiale né delle classifiche standard: sono un proseguimento facoltativo del rischio, successivo a una vittoria già acquisita.

## Feedback

- transizioni di fase leggibili e distinte;
- segnale chiaro alla sconfitta del boss del piano 5 (chiusura della run ufficiale) e scelta esplicita se proseguire oltre;
- indicazione che i piani extra sono oltre l'obiettivo ufficiale, per evitare ambiguità sulla vittoria già ottenuta.

## Interazioni

- con il tema di run, che degenera progressivamente fino al boss del piano 5 e oltre, nei piani extra;
- con la build del giocatore, senza dipendenza da una combinazione obbligatoria;
- con la ricompensa (vedi [rewards-and-economy.md](./rewards-and-economy.md)).

## Regole per contenuti generati

Un boss può essere composto da moduli, ma deve apparire come un'entità coerente, non come una lista casuale di attacchi. Dichiara un'origine (curato | composto | variato | nuovo) e supera la validazione prevista.

- **Bande di potenza boss (DEC-019):** oggi i boss generati sono scalati entro una banda di potenza **[1.4–3.2]**. **Stato: draft, default proposto dall'implementazione attuale, da validare col playtest.**
- **Pesi di rarità del pool ricompense boss (DEC-019):** `{comune: 0, non-comune: 0, rara: 70, leggendaria: 30}`. **Stato: draft, default proposto.** La tassonomia generale di oggetti e rarità è definita in [items-pools-and-rarity.md](./items-pools-and-rarity.md); qui i pesi sono riportati solo nell'ottica della ricompensa boss, senza ridefinire la tassonomia.

## Limite

Il boss finale non deve introdurre contemporaneamente troppe regole mai viste nella run.

## Casi limite

- Il giocatore muore in un piano extra dopo aver già vinto ufficialmente al piano 5: la run resta vinta ai fini di classifica; la morte chiude solo il proseguimento facoltativo.
- Il boss generato per un piano extra eccede la banda di potenza [1.4–3.2]: va respinto o riscalato in validazione, come qualunque boss generato.
- Il giocatore abbandona la run prima di affrontare il boss del piano 5: nessuna vittoria ufficiale registrata.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni boss ha un pool curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce il budget di leggibilità del telegraph (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).
- Non definisce l'ordine di consumo salute (vedi [health-and-resources.md](./health-and-resources.md)).
- Non definisce la regola di fallback dei contenuti generati (vedi [generated-content-validation.md](./generated-content-validation.md)).
- Non ridefinisce la tassonomia oggetti e rarità (vedi [items-pools-and-rarity.md](./items-pools-and-rarity.md)).

## Domande aperte residue

- Valore finale delle bande di potenza boss e dei pesi di rarità dopo playtest (DEC-019).
- Regole esatte di scaling/degenerazione dei piani extra oltre il piano 5.

## Scenari

### Scenario 1 — Vittoria ufficiale al piano 5

Given un giocatore che raggiunge il boss del piano 5 con salute residua
When sconfigge il boss
Then la run si chiude come vittoria ufficiale, valida per classifiche, e il gioco offre la scelta di fermarsi o proseguire in un piano extra

### Scenario 2 — Morte in un piano extra

Given un giocatore che ha già vinto ufficialmente al piano 5 e ha scelto di proseguire
When la sua salute base scende a zero in un piano extra
Then la run termina in permadeath, ma il risultato di vittoria ufficiale ottenuto al piano 5 resta valido per la classifica

### Scenario 3 — Boss generato fuori banda

Given un boss generato per un piano con potenza calcolata sopra 3.2 (banda draft DEC-019)
When la validazione lo verifica
Then il boss è respinto o riscalato entro la banda prima di poter apparire nella run

### Scenario 4 — Ricompensa boss con pesi di rarità draft

Given la sconfitta di un boss
When il gioco genera la ricompensa dal pool pesato {comune: 0, non-comune: 0, rara: 70, leggendaria: 30}
Then l'oggetto assegnato è sempre di rarità rara o leggendaria, coerente con il default attuale (draft, da validare col playtest)
