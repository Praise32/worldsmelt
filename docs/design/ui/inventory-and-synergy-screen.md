---
id: gd-ui-build-screen
title: Inventory and Synergy Screen (BuildScreen)
domain: design
status: approved
authority: canonical
owner: design
summary: "BuildScreen: sinergie implicite attive e possibilità di fusione, senza dettagli tecnici. Espone anche la sezione Prove, sempre consultabile (DEC-042)."
last_reviewed: 2026-07-30
last_verified_commit: d5c5f43
topics: [build-screen, sinergie, fusione, prove, pause-menu, DEC-012, DEC-042, DEC-184, WP16]
related: []
supersedes: []
source_files: [src/render/game_renderer.c, src/app/app.c]
---

# Inventory and Synergy Screen (BuildScreen)

> Aggiunta del 22/07 (DEC-139): `BuildScreen` si apre anche con **TAB direttamente da
> `Gameplay`** (e TAB la richiude), oltre che da Pausa; la simulazione resta ferma mentre
> è aperta. Coerente col TAB del Piano 0.

## Intento

Dare al giocatore una spiegazione comprensibile della propria build corrente: cosa
possiede, quali sinergie implicite sono attive e quali fusioni sono possibili con gli
oggetti posseduti, senza esporre dettagli tecnici.

## Condizioni di ingresso

Da `PauseMenu` ("Visualizza build e sinergie").

## Focus iniziale

L'ultimo oggetto acquisito, o il primo oggetto della lista se la build è vuota di oggetti recenti.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Lista oggetti acquisiti | Sempre | Sempre | Seleziona un oggetto | Mostra dettagli dell'oggetto | Evidenzia gli oggetti coinvolti in sinergie attive |
| Slot Innesto | Sempre | Un Innesto è equipaggiato | Seleziona l'Innesto | Mostra dettagli dell'Innesto | — |
| Slot attivo | Sempre | Un oggetto attivo è equipaggiato | Seleziona l'oggetto attivo | Mostra dettagli e carica | — |
| Statistiche principali | Sempre | — (sola lettura) | Nessuna | — | Valori aggiornati in tempo reale |
| Sinergie implicite attive | Almeno una sinergia implicita è attiva | Sempre, se visibile | Seleziona una sinergia | Vedi "Regola di comprensione" | Componenti coinvolti evidenziati insieme |
| Fusioni possibili | Il giocatore possiede almeno due oggetti fondibili e catalizzatore di fusione sufficiente | Sempre, se visibile | Seleziona una combinazione possibile | Mostra un'anteprima non tecnica del risultato atteso | Indicazione se manca il catalizzatore di fusione |
| Conferma fusione (**demo**, vedi la nota sotto) | Sempre, nella fascia «Fusione» | Due sorgenti scelte e almeno un catalizzatore | Confermare l'operazione | Esegue la fusione secondo `systems/item-fusion.md` | Esito con nome e immagine del composto; se non si può fondere, il motivo in chiaro |
| Effetti temporanei | Almeno un effetto temporaneo è attivo | — (sola lettura) | Nessuna | — | Durata residua indicata |
| Prove | Da quando le prove sono state presentate all'ingresso nel piano 1 (DEC-042) | Sempre, se visibile | Seleziona la sezione prove | Mostra le prove specifiche della run e il loro stato di completamento | Evidenzia le prove già completate |

> **Nota (30/07, W9 playtest round 1 — copertura mouse):** le righe della lista oggetti e la
> riga "Conferma fusione" della fascia «Fusione» sono raggiungibili **sia da tastiera/pad sia
> col mouse** (click sulle righe per scegliere le due sorgenti, rotellina per scorrere la
> lista oltre la finestra visibile, click sulla riga di conferma come il tasto `[F]`): la
> parità di input di DEC-057 vale in questa schermata come in ogni altro menu, e senza il
> click sulla conferma nessun percorso col solo mouse avrebbe portato a termine una fusione.
> Fonte unica della regola e stato di implementazione:
> [Options and Accessibility](options-and-accessibility.md).
>
> **Nota (30/07, DEC-184):** le stesse statistiche della riga "Statistiche principali" —
> danno, cadenza, velocità del colpo, velocità di movimento, raggio, Fortuna — sono ora
> consultabili anche in un blocco compatto dell'HUD durante `Gameplay`, visibile di
> default con un tasto di toggle: vedi [HUD](./hud.md). Questa schermata resta comunque la
> vista completa e non è sostituita da quel blocco.

## Sinergie implicite e fusione: due binari distinti

Questa schermata mostra entrambi i binari (DEC-012):

- **Sinergie implicite/automatiche**: attive quando due o più oggetti compatibili convivono nella build, senza consumo di oggetti.
- **Possibilità di fusione esplicita**: combinazioni di due oggetti posseduti che, con catalizzatore sufficiente, producono un oggetto nuovo. Fonte di sistema: `systems/item-fusion.md`.

> **Nota di implementazione (demo, 2026-07-27; aggiornata 30/07 con WP4) — default
> proposto dall'implementazione (stile DEC-019).** Il modello canonico vuole la fusione
> *eseguita* nella stanza di fusione (`systems/special-rooms.md`) e questa schermata come
> sola consultazione. Dal WP4 `ROOM_FUSION` **esiste** nel motore e il crogiolo della stanza
> apre questa schermata già pronta alla fusione (le due sorgenti restano comunque da
> scegliere qui), ma la conferma resta un'azione di `BuildScreen`, raggiungibile da **tre
> porte equivalenti**: il crogiolo della stanza di fusione, **TAB** da `Gameplay`, e la voce
> dedicata nel `PauseMenu`. Comandi: su/giù scorrono gli oggetti, **INVIO**
> seleziona/deseleziona una sorgente, **F** conferma (tasto dedicato: l'operazione è
> irreversibile), **ESC/TAB** escono. Se la conferma debba spostarsi integralmente nella
> stanza di fusione (con questa schermata tornata sola consultazione) resta una domanda
> aperta per il proprietario: vedi `governance/open-questions.md`.

## Regola di comprensione

Selezionando una sinergia implicita, il giocatore vede:

- componenti coinvolti;
- risultato comportamentale;
- modifiche visive;
- eventuali incompatibilità o limiti dichiarati.

Selezionando una possibilità di fusione, il giocatore vede quali due oggetti verrebbero
consumati e un'anteprima non tecnica dell'effetto atteso, senza specifiche numeriche interne.

## Prove (DEC-042, rimando)

La sezione Prove mostra le prove specifiche della run e il loro stato di completamento; le
stesse prove sono anche consultabili da `ui/pause-menu.md`, dove sono state presentate per
la prima volta al passaggio dal Piano 0 al piano 1. Il contenuto e il punteggio bonus delle
prove sono definiti in `systems/rewards-and-economy.md`; questo documento non li ripete,
colloca solo la sezione qui.

> **Nota di implementazione (WP16, 2026-07-30) — limite dichiarato.** La tabella sopra chiede
> una sezione **selezionabile** ("Seleziona la sezione prove" → elenco completo). Oggi
> `DrawBuildScreenOverlay` (`src/render/game_renderer.c`) disegna solo una riga di
> **riepilogo** nel blocco statistiche di destra — "Prove: N/M, +X" — visibile quando
> `game->trialCount > 0`, con lo stesso pattern delle altre righe statistiche (sola lettura,
> nessuna selezione dedicata). Il denominatore M di questa riga è `TrialsCountedTotal`
> (aggiornamento 30/07, seconda tornata), non il grezzo `trialCount`: esclude le prove
> "annullate" (`TRIAL_VOID`, vedi `systems/rewards-and-economy.md`, "Casi limite") il cui
> archetipo non è mai comparso in questa run. L'elenco COMPLETO per prova (testo + stato)
> esiste solo nel pannello di `PauseMenu` (`DrawTrialsPanel`, vedi `ui/pause-menu.md`),
> coerente in numeri con questa riga (`TrialsPassedCount`/`TrialsCountedTotal`/
> `TrialsBonusTotal`, unica fonte per entrambe le schermate) ma non duplicato qui. Colmare il
> gap — rendere la riga selezionabile e aprire lo stesso dettaglio anche da `BuildScreen` —
> resta lavoro futuro, non fatto in questo giro.

## Non mostrare

- formule interne complete;
- prompt dell'IA;
- punteggi di validazione tecnici o stati di validazione interni (fonte unica della
  trasparenza: `06-ai-content-generation-model.md`).

## Non-obiettivi

- Non esegue la fusione come atto isolato di stanza: il modello canonico vuole la
  conferma spostata integralmente nella stanza di fusione, con questa schermata come sola
  consultazione. Dal WP4 la conferma resta in `BuildScreen` anche dopo l'ingresso dal
  crogiolo (**default proposto, vedi la nota di implementazione sopra e la domanda aperta
  in `governance/open-questions.md`**).
- Non sostituisce l'HUD per le decisioni immediate in combattimento.

## Domande aperte residue

- Nessuna specifica; i dettagli tecnici della fusione (numero massimo di fusioni per run,
  se esistono) restano da definire in `systems/item-fusion.md`, fuori scope di questo documento.

## Scenari verificabili

1. **Given** il giocatore possiede due oggetti compatibili con una sinergia implicita attiva, **when** apre `BuildScreen` e seleziona la sinergia, **then** vede componenti, risultato comportamentale e modifiche visive, senza formule interne.
2. **Given** il giocatore possiede due oggetti fondibili ma catalizzatore di fusione insufficiente, **when** apre la sezione "Fusioni possibili", **then** la combinazione appare ma con un'indicazione chiara che manca il catalizzatore.
3. **Given** il giocatore non possiede alcuna sinergia implicita attiva, **when** apre `BuildScreen`, **then** la sezione "Sinergie implicite attive" non è visibile.
4. **Given** il giocatore seleziona una possibilità di fusione, **when** consulta l'anteprima, **then** non vede alcun punteggio di validazione tecnico né prompt dell'IA.
5. **Given** le prove sono state presentate al passaggio dal Piano 0 al piano 1 (DEC-042), **when** il giocatore apre `BuildScreen` e seleziona "Prove", **then** vede l'elenco delle prove specifiche della run e il loro stato di completamento, coerente con quanto mostrato in `ui/pause-menu.md`.
6. **Given** il giocatore possiede due oggetti e un catalizzatore, **when** apre `BuildScreen`, seleziona le due sorgenti e conferma la fusione, **then** i due oggetti e un catalizzatore si consumano e la schermata mostra l'esito con nome e immagine dell'oggetto composto (nota di implementazione della demo).
7. **Given** il giocatore ha selezionato due sorgenti ma non possiede alcun catalizzatore, **when** guarda la fascia «Fusione», **then** legge in chiaro che serve un catalizzatore e la conferma non produce alcun effetto.
