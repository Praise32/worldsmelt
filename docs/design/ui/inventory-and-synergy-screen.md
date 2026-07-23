---
id: gd-ui-build-screen
title: Inventory and Synergy Screen (BuildScreen)
domain: design
status: approved
authority: canonical
owner: design
summary: "BuildScreen: sinergie implicite attive e possibilità di fusione, senza dettagli tecnici. Espone anche la sezione Prove, sempre consultabile (DEC-042)."
last_reviewed: 2026-07-18
last_verified_commit: 0ec60d0
topics: [build-screen, sinergie, fusione, prove, pause-menu, DEC-012, DEC-042]
related: []
supersedes: []
source_files: []
---

# Inventory and Synergy Screen (BuildScreen)

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
| Effetti temporanei | Almeno un effetto temporaneo è attivo | — (sola lettura) | Nessuna | — | Durata residua indicata |
| Prove | Da quando le prove sono state presentate all'ingresso nel piano 1 (DEC-042) | Sempre, se visibile | Seleziona la sezione prove | Mostra le prove specifiche della run e il loro stato di completamento | Evidenzia le prove già completate |

## Sinergie implicite e fusione: due binari distinti

Questa schermata mostra entrambi i binari (DEC-012):

- **Sinergie implicite/automatiche**: attive quando due o più oggetti compatibili convivono nella build, senza consumo di oggetti.
- **Possibilità di fusione esplicita**: combinazioni di due oggetti posseduti che, nella stanza di fusione e con catalizzatore sufficiente, produrrebbero un oggetto nuovo generato dall'IA. La fusione effettiva avviene solo nella stanza di fusione (vedi `systems/special-rooms.md`), non da questa schermata. Fonte di sistema: `systems/item-fusion.md`.

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

## Non mostrare

- formule interne complete;
- prompt dell'IA;
- punteggi di validazione tecnici o stati di validazione interni (fonte unica della
  trasparenza: `06-ai-content-generation-model.md`).

## Non-obiettivi

- Non esegue la fusione: la fusione richiede la stanza di fusione durante `Gameplay`.
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
