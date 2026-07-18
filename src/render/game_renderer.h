#ifndef MELTING_RUN_GAME_RENDERER_H
#define MELTING_RUN_GAME_RENDERER_H

#include "core/game_types.h"

UiLayout UiComputeLayout(void);
bool UiScreenToGameMouse(UiLayout layout, Vector2 *out);

/* M4: nucleo PURO di UiComputeLayout (nessuna chiamata raylib, solo matematica
   sulle struct) -- separato apposta perche' --layout-test (src/app/app.c) possa
   esercitarlo su risoluzioni sintetiche PRIMA di InitWindow, come --gen-test.
   UiComputeLayout(void) resta il wrapper che legge GetScreenWidth/Height e
   chiama questa. */
UiLayout UiComputeLayoutFor(float sw, float sh);

/* Suite di autotest pura di UiComputeLayoutFor/della geometria dei menu
   (MenuBoxForMode/MenuItemRect), per un set fisso di risoluzioni (720p..4K):
   nessuna sovrapposizione pannelli/canvas, scala del canvas massimale,
   non-regressione a 1600x900, monotonia al crescere della risoluzione, voci di
   menu dentro il box e non sovrapposte. Vedi --layout-test in src/app/app.c. */
bool UiLayoutSelfTest(void);

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
