---
id: gd-ui-multiplayer-lobby
status: experimental
owner: design
last_reviewed: 2026-07-17
summary: "Selezione asincrona di una run/seed pubblicata (DEC-016); resta experimental."
---

# Multiplayer Lobby

## Intento

Permettere al giocatore di scegliere una gara asincrona sulla stessa run di altri
giocatori, coerentemente con la visione fissata in DEC-016. I dettagli restano
`experimental` e non sono ancora comportamento approvato.

## Visione fissata (DEC-016, approved)

- Le gare sono **asincrone**: non esiste una lobby live con partecipanti in attesa
  simultanea. Il giocatore sceglie una run/seed già pubblicata (stesso seed/manifest per
  tutti i partecipanti, determinismo già esistente nel sistema).
- Le classifiche si basano su tempo e punteggio.
- I pool sbloccati dalla meta-progressione sono esclusi dalle run competitive.

## Condizioni di ingresso

Da `MainMenu` ("Multiplayer").

## Focus iniziale

La run/seed pubblicata più recente o più rilevante per il giocatore.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Elenco run/seed pubblicate | Sempre | Almeno una run pubblicata disponibile | Seleziona una run/seed | Mostra dettagli della gara | Indica se già completata dal giocatore |
| Regole della run | Una run/seed è selezionata | Sempre, se visibile | Consulta le regole | Mostra modalità, versione e manifest | — |
| Classificata/non classificata | Una run/seed è selezionata | — (sola lettura) | Nessuna | — | Etichetta chiara |
| Avvia | Una run/seed è selezionata | Manifest validato | Entra in `FloorZero` con quel manifest | Avvia la run asincrona | Blocco input duplicati |

## Elementi ancora experimental (non decisi)

- Se esistono lobby custom con contenuti sbloccati: **no**, è un'idea futura parcheggiata (DEC-018), non nel gioco base.
- Gestione delle disconnessioni.
- Metriche di classifica oltre a tempo e punteggio.
- Regole di parità tra risultati identici.

Queste voci restano in `governance/open-questions.md` finché non vengono decise.

## Uscita

Uscire prima di "Avvia" non assegna alcuna sconfitta. Dopo l'avvio valgono le regole della
modalità (vedi `ui/results-and-leaderboards.md`).

## Non-obiettivi

- Non implementa matchmaking in tempo reale: non è la visione approvata.
- Non gestisce gli sblocchi di meta-progressione, esclusi per definizione dalle run competitive.

## Domande aperte residue

- Vedi `governance/open-questions.md`, sezione Multiplayer.

## Scenari verificabili

1. **Given** esistono run/seed pubblicate, **when** il giocatore apre `MultiplayerLobby`, **then** vede l'elenco senza dover attendere altri partecipanti online.
2. **Given** il giocatore seleziona una run/seed classificata, **when** consulta le regole, **then** vede modalità, versione e manifest usati per quella gara.
3. **Given** il giocatore esce dalla lobby prima di premere "Avvia", **when** torna a `MainMenu`, **then** nessuna sconfitta o penalità viene registrata.
4. **Given** il giocatore cerca contenuti sbloccati dalla meta-progressione in una run competitiva, **when** consulta i pool disponibili, **then** non li trova, perché esclusi per definizione (DEC-016).
