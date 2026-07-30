---
id: gd-system-floor-zero
title: Floor Zero
domain: design
status: approved
authority: canonical
owner: design
summary: "Piano 0: hub ibrido di rifugio e arene opzionali, dove si sceglie tema (con anteprima visiva generata, DEC-039) e personaggio mentre la run si prepara — entrambi modificabili finché non si attraversa l'uscita verso il piano 1, con il cambio di tema che riavvia la generazione dei piani (DEC-091) — carte tema e schede personaggio sono cliccabili col mouse, perché il Piano 0 conta come menu ai fini di DEC-057 (DEC-075); completare un'arena dà una piccola dote iniziale alla run (DEC-029), disattivata in modalità Classificata. Le arene sono simulazioni a rischio zero che ripristinano esattamente lo stato d'ingresso e non hanno un'economia propria: le uniche ricompense sono la dote e la meta-progressione (DEC-055, DEC-092, DEC-093); basta un solo contenuto \"best-of\" perché un'arena si apra, seminata dal pool curato minimo quando mancano (DEC-094). Il museo permette anche di provare le creazioni esposte, senza alcun limite di tentativi, tempo o usi (DEC-040, DEC-095) ed è curato in modo misto: promozione automatica per metriche più preferiti del giocatore, che hanno la precedenza e non escono mai dal museo (DEC-063); un preferito diventato Reliquia resta esposto ma non più provabile in arena, mentre una promozione solo per metriche esce automaticamente (DEC-085). Le prove specifiche della run vengono presentate al passaggio verso il piano 1 (DEC-042). Il Piano 0 è il crogiolo dei mondi della cornice narrativa (DEC-067). L'abbandono del Piano 0 passa da ESC a `ExitConfirm` (DEC-074). Al primissimo avvio, prima della visita guidata, il gioco propone la scelta binaria completo/solo curato con una schermata dedicata a due carte, senza default silenzioso (DEC-070, DEC-086). La primissima visita al Piano 0 è un tutorial integrato nelle arene opzionali, senza tutorial separato (DEC-047). Il contenuto curato di fallback del Piano 0 è lo stato base del gioco, precaricato e sempre disponibile, senza attesa possibile (DEC-153); l'HUD di combattimento resta nascosto nel Piano 0, consultabile dal menu di pausa, visibile durante le prove (DEC-169)."
last_reviewed: 2026-07-30
last_verified_commit: 63753fc
topics: [Piano 0, hub, tema, arene, museo, DEC-091, onboarding, Reliquie, DEC-153, DEC-169, prove, DEC-042, WP16, WP15a, DEC-004, DEC-047, DEC-092, DEC-093, DEC-094, DEC-095, best-of, simulazione]
related: []
supersedes: []
source_files: [src/render/game_renderer.c, src/render/game_renderer.h, src/core/game_types.h, src/world/floor_zero.c, src/world/floor_zero.h, src/world/floor_zero_arena.c, src/world/floor_zero_arena.h, src/content/run_catalog.c, src/content/run_catalog.h, src/gameplay/combat.c, src/game/trials.c, src/app/app.c, src/tests/floor_zero_arena_tests.c]
---

# Floor Zero

## Intento per il giocatore

Il Piano 0 è un luogo sicuro, sempre disponibile, in cui il giocatore prepara la run
successiva senza fretta e senza rischio di perdita. Deve dare la sensazione di un rifugio
personale che cresce con le run passate, non di una schermata di attesa.

Il Piano 0 è il crogiolo della cornice narrativa minima del gioco (DEC-067): il luogo-fucina
fuori dal tempo dove i mondi generati nascono, si fondono e si sciolgono. Cornice completa in
[Narrative Tone](../content/narrative-tone.md) (rimando, non riformulata qui).

## Condizioni di ingresso

- Si entra nel Piano 0 dallo stato `RunSetup`, come descritto in
  [Game States and Flow](../05-game-states-and-flow.md).
- Si rientra nel Piano 0 anche al termine di una run (vittoria o sconfitta, DEC-006/DEC-031),
  prima di tornare al menu principale o avviarne un'altra.
- Il Piano 0 non richiede che alcun contenuto della run futura sia già pronto: è sempre
  giocabile, per costruzione (vedi "Regole per contenuti generati").

## Abbandono del Piano 0 (DEC-074)

L'abbandono del Piano 0 passa da `ExitConfirm`, come dagli altri stati di gioco: premere ESC
(o l'azione equivalente su controller) apre la conferma, che protegge la preparazione già
fatta (tema e personaggio scelti, generazione dei piani successivi in corso). Confermando,
la preparazione si interrompe e il giocatore torna al menu principale; annullando, resta nel
Piano 0 senza alcuna conseguenza. L'arco `FloorZero → ExitConfirm` è canonico nella mappa
degli stati: dettaglio e diagramma in
[Game States and Flow](../05-game-states-and-flow.md) e
[Navigation Map](../ui/navigation-map.md) (rimando, non riformulato qui).

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Carte tema (2-3 proposte) | Sempre, all'ingresso nel Piano 0 | Sempre, anche dopo aver già scelto un tema: resta modificabile finché non si attraversa l'uscita (DEC-091) | Selezionare (o cambiare) una carta tema, con tastiera/pad o mouse (DEC-075) | Il tema guida la generazione dei piani 1-5 e la sua evoluzione/degenerazione fino al boss del piano 5; cambiare tema dopo una scelta precedente riavvia la generazione dei piani (DEC-091) | Ogni carta mostra nome, breve descrizione e, quando pronta, un'anteprima visiva già generata (es. uno sprite campione di un nemico del tema, DEC-039); se l'anteprima non è ancora pronta, la carta mostra comunque nome e descrizione (fallback); cambiando tema, l'uscita torna chiusa finché il piano 1 del nuovo tema non è pronto (DEC-091) |
| Selettore personaggio | Sempre | Sempre, anche dopo una scelta precedente: resta modificabile finché non si attraversa l'uscita (DEC-091) | Scegliere (o cambiare) un personaggio della rosa base o l'alternativa generata per la run, oppure rifiutare l'alternativa — il rifiuto resta ripensabile fino all'uscita, fonte unica in [Characters](characters.md) (DEC-097) — con tastiera/pad o mouse (DEC-075) | Il personaggio scelto definisce statistiche e trait per l'intera run (vedi [Characters](characters.md)) | Scheda personaggio con statistiche e trait in evidenza |
| Ingresso arena di sfida | Quando esiste almeno un contenuto "best-of" valido per un'arena — basta uno solo (DEC-094); al primissimo avvio, o ogni volta che i best-of mancano, l'arena è seminata dal pool curato minimo (DEC-087, DEC-094) | Quando il giocatore non è già impegnato in un'altra attività del Piano 0 | Entrare nell'arena opzionale | Sfida autonoma locale al Piano 0, simulazione pura senza economia propria (DEC-093): completarla dà una piccola dote iniziale (risorse o un oggetto comune) per la run che sta per cominciare (DEC-029), salvo modalità Classificata dove la dote è disattivata; vittoria o sconfitta, il giocatore esce con esattamente la salute e lo stato con cui è entrato (DEC-092), perché l'arena è una simulazione a rischio zero (DEC-055) | Segnale d'ingresso dedicato, esito mostrato a fine sfida; segnale distinto quando la dote viene assegnata alla run in preparazione; all'uscita, vittoria o sconfitta, il giocatore ha lo stesso stato con cui è entrato |
| Museo delle creazioni | Sempre | Sempre | Sfogliare la galleria delle migliori creazioni (oggetti, nemici, boss, fusioni, personaggi), ciascuna con nome e storia; selezionare una creazione per provarla (DEC-040) | Consultazione libera senza effetto meccanico; provare una creazione apre una saletta dedicata (per un oggetto) o un'arena di sfida (per riaffrontare un boss, collegata alle arene DEC-004/DEC-029); non disponibile se la creazione è una Reliquia (DEC-069/DEC-085); nessun limite di tentativi, tempo o usi per le prove (DEC-095) | Galleria consultabile; feedback dedicato quando si avvia una prova |
| Indicatore di generazione | Sempre, da quando inizia la preparazione dei piani successivi | Sola lettura, non interagibile | Nessuna (informativo) | Comunica lo stato di preparazione dei piani | Messaggio descrittivo stabile, vedi [Generation Status](../ui/generation-status.md) |
| Uscita verso il piano 1 | Sempre visibile nel Piano 0 | Quando il piano 1 è pronto (validato o in fallback) | Attraversare l'uscita | Avvio della run: il piano 1 viene caricato e le prove specifiche della run vengono presentate al giocatore (DEC-042) | L'uscita si apre visibilmente solo quando diventa abilitata; l'attraversamento mostra la presentazione delle prove prima o durante il caricamento del piano 1 |
| Abbandono (ESC) | Sempre nel Piano 0 | Sempre | Premere ESC (o equivalente su controller) | Apre `ExitConfirm`; confermando si interrompe la preparazione e si torna al menu principale (DEC-074) | Vedi [Game States and Flow](../05-game-states-and-flow.md) per la mappa completa degli stati |

## Risultato

Al termine della preparazione nel Piano 0, il giocatore entra nella run con: un tema
scelto tra quelli proposti, un personaggio scelto, e un piano 1 pronto. Nessuno di questi
tre elementi può restare indefinito quando si attraversa l'uscita. Fino a quel momento, sia
il tema sia il personaggio restano liberamente modificabili (DEC-091): cambiare tema
riavvia la generazione dei piani e richiude l'uscita finché il piano 1 del nuovo tema non
è pronto.

## Feedback

- Il tema scelto e il personaggio scelto restano visibili in un riepilogo mentre si
  esplora il resto del Piano 0, così il giocatore non perde la propria decisione.
- L'apertura dell'uscita verso il piano 1 è un evento visibile e distinto (non un
  semplice cambio di stato silenzioso), perché segna la fine dell'attesa.
- L'indicatore di generazione non usa percentuali finte né promesse di tempo, in linea con
  [Generation Status](../ui/generation-status.md).
- Ogni carta tema comunica anteprima visiva, nome e descrizione insieme, non in sequenza:
  il giocatore deve poter confrontare le proposte a colpo d'occhio (DEC-039).
- Cambiare il tema dopo averlo già scelto mostra chiaramente che la generazione dei piani
  riparte e che l'uscita torna chiusa, così il giocatore capisce il costo della scelta
  (DEC-091).

## Tema e personaggio modificabili fino all'uscita (DEC-091)

Il Piano 0 è il luogo della scelta, non il luogo del vincolo: sia il tema sia il
personaggio restano modificabili in qualunque momento, finché il giocatore non attraversa
l'uscita verso il piano 1 — coerente con la preselezione modificabile del codice di
condivisione (DEC-077). Non esiste una "conferma" che blocchi la scelta prima di quel
momento.

Cambiare il tema dopo averne già scelto uno ha un costo esplicito: **riavvia la
generazione dei piani** 1-5 per il nuovo tema. L'uscita verso il piano 1, se nel frattempo
si era aperta, **torna chiusa** finché il piano 1 del nuovo tema non ha di nuovo superato
la validazione. Cambiare personaggio non ha invece alcun costo di rigenerazione: il
personaggio non guida la generazione dei piani.

> **Nota di implementazione (M5, gap esplicito stile DEC-052-pre-M3):** la milestone M5
> tratta la conferma del tema come definitiva, senza permettere di tornare indietro dopo la
> scelta. DEC-091 stabilisce che il tema (e il personaggio) restano modificabili fino
> all'uscita: colmare questo gap — incluso il riavvio della generazione al cambio tema — è
> lavoro di implementazione ancora da fare, non una nuova decisione di design.

## Anteprime visive dei temi (DEC-039)

Ognuna delle 2-3 proposte di tema mostra nome, breve descrizione **e** un'anteprima visiva
già generata (es. uno sprite campione di un nemico del tema). La generazione delle anteprime
ha **priorità altissima** a inizio run, prima di altre generazioni in corso, perché la
scelta del tema dipende da queste anteprime.

Se un'anteprima non è ancora pronta quando il giocatore vede le carte tema, la carta
corrispondente mostra comunque nome e descrizione, senza bloccare la scelta: questo è un
fallback specifico, riconducibile alla regola generale di
[Generated Content Validation](generated-content-validation.md).

> **Nota di implementazione (M5, 18/07/2026, gap esplicito stile DEC-052-pre-M3):** nella
> milestone che introduce la scelta del tema nel Piano 0, il modello immagini è ancora
> provvisorio — le anteprime visive NON vengono generate. Le carte usano **sempre** il
> percorso fallback nome+descrizione descritto sopra, non solo quando l'anteprima non fa in
> tempo: è un gap di implementazione temporaneo, non una nuova decisione di design. DEC-039
> resta il comportamento di riferimento; questa nota si aggiorna/si rimuove quando il modello
> immagini smette di essere provvisorio.

## Il Piano 0 conta come menu per il mouse (DEC-075)

Ai fini della parità rigorosa di input, il mouse è ammesso solo nei menu (DEC-057, fonte
unica in [Options and Accessibility](../ui/options-and-accessibility.md), rimando, non
riformulato qui). Il Piano 0 conta come **menu** in questo senso: le carte tema, le schede
personaggio e i pannelli selezionabili del Piano 0 sono cliccabili col mouse, esattamente
come le voci di menu degli altri stati. Il movimento del personaggio nel Piano 0 e ogni
meccanica giocata (arene comprese) restano su tastiera/controller con parità rigorosa: il
mouse non muove il personaggio e non è mai richiesto — il nucleo di DEC-057 resta intatto.

> **Nota di implementazione (gap esplicito stile DEC-009/DEC-052):** le carte tema e le
> schede personaggio realizzate in M5/M6a rispondono oggi solo a tastiera/pad; vanno rese
> cliccabili col mouse per essere conformi a DEC-075. Non è una nuova decisione di design,
> è un gap da colmare nell'implementazione esistente.

## Museo con interazione (DEC-040)

Il museo del Piano 0 non è solo consultazione passiva. È una galleria delle creazioni
migliori — oggetti, nemici, boss, fusioni, personaggi — ciascuna presentata con **nome e
storia**. Il giocatore può inoltre **provare** una creazione esposta:

- un **oggetto** si prova in una saletta dedicata, isolata dal resto della run in
  preparazione;
- un **boss** si riaffronta in un'**arena di sfida**, riusando l'infrastruttura già
  descritta per le arene opzionali del Piano 0 (DEC-004) e la dote iniziale (DEC-029) se
  applicabile.

Provare una creazione dal museo non altera la run in preparazione, salvo l'eventuale dote di
un'arena completata (DEC-029).

Le prove dal museo sono **illimitate** (DEC-095): nessun tetto di tentativi, di tempo o di
usi, né per la saletta oggetto né per l'arena boss. Il museo è un parco giochi della
memoria, non una risorsa da dosare — coerente con il rischio zero delle arene (DEC-055,
DEC-092).

## Criteri di ingresso nel museo (DEC-063)

Il museo del Piano 0 (vedi sopra, DEC-040) è curato in modo **misto**: metà specchio delle
metriche di gioco, metà curatela diretta del giocatore.

- **Promozione automatica per metriche**: un contenuto del catalogo (vedi
  [Save and Meta Progression](save-and-meta-progression.md)) entra nel museo quando le sue
  metriche di uso, di sopravvivenza col giocatore e di contributo alle vittorie superano una
  soglia (valore esatto non fissato qui, vedi Domande aperte residue).
- **Curatela del giocatore**: i contenuti che il giocatore segna come preferiti nel Catalogo
  (DEC-045, vedi [Save and Meta Progression](save-and-meta-progression.md)) hanno la
  **precedenza** sulla promozione automatica: entrano nel museo e non ne escono mai finché
  restano segnati come preferiti, indipendentemente dalle loro metriche.

Un contenuto promosso solo per metriche può invece essere sostituito da un altro contenuto
se le metriche di quest'ultimo lo superano.

## Reliquie nel museo (DEC-085)

Un contenuto esposto nel museo può diventare una **Reliquia** dopo un aggiornamento del
gioco: fonte unica delle Reliquie in
[Save and Meta Progression](save-and-meta-progression.md) (DEC-069, rimando, non riformulato
qui). Cosa succede al museo dipende da come il contenuto vi era entrato:

- se era esposto **come preferito del giocatore**, resta esposto: la curatela non esce mai
  finché marcata (DEC-063, sopra). La scheda museale segnala che si tratta di una Reliquia, e
  la prova in arena (DEC-040) non è più disponibile, perché le Reliquie non sono giocabili
  (DEC-069);
- se era esposto **solo per promozione automatica per metriche**, esce automaticamente dal
  museo: la promozione automatica riguarda solo contenuti ancora in circolazione, non le
  Reliquie.

## Scelta al primo avvio: completo o solo curato (DEC-070)

Al primissimo avvio del gioco — prima ancora della primissima visita guidata al Piano 0
descritta sotto (DEC-047) — il gioco misura l'hardware con il benchmark già presente nel
progetto e propone al giocatore una scelta **binaria**, senza livelli intermedi di download:

- **esperienza completa**: scarica e attiva i modelli IA di generazione (un solo set di
  modelli; non esistono alternative o livelli di qualità tra cui scegliere);
- **solo curato**: nessun modello IA attivo; si gioca con i contenuti curati e il fallback
  procedurale sempre disponibile (vedi
  [Generated Content Validation](generated-content-validation.md)), con la possibilità di
  attivare la generazione in un secondo momento.

Se il benchmark rileva che l'hardware non regge la generazione, il gioco lo dichiara
chiaramente e consiglia "solo curato", ma non impedisce al giocatore di scegliere comunque
"completo". Questa scelta è coerente con la garanzia che il gioco è sempre giocabile
(DEC-002, DEC-020): "solo curato" non è un fallback d'emergenza temporaneo, è una modalità di
gioco legittima e permanente (dettaglio in
[AI Content Generation Model](../06-ai-content-generation-model.md), rimando, non
riformulato qui).

**Interfaccia della scelta (DEC-086):** la scelta è una **schermata dedicata a due carte**
(esperienza completa / solo curato), mostrata subito dopo il benchmark. Non esiste un
default silenzioso: se il giocatore annulla la scelta o chiude il gioco senza sceglierne
una, la schermata si ripresenta al rientro, finché una scelta non viene fatta — il gioco non
procede alla primissima visita guidata del Piano 0 (DEC-047) senza una scelta esplicita. Chi
ha scelto "solo curato" può **riattivare** la generazione IA in un secondo momento dalle
**Impostazioni**, accompagnata dalla stessa informazione del benchmark sull'hardware:
questo documento è la fonte unica della regola (DEC-086); la voce corrispondente è
registrata in [Options and Accessibility](../ui/options-and-accessibility.md).

Dopo questa scelta iniziale, il gioco procede alla primissima visita guidata al Piano 0
(DEC-047, sotto), in entrambe le modalità.

## Primissima visita: tutorial integrato (DEC-047)

La primissima volta che il giocatore entra nel Piano 0, la visita è **guidata**: le arene di
sfida opzionali (vedi "Dote iniziale dall'arena di sfida (DEC-029)" sopra e DEC-004)
insegnano movimento, sparo, risorse e fusione tramite cartelli e prove pratiche, direttamente
dentro l'arena. Non esiste un tutorial separato dal resto del gioco: l'insegnamento avviene
attraverso le stesse arene opzionali che il giocatore userà anche più avanti per allenarsi o
guadagnare la dote iniziale (DEC-029).

Le visite successive al Piano 0, nella stessa run o in run future, non ripropongono la guida:
le arene restano visitabili e riutilizzabili per allenarsi, ma senza cartelli o prove
pratiche di introduzione.

## HUD nascosto nel Piano 0 (DEC-169)

Nel Piano 0 l'HUD di combattimento (vite, munizioni/cariche, risorse, timer di run, ecc. —
vedi [HUD](../ui/hud.md)) resta **nascosto** di norma: il Piano 0 è un rifugio, non
un'arena permanente, e non ha bisogno di mostrare informazioni di combattimento quando il
giocatore non sta combattendo.

- L'HUD resta comunque **consultabile dal menu di pausa** in qualunque momento, come ogni
  altra informazione di stato della run in preparazione (vedi
  [Pause Menu](../ui/pause-menu.md)). Con quale comando il menu di pausa si apra dal Piano 0
  — dove ESC è già assegnato a `ExitConfirm` (DEC-074) — DEC-169 non lo fissa: è la domanda
  aperta 22 in `../governance/open-questions.md`.
- Durante le **arene di sfida** (opzionali o del museo) — incluse quelle della primissima
  visita guidata (DEC-047) — l'HUD torna **visibile**: lì il giocatore combatte davvero e
  ha bisogno delle stesse informazioni che userebbe nei piani 1-5.

Fuori dalle arene, nel resto del Piano 0 (scelta tema/personaggio, museo, riepilogo),
l'HUD di combattimento resta nascosto.

> **Nota di implementazione (WP15a, 2026-07-30, supera la nota W3 del 2026-07-28):** la
> regola di visibilità dell'HUD è una funzione pura, `HudCombatShouldDraw(mode,
> floorZeroTrialActive)` (`src/render/game_renderer.h`/`.c`, dettaglio in `ui/hud.md`), e
> l'hook **è ora collegato**: `FloorZeroArenaEnter`/`FloorZeroArenaExit`
> (`src/world/floor_zero_arena.c`) sono i due soli punti che scrivono
> `Game.floorZeroTrialActive`, quindi l'HUD di combattimento ricompare durante una
> simulazione e torna nascosto all'uscita, senza alcun lavoro aggiuntivo sul renderer —
> esattamente come la nota precedente prevedeva. **Limite dichiarato:** dentro la
> simulazione il timer di run **si disegna ma resta fermo a `0:00`**, perché il Piano 0 non
> è una run cronometrata (`FloorZeroEnter` spegne `inRealRun` e azzera
> `runElapsedSeconds`); non è un difetto di questo lavoro, è la conseguenza corretta di
> DEC-051 applicata a un luogo dove il tempo della run non scorre. La consultazione dal menu
> di pausa (sotto) era già disegnata (riquadro dedicato in `DrawPauseMenuOverlay`,
> condizionato solo a `game->floor == 0`) e ha ora anche il comando che la apre: un
> **default proposto** per la domanda aperta 22, vedi "Stato di implementazione" sotto.

## Interazioni

- [Characters](characters.md): la scelta del personaggio avviene qui, nel Piano 0; il
  rifiuto dell'alternativa generata resta ripensabile fino all'uscita, fonte unica lì
  (DEC-097).
- [Rooms and Floor Generation](rooms-and-floor-generation.md): il piano 1 che si apre
  dall'uscita segue le regole di struttura dei piani lì definite.
- [Run Manifest and Reproducibility](run-manifest-and-reproducibility.md): il tema e il
  personaggio scelti nel Piano 0 entrano nel manifest della run.
- [Save and Meta Progression](save-and-meta-progression.md): il museo delle creazioni
  migliori e il catalogo dei contenuti generati sono meta-progressione persistente; i criteri
  di ingresso nel museo — metriche più preferiti del Catalogo (DEC-045) — sono descritti qui
  (DEC-063).
- [Special Rooms](special-rooms.md): le arene di sfida del Piano 0, la saletta di prova per
  un oggetto del museo e l'arena per riaffrontare un boss condividono l'infrastruttura delle
  stanze speciali.
- [Multiplayer and Competition](../08-multiplayer-and-competition.md): la dote iniziale
  dell'arena di sfida (DEC-029) è disattivata nelle run in modalità Classificata, per
  coerenza con DEC-016/DEC-021.
- [Rewards and Economy](rewards-and-economy.md): le prove specifiche presentate qui
  all'ingresso nel piano 1 sono il canale bonus dei punti sblocco (DEC-027, DEC-042).
- [Pause Menu](../ui/pause-menu.md) e
  [Inventory and Synergy Screen](../ui/inventory-and-synergy-screen.md): le prove restano
  consultabili da lì per tutta la run (DEC-042).
- [Options and Accessibility](../ui/options-and-accessibility.md): fonte unica della parità
  rigorosa di input e del perimetro dell'ammissione del mouse, di cui il Piano 0 fa parte
  come menu (DEC-057, DEC-075); ospita anche la voce di riattivazione della generazione IA
  per chi ha scelto solo curato al primo avvio (DEC-086).
- [Game States and Flow](../05-game-states-and-flow.md) e
  [Navigation Map](../ui/navigation-map.md): l'abbandono del Piano 0 passa da `ExitConfirm`
  (DEC-074).
- [HUD](../ui/hud.md): fonte unica del contenuto e della presentazione dell'HUD di
  combattimento; questo documento registra solo che nel Piano 0 resta nascosto fuori dalle
  arene, consultabile dal menu di pausa e visibile durante le prove del Piano 0 — arene di
  sfida e tutorial integrato (DEC-047), non le prove della run di DEC-042 (DEC-169).

## Dote iniziale dall'arena di sfida (DEC-029)

Completare un'arena di sfida del Piano 0 dà una **piccola dote** (risorse o un oggetto
comune) per la run che sta per iniziare: un piccolo vantaggio d'apertura, non un
potenziamento permanente del personaggio (coerente con
[Save and Meta Progression](save-and-meta-progression.md), che esclude potenziamenti
permanenti).

Questa dote è **disattivata nelle run in modalità Classificata**, per coerenza con la
parità richiesta dalle gare competitive (vedi DEC-016 e DEC-021 in
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md)): una run
Classificata non deve poter partire avvantaggiata da attività extra del Piano 0.

Oltre a questa dote, le arene non offrono altre ricompense (DEC-093, dettaglio sotto).

## Le arene sono a rischio zero e ripristinano lo stato d'ingresso (DEC-055, DEC-092)

Un'arena di sfida del Piano 0 è una **simulazione pura**: uscendone, che sia per vittoria o
per sconfitta, il giocatore ha **esattamente** la salute e lo stato con cui era entrato
(DEC-092). Non ci sono perdite da recuperare né benefici accumulati durante l'arena da
portare fuori: l'unico effetto che sopravvive all'uscita dall'arena è la dote iniziale
(DEC-029), e solo in caso di vittoria. La sconfitta in particolare non ha **alcun costo**
oltre la dote mancata — nessuna perdita di salute, risorse o oggetti della run in
preparazione. Questo vale sia per le arene opzionali standard sia per quelle riusate dal
museo per riaffrontare un boss (DEC-040): il Piano 0 nel suo complesso resta a rischio zero,
coerente con il suo essere un rifugio sicuro (vedi "Intento per il giocatore" sopra).

Le arene non hanno inoltre un'**economia propria** (DEC-093): oltre alla dote iniziale
(DEC-029) e all'eventuale meta-progressione (punti, sblocchi), non esistono altre
ricompense per l'attività nel Piano 0. Il Piano 0 non è un posto dove si "farma".

## Presentazione delle prove all'ingresso nel piano 1 (DEC-042)

Le prove specifiche della run — quelle che danno i punti bonus di DEC-027 — vengono
presentate al giocatore al passaggio dal Piano 0 al piano 1: il momento di ingresso nella
run vera, quando il giocatore attraversa l'uscita. Da quel momento in poi, l'elenco delle
prove resta sempre consultabile dal menu di pausa (`ui/pause-menu.md`) e dalla schermata
build (`ui/inventory-and-synergy-screen.md`); questo documento non ripete il dettaglio della
loro generazione o del loro punteggio, definito in
[Rewards and Economy](rewards-and-economy.md).

> **Nota di implementazione (WP16, 2026-07-30):** l'assegnazione (`TrialsAssignForRun`,
> `src/game/trials.c`) vive dentro `GameResetRunWithSeed` — l'unica funzione che fa partire
> una run vera — chiamata esattamente al vero attraversamento del varco (`floorZeroExitCrossed`,
> `src/app/app.c`). "Presentare" oggi riusa il componente di sistema già esistente per le
> scoperte (DEC-065/131/152): una card di scoperta ("Prova") per ogni prova assegnata, invece
> di un overlay dedicato nuovo — nessun contenuto nuovo, solo un titolo diverso sullo stesso
> componente. Le prove restano consultabili per l'intera run indipendentemente da quanto la
> card resta a schermo o viene scartata (scartare la coda di notifica non tocca mai lo STATO
> della prova, `Game.trials[]`) — nessun campo separato traccia "presentate" (aggiornamento
> 30/07, seconda tornata: `Game.trialsPresented` non era mai letto da nulla, rimosso insieme
> al commento che ne descriveva un ruolo inesistente; la consultabilità dipende solo da
> `game->trialCount > 0`). Verificato da `--trials-test`, vedi
> [Rewards and Economy](rewards-and-economy.md), "Stato di implementazione: le prove
> specifiche".

## Stato di implementazione: le arene di sfida del Piano 0 (WP15a, 2026-07-30)

Le arene di sfida opzionali di DEC-004 **esistono ora nel motore**: chiude il gap dichiarato
dalla nota M5 di questo documento e sblocca il tutorial integrato di DEC-047 e la metà
"HUD visibile in arena" di DEC-169. Modulo dedicato: `src/world/floor_zero_arena.{h,c}`.

- **Struttura**: tre **piazzole** segnalate nel crogiolo (`PICKUP_TRIAL_GATE`,
  `FloorZeroArenaPlaceGates`, chiamata da `FloorZeroEnter`), una per tema di pratica —
  *movimento e tiro*, *risorse e bombe*, *fusione*. Stanno sulla **croce centrale** della
  stanza, l'unica zona che `RoomLayoutBuild` garantisce sempre libera, mai sopra il punto in
  cui nasce il giocatore. Nessun asset nuovo: riusano `assets/art/props/piedistallo` e, se
  manca, una forma geometrica dedicata (un arco su basamento). L'etichetta del tema è
  **sempre scritta**, anche senza sprite (DEC-058).
- **Ingresso a conferma esplicita**: il tasto di interazione `X` **a contatto** con la
  piazzola, lo stesso gesto dell'arena incontrata nel piano e della Pourhouse
  (`Game.interactQueued` → `FloorZeroArenaQueueEntry`). Il solo **tocco non fa partire
  nulla**: entrare non è irreversibile, ma girando per l'hub non si deve finire dentro una
  simulazione per averci camminato sopra.
- **Simulazione senza economia (DEC-092/093), il cuore del design.** All'ingresso si cattura
  una `FloorZeroTrialSnapshot`: il `Player` copiato **per intero** (mai campo per campo — è
  l'unico modo per cui una statistica aggiunta domani non possa restare fuori dal
  ripristino), più punteggio, stream RNG e messaggio a schermo. All'uscita — vittoria,
  sconfitta o abbandono — si riapplica tutto. Conseguenze verificate dal test: niente danno
  permanente, nessuna valuta o oggetto raccolto lì dentro sopravvive, nessun avanzamento
  delle **prove della run** di DEC-042 (guardia esplicita in `src/game/trials.c`, non un
  effetto collaterale del fatto che nel Piano 0 il conteggio sia di solito zero), timer di
  run fermo (`inRealRun` resta falso: verificato).
- **Vinta ma non finita.** Abbattere tutti i nemici **annuncia** la vittoria e nient'altro:
  la simulazione resta aperta e la si lascia quando si vuole. È la lettura letterale di
  DEC-095 (prove illimitate, nessun tetto di tempo) e l'unica compatibile con la piazzola
  della fusione, dove il combattimento è il contorno.
- **Uscita sempre disponibile, morte mai un game over (DEC-055).** ESC chiude la simulazione
  e riporta nell'hub (fuori da una prova ESC resta `ExitConfirm`, DEC-074, invariato). La
  salute a zero **dentro** una simulazione scrive `Game.floorZeroTrialDefeated` invece di
  `PHASE_GAME_OVER` (`CombatDamagePlayer`): si esce con un messaggio ironico-leggero
  (DEC-105) e lo stato d'ingresso intatto. Il varco verso il piano 1 **non si attraversa**
  durante una simulazione: l'attraversamento azzera l'intero `Game`, snapshot compreso.
- **Contenuti best-of (DEC-004/094)**: `RunCatalogBestOfEnemies`
  (`src/content/run_catalog.c`) rilegge i tipi di nemico e di boss registrati nel catalogo
  delle run passate — definizioni **già validate e già viste in una run vera**, mai
  generate sul momento. Un boss del catalogo entra come nemico normale della simulazione:
  riaffrontare un boss è la prova dal **museo** (DEC-040), che non esiste ancora nel motore.
- **Fallback obbligatorio, l'arena funziona SEMPRE**: nessuna run passata (primissimo avvio,
  catalogo cancellato) → i nemici del contenuto curato già caricato e, se mancassero anche
  quelli, i tipi d'esempio del motore. È il caso limite dichiarato qui sopra
  (DEC-087/094/153), verificato dal test con un catalogo vuoto.
- **Prove illimitate (DEC-095)**: nessun tetto di tentativi, di tempo o di usi. Si rientra
  quante volte si vuole e la composizione è **deterministica** (stream locale derivato da un
  seme fisso per tema, mai `game->rng`, che viene salvato e rimesso: una simulazione non
  sposta gli stream della run in preparazione).
- **Tutorial integrato (DEC-047)**: alla **primissima visita di ciascuna piazzola** compare
  un **cartello** — una riga breve che spiega i comandi di quel tema (WASD e frecce/mouse;
  SPAZIO per la carica; TAB per la fucina), tono ironico-leggero, senza accentate. Resta
  visibile per tutta la simulazione, non scade come un messaggio. Le visite successive
  restano mute, come il documento chiede. Nella prova FUSIONE la lezione è **praticabile**:
  la simulazione mette a terra due oggetti e un catalizzatore, il minimo esatto per fondere
  davvero, e TAB apre `BuildScreen` come in `Gameplay` — tutto restituito all'uscita.
- **LIMITE DICHIARATO — il "già visto" non è persistito su disco.** Vive su
  `AppUi.floorZeroTrialTutorialSeen[]`, cioè in memoria di **processo**: sopravvive a run
  successive nella stessa sessione, non a un riavvio del gioco. Nessun profilo persistente
  esiste ancora nel motore (vedi `save-and-meta-progression.md` e
  `docs/engineering/known-issues.md`): quando arriverà, questo flag è il primo candidato a
  traslocarci.
- **LIMITE DICHIARATO — la dote iniziale di DEC-029 non è implementata.** Completare una
  simulazione non dà oggi alcuna dote alla run in preparazione: questo lavoro applica alla
  lettera "nulla di ciò che accade lì dentro tocca la run" (DEC-092/093) e lascia la sola
  eccezione prevista — la dote — a un lavoro successivo, insieme alla sua disattivazione in
  modalità Classificata. Registrato in `docs/engineering/known-issues.md`.
- **Test**: `GameArenaHubTest` (`src/tests/floor_zero_arena_tests.c`, `--arena-hub-test` in
  `make test`) — dodici blocchi: piazzole e conferma esplicita, **ripristino integrale**
  confrontato con `memcmp` sull'intero `Player` dopo aver subito danno e raccolto risorse
  dentro la simulazione, morte che non è mai `PHASE_GAME_OVER`, prove della run ferme
  dentro, best-of da un catalogo sintetico costruito perché **solo l'esito** possa
  decidere quale run vince, fallback a catalogo vuoto, determinismo e stream RNG mai
  spostato, varco non attraversabile, tutorial alla prima visita e mai più, consultazione
  dal Piano 0 col comando di pausa.

### Default proposti dall'implementazione (stile DEC-019)

| Cosa | Default proposto | Dove |
|---|---|---|
| **Quante arene e quali temi** | Tre piazzole: movimento e tiro, risorse e bombe, fusione. DEC-047 elenca "movimento, sparo, risorse e fusione" senza dire quante arene siano: movimento e sparo stanno insieme perché sono lo stesso gesto continuo, la bomba sta con le risorse perché *è* una risorsa spendibile. | `FloorZeroTrialTheme`, `src/core/game_types.h` |
| **Criterio "best-of"** | `10000 × (esito vittoria) + 100 × piano raggiunto + 50 × boss sconfitti`, e si pesca da **una sola** run, la migliore — non da un miscuglio: un'arena best-of deve sapere di qualcosa. Spareggio sul nome del file, mai sull'ordine di enumerazione della cartella. | `RunCatalogBestOfEnemiesFromPath`, `src/content/run_catalog.c` |
| **Taglia dell'ondata** | 3 nemici (movimento), 2 (risorse), 1 (fusione). Poche di proposito: l'arena del Piano 0 **insegna**, non mette alla prova come quella incontrata nel piano (che ha budget ×1.5 e nemici in fascia alta, WP6). | `FLOOR_ZERO_ARENA_ENEMIES_*`, `src/world/floor_zero_arena.c` |
| **Grado dei nemici** | Quello registrato nel catalogo, senza maggiorazioni: un contenuto best-of si riaffronta com'era, non più cattivo. Un boss entra come nemico normale. | `FloorZeroArenaEnter` |
| **Arredo della simulazione** | Nessun ostacolo: un'ondata va schivata, stesso ragionamento già fatto per `ROOM_ARENA` (WP6). L'arredo curato del crogiolo torna intatto all'uscita, ricostruito dal suo seme fisso. | `FloorZeroArenaEnter`/`FloorZeroArenaExit` |
| **Tasto di ingresso** | `X` a contatto con la piazzola, lo stesso dell'arena del piano e della Pourhouse: per il giocatore è un solo gesto, "accetto ciò che questo posto propone". | `Game.interactQueued`, `FloorZeroArenaQueueEntry` |
| **Uscita dalla simulazione** | ESC. Dentro una prova "indietro" significa "torna nell'hub"; fuori resta `ExitConfirm` (DEC-074). | `src/app/app.c`, case `APP_FLOOR_ZERO` |
| **Vittoria** | Tutti i nemici abbattuti. Si **annuncia** e basta: la prova NON si chiude da sola (DEC-095, le prove sono illimitate — chiuderla d'ufficio taglierebbe corta la lezione della piazzola FUSIONE, dove i nemici sono il contorno e la fucina è il punto). Nessuna ricompensa: la dote di DEC-029 non è implementata (limite dichiarato sopra). | `FloorZeroArenaCleared`, `FloorZeroArenaNoteVictory` |
| **Comando di pausa dal Piano 0** (domanda aperta 22) | Il **tasto di pausa** apre `PauseMenu` in consultazione; ESC resta `ExitConfirm` (DEC-074) e TAB resta il pannello mondi/personaggi (M5). Vedi `ui/pause-menu.md`. **Default proposto, la domanda resta aperta.** | `AppUi.pauseFromFloorZero`, `src/app/app.c` |

## Regole per contenuti generati

- Le arene di sfida usano solo contenuti "best-of" già validati nelle run passate: non
  generano nulla di nuovo sul momento. Basta **un solo** contenuto valido perché un'arena si
  apra (DEC-094); al primissimo avvio, e ogni volta che i best-of mancano, l'arena è
  seminata dal pool curato minimo (DEC-087), così il tutorial integrato (DEC-047) funziona
  sempre.
- I 2-3 temi proposti nella scelta del tema sono generati dall'IA per quella sessione nel
  Piano 0 (vedi [Characters](characters.md) per il meccanismo analogo applicato al
  personaggio alternativo).
- L'anteprima visiva di ciascun tema proposto è generata con priorità altissima rispetto
  alle altre generazioni in corso a inizio run (DEC-039); se non è pronta in tempo, la carta
  tema resta comunque valida con solo nome e descrizione.
- Il gioco è sempre avviabile: il contenuto curato del Piano 0 non è un ripiego
  temporaneo in cerca di sostituzione, è **lo stato base del gioco** — precaricato e
  sempre disponibile fin dal primo avvio, senza alcuna generazione necessaria e senza
  alcuna attesa possibile per il giocatore (DEC-153). Gli eventuali asset dedicati
  generati (anteprime dei temi, contenuti del museo, ecc.) si aggiungono sopra questo
  stato base quando pronti, senza mai bloccarlo né richiedere che il giocatore attenda:
  vedi [Generated Content Validation](generated-content-validation.md) per la regola
  generale di fallback.

## Casi limite

- Il giocatore raggiunge l'uscita prima che il piano 1 sia pronto: l'uscita resta chiusa e
  il giocatore può continuare a usare il resto del Piano 0 (museo, arene) senza essere
  bloccato in un'attesa passiva.
- Nessuna delle 2-3 proposte di tema generate supera la validazione: il giocatore vede
  sempre le 3 carte tema curate di fallback, il numero canonico (DEC-076), della stessa
  forma delle proposte ordinarie — mai meno di un'opzione di tema selezionabile.
- Il giocatore preme ESC (o l'azione equivalente) mentre la preparazione della run è in
  corso (tema o personaggio già scelti, generazione dei piani successivi in corso):
  `ExitConfirm` si apre prima di interrompere qualunque cosa; annullando, la preparazione
  prosegue esattamente com'era (DEC-074).
- Il giocatore cambia tema dopo averne già scelto uno, con l'uscita eventualmente già
  aperta: il nuovo tema riavvia la generazione dei piani e l'uscita torna chiusa finché il
  piano 1 del nuovo tema non supera la validazione (DEC-091).
- Non esiste ancora alcun contenuto "best-of" per nessuna arena (tipicamente al
  primissimo avvio): le arene restano comunque disponibili, seminate dal pool curato minimo
  (DEC-087, DEC-094), così il tutorial integrato (DEC-047) funziona normalmente.
- Il giocatore entra in un'arena di sfida e la abbandona a metà: il ritorno al resto del
  Piano 0 deve restare disponibile senza penalità sulla run in preparazione.
- Il giocatore viene sconfitto dentro un'arena di sfida: esce dall'arena con esattamente
  la salute e lo stato con cui era entrato (DEC-092), senza alcuna perdita sulla run in
  preparazione, e semplicemente non ottiene la dote di quell'arena (DEC-055); può rientrare
  subito nell'arena o proseguire nel resto del Piano 0.
- Il giocatore completa un'arena di sfida mentre sta per avviare una run in modalità
  Classificata: la dote iniziale non viene assegnata, e l'interfaccia lo segnala prima
  dell'ingresso in arena, non solo dopo.
- L'anteprima visiva di uno o più temi proposti non è pronta quando le carte tema vengono
  mostrate: la carta corrispondente mostra solo nome e descrizione, senza bloccare la scelta
  (DEC-039).
- Il giocatore prova un oggetto del museo in saletta, ma quell'oggetto non è ancora
  sbloccato per l'uso nella run in preparazione: la prova resta locale e non aggiunge
  l'oggetto alla build della run che sta per iniziare (salvo l'eventuale dote di un'arena
  completata, DEC-029).
- Il giocatore entra nel Piano 0 per la seconda volta (nella stessa run o in una
  successiva): la guida della primissima visita (DEC-047) non viene riproposta, ma le
  arene restano accessibili come sempre.
- Il giocatore affronta la scelta binaria al primo avvio prima ancora di raggiungere il Piano
  0 per la prima volta: qualunque sia l'esito (completo o solo curato), il Piano 0 resta
  interamente giocabile e la primissima visita guidata (DEC-047) procede normalmente subito
  dopo (DEC-070).
- Il benchmark consiglia "solo curato" perché l'hardware non regge la generazione, ma il
  giocatore sceglie comunque "completo": la scelta del giocatore è rispettata, il consiglio
  resta solo un'indicazione (DEC-070).

## Fallback

Se un tema, il personaggio alternativo o gli asset del Piano 0 non sono disponibili o non
superano la validazione, si applica la regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- Il Piano 0 non è una stanza di combattimento obbligatoria: le arene sono opzionali.
- Il Piano 0 non applica potenziamenti permanenti al personaggio tra una run e l'altra.
- Il Piano 0 non sostituisce il menu principale (`MainMenu`): resta uno stato successivo a
  `RunSetup`, come da [Game States and Flow](../05-game-states-and-flow.md).

## Domande aperte residue

- Valore esatto della dote iniziale (quali risorse, quale oggetto comune, quantità) —
  DEC-029 fissa solo che sia "piccola", non i numeri. **Aggiornamento 30/07 (WP15a):** la
  dote non è ancora implementata affatto — le simulazioni non lasciano oggi nulla alla run
  in preparazione; vedi "Stato di implementazione: le arene di sfida del Piano 0" e
  `docs/engineering/known-issues.md`.
- Quale criterio rende una run passata "migliore" ai fini dei contenuti best-of di
  un'arena, e quanti contenuti si pescano. **Aggiornamento 30/07 (WP15a):** esiste ora un
  default proposto e implementato (esito, poi piano raggiunto, poi boss sconfitti; una sola
  run, la migliore) — vedi `governance/open-questions.md`, voce 50. Resta un default di
  implementazione, non canone.
- Soglia esatta delle metriche (uso, sopravvivenza col giocatore, contributo alle vittorie)
  che fa scattare la promozione automatica al museo (DEC-063 fissa solo il principio misto,
  non i numeri).

## Scenari

**Scenario: il piano 1 non è ancora pronto**
- Given il giocatore è nel Piano 0 e ha già scelto tema e personaggio
- When il piano 1 non ha ancora superato la validazione
- Then l'uscita verso il piano 1 resta chiusa e il giocatore può continuare a usare museo
  e arene senza essere bloccato

**Scenario: scelta del tema tra le proposte generate**
- Given il giocatore è entrato nel Piano 0 per la prima volta in questa sessione
- When vengono mostrate 2-3 carte tema generate dall'IA
- Then il giocatore ne seleziona una e il riepilogo del Piano 0 mostra il tema scelto

**Scenario: anteprima visiva del tema pronta**
- Given l'IA ha proposto un tema e la sua anteprima visiva ha completato la generazione con
  priorità altissima (DEC-039)
- When la carta tema viene mostrata al giocatore
- Then la carta mostra nome, breve descrizione e l'anteprima visiva generata (es. lo sprite
  campione di un nemico del tema) insieme

**Scenario: anteprima visiva del tema non pronta, fallback nome+descrizione**
- Given l'IA ha proposto un tema ma la sua anteprima visiva non ha completato la
  generazione in tempo
- When la carta tema viene mostrata al giocatore
- Then la carta mostra comunque nome e descrizione, senza anteprima, e resta comunque
  selezionabile (DEC-039)

**Scenario: nessuna proposta di tema supera la validazione**
- Given l'IA propone 2-3 temi per il Piano 0
- When nessuna delle proposte supera la validazione (o l'IA non è disponibile)
- Then il giocatore vede le 3 carte tema curate di fallback, il numero canonico (DEC-076),
  indistinguibili nella forma dalle proposte generate

**Scenario: selezione di una carta tema col mouse**
- Given il giocatore è nel Piano 0 con le carte tema visibili
- When clicca una carta tema col mouse
- Then il tema viene selezionato esattamente come con tastiera o pad, perché il Piano 0
  conta come menu ai fini della parità di input (DEC-057, DEC-075)

**Scenario: cambiare tema dopo averlo già scelto**
- Given il giocatore ha già scelto un tema nel Piano 0
- When seleziona una carta tema diversa prima di attraversare l'uscita
- Then il nuovo tema sostituisce il precedente, la generazione dei piani riparte da capo e
  l'uscita torna chiusa finché il piano 1 del nuovo tema non supera la validazione (DEC-091)

**Scenario: cambiare personaggio dopo averlo già scelto**
- Given il giocatore ha già scelto un personaggio nel Piano 0
- When seleziona un personaggio diverso prima di attraversare l'uscita
- Then il nuovo personaggio sostituisce il precedente nel riepilogo, senza alcun effetto
  sulla generazione dei piani (DEC-091)

**Scenario: abbandono del Piano 0 confermato**
- Given il giocatore è nel Piano 0 con la preparazione della run in corso (tema o
  personaggio già scelti, o generazione dei piani successivi in corso)
- When preme ESC e conferma in `ExitConfirm`
- Then la preparazione si interrompe e il giocatore torna al menu principale (DEC-074)

**Scenario: abbandono del Piano 0 annullato**
- Given il giocatore è nel Piano 0 e apre `ExitConfirm` premendo ESC
- When annulla la conferma
- Then resta nel Piano 0 con la preparazione della run intatta (DEC-074)

**Scenario: provare un oggetto del museo in saletta**
- Given il giocatore consulta il museo e seleziona un oggetto tra le migliori creazioni
  esposte (DEC-040)
- When avvia la prova
- Then entra in una saletta dedicata dove può testare l'oggetto, senza che questo alteri la
  build della run in preparazione

**Scenario: riaffrontare un boss dal museo in arena**
- Given il giocatore consulta il museo e seleziona un boss tra le migliori creazioni
  esposte (DEC-040)
- When sceglie di riaffrontarlo
- Then entra in un'arena di sfida che riusa l'infrastruttura delle arene opzionali del
  Piano 0 (DEC-004), con l'eventuale dote iniziale (DEC-029) applicabile come per le altre
  arene

**Scenario: prove dal museo senza alcun limite**
- Given un giocatore ha già completato più volte la prova di un oggetto in saletta o ha
  riaffrontato un boss in arena dal museo
- When tenta di ripetere la prova ancora una volta
- Then può farlo senza alcun tetto di tentativi, tempo o usi (DEC-095)

**Scenario: prima run in assoluto, senza asset dedicati generati**
- Given non esistono ancora asset dedicati generati per il Piano 0
- When il giocatore avvia il gioco per la prima volta
- Then il Piano 0 mostra la versione statica curata di fallback e resta comunque
  interamente giocabile

**Scenario: arena seminata dal pool curato al primissimo avvio**
- Given il giocatore avvia il gioco per la primissima volta e non esiste ancora alcun
  contenuto "best-of" validato nelle run passate
- When entra nel Piano 0 e vede le arene opzionali
- Then le arene sono comunque disponibili, seminate dal pool curato minimo (DEC-087), e il
  tutorial integrato (DEC-047) funziona normalmente (DEC-094)

**Scenario: uscita verso il piano 1 abilitata**
- Given tema e personaggio sono stati scelti e il piano 1 ha superato la validazione
- When il giocatore raggiunge l'uscita
- Then l'uscita si apre e attraversarla avvia la run sul piano 1

**Scenario: dote iniziale da un'arena completata**
- Given un giocatore completa un'arena di sfida nel Piano 0 prima di avviare una run non
  Classificata
- When attraversa l'uscita verso il piano 1
- Then la run inizia con la piccola dote (risorse o un oggetto comune) guadagnata
  dall'arena, oltre a tema e personaggio scelti

**Scenario: dote disattivata in modalità Classificata**
- Given un giocatore completa un'arena di sfida nel Piano 0 mentre sta per avviare una run
  in modalità Classificata
- When attraversa l'uscita verso il piano 1
- Then la run inizia senza alcuna dote iniziale dall'arena, per coerenza con la parità
  richiesta dalla modalità Classificata (DEC-016, DEC-021)

**Scenario: primissima visita guidata al Piano 0**
- Given un giocatore che avvia il gioco per la primissima volta
- When entra nel Piano 0
- Then le arene opzionali propongono cartelli e prove pratiche che insegnano movimento,
  sparo, risorse e fusione, senza alcun tutorial separato dal resto del gioco (DEC-047)

**Scenario: visite successive senza guida ripetuta**
- Given un giocatore che ha già completato la primissima visita guidata al Piano 0
- When rientra nel Piano 0 in una run successiva
- Then le arene opzionali restano disponibili per allenarsi, ma senza ripresentare cartelli
  o prove pratiche di introduzione (DEC-047)

**Scenario: sconfitta senza costo in un'arena di sfida**
- Given un giocatore che entra in un'arena di sfida opzionale del Piano 0
- When viene sconfitto dentro l'arena
- Then esce dall'arena illeso, senza alcuna perdita sulla run in preparazione, e non
  ottiene la dote iniziale di quell'arena (DEC-055)

**Scenario: lo stato d'ingresso è ripristinato qualunque sia l'esito dell'arena**
- Given un giocatore entra in un'arena di sfida con una certa salute e un certo stato
- When ne esce, sia in vittoria sia in sconfitta
- Then ha esattamente la salute e lo stato con cui era entrato, con la sola eccezione
  dell'eventuale dote guadagnata in caso di vittoria (DEC-092)

**Scenario: presentazione delle prove all'ingresso nel piano 1**
- Given un giocatore nel Piano 0 con tema, personaggio e piano 1 pronti
- When attraversa l'uscita verso il piano 1
- Then il gioco presenta le prove specifiche della run (DEC-042) prima o durante l'ingresso,
  e da quel momento l'elenco resta consultabile dal menu di pausa e dalla schermata build

**Scenario: promozione automatica per metriche**
- Given un contenuto del catalogo supera la soglia di uso, sopravvivenza col giocatore e
  contributo alle vittorie
- When il museo si aggiorna
- Then il contenuto entra nel museo per promozione automatica (DEC-063)

**Scenario: un preferito non esce mai dal museo**
- Given un contenuto del Catalogo è stato segnato come preferito dal giocatore (DEC-045)
- When le metriche di altri contenuti superano le sue
- Then il contenuto preferito resta comunque esposto nel museo, perché la curatela del
  giocatore ha la precedenza sulla promozione per metriche (DEC-063)

**Scenario: un preferito diventato Reliquia resta esposto ma non più provabile**
- Given un contenuto del museo era esposto come preferito del giocatore e diventa una
  Reliquia dopo un aggiornamento del gioco (DEC-069)
- When il giocatore consulta il museo
- Then il contenuto resta esposto con la scheda che segnala "Reliquia", ma la prova in
  arena (DEC-040) non è più disponibile, perché le Reliquie non sono giocabili (DEC-085)

**Scenario: un contenuto promosso solo per metriche esce dal museo diventando Reliquia**
- Given un contenuto del museo era esposto solo per promozione automatica per metriche
  (DEC-063) e diventa una Reliquia dopo un aggiornamento del gioco (DEC-069)
- When il museo si aggiorna
- Then il contenuto esce automaticamente dal museo (DEC-085)

**Scenario: primo avvio con hardware sufficiente**
- Given un giocatore avvia il gioco per la primissima volta
- When il benchmark misura un hardware che regge la generazione
- Then il gioco mostra la schermata dedicata a due carte (DEC-086) con la scelta binaria
  completo/solo curato, senza sconsigliare nessuna delle due opzioni, e procede poi alla
  primissima visita guidata al Piano 0 dopo che il giocatore ha scelto (DEC-070, DEC-047)

**Scenario: primo avvio con hardware insufficiente**
- Given un giocatore avvia il gioco per la primissima volta
- When il benchmark misura un hardware che non regge la generazione
- Then il gioco lo dichiara chiaramente e consiglia "solo curato" nella schermata dedicata a
  due carte, pur lasciando al giocatore la possibilità di scegliere comunque "completo"
  (DEC-070, DEC-086)

**Scenario: il giocatore annulla la scelta binaria al primo avvio**
- Given il giocatore vede la schermata dedicata a due carte dopo il benchmark
- When annulla la scelta o chiude il gioco senza sceglierne una
- Then al rientro la schermata si ripresenta, e il gioco non procede alla primissima visita
  guidata al Piano 0 finché una scelta non viene fatta (DEC-086)

**Scenario: riattivare la generazione dopo aver scelto solo curato**
- Given un giocatore ha scelto "solo curato" al primo avvio
- When apre le Impostazioni in una sessione successiva
- Then trova la voce per riattivare la generazione IA, con la stessa informazione del
  benchmark sull'hardware mostrata al primo avvio (DEC-086)

**Scenario: "solo curato" resta una modalità permanente, non un'attesa**
- Given un giocatore ha scelto "solo curato" al primo avvio
- When gioca run successive senza mai riattivare la generazione
- Then il gioco resta interamente giocabile con contenuti curati e fallback procedurale,
  senza che questa condizione sia trattata come temporanea o come un errore da correggere
  (DEC-070)

**Scenario: il Piano 0 curato è pronto senza alcuna attesa (DEC-153)**
- Given un giocatore avvia il gioco per la primissima volta, prima che qualunque
  generazione IA abbia prodotto un solo asset per il Piano 0
- When entra nel Piano 0
- Then trova immediatamente il contenuto curato di base, già pronto e completo, senza
  alcuna schermata o indicatore di attesa: il contenuto curato è lo stato base del gioco,
  non un ripiego temporaneo (DEC-153)

**Scenario: HUD nascosto fuori dalle arene del Piano 0**
- Given un giocatore nel Piano 0 sta scegliendo il tema o consultando il museo, fuori da
  qualunque arena
- When osserva lo schermo
- Then l'HUD di combattimento resta nascosto (DEC-169)

**Scenario: HUD consultabile dal menu di pausa nel Piano 0**
- Given un giocatore nel Piano 0, con l'HUD di combattimento normalmente nascosto
- When apre il menu di pausa
- Then può consultare le stesse informazioni dell'HUD (vite, risorse, timer di run) da lì
  (DEC-169)

**Scenario: HUD visibile durante un'arena di sfida**
- Given un giocatore entra in un'arena di sfida del Piano 0 (opzionale o dal museo)
- When il combattimento nell'arena è in corso
- Then l'HUD di combattimento torna visibile, come nei piani 1-5 (DEC-169)

**Scenario: si entra in un'arena solo con una conferma esplicita (WP15a)**
- Given un giocatore che gira nel crogiolo e attraversa una piazzola d'arena senza premere
  nulla
- When ci cammina sopra
- Then non entra in alcuna simulazione: la piazzola resta lì e serve il tasto di
  interazione a contatto per aprirla

**Scenario: il cartello del tutorial è per piazzola, non globale (DEC-047, WP15a)**
- Given un giocatore che ha già visitato la piazzola delle risorse e ne rivede il cartello
  scomparso alla seconda visita
- When entra per la prima volta nella piazzola della fusione
- Then quella piazzola mostra il proprio cartello, perché "la primissima visita" vale per
  ciascuna arena separatamente
