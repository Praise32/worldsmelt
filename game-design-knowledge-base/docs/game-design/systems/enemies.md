---
id: gd-system-enemies
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Grammatica degli avversari generati o curati, incluso il Veterano (nemico potenziato non-boss). Grado ed escalation per piano (DEC-024): Veterani più frequenti nei piani alti. Bande di potenza (DEC-019) restano un default draft da validare col playtest. Il tema genera un roster compatto di 6-8 tipi di nemici per l'intera run, distribuiti sui piani (DEC-053). Il danno da contatto è dichiarato dalla forma: solo i nemici la cui silhouette lo telegrafa feriscono al contatto, gli altri spingono soltanto (DEC-061)."
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
- **Veterano** — variante potenziata di un ruolo base (termine di lavoro; in-game: **Tempered**, DEC-072). Non è un boss: non ha fasi né arena dedicata (vedi [bosses.md](./bosses.md)).

## Roster compatto per run (DEC-053)

Il tema di una run non genera un numero illimitato di tipi di nemici: genera un **roster
compatto di 6-8 tipi di nemici** per l'intera run, distribuiti sui cinque piani e potenziati
dalla degenerazione del tema (gradi crescenti, DEC-024; vedi anche il Veterano sopra).
Questi 6-8 tipi sono istanze concrete generate dal tema, distinte dai **ruoli** elencati
sopra (la tassonomia tattica fissa): più tipi generati possono condividere lo stesso ruolo.

Un roster compatto rende l'**apprendimento dei pattern** parte del design: il giocatore
rivede lo stesso piccolo insieme di nemici, sempre più potenziati, invece di un flusso
continuo di nemici mai visti (coerente con la difficoltà unica del gioco, senza livelli
selezionabili — vedi [Difficulty and Progression](../07-difficulty-and-progression.md),
DEC-038).

## Escalation per piano (DEC-024)

Il tema del piano non si limita a colorare i nemici: piano dopo piano gli stessi ruoli si
esprimono con un **grado crescente** dentro il tema (es. un tema "fantasy medievale" mostra
cavalieri di grado infimo al piano 1 e cavalieri esperti al piano 5), non con ruoli
scollegati dal tema. Questo documento non ridefinisce il principio generale di escalation
leggibile, che vive in
[Difficulty and Progression](../07-difficulty-and-progression.md); qui si registra solo la
conseguenza specifica per i nemici:

- il **Veterano** compare con **frequenza crescente** nei piani alti;
- il grado dei nemici generati sale con il piano, restando sempre dentro le bande di
  potenza dichiarate e il budget di leggibilità di
  [Combat and Projectiles](./combat-and-projectiles.md).

## Danno da contatto dichiarato dalla forma (DEC-061)

Non tutti i nemici feriscono al contatto fisico. Solo i nemici la cui **forma lo telegrafa
visivamente** (es. spine, corpi ustionanti) infliggono danno al contatto; gli **altri**
nemici, al contatto, **spingono ma non feriscono**. La lettura visiva decide, non una
proprietà nascosta: se la silhouette di un nemico non comunica un pericolo di contatto, quel
nemico non può ferire toccando il giocatore.

Questa regola si aggancia a due fonti uniche, senza riformularle:

- il **vocabolario delle forme dei nemici**, in particolare la regola secondo cui "la
  silhouette deve comunicare almeno il ruolo dominante" (vedi "Regole per contenuti
  generati" sotto, e i 7 strati di trasformazione visiva in
  [Visual Language](../content/visual-language.md), strato 1, silhouette);
- il **budget di leggibilità**, fonte unica
  [Combat and Projectiles](./combat-and-projectiles.md) (rimando, non riformulato qui).

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
- Se un nemico generato infligge danno al contatto, la sua silhouette deve telegrafarlo
  (es. spine, corpi ustionanti, ecc.); un nemico generato senza una forma che lo comunica
  spinge ma non ferisce al contatto (DEC-061).
- **Bande di potenza (DEC-019):** i nemici generati vengono oggi scalati entro una banda di potenza **[0.7–1.35]** rispetto al nemico base di riferimento. **Stato: draft, default proposto dall'implementazione attuale, da validare col playtest** — non è una decisione di design chiusa.
- Il Veterano occupa la fascia alta della stessa banda, o una variante dichiarata dal generatore, restando comunque un nemico potenziato non-boss.
- Ogni nemico generato dichiara un'origine (curato | composto | variato | nuovo) e supera la validazione prevista prima di apparire in una run standard: la regola di garanzia entro cui l'IA inventa nemici è descritta in [generated-content-validation.md](./generated-content-validation.md), non qui.

## Casi limite

- Nemico generato con ruoli incompatibili nella stessa stanza: la stanza va respinta o rigenerata prima di essere proposta al giocatore.
- Nemico Veterano generato in una stanza incompatibile con il suo pattern: la generazione deve rispettare la compatibilità dichiarata dalla stanza.
- Attacco senza telegraph rilevato in validazione: il nemico non è approvato per la run.
- Il tema genera più di 8 tipi di nemici distinti: il pool va ridotto a 6-8 prima di essere approvato per la run, secondo [generated-content-validation.md](./generated-content-validation.md).
- Un nemico generato infligge danno al contatto ma la sua silhouette non lo telegrafa: la validazione lo respinge o richiede la correzione della forma prima di essere `approvato-per-run` (DEC-061).

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di nemico ha un pool curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce il budget di leggibilità (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).
- Non definisce la regola di fallback dei contenuti generati (vedi [generated-content-validation.md](./generated-content-validation.md)).
- Non tratta i boss, che hanno documento dedicato (vedi [bosses.md](./bosses.md)).

## Domande aperte residue

- Valore finale delle bande di potenza nemico e Veterano dopo playtest (DEC-019).
- Numero massimo di Veterano contemporanei per stanza.
- Tasso esatto di crescita della frequenza di Veterani per piano (DEC-024 fissa solo che
  cresce con il piano, non i numeri).
- Se il roster di 6-8 tipi (DEC-053) è fisso per l'intera run fin dall'inizio, o può essere
  esteso da contenuti "best-of" incontrati durante la run.

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

### Scenario 5 — Veterani più frequenti nei piani alti

Given due piani della stessa run, uno basso e uno alto
When il gioco genera i nemici per ciascun piano
Then la frequenza con cui compare il Veterano nel piano alto è maggiore di quella nel piano basso, coerente con DEC-024

### Scenario 6 — Roster compatto per l'intera run

Given una run con un tema scelto nel Piano 0
When il gioco genera i tipi di nemici della run
Then il roster generato conta tra 6 e 8 tipi di nemici in totale, distribuiti sui piani e potenziati piano dopo piano dalla degenerazione del tema, coerente con DEC-053

### Scenario 7 — Danno da contatto solo se telegrafato dalla forma

Given un nemico con una silhouette che mostra chiaramente spine o un corpo ustionante, e un nemico con una silhouette senza alcun elemento di pericolo di contatto
When il giocatore tocca ciascuno dei due nemici
Then il primo nemico infligge danno al contatto, il secondo spinge il giocatore ma non lo ferisce, coerente con DEC-061
