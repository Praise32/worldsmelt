#ifndef MELTING_RUN_GAME_H
#define MELTING_RUN_GAME_H

#include "assets/game_assets.h"
#include "core/game_types.h"
#include "tests/game_tests.h"
#include "world/world.h"

void GameResetRun(Game *game);
/* DEC-141: come GameResetRun, ma il seed di gameplay ('rng') e il seed di
   generazione (passato a RunContentLoad) derivano ENTRAMBI da 'runSeed'
   (quello di RunSetup/RunBundle) invece che dall'orologio -- vedi il
   commento su Game.runSeed in core/game_types.h e sulla funzione in
   src/game/game.c. GameResetRun resta la wrapper storica (nessun seed di
   run disponibile: usata dall'avvio provvisorio e dai binari *Test), API
   invariata per ogni chiamante esistente. */
void GameResetRunWithSeed(Game *game, unsigned int runSeed);
void GameUpdate(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame);
void GameUpdateParticles(Game *game, float dt);

#endif
