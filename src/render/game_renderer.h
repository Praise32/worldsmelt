#ifndef MELTING_RUN_GAME_RENDERER_H
#define MELTING_RUN_GAME_RENDERER_H

#include "core/game_types.h"

UiLayout UiComputeLayout(void);
bool UiScreenToGameMouse(UiLayout layout, Vector2 *out);
/* screenshotPath e' usato SOLO se takeScreenshot e' vero (vedi
   game_renderer.c): il chiamante decide dove finisce il frame catturato.
   app.c passa sempre "logs/melting-run-screen.png" per --screenshot-test
   (invariato); i test interni (src/tests/game_tests.c) possono passare un
   percorso diverso per non toccare quel file. */
void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot, const GenProgress *genProgress, const char *screenshotPath);

#endif
