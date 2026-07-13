#include "tests/game_tests.h"

#include "game/game_internal.h"

#include <stdio.h>
#include <string.h>

bool GamePortalRespawnTest(Game *game)
{
    int bossX = -1;
    int bossY = -1;
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (game->rooms[y][x].kind == ROOM_BOSS)
            {
                bossX = x;
                bossY = y;
            }
        }
    }
    if (bossX < 0 || bossY < 0) return false;

    game->roomX = bossX;
    game->roomY = bossY;
    RoomState *room = WorldCurrentRoomMutable(game);
    room->visited = true;
    room->cleared = true;
    room->rewardTaken = true;

    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    return EntitiesCountActivePickups(game, PICKUP_EXIT) == 1;
}

static int CountActiveShots(const Game *game)
{
    int count = 0;
    for (int i = 0; i < MAX_SHOTS; i++) if (game->shots[i].active) count++;
    return count;
}

bool GameScriptSandboxTest(Game *game)
{
    Item item = { 0 };
    item.active = true;
    snprintf(item.name, sizeof(item.name), "Script Test");
    item.slot = SLOT_HAND;
    item.color = game->theme.accent2;
    snprintf(item.script, sizeof(item.script), "on_fire:burst,3,0.36,split");
    game->player.items[0] = item;
    game->player.itemCount = 1;

    int before = CountActiveShots(game);
    CombatFirePlayer(game, (Vector2){ 1.0f, 0.0f });
    int created = CountActiveShots(game) - before;
    return created >= 4 && created <= 8;
}

bool GameManifestTest(Game *game)
{
    if (!game->content.loaded) return false;
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        if (!game->content.floors[f].theme.name[0]) return false;
        for (int i = 0; i < 3; i++)
        {
            const Item *item = &game->content.floors[f].items[i];
            if (!item->name[0] || !strchr(item->script, ':')) return false;
        }
    }
    return true;
}
