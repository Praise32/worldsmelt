#ifndef MELTING_RUN_GAME_RENDERER_H
#define MELTING_RUN_GAME_RENDERER_H

#include "core/game_types.h"

#include <stddef.h>   /* size_t, per HudCrustLineFormat sotto */

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

/* WP22 (DEC-090, gap G9 ui-cornice): nucleo PURO (nessuna chiamata raylib) che
   decide se il dialogo ExitConfirm aperto da 'openedFrom' resta un dialogo
   modale LEGGERO (MainMenu ridisegnato sotto, velo di fondo attenuato) o a
   schermo pieno come sempre -- vero SOLO per APP_MAIN_MENU (chiusura del
   gioco); FloorZero e PauseMenu restano a schermo pieno, presentazione gia'
   documentata (DEC-090). Esposta cosi' DrawExitConfirmOverlay/RendererDrawApp
   (game_renderer.c) e --layout-test (src/app/app.c) condividono la STESSA
   regola, mai due copie che potrebbero divergere. */
bool ExitConfirmIsLightModalFor(AppMode openedFrom);

/* Zona cliccabile della voce di menu all'indice restituito, per lo stato
   'mode', nella STESSA geometria che RendererDrawApp usa per disegnarla
   (game_renderer.c: un'unica sorgente per le due cose, mai duplicata in
   src/app). Il mouse e' ammesso nei menu (DEC-057, la tastiera resta la via
   primaria): src/app/app.c la interroga dentro UpdateApp per tradurre un
   click in un confirm sintetico sulla voce sotto il puntatore. Ritorna -1 se
   il punto non e' su nessuna voce, o se 'mode' non ha un overlay di menu
   (FloorZero, Gameplay).
   'exitConfirmLight' (WP22, terza passata) e' il contesto che serve alla sola
   geometria di APP_EXIT_CONFIRM: il dialogo leggero "MainMenu -> Esci" ha un
   riquadro piu' stretto degli altri tre contesti (vedi MenuBoxForModeFor in
   game_renderer.c), quindi anche voci piu' strette. Chi chiama passa
   ExitConfirmIsLightModalFor(ui->openedFrom); 'false' -- il valore piu'
   innocuo, cioe' la geometria a schermo pieno di sempre -- va bene per ogni
   altro 'mode', che lo ignora. */
int RendererMenuItemAt(AppMode mode, Vector2 mouse, bool exitConfirmLight);

/* WP22 (terza passata, ui/run-setup.md): etichetta e fascia occupata dalla
   riga informativa "Modalita': Standard" di RunSetup -- fonte UNICA condivisa
   fra DrawRunSetupOverlay (che ci disegna il testo) e i test
   (GameRunSetupModeLineTest a pixel su un frame vero, UiLayoutSelfTest voce
   'h' come nucleo puro). La riga NON e' una voce di menu: non ha indice,
   RendererMenuItemAt non deve mai rispondere per un punto dentro questa
   fascia, e la fascia non tocca nessuna delle tre voci selezionabili. */
const char *RendererRunSetupModeLabel(void);
Rectangle RendererRunSetupModeLabelBandFor(float sw, float sh);

/* W9 (playtest round 1, "mouse ovunque"): il resto delle geometrie che
   RendererMenuItemAt sopra non copre, stesso principio -- fonte UNICA sia per
   disegnare sia per il hit-test, mai duplicata in src/app.
   - RendererBuildItemRowAt: indice in Player.items[] sotto 'mouse' dentro la
     lista OGGETTI PRESI di BuildScreen (finestra scorrevole: solo le righe
     DAVVERO disegnate in questo momento, con l'ancora 'ui->buildItemScroll'
     corrente, sono cliccabili), o -1. W9 correzione round 1: la finestra
     dipende dall'ANCORA, non dal focus -- cosi' questa mappatura non dipende
     dal campo che l'hover del mouse scrive (niente anello di retroazione, vedi
     il commento su AppUi.buildItemScroll in game_types.h).
   - RendererBuildItemRowsVisible: quante righe stanno nella finestra visibile
     con la geometria corrente (>= 1) -- il solo dato che src/app/app.c deve
     sapere per tenere l'ancora allineata al focus.
   - RendererFusionConfirmAt: vero se 'mouse' e' sulla riga di stato/azione
     della fascia FUSIONE ("fondi ...", o il motivo per cui non si puo'): un
     click qui vale [F].
   - RendererFloorZeroCardAt: indice di carta sotto 'mouse' nella sezione
     ATTIVA (game->floorZeroPanelSection) del pannello combinato del Piano 0
     (DEC-075), o -1 se il pannello e' chiuso o il punto non cade su nessuna
     carta.
   - RendererFloorZeroSectionTabAt: 0 (MONDI) o 1 (PERSONAGGI) sotto 'mouse',
     o -1 (pannello chiuso o punto fuori dalle due schedine).
   - RendererFloorZeroHintChipAt: vero se 'mouse' e' sul fumetto "TAB -- mondo
     e personaggio" mostrato a pannello chiuso (un click qui lo apre, come
     TAB); sempre falso a pannello gia' aperto.
   - RendererOptionsSliderValueAt: valore 0..1 corrispondente alla posizione
     orizzontale 'mouseX' dentro la barra della riga-slider 'index' (0..2:
     generale/musica/effetti) di Options, clampato ai due estremi -- usata da
     UpdateApp per il trascinamento col mouse.
   - RendererOptionsSliderHit: vero se 'mouse' cade sulla barra della
     riga-slider 'index' (piu' un piccolo margine di presa). E' il cancello
     del PRESS: il trascinamento si apre SOLO da qui, mai da un click
     sull'etichetta o sulle frecce-promemoria della riga (che valgono solo
     come navigazione). */
int RendererBuildItemRowAt(Game *game, const AppUi *ui, Vector2 mouse);
int RendererBuildItemRowsVisible(Game *game);
bool RendererFusionConfirmAt(Game *game, Vector2 mouse);
int RendererFloorZeroCardAt(const Game *game, Vector2 mouse);
int RendererFloorZeroSectionTabAt(const Game *game, Vector2 mouse);
bool RendererFloorZeroHintChipAt(const Game *game, Vector2 mouse);
float RendererOptionsSliderValueAt(int index, float mouseX);
bool RendererOptionsSliderHit(int index, Vector2 mouse);

/* Self-test delle geometrie sopra, dopo InitWindow (--mouse-hit-test in
   src/app/app.c): a differenza di UiLayoutSelfTest (puramente matematico,
   PRIMA della finestra) queste funzioni hanno bisogno del font di default di
   raylib gia' caricato (BuildScreenItemListLayoutFor misura con DrawBuildBlock,
   che chiama UiTextW/MeasureText). 'game' e' quello gia' pronto passato da
   AppRun (GameResetRun gia' chiamata), stesso schema di GameRoomsTest. */
bool RendererMouseHitTestSelfTest(Game *game);

/* DEC-169 (ui/hud.md, systems/floor-zero.md): l'HUD di combattimento si
   disegna in Gameplay SEMPRE, e nel Piano 0 SOLO durante una prova (arena di
   sfida/tutorial integrato, DEC-004/047) -- 'floorZeroTrialActive' e' il gap
   esplicito di core/game_types.h Game.floorZeroTrialActive: nessun codice lo
   imposta ancora a vero (le prove non esistono nel motore), quindi oggi
   questa funzione ritorna sempre false per APP_FLOOR_ZERO, esattamente il
   comportamento attuale (nascosto fuori da Gameplay). Nucleo PURO (nessuna
   chiamata raylib): testabile direttamente, stesso stile di
   UiComputeLayoutFor sopra. */
bool HudCombatShouldDraw(AppMode mode, bool floorZeroTrialActive);

/* Salute temporanea/protettiva (Crust, DEC-008, WP2): tre nuclei PURI (nessuna
   chiamata raylib) dietro il contatore Crust dell'HUD, stesso stile di
   HudCombatShouldDraw sopra -- testabili direttamente senza finestra aperta
   (--temp-health-test, GameTempHealthTest, src/tests/game_tests.c), cosi'
   nessuno dei due percorsi di disegno (layout V3 con icone, ripiego a testo
   senza pacchetto artistico) resta senza copertura. Vedi i commenti sulle
   definizioni in game_renderer.c per il dettaglio.
   - HudTempHeartsSlotCount: quante icone 'heart_temp' disegnare per un dato
     tempHp (1 icona per punto, mai 2 come i cuori base).
   - HudTempHeartsX: la X di partenza del contatore Crust nel layout V3, dato
     maxHp (da cui deriva l'ultimo slot di cuore base).
   - HudCrustLineFormat: vero se il ripiego a testo (senza pacchetto
     artistico) deve disegnare "+N", con 'buf' riempito di conseguenza. */
int HudTempHeartsSlotCount(int tempHp);
int HudTempHeartsX(int maxHp);
bool HudCrustLineFormat(int tempHp, char *buf, size_t bufSize);

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
