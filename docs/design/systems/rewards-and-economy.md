---
id: gd-system-rewards-economy
title: Rewards and Economy
domain: design
status: approved
authority: canonical
owner: design
summary: "Distribuzione di ricompense, uso economico della valuta principale (DEC-013) — guadagnata da nemici sconfitti e stanze ripulite, dove «ripulita» è qualunque stanza completata secondo la propria condizione (DEC-167), col negozio che ricompra oggetti e Innesti indesiderati a prezzo ridotto (DEC-048) —, negozio a prezzi fissi più offerta speciale (DEC-026), scambio ad alto rischio a puntata generata (DEC-044, dettaglio in special-rooms.md), ricompense a tempo nei piani avanzati (DEC-051, archetipo in special-rooms.md/rooms-and-floor-generation.md) e punti sblocco a doppio canale esclusivi al singleplayer (DEC-015, DEC-027), con presentazione delle prove specifiche al passaggio verso il piano 1 (DEC-042, dettaglio in floor-zero.md); pattern rischio/ricompensa dell'arena di sfida. Punteggio composito multi-percorso: somma bonus da tempo, prove, esplorazione, scoperte, eliminazioni e Veterani, con bonus di efficienza per chi completa in fretta ed esplorando poco, e bonus per chi esplora tutto (DEC-060)."
last_reviewed: 2026-07-27
last_verified_commit: 0ec60d0
topics: [economia, ricompense, negozio, valuta, punti-sblocco, punteggio, DEC-167]
related: []
supersedes: []
source_files: []
---

# Rewards and Economy

## Intento per il giocatore

Ogni ricompensa deve sembrare proporzionata a rischio, costo e rarità, e deve alimentare decisioni economiche significative durante la run, senza diventare accumulo fine a sé stesso.

## Condizioni di ingresso

Le ricompense sono distribuite al completamento di stanze, alla sconfitta di nemici e boss, e al superamento di sfide o scambi (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md), [special-rooms.md](./special-rooms.md), [bosses.md](./bosses.md), [enemies.md](./enemies.md)).

## Tipi di ricompensa

- salute;
- risorse (inclusa la valuta principale);
- oggetti (attivi, passivi, Innesti — vedi [items-pools-and-rarity.md](./items-pools-and-rarity.md));
- accesso;
- informazione;
- modifica temporanea;
- punteggio o progressione competitiva.

## Input/azioni

Il giocatore raccoglie, spende o scambia ricompense: acquista nel negozio, offre risorse in uno scambio ad alto rischio, o investe la valuta principale nelle occasioni offerte dalla run.

## La valuta principale (DEC-013)

La **valuta principale** (in-game: **Ingots**, DEC-072) sostituisce per funzione le "monete" del genere. Le regole generali della risorsa (cap massimo, ordine di consumo, visibilità in HUD) sono definite in [health-and-resources.md](./health-and-resources.md) come fonte unica; questo documento non le ripete, descrive solo l'**uso economico** e le **fonti canoniche**.

### Fonti canoniche della valuta principale (DEC-048)

La valuta principale si guadagna da **nemici sconfitti** e da **stanze ripulite** (completate): queste sono, per ora, le uniche fonti canoniche. Nessun'altra fonte è prevista in questa fase del progetto.

Per **stanza ripulita** si intende qualunque stanza completata secondo la **propria
condizione di completamento**, qualunque sia l'archetipo (DEC-167): non esiste
un'unica definizione di "ripulita" indipendente dal tipo di stanza. Esempi: una stanza di
combattimento standard è ripulita quando tutti i nemici sono sconfitti; una stanza a
tempo ([Special Rooms](./special-rooms.md), DEC-051) è ripulita quando il giocatore la
raggiunge entro la soglia richiesta; una stanza tesoro quando il tesoro è aperto, un
negozio quando è stato visitato, una stanza segreta quando è stata trovata (DEC-167). Ogni
archetipo di stanza definisce la propria condizione di completamento (vedi
[rooms-and-floor-generation.md](./rooms-and-floor-generation.md) e
[special-rooms.md](./special-rooms.md)); questo documento non la ridefinisce, registra
solo che qualunque condizione di completamento soddisfatta conta come "ripulita" ai fini
della valuta principale.

### Stato di implementazione (2026-07-27)

DEC-167 vive lato motore in `WorldAwardRoomCompletionCurrency`
(`src/world/world.c`), chiamata dal punto che rileva CIASCUNA condizione di
completamento — la funzione stessa non rileva nulla, assegna solo l'importo:

- **combattimento ripulito** e **boss sconfitto**: da `WorldCheckRoomClear`,
  subito dopo che `room->cleared` diventa vero (guardia contro il doppio
  pagamento rientrando in una stanza già ripulita già presente su quel
  campo);
- **tesoro aperto**: da `CombatPickup` (`src/gameplay/combat.c`), quando
  l'oggetto della stanza tesoro viene preso per la prima volta
  (`room->rewardTaken` false → vero — non quando lo scambia sul piedistallo
  una seconda volta);
- **negozio visitato**: da `WorldSpawnRoomContents` (`src/world/world.c`),
  alla PRIMA volta che `room->visited` diventa vero — il negozio paga
  all'ingresso, non all'acquisto (quello resta un evento economico separato,
  DEC-026/DEC-048);
- **stanza segreta trovata** e **stanza a tempo**: non hanno ancora un
  `RoomKind` nel motore (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)),
  quindi DEC-167 non ha ancora un punto di innesto per loro; la tavola dei
  compensi (sotto) li ignora per costruzione fino a quel momento.

Importi per tipo di stanza, **default proposti dall'implementazione (stile
DEC-019)**: nessun documento fissa i numeri, solo che la fonte esiste per
ogni archetipo (DEC-167) — questi restano da confermare col playtest.

| Archetipo | Ingots |
|---|---|
| combattimento | 4 |
| boss | 12 |
| tesoro | 3 |
| negozio | 2 |

Il boss vale più di un combattimento normale (è la stanza più impegnativa del
piano); tesoro e negozio meno di un combattimento perché non richiedono di
sopravvivere a nulla — coerente col §Principio sopra. Con il budget di celle
per piano introdotto da DEC-170 (`6 + numero_piano + estrazione(0..3)` celle,
tipicamente ~5-9 stanze più boss/tesoro/negozio/partenza per piano, vedi
[rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), un piano
tipico frutta approssimativamente 20-40 Ingots solo da DEC-167 (4-6
combattimenti + 1 tesoro + 1 negozio + 1 boss), sufficienti a coprire almeno
un acquisto comune (8 Ingots, [items-pools-and-rarity.md](./items-pools-and-rarity.md))
per piano anche senza contare le monete sparse dai nemici — coerente col caso
limite "almeno un'occasione di spesa significativa per piano" (sopra). Verificato
da `--economy-test` (`make test`): i quattro importi, la mancata doppia
assegnazione rientrando/richiamando su una stanza già completata, e la
consegna vera dell'oggetto tesoro.

L'**uso economico** della valuta principale:

- acquisti nel negozio (stanza standard, vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md));
- partecipazione a scambi nella stanza di scambio ad alto rischio (vedi [special-rooms.md](./special-rooms.md));
- eventuale pagamento parziale di costi in altre stanze speciali, quando previsto dal loro contratto.

## Negozio: prezzi fissi e offerta speciale (DEC-026)

Il negozio (stanza standard) usa **prezzi base fissi per fascia di rarità**: il giocatore
impara nel tempo il valore delle cose, invece di dover ricalcolare un prezzo variabile ogni
volta. Ogni negozio propone inoltre **una sola offerta speciale generata** (sconto,
pacchetto o oggetto legato al tema della run). **Non esistono "patti" a costo salute nel
negozio**: cedere salute in cambio di un guadagno resta esclusivo della stanza di scambio
ad alto rischio (vedi [Special Rooms](./special-rooms.md)). Questo risolve la domanda di
design precedentemente aperta su prezzi dinamici e costi in salute nel negozio.

Nella stanza di scambio ad alto rischio, offerta e prezzo sono generati dall'IA dentro un
budget di equità (DEC-044): fonte unica del dettaglio è [Special Rooms](./special-rooms.md),
non riformulato qui.

### Ricompra nel negozio (DEC-048)

Oltre a vendere, il negozio **ricompra** oggetti e Innesti indesiderati dal giocatore, a un
prezzo ridotto rispetto al valore di acquisto della stessa fascia di rarità (DEC-026 fissa i
prezzi base fissi per fascia; DEC-048 aggiunge il lato ricompra). Questa è, per ora, l'unica
via per convertire oggetti indesiderati in valuta principale durante la run. Non è un patto a
costo salute: resta un'operazione economica ordinaria del negozio, distinta dallo scambio ad
alto rischio (vedi [Special Rooms](./special-rooms.md)).

## Risultato

Una ricompensa raccolta entra nell'inventario, nella salute o nella valuta principale del giocatore secondo il tipo; una spesa riduce la risorsa usata e sblocca l'accesso, l'oggetto o l'effetto pagato.

## Feedback

- indicazione chiara del prezzo/costo prima della conferma di un acquisto o scambio;
- distinzione visiva tra valore nominale e valore reale per la build corrente;
- segnale quando una ricompensa proviene da un contesto ad alto rischio (arena di sfida, scambio ad alto rischio).

## Principio

La ricompensa deve essere proporzionata a rischio, costo e rarità, considerando anche il valore della sinergia per la build corrente.

## Pattern rischio/ricompensa dell'arena di sfida

L'arena di sfida ([special-rooms.md](./special-rooms.md), DEC-010) è una fonte dichiarata di rischio-ricompensa: propone contenuti "best-of" più impegnativi (anche in versione Piano 0, vedi [floor-zero.md](./floor-zero.md)) in cambio di ricompense superiori alla media di una stanza equivalente non a rischio. Il dettaglio dell'archetipo — accesso, costo, frequenza — è definito in [special-rooms.md](./special-rooms.md); questo documento descrive solo il pattern economico che ne deriva.

## Ricompense delle stanze a tempo (DEC-051)

Il tempo di run è sempre visibile nell'HUD (vedi [HUD](../ui/hud.md), DEC-051): il gioco si dichiara esplicitamente una corsa. Coerente con questo, nei piani avanzati esistono stanze fisse — l'archetipo "stanza a tempo", descritto in [Rooms and Floor Generation](./rooms-and-floor-generation.md) e [Special Rooms](./special-rooms.md) — che danno una ricompensa aggiuntiva se il giocatore le raggiunge entro una soglia di tempo. Questo documento descrive solo la ricompensa; l'archetipo di stanza (accesso, posizione, segnale) è definito nei documenti citati, non riformulato qui.

Soglia di tempo e valore esatto della ricompensa sono da definire col playtest (stile DEC-019): questa decisione fissa solo che la ricompensa esiste ed è bilanciata col resto del gameplay, non i numeri.

## Meta-progressione e punti sblocco (DEC-015, DEC-027)

I **punti sblocco** si guadagnano giocando in singleplayer e sono spendibili per sbloccare contenuti generati nei pool delle run future. Sono **esclusi dalle modalità competitive**. Il dettaglio della meta-progressione (catalogo, museo del Piano 0, cosa persiste) è definito in [save-and-meta-progression.md](./save-and-meta-progression.md) come fonte unica; questo documento non lo ripete, registra solo che:

- i punti sblocco sono un tipo di ricompensa/progressione guadagnata in singleplayer;
- non hanno valore o effetto nelle gare asincrone o in altre modalità competitive.

### Doppio canale di guadagno (DEC-027)

I punti sblocco a fine run si compongono di due canali:

- **punti base**, assegnati in funzione del risultato della run (piani completati, boss
  sconfitti, scoperte fatte);
- **bonus da prove specifiche**, fisse o generate (esempi: sconfiggere un boss senza
  subire danni, trovare 2 stanze segrete, completare un'arena di sfida).

Resta valido solo in singleplayer (DEC-015 invariata): nessun canale di questo doppio
sistema si applica alle modalità competitive.

### Presentazione delle prove specifiche (DEC-042, rimando)

Le prove specifiche del canale bonus vengono presentate al giocatore al passaggio dal Piano
0 al piano 1 e restano sempre consultabili dal menu di pausa e dalla schermata build; fonte
unica del comportamento di presentazione: [Floor Zero](./floor-zero.md). Questo documento non
lo ripete, definisce solo che le prove sono la fonte del bonus punti descritto sopra.

## Punteggio composito multi-percorso (DEC-060)

Il **punteggio** è una delle metriche di classifica (vedi
[Results and Leaderboards](../ui/results-and-leaderboards.md) e
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md), DEC-062,
rimando) ed è distinto dai punti sblocco (DEC-015, DEC-027, sopra): il punteggio misura la
prestazione della run, i punti sblocco alimentano la meta-progressione.

Il punteggio si compone sommando **piccoli bonus** da sei fonti:

- tempo;
- prove/sfide superate;
- esplorazione;
- scoperte (nuovi contenuti generati incontrati);
- eliminazioni;
- Veterani sconfitti.

### Vincolo di bilanciamento: percorsi diversi restano competitivi

Il proprietario ha fissato un vincolo esplicito: **percorsi di gioco diversi devono
restare competitivi tra loro** nel punteggio finale.

- Chi completa i 5 piani con il **minor numero di stanze visitate** e nel **minor tempo**
  riceve un **bonus di efficienza**.
- Chi **esplora tutto** e impiega più tempo accumula comunque bonus per tutto ciò che fa
  (esplorazione, scoperte, eliminazioni, prove), senza essere penalizzato rispetto a un
  percorso rapido.

Il bilanciamento fine tra questi bonus — quanto vale ciascuna fonte, come si equivalgono
un percorso rapido ed efficiente e un percorso lento ed esaustivo — è **da playtest**
(domanda aperta, vedi sotto), non fissato da questa decisione.

## Interazioni

- con il negozio e gli scambi ([rooms-and-floor-generation.md](./rooms-and-floor-generation.md), [special-rooms.md](./special-rooms.md));
- con la salute e le altre risorse ([health-and-resources.md](./health-and-resources.md));
- con la tassonomia oggetti e rarità ([items-pools-and-rarity.md](./items-pools-and-rarity.md));
- con la meta-progressione ([save-and-meta-progression.md](./save-and-meta-progression.md));
- con la presentazione delle prove specifiche all'ingresso nel piano 1 ([floor-zero.md](./floor-zero.md), DEC-042);
- con l'archetipo "stanza a tempo" ([rooms-and-floor-generation.md](./rooms-and-floor-generation.md), [special-rooms.md](./special-rooms.md), DEC-051) e col timer sempre visibile ([hud.md](../ui/hud.md));
- con la classifica a punteggio, che usa il punteggio composito qui definito come metrica separata dalla classifica a tempo ([results-and-leaderboards.md](../ui/results-and-leaderboards.md), [08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md), DEC-062).

## Regole per contenuti generati

Ogni ricompensa generata (oggetto, effetto, quantità di valuta) dichiara un'origine (curato | composto | variato | nuovo) e rispetta il principio di proporzione tra rischio/costo e valore.

## Protezioni

- evitare lunghi periodi senza scelte significative;
- limitare loop economici infiniti;
- impedire duplicazioni non previste;
- distinguere valore nominale e valore reale per la build.

## Casi limite

- Un giocatore accumula valuta principale senza occasioni di spesa nel piano corrente: la generazione deve garantire almeno un'occasione di spesa significativa per piano.
- Uno scambio ad alto rischio propone un costo che il giocatore non può permettersi: la stanza non deve bloccare il progresso (vedi [special-rooms.md](./special-rooms.md), casi limite).
- I punti sblocco vengono guadagnati durante una sessione che poi entra in una modalità competitiva: restano registrati come singleplayer e non si applicano alla sessione competitiva in corso.
- Una prova specifica generata per il bonus punti sblocco (es. "boss senza danni") risulta impossibile da verificare per un difetto di generazione: la prova va scartata o corretta in validazione prima di essere proposta, e non deve mai negare i punti base già maturati.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di ricompensa ha un pool curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Non-obiettivi

- Non definisce le regole generali della valuta principale come risorsa (vedi [health-and-resources.md](./health-and-resources.md)).
- Non dettaglia la meta-progressione oltre l'uso dei punti sblocco (vedi [save-and-meta-progression.md](./save-and-meta-progression.md)).
- Non ridefinisce la tassonomia oggetti (vedi [items-pools-and-rarity.md](./items-pools-and-rarity.md)) né l'archetipo arena di sfida (vedi [special-rooms.md](./special-rooms.md)).

## Domande aperte residue

- Valori esatti dei prezzi fissi per fascia di rarità nel negozio (DEC-026 fissa il
  modello "prezzi fissi", non i numeri).
- Tasso esatto di guadagno dei punti base e dei bonus da prova specifica (DEC-027 fissa la
  struttura a doppio canale, non i numeri).
- Elenco completo delle prove specifiche (fisse e generate) che danno bonus punti sblocco.
- Prezzi e range esatti degli scambi nella stanza ad alto rischio.
- Quanti "slot" di spesa significativa devono esistere per piano.
- Valori esatti di soglia e ricompensa delle stanze a tempo (DEC-051, da playtest).
- Bilanciamento fine del punteggio composito multi-percorso: peso relativo di tempo,
  prove, esplorazione, scoperte, eliminazioni e Veterani, e come si equivalgono un
  percorso rapido/efficiente e un percorso lento/esaustivo (DEC-060 fissa solo le fonti e
  il vincolo di competitività tra percorsi, non i numeri; da playtest).

## Scenari

### Scenario 1 — Acquisto nel negozio con valuta principale

Given un giocatore con valuta principale sufficiente
When acquista un oggetto nel negozio
Then la valuta si riduce del prezzo indicato e l'oggetto entra nell'inventario, secondo l'ordine di consumo definito in [health-and-resources.md](./health-and-resources.md)

### Scenario 2 — Ricompensa da arena di sfida

Given un giocatore che completa un'arena di sfida durante un piano
When riceve la ricompensa finale
Then il valore della ricompensa è superiore alla media di una stanza di combattimento equivalente non a rischio, coerente con il pattern rischio/ricompensa descritto qui e con il contratto dell'archetipo in [special-rooms.md](./special-rooms.md)

### Scenario 3 — Punti sblocco esclusi dal competitivo

Given un giocatore che ha accumulato punti sblocco in sessioni singleplayer
When partecipa a una gara asincrona competitiva
Then i punti sblocco e i contenuti sbloccati non hanno effetto nella sessione competitiva

### Scenario 4 — Scambio ad alto rischio non sostenibile

Given un giocatore con risorse insufficienti per completare uno scambio ad alto rischio proposto
When valuta le opzioni nella stanza
Then può uscire senza penalità, senza che la progressione della run sia bloccata

### Scenario 5 — Negozio con prezzo fisso e offerta speciale

Given un giocatore che entra in due negozi diversi nella stessa run
When confronta i prezzi base di un oggetto della stessa fascia di rarità nei due negozi
Then i prezzi base coincidono, mentre ciascun negozio propone una propria offerta speciale generata diversa dall'altro

### Scenario 6 — Punti sblocco dal doppio canale a fine run

Given un giocatore che completa una run singleplayer sconfiggendo un boss senza subire danni e trovando 2 stanze segrete
When la run termina e i punti sblocco vengono calcolati
Then il totale include sia i punti base per il risultato della run sia i bonus per le prove specifiche completate (DEC-027)

### Scenario 7 — Ricompra nel negozio

Given un giocatore con un oggetto che non vuole più tenere
When lo vende al negozio
Then riceve valuta principale a un prezzo ridotto rispetto al valore di acquisto della stessa fascia di rarità (DEC-048)

### Scenario 8 — Ricompensa da una stanza a tempo

Given un giocatore che raggiunge una stanza a tempo di un piano avanzato entro la soglia richiesta
When riceve la ricompensa della stanza
Then ottiene un premio aggiuntivo bilanciato col gameplay, i cui valori esatti di soglia e ricompensa restano da definire col playtest (DEC-051)

### Scenario 9 — Percorso rapido con bonus di efficienza

Given un giocatore che completa i 5 piani visitando il minor numero di stanze possibile e nel minor tempo
When il punteggio finale viene calcolato
Then riceve un bonus di efficienza che si somma agli altri bonus del punteggio composito, restando competitivo con un percorso più esplorativo (DEC-060)

### Scenario 10 — Percorso esplorativo con bonus cumulativi

Given un giocatore che esplora ogni stanza raggiungibile di ogni piano, impiegando più tempo di un percorso rapido
When il punteggio finale viene calcolato
Then accumula bonus per l'esplorazione, le scoperte, le eliminazioni e i Veterani sconfitti incontrati lungo il percorso, restando competitivo col punteggio di un percorso rapido ed efficiente (DEC-060)

### Scenario 11 — Valuta da una stanza a tempo ripulita secondo la propria condizione

Given un giocatore raggiunge una stanza a tempo entro la soglia richiesta dal suo archetipo
When la stanza risulta completata secondo la propria condizione
Then il giocatore guadagna valuta principale come da qualunque altra stanza ripulita, perché "ripulita" dipende dalla condizione di completamento specifica di quell'archetipo, non da una regola unica (DEC-167)
