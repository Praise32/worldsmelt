#include "world/world.h"

#include "audio/audio.h"
#include "content/run_content.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "game/trials.h"
#include "gameplay/item_pool.h"
#include "gameplay/item_slots.h"
#include "gameplay/item_traits.h"
#include "world/pourhouse.h"
#include "world/room_camera.h"

#include <math.h>
#include <stdio.h>
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
        /* WP4: nome distinto da "crogiolo" sopra -- quel nome resta il tema del
           Piano 0 (ROOM_HUB, DEC-067), questo e' l'archetipo speciale di
           special-rooms.md. Nessuna ambiguita' in-game: le due stanze non
           coesistono mai sulla stessa minimappa (ROOM_HUB vive solo al piano 0,
           mai generato dentro WorldGenerateFloorMap). */
        case ROOM_FUSION: return "fusione";
        /* WP5: stesso motivo del commento su ROOM_FUSION sopra -- nome
           dedicato, non il "vuota" del default, per il quinto archetipo
           speciale (special-rooms.md, "Stanza a tempo"). */
        case ROOM_TIMED: return "a tempo";
        /* WP6: stesso motivo dei due nomi sopra -- l'arena di sfida
           (special-rooms.md) e' un archetipo suo, non una stanza di
           combattimento con piu' nemici. */
        case ROOM_ARENA: return "arena";
        /* WP7: il nome IN-GAME dell'archetipo, fissato da DEC-136
           (governance/glossary.md) -- «Pourhouse», Casa della Colata. A
           differenza di fusione/a tempo/arena qui il nome canonico e' gia'
           quello, non un termine di lavoro italiano da tradurre. */
        case ROOM_POURHOUSE: return "pourhouse";
        /* WP8: l'ultimo dei cinque archetipi speciali (special-rooms.md,
           "Stanza segreta"). Nome dedicato come i quattro sopra. */
        case ROOM_SECRET: return "segreta";
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
   ROOM_SECRET ha un RoomKind dal WP8 e il suo importo sta in world.h
   (WORLD_ROOM_CURRENCY_SECRET), pubblico come quelli di stanza a tempo/arena
   perche' il test dedicato lo verifica contro la fonte vera. La stanza di
   fusione (ROOM_FUSION, WP4) e la stanza a tempo (ROOM_TIMED, WP5) hanno
   invece regole di completamento proprie, viste sotto in
   WorldAwardRoomCompletionCurrency. */
#define WORLD_ROOM_CURRENCY_COMBAT   4
#define WORLD_ROOM_CURRENCY_BOSS    12
#define WORLD_ROOM_CURRENCY_TREASURE 3
#define WORLD_ROOM_CURRENCY_SHOP     2
/* WORLD_ROOM_CURRENCY_TIMED e WORLD_TIMED_ROOM_MIN_FLOOR sono definite in
   world.h (pubbliche): il test dedicato le usa direttamente, vedi il
   commento li'. */

/* WP5: soglia di tempo (in secondi, misurata dall'INGRESSO NEL PIANO --
   Game.floorEntryElapsedSeconds, mai dall'inizio della run) per ottenere la
   ricompensa della stanza a tempo. BASE + un contributo PER CELLA della
   taglia vera del piano (Game.floorCellCount): un piano piu' grande da'
   comprensibilmente piu' tempo per raggiungerla. Default proposto
   dall'implementazione (stile DEC-019): i numeri restano da confermare col
   playtest (governance/open-questions.md, voce 3). */
#define WORLD_TIMED_ROOM_THRESHOLD_BASE_SECONDS 40.0f
#define WORLD_TIMED_ROOM_THRESHOLD_PER_CELL_SECONDS 6.0f

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

/* Crust (in-game, DEC-008): systems/health-and-resources.md fissa la
   COMPOSIZIONE e l'ORDINE DI CONSUMO della salute temporanea/protettiva ma
   non una fonte concreta in-run (WP2, gap G1 del dominio
   combattimento-e-salute). DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile
   DEC-019, registrato in health-and-resources.md e
   governance/open-questions.md): il negozio, stessa tecnica hash-based di
   WorldShopStocksFlux sopra (mai game->rng, cosi' uscire/rientrare non
   fa comparire/sparire la scorta) e stesso ordine di grandezza del costo
   Flux -- una spesa consapevole, non un pickup gratuito di stanza. */
#define WORLD_SHOP_CRUST_STOCK_PERCENT 40
#define WORLD_SHOP_CRUST_COST 25

void WorldAwardRoomCompletionCurrency(Game *game, RoomKind kind)
{
    int amount;
    switch (kind)
    {
        case ROOM_COMBAT:   amount = WORLD_ROOM_CURRENCY_COMBAT;   break;
        case ROOM_BOSS:     amount = WORLD_ROOM_CURRENCY_BOSS;     break;
        case ROOM_TREASURE: amount = WORLD_ROOM_CURRENCY_TREASURE; break;
        case ROOM_SHOP:     amount = WORLD_ROOM_CURRENCY_SHOP;     break;
        /* WP4: la stanza di fusione non ha una condizione di "ripulita" come
           combattimento/tesoro/negozio -- resta sempre visitabile (Scenario 4
           di special-rooms.md), quindi non ha un momento di completamento a
           cui agganciare una valuta. Nessuna DEC la richiede: cade qui sotto
           esplicitamente, non nel default, cosi' la scelta si legge. */
        case ROOM_FUSION:   return;
        /* WP5: qui SOLO quando la stanza e' stata raggiunta in tempo --
           WorldSpawnRoomContents non chiama questa funzione affatto quando la
           soglia e' scaduta (nessuna doppia via per "nessun bonus"). */
        case ROOM_TIMED:    amount = WORLD_ROOM_CURRENCY_TIMED;    break;
        /* WP6: la condizione di completamento dell'arena e' "sfida ACCETTATA e
           vinta" (WorldCheckRoomClear, che chiama qui solo in quel caso) --
           attraversarla senza accettare non e' un completamento e non paga
           nulla, coerente con "ripulita secondo la PROPRIA condizione"
           (DEC-167). Il doppio di un combattimento: rischio scelto, premio
           maggiore (rewards-and-economy.md, "Pattern rischio/ricompensa
           dell'arena di sfida"). */
        case ROOM_ARENA:    amount = WORLD_ROOM_CURRENCY_ARENA;    break;
        /* WP7: la Pourhouse non ha una condizione di "ripulita" -- accettare o
           rifiutare una puntata e' uno SCAMBIO, non un completamento, e il
           documento non le assegna alcuna ricompensa di stanza (il guadagno e'
           l'offerta stessa, gia' pagata col prezzo). Stessa scelta esplicita
           della stanza di fusione sopra: cade qui e non nel default, cosi' si
           legge che e' una decisione e non una dimenticanza. */
        case ROOM_POURHOUSE: return;
        /* WP8: rewards-and-economy.md elenca esplicitamente "una stanza
           segreta quando e' stata trovata" fra le condizioni di completamento
           di DEC-167 -- e' l'unica riga di quella lista che fino al WP8 non
           aveva un punto d'innesto nel motore. "Trovata" = ci si e' entrati la
           prima volta (WorldSpawnRoomContents, firstVisit): il varco era gia'
           stato aperto un istante prima, e aprirlo E' il completamento di
           questo archetipo. Stesso importo per la segreta normale e per la
           super-segreta: la ricompensa superiore della seconda e' il
           catalizzatore di fusione, non piu' valuta. */
        case ROOM_SECRET:   amount = WORLD_ROOM_CURRENCY_SECRET; break;
        default: return;   /* hub/start/vuota/non ancora implementato: nessuna valuta */
    }
    game->player.coins += amount;
}

/* WP5: la soglia di tempo della stanza a tempo (vedi le due costanti sopra),
   proporzionata alla taglia VERA del piano corrente. Se chiamata prima che
   WorldGenerateFloorMap abbia scritto Game.floorCellCount (non dovrebbe mai
   succedere: la stanza a tempo esiste solo dentro un piano gia' generato) si
   ripiega su una stima dal solo numero di piano, mai su zero -- zero
   renderebbe la soglia impossibile da rispettare, l'esatto contrario del
   default "innocuo" per una soglia (disciplina zero-default). */
float WorldTimedRoomThresholdSeconds(const Game *game)
{
    int cells = game->floorCellCount > 0 ? game->floorCellCount : (6 + game->floor);
    return WORLD_TIMED_ROOM_THRESHOLD_BASE_SECONDS
         + WORLD_TIMED_ROOM_THRESHOLD_PER_CELL_SECONDS*(float)cells;
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
    /* DEC-180 (30/07): superato il default per-cella delle forme a L. Ora
       TUTTE le taglie maggiori -- L compresa -- clampano al riquadro
       dell'INTERA stanza (il blocco 2x2 che la contiene): la telecamera segue
       in continuo, senza salti al cambio di cella. L'angolo mancante di una L
       puo' quindi entrare in inquadratura: da W8 il tileset lo rende come
       muro/sfondo, quindi il vincolo di DEC-170 ("mai area fuori dalla
       stanza") resta soddisfatto dal rendering del vuoto, non piu' dal
       clamp per cella. */
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

/* WP6: l'arena chiude le porte SOLO a sfida accettata (room->arenaActive) --
   e' esattamente il cuore dell'opzionalita' dell'archetipo: finche' il
   giocatore non conferma, la stanza si attraversa come una stanza vuota
   (special-rooms.md, "Casi limite": mai un passaggio obbligato, mai un blocco
   se ignorata). Da li' in poi vale lo stesso pattern di ROOM_COMBAT/ROOM_BOSS,
   nessuna regola nuova: porte chiuse finche' non e' 'cleared'. */
bool GameRoomIsLocked(const Game *game)
{
    const RoomState *room = GameCurrentRoom(game);
    bool gated = (room->kind == ROOM_COMBAT || room->kind == ROOM_BOSS) ||
                 (room->kind == ROOM_ARENA && room->arenaActive);
    return gated && !room->cleared && !WorldNoEnemiesActive(game);
}

/* WP8 (systems/special-rooms.md, "Stanza segreta"): i due predicati PURI che
   decidono cosa il giocatore puo' VEDERE di una stanza segreta. Vivono qui e
   non nel renderer per un motivo preciso: sono garanzie verificabili senza
   aprire una finestra (il test dedicato li chiama direttamente), e "non
   compare sulla mappa" / "la super-segreta non ha alcun indizio" sono
   affermazioni di DESIGN, non dettagli di disegno. */
bool WorldRoomHiddenOnMap(const RoomState *room)
{
    if (!room) return false;
    return room->kind == ROOM_SECRET && !room->secretOpened;
}

bool WorldSecretClueVisible(const RoomState *room)
{
    if (!room) return false;
    /* DEC-025: l'indizio esiste SOLO per la segreta di livello normale e solo
       finche' il varco e' murato. La super-segreta non ne ha mai uno -- e'
       tutta la sua definizione, e il documento vieta esplicitamente di
       introdurne uno implicito ("Regole per contenuti generati"). */
    return room->kind == ROOM_SECRET && !room->secretOpened && !room->secretSuper;
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

/* DEC-182: quante stanze ESISTENTI distinte toccano il perimetro della forma
   'cells' ancorata a (ox,oy) -- e' il grado che quella forma avrebbe nel
   grafo di adiacenza del piano una volta aperte le porte (WorldLinkRooms
   apre sempre esattamente una porta per coppia di stanze adiacenti, DEC-181,
   quindi il grado nel grafo coincide col numero di stanze vicine distinte,
   non col numero di coppie di celle adiacenti). Le celle della stessa forma
   non contano come "vicine": sono lo stesso spazio continuo. Le stanze
   distinte si contano tramite la CELLA DI STATO risolta da WorldRoomAt, MAI
   confrontando originX/originY grezzi: due stanze diverse possono avere lo
   stesso valore numerico di origine quando la maschera di una di esse non
   include il bit (0,0) (l'origine e' allora solo un ancoraggio geometrico,
   non la cella di stato) -- originX/originY da soli non sono un identificativo
   univoco di stanza, la cella di stato risolta lo e' sempre. */
static int WorldShapeNeighborRoomCount(const Game *game, int ox, int oy, unsigned char cells)
{
    const RoomState *seenRooms[8];
    int n = 0;
    for (int i = 0; i < 4; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        int cx = ox + CellBitDx(i), cy = oy + CellBitDy(i);
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + DirDx(d), ny = cy + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            bool ownCell = false;
            for (int j = 0; j < 4; j++)
            {
                if (!(cells & (unsigned char)(1u << j))) continue;
                if (ox + CellBitDx(j) == nx && oy + CellBitDy(j) == ny) { ownCell = true; break; }
            }
            if (ownCell) continue;
            const RoomState *nb = WorldRoomAt(game, nx, ny);
            bool seen = false;
            for (int k = 0; k < n; k++)
                if (seenRooms[k] == nb) { seen = true; break; }
            if (!seen && n < 8) { seenRooms[n] = nb; n++; }
        }
    }
    return n;
}

/* Come WorldPlaceShapeAt, ma piazza solo se la forma tocca ESATTAMENTE una
   stanza esistente distinta (DEC-182: la stanza boss e' sempre foglia del
   grafo). Provare piu' orientamenti (il ciclo 'rot' gia' presente) da' piu'
   occasioni di trovare un incastro che tocchi una sola stanza, prima di
   scartare la posizione. */
static RoomState *WorldPlaceShapeAtLeaf(Game *game, int nx, int ny, unsigned char cells, RoomKind kind, int rot)
{
    for (int k = 0; k < 4; k++)
    {
        int i = (rot + k)%4;
        if (!(cells & (unsigned char)(1u << i))) continue;
        int ox = nx - CellBitDx(i);
        int oy = ny - CellBitDy(i);
        if (!WorldShapeFits(game, ox, oy, cells)) continue;
        if (WorldShapeNeighborRoomCount(game, ox, oy, cells) != 1) continue;
        return WorldWriteRoom(game, ox, oy, cells, kind);
    }
    return NULL;
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
   prima cella libera entra sempre. DEC-182: ogni tentativo di incastro deve
   anche toccare una sola stanza esistente (WorldPlaceShapeAtLeaf) -- il boss
   e' sempre una foglia del grafo di adiacenza, mai un nodo di passaggio;
   questo puo' far scendere di classe piu' spesso di prima (una 2x2 ha piu'
   perimetro, quindi piu' occasioni di toccare due stanze diverse), ma la
   ricerca esaurisce comunque tutte le celle candidate e i quattro
   orientamenti prima di rinunciare a una taglia. */
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
                if (WorldPlaceShapeAtLeaf(game, nx, ny, kBossShapes[s], ROOM_BOSS, rot)) return true;
            }
        }
    }
    return false;
}

/* WP6: vero se la forma 'cells' ancorata a (ox,oy) tocca una stanza che DEVE
   restare foglia del grafo -- la stanza boss (DEC-182) e l'arena di sfida
   (special-rooms.md: mai un passaggio obbligato). Attaccarsi a una delle due
   le regalerebbe una seconda porta e romperebbe la garanzia. Il tipo vive solo
   sulla cella di stato, quindi si legge SEMPRE da WorldRoomAt, mai dal campo
   '.kind' grezzo della cella vicina.
   WP8: la stanza SEGRETA entra nella stessa lista, e per una ragione anche
   piu' stretta delle due sopra -- il suo unico muro condiviso E' il varco
   murato. Una stanza piazzata dopo di lei e attaccata al suo secondo lato le
   darebbe una porta NORMALE, cioe' un modo di entrarci senza sbrecciare
   niente: il segreto smetterebbe di essere un segreto. */
static bool WorldShapeTouchesLeafRoom(const Game *game, int ox, int oy, unsigned char cells)
{
    for (int i = 0; i < 4; i++)
    {
        if (!(cells & (unsigned char)(1u << i))) continue;
        int cx = ox + CellBitDx(i), cy = oy + CellBitDy(i);
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + DirDx(d), ny = cy + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            RoomKind k = WorldRoomAt(game, nx, ny)->kind;
            if (k == ROOM_BOSS || k == ROOM_ARENA || k == ROOM_SECRET) return true;
        }
    }
    return false;
}

/* WP6 (systems/special-rooms.md, "Arena di sfida"): l'arena NON passa da
   WorldPlaceSpecialRoom (sotto), che resta a quattro chiamanti tutti 1x1.
   Due motivi, entrambi di design e non di comodo:
     - E' una stanza di COMBATTIMENTO, la piu' impegnativa del piano dopo il
       boss: una 1x1 stretta la mortificherebbe (nessuno spazio per schivare
       un'ondata maggiorata). Si provano le taglie GRANDI per prime -- 2x2, poi
       le quattro L, poi 1x2/2x1 -- e non si scende MAI sotto le due celle:
       meglio nessuna arena su questo piano che un'arena che non si puo'
       giocare. DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019: il
       documento non fissa la taglia di questo archetipo).
     - Deve essere una FOGLIA del grafo di adiacenza, esattamente come la
       stanza boss (WorldPlaceShapeAtLeaf sopra usa lo stesso predicato di
       grado): e' il modo STRUTTURALE -- verificabile con un test, non con una
       dichiarazione -- di garantire il caso limite del documento, "l'arena non
       deve mai essere un passaggio obbligato ne' bloccare il piano se
       ignorata". Se fosse un nodo di passaggio, accettare la sfida (che chiude
       le porte) taglierebbe in due il piano.
   Si piazza DOPO il boss, e ogni incastro deve toccare esattamente una stanza
   esistente che NON sia il boss (WorldShapeTouchesLeafRoom): attaccarsi al
   boss gli darebbe la seconda porta che DEC-182 vieta. Un solo tentativo per
   piano, non garantito: come per gli altri speciali, una griglia satura lascia
   il piano senza arena -- ed e' innocuo, l'arena e' opzionale per definizione. */
static bool WorldPlaceArenaRoom(Game *game, const int *orderX, const int *orderY, int orderCount)
{
    static const unsigned char kArenaShapes[] = {
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0) | ROOM_CELL_BIT(0, 1) | ROOM_CELL_BIT(1, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(1, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(0, 1)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(1, 0)),
        (unsigned char)(0x0Fu & ~ROOM_CELL_BIT(0, 0)),
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(1, 0)),
        (unsigned char)(ROOM_CELL_BIT(0, 0) | ROOM_CELL_BIT(0, 1))
        /* Nessuna 1x1 in coda, di proposito: vedi il commento sopra. */
    };
    const int shapeCount = (int)(sizeof(kArenaShapes)/sizeof(kArenaShapes[0]));
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
                for (int j = 0; j < 4; j++)
                {
                    int i = (rot + j)%4;
                    if (!(kArenaShapes[s] & (unsigned char)(1u << i))) continue;
                    int ox = nx - CellBitDx(i), oy = ny - CellBitDy(i);
                    if (!WorldShapeFits(game, ox, oy, kArenaShapes[s])) continue;
                    if (WorldShapeNeighborRoomCount(game, ox, oy, kArenaShapes[s]) != 1) continue;
                    if (WorldShapeTouchesLeafRoom(game, ox, oy, kArenaShapes[s])) continue;
                    WorldWriteRoom(game, ox, oy, kArenaShapes[s], ROOM_ARENA);
                    return true;
                }
            }
        }
    }
    return false;
}

/* DEC-182: le stanze speciali 1x1 (tesoro, negozio, la stanza di fusione dal
   WP4, la stanza a tempo dal WP5 e la Pourhouse dal WP7, CINQUE chiamanti --
   l'arena di sfida del WP6 NON passa di qui, ha un piazzamento suo, vedi
   WorldPlaceArenaRoom sopra)
   si piazzano DOPO il boss (sotto), quindi NON devono mai attaccarsi alla
   stanza boss -- le farebbero guadagnare una seconda porta, rompendo la
   foglia. WP6 estende la stessa regola all'arena, che per lo stesso motivo
   deve restare anch'essa una foglia: si scarta l'intera cella candidata se
   anche solo UNO dei suoi vicini esistenti e' il boss o l'arena (il tipo vive
   solo sulla cella di stato, WorldRoomAt lo risolve sempre); altrimenti si
   piazza appena si trova un vicino qualsiasi, come prima. */
static void WorldPlaceSpecialRoom(Game *game, RoomKind kind)
{
    int tries = 120;
    while (tries-- > 0)
    {
        int x = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        int y = GameRngRange(&game->rng, 0, GRID_SIZE - 1);
        if (!WorldCellFree(game, x, y)) continue;
        bool touchesAny = false;
        for (int d = 0; d < 4; d++)
        {
            int nx = x + DirDx(d);
            int ny = y + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            touchesAny = true;
            break;
        }
        if (!touchesAny) continue;
        if (WorldShapeTouchesLeafRoom(game, x, y, ROOM_CELL_BIT(0, 0))) continue;
        /* Le stanze speciali di questo cammino (tesoro, negozio, fusione,
           stanza a tempo e Pourhouse) restano 1x1 (default proposto): una
           funzione sola, tutta visibile appena si entra -- e' proprio il caso
           in cui la telecamera fissa di DEC-170 e' un pregio. Vale anche per
           la Pourhouse (WP7): leggere una puntata e decidere non ha bisogno di
           spazio, ha bisogno che il banco sia sotto gli occhi. */
        RoomState *state = WorldWriteRoom(game, x, y, ROOM_CELL_BIT(0, 0), kind);
        state->cleared = true;
        return;
    }
}

/* WP8 (systems/special-rooms.md "Stanza segreta" + systems/secrets-and-obstacles.md
   "Segreti", DEC-025): la stanza segreta NON passa da WorldPlaceSpecialRoom
   sopra. Tre differenze, tutte di sostanza:

   (1) UNA SOLA CELLA VICINA, non "almeno una". WorldPlaceSpecialRoom si
       accontenta di toccare qualcosa; qui la cella candidata deve toccare
       ESATTAMENTE UNA cella esistente. E' cio' che rende la segreta una foglia
       per costruzione (mai un passaggio) e le da' UN SOLO muro condiviso,
       cioe' un solo punto dove l'indizio ha senso e un solo varco da
       sbrecciare. Con due muri condivisi si dovrebbe decidere quale disegnare
       e quale apre: una complicazione senza alcun guadagno di design.

   (2) LA VICINA DEVE ESSERE UNA STANZA NORMALE (partenza o combattimento) --
       "adiacente a una stanza normale del piano" del documento. Boss e arena
       sono escluse perche' devono restare foglie (DEC-182 e special-rooms.md:
       una seconda porta le romperebbe); le altre speciali 1x1 sono escluse per
       leggibilita' -- un segreto dietro la parete di un negozio o, peggio, di
       una stanza tesoro (che chiede una chiave per entrarci) sposterebbe il
       costo del segreto su una risorsa che non c'entra nulla con lo strumento
       di breccia. Un'altra segreta e' esclusa per lo stesso motivo di (1).

   (3) STREAM DETERMINISTICO LOCALE, mai game->rng. WorldGenerateFloorMap gira
       una volta sola per piano, quindi game->rng sarebbe stato corretto quanto
       lo e' per la Pourhouse; lo stream locale serve a un'altra cosa --
       lasciare INTATTO il flusso di estrazioni di tutti i piazzamenti gia'
       misurati (boss, arena, le cinque 1x1) e delle porte di WorldLinkRooms.
       Aggiungere estrazioni a game->rng qui avrebbe cambiato la forma di ogni
       piano gia' generato a parita' di seed: un segreto non deve riscrivere il
       mondo attorno a se'. Il seme e' composto da runSeed + piano + una
       costante di dominio, esattamente come il seme per-cella degli ostacoli
       (WorldBuildObstacles): stesso seed di run => stesso piano => stessa
       segreta, sempre.

   Un solo tentativo per livello, non garantito: come per ogni altra speciale,
   una griglia satura lascia il piano senza segreta -- ed e' innocuo per
   costruzione, la segreta e' fuori dalla connettivita' del piano. */
static bool WorldPlaceSecretRoom(Game *game, unsigned int *stream, bool super)
{
    int tries = 240;
    while (tries-- > 0)
    {
        int x = GameRngRange(stream, 0, GRID_SIZE - 1);
        int y = GameRngRange(stream, 0, GRID_SIZE - 1);
        if (!WorldCellFree(game, x, y)) continue;

        int adjacent = 0;
        int ax = -1, ay = -1;
        for (int d = 0; d < 4; d++)
        {
            int nx = x + DirDx(d), ny = y + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            adjacent++;
            ax = nx; ay = ny;
        }
        if (adjacent != 1) continue;

        /* Il tipo vive SOLO sulla cella di stato: si legge sempre da
           WorldRoomAt, mai dal campo '.kind' grezzo della cella vicina. */
        RoomKind neighbour = WorldRoomAt(game, ax, ay)->kind;
        if (neighbour != ROOM_START && neighbour != ROOM_COMBAT) continue;

        RoomState *state = WorldWriteRoom(game, x, y, ROOM_CELL_BIT(0, 0), ROOM_SECRET);
        /* 'cleared' come ogni altra speciale non-combattiva: dentro non c'e'
           nulla da sconfiggere, e le porte non devono mai risultare bloccate
           (GameRoomIsLocked non elenca ROOM_SECRET, ma restare coerenti con
           tesoro/negozio/fusione costa una riga). */
        state->cleared = true;
        state->secretSuper = super;
        return true;
    }
    return false;
}

/* WP8: vero se la coppia di celle adiacenti (ax,ay)-(bx,by) e' il VARCO MURATO
   di una stanza segreta non ancora sbrecciata. WorldLinkRooms lo usa per NON
   aprire mai quella porta in generazione: e' il muro che rende segreta la
   stanza, e la porta comparira' solo dallo strumento di breccia
   (WorldTryBreachSecretWall). Conseguenza dichiarata e voluta: la segreta non
   entra nella connettivita' del piano, quindi il piano resta completabile
   ignorandola del tutto. */
static bool WorldSecretWallBetween(const Game *game, int ax, int ay, int bx, int by)
{
    return WorldRoomHiddenOnMap(WorldRoomAt(game, ax, ay)) ||
           WorldRoomHiddenOnMap(WorldRoomAt(game, bx, by));
}

/* Un segmento di confine: la cella (ax,ay) e la sua vicina in direzione
   'dir', di un'ALTRA stanza. WorldLinkRooms scandisce ogni confine fisico UNA
   sola volta (solo DESTRA/BASSO, mai il verso opposto), poi raggruppa i
   segmenti per coppia di stanze per applicare DEC-181. */
typedef struct WorldDoorSeg
{
    int ax, ay;
    int dir;
} WorldDoorSeg;

/* Una porta collega due celle ADIACENTI di stanze DIVERSE. Due celle della
   stessa stanza non hanno porta fra loro: sono lo stesso spazio continuo, il
   giocatore ci passa camminando (DEC-170). DEC-181: quando due stanze
   condividono piu' di una coppia di celle adiacenti sul confine, si apre
   UNA sola porta per la coppia, nel segmento piu' centrale -- mai una porta
   per ogni coppia di celle, mai porte multiple affiancate. Le stanze restano
   al massimo un blocco 2x2 (o una L nello stesso riquadro), quindi due
   stanze non condividono mai piu' di due coppie di celle adiacenti sullo
   stesso confine: con una sola coppia si apre quella (e' gia' il centro);
   con due, nessuna e' piu' centrale dell'altra (confine di lunghezza pari,
   nessun centro esatto), e la scelta fra le due e' deterministica dal seed
   del piano (stessa run, stesso seed, stessa porta). */
static void WorldLinkRooms(Game *game)
{
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            for (int d = 0; d < 4; d++)
                game->rooms[y][x].doors[d] = false;

    WorldDoorSeg segs[GRID_SIZE*GRID_SIZE*2];
    int segCount = 0;
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!game->rooms[y][x].exists) continue;
            static const int kFwdDirs[2] = { DIR_RIGHT, DIR_DOWN };
            for (int k = 0; k < 2; k++)
            {
                int d = kFwdDirs[k];
                int nx = x + DirDx(d), ny = y + DirDy(d);
                if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                if (!game->rooms[ny][nx].exists) continue;
                if (WorldSameRoom(game, x, y, nx, ny)) continue;
                /* WP8: il varco murato di una stanza segreta non e' un
                   segmento di confine come gli altri -- non genera porta qui,
                   la generera' lo strumento di breccia. */
                if (WorldSecretWallBetween(game, x, y, nx, ny)) continue;
                segs[segCount].ax = x; segs[segCount].ay = y; segs[segCount].dir = d;
                segCount++;
            }
        }
    }

    bool used[GRID_SIZE*GRID_SIZE*2];
    for (int i = 0; i < segCount; i++) used[i] = false;

    for (int i = 0; i < segCount; i++)
    {
        if (used[i]) continue;
        const RoomState *ra = WorldRoomAt(game, segs[i].ax, segs[i].ay);
        int nx0 = segs[i].ax + DirDx(segs[i].dir), ny0 = segs[i].ay + DirDy(segs[i].dir);
        const RoomState *rb = WorldRoomAt(game, nx0, ny0);

        int group[GRID_SIZE*GRID_SIZE*2];
        int groupCount = 0;
        group[groupCount++] = i;
        used[i] = true;
        for (int j = i + 1; j < segCount; j++)
        {
            if (used[j]) continue;
            const RoomState *ra2 = WorldRoomAt(game, segs[j].ax, segs[j].ay);
            int nx1 = segs[j].ax + DirDx(segs[j].dir), ny1 = segs[j].ay + DirDy(segs[j].dir);
            const RoomState *rb2 = WorldRoomAt(game, nx1, ny1);
            bool samePair = (ra2 == ra && rb2 == rb) || (ra2 == rb && rb2 == ra);
            if (!samePair) continue;
            group[groupCount++] = j;
            used[j] = true;
        }

        /* Ordine stabile lungo il confine (somma delle coordinate della
           cella sorgente: per un confine dritto e' monotona), cosi' la
           scelta fra i due segmenti centrali e' sempre nello stesso ordine a
           parita' di seed. */
        for (int a = 1; a < groupCount; a++)
        {
            int key = segs[group[a]].ax + segs[group[a]].ay;
            int b = a - 1;
            int val = group[a];
            while (b >= 0 && (segs[group[b]].ax + segs[group[b]].ay) > key)
            {
                group[b + 1] = group[b];
                b--;
            }
            group[b + 1] = val;
        }

        int chosen;
        if (groupCount == 1)
        {
            chosen = group[0];
        }
        else
        {
            int lo = (groupCount - 1)/2;
            int hi = groupCount/2;
            chosen = (lo == hi) ? group[lo] : group[lo + GameRngRange(&game->rng, 0, 1)];
        }

        int cx = segs[chosen].ax, cy = segs[chosen].ay, cd = segs[chosen].dir;
        int cnx = cx + DirDx(cd), cny = cy + DirDy(cd);
        game->rooms[cy][cx].doors[cd] = true;
        game->rooms[cny][cnx].doors[(cd + 2)%4] = true;
    }
}

static void WorldGenerateFloorMap(Game *game)
{
    memset(game->rooms, 0, sizeof(game->rooms));
    /* DEC-183: un Innesto lasciato sul piano precedente non segue il
       giocatore -- default proposto DEC-183 (i piani si attraversano in un
       solo verso in questa demo). Azzerato ESPLICITAMENTE qui, non per un
       effetto collaterale del memset sopra (campo diverso, vedi il commento
       su Game.droppedGrafts in core/game_types.h). */
    memset(game->droppedGrafts, 0, sizeof(game->droppedGrafts));
    /* WP3: la persistenza dei distruttibili spaccati (Game.destroyedObstacleMask)
       e' per-PIANO come Game.droppedGrafts appena sopra -- stesso motivo (le
       coordinate di cella hanno senso solo dentro il piano che le ha generate)
       e stesso schema di azzeramento esplicito, separato dal memset di
       game->rooms. */
    memset(game->destroyedObstacleMask, 0, sizeof(game->destroyedObstacleMask));
    /* WP7: la puntata della Pourhouse e' un fatto del PIANO (porta con se' le
       coordinate della stanza che l'ha composta), quindi si azzera qui come
       droppedGrafts/destroyedObstacleMask sopra e per lo stesso motivo --
       ereditarla nel piano nuovo significherebbe trovare al banco la puntata
       di un'altra stanza. Game.pourhouseLastSignature invece NON si tocca: e'
       un fatto della RUN, ed e' proprio cio' che fa proporre puntate diverse a
       due Pourhouse successive (special-rooms.md, Scenario 8). */
    memset(&game->pourhouse, 0, sizeof(game->pourhouse));
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
        /* Griglia satura: come ultima rete si promuove a boss una stanza gia'
           esistente. Un piano SENZA stanza boss non e' completabile, quindi
           qui non si esce mai a mani vuote se esiste almeno una stanza oltre
           la partenza. DEC-182 vale anche qui: si preferisce, fra le stanze
           gia' piazzate (in ordine di distanza DECRESCENTE dalla partenza,
           come sopra), la piu' lontana che sia gia' una foglia del grafo
           (grado <=1); se nessuna lo e' (griglia davvero satura), si ripiega
           deterministicamente sulla stanza di grado minimo disponibile -- il
           meglio che la geometria del piano permetta. */
        RoomState *chosen = NULL;
        int bestDeg = 1000;
        for (int c = 0; c < orderCount; c++)
        {
            RoomState *cand = WorldRoomAtMutable(game, orderX[c], orderY[c]);
            if (cand->kind == ROOM_START) continue;
            int deg = WorldShapeNeighborRoomCount(game, cand->originX, cand->originY, cand->cells);
            if (deg < bestDeg)
            {
                bestDeg = deg;
                chosen = cand;
                if (deg <= 1) break;
            }
        }
        if (chosen)
        {
            chosen->kind = ROOM_BOSS;
            chosen->cleared = false;
            chosen->visited = false;
        }
    }

    /* WP6 (systems/special-rooms.md, "Arena di sfida"): PRIMA delle quattro
       speciali 1x1 sotto e subito DOPO il boss. L'ordine e' una scelta
       misurata, non estetica: l'arena e' l'unica di queste cinque che ha
       bisogno di celle libere CONTIGUE (mai meno di due), e una griglia 5x5
       gia' cresciuta si frammenta in fretta -- piazzata per ultima trova posto
       in 17 piani candidati su 96, piazzata qui in 82 su 96 (misure di
       --rooms-test). Il prezzo lo pagano le speciali 1x1, che perdono qualche
       cella libera (la stanza di fusione scende da 119/120 a 101/120, la
       stanza a tempo da 69/72 a 40/72): entrambe restano frequenti e nessuna
       delle due e' necessaria a completare un piano (la fusione ha anche
       l'accesso globale TAB/PauseMenu come rete di sicurezza dichiarata dal
       WP4). Dopo il boss perche' deve poterlo riconoscere per non
       attaccarcisi. Solo dai piani >= WORLD_ARENA_ROOM_MIN_FLOOR: DEFAULT
       PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019, vedi il commento sulle
       costanti in world.h) -- il documento fissa l'archetipo, non la
       frequenza. Un solo tentativo, non garantito: l'arena e' opzionale per
       definizione, un piano senza arena non perde nulla di necessario. */
    if (game->floor >= WORLD_ARENA_ROOM_MIN_FLOOR)
        WorldPlaceArenaRoom(game, orderX, orderY, orderCount);
    WorldPlaceSpecialRoom(game, ROOM_TREASURE);
    WorldPlaceSpecialRoom(game, ROOM_SHOP);
    /* WP8 (systems/special-rooms.md, "Stanza segreta", DEC-025): DOPO boss,
       arena, tesoro e negozio; PRIMA di fusione, stanza a tempo e Pourhouse.
       Ogni pezzo di quest'ordine e' una scelta misurata su --rooms-test, non
       estetica:
       - MAI PRIMA DEL BOSS. Il boss si piazza dove tocca UNA sola stanza
         esistente: se una segreta fosse gia' sulla griglia, potrebbe essere
         proprio quella, e il boss diventerebbe raggiungibile SOLO sbrecciando
         un muro -- il piano non sarebbe piu' completabile ignorando i segreti,
         cioe' l'esatto contrario della garanzia che questo archetipo deve
         rispettare.
       - Dopo l'arena perche' anche lei deve restare foglia
         (special-rooms.md) e la segreta non deve toccarla; dopo tesoro e
         negozio perche' sono i due servizi di piano piu' importanti e non e'
         giusto che paghino per primi il prezzo di questo archetipo.
       - Prima di fusione/stanza a tempo/Pourhouse perche' la segreta ha un
         vincolo di posizione MOLTO piu' stretto di tutte loro: le serve una
         cella libera che tocchi ESATTAMENTE UNA cella esistente (un solo muro
         condiviso). Su una griglia 5x5 gia' cresciuta quelle celle sono poche
         e spariscono in fretta -- piazzata per ULTIMA trovava posto in 23
         piani su 120 e la super-segreta in 0 su 96 (cioe' un intero livello
         di DEC-025 non sarebbe mai esistito nel gioco vero); qui la normale
         trova posto in 36 piani su 120 e la super in 13 su 96.
       Il prezzo e' dichiarato e misurato, come per l'arena del WP6: la stanza
       di fusione scende da 101/120 a 95/120, la stanza a tempo da 40/72 a
       30/72, la Pourhouse da 27/96 a 17/96. Nessuna delle tre e' necessaria a
       completare un piano (la fusione ha anche l'accesso globale TAB/PauseMenu
       come rete di sicurezza dichiarata dal WP4), e la segreta non e' un
       servizio in piu': e' il contenuto che ripaga chi esplora.

       Le due segrete leggono uno STREAM DETERMINISTICO LOCALE, mai game->rng.
       Non e' una comodita': cosi' l'aggiunta di questo archetipo non sposta di
       un solo bit le ESTRAZIONI di nessun altro piazzamento (a parita' di seed
       i piani cambiano solo per le celle che le segrete occupano davvero, mai
       perche' qualcun altro ha letto numeri diversi). Il seme mescola il seed
       di RUN e il piano con due costanti di dominio diverse per i due livelli,
       cosi' i due non ricalcano mai lo stesso cammino; stessa tecnica del seme
       per-cella degli ostacoli (WorldBuildObstacles).

       ORDINE FRA LE DUE: prima la SUPER, poi la normale. Sembra il contrario
       dell'intuizione (la piu' rara per prima), ed e' invece l'unico ordine
       che le fa esistere entrambe: le celle candidate sono cosi' poche che chi
       va per seconda quasi sempre non ne trova (con la normale per prima, la
       super finiva a 0 su 96). La super resta comunque il livello PIU' RARO --
       e' la sua estrazione al 50% a renderla tale, non l'ordine, e il
       controllo (s) di GameRoomsTest confronta i due conteggi proprio per
       questo. Nessuna delle due si attacca all'altra:
       WorldShapeTouchesLeafRoom include ROOM_SECRET e WorldPlaceSecretRoom
       pretende comunque una vicina di tipo partenza/combattimento. */
    if (game->floor >= WORLD_SECRET_SUPER_MIN_FLOOR)
    {
        unsigned int superStream = game->runSeed ^ ((unsigned int)game->floor*40503u) ^ 0x50FE8E77u;
        if (GameRngRange(&superStream, 0, 99) < WORLD_SECRET_SUPER_CHANCE_PERCENT)
            WorldPlaceSecretRoom(game, &superStream, true);
    }
    if (game->floor >= WORLD_SECRET_ROOM_MIN_FLOOR)
    {
        unsigned int secretStream = game->runSeed ^ ((unsigned int)game->floor*2654435761u) ^ 0x5EC8E7A1u;
        WorldPlaceSecretRoom(game, &secretStream, false);
    }
    /* WP4 (systems/special-rooms.md, "Stanza di fusione"): stesso algoritmo di
       tesoro/negozio sopra -- 1x1, mai adiacente al boss (WorldPlaceSpecialRoom
       scarta le celle candidate che lo toccano), deterministica dal seed del
       piano. Un solo tentativo per piano, non garantito: se la griglia e' satura
       o ogni cella libera tocca solo il boss, il piano resta senza stanza di
       fusione per quel giro -- l'accesso globale (TAB/PauseMenu) resta comunque
       sempre disponibile (rete di sicurezza, vedi il commento su ROOM_FUSION in
       core/game_types.h). DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019):
       il documento lascia esplicitamente aperta la frequenza esatta di ciascun
       archetipo per piano (governance/open-questions.md). */
    WorldPlaceSpecialRoom(game, ROOM_FUSION);
    /* WP5 (systems/special-rooms.md, "Stanza a tempo", DEC-051): stesso
       algoritmo di tesoro/negozio/fusione sopra -- 1x1, mai adiacente al
       boss, deterministica dal seed del piano, un solo tentativo, non
       garantito. Esclusiva dei PIANI AVANZATI (default proposto: dal piano
       3, stesso confine gia' scelto per l'escalation del tileset e i boss a
       due fasi, governance/open-questions.md voce 23) -- nei piani 1-2 non
       si tenta nemmeno il piazzamento, non e' solo un default di frequenza,
       fa parte della decisione stessa (DEC-051, "esclusiva dei piani
       avanzati"). */
    if (game->floor >= WORLD_TIMED_ROOM_MIN_FLOOR) WorldPlaceSpecialRoom(game, ROOM_TIMED);
    /* WP7 (systems/special-rooms.md, "Scambio ad alto rischio", DEC-136): la
       QUINTA chiamante di WorldPlaceSpecialRoom -- stesso algoritmo delle
       altre quattro (1x1, mai adiacente a boss/arena, deterministica dal seed
       del piano). L'unica differenza, ed e' la ragione per cui il tentativo e'
       dentro un 'if' invece che incondizionato come tesoro/negozio/fusione:
       la Pourhouse e' un archetipo RARO, non un servizio di piano. DEFAULT
       PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019, il documento non fissa la
       frequenza di questo archetipo): dai piani
       >= WORLD_POURHOUSE_ROOM_MIN_FLOOR e solo se l'estrazione del piano la
       concede (WORLD_POURHOUSE_ROOM_CHANCE_PERCENT).
       Tiratura vera di game->rng e non un hash del seed come il banco del
       negozio: WorldGenerateFloorMap gira UNA volta sola per piano (uscire e
       rientrare da una stanza non la richiama), quindi non esiste il modo di
       ri-tirarla che quella tecnica serve a chiudere. Ultima delle cinque,
       DOPO ogni altro piazzamento, cosi' la tiratura non sposta il flusso di
       nessuno di essi: i numeri misurati di fusione/stanza a tempo/arena
       restano quelli del WP6. */
    if (game->floor >= WORLD_POURHOUSE_ROOM_MIN_FLOOR &&
        GameRngRange(&game->rng, 0, 99) < WORLD_POURHOUSE_ROOM_CHANCE_PERCENT)
        WorldPlaceSpecialRoom(game, ROOM_POURHOUSE);
    WorldLinkRooms(game);
    /* WP5: la taglia VERA del piano appena generato, in celle -- il totale
       finale (partenza + combattimento + boss + speciali 1x1), non il
       bersaglio pre-estrazione 'targetCells' sopra (che puo' sforare o non
       raggiungere il budget). Scritta una sola volta qui, DOPO ogni
       piazzamento (WorldLinkRooms non aggiunge/toglie celle, solo porte):
       fonte di WorldTimedRoomThresholdSeconds. */
    int cellsX[GRID_SIZE*GRID_SIZE], cellsY[GRID_SIZE*GRID_SIZE];
    game->floorCellCount = WorldCollectCells(game, cellsX, cellsY);
    /* WP16, seconda tornata (vedi il commento su Game.timedRoomEverGenerated
       in core/game_types.h): registra se questo piano ha piazzato uno dei tre
       archetipi non garantiti -- un OR con lo stato dei piani precedenti
       della stessa run, mai un azzeramento (solo GameResetRunWithSeed
       azzera). WorldWriteRoom scrive 'kind' su una sola cella per stanza (la
       "cella di STATO", il primo bit della maschera): scandire ogni cella
       'exists' senza filtrare quella cella e' comunque corretto, perche' le
       altre celle della stessa stanza multi-cella hanno 'kind' a zero
       (ROOM_EMPTY, mai un archetipo) e non possono produrre un falso
       positivo. */
    for (int fy = 0; fy < GRID_SIZE; fy++)
        for (int fx = 0; fx < GRID_SIZE; fx++)
        {
            if (!game->rooms[fy][fx].exists) continue;
            RoomKind fk = game->rooms[fy][fx].kind;
            if (fk == ROOM_TIMED) game->timedRoomEverGenerated = true;
            else if (fk == ROOM_ARENA) game->arenaRoomEverGenerated = true;
            else if (fk == ROOM_SECRET) game->secretRoomEverGenerated = true;
        }
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
/* WP3 (docs/design/systems/secrets-and-obstacles.md, "Ostacoli generati a
   tema" + DEC-024 "degenerazione del tema"; default proposto stile DEC-019,
   registrato in secrets-and-obstacles.md "Default proposti dall'implementazione"
   e in governance/open-questions.md voce 29 -- NON canone, da playtest).
   Quanti dei blocchi che RoomLayoutBuild produce diventano DISTRUTTIBILI o
   PERICOLO invece di restare SOLIDI (zero-default): chance FLAT per i
   distruttibili (il documento non chiede una scala col piano), chance
   CRESCENTE col piano per i pericoli (piu' aggressivi nei piani alti),
   testata PRIMA cosi' le due percentuali non si accavallano mai. */
#define OBSTACLE_DESTRUCTIBLE_CHANCE 0.35f
#define OBSTACLE_HAZARD_CHANCE_BASE 0.08f
#define OBSTACLE_HAZARD_CHANCE_PER_FLOOR 0.05f
#define OBSTACLE_HAZARD_CHANCE_MAX 0.40f

static void WorldBuildObstacles(Game *game, const RoomState *room)
{
    game->obstacleCount = 0;
    game->obstacleHoleCount = 0;

    int holes = WorldRoomHoleCount(game);
    for (int i = 0; i < holes && game->obstacleCount < MAX_OBSTACLES; i++)
    {
        Rectangle hole = WorldRoomHoleRect(game, i);
        if (hole.width <= 0.0f || hole.height <= 0.0f) continue;
        int idx = game->obstacleCount;
        game->obstacles[idx].x = hole.x;
        game->obstacles[idx].y = hole.y;
        game->obstacles[idx].w = hole.width;
        game->obstacles[idx].h = hole.height;
        /* WP3: l'angolo mancante di una L e' sempre muro vero, mai
           distruttibile ne' pericolo -- e senza identita' di cella: nessuno
           stato persistente ha senso per un buco strutturale del piano. */
        game->obstacles[idx].family = OBSTACLE_SOLID;
        game->obstacleCellX[idx] = -1;
        game->obstacleCellY[idx] = -1;
        game->obstacleLocalIndex[idx] = -1;
        game->obstacleCount++;
    }
    game->obstacleHoleCount = game->obstacleCount;

    /* WP6: l'arena di sfida (ROOM_ARENA) NON riceve l'arredo del layout, come
       gia' non lo ricevono boss/tesoro/negozio -- resta uno spazio libero. E'
       una scelta di design, non una dimenticanza: l'ondata maggiorata ha
       bisogno di spazio per essere schivabile, ed e' anche il motivo per cui
       l'arena non e' mai 1x1 (WorldPlaceArenaRoom). Conseguenza dichiarata: la
       riduzione di budget per ostacoli di DEC-043 (WorldSpawnEnemyWave) non
       tocca mai l'arena, che non ne ha. */
    if (room->kind != ROOM_COMBAT || room->cleared) return;
    const RoomLayoutDef *layout = &game->content.floors[game->floor - 1].roomLayout;
    if (!layout->active) return;

    float hazardChance = OBSTACLE_HAZARD_CHANCE_BASE +
                          (float)(game->floor - 1)*OBSTACLE_HAZARD_CHANCE_PER_FLOOR;
    if (hazardChance > OBSTACLE_HAZARD_CHANCE_MAX) hazardChance = OBSTACLE_HAZARD_CHANCE_MAX;

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

        /* La FORMA (posizioni/misure) si costruisce prima in un buffer locale:
           RoomLayoutBuild resta un modulo puro, non sa nulla di famiglie ne' di
           persistenza (vedi il commento su ObstacleFamily in room_layout.h).
           La famiglia si assegna DOPO con un secondo stream deterministico
           derivato dallo STESSO seme (mai game->rng: due visite alla stessa
           cella devono produrre la STESSA disposizione E la STESSA famiglia,
           altrimenti la persistenza sotto perderebbe senso). */
        Obstacle cellObs[ROOM_LAYOUT_MAX_PER_CELL];
        int n = RoomLayoutBuild(layout, seed, rect.x, rect.y, rect.width, rect.height, cellObs, budget);

        unsigned int familySeed = seed ^ 0x9E3779B9u;
        unsigned short destroyedMask = 0;
        if (cx >= 0 && cx < GRID_SIZE && cy >= 0 && cy < GRID_SIZE) destroyedMask = game->destroyedObstacleMask[cy][cx];

        for (int k = 0; k < n && game->obstacleCount < MAX_OBSTACLES; k++)
        {
            ObstacleFamily family = OBSTACLE_SOLID;
            float roll = GameRngFloat(&familySeed, 0.0f, 1.0f);
            if (roll < hazardChance) family = OBSTACLE_HAZARD;
            else if (roll < hazardChance + OBSTACLE_DESTRUCTIBLE_CHANCE) family = OBSTACLE_DESTRUCTIBLE;

            /* Gia' spaccato in questa run (CombatExplodeAt, vedi combat.c):
               non rientra fra gli ostacoli di questa visita. Solo i
               distruttibili possono essere segnati -- il bit resta 0 per
               chiunque altro, quindi questo controllo e' innocuo anche se
               'family' fosse cambiata fra una visita e l'altra (non puo':
               stesso seme, stesso roll). */
            if (family == OBSTACLE_DESTRUCTIBLE && (destroyedMask & (unsigned short)(1u << k))) continue;

            int idx = game->obstacleCount;
            game->obstacles[idx] = cellObs[k];
            game->obstacles[idx].family = family;
            game->obstacleCellX[idx] = cx;
            game->obstacleCellY[idx] = cy;
            game->obstacleLocalIndex[idx] = k;
            game->obstacleCount++;
        }
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
#define WORLD_OBSTACLE_ENEMY_BUDGET_COST 0.18f
#define WORLD_OBSTACLE_ENEMY_BUDGET_FLOOR 0.35f

/* WP6 ("grado piu' alto" dell'arena di sfida, systems/special-rooms.md +
   systems/enemies.md): porta un tipo di nemico alla FASCIA ALTA della sua
   banda di potenza dichiarata -- esattamente cio' che enemies.md concede al
   Veterano ("occupa la fascia alta della stessa banda"), non un'uscita dalla
   banda. La potenza e' lineare in hpMul (EnemyTypePower = PowerDurability *
   resto, e solo PowerDurability contiene hpMul), quindi un solo fattore
   centra il bersaglio; EnemyTypeClamp riporta comunque hpMul dentro le sue
   manopole, che puo' lasciare la potenza sotto il bersaglio -- va bene: e'
   un tetto, non una promessa.
   Deterministico e senza RNG di proposito: due run con lo stesso seed devono
   trovare la stessa arena, non una piu' cattiva dell'altra. Un tipo gia' in
   fascia alta resta com'e' (mai indebolito: sarebbe il contrario del senso di
   questa funzione). */
static void WorldArenaGradeUpEnemyType(EnemyTypeDef *type)
{
    if (!type || !type->active) return;
    float target = type->boss ? ENEMY_TYPE_BOSS_POWER_MAX : ENEMY_TYPE_POWER_MAX;
    float power = EnemyTypePower(type);
    if (power <= 0.0f || power >= target) return;
    type->hpMul *= target/power;
    EnemyTypeClamp(type);
}

/* L'ondata di nemici di una stanza. 'budgetScale' e 'gradeUp' esistono solo
   per l'arena di sfida (WP6): la stanza di combattimento normale chiama
   sempre con (1.0f, false) e si comporta ESATTAMENTE come prima -- stesse
   estrazioni, stesso ordine, stesso risultato a parita' di seed. */
static void WorldSpawnEnemyWave(Game *game, float budgetScale, bool gradeUp)
{
    const FloorContent *fc = &game->content.floors[game->floor - 1];
    float budget = 3.0f + (float)game->floor + (float)GameRngRange(&game->rng, 0, 2);
    /* DEC-170 (default proposto): il budget cresce con le celle, ma SOTTO la
       proporzione (radice quadrata invece che lineare) -- una stanza grande
       deve sembrare piu' grande, non quattro stanze appiccicate. */
    int cellCount = WorldRoomCellCount(game, game->roomX, game->roomY);
    if (cellCount > 1) budget *= sqrtf((float)cellCount);

    /* WP6: il "+50%" dell'arena di sfida (WORLD_ARENA_BUDGET_MULTIPLIER,
       world.h) -- qui, DOPO la scala per celle di DEC-170 e PRIMA della
       riduzione per ostacoli di DEC-043, cosi' resta un moltiplicatore della
       difficolta' che quella stanza avrebbe avuto come combattimento normale,
       non un numero assoluto scollegato dal piano e dalla taglia. Per ogni
       altra stanza budgetScale vale 1.0f: nessun cambiamento. */
    budget *= budgetScale;

    /* DEC-043 (budget di difficolta' condiviso; default proposto stile
       DEC-019, registrato in secrets-and-obstacles.md "Default proposti
       dall'implementazione" e in governance/open-questions.md voce 29): ogni
       ostacolo ambientale della stanza -- di QUALUNQUE famiglia, solido,
       distruttibile o pericolo -- costa un pezzo del budget nemici; le
       celle-buco di una L NON contano (sono struttura del piano, non arredo a
       tema). Gli ostacoli di QUESTA stanza sono gia' pronti: WorldSpawnRoomContents
       chiama WorldBuildObstacles prima di questa funzione. Non si scende mai
       sotto WORLD_OBSTACLE_ENEMY_BUDGET_FLOOR (la stessa soglia minima di
       costo di un singolo nemico, poco sotto): garantisce che il budget resti
       > 0 e che il PRIMO nemico si spawni sempre, anche in una stanza fittissima
       di ostacoli (secrets-and-obstacles.md, "Casi limite": "non deve
       azzerarsi la presenza di nemici"). */
    int obstacleSpend = game->obstacleCount - game->obstacleHoleCount;
    if (obstacleSpend > 0)
    {
        budget -= (float)obstacleSpend * WORLD_OBSTACLE_ENEMY_BUDGET_COST;
        if (budget < WORLD_OBSTACLE_ENEMY_BUDGET_FLOOR) budget = WORLD_OBSTACLE_ENEMY_BUDGET_FLOOR;
    }

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
            /* WP6: nell'arena il tipo si affronta nella sua fascia alta -- si
               lavora su una COPIA, mai sul contenuto del piano (fc e' const, e
               una modifica in place renderebbe piu' cattivi anche i nemici
               delle stanze normali dello stesso piano). Costare di piu' e'
               voluto: il budget maggiorato compra nemici migliori, non solo
               piu' nemici. */
            EnemyTypeDef graded;
            const EnemyTypeDef *type = &fc->enemies[slot];
            if (gradeUp)
            {
                graded = *type;
                WorldArenaGradeUpEnemyType(&graded);
                type = &graded;
            }
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
                GameQueueDiscoveryCardWithImage(game, type->name[0] ? type->name : "Nemico sconosciuto",
                                                "Una nuova minaccia di questo mondo.", type->imageId);
            game->enemyEncountered[game->floor - 1][slot] = true;
            budget -= cost;
        }
        else
        {
            /* Nessun tipo generato (manifest vecchio/assente): restano i quattro
               nemici storici, che non hanno manopole da alzare -- l'arena
               (gradeUp) qui sale di sola QUANTITA', col budget maggiorato.
               Limite dichiarato, non dimenticanza: il grado piu' alto vive nel
               vocabolario dei TIPI (core/enemy_type.h), che e' il cammino vero
               del gioco generato. */
            EnemyKind kind = (EnemyKind)GameRngRange(&game->rng, 0, 2);
            EntitiesAddEnemy(game, kind, WorldFreeRoomPosition(game, 58.0f));
            budget -= 1.0f;   /* i nemici storici valgono 1.0 per definizione */
        }
        spawned++;
    }
}

void WorldSpawnCombatRoom(Game *game)
{
    WorldSpawnEnemyWave(game, 1.0f, false);
}

/* WP6: l'ondata dell'arena di sfida -- budget maggiorato e tipi in fascia alta
   della banda. Stessa funzione, stessi vincoli (tetto per cella, soglia minima
   di budget, posizioni libere dagli ostacoli): l'arena e' una stanza di
   combattimento piu' impegnativa, non un sistema di spawn a parte. */
static void WorldSpawnArenaWave(Game *game)
{
    WorldSpawnEnemyWave(game, WORLD_ARENA_BUDGET_MULTIPLIER, true);
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

/* Vero se il negozio di questo piano tiene Crust in banco (DEC-008, WP2 --
   vedi WORLD_SHOP_CRUST_STOCK_PERCENT sopra). Stessa disciplina di
   WorldShopStocksFlux appena sopra: deterministico dal seed di run e dal
   piano, MAI da game->rng, con un dominio di hash diverso ('CRST') cosi'
   le due tirature non si correlano artificialmente (un piano che ha il
   Flux non ha percio' ne' piu' ne' meno probabilita' di avere anche il
   Crust). */
static bool WorldShopStocksCrust(const Game *game)
{
    unsigned int state = game->runSeed ^ ((unsigned int)game->floor*2654435761u) ^ 0x43525354u;   /* 'CRST' */
    return (int)(GameRngNext(&state)%100u) < WORLD_SHOP_CRUST_STOCK_PERCENT;
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
    /* WP5 (DEC-051, "stanza a tempo"): l'esito si decide UNA volta sola, al
       primo ingresso -- "raggiunta entro soglia" e' un evento, non uno stato
       che possa cambiare rientrando piu' tardi. Si riusa 'rewardTaken' con lo
       stesso significato di sempre ("il premio di questa stanza e' stato
       assegnato"): qui non c'e' un oggetto fisico da prendere, il premio si
       assegna da solo se in tempo, quindi il campo resta true/false per
       sempre da qui in poi -- esattamente cio' che serve per ricordare
       l'esito ad ogni ri-materializzazione della stanza (sotto, e per il
       marcatore visivo in DrawPickup). */
    if (firstVisit && room->kind == ROOM_TIMED)
    {
        float threshold = WorldTimedRoomThresholdSeconds(game);
        float elapsed = game->runElapsedSeconds - game->floorEntryElapsedSeconds;
        room->rewardTaken = elapsed <= threshold;
        if (room->rewardTaken)
        {
            WorldAwardRoomCompletionCurrency(game, ROOM_TIMED);
            /* WP16 (DEC-042): stessa condizione che paga la valuta sopra --
               "entro soglia" e' esattamente cosa chiede TRIAL_TIMED_ROOM_
               WITHIN_THRESHOLD. */
            TrialsOnTimedRoomWithinThreshold(game);
        }
    }
    /* WP8 (DEC-167, rewards-and-economy.md: "una stanza segreta quando e'
       stata trovata"): la condizione di completamento di QUESTO archetipo e'
       esserci entrati -- il varco murato era gia' stato aperto un istante
       prima, e aprirlo E' il lavoro. Solo al primo ingresso, come il negozio:
       rientrare non paga una seconda volta.
       La super-segreta aggiunge qui il catalizzatore di fusione (vedi il
       commento su WORLD_SECRET_SUPER_FLUX in world.h: versato direttamente e
       non come pickup, cosi' non e' ne' perdibile ne' raccoglibile due
       volte). */
    if (firstVisit && room->kind == ROOM_SECRET)
    {
        WorldAwardRoomCompletionCurrency(game, ROOM_SECRET);
        if (room->secretSuper) game->player.flux += WORLD_SECRET_SUPER_FLUX;
        /* WP16 (DEC-042): "trovata" e' la stessa condizione di DEC-167 sopra
           -- entrambi i livelli (normale/super) contano per la prova. */
        TrialsOnSecretFound(game);
    }
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
        /* WP16 (DEC-042): l'inizio di un tentativo "pulito" per
           TRIAL_BOSS_NO_DAMAGE -- vedi il commento sulla funzione in
           trials.h per il perche' un ri-ingresso qui e' sempre sicuro (le
           porte restano chiuse finche' il boss e' vivo, GameRoomIsLocked). */
        TrialsOnBossRoomEntered(game);
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
            GameQueueDiscoveryCardWithImage(game, bossName, "Il guardiano di questo piano.",
                                            bossType ? bossType->imageId : NULL);
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
        /* DEC-008/WP2, fonte scelta per la demo: stessa tecnica di cui sopra
           (hash del seed di run e del piano, mai game->rng), un'altra riga
           del banco separata dal resto (y diverso) per non sovrapporsi al
           Flux quando entrambi sono in banco lo stesso piano. 'value'=2
           equivale a 2 icone 'heart_temp' nell'HUD (un'icona per punto di
           tempHp, HudTempHeartsSlotCount in src/render/game_renderer.c). */
        if (WorldShopStocksCrust(game))
            EntitiesAddPickup(game, PICKUP_CRUST, (Vector2){ center.x + 260.0f, center.y - 70.0f }, 2, WORLD_SHOP_CRUST_COST);
        GameSetMessage(game, "Negozio: tocca un oggetto per comprarlo.");
    }
    else if (room->kind == ROOM_BOSS && room->cleared)
    {
        EntitiesAddPickup(game, PICKUP_EXIT, (Vector2){ center.x + 70.0f, center.y }, 0, 0);
        GameSetMessage(game, game->floor == FLOOR_COUNT ? "Portale finale riaperto." : "Portale per il prossimo piano riaperto.");
    }
    else if (room->kind == ROOM_FUSION)
    {
        /* WP4: nessun 'rewardTaken'/gate -- il crogiolo non e' una ricompensa
           estratta una volta sola, e' un arredo fisso della stanza. Va
           ri-materializzato a OGNI ingresso perche' EntitiesClear (sopra, a
           inizio funzione) ha appena svuotato tutti i pickup della stanza,
           esattamente come i piedistalli degli attivi/i piedistalli-Innesto
           persistenti qui sotto. Scenario 4 di special-rooms.md (senza due
           oggetti idonei o senza Flux la stanza resta visitabile): il crogiolo
           apre comunque BuildScreen, che mostra FusionStatusText -- questa
           funzione non sa nulla dei requisiti di fusione, ne' deve saperlo. */
        EntitiesAddPickup(game, PICKUP_FUSION_ALTAR, center, 0, 0);
        GameSetMessage(game, "Crogiolo: tocca l'altare per fondere due oggetti.");
    }
    else if (room->kind == ROOM_TIMED)
    {
        /* WP5: nessun 'rewardTaken'/gate sulla RI-materializzazione -- come il
           crogiolo di ROOM_FUSION sopra, la clessidra e' un arredo fisso della
           stanza, ricreata ad OGNI ingresso perche' EntitiesClear (inizio
           funzione) ha appena svuotato tutti i pickup. 'rewardTaken' qui sopra
           e' invece l'ESITO gia' deciso (WP5, blocco sopra): 'value'=1/0
           sceglie il tag "attiva"/"scaduta" e l'etichetta in DrawPickup, cosi'
           l'indicazione dentro la stanza resta leggibile per tutta la
           permanenza, non solo al primo ingresso (il messaggio sotto invece
           sparisce dopo pochi secondi, GameSetMessage). */
        EntitiesAddPickup(game, PICKUP_TIMED_MARKER, center, room->rewardTaken ? 1 : 0, 0);
        /* Il dettaglio numerico (secondi impiegati/soglia) ha senso SOLO al
           primo ingresso, il momento vero in cui l'esito si decide: su un
           rientro successivo il tempo trascorso e' ormai un altro numero (la
           run e' andata avanti) e mostrarlo di nuovo accanto a un esito
           ormai fisso confonderebbe piu' che informare -- si ripiega su un
           messaggio senza numeri, coerente con l'esito gia' scritto su
           'rewardTaken' e mostrato in modo persistente dalla clessidra
           stessa (sopra). */
        if (firstVisit && room->rewardTaken)
        {
            char msg[96];
            snprintf(msg, sizeof(msg), "Stanza a tempo: raggiunta in tempo (%.0fs/%.0fs). Ricompensa!",
                     (double)(game->runElapsedSeconds - game->floorEntryElapsedSeconds),
                     (double)WorldTimedRoomThresholdSeconds(game));
            GameSetMessage(game, msg);
        }
        else if (firstVisit)
        {
            char msg[96];
            snprintf(msg, sizeof(msg), "Stanza a tempo: soglia scaduta (%.0fs/%.0fs). Nessun bonus.",
                     (double)(game->runElapsedSeconds - game->floorEntryElapsedSeconds),
                     (double)WorldTimedRoomThresholdSeconds(game));
            GameSetMessage(game, msg);
        }
        else
        {
            GameSetMessage(game, room->rewardTaken ? "Stanza a tempo: ricompensa gia' raccolta."
                                                     : "Stanza a tempo: soglia gia' scaduta.");
        }
    }
    else if (room->kind == ROOM_ARENA)
    {
        /* WP6 (systems/special-rooms.md, "Arena di sfida"): il segnale della
           sfida e' un arredo fisso, ri-materializzato a OGNI ingresso come il
           crogiolo (ROOM_FUSION) e la clessidra (ROOM_TIMED) -- EntitiesClear a
           inizio funzione ha appena svuotato i pickup. Il suo 'value' porta lo
           STATO della stanza, i tre di questo archetipo. */
        int arenaState = room->cleared ? 2 : (room->arenaActive ? 1 : 0);
        EntitiesAddPickup(game, PICKUP_ARENA_ALTAR, center, arenaState, 0);
        if (room->cleared)
        {
            GameSetMessage(game, "Arena: sfida gia' superata.");
        }
        else if (room->arenaActive)
        {
            /* Ramo NON raggiungibile in gioco: a sfida accettata le porte
               restano chiuse (GameRoomIsLocked) e l'unico esito possibile e'
               vincere -- o morire, che chiude la run (permadeath, nessun
               retry). Esiste comunque, e ri-crea davvero l'ondata, perche' un
               rientro con la sfida accettata e non vinta NON deve trovare una
               stanza vuota: WorldCheckRoomClear la dichiarerebbe superata al
               primo passo e regalerebbe la ricompensa senza combattere. */
            WorldSpawnArenaWave(game);
            GameSetMessage(game, "Arena: sfida in corso.");
        }
        else
        {
            GameSetMessage(game, "Arena: tocca il segnale e premi X per accettare la sfida.");
        }
    }
    else if (room->kind == ROOM_POURHOUSE)
    {
        /* WP7: tutto il contenuto di questa stanza e' LA PUNTATA, che vive in
           src/world/pourhouse.c -- composizione, banco e messaggio d'ingresso
           insieme, perche' sono tre facce dello stesso stato. Come il crogiolo
           e la clessidra, il banco e' arredo fisso: ri-materializzato ad OGNI
           ingresso perche' EntitiesClear (inizio funzione) ha appena svuotato i
           pickup. Nessun 'firstVisit' qui: la puntata sa da sola se e' gia'
           composta per QUESTA stanza. */
        WorldPourhousePrepareRoom(game);
    }
    else if (room->kind == ROOM_SECRET)
    {
        /* WP8 (systems/special-rooms.md, "Stanza segreta"): la ricompensa
           "degna del segreto" di secrets-and-obstacles.md. Un oggetto dal pool
           del piano ma con la RARITA' MINIMA ALZATA -- il migliore dei tre
           candidati, esattamente la tecnica dell'arena di sfida
           (WorldSpawnRoomReward) e per lo stesso motivo: un segreto che paga
           una comune quando nel pool c'e' una rara non sarebbe proporzionato
           al costo di trovarlo (uno strumento di breccia speso, per una stanza
           che il piano non chiede mai di visitare).
           DETERMINISTICO E SENZA ALCUNA TIRATURA di proposito. Il tesoro
           ri-pesca a ogni ingresso finche' l'oggetto non e' preso (superficie
           di sfruttamento nota, vedi il commento nel ramo ROOM_TREASURE
           sopra); qui la scelta non estrae nulla, quindi uscire e rientrare
           ritrova SEMPRE lo STESSO oggetto: "contenuto assegnato una sola
           volta" senza bisogno di consumare il premio al primo ingresso (che
           lo farebbe perdere a chi esce senza raccoglierlo).
           'rewardTaken' passa a vero quando l'oggetto viene DAVVERO preso
           (CombatPickup, stesso ramo del tesoro), e da li' in poi la stanza
           resta attraversabile e vuota. */
        if (!room->rewardTaken)
        {
            const FloorContent *sfc = &game->content.floors[game->floor - 1];
            int best = 0;
            for (int i = 1; i < 3; i++)
                if ((int)sfc->items[i].rarity > (int)sfc->items[best].rarity) best = i;
            EntitiesAddItemPickup(game, center, sfc->items[best], 0);
            GameSetMessage(game, room->secretSuper ? "Stanza super-segreta: Flux versato, prendi la ricompensa."
                                                    : "Stanza segreta: prendi la ricompensa.");
        }
        else
        {
            GameSetMessage(game, "Stanza segreta: gia' svuotata.");
        }
    }
    else
    {
        GameSetMessage(game, "Scegli una porta.");
    }
    /* DEC-183: ogni Innesto sganciato in questa stanza (CombatDropGraft, o
       lasciato su un piedistallo da uno scambio successivo di un Innesto
       persistente, vedi CombatPickup) resta A TERRA e recuperabile per
       TUTTA LA RUN -- vanno ri-materializzati TUTTI qui perche' EntitiesClear
       (sopra, a inizio funzione) ha appena svuotato TUTTI i pickup della
       stanza: senza questo, uscire e rientrare li farebbe sparire, esattamente
       il gap che DEC-183 chiude (vedi la nota nel decision-log). Un ciclo su
       Game.droppedGrafts, non un campo singolo: in questa stessa stanza
       possono coesistere piu' record (due sganci, o uno sgancio piu' un
       Innesto lasciato da uno scambio) e nessuno dei due deve sovrascrivere
       l'altro. Indipendente dal 'kind' della stanza -- si puo' sganciare un
       Innesto ovunque -- e percio' vive FUORI dalla catena if/else sopra,
       cosi' da coesistere con qualunque altro contenuto della stanza. */
    for (int i = 0; i < MAX_DROPPED_GRAFTS; i++)
    {
        DroppedGraftRecord *rec = &game->droppedGrafts[i];
        if (!rec->active || rec->roomX != game->roomX || rec->roomY != game->roomY) continue;
        Pickup *ground = EntitiesAddItemPickup(game, rec->pos, rec->item, 0);
        if (!ground) continue;
        ground->isPersistedGraft = true;
        ground->droppedGraftSlot = i;
        /* Ri-nato sotto (o vicino a) i piedi del giocatore: si atterra nella
           cella di arrivo PRIMA che questa funzione giri (WorldTryEnterRoom/
           WorldStartFloor impostano game->player.pos, poi chiamano
           WorldSpawnRoomContents) -- se la posizione salvata allo sgancio e'
           vicina alla porta da cui si rientra (la posizione piu' naturale in
           cui lasciare qualcosa per ritrovarlo), senza questa guardia
           l'Innesto si riequipaggerebbe/scambierebbe DA SOLO al primo frame:
           esattamente il caso per cui esiste Pickup.locked, e la stessa
           guardia che CombatDropGraft applica gia' al momento dello
           sgancio. */
        float r = ground->radius + game->player.radius;
        if (GameMathLengthSquared(GameMathSubtract(ground->pos, game->player.pos)) < r*r) ground->locked = true;
    }
    /* DEC-170: entrare in una stanza NON e' un movimento di telecamera -- si
       riparte dall'inquadratura giusta, senza scivolate. */
    WorldSnapCamera(game);
}

/* WP6 (systems/special-rooms.md, "Arena di sfida"): la CONFERMA esplicita
   della sfida. Tre guardie, tutte necessarie: si deve essere in un'arena, la
   sfida non deve essere gia' accettata (una seconda conferma non ri-crea
   l'ondata) ne' gia' superata, e il giocatore deve essere A CONTATTO col
   segnale -- il tasto premuto in mezzo alla stanza non fa nulla. La geometria
   di contatto e' la stessa di ogni altro pickup (CombatUpdatePickups): un
   segnale che si "accende" quando ci sei sopra e non a distanza.
   L'irreversibilita' e' voluta e dichiarata: da qui in poi si esce solo
   vincendo (le porte si chiudono) o morendo, che chiude la run -- permadeath,
   nessun retry (special-rooms.md). */
bool WorldTryStartArenaChallenge(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if (room->kind != ROOM_ARENA || room->arenaActive || room->cleared) return false;

    Pickup *altar = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_ARENA_ALTAR) { altar = &game->pickups[i]; break; }
    }
    if (!altar) return false;
    float r = altar->radius + game->player.radius;
    if (GameMathLengthSquared(GameMathSubtract(altar->pos, game->player.pos)) > r*r) return false;

    room->arenaActive = true;
    altar->value = 1;   /* "IN CORSO": il segnale racconta lo stato per tutta la sfida */
    WorldSpawnArenaWave(game);
    /* audio-and-feedback.md: la conferma di un'azione volontaria usa il suono
       di conferma gia' esistente -- nessun evento sonoro nuovo per questo
       archetipo (le porte che si chiudono le racconta il loro stato visivo). */
    AudioPlaySfx(AUDIO_SFX_UI_CONFIRM);
    GameSetMessage(game, "Sfida accettata: porte chiuse fino alla fine.");
    return true;
}

/* WP8: la fascia di PARETE CONDIVISA fra la cella (cx,cy) e la sua vicina in
   direzione 'dir'. Larga quanto una porta (DOOR_HALF*2, centrata sul lato,
   esattamente dove la porta comparira' se il varco si apre) e profonda
   WORLD_SECRET_BREACH_DEPTH DENTRO la cella: e' la zona in cui la bomba deve
   esplodere per sbrecciare, ed e' il punto in cui il renderer ancora la crepa.
   Dentro la cella e non fuori di proposito -- il giocatore e la bomba stanno
   sempre dentro la stanza, mai nello spessore del muro. */
Rectangle WorldSecretWallRect(const Game *game, int cx, int cy, int dir)
{
    Rectangle c = WorldCellRect(game, cx, cy);
    float ccx = c.x + c.width*0.5f, ccy = c.y + c.height*0.5f;
    float d = WORLD_SECRET_BREACH_DEPTH;
    switch (dir)
    {
        case DIR_UP:    return (Rectangle){ ccx - DOOR_HALF, c.y, DOOR_HALF*2.0f, d };
        case DIR_DOWN:  return (Rectangle){ ccx - DOOR_HALF, c.y + c.height - d, DOOR_HALF*2.0f, d };
        case DIR_LEFT:  return (Rectangle){ c.x, ccy - DOOR_HALF, d, DOOR_HALF*2.0f };
        case DIR_RIGHT: return (Rectangle){ c.x + c.width - d, ccy - DOOR_HALF, d, DOOR_HALF*2.0f };
    }
    /* Direzione fuori dai quattro valori di Direction: rettangolo VUOTO, mai
       la cella intera -- il default piu' innocuo e' "nessuna parete
       sbrecciabile", non "tutta la stanza lo e'" (disciplina zero-default). */
    return (Rectangle){ 0.0f, 0.0f, 0.0f, 0.0f };
}

/* Cerchio contro rettangolo: il punto del rettangolo piu' vicino al centro del
   cerchio, poi una distanza al quadrato. Stessa tecnica di
   CombatCircleTouchesObstacle (src/gameplay/combat.c), ripetuta qui invece di
   esportare quella: e' privata al modulo combattimento e questo modulo non
   deve dipendere da lui (la dipendenza corre nel verso opposto). */
static bool WorldCircleTouchesRect(Vector2 c, float r, Rectangle rect)
{
    if (rect.width <= 0.0f || rect.height <= 0.0f) return false;
    float nx = c.x, ny = c.y;
    if (nx < rect.x) nx = rect.x;
    if (nx > rect.x + rect.width) nx = rect.x + rect.width;
    if (ny < rect.y) ny = rect.y;
    if (ny > rect.y + rect.height) ny = rect.y + rect.height;
    float dx = c.x - nx, dy = c.y - ny;
    return dx*dx + dy*dy <= r*r;
}

/* WP8 (systems/secrets-and-obstacles.md, Scenario 4 e Scenario 5): lo
   strumento di breccia apre il varco murato di una stanza segreta.
   Si guarda SOLO dalla stanza corrente verso le sue vicine -- una segreta si
   apre stando dalla parte visibile del muro, mai dall'interno (dove il
   giocatore non puo' ancora essere) -- e ogni cella della stanza corrente
   guarda le proprie quattro vicine, cosi' la cosa funziona identica in una
   1x1 e in una 2x2.
   La guardia vera e' geometrica: l'esplosione deve toccare la fascia di parete
   condivisa (WorldSecretWallRect). E' questo che rende l'indizio UTILE invece
   che decorativo -- una bomba lasciata in mezzo alla stanza non apre nulla,
   nemmeno se il suo raggio arrivasse fin la' passando per un'altra parete.
   Vale identica per la super-segreta: nessun indizio, ma il muro giusto
   bombardato alla cieca si apre lo stesso (DEC-025: "oppure per intuizione
   estrema del giocatore"). Il rivelatore aiuta, non e' un requisito. */
bool WorldTryBreachSecretWall(Game *game, Vector2 pos, float radius)
{
    if (!game || game->floor <= 0) return false;
    bool opened = false;
    int cellX[4], cellY[4];
    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
    for (int i = 0; i < cellCount; i++)
    {
        int cx = cellX[i], cy = cellY[i];
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + DirDx(d), ny = cy + DirDy(d);
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            RoomState *secret = WorldRoomAtMutable(game, nx, ny);
            if (secret->kind != ROOM_SECRET || secret->secretOpened) continue;
            if (!WorldCircleTouchesRect(pos, radius, WorldSecretWallRect(game, cx, cy, d))) continue;

            secret->secretOpened = true;
            /* La porta si apre sui DUE lati, come farebbe WorldLinkRooms: una
               porta e' un fatto del LATO di UNA cella (vedi l'invariante su
               RoomState), e aprirne solo uno lascerebbe una porta che si
               attraversa in un verso solo. Resta l'UNICA porta della coppia
               (DEC-181): il piazzamento garantisce che la segreta condivida
               esattamente una coppia di celle con esattamente una stanza. */
            game->rooms[cy][cx].doors[d] = true;
            game->rooms[ny][nx].doors[(d + 2)%4] = true;
            opened = true;

            EntitiesAddParticle(game, pos, game->theme.wall, 30);
        }
    }
    if (opened)
    {
        /* audio-and-feedback.md: nessun evento sonoro NUOVO per questo
           archetipo -- si riusa la porta che si apre, che e' esattamente cio'
           che e' appena successo. */
        AudioPlaySfx(AUDIO_SFX_DOOR_OPEN);
        GameSetMessage(game, "Il muro cede: si apre un varco.");
    }
    return opened;
}

void WorldStartFloor(Game *game, int floor)
{
    game->floor = floor;
    /* WP5 (DEC-051): l'ISTANTE dell'ingresso nel piano, la base da cui si
       misura la soglia della stanza a tempo (mai dall'inizio della run --
       vedi il commento sul campo in core/game_types.h). Nessun tempo passa
       durante la generazione sotto, quindi catturarlo qui o dopo
       WorldGenerateFloorMap e' equivalente; qui e' piu' vicino alla sua
       definizione. */
    game->floorEntryElapsedSeconds = game->runElapsedSeconds;
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
        /* WP15a: dentro una simulazione d'arena il varco verso il piano 1 non
           si attraversa. Non e' una comodita': l'attraversamento chiama
           GameResetRunWithSeed, che azzera l'intero Game -- compreso lo
           snapshot dello stato d'ingresso ancora da restituire. Uscire dalla
           simulazione e' sempre disponibile e costa un tasto, quindi qui non
           si perde nulla. */
        if (game->floorZeroTrialActive) return;
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
    else if (room->kind == ROOM_ARENA)
    {
        /* WP6 -- la ricompensa maggiorata di rewards-and-economy.md
           ("superiore alla media di una stanza di combattimento equivalente
           non a rischio", Scenario 2), su tre canali:
           1) l'oggetto: dal pool del piano ma con la RARITA' MINIMA ALZATA --
              qui significa "il migliore dei tre candidati", non un'estrazione
              pesata come tesoro/negozio (ItemPoolDrawIndex). Deterministico e
              senza tiratura di proposito: una sfida vinta non deve poter
              pagare una comune quando nel pool c'e' una rara, altrimenti il
              rischio non sarebbe proporzionato al premio. A parita' di rarita'
              vince l'indice piu' basso (stabile a parita' di seed).
           2) la valuta: WORLD_ROOM_CURRENCY_ARENA, gia' assegnata da
              WorldCheckRoomClear.
           3) il catalizzatore di fusione: DEC-022 dichiara le arene di sfida
              una delle tre fonti di Flux, e prima del WP6 ne esistevano solo
              due nel motore (vedi il commento su WORLD_BOSS_FLUX_DROP_PERCENT
              in cima al file, ora superato). Tiratura vera di game->rng: come
              per il boss, questa funzione gira una volta sola per stanza
              (protetta da 'rewardTaken' in cima). */
        const FloorContent *afc = &game->content.floors[game->floor - 1];
        int best = 0;
        for (int i = 1; i < 3; i++) if ((int)afc->items[i].rarity > (int)afc->items[best].rarity) best = i;
        EntitiesAddItemPickup(game, (Vector2){ center.x - 52.0f, center.y }, afc->items[best], 0);
        if (GameRngRange(&game->rng, 0, 99) < WORLD_ARENA_FLUX_DROP_PERCENT)
            EntitiesAddPickup(game, PICKUP_FLUX, (Vector2){ center.x + 60.0f, center.y }, 1, 0);
        GameSetMessage(game, "Sfida superata: prendi la ricompensa.");
    }
}

void WorldCheckRoomClear(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    /* WP6: l'arena si "ripulisce" solo se la sfida e' stata ACCETTATA -- una
       stanza attraversata senza accettare non e' completata (non paga valuta,
       non lascia ricompensa) e non deve nemmeno diventarlo per il fatto banale
       che non ci sono nemici dentro. E' la stessa disciplina "ogni archetipo
       ha la propria condizione di completamento" di DEC-167. */
    bool clearable = (room->kind == ROOM_COMBAT || room->kind == ROOM_BOSS) ||
                     (room->kind == ROOM_ARENA && room->arenaActive);
    if (clearable && !room->cleared && WorldNoEnemiesActive(game))
    {
        room->cleared = true;
        /* WP6: il segnale della sfida passa a "SUPERATA" subito, senza
           aspettare un rientro nella stanza: il giocatore e' li' adesso ed e'
           adesso che l'esito cambia. */
        if (room->kind == ROOM_ARENA)
        {
            for (int i = 0; i < MAX_PICKUPS; i++)
                if (game->pickups[i].active && game->pickups[i].kind == PICKUP_ARENA_ALTAR) game->pickups[i].value = 2;
        }
        /* M7 (substrato del catalogo): il boss di QUESTO piano e' appena
           stato sconfitto -- vedi il commento su Game.bossDefeated in
           core/game_types.h. game->floor e' sempre valido qui (la stanza
           boss esiste solo dentro un piano vero, mai nel Piano 0). */
        if (room->kind == ROOM_BOSS) game->bossDefeated[game->floor - 1] = true;
        /* WP16 (DEC-042): TRIAL_BOSS_NO_DAMAGE/TRIAL_FLOOR_UNDER_TIME (boss)
           e TRIAL_ARENA_WON leggono qui, subito dopo che 'cleared' e'
           diventato vero -- stessa guardia "mai due volte" dell'if di sopra
           (una stanza gia' ripulita non richiama mai questo blocco). */
        TrialsOnRoomCleared(game, room->kind);
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
