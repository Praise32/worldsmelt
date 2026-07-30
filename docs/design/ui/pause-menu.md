---
id: gd-ui-pause
title: Pause Menu
domain: design
status: approved
authority: canonical
owner: design
summary: "La pausa ferma la simulazione in singleplayer; il tempo continua in asincrono competitivo. Espone anche l'elenco delle prove specifiche della run, sempre consultabile (DEC-042), ed è il punto in cui l'HUD di combattimento resta consultabile su richiesta durante il Piano 0, dove è nascosto (DEC-169)."
last_reviewed: 2026-07-31
last_verified_commit: c5c1bc4
topics: [pause-menu, pausa, prove, abbandono-run, reroll, DEC-042, DEC-082, DEC-089, DEC-114, DEC-159, DEC-169, WP21, WP19, WP16, WP15a, Piano-0, consultazione]
related: []
supersedes: []
source_files: [src/render/game_renderer.c, src/app/app.c, src/core/game_types.h]
---

# Pause Menu

## Intento

Permettere di interrompere temporaneamente l'input di gioco, consultare la build e
accedere alle opzioni senza perdere progressi né subire azioni distruttive accidentali.

## Condizioni di ingresso

Da `Gameplay`, tramite il comando di pausa. Dal **Piano 0** con lo stesso comando, per la
sola consultazione dell'HUD nascosto (DEC-169, vedi sotto): è un **default proposto
dall'implementazione**, non canone — la domanda aperta 22 resta del proprietario.

## Focus iniziale

Il focus iniziale è su "Riprendi".

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Riprendi | Sempre | Sempre | Chiude `PauseMenu` | Torna in `Gameplay` | La simulazione riprende da dove si era fermata |
| Visualizza build e sinergie | Sempre | Sempre | Apre `BuildScreen` | Entra in `BuildScreen` | Al ritorno, focus su questo elemento |
| Prove | Da quando le prove sono state presentate all'ingresso nel piano 1 (DEC-042) | Sempre, se visibile | Apre l'elenco delle prove specifiche della run | Mostra le prove attive e il relativo stato di completamento | Al ritorno, focus su questo elemento |
| Opzioni | Sempre | Sempre | Apre `Options` | Entra in `Options` | Al ritorno, focus sull'elemento "Opzioni" |
| Rigenera la run | Sempre (nessuna condizione di modalità implementata oggi, WP21) | Sempre, se visibile | Chiede conferma tramite `ExitConfirm` | Reroll di DEC-089: salta i risultati, accredita i punti in silenzio, riparte dal Piano 0 con un seed NUOVO (mai lo stesso) | Conferma esplicita richiesta; è l'UNICA via per il reroll a nuovo seed (DEC-114) |
| Abbandona run | Sempre | Sempre | Chiede conferma tramite `ExitConfirm` | Entra in `RunResults`; la run si chiude come sconfitta, con punti sblocco ridotti visibili (DEC-082, DEC-089) | Conferma esplicita richiesta |

## Decisione approvata: la pausa ferma la simulazione

In singleplayer, `PauseMenu` ferma la simulazione: nemici, timer di run e generazione in
primo piano si sospendono finché il giocatore non seleziona "Riprendi" (approved).

## Nota: run competitive asincrone

Nelle run competitive asincrone (DEC-016), il tempo della run continua a contare mentre
`PauseMenu` è aperto, perché la metrica di classifica è il tempo totale della run
pubblicata: mettere in pausa non deve poter essere usato per "congelare" un vantaggio
temporale. Questa distinzione va comunicata al giocatore quando la modalità è competitiva.

## Regole

- Nessuna azione distruttiva immediata **come funzione di gioco**: il reroll (nuovo seed, DEC-114) e l'abbandono passano sempre da una conferma esplicita (`ExitConfirm`). Il tasto rapido `R` in `Gameplay` (riga sotto) è un'eccezione dichiarata a questa regola, non una sua violazione silenziosa.
- Il reroll (nuovo seed) non ha tasti rapidi diretti in `Gameplay` (DEC-114): l'unica collocazione è la voce "Rigenera la run" di questo menu. Il tasto `R` in `Gameplay` resta invece SOLO il **reset rapido di sviluppo** della stessa run (stesso seed, mai un seed nuovo): butta via l'intero progresso della run corrente SENZA alcuna conferma. Nessuna decisione approvata autorizza oggi questo comportamento come funzione di gioco rivolta al giocatore finale — anzi DEC-114 dice il contrario ("nessun tasto rapido diretto: buttare una run per un tasto sbagliato è il caso peggiore"). Resta invariato da prima di questo lavoro (il mandato di WP21 è solo il reroll a nuovo seed, non questo tasto): è un residuo dei comandi di sviluppo, mai formalizzato come funzione di gioco. **Default proposto dall'implementazione, non canone** (stile DEC-019): vedi `../governance/open-questions.md`, la voce aggiunta da WP21.
- Il focus iniziale è "Riprendi".
- Tornando da `Options`, il focus ritorna sull'elemento "Opzioni" di `PauseMenu`.
- Tornando da `BuildScreen`, il focus ritorna sull'elemento "Visualizza build e sinergie".
- Tornando dall'elenco delle prove, il focus ritorna sull'elemento "Prove".
- L'abbandono confermato conta come sconfitta ai fini dei punti sblocco (DEC-082) e porta a `RunResults`, non più a `MainMenu` diretto (DEC-089); il dettaglio dei punti ridotti, dell'esito mostrato e del ritorno al menu da lì è definito in `ui/results-and-leaderboards.md`, non ripetuto qui.

> **Nota di implementazione (WP19, 2026-07-30):** il case `APP_EXIT_CONFIRM`
> (`src/app/app.c`) distingue ora l'abbandono di una **run vera** da quello della sola
> **preparazione** con un'unica guardia, `game->floor >= 1` (vera solo dopo
> `WorldStartFloor`, mai durante il Piano 0): sopra la soglia, il ramo `exitAbandonsRun`
> chiama `TrialsFinalizeAtRunEnd` (le prove ancora in corso si chiudono come a fine run
> vera, WP16, mai lasciate `TRIAL_IN_PROGRESS`), scrive il catalogo con l'esito
> `RUN_CATALOG_OUTCOME_ABANDON` (invariato), imposta `Game.runAbandoned` (nuovo campo,
> `src/core/game_types.h`, letto da `DrawRunResultsOverlay` per la riga "Causa: abbandono
> volontario.", distinta dal colpo letale di DEC-159 che qui resta vuoto per costruzione)
> ed entra in `APP_RUN_RESULTS`. Sotto la soglia (Piano 0, sia da `ExitConfirm` diretto
> sia da `PauseMenu` aperto dal Piano 0, WP15a `pauseFromFloorZero`) il comportamento resta
> quello storico verso `APP_MAIN_MENU` (DEC-074), invariato da questo lavoro. Verificato da
> `--states-test` (`src/tests/game_tests.c`), `--catalog-test`
> (`src/tests/catalog_tests.c`, test G) e `--arena-hub-test`
> (`src/tests/floor_zero_arena_tests.c`, blocco (m)).

> **Nota di implementazione (WP21, 2026-07-31):** chiude il gap dichiarato da
> DEC-114 ("oggi il tasto `R` rigenera direttamente"). "Rigenera la run" è ora
> la quinta riga di `PauseMenu` (indice 4, tra "Opzioni" e "Abbandona run", che
> scala a indice 5 — sei righe in tutto,
> `MenuItemCountForMode`/`DrawPauseMenuOverlay` in
> `src/render/game_renderer.c`), visibile ogni volta che questo menu è
> raggiungibile (da `Gameplay` come da `PauseMenu` aperto dal Piano 0, WP15a —
> nessuna condizione di modalità implementata: l'unica modalità esistente oggi
> è quella singleplayer, DEC-038). Confermarla apre `ExitConfirm` con un nuovo
> campo dedicato, `AppUi.exitRerollsRun` (`src/core/game_types.h`), mutuamente
> esclusivo con `exitAbandonsRun` che governa l'abbandono: i due contesti non
> possono mai essere veri insieme, e ogni punto che ne accende uno spegne
> esplicitamente l'altro. Confermato in `ExitConfirm` ("Rigenerare la run con
> un nuovo seed? Il progresso non salvato si perde."), il reroll segue
> ESATTAMENTE la strada di DEC-089 ("il reroll salta i risultati"): scrive il
> catalogo in silenzio con l'esito `RUN_CATALOG_OUTCOME_ABANDON` (stessa
> funzione-hook di vittoria/sconfitta/abbandono, mai `RunResults`) e riparte
> SUBITO con un seed nuovo (`NextGenSeed`) attraverso `AppEnterFloorZero`, la
> stessa via canonica di `RunSetup`/Avvia e di `RunResults`/"Nuova run
> subito" — nessuna generazione duplicata, nessuna seconda via. Annullare
> (`ExitConfirm`/"Annulla") non tocca `Game` in alcun modo: si torna in
> `PauseMenu` col focus sull'indice 4, esattamente come ogni altro annullamento
> di questo menu. Il vecchio comportamento del tasto `R` **in `Gameplay`** (con
> la generazione abilitata, chiamava direttamente `AppEnterFloorZero` con un
> seed nuovo, senza alcuna conferma — proprio il gap che DEC-114 dichiarava) è
> rimosso: in `Gameplay`, `R` chiama oggi sempre e soltanto
> `game->resetQueued` (il reset rapido stesso seed, invariato — vedi "Regole"
> sopra per il suo status di eccezione dichiarata, non ancora una DEC).
> `MenuBoxForModeFor` riserva ora 560px di altezza per `APP_PAUSE_MENU` (non
> più 400, come `BuildScreen`) per fare spazio alla sesta riga; il riquadro di
> consultazione del Piano 0 (`DrawPauseMenuFloorZeroConsult`, DEC-169 sotto)
> segue di conseguenza.
>
> **Correzione (secondo tentativo, bocciatura 2026-07-31):** la conferma in
> `ExitConfirm` cancella ora ESPLICITAMENTE, PRIMA di `AppEnterFloorZero`, i
> runner di primo piano ancora attivi (`AppCancelFloorZeroGeneration`) e la
> ripresa in sottofondo (`AppStopLazyGeneration`) — no-op sicuri se non stanno
> girando, come li usa già il ramo gemello dell'abbandono. Senza, confermare
> "Rigenera la run" dal `PauseMenu` aperto DAL Piano 0 (`pauseFromFloorZero`)
> mentre `gen->proposeRunner`/`gen->runner` sono ancora `RUNNING` (~8-12s o
> minuti), o dal `PauseMenu` di una run vera mentre `gen->lazyRunner` riprende
> i piani 2-5 in sottofondo, avrebbe azzerato con `memset` un runner ancora
> vivo (`GenRunnerStartWithArgs`, `src/gen/gen_runner.c`): il pid del processo
> figlio si sarebbe perso, senza ucciderlo né raccoglierlo, con un secondo
> `melting-gen` in parallelo — esattamente ciò contro cui il codice si difende
> altrove ("mai due `melting-gen` insieme").
>
> Verificato da `--states-test`
> (`src/tests/game_tests.c`, blocco "PauseMenu -> Rigenera la run": conferma ->
> nuovo `runSeed` e ritorno a `Gameplay` via Piano 0; annulla -> nessun
> effetto — con `gen.enabled = false` per tutto `GameStatesTest`, quindi NON
> prova il caso `R` con la generazione abilitata), `--floor-zero-test`
> (`src/tests/game_tests.c`, `GameFloorZeroTest`, scenario 13: con
> `gen.enabled = true` e una run vera completata via `tests/fake-gen.sh`, `R`
> in `Gameplay` resta `APP_GAMEPLAY`/`resetQueued`, `game->runSeed` e
> `gen.pendingGenSeed` invariati, nessun `proposeRunner` avviato — la prova
> diretta del gap dichiarato da DEC-114, assente nel primo tentativo di questo
> WP), `--arena-hub-test` (`src/tests/floor_zero_arena_tests.c`, blocco (m),
> indici aggiornati) e `--layout-test` (geometria delle sei righe dentro il
> riquadro, nessuna sovrapposizione).

## Prove (DEC-042)

Le prove specifiche della run (fisse o generate, DEC-027) vengono presentate al giocatore
al passaggio dal Piano 0 al piano 1 e restano sempre consultabili da qui, senza bisogno di
attendere la fine della run. Il dettaglio di quando e come vengono presentate è definito in
`systems/floor-zero.md`; il dettaglio del loro contenuto e del punteggio bonus è definito in
`systems/rewards-and-economy.md`. Questo documento non ripete quei dettagli, colloca solo
la voce di menu.

> **Nota di implementazione (WP16, 2026-07-30, aggiornata 30/07 seconda tornata, indici
> aggiornati 31/07 da WP21):** "Prove" è la terza riga di `PauseMenu` (indice 2, tra
> "Visualizza build e sinergie" e "Opzioni" — sei righe in tutto oggi (cinque prima di WP21),
> `DrawPauseMenuOverlay`/`src/render/game_renderer.c`), visibile
> ogni volta che questo menu è raggiungibile (solo da `Gameplay`, cioè sempre dopo che le
> prove sono state assegnate). Confermarla apre un pannello INTERNO a `PauseMenu` (nessun
> nuovo `AppMode`, stessa scelta architetturale del Catalogo dentro `APP_MAIN_MENU`): elenco
> delle prove con testo e stato ("in corso"/"superata"/"fallita"/"annullata", mai il solo
> colore, DEC-058) più il totale "N/M superate, +X punti" (`TrialsPassedCount`/
> `TrialsCountedTotal`/`TrialsBonusTotal`, la stessa fonte che `RunResults` e `BuildScreen`
> leggono, così le tre schermate non possono mai divergere). Il quarto stato, "annullata"
> (`TRIAL_VOID`), è una prova scartata perché il suo archetipo non è mai comparso in questa
> run (vedi `systems/rewards-and-economy.md`, "Casi limite"): esclusa dal denominatore M,
> mai contata contro il giocatore. ESC o INVIO chiudono il pannello e riportano il focus
> sull'indice 2, come da tabella sopra. Mentre il pannello è aperto il mouse è sospeso sulle
> righe di menu sottostanti (stessa esclusione del Catalogo dentro `APP_MAIN_MENU`,
> `src/app/app.c`): senza questa guardia un click o un hover sui rettangoli invisibili delle
> righe di `PauseMenu` chiuderebbe il pannello o ne corromperebbe il focus a caso.

## Consultazione dell'HUD nel Piano 0 (DEC-169)

Nel Piano 0 l'HUD di combattimento è **nascosto** durante l'esplorazione dell'hub e torna
visibile solo nelle prove del Piano 0 — arene di sfida e tutorial integrato (DEC-047), da non
confondere con le «prove» specifiche della run di DEC-042 di cui sopra (fonte unica della
regola: [HUD](hud.md), dettaglio del Piano 0 in `systems/floor-zero.md`). Il menu di pausa è
il punto in cui quell'informazione resta
**consultabile su richiesta**: chi vuole controllare salute, risorse e build mentre la run è
ancora in preparazione lo fa da qui, senza uscire dal Piano 0 e senza che l'hub mostri l'HUD
in permanenza. Questo documento colloca la consultazione, non ridefinisce il contenuto
dell'HUD né la regola di visibilità.

DEC-169 **non fissa il comando** con cui il menu di pausa si apre dal Piano 0: ESC è già
assegnato a `ExitConfirm` (DEC-074) e le condizioni di ingresso qui sopra prevedono la sola
provenienza da `Gameplay`. Il punto resta la domanda aperta
(`../governance/open-questions.md`, punto 22): il default qui sotto **non la chiude**.

> **Nota di implementazione (WP15a, 2026-07-30, supera la nota W3 del 2026-07-28):** il
> riquadro di consultazione è disegnato in `DrawPauseMenuOverlay`
> (`src/render/game_renderer.c`), condizionato solo a `game->floor == 0` — indipendente da
> quale comando abbia aperto `PauseMenu`, come previsto. Mostra salute (cuori) e risorse
> (`Ingots`/`Blast Charges`/`Cast Keys`/`Flux`), le stesse informazioni della riga vitali
> dell'HUD di `Gameplay`.
>
> **DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019) per la domanda aperta 22 — non
> canone:** dal Piano 0 il menu di pausa si apre con il **comando di pausa**, lo stesso di
> `Gameplay`. Gli altri due candidati erano già occupati e non si potevano riusare senza una
> perdita: ESC è `ExitConfirm` (DEC-074) e TAB è il pannello mondi/personaggi del Piano 0
> (M5/M6a, con l'invito "TAB per le carte" scritto a schermo). Il comando di pausa era
> l'unico tasto ancora libero nel Piano 0 ed è anche l'unico che significhi già "fermati e
> guarda lo stato" per chi ha giocato una run. `AppUi.pauseFromFloorZero` (zero-default
> falso = provenienza `Gameplay`, comportamento storico invariato) è l'unica cosa che
> cambia: decide dove torna "Riprendi". Le altre righe del menu restano quelle di sempre —
> "Abbandona run" dal Piano 0 interrompe la preparazione esattamente come l'ESC di DEC-074,
> generazione in sottofondo annullata compresa. Verificato da `--arena-hub-test`
> (`src/tests/floor_zero_arena_tests.c`, blocco (l)).

## Non-obiettivi

- Non spiega le sinergie o la fusione: rimanda a `ui/inventory-and-synergy-screen.md`.
- Non gestisce le opzioni: rimanda a `ui/options-and-accessibility.md`.
- Non definisce il contenuto o il punteggio delle prove: rimanda a `systems/rewards-and-economy.md` e `systems/floor-zero.md`.
- Non definisce il contenuto di `RunResults` né il ritorno al menu da lì: rimanda a `ui/results-and-leaderboards.md`.

## Domande aperte residue

- Il comportamento della pausa è approved sia in singleplayer sia in asincrono competitivo.
- Con quale comando il menu di pausa si apre dal Piano 0, dove DEC-169 lo indica come luogo
  di consultazione dell'HUD ma ESC è già assegnato a `ExitConfirm` (DEC-074):
  `../governance/open-questions.md`, punto 22. **Aggiornamento 30/07 (WP15a):** esiste ora
  un default proposto e implementato — il comando di pausa — che **non chiude** la domanda:
  la scelta resta del proprietario.

## Scenari verificabili

1. **Given** il giocatore è in `Gameplay` in singleplayer, **when** apre `PauseMenu`, **then** nemici e timer di run si fermano finché non seleziona "Riprendi".
2. **Given** il giocatore è in una run competitiva asincrona, **when** apre `PauseMenu`, **then** il tempo della run continua a contare, con un'indicazione visibile di questo comportamento.
3. **Given** il giocatore è in `PauseMenu` e apre `Options`, **when** torna indietro, **then** il focus è sull'elemento "Opzioni" di `PauseMenu`.
4. **Given** il giocatore seleziona "Abbandona run", **when** conferma in `ExitConfirm`, **then** entra in `RunResults`, che mostra la run chiusa come sconfitta con i punti sblocco maturati fino a quel momento in misura ridotta (DEC-082, DEC-089); da lì il ritorno al menu segue `ui/results-and-leaderboards.md`.
5. **Given** il giocatore ha attraversato l'uscita del Piano 0 verso il piano 1 e le prove sono state presentate (DEC-042), **when** apre `PauseMenu` e seleziona "Prove", **then** vede l'elenco delle prove specifiche della run e il loro stato di completamento.
6. **Given** il giocatore è nel Piano 0, dove l'HUD di combattimento è nascosto (DEC-169), **when** apre il menu di pausa, **then** può consultare salute, risorse e build senza uscire dal Piano 0, e l'HUD torna nascosto quando la pausa si chiude.
7. **Given** il giocatore è nel Piano 0 e apre il menu di pausa con il comando di pausa (default proposto WP15a), **when** sceglie "Riprendi" o preme ESC, **then** torna nel Piano 0 e non in `Gameplay`, con la preparazione della run intatta.
8. **Given** il giocatore è in `Gameplay` in una run vera e apre `PauseMenu`, **when** seleziona "Rigenera la run" e conferma in `ExitConfirm` (WP21, DEC-114), **then** la run corrente si chiude in silenzio (nessun `RunResults`, DEC-089) e riparte subito dal Piano 0 con un seed diverso; **when** invece annulla, **then** torna in `PauseMenu` senza alcun effetto sulla run in corso; il tasto rapido `R` in `Gameplay` resta, in entrambi i casi, il solo reset rapido della stessa run allo stesso seed (DEC-141), mai questa via.
