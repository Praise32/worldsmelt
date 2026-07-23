---
id: gd-ui-multiplayer-lobby
title: Multiplayer Lobby
domain: design
status: experimental
authority: canonical
owner: design
summary: "Menu multiplayer a due assi (DEC-021: Leggera/Classificata × stesso seed/seed diversi) e selezione asincrona della gara (DEC-016); la Classificata si estende a tre istanze con la Classificata giornaliera pubblica ('Daily') e classifiche divise per metrica (DEC-062); la Daily premia con medaglie/cornici cosmetiche (DEC-064). La condivisione di una run tramite codice breve o file RunBundle (DEC-066) è un canale separato dalla lobby, sempre non classificato. I dettagli restano experimental."
last_reviewed: 2026-07-19
topics: [multiplayer, lobby, classificata, daily, DEC-016, DEC-021, DEC-062, DEC-066]
related: []
supersedes: []
source_files: []
---

# Multiplayer Lobby

## Intento

Permettere al giocatore di scegliere una gara asincrona sulla stessa run di altri
giocatori, coerentemente con la visione fissata in DEC-016. I dettagli restano
`experimental` e non sono ancora comportamento approvato.

## Visione fissata (DEC-016 + DEC-021 + DEC-062, approved)

- Le gare sono **asincrone**: non esiste una lobby live con partecipanti in attesa
  simultanea (DEC-016).
- Il menu offre due scelte indipendenti (DEC-021):
  **Modalità** = Leggera (non classificata) o Classificata;
  **Tipo di gara** = Stesso seed (stessa run esatta per tutti, stesso seed/manifest) o
  Seed diversi (ogni giocatore una run propria). Tutte e quattro le combinazioni esistono.
- Dentro la Modalità **Classificata**, il Tipo di gara si allarga a **tre istanze**
  (DEC-062): sfida a stesso seed, sfida a seed diversi, e la **Classificata giornaliera
  pubblica ("Daily")** — una run scelta dallo sviluppatore, uguale per tutti i giocatori,
  che cambia ogni giorno, con classifica globale giornaliera. La Modalità Leggera resta a
  due istanze (stesso seed / seed diversi): la Daily esiste solo in Classificata.
- Le classifiche si basano su tempo e punteggio, e sono **divise per metrica**: una
  graduatoria per il tempo e una separata per il punteggio, mai combinate (DEC-062).
  Valgono solo per la modalità Classificata.
- I pool sbloccati dalla meta-progressione sono esclusi dalle run competitive.

## Condizioni di ingresso

Da `MainMenu` ("Multiplayer").

## Focus iniziale

La run/seed pubblicata più recente o più rilevante per il giocatore.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Selettore Modalità: Leggera / Classificata | Sempre | Sempre | Sceglie la modalità (DEC-021) | Filtra le gare mostrate | La scelta corrente è sempre visibile |
| Selettore Tipo di gara: Stesso seed / Seed diversi / Daily | Sempre | Le tre opzioni sono disponibili solo con Modalità = Classificata (DEC-062); con Modalità = Leggera solo Stesso seed / Seed diversi | Sceglie il tipo di gara (DEC-021, esteso da DEC-062) | Filtra le gare mostrate | La scelta corrente è sempre visibile |
| Classificata giornaliera pubblica ("Daily") | Modalità = Classificata (DEC-062) | Sempre | Seleziona la Daily del giorno corrente | Mostra la run del giorno, uguale per tutti i giocatori | Conto alla rovescia al prossimo cambio, alle 00:00 UTC (DEC-081; fonte unica in `08-multiplayer-and-competition.md`), e se il giocatore l'ha già completata |
| Elenco gare pubblicate | Sempre | Almeno una gara disponibile per i filtri scelti | Seleziona una gara | Mostra dettagli della gara | Indica se già completata dal giocatore |
| Regole della gara | Una gara è selezionata | Sempre, se visibile | Consulta le regole | Mostra modalità, tipo di gara (inclusa l'istanza Daily se applicabile), versione e manifest | — |
| Avvia | Una gara è selezionata | Manifest validato (stesso seed, incluso Daily) o run generabile (seed diversi) | Entra in `FloorZero` | Avvia la run asincrona | Blocco input duplicati |

## Elementi ancora experimental (non decisi)

- Se esistono lobby custom con contenuti sbloccati: **no**, è un'idea futura parcheggiata (DEC-018), non nel gioco base.
- Gestione delle disconnessioni.
- Metriche di classifica oltre a tempo e punteggio.
- Regole di parità tra risultati identici.
- Criterio di normalizzazione per la Classificata a seed diversi (come garantire run
  confrontabili; vedi `08-multiplayer-and-competition.md`).

Queste voci restano in `governance/open-questions.md` finché non vengono decise. (L'orario di
rotazione della Daily è ora fissato: 00:00 UTC, DEC-081; fonte unica in
`08-multiplayer-and-competition.md`, rimando, non riformulato qui.)

## Condivisione fuori lobby (DEC-066, rimando)

La condivisione di una run tramite codice breve o file RunBundle (DEC-066) è un canale
distinto da questa lobby: non passa dalle gare pubblicate qui elencate e non richiede
l'ingresso in `MultiplayerLobby`. Chi riceve un codice o un RunBundle importa la run
direttamente da `RunSetup` ([ui/run-setup.md](run-setup.md)); il risultato resta sempre non
classificato, per la stessa ragione delle gare "Riprova la stessa run" in
`ui/results-and-leaderboards.md`. Fonte unica:
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md#condivisione-run-a-due-vie-dec-066)
(rimando, non riformulato qui).

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
6. **Given** il giocatore seleziona Modalità = Classificata, **when** apre il selettore Tipo di gara, **then** vede tre opzioni (stesso seed, seed diversi, Daily), mentre con Modalità = Leggera ne vede solo due (DEC-062).
7. **Given** il giocatore seleziona la Classificata giornaliera pubblica (Daily), **when** avvia la run, **then** riceve esattamente lo stesso seed di tutti gli altri giocatori che giocano la Daily quel giorno, e il risultato entra nella classifica globale giornaliera, divisa per tempo e per punteggio (DEC-062).
8. **Given** il giocatore riceve un codice breve di run condivisa da un altro giocatore, **when** lo incolla in `RunSetup` invece di passare dalla lobby, **then** ottiene una run rigenerata identica, sempre non classificata (DEC-066).
9. **Given** il giocatore ha aperto la lobby con Modalità = Classificata, **when** guarda la voce Daily, **then** vede il conto alla rovescia al prossimo cambio, calcolato sulle 00:00 UTC (DEC-081), non su un orario locale o europeo.
