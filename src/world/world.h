#ifndef MELTING_RUN_WORLD_H
#define MELTING_RUN_WORLD_H

#include "core/game_types.h"

const char *GameRoomKindName(RoomKind kind);
const RoomState *GameCurrentRoom(const Game *game);
bool GameRoomIsLocked(const Game *game);

#endif
