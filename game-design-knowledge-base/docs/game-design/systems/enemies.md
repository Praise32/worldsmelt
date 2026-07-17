---
id: gd-system-enemies
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Grammatica degli avversari generati o curati, incluso il Veterano (nemico potenziato non-boss). Bande di potenza (DEC-019) restano un default draft da validare col playtest."
---

# Enemies

## Intento per il giocatore

Ogni nemico deve essere leggibile a colpo d'occhio: ruolo, minaccia e finestra di risposta riconoscibili anche se il nemico è generato dall'IA e mai visto prima nella run.

## Condizioni di ingresso

- Il nemico appare in una stanza di combattimento, in un'arena di sfida (vedi [special-rooms.md](./special-rooms.md)) o come parte di un evento generato.
- Il piano e il tema correnti determinano il pool di ruoli e tag disponibili (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).

## Componenti di un nemico

- ruolo tattico;
- movimento;
- attacco;
- telegraph;
- finestra di vulnerabilità;
- salute o resistenza;
- interazioni ambientali;
- ricompensa;
- tag visivi e di difficoltà.

## Ruoli iniziali

- inseguitore;
- tiratore;
- controllore dello spazio;
- supporto;
- evocatore;
- ostacolo mobile;
- **Veterano** — variante potenziata di un ruolo base (nome placeholder canonico, sostituisce "élite"). Non è un boss: non ha fasi né arena dedicata (vedi [bosses.md](./bosses.md)).

## Input/azioni

Il giocatore osserva pattern e telegraph, si muove liberamente, spara nelle quattro direzioni cardinali e usa risorse per creare spazio o vantaggio.

## Risultato

- Nemico sconfitto: eroga una ricompensa secondo [rewards-and-economy.md](./rewards-and-economy.md).
- Nemico non sconfitto: mantiene pressione sulla stanza fino a completamento o fuga del giocatore.

## Feedback

- telegraph visivo/sonoro prima di ogni attacco pericoloso;
- reazione visibile a colpo subito;
- segnale di sconfitta chiaro;
- il Veterano ha un tag visivo distintivo che lo separa dalla versione base dello stesso ruolo.

## Interazioni

- con ostacoli e distruttibili (vedi [secrets-and-obstacles.md](./secrets-and-obstacles.md));
- con altri nemici nella stessa stanza: le stanze devono evitare combinazioni senza spazio di risposta;
- con la generazione del piano corrente (tema, budget di difficoltà, vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)).

## Regole per contenuti generati

- Non combinare più ruoli complessi senza aumentare il budget.
- Ogni attacco pericoloso deve avere telegraph: il budget di leggibilità di un attacco è definito in [combat-and-projectiles.md](./combat-and-projectiles.md); questo documento non lo ridefinisce.
- La silhouette deve comunicare almeno il ruolo dominante.
- **Bande di potenza (DEC-019):** i nemici generati vengono oggi scalati entro una banda di potenza **[0.7–1.35]** rispetto al nemico base di riferimento. **Stato: draft, default proposto dall'implementazione attuale, da validare col playtest** — non è una decisione di design chiusa.
- Il Veterano occupa la fascia alta della stessa banda, o una variante dichiarata dal generatore, restando comunque un nemico potenziato non-boss.
- Ogni nemico generato dichiara un'origine (curato | composto | variato | nuovo) e supera la validazione prevista prima di apparire in una run standard: la regola di garanzia entro cui l'IA inventa nemici è descritta in [generated-content-validation.md](./generated-content-validation.md), non qui.

## Casi limite

- Nemico generato con ruoli incompatibili nella stessa stanza: la stanza va respinta o rigenerata prima di essere proposta al giocatore.
- Nemico Veterano generato in una stanza incompatibile con il suo pattern: la generazione deve rispettare la compatibilità dichiarata dalla stanza.
- Attacco senza telegraph rilevato in validazione: il nemico non è approvato per la run.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di nemico ha un pool curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce il budget di leggibilità (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).
- Non definisce la regola di fallback dei contenuti generati (vedi [generated-content-validation.md](./generated-content-validation.md)).
- Non tratta i boss, che hanno documento dedicato (vedi [bosses.md](./bosses.md)).

## Domande aperte residue

- Valore finale delle bande di potenza nemico e Veterano dopo playtest (DEC-019).
- Numero massimo di Veterano contemporanei per stanza.

## Scenari

### Scenario 1 — Telegraph di un Veterano

Given un Veterano generato con attacco a raffica
When entra nella finestra d'attacco visibile al giocatore
Then mostra un telegraph riconoscibile prima di sparare, secondo il budget di leggibilità di [combat-and-projectiles.md](./combat-and-projectiles.md)

### Scenario 2 — Nemico fuori banda di potenza

Given un nemico generato con potenza calcolata sopra 1.35 (banda draft DEC-019)
When la validazione lo verifica
Then il nemico è respinto o riscalato entro la banda prima di poter apparire in una run standard

### Scenario 3 — Combinazione senza spazio di risposta

Given una stanza con due o più nemici generati
When la combinazione non lascia spazio di risposta al giocatore
Then la generazione è respinta o la stanza viene rigenerata

### Scenario 4 — Fallback su categoria di nemico

Given una categoria di nemico richiesta dal piano corrente senza contenuto generato approvato
When la run deve comunque proseguire
Then si usa il pool curato di fallback, secondo [generated-content-validation.md](./generated-content-validation.md), senza bloccare la partita
