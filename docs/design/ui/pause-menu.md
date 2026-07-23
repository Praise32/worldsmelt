---
id: gd-ui-pause
title: Pause Menu
domain: design
status: approved
authority: canonical
owner: design
summary: "La pausa ferma la simulazione in singleplayer; il tempo continua in asincrono competitivo. Espone anche l'elenco delle prove specifiche della run, sempre consultabile (DEC-042)."
last_reviewed: 2026-07-19
last_verified_commit: 0ec60d0
topics: [pause-menu, pausa, prove, abbandono-run, DEC-042, DEC-082, DEC-089]
related: []
supersedes: []
source_files: []
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
| Prove | Da quando le prove sono state presentate all'ingresso nel piano 1 (DEC-042) | Sempre, se visibile | Apre l'elenco delle prove specifiche della run | Mostra le prove attive e il relativo stato di completamento | Al ritorno, focus su questo elemento |
| Opzioni | Sempre | Sempre | Apre `Options` | Entra in `Options` | Al ritorno, focus sull'elemento "Opzioni" |
| Riavvia run | Se consentito dalla modalità | Sempre, se visibile | Chiede conferma tramite `ExitConfirm` | Riavvia la run con lo stesso o nuovo seed, secondo modalità | Conferma esplicita richiesta |
| Abbandona run | Sempre | Sempre | Chiede conferma tramite `ExitConfirm` | Entra in `RunResults`; la run si chiude come sconfitta, con punti sblocco ridotti visibili (DEC-082, DEC-089) | Conferma esplicita richiesta |

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
- Tornando dall'elenco delle prove, il focus ritorna sull'elemento "Prove".
- L'abbandono confermato conta come sconfitta ai fini dei punti sblocco (DEC-082) e porta a `RunResults`, non più a `MainMenu` diretto (DEC-089); il dettaglio dei punti ridotti, dell'esito mostrato e del ritorno al menu da lì è definito in `ui/results-and-leaderboards.md`, non ripetuto qui.

## Prove (DEC-042)

Le prove specifiche della run (fisse o generate, DEC-027) vengono presentate al giocatore
al passaggio dal Piano 0 al piano 1 e restano sempre consultabili da qui, senza bisogno di
attendere la fine della run. Il dettaglio di quando e come vengono presentate è definito in
`systems/floor-zero.md`; il dettaglio del loro contenuto e del punteggio bonus è definito in
`systems/rewards-and-economy.md`. Questo documento non ripete quei dettagli, colloca solo
la voce di menu.

## Non-obiettivi

- Non spiega le sinergie o la fusione: rimanda a `ui/inventory-and-synergy-screen.md`.
- Non gestisce le opzioni: rimanda a `ui/options-and-accessibility.md`.
- Non definisce il contenuto o il punteggio delle prove: rimanda a `systems/rewards-and-economy.md` e `systems/floor-zero.md`.
- Non definisce il contenuto di `RunResults` né il ritorno al menu da lì: rimanda a `ui/results-and-leaderboards.md`.

## Domande aperte residue

- Nessuna specifica; il comportamento della pausa è approved sia in singleplayer sia in asincrono competitivo.

## Scenari verificabili

1. **Given** il giocatore è in `Gameplay` in singleplayer, **when** apre `PauseMenu`, **then** nemici e timer di run si fermano finché non seleziona "Riprendi".
2. **Given** il giocatore è in una run competitiva asincrona, **when** apre `PauseMenu`, **then** il tempo della run continua a contare, con un'indicazione visibile di questo comportamento.
3. **Given** il giocatore è in `PauseMenu` e apre `Options`, **when** torna indietro, **then** il focus è sull'elemento "Opzioni" di `PauseMenu`.
4. **Given** il giocatore seleziona "Abbandona run", **when** conferma in `ExitConfirm`, **then** entra in `RunResults`, che mostra la run chiusa come sconfitta con i punti sblocco maturati fino a quel momento in misura ridotta (DEC-082, DEC-089); da lì il ritorno al menu segue `ui/results-and-leaderboards.md`.
5. **Given** il giocatore ha attraversato l'uscita del Piano 0 verso il piano 1 e le prove sono state presentate (DEC-042), **when** apre `PauseMenu` e seleziona "Prove", **then** vede l'elenco delle prove specifiche della run e il loro stato di completamento.
