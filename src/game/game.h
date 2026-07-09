#ifndef MELTING_RUN_GAME_H
#define MELTING_RUN_GAME_H

#include "assets/game_assets.h"
#include "core/game_types.h"
#include "tests/game_tests.h"
#include "world/world.h"

void GameResetRun(Game *game);
void GameUpdate(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame);
void GameUpdateParticles(Game *game, float dt);

#endif
