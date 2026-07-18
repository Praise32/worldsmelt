#ifndef MELTING_RUN_WORLD_H
#define MELTING_RUN_WORLD_H

#include "core/game_types.h"

const char *GameRoomKindName(RoomKind kind);
const RoomState *GameCurrentRoom(const Game *game);
bool GameRoomIsLocked(const Game *game);

/* M2 (DEC-009): l'UNICO modo corretto di leggere il rettangolo di gioco di
   una stanza (rx,ry) sono coordinate di griglia, non pixel). Centrato dentro
   il rettangolo massimo del canvas (ROOM_X/Y/W/H, mai una camera); ripiega su
   quel massimo se la stanza non ha una taglia impostata (w o h <= 0: il Piano
   0/hub, o un Game costruito a mano nei test), cosi' ogni punto che non sa
   nulla di M2 continua a vedere esattamente il rettangolo fisso di sempre. */
Rectangle WorldRoomRect(const Game *game, int rx, int ry);
/* Comodo per il caso di gran lunga piu' frequente: la stanza dove si trova
   davvero il giocatore adesso (game->roomX/roomY). */
Rectangle WorldCurrentRoomRect(const Game *game);

/* M2 (DEC-009, default PROPOSTO): grandezza minima garantita di una stanza,
   in pixel -- il valore piu' piccolo del lattice di taglie in world.c
   (WorldAssignRoomSizes). Duplicata qui come costante pubblica invece di
   esporre l'intero lattice (stesso spirito di TEST_ROOM_X in
   script_items_tests.c): se il lattice in world.c cambia, questi due valori
   vanno tenuti allineati a mano -- --rooms-test (src/tests/game_tests.c) si
   rompe subito altrimenti. */
#define WORLD_ROOM_MIN_W 556
#define WORLD_ROOM_MIN_H 298

#endif
