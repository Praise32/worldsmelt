---
id: gd-ui-generation-status
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Comunicazione della preparazione dei contenuti."
---

# Generation Status

## Obiettivo

Ridurre l'attesa percepita e comunicare soltanto informazioni utili.

## Stati

- preparazione iniziale;
- primo piano pronto;
- contenuti successivi in preparazione;
- recupero tramite fallback;
- errore non recuperabile.

## Regole

- Avviare la run quando il primo piano e i requisiti minimi sono pronti.
- Non mostrare percentuali false.
- Preferire messaggi descrittivi stabili.
- Non interrompere il combattimento per comunicare generazione in background.
- In caso di fallback riuscito, continuare senza allarmare inutilmente il giocatore.
