#include "render/game_renderer.h"

#include "assets/game_assets.h"
#include "content/character_roster.h"
#include "core/game_math.h"
#include "game/game.h"
#include "game/game_internal.h"
#include "game/trials.h"
#include "gameplay/fusion.h"
#include "gameplay/item_slots.h"
#include "gameplay/synergies.h"
#include "audio/audio.h"
#include "render/art_draw.h"
#include "render/item_layers.h"
#include "render/rarity_style.h"
#include "render/ui_theme.h"
/* WP7: i due testi della puntata della Pourhouse (offerta e prezzo per esteso,
   DEC-058). Il renderer non li compone: li chiede al modulo che possiede la
   puntata, cosi' non esistono due formulazioni della stessa cosa. */
#include "world/pourhouse.h"
/* WP15a: stessa ragione della riga sopra -- l'etichetta del tema di una
   piazzola d'arena la possiede il modulo delle arene del Piano 0, non il
   renderer (FloorZeroArenaThemeLabel, src/world/floor_zero_arena.h). */
#include "world/floor_zero_arena.h"

#include "raygui.h"   /* solo dichiarazioni: l'implementazione e' in src/render/raygui_impl.c */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

/* ============================================================
   W8: TUTTO il testo del gioco passa da qui.
 *
 * UiText/UiTextW hanno la stessa firma di DrawText/MeasureText di raylib e
 * hanno preso il loro posto in ogni punto di questo file -- HUD, overlay dei
 * menu, Piano 0, catalogo, nome del boss sulla scena. Un solo passaggio
 * obbligato significa che vestire l'interfaccia col font pixel di
 * assets/art/ui/font-5px e' una decisione presa UNA volta, e che il ripiego su
 * DrawText (font vettoriale di raylib) resta disponibile per l'intera
 * interfaccia in blocco, non a chiazze: senza questo, la prima schermata
 * dimenticata avrebbe mostrato due tipografie diverse nello stesso frame.
 *
 * 'size' resta la dimensione in PIXEL che il chiamante chiedeva a DrawText:
 * si traduce nel moltiplicatore intero piu' vicino del font da 5 pixel. Intero
 * per forza (vedi ArtDrawText): un font pixel art scalato di 1.5 perde i
 * tratti da un pixel. La divisione per 6 e non per 5 tiene conto della riga di
 * spaziatura sotto i glifi -- a size 12 si ottiene scala 2 (10px di glifi,
 * ~12px di riga), a size 15-18 scala 3, che e' la corrispondenza con cui i
 * mock del layout V3 sono stati approvati.
   ============================================================ */
static int UiFontScale(int size)
{
    int scale = (size + 3)/6;
    if (scale < 1) scale = 1;
    if (scale > 6) scale = 6;
    return scale;
}

static void UiText(const char *text, int x, int y, int size, Color color)
{
    const ArtSheet *font = ArtUiFont();
    if (font) ArtDrawText(font, text, x, y, UiFontScale(size), color);
    else DrawText(text, x, y, size, color);
}

static int UiTextW(const char *text, int size)
{
    const ArtSheet *font = ArtUiFont();
    if (font) return ArtTextWidth(font, text, UiFontScale(size));
    return MeasureText(text, size);
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

/* ============================================================
   DEC-200 (WP-UI-0): DOVE SI DISEGNA.
 *
 * Da questa migrazione esiste UNA sola superficie di disegno -- il canvas
 * interno SCREEN_WIDTH x SCREEN_HEIGHT (640x360) -- e la finestra e' solo il
 * posto dove quel canvas viene ingrandito di un fattore INTERO alla fine del
 * frame (RendererDrawApp). "Schermo", in tutto il resto di questo file, vuol
 * quindi dire CANVAS: GetScreenWidth/GetScreenHeight NON compaiono piu' da
 * nessuna parte tranne UiComputeLayout qui sotto, che e' il solo punto in cui
 * le due coordinate si incontrano.
 *
 * Prima di WP-UI-0 non era cosi': la scena viveva nel canvas mentre HUD e
 * overlay si disegnavano DIRETTAMENTE nel framebuffer di finestra, scalandosi
 * a mano con uiScale. Quel doppio spazio e' esattamente cio' che rendeva
 * impossibile "un pixel di interfaccia = un pixel d'arte": a 1600x900 una
 * riga di menu era alta 40 px di finestra e il font pixel ne occupava 15,
 * senza alcun rapporto stabile fra i due.
   ============================================================ */
static float UiCanvasW(void) { return (float)SCREEN_WIDTH; }
static float UiCanvasH(void) { return (float)SCREEN_HEIGHT; }

/* Le geometrie storiche degli overlay (riquadri, quote delle righe, margini)
   sono scritte nella griglia di progetto 1600x900 -- l'uiScale 1.0 di M4, la
   finestra grande di riferimento. Il canvas 640x360 ne e' esattamente 0.4 su
   ENTRAMBI gli assi, quindi un unico fattore le riporta dentro il canvas
   conservando le proporzioni con cui erano state approvate, invece di
   rimisurare a mano sette schermate. E' una COSTANTE e non piu' una funzione
   della finestra: a scalare con la finestra, adesso, e' il blit finale, e lo
   fa per tutto insieme. Le schermate rivestite col nuovo sistema di token
   (src/render/ui_theme.h) non lo usano affatto -- lavorano in pixel di canvas
   diretti, che e' il modo giusto. */
#define UI_CANVAS_SCALE 0.4f

UiLayout UiComputeLayoutFor(float sw, float sh)
{
    /* Scala INTERA, mai frazionaria (DEC-200): il canvas e' campionato POINT e
       una scala come 1.625 (il vecchio aggancio a 1/8) fa cadere i pixel
       raddoppiati a distanze irregolari -- alcuni larghi 2, altri 1 -- che
       "brillano" appena la camera si muove. A scala intera ogni pixel di
       canvas diventa un quadrato identico a tutti gli altri. 640x360 e' 16:9,
       quindi su un monitor 16:9 la scala intera non lascia nemmeno bande:
       x2 = 720p, x3 = 1080p, x6 = 4K. */
    float scale = fminf(sw/(float)SCREEN_WIDTH, sh/(float)SCREEN_HEIGHT);
    scale = floorf(scale);
    /* Minimo 1: sotto, il canvas andrebbe RIMPICCIOLITO, cioe' si perderebbero
       righe di pixel. Meglio che una finestra piu' piccola del canvas ne
       tagli i bordi (caso di sola manutenzione: nessuna configurazione reale
       ha meno di 640x360 di area utile). */
    if (scale < 1.0f) scale = 1.0f;
    float gw = (float)SCREEN_WIDTH*scale;
    float gh = (float)SCREEN_HEIGHT*scale;

    UiLayout layout = { 0 };
    /* Centrato, con quel che avanza lasciato al letterbox (UI_GROUND): la
       centratura e' cio' che garantisce bande uguali sui due lati. Le
       coordinate restano intere -- una mezza unita' di offset spalmerebbe una
       colonna di pixel del canvas su due colonne di finestra. */
    layout.gameRect = (Rectangle){ floorf((sw - gw)*0.5f), floorf((sh - gh)*0.5f), gw, gh };
    layout.gameScale = scale;
    layout.uiScale = UI_CANVAS_SCALE;
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

Vector2 UiCanvasMouse(void)
{
    UiLayout layout = UiComputeLayout();
    Vector2 mouse = GetMousePosition();
    /* Senza clamp, a differenza di UiScreenToGameMouse sopra: un puntatore
       fuori dal canvas deve restare fuori da OGNI rettangolo interrogato,
       mentre un clamp ai bordi lo farebbe cadere su un widget appoggiato al
       bordo (in un menu centrato non succede, ma un hit-test che dipende da
       "per fortuna il pannello non tocca il bordo" e' un difetto in attesa). */
    return (Vector2){ (mouse.x - layout.gameRect.x)/layout.gameScale,
                      (mouse.y - layout.gameRect.y)/layout.gameScale };
}

/* Il canvas visto come UiLayout, per i disegni che vogliono ancora "il
   rettangolo su cui appoggiarsi" (l'HUD di ripiego, gli indicatori del Piano
   0). Da WP-UI-0 quel rettangolo E' il canvas, senza scala di mezzo. */
static UiLayout UiCanvasLayout(void)
{
    UiLayout layout = { 0 };
    layout.gameRect = (Rectangle){ 0.0f, 0.0f, UiCanvasW(), UiCanvasH() };
    layout.gameScale = 1.0f;
    layout.uiScale = UI_CANVAS_SCALE;
    return layout;
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

/* ============================================================
   W8: il TILESET della stanza (assets/art/tiles/<tema>.png).
 *
 * DA COSA SI SCEGLIE. Il motore non ha un identificatore di tema: Theme
 * (core/game_types.h) porta un NOME testuale e nient'altro -- nessun enum,
 * nessuno slug, perche' il nome lo INVENTA il modello ("Library of Radiation")
 * e i cinque temi della demo esistono solo come nomi nel contenuto di ripiego
 * (MakeFallbackTheme, content/run_content.c). La corrispondenza e' quindi in
 * due gradini:
 *   1. lo SLUG del nome (minuscolo, non-alfanumerico -> '-') e' provato come
 *      nome di file: "Lunar Forge" -> tiles/lunar-forge. E' il cammino esatto
 *      dei cinque temi curati, e resta valido per qualunque tema futuro a cui
 *      la sessione artistica dedichi un tileset omonimo, senza toccare il C;
 *   2. per un tema GENERATO, che non ha (e non avra') un tileset suo, si
 *      sceglie uno dei cinque per HASH del nome. Deterministico dal nome:
 *      lo stesso mondo si veste sempre allo stesso modo, in ogni run e in ogni
 *      macchina, che e' il minimo perche' la vestizione non sembri un difetto.
 * Nessun tileset trovato (checkout senza assets/art/) -> NULL, e la stanza si
 * disegna coi colori piatti del tema come prima di W8.
 *
 * VARIANTE DI ESCALATION (DEC-024, "il tema si intensifica piano dopo piano"
 * sull'asse aspetto). Il contratto d'arte emette tre ruoli col suffisso _deg
 * (floor_deg/wall_deg/void_deg, "crepe di brace"), ma NESSUN documento fissa a
 * quale piano scattano: e' un buco fra il contratto e il motore, registrato
 * come domanda aperta. DEFAULT PROPOSTO qui (stile DEC-019): scattano dal
 * PIANO 3, cioe' esattamente dove passa la seconda traccia di gameplay
 * (AUDIO_GAMEPLAY_1_MAX_FLOOR, src/audio/audio.c) e dove i boss passano a due
 * fasi (DEC-028/106). Far coincidere i tre assi dell'escalation su uno stesso
 * confine e' l'ipotesi piu' leggibile per il giocatore, ed e' quella che il
 * playtest dovra' confermare o spostare.
   ============================================================ */
#define ROOM_TILESET_DEGRADED_FROM_FLOOR 3

static const char *const ROOM_TILESET_KEYS[] = {
    "tiles/cathedral-of-sugar",
    "tiles/lunar-forge",
    "tiles/moldy-library",
    "tiles/neon-cellar",
    "tiles/radioactive-aquarium",
};
#define ROOM_TILESET_COUNT ((int)(sizeof(ROOM_TILESET_KEYS)/sizeof(ROOM_TILESET_KEYS[0])))

static const ArtSheet *RoomTileset(const Theme *theme)
{
    if (!theme) return NULL;

    char slug[80];
    int out = 0;
    for (int i = 0; theme->name[i] && out < (int)sizeof(slug) - 1; i++)
    {
        char c = theme->name[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        bool alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (alnum) slug[out++] = c;
        else if (out > 0 && slug[out - 1] != '-') slug[out++] = '-';
    }
    while (out > 0 && slug[out - 1] == '-') out--;
    slug[out] = '\0';

    if (out > 0)
    {
        char key[ART_KEY_LEN];
        int written = snprintf(key, sizeof(key), "tiles/%s", slug);
        if (written > 0 && written < (int)sizeof(key))
        {
            const ArtSheet *sheet = ArtAtlasGet(key);
            if (sheet) return sheet;
        }
    }

    /* Hash FNV-1a a 32 bit sul nome: qualunque funzione stabile andrebbe, ma
       una moltiplicativa distribuisce i nomi generati fra i cinque temi molto
       meglio della somma dei caratteri (i nomi del ripiego del generatore
       condividono quasi tutte le lettere: "Forge of Neon"/"Forge of Mold"). */
    unsigned int hash = 2166136261u;
    for (int i = 0; theme->name[i]; i++)
    {
        hash ^= (unsigned char)theme->name[i];
        hash *= 16777619u;
    }
    for (int i = 0; i < ROOM_TILESET_COUNT; i++)
    {
        const ArtSheet *sheet = ArtAtlasGet(ROOM_TILESET_KEYS[(hash + (unsigned int)i)%ROOM_TILESET_COUNT]);
        if (sheet) return sheet;
    }
    return NULL;
}

/* Il ruolo da usare per 'base' al piano corrente: la variante _deg dai piani
   avanzati, il ruolo normale prima (vedi il commento sopra). Il ripiego e'
   dentro ArtDrawTile: se un tileset non dichiarasse i ruoli _deg si otterrebbe
   false e la stanza tornerebbe al colore piatto -- quindi qui si prova la
   variante e chi chiama riprova il ruolo base. */
static bool RoomTileDegraded(const Game *game)
{
    return game->floor >= ROOM_TILESET_DEGRADED_FROM_FLOOR;
}

/* Disegna 'role' in 'dst' provando prima la variante di escalation quando il
   piano la richiede. Un tileset senza ruoli _deg si comporta come prima. */
static bool DrawRoomTile(const Game *game, const ArtSheet *tiles, const char *role,
                         const char *degradedRole, Rectangle dst, Color tint)
{
    if (degradedRole && RoomTileDegraded(game) && ArtDrawTile(tiles, degradedRole, dst, tint)) return true;
    return ArtDrawTile(tiles, role, dst, tint);
}

/* Riempie 'area' ripetendo il tile 'role'. 'varyRoles' (opzionale, chiuso da
   NULL) sono le varianti da alternare in modo DETERMINISTICO dalla posizione
   della cella: un pavimento fatto di un solo tile si legge come una
   quadrettatura, e alternare a caso lo farebbe sfarfallare da un frame
   all'altro -- la variante dipende quindi solo dalle coordinate del tile,
   quindi resta ferma mentre la telecamera si muove. */
static void DrawTiledArea(const Game *game, const ArtSheet *tiles, Rectangle area,
                          const char *role, const char *degradedRole,
                          const char *const *varyRoles, Color tint)
{
    if (!tiles || tiles->tileW <= 0 || tiles->tileH <= 0) return;
    if (area.width <= 0.0f || area.height <= 0.0f) return;
    float tw = (float)tiles->tileW, th = (float)tiles->tileH;
    int varyCount = 0;
    while (varyRoles && varyRoles[varyCount]) varyCount++;

    int row = 0;
    for (float y = area.y; y < area.y + area.height; y += th, row++)
    {
        int col = 0;
        for (float x = area.x; x < area.x + area.width; x += tw, col++)
        {
            Rectangle dst = { x, y, fminf(tw, area.x + area.width - x), fminf(th, area.y + area.height - y) };
            const char *use = role;
            if (varyCount > 0)
            {
                /* Due primi diversi per riga e colonna: una moltiplicazione
                   sola darebbe fasce verticali regolari invece di una
                   distribuzione sparsa. Il 3 su 4 di probabilita' che esca il
                   tile base tiene le varianti "occasionali" come vuole il
                   contratto (floor + tre varianti). */
                unsigned int h = (unsigned int)(col*73856093) ^ (unsigned int)(row*19349663);
                h ^= h >> 13;
                if ((h & 3u) != 0u) use = varyRoles[(int)((h >> 2)%(unsigned int)varyCount)];
            }
            if (!DrawRoomTile(game, tiles, use, degradedRole, dst, tint) && use != role)
                DrawRoomTile(game, tiles, role, degradedRole, dst, tint);
        }
    }
}

/* Lo STATO della porta di un lato, nel vocabolario del tileset (italiano, come
   props/porta): "bloccata" quando la stanza tiene chiuse le uscite finche' non
   la si pulisce (GameRoomIsLocked), "chiusa" quando la stanza non e' ancora
   pulita ma nemmeno bloccata, "aperta" altrimenti. Le tre parole sono i tre
   ruoli door_<lato>_<stato> del contratto, e sono anche le tre animazioni di
   props/porta: un solo vocabolario per il tile e per lo sprite. */
static const char *RoomDoorState(Game *game)
{
    if (GameRoomIsLocked(game)) return "bloccata";
    const RoomState *room = GameCurrentRoom(game);
    return (room && !room->cleared) ? "chiusa" : "aperta";
}

/* Una cella del riquadro che NON appartiene alla stanza (l'angolo mancante di
   una forma a L, DEC-170): muro pieno, con la stessa faccia illuminata in alto
   e lo stesso spigolo del muro di fondo, cosi' si legge come parete e non come
   un buco nel pavimento. Dal lato del giocatore e' solida davvero: e' un
   ostacolo (Game.obstacleHoleCount), non solo un disegno.
   Col tileset caricato il riempimento diventa il ruolo dedicato 'l_block', che
   il contratto d'arte emette esattamente per questo caso. */
static void DrawRoomHole(Game *game, const ArtSheet *tiles, Rectangle hole, Color wallDark, Color wallLit)
{
    if (tiles)
    {
        DrawTiledArea(game, tiles, hole, "l_block", NULL, NULL, WHITE);
        return;
    }
    DrawRectangleRec(hole, wallDark);
    DrawRectangleGradientV((int)hole.x, (int)hole.y, (int)hole.width, (int)WALL_BACK_H, wallLit, wallDark);
    DrawRectangle((int)hole.x, (int)(hole.y + hole.height - 2.0f), (int)hole.width, 2, wallLit);
}

/* WP8 (systems/secrets-and-obstacles.md, "Segreti" + "Feedback", DEC-025):
   l'INDIZIO della stanza segreta, sulla parete della stanza VISIBILE -- una
   crepa sottile (assets/art/props/crepa-segreta, tag "indizio", 2 fotogrammi)
   esattamente dove il varco si puo' sbrecciare, cioe' centrata sul segmento di
   parete che WorldSecretWallRect dichiara. Dopo la breccia lo stesso punto
   mostra il tag "aperta" (1 fotogramma), sopra la porta appena comparsa.

   Tre regole del documento, tutte qui e tutte verificabili:
   - "indizi leggibili ma discreti": nessuna freccia, nessun testo, nessuna
     evidenziazione lampeggiante -- solo la crepa, con la sua animazione lenta
     a due fotogrammi;
   - la SUPER-SEGRETA non ha MAI indizio (DEC-025, e il documento vieta
     esplicitamente di introdurne uno implicito): il filtro e'
     WorldSecretClueVisible, non una condizione scritta a mano qui;
   - l'indizio sta sulla parete della stanza da cui si vede, mai dentro la
     segreta: si itera sulle celle della stanza CORRENTE e si guardano le
     vicine, esattamente come fa WorldTryBreachSecretWall per decidere dove la
     bomba apre.

   Degrado quando l'asset manca (checkout senza assets/art/, pacchetto
   parziale): forma geometrica di riserva, MAI nessun indizio -- un indizio che
   sparisce col pacchetto artistico renderebbe la segreta normale
   indistinguibile da una super-segreta, cioe' cambierebbe il DESIGN e non solo
   la veste. */
static void DrawSecretWallHints(Game *game)
{
    if (game->floor <= 0) return;
    int cellX[4], cellY[4];
    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
    const ArtSheet *crack = ArtAtlasGet("props/crepa-segreta");
    for (int i = 0; i < cellCount; i++)
    {
        int cx = cellX[i], cy = cellY[i];
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
            int ny = cy + ((d == DIR_DOWN) - (d == DIR_UP));
            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
            if (!game->rooms[ny][nx].exists) continue;
            const RoomState *secret = WorldRoomAt(game, nx, ny);
            if (secret->kind != ROOM_SECRET) continue;
            bool clue = WorldSecretClueVisible(secret);
            if (!clue && !secret->secretOpened) continue;   /* super-segreta murata: nulla, mai */

            /* Il centro del SEGMENTO DI PARETE, riportato sul bordo vero della
               cella: la crepa e' sul muro, non a mezzo metro dentro la stanza.
               L'ancora dell'asset e' al centro del tile (manifest), quindi
               questo punto e' anche il centro dello sprite. */
            Rectangle wall = WorldSecretWallRect(game, cx, cy, d);
            Vector2 at = { wall.x + wall.width*0.5f, wall.y + wall.height*0.5f };
            Rectangle cell = WorldCellRect(game, cx, cy);
            if (d == DIR_UP) at.y = cell.y;
            else if (d == DIR_DOWN) at.y = cell.y + cell.height;
            else if (d == DIR_LEFT) at.x = cell.x;
            else if (d == DIR_RIGHT) at.x = cell.x + cell.width;

            bool drew = false;
            if (crack)
            {
                float scale = ArtScaleForWidth(crack->frameW, 64.0f);
                drew = ArtDrawAnim(crack, clue ? "indizio" : "aperta",
                                   (float)GetTime() + at.x*0.01f, at, scale, false, WHITE);
            }
            if (drew) continue;
            /* Riserva geometrica: due segni scuri incrociati, discreti quanto
               la crepa e leggibili senza colore (DEC-058). Aperto: un varco
               scuro pieno, cosi' i due stati non si confondono mai. */
            Color mark = GameColorLerp(game->theme.wall, BLACK, 0.65f);
            if (clue)
            {
                DrawLineEx((Vector2){ at.x - 14.0f, at.y - 10.0f }, (Vector2){ at.x + 4.0f, at.y + 8.0f }, 3.0f, mark);
                DrawLineEx((Vector2){ at.x + 2.0f, at.y - 12.0f }, (Vector2){ at.x + 15.0f, at.y + 6.0f }, 2.0f, mark);
            }
            else
            {
                DrawRectangle((int)(at.x - 22.0f), (int)(at.y - 12.0f), 44, 24, mark);
            }
        }
    }
}

/* La stanza VESTITA dal tileset (W8). Sostituisce i colori piatti + griglia in
 * prospettiva di DrawRoomFlat qui sotto, che resta il ripiego integrale.
 *
 * Ordine di disegno, dal fondo in avanti: il vuoto fuori dalla stanza, il
 * pavimento cella per cella, la cornice di muro sui quattro lati, gli angoli,
 * le celle-buco di una forma a L, e per ultime le porte (che sono buchi NEL
 * muro, quindi vanno sopra).
 *
 * PERCHE' LE FASCE DI MURO RESTANO QUELLE DECORATIVE DI SEMPRE
 * (WALL_BACK_H/WALL_SIDE_W/WALL_FRONT_H, 34/12/14 px) invece di diventare una
 * fila intera di tile da 32: quelle misure sono lo SPESSORE che la resa 2.5D
 * dichiara da sempre, e sono ancorate al bordo REALE del campo di gioco --
 * allargarle a 32 px per lato avrebbe spostato di 20 px la parete rispetto
 * alla collisione, cioe' avrebbe fatto camminare il giocatore dentro il muro.
 * Il tile viene quindi RITAGLIATO alla fascia (ArtDrawTile ritaglia il
 * sorgente, non comprime), e il muro laterale mostra i suoi primi 12 px: e'
 * esattamente cio' che si vede di uno spessore visto di taglio.
 */
static void DrawRoomTiled(Game *game, const ArtSheet *tiles)
{
    Rectangle roomRect = WorldCurrentRoomRect(game);
    /* Il vuoto copre tutta l'inquadratura: cosi' non resta mai una banda di
       ClearBackground visibile fra la cornice di muro e il bordo dello schermo,
       nemmeno mentre la telecamera scorre su una stanza multi-cella. */
    DrawTiledArea(game, tiles, WorldCameraView(game), "void", "void_deg", NULL, WHITE);

    static const char *const FLOOR_VARIANTS[] = { "floor_var1", "floor_var2", "floor_var3", NULL };
    int cellX[4], cellY[4];
    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
    for (int i = 0; i < cellCount; i++)
        DrawTiledArea(game, tiles, WorldCellRect(game, cellX[i], cellY[i]), "floor", "floor_deg", FLOOR_VARIANTS, WHITE);

    const float rx = roomRect.x, ry = roomRect.y, rw = roomRect.width, rh = roomRect.height;
    const float rRight = rx + rw, rBottom = ry + rh;
    DrawTiledArea(game, tiles, (Rectangle){ rx, ry - WALL_BACK_H, rw, WALL_BACK_H }, "wall_n", "wall_deg", NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rx, rBottom, rw, WALL_FRONT_H }, "wall_s", "wall_deg", NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rx - WALL_SIDE_W, ry, WALL_SIDE_W, rh }, "wall_w", "wall_deg", NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rRight, ry, WALL_SIDE_W, rh }, "wall_e", "wall_deg", NULL, WHITE);
    /* Angoli ESTERNI ai quattro spigoli della cornice: sono i quattro
       rettangoli che le quattro fasce qui sopra lasciano scoperti. */
    DrawTiledArea(game, tiles, (Rectangle){ rx - WALL_SIDE_W, ry - WALL_BACK_H, WALL_SIDE_W, WALL_BACK_H }, "corner_nw", NULL, NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rRight, ry - WALL_BACK_H, WALL_SIDE_W, WALL_BACK_H }, "corner_ne", NULL, NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rRight, rBottom, WALL_SIDE_W, WALL_FRONT_H }, "corner_se", NULL, NULL, WHITE);
    DrawTiledArea(game, tiles, (Rectangle){ rx - WALL_SIDE_W, rBottom, WALL_SIDE_W, WALL_FRONT_H }, "corner_sw", NULL, NULL, WHITE);

    int holes = WorldRoomHoleCount(game);
    for (int i = 0; i < holes; i++) DrawRoomHole(game, tiles, WorldRoomHoleRect(game, i), BLACK, BLACK);

    const char *state = RoomDoorState(game);
    for (int i = 0; i < cellCount; i++)
    {
        const RoomState *cell = &game->rooms[cellY[i]][cellX[i]];
        Rectangle c = WorldCellRect(game, cellX[i], cellY[i]);
        float ccx = c.x + c.width*0.5f, ccy = c.y + c.height*0.5f;
        float cRight = c.x + c.width, cBottom = c.y + c.height;
        char role[ART_ROLE_NAME_LEN];
        if (cell->doors[DIR_UP])
        {
            snprintf(role, sizeof(role), "door_n_%s", state);
            DrawTiledArea(game, tiles, (Rectangle){ ccx - DOOR_HALF, c.y - WALL_BACK_H, DOOR_HALF*2.0f, WALL_BACK_H }, role, NULL, NULL, WHITE);
        }
        if (cell->doors[DIR_DOWN])
        {
            snprintf(role, sizeof(role), "door_s_%s", state);
            DrawTiledArea(game, tiles, (Rectangle){ ccx - DOOR_HALF, cBottom, DOOR_HALF*2.0f, WALL_FRONT_H }, role, NULL, NULL, WHITE);
        }
        if (cell->doors[DIR_LEFT])
        {
            snprintf(role, sizeof(role), "door_w_%s", state);
            DrawTiledArea(game, tiles, (Rectangle){ c.x - WALL_SIDE_W, ccy - DOOR_HALF, WALL_SIDE_W, DOOR_HALF*2.0f }, role, NULL, NULL, WHITE);
        }
        if (cell->doors[DIR_RIGHT])
        {
            snprintf(role, sizeof(role), "door_e_%s", state);
            DrawTiledArea(game, tiles, (Rectangle){ cRight, ccy - DOOR_HALF, WALL_SIDE_W, DOOR_HALF*2.0f }, role, NULL, NULL, WHITE);
        }
    }

    /* La sfumatura che scurisce il fondo della stanza resta anche col tileset:
       e' la prospettiva atmosferica della resa 2.5D (step E, trucco 3), non una
       texture -- senza, un pavimento a tile piatto perde ogni profondita'. */
    DrawRectangleGradientV((int)rx, (int)ry, (int)rw, (int)(rh*0.35f),
                           GameColorWithAlpha(BLACK, 52), BLANK);
    /* WP8: la crepa va SOPRA la parete e sopra la porta appena aperta (che e'
       un buco NEL muro, disegnato poco sopra), ma sotto le entita': e' un
       dettaglio dell'ambiente, non un'entita'. */
    DrawSecretWallHints(game);
    if (game->floor == 0) DrawFloorZeroExitGate(game);
}

static void DrawRoomFlat(Game *game)
{
    /* DEC-170: si disegna in coordinate di MONDO, dentro la telecamera (vedi
       DrawGameplayCanvas). La stanza e' il RIQUADRO delle sue celle: una per
       la 1x1 (inquadratura fissa di sempre), fino a quattro per una 2x2. Si
       riempie PRIMA tutto il riquadro piu' lo spessore dei muri di muro scuro,
       poi ci si disegna sopra il pavimento: lo spazio fra stanza e bordo
       dell'inquadratura resta automaticamente muro, senza calcolare margini
       variabili per lato. */
    Color wallDark = GameColorLerp(game->theme.wall, BLACK, 0.45f);
    Color wallLit = GameColorLerp(game->theme.wall, WHITE, 0.18f);

    Rectangle roomRect = WorldCurrentRoomRect(game);
    DrawRectangleRec((Rectangle){ roomRect.x - WALL_SIDE_W, roomRect.y - WALL_BACK_H,
                                   roomRect.width + WALL_SIDE_W*2.0f,
                                   roomRect.height + WALL_BACK_H + WALL_FRONT_H }, wallDark);

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

    /* DEC-170: l'angolo mancante di una forma a L e' muro, disegnato SOPRA il
       pavimento del riquadro (che e' stato riempito per intero sopra: un
       rettangolo pieno costa meno di quattro rettangoli condizionati, e la
       griglia in prospettiva resta una sola per stanza invece di spezzarsi in
       quattro). */
    int holes = WorldRoomHoleCount(game);
    for (int i = 0; i < holes; i++) DrawRoomHole(game, NULL, WorldRoomHoleRect(game, i), wallDark, wallLit);

    Color doorColor = GameRoomIsLocked(game) ? (Color){ 200, 58, 58, 255 } : game->theme.accent2;
    /* Le porte si disegnano SOPRA i muri (sono buchi nel muro), al centro del
       lato della CELLA a cui appartengono (DEC-170: una stanza multi-cella ha
       porte su piu' celle, fino a otto lati esterni per una 2x2): quella di
       fondo occupa tutta la faccia del muro, cosi' si legge come un passaggio
       e non come una striscia appoggiata. */
    int cellX[4], cellY[4];
    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
    for (int i = 0; i < cellCount; i++)
    {
        const RoomState *cell = &game->rooms[cellY[i]][cellX[i]];
        Rectangle c = WorldCellRect(game, cellX[i], cellY[i]);
        float ccx = c.x + c.width*0.5f, ccy = c.y + c.height*0.5f;
        float cRight = c.x + c.width, cBottom = c.y + c.height;
        if (cell->doors[DIR_UP]) DrawRectangle((int)(ccx - DOOR_HALF), (int)(c.y - WALL_BACK_H), (int)(DOOR_HALF*2), (int)WALL_BACK_H, doorColor);
        if (cell->doors[DIR_DOWN]) DrawRectangle((int)(ccx - DOOR_HALF), (int)cBottom, (int)(DOOR_HALF*2), (int)WALL_FRONT_H, doorColor);
        if (cell->doors[DIR_LEFT]) DrawRectangle((int)(c.x - WALL_SIDE_W), (int)(ccy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);
        if (cell->doors[DIR_RIGHT]) DrawRectangle((int)cRight, (int)(ccy - DOOR_HALF), (int)WALL_SIDE_W, (int)(DOOR_HALF*2), doorColor);
    }

    /* WP8: la crepa della stanza segreta, sopra la parete e sopra la porta --
       stesso punto e stesso ordine del percorso col tileset (DrawRoomTiled).
       Deve esistere su ENTRAMBI i percorsi di disegno: il ripiego piatto non
       e' un degrado accettabile per un INDIZIO di design (senza, la segreta
       normale sarebbe indistinguibile da una super-segreta). */
    DrawSecretWallHints(game);

    /* Piano 0 (M1b): il varco verso il piano 1, nel muro di fondo. NON e' un
       room->doors[DIR_UP] (vedi FloorZeroEnter, src/world/floor_zero.c: la
       griglia ha una sola cella, quindi quell'array resta tutto falso) --
       ha un aspetto dedicato apposta, cosi' resta leggibile come "l'uscita
       speciale della sala d'attesa" anche a chi non guarda la minimappa. Usa
       ROOM_X/ROOM_W fissi (non roomRect): il Piano 0 e' sempre una sola cella,
       dove le due cose coincidono. */
    if (game->floor == 0) DrawFloorZeroExitGate(game);
}

/* Il punto di scelta fra le due rese della stanza: il tileset originale se il
   tema ne ha uno caricabile (W8), i colori piatti del tema altrimenti. Un solo
   ramo, qui: nessun altro punto del renderer deve sapere che esistono due
   percorsi. */
static void DrawRoom(Game *game)
{
    const ArtSheet *tiles = RoomTileset(&game->theme);
    if (tiles) DrawRoomTiled(game, tiles);
    else DrawRoomFlat(game);
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

/* ============================================================
   W8: dove POGGIA uno sprite, e quanto e' grande.
 *
 * L'ancora dei manifest e' ai piedi (anchor y ~28 su 32 per un personaggio, 56
 * su 64 per un boss), mentre la posizione di un'entita' nel motore e' il CENTRO
 * del suo cerchio di collisione. Il punto di appoggio si ricava quindi dal
 * raggio, e -- deliberatamente -- con la STESSA frazione che DrawGroundShadow
 * usa per la sua ombra: sprite e ombra devono toccare il pavimento nello stesso
 * punto, o l'entita' sembra sollevata da terra (era proprio il difetto che
 * l'ombra del giocatore, ancorata al raggio invece che al piede, aveva mostrato
 * nella resa a stickman).
   ============================================================ */
static Vector2 SpriteGroundPos(Vector2 pos, float radius, float footFraction)
{
    return (Vector2){ pos.x, pos.y + radius*footFraction };
}

/* La scala di uno sprite di entita': tanto quanto serve perche' occupi la
   stessa larghezza a schermo che la cella d'atlas occupava (raggio*3.3),
   agganciata a mezzi passi. Cosi' un nemico "grande" (sizeMul alto) resta
   grande e uno piccolo resta piccolo, come dice il suo tipo -- il tier
   disegnato (32/48/64 px) cambia solo la definizione, mai l'ingombro. */
static float EntitySpriteScale(const ArtSheet *sheet, float radius)
{
    return ArtScaleForWidth(sheet->frameW, radius*3.3f);
}

/* Lo spritesheet di un nemico: l'image-id del suo tipo (DEC-175(b)) risolto
   nella categoria giusta. Un boss cerca in bosses/, un nemico normale in
   enemies/; ArtAtlasFindByImageId scandisce comunque tutte le categorie, quindi
   un boss il cui sprite fosse stato consegnato fra i nemici (o viceversa) si
   trova comunque. NULL = nessun originale: si ricade sulle sagome geometriche. */
static const ArtSheet *EnemySheet(const Enemy *e)
{
    if (!e->type.imageId[0]) return NULL;
    return ArtAtlasFindByImageId(e->type.imageId);
}

/* L'animazione di un nemico VIVO. 'hit' vince su tutto (il colpo subito deve
   vedersi), poi la camminata; 'idle' esiste solo sui boss e fa da riposo. La
   morte non passa da qui: e' un ArtFx che sopravvive all'entita' (vedi ArtFx in
   core/game_types.h). */
static void DrawEnemySprite(const ArtSheet *sheet, const Enemy *e, Color tint)
{
    Vector2 ground = SpriteGroundPos(e->pos, e->radius, 0.62f);
    float scale = EntitySpriteScale(sheet, e->radius);
    /* Si guarda a sinistra quando ci si muove a sinistra, e si RESTA girati
       quando si e' fermi (nessuna soglia sullo zero: uno |vel.x| minuscolo
       farebbe sfarfallare il verso ad ogni frame). Gli sprite dei nemici sono
       disegnati verso destra per contratto, quindi si specchia solo l'altro
       verso. */
    bool flip = e->vel.x < -1.0f;
    if (e->hitFlash > 0.0f && ArtDrawAnim(sheet, "hit", 0.0f, ground, scale, flip, tint)) return;
    /* La fase per-nemico entra nel tempo dell'animazione: senza, tutti i
       nemici della stanza camminerebbero all'unisono (lo stesso motivo per cui
       EntitiesAddEnemyTyped randomizza e->phase per il movimento). */
    float elapsed = (float)GetTime() + e->phase;
    bool moving = (e->vel.x*e->vel.x + e->vel.y*e->vel.y) > 4.0f;
    if (!moving && ArtDrawAnim(sheet, "idle", elapsed, ground, scale, flip, tint)) return;
    if (ArtDrawAnim(sheet, "walk", elapsed, ground, scale, flip, tint)) return;
    if (ArtDrawAnim(sheet, "idle", elapsed, ground, scale, flip, tint)) return;
    /* Nessuno dei nomi canonici: si disegna comunque la prima riga dello
       sheet, che e' sempre l'animazione principale per contratto. Meglio uno
       sprite fermo che nessuno sprite. */
    if (sheet->animCount > 0)
        ArtDrawFrame(sheet, sheet->anims[0].row, ArtAnimFrameAt(&sheet->anims[0], elapsed), ground, scale, flip, tint);
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
    /* W8, PRIMO gradino della priorita': l'originale animato di assets/art/.
       Sta PRIMA del ramo 'atlasLoaded' perche' vince sull'atlas generato: e'
       arte disegnata a mano per QUESTO nemico, contro una cella scelta dalla
       sua forma generica. */
    const ArtSheet *sheet = EnemySheet(e);
    if (sheet)
    {
        DrawEnemySprite(sheet, e, WHITE);
        drew = true;
    }
    if (!drew && game->atlasLoaded)
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
       disegni lo sprite sia la forma di riserva. DEC-170: ancorato all'angolo
       dell'INQUADRATURA, non a quello della stanza -- con una arena 2x2
       l'angolo della stanza puo' restare fuori schermo per tutto lo scontro.
       Si disegna dentro la telecamera (come tutto il resto di questa
       funzione), quindi il punto va espresso in coordinate di mondo; per una
       1x1 cade esattamente dove cadeva prima. */
    if (e->kind == ENEMY_BOSS)
    {
        Rectangle view = WorldCameraView(game);
        UiText(game->theme.bossName, (int)(view.x + ROOM_X + 20.0f), (int)(view.y + ROOM_Y + 16.0f), 18, RAYWHITE);
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

/* L'icona di un oggetto: lo sprite CURATO se l'oggetto ne porta uno
   (DEC-171 -- oggi solo gli oggetti nati da una fusione, vedi
   Item.imagePath), altrimenti la cella d'atlas SPR_ITEM di sempre. Ritorna
   false quando non ha disegnato nulla (nessuna delle due sorgenti
   disponibile): il chiamante ricade sulla forma geometrica, esattamente come
   faceva prima con DrawAtlasCell da sola.
   Le proporzioni dell'immagine curata si conservano (le sorgenti CC0 non
   sono quadrate: 52x49, 32x43...): si adatta il lato piu' lungo a 'size' e
   si centra, invece di stirare lo sprite dentro un quadrato. */
static bool DrawItemIcon(Game *game, const Item *item, Vector2 center, float size)
{
    /* W8, PRIMO gradino della priorita' delle immagini (DEC-175(b) + DEC-171):
       l'originale animato di assets/art/ risolto dall'image-id, POI il ponte
       CC0 di assets/curated/ risolto dal percorso, POI la cella d'atlas, POI la
       forma geometrica. I quattro gradini sono uno solo per punto di disegno --
       inventario, piedistallo, anteprima del piano e fascia di fusione passano
       tutti da qui, cosi' un oggetto non puo' mostrare due immagini diverse in
       due schermate.
       L'icona di un oggetto usa 'glow' quando c'e' (una pulsazione lenta che lo
       fa notare a terra) e 'idle' altrimenti: sono i due nomi che il contratto
       emette per la categoria oggetti. */
    if (item->imageId[0])
    {
        const ArtSheet *sheet = ArtAtlasFindByImageId(item->imageId);
        if (sheet)
        {
            float scale = ArtScaleForWidth(sheet->frameW, size);
            /* Un'icona si centra sul punto richiesto, non ci poggia: qui il
               punto e' il centro di una casella d'inventario o di un pickup,
               non un pavimento. Si compensa quindi l'ancora ai piedi
               riportando il centro del fotogramma sul centro chiesto. */
            Vector2 anchorPos = { center.x + ((float)sheet->anchorX - (float)sheet->frameW*0.5f)*scale,
                                  center.y + ((float)sheet->anchorY - (float)sheet->frameH*0.5f)*scale };
            if (ArtDrawAnim(sheet, "glow", (float)GetTime(), anchorPos, scale, false, WHITE)) return true;
            if (ArtDrawAnim(sheet, "idle", (float)GetTime(), anchorPos, scale, false, WHITE)) return true;
        }
    }
    if (item->imagePath[0])
    {
        const Texture2D *tex = AssetsCuratedTexture(game, item->imagePath);
        if (tex && tex->width > 0 && tex->height > 0)
        {
            float scale = (tex->width >= tex->height) ? size/(float)tex->width : size/(float)tex->height;
            float w = (float)tex->width*scale;
            float h = (float)tex->height*scale;
            Rectangle src = { 0.0f, 0.0f, (float)tex->width, (float)tex->height };
            Rectangle dst = { center.x - w*0.5f, center.y - h*0.5f, w, h };
            DrawTexturePro(*tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            return true;
        }
    }
    return DrawAtlasCell(game, SPR_ITEM, center, size, WHITE);
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
       (gate di qualita' fallito) deve far ripiegare sulla forma geometrica
       (con la sua etichetta, es. "EXIT" per PICKUP_EXIT piu' sotto), mai
       lasciare il pickup invisibile. */
    bool drew = false;
    /* W8/WP-INT: i prop originali di assets/art/props per le raccolte che ne
       hanno uno -- lingotto, Flux, e da WP-INT anche cuore/bomba/chiave
       (known-issues.md #10.3, CHIUSO per queste tre). Energia e uscita
       restano forme geometriche PER DECISIONE (nessuna cella d'atlas
       prevista, vedi il commento sotto), non per mancanza di asset. */
    const char *propKey = NULL;
    if (p->kind == PICKUP_COIN) propKey = "props/pickup-lingotto";
    else if (p->kind == PICKUP_FLUX) propKey = "props/pickup-flux";
    else if (p->kind == PICKUP_HEART) propKey = "props/pickup-cuore";
    else if (p->kind == PICKUP_BOMB) propKey = "props/pickup-bomba";
    else if (p->kind == PICKUP_KEY) propKey = "props/pickup-chiave";
    /* WP4: il crogiolo interagibile della stanza di fusione. L'asset dedicato
       (assets/art/props/crogiolo, tag "spento"/"attivo") e' gia' nel dataset
       curato -- ma MAI bloccarsi su asset in produzione: se dovesse mancare
       (rigenerazione, dataset parziale) si ripiega sul piedistallo generico
       (tag "vuoto"/"pieno", stesso ruolo visivo "oggetto interagibile su un
       basamento"), e se anche quello manca la forma geometrica sotto resta
       comunque disegnata. */
    else if (p->kind == PICKUP_FUSION_ALTAR) propKey = "props/crogiolo";
    /* WP5: il segnale della stanza a tempo (DEC-051). A differenza del
       crogiolo sopra, l'etichetta si scrive SEMPRE (anche se il prop dedicato
       manca e si ripiega sulla forma geometrica sotto): l'esito deve restare
       leggibile "senza solo colore" (special-rooms.md) in ogni caso, non solo
       quando lo sprite carica. */
    else if (p->kind == PICKUP_TIMED_MARKER)
    {
        propKey = "props/clessidra";
        label = (p->value != 0) ? "IN TEMPO" : "SCADUTO";
    }
    /* WP6: il segnale dell'arena di sfida. Come la clessidra sopra l'etichetta
       si scrive SEMPRE, anche quando il prop manca e si ripiega sulla forma
       geometrica: lo stato della sfida deve restare leggibile "senza solo
       colore" (DEC-058) in ogni caso. Tre stati, tre parole diverse -- mai due
       tinte dello stesso testo. */
    else if (p->kind == PICKUP_ARENA_ALTAR)
    {
        propKey = "props/piedistallo";
        label = (p->value >= 2) ? "SUPERATA" : ((p->value == 1) ? "IN CORSO" : "SFIDA");
    }
    /* WP7: il banco della Pourhouse. Come i due segnali sopra l'etichetta si
       scrive SEMPRE; le due righe con offerta e prezzo PER ESTESO si
       aggiungono in fondo alla funzione, dove c'e' spazio sotto lo sprite. */
    else if (p->kind == PICKUP_POURHOUSE_BANK)
    {
        propKey = "props/piedistallo";
        label = (p->value >= 2) ? "VERSATA" : ((p->value == 1) ? "PUNTATA" : "FREDDA");
    }
    /* WP15a: la piazzola d'arena del Piano 0. Come i tre segnali sopra
       l'etichetta si scrive SEMPRE, anche quando il prop manca: il TEMA della
       prova e' l'unica informazione che distingue una piazzola dall'altra e non
       puo' viaggiare sul solo colore (DEC-058). */
    else if (p->kind == PICKUP_TRIAL_GATE)
    {
        propKey = "props/piedistallo";
        label = FloorZeroArenaThemeLabel((FloorZeroTrialTheme)((p->value >= 0 && p->value < FLOOR_ZERO_TRIAL_COUNT) ? p->value : 0));
    }
    if (propKey)
    {
        const ArtSheet *prop = ArtAtlasGet(propKey);
        /* WP4: il crogiolo dedicato ("attivo"/"spento") e il suo ripiego
           generico ("pieno"/"vuoto") non condividono il vocabolario di
           animazione di pickup-flux/pickup-lingotto ("idle") -- il crogiolo
           e' SEMPRE utilizzabile (apre BuildScreen a prescindere dai requisiti
           di fusione, Scenario 4 di special-rooms.md), quindi si sceglie
           sempre lo stato "acceso"/"pieno" del prop, mai quello spento. */
        const char *animName = "idle";
        if (p->kind == PICKUP_FUSION_ALTAR)
        {
            animName = "attivo";
            if (!prop) { prop = ArtAtlasGet("props/piedistallo"); animName = "pieno"; }
        }
        /* WP5: la clessidra usa DAVVERO il proprio vocabolario a due tag
           (assets/art/props/clessidra.json: "attiva"/"scaduta") -- niente
           ripiego di prop generico come il crogiolo sopra (nessun prop
           esistente ha un vocabolario "in tempo/scaduto" compatibile): se
           manca, si scende direttamente alla forma geometrica sotto, il
           degrado standard di ogni altro pickup di questa funzione. */
        else if (p->kind == PICKUP_TIMED_MARKER)
        {
            animName = (p->value != 0) ? "attiva" : "scaduta";
        }
        /* WP6: nessun prop dedicato all'arena e' stato prodotto -- si riusa il
           piedistallo generico (vocabolario "vuoto"/"pieno"), con "pieno"
           finche' la sfida e' disponibile o in corso (c'e' qualcosa da fare) e
           "vuoto" quando e' superata (non c'e' piu' nulla). Se manca anche
           quello si scende alla forma geometrica sotto, il degrado standard. */
        else if (p->kind == PICKUP_ARENA_ALTAR)
        {
            animName = (p->value >= 2) ? "vuoto" : "pieno";
        }
        /* WP7: nessun prop dedicato al banco della Pourhouse -- si riusa il
           piedistallo generico come l'arena, con "pieno" finche' c'e' una
           puntata aperta e "vuoto" quando e' gia' versata o quando la colata
           e' fredda. Se manca anche quello si scende alla forma geometrica
           sotto, il degrado standard. */
        else if (p->kind == PICKUP_POURHOUSE_BANK)
        {
            animName = (p->value == 1) ? "pieno" : "vuoto";
        }
        /* WP15a: la piazzola e' sempre disponibile -- le prove sono illimitate
           (DEC-095), quindi non ha uno stato "consumato": sempre "pieno". */
        else if (p->kind == PICKUP_TRIAL_GATE)
        {
            animName = "pieno";
        }
        if (prop)
        {
            float scale = ArtScaleForWidth(prop->frameW, 28.0f);
            drew = ArtDrawAnim(prop, animName, (float)GetTime() + p->pos.x*0.01f,
                               SpriteGroundPos(pos, p->radius, 0.7f), scale, false, WHITE);
        }
    }
    if (!drew && p->kind == PICKUP_ITEM && (p->item.imageId[0] || p->item.imagePath[0]))
    {
        drew = DrawItemIcon(game, &p->item, pos, 46.0f);
        if (drew) label = p->item.name;
    }
    if (!drew && game->atlasLoaded)
    {
        int cell = SPR_ITEM;
        float size = 46.0f;
        if (p->kind == PICKUP_HEART) { cell = SPR_HEART; label = "HP"; }
        else if (p->kind == PICKUP_COIN) { cell = SPR_COIN; label = "$"; }
        else if (p->kind == PICKUP_BOMB) { cell = SPR_BOMB; label = "B"; }
        else if (p->kind == PICKUP_KEY) { cell = SPR_KEY; label = "K"; }
        else if (p->kind == PICKUP_EXIT) { cell = SPR_EXIT; label = "EXIT"; size = 78.0f; }
        else if (p->kind == PICKUP_ENERGY) cell = -1;   /* nessuna cella d'atlas: vedi PickupKind in core/game_types.h */
        else if (p->kind == PICKUP_FLUX) cell = -1;     /* idem: il catalizzatore di fusione e' una forma geometrica */
        else if (p->kind == PICKUP_CRUST) cell = -1;    /* idem: Crust (DEC-008/WP2) e' una forma geometrica */
        else if (p->kind == PICKUP_FUSION_ALTAR) cell = -1;   /* idem: il crogiolo (WP4) e' un prop/forma, mai una cella d'atlas */
        /* WP5: idem -- senza questo ramo, quando props/clessidra manca ma
           l'atlas generato E' caricato, si cadrebbe nel ramo 'else' sotto:
           l'etichetta "IN TEMPO"/"SCADUTO" gia' scritta sopra verrebbe
           SOVRASCRITTA da p->item.name (non azzerato da EntitiesAddPickup per
           un pickup non-oggetto: resta il nome residuo di un vecchio slot
           riciclato) e verrebbe disegnata la cella generica SPR_ITEM, quindi
           la clessidra geometrica di riserva (sotto) non si raggiungerebbe
           mai. La clessidra e' una forma/prop dedicata, mai una cella
           d'atlas generica, stessa ragione del crogiolo sopra. */
        else if (p->kind == PICKUP_TIMED_MARKER) cell = -1;
        else if (p->kind == PICKUP_ARENA_ALTAR) cell = -1;   /* WP6: idem -- prop/forma dedicata, mai una cella d'atlas generica (e l'etichetta di stato non va sovrascritta da p->item.name) */
        else if (p->kind == PICKUP_POURHOUSE_BANK) cell = -1;   /* WP7: idem per il banco della Pourhouse */
        else if (p->kind == PICKUP_TRIAL_GATE) cell = -1;      /* WP15a: idem per la piazzola d'arena del Piano 0 */
        else label = p->item.name;
        if (cell >= 0) drew = DrawAtlasCell(game, cell, pos, size, WHITE);
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
        else if (p->kind == PICKUP_ENERGY)
        {
            /* DEC-059, canale 2: una scintilla, non uno sprite. Forma
               geometrica di proposito -- una cella d'atlas nuova
               invaliderebbe ogni atlas gia' generato (vedi AtlasSprite). */
            c = (Color){ 126, 232, 152, 255 };
            label = "E";
            DrawCircleV(pos, 9, GameColorWithAlpha(c, 110));
            DrawCircleV(pos, 5, c);
        }
        else if (p->kind == PICKUP_FLUX)
        {
            /* Catalizzatore di fusione (DEC-022): un rombo, l'unica silhouette
               non ancora usata da un'altra raccolta -- si distingue a colpo
               d'occhio da moneta (cerchio), cuore, bomba e scintilla anche
               senza leggere l'etichetta. Come l'energia e' una forma
               geometrica e non una cella d'atlas: aggiungerne una
               invaliderebbe ogni atlas gia' generato (vedi AtlasSprite). */
            c = (Color){ 226, 138, 255, 255 };
            label = "F";
            Vector2 up = { pos.x, pos.y - 13 }, right = { pos.x + 11, pos.y }, down = { pos.x, pos.y + 13 }, left = { pos.x - 11, pos.y };
            DrawTriangle(left, down, up, c);
            DrawTriangle(up, down, right, c);
            DrawCircleV(pos, 4, GameColorWithAlpha(RAYWHITE, 200));
        }
        else if (p->kind == PICKUP_CRUST)
        {
            /* Crust (DEC-008/WP2): un piccolo scudo esagonale, la silhouette
               non ancora usata da un'altra raccolta (cerchio/rombo/scintilla
               gia' presi sopra) -- si distingue a colpo d'occhio anche senza
               leggere l'etichetta. Forma geometrica di proposito, stessa
               ragione di energia/Flux sopra: una cella d'atlas nuova
               invaliderebbe ogni atlas gia' generato (vedi AtlasSprite). */
            c = (Color){ 150, 176, 214, 255 };
            label = "C";
            Vector2 hex[6] = {
                { pos.x, pos.y - 13 }, { pos.x + 11, pos.y - 6 }, { pos.x + 11, pos.y + 7 },
                { pos.x, pos.y + 14 }, { pos.x - 11, pos.y + 7 }, { pos.x - 11, pos.y - 6 },
            };
            for (int i = 0; i < 6; i++) DrawTriangle(pos, hex[i], hex[(i + 1) % 6], c);
            DrawCircleV(pos, 4, GameColorWithAlpha(RAYWHITE, 200));
        }
        else if (p->kind == PICKUP_FUSION_ALTAR)
        {
            /* WP4: un basamento (rettangolo) con una fiamma (triangolo) sopra
               -- la silhouette "crogiolo/altare" che nessun'altra raccolta usa,
               distinguibile a colpo d'occhio anche senza etichetta. Colore
               ambrato/braci, coerente col tema del crogiolo del Piano 0
               (ROOM_HUB) senza riusarne l'esatta tinta -- gia' presa
               altrove (l'identita' di ROOM_HUB) per non confondere le due
               stanze. */
            c = (Color){ 255, 148, 61, 255 };
            label = "CR";
            DrawRectangle((int)pos.x - 13, (int)pos.y + 2, 26, 12, DARKGRAY);
            Vector2 flameL = { pos.x - 9, pos.y + 3 }, flameR = { pos.x + 9, pos.y + 3 }, flameTip = { pos.x, pos.y - 14 };
            DrawTriangle(flameL, flameR, flameTip, c);
            DrawCircleV((Vector2){ pos.x, pos.y - 2 }, 4, GameColorWithAlpha(RAYWHITE, 200));
        }
        else if (p->kind == PICKUP_TIMED_MARKER)
        {
            /* WP5: una clessidra disegnata (due triangoli contrapposti dentro
               una cornice) -- la silhouette "tempo" che nessun'altra raccolta
               usa. Colore acceso/spento coerente col tag reale della
               clessidra ("attiva"/"scaduta") cosi' anche il degrado
               geometrico porta lo stesso segnale del prop dedicato, non solo
               l'etichetta testuale sopra. */
            c = (p->value != 0) ? (Color){ 96, 224, 214, 255 } : (Color){ 140, 140, 148, 255 };
            DrawRectangleLines((int)pos.x - 11, (int)pos.y - 14, 22, 28, DARKGRAY);
            Vector2 topL = { pos.x - 9, pos.y - 12 }, topR = { pos.x + 9, pos.y - 12 };
            Vector2 botL = { pos.x - 9, pos.y + 12 }, botR = { pos.x + 9, pos.y + 12 };
            Vector2 mid = { pos.x, pos.y };
            DrawTriangle(topL, topR, mid, c);
            DrawTriangle(mid, botL, botR, c);
        }
        else if (p->kind == PICKUP_ARENA_ALTAR)
        {
            /* WP6: due lame incrociate su un basamento -- la silhouette
               "sfida" che nessun'altra raccolta usa (cerchio, rombo, esagono,
               fiamma e clessidra sono gia' prese). Il colore segue lo stato
               (disponibile / in corso / superata), ma non e' MAI l'unico
               canale: l'etichetta testuale sopra e' sempre disegnata. */
            c = (p->value >= 2) ? (Color){ 140, 140, 148, 255 }
                                : ((p->value == 1) ? (Color){ 246, 128, 96, 255 } : (Color){ 122, 168, 255, 255 });
            DrawRectangle((int)pos.x - 13, (int)pos.y + 4, 26, 10, DARKGRAY);
            DrawLineEx((Vector2){ pos.x - 10, pos.y + 6 }, (Vector2){ pos.x + 10, pos.y - 14 }, 4.0f, c);
            DrawLineEx((Vector2){ pos.x + 10, pos.y + 6 }, (Vector2){ pos.x - 10, pos.y - 14 }, 4.0f, c);
        }
        else if (p->kind == PICKUP_POURHOUSE_BANK)
        {
            /* WP7: un crogiolo rovesciato che cola -- il banco della «Casa
               della Colata» (DEC-136). Silhouette non ancora usata da nessuna
               altra raccolta (cerchio, rombo, esagono, fiamma, clessidra e
               lame incrociate sono gia' prese). Il colore segue lo stato del
               banco, ma non e' MAI l'unico canale: l'etichetta e le due righe
               della puntata sono sempre scritte. */
            c = (p->value >= 2) ? (Color){ 140, 140, 148, 255 }
                                : ((p->value == 1) ? (Color){ 245, 128, 186, 255 } : (Color){ 132, 122, 130, 255 });
            DrawRectangle((int)pos.x - 15, (int)pos.y + 4, 30, 9, DARKGRAY);
            Vector2 lipL = { pos.x - 13, pos.y - 13 }, lipR = { pos.x + 13, pos.y - 13 }, spout = { pos.x + 2, pos.y + 3 };
            DrawTriangle(lipL, spout, lipR, c);
            DrawLineEx((Vector2){ pos.x + 2, pos.y + 3 }, (Vector2){ pos.x + 2, pos.y + 12 }, 3.0f, c);
        }
        else if (p->kind == PICKUP_TRIAL_GATE)
        {
            /* WP15a: un arco su un basamento -- il "varco" della piazzola. E'
               la silhouette che nessun'altra raccolta usa (cerchio, rombo,
               esagono, fiamma, clessidra, lame incrociate e crogiolo colante
               sono gia' prese). Tinta ambrata come il crogiolo del Piano 0:
               le piazzole appartengono all'hub, non a una stanza di un piano. */
            c = (Color){ 236, 178, 92, 255 };
            DrawRectangle((int)pos.x - 16, (int)pos.y + 6, 32, 9, DARKGRAY);
            DrawLineEx((Vector2){ pos.x - 12, pos.y + 6 }, (Vector2){ pos.x - 12, pos.y - 6 }, 4.0f, c);
            DrawLineEx((Vector2){ pos.x + 12, pos.y + 6 }, (Vector2){ pos.x + 12, pos.y - 6 }, 4.0f, c);
            DrawLineEx((Vector2){ pos.x - 12, pos.y - 6 }, (Vector2){ pos.x + 12, pos.y - 6 }, 4.0f, c);
        }
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
    if (p->kind != PICKUP_ITEM && p->kind != PICKUP_EXIT) UiText(label, (int)pos.x - 6, (int)pos.y - 8, 14, BLACK);
    if (p->cost > 0) UiText(TextFormat("%dc", p->cost), (int)pos.x - 11, (int)pos.y + 24, 14, GOLD);
    else if (p->kind == PICKUP_ITEM) UiText(label, (int)pos.x - 55, (int)pos.y + 24, 12, RAYWHITE);
    /* WP7 (systems/special-rooms.md + DEC-058): LA PUNTATA SCRITTA PER ESTESO.
       Offerta e prezzo devono essere leggibili PRIMA di accettare -- e' l'unica
       stanza in cui una decisione irreversibile dipende da un contenuto
       composto al momento, quindi il testo non e' decorazione, e' il contratto.
       Si legge da Game.pourhouse e non da campi del Pickup apposta: dentro un
       Pickup andrebbe troncato, e un contratto troncato non e' un contratto.
       Sotto lo sprite, dove non copre il banco ne' il giocatore. */
    /* WP15a: l'invito esplicito. La piazzola richiede il tasto di interazione
       a contatto -- come l'arena del piano e il banco della Pourhouse -- e
       quel tasto va scritto, non indovinato. */
    if (p->kind == PICKUP_TRIAL_GATE)
        UiText("X per entrare", (int)pos.x - 40, (int)pos.y + 26, 12, RAYWHITE);
    if (p->kind == PICKUP_POURHOUSE_BANK)
    {
        char offerText[96], priceText[96];
        WorldPourhouseOfferText(&game->pourhouse, offerText, (int)sizeof(offerText));
        WorldPourhousePriceText(&game->pourhouse, priceText, (int)sizeof(priceText));
        if (game->pourhouse.valid && !game->pourhouse.accepted)
        {
            UiText(TextFormat("DAI: %s", priceText), (int)pos.x - 120, (int)pos.y + 26, 14, (Color){ 255, 176, 176, 255 });
            UiText(TextFormat("PRENDI: %s", offerText), (int)pos.x - 120, (int)pos.y + 42, 14, (Color){ 176, 255, 208, 255 });
            UiText("X per accettare, esci per rifiutare", (int)pos.x - 120, (int)pos.y + 58, 12, RAYWHITE);
        }
        else if (game->pourhouse.accepted)
        {
            UiText(TextFormat("VERSATO: %s", priceText), (int)pos.x - 120, (int)pos.y + 26, 14, (Color){ 190, 190, 198, 255 });
            UiText(TextFormat("RICEVUTO: %s", offerText), (int)pos.x - 120, (int)pos.y + 42, 14, (Color){ 190, 190, 198, 255 });
        }
        else
        {
            UiText("La colata e' fredda: non hai nulla da versare.", (int)pos.x - 120, (int)pos.y + 26, 14, (Color){ 200, 190, 190, 255 });
            UiText("Nessun prezzo, nessuna penalita': esci quando vuoi.", (int)pos.x - 120, (int)pos.y + 42, 12, RAYWHITE);
        }
    }
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
/* W8: la BASE del personaggio disegnata a mano (assets/art/character/), al
 * posto dello stickman.
 *
 * NON contraddice la decisione citata su DrawBaseStickman, la ATTUA: l'obiezione
 * del documento era contro una base GENERATA -- "se il personaggio base cambiasse
 * ad ogni run generata, DrawEquipment non saprebbe piu' dove mettere
 * cappello/occhiali con certezza". Uno sprite disegnato a mano e versionato nel
 * repo e' MENO variabile dello stickman procedurale, non piu': gli agganci di
 * PlayerComputeAnchors continuano a derivare da posizione e raggio, che non
 * cambiano, e i layer degli oggetti restano esattamente dove erano.
 *
 * Quattro camminate + idle si scelgono da Player.animFacing/walkTime (scritti
 * da GameUpdatePlayer); 'hit' scatta durante l'invulnerabilita' da danno, che e'
 * il segnale che il motore ha gia'. La morte non passa da qui: e' un ArtFx.
 * false = nessuno sprite (checkout senza assets/art/): si torna allo stickman. */
/* WP-INT (known-issues.md #10.2, CHIUSO per gli sheet): la scelta dello sheet
   segue l'indice del personaggio scelto, STESSO ordine di
   content/character_roster.c (0=Wayfinder/fonditrice, 1=Ashblade, 2=Bulwark).
   Il personaggio GENERATO per-run (characterIndex == CHARACTER_COUNT) e
   qualunque indice fuori dai tre curati (-1 = nessun personaggio applicato,
   fuori range = difesa in profondita') restano su fonditrice: DEFAULT
   PROPOSTO dall'implementazione (stile DEC-019, non canone -- registrato in
   docs/design/systems/characters.md, "Default proposti dall'implementazione",
   e in governance/open-questions.md). Resta aperta solo la TINTA del
   personaggio generato (vedi il commento sull'alfa sotto), non lo sheet. */
static const char *CharacterSheetKey(int characterIndex)
{
    switch (characterIndex)
    {
        case 0: return "character/fonditrice";
        case 1: return "character/ashblade";
        case 2: return "character/bulwark";
        default: return "character/fonditrice";
    }
}

static bool DrawCharacterSprite(const Player *p, Color palette, int characterIndex)
{
    const ArtSheet *sheet = ArtAtlasGet(CharacterSheetKey(characterIndex));
    if (!sheet) return false;
    /* Della tinta del personaggio si conserva SOLO l'alfa. Lo stickman era una
       silhouette monocroma e la palette del personaggio (M6a) era l'unico modo
       di dargli identita'; uno sprite disegnato ha la sua palette dentro, e
       moltiplicarla per un colore la sporcherebbe. L'alfa invece porta ancora
       informazione di gioco: e' il lampeggio di invulnerabilita', che deve
       restare visibile. */
    Color tint = (Color){ 255, 255, 255, palette.a };
    /* Lo stickman ha i piedi a +31 px dal centro (DrawBaseStickman) e il
       PLAYER_FOOT_Y di DrawGameplayCanvas ancora l'ombra la': lo sprite poggia
       sullo STESSO punto, o l'ombra resterebbe staccata dai piedi. */
    Vector2 ground = { p->pos.x, p->pos.y + 31.0f*(p->radius/14.0f) };
    float scale = ArtScaleForWidth(sheet->frameW, p->radius*4.6f);

    if (p->invuln > 0.0f && ArtDrawAnim(sheet, "hit", 0.0f, ground, scale, false, tint)) return true;

    /* Si traduce Direction in NOME di animazione con uno switch esplicito e non
       indicizzando una tabella con l'enum: l'ordine di Direction e' UP, RIGHT,
       DOWN, LEFT (core/game_types.h) e non quello alfabetico o cardinale che
       verrebbe naturale scrivere in una tabella -- un giorno che qualcuno
       aggiunga una direzione, uno switch e' un -Wswitch, una tabella e' una
       camminata che punta dalla parte sbagliata in silenzio. */
    const char *walk = "walk_down";
    switch ((Direction)p->animFacing)
    {
        case DIR_UP: walk = "walk_up"; break;
        case DIR_RIGHT: walk = "walk_right"; break;
        case DIR_DOWN: walk = "walk_down"; break;
        case DIR_LEFT: walk = "walk_left"; break;
    }
    if (p->walkTime > 0.0f && ArtDrawAnim(sheet, walk, p->walkTime, ground, scale, false, tint)) return true;
    if (ArtDrawAnim(sheet, "idle", (float)GetTime(), ground, scale, false, tint)) return true;
    return ArtDrawAnim(sheet, walk, 0.0f, ground, scale, false, tint);
}

static void DrawEquipment(const Player *p, Vector2 pos, Color tint, int characterIndex)
{
    PlayerAnchors anchors = PlayerComputeAnchors(pos, p->radius);
    ItemLayer layers[MAX_ITEMS];
    int count = BuildItemLayers(p->items, p->itemCount, layers, MAX_ITEMS);

    int i = 0;
    for (; i < count && ItemLayerIsBehindBase(layers[i].slot); i++) DrawItemLayer(anchors, layers[i]);
    if (!DrawCharacterSprite(p, tint, characterIndex)) DrawBaseStickman(pos, tint);
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
    DrawEquipment(p, p->pos, tint, game->characterChosenIndex);
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
/* ============================================================
   Le quote del layout HUD in pixel art (WP-UI-1, mock 02/08 approvato dal
   proprietario: vedi HANDOFF.md). Tutte in pixel di CANVAS (640x360,
   DEC-200), raccolte in UN blocco solo e non sparse nelle funzioni -- sono il
   contratto col mock, un giro artistico futuro deve poterle rileggere in un
   colpo d'occhio (15-UI-DESIGN-PIPELINE.md, "nessun numero magico sparso").
   Il blocco vive QUI, prima di DrawTransientMessage/DrawFloorZeroTrialHint
   (i due banner subito sotto), perche' quei due condividono l'angolo basso
   dell'HUD con la barra comandi e la card di scoperta (DrawHudV3Card, molto
   piu' sotto nel file, dove vive il resto dell'HUD): le loro quote
   (HUD_MSG_Y/HUD_TRIALHINT_Y) vanno impilate SOPRA HUD_V3_CARD_Y, e il
   preprocessore risolve le macro nell'ordine in cui compaiono nel file --
   da qui la necessita' di un solo blocco, presto, invece di uno vicino a
   ogni funzione. */
#define HUD_V3_MARGIN 4                 /* margine dal bordo canvas: pannelli e testo allineato al bordo */
#define HUD_V3_PANEL_X HUD_V3_MARGIN
#define HUD_V3_PANEL_Y HUD_V3_MARGIN
/* Larghezza fissa, come il resto delle quote del mock -- non si allarga con
   valori piu' lunghi. La riga risorse (DrawHudV3Resources) somma passi FISSI
   (RES_ICON_ADVANCE/RES_GROUP_GAP), non misura la larghezza vera dei tre
   numeri come fa invece DrawHudV3Stats per le sue etichette: con lingotti a
   QUATTRO cifre (>=1000, oggi non raggiungibile in una run normale ma non
   impedito dal tipo, int) la terza coppia quadratino+numero (chiavi)
   potrebbe uscire dal bordo destro del pannello. Limite noto, non corretto
   qui: servirebbe misurare la riga come in DrawHudV3Stats, un cambio piu'
   largo di questo giro. */
#define HUD_V3_PANEL_W 150
#define HUD_V3_PANEL_H 58
#define HUD_V3_PANEL_PAD 4
#define HUD_V3_CONTENT_X (HUD_V3_PANEL_X + HUD_V3_PANEL_PAD)
#define HUD_V3_NAME_Y (HUD_V3_PANEL_Y + HUD_V3_PANEL_PAD)
#define HUD_V3_HEARTS_Y (HUD_V3_NAME_Y + 8)
#define HUD_V3_HEART_PX 14
#define HUD_V3_HEART_STEP 15
/* Margine fra l'ultimo slot di cuore BASE e il primo cuore Crust (DEC-008/WP2),
   cosi' i due contatori non si toccano -- vedi HudTempHeartsX piu' sotto. */
#define HUD_V3_TEMP_HEARTS_GAP 6
#define HUD_V3_RES_Y (HUD_V3_HEARTS_Y + HUD_V3_HEART_PX + 3)
#define HUD_V3_RES_ICON_PX 8            /* lato del quadratino colore -- il mock sostituisce l'icona a sprite con una tinta piatta */
#define HUD_V3_RES_ICON_ADVANCE 10
#define HUD_V3_RES_GROUP_GAP 10
#define HUD_V3_FLUX_Y (HUD_V3_RES_Y + 13)

/* Blocco statistiche (toggle C, DEC-184): SOTTO il pannello vitali (chiuso a
   PANEL_Y+PANEL_H), non piu' sovrapposto -- il vecchio Y fisso (50) cadeva
   dentro un pannello che allora non esisteva ancora come massa unica. */
#define HUD_V3_STATS_X HUD_V3_MARGIN
#define HUD_V3_STATS_Y (HUD_V3_PANEL_Y + HUD_V3_PANEL_H + 4)
#define HUD_V3_STATS_ROW_H 7
#define HUD_V3_STATS_PAD 4
#define HUD_V3_STATS_GAP 4

#define HUD_V3_SLOT_BOX 26
#define HUD_V3_SLOT_STEP 34
/* Barra comandi basso-sinistra: altezza fissa, il mock la vuole ben piu' alta
   della singola riga TAGLIA_1 che contiene (margine generoso sopra e sotto).
   Le caselle attivo/Innesto condividono il suo bordo INFERIORE ma sono piu'
   alte (SLOT_BOX=26 > BAR_H=21): SLOTS_Y risale di conseguenza, non e' un
   numero indipendente da tarare a mano. */
#define HUD_V3_BAR_H 21
#define HUD_V3_BAR_Y (SCREEN_HEIGHT - HUD_V3_MARGIN - HUD_V3_BAR_H)
#define HUD_V3_SLOTS_Y (HUD_V3_BAR_Y + HUD_V3_BAR_H - HUD_V3_SLOT_BOX)

#define HUD_V3_MINIMAP_CELL 11
#define HUD_V3_MINIMAP_GAP 2
#define HUD_V3_MINIMAP_PAD 4
#define HUD_V3_MINIMAP_Y 25

#define HUD_V3_CARD_W 250
#define HUD_V3_CARD_H 48
/* La card di scoperta sta SOPRA la barra comandi con un margine, non piu'
   incollata al fondo del canvas: prima (Y=SCREEN_HEIGHT-56=304, alta 48, fino
   a 352) copriva in parte la barra comandi nuova (che comincia a
   HUD_V3_BAR_Y=335) -- il difetto noto ("la barra si sovrapponeva... alla
   card scoperta") che questo giro chiude. */
#define HUD_V3_CARD_Y (HUD_V3_BAR_Y - 6 - HUD_V3_CARD_H)

/* I due banner larghi di DrawGameplayCanvas qui sotto (trial hint e messaggio
   di raccolta) impilati SOPRA la card con lo stesso margine: stessa causa
   dello spostamento della card qui sopra (la barra comandi nuova occupava lo
   stesso angolo), la stessa correzione -- "ricolloca le etichette
   raccolta/card" del difetto noto. */
#define HUD_MSG_H 28
#define HUD_MSG_Y (HUD_V3_CARD_Y - 6 - HUD_MSG_H)
#define HUD_TRIALHINT_H 28
#define HUD_TRIALHINT_Y (HUD_MSG_Y - 6 - HUD_TRIALHINT_H)

static void DrawTransientMessage(Game *game)
{
    if (game->messageTimer <= 0.0f) return;
    Rectangle box = { 18.0f, (float)HUD_MSG_Y, (float)SCREEN_WIDTH - 36.0f, (float)HUD_MSG_H };
    UiPanel(box);
    UiTextAt(game->message, (int)box.x + 10, (int)box.y + (HUD_MSG_H - UiTextHeight(UI_TAGLIA_2))/2,
             UI_TAGLIA_2, UI_TESTO);
}

/* WP15a (DEC-047, systems/floor-zero.md "Primissima visita: tutorial
   integrato"): il CARTELLO della prima visita a una piazzola d'arena. A
   differenza del messaggio transitorio qui sopra non scade: resta per tutta la
   simulazione, perche' e' una spiegazione di comandi da consultare mentre si
   prova, non una notifica. Si disegna SOLO quando Game.floorZeroTrialHint non
   e' vuoto, cioe' solo alla primissima visita di QUELLA piazzola -- le visite
   successive restano mute, come chiede il documento. Sopra il messaggio
   transitorio e non al suo posto: i due possono convivere nello stesso
   istante. */
static void DrawFloorZeroTrialHint(const Game *game)
{
    if (!game->floorZeroTrialActive || !game->floorZeroTrialHint[0]) return;
    /* WP-UI-1: via il bordo arancio (la palette Fucina bandisce le cornici
       accese fuori da UiPanel, "niente... arancio-cornice") -- UI_GLINT sul
       testo ("punte di luce, evidenziazioni brevi") porta lo stesso segnale
       di attenzione senza un colore fuori tavolozza. */
    Rectangle box = { 18.0f, (float)HUD_TRIALHINT_Y, (float)SCREEN_WIDTH - 36.0f, (float)HUD_TRIALHINT_H };
    UiPanel(box);
    UiTextAt(game->floorZeroTrialHint, (int)box.x + 10, (int)box.y + (HUD_TRIALHINT_H - UiTextHeight(UI_TAGLIA_2))/2,
             UI_TAGLIA_2, UI_GLINT);
}

/* Un colpo, disegnato secondo la sua FORMA (step C, core/shot_type.h). Le forme
   non sono cinque colori diversi: sono cinque disegni diversi, perche' il punto
   del feedback che ha aperto questa fase era "un tipo di colpo nuovo deve avere
   un ASPETTO diverso e un COMPORTAMENTO diverso". Il comportamento vive in
   combat.c (pallettoni, perforazione, catena, moltiplicatori); l'aspetto e' qui.
   SHOT_FORM_ORB e' lo zero-default: ogni colpo nemico, ogni colpo generato da uno
   script Lua e ogni colpo di una run senza tipi di colpo passa di qui e viene
   disegnato ESATTAMENTE come prima di questa fase (due cerchi). */
/* W8: lo spritesheet di un colpo, scelto dalla sua FORMA. La corrispondenza
   forma->file e' 1:1 col vocabolario di core/shot_type.h e coi file consegnati
   (assets/art/shots/{orb,spike,beam,arc,blade}); orb ha anche un tier "grande"
   (orb-grande) che si usa per i colpi di raggio alto, esattamente come il
   contratto della pipeline prevede ("il motore sceglie il tier piu' vicino").
   NULL = nessun originale: si ricade sui cinque disegni geometrici di sempre. */
static const ArtSheet *ShotSheet(const Shot *shot)
{
    const char *key = NULL;
    switch (shot->form)
    {
        case SHOT_FORM_SPIKE: key = "shots/spike"; break;
        case SHOT_FORM_BEAM: key = "shots/beam"; break;
        case SHOT_FORM_ARC: key = "shots/arc"; break;
        case SHOT_FORM_BLADE: key = "shots/blade"; break;
        case SHOT_FORM_ORB:
        case SHOT_FORM_COUNT:
        default: key = (shot->radius >= 11.0f) ? "shots/orb-grande" : "shots/orb"; break;
    }
    const ArtSheet *sheet = ArtAtlasGet(key);
    if (!sheet && shot->form == SHOT_FORM_ORB) sheet = ArtAtlasGet("shots/orb");
    return sheet;
}

/* Il colpo con lo sprite vero. Lo sprite NON ruota con la direzione: i colpi
   consegnati sono simmetrici o radiali, e ruotare pixel art di un angolo
   arbitrario la sfoca -- e' la stessa ragione per cui la scala si aggancia a
   mezzi passi. Il colore del colpo (che porta informazione: e' il colore
   dell'oggetto che ha dato il tipo) si applica come TINTA moltiplicativa, cosi'
   due build diverse hanno colpi distinguibili pur condividendo il disegno. */
static bool DrawShotSprite(const Shot *shot)
{
    const ArtSheet *sheet = ShotSheet(shot);
    if (!sheet) return false;
    float scale = ArtScaleForWidth(sheet->frameW, shot->radius*3.2f);
    /* L'ancora dei colpi e' il CENTRO del fotogramma (8,8 su 16): il punto di
       appoggio e' quindi la posizione del colpo, senza correzione di piede. */
    return ArtDrawAnim(sheet, "fly", (float)GetTime()*1.0f + shot->pos.x*0.01f,
                       shot->pos, scale, false, shot->color);
}

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

    /* W8: lo sprite vince sulle cinque forme geometriche. L'anello di sinergia
       qui sopra resta comunque disegnato PRIMA in entrambi i casi: e'
       informazione di gioco (step D), non decorazione della forma. */
    if (DrawShotSprite(shot)) return;

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

/* WP3 (secrets-and-obstacles.md, "Feedback" + DEC-058 "mai un'informazione
   affidata al solo colore"): segnale leggibile per le due famiglie che non
   sono il solido di sempre -- varianti di DISEGNO sopra il tile/blocco
   esistente, NESSUN asset nuovo (stessa grammatica visiva delle strisce del
   cancello del Piano 0 non ancora aperto, DrawFloorZeroExitGate sopra: una
   FORMA a bande per il pericolo, non solo un colore, leggibile anche in
   scala di grigi; una crepa a X per il distruttibile). 'r' e' il rettangolo
   VISIVO gia' trasformato dal chiamante (nel fallback senza tileset la
   faccia superiore e' sollevata di LIFT, nel tileset no): questa funzione
   non sa nulla di quella differenza, disegna solo dentro 'r'.
   WP-SPIKE (DEC-198): 'hazardExtended' arriva GIA' RISOLTO dal chiamante
   tramite WorldHazardSpikesExtendedAt (src/world/world.c) -- questa funzione
   non ricalcola mai la fase, disegna solo la veste che quella fase implica.
   Ignorato per OBSTACLE_DESTRUCTIBLE (il default del parametro per quella
   famiglia e' 'true', vedi i chiamanti). Fase ESTESA: le bande diagonali di
   sempre (il pericolo sta colpendo ORA). Fase RETRATTA: telegraph DEC-058
   "la piastra resta visibile e leggibile come pericolo" -- niente bande
   piene (prometterebbero un contatto che oggi non c'e'), solo una fila di
   punte accennate e piatte alla base del riquadro, stesso spaziamento delle
   bande cosi' la grammatica visiva resta la stessa fra le due fasi, cambia
   solo l'intensita'. */
static void DrawObstacleFamilyOverlay(ObstacleFamily family, Rectangle r, bool hazardExtended)
{
    if (family == OBSTACLE_DESTRUCTIBLE)
    {
        Color crack = (Color){ 20, 16, 12, 210 };
        float pad = fminf(r.width, r.height)*0.18f + 3.0f;
        DrawLineEx((Vector2){ r.x + pad, r.y + pad }, (Vector2){ r.x + r.width - pad, r.y + r.height - pad }, 3.0f, crack);
        DrawLineEx((Vector2){ r.x + r.width - pad, r.y + pad }, (Vector2){ r.x + pad, r.y + r.height - pad }, 3.0f, crack);
    }
    else if (family == OBSTACLE_HAZARD)
    {
        Color hazard = (Color){ 224, 168, 42, 255 };
        if (hazardExtended)
        {
            for (float sx = r.x - r.height; sx < r.x + r.width; sx += 14.0f)
            {
                Vector2 p1 = { sx, r.y + r.height };
                Vector2 p2 = { sx + r.height, r.y };
                /* Clamp orizzontale, stesso spirito di DrawFloorZeroExitGate: le
                   strisce nascono/muoiono fuori da 'r' per coprirlo fino ai
                   bordi, ma disegnate intere sforerebbero fuori dal blocco. */
                if (p1.x < r.x) p1.x = r.x;
                if (p2.x > r.x + r.width) p2.x = r.x + r.width;
                DrawLineEx(p1, p2, 3.0f, hazard);
            }
        }
        else
        {
            for (float sx = r.x; sx < r.x + r.width; sx += 14.0f)
            {
                float tipX = sx + 5.0f;
                if (tipX > r.x + r.width) tipX = r.x + r.width;
                DrawLineEx((Vector2){ sx, r.y + r.height }, (Vector2){ tipX, r.y + r.height - 4.0f }, 2.0f, hazard);
            }
        }
        DrawRectangleLinesEx(r, 2.0f, hazard);
    }
}

/* WP-INT (correzione: la prima versione disegnava UN sprite unico ancorato al
   centro-base di 'r' e scalava SOLO dalla larghezza -- corretto per i blocchi
   quasi quadrati di ROOM_LAYOUT_PILLARS, ma per CORRIDOR/ARENA i blocchi di
   RoomLayoutBuild sono molto piu' larghi che alti (es. 308x79: ArtScaleForWidth
   sceglie 9.5x su un fotogramma 32x32 quadrato, cioe' 304x304 disegnati su un
   rettangolo alto 79 -- 196 px fuori sopra, sotto la zona che fa davvero danno/
   blocca, la promessa esattamente opposta al telegraph "leggibile prima di
   ogni contatto" di secrets-and-obstacles.md/DEC-058). Qui il prop non si
   ancora piu' come uno sprite a se': RIEMPIE 'r' ripetendo il fotogramma,
   stessa disciplina di DrawTiledArea sopra (che e' cio' che sostituisce): la
   scala si sceglie dall'ALTEZZA del blocco (i layout di RoomLayoutBuild sono
   sempre piu' bassi che larghi, mai il contrario, vedi ROOM_CROSS_HALF), poi
   ogni cella viene ritagliata a filo di 'r' come i tile di pavimento/muro --
   quindi l'ingombro disegnato non supera MAI 'r', su nessuna delle quattro
   forme, indipendentemente dall'arrotondamento della scala. */
static void DrawArtSheetFrameTiled(const ArtSheet *sheet, int row, int frame, Rectangle r, Color tint)
{
    Rectangle src = ArtSheetFrameRect(sheet, row, frame);
    if (src.width <= 0.0f || src.height <= 0.0f) return;
    float scale = ArtScaleForWidth(sheet->frameH, r.height);
    float tw = src.width*scale, th = src.height*scale;
    if (tw <= 0.0f || th <= 0.0f) return;
    for (float y = r.y; y < r.y + r.height; y += th)
    {
        float cellH = fminf(th, r.y + r.height - y);
        for (float x = r.x; x < r.x + r.width; x += tw)
        {
            float cellW = fminf(tw, r.x + r.width - x);
            Rectangle s = { src.x, src.y, src.width*(cellW/tw), src.height*(cellH/th) };
            Rectangle dst = { x, y, cellW, cellH };
            DrawTexturePro(sheet->texture, s, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
        }
    }
}

/* WP-INT: la veste dedicata delle due famiglie non-solide, quando il pacchetto
   artistico la offre -- indipendente dal tileset del piano (le due famiglie
   sono un elemento di GAMEPLAY, non di ambientazione, quindi lo stesso prop
   compare su qualunque tema). 'r' e' il rettangolo VISIVO gia' scelto dal
   chiamante (con o senza LIFT, vedi il commento su DrawObstacleFamilyOverlay
   sotto): il prop RIEMPIE 'r' (DrawArtSheetFrameTiled sopra), non si ancora
   piu' come un prop isolato -- coerente col fatto che sostituisce l'intera
   area del blocco (DrawTiledArea/le facce 2.5D), non solo un dettaglio sopra.
   false quando l'asset manca (checkout senza assets/art/, o rigenerazione
   parziale del pacchetto): chi chiama ricade sul tile/blocco 2.5D di sempre,
   MAI un buco -- stesso contratto di ogni altro propKey in questo file.
   WP-SPIKE (DEC-198): 'hazardExtended' arriva GIA' RISOLTO dal chiamante
   tramite WorldHazardSpikesExtendedAt -- la STESSA funzione che
   CombatResolveHazards (src/gameplay/combat.c) usa per decidere il danno.
   Sceglie solo il TAG dell'asset (props/spuntoni ha gia' "estesi", 2
   fotogrammi, ed "retratti", 1 fotogramma, consegnati): quando la fase e'
   retratta si vede "retratti" ed e' garantito -- per costruzione, non per
   promessa separata -- che il contatto in quello stesso istante non
   danneggia. Ignorato per le altre famiglie. */
static bool DrawObstacleFamilyProp(ObstacleFamily family, Rectangle r, bool hazardExtended)
{
    const char *key = NULL;
    const char *animName = "idle";
    if (family == OBSTACLE_HAZARD)
    {
        key = "props/spuntoni";
        animName = hazardExtended ? "estesi" : "retratti";
    }
    else if (family == OBSTACLE_DESTRUCTIBLE)
    {
        /* Default proposto dall'implementazione: "cassa" invece di "vaso" come
           veste standard -- un contenitore di legno si legge come
           "distruttibile" in qualunque ambientazione del gioco (industriale,
           naturale, anomala) senza bisogno del tema, mentre un vaso presuppone
           un arredo domestico/decorativo che non tutti i temi condividono.
           Solo "idle": lo stato distrutto non si disegna affatto (l'ostacolo
           sparisce, comportamento invariato di WP3), quindi "break" del prop
           non serve qui. */
        key = "props/cassa";
        animName = "idle";
    }
    else return false;

    const ArtSheet *prop = ArtAtlasGet(key);
    if (!prop) return false;
    const ArtAnim *anim = ArtSheetAnim(prop, animName);
    if (!anim) return false;
    int frame = ArtAnimFrameAt(anim, (float)GetTime() + r.x*0.01f);
    DrawArtSheetFrameTiled(prop, anim->row, frame, r, WHITE);
    return true;
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
    /* W8: gli ostacoli vestiti dal tileset. Il ruolo si sceglie dalla FAMIGLIA
       di layout del piano (obst_pillar/corridor/arena/scatter, uno per valore di
       RoomForm): il contratto d'arte emette esattamente un tile per famiglia,
       cosi' una stanza a colonne si vede come colonne e una a strozzature come
       strozzature, non come lo stesso blocco quattro volte. */
    const ArtSheet *tiles = RoomTileset(&game->theme);
    if (tiles)
    {
        int floorIndex = GameMathClampInt(game->floor - 1, 0, FLOOR_COUNT - 1);
        const RoomLayoutDef *layout = &game->content.floors[floorIndex].roomLayout;
        const char *role = "obst_pillar";
        switch (layout->form)
        {
            case ROOM_LAYOUT_CORRIDOR: role = "obst_corridor"; break;
            case ROOM_LAYOUT_ARENA: role = "obst_arena"; break;
            case ROOM_LAYOUT_SCATTER: role = "obst_scatter"; break;
            case ROOM_LAYOUT_PILLARS:
            case ROOM_LAYOUT_OPEN:
            case ROOM_LAYOUT_COUNT:
            default: role = "obst_pillar"; break;
        }
        for (int i = game->obstacleHoleCount; i < game->obstacleCount; i++)
        {
            Obstacle *o = &game->obstacles[i];
            /* L'ombra a terra resta: e' cio' che fa poggiare il blocco sul
               pavimento, e nessun tile puo' disegnarla (non sa cosa ha sotto). */
            DrawEllipse((int)(o->x + o->w*0.5f), (int)(o->y + o->h + 4.0f), o->w*0.55f, o->h*0.22f, (Color){ 0, 0, 0, 90 });
            Rectangle rr = { o->x, o->y, o->w, o->h };
            /* WP-SPIKE (DEC-198): STESSO predicato di CombatResolveHazards,
               risolto una volta per ostacolo e passato a entrambe le funzioni
               di disegno sotto -- mai due calcoli paralleli. Ignorato dalle
               altre famiglie (il default 'true' e' innocuo per loro). */
            bool hazardExtended = (o->family != OBSTACLE_HAZARD) ||
                WorldHazardSpikesExtendedAt(game->obstacleCellX[i], game->obstacleCellY[i], game->obstacleLocalIndex[i], game->runElapsedSeconds);
            if (!DrawObstacleFamilyProp(o->family, rr, hazardExtended)) DrawTiledArea(game, tiles, rr, role, NULL, NULL, WHITE);
            DrawObstacleFamilyOverlay(o->family, rr, hazardExtended);
        }
        return;
    }

    const float LIFT = 16.0f;   /* quanto e' "alto" il blocco: la faccia superiore e' spostata su di tanto */
    Color side = GameColorLerp(game->theme.wall, BLACK, 0.55f);
    Color top = GameColorLerp(game->theme.wall, WHITE, 0.12f);
    Color edge = GameColorLerp(game->theme.wall, BLACK, 0.30f);
    /* DEC-170: i primi obstacleHoleCount ostacoli sono le celle-buco della
       stanza (l'angolo mancante di una forma a L). Solide come le altre, ma
       gia' disegnate da DrawRoom come parete vera: qui si saltano, o si
       vedrebbe un blocco decorativo grande quanto una schermata. */
    for (int i = game->obstacleHoleCount; i < game->obstacleCount; i++)
    {
        Obstacle *o = &game->obstacles[i];
        /* Ombra a terra alla base del blocco. */
        DrawEllipse((int)(o->x + o->w*0.5f), (int)(o->y + o->h + 4.0f), o->w*0.55f, o->h*0.22f, (Color){ 0, 0, 0, 90 });
        Rectangle topRect = { o->x, o->y - LIFT, o->w, o->h };
        /* WP-SPIKE (DEC-198): stesso predicato del ramo col tileset sopra e di
           CombatResolveHazards -- vedi il commento li'. */
        bool hazardExtended = (o->family != OBSTACLE_HAZARD) ||
            WorldHazardSpikesExtendedAt(game->obstacleCellX[i], game->obstacleCellY[i], game->obstacleLocalIndex[i], game->runElapsedSeconds);
        if (!DrawObstacleFamilyProp(o->family, topRect, hazardExtended))
        {
            /* Faccia frontale (lo spessore): dalla base del blocco giu' di LIFT. */
            DrawRectangle((int)o->x, (int)(o->y + o->h - LIFT), (int)o->w, (int)LIFT, side);
            /* Faccia superiore: il rettangolo del blocco, spostato SU di LIFT. */
            DrawRectangle((int)o->x, (int)(o->y - LIFT), (int)o->w, (int)o->h, top);
            DrawRectangleLinesEx(topRect, 2.0f, edge);
        }
        DrawObstacleFamilyOverlay(o->family, topRect, hazardExtended);
    }
}

static void DrawGameplayCanvas(Game *game)
{
    /* Lo sfondo e' l'unica cosa in coordinate di CANVAS qui dentro: tutto il
       resto della scena vive in coordinate di MONDO, dentro la telecamera
       (DEC-170). Per una stanza 1x1 la telecamera e' l'identita' -- ogni pixel
       di questa funzione cade dove cadeva prima. */
    ClearBackground(game->theme.bg);
    BeginMode2D(WorldGameCamera(game));
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
    /* W8: le animazioni di morte (ArtFx). Dopo le entita' e prima delle
       particelle: un nemico che muore e' ormai scenografia, e le sue particelle
       (che esplodono nello stesso istante) devono restare sopra di lui. */
    for (int i = 0; i < MAX_ART_FX; i++)
    {
        const ArtFx *fx = &game->artFx[i];
        if (!fx->active) continue;
        const ArtSheet *sheet = ArtAtlasFindByImageId(fx->imageId);
        if (!sheet) continue;
        /* Il punto di appoggio e' la stessa frazione di raggio usata per il
           nemico vivo, ricostruita dalla larghezza voluta: senza, lo sprite
           della morte "salterebbe" verso l'alto nell'istante del passaggio. */
        Vector2 ground = { fx->pos.x, fx->pos.y + fx->wantedWidth*(0.62f/3.3f) };
        float scale = ArtScaleForWidth(sheet->frameW, fx->wantedWidth);
        ArtDrawAnim(sheet, fx->anim, fx->elapsed, ground, scale, fx->flipX, fx->tint);
    }
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        DrawCircleV(p->pos, p->radius, GameColorWithAlpha(p->color, (unsigned char)GameMathClampFloat(p->life*420.0f, 0.0f, 255.0f)));
    }
    EndMode2D();   /* da qui in giu' si torna in coordinate di canvas */

    DrawVignette();
    DrawFloorZeroTrialHint(game);
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
    /* W8: due colonne allineate a mano invece di due GuiLabel -- raygui
       resterebbe l'ultimo punto con una tipografia propria dentro una
       schermata vestita coi componenti. Le due quote (0 e 118) sono quelle di
       prima, moltiplicate per la scala come faceva il rettangolo di GuiLabel. */
    int font = UiRound(14.0f*uiScale);
    UiText(label, x, y, font, (Color){ 150, 158, 172, 255 });
    UiText(value, x + UiRound(118.0f*uiScale), y, font, color);
}

/* DEC-184 (ui/hud.md, "Blocco statistiche"): le SEI righe -- danno, cadenza,
   velocita' del colpo, velocita' di movimento, raggio, Fortuna -- nell'ORDINE
   fissato dal documento. UN SOLO punto che legge Player per queste sei righe:
   sia il pannello "PERSONAGGIO" di BuildScreen (DrawBuildScreenOverlay) sia il
   blocco compatto dell'HUD V3 (DrawHudV3Stats) chiamano SOLO questa funzione,
   mai un TextFormat locale proprio -- "stessa fonte dati, non duplicare i
   calcoli" del requisito, applicato alla lettera: non e' solo lo stesso
   campo Player, e' la stessa formattazione, in un punto solo. */
static void HudStatRowsFill(const Player *p, const char *labels[6], char values[6][16])
{
    labels[0] = "Danno";        snprintf(values[0], 16, "%.1f", p->damage);
    labels[1] = "Cadenza";      snprintf(values[1], 16, "%.2fs", p->fireDelay);
    labels[2] = "Vel. colpo";   snprintf(values[2], 16, "%.0f", p->shotSpeed);
    labels[3] = "Vel. mov.";    snprintf(values[3], 16, "%.0f", p->speed);
    labels[4] = "Raggio";       snprintf(values[4], 16, "%.1f", p->shotRadius);
    labels[5] = "Fortuna";      snprintf(values[5], 16, "%+.1f", p->luck);
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
static void DrawRoomIcon(RoomKind kind, Rectangle cell, Color color)
{
    const char *g = NULL;
    if (kind == ROOM_TREASURE) g = "T";
    else if (kind == ROOM_SHOP) g = "$";
    else if (kind == ROOM_BOSS) g = "B";
    else if (kind == ROOM_FUSION) g = "F";   /* WP4: distingue la stanza senza colore, DEC-058 */
    else if (kind == ROOM_TIMED) g = "!";    /* WP5: idem, stanza a tempo (DEC-051) */
    else if (kind == ROOM_ARENA) g = "A";    /* WP6: idem, arena di sfida (special-rooms.md) */
    else if (kind == ROOM_POURHOUSE) g = "P";   /* WP7: idem, Pourhouse (DEC-136) */
    else if (kind == ROOM_SECRET) g = "S";      /* WP8: idem, stanza segreta gia' aperta (DEC-025) */
    if (!g) return;
    if (ArtUiReady())
    {
        /* Minimappa in pixel art (mock 02/08): TAGLIA_1 fissa, non piu'
           'uiScale*14' -- la cella e' 11px (spec del mock), un glifo da 10px
           (TAGLIA_2) non ci starebbe piu' dentro col margine di prima.
           ArtUiReady (font E panel E slot), non ArtUiFont: con un pacchetto
           parziale l'HUD sceglie il ripiego a celle grandi (~34px) e la
           lettera deve seguire quel ramo proporzionale, non restare a 5px. */
        int w = UiTextWidth(g, UI_TAGLIA_1);
        int h = UiTextHeight(UI_TAGLIA_1);
        UiTextAt(g, (int)(cell.x + cell.width*0.5f - (float)w*0.5f), (int)(cell.y + cell.height*0.5f - (float)h*0.5f),
                 UI_TAGLIA_1, color);
        return;
    }
    /* Ripiego SENZA pacchetto artistico (DrawHudRunStatus: celle ben piu'
       grandi della minimappa in pixel art, fino a ~34px alle risoluzioni
       piu' larghe): una TAGLIA_1 fissa ci lascerebbe la lettera a 5-8px persa
       dentro la cella (correzione da review del giudice). Proporzionale alla
       cella VERA che questa chiamata sta decorando -- niente parametro
       uiScale in piu' da tenere sincronizzato con 'cell', la cella stessa
       e' gia' la fonte di verita' sulla scala. */
    int fontSize = UiRound(cell.height*0.9f);
    if (fontSize < 8) fontSize = 8;
    int w = UiTextW(g, fontSize);
    UiText(g, (int)(cell.x + cell.width*0.5f - (float)w*0.5f), (int)(cell.y + cell.height*0.5f - (float)fontSize*0.5f),
           fontSize, color);
}

/* DEC-137: la minimappa e' passata da pannello laterale a overlay in un angolo
   (DrawHudRunStatus). Disegna solo la griglia GRID_SIZE x GRID_SIZE all'angolo
   (baseX, baseY), con cella/gap scelti dal chiamante: piu' piccola di prima
   (era 26px), perche' galleggia sul gioco e non deve rubargli l'angolo. La
   legenda testuale di prima ("T tesoro / $ negozio / B boss") e' caduta: sopra
   il gioco sarebbe una riga di testo larga e invadente, e le lettere sulle
   stanze GIA' visitate (DrawRoomIcon) dicono la stessa cosa dove serve. Larghezza
   e altezza della griglia le ricava il chiamante: GRID_SIZE*cell + (GRID_SIZE-1)*gap.
   DEC-170: la taglia non e' piu' un indizio (il vecchio riquadro rimpicciolito
   dentro lo slot) ma una FORMA -- le celle di una stessa stanza si fondono,
   perche' il bordo si disegna solo dove finisce davvero la stanza e il gap si
   riempie fra due celle sorelle. Una 2x2 si legge come un blocco unico, come in
   Isaac. */
/* Fucina (WP-UI-1, mock 02/08): tre soli stati di riempimento -- corrente
   (oro, UI_TITOLO), boss (brace-scura), tutto il resto (ardesia-scura) -- al
   posto della vecchia tavolozza per-archetipo (RoomMapColor, rimossa: vedi
   docs/engineering/known-issues.md #12), che nel dettaglio comprendeva un
   ciano (ROOM_TIMED) e un magenta (ROOM_POURHOUSE), entrambi banditi dalla
   palette dell'interfaccia (ui_theme.h). Le stanze speciali restano
   leggibili dalla LETTERA di DrawRoomIcon dopo la visita, esattamente come
   DEC-137 gia' notava ("le lettere... dicono la stessa cosa"): il colore
   extra era un secondo canale ridondante, non l'unica fonte di
   quell'informazione -- e la sua rimozione chiude anche known-issues.md #12
   (nessun colore-solo pre-visita da cui DEC-058 debba piu' proteggere). */
static const Color HUD_MAP_NORMAL = { 38, 48, 63, 255 };   /* ardesia-scura */
/* Lo stesso "sotto" del gradiente del cuore (assets/art/ui/icons.png, riga
   'heart': outline 126,34,22 + fill 177,58,30) gia' sulla palette Fucina --
   qui diventa lo stato "stanza del boss". Costante locale e non un token
   ui_theme.h perche' usata in un solo punto (stessa scelta per
   HUD_RES_KEY_BRONZE piu' sotto). */
static const Color HUD_MAP_BOSS = { 126, 34, 22, 255 };    /* brace-scura */

static void DrawMinimap(Game *game, int baseX, int baseY, int size, int gap)
{
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!game->rooms[y][x].exists) continue;   /* niente cella sulle celle inesistenti: la mappa "respira" */
            const RoomState *room = WorldRoomAt(game, x, y);
            /* WP8 (systems/special-rooms.md, "Stanza segreta"): una segreta col
               varco ancora murato NON compare sulla mappa -- nemmeno smorzata
               come una stanza nota-ma-non-visitata, che sarebbe gia' un
               indizio (e l'indizio, per DEC-025, sta sulla PARETE dentro la
               stanza vicina, non qui). Dopo la breccia il predicato torna
               falso e la stanza si disegna come ogni altra. Il gap si riempie
               da solo: le celle sorelle non esistono (la segreta e' sempre
               1x1) e la cella accanto non e' della stessa stanza, quindi
               nessun bordo/gap la tradisce. */
            if (WorldRoomHiddenOnMap(room)) continue;
            Rectangle cell = { (float)(baseX + x*(size + gap)), (float)(baseY + y*(size + gap)), (float)size, (float)size };
            /* "Stanza corrente" = tutte le celle della stanza in cui si trova
               il giocatore, non solo quella di stato (game->roomX/roomY). */
            bool current = WorldSameRoom(game, x, y, game->roomX, game->roomY);
            /* Correzione dal giudizio del verifier (DEC-058/known-issues.md
               #12): il boss si tinge di HUD_MAP_BOSS SOLO se la stanza e'
               visited -- altrimenti (nota-ma-non-visitata) resta
               HUD_MAP_NORMAL come ogni altro archetipo, e lo smorzamento qui
               sotto la rende IDENTICA a una stanza qualunque non visitata.
               Prima 'solid' sceglieva il boss a prescindere da 'visited' e lo
               smorzamento si limitava ad attenuarlo: una stanza boss nota ma
               mai visitata restava riconoscibile dal SOLO colore (una brace
               smorzata, distinta dall'ardesia smorzata), esattamente il
               canale-solo-colore che DEC-058 vieta -- la lettera "B" di
               DrawRoomIcon compare solo dopo la visita, quindi prima non
               c'era nessun secondo canale a compensare. */
            Color solid = current ? UI_TITOLO : ((room->kind == ROOM_BOSS && room->visited) ? HUD_MAP_BOSS : HUD_MAP_NORMAL);
            /* Visitata: colore pieno. Non visitata ma esistente (adiacente a
               una visitata): smorzata, cosi' si vede DOVE si puo' andare
               senza svelare cosa c'e' (la stanza corrente e' per definizione
               visitata, lo smorzamento qui non la riguarda mai). */
            Color base = room->visited ? solid : GameColorLerp(solid, (Color){ 30, 33, 40, 255 }, 0.7f);
            DrawRectangleRec(cell, base);
            /* Il gap verso una cella SORELLA si riempie: e' cio' che fonde le
               celle in un blocco unico invece di lasciarle come stanze vicine. */
            if (gap > 0 && WorldSameRoom(game, x, y, x + 1, y))
                DrawRectangle((int)(cell.x + cell.width), (int)cell.y, gap, size, base);
            if (gap > 0 && WorldSameRoom(game, x, y, x, y + 1))
                DrawRectangle((int)cell.x, (int)(cell.y + cell.height), size, gap, base);

            /* Porte come TACCHE (mock 02/08): un segno solo dove c'e' un
               varco vero fra due stanze DIVERSE -- non piu' un bordo pieno su
               ogni lato esterno della stanza, che ridondava con la fusione
               delle celle sorelle qui sopra e nel mock non c'e'. Un solo
               verso per lato condiviso (RIGHT/DOWN): il verso opposto (LEFT
               della cella a destra, UP di quella sotto) e' lo STESSO segmento
               fisico -- ridisegnarlo da entrambi i lati sarebbe lo stesso
               pixel due volte.
               LETTO DA 'game->rooms[y][x].doors[]', NON da 'room->doors[]'
               (correzione dal giudizio del verifier): 'room' e' la cella di
               STATO (WorldRoomAt), la stessa per OGNI cella della stanza --
               'doors[]' invece e' un fatto del LATO di QUESTA cella
               (game_types.h, commento su RoomState), valido su ognuna
               separatamente. Su una stanza 2x2 leggerlo da 'room' avrebbe
               fatto sparire le tacche sui lati destro/inferiore delle celle
               sorelle (i loro doors[] veri non sono quelli della cella di
               stato) e ne avrebbe disegnate di fantasma dove la cella di
               stato aveva una porta ma QUESTA cella no. */
            if (gap > 0 && game->rooms[y][x].doors[DIR_RIGHT] && !WorldSameRoom(game, x, y, x + 1, y)
                && x + 1 < GRID_SIZE && game->rooms[y][x + 1].exists)
                DrawRectangleRec((Rectangle){ cell.x + cell.width, cell.y + cell.height*0.3f, (float)gap, cell.height*0.4f }, UI_SECONDARIO);
            if (gap > 0 && game->rooms[y][x].doors[DIR_DOWN] && !WorldSameRoom(game, x, y, x, y + 1)
                && y + 1 < GRID_SIZE && game->rooms[y + 1][x].exists)
                DrawRectangleRec((Rectangle){ cell.x + cell.width*0.3f, cell.y + cell.height, cell.width*0.4f, (float)gap }, UI_SECONDARIO);

            /* Le icone delle stanze speciali si vedono solo dopo averle visitate:
               un pizzico di scoperta, come in Isaac. UNA per stanza, sulla sua
               cella di stato, non una per cella. Lettera scura sul fondo oro
               (corrente, chiaro) e chiara altrimenti (ardesia/brace, scuri):
               un nero fisso come prima leggeva bene solo sui colori chiari
               della vecchia tavolozza, non sull'ardesia scura di adesso. */
            if (room->visited && room == &game->rooms[y][x])
                DrawRoomIcon(room->kind, cell, current ? UI_GROUND : UI_TESTO);
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
    bool hover = CheckCollisionPointRec(UiCanvasMouse(), row);
    DrawRectangleRec(row, hover ? (Color){ 40, 45, 56, 235 } : (owned ? (Color){ 28, 32, 40, 220 } : (Color){ 24, 27, 34, 210 }));
    DrawRectangleLinesEx(row, 2.0f, GameColorWithAlpha(rarityColor, hover ? 255 : 200));
    if (!DrawItemIcon(game, item, (Vector2){ x + 28.0f*uiScale, y + 29.0f*uiScale }, 36.0f*uiScale))
    {
        DrawItemShape((Vector2){ x + 28.0f*uiScale, y + 29.0f*uiScale }, *item, 12.0f*uiScale);
    }
    /* Se l'oggetto cambia il modo di sparare (step C), un puntino del colore del
       colpo in coda al nome: si vede a colpo d'occhio quali oggetti danno un tipo
       di colpo. */
    if (item->shotType.active) DrawCircleV((Vector2){ x + 47.0f*uiScale, y + 15.0f*uiScale }, 4.0f*uiScale, item->color);
    UiText(item->name, x + UiRound(55.0f*uiScale), y + UiRound(9.0f*uiScale), fontName, RAYWHITE);
    /* Nome della rarita' in coda alla riga slot/traits, nel suo colore (design
       doc, sezione 6: "...e col nome nel pannello"): due DrawText invece di
       uno solo cosi' SOLO il nome della rarita' prende il suo colore, il
       resto della riga resta nel grigio neutro gia' in uso. */
    char slotTraits[200];
    snprintf(slotTraits, sizeof(slotTraits), "%s  |  %s  |  ", SlotName(item->slot), traits);
    int textX = x + UiRound(55.0f*uiScale);
    int textY = y + UiRound(31.0f*uiScale);
    UiText(slotTraits, textX, textY, fontSlot, (Color){ 190, 198, 211, 255 });
    int rarityTextX = textX + UiTextW(slotTraits, fontSlot);
    UiText(RarityName(item->rarity), rarityTextX, textY, fontSlot, rarityColor);
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

    char lines[5][160];
    int n = 0;
    /* La CATEGORIA in testa: con quattro categorie e due di esse legate a uno
       slot esclusivo (attivo, Innesto), sapere "che tipo di oggetto e'
       questo" viene prima di slot visivo e rarita'. */
    snprintf(lines[n++], sizeof(lines[0]), "%s  -  %s  -  %s", ItemKindLabel(item->kind), SlotName(item->slot), RarityName(item->rarity));
    snprintf(lines[n++], sizeof(lines[0]), "Effetti: %s", traits);
    if (item->shotType.active)
        snprintf(lines[n++], sizeof(lines[0]), "Spari: %s (%s)", item->shotType.name, ShotFormName(item->shotType.form));
    if (item->kind == ITEM_STATUP)
        snprintf(lines[n++], sizeof(lines[0]), "Ricompensa del boss: potenzia una statistica");
    else if (item->kind == ITEM_ACTIVE)
    {
        if (ItemActiveIsChargeBased(item))
            snprintf(lines[n++], sizeof(lines[0]), "Attivo [E]: %d/%d cariche", item->chargeNow, ItemActiveChargeCapacity(item));
        else
            snprintf(lines[n++], sizeof(lines[0]), "Attivo [E]: ricarica %.0fs", (double)ItemActiveCooldownSeconds(item));
    }
    else if (item->kind == ITEM_GRAFT)
        snprintf(lines[n++], sizeof(lines[0]), "Innesto: [G] per sganciarlo a terra");

    int titleFont = UiRound(16.0f*uiScale);
    int lineFont = UiRound(13.0f*uiScale);
    int w = UiTextW(item->name, titleFont);
    for (int i = 0; i < n; i++) { int lw = UiTextW(lines[i], lineFont); if (lw > w) w = lw; }
    w += UiRound(24.0f*uiScale);
    int lineStep = UiRound(18.0f*uiScale);
    int h = UiRound(30.0f*uiScale) + n*lineStep + UiRound(8.0f*uiScale);

    Vector2 m = UiCanvasMouse();
    float bx = m.x + 18.0f*uiScale;
    float by = m.y + 8.0f*uiScale;
    if (bx + w > UiCanvasW() - 6.0f) bx = m.x - (float)w - 8.0f*uiScale;
    if (by + h > UiCanvasH() - 6.0f) by = UiCanvasH() - (float)h - 6.0f;

    DrawRectangleRec((Rectangle){ bx, by, (float)w, (float)h }, (Color){ 12, 14, 19, 245 });
    DrawRectangleLinesEx((Rectangle){ bx, by, (float)w, (float)h }, 2.0f, GameColorWithAlpha(RarityColor(item->rarity), 230));
    UiText(item->name, (int)bx + UiRound(12.0f*uiScale), (int)by + UiRound(8.0f*uiScale), titleFont, RAYWHITE);
    for (int i = 0; i < n; i++)
        UiText(lines[i], (int)bx + UiRound(12.0f*uiScale), (int)by + UiRound(30.0f*uiScale) + i*lineStep, lineFont, (Color){ 198, 205, 217, 255 });
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
            UiText(TextFormat("%s  (%s)", p->shotType.name, ShotFormName(p->shotType.form)), x + UiRound(30.0f*uiScale), cy + UiRound(7.0f*uiScale), fontShot, RAYWHITE);
        else
            UiText("Colpo base", x + UiRound(30.0f*uiScale), cy + UiRound(7.0f*uiScale), fontShot, (Color){ 170, 178, 190, 255 });
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
        int w = UiTextW(name, fontPill) + UiRound(20.0f*uiScale);
        if (chipX + w > x + width) { chipX = x; chipY += chipRowStep; }
        if (!measureOnly)
        {
            DrawRectangleRounded((Rectangle){ (float)chipX, (float)chipY, (float)w, (float)chipH }, 0.5f, 6, GameColorWithAlpha(GOLD, 40));
            DrawRectangleRoundedLines((Rectangle){ (float)chipX, (float)chipY, (float)w, (float)chipH }, 0.5f, 6, GOLD);
            UiText(name, chipX + UiRound(10.0f*uiScale), chipY + UiRound(4.0f*uiScale), fontPill, GOLD);
        }
        chipX += w + UiRound(8.0f*uiScale);
        anyPill++;
    }
    if (!anyPill)
    {
        if (!measureOnly) UiText("Nessuna sinergia: combina gli oggetti.", x, cy + UiRound(2.0f*uiScale), fontPill, (Color){ 150, 158, 172, 255 });
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
    char resLine[64];
    /* Il catalizzatore di fusione (Flux, DEC-022/DEC-072) entra nella riga
       SOLO quando se ne possiede almeno uno: e' una risorsa rara, e "0f"
       fisso in HUD sarebbe rumore per la maggior parte della run (oltre a
       cambiare l'ingombro del pannello per ogni run gia' vista). Comparire
       quando arriva e' anche il feedback che la fusione e' ora possibile. */
    if (p->flux > 0) snprintf(resLine, sizeof(resLine), "%dc  %db  %dk  %df", p->coins, p->bombs, p->keys, p->flux);
    else snprintf(resLine, sizeof(resLine), "%dc  %db  %dk", p->coins, p->bombs, p->keys);

    /* Slot funzionali (active-items.md "Feedback": stato disponibile/in
       ricarica SEMPRE visibile; grafts.md: lo slot Innesto e' visivamente
       distinto e l'interfaccia mostra quanti slot sono disponibili e quanti
       occupati). Due righe compatte sotto le risorse: il tasto fra parentesi
       quadre e' l'unico posto in cui il giocatore scopre come si usano.
       Colore: pronto = accento, in ricarica = grigio spento -- lo stato si
       legge senza rileggere i numeri. */
    int activeIndex = ItemSelectedActiveIndex(p);
    const Item *activeItem = (activeIndex >= 0) ? &p->items[activeIndex] : NULL;
    char activeLine[80];
    Color activeColor;
    if (activeItem == NULL)
    {
        snprintf(activeLine, sizeof(activeLine), "[E] attivo %d/%d  -  vuoto", 0, ItemActiveSlotCount(p));
        activeColor = (Color){ 120, 126, 138, 255 };
    }
    else if (ItemActiveIsChargeBased(activeItem))
    {
        snprintf(activeLine, sizeof(activeLine), "[E] %s  %d/%d", activeItem->name, activeItem->chargeNow, ItemActiveChargeCapacity(activeItem));
        activeColor = ItemActiveIsReady(activeItem) ? game->theme.accent : (Color){ 140, 146, 158, 255 };
    }
    else
    {
        if (ItemActiveIsReady(activeItem)) snprintf(activeLine, sizeof(activeLine), "[E] %s  pronto", activeItem->name);
        else snprintf(activeLine, sizeof(activeLine), "[E] %s  %.1fs", activeItem->name, (double)activeItem->cooldownTimer);
        activeColor = ItemActiveIsReady(activeItem) ? game->theme.accent : (Color){ 140, 146, 158, 255 };
    }

    int graftOwned = ItemCountOfKind(p, ITEM_GRAFT);
    int graftIndex = (graftOwned > 0) ? ItemIndexOfKind(p, ITEM_GRAFT, graftOwned - 1) : -1;
    char graftLine[80];
    if (graftIndex >= 0) snprintf(graftLine, sizeof(graftLine), "[G] %s  %d/%d", p->items[graftIndex].name, graftOwned, ItemGraftSlotCount(p));
    else snprintf(graftLine, sizeof(graftLine), "[G] innesto %d/%d  -  vuoto", graftOwned, ItemGraftSlotCount(p));
    Color graftColor = (graftIndex >= 0) ? game->theme.accent2 : (Color){ 120, 126, 138, 255 };

    int heartSlots = (p->maxHp + 1)/2;
    if (heartSlots < 1) heartSlots = 1;
    float heartS = 15.0f*s;
    float heartsW = (float)heartSlots*heartS*1.15f + heartS;

    /* Crust (DEC-008/WP2), ripiego SENZA pacchetto artistico (nessuna icona
       'heart_temp' qui: questo cluster disegna con DrawHeart/primitive, vedi
       il commento su DrawHudVitals piu' sopra) -- formattato da
       HudCrustLineFormat (game_renderer.h), stesso nucleo puro testato da
       --temp-health-test. Visibile solo quando il giocatore ne possiede,
       come in V3. */
    char crustLine[24];
    bool showCrust = HudCrustLineFormat(p->tempHp, crustLine, sizeof(crustLine));
    int crustFont = UiRound(14.0f*s);
    float crustGap = 6.0f*s;
    float heartsRowW = showCrust ? heartsW + crustGap + (float)UiTextW(crustLine, crustFont) : heartsW;

    int slotFont = UiRound(13.0f*s);
    float contentW = fmaxf(fmaxf((float)UiTextW(nameLine, nameFont), (float)UiTextW(resLine, resFont)), heartsRowW);
    contentW = fmaxf(contentW, (float)UiTextW(activeLine, slotFont));
    contentW = fmaxf(contentW, (float)UiTextW(graftLine, slotFont));
    float boxW = contentW + ip*2.0f;
    float rowName = 22.0f*s, rowHeart = heartS + 10.0f*s, rowRes = 22.0f*s, rowSlot = 18.0f*s;
    float boxH = ip*2.0f + rowName + rowHeart + rowRes + rowSlot*2.0f;

    Rectangle box = { gr.x + margin, gr.y + margin, boxW, boxH };
    DrawHudBox(box, game->theme.accent2, s, 214);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    UiText(nameLine, cx, cy, nameFont, character ? character->palette : (Color){ 205, 210, 220, 255 });
    cy += UiRound(rowName);
    DrawHearts(p, cx, cy, s);
    if (showCrust)
        UiText(crustLine, (int)((float)cx + heartsW + crustGap), (int)((float)cy + heartS*0.35f), crustFont, (Color){ 162, 201, 255, 255 });
    cy += UiRound(rowHeart);
    UiText(resLine, cx, cy, resFont, GOLD);
    cy += UiRound(rowRes);
    UiText(activeLine, cx, cy, slotFont, activeColor);
    cy += UiRound(rowSlot);
    UiText(graftLine, cx, cy, slotFont, graftColor);
}

/* Alto-destra: progressione della run e minimappa (priorita' 4 di ui/hud.md).
   Mondo/boss/piano/stanza/tempo/fonte in testa, poi la mappa in un angolo
   semitrasparente (DEC-137, "minimappa in un angolo"). L'FPS resta ma discreto,
   in coda alla riga della fonte. */
static void DrawHudRunStatus(Game *game, Rectangle gr, float s)
{
    float margin = 12.0f*s, ip = 11.0f*s;
    int font = UiRound(13.0f*s);
    int fpsFont = UiRound(12.0f*s);

    char worldLine[128], floorLine[96], bossLine[80], timerLine[32];
    snprintf(worldLine, sizeof(worldLine), "%s / %s", game->theme.name, game->theme.style);
    snprintf(bossLine, sizeof(bossLine), "Boss: %s", game->theme.bossName);
    snprintf(floorLine, sizeof(floorLine), "Piano %d/%d  -  %s", game->floor, FLOOR_COUNT, GameRoomKindName(GameCurrentRoom(game)->kind));
    /* DEC-051 chiede il timer SEMPRE visibile: questo e' il ripiego integrale
       dell'HUD (nessun pacchetto artistico), quindi il cronometro deve esserci
       anche qui, non solo nel layout V3 di DrawHudCanvas. Sta in questo
       cluster perche' e' progressione della run come piano e mondo; stesso
       formato m:ss dell'HUD V3 e di RunResults, mai ricalcolato. */
    snprintf(timerLine, sizeof(timerLine), "Tempo: %d:%02d",
             (int)game->runElapsedSeconds / 60, (int)game->runElapsedSeconds % 60);
    const char *sourceLine = game->content.loaded ? "Fonte: LLM cache" : "Fonte: fallback";
    const char *fpsText = TextFormat("%d FPS", GetFPS());

    /* Mappa compatta: celle piu' piccole del vecchio pannello (era 26px), qui
       galleggia sul gioco e non deve invadere l'angolo. */
    int mmCell = UiRound(15.0f*s), mmGap = UiRound(4.0f*s);
    int mmW = GRID_SIZE*mmCell + (GRID_SIZE - 1)*mmGap;
    int mmH = mmW;

    float lineH = 18.0f*s;
    float wText = fmaxf(fmaxf((float)UiTextW(worldLine, font), (float)UiTextW(bossLine, font)),
                        fmaxf(fmaxf((float)UiTextW(floorLine, font), (float)UiTextW(timerLine, font)),
                              (float)UiTextW(sourceLine, font) + (float)UiTextW(fpsText, fpsFont) + 14.0f*s));
    float contentW = fmaxf(wText, (float)mmW);
    float boxW = contentW + ip*2.0f;
    float gapAfterText = 8.0f*s;
    /* 5 righe e non piu' 4: la riga del tempo (DEC-051) e' fissa come le altre
       quattro, il riquadro deve crescere o la minimappa le finirebbe sopra. */
    float boxH = ip*2.0f + lineH*5.0f + gapAfterText + (float)mmH;

    Rectangle box = { gr.x + gr.width - margin - boxW, gr.y + margin, boxW, boxH };
    DrawHudBox(box, game->theme.accent2, s, 214);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    UiText(worldLine, cx, cy, font, game->theme.accent2); cy += UiRound(lineH);
    UiText(bossLine, cx, cy, font, (Color){ 214, 218, 226, 255 }); cy += UiRound(lineH);
    UiText(floorLine, cx, cy, font, RAYWHITE); cy += UiRound(lineH);
    UiText(timerLine, cx, cy, font, RAYWHITE); cy += UiRound(lineH);
    UiText(sourceLine, cx, cy, font, (Color){ 170, 178, 190, 255 });
    /* FPS in coda alla riga della fonte, allineato al bordo destro del riquadro. */
    UiText(fpsText, (int)(box.x + boxW - ip) - UiTextW(fpsText, fpsFont), cy + UiRound(1.0f*s), fpsFont, (Color){ 126, 232, 152, 255 });
    cy += UiRound(lineH + gapAfterText);
    DrawMinimap(game, (int)(box.x + (boxW - (float)mmW)*0.5f), cy, mmCell, mmGap);
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
    float innerW = fmaxf((float)UiTextW(statLine, statFont), 300.0f*s);
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
    UiText(statLine, cx, (int)(box.y + box.height - ip) - statFont, statFont, (Color){ 198, 205, 217, 255 });
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
    float contentW = (float)UiTextW(atlasMode, font);
    if (hasArch) contentW = fmaxf(contentW, (float)UiTextW(archLine, font));
    float boxW = contentW + ip*2.0f;
    float boxH = ip*2.0f + (float)nLines*lineH;

    Rectangle box = { gr.x + gr.width - margin - boxW, gr.y + gr.height - margin - boxH, boxW, boxH };
    DrawHudBox(box, game->theme.wall, s, 170);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    UiText(atlasMode, cx, cy, font, GameColorWithAlpha(game->theme.accent2, 210));
    if (hasArch) UiText(archLine, cx, cy + UiRound(lineH), font, (Color){ 170, 178, 190, 255 });
}

/* Alto-centro: la card di scoperta breve (DEC-065), quinto cluster dell'HUD.
   'Game.discoveryActive'/'discoveryActiveValid' sono la card CORRENTEMENTE in
   mostra (GameUpdate promuove la coda, src/game/game.c); questa funzione e' il
   SOLO lettore di quei campi nel binario -- prima di questo cluster nessun
   punto del renderer li leggeva, quindi la card non compariva mai a schermo
   (regressione corretta qui). Non bloccante e non centrata sul personaggio: un
   riquadro in alto, fuori dai quattro angoli gia' occupati da Vitals/RunStatus/
   Build/Log, cosi' non si sovrappone a nessuno degli altri cluster. v1 e' solo
   testo (nome + riga): nessuno sprite finche' l'HUD pixel art di W7 non ne ha
   bisogno (stesso gap dichiarato in DiscoveryCard, core/game_types.h). */
static void DrawHudDiscovery(Game *game, Rectangle gr, float s)
{
    if (!game->discoveryActiveValid) return;

    const DiscoveryCard *card = &game->discoveryActive;
    float margin = 12.0f*s, ip = 11.0f*s;
    int nameFont = UiRound(16.0f*s);
    int lineFont = UiRound(13.0f*s);

    char nameLine[64];
    snprintf(nameLine, sizeof(nameLine), "Scoperta: %s", card->name);

    float contentW = fmaxf((float)UiTextW(nameLine, nameFont), (float)UiTextW(card->line, lineFont));
    contentW = fminf(contentW, gr.width*0.6f - ip*2.0f);
    float boxW = contentW + ip*2.0f;
    float rowName = 20.0f*s, rowLine = 18.0f*s;
    float boxH = ip*2.0f + rowName + rowLine;

    Rectangle box = { gr.x + (gr.width - boxW)*0.5f, gr.y + margin, boxW, boxH };
    DrawHudBox(box, game->theme.accent, s, 224);

    int cx = (int)(box.x + ip);
    int cy = (int)(box.y + ip);
    UiText(nameLine, cx, cy, nameFont, RAYWHITE);
    cy += UiRound(rowName);
    UiText(card->line, cx, cy, lineFont, (Color){ 205, 210, 220, 255 });
}

/* ============================================================
   W8/WP-UI-1: l'HUD in PIXEL ART, layout V3 rivestito coi token Fucina.
 *
 * DOVE SI DISEGNA. Dentro il CANVAS logico 640x360 (DEC-200), non in overlay
 * sullo schermo -- e' quanto prescrive DEC-174 ("l'HUD in pixel art si
 * disegna per il canvas logico") e ha una conseguenza pratica decisiva: le
 * coordinate delle funzioni qui sotto sono ESATTAMENTE quelle del mock
 * approvato (vedi il blocco #define poco sopra DrawTransientMessage, molto
 * piu' su nel file), numero per numero, invece di essere riderivate da
 * uiScale. L'HUD scala quindi con la game view, a passi interi, e un pixel
 * dell'icona di un cuore resta grande come un pixel del pavimento -- che e'
 * la sola cosa che fa leggere l'insieme come pixel art e non come due
 * grafiche sovrapposte.
 * Il vecchio HUD in overlay (DrawOuterUi) resta come RIPIEGO integrale per il
 * caso "assets/art/ui assente": mai i due mescolati.
 *
 * CIFRA DEL LAYOUT (WP-UI-1, mock 02/08): pannelli tonali UiPanel
 * (src/render/ui_theme.h, DEC-205 "niente cornici colorate da 1px") al posto
 * dei riquadri semitrasparenti col bordo acceso di prima -- stessa massa
 * visiva del resto dell'interfaccia rivestita in WP-UI-0 (MainMenu). Il
 * testo passa da UiTextOutlined (contorno nero, per poggiare direttamente
 * sulla scena) a UiTextAt su UI_TAGLIA_1/2 quando sta DENTRO un pannello (il
 * pannello gia' garantisce il contrasto); un'ombra a doppio disegno
 * (UI_GROUND scalato di 1px) resta SOLO sui tre testi ancora flottanti sulla
 * scena -- piano/mondo in alto a destra, timer in alto al centro -- che
 * sono gli unici senza un pannello sotto. */

/* La riga dei cuori (priorita' 1 di ui/hud.md). Tre icone dal set consegnato:
   heart pieno, heart_half per il mezzo cuore, heart_empty per lo slot vuoto --
   la stessa semantica di DrawHearts (2 punti vita per cuore), disegnata con gli
   sprite invece che con due cerchi e un triangolo. Gia' sulla palette Fucina
   (assets/art/ui/icons.png: outline brace 126,34,22 + riempimento 177,58,30
   per il cuore pieno, ardesia scura per il vuoto) -- nessuna tinta da
   cambiare qui, solo la taglia (WP-UI-1: 12px -> 14px, mock 02/08). */
static void DrawHudV3Hearts(const Player *p, int x, int y)
{
    int full = p->hp/2;
    bool half = (p->hp%2) != 0;
    int slots = (p->maxHp + 1)/2;
    if (slots < 1) slots = 1;
    float scale = (float)HUD_V3_HEART_PX/16.0f;   /* le icone sono 16x16, il layout ne vuole 14 */
    for (int i = 0; i < slots; i++)
    {
        float ix = (float)(x + i*HUD_V3_HEART_STEP);
        const char *icon = (i < full) ? "heart" : ((i == full && half) ? "heart_half" : "heart_empty");
        if (i == full && half) ArtDrawIcon("heart_empty", ix, (float)y, scale, WHITE);
        ArtDrawIcon(icon, ix, (float)y, scale, WHITE);
    }
}

/* Salute temporanea/protettiva (Crust, DEC-008, WP2): nucleo PURO del conteggio
   di icone heart_temp, nessuna chiamata raylib -- stesso stile di
   HudCombatShouldDraw (game_renderer.h), testabile senza finestra aperta
   (--temp-health-test, GameTempHealthTest, src/tests/game_tests.c).
   UN'ICONA PER PUNTO di tempHp, non 2 come i cuori base (DrawHudV3Hearts
   sopra): con tutto il danno del motore pari a 1 (vedi CombatDamagePlayer,
   src/gameplay/combat.c) un passo a 2 punti per icona lasciava l'HUD
   indistinguibile fra alcuni valori consecutivi (es. 4->3 e 2->1 identici a
   schermo) e mostrava un solo punto residuo come icona piena. La granularita'
   1:1 mostra ogni punto senza bisogno di una variante 'heart_temp_half'
   nell'atlas (che non esiste, solo 'heart'/'heart_half'/'heart_empty' la
   hanno). Con PLAYER_TEMP_HP_CAP=4 (core/game_types.h) il numero massimo di
   icone e' 4, stesso ordine di grandezza dei cuori base tipici. */
int HudTempHeartsSlotCount(int tempHp)
{
    return (tempHp > 0) ? tempHp : 0;
}

/* X di partenza del contatore Crust nel layout V3: subito a destra
   dell'ultimo slot di cuore BASE (maxHp, non hp: lo slot dei cuori base non
   si restringe quando il giocatore e' ferito, vedi DrawHudV3Hearts sopra),
   con HUD_V3_TEMP_HEARTS_GAP di margine cosi' i due contatori non si
   toccano. Parte da HUD_V3_CONTENT_X (l'interno del pannello vitali, WP-UI-1)
   e non piu' da HUD_V3_MARGIN (il bordo del canvas): i cuori vivono dentro il
   pannello, non piu' flottanti sul bordo. Nucleo PURO, stesso stile di
   HudTempHeartsSlotCount sopra. */
int HudTempHeartsX(int maxHp)
{
    int baseHeartSlots = (maxHp + 1)/2;
    if (baseHeartSlots < 1) baseHeartSlots = 1;
    return HUD_V3_CONTENT_X + baseHeartSlots*HUD_V3_HEART_STEP + HUD_V3_TEMP_HEARTS_GAP;
}

/* Ripiego SENZA pacchetto artistico (DrawHudVitals sotto, il cluster che
   disegna con DrawHeart/primitive invece delle icone V3): un TESTO, non un
   colore o una forma extra, esattamente per DEC-058 ("mai solo colore") --
   "+N" e' leggibile indipendentemente da qualunque tinta usata altrove nel
   cluster. Nucleo PURO, stesso stile di HudTempHeartsSlotCount sopra: vero se
   il giocatore possiede Crust (va disegnato), riempie 'buf' con "+N"
   (stringa vuota altrimenti). */
bool HudCrustLineFormat(int tempHp, char *buf, size_t bufSize)
{
    if (tempHp <= 0)
    {
        if (bufSize > 0) buf[0] = '\0';
        return false;
    }
    snprintf(buf, bufSize, "+%d", tempHp);
    return true;
}

/* Salute temporanea/protettiva (in-game: Crust, DEC-008, WP2): un contatore
   SEPARATO, accanto ai cuori base (chiamato subito dopo DrawHudV3Hearts in
   DrawHudCanvas, alla stessa quota HUD_V3_HEARTS_Y), con l'icona dedicata
   'heart_temp' dell'atlas icone (known-issues.md #10.4: l'icona esisteva gia'
   ma nessun contatore la usava -- chiuso qui). Visibile SOLO quando il
   giocatore ne possiede (ui/hud.md, riga "Salute temporanea/protettiva":
   "Visibile quando: il giocatore ne possiede"), a differenza dei cuori base
   che restano SEMPRE visibili anche vuoti: niente slot spenti per uno strato
   che nella maggior parte della run vale zero. */
static void DrawHudV3TempHearts(const Player *p, int x, int y)
{
    int slots = HudTempHeartsSlotCount(p->tempHp);
    float scale = (float)HUD_V3_HEART_PX/16.0f;
    for (int i = 0; i < slots; i++)
        ArtDrawIcon("heart_temp", (float)(x + i*HUD_V3_HEART_STEP), (float)y, scale, WHITE);
}

/* Bronzo-chiaro delle chiavi (WP-UI-1, mock 02/08): la stessa tinta gia'
   presente nell'icona 'key'/'active' di assets/art/ui/icons.png (curata,
   sulla palette Fucina) promossa a costante locale perche' qui serve un
   quadratino di tinta pura e non esiste un token ui_theme.h dedicato per un
   solo riuso (stessa scelta di HUD_MAP_BOSS sopra in DrawMinimap). */
static const Color HUD_RES_KEY_BRONZE = { 201, 138, 46, 255 };

/* Le tre risorse spendibili come QUADRATINO di tinta piatta + numero
   (WP-UI-1, mock 02/08): lingotti, cariche di breccia, chiavi (DEC-072 per i
   nomi in gioco, DEC-013 per il raggruppamento per funzione) -- il mock
   sostituisce le icone a sprite di prima con un blocco di colore, piu'
   leggibile alla taglia minuta del pannello. I tre colori non sono
   invenzioni: oro e' UI_TITOLO, cenere e' la stessa tinta "comune" gia'
   dichiarata in UiRarityTint (nessuna nuova costante per un solo riuso),
   bronzo-chiaro e' HUD_RES_KEY_BRONZE sopra. Il Flux ha una riga propria
   sotto (DrawHudV3Flux): nel mock non e' un quarto quadratino nella stessa
   fila, e' un testo isolato. */
static void DrawHudV3Resources(Game *game, int x, int y)
{
    const Player *p = &game->player;
    struct { Color color; int value; } row[3] = {
        { UI_TITOLO, p->coins },
        { UiRarityTint(RARITY_COMMON), p->bombs },
        { HUD_RES_KEY_BRONZE, p->keys },
    };
    int cx = x;
    for (int i = 0; i < 3; i++)
    {
        DrawRectangle(cx, y, HUD_V3_RES_ICON_PX, HUD_V3_RES_ICON_PX, row[i].color);
        cx += HUD_V3_RES_ICON_ADVANCE;
        char text[16];
        snprintf(text, sizeof(text), "%d", row[i].value);
        UiTextAt(text, cx, y - 1, UI_TAGLIA_2, UI_TESTO);
        cx += UiTextWidth(text, UI_TAGLIA_2) + HUD_V3_RES_GROUP_GAP;
    }
}

/* Il Flux (DEC-022/072), riga propria sotto le tre risorse (mock 02/08):
   stessa regola di sempre, visibile solo quando se ne possiede almeno uno --
   e' una risorsa rara, "0" fisso sarebbe rumore per la maggior parte della
   run. FUSION_FLUX_COST vale 1 (gameplay/fusion.c, FusionCheck): "abbastanza
   per fondere" e "se ne possiede almeno uno" sono quindi la STESSA
   condizione -- il riquadro di evidenza di prima ("evidenziato quando
   sufficiente per una fusione", ui/hud.md) e' gia' garantito dal solo fatto
   che la riga compaia, in UI_GLINT ("punte di luce, evidenziazioni brevi"):
   un secondo segnale sovrapposto sarebbe stata una cornice accesa in piu',
   proprio cio' che la palette Fucina vieta fuori da UiPanel. */
static void DrawHudV3Flux(Game *game, int x, int y)
{
    const Player *p = &game->player;
    if (p->flux <= 0) return;
    char text[24];
    snprintf(text, sizeof(text), "FLUX %d", p->flux);
    UiTextAt(text, x, y, UI_TAGLIA_1, UI_GLINT);
}

/* DEC-184 (ui/hud.md, "Blocco statistiche"): il blocco compatto di sola
   lettura sotto salute/risorse -- danno, cadenza, velocita' del colpo,
   velocita' di movimento, raggio, Fortuna, nell'ordine del documento e con
   gli STESSI valori del pannello "PERSONAGGIO" di BuildScreen (HudStatRowsFill
   sopra e' l'unica fonte per entrambi: nessun calcolo qui, solo disegno).
   Priorita' visiva 4 (sotto sopravvivenza/minacce/risorse, sopra
   progressione): una cornice 9-patch piccola e un font 5px, mai un riquadro
   grande quanto quello dei cuori/risorse -- "leggibile ma discreto", non deve
   competere con un telegraph di minaccia. Il CHIAMANTE (DrawHudCanvas) decide
   se disegnarla, leggendo AppUi.hudStatsHidden: questa funzione non conosce
   il toggle, stessa disciplina di HudCombatShouldDraw/DrawHudCanvas per il
   resto dell'HUD (una sola regola di visibilita' per elemento, in un punto
   solo). */
static void DrawHudV3Stats(Game *game, int x, int y)
{
    const Player *p = &game->player;
    const char *labels[6];
    char values[6][16];
    HudStatRowsFill(p, labels, values);

    /* Colonna valori allineata alla LARGHEZZA VERA della piu' lunga fra le sei
       etichette (misurata, non un numero magico fisso): "Vel. colpo"/"Vel.
       mov." sono piu' larghe di "Danno"/"Raggio", un HUD_V3_STATS_LABEL_W
       tarato sulle corte le avrebbe fatte scontrare col valore (visto nello
       screenshot di verifica prima di questa correzione). */
    int labelW = 0, valueW = 0;
    for (int i = 0; i < 6; i++)
    {
        int lw = UiTextWidth(labels[i], UI_TAGLIA_1);
        if (lw > labelW) labelW = lw;
        int vw = UiTextWidth(values[i], UI_TAGLIA_1);
        if (vw > valueW) valueW = vw;
    }
    int valueX = x + HUD_V3_STATS_PAD + labelW + HUD_V3_STATS_GAP;
    Rectangle box = { (float)x, (float)y,
                      (float)(HUD_V3_STATS_PAD*2 + labelW + HUD_V3_STATS_GAP + valueW),
                      (float)(HUD_V3_STATS_PAD*2 + 6*HUD_V3_STATS_ROW_H) };
    UiPanel(box);

    /* WP-UI-1 (mock 02/08): etichette UI_MUTO, valori UI_TESTO allineati a
       destra -- via il verde speciale sulla Fortuna (indice 5) di prima, il
       mock tratta le sei righe in modo uniforme. */
    for (int i = 0; i < 6; i++)
    {
        int ry = y + HUD_V3_STATS_PAD + i*HUD_V3_STATS_ROW_H;
        UiTextAt(labels[i], x + HUD_V3_STATS_PAD, ry, UI_TAGLIA_1, UI_MUTO);
        UiTextAt(values[i], valueX, ry, UI_TAGLIA_1, UI_TESTO);
    }
}

/* Le due CASELLE funzionali: attivo (tasto E) e Innesto (tasto G). WP-UI-1
   (mock 02/08): la cornice a slot 9-patch diventa un UiPanel 26px come ogni
   altro riquadro dell'HUD -- il mock le mostra vuote (nessun oggetto attivo
   nella scena di riferimento), una lettera sola al centro, niente icona ne'
   barra: quello resta il caso "slot occupato", non coperto dal mock ma
   necessario al gioco vero.
   Lo STATO di ricarica e' la barra sotto l'icona quando lo slot e' occupato:
   piena in proporzione alle cariche o al cooldown residuo. active-items.md
   chiede che "disponibile/in ricarica" sia SEMPRE visibile, e una barra lo
   dice senza far leggere numeri. Pronto = oro (UI_TITOLO, lo stesso segnale
   di "in fuoco" di UiMenuRow), in carica = cenere-scura (UI_MUTO): via il
   colore procedurale della run (game->theme.accent), che ui_theme.h bandisce
   dall'interfaccia ("mai dal tema generato della run"). Una casella vuota
   resta disegnata (UiPanel comunque): grafts.md chiede che l'interfaccia
   mostri quanti slot sono disponibili, non solo quelli pieni. */
static void DrawHudV3Slot(int x, int y, const char *icon, const char *key,
                          bool filled, float fill, bool ready)
{
    Rectangle box = { (float)x, (float)y, (float)HUD_V3_SLOT_BOX, (float)HUD_V3_SLOT_BOX };
    UiPanel(box);
    if (filled)
    {
        ArtDrawIcon(icon, box.x + (HUD_V3_SLOT_BOX - 12)*0.5f, box.y + 2.0f, 12.0f/16.0f, WHITE);
        Rectangle bar = { box.x + 3.0f, box.y + 15.0f, HUD_V3_SLOT_BOX - 6.0f, 3.0f };
        DrawRectangleRec(bar, UI_GROUND);
        float clamped = GameMathClampFloat(fill, 0.0f, 1.0f);
        if (clamped > 0.0f)
            DrawRectangleRec((Rectangle){ bar.x, bar.y, bar.width*clamped, bar.height },
                             ready ? UI_TITOLO : UI_MUTO);
    }
    /* La lettera resta sempre leggibile: centrata nel riquadro quando vuoto
       (come il mock), spostata in basso quando pieno per non finire sotto
       l'icona/la barra sopra. */
    int keyW = UiTextWidth(key, UI_TAGLIA_1);
    int keyX = x + (HUD_V3_SLOT_BOX - keyW)/2;
    int keyY = filled ? (y + 20) : (y + (HUD_V3_SLOT_BOX - UiTextHeight(UI_TAGLIA_1))/2);
    UiTextAt(key, keyX, keyY, UI_TAGLIA_1, UI_SECONDARIO);
}

static void DrawHudV3Slots(Game *game, int x, int y)
{
    const Player *p = &game->player;
    int activeIndex = ItemSelectedActiveIndex(p);
    const Item *active = (activeIndex >= 0) ? &p->items[activeIndex] : NULL;
    float activeFill = 0.0f;
    bool activeReady = false;
    if (active)
    {
        activeReady = ItemActiveIsReady(active);
        if (ItemActiveIsChargeBased(active))
        {
            int capacity = ItemActiveChargeCapacity(active);
            activeFill = (capacity > 0) ? (float)active->chargeNow/(float)capacity : 0.0f;
        }
        else
        {
            /* Cooldown: la barra si RIEMPIE mentre l'attesa scende, cosi' "piena
               = pronto" vale per entrambi i modi di ricarica e il giocatore non
               deve ricordare quale dei due ha in mano. */
            float total = (active->cooldown > 0.0f) ? active->cooldown : 1.0f;
            activeFill = 1.0f - GameMathClampFloat(active->cooldownTimer/total, 0.0f, 1.0f);
        }
    }
    /* WP-UI-1: "E"/"G" senza parentesi quadre (mock 02/08) -- le parentesi
       restano il segno delle scorciatoie in TESTO scorrevole (barra comandi
       qui sotto), non della lettera isolata dentro una casella gia' quadrata. */
    DrawHudV3Slot(x, y, "active", "E", active != NULL, activeFill, activeReady);

    int graftOwned = ItemCountOfKind(p, ITEM_GRAFT);
    /* Un Innesto non ha ricarica (grafts.md: e' passivo finche' resta
       innestato): la barra e' piena quando lo slot e' occupato, vuota se no --
       "occupato/libero", che e' l'unico stato che quello slot ha. */
    DrawHudV3Slot(x + HUD_V3_SLOT_STEP, y, "graft", "G", graftOwned > 0,
                  graftOwned > 0 ? 1.0f : 0.0f, graftOwned > 0);
}

/* La card di scoperta (DEC-065/131/152): sprite, nome, una riga di
   descrizione. Posizione BASSO-CENTRO come sempre, ma piu' in alto di prima
   (HUD_V3_CARD_Y, blocco di quote sopra DrawTransientMessage): il vecchio
   Y (SCREEN_HEIGHT-56) finiva a ridosso della barra comandi nuova, il
   difetto noto che questo giro chiude spostando la card, non la barra.
   Le regole di visibilita' non cambiano di una riga: una card alla volta, coda
   in Game, scarto silenzioso su morte e cambio stanza -- tutte in src/game. */
static void DrawHudV3Card(Game *game)
{
    if (!game->discoveryActiveValid) return;
    const DiscoveryCard *card = &game->discoveryActive;
    int x = SCREEN_WIDTH/2 - HUD_V3_CARD_W/2;
    int y = HUD_V3_CARD_Y;
    Rectangle box = { (float)x, (float)y, (float)HUD_V3_CARD_W, (float)HUD_V3_CARD_H };
    UiPanel(box);   /* WP-UI-1: via il bordo acceso (game->theme.accent) del riquadro di prima -- "niente cornice arancio" */
    Rectangle slot = { box.x + 5.0f, box.y + 5.0f, 38.0f, 38.0f };
    UiPanel(slot);
    /* Lo sprite della scoperta: l'image-id sta nella card se chi l'ha accodata
       lo conosceva (GameQueueDiscoveryCard). Assente = la casella resta vuota,
       come nella v1 a solo testo: mai un rettangolo bianco al suo posto. */
    const ArtSheet *sheet = card->imageId[0] ? ArtAtlasFindByImageId(card->imageId) : NULL;
    if (sheet)
    {
        float scale = ArtScaleForWidth(sheet->frameW, 28.0f);
        Vector2 center = { slot.x + slot.width*0.5f, slot.y + slot.height*0.5f };
        Vector2 anchorPos = { center.x + ((float)sheet->anchorX - (float)sheet->frameW*0.5f)*scale,
                              center.y + ((float)sheet->anchorY - (float)sheet->frameH*0.5f)*scale };
        if (!ArtDrawAnim(sheet, "idle", (float)GetTime(), anchorPos, scale, false, WHITE))
            ArtDrawAnim(sheet, "walk", (float)GetTime(), anchorPos, scale, false, WHITE);
    }
    /* Titolo TAGLIA_2 oro, testo TAGLIA_1 (spec del mock): entrambi dentro il
       pannello, nessuna ombra -- il contrasto lo da' gia' UiPanel. */
    UiTextAt(card->name, x + 50, y + 8, UI_TAGLIA_2, UI_TITOLO);
    UiTextAt(card->line, x + 50, y + 8 + UiTextHeight(UI_TAGLIA_2) + 3, UI_TAGLIA_1, UI_TESTO);
    const char *badge = "NUOVO!";
    UiTextAt(badge, x + HUD_V3_CARD_W - UiTextWidth(badge, UI_TAGLIA_1) - 6, y + 6, UI_TAGLIA_1, UI_GLINT);
}

/* L'orchestratore dell'HUD V3. Chiamato DENTRO il canvas (vedi RendererDrawApp)
   e solo quando HudCombatShouldDraw lo consente: la regola di visibilita'
   (DEC-169) resta quella di prima, in un solo punto. 'ui' puo' essere NULL
   (alcuni test disegnano APP_GAMEPLAY senza costruire un AppUi, vedi i vari
   RendererDrawApp(..., NULL, ...) in src/tests/game_tests.c): NULL si legge
   come "nessuna preferenza salvata", quindi blocco statistiche VISIBILE, la
   stessa scelta zero-default di AppUi.hudStatsHidden. */
/* Ombra a doppio disegno per il testo ancora flottante DIRETTAMENTE sulla
   scena (nessun UiPanel sotto): 1px in UI_GROUND, poi il testo vero sopra --
   la stessa cifra del titolo di MainMenu (DrawMainMenuOverlay, WP-UI-0), non
   piu' il contorno nero di UiTextOutlined (che apparteneva al vecchio
   "elementi flottanti con contorno" del layout V3, sostituito qui). */
static void DrawHudFloatingText(const char *text, int x, int y, int taglia, Color tint)
{
    UiTextAt(text, x + 1, y + 1, taglia, UI_GROUND);
    UiTextAt(text, x, y, taglia, tint);
}

static void DrawHudCanvas(Game *game, const AppUi *ui)
{
    const Player *p = &game->player;
    const CharacterDef *character = GameResolveCharacterDef(game, game->characterChosenIndex);

    /* Larghezza della barra comandi, calcolata QUI (non piu' giu' insieme al
       suo disegno) perche' le caselle attivo/Innesto -- disegnate prima nel
       file, sotto -- devono partire alla sua DESTRA: un ordine di disegno
       invertito (barra sopra, caselle sotto) le avrebbe lasciate alla stessa
       X della barra, e la barra le avrebbe coperte disegnandosi per ultima
       (bug visto nello screenshot di verifica di questo giro). */
    const char *hint1 = "[TAB] BUILD";
    const char *hint2 = "[C] STATS";
    int hint1W = UiTextWidth(hint1, UI_TAGLIA_1);
    int hint2W = UiTextWidth(hint2, UI_TAGLIA_1);
    const int barGap = 14;
    int barW = HUD_V3_PANEL_PAD*2 + hint1W + barGap + hint2W;

    /* Pannello alto-sinistra (mock 02/08): nome, cuori, risorse, Flux -- una
       sola massa UiPanel al posto delle scritte flottanti di prima. */
    UiPanel((Rectangle){ (float)HUD_V3_PANEL_X, (float)HUD_V3_PANEL_Y, (float)HUD_V3_PANEL_W, (float)HUD_V3_PANEL_H });
    UiTextAt(character ? character->name : "Senza personaggio",
             HUD_V3_CONTENT_X, HUD_V3_NAME_Y, UI_TAGLIA_1, UI_SECONDARIO);
    DrawHudV3Hearts(p, HUD_V3_CONTENT_X, HUD_V3_HEARTS_Y);
    /* DEC-008/WP2: "accanto ai cuori" -- stessa riga (HUD_V3_HEARTS_Y), X dalla
       funzione pura HudTempHeartsX sopra (nessun numero magico qui, vedi
       il blocco di quote sopra DrawTransientMessage). */
    DrawHudV3TempHearts(p, HudTempHeartsX(p->maxHp), HUD_V3_HEARTS_Y);
    DrawHudV3Resources(game, HUD_V3_CONTENT_X, HUD_V3_RES_Y);
    DrawHudV3Flux(game, HUD_V3_CONTENT_X, HUD_V3_FLUX_Y);

    /* DEC-184: priorita' visiva 4 (sotto sopravvivenza/minacce/risorse sopra,
       sopra progressione sotto) -- disegnata PRIMA delle caselle attivo/
       Innesto nell'ordine di chiamata solo perche' sta piu' in alto sullo
       schermo, non per priorita': le due caselle restano priorita' 2. */
    if (!(ui && ui->hudStatsHidden)) DrawHudV3Stats(game, HUD_V3_STATS_X, HUD_V3_STATS_Y);
    /* A destra della barra comandi (disegnata piu' sotto, ma la sua larghezza
       e' gia' nota qui sopra): condividono il bordo inferiore, vedi
       HUD_V3_SLOTS_Y nel blocco di quote. */
    DrawHudV3Slots(game, HUD_V3_MARGIN + barW + 8, HUD_V3_SLOTS_Y);

    /* DEC-051 (ui/hud.md, "Timer di run sempre visibile"): centrato in alto,
       formato MM:SS -- l'unico testo che non sta ne' a sinistra ne' a
       destra, resta flottante con l'ombra (nessun pannello lo ospiterebbe
       senza rubare spazio alla scena al centro). */
    int minutes = (int)game->runElapsedSeconds / 60;
    int seconds = (int)game->runElapsedSeconds % 60;
    char timerText[16];
    snprintf(timerText, sizeof(timerText), "%d:%02d", minutes, seconds);
    int timerW = UiTextWidth(timerText, UI_TAGLIA_2);
    DrawHudFloatingText(timerText, SCREEN_WIDTH/2 - timerW/2, HUD_V3_NAME_Y, UI_TAGLIA_2, UI_SECONDARIO);

    /* Progressione a DESTRA, allineata al bordo: piano/stanza sopra (oro, TAGLIA_2),
       mondo sotto (UI_SECONDARIO, TAGLIA_1, MAI il colore procedurale della run --
       ui_theme.h vieta i colori del tema generato nell'interfaccia). Entrambi
       ancora flottanti sulla scena (nessun pannello dietro nel mock): l'ombra
       e' la sola concessione al contrasto. */
    char floorLine[96];
    snprintf(floorLine, sizeof(floorLine), "PIANO %d/%d : %s", game->floor, FLOOR_COUNT,
             GameRoomKindName(GameCurrentRoom(game)->kind));
    DrawHudFloatingText(floorLine, SCREEN_WIDTH - UiTextWidth(floorLine, UI_TAGLIA_2) - HUD_V3_MARGIN,
                        HUD_V3_NAME_Y, UI_TAGLIA_2, UI_TITOLO);
    DrawHudFloatingText(game->theme.name, SCREEN_WIDTH - UiTextWidth(game->theme.name, UI_TAGLIA_1) - HUD_V3_MARGIN,
                        HUD_V3_NAME_Y + UiTextHeight(UI_TAGLIA_2) + 2, UI_TAGLIA_1, UI_SECONDARIO);

    /* Minimappa su UiPanel (mock 02/08): il pannello lo disegna il chiamante
       (non DrawMinimap, che resta condivisa col ripiego DrawHudRunStatus e
       gestisce gia' un proprio riquadro esterno -- un secondo pannello
       innestato li' sarebbe ridondante). */
    int mmW = GRID_SIZE*HUD_V3_MINIMAP_CELL + (GRID_SIZE - 1)*HUD_V3_MINIMAP_GAP;
    Rectangle mmPanel = { (float)(SCREEN_WIDTH - mmW - HUD_V3_MINIMAP_PAD*2 - HUD_V3_MARGIN), (float)HUD_V3_MINIMAP_Y,
                          (float)(mmW + HUD_V3_MINIMAP_PAD*2), (float)(mmW + HUD_V3_MINIMAP_PAD*2) };
    UiPanel(mmPanel);
    DrawMinimap(game, (int)mmPanel.x + HUD_V3_MINIMAP_PAD, (int)mmPanel.y + HUD_V3_MINIMAP_PAD,
                HUD_V3_MINIMAP_CELL, HUD_V3_MINIMAP_GAP);

    DrawHudV3Card(game);

    /* Barra comandi basso-sinistra: dimensionata sul proprio contenuto (come
       ogni altro pannello di questo file, hint1/hint2/barW calcolati in testa
       alla funzione), non piu' due UiTextOutlined sciolti a X=90 fisso --
       quel numero magico presumeva una larghezza del pannello vitali che nel
       mock non esiste piu'. */
    Rectangle barBox = { (float)HUD_V3_MARGIN, (float)HUD_V3_BAR_Y, (float)barW, (float)HUD_V3_BAR_H };
    UiPanel(barBox);
    int barTextY = HUD_V3_BAR_Y + (HUD_V3_BAR_H - UiTextHeight(UI_TAGLIA_1))/2;
    UiTextAt(hint1, HUD_V3_MARGIN + HUD_V3_PANEL_PAD, barTextY, UI_TAGLIA_1, UI_SECONDARIO);
    UiTextAt(hint2, HUD_V3_MARGIN + HUD_V3_PANEL_PAD + hint1W + barGap, barTextY, UI_TAGLIA_1, UI_SECONDARIO);

    /* Diagnostica di fonte grafica (era il cluster LOG in basso a destra): niente
       pannello per lei (informazione di contorno per chi verifica una build, non
       per il giocatore) -- UI_MUTO al posto del colore procedurale della run,
       stesso motivo della riga "mondo" qui sopra. */
    const char *atlasMode = strstr(game->content.atlasPath, ".png") ? "Sprite locali (SD)" : "Atlas fallback";
    UiTextAt(atlasMode, SCREEN_WIDTH - UiTextWidth(atlasMode, UI_TAGLIA_1) - HUD_V3_MARGIN,
             SCREEN_HEIGHT - UiTextHeight(UI_TAGLIA_1) - HUD_V3_MARGIN, UI_TAGLIA_1, UI_MUTO);
}

/* DEC-169: vedi il commento sulla dichiarazione in game_renderer.h. Nucleo
   PURO, nessuna chiamata raylib -- lo stesso stile di UiComputeLayoutFor,
   testabile senza finestra aperta. */
bool HudCombatShouldDraw(AppMode mode, bool floorZeroTrialActive)
{
    if (mode == APP_GAMEPLAY) return true;
    if (mode == APP_FLOOR_ZERO) return floorZeroTrialActive;
    return false;
}

/* Orchestratore dell'HUD in overlay (DEC-137). Chiamato in APP_GAMEPLAY e, da
   DEC-169, nel Piano 0 durante una prova (HudCombatShouldDraw sopra decide
   quando): fuori da queste due situazioni l'HUD e' nascosto, come vuole
   ui/hud.md ("nascosto o attenuato durante PauseMenu e BuildScreen"), e il
   Piano 0 fuori da una prova ha i suoi overlay dedicati (riepilogo + carte)
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
    DrawHudDiscovery(game, gr, s);
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
/* 'exitConfirmLight' e' il SOLO contesto che questa geometria deve conoscere
   oltre a 'mode' (WP22, terza passata): falso = presentazione a schermo pieno
   di sempre, il valore piu' innocuo (nessun restringimento, nessun cambiamento
   rispetto a prima di WP22) -- cosi' un chiamante che non sa nulla di
   ExitConfirm, o una struct azzerata, ottiene il comportamento invariato.
   Vero SOLO dove ExitConfirmIsLightModalFor(ui->openedFrom) lo e'. Viaggia
   insieme a 'mode' per tutta la catena della geometria (MenuBoxForMode/
   MenuItemRectFor/MenuItemRect/RendererMenuItemAt) perche' disegno e hit-test
   del mouse devono restare la STESSA geometria: se il dialogo leggero si
   disegnasse stretto ma il hit-test rispondesse largo, i 70px di margine per
   lato risponderebbero a click su voci che li' non sono disegnate. */
/* ============================================================
   WP-UI-0: la geometria del MainMenu e' la PRIMA scritta in PIXEL DI CANVAS
   veri, invece che nella griglia storica 1600x900 riportata a 640x360 da
   UI_CANVAS_SCALE. E' la differenza fra "una vecchia schermata rimpicciolita"
   e "una schermata disegnata per questa risoluzione": i numeri qui sotto
   vengono dal mockup approvato dal proprietario (mock-mainmenu.png, 1280x720
   = 2x il canvas), misurati e divisi per due, non da una conversione
   automatica. Le altre sei schermate restano sulla griglia storica finche'
   non tocchera' a loro (un WP per schermata): mescolare i due sistemi in una
   schermata sola sarebbe peggio di tenerli separati fra schermate.
   ============================================================ */
#define UI_MAIN_MENU_W 340.0f
#define UI_MAIN_MENU_H 240.0f
/* Quote RELATIVE alla cima del riquadro. La prima riga comincia sotto il
   blocco d'intestazione (titolo + filetto + sottotitolo); il passo di 32
   lascia 6 px di respiro fra due fasce di fuoco alte 26. */
#define UI_MAIN_MENU_ROW_TOP 52.0f
#define UI_MAIN_MENU_ROW_H 26.0f
#define UI_MAIN_MENU_ROW_PITCH 32.0f
/* Margine orizzontale delle righe dentro il riquadro: la fascia del fuoco
   deve arrivare quasi al bordo del pannello (una fascia troppo rientrata
   sembra un bottone, e le voci NON sono bottoni). */
#define UI_MAIN_MENU_ROW_INSET 10.0f
/* Margine del blocco d'intestazione (titolo, filetto, sottotitolo): 4 px piu'
   dentro delle righe, cosi' il testo del titolo si allinea otticamente col
   testo delle voci, che ha il proprio rientro dentro la fascia. */
#define UI_MAIN_MENU_PAD 14.0f

static Rectangle MenuBoxForModeFor(AppMode mode, float sw, float sh, bool exitConfirmLight)
{
    if (mode == APP_MAIN_MENU)
    {
        return (Rectangle){ floorf(sw*0.5f - UI_MAIN_MENU_W*0.5f), floorf(sh*0.5f - UI_MAIN_MENU_H*0.5f),
                            UI_MAIN_MENU_W, UI_MAIN_MENU_H };
    }
    float uiScale = UI_CANVAS_SCALE;
    /* BuildScreen e' l'unico overlay "grande" (spec M1a: mostra la build
       intera a schermo pieno, non solo poche voci): riusa le stesse fonti
       dati del pannello BUILD/OGGETTI PRESI di DrawOuterUi, che hanno bisogno
       di piu' spazio delle 1-4 voci di un menu qualunque. */
    /* WP22 (DEC-090, gap G9 ui-cornice, TERZA passata): il riquadro piu'
       stretto (460 invece dei 600 di MainMenu/RunSetup/Options/RunResults)
       vale SOLO per il dialogo leggero "MainMenu -> Esci", cioe' quando
       'exitConfirmLight' e' vero (ExitConfirmIsLightModalFor(ui->openedFrom),
       vedi RendererDrawApp/DrawExitConfirmOverlay). Serve li' e solo li':
       il dialogo si disegna SOPRA il MainMenu ridisegnato sotto, e con la
       stessa larghezza di 600 i due riquadri combaciavano esattamente,
       coprendo il menu per intero -- nessun punto dello schermo restava
       "leggibile dietro" come richiede DEC-090. Il margine risparmiato
       (600-460=140, 70 per lato) e' maggiore del margine orizzontale delle
       righe di menu (60, vedi l'offset "60.0f*uiScale" in MenuItemRectFor
       sotto): un bordo di 10px di ciascuna riga del MainMenu (il suo
       riquadro pieno/la sua cornice, non solo il testo) ricade quindi FUORI
       dal riquadro piu' stretto di ExitConfirm, su entrambi i lati -- e
       resta visibile perche' RendererDrawApp non ridisegna piu' un secondo
       velo scuro sopra il MainMenu gia' disegnato (vedi
       DrawMainMenuOverlay/BeginMenuOverlayDim sotto).
       Gli altri TRE contesti di ExitConfirm (abbandono della preparazione nel
       Piano 0, abbandono di una run in corso da PauseMenu, rigenerazione
       della run di WP21/DEC-114) restano a schermo pieno e conservano i 600
       di sempre: DEC-090 li vuole invariati, e la seconda passata di questo
       lavoro li aveva stretti anche loro senza motivo. La misura che l'ha
       smentita (font reale assets/art/ui/font-5px.json, UiFontScale(16)=3,
       uiScale 1.0): le tre domande a schermo pieno sono larghe 765, 849 e 864
       px -- a 460 di box restavano 380 px di spazio utile fra i due margini
       da 40, meno della meta' del necessario. Da questa passata la domanda
       non e' piu' una riga sola: DrawExitConfirmOverlay la manda a capo con
       WrapTextLines dentro box.width-80, cosi' STA nel pannello in tutti e
       quattro i contesti (a 600: due righe da 501+249 / 378+456 / 480+369 px;
       a 460 l'unica domanda del contesto leggero, "Uscire dal gioco?", e'
       larga 201 px e resta su una riga sola).
       L'altezza resta 400 in tutti i contesti: le due righe di ExitConfirm
       ("Conferma"/"Annulla") cominciano a 110 (MENU_ROW_START_Y_BASE) e la
       domanda mandata a capo occupa al massimo 52..107 (tre righe da 20,
       glifi alti 15), quindi non le tocca mai.
       Verificato da UiLayoutSelfTest (voce 'e': le voci restano dentro il box
       a ogni risoluzione sintetica, in ENTRAMBE le geometrie; voce 'f': la
       geometria leggera e' davvero piu' stretta e quella a schermo pieno e'
       davvero uguale a MainMenu) e da GameExitConfirmLightModalTest
       (game_tests.c, --exit-confirm-light-modal-test) che campiona i pixel
       del frame vero. */
    float w = (mode == APP_BUILD_SCREEN ? 760.0f
             : ((mode == APP_EXIT_CONFIRM && exitConfirmLight) ? 460.0f : 600.0f))*uiScale;
    /* 560 e non piu' 520: la fascia FUSIONE in fondo (DrawFusionBand) e' una
       riga di contenuto in piu' rispetto a quando questo riquadro e' nato, e
       comprimere le liste sopra sarebbe stato peggio. A 640 px di altezza --
       la finestra minima, SCREEN_HEIGHT -- il riquadro resta comunque dentro
       lo schermo (40..600).
       PAUSE_MENU condivide lo stesso 560 da WP21 (DEC-114): con la sesta riga
       "Rigenera la run" (MenuItemCountForMode sotto) l'ultima voce arriva a
       110 + 5*52 + 40 = 410 (MENU_ROW_START_Y_BASE/MENU_ROW_H_BASE piu' sotto),
       il vecchio box da 400 la avrebbe tagliata fuori -- 560 lascia margine
       anche per il riquadro di consultazione del Piano 0 sotto le righe
       (DrawPauseMenuFloorZeroConsult, DEC-169), che a sua volta segue questa
       stessa quota (vedi il commento li'). */
    float h = (mode == APP_BUILD_SCREEN || mode == APP_PAUSE_MENU ? 560.0f : 400.0f)*uiScale;
    return (Rectangle){ sw*0.5f - w*0.5f, sh*0.5f - h*0.5f, w, h };
}

static Rectangle MenuBoxForMode(AppMode mode, bool exitConfirmLight)
{
    return MenuBoxForModeFor(mode, UiCanvasW(), UiCanvasH(), exitConfirmLight);
}

static int MenuItemCountForMode(AppMode mode, RendererMenuCtx ctx)
{
    switch (mode)
    {
        /* WP17 (DEC-050): "Continua" e' la PRIMA voce quando una sospensione
           valida esiste (ui/main-menu.md: in quel caso ha anche il focus
           iniziale), e sposta le altre quattro di un indice. Senza sospensione
           il menu resta esattamente quello di prima. */
        case APP_MAIN_MENU: return ctx.mainMenuContinue ? 5 : 4;   /* [Continua,] Nuova run, Catalogo (M8, DEC-045), Opzioni, Esci */
        case APP_RUN_SETUP: return 3;    /* Seed, Avvia, Indietro ("Modalita'" non e' selezionabile: unica modalita' esistente) */
        /* WP16 (DEC-042): "Prove" si e' inserita fra "Build e sinergie" e
           "Opzioni" (ui/pause-menu.md, tabella "Elementi interattivi"),
           spostando le due voci successive di un indice -- vedi
           DrawPauseMenuOverlay e il case APP_PAUSE_MENU in src/app/app.c per
           i nuovi indici. In pratica la riga e' SEMPRE visibile: chi apre
           PauseMenu da Gameplay lo fa da una run gia' iniziata, cioe' da DOPO
           che TrialsAssignForRun ha gia' scritto game->trialCount
           (src/game/trials.c, chiamata da GameResetRunWithSeed) -- WP15a ha
           poi aggiunto una SECONDA provenienza, il comando di pausa dal Piano
           0 (ui->pauseFromFloorZero, DEC-169), dove trialCount resta a zero
           per costruzione (nessuna run vera avviata) ma la riga non ha
           bisogno di quel dato per disegnarsi.
           WP21 (DEC-114): "Rigenera la run" si e' inserita fra "Opzioni" e
           "Abbandona run" (ora ultima, indice 5) -- vedi il commento su
           AppUi.exitRerollsRun in core/game_types.h e il case
           APP_PAUSE_MENU/APP_EXIT_CONFIRM in src/app/app.c. */
        /* WP17 (DEC-050): "Sospendi e esci" si inserisce fra "Rigenera la run"
           e "Abbandona run" (che scala a indice 6) SOLO dentro una run vera --
           dal Piano 0 il menu resta a sei righe, ed e' anche cio' che tiene il
           riquadro di consultazione di DEC-169 (DrawPauseMenuFloorZeroConsult,
           quota 420) sotto l'ultima riga senza sovrapporsi: con sette righe
           l'ultima arriverebbe a 462. */
        case APP_PAUSE_MENU: return ctx.pauseSuspend ? 7 : 6;   /* Riprendi, Build e sinergie, Prove, Opzioni, Rigenera la run, [Sospendi e esci,] Abbandona run */
        /* W8 (chiude la parte UI del difetto noto 9): tre volumi + Indietro.
           Le tre righe sono voci di menu a pieno titolo -- stesso indice, stessa
           geometria, stesso hit-test del mouse -- perche' la parita'
           tastiera/controller di DEC-057 vale anche per gli slider: su/giu'
           scelgono la riga, sinistra/destra cambiano il valore. */
        case APP_OPTIONS: return 4;      /* Volume generale, Musica, Effetti, Indietro */
        case APP_BUILD_SCREEN: return 1; /* Indietro */
        case APP_RUN_RESULTS: return 2;  /* Nuova run subito, Menu principale */
        case APP_EXIT_CONFIRM: return 2; /* Conferma, Annulla */
        default: return 0;               /* FloorZero, Gameplay: nessun menu */
    }
}

/* _BASE: i valori pre-M4, moltiplicati per uiScale in MenuItemRectFor. */
#define MENU_ROW_START_Y_BASE 110.0f
#define MENU_ROW_H_BASE 52.0f
/* DEC-159/DEC-051/WP16: RunResults ha, sopra le sue due voci, un blocco di
   righe SEMPRE presenti (esito, piano raggiunto, tempo) piu' un numero
   VARIABILE di righe facoltative (causa della sconfitta se game over,
   conteggio catalogo se >0, prove superate se ne esiste almeno una, WP16) --
   una quota fissa piu' bassa di MENU_ROW_START_Y_BASE lascia sempre spazio a
   tutte, comparissero o no le facoltative, senza dover far dipendere la
   geometria delle voci (quindi anche il hit-test del mouse,
   RendererMenuItemAt) dal contenuto della run. 172 e non piu' 150: la riga
   Tempo (WP1) e' una quinta riga SEMPRE disegnata, non una facoltativa in
   piu' -- serve lo stesso passo di 22px delle altre per non farla toccare la
   prima voce di menu. 194 e non piu' 172 (WP16): la riga "Prove superate" e'
   una TERZA facoltativa, che si somma a causa/catalogo -- stesso passo di
   22px, stesso motivo. */
#define MENU_ROW_START_Y_RUN_RESULTS 194.0f

/* M4: nucleo puro gemello di MenuBoxForModeFor -- stessa ragione (--layout-test),
   stessa garanzia (uiScale==1.0 => letterali identici a prima). */
static Rectangle MenuItemRectFor(AppMode mode, int index, float sw, float sh, bool exitConfirmLight)
{
    Rectangle box = MenuBoxForModeFor(mode, sw, sh, exitConfirmLight);
    /* WP-UI-0: il MainMenu ha la propria geometria in pixel di canvas (vedi il
       blocco UI_MAIN_MENU_* sopra). Resta comunque QUESTA la fonte unica per
       disegno e hit-test del mouse, come per ogni altra schermata: la
       differenza e' solo da dove vengono i numeri. */
    if (mode == APP_MAIN_MENU)
    {
        return (Rectangle){ box.x + UI_MAIN_MENU_ROW_INSET,
                            box.y + UI_MAIN_MENU_ROW_TOP + (float)index*UI_MAIN_MENU_ROW_PITCH,
                            box.width - UI_MAIN_MENU_ROW_INSET*2.0f, UI_MAIN_MENU_ROW_H };
    }
    float uiScale = UI_CANVAS_SCALE;
    /* BuildScreen non e' un menu di voci: e' una schermata piena con UNA sola
       riga d'azione ("Indietro"). Alla quota comune (MENU_ROW_START_Y_BASE)
       quella riga cadeva in MEZZO al contenuto, sopra "OGGETTI PRESI"; qui
       sta in fondo al riquadro, sotto la fascia FUSIONE, dove il giocatore
       la cerca. Resta una fonte di geometria SOLA, quindi il hit-test del
       mouse (RendererMenuItemAt) la segue senza sapere nulla di questa
       eccezione. */
    float rowStartY = (mode == APP_RUN_RESULTS) ? MENU_ROW_START_Y_RUN_RESULTS : MENU_ROW_START_Y_BASE;
    float top = (mode == APP_BUILD_SCREEN)
        ? box.y + box.height - 46.0f*uiScale
        : box.y + rowStartY*uiScale + (float)index*MENU_ROW_H_BASE*uiScale;
    return (Rectangle){ box.x + 60.0f*uiScale, top, box.width - 120.0f*uiScale, 40.0f*uiScale };
}

static Rectangle MenuItemRect(AppMode mode, int index, bool exitConfirmLight)
{
    return MenuItemRectFor(mode, index, UiCanvasW(), UiCanvasH(), exitConfirmLight);
}

/* Vedi game_renderer.h: la geometria dei menu vista dall'altro verso, per chi
   deve sapere DOVE guardare invece di CHI c'e' sotto un punto. */
Rectangle RendererMenuBoxBounds(AppMode mode, bool exitConfirmLight)
{
    return MenuBoxForMode(mode, exitConfirmLight);
}

Rectangle RendererMenuItemBounds(AppMode mode, int index, bool exitConfirmLight)
{
    return MenuItemRect(mode, index, exitConfirmLight);
}

int RendererMenuItemAtCtx(AppMode mode, Vector2 mouse, RendererMenuCtx ctx)
{
    int count = MenuItemCountForMode(mode, ctx);
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionPointRec(mouse, MenuItemRect(mode, i, ctx.exitConfirmLight))) return i;
    }
    return -1;
}

int RendererMenuItemAt(AppMode mode, Vector2 mouse, bool exitConfirmLight)
{
    RendererMenuCtx ctx = { 0 };
    ctx.exitConfirmLight = exitConfirmLight;
    return RendererMenuItemAtCtx(mode, mouse, ctx);
}

/* Una voce di menu: riquadro pieno + bordo se ha il focus ('focus').
   W9 (playtest round 1, copertura mouse totale): il passaggio del mouse
   SPOSTA il focus (UpdateApp lo fa PRIMA di questa chiamata, stesso frame --
   vedi il commento sul passo "hover" in UpdateApp), quindi 'hover' qui sotto
   coincide quasi sempre con 'hasFocus' per costruzione; resta calcolato a
   parte (invece di essere tolto) perche' e' innocuo e protegge il caso limite
   in cui la geometria letta qui (a disegno) e quella letta da UpdateApp (un
   frame "fa", nello stesso game loop) potessero mai divergere. 'uiScale' si
   ricalcola qui dalla finestra VERA (non e' un parametro): stessa fonte di
   MenuItemRect, che questa funzione chiama per la propria geometria -- cosi'
   il font della riga scala sempre in accordo col riquadro che lo contiene,
   senza dover far transitare uiScale per ogni DrawXOverlay che la chiama. */
static void DrawMenuRowCtx(AppMode mode, int index, const char *label, int focus, Color accent, bool exitConfirmLight)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle row = MenuItemRect(mode, index, exitConfirmLight);
    bool hasFocus = (index == focus);
    bool hover = CheckCollisionPointRec(UiCanvasMouse(), row);
    DrawRectangleRec(row, hasFocus ? GameColorWithAlpha(accent, 55) : (hover ? GameColorWithAlpha(accent, 25) : GameColorWithAlpha(BLACK, 90)));
    /* W8: la cornice della riga e' il 9-patch a SLOT (bordo sottile, senza
       rivetti), tinto dall'accento quando la riga ha il fuoco. Il fuoco resta
       segnalato da DUE cose (cornice accesa piu' riempimento piu' chiaro), non
       dal solo colore: DEC-058. */
    if (!ArtDrawSlot(row, hasFocus ? accent : GameColorWithAlpha(accent, 150)))
        DrawRectangleLinesEx(row, hasFocus ? 2.0f : 1.0f, hasFocus ? accent : GameColorWithAlpha(accent, 130));
    UiText(label, (int)row.x + UiRound(16.0f*uiScale), (int)row.y + UiRound(10.0f*uiScale), UiRound(18.0f*uiScale), hasFocus ? RAYWHITE : (Color){ 205, 210, 220, 255 });
}

/* La forma comune: nessuno degli altri overlay ha un contesto da propagare
   (il riquadro stretto esiste SOLO per il dialogo leggero di ExitConfirm,
   vedi MenuBoxForModeFor), quindi passano 'false' una volta sola qui invece
   di portarsi dietro un parametro che per loro non significherebbe nulla.
   L'unico chiamante di DrawMenuRowCtx con un valore vero e'
   DrawExitConfirmOverlay. */
static void DrawMenuRow(AppMode mode, int index, const char *label, int focus, Color accent)
{
    DrawMenuRowCtx(mode, index, label, focus, accent, false);
}

/* Cornice comune a tutti gli overlay di menu: fondo scurito su tutto lo
   schermo (mette in pausa visiva la scena sotto) + pannello raygui col
   titolo. Estratta da BeginMenuOverlay (M8) perche' il Catalogo (vedi
   BeginCatalogOverlay sotto) ha bisogno della STESSA cornice ma di un box di
   dimensioni proprie -- MenuBoxForMode e' agganciato a un AppMode dei 7
   overlay canonici, e il Catalogo vive dentro APP_MAIN_MENU (nessun nuovo
   AppMode, spec M8), quindi non puo' fornirne uno adatto da solo.
   'dimAlpha' e' il SOLO parametro che varia fra i due usi (WP22, DEC-090,
   gap G9): schermo pieno quasi opaco (190) per ogni overlay canonico, o un
   velo leggero (vedi EXIT_CONFIRM_LIGHT_DIM_ALPHA sotto) per il solo dialogo
   "MainMenu -> ExitConfirm" che deve restare un dialogo leggero SOPRA il
   menu ancora leggibile dietro -- il pannello stesso (9-patch/titolo) resta
   identico in entrambi i casi, cambia solo quanto si scurisce quello che sta
   SOTTO il box. */
static void DrawMenuOverlayChromeDim(Rectangle box, Game *game, const char *title, Color accent, int dimAlpha)
{
    float uiScale = UI_CANVAS_SCALE;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GameColorWithAlpha(BLACK, dimAlpha));
    /* W8: la cornice di OGNI schermata e' il pannello 9-patch di
       assets/art/ui, non piu' GuiPanel. Non e' solo estetica: raygui disegnava
       col proprio font vettoriale e col proprio tema, cioe' con una tipografia
       diversa da quella dei contenuti dentro il pannello -- due grafiche nello
       stesso riquadro. Il tema raygui si applica comunque (UiApplyTheme) perche'
       DrawStatLine usa ancora GuiLabel per l'allineamento delle sue due
       colonne; il resto dell'interfaccia non passa piu' da raygui.
       Il riquadro di fondo scuro si disegna PRIMA della cornice: il 9-patch e'
       una cornice con un centro semitrasparente, e senza un fondo pieno sotto
       il testo della scena si leggerebbe attraverso. */
    UiApplyTheme(&game->theme, uiScale);
    DrawRectangleRec(box, (Color){ 14, 16, 22, 240 });
    if (!ArtDrawPanel(box, WHITE))
    {
        GuiPanel(box, title);
        /* Il "24" non scala: stesso motivo di DrawPanel (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT
           e' un #define fisso di raygui). */
        DrawRectangle((int)box.x, (int)box.y + 24, (int)box.width, UiRound(2.0f*uiScale), GameColorWithAlpha(accent, 200));
        return;
    }
    /* Titolo e filetto: la stessa gerarchia che GuiPanel dava (una barra in
       testa col nome della schermata), ridisegnata coi componenti. */
    UiText(title, (int)box.x + UiRound(14.0f*uiScale), (int)box.y + UiRound(8.0f*uiScale),
           UiRound(16.0f*uiScale), accent);
    DrawRectangle((int)box.x + UiRound(8.0f*uiScale), (int)box.y + UiRound(28.0f*uiScale),
                  (int)box.width - UiRound(16.0f*uiScale), UiRound(2.0f*uiScale), GameColorWithAlpha(accent, 200));
}

/* Schermo pieno quasi opaco (190): il comportamento di sempre, per i sei
   overlay canonici che restano a schermo pieno (nessuna scena sotto deve
   restare leggibile). */
static void DrawMenuOverlayChrome(Rectangle box, Game *game, const char *title, Color accent)
{
    DrawMenuOverlayChromeDim(box, game, title, accent, 190);
}

/* WP22 (DEC-090, gap G9 ui-cornice): quanto si scurisce lo schermo dietro il
   SOLO dialogo "MainMenu -> ExitConfirm" (chiusura del gioco) -- abbastanza
   leggero da lasciare il menu sotto ancora leggibile (non solo intuibile),
   a differenza del 190 usato ovunque altrove (Piano 0/PauseMenu, che restano
   a schermo pieno per DEC-090 stesso, "gli altri usi di ExitConfirm
   mantengono la presentazione gia' documentata"). */
#define EXIT_CONFIRM_LIGHT_DIM_ALPHA 90

/* Vedi la dichiarazione in game_renderer.h: nucleo puro, nessuna chiamata
   raylib, condiviso da disegno e test (--layout-test). */
bool ExitConfirmIsLightModalFor(AppMode openedFrom, bool dropsSuspendedRun)
{
    /* WP17: la conferma di buttare via una run sospesa nasce anch'essa da
       APP_MAIN_MENU ("Nuova run" con una sospensione attiva), ma non e' la
       chiusura del gioco -- DEC-090 riserva il dialogo leggero a quel solo
       contesto, quindi qui la presentazione resta a schermo pieno come per
       l'abbandono della preparazione e l'abbandono della run. */
    return openedFrom == APP_MAIN_MENU && !dropsSuspendedRun;
}

/* Vedi le dichiarazioni in game_renderer.h: le due righe condizionali della
   sospensione, nucleo puro condiviso da disegno, hit-test e src/app. */
bool RendererMainMenuHasContinueRow(const AppUi *ui)
{
    return ui != NULL && ui->suspendEnabled && ui->suspendAvailable;
}

bool RendererPauseMenuHasSuspendRow(const Game *game, const AppUi *ui)
{
    return game != NULL && ui != NULL && ui->suspendEnabled && game->floor >= 1;
}

RendererMenuCtx RendererMenuCtxFor(const Game *game, const AppUi *ui)
{
    RendererMenuCtx ctx = { 0 };
    if (!ui) return ctx;
    ctx.exitConfirmLight = ExitConfirmIsLightModalFor(ui->openedFrom, ui->exitDropsSuspendedRun);
    ctx.mainMenuContinue = RendererMainMenuHasContinueRow(ui);
    ctx.pauseSuspend = RendererPauseMenuHasSuspendRow(game, ui);
    return ctx;
}

/* Nucleo comune a BeginMenuOverlay/BeginMenuOverlayLight sotto: il box e il
   pannello sono sempre gli stessi per un dato 'mode', cambia solo 'dimAlpha'
   (vedi il commento su DrawMenuOverlayChromeDim). WP22 (seconda passata,
   correzione del "doppio velo" contestato dal giudice): questa e' anche la
   via con cui DrawMainMenuOverlay disegna se stesso SENZA alcun
   oscuramento a schermo pieno (dimAlpha 0) quando serve come sfondo di
   ExitConfirm -- prima quel caso richiamava BeginMenuOverlay (dimAlpha fisso
   190), e l'oscuramento leggero di BeginMenuOverlayLight sopra ci si
   sommava (190+90 compositi, PIU' scuro del solo 190 di prima di WP22): con
   dimAlpha 0 qui il MainMenu sotto non disegna alcun velo proprio, e resta
   un SOLO velo leggero (quello di BeginMenuOverlayLight) sull'intero
   schermo. */
static Rectangle BeginMenuOverlayDim(AppMode mode, Game *game, const char *title, Color accent, int dimAlpha, bool exitConfirmLight)
{
    Rectangle box = MenuBoxForMode(mode, exitConfirmLight);
    DrawMenuOverlayChromeDim(box, game, title, accent, dimAlpha);
    return box;
}

/* Ritorna il box, cosi' il chiamante posiziona il resto del proprio
   contenuto senza ricalcolarlo. */
static Rectangle BeginMenuOverlay(AppMode mode, Game *game, const char *title, Color accent)
{
    return BeginMenuOverlayDim(mode, game, title, accent, 190, false);
}

/* WP22: gemella leggera di BeginMenuOverlay, stesso box/pannello ma velo di
   fondo attenuato (EXIT_CONFIRM_LIGHT_DIM_ALPHA) -- solo per il dialogo
   "MainMenu -> ExitConfirm" (vedi DrawExitConfirmOverlay e il case
   APP_EXIT_CONFIRM in RendererDrawApp, che ridisegna il MainMenu SOTTO
   prima di chiamare questa). */
static Rectangle BeginMenuOverlayLight(AppMode mode, Game *game, const char *title, Color accent)
{
    return BeginMenuOverlayDim(mode, game, title, accent, EXIT_CONFIRM_LIGHT_DIM_ALPHA, true);
}

/* 'focus' arriva come intero, non piu' come 'const AppUi *ui' (WP22, seconda
   passata): la sola cosa che questa funzione legge da 'ui' era ui->focus, e
   il chiamante "sfondo di ExitConfirm" (RendererDrawApp, case
   APP_EXIT_CONFIRM) doveva altrimenti copiare l'INTERA AppUi (337280 byte
   misurati, sizeof(AppUi)) solo per sostituire quel campo -- 330KB di
   memcpy/stack per ogni frame col dialogo aperto, per cambiare un int. Con
   'focus' diretto il chiamante normale passa ui->focus, quello "sfondo"
   passa ui->returnFocus (la riga su cui il giocatore stava davvero prima di
   aprire ExitConfirm), senza copie.
   'dimBackground' sceglie fra BeginMenuOverlay (schermo pieno quasi opaco,
   il MainMenu disegnato da solo, comportamento di sempre) e la variante
   senza velo (vedi BeginMenuOverlayDim sopra), usata SOLO quando questa
   funzione disegna il MainMenu come sfondo di ExitConfirm: il velo
   dell'intero schermo lo applica in quel caso SOLO ExitConfirm sopra (un
   velo solo, mai due sommati). */
/* WP17 (DEC-050): 'hasContinue' arriva dal chiamante (RendererMainMenuHasContinueRow,
   fonte unica) e non viene ricalcolato qui, perche' questa funzione riceve
   'focus' come intero e non l'AppUi -- vedi il commento sopra. */
/* L'etichetta di build mostrata in basso a sinistra. Vive qui e non in
   core/game_types.h perche' oggi la mostra SOLO questa schermata: il giorno in
   cui servisse anche altrove (RunResults, un log di bug) e' quello di
   spostarla, non prima. */
#define UI_BUILD_LABEL "v0.9 demo"

/* WP-UI-0: la PRIMA schermata rivestita coi token di src/render/ui_theme.h.
   Cosa cambia rispetto alla versione precedente, e perche':
   - il fondo e' pieno (UI_GROUND) e non piu' un velo nero sopra la scena di
     gioco. Il MainMenu e' una SCHERMATA, non un overlay: la stanza che si
     intravedeva dietro portava con se' i colori generati della run (i
     rosa/ciano che il proprietario ha bocciato) proprio nel primo sguardo sul
     gioco;
   - niente cornice colorata da 1 px attorno al riquadro (DEC-205): un
     pannello e' una massa tonale col bevel a due valori, che e' anche il modo
     in cui l'arte del gioco definisce i bordi;
   - i colori NON vengono piu' da game->theme (l'accento generato della run):
     l'interfaccia ha la sua tavolozza, il tema resta il colore del mondo.
   'dimBackground' falso significa "questa schermata sta facendo da SFONDO a
   un dialogo" (ExitConfirm leggero, DEC-090): in quel caso non si dipinge il
   fondo pieno, cosi' l'unico velo del frame resta quello del dialogo sopra e
   non se ne sommano due. */
static void DrawMainMenuOverlay(Game *game, int focus, bool dimBackground, bool hasContinue)
{
    (void)game;
    Rectangle box = MenuBoxForMode(APP_MAIN_MENU, false);
    if (dimBackground) DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, UI_GROUND);
    UiPanel(box);

    int pad = (int)UI_MAIN_MENU_PAD;
    int titleY = (int)box.y + 12;
    /* Ombra di 1 px in slag-nero sotto il titolo: e' l'unico "effetto" della
       schermata e serve a staccare l'oro dal marrone del pannello, che sono
       due colori vicini di valore. Un contorno su quattro lati (UiTextOutlined)
       qui non va: DEC-205 vale anche per il testo, e a taglia 2 l'oro
       contornato di nero diventa una scritta di plastica. */
    UiTextAt("Worldsmelt", (int)box.x + pad + 1, titleY + 1, UI_TAGLIA_2, UI_GROUND);
    UiTextAt("Worldsmelt", (int)box.x + pad, titleY, UI_TAGLIA_2, UI_TITOLO);

    int ruleY = titleY + UiTextHeight(UI_TAGLIA_2) + 7;
    UiDivider((int)box.x + pad, ruleY, (int)box.width - pad*2);
    UiTextAt("Roguelite con contenuti generati in locale.",
             (int)box.x + pad, ruleY + 7, UI_TAGLIA_1, UI_SECONDARIO);

    /* Le quattro voci storiche scalano di uno quando "Continua" c'e': la
       mappatura indice -> azione del case APP_MAIN_MENU (src/app/app.c) usa lo
       STESSO scarto, calcolato dalla stessa condizione. */
    int base = hasContinue ? 1 : 0;
    if (hasContinue) UiMenuRow(MenuItemRect(APP_MAIN_MENU, 0, false), "Continua", focus == 0);
    UiMenuRow(MenuItemRect(APP_MAIN_MENU, base + 0, false), "Nuova run", focus == base + 0);
    UiMenuRow(MenuItemRect(APP_MAIN_MENU, base + 1, false), "Catalogo", focus == base + 1);
    UiMenuRow(MenuItemRect(APP_MAIN_MENU, base + 2, false), "Opzioni", focus == base + 2);
    UiMenuRow(MenuItemRect(APP_MAIN_MENU, base + 3, false), "Esci", focus == base + 3);

    /* Fuori dal pannello, nell'angolo: c'e' per chi la cerca (un rapporto di
       bug), non deve competere con niente. */
    UiTextAt(UI_BUILD_LABEL, 8, SCREEN_HEIGHT - UiTextHeight(UI_TAGLIA_1) - 7, UI_TAGLIA_1, UI_MUTO);
}

/* WP22 (terza passata, ui/run-setup.md): etichetta e fascia della riga
   informativa "Modalita'" di RunSetup, condivise fra il disegno
   (DrawRunSetupOverlay qui sotto) e il test (GameRunSetupModeLineTest,
   src/tests/game_tests.c) -- fonte UNICA, stesso principio di
   MenuBoxForMode/MenuItemRect: senza questo il test avrebbe dovuto indovinare
   la quota, e cancellare la riga sarebbe rimasto invisibile a make test
   (difetto contestato dal giudice).
   La quota 78 non e' cosmesi: fino alla seconda passata la riga si disegnava a
   MENU_ROW_START_Y_BASE + MENU_ROW_H_BASE*0.62 = 142, cioe' DENTRO la fascia
   della voce "Seed" (110..150, vedi MenuItemRectFor) -- si sovrapponeva al
   bordo inferiore di una riga selezionabile. Fra il filetto del titolo (che
   finisce a 30) e la prima voce (110) c'e' invece una fascia libera larga 80:
   78..93 (glifi alti 10 a UiFontScale(14)=2) la usa lasciando 48px sopra e
   17px sotto, senza toccare nessuna delle tre voci di menu. */
#define RUN_SETUP_MODE_LABEL_X_BASE 76.0f
#define RUN_SETUP_MODE_LABEL_Y_BASE 78.0f
#define RUN_SETUP_MODE_LABEL_H_BASE 20.0f

const char *RendererRunSetupModeLabel(void)
{
    return "Modalita': Standard";
}

Rectangle RendererRunSetupModeLabelBand(void)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = MenuBoxForMode(APP_RUN_SETUP, false);
    float x = box.x + RUN_SETUP_MODE_LABEL_X_BASE*uiScale;
    return (Rectangle){ x, box.y + RUN_SETUP_MODE_LABEL_Y_BASE*uiScale,
                        box.x + box.width - 40.0f*uiScale - x, RUN_SETUP_MODE_LABEL_H_BASE*uiScale };
}

static void DrawRunSetupOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    BeginMenuOverlay(APP_RUN_SETUP, game, "NUOVA RUN", game->theme.accent2);   /* il box torna da RendererRunSetupModeLabelBand/MenuItemRect, non serve qui */
    /* "Modalita'" e' un'etichetta fissa (unica modalita' esistente, DEC-038:
       niente selettore di difficolta'), non una voce selezionabile: disegnata
       SOPRA le tre voci, nella fascia libera fra il filetto del titolo e la
       riga "Seed", senza passare da DrawMenuRow/MenuItemRect -- cosi' non
       occupa un indice, non e' cliccabile e non si sovrappone piu' a nessuna
       riga (vedi RendererRunSetupModeLabelBand sopra). */
    Rectangle band = RendererRunSetupModeLabelBand();
    UiText(RendererRunSetupModeLabel(), (int)band.x, (int)band.y, UiRound(14.0f*uiScale), (Color){ 176, 184, 198, 255 });
    DrawMenuRow(APP_RUN_SETUP, 0, TextFormat("Seed: %u  (R rigenera)", ui->seed), ui->focus, game->theme.accent2);
    DrawMenuRow(APP_RUN_SETUP, 1, "Avvia", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_RUN_SETUP, 2, "Indietro", ui->focus, game->theme.accent2);
}

/* DEC-169 (ui/pause-menu.md, "Consultazione dell'HUD nel Piano 0"): nel Piano
   0 l'HUD di combattimento resta nascosto fuori dalle prove (vedi
   HudCombatShouldDraw sopra); questo riquadro e' il punto in cui quella
   stessa informazione (salute, risorse) resta consultabile su richiesta,
   senza uscire dal Piano 0. Si disegna ogni volta che APP_PAUSE_MENU viene
   raggiunto con game->floor == 0, indipendentemente da QUALE comando abbia
   aperto la pausa: quel comando resta la domanda aperta 22
   (governance/open-questions.md, DEC-169 non lo fissa, ESC e' gia'
   ExitConfirm nel Piano 0) -- gap esplicito stile DEC-009/052, non deciso
   qui. */
static void DrawPauseMenuFloorZeroConsult(Game *game, Rectangle box, float uiScale)
{
    const Player *p = &game->player;
    int labelFont = UiRound(15.0f*uiScale);
    int x = (int)box.x + UiRound(40.0f*uiScale);
    /* 420 e non piu' 368: con la sesta voce "Rigenera la run" (WP21, DEC-114)
       le righe del PauseMenu arrivano a box.y+410 (110 + 5*52 + 40, l'ultima
       riga e' ora "Abbandona run" all'indice 5) e la vecchia quota (scelta
       per cinque righe, 110 + 4*52 + 40 = 358) ci sarebbe finita sotto per
       soli 10px, sovrapponendosi al testo della riga. Questa funzione E'
       raggiungibile dal Piano 0 (WP15a, pauseFromFloorZero): la quota segue
       le righe, come da nota precedente -- MenuBoxForModeFor riserva ora 560
       (non piu' 400) di altezza per APP_PAUSE_MENU proprio per lasciare
       spazio a questo riquadro sotto le sei righe. */
    int y = (int)box.y + UiRound(420.0f*uiScale);
    UiText("Stato (Piano 0):", x, y, labelFont, game->theme.accent2);
    y += UiRound(22.0f*uiScale);
    DrawHearts(p, x, y, uiScale);
    UiText(TextFormat("%dc  %db  %dk  %df", p->coins, p->bombs, p->keys, p->flux),
              x + UiRound(140.0f*uiScale), y, labelFont, GOLD);
}

/* WP16 (DEC-042, ui/pause-menu.md "Prove"): il pannello che sostituisce le
   righe di menu mentre 'ui->pauseTrialsOpen' e' vero -- elenco delle prove
   della run con testo e stato, piu' il totale superate/bonus (la stessa
   somma che DrawRunResultsOverlay/il riepilogo di BuildScreen mostrano,
   TrialsBonusTotal e' l'unica fonte). Colore per stato: nessuna informazione
   affidata al solo colore (DEC-058), l'etichetta testuale (TrialStateLabel)
   resta sempre disegnata accanto. */
static void DrawTrialsPanel(Game *game, Rectangle box, float uiScale)
{
    int headFont = UiRound(16.0f*uiScale);
    int labelFont = UiRound(14.0f*uiScale);
    int x = (int)box.x + UiRound(40.0f*uiScale);
    int y = (int)box.y + UiRound(64.0f*uiScale);
    UiText(TextFormat("PROVE DELLA RUN -- %d/%d superate, +%d punti",
                       TrialsPassedCount(game), TrialsCountedTotal(game), TrialsBonusTotal(game)),
           x, y, headFont, game->theme.accent2);
    y += UiRound(34.0f*uiScale);
    for (int i = 0; i < game->trialCount; i++)
    {
        const Trial *t = &game->trials[i];
        /* TRIAL_VOID (WP16, seconda tornata) cade nello stesso grigio neutro
           di TRIAL_IN_PROGRESS di proposito: non e' un esito negativo (il
           denominatore in testa gia' la esclude, TrialsCountedTotal), quindi
           non merita il rosso di TRIAL_FAILED -- l'etichetta testuale
           ("annullata") resta comunque la fonte che distingue i due casi. */
        Color color = (t->state == TRIAL_PASSED) ? (Color){ 126, 232, 152, 255 }
                    : (t->state == TRIAL_FAILED) ? (Color){ 232, 120, 120, 255 }
                                                  : (Color){ 205, 210, 220, 255 };
        UiText(TextFormat("- %s", t->text), x, y, labelFont, color);
        y += UiRound(20.0f*uiScale);
        UiText(TextFormat("  [%s, +%d]", TrialStateLabel(t->state), t->bonus), x, y, labelFont, color);
        y += UiRound(28.0f*uiScale);
    }
    UiText("ESC o INVIO per tornare.", x, (int)(box.y + box.height) - UiRound(40.0f*uiScale),
           labelFont, (Color){ 150, 158, 172, 255 });
}

static void DrawPauseMenuOverlay(Game *game, const AppUi *ui)
{
    Rectangle box = BeginMenuOverlay(APP_PAUSE_MENU, game, "PAUSA", game->theme.accent2);
    if (ui->pauseTrialsOpen)
    {
        DrawTrialsPanel(game, box, UI_CANVAS_SCALE);
        return;
    }
    DrawMenuRow(APP_PAUSE_MENU, 0, "Riprendi", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 1, "Build e sinergie", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 2, "Prove", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, 3, "Opzioni", ui->focus, game->theme.accent2);
    /* WP21 (DEC-114): "Rigenera la run" -- il reroll a seed nuovo, con
       conferma esplicita via ExitConfirm (ui->exitRerollsRun). Mai un tasto
       rapido diretto (il vecchio R e' ora SOLO il reset rapido stesso seed,
       src/app/app.c, case APP_GAMEPLAY). */
    DrawMenuRow(APP_PAUSE_MENU, 4, "Rigenera la run", ui->focus, game->theme.accent2);
    /* WP17 (DEC-050): l'uscita NON distruttiva -- scrive la sospensione e
       torna al menu principale, dove "Continua" riportera' qui. Sta subito
       sopra "Abbandona run" apposta: fra le due uscite, quella che non perde
       nulla si incontra per prima. Presente solo in una run vera
       (RendererPauseMenuHasSuspendRow, fonte unica anche per il conteggio
       delle voci e per src/app/app.c). */
    bool hasSuspend = RendererPauseMenuHasSuspendRow(game, ui);
    if (hasSuspend) DrawMenuRow(APP_PAUSE_MENU, 5, "Sospendi e esci", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_PAUSE_MENU, hasSuspend ? 6 : 5, "Abbandona run", ui->focus, game->theme.accent2);
    if (game->floor == 0)
        DrawPauseMenuFloorZeroConsult(game, box, UI_CANVAS_SCALE);
}

/* Geometria della barra di una riga-slider (indice 0..2: generale/musica/
   effetti), separata dal disegno (W9, playtest round 1: le barre diventano
   TRASCINABILI col mouse) -- stessa ragione di MenuItemRect/ThemeCardRectFor:
   una sola fonte, sia per disegnare la barra sia per il hit-test/trascinamento
   che UpdateApp (src/app/app.c) le applica in RendererOptionsSliderValueAt
   sotto. 'sw'/'sh' espliciti (mai GetScreenWidth/Height qui dentro) cosi'
   resta nello stile "For" del resto del file, testabile su risoluzioni
   sintetiche. */
static Rectangle OptionsSliderBarRectFor(int index, float sw, float sh)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle row = MenuItemRectFor(APP_OPTIONS, index, sw, sh, false);
    float barW = row.width*0.42f;
    float barX = row.x + row.width - barW - UiRound(48.0f*uiScale);
    return (Rectangle){ barX, row.y, barW, row.height };
}

/* Il valore 0..1 che corrisponde a una posizione orizzontale del mouse dentro
   la barra della riga-slider 'index' (schermo VERO, come RendererMenuItemAt):
   clampato ai due estremi, cosi' trascinare oltre il bordo della barra non fa
   'perdere' il valore 0%/100%, il comportamento atteso di uno slider.
   W9 correzione round 0 (MINORE): il risultato viene poi AGGANCIATO al passo
   di OPTIONS_VOLUME_STEP (arrotondamento alla casella piu' vicina delle
   OPTIONS_VOLUME_CELLS) -- senza questo, il trascinamento produceva un valore
   CONTINUO (es. 47.6%) che nessun input da tastiera puo' mai produrre
   (sinistra/destra si muovono solo a passi del 10%, docs/design/ui/
   options-and-accessibility.md) e che la barra a caselle non puo'
   rappresentare fedelmente. Il clamp resta comunque valido dopo lo snap
   (0.0 e 1.0 sono gia' multipli esatti del passo). */
float RendererOptionsSliderValueAt(int index, float mouseX)
{
    Rectangle bar = OptionsSliderBarRectFor(index, UiCanvasW(), UiCanvasH());
    if (bar.width <= 0.0f) return 0.0f;
    float raw = GameMathClampFloat((mouseX - bar.x)/bar.width, 0.0f, 1.0f);
    float snapped = roundf(raw/OPTIONS_VOLUME_STEP)*OPTIONS_VOLUME_STEP;
    return GameMathClampFloat(snapped, 0.0f, 1.0f);
}

/* W9, correzione dopo tripla bocciatura del passo mouse: il cancello del
   PRESS. Senza questo, il click su QUALUNQUE punto della riga (etichetta e
   frecce comprese) veniva mappato sulla barra da ValueAt, e il clamp lo
   trasformava in 0%/100%: un click di navigazione non deve mai cambiare un
   volume. Il margine di presa rende comoda la barra senza inghiottire la
   riga; una volta aperto il trascinamento, conta solo la X (comportamento
   da slider gia' esistente in UpdateApp). Stessa geometria di ValueAt. */
bool RendererOptionsSliderHit(int index, Vector2 mouse)
{
    Rectangle bar = OptionsSliderBarRectFor(index, UiCanvasW(), UiCanvasH());
    float slack = 6.0f*UI_CANVAS_SCALE;
    Rectangle grab = { bar.x - slack, bar.y, bar.width + 2.0f*slack, bar.height };
    return CheckCollisionPointRec(mouse, grab);
}

/* Una riga-slider del menu Opzioni (W8): etichetta, barra a passi discreti,
   valore in percentuale. La barra e' fatta di CASELLE e non di un cursore
   continuo -- il volume si muove a passi del 10% (OPTIONS_VOLUME_STEP), e dieci
   caselle dicono a colpo d'occhio quante ce ne sono e quante sono accese, cosa
   che un cursore su una guida liscia non dice. Le frecce ai due lati
   dell'etichetta sono il promemoria del comando da tastiera; W9: la barra e'
   anche trascinabile col mouse (RendererOptionsSliderValueAt, applicata da
   UpdateApp), DEC-057, il mouse e' ammesso nei menu.
   Il valore si legge SIA dalla barra SIA dalla percentuale scritta: nessuna
   informazione affidata al solo colore o alla sola lunghezza (DEC-058). */
static void DrawOptionsSliderRow(Game *game, const AppUi *ui, int index, const char *label, float value)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle row = MenuItemRect(APP_OPTIONS, index, false);
    DrawMenuRow(APP_OPTIONS, index, label, ui->focus, game->theme.accent2);

    bool hasFocus = (index == ui->focus);
    int font = UiRound(14.0f*uiScale);
    char percent[8];
    snprintf(percent, sizeof(percent), "%d%%", (int)(GameMathClampFloat(value, 0.0f, 1.0f)*100.0f + 0.5f));

    /* La barra occupa la meta' destra della riga, il testo la sinistra: cosi'
       un'etichetta piu' lunga non spinge mai la barra fuori dal riquadro.
       barX/barW vengono da OptionsSliderBarRectFor sopra -- stessa geometria
       del trascinamento, mai due formule a rischio di disallinearsi. */
    float cellGap = 2.0f*uiScale;
    Rectangle barRect = OptionsSliderBarRectFor(index, UiCanvasW(), UiCanvasH());
    float barW = barRect.width;
    float barX = barRect.x;
    float cellW = (barW - cellGap*(float)(OPTIONS_VOLUME_CELLS - 1))/(float)OPTIONS_VOLUME_CELLS;
    float cellH = UiRound(12.0f*uiScale);
    float barY = row.y + (row.height - cellH)*0.5f;
    int lit = (int)(GameMathClampFloat(value, 0.0f, 1.0f)*(float)OPTIONS_VOLUME_CELLS + 0.5f);
    for (int i = 0; i < OPTIONS_VOLUME_CELLS; i++)
    {
        Rectangle cell = { barX + (float)i*(cellW + cellGap), barY, cellW, cellH };
        DrawRectangleRec(cell, i < lit ? (hasFocus ? game->theme.accent : GameColorWithAlpha(game->theme.accent, 170))
                                       : (Color){ 34, 37, 46, 220 });
        DrawRectangleLinesEx(cell, 1.0f, GameColorWithAlpha(BLACK, 160));
    }
    UiText(percent, (int)(row.x + row.width - UiRound(42.0f*uiScale)), (int)(row.y + UiRound(11.0f*uiScale)),
           font, hasFocus ? RAYWHITE : (Color){ 190, 196, 208, 255 });
    if (hasFocus)
    {
        UiText("<", (int)(barX - UiRound(14.0f*uiScale)), (int)(barY - UiRound(1.0f*uiScale)), font, game->theme.accent2);
        UiText(">", (int)(barX + barW + UiRound(4.0f*uiScale)), (int)(barY - UiRound(1.0f*uiScale)), font, game->theme.accent2);
    }
}

static void DrawOptionsOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = BeginMenuOverlay(APP_OPTIONS, game, "OPZIONI", game->theme.accent2);
    /* La categoria "audio" e' la prima delle categorie minime di
       ui/options-and-accessibility.md; le altre (video, controlli,
       accessibilita', gameplay) restano da scrivere e non si inventano qui.
       Lo schermo intero resta l'unica informazione consultabile non ancora
       promossa a voce, come prima di W8. */
    UiText("AUDIO", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(52.0f*uiScale),
           UiRound(14.0f*uiScale), game->theme.accent);
    DrawOptionsSliderRow(game, ui, 0, "Volume generale", AudioGetMasterVolume());
    DrawOptionsSliderRow(game, ui, 1, "Musica", AudioGetMusicVolume());
    DrawOptionsSliderRow(game, ui, 2, "Effetti", AudioGetSfxVolume());
    DrawMenuRow(APP_OPTIONS, 3, "Indietro", ui->focus, game->theme.accent2);
    UiText("Schermo intero -- F11", (int)box.x + UiRound(40.0f*uiScale),
           (int)(box.y + box.height - UiRound(34.0f*uiScale)), UiRound(13.0f*uiScale),
           (Color){ 160, 168, 182, 255 });
}

/* ============================================================
   La fascia FUSIONE in fondo a BuildScreen (systems/item-fusion.md +
   ui/inventory-and-synergy-screen.md, riga "Fusioni possibili").
   Mostra esattamente cio' che il documento chiede e nient'altro: quali due
   oggetti verrebbero consumati, se il catalizzatore c'e', e l'esito
   (nome + immagine). Nessun numero interno, nessuno stato di validazione,
   nessun prompt: sono i "Non mostrare" del documento.
   ============================================================ */
#define FUSION_BAND_H_BASE 176.0f

/* Marcatori sulla riga di un oggetto: la barra a sinistra e' il fuoco da
   tastiera, il quadratino a destra col numero dice che quell'oggetto e' la
   prima o la seconda sorgente scelta. Disegnati QUI e non dentro
   DrawItemPreview perche' quella funzione la condivide l'HUD di gioco, dove
   una selezione per la fusione non esiste. */
static void DrawFusionRowMark(const AppUi *ui, int index, int focus, int x, int y, int width, float uiScale, Color accent)
{
    int rowH = UiRound(58.0f*uiScale);
    if (index == focus)
        DrawRectangle(x - UiRound(9.0f*uiScale), y, UiRound(4.0f*uiScale), rowH, accent);

    int a = FUSION_UI_SLOT(ui->fusionSourceA);
    int b = FUSION_UI_SLOT(ui->fusionSourceB);
    const char *badge = (index == a) ? "1" : ((index == b) ? "2" : NULL);
    if (!badge) return;

    int size = UiRound(20.0f*uiScale);
    Rectangle mark = { (float)(x + width - size - UiRound(8.0f*uiScale)), (float)(y + UiRound(8.0f*uiScale)), (float)size, (float)size };
    DrawRectangleRec(mark, GameColorWithAlpha(accent, 200));
    UiText(badge, (int)mark.x + UiRound(6.0f*uiScale), (int)mark.y + UiRound(3.0f*uiScale), UiRound(14.0f*uiScale), BLACK);
}

/* DrawText che non deborda: se il testo e' piu' largo di 'maxWidth' lo taglia
   e chiude con "..". Serve alla fascia FUSIONE, dove i nomi degli oggetti sono
   contenuto GENERATO (fino a 47 caratteri) dentro riquadri di larghezza fissa:
   senza, un nome lungo scriverebbe sopra il riquadro accanto. */
static void DrawTextClipped(const char *text, int x, int y, int font, Color color, int maxWidth)
{
    if (UiTextW(text, font) <= maxWidth) { UiText(text, x, y, font, color); return; }

    char head[160];
    snprintf(head, sizeof(head), "%s", text);
    for (int len = (int)strlen(head); len > 0; len--)
    {
        char probe[164];
        head[len - 1] = '\0';
        snprintf(probe, sizeof(probe), "%s..", head);
        if (UiTextW(probe, font) <= maxWidth) { UiText(probe, x, y, font, color); return; }
    }
    /* Nemmeno ".." ci starebbe: meglio non scrivere nulla che debordare. */
}

static void DrawFusionSourceSlot(const Game *game, int field, int ordinal, int x, int y, int width, float uiScale)
{
    const Player *p = &game->player;
    int slot = FUSION_UI_SLOT(field);
    int count = GameMathClampInt(p->itemCount, 0, MAX_ITEMS);
    bool filled = slot >= 0 && slot < count;
    Rectangle box = { (float)x, (float)y, (float)width, 34.0f*uiScale };

    DrawRectangleRec(box, GameColorWithAlpha(BLACK, 120));
    Color frame = filled ? RarityColor(p->items[slot].rarity) : (Color){ 90, 96, 110, 255 };
    if (!ArtDrawSlot(box, frame)) DrawRectangleLinesEx(box, filled ? 2.0f : 1.0f, frame);
    /* W8: la casella mostra anche lo SPRITE della sorgente scelta, non solo il
       nome -- e' cio' che rende leggibile "quali due oggetti si consumano"
       senza rileggere due righe di testo (mock V3 della fascia FUSIONE). Il
       testo si sposta a destra dell'icona solo quando l'icona c'e' davvero:
       una casella vuota non deve avere un rientro senza motivo. */
    int textX = x + UiRound(10.0f*uiScale);
    if (filled)
    {
        float iconSize = 26.0f*uiScale;
        Vector2 center = { box.x + iconSize*0.5f + 4.0f*uiScale, box.y + box.height*0.5f };
        /* Cast: DrawItemIcon deve poter riempire la cache delle texture curate,
           che vive dentro Game -- questa funzione riceve un Game const perche'
           non tocca stato di gioco, e la cache non e' stato di gioco. Stessa
           natura del cast che DrawHudBuild non ha bisogno di fare solo perche'
           riceve un Game* non-const. */
        if (DrawItemIcon((Game *)game, &p->items[slot], center, iconSize))
            textX = x + UiRound(38.0f*uiScale);
    }
    char line[96];
    if (filled) snprintf(line, sizeof(line), "%d.  %s", ordinal, p->items[slot].name);
    else snprintf(line, sizeof(line), "%d.  -- (INVIO sceglie)", ordinal);
    DrawTextClipped(line, textX, y + UiRound(9.0f*uiScale), UiRound(14.0f*uiScale),
                    filled ? RAYWHITE : (Color){ 140, 148, 162, 255 }, width - (textX - x) - UiRound(10.0f*uiScale));
}

/* W9 correzione round 1 (MINORE, "nessun percorso col solo mouse porta a
   termine una fusione"): geometria della riga di stato/azione della fascia
   FUSIONE -- quella che dice "fondi" quando si puo' fondere e il PERCHE' no
   altrimenti. Fattorizzata perche' e' l'unica fonte sia del disegno (sotto) sia
   del hit-test del mouse (RendererFusionConfirmAt): un click qui vale [F],
   stessa funzione (AppFusionConfirm), stesso messaggio di esito anche quando la
   fusione non si puo' fare. La riga e' un po' piu' alta del testo (22 contro 14
   punti di font) perche' un bersaglio di click sotto i ~20 px e' scomodo; resta
   ben separata sia dalle righe oggetto sopra (che finiscono prima di 'bandY',
   vedi BuildScreenItemListLayoutFor) sia dalla riga "Indietro" sotto
   (MenuItemRectFor: box.y + box.height - 46*uiScale), verificato da
   RendererMouseHitTestSelfTest. */
static Rectangle FusionConfirmRowRectFor(int x, int y, int width, float uiScale)
{
    int hy = y + UiRound(10.0f*uiScale) + UiRound(24.0f*uiScale) + UiRound(40.0f*uiScale);
    return (Rectangle){ (float)x, (float)hy - 2.0f*uiScale, (float)width, 22.0f*uiScale };
}

static void DrawFusionBand(Game *game, const AppUi *ui, int x, int y, int width, float uiScale)
{
    const Player *p = &game->player;
    Color accent = game->theme.accent2;
    DrawRectangle(x, y, width, UiRound(2.0f*uiScale), GameColorWithAlpha(accent, 120));

    int ty = y + UiRound(10.0f*uiScale);
    UiText("FUSIONE", x, ty, UiRound(16.0f*uiScale), accent);
    /* Il catalizzatore in chiaro: e' l'unica condizione che il giocatore non
       puo' dedurre dalla lista oggetti (item-fusion.md, caso limite
       "nessun catalizzatore": l'interfaccia lo segnala). */
    char fluxLine[48];
    snprintf(fluxLine, sizeof(fluxLine), "Flux: %d", p->flux);
    int fluxFont = UiRound(15.0f*uiScale);
    UiText(fluxLine, x + width - UiTextW(fluxLine, fluxFont), ty, fluxFont,
             p->flux > 0 ? (Color){ 226, 138, 255, 255 } : (Color){ 150, 158, 172, 255 });

    int sy = ty + UiRound(24.0f*uiScale);
    int gap = UiRound(14.0f*uiScale);
    int slotW = (width - gap)/2;
    DrawFusionSourceSlot(game, ui->fusionSourceA, 1, x, sy, slotW, uiScale);
    DrawFusionSourceSlot(game, ui->fusionSourceB, 2, x + slotW + gap, sy, slotW, uiScale);

    /* Riga di stato: se si puo' fondere lo dice, altrimenti dice PERCHE' no
       -- e' lo stesso testo che comparirebbe premendo F, mostrato prima di
       premerlo. */
    FusionStatus status = FusionCheck(p, FUSION_UI_SLOT(ui->fusionSourceA), FUSION_UI_SLOT(ui->fusionSourceB));
    /* La quota della riga viene dalla SUA geometria (FusionConfirmRowRectFor
       sopra), non da un secondo calcolo: e' anche il bersaglio del click. */
    Rectangle confirmRow = FusionConfirmRowRectFor(x, y, width, uiScale);
    /* Hover del mouse sulla riga (AppUi.fusionConfirmHover): velo leggero
       dello stesso verde della conferma, cosi' il bersaglio cliccabile si
       vede PRIMA del click, come per ogni voce di menu. */
    if (ui->fusionConfirmHover)
        DrawRectangleRec(confirmRow, (Color){ 126, 232, 152, 30 });
    int hy = (int)(confirmRow.y + 2.0f*uiScale);
    /* W9 correzione round 1: l'etichetta nomina ENTRAMBE le vie, perche' adesso
       la riga e' anche cliccabile (DEC-057: il mouse e' ammesso in tutto cio'
       che e' menu, e senza questo un giocatore col solo mouse non poteva
       portare a termine nessuna fusione). */
    DrawTextClipped(status == FUSION_OK ? "[F] o click: fondi -- consuma 2 oggetti + 1 Flux" : FusionStatusText(status),
                    x, hy, UiRound(14.0f*uiScale),
                    status == FUSION_OK ? (Color){ 126, 232, 152, 255 } : (Color){ 205, 160, 160, 255 }, width);

    if (!ui->fusionResultName[0]) return;

    /* Esito dell'ultima fusione: nome + immagine curata (DEC-171). Se
       l'immagine manca (pacchetto assente, file rimosso) resta il riquadro
       vuoto col nome: mai una schermata rotta. */
    int ry = hy + UiRound(22.0f*uiScale);
    float thumb = 44.0f*uiScale;
    Rectangle dst = { (float)x, (float)ry, thumb, thumb };
    /* W8: si riusa DrawItemIcon (la sola fonte della priorita' delle immagini)
       costruendo un Item minimo coi due riferimenti che 'ui' porta -- invece di
       ripetere qui la catena originale/curato/atlas, che sarebbe divergita al
       primo ritocco. Un Item azzerato con solo imageId/imagePath e' esattamente
       cio' che quella funzione legge. */
    Item resultIcon;
    memset(&resultIcon, 0, sizeof(resultIcon));
    snprintf(resultIcon.imageId, sizeof(resultIcon.imageId), "%s", ui->fusionResultImageId);
    snprintf(resultIcon.imagePath, sizeof(resultIcon.imagePath), "%s", ui->fusionResultImage);
    DrawItemIcon(game, &resultIcon, (Vector2){ dst.x + thumb*0.5f, dst.y + thumb*0.5f }, thumb);
    DrawRectangleLinesEx(dst, 1.0f, GameColorWithAlpha(accent, 160));
    int textX = x + UiRound(54.0f*uiScale);
    int textW = x + width - textX;
    DrawTextClipped(ui->fusionResultName, textX, ry + UiRound(2.0f*uiScale), UiRound(15.0f*uiScale), RAYWHITE, textW);
    DrawTextClipped(ui->fusionMessage, textX, ry + UiRound(22.0f*uiScale), UiRound(13.0f*uiScale), (Color){ 176, 184, 198, 255 }, textW);
}

/* W9 (playtest round 1, copertura mouse totale): geometria della colonna
   sinistra di BuildScreen -- la parte che conta per il hit-test del mouse
   sulla lista OGGETTI PRESI (RendererBuildItemRowAt sotto). Fattorizzata fuori
   da DrawBuildScreenOverlay perche' e' l'UNICA fonte di questa geometria, sia
   per disegnare le righe sia per sapere quale riga c'e' sotto il puntatore --
   stesso principio di MenuBoxForMode/MenuItemRect (vedi il commento li'
   sopra): duplicarla avrebbe fatto disallineare "cosa si vede" da "cosa si
   clicca" al primo ritocco di uno dei due lati. Chiama DrawBuildBlock in
   modalita' SOLA MISURA (measureOnly=true): l'altezza del blocco BUILD dipende
   dal numero di sinergie attive, quindi anche il punto in cui comincia la
   lista OGGETTI PRESI e' dinamico -- non si puo' indovinarlo senza rifare
   la stessa misura che fa il disegno vero.
   W9 correzione round 1 (BOCCIATO): la finestra visibile ('first') dipende SOLO
   da 'ui->buildItemScroll', MAI da 'ui->buildItemFocus' -- vedi il commento su
   quel campo in game_types.h. Con la vecchia formula "first = focus - maxShow +
   1" lo slot inferiore era l'unico punto fisso della mappatura punto->riga, e
   l'hover del mouse (che scrive il focus) faceva scorrere la lista da sola di
   uno step per ogni frame di movimento. Chi muove il focus tiene l'ancora
   allineata da src/app/app.c (AppBuildScrollFollowFocus), che per sapere
   quante righe stanno nella finestra chiama RendererBuildItemRowsVisible qui
   sotto: STESSA misura, mai una copia. */
static void BuildScreenItemListLayoutFor(Game *game, const AppUi *ui, float sw, float sh,
                                          int *outInnerX, int *outLeftW, int *outLy, int *outBandY,
                                          int *outRowStep, int *outFirst, int *outMaxShow, int *outCount)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = MenuBoxForModeFor(APP_BUILD_SCREEN, sw, sh, false);
    int innerX = (int)box.x + UiRound(40.0f*uiScale);
    int innerY = (int)box.y + UiRound(52.0f*uiScale);
    int innerW = (int)box.width - UiRound(80.0f*uiScale);
    int leftW = (int)(innerW*0.56f);
    int rowStep = UiRound(64.0f*uiScale);
    int bandY = (int)(box.y + box.height) - UiRound(FUSION_BAND_H_BASE*uiScale) - UiRound(14.0f*uiScale);

    int ly = innerY;
    int buildH = DrawBuildBlock(game, innerX, ly, leftW, uiScale, true);   /* measureOnly: nessun disegno */
    ly += buildH + UiRound(12.0f*uiScale);
    ly += UiRound(28.0f*uiScale);   /* l'etichetta "OGGETTI PRESI" */

    int count = GameMathClampInt(game->player.itemCount, 0, MAX_ITEMS);
    int maxShow = (bandY - ly)/rowStep;
    if (maxShow < 1) maxShow = 1;
    int first = ui->buildItemScroll;
    if (first > count - maxShow) first = count - maxShow;
    if (first < 0) first = 0;

    *outInnerX = innerX; *outLeftW = leftW; *outLy = ly; *outBandY = bandY;
    *outRowStep = rowStep; *outFirst = first; *outMaxShow = maxShow; *outCount = count;
}

/* Zona cliccabile della riga di un oggetto posseduto nella lista OGGETTI PRESI
   di BuildScreen (DEC-057/DEC-143: selezionare le due sorgenti della fusione
   e' gia' un'azione di menu ammessa al mouse) -- indice dentro Player.items[],
   o -1 se il punto non cade su nessuna riga visibile (finestra scorrevole:
   solo le righe DAVVERO disegnate in questo momento sono cliccabili, esattamente
   quelle che il giocatore vede). */
/* Quante righe della lista OGGETTI PRESI stanno DAVVERO nella finestra
   visibile, con la geometria corrente (dipende dall'altezza della finestra e
   dal numero di sinergie attive, che spingono giu' l'inizio della lista) --
   l'unico dato di questa geometria che src/app/app.c deve conoscere per tenere
   l'ancora di scorrimento ('AppUi.buildItemScroll') allineata al focus.
   Sempre >= 1. Non dipende dall'ancora ne' dal focus, quindi 'ui' non serve. */
int RendererBuildItemRowsVisible(Game *game)
{
    AppUi probe = { 0 };   /* solo per la firma: la misura non legge nessun campo di scorrimento */
    int innerX, leftW, ly, bandY, rowStep, first, maxShow, count;
    BuildScreenItemListLayoutFor(game, &probe, UiCanvasW(), UiCanvasH(),
                                  &innerX, &leftW, &ly, &bandY, &rowStep, &first, &maxShow, &count);
    (void)innerX; (void)leftW; (void)ly; (void)bandY; (void)rowStep; (void)first; (void)count;
    return maxShow > 0 ? maxShow : 1;
}

int RendererBuildItemRowAt(Game *game, const AppUi *ui, Vector2 mouse)
{
    float sw = UiCanvasW();
    float sh = UiCanvasH();
    float uiScale = UI_CANVAS_SCALE;
    int innerX, leftW, ly, bandY, rowStep, first, maxShow, count;
    BuildScreenItemListLayoutFor(game, ui, sw, sh, &innerX, &leftW, &ly, &bandY, &rowStep, &first, &maxShow, &count);
    (void)bandY;
    for (int i = first; i < count && i - first < maxShow; i++)
    {
        int rowY = ly + (i - first)*rowStep;
        Rectangle row = { (float)innerX, (float)rowY, (float)leftW, 58.0f*uiScale };
        if (CheckCollisionPointRec(mouse, row)) return i;
    }
    return -1;
}

/* Zona cliccabile della riga di conferma della fascia FUSIONE: un click qui
   equivale a [F] (W9 correzione round 1 -- vedi FusionConfirmRowRectFor sopra).
   Geometricamente indipendente dallo scorrimento della lista: la fascia sta
   sempre in fondo al riquadro, quindi 'ui' non serve al chiamante. */
bool RendererFusionConfirmAt(Game *game, Vector2 mouse)
{
    AppUi probe = { 0 };   /* la posizione della fascia non dipende da nessun campo di 'ui' */
    float uiScale = UI_CANVAS_SCALE;
    int innerX, leftW, ly, bandY, rowStep, first, maxShow, count;
    BuildScreenItemListLayoutFor(game, &probe, UiCanvasW(), UiCanvasH(),
                                  &innerX, &leftW, &ly, &bandY, &rowStep, &first, &maxShow, &count);
    (void)ly; (void)rowStep; (void)first; (void)maxShow; (void)count;
    return CheckCollisionPointRec(mouse, FusionConfirmRowRectFor(innerX, bandY, leftW, uiScale));
}

static void DrawBuildScreenOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = BeginMenuOverlay(APP_BUILD_SCREEN, game, "BUILD E SINERGIE", game->theme.accent2);
    const Player *p = &game->player;
    const Item *hoveredItem = NULL;   /* il tooltip va disegnato per ULTIMO, sopra tutto */

    /* DEC-137: la lista oggetti, l'anteprima del piano e le statistiche estese
       non stanno piu' sempre in una colonna laterale sul gioco -- vivono qui, in
       questo overlay centrale (il "pannello build" di DEC-137), che l'HUD di
       gioco rimanda con la sua riga compatta. Due colonne: a sinistra la build
       vera (colpo + sinergie + oggetti presi), a destra le statistiche e cosa
       offre il piano corrente. Stesse fonti dati del vecchio pannello GIOCATORE. */
    int innerX, leftW, ly, bandY, rowStep, first, maxShow, count;
    BuildScreenItemListLayoutFor(game, ui, UiCanvasW(), UiCanvasH(),
                                  &innerX, &leftW, &ly, &bandY, &rowStep, &first, &maxShow, &count);
    int innerY = (int)box.y + UiRound(52.0f*uiScale);
    int innerW = (int)box.width - UiRound(80.0f*uiScale);
    int gap = UiRound(28.0f*uiScale);
    int rightX = innerX + leftW + gap;
    int rightW = innerW - leftW - gap;

    /* Colonna sinistra: colpo + sinergie, poi gli oggetti presi (disegnate SUL
       SERIO stavolta, measureOnly=false -- BuildScreenItemListLayoutFor sopra
       ha gia' fatto la stessa chiamata solo per misurare). */
    DrawBuildBlock(game, innerX, innerY, leftW, uiScale, false);
    UiText("OGGETTI PRESI", innerX, ly - UiRound(28.0f*uiScale), UiRound(16.0f*uiScale), game->theme.accent2);
    if (p->itemCount == 0) UiText("Nessun oggetto ancora.", innerX, ly, UiRound(14.0f*uiScale), (Color){ 150, 158, 172, 255 });
    else
    {
        /* Finestra scorrevole: la riga a fuoco resta sempre visibile anche
           con piu' oggetti di quanti ne stiano nel riquadro (l'inventario
           arriva a MAX_ITEMS, la finestra ne mostra 3-4). Senza, selezionare
           il decimo oggetto sarebbe impossibile a occhio. 'first'/'maxShow'
           vengono gia' calcolati da BuildScreenItemListLayoutFor sopra. */
        int focus = GameMathClampInt(ui->buildItemFocus, 0, count - 1);
        for (int i = first; i < count && i - first < maxShow; i++)
        {
            int rowY = ly + (i - first)*rowStep;
            if (DrawItemPreview(game, &p->items[i], innerX, rowY, leftW, true, uiScale)) hoveredItem = &p->items[i];
            DrawFusionRowMark(ui, i, focus, innerX, rowY, leftW, uiScale, game->theme.accent2);
        }
    }

    /* Colonna destra: statistiche estese (le stesse righe del vecchio pannello
       GIOCATORE, con DrawStatLine) e cuori, poi cosa puo' offrire il piano. */
    int ry = innerY;
    UiText("PERSONAGGIO", rightX, ry, UiRound(16.0f*uiScale), game->theme.accent2);
    ry += UiRound(26.0f*uiScale);
    DrawHearts(p, rightX, ry, uiScale);
    ry += UiRound(30.0f*uiScale);
    const char *statLabels[6];
    char statValues[6][16];
    HudStatRowsFill(p, statLabels, statValues);
    for (int i = 0; i < 6; i++)
    {
        /* Fortuna (indice 5) resta evidenziata in verde come prima di DEC-184:
           e' l'unica delle sei che il giocatore legge come "buono/cattivo"
           col segno, le altre sono grandezze neutre. */
        Color color = (i == 5) ? (Color){ 126, 232, 152, 255 } : RAYWHITE;
        DrawStatLine(statLabels[i], statValues[i], rightX, ry + UiRound((float)(22*i)*uiScale), color, uiScale);
    }
    DrawStatLine("Risorse", TextFormat("%dc  %db  %dk", p->coins, p->bombs, p->keys), rightX, ry + UiRound(132.0f*uiScale), GOLD, uiScale);
    /* WP16 (DEC-042, ui/inventory-and-synergy-screen.md "Prove"): riga
       dedicata, pattern delle righe statistiche esistenti sopra -- il
       dettaglio per prova (testo + stato) vive nel pannello di PauseMenu
       (DrawTrialsPanel), qui solo il riepilogo "N/M superate, +X punti"
       (TrialsPassedCount/TrialsBonusTotal, unica fonte del conteggio: le due
       schermate non possono mai divergere). Visibile solo da quando le prove
       sono state presentate (game->trialCount > 0): nel Piano 0, dove questa
       schermata e' comunque raggiungibile solo dal crogiolo di fusione
       inesistente li', il conteggio e' zero e la riga resta silenziosa. */
    if (game->trialCount > 0)
        DrawStatLine("Prove", TextFormat("%d/%d, +%d", TrialsPassedCount(game), TrialsCountedTotal(game), TrialsBonusTotal(game)),
                     rightX, ry + UiRound(154.0f*uiScale), (Color){ 126, 232, 152, 255 }, uiScale);
    ry += UiRound(184.0f*uiScale);

    int floorIndex = GameMathClampInt(game->floor - 1, 0, FLOOR_COUNT - 1);
    UiText("OGGETTI DEL PIANO", rightX, ry, UiRound(16.0f*uiScale), game->theme.accent2);
    ry += UiRound(28.0f*uiScale);
    /* Le righe del piano si fermano sopra la riga "Indietro" (la fascia
       FUSIONE occupa solo la colonna sinistra, quindi non le toglie spazio). */
    int floorRows = ((int)(box.y + box.height) - UiRound(56.0f*uiScale) - ry)/rowStep;
    if (floorRows > 3) floorRows = 3;
    for (int i = 0; i < floorRows; i++)
        if (DrawItemPreview(game, &game->content.floors[floorIndex].items[i], rightX, ry + i*rowStep, rightW, false, uiScale))
            hoveredItem = &game->content.floors[floorIndex].items[i];

    DrawFusionBand(game, ui, innerX, bandY, leftW, uiScale);
    DrawMenuRow(APP_BUILD_SCREEN, 0, "Indietro", ui->focus, game->theme.accent2);
    /* Il tooltip per ultimo, sopra tutto (come nel vecchio pannello). */
    if (hoveredItem) DrawItemTooltip(hoveredItem, uiScale);
}

static void DrawRunResultsOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    const char *title = (game->phase == PHASE_WIN) ? "VITTORIA UFFICIALE" : "SCONFITTA";
    Rectangle box = BeginMenuOverlay(APP_RUN_RESULTS, game, title, game->theme.accent2);
    const char *outcome = (game->phase == PHASE_WIN)
        ? "Boss del piano 5 sconfitto."
        : "La run e' finita qui.";
    UiText(outcome, (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(56.0f*uiScale), UiRound(16.0f*uiScale), game->theme.accent2);
    UiText(TextFormat("Piano raggiunto: %d / %d", game->floor, FLOOR_COUNT), (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(80.0f*uiScale), UiRound(15.0f*uiScale), (Color){ 205, 210, 220, 255 });
    /* WP1 (DEC-051/DEC-056, ui/results-and-leaderboards.md riga "Tempo e piano
       raggiunto | Sempre"): il tempo finale, sia a vittoria che a sconfitta --
       stesso game->runElapsedSeconds gia' accumulato durante PHASE_PLAY in una
       run vera (game.c), mai ricalcolato qui. Stesso formato m:ss dell'HUD
       (DrawHudCanvas). Testo senza accentate ne' parentesi PER SCELTA di
       questo giro di lavoro, non piu' per limite tecnico: il font pixel ora
       supporta le sei maiuscole accentate italiane piu' comuni e le
       parentesi tonde (WP-INT, ArtSheetGlyphExt/glyphs_ext, art_draw.h),
       known-issues.md voce 10 punto 1 chiude questa parte -- ma riscrivere i
       testi esistenti e' un giro contenuti a parte, fuori scope qui. */
    int runMinutes = (int)game->runElapsedSeconds / 60;
    int runSeconds = (int)game->runElapsedSeconds % 60;
    UiText(TextFormat("Tempo: %d:%02d", runMinutes, runSeconds), (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(102.0f*uiScale), UiRound(15.0f*uiScale), (Color){ 205, 210, 220, 255 });
    /* DEC-159: la causa della sconfitta, SOLO a game over (mai a vittoria: li'
       game->deathCause resta la stringa vuota dello zero-default, scritta
       unicamente da CombatDamagePlayer). Riga indipendente da quella del
       catalogo sotto: 'lineY' avanza SOLO per le righe davvero disegnate, cosi'
       le due righe facoltative non si sovrappongono ne' lasciano un buco
       quando una delle due manca. Parte da 124 (non piu' 102): la riga Tempo
       sopra ha gia' preso la quota 102. */
    float lineY = 124.0f;
    if (game->phase == PHASE_GAME_OVER && game->deathCause[0])
    {
        UiText(TextFormat("Causa: %s.", game->deathCause), (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(lineY*uiScale), UiRound(14.0f*uiScale), (Color){ 205, 210, 220, 255 });
        lineY += 22.0f;
    }
    /* WP19 (DEC-082/089): l'abbandono confermato di una run vera chiude come
       sconfitta (il titolo sopra segue gia' "phase != PHASE_WIN", e phase
       resta PHASE_PLAY qui: mai un vero game over) ma con una causa DISTINTA
       dal colpo letale di DEC-159 -- game->deathCause resta vuota per
       costruzione su questo percorso (CombatDamagePlayer non viene mai
       chiamato durante un abbandono), quindi questa riga e quella sopra non
       si disegnano mai insieme. */
    if (game->runAbandoned)
    {
        UiText("Causa: abbandono volontario.", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(lineY*uiScale), UiRound(14.0f*uiScale), (Color){ 205, 210, 220, 255 });
        lineY += 22.0f;
    }
    /* M7 (DEC-015/041/045/069, substrato del catalogo): il feedback canonico
       "se sono stati registrati nuovi contenuti nel catalogo"
       (05-game-states-and-flow.md, righe 83-85). game->catalogRecordsWritten
       e' 0 (riga OMESSA, mai "0" a schermo, spec M7 punto 4) per una run
       fallback, per una run senza nulla di nuovo da registrare, o quando
       AppWriteRunCatalog non e' mai stata chiamata per questa run (il caso
       "0" di GameResetRun, invariato finche' non arriva PHASE_WIN/GAME_OVER). */
    if (game->catalogRecordsWritten > 0)
    {
        UiText(TextFormat("Creazioni registrate nel catalogo: %d", game->catalogRecordsWritten),
                 (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(lineY*uiScale), UiRound(14.0f*uiScale), game->theme.accent2);
        lineY += 22.0f;
    }
    /* WP16 (DEC-042/DEC-027): il bonus delle prove specifiche, riga dedicata
       come richiesto dal work package -- TrialsFinalizeAtRunEnd (chiamata da
       CombatDamagePlayer/CombatPickup non appena game->phase diventa
       terminale, oppure -- WP19, src/app/app.c, case APP_EXIT_CONFIRM -- a
       mano quando il giocatore conferma l'abbandono di una run vera, dove
       game->phase resta PHASE_PLAY -- prima che questo overlay possa mai
       disegnarsi) ha gia' risolto ogni prova rimasta in corso, quindi qui il
       conteggio e' gia' DEFINITIVO. Omessa (mai "0/0") quando non c'e' NULLA da contare --
       stessa disciplina della riga del catalogo sopra. La guardia e' sul
       DENOMINATORE stampato (TrialsCountedTotal), non su trialCount: con
       tutte le prove assegnate finite TRIAL_VOID (raro ma possibile, es.
       due prove fra segreta/arena/a tempo mai generate) trialCount resta
       positivo ma il conteggio e' 0/0 -- e la riga deve tacere.
       Limite dichiarato: questo E' "il punteggio che RunResults mostra oggi"
       per le prove (nessun punteggio composito DEC-060 esiste ancora, vedi
       rewards-and-economy.md); il bonus non alimenta ancora il canale punti
       sblocco di DEC-027 (nessun sistema di punti sblocco esiste ancora nel
       motore, gap dichiarato in save-and-meta-progression.md). */
    if (TrialsCountedTotal(game) > 0)
        UiText(TextFormat("Prove superate: %d/%d, +%d punti", TrialsPassedCount(game), TrialsCountedTotal(game), TrialsBonusTotal(game)),
                 (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(lineY*uiScale), UiRound(14.0f*uiScale), game->theme.accent2);
    DrawMenuRow(APP_RUN_RESULTS, 0, "Nuova run subito", ui->focus, game->theme.accent2);
    DrawMenuRow(APP_RUN_RESULTS, 1, "Menu principale", ui->focus, game->theme.accent2);
}

/* Dichiarata qui perche' DrawExitConfirmOverlay (subito sotto) la usa e la
   definizione sta piu' in basso, insieme al resto del Catalogo che l'ha
   introdotta (M5/DEC-005): spostarla avrebbe mosso decine di righe senza
   guadagno. */
static int WrapTextLines(const char *text, int fontSize, float maxWidth, char out[][160], int maxLines);

/* Quante righe al massimo puo' occupare la domanda di ExitConfirm, e con che
   passo verticale (WP22, terza passata). Le due voci ("Conferma"/"Annulla")
   cominciano a MENU_ROW_START_Y_BASE = 110: partendo da 52, tre righe da 20
   con glifi alti 15 (font-5px a UiFontScale(16)=3) finiscono a 107, sotto la
   soglia. Le domande vere ne occupano al massimo due (misure nel commento di
   MenuBoxForModeFor); la terza e' il margine di sicurezza per un testo futuro
   piu' lungo -- oltre, WrapTextLines tronca con "..." invece di sconfinare
   sulle voci. */
#define EXIT_CONFIRM_QUESTION_Y_BASE 52.0f
#define EXIT_CONFIRM_QUESTION_STEP_BASE 20.0f
#define EXIT_CONFIRM_QUESTION_MAX_LINES 3

static void DrawExitConfirmOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    /* WP22 (DEC-090, gap G9): il contesto "MainMenu -> Esci" (chiusura del
       gioco) e' uno dei DUE (dei CINQUE, vedi 'question' sotto) in cui
       openedFrom vale APP_MAIN_MENU -- exitAbandonsRun resta sempre falso li'
       (vedi il case APP_MAIN_MENU in src/app/app.c), e WP17 aggiunge l'altro
       ("Nuova run" con una run sospesa), che ExitConfirmIsLightModalFor
       distingue dal secondo parametro 'exitDropsSuspendedRun': DEC-090
       riserva il dialogo leggero alla sola chiusura del gioco. Il case APP_EXIT_CONFIRM in
       RendererDrawApp ha gia' ridisegnato il MainMenu SOTTO in questo stesso
       frame quando la condizione e' vera: qui serve solo il velo di fondo piu'
       leggero (non un altro schermo pieno che lo cancellerebbe), e il riquadro
       piu' stretto che 'lightModal' seleziona in MenuBoxForModeFor. */
    bool lightModal = ExitConfirmIsLightModalFor(ui->openedFrom, ui->exitDropsSuspendedRun);
    Rectangle box = lightModal
        ? BeginMenuOverlayLight(APP_EXIT_CONFIRM, game, "CONFERMA", game->theme.accent2)
        : BeginMenuOverlay(APP_EXIT_CONFIRM, game, "CONFERMA", game->theme.accent2);
    /* Contesti distinti (DEC-057 + M1b + WP21/DEC-114 + WP17/DEC-050), tutti
       derivati da 'ui': MainMenu/Esci ha tutti e tre i booleani di contesto
       falsi; i due abbandoni (Piano 0/
       run vera) hanno exitAbandonsRun vero e si distinguono da ui->openedFrom
       (chi ha aperto ExitConfirm, gia' scritto da UpdateApp prima del cambio
       di stato); il reroll ("Rigenera la run" di PauseMenu) ha invece
       exitRerollsRun vero -- controllato PER PRIMO perche' i due booleani
       sono mutuamente esclusivi per costruzione (vedi il commento su
       AppUi.exitRerollsRun in core/game_types.h), quindi l'ordine qui non
       cambia il risultato ma rispecchia quello del ramo "Conferma" in
       src/app/app.c (case APP_EXIT_CONFIRM). */
    /* WP17 (DEC-050) porta i contesti a CINQUE: si aggiunge "Nuova run con una
       run sospesa" (ui/main-menu.md, riga "Nuova run"), controllato per primo
       come il reroll -- i tre booleani restano mutuamente esclusivi per
       costruzione, quindi l'ordine non cambia il risultato. */
    const char *question = ui->exitDropsSuspendedRun
        ? "Iniziare una nuova run? La run sospesa verra' cancellata."
        : (ui->exitRerollsRun
            ? "Rigenerare la run con un nuovo seed? Il progresso non salvato si perde."
            : (!ui->exitAbandonsRun
                ? "Uscire dal gioco?"
                : (ui->openedFrom == APP_FLOOR_ZERO
                    ? "Abbandonare la preparazione? La generazione in corso verra' annullata."
                    : "Abbandonare la run in corso? Il progresso non salvato si perde.")));
    /* WP22, terza passata: la domanda va A CAPO dentro il pannello invece di
       essere disegnata come una riga sola. Prima sconfinava SEMPRE, in tutti i
       contesti a schermo pieno (765/849/864 px di testo contro i 520 di spazio
       utile di un box da 600, misurati col font reale): il testo usciva dal
       riquadro e, su finestre strette, veniva pure tagliato dal bordo dello
       schermo. Stesso WrapTextLines gia' usato dal Catalogo e dai blurb del
       Piano 0, stesso margine di 40 per lato del testo di prima. */
    int questionFont = UiRound(16.0f*uiScale);
    char lines[EXIT_CONFIRM_QUESTION_MAX_LINES][160];
    int lineCount = WrapTextLines(question, questionFont, box.width - 80.0f*uiScale, lines, EXIT_CONFIRM_QUESTION_MAX_LINES);
    for (int i = 0; i < lineCount; i++)
    {
        UiText(lines[i], (int)box.x + UiRound(40.0f*uiScale),
               (int)box.y + UiRound((EXIT_CONFIRM_QUESTION_Y_BASE + (float)i*EXIT_CONFIRM_QUESTION_STEP_BASE)*uiScale),
               questionFont, (Color){ 205, 210, 220, 255 });
    }
    /* Le due voci seguono la STESSA geometria del pannello appena disegnato
       (stretta nel dialogo leggero, larga negli altri tre contesti): e' la
       stessa che RendererMenuItemAt riceve da src/app/app.c per il hit-test
       del mouse, mai una seconda copia. */
    DrawMenuRowCtx(APP_EXIT_CONFIRM, 0, "Conferma", ui->focus, RED, lightModal);
    DrawMenuRowCtx(APP_EXIT_CONFIRM, 1, "Annulla", ui->focus, game->theme.accent2, lightModal);
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
    int tw = UiTextW(text, font);
    Rectangle box = { gameRect.x + gameRect.width*0.5f - ((float)tw*0.5f + 16.0f*uiScale), gameRect.y + 40.0f*uiScale, (float)tw + 32.0f*uiScale, 30.0f*uiScale };
    DrawRectangleRec(box, (Color){ 16, 18, 24, 205 });
    DrawRectangleLinesEx(box, 1.5f, (Color){ 150, 158, 172, 200 });
    UiText(text, (int)box.x + UiRound(16.0f*uiScale), (int)box.y + UiRound(7.0f*uiScale), font, RAYWHITE);
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

        if (UiTextW(candidate, fontSize) <= (int)maxWidth || !line[0])
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
        while (len > 0 && UiTextW(TextFormat("%.*s...", (int)len, original), fontSize) > (int)maxWidth) len--;
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
    float uiScale = UI_CANVAS_SCALE;
    float w = 760.0f*uiScale;
    float h = 520.0f*uiScale;
    return (Rectangle){ sw*0.5f - w*0.5f, sh*0.5f - h*0.5f, w, h };
}

static Rectangle BeginCatalogOverlay(Game *game, const char *title, Color accent)
{
    Rectangle box = CatalogBoxFor(UiCanvasW(), UiCanvasH());
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
        UiText(label, (int)tab.x + UiRound(6.0f*uiScale), (int)tab.y + UiRound(6.0f*uiScale), font, isActive ? RAYWHITE : (Color){ 190, 196, 206, 255 });
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
        UiText("Nessuna voce in questa categoria.", (int)box.x + UiRound(24.0f*uiScale), (int)listTop,
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
        UiText(label, (int)row.x + UiRound(8.0f*uiScale), (int)row.y + UiRound(5.0f*uiScale),
                 UiRound(13.0f*uiScale), hasFocus ? RAYWHITE : (Color){ 200, 206, 216, 255 });
    }

    if (count > visibleMax)
    {
        char pos[24];
        snprintf(pos, sizeof(pos), "%d/%d", focus + 1, count);
        UiText(pos, (int)(box.x + 20.0f*uiScale + listW - UiRound(40.0f*uiScale)), (int)(listTop - UiRound(16.0f*uiScale)),
                 UiRound(11.0f*uiScale), (Color){ 150, 158, 172, 255 });
    }
    if (cat->overflowCount[active] > 0)
    {
        char more[48];
        snprintf(more, sizeof(more), "-- e altre %d", cat->overflowCount[active]);
        UiText(more, (int)box.x + UiRound(20.0f*uiScale), (int)(listTop + (float)visibleMax*rowH + 2.0f*uiScale),
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
    UiText(e->name, (int)detailX, (int)listTop, nameFont, accent);

    int lineY = (int)listTop + UiRound(26.0f*uiScale);
    if (e->detail[0])
    {
        int detailFont = UiRound(12.0f*uiScale);
        char lines[6][160];
        int n = WrapTextLines(e->detail, detailFont, detailW, lines, 6);
        for (int l = 0; l < n; l++)
        {
            UiText(lines[l], (int)detailX, lineY, detailFont, (Color){ 205, 210, 220, 255 });
            lineY += UiRound(16.0f*uiScale);
        }
    }
    lineY += UiRound(6.0f*uiScale);
    UiText(TextFormat("Incontri: %d  --  Run: %d", e->encounterCount, e->runCount),
             (int)detailX, lineY, UiRound(12.0f*uiScale), (Color){ 176, 184, 198, 255 });
}

static void DrawCatalogOverlay(Game *game, const AppUi *ui)
{
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = BeginCatalogOverlay(game, "CATALOGO", game->theme.accent2);
    const RunCatalogSummary *cat = &ui->catalog;

    int totalEntries = 0;
    for (int c = 0; c < RUN_CATALOG_CATEGORY_COUNT; c++) totalEntries += cat->entryCount[c];
    if (totalEntries == 0)
    {
        /* Catalogo vuoto (spec M8): un messaggio sobrio, MAI un errore -- vale
           per l'intera vista (nessuna categoria ha nulla da mostrare, quindi
           niente tabs/lista/dettaglio vuoti a fare da rumore). */
        UiText("Il crogiolo non ricorda ancora nulla: gioca una run.",
                 (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(70.0f*uiScale),
                 UiRound(16.0f*uiScale), (Color){ 205, 210, 220, 255 });
        UiText("ESC -- torna al menu.", (int)box.x + UiRound(40.0f*uiScale), (int)box.y + UiRound(98.0f*uiScale),
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

    UiText("Sinistra/destra: categoria -- Su/giu': voce -- ESC: torna al menu",
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
    float uiScale = UI_CANVAS_SCALE;
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

/* Geometria di UNA delle due schedine MONDI/PERSONAGGI (indice 0/1), PRIMA del
 * sollevamento visivo che la sezione attiva riceve in DrawFloorZeroSectionTabs
 * -- fattorizzata (W9, playtest round 1) perche' RendererFloorZeroSectionTabAt
 * (hit-test del mouse) deve interrogare la STESSA area cliccabile che si vede,
 * mai una copia a parte. */
static Rectangle FloorZeroSectionTabRectFor(Rectangle box, int s, float uiScale)
{
    float pad = 22.0f*uiScale;
    float tabY = box.y + 8.0f*uiScale;
    float tabH = 22.0f*uiScale;
    if (s == 0) return (Rectangle){ box.x + pad, tabY, 150.0f*uiScale, tabH };
    return (Rectangle){ box.x + pad + 158.0f*uiScale, tabY, 170.0f*uiScale, tabH };
}

/* M6a, requisito 3: le due schedine "MONDI"/"PERSONAGGI" in cima al pannello
 * combinato -- dicono quale sezione ha il focus (su/giu' da tastiera, click
 * diretto col mouse da W9, RendererFloorZeroSectionTabAt). Come il focus di
 * una carta (DEC-058), MAI il solo colore: la sezione attiva ha un bordo piu'
 * spesso, e' leggermente sollevata (stesso trucco di "scala" delle carte, qui
 * verticale) e porta lo stesso piccolo triangolo puntato verso il basso, sopra
 * le carte della sua sezione. */
static void DrawFloorZeroSectionTabs(Rectangle box, int section, float uiScale, Color accent)
{
    const char *labels[2] = { "MONDI", "PERSONAGGI" };
    for (int s = 0; s < 2; s++)
    {
        bool active = (s == section);
        Rectangle tab = FloorZeroSectionTabRectFor(box, s, uiScale);
        if (active) { tab.y -= 3.0f*uiScale; tab.height += 3.0f*uiScale; }
        DrawRectangleRec(tab, active ? GameColorWithAlpha(accent, 50) : GameColorWithAlpha(BLACK, 120));
        DrawRectangleLinesEx(tab, active ? 2.5f : 1.0f, active ? accent : GameColorWithAlpha(accent, 130));
        int font = UiRound(13.0f*uiScale);
        UiText(labels[s], (int)tab.x + UiRound(10.0f*uiScale), (int)tab.y + UiRound(4.0f*uiScale), font,
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
        UiText(proposal->name, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(10.0f*uiScale),
                 titleFont, focused ? RAYWHITE : (Color){ 205, 210, 220, 255 });
        if (selected)
            UiText("SCELTO", (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(30.0f*uiScale),
                     UiRound(11.0f*uiScale), GOLD);

        char lines[4][160];
        int wrapY = selected ? 48 : 34;
        int n = WrapTextLines(proposal->blurb, blurbFont, card.width - 20.0f*uiScale, lines, 4);
        int ly = (int)card.y + UiRound((float)wrapY*uiScale);
        int lineStep = UiRound(16.0f*uiScale);
        for (int l = 0; l < n; l++)
            UiText(lines[l], (int)card.x + UiRound(10.0f*uiScale), ly + l*lineStep, blurbFont, (Color){ 190, 196, 206, 255 });
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
        UiText(c->name, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(10.0f*uiScale),
                 nameFont, focused ? RAYWHITE : (Color){ 205, 210, 220, 255 });
        UiText(c->role, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(30.0f*uiScale),
                 roleFont, game->theme.accent2);
        if (selected)
            UiText("SCELTO", (int)card.x + UiRound(10.0f*uiScale), (int)card.y + UiRound(48.0f*uiScale),
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
            UiText(traitText, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + blurbOffset,
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
            UiText(shotText, (int)card.x + UiRound(10.0f*uiScale), (int)card.y + blurbOffset,
                     UiRound(11.0f*uiScale), game->theme.accent2);
            blurbOffset += UiRound(15.0f*uiScale);
        }

        char lines[3][160];
        int n = WrapTextLines(c->blurb, blurbFont, card.width - 20.0f*uiScale, lines, 3);
        int ly = (int)card.y + blurbOffset;
        int lineStep = UiRound(15.0f*uiScale);
        for (int l = 0; l < n; l++)
            UiText(lines[l], (int)card.x + UiRound(10.0f*uiScale), ly + l*lineStep, blurbFont, (Color){ 190, 196, 206, 255 });

        /* Statistiche chiave in piccola tabella (requisito 3), ancorate al
           fondo della carta cosi' restano leggibili a qualunque numero di
           righe di blurb sopra. */
        int statsY = (int)(card.y + card.height - 40.0f*uiScale);
        UiText(TextFormat("DMG %.0f", c->baseDamage), (int)card.x + UiRound(10.0f*uiScale), statsY, statFont, RAYWHITE);
        UiText(TextFormat("SPD %.0f", c->baseSpeed), (int)card.x + UiRound(10.0f*uiScale), statsY + UiRound(16.0f*uiScale), statFont, RAYWHITE);
        UiText(TextFormat("HP %d/%d", c->baseMaxHp, c->hpCap), (int)card.x + UiRound(10.0f*uiScale), statsY + UiRound(32.0f*uiScale), statFont, RAYWHITE);
    }
}

/* Il pannello COMBINATO MONDI/PERSONAGGI (M5 requisito 9 + M6a requisito 3):
 * TAB lo apre/chiude (src/app/app.c), su/giu' cambia sezione, sinistra/
 * destra sposta il focus, conferma sceglie. W9 (playtest round 1, DEC-075):
 * il mouse ora ci entra anche lui -- hover sposta il focus, click sceglie,
 * esattamente come una voce di menu (RendererFloorZeroCardAt/
 * RendererFloorZeroSectionTabAt/RendererFloorZeroHintChipAt, sotto). A
 * differenza di M5, questo pannello NON smette di disegnare nulla dopo la
 * scelta del mondo: la sezione PERSONAGGI resta viva per tutta la
 * permanenza nel Piano 0 (requisito 1). Il riepilogo persistente lo fa
 * comunque DrawFloorZeroSummary sotto, per chi vuole lo stato SENZA aprire
 * il pannello. */
/* Il fumetto "TAB o click -- mondo e personaggio" mostrato quando il pannello e'
 * chiuso -- fattorizzato (W9) perche' RendererFloorZeroHintChipAt deve poterlo
 * aprire anche con un click, sulla STESSA area che si vede. */
static Rectangle FloorZeroHintChipRectFor(float sw, float sh)
{
    /* 'sh' non serve piu' da DEC-200: la quota del fumetto era proporzionale
       all'altezza dello schermo, ora e' una quota fissa del canvas. Il
       parametro resta per non spezzare la simmetria con le altre *For (tutte
       (sw, sh)) e con i loro chiamanti/test. */
    (void)sh;
    float uiScale = UI_CANVAS_SCALE;
    const char *hint = "TAB o click -- mondo e personaggio";
    int font = UiRound(14.0f*uiScale);
    int tw = UiTextW(hint, font);
    return (Rectangle){ sw*0.5f - ((float)tw*0.5f + 14.0f*uiScale), 80.0f*uiScale, (float)tw + 28.0f*uiScale, 26.0f*uiScale };
}

static void DrawFloorZeroPanel(const Game *game, float sw, float sh)
{
    if (game->themeCardCount <= 0) return;
    float uiScale = UI_CANVAS_SCALE;

    if (!game->themeCardsPanelOpen)
    {
        /* Pannello chiuso: solo un invito discreto, mai un riquadro vuoto --
           il messaggio stabile "in attesa della scelta del mondo" (vedi
           AppFloorZeroStatusText) gia' dice CHE COSA manca prima della
           scelta; dopo, questo resta comunque l'invito a riaprire per
           cambiare personaggio (requisito 1: sempre modificabile). Cliccabile
           da W9 (RendererFloorZeroHintChipAt): un click qui apre il pannello
           esattamente come TAB. */
        const char *hint = "TAB o click -- mondo e personaggio";
        int font = UiRound(14.0f*uiScale);
        Rectangle box = FloorZeroHintChipRectFor(sw, sh);
        DrawRectangleRec(box, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(box, 1.5f, (Color){ 150, 158, 172, 180 });
        UiText(hint, (int)box.x + UiRound(14.0f*uiScale), (int)box.y + UiRound(6.0f*uiScale), font, (Color){ 205, 210, 220, 255 });
        return;
    }

    DrawRectangle(0, 0, (int)sw, (int)sh, GameColorWithAlpha(BLACK, 170));
    Rectangle box = ThemeCardsPanelBoxFor(sw, sh);
    DrawRectangleRec(box, (Color){ 18, 20, 27, 235 });
    DrawRectangleLinesEx(box, 2.0f, game->theme.accent2);
    DrawFloorZeroSectionTabs(box, game->floorZeroPanelSection, uiScale, game->theme.accent2);

    const char *title = (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS)
                         ? "Scegli il mondo -- click o sinistra/destra, conferma (su/giu': personaggio)"
                         : "Scegli il personaggio -- sinistra/destra, conferma (su/giu': mondo)";
    UiText(title, (int)box.x + UiRound(20.0f*uiScale), (int)box.y + UiRound(42.0f*uiScale),
             UiRound(13.0f*uiScale), (Color){ 205, 210, 220, 255 });

    /* La geometria delle carte usa la stessa 'box' del titolo (ThemeCardRectFor
       misura dal bordo del pannello, non dalla riga del titolo): entrambe le
       sezioni condividono lo stesso riquadro, se ne disegna una sola alla
       volta -- e' quella col focus (game->floorZeroPanelSection) a decidere
       quale. */
    if (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS) DrawWorldCards(game, box, uiScale);
    else DrawCharacterCards(game, box, uiScale);
}

/* Zona cliccabile di una carta della SEZIONE ATTIVA del pannello combinato
 * (DEC-075: il Piano 0 conta come menu ai fini del mouse) -- stessa geometria
 * di ThemeCardRectFor/DrawWorldCards/DrawCharacterCards, mai duplicata: il
 * chiamante (UpdateApp) non conosce il layout, sa solo "quale carta sotto il
 * mouse" e decide da solo, con game->floorZeroPanelSection, se e' una carta-
 * mondo o una carta-personaggio. Ritorna -1 se il pannello e' chiuso, se il
 * punto non cade su nessuna carta, o se la sezione attiva non ha ancora
 * carte da mostrare. */
int RendererFloorZeroCardAt(const Game *game, Vector2 mouse)
{
    if (!game->themeCardsPanelOpen) return -1;
    float sw = UiCanvasW();
    float sh = UiCanvasH();
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = ThemeCardsPanelBoxFor(sw, sh);
    int count = (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS)
                ? game->themeCardCount : GameCharacterCardCount(game);
    if (count <= 0) return -1;
    for (int i = 0; i < count; i++)
        if (CheckCollisionPointRec(mouse, ThemeCardRectFor(box, i, count, uiScale))) return i;
    return -1;
}

/* Zona cliccabile delle due schedine MONDI/PERSONAGGI: click = cambia
 * sezione, la stessa azione di su/giu' da tastiera. -1 se il pannello e'
 * chiuso o il punto non cade su nessuna delle due. */
int RendererFloorZeroSectionTabAt(const Game *game, Vector2 mouse)
{
    if (!game->themeCardsPanelOpen) return -1;
    float sw = UiCanvasW();
    float sh = UiCanvasH();
    float uiScale = UI_CANVAS_SCALE;
    Rectangle box = ThemeCardsPanelBoxFor(sw, sh);
    for (int s = 0; s < 2; s++)
        if (CheckCollisionPointRec(mouse, FloorZeroSectionTabRectFor(box, s, uiScale))) return s;
    return -1;
}

/* Zona cliccabile del fumetto "TAB o click -- mondo e personaggio" mostrato quando il
 * pannello e' chiuso: un click qui lo apre, come TAB. Falso se il pannello e'
 * gia' aperto o se le carte non sono ancora pronte (il fumetto stesso non si
 * disegna in quel caso, vedi DrawFloorZeroPanel). */
bool RendererFloorZeroHintChipAt(const Game *game, Vector2 mouse)
{
    if (game->themeCardCount <= 0 || game->themeCardsPanelOpen) return false;
    Rectangle box = FloorZeroHintChipRectFor(UiCanvasW(), UiCanvasH());
    return CheckCollisionPointRec(mouse, box);
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
        int tw = UiTextW(text, font);
        Rectangle box = { gameRect.x + 12.0f*uiScale, y, (float)tw + 24.0f*uiScale, 26.0f*uiScale };
        DrawRectangleRec(box, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(box, 1.5f, game->theme.accent2);
        UiText(text, (int)box.x + UiRound(12.0f*uiScale), (int)box.y + UiRound(6.0f*uiScale), font, RAYWHITE);
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
        int ctw = UiTextW(ctext, font);
        Rectangle cbox = { gameRect.x + 12.0f*uiScale, y, (float)ctw + 24.0f*uiScale, 26.0f*uiScale };
        DrawRectangleRec(cbox, (Color){ 16, 18, 24, 190 });
        DrawRectangleLinesEx(cbox, 1.5f, character->palette);
        UiText(ctext, (int)cbox.x + UiRound(12.0f*uiScale), (int)cbox.y + UiRound(6.0f*uiScale), font, RAYWHITE);
    }
}

void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, const AppUi *ui,
                     bool takeScreenshot, const GenProgress *genProgress, const char *screenshotPath)
{
    /* DEC-200 (WP-UI-0): il frame INTERO -- scena, HUD e overlay delle
       schermate -- si compone dentro il canvas 640x360. Fino a WP-UI-0 solo la
       scena stava qui e tutto il resto si disegnava dopo, nel framebuffer di
       finestra, a coordinate di finestra: due spazi diversi nello stesso
       frame. Da qui in poi la finestra riceve UNA sola cosa, il canvas
       ingrandito di un fattore intero. */
    UiLayout canvasLayout = UiCanvasLayout();
    BeginTextureMode(canvas);
    DrawGameplayCanvas(game);
    /* W8: l'HUD in pixel art vive DENTRO il canvas (DEC-174). La regola di
       visibilita' resta HudCombatShouldDraw, una sola: e' la stessa condizione
       che governa il ripiego a primitive qui accanto -- i due percorsi si
       escludono a vicenda (ArtUiReady), mai entrambi nello stesso frame. */
    bool hudVisible = HudCombatShouldDraw(mode, game->floorZeroTrialActive);
    bool hudPixelArt = ArtUiReady();
    /* HUD di gioco SOLO in Gameplay, o nel Piano 0 durante una prova (DEC-169):
       fuori da queste due situazioni e' nascosto (ui/hud.md), e il Piano 0 ha
       i suoi overlay dedicati (riepilogo + carte) sullo stesso angolo -- vedi
       il case APP_FLOOR_ZERO sotto. */
    if (hudVisible && hudPixelArt) DrawHudCanvas(game, ui);
    else if (hudVisible) DrawOuterUi(game, canvasLayout);

    /* UN overlay per stato (switch esplicito, M1a): 'ui' e' NULL solo per
       Gameplay (che non ne ha bisogno) e per FloorZero (che legge
       genProgress, non ui). */
    switch (mode)
    {
        /* M8 (DEC-045): la vista Catalogo sostituisce il disegno del menu
           quando aperta -- nessun nuovo AppMode, il case resta uno solo. */
        case APP_MAIN_MENU: if (ui->catalogOpen) DrawCatalogOverlay(game, ui); else DrawMainMenuOverlay(game, ui->focus, true, RendererMainMenuHasContinueRow(ui)); break;
        case APP_RUN_SETUP: DrawRunSetupOverlay(game, ui); break;
        case APP_FLOOR_ZERO:
            DrawFloorZeroIndicator(canvasLayout.gameRect, canvasLayout.uiScale, genProgress);
            /* M6a: il pannello combinato (aperto o solo l'invito, a seconda
               di game->themeCardsPanelOpen) e il riepilogo persistente
               convivono SEMPRE, a differenza di M5 -- il riepilogo mostra
               anche il personaggio (sempre definito) anche col pannello
               chiuso, il pannello resta apribile anche dopo la scelta del
               mondo (requisito 1: il personaggio resta modificabile). */
            DrawFloorZeroPanel(game, UiCanvasW(), UiCanvasH());
            DrawFloorZeroSummary(game, canvasLayout.gameRect, canvasLayout.uiScale);
            break;
        case APP_GAMEPLAY: break;
        case APP_PAUSE_MENU: DrawPauseMenuOverlay(game, ui); break;
        case APP_OPTIONS: DrawOptionsOverlay(game, ui); break;
        case APP_BUILD_SCREEN: DrawBuildScreenOverlay(game, ui); break;
        case APP_RUN_RESULTS: DrawRunResultsOverlay(game, ui); break;
        case APP_EXIT_CONFIRM:
            /* WP22 (DEC-090, gap G9 ui-cornice, seconda passata): il dialogo
               "MainMenu -> Esci" e' un dialogo modale LEGGERO -- il menu resta
               visibile/leggibile dietro, non sostituito come gli altri TRE
               contesti di ExitConfirm (abbandono dal Piano 0, abbandono di
               una run da PauseMenu, reroll di WP21/DEC-114: tutti e tre
               restano a schermo pieno). Si passa 'ui->returnFocus' (la riga del
               MainMenu su cui il giocatore stava davvero, salvata da
               UpdateApp prima del cambio di stato -- src/app/app.c, righe
               864/896) invece di 'ui->focus' (che apparterrebbe alla
               schermata sbagliata, 0/1 "Conferma"/"Annulla"): NESSUNA copia
               di AppUi (correzione della revisione precedente, che duplicava
               l'intera struct -- 337280 byte misurati -- solo per questo
               campo). 'dimBackground=false': il MainMenu qui sotto non
               disegna il proprio velo scuro a schermo pieno, cosi' il velo
               unico applicato resta quello (piu' leggero) di
               DrawExitConfirmOverlay subito sotto -- mai due veli sommati. */
            if (ExitConfirmIsLightModalFor(ui->openedFrom, ui->exitDropsSuspendedRun))
                DrawMainMenuOverlay(game, ui->returnFocus, false, RendererMainMenuHasContinueRow(ui));
            DrawExitConfirmOverlay(game, ui);
            break;
    }
    EndTextureMode();

    BeginDrawing();
    /* Letterbox slag-nero (UI_GROUND, il fondo assoluto della palette Fucina):
       riempie quel che avanza quando il monitor non e' 16:9, o quando la
       finestra non e' un multiplo esatto di 640x360. Non e' spazio riservato a
       nulla -- e' fuori dal gioco. */
    ClearBackground(UI_GROUND);
    UiLayout layout = UiComputeLayout();
    /* L'altezza NEGATIVA della sorgente ribalta la texture: il framebuffer
       OpenGL e' memorizzato dal basso verso l'alto. */
    Rectangle src = { 0.0f, 0.0f, (float)canvas.texture.width, -(float)canvas.texture.height };
    DrawTexturePro(canvas.texture, src, layout.gameRect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing();

    /* screenshotPath e' del chiamante (mai NULL quando takeScreenshot e'
       vero, vedi game_renderer.h): --screenshot-test continua a scrivere
       logs/melting-run-screen.png esattamente come prima (vedi app.c),
       questo parametro serve solo a chi (come --layer-test, vedi
       src/tests/game_tests.c) vuole un frame catturato altrove, senza
       toccare quel file.
       DEC-200: si cattura il CANVAS, non piu' il backbuffer (TakeScreenshot).
       Tre ragioni. (1) Il canvas e' il frame VERO: il backbuffer aggiunge il
       letterbox e la scala della finestra del momento, cioe' rumore che
       cambia da una macchina all'altra (sotto Xvfb erano 1920x1080 con bande).
       (2) Cosi' uno scatto e' pixel-esatto e confrontabile con quello di ieri.
       (3) Il raddoppio a punto (x2, 1280x720) e' li' solo perche' un PNG
       640x360 e' scomodo da guardare a occhio: NON aggiunge informazione, ogni
       pixel diventa un quadrato 2x2 esatto. Dopo EndDrawing per non forzare un
       flush a meta' frame. */
    if (takeScreenshot && screenshotPath)
    {
        Image shot = LoadImageFromTexture(canvas.texture);
        /* Stesso ribaltamento del blit qui sopra, per la stessa ragione. */
        ImageFlipVertical(&shot);
        ImageResizeNN(&shot, shot.width*2, shot.height*2);
        ExportImage(shot, screenshotPath);
        UnloadImage(shot);
    }
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

        /* (a) il canvas sta dentro lo schermo ed e' CENTRATO. La centratura e'
           cio' che garantisce bande di letterbox uguali sui due lati quando il
           monitor non e' un multiplo esatto di 640x360. */
        if (!UiRectInside(screen, L.gameRect))
        {
            fprintf(stderr, "UiLayoutSelfTest: (a) la game view esce dallo schermo a %.0fx%.0f\n", sw, sh);
            return false;
        }
        if (fabsf(L.gameRect.x - floorf((sw - L.gameRect.width)*0.5f)) > UI_LAYOUT_TEST_EPS ||
            fabsf(L.gameRect.y - floorf((sh - L.gameRect.height)*0.5f)) > UI_LAYOUT_TEST_EPS)
        {
            fprintf(stderr, "UiLayoutSelfTest: (a) game view non centrata a %.0fx%.0f\n", sw, sh);
            return false;
        }

        /* (b) DEC-200: la scala e' un INTERO, ed e' il PIU' GRANDE che ci sta.
           Due verifiche in una, perche' sono la stessa promessa: se fosse
           frazionaria i pixel del canvas cadrebbero a distanze irregolari, se
           non fosse la massima il gioco lascerebbe bande evitabili. Sotto il
           minimo 1 il secondo vincolo non si applica: quello e' un pavimento
           imposto, non "il massimo che ci sta". */
        if (fabsf(L.gameScale - floorf(L.gameScale)) > 0.0001f)
        {
            fprintf(stderr, "UiLayoutSelfTest: (b) scala non intera a %.0fx%.0f (%.3f)\n", sw, sh, L.gameScale);
            return false;
        }
        float nextScale = L.gameScale + 1.0f;
        bool nextFits = (nextScale*(float)SCREEN_WIDTH <= sw + UI_LAYOUT_TEST_EPS) &&
                        (nextScale*(float)SCREEN_HEIGHT <= sh + UI_LAYOUT_TEST_EPS);
        if (nextFits && L.gameScale > 1.0f + 0.001f)
        {
            fprintf(stderr, "UiLayoutSelfTest: (b) bande evitabili a %.0fx%.0f (scale %.3f, il passo successivo ci stava)\n", sw, sh, L.gameScale);
            return false;
        }

        /* (d) monotonia: la lista e' gia' ordinata per risoluzione crescente
           (larghezza E altezza mai decrescenti da una riga alla successiva) --
           gameRect (in area) e uiScale non devono MAI restringersi. Da DEC-200
           uiScale e' costante, quindi il suo lato del confronto passa sempre:
           resta perche' il giorno in cui tornasse a variare l'invariante deve
           valere anche per lui. */
        float area = L.gameRect.width*L.gameRect.height;
        if (i > 0 && (area < prevArea - UI_LAYOUT_TEST_EPS || L.uiScale < prevUiScale - 0.0001f))
        {
            fprintf(stderr, "UiLayoutSelfTest: (d) canvas/uiScale rimpiccioliti a %.0fx%.0f rispetto alla risoluzione precedente\n", sw, sh);
            return false;
        }
        prevArea = area;
        prevUiScale = L.uiScale;

    }

    /* (e) geometria dei menu: ogni voce dentro il proprio box, nessuna
       sovrapposizione fra voci consecutive dello stesso menu.
       DEC-200: FUORI dal giro sulle risoluzioni, dove stava fino a WP-UI-0.
       Gli overlay non vivono piu' in coordinate di finestra ma sul canvas
       (640x360), quindi ripetere queste verifiche per ogni risoluzione
       sintetica non proverebbe piu' nulla -- darebbe sette volte lo stesso
       risultato. Il fatto che la geometria NON dipenda piu' dalla finestra e'
       proprio cio' che la migrazione ha comprato.
       WP22 (terza passata): il giro interno 'lite' ripete la verifica sulla
       geometria RISTRETTA del dialogo leggero (ExitConfirm da MainMenu),
       che dalla terza passata e' un box diverso -- senza, meta' della
       geometria di ExitConfirm resterebbe fuori da --layout-test. Gli altri
       overlay non hanno una seconda geometria: per loro il secondo giro
       sarebbe una copia identica, quindi si salta. */
    {
        float sw = UiCanvasW(), sh = UiCanvasH();
        Rectangle canvasRect = { 0.0f, 0.0f, sw, sh };
        for (int m = 0; m < (int)(sizeof(kMenuModes)/sizeof(kMenuModes[0])); m++)
        {
            AppMode mode = kMenuModes[m];
            for (int lite = 0; lite < 2; lite++)
            {
                if (lite == 1 && mode != APP_EXIT_CONFIRM) continue;
                bool light = (lite == 1);
                Rectangle box = MenuBoxForModeFor(mode, sw, sh, light);
                /* (e0) DEC-200: il riquadro sta dentro il CANVAS. E' la
                   verifica nata con la migrazione: le quote degli overlay sono
                   scritte nella griglia 1600x900 e riportate a 640x360 da
                   UI_CANVAS_SCALE -- se qualcuno aggiungesse una riga a
                   PauseMenu (il piu' alto, 560 di griglia) senza rifare i
                   conti, il pannello sfonderebbe il canvas invece di uscire
                   dalla finestra, che nessuno vedrebbe in un test. */
                if (!UiRectInside(canvasRect, box))
                {
                    fprintf(stderr, "UiLayoutSelfTest: (e0) il box del menu %d (leggero=%d) esce dal canvas %.0fx%.0f\n", (int)mode, lite, sw, sh);
                    return false;
                }
                /* WP17 (DEC-050): il giro 'cond' ripete la verifica col menu
                   nella sua forma PIU' LUNGA -- MainMenu con "Continua" (5
                   voci) e PauseMenu con "Sospendi e esci" (7) -- perche' e' la
                   forma che rischia di sfondare il box, non quella corta.
                   Senza questo giro una voce condizionale fuori dal riquadro
                   resterebbe invisibile a make test. */
                for (int cond = 0; cond < 2; cond++)
                {
                    RendererMenuCtx ctx = { 0 };
                    ctx.exitConfirmLight = light;
                    ctx.mainMenuContinue = (cond == 1);
                    ctx.pauseSuspend = (cond == 1);
                    int count = MenuItemCountForMode(mode, ctx);
                    Rectangle prevItem = { 0 };
                    for (int idx = 0; idx < count; idx++)
                    {
                        Rectangle item = MenuItemRectFor(mode, idx, sw, sh, light);
                        if (!UiRectInside(box, item))
                        {
                            fprintf(stderr, "UiLayoutSelfTest: (e) voce %d del menu %d (leggero=%d, condizionali=%d) fuori dal box a %.0fx%.0f\n", idx, (int)mode, lite, cond, sw, sh);
                            return false;
                        }
                        if (idx > 0 && UiRectOverlap(prevItem, item))
                        {
                            fprintf(stderr, "UiLayoutSelfTest: (e) voci %d/%d del menu %d (leggero=%d, condizionali=%d) sovrapposte a %.0fx%.0f\n", idx - 1, idx, (int)mode, lite, cond, sw, sh);
                            return false;
                        }
                        prevItem = item;
                    }
                }
            }
        }

        /* (g) WP22 (terza passata): le DUE geometrie di ExitConfirm restano
           quelle che devono essere.
           - a schermo pieno (i tre contesti che DEC-090 vuole invariati:
             abbandono dal Piano 0, abbandono di una run in corso, reroll di
             DEC-114) il box e' quello STANDARD delle schermate ancora sulla
             griglia storica -- si confronta con RunSetup, non piu' con
             MainMenu: WP-UI-0 ha dato al MainMenu una geometria propria in
             pixel di canvas, quindi "uguale a MainMenu" ha smesso di essere
             la formulazione giusta dell'invariante (la sostanza no: la
             seconda passata di WP22 aveva stretto ANCHE i tre contesti a
             schermo pieno, e la domanda sconfinava dal pannello);
           - nel dialogo leggero e' strettamente piu' stretto SIA dello
             standard sia del riquadro del MainMenu che gli sta dietro, ed e'
             quello a rendere possibile la "leggibilita' dietro" di DEC-090. */
        Rectangle mainBox = MenuBoxForModeFor(APP_MAIN_MENU, sw, sh, false);
        Rectangle standardBox = MenuBoxForModeFor(APP_RUN_SETUP, sw, sh, false);
        Rectangle exitFull = MenuBoxForModeFor(APP_EXIT_CONFIRM, sw, sh, false);
        Rectangle exitLight = MenuBoxForModeFor(APP_EXIT_CONFIRM, sw, sh, true);
        if (fabsf(exitFull.width - standardBox.width) > UI_LAYOUT_TEST_EPS)
        {
            fprintf(stderr, "UiLayoutSelfTest: (g) il box di ExitConfirm a schermo pieno (%.1f) non e' piu' quello standard (%.1f) -- i tre contesti a schermo pieno devono restare invariati (DEC-090)\n",
                    exitFull.width, standardBox.width);
            return false;
        }
        if (!(exitLight.width < exitFull.width - UI_LAYOUT_TEST_EPS) ||
            !(exitLight.width < mainBox.width - UI_LAYOUT_TEST_EPS))
        {
            fprintf(stderr, "UiLayoutSelfTest: (g) il box del dialogo leggero ExitConfirm (%.1f) non e' piu' stretto sia dello standard (%.1f) sia del MainMenu (%.1f) (WP22, DEC-090)\n",
                    exitLight.width, exitFull.width, mainBox.width);
            return false;
        }
    }

    /* (c) tre riferimenti CONGELATI, scritti a mano e non ricalcolati con la
       stessa formula della funzione sotto esame (che non proverebbe nulla):
       - 1600x900, la finestra grande del progetto (APP_WINDOW_WIDTH/HEIGHT):
         min(1600/640, 900/360) = 2.5, quindi scala 2 e bande di 320x180 in
         totale -- il caso "monitor non multiplo", quello che il letterbox
         serve a coprire;
       - 1920x1080, il caso che DEC-200 cita come motivo della scelta: x3
         ESATTO, zero bande;
       - 1280x720: x2 esatto, la risoluzione dei mock approvati.
       uiScale e' costante da DEC-200 (UI_CANVAS_SCALE): se tornasse a
       dipendere dalla finestra, queste tre righe se ne accorgerebbero. */
    {
        static const float kRefW[] = { 1600.0f, 1920.0f, 1280.0f };
        static const float kRefH[] = {  900.0f, 1080.0f,  720.0f };
        static const float kRefScale[] = { 2.0f, 3.0f, 2.0f };
        for (int r = 0; r < 3; r++)
        {
            UiLayout L = UiComputeLayoutFor(kRefW[r], kRefH[r]);
            if (fabsf(L.uiScale - UI_CANVAS_SCALE) > 0.0001f)
            {
                fprintf(stderr, "UiLayoutSelfTest: (c) uiScale a %.0fx%.0f e' %.4f, atteso %.4f\n", kRefW[r], kRefH[r], L.uiScale, UI_CANVAS_SCALE);
                return false;
            }
            if (fabsf(L.gameScale - kRefScale[r]) > 0.001f)
            {
                fprintf(stderr, "UiLayoutSelfTest: (c) gameScale a %.0fx%.0f e' %.3f, atteso %.0f\n", kRefW[r], kRefH[r], L.gameScale, kRefScale[r]);
                return false;
            }
        }
    }

    /* (f) WP22 (DEC-090, gap G9 ui-cornice): ExitConfirmIsLightModalFor resta
       vera SOLO per APP_MAIN_MENU (chiusura del gioco) -- gli altri TRE
       contesti che possono aprire ExitConfirm (abbandono della preparazione
       nel Piano 0, abbandono di una run in corso da PauseMenu, rigenerazione
       della run di WP21/DEC-114, che parte anch'essa da PauseMenu) restano a
       schermo pieno, presentazione gia' documentata da DEC-090 stesso.
       WP17 (DEC-050) aggiunge il QUINTO contesto, "Nuova run con una run
       sospesa": nasce da APP_MAIN_MENU come la chiusura del gioco, ma non e'
       la chiusura del gioco -- deve restare a schermo pieno anche lui. */
    if (!ExitConfirmIsLightModalFor(APP_MAIN_MENU, false))
    {
        fprintf(stderr, "UiLayoutSelfTest: (f) ExitConfirm da MainMenu non e' riconosciuto come dialogo leggero (DEC-090)\n");
        return false;
    }
    if (ExitConfirmIsLightModalFor(APP_MAIN_MENU, true))
    {
        fprintf(stderr, "UiLayoutSelfTest: (f) ExitConfirm da MainMenu per la rinuncia a una run sospesa e' stato marcato leggero (WP17: deve restare a schermo pieno)\n");
        return false;
    }
    if (ExitConfirmIsLightModalFor(APP_FLOOR_ZERO, false))
    {
        fprintf(stderr, "UiLayoutSelfTest: (f) ExitConfirm da FloorZero e' stato marcato leggero (deve restare a schermo pieno)\n");
        return false;
    }
    if (ExitConfirmIsLightModalFor(APP_PAUSE_MENU, false))
    {
        fprintf(stderr, "UiLayoutSelfTest: (f) ExitConfirm da PauseMenu e' stato marcato leggero (deve restare a schermo pieno)\n");
        return false;
    }

    /* (h) WP22 (terza passata, ui/run-setup.md): la riga informativa
       "Modalita'" di RunSetup non deve toccare NESSUNA delle tre voci
       selezionabili -- fino alla seconda passata cadeva a 142, dentro la
       fascia della voce "Seed" (110..150). Nucleo puro, stessa geometria che
       DrawRunSetupOverlay usa per disegnarla (RendererRunSetupModeLabelBand).
       Un solo giro, non piu' uno per risoluzione: come per la voce (e), da
       DEC-200 questa geometria vive sul canvas e non dipende dalla finestra. */
    {
        float sw = UiCanvasW(), sh = UiCanvasH();
        Rectangle band = RendererRunSetupModeLabelBand();
        Rectangle box = MenuBoxForModeFor(APP_RUN_SETUP, sw, sh, false);
        if (!UiRectInside(box, band))
        {
            fprintf(stderr, "UiLayoutSelfTest: (h) la riga 'Modalita' di RunSetup esce dal box\n");
            return false;
        }
        RendererMenuCtx runSetupCtx = { 0 };   /* RunSetup non ha voci condizionali: il contesto azzerato e' il suo */
        for (int idx = 0; idx < MenuItemCountForMode(APP_RUN_SETUP, runSetupCtx); idx++)
        {
            if (UiRectOverlap(band, MenuItemRectFor(APP_RUN_SETUP, idx, sw, sh, false)))
            {
                fprintf(stderr, "UiLayoutSelfTest: (h) la riga 'Modalita' di RunSetup si sovrappone alla voce %d\n", idx);
                return false;
            }
        }
    }

    return true;
}

/* W9 (playtest round 1, "mouse ovunque"): hit-test delle geometrie che
 * RendererMenuItemAt/UiLayoutSelfTest sopra NON coprono -- righe oggetti di
 * BuildScreen, carte/schedine/fumetto del pannello combinato del Piano 0
 * (DEC-075), barre trascinabili di Opzioni. A differenza di UiLayoutSelfTest,
 * gira DOPO InitWindow (--mouse-hit-test, come --rooms-test/--fusion-test):
 * BuildScreenItemListLayoutFor misura il blocco BUILD con DrawBuildBlock
 * (measureOnly), che chiama UiTextW/MeasureText -- serve il font di default
 * gia' caricato da raylib, quindi non puo' girare PRIMA della finestra come
 * il self-test puramente matematico sopra.
 * Stile "scansione a griglia" invece di ricalcolare a mano le formule di
 * game_renderer.c: verifica che OGNI indice atteso sia raggiungibile da
 * qualche punto dello schermo e che un punto lontano da tutto ritorni -1/false
 * -- cosi' il test resta valido anche se le costanti di layout cambiano,
 * l'importante e' che "cosa si vede" e "cosa si clicca" restino la stessa
 * cosa (la garanzia che tutta questa fetta di file esiste per dare). 'game'
 * e' quello gia' pronto passato da AppRun (GameResetRun gia' chiamata), stesso
 * schema di GameRoomsTest/GameFusionTest. */
bool RendererMouseHitTestSelfTest(Game *game)
{
    float sw = UiCanvasW();
    float sh = UiCanvasH();

    /* (a) Opzioni: il valore lungo la barra e' clampato a [0,1] oltre i due
       estremi, e monotono (mai decrescente) muovendo il mouse da sinistra a
       destra -- il comportamento atteso di una barra trascinabile. */
    for (int idx = 0; idx < 3; idx++)
    {
        float lo = RendererOptionsSliderValueAt(idx, -100000.0f);
        float hi = RendererOptionsSliderValueAt(idx, 100000.0f);
        if (lo != 0.0f || hi != 1.0f)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (a) slider %d non clampa a [0,1] (lo=%.3f hi=%.3f)\n", idx, lo, hi);
            return false;
        }
        /* (a2) Il cancello del press (correzione di Fable): dentro la barra
           si aggancia, sull'etichetta della riga no -- il difetto originale
           era proprio un click sull'etichetta che azzerava il volume. */
        {
            Rectangle bar = OptionsSliderBarRectFor(idx, sw, sh);
            Vector2 inside = { bar.x + bar.width*0.5f, bar.y + bar.height*0.5f };
            Vector2 label = { bar.x - bar.width*0.5f, bar.y + bar.height*0.5f };
            if (!RendererOptionsSliderHit(idx, inside) || RendererOptionsSliderHit(idx, label))
            {
                fprintf(stderr, "RendererMouseHitTestSelfTest: (a2) slider %d: il cancello del press non separa barra ed etichetta\n", idx);
                return false;
            }
        }
        float prev = -1.0f;
        bool sawZero = false, sawOne = false;
        for (int step = 0; step <= 40; step++)
        {
            float x = -50.0f + ((float)step/40.0f)*(sw + 100.0f);
            float v = RendererOptionsSliderValueAt(idx, x);
            if (v < -0.0001f || v > 1.0001f)
            {
                fprintf(stderr, "RendererMouseHitTestSelfTest: (a) slider %d fuori [0,1] a x=%.1f (v=%.3f)\n", idx, x, v);
                return false;
            }
            if (v < prev - 0.0001f)
            {
                fprintf(stderr, "RendererMouseHitTestSelfTest: (a) slider %d non monotono a x=%.1f\n", idx, x);
                return false;
            }
            if (v <= 0.0001f) sawZero = true;
            if (v >= 0.9999f) sawOne = true;
            prev = v;
        }
        if (!sawZero || !sawOne)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (a) slider %d non raggiunge sia 0%% sia 100%%\n", idx);
            return false;
        }
    }

    /* (b) BuildScreen: ogni riga dell'inventario (0..count-1) e' raggiungibile
       da qualche punto dello schermo QUANDO l'ANCORA di scorrimento e' gia' li'
       vicino -- la lista e' una finestra SCORREVOLE (BuildScreenItemListLayoutFor:
       'first' viene da 'ui->buildItemScroll', esattamente come la barra di
       scorrimento di un inventario piu' lungo della finestra), quindi non tutte
       le righe sono disegnate INSIEME con un'ancora fissa -- si verifica riga
       per riga, ciascuna con la propria ancora (equivalente a "ci si e' gia'
       scorsi fin li'", che sia con su/giu' da tastiera o con la rotellina del
       mouse in UpdateApp). Il CONTENUTO degli oggetti non conta per questa
       geometria (BuildScreenItemListLayoutFor legge solo 'itemCount'), quindi
       non serve popolare Player.items[] davvero.
       W9 correzione round 1: si verifica ANCHE che la mappatura punto->riga sia
       INDIPENDENTE da 'buildItemFocus' -- e' la garanzia strutturale che rompe
       l'anello di retroazione dell'hover del mouse (l'hover scrive il focus; se
       la finestra dipendesse dal focus, la lista scorrerebbe da sola ad ogni
       frame di movimento del puntatore). */
    {
        int savedCount = game->player.itemCount;
        game->player.itemCount = 5;

        bool outOfRange = false;
        bool seen[5] = { false };
        for (int anchor = 0; anchor < 5; anchor++)
        {
            AppUi ui = { 0 };
            ui.buildItemScroll = anchor;
            for (float y = 0.0f; y < sh && !seen[anchor]; y += 4.0f)
            {
                for (float x = 0.0f; x < sw && !seen[anchor]; x += 8.0f)
                {
                    int row = RendererBuildItemRowAt(game, &ui, (Vector2){ x, y });
                    if (row < -1 || row >= 5) { outOfRange = true; continue; }
                    if (row == anchor) seen[anchor] = true;
                }
            }
        }
        AppUi farUi = { 0 };
        bool farOutside = (RendererBuildItemRowAt(game, &farUi, (Vector2){ -500.0f, -500.0f }) != -1);

        /* Indipendenza dal focus: stessa ancora, focus a ogni valore possibile
           -- la riga sotto un punto qualunque non deve cambiare mai. */
        bool focusLeaks = false;
        {
            AppUi refUi = { 0 };
            refUi.buildItemScroll = 1;
            for (int focusRow = 0; focusRow < 5 && !focusLeaks; focusRow++)
            {
                AppUi probe = refUi;
                probe.buildItemFocus = focusRow;
                for (float y = 0.0f; y < sh && !focusLeaks; y += 4.0f)
                    for (float x = 0.0f; x < sw && !focusLeaks; x += 8.0f)
                        if (RendererBuildItemRowAt(game, &probe, (Vector2){ x, y }) !=
                            RendererBuildItemRowAt(game, &refUi, (Vector2){ x, y })) focusLeaks = true;
            }
        }
        game->player.itemCount = savedCount;

        if (outOfRange)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (b) RendererBuildItemRowAt fuori range\n");
            return false;
        }
        for (int i = 0; i < 5; i++)
        {
            if (!seen[i])
            {
                fprintf(stderr, "RendererMouseHitTestSelfTest: (b) riga oggetto %d mai raggiunta dalla scansione (con l'ancora su di essa)\n", i);
                return false;
            }
        }
        if (farOutside)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (b) un punto fuori schermo colpisce comunque una riga\n");
            return false;
        }
        if (focusLeaks)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (b) la mappatura punto->riga dipende da buildItemFocus (anello di retroazione dell'hover, regressione W9)\n");
            return false;
        }
    }

    /* (b2) W9 correzione round 1: la riga di conferma della fascia FUSIONE e'
       raggiungibile col mouse (il solo percorso mouse per portare a termine una
       fusione) e NON si sovrappone ne' alle righe oggetto ne' alla riga
       "Indietro" -- un click li' non deve mai fare due cose insieme (fondere e
       uscire dalla schermata, o fondere e cambiare sorgente). Con la lista
       PIENA (MAX_ITEMS), il caso peggiore per la sovrapposizione. */
    {
        int savedCount = game->player.itemCount;
        game->player.itemCount = MAX_ITEMS;
        AppUi ui = { 0 };

        bool reachable = false, overlapRow = false, overlapBack = false;
        for (float y = 0.0f; y < sh; y += 2.0f)
            for (float x = 0.0f; x < sw; x += 4.0f)
            {
                Vector2 pt = { x, y };
                if (!RendererFusionConfirmAt(game, pt)) continue;
                reachable = true;
                if (RendererBuildItemRowAt(game, &ui, pt) >= 0) overlapRow = true;
                if (RendererMenuItemAt(APP_BUILD_SCREEN, pt, false) >= 0) overlapBack = true;
            }
        game->player.itemCount = savedCount;

        if (!reachable)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (b2) la riga di conferma della fusione non e' raggiungibile\n");
            return false;
        }
        if (overlapRow || overlapBack)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (b2) la riga di conferma della fusione si sovrappone a %s\n",
                    overlapRow ? "una riga oggetto" : "la riga Indietro");
            return false;
        }
    }

    /* (c) Pannello del Piano 0 (DEC-075): fumetto cliccabile SOLO a pannello
       chiuso, carte/schedine cliccabili SOLO a pannello aperto, ogni carta
       delle due sezioni (MONDI e PERSONAGGI) e le due schedine raggiungibili,
       e -1/falso ovunque quando il pannello e' chiuso o non ci sono carte. */
    {
        int savedCount = game->themeCardCount;
        bool savedOpen = game->themeCardsPanelOpen;
        int savedSection = game->floorZeroPanelSection;

        game->themeCardCount = 3;
        game->themeCardsPanelOpen = false;

        bool hintHit = false;
        for (float y = 0.0f; y < sh && !hintHit; y += 4.0f)
            for (float x = 0.0f; x < sw && !hintHit; x += 8.0f)
                if (RendererFloorZeroHintChipAt(game, (Vector2){ x, y })) hintHit = true;
        if (!hintHit)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (c) il fumetto TAB non e' raggiungibile a pannello chiuso\n");
            game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen; game->floorZeroPanelSection = savedSection;
            return false;
        }
        if (RendererFloorZeroCardAt(game, (Vector2){ sw*0.5f, sh*0.5f }) != -1 ||
            RendererFloorZeroSectionTabAt(game, (Vector2){ sw*0.5f, sh*0.5f }) != -1)
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (c) carte/schedine rispondono a pannello CHIUSO\n");
            game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen; game->floorZeroPanelSection = savedSection;
            return false;
        }

        game->themeCardsPanelOpen = true;
        if (RendererFloorZeroHintChipAt(game, (Vector2){ sw*0.5f, 90.0f }))
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (c) il fumetto TAB risponde a pannello APERTO\n");
            game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen; game->floorZeroPanelSection = savedSection;
            return false;
        }

        bool seenTab[2] = { false, false };
        for (float y = 0.0f; y < sh; y += 4.0f)
            for (float x = 0.0f; x < sw; x += 8.0f)
            {
                int tab = RendererFloorZeroSectionTabAt(game, (Vector2){ x, y });
                if (tab >= 0 && tab < 2) seenTab[tab] = true;
            }
        if (!seenTab[0] || !seenTab[1])
        {
            fprintf(stderr, "RendererMouseHitTestSelfTest: (c) una delle due schedine MONDI/PERSONAGGI non e' raggiungibile\n");
            game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen; game->floorZeroPanelSection = savedSection;
            return false;
        }

        static const int kSections[2] = { FLOOR_ZERO_PANEL_WORLDS, FLOOR_ZERO_PANEL_CHARACTERS };
        for (int s = 0; s < 2; s++)
        {
            game->floorZeroPanelSection = kSections[s];
            int count = (kSections[s] == FLOOR_ZERO_PANEL_WORLDS) ? game->themeCardCount : GameCharacterCardCount(game);
            if (count <= 0) continue;   /* la rosa dinamica puo' davvero essere vuota, niente da verificare */
            bool *seenCard = (bool *)calloc((size_t)count, sizeof(bool));
            for (float y = 0.0f; y < sh; y += 4.0f)
                for (float x = 0.0f; x < sw; x += 8.0f)
                {
                    int card = RendererFloorZeroCardAt(game, (Vector2){ x, y });
                    if (card >= 0 && card < count) seenCard[card] = true;
                }
            bool allSeen = true;
            for (int i = 0; i < count; i++) if (!seenCard[i]) allSeen = false;
            free(seenCard);
            if (!allSeen)
            {
                fprintf(stderr, "RendererMouseHitTestSelfTest: (c) una carta della sezione %d non e' raggiungibile\n", kSections[s]);
                game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen; game->floorZeroPanelSection = savedSection;
                return false;
            }
        }

        game->themeCardCount = savedCount;
        game->themeCardsPanelOpen = savedOpen;
        game->floorZeroPanelSection = savedSection;
    }

    return true;
}
