---
id: gd-ui-pause
title: Pause Menu
domain: design
status: approved
authority: canonical
owner: design
summary: "La pausa ferma la simulazione in singleplayer; il tempo continua in asincrono competitivo. Espone anche l'elenco delle prove specifiche della run, sempre consultabile (DEC-042), ed è il punto in cui l'HUD di combattimento resta consultabile su richiesta durante il Piano 0, dove è nascosto (DEC-169)."
last_reviewed: 2026-07-30
last_verified_commit: d5c5f43
topics: [pause-menu, pausa, prove, abbandono-run, reroll, DEC-042, DEC-082, DEC-089, DEC-114, DEC-169, WP16]
related: []
supersedes: []
source_files: [src/render/game_renderer.c]
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
| Riavvia run (reroll) | Se consentito dalla modalità | Sempre, se visibile | Chiede conferma tramite `ExitConfirm` | Reroll di DEC-089: salta i risultati, accredita i punti in silenzio, riavvia con lo stesso o nuovo seed secondo modalità | Conferma esplicita richiesta; è l'UNICA via per il reroll (DEC-114) |
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
- Il reroll non ha tasti rapidi diretti in `Gameplay` (DEC-114): l'unica collocazione è questa voce. Gap di implementazione: oggi il tasto `R` rigenera direttamente, da adeguare.
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

> **Nota di implementazione (WP16, 2026-07-30, aggiornata 30/07 seconda tornata):** "Prove" è
> la terza riga di `PauseMenu` (indice 2, tra "Visualizza build e sinergie" e "Opzioni" —
> cinque righe in tutto oggi, `DrawPauseMenuOverlay`/`src/render/game_renderer.c`), visibile
> ogni volta che questo menu è raggiungibile (solo da `Gameplay`, cioè sempre dopo che le
> prove sono state assegnate). Confermarla apre un pannello INTERNO a `PauseMenu` (nessun
> nuovo `AppMode`, stessa scelta architetturale del Catalogo dentro `APP_MAIN_MENU`): elenco
> delle prove con testo e stato ("in corso"/"superata"/"fallita"/"annullata", mai il solo
> colore, DEC-058) più il totale "N/M superate, +X punti" (`TrialsPassedCount`/
> `TrialsCountedTotal`/`TrialsBonusTotal`, la stessa fonte che `RunResults` e `BuildScreen`
> leggono, così le tre schermate non possono mai divergere). Il quarto stato, "annullata"
> (`TRIAL_VOID`), è una prova scartata perché il suo archetipo non è mai comparso in questa
> run (vedi `systems/rewards-and-economy.md`, "Casi limite"): esclusa dal denominatore M,
> mai contata contro il giocatore. ESC o INVIO chiudono il pannello e riportano il focus
> sull'indice 2, come da tabella sopra. Mentre il pannello è aperto il mouse è sospeso sulle
> righe di menu sottostanti (stessa esclusione del Catalogo dentro `APP_MAIN_MENU`,
> `src/app/app.c`): senza questa guardia un click o un hover sui rettangoli invisibili delle
> righe di `PauseMenu` chiuderebbe il pannello o ne corromperebbe il focus a caso.

## Consultazione dell'HUD nel Piano 0 (DEC-169)

Nel Piano 0 l'HUD di combattimento è **nascosto** durante l'esplorazione dell'hub e torna
visibile solo nelle prove del Piano 0 — arene di sfida e tutorial integrato (DEC-047), da non
confondere con le «prove» specifiche della run di DEC-042 di cui sopra (fonte unica della
regola: [HUD](hud.md), dettaglio del Piano 0 in `systems/floor-zero.md`). Il menu di pausa è
il punto in cui quell'informazione resta
**consultabile su richiesta**: chi vuole controllare salute, risorse e build mentre la run è
ancora in preparazione lo fa da qui, senza uscire dal Piano 0 e senza che l'hub mostri l'HUD
in permanenza. Questo documento colloca la consultazione, non ridefinisce il contenuto
dell'HUD né la regola di visibilità.

DEC-169 **non fissa il comando** con cui il menu di pausa si apre dal Piano 0: ESC è già
assegnato a `ExitConfirm` (DEC-074) e le condizioni di ingresso qui sopra prevedono la sola
provenienza da `Gameplay`. Il punto è registrato come domanda aperta
(`../governance/open-questions.md`, punto 22) e non viene deciso qui.

> **Nota di implementazione (demo W3, 2026-07-28):** il riquadro di consultazione è già
> disegnato oggi in `DrawPauseMenuOverlay` (`src/render/game_renderer.c`), condizionato solo
> a `game->floor == 0` — indipendente da quale comando abbia aperto `PauseMenu`, così
> funzionerà senza altro lavoro sul renderer non appena la domanda aperta 22 verrà risolta.
> Mostra salute (cuori) e risorse (`Ingots`/`Blast Charges`/`Cast Keys`/`Flux`), le stesse
> informazioni della riga vitali dell'HUD di `Gameplay` — non il timer di run, che non esiste
> ancora nel motore (gap noto, indipendente da questo lavoro, vedi `ui/hud.md`).

## Non-obiettivi

- Non spiega le sinergie o la fusione: rimanda a `ui/inventory-and-synergy-screen.md`.
- Non gestisce le opzioni: rimanda a `ui/options-and-accessibility.md`.
- Non definisce il contenuto o il punteggio delle prove: rimanda a `systems/rewards-and-economy.md` e `systems/floor-zero.md`.
- Non definisce il contenuto di `RunResults` né il ritorno al menu da lì: rimanda a `ui/results-and-leaderboards.md`.

## Domande aperte residue

- Il comportamento della pausa è approved sia in singleplayer sia in asincrono competitivo.
- Con quale comando il menu di pausa si apre dal Piano 0, dove DEC-169 lo indica come luogo
  di consultazione dell'HUD ma ESC è già assegnato a `ExitConfirm` (DEC-074):
  `../governance/open-questions.md`, punto 22.

## Scenari verificabili

1. **Given** il giocatore è in `Gameplay` in singleplayer, **when** apre `PauseMenu`, **then** nemici e timer di run si fermano finché non seleziona "Riprendi".
2. **Given** il giocatore è in una run competitiva asincrona, **when** apre `PauseMenu`, **then** il tempo della run continua a contare, con un'indicazione visibile di questo comportamento.
3. **Given** il giocatore è in `PauseMenu` e apre `Options`, **when** torna indietro, **then** il focus è sull'elemento "Opzioni" di `PauseMenu`.
4. **Given** il giocatore seleziona "Abbandona run", **when** conferma in `ExitConfirm`, **then** entra in `RunResults`, che mostra la run chiusa come sconfitta con i punti sblocco maturati fino a quel momento in misura ridotta (DEC-082, DEC-089); da lì il ritorno al menu segue `ui/results-and-leaderboards.md`.
5. **Given** il giocatore ha attraversato l'uscita del Piano 0 verso il piano 1 e le prove sono state presentate (DEC-042), **when** apre `PauseMenu` e seleziona "Prove", **then** vede l'elenco delle prove specifiche della run e il loro stato di completamento.
6. **Given** il giocatore è nel Piano 0, dove l'HUD di combattimento è nascosto (DEC-169), **when** apre il menu di pausa, **then** può consultare salute, risorse e build senza uscire dal Piano 0, e l'HUD torna nascosto quando la pausa si chiude.
