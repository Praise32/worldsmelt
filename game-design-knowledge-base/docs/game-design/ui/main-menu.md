---
id: gd-ui-main-menu
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Azioni e stati del menu principale, incluso il focus con run sospesa."
---

# Main Menu

## Intento

Dare accesso rapido ad avviare o riprendere una run, alle modalità competitive, alla
meta-progressione e alle opzioni, senza mai bloccare l'accesso al singleplayer.

## Condizioni di ingresso

- All'avvio del gioco (stato `Boot → MainMenu`).
- Al ritorno da `RunResults`.
- Al ritorno da `ExitConfirm` con annullamento.

## Focus iniziale

- Se esiste una run sospesa valida, il focus iniziale è su **Continua** (decisione approvata).
- Altrimenti il focus iniziale è su **Nuova run**.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Continua | Esiste una run sospesa valida | Sempre, se visibile | Riprende la run sospesa | Entra in `FloorZero` o `Gameplay` nello stato salvato | Riceve il focus iniziale del menu |
| Nuova run | Sempre | Sempre | Se esiste una run sospesa, chiede conferma di abbandono; altrimenti apre `RunSetup` | Entra in `RunSetup` (dopo conferma, se richiesta) | Conferma mostrata come dialogo bloccante |
| Multiplayer | Se la modalità è disponibile (`experimental`) | Se almeno una run/seed pubblicata è raggiungibile | Apre la selezione di run/seed asincrona (DEC-016; vedi `ui/multiplayer-lobby.md`, fuori dal set canonico di stati) | Entra nella selezione | Indicazione se il servizio non è disponibile |
| Classifiche | Se disponibili | Sempre, se visibile | Apre `RunResults` in modalità consultazione | Mostra le classifiche | — |
| Catalogo/Museo | Sempre (DEC-015) | Sempre | Mostra il catalogo dei contenuti generati e il museo del Piano 0 | Apre la schermata del catalogo | — |
| Opzioni | Sempre | Sempre | Apre `Options` | Entra in `Options` | Al ritorno, focus sull'elemento "Opzioni" |
| Esci | Piattaforme applicabili | Sempre, se visibile | Apre `ExitConfirm` | Chiede conferma di uscita | Dialogo di conferma |

## Decisione approvata: focus e conferma di abbandono

Se esiste una run sospesa, "Continua" ha il focus iniziale invece di "Nuova run"; scegliere
"Nuova run" con una run sospesa attiva richiede una conferma esplicita di abbandono prima
di aprire `RunSetup`. La voce Catalogo/Museo è sempre presente (DEC-015).

## Navigazione

`MainMenu` è raggiungibile all'avvio del gioco, da `RunResults` e da `ExitConfirm`
(annullamento o conferma di abbandono run). Da `MainMenu` si raggiungono `RunSetup`, lo
stato salvato della run sospesa (via Continua), `Options`, il Catalogo/Museo e
`ExitConfirm`; la selezione multiplayer resta `experimental`.

## Comando Indietro

Da `MainMenu` il comando Indietro apre `ExitConfirm` sulle piattaforme dove è applicabile;
altrove non ha effetto.

## Stato di caricamento

Se un servizio opzionale (multiplayer, classifiche, catalogo remoto) non risponde, il menu
resta interamente navigabile: solo le voci dipendenti da quel servizio si disabilitano.

## Errori

Se un servizio opzionale non è disponibile, il singleplayer deve rimanere accessibile.
Nessun errore tecnico va mostrato nel menu; un messaggio descrittivo basta.

## Accessibilità

Tutte le voci sono raggiungibili da tastiera, controller e lettore di schermo nello stesso
ordine di navigazione della tabella.

## Non-obiettivi

- Questo documento non definisce il contenuto del Catalogo/Museo (vedi `systems/save-and-meta-progression.md`) né della selezione run/seed (vedi `ui/multiplayer-lobby.md`).

## Domande aperte residue

- Nessuna per questo documento: il comportamento del menu è coperto da DEC-004, DEC-015, DEC-016 e dalla decisione di focus/conferma qui sopra.

## Scenari verificabili

1. **Given** esiste una run sospesa valida, **when** il giocatore apre `MainMenu`, **then** il focus iniziale è su "Continua".
2. **Given** esiste una run sospesa valida, **when** il giocatore seleziona "Nuova run", **then** il gioco mostra una conferma di abbandono prima di aprire `RunSetup`.
3. **Given** non esiste alcuna run sospesa, **when** il giocatore apre `MainMenu`, **then** il focus iniziale è su "Nuova run" e la voce "Continua" non è visibile.
4. **Given** il servizio multiplayer non è disponibile, **when** il giocatore apre `MainMenu`, **then** la voce Multiplayer risulta disabilitata ma il resto del menu, incluso "Nuova run", resta pienamente utilizzabile.
