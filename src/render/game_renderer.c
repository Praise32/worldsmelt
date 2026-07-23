#include "render/game_renderer.h"

#include "content/character_roster.h"
#include "core/game_math.h"
#include "game/game.h"
#include "game/game_internal.h"
#include "gameplay/synergies.h"
#include "render/item_layers.h"
#include "render/rarity_style.h"

#include "rlgl.h"
#include "raygui.h"   /* solo dichiarazioni: l'implementazione e' in src/render/raygui_impl.c */

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Fase 4 (GUI completa con raygui): applica al tema globale di raygui i colori
   della run corrente, cosi' i pannelli/etichette/righe hanno l'estetica scura del
   gioco invece del grigio di default di raygui. Chiamata una volta per frame prima
   di disegnare la GUI (costa niente: solo GuiSetStyle, che scrive in un array
   globale). I colori si passano a raygui come interi 0xRRGGBBAA (ColorToInt). */
/* M4: arrotondamento comune per font/spessori scalati da uiScale -- una
   dimensione di testo o una linea non puo' restare frazionaria (DrawText/
   DrawRectangleLinesEx vogliono un int), e va arrotondata al piu' vicino
   (non troncata) perche' un font che si aggancia sempre per difetto sembra
   sistematicamente piu' piccolo di quanto la scala richiederebbe. */
static int UiRound(float v)
{
    return (int)(v + 0.5f);
}

/* M4: 'uiScale' arrotonda il TEXT_SIZE di raygui (GuiLabel/GuiPanel/GuiStatusBar
   -- tutto cio' che raygui disegna da solo, a differenza dei DrawText diretti
   di DrawOuterUi/gli overlay, che si scalano ciascuno per conto proprio, vedi
   UiRound piu' sotto). */
static void UiApplyTheme(const Theme *theme, float uiScale)
{
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(15.0f*uiScale + 0.5f));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt((Color){ 16, 18, 24, 255 }));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(GameColorWithAlpha(theme->accent, 120)));
    /* Pannelli e riquadri: sfondo scuro semitrasparente, bordo nel colore accento. */
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt((Color){ 20, 22, 29, 236 }));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(GameColorWithAlpha(theme->accent, 170)));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt((Color){ 224, 228, 236, 255 }));
    GuiSetStyle(STATUSBAR, TEXT_COLOR_NORMAL, ColorToInt((Color){ 224, 228, 236, 255 }));
    GuiSetStyle(STATUSBAR, BASE_COLOR_NORMAL, ColorToInt((Color){ 24, 27, 34, 236 }));
    GuiSetStyle(STATUSBAR, BORDER_COLOR_NORMAL, ColorToInt(GameColorWithAlpha(theme->accent2, 150)));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
}

/* M4 (fullscreen-first): fattore di scala dell'interfaccia ESTERNA (pannelli,
   font, overlay dei menu -- mai il canvas 960x640, che ha la sua scala a parte,
   vedi 'scale' sotto). Derivato dalla sola ALTEZZA dello schermo (la larghezza
   varia troppo con l'ultrawide per essere un indizio affidabile di "quanto e'
   grande lo schermo"): 900px di altezza -> 1.0, cioe' l'aspetto di sempre prima
   di M4 (la finestra grande di riferimento del progetto e' 1600x900, vedi
   APP_WINDOW_WIDTH/HEIGHT in core/game_types.h). Sopra i 900px cresce fino a un
   tetto di 3.0 (oltre, l'interfaccia diventerebbe piu' un ostacolo che un aiuto
   anche su un 4K). QUANTIZZATO a passi di 0.25 con un floor (non arrotondato):
   stessa scelta della scala del canvas qui sotto, un valore continuo farebbe
   "respirare" pannelli e font ad ogni pixel di resize invece di scattare fra
   pochi valori stabili. 1080p (rapporto 1.2) cade quindi a 1.0 (floor di 4.8/4),
   1440p (1.6) a 1.5, 2160p (2.4) a 2.25: coerente col commento del task e con
   quanto verifica --layout-test (punto c, non-regressione a 1600x900). */
static float UiScaleForHeight(float sh)
{
    float raw = GameMathClampFloat(sh/900.0f, 1.0f, 3.0f);
    return floorf(raw*4.0f)/4.0f;
}

UiLayout UiComputeLayoutFor(float sw, float sh)
{
    float uiScale = UiScaleForHeight(sh);
    /* DEC-137: una sola superficie. La game view riempie TUTTO lo schermo --
       niente piu' colonne sottratte alla UI, che ora vive in overlay sopra il
       canvas (DrawOuterUi). Il canvas resta pixel-perfect con la STESSA
       disciplina di prima: si sceglie il piu' grande passo di 1/8 che ci sta
       INTERO (fit, mai crop -- la stanza e' fissa 960x640 senza camera, tagliarla
       nasconderebbe gameplay), e lo si centra. Sparita ogni riserva di spazio per
       i pannelli: lo spazio libero e' l'intero schermo. */
    float scale = fminf(sw/(float)SCREEN_WIDTH, sh/(float)SCREEN_HEIGHT);
    /* Scala AGGANCIATA a passi di 1/8 (mai continua): il canvas e' campionato
       con filtro POINT (vedi app.c), e a scala continua i pixel raddoppiati
       cadrebbero a distanze irregolari che "brillano" quando la finestra
       cambia o la camera si muove. A passi di 1/8 la cadenza dei pixel
       raddoppiati e' fissa e regolare. Il floor sceglie il passo INFERIORE:
       meglio una cornice di margine in piu' (le bande che restano dal rapporto
       3:2 del canvas su uno schermo 16:9) che tagliare la stanza. */
    scale = floorf(scale*8.0f)/8.0f;
    /* M4: nessun tetto sulla scala -- la game view deve riempire lo schermo su
       1440p/4K. Resta SOLO il minimo 0.75, a guardia delle finestre ridotte a
       mano sotto la finestra di test compatta (960x640, dove scale vale gia' 1.0
       esatto). Le bande dal rapporto 3:2 le riempie ClearBackground (scuro), e
       l'HUD in overlay ci si appoggia sopra ai bordi di gameRect. */
    if (scale < 0.75f) scale = 0.75f;
    float gw = (float)SCREEN_WIDTH*scale;
    float gh = (float)SCREEN_HEIGHT*scale;

    UiLayout layout = { 0 };
    layout.gameRect = (Rectangle){ (sw - gw)*0.5f, (sh - gh)*0.5f, gw, gh };
    layout.gameScale = scale;
    layout.uiScale = uiScale;
    return layout;
}

UiLayout UiComputeLayout(void)
{
    return UiComputeLayoutFor((float)GetScreenWidth(), (float)GetScreenHeight());
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
        /* M1b: colore ambra/braci, distinto dalle stanze di un piano vero --
           coerente con l'aspetto "curato e distinto" richiesto per il
           crogiolo (DEC-067), anche sulla minimappa (che qui mostra una
           sola cella). */
        case ROOM_HUB: return (Color){ 224, 140, 62, 255 };
        default: return (Color){ 40, 44, 50, 255 };
    }
}

/* ============================================================
   Resa 2.5D (step E, docs/engineering/specs/2026-07-14-feedback-roadmap.md
   punto 5, e docs/archive/legacy-notes/appunti.md sezione 7).
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

/* Spessore DECORATIVO dei muri (solo resa, nessuna collisione cambia): il
   campo di gioco vero e' il rettangolo della stanza CORRENTE (M2,
   WorldCurrentRoomRect), non piu' sempre ROOM_X..ROOM_RIGHT/ROOM_Y..
   ROOM_BOTTOM -- quello resta il bordo del canvas, oltre il quale i muri
   riempiono comunque tutto (vedi DrawRoom). Il muro di fondo e' piu' alto
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

/* Piano 0 (M1b): il varco verso il piano 1 non e' una porta normale (vedi il
   commento su DrawRoom sotto e su WorldHandleTransitions in world.c) -- ha un
   aspetto dedicato che DEVE distinguersi visibilmente fra chiuso e aperto
   (KB systems/floor-zero.md, "Feedback": "l'uscita si apre visibilmente solo
   quando diventa abilitata"). Chiuso: sbarrato con strisce diagonali (un
   cantiere, non un buco nel muro). Aperto: luminoso e pulsante -- lo stesso
   punto dove EntitiesAddParticle spruzza il burst all'apertura (vedi
   AppOpenFloorZeroExit, src/app/app.c), cosi' il bagliore e le particelle
   coincidono. */
static void DrawFloorZeroExitGate(Game *game)
{
    float cx = ROOM_X + ROOM_W*0.5f;
    Rectangle gate = { cx - DOOR_HALF, ROOM_Y - WALL_BACK_H, DOOR_HALF*2.0f, WALL_BACK_H };
    if (game->floorZeroExitOpen)
    {
        float pulse = (sinf((float)GetTime()*2.4f) + 1.0f)*0.5f;
        Color glow = GameColorLerp(game->theme.accent2, WHITE, 0.25f + pulse*0.25f);
        DrawRectangleRec(gate, glow);
        DrawRectangleLinesEx(gate, 3.0f, WHITE);
    }
    else
    {
        Color hazard = (Color){ 224, 168, 42, 255 };
        DrawRectangleRec(gate, GameColorLerp(game->theme.wall, BLACK, 0.55f));
        for (float sx = gate.x - gate.height; sx < gate.x + gate.width; sx += 16.0f)
        {
            Vector2 p1 = { sx, gate.y + gate.height };
            Vector2 p2 = { sx + gate.height, gate.y };
            /* Clamp orizzontale: le strisce nascono/muoiono fuori da 'gate' per
               coprirlo fino ai bordi, ma disegnate intere sforerebbero visibilmente
               nel muro adiacente. */
            if (p1.x < gate.x) p1.x = gate.x;
            if (p2.x > gate.x + gate.width) p2.x = gate.x + gate.width;
            DrawLineEx(p1, p2, 3.0f, hazard);
        }
        DrawRectangleLinesEx(gate, 2.0f, hazard);
    }
}

static void DrawRoom(Game *game)
{
    ClearBackground(game->theme.bg);

    /* M2 (DEC-009): la stanza VERA (WorldCurrentRoomRect) puo' essere piu'
       piccola del rettangolo massimo del canvas (ROOM_X/Y/W/H, che resta
       fisso -- nessuna camera). Si riempie PRIMA l'intero massimo di muro
       scuro, poi ci si disegna sopra il pavimento della stanza: lo spazio fra
       il bordo della stanza e il bordo del canvas resta automaticamente muro,
       qualunque sia la cornice, senza calcolare margini variabili per lato
       (per il Piano 0/hub, che non ha una taglia impostata, la stanza
       coincide col massimo: questa cornice e' quindi vuota, esattamente come
       prima di questa fase). */
    Color wallDark = GameColorLerp(game->theme.wall, BLACK, 0.45f);
    Color wallLit = GameColorLerp(game->theme.wall, WHITE, 0.18f);
    DrawRectangleRec((Rectangle){ ROOM_X - WALL_SIDE_W, ROOM_Y - WALL_BACK_H,
                                   ROOM_W + WALL_SIDE_W*2.0f, ROOM_H + WALL_BACK_H + WALL_FRONT_H }, wallDark);

    Rectangle roomRect = WorldCurrentRoomRect(game);
    const float rx = roomRect.x, ry = roomRect.y, rw = roomRect.width, rh = roomRect.height;
    const float rRight = rx + rw, rBottom = ry + rh;

    /* Pavimento: tinta piena, poi una sfumatura che scurisce il FONDO della
       stanza (la parte lontana). E' la stessa cosa che fa l'atmosfera in
       prospettiva: cio' che e' lontano perde contrasto. */
    DrawRectangleRec(roomRect, game->theme.floor);
    DrawRectangleGradientV((int)rx, (int)ry, (int)rw, (int)(rh*0.45f),
                           GameColorWithAlpha(BLACK, 58), BLANK);

    /* Griglia in prospettiva: le linee "verticali" convergono verso un punto di
       fuga sopra il muro di fondo, quelle orizzontali si infittiscono verso il
       fondo (spaziatura non lineare). Il punto di fuga sta FUORI dalla stanza, in
       alto: e' cio' che fa leggere il pavimento come un piano inclinato sotto lo
       sguardo, invece che come un rettangolo visto di fronte. */
    const float vpx = rx + rw*0.5f;
    const float vpy = ry - 340.0f;
    Color gridColor = GameColorWithAlpha(game->theme.accent, 34);
    for (int i = 0; i <= 14; i++)
    {
        float x = rx + rw*(float)i/14.0f;
        Vector2 near = { x, rBottom };
        /* Quanto si e' "risaliti" verso il punto di fuga arrivando al muro di
           fondo: t = 0 al bordo vicino, cresce verso il punto di fuga. */
        float t = (rBottom - ry)/(rBottom - vpy);
        Vector2 far = { near.x + (vpx - near.x)*t, ry };
        DrawLineEx(near, far, 1.0f, gridColor);
    }
    for (int i = 1; i < 9; i++)
    {
        float f = (float)i/9.0f;
        float y = ry + (rBottom - ry)*powf(f, 1.7f);   /* piu' fitte in alto = piu' lontane */
        DrawLine((int)rx, (int)y, (int)rRight, (int)y, gridColor);
    }

    /* Muri con spessore, ancorati al bordo REALE della stanza (non piu' al
       massimo fisso). Il muro di FONDO e' l'unico di cui si vede la faccia
       (una fascia sopra il pavimento, piu' chiara in alto dove prende luce, con
       uno spigolo netto in basso dove incontra il pavimento). I laterali e quello
       davanti mostrano solo il loro spessore, senza faccia: e' esattamente cio'
       che si vedrebbe da una camera inclinata di poco. Il resto della cornice
       (fra questo spessore decorativo e il bordo del canvas) e' gia' muro scuro
       piatto grazie al riempimento fatto sopra. */
    DrawRectangleGradientV((int)(rx - WALL_SIDE_W), (int)(ry - WALL_BACK_H),
                           (int)(rw + WALL_SIDE_W*2.0f), (int)WALL_BACK_H, wallLit, wallDark);
    DrawRectangle((int)(rx - WALL_SIDE_W), (int)(ry - 3.0f), (int)(rw + WALL_SIDE_W*2.0f), 3, wallDark);
    DrawRectangle((int)(rx - WALL_SIDE_W), (int)ry, (int)WALL_SIDE_W, (int)rh, wallDark);
    DrawRectangle((int)rRight, (int)ry, (int)WALL_SIDE_W, (int)rh, wallDark);
    DrawRectangle((int)(rx - WALL_SIDE_W), (int)rBottom,
                  (int)(rw + WALL_SIDE_W*2.0f), (int)WALL_FRONT_H, wallDark);
    /* Spigolo illuminato dei muri laterali/davanti: la linea sottile che fa
       leggere lo spessore come uno spessore e non come una cornice piatta. */
    DrawRectangle((int)(rx - WALL_SIDE_W), (int)rBottom, (int)(rw + WALL_SIDE_W*2.0f), 2, wallLit);

    const RoomState *room = GameCurrentRoom(game);
    float cx = rx + rw*0.5f;
    float cy = ry + rh*0.5f;
    Color doorColor = GameRoomIsLocked(game) ? (Color){ 200, 58, 58, 255 } : game->theme.accent2;
    /* Le porte si disegnano SOPRA i muri (sono buchi nel muro), CENTRATE sulle
       pareti REALI della stanza (M2: non piu' sempre allo stesso punto dello
       schermo se la stanza e' piu' piccola): quella di fondo occupa tutta la
       faccia del muro, cosi' si legge come un passaggio e non come una
       striscia appoggiata. */
    if (room->doors[DIR_UP]) DrawRectangle((int)(cx - DOOR_HALF), (int)(ry - WALL_BACK_H), (int)(DOOR_HALF*2), (int)WALL_BACK_H, doorColor);
    if (room->doors[DIR_DOWN]) DrawRectangle((int)(cx - DOOR_HALF), (int)rBottom, (int)(DOOR_HALF*2), (int)WALL_FRONT_H, doorColor);
    if (room->doors[DIR_LEFT]) DrawRectangle((int)(rx - WALL_SIDE_W), (int)(cy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);
    if (room->doors[DIR_RIGHT]) DrawRectangle((int)rRight, (int)(cy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);

    /* Piano 0 (M1b): il varco verso il piano 1, nel muro di fondo. NON e' un
       room->doors[DIR_UP] (vedi FloorZeroEnter, src/world/floor_zero.c: la
       griglia ha una sola cella, quindi quell'array resta tutto falso) --
       ha un aspetto dedicato apposta, cosi' resta leggibile come "l'uscita
       speciale della sala d'attesa" anche a chi non guarda la minimappa. Usa
       ancora ROOM_X/ROOM_W fissi (non roomRect): il Piano 0 non e' generato,
       la sua stanza coincide sempre col massimo (vedi il commento sopra). */
    if (game->floor == 0) DrawFloorZeroExitGate(game);
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
       disegni lo sprite sia la forma di riserva. M2: ancorato all'angolo
       della stanza CORRENTE (in pratica coincide sempre col massimo, la
       stanza boss e' sempre alla taglia massima -- ma leggerlo dall'accessore
       invece che dalla macro lo rende corretto per costruzione, non per
       coincidenza). */
    if (e->kind == ENEMY_BOSS)
    {
        Rectangle bossRoom = WorldCurrentRoomRect(game);
        DrawText(game->theme.bossName, (int)(bossRoom.x + 20.0f), (int)(bossRoom.y + 16.0f), 18, RAYWHITE);
    }
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
   doc, docs/engineering/specs/2026-07-13-items-synergy-vision.md, sezione
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
    /* M6a (requisito 3): lo stickman usa la palette del personaggio scelto
       invece del WHITE fisso di sempre -- MA solo quando un personaggio e'
       davvero stato applicato (GameResolveCharacterDef non-NULL, scritto dal
       Piano 0 o dall'attraversamento, vedi il commento su
       Game.characterChosenIndex in core/game_types.h). NULL (nessun
       personaggio: GameResetRun chiamata fuori dal cammino del Piano 0, es.
       molti test; o -1 storico) ricade sul WHITE storico -- da M6b-1
       GameResolveCharacterDef e' anche l'unico punto che sa risolvere
       CHARACTER_COUNT (il personaggio generato) alla sua Color vera, invece
       della palette di Wayfinder che CharacterRosterGet da sola avrebbe
       dato per un indice fuori dalla rosa. Il flash di invulnerabilita' si
       COMPONE sopra la tinta (alpha ridotto sullo stesso colore), non la
       sostituisce -- altrimenti un personaggio colpito lampeggerebbe
       bianco per un istante, tradendo la sua identita' visiva proprio nel
       momento in cui il giocatore la guarda di piu'. */
    const CharacterDef *appliedCharacter = GameResolveCharacterDef(game, game->characterChosenIndex);
    Color base = appliedCharacter ? appliedCharacter->palette : WHITE;
    Color tint = (p->invuln > 0.0f && ((int)(GetTime()*18.0)%2 == 0)) ? GameColorWithAlpha(base, 115) : base;
    DrawEquipment(p, p->pos, tint);
}

/* Il vecchio DrawHud (titolo, FPS, "Piano X/Y HP.. Monete.. Bombe.. Chiavi..",
   riga del tema, minimappa) e' stato tolto (GUI fix, step A,
   docs/engineering/specs/2026-07-14-feedback-roadmap.md punto 1): duplicava le
   stesse informazioni della UI. Il canvas mostra solo gameplay -- stanza,
   entita', proiettili, effetti -- mai le statistiche, che vivono nell'HUD in
   overlay (DEC-137, DrawOuterUi: cuori/risorse in alto a sinistra, mappa e
   progressione in alto a destra, build in basso). Il nome/vita del boss restano
   sulla vista (DrawEnemy): quello e' overlay di combattimento sopra il nemico
   stesso, non un duplicato dell'HUD. Gli FPS, utili in debug ma non gameplay,
   stanno nel cluster di stato in alto a destra (DrawHudRunStatus). */

/* Messaggio di gioco transitorio (oggetto raccolto, porta bloccata, ecc.):
   resta SOLO qui, dentro il canvas vicino all'azione -- e' l'unica sede di
   questa informazione. Il vecchio LOG a pannello mostrava lo stesso testo; il
   cluster LOG dell'HUD (DEC-137, DrawHudLog) mostra invece fonte grafica e
   architettura del piano, non i messaggi, cosi' non compaiono due volte. La
   riga dei comandi che stava qui sotto e' caduta col layout a pannelli
   (DEC-137): i comandi si imparano giocando, l'HUD in overlay resta essenziale. */
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

/* Fase 3c: gli ostacoli solidi della stanza, in 2.5D (blocchi rialzati). Ogni
   blocco ha un'ombra a terra, una FACCIA FRONTALE scura (lo spessore, verso
   l'osservatore) e una FACCIA SUPERIORE piu' chiara (la cima, dove batte la luce),
   coerenti con muri/ombre del resto della resa 2.5D (step E). Disegnati subito dopo
   il pavimento e PRIMA delle entita': sono coperture basse, le entita' ci passano
   davanti (un ordinamento per profondita' rettangolo-vs-cerchio non varrebbe la
   complessita' per coperture cosi' basse). */
static void DrawObstacles(Game *game)
{
    const float LIFT = 16.0f;   /* quanto e' "alto" il blocco: la faccia superiore e' spostata su di tanto */
    Color side = GameColorLerp(game->theme.wall, BLACK, 0.55f);
    Color top = GameColorLerp(game->theme.wall, WHITE, 0.12f);
    Color edge = GameColorLerp(game->theme.wall, BLACK, 0.30f);
    for (int i = 0; i < game->obstacleCount; i++)
    {
        Obstacle *o = &game->obstacles[i];
        /* Ombra a terra alla base del blocco. */
        DrawEllipse((int)(o->x + o->w*0.5f), (int)(o->y + o->h + 4.0f), o->w*0.55f, o->h*0.22f, (Color){ 0, 0, 0, 90 });
        /* Faccia frontale (lo spessore): dalla base del blocco giu' di LIFT. */
        DrawRectangle((int)o->x, (int)(o->y + o->h - LIFT), (int)o->w, (int)LIFT, side);
        /* Faccia superiore: il rettangolo del blocco, spostato SU di LIFT. */
        DrawRectangle((int)o->x, (int)(o->y - LIFT), (int)o->w, (int)o->h, top);
        DrawRectangleLinesEx((Rectangle){ o->x, o->y - LIFT, o->w, o->h }, 2.0f, edge);
    }
}

static void DrawGameplayCanvas(Game *game)
{
    DrawRoom(game);
    DrawObstacles(game);

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

    /* M1a: il vecchio overlay "RUN COMPLETATA/FALLITA, premi R" e' sparito da
       QUI -- fine run e' ora lo stato canonico RunResults (DrawRunResultsOverlay
       piu' sotto), che copre l'intero schermo con l'esito vero e le due voci
       canoniche ("Nuova run subito"/"Menu principale"): "premi R" era gia'
       falso appena scritto in M1a (R rigenera in Gameplay, non in fine run). */
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

static void DrawStatLine(const char *label, const char *value, int x, int y, Color color, float uiScale)
{
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt((Color){ 150, 158, 172, 255 }));
    GuiLabel((Rectangle){ (float)x, (float)y, 116.0f*uiScale, 18.0f*uiScale }, label);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(color));
    GuiLabel((Rectangle){ (float)x + 118.0f*uiScale, (float)y, 220.0f*uiScale, 18.0f*uiScale }, value);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt((Color){ 224, 228, 236, 255 }));   /* ripristina il default */
}

/* La vita come CUORI (fase 4, richiesta dell'utente): stile Isaac, un cuore pieno
   ogni due HP piu' un mezzo cuore per l'HP dispari, poi cuori vuoti fino a maxHp.
   Disegnati come due lobi + un triangolo, cosi' non servono sprite. Ritorna la
   larghezza occupata, per allineare cio' che segue. */
static void DrawHeart(float cx, float cy, float s, Color fill, bool half)
{
    /* Due lobi in alto + la punta in basso. */
    DrawCircleV((Vector2){ cx - s*0.42f, cy - s*0.18f }, s*0.46f, fill);
    if (!half) DrawCircleV((Vector2){ cx + s*0.42f, cy - s*0.18f }, s*0.46f, fill);
    DrawTriangle((Vector2){ cx - s*0.86f, cy - s*0.02f },
                 (Vector2){ cx + (half ? 0.06f : 0.86f)*s, cy - s*0.02f },
                 (Vector2){ cx - (half ? 0.4f : 0.0f)*s, cy + s*0.72f }, fill);
    if (half)
    {
        /* mezzo cuore: il lobo destro e la meta' destra restano "vuoti" -- si
           ridisegna il contorno del cuore pieno in scuro per suggerire la meta'
           mancante. */
        DrawCircleLines((int)(cx + s*0.42f), (int)(cy - s*0.18f), s*0.46f, GameColorWithAlpha(fill, 90));
    }
}

static int DrawHearts(const Player *p, int x, int y, float uiScale)
{
    const float s = 15.0f*uiScale;
    const float step = s*1.15f;
    int full = p->hp/2;
    bool half = (p->hp%2) != 0;
    int totalSlots = (p->maxHp + 1)/2;
    Color red = (Color){ 226, 72, 78, 255 };
    Color empty = (Color){ 60, 46, 52, 255 };
    int drawn = 0;
    for (int i = 0; i < totalSlots; i++)
    {
        float cx = x + s + drawn*step;
        float cy = (float)y + s*0.5f;
        if (i < full) DrawHeart(cx, cy, s, red, false);
        else if (i == full && half) { DrawHeart(cx, cy, s, empty, false); DrawHeart(cx, cy, s, red, true); }
        else DrawHeart(cx, cy, s, empty, false);
        drawn++;
    }
    return (int)(drawn*step);
}

/* Una lettera-icona al centro di una cella della minimappa per le stanze speciali
   (fase 4): T tesoro, $ negozio, B boss. Le stanze normali/di partenza restano
   vuote. Reso leggibile anche a 26px. */
static void DrawRoomIcon(RoomKind kind, Rectangle cell, Color color, float uiScale)
{
    const char *g = NULL;
    if (kind == ROOM_TREASURE) g = "T";
    else if (kind == ROOM_SHOP) g = "$";
    else if (kind == ROOM_BOSS) g = "B";
    if (!g) return;
    int fontSize = UiRound(14.0f*uiScale);
    int w = MeasureText(g, fontSize);
    DrawText(g, (int)(cell.x + cell.width*0.5f - (float)w*0.5f), (int)(cell.y + cell.height*0.5f - (float)fontSize*0.5f), fontSize, color);
}

/* M2 (DEC-009): la minimappa comunica la taglia VERA della stanza (griglia
   fissa, taglie diverse) senza scompaginare la griglia di celle a passo
   fisso (size+gap) -- si scala solo il RIQUADRO COLORATO dentro lo slot,
   lasciando bordo/icona/evidenziazione "stanza corrente" sullo slot intero,
   cosi' la mappa resta leggibile e cliccabile come sempre. Tre classi
   bastano (piccola/media/grande, come richiesto): non serve una scala
   continua per un'informazione che qui è solo un indizio a colpo d'occhio.
   w/h <= 0 (Piano 0/hub, o una RoomState di test mai passata da
   WorldGenerateFloorMap) => 1.0, la cella piena di sempre. */
static float RoomMapSizeScale(const RoomState *room)
{
    if (room->w <= 0 || room->h <= 0) return 1.0f;
    float frac = ((float)room->w*(float)room->h)/(ROOM_W*ROOM_H);
    if (frac >= 0.78f) return 1.0f;    /* grande */
    if (frac < 0.55f) return 0.7f;     /* piccola */
    return 0.85f;                      /* media */
}

/* DEC-137: la minimappa e' passata da pannello laterale a overlay in un angolo
   (DrawHudRunStatus). Disegna solo la griglia GRID_SIZE x GRID_SIZE all'angolo
   (baseX, baseY), con cella/gap scelti dal chiamante: piu' piccola di prima
   (era 26px), perche' galleggia sul gioco e non deve rubargli l'angolo. La
   legenda testuale di prima ("T tesoro / $ negozio / B boss") e' caduta: sopra
   il gioco sarebbe una riga di testo larga e invadente, e le lettere sulle
   stanze GIA' visitate (DrawRoomIcon) dicono la stessa cosa dove serve. Larghezza
   e altezza della griglia le ricava il chiamante: GRID_SIZE*cell + (GRID_SIZE-1)*gap. */
static void DrawMinimap(Game *game, int baseX, int baseY, int size, int gap, float uiScale)
{
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            const RoomState *room = &game->rooms[y][x];
            if (!room->exists) continue;   /* niente cornice sulle celle inesistenti: la mappa "respira" */
            Rectangle cell = { (float)(baseX + x*(size + gap)), (float)(baseY + y*(size + gap)), (float)size, (float)size };
            bool current = (x == game->roomX && y == game->roomY);
            /* Visitata: colore pieno del suo tipo. Non visitata ma esistente
               (adiacente a una visitata): smorzata, cosi' si vede DOVE si puo'
               andare senza svelare cosa c'e'. */
            Color base = room->visited ? RoomMapColor(room->kind) : GameColorLerp(RoomMapColor(room->kind), (Color){ 30, 33, 40, 255 }, 0.7f);
            float scale = RoomMapSizeScale(room);
            Rectangle fillCell = cell;
            if (scale < 1.0f)
            {
                float shrink = (float)size*(1.0f - scale)*0.5f;
                fillCell = (Rectangle){ cell.x + shrink, cell.y + shrink, (float)size*scale, (float)size*scale };
            }
            DrawRectangleRec(fillCell, base);
            /* Le icone delle stanze speciali si vedono solo dopo averle visitate:
               un pizzico di scoperta, come in Isaac. Icona ed evidenziazione
               restano ancorate allo SLOT intero (cell), non al riquadro
               rimpicciolito: la griglia deve restare leggibile/cliccabile
               uguale a prima, la taglia e' solo un indizio in piu'. */
            if (room->visited) DrawRoomIcon(room->kind, cell, GameColorWithAlpha(BLACK, 200), uiScale);
            if (current) DrawRectangleLinesEx(cell, UiRound(3.0f*uiScale), RAYWHITE);
            else DrawRectangleLinesEx(cell, 1.0f, GameColorWithAlpha(BLACK, 150));
        }
    }
}

/* Ritorna true se il mouse e' sopra la riga (fase 4: per il tooltip). Il tooltip
   VERO lo disegna il chiamante dopo tutti i pannelli, cosi' non finisce sotto la
   riga successiva. */
static bool DrawItemPreview(Game *game, const Item *item, int x, int y, int width, bool owned, float uiScale)
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
    /* M4: l'altezza scala con uiScale -- il chiamante (DrawOuterUi) usa lo
       STESSO passo (64*uiScale) per la spaziatura fra righe, cosi' il margine
       di 6px fra una riga e la successiva resta proporzionale invece di
       sparire (o esplodere) quando la riga cresce. */
    int fontName = UiRound(14.0f*uiScale);
    int fontSlot = UiRound(12.0f*uiScale);
    Rectangle row = { (float)x, (float)y, (float)width, 58.0f*uiScale };
    bool hover = CheckCollisionPointRec(GetMousePosition(), row);
    DrawRectangleRec(row, hover ? (Color){ 40, 45, 56, 235 } : (owned ? (Color){ 28, 32, 40, 220 } : (Color){ 24, 27, 34, 210 }));
    DrawRectangleLinesEx(row, 2.0f, GameColorWithAlpha(rarityColor, hover ? 255 : 200));
    if (!DrawAtlasCell(game, SPR_ITEM, (Vector2){ x + 28.0f*uiScale, y + 29.0f*uiScale }, 36.0f*uiScale, WHITE))
    {
        DrawItemShape((Vector2){ x + 28.0f*uiScale, y + 29.0f*uiScale }, *item, 12.0f*uiScale);
    }
    /* Se l'oggetto cambia il modo di sparare (step C), un puntino del colore del
       colpo in coda al nome: si vede a colpo d'occhio quali oggetti danno un tipo
       di colpo. */
    if (item->shotType.active) DrawCircleV((Vector2){ x + 47.0f*uiScale, y + 15.0f*uiScale }, 4.0f*uiScale, item->color);
    DrawText(item->name, x + UiRound(55.0f*uiScale), y + UiRound(9.0f*uiScale), fontName, RAYWHITE);
    /* Nome della rarita' in coda alla riga slot/traits, nel suo colore (design
       doc, sezione 6: "...e col nome nel pannello"): due DrawText invece di
       uno solo cosi' SOLO il nome della rarita' prende il suo colore, il
       resto della riga resta nel grigio neutro gia' in uso. */
    char slotTraits[200];
    snprintf(slotTraits, sizeof(slotTraits), "%s  |  %s  |  ", SlotName(item->slot), traits);
    int textX = x + UiRound(55.0f*uiScale);
    int textY = y + UiRound(31.0f*uiScale);
    DrawText(slotTraits, textX, textY, fontSlot, (Color){ 190, 198, 211, 255 });
    int rarityTextX = textX + MeasureText(slotTraits, fontSlot);
    DrawText(RarityName(item->rarity), rarityTextX, textY, fontSlot, rarityColor);
    return hover;
}

/* Il tooltip di un oggetto (fase 4, richiesta dell'utente): passando il mouse su un
   oggetto, una scheda con COSA FA -- slot, trait, rarita', e il tipo di colpo che
   conferisce. Disegnato per ULTIMO (sopra tutto), ancorato vicino al mouse ma
   tenuto dentro lo schermo. */
static void DrawItemTooltip(const Item *item, float uiScale)
{
    char traits[128];
    TraitsToText(item->traits, traits, sizeof(traits));

    char lines[4][160];
    int n = 0;
    snprintf(lines[n++], sizeof(lines[0]), "%s  -  %s", SlotName(item->slot), RarityName(item->rarity));
    snprintf(lines[n++], sizeof(lines[0]), "Effetti: %s", traits);
    if (item->shotType.active)
        snprintf(lines[n++], sizeof(lines[0]), "Spari: %s (%s)", item->shotType.name, ShotFormName(item->shotType.form));
    if (item->kind == ITEM_STATUP)
        snprintf(lines[n++], sizeof(lines[0]), "Ricompensa del boss: potenzia una statistica");

    int titleFont = UiRound(16.0f*uiScale);
    int lineFont = UiRound(13.0f*uiScale);
    int w = MeasureText(item->name, titleFont);
    for (int i = 0; i < n; i++) { int lw = MeasureText(lines[i], lineFont); if (lw > w) w = lw; }
    w += UiRound(24.0f*uiScale);
    int lineStep = UiRound(18.0f*uiScale);
    int h = UiRound(30.0f*uiScale) + n*lineStep + UiRound(8.0f*uiScale);

    Vector2 m = GetMousePosition();
    float bx = m.x + 18.0f*uiScale;
    float by = m.y + 8.0f*uiScale;
    if (bx + w > (float)GetScreenWidth() - 6.0f) bx = m.x - (float)w - 8.0f*uiScale;
    if (by + h > (float)GetScreenHeight() - 6.0f) by = (float)GetScreenHeight() - (float)h - 6.0f;

    DrawRectangleRec((Rectangle){ bx, by, (float)w, (float)h }, (Color){ 12, 14, 19, 245 });
    DrawRectangleLinesEx((Rectangle){ bx, by, (float)w, (float)h }, 2.0f, GameColorWithAlpha(RarityColor(item->rarity), 230));
    DrawText(item->name, (int)bx + UiRound(12.0f*uiScale), (int)by + UiRound(8.0f*uiScale), titleFont, RAYWHITE);
    for (int i = 0; i < n; i++)
        DrawText(lines[i], (int)bx + UiRound(12.0f*uiScale), (int)by + UiRound(30.0f*uiScale) + i*lineStep, lineFont, (Color){ 198, 205, 217, 255 });
}

/* Il blocco BUILD (fase 4, richiesta dell'utente: "sinergie/colpo piu' in vista").
   Il tipo di colpo attivo in una barra tinta del suo colore, e le sinergie attive
   come pillole dorate. E' lo stato piu' importante di una build, e prima era
   confinato in righe piccole (il colpo nel pannello, le sinergie nel LOG). Ritorna
   l'altezza occupata, per impaginare cio' che segue.
   DEC-137: 'measureOnly' restituisce quell'altezza SENZA disegnare niente -- serve
   all'HUD in overlay (DrawHudBuild), che deve dimensionare il riquadro scuro DIETRO
   il blocco prima di disegnarlo, e l'altezza dipende da quante righe di pillole
   servono (numero di sinergie x larghezza disponibile). Unica sorgente
   dell'impaginazione per le due cose: nessun conteggio di righe duplicato altrove. */
static int DrawBuildBlock(Game *game, int x, int y, int width, float uiScale, bool measureOnly)
{
    const Player *p = &game->player;
    int cy = y;
    int fontShot = UiRound(15.0f*uiScale);
    int fontPill = UiRound(13.0f*uiScale);

    /* Barra del tipo di colpo. */
    Rectangle bar = { (float)x, (float)cy, (float)width, 30.0f*uiScale };
    Color shotTint = p->shotType.active ? p->shotColor : (Color){ 60, 66, 78, 255 };
    if (!measureOnly)
    {
        DrawRectangleRec(bar, GameColorWithAlpha(shotTint, 60));
        DrawRectangleLinesEx(bar, 2.0f, GameColorWithAlpha(shotTint, 200));
        DrawCircleV((Vector2){ (float)x + 16.0f*uiScale, (float)cy + 15.0f*uiScale }, 6.0f*uiScale, shotTint);
        if (p->shotType.active)
            DrawText(TextFormat("%s  (%s)", p->shotType.name, ShotFormName(p->shotType.form)), x + UiRound(30.0f*uiScale), cy + UiRound(7.0f*uiScale), fontShot, RAYWHITE);
        else
            DrawText("Colpo base", x + UiRound(30.0f*uiScale), cy + UiRound(7.0f*uiScale), fontShot, (Color){ 170, 178, 190, 255 });
    }
    cy += UiRound(38.0f*uiScale);

    /* Pillole delle sinergie attive: vanno a capo da sole se non ci stanno in
       larghezza. */
    int chipH = UiRound(22.0f*uiScale);
    int chipRowStep = UiRound(26.0f*uiScale);
    int chipX = x, chipY = cy, anyPill = 0;
    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        if (!(p->synergies & (1u << i))) continue;
        const char *name = SynergyName(i);
        int w = MeasureText(name, fontPill) + UiRound(20.0f*uiScale);
        if (chipX + w > x + width) { chipX = x; chipY += chipRowStep; }
        if (!measureOnly)
        {
            DrawRectangleRounded((Rectangle){ (float)chipX, (float)chipY, (float)w, (float)chipH }, 0.5f, 6, GameColorWithAlpha(GOLD, 40));
            DrawRectangleRoundedLines((Rectangle){ (float)chipX, (float)chipY, (float)w, (float)chipH }, 0.5f, 6, GOLD);
            DrawText(name, chipX + UiRound(10.0f*uiScale), chipY + UiRound(4.0f*uiScale), fontPill, GOLD);
        }
        chipX += w + UiRound(8.0f*uiScale);
        anyPill++;
    }
    if (!anyPill)
    {
        if (!measureOnly) DrawText("Nessuna sinergia: combina gli oggetti.", x, cy + UiRound(2.0f*uiScale), fontPill, (Color){ 150, 158, 172, 255 });
        chipY = cy;
    }
    return (chipY + chipRowStep) - y;
}

/* ============================================================
   DEC-137: l'HUD in overlay sulla game view a tutto schermo. Non piu' tre
   pannelli-colonna (RUN/GIOCATORE/LOG) che sottraevano spazio al mondo, ma
   quattro cluster ANCORATI AGLI ANGOLI di gameRect, ciascuno un riquadro scuro
   semitrasparente che galleggia sopra il gioco. La disposizione segue la
   priorita' visiva di ui/hud.md: sopravvivenza in alto a sinistra (dove
   l'occhio parte), progressione+mappa in alto a destra, build in basso a
   sinistra, log discreto in basso a destra. Tutto scala con layout.uiScale (M4),
   mai col canvas. Il dettaglio prolisso (lista oggetti, anteprima piano, statistiche
   estese) e' migrato in BuildScreen (l'overlay centrale), cosi' l'HUD sempre
   visibile resta essenziale e non affoga il gioco.
   ============================================================ */

/* Il fondo comune di ogni cluster: riquadro NETTO (pixel-art, DEC-046: niente
   angoli arrotondati per lo sfondo, coerente con le targhette del Piano 0)
   scuro e semitrasparente, col bordo a tema. L'alfa alto garantisce il contrasto
   del testo sopra QUALUNQUE cosa passi sotto nel gioco -- e' la leggibilita'
   richiesta da DEC-137, affidata all'alfa e non alla fortuna dei colori sotto. */
static void DrawHudBox(Rectangle rec, Color border, float uiScale, unsigned char fillAlpha)
{
    DrawRectangleRec(rec, (Color){ 13, 15, 21, fillAlpha });
    DrawRectangleLinesEx(rec, GameMathClampFloat(1.5f*uiScale, 1.5f, 3.0f), GameColorWithAlpha(border, 190));
}

/* Alto-sinistra: sopravvivenza e identita' (priorita' 1 e 3 di ui/hud.md).
   Personaggio, cuori, risorse spendibili. Il riquadro si dimensiona sul
   contenuto (cuori e testo piu' larghi) cosi' resta stretto quando la vita e'
   poca e cresce solo se serve. */
static void DrawHudVitals(Game *game, Rectangle gr, float s)
{
    const Player *p = &game->player;
    const CharacterDef *character = GameResolveCharacterDef(game, game->characterChosenIndex);
    float margin = 12.0f*s, ip = 11.0f*s;
    int nameFont = UiRound(14.0f*s);
    int resFont = UiRound(15.0f*s);

    char nameLine[80];
    if (character) snprintf(nameLine, sizeof(nameLine), "%s -- %s", character->name, character->role);
    else snprintf(nameLine, sizeof(nameLine), "Senza personaggio");
    /* Risorse per funzione (DEC-013): la forma compatta storica del pannello --
       lettere per moneta/bomba/chiave. I nomi ufficiali (Ingots/Blast/Keys,
       DEC-072) restano materia del content designer, questo e' solo un
       trasloco di layout, non una riscrittura di contenuti. */
    char resLine[48];
    snprintf(resLine, sizeof(resLine), "%dc  %db  %dk", p->coins, p->bombs, p->keys);

    int heartSlots = (p->maxHp + 1)/2;
    if (heartSlots < 1) heartSlots = 1;
    float heartS = 15.0f*s;
    float heartsW = (float)heartSlots*heartS*1.15f + heartS;

    float contentW = fmaxf(fmaxf((float)MeasureText(nameLine, nameFont), (float)MeasureText(resLine, resFont)), heartsW);
    float boxW = contentW + ip*2.0f;
    float rowName = 22.0f*s, rowHeart = heartS + 10.0f*s, rowRes = 22.0f*s;
    float boxH = ip*2.0f + rowName + rowHeart + rowRes;

    Rectangle box = { gr.x + margin, gr.y + margin, boxW, boxH };
    DrawHudBox(box, game->theme.accent2, s, 214);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    DrawText(nameLine, cx, cy, nameFont, character ? character->palette : (Color){ 205, 210, 220, 255 });
    cy += UiRound(rowName);
    DrawHearts(p, cx, cy, s);
    cy += UiRound(rowHeart);
    DrawText(resLine, cx, cy, resFont, GOLD);
}

/* Alto-destra: progressione della run e minimappa (priorita' 4 di ui/hud.md).
   Mondo/boss/piano/stanza/fonte in testa, poi la mappa in un angolo
   semitrasparente (DEC-137, "minimappa in un angolo"). L'FPS resta ma discreto,
   in coda alla riga della fonte. */
static void DrawHudRunStatus(Game *game, Rectangle gr, float s)
{
    float margin = 12.0f*s, ip = 11.0f*s;
    int font = UiRound(13.0f*s);
    int fpsFont = UiRound(12.0f*s);

    char worldLine[128], floorLine[96], bossLine[80];
    snprintf(worldLine, sizeof(worldLine), "%s / %s", game->theme.name, game->theme.style);
    snprintf(bossLine, sizeof(bossLine), "Boss: %s", game->theme.bossName);
    snprintf(floorLine, sizeof(floorLine), "Piano %d/%d  -  %s", game->floor, FLOOR_COUNT, GameRoomKindName(GameCurrentRoom(game)->kind));
    const char *sourceLine = game->content.loaded ? "Fonte: LLM cache" : "Fonte: fallback";
    const char *fpsText = TextFormat("%d FPS", GetFPS());

    /* Mappa compatta: celle piu' piccole del vecchio pannello (era 26px), qui
       galleggia sul gioco e non deve invadere l'angolo. */
    int mmCell = UiRound(15.0f*s), mmGap = UiRound(4.0f*s);
    int mmW = GRID_SIZE*mmCell + (GRID_SIZE - 1)*mmGap;
    int mmH = mmW;

    float lineH = 18.0f*s;
    float wText = fmaxf(fmaxf((float)MeasureText(worldLine, font), (float)MeasureText(bossLine, font)),
                        fmaxf((float)MeasureText(floorLine, font),
                              (float)MeasureText(sourceLine, font) + (float)MeasureText(fpsText, fpsFont) + 14.0f*s));
    float contentW = fmaxf(wText, (float)mmW);
    float boxW = contentW + ip*2.0f;
    float gapAfterText = 8.0f*s;
    float boxH = ip*2.0f + lineH*4.0f + gapAfterText + (float)mmH;

    Rectangle box = { gr.x + gr.width - margin - boxW, gr.y + margin, boxW, boxH };
    DrawHudBox(box, game->theme.accent2, s, 214);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    DrawText(worldLine, cx, cy, font, game->theme.accent2); cy += UiRound(lineH);
    DrawText(bossLine, cx, cy, font, (Color){ 214, 218, 226, 255 }); cy += UiRound(lineH);
    DrawText(floorLine, cx, cy, font, RAYWHITE); cy += UiRound(lineH);
    DrawText(sourceLine, cx, cy, font, (Color){ 170, 178, 190, 255 });
    /* FPS in coda alla riga della fonte, allineato al bordo destro del riquadro. */
    DrawText(fpsText, (int)(box.x + boxW - ip) - MeasureText(fpsText, fpsFont), cy + UiRound(1.0f*s), fpsFont, (Color){ 126, 232, 152, 255 });
    cy += UiRound(lineH + gapAfterText);
    DrawMinimap(game, (int)(box.x + (boxW - (float)mmW)*0.5f), cy, mmCell, mmGap, s);
}

/* Basso-sinistra: la build a colpo d'occhio -- tipo di colpo attivo, sinergie
   (DrawBuildBlock, la stessa fonte del pannello di prima) e una riga compatta
   di statistiche chiave. Il riquadro si ancora al fondo di gameRect e la sua
   altezza segue le righe di pillole (DrawBuildBlock in misura). Il dettaglio
   pieno delle statistiche e la lista oggetti vivono in BuildScreen. */
static void DrawHudBuild(Game *game, Rectangle gr, float s)
{
    const Player *p = &game->player;
    float margin = 12.0f*s, ip = 11.0f*s;
    int statFont = UiRound(13.0f*s);

    char statLine[128];
    snprintf(statLine, sizeof(statLine), "DMG %.1f   CAD %.2fs   VEL %.0f   RAG %.1f   FOR %+.1f   Ogg %d",
             p->damage, p->fireDelay, p->shotSpeed, p->shotRadius, p->luck, p->itemCount);

    /* Larghezza interna: abbastanza per la riga statistiche, entro meta' della
       game view (non deve diventare una colonna). */
    float innerW = fmaxf((float)MeasureText(statLine, statFont), 300.0f*s);
    innerW = fminf(innerW, gr.width*0.5f - ip*2.0f);
    int buildH = DrawBuildBlock(game, 0, 0, (int)innerW, s, true);   /* misura, non disegna */

    float statRow = 22.0f*s;
    float boxW = innerW + ip*2.0f;
    float boxH = ip*2.0f + (float)buildH + 4.0f*s + statRow;
    Rectangle box = { gr.x + margin, gr.y + gr.height - margin - boxH, boxW, boxH };
    DrawHudBox(box, game->theme.accent, s, 210);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    DrawBuildBlock(game, cx, cy, (int)innerW, s, false);
    DrawText(statLine, cx, (int)(box.y + box.height - ip) - statFont, statFont, (Color){ 198, 205, 217, 255 });
}

/* Basso-destra: il LOG come area discreta (DEC-137, "log come toast/area
   discreta") -- fonte grafica e nome dell'architettura del piano, in piccolo e
   smorzato, con un riquadro piu' trasparente degli altri: e' informazione di
   contorno, non deve competere con l'HUD di sopravvivenza. */
static void DrawHudLog(Game *game, Rectangle gr, float s)
{
    float margin = 12.0f*s, ip = 9.0f*s;
    int font = UiRound(12.0f*s);
    int floorIndex = GameMathClampInt(game->floor - 1, 0, FLOOR_COUNT - 1);

    const char *atlasMode = strstr(game->content.atlasPath, ".png") ? "Sprite locali (SD)" : "Atlas fallback";
    const RoomLayoutDef *rl = &game->content.floors[floorIndex].roomLayout;
    char archLine[96];
    bool hasArch = rl->active && rl->form != ROOM_LAYOUT_OPEN;
    if (hasArch) snprintf(archLine, sizeof(archLine), "%s (%s)", rl->name, RoomFormName(rl->form));
    else archLine[0] = '\0';

    float lineH = 16.0f*s;
    int nLines = hasArch ? 2 : 1;
    float contentW = (float)MeasureText(atlasMode, font);
    if (hasArch) contentW = fmaxf(contentW, (float)MeasureText(archLine, font));
    float boxW = contentW + ip*2.0f;
    float boxH = ip*2.0f + (float)nLines*lineH;

    Rectangle box = { gr.x + gr.width - margin - boxW, gr.y + gr.height - margin - boxH, boxW, boxH };
    DrawHudBox(box, game->theme.wall, s, 170);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    DrawText(atlasMode, cx, cy, font, GameColorWithAlpha(game->theme.accent2, 210));
    if (hasArch) DrawText(archLine, cx, cy + UiRound(lineH), font, (Color){ 170, 178, 190, 255 });
}

/* Orchestratore dell'HUD in overlay (DEC-137). Chiamato SOLO in APP_GAMEPLAY
   (RendererDrawApp): fuori dal gioco (menu, pausa, risultati) l'HUD e'
   nascosto, come vuole ui/hud.md ("nascosto o attenuato durante PauseMenu e
   BuildScreen"), e il Piano 0 ha i suoi overlay dedicati (riepilogo + carte)
   che occuperebbero lo stesso angolo. Nessun raygui qui: i cluster usano solo
   DrawText/DrawRectangle, quindi non serve UiApplyTheme (gli overlay dei menu
   applicano il proprio tema per conto loro). */
static void DrawOuterUi(Game *game, UiLayout layout)
{
    float s = layout.uiScale;
    Rectangle gr = layout.gameRect;
    DrawHudVitals(game, gr, s);
    DrawHudRunStatus(game, gr, s);
    DrawHudBuild(game, gr, s);
    DrawHudLog(game, gr, s);
}

/* ============================================================
   Overlay dei 9 stati canonici (M1a, ui/navigation-map.md). Ciascuno dei 7
   stati con un vero "menu" (MainMenu, RunSetup, PauseMenu, Options,
   BuildScreen, RunResults, ExitConfirm) disegna il proprio riquadro con
   DrawXOverlay; FloorZero disegna invece l'indicatore di generazione (M1b,
   DrawFloorZeroIndicator: una riga discreta dentro la scena, non piu' un
   overlay bloccante) e Gameplay non disegna nessun overlay sopra la scena.
   MenuBoxForMode/MenuItemCountForMode/MenuItemRect sono la fonte UNICA della
   geometria delle voci: sia per disegnarle (DrawMenuRow) sia per il hit-test
   del mouse (RendererMenuItemAt, chiamata da UpdateApp in src/app/app.c) --
   duplicarla in due posti avrebbe fatto disallineare "cosa si vede" da "cosa
   si clicca" al primo ritocco di uno dei due lati. */
/* M4: nucleo PURO (nessuna chiamata raylib) di MenuBoxForMode -- e' quello che
   --layout-test (src/app/app.c) esercita su risoluzioni sintetiche, PRIMA di
   InitWindow. Scala CENTRATA sullo schermo: la larghezza/altezza cresce con
   uiScale ma il centro (sw*0.5, sh*0.5) resta fermo, cosi' il box rimane
   sempre in mezzo qualunque sia la scala -- esattamente il requisito M4
   "restando centrati". A uiScale==1.0 il centro-larghezza/2 e' aritmeticamente
   identico ai vecchi letterali fissi (sw*0.5-380 == sw*0.5-760*1.0*0.5). */
static Rectangle MenuBoxForModeFor(AppMode mode, float sw, float sh)
{
    float uiScale = UiScaleForHeight(sh);
    /* BuildScreen e' l'unico overlay "grande" (spec M1a: mostra la build
       intera a schermo pieno, non solo poche voci): riusa le stesse fonti
       dati del pannello BUILD/OGGETTI PRESI di DrawOuterUi, che hanno bisogno
       di piu' spazio delle 1-4 voci di un menu qualunque. */
    float w = (mode == APP_BUILD_SCREEN ? 760.0f : 600.0f)*uiScale;
    float h = (mode == APP_BUILD_SCREEN ? 520.0f : 400.0f)*uiScale;
    return (Rectangle){ sw*0.5f - w*0.5f, sh*0.5f - h*0.5f, w, h };
}

static Rectangle MenuBoxForMode(AppMode mode)
{
    return MenuBoxForModeFor(mode, (float)GetScreenWidth(), (float)GetScreenHeight());
}

static int MenuItemCountForMode(AppMode mode)
{
    switch (mode)
    {
        case APP_MAIN_MENU: return 4;    /* Nuova run, Catalogo (M8, DEC-045), Opzioni, Esci */
        case APP_RUN_SETUP: return 3;    /* Seed, Avvia, Indietro ("Modalita'" non e' selezionabile: unica modalita' esistente) */
        case APP_PAUSE_MENU: return 4;   /* Riprendi, Build e sinergie, Opzioni, Abbandona run */
        case APP_OPTIONS: return 1;      /* Indietro */
        case APP_BUILD_SCREEN: return 1; /* Indietro */
        case APP_RUN_RESULTS: return 2;  /* Nuova run subito, Menu principale */
        case APP_EXIT_CONFIRM: return 2; /* Conferma, Annulla */
        default: return 0;               /* FloorZero, Gameplay: nessun menu */
    }
}

/* _BASE: i valori pre-M4, moltiplicati per uiScale in MenuItemRectFor. */
#define MENU_ROW_START_Y_BASE 110.0f
#define MENU_ROW_H_BASE 52.0f

/* M4: nucleo puro gemello di MenuBoxForModeFor -- stessa ragione (--layout-test),
   stessa garanzia (uiScale==1.0 => letterali identici a prima). */
static Rectangle MenuItemRectFor(AppMode mode, int index, float sw, float sh)
{
    Rectangle box = MenuBoxForModeFor(mode, sw, sh);
    float uiScale = UiScaleForHeight(sh);
    return (Rectangle){ box.x + 60.0f*uiScale, box.y + MENU_ROW_START_Y_BASE*uiScale + (float)index*MENU_ROW_H_BASE*uiScale,
                        box.width - 120.0f*uiScale, 40.0f*uiScale };
}

static Rectangle MenuItemRect(AppMode mode, int index)
{
    return MenuItemRectFor(mode, index, (float)GetScreenWidth(), (float)GetScreenHeight());
}

int RendererMenuItemAt(AppMode mode, Vector2 mouse)
{
    int count = MenuItemCountForMode(mode);
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionPointRec(mouse, MenuItemRect(mode, i))) return i;
    }
    return -1;
}

/* Una voce di menu: riquadro pieno + bordo se ha il focus da tastiera
   ('focus'), un riempimento piu' tenue al solo passaggio del mouse (DEC-057:
   il mouse e' ammesso, ma non "ruba" il focus da tastiera solo passandoci
   sopra -- quello lo fa un click vero, gestito in UpdateApp). uiScale si
   ricalcola qui dalla finestra VERA (non e' un parametro): stessa fonte di
   MenuItemRect, che questa funzione chiama per la propria geometria -- cosi'
   il font della riga scala sempre in accordo col riquadro che lo contiene,
   senza dover far transitare uiScale per ogni DrawXOverlay che la chiama. */
static void DrawMenuRow(AppMode mode, int index, const char *label, int focus, Color accent)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle row = MenuItemRect(mode, index);
    bool hasFocus = (index == focus);
    bool hover = CheckCollisionPointRec(GetMousePosition(), row);
    DrawRectangleRec(row, hasFocus ? GameColorWithAlpha(accent, 55) : (hover ? GameColorWithAlpha(accent, 25) : GameColorWithAlpha(BLACK, 90)));
    DrawRectangleLinesEx(row, hasFocus ? 2.0f : 1.0f, hasFocus ? accent : GameColorWithAlpha(accent, 130));
    DrawText(label, (int)row.x + UiRound(16.0f*uiScale), (int)row.y + UiRound(10.0f*uiScale), UiRound(18.0f*uiScale), hasFocus ? RAYWHITE : (Color){ 205, 210, 220, 255 });
}

/* Cornice comune a tutti gli overlay di menu: fondo scurito su tutto lo
   schermo (mette in pausa visiva la scena sotto) + pannello raygui col
   titolo. Estratta da BeginMenuOverlay (M8) perche' il Catalogo (vedi
   BeginCatalogOverlay sotto) ha bisogno della STESSA cornice ma di un box di
   dimensioni proprie -- MenuBoxForMode e' agganciato a un AppMode dei 7
   overlay canonici, e il Catalogo vive dentro APP_MAIN_MENU (nessun nuovo
   AppMode, spec M8), quindi non puo' fornirne uno adatto da solo. */
static void DrawMenuOverlayChrome(Rectangle box, Game *game, const char *title, Color accent)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float uiScale = UiScaleForHeight((float)sh);
    DrawRectangle(0, 0, sw, sh, GameColorWithAlpha(BLACK, 190));
    UiApplyTheme(&game->theme, uiScale);
    GuiPanel(box, title);
    /* Il "24" non scala: stesso motivo di DrawPanel (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT
       e' un #define fisso di raygui). */
    DrawRectangle((int)box.x, (int)box.y + 24, (int)box.width, UiRound(2.0f*uiScale), GameColorWithAlpha(accent, 200));
}

/* Ritorna il box, cosi' il chiamante posiziona il resto del proprio
   contenuto senza ricalcolarlo. */
static Rectangle BeginMenuOverlay(AppMode mode, Game *game, const char *title, Color accent)
{
    Rectangle box = MenuBoxForMode(mode);
    DrawMenuOverlayChrome(box, game, title, accent);
    return box;
}

static void DrawMainMenuOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginMenuOverlay(APP_MAIN_MENU, game, "WORLDSMELT", game->theme.accent2);
    DrawText("Roguelite con contenuti generati in locale.", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(56.0f*uiScale), UiRound(15.0f*uiScale), game->theme.accent2);
    DrawMenuRow(APP_MAIN_MENU, 0, "Nuova run", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_MAIN_MENU, 1, "Catalogo", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_MAIN_MENU, 2, "Opzioni", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_MAIN_MENU, 3, "Esci", ui->focus, game->theme.accent2);
}

static void DrawRunSetupOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginMenuOverlay(APP_RUN_SETUP, game, "NUOVA RUN", game->theme.accent2);
    DrawMenuRow(APP_RUN_SETUP, 0, TextFormat("Seed: %u  (R rigenera)", ui->seed), ui->focus, game->theme.accent2);
    /* "Modalita'" e' un'etichetta fissa (unica modalita' esistente, DEC-038:
       niente selettore di difficolta'), non una voce selezionabile: disegnata
       fra le righe 0 e 1 senza passare da DrawMenuRow/MenuItemRect, cosi' non
       occupa un indice ne' e' cliccabile. */
    DrawText("Modalita': Standard", (int)box.x + UiRound(76.0f*uiScale), (int)(box.y + (MENU_ROW_START_Y_BASE + MENU_ROW_H_BASE*0.62f)*uiScale), UiRound(14.0f*uiScale), (Color){ 176, 184, 198, 255 });
    DrawMenuRow(APP_RUN_SETUP, 1, "Avvia", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_RUN_SETUP, 2, "Indietro", ui->focus, game->theme.accent2);
}

static void DrawPauseMenuOverlay(Game *game, const AppUi *ui)
{
    BeginMenuOverlay(APP_PAUSE_MENU, game, "PAUSA", game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 0, "Riprendi", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 1, "Build e sinergie", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 2, "Opzioni", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 3, "Abbandona run", ui->focus, game->theme.accent2);
}

static void DrawOptionsOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginMenuOverlay(APP_OPTIONS, game, "OPZIONI", game->theme.accent2);
    /* Schermata minima M1a (spec): una sola informazione consultabile, non
       modificabile da qui. Le opzioni vere arrivano con
       ui/options-and-accessibility.md, fuori scope in M1a. */
    DrawText("Schermo intero -- F11", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(56.0f*uiScale), UiRound(16.0f*uiScale), (Color){ 205, 210, 220, 255 });
    DrawMenuRow(APP_OPTIONS, 0, "Indietro", ui->focus, game->theme.accent2);
}

static void DrawBuildScreenOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginMenuOverlay(APP_BUILD_SCREEN, game, "BUILD E SINERGIE", game->theme.accent2);
    const Player *p = &game->player;
    const Item *hoveredItem = NULL;   /* il tooltip va disegnato per ULTIMO, sopra tutto */

    /* DEC-137: la lista oggetti, l'anteprima del piano e le statistiche estese
       non stanno piu' sempre in una colonna laterale sul gioco -- vivono qui, in
       questo overlay centrale (il "pannello build" di DEC-137), che l'HUD di
       gioco rimanda con la sua riga compatta. Due colonne: a sinistra la build
       vera (colpo + sinergie + oggetti presi), a destra le statistiche e cosa
       offre il piano corrente. Stesse fonti dati del vecchio pannello GIOCATORE. */
    int innerX = (int)box.x + UiRound(40.0f*uiScale);
    int innerY = (int)box.y + UiRound(52.0f*uiScale);
    int innerW = (int)box.width - UiRound(80.0f*uiScale);
    int gap = UiRound(28.0f*uiScale);
    int leftW = (int)(innerW*0.56f);
    int rightX = innerX + leftW + gap;
    int rightW = innerW - leftW - gap;
    int rowStep = UiRound(64.0f*uiScale);

    /* Colonna sinistra: colpo + sinergie, poi gli oggetti presi. */
    int ly = innerY;
    int buildH = DrawBuildBlock(game, innerX, ly, leftW, uiScale, false);
    ly += buildH + UiRound(12.0f*uiScale);
    DrawText("OGGETTI PRESI", innerX, ly, UiRound(16.0f*uiScale), game->theme.accent2);
    ly += UiRound(28.0f*uiScale);
    if (p->itemCount == 0) DrawText("Nessun oggetto ancora.", innerX, ly, UiRound(14.0f*uiScale), (Color){ 150, 158, 172, 255 });
    else
    {
        int maxShow = (((int)box.height - (ly - (int)box.y) - UiRound(40.0f*uiScale))/rowStep);
        if (maxShow < 1) maxShow = 1;
        int shown = 0;
        for (int i = 0; i < p->itemCount && shown < maxShow; i++, shown++)
            if (DrawItemPreview(game, &p->items[i], innerX, ly + shown*rowStep, leftW, true, uiScale)) hoveredItem = &p->items[i];
    }

    /* Colonna destra: statistiche estese (le stesse righe del vecchio pannello
       GIOCATORE, con DrawStatLine) e cuori, poi cosa puo' offrire il piano. */
    int ry = innerY;
    DrawText("PERSONAGGIO", rightX, ry, UiRound(16.0f*uiScale), game->theme.accent2);
    ry += UiRound(26.0f*uiScale);
    DrawHearts(p, rightX, ry, uiScale);
    ry += UiRound(30.0f*uiScale);
    DrawStatLine("Danno", TextFormat("%.1f", p->damage), rightX, ry, RAYWHITE, uiScale);
    DrawStatLine("Cadenza", TextFormat("%.2fs", p->fireDelay), rightX, ry + UiRound(22.0f*uiScale), RAYWHITE, uiScale);
    DrawStatLine("Vel. colpo", TextFormat("%.0f", p->shotSpeed), rightX, ry + UiRound(44.0f*uiScale), RAYWHITE, uiScale);
    DrawStatLine("Raggio", TextFormat("%.1f", p->shotRadius), rightX, ry + UiRound(66.0f*uiScale), RAYWHITE, uiScale);
    DrawStatLine("Fortuna", TextFormat("%+.1f", p->luck), rightX, ry + UiRound(88.0f*uiScale), (Color){ 126, 232, 152, 255 }, uiScale);
    DrawStatLine("Risorse", TextFormat("%dc  %db  %dk", p->coins, p->bombs, p->keys), rightX, ry + UiRound(110.0f*uiScale), GOLD, uiScale);
    ry += UiRound(140.0f*uiScale);

    int floorIndex = GameMathClampInt(game->floor - 1, 0, FLOOR_COUNT - 1);
    DrawText("OGGETTI DEL PIANO", rightX, ry, UiRound(16.0f*uiScale), game->theme.accent2);
    ry += UiRound(28.0f*uiScale);
    for (int i = 0; i < 3; i++)
        if (DrawItemPreview(game, &game->content.floors[floorIndex].items[i], rightX, ry + i*rowStep, rightW, false, uiScale))
            hoveredItem = &game->content.floors[floorIndex].items[i];

    DrawMenuRow(APP_BUILD_SCREEN, 0, "Indietro", ui->focus, game->theme.accent2);
    /* Il tooltip per ultimo, sopra tutto (come nel vecchio pannello). */
    if (hoveredItem) DrawItemTooltip(hoveredItem, uiScale);
}

static void DrawRunResultsOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    const char *title = (game->phase == PHASE_WIN) ? "VITTORIA UFFICIALE" : "SCONFITTA";
    Rectangle box = BeginMenuOverlay(APP_RUN_RESULTS, game, title, game->theme.accent2);
    const char *outcome = (game->phase == PHASE_WIN)
        ? "Boss del piano 5 sconfitto."
        : "La run e' finita qui.";
    DrawText(outcome, (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(56.0f*uiScale), UiRound(16.0f*uiScale), game->theme.accent2);
    DrawText(TextFormat("Piano raggiunto: %d / %d", game->floor, FLOOR_COUNT), (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(80.0f*uiScale), UiRound(15.0f*uiScale), (Color){ 205, 210, 220, 255 });
    /* M7 (DEC-015/041/045/069, substrato del catalogo): il feedback canonico
       "se sono stati registrati nuovi contenuti nel catalogo"
       (05-game-states-and-flow.md, righe 83-85). game->catalogRecordsWritten
       e' 0 (riga OMESSA, mai "0" a schermo, spec M7 punto 4) per una run
       fallback, per una run senza nulla di nuovo da registrare, o quando
       AppWriteRunCatalog non e' mai stata chiamata per questa run (il caso
       "0" di GameResetRun, invariato finche' non arriva PHASE_WIN/GAME_OVER). */
    if (game->catalogRecordsWritten > 0)
        DrawText(TextFormat("Creazioni registrate nel catalogo: %d", game->catalogRecordsWritten),
                 (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(102.0f*uiScale), UiRound(14.0f*uiScale), game->theme.accent2);
    DrawMenuRow(APP_RUN_RESULTS, 0, "Nuova run subito", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_RUN_RESULTS, 1, "Menu principale", ui->focus, game->theme.accent2);
}

static void DrawExitConfirmOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginMenuOverlay(APP_EXIT_CONFIRM, game, "CONFERMA", game->theme.accent2);
    /* Tre contesti distinti (DEC-057 + M1b), tutti derivati da 'ui' senza un
       campo dedicato in piu': MainMenu/Esci ha exitAbandonsRun falso, gli
       altri due lo hanno vero e si distinguono da ui->openedFrom (chi ha
       aperto ExitConfirm, gia' scritto da UpdateApp prima del cambio di
       stato). */
    const char *question = !ui->exitAbandonsRun
        ? "Uscire dal gioco?"
        : (ui->openedFrom == APP_FLOOR_ZERO
            ? "Abbandonare la preparazione? La generazione in corso verra' annullata."
            : "Abbandonare la run in corso? Il progresso non salvato si perde.");
    DrawText(question, (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(56.0f*uiScale), UiRound(16.0f*uiScale), (Color){ 205, 210, 220, 255 });
    DrawMenuRow(APP_EXIT_CONFIRM, 0, "Conferma", ui->focus, RED);
    DrawMenuRow(APP_EXIT_CONFIRM, 1, "Annulla", ui->focus, game->theme.accent2);
}

/* Indicatore di generazione DENTRO il Piano 0 (M1b, ui/generation-status.md):
   una riga discreta, MAI un overlay modale -- la generazione ormai non
   blocca piu' nulla (vedi il case APP_FLOOR_ZERO in UpdateApp), quindi non
   deve piu' oscurare la scena ne' rubare l'input. Niente percentuali/barre
   (KB: "preferire messaggi descrittivi stabili a barre di progresso
   ingannevoli") -- 'status->message' arriva gia' composto da
   AppFloorZeroStatusText (src/app/app.c), uno dei tre messaggi canonici
   della KB; qui ci si limita a mostrarlo. 'status' NULL o messaggio vuoto =
   nulla da disegnare (mai un riquadro vuoto). 'uiScale' arriva dal chiamante
   (RendererDrawApp ha gia' il UiLayout del frame): a differenza degli overlay
   di menu sopra, qui non serve ricalcolarla, gameRect e' gia' parte dello
   stesso layout. */
static void DrawFloorZeroIndicator(Rectangle gameRect, float uiScale, const GenProgress *status)
{
    const char *text = (status && status->message[0]) ? status->message : NULL;
    if (!text) return;
    int font = UiRound(16.0f*uiScale);
    int tw = MeasureText(text, font);
    Rectangle box = { gameRect.x + gameRect.width*0.5f - ((float)tw*0.5f + 16.0f*uiScale), gameRect.y + 40.0f*uiScale, (float)tw + 32.0f*uiScale, 30.0f*uiScale };
    DrawRectangleRec(box, (Color){ 16, 18, 24, 205 });
    DrawRectangleLinesEx(box, 1.5f, (Color){ 150, 158, 172, 200 });
    DrawText(text, (int)box.x + UiRound(16.0f*uiScale), (int)box.y + UiRound(7.0f*uiScale), font, RAYWHITE);
}

/* M5 (DEC-005): spezza 'text' in righe che stanno entro 'maxWidth' pixel a
 * 'fontSize' (word-wrap greedy, sugli spazi -- il blurb e' prosa inglese
 * semplice, mai una singola parola piu' larga della carta a queste
 * dimensioni). Scrive fino a 'maxLines' righe in 'out' (ciascuna fino a 159
 * char), ritorna quante ne ha scritte davvero: l'ultima riga che non ci
 * sta piu' viene TRONCATA con "..." invece di sparire silenziosamente (un
 * blurb tagliato a meta' senza segno sarebbe peggio di uno tagliato con
 * segno). */
static int WrapTextLines(const char *text, int fontSize, float maxWidth, char out[][160], int maxLines)
{
    int lineCount = 0;
    char line[160] = { 0 };
    const char *word = text;
    while (*word && lineCount < maxLines)
    {
        const char *spaceAt = strchr(word, ' ');
        size_t wordLen = spaceAt ? (size_t)(spaceAt - word) : strlen(word);
        char candidate[160];
        if (line[0]) snprintf(candidate, sizeof(candidate), "%s %.*s", line, (int)wordLen, word);
        else snprintf(candidate, sizeof(candidate), "%.*s", (int)wordLen, word);

        if (MeasureText(candidate, fontSize) <= (int)maxWidth || !line[0])
        {
            snprintf(line, sizeof(line), "%s", candidate);
        }
        else
        {
            snprintf(out[lineCount++], 160, "%s", line);
            line[0] = '\0';
            continue;   /* riprova la stessa parola sulla riga nuova, senza avanzare 'word' */
        }
        word += wordLen;
        while (*word == ' ') word++;
    }
    if (line[0] && lineCount < maxLines) snprintf(out[lineCount++], 160, "%s", line);
    /* Testo residuo oltre 'maxLines': l'ultima riga scritta guadagna "..." --
       mai un blurb che sparisce a meta' senza indicarlo. */
    if (*word && lineCount == maxLines)
    {
        char *last = out[maxLines - 1];
        char original[160];
        snprintf(original, sizeof(original), "%s", last);
        size_t len = strlen(original);
        while (len > 0 && MeasureText(TextFormat("%.*s...", (int)len, original), fontSize) > (int)maxWidth) len--;
        snprintf(last, 160, "%.*s...", (int)len, original);
    }
    return lineCount;
}

/* ============================================================
   M8 (DEC-045, vista Catalogo v1): enciclopedia consultabile, DENTRO
   APP_MAIN_MENU (nessun nuovo AppMode, nota architetturale della spec).
   Sostituisce DrawMainMenuOverlay quando ui->catalogOpen e' vero (vedi il
   dispatch in RendererDrawApp piu' sotto) -- il menu resta INTATTO sotto
   (ui->focus fermo su 1/"Catalogo"), niente lo ridisegna finche' la vista
   non si richiude. Dopo WrapTextLines (sopra) apposta: DrawCatalogDetail lo
   usa per il testo del dettaglio, e in questo file le funzioni si usano solo
   dopo essere state definite (nessun blocco di forward declaration).
   ============================================================ */

/* Stessa formula "box grande" di BuildScreen (760x520*uiScale, l'unico
   overlay canonico che non e' 600x400): il Catalogo ha bisogno dello stesso
   spazio (tabs di categoria + lista + dettaglio) ma non e' quell'AppMode,
   quindi non puo' passare da MenuBoxForModeFor -- una copia dei due soli
   letterali che servono, non l'intera funzione. */
static Rectangle CatalogBoxFor(float sw, float sh)
{
    float uiScale = UiScaleForHeight(sh);
    float w = 760.0f*uiScale;
    float h = 520.0f*uiScale;
    return (Rectangle){ sw*0.5f - w*0.5f, sh*0.5f - h*0.5f, w, h };
}

static Rectangle BeginCatalogOverlay(Game *game, const char *title, Color accent)
{
    Rectangle box = CatalogBoxFor((float)GetScreenWidth(), (float)GetScreenHeight());
    DrawMenuOverlayChrome(box, game, title, accent);
    return box;
}

static const char *CatalogCategoryLabel(RunCatalogCategory cat)
{
    switch (cat)
    {
        case RUN_CATALOG_CAT_WORLD: return "Mondi";
        case RUN_CATALOG_CAT_LAYOUT: return "Stanze";
        case RUN_CATALOG_CAT_ITEM: return "Oggetti";
        case RUN_CATALOG_CAT_SHOT: return "Colpi";
        case RUN_CATALOG_CAT_ENEMY: return "Nemici";
        case RUN_CATALOG_CAT_BOSS: return "Boss";
        case RUN_CATALOG_CAT_CHARACTER: default: return "Personaggi";
    }
}

/* Le sette schedine di categoria in cima al pannello: sinistra/destra le
   scorre (UpdateApp, src/app/app.c), qui solo disegno. Focus MAI dal solo
   colore (DEC-058): la categoria attiva ha bordo piu' spesso E il conteggio
   fra parentesi (un secondo segnale indipendente, non decorativo). */
static void DrawCatalogTabs(Rectangle box, const RunCatalogSummary *cat, int active, float uiScale, Color accent)
{
    float tabY = box.y + 46.0f*uiScale;
    float tabW = (box.width - 40.0f*uiScale)/(float)RUN_CATALOG_CATEGORY_COUNT;
    for (int c = 0; c < RUN_CATALOG_CATEGORY_COUNT; c++)
    {
        Rectangle tab = { box.x + 20.0f*uiScale + (float)c*tabW, tabY, tabW - 4.0f*uiScale, 26.0f*uiScale };
        bool isActive = (c == active);
        DrawRectangleRec(tab, isActive ? GameColorWithAlpha(accent, 55) : GameColorWithAlpha(BLACK, 90));
        DrawRectangleLinesEx(tab, isActive ? 2.5f : 1.0f, isActive ? accent : GameColorWithAlpha(accent, 130));
        char label[24];
        snprintf(label, sizeof(label), "%s (%d)", CatalogCategoryLabel((RunCatalogCategory)c), cat->entryCount[c]);
        int font = UiRound(11.0f*uiScale);
        DrawText(label, (int)tab.x + UiRound(6.0f*uiScale), (int)tab.y + UiRound(6.0f*uiScale), font, isActive ? RAYWHITE : (Color){ 190, 196, 206, 255 });
    }
}

/* La colonna sinistra: le voci della categoria attiva, in una finestra
   SCORREVOLE larga 'visibleMax' righe (spec M8: fino a 256 voci per
   categoria -- non entrano mai tutte nel box, serve scorrere). 'focus' e'
   gia' clampato dal chiamante (DrawCatalogOverlay). Il rientro (DEC-058:
   focus mai dal solo colore) usa lo stesso schema di DrawMenuRow -- bordo
   piu' spesso sulla voce a fuoco, non solo un colore di sfondo diverso. */
static void DrawCatalogList(Rectangle box, const RunCatalogSummary *cat, RunCatalogCategory active, int focus,
                             float listTop, float rowH, int visibleMax, float uiScale, Color accent)
{
    int count = cat->entryCount[active];
    if (count == 0)
    {
        DrawText("Nessuna voce in questa categoria.", (int)box.x + UiRound(24.0f*uiScale), (int)listTop,
                 UiRound(13.0f*uiScale), (Color){ 150, 158, 172, 255 });
        return;
    }

    int start = focus - visibleMax/2;
    if (start > count - visibleMax) start = count - visibleMax;
    if (start < 0) start = 0;

    float listW = box.width*0.55f - 30.0f*uiScale;
    int shown = 0;
    for (int i = start; i < count && shown < visibleMax; i++, shown++)
    {
        const RunCatalogEntry *e = &cat->entries[active][i];
        Rectangle row = { box.x + 20.0f*uiScale, listTop + (float)shown*rowH, listW, rowH - 4.0f*uiScale };
        bool hasFocus = (i == focus);
        DrawRectangleRec(row, hasFocus ? GameColorWithAlpha(accent, 55) : GameColorWithAlpha(BLACK, 70));
        DrawRectangleLinesEx(row, hasFocus ? 2.0f : 1.0f, hasFocus ? accent : GameColorWithAlpha(accent, 110));
        char label[96];
        if (active == RUN_CATALOG_CAT_BOSS)
            snprintf(label, sizeof(label), "%s -- %s", e->name, e->bossDefeated ? "sconfitto" : "incontrato");
        else
            snprintf(label, sizeof(label), "%s (x%d)", e->name, e->encounterCount);
        DrawText(label, (int)row.x + UiRound(8.0f*uiScale), (int)row.y + UiRound(5.0f*uiScale),
                 UiRound(13.0f*uiScale), hasFocus ? RAYWHITE : (Color){ 200, 206, 216, 255 });
    }

    if (count > visibleMax)
    {
        char pos[24];
        snprintf(pos, sizeof(pos), "%d/%d", focus + 1, count);
        DrawText(pos, (int)(box.x + 20.0f*uiScale + listW - UiRound(40.0f*uiScale)), (int)(listTop - UiRound(16.0f*uiScale)),
                 UiRound(11.0f*uiScale), (Color){ 150, 158, 172, 255 });
    }
    if (cat->overflowCount[active] > 0)
    {
        char more[48];
        snprintf(more, sizeof(more), "-- e altre %d", cat->overflowCount[active]);
        DrawText(more, (int)box.x + UiRound(20.0f*uiScale), (int)(listTop + (float)visibleMax*rowH + 2.0f*uiScale),
                 UiRound(11.0f*uiScale), (Color){ 150, 158, 172, 255 });
    }
}

/* La colonna destra: dettaglio della voce a fuoco (spec M8: "dettaglio breve
   alla voce a fuoco" -- slot/rarita'/tratti per gli oggetti, forma/
   movimento per i nemici, ruolo/trait hook/colpo firmato per i personaggi,
   gia' composto in RunCatalogEntry.detail da RunCatalogAggregate). Word-wrap
   con lo stesso WrapTextLines del blurb dei temi/personaggi (M5/M6a): stesso
   trattamento testuale in tutta la UI, non una regola nuova qui. */
static void DrawCatalogDetail(Rectangle box, const RunCatalogEntry *e, float listTop, float uiScale, Color accent)
{
    float detailX = box.x + box.width*0.58f;
    float detailW = box.width - box.width*0.58f - 20.0f*uiScale;
    int nameFont = UiRound(15.0f*uiScale);
    DrawText(e->name, (int)detailX, (int)listTop, nameFont, accent);

    int lineY = (int)listTop + UiRound(26.0f*uiScale);
    if (e->detail[0])
    {
        int detailFont = UiRound(12.0f*uiScale);
        char lines[6][160];
        int n = WrapTextLines(e->detail, detailFont, detailW, lines, 6);
        for (int l = 0; l < n; l++)
        {
            DrawText(lines[l], (int)detailX, lineY, detailFont, (Color){ 205, 210, 220, 255 });
            lineY += UiRound(16.0f*uiScale);
        }
    }
    lineY += UiRound(6.0f*uiScale);
    DrawText(TextFormat("Incontri: %d  --  Run: %d", e->encounterCount, e->runCount),
             (int)detailX, lineY, UiRound(12.0f*uiScale), (Color){ 176, 184, 198, 255 });
}

static void DrawCatalogOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UiScaleForHeight((float)GetScreenHeight());
    Rectangle box = BeginCatalogOverlay(game, "CATALOGO", game->theme.accent2);
    const RunCatalogSummary *cat = &ui->catalog;

    int totalEntries = 0;
    for (int c = 0; c < RUN_CATALOG_CATEGORY_COUNT; c++) totalEntries += cat->entryCount[c];
    if (totalEntries == 0)
    {
        /* Catalogo vuoto (spec M8): un messaggio sobrio, MAI un errore -- vale
           per l'intera vista (nessuna categoria ha nulla da mostrare, quindi
           niente tabs/lista/dettaglio vuoti a fare da rumore). */
        DrawText("Il crogiolo non ricorda ancora nulla: gioca una run.",
                 (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(70.0f*uiScale),
                 UiRound(16.0f*uiScale), (Color){ 205, 210, 220, 255 });
        DrawText("ESC -- torna al menu.", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(98.0f*uiScale),
                 UiRound(13.0f*uiScale), (Color){ 150, 158, 172, 255 });
        return;
    }

    DrawCatalogTabs(box, cat, ui->catalogCategory, uiScale, game->theme.accent2);

    RunCatalogCategory active = (RunCatalogCategory)ui->catalogCategory;
    int count = cat->entryCount[active];
    int focus = ui->catalogItemFocus;
    if (focus >= count) focus = count > 0 ? count - 1 : 0;
    if (focus < 0) focus = 0;

    float listTop = box.y + 92.0f*uiScale;
    float rowH = 26.0f*uiScale;
    float detailH = 78.0f*uiScale;
    int visibleMax = (int)((box.y + box.height - detailH - listTop)/rowH);
    if (visibleMax < 1) visibleMax = 1;

    DrawCatalogList(box, cat, active, focus, listTop, rowH, visibleMax, uiScale, game->theme.accent2);
    if (count > 0) DrawCatalogDetail(box, &cat->entries[active][focus], listTop, uiScale, game->theme.accent2);

    DrawText("Sinistra/destra: categoria -- Su/giu': voce -- ESC: torna al menu",
             (int)box.x + UiRound(20.0f*uiScale), (int)(box.y + box.height - UiRound(24.0f*uiScale)),
             UiRound(11.0f*uiScale), (Color){ 150, 158, 172, 255 });
}

/* M5 (DEC-005), requisito 9: geometria PROPRIA del pannello di scelta del
 * tema -- non MenuBoxForMode/MenuItemCountForMode (FloorZero non e' uno dei 7
 * stati con overlay di menu canonico, quel conteggio ritorna 0 apposta per
 * lui, vedi il commento sopra). Scala CENTRATA come MenuBoxForModeFor, stessa
 * garanzia M4 (uiScale==1.0 alle risoluzioni di riferimento). M6a: 320 invece
 * dei 300 di M5 -- lo spazio in piu' e' per le due schedine di sezione
 * (DrawFloorZeroSectionTabs) sopra il titolo, che M5 non aveva. */
static Rectangle ThemeCardsPanelBoxFor(float sw, float sh)
{
    float uiScale = UiScaleForHeight(sh);
    float w = 760.0f*uiScale;
    float h = 320.0f*uiScale;
    return (Rectangle){ sw*0.5f - w*0.5f, sh*0.5f - h*0.5f, w, h };
}

/* M6a: 'titleH' e' cresciuta da 40 a 58 (stesso motivo di ThemeCardsPanelBoxFor
 * sopra: le schedine di sezione + il titolo occupano piu' spazio verticale
 * del solo titolo di M5). Generica sul CONTEGGIO -- la usano sia le carte-
 * mondo sia le carte-personaggio, stessa geometria per entrambe le sezioni
 * (mai due layout diversi da coordinare a mano). */
static Rectangle ThemeCardRectFor(Rectangle box, int index, int count, float uiScale)
{
    float pad = 22.0f*uiScale;
    float gap = 16.0f*uiScale;
    float titleH = 58.0f*uiScale;
    float cardW = (box.width - pad*2.0f - gap*(float)(count - 1))/(float)count;
    float cardH = box.height - pad*2.0f - titleH;
    return (Rectangle){ box.x + pad + (float)index*(cardW + gap), box.y + pad + titleH, cardW, cardH };
}

/* M6a, requisito 3: le due schedine "MONDI"/"PERSONAGGI" in cima al pannello
 * combinato -- dicono quale sezione ha il focus da tastiera (su/giu' la
 * cambia). Come il focus di una carta (DEC-058), MAI il solo colore: la
 * sezione attiva ha un bordo piu' spesso, e' leggermente sollevata (stesso
 * trucco di "scala" delle carte, qui verticale) e porta lo stesso piccolo
 * triangolo puntato verso il basso, sopra le carte della sua sezione. */
static void DrawFloorZeroSectionTabs(Rectangle box, int section, float uiScale, Color accent)
{
    float pad = 22.0f*uiScale;
    float tabY = box.y + 8.0f*uiScale;
    float tabH = 22.0f*uiScale;
    Rectangle tabs[2] = {
        { box.x + pad, tabY, 150.0f*uiScale, tabH },
        { box.x + pad + 158.0f*uiScale, tabY, 170.0f*uiScale, tabH },
    };
    const char *labels[2] = { "MONDI", "PERSONAGGI" };
    for (int s = 0; s < 2; s++)
    {
        bool active = (s == section);
        Rectangle tab = tabs[s];
        if (active) { tab.y -= 3.0f*uiScale; tab.height += 3.0f*uiScale; }
        DrawRectangleRec(tab, active ? GameColorWithAlpha(accent, 50) : GameColorWithAlpha(BLACK, 120));
        DrawRectangleLinesEx(tab, active ? 2.5f : 1.0f, active ? accent : GameColorWithAlpha(accent, 130));
        int font = UiRound(13.0f*uiScale);
        DrawText(labels[s], (int)tab.x + UiRound(10.0f*uiScale), (int)tab.y + UiRound(4.0f*uiScale), font,
                 active ? RAYWHITE : (Color){ 190, 196, 206, 255 });
        if (active)
        {
            float cx = tab.x + tab.width*0.5f;
            float ty = tab.y + tab.height + 8.0f*uiScale;
            DrawTriangle((Vector2){ cx - 6.0f*uiScale, ty - 7.0f*uiScale }, (Vector2){ cx + 6.0f*uiScale, ty - 7.0f*uiScale },
                         (Vector2){ cx, ty }, accent);
        }
    }
}

/* Una carta (mondo o personaggio, stessa geometria via ThemeCardRectFor) col
 * bordo/scala/triangolo del focus (DEC-058, mai il solo colore) -- fattorizzata
 * fuori da DrawFloorZeroPanel (M6a) perche' ora la disegnano DUE sezioni
 * diverse con lo stesso identico linguaggio visivo. 'selected' e' un segnale
 * DISTINTO dal focus (requisito 3: "la scheda del personaggio selezionato
 * resta evidenziata, segnale distinto dal focus"): un bordo pieno dell'accento
 * anche senza focus, cosi' si vede quale scelta e' GIA' attiva mentre si
 * naviga altrove con le frecce. */
static Rectangle DrawFloorZeroCardFrame(Rectangle box, int index, int count, float uiScale, bool focused, bool selected, Color accent)
{
    Rectangle card = ThemeCardRectFor(box, index, count, uiScale);
    if (focused)
    {
        float grow = 6.0f*uiScale;
        card = (Rectangle){ card.x - grow*0.5f, card.y - grow*0.5f, card.width + grow, card.height + grow };
    }
    Color fill = focused ? GameColorWithAlpha(accent, 40) : (selected ? GameColorWithAlpha(accent, 26) : GameColorWithAlpha(BLACK, 130));
    DrawRectangleRec(card, fill);
    float thick = focused ? 3.0f : (selected ? 2.0f : 1.5f);
    Color line = focused ? accent : (selected ? accent : GameColorWithAlpha(accent, 150));
    DrawRectangleLinesEx(card, thick, line);
    if (focused)
    {
        float cx = card.x + card.width*0.5f;
        float ty = card.y - 10.0f*uiScale;
        DrawTriangle((Vector2){ cx - 7.0f*uiScale, ty - 8.0f*uiScale }, (Vector2){ cx + 7.0f*uiScale, ty - 8.0f*uiScale },
                     (Vector2){ cx, ty }, accent);
    }
    return card;
}

static void DrawWorldCards(const Game *game, Rectangle box, float uiScale)
{
    int titleFont = UiRound(15.0f*uiScale);
    int blurbFont = UiRound(12.0f*uiScale);
    for (int i = 0; i < game->themeCardCount; i++)
    {
        bool focused = (i == game->themeCardFocus) && (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS);
        bool selected = (i == game->themeChosenIndex);
        Rectangle card = DrawFloorZeroCardFrame(box, i, game->themeCardCount, uiScale, focused, selected, game->theme.accent2);

        const ThemeCard *proposal = &game->themeCards[i];
        DrawText(proposal->name, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(10.0f*uiScale),
                 titleFont, focused ? RAYWHITE : (Color){ 205, 210, 220, 255 });
        if (selected)
            DrawText("SCELTO", (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(30.0f*uiScale),
                     UiRound(11.0f*uiScale), GOLD);

        char lines[4][160];
        int wrapY = selected ? 48 : 34;
        int n = WrapTextLines(proposal->blurb, blurbFont, card.width - 20.0f*uiScale, lines, 4);
        int ly = (int)card.y + UiRound((float)wrapY*uiScale);
        int lineStep = UiRound(16.0f*uiScale);
        for (int l = 0; l < n; l++)
            DrawText(lines[l], (int)card.x + UiRound(10.0f*uiScale), ly + l*lineStep, blurbFont, (Color){ 190, 196, 206, 255 });
    }
}

/* M6a, requisito 3: le schede della rosa base -- nome, ruolo, blurb, una
 * piccola tabella di statistiche chiave e un pallino della palette (mai il
 * SOLO colore per identificare il personaggio: nome/ruolo restano il
 * segnale primario, il pallino e' un tocco in piu', coerente col resto
 * della UI che non affida MAI un significato al solo colore, DEC-058).
 * M6b-1 (DEC-014): da CHARACTER_COUNT fisso a GameCharacterCardCount(game)
 * -- un quarto slot DINAMICO compare quando il personaggio generato per
 * questa run e' valido, disegnato con la stessa identica geometria/gli
 * stessi campi delle carte curate (GameResolveCharacterDef nasconde la
 * differenza: rosa o generato, qui e' solo "la CharacterDef all'indice i").
 * La sua etichetta di origine e' il campo 'role' stesso ("FORGED THIS RUN",
 * scritto da RunContentLoadCharacterProposal): nessun ramo di disegno in
 * piu' da mantenere. */
static void DrawCharacterCards(const Game *game, Rectangle box, float uiScale)
{
    int nameFont = UiRound(15.0f*uiScale);
    int roleFont = UiRound(12.0f*uiScale);
    int blurbFont = UiRound(11.0f*uiScale);
    int statFont = UiRound(12.0f*uiScale);
    int cardCount = GameCharacterCardCount(game);
    for (int i = 0; i < cardCount; i++)
    {
        const CharacterDef *c = GameResolveCharacterDef(game, i);
        if (!c) continue;   /* difesa a buon mercato: non dovrebbe mai capitare per i < cardCount */
        bool focused = (i == game->characterCardFocus) && (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_CHARACTERS);
        bool selected = (i == game->characterChosenIndex);
        Rectangle card = DrawFloorZeroCardFrame(box, i, cardCount, uiScale, focused, selected, game->theme.accent2);

        float dotR = 6.0f*uiScale;
        DrawCircleV((Vector2){ card.x + card.width - 16.0f*uiScale, card.y + 16.0f*uiScale }, dotR, c->palette);
        DrawText(c->name, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(10.0f*uiScale),
                 nameFont, focused ? RAYWHITE : (Color){ 205, 210, 220, 255 });
        DrawText(c->role, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(30.0f*uiScale),
                 roleFont, game->theme.accent2);
        if (selected)
            DrawText("SCELTO", (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(48.0f*uiScale),
                     UiRound(11.0f*uiScale), GOLD);

        /* M6b-2 (DEC-037): riga onesta e sobria sul trait Lua del personaggio
           GENERATO -- SOLO se c->traitHook non e' vuoto (nessun testo
           inventato, vedi il commento sul campo in core/game_types.h): il
           nome LETTERALE della callback che lo script definisce
           ("on_fire"/"on_hit"/"on_tick"/"on_evaluate"), mai una descrizione
           immaginata del suo effetto. La rosa curata non imposta mai questo
           campo, quindi questa riga compare solo sul quarto slot dinamico. */
        int blurbOffset = UiRound((selected ? 66.0f : 50.0f)*uiScale);
        if (c->traitHook[0])
        {
            char traitText[32];
            snprintf(traitText, sizeof(traitText), "Trait: %s", c->traitHook);
            DrawText(traitText, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + blurbOffset,
                     UiRound(11.0f*uiScale), game->theme.accent2);
            blurbOffset += UiRound(15.0f*uiScale);
        }
        /* M6b-3 (DEC-068): riga onesta sul colpo firmato -- SOLO se
           c->signatureShot.active (mai dal solo colore, DEC-058, e mai
           quando assente: characters.md, "senza colpo firmato non c'e' una
           riga apposita"). Il NOME leggibile, mai una descrizione delle
           sue manopole (le manopole si vedono gia' giocando, come per ogni
           altro tipo di colpo). La rosa curata non imposta mai questo
           campo (character_roster.c), quindi questa riga compare solo sul
           quarto slot dinamico, esattamente come "Trait:" sopra. */
        if (c->signatureShot.active)
        {
            char shotText[48];
            snprintf(shotText, sizeof(shotText), "Signature: %s", c->signatureShot.name);
            DrawText(shotText, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + blurbOffset,
                     UiRound(11.0f*uiScale), game->theme.accent2);
            blurbOffset += UiRound(15.0f*uiScale);
        }

        char lines[3][160];
        int n = WrapTextLines(c->blurb, blurbFont, card.width - 20.0f*uiScale, lines, 3);
        int ly = (int)card.y + blurbOffset;
        int lineStep = UiRound(15.0f*uiScale);
        for (int l = 0; l < n; l++)
            DrawText(lines[l], (int)card.x + UiRound(10.0f*uiScale), ly + l*lineStep, blurbFont, (Color){ 190, 196, 206, 255 });

        /* Statistiche chiave in piccola tabella (requisito 3), ancorate al
           fondo della carta cosi' restano leggibili a qualunque numero di
           righe di blurb sopra. */
        int statsY = (int)(card.y + card.height - 40.0f*uiScale);
        DrawText(TextFormat("DMG %.0f", c->baseDamage), (int)card.x + UiRound(10.0f*uiScale), statsY, statFont, RAYWHITE);
        DrawText(TextFormat("SPD %.0f", c->baseSpeed), (int)card.x + UiRound(10.0f*uiScale), statsY + UiRound(16.0f*uiScale), statFont, RAYWHITE);
        DrawText(TextFormat("HP %d/%d", c->baseMaxHp, c->hpCap), (int)card.x + UiRound(10.0f*uiScale), statsY + UiRound(32.0f*uiScale), statFont, RAYWHITE);
    }
}

/* Il pannello COMBINATO MONDI/PERSONAGGI (M5 requisito 9 + M6a requisito 3):
 * TAB lo apre/chiude (src/app/app.c), su/giu' cambia sezione, sinistra/
 * destra sposta il focus, conferma sceglie -- MAI il click (l'ambiguita'
 * DEC-057 sul mouse in FloorZero resta una domanda aperta della KB). A
 * differenza di M5, questo pannello NON smette di disegnare nulla dopo la
 * scelta del mondo: la sezione PERSONAGGI resta viva per tutta la
 * permanenza nel Piano 0 (requisito 1). Il riepilogo persistente lo fa
 * comunque DrawFloorZeroSummary sotto, per chi vuole lo stato SENZA aprire
 * il pannello. */
static void DrawFloorZeroPanel(const Game *game, float sw, float sh)
{
    if (game->themeCardCount <= 0) return;
    float uiScale = UiScaleForHeight(sh);

    if (!game->themeCardsPanelOpen)
    {
        /* Pannello chiuso: solo un invito discreto, mai un riquadro vuoto --
           il messaggio stabile "in attesa della scelta del mondo" (vedi
           AppFloorZeroStatusText) gia' dice CHE COSA manca prima della
           scelta; dopo, questo resta comunque l'invito a riaprire per
           cambiare personaggio (requisito 1: sempre modificabile). */
        const char *hint = "TAB -- mondo e personaggio";
        int font = UiRound(14.0f*uiScale);
        int tw = MeasureText(hint, font);
        Rectangle box = { sw*0.5f - ((float)tw*0.5f + 14.0f*uiScale), 80.0f*uiScale, (float)tw + 28.0f*uiScale, 26.0f*uiScale };
        DrawRectangleRec(box, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(box, 1.5f, (Color){ 150, 158, 172, 180 });
        DrawText(hint, (int)box.x + UiRound(14.0f*uiScale), (int)box.y + UiRound(6.0f*uiScale), font, (Color){ 205, 210, 220, 255 });
        return;
    }

    DrawRectangle(0, 0, (int)sw, (int)sh, GameColorWithAlpha(BLACK, 170));
    Rectangle box = ThemeCardsPanelBoxFor(sw, sh);
    DrawRectangleRec(box, (Color){ 18, 20, 27, 235 });
    DrawRectangleLinesEx(box, 2.0f, game->theme.accent2);
    DrawFloorZeroSectionTabs(box, game->floorZeroPanelSection, uiScale, game->theme.accent2);

    const char *title = (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS)
                         ? "Scegli il mondo -- sinistra/destra, conferma (su/giu': personaggio)"
                         : "Scegli il personaggio -- sinistra/destra, conferma (su/giu': mondo)";
    DrawText(title, (int)box.x + UiRound(20.0f*uiScale), (int)box.y + UiRound(42.0f*uiScale),
             UiRound(13.0f*uiScale), (Color){ 205, 210, 220, 255 });

    /* La geometria delle carte usa la stessa 'box' del titolo (ThemeCardRectFor
       misura dal bordo del pannello, non dalla riga del titolo): entrambe le
       sezioni condividono lo stesso riquadro, se ne disegna una sola alla
       volta -- e' quella col focus (game->floorZeroPanelSection) a decidere
       quale. */
    if (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS) DrawWorldCards(game, box, uiScale);
    else DrawCharacterCards(game, box, uiScale);
}

/* Riepilogo persistente di mondo + personaggio (M5 requisito 9 + M6a
 * requisito 3, "Feedback": "Il tema scelto e il personaggio scelto restano
 * visibili in un riepilogo"): visibile per TUTTA la permanenza nel Piano 0,
 * non solo nell'istante della conferma. Il personaggio e' SEMPRE definito
 * (preselezione di default, vedi FloorZeroEnter) quindi la sua targhetta
 * compare da subito; quella del mondo resta gating su themeChosenIndex>=0
 * come in M5 -- il mondo puo' davvero essere ancora indefinito. */
static void DrawFloorZeroSummary(const Game *game, Rectangle gameRect, float uiScale)
{
    float y = gameRect.y + 12.0f*uiScale;
    int font = UiRound(14.0f*uiScale);

    if (game->themeChosenIndex >= 0)
    {
        const ThemeCard *chosen = &game->themeCards[game->themeChosenIndex];
        char text[64];
        snprintf(text, sizeof(text), "Mondo: %s", chosen->name);
        int tw = MeasureText(text, font);
        Rectangle box = { gameRect.x + 12.0f*uiScale, y, (float)tw + 24.0f*uiScale, 26.0f*uiScale };
        DrawRectangleRec(box, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(box, 1.5f, game->theme.accent2);
        DrawText(text, (int)box.x + UiRound(12.0f*uiScale), (int)box.y + UiRound(6.0f*uiScale), font, RAYWHITE);
        y += 30.0f*uiScale;
    }

    /* M6b-1: GameResolveCharacterDef risolve sia la rosa sia il quarto slot
       generato -- il commento sopra ("il personaggio e' SEMPRE definito")
       resta vero nel cammino normale (FloorZeroEnter preseleziona subito),
       ma un NULL qui (es. un test che chiama questa funzione fuori dal
       Piano 0) non deve leggere un puntatore morto. */
    const CharacterDef *character = GameResolveCharacterDef(game, game->characterChosenIndex);
    if (character)
    {
        char ctext[64];
        snprintf(ctext, sizeof(ctext), "Personaggio: %s", character->name);
        int ctw = MeasureText(ctext, font);
        Rectangle cbox = { gameRect.x + 12.0f*uiScale, y, (float)ctw + 24.0f*uiScale, 26.0f*uiScale };
        DrawRectangleRec(cbox, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(cbox, 1.5f, character->palette);
        DrawText(ctext, (int)cbox.x + UiRound(12.0f*uiScale), (int)cbox.y + UiRound(6.0f*uiScale), font, RAYWHITE);
    }
}

void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, const AppUi *ui,
                     bool takeScreenshot, const GenProgress *genProgress, const char *screenshotPath)
{
    BeginTextureMode(canvas);
    DrawGameplayCanvas(game);
    EndTextureMode();

    BeginDrawing();
    /* Lo sfondo scuro riempie le bande che il rapporto 3:2 del canvas lascia sui
       lati di uno schermo 16:9 (DEC-137): la game view e' centrata e massimale,
       le bande sono una cornice scura, non spazio riservato alla UI. */
    ClearBackground((Color){ 9, 11, 16, 255 });
    UiLayout layout = UiComputeLayout();
    /* DEC-137: la game view riempie lo schermo e la GUI vive SOPRA di essa. Il
       canvas scalato PRIMA della GUI: l'HUD in overlay (DrawOuterUi) e gli overlay
       di stato ci si appoggiano sopra. Niente piu' cornice/etichetta "GAME VIEW":
       era chrome da colonne separate, ora la vista di gioco E' lo schermo. */
    Rectangle src = { 0.0f, 0.0f, (float)canvas.texture.width, -(float)canvas.texture.height };
    DrawTexturePro(canvas.texture, src, layout.gameRect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    /* HUD di gioco SOLO in Gameplay: fuori dal gioco (menu/pausa/risultati) e' nascosto
       (ui/hud.md), e il Piano 0 ha i suoi overlay dedicati (riepilogo + carte) sullo
       stesso angolo -- vedi il case APP_FLOOR_ZERO sotto. */
    if (mode == APP_GAMEPLAY) DrawOuterUi(game, layout);

    /* UN overlay per stato (switch esplicito, M1a): 'ui' e' NULL solo per
       Gameplay (che non ne ha bisogno) e per FloorZero (che legge
       genProgress, non ui). */
    switch (mode)
    {
        /* M8 (DEC-045): la vista Catalogo sostituisce il disegno del menu
           quando aperta -- nessun nuovo AppMode, il case resta uno solo. */
        case APP_MAIN_MENU: if (ui->catalogOpen) DrawCatalogOverlay(game, ui); else DrawMainMenuOverlay(game, ui); break;
        case APP_RUN_SETUP: DrawRunSetupOverlay(game, ui); break;
        case APP_FLOOR_ZERO:
            DrawFloorZeroIndicator(layout.gameRect, layout.uiScale, genProgress);
            /* M6a: il pannello combinato (aperto o solo l'invito, a seconda
               di game->themeCardsPanelOpen) e il riepilogo persistente
               convivono SEMPRE, a differenza di M5 -- il riepilogo mostra
               anche il personaggio (sempre definito) anche col pannello
               chiuso, il pannello resta apribile anche dopo la scelta del
               mondo (requisito 1: il personaggio resta modificabile). */
            DrawFloorZeroPanel(game, (float)GetScreenWidth(), (float)GetScreenHeight());
            DrawFloorZeroSummary(game, layout.gameRect, layout.uiScale);
            break;
        case APP_GAMEPLAY: break;
        case APP_PAUSE_MENU: DrawPauseMenuOverlay(game, ui); break;
        case APP_OPTIONS: DrawOptionsOverlay(game, ui); break;
        case APP_BUILD_SCREEN: DrawBuildScreenOverlay(game, ui); break;
        case APP_RUN_RESULTS: DrawRunResultsOverlay(game, ui); break;
        case APP_EXIT_CONFIRM: DrawExitConfirmOverlay(game, ui); break;
    }

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

/* ============================================================
   --layout-test (src/app/app.c), matematica pura -- nessuna InitWindow,
   nessun GetScreenWidth/Height: solo UiComputeLayoutFor/MenuBoxForModeFor/
   MenuItemRectFor su risoluzioni sintetiche. Aggiornato a DEC-137: non ci
   sono piu' tre pannelli-colonna da tenere separati dal canvas, c'e' UNA sola
   superficie (la game view a tutto schermo) e la geometria dei menu-overlay.
   Il piccolo margine di tolleranza (EPS) assorbe il rumore in virgola mobile
   delle divisioni/moltiplicazioni a catena: un pixel di errore non e' un buco.
   ============================================================ */
#define UI_LAYOUT_TEST_EPS 0.05f

static bool UiRectInside(Rectangle outer, Rectangle inner)
{
    return inner.x >= outer.x - UI_LAYOUT_TEST_EPS && inner.y >= outer.y - UI_LAYOUT_TEST_EPS &&
           inner.x + inner.width <= outer.x + outer.width + UI_LAYOUT_TEST_EPS &&
           inner.y + inner.height <= outer.y + outer.height + UI_LAYOUT_TEST_EPS;
}

static bool UiRectOverlap(Rectangle a, Rectangle b)
{
    return a.x < b.x + b.width - UI_LAYOUT_TEST_EPS && a.x + a.width > b.x + UI_LAYOUT_TEST_EPS &&
           a.y < b.y + b.height - UI_LAYOUT_TEST_EPS && a.y + a.height > b.y + UI_LAYOUT_TEST_EPS;
}

bool UiLayoutSelfTest(void)
{
    static const float kW[] = { 1280.0f, 1366.0f, 1600.0f, 1920.0f, 2560.0f, 3440.0f, 3840.0f };
    static const float kH[] = {  720.0f,  768.0f,  900.0f, 1080.0f, 1440.0f, 1440.0f, 2160.0f };
    static const AppMode kMenuModes[] = {
        APP_MAIN_MENU, APP_RUN_SETUP, APP_PAUSE_MENU, APP_OPTIONS,
        APP_BUILD_SCREEN, APP_RUN_RESULTS, APP_EXIT_CONFIRM
    };
    const int n = (int)(sizeof(kW)/sizeof(kW[0]));

    float prevArea = -1.0f, prevUiScale = -1.0f;
    for (int i = 0; i < n; i++)
    {
        float sw = kW[i], sh = kH[i];
        UiLayout L = UiComputeLayoutFor(sw, sh);
        Rectangle screen = { 0.0f, 0.0f, sw, sh };

        /* (a) DEC-137: la game view sta dentro lo schermo ed e' CENTRATA -- una
           sola superficie, niente piu' pannelli-colonna da separare. Il canvas
           3:2 su uno schermo di rapporto diverso lascia bande simmetriche, ed e'
           proprio la centratura a garantirle uguali sui due lati. */
        if (!UiRectInside(screen, L.gameRect))
        {
            fprintf(stderr, "UiLayoutSelfTest: (a) la game view esce dallo schermo a %.0fx%.0f\n", sw, sh);
            return false;
        }
        if (fabsf(L.gameRect.x - (sw - L.gameRect.width)*0.5f) > UI_LAYOUT_TEST_EPS ||
            fabsf(L.gameRect.y - (sh - L.gameRect.height)*0.5f) > UI_LAYOUT_TEST_EPS)
        {
            fprintf(stderr, "UiLayoutSelfTest: (a) game view non centrata a %.0fx%.0f\n", sw, sh);
            return false;
        }

        /* (b) la scala e' il MASSIMO passo di 1/8 che riempie lo schermo INTERO
           (niente piu' spazio riservato ai pannelli: maxW/maxH sono sw/sh): il
           passo SUCCESSIVO (+1/8) non deve piu' starci, altrimenti la game view
           avrebbe lasciato bande evitabili. Sotto il minimo 0.75 il vincolo non
           si applica: quello e' un pavimento imposto, non "il massimo che ci sta". */
        float nextScale = L.gameScale + 0.125f;
        bool nextFits = (nextScale*(float)SCREEN_WIDTH <= sw + UI_LAYOUT_TEST_EPS) &&
                        (nextScale*(float)SCREEN_HEIGHT <= sh + UI_LAYOUT_TEST_EPS);
        if (nextFits && L.gameScale > 0.75f + 0.001f)
        {
            fprintf(stderr, "UiLayoutSelfTest: (b) bande evitabili a %.0fx%.0f (scale %.3f, il passo successivo ci stava)\n", sw, sh, L.gameScale);
            return false;
        }

        /* (d) monotonia: la lista e' gia' ordinata per risoluzione crescente
           (larghezza E altezza mai decrescenti da una riga alla successiva) --
           gameRect (in area) e uiScale non devono MAI restringersi. */
        float area = L.gameRect.width*L.gameRect.height;
        if (i > 0 && (area < prevArea - UI_LAYOUT_TEST_EPS || L.uiScale < prevUiScale - 0.0001f))
        {
            fprintf(stderr, "UiLayoutSelfTest: (d) canvas/uiScale rimpiccioliti a %.0fx%.0f rispetto alla risoluzione precedente\n", sw, sh);
            return false;
        }
        prevArea = area;
        prevUiScale = L.uiScale;

        /* (e) geometria dei menu: ogni voce dentro il proprio box, nessuna
           sovrapposizione fra voci consecutive dello stesso menu. Invariata da
           DEC-137: gli overlay dei menu erano gia' box centrati, indipendenti
           dai pannelli spariti. */
        for (int m = 0; m < (int)(sizeof(kMenuModes)/sizeof(kMenuModes[0])); m++)
        {
            AppMode mode = kMenuModes[m];
            Rectangle box = MenuBoxForModeFor(mode, sw, sh);
            int count = MenuItemCountForMode(mode);
            Rectangle prevItem = { 0 };
            for (int idx = 0; idx < count; idx++)
            {
                Rectangle item = MenuItemRectFor(mode, idx, sw, sh);
                if (!UiRectInside(box, item))
                {
                    fprintf(stderr, "UiLayoutSelfTest: (e) voce %d del menu %d fuori dal box a %.0fx%.0f\n", idx, (int)mode, sw, sh);
                    return false;
                }
                if (idx > 0 && UiRectOverlap(prevItem, item))
                {
                    fprintf(stderr, "UiLayoutSelfTest: (e) voci %d/%d del menu %d sovrapposte a %.0fx%.0f\n", idx - 1, idx, (int)mode, sw, sh);
                    return false;
                }
                prevItem = item;
            }
        }
    }

    /* (c) riferimento congelato a 1600x900 (la finestra grande del progetto,
       APP_WINDOW_WIDTH/HEIGHT): uiScale ESATTAMENTE 1.0 e gameScale pari alla
       formula DEC-137 a tutto schermo (fit + passo di 1/8, minimo 0.75, nessun
       tetto). A 1600x900 vale floor(min(1600/960, 900/640)*8)/8 = floor(11.25)/8
       = 11/8 = 1.375. Ripetuta qui SOLO come paragone: se UiComputeLayoutFor
       divergesse da questi numeri, la vista di riferimento sarebbe cambiata. */
    {
        float sw = 1600.0f, sh = 900.0f;
        UiLayout L = UiComputeLayoutFor(sw, sh);
        if (fabsf(L.uiScale - 1.0f) > 0.0001f)
        {
            fprintf(stderr, "UiLayoutSelfTest: (c) uiScale a 1600x900 e' %.4f, atteso 1.0\n", L.uiScale);
            return false;
        }
        float expScale = floorf(fminf(sw/(float)SCREEN_WIDTH, sh/(float)SCREEN_HEIGHT)*8.0f)/8.0f;
        if (expScale < 0.75f) expScale = 0.75f;
        if (fabsf(L.gameScale - expScale) > 0.001f ||
            fabsf(L.gameRect.x - (sw - L.gameRect.width)*0.5f) > UI_LAYOUT_TEST_EPS ||
            fabsf(L.gameRect.y - (sh - L.gameRect.height)*0.5f) > UI_LAYOUT_TEST_EPS)
        {
            fprintf(stderr, "UiLayoutSelfTest: (c) layout a 1600x900 diverge dalla formula DEC-137 (scale %.3f, atteso %.3f)\n", L.gameScale, expScale);
            return false;
        }
    }

    return true;
}
