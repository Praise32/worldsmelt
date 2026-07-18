#ifndef MELTING_RUN_GAME_RENDERER_H
#define MELTING_RUN_GAME_RENDERER_H

#include "core/game_types.h"

UiLayout UiComputeLayout(void);
bool UiScreenToGameMouse(UiLayout layout, Vector2 *out);

/* Zona cliccabile della voce di menu all'indice restituito, per lo stato
   'mode', nella STESSA geometria che RendererDrawApp usa per disegnarla
   (game_renderer.c: un'unica sorgente per le due cose, mai duplicata in
   src/app). Il mouse e' ammesso nei menu (DEC-057, la tastiera resta la via
   primaria): src/app/app.c la interroga dentro UpdateApp per tradurre un
   click in un confirm sintetico sulla voce sotto il puntatore. Ritorna -1 se
   il punto non e' su nessuna voce, o se 'mode' non ha un overlay di menu
   (FloorZero, Gameplay). */
int RendererMenuItemAt(AppMode mode, Vector2 mouse);

/* 'ui' e' letto per disegnare la voce col focus in evidenza e i contenuti
   propri di RunSetup (seed)/ExitConfirm (contesto): puo' essere NULL SOLO
   per gli stati senza overlay di menu (Gameplay, e FloorZero che disegna
   invece l'indicatore di generazione via genProgress) -- i test interni che
   disegnano solo la scena di gioco (src/tests/game_tests.c) passano sempre
   APP_GAMEPLAY e possono quindi passare NULL.
   screenshotPath e' usato SOLO se takeScreenshot e' vero: il chiamante
   decide dove finisce il frame catturato. app.c passa sempre
   "logs/melting-run-screen.png" per --screenshot-test (invariato); i test
   interni possono passare un percorso diverso per non toccare quel file. */
void RendererDrawApp(Game *game, RenderTexture2D canvas, AppMode mode, const AppUi *ui,
                     bool takeScreenshot, const GenProgress *genProgress, const char *screenshotPath);

#endif
