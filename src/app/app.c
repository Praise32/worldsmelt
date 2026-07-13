#include "app/app.h"

#include "game/game.h"
#include "render/game_renderer.h"
#include "tests/game_tests.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>

static bool UpdateApp(Game *game, AppMode *mode, UiLayout layout, float dt)
{
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    if (*mode == APP_MENU)
    {
        if (IsKeyPressed(KEY_Q)) return true;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            GameResetRun(game);
            *mode = APP_PLAY;
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
            GameResetRun(game);
            *mode = APP_PLAY;
        }
        if (IsKeyPressed(KEY_M)) *mode = APP_MENU;
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
        if (UpdateApp(&game, &appMode, layout, dt)) break;
        RendererDrawApp(&game, gameCanvas, appMode, screenshotTest && !screenshotDone);
        if (screenshotTest && !screenshotDone) screenshotDone = true;
        if (frames > 0)
        {
            frames--;
            if (frames == 0) break;
        }
    }

    UnloadRenderTexture(gameCanvas);
    GameUnloadAssets(&game);
    CloseWindow();
    return 0;
}
