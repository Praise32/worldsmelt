---
id: gd-ui-run-setup
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Seed e modalità della run; tema e personaggio si scelgono nel Piano 0. Nessun selettore di difficoltà: la curva dei 5 piani è unica e uguale per tutti (DEC-038)."
---

# Run Setup

## Intento

Raccogliere le sole informazioni necessarie ad avviare una run e a determinarne la
confrontabilità competitiva, prima di entrare nel Piano 0.

## Condizioni di ingresso

Da `MainMenu` (Nuova run), da `RunResults` (Nuova run o Riprova stessa run).

## Focus iniziale

Il primo campo modificabile disponibile (seed o modalità), a seconda del punto di ingresso.

## Cosa NON sta qui

La scelta del tema della run e la scelta del personaggio non avvengono in `RunSetup`:
avvengono nel Piano 0 (DEC-004, DEC-005, DEC-014). Vedi `systems/floor-zero.md` per la
loro spec completa. `RunSetup` prepara solo l'identità tecnica della run.

Non esiste un selettore di difficoltà: la curva di difficoltà dei 5 piani è unica, uguale
per tutte le run e per tutti i giocatori (DEC-038, vedi
`07-difficulty-and-progression.md`). Questo rende le classifiche immediatamente
confrontabili, senza bisogno di normalizzazione per un livello di difficoltà scelto.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Seed | Sempre | Sempre | Imposta o genera un seed | Determina il manifest di run | Mostra il seed corrente |
| Modalità (standard) | Sempre | Sempre | Seleziona la modalità standard | Run classificabile | Indicazione di modalità attiva |
| Run condivisa tramite codice/manifest | Se il giocatore ha un codice | Sempre, se visibile | Importa seed e manifest condiviso | Prepara una run identica a quella condivisa | Validazione del codice |
| Modificatori sbloccati | Se il giocatore ne possiede | Fuori da modalità competitiva (DEC-015) | Attiva un modificatore | Cambia composizione dei pool | Indicazione se disattivati in competitivo |
| Avvia | Sempre | Scelte valide | Vedi "Azione Avvia" | Entra in `FloorZero` | Blocco input duplicati |

## Azione Avvia

1. Valida le scelte (seed, modalità, eventuali modificatori).
2. Crea l'identità/manifest della run.
3. Entra in `FloorZero`: la scelta del tema, la scelta del personaggio e l'indicatore di generazione avvengono lì, non prima.
4. Impedisce input duplicati durante la transizione.

## Regola

Il giocatore deve sapere quali opzioni incidono su classifiche e confrontabilità: modalità,
seed condiviso e modificatori sbloccati vanno etichettati come tali (DEC-016).

## Idee future (non requisiti)

- **Modalità caos** (DEC-018): parcheggiata come idea futura, non è più un'opzione attiva in questa schermata.

## Non-obiettivi

- Non gestisce la scelta di tema o personaggio (vedi `systems/floor-zero.md`).
- Non gestisce la generazione dei contenuti (vedi `ui/generation-status.md`).

## Domande aperte residue

- Nessuna specifica a questa schermata; per l'economia degli sblocchi vedi `governance/open-questions.md`.

## Scenari verificabili

1. **Given** il giocatore apre `RunSetup` da "Nuova run", **when** conferma "Avvia" senza modificare nulla, **then** il gioco crea un manifest con seed casuale ed entra in `FloorZero`.
2. **Given** il giocatore importa un codice di run condivisa valido, **when** conferma "Avvia", **then** il manifest riprodotto è identico a quello condiviso.
3. **Given** una modalità competitiva è attiva, **when** il giocatore prova ad attivare un modificatore sbloccato, **then** l'elemento risulta disabilitato con un'indicazione del motivo.
4. **Given** il giocatore cerca la modalità caos, **when** consulta `RunSetup`, **then** non la trova: è documentata solo come idea futura (DEC-018).
5. **Given** il giocatore apre `RunSetup`, **when** cerca un'opzione per scegliere il livello di difficoltà, **then** non la trova: nessun livello di difficoltà è selezionabile, la curva dei 5 piani è unica per tutte le run (DEC-038).
