#include "render/game_renderer.h"

#include "core/game_math.h"
#include "game/game.h"
#include "gameplay/synergies.h"
#include "render/item_layers.h"
#include "render/rarity_style.h"

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

/* ============================================================
   Resa 2.5D (step E, docs/superpowers/specs/2026-07-14-feedback-roadmap.md
   punto 5, e docs/APPUNTI.md sezione 7).
 *
 * La valutazione chiesta dal proprietario ha dato questo esito: il 2.5D "alla
 * Isaac" si ottiene quasi tutto COL RENDERING, non generando piu' roba. Costa
 * quindi ZERO secondi di generazione (che era il vincolo vero: i tempi di
 * caricamento sono gia' il punto dolente numero 2 del feedback). Quattro trucchi,
 * tutti qui sotto, piu' uno nel prompt degli sprite (vista a 3/4 invece che di
 * fronte, tools/melting-sprites/prompts/):
 *   1. OMBRA A TERRA: un'ellisse schiacciata sotto ogni entita'. E' il singolo
 *      trucco che da' piu' profondita' per riga di codice: senza, ogni sprite
 *      "galleggia" su uno sfondo piatto; con, il pavimento diventa un piano su
 *      cui le cose POGGIANO.
 *   2. ORDINAMENTO PER PROFONDITA': chi sta piu' in basso e' piu' vicino
 *      all'osservatore, quindi va disegnato DOPO (davanti). Prima l'ordine era
 *      fisso per categoria (tutti i pickup, poi tutti i nemici, poi il
 *      giocatore), e un nemico "dietro" poteva coprire il giocatore "davanti".
 *   3. PAVIMENTO IN PROSPETTIVA: griglia con punto di fuga sopra il muro di
 *      fondo, righe orizzontali che si infittiscono verso il fondo, e una
 *      sfumatura che scurisce la parte lontana.
 *   4. MURI CON SPESSORE: il muro di fondo mostra la sua FACCIA (una fascia
 *      verticale sopra il pavimento) e il suo spigolo superiore; i muri laterali
 *      e quello davanti mostrano il loro spessore. Il campo di gioco (ROOM_*) NON
 *      cambia di un pixel: e' tutta resa, nessuna modifica alla collisione.
   ============================================================ */

/* Spessore dei muri (solo resa: il campo di gioco resta ROOM_X..ROOM_RIGHT,
   ROOM_Y..ROOM_BOTTOM, nessuna collisione cambia). Il muro di fondo e' piu' alto
   degli altri perche' e' l'unico di cui vediamo la FACCIA. */
#define WALL_BACK_H  34.0f
#define WALL_SIDE_W  12.0f
#define WALL_FRONT_H 14.0f

/* Ombra a terra. 'lift' e' quanto sotto il centro dell'entita' poggia l'ombra
   (di norma ~60% del raggio: il "piede"), lo schiacciamento verticale e' fisso a
   0.42 -- e' l'inclinazione della camera, e deve restare UGUALE per tutte le
   entita', altrimenti sembrerebbero riprese da angoli diversi. */
static void DrawGroundShadow(Vector2 pos, float radius, float lift, unsigned char alpha)
{
    if (radius <= 0.5f) return;
    DrawEllipse((int)pos.x, (int)(pos.y + lift), radius, radius*0.42f, (Color){ 0, 0, 0, alpha });
}

static void DrawRoom(Game *game)
{
    ClearBackground(game->theme.bg);

    /* Pavimento: tinta piena, poi una sfumatura che scurisce il FONDO della
       stanza (la parte lontana). E' la stessa cosa che fa l'atmosfera in
       prospettiva: cio' che e' lontano perde contrasto. */
    DrawRectangleRec((Rectangle){ ROOM_X, ROOM_Y, ROOM_W, ROOM_H }, game->theme.floor);
    DrawRectangleGradientV((int)ROOM_X, (int)ROOM_Y, (int)ROOM_W, (int)(ROOM_H*0.45f),
                           GameColorWithAlpha(BLACK, 58), BLANK);

    /* Griglia in prospettiva: le linee "verticali" convergono verso un punto di
       fuga sopra il muro di fondo, quelle orizzontali si infittiscono verso il
       fondo (spaziatura non lineare). Il punto di fuga sta FUORI dalla stanza, in
       alto: e' cio' che fa leggere il pavimento come un piano inclinato sotto lo
       sguardo, invece che come un rettangolo visto di fronte. */
    const float vpx = ROOM_X + ROOM_W*0.5f;
    const float vpy = ROOM_Y - 340.0f;
    Color gridColor = GameColorWithAlpha(game->theme.accent, 34);
    for (int i = 0; i <= 14; i++)
    {
        float x = ROOM_X + ROOM_W*(float)i/14.0f;
        Vector2 near = { x, ROOM_BOTTOM };
        /* Quanto si e' "risaliti" verso il punto di fuga arrivando al muro di
           fondo: t = 0 al bordo vicino, cresce verso il punto di fuga. */
        float t = (ROOM_BOTTOM - ROOM_Y)/(ROOM_BOTTOM - vpy);
        Vector2 far = { near.x + (vpx - near.x)*t, ROOM_Y };
        DrawLineEx(near, far, 1.0f, gridColor);
    }
    for (int i = 1; i < 9; i++)
    {
        float f = (float)i/9.0f;
        float y = ROOM_Y + (ROOM_BOTTOM - ROOM_Y)*powf(f, 1.7f);   /* piu' fitte in alto = piu' lontane */
        DrawLine((int)ROOM_X, (int)y, (int)ROOM_RIGHT, (int)y, gridColor);
    }

    /* Muri con spessore. Il muro di FONDO e' l'unico di cui si vede la faccia
       (una fascia sopra il pavimento, piu' chiara in alto dove prende luce, con
       uno spigolo netto in basso dove incontra il pavimento). I laterali e quello
       davanti mostrano solo il loro spessore, senza faccia: e' esattamente cio'
       che si vedrebbe da una camera inclinata di poco. */
    Color wallDark = GameColorLerp(game->theme.wall, BLACK, 0.45f);
    Color wallLit = GameColorLerp(game->theme.wall, WHITE, 0.18f);
    DrawRectangleGradientV((int)(ROOM_X - WALL_SIDE_W), (int)(ROOM_Y - WALL_BACK_H),
                           (int)(ROOM_W + WALL_SIDE_W*2.0f), (int)WALL_BACK_H, wallLit, wallDark);
    DrawRectangle((int)(ROOM_X - WALL_SIDE_W), (int)(ROOM_Y - 3.0f), (int)(ROOM_W + WALL_SIDE_W*2.0f), 3, wallDark);
    DrawRectangle((int)(ROOM_X - WALL_SIDE_W), (int)ROOM_Y, (int)WALL_SIDE_W, (int)ROOM_H, wallDark);
    DrawRectangle((int)ROOM_RIGHT, (int)ROOM_Y, (int)WALL_SIDE_W, (int)ROOM_H, wallDark);
    DrawRectangle((int)(ROOM_X - WALL_SIDE_W), (int)ROOM_BOTTOM,
                  (int)(ROOM_W + WALL_SIDE_W*2.0f), (int)WALL_FRONT_H, wallDark);
    /* Spigolo illuminato dei muri laterali/davanti: la linea sottile che fa
       leggere lo spessore come uno spessore e non come una cornice piatta. */
    DrawRectangle((int)(ROOM_X - WALL_SIDE_W), (int)ROOM_BOTTOM, (int)(ROOM_W + WALL_SIDE_W*2.0f), 2, wallLit);

    const RoomState *room = GameCurrentRoom(game);
    float cx = ROOM_X + ROOM_W*0.5f;
    float cy = ROOM_Y + ROOM_H*0.5f;
    Color doorColor = GameRoomIsLocked(game) ? (Color){ 200, 58, 58, 255 } : game->theme.accent2;
    /* Le porte si disegnano SOPRA i muri (sono buchi nel muro): quella di fondo
       occupa tutta la faccia del muro, cosi' si legge come un passaggio e non
       come una striscia appoggiata. */
    if (room->doors[DIR_UP]) DrawRectangle((int)(cx - DOOR_HALF), (int)(ROOM_Y - WALL_BACK_H), (int)(DOOR_HALF*2), (int)WALL_BACK_H, doorColor);
    if (room->doors[DIR_DOWN]) DrawRectangle((int)(cx - DOOR_HALF), (int)ROOM_BOTTOM, (int)(DOOR_HALF*2), (int)WALL_FRONT_H, doorColor);
    if (room->doors[DIR_LEFT]) DrawRectangle((int)(ROOM_X - WALL_SIDE_W), (int)(cy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);
    if (room->doors[DIR_RIGHT]) DrawRectangle((int)ROOM_RIGHT, (int)(cy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);
}

/* Vignettatura: quattro sfumature ai bordi del canvas. Non e' decorazione fine a
   se stessa -- scurendo i bordi si spinge l'occhio verso il centro e si accentua
   la sensazione di volume data dagli altri tre trucchi (e' il quarto della lista
   negli APPUNTI, sezione 7). Ultima cosa disegnata nella scena, prima solo del
   messaggio transitorio (che deve restare leggibile). */
static void DrawVignette(void)
{
    /* Banda stretta e alpha contenuta: la vista di gioco e' 960x640, non un
       monitor intero -- una vignettatura tarata "da fotografia" (bande larghe,
       alpha alta) qui non incornicia, SPEGNE la stanza. Provata a 110/130 e
       rifatta: mangiava un quarto della larghezza per lato. */
    const int band = 72;
    const Color edge = (Color){ 0, 0, 0, 92 };
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, band, edge, BLANK);
    DrawRectangleGradientV(0, SCREEN_HEIGHT - band, SCREEN_WIDTH, band, BLANK, edge);
    DrawRectangleGradientH(0, 0, band, SCREEN_HEIGHT, edge, BLANK);
    DrawRectangleGradientH(SCREEN_WIDTH - band, 0, band, SCREEN_HEIGHT, BLANK, edge);
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
    /* drew traccia se l'atlas ha davvero disegnato qualcosa per QUESTA
       entita': game->atlasLoaded dice solo che l'atlas e' caricato, non che
       la cella di questo nemico sia presente (vedi DrawAtlasCell). Se la
       cella e' vuota si ripiega sulla forma geometrica, invece di lasciare
       il nemico invisibile. */
    bool drew = false;
    if (game->atlasLoaded)
    {
        /* Fase 3b: lo sprite si sceglie dalla FORMA del nemico (inventata dal
           modello), non piu' dal vecchio 'kind'. Senza questo, con un atlas
           presente -- cioe' nel caso NORMALE -- le forme sarebbero state
           completamente invisibili: quattro nemici diversi avrebbero mostrato lo
           stesso sprite, e l'unica cosa che il modello puo' dire sull'aspetto di un
           nemico non sarebbe arrivata a schermo. Trovato guardando lo screenshot,
           non da un test: nessun assert poteva accorgersene. */
        int cell = SPR_ENEMY_CHASER;
        if (e->kind == ENEMY_BOSS) cell = SPR_BOSS;   /* un boss e' un boss, qualunque forma abbia */
        else if (e->type.active)
        {
            if (e->type.form == ENEMY_FORM_SPIKY) cell = SPR_ENEMY_SHOOTER;
            else if (e->type.form == ENEMY_FORM_ARMORED) cell = SPR_ENEMY_TANK;
            else if (e->type.form == ENEMY_FORM_FLOATER) cell = SPR_ENEMY_FLOATER;
        }
        else if (e->kind == ENEMY_SHOOTER) cell = SPR_ENEMY_SHOOTER;
        else if (e->kind == ENEMY_TANK) cell = SPR_ENEMY_TANK;
        drew = DrawAtlasCell(game, cell, e->pos, e->radius*3.3f, WHITE);
    }
    /* Il nome del boss va sempre mostrato durante lo scontro, sia che si
       disegni lo sprite sia la forma di riserva. */
    if (e->kind == ENEMY_BOSS) DrawText(game->theme.bossName, (int)(ROOM_X + 20), (int)(ROOM_Y + 16), 18, RAYWHITE);
    if (!drew)
    {
        /* Fase 3b: la FORMA del nemico (inventata dal modello, core/enemy_type.h)
           decide come si disegna la sagoma di riserva. Un nemico SENZA tipo
           (zero-default: manifest vecchio, o nessun manifest) ricade sulle forme
           storiche legate a kind, identiche a prima. */
        EnemyForm form = e->type.active ? e->type.form : ENEMY_FORM_BLOB;
        if (!e->type.active)
        {
            if (e->kind == ENEMY_SHOOTER) form = ENEMY_FORM_SPIKY;
            else if (e->kind == ENEMY_TANK) form = ENEMY_FORM_ARMORED;
            else if (e->kind == ENEMY_BOSS) form = ENEMY_FORM_FLOATER;
        }

        switch (form)
        {
            case ENEMY_FORM_SPIKY:
            {
                /* Stella spinosa: un corpo tondo con punte tutt'intorno. */
                int spikes = 8;
                for (int s = 0; s < spikes; s++)
                {
                    float a = (float)s*PI_F*2.0f/(float)spikes + e->phase*0.35f;
                    Vector2 tip = { e->pos.x + cosf(a)*e->radius*1.6f, e->pos.y + sinf(a)*e->radius*1.6f };
                    Vector2 l = { e->pos.x + cosf(a - 0.22f)*e->radius*0.9f, e->pos.y + sinf(a - 0.22f)*e->radius*0.9f };
                    Vector2 r = { e->pos.x + cosf(a + 0.22f)*e->radius*0.9f, e->pos.y + sinf(a + 0.22f)*e->radius*0.9f };
                    DrawTriangle(tip, l, r, c);
                }
                DrawCircleV(e->pos, e->radius*0.85f, c);
                DrawCircleV(e->pos, e->radius*0.30f, BLACK);
                break;
            }
            case ENEMY_FORM_ARMORED:
            {
                /* Blocco squadrato con piastre: spigoli, niente curve. */
                DrawRectangleRounded((Rectangle){ e->pos.x - e->radius, e->pos.y - e->radius*0.8f, e->radius*2.0f, e->radius*1.6f }, 0.18f, 6, c);
                DrawRectangleLinesEx((Rectangle){ e->pos.x - e->radius, e->pos.y - e->radius*0.8f, e->radius*2.0f, e->radius*1.6f }, 3.0f, GameColorLerp(c, BLACK, 0.45f));
                DrawRectangle((int)(e->pos.x - e->radius*0.55f), (int)(e->pos.y - e->radius*0.25f), (int)(e->radius*1.1f), (int)(e->radius*0.28f), GameColorLerp(c, BLACK, 0.5f));
                DrawCircleV((Vector2){ e->pos.x - e->radius*0.42f, e->pos.y - e->radius*0.15f }, e->radius*0.16f, BLACK);
                DrawCircleV((Vector2){ e->pos.x + e->radius*0.42f, e->pos.y - e->radius*0.15f }, e->radius*0.16f, BLACK);
                break;
            }
            case ENEMY_FORM_FLOATER:
            {
                /* Medusa: cupola tonda e tentacoli che ondeggiano. */
                DrawCircleV(e->pos, e->radius, c);
                DrawCircleV((Vector2){ e->pos.x, e->pos.y - e->radius*0.25f }, e->radius*0.62f, GameColorLerp(c, WHITE, 0.25f));
                for (int t = 0; t < 5; t++)
                {
                    float ox = ((float)t - 2.0f)*e->radius*0.36f;
                    float wob = sinf((float)GetTime()*3.4f + (float)t*0.8f + e->phase)*e->radius*0.22f;
                    DrawLineEx((Vector2){ e->pos.x + ox, e->pos.y + e->radius*0.55f },
                               (Vector2){ e->pos.x + ox + wob, e->pos.y + e->radius*1.5f },
                               e->radius*0.16f, GameColorLerp(c, BLACK, 0.25f));
                }
                DrawCircleV((Vector2){ e->pos.x - e->radius*0.3f, e->pos.y - e->radius*0.25f }, e->radius*0.14f, BLACK);
                DrawCircleV((Vector2){ e->pos.x + e->radius*0.3f, e->pos.y - e->radius*0.25f }, e->radius*0.14f, BLACK);
                break;
            }
            case ENEMY_FORM_BLOB:
            default:
                DrawCircleV(e->pos, e->radius, c);
                DrawCircleV((Vector2){ e->pos.x - e->radius*0.33f, e->pos.y - e->radius*0.27f }, e->radius*0.2f, BLACK);
                DrawCircleV((Vector2){ e->pos.x + e->radius*0.33f, e->pos.y - e->radius*0.27f }, e->radius*0.2f, BLACK);
                break;
        }
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

/* Bordo/glow di rarita' del pickup (design doc, sezione 6: "il pickup
   dell'oggetto ha un bordo del colore della rarita'"). Disegnato attorno
   alla posizione DOPO l'oggetto stesso (sprite d'atlas o forma di riserva,
   vedi il chiamante) ma PRIMA delle etichette di testo (nome/costo): resta
   quindi sempre dietro il testo, mai sopra, cosi' un leggendario nel
   negozio si legge sia come "prezioso" (costo in monete) sia come
   "leggendario" (anello arancio) senza che i due si accavallino. Raggio 25:
   piu' largo dei 23px di meta' sprite (ATLAS_CELL 128 scalato a 46px in
   DrawPickup) cosi' l'anello circonda l'oggetto invece di tagliarlo, e
   sopra i ~19px del cerchio di riserva di DrawItemShape (size+5 con size=14)
   cosi' resta leggibile anche quando l'atlas non ha disegnato nulla. Il
   pulso (raggio+alpha che respirano) e' riservato a raro/leggendario, per
   farli risaltare "da lontano" (task brief) senza affaticare l'occhio sugli
   oggetti comuni/non-comuni che restano un anello fermo. */
static void DrawItemRarityRing(Vector2 pos, Rarity rarity)
{
    Color rc = RarityColor(rarity);
    const float radius = 25.0f;
    if (rarity == RARITY_RARE || rarity == RARITY_LEGENDARY)
    {
        float pulse = (sinf((float)GetTime()*3.0f) + 1.0f)*0.5f;   /* 0..1 */
        DrawCircleV(pos, radius + 4.0f + pulse*3.0f, GameColorWithAlpha(rc, (unsigned char)(35.0f + pulse*35.0f)));
    }
    DrawCircleLines((int)pos.x, (int)pos.y, radius, rc);
    DrawCircleLines((int)pos.x, (int)pos.y, radius - 1.0f, rc);
}

static void DrawPickup(Game *game, const Pickup *p)
{
    float bob = sinf((float)GetTime()*4.0f + p->pos.x*0.01f)*3.5f;
    Vector2 pos = { p->pos.x, p->pos.y + bob };
    Color c = RAYWHITE;
    const char *label = "";
    /* Come in DrawEnemy: drew dice se l'atlas ha davvero disegnato la cella
       di QUESTO pickup, non solo se l'atlas e' caricato. Una cella vuota
       (gate di qualita' fallito) deve far ripiegare sulla forma geometrica,
       mai lasciare il pickup invisibile (l'uscita PICKUP_EXIT non aveva
       nemmeno un'etichetta di riserva: senza sprite era del tutto invisibile). */
    bool drew = false;
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
        drew = DrawAtlasCell(game, cell, pos, size, WHITE);
    }
    if (!drew)
    {
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
    }
    /* Anello di rarita' SOLO per i pickup di oggetto (design doc, sezione 6):
       le altre raccolte (cuore/moneta/bomba/chiave/uscita) non hanno una
       Rarity significativa e restano invariate. Disegnato qui, DOPO lo
       sprite/forma ma PRIMA delle etichette sotto, cosi' il testo (nome o
       costo in monete) resta sempre leggibile sopra l'anello. */
    if (p->kind == PICKUP_ITEM) DrawItemRarityRing(pos, p->item.rarity);
    if (p->kind != PICKUP_ITEM && p->kind != PICKUP_EXIT) DrawText(label, (int)pos.x - 6, (int)pos.y - 8, 14, BLACK);
    if (p->cost > 0) DrawText(TextFormat("%dc", p->cost), (int)pos.x - 11, (int)pos.y + 24, 14, GOLD);
    else if (p->kind == PICKUP_ITEM) DrawText(label, (int)pos.x - 55, (int)pos.y + 24, 12, RAYWHITE);
}

/* Lo stickman minimale e FISSO: il personaggio base, mai generato (vision
   doc, docs/superpowers/specs/2026-07-13-items-synergy-vision.md, sezione
   3; APPUNTI.md sezioni 4 e 6, "la tela vuota"). Decisione esplicita del
   proprietario: la base NON usa mai lo sprite generato SPR_PLAYER, anche
   quando l'atlas e' caricato e la cella e' presente. Due motivi, entrambi
   nel documento: (1) agganci affidabili per i layer degli oggetti — se il
   personaggio base cambiasse ad ogni run generata, DrawEquipment non
   saprebbe piu' dove mettere cappello/occhiali con certezza; (2) massima
   semplicita' — il personaggio non deve rubare la scena agli oggetti che ci
   si mette sopra. Percio' questo E' il fallback storico (quello che prima
   scattava solo se l'atlas mancava o la cella era vuota), promosso a UNICO
   percorso: SPR_PLAYER resta un indice valido nell'atlas (melting-sprites
   continua a generare quella cella) per non rompere il layout, ma
   DrawPlayer non la disegna mai piu'. Un domani, se si vorra' un
   interruttore per tornare allo sprite generato come base, e' un secondo
   percorso esplicito da aggiungere qui — non il default. */
static void DrawBaseStickman(Vector2 pos, Color tint)
{
    DrawLineEx((Vector2){ pos.x, pos.y - 22 }, (Vector2){ pos.x, pos.y + 14 }, 5, tint);
    DrawCircleV((Vector2){ pos.x, pos.y - 30 }, 10, tint);
    DrawLineEx((Vector2){ pos.x - 18, pos.y - 4 }, (Vector2){ pos.x + 18, pos.y - 4 }, 4, tint);
    DrawLineEx((Vector2){ pos.x, pos.y + 14 }, (Vector2){ pos.x - 15, pos.y + 31 }, 4, tint);
    DrawLineEx((Vector2){ pos.x, pos.y + 14 }, (Vector2){ pos.x + 15, pos.y + 31 }, 4, tint);
}

/* Costruisce la lista dei layer (item_layers.h) e la disegna nell'ordine in
   cui BuildItemLayers l'ha gia' scritta: corpo/mantello (dietro la base),
   POI la base stessa, POI mano/occhi/cappello/aura (davanti). Un buffer
   sullo stack dimensionato su MAX_ITEMS basta sempre, perche' BuildItemLayers
   non puo' mai scrivere piu' elementi di quanti oggetti attivi esistano —
   zero allocazioni per frame. */
static void DrawEquipment(const Player *p, Vector2 pos, Color tint)
{
    PlayerAnchors anchors = PlayerComputeAnchors(pos, p->radius);
    ItemLayer layers[MAX_ITEMS];
    int count = BuildItemLayers(p->items, p->itemCount, layers, MAX_ITEMS);

    int i = 0;
    for (; i < count && ItemLayerIsBehindBase(layers[i].slot); i++) DrawItemLayer(anchors, layers[i]);
    DrawBaseStickman(pos, tint);
    for (; i < count; i++) DrawItemLayer(anchors, layers[i]);
}

static void DrawPlayer(Game *game)
{
    Player *p = &game->player;
    Color tint = (p->invuln > 0.0f && ((int)(GetTime()*18.0)%2 == 0)) ? GameColorWithAlpha(WHITE, 115) : WHITE;
    DrawEquipment(p, p->pos, tint);
}

/* Il vecchio DrawHud (titolo, FPS, "Piano X/Y HP.. Monete.. Bombe.. Chiavi..",
   riga del tema, minimappa) e' stato tolto (GUI fix, step A,
   docs/superpowers/specs/2026-07-14-feedback-roadmap.md punto 1): duplicava
   in pieno il pannello sinistro "RUN" (titolo/tema/piano/stanza/minimappa,
   vedi DrawOuterUi) e il pannello destro "GIOCATORE" (HP/monete/bombe/chiavi).
   La vista centrale ora mostra solo gameplay -- stanza, entita', proiettili,
   effetti -- mai le stesse statistiche gia' leggibili nei pannelli laterali.
   Il nome/vita del boss restano sulla vista (DrawEnemy): quello e' overlay
   di combattimento sopra il nemico stesso, non un duplicato dei pannelli.
   Gli FPS, utili in debug ma non gameplay, si sono spostati nell'angolo del
   pannello "GIOCATORE" (vedi DrawOuterUi). */

/* Messaggio di gioco transitorio (oggetto raccolto, porta bloccata, ecc.):
   resta SOLO qui, vicino all'azione nella vista centrale -- e' l'unica sede
   di questa informazione. Il pannello "LOG" in basso (DrawOuterUi) mostrava
   lo stesso testo: ora mostra un suggerimento fisso, cosi' il messaggio non
   compare due volte a schermo. Via anche la riga dei comandi che stava qui
   sotto: i comandi vivono gia' nel pannello sinistro ("COMANDI"). */
static void DrawTransientMessage(Game *game)
{
    if (game->messageTimer <= 0.0f) return;
    Rectangle box = { 18.0f, (float)SCREEN_HEIGHT - 46.0f, (float)SCREEN_WIDTH - 36.0f, 28.0f };
    DrawRectangleRec(box, GameColorWithAlpha(BLACK, 160));
    DrawText(game->message, (int)box.x + 10, (int)box.y + 6, 15, RAYWHITE);
}

/* Un colpo, disegnato secondo la sua FORMA (step C, core/shot_type.h). Le forme
   non sono cinque colori diversi: sono cinque disegni diversi, perche' il punto
   del feedback che ha aperto questa fase era "un tipo di colpo nuovo deve avere
   un ASPETTO diverso e un COMPORTAMENTO diverso". Il comportamento vive in
   combat.c (pallettoni, perforazione, catena, moltiplicatori); l'aspetto e' qui.
   SHOT_FORM_ORB e' lo zero-default: ogni colpo nemico, ogni colpo generato da uno
   script Lua e ogni colpo di una run senza tipi di colpo passa di qui e viene
   disegnato ESATTAMENTE come prima di questa fase (due cerchi). */
static void DrawShot(const Shot *shot)
{
    Color halo = GameColorWithAlpha(shot->color, 80);
    float angle = atan2f(shot->vel.y, shot->vel.x);
    float angleDeg = angle*180.0f/PI_F;

    /* Step D: un colpo toccato da una sinergia porta un anello pulsante. E' il
       segnale VISIVO che la coppia sta lavorando davvero -- il feedback diceva
       che le sinergie non si notavano: ora si vedono a ogni colpo, non solo nel
       messaggio del momento in cui le sblocchi. Disegnato PRIMA della forma, cosi'
       resta un contorno e non copre il colpo. */
    if (shot->synergized)
    {
        float pulse = 2.0f + sinf((float)GetTime()*9.0f)*1.1f;
        DrawCircleLines((int)shot->pos.x, (int)shot->pos.y, shot->radius + pulse + 3.0f,
                        GameColorWithAlpha(RAYWHITE, 150));
    }

    switch (shot->form)
    {
        case SHOT_FORM_SPIKE:
        {
            /* Chiodo/dardo: un rettangolo allungato orientato dalla velocita',
               con la punta luminosa in testa. */
            Rectangle body = { shot->pos.x, shot->pos.y, shot->radius*3.4f, shot->radius*1.1f };
            Vector2 origin = { shot->radius*1.7f, shot->radius*0.55f };
            DrawRectanglePro(body, origin, angleDeg, shot->color);
            DrawCircleV(GameMathAdd(shot->pos, GameMathScale((Vector2){ cosf(angle), sinf(angle) }, shot->radius*1.5f)),
                        shot->radius*0.6f, GameColorWithAlpha(RAYWHITE, 190));
            break;
        }
        case SHOT_FORM_BEAM:
        {
            /* Raggio: una scia lunga e sottile dietro il colpo, con un nucleo
               chiaro. La scia sta DIETRO (verso -vel): il colpo "traccia" il suo
               percorso invece di occupare spazio davanti a se'. */
            Vector2 back = GameMathAdd(shot->pos, GameMathScale((Vector2){ cosf(angle), sinf(angle) }, -shot->radius*7.0f));
            DrawLineEx(back, shot->pos, shot->radius*1.6f, halo);
            DrawLineEx(back, shot->pos, shot->radius*0.7f, GameColorWithAlpha(RAYWHITE, 210));
            DrawCircleV(shot->pos, shot->radius*0.9f, shot->color);
            break;
        }
        case SHOT_FORM_ARC:
        {
            /* Scarica: una spezzata a zig-zag dietro il colpo. L'oscillazione
               dipende dal tempo, cosi' la scarica "sfrigola" invece di essere una
               linea rigida. */
            Vector2 dir = { cosf(angle), sinf(angle) };
            Vector2 perp = GameMathPerpendicular(dir);
            Vector2 prev = shot->pos;
            for (int seg = 1; seg <= 4; seg++)
            {
                float back = -shot->radius*1.9f*(float)seg;
                float wobble = sinf((float)GetTime()*38.0f + (float)seg*2.1f)*shot->radius*1.15f*((seg%2) ? 1.0f : -1.0f);
                Vector2 next = GameMathAdd(shot->pos, GameMathAdd(GameMathScale(dir, back), GameMathScale(perp, wobble)));
                DrawLineEx(prev, next, shot->radius*0.75f, seg == 1 ? shot->color : halo);
                prev = next;
            }
            DrawCircleV(shot->pos, shot->radius*0.85f, GameColorWithAlpha(RAYWHITE, 200));
            break;
        }
        case SHOT_FORM_BLADE:
        {
            /* Lama: un quadrato che ruota su se' stesso (rotazione dal tempo, non
               dalla direzione: e' cio' che la fa leggere come "che gira"). */
            float spin = (float)GetTime()*680.0f;
            float side = shot->radius*2.4f;
            Rectangle body = { shot->pos.x, shot->pos.y, side, side };
            Vector2 origin = { side*0.5f, side*0.5f };
            DrawCircleV(shot->pos, shot->radius + 3.0f, halo);
            DrawRectanglePro(body, origin, spin, shot->color);
            DrawRectanglePro((Rectangle){ shot->pos.x, shot->pos.y, side*0.45f, side*0.45f }, (Vector2){ side*0.225f, side*0.225f },
                             -spin, GameColorWithAlpha(RAYWHITE, 170));
            break;
        }
        case SHOT_FORM_ORB:
        default:
            DrawCircleV(shot->pos, shot->radius + 3.0f, halo);
            DrawCircleV(shot->pos, shot->radius, shot->color);
            break;
    }
}

/* Una cosa da disegnare, con la sua profondita' (step E, trucco 2). L'ordine di
   disegno non e' piu' per CATEGORIA (tutti i pickup, poi tutti i nemici, poi il
   giocatore -- che faceva coprire il giocatore "davanti" da un nemico "dietro")
   ma per POSIZIONE: chi ha la y piu' grande e' piu' vicino all'osservatore e va
   disegnato per ultimo, quindi davanti. */
typedef enum DepthKind { DEPTH_PICKUP, DEPTH_BOMB, DEPTH_ENEMY, DEPTH_PLAYER } DepthKind;

typedef struct DepthEntry {
    float y;
    DepthKind kind;
    int index;
} DepthEntry;

#define DEPTH_MAX (MAX_PICKUPS + MAX_BOMBS + MAX_ENEMIES + 1)

/* Insertion sort: l'array e' piccolo (al massimo ~101 voci, in pratica una
   decina) ed e' gia' quasi ordinato da un frame all'altro -- e' il caso in cui
   l'insertion sort e' imbattibile, e non serve nessuna allocazione. */
static void DepthSort(DepthEntry *list, int count)
{
    for (int i = 1; i < count; i++)
    {
        DepthEntry key = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].y > key.y)
        {
            list[j + 1] = list[j];
            j--;
        }
        list[j + 1] = key;
    }
}

static void DrawGameplayCanvas(Game *game)
{
    DrawRoom(game);

    /* Tutte le ombre PRIMA di tutte le entita': un'ombra e' sul pavimento, e sul
       pavimento deve restare -- se le si disegnasse insieme alla propria entita',
       l'ombra di chi sta davanti finirebbe SOPRA chi sta dietro. */
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        const Pickup *p = &game->pickups[i];
        if (p->active) DrawGroundShadow(p->pos, p->radius*0.85f, p->radius*0.7f, 70);
    }
    for (int i = 0; i < MAX_BOMBS; i++)
    {
        if (game->bombs[i].active) DrawGroundShadow(game->bombs[i].pos, 13.0f, 9.0f, 80);
    }
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        const Enemy *e = &game->enemies[i];
        if (e->active) DrawGroundShadow(e->pos, e->radius*0.95f, e->radius*0.62f, 90);
    }
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        const Shot *s = &game->shots[i];
        if (s->active) DrawGroundShadow(s->pos, s->radius*0.8f, s->radius*2.2f, 55);
    }
    /* Il giocatore e' l'unica entita' il cui DISEGNO non coincide col suo raggio di
       collisione: lo stickman ha i piedi a +31 px (DrawBaseStickman) mentre il
       raggio e' 14. Ancorare la sua ombra a una frazione del raggio, come per le
       altre entita', la lasciava a meta' gamba -- il giocatore sembrava sprofondato
       nel pavimento (correzione da review). PLAYER_FOOT_Y e' proprio quel +31,
       scalato se un giorno il raggio del giocatore cambiasse. */
    const float PLAYER_FOOT_Y = 31.0f;
    DrawGroundShadow(game->player.pos, game->player.radius*1.05f,
                     PLAYER_FOOT_Y*(game->player.radius/14.0f), 95);

    /* Entita' che POGGIANO sul pavimento, ordinate per profondita'. */
    DepthEntry order[DEPTH_MAX];
    int count = 0;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (game->pickups[i].active) order[count++] = (DepthEntry){ game->pickups[i].pos.y, DEPTH_PICKUP, i };
    }
    for (int i = 0; i < MAX_BOMBS; i++)
    {
        if (game->bombs[i].active) order[count++] = (DepthEntry){ game->bombs[i].pos.y, DEPTH_BOMB, i };
    }
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (game->enemies[i].active) order[count++] = (DepthEntry){ game->enemies[i].pos.y, DEPTH_ENEMY, i };
    }
    order[count++] = (DepthEntry){ game->player.pos.y, DEPTH_PLAYER, 0 };
    DepthSort(order, count);

    for (int i = 0; i < count; i++)
    {
        switch (order[i].kind)
        {
            case DEPTH_PICKUP: DrawPickup(game, &game->pickups[order[i].index]); break;
            case DEPTH_BOMB:
            {
                Bomb *b = &game->bombs[order[i].index];
                DrawCircleV(b->pos, 14.0f + sinf((float)GetTime()*10.0f)*2.0f, DARKGRAY);
                DrawCircleLines((int)b->pos.x, (int)b->pos.y, b->radius*(1.0f - b->timer/1.05f), ORANGE);
                break;
            }
            case DEPTH_ENEMY: DrawEnemy(game, &game->enemies[order[i].index]); break;
            case DEPTH_PLAYER: DrawPlayer(game); break;
        }
    }

    /* I colpi VOLANO: stanno sopra tutto cio' che poggia a terra (la loro ombra,
       gia' disegnata sul pavimento, e' cio' che dice a che altezza sono). Fuori
       dall'ordinamento per profondita' apposta: un proiettile che sparisse dietro
       un nemico sarebbe illeggibile, ed e' informazione di gioco, non scenografia. */
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        Shot *s = &game->shots[i];
        if (!s->active) continue;
        DrawShot(s);
    }
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        DrawCircleV(p->pos, p->radius, GameColorWithAlpha(p->color, (unsigned char)GameMathClampFloat(p->life*420.0f, 0.0f, 255.0f)));
    }

    DrawVignette();
    DrawTransientMessage(game);

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
    /* Bordo della riga = colore della rarita' (design doc, sezione 6: "si
       vede col colore del bordo... nel pannello"), non piu' il colore
       proprio dell'oggetto (item->color resta usato per il layer sul
       personaggio e per la forma di riserva qui sotto -- solo il bordo del
       PANNELLO passa alla rarita'). 2px invece di 1: leggibile anche a
       sguardo veloce, senza diventare invadente sulle righe COMUNI/grigie. */
    Color rarityColor = RarityColor(item->rarity);
    Rectangle row = { (float)x, (float)y, (float)width, 58.0f };
    DrawRectangleRec(row, owned ? (Color){ 28, 32, 40, 220 } : (Color){ 24, 27, 34, 210 });
    DrawRectangleLinesEx(row, 2.0f, GameColorWithAlpha(rarityColor, 200));
    if (!DrawAtlasCell(game, SPR_ITEM, (Vector2){ x + 28.0f, y + 29.0f }, 36.0f, WHITE))
    {
        DrawItemShape((Vector2){ x + 28.0f, y + 29.0f }, *item, 12.0f);
    }
    DrawText(item->name, x + 55, y + 9, 14, RAYWHITE);
    /* Nome della rarita' in coda alla riga slot/traits, nel suo colore (design
       doc, sezione 6: "...e col nome nel pannello"): due DrawText invece di
       uno solo cosi' SOLO il nome della rarita' prende il suo colore, il
       resto della riga resta nel grigio neutro gia' in uso. */
    char slotTraits[144];
    snprintf(slotTraits, sizeof(slotTraits), "%s  |  %s  |  ", SlotName(item->slot), traits);
    DrawText(slotTraits, x + 55, y + 31, 12, (Color){ 190, 198, 211, 255 });
    int rarityTextX = x + 55 + MeasureText(slotTraits, 12);
    DrawText(RarityName(item->rarity), rarityTextX, y + 31, 12, rarityColor);
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
    DrawStatLine("Fonte", game->content.loaded ? "LLM cache" : "fallback", lx, ly + 140, RAYWHITE);
    DrawText("MAPPA", lx, ly + 184, 16, game->theme.accent2);
    DrawLargeMinimap(game, (Rectangle){ (float)lx, (float)(ly + 214), layout.leftPanel.width - 36.0f, 190.0f });
    DrawText("COMANDI", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 142), 16, game->theme.accent2);
    DrawText("WASD muovi", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 112), 14, RAYWHITE);
    DrawText("Mouse/Frecce spara", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 90), 14, RAYWHITE);
    DrawText("SPACE bomba", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 68), 14, RAYWHITE);
    DrawText("ESC/P pausa   F11 fullscreen", lx, (int)(layout.leftPanel.y + layout.leftPanel.height - 46), 14, RAYWHITE);

    DrawPanel(layout.rightPanel, "GIOCATORE", game->theme.accent2);
    /* FPS: utile in debug, non e' gameplay -- spostato qui, angolo in alto a
       destra del pannello, invece che sopra la scena (GUI fix, step A).
       Piccolo e defilato apposta, non deve competere con le statistiche. */
    const char *fpsText = TextFormat("%d FPS", GetFPS());
    int fpsW = MeasureText(fpsText, 13);
    DrawText(fpsText, (int)(layout.rightPanel.x + layout.rightPanel.width - 16.0f) - fpsW,
             (int)layout.rightPanel.y + 12, 13, (Color){ 126, 232, 152, 255 });
    int rx = (int)layout.rightPanel.x + 18;
    int ry = (int)layout.rightPanel.y + 48;
    Player *p = &game->player;
    DrawStatLine("HP", TextFormat("%d / %d", p->hp, p->maxHp), rx, ry, RED);
    DrawStatLine("Danno", TextFormat("%.1f", p->damage), rx, ry + 24, RAYWHITE);
    DrawStatLine("Cadenza", TextFormat("%.2fs", p->fireDelay), rx, ry + 48, RAYWHITE);
    DrawStatLine("Vel. colpo", TextFormat("%.0f", p->shotSpeed), rx, ry + 72, RAYWHITE);
    DrawStatLine("Raggio", TextFormat("%.1f", p->shotRadius), rx, ry + 96, RAYWHITE);
    /* Step C: le due statistiche nuove. La fortuna col segno esplicito (puo'
       essere negativa), il tipo di colpo attivo col colore dell'oggetto che l'ha
       dato -- lo stesso colore con cui il colpo si disegna in scena, cosi' il
       collegamento "questo oggetto -> questo proiettile" e' immediato. Il nome lo
       ha inventato il modello: la GUI non lo interpreta, lo mostra e basta. */
    DrawStatLine("Fortuna", TextFormat("%+.1f", p->luck), rx, ry + 120, (Color){ 126, 232, 152, 255 });
    if (p->shotType.active)
    {
        DrawStatLine("Colpo", TextFormat("%s (%s)", p->shotType.name, ShotFormName(p->shotType.form)), rx, ry + 144, p->shotColor);
    }
    else
    {
        DrawStatLine("Colpo", "base", rx, ry + 144, (Color){ 155, 163, 176, 255 });
    }
    DrawStatLine("Risorse", TextFormat("%dc  %db  %dk", p->coins, p->bombs, p->keys), rx, ry + 168, GOLD);

    DrawText("OGGETTI PRESI", rx, ry + 212, 16, game->theme.accent2);
    int rowY = ry + 242;
    int shown = 0;
    /* Cinque righe invece di sei: le due statistiche nuove sopra costano 48px, e
       questa lista e' l'unico blocco elastico del pannello (l'anteprima del piano
       sotto e' ancorata al fondo). Meglio un oggetto in meno mostrato che due
       blocchi sovrapposti. */
    int start = p->itemCount > 5 ? p->itemCount - 5 : 0;
    for (int i = start; i < p->itemCount && shown < 5; i++, shown++)
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
    /* Il messaggio transitorio (raccolta oggetto, porta bloccata, ecc.) vive
       SOLO nella vista centrale, vicino all'azione (DrawTransientMessage): qui
       NON si ripete mai (GUI fix, step A). Questo spazio ospita invece le
       SINERGIE ATTIVE (step D): sono lo stato piu' importante di una build e
       finora non erano visibili da nessuna parte. Riga fissa di suggerimento solo
       quando non ce n'e' ancora nessuna. */
    /* Colonne calcolate dalla LARGHEZZA VERA del pannello, non da un letterale
       (correzione da review): il pannello e' elastico (UiComputeLayout lo dimensiona
       sulla finestra), e con un passo fisso di 300 px le sinergie finivano scritte
       fuori dal suo bordo destro su una finestra stretta. Stessa cosa in verticale:
       la riga di descrizione si disegna solo se il pannello e' abbastanza alto. */
    int usableW = (int)layout.bottomPanel.width - 36;
    int colW = usableW/3;
    int maxCols = (colW > 140) ? 3 : ((usableW > 280) ? 2 : 1);
    if (maxCols > 1) colW = usableW/maxCols;
    bool roomForDescription = (by + 46 + 13) < (int)(layout.bottomPanel.y + layout.bottomPanel.height - 6);
    int activeSynergies = 0;
    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        if (!(game->player.synergies & (1u << i))) continue;
        int col = bx + activeSynergies*colW;
        DrawText(TextFormat("* %s", SynergyName(i)), col, by + 26, 16, GOLD);
        if (roomForDescription) DrawText(SynergyDescription(i), col, by + 46, 13, (Color){ 200, 206, 216, 255 });
        activeSynergies++;
        if (activeSynergies >= maxCols) break;   /* le altre restano ATTIVE: solo non elencate, il pannello non e' la verita' */
    }
    if (activeSynergies == 0)
    {
        DrawText("Nessuna sinergia attiva: raccogli oggetti che si combinano.", bx, by + 28, 16, RAYWHITE);
    }
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
        DrawText("Run generata in locale: testo LLM, sprite Stable Diffusion.", (int)box.x + 62, (int)box.y + 98, 18, game->theme.accent2);
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

void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress, const char *screenshotPath)
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
    /* screenshotPath e' del chiamante (mai NULL quando takeScreenshot e'
       vero, vedi game_renderer.h): --screenshot-test continua a scrivere
       logs/melting-run-screen.png esattamente come prima (vedi app.c),
       questo parametro serve solo a chi (come --layer-test, vedi
       src/tests/game_tests.c) vuole un frame catturato altrove, senza
       toccare quel file. */
    if (takeScreenshot)
    {
        rlDrawRenderBatchActive();
        TakeScreenshot(screenshotPath);
    }
    EndDrawing();
}
