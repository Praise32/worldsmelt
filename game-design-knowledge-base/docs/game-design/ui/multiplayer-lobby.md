---
id: gd-ui-multiplayer-lobby
status: experimental
owner: design
last_reviewed: 2026-07-18
summary: "Menu multiplayer a due assi (DEC-021: Leggera/Classificata × stesso seed/seed diversi) e selezione asincrona della gara (DEC-016); i dettagli restano experimental."
---

# Multiplayer Lobby

## Intento

Permettere al giocatore di scegliere una gara asincrona sulla stessa run di altri
giocatori, coerentemente con la visione fissata in DEC-016. I dettagli restano
`experimental` e non sono ancora comportamento approvato.

## Visione fissata (DEC-016 + DEC-021, approved)

- Le gare sono **asincrone**: non esiste una lobby live con partecipanti in attesa
  simultanea (DEC-016).
- Il menu offre due scelte indipendenti (DEC-021):
  **Modalità** = Leggera (non classificata) o Classificata;
  **Tipo di gara** = Stesso seed (stessa run esatta per tutti, stesso seed/manifest) o
  Seed diversi (ogni giocatore una run propria). Tutte e quattro le combinazioni esistono.
- Le classifiche si basano su tempo e punteggio; valgono solo per la modalità
  Classificata.
- I pool sbloccati dalla meta-progressione sono esclusi dalle run competitive.

## Condizioni di ingresso

Da `MainMenu` ("Multiplayer").

## Focus iniziale

La run/seed pubblicata più recente o più rilevante per il giocatore.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Selettore Modalità: Leggera / Classificata | Sempre | Sempre | Sceglie la modalità (DEC-021) | Filtra le gare mostrate | La scelta corrente è sempre visibile |
| Selettore Tipo di gara: Stesso seed / Seed diversi | Sempre | Sempre | Sceglie il tipo di gara (DEC-021) | Filtra le gare mostrate | La scelta corrente è sempre visibile |
| Elenco gare pubblicate | Sempre | Almeno una gara disponibile per i filtri scelti | Seleziona una gara | Mostra dettagli della gara | Indica se già completata dal giocatore |
| Regole della gara | Una gara è selezionata | Sempre, se visibile | Consulta le regole | Mostra modalità, tipo di gara, versione e manifest | — |
| Avvia | Una gara è selezionata | Manifest validato (stesso seed) o run generabile (seed diversi) | Entra in `FloorZero` | Avvia la run asincrona | Blocco input duplicati |

## Elementi ancora experimental (non decisi)

- Se esistono lobby custom con contenuti sbloccati: **no**, è un'idea futura parcheggiata (DEC-018), non nel gioco base.
- Gestione delle disconnessioni.
- Metriche di classifica oltre a tempo e punteggio.
- Regole di parità tra risultati identici.
- Criterio di normalizzazione per la Classificata a seed diversi (come garantire run
  confrontabili; vedi `08-multiplayer-and-competition.md`).

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

1. **Given** esistono gare pubblicate, **when** il giocatore apre la lobby, **then** vede i due selettori (Modalità, Tipo di gara) e l'elenco filtrato, senza dover attendere altri partecipanti online.
2. **Given** il giocatore seleziona una gara Classificata a stesso seed, **when** consulta le regole, **then** vede modalità, tipo di gara, versione e manifest usati per quella gara.
3. **Given** il giocatore esce dalla lobby prima di premere "Avvia", **when** torna a `MainMenu`, **then** nessuna sconfitta o penalità viene registrata.
4. **Given** il giocatore cerca contenuti sbloccati dalla meta-progressione in una run competitiva, **when** consulta i pool disponibili, **then** non li trova, perché esclusi per definizione (DEC-016).
5. **Given** il giocatore sceglie Leggera + Seed diversi, **when** avvia, **then** riceve una run propria e il risultato non entra in classifica.
