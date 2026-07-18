---
id: gd-system-rewards-economy
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Distribuzione di ricompense, uso economico della valuta principale (DEC-013), negozio a prezzi fissi più offerta speciale (DEC-026), scambio ad alto rischio a puntata generata (DEC-044, dettaglio in special-rooms.md) e punti sblocco a doppio canale esclusivi al singleplayer (DEC-015, DEC-027), con presentazione delle prove specifiche al passaggio verso il piano 1 (DEC-042, dettaglio in floor-zero.md); pattern rischio/ricompensa dell'arena di sfida."
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

La **valuta principale** è il nome placeholder per funzione che sostituisce "monete", finché non esiste il nome definitivo del gioco. Le regole generali della risorsa (come si ottiene, cap massimo, ordine di consumo, visibilità in HUD) sono definite in [health-and-resources.md](./health-and-resources.md) come fonte unica; questo documento non le ripete, descrive solo l'**uso economico**:

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

## Interazioni

- con il negozio e gli scambi ([rooms-and-floor-generation.md](./rooms-and-floor-generation.md), [special-rooms.md](./special-rooms.md));
- con la salute e le altre risorse ([health-and-resources.md](./health-and-resources.md));
- con la tassonomia oggetti e rarità ([items-pools-and-rarity.md](./items-pools-and-rarity.md));
- con la meta-progressione ([save-and-meta-progression.md](./save-and-meta-progression.md));
- con la presentazione delle prove specifiche all'ingresso nel piano 1 ([floor-zero.md](./floor-zero.md), DEC-042).

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
