#include "app/app.h"

#include "game/game.h"
#include "game/game_internal.h"
#include "gen/gen_runner.h"
#include "render/game_renderer.h"
#include "tests/game_tests.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Contesto della generazione in-game: se abilitata (flag --generate), il
 * gioco avvia il processo esterno (melting-gen o un finto sostituto) invece
 * di limitarsi a rileggere il manifest gia' su disco. */
typedef struct AppGen {
    bool enabled;
    const char *command;
    GenRunner runner;
} AppGen;

static bool AppStartGeneration(AppGen *gen)
{
    /* time(NULL) da solo ha risoluzione di un secondo: premere R due volte
     * nello stesso secondo produceva lo stesso seed e quindi una run
     * identica. Si mescola clock() (risoluzione sub-secondo, e portabile:
     * ISO C, disponibile sia su Linux sia su Windows/MinGW senza bisogno di
     * header specifici della piattaforma) e un contatore di chiamate che
     * garantisce unicita' anche se il clock non avanzasse a sufficienza tra
     * due pressioni ravvicinate. */
    static unsigned int callCount = 0;
    callCount++;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)clock() ^ (callCount * 2654435761u);
    return GenRunnerStart(&gen->runner, gen->command, seed, 180.0, "generated/gen_progress.txt");
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
        GenRunnerUpdate(&gen->runner);
        if (IsKeyPressed(KEY_ESCAPE))
        {
            GenRunnerCancel(&gen->runner);
            *mode = APP_MENU;
            return false;
        }
        if (gen->runner.state == GEN_RUNNER_SUCCEEDED)
        {
            GameResetRun(game);
            *mode = APP_PLAY;
        }
        else if (gen->runner.state == GEN_RUNNER_FAILED)
        {
            GameResetRun(game);
            GameSetMessage(game, "Generazione fallita: uso i contenuti di riserva");
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
    AppGen gen = { 0 };
    gen.command = "bin/melting-gen";
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
        if (strcmp(argv[i], "--generate") == 0) gen.enabled = true;
        if (strcmp(argv[i], "--gen-cmd") == 0 && i + 1 < argc) gen.command = argv[++i];
    }

    if (genTest)
    {
        bool ok = GenRunnerSelfTest();
        printf("Gen runner test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 6;
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
        RendererDrawApp(&game, gameCanvas, appMode, screenshotTest && !screenshotDone,
                        appMode == APP_GENERATING ? &gen.runner.progress : NULL);
        if (screenshotTest && !screenshotDone) screenshotDone = true;
        if (frames > 0)
        {
            frames--;
            if (frames == 0) break;
        }
    }

    /* Se il ciclo termina (finestra chiusa, o contatore di frame dello
     * smoke-test esaurito) mentre la generazione e' ancora in corso, il
     * processo figlio va cancellato qui: altrimenti melting-gen resta a
     * girare fino a 3 minuti con un modello da 7B sulla GPU e poi scrive in
     * generated/, in corsa con un rilancio del gioco. */
    if (gen.runner.state == GEN_RUNNER_RUNNING) GenRunnerCancel(&gen.runner);

    UnloadRenderTexture(gameCanvas);
    GameUnloadAssets(&game);
    CloseWindow();
    return 0;
}
