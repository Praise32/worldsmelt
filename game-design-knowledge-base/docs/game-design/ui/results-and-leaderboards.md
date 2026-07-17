---
id: gd-ui-results-leaderboards
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Run ufficiale chiusa al boss del piano 5; prosecuzione registrata a parte; riprova non classificata."
---

# Results and Leaderboards (RunResults)

## Intento

Comunicare l'esito della run in modo chiaro, distinguendo la run ufficiale dalla
prosecuzione oltre il piano 5, e dare accesso alle classifiche coerenti con DEC-016.

## Condizioni di ingresso

Da `Gameplay`, quando la run termina: vittoria sul boss del piano 5, morte in un piano
qualunque (permadeath), o abbandono confermato da `PauseMenu`.

## Run ufficiale e prosecuzione (DEC-006)

- Sconfiggere il boss del piano 5 chiude la **run ufficiale**, valida per le classifiche.
- Da quel momento il giocatore può scegliere di proseguire in piani extra sempre più
  degenerati; questa prosecuzione viene registrata **a parte** e non altera il risultato
  già acquisito della run ufficiale.
- La salute a zero, in qualunque piano, è run persa (permadeath): non esiste un "continua"
  dopo la morte.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Esito | Sempre | — (sola lettura) | Nessuna | — | Vittoria (boss piano 5), sconfitta, o abbandono, con eventuale prosecuzione extra indicata a parte |
| Tempo e piano raggiunto | Sempre | — (sola lettura) | Nessuna | — | — |
| Build finale e sinergie notevoli | Sempre | — (sola lettura) | Consulta dettagli | Apre una vista sola-lettura della build | — |
| Punteggio | Sempre | — (sola lettura) | Nessuna | — | — |
| Identificatore condivisibile della run | Sempre | Sempre | Copia/condividi identificatore | Permette ad altri di scegliere la stessa run/seed | Conferma di copia |
| Nuova run | Sempre | Sempre | Apre `RunSetup` | Entra in `RunSetup` senza vincoli dalla run appena conclusa | — |
| Riprova la stessa run | Sempre | Sempre | Apre `RunSetup` con lo stesso seed/manifest precompilato | Riavvia la stessa run, ma **non classificata** | Etichetta esplicita "non classificata" |
| Torna al menu | Sempre | Sempre | Chiude `RunResults` | Entra in `MainMenu` | — |

## Decisione approvata: riprova non classificata

"Riprova la stessa run" è permesso ma la nuova esecuzione non è classificata: rigiocare
lo stesso seed non deve poter essere usato per manipolare una classifica basata su tempo o
punteggio (approved).

## Classifica

Ogni voce di classifica è legata a modalità, versione delle regole e run/stagione (DEC-016);
le classifiche sono a tempo o a punteggio; i pool sbloccati dalla meta-progressione sono
esclusi dal calcolo competitivo.

## Fallback rilevanti

Se un fallback ha sostituito contenuti durante la run, e ciò incide sulla validità
competitiva, va segnalato qui in modo sintetico, senza dettagli tecnici. Fonte unica delle
regole di fallback: `systems/generated-content-validation.md`.

## Non-obiettivi

- Non ridefinisce le condizioni di vittoria: rimanda a `04-run-structure.md` e DEC-006.
- Non gestisce la scelta del prossimo tema o personaggio: quella avviene nel Piano 0 della run successiva.

## Domande aperte residue

- Metriche di classifica oltre a tempo e punteggio restano `experimental` (vedi `governance/open-questions.md`).

## Scenari verificabili

1. **Given** il giocatore sconfigge il boss del piano 5, **when** entra in `RunResults`, **then** l'esito mostra "vittoria" per la run ufficiale, indipendentemente da eventuali piani extra successivi.
2. **Given** il giocatore muore in un piano qualunque, **when** entra in `RunResults`, **then** l'esito mostra sconfitta e nessuna opzione di "continua" è disponibile (permadeath).
3. **Given** il giocatore sceglie "Riprova la stessa run", **when** la nuova esecuzione parte, **then** è etichettata come non classificata fin dall'inizio.
4. **Given** il giocatore prosegue oltre il piano 5 dopo la vittoria, **when** muore in un piano extra, **then** il risultato della run ufficiale resta la vittoria già registrata, e la morte extra è annotata separatamente.
