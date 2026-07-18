---
id: gd-ui-results-leaderboards
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Vittoria chiusa al boss del piano 5 (DEC-031); alla sconfitta restano i punti sblocco maturati in misura ridotta (DEC-041); riprova non classificata."
---

# Results and Leaderboards (RunResults)

## Intento

Comunicare l'esito della run in modo chiaro — vittoria al boss del piano 5 o sconfitta — e
dare accesso alle classifiche coerenti con DEC-016.

## Condizioni di ingresso

Da `Gameplay`, quando la run termina: vittoria sul boss del piano 5, morte in un piano
qualunque (permadeath), o abbandono confermato da `PauseMenu`.

## Vittoria e sconfitta (DEC-006, aggiornata da DEC-031)

- Sconfiggere il boss del piano 5 chiude la run con **vittoria**, valida per le classifiche.
  La run finisce lì: la prosecuzione in piani extra non è implementata in questa fase del
  gioco e resta un'idea futura parcheggiata (DEC-018, DEC-031), non un'opzione presentata in
  `RunResults`.
- La salute a zero, in qualunque piano, è run persa (permadeath): non esiste un "continua"
  dopo la morte.

## Alla sconfitta (DEC-041)

Alla sconfitta restano i punti sblocco maturati durante la run, ma in **misura ridotta**
rispetto alla vittoria; il catalogo si aggiorna comunque con le creazioni incontrate e con le
statistiche della run. Nessun oggetto sopravvive alla run in nessun caso (permadeath,
DEC-006).

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Esito | Sempre | — (sola lettura) | Nessuna | — | Vittoria (boss piano 5), sconfitta, o abbandono |
| Tempo e piano raggiunto | Sempre | — (sola lettura) | Nessuna | — | — |
| Build finale e sinergie notevoli | Sempre | — (sola lettura) | Consulta dettagli | Apre una vista sola-lettura della build | — |
| Punteggio | Sempre | — (sola lettura) | Nessuna | — | — |
| Punti sblocco maturati | Sempre | — (sola lettura) | Nessuna | — | Valore intero alla vittoria, ridotto alla sconfitta (DEC-041) |
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

1. **Given** il giocatore sconfigge il boss del piano 5, **when** entra in `RunResults`, **then** l'esito mostra "vittoria" e la run risulta conclusa lì, senza alcuna opzione di prosecuzione (DEC-031).
2. **Given** il giocatore muore in un piano qualunque, **when** entra in `RunResults`, **then** l'esito mostra sconfitta e nessuna opzione di "continua" è disponibile (permadeath).
3. **Given** il giocatore sceglie "Riprova la stessa run", **when** la nuova esecuzione parte, **then** è etichettata come non classificata fin dall'inizio.
4. **Given** il giocatore muore (permadeath) prima del boss del piano 5, **when** entra in `RunResults`, **then** i punti sblocco maturati sono mostrati in misura ridotta rispetto a quanto avrebbe ottenuto con una vittoria, e il catalogo risulta comunque aggiornato con le creazioni incontrate (DEC-041).
