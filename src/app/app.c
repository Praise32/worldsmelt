#include "app/app.h"

#include "game/game.h"
#include "game/game_internal.h"
#include "gen/gen_runner.h"
#include "render/game_renderer.h"
#include "tests/game_tests.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Contesto della generazione in-game: se abilitata (flag --generate), il
 * gioco avvia due processi esterni IN SEQUENZA (melting-gen per il testo,
 * poi melting-sprites per gli sprite) invece di limitarsi a rileggere il
 * manifest gia' su disco. I due restano processi separati (vedi la spec,
 * sezione 3: due ggml incompatibili, VRAM che non basta per entrambi i
 * modelli insieme) e src/gen non sa nulla del fatto che ce ne siano due:
 * l'orchestrazione a due passi vive qui, in src/app, che e' il modulo che
 * possiede le modalita' applicative (vedi AGENTS.md). */
typedef struct AppGen {
    bool enabled;
    bool noSprites;              /* --no-sprites: salta sempre il passo sprite */
    const char *command;         /* melting-gen (passo 1: testo) */
    const char *spritesCommand;  /* melting-sprites (passo 2: sprite) */
    GenRunner runner;            /* passo 1 */
    GenRunner spritesRunner;     /* passo 2 */
    bool inSpritesStage;         /* quale dei due passi e' quello attivo ora */
    /* Deciso all'avvio della generazione (AppStartGeneration), non ad ogni
       frame: se cambiasse a meta' generazione la barra di progresso (che
       mappa il passo testo su 0-50% o 0-100% a seconda di questo flag)
       salterebbe in modo visibile. */
    bool spritesPlannedThisRun;
} AppGen;

/* Stessi percorsi di default dei modelli SD di tools/melting-sprites/main.c
 * (ParseArgs): il gioco non linka stable-diffusion.cpp (vedi AGENTS.md), si
 * limita a controllare che i file esistano. Se mancano, o non sono validi,
 * melting-sprites stesso ripiega su --dry-run senza mai andare in crash: qui
 * serve solo a decidere se vale la pena avviare il secondo passo. */
static bool SpritesModelsPresent(void)
{
    return FileExists("models/Public-Prompts-Pixel-Model.ckpt") &&
           FileExists("models/lcm-lora-sdv1-5.safetensors");
}

/* time(NULL) da solo ha risoluzione di un secondo: premere R due volte nello
 * stesso secondo produceva lo stesso seed e quindi una run identica. Si
 * mescola clock() (risoluzione sub-secondo, e portabile: ISO C, disponibile
 * sia su Linux sia su Windows/MinGW senza bisogno di header specifici della
 * piattaforma), un contatore di chiamate che garantisce unicita' anche se il
 * clock non avanzasse a sufficienza tra due pressioni ravvicinate, e un sale
 * diverso per i due passi cosi' non condividono mai lo stesso seed. */
static unsigned int NextGenSeed(unsigned int salt)
{
    static unsigned int callCount = 0;
    callCount++;
    return (unsigned int)time(NULL) ^ (unsigned int)clock() ^ (callCount * 2654435761u) ^ salt;
}

static bool AppStartGeneration(AppGen *gen)
{
    gen->inSpritesStage = false;
    gen->spritesPlannedThisRun = !gen->noSprites && SpritesModelsPresent();
    unsigned int seed = NextGenSeed(0u);
    /* 420s, non piu' 180s: da fase 3a-L3 melting-gen non genera solo il JSON
     * dei piani, ma anche (con lo stesso modello gia' caricato) fino a 15
     * script Lua per run, ciascuno con fino a 2 ritenti (vedi
     * tools/melting-gen/main.c e gen_lua.c). melting-gen ha il suo stesso
     * budget interno, piu' stretto (GEN_LUA_PHASE_BUDGET_SEC=300s assoluti
     * dall'avvio del processo, in tools/melting-gen/melting_gen.h): oltre
     * quella soglia smette di tentare nuovi script (gli oggetti restanti
     * restano sulla mini-VM) e scrive comunque la run. I 420s qui sono il
     * tetto ESTERNO, solo per il caso patologico in cui anche quel budget
     * interno non bastasse a lasciare il tempo di scrivere manifest/atlas. */
    return GenRunnerStart(&gen->runner, gen->command, seed, 420.0, "generated/gen_progress.txt");
}

static bool AppStartSpritesGeneration(AppGen *gen)
{
    unsigned int seed = NextGenSeed(0x5F3759DFu);
    return GenRunnerStart(&gen->spritesRunner, gen->spritesCommand, seed, 240.0, "generated/gen_progress.txt");
}

/* Combina il progresso del passo attivo in un'unica barra continua 0-100%:
 * col passo sprite pianificato, il testo occupa 0-50% e gli sprite 50-100%;
 * senza passo sprite (modelli assenti, --no-sprites, o passo testo mai
 * partito) il testo da solo occupa l'intera barra, cosi' arriva comunque al
 * 100% invece di fermarsi a meta' in modo ingannevole. */
static GenProgress AppCombinedProgress(const AppGen *gen)
{
    const GenRunner *active = gen->inSpritesStage ? &gen->spritesRunner : &gen->runner;
    GenProgress combined = active->progress;
    if (gen->spritesPlannedThisRun)
    {
        combined.percent = gen->inSpritesStage ? 50 + active->progress.percent/2 : active->progress.percent/2;
    }
    return combined;
}

static bool UpdateApp(Game *game, AppMode *mode, UiLayout layout, float dt, AppGen *gen)
{
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    if (*mode == APP_MENU)
    {
        if (IsKeyPressed(KEY_Q)) return true;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            if (gen->enabled && AppStartGeneration(gen)) *mode = APP_GENERATING;
            else
            {
                GameResetRun(game);
                if (gen->enabled) GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
                *mode = APP_PLAY;
            }
        }
        GameUpdateParticles(game, dt);
        return false;
    }

    if (*mode == APP_PAUSE)
    {
        if (IsKeyPressed(KEY_Q)) return true;
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) *mode = APP_PLAY;
        if (IsKeyPressed(KEY_R))
        {
            if (gen->enabled && AppStartGeneration(gen)) *mode = APP_GENERATING;
            else
            {
                GameResetRun(game);
                if (gen->enabled) GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
                *mode = APP_PLAY;
            }
        }
        if (IsKeyPressed(KEY_M)) *mode = APP_MENU;
        GameUpdateParticles(game, dt);
        return false;
    }

    if (*mode == APP_GENERATING)
    {
        GenRunner *active = gen->inSpritesStage ? &gen->spritesRunner : &gen->runner;
        GenRunnerUpdate(active);
        if (IsKeyPressed(KEY_ESCAPE))
        {
            GenRunnerCancel(active);
            *mode = APP_MENU;
            return false;
        }
        if (active->state == GEN_RUNNER_SUCCEEDED)
        {
            if (!gen->inSpritesStage && gen->spritesPlannedThisRun)
            {
                /* Passo testo riuscito e modelli SD presenti: si passa al
                   passo sprite invece di entrare subito in gioco. L'overlay
                   resta a schermo, il progresso continua sulla stessa barra
                   (vedi AppCombinedProgress). */
                if (AppStartSpritesGeneration(gen)) gen->inSpritesStage = true;
                else
                {
                    /* fork() fallita (rarissimo): si gioca comunque con
                       l'atlas BMP gia' scritto dal passo testo, mai bloccare
                       la run per un secondo passo che non e' nemmeno partito. */
                    GameResetRun(game);
                    GameSetMessage(game, "Sprite non avviati: si gioca con l'atlas di riserva");
                    *mode = APP_PLAY;
                }
            }
            else
            {
                GameResetRun(game);
                if (gen->inSpritesStage) GameSetMessage(game, "Sprite generati: run pronta");
                *mode = APP_PLAY;
            }
        }
        else if (active->state == GEN_RUNNER_FAILED)
        {
            /* Il passo testo o quello sprite e' fallito o e' andato in
               timeout (GenRunnerUpdate cancella e marca FAILED da sola): si
               gioca comunque, con qualunque atlas sia gia' su disco (vedi
               RunContentLoad/PreferPngAtlasIfFresh: mai una scrittura a
               meta'). */
            GameResetRun(game);
            GameSetMessage(game, gen->inSpritesStage
                ? "Sprite generati saltati: si gioca con l'atlas di riserva"
                : "Generazione fallita: uso i contenuti di riserva");
            *mode = APP_PLAY;
        }
        GameUpdateParticles(game, dt);
        return false;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
        *mode = APP_PAUSE;
        return false;
    }
    if (IsKeyPressed(KEY_M))
    {
        *mode = APP_MENU;
        return false;
    }

    if (gen->enabled && IsKeyPressed(KEY_R))
    {
        if (AppStartGeneration(gen)) *mode = APP_GENERATING;
        else
        {
            GameResetRun(game);
            GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
        }
        return false;
    }

    Vector2 mouseGame = { 0.0f, 0.0f };
    bool mouseInsideGame = UiScreenToGameMouse(layout, &mouseGame);
    GameUpdate(game, dt, mouseGame, mouseInsideGame);
    return false;
}

int AppRun(int argc, char **argv)
{
    bool smokeTest = false;
    bool screenshotTest = false;
    bool menuScreenshotTest = false;
    bool portalTest = false;
    bool scriptTest = false;
    bool manifestTest = false;
    bool genTest = false;
    bool atlasFallbackTest = false;
    bool layerTest = false;
    bool scriptSandboxTest = false;
    bool scriptDeterminismTest = false;
    bool scriptItemsTest = false;
    unsigned int scriptSeed = 12345u;
    AppGen gen = { 0 };
    gen.command = "bin/melting-gen";
    gen.spritesCommand = "bin/melting-sprites";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--smoke-test") == 0) smokeTest = true;
        if (strcmp(argv[i], "--portal-test") == 0)
        {
            smokeTest = true;
            portalTest = true;
        }
        if (strcmp(argv[i], "--script-test") == 0)
        {
            smokeTest = true;
            scriptTest = true;
        }
        if (strcmp(argv[i], "--manifest-test") == 0)
        {
            smokeTest = true;
            manifestTest = true;
        }
        if (strcmp(argv[i], "--atlas-fallback-test") == 0)
        {
            smokeTest = true;
            atlasFallbackTest = true;
        }
        /* Personaggio a strati (fase 3): come --atlas-fallback-test, disegna
           un frame vero (serve la finestra/GL, quindi Xvfb in make test) per
           verificare che BuildItemLayers/DrawItemLayer non vadano in crash
           con un giocatore equipaggiato a fondo. Vedi GameLayerTest. */
        if (strcmp(argv[i], "--layer-test") == 0)
        {
            smokeTest = true;
            layerTest = true;
        }
        if (strcmp(argv[i], "--screenshot-test") == 0)
        {
            smokeTest = true;
            screenshotTest = true;
        }
        if (strcmp(argv[i], "--menu-screenshot-test") == 0)
        {
            smokeTest = true;
            screenshotTest = true;
            menuScreenshotTest = true;
        }
        if (strcmp(argv[i], "--gen-test") == 0) genTest = true;
        if (strcmp(argv[i], "--script-sandbox-test") == 0) scriptSandboxTest = true;
        if (strcmp(argv[i], "--script-determinism-test") == 0) scriptDeterminismTest = true;
        if (strcmp(argv[i], "--script-items-test") == 0) scriptItemsTest = true;
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) scriptSeed = (unsigned int)strtoul(argv[++i], NULL, 10);
        if (strcmp(argv[i], "--generate") == 0) gen.enabled = true;
        if (strcmp(argv[i], "--no-sprites") == 0) gen.noSprites = true;
        if (strcmp(argv[i], "--gen-cmd") == 0 && i + 1 < argc) gen.command = argv[++i];
        if (strcmp(argv[i], "--sprites-cmd") == 0 && i + 1 < argc) gen.spritesCommand = argv[++i];
    }

    if (genTest)
    {
        bool ok = GenRunnerSelfTest();
        printf("Gen runner test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 6;
    }

    /* Come --gen-test: la sandbox Lua (src/script/script_sandbox.c) non
       tocca raylib in nessun modo, quindi non serve nessuna finestra ne'
       Xvfb per questi due (vedi scripts/test-script.sh, che infatti li
       lancia senza il wrapper xvfb-run usato altrove in questo file). */
    if (scriptSandboxTest)
    {
        bool ok = ScriptSandboxSelfTest();
        printf("Lua sandbox test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 8;
    }
    if (scriptDeterminismTest)
    {
        char out[512];
        bool ok = ScriptSandboxDeterminismProbe(scriptSeed, out, sizeof(out));
        printf("%s\n", out);
        return ok ? 0 : 9;
    }
    /* Come sopra: l'API di gioco a handle (src/script/script_api.c) e le
       callback degli oggetti (src/script/script_items.c) non toccano mai
       raylib direttamente (i loro test costruiscono un Game minimo sullo
       stack, vedi src/tests/script_items_tests.c), quindi anche questo flag
       gira prima di InitWindow. */
    if (scriptItemsTest)
    {
        bool ok = ScriptItemsSelfTest();
        printf("Script items test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 10;
    }

    bool compactTestWindow = smokeTest && !screenshotTest;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(compactTestWindow ? SCREEN_WIDTH : APP_WINDOW_WIDTH, compactTestWindow ? SCREEN_HEIGHT : APP_WINDOW_HEIGHT, "Melting Run");
    SetExitKey(KEY_NULL);
    if (!smokeTest)
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }
    SetTargetFPS(60);

    Game game = { 0 };
    GameResetRun(&game);
    if (portalTest)
    {
        bool ok = GamePortalRespawnTest(&game);
        printf("Portal test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 2;
    }
    if (scriptTest)
    {
        bool ok = GameScriptSandboxTest(&game);
        printf("Script sandbox test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 3;
    }
    if (manifestTest)
    {
        bool ok = GameManifestTest(&game);
        printf("Manifest test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 5;
    }
    if (atlasFallbackTest)
    {
        bool ok = GameAtlasFallbackTest(&game);
        printf("Atlas fallback test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 7;
    }
    if (layerTest)
    {
        bool ok = GameLayerTest(&game);
        printf("Layer test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 11;
    }

    RenderTexture2D gameCanvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(gameCanvas.texture, TEXTURE_FILTER_BILINEAR);
    AppMode appMode = (smokeTest && !menuScreenshotTest) ? APP_PLAY : APP_MENU;
    int frames = smokeTest ? 10 : -1;
    bool screenshotDone = false;
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        UiLayout layout = UiComputeLayout();
        if (UpdateApp(&game, &appMode, layout, dt, &gen)) break;
        GenProgress combinedProgress = { 0 };
        if (appMode == APP_GENERATING) combinedProgress = AppCombinedProgress(&gen);
        RendererDrawApp(&game, gameCanvas, appMode, screenshotTest && !screenshotDone,
                        appMode == APP_GENERATING ? &combinedProgress : NULL, "logs/melting-run-screen.png");
        if (screenshotTest && !screenshotDone) screenshotDone = true;
        if (frames > 0)
        {
            frames--;
            if (frames == 0) break;
        }
    }

    /* Se il ciclo termina (finestra chiusa, o contatore di frame dello
     * smoke-test esaurito) mentre la generazione e' ancora in corso, il
     * processo figlio va cancellato qui: altrimenti melting-gen (fino a 3
     * minuti, modello da 7B) o melting-sprites (fino a 240s, Stable
     * Diffusion) restano a girare sulla GPU e poi scrivono in generated/, in
     * corsa con un rilancio del gioco. Solo uno dei due puo' essere davvero
     * RUNNING in un dato momento, ma non costa nulla controllarli entrambi. */
    if (gen.runner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.runner);
    if (gen.spritesRunner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.spritesRunner);

    UnloadRenderTexture(gameCanvas);
    GameUnloadAssets(&game);
    CloseWindow();
    return 0;
}
