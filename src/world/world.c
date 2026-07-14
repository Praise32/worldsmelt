#include "world/world.h"

#include "content/run_content.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *GameRoomKindName(RoomKind kind)
{
    switch (kind)
    {
        case ROOM_START: return "start";
        case ROOM_COMBAT: return "combat";
        case ROOM_TREASURE: return "tesoro";
        case ROOM_SHOP: return "negozio";
        case ROOM_BOSS: return "boss";
        default: return "vuota";
    }
}

static int OppositeDir(int dir)
{
    return (dir + 2)%4;
}

static int DirDx(int dir)
{
    return (dir == DIR_RIGHT) - (dir == DIR_LEFT);
}

static int DirDy(int dir)
{
    return (dir == DIR_DOWN) - (dir == DIR_UP);
}

RoomState *WorldCurrentRoomMutable(Game *game)
{
    return &game->rooms[game->roomY][game->roomX];
}

const RoomState *GameCurrentRoom(const Game *game)
{
    return &game->rooms[game->roomY][game->roomX];
}

bool WorldNoEnemiesActive(const Game *game)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (game->enemies[i].active) return false;
    }
    return true;
}

bool GameRoomIsLocked(const Game *game)
{
    const RoomState *room = GameCurrentRoom(game);
    return (room->kind == ROOM_COMBAT || room->kind == ROOM_BOSS) && !room->cleared && !WorldNoEnemiesActive(game);
}


static void WorldPlaceSpecialRoom(Game *game, RoomKind kind)
{
    int tries = 120;
    while (tries-- > 0)
    {
        int x = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        int y = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        if (game->rooms[y][x].exists) continue;
        for (int d = 0; d < 4; d++)
        {
            int nx = x + DirDx(d);
            int ny = y + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            game->rooms[y][x].exists = true;
            game->rooms[y][x].kind = kind;
            game->rooms[y][x].cleared = true;
            return;
        }
    }
}

static void WorldLinkRooms(Game *game)
{
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            RoomState *r = &game->rooms[y][x];
            if (!r->exists) continue;
            for (int d = 0; d < 4; d++)
            {
                int nx = x + DirDx(d);
                int ny = y + DirDy(d);
                r->doors[d] = (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE && game->rooms[ny][nx].exists);
            }
        }
    }
}

static void WorldGenerateFloorMap(Game *game)
{
    memset(game->rooms, 0, sizeof(game->rooms));
    int x = GRID_SIZE/2;
    int y = GRID_SIZE/2;
    int lastX = x;
    int lastY = y;
    game->roomX = x;
    game->roomY = y;

    game->rooms[y][x].exists = true;
    game->rooms[y][x].kind = ROOM_START;
    game->rooms[y][x].cleared = true;
    game->rooms[y][x].visited = true;

    int targetRooms = 7 + game->floor;
    int made = 1;
    int guard = 300;
    while (made < targetRooms && guard-- > 0)
    {
        int d = GameRngRange(&game->rng, 0, 3);
        int nx = x + DirDx(d);
        int ny = y + DirDy(d);
        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
        x = nx;
        y = ny;
        if (!game->rooms[y][x].exists)
        {
            game->rooms[y][x].exists = true;
            game->rooms[y][x].kind = ROOM_COMBAT;
            lastX = x;
            lastY = y;
            made++;
        }
    }

    if (lastX == GRID_SIZE/2 && lastY == GRID_SIZE/2)
    {
        lastX = GRID_SIZE/2 + 1;
        lastY = GRID_SIZE/2;
        game->rooms[lastY][lastX].exists = true;
    }
    game->rooms[lastY][lastX].kind = ROOM_BOSS;
    game->rooms[lastY][lastX].cleared = false;

    WorldPlaceSpecialRoom(game, ROOM_TREASURE);
    WorldPlaceSpecialRoom(game, ROOM_SHOP);
    WorldLinkRooms(game);
}

void WorldSpawnRoomContents(Game *game)
{
    EntitiesClear(game);
    RoomState *room = WorldCurrentRoomMutable(game);
    room->visited = true;
    game->roomNumber++;

    if (room->kind == ROOM_COMBAT && !room->cleared)
    {
        int count = 3 + game->floor + GameRngRange(&game->rng, 0, 2);
        for (int i = 0; i < count; i++)
        {
            EnemyKind kind = (EnemyKind)GameRngRange(&game->rng, 0, 2);
            EntitiesAddEnemy(game, kind, EntitiesRandomRoomPosition(&game->rng, 58.0f));
        }
        GameSetMessage(game, "Ripulisci la stanza per sbloccare le porte.");
    }
    else if (room->kind == ROOM_BOSS && !room->cleared)
    {
        EntitiesAddEnemy(game, ENEMY_BOSS, (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + 118.0f });
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Boss finale: ultimo piano." : "Boss del piano.");
    }
    else if (room->kind == ROOM_TREASURE && !room->rewardTaken)
    {
        int itemIndex = GameRngRange(&game->rng, 0, 2);
        EntitiesAddItemPickup(game, (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f }, game->content.floors[game->floor - 1].items[itemIndex], 0);
        GameSetMessage(game, "Stanza tesoro: prendi l'oggetto.");
    }
    else if (room->kind == ROOM_SHOP && !room->rewardTaken)
    {
        FloorContent *fc = &game->content.floors[game->floor - 1];
        /* Fase 3b (design doc, sezione 4): il costo in monete scala con la
           rarita' dell'oggetto pescato (ItemShopCostForRarity,
           src/gameplay/item_traits.c), non piu' un letterale fisso "8". */
        int shopItemIndex = GameRngRange(&game->rng, 0, 2);
        Item shopItem = fc->items[shopItemIndex];
        EntitiesAddItemPickup(game, (Vector2){ ROOM_X + ROOM_W*0.5f - 130.0f, ROOM_Y + ROOM_H*0.5f }, shopItem, ItemShopCostForRarity(shopItem.rarity));
        EntitiesAddPickup(game, PICKUP_HEART, (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f }, 1, 3);
        EntitiesAddPickup(game, PICKUP_KEY, (Vector2){ ROOM_X + ROOM_W*0.5f + 100.0f, ROOM_Y + ROOM_H*0.5f }, 1, 4);
        EntitiesAddPickup(game, PICKUP_BOMB, (Vector2){ ROOM_X + ROOM_W*0.5f + 180.0f, ROOM_Y + ROOM_H*0.5f }, 1, 3);
        GameSetMessage(game, "Negozio: tocca un oggetto per comprarlo.");
    }
    else if (room->kind == ROOM_BOSS && room->cleared)
    {
        Vector2 center = { ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
        EntitiesAddPickup(game, PICKUP_EXIT, (Vector2){ center.x + 70.0f, center.y }, 0, 0);
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Portale finale riaperto." : "Portale per il prossimo piano riaperto.");
    }
    else
    {
        GameSetMessage(game, "Scegli una porta.");
    }
}

void WorldStartFloor(Game *game, int floor)
{
    game->floor = floor;
    /* Step B2 (generazione pigra dei piani): con la generazione pigra il gioco
       parte quando e' pronto il solo piano 1, e un secondo processo melting-gen
       scrive gli script Lua dei piani 2-5 in sottofondo mentre si gioca,
       ripubblicando il manifest dopo ogni piano. Entrare in un piano e' il momento
       ESATTO in cui vale la pena riguardare il manifest: gli script di questo
       piano potrebbero essere arrivati nel frattempo. Se non ci sono ancora, non
       cambia nulla (gli oggetti restano sulla mini-VM, la degradazione di sempre).
       Nessun costo quando la generazione pigra non c'e': e' una lettura di un file
       di testo, una volta per piano. */
    RunContentRefreshFloorScripts(&game->content, floor - 1);
    game->theme = game->content.floors[floor - 1].theme;
    WorldGenerateFloorMap(game);
    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    WorldSpawnRoomContents(game);
}

void WorldTryEnterRoom(Game *game, int dir)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if (!room->doors[dir]) return;
    if (GameRoomIsLocked(game))
    {
        GameSetMessage(game, "Porte bloccate: elimina i nemici.");
        return;
    }

    int nx = game->roomX + DirDx(dir);
    int ny = game->roomY + DirDy(dir);
    if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) return;
    RoomState *next = &game->rooms[ny][nx];
    if (!next->exists) return;

    if (next->kind == ROOM_TREASURE && !next->visited)
    {
        if (game->player.keys <= 0)
        {
            GameSetMessage(game, "Serve una chiave per la stanza tesoro.");
            return;
        }
        game->player.keys--;
        GameSetMessage(game, "Chiave usata.");
    }

    game->roomX = nx;
    game->roomY = ny;
    if (dir == DIR_UP) game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_BOTTOM - 38.0f };
    if (dir == DIR_DOWN) game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + 38.0f };
    if (dir == DIR_LEFT) game->player.pos = (Vector2){ ROOM_RIGHT - 38.0f, ROOM_Y + ROOM_H*0.5f };
    if (dir == DIR_RIGHT) game->player.pos = (Vector2){ ROOM_X + 38.0f, ROOM_Y + ROOM_H*0.5f };
    (void)OppositeDir(dir);
    WorldSpawnRoomContents(game);
}

void WorldHandleTransitions(Game *game, Vector2 move)
{
    float cx = ROOM_X + ROOM_W*0.5f;
    float cy = ROOM_Y + ROOM_H*0.5f;
    float edge = game->player.radius + 7.0f;
    if (move.y < -0.1f && game->player.pos.y <= ROOM_Y + edge && fabsf(game->player.pos.x - cx) < DOOR_HALF) WorldTryEnterRoom(game, DIR_UP);
    else if (move.y > 0.1f && game->player.pos.y >= ROOM_BOTTOM - edge && fabsf(game->player.pos.x - cx) < DOOR_HALF) WorldTryEnterRoom(game, DIR_DOWN);
    else if (move.x < -0.1f && game->player.pos.x <= ROOM_X + edge && fabsf(game->player.pos.y - cy) < DOOR_HALF) WorldTryEnterRoom(game, DIR_LEFT);
    else if (move.x > 0.1f && game->player.pos.x >= ROOM_RIGHT - edge && fabsf(game->player.pos.y - cy) < DOOR_HALF) WorldTryEnterRoom(game, DIR_RIGHT);
}

static void WorldSpawnRoomReward(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if (room->rewardTaken) return;
    room->rewardTaken = true;
    Vector2 center = { ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    if (room->kind == ROOM_COMBAT)
    {
        int roll = GameRngRange(&game->rng, 0, 99);
        if (roll < 30) EntitiesAddPickup(game, PICKUP_COIN, center, GameRngRange(&game->rng, 2, 5), 0);
        else if (roll < 48) EntitiesAddPickup(game, PICKUP_HEART, center, 1, 0);
        else if (roll < 65) EntitiesAddPickup(game, PICKUP_BOMB, center, 1, 0);
        else if (roll < 82) EntitiesAddPickup(game, PICKUP_KEY, center, 1, 0);
    }
    else if (room->kind == ROOM_BOSS)
    {
        /* Fase 3 (vedi la vision doc, docs/superpowers/specs/2026-07-13-items-synergy-vision.md
           sezioni 1,2,5): il boss lascia SEMPRE l'oggetto stat-up del piano,
           mai uno a caso fra i tre attivi (quelli restano la ricompensa di
           tesoro/negozio, vedi WorldSpawnRoomContents sopra e
           game->content.floors[...].items[...] li'). */
        EntitiesAddItemPickup(game, (Vector2){ center.x - 52.0f, center.y }, game->content.floors[game->floor - 1].bossItem, 0);
        EntitiesAddPickup(game, PICKUP_EXIT, (Vector2){ center.x + 70.0f, center.y }, 0, 0);
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Boss finale sconfitto. Entra nell'uscita." : "Boss sconfitto. Prendi il premio e scendi.");
    }
}

void WorldCheckRoomClear(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if ((room->kind == ROOM_COMBAT || room->kind == ROOM_BOSS) && !room->cleared && WorldNoEnemiesActive(game))
    {
        room->cleared = true;
        WorldSpawnRoomReward(game);
        if (room->kind != ROOM_BOSS) GameSetMessage(game, "Stanza ripulita.");
    }
}
