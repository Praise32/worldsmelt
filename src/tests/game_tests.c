#include "tests/game_tests.h"

#include "assets/art_atlas.h"
#include "app/app.h"
#include "app/app_internal.h"
#include "audio/audio.h"
#include "content/character_roster.h"
#include "content/run_catalog.h"
#include "content/run_content.h"
#include "core/character_type.h"
#include "game/game_internal.h"
#include "content/curated_images.h"
#include "core/game_math.h"
#include "core/shot_type.h"
#include "gameplay/fusion.h"
#include "gameplay/item_pool.h"
#include "gameplay/item_slots.h"
#include "gameplay/item_traits.h"
#include "render/game_renderer.h"
#include "render/item_layers.h"
#include "script/script_api.h"
#include "script/script_items.h"
#include "script/script_sandbox.h"
#include "world/floor_zero.h"
#include "world/pourhouse.h"
#include "world/room_camera.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>   /* _mkdir/_rmdir: mkdtemp() non esiste ne' in UCRT ne' nel runtime MinGW-w64 */
#else
#include <unistd.h>   /* mkdtemp */
#endif

/* Crea una directory temporanea VUOTA e univoca per isolare il catalogo del
   test dal contenuto reale di catalog/ (known-issues.md #1). Scrive il
   percorso risultante in pathBuf (deve avere spazio a sufficienza) e lo
   ritorna, o NULL se la creazione fallisce dopo qualche tentativo. Su POSIX
   usa mkdtemp (sostituisce le 'X' finali in modo atomico); su Windows (nessun
   mkdtemp disponibile) prova una manciata di nomi resi univoci da rand() + un
   contatore finche' _mkdir() non ne accetta uno. Copia privata identica in
   src/tests/catalog_tests.c, stessa convenzione delle InputXxx di quel file. */
static char *CreateTempCatalogTestDir(char *pathBuf, size_t pathBufSize, const char *namePrefix)
{
    const char *base = getenv("TMPDIR");
#ifdef _WIN32
    if (!base) base = getenv("TEMP");
    if (!base) base = getenv("TMP");
    if (!base) base = ".";
    for (int attempt = 0; attempt < 64; attempt++)
    {
        snprintf(pathBuf, pathBufSize, "%s\\%s-%d-%d", base, namePrefix, rand(), attempt);
        if (_mkdir(pathBuf) == 0) return pathBuf;
    }
    return NULL;
#else
    if (!base) base = "/tmp";
    snprintf(pathBuf, pathBufSize, "%s/%s-XXXXXX", base, namePrefix);
    return mkdtemp(pathBuf);
#endif
}

/* Rimuove la directory temporanea creata da CreateTempCatalogTestDir sopra
   (gia' svuotata dal chiamante prima di chiamarla). */
static void RemoveTempCatalogTestDir(const char *path)
{
#ifdef _WIN32
    _rmdir(path);
#else
    rmdir(path);
#endif
}

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
static AppInput InputUp(void)      { AppInput in = { 0 }; in.up = true; return in; }   /* M6a: cambia sezione nel pannello combinato del Piano 0 */
static AppInput InputTab(void)     { AppInput in = { 0 }; in.tab = true; return in; }
static AppInput InputLeft(void)    { AppInput in = { 0 }; in.left = true; return in; }   /* M5: pannello di scelta del tema */
static AppInput InputRight(void)   { AppInput in = { 0 }; in.right = true; return in; }
static AppInput InputReroll(void)  { AppInput in = { 0 }; in.reroll = true; return in; }
static AppInput InputPause(void)   { AppInput in = { 0 }; in.pause = true; return in; }

#define STATES_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameStatesTest: %s\n", (msg)); return false; } } while (0)

/* M7 (substrato del catalogo): quanti file ci sono OGGI nella directory in
   cui RunCatalogWriteRun scriverebbe DAVVERO in questo momento (0 se la
   cartella non esiste ancora -- prima di qualunque scrittura vera, es. su
   una checkout pulita, e' il caso normale). Usata per verificare che i
   PHASE_WIN/PHASE_GAME_OVER sintetici di questo test NON scrivano nulla
   (spec M7, punto (b): "i game test esistenti... non devono scrivere file
   catalogo" -- la guardia vera e' AppUi.catalogWritesEnabled, zero-default,
   mai acceso qui sopra; questo conteggio e' la controprova osservabile su
   disco, non solo "confidiamo nella guardia"). Deve seguire lo STESSO
   percorso di RunCatalogWriteRun (RunCatalogGetTestPath() con fallback
   "catalog", src/content/run_catalog.c): per tutta la durata di
   GameStatesTest quel percorso e' la directory temporanea isolata da
   RunCatalogSetTestPath, quindi la controprova resta valida (non
   tautologica) anche se la guardia si rompesse davvero -- il file
   comparirebbe li' dentro, ed e' li' che questa funzione guarda. */
static int CatalogFileCount(void)
{
    const char *catalogPath = RunCatalogGetTestPath();
    if (!catalogPath) catalogPath = "catalog";
    if (!DirectoryExists(catalogPath)) return 0;
    FilePathList files = LoadDirectoryFilesEx(catalogPath, ".txt", false);
    int count = (int)files.count;
    UnloadDirectoryFiles(files);
    return count;
}

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
    bool test_success = true;

    /* W9 correzione round 1 (MINORE): il cursore VIRTUALE va portato in un
       angolo PRIMA del primo UpdateApp -- da W9 il primo sguardo sul mouse
       applica l'hover in base alla posizione reale del puntatore (vedi il
       commento su AppUi.mouseTracked in game_types.h), e questo test naviga i
       menu da tastiera dando per scontato il focus che scrive lui. (2,2) e'
       fuori da ogni geometria di menu per costruzione (i riquadri sono
       CENTRATI, vedi MenuBoxForModeFor), quindi l'esito di questo test non
       dipende piu' da dove Xvfb o il window manager lasciano il puntatore ne'
       dalle dimensioni della finestra. Stessa apertura di
       GameMouseHoverFocusTest. */
    SetMousePosition(2, 2);

    /* Macro locale di STATES_CHECK che usa goto per cleanup in caso di errore */
#undef STATES_CHECK
#define STATES_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameStatesTest: %s\n", (msg)); test_success = false; goto cleanup_test_catalog; } } while (0)

    /* Isolamento del catalogo del test: crea una directory temporanea VUOTA e usa quella
       invece del catalogo reale, cosi' il test non dipende dai file scritti dall'utente.
       Il test si aspetta un catalogo vuoto per costruzione (vedi commento nel test sui
       controlli della categoria 0). Vedi issue: known-issues.md #1. */
    char testCatalogPath[256] = { 0 };
    char *testCatalogDir = CreateTempCatalogTestDir(testCatalogPath, sizeof(testCatalogPath), "melting-test-catalog");
    if (!testCatalogDir)
    {
        fprintf(stderr, "GameStatesTest: errore nella creazione della directory temporanea\n");
        return false;
    }

    /* Settare il percorso del catalogo di test per isolare il catalogo reale.
       La directory temporanea è vuota, come si aspetta il test. */
    RunCatalogSetTestPath(testCatalogDir);

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
    STATES_CHECK(!ExitConfirmIsLightModalFor(ui.openedFrom, ui.exitDropsSuspendedRun),
        "ExitConfirm da FloorZero e' stato marcato come dialogo leggero (WP22, deve restare a schermo pieno, DEC-090)");
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

    /* WP22 (ui/inventory-and-synergy-screen.md, "Focus iniziale", gap G8
       ui-gioco): l'ingresso in BuildScreen mette il fuoco sull'ULTIMO oggetto
       acquisito, non sul primo -- verificato con un pickup VERO (CombatUpdatePickups,
       lo stesso consumo che il giocatore innesca camminando su un oggetto),
       non solo con itemCount scritto a mano come nei blocchi di fusione. */
    {
        /* Inventario noto: un solo oggetto "gia' posseduto" prima del pickup
           vero, cosi' l'indice dell'ultimo acquisito e' prevedibile. */
        game->player.itemCount = 0;
        Item alreadyOwned = { 0 };
        alreadyOwned.active = true;
        snprintf(alreadyOwned.name, sizeof(alreadyOwned.name), "%s", "Oggetto Precedente");
        alreadyOwned.kind = ITEM_PASSIVE;
        alreadyOwned.rarity = RARITY_COMMON;
        alreadyOwned.slot = SLOT_HAND;
        alreadyOwned.color = (Color){ 120, 160, 220, 255 };
        alreadyOwned.shape = 2;
        game->player.items[game->player.itemCount++] = alreadyOwned;

        /* Il pickup vero: un Pickup PICKUP_ITEM esattamente sulla posizione
           del giocatore, consumato da CombatUpdatePickups -- lo stesso
           percorso di CombatApplyItem che AppEnterBuildScreen deve
           rincorrere, non una scrittura diretta di itemCount.
           EntitiesAddItemPickup ritorna il puntatore al pickup appena creato:
           nessuna scansione ambigua fra eventuali pickup gia' presenti nella
           stanza (loot della generazione). */
        Item freshlyAcquired = { 0 };
        freshlyAcquired.active = true;
        snprintf(freshlyAcquired.name, sizeof(freshlyAcquired.name), "%s", "Oggetto Appena Preso");
        freshlyAcquired.kind = ITEM_PASSIVE;
        freshlyAcquired.rarity = RARITY_COMMON;
        freshlyAcquired.slot = SLOT_EYES;
        freshlyAcquired.color = (Color){ 220, 160, 120, 255 };
        freshlyAcquired.shape = 3;
        Pickup *seeded = EntitiesAddItemPickup(game, game->player.pos, freshlyAcquired, 0);
        STATES_CHECK(seeded != NULL, "EntitiesAddItemPickup non ha piazzato il pickup vero (precondizione del controllo del focus)");

        int countBeforePickup = game->player.itemCount;
        CombatUpdatePickups(game);
        STATES_CHECK(game->player.itemCount == countBeforePickup + 1,
            "CombatUpdatePickups non ha raccolto il pickup vero (precondizione del controllo del focus)");
        STATES_CHECK(strcmp(game->player.items[game->player.itemCount - 1].name, "Oggetto Appena Preso") == 0,
            "il pickup vero non e' finito nell'ultimo slot (precondizione del controllo del focus)");

        { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Gameplay -> BuildScreen */
        STATES_CHECK(mode == APP_BUILD_SCREEN, "TAB in Gameplay non apre BuildScreen (precondizione del controllo del focus)");
        STATES_CHECK(ui.buildItemFocus == game->player.itemCount - 1,
            "l'ingresso in BuildScreen non ha messo il fuoco sull'ULTIMO oggetto acquisito (WP22, gap G8 ui-gioco)");
        STATES_CHECK(strcmp(game->player.items[ui.buildItemFocus].name, "Oggetto Appena Preso") == 0,
            "il fuoco d'ingresso in BuildScreen non punta all'oggetto appena raccolto (WP22, gap G8 ui-gioco)");
        { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* torna a Gameplay */
        STATES_CHECK(mode == APP_GAMEPLAY, "BuildScreen/back non torna a Gameplay (dopo il controllo del focus sull'ultimo acquisito)");

        /* Fallback a 0 con la build vuota (comportamento invariato): svuotare
           l'inventario e rientrare deve dare focus 0, mai un indice negativo
           ne' un residuo del giro precedente. */
        game->player.itemCount = 0;
        { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
        STATES_CHECK(mode == APP_BUILD_SCREEN, "TAB in Gameplay non apre BuildScreen a build vuota");
        STATES_CHECK(ui.buildItemFocus == 0, "il fallback a build vuota non porta il fuoco a 0 (WP22, gap G8 ui-gioco)");
        { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* torna a Gameplay */
        STATES_CHECK(mode == APP_GAMEPLAY, "BuildScreen/back non torna a Gameplay (dopo il controllo del fallback a build vuota)");
    }

    /* Gameplay -> PauseMenu -> Prove -> (back) -> PauseMenu, focus su "Prove"
       (WP16, DEC-042): il pannello e' un ramo DENTRO APP_PAUSE_MENU (nessun
       nuovo AppMode), voce d'indice 2 inserita tra "Build e sinergie" e
       "Opzioni" -- vedi il commento su AppUi.pauseTrialsOpen. */
    { AppInput in = InputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "P in Gameplay non apre PauseMenu");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di PauseMenu non e' 0 (Riprendi)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Riprendi -> Build e sinergie */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Build e sinergie -> Prove */
    STATES_CHECK(ui.focus == 2, "due 'down' da Riprendi non arrivano su Prove (indice 2)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "confirm su Prove ha lasciato PauseMenu (deve restare, e' un pannello interno)");
    STATES_CHECK(ui.pauseTrialsOpen, "confirm su Prove non apre il pannello delle prove");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(!ui.pauseTrialsOpen, "back sul pannello Prove non lo chiude");
    STATES_CHECK(ui.focus == 2, "chiudere il pannello Prove non ripristina il focus sull'indice 2 (Prove)");

    /* PauseMenu -> Options -> (back) -> PauseMenu, focus su "Opzioni" */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Prove -> Opzioni */
    STATES_CHECK(ui.focus == 3, "un 'down' da Prove non arriva su Opzioni (indice 3)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_OPTIONS, "confirm su Opzioni non apre Options");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_PAUSE_MENU, "Options/back non torna a PauseMenu");
    STATES_CHECK(ui.focus == 3, "il ritorno da Options non ripristina il focus su Opzioni");

    /* PauseMenu -> BuildScreen -> (back) -> PauseMenu. Sei righe da WP21
       (DEC-114, "Rigenera la run" all'indice 4): un giro completo da Opzioni
       richiede ora QUATTRO 'down' (Opzioni -> Rigenera la run -> Abbandona
       run -> Riprendi -> Build e sinergie), non piu' tre. */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Opzioni -> Rigenera la run */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Rigenera la run -> Abbandona run */
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

    /* Slot funzionali (systems/active-items.md, systems/grafts.md): i due
       tasti degli slot LATCHANO in Gameplay -- la simulazione a passo fisso
       li consuma poi una volta sola, come la bomba. Qui si verifica solo il
       cablaggio input->latch; l'effetto vero (cariche, sgancio) e' materia
       dei test AQ/AR di --script-items-test. */
    game->useActiveQueued = false;
    game->dropGraftQueued = false;
    { AppInput in = { 0 }; in.useActive = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(game->useActiveQueued, "E in Gameplay non mette in coda l'uso dell'attivo");
    STATES_CHECK(!game->dropGraftQueued, "E in Gameplay ha messo in coda anche lo sgancio dell'Innesto");
    { AppInput in = { 0 }; in.dropGraft = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(game->dropGraftQueued, "G in Gameplay non mette in coda lo sgancio dell'Innesto");
    STATES_CHECK(mode == APP_GAMEPLAY, "i tasti degli slot funzionali hanno cambiato stato applicativo");
    game->useActiveQueued = false;
    game->dropGraftQueued = false;

    /* DEC-184 (ui/hud.md, "Blocco statistiche"): il toggle C nasconde/mostra
       il blocco -- vive su AppUi (sopravvive a GameResetRun), zero-default
       falso = VISIBILE, come il documento chiede ("visibile di default").
       Nessun effetto sullo stato applicativo (mode) ne' su 'game'. */
    STATES_CHECK(!ui.hudStatsHidden, "il blocco statistiche non e' visibile di default (hudStatsHidden zero-default atteso falso)");
    { AppInput in = { 0 }; in.toggleStats = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(ui.hudStatsHidden, "C in Gameplay non ha nascosto il blocco statistiche");
    STATES_CHECK(mode == APP_GAMEPLAY, "C in Gameplay ha cambiato stato applicativo");
    { AppInput in = { 0 }; in.toggleStats = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(!ui.hudStatsHidden, "una seconda pressione di C non ha rimostrato il blocco statistiche");
    /* Fuori da Gameplay il tasto non deve toccare la preferenza: stessa regola
       di E/G verificata sopra per useActive/dropGraft. */
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Gameplay -> BuildScreen */
    STATES_CHECK(mode == APP_BUILD_SCREEN, "TAB in Gameplay non apre BuildScreen (pre-condizione del controllo C fuori Gameplay)");
    { AppInput in = { 0 }; in.toggleStats = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(!ui.hudStatsHidden, "C fuori da Gameplay ha comunque nascosto il blocco statistiche");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "il ritorno a Gameplay dopo il controllo C e' fallito");

    /* Gameplay -> TAB -> BuildScreen -> (back) -> Gameplay */
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_BUILD_SCREEN, "TAB in Gameplay non apre BuildScreen");
    /* Gli stessi tasti FUORI da Gameplay non devono fare nulla (sono di
       gioco, non di navigazione: stessa regola della bomba). */
    { AppInput in = { 0 }; in.useActive = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = { 0 }; in.dropGraft = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(!game->useActiveQueued && !game->dropGraftQueued, "E/G fuori da Gameplay mettono comunque in coda un'azione di gioco");
    STATES_CHECK(mode == APP_BUILD_SCREEN, "E/G in BuildScreen hanno cambiato stato applicativo");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "BuildScreen/back da Gameplay non torna a Gameplay");

    /* PauseMenu -> Rigenera la run -> ExitConfirm -> (annulla) -> PauseMenu
       senza alcun effetto, poi di nuovo -> (conferma) -> nuova run con seed
       DIVERSO, mai RunResults (WP21, DEC-114: reroll SOLO da qui, con
       conferma esplicita; DEC-089: il reroll salta i risultati). Il vecchio
       tasto rapido R resta invece il reset rapido STESSO seed: qui sopra
       gen.enabled e' false per tutto GameStatesTest (vedi il commento
       introduttivo alla riga 115), quindi R in Gameplay in questo blocco
       esercita solo il cablaggio resetQueued, non prova nulla sul gap che
       DEC-114 dichiarava ("oggi R rigenera direttamente" con la generazione
       ABILITATA). GameRngSeedTest non lo prova nemmeno per contro: confronta
       due GameResetRunWithSeed con lo stesso seed, senza mai chiamare
       UpdateApp ne' toccare il tasto R. La prova vera che R NON rigenera piu'
       nemmeno con gen.enabled=true e' lo scenario 13 di GameFloorZeroTest
       (src/tests/game_tests.c, gen.command="tests/fake-gen.sh"): mode resta
       APP_GAMEPLAY, resetQueued si accende, game->runSeed e
       gen.pendingGenSeed restano invariati e nessun proposeRunner parte. */
    {
        int floorBeforeReroll = game->floor;
        int catalogBeforeRerollCancel = CatalogFileCount();

        { AppInput in = InputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* PauseMenu, focus su Riprendi */
        { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Build e sinergie */
        { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Prove (WP16) */
        { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Opzioni */
        { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Rigenera la run */
        STATES_CHECK(ui.focus == 4, "la navigazione in PauseMenu non arriva su Rigenera la run (indice 4)");
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        STATES_CHECK(mode == APP_EXIT_CONFIRM, "confirm su Rigenera la run non apre ExitConfirm");
        STATES_CHECK(ui.exitRerollsRun, "il contesto di ExitConfirm da 'Rigenera la run' non e' 'reroll'");
        STATES_CHECK(!ui.exitAbandonsRun, "il reroll si e' marcato anche come abbandono (i due contesti devono essere esclusivi)");
        STATES_CHECK(ui.focus == 1, "il focus iniziale di ExitConfirm (reroll) non e' 1 (Annulla)");

        /* Annulla: nessun effetto su 'game', si resta in pausa. */
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla */
        STATES_CHECK(mode == APP_PAUSE_MENU, "ExitConfirm(reroll)/Annulla non torna a PauseMenu");
        STATES_CHECK(ui.focus == 4, "il ritorno da ExitConfirm(reroll)/Annulla non ripristina il focus su Rigenera la run");
        STATES_CHECK(game->floor == floorBeforeReroll, "Annulla il reroll ha comunque cambiato il piano corrente");
        STATES_CHECK(CatalogFileCount() == catalogBeforeRerollCancel, "Annulla il reroll ha scritto un file di catalogo");

        /* Conferma: nuova run, seed DIVERSO, dritta a FloorZero (mai
           RunResults). */
        unsigned int seedBeforeReroll = game->runSeed;
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* di nuovo in ExitConfirm */
        STATES_CHECK(mode == APP_EXIT_CONFIRM, "rientro in ExitConfirm(reroll) fallito");
        { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }      /* Annulla -> Conferma */
        STATES_CHECK(ui.focus == 0, "down da Annulla in ExitConfirm(reroll) non porta a Conferma");
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        STATES_CHECK(mode == APP_FLOOR_ZERO, "ExitConfirm/Conferma (reroll) non riparte da FloorZero");
        STATES_CHECK(!game->floorZeroExitOpen, "il reroll confermato apre gia' l'uscita prima della scelta del tema");
        STATES_CHECK(CatalogFileCount() == catalogBeforeRerollCancel, "ExitConfirm/Conferma (reroll) ha scritto un file in catalog/ (guardia test-safe non attiva)");
        STATES_CHECK(game->catalogRecordsWritten == 0, "ExitConfirm/Conferma (reroll) ha valorizzato catalogRecordsWritten (guardia test-safe non attiva)");
        ChooseFirstThemeCard(game, &mode, &gen, &ui);
        STATES_CHECK(game->floorZeroExitOpen, "il reroll confermato non apre l'uscita dopo la scelta del tema");
        game->floorZeroExitCrossed = true;
        { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        STATES_CHECK(mode == APP_GAMEPLAY, "l'attraversamento dopo il reroll non porta a Gameplay");
        STATES_CHECK(game->phase == PHASE_PLAY, "il reroll confermato non ha richiamato GameResetRunWithSeed (fase non tornata a PLAY)");
        STATES_CHECK(game->runSeed != seedBeforeReroll, "il reroll confermato non ha cambiato game->runSeed (stesso seed di prima)");
    }

    /* PauseMenu -> Abbandona run -> ExitConfirm -> (annulla) -> PauseMenu,
       poi di nuovo -> (conferma) -> MainMenu. Sei righe da WP21: "Abbandona
       run" e' ora l'ultima, indice 5. */
    { AppInput in = InputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* PauseMenu, focus su Riprendi */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Build e sinergie */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Prove (WP16) */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Opzioni */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Rigenera la run */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }    /* Abbandona run */
    STATES_CHECK(ui.focus == 5, "la navigazione in PauseMenu non arriva su Abbandona run (indice 5)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "confirm su Abbandona run non apre ExitConfirm");
    STATES_CHECK(ui.exitAbandonsRun, "il contesto di ExitConfirm da PauseMenu non e' 'abbandono run'");
    STATES_CHECK(!ui.exitRerollsRun, "l'abbandono si e' marcato anche come reroll (i due contesti devono essere esclusivi)");
    STATES_CHECK(!ExitConfirmIsLightModalFor(ui.openedFrom, ui.exitDropsSuspendedRun),
        "ExitConfirm da PauseMenu e' stato marcato come dialogo leggero (WP22, deve restare a schermo pieno, DEC-090)");
    STATES_CHECK(ui.focus == 1, "il focus iniziale di ExitConfirm non e' 1 (Annulla)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla */
    STATES_CHECK(mode == APP_PAUSE_MENU, "ExitConfirm/Annulla non torna a PauseMenu");
    STATES_CHECK(ui.focus == 5, "il ritorno da ExitConfirm/Annulla non ripristina il focus su Abbandona run");

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* di nuovo in ExitConfirm */
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "rientro in ExitConfirm fallito");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }      /* Annulla -> Conferma */
    STATES_CHECK(ui.focus == 0, "down da Annulla in ExitConfirm non porta a Conferma");
    /* M7: questo e' il SECONDO chiamante dell'hook (spec, punto 3) --
       abbandono confermato da PauseMenu, game->floor >= 1 per davvero (la run
       e' entrata in Gameplay poco sopra). Stessa controprova su disco delle
       due sopra: la guardia test-safe deve tenere anche col piano davvero
       giocato, non solo nei due casi PHASE_WIN/PHASE_GAME_OVER.
       WP19 (DEC-082/089, 05-game-states-and-flow.md): l'abbandono confermato
       di una run VERA non torna piu' a MainMenu diretto -- chiude come
       sconfitta e passa da RunResults, con le prove ancora TRIAL_IN_PROGRESS
       finalizzate come a fine run vera (WP16). Precondizione verificata qui:
       nessuna prova e' ancora decisa prima della conferma, altrimenti il
       controllo sotto ("nessuna e' rimasta in corso") sarebbe vuoto di
       significato. */
    for (int t = 0; t < game->trialCount; t++)
        STATES_CHECK(game->trials[t].state == TRIAL_IN_PROGRESS, "precondizione: una prova e' gia' decisa prima dell'abbandono, il controllo di finalizzazione sotto non proverebbe nulla");
    int catalogBeforeAbandon = CatalogFileCount();
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_RESULTS, "ExitConfirm/Conferma (abbandono di una run vera) non porta a RunResults");
    STATES_CHECK(game->phase != PHASE_WIN, "l'abbandono confermato non deve mai apparire come vittoria");
    STATES_CHECK(game->runAbandoned, "l'abbandono confermato non ha impostato game->runAbandoned (causa dichiarata assente)");
    STATES_CHECK(CatalogFileCount() == catalogBeforeAbandon, "ExitConfirm/Conferma (abbandono) ha scritto un file in catalog/ (guardia test-safe non attiva)");
    STATES_CHECK(game->catalogRecordsWritten == 0, "ExitConfirm/Conferma (abbandono) ha valorizzato catalogRecordsWritten (guardia test-safe non attiva)");
    for (int t = 0; t < game->trialCount; t++)
        STATES_CHECK(game->trials[t].state != TRIAL_IN_PROGRESS, "l'abbandono confermato ha lasciato una prova ancora TRIAL_IN_PROGRESS (TrialsFinalizeAtRunEnd non chiamata)");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di RunResults dopo l'abbandono non e' 0 (Nuova run subito)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(ui.focus == 1, "down in RunResults (dopo abbandono) non porta a Menu principale (indice 1)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "RunResults/Menu principale (dopo l'abbandono) non torna a MainMenu");
    STATES_CHECK(ui.focus == 0, "il ritorno a MainMenu da RunResults (dopo abbandono) non ripristina il focus su 'Nuova run'");

    /* MainMenu -> Catalogo (M8, DEC-045): niente nuovo AppMode, la vista vive
       dentro APP_MAIN_MENU (spec M8, nota architetturale). Catalogo vuoto per
       costruzione qui (nessun file scritto in questo processo finora:
       'catalogWritesEnabled' resta falso per tutta la durata di questo
       test): verifica il messaggio sobrio via aggregato vuoto, la
       navigazione categorie/voci senza crash su una vista vuota, ed ESC che
       richiude la vista SENZA lasciare MainMenu, col focus che resta su
       "Catalogo" (indice 1). Copre anche l'invariante di M8: la vista NON
       cambia AppMode. */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Nuova run -> Catalogo */
    STATES_CHECK(ui.focus == 1, "down da Nuova run non porta il focus su Catalogo (indice 1)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "confirm su Catalogo cambia AppMode (nessun nuovo stato atteso, spec M8)");
    STATES_CHECK(ui.catalogOpen, "confirm su Catalogo non apre la vista");
    STATES_CHECK(ui.catalogCategory == 0, "l'apertura del Catalogo non riparte dalla prima categoria");
    STATES_CHECK(ui.catalogItemFocus == 0, "l'apertura del Catalogo non riparte dal primo elemento");
    { AppInput in = InputLeft(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(ui.catalogCategory == RUN_CATALOG_CATEGORY_COUNT - 1, "sinistra dalla prima categoria non fa wrap sull'ultima");
    { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(ui.catalogCategory == 0, "destra dall'ultima categoria non fa wrap sulla prima");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* su una categoria vuota: mai un crash, ne' uno spostamento di focus */
    STATES_CHECK(ui.catalogItemFocus == 0, "su/giu' su una categoria vuota ha spostato il focus voce");
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "ESC dal Catalogo lascia APP_MAIN_MENU");
    STATES_CHECK(!ui.catalogOpen, "ESC dal Catalogo non richiude la vista");
    STATES_CHECK(ui.focus == 1, "il ritorno dal Catalogo non lascia il focus su 'Catalogo' (indice 1)");

    /* MainMenu -> Esci -> ExitConfirm -> (conferma) -> UpdateApp ritorna true */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Catalogo -> Opzioni */
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Opzioni -> Esci */
    STATES_CHECK(ui.focus == 3, "la navigazione in MainMenu non arriva su Esci (indice 3)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_EXIT_CONFIRM, "confirm su Esci non apre ExitConfirm");
    STATES_CHECK(!ui.exitAbandonsRun, "il contesto di ExitConfirm da MainMenu non e' 'uscita dal gioco'");
    STATES_CHECK(ExitConfirmIsLightModalFor(ui.openedFrom, ui.exitDropsSuspendedRun),
        "ExitConfirm da MainMenu (chiusura del gioco) non e' riconosciuto come dialogo leggero (WP22, DEC-090, gap G9)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla -> Conferma */
    STATES_CHECK(ui.focus == 0, "down da Annulla in ExitConfirm (quit) non porta a Conferma");
    {
        AppInput in = InputConfirm();
        bool wantsExit = UpdateApp(game, &mode, &gen, &ui, &in);
        STATES_CHECK(wantsExit, "ExitConfirm/Conferma (uscita dal gioco) non fa ritornare true a UpdateApp");
    }

    /* Fase Game vittoria -> RunResults -> Menu principale -> MainMenu.
       M7: 'ui' e' zero-inizializzata sopra (catalogWritesEnabled falso per
       costruzione, mai acceso in questo test) -- il conteggio prima/dopo e'
       la controprova osservabile su disco che la guardia tiene per davvero,
       non solo "il campo e' falso". */
    int catalogBeforeWin = CatalogFileCount();
    mode = APP_GAMEPLAY;
    ui.focus = 0;
    game->phase = PHASE_WIN;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_RESULTS, "PHASE_WIN in Gameplay non porta a RunResults");
    STATES_CHECK(CatalogFileCount() == catalogBeforeWin, "PHASE_WIN sintetico ha scritto un file in catalog/ (guardia test-safe non attiva)");
    STATES_CHECK(game->catalogRecordsWritten == 0, "PHASE_WIN sintetico ha valorizzato catalogRecordsWritten (guardia test-safe non attiva)");
    STATES_CHECK(ui.focus == 0, "il focus iniziale di RunResults non e' 0 (Nuova run subito)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Nuova run subito -> Menu principale */
    STATES_CHECK(ui.focus == 1, "down in RunResults non porta a Menu principale (indice 1)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_MAIN_MENU, "RunResults/Menu principale non torna a MainMenu");

    /* Fase Game sconfitta -> RunResults -> Nuova run subito -> FloorZero (M5:
       nuove proposte, nuova scelta -- l'uscita si apre solo dopo, requisito
       10) -> attraversamento -> Gameplay */
    int catalogBeforeLoss = CatalogFileCount();
    mode = APP_GAMEPLAY;
    ui.focus = 0;
    game->phase = PHASE_GAME_OVER;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_RUN_RESULTS, "PHASE_GAME_OVER in Gameplay non porta a RunResults");
    STATES_CHECK(CatalogFileCount() == catalogBeforeLoss, "PHASE_GAME_OVER sintetico ha scritto un file in catalog/ (guardia test-safe non attiva)");
    STATES_CHECK(game->catalogRecordsWritten == 0, "PHASE_GAME_OVER sintetico ha valorizzato catalogRecordsWritten (guardia test-safe non attiva)");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* focus 0: Nuova run subito */
    STATES_CHECK(mode == APP_FLOOR_ZERO, "RunResults/Nuova run subito non porta a FloorZero");
    STATES_CHECK(!game->floorZeroExitOpen, "l'uscita del Piano 0 dopo RunResults e' aperta prima della scelta del tema");
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    STATES_CHECK(game->floorZeroExitOpen, "l'uscita del Piano 0 dopo RunResults non e' aperta dopo la scelta del tema");
    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    STATES_CHECK(mode == APP_GAMEPLAY, "l'attraversamento dopo RunResults non porta a Gameplay");
    STATES_CHECK(game->phase == PHASE_PLAY, "la nuova run non ha richiamato GameResetRun (fase non tornata a PLAY)");

    /* Cleanup della directory temporanea del test e ripristino del percorso del catalogo */
cleanup_test_catalog:
    RunCatalogSetTestPath(NULL);  /* Ripristina il percorso del catalogo di default */

    /* Rimuovi tutti i file dalla directory temporanea */
    if (testCatalogDir && DirectoryExists(testCatalogDir))
    {
        FilePathList files = LoadDirectoryFilesEx(testCatalogDir, "", false);
        for (unsigned int i = 0; i < files.count; i++)
        {
            remove(files.paths[i]);
        }
        UnloadDirectoryFiles(files);
        /* Rimuovi la directory stessa */
        RemoveTempCatalogTestDir(testCatalogDir);
    }

    return test_success;
}

/* W9 (playtest round 1, "mouse ovunque"): il PASSAGGIO del mouse (nessun
 * click, mai un vero evento hardware, quindi non simulabile sotto Xvfb) sposta
 * gia' il fuoco -- verificato posizionando il cursore VIRTUALE con
 * SetMousePosition (che GetMousePosition riflette subito, a differenza dei
 * pulsanti: nessun equivalente sintetico esiste per quelli) su un punto
 * trovato per scansione con le stesse funzioni di hit-test del renderer
 * (RendererMenuItemAt/RendererBuildItemRowAt/RendererFloorZeroCardAt), poi
 * chiamando UpdateApp con un AppInput VUOTO (nessun tasto, nessun click) e
 * controllando che il campo di focus giusto sia cambiato lo stesso. Tre
 * superfici rappresentative: una voce di MainMenu, una riga di BuildScreen,
 * una carta del pannello del Piano 0 (DEC-075). Il mouse viene riportato
 * nell'angolo (2,2) -- fuori da ogni geometria di menu per costruzione, box
 * centrati -- alla fine di ogni blocco, cosi' non influenza lo scenario
 * successivo. Come GameStatesTest, gira dopo InitWindow e chiama UpdateApp
 * direttamente. */
bool GameMouseHoverFocusTest(Game *game)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

#define HOVER_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameMouseHoverFocusTest: %s\n", (msg)); SetMousePosition(2, 2); return false; } } while (0)

    /* (a) MainMenu: hover su "Opzioni" (indice 2) sposta ui.focus, senza
       cambiare stato (nessun click, nessun tasto). */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_MAIN_MENU;
        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererMenuItemAt(APP_MAIN_MENU, (Vector2){ x, y }, false) == 2) target = (Vector2){ x, y };
        HOVER_CHECK(target.x >= 0.0f, "(a) nessun punto dello schermo colpisce la voce 'Opzioni' di MainMenu");

        SetMousePosition((int)target.x, (int)target.y);
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        HOVER_CHECK(mode == APP_MAIN_MENU, "(a) il solo hover ha cambiato lo stato applicativo");
        HOVER_CHECK(ui.focus == 2, "(a) l'hover su 'Opzioni' non ha spostato ui.focus");
    }
    SetMousePosition(2, 2);

    /* (b) BuildScreen: hover su una riga dell'inventario sposta
       ui.buildItemFocus. Si entra nello stato col TAB da Gameplay (DEC-139),
       la via piu' corta -- il contenuto degli oggetti non conta (vedi il
       commento su BuildScreenItemListLayoutFor in game_renderer.c), basta
       'itemCount'. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_GAMEPLAY;
        int savedCount = game->player.itemCount;
        game->player.itemCount = 3;

        AppInput tabIn = InputTab();
        UpdateApp(game, &mode, &gen, &ui, &tabIn);
        if (mode != APP_BUILD_SCREEN)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(b) TAB da Gameplay non porta a BuildScreen");
        }

        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererBuildItemRowAt(game, &ui, (Vector2){ x, y }) == 1) target = (Vector2){ x, y };
        if (target.x < 0.0f)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(b) nessun punto dello schermo colpisce la riga oggetto 1");
        }

        SetMousePosition((int)target.x, (int)target.y);
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        game->player.itemCount = savedCount;
        HOVER_CHECK(ui.buildItemFocus == 1, "(b) l'hover sulla riga 1 non ha spostato ui.buildItemFocus");
    }
    SetMousePosition(2, 2);

    /* (c) Pannello del Piano 0 (DEC-075): hover su una carta MONDI sposta
       game->themeCardFocus. Si entra in FloorZero senza passare da
       AppEnterFloorZero (che avvierebbe una generazione vera): lo stato
       minimo che il ramo del pannello legge -- 'themeCardCount'/
       'themeCardsPanelOpen'/'floorZeroPanelSection' -- basta e avanza. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_FLOOR_ZERO;
        int savedCount = game->themeCardCount;
        bool savedOpen = game->themeCardsPanelOpen;
        int savedSection = game->floorZeroPanelSection;
        int savedFocus = game->themeCardFocus;
        game->themeCardCount = 3;
        game->themeCardsPanelOpen = true;
        game->floorZeroPanelSection = FLOOR_ZERO_PANEL_WORLDS;
        game->themeCardFocus = 0;

        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererFloorZeroCardAt(game, (Vector2){ x, y }) == 1) target = (Vector2){ x, y };
        if (target.x < 0.0f)
        {
            game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen;
            game->floorZeroPanelSection = savedSection; game->themeCardFocus = savedFocus;
            HOVER_CHECK(false, "(c) nessun punto dello schermo colpisce la carta-mondo 1");
        }

        SetMousePosition((int)target.x, (int)target.y);
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        int gotFocus = game->themeCardFocus;
        game->themeCardCount = savedCount; game->themeCardsPanelOpen = savedOpen;
        game->floorZeroPanelSection = savedSection; game->themeCardFocus = savedFocus;
        HOVER_CHECK(gotFocus == 1, "(c) l'hover sulla carta-mondo 1 non ha spostato game->themeCardFocus");
    }
    SetMousePosition(2, 2);

    /* (d)-(g) W9 correzione round 0 (BOCCIATO): il vero difetto non era che
       l'hover sposti il focus (verificato sopra), ma che lo riscriva ad OGNI
       frame anche quando il puntatore resta FERMO -- la situazione normale
       dopo un click, o quando il giocatore torna a tastiera/pad -- cancellando
       cosi' la navigazione da tastiera/pad (DEC-057) e persino i default non
       distruttivi scelti dal codice. Ogni blocco fa prima un frame di
       "assestamento" (il primo sguardo sul mouse ancora fa hover, per
       costruzione: vedi il commento su AppUi.mouseTracked in game_types.h),
       poi verifica che un frame SUCCESSIVO col mouse ancora li' non
       ricancelli tastiera/pad. */

    /* (d) MainMenu: puntatore fermo su "Nuova run" (indice 0) -- GIU' da
       tastiera deve spostare il focus a 1, e deve RESTARCI su un frame
       successivo senza alcun input (prima della correzione l'hover lo
       riportava a 0 ad ogni frame). */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_MAIN_MENU;
        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererMenuItemAt(APP_MAIN_MENU, (Vector2){ x, y }, false) == 0) target = (Vector2){ x, y };
        HOVER_CHECK(target.x >= 0.0f, "(d) nessun punto dello schermo colpisce la voce 'Nuova run' di MainMenu");

        SetMousePosition((int)target.x, (int)target.y);
        AppInput settle = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &settle);   /* primo sguardo: hover fisiologico */
        HOVER_CHECK(ui.focus == 0, "(d) l'hover al primo sguardo non ha allineato ui.focus a 'Nuova run'");

        AppInput down = InputDown();
        UpdateApp(game, &mode, &gen, &ui, &down);
        HOVER_CHECK(ui.focus == 1, "(d) GIU' da tastiera non ha spostato ui.focus col mouse fermo su 'Nuova run'");

        AppInput none = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &none);
        HOVER_CHECK(ui.focus == 1,
            "(d) il focus scelto da tastiera e' tornato alla voce sotto il puntatore fermo (regressione W9)");
    }
    SetMousePosition(2, 2);

    /* (e) ExitConfirm (la conseguenza piu' distruttiva): il default non
       distruttivo "Annulla" (ui.focus = 1, come lo scrive il codice reale
       alla transizione) deve SOPRAVVIVERE a un frame senza input col
       puntatore fermo sulla riga "Conferma" (indice 0) -- prima della
       correzione, un solo frame di hover ribaltava il default su
       "Conferma", e il primo INVIO/pad-A successivo abbandonava la run (o
       usciva dal gioco) invece di annullare. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_EXIT_CONFIRM;
        Vector2 target = { -1.0f, -1.0f };
        /* 'true': 'ui' e' azzerata, quindi ui.openedFrom vale APP_MAIN_MENU
           (primo valore dell'enum) e UpdateApp interroga la geometria del
           dialogo LEGGERO -- la ricerca del punto deve usare la stessa, o
           colpirebbe una riga che in quel contesto non e' disegnata li'
           (WP22, terza passata). */
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererMenuItemAt(APP_EXIT_CONFIRM, (Vector2){ x, y }, true) == 0) target = (Vector2){ x, y };
        HOVER_CHECK(target.x >= 0.0f, "(e) nessun punto dello schermo colpisce la riga 'Conferma' di ExitConfirm");

        SetMousePosition((int)target.x, (int)target.y);
        AppInput settle = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &settle);   /* primo sguardo: hover fisiologico, irrilevante qui */
        ui.focus = 1;   /* il default che il codice reale scrive alla transizione in ExitConfirm */

        AppInput none = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &none);
        HOVER_CHECK(ui.focus == 1,
            "(e) il default 'Annulla' di ExitConfirm non sopravvive a un frame di hover col mouse fermo (regressione W9)");
    }
    SetMousePosition(2, 2);

    /* (f) BuildScreen: con il puntatore FERMO su una riga dell'inventario,
       ui.buildItemFocus deve restare STABILE per piu' frame senza alcun
       input -- prima della correzione del round 0, l'hover riscriveva il focus
       ad ogni frame anche a mouse fermo. 12 oggetti (finestra scorrevole vera,
       come nella prova del giudice), focus iniziale sull'ultimo.
       Il caso col mouse IN MOVIMENTO -- quello che il round 0 non copriva, e
       che e' proprio la situazione in cui il giocatore usa il mouse -- e' il
       blocco (h) qui sotto. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_GAMEPLAY;
        int savedCount = game->player.itemCount;
        game->player.itemCount = 12;

        AppInput tabIn = InputTab();
        UpdateApp(game, &mode, &gen, &ui, &tabIn);
        if (mode != APP_BUILD_SCREEN)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(f) TAB da Gameplay non porta a BuildScreen");
        }
        ui.buildItemFocus = 11;   /* l'ultimo oggetto: la finestra scorrevole si sposta in fondo */

        int targetRow = -1;
        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
            {
                int row = RendererBuildItemRowAt(game, &ui, (Vector2){ x, y });
                if (row >= 0) { target = (Vector2){ x, y }; targetRow = row; }
            }
        if (target.x < 0.0f)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(f) nessun punto dello schermo colpisce una riga oggetto con la finestra scorrevole a focus=11");
        }
        (void)targetRow;

        SetMousePosition((int)target.x, (int)target.y);
        AppInput settle = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &settle);   /* primo sguardo: hover fisiologico */
        int afterSettle = ui.buildItemFocus;

        bool stable = true;
        for (int i = 0; i < 10 && stable; i++)
        {
            AppInput none = InputNone();
            UpdateApp(game, &mode, &gen, &ui, &none);
            if (ui.buildItemFocus != afterSettle) stable = false;
        }
        game->player.itemCount = savedCount;
        HOVER_CHECK(stable, "(f) ui.buildItemFocus deriva da solo nei frame successivi col mouse fermo (regressione W9)");
    }
    SetMousePosition(2, 2);

    /* (g) Options: con il puntatore FERMO sulla riga "Indietro" (indice 3),
       DESTRA da tastiera deve ancora alzare il volume generale, e INVIO non
       deve chiudere la schermata -- prima della correzione l'hover forzava
       ui.focus a 3 ad ogni frame, quindi "sinistra/destra sotto
       OPTIONS_ROW_BACK" non scattava mai col mouse fermo li' sopra, e
       effective.confirm chiudeva la schermata (contraddice
       docs/design/ui/options-and-accessibility.md, "ENTER esce solo dalla
       riga Indietro"). */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_OPTIONS;
        ui.openedFrom = APP_MAIN_MENU;
        ui.returnFocus = 2;
        float savedVolume = AudioGetMasterVolume();
        AudioSetMasterVolume(0.5f);

        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererMenuItemAt(APP_OPTIONS, (Vector2){ x, y }, false) == 3) target = (Vector2){ x, y };
        if (target.x < 0.0f)
        {
            AudioSetMasterVolume(savedVolume);
            HOVER_CHECK(false, "(g) nessun punto dello schermo colpisce la riga 'Indietro' di Options");
        }

        SetMousePosition((int)target.x, (int)target.y);
        AppInput settle = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &settle);   /* primo sguardo: hover fisiologico, focus->3 */
        if (mode != APP_OPTIONS || ui.focus != 3)
        {
            AudioSetMasterVolume(savedVolume);
            HOVER_CHECK(false, "(g) l'assestamento non ha portato ui.focus a 'Indietro' (precondizione del test non soddisfatta)");
        }
        ui.focus = 0;   /* il giocatore naviga da tastiera sulla riga 'Volume generale' */

        AppInput right = InputRight();
        UpdateApp(game, &mode, &gen, &ui, &right);
        if (mode != APP_OPTIONS || AudioGetMasterVolume() <= 0.5f)
        {
            AudioSetMasterVolume(savedVolume);
            HOVER_CHECK(false,
                "(g) DESTRA col mouse fermo su 'Indietro' non ha alzato il volume generale (regressione W9)");
        }

        AppInput enter = InputConfirm();
        UpdateApp(game, &mode, &gen, &ui, &enter);
        AudioSetMasterVolume(savedVolume);
        HOVER_CHECK(mode == APP_OPTIONS,
            "(g) INVIO su una riga-slider (mouse fermo su 'Indietro') ha chiuso Options (regressione W9)");
    }
    SetMousePosition(2, 2);

    /* (h) W9 correzione round 1 (BOCCIATO, "l'anello di retroazione della lista
       scorrevole"): BuildScreen con il mouse IN MOVIMENTO sopra la lista -- il
       caso che (f) non poteva rilevare, e l'unico che conta davvero (il mouse
       si muove proprio quando il giocatore lo sta usando). Prima della
       correzione la finestra visibile era DERIVATA dal focus ("first = focus -
       maxShow + 1"), quindi lo slot inferiore era l'unico punto fisso della
       mappatura punto->riga e l'hover -- che scrive il focus -- faceva scorrere
       la lista di uno step per ogni frame di MOVIMENTO: con 12 oggetti e una
       finestra da 3, il puntatore appoggiato sullo slot in cima portava la
       lista in cima in ~5 frame, annullando la rotellina appena usata.
       Il puntatore si mette esattamente sullo slot peggiore (la scansione si
       ferma al primo colpo, cioe' la riga in CIMA alla finestra) e poi si
       "tremola" di un pixel ad ogni frame: il movimento e' vero (mouseMoved
       scatta), la riga sotto il puntatore no. Il focus deve restare quello di
       quella riga per tutti i frame. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_GAMEPLAY;
        int savedCount = game->player.itemCount;
        game->player.itemCount = 12;

        AppInput tabIn = InputTab();
        UpdateApp(game, &mode, &gen, &ui, &tabIn);
        if (mode != APP_BUILD_SCREEN)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(h) TAB da Gameplay non porta a BuildScreen");
        }

        /* Lista scorsa in fondo (come dopo qualche giro di rotellina): il focus
           sull'ultimo oggetto, e un frame a vuoto perche' l'ancora lo segua
           (AppBuildScrollFollowFocus in src/app/app.c). Il mouse e' ancora
           nell'angolo, quindi questo frame non fa nessun hover. */
        ui.buildItemFocus = 11;
        AppInput settle = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &settle);

        int targetRow = -1;
        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
            {
                int row = RendererBuildItemRowAt(game, &ui, (Vector2){ x, y });
                if (row >= 0) { target = (Vector2){ x, y }; targetRow = row; }
            }
        if (target.x < 0.0f || targetRow < 0)
        {
            game->player.itemCount = savedCount;
            HOVER_CHECK(false, "(h) nessun punto dello schermo colpisce una riga oggetto con la lista scorsa in fondo");
        }

        bool stable = true;
        int drifted = targetRow;
        for (int i = 0; i < 12 && stable; i++)
        {
            /* Un pixel avanti e uno indietro: movimento VERO, stessa riga. */
            SetMousePosition((int)target.x + (i%2), (int)target.y);
            AppInput none = InputNone();
            UpdateApp(game, &mode, &gen, &ui, &none);
            if (ui.buildItemFocus != targetRow) { stable = false; drifted = ui.buildItemFocus; }
        }
        game->player.itemCount = savedCount;
        if (!stable)
            fprintf(stderr, "GameMouseHoverFocusTest: (h) la lista e' scorsa da sola col mouse in movimento (riga sotto il puntatore %d, focus finito a %d) -- anello di retroazione, regressione W9\n",
                    targetRow, drifted);
        HOVER_CHECK(stable, "(h) ui.buildItemFocus deriva col mouse in MOVIMENTO sulla stessa riga (regressione W9)");
    }
    SetMousePosition(2, 2);

    /* (i) W9 correzione round 1 (BLOCCANTE): quale carta conferma un click nel
       pannello del Piano 0. I pulsanti del mouse non sono simulabili sotto Xvfb
       (nessun equivalente di SetMousePosition per loro), quindi si verifica il
       nucleo PURO che il case APP_FLOOR_ZERO usa (AppFloorZeroCardToConfirm):
       un click su una carta conferma QUELLA carta anche quando il focus e'
       altrove -- il caso del difetto, riproducibile senza input esotici
       (puntatore fermo sull'area del pannello, TAB da tastiera per aprirlo,
       click: 'mouseMoved' falso, nessun hover girato, focus ancora 0). La
       scelta del mondo e' irreversibile (avvia la generazione), quindi
       "conferma la carta sbagliata" non e' un difetto recuperabile. */
    {
        HOVER_CHECK(AppFloorZeroCardToConfirm(2, true, false, 0) == 2,
            "(i) il click su una carta con il focus altrove non conferma la carta cliccata (regressione W9)");
        HOVER_CHECK(AppFloorZeroCardToConfirm(2, true, true, 0) == 2,
            "(i) click e conferma nello stesso frame: deve vincere la carta cliccata");
        HOVER_CHECK(AppFloorZeroCardToConfirm(-1, false, true, 1) == 1,
            "(i) la conferma da tastiera non usa piu' la carta a fuoco");
        HOVER_CHECK(AppFloorZeroCardToConfirm(-1, false, false, 1) == -1,
            "(i) senza click ne' conferma non si deve confermare nulla");
        HOVER_CHECK(AppFloorZeroCardToConfirm(-1, true, false, 1) == -1,
            "(i) un click FUORI dalle carte non deve confermare la carta a fuoco");
    }

    /* (j) WP16, seconda tornata (bocciatura del giudice): il pannello "Prove"
       e' un ramo di sola lettura DENTRO APP_PAUSE_MENU (nessuna geometria
       propria, come il pannello Catalogo dentro APP_MAIN_MENU al blocco (a))
       -- mentre e' aperto (ui.pauseTrialsOpen) i rettangoli delle righe di
       menu (6 da WP21, 5 all'epoca di questo test) restano vivi SOTTO al
       pannello per RendererMenuItemAt: senza la guardia in src/app/app.c (la
       stessa esclusione di 'catalogOpen') un puntatore fermo su una riga
       qualunque riscriverebbe ui.focus dietro al pannello ad ogni frame.
       Verificato: prima di questa correzione, questo test falliva davvero
       (l'hover su 'Abbandona run', allora indice 4 -- oggi "Rigenera la run",
       WP21 -- spostava ui.focus da 2 a 4). L'indice 4 resta un punto di
       osservazione valido: esiste ancora, solo il nome della voce li' e'
       cambiato. */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_PAUSE_MENU;
        ui.pauseTrialsOpen = true;
        ui.focus = 2;   /* "Prove": il focus con cui il codice reale apre il pannello */

        Vector2 target = { -1.0f, -1.0f };
        for (float y = 0.0f; y < sh && target.x < 0.0f; y += 2.0f)
            for (float x = 0.0f; x < sw && target.x < 0.0f; x += 2.0f)
                if (RendererMenuItemAt(APP_PAUSE_MENU, (Vector2){ x, y }, false) == 4) target = (Vector2){ x, y };
        HOVER_CHECK(target.x >= 0.0f, "(j) nessun punto dello schermo colpisce la riga 4 ('Rigenera la run') di PauseMenu");

        SetMousePosition((int)target.x, (int)target.y);
        AppInput in = InputNone();
        UpdateApp(game, &mode, &gen, &ui, &in);
        HOVER_CHECK(mode == APP_PAUSE_MENU, "(j) il solo hover sul pannello Prove ha cambiato lo stato applicativo");
        HOVER_CHECK(ui.pauseTrialsOpen, "(j) il solo hover ha richiuso il pannello Prove");
        HOVER_CHECK(ui.focus == 2,
            "(j) l'hover su una riga di menu sotto il pannello Prove ha riscritto ui.focus (regressione WP16)");
    }
    SetMousePosition(2, 2);

#undef HOVER_CHECK
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
            if (!item->name[0]) return false;
            /* Tassonomia a 4 categorie (task "melting-gen emette e valida le
               4 categorie", 2026-07-27): i 3 oggetti normali del piano ora
               ricevono un MIX delle 4 categorie
               (docs/design/systems/items-pools-and-rarity.md), non piu'
               sempre passivi -- questo test verificava prima solo la
               mappatura di compatibilita' "kind=active senza ricarica ->
               ITEM_PASSIVE"; ora verifica invece l'invariante vero per
               OGNI categoria: uno stat-up nel pool normale non ha
               comportamento mini-VM (stessa regola del bossItem sotto, mai
               una riga ".script="; RunContentLoad in run_content.c azzera
               esplicitamente item->script quando il kind risolve a
               ITEM_STATUP, altrimenti il per-key-fallback lascerebbe lo
               script procedurale sottostante), ogni altra categoria ne ha
               sempre uno (nessun oggetto senza comportamento esce da
               melting-gen), e un attivo vero dichiara sempre cariche o
               cooldown (active-items.md). */
            if (item->kind == ITEM_STATUP)
            {
                if (item->script[0] != '\0') return false;
                /* Bloccante round 0 (stessa revisione): uno stat-up del pool
                   normale non porta MAI il tipo di colpo del piano -- stessa
                   regola sopra, applicata al campo "comportamento" gemello
                   (shotType invece di script). Prova il round-trip vero
                   Item.shotType, non solo il manifest testuale (quello lo
                   copre scripts/test-gen.sh via grep). Difesa applicata sia a
                   monte (melting-gen non assegna piu' shotItem a una
                   posizione statup, gen_fallback.c/gen_validate.c) sia qui a
                   valle (run_content.c azzera item->shotType.active per
                   ITEM_STATUP): questo test verifica il risultato finale
                   qualunque delle due reti l'abbia garantito. */
                if (item->shotType.active) return false;
            }
            else if (!strchr(item->script, ':'))
            {
                return false;
            }
            if (item->kind == ITEM_ACTIVE && item->charges <= 0 && item->cooldown <= 0.0f) return false;
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
    /* W8: questo test verifica il ripiego dell'ATLAS generato, e la sua
       controprova sul giocatore e' un pixel bianco preciso -- la testa dello
       stickman di DrawBaseStickman. Da W8 il giocatore si disegna col suo
       spritesheet quando assets/art/ c'e', quindi quel pixel non e' piu' bianco
       e l'assert sarebbe diventato falso pur essendo il gioco MIGLIORATO.
       Si punta quindi il pacchetto artistico su una cartella inesistente per la
       durata del test: cosi' si esercita di proposito il gradino piu' basso
       della priorita' delle immagini (nessun originale, nessun atlas utile ->
       primitive geometriche), che e' esattamente cio' che questo test esiste per
       proteggere. Il gradino ALTO (sprite presenti) e' coperto da
       --art-atlas-test e dagli screenshot di W8.
       Ripristinato SEMPRE prima di ogni return, anche sui rami di errore. */
    ArtAtlasSetTestDir("generated/mai-esistita-art-fallback");

    const char *testPath = "generated/test_atlas_fallback.png";
    Image img = GenImageColor(ATLAS_CELL*ATLAS_COLS, ATLAS_CELL*ATLAS_COLS, WHITE);
    ImageDrawRectangle(&img, (SPR_PLAYER%ATLAS_COLS)*ATLAS_CELL, (SPR_PLAYER/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    ImageDrawRectangle(&img, (SPR_ENEMY_CHASER%ATLAS_COLS)*ATLAS_CELL, (SPR_ENEMY_CHASER/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    ImageDrawRectangle(&img, (SPR_EXIT%ATLAS_COLS)*ATLAS_CELL, (SPR_EXIT/ATLAS_COLS)*ATLAS_CELL, ATLAS_CELL, ATLAS_CELL, BLANK);
    bool exported = ExportImage(img, testPath);
    UnloadImage(img);
    if (!exported) { ArtAtlasSetTestDir(NULL); return false; }

    GameUnloadAssets(game);
    snprintf(game->content.atlasPath, sizeof(game->content.atlasPath), "%s", testPath);
    AssetsLoad(game);
    remove(testPath);

    if (!game->atlasLoaded ||
        game->atlasCellPresent[SPR_PLAYER] ||                     /* celle vuote: devono risultare assenti */
        game->atlasCellPresent[SPR_ENEMY_CHASER] ||
        game->atlasCellPresent[SPR_EXIT] ||
        !game->atlasCellPresent[SPR_ITEM])                        /* cella piena: deve risultare presente */
    {
        ArtAtlasSetTestDir(NULL);
        return false;
    }

    /* DEC-200: le coordinate di MONDO non coincidono piu' con quelle del
       canvas -- la telecamera traduce (e con un canvas 640x360 piu' piccolo
       della cella inquadrata traduce DAVVERO, anche in una stanza 1x1). I due
       punti di sonda si scelgono quindi DENTRO l'inquadratura corrente invece
       che a coordinate di mondo fisse: prima di WP-UI-0 la vista era tutta la
       cella e qualunque punto della stanza era per forza visibile, ora un
       letterale come (150,200) puo' cadere fuori dallo schermo e il test
       misurerebbe due pixel di pavimento identici.
       WorldCameraView e' la stessa fonte che RendererDrawApp usa per la
       Camera2D, quindi qui non si duplica nessuna formula. */
    Rectangle view = WorldCameraView(game);
    Vector2 enemyPos = { view.x + view.width*0.25f, view.y + view.height*0.35f };
    Vector2 exitPos = { view.x + view.width*0.72f, view.y + view.height*0.62f };
    /* LoadImageFromTexture su una RenderTexture2D legge i pixel col
       framebuffer OpenGL grezzo, che e' memorizzato capovolto rispetto alle
       coordinate con cui si e' disegnato (stesso motivo per cui
       RendererDrawApp usa un'altezza negativa quando ricompone il canvas
       sullo schermo): riga 0 dell'immagine letta corrisponde al FONDO del
       canvas disegnato, non all'alto. Le coordinate vanno quindi prima
       tradotte da mondo a canvas (meno l'origine dell'inquadratura) e poi
       capovolte in verticale. */
    int enemyImgX = (int)(enemyPos.x - view.x);
    int exitImgX = (int)(exitPos.x - view.x);
    int enemyImgY = SCREEN_HEIGHT - 1 - (int)(enemyPos.y - view.y);
    int exitImgY = SCREEN_HEIGHT - 1 - (int)(exitPos.y - view.y);

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    /* Primo render SENZA nemico ne' pickup extra (solo cio' che GameResetRun
       ha gia' piazzato viene rimosso da EntitiesClear): cattura il pixel di
       sfondo esatto (pavimento o riga diagonale della griglia, non importa
       quale) in ciascuna posizione candidata, cosi' il confronto sotto non
       deve indovinare la geometria della griglia. */
    EntitiesClear(game);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, false, NULL, NULL);
    Image before = LoadImageFromTexture(canvas.texture);
    Color enemyBefore = GetImageColor(before, enemyImgX, enemyImgY);
    Color exitBefore = GetImageColor(before, exitImgX, exitImgY);
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
    /* Stessa traduzione mondo -> canvas -> immagine capovolta delle due sonde
       sopra. Fino a WP-UI-0 questa riga leggeva le coordinate di mondo grezze
       e passava per coincidenza (la sagoma di riserva del giocatore e' un
       unico colore abbastanza esteso da coprire anche il pixel sbagliato): non
       era una controprova valida, e con la telecamera che traduce davvero non
       lo sarebbe piu' nemmeno per caso. */
    Color headPixel = GetImageColor(after, (int)(game->player.pos.x - view.x),
                                    SCREEN_HEIGHT - 1 - (int)(game->player.pos.y - 30.0f - view.y));
    Color enemyAfter = GetImageColor(after, enemyImgX, enemyImgY);
    Color exitAfter = GetImageColor(after, exitImgX, exitImgY);
    UnloadImage(after);
    UnloadRenderTexture(canvas);

    bool playerDrew = headPixel.r > 200 && headPixel.g > 200 && headPixel.b > 200;
    bool enemyDrew = ColorChannelDiff(enemyBefore, enemyAfter) > 40;
    bool exitDrew = ColorChannelDiff(exitBefore, exitAfter) > 40;

    ArtAtlasSetTestDir(NULL);
    return playerDrew && enemyDrew && exitDrew;
}

/* Il personaggio a strati (fase 3, vedi docs/engineering/specs/2026-07-13-
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

/* WP22 (DEC-090, gap G9 ui-cornice, seconda e TERZA passata): il dialogo
   "MainMenu -> Esci" deve restare un dialogo modale LEGGERO -- MainMenu ancora
   disegnato e leggibile dietro, un SOLO velo di oscuramento (mai due sommati)
   -- a differenza degli altri TRE contesti di ExitConfirm (abbandono dal Piano
   0, abbandono di una run da PauseMenu, reroll di WP21/DEC-114), che restano a
   schermo pieno come sempre (DEC-090). La prima passata di questo lavoro si
   limitava ad asserire 'ExitConfirmIsLightModalFor(openedFrom)'
   (--states-test): tautologico, perche' quel nucleo puro non dice nulla sul
   FRAME VERO (ne' sul velo, ne' sul fatto che MainMenu sia davvero
   ridisegnato). Questo test campiona invece i pixel di RendererDrawApp REALE,
   tre catture:

   (1) 'raw'   -- APP_GAMEPLAY, nessun overlay: la scena nuda.
   (2) 'light' -- APP_EXIT_CONFIRM aperto da APP_MAIN_MENU (il SOLO contesto
                  leggero, DEC-090).
   (3) 'full'  -- APP_EXIT_CONFIRM aperto da APP_FLOOR_ZERO (contesto a
                  schermo pieno, invariato): riferimento per il confronto,
                  sulla STESSA scena di 'game' (nessuna mutazione fra le tre).

   DEC-200 (WP-UI-0) cambia DUE cose in questo test, entrambe in meglio.
   - DOVE si leggono i pixel. Fino a WP-UI-0 gli overlay si disegnavano
     direttamente nel framebuffer di finestra e il canvas conteneva solo la
     scena, quindi il test doveva leggere lo SCHERMO (LoadImageFromScreen) e
     leggere il canvas avrebbe dato tre catture identiche (falso positivo preso
     davvero in sviluppo). Ora il frame INTERO vive nel canvas: si legge quello,
     e le coordinate non dipendono piu' dalla finestra di Xvfb.
   - COME si scelgono le coordinate. Non piu' letterali ricostruiti a mano
     ("cx - 235*uiScale") ma la geometria VERA, interrogata a
     RendererMenuBoxBounds/RendererMenuItemBounds: se i riquadri cambiano, il
     test continua a guardare il punto giusto invece di misurare pavimento.

   Verifiche:
   (0) Nucleo puro, nessun rendering: una coordinata che cade sulla riga 0 di
       MainMenu ("Nuova run") deve restare FUORI da qualunque voce di
       ExitConfirm interrogata con la STESSA geometria che il frame disegna
       ('exitConfirmLight = true'). Fallisce se il dialogo leggero tornasse
       largo quanto il menu che gli sta dietro (il difetto contestato dal
       giudice in WP22).
   (1) La luminanza media di 'corner' (2,2 -- fuori da qualunque riquadro in
       entrambi i contesti) deve risultare PIU' CHIARA in 'light' che in
       'full': un rapporto teorico di 165/65 = 2.54x fra i due veli (90 e 190
       su 255). Fallisce SUBITO se l'alfa del velo leggero tornasse a 190 (il
       difetto principale contestato dal giudice: 190+90 compositi erano PIU'
       scuri del 190 di prima di WP22).
   (2) MainMenu e' DAVVERO disegnato sotto il dialogo leggero. Il segnale e' la
       barra del fuoco della riga 0 (UI_FOCUS, fiamma 224/91/35, larga 4 px --
       vedi UiMenuRow in src/render/ui_theme.c): un colore della tavolozza
       dell'interfaccia, saturo e rosso-dominante, che NON puo' comparire per
       caso in quel punto se il menu non viene ridisegnato -- li' sotto ci
       sarebbe la sola scena attenuata dal velo. Sostituisce il confronto sulla
       CROMA della passata precedente, che stimava "quanto sarebbe colorato il
       pixel col solo velo" e dipendeva dalla scena generata: ora che
       l'interfaccia ha una tavolozza fissa (WP-UI-0) il segnale si puo'
       riconoscere direttamente, senza stime.
   (3) (terza passata) Nel contesto a schermo pieno la DOMANDA deve stare
       dentro il pannello: nessun pixel chiaro, nella fascia in cui e'
       disegnata, oltre il bordo destro del box. Vedi il commento sul posto
       (l'unica cosa che li' puo' essere chiara e' il testo: fuori dal box il
       velo a 190/255 non lascia passare abbastanza luce). Fallisce se si
       togliesse WrapTextLines da DrawExitConfirmOverlay, o se il box tornasse
       stretto anche in questo contesto -- i due modi in cui il testo era
       finito fuori dal riquadro (rispettivamente da sempre e nella seconda
       passata di WP22). */
/* Il frame appena disegnato, in coordinate di DISEGNO: LoadImageFromTexture
   legge il framebuffer OpenGL grezzo, memorizzato dal basso verso l'alto, e il
   ribaltamento qui evita di spargere "SCREEN_HEIGHT - 1 - y" per tutto il
   test. */
static Image ExitConfirmCaptureCanvas(RenderTexture2D canvas)
{
    Image shot = LoadImageFromTexture(canvas.texture);
    ImageFlipVertical(&shot);
    return shot;
}

static float ExitConfirmLuminance(Color c)
{
    return ((float)c.r + (float)c.g + (float)c.b)/3.0f;
}

bool GameExitConfirmLightModalTest(Game *game)
{
    float uiScale = UiComputeLayout().uiScale;

    Rectangle row0 = RendererMenuItemBounds(APP_MAIN_MENU, 0, false);
    Rectangle exitFullBox = RendererMenuBoxBounds(APP_EXIT_CONFIRM, false);

    int cornerX = 2, cornerY = 2;
    /* Sulla riga 0 di MainMenu, appena dentro il suo bordo sinistro: e' il
       lato da cui il riquadro (piu' stretto) di ExitConfirm si ritira. */
    int row0X = (int)row0.x + 2;
    int row0Y = (int)(row0.y + row0.height*0.5f);
    /* La barra del fuoco occupa i primi 4 px della riga a fuoco. */
    int focusBarX = (int)row0.x + 1;

    /* (0) Nucleo puro, nessun rendering. */
    if (RendererMenuItemAt(APP_MAIN_MENU, (Vector2){ (float)row0X, (float)row0Y }, false) != 0)
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: (%d,%d) non cade sulla riga 0 di MainMenu (precondizione geometrica)\n", row0X, row0Y);
        return false;
    }
    if (RendererMenuItemAt(APP_EXIT_CONFIRM, (Vector2){ (float)row0X, (float)row0Y }, true) != -1)
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: (%d,%d) cade DENTRO una voce di ExitConfirm -- il suo box non e' piu' stretto di quello di MainMenu (WP22, DEC-090, gap G9)\n", row0X, row0Y);
        return false;
    }

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, false, NULL, NULL);
    Image raw = ExitConfirmCaptureCanvas(canvas);
    Color sceneFocusBar = GetImageColor(raw, focusBarX, row0Y);
    UnloadImage(raw);

    AppUi uiLight = { 0 };
    uiLight.openedFrom = APP_MAIN_MENU;
    uiLight.returnFocus = 0;   /* riga 0 "Nuova run" a fuoco: e' la barra che la verifica (2) cerca */
    uiLight.focus = 1;
    if (!ExitConfirmIsLightModalFor(uiLight.openedFrom, uiLight.exitDropsSuspendedRun))
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: ExitConfirmIsLightModalFor(APP_MAIN_MENU) non e' vero (precondizione)\n");
        UnloadRenderTexture(canvas);
        return false;
    }
    RendererDrawApp(game, canvas, APP_EXIT_CONFIRM, &uiLight, false, NULL, NULL);
    Image light = ExitConfirmCaptureCanvas(canvas);
    Color cornerLight = GetImageColor(light, cornerX, cornerY);
    Color focusBarLight = GetImageColor(light, focusBarX, row0Y);
    UnloadImage(light);

    AppUi uiFull = { 0 };
    uiFull.openedFrom = APP_FLOOR_ZERO;   /* contesto a schermo pieno, invariato (DEC-090) */
    uiFull.exitAbandonsRun = true;
    uiFull.returnFocus = 0;
    uiFull.focus = 1;
    if (ExitConfirmIsLightModalFor(uiFull.openedFrom, uiFull.exitDropsSuspendedRun))
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: ExitConfirmIsLightModalFor(APP_FLOOR_ZERO) e' vero (precondizione, deve restare a schermo pieno)\n");
        UnloadRenderTexture(canvas);
        return false;
    }
    RendererDrawApp(game, canvas, APP_EXIT_CONFIRM, &uiFull, false, NULL, NULL);
    Image full = ExitConfirmCaptureCanvas(canvas);
    Color cornerFull = GetImageColor(full, cornerX, cornerY);
    Color focusBarFull = GetImageColor(full, focusBarX, row0Y);
    /* (3) La domanda deve STARE dentro il pannello. Si cerca il pixel chiaro
       (luminanza >= 100) piu' a destra nella fascia della domanda, su TUTTA la
       larghezza del canvas: fuori dal box, in questo contesto, non ci puo'
       essere nulla di cosi' chiaro (il velo a 190/255 lascia passare al
       massimo il 25,5% della scena, cioe' meno di 65 di luminanza anche su un
       bianco pieno), quindi un pixel chiaro oltre il bordo destro del box e'
       per forza testo sconfinato. Fascia: 45..112 sotto la cima del box nella
       griglia storica (la domanda comincia a EXIT_CONFIRM_QUESTION_Y_BASE=52 e
       occupa al massimo tre righe da 20 con glifi alti 15, vedi
       game_renderer.c), riportata sul canvas da uiScale.
       Prima della terza passata di WP22 il testo usciva SEMPRE: 849 px di
       domanda contro i 520 px di spazio utile del pannello. Ora WrapTextLines
       lo manda a capo. */
    int questionMaxX = -1;
    int bandTop = (int)(exitFullBox.y + 45.0f*uiScale);
    int bandBottom = (int)(exitFullBox.y + 112.0f*uiScale);
    for (int y = bandTop; y < bandBottom; y++)
    {
        for (int x = SCREEN_WIDTH - 1; x > questionMaxX; x--)
        {
            Color c = GetImageColor(full, x, y);
            if (((int)c.r + (int)c.g + (int)c.b)/3 >= 100) { questionMaxX = x; break; }
        }
    }
    UnloadImage(full);

    UnloadRenderTexture(canvas);

    int boxRightFull = (int)(exitFullBox.x + exitFullBox.width);
    if (questionMaxX > boxRightFull)
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: la domanda a schermo pieno arriva a x=%d, oltre il bordo destro del pannello (x=%d) -- il testo sconfina dal riquadro (WP22, terza passata: deve andare a capo con WrapTextLines)\n",
                questionMaxX, boxRightFull);
        return false;
    }

    /* (1) vedi il commento sopra la funzione. */
    float lumLight = ExitConfirmLuminance(cornerLight);
    float lumFull = ExitConfirmLuminance(cornerFull);
    if (!(lumLight > lumFull + 3.0f))
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: luminanza fuori dal box (%.1f contesto leggero, %.1f contesto a schermo pieno) non e' piu' chiara nel contesto leggero (WP22, DEC-090)\n", lumLight, lumFull);
        return false;
    }

    /* (2) La barra del fuoco della riga 0 di MainMenu, vista attraverso il velo
       leggero: rosso nettamente dominante (UI_FOCUS e' 224/91/35 -- un velo
       nero e' una moltiplicazione uniforme, quindi il dominio del rosso
       sopravvive a qualunque alfa) e molto piu' chiara dello stesso punto nel
       contesto a schermo pieno, dove il MainMenu non c'e'. */
    if (!(focusBarLight.r > focusBarLight.g + 25 && focusBarLight.r > focusBarLight.b + 40))
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: il pixel della barra del fuoco nel dialogo leggero (%d,%d,%d) non e' il rosso-fiamma di UI_FOCUS -- MainMenu forse non e' piu' disegnato come sfondo (WP22, DEC-090, gap G9). Scena nuda nello stesso punto: (%d,%d,%d)\n",
                focusBarLight.r, focusBarLight.g, focusBarLight.b, sceneFocusBar.r, sceneFocusBar.g, sceneFocusBar.b);
        return false;
    }
    if (!(ExitConfirmLuminance(focusBarLight) > ExitConfirmLuminance(focusBarFull) + 20.0f))
    {
        fprintf(stderr, "GameExitConfirmLightModalTest: la barra del fuoco e' luminosa quanto lo stesso punto senza MainMenu dietro (%.1f contro %.1f): il contesto leggero non sta ridisegnando il menu (WP22, DEC-090)\n",
                ExitConfirmLuminance(focusBarLight), ExitConfirmLuminance(focusBarFull));
        return false;
    }

    return true;
}

/* WP22 (terza passata, ui/run-setup.md, "Elementi interattivi"): la riga
   informativa "Modalita': Standard" di RunSetup.
   Perche' esiste questo test: la specifica del work package chiedeva quella
   riga, la seconda passata l'aveva dichiarata "gia' presente e verificata", ma
   NESSUNA suite la copriva -- cancellando la UiText in DrawRunSetupOverlay
   make test restava interamente verde (difetto contestato dal giudice). Due
   verifiche, come per GameExitConfirmLightModalTest sopra:

   (0) Nucleo puro, nessun rendering: la fascia occupata dalla riga
       (RendererRunSetupModeLabelBand, la STESSA che il disegno usa) non
       deve contenere NESSUNA voce di menu -- RendererMenuItemAt deve
       rispondere -1 su tutta la fascia. Fallisce se la riga diventasse
       selezionabile (una DrawMenuRow al posto della UiText: la fascia
       coinciderebbe con una voce), e fallisce anche se la riga tornasse alla
       vecchia quota +142, dentro la fascia della voce "Seed" (110..150) --
       che era il difetto di posizione contestato dal giudice.
       La contro-prova che il metodo sia sensibile ("una voce esiste e viene
       riconosciuta") sta nella riga sotto: il centro della voce 0 DEVE
       rispondere 0.
   (1) Pixel di un frame VERO di RendererDrawApp in APP_RUN_SETUP: si contano
       i pixel CHIARI (luminanza >= 100 -- il testo e' 176,184,198 su un
       pannello quasi nero, 14,16,22) dentro la fascia della riga e dentro una
       fascia di CONTROLLO identica per dimensioni subito sopra, dove non si
       disegna nulla. La riga e' presente solo se la prima ne ha molti e la
       seconda quasi nessuno: cancellando la UiText il conteggio della fascia
       crolla a zero e il test fallisce. La fascia di controllo non e' un
       ornamento -- e' cio' che rende la soglia una MISURA e non un numero
       indovinato: se un giorno il pannello diventasse chiaro, fallirebbero
       entrambe le fasce e il test lo direbbe invece di passare per caso. */
bool GameRunSetupModeLineTest(Game *game)
{
    /* DEC-200: tutto in coordinate di CANVAS. Fino a WP-UI-0 questo test
       chiedeva la fascia passando la taglia della FINESTRA e poi leggeva i
       pixel dallo SCHERMO: due spazi diversi mescolati, che davano un
       rettangolo centrato ma misurato in pixel di canvas -- il conteggio
       finiva su una zona vicina a quella giusta e passava per fortuna. */
    float sw = (float)SCREEN_WIDTH;
    float sh = (float)SCREEN_HEIGHT;
    Rectangle band = RendererRunSetupModeLabelBand();

    const char *label = RendererRunSetupModeLabel();
    if (!label || label[0] == '\0')
    {
        fprintf(stderr, "GameRunSetupModeLineTest: l'etichetta della riga Modalita' e' vuota\n");
        return false;
    }

    /* (0) nucleo puro. */
    for (float y = band.y + 1.0f; y < band.y + band.height; y += 2.0f)
    {
        for (float x = band.x + 1.0f; x < band.x + band.width; x += 4.0f)
        {
            int hit = RendererMenuItemAt(APP_RUN_SETUP, (Vector2){ x, y }, false);
            if (hit >= 0)
            {
                fprintf(stderr, "GameRunSetupModeLineTest: il punto (%.0f,%.0f) della fascia 'Modalita' cade sulla voce di menu %d -- la riga deve restare NON selezionabile e non sovrapporsi ad alcuna voce (ui/run-setup.md)\n", x, y, hit);
                return false;
            }
        }
    }
    /* Contro-prova: il metodo sa riconoscere una voce quando c'e' davvero. */
    {
        bool sawRow0 = false;
        for (float y = 0.0f; y < sh && !sawRow0; y += 2.0f)
            for (float x = 0.0f; x < sw && !sawRow0; x += 2.0f)
                if (RendererMenuItemAt(APP_RUN_SETUP, (Vector2){ x, y }, false) == 0) sawRow0 = true;
        if (!sawRow0)
        {
            fprintf(stderr, "GameRunSetupModeLineTest: nessun punto dello schermo colpisce la voce 0 di RunSetup (precondizione: il test non saprebbe distinguere 'non selezionabile' da 'geometria assente')\n");
            return false;
        }
    }

    /* (1) pixel del frame vero. */
    RenderTexture2D canvas = LoadRenderTexture((int)sw, (int)sh);
    AppUi ui = { 0 };
    ui.seed = 12345u;
    ui.focus = 1;   /* "Avvia": il fuoco NON sta sulla riga sopra la fascia, cosi' l'evidenziazione di una voce non puo' contaminare il conteggio */
    RendererDrawApp(game, canvas, APP_RUN_SETUP, &ui, false, NULL, NULL);
    /* Il frame intero vive nel canvas da DEC-200: si legge quello (capovolto,
       vedi ExitConfirmCaptureCanvas sopra) invece dello schermo. */
    Image frame = LoadImageFromTexture(canvas.texture);
    ImageFlipVertical(&frame);
    UnloadRenderTexture(canvas);

    int bright = 0, brightControl = 0;
    for (int y = 0; y < (int)band.height; y++)
    {
        for (int x = 0; x < (int)band.width; x++)
        {
            Color c = GetImageColor(frame, (int)band.x + x, (int)band.y + y);
            if (((int)c.r + (int)c.g + (int)c.b)/3 >= 100) bright++;
            Color k = GetImageColor(frame, (int)band.x + x, (int)band.y - (int)band.height + y);
            if (((int)k.r + (int)k.g + (int)k.b)/3 >= 100) brightControl++;
        }
    }
    UnloadImage(frame);

    if (bright < 40)
    {
        fprintf(stderr, "GameRunSetupModeLineTest: solo %d pixel chiari nella fascia della riga 'Modalita' (%.0f,%.0f %.0fx%.0f) -- la riga non risulta disegnata nel frame (ui/run-setup.md, tabella 'Elementi interattivi')\n",
                bright, band.x, band.y, band.width, band.height);
        return false;
    }
    if (bright < brightControl*4 + 20)
    {
        fprintf(stderr, "GameRunSetupModeLineTest: la fascia della riga 'Modalita' (%d pixel chiari) non si distingue dalla fascia vuota di controllo (%d) -- soglia non discriminante\n",
                bright, brightControl);
        return false;
    }

    return true;
}

/* Fase 3b VISIVA (docs/engineering/specs/2026-07-13-pools-rarity-design.md,
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
        it->kind = ITEM_PASSIVE;
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
        pickupItem.kind = ITEM_PASSIVE;
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

/* Step C (docs/engineering/specs/2026-07-14-step-c-shottype-balance.md): il
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
    item->kind = ITEM_PASSIVE;
    item->color = game->theme.accent;
    ShotTypeExample(&item->shotType, 0);

    Item *homing = &game->player.items[1];
    homing->active = true;
    snprintf(homing->name, sizeof(homing->name), "Occhio Rapace");
    homing->slot = SLOT_EYES;
    homing->rarity = RARITY_UNCOMMON;
    homing->kind = ITEM_PASSIVE;
    homing->color = game->theme.accent2;
    homing->traits = TRAIT_HOMING;

    Item *pierce = &game->player.items[2];
    pierce->active = true;
    snprintf(pierce->name, sizeof(pierce->name), "Punteruolo Lungo");
    pierce->slot = SLOT_BACK;
    pierce->rarity = RARITY_UNCOMMON;
    pierce->kind = ITEM_PASSIVE;
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

/* DEC-184 (ui/hud.md, "Blocco statistiche"), SOLO manuale
   (--hud-stats-screenshot-test, mai in make test, stessa tradizione di
   GameRarityScreenshotTest/GameShotFormsScreenshotTest sopra). Impone sul
   Player valori non-default e ben distinguibili l'uno dall'altro per le sei
   statistiche del blocco -- danno, cadenza, vel. colpo, vel. movimento,
   raggio, Fortuna -- cosi' lo screenshot mostra sei numeri diversi e non un
   campo azzerato per errore. Due scatti con lo STESSO Player: uno col blocco
   visibile (default, AppUi.hudStatsHidden falso) e uno col blocco nascosto
   dopo il toggle C, per verificare a occhio che DEC-184 "tasto di toggle" e
   "visibile di default" reggano entrambi nello stesso frame di riferimento. */
bool GameHudStatsScreenshotTest(Game *game)
{
    Player *p = &game->player;
    p->damage = 12.5f;
    p->fireDelay = 0.35f;
    p->shotSpeed = 480.0f;
    p->speed = 220.0f;
    p->shotRadius = 9.5f;
    p->luck = 3.5f;

    AppUi ui = { 0 };   /* hudStatsHidden falso: blocco visibile di default (DEC-184) */
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_GAMEPLAY, &ui, true, NULL, "logs/worldsmelt-hud-stats-visible-screen.png");
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);

    /* Stesso identico Player, stesso identico frame: la SOLA differenza fra i
       due scatti e' il toggle -- se qualcos'altro cambiasse fra i due file non
       sarebbe piu' un confronto valido "visibile vs nascosto". */
    AppMode mode = APP_GAMEPLAY;
    AppGen gen = { 0 };
    { AppInput in = { 0 }; in.toggleStats = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    if (!ui.hudStatsHidden)
    {
        fprintf(stderr, "GameHudStatsScreenshotTest: il toggle C non ha nascosto il blocco statistiche\n");
        return false;
    }

    RenderTexture2D canvas2 = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas2, APP_GAMEPLAY, &ui, true, NULL, "logs/worldsmelt-hud-stats-hidden-screen.png");
    bool textureValid2 = canvas2.texture.id != 0;
    UnloadRenderTexture(canvas2);

    return textureValid && textureValid2;
}

/* M1b/M5/M6a, SOLO manuale (--floor-zero-screenshot-test, mai in make test:
   stessa tradizione degli screenshot sopra, "per l'occhio del proprietario").
   Gen disabilitata: le carte curate lato gioco compaiono SUBITO all'ingresso
   (AppUseFallbackThemeCards, vedi AppEnterFloorZero in src/app/app.c) -- il
   pannello si apre a mano (TAB) cosi' lo screenshot mostra DAVVERO le carte
   (requisito 13 della spec M5), non solo l'indicatore testuale. M6a: due
   scatti, uno per sezione del pannello combinato (MONDI/PERSONAGGI). */
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
    /* Sezione MONDI, quella con cui il pannello si apre di default
       (FloorZeroEnter, floorZeroPanelSection = FLOOR_ZERO_PANEL_WORLDS). */
    RendererDrawApp(game, canvas, APP_FLOOR_ZERO, &ui, true, &status, "logs/worldsmelt-floorzero-screen.png");
    bool worldsTextureValid = canvas.texture.id != 0;

    /* M6b-1 (DEC-014, prima fetta): niente propose vero qui (gen disabilitata,
       come il resto di questo test) -- si inietta un personaggio generato
       FINTO direttamente nel canale dati dinamico (Game.generatedCharacter/
       generatedCharacterValid), esattamente come farebbe
       AppLoadCharacterProposal su un file vero, cosi' lo screenshot mostra
       ANCHE il quarto slot dinamico (spec, "--floor-zero-screenshot-test: la
       sezione PERSONAGGI con la quarta carta (fake), per l'occhio del
       proprietario"). */
    memset(&game->generatedCharacter, 0, sizeof(game->generatedCharacter));
    snprintf(game->generatedCharacter.name, sizeof(game->generatedCharacter.name), "Screenshot Forgeling");
    snprintf(game->generatedCharacter.role, sizeof(game->generatedCharacter.role), "FORGED THIS RUN");
    snprintf(game->generatedCharacter.blurb, sizeof(game->generatedCharacter.blurb),
             "A fake generated character, only for a manual screenshot check.");
    game->generatedCharacter.baseDamage = 9.0f;
    game->generatedCharacter.baseFireDelay = 0.22f;
    game->generatedCharacter.baseShotSpeed = 520.0f;
    game->generatedCharacter.baseShotRadius = 5.0f;
    game->generatedCharacter.baseSpeed = 215.0f;
    game->generatedCharacter.baseMaxHp = 7;
    game->generatedCharacter.hpCap = 14;
    game->generatedCharacter.baseLuck = 0.8f;
    game->generatedCharacter.palette = (Color){ 204, 119, 51, 255 };
    game->generatedCharacterValid = true;

    /* M6a, requisito 4 della spec ("--floor-zero-screenshot-test: aggiornato
       per mostrare anche la sezione PERSONAGGI"): un secondo scatto, stesso
       pannello ma sull'altra sezione (su, wrap fra le due) -- una schedina
       resta selezionata (SCELTO, il preselezionato di default) mentre il
       focus e' sul quarto slot (il personaggio generato, appena iniettato
       sopra), cosi' lo screenshot mostra ANCHE il segnale di selezione
       distinto dal focus (requisito 3) e la carta generata fianco a fianco
       con la rosa curata. */
    { AppInput in = InputUp();    UpdateApp(game, &mode, &gen, &ui, &in); }   /* MONDI -> PERSONAGGI */
    for (int i = 0; i < CHARACTER_COUNT; i++) { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* focus sul quarto slot generato */
    RendererDrawApp(game, canvas, APP_FLOOR_ZERO, &ui, true, &status, "logs/worldsmelt-floorzero-characters-screen.png");
    bool charactersTextureValid = canvas.texture.id != 0;

    UnloadRenderTexture(canvas);
    return worldsTextureValid && charactersTextureValid;
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

/* DEC-137, SOLO manuale (--overlay-screenshot-test): uno scatto dell'HUD in
   overlay sulla game view a tutto schermo, per il giudizio di gusto del
   proprietario. Come GameFullscreenScreenshotTest gira mentre la finestra e' a
   dimensione del MONITOR (src/app/app.c), cosi' l'HUD si vede alla risoluzione
   vera; a differenza di quello scatta APP_GAMEPLAY (non il Piano 0) con una
   scena ricca -- personaggio scelto, oggetti con un tipo di colpo e sinergie,
   risorse, vita intaccata, mappa svelata, qualche nemico -- perche' ciascun
   cluster dell'HUD (cuori, risorse, build, mappa, log) mostri contenuto vero
   sopra il gioco. Il file esce in logs/worldsmelt-overlay-<W>x<H>.png: il nome
   porta la risoluzione, cosi' due scatti a risoluzioni diverse non si pestano. */
/* La scena "da vetrina" condivisa dagli screenshot di verifica: personaggio
   scelto, mappa svelata, tre oggetti in mano con sinergie, risorse e vita
   intaccate, qualche nemico, un pickup e un colpo in volo. Estratta da
   GameOverlayScreenshotTest (W8) perche' gli scatti delle SCHERMATE
   (GameArtScreensScreenshotTest) hanno bisogno esattamente della stessa scena:
   due copie sarebbero divergute al primo ritocco, e uno scatto di verifica che
   mostra una scena diversa dall'altro non e' confrontabile. */
static void SetupShowcaseScene(Game *game)
{
    /* Un personaggio scelto: il cluster vitali mostra nome/ruolo (rosa base,
       indice 0). GameResetRun ha gia' preparato piano 1 e statistiche base. */
    game->characterChosenIndex = 0;

    /* Mappa svelata: marca visitate tutte le stanze esistenti del piano, cosi'
       la minimappa in alto a destra mostra i colori e le lettere delle stanze
       speciali invece di una griglia quasi vuota (e' uno screenshot di gusto). */
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            if (game->rooms[y][x].exists) WorldRoomAtMutable(game, x, y)->visited = true;   /* DEC-170: 'visited' vive nella cella di STATO della stanza */

    /* Tre oggetti con un tipo di colpo firmato in mano (il primo) e due trait,
       come lo screenshot delle forme di colpo -- riempiono la lista di
       BuildScreen e danno alla barra del colpo un nome vero. */
    memset(game->player.items, 0, sizeof(game->player.items));
    Item *hand = &game->player.items[0];
    hand->active = true;
    snprintf(hand->name, sizeof(hand->name), "Guanto di Schegge");
    hand->slot = SLOT_HAND; hand->rarity = RARITY_RARE; hand->kind = ITEM_PASSIVE;
    hand->color = game->theme.accent;
    ShotTypeExample(&hand->shotType, 0);

    Item *eyes = &game->player.items[1];
    eyes->active = true;
    snprintf(eyes->name, sizeof(eyes->name), "Occhio Rapace");
    eyes->slot = SLOT_EYES; eyes->rarity = RARITY_UNCOMMON; eyes->kind = ITEM_PASSIVE;
    eyes->color = game->theme.accent2; eyes->traits = TRAIT_HOMING;

    Item *back = &game->player.items[2];
    back->active = true;
    snprintf(back->name, sizeof(back->name), "Punteruolo Lungo");
    back->slot = SLOT_BACK; back->rarity = RARITY_UNCOMMON; back->kind = ITEM_PASSIVE;
    back->color = game->theme.accent; back->traits = TRAIT_PIERCE;

    game->player.itemCount = 3;
    ScriptItemsRecomputeStats(game);

    /* Risorse e vita intaccata: i contatori in alto a sinistra mostrano numeri
       veri e i cuori mostrano un mix pieno/mezzo/vuoto (ordine di consumo a
       parte, e' resa). Sinergie forzate DOPO il ricalcolo, cosi' le pillole
       dorate del cluster build compaiono nello scatto (il ricalcolo azzera la
       maschera). */
    game->player.coins = 27;
    game->player.bombs = 3;
    game->player.keys = 2;
    if (game->player.maxHp > 3) game->player.hp = game->player.maxHp - 3;
    game->player.synergies = (1u << 0) | (1u << 2);

    /* Un paio di nemici e un pickup, cosi' il gioco sotto l'HUD non e' vuoto. */
    for (int i = 0; i < 3; i++)
    {
        EnemyTypeDef foe; memset(&foe, 0, sizeof(foe));
        foe.active = true; foe.form = ENEMY_FORM_BLOB; foe.move = ENEMY_MOVE_CHASE; foe.fire = ENEMY_FIRE_NONE;
        foe.hpMul = 1.0f; foe.speedMul = 1.0f; foe.sizeMul = 1.2f; foe.pellets = 1;
        snprintf(foe.name, sizeof(foe.name), "Sentinella %d", i + 1);
        EntitiesAddEnemyTyped(game, ENEMY_CHASER,
                              (Vector2){ ROOM_X + 220.0f + (float)i*200.0f, ROOM_Y + ROOM_H*0.4f }, &foe);
    }
    EntitiesAddPickup(game, PICKUP_COIN, (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.7f }, 3, 0);

    /* Un colpo vero in volo, per un po' di vita nella scena. */
    CombatFirePlayer(game, (Vector2){ 0.0f, -1.0f });
    for (int frame = 0; frame < 10; frame++) CombatUpdateShots(game, 1.0f/60.0f);

    /* Zittisci il messaggio transitorio di inizio stanza ("Scegli una porta"):
       vive dentro il canvas, al fondo, dove ora si appoggia il cluster build
       dell'HUD -- per lo scatto di gusto meglio pulito (in gioco vero il
       messaggio e' comunque effimero e sfuma da solo). */
    game->messageTimer = 0.0f;
}

bool GameOverlayScreenshotTest(Game *game)
{
    SetupShowcaseScene(game);

    char path[96];
    snprintf(path, sizeof(path), "logs/worldsmelt-overlay-%dx%d.png", GetScreenWidth(), GetScreenHeight());

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);   /* pixel-perfect come il canvas del gioco vero */
    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, path);
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);
    return textureValid;
}

/* W8: uno scatto per SCHERMATA, con gli asset artistici veri (tileset, HUD in
   pixel art, cornici 9-patch, font da 5 pixel). Serve al giudizio di gusto sul
   reskin -- nessun assert puo' dire "questa schermata e' bella" -- e alla
   verifica che nessuna delle schermate rivestite sia rimasta indietro: se una
   sola continuasse a disegnarsi col font vettoriale, si vedrebbe subito
   mettendo gli scatti in fila.
   Gli image-id vengono dal catalogo curato vero (le voci che la sessione
   artistica ha disegnato): senza, il nemico e gli oggetti dello scatto
   ricadrebbero sulle sagome geometriche e lo screenshot non mostrerebbe cio'
   che deve mostrare. Un checkout senza assets/art/ produce comunque gli scatti,
   col ripiego: e' la prova visiva del degrado. */
bool GameArtScreensScreenshotTest(Game *game)
{
    SetupShowcaseScene(game);

    /* Un nemico e un boss con un image-id disegnato davvero (assets/art/). */
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active) continue;
        snprintf(game->enemies[i].type.imageId, sizeof(game->enemies[i].type.imageId), "%s",
                 (i%2 == 0) ? "goblin-di-slag" : "ragno-di-cenere");
    }
    /* Gli oggetti in mano: l'inventario di BuildScreen deve mostrare gli sprite
       nuovi, non i rombi colorati (requisito 2 del task W8). */
    static const char *const SHOWCASE_ITEM_IDS[] = { "dice-core", "lens-goggles", "root-tendril" };
    for (int i = 0; i < 3 && i < MAX_ITEMS; i++)
    {
        if (!game->player.items[i].active) continue;
        snprintf(game->player.items[i].imageId, sizeof(game->player.items[i].imageId), "%s",
                 SHOWCASE_ITEM_IDS[i]);
    }
    /* Un pickup di oggetto a terra, con sprite: e' il caso "piedistallo". */
    EntitiesAddPickup(game, PICKUP_ITEM, (Vector2){ ROOM_X + ROOM_W*0.3f, ROOM_Y + ROOM_H*0.75f }, 0, 0);
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (!game->pickups[i].active || game->pickups[i].kind != PICKUP_ITEM) continue;
        game->pickups[i].item = game->player.items[0];
        snprintf(game->pickups[i].item.imageId, sizeof(game->pickups[i].item.imageId), "%s", "crystal-shard");
        break;
    }
    /* Una card di scoperta in mostra, con il suo sprite: e' l'unico modo di
       vederla in uno scatto (in gioco dura ~3 s). */
    GameQueueDiscoveryCardWithImage(game, "Goblin di Slag",
                                    "Ruba lingotti quando ti colpisce.", "goblin-di-slag");
    game->discoveryActive = game->discoveryQueue[0];
    game->discoveryActiveValid = true;
    game->discoveryActiveTimer = 3.0f;
    game->player.flux = 2;   /* il Flux compare in HUD solo se se ne ha: qui deve comparire, col riquadro di evidenza */

    /* Le sette schermate del reskin. FloorZero ha bisogno delle carte-tema
       (altrimenti il pannello e' vuoto) e Gameplay dello stesso scatto di
       GameOverlayScreenshotTest ma coi nuovi asset: si tengono entrambi, sono
       due verifiche diverse (l'HUD in pixel art e il suo ripiego). */
    AppUi ui;
    memset(&ui, 0, sizeof(ui));
    ui.seed = 4242u;
    ui.focus = 1;   /* una voce col fuoco che NON e' la prima: la cornice accesa si vede meglio */

    struct { AppMode mode; const char *name; } shots[] = {
        { APP_GAMEPLAY, "gameplay" },
        { APP_MAIN_MENU, "mainmenu" },
        { APP_RUN_SETUP, "runsetup" },
        { APP_OPTIONS, "options" },
        { APP_PAUSE_MENU, "pausemenu" },
        { APP_BUILD_SCREEN, "buildscreen" },
        { APP_RUN_RESULTS, "runresults" },
        { APP_EXIT_CONFIRM, "exitconfirm" },
        { APP_FLOOR_ZERO, "floorzero" },
    };
    const int shotCount = (int)(sizeof(shots)/sizeof(shots[0]));

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    bool textureValid = canvas.texture.id != 0;
    for (int i = 0; i < shotCount && textureValid; i++)
    {
        char path[128];
        snprintf(path, sizeof(path), "logs/worldsmelt-w8-%s.png", shots[i].name);
        /* Il Piano 0 disegna l'indicatore di generazione da 'genProgress' e non
           da 'ui': gli si passa uno stato inattivo, che e' il caso normale di
           una demo curata senza generazione. */
        GenProgress progress;
        memset(&progress, 0, sizeof(progress));
        int savedFloor = game->floor;
        if (shots[i].mode == APP_FLOOR_ZERO)
        {
            /* Il Piano 0 e' il piano 0 per definizione: senza questo, il tema e
               la stanza sarebbero quelli del piano 1 e lo scatto mostrerebbe
               una sala d'attesa vestita da stanza di combattimento. */
            game->floor = 0;
        }
        RendererDrawApp(game, canvas, shots[i].mode, &ui, true, &progress, path);
        game->floor = savedFloor;
    }
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

    /* --- scenario 7 (M6a, spec requisito 4a-d): il selettore di personaggio
       nel pannello combinato. Gen disabilitata, stesso schema sintetico
       dello scenario 4: le carte-mondo curate compaiono SUBITO, quindi il
       pannello e' apribile dal primo frame -- niente attesa di un finto
       generatore, solo la scelta del personaggio conta qui. --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = false;
    mode = APP_MAIN_MENU;

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (7) Avvia non porta a FloorZero\n"); return false; }

    /* (a) il personaggio 0 (Wayfinder) e' preselezionato all'ingresso, e le
       sue base* sono gia' applicate al player dell'hub (il giocatore SENTE
       la velocita' dal primo frame, requisito 1/2 della spec). */
    if (game->characterChosenIndex != 0)
    {
        fprintf(stderr, "GameFloorZeroTest: (7a) il personaggio 0 non e' preselezionato (e' %d)\n", game->characterChosenIndex);
        return false;
    }
    if (game->player.speed != 240.0f || game->player.hpCap != 12 || game->player.luck != 0.5f)
    {
        fprintf(stderr, "GameFloorZeroTest: (7a) le stats del preselezionato (Wayfinder) non sono applicate nell'hub (speed=%.1f hpCap=%d luck=%.2f)\n",
                game->player.speed, game->player.hpCap, game->player.luck);
        return false;
    }

    /* (b) selezionarne un altro nella sezione PERSONAGGI cambia SUBITO le
       base* nell'hub (Ashblade, indice 1: speed 230, hpCap 8). */
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* apre il pannello, sezione MONDI di default */
    if (!game->themeCardsPanelOpen) { fprintf(stderr, "GameFloorZeroTest: (7b) TAB non apre il pannello combinato\n"); return false; }
    { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* MONDI -> PERSONAGGI (wrap fra le due sezioni) */
    if (game->floorZeroPanelSection != FLOOR_ZERO_PANEL_CHARACTERS)
    {
        fprintf(stderr, "GameFloorZeroTest: (7b) su non sposta il focus sulla sezione PERSONAGGI\n");
        return false;
    }
    { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* focus personaggio 0 -> 1 */
    if (game->characterCardFocus != 1) { fprintf(stderr, "GameFloorZeroTest: (7b) destra non sposta il focus sul personaggio 1\n"); return false; }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* sceglie Ashblade */
    if (game->characterChosenIndex != 1) { fprintf(stderr, "GameFloorZeroTest: (7b) confirm non sceglie il personaggio col focus\n"); return false; }
    if (game->player.speed != 230.0f || game->player.hpCap != 8)
    {
        fprintf(stderr, "GameFloorZeroTest: (7b) la selezione non ha cambiato SUBITO le stats nell'hub (speed=%.1f hpCap=%d)\n",
                game->player.speed, game->player.hpCap);
        return false;
    }
    if (!game->themeCardsPanelOpen)
    {
        fprintf(stderr, "GameFloorZeroTest: (7b) confermare un personaggio chiude il pannello (deve restare aperto, a differenza del mondo)\n");
        return false;
    }

    /* (d) la sezione PERSONAGGI naviga con wrap (focus 1 -> 0 -> ultimo -> 0)
       e la selezione e' idempotente (riconfermare lo stesso indice non
       cambia nulla). */
    { AppInput in = InputLeft(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterCardFocus != 0) { fprintf(stderr, "GameFloorZeroTest: (7d) sinistra da 1 non porta il focus a 0\n"); return false; }
    { AppInput in = InputLeft(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterCardFocus != CHARACTER_COUNT - 1)
    {
        fprintf(stderr, "GameFloorZeroTest: (7d) sinistra da 0 non fa il wrap sull'ultimo personaggio\n");
        return false;
    }
    { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterCardFocus != 0) { fprintf(stderr, "GameFloorZeroTest: (7d) destra dall'ultimo non fa il wrap su 0\n"); return false; }
    { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* torna sul focus 1 (Ashblade, gia' scelto) */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    float speedAfterReconfirm = game->player.speed;
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* riconferma: idempotente */
    if (game->characterChosenIndex != 1 || game->player.speed != speedAfterReconfirm)
    {
        fprintf(stderr, "GameFloorZeroTest: (7d) riconfermare lo stesso personaggio non e' idempotente\n");
        return false;
    }

    /* (c) attraversamento: la run parte con le stats E l'hpCap del
       personaggio scelto -- serve prima scegliere anche il mondo (l'uscita
       non si apre altrimenti, gating invariato dalla spec M5). */
    { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* PERSONAGGI -> MONDI */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* sceglie il mondo col focus (0) */
    if (game->themeChosenIndex != 0) { fprintf(stderr, "GameFloorZeroTest: (7c) la scelta del mondo e' fallita\n"); return false; }
    if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (7c) l'uscita non si apre subito con gen disabilitata\n"); return false; }

    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (7c) l'attraversamento non porta a Gameplay\n"); return false; }
    if (game->characterChosenIndex != 1) { fprintf(stderr, "GameFloorZeroTest: (7c) la run non ricorda il personaggio scelto nell'hub\n"); return false; }
    if (game->player.speed != 230.0f || game->player.hpCap != 8 || game->player.maxHp != 4)
    {
        fprintf(stderr, "GameFloorZeroTest: (7c) la run non parte con le stats/hpCap del personaggio scelto (speed=%.1f hpCap=%d maxHp=%d)\n",
                game->player.speed, game->player.hpCap, game->player.maxHp);
        return false;
    }

    /* FIX B: reset rapido R mantiene il personaggio scelto. */
    float speedBeforeReset = game->player.speed;
    int chosenCharacterBeforeReset = game->characterChosenIndex;
    int hpCapBeforeReset = game->player.hpCap;
    int maxHpBeforeReset = game->player.maxHp;

    game->resetQueued = true;
    GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);

    if (game->characterChosenIndex != chosenCharacterBeforeReset)
    {
        fprintf(stderr, "GameFloorZeroTest: (7e) reset rapido ha perso il personaggio scelto (era %d, ora %d)\n",
                chosenCharacterBeforeReset, game->characterChosenIndex);
        return false;
    }
    if (game->player.speed != speedBeforeReset || game->player.hpCap != hpCapBeforeReset || game->player.maxHp != maxHpBeforeReset)
    {
        fprintf(stderr, "GameFloorZeroTest: (7e) reset rapido ha modificato le stats/hpCap del personaggio (speed %.1f->%.1f hpCap %d->%d maxHp %d->%d)\n",
                speedBeforeReset, game->player.speed, hpCapBeforeReset, game->player.hpCap, maxHpBeforeReset, game->player.maxHp);
        return false;
    }

    /* --- scenario 8 (M6b-1, DEC-014 prima fetta -- spec floor-zero-test
       (a)+(c); esteso da M6b-2, DEC-037, requisito 3, floor-zero-test (a)):
       il quarto slot dinamico del pannello PERSONAGGI, la carta del
       personaggio generato per QUESTA run, letta da
       generated/character_proposal.json (fake propose esteso, vedi
       tests/fake-gen.sh FAKE_GEN_CHARACTER_MODE/FAKE_GEN_CHARACTER_LUA_MODE).
       Le sue stats applicate sono quelle DEL FILE (in banda qui: il clamp
       fuori banda e' lo scenario 10 sotto), e sopravvivono
       all'attraversamento -- GameResetRun le azzererebbe se la def generata
       non venisse ricatturata a parte, vedi il commento sul case
       APP_FLOOR_ZERO in src/app/app.c. Il fake propose (modalita' default
       "ok") scrive ANCHE un trait valido (on_evaluate: stats.max_hp += 1,
       un effetto OSSERVABILE e noto): maxHp atteso e' quindi 7 (dal file) +
       1 (dal trait) = 8, non 7 -- e' esattamente l'asserzione che prova che
       il trait e' ATTIVO dopo l'attraversamento (spec floor-zero-test,
       scenario a). --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_CHARACTER_MODE", "ok", 1);
    setenv("FAKE_GEN_MODE", "ok", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (8) Avvia non porta a FloorZero\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (8) il finto propose non e' mai terminato\n");
        return false;
    }
    if (!game->generatedCharacterValid)
    {
        fprintf(stderr, "GameFloorZeroTest: (8) la carta del personaggio generato non e' arrivata\n");
        return false;
    }
    if (strcmp(game->generatedCharacter.name, "Fake Ember Twin") != 0)
    {
        fprintf(stderr, "GameFloorZeroTest: (8) nome del personaggio generato inatteso ('%s')\n", game->generatedCharacter.name);
        return false;
    }

    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* apre il pannello, sezione MONDI di default */
    { AppInput in = InputUp();  UpdateApp(game, &mode, &gen, &ui, &in); }   /* MONDI -> PERSONAGGI */
    for (int i = 0; i < CHARACTER_COUNT; i++) { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterCardFocus != CHARACTER_COUNT)
    {
        fprintf(stderr, "GameFloorZeroTest: (8) il focus non raggiunge il quarto slot generato (e' %d)\n", game->characterCardFocus);
        return false;
    }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterChosenIndex != CHARACTER_COUNT)
    {
        fprintf(stderr, "GameFloorZeroTest: (8a) confirm non sceglie il personaggio generato\n");
        return false;
    }
    if (game->player.damage != 9.0f || game->player.speed != 215.0f || game->player.hpCap != 14 || game->player.maxHp != 8)
    {
        fprintf(stderr, "GameFloorZeroTest: (8a) le stats applicate non sono quelle del file + il trait (damage=%.1f speed=%.1f hpCap=%d maxHp=%d, atteso maxHp=8=7+1)\n",
                game->player.damage, game->player.speed, game->player.hpCap, game->player.maxHp);
        return false;
    }

    { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* PERSONAGGI -> MONDI */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* sceglie il mondo col focus (0), avvia la generazione completa (fake) */
    if (game->themeChosenIndex != 0) { fprintf(stderr, "GameFloorZeroTest: (8) la scelta del mondo e' fallita\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.runner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (8) la generazione completa (fake) non e' mai terminata\n");
        return false;
    }
    if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (8) l'uscita non si apre dopo il successo del finto generatore\n"); return false; }

    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (8c) l'attraversamento non porta a Gameplay\n"); return false; }
    if (game->characterChosenIndex != CHARACTER_COUNT)
    {
        fprintf(stderr, "GameFloorZeroTest: (8c) la run non ricorda il personaggio generato scelto\n");
        return false;
    }
    if (game->player.damage != 9.0f || game->player.speed != 215.0f || game->player.hpCap != 14 || game->player.maxHp != 8)
    {
        fprintf(stderr, "GameFloorZeroTest: (8c) la run non parte con le stats/hpCap/trait del personaggio generato (damage=%.1f speed=%.1f hpCap=%d maxHp=%d, atteso maxHp=8=7+1)\n",
                game->player.damage, game->player.speed, game->player.hpCap, game->player.maxHp);
        return false;
    }
    if (!game->generatedCharacterValid || strcmp(game->generatedCharacter.name, "Fake Ember Twin") != 0)
    {
        fprintf(stderr, "GameFloorZeroTest: (8c) la def generata non sopravvive a GameResetRun (valid=%d nome='%s')\n",
                game->generatedCharacterValid, game->generatedCharacter.name);
        return false;
    }
    if (game->generatedCharacter.traitHook[0] == '\0' || strcmp(game->generatedCharacter.traitHook, "on_evaluate") != 0)
    {
        fprintf(stderr, "GameFloorZeroTest: (8c) traitHook inatteso ('%s', atteso 'on_evaluate')\n", game->generatedCharacter.traitHook);
        return false;
    }

    /* --- scenario 8d (M6b-2, requisito 3, floor-zero-test (c)): switch
       generato -> base -> generato, sandbox del trait scaricata/ricaricata
       senza leak. Un solo slot (Game.characterTrait), quindi "senza leak"
       qui vuol dire "lo stato torna esattamente quello atteso ad ogni
       passo" -- se ScriptCharacterShutdown non liberasse la sandbox
       precedente prima di ScriptCharacterLoad, valgrind lo vedrebbe, ma
       anche un semplice crash/hang sotto lo stress di tre cicli di
       selezione lo rivelerebbe (nessuno dei due qui sotto). --- */
    /* Siamo gia' in Gameplay (post-attraversamento): il pannello del Piano 0
       non e' piu' raggiungibile da qui, quindi lo switch si esercita
       direttamente con le stesse due chiamate che AppConfirmCharacterChoice
       fa davvero (GamePlayerResetBaseStatsFor + ScriptItemsInit), non una
       reimplementazione -- e' cio' che il pannello chiamerebbe comunque. */
    game->resetQueued = true;
    GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);   /* il reset rapido mantiene il personaggio generato scelto: maxHp resta 8 */
    if (game->player.maxHp != 8)
    {
        fprintf(stderr, "GameFloorZeroTest: (8d) il reset rapido non mantiene il trait del personaggio generato (maxHp=%d, atteso 8)\n", game->player.maxHp);
        return false;
    }
    for (int cycle = 0; cycle < 3; cycle++)
    {
        GamePlayerResetBaseStatsFor(&game->player, CharacterRosterGet(0));
        ScriptItemsInit(game, CharacterRosterGet(0));   /* -> base (Wayfinder): nessun trait, il trait generato va SCARICATO */
        if (game->player.maxHp != 6)
        {
            fprintf(stderr, "GameFloorZeroTest: (8d) switch verso il base non scarica il trait (ciclo %d, maxHp=%d, atteso 6)\n", cycle, game->player.maxHp);
            return false;
        }
        const CharacterDef *generated = GameResolveCharacterDef(game, CHARACTER_COUNT);
        if (!generated) { fprintf(stderr, "GameFloorZeroTest: (8d) la carta generata e' sparita a meta' del ciclo %d\n", cycle); return false; }
        GamePlayerResetBaseStatsFor(&game->player, generated);
        ScriptItemsInit(game, generated);   /* -> generato: il trait va RICARICATO */
        if (game->player.maxHp != 8)
        {
            fprintf(stderr, "GameFloorZeroTest: (8d) switch di ritorno al generato non ricarica il trait (ciclo %d, maxHp=%d, atteso 8)\n", cycle, game->player.maxHp);
            return false;
        }
    }

    /* --- scenario 9 (spec floor-zero-test (b)): file assente -- solo le tre
       carte base, nessun crash, il focus non raggiunge mai un quarto slot
       inesistente. --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    remove("generated/character_proposal.json");   /* nessun residuo dello scenario 8 */
    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_CHARACTER_MODE", "none", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (9) Avvia non porta a FloorZero\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (9) il finto propose non e' mai terminato\n");
        return false;
    }
    if (game->generatedCharacterValid)
    {
        fprintf(stderr, "GameFloorZeroTest: (9) una carta generata e' comparsa senza file\n");
        return false;
    }
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputUp();  UpdateApp(game, &mode, &gen, &ui, &in); }
    for (int i = 0; i < CHARACTER_COUNT + 2; i++) { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (game->characterCardFocus < 0 || game->characterCardFocus >= CHARACTER_COUNT)
    {
        fprintf(stderr, "GameFloorZeroTest: (9) il focus ha raggiunto uno slot fuori dalla rosa base senza carta generata (e' %d)\n",
                game->characterCardFocus);
        return false;
    }

    /* --- scenario 10 (spec floor-zero-test (d)): proposta fuori banda
       (damage 99, maxHp 40...) -- clampata ALLA LETTURA (seconda rete di
       sicurezza, RunContentLoadCharacterProposal), mai propagata al player
       cosi' com'e'. hpCap non supera MAI 18 (banda) ne' 24 (guardia
       assoluta di motore, SCRIPT_ITEMS_MAX_HP_ABSOLUTE_MAX). --- */
    memset(&ui, 0, sizeof(ui));
    memset(&gen, 0, sizeof(gen));
    gen.enabled = true;
    gen.noSprites = true;
    gen.command = "tests/fake-gen.sh";
    mode = APP_MAIN_MENU;

    setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
    setenv("FAKE_GEN_CHARACTER_MODE", "outofband", 1);
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (10) Avvia non porta a FloorZero\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (10) il finto propose non e' mai terminato\n");
        return false;
    }
    if (!game->generatedCharacterValid)
    {
        fprintf(stderr, "GameFloorZeroTest: (10) la carta fuori banda non e' arrivata\n");
        return false;
    }
    if (game->generatedCharacter.baseDamage > CHARACTER_DAMAGE_MAX + 0.001f ||
        game->generatedCharacter.baseSpeed < CHARACTER_SPEED_MIN - 0.001f ||
        game->generatedCharacter.baseMaxHp > CHARACTER_MAX_HP_MAX ||
        game->generatedCharacter.hpCap > CHARACTER_HP_CAP_MAX ||
        game->generatedCharacter.hpCap > 24)
    {
        fprintf(stderr, "GameFloorZeroTest: (10) la carta fuori banda non e' stata clampata (damage=%.1f speed=%.1f maxHp=%d hpCap=%d)\n",
                game->generatedCharacter.baseDamage, game->generatedCharacter.baseSpeed,
                game->generatedCharacter.baseMaxHp, game->generatedCharacter.hpCap);
        return false;
    }
    if (gen.proposeRunner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.proposeRunner);

    /* --- scenario 11 (M6b-2, DEC-037, requisito 3, floor-zero-test (b)):
       "proposta senza trait (file lua assente ma json presente, caso
       anomalo)" -- il json dice "lua":true ma generated/scripts/
       character_trait.lua NON esiste (FAKE_GEN_CHARACTER_LUA_MODE=missing)
       o esiste ma non compila (=broken): in ENTRAMBI i casi la carta resta
       presente e selezionabile (il fallimento e' del CARICAMENTO a run
       gia' iniziata, non della proposta), il trait resta silenziosamente
       INATTIVO (nessun +1 a maxHp: resta 7, il valore del file), e non c'e'
       alcun crash. --- */
    for (int lm = 0; lm < 2; lm++)
    {
        const char *luaMode = (lm == 0) ? "missing" : "broken";

        memset(&ui, 0, sizeof(ui));
        memset(&gen, 0, sizeof(gen));
        gen.enabled = true;
        gen.noSprites = true;
        gen.command = "tests/fake-gen.sh";
        mode = APP_MAIN_MENU;

        remove("generated/character_proposal.json");
        remove("generated/scripts/character_trait.lua");
        setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
        setenv("FAKE_GEN_CHARACTER_MODE", "ok", 1);
        setenv("FAKE_GEN_CHARACTER_LUA_MODE", luaMode, 1);
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (11-%s) Avvia non porta a FloorZero\n", luaMode); return false; }
        if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) il finto propose non e' mai terminato\n", luaMode);
            return false;
        }
        if (!game->generatedCharacterValid)
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) la carta e' sparita per un trait anomalo (deve restare presente)\n", luaMode);
            return false;
        }

        { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputUp();  UpdateApp(game, &mode, &gen, &ui, &in); }
        for (int i = 0; i < CHARACTER_COUNT; i++) { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (game->characterChosenIndex != CHARACTER_COUNT)
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) confirm non sceglie il personaggio generato\n", luaMode);
            return false;
        }
        if (game->player.maxHp != 7)
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) maxHp=%d, atteso 7 (trait inattivo, nessun +1)\n", luaMode, game->player.maxHp);
            return false;
        }

        { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.runner.state, 5.0))
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) la generazione completa (fake) non e' mai terminata\n", luaMode);
            return false;
        }
        if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (11-%s) l'uscita non si apre\n", luaMode); return false; }

        game->floorZeroExitCrossed = true;
        { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (11-%s) l'attraversamento non porta a Gameplay\n", luaMode); return false; }
        if (game->player.maxHp != 7)
        {
            fprintf(stderr, "GameFloorZeroTest: (11-%s) dopo l'attraversamento maxHp=%d, atteso 7 (trait ancora inattivo, mai un crash)\n", luaMode, game->player.maxHp);
            return false;
        }
    }

    /* --- scenario 12 (M6b-3, DEC-068, requisito 6, spec floor-zero-test
       (a)+(b)+(c)): il colpo firmato OPZIONALE del personaggio generato,
       nei suoi tre stati (tests/fake-gen.sh, FAKE_GEN_CHARACTER_SHOT_MODE
       none/ok/outofband -- indipendente da FAKE_GEN_CHARACTER_LUA_MODE, qui
       sempre "ok" cosi' il trait (+1 a maxHp) resta un contributo NOTO in
       ogni ramo). Le stesse stats "ok" di fake-gen.sh (damage=9,
       fireDelay=0.22, maxHp=7, luck=0.8) attraversano la compressione cauta
       (M6b-3, requisito 2) SOLO quando c'e' un colpo firmato: senza, restano
       quelle del file (scenario 8, gia' verificato sopra); con, damage resta
       9.0 (proprio il bordo cauto 6.0+0.6*5.0), fireDelay sale a 0.226
       (0.28-0.6*0.09, il file dice 0.22 < banda cauta) e maxHp scende a 6
       (int(3+0.6*6)=int(6.6), il file dice 7 > banda cauta) -- hpCap deriva
       da QUESTO maxHp gia' compresso (12, non 14). --- */
    for (int sm = 0; sm < 3; sm++)
    {
        const char *shotMode = (sm == 0) ? "none" : (sm == 1) ? "ok" : "outofband";
        bool hasShot = (sm != 0);
        float expectFireDelay = hasShot ? 0.226f : 0.22f;
        int expectMaxHp = hasShot ? 6 : 7;          /* dal file, PRIMA del trait */
        int expectHpCap = hasShot ? 12 : 14;
        int expectFinalMaxHp = expectMaxHp + 1;      /* trait "ok": sempre +1 */

        memset(&ui, 0, sizeof(ui));
        memset(&gen, 0, sizeof(gen));
        gen.enabled = true;
        gen.noSprites = true;
        gen.command = "tests/fake-gen.sh";
        mode = APP_MAIN_MENU;

        remove("generated/character_proposal.json");
        remove("generated/scripts/character_trait.lua");
        setenv("FAKE_GEN_PROPOSE_MODE", "ok", 1);
        setenv("FAKE_GEN_CHARACTER_MODE", "ok", 1);
        setenv("FAKE_GEN_CHARACTER_LUA_MODE", "ok", 1);
        setenv("FAKE_GEN_CHARACTER_SHOT_MODE", shotMode, 1);
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputDown();    UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (12-%s) Avvia non porta a FloorZero\n", shotMode); return false; }
        if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) il finto propose non e' mai terminato\n", shotMode);
            return false;
        }
        if (!game->generatedCharacterValid)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) la carta del personaggio generato non e' arrivata\n", shotMode);
            return false;
        }
        if (game->generatedCharacter.signatureShot.active != hasShot)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) signatureShot.active=%d, atteso %d\n",
                    shotMode, game->generatedCharacter.signatureShot.active, hasShot);
            return false;
        }
        if (fabsf(game->generatedCharacter.baseFireDelay - expectFireDelay) > 0.001f ||
            game->generatedCharacter.baseMaxHp != expectMaxHp ||
            game->generatedCharacter.hpCap != expectHpCap)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) budget cauto non applicato come atteso (fireDelay=%.3f atteso %.3f, maxHp=%d atteso %d, hpCap=%d atteso %d)\n",
                    shotMode, game->generatedCharacter.baseFireDelay, expectFireDelay,
                    game->generatedCharacter.baseMaxHp, expectMaxHp, game->generatedCharacter.hpCap, expectHpCap);
            return false;
        }
        if (hasShot)
        {
            /* Scenario (c): "outofband" scrive form="bogus" e ogni manopola
               a 9 (fuori da ogni banda di shot_type.h) -- ShotTypeClamp/
               ShotTypeBalance (RunContentLoadCharacterProposal ->
               CharacterGenDefClamp, seconda rete lato gioco) devono averlo
               gia' riportato in banda qui, PRIMA ancora della selezione. */
            if (game->generatedCharacter.signatureShot.form < 0 ||
                game->generatedCharacter.signatureShot.form >= SHOT_FORM_COUNT)
            {
                fprintf(stderr, "GameFloorZeroTest: (12-%s) forma del colpo firmato fuori enum dopo il clamp\n", shotMode);
                return false;
            }
            float power = ShotTypePower(&game->generatedCharacter.signatureShot);
            if (power < SHOT_TYPE_POWER_MIN - 0.001f || power > SHOT_TYPE_POWER_MAX + 0.001f)
            {
                fprintf(stderr, "GameFloorZeroTest: (12-%s) potenza del colpo firmato fuori banda dopo il clamp (%.3f)\n", shotMode, power);
                return false;
            }
        }

        { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputUp();  UpdateApp(game, &mode, &gen, &ui, &in); }
        for (int i = 0; i < CHARACTER_COUNT; i++) { AppInput in = InputRight(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (game->characterChosenIndex != CHARACTER_COUNT)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) confirm non sceglie il personaggio generato\n", shotMode);
            return false;
        }
        /* Requisito 6, scenario (a)/(b): il colpo ATTIVO fin da subito nel
           pannello del giocatore (prima ancora di attraversare) e' gia' il
           firmato quando c'e', lo standard quando non c'e' -- il ricalcolo
           (ScriptItemsInit->ScriptItemsRecomputeStats) riparte SEMPRE da
           Player.characterShotType, gia' applicato dalla selezione. */
        if (game->player.shotType.active != hasShot)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) colpo attivo nell'hub: active=%d, atteso %d\n",
                    shotMode, game->player.shotType.active, hasShot);
            return false;
        }
        if (hasShot)
        {
            const char *expectName = (sm == 1) ? "Fake Ember Fang" : "Fake Overshot";
            if (strcmp(game->player.shotType.name, expectName) != 0)
            {
                fprintf(stderr, "GameFloorZeroTest: (12-%s) nome del colpo attivo inatteso ('%s', atteso '%s')\n",
                        shotMode, game->player.shotType.name, expectName);
                return false;
            }
        }

        { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.runner.state, 5.0))
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) la generazione completa (fake) non e' mai terminata\n", shotMode);
            return false;
        }
        if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (12-%s) l'uscita non si apre\n", shotMode); return false; }

        game->floorZeroExitCrossed = true;
        { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (12-%s) l'attraversamento non porta a Gameplay\n", shotMode); return false; }
        /* Requisito 6, scenario (a): DOPO l'attraversamento (GameResetRun +
           riapplicazione del personaggio scelto), il tipo di colpo attivo
           resta il firmato -- sopravvive esattamente come stats/trait
           (scenario 8c sopra), stesso pattern, stesso motivo (la def
           generata viene ricatturata PRIMA del memset di GameResetRun). */
        if (game->player.shotType.active != hasShot)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) colpo attivo dopo l'attraversamento: active=%d, atteso %d\n",
                    shotMode, game->player.shotType.active, hasShot);
            return false;
        }
        if (game->player.maxHp != expectFinalMaxHp)
        {
            fprintf(stderr, "GameFloorZeroTest: (12-%s) maxHp dopo l'attraversamento=%d, atteso %d (=%d dal file/budget cauto + 1 dal trait)\n",
                    shotMode, game->player.maxHp, expectFinalMaxHp, expectMaxHp);
            return false;
        }
    }
    unsetenv("FAKE_GEN_CHARACTER_SHOT_MODE");

    /* --- scenario 13 (WP21, DEC-114, must_fix 1 della bocciatura): il cuore
       del gap che questo lavoro doveva chiudere -- R IN GAMEPLAY, con la
       generazione ABILITATA (gen.command="tests/fake-gen.sh", non gen
       azzerata come in GameStatesTest), non deve piu' rigenerare nulla. Era
       esattamente il difetto che DEC-114 dichiarava ("oggi R rigenera
       direttamente, da adeguare"): prima di questo WP, con gen.enabled=true,
       il case APP_GAMEPLAY chiamava AppEnterFloorZero(game, gen, mode,
       NextGenSeed(0u)) su R, un vero reroll senza conferma. Si arriva a
       Gameplay con una run vera generata (fake-gen "ok", stesso schema dello
       scenario 3), poi si preme R: deve restare il reset rapido STESSO seed
       (resetQueued=true, game->runSeed e gen.pendingGenSeed invariati,
       nessun proposeRunner avviato) -- il reroll a seed nuovo vive SOLO
       nella voce "Rigenera la run" di PauseMenu (verificato sopra da
       GameStatesTest, ma li' con gen.enabled=false: nessuno dei due test da
       solo basterebbe).
       Criterio di accettazione (dichiarato nel verdetto): rimettendo a
       src/app/app.c il vecchio ramo "if (gen->enabled) AppEnterFloorZero(
       game, gen, mode, NextGenSeed(0u)); else" dentro il case APP_GAMEPLAY,
       questo scenario DEVE fallire sulla riga "mode != APP_GAMEPLAY" o su
       "game->runSeed != seedBeforeR" qui sotto. */
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
    if (mode != APP_FLOOR_ZERO) { fprintf(stderr, "GameFloorZeroTest: (13) Avvia non porta a FloorZero\n"); return false; }
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.proposeRunner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (13) il finto propose non e' mai terminato\n");
        return false;
    }
    ChooseFirstThemeCard(game, &mode, &gen, &ui);
    if (!FloorZeroRunnerSettle(&gen, &mode, &ui, game, &gen.runner.state, 5.0))
    {
        fprintf(stderr, "GameFloorZeroTest: (13) la generazione completa (fake) non e' mai terminata\n");
        return false;
    }
    if (!game->floorZeroExitOpen) { fprintf(stderr, "GameFloorZeroTest: (13) l'uscita non si apre dopo il successo del finto generatore\n"); return false; }

    game->floorZeroExitCrossed = true;
    { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_GAMEPLAY) { fprintf(stderr, "GameFloorZeroTest: (13) l'attraversamento non porta a Gameplay\n"); return false; }

    unsigned int seedBeforeR = game->runSeed;
    unsigned int pendingSeedBeforeR = gen.pendingGenSeed;
    { AppInput in = InputReroll(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_GAMEPLAY)
    {
        fprintf(stderr, "GameFloorZeroTest: (13) R in Gameplay con gen abilitata ha cambiato stato applicativo (mode=%d)\n", (int)mode);
        return false;
    }
    if (!game->resetQueued)
    {
        fprintf(stderr, "GameFloorZeroTest: (13) R in Gameplay con gen abilitata non ha messo in coda il reset rapido\n");
        return false;
    }
    if (game->runSeed != seedBeforeR)
    {
        fprintf(stderr, "GameFloorZeroTest: (13) R in Gameplay con gen abilitata ha cambiato game->runSeed (era %u, ora %u)\n", seedBeforeR, game->runSeed);
        return false;
    }
    if (gen.pendingGenSeed != pendingSeedBeforeR)
    {
        fprintf(stderr, "GameFloorZeroTest: (13) R in Gameplay con gen abilitata ha cambiato gen.pendingGenSeed (era %u, ora %u)\n", pendingSeedBeforeR, gen.pendingGenSeed);
        return false;
    }
    if (gen.proposeRunner.state == GEN_RUNNER_RUNNING)
    {
        fprintf(stderr, "GameFloorZeroTest: (13) R in Gameplay con gen abilitata ha avviato un nuovo proposeRunner (un reroll mascherato)\n");
        return false;
    }
    game->resetQueued = false;   /* consuma la coda: questo scenario prova solo il cablaggio dell'input, non l'effetto del reset */

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
   DEC-170 (taglie multiple stile Isaac + telecamera; supera il lattice in
   pixel di M2/DEC-009). Vedi il commento in game_tests.h e
   docs/design/systems/rooms-and-floor-generation.md ("Taglie multiple e
   telecamera" + "Default proposti"). Portabile (nessuna dipendenza da
   melting-gen/Xvfb): gira su entrambe le piattaforme, a differenza del
   blocco sopra.
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

/* Test (g): RoomLayoutBuild alla taglia MINIMA garantita (DEC-170: una
   cella), con ogni forma non-vuota alla densita' MASSIMA (il caso piu'
   affollato possibile): deve continuare a produrre almeno un ostacolo (niente
   collasso silenzioso dei quadranti sotto i 20px, vedi room_layout.c) e
   rispettare comunque croce/cerchio centrali. Stessa logica di verifica di
   TestRoomLayoutAlwaysPlayable (src/tests/script_items_tests.c). */
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
            int n = RoomLayoutBuild(&def, seed, rx, ry, rw, rh, obs, ROOM_LAYOUT_MAX_PER_CELL);
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
    printf("  [rooms-g] taglia minima (%dx%d = una cella), ogni forma/densita' massima: minimo blocchi visti %d -> giocabile=%s\n",
           WORLD_ROOM_MIN_W, WORLD_ROOM_MIN_H, minBlocksSeen, ok ? "si" : "NO");
    if (!ok) printf("      FALLITO: alla taglia minima un layout ha collassato (0 ostacoli), murato croce/cerchio, o e' uscito dalla stanza\n");
    return ok;
}

/* Test (h): la telecamera e' fatta di due funzioni PURE (world/room_camera.h),
   quindi il suo contratto si verifica senza finestra e senza Game:
     - il rettangolo di clamp di UNA cella e' esattamente la cella inquadrata
       (ROOM_FRAME_W x ROOM_FRAME_H), cioe' la stanza piu' la sua cornice di
       muro: la garanzia che non si perdano ne' muri ne' porte;
     - a qualunque posizione del giocatore, l'inquadratura non esce MAI dai
       bordi del rettangolo di clamp -- la garanzia esplicita di DEC-170;
     - la telecamera SEGUE il giocatore su ogni asse in cui la vista e' piu'
       piccola del rettangolo di clamp, e resta FERMA su ogni asse in cui e'
       piu' grande o uguale;
     - l'inseguimento converge e si aggancia (nessuna deriva sotto il pixel).
   DEC-200 riscrive la terza voce. Fino a WP-UI-0 diceva "la 1x1 e' sempre
   ferma su entrambi gli assi", perche' il canvas (960x640) era grande esatta-
   mente quanto la cella inquadrata e il clamp degenerava da solo. Col canvas a
   640x360 la vista e' piu' piccola della cella su entrambi gli assi, quindi
   anche una 1x1 scorre -- con lo stesso clamp e senza righe in piu'. La forma
   generale ("ferma se la vista copre l'asse, altrimenti segue") copre ENTRAMBE
   le epoche e continuerebbe a fallire se il clamp si rompesse davvero: e' cio'
   che si voleva verificare, la costante 1x1 era solo il caso particolare di
   allora. Se si rivuole la 1x1 a inquadratura fissa la leva e' ROOM_W/ROOM_H
   (una decisione di design, vedi governance/open-questions.md), e questo test
   la riconoscerebbe automaticamente. */
static bool RoomsTestCameraClamp(void)
{
    bool ok = true;
    struct { const char *name; float w; float h; } cases[] = {
        { "1x1", ROOM_W, ROOM_H },
        { "1x2", ROOM_W*2.0f, ROOM_H },
        { "2x1", ROOM_W, ROOM_H*2.0f },
        { "2x2", ROOM_W*2.0f, ROOM_H*2.0f },
    };
    const float viewW = (float)SCREEN_WIDTH, viewH = (float)SCREEN_HEIGHT;
    int checked = 0;

    for (int c = 0; c < (int)(sizeof(cases)/sizeof(cases[0])); c++)
    {
        Rectangle room = { ROOM_X, ROOM_Y, cases[c].w, cases[c].h };
        Rectangle bounds = WorldCameraBoundsFromRoom(room);
        /* Il rettangolo di clamp di UNA cella e' esattamente la cella
           INQUADRATA (ROOM_FRAME_*): la stanza piu' la cornice di muro e le
           due fasce. Era scritto "= il canvas" finche' le due cose erano lo
           stesso 960x640; DEC-200 le ha separate e questo e' il valore che
           conta davvero -- se sparisse, la 1x1 perderebbe i propri muri. */
        if (c == 0)
        {
            if (fabsf(bounds.x) > 0.01f || fabsf(bounds.y) > 0.01f ||
                fabsf(bounds.width - ROOM_FRAME_W) > 0.01f || fabsf(bounds.height - ROOM_FRAME_H) > 0.01f)
            {
                fprintf(stderr, "GameRoomsTest: (h) il rettangolo di clamp di una cella non e' la cella inquadrata (%.0fx%.0f): %.1f,%.1f %.1fx%.1f\n",
                        ROOM_FRAME_W, ROOM_FRAME_H, bounds.x, bounds.y, bounds.width, bounds.height);
                ok = false;
            }
        }
        Vector2 first = { 0.0f, 0.0f };
        bool firstSet = false;
        for (int iy = 0; iy <= 10; iy++)
        {
            for (int ix = 0; ix <= 10; ix++)
            {
                Vector2 focus = { room.x + room.width*(float)ix/10.0f, room.y + room.height*(float)iy/10.0f };
                Vector2 target = WorldCameraClampTarget(bounds, focus, viewW, viewH);
                Rectangle view = { target.x - viewW*0.5f, target.y - viewH*0.5f, viewW, viewH };
                checked++;
                if (view.x < bounds.x - 0.01f || view.y < bounds.y - 0.01f ||
                    view.x + view.width > bounds.x + bounds.width + 0.01f ||
                    view.y + view.height > bounds.y + bounds.height + 0.01f)
                {
                    fprintf(stderr, "GameRoomsTest: (h) taglia %s: l'inquadratura esce dai bordi (view %.1f,%.1f focus %.1f,%.1f)\n",
                            cases[c].name, view.x, view.y, focus.x, focus.y);
                    ok = false;
                }
                if (!firstSet) { first = target; firstSet = true; }
                /* Asse per asse: FERMA dove la vista copre gia' tutto il
                   rettangolo di clamp (non c'e' una seconda inquadratura da
                   scegliere), altrimenti SEGUE. Prima di DEC-200 la 1x1 era
                   ferma su entrambi gli assi -- ora e' solo il caso in cui i
                   due rami si decidono in modo diverso, non piu' una taglia
                   speciale. */
                bool freeX = bounds.width > viewW;
                bool freeY = bounds.height > viewH;
                if (!freeX && fabsf(target.x - first.x) > 0.01f)
                {
                    fprintf(stderr, "GameRoomsTest: (h) taglia %s: la telecamera si e' mossa in x con la vista piu' larga del limite (%.1f -> %.1f)\n",
                            cases[c].name, first.x, target.x);
                    ok = false;
                }
                if (!freeY && fabsf(target.y - first.y) > 0.01f)
                {
                    fprintf(stderr, "GameRoomsTest: (h) taglia %s: la telecamera si e' mossa in y con la vista piu' alta del limite (%.1f -> %.1f)\n",
                            cases[c].name, first.y, target.y);
                    ok = false;
                }
                {
                    Vector2 low = WorldCameraClampTarget(bounds, (Vector2){ room.x, room.y }, viewW, viewH);
                    Vector2 high = WorldCameraClampTarget(bounds, (Vector2){ room.x + room.width, room.y + room.height }, viewW, viewH);
                    if (freeX && !(high.x > low.x + 1.0f))
                    {
                        fprintf(stderr, "GameRoomsTest: (h) taglia %s: la telecamera non segue sull'asse x\n", cases[c].name);
                        ok = false;
                    }
                    if (freeY && !(high.y > low.y + 1.0f))
                    {
                        fprintf(stderr, "GameRoomsTest: (h) taglia %s: la telecamera non segue sull'asse y\n", cases[c].name);
                        ok = false;
                    }
                }
            }
        }
    }

    /* Inseguimento: converge verso il bersaglio e ci si aggancia esattamente
       (un canvas campionato POINT non deve tremolare per mezzo pixel). */
    Vector2 cur = { 0.0f, 0.0f };
    Vector2 want = { 300.0f, -120.0f };
    float prevDist = -1.0f;
    for (int step = 0; step < 240; step++)
    {
        cur = WorldCameraApproach(cur, want, 1.0f/60.0f, WORLD_CAMERA_RATE);
        float dist = fabsf(cur.x - want.x) + fabsf(cur.y - want.y);
        if (prevDist >= 0.0f && dist > prevDist + 0.001f)
        {
            fprintf(stderr, "GameRoomsTest: (h) l'inseguimento della telecamera non e' monotono (%.3f -> %.3f)\n", prevDist, dist);
            ok = false;
            break;
        }
        prevDist = dist;
    }
    if (cur.x != want.x || cur.y != want.y)
    {
        fprintf(stderr, "GameRoomsTest: (h) l'inseguimento non si aggancia al bersaglio (%.4f,%.4f invece di %.1f,%.1f)\n",
                cur.x, cur.y, want.x, want.y);
        ok = false;
    }

    printf("  [rooms-h] telecamera: %d inquadrature provate su 4 taglie, mai fuori dai bordi; ferma sugli assi coperti dalla vista e in inseguimento sugli altri; inseguimento monotono e agganciato -> %s\n",
           checked, ok ? "ok" : "FALLITO");
    return ok;
}

/* Test (k): l'angolo mancante di una forma a L e' MURO VERO, non un buco nel
   disegno. Si cerca fra i semi un piano con una stanza a L, ci si entra, si
   piazza il giocatore in mezzo all'angolo mancante e si fa girare un passo di
   simulazione vero (GameUpdate): deve venire respinto fuori, esattamente come
   da un ostacolo qualsiasi -- e' cosi' che DEC-170 e' implementata (la cella
   mancante entra in game->obstacles), quindi e' cosi' che va verificata. */
static bool RoomsTestHoleIsSolid(void)
{
    static const unsigned int kSeeds[] = { 1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        for (int floor = 1; floor <= FLOOR_COUNT; floor++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(&probe, x, y);
                    if (state != &probe.rooms[y][x]) continue;
                    if (WorldRoomSizeFromCells(state->cells) != ROOM_SIZE_L) continue;

                    probe.roomX = x;
                    probe.roomY = y;
                    WorldSpawnRoomContents(&probe);
                    if (WorldRoomHoleCount(&probe) != 1)
                    {
                        fprintf(stderr, "GameRoomsTest: (k) una forma a L deve avere esattamente una cella-buco (ne ha %d)\n",
                                WorldRoomHoleCount(&probe));
                        return false;
                    }
                    if (probe.obstacleHoleCount != 1 || probe.obstacleCount < 1)
                    {
                        fprintf(stderr, "GameRoomsTest: (k) la cella-buco non e' entrata fra gli ostacoli (hole=%d totale=%d)\n",
                                probe.obstacleHoleCount, probe.obstacleCount);
                        return false;
                    }
                    Rectangle hole = WorldRoomHoleRect(&probe, 0);
                    probe.player.pos = (Vector2){ hole.x + hole.width*0.5f, hole.y + hole.height*0.5f };
                    probe.phase = PHASE_PLAY;
                    GameUpdate(&probe, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
                    bool stillInside = probe.player.pos.x > hole.x && probe.player.pos.x < hole.x + hole.width &&
                                       probe.player.pos.y > hole.y && probe.player.pos.y < hole.y + hole.height;
                    if (stillInside)
                    {
                        fprintf(stderr, "GameRoomsTest: (k) il giocatore resta dentro l'angolo mancante della forma a L (%.1f,%.1f)\n",
                                probe.player.pos.x, probe.player.pos.y);
                        return false;
                    }
                    /* DEC-180: la telecamera in una L clampa al riquadro
                       dell'INTERA stanza (il blocco 2x2), esattamente come le
                       altre taglie maggiori -- l'angolo mancante puo' entrare
                       in inquadratura, e' compito del tileset (W8) vestirlo da
                       muro/sfondo, non piu' del clamp evitarlo. */
                    Rectangle focus = WorldCameraFocusRect(&probe);
                    if (fabsf(focus.width - ROOM_W*2.0f) > 0.5f || fabsf(focus.height - ROOM_H*2.0f) > 0.5f)
                    {
                        fprintf(stderr, "GameRoomsTest: (k) in una forma a L il clamp della telecamera non e' il riquadro 2x2 intero (%.0fx%.0f)\n",
                                focus.width, focus.height);
                        return false;
                    }
                    printf("  [rooms-k] forma a L (seed %u piano %d): angolo mancante solido (giocatore respinto), clamp telecamera = riquadro 2x2 intero (DEC-180) -> ok\n",
                           kSeeds[si], floor);
                    return true;
                }
            }
        }
    }
    fprintf(stderr, "GameRoomsTest: (k) nessuna forma a L nei semi di prova: verifica non eseguita\n");
    return false;
}

/* Test (o), WP4 (systems/special-rooms.md, "Stanza di fusione"): interazione
   col crogiolo. Cerca fra i semi un piano che abbia trovato posto per
   ROOM_FUSION, ci entra, e verifica l'intera catena: il crogiolo esiste come
   Pickup di kind PICKUP_FUSION_ALTAR; toccarlo (stessa geometria di overlap di
   ogni altro pickup, CombatUpdatePickups) scrive Game.fusionRoomTriggered
   SENZA consumare il pickup (resta 'active'); UpdateApp lo legge e apre
   APP_BUILD_SCREEN esattamente come farebbe TAB da Gameplay (la rete di
   sicurezza che RESTA, vedi il commento su ROOM_FUSION in core/game_types.h),
   consumando il segnale (torna falso); allontanandosi il blocco si scioglie,
   cosi' il crogiolo resta ritoccabile per tutta la permanenza nella stanza. */
static bool RoomsTestFusionInteraction(void)
{
    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u, 90210u, 5150u
    };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        for (int floor = 1; floor <= FLOOR_COUNT; floor++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(&probe, x, y);
                    if (state != &probe.rooms[y][x]) continue;
                    if (state->kind != ROOM_FUSION) continue;

                    probe.roomX = x;
                    probe.roomY = y;
                    WorldSpawnRoomContents(&probe);

                    Pickup *altar = NULL;
                    for (int i = 0; i < MAX_PICKUPS; i++)
                    {
                        if (probe.pickups[i].active && probe.pickups[i].kind == PICKUP_FUSION_ALTAR)
                        {
                            altar = &probe.pickups[i];
                            break;
                        }
                    }
                    if (!altar)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: nessun crogiolo nella stanza di fusione\n",
                                floor, kSeeds[si]);
                        return false;
                    }

                    probe.player.pos = altar->pos;
                    CombatUpdatePickups(&probe);
                    if (!probe.fusionRoomTriggered)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: toccare il crogiolo non scrive fusionRoomTriggered\n",
                                floor, kSeeds[si]);
                        return false;
                    }
                    if (!altar->active)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: il crogiolo si e' consumato al tocco (non deve mai succedere)\n",
                                floor, kSeeds[si]);
                        return false;
                    }

                    AppGen gen; memset(&gen, 0, sizeof(gen));
                    AppUi ui; memset(&ui, 0, sizeof(ui));
                    AppMode mode = APP_GAMEPLAY;
                    AppInput in; memset(&in, 0, sizeof(in));
                    UpdateApp(&probe, &mode, &gen, &ui, &in);
                    if (mode != APP_BUILD_SCREEN)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: il tocco del crogiolo non apre BuildScreen (mode=%d)\n",
                                floor, kSeeds[si], (int)mode);
                        return false;
                    }
                    if (probe.fusionRoomTriggered)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: UpdateApp non ha consumato fusionRoomTriggered\n",
                                floor, kSeeds[si]);
                        return false;
                    }

                    /* Allontanandosi il blocco si scioglie (CombatUpdatePickups,
                       stessa disciplina del piedistallo degli attivi, DEC-117):
                       il crogiolo resta ritoccabile per tutta la permanenza
                       nella stanza, non un varco a uso singolo. */
                    probe.player.pos = (Vector2){ altar->pos.x + 500.0f, altar->pos.y + 500.0f };
                    CombatUpdatePickups(&probe);
                    if (altar->locked)
                    {
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: il crogiolo resta bloccato dopo che il giocatore si e' allontanato\n",
                                floor, kSeeds[si]);
                        return false;
                    }

                    printf("  [rooms-o] crogiolo (seed %u piano %d): pickup persistente, tocco -> fusionRoomTriggered -> UpdateApp apre BuildScreen, si sblocca allontanandosi -> ok\n",
                           kSeeds[si], floor);
                    return true;
                }
            }
        }
    }
    fprintf(stderr, "GameRoomsTest: (o) nessuna stanza di fusione nei semi di prova: verifica non eseguita\n");
    return false;
}

/* WP5 (DEC-051, "stanza a tempo"): trova la prima stanza a tempo nei semi di
   prova e la esercita DUE volte -- una raggiunta "in tempo" (elapsed=0, che
   e' sempre <= soglia, la soglia e' sempre positiva) e una "oltre soglia"
   (elapsed = soglia + margine, calcolato con la STESSA funzione del motore,
   WorldTimedRoomThresholdSeconds, cosi' il test non duplica la formula).
   Stesso schema di RoomsTestFusionInteraction sopra: Game LOCALI, nessuna
   dipendenza da AppRun. */
static bool RoomsTestTimedRoomInteraction(void)
{
    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u, 90210u, 5150u
    };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        for (int floor = WORLD_TIMED_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT; floor++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);

            int tx = -1, ty = -1;
            for (int y = 0; y < GRID_SIZE && tx < 0; y++)
                for (int x = 0; x < GRID_SIZE && tx < 0; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(&probe, x, y);
                    if (state != &probe.rooms[y][x]) continue;
                    if (state->kind == ROOM_TIMED) { tx = x; ty = y; }
                }
            if (tx < 0) continue;   /* niente stanza a tempo su questo piano/seme: si prova il prossimo */

            /* Caso A: raggiunta SUBITO (elapsed = 0, sempre <= soglia). */
            probe.roomX = tx; probe.roomY = ty;
            int coinsBefore = probe.player.coins;
            WorldSpawnRoomContents(&probe);
            const RoomState *timedRoom = WorldRoomAt(&probe, tx, ty);
            if (!timedRoom->rewardTaken)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: raggiunta subito ma nessuna ricompensa registrata\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (probe.player.coins != coinsBefore + WORLD_ROOM_CURRENCY_TIMED)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: valuta attesa %d, ottenuta %d\n",
                        floor, kSeeds[si], coinsBefore + WORLD_ROOM_CURRENCY_TIMED, probe.player.coins);
                return false;
            }
            if (GameRoomIsLocked(&probe))
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: la stanza a tempo blocca le porte (mai ammesso)\n",
                        floor, kSeeds[si]);
                return false;
            }
            Pickup *marker = NULL;
            for (int i = 0; i < MAX_PICKUPS; i++)
            {
                if (probe.pickups[i].active && probe.pickups[i].kind == PICKUP_TIMED_MARKER) { marker = &probe.pickups[i]; break; }
            }
            if (!marker || marker->value != 1)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: nessuna clessidra 'in tempo' nella stanza\n",
                        floor, kSeeds[si]);
                return false;
            }
            /* Toccarla non deve fare NULLA: e' un segnale, non una raccolta
               (stessa disciplina del crogiolo -- RoomsTestFusionInteraction
               sopra -- ma senza alcun effetto, nemmeno un campo Game). */
            probe.player.pos = marker->pos;
            int coinsAtTouch = probe.player.coins;
            CombatUpdatePickups(&probe);
            if (!marker->active || probe.player.coins != coinsAtTouch)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: la clessidra si e' consumata o ha pagato al tocco (non deve mai succedere)\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* Caso B: raggiunta OLTRE la soglia -- si rigenera lo STESSO piano
               (stesso seed, deterministico) e si sposta l'orologio avanti
               prima del primo ingresso nella stanza. */
            Game probeLate;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probeLate);
            float threshold = WorldTimedRoomThresholdSeconds(&probeLate);
            probeLate.runElapsedSeconds = probeLate.floorEntryElapsedSeconds + threshold + 5.0f;
            probeLate.roomX = tx; probeLate.roomY = ty;
            int coinsBeforeLate = probeLate.player.coins;
            WorldSpawnRoomContents(&probeLate);
            const RoomState *timedRoomLate = WorldRoomAt(&probeLate, tx, ty);
            if (timedRoomLate->rewardTaken)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: oltre soglia ma la ricompensa risulta comunque assegnata\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (probeLate.player.coins != coinsBeforeLate)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: oltre soglia ma la valuta e' comunque cambiata (%d -> %d)\n",
                        floor, kSeeds[si], coinsBeforeLate, probeLate.player.coins);
                return false;
            }
            if (GameRoomIsLocked(&probeLate))
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: oltre soglia la stanza a tempo blocca le porte (mai ammesso, special-rooms.md)\n",
                        floor, kSeeds[si]);
                return false;
            }
            Pickup *markerLate = NULL;
            for (int i = 0; i < MAX_PICKUPS; i++)
            {
                if (probeLate.pickups[i].active && probeLate.pickups[i].kind == PICKUP_TIMED_MARKER) { markerLate = &probeLate.pickups[i]; break; }
            }
            if (!markerLate || markerLate->value != 0)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: nessuna clessidra 'scaduta' nella stanza oltre soglia\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* Caso C: soglia rispettata quando 'Game.floorEntryElapsedSeconds'
               NON e' zero -- una run che arriva a questo piano dopo avere gia'
               speso tempo nei piani precedenti (qui simulato: 500s). Nei casi
               A/B sopra 'RoomsTestGenerateFloor' riparte sempre da un Game
               azzerato (memset), quindi floorEntryElapsedSeconds == 0 e "delta
               dall'ingresso nel piano" coincide numericamente con "valore
               assoluto del timer di run": un'implementazione che misurasse per
               errore la soglia dal solo runElapsedSeconds (o dall'inizio della
               run, l'alternativa esplicitamente scartata dalla voce 33 di
               governance/open-questions.md) passerebbe comunque i casi A/B
               senza che nessuno se ne accorga. Qui runElapsedSeconds e' gia' a
               500s PRIMA di WorldStartFloor, cosi' le due basi si separano
               davvero: solo il delta (pochi secondi) resta entro soglia, il
               valore assoluto (500+) no. */
            Game probeShifted;
            memset(&probeShifted, 0, sizeof(probeShifted));
            probeShifted.rng = kSeeds[si];
            probeShifted.phase = PHASE_PLAY;
            probeShifted.runElapsedSeconds = 500.0f;
            WorldStartFloor(&probeShifted, floor);
            if (probeShifted.floorEntryElapsedSeconds != 500.0f)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: WorldStartFloor non ha catturato floorEntryElapsedSeconds dal runElapsedSeconds gia' accumulato (atteso 500.000, ottenuto %.3f)\n",
                        floor, kSeeds[si], (double)probeShifted.floorEntryElapsedSeconds);
                return false;
            }
            probeShifted.roomX = tx; probeShifted.roomY = ty;
            int coinsBeforeShifted = probeShifted.player.coins;
            probeShifted.runElapsedSeconds = 500.0f + 3.0f;   /* pochi secondi DOPO l'ingresso nel piano: entro qualunque soglia misurata (min ~100s, vedi WorldTimedRoomThresholdSeconds) */
            WorldSpawnRoomContents(&probeShifted);
            const RoomState *timedRoomShifted = WorldRoomAt(&probeShifted, tx, ty);
            if (!timedRoomShifted->rewardTaken)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: con floorEntryElapsedSeconds=500 e 3s di ritardo nel piano, nessuna ricompensa (la soglia deve misurarsi dal DELTA piano, mai dal timer di run assoluto)\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (probeShifted.player.coins != coinsBeforeShifted + WORLD_ROOM_CURRENCY_TIMED)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u (floorEntryElapsedSeconds=500): valuta attesa %d, ottenuta %d\n",
                        floor, kSeeds[si], coinsBeforeShifted + WORLD_ROOM_CURRENCY_TIMED, probeShifted.player.coins);
                return false;
            }

            printf("  [rooms-p] stanza a tempo (seed %u piano %d): entro soglia -> ricompensa+clessidra attiva; oltre soglia -> nessuna ricompensa, clessidra scaduta, stanza sempre percorribile; soglia misurata dal delta piano anche con floorEntryElapsedSeconds!=0 -> ok\n",
                   kSeeds[si], floor);
            return true;
        }
    }
    fprintf(stderr, "GameRoomsTest: (p) nessuna stanza a tempo nei semi di prova: verifica non eseguita\n");
    return false;
}

/* WP6: contenuto MINIMO ma non vuoto per il piano di prova -- un tipo di
   nemico attivo (senza il quale WorldSpawnEnemyWave cadrebbe sul ramo storico
   "nemici senza tipo", dove il grado non ha manopole da alzare) e tre oggetti
   con rarita' DIVERSE, per poter verificare che la ricompensa dell'arena
   peschi davvero la migliore delle tre e non la prima. Un Game azzerato dal
   memset di RoomsTestGenerateFloor non ha nulla di tutto questo. */
static void RoomsTestInstallArenaContent(Game *g, int floor)
{
    FloorContent *fc = &g->content.floors[floor - 1];
    EnemyTypeDef base;
    EnemyTypeExample(&base, 0);   /* l'inseguitore d'esempio, gia' bilanciato in banda */
    fc->enemies[0] = base;
    fc->enemies[1].active = false;
    fc->items[0].rarity = RARITY_COMMON;
    fc->items[1].rarity = RARITY_RARE;
    fc->items[2].rarity = RARITY_UNCOMMON;
    snprintf(fc->items[0].name, sizeof(fc->items[0].name), "%s", "Comune di prova");
    snprintf(fc->items[1].name, sizeof(fc->items[1].name), "%s", "Rara di prova");
    snprintf(fc->items[2].name, sizeof(fc->items[2].name), "%s", "Non comune di prova");
}

static float RoomsTestTotalEnemyHp(const Game *g)
{
    float total = 0.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) if (g->enemies[i].active) total += g->enemies[i].maxHp;
    return total;
}

static int RoomsTestCountActiveEnemies(const Game *g)
{
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (g->enemies[i].active) n++;
    return n;
}

static Pickup *RoomsTestFindPickup(Game *g, PickupKind kind)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (g->pickups[i].active && g->pickups[i].kind == kind) return &g->pickups[i];
    return NULL;
}

/* Test (q), WP6 (systems/special-rooms.md, "Arena di sfida"): l'intero ciclo di
   vita dell'archetipo su un'arena vera, pescata dai semi di prova.
     A. sfida NON accettata: nessun nemico, porte mai bloccate, si esce davvero
        dalla stanza, toccare il segnale non fa partire nulla, premere il tasto
        di conferma LONTANO dal segnale non fa partire nulla, e la stanza non
        si "ripulisce" da sola (attraversarla non e' completarla: niente
        valuta, niente ricompensa).
     B. sfida ACCETTATA: nemici presenti, porte chiuse, segnale in stato "in
        corso"; budget maggiorato rispetto a una stanza di COMBATTIMENTO della
        stessa taglia/piano con la stessa identica estrazione, e nemici portati
        in fascia alta della banda di potenza.
     C. VITTORIA: valuta dell'arena (mai quella del combattimento), oggetto
        della rarita' migliore fra i tre del piano, porte riaperte, segnale in
        stato "superata" anche rientrando.
     D. controprova: la stessa vittoria in una stanza di COMBATTIMENTO paga la
        valuta del combattimento e non lascia alcun oggetto -- i nemici uccisi
        fuori dall'arena non pagano mai la ricompensa dell'arena. */
static bool RoomsTestArenaInteraction(void)
{
    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u, 90210u, 5150u
    };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        for (int floor = WORLD_ARENA_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT; floor++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);
            RoomsTestInstallArenaContent(&probe, floor);

            int ax = -1, ay = -1, combatX = -1, combatY = -1;
            for (int y = 0; y < GRID_SIZE; y++)
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(&probe, x, y);
                    if (state != &probe.rooms[y][x]) continue;
                    if (state->kind == ROOM_ARENA && ax < 0) { ax = x; ay = y; }
                    if (state->kind == ROOM_COMBAT && combatX < 0) { combatX = x; combatY = y; }
                }
            if (ax < 0) continue;   /* niente arena su questo piano/seme: si prova il prossimo */

            /* ---- A. sfida non accettata ---- */
            probe.roomX = ax; probe.roomY = ay;
            int coinsIdle = probe.player.coins;
            WorldSpawnRoomContents(&probe);
            Pickup *altar = RoomsTestFindPickup(&probe, PICKUP_ARENA_ALTAR);
            if (!altar || altar->value != 0)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: nessun segnale 'sfida disponibile' nell'arena\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (RoomsTestCountActiveEnemies(&probe) != 0 || GameRoomIsLocked(&probe))
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: l'arena parte gia' con nemici o porte bloccate (la sfida NON deve partire entrando)\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* Toccare il segnale non e' una conferma: nessun nemico, nessuno
               stato che cambia, e il segnale non si consuma mai. */
            probe.player.pos = altar->pos;
            CombatUpdatePickups(&probe);
            if (!altar->active || altar->value != 0 || WorldRoomAt(&probe, ax, ay)->arenaActive ||
                RoomsTestCountActiveEnemies(&probe) != 0)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: toccare il segnale fa partire la sfida (serve la conferma esplicita)\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* Il tasto di conferma LONTANO dal segnale non fa partire nulla. */
            Vector2 onAltar = altar->pos;
            probe.player.pos = (Vector2){ onAltar.x + 400.0f, onAltar.y + 200.0f };
            probe.interactQueued = true;
            if (WorldTryStartArenaChallenge(&probe) || WorldRoomAt(&probe, ax, ay)->arenaActive)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: la sfida parte anche lontano dal segnale\n",
                        floor, kSeeds[si]);
                return false;
            }
            probe.interactQueued = false;

            /* Attraversare NON e' completare: WorldCheckRoomClear su un'arena
               senza sfida accettata non deve dichiararla ripulita ne' pagare
               nulla, anche se dentro non c'e' un solo nemico. */
            WorldCheckRoomClear(&probe);
            if (WorldRoomAt(&probe, ax, ay)->cleared || probe.player.coins != coinsIdle)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: l'arena si e' completata (o ha pagato) senza che la sfida partisse\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* Si esce davvero: la stanza e' attraversabile come una vuota. */
            bool leftRoom = false;
            for (int d = 0; d < 4 && !leftRoom; d++)
            {
                Game exitProbe = probe;
                int px, py;
                WorldPlayerCell(&exitProbe, &px, &py);
                for (int cy = 0; cy < GRID_SIZE && !leftRoom; cy++)
                    for (int cx = 0; cx < GRID_SIZE && !leftRoom; cx++)
                    {
                        if (!exitProbe.rooms[cy][cx].exists || !exitProbe.rooms[cy][cx].doors[d]) continue;
                        if (WorldRoomAt(&exitProbe, cx, cy)->kind != ROOM_ARENA) continue;
                        exitProbe.player.keys = 9;
                        Rectangle fromCell = WorldCellRect(&exitProbe, cx, cy);
                        exitProbe.player.pos = (Vector2){ fromCell.x + fromCell.width*0.5f, fromCell.y + fromCell.height*0.5f };
                        exitProbe.roomX = cx; exitProbe.roomY = cy;
                        WorldTryEnterRoom(&exitProbe, d);
                        if (WorldRoomAt(&exitProbe, exitProbe.roomX, exitProbe.roomY)->kind != ROOM_ARENA) leftRoom = true;
                    }
                (void)px; (void)py;
            }
            if (!leftRoom)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: dall'arena con sfida non accettata non si esce\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* ---- B. riferimento: la STESSA stanza come combattimento ---- */
            Game reference = probe;
            RoomState *refRoom = WorldRoomAtMutable(&reference, ax, ay);
            refRoom->kind = ROOM_COMBAT;
            refRoom->cleared = false;
            reference.rng = 4242u;
            EntitiesClear(&reference);
            WorldSpawnCombatRoom(&reference);
            float combatHp = RoomsTestTotalEnemyHp(&reference);
            int combatCount = RoomsTestCountActiveEnemies(&reference);

            /* ---- B. sfida accettata, stessa estrazione ---- */
            probe.player.pos = onAltar;
            probe.rng = 4242u;
            if (!WorldTryStartArenaChallenge(&probe))
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: la conferma sul segnale non fa partire la sfida\n",
                        floor, kSeeds[si]);
                return false;
            }
            const RoomState *arenaRoom = WorldRoomAt(&probe, ax, ay);
            if (!arenaRoom->arenaActive || altar->value != 1)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: sfida accettata ma stato/segnale non aggiornati\n",
                        floor, kSeeds[si]);
                return false;
            }
            int arenaCount = RoomsTestCountActiveEnemies(&probe);
            float arenaHp = RoomsTestTotalEnemyHp(&probe);
            if (arenaCount <= 0 || !GameRoomIsLocked(&probe))
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: sfida accettata ma nessun nemico o porte non bloccate\n",
                        floor, kSeeds[si]);
                return false;
            }
            /* Budget maggiorato: il moltiplicatore e' 1.5, il margine di
               verifica 1.2 assorbe la discretizzazione (l'ultimo nemico si
               spawna anche se sfora un po'). Con lo stesso seme e la stessa
               stanza, una stanza di combattimento normale non puo' avvicinarsi. */
            if (!(arenaHp > combatHp*1.2f))
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: budget non maggiorato -- arena %.1f HP totali (%d nemici) contro combattimento %.1f (%d nemici)\n",
                        floor, kSeeds[si], (double)arenaHp, arenaCount, (double)combatHp, combatCount);
                return false;
            }
            /* Grado piu' alto: ogni nemico dell'arena e' portato in fascia alta
               della sua banda di potenza (enemies.md, il Veterano), mai lasciato
               al valore del tipo di base. */
            float basePower = EnemyTypePower(&probe.content.floors[floor - 1].enemies[0]);
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (!probe.enemies[i].active || !probe.enemies[i].type.active) continue;
                float p = EnemyTypePower(&probe.enemies[i].type);
                if (p <= basePower + 0.001f)
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: nemico d'arena a potenza %.3f, non sopra il tipo base %.3f\n",
                            floor, kSeeds[si], (double)p, (double)basePower);
                    return false;
                }
                if (p > ENEMY_TYPE_POWER_MAX + 0.001f)
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: nemico d'arena a potenza %.3f, FUORI dalla banda dichiarata (max %.3f)\n",
                            floor, kSeeds[si], (double)p, (double)ENEMY_TYPE_POWER_MAX);
                    return false;
                }
            }

            /* ---- C. vittoria ---- */
            for (int i = 0; i < MAX_ENEMIES; i++) probe.enemies[i].active = false;
            int coinsBeforeWin = probe.player.coins;
            WorldCheckRoomClear(&probe);
            if (!WorldRoomAt(&probe, ax, ay)->cleared || GameRoomIsLocked(&probe))
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: sfida vinta ma stanza non completata o porte ancora chiuse\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (probe.player.coins != coinsBeforeWin + WORLD_ROOM_CURRENCY_ARENA)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: valuta attesa %d, ottenuta %d\n",
                        floor, kSeeds[si], coinsBeforeWin + WORLD_ROOM_CURRENCY_ARENA, probe.player.coins);
                return false;
            }
            Pickup *prize = RoomsTestFindPickup(&probe, PICKUP_ITEM);
            if (!prize || prize->item.rarity != RARITY_RARE)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: la ricompensa dell'arena non e' l'oggetto di rarita' migliore del piano\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (altar->value != 2)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: il segnale non passa a 'superata' alla vittoria\n",
                        floor, kSeeds[si]);
                return false;
            }
            /* Rientrando, l'esito resta scritto: nessun nemico, segnale
               'superata', nessuna seconda ricompensa. */
            int coinsAfterWin = probe.player.coins;
            WorldSpawnRoomContents(&probe);
            Pickup *altarAgain = RoomsTestFindPickup(&probe, PICKUP_ARENA_ALTAR);
            if (!altarAgain || altarAgain->value != 2 || RoomsTestCountActiveEnemies(&probe) != 0 ||
                probe.player.coins != coinsAfterWin)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: rientrando nell'arena superata la sfida riparte o ripaga\n",
                        floor, kSeeds[si]);
                return false;
            }

            /* ---- D. controprova fuori dall'arena ---- */
            if (combatX >= 0)
            {
                Game outside;
                RoomsTestGenerateFloor(kSeeds[si], floor, &outside);
                RoomsTestInstallArenaContent(&outside, floor);
                outside.roomX = combatX; outside.roomY = combatY;
                WorldSpawnRoomContents(&outside);
                for (int i = 0; i < MAX_ENEMIES; i++) outside.enemies[i].active = false;
                int coinsBeforeCombat = outside.player.coins;
                WorldCheckRoomClear(&outside);
                /* WORLD_ROOM_CURRENCY_COMBAT resta privata a src/world/world.c:
                   il numero si ripete qui come lo ripete gia' GameEconomyTest
                   (stessa convenzione dichiarata nel suo commento -- se quel
                   valore cambia, entrambi i test vanno aggiornati). Il
                   confronto che conta davvero e' comunque quello con la valuta
                   dell'arena, che NON deve mai uscire da qui. */
                const int kCombatCurrency = 4;
                int paid = outside.player.coins - coinsBeforeCombat;
                if (paid == WORLD_ROOM_CURRENCY_ARENA || paid != kCombatCurrency)
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: una stanza di combattimento ripulita paga %d invece di %d (mai la valuta dell'arena, %d)\n",
                            floor, kSeeds[si], paid, kCombatCurrency, WORLD_ROOM_CURRENCY_ARENA);
                    return false;
                }
                if (RoomsTestFindPickup(&outside, PICKUP_ITEM))
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: una stanza di combattimento ripulita lascia l'oggetto-ricompensa dell'arena\n",
                            floor, kSeeds[si]);
                    return false;
                }
            }

            printf("  [rooms-q] arena di sfida (seed %u piano %d): non accettata -> attraversabile e mai completata; accettata -> %d nemici e %.0f HP contro %d e %.0f di un combattimento pari (budget +50%%, grado in fascia alta), porte chiuse; vinta -> %d Ingots + oggetto di rarita' migliore, segnale 'superata' -> ok\n",
                   kSeeds[si], floor, arenaCount, (double)arenaHp, combatCount, (double)combatHp, WORLD_ROOM_CURRENCY_ARENA);
            return true;
        }
    }
    fprintf(stderr, "GameRoomsTest: (q) nessuna arena di sfida nei semi di prova: verifica non eseguita\n");
    return false;
}

/* ============================================================
   WP7 -- la Pourhouse (systems/special-rooms.md, "Scambio ad alto rischio",
   DEC-136/DEC-044). Verifica il contratto dell'archetipo:
     - la puntata si compone in modo DETERMINISTICO dal seed (stesso seed e
       stesso stato -> stessa puntata; semi diversi -> puntate diverse; due
       Pourhouse nella stessa run -> puntate diverse, Scenario 8);
     - il prezzo non chiede MAI risorse che il giocatore non ha, su piu' stati
       del giocatore, e non porta MAI il tetto di salute sotto un cuore;
     - senza nulla di cedibile la stanza offre l'uscita libera (Scenario 3);
     - l'accettazione e' ATOMICA: prezzo pagato E offerta ricevuta, mai una
       sola delle due, nemmeno quando l'inventario e' pieno o la risorsa e'
       sparita nel frattempo;
     - rifiutare (uscire senza confermare) non costa nulla e non consuma la
       puntata;
     - il Crust non paga mai un prezzo di salute (DEC-008).
   ============================================================ */

/* Contenuto minimo del piano per la Pourhouse: tre oggetti di rarita' diverse
   (l'offerta pesca il migliore) con nomi veri, altrimenti un Game azzerato
   offrirebbe un oggetto senza nome e senza valore. */
static void RoomsTestInstallPourhouseContent(Game *g, int floor)
{
    FloorContent *fc = &g->content.floors[floor - 1];
    fc->items[0].rarity = RARITY_COMMON;
    fc->items[1].rarity = RARITY_RARE;
    fc->items[2].rarity = RARITY_UNCOMMON;
    snprintf(fc->items[0].name, sizeof(fc->items[0].name), "%s", "Scoria Comune");
    snprintf(fc->items[1].name, sizeof(fc->items[1].name), "%s", "Lingua di Fiamma");
    snprintf(fc->items[2].name, sizeof(fc->items[2].name), "%s", "Tenaglia Fredda");
}

/* Un giocatore "ricco": ha qualcosa di ognuna delle cinque categorie di
   prezzo, cosi' il compositore ha davvero da scegliere. Costruito a mano e non
   via GameResetRun perche' questi test girano su un Game locale senza asset. */
static void RoomsTestMakeRichPlayer(Game *g)
{
    Player *p = &g->player;
    p->radius = 14.0f;
    p->baseMaxHp = 12;
    p->maxHp = 12;
    p->hp = 12;
    p->hpCap = 16;
    p->coins = 90;
    p->flux = 3;
    p->bombs = 1;
    p->keys = 1;
    p->tempHp = 0;
    p->itemCount = 2;
    memset(p->items, 0, sizeof(p->items));
    snprintf(p->items[0].name, sizeof(p->items[0].name), "%s", "Anello di Scoria");
    p->items[0].rarity = RARITY_COMMON;
    snprintf(p->items[1].name, sizeof(p->items[1].name), "%s", "Molla Temprata");
    p->items[1].rarity = RARITY_UNCOMMON;
}

/* Il budget di equita' dichiarato in src/world/pourhouse.h, ricalcolato qui
   dalle stesse costanti: se qualcuno allargasse la tolleranza nel codice senza
   passare dalle costanti, questo controllo lo vedrebbe. */
static bool RoomsTestPourhouseEquityOk(const PourhouseWager *w)
{
    int tolerance = w->offerValue*POURHOUSE_EQUITY_TOLERANCE_PERCENT/100;
    if (tolerance < POURHOUSE_EQUITY_TOLERANCE_MIN) tolerance = POURHOUSE_EQUITY_TOLERANCE_MIN;
    int diff = w->offerValue - w->priceValue;
    if (diff < 0) diff = -diff;
    return diff <= tolerance;
}

/* Il controllo che conta di piu' del work package: il prezzo non puo' MAI
   chiedere piu' di quello che il giocatore possiede, e il tetto di salute non
   puo' MAI scendere sotto un cuore. Scritto in modo indipendente da
   WorldPourhousePricePayable (che il codice usa) proprio perche' un difetto
   dentro quella funzione non deve poter passare inosservato qui. */
static bool RoomsTestPourhousePriceWithinMeans(const Game *g, const PourhouseWager *w, const char *ctx)
{
    const Player *p = &g->player;
    switch (w->priceKind)
    {
        case POURHOUSE_PRICE_COINS:
            if (w->priceAmount > p->coins)
            {
                fprintf(stderr, "GameRoomsTest: (r) %s: prezzo di %d Ingots con %d posseduti\n", ctx, w->priceAmount, p->coins);
                return false;
            }
            return true;
        case POURHOUSE_PRICE_HP:
            if (w->priceAmount >= p->hp)
            {
                fprintf(stderr, "GameRoomsTest: (r) %s: prezzo di %d salute con %d punti vita (mai letale)\n",
                        ctx, w->priceAmount, p->hp);
                return false;
            }
            return true;
        case POURHOUSE_PRICE_MAX_HP:
            if (p->baseMaxHp - w->priceAmount < POURHOUSE_MIN_BASE_MAX_HP)
            {
                fprintf(stderr, "GameRoomsTest: (r) %s: prezzo di %d di tetto con tetto %d (mai sotto %d, un cuore)\n",
                        ctx, w->priceAmount, p->baseMaxHp, POURHOUSE_MIN_BASE_MAX_HP);
                return false;
            }
            return true;
        case POURHOUSE_PRICE_FLUX:
            if (w->priceAmount > p->flux)
            {
                fprintf(stderr, "GameRoomsTest: (r) %s: prezzo di %d Flux con %d posseduti\n", ctx, w->priceAmount, p->flux);
                return false;
            }
            return true;
        case POURHOUSE_PRICE_ITEM:
        {
            for (int i = 0; i < p->itemCount && i < MAX_ITEMS; i++)
                if (strcmp(p->items[i].name, w->priceItemName) == 0) return true;
            fprintf(stderr, "GameRoomsTest: (r) %s: prezzo l'oggetto '%s', che il giocatore non possiede\n",
                    ctx, w->priceItemName);
            return false;
        }
        case POURHOUSE_PRICE_COUNT: break;
    }
    fprintf(stderr, "GameRoomsTest: (r) %s: categoria di prezzo sconosciuta (%d)\n", ctx, (int)w->priceKind);
    return false;
}

/* Cerca fra i semi/piani una Pourhouse davvero piazzata, la rende la stanza
   corrente su 'out' e scrive le sue coordinate. Falso se nessuno dei semi ne
   produce una (sarebbe gia' di per se' un difetto: lo segnala il chiamante). */
static bool RoomsTestFindPourhouse(unsigned int seed, int floor, Game *out, int *px, int *py)
{
    RoomsTestGenerateFloor(seed, floor, out);
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!out->rooms[y][x].exists) continue;
            const RoomState *state = WorldRoomAt(out, x, y);
            if (state != &out->rooms[y][x] || state->kind != ROOM_POURHOUSE) continue;
            *px = x; *py = y;
            return true;
        }
    return false;
}

/* Costruisce a mano una puntata di una categoria voluta e la installa nella
   stanza corrente, saltando il compositore: i controlli sull'ATOMICITA' e sul
   tetto devono poter esercitare una categoria PRECISA, non quella che il seed
   ha scelto quel giorno. */
static void RoomsTestForceWager(Game *g, PourhouseOfferKind offerKind, int offerAmount,
                                PourhousePriceKind priceKind, int priceAmount)
{
    PourhouseWager *w = &g->pourhouse;
    memset(w, 0, sizeof(*w));
    w->composed = true;
    w->valid = true;
    w->accepted = false;
    w->roomX = g->roomX;
    w->roomY = g->roomY;
    w->offerKind = offerKind;
    w->offerAmount = offerAmount;
    w->offerKeys = (offerKind == POURHOUSE_OFFER_SUPPLIES) ? offerAmount : 0;
    w->offerValue = 24;
    w->priceKind = priceKind;
    w->priceAmount = priceAmount;
    w->priceValue = 24;
    if (offerKind == POURHOUSE_OFFER_ITEM)
    {
        snprintf(w->offerItem.name, sizeof(w->offerItem.name), "%s", "Colata Vetrificata");
        w->offerItem.rarity = RARITY_RARE;
        w->offerAmount = 1;
    }
    if (priceKind == POURHOUSE_PRICE_ITEM)
    {
        snprintf(w->priceItemName, sizeof(w->priceItemName), "%s", g->player.items[0].name);
        w->priceItemRarity = g->player.items[0].rarity;
        w->priceAmount = 1;
    }
}

static bool RoomsTestPourhouseInteraction(void)
{
    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u,
        20260727u, 424242u, 90210u, 5150u, 137u, 2718u, 31415u, 65537u
    };
    const int kSeedCount = (int)(sizeof(kSeeds)/sizeof(kSeeds[0]));

    /* ---------- A. composizione deterministica e varia ---------- */
    unsigned int seenSignatures[64];
    int seenSignatureCount = 0;
    int composedValid = 0;
    for (int si = 0; si < kSeedCount; si++)
    {
        for (int floor = WORLD_POURHOUSE_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT; floor++)
        {
            Game probe;
            int px = -1, py = -1;
            if (!RoomsTestFindPourhouse(kSeeds[si], floor, &probe, &px, &py)) continue;
            probe.runSeed = kSeeds[si];
            RoomsTestInstallPourhouseContent(&probe, floor);
            RoomsTestMakeRichPlayer(&probe);
            probe.roomX = px; probe.roomY = py;

            PourhouseWager a, b;
            WorldComposePourhouseWager(&probe, px, py, &a);
            WorldComposePourhouseWager(&probe, px, py, &b);
            if (!a.valid)
            {
                fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: un giocatore con Ingots, Flux, salute e due oggetti non trova alcuna puntata\n",
                        floor, kSeeds[si]);
                return false;
            }
            composedValid++;
            if (WorldPourhouseSignature(&a) != WorldPourhouseSignature(&b) ||
                a.offerKind != b.offerKind || a.priceKind != b.priceKind ||
                a.offerAmount != b.offerAmount || a.priceAmount != b.priceAmount ||
                strcmp(a.priceItemName, b.priceItemName) != 0)
            {
                fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: due composizioni con lo stesso seed e lo stesso stato danno puntate diverse\n",
                        floor, kSeeds[si]);
                return false;
            }
            if (!RoomsTestPourhouseEquityOk(&a))
            {
                fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: puntata fuori dal budget di equita' (offerta %d, prezzo %d)\n",
                        floor, kSeeds[si], a.offerValue, a.priceValue);
                return false;
            }
            if (!RoomsTestPourhousePriceWithinMeans(&probe, &a, "composizione su giocatore ricco")) return false;

            unsigned int sig = WorldPourhouseSignature(&a);
            bool seen = false;
            for (int i = 0; i < seenSignatureCount; i++) if (seenSignatures[i] == sig) { seen = true; break; }
            if (!seen && seenSignatureCount < 64) seenSignatures[seenSignatureCount++] = sig;
        }
    }
    if (composedValid == 0)
    {
        fprintf(stderr, "GameRoomsTest: (r) nessuna Pourhouse nei semi di prova: verifica non eseguita\n");
        return false;
    }
    /* Semi diversi -> puntate diverse: se il compositore ignorasse il seed
       tutte le firme coinciderebbero e questo controllo cadrebbe. */
    if (seenSignatureCount < 3)
    {
        fprintf(stderr, "GameRoomsTest: (r) su %d Pourhouse composte esistono solo %d puntate distinte: la composizione non dipende davvero dal seed\n",
                composedValid, seenSignatureCount);
        return false;
    }

    /* ---------- A2. Scenario 8: due Pourhouse nella stessa run ---------- */
    {
        Game run;
        int px = -1, py = -1;
        int firstFloor = -1;
        for (int si = 0; si < kSeedCount && firstFloor < 0; si++)
        {
            for (int floor = WORLD_POURHOUSE_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT; floor++)
            {
                if (!RoomsTestFindPourhouse(kSeeds[si], floor, &run, &px, &py)) continue;
                run.runSeed = kSeeds[si];
                RoomsTestInstallPourhouseContent(&run, floor);
                RoomsTestMakeRichPlayer(&run);
                run.roomX = px; run.roomY = py;
                firstFloor = floor;
                break;
            }
        }
        if (firstFloor < 0)
        {
            fprintf(stderr, "GameRoomsTest: (r) Scenario 8 non eseguibile: nessuna Pourhouse trovata\n");
            return false;
        }
        PourhouseWager first;
        WorldComposePourhouseWager(&run, px, py, &first);

        /* (b) la GARANZIA nuda: stessi identici ingressi (stesso seed, stesso
           piano, stessa cella, stesso giocatore), cambia SOLO la memoria della
           run. Senza il filtro sulla firma dell'ultima puntata la composizione
           tornerebbe identica per costruzione -- e' questo il controllo che
           esercita davvero il meccanismo, non il caso (c) sotto (dove basta il
           cambio di piano a far divergere lo stream). */
        run.pourhouseLastSignature = WorldPourhouseSignature(&first);
        PourhouseWager guarded;
        WorldComposePourhouseWager(&run, px, py, &guarded);
        if (!guarded.valid || WorldPourhouseSignature(&guarded) == WorldPourhouseSignature(&first))
        {
            fprintf(stderr, "GameRoomsTest: (r) Scenario 8: con la firma dell'ultima puntata gia' registrata la composizione ripropone la STESSA puntata\n");
            return false;
        }

        /* (c) il caso di gioco vero: la seconda Pourhouse della stessa run --
           stesso giocatore, piano successivo (e' l'unico modo in cui due
           Pourhouse coesistono, una per piano). Deve proporre una puntata
           DIVERSA da quella appena composta. */
        run.pourhouseLastSignature = WorldPourhouseSignature(&first);
        int nextFloor = (firstFloor < FLOOR_COUNT) ? firstFloor + 1 : firstFloor - 1;
        run.floor = nextFloor;
        RoomsTestInstallPourhouseContent(&run, nextFloor);
        PourhouseWager second;
        WorldComposePourhouseWager(&run, px, py, &second);
        if (!second.valid || WorldPourhouseSignature(&second) == WorldPourhouseSignature(&first))
        {
            fprintf(stderr, "GameRoomsTest: (r) Scenario 8: le due Pourhouse della stessa run propongono la STESSA puntata\n");
            return false;
        }
    }

    /* ---------- B. il prezzo su piu' stati del giocatore ---------- */
    {
        Game probe;
        int px = -1, py = -1;
        int foundFloor = -1;
        for (int si = 0; si < kSeedCount && foundFloor < 0; si++)
            for (int floor = WORLD_POURHOUSE_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT && foundFloor < 0; floor++)
                if (RoomsTestFindPourhouse(kSeeds[si], floor, &probe, &px, &py))
                {
                    probe.runSeed = kSeeds[si];
                    RoomsTestInstallPourhouseContent(&probe, floor);
                    foundFloor = floor;
                }

        /* Sei stati diversi, ciascuno con una risorsa quasi esaurita: la
           composizione deve o proporre una puntata pagabile, o dichiararsi
           fredda -- mai una via di mezzo. */
        for (int state = 0; state < 6; state++)
        {
            Game probeState = probe;
            RoomsTestMakeRichPlayer(&probeState);
            probeState.roomX = px; probeState.roomY = py;
            Player *p = &probeState.player;
            switch (state)
            {
                case 0: break;                                     /* ricco */
                case 1: p->coins = 0; p->flux = 0; break;          /* senza valute */
                case 2: p->itemCount = 0; break;                   /* senza oggetti */
                case 3: p->hp = 1; break;                          /* un solo punto vita */
                case 4: p->baseMaxHp = POURHOUSE_MIN_BASE_MAX_HP; p->hp = 2; break;   /* tetto gia' al minimo */
                default:
                    /* Nulla di nulla: e' il caso limite/Scenario 3. */
                    p->coins = 0; p->flux = 0; p->itemCount = 0; p->hp = 1;
                    p->baseMaxHp = POURHOUSE_MIN_BASE_MAX_HP;
                    break;
            }
            PourhouseWager w;
            WorldComposePourhouseWager(&probeState, px, py, &w);
            if (!w.valid)
            {
                if (state != 5 && state != 1 && state != 2 && state != 3 && state != 4)
                {
                    fprintf(stderr, "GameRoomsTest: (r) stato %d: nessuna puntata anche con risorse abbondanti\n", state);
                    return false;
                }
                continue;
            }
            char ctx[64];
            snprintf(ctx, sizeof(ctx), "stato %d", state);
            if (!RoomsTestPourhousePriceWithinMeans(&probeState, &w, ctx)) return false;
            if (!RoomsTestPourhouseEquityOk(&w))
            {
                fprintf(stderr, "GameRoomsTest: (r) stato %d: puntata fuori dal budget di equita'\n", state);
                return false;
            }
            if (!WorldPourhousePricePayable(&probeState, &w))
            {
                fprintf(stderr, "GameRoomsTest: (r) stato %d: puntata dichiarata valida ma non pagabile\n", state);
                return false;
            }
        }

        /* Lo stato 5 e' anche il caso limite dichiarato dal documento: nessuna
           puntata pagabile -> la stanza dice che la colata e' fredda, non
           blocca le porte, e il tasto di conferma non fa nulla. */
        Game broke;
        RoomsTestGenerateFloor(kSeeds[0], foundFloor, &broke);
        /* Game azzerato: giocatore senza niente per costruzione. */
        broke.roomX = px; broke.roomY = py;
        if (WorldRoomAt(&broke, px, py)->kind == ROOM_POURHOUSE)
        {
            broke.player.radius = 14.0f;
            WorldSpawnRoomContents(&broke);
            Pickup *bank = RoomsTestFindPickup(&broke, PICKUP_POURHOUSE_BANK);
            if (!bank || bank->value != 0 || broke.pourhouse.valid)
            {
                fprintf(stderr, "GameRoomsTest: (r) senza nulla da cedere il banco non dichiara la colata fredda\n");
                return false;
            }
            if (GameRoomIsLocked(&broke))
            {
                fprintf(stderr, "GameRoomsTest: (r) la Pourhouse blocca le porte (mai ammesso, Scenario 3)\n");
                return false;
            }
            Player before = broke.player;
            broke.player.pos = bank->pos;
            broke.interactQueued = true;
            if (WorldTryAcceptPourhouseWager(&broke))
            {
                fprintf(stderr, "GameRoomsTest: (r) si accetta una puntata che non esiste\n");
                return false;
            }
            if (broke.player.coins != before.coins || broke.player.hp != before.hp ||
                broke.player.baseMaxHp != before.baseMaxHp || broke.player.itemCount != before.itemCount)
            {
                fprintf(stderr, "GameRoomsTest: (r) il banco a colata fredda ha comunque cambiato lo stato del giocatore\n");
                return false;
            }
        }
    }

    /* ---------- C/D/E/F. il ciclo di vita su una Pourhouse vera ---------- */
    {
        Game probe;
        int px = -1, py = -1;
        int foundFloor = -1;
        for (int si = 0; si < kSeedCount && foundFloor < 0; si++)
            for (int floor = WORLD_POURHOUSE_ROOM_MIN_FLOOR; floor <= FLOOR_COUNT && foundFloor < 0; floor++)
                if (RoomsTestFindPourhouse(kSeeds[si], floor, &probe, &px, &py))
                {
                    probe.runSeed = kSeeds[si];
                    RoomsTestInstallPourhouseContent(&probe, floor);
                    foundFloor = floor;
                }
        if (foundFloor < 0)
        {
            fprintf(stderr, "GameRoomsTest: (r) nessuna Pourhouse per il ciclo di vita\n");
            return false;
        }

        /* --- C1. accettazione atomica: valuta contro Crust --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_CRUST, 2, POURHOUSE_PRICE_COINS, 24);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank)
            {
                fprintf(stderr, "GameRoomsTest: (r) nessun banco nella Pourhouse\n");
                return false;
            }
            /* Toccare il banco non accetta nulla: serve la conferma esplicita. */
            g.player.pos = bank->pos;
            CombatUpdatePickups(&g);
            if (!bank->active || g.pourhouse.accepted)
            {
                fprintf(stderr, "GameRoomsTest: (r) toccare il banco accetta la puntata (serve la conferma esplicita)\n");
                return false;
            }
            /* Il tasto premuto LONTANO dal banco non fa nulla. */
            Vector2 onBank = bank->pos;
            g.player.pos = (Vector2){ onBank.x + 400.0f, onBank.y + 200.0f };
            if (WorldTryAcceptPourhouseWager(&g) || g.pourhouse.accepted)
            {
                fprintf(stderr, "GameRoomsTest: (r) la puntata si accetta anche lontano dal banco\n");
                return false;
            }
            int coinsBefore = g.player.coins, crustBefore = g.player.tempHp;
            g.player.pos = onBank;
            if (!WorldTryAcceptPourhouseWager(&g))
            {
                fprintf(stderr, "GameRoomsTest: (r) la conferma sul banco non accetta la puntata\n");
                return false;
            }
            if (g.player.coins != coinsBefore - 24 || g.player.tempHp != crustBefore + 2)
            {
                fprintf(stderr, "GameRoomsTest: (r) accettazione non atomica: Ingots %d -> %d, Crust %d -> %d\n",
                        coinsBefore, g.player.coins, crustBefore, g.player.tempHp);
                return false;
            }
            if (!g.pourhouse.accepted || bank->value != 2)
            {
                fprintf(stderr, "GameRoomsTest: (r) la puntata accettata non risulta versata sul banco\n");
                return false;
            }
            /* Una seconda conferma non ripaga e non ricompra: una sola puntata
               per stanza per run. */
            int coinsAfter = g.player.coins, crustAfter = g.player.tempHp;
            if (WorldTryAcceptPourhouseWager(&g) || g.player.coins != coinsAfter || g.player.tempHp != crustAfter)
            {
                fprintf(stderr, "GameRoomsTest: (r) la stessa puntata si accetta due volte\n");
                return false;
            }
            /* Rientrando: la puntata resta versata, il banco lo dice, niente si ripete. */
            WorldSpawnRoomContents(&g);
            Pickup *bankAgain = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bankAgain || bankAgain->value != 2 || g.player.coins != coinsAfter || g.player.tempHp != crustAfter)
            {
                fprintf(stderr, "GameRoomsTest: (r) rientrando nella Pourhouse la puntata si ricompone o si ripaga\n");
                return false;
            }
        }

        /* --- C2. fallimento a meta': inventario pieno --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.player.itemCount = MAX_ITEMS;   /* nessuno slot libero */
            for (int i = 0; i < MAX_ITEMS; i++) snprintf(g.player.items[i].name, sizeof(g.player.items[i].name), "Zavorra %d", i);
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_ITEM, 1, POURHOUSE_PRICE_COINS, 24);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank) return false;
            g.player.pos = bank->pos;
            int coinsBefore = g.player.coins;
            int itemsBefore = g.player.itemCount;
            if (WorldTryAcceptPourhouseWager(&g))
            {
                fprintf(stderr, "GameRoomsTest: (r) si accetta una puntata la cui offerta non entra nell'inventario\n");
                return false;
            }
            if (g.player.coins != coinsBefore || g.player.itemCount != itemsBefore || g.pourhouse.accepted)
            {
                fprintf(stderr, "GameRoomsTest: (r) offerta non consegnabile ma prezzo comunque incassato (%d -> %d Ingots)\n",
                        coinsBefore, g.player.coins);
                return false;
            }
        }

        /* --- C3. la risorsa sparisce fra composizione e conferma --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_COINS, 30, POURHOUSE_PRICE_ITEM, 1);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank) return false;
            /* Il giocatore sgancia/perde proprio quell'oggetto prima di
               confermare: la puntata non e' piu' pagabile. */
            g.player.itemCount = 0;
            g.player.pos = bank->pos;
            int coinsBefore = g.player.coins;
            if (WorldTryAcceptPourhouseWager(&g) || g.player.coins != coinsBefore || g.pourhouse.accepted)
            {
                fprintf(stderr, "GameRoomsTest: (r) si accetta una puntata il cui prezzo non e' piu' posseduto\n");
                return false;
            }
        }

        /* --- C4. il prezzo in oggetto toglie DAVVERO quell'oggetto --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_COINS, 30, POURHOUSE_PRICE_ITEM, 1);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank) return false;
            char sold[48];
            snprintf(sold, sizeof(sold), "%s", g.pourhouse.priceItemName);
            int itemsBefore = g.player.itemCount, coinsBefore = g.player.coins;
            g.player.pos = bank->pos;
            if (!WorldTryAcceptPourhouseWager(&g))
            {
                fprintf(stderr, "GameRoomsTest: (r) la puntata a prezzo di oggetto non si accetta\n");
                return false;
            }
            if (g.player.itemCount != itemsBefore - 1 || g.player.coins != coinsBefore + 30)
            {
                fprintf(stderr, "GameRoomsTest: (r) prezzo in oggetto: oggetti %d -> %d, Ingots %d -> %d\n",
                        itemsBefore, g.player.itemCount, coinsBefore, g.player.coins);
                return false;
            }
            for (int i = 0; i < g.player.itemCount; i++)
                if (strcmp(g.player.items[i].name, sold) == 0)
                {
                    fprintf(stderr, "GameRoomsTest: (r) l'oggetto venduto '%s' e' ancora nell'inventario\n", sold);
                    return false;
                }
        }

        /* --- D. rifiuto: uscire senza confermare non costa nulla --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            if (!g.pourhouse.valid)
            {
                fprintf(stderr, "GameRoomsTest: (r) il giocatore ricco non trova una puntata all'ingresso\n");
                return false;
            }
            unsigned int sigBefore = WorldPourhouseSignature(&g.pourhouse);
            Player before = g.player;
            /* Si esce davvero dalla stanza: la Pourhouse non blocca mai. */
            bool leftRoom = false;
            for (int d = 0; d < 4 && !leftRoom; d++)
            {
                Game exitProbe = g;
                if (!exitProbe.rooms[py][px].doors[d]) continue;
                exitProbe.player.keys = 9;
                Rectangle fromCell = WorldCellRect(&exitProbe, px, py);
                exitProbe.player.pos = (Vector2){ fromCell.x + fromCell.width*0.5f, fromCell.y + fromCell.height*0.5f };
                WorldTryEnterRoom(&exitProbe, d);
                if (WorldRoomAt(&exitProbe, exitProbe.roomX, exitProbe.roomY)->kind != ROOM_POURHOUSE) leftRoom = true;
            }
            if (!leftRoom)
            {
                fprintf(stderr, "GameRoomsTest: (r) dalla Pourhouse senza accettare non si esce\n");
                return false;
            }
            if (g.player.coins != before.coins || g.player.hp != before.hp ||
                g.player.baseMaxHp != before.baseMaxHp || g.player.flux != before.flux ||
                g.player.itemCount != before.itemCount || g.pourhouse.accepted)
            {
                fprintf(stderr, "GameRoomsTest: (r) rifiutare la puntata ha comunque avuto un costo\n");
                return false;
            }
            /* Tornando indietro si ritrova LA STESSA puntata, non una nuova:
               il rifiuto non la consuma (default proposto dall'implementazione). */
            WorldSpawnRoomContents(&g);
            if (WorldPourhouseSignature(&g.pourhouse) != sigBefore)
            {
                fprintf(stderr, "GameRoomsTest: (r) rientrando dopo un rifiuto la puntata e' cambiata\n");
                return false;
            }
        }

        /* --- E. salute massima: mai sotto un cuore, mai col Crust --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.player.tempHp = PLAYER_TEMP_HP_CAP;   /* Crust pieno: non deve pagare nulla */
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_COINS, 30, POURHOUSE_PRICE_MAX_HP, 2);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank) return false;
            g.player.pos = bank->pos;
            int capBefore = g.player.baseMaxHp, crustBefore = g.player.tempHp;
            if (!WorldTryAcceptPourhouseWager(&g))
            {
                fprintf(stderr, "GameRoomsTest: (r) la puntata a prezzo di salute massima non si accetta\n");
                return false;
            }
            if (g.player.baseMaxHp != capBefore - 2)
            {
                fprintf(stderr, "GameRoomsTest: (r) il tetto non e' sceso: %d -> %d\n", capBefore, g.player.baseMaxHp);
                return false;
            }
            if (g.player.maxHp > g.player.baseMaxHp)
            {
                fprintf(stderr, "GameRoomsTest: (r) il tetto derivato (maxHp %d) non segue baseMaxHp %d\n",
                        g.player.maxHp, g.player.baseMaxHp);
                return false;
            }
            if (g.player.tempHp != crustBefore)
            {
                fprintf(stderr, "GameRoomsTest: (r) DEC-008: il Crust ha pagato un prezzo di salute (%d -> %d)\n",
                        crustBefore, g.player.tempHp);
                return false;
            }
            /* Un ricalcolo successivo (raccogliere un oggetto, un frame
               qualunque) non deve restituire il tetto perduto: il prezzo piu'
               rischioso della Pourhouse dev'essere davvero permanente. */
            int capAfter = g.player.maxHp;
            g.statsDirty = true;
            ScriptItemsProcessDirty(&g);
            if (g.player.maxHp != capAfter)
            {
                fprintf(stderr, "GameRoomsTest: (r) il tetto perduto torna al primo ricalcolo (%d -> %d)\n",
                        capAfter, g.player.maxHp);
                return false;
            }
            /* Il limite duro: il tetto non scende mai sotto un cuore, per
               quanto grande sia la richiesta. */
            CombatReducePlayerMaxHp(&g, 999);
            if (g.player.baseMaxHp < POURHOUSE_MIN_BASE_MAX_HP || g.player.hp < 1)
            {
                fprintf(stderr, "GameRoomsTest: (r) il tetto e' sceso sotto un cuore (baseMaxHp %d, hp %d)\n",
                        g.player.baseMaxHp, g.player.hp);
                return false;
            }
        }

        /* --- F. prezzo in salute: solo la salute BASE, mai il Crust --- */
        {
            Game g = probe;
            RoomsTestMakeRichPlayer(&g);
            g.player.tempHp = 3;
            g.roomX = px; g.roomY = py;
            WorldSpawnRoomContents(&g);
            RoomsTestForceWager(&g, POURHOUSE_OFFER_COINS, 24, POURHOUSE_PRICE_HP, 6);
            Pickup *bank = RoomsTestFindPickup(&g, PICKUP_POURHOUSE_BANK);
            if (!bank) return false;
            g.player.pos = bank->pos;
            int hpBefore = g.player.hp, crustBefore = g.player.tempHp;
            if (!WorldTryAcceptPourhouseWager(&g)) return false;
            if (g.player.hp != hpBefore - 6 || g.player.tempHp != crustBefore)
            {
                fprintf(stderr, "GameRoomsTest: (r) prezzo in salute: hp %d -> %d, Crust %d -> %d (DEC-008: il Crust non paga mai)\n",
                        hpBefore, g.player.hp, crustBefore, g.player.tempHp);
                return false;
            }
            if (g.player.hp < 1)
            {
                fprintf(stderr, "GameRoomsTest: (r) un prezzo in salute ha ucciso il giocatore\n");
                return false;
            }
        }
    }

    printf("  [rooms-r] Pourhouse (WP7): %d puntate composte, %d distinte fra i semi; prezzo sempre dentro le risorse possedute, budget di equita' rispettato, accettazione atomica, rifiuto gratuito, tetto mai sotto un cuore, Crust mai usato per pagare -> ok\n",
           composedValid, seenSignatureCount);
    return true;
}

/* WP8 (systems/special-rooms.md "Stanza segreta", systems/secrets-and-obstacles.md
   "Segreti", DEC-025): cerca nel piano (seed, floor) una stanza segreta del
   livello richiesto. Scrive la cella della segreta, la cella VICINA da cui si
   vede il muro, e la direzione che porta DALLA VICINA ALLA SEGRETA (la
   direzione del varco murato). Falso se quel piano non ne ha una. */
static bool RoomsTestFindSecret(unsigned int seed, int floor, bool wantSuper, Game *out,
                                int *sx, int *sy, int *nx, int *ny, int *dirToSecret)
{
    RoomsTestGenerateFloor(seed, floor, out);
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!out->rooms[y][x].exists) continue;
            const RoomState *state = WorldRoomAt(out, x, y);
            if (state != &out->rooms[y][x]) continue;
            if (state->kind != ROOM_SECRET) continue;
            if (state->secretSuper != wantSuper) continue;
            for (int d = 0; d < 4; d++)
            {
                int cx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                int cy = y + ((d == DIR_DOWN) - (d == DIR_UP));
                if (cx < 0 || cx >= GRID_SIZE || cy < 0 || cy >= GRID_SIZE) continue;
                if (!out->rooms[cy][cx].exists) continue;
                *sx = x; *sy = y;
                *nx = cx; *ny = cy;
                *dirToSecret = (d + 2)%4;   /* dalla vicina verso la segreta */
                return true;
            }
        }
    }
    return false;
}

/* WP8: porta il Game nella stanza VICINA alla segreta, col giocatore al centro
   della cella da cui si vede il muro. Non serve nulla di piu': lo strumento di
   breccia guarda solo (posizione, raggio) e la stanza corrente. */
static void RoomsTestStandNextToSecret(Game *g, int nx, int ny)
{
    g->roomX = nx;
    g->roomY = ny;
    WorldRoomAtMutable(g, nx, ny)->cleared = true;   /* niente porte bloccate: la geometria e' il soggetto */
    Rectangle cell = WorldCellRect(g, nx, ny);
    g->player.pos = (Vector2){ cell.x + cell.width*0.5f, cell.y + cell.height*0.5f };
    g->player.keys = 9;
}

/* Test (s) dedicato, WP8: il ciclo di vita completo dell'archetipo su una
   segreta vera pescata dai semi di prova.
     A. PRIMA della breccia: fuori dalla minimappa, zero porte, indizio visibile
        solo per il livello NORMALE (mai per la super-segreta, DEC-025).
     B. Esplosione LONTANA dalla parete: non apre nulla. Esplosione SULLA
        parete ma di origine NEMICA (breach=false): non apre nulla -- la stessa
        garanzia che il WP3 dichiara per i distruttibili.
     C. Esplosione SULLA parete con lo strumento di breccia: apre il varco, su
        entrambi i lati, e la stanza compare sulla mappa.
     D. PERSISTENZA: si entra, si esce, si rientra -- il varco resta aperto e
        la valuta di "segreta trovata" (DEC-167) si paga UNA volta sola.
     E. CONTENUTO: l'oggetto e' quello di rarita' migliore fra i tre del piano,
        e' lo STESSO ad ogni rientro (mai una seconda estrazione) e sparisce
        quando la stanza e' stata svuotata. La super-segreta versa in piu' il
        catalizzatore di fusione, una volta sola.
     F. La super-segreta si apre lo stesso senza alcun indizio (DEC-025:
        "intuizione estrema"). */
static bool RoomsTestSecretRooms(void)
{
    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u,
        11u, 137u, 2718u, 31415u, 20260727u, 424242u, 90210u, 5150u
    };
    const int kSeedCount = (int)(sizeof(kSeeds)/sizeof(kSeeds[0]));

    for (int level = 0; level < 2; level++)
    {
        bool wantSuper = (level == 1);
        Game g;
        int sx = -1, sy = -1, nx = -1, ny = -1, dir = -1;
        unsigned int usedSeed = 0;
        int usedFloor = 0;
        bool found = false;
        for (int floor = 1; floor <= FLOOR_COUNT && !found; floor++)
        {
            for (int si = 0; si < kSeedCount && !found; si++)
            {
                if (!RoomsTestFindSecret(kSeeds[si], floor, wantSuper, &g, &sx, &sy, &nx, &ny, &dir)) continue;
                found = true;
                usedSeed = kSeeds[si];
                usedFloor = floor;
            }
        }
        if (!found)
        {
            fprintf(stderr, "GameRoomsTest: (s) nessuna stanza segreta di livello %s nei semi di prova: verifica non eseguita\n",
                    wantSuper ? "super" : "normale");
            return false;
        }
        RoomsTestInstallArenaContent(&g, usedFloor);   /* tre oggetti con rarita' diverse: comune/rara/non comune */
        const FloorContent *fc = &g.content.floors[usedFloor - 1];
        const char *bestName = fc->items[1].name;      /* RARITY_RARE, la migliore delle tre */

        /* A. prima della breccia. */
        const RoomState *secret = &g.rooms[sy][sx];
        if (!WorldRoomHiddenOnMap(secret) || secret->secretOpened)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: la segreta %s non nasce murata\n",
                    usedSeed, usedFloor, wantSuper ? "super" : "normale");
            return false;
        }
        if (WorldSecretClueVisible(secret) == wantSuper)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: indizio visibile %d su una segreta %s (DEC-025: la super non ne ha MAI uno)\n",
                    usedSeed, usedFloor, (int)WorldSecretClueVisible(secret), wantSuper ? "super" : "normale");
            return false;
        }
        if (g.rooms[sy][sx].doors[(dir + 2)%4] || g.rooms[ny][nx].doors[dir])
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: il varco murato ha gia' una porta\n", usedSeed, usedFloor);
            return false;
        }

        /* B. due esplosioni che NON devono aprire nulla. */
        RoomsTestStandNextToSecret(&g, nx, ny);
        CombatExplodeAt(&g, g.player.pos, 74.0f, 0.0f, true);   /* al centro della stanza: lontano dalla parete */
        if (g.rooms[sy][sx].secretOpened)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: una bomba in mezzo alla stanza apre il varco (deve servire la parete giusta)\n",
                    usedSeed, usedFloor);
            return false;
        }
        Rectangle wall = WorldSecretWallRect(&g, nx, ny, dir);
        Vector2 onWall = { wall.x + wall.width*0.5f, wall.y + wall.height*0.5f };
        CombatExplodeAt(&g, onWall, 74.0f, 0.0f, false);   /* origine NEMICA: mai un varco */
        if (g.rooms[sy][sx].secretOpened)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: un'esplosione di origine nemica apre il varco (mai ammesso)\n",
                    usedSeed, usedFloor);
            return false;
        }

        /* C. lo strumento di breccia, sulla parete giusta. */
        if (!WorldTryBreachSecretWall(&g, onWall, 74.0f))
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: lo strumento di breccia sulla parete condivisa non apre il varco\n",
                    usedSeed, usedFloor);
            return false;
        }
        if (!g.rooms[sy][sx].secretOpened || !g.rooms[sy][sx].doors[(dir + 2)%4] || !g.rooms[ny][nx].doors[dir])
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: varco aperto ma la porta manca su un lato (%d/%d)\n",
                    usedSeed, usedFloor, (int)g.rooms[sy][sx].doors[(dir + 2)%4], (int)g.rooms[ny][nx].doors[dir]);
            return false;
        }
        if (WorldRoomHiddenOnMap(&g.rooms[sy][sx]) || WorldSecretClueVisible(&g.rooms[sy][sx]))
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: aperta, la segreta resta fuori dalla mappa o mostra ancora l'indizio\n",
                    usedSeed, usedFloor);
            return false;
        }

        /* D+E. si entra: valuta di "segreta trovata", oggetto migliore del
           piano, e per la super anche il catalizzatore di fusione. */
        int coinsBefore = g.player.coins;
        int fluxBefore = g.player.flux;
        WorldTryEnterRoom(&g, dir);
        if (WorldRoomAt(&g, g.roomX, g.roomY)->kind != ROOM_SECRET)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: dal varco aperto non si entra nella segreta\n", usedSeed, usedFloor);
            return false;
        }
        if (g.player.coins - coinsBefore != WORLD_ROOM_CURRENCY_SECRET)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: valuta di segreta trovata attesa %d, ottenuta %d\n",
                    usedSeed, usedFloor, WORLD_ROOM_CURRENCY_SECRET, g.player.coins - coinsBefore);
            return false;
        }
        int expectedFlux = wantSuper ? WORLD_SECRET_SUPER_FLUX : 0;
        if (g.player.flux - fluxBefore != expectedFlux)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: Flux atteso %d sulla segreta %s, ottenuto %d\n",
                    usedSeed, usedFloor, expectedFlux, wantSuper ? "super" : "normale", g.player.flux - fluxBefore);
            return false;
        }
        if (GameRoomIsLocked(&g))
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: la stanza segreta blocca le porte (mai ammesso)\n", usedSeed, usedFloor);
            return false;
        }
        int itemCount = 0;
        Pickup *prize = NULL;
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (g.pickups[i].active && g.pickups[i].kind == PICKUP_ITEM) { itemCount++; prize = &g.pickups[i]; }
        if (itemCount != 1 || !prize || strcmp(prize->item.name, bestName) != 0)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: %d oggetti nella segreta (atteso 1, '%s')\n",
                    usedSeed, usedFloor, itemCount, bestName);
            return false;
        }

        /* D. si esce e si rientra: il varco resta aperto, la valuta non si
           ripaga, l'oggetto e' lo STESSO e resta uno solo. */
        WorldTryEnterRoom(&g, (dir + 2)%4);
        if (WorldRoomAt(&g, g.roomX, g.roomY)->kind == ROOM_SECRET)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: dalla segreta aperta non si esce\n", usedSeed, usedFloor);
            return false;
        }
        int coinsOutside = g.player.coins;
        int fluxOutside = g.player.flux;
        WorldTryEnterRoom(&g, dir);
        if (WorldRoomAt(&g, g.roomX, g.roomY)->kind != ROOM_SECRET || !g.rooms[sy][sx].secretOpened)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: rientrando, il varco si e' richiuso\n", usedSeed, usedFloor);
            return false;
        }
        if (g.player.coins != coinsOutside || g.player.flux != fluxOutside)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: rientrare nella segreta ripaga (Ingots %d->%d, Flux %d->%d)\n",
                    usedSeed, usedFloor, coinsOutside, g.player.coins, fluxOutside, g.player.flux);
            return false;
        }
        itemCount = 0;
        prize = NULL;
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (g.pickups[i].active && g.pickups[i].kind == PICKUP_ITEM) { itemCount++; prize = &g.pickups[i]; }
        if (itemCount != 1 || !prize || strcmp(prize->item.name, bestName) != 0)
        {
            fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: rientrando la segreta offre %d oggetti (atteso sempre lo stesso, uno solo)\n",
                    usedSeed, usedFloor, itemCount);
            return false;
        }

        /* E. una volta svuotata (l'oggetto e' stato preso: CombatPickup scrive
           'rewardTaken', stesso campo del tesoro) la stanza resta
           attraversabile e vuota. */
        WorldRoomAtMutable(&g, sx, sy)->rewardTaken = true;
        WorldTryEnterRoom(&g, (dir + 2)%4);
        WorldTryEnterRoom(&g, dir);
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (g.pickups[i].active && g.pickups[i].kind == PICKUP_ITEM)
            {
                fprintf(stderr, "GameRoomsTest: (s) seed %u piano %d: la segreta gia' svuotata offre un altro oggetto\n",
                        usedSeed, usedFloor);
                return false;
            }

        printf("  [rooms-s] segreta %s (seed %u piano %d): murata e fuori mappa, indizio %s; bomba lontana o di origine nemica -> nessun varco; bomba sulla parete -> varco su entrambi i lati; valuta %d una volta sola%s; oggetto di rarita' migliore, lo stesso ad ogni rientro -> ok\n",
               wantSuper ? "super" : "normale", usedSeed, usedFloor,
               wantSuper ? "assente" : "presente", WORLD_ROOM_CURRENCY_SECRET,
               wantSuper ? ", piu' il catalizzatore di fusione" : "");
    }
    return true;
}

/* Le stanze del piano, una riga per stanza (la sua cella di STATO). */
typedef struct RoomsTestRoom {
    int stateX, stateY;
    unsigned char cells;
    RoomKind kind;
} RoomsTestRoom;

static int RoomsTestCollectRooms(const Game *game, RoomsTestRoom *out, int maxOut)
{
    int n = 0;
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!game->rooms[y][x].exists) continue;
            const RoomState *state = WorldRoomAt(game, x, y);
            if (state != &game->rooms[y][x]) continue;   /* gia' contata dalla sua cella di stato */
            if (n >= maxOut) return n;
            out[n].stateX = x;
            out[n].stateY = y;
            out[n].cells = state->cells;
            out[n].kind = state->kind;
            n++;
        }
    }
    return n;
}

bool GameRoomsTest(Game *game)
{
    /* Si genera sempre un Game LOCALE pulito (RoomsTestGenerateFloor): quello
       passato da AppRun serve solo a rispettare la stessa firma/convenzione
       di GamePortalRespawnTest e simili. */
    (void)game;
    bool ok = true;

    static const unsigned int kSeeds[] = {
        1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u,
        11u, 137u, 2718u, 31415u, 65537u, 99991u, 123456u, 777777u,
        20260727u, 90210u, 424242u, 5150u, 8675309u, 314159u, 271828u, 161803u
    };
    const int kSeedCount = (int)(sizeof(kSeeds)/sizeof(kSeeds[0]));

    /* (c) "varia tra piani/seed": si registrano i conteggi distinti di CELLE
       visti durante l'intero giro sotto, e si pretende almeno due valori. */
    int seenCounts[64];
    int seenCountsN = 0;
    /* (b) quante volte si e' vista ciascuna classe di taglia: la distribuzione
       deve davvero produrre piu' di una classe, o "taglie multiple" sarebbe
       vero solo sulla carta. */
    int sizeSeen[ROOM_SIZE_COUNT];
    for (int i = 0; i < (int)ROOM_SIZE_COUNT; i++) sizeSeen[i] = 0;
    int bossSizeSeen[ROOM_SIZE_COUNT];
    for (int i = 0; i < (int)ROOM_SIZE_COUNT; i++) bossSizeSeen[i] = 0;
    int floorsChecked = 0;
    /* WP4 (o): quanti dei piani/semi sotto hanno trovato posto per la stanza di
       fusione -- "quando la fusione trova posto le garanzie esistenti reggono"
       (spec del work package) presuppone che il giro di prova la eserciti
       davvero almeno una volta, non solo che non si rompa quando manca. */
    int floorsWithFusion = 0;
    /* WP5 (p): stessa idea di floorsWithFusion sopra, ma per la stanza a
       tempo -- SOLO piani avanzati (WORLD_TIMED_ROOM_MIN_FLOOR), quindi ci si
       aspetta zero occorrenze sui piani 1-2 e almeno una sui piani 3-5. */
    int floorsWithTimed = 0;
    /* WP5: denominatore corretto per il rendiconto (p) sotto -- i piani 1-2
       non tentano NEMMENO il piazzamento (WORLD_TIMED_ROOM_MIN_FLOOR), quindi
       contarli nel totale ("69 su 120") sarebbe fuorviante quanto dire che un
       test "fallisce" su domande a cui non e' nemmeno stato sottoposto.
       floorsWithFusion/floorsChecked sopra non ha lo stesso problema: la
       fusione non ha un piano minimo. */
    int floorsEligibleForTimed = 0;
    /* WP6 (q): stessa idea di floorsWithFusion/floorsWithTimed -- quante volte
       l'arena di sfida ha trovato posto, sui soli piani candidati
       (WORLD_ARENA_ROOM_MIN_FLOOR). */
    int floorsWithArena = 0;
    int floorsEligibleForArena = 0;
    /* WP6: quante volte ciascuna classe di taglia e' toccata all'arena --
       serve a verificare che il piazzamento "prima le grandi" sia vivo, non
       solo dichiarato nel commento. */
    int arenaSizeSeen[ROOM_SIZE_COUNT];
    for (int i = 0; i < (int)ROOM_SIZE_COUNT; i++) arenaSizeSeen[i] = 0;
    /* WP7 (r): quante volte la Pourhouse compare, sui soli piani candidati
       (WORLD_POURHOUSE_ROOM_MIN_FLOOR). A differenza delle altre quattro 1x1
       qui ci si aspetta un numero SENSIBILMENTE minore dei candidati: non e' un
       servizio di piano, e' un archetipo raro (WORLD_POURHOUSE_ROOM_CHANCE_PERCENT). */
    int floorsWithPourhouse = 0;
    int floorsEligibleForPourhouse = 0;
    /* WP8 (s): quante volte ciascun livello di stanza segreta ha trovato posto,
       sui rispettivi piani candidati. La super-segreta e' dichiarata PIU' RARA
       della normale (DEC-025): la verifica finale confronta i due conteggi,
       non solo "almeno una". */
    int floorsWithSecret = 0;
    int floorsWithSuperSecret = 0;
    int floorsEligibleForSecret = 0;
    int floorsEligibleForSuperSecret = 0;

    for (int floor = 1; floor <= FLOOR_COUNT; floor++)
    {
        for (int si = 0; si < kSeedCount; si++)
        {
            Game probe;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe);
            floorsChecked++;
            if (floor >= WORLD_TIMED_ROOM_MIN_FLOOR) floorsEligibleForTimed++;
            if (floor >= WORLD_ARENA_ROOM_MIN_FLOOR) floorsEligibleForArena++;
            if (floor >= WORLD_POURHOUSE_ROOM_MIN_FLOOR) floorsEligibleForPourhouse++;
            if (floor >= WORLD_SECRET_ROOM_MIN_FLOOR) floorsEligibleForSecret++;
            if (floor >= WORLD_SECRET_SUPER_MIN_FLOOR) floorsEligibleForSuperSecret++;

            RoomsTestRoom rooms[GRID_SIZE*GRID_SIZE];
            int roomCount = RoomsTestCollectRooms(&probe, rooms, GRID_SIZE*GRID_SIZE);
            int cellCount = 0;
            int bossRooms = 0, startRooms = 0;
            int bossX = -1, bossY = -1;
            /* WP4: quante stanze di fusione compaiono in QUESTO piano (al piu'
               una, WorldPlaceSpecialRoom fa un solo tentativo) e la sua cella di
               stato, per il controllo (o) sotto. */
            int fusionRooms = 0;
            int fusionX = -1, fusionY = -1;
            /* WP5: idem per la stanza a tempo, controllo (p) sotto. */
            int timedRooms = 0;
            int timedX = -1, timedY = -1;
            /* WP6: idem per l'arena di sfida, controllo (q) sotto. */
            int arenaRooms = 0;
            int arenaX = -1, arenaY = -1;
            /* WP7: idem per la Pourhouse, controllo (r) sotto. */
            int pourhouseRooms = 0;
            int pourhouseX = -1, pourhouseY = -1;
            /* WP8: le stanze segrete di questo piano, per il controllo (s)
               sotto. Al piu' DUE (una normale + una super), e i due livelli si
               contano separati perche' hanno frequenze diverse. */
            int secretRooms = 0, superSecretRooms = 0;
            int secretX[2], secretY[2];

            /* (a) ogni CELLA esistente appartiene a una stanza sola, e la
               stanza e' una delle cinque classi di DEC-170; (b) niente
               sovrapposizioni: le celle dichiarate dalla maschera esistono
               tutte e puntano tutte alla stessa origine. */
            for (int r = 0; r < roomCount; r++)
            {
                const RoomState *state = &probe.rooms[rooms[r].stateY][rooms[r].stateX];
                unsigned char mask = rooms[r].cells;
                int bits = 0;
                for (int i = 0; i < 4; i++) if (mask & (1u << i)) bits++;
                if (bits < 1 || bits > 4)
                {
                    fprintf(stderr, "GameRoomsTest: (a) piano %d seed %u: maschera di celle non valida (0x%X)\n",
                            floor, kSeeds[si], mask);
                    ok = false;
                    continue;
                }
                RoomSize size = WorldRoomSizeFromCells(mask);
                sizeSeen[size]++;
                if (rooms[r].kind == ROOM_BOSS) { bossRooms++; bossSizeSeen[size]++; bossX = rooms[r].stateX; bossY = rooms[r].stateY; }
                if (rooms[r].kind == ROOM_START) startRooms++;
                if (rooms[r].kind == ROOM_FUSION) { fusionRooms++; fusionX = rooms[r].stateX; fusionY = rooms[r].stateY; }
                if (rooms[r].kind == ROOM_TIMED) { timedRooms++; timedX = rooms[r].stateX; timedY = rooms[r].stateY; }
                if (rooms[r].kind == ROOM_ARENA) { arenaRooms++; arenaSizeSeen[size]++; arenaX = rooms[r].stateX; arenaY = rooms[r].stateY; }
                if (rooms[r].kind == ROOM_SECRET)
                {
                    if (secretRooms < 2) { secretX[secretRooms] = rooms[r].stateX; secretY[secretRooms] = rooms[r].stateY; }
                    secretRooms++;
                    if (state->secretSuper) superSecretRooms++;
                    if (size != ROOM_SIZE_1X1)
                    {
                        fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta non e' 1x1 (classe %d)\n",
                                floor, kSeeds[si], (int)size);
                        ok = false;
                    }
                    if (state->secretOpened)
                    {
                        fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta nasce gia' aperta (il varco deve essere murato)\n",
                                floor, kSeeds[si]);
                        ok = false;
                    }
                }
                if (rooms[r].kind == ROOM_POURHOUSE)
                {
                    pourhouseRooms++;
                    pourhouseX = rooms[r].stateX;
                    pourhouseY = rooms[r].stateY;
                    if (size != ROOM_SIZE_1X1)
                    {
                        fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: la Pourhouse non e' 1x1 (classe %d)\n",
                                floor, kSeeds[si], (int)size);
                        ok = false;
                    }
                }
                cellCount += bits;

                /* La forma a L e' TRE celle di un blocco 2x2 con un angolo
                   mancante: con tre bit accesi su quattro e' vero per
                   costruzione, ma il test lo verifica lo stesso perche' e' la
                   forma che potrebbe piu' facilmente degenerare in due celle
                   non contigue se la maschera venisse scritta male. */
                if (size == ROOM_SIZE_L && bits != 3)
                {
                    fprintf(stderr, "GameRoomsTest: (a) piano %d seed %u: forma a L con %d celle\n", floor, kSeeds[si], bits);
                    ok = false;
                }
                for (int i = 0; i < 4; i++)
                {
                    if (!(mask & (1u << i))) continue;
                    int cx = state->originX + (i & 1);
                    int cy = state->originY + (i >> 1);
                    if (cx < 0 || cx >= GRID_SIZE || cy < 0 || cy >= GRID_SIZE)
                    {
                        fprintf(stderr, "GameRoomsTest: (b) piano %d seed %u: stanza fuori griglia in (%d,%d)\n",
                                floor, kSeeds[si], cx, cy);
                        ok = false;
                        continue;
                    }
                    const RoomState *cell = &probe.rooms[cy][cx];
                    if (!cell->exists || cell->cells != mask ||
                        cell->originX != state->originX || cell->originY != state->originY)
                    {
                        fprintf(stderr, "GameRoomsTest: (b) piano %d seed %u: la cella (%d,%d) non appartiene alla stanza che la dichiara\n",
                                floor, kSeeds[si], cx, cy);
                        ok = false;
                    }
                }

                /* Il riquadro non scende mai sotto la grandezza minima
                   garantita (DEC-009, oggi la taglia 1x1). */
                Rectangle rect = WorldRoomRect(&probe, rooms[r].stateX, rooms[r].stateY);
                if (rect.width < (float)WORLD_ROOM_MIN_W - 0.5f || rect.height < (float)WORLD_ROOM_MIN_H - 0.5f)
                {
                    fprintf(stderr, "GameRoomsTest: (a) piano %d seed %u: stanza sotto il minimo garantito (%.0fx%.0f)\n",
                            floor, kSeeds[si], rect.width, rect.height);
                    ok = false;
                }
            }

            /* Ogni cella esistente deve essere coperta da esattamente una
               stanza (il contrario della verifica sopra: si guarda dal lato
               delle celle, cosi' una cella "orfana" non passa inosservata). */
            int existing = 0;
            for (int y = 0; y < GRID_SIZE; y++)
                for (int x = 0; x < GRID_SIZE; x++)
                    if (probe.rooms[y][x].exists) existing++;
            if (existing != cellCount)
            {
                fprintf(stderr, "GameRoomsTest: (b) piano %d seed %u: %d celle esistenti ma %d dichiarate dalle stanze\n",
                        floor, kSeeds[si], existing, cellCount);
                ok = false;
            }

            /* (f) esattamente una stanza boss e una di partenza. */
            if (bossRooms != 1 || startRooms != 1)
            {
                fprintf(stderr, "GameRoomsTest: (f) piano %d seed %u: %d stanze boss, %d di partenza\n",
                        floor, kSeeds[si], bossRooms, startRooms);
                ok = false;
            }
            if (bossX >= 0 && probe.rooms[bossY][bossX].cleared)
            {
                fprintf(stderr, "GameRoomsTest: (f) piano %d seed %u: la stanza boss nasce gia' ripulita\n", floor, kSeeds[si]);
                ok = false;
            }

            /* (o) WP4: al piu' una stanza di fusione per piano (un solo
               tentativo di piazzamento, mai garantito -- puo' anche essere
               zero) e, quando c'e', mai adiacente alla stanza boss --
               WorldPlaceSpecialRoom scarta gia' le celle candidate che la
               toccano (stessa regola di tesoro/negozio, DEC-182), ma questo
               test lo verifica sul risultato invece di fidarsi soltanto
               dell'implementazione. */
            if (fusionRooms > 1)
            {
                fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: %d stanze di fusione (atteso al piu' 1)\n",
                        floor, kSeeds[si], fusionRooms);
                ok = false;
            }
            if (fusionRooms == 1)
            {
                floorsWithFusion++;
                const RoomState *fusionState = &probe.rooms[fusionY][fusionX];
                for (int i = 0; i < 4; i++)
                {
                    if (!(fusionState->cells & (unsigned char)(1u << i))) continue;
                    int cx = fusionState->originX + (i & 1), cy = fusionState->originY + (i >> 1);
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = cx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = cy + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists) continue;
                        if (WorldRoomAt(&probe, nx, ny)->kind != ROOM_BOSS) continue;
                        fprintf(stderr, "GameRoomsTest: (o) piano %d seed %u: la stanza di fusione tocca la stanza boss\n",
                                floor, kSeeds[si]);
                        ok = false;
                    }
                }
            }

            /* (p) WP5: al piu' una stanza a tempo per piano (un solo tentativo
               di piazzamento, mai garantito), MAI prima del piano 3
               (WORLD_TIMED_ROOM_MIN_FLOOR: e' parte della decisione DEC-051
               stessa, "esclusiva dei piani avanzati", non solo un default di
               frequenza) e, quando c'e', mai adiacente alla stanza boss --
               stesso schema del controllo (o) sopra per la fusione. */
            if (timedRooms > 1)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: %d stanze a tempo (atteso al piu' 1)\n",
                        floor, kSeeds[si], timedRooms);
                ok = false;
            }
            if (timedRooms > 0 && floor < 3)
            {
                fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: stanza a tempo fuori dai piani avanzati\n",
                        floor, kSeeds[si]);
                ok = false;
            }
            if (timedRooms == 1)
            {
                floorsWithTimed++;
                const RoomState *timedState = &probe.rooms[timedY][timedX];
                for (int i = 0; i < 4; i++)
                {
                    if (!(timedState->cells & (unsigned char)(1u << i))) continue;
                    int cx = timedState->originX + (i & 1), cy = timedState->originY + (i >> 1);
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = cx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = cy + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists) continue;
                        if (WorldRoomAt(&probe, nx, ny)->kind != ROOM_BOSS) continue;
                        fprintf(stderr, "GameRoomsTest: (p) piano %d seed %u: la stanza a tempo tocca la stanza boss\n",
                                floor, kSeeds[si]);
                        ok = false;
                    }
                }
            }

            /* (q) WP6, arena di sfida: al piu' una per piano (un solo
               tentativo di piazzamento, mai garantito), MAI prima del piano
               WORLD_ARENA_ROOM_MIN_FLOOR, MAI adiacente alla stanza boss (le
               darebbe una seconda porta, DEC-182), MAI piu' piccola di due
               celle (una 1x1 mortificherebbe un combattimento maggiorato) e
               SEMPRE foglia del grafo di adiacenza -- esattamente una porta su
               tutto il perimetro. La foglia e' la garanzia strutturale del
               caso limite di special-rooms.md ("mai un passaggio obbligato,
               mai un blocco del piano se ignorata"): accettare la sfida chiude
               le porte, e su un nodo di passaggio taglierebbe il piano in due. */
            if (arenaRooms > 1)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: %d arene di sfida (atteso al piu' 1)\n",
                        floor, kSeeds[si], arenaRooms);
                ok = false;
            }
            if (arenaRooms > 0 && floor < WORLD_ARENA_ROOM_MIN_FLOOR)
            {
                fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: arena di sfida prima del piano minimo %d\n",
                        floor, kSeeds[si], WORLD_ARENA_ROOM_MIN_FLOOR);
                ok = false;
            }
            if (arenaRooms == 1)
            {
                floorsWithArena++;
                const RoomState *arenaState = &probe.rooms[arenaY][arenaX];
                int arenaCellCount = 0, arenaDoorCount = 0;
                for (int i = 0; i < 4; i++)
                {
                    if (!(arenaState->cells & (unsigned char)(1u << i))) continue;
                    arenaCellCount++;
                    int cx = arenaState->originX + (i & 1), cy = arenaState->originY + (i >> 1);
                    for (int d = 0; d < 4; d++)
                    {
                        if (probe.rooms[cy][cx].doors[d]) arenaDoorCount++;
                        int nx = cx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = cy + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists) continue;
                        if (WorldRoomAt(&probe, nx, ny)->kind != ROOM_BOSS) continue;
                        fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: l'arena di sfida tocca la stanza boss\n",
                                floor, kSeeds[si]);
                        ok = false;
                    }
                }
                if (arenaCellCount < 2)
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: arena di sfida da %d cella (mai sotto le due, special-rooms.md)\n",
                            floor, kSeeds[si], arenaCellCount);
                    ok = false;
                }
                if (arenaDoorCount != 1)
                {
                    fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: l'arena di sfida ha grado %d nel grafo (atteso 1: mai un passaggio obbligato)\n",
                            floor, kSeeds[si], arenaDoorCount);
                    ok = false;
                }

                /* (q) "mai bloccare il piano se ignorata": BFS dalla partenza
                   che NON entra mai in una cella dell'arena -- tutte le altre
                   stanze restano raggiungibili. Stesso schema del controllo
                   (n) per la stanza boss, ma qui la garanzia e' piu' forte
                   nella pratica: la stanza boss si attraversa una volta sola a
                   fine piano, l'arena si puo' ignorare per sempre. */
                bool reachedNoArena[GRID_SIZE][GRID_SIZE];
                memset(reachedNoArena, 0, sizeof(reachedNoArena));
                int aqx[GRID_SIZE*GRID_SIZE], aqy[GRID_SIZE*GRID_SIZE];
                int aHead = 0, aTail = 0;
                aqx[aTail] = probe.roomX; aqy[aTail] = probe.roomY; aTail++;
                reachedNoArena[probe.roomY][probe.roomX] = true;
                while (aHead < aTail)
                {
                    int x = aqx[aHead], y = aqy[aHead];
                    aHead++;
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = y + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists || reachedNoArena[ny][nx]) continue;
                        if (WorldRoomAt(&probe, nx, ny)->kind == ROOM_ARENA) continue;   /* nodo ignorato dal giocatore */
                        if (!probe.rooms[y][x].doors[d] && !WorldSameRoom(&probe, x, y, nx, ny)) continue;
                        reachedNoArena[ny][nx] = true;
                        aqx[aTail] = nx; aqy[aTail] = ny; aTail++;
                    }
                }
                for (int y = 0; y < GRID_SIZE; y++)
                    for (int x = 0; x < GRID_SIZE; x++)
                        if (probe.rooms[y][x].exists && WorldRoomAt(&probe, x, y)->kind != ROOM_ARENA &&
                            !WorldRoomHiddenOnMap(WorldRoomAt(&probe, x, y)) && !reachedNoArena[y][x])
                        {
                            fprintf(stderr, "GameRoomsTest: (q) piano %d seed %u: la cella (%d,%d) non e' raggiungibile ignorando l'arena di sfida\n",
                                    floor, kSeeds[si], x, y);
                            ok = false;
                        }
            }

            /* (r) WP7, Pourhouse: al piu' una per piano, MAI prima del piano
               WORLD_POURHOUSE_ROOM_MIN_FLOOR e, quando c'e', mai adiacente
               alla stanza boss -- stesso schema dei controlli (o)/(p) per
               fusione e stanza a tempo, perche' e' la stessa
               WorldPlaceSpecialRoom a piazzarla (la sua quinta chiamante). */
            if (pourhouseRooms > 1)
            {
                fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: %d Pourhouse (atteso al piu' 1)\n",
                        floor, kSeeds[si], pourhouseRooms);
                ok = false;
            }
            if (pourhouseRooms > 0 && floor < WORLD_POURHOUSE_ROOM_MIN_FLOOR)
            {
                fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: Pourhouse prima del piano minimo %d\n",
                        floor, kSeeds[si], WORLD_POURHOUSE_ROOM_MIN_FLOOR);
                ok = false;
            }
            if (pourhouseRooms == 1)
            {
                floorsWithPourhouse++;
                const RoomState *phState = &probe.rooms[pourhouseY][pourhouseX];
                for (int i = 0; i < 4; i++)
                {
                    if (!(phState->cells & (unsigned char)(1u << i))) continue;
                    int cx = phState->originX + (i & 1), cy = phState->originY + (i >> 1);
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = cx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = cy + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists) continue;
                        RoomKind nk = WorldRoomAt(&probe, nx, ny)->kind;
                        if (nk != ROOM_BOSS && nk != ROOM_ARENA) continue;
                        fprintf(stderr, "GameRoomsTest: (r) piano %d seed %u: la Pourhouse tocca una stanza che deve restare foglia (%s)\n",
                                floor, kSeeds[si], GameRoomKindName(nk));
                        ok = false;
                    }
                }
            }

            /* (s) WP8, stanze segrete (DEC-025, special-rooms.md "Stanza
               segreta"). Le garanzie, tutte verificate sul RISULTATO e non
               sulla fiducia nell'implementazione:
                 - al piu' UNA per livello, quindi al piu' due in tutto;
                 - la super-segreta mai prima del suo piano minimo;
                 - SEMPRE 1x1 e mai gia' aperta (verificato sopra, nel giro
                   sulle stanze);
                 - ZERO porte finche' il varco e' murato: e' l'invariante che
                   la tiene fuori dalla connettivita' del piano;
                 - ESATTAMENTE UNA cella vicina esistente -- un solo muro
                   condiviso, cioe' un solo indizio e un solo varco;
                 - quella vicina e' una stanza NORMALE (partenza/combattimento):
                   mai boss ne' arena (devono restare foglie, DEC-182 e
                   special-rooms.md), mai un'altra speciale, mai un'altra
                   segreta. */
            if (secretRooms > 2 || superSecretRooms > 1 || (secretRooms - superSecretRooms) > 1)
            {
                fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: %d stanze segrete di cui %d super (atteso al piu' 1 per livello)\n",
                        floor, kSeeds[si], secretRooms, superSecretRooms);
                ok = false;
            }
            if (superSecretRooms > 0 && floor < WORLD_SECRET_SUPER_MIN_FLOOR)
            {
                fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: super-segreta prima del piano minimo %d\n",
                        floor, kSeeds[si], WORLD_SECRET_SUPER_MIN_FLOOR);
                ok = false;
            }
            if (secretRooms > 0 && floor < WORLD_SECRET_ROOM_MIN_FLOOR)
            {
                fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: stanza segreta prima del piano minimo %d\n",
                        floor, kSeeds[si], WORLD_SECRET_ROOM_MIN_FLOOR);
                ok = false;
            }
            floorsWithSecret += (secretRooms - superSecretRooms) > 0 ? 1 : 0;
            floorsWithSuperSecret += superSecretRooms > 0 ? 1 : 0;
            for (int s = 0; s < secretRooms && s < 2; s++)
            {
                int sx = secretX[s], sy = secretY[s];
                const RoomState *secretState = &probe.rooms[sy][sx];
                int secretDoors = 0, secretNeighbours = 0;
                for (int d = 0; d < 4; d++)
                {
                    if (probe.rooms[sy][sx].doors[d]) secretDoors++;
                    int nx = sx + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                    int ny = sy + ((d == DIR_DOWN) - (d == DIR_UP));
                    if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                    if (!probe.rooms[ny][nx].exists) continue;
                    secretNeighbours++;
                    RoomKind nk = WorldRoomAt(&probe, nx, ny)->kind;
                    if (nk != ROOM_START && nk != ROOM_COMBAT)
                    {
                        fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta confina con '%s' invece che con una stanza normale\n",
                                floor, kSeeds[si], GameRoomKindName(nk));
                        ok = false;
                    }
                }
                if (secretDoors != 0)
                {
                    fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta ha %d porte aperte in generazione (il varco deve essere murato)\n",
                            floor, kSeeds[si], secretDoors);
                    ok = false;
                }
                if (secretNeighbours != 1)
                {
                    fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta tocca %d celle esistenti (atteso esattamente 1: un solo muro condiviso)\n",
                            floor, kSeeds[si], secretNeighbours);
                    ok = false;
                }
                if (!WorldRoomHiddenOnMap(secretState))
                {
                    fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: la stanza segreta compare sulla minimappa prima di essere aperta\n",
                            floor, kSeeds[si]);
                    ok = false;
                }
                /* DEC-025: la super-segreta non mostra MAI l'indizio; la
                   normale lo mostra sempre finche' il varco e' murato. */
                if (WorldSecretClueVisible(secretState) == secretState->secretSuper)
                {
                    fprintf(stderr, "GameRoomsTest: (s) piano %d seed %u: livello %s e indizio visibile %d: i due livelli di DEC-025 non si distinguono\n",
                            floor, kSeeds[si], secretState->secretSuper ? "super" : "normale",
                            (int)WorldSecretClueVisible(secretState));
                    ok = false;
                }
            }

            /* (c) banda attesa delle CELLE: il budget e' 6+piano+(0..3), piu'
               fino a 4 celle di stanza boss e le celle speciali 1x1 (tesoro,
               negozio, fusione -- WP4 -- e, dal piano 3, anche la stanza a
               tempo -- WP5, la QUARTA); una forma grande puo' sforare il
               budget di al massimo 3 celle (l'ultima piazzata). WP6: dal piano
               WORLD_ARENA_ROOM_MIN_FLOOR si aggiunge l'arena di sfida, che NON
               e' 1x1 -- ha un piazzamento suo che prova le taglie grandi per
               prime e non scende mai sotto le due celle, quindi vale fino a
               4 celle in piu' come la stanza boss. WP7: dal piano
               WORLD_POURHOUSE_ROOM_MIN_FLOOR si aggiunge la QUINTA speciale
               1x1, la Pourhouse -- una cella sola, e solo quando l'estrazione
               del piano la concede. */
            int lowerBound = 6 + floor;
            int specialRoomSlots = (floor >= 3) ? 4 : 3;
            if (floor >= WORLD_POURHOUSE_ROOM_MIN_FLOOR) specialRoomSlots++;
            int arenaCells = (floor >= WORLD_ARENA_ROOM_MIN_FLOOR) ? 4 : 0;
            /* WP8: le stanze segrete sono celle IN PIU', mai una sostituzione
               (spec: "extra, non sostitutiva") -- una 1x1 per la segreta
               normale e una per la super-segreta, ciascuna solo dai rispettivi
               piani minimi. */
            int secretCells = (floor >= WORLD_SECRET_ROOM_MIN_FLOOR) ? 1 : 0;
            if (floor >= WORLD_SECRET_SUPER_MIN_FLOOR) secretCells++;
            int upperBound = 6 + floor + 3 + 3 + 4 + specialRoomSlots + arenaCells + secretCells;
            if (existing < lowerBound || existing > upperBound)
            {
                fprintf(stderr, "GameRoomsTest: (c) piano %d seed %u ha %d celle, fuori dalla banda attesa [%d,%d]\n",
                        floor, kSeeds[si], existing, lowerBound, upperBound);
                ok = false;
            }
            bool alreadySeen = false;
            for (int i = 0; i < seenCountsN; i++) if (seenCounts[i] == existing) { alreadySeen = true; break; }
            if (!alreadySeen && seenCountsN < 64) seenCounts[seenCountsN++] = existing;

            /* (i) le porte: doors[d] vero implica SEMPRE che il vicino esista
               e sia di un'ALTRA stanza (mai attraverso il bordo della
               griglia, mai fra due celle sorelle della stessa stanza -- sono
               lo stesso spazio continuo, DEC-170). Non e' piu' vero il
               contrario da DEC-181: due stanze adiacenti possono condividere
               una coppia di celle SENZA porta, se quella coppia non e' il
               segmento scelto -- verificato sotto dal test (l). */
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    for (int d = 0; d < 4; d++)
                    {
                        if (!probe.rooms[y][x].doors[d]) continue;
                        int nx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = y + ((d == DIR_DOWN) - (d == DIR_UP));
                        bool inGrid = (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE);
                        bool validDoor = inGrid && probe.rooms[ny][nx].exists && !WorldSameRoom(&probe, x, y, nx, ny);
                        if (!validDoor)
                        {
                            fprintf(stderr, "GameRoomsTest: (i) piano %d seed %u: porta incoerente in (%d,%d) dir %d\n",
                                    floor, kSeeds[si], x, y, d);
                            ok = false;
                        }
                    }
                }
            }

            /* (l) DEC-181: al massimo (e almeno, o la coppia non sarebbe
               connessa) UNA porta per coppia di stanze adiacenti, anche
               quando il confine condiviso copre piu' di una coppia di
               celle. Si raggruppano i segmenti di confine per coppia di
               stanze, identificata dal PUNTATORE alla cella di stato
               risolto da WorldRoomAt -- MAI dall'origine grezza
               (originX/originY): due stanze diverse possono avere lo
               stesso valore numerico di origine quando la maschera di una
               di esse non include il bit (0,0) (l'origine e' allora solo
               un ancoraggio geometrico, non la cella di stato), quindi
               originX/originY da soli non sono un identificativo univoco
               di stanza. Si conta quante porte sono aperte in ciascun
               gruppo: deve essere sempre esattamente 1. */
            {
                typedef struct { const RoomState *ra, *rb; int doorsOpen; bool sealedSecret; } RoomsTestPair;
                RoomsTestPair pairs[GRID_SIZE*GRID_SIZE*2];
                int pairCount = 0;
                for (int y = 0; y < GRID_SIZE; y++)
                {
                    for (int x = 0; x < GRID_SIZE; x++)
                    {
                        if (!probe.rooms[y][x].exists) continue;
                        static const int kFwdDirs[2] = { DIR_RIGHT, DIR_DOWN };
                        for (int k = 0; k < 2; k++)
                        {
                            int d = kFwdDirs[k];
                            int nx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                            int ny = y + ((d == DIR_DOWN) - (d == DIR_UP));
                            if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                            if (!probe.rooms[ny][nx].exists) continue;
                            if (WorldSameRoom(&probe, x, y, nx, ny)) continue;
                            const RoomState *ra = WorldRoomAt(&probe, x, y);
                            const RoomState *rb = WorldRoomAt(&probe, nx, ny);
                            if (ra > rb) { const RoomState *t = ra; ra = rb; rb = t; }
                            int pi = -1;
                            for (int p = 0; p < pairCount; p++)
                                if (pairs[p].ra == ra && pairs[p].rb == rb) { pi = p; break; }
                            if (pi < 0)
                            {
                                pi = pairCount++;
                                pairs[pi].ra = ra; pairs[pi].rb = rb;
                                pairs[pi].doorsOpen = 0;
                                /* WP8: la coppia che contiene una stanza
                                   segreta col varco ancora MURATO e' l'unica
                                   che deve avere ZERO porte -- e' il muro che
                                   la rende segreta (DEC-025), non un difetto
                                   di DEC-181. Appena il varco si apre la
                                   coppia torna sotto la regola di sempre (una
                                   porta e una sola), verificato dal test
                                   dedicato RoomsTestSecretRooms. */
                                pairs[pi].sealedSecret = WorldRoomHiddenOnMap(ra) || WorldRoomHiddenOnMap(rb);
                            }
                            if (probe.rooms[y][x].doors[d]) pairs[pi].doorsOpen++;
                        }
                    }
                }
                for (int p = 0; p < pairCount; p++)
                {
                    int expected = pairs[p].sealedSecret ? 0 : 1;
                    if (pairs[p].doorsOpen != expected)
                    {
                        fprintf(stderr, "GameRoomsTest: (l) piano %d seed %u: coppia di stanze stato (%d,%d)-(%d,%d) ha %d porte (atteso %d%s)\n",
                                floor, kSeeds[si], pairs[p].ra->originX, pairs[p].ra->originY,
                                pairs[p].rb->originX, pairs[p].rb->originY, pairs[p].doorsOpen, expected,
                                pairs[p].sealedSecret ? ", varco segreto murato" : "");
                        ok = false;
                    }
                }
            }

            /* (m) DEC-182: la stanza boss ha grado 1 -- esattamente una porta
               aperta su tutto il perimetro delle sue celle (mai zero, mai
               piu' di una: e' sempre una foglia del grafo di adiacenza). */
            if (bossX >= 0)
            {
                const RoomState *bossState = &probe.rooms[bossY][bossX];
                int bossDoorCount = 0;
                for (int i = 0; i < 4; i++)
                {
                    if (!(bossState->cells & (unsigned char)(1u << i))) continue;
                    int cx = bossState->originX + (i & 1), cy = bossState->originY + (i >> 1);
                    for (int d = 0; d < 4; d++) if (probe.rooms[cy][cx].doors[d]) bossDoorCount++;
                }
                if (bossDoorCount != 1)
                {
                    fprintf(stderr, "GameRoomsTest: (m) piano %d seed %u: la stanza boss ha grado %d nel grafo (atteso 1, DEC-182)\n",
                            floor, kSeeds[si], bossDoorCount);
                    ok = false;
                }

                /* (n) DEC-182: rimuovendo la stanza boss (e la sua unica
                   porta) dal grafo, tutte le ALTRE stanze del piano restano
                   raggiungibili fra loro -- BFS dalla partenza che non entra
                   MAI in una cella della stanza boss. Il tipo di una stanza
                   e' scritto SOLO sulla sua cella di stato (WorldWriteRoom):
                   le altre celle di una stanza boss multi-cella leggono
                   '.kind' col valore di default (ROOM_START, l'enum a zero
                   di un memset), quindi il confronto va fatto sul tipo della
                   cella di stato risolta da WorldRoomAt, mai sul campo
                   '.kind' grezzo della cella visitata. */
                bool reachedNoBoss[GRID_SIZE][GRID_SIZE];
                memset(reachedNoBoss, 0, sizeof(reachedNoBoss));
                int nqx[GRID_SIZE*GRID_SIZE], nqy[GRID_SIZE*GRID_SIZE];
                int nHead = 0, nTail = 0;
                nqx[nTail] = probe.roomX; nqy[nTail] = probe.roomY; nTail++;
                reachedNoBoss[probe.roomY][probe.roomX] = true;
                while (nHead < nTail)
                {
                    int x = nqx[nHead], y = nqy[nHead];
                    nHead++;
                    for (int d = 0; d < 4; d++)
                    {
                        int nx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                        int ny = y + ((d == DIR_DOWN) - (d == DIR_UP));
                        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                        if (!probe.rooms[ny][nx].exists || reachedNoBoss[ny][nx]) continue;
                        if (WorldRoomAt(&probe, nx, ny)->kind == ROOM_BOSS) continue;  /* nodo rimosso dal grafo */
                        if (!probe.rooms[y][x].doors[d] && !WorldSameRoom(&probe, x, y, nx, ny)) continue;
                        reachedNoBoss[ny][nx] = true;
                        nqx[nTail] = nx; nqy[nTail] = ny; nTail++;
                    }
                }
                for (int y = 0; y < GRID_SIZE; y++)
                    for (int x = 0; x < GRID_SIZE; x++)
                        if (probe.rooms[y][x].exists && WorldRoomAt(&probe, x, y)->kind != ROOM_BOSS &&
                            !WorldRoomHiddenOnMap(WorldRoomAt(&probe, x, y)) && !reachedNoBoss[y][x])
                        {
                            fprintf(stderr, "GameRoomsTest: (n) piano %d seed %u: la cella (%d,%d) non e' raggiungibile senza passare dalla stanza boss\n",
                                    floor, kSeeds[si], x, y);
                            ok = false;
                        }
            }

            /* (j) CONNETTIVITA': dalla partenza si raggiunge ogni stanza del
               piano attraversando porte. Una stanza isolata renderebbe il piano
               incompletabile, ed e' il rischio vero di un generatore a forme.
               WP8: la connettivita' si misura SENZA CONTARE LE SEGRETE ancora
               murate -- sono per definizione fuori dal grafo finche' non si
               sbreccia il muro, ed e' esattamente la garanzia del documento
               ("il piano resta completabile ignorandole"). Che si aprano
               davvero, e che aperte entrino nel grafo come una stanza
               qualsiasi, lo verifica RoomsTestSecretRooms. */
            bool reached[GRID_SIZE][GRID_SIZE];
            memset(reached, 0, sizeof(reached));
            int queueX[GRID_SIZE*GRID_SIZE], queueY[GRID_SIZE*GRID_SIZE];
            int head = 0, tail = 0;
            queueX[tail] = probe.roomX; queueY[tail] = probe.roomY; tail++;
            reached[probe.roomY][probe.roomX] = true;
            while (head < tail)
            {
                int x = queueX[head], y = queueY[head];
                head++;
                for (int d = 0; d < 4; d++)
                {
                    int nx = x + ((d == DIR_RIGHT) - (d == DIR_LEFT));
                    int ny = y + ((d == DIR_DOWN) - (d == DIR_UP));
                    if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE) continue;
                    if (!probe.rooms[ny][nx].exists || reached[ny][nx]) continue;
                    /* Si passa se e' la stessa stanza (spazio continuo) o se
                       c'e' una porta. */
                    if (!probe.rooms[y][x].doors[d] && !WorldSameRoom(&probe, x, y, nx, ny)) continue;
                    reached[ny][nx] = true;
                    queueX[tail] = nx; queueY[tail] = ny; tail++;
                }
            }
            for (int y = 0; y < GRID_SIZE; y++)
                for (int x = 0; x < GRID_SIZE; x++)
                    if (probe.rooms[y][x].exists && !WorldRoomHiddenOnMap(WorldRoomAt(&probe, x, y)) && !reached[y][x])
                    {
                        fprintf(stderr, "GameRoomsTest: (j) piano %d seed %u: la cella (%d,%d) non e' raggiungibile dalla partenza\n",
                                floor, kSeeds[si], x, y);
                        ok = false;
                    }

            /* (d) determinismo: rigenerare LO STESSO piano con lo stesso seed
               deve produrre la STESSA griglia (esistenza, tipo, FORMA, origine,
               porte). */
            Game probe2;
            RoomsTestGenerateFloor(kSeeds[si], floor, &probe2);
            bool detOk = true;
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    RoomState *a = &probe.rooms[y][x];
                    RoomState *b = &probe2.rooms[y][x];
                    if (a->exists != b->exists) { detOk = false; continue; }
                    if (!a->exists) continue;
                    if (a->kind != b->kind || a->cells != b->cells ||
                        a->originX != b->originX || a->originY != b->originY) detOk = false;
                    /* WP8: anche il LIVELLO della segreta e lo stato del suo
                       varco fanno parte del piano -- due run con lo stesso
                       seed devono trovare la stessa segreta, dello stesso
                       livello, murata allo stesso modo. Senza questo confronto
                       uno stream non deterministico nel piazzamento passerebbe
                       inosservato ogni volta che le due segrete cadono
                       comunque sulla stessa cella. */
                    if (a->secretSuper != b->secretSuper || a->secretOpened != b->secretOpened) detOk = false;
                    for (int d = 0; d < 4; d++) if (a->doors[d] != b->doors[d]) detOk = false;
                }
            }
            if (!detOk)
            {
                fprintf(stderr, "GameRoomsTest: (d) piano %d seed %u non deterministico: due generazioni con lo stesso seed differiscono\n",
                        floor, kSeeds[si]);
                ok = false;
            }

            /* (e) ogni transizione di porta atterra DENTRO una cella occupata
               della stanza di arrivo (non nel riquadro e basta: l'angolo
               mancante di una forma a L e' muro). Si forza 'cleared' sulla
               stanza di partenza per isolare la geometria dal gate di
               combattimento (gia' coperto da --portal-test) e si danno chiavi
               in abbondanza per non far fallire l'ingresso in una stanza
               tesoro non ancora visitata. */
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!probe.rooms[y][x].exists) continue;
                    for (int dir = 0; dir < 4; dir++)
                    {
                        if (!probe.rooms[y][x].doors[dir]) continue;
                        /* roomX/roomY puo' essere QUALUNQUE cella della
                           stanza (gli accessori risolvono da soli la cella di
                           stato); a dire da quale porta si esce e' la
                           POSIZIONE del giocatore, non piu' la cella corrente. */
                        probe.roomX = x;
                        probe.roomY = y;
                        WorldRoomAtMutable(&probe, x, y)->cleared = true;
                        Rectangle fromCell = WorldCellRect(&probe, x, y);
                        probe.player.pos = (Vector2){ fromCell.x + fromCell.width*0.5f, fromCell.y + fromCell.height*0.5f };
                        probe.player.keys = 9;
                        WorldTryEnterRoom(&probe, dir);

                        int landX, landY;
                        WorldPlayerCell(&probe, &landX, &landY);
                        Rectangle landCell = WorldCellRect(&probe, landX, landY);
                        const float tol = 0.5f;
                        bool inside = probe.player.pos.x >= landCell.x - tol && probe.player.pos.x <= landCell.x + landCell.width + tol &&
                                      probe.player.pos.y >= landCell.y - tol && probe.player.pos.y <= landCell.y + landCell.height + tol;
                        if (!inside || !probe.rooms[landY][landX].exists)
                        {
                            fprintf(stderr, "GameRoomsTest: (e) transizione da (%d,%d) dir %d piano %d seed %u non atterra dentro una cella della stanza di arrivo\n",
                                    x, y, dir, floor, kSeeds[si]);
                            ok = false;
                        }
                    }
                }
            }
        }
    }

    if (seenCountsN < 2)
    {
        fprintf(stderr, "GameRoomsTest: (c) il numero di celle non varia mai fra i piani/seed testati (osservato un solo valore)\n");
        ok = false;
    }
    /* Tutte e cinque le classi devono comparire davvero nel giro di prova: e'
       la verifica che la distribuzione di DEC-170 sia viva, non un enum
       inutilizzato. */
    for (int i = 0; i < (int)ROOM_SIZE_COUNT; i++)
    {
        if (sizeSeen[i] > 0) continue;
        fprintf(stderr, "GameRoomsTest: (b) la classe di taglia %d non compare mai in %d piani generati\n", i, floorsChecked);
        ok = false;
    }
    /* Default proposto: la stanza boss e' un'arena, e la 2x2 resta la classe
       PREFERITA -- non garantita (puo' non entrare nella griglia), e da
       DEC-182 (30/07) deve anche toccare una sola stanza esistente (grado 1,
       foglia del grafo): una 2x2 ha piu' perimetro di una 1x1, quindi piu'
       occasioni di toccare due stanze diverse, e la soglia di frequenza
       scende di conseguenza rispetto al default pre-DEC-182 (era "quasi
       sempre", ~110/120: misurato ora ~54/120). La soglia resta comunque un
       margine di sicurezza sotto il valore misurato, non il valore esatto. */
    if (bossSizeSeen[ROOM_SIZE_2X2] < floorsChecked/4)
    {
        fprintf(stderr, "GameRoomsTest: (f) la stanza boss e' 2x2 solo %d volte su %d piani (default proposto: la piu' frequente fra le classi grandi anche dopo DEC-182)\n",
                bossSizeSeen[ROOM_SIZE_2X2], floorsChecked);
        ok = false;
    }
    /* (o) WP4: la stanza di fusione deve trovare posto ALMENO una volta nel
       giro di prova, o il resto del controllo (o) sopra (unicita', mai
       adiacente al boss) non avrebbe mai esercitato nulla di davvero
       piazzato. */
    if (floorsWithFusion == 0)
    {
        fprintf(stderr, "GameRoomsTest: (o) la stanza di fusione non trova mai posto in %d piani generati\n", floorsChecked);
        ok = false;
    }
    /* (p) WP5: stessa idea di (o) sopra -- il resto del controllo (p)
       (unicita', mai adiacente al boss, mai prima del piano 3) presuppone
       che il giro di prova piazzi davvero la stanza a tempo almeno una
       volta, non solo che non si rompa quando manca. */
    if (floorsWithTimed == 0)
    {
        fprintf(stderr, "GameRoomsTest: (p) la stanza a tempo non trova mai posto in %d piani generati\n", floorsChecked);
        ok = false;
    }
    /* (q) WP6: stessa idea di (o)/(p) -- il resto del controllo (q) va
       esercitato davvero almeno una volta. */
    if (floorsWithArena == 0)
    {
        fprintf(stderr, "GameRoomsTest: (q) l'arena di sfida non trova mai posto in %d piani candidati\n", floorsEligibleForArena);
        ok = false;
    }
    /* (q) WP6: l'arena non e' MAI 1x1 (garanzia gia' verificata piano per
       piano sopra) e la preferenza per le taglie grandi deve essere viva --
       la 2x2, che il piazzamento prova per prima, deve comparire davvero. */
    if (arenaSizeSeen[ROOM_SIZE_1X1] > 0)
    {
        fprintf(stderr, "GameRoomsTest: (q) l'arena di sfida e' 1x1 in %d piani (mai ammesso)\n", arenaSizeSeen[ROOM_SIZE_1X1]);
        ok = false;
    }
    /* La preferenza per le taglie GRANDI deve essere viva, non solo dichiarata
       nel commento del piazzamento: una buona parte delle arene deve avere TRE
       o quattro celle (L o 2x2), non fermarsi sempre alle due. Soglia con
       margine (un quarto), non il valore misurato: la 2x2 in particolare resta
       rara perche' il vincolo di foglia la penalizza esattamente come penalizza
       la stanza boss (piu' perimetro = piu' occasioni di toccare due stanze). */
    int arenaBigSeen = arenaSizeSeen[ROOM_SIZE_2X2] + arenaSizeSeen[ROOM_SIZE_L];
    if (floorsWithArena > 0 && arenaBigSeen < floorsWithArena/4)
    {
        fprintf(stderr, "GameRoomsTest: (q) l'arena di sfida ha 3+ celle solo %d volte su %d (il piazzamento deve preferire le taglie grandi)\n",
                arenaBigSeen, floorsWithArena);
        ok = false;
    }

    /* (r) WP7: la Pourhouse deve trovare posto almeno una volta (o il resto
       del controllo (r) non avrebbe esercitato nulla) ma NON su ogni piano
       candidato -- "non ogni piano" e' il default proposto dichiarato, e
       questo controllo fallirebbe se qualcuno rendesse il tentativo
       incondizionato come tesoro/negozio. */
    if (floorsWithPourhouse == 0)
    {
        fprintf(stderr, "GameRoomsTest: (r) la Pourhouse non trova mai posto in %d piani candidati\n", floorsEligibleForPourhouse);
        ok = false;
    }
    /* La soglia e' UN TERZO dei piani candidati, non "meno di tutti": la
       griglia e' gia' satura quando tocca alla Pourhouse, quindi anche
       tentando il piazzamento a OGNI piano candidato ne uscirebbe circa il 44%
       (42 su 96, misurato) -- un controllo del tipo "< 100%" passerebbe lo
       stesso e non direbbe nulla. Con l'estrazione dichiarata (70%) il valore
       misurato e' 27 su 96, cioe' 28%: la soglia di 32 sta comodamente in
       mezzo ai due, e fallisce davvero se qualcuno rende il tentativo
       incondizionato. Se un giorno l'estrazione salisse per decisione di
       design, questo numero va aggiornato con lei -- ed e' voluto: e' un
       default proposto che deve restare visibile. */
    if (floorsWithPourhouse > floorsEligibleForPourhouse/3)
    {
        fprintf(stderr, "GameRoomsTest: (r) la Pourhouse compare in %d piani candidati su %d (soglia %d): dovrebbe essere un archetipo raro, non un servizio di ogni piano\n",
                floorsWithPourhouse, floorsEligibleForPourhouse, floorsEligibleForPourhouse/3);
        ok = false;
    }

    /* (s) WP8: entrambi i livelli devono trovare posto almeno una volta (o il
       resto del controllo (s) non avrebbe esercitato nulla) e la super-segreta
       deve restare PIU' RARA della normale -- e' letteralmente cio' che DEC-025
       chiede, e questo confronto fallisce se qualcuno rendesse i due
       piazzamenti simmetrici. */
    if (floorsWithSecret == 0)
    {
        fprintf(stderr, "GameRoomsTest: (s) la stanza segreta non trova mai posto in %d piani candidati\n", floorsEligibleForSecret);
        ok = false;
    }
    if (floorsWithSuperSecret == 0)
    {
        fprintf(stderr, "GameRoomsTest: (s) la super-segreta non trova mai posto in %d piani candidati\n", floorsEligibleForSuperSecret);
        ok = false;
    }
    if (floorsWithSuperSecret >= floorsWithSecret)
    {
        fprintf(stderr, "GameRoomsTest: (s) la super-segreta compare %d volte contro %d della segreta normale: deve restare il livello PIU' RARO (DEC-025)\n",
                floorsWithSuperSecret, floorsWithSecret);
        ok = false;
    }

    printf("  [rooms-s] stanze segrete (WP8, DEC-025): normale in %d piani su %d candidati, super-segreta in %d su %d (estrazione %d%%) -- sempre 1x1, sempre murate in generazione, sempre con una sola parete condivisa verso una stanza normale, mai sulla minimappa prima della breccia\n",
           floorsWithSecret, floorsEligibleForSecret, floorsWithSuperSecret, floorsEligibleForSuperSecret,
           WORLD_SECRET_SUPER_CHANCE_PERCENT);

    printf("  [rooms-r] Pourhouse (WP7) piazzata in %d piani su %d candidati (piani >= %d, estrazione %d%%): sempre 1x1, mai piu' di una per piano, mai adiacente a boss o arena\n",
           floorsWithPourhouse, floorsEligibleForPourhouse, WORLD_POURHOUSE_ROOM_MIN_FLOOR, WORLD_POURHOUSE_ROOM_CHANCE_PERCENT);

    printf("  [rooms-abcdefijlmno] %d piani x %d semi: minimo garantito, forme valide senza sovrapposizioni (1x1 %d, 1x2 %d, 2x1 %d, 2x2 %d, L %d), celle %d valori diversi, porte coerenti (una per coppia, DEC-181), boss foglia+connettivita' senza boss (DEC-182), connettivita', determinismo, transizioni -> %s\n",
           FLOOR_COUNT, kSeedCount, sizeSeen[ROOM_SIZE_1X1], sizeSeen[ROOM_SIZE_1X2], sizeSeen[ROOM_SIZE_2X1],
           sizeSeen[ROOM_SIZE_2X2], sizeSeen[ROOM_SIZE_L], seenCountsN, ok ? "ok" : "FALLITO");
    printf("  [rooms-f] stanza boss 2x2 in %d piani su %d (il resto ripiega su una classe piu' piccola quando la griglia e' satura o quando la 2x2 non troverebbe una sola stanza vicina, DEC-182)\n",
           bossSizeSeen[ROOM_SIZE_2X2], floorsChecked);
    printf("  [rooms-o] stanza di fusione (WP4) piazzata in %d piani su %d, mai adiacente al boss, mai piu' di una per piano\n",
           floorsWithFusion, floorsChecked);
    printf("  [rooms-p] stanza a tempo (WP5) piazzata in %d piani su %d candidati (piani >= %d), mai prima del piano 3, mai adiacente al boss, mai piu' di una per piano\n",
           floorsWithTimed, floorsEligibleForTimed, WORLD_TIMED_ROOM_MIN_FLOOR);
    printf("  [rooms-q] arena di sfida (WP6) piazzata in %d piani su %d candidati (piani >= %d): taglie 1x2 %d, 2x1 %d, 2x2 %d, L %d (mai 1x1), sempre foglia del grafo, mai adiacente al boss\n",
           floorsWithArena, floorsEligibleForArena, WORLD_ARENA_ROOM_MIN_FLOOR,
           arenaSizeSeen[ROOM_SIZE_1X2], arenaSizeSeen[ROOM_SIZE_2X1], arenaSizeSeen[ROOM_SIZE_2X2], arenaSizeSeen[ROOM_SIZE_L]);

    if (!RoomsTestMinSizeStillPlayable()) ok = false;
    if (!RoomsTestCameraClamp()) ok = false;
    if (!RoomsTestHoleIsSolid()) ok = false;
    if (!RoomsTestFusionInteraction()) ok = false;
    if (!RoomsTestTimedRoomInteraction()) ok = false;
    if (!RoomsTestArenaInteraction()) ok = false;
    if (!RoomsTestPourhouseInteraction()) ok = false;
    if (!RoomsTestSecretRooms()) ok = false;

    return ok;
}

/* DEC-170, SOLO manuale (mai in make test): gli scatti che --rooms-test non
   puo' fare. Cerca fra i seed un piano che contenga la taglia voluta, ci
   entra col giocatore in un punto scelto e disegna un frame vero: serve a
   guardare la telecamera (segue? sbatte contro il clamp? l'angolo mancante di
   una L sembra un muro?), non a verificare numeri. */
static bool RoomShapesShoot(Game *game, RoomSize wanted, int cellIndex, float ux, float uy, const char *path)
{
    static const unsigned int kSeeds[] = { 1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        game->rng = kSeeds[si];
        for (int floor = 1; floor <= FLOOR_COUNT; floor++)
        {
            WorldStartFloor(game, floor);
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!game->rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(game, x, y);
                    if (state != &game->rooms[y][x]) continue;
                    if (WorldRoomSizeFromCells(state->cells) != wanted) continue;
                    game->roomX = x;
                    game->roomY = y;
                    /* Il giocatore si piazza PRIMA di WorldSpawnRoomContents:
                       e' quella a fissare l'inquadratura d'ingresso
                       (WorldSnapCamera), esattamente come nel gioco vero.
                       Il punto si sceglie dentro una CELLA OCCUPATA, mai nel
                       riquadro grezzo: su una forma a L il riquadro comprende
                       l'angolo mancante, e senza un passo di simulazione a
                       respingerlo il giocatore resterebbe dentro il muro. */
                    int cellX[4], cellY[4];
                    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
                    int pick = (cellIndex < cellCount) ? cellIndex : (cellCount - 1);
                    Rectangle rect = WorldCellRect(game, cellX[pick], cellY[pick]);
                    game->player.pos = (Vector2){ rect.x + rect.width*ux, rect.y + rect.height*uy };
                    WorldSpawnRoomContents(game);
                    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
                    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, path);
                    bool valid = canvas.texture.id != 0;
                    UnloadRenderTexture(canvas);
                    printf("  [room-shot] taglia %d, seed %u piano %d cella (%d,%d) -> %s\n",
                           (int)wanted, kSeeds[si], floor, x, y, path);
                    return valid;
                }
            }
        }
    }
    fprintf(stderr, "GameRoomShapesScreenshotTest: nessun piano coi semi di prova contiene la taglia %d\n", (int)wanted);
    return false;
}

/* DEC-180, SOLO manuale (mai in make test): la forma a L col giocatore
   piazzato NON al centro cella ma vicino al confine con l'angolo mancante --
   il caso che il clamp per-cella di prima (DEC-170 default) non poteva mai
   mostrare, e che il clamp continuo sul riquadro 2x2 (DEC-180) invece lascia
   entrare in inquadratura. Trova la cella occupata piu' vicina al centro del
   buco (quella che ci confina, mai quella diagonale) e ci si piazza spostati
   verso il buco: e' lo scatto giusto per controllare a vista che il tileset
   (W8) vesta l'angolo da muro/sfondo, senza buchi neri non renderizzati. */
static bool RoomShapesShootLCorner(Game *game, const char *path)
{
    static const unsigned int kSeeds[] = { 1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 20260727u, 424242u };
    for (int si = 0; si < (int)(sizeof(kSeeds)/sizeof(kSeeds[0])); si++)
    {
        game->rng = kSeeds[si];
        for (int floor = 1; floor <= FLOOR_COUNT; floor++)
        {
            WorldStartFloor(game, floor);
            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    if (!game->rooms[y][x].exists) continue;
                    const RoomState *state = WorldRoomAt(game, x, y);
                    if (state != &game->rooms[y][x]) continue;
                    if (WorldRoomSizeFromCells(state->cells) != ROOM_SIZE_L) continue;
                    game->roomX = x;
                    game->roomY = y;

                    Rectangle hole = WorldRoomHoleRect(game, 0);
                    float holeCx = hole.x + hole.width*0.5f;
                    float holeCy = hole.y + hole.height*0.5f;

                    int cellX[4], cellY[4];
                    int cellCount = WorldRoomCells(game, cellX, cellY, 4);
                    int best = 0;
                    float bestDist = -1.0f;
                    for (int i = 0; i < cellCount; i++)
                    {
                        Rectangle r = WorldCellRect(game, cellX[i], cellY[i]);
                        float rcx = r.x + r.width*0.5f, rcy = r.y + r.height*0.5f;
                        float d = (rcx - holeCx)*(rcx - holeCx) + (rcy - holeCy)*(rcy - holeCy);
                        if (bestDist < 0.0f || d < bestDist) { bestDist = d; best = i; }
                    }
                    Rectangle rect = WorldCellRect(game, cellX[best], cellY[best]);
                    float rcx = rect.x + rect.width*0.5f, rcy = rect.y + rect.height*0.5f;
                    /* Dal centro cella verso il lato che confina col buco, ma
                       resta dentro la cella (mai sopra il buco: e' un
                       ostacolo solido). */
                    float ux = 0.5f + ((holeCx < rcx) ? -0.44f : (holeCx > rcx) ? 0.44f : 0.0f);
                    float uy = 0.5f + ((holeCy < rcy) ? -0.44f : (holeCy > rcy) ? 0.44f : 0.0f);
                    game->player.pos = (Vector2){ rect.x + rect.width*ux, rect.y + rect.height*uy };
                    WorldSpawnRoomContents(game);
                    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
                    RendererDrawApp(game, canvas, APP_GAMEPLAY, NULL, true, NULL, path);
                    bool valid = canvas.texture.id != 0;
                    UnloadRenderTexture(canvas);
                    printf("  [room-shot] taglia L (angolo, DEC-180): seed %u piano %d cella (%d,%d) -> %s\n",
                           kSeeds[si], floor, x, y, path);
                    return valid;
                }
            }
        }
    }
    fprintf(stderr, "GameRoomShapesScreenshotTest: nessun piano coi semi di prova contiene una forma a L (angolo)\n");
    return false;
}

bool GameRoomShapesScreenshotTest(Game *game)
{
    bool ok = true;
    /* 2x2: al centro della stanza (l'angolo comune alle quattro celle, dove la
       telecamera non tocca nessun bordo) e poi in fondo alla quarta cella,
       dove sbatte contro il clamp e si vedono due muri. */
    if (!RoomShapesShoot(game, ROOM_SIZE_2X2, 0, 0.98f, 0.98f, "logs/worldsmelt-room-2x2-centro.png")) ok = false;
    if (!RoomShapesShoot(game, ROOM_SIZE_2X2, 3, 0.92f, 0.92f, "logs/worldsmelt-room-2x2-angolo.png")) ok = false;
    if (!RoomShapesShoot(game, ROOM_SIZE_1X2, 0, 0.10f, 0.50f, "logs/worldsmelt-room-1x2.png")) ok = false;
    /* Forma a L (DEC-180, 30/07): la telecamera clampa al riquadro 2x2
       INTERO, in continuo, esattamente come le altre taglie maggiori -- non
       piu' un salto discreto fra due inquadrature di cella. I due scatti nelle
       due celle centrali mostrano lo scorrimento fra due posizioni diverse
       (non piu' un aggancio rigido alla cella corrente); il terzo scatto
       (RoomShapesShootLCorner) e' quello dedicato a verificare l'angolo
       mancante: il giocatore vicino al confine col buco, cosi' l'angolo entra
       davvero in inquadratura e si vede se il tileset lo veste bene. */
    if (!RoomShapesShoot(game, ROOM_SIZE_L, 0, 0.50f, 0.50f, "logs/worldsmelt-room-l-cella1.png")) ok = false;
    if (!RoomShapesShoot(game, ROOM_SIZE_L, 1, 0.50f, 0.50f, "logs/worldsmelt-room-l-cella2.png")) ok = false;
    if (!RoomShapesShootLCorner(game, "logs/worldsmelt-room-l-angolo.png")) ok = false;
    return ok;
}

/* ============================================================
   DEC-141: l'RNG di gameplay ('game->rng') derivato dal seed di run
   (GameResetRunWithSeed, src/game/game.c), non piu' dall'orologio. Prova
   che DAVVERO "stessa run + stesso seed => stessa sequenza di
   spawn/drop/combattimento": due reset con lo stesso seed producono
   nemici identici (tipo, posizione, hp) nella stessa stanza di
   combattimento dopo un passo di simulazione vera (GameUpdate, non solo
   WorldSpawnRoomContents), seed diversi ne producono di diversi.
   ============================================================ */

typedef struct RngSeedTestEnemy
{
    EnemyKind kind;
    Vector2 pos;
    float hp;
    float maxHp;
    float phase;
} RngSeedTestEnemy;

typedef struct RngSeedTestSnapshot
{
    unsigned int rng;
    Vector2 playerPos;
    int enemyCount;
    RngSeedTestEnemy enemies[MAX_ENEMIES];
} RngSeedTestSnapshot;

static RngSeedTestSnapshot RngSeedTestCapture(const Game *game)
{
    RngSeedTestSnapshot snap = { 0 };
    snap.rng = game->rng;
    snap.playerPos = game->player.pos;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active) continue;
        RngSeedTestEnemy *out = &snap.enemies[snap.enemyCount++];
        out->kind = game->enemies[i].kind;
        out->pos = game->enemies[i].pos;
        out->hp = game->enemies[i].hp;
        out->maxHp = game->enemies[i].maxHp;
        out->phase = game->enemies[i].phase;
    }
    return snap;
}

static bool RngSeedTestSnapshotsEqual(const RngSeedTestSnapshot *a, const RngSeedTestSnapshot *b)
{
    if (a->rng != b->rng) return false;
    if (a->playerPos.x != b->playerPos.x || a->playerPos.y != b->playerPos.y) return false;
    if (a->enemyCount != b->enemyCount) return false;
    for (int i = 0; i < a->enemyCount; i++)
    {
        const RngSeedTestEnemy *ea = &a->enemies[i];
        const RngSeedTestEnemy *eb = &b->enemies[i];
        if (ea->kind != eb->kind) return false;
        if (ea->pos.x != eb->pos.x || ea->pos.y != eb->pos.y) return false;
        if (ea->hp != eb->hp || ea->maxHp != eb->maxHp) return false;
        if (ea->phase != eb->phase) return false;
    }
    return true;
}

/* Rigioca l'inizio di una run col seed dato fino alla prima stanza di
   combattimento del piano 1 (la stanza START, kind fisso, non fa mai
   spawnare nulla: serve entrare in una stanza vera perche' game->rng
   avanzi davvero, stesso schema di GamePortalRespawnTest sopra per la
   stanza boss). Un passo di GameUpdate vero (non solo
   WorldSpawnRoomContents) fa avanzare ANCHE l'AI/il combattimento
   (entities.c/combat.c consumano game->rng per fasi/orbite/tiri), cosi'
   lo snapshot copre spawn E il primo pezzetto di comportamento, non solo
   la creazione iniziale. */
static bool RngSeedTestSpawn(Game *game, unsigned int seed, RngSeedTestSnapshot *out)
{
    GameResetRunWithSeed(game, seed);
    bool foundCombatRoom = false;
    for (int y = 0; y < GRID_SIZE && !foundCombatRoom; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (game->rooms[y][x].kind == ROOM_COMBAT)
            {
                game->roomX = x;
                game->roomY = y;
                foundCombatRoom = true;
                break;
            }
        }
    }
    if (!foundCombatRoom) return false;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
    *out = RngSeedTestCapture(game);
    return true;
}

bool GameRngSeedTest(Game *game)
{
    const unsigned int seedA = 20260727u;
    const unsigned int seedB = 90210u;

    RngSeedTestSnapshot firstA, secondA, firstB;
    if (!RngSeedTestSpawn(game, seedA, &firstA))
    {
        fprintf(stderr, "GameRngSeedTest: nessuna stanza di combattimento nel piano 1 col seed %u (mappa senza nemici, test non significativo)\n", seedA);
        return false;
    }
    if (!RngSeedTestSpawn(game, seedA, &secondA))
    {
        fprintf(stderr, "GameRngSeedTest: seconda generazione col seed %u fallita\n", seedA);
        return false;
    }
    if (!RngSeedTestSnapshotsEqual(&firstA, &secondA))
    {
        fprintf(stderr, "GameRngSeedTest: due reset con lo STESSO seed (%u) hanno prodotto sequenze diverse (game->rng %u vs %u, %d vs %d nemici)\n",
                seedA, firstA.rng, secondA.rng, firstA.enemyCount, secondA.enemyCount);
        return false;
    }
    if (firstA.enemyCount == 0)
    {
        fprintf(stderr, "GameRngSeedTest: la stanza di combattimento del seed %u non ha spawnato nemici (confronto vuoto, non significativo)\n", seedA);
        return false;
    }

    if (!RngSeedTestSpawn(game, seedB, &firstB))
    {
        fprintf(stderr, "GameRngSeedTest: nessuna stanza di combattimento nel piano 1 col seed %u (mappa senza nemici, test non significativo)\n", seedB);
        return false;
    }
    if (RngSeedTestSnapshotsEqual(&firstA, &firstB))
    {
        fprintf(stderr, "GameRngSeedTest: seed DIVERSI (%u vs %u) hanno prodotto la stessa sequenza (game->rng %u per entrambi)\n",
                seedA, seedB, firstA.rng);
        return false;
    }

    printf("  [rng-seed] stesso seed (%u) -> %d nemici identici (rng finale %u); seed diverso (%u) -> sequenza diversa (rng finale %u) -> ok\n",
           seedA, firstA.enemyCount, firstA.rng, seedB, firstB.rng);
    return true;
}

/* DEC-051 (ui/hud.md, "Timer di run sempre visibile"): il cronometro della run
   accumula SOLO durante PHASE_PLAY in una run vera ('inRealRun', WP1: NON nel
   Piano 0), si azzera con GameResetRunWithSeed, e non entra mai in alcuna
   decisione di gameplay ne' in alcuno stream RNG. */
bool GameRunTimerTest(Game *game)
{
    const unsigned int seed = 42u;
    const float dt = 0.016f;   /* ~60 Hz */

    /* Azzera il timer e verifica che parta da 0. */
    GameResetRunWithSeed(game, seed);
    if (game->runElapsedSeconds != 0.0f)
    {
        fprintf(stderr, "GameRunTimerTest: GameResetRunWithSeed non ha azzerato runElapsedSeconds (valore: %.3f)\n", game->runElapsedSeconds);
        return false;
    }

    /* Accumula timer durante PHASE_PLAY. */
    game->phase = PHASE_PLAY;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    if (game->runElapsedSeconds < dt * 0.99f)
    {
        fprintf(stderr, "GameRunTimerTest: il timer non accumula in PHASE_PLAY (atteso ~%.3f, ottenuto %.3f)\n", dt, game->runElapsedSeconds);
        return false;
    }
    float timerAfterOneStep = game->runElapsedSeconds;

    /* Un secondo step accumulato. */
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    if (game->runElapsedSeconds < timerAfterOneStep + dt * 0.99f)
    {
        fprintf(stderr, "GameRunTimerTest: il timer non accumula al secondo step (precedente %.3f, atteso ~%.3f, ottenuto %.3f)\n",
                timerAfterOneStep, timerAfterOneStep + dt, game->runElapsedSeconds);
        return false;
    }

    /* Timer NON accumula in PHASE_GAME_OVER. */
    float timerBeforeGameOver = game->runElapsedSeconds;
    game->phase = PHASE_GAME_OVER;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    if (game->runElapsedSeconds > timerBeforeGameOver)
    {
        fprintf(stderr, "GameRunTimerTest: il timer accumula in PHASE_GAME_OVER (prima: %.3f, dopo: %.3f)\n",
                timerBeforeGameOver, game->runElapsedSeconds);
        return false;
    }

    /* Timer NON accumula in PHASE_WIN. */
    GameResetRunWithSeed(game, seed);
    game->phase = PHASE_PLAY;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    float timerBeforeWin = game->runElapsedSeconds;
    game->phase = PHASE_WIN;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    if (game->runElapsedSeconds > timerBeforeWin)
    {
        fprintf(stderr, "GameRunTimerTest: il timer accumula in PHASE_WIN (prima: %.3f, dopo: %.3f)\n",
                timerBeforeWin, game->runElapsedSeconds);
        return false;
    }

    /* Reset rapido R azzera il timer. GameResetRunWithSeed dentro GameUpdate
       lo azzera, ma il dt dello stesso frame del reset viene comunque aggiunto
       (perche' il reset avviene in mezzo al GameUpdate, prima del check
       di PHASE_PLAY). Quindi dopo il reset, il timer contiene il dt di quel
       frame, che e' corretto (il nuovo gioco ha appena iniziato). */
    GameResetRunWithSeed(game, seed);
    game->phase = PHASE_PLAY;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    float timerBeforeReset = game->runElapsedSeconds;
    if (timerBeforeReset < dt * 0.99f)
    {
        fprintf(stderr, "GameRunTimerTest: il timer non accumula prima del reset (atteso ~%.3f, ottenuto %.3f)\n", dt, timerBeforeReset);
        return false;
    }
    game->resetQueued = true;
    GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    /* Dopo il reset, il timer contiene solo il dt dello stesso frame di reset
       (il nuovo gioco e' partito in questo frame). */
    if (game->runElapsedSeconds < dt * 0.99f || game->runElapsedSeconds > dt * 1.01f)
    {
        fprintf(stderr, "GameRunTimerTest: reset rapido non azzera correttamente runElapsedSeconds (atteso ~%.3f, ottenuto %.3f)\n", dt, game->runElapsedSeconds);
        return false;
    }

    /* WP1 (DEC-051): il Piano 0 (crogiolo) NON e' una run cronometrata.
       FloorZeroEnter mette anch'essa PHASE_PLAY per rendere l'hub giocabile
       (M1b, stesso cammino di GameFloorZeroTest sopra), ma spegne 'inRealRun'
       E riporta il cronometro a zero. Il tempo si accumula PRIMA di entrare
       nel crogiolo apposta: e' la seconda visita al Piano 0 (o la prima dopo
       un abbandono) il caso che rompe -- con un Game appena resettato le due
       verifiche sotto sarebbero vere per costruzione e non sorveglierebbero
       nulla. Cosi' invece falliscono sia se qualcuno toglie l'azzeramento in
       FloorZeroEnter (il tempo resterebbe congelato a quello della run
       precedente) sia se qualcuno torna a guardare solo 'phase' come prima
       del gate 'inRealRun' (il timer ripartirebbe nell'hub). */
    GameResetRunWithSeed(game, seed);
    for (int i = 0; i < 30; i++)
        GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    float timerBeforeFloorZero = game->runElapsedSeconds;
    if (timerBeforeFloorZero < dt*0.99f)
    {
        fprintf(stderr, "GameRunTimerTest: la run vera non ha accumulato tempo prima del Piano 0 (valore: %.3f) -- le verifiche sul crogiolo sarebbero vuote\n", timerBeforeFloorZero);
        return false;
    }
    FloorZeroEnter(game);
    if (game->runElapsedSeconds != 0.0f)
    {
        fprintf(stderr, "GameRunTimerTest: FloorZeroEnter non azzera runElapsedSeconds (accumulato prima: %.3f, rimasto: %.3f)\n",
                timerBeforeFloorZero, game->runElapsedSeconds);
        return false;
    }
    for (int i = 0; i < 30; i++)
        GameUpdate(game, dt, (Vector2){ 0, 0 }, false);
    if (game->runElapsedSeconds != 0.0f)
    {
        fprintf(stderr, "GameRunTimerTest: il timer accumula nel Piano 0 (valore dopo 30 step: %.3f)\n", game->runElapsedSeconds);
        return false;
    }

    printf("  [run-timer] accumulo in PHASE_PLAY (%.3f), blocco in GAME_OVER/WIN/Piano 0, azzeramento dopo reset: ok\n", timerBeforeReset);
    return true;
}

/* DEC-008 (Crust, WP2, systems/health-and-resources.md "Salute stratificata"):
   ordine di consumo (prima la temporanea, poi la base, nello STESSO evento),
   nessun overflow oltre PLAYER_TEMP_HP_CAP, la cura normale non tocca il
   Crust, e la morte resta legata solo alla salute base a zero -- perdere
   tutto il Crust in un colpo NON basta a finire la run se la base resta
   sopra zero. Copre anche i nuclei PURI dietro il contatore HUD
   (HudTempHeartsSlotCount/HudTempHeartsX/HudCrustLineFormat, caso (f) sotto,
   game_renderer.h) con tempHp>0, cosi' i due percorsi di disegno restano
   davvero verificati (known-issues.md #10.4). Come GameRunTimerTest, gira
   dopo InitWindow e usa 'game' per davvero (GameResetRunWithSeed chiama
   AssetsLoad) ma non disegna nulla. */
bool GameTempHealthTest(Game *game)
{
    const unsigned int seed = 7u;
    bool ok = true;

    /* (a) Il danno consuma PRIMA il Crust, e solo l'eccedenza va alla base,
       nello stesso evento (scenario 1 del documento). */
    GameResetRunWithSeed(game, seed);
    game->phase = PHASE_PLAY;
    game->player.maxHp = 6;
    game->player.hp = 6;
    game->player.tempHp = 4;
    game->player.invuln = 0.0f;
    CombatDamagePlayer(game, 2, "prova");
    if (game->player.tempHp != 2 || game->player.hp != 6)
    {
        fprintf(stderr, "GameTempHealthTest: danno 2 su Crust=4/hp=6 atteso Crust=2/hp=6, ottenuto Crust=%d/hp=%d\n",
                game->player.tempHp, game->player.hp);
        ok = false;
    }
    /* Un secondo colpo, oltre l'i-frame appena impostato, deve poter
       eccedere il Crust residuo e intaccare la base nello STESSO evento. */
    game->player.invuln = 0.0f;
    CombatDamagePlayer(game, 5, "prova");
    if (game->player.tempHp != 0 || game->player.hp != 3)
    {
        fprintf(stderr, "GameTempHealthTest: danno 5 su Crust=2/hp=6 atteso Crust=0/hp=3 (eccedenza 3), ottenuto Crust=%d/hp=%d\n",
                game->player.tempHp, game->player.hp);
        ok = false;
    }

    /* (b) Perdere Crust resta comunque "subire un colpo" (DEC-159): i-frame
       impostati anche quando il colpo e' assorbito INTERAMENTE dal Crust,
       la base resta intatta e la run non finisce. tempHp parte dal tetto
       PLAYER_TEMP_HP_CAP (4): uno stato davvero raggiungibile in gioco, non
       un valore oltre cap che CombatPickup non lascerebbe mai passare. */
    game->player.invuln = 0.0f;
    game->player.tempHp = PLAYER_TEMP_HP_CAP;
    game->player.hp = 1;
    game->player.maxHp = 6;
    CombatDamagePlayer(game, 3, "prova");
    if (game->player.tempHp != 1 || game->player.hp != 1 || game->player.invuln <= 0.0f || game->phase != PHASE_PLAY)
    {
        fprintf(stderr, "GameTempHealthTest: colpo interamente assorbito dal Crust (%d contro 3) atteso Crust=1/hp=1/invuln>0/PHASE_PLAY, ottenuto Crust=%d/hp=%d/invuln=%.2f/phase=%d\n",
                PLAYER_TEMP_HP_CAP, game->player.tempHp, game->player.hp, game->player.invuln, (int)game->phase);
        ok = false;
    }

    /* (c) La morte resta legata SOLO alla salute base a zero, mai al solo
       esaurimento del Crust (scenario 2 del documento + DEC-159). */
    game->player.invuln = 0.0f;
    game->player.tempHp = 0;
    game->player.hp = 1;
    CombatDamagePlayer(game, 1, "prova");
    if (game->player.hp != 0 || game->phase != PHASE_GAME_OVER)
    {
        fprintf(stderr, "GameTempHealthTest: hp=1/Crust=0 con danno 1 doveva finire la run (PHASE_GAME_OVER, hp=0), ottenuto hp=%d/phase=%d\n",
                game->player.hp, (int)game->phase);
        ok = false;
    }

    /* (d) Nessun overflow: raccogliere Crust oltre PLAYER_TEMP_HP_CAP clampa,
       non accumula all'infinito -- attraverso il percorso VERO (pickup di
       negozio -> CombatUpdatePickups), non solo il campo a mano. */
    GameResetRunWithSeed(game, seed);
    game->phase = PHASE_PLAY;
    game->player.tempHp = 0;
    Pickup *slot = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (!game->pickups[i].active) { slot = &game->pickups[i]; break; }
    if (!slot)
    {
        fprintf(stderr, "GameTempHealthTest: nessuno slot pickup libero per il test overflow\n");
        return false;
    }
    *slot = (Pickup){ 0 };
    slot->active = true;
    slot->kind = PICKUP_CRUST;
    slot->pos = game->player.pos;
    slot->radius = 20.0f;
    slot->value = PLAYER_TEMP_HP_CAP + 10;   /* ben oltre il tetto, di proposito */
    CombatUpdatePickups(game);
    if (game->player.tempHp != PLAYER_TEMP_HP_CAP)
    {
        fprintf(stderr, "GameTempHealthTest: raccogliere Crust=%d (oltre il tetto %d) atteso tempHp=%d, ottenuto %d\n",
                slot->value, PLAYER_TEMP_HP_CAP, PLAYER_TEMP_HP_CAP, game->player.tempHp);
        ok = false;
    }

    /* (e) La cura normale (PICKUP_HEART) non ricarica MAI il Crust. */
    game->player.tempHp = 3;
    game->player.maxHp = 6;
    game->player.hp = 2;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (!game->pickups[i].active) { slot = &game->pickups[i]; break; }
    *slot = (Pickup){ 0 };
    slot->active = true;
    slot->kind = PICKUP_HEART;
    slot->pos = game->player.pos;
    slot->radius = 20.0f;
    slot->value = 4;
    CombatUpdatePickups(game);
    if (game->player.tempHp != 3 || game->player.hp != 6)
    {
        fprintf(stderr, "GameTempHealthTest: la cura normale ha toccato il Crust o non ha curato la base come atteso (Crust atteso 3, hp atteso 6), ottenuto Crust=%d/hp=%d\n",
                game->player.tempHp, game->player.hp);
        ok = false;
    }

    /* (f) I nuclei PURI dietro il contatore HUD (game_renderer.h): finche'
       nessun test li chiamava mai con tempHp>0, DrawHudV3TempHearts (layout
       V3) e il ramo "+N" di DrawHudVitals (ripiego senza pacchetto
       artistico) non venivano mai esercitati, pur essendo la parte
       osservabile dal giocatore di questo work package (known-issues.md
       #10.4). Sul tempHp=3/maxHp=6 gia' in gioco da (e) sopra, piu' il caso
       limite tempHp=0 (nessuna icona, nessun testo). */
    if (HudTempHeartsSlotCount(game->player.tempHp) != 3)
    {
        fprintf(stderr, "GameTempHealthTest: HudTempHeartsSlotCount(tempHp=3) atteso 3, ottenuto %d\n",
                HudTempHeartsSlotCount(game->player.tempHp));
        ok = false;
    }
    int tempHeartsX = HudTempHeartsX(game->player.maxHp);
    if (tempHeartsX != 55)   /* HUD_V3_MARGIN 10 + baseHeartSlots(maxHp=6)=3 * HUD_V3_HEART_STEP 13 + HUD_V3_TEMP_HEARTS_GAP 6 */
    {
        fprintf(stderr, "GameTempHealthTest: HudTempHeartsX(maxHp=6) atteso 55, ottenuto %d\n", tempHeartsX);
        ok = false;
    }
    char crustHudLine[24];
    bool showCrustHud = HudCrustLineFormat(game->player.tempHp, crustHudLine, sizeof(crustHudLine));
    if (!showCrustHud || strcmp(crustHudLine, "+3") != 0)
    {
        fprintf(stderr, "GameTempHealthTest: HudCrustLineFormat(tempHp=3) atteso mostra=vero testo=\"+3\", ottenuto mostra=%d testo=\"%s\"\n",
                showCrustHud, crustHudLine);
        ok = false;
    }
    if (HudTempHeartsSlotCount(0) != 0)
    {
        fprintf(stderr, "GameTempHealthTest: HudTempHeartsSlotCount(tempHp=0) atteso 0, ottenuto %d\n", HudTempHeartsSlotCount(0));
        ok = false;
    }
    char emptyCrustLine[24] = "sentinella";
    bool showEmptyCrustHud = HudCrustLineFormat(0, emptyCrustLine, sizeof(emptyCrustLine));
    if (showEmptyCrustHud || emptyCrustLine[0] != '\0')
    {
        fprintf(stderr, "GameTempHealthTest: HudCrustLineFormat(tempHp=0) atteso mostra=falso testo vuoto, ottenuto mostra=%d testo=\"%s\"\n",
                showEmptyCrustHud, emptyCrustLine);
        ok = false;
    }

    if (ok) printf("  [temp-health] Crust (DEC-008): consumo prima della base con eccedenza nello stesso evento, colpo assorbito senza toccare la base, morte solo a base 0, cap %d senza overflow, cura normale non ricarica il Crust, nuclei HUD (icone/X/testo +N) corretti con tempHp>0: ok\n", PLAYER_TEMP_HP_CAP);
    else fprintf(stderr, "GameTempHealthTest: FALLITO -- vedi i messaggi sopra\n");
    return ok;
}

/* ============================================================
   WP3 (docs/design/systems/secrets-and-obstacles.md, "Ostacoli generati a
   tema" + DEC-043): tre famiglie di ostacolo (solido/distruttibile/pericolo,
   room_layout.h), persistenza dei distruttibili spaccati per tutto il piano
   (Game.destroyedObstacleMask, INFRASTRUTTURA -- vedi il commento sul test
   (a) sotto e docs/engineering/known-issues.md voce 11), danno da contatto
   telegrafato dei pericoli (CombatResolveHazards, chiamata attraverso il
   frame VERO -- CombatUpdatePlayer, mai isolata), croce centrale libera
   indipendente dalla famiglia, budget nemici ridotto dagli ostacoli della
   stanza (WorldSpawnCombatRoom). Come GameRoomsTest, gira su Game LOCALI
   (RoomsTestGenerateFloor, definita sopra): non serve la finestra per
   davvero (girando comunque DOPO InitWindow, vedi app.c -- CombatUpdatePlayer
   legge IsKeyDown/IsMouseButtonDown, che senza finestra darebbero risultati
   indefiniti), il parametro 'game' rispetta solo la convenzione di firma di
   AppRun. */
bool GameObstaclesTest(Game *game)
{
    (void)game;
    bool ok = true;
    const unsigned int seed = 4200u;
    const float crossHalf = 70.0f;   /* stessa fascia di prova di RoomsTestMinSizeStillPlayable: piu' stretta della vera ROOM_CROSS_HALF (90px) */

    /* (a) La bomba (lo strumento di breccia, CombatExplodeAt con breach=true)
       rimuove il distruttibile nel raggio, e il BIT di persistenza
       (Game.destroyedObstacleMask) resta segnato quando WorldSpawnRoomContents
       ricostruisce gli ostacoli della stessa cella. Questo e' un test di
       INFRASTRUTTURA (chiama WorldSpawnRoomContents due volte a mano sullo
       stesso Game, come farebbe una transizione vera): nel gioco vero, oggi,
       una stanza di combattimento perde comunque TUTTI i suoi ostacoli non
       appena si ripulisce (WorldBuildObstacles esce subito se
       'room->cleared', comportamento preesistente a WP3 e non toccato qui) e
       la porta resta bloccata finche' non si ripulisce (GameRoomIsLocked):
       non esiste quindi ancora, in gioco, una sequenza "esco e rientro in una
       stanza di combattimento ancora aperta" in cui osservare la persistenza
       -- il meccanismo serve alle stanze segrete di un lavoro successivo, che
       potranno rientrare piu' volte prima di essere "ripulite" in quel senso
       (vedi docs/design/systems/secrets-and-obstacles.md, "Persistenza dei
       distruttibili", e docs/engineering/known-issues.md voce 11). Insieme,
       in questo stesso blocco, (c): la croce centrale resta libera da
       QUALUNQUE ostacolo, di QUALUNQUE famiglia. */
    {
        bool found = false;
        for (int floor = 1; floor <= FLOOR_COUNT && !found; floor++)
        {
            Game g;
            RoomsTestGenerateFloor(seed, floor, &g);
            /* La stanza di partenza e' sempre 1x1 (WorldGenerateFloorMap): il
               modo piu' semplice di ottenere ostacoli VERI (WorldBuildObstacles,
               famiglie comprese) e' forzarla a stanza di combattimento non
               ripulita, con un layout SCATTER a densita' massima (il maggior
               numero possibile di blocchi -- ROOM_LAYOUT_MAX_PER_CELL -- alza le
               probabilita' di trovare almeno un distruttibile). */
            g.content.floors[floor - 1].roomLayout.active = true;
            g.content.floors[floor - 1].roomLayout.form = ROOM_LAYOUT_SCATTER;
            g.content.floors[floor - 1].roomLayout.density = ROOM_LAYOUT_DENSITY_MAX;
            RoomState *r = WorldCurrentRoomMutable(&g);
            r->kind = ROOM_COMBAT;
            r->cleared = false;
            WorldSpawnRoomContents(&g);   /* costruisce ostacoli VERI (WorldBuildObstacles) */

            /* (c) croce centrale libera, qualunque famiglia -- stessa logica
               di rifiuto di BlockInQuadrant (room_layout.c): un ostacolo che
               intersechi la fascia verticale O quella orizzontale del centro
               non e' ammesso. */
            Rectangle roomRect = WorldCurrentRoomRect(&g);
            float ccx = roomRect.x + roomRect.width*0.5f, ccy = roomRect.y + roomRect.height*0.5f;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
            {
                Obstacle *o = &g.obstacles[i];
                bool hitsVertical = (o->x < ccx + crossHalf) && (o->x + o->w > ccx - crossHalf);
                bool hitsHorizontal = (o->y < ccy + crossHalf) && (o->y + o->h > ccy - crossHalf);
                if (hitsVertical || hitsHorizontal)
                {
                    fprintf(stderr, "GameObstaclesTest: (c) piano %d: un ostacolo (famiglia %d) invade la croce centrale (%.1f,%.1f %.1fx%.1f)\n",
                            floor, (int)o->family, o->x, o->y, o->w, o->h);
                    ok = false;
                }
            }

            int destroyIdx = -1;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
                if (g.obstacles[i].family == OBSTACLE_DESTRUCTIBLE) { destroyIdx = i; break; }
            if (destroyIdx < 0) continue;   /* questo piano non ne ha piazzato uno: si riprova al prossimo */
            found = true;

            Obstacle target = g.obstacles[destroyIdx];
            int cellX = g.obstacleCellX[destroyIdx], cellY = g.obstacleCellY[destroyIdx], local = g.obstacleLocalIndex[destroyIdx];
            Vector2 center = { target.x + target.w*0.5f, target.y + target.h*0.5f };

            CombatExplodeAt(&g, center, fmaxf(target.w, target.h)*0.5f + 4.0f, 40.0f, true);   /* la bomba: sbreccia */

            bool stillThereAfterBlast = false;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
                if (g.obstacleCellX[i] == cellX && g.obstacleCellY[i] == cellY && g.obstacleLocalIndex[i] == local)
                    stillThereAfterBlast = true;
            if (stillThereAfterBlast)
            {
                fprintf(stderr, "GameObstaclesTest: (a) piano %d: CombatExplodeAt non ha rimosso il distruttibile (cella %d,%d indice %d)\n",
                        floor, cellX, cellY, local);
                ok = false;
            }
            if (cellX < 0 || cellY < 0 || !(g.destroyedObstacleMask[cellY][cellX] & (unsigned short)(1u << local)))
            {
                fprintf(stderr, "GameObstaclesTest: (a) piano %d: il bit di persistenza non e' stato segnato per cella (%d,%d) indice %d\n",
                        floor, cellX, cellY, local);
                ok = false;
            }

            /* Uscita e rientro nella STESSA stanza: WorldSpawnRoomContents
               gira di nuovo, esattamente come ad ogni ingresso vero. */
            WorldSpawnRoomContents(&g);
            bool reappeared = false;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
                if (g.obstacleCellX[i] == cellX && g.obstacleCellY[i] == cellY && g.obstacleLocalIndex[i] == local)
                    reappeared = true;
            if (reappeared)
            {
                fprintf(stderr, "GameObstaclesTest: (a) piano %d: il distruttibile spaccato e' ricomparso rientrando nella stanza (cella %d,%d indice %d)\n",
                        floor, cellX, cellY, local);
                ok = false;
            }
        }
        if (!found)
        {
            fprintf(stderr, "GameObstaclesTest: (a)/(c) nessun ostacolo DISTRUTTIBILE trovato in %d piani a densita' massima: verifica (a) non eseguita\n", FLOOR_COUNT);
            ok = false;
        }
    }

    /* (b) Il pericolo passivo (OBSTACLE_HAZARD) e' gia' presente -- quindi
       gia' disegnato con un segnale distinto, DrawObstacles in
       game_renderer.c -- nell'istante in cui la stanza e' pronta, PRIMA che
       il giocatore possa toccarlo: nessuna finestra in cui colpisca "a
       sorpresa" (telegraph). Danneggia al contatto dentro gli i-frames
       esistenti, e i nemici lo ignorano. */
    {
        bool found = false;
        for (int floor = 1; floor <= FLOOR_COUNT && !found; floor++)
        {
            Game g;
            RoomsTestGenerateFloor(seed + 1u, floor, &g);
            g.content.floors[floor - 1].roomLayout.active = true;
            g.content.floors[floor - 1].roomLayout.form = ROOM_LAYOUT_SCATTER;
            g.content.floors[floor - 1].roomLayout.density = ROOM_LAYOUT_DENSITY_MAX;
            RoomState *r = WorldCurrentRoomMutable(&g);
            r->kind = ROOM_COMBAT;
            r->cleared = false;
            g.phase = PHASE_PLAY;
            WorldSpawnRoomContents(&g);

            int hazardIdx = -1;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
                if (g.obstacles[i].family == OBSTACLE_HAZARD) { hazardIdx = i; break; }
            if (hazardIdx < 0) continue;
            found = true;
            Obstacle hz = g.obstacles[hazardIdx];   /* esiste (ed e' quindi gia' telegrafato) da PRIMA di ogni contatto */

            /* WP-SPIKE (DEC-198): il danno di contatto e' ora gated dalla
               fase del ciclo (WorldHazardSpikesExtendedAt, src/world/world.c) --
               questo blocco (b) verifica "telegrafato che danneggia" nella
               fase ESTESA (la fase in cui il tag visivo promette danno). Cerca
               il primo istante ESTESO avanzando con dt espliciti (1/60s, mai
               un salto a un tempo comodo) dal tempo di run zero; la fase
               RETRATTA (nessun danno) ha il proprio blocco dedicato in (e). */
            {
                int hazCellX = g.obstacleCellX[hazardIdx], hazCellY = g.obstacleCellY[hazardIdx];
                int hazLocal = g.obstacleLocalIndex[hazardIdx];
                float probeDt = 1.0f/60.0f;
                g.runElapsedSeconds = 0.0f;
                int probeSteps = 0;
                int probeMax = (int)(WORLD_HAZARD_PERIOD_SECONDS/probeDt) + 4;
                while (!WorldHazardSpikesExtendedAt(hazCellX, hazCellY, hazLocal, g.runElapsedSeconds) && probeSteps < probeMax)
                {
                    g.runElapsedSeconds += probeDt;
                    probeSteps++;
                }
                if (!WorldHazardSpikesExtendedAt(hazCellX, hazCellY, hazLocal, g.runElapsedSeconds))
                {
                    fprintf(stderr, "GameObstaclesTest: (b) piano %d: nessun istante ESTESO trovato in un periodo pieno per cella (%d,%d)\n",
                            floor, hazCellX, hazCellY);
                    ok = false;
                    continue;
                }
            }

            Rectangle roomRectB = WorldCurrentRoomRect(&g);

            int startHp = 6;
            g.player.maxHp = startHp;
            g.player.hp = startHp;
            g.player.tempHp = 0;
            g.player.invuln = 0.0f;
            g.player.radius = 14.0f;
            g.player.pos = (Vector2){ hz.x + hz.w*0.5f, hz.y + hz.h*0.5f };

            /* Il contatto vero passa dal FRAME intero (CombatUpdatePlayer),
               non da CombatResolveHazards isolata: e' l'unico modo per cui
               togliere lo skip dei pericoli da CombatResolveObstacles (che
               tornerebbe a bloccare il movimento invece di danneggiare) o
               togliere la chiamata a CombatResolveHazards da
               CombatUpdatePlayer (che spegnerebbe la feature per l'intero
               gioco) facciano fallire QUESTO test. Nessun tasto/mouse premuto
               (move e mira restano zero): il giocatore resta fermo sul
               pericolo, come se ci fosse nato sopra. */
            CombatUpdatePlayer(&g, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
            if (g.player.hp >= startHp || g.player.invuln <= 0.0f)
            {
                fprintf(stderr, "GameObstaclesTest: (b) piano %d: il contatto col pericolo non ha fatto danno/aperto gli i-frames (hp %d->%d, invuln=%.2f)\n",
                        floor, startHp, g.player.hp, g.player.invuln);
                ok = false;
            }
            int hpAfterFirst = g.player.hp;

            /* Dentro gli STESSI i-frames: un secondo contatto immediato non
               deve fare danno una seconda volta (CombatDamagePlayer li
               rispetta gia', qui si verifica solo che il frame intero non li
               scavalchi). */
            CombatUpdatePlayer(&g, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
            if (g.player.hp != hpAfterFirst)
            {
                fprintf(stderr, "GameObstaclesTest: (b) piano %d: un secondo contatto dentro gli i-frames ha fatto danno di nuovo (hp %d->%d)\n",
                        floor, hpAfterFirst, g.player.hp);
                ok = false;
            }

            /* I nemici lo ignorano (default proposto, governance/open-questions.md
               voce 29): un nemico piazzato ESATTAMENTE al centro del pericolo,
               con il giocatore alla STESSA posizione (dir verso il giocatore
               nulla -> nessun movimento d'IA), non deve MUOVERSI affatto:
               ne' spinto fuori (se CombatResolveObstacles trattasse il
               pericolo come un solido, lo spingerebbe fuori dal rettangolo),
               ne' altrimenti alterato. Confrontare la POSIZIONE (non l'hp,
               che nessuna variante di questo codice puo' toccare per un
               nemico: CombatResolveHazards agisce solo sul giocatore) e'
               cio' che rende il controllo capace di fallire se lo skip dei
               pericoli sparisse da CombatResolveObstacles. */
            EntitiesClear(&g);
            Vector2 hazardCenter = { hz.x + hz.w*0.5f, hz.y + hz.h*0.5f };
            g.player.pos = hazardCenter;   /* dir verso il giocatore = 0: l'IA del nemico non lo sposta */
            EntitiesAddEnemy(&g, ENEMY_CHASER, hazardCenter);
            int enemyIdx = -1;
            Vector2 enemyPosBefore = { 0.0f, 0.0f };
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (g.enemies[i].active) { enemyIdx = i; enemyPosBefore = g.enemies[i].pos; break; }
            if (enemyIdx < 0)
            {
                fprintf(stderr, "GameObstaclesTest: (b) piano %d: nessuno slot nemico libero per il controllo\n", floor);
                ok = false;
            }
            else
            {
                CombatUpdateEnemies(&g, 1.0f/60.0f);
                Vector2 enemyPosAfter = g.enemies[enemyIdx].pos;
                float moved2 = GameMathLengthSquared(GameMathSubtract(enemyPosAfter, enemyPosBefore));
                if (moved2 > 0.001f)
                {
                    fprintf(stderr, "GameObstaclesTest: (b) piano %d: un nemico sul pericolo si e' spostato (atteso: fermo, lo ignora), (%.2f,%.2f) -> (%.2f,%.2f)\n",
                            floor, enemyPosBefore.x, enemyPosBefore.y, enemyPosAfter.x, enemyPosAfter.y);
                    ok = false;
                }
            }

            /* Un colpo ATTRAVERSA un pericolo (non rimbalza, non sparisce):
               parte appena fuori dal lato del pericolo rivolto verso il
               BORDO della stanza e attraversa verso il CENTRO (la croce e'
               sempre libera oltre, mai un altro ostacolo sulla rotta), cosi'
               lo spazio necessario c'e' sempre, qualunque sia la posizione
               del pericolo nel suo quadrante. */
            EntitiesClear(&g);
            float centerX = roomRectB.x + roomRectB.width*0.5f;
            bool hazardLeftOfCenter = (hz.x + hz.w*0.5f) < centerX;
            float shotRadius = 4.0f;
            float buffer = shotRadius + 4.0f;
            float travel = hz.w + buffer*2.0f + 12.0f;
            Vector2 shotStart = hazardLeftOfCenter
                ? (Vector2){ hz.x - buffer, hz.y + hz.h*0.5f }
                : (Vector2){ hz.x + hz.w + buffer, hz.y + hz.h*0.5f };
            Vector2 shotDir = hazardLeftOfCenter ? (Vector2){ 1.0f, 0.0f } : (Vector2){ -1.0f, 0.0f };
            float crossSpeed = travel*60.0f;   /* copre 'travel' in un solo passo a 1/60s */
            float expectedPastEdge = hazardLeftOfCenter ? (hz.x + hz.w) : hz.x;
            Shot *hazardShot = EntitiesAddShot(&g, true, shotStart, shotDir, crossSpeed, 10.0f, shotRadius, 0, WHITE);
            if (!hazardShot)
            {
                fprintf(stderr, "GameObstaclesTest: (b) piano %d: impossibile creare il colpo di prova per l'attraversamento\n", floor);
                ok = false;
            }
            else
            {
                CombatUpdateShots(&g, 1.0f/60.0f);
                bool crossed = hazardLeftOfCenter ? (hazardShot->pos.x > expectedPastEdge) : (hazardShot->pos.x < expectedPastEdge);
                if (!hazardShot->active || !crossed)
                {
                    fprintf(stderr, "GameObstaclesTest: (b) piano %d: un colpo non ha attraversato il pericolo (active=%d, pos.x=%.1f, atteso %s %.1f)\n",
                            floor, hazardShot->active, hazardShot->pos.x, hazardLeftOfCenter ? "oltre" : "sotto", expectedPastEdge);
                    ok = false;
                }
            }
        }
        if (!found)
        {
            fprintf(stderr, "GameObstaclesTest: (b) nessun ostacolo PERICOLO trovato in %d piani a densita' massima: verifica non eseguita\n", FLOOR_COUNT);
            ok = false;
        }
    }

    /* (d) DEC-043: il budget nemici condiviso si riduce con gli ostacoli
       della stanza, mai sotto la soglia minima che garantisce almeno un
       nemico (secrets-and-obstacles.md, "Casi limite"). Stesso stato di
       partenza (rng SALVATO e ripristinato) per isolare l'unica differenza
       che conta: quanti ostacoli ha la stanza. */
    {
        Game g;
        RoomsTestGenerateFloor(seed + 2u, 1, &g);
        unsigned int savedRng = g.rng;

        EntitiesClear(&g);
        g.obstacleCount = 0;
        g.obstacleHoleCount = 0;
        g.rng = savedRng;
        WorldSpawnCombatRoom(&g);
        int spawnedBaseline = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) if (g.enemies[i].active) spawnedBaseline++;

        EntitiesClear(&g);
        for (int i = 0; i < 30; i++) g.obstacles[i] = (Obstacle){ -1000.0f, -1000.0f, 10.0f, 10.0f, OBSTACLE_SOLID };
        g.obstacleCount = 30;   /* ben oltre quanto un budget di poche unita' possa assorbire */
        g.obstacleHoleCount = 0;
        g.rng = savedRng;
        WorldSpawnCombatRoom(&g);
        int spawnedWithObstacles = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) if (g.enemies[i].active) spawnedWithObstacles++;

        if (spawnedBaseline <= 1)
        {
            fprintf(stderr, "GameObstaclesTest: (d) baseline sospetta (spawnedBaseline=%d, atteso >1 per un confronto significativo)\n", spawnedBaseline);
            ok = false;
        }
        if (spawnedWithObstacles != 1)
        {
            fprintf(stderr, "GameObstaclesTest: (d) con 30 ostacoli il budget nemici doveva ridursi al minimo garantito (1 nemico), ottenuto %d\n", spawnedWithObstacles);
            ok = false;
        }
        if (spawnedWithObstacles >= spawnedBaseline)
        {
            fprintf(stderr, "GameObstaclesTest: (d) gli ostacoli non hanno ridotto il budget nemici (baseline=%d, con ostacoli=%d)\n",
                    spawnedBaseline, spawnedWithObstacles);
            ok = false;
        }
    }

    /* (e) WP-SPIKE (DEC-198): gli spuntoni (OBSTACLE_HAZARD) diventano
       TEMPORIZZATI -- il danno di contatto segue ESATTAMENTE
       WorldHazardSpikesExtendedAt (src/world/world.c), il predicato puro
       condiviso con il renderer (DrawObstacleFamilyProp/DrawObstacleFamilyOverlay
       in src/render/game_renderer.c, mai un secondo calcolo indipendente).
       Guida sempre il tempo con dt ESPLICITI (1/60s, mai GetTime() ne' un
       contatore indipendente) attraverso Game.runElapsedSeconds, lo stesso
       campo che CombatResolveHazards legge in gioco vero. */
    {
        bool found = false;
        for (int floor = 1; floor <= FLOOR_COUNT && !found; floor++)
        {
            Game g;
            RoomsTestGenerateFloor(seed + 3u, floor, &g);
            g.content.floors[floor - 1].roomLayout.active = true;
            g.content.floors[floor - 1].roomLayout.form = ROOM_LAYOUT_SCATTER;
            g.content.floors[floor - 1].roomLayout.density = ROOM_LAYOUT_DENSITY_MAX;
            RoomState *r = WorldCurrentRoomMutable(&g);
            r->kind = ROOM_COMBAT;
            r->cleared = false;
            g.phase = PHASE_PLAY;
            g.inRealRun = true;
            WorldSpawnRoomContents(&g);

            int hazardIdx = -1;
            for (int i = g.obstacleHoleCount; i < g.obstacleCount; i++)
                if (g.obstacles[i].family == OBSTACLE_HAZARD) { hazardIdx = i; break; }
            if (hazardIdx < 0) continue;
            found = true;

            Obstacle hz = g.obstacles[hazardIdx];
            Vector2 hazardCenter = { hz.x + hz.w*0.5f, hz.y + hz.h*0.5f };
            int cellX = g.obstacleCellX[hazardIdx], cellY = g.obstacleCellY[hazardIdx];
            int localIndex = g.obstacleLocalIndex[hazardIdx];
            const float dt = 1.0f/60.0f;
            const int startHp = 6;

            /* (e1) DETERMINISMO: non la stessa chiamata confrontata con se'
               stessa (varrebbe per QUALUNQUE funzione pura, hash costante
               incluso) -- rigenera lo STESSO piano da una Game indipendente
               (stesso seme, stessa forma/densita' forzate) e verifica che il
               pericolo nello stesso slot riceva la stessa terna cella/indice
               locale, poi che il predicato concordi su un periodo intero
               campionato fra le due terne ottenute dalle due generazioni: il
               ciclo non si sposta rientrando nella stanza (uscire e rientrare
               rigenera RoomState ma non il piano). */
            {
                Game g2;
                RoomsTestGenerateFloor(seed + 3u, floor, &g2);
                g2.content.floors[floor - 1].roomLayout.active = true;
                g2.content.floors[floor - 1].roomLayout.form = ROOM_LAYOUT_SCATTER;
                g2.content.floors[floor - 1].roomLayout.density = ROOM_LAYOUT_DENSITY_MAX;
                RoomState *r2 = WorldCurrentRoomMutable(&g2);
                r2->kind = ROOM_COMBAT;
                r2->cleared = false;
                g2.phase = PHASE_PLAY;
                g2.inRealRun = true;
                WorldSpawnRoomContents(&g2);

                if (hazardIdx >= g2.obstacleCount || g2.obstacles[hazardIdx].family != OBSTACLE_HAZARD ||
                    g2.obstacleCellX[hazardIdx] != cellX || g2.obstacleCellY[hazardIdx] != cellY ||
                    g2.obstacleLocalIndex[hazardIdx] != localIndex)
                {
                    fprintf(stderr, "GameObstaclesTest: (e1) piano %d: due generazioni indipendenti dello stesso seme/piano non hanno prodotto lo stesso pericolo nello slot %d\n",
                            floor, hazardIdx);
                    ok = false;
                }
                else
                {
                    for (float t = 0.0f; t < WORLD_HAZARD_PERIOD_SECONDS; t += 0.31f)
                    {
                        if (WorldHazardSpikesExtendedAt(cellX, cellY, localIndex, t) !=
                            WorldHazardSpikesExtendedAt(g2.obstacleCellX[hazardIdx], g2.obstacleCellY[hazardIdx], g2.obstacleLocalIndex[hazardIdx], t))
                        {
                            fprintf(stderr, "GameObstaclesTest: (e1) piano %d: t=%.2f la fase differisce fra due generazioni indipendenti dello stesso piano/seme per cella (%d,%d) indice %d\n",
                                    floor, t, cellX, cellY, localIndex);
                            ok = false;
                            break;
                        }
                    }
                }
            }

            /* (e2) NUCLEO: il predicato puro e' la SOLA fonte del danno --
               campiona piu' istanti nell'arco di un periodo pieno, resetta il
               giocatore a ogni campione, e verifica che il danno di contatto
               segua ESATTAMENTE cio' che WorldHazardSpikesExtendedAt
               restituisce per quello stesso (cellX,cellY,t): mai un contatto
               che danneggia mentre il predicato dice "retratti", mai un
               contatto senza danno mentre il predicato dice "estesi" -- il
               test che il giudice puo' mutare per far fallire una qualunque
               finestra fasulla. */
            bool sawExtendedSample = false, sawRetractedSample = false;
            for (float t = 0.0f; t < WORLD_HAZARD_PERIOD_SECONDS; t += 0.2f)
            {
                bool expectExtended = WorldHazardSpikesExtendedAt(cellX, cellY, localIndex, t);
                if (expectExtended) sawExtendedSample = true; else sawRetractedSample = true;

                g.player.maxHp = startHp; g.player.hp = startHp; g.player.tempHp = 0;
                g.player.invuln = 0.0f; g.player.radius = 14.0f; g.player.pos = hazardCenter;
                g.runElapsedSeconds = t;
                CombatUpdatePlayer(&g, dt, (Vector2){ 0.0f, 0.0f }, false);
                bool damaged = (g.player.hp < startHp) && (g.player.invuln > 0.0f);

                if (expectExtended && !damaged)
                {
                    fprintf(stderr, "GameObstaclesTest: (e2) piano %d: t=%.2f ESTESO secondo il predicato ma il contatto non ha danneggiato\n",
                            floor, t);
                    ok = false;
                }
                if (!expectExtended && damaged)
                {
                    fprintf(stderr, "GameObstaclesTest: (e2) piano %d: t=%.2f RETRATTO secondo il predicato ma il contatto ha danneggiato (finestra fasulla)\n",
                            floor, t);
                    ok = false;
                }
            }
            if (!sawExtendedSample || !sawRetractedSample)
            {
                fprintf(stderr, "GameObstaclesTest: (e2) piano %d: il campionamento non ha attraversato entrambe le fasi (estesa=%d, retratta=%d) -- periodo/costanti sospetti\n",
                        floor, sawExtendedSample, sawRetractedSample);
                ok = false;
            }

            /* (e3) TRANSIZIONE guidata con dt ESPLICITI (mai un salto diretto
               a un tempo comodo): parte da un istante RETRATTO noto e avanza
               un frame alla volta finche' il predicato non passa a ESTESO,
               verificando ad OGNI frame che il danno segua la fase di
               quell'istante -- e non un frame prima o dopo. */
            g.player.maxHp = startHp; g.player.hp = startHp; g.player.tempHp = 0;
            g.player.invuln = 0.0f; g.player.radius = 14.0f; g.player.pos = hazardCenter;
            g.runElapsedSeconds = 0.0f;
            /* Riparte sempre da un istante RETRATTO: se la cella e' ESTESA a
               t=0, la fase retratta del PERIODO SUCCESSIVO e' comunque
               raggiungibile avanzando (il ciclo e' periodico). */
            int guardSteps = 0;
            int guardMax = (int)(2.0f*WORLD_HAZARD_PERIOD_SECONDS/dt) + 8;
            while (WorldHazardSpikesExtendedAt(cellX, cellY, localIndex, g.runElapsedSeconds) && guardSteps < guardMax)
            {
                g.runElapsedSeconds += dt;
                guardSteps++;
            }
            bool transitionSawDamage = false;
            guardSteps = 0;
            while (guardSteps < guardMax)
            {
                bool extendedNow = WorldHazardSpikesExtendedAt(cellX, cellY, localIndex, g.runElapsedSeconds);
                int hpBefore = g.player.hp;
                CombatUpdatePlayer(&g, dt, (Vector2){ 0.0f, 0.0f }, false);
                bool damagedNow = (g.player.hp < hpBefore);
                if (!extendedNow && damagedNow)
                {
                    fprintf(stderr, "GameObstaclesTest: (e3) piano %d: danno durante la transizione mentre il predicato diceva RETRATTI (t=%.3f)\n",
                            floor, g.runElapsedSeconds);
                    ok = false;
                    break;
                }
                if (extendedNow)
                {
                    if (!damagedNow)
                    {
                        fprintf(stderr, "GameObstaclesTest: (e3) piano %d: nessun danno al primo istante ESTESO raggiunto dalla transizione (t=%.3f)\n",
                                floor, g.runElapsedSeconds);
                        ok = false;
                    }
                    transitionSawDamage = true;
                    break;
                }
                g.runElapsedSeconds += dt;
                guardSteps++;
            }
            if (!transitionSawDamage && guardSteps >= guardMax)
            {
                fprintf(stderr, "GameObstaclesTest: (e3) piano %d: la transizione guidata non ha mai raggiunto una fase ESTESA entro due periodi\n", floor);
                ok = false;
            }

            /* (e4) I nemici continuano a ignorare il pericolo ANCHE in fase
               ESTESA (il gate del danno riguarda solo CombatResolveHazards;
               CombatResolveObstacles salta i pericoli per QUALUNQUE cerchio,
               invariato da WP3). */
            EntitiesClear(&g);
            /* g.runElapsedSeconds e' gia' fermo sul primo istante ESTESO
               raggiunto dalla transizione guidata in (e3) sopra: lo stesso
               istante, senza ricalcolarlo una seconda volta. */
            g.player.pos = hazardCenter;
            EntitiesAddEnemy(&g, ENEMY_CHASER, hazardCenter);
            int enemyIdx = -1;
            Vector2 enemyPosBefore = { 0.0f, 0.0f };
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (g.enemies[i].active) { enemyIdx = i; enemyPosBefore = g.enemies[i].pos; break; }
            if (enemyIdx < 0)
            {
                fprintf(stderr, "GameObstaclesTest: (e4) piano %d: nessuno slot nemico libero per il controllo\n", floor);
                ok = false;
            }
            else
            {
                CombatUpdateEnemies(&g, dt);
                Vector2 enemyPosAfter = g.enemies[enemyIdx].pos;
                float moved2 = GameMathLengthSquared(GameMathSubtract(enemyPosAfter, enemyPosBefore));
                if (moved2 > 0.001f)
                {
                    fprintf(stderr, "GameObstaclesTest: (e4) piano %d: un nemico sul pericolo in fase ESTESA si e' spostato (atteso: fermo, lo ignora)\n",
                            floor);
                    ok = false;
                }
            }
        }
        if (!found)
        {
            fprintf(stderr, "GameObstaclesTest: (e) nessun ostacolo PERICOLO trovato in %d piani a densita' massima: verifica WP-SPIKE (DEC-198) non eseguita\n", FLOOR_COUNT);
            ok = false;
        }
    }

    /* (e5) WP-SPIKE (DEC-198, seconda revisione): DECORRELAZIONE per SINGOLO
       spuntone, non solo per cella -- oltre meta' delle stanze del gioco e'
       1x1 (WORLD_SIZE_CUM_1X1, src/world/world.c), quindi due o piu' pericoli
       nella stessa stanza finiscono quasi sempre nella STESSA cella
       (cellX,cellY): se la fase dipendesse solo dalla cella pulserebbero
       all'unisono nel caso piu' frequente, esattamente il difetto che questa
       revisione corregge. Test PURO sul predicato -- non passa dalla
       generazione procedurale (che potrebbe non piazzare mai due pericoli
       nella stessa cella per i semi di questa suite): gira sempre, ad ogni
       esecuzione, e fallisce davvero con la mutazione che azzera cellX/cellY
       nell'hash o che omette localIndex dal mescolamento. */
    {
        int testCellX = 3, testCellY = -2;
        bool anyDiffer = false;
        for (int li = 1; li < ROOM_LAYOUT_MAX_PER_CELL && !anyDiffer; li++)
        {
            for (float t = 0.0f; t < WORLD_HAZARD_PERIOD_SECONDS; t += 0.05f)
            {
                if (WorldHazardSpikesExtendedAt(testCellX, testCellY, 0, t) !=
                    WorldHazardSpikesExtendedAt(testCellX, testCellY, li, t))
                {
                    anyDiffer = true;
                    break;
                }
            }
        }
        if (!anyDiffer)
        {
            fprintf(stderr, "GameObstaclesTest: (e5) spuntoni nella stessa cella (%d,%d) con indice locale diverso (0 contro 1..%d) restano PERFETTAMENTE in fase per un periodo intero: la fase non e' derivata dal singolo spuntone\n",
                    testCellX, testCellY, ROOM_LAYOUT_MAX_PER_CELL - 1);
            ok = false;
        }
    }

    if (ok) printf("  [obstacles] WP3: distruttibile rimosso dalla bomba e persistente rientrando, croce centrale libera per ogni famiglia, pericolo telegrafato che danneggia dentro gli i-frames (nemici lo ignorano), budget nemici ridotto dagli ostacoli (DEC-043); WP-SPIKE (DEC-198): spuntoni temporizzati, danno solo in fase estesa, stesso predicato puro di gameplay e resa, determinismo dalla cella E dall'indice locale (spuntoni della stessa cella decorrelati): ok\n");
    else fprintf(stderr, "GameObstaclesTest: FALLITO -- vedi i messaggi sopra\n");
    return ok;
}

/* ============================================================
   DEC-144 + DEC-145 (docs/design/systems/items-pools-and-rarity.md):
   estrazione dai pool con pesi di rarita' DEC-019, garanzia di copertura del
   pool curato minimo (DEC-144), correzione di fortuna con soglia N ridotta
   dalla Fortuna (DEC-145). Il modulo puro vive in src/gameplay/item_pool.c;
   questo test copre (a) l'esempio NORMATIVO del documento e il pool VERO di
   15 posizioni usato dal contenuto di ripiego, (b) la soglia N e la sua
   riduzione dalla Fortuna, (c) che la correzione limita DAVVERO la lunghezza
   di una sequenza sfortunata (non solo "di solito": prova avversariale con
   pesi apposta sbilanciati), (d) la DISTRIBUZIONE MARGINALE dell'estrazione
   fra candidati gia' pesati -- Scenario 1 del documento, "l'oggetto estratto
   rispetta i pesi configurati": e' la regressione misurata (rara/leggendaria
   dimezzate o quasi azzerate da una doppia applicazione dei pesi) che
   nessuno degli altri test qui sotto copriva, (e) la stessa cosa in forma
   statistica su molti semi/estrazioni con i pesi standard veri per la
   correzione di fortuna, (f) che il contenuto di ripiego (nessun manifest
   sul disco) rispetta la garanzia sull'INTERA run su molti semi e il boss
   non delude mai, (g) determinismo end-to-end dal seed di run.
   ============================================================ */

static bool ItemPoolTestMinimumCounts(void)
{
    int counts[ITEM_POOL_RARITY_COUNT];

    /* Esempio NORMATIVO del documento (items-pools-and-rarity.md, "Pool
       curato minimo"): pool di 20, pesi standard -> 11/6/2/1. */
    ItemPoolMinimumCounts(20, ItemPoolWeightsStandard, counts);
    if (counts[RARITY_COMMON] != 11 || counts[RARITY_UNCOMMON] != 6 ||
        counts[RARITY_RARE] != 2 || counts[RARITY_LEGENDARY] != 1)
    {
        fprintf(stderr, "ItemPoolTestMinimumCounts: pool 20 atteso {11,6,2,1}, ottenuto {%d,%d,%d,%d}\n",
                counts[0], counts[1], counts[2], counts[3]);
        return false;
    }

    /* Un pool di sole 3 posizioni (le posizioni di UN SOLO piano) azzera la
       comune per intero -- questo E' il motivo per cui DEC-144, nel
       contenuto di ripiego, NON si applica piu' alle 3 posizioni di ogni
       singolo piano ma alle 15 posizioni normali dell'INTERA run
       (run_content.c, GenerateFallbackContent): senza comuni la correzione
       di fortuna DEC-145 non potrebbe mai crescere il suo streak. Questo
       blocco resta solo l'esempio generico della funzione pura su un pool
       stretto, vedi il pool di 15 subito sotto per il caso che il gioco usa
       davvero. */
    ItemPoolMinimumCounts(3, ItemPoolWeightsStandard, counts);
    if (counts[RARITY_COMMON] != 0 || counts[RARITY_UNCOMMON] != 1 ||
        counts[RARITY_RARE] != 1 || counts[RARITY_LEGENDARY] != 1)
    {
        fprintf(stderr, "ItemPoolTestMinimumCounts: pool 3 atteso {0,1,1,1}, ottenuto {%d,%d,%d,%d}\n",
                counts[0], counts[1], counts[2], counts[3]);
        return false;
    }

    /* Il pool VERO usato dal contenuto di ripiego (run_content.c,
       GenerateFallbackContent): le 15 posizioni normali dell'intera run (5
       piani x 3, bossItem escluso). La comune resta la fascia maggioritaria
       -- a differenza del pool di 3 sopra -- cosi' la correzione di fortuna
       resta viva su questo cammino (GameItemPoolFallbackCoverageTest sotto
       verifica lo streak su questo stesso pool). */
    ItemPoolMinimumCounts(FLOOR_COUNT * 3, ItemPoolWeightsStandard, counts);
    if (counts[RARITY_COMMON] < 1)
    {
        fprintf(stderr, "ItemPoolTestMinimumCounts: pool %d (run intera) senza comuni, {%d,%d,%d,%d}\n",
                FLOOR_COUNT * 3, counts[0], counts[1], counts[2], counts[3]);
        return false;
    }
    if (counts[RARITY_UNCOMMON] < 1 || counts[RARITY_RARE] < 1 || counts[RARITY_LEGENDARY] < 1)
    {
        fprintf(stderr, "ItemPoolTestMinimumCounts: pool %d (run intera) senza copertura, {%d,%d,%d,%d}\n",
                FLOOR_COUNT * 3, counts[0], counts[1], counts[2], counts[3]);
        return false;
    }

    /* Invarianti su una banda di taglie: il totale non si sposta MAI da
       poolSize, e per un pool abbastanza grande (>= 4, il minimo che puo'
       ospitare le 4 rarita') nessuna rarita' a peso > 0 resta a zero. */
    for (int size = 0; size <= 40; size++)
    {
        ItemPoolMinimumCounts(size, ItemPoolWeightsStandard, counts);
        int sum = counts[0] + counts[1] + counts[2] + counts[3];
        if (sum != size)
        {
            fprintf(stderr, "ItemPoolTestMinimumCounts: pool %d, somma %d != poolSize\n", size, sum);
            return false;
        }
        if (size >= 4)
        {
            for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
            {
                if (ItemPoolWeightsStandard[r] > 0 && counts[r] < 1)
                {
                    fprintf(stderr, "ItemPoolTestMinimumCounts: pool %d, rarita' %d senza copertura\n", size, r);
                    return false;
                }
            }
        }
    }

    /* Pool boss (DEC-019 {0,0,70,30}): comune/non-comune restano SEMPRE a
       zero qualunque sia la taglia -- e' un peso 0 per costruzione, non un
       buco di arrotondamento (DEC-145, nota sul pool boss). */
    for (int size = 0; size <= 20; size++)
    {
        ItemPoolMinimumCounts(size, ItemPoolWeightsBoss, counts);
        if (counts[RARITY_COMMON] != 0 || counts[RARITY_UNCOMMON] != 0)
        {
            fprintf(stderr, "ItemPoolTestMinimumCounts: pool boss %d ha generato comune/non-comune (%d,%d)\n",
                    size, counts[0], counts[1]);
            return false;
        }
        if (counts[0] + counts[1] + counts[2] + counts[3] != size)
        {
            fprintf(stderr, "ItemPoolTestMinimumCounts: pool boss %d, somma non torna\n", size);
            return false;
        }
    }

    return true;
}

static bool ItemPoolTestLuckThreshold(void)
{
    struct { float luck; int expected; } cases[] = {
        { 0.0f, 4 }, { 4.0f, 3 }, { 8.0f, 2 }, { 12.0f, 1 }, { 15.0f, 1 },
        { 100.0f, 1 },    /* clamp al minimo 1 (richiesto dal documento) */
        { -5.0f, 4 },     /* Fortuna negativa non peggiora oltre la base (default proposto) */
    };
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++)
    {
        int n = ItemPoolLuckThreshold(cases[i].luck);
        if (n != cases[i].expected)
        {
            fprintf(stderr, "ItemPoolTestLuckThreshold: luck=%.1f atteso N=%d, ottenuto %d\n",
                    cases[i].luck, cases[i].expected, n);
            return false;
        }
        if (n < 1)
        {
            fprintf(stderr, "ItemPoolTestLuckThreshold: N sotto il clamp minimo (luck=%.1f -> %d)\n", cases[i].luck, n);
            return false;
        }
    }
    return true;
}

/* Prova AVVERSARIALE (non statistica): pesi apposta sbilanciati quasi certi
   (comune 1000000 contro 1 di non-comune), su un pool sintetico di soli 2
   candidati costruito apposta per isolare la correzione dalla distribuzione
   dei candidati stessi. Con lo streak GIA' alla soglia, ogni singola
   estrazione (su molti stati di RNG diversi) deve uscire non-comune --
   prova che la correzione SOVRASCRIVE del tutto la scelta (si restringe ai
   soli candidati non-comuni), non solo "di solito".
   Sotto soglia la correzione non deve intervenire affatto: la scelta fra i
   2 candidati e' UNIFORME (non piu' pesata da 'skewedWeights' -- vedi il
   commento sopra ItemPoolDrawIndex, item_pool.h: i pesi arrivano GIA'
   applicati da chi genera i candidati, ripesarli qui dentro e' esattamente
   il difetto corretto in questa fase, misurato su 2.000.000 di estrazioni
   reali: rara/leggendaria dimezzate o quasi azzerate da una doppia
   applicazione dei pesi). La tabella 'skewedWeights' resta qui SOLO per
   verificare che un peso estremo non distorca la scelta uniforme (ne' la
   forzi verso il comune ne' la escluda: entrambi i pesi sono > 0, quindi
   il filtro di ammissione difensivo li lascia passare invariati) -- il test
   sulla distribuzione marginale VERA rispetto ai pesi vive in
   ItemPoolTestDrawIndexRespectsWeights sotto. */
static bool ItemPoolTestDrawIndexForces(void)
{
    static const int skewedWeights[ITEM_POOL_RARITY_COUNT] = { 1000000, 1, 0, 0 };
    const Rarity candidates[2] = { RARITY_COMMON, RARITY_UNCOMMON };
    const float luck = 0.0f;
    const int threshold = ItemPoolLuckThreshold(luck);

    for (unsigned int rngSeed = 1; rngSeed <= 200; rngSeed++)
    {
        unsigned int rng = rngSeed;
        int streak = threshold;   /* correzione GIA' attiva */
        int picked = ItemPoolDrawIndex(&rng, candidates, 2, skewedWeights, luck, &streak);
        if (candidates[picked] == RARITY_COMMON)
        {
            fprintf(stderr, "ItemPoolTestDrawIndexForces: streak=%d (soglia %d) ma l'estrazione e' comune (seed %u)\n",
                    threshold, threshold, rngSeed);
            return false;
        }
        if (streak != 0)
        {
            fprintf(stderr, "ItemPoolTestDrawIndexForces: streak non azzerato dopo la correzione (seed %u)\n", rngSeed);
            return false;
        }
    }

    int commonCount = 0;
    const int belowThresholdTrials = 2000;
    for (int t = 0; t < belowThresholdTrials; t++)
    {
        unsigned int rng = (unsigned int)(t + 1) * 7919u;
        int streak = threshold - 1;   /* correzione NON ancora attiva */
        int picked = ItemPoolDrawIndex(&rng, candidates, 2, skewedWeights, luck, &streak);
        if (candidates[picked] == RARITY_COMMON) commonCount++;
    }
    /* Uniforme fra 2 candidati -> atteso ~50%; banda larga (35-65%) per non
       essere fragile ma comunque incompatibile con "quasi sempre comune"
       (il comportamento pesato che questo test validava prima del fix) o
       "quasi mai comune". */
    if (commonCount < (belowThresholdTrials * 35 / 100) || commonCount > (belowThresholdTrials * 65 / 100))
    {
        fprintf(stderr, "ItemPoolTestDrawIndexForces: sotto soglia, %d/%d estrazioni comuni (atteso ~50%%, scelta uniforme fra 2 candidati)\n",
                commonCount, belowThresholdTrials);
        return false;
    }

    return true;
}

/* Scenario 1 del documento ("l'oggetto estratto rispetta i pesi configurati,
   salvo intervento della correzione di fortuna"): DISTRIBUZIONE MARGINALE
   dell'estrazione su candidati gia' pesati -- come i 3 candidati REALI di un
   piano (ciascuno con una rarita' gia' tirata da ItemPoolRollRarity con gli
   stessi pesi standard, esattamente il pattern di melting-gen/GenRollRarity
   e di ItemPoolMinimumCounts). Streak azzerato ad ogni prova (luck=0, soglia
   mai raggiunta): isola la distribuzione dell'estrazione dalla correzione di
   fortuna, che ha gia' il suo test dedicato sopra e sotto.
   Questa e' la regressione misurata nel round di revisione precedente:
   pesare di nuovo i candidati (gia' pesati a monte) dentro ItemPoolDrawIndex
   eleva la distribuzione al quadrato -- su 2.000.000 di estrazioni, rara
   scendeva da 12% a 6.4% (-46%) e leggendaria da 3% a 0.67% (-78%). La
   scelta UNIFORME fra candidati gia' pesati (vedi il commento sopra
   ItemPoolDrawIndex, item_pool.h) riproduce i pesi esattamente: nessun altro
   test in questo file eserciterebbe questa regressione, perche' gli altri
   usano tabelle sintetiche costruite ad hoc per isolare streak/soglia. */
static bool ItemPoolTestDrawIndexRespectsWeights(void)
{
    const int trials = 200000;
    int counts[ITEM_POOL_RARITY_COUNT] = { 0, 0, 0, 0 };
    unsigned int rng = 909090u;

    for (int t = 0; t < trials; t++)
    {
        Rarity candidates[3];
        for (int i = 0; i < 3; i++) candidates[i] = ItemPoolRollRarity(&rng, ItemPoolWeightsStandard);
        int streak = 0;   /* isolata dalla correzione di fortuna, vedi sopra */
        int picked = ItemPoolDrawIndex(&rng, candidates, 3, ItemPoolWeightsStandard, 0.0f, &streak);
        counts[candidates[picked]]++;
    }

    static const double expectedPct[ITEM_POOL_RARITY_COUNT] = { 55.0, 30.0, 12.0, 3.0 };
    static const double tolerancePct[ITEM_POOL_RARITY_COUNT] = { 2.0, 2.0, 1.5, 1.0 };
    double gotPct[ITEM_POOL_RARITY_COUNT];
    bool ok = true;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
    {
        gotPct[r] = 100.0 * (double)counts[r] / (double)trials;
        if (fabs(gotPct[r] - expectedPct[r]) > tolerancePct[r]) ok = false;
    }
    printf("  [item-pool] distribuzione marginale su %d estrazioni: comune %.2f%% non-comune %.2f%% rara %.2f%% leggendaria %.2f%% (attese 55/30/12/3)\n",
           trials, gotPct[0], gotPct[1], gotPct[2], gotPct[3]);
    if (!ok)
    {
        fprintf(stderr, "ItemPoolTestDrawIndexRespectsWeights: distribuzione %.2f/%.2f/%.2f/%.2f fuori tolleranza da 55/30/12/3\n",
                gotPct[0], gotPct[1], gotPct[2], gotPct[3]);
        return false;
    }
    return true;
}

/* Statistica su molti semi/molte estrazioni con i pesi STANDARD veri (non
   una tabella costruita ad hoc): la Fortuna alta abbassa la soglia N e
   quindi limita quanto puo' crescere una sequenza sfortunata consecutiva. */
static bool ItemPoolTestLuckShortensStreaks(void)
{
    const Rarity candidates[3] = { RARITY_COMMON, RARITY_COMMON, RARITY_UNCOMMON };
    const int seeds = 200;
    const int drawsPerSeed = 60;

    int maxStreakLowLuck = 0;
    int maxStreakHighLuck = 0;
    const float lowLuck = 0.0f;
    const float highLuck = 15.0f;   /* tetto del clamp di gioco, vedi player.md */
    const int thresholdLow = ItemPoolLuckThreshold(lowLuck);
    const int thresholdHigh = ItemPoolLuckThreshold(highLuck);

    for (int s = 0; s < seeds; s++)
    {
        unsigned int rngLow = (unsigned int)(s * 2654435761u + 1u);
        unsigned int rngHigh = rngLow;   /* stesso stream: confronto a parita' di sfortuna sui tiri */
        int streakLow = 0;
        int streakHigh = 0;
        int runLow = 0;
        int runHigh = 0;
        for (int d = 0; d < drawsPerSeed; d++)
        {
            int pickedLow = ItemPoolDrawIndex(&rngLow, candidates, 3, ItemPoolWeightsStandard, lowLuck, &streakLow);
            if (candidates[pickedLow] == RARITY_COMMON) { runLow++; if (runLow > maxStreakLowLuck) maxStreakLowLuck = runLow; }
            else runLow = 0;

            int pickedHigh = ItemPoolDrawIndex(&rngHigh, candidates, 3, ItemPoolWeightsStandard, highLuck, &streakHigh);
            if (candidates[pickedHigh] == RARITY_COMMON) { runHigh++; if (runHigh > maxStreakHighLuck) maxStreakHighLuck = runHigh; }
            else runHigh = 0;
        }
    }

    if (maxStreakLowLuck > thresholdLow)
    {
        fprintf(stderr, "ItemPoolTestLuckShortensStreaks: streak comune massimo con luck=0 (%d) supera la soglia (%d)\n",
                maxStreakLowLuck, thresholdLow);
        return false;
    }
    if (maxStreakHighLuck > thresholdHigh)
    {
        fprintf(stderr, "ItemPoolTestLuckShortensStreaks: streak comune massimo con luck=15 (%d) supera la soglia (%d)\n",
                maxStreakHighLuck, thresholdHigh);
        return false;
    }
    if (maxStreakHighLuck > maxStreakLowLuck)
    {
        fprintf(stderr, "ItemPoolTestLuckShortensStreaks: la Fortuna alta ha prodotto una sequenza PIU' lunga (%d) di quella bassa (%d)\n",
                maxStreakHighLuck, maxStreakLowLuck);
        return false;
    }
    if (maxStreakLowLuck < 2)
    {
        /* 12000 estrazioni (200 semi x 60) con peso comune >> non-comune che
           non producono MAI due comuni di fila e' statisticamente
           implausibile: se succede, e' il test ad essere scritto male, non
           il codice -- meglio farlo fallire rumorosamente che lasciarlo
           passare senza aver messo alla prova nulla. */
        fprintf(stderr, "ItemPoolTestLuckShortensStreaks: la correzione non e' mai stata messa alla prova (streak massimo osservato %d)\n", maxStreakLowLuck);
        return false;
    }

    printf("  [item-pool] streak massimo comune: luck=0 -> %d (soglia %d), luck=15 -> %d (soglia %d), su %d semi x %d estrazioni\n",
           maxStreakLowLuck, thresholdLow, maxStreakHighLuck, thresholdHigh, seeds, drawsPerSeed);
    return true;
}

/* DEC-144 sul contenuto di ripiego VERO (RunContentLoad quando
   generated/current_run.txt non esiste, MakeFallbackItem/MakeFallbackBossItem
   in run_content.c): la garanzia di copertura si applica alle 15 posizioni
   NORMALI dell'INTERA run (5 piani x 3), non alle 3 di un singolo piano --
   vedi il commento su GenerateFallbackContent, run_content.c. Per molti semi
   di run l'AGGREGATO delle 15 posizioni deve rispettare esattamente
   ItemPoolMinimumCounts(FLOOR_COUNT*3, ...) e il boss di ogni piano non deve
   mai essere comune o non-comune ("il boss non delude mai", tramite i pesi
   del pool boss invece di un valore forzato). Sposta via il manifest PRIMA
   di caricare (rename, non remove: un giro precedente di make/make test-gen
   puo' averne lasciato uno vero in generated/, e questo test non e' la sede
   giusta per distruggerlo) per essere sicuri di esercitare il ramo di
   ripiego puro -- stesso schema di TestFallbackBossItemIsRare in
   src/tests/script_items_tests.c -- poi lo rimette al suo posto subito
   dopo, che il test passi o fallisca. Non serve un Game/una finestra:
   RunContentLoad e' indipendente da AssetsLoad. */
static bool GameItemPoolFallbackCoverageTest(void)
{
    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.item-pool-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    int expected[ITEM_POOL_RARITY_COUNT];
    ItemPoolMinimumCounts(FLOOR_COUNT * 3, ItemPoolWeightsStandard, expected);

    bool ok = true;
    for (unsigned int seed = 1000u; ok && seed < 1000u + 60u; seed++)
    {
        RunContent content;
        memset(&content, 0, sizeof(content));
        RunContentLoad(&content, seed);

        int seen[ITEM_POOL_RARITY_COUNT] = { 0, 0, 0, 0 };
        for (int f = 0; f < FLOOR_COUNT && ok; f++)
        {
            const FloorContent *fc = &content.floors[f];
            for (int i = 0; i < 3; i++) seen[fc->items[i].rarity]++;
            if (fc->bossItem.rarity != RARITY_RARE && fc->bossItem.rarity != RARITY_LEGENDARY)
            {
                fprintf(stderr, "GameItemPoolFallbackCoverageTest: seed %u piano %d, il boss ha rarita' %d (attesa rara o leggendaria)\n",
                        seed, f + 1, (int)fc->bossItem.rarity);
                ok = false;
            }
        }
        if (ok && (seen[0] != expected[0] || seen[1] != expected[1] || seen[2] != expected[2] || seen[3] != expected[3]))
        {
            fprintf(stderr, "GameItemPoolFallbackCoverageTest: seed %u, run intera attesa {%d,%d,%d,%d} ottenuta {%d,%d,%d,%d}\n",
                    seed, expected[0], expected[1], expected[2], expected[3], seen[0], seen[1], seen[2], seen[3]);
            ok = false;
        }
    }

    if (hadManifest) rename(kBackup, kManifest);
    return ok;
}

/* Gli streak di correzione (DEC-145) sono stato di RUN, non di contenuto:
   devono tornare a zero a ogni reset esattamente come ogni altro campo che
   il memset di GameResetRunWithSeed azzera. Indipendente da un manifest sul
   disco (non tocca la rarita' di nulla), quindi nessun rename qui. */
static bool GameItemPoolStreakResetTest(Game *game)
{
    GameResetRunWithSeed(game, 55u);
    game->treasureLuckStreak = 3;
    game->shopLuckStreak = 2;

    GameResetRunWithSeed(game, 56u);
    if (game->treasureLuckStreak != 0 || game->shopLuckStreak != 0)
    {
        fprintf(stderr, "GameItemPoolStreakResetTest: gli streak di correzione non sono azzerati da un reset (%d, %d)\n",
                game->treasureLuckStreak, game->shopLuckStreak);
        return false;
    }
    return true;
}

typedef struct ItemPoolFingerprintItem
{
    char name[48];
    Rarity rarity;
    ItemSlot slot;
    int shape;
} ItemPoolFingerprintItem;

typedef struct ItemPoolFingerprint
{
    ItemPoolFingerprintItem items[FLOOR_COUNT][3];
    ItemPoolFingerprintItem boss[FLOOR_COUNT];
} ItemPoolFingerprint;

static ItemPoolFingerprintItem ItemPoolFingerprintOf(const Item *item)
{
    ItemPoolFingerprintItem out = { 0 };
    snprintf(out.name, sizeof(out.name), "%s", item->name);
    out.rarity = item->rarity;
    out.slot = item->slot;
    out.shape = item->shape;
    return out;
}

static ItemPoolFingerprint ItemPoolCaptureFingerprint(const Game *game)
{
    ItemPoolFingerprint fp;
    memset(&fp, 0, sizeof(fp));
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        const FloorContent *fc = &game->content.floors[f];
        for (int i = 0; i < 3; i++) fp.items[f][i] = ItemPoolFingerprintOf(&fc->items[i]);
        fp.boss[f] = ItemPoolFingerprintOf(&fc->bossItem);
    }
    return fp;
}

static bool ItemPoolFingerprintItemEqual(const ItemPoolFingerprintItem *a, const ItemPoolFingerprintItem *b)
{
    return strcmp(a->name, b->name) == 0 && a->rarity == b->rarity && a->slot == b->slot && a->shape == b->shape;
}

static bool ItemPoolFingerprintEqual(const ItemPoolFingerprint *a, const ItemPoolFingerprint *b)
{
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        for (int i = 0; i < 3; i++)
            if (!ItemPoolFingerprintItemEqual(&a->items[f][i], &b->items[f][i])) return false;
        if (!ItemPoolFingerprintItemEqual(&a->boss[f], &b->boss[f])) return false;
    }
    return true;
}

/* Determinismo end-to-end (requisito esplicito di DEC-144/DEC-145: "tutto
   deterministico dall'RNG di gameplay derivato dal seed, mai time/rand
   globali"): stesso seed -> stesso contenuto di ripiego e stesso RNG
   finale; seed diverso -> il contenuto differisce da qualche parte. */
static bool GameItemPoolDeterminismTest(Game *game)
{
    const unsigned int seedA = 424242u;
    const unsigned int seedB = 777777u;

    GameResetRunWithSeed(game, seedA);
    ItemPoolFingerprint firstA = ItemPoolCaptureFingerprint(game);
    unsigned int rngA1 = game->rng;

    GameResetRunWithSeed(game, seedA);
    ItemPoolFingerprint secondA = ItemPoolCaptureFingerprint(game);
    unsigned int rngA2 = game->rng;

    if (rngA1 != rngA2)
    {
        fprintf(stderr, "GameItemPoolDeterminismTest: due reset con lo STESSO seed (%u) hanno lasciato game->rng diverso (%u vs %u)\n",
                seedA, rngA1, rngA2);
        return false;
    }
    if (!ItemPoolFingerprintEqual(&firstA, &secondA))
    {
        fprintf(stderr, "GameItemPoolDeterminismTest: due reset con lo STESSO seed (%u) hanno prodotto contenuti diversi\n", seedA);
        return false;
    }

    GameResetRunWithSeed(game, seedB);
    ItemPoolFingerprint firstB = ItemPoolCaptureFingerprint(game);

    if (ItemPoolFingerprintEqual(&firstA, &firstB))
    {
        fprintf(stderr, "GameItemPoolDeterminismTest: semi DIVERSI (%u vs %u) hanno prodotto lo STESSO contenuto\n", seedA, seedB);
        return false;
    }

    return true;
}

bool GameItemPoolTest(Game *game)
{
    if (!ItemPoolTestMinimumCounts()) return false;
    if (!ItemPoolTestLuckThreshold()) return false;
    if (!ItemPoolTestDrawIndexForces()) return false;
    if (!ItemPoolTestDrawIndexRespectsWeights()) return false;
    if (!ItemPoolTestLuckShortensStreaks()) return false;
    if (!GameItemPoolFallbackCoverageTest()) return false;
    if (!GameItemPoolStreakResetTest(game)) return false;
    if (!GameItemPoolDeterminismTest(game)) return false;
    return true;
}

/* ============================================================
   DEC-167 (docs/design/systems/rewards-and-economy.md, "Fonti canoniche
   della valuta principale"): la valuta principale si guadagna da QUALUNQUE
   stanza completata secondo la PROPRIA condizione -- combattimento
   ripulito, boss sconfitto, tesoro aperto, negozio visitato -- non solo dal
   combattimento. Come GameRngSeedTest, gira dopo InitWindow e usa 'game'
   per davvero (GameResetRunWithSeed chiama AssetsLoad), ma entra
   direttamente nelle stanze (roomX/roomY + WorldSpawnRoomContents) invece
   di navigare per porte: qui interessa solo l'assegnazione di valuta
   (WorldAwardRoomCompletionCurrency, src/world/world.c), non la
   navigazione. Gli importi letterali sotto (4/12/3/2) sono gli stessi
   "default proposti dall'implementazione" (stile DEC-019) delle costanti
   WORLD_ROOM_CURRENCY_* in world.c: se quei numeri cambiano, questo test va
   aggiornato in coppia (stessa convenzione di TestShopCostScalesWithRarity
   in script_items_tests.c, che hardcoda 8/16/28/45).
   ============================================================ */
static bool EconomyEnterRoomOfKind(Game *game, RoomKind kind)
{
    for (int y = 0; y < GRID_SIZE; y++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (game->rooms[y][x].kind == kind)
            {
                game->roomX = x;
                game->roomY = y;
                return true;
            }
        }
    }
    return false;
}

bool GameEconomyTest(Game *game)
{
    bool ok = true;
    const unsigned int seed = 20260727u;
    const int kCombatCurrency = 4, kBossCurrency = 12, kTreasureCurrency = 3, kShopCurrency = 2;
    GameResetRunWithSeed(game, seed);

    /* (a) Combattimento ripulito: entrare, disattivare tutti i nemici a mano
       (stesso spirito di GameRoomsTest, che forza 'cleared' a mano per
       isolare cio' che si vuole verificare -- qui interessa solo la
       valuta, non il combattimento vero), poi WorldCheckRoomClear. */
    if (!EconomyEnterRoomOfKind(game, ROOM_COMBAT))
    {
        fprintf(stderr, "GameEconomyTest: nessuna stanza di combattimento nel piano 1 col seed %u\n", seed);
        return false;
    }
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    int coinsBeforeCombat = game->player.coins;
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    int combatGain = game->player.coins - coinsBeforeCombat;
    bool combatGainOk = combatGain == kCombatCurrency;
    if (!combatGainOk) fprintf(stderr, "GameEconomyTest: combattimento ripulito ha dato %d monete (attese %d)\n", combatGain, kCombatCurrency);

    /* Rientrare/richiamare il controllo su una stanza gia' ripulita non deve
       pagare una seconda volta (guardia 'cleared' in WorldCheckRoomClear). */
    int coinsAfterCombat = game->player.coins;
    WorldSpawnRoomContents(game);
    WorldCheckRoomClear(game);
    bool noDoublePayCombat = game->player.coins == coinsAfterCombat;
    if (!noDoublePayCombat) fprintf(stderr, "GameEconomyTest: rientrare in un combattimento gia' ripulito ha pagato di nuovo (%d -> %d)\n", coinsAfterCombat, game->player.coins);

    /* (b) Boss sconfitto: stessa tecnica (disattivare il boss a mano). */
    if (!EconomyEnterRoomOfKind(game, ROOM_BOSS))
    {
        fprintf(stderr, "GameEconomyTest: nessuna stanza boss nel piano 1 col seed %u\n", seed);
        return false;
    }
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    int coinsBeforeBoss = game->player.coins;
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    int bossGain = game->player.coins - coinsBeforeBoss;
    bool bossGainOk = bossGain == kBossCurrency;
    if (!bossGainOk) fprintf(stderr, "GameEconomyTest: boss sconfitto ha dato %d monete (attese %d)\n", bossGain, kBossCurrency);

    /* (c) Tesoro aperto: si apre quando l'oggetto viene preso (rewardTaken),
       non solo entrando. Il giocatore si piazza sul centro della stanza,
       dove WorldSpawnRoomContents ha messo il pickup, e CombatUpdatePickups
       fa il resto (nessuna chiave necessaria: qui si entra saltando
       WorldTryEnterRoom, che e' l'unico punto che la richiede). */
    if (!EconomyEnterRoomOfKind(game, ROOM_TREASURE))
    {
        fprintf(stderr, "GameEconomyTest: nessuna stanza tesoro nel piano 1 col seed %u\n", seed);
        return false;
    }
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    int coinsBeforeTreasure = game->player.coins;
    int itemsBeforeTreasure = game->player.itemCount;
    game->player.pos = WorldRoomCenter(game);
    CombatUpdatePickups(game);
    int treasureGain = game->player.coins - coinsBeforeTreasure;
    bool treasureGainOk = treasureGain == kTreasureCurrency;
    bool treasureItemTaken = game->player.itemCount == itemsBeforeTreasure + 1;
    if (!treasureGainOk) fprintf(stderr, "GameEconomyTest: tesoro aperto ha dato %d monete (attese %d)\n", treasureGain, kTreasureCurrency);
    if (!treasureItemTaken) fprintf(stderr, "GameEconomyTest: il tesoro non ha consegnato l'oggetto (itemCount %d -> %d)\n", itemsBeforeTreasure, game->player.itemCount);

    /* Guardia 'rewardTaken': un secondo passaggio di CombatPickup sulla
       STESSA stanza (pickup riattivato a mano, stesso spirito delle prove
       sopra) non deve ripagare -- e' la garanzia che protegge da loop
       economici (rewards-and-economy.md, §Protezioni). */
    int coinsAfterTreasure = game->player.coins;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (game->pickups[i].kind == PICKUP_ITEM) { game->pickups[i].active = true; game->pickups[i].locked = false; break; }
    }
    CombatUpdatePickups(game);
    bool noDoublePayTreasure = game->player.coins == coinsAfterTreasure;
    if (!noDoublePayTreasure) fprintf(stderr, "GameEconomyTest: un secondo pickup nella stanza tesoro ha pagato di nuovo (%d -> %d)\n", coinsAfterTreasure, game->player.coins);

    /* (d) Negozio visitato: la valuta arriva SUBITO all'ingresso (DEC-167:
       "ripulito" per il negozio significa "visitato", non "si e' comprato
       qualcosa" -- quello e' un evento economico a parte). */
    if (!EconomyEnterRoomOfKind(game, ROOM_SHOP))
    {
        fprintf(stderr, "GameEconomyTest: nessun negozio nel piano 1 col seed %u\n", seed);
        return false;
    }
    EntitiesClear(game);
    int coinsBeforeShop = game->player.coins;
    WorldSpawnRoomContents(game);
    int shopGain = game->player.coins - coinsBeforeShop;
    bool shopGainOk = shopGain == kShopCurrency;
    if (!shopGainOk) fprintf(stderr, "GameEconomyTest: il negozio visitato ha dato %d monete (attese %d)\n", shopGain, kShopCurrency);

    /* Rientrare nello stesso negozio non deve ripagare (guardia 'visited'). */
    int coinsAfterShop = game->player.coins;
    WorldSpawnRoomContents(game);
    bool noDoublePayShop = game->player.coins == coinsAfterShop;
    if (!noDoublePayShop) fprintf(stderr, "GameEconomyTest: rientrare in un negozio gia' visitato ha pagato di nuovo (%d -> %d)\n", coinsAfterShop, game->player.coins);

    printf("  economia (DEC-167): combattimento +%d (atteso %d, niente doppio pagamento=%s) | boss +%d (atteso %d) | tesoro +%d (atteso %d, oggetto preso=%s, niente doppio=%s) | negozio +%d (atteso %d, niente doppio=%s)\n",
           combatGain, kCombatCurrency, noDoublePayCombat ? "si" : "NO",
           bossGain, kBossCurrency,
           treasureGain, kTreasureCurrency, treasureItemTaken ? "si" : "NO", noDoublePayTreasure ? "si" : "NO",
           shopGain, kShopCurrency, noDoublePayShop ? "si" : "NO");

    ok = combatGainOk && noDoublePayCombat && bossGainOk && treasureGainOk && treasureItemTaken &&
         noDoublePayTreasure && shopGainOk && noDoublePayShop;
    if (!ok) fprintf(stderr, "GameEconomyTest: FALLITO -- vedi i messaggi sopra\n");
    return ok;
}

/* ============================================================
   LA FUSIONE (docs/design/systems/item-fusion.md; DEC-022/023/101/102/143/
   162/171). Come GameEconomyTest, gira dopo InitWindow e usa 'game' per
   davvero (GameResetRunWithSeed chiama AssetsLoad) ma non disegna nulla:
   la fusione e' interamente logica, e l'unica parte che tocca la GPU (la
   texture dell'immagine curata) vive nel renderer, fuori da qui.
   Gli oggetti sorgente li costruisce il test a mano invece di pescarli dal
   contenuto della run: servono coppie CONTROLLATE (rarita' e categorie
   scelte) per verificare dominanza e tie-break, e il contenuto vero cambia
   con il manifest presente sul disco.
   ============================================================ */
#define FUSION_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameFusionTest: %s\n", (msg)); return false; } } while (0)

static Item FusionTestItem(const char *name, ItemKind kind, Rarity rarity, unsigned int traits, ItemSlot slot)
{
    Item item = { 0 };
    item.active = true;
    snprintf(item.name, sizeof(item.name), "%s", name);
    item.kind = kind;
    item.rarity = rarity;
    item.traits = traits;
    item.slot = slot;
    item.color = (Color){ 120, 160, 220, 255 };
    item.shape = 2;
    return item;
}

static Item FusionTestShotItem(const char *name, Rarity rarity, unsigned int traits, int exampleIndex)
{
    Item item = FusionTestItem(name, ITEM_PASSIVE, rarity, traits, SLOT_HAND);
    ShotTypeExample(&item.shotType, exampleIndex);
    return item;
}

/* Un Game pronto a fondere: run vera dal seed, inventario azzerato e
   sostituito dagli oggetti del test, catalizzatori a volonta'. */
static void FusionTestSetup(Game *game, unsigned int seed, const Item *items, int count, int flux)
{
    GameResetRunWithSeed(game, seed);
    ScriptItemsInit(game, NULL);
    game->player.itemCount = 0;
    for (int i = 0; i < count && i < MAX_ITEMS; i++)
    {
        game->player.items[i] = items[i];
        game->player.itemCount = i + 1;
        ScriptItemsOnAcquire(game, i);
    }
    ScriptItemsProcessDirty(game);
    game->player.flux = flux;
}

static bool FusionTestItemsEqual(const Item *a, const Item *b)
{
    return strcmp(a->name, b->name) == 0 && a->kind == b->kind && a->rarity == b->rarity &&
           a->traits == b->traits && a->slot == b->slot && a->shape == b->shape &&
           a->color.r == b->color.r && a->color.g == b->color.g && a->color.b == b->color.b &&
           a->shotType.active == b->shotType.active &&
           a->shotType.damageMul == b->shotType.damageMul &&
           a->shotType.pellets == b->shotType.pellets &&
           strcmp(a->imagePath, b->imagePath) == 0;
}

/* (a) Determinismo: stesso seed + stessa coppia => stesso fuso, bit per bit
   sui campi che il giocatore vede. E' la garanzia che rende la fusione
   riproducibile quanto il resto della run (DEC-141). */
static bool FusionTestDeterminism(Game *game)
{
    const unsigned int seed = 20260727u;
    Item sources[2] = {
        FusionTestShotItem("Lama Cava", RARITY_UNCOMMON, TRAIT_BOUNCE | TRAIT_PIERCE, 0),
        FusionTestShotItem("Occhio Freddo", RARITY_RARE, TRAIT_SLOW, 2)
    };

    Item first, second;
    FusionTestSetup(game, seed, sources, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &first) == FUSION_OK, "la prima fusione non e' riuscita");
    FusionTestSetup(game, seed, sources, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &second) == FUSION_OK, "la seconda fusione non e' riuscita");
    FUSION_CHECK(FusionTestItemsEqual(&first, &second), "stesso seed e stessa coppia hanno dato due oggetti diversi");

    /* Seed diverso: la chiave cambia, quindi il risultato PUO' cambiare --
       non e' un requisito bit-per-bit (con questa coppia molte regole sono
       deterministiche per costruzione), ma la chiave deve essere diversa,
       altrimenti il seed non entrerebbe affatto nel risultato. */
    unsigned int keyA = FusionKey(seed, 0, &sources[0], &sources[1]);
    unsigned int keyB = FusionKey(seed + 1u, 0, &sources[0], &sources[1]);
    unsigned int keyLater = FusionKey(seed, 1, &sources[0], &sources[1]);
    FUSION_CHECK(keyA != keyB, "seed diversi producono la stessa chiave di fusione");
    FUSION_CHECK(keyA != keyLater, "la seconda fusione della run riusa la chiave della prima");
    printf("  fusione (a) determinismo: stesso seed => \"%s\" identico; chiavi distinte per seed/ordinale: si\n", first.name);
    return true;
}

/* (b) DEC-143: la categoria del risultato e' quella della sorgente DOMINANTE
   per rarita'; a parita' vince quella selezionata per prima. */
static bool FusionTestDominantCategory(Game *game)
{
    Item cross[2] = {
        FusionTestItem("Guanto Vivo", ITEM_ACTIVE, RARITY_COMMON, TRAIT_EXPLODE, SLOT_HAND),
        FusionTestItem("Radice Nera", ITEM_GRAFT, RARITY_RARE, TRAIT_VAMP, SLOT_BODY)
    };
    cross[0].cooldown = 8.0f;

    Item fused;
    FusionTestSetup(game, 4242u, cross, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &fused) == FUSION_OK, "la fusione cross-categoria e' stata rifiutata (DEC-101)");
    FUSION_CHECK(fused.kind == ITEM_GRAFT, "la categoria non e' quella della sorgente piu' rara (DEC-143)");
    FUSION_CHECK(fused.slot == SLOT_BODY, "lo slot visivo non e' quello della sorgente dominante");

    /* L'ORDINE di selezione non cambia il dominante quando le rarita' sono
       diverse: scegliendo prima il comune il risultato resta Innesto. */
    Item swapped;
    FusionTestSetup(game, 4242u, cross, 2, 1);
    FUSION_CHECK(FusionPerform(game, 1, 0, &swapped) == FUSION_OK, "la fusione con ordine invertito e' stata rifiutata");
    FUSION_CHECK(swapped.kind == ITEM_GRAFT, "invertendo l'ordine la rarita' piu' alta non domina piu'");

    /* Tie-break: stessa rarita' => vince la PRIMA selezionata. */
    Item tie[2] = {
        FusionTestItem("Ala Corta", ITEM_ACTIVE, RARITY_UNCOMMON, TRAIT_RAPID, SLOT_BACK),
        FusionTestItem("Pietra Muta", ITEM_STATUP, RARITY_UNCOMMON, TRAIT_GIANT, SLOT_BODY)
    };
    tie[0].charges = 3;
    Item tieFirst, tieSecond;
    FusionTestSetup(game, 4242u, tie, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &tieFirst) == FUSION_OK, "fusione a parita' di rarita' rifiutata");
    FUSION_CHECK(tieFirst.kind == ITEM_ACTIVE, "a parita' di rarita' non vince la categoria del primo selezionato");
    FusionTestSetup(game, 4242u, tie, 2, 1);
    FUSION_CHECK(FusionPerform(game, 1, 0, &tieSecond) == FUSION_OK, "fusione a parita' di rarita' (ordine invertito) rifiutata");
    FUSION_CHECK(tieSecond.kind == ITEM_STATUP, "il tie-break non segue l'ordine di selezione del giocatore");
    /* Un attivo fuso nasce carico e con una dichiarazione di ricarica
       valida: mai un attivo che ricade sul cooldown di riserva del motore
       solo perche' e' nato da una fusione. */
    FUSION_CHECK(ItemActiveIsChargeBased(&tieFirst) || ItemActiveIsCooldownBased(&tieFirst),
                 "l'attivo fuso non dichiara ne' cariche ne' cooldown");
    FUSION_CHECK(ItemActiveIsReady(&tieFirst), "l'attivo fuso non nasce pronto all'uso");

    printf("  fusione (b) DEC-143: cross-categoria -> %s | tie-break sul primo selezionato: si\n",
           fused.kind == ITEM_GRAFT ? "Innesto (piu' raro)" : "??");
    return true;
}

/* (c) DEC-162: il risultato ha un budget di potenza DEDICATO, piu' alto di
   quello di un singolo oggetto -- rarita' di un gradino sopra la dominante
   (il canale statistico) e tipo di colpo bilanciato sulla banda della
   fusione (il canale comportamentale), senza mai sfondare il tetto. */
static bool FusionTestPowerBudget(Game *game)
{
    Item pair[2] = {
        FusionTestShotItem("Chiodo Lungo", RARITY_COMMON, TRAIT_PIERCE, 0),
        FusionTestShotItem("Scarica Bassa", RARITY_UNCOMMON, TRAIT_SLOW, 2)
    };
    Item fused;
    FusionTestSetup(game, 777u, pair, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &fused) == FUSION_OK, "fusione con due tipi di colpo rifiutata");
    FUSION_CHECK(fused.rarity == RARITY_RARE, "la rarita' del risultato non sale di un gradino sopra la dominante");
    FUSION_CHECK(fused.shotType.active, "il risultato ha perso il tipo di colpo di entrambi i genitori");
    float power = ShotTypePower(&fused.shotType);
    FUSION_CHECK(power >= SHOT_TYPE_FUSION_POWER_MIN - 0.001f && power <= SHOT_TYPE_FUSION_POWER_MAX + 0.001f,
                 "la potenza del colpo fuso e' fuori dalla banda dedicata (DEC-162)");
    FUSION_CHECK(power > SHOT_TYPE_POWER_MAX - 0.001f, "il colpo fuso non e' piu' forte del miglior colpo singolo");
    /* Il budget di LEGGIBILITA' invece NON si allarga (DEC-146). */
    FUSION_CHECK(ShotTypeReadabilityOk(&fused.shotType), "il colpo fuso sfonda il budget di leggibilita' (DEC-146)");
    /* Trait: il risultato porta segni di entrambi ma non piu' di
       FUSION_MAX_TRAITS, e mai due tratti in conflitto sulla stessa
       proprieta'. */
    int traitCount = 0;
    for (int bit = 0; bit < 9; bit++) if (fused.traits & (1u << bit)) traitCount++;
    FUSION_CHECK(traitCount <= FUSION_MAX_TRAITS, "il fuso porta piu' trait del tetto dichiarato");
    FUSION_CHECK((fused.traits & (TRAIT_PIERCE | TRAIT_SLOW)) != 0u, "il fuso non ha ereditato alcun trait dai genitori");

    /* Il gradino di rarita' si ferma al leggendario: mai un quinto livello. */
    Item legendary[2] = {
        FusionTestItem("Corona", ITEM_PASSIVE, RARITY_LEGENDARY, TRAIT_HOMING, SLOT_HAT),
        FusionTestItem("Scaglia", ITEM_PASSIVE, RARITY_LEGENDARY, TRAIT_GIANT, SLOT_BODY)
    };
    Item legendaryFused;
    FusionTestSetup(game, 778u, legendary, 2, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, &legendaryFused) == FUSION_OK, "fusione fra leggendari rifiutata");
    FUSION_CHECK(legendaryFused.rarity == RARITY_LEGENDARY, "la rarita' del fuso ha sfondato il leggendario");

    printf("  fusione (c) DEC-162: rarita' %d -> %d | potenza del colpo %.2f in [%.2f, %.2f] | trait %d <= %d\n",
           (int)RARITY_UNCOMMON, (int)fused.rarity, (double)power,
           (double)SHOT_TYPE_FUSION_POWER_MIN, (double)SHOT_TYPE_FUSION_POWER_MAX,
           traitCount, FUSION_MAX_TRAITS);
    return true;
}

/* (d) Consumo e inventario: i due sorgenti e un catalizzatore spariscono, il
   fuso entra, e un tentativo non valido non consuma NULLA. */
static bool FusionTestConsumption(Game *game)
{
    Item three[3] = {
        FusionTestItem("Uno", ITEM_PASSIVE, RARITY_COMMON, TRAIT_BOUNCE, SLOT_HAT),
        FusionTestItem("Due", ITEM_PASSIVE, RARITY_UNCOMMON, TRAIT_SPLIT, SLOT_HAND),
        FusionTestItem("Tre", ITEM_PASSIVE, RARITY_COMMON, TRAIT_RAPID, SLOT_BACK)
    };
    Item fused;
    FusionTestSetup(game, 99u, three, 3, 2);
    FUSION_CHECK(FusionPerform(game, 0, 1, &fused) == FUSION_OK, "fusione valida rifiutata");
    FUSION_CHECK(game->player.itemCount == 2, "l'inventario non e' passato da 3 a 2 oggetti");
    FUSION_CHECK(game->player.flux == 1, "il catalizzatore non e' stato consumato (o ne e' sparito piu' di uno)");
    FUSION_CHECK(strcmp(game->player.items[0].name, "Tre") == 0, "l'oggetto non coinvolto non e' rimasto in inventario");
    FUSION_CHECK(strcmp(game->player.items[1].name, fused.name) == 0, "il fuso non e' entrato in inventario");
    FUSION_CHECK(strcmp(game->player.items[1].fusedFrom[0], "Uno") == 0 &&
                 strcmp(game->player.items[1].fusedFrom[1], "Due") == 0,
                 "il fuso non dichiara i due genitori da cui deriva");

    /* Senza catalizzatore non succede nulla (scenario "catalizzatore
       mancante"): stesso inventario, stesso Flux. */
    FusionTestSetup(game, 99u, three, 3, 0);
    FUSION_CHECK(FusionPerform(game, 0, 1, NULL) == FUSION_ERR_NO_CATALYST, "senza Flux la fusione non e' stata rifiutata");
    FUSION_CHECK(game->player.itemCount == 3 && game->player.flux == 0, "una fusione rifiutata ha comunque consumato qualcosa");
    /* Stesso oggetto due volte, e un solo oggetto in inventario. */
    FusionTestSetup(game, 99u, three, 3, 1);
    FUSION_CHECK(FusionPerform(game, 1, 1, NULL) == FUSION_ERR_SAME_ITEM, "lo stesso oggetto due volte non e' stato rifiutato");
    FUSION_CHECK(game->player.itemCount == 3, "il rifiuto 'stesso oggetto' ha comunque toccato l'inventario");
    FusionTestSetup(game, 99u, three, 1, 1);
    FUSION_CHECK(FusionPerform(game, 0, 1, NULL) == FUSION_ERR_NEED_TWO, "con un solo oggetto la fusione non e' stata rifiutata");

    printf("  fusione (d) consumo: 3 oggetti + 2 Flux -> 2 oggetti + 1 Flux | rifiuti senza effetti collaterali: si\n");
    return true;
}

/* (e) DEC-171: l'immagine del fuso si pesca dal pacchetto curato e non si
   ripete MAI nella stessa run. (f) DEC-102: un fuso puo' tornare sorgente. */
static bool FusionTestImagesAndRefusion(Game *game)
{
    int manifestCount = CuratedImagesCount(CURATED_MANIFEST_PATH);
    Item stock[6];
    for (int i = 0; i < 6; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "Pezzo %d", i);
        stock[i] = FusionTestItem(name, ITEM_PASSIVE, (Rarity)(i%2), TRAIT_BOUNCE << (i%4), SLOT_HAND);
    }
    FusionTestSetup(game, 31337u, stock, 6, 5);

    char seen[3][64];
    int seenCount = 0;
    for (int i = 0; i < 3; i++)
    {
        Item fused;
        FUSION_CHECK(FusionPerform(game, 0, 1, &fused) == FUSION_OK, "una delle fusioni in catena e' stata rifiutata");
        if (fused.imagePath[0])
        {
            for (int j = 0; j < seenCount; j++)
                FUSION_CHECK(strcmp(seen[j], fused.imagePath) != 0, "la stessa immagine curata e' stata pescata due volte nella run (DEC-171)");
            snprintf(seen[seenCount++], sizeof(seen[0]), "%s", fused.imagePath);
        }
    }
    if (manifestCount > 0)
    {
        FUSION_CHECK(seenCount == 3, "il pacchetto curato c'e' ma qualche fusione e' rimasta senza immagine");
    }

    /* Ri-fusione (DEC-102): l'ultimo fuso e' in fondo all'inventario, si
       fonde di nuovo con un oggetto qualunque. */
    int count = game->player.itemCount;
    FUSION_CHECK(count >= 2, "dopo tre fusioni non restano abbastanza oggetti per la ri-fusione");
    char previousName[48];
    snprintf(previousName, sizeof(previousName), "%s", game->player.items[count - 1].name);
    FUSION_CHECK(game->player.items[count - 1].fusedFrom[0][0] != '\0', "l'ultimo oggetto non risulta nato da una fusione");
    Item refused;
    FUSION_CHECK(FusionPerform(game, count - 1, 0, &refused) == FUSION_OK, "la ri-fusione di un fuso e' stata rifiutata (DEC-102)");
    FUSION_CHECK(strcmp(refused.fusedFrom[0], previousName) == 0, "la ri-fusione non dichiara il fuso precedente come genitore");

    printf("  fusione (e/f) DEC-171/DEC-102: %d immagini nel pacchetto, %d fusioni con immagine distinta | ri-fusione: ok\n",
           manifestCount, seenCount);
    return true;
}

/* (g) Il cablaggio dell'interfaccia: TAB apre BuildScreen, INVIO seleziona i
   due oggetti a fuoco, F fonde. Tutto attraverso UpdateApp con AppInput
   sintetici, come --states-test: e' la prova che il flusso e' davvero
   raggiungibile dal giocatore, non solo dalle API. */
static bool FusionTestBuildScreenFlow(Game *game)
{
    Item pair[2] = {
        FusionTestItem("Manico Torto", ITEM_PASSIVE, RARITY_COMMON, TRAIT_BOUNCE, SLOT_HAND),
        FusionTestItem("Lente Rotta", ITEM_PASSIVE, RARITY_UNCOMMON, TRAIT_HOMING, SLOT_EYES)
    };
    FusionTestSetup(game, 5150u, pair, 2, 1);

    AppMode mode = APP_GAMEPLAY;
    AppGen gen = { 0 };
    AppUi ui = { 0 };
    { AppInput in = InputTab(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(mode == APP_BUILD_SCREEN, "TAB non ha aperto BuildScreen");
    FUSION_CHECK(ui.fusionSourceA == FUSION_UI_NONE && ui.fusionSourceB == FUSION_UI_NONE,
                 "entrando in BuildScreen la selezione di fusione non e' vuota");
    /* WP22 (ui/inventory-and-synergy-screen.md, "Focus iniziale"): con due
       oggetti in inventario, l'ingresso mette il fuoco sull'ULTIMO acquisito
       (indice 1, "Lente Rotta"), non sul primo -- vedi AppEnterBuildScreen. */
    FUSION_CHECK(ui.buildItemFocus == 1, "l'ingresso in BuildScreen non ha messo il fuoco sull'ultimo oggetto acquisito");

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(mode == APP_BUILD_SCREEN, "la conferma in BuildScreen ha chiuso la schermata invece di selezionare");
    FUSION_CHECK(FUSION_UI_SLOT(ui.fusionSourceA) == 1, "la conferma non ha selezionato l'oggetto a fuoco (l'ultimo acquisito)");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(FUSION_UI_SLOT(ui.fusionSourceB) == 0, "la seconda conferma non ha selezionato il secondo oggetto");

    { AppInput in = { 0 }; in.fuse = true; UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(mode == APP_BUILD_SCREEN, "F ha cambiato stato applicativo");
    FUSION_CHECK(game->player.itemCount == 1, "F non ha eseguito la fusione (inventario invariato)");
    FUSION_CHECK(game->player.flux == 0, "F non ha consumato il catalizzatore");
    FUSION_CHECK(ui.fusionResultName[0] != '\0', "l'esito della fusione non e' visibile nell'interfaccia");
    FUSION_CHECK(ui.fusionSourceA == FUSION_UI_NONE && ui.fusionSourceB == FUSION_UI_NONE,
                 "dopo la fusione la selezione punta ancora a slot ormai spariti");

    /* Deselezione: riscegliendo lo stesso oggetto lo slot si svuota. */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(FUSION_UI_SLOT(ui.fusionSourceA) == 0, "la selezione dopo la fusione non funziona piu'");
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(ui.fusionSourceA == FUSION_UI_NONE, "riconfermare lo stesso oggetto non lo deseleziona");

    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    FUSION_CHECK(mode == APP_GAMEPLAY, "ESC da BuildScreen non torna a Gameplay");

    printf("  fusione (g) BuildScreen: TAB -> INVIO x2 -> F -> fuso \"%s\" | uscita con ESC: ok\n", ui.fusionResultName);
    return true;
}

bool GameFusionTest(Game *game)
{
    if (!FusionTestDeterminism(game)) return false;
    if (!FusionTestDominantCategory(game)) return false;
    if (!FusionTestPowerBudget(game)) return false;
    if (!FusionTestConsumption(game)) return false;
    if (!FusionTestImagesAndRefusion(game)) return false;
    if (!FusionTestBuildScreenFlow(game)) return false;
    return true;
}

/* SOLO manuale (--fusion-screenshot-test, mai in make test, stessa tradizione
   di GameRarityScreenshotTest/GameRoomShapesScreenshotTest): mette in scena
   BuildScreen con la fascia FUSIONE viva -- due sorgenti selezionate, un
   catalizzatore in tasca e l'esito dell'ultima fusione col suo sprite curato
   (DEC-171) -- e salva logs/worldsmelt-fusion-screen.png. Nessun assert puo'
   dire se la fascia entra nel riquadro senza pestare le liste sopra o la
   riga "Indietro": quello si guarda. L'assert automatico resta "non va in
   crash e la texture e' valida". */
bool GameFusionScreenshotTest(Game *game)
{
    Item bench[4] = {
        FusionTestItem("Manico Torto", ITEM_PASSIVE, RARITY_COMMON, TRAIT_BOUNCE, SLOT_HAND),
        FusionTestItem("Lente Rotta", ITEM_PASSIVE, RARITY_UNCOMMON, TRAIT_HOMING, SLOT_EYES),
        FusionTestItem("Ala di Cenere", ITEM_GRAFT, RARITY_RARE, TRAIT_SLOW, SLOT_BACK),
        FusionTestShotItem("Chiodo Lungo", RARITY_LEGENDARY, TRAIT_PIERCE, 0)
    };
    FusionTestSetup(game, 20260727u, bench, 4, 2);

    AppUi ui = { 0 };
    Item fused;
    if (FusionPerform(game, 2, 3, &fused) == FUSION_OK)
    {
        snprintf(ui.fusionResultName, sizeof(ui.fusionResultName), "%s", fused.name);
        snprintf(ui.fusionResultImage, sizeof(ui.fusionResultImage), "%s", fused.imagePath);
        snprintf(ui.fusionMessage, sizeof(ui.fusionMessage), "Fuso: %.30s (da %.22s + %.22s).",
                 fused.name, fused.fusedFrom[0], fused.fusedFrom[1]);
    }
    /* Selezione viva sui due oggetti rimasti, cosi' lo scatto mostra anche i
       marcatori di riga e gli slot sorgente pieni. */
    ui.buildItemFocus = 1;
    ui.fusionSourceA = FUSION_UI_FIELD(0);
    ui.fusionSourceB = FUSION_UI_FIELD(1);

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, APP_BUILD_SCREEN, &ui, true, NULL, "logs/worldsmelt-fusion-screen.png");
    bool textureValid = canvas.texture.id != 0;
    UnloadRenderTexture(canvas);
    return textureValid;
}
