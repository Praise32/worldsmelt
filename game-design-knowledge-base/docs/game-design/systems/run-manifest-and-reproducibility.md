---
id: gd-system-run-manifest
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Identità della run e possibilità di riprodurla."
---

# Run Manifest and Reproducibility

## Scopo

Ogni run deve avere una descrizione stabile sufficiente a ricostruire contenuti, regole e versione del gioco.

## Informazioni concettuali

- identificatore della run;
- versione delle regole;
- modalità;
- pool disponibili;
- piani e contenuti approvati;
- modificatori;
- regole competitive;
- fallback avvenuti.

## Uso

- condivisione di run;
- classifiche;
- replay o verifica;
- debug;
- confronto tra giocatori.

## Regola competitiva

Una run classificata non può cambiare dopo l'avvio in modo non deterministico tra concorrenti, salvo eventi esplicitamente sincronizzati e verificabili.
