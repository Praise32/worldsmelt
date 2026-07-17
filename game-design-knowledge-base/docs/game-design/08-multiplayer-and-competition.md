---
id: gd-multiplayer
status: experimental
owner: design
last_reviewed: 2026-07-18
summary: "Gare tra giocatori e classifiche: due assi approvati (Leggera/Classificata × stesso seed/seed diversi), dettagli experimental."
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

## Struttura del menu multiplayer (DEC-021, approved 2026-07-18)

Il menu multiplayer offre due scelte indipendenti, entrambe approvate dal proprietario:

1. **Modalità**: **Leggera** (non classificata, senza vincoli di classifica) oppure
   **Classificata** (valida per le classifiche, con le regole di correttezza di DEC-016).
2. **Tipo di gara**: **Stesso seed** (tutti i partecipanti affrontano la stessa run
   esatta, stesso seed/manifest, sfruttando il determinismo esistente) oppure
   **Seed diversi** (ogni giocatore ha una run propria).

Le quattro combinazioni sono tutte previste. Per la combinazione Classificata + Seed
diversi la confrontabilità dei risultati (pillar "Competizione verificabile") richiede
un'equivalenza di difficoltà tra run diverse: il criterio di normalizzazione è una
domanda aperta (vedi `governance/open-questions.md`).

Ogni altro dettaglio del multiplayer (informazioni visibili durante la
gara, assistenze consentite, dettagli implementativi) resta `experimental` e non è ancora
deciso.

## Obiettivo

Permettere a due o più giocatori di affrontare una sfida confrontabile e competere su tempo, punteggio o completamento.

## Gara a stesso seed (asse "Tipo di gara", DEC-021)

- I giocatori ricevono lo stesso manifest di run (stesso seed), coerentemente con la
  visione approvata sopra.
- Contenuti, pool, ordine dei piani e principali opportunità sono identici.
- Il risultato considera tempo, completamento e possibili penalità.
- La run deve essere riproducibile per verifica e replay.
- Poiché la gara è asincrona, ogni giocatore affronta la run separatamente: non è previsto
  un incontro in tempo reale nella stessa sessione.

## Gara a seed diversi (asse "Tipo di gara", DEC-021)

Ogni giocatore affronta una run propria. In modalità Leggera non serve alcuna garanzia di
equivalenza. In modalità Classificata le run diverse devono avere budget di difficoltà
equivalente: il criterio di normalizzazione (come si dichiara che due run sono
confrontabili) è una domanda aperta e resta `experimental`.

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

Risolte da DEC-016 e DEC-021 (non più aperte): multiplayer simultaneo o asincrono →
**asincrono**; struttura del menu → **Leggera/Classificata × stesso seed/seed diversi**.

Ancora aperte, tutte `experimental`:

- Quali assistenze e mod sono consentite.
- Il criterio di normalizzazione per la Classificata a seed diversi.
- Quali informazioni della run di un avversario mostrare e quando.
