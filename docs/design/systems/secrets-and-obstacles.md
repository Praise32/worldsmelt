---
id: gd-system-secrets-obstacles
title: Secrets and Obstacles
domain: design
status: approved
authority: canonical
owner: design
summary: "Ostacoli distruttibili/attraversabili e uso dello strumento di breccia (DEC-013, funzione che sostituisce le bombe). Ostacoli generati a tema, con croce centrale libera e telegraph leggibili, a budget di difficoltà condiviso con i nemici (DEC-043). Stanze segrete a due livelli — normali e super-segrete (DEC-025) — implementate nel motore dal WP8 (30/07): varco murato apribile solo con lo strumento di breccia, indizio a parete per il livello normale, nessun indizio per la super-segreta."
last_reviewed: 2026-07-30
last_verified_commit: a8a85bf
topics: [ostacoli, segreti, strumento-di-breccia, telegraph, budget-difficoltà, DEC-043, WP3, ostacoli-distruttibili, pericoli-passivi, WP8, DEC-025, DEC-127, stanze-segrete, super-segrete, ROOM_SECRET]
related: []
supersedes: []
source_files: [src/core/room_layout.h, src/core/room_layout.c, src/core/game_types.h, src/world/world.c, src/gameplay/combat.c, src/render/game_renderer.c]
---

# Secrets and Obstacles

## Intento per il giocatore

Gli ostacoli devono creare decisioni tattiche immediate (aprire un varco o no, quando e con cosa); i segreti devono premiare l'osservazione (stanze segrete "normali") o la preparazione/intuizione (stanze "super-segrete"), senza dipendere solo da tentativi casuali (DEC-025).

## Condizioni di ingresso

Ostacoli e passaggi segreti compaiono all'interno di qualunque stanza generata o curata del piano (vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md)), inclusa la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md)).

## Ostacoli

Gli ostacoli possono bloccare movimento, linea di tiro, ricompense o percorsi. Devono avere regole coerenti di distruzione o attraversamento.

## Ostacoli generati a tema (DEC-043)

Gli ostacoli ambientali sono un tipo di contenuto **generato dal tema** della run: forma e
comportamento semplice, sempre dentro le garanzie di giocabilità:

- **croce centrale libera**: la stanza mantiene sempre un corridoio centrale a croce libero
  da ostacoli, così il movimento e la linea di tiro principali restano garantiti anche nella
  stanza più densa di ostacoli;
- **telegraph leggibili**: ogni ostacolo che rappresenta un pericolo (non solo un blocco
  passivo) segnala il proprio comportamento in anticipo, secondo il budget di leggibilità di
  [Combat and Projectiles](./combat-and-projectiles.md), non riformulato qui.

Gli ostacoli generati a tema comprendono due famiglie:

- **blocchi**: ostacoli passivi che limitano movimento, linea di tiro o accesso, senza
  infliggere danno diretto;
- **pericoli passivi telegrafati**: ostacoli che infliggono danno o effetti se il giocatore
  li tocca o li attraversa senza attenzione, sempre segnalati in anticipo (telegraph).

La **degenerazione del tema** (DEC-024) rende gli ostacoli più insidiosi nei piani alti:
più densi, più aggressivi nei pericoli passivi, ma sempre dentro la croce centrale libera e
i telegraph leggibili. Il **budget di difficoltà della stanza** copre insieme ostacoli e
nemici: spendere budget in ostacoli riduce quanto resta disponibile per i nemici della
stessa stanza, e viceversa (fonte del principio di budget condiviso:
[Rooms and Floor Generation](./rooms-and-floor-generation.md), non riformulato qui).

## Input/azioni

Il giocatore usa lo **strumento di breccia** — che **è la bomba** (DEC-128; in-game: **Blast Charges**, DEC-013/DEC-072), universale: un uso = un'esplosione, e la variabilità sta solo in che cosa è colpito e che cosa no, sempre dichiarato prima dell'uso — per distruggere ostacoli, muri deboli o aprire varchi. Le regole della risorsa stessa (come si ottiene, cap massimo, ordine di consumo, rarità e fonti) sono definite in [health-and-resources.md](./health-and-resources.md) come fonte unica; questo documento descrive solo il suo **uso** nel contesto di ostacoli e segreti, senza ripetere quelle regole.

## Risultato

- Uso riuscito dello strumento di breccia su un ostacolo compatibile: l'ostacolo viene rimosso, aprendo un percorso, una scorciatoia o una ricompensa.
- Uso su un bersaglio incompatibile o senza scorte disponibili: nessun effetto; dove possibile, va segnalato chiaramente prima della conferma per evitare di sprecare la risorsa.

## Feedback

- indicazione visiva che un ostacolo è distruttibile/attraversabile con lo strumento di breccia;
- conferma visiva della distruzione e di cosa si apre;
- per i segreti: indizio ambientale deducibile (grammatica), non puramente casuale.

## Segreti (DEC-025, due livelli)

Le stanze segrete si dividono in due livelli, entrambi deducibili tramite una grammatica
diversa, mai da tentativi puramente casuali:

- **Stanze segrete "normali"**: hanno indizi visivi leggibili (crepe, anomalie del tema)
  che il giocatore può osservare e interpretare; si aprono con lo **strumento di breccia**
  (vedi sotto), come qualunque altro ostacolo distruttibile compatibile.
- **Stanze "super-segrete"**: non hanno alcun indizio visivo leggibile. Si trovano solo
  con i rivelatori — Innesti «sensore» dedicati o oggetti rari che offrono la rivelazione
  come effetto secondario (DEC-127) — oppure per intuizione estrema del giocatore
  (esplorazione sistematica senza alcun aiuto di sistema).

Questo risolve la domanda di design precedentemente aperta sul metodo di scoperta delle
stanze segrete.

### Stato di implementazione: i due livelli nel motore (WP8, 2026-07-30)

Entrambi i livelli di DEC-025 esistono nel motore come un `RoomKind` dedicato
(`ROOM_SECRET`, `src/core/game_types.h`) con un campo di livello
(`RoomState.secretSuper`). L'archetipo come TIPO DI STANZA è descritto in
[special-rooms.md](./special-rooms.md), "Stato di implementazione: la stanza segreta";
qui si registra solo ciò che riguarda **i segreti e lo strumento di breccia**.

- **Il varco è murato, non è una porta.** In generazione `WorldLinkRooms`
  (`src/world/world.c`) NON apre mai la porta di una segreta ancora sigillata: la stanza
  esiste sulla griglia ma è fuori dal grafo di adiacenza del piano, che resta quindi
  completabile ignorandola del tutto. La porta compare **solo** quando lo strumento di
  breccia esplode addosso alla parete condivisa (`WorldTryBreachSecretWall`), e da quel
  momento è una porta come tutte le altre, su entrambi i lati.
- **Deve essere la parete GIUSTA, e da vicino.** La breccia funziona solo se
  l'esplosione tocca la fascia di parete condivisa — larga quanto una porta, profonda
  `WORLD_SECRET_BREACH_DEPTH` dentro la cella (`WorldSecretWallRect`, `src/world/world.h`).
  Una bomba lasciata in mezzo alla stanza non apre nulla: è ciò che rende l'indizio
  **utile** invece che decorativo (Scenario 4).
- **Stessa garanzia di origine dei distruttibili.** Il varco si apre solo da
  un'esplosione di **origine giocatore**: `CombatExplodeAt` chiama la breccia sotto lo
  stesso parametro `breach` che governa i distruttibili (vedi "Default proposti
  dall'implementazione" sotto, "Origine delle esplosioni"). Nessuna esplosione nemica ha
  mai aperto e mai aprirà un segreto.
- **L'indizio (livello normale).** Sulla parete **lato stanza visibile**, esattamente dove
  il varco si può aprire, si disegna `assets/art/props/crepa-segreta` (tag `indizio`, 2
  fotogrammi; `aperta`, 1 fotogramma, dopo la breccia) — `DrawSecretWallHints`,
  `src/render/game_renderer.c`. Nessuna freccia, nessun testo, nessun lampeggio: "leggibile
  ma discreto". Se l'asset manca (checkout senza `assets/art/`, pacchetto parziale) si
  disegna comunque una forma geometrica di riserva: un indizio che sparisce col pacchetto
  artistico cambierebbe il **design** (renderebbe la segreta normale indistinguibile da una
  super-segreta), non solo la veste.
- **La super-segreta non ha MAI indizio** (Scenario 5). Il filtro è un predicato unico e
  puro, `WorldSecretClueVisible` (`src/world/world.h`), non una condizione sparsa nel
  renderer — così "la super-segreta non ha alcun indizio" è verificabile da un test senza
  aprire una finestra, ed è esattamente ciò che il controllo (s) di `GameRoomsTest` fa.
  Bombardare alla cieca il muro giusto la apre **lo stesso**: l'"intuizione estrema" di
  DEC-025 è una via completa, non un ripiego.
- **I rivelatori di DEC-127 non esistono ancora**: nessun oggetto o Innesto «sensore» è
  implementato, quindi oggi la super-segreta si trova SOLO per intuizione. È il limite
  dichiarato in `docs/engineering/known-issues.md`, voce 14 (con la motivazione della
  scelta e il punto di innesto già pronto lato mondo); nessuna garanzia di design è violata
  — il documento prevede esplicitamente questo caso in "Casi limite".
- **Test**: `RoomsTestSecretRooms` e il controllo (s) di `GameRoomsTest`
  (`src/tests/game_tests.c`, `--rooms-test`/`make test`) — bomba lontana o di origine
  nemica: nessun varco; bomba sulla parete: varco su entrambi i lati; il varco resta aperto
  uscendo e rientrando; indizio presente solo per il livello normale.

## Interazioni

- con lo strumento di breccia e il suo costo (vedi [health-and-resources.md](./health-and-resources.md));
- con la stanza segreta come archetipo speciale (vedi [special-rooms.md](./special-rooms.md));
- con la generazione del piano, che colloca ostacoli e segreti nella griglia e condivide il budget di difficoltà tra ostacoli e nemici (DEC-043, vedi [rooms-and-floor-generation.md](./rooms-and-floor-generation.md));
- con l'escalation leggibile del tema, che rende gli ostacoli più insidiosi nei piani alti (DEC-024, vedi [Difficulty and Progression](../07-difficulty-and-progression.md));
- con il budget di leggibilità dei telegraph (vedi [combat-and-projectiles.md](./combat-and-projectiles.md)).

## Regole per contenuti generati

L'IA può variare forma e presentazione di ostacoli e indizi, ma non deve nascondere completamente gli indizi previsti dal sistema per le stanze segrete "normali" (DEC-025): l'indizio (crepa, anomalia del tema) deve restare deducibile. Per le stanze "super-segrete" l'IA può variare liberamente la forma dell'oggetto/Innesto rivelatore, ma non deve introdurre indizi ambientali impliciti che le rendano scopribili senza quell'oggetto o senza intuizione estrema. Ogni ostacolo/segreto generato dichiara un'origine (curato | composto | variato | nuovo) e rispetta la grammatica di deducibilità richiesta per il proprio livello.

Per gli ostacoli generati a tema (DEC-043), l'IA inventa forma e comportamento semplice
dentro le bande di garanzia dichiarate, ma non può mai generare un ostacolo che occupi la
croce centrale libera della stanza né un pericolo passivo privo di telegraph leggibile: sono
garanzie di giocabilità non negoziabili, verificate secondo
[Generated Content Validation](./generated-content-validation.md).

## Casi limite

- Il giocatore esaurisce lo strumento di breccia davanti a un ostacolo necessario per il progresso principale (non un segreto opzionale): l'ostacolo non deve bloccare l'unico percorso critico della stanza.
- Un indizio generato risulta illeggibile in validazione: il contenuto va respinto o corretto prima di apparire nella run.
- Una stanza segreta generata resta irraggiungibile per un difetto di layout: va trattata come caso limite di generazione, non come segreto "voluto" (va respinta in validazione).
- Una stanza "super-segreta" generata in un piano dove nessun oggetto/Innesto rivelatore è ancora apparso nella run: la stanza resta comunque scopribile per intuizione estrema, e non deve essere l'unico modo di completare un requisito obbligatorio della run.
- Un ostacolo generato che finirebbe per occupare la croce centrale libera della stanza: va respinto o riposizionato prima di comparire nella run (DEC-043).
- Una stanza con budget di difficoltà quasi interamente speso in ostacoli: il budget residuo per i nemici va comunque rispettato, anche se minimo (DEC-043); non deve azzerarsi la presenza di nemici quando la tassonomia della stanza la richiede.

## Fallback

Vale la regola unica di [generated-content-validation.md](./generated-content-validation.md): ogni categoria di ostacolo/indizio ha un contenuto curato sufficiente a completare la run senza generazione nuova. Non ripetuta qui.

## Default proposti dall'implementazione

Voci aggiunte da WP3 (2026-07-30, stile DEC-019): il motore ora distingue tre **famiglie**
di ostacolo generato a tema — **solido** (il blocco di sempre, zero-default), **distruttibile**
(si comporta come solido finché lo strumento di breccia non lo rimuove) e **pericolo
passivo** (non blocca, danneggia al contatto, sempre telegrafato prima). Il documento fissa
il principio (due famiglie oltre al solido, budget di difficoltà condiviso, degenerazione del
tema più aggressiva nei piani alti) ma non i numeri: quelli restano da confermare al
playtest, vedi `governance/open-questions.md` voce 29.

- **Proporzione fra famiglie:** probabilità **piatta** (non scalata col piano) del 35% che un
  blocco generato diventi **distruttibile**; probabilità di **pericolo** che parte all'8% al
  piano 1 e cresce del 5% per piano (DEC-024, degenerazione del tema), fino a un tetto del
  40%; il resto resta **solido**. La scelta è testata **prima per il pericolo**, così le due
  percentuali non si accavallano mai. Deterministica dal seme della cella (stessa cella,
  stesso piano ⇒ stessa famiglia ad ogni rientro), mai da un flusso condiviso col resto del
  gioco.
- **Costo nel budget nemici (DEC-043):** ogni ostacolo della stanza — di qualunque famiglia,
  le celle-buco di una forma a L escluse — sottrae 0.18 punti al budget nemici della stanza;
  il budget non scende mai sotto 0.35 punti residui (la stessa soglia minima di costo di un
  singolo nemico), così la stanza ha sempre almeno un nemico anche quando è fittissima di
  ostacoli ("Casi limite" sopra).
- **Danno di contatto di un pericolo:** 1 punto, dentro gli i-frames esistenti del
  giocatore (stessa finestra di invulnerabilità di ogni altro danno). I nemici lo
  **ignorano** del tutto — l'interpretazione scelta fra le due ammesse più sopra
  ("ignorano o evitano"): il motore non ha ancora un'euristica di pathing per "evitare" un
  ostacolo.
- **Telegraph dei pericoli:** nessun windup a tempo — il pericolo è disegnato con un segnale
  distinto (forma a bande, leggibile senza colore, DEC-058) fin dal primo frame in cui esiste
  nella stanza, quindi sempre prima di qualunque contatto possibile.
- **Veste visiva delle due famiglie non-solide (WP-INT, 30/07):** con `props/spuntoni` e
  `props/cassa` agganciati al motore, il pericolo passivo si disegna **SEMPRE** col tag
  "estesi", mai "retratti" — il danno di contatto è costante per tutta la vita del pericolo
  (`CombatResolveHazards` non ha alcun gate temporale), quindi mostrare "retratti" anche solo
  a intermittenza avrebbe promesso una finestra di sicurezza che il motore non offre mai (una
  prima versione alternava i due tag nel tempo ed è stata bocciata proprio per questo). Il
  tag "retratti" resta consegnato nell'asset ma inutilizzato, riservato a una futura variante
  di pericolo davvero temporizzata (oggi assente). Il vero telegraph resta SEMPRE la
  sovrapposizione a bande sopra, disegnata incondizionatamente. Il distruttibile usa
  `props/cassa` (non `props/vaso`, anch'esso consegnato): un contenitore di legno si legge
  come "distruttibile" in ogni ambientazione del gioco senza dipendere dal tema. I due prop
  RIEMPIONO il rettangolo dell'ostacolo ripetendo il fotogramma (stessa disciplina di
  `DrawTiledArea`, che è cosa sostituiscono), non si ancorano più come uno sprite isolato: i
  blocchi di `RoomLayoutBuild` sono spesso molto più larghi che alti (forme CORRIDOR/ARENA),
  e un singolo sprite scalato dalla sola larghezza sforerebbe il rettangolo sull'asse
  verticale. Degrado invariato quando l'asset manca: si ricade sul tile del piano o sul
  blocco 2.5D di sempre, con la stessa sovrapposizione a bande/crepa sopra. Da confermare al
  playtest (`DrawObstacleFamilyProp`, `src/render/game_renderer.c`; `governance/open-questions.md`
  punto 35).
- **Persistenza dei distruttibili (infrastruttura, non ancora osservabile in gioco):** il
  motore registra per cella/piano quali distruttibili sono stati spaccati e non li
  ricostruisce a un ingresso successivo nella stessa cella; si azzera al piano successivo (i
  piani non si riattraversano in questa fase, DEC-183 registra lo stesso principio per gli
  Innesti sganciati). **Limite noto (revisione WP3, 30/07)**: nel motore attuale una stanza
  di combattimento perde TUTTI i suoi ostacoli, di qualunque famiglia, non appena si ripulisce
  (comportamento del motore preesistente a questo lavoro, non toccato qui), e la porta resta
  bloccata finché non si ripulisce: non esiste quindi ancora, nel gioco vero, una sequenza
  "esco e rientro in una stanza di combattimento ancora aperta" in cui osservare questa
  persistenza. Il meccanismo è comunque implementato e testato come infrastruttura, in vista
  delle stanze segrete di un lavoro successivo (che potranno rientrare più volte prima di
  essere "ripulite" in quel senso) — vedi `docs/engineering/known-issues.md`, voce 11.
- **Origine delle esplosioni che sbrecciano un distruttibile:** nel motore, `CombatExplodeAt`
  è la stessa funzione dietro la bomba (lo strumento di breccia in senso stretto, DEC-128) E
  dietro le altre esplosioni di ORIGINE giocatore già esistenti (un colpo con il trait
  Esplosivo, un attivo con trait Esplosivo/Gigante): tutte aprono i distruttibili nel raggio,
  non solo la bomba in senso stretto — un parametro esplicito (`breach`) tiene comunque la
  garanzia che un'esplosione di origine NEMICA (oggi nessuna: nessun colpo nemico porta il
  trait Esplosivo) non apra mai un varco. Default proposto, non canone: questo documento
  descrive solo la bomba come strumento di breccia (DEC-128); estenderlo a "qualunque
  esplosione di origine giocatore" è una scelta di implementazione, non ancora una decisione
  di design.

Voci aggiunte da WP8 (2026-07-30, stesso stile) per le **stanze segrete**. Il documento
fissa i due livelli, la grammatica di scoperta e lo strumento (la bomba); non fissa nessun
numero né la geometria della breccia — quelli sono tutti scelte dell'implementazione, da
confermare al playtest (`governance/open-questions.md`, voci 44-46).

- **Quanto deve essere vicina la bomba:** la fascia sbrecciabile è larga esattamente
  quanto una porta (`DOOR_HALF*2`, centrata sul lato della cella — cioè dove la porta
  comparirà davvero) e profonda **56 px** dentro la stanza
  (`WORLD_SECRET_BREACH_DEPTH`, `src/world/world.h`). Col raggio di esplosione attuale
  (74 px) significa: **addosso al muro giusto, non "verso quella parete"**. Alternativa
  scartata: accettare qualunque esplosione dentro la stanza — avrebbe reso l'indizio
  decorativo e la super-segreta banale (bastava una bomba a caso in ogni stanza).
- **Frequenza dei due livelli:** la segreta **normale** si tenta a ogni piano dal piano 1
  (`WORLD_SECRET_ROOM_MIN_FLOOR`); la **super-segreta** dal piano 2 e solo a estrazione
  concessa (`WORLD_SECRET_SUPER_MIN_FLOOR`, `WORLD_SECRET_SUPER_CHANCE_PERCENT` = 50%).
  Misura su 120 piani generati (`--rooms-test`): normale in **36 casi su 120**,
  super-segreta in **13 su 96** candidati — la super resta il livello più raro, come
  DEC-025 richiede, e il test lo verifica confrontando i due conteggi invece di fidarsi
  della sola estrazione.
- **Ricompensa "degna del segreto":** un oggetto dal pool del piano con la **rarità
  minima alzata** — il migliore dei tre candidati, stessa tecnica dell'arena di sfida e
  senza alcuna estrazione, così uscire e rientrare ritrova sempre lo **stesso** oggetto
  (il "contenuto assegnato una sola volta" senza consumarlo al primo ingresso, che lo
  farebbe perdere a chi esce senza raccoglierlo). Più la valuta di "stanza segreta
  trovata" (DEC-167, `WORLD_ROOM_CURRENCY_SECRET` = 6 Ingots). La **super-segreta**
  aggiunge un **catalizzatore di fusione** (`WORLD_SECRET_SUPER_FLUX` = 1), versato
  direttamente al primo ingresso e non come pickup a terra: un pickup sarebbe stato
  raccoglibile all'infinito rientrando, o perdibile uscendo senza prenderlo. È questa la
  "ricompensa superiore" del livello 2, non un numero di Ingots più grande — vedi
  [rewards-and-economy.md](./rewards-and-economy.md).

(`OBSTACLE_DESTRUCTIBLE_CHANCE`, `OBSTACLE_HAZARD_CHANCE_BASE/PER_FLOOR/MAX`,
`WORLD_OBSTACLE_ENEMY_BUDGET_COST/FLOOR` in `src/world/world.c`; `ObstacleFamily` in
`src/core/room_layout.h`; `CombatResolveHazards`/`CombatExplodeAt` in
`src/gameplay/combat.c`. WP8: `WORLD_SECRET_BREACH_DEPTH`,
`WORLD_SECRET_ROOM_MIN_FLOOR`, `WORLD_SECRET_SUPER_MIN_FLOOR`,
`WORLD_SECRET_SUPER_CHANCE_PERCENT`, `WORLD_ROOM_CURRENCY_SECRET`,
`WORLD_SECRET_SUPER_FLUX`, `WorldSecretWallRect`, `WorldRoomHiddenOnMap`,
`WorldSecretClueVisible` in `src/world/world.h`; `WorldPlaceSecretRoom`,
`WorldTryBreachSecretWall` in `src/world/world.c`; `DrawSecretWallHints` in
`src/render/game_renderer.c`.)

## Non-obiettivi

- Non definisce le regole generali dello strumento di breccia come risorsa (come si ottiene, cap, ecc.): vedi [health-and-resources.md](./health-and-resources.md).
- Non definisce il tasso esatto di comparsa degli oggetti/Innesti rivelatori delle stanze super-segrete (vedi Domande aperte residue).
- Non dettaglia l'archetipo di stanza segreta come tipo di stanza (vedi [special-rooms.md](./special-rooms.md)).

## Domande aperte residue

- ~~Come esistono i rivelatori delle super-segrete~~: risolto da DEC-127 — Innesti
  «sensore» dedicati + oggetti rari con la rivelazione come effetto secondario, con almeno
  un'occasione realistica per run; restano da playtest solo i tassi esatti. **Nota di
  implementazione (WP8, 30/07)**: la decisione resta valida e non è riaperta, ma nel motore
  nessun rivelatore esiste ancora — la super-segreta è oggi trovabile solo per intuizione
  estrema, l'altra via che DEC-025 ammette. Limite dichiarato in
  `docs/engineering/known-issues.md`, voce 14.
- Quanto deve essere vicina l'esplosione alla parete perché il varco si apra, e con quale
  frequenza compaiono i due livelli: **default proposti dall'implementazione** (WP8,
  sezione sopra), non canone — vedi `governance/open-questions.md` voci 44 e 45.
- ~~Un solo tipo di bersaglio o categorie a costi diversi~~: risolto da DEC-128 — lo
  strumento di breccia è la bomba (Blast Charges), universale: un uso = un'esplosione, la
  variabilità sta solo in che cosa è breccia-bile e che cosa no.
- Proporzione esatta tra blocchi e pericoli passivi telegrafati nella generazione a tema, e
  valori numerici del budget di difficoltà condiviso tra ostacoli e nemici (DEC-043 fissa il
  principio, non i numeri): **default proposti dall'implementazione** (WP3, sezione sopra),
  non canone — vedi `governance/open-questions.md` voce 29.

## Scenari

### Scenario 1 — Uso riuscito dello strumento di breccia

Given un giocatore con almeno una scorta di strumento di breccia
When lo usa su un ostacolo distruttibile compatibile
Then l'ostacolo viene rimosso, il percorso o la ricompensa dietro di esso diventa accessibile, e la scorta si riduce secondo le regole di [health-and-resources.md](./health-and-resources.md)

### Scenario 2 — Scorte esaurite davanti a un ostacolo critico

Given un giocatore senza scorte di strumento di breccia
When incontra un ostacolo che blocca l'unico percorso verso l'uscita della stanza
Then la generazione deve garantire un percorso alternativo, perché l'ostacolo non può essere l'unico blocco critico

### Scenario 3 — Indizio di segreto illeggibile

Given un indizio ambientale generato per una stanza segreta
When la validazione lo controlla per leggibilità
Then un indizio non deducibile viene respinto o corretto prima di poter apparire nella run

### Scenario 4 — Stanza segreta "normale" con indizio leggibile

Given un giocatore che esplora un piano e nota una crepa o un'anomalia del tema in una parete
When usa lo strumento di breccia sull'indizio
Then la stanza segreta "normale" si apre, coerente con la grammatica deducibile di DEC-025

### Scenario 5 — Stanza "super-segreta" senza indizi

Given un giocatore in possesso di un oggetto o Innesto rivelatore
When esplora un'area priva di qualunque indizio visivo
Then può individuare una stanza "super-segreta" solo grazie all'oggetto/Innesto rivelatore o a un'intuizione estrema, senza alcun aiuto ambientale

### Scenario 6 — Ostacolo generato a tema rispetta la croce centrale libera

Given una stanza generata con ostacoli a tema per un piano avanzato (DEC-043)
When la validazione controlla il layout della stanza
Then il corridoio centrale a croce resta libero da ostacoli, indipendentemente da quanto il tema si sia intensificato

### Scenario 7 — Pericolo passivo telegrafato nei piani alti

Given una stanza di un piano alto con un pericolo passivo generato a tema, reso più insidioso dalla degenerazione (DEC-024)
When il giocatore vi si avvicina
Then il pericolo mostra sempre un telegraph leggibile prima di poter infliggere danno, coerente col budget di leggibilità di [Combat and Projectiles](./combat-and-projectiles.md)
