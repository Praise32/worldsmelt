---
id: gd-multiplayer
status: experimental
owner: design
last_reviewed: 2026-07-17
summary: "Gare tra giocatori e classifiche."
---

# Multiplayer and Competition

## Visione approvata (DEC-016, approved)

Questa sezione, a differenza del resto del documento (che resta `experimental`), registra
una decisione approvata dal proprietario il 2026-07-17:

- Il multiplayer competitivo è **asincrono**: i giocatori non condividono una sessione in
  tempo reale, ma affrontano **la stessa run** — stesso seed/manifest, sfruttando il
  determinismo già presente nel run manifest (vedi
  [Run Manifest and Reproducibility](systems/run-manifest-and-reproducibility.md)).
- Le classifiche si basano su **tempo** e **punteggio**.
- I pool sbloccati tramite meta-progressione sono **esclusi** dalle modalità competitive:
  ogni giocatore affronta la run competitiva con gli stessi pool di base (vedi
  [Core Loop](03-core-loop.md), loop metagioco, DEC-015).

Ogni altro dettaglio del multiplayer (modalità simultanee, informazioni visibili durante la
gara, assistenze consentite, dettagli implementativi) resta `experimental` e non è ancora
deciso.

## Obiettivo

Permettere a due o più giocatori di affrontare una sfida confrontabile e competere su tempo, punteggio o completamento.

## Modalità proposta: Shared Run Race (experimental)

- I giocatori ricevono lo stesso manifest di run (stesso seed), coerentemente con la
  visione approvata sopra.
- Contenuti, pool, ordine dei piani e principali opportunità sono equivalenti.
- Il risultato considera tempo, completamento e possibili penalità.
- La run deve essere riproducibile per verifica e replay.
- Poiché la gara è asincrona, ogni giocatore affronta la run separatamente: non è previsto
  un incontro in tempo reale nella stessa sessione (vedi Decisioni aperte per i dettagli
  ancora da chiarire).

## Modalità non classificata proposta: Unique Run Duel (experimental)

Ogni giocatore affronta una run differente ma con budget di difficoltà equivalente. È più
spettacolare ma meno adatta a classifiche rigorose. Questa modalità non è coperta dalla
visione approvata (DEC-016 riguarda la modalità classificata a stessa run/seed) e resta
un'idea `experimental`.

## Informazioni visibili (experimental)

Poiché la modalità classificata approvata è asincrona, il confronto tra giocatori avviene
sul risultato registrato, non su una sessione condivisa in tempo reale. Da definire, come
dettaglio `experimental`, quali informazioni della run registrata di un avversario possono
essere mostrate, ad esempio:

- piano raggiunto e tempo totale dell'avversario, a run conclusa;
- eventi principali della sua run, evitando di rivelare informazioni strategiche eccessive
  prima che il giocatore corrente completi la propria.

## Classifiche

Categorie possibili:

- completamento più rapido;
- punteggio;
- serie di vittorie;
- stagione o set di run ufficiali;
- modalità caos separata.

## Correttezza

Le classifiche richiedono regole stabili, identificazione della versione di gioco e manifest verificabile.

## Decisioni aperte

Risolte da DEC-016 (non più aperte per la modalità classificata): multiplayer simultaneo o
asincrono → **asincrono**; stessa run esatta o equivalenza statistica → **stessa run
esatta, stesso seed**.

Ancora aperte, tutte `experimental`:

- Quali assistenze e mod sono consentite.
- Se e come implementare la modalità non classificata Unique Run Duel.
- Quali informazioni della run di un avversario mostrare e quando.
