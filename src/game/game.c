#include "game/game.h"

#include "content/run_content.h"
#include "game/game_internal.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

void GameSetMessage(Game *game, const char *message)
{
    snprintf(game->message, sizeof(game->message), "%s", message);
    game->messageTimer = 3.2f;
}

void GameResetRun(Game *game)
{
    GameUnloadAssets(game);
    memset(game, 0, sizeof(*game));
    game->rng = (unsigned int)time(NULL) ^ 0x514AACu;
    RunContentLoad(&game->content, game->rng);
    AssetsLoad(game);
    game->phase = PHASE_PLAY;
    game->player.radius = 14.0f;
    game->player.speed = 224.0f;
    game->player.maxHp = 6;
    game->player.hp = 6;
    game->player.coins = 3;
    game->player.bombs = 2;
    game->player.keys = 1;
    game->player.damage = 8.0f;
    game->player.fireDelay = 0.23f;
    game->player.shotSpeed = 520.0f;
    game->player.shotRadius = 5.0f;
    WorldStartFloor(game, 1);
}

void GameUpdate(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame)
{
    if (dt > 0.033f) dt = 0.033f;
    if (IsKeyPressed(KEY_R)) GameResetRun(game);
    if (game->messageTimer > 0.0f) game->messageTimer -= dt;

    if (game->phase == PHASE_GAME_OVER || game->phase == PHASE_WIN)
    {
        GameUpdateParticles(game, dt);
        return;
    }

    CombatUpdatePlayer(game, dt, mouseGame, mouseInsideGame);
    CombatUpdateEnemies(game, dt);
    CombatUpdateShots(game, dt);
    CombatUpdateBombs(game, dt);
    CombatUpdatePickups(game);
    GameUpdateParticles(game, dt);
    WorldCheckRoomClear(game);
}
