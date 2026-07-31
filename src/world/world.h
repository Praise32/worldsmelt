#ifndef MELTING_RUN_WORLD_H
#define MELTING_RUN_WORLD_H

#include "core/game_types.h"

const char *GameRoomKindName(RoomKind kind);
const RoomState *GameCurrentRoom(const Game *game);
bool GameRoomIsLocked(const Game *game);

/* DEC-170 (stanze multi-cella). La cella (cx,cy) della griglia appartiene a una
   stanza che puo' occuparne fino a quattro: questi due accessori danno la cella
   di STATO di quella stanza (kind/visited/cleared/rewardTaken, vedi
   l'invariante su RoomState in core/game_types.h). Sono l'UNICO modo corretto
   di leggere "che stanza c'e' qui" -- game->rooms[y][x] direttamente risponde
   solo per exists/doors[]/origin/cells. Fuori griglia, o su una cella vuota,
   tornano la cella stessa (mai NULL: nessun chiamante deve difendersi). */
const RoomState *WorldRoomAt(const Game *game, int cx, int cy);
RoomState *WorldRoomAtMutable(Game *game, int cx, int cy);
/* Vero se le due celle appartengono alla STESSA stanza (entrambe esistenti). */
bool WorldSameRoom(const Game *game, int ax, int ay, int bx, int by);
/* La classe di taglia di una maschera di celle (DEC-170). */
RoomSize WorldRoomSizeFromCells(unsigned char cells);
/* Quante celle occupa la stanza della cella (cx,cy). */
int WorldRoomCellCount(const Game *game, int cx, int cy);

/* Il rettangolo di gioco della stanza che possiede la cella (cx,cy): il
   RIQUADRO (bounding box) delle sue celle, in coordinate LOCALI alla stanza --
   parte sempre da (ROOM_X, ROOM_Y) e misura un multiplo di ROOM_W x ROOM_H.
   Per una forma a L il riquadro contiene anche la cella mancante: quella e'
   solida (vedi Game.obstacleHoleCount), non giocabile. */
Rectangle WorldRoomRect(const Game *game, int cx, int cy);
/* Comodo per il caso di gran lunga piu' frequente: la stanza dove si trova
   davvero il giocatore adesso (game->roomX/roomY). */
Rectangle WorldCurrentRoomRect(const Game *game);
/* Il rettangolo della SINGOLA cella (cx,cy), nelle stesse coordinate locali. */
Rectangle WorldCellRect(const Game *game, int cx, int cy);
/* Il centro "buono" della stanza corrente per piazzarci qualcosa: il baricentro
   delle celle occupate, non il centro del riquadro -- su una forma a L il
   centro del riquadro cade sull'angolo mancante (dentro il muro), il baricentro
   no, per costruzione. Coincide col centro della cella per una 1x1. */
Vector2 WorldRoomCenter(const Game *game);
/* Le celle OCCUPATE della stanza corrente, in coordinate di griglia: torna
   quante ne ha scritte (1..4, mai 0). */
int WorldRoomCells(const Game *game, int *outX, int *outY, int maxOut);
/* Le celle del riquadro che NON appartengono alla stanza corrente: zero per
   tutte le taglie rettangolari, esattamente una per una forma a L. Sono muro
   pieno -- il gioco le tratta come ostacoli (Game.obstacleHoleCount) e il
   renderer le disegna come parete. */
int WorldRoomHoleCount(const Game *game);
Rectangle WorldRoomHoleRect(const Game *game, int index);
/* La cella della griglia in cui si trova il giocatore adesso (una stanza
   multi-cella ne ha piu' d'una; cambiarla NON cambia stanza). Se il giocatore
   e' fuori dalle celle occupate (forma a L: dentro il buco, un frame prima che
   la collisione lo respinga) torna la piu' vicina. */
void WorldPlayerCell(const Game *game, int *cx, int *cy);
/* Riporta 'pos' dentro il riquadro della stanza corrente, a distanza 'radius'
   dai bordi. Le celle-buco di una forma a L sono ostacoli solidi
   (CombatResolveObstacles), non un caso a parte di questo clamp. */
void WorldClampToRoom(const Game *game, Vector2 *pos, float radius);

/* DEC-170/DEC-180, telecamera. 'WorldCameraFocusRect' e' il rettangolo che la
   telecamera non deve mai sforare: sempre il riquadro dell'INTERA stanza,
   incluse le forme a L (DEC-180, 30/07: superato il default per-cella; il
   tileset veste l'angolo mancante da W8, quindi puo' entrare in inquadratura
   senza violare "mai area fuori dalla stanza"). */
Rectangle WorldCameraFocusRect(const Game *game);
Camera2D WorldGameCamera(const Game *game);
/* Il rettangolo di MONDO inquadrato adesso (utile a chi disegna in coordinate
   di mondo qualcosa che deve restare ancorato allo schermo). */
Rectangle WorldCameraView(const Game *game);
Vector2 WorldCanvasToWorld(const Game *game, Vector2 canvasPos);
void WorldSnapCamera(Game *game);
void WorldUpdateCamera(Game *game, float dt);

/* DEC-170: la grandezza minima garantita di DEC-009 e' ora la taglia 1x1,
   cioe' esattamente UNA cella. Restano due costanti pubbliche (invece di
   ROOM_W/ROOM_H nudi) perche' e' il MINIMO GARANTITO che i test verificano,
   non "la taglia del canvas": se un giorno la cella cambiasse, il significato
   di queste due resterebbe quello. */
#define WORLD_ROOM_MIN_W ((int)ROOM_W)
#define WORLD_ROOM_MIN_H ((int)ROOM_H)

/* WP5 (DEC-051, "stanza a tempo"): pubbliche (invece che locali a
   src/world/world.c) per lo stesso motivo delle due sopra -- il test dedicato
   (src/tests/game_tests.c, RoomsTestTimedRoomInteraction) verifica proprio
   "mai prima di questo piano" e "esattamente questa valuta se in tempo", e
   deve farlo confrontandosi con la fonte vera, non un numero duplicato a
   mano che potrebbe divergere in silenzio. Definite qui, usate da
   src/world/world.c (che include gia' questo header).
   WORLD_TIMED_ROOM_MIN_FLOOR: default proposto dall'implementazione (stile
   DEC-019) -- stesso confine gia' scelto per l'escalation del tileset e il
   passaggio dei boss a due fasi (ROOM_TILESET_DEGRADED_FROM_FLOOR,
   src/render/game_renderer.c; DEC-028/106; governance/open-questions.md,
   voce 23): allineare i tre assi su un solo confine e' l'ipotesi piu'
   leggibile per il giocatore. */
#define WORLD_TIMED_ROOM_MIN_FLOOR 3
#define WORLD_ROOM_CURRENCY_TIMED 6

/* WP6 (systems/special-rooms.md, "Arena di sfida"). Pubbliche per lo stesso
   motivo delle due sopra: il test dedicato (src/tests/game_tests.c,
   RoomsTestArenaInteraction e il controllo (q) di GameRoomsTest) verifica
   proprio "mai prima di questo piano", "esattamente questa valuta alla
   vittoria" e "budget davvero maggiorato", e deve confrontarsi con la fonte
   vera invece che con numeri duplicati a mano.
   Tutti e quattro sono DEFAULT PROPOSTI DALL'IMPLEMENTAZIONE (stile DEC-019):
   nessun documento fissa questi numeri, solo il principio "piu' impegnativa
   in cambio di una ricompensa maggiore" (special-rooms.md) e la proporzione
   rischio/ricompensa di rewards-and-economy.md.
   - MIN_FLOOR 2: il piano 1 insegna le basi (nessun trattamento speciale, ma
     e' il primo contatto col mondo generato); l'arena e' un'escalation
     volontaria e comincia appena dopo. Confine DIVERSO da quello della stanza
     a tempo (piano 3) di proposito: quello e' parte della decisione DEC-051
     ("esclusiva dei piani avanzati"), questo e' solo frequenza.
   - CURRENCY 8: il doppio di una stanza di combattimento (4), sotto il boss
     (12) -- "superiore alla media di una stanza di combattimento equivalente
     non a rischio" (rewards-and-economy.md, Scenario 2).
   - BUDGET_MULTIPLIER 1.5: il "+50%" sul budget nemici della stanza, applicato
     DOPO la scala per celle di DEC-170 e PRIMA della riduzione per ostacoli
     (DEC-043), cosi' resta un moltiplicatore della difficolta' della stanza,
     non un numero assoluto scollegato dal piano.
   - FLUX_DROP_PERCENT 50: DEC-022 dichiara le arene di sfida una delle TRE
     fonti del catalizzatore di fusione, e prima del WP6 ne esistevano solo
     due (drop di boss, acquisto in negozio). Piu' alto del boss (35%) perche'
     l'arena e' un rischio SCELTO, non una tappa obbligata. */
#define WORLD_ARENA_ROOM_MIN_FLOOR 2
#define WORLD_ROOM_CURRENCY_ARENA 8
#define WORLD_ARENA_BUDGET_MULTIPLIER 1.5f
#define WORLD_ARENA_FLUX_DROP_PERCENT 50

/* WP7 (systems/special-rooms.md, "Scambio ad alto rischio" -- Pourhouse,
   DEC-136). Pubbliche per lo stesso motivo delle precedenti: il test dedicato
   (src/tests/game_tests.c, RoomsTestPourhouseInteraction e il controllo (r) di
   GameRoomsTest) verifica "mai prima di questo piano" e "non ad ogni piano", e
   deve confrontarsi con la fonte vera invece che con numeri duplicati a mano.
   Entrambi sono DEFAULT PROPOSTI DALL'IMPLEMENTAZIONE (stile DEC-019): il
   documento fissa l'archetipo e le categorie della puntata, non la frequenza.
   - MIN_FLOOR 2: come l'arena, il piano 1 resta il primo contatto col mondo
     generato -- e una puntata proposta prima che il giocatore possieda
     qualcosa avrebbe quasi sempre come unica risposta "la colata e' fredda".
   - CHANCE_PERCENT 70: la Pourhouse e' un archetipo RARO, non un servizio di
     piano come tesoro/negozio -- il tentativo si fa solo quando l'estrazione
     del piano lo concede. La tiratura e' dell'RNG del piano, quindi
     deterministica dal seed di run. Il 70 e' la probabilita' del TENTATIVO,
     non del risultato: piazzandosi per ULTIMA su una griglia 5x5 gia' occupata
     da boss, arena e quattro speciali 1x1, la stanza trova posto in circa il
     44% dei casi anche quando il tentativo si fa sempre (misurato con
     l'estrazione forzata al 100%). Il risultato composto, misurato su 120
     piani generati (`--rooms-test`; solo i piani 2-5, 96 candidati): piazzata
     in 27 casi su 96, cioe' circa un piano candidato su quattro, ~73% delle
     run ne incontra almeno una. Numeri in questo ordine di grandezza sono
     l'intento: non ogni piano, ma nemmeno una rarita' che quasi nessuno vede. */
#define WORLD_POURHOUSE_ROOM_MIN_FLOOR 2
#define WORLD_POURHOUSE_ROOM_CHANCE_PERCENT 70

/* WP6: la conferma esplicita della sfida (Game.interactQueued, consumato da
   CombatUpdatePlayer). Vero SOLO se ha davvero fatto partire la sfida: stanza
   ROOM_ARENA, sfida non gia' accettata ne' superata, e giocatore a contatto
   col segnale (PICKUP_ARENA_ALTAR). Altrimenti falso e nessun effetto --
   premere il tasto ovunque altro non fa niente. */
bool WorldTryStartArenaChallenge(Game *game);

/* WP8 (systems/special-rooms.md, "Stanza segreta"; systems/secrets-and-obstacles.md,
   "Segreti", DEC-025). Pubbliche per lo stesso motivo di tutte le costanti
   sopra: il test dedicato (src/tests/game_tests.c, RoomsTestSecretRooms e il
   controllo (s) di GameRoomsTest) verifica "mai prima di questo piano", "non
   ad ogni piano per la super-segreta" ed "esattamente questa valuta quando la
   segreta e' trovata", e deve confrontarsi con la fonte vera invece che con
   numeri duplicati a mano.
   Tutti DEFAULT PROPOSTI DALL'IMPLEMENTAZIONE (stile DEC-019): i documenti
   fissano l'archetipo, i due livelli e la grammatica di scoperta, mai i
   numeri -- vedi governance/open-questions.md, voce 36.
   - SECRET_MIN_FLOOR 1: la segreta normale si tenta da SUBITO. E' l'archetipo
     che insegna a leggere il mondo (una crepa in una parete), e ha senso che
     il giocatore incontri quella lezione al primo piano.
   - SUPER_MIN_FLOOR 2 + SUPER_CHANCE_PERCENT 50: la super-segreta e' piu'
     rara della normale, come chiede DEC-025 ("solo con i rivelatori o per
     intuizione estrema") -- il piano 1 resta il primo contatto col mondo, e
     dal piano 2 in poi il tentativo si fa solo a estrazione concessa.
   - CURRENCY_SECRET 6: la valuta di "stanza segreta trovata" (DEC-167,
     rewards-and-economy.md la elenca esplicitamente fra le condizioni di
     completamento). Sopra tesoro (3) e negozio (2) -- trovarla e' costato uno
     strumento di breccia -- sotto l'arena (8), che chiede di combattere.
     UGUALE per i due livelli: la ricompensa SUPERIORE della super-segreta e'
     il catalizzatore di fusione garantito (WorldSpawnRoomReward), non un
     numero di Ingots piu' grande. */
#define WORLD_SECRET_ROOM_MIN_FLOOR 1
#define WORLD_SECRET_SUPER_MIN_FLOOR 2
#define WORLD_SECRET_SUPER_CHANCE_PERCENT 50
#define WORLD_ROOM_CURRENCY_SECRET 6
/* WP8: la RICOMPENSA SUPERIORE della super-segreta (DEC-025 chiede che il
   livello 2 paghi piu' del livello 1). Non piu' Ingots -- il catalizzatore di
   fusione, la risorsa piu' rara del gioco (DEC-022: oggi solo drop di boss al
   35%, arena al 50%, o un acquisto costoso in negozio). Versato DIRETTAMENTE
   sul giocatore al primo ingresso, non come pickup a terra: un pickup che
   ricompare finche' la stanza non e' "svuotata" sarebbe raccoglibile
   all'infinito uscendo e rientrando (la stanza segreta si puo' riattraversare
   quanto si vuole, a differenza di una stanza di combattimento che si
   ripulisce), e uno spawnato solo al primo ingresso andrebbe perso uscendo
   senza raccoglierlo. Player.flux non ha alcun cap (DEC-129), quindi la somma
   e' sempre valida. */
#define WORLD_SECRET_SUPER_FLUX 1

/* WP8: vero se la stanza della cella di STATO passata NON deve comparire sulla
   minimappa -- oggi solo una stanza segreta col varco ancora murato
   (special-rooms.md: "stanza non indicata direttamente sulla mappa"). Aperta
   la breccia torna falso e la stanza si comporta come ogni altra.
   Predicato PURO e senza raylib apposta: e' la stessa condizione che
   DrawMinimap applica e che il test verifica senza aprire una finestra. */
bool WorldRoomHiddenOnMap(const RoomState *room);

/* WP8: vero se questa stanza segreta deve mostrare l'INDIZIO visivo (la crepa)
   sulla parete condivisa -- segreta di livello NORMALE, varco ancora murato
   (DEC-025). Falso per la super-segreta (nessun indizio, mai) e per una
   segreta gia' aperta (li' si disegna la crepa "aperta", vedi il renderer).
   Puro come sopra, e per lo stesso motivo. */
bool WorldSecretClueVisible(const RoomState *room);

/* WP8: il rettangolo della PARETE CONDIVISA fra la cella (cx,cy) e la sua
   vicina in direzione 'dir' -- la fascia dove il varco murato di una stanza
   segreta si puo' sbrecciare, e dove il renderer ancora la crepa. Larga quanto
   una porta (DOOR_HALF*2) e profonda WORLD_SECRET_BREACH_DEPTH dentro la
   cella: bombardare in mezzo alla stanza non apre nulla, bisogna essere
   ADDOSSO alla parete giusta. Coordinate di mondo, come WorldCellRect. */
#define WORLD_SECRET_BREACH_DEPTH 56.0f
Rectangle WorldSecretWallRect(const Game *game, int cx, int cy, int dir);

/* WP8: lo strumento di breccia (DEC-128: la bomba, e ogni altra esplosione di
   ORIGINE GIOCATORE -- vedi il parametro 'breach' di CombatExplodeAt) tenta di
   aprire il varco murato di una stanza segreta adiacente alla stanza CORRENTE.
   L'esplosione (pos, radius) deve toccare la fascia di parete condivisa
   (WorldSecretWallRect sopra): lontano dalla parete non succede nulla, ed e'
   proprio cio' che rende l'indizio utile invece che decorativo.
   Vero se ha aperto almeno un varco (allora la porta e' aperta sui DUE lati e
   RoomState.secretOpened e' scritto, definitivo per tutto il piano). */
bool WorldTryBreachSecretWall(Game *game, Vector2 pos, float radius);

/* WP-SPIKE (DEC-198, secrets-and-obstacles.md "Default proposti
   dall'implementazione"): gli spuntoni (OBSTACLE_HAZARD) diventano
   TEMPORIZZATI -- cicli retratti/estesi, danno di contatto SOLO durante la
   fase estesa. Periodo fisso, default proposto non canone (registrato in
   governance/open-questions.md): 2.0s estesi + 1.2s retratti = 3.2s di
   periodo. La FASE (l'offset dentro il periodo) e' derivata dalla cella
   ASSOLUTA della griglia del piano che possiede il pericolo
   (Game.obstacleCellX/obstacleCellY, le stesse coordinate che WorldBuildObstacles
   gia' assegna a ogni ostacolo) E dall'indice LOCALE dentro la cella
   (Game.obstacleLocalIndex, fino a ROOM_LAYOUT_MAX_PER_CELL-1, gia' assegnato
   allo stesso ostacolo) tramite un hash a interi puro (nessuno stream RNG,
   nessun time()): celle diverse hanno fasi diverse, e SPUNTONI DIVERSI DENTRO
   LA STESSA CELLA hanno fasi diverse a loro volta (seconda revisione WP-SPIKE:
   la prima versione condivideva la fase per cella, il caso piu' frequente
   visto che oltre meta' delle stanze e' 1x1 -- corretto perche' "gli spuntoni
   non pulsano tutti insieme" valesse anche li'), cosi' gli spuntoni del piano
   non pulsano tutti insieme, e la stessa terna cella/indice locale allo
   stesso tempo di gioco produce sempre la stessa fase (determinismo dal seme
   di run tramite il piano, non serve altro). */
#define WORLD_HAZARD_EXTENDED_SECONDS 2.0f
#define WORLD_HAZARD_RETRACTED_SECONDS 1.2f
#define WORLD_HAZARD_PERIOD_SECONDS (WORLD_HAZARD_EXTENDED_SECONDS + WORLD_HAZARD_RETRACTED_SECONDS)

/* WP-SPIKE: l'UNICO predicato che decide se lo spuntone della cella
   (cellX,cellY), indice locale 'localIndex' dentro la cella, e' in fase
   ESTESA al tempo di gioco 'timeSeconds' -- usato SIA da CombatResolveHazards
   (il danno) SIA dal renderer (la scelta del tag "estesi"/"retratti" e del
   ripiego geometrico quando l'asset manca): la STESSA funzione, mai due
   calcoli paralleli, cosi' "quando si vede retratti il contatto non
   danneggia MAI" e' una garanzia strutturale, non una coincidenza fra due
   implementazioni che devono restare sincronizzate a mano (DEC-198, il
   difetto esplicito che questa decisione corregge).
   Puro: nessun accesso a Game, nessun raylib, verificabile da un test senza
   aprire una finestra. 'timeSeconds' e' Game.runElapsedSeconds (il timer di
   run gia' esistente, accumulato per dt SOLO durante PHASE_PLAY e
   inRealRun, mai time() ne' un contatore separato): stesso motivo per cui il
   ciclo si ferma in pausa insieme al resto della simulazione, invece di
   continuare a girare "dietro le quinte". */
bool WorldHazardSpikesExtendedAt(int cellX, int cellY, int localIndex, float timeSeconds);

#endif
