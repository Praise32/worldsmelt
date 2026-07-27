#include "world/world.h"

#include "audio/audio.h"
#include "content/run_content.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_pool.h"
#include "gameplay/item_slots.h"
#include "gameplay/item_traits.h"
#include "world/room_camera.h"

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
        case ROOM_HUB: return "crogiolo";
        default: return "vuota";
    }
}

/* DEC-167 (docs/design/systems/rewards-and-economy.md, "Fonti canoniche della
   valuta principale"): "stanza ripulita" e' qualunque stanza completata
   secondo la PROPRIA condizione -- combattimento vinto, tesoro aperto,
   negozio visitato, segreto trovato -- non solo il combattimento. Importi
   "default proposti dall'implementazione" (stile DEC-019): nessun documento
   fissa i numeri, solo che la fonte esiste per ogni archetipo. Il boss vale
   piu' di un combattimento normale (e' la stanza piu' impegnativa del
   piano); tesoro e negozio meno di un combattimento perche' non richiedono
   di sopravvivere a nulla -- coerente con "la ricompensa deve essere
   proporzionata a rischio, costo e rarita'" (stesso documento, §Principio).
   ROOM_SECRET e la stanza a tempo (DEC-051) non hanno ancora un RoomKind nel
   motore (vedi rooms-and-floor-generation.md): questa tavola coprira' anche
   loro quando arriveranno, il default sotto le ignora per costruzione. */
#define WORLD_ROOM_CURRENCY_COMBAT   4
#define WORLD_ROOM_CURRENCY_BOSS    12
#define WORLD_ROOM_CURRENCY_TREASURE 3
#define WORLD_ROOM_CURRENCY_SHOP     2

/* Catalizzatore di fusione (Flux) -- DEC-022: "si ottiene da drop di boss o
   di arene di sfida, oppure con un acquisto costoso nel negozio", con una
   cadenza attesa di 1-2 FUSIONI per run. Le arene di sfida non esistono
   ancora nel motore (vedi systems/special-rooms.md e la matrice di
   copertura), quindi oggi le fonti sono due su tre.
   I tre numeri sono "default proposti dall'implementazione" (stile DEC-019):
   il documento fissa le FONTI e la cadenza attesa, non le probabilita'.
   Conto della cadenza su una run intera (5 piani, 5 boss, 5 negozi):
   5 x 0.35 = 1.75 catalizzatori dal boss, piu' quelli comprati -- e il
   prezzo (30 monete, piu' di un oggetto raro del negozio, vedi
   ITEM_SHOP_COST_BY_RARITY) fa si' che comprarne uno costi davvero una
   scelta. La banda 1-2 di DEC-022 e' quindi centrata sulla sola fonte
   gratuita, con l'acquisto come modo per forzarne una in piu'. */
#define WORLD_BOSS_FLUX_DROP_PERCENT 35
#define WORLD_SHOP_FLUX_STOCK_PERCENT 45
#define WORLD_SHOP_FLUX_COST 30

void WorldAwardRoomCompletionCurrency(Game *game, RoomKind kind)
{
    int amount;
    switch (kind)
    {
        case ROOM_COMBAT:   amount = WORLD_ROOM_CURRENCY_COMBAT;   break;
        case ROOM_BOSS:     amount = WORLD_ROOM_CURRENCY_BOSS;     break;
        case ROOM_TREASURE: amount = WORLD_ROOM_CURRENCY_TREASURE; break;
        case ROOM_SHOP:     amount = WORLD_ROOM_CURRENCY_SHOP;     break;
        default: return;   /* hub/start/vuota/non ancora implementato: nessuna valuta */
    }
    game->player.coins += amount;
}

static int DirDx(int dir)
{
    return (dir == DIR_RIGHT) - (dir == DIR_LEFT);
}

static int DirDy(int dir)
{
    return (dir == DIR_DOWN) - (dir == DIR_UP);
}

/* ============================================================
   DEC-170 -- celle, stanze multi-cella, geometria.

   Una stanza e' una MASCHERA di celle dentro un riquadro 2x2 (RoomState.cells,
   core/game_types.h) ancorata alla cella (originX, originY). Tutto il resto di
   questa sezione e' derivato da quei due dati: nessun rettangolo in pixel
   memorizzato, nessuna taglia da tenere sincronizzata a mano (era il rischio
   dichiarato del lattice di M2, che duplicava il minimo garantito in world.h).
   ============================================================ */

static int CellBitDx(int bit) { return bit & 1; }
static int CellBitDy(int bit) { return bit >> 1; }

/* Indice del primo bit acceso in ordine di lettura ((0,0), (1,0), (0,1),
   (1,1)): la cella di STATO di una stanza. Sempre >= 0 per una maschera non
   vuota, quindi la cella di stato esiste SEMPRE davvero -- anche per la forma
   a L a cui manca proprio l'angolo in alto a sinistra. */
static int CellMaskFirstBit(unsigned char cells)
{
    for (int i = 0; i < 4; i++) if (cells & (unsigned char)(1u << i)) return i;
    return 0;
}

static int CellMaskCount(unsigned char cells)
{
    int n = 0;
    for (int i = 0; i < 4; i++) if (cells & (unsigned char)(1u << i)) n++;
    return n;
}

static void WorldClampCell(int *cx, int *cy)
{
    if (*cx < 0) *cx = 0;
    if (*cx >= GRID_SIZE) *cx = GRID_SIZE - 1;
    if (*cy < 0) *cy = 0;
    if (*cy >= GRID_SIZE) *cy = GRID_SIZE - 1;
}

RoomState *WorldRoomAtMutable(Game *game, int cx, int cy)
{
    WorldClampCell(&cx, &cy);
    RoomState *cell = &game->rooms[cy][cx];
    if (!cell->exists || cell->cells == 0) return cell;   /* cella mai generata: 1x1 ancorata a se stessa */
    int bit = CellMaskFirstBit(cell->cells);
    int ax = cell->originX + CellBitDx(bit);
    int ay = cell->originY + CellBitDy(bit);
    if (ax < 0 || ax >= GRID_SIZE || ay < 0 || ay >= GRID_SIZE) return cell;   /* difensivo: origine corrotta */
    return &game->rooms[ay][ax];
}

const RoomState *WorldRoomAt(const Game *game, int cx, int cy)
{
    return WorldRoomAtMutable((Game *)game, cx, cy);
}

bool WorldSameRoom(const Game *game, int ax, int ay, int bx, int by)
{
    if (ax < 0 || ax >= GRID_SIZE || ay < 0 || ay >= GRID_SIZE) return false;
    if (bx < 0 || bx >= GRID_SIZE || by < 0 || by >= GRID_SIZE) return false;
    if (!game->rooms[ay][ax].exists || !game->rooms[by][bx].exists) return false;
    return WorldRoomAt(game, ax, ay) == WorldRoomAt(game, bx, by);
}

RoomSize WorldRoomSizeFromCells(unsigned char cells)
{
    int count = CellMaskCount(cells);
    if (count <= 1) return ROOM_SIZE_1X1;
    if (count == 4) return ROOM_SIZE_2X2;
    if (count == 3) return ROOM_SIZE_L;
    /* Due celle: orizzontali se condividono la riga, verticali se la colonna.
       Non si assume che la coppia sia ancorata a (0,0): una maschera scritta a
       mano in un test resta classificata bene. */
    int first = CellMaskFirstBit(cells);
    int second = CellMaskFirstBit((unsigned char)(cells & ~(unsigned char)(1u << first)));
    return (CellBitDy(first) == CellBitDy(second)) ? ROOM_SIZE_1X2 : ROOM_SIZE_2X1;
}

int WorldRoomCellCount(const Game *game, int cx, int cy)
{
    const RoomState *cell = WorldRoomAt(game, cx, cy);
    /* Mai 0: una cella mai generata (Piano 0/hub, un Game di test costruito a
       mano) vale una stanza 1x1, esattamente come per la geometria sopra --
       chi legge questo conteggio lo usa per scalare qualcosa, e uno zero lo
       azzererebbe in silenzio. */
    if (!cell->exists || cell->cells == 0) return 1;
    return CellMaskCount(cell->cells);
}

RoomState *WorldCurrentRoomMutable(Game *game)
{
    return WorldRoomAtMutable(game, game->roomX, game->roomY);
}

const RoomState *GameCurrentRoom(const Game *game)
{
    return WorldRoomAt(game, game->roomX, game->roomY);
}

/* Origine del riquadro della stanza che possiede (cx,cy). Per una cella mai
   passata dal generatore (cells == 0) l'origine e' la cella stessa: il Piano
   0/hub e un Game di test costruito a mano si comportano esattamente come
   prima di DEC-170, senza dover inizializzare nulla in piu'. */
static void WorldRoomOrigin(const Game *game, int cx, int cy, int *ox, int *oy, unsigned char *cells)
{
    WorldClampCell(&cx, &cy);
    const RoomState *cell = &game->rooms[cy][cx];
    if (cell->cells == 0)
    {
        *ox = cx; *oy = cy; *cells = ROOM_CELL_BIT(0, 0);
        return;
    }
    *ox = cell->originX; *oy = cell->originY; *cells = cell->cells;
}

/* La cella di STATO della stanza che possiede (cx,cy), in coordinate di
   griglia (la prima occupata in ordine di lettura: esiste sempre). */
static void WorldRoomStateCell(const Game *game, int cx, int cy, int *sx, int *sy)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, cx, cy, &ox, &oy, &cells);
    int bit = CellMaskFirstBit(cells);
    *sx = ox + CellBitDx(bit);
    *sy = oy + CellBitDy(bit);
}

static Rectangle WorldCellRectFrom(int ox, int oy, int cx, int cy)
{
    return (Rectangle){ ROOM_X + (float)(cx - ox)*ROOM_W, ROOM_Y + (float)(cy - oy)*ROOM_H, ROOM_W, ROOM_H };
}

Rectangle WorldCellRect(const Game *game, int cx, int cy)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, cx, cy, &ox, &oy, &cells);
    return WorldCellRectFrom(ox, oy, cx, cy);
}

Rectangle WorldRoomRect(const Game *game, int cx, int cy)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, cx, cy, &ox, &oy, &cells);
    (void)ox; (void)oy;
    float w = (cells & (ROOM_CELL_BIT(1, 0) | ROOM_CELL_BIT(1, 1))) ? ROOM_W*2.0f : ROOM_W;
    float h = (cells & (ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1))) ? ROOM_H*2.0f : ROOM_H;
    return (Rectangle){ ROOM_X, ROOM_Y, w, h };
}

Rectangle WorldCurrentRoomRect(const Game *game)
{
    return WorldRoomRect(game, game->roomX, game->roomY);
}

Vector2 WorldRoomCenter(const Game *game)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, game->roomX, game->roomY, &ox, &oy, &cells);
    float sx = 0.0f, sy = 0.0f;
    int n = 0;
    for (int i = 0; i < 4; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        Rectangle r = WorldCellRectFrom(ox, oy, ox + CellBitDx(i), oy + CellBitDy(i));
        sx += r.x + r.width*0.5f;
        sy += r.y + r.height*0.5f;
        n++;
    }
    if (n == 0) return (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    return (Vector2){ sx/(float)n, sy/(float)n };
}

int WorldRoomCells(const Game *game, int *outX, int *outY, int maxOut)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, game->roomX, game->roomY, &ox, &oy, &cells);
    int n = 0;
    for (int i = 0; i < 4 && n < maxOut; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        if (outX) outX[n] = ox + CellBitDx(i);
        if (outY) outY[n] = oy + CellBitDy(i);
        n++;
    }
    return n;
}

int WorldRoomHoleCount(const Game *game)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, game->roomX, game->roomY, &ox, &oy, &cells);
    (void)ox; (void)oy;
    /* Solo le celle DENTRO il riquadro: una 1x2 non ha buchi, il suo riquadro
       e' alto una cella sola. */
    int w = (cells & (ROOM_CELL_BIT(1, 0) | ROOM_CELL_BIT(1, 1))) ? 2 : 1;
    int h = (cells & (ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1))) ? 2 : 1;
    return w*h - CellMaskCount(cells);
}

Rectangle WorldRoomHoleRect(const Game *game, int index)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, game->roomX, game->roomY, &ox, &oy, &cells);
    int w = (cells & (ROOM_CELL_BIT(1, 0) | ROOM_CELL_BIT(1, 1))) ? 2 : 1;
    int h = (cells & (ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1))) ? 2 : 1;
    int seen = 0;
    for (int dy = 0; dy < h; dy++)
    {
        for (int dx = 0; dx < w; dx++)
        {
            if (cells & ROOM_CELL_BIT(dx, dy)) continue;
            if (seen == index) return WorldCellRectFrom(ox, oy, ox + dx, oy + dy);
            seen++;
        }
    }
    return (Rectangle){ 0.0f, 0.0f, 0.0f, 0.0f };
}

void WorldPlayerCell(const Game *game, int *cx, int *cy)
{
    int ox, oy; unsigned char cells;
    WorldRoomOrigin(game, game->roomX, game->roomY, &ox, &oy, &cells);
    int dx = (int)floorf((game->player.pos.x - ROOM_X)/ROOM_W);
    int dy = (int)floorf((game->player.pos.y - ROOM_Y)/ROOM_H);
    if (dx < 0) dx = 0;
    if (dx > 1) dx = 1;
    if (dy < 0) dy = 0;
    if (dy > 1) dy = 1;
    if (!(cells & ROOM_CELL_BIT(dx, dy)))
    {
        /* Fuori dalle celle occupate (il buco di una forma a L, per il frame in
           cui la collisione non ha ancora respinto): la cella occupata piu'
           vicina, mai una cella che non esiste. */
        float best = -1.0f;
        for (int i = 0; i < 4; i++)
        {
            if (!(cells & (unsigned char)(1u << i))) continue;
            Rectangle r = WorldCellRectFrom(ox, oy, ox + CellBitDx(i), oy + CellBitDy(i));
            float px = GameMathClampFloat(game->player.pos.x, r.x, r.x + r.width);
            float py = GameMathClampFloat(game->player.pos.y, r.y, r.y + r.height);
            float d = (px - game->player.pos.x)*(px - game->player.pos.x) + (py - game->player.pos.y)*(py - game->player.pos.y);
            if (best < 0.0f || d < best) { best = d; dx = CellBitDx(i); dy = CellBitDy(i); }
        }
    }
    if (cx) *cx = ox + dx;
    if (cy) *cy = oy + dy;
}

void WorldClampToRoom(const Game *game, Vector2 *pos, float radius)
{
    Rectangle room = WorldCurrentRoomRect(game);
    pos->x = GameMathClampFloat(pos->x, room.x + radius, room.x + room.width - radius);
    pos->y = GameMathClampFloat(pos->y, room.y + radius, room.y + room.height - radius);
}

/* ============================================================
   DEC-170 -- telecamera (il principio sta in world/room_camera.h).
   ============================================================ */

Rectangle WorldCameraFocusRect(const Game *game)
{
    const RoomState *room = GameCurrentRoom(game);
    if (WorldRoomSizeFromCells(room->cells) == ROOM_SIZE_L)
    {
        /* Forma a L: il clamp usa la CELLA CORRENTE, non il riquadro. E' la
           sola scelta che rispetta DEC-170 alla lettera ("non mostra mai area
           fuori dal rettangolo occupato") senza inventare uno zoom dinamico o
           una maschera: il riquadro di una L contiene l'angolo mancante, e una
           telecamera libera dentro il riquadro lo mostrerebbe. Il prezzo e' un
           salto del bersaglio quando si cambia cella, assorbito
           dall'interpolazione di WorldCameraApproach. */
        int cx, cy;
        WorldPlayerCell(game, &cx, &cy);
        return WorldCellRect(game, cx, cy);
    }
    return WorldCurrentRoomRect(game);
}

static Vector2 WorldCameraDesiredTarget(const Game *game)
{
    Rectangle bounds = WorldCameraBoundsFromRoom(WorldCameraFocusRect(game));
    return WorldCameraClampTarget(bounds, game->player.pos, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
}

Camera2D WorldGameCamera(const Game *game)
{
    Camera2D cam;
    cam.offset = (Vector2){ (float)SCREEN_WIDTH*0.5f, (float)SCREEN_HEIGHT*0.5f };
    cam.target = game->cameraTarget;
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;   /* DEC-170: MAI un valore diverso da 1 */
    return cam;
}

Rectangle WorldCameraView(const Game *game)
{
    return (Rectangle){ game->cameraTarget.x - (float)SCREEN_WIDTH*0.5f,
                        game->cameraTarget.y - (float)SCREEN_HEIGHT*0.5f,
                        (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
}

Vector2 WorldCanvasToWorld(const Game *game, Vector2 canvasPos)
{
    Rectangle view = WorldCameraView(game);
    return (Vector2){ canvasPos.x + view.x, canvasPos.y + view.y };
}

void WorldSnapCamera(Game *game)
{
    game->cameraTarget = WorldCameraDesiredTarget(game);
}

void WorldUpdateCamera(Game *game, float dt)
{
    game->cameraTarget = WorldCameraApproach(game->cameraTarget, WorldCameraDesiredTarget(game), dt, WORLD_CAMERA_RATE);
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

/* ============================================================
   DEC-170 -- generazione del piano a FORME.

   Il generatore non scava piu' celle singole: piazza FORME (le cinque classi
   di DEC-170) una accanto all'altra. Due garanzie strutturali, entrambe per
   costruzione e non per tentativi:
     - NIENTE SOVRAPPOSIZIONI: una forma si scrive solo se TUTTE le sue celle
       sono libere e dentro la griglia (WorldShapeFits);
     - PIANO CONNESSO: ogni forma nuova deve contenere una cella ADIACENTE a
       una cella gia' occupata, quindi la stanza nuova confina sempre con una
       vecchia (e WorldLinkRooms ci mette una porta).
   Tutte le estrazioni passano da game->rng: stesso seed => stesso piano,
   taglie comprese.
   ============================================================ */

/* Distribuzione delle taglie (DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE, stile
   DEC-019: DEC-170 fissa le classi, non le percentuali -- vedi
   rooms-and-floor-generation.md, "Default proposti"). Percentuali cumulate su
   100. La 1x1 resta la maggioranza netta: le taglie grandi devono restare un
   evento, o il piano perde il ritmo "una stanza, una schermata" e la
   telecamera fissa (il caso che DEC-170 vuole invariato) diventerebbe
   l'eccezione. */
#define WORLD_SIZE_CUM_1X1 55
#define WORLD_SIZE_CUM_1X2 70
#define WORLD_SIZE_CUM_2X1 85
#define WORLD_SIZE_CUM_2X2 93
/* il resto (7%) e' la forma a L */

static unsigned char WorldRollShapeCells(Game *game)
{
    int roll = GameRngRange(&game->rng, 0, 99);
    if (roll < WORLD_SIZE_CUM_1X1) return ROOM_CELL_BIT(0, 0);
    if (roll < WORLD_SIZE_CUM_1X2) return (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0));
    if (roll < WORLD_SIZE_CUM_2X1) return (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(0, 1));
    if (roll < WORLD_SIZE_CUM_2X2) return (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0) |
                                                          ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1));
    /* Forma a L: il blocco 2x2 meno UN angolo, tutti e quattro gli
       orientamenti (la cella di stato resta occupata per costruzione, vedi
       CellMaskFirstBit). */
    int missing = GameRngRange(&game->rng, 0, 3);
    return (unsigned char)(0x0Fu & ~(unsigned char)(1u << missing));
}

static bool WorldCellFree(const Game *game, int cx, int cy)
{
    if (cx < 0 || cx >= GRID_SIZE || cy < 0 || cy >= GRID_SIZE) return false;
    return !game->rooms[cy][cx].exists;
}

static bool WorldShapeFits(const Game *game, int ox, int oy, unsigned char cells)
{
    for (int i = 0; i < 4; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        if (!WorldCellFree(game, ox + CellBitDx(i), oy + CellBitDy(i))) return false;
    }
    return true;
}

static RoomState *WorldWriteRoom(Game *game, int ox, int oy, unsigned char cells, RoomKind kind)
{
    for (int i = 0; i < 4; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        RoomState *cell = &game->rooms[oy + CellBitDy(i)][ox + CellBitDx(i)];
        memset(cell, 0, sizeof(*cell));
        cell->exists = true;
        cell->cells = cells;
        cell->originX = ox;
        cell->originY = oy;
    }
    int bit = CellMaskFirstBit(cells);
    RoomState *state = &game->rooms[oy + CellBitDy(bit)][ox + CellBitDx(bit)];
    state->kind = kind;
    return state;
}

/* Piazza 'cells' in modo che la cella LIBERA (nx,ny) ne faccia parte, provando
   ogni ruolo che quella cella puo' avere dentro la forma (la stessa L incastra
   in quattro modi diversi attorno allo stesso punto). 'rot' sposta il punto di
   partenza della ricerca: e' cio' che rende varie le forme grandi in una
   griglia stretta, invece di ancorarle sempre in alto a sinistra. */
static RoomState *WorldPlaceShapeAt(Game *game, int nx, int ny, unsigned char cells, RoomKind kind, int rot)
{
    for (int k = 0; k < 4; k++)
    {
        int i = (rot + k)%4;
        if (!(cells & (unsigned char)(1u << i))) continue;
        int ox = nx - CellBitDx(i);
        int oy = ny - CellBitDy(i);
        if (!WorldShapeFits(game, ox, oy, cells)) continue;
        return WorldWriteRoom(game, ox, oy, cells, kind);
    }
    return NULL;
}

/* Le celle occupate del piano, compattate: il generatore ci pesca dentro per
   scegliere da dove far crescere la stanza successiva. Pescare una CELLA (e
   non una stanza) da' piu' peso alle stanze grandi, che infatti hanno piu'
   perimetro su cui attaccare una vicina. */
static int WorldCollectCells(const Game *game, int *outX, int *outY)
{
    int n = 0;
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            if (game->rooms[y][x].exists) { outX[n] = x; outY[n] = y; n++; }
    return n;
}

/* Distanza in celle dalla partenza (BFS sulle celle esistenti): serve solo a
   mettere il boss lontano, quindi una BFS a costo uniforme basta e avanza. */
static void WorldCellDistances(const Game *game, int sx, int sy, int dist[GRID_SIZE][GRID_SIZE])
{
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            dist[y][x] = -1;
    int queueX[GRID_SIZE*GRID_SIZE], queueY[GRID_SIZE*GRID_SIZE];
    int head = 0, tail = 0;
    dist[sy][sx] = 0;
    queueX[tail] = sx; queueY[tail] = sy; tail++;
    while (head < tail)
    {
        int x = queueX[head], y = queueY[head];
        head++;
        for (int d = 0; d < 4; d++)
        {
            int nx = x + DirDx(d), ny = y + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1;
            queueX[tail] = nx; queueY[tail] = ny; tail++;
        }
    }
}

/* La stanza boss si piazza NUOVA, attaccata al piano gia' cresciuto, e si
   prova prima la classe piu' GRANDE: DEC-170 non fissa la taglia del boss, e
   un'arena 2x2 e' il default proposto qui (bosses.md chiede spazio, e la 2x2 e'
   il massimo che la griglia possa dare). L'ordine delle prove e' "prima la
   taglia, poi la distanza": fra le celle candidate si parte sempre dalla piu'
   lontana dalla partenza, ma una 2x2 un po' meno lontana batte una 1x1 in
   fondo al piano -- l'arena e' cio' che rende leggibile lo scontro, e la
   frontiera libera di un piano cresciuto dal centro e' comunque periferia.
   Se non entra nulla di grande si scende di classe fino alla 1x1, che nella
   prima cella libera entra sempre. */
static bool WorldPlaceBossRoom(Game *game, const int *orderX, const int *orderY, int orderCount)
{
    static const unsigned char kBossShapes[] = {
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0) | ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(1, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(0, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(1, 0)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(0, 0)),
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0)),
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(0, 1)),
        ROOM_CELL_BIT(0, 0)
    };
    const int shapeCount = (int)(sizeof(kBossShapes)/sizeof(kBossShapes[0]));
    int dirStart = GameRngRange(&game->rng, 0, 3);
    int rot = GameRngRange(&game->rng, 0, 3);
    for (int s = 0; s < shapeCount; s++)
    {
        for (int c = 0; c < orderCount; c++)
        {
            for (int k = 0; k < 4; k++)
            {
                int d = (dirStart + k)%4;
                int nx = orderX[c] + DirDx(d), ny = orderY[c] + DirDy(d);
                if (!WorldCellFree(game, nx, ny)) continue;
                if (WorldPlaceShapeAt(game, nx, ny, kBossShapes[s], ROOM_BOSS, rot)) return true;
            }
        }
    }
    return false;
}

static void WorldPlaceSpecialRoom(Game *game, RoomKind kind)
{
    int tries = 120;
    while (tries-- > 0)
    {
        int x = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        int y = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        if (!WorldCellFree(game, x, y)) continue;
        for (int d = 0; d < 4; d++)
        {
            int nx = x + DirDx(d);
            int ny = y + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            /* Tesoro e negozio restano 1x1 (default proposto): sono stanze da
               una ricompensa, tutta visibile appena si entra -- e' proprio il
               caso in cui la telecamera fissa di DEC-170 e' un pregio. */
            RoomState *state = WorldWriteRoom(game, x, y, ROOM_CELL_BIT(0, 0), kind);
            state->cleared = true;
            return;
        }
    }
}

/* Una porta collega due celle ADIACENTI di stanze DIVERSE. Due celle della
   stessa stanza non hanno porta fra loro: sono lo stesso spazio continuo, il
   giocatore ci passa camminando (DEC-170). */
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
                bool inGrid = (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE);
                r->doors[d] = inGrid && game->rooms[ny][nx].exists && !WorldSameRoom(game, x, y, nx, ny);
            }
        }
    }
}

static void WorldGenerateFloorMap(Game *game)
{
    memset(game->rooms, 0, sizeof(game->rooms));
    int sx = GRID_SIZE/2;
    int sy = GRID_SIZE/2;
    game->roomX = sx;
    game->roomY = sy;

    /* Stanza di partenza SEMPRE 1x1 (default proposto): il primo schermo di un
       piano e' anche quello che insegna a leggere lo spazio, e la 1x1 e' la
       taglia che il giocatore vede per intero senza muovere la telecamera. */
    RoomState *start = WorldWriteRoom(game, sx, sy, ROOM_CELL_BIT(0, 0), ROOM_START);
    start->cleared = true;
    start->visited = true;

    /* DEC-170 cambia l'unita' di misura del piano: il budget resta quello di
       DEC-009 (6 + piano + 0..3) ma conta CELLE, non stanze -- una stanza ne
       occupa da 1 a 4. Cosi' la superficie giocabile di un piano resta quella
       di sempre e non esplode con le taglie grandi; il numero di STANZE scende
       (e' la conseguenza dichiarata di DEC-170, non un effetto collaterale). */
    int targetCells = 6 + game->floor + GameRngRange(&game->rng, 0, 3);
    int placedCells = 1;
    int guard = 400;
    while (placedCells < targetCells && guard-- > 0)
    {
        int cellsX[GRID_SIZE*GRID_SIZE], cellsY[GRID_SIZE*GRID_SIZE];
        int occupied = WorldCollectCells(game, cellsX, cellsY);
        if (occupied <= 0) break;
        int pick = GameRngRange(&game->rng, 0, occupied - 1);
        int d = GameRngRange(&game->rng, 0, 3);
        int nx = cellsX[pick] + DirDx(d);
        int ny = cellsY[pick] + DirDy(d);
        if (!WorldCellFree(game, nx, ny)) continue;

        unsigned char shape = WorldRollShapeCells(game);
        int rot = GameRngRange(&game->rng, 0, 3);
        if (WorldPlaceShapeAt(game, nx, ny, shape, ROOM_COMBAT, rot) != NULL)
        {
            placedCells += CellMaskCount(shape);
        }
        else
        {
            /* La forma estratta non entra qui: si ripiega sulla 1x1, che nella
               cella libera (nx,ny) entra sempre. Mai saltare il turno: e' cio'
               che tiene il numero di celle del piano indipendente dalla
               fortuna delle estrazioni. */
            WorldPlaceShapeAt(game, nx, ny, ROOM_CELL_BIT(0, 0), ROOM_COMBAT, 0);
            placedCells += 1;
        }
    }

    /* Boss: le celle candidate a cui attaccarlo, ordinate per distanza
       DECRESCENTE dalla partenza (ordinamento per inserzione: 25 elementi al
       massimo, e serve stabile per restare deterministico). */
    int dist[GRID_SIZE][GRID_SIZE];
    WorldCellDistances(game, sx, sy, dist);
    int orderX[GRID_SIZE*GRID_SIZE], orderY[GRID_SIZE*GRID_SIZE];
    int orderCount = WorldCollectCells(game, orderX, orderY);
    for (int i = 1; i < orderCount; i++)
    {
        int cx = orderX[i], cy = orderY[i], key = dist[cy][cx];
        int j = i - 1;
        while (j >= 0 && dist[orderY[j]][orderX[j]] < key)
        {
            orderX[j + 1] = orderX[j];
            orderY[j + 1] = orderY[j];
            j--;
        }
        orderX[j + 1] = cx;
        orderY[j + 1] = cy;
    }

    if (!WorldPlaceBossRoom(game, orderX, orderY, orderCount))
    {
        /* Griglia satura: come ultima rete si promuove a boss la stanza piu'
           lontana gia' esistente. Un piano SENZA stanza boss non e'
           completabile, quindi qui non si esce mai a mani vuote se esiste
           almeno una stanza oltre la partenza. */
        RoomState *far = WorldRoomAtMutable(game, orderX[0], orderY[0]);
        if (far->kind != ROOM_START)
        {
            far->kind = ROOM_BOSS;
            far->cleared = false;
            far->visited = false;
        }
    }

    WorldPlaceSpecialRoom(game, ROOM_TREASURE);
    WorldPlaceSpecialRoom(game, ROOM_SHOP);
    WorldLinkRooms(game);
}

/* Fase 3c (DEC-170: una espansione PER CELLA): ricostruisce gli ostacoli della
   stanza CORRENTE dal layout del piano. Le celle-buco di una forma a L entrano
   sempre, per qualunque tipo di stanza: sono muro vero, e devono fermare
   giocatore, nemici e colpi con lo stesso codice che ferma un ostacolo
   qualsiasi. I blocchi del layout restano solo per le stanze di
   COMBATTIMENTO non ripulite: il boss ha bisogno di spazio, e in
   tesoro/negozio l'oggetto da raccogliere non deve mai finire dietro un muro.
   Il layout si espande una volta per cella (scelta di implementazione ammessa
   da DEC-170) col seme mescolato dalle coordinate ASSOLUTE della cella: due
   celle della stessa stanza non hanno lo stesso arredo, e ognuna conserva le
   garanzie di RoomLayoutBuild (croce centrale libera => la porta al centro di
   ogni lato, e il passaggio verso la cella accanto, restano sempre
   raggiungibili). */
static void WorldBuildObstacles(Game *game, const RoomState *room)
{
    game->obstacleCount = 0;
    game->obstacleHoleCount = 0;

    int holes = WorldRoomHoleCount(game);
    for (int i = 0; i < holes && game->obstacleCount < MAX_OBSTACLES; i++)
    {
        Rectangle hole = WorldRoomHoleRect(game, i);
        if (hole.width <= 0.0f || hole.height <= 0.0f) continue;
        game->obstacles[game->obstacleCount].x = hole.x;
        game->obstacles[game->obstacleCount].y = hole.y;
        game->obstacles[game->obstacleCount].w = hole.width;
        game->obstacles[game->obstacleCount].h = hole.height;
        game->obstacleCount++;
    }
    game->obstacleHoleCount = game->obstacleCount;

    if (room->kind != ROOM_COMBAT || room->cleared) return;
    const RoomLayoutDef *layout = &game->content.floors[game->floor - 1].roomLayout;
    if (!layout->active) return;

    int cellX[4], cellY[4];
    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
    for (int i = 0; i < cellCount; i++)
    {
        int cx = cellX[i], cy = cellY[i];
        unsigned int seed = (unsigned int)(cx*73856093) ^ (unsigned int)(cy*19349663) ^ ((unsigned int)game->floor*83492791u);
        Rectangle rect = WorldCellRect(game, cx, cy);
        int budget = MAX_OBSTACLES - game->obstacleCount;
        if (budget > ROOM_LAYOUT_MAX_PER_CELL) budget = ROOM_LAYOUT_MAX_PER_CELL;
        if (budget <= 0) break;
        game->obstacleCount += RoomLayoutBuild(layout, seed, rect.x, rect.y, rect.width, rect.height,
                                               game->obstacles + game->obstacleCount, budget);
    }
}

/* Una posizione casuale nella stanza che NON cade dentro un ostacolo (fase 3c): un
   nemico non deve mai nascere incastrato in un muro. Riprova fino a 12 volte; se non
   trova un punto libero (stanza fittissima) usa comunque l'ultima -- la risoluzione
   della collisione lo spingera' fuori al primo frame. Con DEC-170 questo copre
   gratis anche l'angolo mancante di una forma a L: e' un ostacolo come gli altri. */
static Vector2 WorldFreeRoomPosition(Game *game, float pad)
{
    Rectangle room = WorldCurrentRoomRect(game);
    Vector2 pos = EntitiesRandomRoomPosition(&game->rng, room, pad);
    for (int tries = 0; tries < 12; tries++)
    {
        bool inside = false;
        for (int i = 0; i < game->obstacleCount; i++)
        {
            Obstacle *o = &game->obstacles[i];
            if (pos.x > o->x - pad && pos.x < o->x + o->w + pad &&
                pos.y > o->y - pad && pos.y < o->y + o->h + pad) { inside = true; break; }
        }
        if (!inside) return pos;
        pos = EntitiesRandomRoomPosition(&game->rng, room, pad);
    }
    return pos;
}

/* Il BUDGET DI DIFFICOLTA' della stanza (fase 3b, la seconda delle due reti che
   permettono di lasciare a un 7B l'invenzione dei nemici -- la prima e'
   EnemyTypeBalance, core/enemy_type.c).
 *
 * La stanza non spawna piu' "N nemici": SPENDE un budget di punti, e ogni nemico
 * costa la propria potenza (EnemyTypePower, 1.0 = il nemico base). Un piano i cui
 * nemici sono cattivi ne riceve quindi MENO; uno con nemici fiacchi, di piu'. La
 * difficolta' della stanza resta cosi' una decisione del C -- il modello decide
 * COM'E' un nemico, non QUANTA roba ti trovi davanti.
 *
 * Il budget e' esattamente quello che spendeva la formula di prima (3 + piano
 * nemici da 1.0 di potenza), quindi una run senza tipi (manifest vecchio) genera
 * stanze della stessa difficolta' di sempre. DEC-170 aggiunge una scala per
 * CELLA: una stanza 2x2 e' quattro schermate di spazio, e col budget di una
 * sola schermata sarebbe vuota. */
void WorldSpawnCombatRoom(Game *game)
{
    const FloorContent *fc = &game->content.floors[game->floor - 1];
    float budget = 3.0f + (float)game->floor + (float)GameRngRange(&game->rng, 0, 2);
    /* DEC-170 (default proposto): il budget cresce con le celle, ma SOTTO la
       proporzione (radice quadrata invece che lineare) -- una stanza grande
       deve sembrare piu' grande, non quattro stanze appiccicate. */
    int cellCount = WorldRoomCellCount(game, game->roomX, game->roomY);
    if (cellCount > 1) budget *= sqrtf((float)cellCount);

    /* Gli INDICI degli slot attivi, compattati (correzione da review). Contare
       quanti sono e poi indicizzare enemies[0..typeCount-1] presuppone che gli slot
       attivi siano impaccati dall'indice 0 -- falso per un manifest scritto a mano
       che definisce enemy2 ma non enemy1 (ReadEnemyType riempie i due slot in modo
       indipendente): con typeCount=1 e GameRngRange(0,0)=0 si pescava sempre lo
       slot 0 INATTIVO, spawnando il chaser storico invece del nemico del modello --
       per tutto il piano. Con la lista compattata si pesca solo fra gli slot che
       esistono davvero. */
    int activeIdx[2];
    int typeCount = 0;
    for (int i = 0; i < 2; i++) if (fc->enemies[i].active) activeIdx[typeCount++] = i;

    int spawned = 0;
    /* Il tetto duro resta MAX_ENEMIES; 16 per cella e' il tetto di BUON SENSO
       (con nemici fiacchissimi il budget potrebbe altrimenti farne comparire una
       folla illeggibile). */
    int spawnCap = 16*cellCount;
    if (spawnCap > MAX_ENEMIES) spawnCap = MAX_ENEMIES;
    while (budget > 0.0f && spawned < spawnCap)
    {
        if (typeCount > 0)
        {
            int slot = activeIdx[GameRngRange(&game->rng, 0, typeCount - 1)];
            const EnemyTypeDef *type = &fc->enemies[slot];
            float cost = EnemyTypePower(type);
            if (cost < 0.35f) cost = 0.35f;   /* mai gratis: un nemico costa sempre qualcosa, o il ciclo non finirebbe */
            /* L'ultimo nemico si spawna anche se sfora un po': una stanza deve
               avere almeno un nemico, e un budget di 0.2 avanzato non deve
               lasciare una stanza vuota che si apre da sola. */
            if (cost > budget && spawned > 0) break;
            /* Il "kind" storico serve solo a scegliere la cella dell'atlas: si
               deriva dalla forma, cosi' uno sprite plausibile c'e' comunque. */
            EnemyKind kind = ENEMY_CHASER;
            if (type->form == ENEMY_FORM_SPIKY) kind = ENEMY_SHOOTER;
            else if (type->form == ENEMY_FORM_ARMORED) kind = ENEMY_TANK;
            EntitiesAddEnemyTyped(game, kind, WorldFreeRoomPosition(game, 58.0f), type);
            /* M7 (substrato del catalogo): questo TIPO e' appena comparso in una
               stanza dove il giocatore e' presente per costruzione (questa
               funzione gira solo da WorldSpawnRoomContents, chiamata solo
               all'ingresso in una stanza) -- vedi il commento su
               Game.enemyEncountered in core/game_types.h.
               DEC-065/152: la card di scoperta si accoda SOLO la prima volta
               (il flag e' ancora falso QUI, prima di scriverlo sotto) --
               registrazione ed annuncio restano due cose distinte: il flag si
               scrive SEMPRE, la card e' solo un di piu' non garantito (puo'
               finire scartata dalla coda, DEC-131/152, senza che il Catalogo
               se ne accorga). */
            if (!game->enemyEncountered[game->floor - 1][slot])
                GameQueueDiscoveryCard(game, type->name[0] ? type->name : "Nemico sconosciuto",
                                       "Una nuova minaccia di questo mondo.");
            game->enemyEncountered[game->floor - 1][slot] = true;
            budget -= cost;
        }
        else
        {
            EnemyKind kind = (EnemyKind)GameRngRange(&game->rng, 0, 2);
            EntitiesAddEnemy(game, kind, WorldFreeRoomPosition(game, 58.0f));
            budget -= 1.0f;   /* i nemici storici valgono 1.0 per definizione */
        }
        spawned++;
    }
}

/* Vero se il negozio di questo piano tiene un catalizzatore in banco (vedi
   WORLD_SHOP_FLUX_STOCK_PERCENT). Deterministico dal seed di run e dal piano,
   MAI da game->rng: deve dare la stessa risposta ad ogni ingresso nella stessa
   stanza, altrimenti uscire e rientrare diventerebbe un modo per farlo
   comparire. Stream locale (una variabile sullo stack passata a GameRngNext)
   proprio per non toccare la sequenza di gioco. */
static bool WorldShopStocksFlux(const Game *game)
{
    unsigned int state = game->runSeed ^ ((unsigned int)game->floor*2654435761u) ^ 0x464C5558u;   /* 'FLUX' */
    return (int)(GameRngNext(&state)%100u) < WORLD_SHOP_FLUX_STOCK_PERCENT;
}

void WorldSpawnRoomContents(Game *game)
{
    EntitiesClear(game);
    RoomState *room = WorldCurrentRoomMutable(game);
    /* DEC-167: il negozio conta come "ripulito" quando e' stato VISITATO
       (non quando ci si compra qualcosa, che e' un evento economico a
       parte, vedi CombatPickup) -- catturato PRIMA di scrivere 'visited',
       cosi' rientrare in un negozio gia' visto non paga una seconda volta
       (questa funzione gira a OGNI ingresso, non solo al primo). */
    bool firstVisit = !room->visited;
    room->visited = true;
    game->roomNumber++;
    if (firstVisit && room->kind == ROOM_SHOP) WorldAwardRoomCompletionCurrency(game, ROOM_SHOP);
    /* DEC-170: le posizioni di spawno sono relative al BARICENTRO delle celle
       occupate, non al centro del riquadro -- su una forma a L quel centro
       cadrebbe nell'angolo mancante, cioe' dentro il muro. */
    Vector2 center = WorldRoomCenter(game);

    /* Fase 3c: gli ostacoli si costruiscono PRIMA di piazzare i nemici, cosi'
       WorldFreeRoomPosition puo' evitarli. */
    WorldBuildObstacles(game, room);

    if (room->kind == ROOM_COMBAT && !room->cleared)
    {
        WorldSpawnCombatRoom(game);
        GameSetMessage(game, "Ripulisci la stanza per sbloccare le porte.");
    }
    else if (room->kind == ROOM_BOSS && !room->cleared)
    {
        /* Fase 3b: il boss del piano e' il TIPO che il modello ha inventato per
           questo piano. Se non c'e' (manifest vecchio, nessun manifest) resta il
           boss storico: EntitiesAddEnemyTyped con un tipo non attivo e' esattamente
           EntitiesAddEnemy. Sta un quarto di cella sopra il baricentro: lontano
           dal giocatore qualunque porta abbia usato per entrare. */
        const FloorContent *fc = &game->content.floors[game->floor - 1];
        const EnemyTypeDef *bossType = fc->bossType.active ? &fc->bossType : NULL;
        EntitiesAddEnemyTyped(game, ENEMY_BOSS, (Vector2){ center.x, center.y - ROOM_H*0.25f }, bossType);
        /* M7 (substrato del catalogo): il boss e' appena comparso -- il
           giocatore e' per costruzione nella stanza (vedi il commento sopra
           su WorldSpawnCombatRoom). L'esito (incontrato/sconfitto) lo marca
           WorldCheckRoomClear quando la stanza si ripulisce.
           DEC-065/152: stessa disciplina di WorldSpawnCombatRoom -- la card
           si accoda SOLO la prima volta (flag ancora falso QUI), il flag si
           scrive comunque SEMPRE subito dopo. */
        if (!game->bossEncountered[game->floor - 1])
        {
            const char *bossName = (bossType && bossType->name[0]) ? bossType->name : "Il boss del piano";
            GameQueueDiscoveryCard(game, bossName, "Il guardiano di questo piano.");
        }
        game->bossEncountered[game->floor - 1] = true;
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Boss finale: ultimo piano." : "Boss del piano.");
    }
    else if (room->kind == ROOM_TREASURE && !room->rewardTaken)
    {
        /* DEC-019/DEC-145: estrazione UNIFORME fra i 3 candidati del piano
           (gia' pesati per rarita' a monte, vedi il commento sopra
           ItemPoolDrawIndex in item_pool.h -- pesare di nuovo qui
           applicherebbe DEC-019 due volte), con la correzione di fortuna del
           giocatore. Vedi ItemPoolDrawIndex, src/gameplay/item_pool.h;
           game->treasureLuckStreak e' il contatore persistente di
           estrazioni comuni consecutive per QUESTO pool (core/game_types.h).

           NOTA: questa funzione gira a OGNI ingresso in stanza (vedi il
           commento su WorldSpawnRoomContents, game_types.h) e finche'
           'rewardTaken' resta falso ritira un oggetto NUOVO e avanza
           'treasureLuckStreak' ad ogni ingresso -- entrare/uscire dalla
           stanza senza raccogliere l'oggetto e' percio' un modo per far
           avanzare (o azzerare) il contatore di correzione di fortuna senza
           aver davvero ricevuto una ricompensa. Scelta accettata per questa
           fase (il ri-tiro ad ogni ingresso preesisteva a DEC-145, vedi
           combat.c riga ~466): non e' un problema di correttezza del
           pity/Fortuna (lo stato resta comunque deterministico dal seed),
           solo una superficie di sfruttamento minore non chiusa qui. */
        const FloorContent *tfc = &game->content.floors[game->floor - 1];
        Rarity treasureRarities[3] = { tfc->items[0].rarity, tfc->items[1].rarity, tfc->items[2].rarity };
        int itemIndex = ItemPoolDrawIndex(&game->rng, treasureRarities, 3, ItemPoolWeightsStandard,
                                           game->player.luck, &game->treasureLuckStreak);
        EntitiesAddItemPickup(game, center, tfc->items[itemIndex], 0);
        GameSetMessage(game, "Stanza tesoro: prendi l'oggetto.");
    }
    else if (room->kind == ROOM_SHOP && !room->rewardTaken)
    {
        FloorContent *fc = &game->content.floors[game->floor - 1];
        /* Fase 3b (design doc, sezione 4): il costo in monete scala con la
           rarita' dell'oggetto pescato (ItemShopCostForRarity,
           src/gameplay/item_traits.c), non piu' un letterale fisso "8". */
        /* Stessa estrazione uniforme + correzione di fortuna della stanza
           tesoro sopra (stessa nota sul ri-ingresso), ma sul CONTATORE del
           negozio (game->shopLuckStreak): i due pool sono la stessa fonte
           (fc->items[0..2]) ma il documento tratta ogni pool come
           indipendente ai fini della soglia N (items-pools-and-rarity.md,
           "Scope: tutti i pool"). */
        Rarity shopRarities[3] = { fc->items[0].rarity, fc->items[1].rarity, fc->items[2].rarity };
        int shopItemIndex = ItemPoolDrawIndex(&game->rng, shopRarities, 3, ItemPoolWeightsStandard,
                                               game->player.luck, &game->shopLuckStreak);
        Item shopItem = fc->items[shopItemIndex];
        EntitiesAddItemPickup(game, (Vector2){ center.x - 130.0f, center.y }, shopItem, ItemShopCostForRarity(shopItem.rarity));
        EntitiesAddPickup(game, PICKUP_HEART, center, 1, 3);
        EntitiesAddPickup(game, PICKUP_KEY, (Vector2){ center.x + 100.0f, center.y }, 1, 4);
        EntitiesAddPickup(game, PICKUP_BOMB, (Vector2){ center.x + 180.0f, center.y }, 1, 3);
        /* DEC-022, seconda fonte del catalizzatore: l'acquisto costoso.
           Se il negozio di QUESTO piano lo tiene in banco non lo decide una
           tiratura di game->rng ma un hash del seed di run e del piano: questa
           funzione gira a OGNI ingresso in stanza finche' 'rewardTaken' e'
           falso, quindi una tiratura vera si potrebbe rifare uscendo e
           rientrando finche' il Flux compare. */
        if (WorldShopStocksFlux(game))
            EntitiesAddPickup(game, PICKUP_FLUX, (Vector2){ center.x + 260.0f, center.y }, 1, WORLD_SHOP_FLUX_COST);
        GameSetMessage(game, "Negozio: tocca un oggetto per comprarlo.");
    }
    else if (room->kind == ROOM_BOSS && room->cleared)
    {
        EntitiesAddPickup(game, PICKUP_EXIT, (Vector2){ center.x + 70.0f, center.y }, 0, 0);
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Portale finale riaperto." : "Portale per il prossimo piano riaperto.");
    }
    else
    {
        GameSetMessage(game, "Scegli una porta.");
    }
    /* DEC-170: entrare in una stanza NON e' un movimento di telecamera -- si
       riparte dall'inquadratura giusta, senza scivolate. */
    WorldSnapCamera(game);
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
    WorldGenerateFloorMap(game);   /* fissa game->roomX/roomY e la FORMA di ogni stanza (DEC-170) */
    game->player.pos = WorldRoomCenter(game);
    WorldSpawnRoomContents(game);
}

void WorldTryEnterRoom(Game *game, int dir)
{
    /* DEC-170: si esce dalla CELLA in cui si trova il giocatore, non "dalla
       stanza": una stanza multi-cella ha porte su piu' celle, e quale si
       attraversi dipende da dove si e' fermi. */
    int cx, cy;
    WorldPlayerCell(game, &cx, &cy);
    RoomState *cell = &game->rooms[cy][cx];
    if (!cell->doors[dir]) return;
    if (GameRoomIsLocked(game))
    {
        GameSetMessage(game, "Porte bloccate: elimina i nemici.");
        return;
    }

    int nx = cx + DirDx(dir);
    int ny = cy + DirDy(dir);
    if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) return;
    RoomState *next = WorldRoomAtMutable(game, nx, ny);
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

    /* La cella di STATO della stanza di arrivo diventa "la stanza corrente":
       vedi il commento su Game.roomX in core/game_types.h. */
    WorldRoomStateCell(game, nx, ny, &game->roomX, &game->roomY);
    /* Si atterra dentro la CELLA di arrivo (non al centro della stanza): gli
       offset di 38px restano quelli di sempre, misurati dalla porta
       attraversata. */
    Rectangle arrival = WorldCellRect(game, nx, ny);
    float acx = arrival.x + arrival.width*0.5f;
    float acy = arrival.y + arrival.height*0.5f;
    if (dir == DIR_UP) game->player.pos = (Vector2){ acx, arrival.y + arrival.height - 38.0f };
    if (dir == DIR_DOWN) game->player.pos = (Vector2){ acx, arrival.y + 38.0f };
    if (dir == DIR_LEFT) game->player.pos = (Vector2){ arrival.x + arrival.width - 38.0f, acy };
    if (dir == DIR_RIGHT) game->player.pos = (Vector2){ arrival.x + 38.0f, acy };
    /* La porta che si apre (audio-and-feedback.md): da questo punto in poi
       la transizione e' gia' impegnata per costruzione (tutte le guardie
       sopra -- doors[dir], porte bloccate, fuori griglia, stanza non
       esistente, chiave mancante -- sono gia' passate). */
    AudioPlaySfx(AUDIO_SFX_DOOR_OPEN);

    /* DEC-152: un vero cambio stanza -- si scartano silenziosamente le card di
       scoperta ancora IN CODA (non ancora mostrate) prima che
       WorldSpawnRoomContents ne accodi eventualmente di nuove per la stanza
       di arrivo: nessuna coda che insegue il giocatore da una stanza
       all'altra. */
    GameDiscardPendingDiscoveries(game);
    WorldSpawnRoomContents(game);
}

void WorldHandleTransitions(Game *game, Vector2 move)
{
    /* DEC-170: la geometria di trigger e' quella della CELLA in cui si trova il
       giocatore (per una 1x1 la cella E' la stanza: comportamento invariato).
       Premere contro un lato INTERNO -- verso un'altra cella della stessa
       stanza -- non fa nulla: quella cella non ha porta da quel lato, si passa
       camminando. */
    int cx, cy;
    WorldPlayerCell(game, &cx, &cy);
    Rectangle cell = WorldCellRect(game, cx, cy);
    float ccx = cell.x + cell.width*0.5f;
    float ccy = cell.y + cell.height*0.5f;
    float cellRight = cell.x + cell.width;
    float cellBottom = cell.y + cell.height;
    float edge = game->player.radius + 7.0f;
    bool pressingTop = move.y < -0.1f && game->player.pos.y <= cell.y + edge && fabsf(game->player.pos.x - ccx) < DOOR_HALF;

    /* Piano 0 (M1b, src/world/floor_zero.c): il varco verso il piano 1 usa la
       STESSA geometria di trigger di una porta normale (il giocatore preme
       contro il muro di fondo), ma non e' una porta -- la stanza hub non ha
       vicini (FloorZeroEnter lascia doors[] tutto falso), quindi passare da
       WorldTryEnterRoom fallirebbe silenziosamente su 'next->exists' falso.
       Qui si segnala solo l'evento (letto e consumato da UpdateApp, mai da
       qui: src/app possiede lo stato della generazione, vedi il commento su
       Game.floorZeroExitCrossed in core/game_types.h). A uscita CHIUSA non
       succede nulla: il varco chiuso e' solido, il clamp del giocatore ai
       bordi della stanza (CombatUpdatePlayer) basta gia' da solo a fermarlo. */
    if (game->floor == 0)
    {
        if (pressingTop && game->floorZeroExitOpen) game->floorZeroExitCrossed = true;
        return;
    }

    if (pressingTop) WorldTryEnterRoom(game, DIR_UP);
    else if (move.y > 0.1f && game->player.pos.y >= cellBottom - edge && fabsf(game->player.pos.x - ccx) < DOOR_HALF) WorldTryEnterRoom(game, DIR_DOWN);
    else if (move.x < -0.1f && game->player.pos.x <= cell.x + edge && fabsf(game->player.pos.y - ccy) < DOOR_HALF) WorldTryEnterRoom(game, DIR_LEFT);
    else if (move.x > 0.1f && game->player.pos.x >= cellRight - edge && fabsf(game->player.pos.y - ccy) < DOOR_HALF) WorldTryEnterRoom(game, DIR_RIGHT);
}

static void WorldSpawnRoomReward(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if (room->rewardTaken) return;
    room->rewardTaken = true;
    Vector2 center = WorldRoomCenter(game);
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
        /* Fase 3 (vedi la vision doc, docs/engineering/specs/2026-07-13-items-synergy-vision.md
           sezioni 1,2,5): il boss lascia SEMPRE l'oggetto stat-up del piano,
           mai uno a caso fra i tre attivi (quelli restano la ricompensa di
           tesoro/negozio, vedi WorldSpawnRoomContents sopra e
           game->content.floors[...].items[...] li'). */
        EntitiesAddItemPickup(game, (Vector2){ center.x - 52.0f, center.y }, game->content.floors[game->floor - 1].bossItem, 0);
        EntitiesAddPickup(game, PICKUP_EXIT, (Vector2){ center.x + 70.0f, center.y }, 0, 0);
        /* DEC-022, prima fonte del catalizzatore di fusione: il drop di boss.
           Una tiratura vera di game->rng va bene QUI (a differenza del banco
           del negozio, vedi WorldShopStocksFlux): questa funzione gira una
           volta sola per stanza, protetta da 'rewardTaken' in cima. */
        if (GameRngRange(&game->rng, 0, 99) < WORLD_BOSS_FLUX_DROP_PERCENT)
            EntitiesAddPickup(game, PICKUP_FLUX, (Vector2){ center.x + 6.0f, center.y - 60.0f }, 1, 0);
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Boss finale sconfitto. Entra nell'uscita." : "Boss sconfitto. Prendi il premio e scendi.");
    }
}

void WorldCheckRoomClear(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if ((room->kind == ROOM_COMBAT || room->kind == ROOM_BOSS) && !room->cleared && WorldNoEnemiesActive(game))
    {
        room->cleared = true;
        /* M7 (substrato del catalogo): il boss di QUESTO piano e' appena
           stato sconfitto -- vedi il commento su Game.bossDefeated in
           core/game_types.h. game->floor e' sempre valido qui (la stanza
           boss esiste solo dentro un piano vero, mai nel Piano 0). */
        if (room->kind == ROOM_BOSS) game->bossDefeated[game->floor - 1] = true;
        /* DEC-059, primo canale di ricarica degli attivi a cariche: la stanza
           completata. Qui e non in WorldSpawnRoomReward perche' non e' una
           ricompensa estratta (nessuna tiratura, nessun 'rewardTaken' che
           possa averla gia' consumata): e' una conseguenza diretta e sempre
           dovuta del completamento. Il dosaggio resta dell'oggetto. */
        ItemActivesGainRoomCharge(&game->player);
        /* DEC-167: la valuta di completamento, qui e non prima, perche'
           'cleared' e' gia' diventato vero (guardia dell'if di sopra:
           entrare/uscire da una stanza gia' ripulita non paga una seconda
           volta). */
        WorldAwardRoomCompletionCurrency(game, room->kind);
        WorldSpawnRoomReward(game);
        if (room->kind != ROOM_BOSS) GameSetMessage(game, "Stanza ripulita.");
    }
}
