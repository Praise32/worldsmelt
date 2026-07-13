#include "render/game_renderer.h"

#include "core/game_math.h"
#include "game/game.h"

#include "rlgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

UiLayout UiComputeLayout(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float pad = GameMathClampFloat(sw*0.014f, 18.0f, 28.0f);
    float leftW = GameMathClampFloat(sw*0.18f, 250.0f, 330.0f);
    float rightW = GameMathClampFloat(sw*0.24f, 330.0f, 460.0f);
    float bottomH = GameMathClampFloat(sh*0.12f, 82.0f, 128.0f);
    float maxW = sw - leftW - rightW - pad*4.0f;
    float maxH = sh - bottomH - pad*3.0f;
    float scale = fminf(maxW/(float)SCREEN_WIDTH, maxH/(float)SCREEN_HEIGHT);
    scale = GameMathClampFloat(scale, 0.72f, 1.45f);
    float gw = (float)SCREEN_WIDTH*scale;
    float gh = (float)SCREEN_HEIGHT*scale;
    float gx = leftW + pad*2.0f + (maxW - gw)*0.5f;
    float gy = pad + (maxH - gh)*0.5f;

    UiLayout layout = { 0 };
    layout.leftPanel = (Rectangle){ pad, pad, leftW, sh - pad*2.0f };
    layout.rightPanel = (Rectangle){ sw - rightW - pad, pad, rightW, sh - pad*2.0f };
    layout.bottomPanel = (Rectangle){ leftW + pad*2.0f, sh - bottomH - pad, maxW, bottomH };
    layout.gameRect = (Rectangle){ gx, gy, gw, gh };
    layout.gameScale = scale;
    return layout;
}

bool UiScreenToGameMouse(UiLayout layout, Vector2 *out)
{
    Vector2 mouse = GetMousePosition();
    bool inside = CheckCollisionPointRec(mouse, layout.gameRect);
    if (out)
    {
        out->x = (mouse.x - layout.gameRect.x)/layout.gameScale;
        out->y = (mouse.y - layout.gameRect.y)/layout.gameScale;
        out->x = GameMathClampFloat(out->x, 0.0f, (float)SCREEN_WIDTH);
        out->y = GameMathClampFloat(out->y, 0.0f, (float)SCREEN_HEIGHT);
    }
    return inside;
}

static Color RoomMapColor(RoomKind kind)
{
    switch (kind)
    {
        case ROOM_START: return (Color){ 220, 220, 220, 255 };
        case ROOM_TREASURE: return (Color){ 240, 210, 75, 255 };
        case ROOM_SHOP: return (Color){ 95, 220, 130, 255 };
        case ROOM_BOSS: return (Color){ 230, 82, 90, 255 };
        case ROOM_COMBAT: return (Color){ 130, 145, 165, 255 };
        default: return (Color){ 40, 44, 50, 255 };
    }
}

static void DrawRoom(Game *game)
{
    ClearBackground(game->theme.bg);
    DrawRectangleRec((Rectangle){ ROOM_X, ROOM_Y, ROOM_W, ROOM_H }, game->theme.floor);
    for (int x = (int)ROOM_X; x < (int)ROOM_RIGHT; x += 44) DrawLine(x, (int)ROOM_Y, x - 80, (int)ROOM_BOTTOM, GameColorWithAlpha(game->theme.accent, 42));
    DrawRectangleLinesEx((Rectangle){ ROOM_X, ROOM_Y, ROOM_W, ROOM_H }, 5.0f, game->theme.wall);

    const RoomState *room = GameCurrentRoom(game);
    float cx = ROOM_X + ROOM_W*0.5f;
    float cy = ROOM_Y + ROOM_H*0.5f;
    Color doorColor = GameRoomIsLocked(game) ? (Color){ 200, 58, 58, 255 } : game->theme.accent2;
    if (room->doors[DIR_UP]) DrawRectangle((int)(cx - DOOR_HALF), (int)ROOM_Y - 2, (int)(DOOR_HALF*2), 10, doorColor);
    if (room->doors[DIR_DOWN]) DrawRectangle((int)(cx - DOOR_HALF), (int)ROOM_BOTTOM - 8, (int)(DOOR_HALF*2), 10, doorColor);
    if (room->doors[DIR_LEFT]) DrawRectangle((int)ROOM_X - 2, (int)(cy - DOOR_HALF), 10, (int)(DOOR_HALF*2), doorColor);
    if (room->doors[DIR_RIGHT]) DrawRectangle((int)ROOM_RIGHT - 8, (int)(cy - DOOR_HALF), 10, (int)(DOOR_HALF*2), doorColor);
}

static bool DrawAtlasCell(Game *game, int cell, Vector2 pos, float size, Color tint)
{
    if (!game->atlasLoaded) return false;
    /* Cella nota ma rimasta vuota (gate di qualita' di melting-sprites
       fallito, vedi ATLAS_CELL_MIN_OPAQUE in game_types.h): si ripiega sulla
       forma geometrica di riserva per QUESTA entita', non per l'intero
       atlas. cell fuori range (mai dovrebbe capitare con le costanti
       SPR_*) e' trattato come assente, mai come crash. */
    if (cell < 0 || cell >= SPR_COUNT || !game->atlasCellPresent[cell]) return false;
    int col = cell%ATLAS_COLS;
    int row = cell/ATLAS_COLS;
    Rectangle src = { (float)(col*ATLAS_CELL), (float)(row*ATLAS_CELL), (float)ATLAS_CELL, (float)ATLAS_CELL };
    Rectangle dst = { pos.x, pos.y, size, size };
    DrawTexturePro(game->atlas, src, dst, (Vector2){ size*0.5f, size*0.5f }, 0.0f, tint);
    return true;
}

static void DrawEnemy(Game *game, const Enemy *e)
{
    Color c = e->kind == ENEMY_BOSS ? game->theme.boss : game->theme.enemy;
    if (game->atlasLoaded)
    {
        int cell = SPR_ENEMY_CHASER;
        if (e->kind == ENEMY_SHOOTER) cell = SPR_ENEMY_SHOOTER;
        else if (e->kind == ENEMY_TANK) cell = SPR_ENEMY_TANK;
        else if (e->kind == ENEMY_BOSS) cell = SPR_BOSS;
        DrawAtlasCell(game, cell, e->pos, e->radius*3.3f, WHITE);
        if (e->kind == ENEMY_BOSS) DrawText(game->theme.bossName, (int)(ROOM_X + 20), (int)(ROOM_Y + 16), 18, RAYWHITE);
    }
    else if (e->kind == ENEMY_CHASER)
    {
        DrawCircleV(e->pos, e->radius, c);
        DrawCircleV((Vector2){ e->pos.x - 5, e->pos.y - 4 }, 3, BLACK);
        DrawCircleV((Vector2){ e->pos.x + 5, e->pos.y - 4 }, 3, BLACK);
    }
    else if (e->kind == ENEMY_SHOOTER)
    {
        DrawTriangle((Vector2){ e->pos.x, e->pos.y - e->radius },
                     (Vector2){ e->pos.x - e->radius, e->pos.y + e->radius },
                     (Vector2){ e->pos.x + e->radius, e->pos.y + e->radius }, c);
        DrawCircleV(e->pos, 5, BLACK);
    }
    else if (e->kind == ENEMY_TANK)
    {
        DrawRectangleRounded((Rectangle){ e->pos.x - e->radius, e->pos.y - e->radius*0.75f, e->radius*2, e->radius*1.5f }, 0.25f, 8, c);
        DrawCircleV((Vector2){ e->pos.x - 7, e->pos.y - 2 }, 4, BLACK);
        DrawCircleV((Vector2){ e->pos.x + 7, e->pos.y - 2 }, 4, BLACK);
    }
    else
    {
        DrawCircleV(e->pos, e->radius, c);
        for (int i = 0; i < 8; i++)
        {
            float a = (float)i*PI_F/4.0f + (float)GetTime();
            DrawCircleV((Vector2){ e->pos.x + cosf(a)*(e->radius + 12.0f), e->pos.y + sinf(a)*(e->radius + 12.0f) }, 7.0f, game->theme.accent);
        }
        DrawText(game->theme.bossName, (int)(ROOM_X + 20), (int)(ROOM_Y + 16), 18, RAYWHITE);
    }
    if (e->hp < e->maxHp)
    {
        float w = e->radius*2.2f;
        float pct = GameMathClampFloat(e->hp/e->maxHp, 0.0f, 1.0f);
        DrawRectangle((int)(e->pos.x - w*0.5f), (int)(e->pos.y - e->radius - 14), (int)w, 4, BLACK);
        DrawRectangle((int)(e->pos.x - w*0.5f), (int)(e->pos.y - e->radius - 14), (int)(w*pct), 4, game->theme.accent2);
    }
}

static void DrawItemShape(Vector2 pos, Item item, float size)
{
    if (item.shape%3 == 0) DrawCircleV(pos, size, item.color);
    else if (item.shape%3 == 1) DrawRectanglePro((Rectangle){ pos.x, pos.y, size*1.7f, size*1.7f }, (Vector2){ size*0.85f, size*0.85f }, 45.0f, item.color);
    else DrawTriangle((Vector2){ pos.x, pos.y - size }, (Vector2){ pos.x - size, pos.y + size }, (Vector2){ pos.x + size, pos.y + size }, item.color);
    DrawCircleLines((int)pos.x, (int)pos.y, size + 5.0f, RAYWHITE);
}

static void DrawPickup(Game *game, const Pickup *p)
{
    float bob = sinf((float)GetTime()*4.0f + p->pos.x*0.01f)*3.5f;
    Vector2 pos = { p->pos.x, p->pos.y + bob };
    Color c = RAYWHITE;
    const char *label = "";
    if (game->atlasLoaded)
    {
        int cell = SPR_ITEM;
        float size = 46.0f;
        if (p->kind == PICKUP_HEART) { cell = SPR_HEART; label = "HP"; }
        else if (p->kind == PICKUP_COIN) { cell = SPR_COIN; label = "$"; }
        else if (p->kind == PICKUP_BOMB) { cell = SPR_BOMB; label = "B"; }
        else if (p->kind == PICKUP_KEY) { cell = SPR_KEY; label = "K"; }
        else if (p->kind == PICKUP_EXIT) { cell = SPR_EXIT; label = "EXIT"; size = 78.0f; }
        else label = p->item.name;
        DrawAtlasCell(game, cell, pos, size, WHITE);
        if (p->kind != PICKUP_ITEM && p->kind != PICKUP_EXIT) DrawText(label, (int)pos.x - 6, (int)pos.y - 8, 14, BLACK);
        if (p->cost > 0) DrawText(TextFormat("%dc", p->cost), (int)pos.x - 11, (int)pos.y + 24, 14, GOLD);
        else if (p->kind == PICKUP_ITEM) DrawText(label, (int)pos.x - 55, (int)pos.y + 24, 12, RAYWHITE);
        return;
    }
    if (p->kind == PICKUP_HEART)
    {
        c = RED;
        label = "HP";
        DrawCircleV((Vector2){ pos.x - 5, pos.y - 3 }, 8, c);
        DrawCircleV((Vector2){ pos.x + 5, pos.y - 3 }, 8, c);
        DrawTriangle((Vector2){ pos.x - 13, pos.y + 1 }, (Vector2){ pos.x + 13, pos.y + 1 }, (Vector2){ pos.x, pos.y + 16 }, c);
    }
    else if (p->kind == PICKUP_COIN) { c = GOLD; label = "$"; DrawCircleV(pos, 11, c); }
    else if (p->kind == PICKUP_BOMB) { c = DARKGRAY; label = "B"; DrawCircleV(pos, 12, c); DrawCircleV((Vector2){ pos.x + 6, pos.y - 8 }, 3, ORANGE); }
    else if (p->kind == PICKUP_KEY) { c = SKYBLUE; label = "K"; DrawCircleV(pos, 10, c); DrawRectangle((int)pos.x, (int)pos.y - 3, 17, 6, c); }
    else if (p->kind == PICKUP_EXIT) { c = game->theme.accent2; label = "EXIT"; DrawCircleV(pos, 26, GameColorWithAlpha(c, 90)); DrawCircleLines((int)pos.x, (int)pos.y, 24, c); }
    else
    {
        DrawItemShape(pos, p->item, 14.0f);
        label = p->item.name;
    }
    if (p->kind != PICKUP_ITEM && p->kind != PICKUP_EXIT) DrawText(label, (int)pos.x - 6, (int)pos.y - 8, 14, BLACK);
    if (p->cost > 0) DrawText(TextFormat("%dc", p->cost), (int)pos.x - 11, (int)pos.y + 24, 14, GOLD);
    else if (p->kind == PICKUP_ITEM) DrawText(label, (int)pos.x - 55, (int)pos.y + 24, 12, RAYWHITE);
}

static void DrawEquipment(const Game *game)
{
    const Player *p = &game->player;
    int hatStack = 0;
    for (int i = 0; i < p->itemCount; i++)
    {
        const Item *item = &p->items[i];
        if (item->slot == SLOT_HAT)
        {
            float y = p->pos.y - 32.0f - (float)hatStack*8.0f;
            DrawRectangleRounded((Rectangle){ p->pos.x - 14, y, 28, 8 }, 0.3f, 5, item->color);
            DrawRectangleRounded((Rectangle){ p->pos.x - 9, y - 9, 18, 12 }, 0.35f, 5, item->color);
            hatStack++;
        }
        else if (item->slot == SLOT_EYES)
        {
            DrawCircleV((Vector2){ p->pos.x - 6, p->pos.y - 18 }, 4.5f, item->color);
            DrawCircleV((Vector2){ p->pos.x + 6, p->pos.y - 18 }, 4.5f, item->color);
            DrawLineEx((Vector2){ p->pos.x - 2, p->pos.y - 18 }, (Vector2){ p->pos.x + 2, p->pos.y - 18 }, 2.0f, item->color);
        }
        else if (item->slot == SLOT_HAND)
        {
            float side = (i%2 == 0) ? 1.0f : -1.0f;
            DrawLineEx((Vector2){ p->pos.x + side*13.0f, p->pos.y - 2.0f },
                       (Vector2){ p->pos.x + side*32.0f, p->pos.y - 14.0f }, 5.0f, item->color);
        }
        else if (item->slot == SLOT_BACK)
        {
            DrawTriangle((Vector2){ p->pos.x, p->pos.y - 5 },
                         (Vector2){ p->pos.x - 22, p->pos.y + 31 },
                         (Vector2){ p->pos.x + 22, p->pos.y + 31 }, GameColorWithAlpha(item->color, 140));
        }
        else if (item->slot == SLOT_BODY) DrawCircleV((Vector2){ p->pos.x, p->pos.y + 3 }, 6, item->color);
        else
        {
            float a = (float)GetTime()*2.4f + (float)i;
            DrawCircleV((Vector2){ p->pos.x + cosf(a)*34.0f, p->pos.y + sinf(a)*34.0f }, 5, item->color);
        }
    }
}

static void DrawPlayer(Game *game)
{
    Player *p = &game->player;
    Color tint = (p->invuln > 0.0f && ((int)(GetTime()*18.0)%2 == 0)) ? GameColorWithAlpha(WHITE, 115) : WHITE;
    if (!DrawAtlasCell(game, SPR_PLAYER, p->pos, 96.0f, tint))
    {
        DrawLineEx((Vector2){ p->pos.x, p->pos.y - 22 }, (Vector2){ p->pos.x, p->pos.y + 14 }, 5, tint);
        DrawCircleV((Vector2){ p->pos.x, p->pos.y - 30 }, 10, tint);
        DrawLineEx((Vector2){ p->pos.x - 18, p->pos.y - 4 }, (Vector2){ p->pos.x + 18, p->pos.y - 4 }, 4, tint);
        DrawLineEx((Vector2){ p->pos.x, p->pos.y + 14 }, (Vector2){ p->pos.x - 15, p->pos.y + 31 }, 4, tint);
        DrawLineEx((Vector2){ p->pos.x, p->pos.y + 14 }, (Vector2){ p->pos.x + 15, p->pos.y + 31 }, 4, tint);
    }
    DrawEquipment(game);
}

static void DrawHud(Game *game)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, HUD_H, (Color){ 18, 20, 25, 255 });
    DrawRectangle(0, HUD_H - 2, SCREEN_WIDTH, 2, game->theme.wall);
    DrawText("MELTING ISAAC LLM", 20, 10, 22, RAYWHITE);
    DrawText(TextFormat("%d FPS", GetFPS()), 850, 12, 18, (Color){ 126, 232, 152, 255 });
    DrawText(TextFormat("Piano %d/%d  Room %s  HP %d/%d  Monete %d  Bombe %d  Chiavi %d",
                        game->floor, FLOOR_COUNT, GameRoomKindName(GameCurrentRoom(game)->kind),
                        game->player.hp, game->player.maxHp, game->player.coins, game->player.bombs, game->player.keys),
             20, 36, 15, (Color){ 214, 218, 226, 255 });
    DrawText(TextFormat("Tema: %s | %s | %s", game->theme.name, game->theme.style, game->content.loaded ? "LLM cache" : "fallback"),
             20, 58, 14, game->theme.accent2);

    int baseX = 820;
    int baseY = 28;
    int size = 10;
    int gap = 4;
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            const RoomState *room = &game->rooms[y][x];
            if (!room->exists) continue;
            Color c = room->visited ? RoomMapColor(room->kind) : (Color){ 70, 75, 82, 255 };
            if (x == game->roomX && y == game->roomY) c = RAYWHITE;
            DrawRectangle(baseX + x*(size + gap), baseY + y*(size + gap), size, size, c);
        }
    }
}

static void DrawFooter(Game *game)
{
    DrawRectangle(0, SCREEN_HEIGHT - FOOTER_H, SCREEN_WIDTH, FOOTER_H, (Color){ 16, 17, 21, 255 });
    DrawText("WASD muovi   Mouse/Frecce spara   SPACE bomba   R restart   ESC esci",
             22, SCREEN_HEIGHT - 25, 15, RAYWHITE);
    if (game->messageTimer > 0.0f)
    {
        DrawRectangle(18, SCREEN_HEIGHT - 70, SCREEN_WIDTH - 36, 25, GameColorWithAlpha(BLACK, 160));
        DrawText(game->message, 28, SCREEN_HEIGHT - 64, 15, RAYWHITE);
    }
}

static void DrawGameplayCanvas(Game *game)
{
    DrawRoom(game);

    for (int i = 0; i < MAX_PICKUPS; i++) if (game->pickups[i].active) DrawPickup(game, &game->pickups[i]);
    for (int i = 0; i < MAX_BOMBS; i++)
    {
        Bomb *b = &game->bombs[i];
        if (!b->active) continue;
        DrawCircleV(b->pos, 14.0f + sinf((float)GetTime()*10.0f)*2.0f, DARKGRAY);
        DrawCircleLines((int)b->pos.x, (int)b->pos.y, b->radius*(1.0f - b->timer/1.05f), ORANGE);
    }
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        Shot *s = &game->shots[i];
        if (!s->active) continue;
        DrawCircleV(s->pos, s->radius + 3.0f, GameColorWithAlpha(s->color, 80));
        DrawCircleV(s->pos, s->radius, s->color);
    }
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active) DrawEnemy(game, &game->enemies[i]);
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        DrawCircleV(p->pos, p->radius, GameColorWithAlpha(p->color, (unsigned char)GameMathClampFloat(p->life*420.0f, 0.0f, 255.0f)));
    }
    DrawPlayer(game);
    DrawHud(game);
    DrawFooter(game);

    if (game->phase == PHASE_GAME_OVER || game->phase == PHASE_WIN)
    {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GameColorWithAlpha(BLACK, 160));
        const char *title = (game->phase == PHASE_WIN) ? "RUN COMPLETATA" : "RUN FALLITA";
        DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, 38)/2, SCREEN_HEIGHT/2 - 40, 38, RAYWHITE);
        DrawText("Premi R per ricominciare", SCREEN_WIDTH/2 - 118, SCREEN_HEIGHT/2 + 10, 20, game->theme.accent2);
    }
}

static const char *SlotName(ItemSlot slot)
{
    switch (slot)
    {
        case SLOT_HAT: return "testa";
        case SLOT_EYES: return "occhi";
        case SLOT_HAND: return "mano";
        case SLOT_BACK: return "schiena";
        case SLOT_BODY: return "corpo";
        case SLOT_AURA: return "aura";
        default: return "slot";
    }
}

static void TraitsToText(unsigned int traits, char *out, int outSize)
{
    out[0] = '\0';
    struct TraitName { unsigned int mask; const char *name; } names[] = {
        { TRAIT_BOUNCE, "bounce" }, { TRAIT_HOMING, "homing" },
        { TRAIT_EXPLODE, "explode" }, { TRAIT_SPLIT, "split" },
        { TRAIT_PIERCE, "pierce" }, { TRAIT_RAPID, "rapid" },
        { TRAIT_GIANT, "giant" }, { TRAIT_SLOW, "slow" },
        { TRAIT_VAMP, "vamp" }
    };
    for (int i = 0; i < (int)(sizeof(names)/sizeof(names[0])); i++)
    {
        if (!(traits & names[i].mask)) continue;
        if (out[0]) strncat(out, ", ", outSize - (int)strlen(out) - 1);
        strncat(out, names[i].name, outSize - (int)strlen(out) - 1);
    }
    if (!out[0]) snprintf(out, outSize, "nessuno");
}

static void DrawPanel(Rectangle rec, const char *title, Color accent)
{
    DrawRectangleRec(rec, (Color){ 18, 20, 26, 236 });
    DrawRectangleLinesEx(rec, 2.0f, GameColorWithAlpha(accent, 190));
    DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, 34, GameColorWithAlpha(accent, 55));
    DrawText(title, (int)rec.x + 16, (int)rec.y + 10, 16, RAYWHITE);
}

static void DrawStatLine(const char *label, const char *value, int x, int y, Color color)
{
    DrawText(label, x, y, 14, (Color){ 155, 163, 176, 255 });
    DrawText(value, x + 118, y, 15, color);
}

static void DrawLargeMinimap(Game *game, Rectangle rec)
{
    int size = 26;
    int gap = 7;
    int total = GRID_SIZE*size + (GRID_SIZE - 1)*gap;
    int baseX = (int)(rec.x + rec.width*0.5f - total*0.5f);
    int baseY = (int)rec.y;
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            const RoomState *room = &game->rooms[y][x];
            Rectangle cell = { (float)(baseX + x*(size + gap)), (float)(baseY + y*(size + gap)), (float)size, (float)size };
            Color c = room->exists ? (room->visited ? RoomMapColor(room->kind) : (Color){ 72, 78, 88, 255 }) : (Color){ 28, 31, 38, 255 };
            DrawRectangleRec(cell, c);
            if (x == game->roomX && y == game->roomY) DrawRectangleLinesEx(cell, 3.0f, RAYWHITE);
            else DrawRectangleLinesEx(cell, 1.0f, GameColorWithAlpha(BLACK, 160));
        }
    }
}

static void DrawItemPreview(Game *game, const Item *item, int x, int y, int width, bool owned)
{
    char traits[128];
    TraitsToText(item->traits, traits, sizeof(traits));
    Rectangle row = { (float)x, (float)y, (float)width, 58.0f };
    DrawRectangleRec(row, owned ? (Color){ 28, 32, 40, 220 } : (Color){ 24, 27, 34, 210 });
    DrawRectangleLinesEx(row, 1.0f, GameColorWithAlpha(item->color, 170));
    if (!DrawAtlasCell(game, SPR_ITEM, (Vector2){ x + 28.0f, y + 29.0f }, 36.0f, WHITE))
    {
        DrawItemShape((Vector2){ x + 28.0f, y + 29.0f }, *item, 12.0f);
    }
    DrawText(item->name, x + 55, y + 9, 14, RAYWHITE);
    DrawText(TextFormat("%s  |  %s", SlotName(item->slot), traits), x + 55, y + 31, 12, (Color){ 190, 198, 211, 255 });
}

static void DrawOuterUi(Game *game, UiLayout layout)
{
    DrawPanel(layout.leftPanel, "RUN", game->theme.accent);
    int lx = (int)layout.leftPanel.x + 18;
    int ly = (int)layout.leftPanel.y + 48;
    DrawText("MELTING ISAAC LLM", lx, ly, 23, RAYWHITE);
    DrawText(TextFormat("%s / %s", game->theme.name, game->theme.style), lx, ly + 34, 14, game->theme.accent2);
    DrawText(TextFormat("Boss: %s", game->theme.bossName), lx, ly + 56, 14, (Color){ 214, 218, 226, 255 });
    DrawStatLine("Piano", TextFormat("%d / %d", game->floor, FLOOR_COUNT), lx, ly + 92, RAYWHITE);
    DrawStatLine("Stanza", GameRoomKindName(GameCurrentRoom(game)->kind), lx, ly + 116, game->theme.accent2);
    DrawStatLine("Fonte", game->content.loaded ? "OpenAI cache" : "fallback", lx, ly + 140, RAYWHITE);
    DrawText("MAPPA", lx, ly + 184, 16, game->theme.accent2);
    DrawLargeMinimap(game, (Rectangle){ (float)lx, (float)(ly + 214), layout.leftPanel.width - 36.0f, 190.0f });
    DrawText("COMANDI", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 142), 16, game->theme.accent2);
    DrawText("WASD muovi", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 112), 14, RAYWHITE);
    DrawText("Mouse/Frecce spara", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 90), 14, RAYWHITE);
    DrawText("SPACE bomba", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 68), 14, RAYWHITE);
    DrawText("ESC/P pausa   F11 fullscreen", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 46), 14, RAYWHITE);

    DrawPanel(layout.rightPanel, "GIOCATORE", game->theme.accent2);
    int rx = (int)layout.rightPanel.x + 18;
    int ry = (int)layout.rightPanel.y + 48;
    Player *p = &game->player;
    DrawStatLine("HP", TextFormat("%d / %d", p->hp, p->maxHp), rx, ry, RED);
    DrawStatLine("Danno", TextFormat("%.1f", p->damage), rx, ry + 24, RAYWHITE);
    DrawStatLine("Cadenza", TextFormat("%.2fs", p->fireDelay), rx, ry + 48, RAYWHITE);
    DrawStatLine("Vel. colpo", TextFormat("%.0f", p->shotSpeed), rx, ry + 72, RAYWHITE);
    DrawStatLine("Raggio", TextFormat("%.1f", p->shotRadius), rx, ry + 96, RAYWHITE);
    DrawStatLine("Risorse", TextFormat("%dc  %db  %dk", p->coins, p->bombs, p->keys), rx, ry + 120, GOLD);

    DrawText("OGGETTI PRESI", rx, ry + 164, 16, game->theme.accent2);
    int rowY = ry + 194;
    int shown = 0;
    int start = p->itemCount > 6 ? p->itemCount - 6 : 0;
    for (int i = start; i < p->itemCount && shown < 6; i++, shown++)
    {
        DrawItemPreview(game, &p->items[i], rx, rowY + shown*64, (int)layout.rightPanel.width - 36, true);
    }
    if (shown == 0) DrawText("Nessun oggetto ancora.", rx, rowY + 8, 14, (Color){ 155, 163, 176, 255 });

    int floorIndex = GameMathClampInt(game->floor - 1, 0, FLOOR_COUNT - 1);
    int previewY = (int)(layout.rightPanel.y + layout.rightPanel.height - 230);
    DrawText("ANTEPRIMA PIANO", rx, previewY, 16, game->theme.accent2);
    for (int i = 0; i < 3; i++)
    {
        DrawItemPreview(game, &game->content.floors[floorIndex].items[i], rx, previewY + 30 + i*64, (int)layout.rightPanel.width - 36, false);
    }

    DrawPanel(layout.bottomPanel, "LOG", game->theme.wall);
    int bx = (int)layout.bottomPanel.x + 18;
    int by = (int)layout.bottomPanel.y + 46;
    const char *atlasMode = strstr(game->content.atlasPath, ".png") ? "Sprite locali (Stable Diffusion): atlas PNG 128x128" : "Atlas procedurale/fallback BMP";
    DrawText(atlasMode, bx, by, 15, game->theme.accent2);
    DrawText(game->messageTimer > 0.0f ? game->message : "Raccogli oggetti per creare sinergie generate.", bx, by + 28, 16, RAYWHITE);
}

static void DrawMenuOverlay(AppMode mode, Game *game)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, GameColorWithAlpha(BLACK, 180));
    Rectangle box = { sw*0.5f - 300.0f, sh*0.5f - 190.0f, 600.0f, 380.0f };
    DrawRectangleRec(box, (Color){ 20, 22, 29, 246 });
    DrawRectangleLinesEx(box, 2.0f, game->theme.accent2);
    const char *title = mode == APP_MENU ? "MELTING ISAAC LLM" : "PAUSA";
    DrawText(title, (int)(box.x + box.width*0.5f - MeasureText(title, 38)*0.5f), (int)box.y + 36, 38, RAYWHITE);
    if (mode == APP_MENU)
    {
        DrawText("Run generate da OpenAI, spritesheet API ritagliato a celle.", (int)box.x + 62, (int)box.y + 98, 18, game->theme.accent2);
        DrawText("INVIO  nuova run", (int)box.x + 110, (int)box.y + 160, 24, RAYWHITE);
        DrawText("F11    cambia fullscreen", (int)box.x + 110, (int)box.y + 202, 22, RAYWHITE);
        DrawText("Q      esci", (int)box.x + 110, (int)box.y + 242, 22, RAYWHITE);
    }
    else
    {
        DrawText("ESC/P  continua", (int)box.x + 128, (int)box.y + 148, 24, RAYWHITE);
        DrawText("R      nuova run", (int)box.x + 128, (int)box.y + 190, 22, RAYWHITE);
        DrawText("M      menu principale", (int)box.x + 128, (int)box.y + 230, 22, RAYWHITE);
        DrawText("Q      esci", (int)box.x + 128, (int)box.y + 270, 22, RAYWHITE);
    }
}

static void DrawGeneratingOverlay(const Game *game, const GenProgress *progress)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, GameColorWithAlpha(BLACK, 200));
    Rectangle box = { sw*0.5f - 320.0f, sh*0.5f - 130.0f, 640.0f, 260.0f };
    DrawRectangleRec(box, (Color){ 20, 22, 29, 246 });
    DrawRectangleLinesEx(box, 2.0f, game->theme.accent2);
    const char *title = "GENERAZIONE RUN";
    DrawText(title, (int)(box.x + box.width*0.5f - MeasureText(title, 30)*0.5f), (int)box.y + 28, 30, RAYWHITE);

    const char *phase = progress ? progress->phase : "avvio";
    int percent = progress ? progress->percent : 0;
    DrawText(TextFormat("%s  %d%%", phase, percent), (int)box.x + 60, (int)box.y + 84, 18, game->theme.accent2);

    Rectangle bar = { box.x + 60.0f, box.y + 116.0f, box.width - 120.0f, 26.0f };
    DrawRectangleRec(bar, (Color){ 35, 38, 48, 255 });
    DrawRectangleRec((Rectangle){ bar.x, bar.y, bar.width*(float)percent/100.0f, bar.height }, game->theme.accent2);
    DrawRectangleLinesEx(bar, 2.0f, RAYWHITE);

    DrawText(progress ? progress->message : "", (int)box.x + 60, (int)box.y + 158, 16, RAYWHITE);
    DrawText("ESC annulla e torna al menu", (int)box.x + 60, (int)box.y + 206, 15, (Color){ 155, 163, 176, 255 });
}

void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress)
{
    BeginTextureMode(canvas);
    DrawGameplayCanvas(game);
    EndTextureMode();

    BeginDrawing();
    ClearBackground((Color){ 9, 11, 16, 255 });
    UiLayout layout = UiComputeLayout();
    DrawOuterUi(game, layout);
    Rectangle src = { 0.0f, 0.0f, (float)canvas.texture.width, -(float)canvas.texture.height };
    DrawTexturePro(canvas.texture, src, layout.gameRect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    DrawRectangleLinesEx(layout.gameRect, 4.0f, game->theme.accent2);
    DrawText("GAME VIEW", (int)layout.gameRect.x + 16, (int)layout.gameRect.y + 14, 16, GameColorWithAlpha(RAYWHITE, 170));
    if (mode == APP_MENU || mode == APP_PAUSE) DrawMenuOverlay(mode, game);
    if (mode == APP_GENERATING) DrawGeneratingOverlay(game, genProgress);
    if (takeScreenshot)
    {
        rlDrawRenderBatchActive();
        TakeScreenshot("logs/melting-run-screen.png");
    }
    EndDrawing();
}
