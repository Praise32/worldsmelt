---
id: gd-multiplayer
status: experimental
owner: design
last_reviewed: 2026-07-17
summary: "Gare tra giocatori e classifiche."
---

# Multiplayer and Competition

## Obiettivo

Permettere a due o più giocatori di affrontare una sfida confrontabile e competere su tempo, punteggio o completamento.

## Modalità proposta: Shared Run Race

- I giocatori ricevono lo stesso manifest di run.
- Contenuti, pool, ordine dei piani e principali opportunità sono equivalenti.
- Il risultato considera tempo, completamento e possibili penalità.
- La run deve essere riproducibile per verifica e replay.

## Modalità non classificata proposta: Unique Run Duel

Ogni giocatore affronta una run differente ma con budget di difficoltà equivalente. È più spettacolare ma meno adatta a classifiche rigorose.

## Informazioni visibili

Durante la gara si può mostrare:

- piano corrente dell'avversario;
- tempo relativo;
- stato vivo/eliminato;
- eventi principali, evitando di rivelare informazioni strategiche eccessive.

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

- Multiplayer simultaneo online o confronto asincrono?
- Stessa run esatta o equivalenza statistica?
- Quali assistenze e mod sono consentite?
