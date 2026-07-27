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

/* DEC-170, telecamera. 'WorldCameraFocusRect' e' il rettangolo che la
   telecamera non deve mai sforare: il riquadro della stanza per le taglie
   rettangolari, la CELLA CORRENTE per le forme a L (l'unico modo di non
   mostrare mai l'angolo mancante senza inventare uno zoom dinamico, che
   DEC-170 vieta). */
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

#endif
