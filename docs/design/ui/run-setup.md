---
id: gd-ui-run-setup
title: Run Setup
domain: design
status: approved
authority: canonical
owner: design
summary: "Seed e modalità della run; tema e personaggio si scelgono nel Piano 0. Nessun selettore di difficoltà: la curva dei 5 piani è unica e uguale per tutti (DEC-038). Le run condivise tramite codice breve o file RunBundle sono sempre non classificate (DEC-066); il codice breve trasporta anche tema e personaggio scelti, preselezionati nel Piano 0 (DEC-077)."
last_reviewed: 2026-07-31
last_verified_commit: 50911c9
topics: [run-setup, seed, runbundle, condivisione-run, difficolta, modalita, DEC-038, DEC-066, DEC-077, DEC-171, WP22]
related: []
supersedes: []
source_files: []
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
| Modalità (standard) | Sempre | Non selezionabile (unica modalità esistente) | Nessuna: solo indicazione, non è una voce di menu | Run classificabile | Indicazione di modalità attiva |
| Codice breve di run condivisa (DEC-066/DEC-077) | Sempre | Sempre | Incolla un codice breve (seed, versione di gioco, tema e personaggio scelti — contenuto esatto in [Run Manifest and Reproducibility](../systems/run-manifest-and-reproducibility.md), fonte unica, rimando non riformulato qui) ricevuto da un altro giocatore | Rigenera localmente una run identica a quella condivisa, con tema e personaggio già preselezionati nel Piano 0 (il giocatore può comunque cambiarli lì: a quel punto è solo una run con lo stesso seed, non più la stessa run condivisa) | Validazione del codice; conferma se la versione di gioco combacia |
| Importa file RunBundle (DEC-066) | Sempre | Sempre | Seleziona un file RunBundle esportato (formato con verifica d'integrità) | Ricostruisce la run esportata in modo verificabile | Verifica d'integrità del file; errore chiaro se corrotto o incompatibile |
| Modificatori sbloccati | Se il giocatore ne possiede | Fuori da modalità competitiva (DEC-015) | Attiva un modificatore | Cambia composizione dei pool | Indicazione se disattivati in competitivo |
| Avvia | Sempre | Scelte valide | Vedi "Azione Avvia" | Entra in `FloorZero` | Blocco input duplicati |

> **Nota di implementazione (WP22, 2026-07-31, gap G10 di `ui-cornice`; tabella corretta
> nella seconda passata e posizione corretta nella terza, lo stesso giorno):** la riga
> "Modalità: Standard" è disegnata da `DrawRunSetupOverlay`
> (`src/render/game_renderer.c`) come **etichetta fissa, non selezionabile** — sempre
> visibile, senza un proprio indice di menu, senza azione da compiere: l'unica modalità
> esistente non ha nulla su cui navigare. **Sta sopra le tre voci** ("Seed", "Avvia",
> "Indietro"), nella fascia libera fra il filetto del titolo e la prima voce: le prime due
> passate la dicevano "fra le righe Seed e Avvia" ma era disegnata a `box.y + 142`, cioè
> **dentro** la fascia della voce "Seed" (110..150, `MenuItemRectFor`), sovrapposta al suo
> bordo inferiore. La terza passata l'ha spostata a
> `box.y + 78` — posizione senza sovrapposizioni, registrata come **default proposto
> dall'implementazione** (vedi sotto e `governance/open-questions.md`, voce 57) perché
> questo documento non fissa alcuna posizione — e le ha dato il test che non aveva:
> `--run-setup-mode-line-test` (`GameRunSetupModeLineTest`, `src/tests/game_tests.c`)
> fallisce se la riga sparisce dal frame disegnato, se diventa selezionabile o se torna a
> sovrapporsi a una voce; `--layout-test` (voce `h`) ripete la sola verifica geometrica
> come nucleo puro. Prima non era coperta da nulla: cancellarne la `UiText` lasciava
> `make test` interamente verde. La prima passata aveva inoltre lasciato la cella
> "Abilitato quando" della tabella sopra a "Sempre" e "Azione" a "Seleziona la modalità
> standard", in contraddizione con la riga NON selezionabile appena descritta: la tabella
> dichiara ora "Non selezionabile" e "Nessuna azione", allineata al comportamento reale.
> La riga non è un'aggiunta di questo lavoro: risale alla M1a (18/07). Dichiara la modalità
> attiva "con stile, senza promettere nulla" — le modalità competitive sono fuori dallo
> scope della demo (DEC-171) — senza introdurre alcun selettore: coerente con "Nessun
> selettore di difficoltà" più sotto e con l'assenza di qualunque modalità competitiva
> reale nel motore oggi.

### Default proposti dall'implementazione

Non sono canone (stile DEC-019): valgono finché il proprietario non decide diversamente, e
ciascuno ha la sua voce in `governance/open-questions.md`.

| Default | Valore adottato | Voce |
|---|---|---|
| Posizione della riga informativa "Modalità: Standard" | `box.y + 78`, sopra le tre voci, nella fascia libera fra il filetto del titolo e la riga "Seed" (nessuna sovrapposizione a nessuna voce, a ogni risoluzione) | 57 |

## Azione Avvia

1. Valida le scelte (seed, modalità, eventuali modificatori, eventuale codice breve o file
   RunBundle importato).
2. Crea l'identità/manifest della run.
3. Entra in `FloorZero`: la scelta del tema, la scelta del personaggio e l'indicatore di generazione avvengono lì, non prima. Se la run viene da un codice breve di run condivisa, tema e personaggio arrivano già preselezionati (DEC-077); il giocatore resta libero di cambiarli lì.
4. Impedisce input duplicati durante la transizione.

## Condivisione run a due vie (DEC-066)

`RunSetup` accetta due modi per preparare una run condivisa da un altro giocatore, fuori
dalle classifiche:

1. **Codice breve testuale** (seed, versione di gioco, tema e personaggio scelti — contenuto
   esatto in
   [Run Manifest and Reproducibility](../systems/run-manifest-and-reproducibility.md),
   fonte unica, rimando non riformulato qui, DEC-077): il giocatore lo incolla nel campo
   dedicato; il gioco rigenera localmente i contenuti dallo stesso seed, con tema e
   personaggio già preselezionati nel Piano 0. Il giocatore resta libero di cambiarli lì: a
   quel punto sta giocando una run con lo stesso seed, non più la stessa run condivisa.
   Richiede che la versione di gioco combaci per garantire la stessa generazione.
2. **File RunBundle esportato**: il formato con verifica d'integrità già esistente nel
   progetto (vedi
   [Run Manifest and Reproducibility](../systems/run-manifest-and-reproducibility.md)); una
   via completa e verificabile, adatta a gare private e archivio.

In entrambi i casi la run che ne risulta è **sempre non classificata** (coerenza con
DEC-062: la Classificata passa solo dalle gare pubblicate o dalla Daily, vedi
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md#condivisione-run-a-due-vie-dec-066),
rimando, non riformulato qui).

## Regola

Il giocatore deve sapere quali opzioni incidono su classifiche e confrontabilità: modalità,
seed condiviso e modificatori sbloccati vanno etichettati come tali (DEC-016); una run
preparata da codice breve o file RunBundle condiviso è sempre etichettata come non
classificata (DEC-066).

## Idee future (non requisiti)

- **Modalità caos** (DEC-018): parcheggiata come idea futura, non è più un'opzione attiva in questa schermata.

## Non-obiettivi

- Non gestisce la scelta di tema o personaggio (vedi `systems/floor-zero.md`).
- Non gestisce la generazione dei contenuti (vedi `ui/generation-status.md`).

## Domande aperte residue

- Dove va la riga informativa "Modalità: Standard": voce 57 di
  `governance/open-questions.md` (default proposto dall'implementazione, vedi la tabella
  sopra). Per l'economia degli sblocchi vedi lo stesso documento.

## Scenari verificabili

1. **Given** il giocatore apre `RunSetup` da "Nuova run", **when** conferma "Avvia" senza modificare nulla, **then** il gioco crea un manifest con seed casuale ed entra in `FloorZero`.
2. **Given** il giocatore importa un codice breve o un file RunBundle di run condivisa valido (DEC-066), **when** conferma "Avvia", **then** il manifest riprodotto è identico a quello condiviso, ed è sempre non classificato.
3. **Given** una modalità competitiva è attiva, **when** il giocatore prova ad attivare un modificatore sbloccato, **then** l'elemento risulta disabilitato con un'indicazione del motivo.
4. **Given** il giocatore cerca la modalità caos, **when** consulta `RunSetup`, **then** non la trova: è documentata solo come idea futura (DEC-018).
5. **Given** il giocatore apre `RunSetup`, **when** cerca un'opzione per scegliere il livello di difficoltà, **then** non la trova: nessun livello di difficoltà è selezionabile, la curva dei 5 piani è unica per tutte le run (DEC-038).
6. **Given** il giocatore incolla un codice breve di run condivisa valido con la stessa versione di gioco, **when** conferma "Avvia", **then** ottiene una run identica a quella condivisa (stesso seed, stessa versione, stesso tema e stesso personaggio), etichettata come non classificata (DEC-066); tema e personaggio arrivano già preselezionati nel Piano 0 (DEC-077).
7. **Given** il giocatore importa un file RunBundle valido, **when** la verifica d'integrità passa, **then** la run viene ricostruita in modo verificabile e resta comunque non classificata (DEC-066).
8. **Given** il giocatore ha avviato una run da un codice breve condiviso, **when** nel Piano 0 cambia il tema o il personaggio preselezionato, **then** la run prosegue con lo stesso seed ma non è più la stessa run condivisa (DEC-077).
9. **Given** il giocatore apre `RunSetup`, **when** guarda la schermata e prova a navigare o a fare clic sulla riga "Modalità: Standard", **then** la riga è visibile e leggibile, non si sovrappone a nessuna delle tre voci selezionabili, e non riceve mai il fuoco né risponde al clic: è un'indicazione, non una voce di menu.
