#ifndef MELTING_RUN_GAME_RENDERER_H
#define MELTING_RUN_GAME_RENDERER_H

#include "core/game_types.h"

UiLayout UiComputeLayout(void);
bool UiScreenToGameMouse(UiLayout layout, Vector2 *out);
void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, bool takeScreenshot);

#endif
