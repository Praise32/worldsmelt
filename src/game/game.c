#include "game/game.h"

#include "content/run_content.h"
#include "game/game_internal.h"
#include "script/script_items.h"

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
    game->player.hp = 6;
    game->player.coins = 3;
    game->player.bombs = 2;
    game->player.keys = 1;
    /* Valori di PARTENZA del sistema delle cache (spec, sezione 7): non
       vengono piu' assegnati direttamente ai campi "vivi" (damage,
       fireDelay, shotSpeed, shotRadius, speed, maxHp). ScriptItemsInit sotto
       li deriva chiamando ScriptItemsRecomputeStats con zero oggetti
       posseduti, che per costruzione produce esattamente questi stessi
       numeri (nessun cambiamento di comportamento per una run senza
       oggetti). */
    game->player.baseDamage = 8.0f;
    game->player.baseFireDelay = 0.23f;
    game->player.baseShotSpeed = 520.0f;
    game->player.baseShotRadius = 5.0f;
    game->player.baseSpeed = 224.0f;
    game->player.baseMaxHp = 6;
    /* Step C: la fortuna parte da zero (il memset sopra la azzera gia': la riga
       e' esplicita come le altre, perche' "da dove parte una statistica" si deve
       leggere qui e in nessun altro posto). */
    game->player.baseLuck = 0.0f;
    ScriptItemsInit(game);
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

    /* Rete di sicurezza del sistema delle cache (spec, sezione 7): consuma
       Game.statsDirty una volta per frame, PRIMA che CombatUpdatePlayer
       legga player.damage/fireDelay/shotSpeed/shotRadius/speed. In pratica
       CombatApplyItem la consuma gia' subito al momento del pickup (vedi
       combat.c): questa chiamata copre solo l'eventualita' che qualcos'altro
       in futuro sporchi la bandiera senza ricalcolare subito. */
    ScriptItemsProcessDirty(game);
    CombatUpdatePlayer(game, dt, mouseGame, mouseInsideGame);
    CombatUpdateEnemies(game, dt);
    CombatUpdateShots(game, dt);
    CombatUpdateBombs(game, dt);
    CombatUpdatePickups(game);
    GameUpdateParticles(game, dt);
    WorldCheckRoomClear(game);
}
