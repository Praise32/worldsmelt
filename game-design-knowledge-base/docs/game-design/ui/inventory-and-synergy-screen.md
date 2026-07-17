---
id: gd-ui-build-screen
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "BuildScreen: sinergie implicite attive e possibilità di fusione, senza dettagli tecnici."
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
