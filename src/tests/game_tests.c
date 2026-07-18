#include "tests/game_tests.h"

#include "app/app.h"
#include "app/app_internal.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "render/game_renderer.h"
#include "render/item_layers.h"
#include "script/script_api.h"
#include "script/script_items.h"
#include "script/script_sandbox.h"

#include <stdio.h>
#include <stdlib.h>
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

/* M1a: la macchina a stati canonica (9 stati, ui/navigation-map.md). Come
   GamePortalRespawnTest sopra, gira DOPO InitWindow (RendererMenuItemAt,
   letta da UpdateApp per il click del mouse, ha bisogno di
   GetScreenWidth/Height) ma chiama UpdateApp DIRETTAMENTE con AppInput
   sintetici costruiti a mano -- MAI IsKeyPressed, vedi il commento su
   UpdateApp in app_internal.h. 'game' e' quello gia' pronto passato da
   AppRun (GameResetRun gia' chiamata): gli scenari di fine run impostano
   game->phase a mano, esattamente come farebbe combat.c davvero.
   gen.enabled resta false per tutto il test (AppGen azzerato): i cammini con
   una generazione VERA restano fuori dal test sintetico (assunzione
   dichiarata nella spec M1a -- la pipeline bloccante e' gia' coperta
   indirettamente dall'equivalenza col comportamento pre-M1a). */
static AppInput InputNone(void)    { AppInput in = { 0 }; return in; }
static AppInput InputConfirm(void) { AppInput in = { 0 }; in.confirm = true; return in; }
static AppInput InputBack(void)    { AppInput in = { 0 }; in.back = true; return in; }
static AppInput InputDown(void)    { AppInput in = { 0 }; in.down = true; return in; }
static AppInput InputTab(void)     { AppInput in = { 0 }; in.tab = true; return in; }
static AppInput InputLeft(void)    { AppInput in = { 0 }; in.left = true; return in; }   /* M5: pannello di scelta del tema */
static AppInput InputRight(void)   { AppInput in = { 0 }; in.right = true; return in; }
static AppInput InputReroll(void)  { AppInput in = { 0 }; in.reroll = true; return in; }
static AppInput InputPause(void)   { AppInput in = { 0 }; in.pause = true; return in; }

#define STATES_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameStatesTest: %s\n", (msg)); return false; } } while (0)

/* M5 (DEC-005): un ingresso in FloorZero, con gen disabilitata (il caso di
 * TUTTO GameStatesTest, vedi il commento sotto), ha gia' le carte pronte
 * SUBITO (AppUseFallbackThemeCards, chiamata da AppEnterFloorZero): apre il
 * pannello (TAB) e conferma la carta col focus di default (indice 0) -- la
 * scelta sintetica richiesta dal requisito 13 della spec. */
static void ChooseFirstThemeCard(Game *game, AppMode *mode, AppGen *gen, AppUi *ui)
{
    { AppInput in = InputTab();     UpdateApp(game, mode, gen, ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, mode, gen, ui, &in); }
}

bool GameStatesTest(Game *game)
{
    AppGen gen = { 0 };
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;

    /* Un frame senza alcun evento: esercita il caso "niente e' successo", che
       deve lasciare tutto esattamente com'era. */
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "lo stato iniziale non e' MainMenu");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di MainMenu non e' 0 (Nuova run)");

    /* MainMenu -> RunSetup -> (back) -> MainMenu */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_SETUP, "MainMenu/confirm su 'Nuova run' non porta a RunSetup");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "RunSetup/back non torna a MainMenu");
    STATES_CHECK(ui.focus == 0, "il ritorno a MainMenu non ripristina il focus su 'Nuova run'");

    /* RunSetup reroll cambia il seed */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* di nuovo in RunSetup, con un nuovo seed proposto */
    STATES_CHECK(mode == APP_RUN_SETUP, "rientro in RunSetup fallito");
    unsigned int seedBeforeReroll = ui.seed;
    { AppInput in = InputReroll(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(ui.seed != seedBeforeReroll, "R in RunSetup non cambia il seed");
    STATES_CHECK(ui.focus == 0, "il reroll non deve spostare il focus da Seed");

    /* RunSetup -> Avvia -> FloorZero (M1b: si resta nella sala d'attesa
       finche' non si attraversa il varco, non piu' un salto diretto a
       Gameplay -- M5: con gen disabilitata le carte curate compaiono SUBITO,
       ma l'uscita si apre solo DOPO la scelta del tema, vedi
       AppEnterFloorZero/AppConfirmThemeChoice). */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Seed -> Avvia */
    STATES_CHECK(ui.focus == 1, "down da Seed non porta il focus su Avvia");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_FLOOR_ZERO, "RunSetup/Avvia non porta a FloorZero");
    /* M5 (DEC-005): con gen disabilitata le carte curate compaiono SUBITO
       (AppUseFallbackThemeCards), ma l'uscita resta chiusa finche' il tema
       non e' scelto -- gating "tema scelto E pipeline terminale", requisito
       10 della spec. */
    STATES_CHECK(game->themeCardCount > 0, "con gen disabilitata le carte di tema non sono pronte subito");
    STATES_CHECK(game->themeChosenIndex < 0, "un tema risulta gia' scelto prima di qualunque input");
    STATES_CHECK(!game->floorZeroExitOpen, "l'uscita del Piano 0 e' aperta prima della scelta del tema");
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    STATES_CHECK(game->themeChosenIndex == 0, "la scelta sintetica (TAB+confirm) non ha scelto la carta 0");
    STATES_CHECK(game->floorZeroExitOpen, "con gen disabilitata l'uscita del Piano 0 non si apre subito dopo la scelta");
    STATES_CHECK(game->floor == 0, "FloorZeroEnter non ha impostato floor a 0");

    /* ESC in FloorZero -> ExitConfirm (contesto "abbandona la preparazione")
       -> annulla -> di nuovo FloorZero. */
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "ESC in FloorZero non apre ExitConfirm");
    STATES_CHECK(ui.openedFrom == APP_FLOOR_ZERO, "ExitConfirm da FloorZero non ricorda openedFrom");
    STATES_CHECK(ui.exitAbandonsRun, "il contesto di ExitConfirm da FloorZero non e' 'abbandono'");
    STATES_CHECK(ui.focus == 1, "il focus iniziale di ExitConfirm da FloorZero non e' 1 (Annulla)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla */
    STATES_CHECK(mode == APP_FLOOR_ZERO, "ExitConfirm/Annulla da FloorZero non torna a FloorZero");

    /* di nuovo ExitConfirm, stavolta Conferma -> MainMenu */
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla -> Conferma */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "ExitConfirm/Conferma da FloorZero non torna a MainMenu");

    /* Si rientra in FloorZero per l'attraversamento (spec M1b: "l'uscita e'
       APERTA subito; l'attraversamento sintetico porta in APP_GAMEPLAY"). */
    STATES_CHECK(ui.focus == 0, "il ritorno a MainMenu dopo l'abbandono non ripristina il focus su 'Nuova run'");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* MainMenu -> RunSetup */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }      /* Seed -> Avvia */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Avvia -> FloorZero */
    STATES_CHECK(mode == APP_FLOOR_ZERO, "il secondo ingresso in FloorZero e' fallito");
    STATES_CHECK(!game->floorZeroExitOpen, "l'uscita del secondo ingresso e' aperta prima della scelta del tema");
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    STATES_CHECK(game->floorZeroExitOpen, "l'uscita del secondo ingresso in FloorZero non e' aperta dopo la scelta");

    /* Attraversamento sintetico (il flag lo scriverebbe WorldHandleTransitions
       quando il giocatore preme contro il varco aperto): UpdateApp lo consuma
       nel primo frame successivo, GameResetRun scatta SOLO ora. */
    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "l'attraversamento del varco non porta a Gameplay");
    STATES_CHECK(!game->floorZeroExitCrossed, "floorZeroExitCrossed non e' stato consumato");
    STATES_CHECK(game->phase == PHASE_PLAY, "l'ingresso in Gameplay via FloorZero non ha richiamato GameResetRun");

    /* Gameplay -> PauseMenu -> Options -> (back) -> PauseMenu, focus su "Opzioni" */
    { AppInput in = InputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "P in Gameplay non apre PauseMenu");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di PauseMenu non e' 0 (Riprendi)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Riprendi -> Build e sinergie */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Build e sinergie -> Opzioni */
    STATES_CHECK(ui.focus == 2, "due 'down' da Riprendi non arrivano su Opzioni (indice 2)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_OPTIONS, "confirm su Opzioni non apre Options");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "Options/back non torna a PauseMenu");
    STATES_CHECK(ui.focus == 2, "il ritorno da Options non ripristina il focus su Opzioni");

    /* PauseMenu -> BuildScreen -> (back) -> PauseMenu */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Opzioni -> Abbandona run */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Abbandona run -> Riprendi (giro completo) */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Riprendi -> Build e sinergie */
    STATES_CHECK(ui.focus == 1, "la navigazione circolare in PauseMenu non torna su Build e sinergie (indice 1)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_BUILD_SCREEN, "confirm su Build e sinergie non apre BuildScreen");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "BuildScreen/back non torna a PauseMenu");
    STATES_CHECK(ui.focus == 1, "il ritorno da BuildScreen non ripristina il focus su Build e sinergie");

    /* torna in Gameplay per il blocco successivo */
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Riprendi (via ESC/P, non serve il focus) */
    STATES_CHECK(mode == APP_GAMEPLAY, "il ritorno a Gameplay da PauseMenu e' fallito");

    /* Gameplay -> TAB -> BuildScreen -> (back) -> Gameplay */
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_BUILD_SCREEN, "TAB in Gameplay non apre BuildScreen");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "BuildScreen/back da Gameplay non torna a Gameplay");

    /* PauseMenu -> Abbandona run -> ExitConfirm -> (annulla) -> PauseMenu,
       poi di nuovo -> (conferma) -> MainMenu */
    { AppInput in = InputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* PauseMenu, focus su Riprendi */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Build e sinergie */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Opzioni */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Abbandona run */
    STATES_CHECK(ui.focus == 3, "la navigazione in PauseMenu non arriva su Abbandona run (indice 3)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "confirm su Abbandona run non apre ExitConfirm");
    STATES_CHECK(ui.exitAbandonsRun, "il contesto di ExitConfirm da PauseMenu non e' 'abbandono run'");
    STATES_CHECK(ui.focus == 1, "il focus iniziale di ExitConfirm non e' 1 (Annulla)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla */
    STATES_CHECK(mode == APP_PAUSE_MENU, "ExitConfirm/Annulla non torna a PauseMenu");
    STATES_CHECK(ui.focus == 3, "il ritorno da ExitConfirm/Annulla non ripristina il focus su Abbandona run");

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* di nuovo in ExitConfirm */
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "rientro in ExitConfirm fallito");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }      /* Annulla -> Conferma */
    STATES_CHECK(ui.focus == 0, "down da Annulla in ExitConfirm non porta a Conferma");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "ExitConfirm/Conferma (abbandono) non torna a MainMenu");

    /* MainMenu -> Esci -> ExitConfirm -> (conferma) -> UpdateApp ritorna true */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Nuova run -> Opzioni */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Opzioni -> Esci */
    STATES_CHECK(ui.focus == 2, "la navigazione in MainMenu non arriva su Esci (indice 2)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "confirm su Esci non apre ExitConfirm");
    STATES_CHECK(!ui.exitAbandonsRun, "il contesto di ExitConfirm da MainMenu non e' 'uscita dal gioco'");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla -> Conferma */
    STATES_CHECK(ui.focus == 0, "down da Annulla in ExitConfirm (quit) non porta a Conferma");
    {
        AppInput in = InputConfirm();
        bool wantsExit = UpdateApp(game, &mode, &gen, &ui, &in);
        STATES_CHECK(wantsExit, "ExitConfirm/Conferma (uscita dal gioco) non fa ritornare true a UpdateApp");
    }

    /* Fase Game vittoria -> RunResults -> Menu principale -> MainMenu */
    mode = APP_GAMEPLAY;
    ui.focus = 0;
    game->phase = PHASE_WIN;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_RESULTS, "PHASE_WIN in Gameplay non porta a RunResults");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di RunResults non e' 0 (Nuova run subito)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Nuova run subito -> Menu principale */
    STATES_CHECK(ui.focus == 1, "down in RunResults non porta a Menu principale (indice 1)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "RunResults/Menu principale non torna a MainMenu");

    /* Fase Game sconfitta -> RunResults -> Nuova run subito -> FloorZero (M5:
       nuove proposte, nuova scelta -- l'uscita si apre solo dopo, requisito
       10) -> attraversamento -> Gameplay */
    mode = APP_GAMEPLAY;
    ui.focus = 0;
    game->phase = PHASE_GAME_OVER;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_RESULTS, "PHASE_GAME_OVER in Gameplay non porta a RunResults");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* focus 0: Nuova run subito */
    STATES_CHECK(mode == APP_FLOOR_ZERO, "RunResults/Nuova run subito non porta a FloorZero");
    STATES_CHECK(!game->floorZeroExitOpen, "l'uscita del Piano 0 dopo RunResults e' aperta prima della scelta del tema");
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    STATES_CHECK(game->floorZeroExitOpen, "l'uscita del Piano 0 dopo RunResults non e' aperta dopo la scelta del tema");
    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "l'attraversamento dopo RunResults non porta a Gameplay");
    STATES_CHECK(game->phase == PHASE_PLAY, "la nuova run non ha richiamato GameResetRun (fase non tornata a PLAY)");

    return true;
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

/* Fase 3a-L3: se l'oggetto porta uno script Lua (item->luaSource non vuoto,
   caricato da run_content.c da un file referenziato nel manifest), lo carica
   davvero in una ScriptSandbox nuova con l'API di gioco VERA (ScriptApiRegister,
   lo stesso codice che il gioco usa a runtime in ScriptItemsOnAcquire, non uno
   stub): un manifest che referenzia uno script che non compila piu' (build del
   gioco cambiata, file corrotto a mano...) deve far fallire QUESTO test, non
   scoprirsi silenziosamente solo al primo pickup in game. melting-gen ha gia'
   validato lo stesso script con la sua sandbox (gen_lua.c) prima di scriverlo:
   questo e' un secondo controllo, piu' a valle, con l'API vera invece dello
   stub - difesa in profondita', non ridondanza inutile. */
static bool ManifestLuaLoads(Game *game, const Item *item)
{
    if (item->luaSource[0] == '\0') return true;   /* nessun Lua: solo mini-VM, niente da controllare qui */

    ScriptSandbox *sb = ScriptSandboxCreate(1u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb) return false;
    ScriptApiRegister(sb, game);
    char err[160];
    bool ok = ScriptSandboxLoad(sb, item->name, item->luaSource, err, sizeof(err));
    if (!ok) fprintf(stderr, "GameManifestTest: Lua di '%s' non carica: %s\n", item->name, err);
    ScriptSandboxDestroy(sb);
    return ok;
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
            if (item->kind != ITEM_ACTIVE) return false;   /* fase 3: i 3 oggetti del piano sono sempre attivi */
            if (!ManifestLuaLoads(game, item)) return false;
        }
        /* Fase 3: l'oggetto stat-up del piano (ricompensa del boss) e'
           sempre presente, sempre ITEM_STATUP, e non ha script mini-VM
           (nessun comportamento, solo statistiche: vedi
           tools/melting-gen/gen_manifest.c, WriteManifest, che non scrive
           mai "floorN.bossItem.script="). */
        const Item *boss = &game->content.floors[f].bossItem;
        if (!boss->name[0]) return false;
        if (boss->kind != ITEM_STATUP) return false;
        if (boss->script[0] != '\0') return false;
        /* Fase 3b: il bossItem di un manifest GENERATO (non un vecchio
           manifest senza la riga "rarity=") e' sempre raro o leggendario
           (vedi GEN_RARITY_WEIGHTS_BOSS in tools/melting-gen/gen_util.c: zero
           peso su comune/non-comune). Prova il round-trip manifest->Item.rarity
           per davvero, non solo il testo grezzo (quello lo copre
           scripts/test-gen.sh via grep sul manifest). */
        if (boss->rarity != RARITY_RARE && boss->rarity != RARITY_LEGENDARY) return false;
    }
    /* GameManifestTest verificava solo il contenuto testuale del manifest, mai
       game->atlasLoaded: una regressione nello scrittore del BMP (Important 2)
       degraderebbe silenziosamente al rendering per forme con tutti i test
       verdi. Se il manifest referenzia un atlas che esiste su disco, deve
       essere stato caricato. */
    if (game->content.atlasPath[0] && FileExists(game->content.atlasPath) && !game->atlasLoaded) return false;
    return true;
}

/* Somma delle differenze assolute sui tre canali RGB fra due colori. Usata
   sotto per verificare che un pixel sia cambiato nettamente fra due render
   (soglia ben sopra il rumore di anti-aliasing/blend), senza dover predire a
   mano il colore esatto risultante da un blend alpha ne' indovinare se la
   riga diagonale della griglia del pavimento (DrawRoom) passa per quel
   preciso pixel. */
static int ColorChannelDiff(Color a, Color b)
{
    return abs((int)a.r - (int)b.r) + abs((int)a.g - (int)b.g) + abs((int)a.b - (int)b.b);
}

/* Verifica il bug critico della fase 2 (vedi la spec): una cella dell'atlas
   rimasta vuota (gate di qualita' di melting-sprites fallito) non deve
   rendere invisibile l'entita' corrispondente. Costruisce un atlas 1024x1024
   sintetico con un canale alpha vero, tutto opaco tranne TRE celle note
   (player, nemico chaser, uscita) e verifica che: (1) il caricamento
   riconosca SOLO quelle celle come assenti, non l'intero atlas; (2) il
   rendering vero (non solo lo stato) disegni comunque la sagoma di riserva
   per ciascuna delle tre entita' corrispondenti, senza crash.

   Player, nemico e uscita sono le tre entita' i cui call-site in
   game_renderer.c (DrawPlayer, DrawEnemy, DrawPickup) erano storicamente
   trattati in modo diverso: solo DrawPlayer controllava il valore di ritorno
   di DrawAtlasCell. Questo test esiste apposta per coprire anche gli altri
   due, che restavano invisibili quando l'atlas era caricato ma la loro cella
   era vuota (vedi il fix in DrawEnemy/DrawPickup). */
bool GameAtlasFallbackTest(Game *game)
{
    const char *testPath = "generated/test_atlas_fallback.png";
    Image img = GenImageColor(ATLAS_CELL*ATLAS_COLS, ATLAS_CELL*ATLAS_COLS, WHITE);
    ImageDrawRectangle(&img, (SPR_PLAYER%ATLAS_COLS)*ATLAS_CELL, (SPR_PLAYER/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    ImageDrawRectangle(&img, (SPR_ENEMY_CHASER%ATLAS_COLS)*ATLAS_CELL, (SPR_ENEMY_CHASER/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    ImageDrawRectangle(&img, (SPR_EXIT%ATLAS_COLS)*ATLAS_CELL, (SPR_EXIT/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    bool exported = ExportImage(img, testPath);
    UnloadImage(img);
    if (!exported) return false;

    GameUnloadAssets(game);
    snprintf(game->content.atlasPath, sizeof(game->content.atlasPath), "%s", testPath);
    AssetsLoad(game);
    remove(testPath);

    if (!game->atlasLoaded) return false;
    if (game->atlasCellPresent[SPR_PLAYER]) return false;         /* celle vuote: devono risultare assenti */
    if (game->atlasCellPresent[SPR_ENEMY_CHASER]) return false;
    if (game->atlasCellPresent[SPR_EXIT]) return false;
    if (!game->atlasCellPresent[SPR_ITEM]) return false;          /* cella piena: deve risultare presente */

    Vector2 enemyPos = { 150.0f, 200.0f };
    Vector2 exitPos = { 750.0f, 450.0f };
    /* LoadImageFromTexture su una RenderTexture2D legge i pixel col
       framebuffer OpenGL grezzo, che e' memorizzato capovolto rispetto alle
       coordinate con cui si e' disegnato (stesso motivo per cui
       RendererDrawApp usa un'altezza negativa quando ricompone il canvas
       sullo schermo, vedi sotto in questo file): riga 0 dell'immagine letta
       corrisponde al FONDO del canvas disegnato, non all'alto. Le coordinate
       di gioco vanno quindi capovolte in verticale prima di leggere il
       pixel. Il controllo sul giocatore sotto non lo fa (e passa comunque)
       solo perche' la sua intera sagoma di riserva e' un unico colore
       (tint) abbastanza esteso da coprire per coincidenza anche il pixel
       "sbagliato": non e' una controprova valida in generale, qui sotto si
       usa invece la trasformazione corretta. */
    int enemyImgY = SCREEN_HEIGHT - 1 - (int)enemyPos.y;
    int exitImgY = SCREEN_HEIGHT - 1 - (int)exitPos.y;

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    /* Primo render SENZA nemico ne' pickup extra (solo cio' che GameResetRun
       ha gia' piazzato viene rimosso da EntitiesClear): cattura il pixel di
       sfondo esatto (pavimento o riga diagonale della griglia, non importa
       quale) in ciascuna posizione candidata, cosi' il confronto sotto non
       deve indovinare la geometria della griglia. */
    EntitiesClear(game);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, false, NULL, NULL);
    Image before = LoadImageFromTexture(canvas.texture);
    Color enemyBefore = GetImageColor(before, (int)enemyPos.x, enemyImgY);
    Color exitBefore = GetImageColor(before, (int)exitPos.x, exitImgY);
    UnloadImage(before);

    EntitiesAddEnemy(game, ENEMY_CHASER, enemyPos);
    EntitiesAddPickup(game, PICKUP_EXIT, exitPos, 0, 0);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, false, NULL, NULL);   /* non deve andare in crash */

    /* DrawPlayer disegna, come riserva, un cerchio bianco per la testa a
       (pos.x, pos.y - 30): se DrawAtlasCell tornasse ancora "vero" per una
       cella vuota (il bug che questo test previene), li' sotto ci sarebbe
       solo il pavimento della stanza, mai bianco. Nemico e uscita non hanno
       un colore di riserva fisso e prevedibile in anticipo (il nemico usa
       game->theme.enemy generato a caso per la run, l'uscita e' un cerchio
       semitrasparente sopra qualunque cosa ci fosse sotto), quindi si
       verifica invece che il pixel sia CAMBIATO nettamente rispetto al
       render "prima": e' la prova che DrawEnemy/DrawPickup sono ricadute
       sulla forma geometrica invece di lasciare la cella vuota (e quindi
       l'entita') invisibile. */
    Image after = LoadImageFromTexture(canvas.texture);
    Color headPixel = GetImageColor(after, (int)game->player.pos.x, (int)(game->player.pos.y - 30.0f));
    Color enemyAfter = GetImageColor(after, (int)enemyPos.x, enemyImgY);
    Color exitAfter = GetImageColor(after, (int)exitPos.x, exitImgY);
    UnloadImage(after);
    UnloadRenderTexture(canvas);

    bool playerDrew = headPixel.r > 200 && headPixel.g > 200 && headPixel.b > 200;
    bool enemyDrew = ColorChannelDiff(enemyBefore, enemyAfter) > 40;
    bool exitDrew = ColorChannelDiff(exitBefore, exitAfter) > 40;

    return playerDrew && enemyDrew && exitDrew;
}

/* Il personaggio a strati (fase 3, vedi docs/superpowers/specs/2026-07-13-
   items-synergy-vision.md sezione 3, e src/render/item_layers.h). Due parti:

   1. BuildItemLayers e' una funzione PURA (item_layers.c): la si esercita
      qui direttamente, con un array di Item costruito a mano, senza bisogno
      di Game* ne' di una finestra, per verificare (a) un layer per ciascuno
      dei sei slot e (b) il tetto per-slot con badge di overflow su uno slot
      sovraffollato (8 cappelli, oltre ITEM_LAYER_MAX_PER_SLOT = 6).
   2. Il percorso vero, a schermo: lo stesso mix di oggetti finisce nel
      Player VERO e RendererDrawApp disegna un frame completo su una
      RenderTexture. Come --atlas-fallback-test, l'unica cosa richiesta e'
      che non vada in crash e che produca un frame -- il rendering resta
      visivo, non e' questo il posto per predire pixel esatti di una dozzina
      di layer sovrapposti. */
bool GameLayerTest(Game *game)
{
    Item items[13] = { 0 };
    int n = 0;
    for (int i = 0; i < 8; i++)
    {
        items[n].active = true;
        items[n].slot = SLOT_HAT;
        items[n].color = RED;
        n++;
    }
    static const ItemSlot others[] = { SLOT_EYES, SLOT_HAND, SLOT_BACK, SLOT_BODY, SLOT_AURA };
    for (int i = 0; i < 5; i++)
    {
        items[n].active = true;
        items[n].slot = others[i];
        items[n].color = BLUE;
        n++;
    }

    ItemLayer layers[MAX_ITEMS];
    int count = BuildItemLayers(items, n, layers, MAX_ITEMS);

    /* 6 cappelli (il tetto) + 1 layer per ciascuno degli altri cinque slot =
       11, non 13: i due cappelli oltre il tetto restano equipaggiati (il
       gameplay non li vede toccati da questo test) ma non producono un
       altro layer disegnato. */
    if (count != 11)
    {
        fprintf(stderr, "GameLayerTest: attesi 11 layer, trovati %d\n", count);
        return false;
    }

    /* Ordine di disegno atteso: corpo, mantello, mano, occhi, poi i sei
       cappelli, poi l'aura (vedi kSlotDrawOrder in item_layers.c). */
    static const ItemSlot expectedOrder[11] = {
        SLOT_BODY, SLOT_BACK, SLOT_HAND, SLOT_EYES,
        SLOT_HAT, SLOT_HAT, SLOT_HAT, SLOT_HAT, SLOT_HAT, SLOT_HAT,
        SLOT_AURA
    };
    for (int i = 0; i < 11; i++)
    {
        if (layers[i].slot != expectedOrder[i])
        {
            fprintf(stderr, "GameLayerTest: layer %d ha slot %d, atteso %d\n", i, layers[i].slot, expectedOrder[i]);
            return false;
        }
    }

    /* L'ultimo cappello disegnato (stackIndex 5, il tetto) deve riportare
       stackTotal 8: e' il segnale che dice a DrawItemLayer di disegnare il
       badge "+2" invece di lasciare i due cappelli in eccesso silenziosi. */
    bool foundOverflowHat = false;
    for (int i = 0; i < count; i++)
    {
        if (layers[i].slot == SLOT_HAT && layers[i].stackIndex == ITEM_LAYER_MAX_PER_SLOT - 1)
        {
            if (layers[i].stackTotal != 8)
            {
                fprintf(stderr, "GameLayerTest: stackTotal del cappello in overflow e' %d, atteso 8\n", layers[i].stackTotal);
                return false;
            }
            foundOverflowHat = true;
        }
    }
    if (!foundOverflowHat)
    {
        fprintf(stderr, "GameLayerTest: nessun layer HAT con stackIndex al tetto\n");
        return false;
    }

    /* Parte 2: lo stesso mix sul Player vero, disegnato per davvero. Cattura
       anche uno screenshot di comodo (percorso SEPARATO da
       logs/melting-run-screen.png, che resta di --screenshot-test) cosi' il
       proprietario puo' vedere il personaggio a strati senza dover giocare
       una run fino a raccogliere otto cappelli. */
    memset(game->player.items, 0, sizeof(game->player.items));
    for (int i = 0; i < n; i++) game->player.items[i] = items[i];
    game->player.itemCount = n;

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, "logs/melting-run-layers-screen.png");   /* non deve andare in crash */
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);

    return textureValid;
}

/* Fase 3b VISIVA (docs/superpowers/specs/2026-07-13-pools-rarity-design.md,
   sezione 6): verifica manuale/screenshot che RarityColor/RarityName
   (src/render/rarity_style.h) si leggano bene DAVVERO, sia sul pickup a
   terra (DrawPickup) sia nel pannello (DrawItemPreview). Come GameLayerTest
   sopra, costruisce a mano un mix -- qui uno per ciascuna delle quattro
   rarita' -- invece di giocare una run intera sperando di incontrare un
   leggendario: NON tocca game->content (la distribuzione/generazione resta
   quella caricata da GameResetRun, fuori dallo scopo di questo task), si
   limita a impostare player.items[] (pannello "OGGETTI PRESI" + personaggio
   equipaggiato) e a piazzare quattro pickup a terra (uno costa monete, come
   un vero oggetto da negozio, per verificare che l'anello di rarita' e il
   prezzo restino entrambi leggibili). */
bool GameRarityScreenshotTest(Game *game)
{
    static const Rarity kRarities[4] = { RARITY_COMMON, RARITY_UNCOMMON, RARITY_RARE, RARITY_LEGENDARY };
    static const char *kNames[4] = { "Straccio Comune", "Amuleto Verde", "Lente Blu", "Corona Dorata" };
    static const ItemSlot kSlots[4] = { SLOT_HAT, SLOT_EYES, SLOT_HAND, SLOT_BACK };

    memset(game->player.items, 0, sizeof(game->player.items));
    for (int i = 0; i < 4; i++)
    {
        Item *it = &game->player.items[i];
        it->active = true;
        snprintf(it->name, sizeof(it->name), "%s", kNames[i]);
        it->slot = kSlots[i];
        it->rarity = kRarities[i];
        it->kind = ITEM_ACTIVE;
        it->color = game->theme.accent;
        it->shape = i;
    }
    game->player.itemCount = 4;

    EntitiesClear(game);
    for (int i = 0; i < 4; i++)
    {
        Item pickupItem = { 0 };
        pickupItem.active = true;
        snprintf(pickupItem.name, sizeof(pickupItem.name), "%s", kNames[i]);
        pickupItem.slot = kSlots[i];
        pickupItem.rarity = kRarities[i];
        pickupItem.kind = ITEM_ACTIVE;
        pickupItem.color = game->theme.accent2;
        pickupItem.shape = i;
        /* Solo il leggendario ha un costo (com'e' nel negozio vero): verifica
           che l'etichetta "Nc" e l'anello di rarita' non si accavallino
           (task brief, punto 4). */
        int cost = (kRarities[i] == RARITY_LEGENDARY) ? ItemShopCostForRarity(RARITY_LEGENDARY) : 0;
        EntitiesAddItemPickup(game, (Vector2){ ROOM_X + 130.0f + (float)i*170.0f, ROOM_Y + ROOM_H*0.5f }, pickupItem, cost);
    }

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, "logs/melting-run-rarity-screen.png");   /* non deve andare in crash */
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);

    return textureValid;
}

/* Step C (docs/superpowers/specs/2026-07-14-step-c-shottype-balance.md): il
   COMPORTAMENTO dei tipi di colpo e' verificato per davvero dai test R-W della
   suite --script-items-test (bilanciamento, catena, perforazione, ricalcolo). Ma
   il feedback che ha aperto questa fase chiedeva anche un ASPETTO diverso per
   ogni tipo, e quello nessun assert puo' giudicarlo: serve guardarlo. Questo test
   mette in scena un colpo per ciascuna delle cinque forme (piu' uno nemico, che
   resta sempre una palla) e salva uno screenshot -- stessa idea e stesso schema
   di GameRarityScreenshotTest sopra: costruisce a mano un campione impossibile da
   incontrare in una run vera, e l'assert automatico e' solo "non va in crash e la
   texture e' valida" (DrawRectanglePro/DrawLineEx con rotazioni e spessori sono
   proprio il tipo di codice che puo' esplodere su un caso limite).
   Il file esce in logs/melting-run-shotforms-screen.png. */
bool GameShotFormsScreenshotTest(Game *game)
{
    static const ShotForm kForms[SHOT_FORM_COUNT] = {
        SHOT_FORM_ORB, SHOT_FORM_SPIKE, SHOT_FORM_BEAM, SHOT_FORM_ARC, SHOT_FORM_BLADE
    };

    EntitiesClear(game);

    /* Un colpo per forma, in fila, tutti in volo verso destra: cosi' le forme
       orientate dalla velocita' (spike, beam, arc) si vedono orientate davvero. */
    for (int i = 0; i < (int)SHOT_FORM_COUNT; i++)
    {
        Shot *shot = EntitiesAddShot(game, true,
                                     (Vector2){ ROOM_X + 150.0f + (float)i*150.0f, ROOM_Y + ROOM_H*0.45f },
                                     (Vector2){ 1.0f, 0.0f }, 420.0f, 8.0f, 7.0f, 0, game->theme.accent2);
        if (shot) shot->form = kForms[i];
    }
    /* Un colpo nemico: resta SEMPRE una palla (i nemici non hanno tipi di colpo,
       vedi combat.c) -- e' il controllo negativo dello screenshot. */
    EntitiesAddShot(game, false, (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.75f },
                    (Vector2){ -1.0f, 0.0f }, 240.0f, 1.0f, 6.0f, 0, game->theme.enemy);

    /* Fase 3b: un nemico per FORMA, in fila. Stessa ragione delle forme dei colpi --
       il feedback chiedeva nemici che si vedano diversi, e nessun assert puo'
       giudicarlo: serve guardarli. */
    static const EnemyForm kEnemyForms[ENEMY_FORM_COUNT] = {
        ENEMY_FORM_BLOB, ENEMY_FORM_SPIKY, ENEMY_FORM_ARMORED, ENEMY_FORM_FLOATER
    };
    for (int i = 0; i < (int)ENEMY_FORM_COUNT; i++)
    {
        EnemyTypeDef foe;
        memset(&foe, 0, sizeof(foe));
        foe.active = true;
        foe.form = kEnemyForms[i];
        foe.move = ENEMY_MOVE_CHASE;
        foe.fire = ENEMY_FIRE_NONE;
        foe.hpMul = 1.0f; foe.speedMul = 1.0f; foe.sizeMul = 1.2f; foe.pellets = 1;
        snprintf(foe.name, sizeof(foe.name), "Forma %d", i + 1);
        EntitiesAddEnemyTyped(game, ENEMY_CHASER,
                              (Vector2){ ROOM_X + 190.0f + (float)i*170.0f, ROOM_Y + ROOM_H*0.72f }, &foe);
    }

    /* Il pannello "GIOCATORE" mostra il tipo di colpo attivo: gli si mette in
       mano l'oggetto che lo conferisce, cosi' lo screenshot verifica anche
       quella riga (nome inventato + forma) e il colore condiviso col proiettile.
       I due oggetti successivi (inseguimento + perforazione) formano una COPPIA
       (step D): servono a verificare a occhio l'altra meta' del feedback -- le
       sinergie devono VEDERSI. Nello screenshot: l'anello bianco attorno ai colpi
       sparati dal giocatore e l'elenco delle sinergie attive nel pannello "LOG"
       in basso. */
    memset(game->player.items, 0, sizeof(game->player.items));
    Item *item = &game->player.items[0];
    item->active = true;
    snprintf(item->name, sizeof(item->name), "Guanto di Schegge");
    item->slot = SLOT_HAND;
    item->rarity = RARITY_RARE;
    item->kind = ITEM_ACTIVE;
    item->color = game->theme.accent;
    ShotTypeExample(&item->shotType, 0);

    Item *homing = &game->player.items[1];
    homing->active = true;
    snprintf(homing->name, sizeof(homing->name), "Occhio Rapace");
    homing->slot = SLOT_EYES;
    homing->rarity = RARITY_UNCOMMON;
    homing->kind = ITEM_ACTIVE;
    homing->color = game->theme.accent2;
    homing->traits = TRAIT_HOMING;

    Item *pierce = &game->player.items[2];
    pierce->active = true;
    snprintf(pierce->name, sizeof(pierce->name), "Punteruolo Lungo");
    pierce->slot = SLOT_BACK;
    pierce->rarity = RARITY_UNCOMMON;
    pierce->kind = ITEM_ACTIVE;
    pierce->color = game->theme.accent;
    pierce->traits = TRAIT_PIERCE;

    game->player.itemCount = 3;
    ScriptItemsRecomputeStats(game);

    /* Fase 3c: un layout di stanza (colonne) coi suoi ostacoli, per guardare a occhio
       la resa 2.5D dei blocchi e la collisione. */
    RoomLayoutDef layout; memset(&layout, 0, sizeof(layout));
    layout.active = true; layout.form = ROOM_LAYOUT_PILLARS; layout.density = 0.6f;
    snprintf(layout.name, sizeof(layout.name), "Colonne");
    game->obstacleCount = RoomLayoutBuild(&layout, 12345u, ROOM_X, ROOM_Y, ROOM_W, ROOM_H, game->obstacles, MAX_OBSTACLES);

    /* Un colpo VERO sparato dal giocatore (non uno costruito a mano come i cinque
       sopra): e' l'unico che passa da CombatFirePlayer, quindi l'unico che riceve
       davvero il marchio della sinergia -- se l'anello comparisse anche senza
       passare di li', il canale B non sarebbe agganciato dove crediamo. */
    CombatFirePlayer(game, (Vector2){ 0.0f, -1.0f });
    /* Qualche frame di volo: appena sparato il colpo sta ESATTAMENTE sul
       giocatore, che gli viene disegnato sopra (DrawPlayer e' l'ultimo) -- lo
       screenshot mostrerebbe una sinergia invisibile proprio nel test che serve a
       verificare che si veda. */
    for (int frame = 0; frame < 12; frame++) CombatUpdateShots(game, 1.0f/60.0f);

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, "logs/melting-run-shotforms-screen.png");
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);

    return textureValid && game->player.shotType.active && game->player.synergies != 0u;
}

/* M1b/M5, SOLO manuale (--floor-zero-screenshot-test, mai in make test:
   stessa tradizione degli screenshot sopra, "per l'occhio del proprietario").
   Gen disabilitata: le carte curate lato gioco compaiono SUBITO all'ingresso
   (AppUseFallbackThemeCards, vedi AppEnterFloorZero in src/app/app.c) -- il
   pannello si apre a mano (TAB) cosi' lo screenshot mostra DAVVERO le carte
   (requisito 13 della spec M5), non solo l'indicatore testuale. */
bool GameFloorZeroScreenshotTest(Game *game)
{
    AppGen gen = { 0 };   /* enabled=false: le carte curate compaiono subito */
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* MainMenu -> RunSetup */
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* Seed -> Avvia */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Avvia -> FloorZero */
    if (mode != APP_FLOOR_ZERO)
    {
        fprintf(stderr, "GameFloorZeroScreenshotTest: Avvia non porta a FloorZero\n");
        return false;
    }
    if (game->themeCardCount < 2)
    {
        fprintf(stderr, "GameFloorZeroScreenshotTest: le carte di riserva non sono pronte subito\n");
        return false;
    }
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* apre il pannello per lo screenshot */

    /* Stesso messaggio che vedrebbe davvero il giocatore prima della scelta
       (AppFloorZeroStatusText, src/app/app.c: 'static', non esportata,
       quindi lo si ricostruisce qui identico -- e' solo testo per lo
       screenshot, non una regola verificata da questo test). */
    GenProgress status = { 0 };
    snprintf(status.message, sizeof(status.message), "In attesa della scelta del mondo -- TAB per le carte.");

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_FLOOR_ZERO, &ui, true, &status, "logs/worldsmelt-floorzero-screen.png");
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);
    return textureValid;
}

/* M4, SOLO manuale: stesso scenario di GameFloorZeroScreenshotTest sopra
   (identico passo per passo: e' lo stesso "guarda il Piano 0 gia' aperto"),
   chiamato pero' quando la finestra e' gia' a dimensione del monitor
   (src/app/app.c, --fullscreen-screenshot-test) -- RendererDrawApp chiama da
   sola UiComputeLayout() e legge GetScreenWidth/Height VERE, quindi non serve
   nessun parametro in piu' qui: e' la stessa identica chiamata, cambia solo
   la finestra dietro le quinte e il file di destinazione. */
bool GameFullscreenScreenshotTest(Game *game)
{
    AppGen gen = { 0 };   /* enabled=false: l'uscita del Piano 0 si apre subito */
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* MainMenu -> RunSetup */
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* Seed -> Avvia */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Avvia -> FloorZero */
    if (mode != APP_FLOOR_ZERO)
    {
        fprintf(stderr, "GameFullscreenScreenshotTest: Avvia non porta a FloorZero\n");
        return false;
    }

    GenProgress status = { 0 };
    snprintf(status.message, sizeof(status.message), "Primo piano pronto -- l'uscita e' aperta.");

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_FLOOR_ZERO, &ui, true, &status, "logs/worldsmelt-fullscreen-screen.png");
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);
    return textureValid;
}

#ifndef _WIN32
#include "gen/gen_runner.h"

#include <stdlib.h>
#include <time.h>

static bool GenRunnerWait(GenRunner *runner, double maxSeconds)
{
    for (int i = 0; i < (int)(maxSeconds*100.0); i++)
    {
        GenRunnerUpdate(runner);
        if (runner->state != GEN_RUNNER_RUNNING) return true;
        struct timespec ts = { 0, 10L*1000L*1000L };
        nanosleep(&ts, NULL);
    }
    return false;
}

bool GenRunnerSelfTest(void)
{
    const char *cmd = "tests/fake-gen.sh";
    GenRunner runner;
    setenv("FAKE_GEN_OUT", "generated", 1);

    setenv("FAKE_GEN_MODE", "ok", 1);
    if (!GenRunnerStart(&runner, cmd, 1, 10.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 10.0) || runner.state != GEN_RUNNER_SUCCEEDED) return false;
    if (runner.progress.percent != 100) return false;

    setenv("FAKE_GEN_MODE", "fail", 1);
    if (!GenRunnerStart(&runner, cmd, 2, 10.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 10.0) || runner.state != GEN_RUNNER_FAILED) return false;

    setenv("FAKE_GEN_MODE", "hang", 1);   /* timeout: 2s contro uno sleep 30 */
    if (!GenRunnerStart(&runner, cmd, 3, 2.0, "generated/gen_progress.txt")) return false;
    if (!GenRunnerWait(&runner, 8.0) || runner.state != GEN_RUNNER_FAILED) return false;

    setenv("FAKE_GEN_MODE", "hang", 1);   /* annullamento esplicito */
    if (!GenRunnerStart(&runner, cmd, 4, 30.0, "generated/gen_progress.txt")) return false;
    GenRunnerCancel(&runner);
    if (runner.state != GEN_RUNNER_FAILED) return false;

    /* Step B2: gli argomenti IN PIU' devono arrivare davvero al figlio. Se una
       argv malcostruita li perdesse, il processo di ripresa in sottofondo si
       comporterebbe da generatore NORMALE: rigenererebbe una run diversa da quella
       che il giocatore sta giocando, sovrascrivendo la sua -- in silenzio, senza
       che nessun altro test se ne accorga. Il finto generatore in modalita' "args"
       si limita a scrivere la propria riga di comando su file. */
    remove("generated/fake-gen-args.txt");
    setenv("FAKE_GEN_MODE", "args", 1);
    static const char *kExtra[] = { "--from-json", "generated/current_run.json", "--resume", NULL };
    if (!GenRunnerStartWithArgs(&runner, cmd, 4242, 10.0, "generated/gen_progress.txt", kExtra)) return false;
    if (!GenRunnerWait(&runner, 10.0) || runner.state != GEN_RUNNER_SUCCEEDED) return false;

    char *args = LoadFileText("generated/fake-gen-args.txt");
    if (!args) return false;
    bool sawSeed = strstr(args, "--seed 4242") != NULL;
    bool sawJson = strstr(args, "--from-json generated/current_run.json") != NULL;
    bool sawResume = strstr(args, "--resume") != NULL;
    UnloadFileText(args);
    if (!sawSeed || !sawJson || !sawResume)
    {
        fprintf(stderr, "GenRunnerSelfTest: argomenti extra non arrivati al figlio (seed=%d json=%d resume=%d)\n",
                sawSeed, sawJson, sawResume);
        return false;
    }

    return true;
}

/* Attende che 'runner' esca da RUNNING (successo/fallimento), sondando con
 * GenRunnerUpdate come farebbe UpdateApp -- usato dai poll ripetuti sotto
 * (proposeRunner e runner) invece di un ciclo per-scenario scritto a mano
 * ogni volta. Ritorna false se il runner e' ancora RUNNING dopo maxSeconds. */
static bool FloorZeroRunnerSettle(AppGen *gen, AppMode *mode, AppUi *ui, Game *game,
                                   GenRunnerState *watched, double maxSeconds)
{
    for (int i = 0; i < (int)(maxSeconds*100.0); i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, mode, gen, ui, &in);
        if (*watched != GEN_RUNNER_RUNNING) return true;
        struct timespec ts = { 0, 10L*1000L*1000L };
        nanosleep(&ts, NULL);
    }
    return *watched != GEN_RUNNER_RUNNING;
}

/* M1b/M5 (DEC-005): la sala d'attesa giocabile del Piano 0, ORA con la
 * scelta del tema in mezzo (systems/floor-zero.md, ui/generation-status.md).
 * Come GenRunnerSelfTest sopra, usa tests/fake-gen.sh con FAKE_GEN_MODE
 * (generazione completa) e FAKE_GEN_PROPOSE_MODE (--propose-themes, ora un
 * comando SEPARATO dello stesso finto script) per evitare un vero modello,
 * guidato attraverso UpdateApp (mai chiamando GenRunnerStart a mano). Sei
 * scenari indipendenti (requisito 13 della spec M5), ciascuno con un
 * ingresso pulito in FloorZero (stesso schema "un blocco, un AppGen fresco"
 * del test sopra) cosi' un fallimento in uno non trascina lo stato sporco
 * nel successivo. 'game' e' quello gia' pronto passato da AppRun
 * (GameResetRun gia' chiamata): FloorZeroEnter (chiamata da AppEnterFloorZero
 * dentro UpdateApp) lo riprepara da sola ad ogni ingresso. */
bool GameFloorZeroTest(Game *game)
{
    AppUi ui;
    AppMode mode;
    AppGen gen;

    /* --- scenario 1: le proposte sono ancora in corso -- carte non pronte,
       uscita chiusa, il giocatore si muove liberamente nell'hub nel
       frattempo (M1b, "il giocatore gira liberamente"). --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;   /* il passo sprite non serve a questi scenari: meno rumore, stesso principio di --no-sprites */
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "hang", 1);   /* scrive un progresso e poi dorme 30s: resta RUNNING per tutto lo scenario 1 */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* MainMenu -> RunSetup */
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* Seed -> Avvia */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Avvia -> FloorZero, avvia il finto propose */
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (1) Avvia non porta a FloorZero\n"); return false; }
    if (game->themeCardCount != 0) { fprintf(stderr, "GameFloorZeroTest: (1) carte gia' pronte col propose ancora in corso\n"); return false; }
    if (game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (1) l'uscita e' gia' aperta senza nemmeno un tema scelto\n"); return false; }

    /* Il movimento vero passa da IsKeyDown (mai simulabile senza una tastiera
       vera, vedi il commento su AppInput in app_internal.h): qui si sposta il
       giocatore A MANO, un piccolo passo per frame, e si chiama GameUpdate
       come farebbe AppSimStep (che tratta FloorZero come Gameplay, M1b) --
       la prova richiesta e' che nulla in quel percorso vada in crash o
       resetti la posizione mentre le proposte girano in sottofondo, non che
       il movimento reale funzioni (gia' coperto altrove). */
    Vector2 before = game->player.pos;
    for (int i = 0; i < 30; i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
        game->player.pos.x += 2.0f;
    }
    if (game->player.pos.x == before.x) { fprintf(stderr, "GameFloorZeroTest: (1) il giocatore non si muove in FloorZero\n"); return false; }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (1) FloorZero e' uscita da sola senza attraversamento\n"); return false; }
    if (gen.proposeRunner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.proposeRunner);   /* "hang" dorme 30s: non finirebbe mai da solo qui */

    /* --- scenario 2: le proposte arrivano, le carte compaiono ma l'uscita
       resta chiusa finche' non si sceglie -- la scelta sintetica (frecce +
       conferma, requisito 9) fa partire la generazione completa (fake),
       ancora "in corso" (hang) -> uscita ancora chiusa, movimento ok. --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_MODE", "hang", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (2) Avvia non porta a FloorZero\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (2) il finto propose non e' mai terminato\n");
        return false;
    }
    if (game->themeCardCount < 2) { fprintf(stderr, "GameFloorZeroTest: (2) le carte non sono pronte dopo il finto propose\n"); return false; }
    if (game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (2) l'uscita e' aperta prima della scelta del tema\n"); return false; }

    { AppInput in = InputTab();   UpdateApp(game, &mode, &gen, &ui, &in); }   /* apre il pannello */
    if (!game->themeCardsPanelOpen) { fprintf(stderr, "GameFloorZeroTest: (2) TAB non apre il pannello delle carte\n"); return false; }
    { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* sposta il focus dalla carta 0 alla 1 */
    if (game->themeCardFocus != 1) { fprintf(stderr, "GameFloorZeroTest: (2) destra non sposta il focus sulla carta 1\n"); return false; }
    { AppInput in = InputLeft();  UpdateApp(game, &mode, &gen, &ui, &in); }   /* torna sulla 0 */
    if (game->themeCardFocus != 0) { fprintf(stderr, "GameFloorZeroTest: (2) sinistra non riporta il focus sulla carta 0\n"); return false; }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* sceglie la carta 0 */
    if (game->themeChosenIndex != 0) { fprintf(stderr, "GameFloorZeroTest: (2) confirm non sceglie la carta col focus\n"); return false; }
    if (game->themeCardsPanelOpen) { fprintf(stderr, "GameFloorZeroTest: (2) il pannello resta aperto dopo la scelta\n"); return false; }
    if (gen.runner.state != GEN_RUNNER_RUNNING) { fprintf(stderr, "GameFloorZeroTest: (2) la generazione completa non e' partita dopo la scelta\n"); return false; }

    for (int i = 0; i < 30; i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
    }
    if (game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (2) l'uscita e' aperta col finto generatore ancora in corso\n"); return false; }
    if (gen.runner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.runner);   /* "hang" dorme 30s: non finirebbe mai da solo qui */

    /* --- scenario 3: proposte + scelta + generazione completa, TUTTO a buon
       fine -> uscita aperta -> attraversamento -> Gameplay. --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_MODE", "ok", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (3) Avvia non porta a FloorZero\n"); return false; }

    bool cardsReady = false;
    for (int i = 0; i < 1000 && !cardsReady; i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        cardsReady = game->themeCardCount > 0;
        if (!cardsReady) { struct timespec ts = { 0, 10L*1000L*1000L }; nanosleep(&ts, NULL); }
    }
    if (!cardsReady) { fprintf(stderr, "GameFloorZeroTest: (3) le carte non sono mai arrivate\n"); return false; }
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    if (game->themeChosenIndex != 0) { fprintf(stderr, "GameFloorZeroTest: (3) la scelta sintetica e' fallita\n"); return false; }

    bool opened = false;
    for (int i = 0; i < 1000 && !opened; i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        opened = game->floorZeroExitOpen;
        if (!opened) { struct timespec ts = { 0, 10L*1000L*1000L }; nanosleep(&ts, NULL); }
    }
    if (!opened) { fprintf(stderr, "GameFloorZeroTest: (3) l'uscita non si apre dopo il successo del finto generatore\n"); return false; }
    if (game->message[0] == '\0') { fprintf(stderr, "GameFloorZeroTest: (3) nessun messaggio d'apertura emesso\n"); return false; }

    /* Attraversamento del varco (il flag lo scriverebbe WorldHandleTransitions
       quando il giocatore preme contro il muro di fondo con l'uscita aperta
       -- qui si simula direttamente il segnale, esattamente come farebbe il
       world). */
    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (3) l'attraversamento non porta a Gameplay\n"); return false; }
    if (game->floor != 1) { fprintf(stderr, "GameFloorZeroTest: (3) il piano dopo l'attraversamento non e' 1 (e' %d)\n", game->floor); return false; }

    /* --- scenario 4 (requisito 13): gen disabilitata -- carte curate lato
       gioco IMMEDIATE (nessun processo), scelta, uscita aperta SUBITO dopo
       (la pipeline e' gia' "terminale" per costruzione: DEC-002). --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = false;
    mode = APP_MAIN_MENU;

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (4) Avvia non porta a FloorZero\n"); return false; }
    if (game->themeCardCount < 2) { fprintf(stderr, "GameFloorZeroTest: (4) le carte di riserva non sono pronte subito con gen disabilitata\n"); return false; }
    if (game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (4) l'uscita e' aperta prima della scelta, gen disabilitata\n"); return false; }
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (4) l'uscita non si apre subito dopo la scelta, gen disabilitata\n"); return false; }

    /* --- scenario 5 (requisito 13): abbandono con proposeRunner ANCORA
       attivo (prima che qualunque carta compaia) -> cancellato. --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "hang", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (5) Avvia non porta a FloorZero\n"); return false; }
    if (gen.proposeRunner.state != GEN_RUNNER_RUNNING) { fprintf(stderr, "GameFloorZeroTest: (5) il finto propose non risulta in corso\n"); return false; }

    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* ESC -> ExitConfirm */
    if (mode != APP_EXIT_CONFIRM) { fprintf(stderr, "GameFloorZeroTest: (5) ESC in FloorZero non apre ExitConfirm\n"); return false; }
    if (ui.openedFrom != APP_FLOOR_ZERO || !ui.exitAbandonsRun)
    {
        fprintf(stderr, "GameFloorZeroTest: (5) contesto di ExitConfirm da FloorZero sbagliato\n");
        return false;
    }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla -> Conferma */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_MAIN_MENU) { fprintf(stderr, "GameFloorZeroTest: (5) conferma da ExitConfirm/FloorZero non torna a MainMenu\n"); return false; }
    if (gen.proposeRunner.state == GEN_RUNNER_RUNNING) { fprintf(stderr, "GameFloorZeroTest: (5) proposeRunner non e' stato cancellato all'abbandono\n"); return false; }

    /* --- scenario 6: annullo dalla preparazione DOPO la scelta del tema,
       con la generazione completa (fake) ancora in corso -> cancellata
       (stesso scenario del vecchio "4", ora dopo propose+scelta). --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_MODE", "hang", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (6) Avvia non porta a FloorZero\n"); return false; }

    bool cardsReady6 = false;
    for (int i = 0; i < 1000 && !cardsReady6; i++)
    {
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        cardsReady6 = game->themeCardCount > 0;
        if (!cardsReady6) { struct timespec ts = { 0, 10L*1000L*1000L }; nanosleep(&ts, NULL); }
    }
    if (!cardsReady6) { fprintf(stderr, "GameFloorZeroTest: (6) le carte non sono mai arrivate\n"); return false; }
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    if (gen.runner.state != GEN_RUNNER_RUNNING) { fprintf(stderr, "GameFloorZeroTest: (6) il finto generatore non risulta in corso dopo la scelta\n"); return false; }

    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* ESC -> ExitConfirm */
    if (mode != APP_EXIT_CONFIRM) { fprintf(stderr, "GameFloorZeroTest: (6) ESC in FloorZero non apre ExitConfirm\n"); return false; }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla -> Conferma */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_MAIN_MENU) { fprintf(stderr, "GameFloorZeroTest: (6) conferma da ExitConfirm/FloorZero non torna a MainMenu\n"); return false; }
    if (gen.runner.state == GEN_RUNNER_RUNNING) { fprintf(stderr, "GameFloorZeroTest: (6) il generatore non e' stato cancellato all'abbandono\n"); return false; }

    return true;
}
#else
bool GenRunnerSelfTest(void)
{
    return true;   /* la generazione in-game non esiste su Windows */
}

bool GameFloorZeroTest(Game *game)
{
    (void)game;
    return true;   /* la generazione in-game non esiste su Windows */
}
#endif

/* ============================================================
   M2 (DEC-009, default PROPOSTO): stanze di numero e grandezza variabili.
   Vedi il commento in game_tests.h e game-design-knowledge-base/docs/
   game-design/systems/rooms-and-floor-generation.md ("Default proposti
   dall'implementazione"). Portabile (nessuna dipendenza da melting-gen/
   Xvfb): gira su entrambe le piattaforme, a differenza del blocco sopra.
   ============================================================ */

/* Genera il piano 'floor' con seed 'seed' su un Game LOCALE pulito sullo
   stack (come MakeBaseGame in script_items_tests.c): solo i campi che
   WorldStartFloor legge davvero, niente asset/atlas -- questo test non
   disegna nulla, quindi non serve nemmeno la finestra. */
static void RoomsTestGenerateFloor(unsigned int seed, int floor, Game *out)
{
    memset(out, 0, sizeof(*out));
    out->rng = seed;
    out->phase = PHASE_PLAY;
    WorldStartFloor(out, floor);
}

/* Test (g): RoomLayoutBuild alla taglia MINIMA garantita da DEC-009, con
   ogni forma non-vuota alla densita' MASSIMA (il caso piu' affollato
   possibile): deve continuare a produrre almeno un ostacolo (niente collasso
   silenzioso dei quadranti sotto i 20px, vedi room_layout.c) e rispettare
   comunque croce/cerchio centrali. Stessa logica di verifica di
   TestRoomLayoutAlwaysPlayable (src/tests/script_items_tests.c), qui alla
   taglia minima invece che a quella storica fissa. */
static bool RoomsTestMinSizeStillPlayable(void)
{
    const float rx = 10.0f, ry = 10.0f;   /* l'origine non conta nulla per questa verifica */
    const float rw = (float)WORLD_ROOM_MIN_W, rh = (float)WORLD_ROOM_MIN_H;
    const float cx = rx + rw*0.5f, cy = ry + rh*0.5f;
    /* Fascia piu' stretta della vera ROOM_CROSS_HALF (90px, room_layout.c):
       cosi' il test non si rompe se quel valore interno cambiasse di poco,
       ma cattura comunque un ostacolo che invadesse davvero il centro. */
    const float crossHalf = 70.0f;

    bool ok = true;
    int minBlocksSeen = 999;
    for (int form = 1; form < (int)ROOM_LAYOUT_COUNT; form++)   /* da 1: OPEN non ha ostacoli */
    {
        for (unsigned int seed = 1; seed <= 6; seed++)
        {
            RoomLayoutDef def;
            memset(&def, 0, sizeof(def));
            def.active = true;
            def.form = (RoomForm)form;
            def.density = ROOM_LAYOUT_DENSITY_MAX;

            Obstacle obs[MAX_OBSTACLES];
            int n = RoomLayoutBuild(&def, seed, rx, ry, rw, rh, obs, MAX_OBSTACLES);
            if (n < minBlocksSeen) minBlocksSeen = n;
            if (n <= 0) { ok = false; continue; }   /* collasso silenzioso: esattamente cio' che DEC-009 vieta */
            for (int i = 0; i < n; i++)
            {
                if (obs[i].x < cx + crossHalf && obs[i].x + obs[i].w > cx - crossHalf) ok = false;
                if (obs[i].y < cy + crossHalf && obs[i].y + obs[i].h > cy - crossHalf) ok = false;
                if (obs[i].x < rx || obs[i].y < ry ||
                    obs[i].x + obs[i].w > rx + rw || obs[i].y + obs[i].h > ry + rh) ok = false;
            }
        }
    }
    printf("  [rooms-g] taglia minima (%dx%d), ogni forma/densita' massima: minimo blocchi visti %d -> giocabile=%s\n",
           WORLD_ROOM_MIN_W, WORLD_ROOM_MIN_H, minBlocksSeen, ok ? "si" : "NO");
    if (!ok) printf("      FALLITO: alla taglia minima un layout ha collassato (0 ostacoli), murato croce/cerchio, o e' uscito dalla stanza\n");
    return ok;
}

bool GameRoomsTest(Game *game)
{
    /* Si genera sempre un Game LOCALE pulito (RoomsTestGenerateFloor): quello
       passato da AppRun serve solo a rispettare la stessa firma/convenzione
       di GamePortalRespawnTest e simili, non viene letto. */
    (void)game;
    bool ok = true;

    static const unsigned int kSeeds[] = { 1001u, 2002u, 3003u, 4004u };
    const int kSeedCount = (int)(sizeof(kSeeds)/sizeof(kSeeds[0]));

    /* (c) "varia tra piani/seed": si registrano i conteggi distinti visti
       durante l'intero giro sotto, e si pretende almeno due valori diversi. */
    int seenCounts[64];
    int seenCountsN = 0;

    for (int floor = 1; floor <= FLOOR_COUNT; floor++)
    {
        for (int si = 0; si < kSeedCount; si++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);

            typedef struct { int w; int h; } RoomsTestWH;
            RoomsTestWH sizes[GRID_SIZE*GRID_SIZE];
            int sizesN = 0;
            int count = 0;
            int bossX = -1, bossY = -1;

            for (int ry2 = 0; ry2 < GRID_SIZE; ry2++)
            {
                for (int rx2 = 0; rx2 < GRID_SIZE; rx2++)
                {
                    RoomState *r = &probe.rooms[ry2][rx2];
                    if (!r->exists) continue;
                    count++;
                    /* (a) grandezza minima garantita. */
                    if (r->w < WORLD_ROOM_MIN_W || r->h < WORLD_ROOM_MIN_H)
                    {
                        fprintf(stderr, "GameRoomsTest: (a) stanza (%d,%d) piano %d seed %u sotto il minimo garantito: %dx%d\n",
                                rx2, ry2, floor, kSeeds[si], r->w, r->h);
                        ok = false;
                    }
                    sizes[sizesN].w = r->w; sizes[sizesN].h = r->h; sizesN++;
                    if (r->kind == ROOM_BOSS) { bossX = rx2; bossY = ry2; }
                }
            }

            /* (b) nessuna coppia (w,h) ripetuta nello stesso piano. */
            for (int i = 0; i < sizesN; i++)
                for (int j = i + 1; j < sizesN; j++)
                    if (sizes[i].w == sizes[j].w && sizes[i].h == sizes[j].h)
                    {
                        fprintf(stderr, "GameRoomsTest: (b) coppia (w,h) ripetuta nel piano %d seed %u: %dx%d\n",
                                floor, kSeeds[si], sizes[i].w, sizes[i].h);
                        ok = false;
                    }

            /* (f) la stanza boss e' sempre alla taglia massima. */
            if (bossX < 0)
            {
                fprintf(stderr, "GameRoomsTest: (f) nessuna stanza boss nel piano %d seed %u\n", floor, kSeeds[si]);
                ok = false;
            }
            else if (probe.rooms[bossY][bossX].w != (int)ROOM_W || probe.rooms[bossY][bossX].h != (int)ROOM_H)
            {
                fprintf(stderr, "GameRoomsTest: (f) la stanza boss del piano %d seed %u non e' alla taglia massima: %dx%d\n",
                        floor, kSeeds[si], probe.rooms[bossY][bossX].w, probe.rooms[bossY][bossX].h);
                ok = false;
            }

            /* (c) banda attesa: targetRooms = 6+piano+(0..3), piu' fino a 2
               stanze speciali (tesoro/negozio) che WorldPlaceSpecialRoom puo'
               aggiungere oltre il random walk. */
            int lowerBound = 6 + floor;
            int upperBound = 11 + floor;
            if (count < lowerBound || count > upperBound)
            {
                fprintf(stderr, "GameRoomsTest: (c) piano %d seed %u ha %d stanze, fuori dalla banda attesa [%d,%d]\n",
                        floor, kSeeds[si], count, lowerBound, upperBound);
                ok = false;
            }
            bool alreadySeen = false;
            for (int i = 0; i < seenCountsN; i++) if (seenCounts[i] == count) { alreadySeen = true; break; }
            if (!alreadySeen && seenCountsN < 64) seenCounts[seenCountsN++] = count;

            /* (d) determinismo: rigenerare LO STESSO piano con lo stesso seed
               deve produrre la STESSA griglia (esistenza, tipo, taglia, porte). */
            Game probe2;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe2);
            bool detOk = true;
            for (int ry2 = 0; ry2 < GRID_SIZE; ry2++)
            {
                for (int rx2 = 0; rx2 < GRID_SIZE; rx2++)
                {
                    RoomState *a = &probe.rooms[ry2][rx2];
                    RoomState *b = &probe2.rooms[ry2][rx2];
                    if (a->exists != b->exists) { detOk = false; continue; }
                    if (!a->exists) continue;
                    if (a->kind != b->kind || a->w != b->w || a->h != b->h) detOk = false;
                    for (int d = 0; d < 4; d++) if (a->doors[d] != b->doors[d]) detOk = false;
                }
            }
            if (!detOk)
            {
                fprintf(stderr, "GameRoomsTest: (d) piano %d seed %u non deterministico: due generazioni con lo stesso seed differiscono\n",
                        floor, kSeeds[si]);
                ok = false;
            }

            /* (e) ogni transizione di porta atterra DENTRO il rettangolo della
               stanza di arrivo. Si forza 'cleared' sulla stanza di partenza per
               isolare la geometria dal gate di combattimento (gia' coperto da
               --portal-test) e si danno chiavi in abbondanza per non far
               fallire l'ingresso in una stanza tesoro non ancora visitata. */
            for (int ry2 = 0; ry2 < GRID_SIZE; ry2++)
            {
                for (int rx2 = 0; rx2 < GRID_SIZE; rx2++)
                {
                    RoomState *from = &probe.rooms[ry2][rx2];
                    if (!from->exists) continue;
                    for (int dir = 0; dir < 4; dir++)
                    {
                        if (!from->doors[dir]) continue;
                        probe.roomX = rx2;
                        probe.roomY = ry2;
                        from->cleared = true;
                        probe.player.keys = 9;
                        WorldTryEnterRoom(&probe, dir);
                        Rectangle arrival = WorldCurrentRoomRect(&probe);
                        const float tol = 0.5f;
                        bool inside = probe.player.pos.x >= arrival.x - tol && probe.player.pos.x <= arrival.x + arrival.width + tol &&
                                      probe.player.pos.y >= arrival.y - tol && probe.player.pos.y <= arrival.y + arrival.height + tol;
                        if (!inside)
                        {
                            fprintf(stderr, "GameRoomsTest: (e) transizione da (%d,%d) dir %d piano %d seed %u non atterra dentro la stanza di arrivo\n",
                                    rx2, ry2, dir, floor, kSeeds[si]);
                            ok = false;
                        }
                    }
                }
            }
        }
    }

    if (seenCountsN < 2)
    {
        fprintf(stderr, "GameRoomsTest: (c) il numero di stanze non varia mai fra i piani/seed testati (osservato un solo valore)\n");
        ok = false;
    }

    printf("  [rooms-abcdef] %d piani x %d semi: minimo garantito, taglie distinte, banda/variazione stanze (%d valori diversi), determinismo, transizioni di porta -> %s\n",
           FLOOR_COUNT, kSeedCount, seenCountsN, ok ? "ok" : "FALLITO");

    if (!RoomsTestMinSizeStillPlayable()) ok = false;

    return ok;
}

/* Piano strategico 16/07/2026, sezione tier: scrive un finto
   logs/benchmark.txt per ciascuno scenario e verifica AppReadBenchmarkPreset
   (src/app/app.c) -- niente Game ne' finestra, e' solo I/O di file. Il file
   di prova vive in logs/ (gia' creato dal target 'game' del Makefile) con un
   nome che non collide con quello vero scritto da scripts/benchmark.sh, e
   viene rimosso alla fine. */
bool AppBenchmarkPresetSelfTest(void)
{
    const char *path = "logs/bench-preset-selftest.tmp";
    bool ok = true;

    /* tier=lowspec, nessun override manuale: il preset si applica e il
       messaggio dice da dove viene. */
    {
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fputs("benchSchema=1\ntokS=8.00\nimgS=0\ntier=lowspec\nmeasuredAt=1\n", f);
        fclose(f);
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, false, &lowSpec, msg, sizeof(msg));
        if (!lowSpec || !strstr(msg, "preset low-spec dal benchmark"))
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: tier=lowspec non applicato (lowSpec=%d msg=\"%s\")\n", lowSpec, msg);
            ok = false;
        }
    }

    /* tier=unsupported: nessun preset (non blocca niente), ma un avviso. */
    {
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fputs("benchSchema=1\ntokS=2.00\nimgS=0\ntier=unsupported\nmeasuredAt=1\n", f);
        fclose(f);
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, false, &lowSpec, msg, sizeof(msg));
        if (lowSpec || msg[0] == '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: tier=unsupported non gestito (lowSpec=%d msg vuoto=%d)\n", lowSpec, msg[0] == '\0');
            ok = false;
        }
    }

    /* tier=full: nessuna azione, nessun messaggio. */
    {
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fputs("benchSchema=1\ntokS=20.00\nimgS=3.00\ntier=full\nmeasuredAt=1\n", f);
        fclose(f);
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, false, &lowSpec, msg, sizeof(msg));
        if (lowSpec || msg[0] != '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: tier=full ha toccato qualcosa (lowSpec=%d msg=\"%s\")\n", lowSpec, msg);
            ok = false;
        }
    }

    /* Override manuale --low-spec: il file (tier=lowspec) va ignorato del
       tutto -- ne' un secondo messaggio ne' un cambiamento del preset, che
       resta quello gia' scelto a mano dal chiamante. */
    {
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fputs("benchSchema=1\ntokS=8.00\nimgS=0\ntier=lowspec\nmeasuredAt=1\n", f);
        fclose(f);
        bool lowSpec = true;   /* il chiamante lo inizializza al valore manuale */
        char msg[160];
        AppReadBenchmarkPreset(path, true, false, &lowSpec, msg, sizeof(msg));
        if (!lowSpec || msg[0] != '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: --low-spec manuale non ha ignorato il file (lowSpec=%d msg=\"%s\")\n", lowSpec, msg);
            ok = false;
        }
    }

    /* Override manuale --full-spec: idem, anche con tier=lowspec nel file. */
    {
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, true, &lowSpec, msg, sizeof(msg));
        if (lowSpec || msg[0] != '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: --full-spec manuale non ha ignorato il file (lowSpec=%d msg=\"%s\")\n", lowSpec, msg);
            ok = false;
        }
    }

    /* File assente: comportamento di sempre, nessuna azione. */
    remove(path);
    {
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, false, &lowSpec, msg, sizeof(msg));
        if (lowSpec || msg[0] != '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: file assente ha comunque prodotto un effetto\n");
            ok = false;
        }
    }

    /* benchSchema sconosciuto: mai fidarsi di un formato che non riconosciamo,
       anche se tier=lowspec e' scritto alla lettera. */
    {
        FILE *f = fopen(path, "w");
        if (!f) return false;
        fputs("benchSchema=2\ntokS=8.00\nimgS=0\ntier=lowspec\nmeasuredAt=1\n", f);
        fclose(f);
        bool lowSpec = false;
        char msg[160];
        AppReadBenchmarkPreset(path, false, false, &lowSpec, msg, sizeof(msg));
        if (lowSpec || msg[0] != '\0')
        {
            fprintf(stderr, "AppBenchmarkPresetSelfTest: benchSchema sconosciuto non e' stato ignorato\n");
            ok = false;
        }
    }

    remove(path);
    return ok;
}
