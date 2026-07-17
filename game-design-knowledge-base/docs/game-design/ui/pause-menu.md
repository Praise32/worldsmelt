---
id: gd-ui-pause
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "La pausa ferma la simulazione in singleplayer; il tempo continua in asincrono competitivo."
---

# Pause Menu

## Intento

Permettere di interrompere temporaneamente l'input di gioco, consultare la build e
accedere alle opzioni senza perdere progressi né subire azioni distruttive accidentali.

## Condizioni di ingresso

Da `Gameplay`, tramite il comando di pausa.

## Focus iniziale

Il focus iniziale è su "Riprendi".

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Riprendi | Sempre | Sempre | Chiude `PauseMenu` | Torna in `Gameplay` | La simulazione riprende da dove si era fermata |
| Visualizza build e sinergie | Sempre | Sempre | Apre `BuildScreen` | Entra in `BuildScreen` | Al ritorno, focus su questo elemento |
| Opzioni | Sempre | Sempre | Apre `Options` | Entra in `Options` | Al ritorno, focus sull'elemento "Opzioni" |
| Riavvia run | Se consentito dalla modalità | Sempre, se visibile | Chiede conferma tramite `ExitConfirm` | Riavvia la run con lo stesso o nuovo seed, secondo modalità | Conferma esplicita richiesta |
| Abbandona run | Sempre | Sempre | Chiede conferma tramite `ExitConfirm` | Torna a `MainMenu`; la run è registrata come abbandonata | Conferma esplicita richiesta |

## Decisione approvata: la pausa ferma la simulazione

In singleplayer, `PauseMenu` ferma la simulazione: nemici, timer di run e generazione in
primo piano si sospendono finché il giocatore non seleziona "Riprendi" (approved).

## Nota: run competitive asincrone

Nelle run competitive asincrone (DEC-016), il tempo della run continua a contare mentre
`PauseMenu` è aperto, perché la metrica di classifica è il tempo totale della run
pubblicata: mettere in pausa non deve poter essere usato per "congelare" un vantaggio
temporale. Questa distinzione va comunicata al giocatore quando la modalità è competitiva.

## Regole

- Nessuna azione distruttiva immediata: riavvio e abbandono passano sempre da una conferma (`ExitConfirm`).
- Il focus iniziale è "Riprendi".
- Tornando da `Options`, il focus ritorna sull'elemento "Opzioni" di `PauseMenu`.
- Tornando da `BuildScreen`, il focus ritorna sull'elemento "Visualizza build e sinergie".
- L'abbandono registra correttamente esito e stato competitivo della run (vedi `ui/results-and-leaderboards.md`).

## Non-obiettivi

- Non spiega le sinergie o la fusione: rimanda a `ui/inventory-and-synergy-screen.md`.
- Non gestisce le opzioni: rimanda a `ui/options-and-accessibility.md`.

## Domande aperte residue

- Nessuna specifica; il comportamento della pausa è approved sia in singleplayer sia in asincrono competitivo.

## Scenari verificabili

1. **Given** il giocatore è in `Gameplay` in singleplayer, **when** apre `PauseMenu`, **then** nemici e timer di run si fermano finché non seleziona "Riprendi".
2. **Given** il giocatore è in una run competitiva asincrona, **when** apre `PauseMenu`, **then** il tempo della run continua a contare, con un'indicazione visibile di questo comportamento.
3. **Given** il giocatore è in `PauseMenu` e apre `Options`, **when** torna indietro, **then** il focus è sull'elemento "Opzioni" di `PauseMenu`.
4. **Given** il giocatore seleziona "Abbandona run", **when** conferma in `ExitConfirm`, **then** torna a `MainMenu` e l'esito della run viene registrato come abbandonata, non come vittoria né sconfitta per boss.
