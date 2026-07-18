#include "world/floor_zero.h"

#include "game/game_internal.h"
#include "script/script_items.h"

#include <stdio.h>
#include <string.h>

/* Arredo curato e STABILE del crogiolo (M1b, DEC-067): seme FISSO, mai
   game->rng -- il Piano 0 e' la sala d'attesa curata, non una stanza
   generata, e non deve cambiare aspetto a ogni ingresso (a differenza delle
   stanze di combattimento vere, vedi WorldBuildObstacles in world.c, che
   mescola coordinate e piano proprio per NON ripetersi). Densita' bassa e
   forma PILLARS (una colonna per quadrante): arredo decorativo, non un
   ostacolo da combattimento -- e nessuno di questi blocchi e' distruttibile
   (il gioco non ha affatto un sistema di ostacoli distruttibili: una bomba
   piazzata qui non ha nulla da rompere, per costruzione). La croce centrale
   (dove nasce il giocatore e da dove si vede il varco in alto) resta comunque
   sempre libera per garanzia di RoomLayoutBuild (core/room_layout.c). */
static void FloorZeroBuildDressing(Game *game)
{
    RoomLayoutDef dressing;
    memset(&dressing, 0, sizeof(dressing));
    dressing.active = true;
    dressing.form = ROOM_LAYOUT_PILLARS;
    dressing.density = 0.35f;
    snprintf(dressing.name, sizeof(dressing.name), "Crogiolo");
    game->obstacleCount = RoomLayoutBuild(&dressing, 0x50A0u, ROOM_X, ROOM_Y, ROOM_W, ROOM_H,
                                          game->obstacles, MAX_OBSTACLES);
}

void FloorZeroEnter(Game *game)
{
    /* Ogni ScriptSandbox viva dell'inventario della run precedente va chiusa
       PRIMA di azzerare player/itemScripts (stesso ordine di GameResetRun,
       vedi game.c e il commento su GameUnloadAssets in
       src/assets/game_assets.c): altrimenti si perderebbe la memoria di Lua
       senza mai chiamare lua_close. NON si chiama GameUnloadAssets: quella
       scarica anche l'atlas, e il Piano 0 non legge mai generated/ (vedi il
       commento in floor_zero.h) -- l'atlas gia' caricato resta quello buono
       anche qui. */
    ScriptItemsShutdown(game);

    /* Azzeramento MIRATO (non un memset dell'intero Game come GameResetRun):
       content/atlas/theme restano quelli gia' caricati, apposta. */
    memset(game->rooms, 0, sizeof(game->rooms));
    memset(&game->player, 0, sizeof(game->player));
    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->shots, 0, sizeof(game->shots));
    memset(game->pickups, 0, sizeof(game->pickups));
    memset(game->bombs, 0, sizeof(game->bombs));
    memset(game->particles, 0, sizeof(game->particles));
    memset(game->obstacles, 0, sizeof(game->obstacles));
    memset(game->itemScripts, 0, sizeof(game->itemScripts));
    game->obstacleCount = 0;
    game->statsDirty = false;
    game->bombQueued = false;
    game->resetQueued = false;
    game->message[0] = '\0';
    game->messageTimer = 0.0f;
    game->score = 0;
    game->roomNumber = 0;
    game->floorZeroExitOpen = false;
    game->floorZeroExitCrossed = false;
    /* M5: azzeramento MIRATO delle carte-proposta, stesso spirito del resto
       di questa funzione -- ogni nuovo ingresso in FloorZero riparte da
       "nessuna proposta, nessun tema scelto" (AppEnterFloorZero, src/app/
       app.c, le ripopola subito dopo aver chiamato questa funzione). */
    memset(game->themeCards, 0, sizeof(game->themeCards));
    game->themeCardCount = 0;
    game->themeCardFocus = 0;
    game->themeCardsPanelOpen = false;
    game->themeChosenIndex = -1;

    /* Il Piano 0 non eredita hp/oggetti/statistiche della run precedente, ne'
       anticipa quelli della prossima (la scelta del personaggio, DEC-005/
       DEC-014, e' fuori scope in M1b: vedi la spec): stessi numeri di
       partenza di una run vera (GamePlayerResetBaseStats, condivisa con
       GameResetRun apposta, vedi game_internal.h). */
    GamePlayerResetBaseStats(&game->player);
    ScriptItemsInit(game);   /* deriva damage/fireDelay/... dai base* con zero oggetti posseduti */

    game->phase = PHASE_PLAY;
    game->floor = 0;

    int cx = GRID_SIZE/2;
    int cy = GRID_SIZE/2;
    game->roomX = cx;
    game->roomY = cy;
    RoomState *hub = &game->rooms[cy][cx];
    hub->exists = true;
    hub->kind = ROOM_HUB;
    hub->cleared = true;
    hub->visited = true;
    hub->rewardTaken = true;
    /* doors[] resta tutto falso di proposito: il varco verso il piano 1 NON
       e' una porta normale (nessuna stanza adiacente a cui condurrebbe -- la
       griglia ha una sola cella) -- ha geometria di trigger e resa dedicate
       (WorldHandleTransitions in world.c, DrawFloorZeroExitGate in
       src/render/game_renderer.c). Impostare qui doors[DIR_UP] accenderebbe
       ANCHE il disegno generico della porta (DrawRoom), doppiando il varco
       dedicato con un secondo rettangolo nello stesso punto. */

    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };

    FloorZeroBuildDressing(game);
}
