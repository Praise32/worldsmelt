---
id: gd-ui-navigation
status: approved
owner: design
last_reviewed: 2026-07-19
summary: "Mappa degli stati canonici (05-game-states-and-flow.md) e ritorno del focus. Ogni transizione rispetta la parità rigorosa di input (DEC-057, fonte unica in ui/options-and-accessibility.md)."
---

# Navigation Map

## Intento

Dare a giocatore e agenti un'unica mappa affidabile degli stati raggiungibili, coerente
con i nomi canonici di `05-game-states-and-flow.md`: `MainMenu, RunSetup, FloorZero,
Gameplay, PauseMenu, Options, BuildScreen, RunResults, ExitConfirm`.

## Mappa

```mermaid
flowchart TD
    MainMenu -->|Nuova run| RunSetup
    MainMenu -->|Continua: rientro nello stato salvato| Gameplay
    MainMenu --> Options
    MainMenu -->|Esci| ExitConfirm
    RunSetup --> FloorZero
    FloorZero -->|ESC| ExitConfirm
    FloorZero -->|piano 1 pronto| Gameplay
    Gameplay --> PauseMenu
    PauseMenu -->|Riprendi| Gameplay
    PauseMenu -->|Opzioni| Options
    Options -->|Indietro, focus su Opzioni| PauseMenu
    Options -->|Indietro| MainMenu
    PauseMenu -->|Build e sinergie| BuildScreen
    BuildScreen -->|Indietro| PauseMenu
    PauseMenu -->|Abbandona run| ExitConfirm
    ExitConfirm -->|Conferma abbandono preparazione, da FloorZero| MainMenu
    ExitConfirm -->|Conferma abbandono run, da PauseMenu| RunResults
    Gameplay -->|Boss piano 5 o morte| RunResults
    RunResults -->|Nuova run subito| FloorZero
    RunResults -->|Riprova stessa run, non classificata| FloorZero
    RunResults --> MainMenu
```

## Nota sui nodi

- `FloorZero` assorbe la vecchia schermata separata "GenerationStatus"/"GeneratingRun":
  lo stato di generazione è un indicatore dentro il Piano 0, non uno stato di
  navigazione a parte (vedi `ui/generation-status.md`).
- `Pause` non è un nome di nodo valido: lo stato canonico è `PauseMenu`.
- "Continua" rientra nello stato in cui la run è stata sospesa: `Gameplay` nel caso
  tipico, `FloorZero` se la sospensione è avvenuta nel Piano 0 (vedi `ui/main-menu.md`).
- La selezione multiplayer (`ui/multiplayer-lobby.md`) è `experimental` e NON fa parte
  del set canonico dei 9 stati: entrerà nella mappa solo quando la visione asincrona
  (DEC-016) verrà dettagliata e approvata.
- "Error Recovery" non è uno stato: gli errori di generazione si risolvono con fallback
  invisibile (regola, non stato); vedi `systems/generated-content-validation.md`.
- Il Catalogo (`ui/main-menu.md`) è una vista interna dello stato `MainMenu`, non un
  decimo stato: la mappa canonica resta a 9 stati (DEC-084).
- `ExitConfirm` è un solo nodo canonico ma la conferma porta a destinazioni diverse secondo
  la provenienza: da `PauseMenu` (abbandono di una **run in corso**) porta a `RunResults`,
  come una sconfitta con i punti ridotti visibili lì (DEC-089); da `FloorZero` (abbandono
  della sola **preparazione**, DEC-074) porta invece a `MainMenu`, perché nel Piano 0 non
  c'è ancora una run giocata da chiudere (perimetro DEC-082). Da `MainMenu` (chiusura del
  gioco), `ExitConfirm` è un dialogo modale leggero sopra il menu, non una schermata
  dedicata (DEC-090, fonte unica in `05-game-states-and-flow.md`, rimando, non riformulato
  qui).

## Regole comuni

- Ogni schermata definisce il proprio focus iniziale (vedi il documento dedicato).
- Il comando Indietro ha sempre un risultato prevedibile e torna alla schermata che ha aperto quella corrente.
- Le azioni distruttive (abbandonare la run, uscire dall'applicazione) passano sempre da `ExitConfirm`.
- Dopo la chiusura di una schermata secondaria, il focus torna all'elemento che l'ha aperta; l'arco `PauseMenu → Options` restituisce il focus sull'elemento "Opzioni" del menu pausa al ritorno.
- L'arco `FloorZero → ExitConfirm` (abbandono del Piano 0, DEC-074) segue la stessa regola: annullato l'abbandono, il focus torna esattamente dove si trovava nel Piano 0 (carta tema, scheda personaggio o pannello attivo) prima di ESC; confermato l'abbandono, la preparazione in corso si interrompe e si torna a `MainMenu` (nel Piano 0 non c'è una run giocata da chiudere, perimetro DEC-082).
- L'arco `PauseMenu → ExitConfirm` (abbandono di una **run in corso**) è diverso: annullato l'abbandono, il focus torna al `PauseMenu`; confermato, il giocatore entra in `RunResults` come una sconfitta, con i punti sblocco maturati fino a quel momento visibili ridotti lì (DEC-089). Non torna più direttamente a `MainMenu`: quella destinazione resta solo per l'abbandono della preparazione da `FloorZero`, sopra.
- Durante caricamenti critici (ingresso in `FloorZero`, transizione di piano), il sistema previene attivazioni duplicate dell'input.
- La pausa ferma la simulazione in singleplayer; nelle run competitive asincrone il tempo della run continua a contare mentre `PauseMenu` è aperto (coerente con DEC-016; vedi `ui/pause-menu.md`).
- Ogni transizione e ogni comando di questa mappa funziona in modo identico su tastiera e controller; il mouse è ammesso solo nei menu (il Piano 0 conta come menu, DEC-075, fonte unica in `ui/options-and-accessibility.md`). Fonte unica della regola di parità di input: `ui/options-and-accessibility.md` (DEC-057, rimando, non riformulato qui).

## Non-obiettivi

- Questo documento non descrive il contenuto di ciascuna schermata: solo le transizioni tra stati.
- Non introduce stati tecnici (retry, error, loading interno) che non siano visibili come decisione del giocatore.

## Domande aperte residue

- Nessuna: la ripresa di una run sospesa rientra direttamente nello stato salvato
  (regola sancita in `05-game-states-and-flow.md`).

## Scenari verificabili

1. **Given** il giocatore è in `MainMenu` senza run sospesa, **when** seleziona "Nuova run", **then** entra in `RunSetup` e non in `FloorZero` direttamente.
2. **Given** il giocatore è in `Gameplay` e apre la pausa, **when** seleziona "Opzioni" e poi torna indietro, **then** si ritrova in `PauseMenu` con il focus sull'elemento "Opzioni".
3. **Given** il giocatore è in `PauseMenu` e seleziona "Abbandona run", **when** conferma in `ExitConfirm`, **then** entra in `RunResults` con i punti sblocco ridotti visibili, come una sconfitta, e non torna direttamente a `MainMenu` (DEC-089).
4. **Given** il giocatore sconfigge il boss del piano 5, **when** la run termina, **then** il gioco entra in `RunResults` e non torna direttamente a `Gameplay`.
5. **Given** il giocatore è in `FloorZero` con il focus su una carta tema, **when** preme ESC e poi annulla in `ExitConfirm`, **then** torna a `FloorZero` con il focus di nuovo sulla stessa carta tema.
6. **Given** il giocatore è in `FloorZero` con la generazione dei piani in corso, **when** preme ESC e conferma l'abbandono in `ExitConfirm`, **then** la preparazione si interrompe e il gioco torna a `MainMenu` (a differenza dell'abbandono di una run in corso da `PauseMenu`, che ora entra in `RunResults`, DEC-089).
