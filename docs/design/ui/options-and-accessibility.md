---
id: gd-ui-options-accessibility
title: Options and Accessibility
domain: design
status: approved
authority: canonical
owner: design
summary: "Impostazioni e accessibilità, incluso lo schema di controllo approvato. Parità rigorosa tastiera/controller (DEC-057), con il Piano 0 che conta come menu ai fini del mouse (DEC-075), e tre garanzie canoniche di accessibilità: rimappatura totale, nessuna informazione affidata al solo colore, riduzione effetti (DEC-058)."
last_reviewed: 2026-07-31
last_verified_commit: 4d7a410
topics: [opzioni, accessibilita, input-parity, controller, lettore-di-schermo, DEC-057, DEC-058, DEC-075, DEC-086, DEC-166, DEC-189, DEC-190]
related: []
supersedes: []
source_files: [src/app/app.c, src/audio/audio.h, src/render/game_renderer.c]
---

# Options and Accessibility

## Intento

Dare al giocatore controllo su comodità, accessibilità e chiarezza visiva senza alterare
in modo nascosto l'equilibrio competitivo.

## Condizioni di ingresso

Da `MainMenu` o da `PauseMenu`; al ritorno, il focus torna alla voce che ha aperto `Options`
(vedi `ui/navigation-map.md`, `ui/pause-menu.md`, `ui/main-menu.md`).

## Categorie minime

- audio;
- video;
- controlli (rimappatura di movimento libero e sparo a 4 direzioni, DEC-007; parità
  tastiera/controller, DEC-057);
- accessibilità;
- gameplay (per chi ha scelto "solo curato" al primo avvio: voce di riattivazione della
  generazione IA, DEC-086, vedi sotto);
- privacy e online, se applicabile.

### Stato di implementazione: la categoria audio (W8, 2026-07-30)

Delle sei categorie minime esiste oggi **solo l'audio**, con tre righe-slider. Le altre
cinque restano da scrivere e W8 non le ha inventate: `APP_OPTIONS` mostra ancora, in coda,
la sola informazione consultabile che aveva prima ("Schermo intero -- F11").

I tre volumi (`Volume generale`, `Musica`, `Effetti`) sono agganciati ad
`AudioSetMasterVolume`/`AudioSetMusicVolume`/`AudioSetSfxVolume` (`src/audio/audio.h`):
il master **moltiplica** musica e SFX, non è un quarto canale, e le tre funzioni clampano
già in [0,1] — il clamp vive in un solo posto, quello che possiede il valore.

Perché sono **voci di menu a pieno titolo** e non un widget a parte: stesso indice, stessa
geometria, stesso hit-test del mouse delle altre righe, perché la parità rigorosa di
DEC-057 vale anche per uno slider. Su/giù scelgono la riga, sinistra/destra cambiano il
valore, ESC esce sempre, ENTER esce solo dalla riga "Indietro" — su una riga-slider un
ENTER non ha significato (il valore è già applicato) e chiudere la schermata sarebbe una
sorpresa mentre si sta regolando. Il suono di navigazione fa da **anteprima** del volume
appena scelto: è l'unico modo di sentire l'effetto dello slider SFX senza uscire dal menu.
Il valore si legge sia dalle dieci caselle sia dalla percentuale scritta accanto — nessuna
informazione affidata al solo colore o alla sola lunghezza (DEC-058).

**Canone (DEC-190, 31/07):** passo del **10%** su dieci caselle, ordine
`generale`/`musica`/`effetti`, partenza a 1.0 — confermati dal proprietario, non più un
default proposto.

**Persistenza — canone (DEC-189, 31/07):** nasce un file di preferenze del giocatore,
`prefs/settings.txt`, accanto a `catalog/` e con le stesse regole di quel file (dati del
giocatore, mai versionato, tmp+rename atomico, formato chiave=valore con campo di versione,
disciplina zero-default). Primo contenuto: i tre volumi audio. Questa decisione fissa
esistenza, percorso e formato; l'**implementazione resta un gap dichiarato** — oggi il gioco
non legge/scrive ancora questo file e riparte da 1.0 a ogni avvio, vedi voce **9** di
`docs/engineering/known-issues.md` per lo stato del difetto noto. Le domande aperte **24** e
**25** in `../governance/open-questions.md` sono chiuse; questo file di preferenze è
distinto dal salvataggio di run (`suspend/current.txt`, DEC-202) e dal Catalogo.

## Parità rigorosa di input (DEC-057)

Ogni scelta di design deve funzionare in modo **identico su tastiera e su controller**:
questo è un vincolo esplicito su tutti i documenti UI della KB, non solo su questo. Nessuna
meccanica può richiedere un dispositivo specifico per essere completata. Il **mouse è
ammesso solo nei menu** (navigazione, selezione), mai come requisito per un'azione di
`Gameplay`. Questo documento è la **fonte unica** della regola di parità di input; gli
altri documenti UI vi rimandano senza riformulare (vedi ad es.
[Navigation Map](navigation-map.md)).

**Il Piano 0 conta come menu ai fini di questa regola (DEC-075).** Gli elementi di
interfaccia selezionabili del Piano 0 — carte tema, schede personaggio, pannelli — sono
cliccabili col mouse esattamente come le voci di menu degli altri stati. Il movimento del
personaggio nel Piano 0 e ogni meccanica giocata restano invece su tastiera/controller con
parità rigorosa: il mouse non muove mai il personaggio e non è mai richiesto. Dettagli di
input/azioni del Piano 0: `systems/floor-zero.md` (rimando, non riformulato qui).

### Stato di implementazione: copertura mouse totale (W9, playtest round 1, 2026-07-30)

Il primo playtest del proprietario ha trovato menu non navigabili col mouse. Chiuso: ogni
schermata ha ora hover che sposta il fuoco e click che attiva, con la stessa geometria usata
per disegnare (`RendererMenuItemAt`/`RendererBuildItemRowAt`/`RendererFloorZeroCardAt` e
affini, `src/render/game_renderer.c`) — mai una copia duplicata del layout. In particolare:

- le carte tema e le schede personaggio del pannello combinato del Piano 0 (M5/M6a) sono
  ora cliccabili, chiudendo il gap dichiarato sopra fin da M5/M6a; anche le due schedine
  MONDI/PERSONAGGI e il fumetto "TAB -- mondo e personaggio" (che apre il pannello) sono
  cliccabili;
- `BuildScreen`: le righe della lista OGGETTI PRESI sono cliccabili per scegliere le
  sorgenti della fusione (DEC-143), con la rotellina del mouse che scorre la lista scorrevole
  oltre la finestra visibile (senza, un giocatore SOLO mouse non avrebbe potuto raggiungere
  un oggetto oltre i primi visibili) — "Indietro" era già cliccabile, ora lo sono anche le
  righe. Anche la **riga di conferma della fascia «Fusione»** è cliccabile e vale come il
  tasto `[F]` (aggiunta nella correzione del round 1 qui sotto): senza di essa nessun
  percorso col solo mouse portava a termine una fusione, cioè la riga "Conferma fusione" di
  [Inventory and Synergy Screen](inventory-and-synergy-screen.md) restava fuori dalla
  copertura. Il documento non fissa il tasto, quindi le due vie convivono e l'etichetta le
  nomina entrambe;
- `Options`: le tre barre del volume sono trascinabili col mouse (click-e-trascina applica
  il valore sotto il puntatore in continuo), non solo regolabili a passi con sinistra/destra
  da tastiera;
- `RunSetup`: il reroll del seed (riga "Seed") era già raggiungibile col click (che equivale
  a un ENTER sintetico sulla voce), nessun gap qui.

Il **GAMEPLAY resta senza mouse** (DEC-057 non tocco nel suo nucleo): il movimento del
personaggio e ogni meccanica giocata restano tastiera/controller, il mouse non è mai
richiesto. La vista Catalogo (dentro `MainMenu`) resta tastiera-sola DENTRO la vista (v1,
`ui/results-and-leaderboards.md`/M8): una scelta di scope invariata da questo lavoro, non un
gap.

### Correzione (round 0 del playtest, stesso lavoro W9): il fuoco segue il mouse SOLO se il mouse si muove

La prima versione della copertura totale sopra riscriveva il fuoco ad ogni singolo frame
con la voce sotto il puntatore, senza controllare che il puntatore si fosse davvero
spostato. Un puntatore lasciato fermo su una voce — la situazione normale subito dopo un
click, o quando il giocatore torna a tastiera/pad senza aver mai toccato il mouse — uccideva
così la navigazione da tastiera/pad ad ogni frame successivo: violava DEC-057 (parità
rigorosa) invece di realizzarla, e rompeva anche il ritorno del fuoco previsto da
[Navigation Map](navigation-map.md). Il caso più visibile: il default non distruttivo
"Annulla" di `ExitConfirm` veniva ribaltato su "Conferma" dal solo hover, e in `Options` un
puntatore fermo sulla riga "Indietro" rendeva lettera morta sinistra/destra sui volumi.

Fissato spostando il fuoco per hover solo quando la posizione del mouse è cambiata
**rispetto al frame precedente** (non rispetto al click più recente): un click resta un
evento discreto e continua a funzionare a prescindere, l'hover no. Stesso gate per il passo
generico, per le righe di `BuildScreen` e per le carte/schedine del pannello del Piano 0 —
un solo confronto per frame in cima a `UpdateApp`. Questo gate **non bastava**: chiudeva i
casi col puntatore fermo, non quelli col puntatore in movimento, che sono proprio quelli in
cui il giocatore usa il mouse — vedi la correzione del round 1 qui sotto.

Nello stesso passaggio: il trascinamento del mouse sulle barre di `Options` ora si aggancia
al passo del **10%** su dieci caselle (sopra, "Default proposti") invece di produrre un
valore continuo — un valore che nessun input da tastiera potrebbe mai produrre e che la
barra a caselle non potrebbe rappresentare fedelmente; e il click che avvia il trascinamento
non sintetizza più un "confirm" (niente sfx di conferma ad ogni inizio di regolazione,
coerente con "su una riga-slider un ENTER non ha significato" sopra).

### Correzione (round 1 del playtest, stesso lavoro W9): il click agisce sull'elemento cliccato, e la lista non scorre da sola

Due difetti funzionali restavano dentro la copertura appena descritta, entrambi invisibili al
gate "solo se il mouse si muove".

**1. Il click sulle carte del Piano 0 confermava il fuoco, non la carta cliccata.** Con il
puntatore fermo sull'area del pannello e il pannello aperto da tastiera (TAB: il mouse non si
muove, quindi nessun hover gira), un click sulla carta sotto il puntatore confermava la carta
a fuoco — un'altra. La scelta del mondo è **irreversibile** (avvia la generazione della run),
quindi non era un difetto recuperabile. Fissato facendo agire il click sulla carta
**davvero cliccata**: il click sposta anche il fuoco lì (come già faceva il blocco generico
dei menu), e la conferma usa l'indice della carta colpita, non il fuoco — che le frecce da
tastiera potrebbero aver spostato nello stesso frame. Vale per entrambe le sezioni, MONDI e
PERSONAGGI.

**2. La lista OGGETTI PRESI di `BuildScreen` scorreva da sola sotto il puntatore.** La
finestra visibile della lista scorrevole era **derivata dalla riga a fuoco** (la riga a fuoco
sempre in fondo alla finestra), quindi l'hover — che scrive il fuoco — alimentava se stesso:
ogni frame di *movimento* del mouse faceva scorrere la lista di uno step, e con l'inventario
pieno la lista schizzava in cima sotto il puntatore, annullando la rotellina appena usata. Il
gate del round 0 non lo toccava, perché il difetto scatta proprio quando il mouse si muove.
Fissato **separando l'ancora di scorrimento dal fuoco**: la finestra visibile dipende ora
solo dall'ancora (un campo di interfaccia a sé), che si sposta del minimo indispensabile
quando il fuoco esce dalla finestra — e mai per un hover, che per definizione cade su una
riga già visibile. La garanzia visibile al giocatore resta la stessa di prima ("la riga a
fuoco è sempre visibile"), senza la retroazione.

Regola generale che questi due difetti hanno reso esplicita, valida per tutte le schermate:
**l'hover è continuo e va filtrato, il click è discreto e agisce sull'elemento sotto il
puntatore** — mai sul fuoco, che l'hover può non aver aggiornato.

## Garanzie di accessibilità canoniche (DEC-058)

Tre garanzie sono **canone approvato**, non semplici voci di progettazione:

1. **Rimappatura totale** di ogni input, su tastiera e su pad.
2. **Nessuna informazione di gioco affidata al solo colore**: forme e pattern distinti
   comunicano ogni informazione di gioco anche senza percezione del colore; si aggancia al
   budget di leggibilità, fonte unica
   [Combat and Projectiles](../systems/combat-and-projectiles.md) (rimando, non
   riformulare).
3. **Opzione di riduzione effetti** (particelle, scuotimenti, lampi — anche per
   fotosensibilità) che **non altera le informazioni di gioco**: riduce solo la resa
   visiva, mai il contenuto informativo di un telegraph o di un segnale.

La **"modalità assistita" (riduzione della difficoltà) non è nel canone**: queste tre
garanzie riguardano accessibilità e chiarezza, non un abbassamento del livello di sfida
(coerente con DEC-038, difficoltà unica senza livelli selezionabili). Le eventuali
assistenze o velocità che renderebbero una run non classificata restano, distintamente, una
domanda aperta (vedi sotto), non una modalità assistita canonica.

## Obiettivi non-garanzia (DEC-166)

Distinto dalle tre garanzie canoniche sopra, un **obiettivo** di accessibilità non ancora
promosso a garanzia:

- **Lettore di schermo**: supporto **circoscritto a `MainMenu` e ai menu testuali
  semplici** (vedi `ui/main-menu.md`). **Non è esteso al gameplay** e **non entra fra le
  garanzie canoniche di DEC-058**: resta un obiettivo, con perimetro esplicito, non una
  promessa canonica valida su tutta l'interfaccia. Fonte unica di questo perimetro:
  questo documento (DEC-166); `ui/main-menu.md` vi rimanda senza riformulare.

## Riattivazione della generazione IA (DEC-086)

Il giocatore che al primissimo avvio ha scelto "solo curato" trova in `Options` (categoria
gameplay) la voce per **riattivare la generazione IA**. Il resto della regola — dove e
quando avviene la scelta al primo avvio, l'assenza di un default silenzioso e
l'informazione del benchmark che accompagna la voce — ha fonte unica in
`systems/floor-zero.md` (DEC-070/DEC-086, rimando, non riformulato qui).

## Accessibilità da progettare

- rimappatura controlli (canonica, DEC-058, sopra);
- supporto controller e tastiera con parità rigorosa (canonica, DEC-057, sopra);
- riduzione flash e particelle (canonica, DEC-058, sopra);
- contrasto di proiettili e minacce (coerente col budget di leggibilità, fonte unica `systems/combat-and-projectiles.md`);
- dimensione testi;
- alternative ai soli colori (canonica, DEC-058, sopra);
- velocità o assistenze in modalità non classificata (da non confondere con una modalità
  assistita di riduzione difficoltà, esclusa dal canone da DEC-058);
- descrizioni leggibili degli oggetti (Innesti compresi).

## Regola

Le opzioni che alterano la difficoltà competitiva devono essere dichiarate e gestite dalle
regole della classifica (vedi `ui/results-and-leaderboards.md`, DEC-016).

## Non-obiettivi

- Non ridefinisce lo schema di controllo di base (movimento libero, sparo a 4 direzioni): solo la sua rimappatura.
- Non introduce una modalità di riduzione della difficoltà: esclusa esplicitamente dal canone (DEC-058).

## Domande aperte residue

- Le assistenze e le velocità esatte che rendono una run non classificata restano da
  dettagliare in `governance/open-questions.md` (sezione Multiplayer); restano distinte
  dalle tre garanzie di accessibilità canoniche (DEC-058).

## Scenari verificabili

1. **Given** il giocatore apre `Options` da `PauseMenu`, **when** torna indietro, **then** il focus ritorna sull'elemento "Opzioni" di `PauseMenu`.
2. **Given** il giocatore attiva un'assistenza dichiarata come non classificante, **when** avvia una run competitiva, **then** la run viene etichettata come non classificata.
3. **Given** il giocatore aumenta il contrasto di proiettili e minacce, **when** rientra in `Gameplay`, **then** il budget di leggibilità applicato resta coerente con `systems/combat-and-projectiles.md`.
4. **Given** una qualunque meccanica di `Gameplay`, **when** il giocatore la esegue solo con tastiera oppure solo con controller, **then** il risultato è identico in entrambi i casi, e il mouse non è mai richiesto (DEC-057).
5. **Given** il giocatore attiva l'opzione di riduzione effetti, **when** rientra in `Gameplay`, **then** particelle, scuotimenti e lampi sono ridotti, ma nessuna informazione di gioco (telegraph, minacce, stato) va persa (DEC-058).
6. **Given** un giocatore daltonico che gioca senza alcuna assistenza di colore, **when** osserva una minaccia o un indicatore di stato, **then** riesce comunque a distinguerlo perché l'informazione è comunicata anche da forma e pattern, non solo dal colore (DEC-058).
7. **Given** il giocatore è nel Piano 0, **when** clicca col mouse su una carta tema, una scheda personaggio o un pannello, **then** l'elemento viene selezionato come da una voce di menu; **but when** prova a muovere il personaggio o a giocare una qualunque meccanica del Piano 0, **then** il mouse non ha alcun effetto e l'azione richiede tastiera o controller (DEC-075).
8. **Given** un giocatore ha scelto "solo curato" al primissimo avvio, **when** apre `Options` nella categoria gameplay, **then** trova la voce per riattivare la generazione IA, accompagnata dalla stessa informazione del benchmark sull'hardware mostrata al primo avvio (DEC-086).
9. **Given** un giocatore usa un lettore di schermo, **when** naviga `MainMenu` o un menu testuale semplice, **then** trova supporto come obiettivo dichiarato; **but when** entra in `Gameplay`, **then** nessun supporto al lettore di schermo è promesso, perché il perimetro non si estende al gameplay e non è fra le garanzie canoniche di DEC-058 (DEC-166).
