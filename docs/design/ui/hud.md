---
id: gd-ui-hud
title: HUD
domain: design
status: approved
authority: canonical
owner: design
summary: "Salute stratificata, risorse per funzione, slot attivo e Innesto. Stile pixel art come tutta la UI (DEC-046, fonte unica in content/visual-language.md). Timer di run sempre visibile in ogni momento del gameplay, non solo in competitivo (DEC-051). Alla prima occorrenza di un contenuto generato mai visto, una card di scoperta breve appare in coda, non bloccante (DEC-065). L'HUD in pixel art della demo è disegnato per il canvas logico attuale 960×640, senza attendere la risoluzione logica definitiva (DEC-174, domanda aperta 11). Un blocco compatto di statistiche correnti (danno, cadenza, velocità del colpo, velocità di movimento, raggio, Fortuna) è visibile di default sotto salute/risorse, con tasto di toggle (DEC-184)."
last_reviewed: 2026-07-30
last_verified_commit: 1263957
topics: [hud, gameplay, salute, risorse, timer-run, card-scoperta, floor-zero, DEC-065, DEC-051, DEC-152, DEC-169, DEC-174, canvas-960x640, DEC-184, statistiche]
related: []
supersedes: []
source_files: [src/core/game_types.h, src/game/game.c, src/game/game_internal.h, src/world/world.c, src/gameplay/combat.c, src/render/game_renderer.c, src/render/game_renderer.h, src/render/art_draw.h, src/assets/art_atlas.h, src/app/app.c, src/app/app_internal.h, src/tests/discovery_tests.c, scripts/cp2_hud_mocks.lua]
---

# HUD

> Aggiunta del 22/07 (DEC-137): la GUI vive **in overlay sulla game view a tutto schermo**
> — una sola schermata, niente pannelli laterali che sottraggono spazio al mondo; i
> pannelli diventano overlay adattivi/a comparsa. Refactor in corso.

## Intento

Mostrare durante `Gameplay` solo le informazioni necessarie a decisioni immediate di
sopravvivenza e di gestione delle risorse.

## Condizioni di ingresso

Sempre visibile durante `Gameplay`; nascosto o attenuato durante `PauseMenu` e `BuildScreen`.

In `FloorZero` l'HUD segue una regola diversa (DEC-169): **nascosto** durante
l'esplorazione dell'hub, **consultabile su richiesta** aprendo il menu di pausa per chi
vuole controllare salute, risorse e build senza uscire dal Piano 0, e **di nuovo visibile**
quando il giocatore entra in una **prova del Piano 0** — le arene di sfida e il tutorial
integrato (DEC-047), da non confondere con le «prove» specifiche della run di DEC-042. Il
Piano 0 resta uno spazio di preparazione con lo schermo pulito, senza togliere informazione a
chi la cerca. Dettagli delle prove e della consultazione in pausa: fonte unica
`systems/floor-zero.md` e `ui/pause-menu.md` (rimando, non riformulato qui).

> **Nota di implementazione (demo W3, 2026-07-28):** la regola di visibilità è ora una
> funzione pura testabile, `HudCombatShouldDraw(mode, floorZeroTrialActive)`
> (`src/render/game_renderer.h`/`.c`): vero in `Gameplay` sempre, vero in `FloorZero` solo se
> `Game.floorZeroTrialActive`, falso altrimenti — `RendererDrawApp` la chiama al posto del
> vecchio confronto diretto su `APP_GAMEPLAY`. `floorZeroTrialActive` è l'**hook pronto** per
> le prove del Piano 0 (arene di sfida, DEC-004/047): nessun codice lo imposta ancora a vero,
> perché le arene non esistono nel motore (gap esplicito stile DEC-009/052, matrice di
> copertura §17) — non sono state inventate qui. Quando arriveranno, l'entrata/uscita
> dall'arena scriverà quel flag e l'HUD ricomparirà senza altro lavoro sul renderer. Verificato
> da `--discovery-test` (`src/tests/discovery_tests.c`).

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Salute base | Sempre | — (sola lettura) | Nessuna | — | Rappresentazione distinta dalla salute temporanea |
| Salute temporanea/protettiva | Il giocatore ne possiede | — (sola lettura) | Nessuna | — | Livello visivamente separato dalla salute base; si consuma per prima (DEC-008) |
| Valuta principale | Sempre | — (sola lettura) | Nessuna | — | Contatore numerico |
| Strumento di breccia | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Strumento di apertura | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Catalizzatore di fusione | Sempre | Disponibile se quantità > 0 | Nessuna diretta (si spende in stanza di fusione) | — | Contatore numerico, evidenziato quando sufficiente per una fusione |
| Slot attivo | Sempre | Un oggetto attivo è equipaggiato e carico | Attiva l'oggetto attivo | Effetto dell'oggetto attivo | Indicatore di carica/cooldown |
| Slot Innesto | Sempre | Un Innesto è equipaggiato | Passiva, nessuna azione diretta dall'HUD | — | Icona dell'Innesto attivo |
| Piano e stanza | Sempre | — (sola lettura) | Nessuna | — | Indicatore di progressione |
| Timer di run | Sempre, durante `Gameplay` | — (sola lettura) | Nessuna | — | Contatore del tempo trascorso, sempre visibile in ogni modalità (DEC-051) |
| Stato competitivo essenziale | Modalità competitiva attiva | — (sola lettura) | Nessuna | — | Indicatore minimo aggiuntivo, distinto dal timer di run sempre visibile (DEC-051) |
| Card di scoperta breve (DEC-065) | Alla prima occorrenza di un contenuto generato mai visto (oggetto, nemico, boss, sinergia/fusione) | — (non bloccante, non mette in pausa) | Nessuna azione richiesta; si accoda automaticamente se altre card sono in corso (coda limitata ~5, le più vecchie si perdono senza essere mostrate: DEC-131) | Mostra sprite, nome e una riga di descrizione del contenuto scoperto | Appare e scompare da sola senza bloccare l'input; una sola card visibile alla volta, le altre attendono in coda |
| Blocco statistiche (DEC-184) | Sempre, durante `Gameplay` (visibile di default) | — (sola lettura) | Tasto di toggle nasconde/mostra il blocco | Nessun effetto sulla simulazione, solo sulla visibilità | Danno, cadenza, velocità del colpo, velocità di movimento, raggio, Fortuna — stessi valori del pannello "Statistiche principali" di `BuildScreen`, aggiornati in tempo reale |

## Principio

L'HUD mostra informazioni necessarie a decisioni immediate. Dettagli complessi delle
sinergie e della fusione appartengono a `BuildScreen` (vedi
`ui/inventory-and-synergy-screen.md`).

## Salute stratificata

La salute base e la salute temporanea/protettiva devono essere distinguibili a colpo
d'occhio (colore, forma o strato separato); l'ordine di consumo è sempre: prima la
temporanea, poi la base (DEC-008). Fonte di sistema: `systems/health-and-resources.md`.

## Risorse per funzione

Valuta principale, strumento di breccia, strumento di apertura e catalizzatore di fusione
sono definiti per funzione (DEC-013): nessun riferimento al set cuori/monete/bombe/chiavi di
altri giochi. Il catalizzatore di fusione è una risorsa nuova, distinta dalle altre tre.

I nomi mostrati in gioco sono quelli inglesi della nomenclatura ufficiale (Ingots, Blast
Charges, Cast Keys, Flux — DEC-072, fonte unica [Glossary](../governance/glossary.md), non
riformulata qui). Le icone di queste risorse mantengono una silhouette stabile tra i World,
con variazione ammessa solo in palette e dettagli (DEC-073b, fonte unica
[Visual Language](../content/visual-language.md), non riformulata qui).

## Slot attivo e Innesto

Si parte con 1 slot attivo e 1 slot Innesto; oggetti o eventi rari possono aggiungere slot
durante la run (DEC-011). L'HUD mostra sempre lo stato corrente degli slot posseduti, non
il numero massimo teorico.

Gli oggetti equipaggiati si sovrappongono visivamente al personaggio secondo gli stessi
strati/slot visivi definiti in [Visual Language](../content/visual-language.md),
indipendentemente dal fatto che lo sprite del personaggio sia curato o generato (DEC-049);
questo documento non ripete quel dettaglio.

## Blocco statistiche (DEC-184)

Emersa dal primo playtest reale della demo (30/07): il giocatore vuole consultare le
statistiche correnti di build senza dover aprire `BuildScreen`. L'HUD di `Gameplay` mostra
quindi un **blocco compatto delle statistiche correnti**: **danno, cadenza, velocità del
colpo, velocità di movimento, raggio, Fortuna** — le stesse statistiche, con lo stesso
principio di sola lettura e aggiornamento in tempo reale, del pannello "Statistiche
principali" di `BuildScreen` (vedi
[Inventory and Synergy Screen](./inventory-and-synergy-screen.md)): non è una fonte di
dati diversa, solo una seconda collocazione consultabile senza uscire dal gameplay.

**Default proposti dall'implementazione** (stile DEC-019, da confermare):

- **Collocazione:** sotto il blocco salute/risorse.
- **Visibilità:** visibile **di default**, con un **tasto di toggle** (tasto esatto da
  assegnare in implementazione) per nasconderlo a chi preferisce uno schermo più pulito.

> **Nota di implementazione (W9, 2026-07-30):** blocco disegnato, `DrawHudV3Stats`
> (`src/render/game_renderer.c`), componenti V3 (font 5px, cornice 9-patch `ArtDrawPanel`
> con ripiego `DrawHudBox`), sotto la riga risorse (priorità 4, mai sopra cuori/risorse/
> caselle attivo-Innesto). **Nessun calcolo duplicato**: `HudStatRowsFill` è l'UNICA
> funzione che legge `Player` per le sei righe — sia questo blocco sia il pannello
> "PERSONAGGIO" di `BuildScreen` la chiamano, mai un `TextFormat` locale proprio.
> **Tasto di toggle: `C`** (default proposto dall'implementazione, stile DEC-019 — non in
> conflitto con W/A/S/D/E/Q/F/R/TAB/SPACE, mnemonico per "Character"/statistiche del
> PERSONAGGIO). Vive su `AppInput.toggleStats`/`AppUi.hudStatsHidden`
> (`src/app/app_internal.h`, `src/core/game_types.h`): la preferenza sta su `AppUi`, non su
> `Game`, apposta per sopravvivere a `GameResetRun`/`FloorZeroEnter` (un giocatore che
> nasconde il blocco non se lo ritrova acceso alla run successiva). Zero-default falso =
> **visibile**, come il documento chiede. Rispetta DEC-169 (nascosto col resto dell'HUD nel
> Piano 0 fuori da una prova): il chiamante è `DrawHudCanvas`, chiamata solo quando
> `HudCombatShouldDraw` lo consente, nessuna seconda regola di visibilità. Domanda aperta
> registrata per il proprietario sul tasto esatto. Verificato da `--states-test`
> (cablaggio input→toggle) e screenshot manuale `--hud-stats-screenshot-test`
> (`logs/worldsmelt-hud-stats-visible-screen.png`/`-hidden-screen.png`).

## Timer di run sempre visibile (DEC-051)

Il tempo trascorso nella run è **sempre visibile** nell'HUD durante `Gameplay`, in ogni
modalità, non solo nelle modalità competitive: il gioco si dichiara esplicitamente una
corsa. Questo è distinto dall'indicatore minimo di stato competitivo (vedi tabella sopra),
che resta specifico delle modalità competitive e non ripete il timer generale.

Il timer di run è anche il segnale con cui il giocatore valuta se raggiungere in tempo le
stanze a tempo dei piani avanzati (vedi [Rewards and Economy](../systems/rewards-and-economy.md)
e [Special Rooms](../systems/special-rooms.md), DEC-051); questo documento non ripete il
dettaglio di quell'archetipo.

## Card di scoperta breve (DEC-065)

Alla prima occorrenza in assoluto di un contenuto generato mai visto dal giocatore — oggetto,
nemico, boss, sinergia/fusione — il gioco mostra una **card di scoperta breve** nell'HUD:
sprite, nome, una riga di descrizione. La card **non mette in pausa** la simulazione e **non
blocca l'input**: il giocatore continua a muoversi e a combattere mentre la card è visibile.

I dettagli completi del contenuto scoperto vivono nella scheda dedicata del Catalogo (vedi
[Save and Meta Progression](../systems/save-and-meta-progression.md), DEC-045); questa card
è solo un annuncio rapido, non la sostituisce.

Regola di coda: **una sola card alla volta**. Se più scoperte arrivano insieme, si accodano
ed escono in sequenza, senza invadere lo schermo con più card contemporaneamente. I casi
limite di questa coda sono risolti: il cap e l'overflow da DEC-131 (vedi sotto), e il
destino delle card ancora in attesa quando il giocatore muore o cambia stanza da DEC-152
(vedi sotto).

Se il giocatore **muore** o **cambia stanza** mentre altre card attendono in coda, quelle
non ancora mostrate vengono **scartate silenziosamente** (DEC-152): nessuna coda che
insegue il giocatore nella stanza successiva, nessun recupero differito. La scoperta resta
comunque registrata nel Catalogo permanente con la sua scheda, esattamente come
nell'overflow di DEC-131: la card è la notifica, non il contenuto.

> **Nota di implementazione (demo W3, 2026-07-28):** la coda esiste nel motore —
> `Game.discoveryQueue`/`discoveryQueueCount` (cap `DISCOVERY_QUEUE_MAX`=5, FIFO che scarta
> la più vecchia senza mostrarla oltre il cap, DEC-131) e `Game.discoveryActive` per la card
> correntemente in mostra (`src/core/game_types.h`). `GameQueueDiscoveryCard`/
> `GameDiscardPendingDiscoveries` (`src/game/game.c`) sono i soli punti di scrittura, e
> `GameUpdate` (`src/game/game.c`) è il solo punto che promuove la coda in `discoveryActive`.
> **La card si disegna davvero**: `DrawHudDiscovery` (`src/render/game_renderer.c`) è il
> quinto cluster di `DrawOuterUi`, un riquadro in alto al centro (fuori dai quattro angoli di
> Vitals/RunStatus/Build/Log) che legge `discoveryActive`/`discoveryActiveValid` — prima di
> questo cluster quei campi non avevano alcun lettore nel binario di gioco (solo nei test) e
> la card, pur accodata correttamente, non compariva mai a schermo: una versione precedente di
> questa nota affermava per errore che fosse "solo testo, senza sprite", quando in realtà non
> era disegnata affatto. **Da W8 la card ha anche lo sprite** che questo documento chiede
> ("sprite, nome, una riga di descrizione"): `DiscoveryCard.imageId` porta l'image-id di
> DEC-175(b), `GameQueueDiscoveryCardWithImage` lo accoda (i due call site veri, nemico e
> boss, passano l'image-id del tipo incontrato) e `DrawHudV3Card` lo risolve con
> `ArtAtlasFindByImageId`. Un contenuto senza image-id — ogni nemico inventato dal modello —
> lascia la casella dello sprite vuota, mai un rettangolo bianco.
> La card si è spostata in **basso al centro**, come il layout V3 approvato al CP2: in alto
> avrebbe coperto la riga piano/mondo, che in V3 è allineata a destra a quella stessa quota.
> La formulazione "in alto al centro" di questo documento descriveva l'HUD a quattro cluster
> con riquadro, sostituito da V3 — divergenza registrata come **domanda aperta 26**.
> Lo scarto DEC-152 è agganciato a `CombatDamagePlayer` (morte, `src/gameplay/combat.c`) e a
> `WorldTryEnterRoom` (cambio stanza, `src/world/world.c`), **prima** che la stanza di arrivo
> possa accodare le proprie scoperte — verificato che l'invariante "registrazione alla
> scoperta, non alla mostra" regge: `Game.enemyEncountered`/`bossEncountered` si scrivono in
> `WorldSpawnCombatRoom`/`WorldSpawnRoomContents` indipendentemente dalla coda, e lo scarto
> non li tocca mai. `--discovery-test` (`src/tests/discovery_tests.c`) esercita anche il push
> REALE da `WorldSpawnCombatRoom` (una sola card per tipo incontrato, nessun secondo push su
> una stanza già incontrata) e il cap/drop-oldest di DEC-131 (sei push in fila lasciano le
> cinque più recenti), non solo `GameQueueDiscoveryCard` chiamata a mano. Push accodati oggi:
> solo nemico/boss **incontrato per la prima volta in questa run** (proxy di "mai visto",
> non ancora la cross-run vera del Catalogo persistente — gap dichiarato, dipende dal
> profilo persistente di `systems/save-and-meta-progression.md`, non ancora implementato).

## Stile visivo (DEC-046, rimando)

L'HUD, come tutta l'interfaccia del gioco, è pixel art: fonte unica della regola è
[Visual Language](../content/visual-language.md), non riformulata qui.

## Stato di implementazione (W8, 2026-07-30)

L'HUD **è** in pixel art e usa gli asset veri. Cosa questo significa concretamente, per
chi legge il codice:

- **Si disegna DENTRO il canvas 960×640**, non più in overlay sullo schermo — attuazione
  diretta di DEC-174 qui sotto. Conseguenza pratica decisiva: le coordinate del codice sono
  **esattamente** quelle del layout V3 approvato al CP2 (`scripts/cp2_hud_mocks.lua`,
  variante `CP2-V3-minimal`), numero per numero, invece di essere riderivate da un fattore
  di scala dell'interfaccia. L'HUD scala così con la game view a passi interi, e un pixel
  dell'icona di un cuore resta grande come un pixel del pavimento: è la sola cosa che fa
  leggere l'insieme come pixel art e non come due grafiche sovrapposte. Le quote sono
  raccolte in un blocco di `#define HUD_V3_*` in `src/render/game_renderer.c` (criterio di
  accettazione di `docs/ai-production/15-UI-DESIGN-PIPELINE.md`: nessun numero magico
  sparso).
- **Cifra del layout V3**: "niente pannelli, elementi flottanti con contorno". Il testo
  poggia direttamente sulla scena con un contorno nero di 1 px scalato; le cornici 9-patch
  restano solo dove delimitano una CASELLA (slot attivo, slot Innesto, card di scoperta),
  che è informazione di stato e non decoro.
- **Componenti**: i quattro asset di `assets/art/ui` — font da 5 px, atlas icone 16×16,
  cornice a pannello (`slice [6,6,6,6]`, con rivetti), cornice a slot (`slice [4,4,4,4]`) —
  vestono l'HUD **e tutte le nove schermate**, non solo l'HUD.
- **Cuori**: icone `heart`/`heart_half`/`heart_empty`, 12 px con passo 13, stessa semantica
  di prima (2 punti vita per cuore). `heart_temp` esiste nell'atlas ma non è disegnato: la
  salute temporanea di DEC-008 non ha ancora un contatore nel motore (lacuna di gameplay,
  non d'arte — `docs/engineering/known-issues.md` voce 10).
- **Risorse**: ordine fisso `ingot` → `charge` → `key` → `flux`, icona 11 px + numero. Il
  Flux compare solo quando se ne possiede almeno uno (regola già in vigore: una risorsa rara
  con uno "0" fisso sarebbe rumore) e porta il riquadro di evidenza quando basta per una
  fusione, come chiede questo documento.
- **Slot attivo e Innesto**: due caselle 30×30 in basso a sinistra, con barra di ricarica
  24×4 e l'etichetta del tasto (`[E]`/`[G]`) sotto. La barra si RIEMPIE mentre l'attesa
  scende, così "piena = pronto" vale sia per gli attivi a cariche sia per quelli a cooldown
  e il giocatore non deve ricordare quale dei due ha in mano. Una casella vuota resta
  disegnata (cornice spenta): `grafts.md` chiede che l'interfaccia mostri gli slot
  disponibili, non solo quelli pieni.
- **Timer di run (DEC-051)**: il layout V3 lo mette centrato in alto ma `Game` non ha un
  cronometro di run — **non disegnato**, lacuna di gameplay dichiarata in known-issues
  voce 10.
- **Visibilità invariata**: `HudCombatShouldDraw` (DEC-169) resta la regola unica, in un
  solo punto. Il vecchio HUD a quattro cluster in overlay (`DrawOuterUi`) sopravvive come
  **ripiego integrale** per il caso "`assets/art/ui` assente": i due percorsi si escludono
  a vicenda, mai mescolati nello stesso frame.

## Canvas di riferimento della demo (DEC-174)

L'HUD in pixel art della demo si disegna per il **canvas logico attuale, 960×640** — lo
stesso rettangolo su cui sono costruite le stanze multi-taglia e la telecamera a zoom
fisso di [Rooms and Floor Generation](../systems/rooms-and-floor-generation.md) (DEC-170).
Questo **non** fissa la risoluzione logica canonica dell'interfaccia: la domanda aperta 11
(proposta ricorrente 640×360 con scaling intero) **resta aperta**, si decide dopo la demo.
960×640 è il canvas su cui si lavora **oggi**, non un valore di design definitivo — stesso
trattamento dei default proposti stile DEC-019 già usati altrove in questo documento e in
`content/visual-language.md`.

Elementi indipendenti dalla risoluzione restano una buona pratica per i componenti a
9-patch (bordi/riempimento che si adattano a più dimensioni), ma non sono la via
principale scelta per l'HUD della demo: costruire subito componenti solo relativi
avrebbe rimandato la disegnazione concreta dell'HUD senza necessità, dato che il canvas
di lavoro (960×640) è già stabile per l'implementazione M2 in corso.

## Priorità visiva

1. sopravvivenza (salute base e temporanea);
2. minacce e cooldown;
3. risorse spendibili (valuta, breccia, apertura, catalizzatore di fusione);
4. blocco statistiche correnti (danno, cadenza, velocità del colpo, velocità di movimento, raggio, Fortuna) — consultabile, non decisionale immediata come le prime tre (DEC-184);
5. progressione della run (piano e stanza);
6. informazioni competitive.

## Non-obiettivi

- Non mostra formule interne, dettagli tecnici o prompt dell'IA (fonte unica: `06-ai-content-generation-model.md`).
- Non sostituisce `BuildScreen` per la spiegazione delle sinergie.
- La card di scoperta non sostituisce la scheda completa del Catalogo (DEC-045, vedi
  `systems/save-and-meta-progression.md`).

## Domande aperte residue

- ~~Casi limite della coda delle card di scoperta~~: risolti in due decisioni distinte.
  DEC-131 copre **cap e overflow** — coda limitata (~5, valore esatto da playtest); quando
  trabocca le più vecchie escono senza essere mostrate. DEC-152 copre il caso separato di
  **morte o cambio stanza** con card ancora in attesa — si scartano silenziosamente. In
  entrambi i casi la scoperta resta comunque registrata nel Catalogo.

## Scenari verificabili

1. **Given** il giocatore ha sia salute base sia salute temporanea, **when** subisce danno, **then** la salute temporanea si riduce per prima e resta visivamente distinta da quella base.
2. **Given** il giocatore possiede catalizzatore di fusione sufficiente per una fusione, **when** osserva l'HUD, **then** l'indicatore del catalizzatore appare evidenziato rispetto allo stato "insufficiente".
3. **Given** il giocatore raccoglie un oggetto raro che aggiunge uno slot Innesto, **when** l'HUD si aggiorna, **then** compare un secondo slot Innesto vuoto.
4. **Given** una modalità competitiva è attiva, **when** il giocatore gioca in `Gameplay`, **then** l'HUD mostra lo stato competitivo essenziale senza rivelare informazioni tecniche della generazione.
5. **Given** un giocatore in `Gameplay` in qualunque modalità, **when** osserva l'HUD, **then** il timer di run è sempre visibile, indipendentemente dalla modalità (DEC-051).
6. **Given** un giocatore incontra per la prima volta un nemico generato mai visto, **when** lo affronta, **then** l'HUD mostra una card di scoperta breve con sprite, nome e una riga, senza mettere in pausa né bloccare l'input (DEC-065).
7. **Given** più contenuti mai visti compaiono nella stessa stanza, **when** il giocatore li incontra quasi contemporaneamente, **then** le card di scoperta si accodano e vengono mostrate una alla volta, senza sovrapporsi sullo schermo (DEC-065).
8. **Given** il giocatore ha card di scoperta ancora in coda non mostrate, **when** muore oppure cambia stanza, **then** quelle card vengono scartate silenziosamente senza inseguirlo nella stanza successiva, e la scoperta resta comunque registrata nel Catalogo (DEC-152).
9. **Given** il giocatore è in `FloorZero`, **when** esplora l'hub senza aprire la pausa e senza entrare in una prova, **then** l'HUD di combattimento resta nascosto; **when** apre il menu di pausa, **then** può consultare salute, risorse e build; **when** entra in una prova, **then** l'HUD ricompare (DEC-169).
10. **Given** l'HUD della demo è disegnato in pixel art, **when** viene posizionato sullo schermo, **then** usa come riferimento il canvas logico 960×640 in uso oggi, senza attendere la risposta alla domanda aperta 11 sulla risoluzione logica definitiva (DEC-174).
11. **Given** il giocatore è in `Gameplay` con il blocco statistiche visibile (default), **when** la build cambia (es. raccoglie un oggetto che aumenta il danno), **then** i valori del blocco si aggiornano in tempo reale, coerenti con quelli del pannello "Statistiche principali" di `BuildScreen`; **when** preme il tasto di toggle, **then** il blocco si nasconde senza alcun effetto sulla simulazione (DEC-184).
