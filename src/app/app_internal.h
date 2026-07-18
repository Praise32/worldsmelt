#ifndef MELTING_RUN_APP_INTERNAL_H
#define MELTING_RUN_APP_INTERNAL_H

#include "core/game_types.h"
#include "gen/gen_runner.h"

/* Header INTERNO del modulo app (M1a): AppInput e AppGen non servono a
   nessun altro modulo (a differenza di AppMode/AppUi, che src/render deve
   leggere per disegnare -- vedi il commento su AppUi in core/game_types.h).
   Vive fuori da app.c solo perche' src/tests/game_tests.c deve costruire
   AppInput sintetici e chiamare UpdateApp direttamente per --states-test
   (mai IsKeyPressed dentro un test: gli eventi vanno scritti a mano). */

/* Eventi di UN frame di finestra, raccolti UNA SOLA volta (AppInputCollect,
   in app.c, e' l'unico punto che chiama IsKeyPressed per queste chiavi):
   IsKeyPressed e' per definizione un evento per-frame, quindi rileggerlo in
   piu' punti dello stesso frame perderebbe o raddoppierebbe la pressione a
   seconda dell'ordine delle chiamate. F11/ToggleFullscreen resta FUORI di
   proposito (gestito globalmente in UpdateApp, invariato dal comportamento
   pre-M1a): non e' un evento di navigazione fra stati. */
typedef struct AppInput {
    bool confirm;   /* ENTER o SPACE */
    bool back;      /* ESC */
    bool pause;     /* P */
    bool up;
    bool down;
    bool tab;
    bool reroll;    /* R */
    bool quit;      /* Q */
    bool bomb;      /* SPACE (si', lo stesso tasto di confirm: in Gameplay non c'e' ambiguita', vedi UpdateApp) */
} AppInput;

/* Contesto della generazione in-game (invariato da prima di M1a, solo spostato
 * qui perche' src/tests/game_tests.c deve poterne costruire uno per
 * --states-test): se abilitata (flag --generate), il gioco avvia due processi
 * esterni IN SEQUENZA (melting-gen per il testo, poi melting-sprites per gli
 * sprite) invece di limitarsi a rileggere il manifest gia' su disco. I due
 * restano processi separati (due ggml incompatibili, VRAM che non basta per
 * entrambi i modelli insieme) e src/gen non sa nulla del fatto che ce ne
 * siano due: l'orchestrazione a due passi vive qui, in src/app, che e' il
 * modulo che possiede le modalita' applicative (vedi AGENTS.md). */
typedef struct AppGen {
    bool enabled;
    bool noSprites;              /* --no-sprites: salta sempre il passo sprite */
    /* --low-spec: preset MANUALE per hardware sotto la scheda di riferimento
       (5600 XT, che resta il default: senza questo flag non cambia nulla).
       Qui si limita a passare due argomenti in piu' ai processi figli (vedi
       AppStartGeneration/AppStartSpritesGeneration in app.c): NESSUN
       benchmark o rilevamento automatico del tier hardware, quello arrivera'
       dopo. */
    bool lowSpec;
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
    /* Step B2 (generazione pigra dei piani, roadmap punto 2): il passo testo
       genera gli script Lua del SOLO piano 1 (--lua-first 1), cosi' la run parte
       dopo 4 script invece di 20. I 16 restanti li scrive QUESTO processo, avviato
       nel momento in cui la partita comincia e lasciato correre in sottofondo
       mentre si gioca (--resume: rilegge la run dal JSON gia' su disco, genera solo
       cio' che manca, e ripubblica il manifest dopo ogni piano). Il gioco raccoglie
       gli script di un piano quando ci entra (WorldStartFloor ->
       RunContentRefreshFloorScripts): si esplora il piano 1 per minuti, il tempo
       abbondante perche' i piani successivi siano pronti prima di arrivarci.
       Non gira MAI insieme a melting-sprites (i due modelli non stanno insieme
       nella VRAM della scheda di riferimento): parte solo DOPO che il passo
       sprite e' finito, cioe' quando si entra in gioco. */
    GenRunner lazyRunner;
    bool lazyRunning;
    unsigned int lastGenSeed;   /* il seed della generazione in corso: la ripresa DEVE usare lo stesso, o ricostruirebbe un'altra run */
} AppGen;

/* La macchina a stati canonica (9 stati, ui/navigation-map.md): un case per
   ciascun AppMode, "default" volutamente assente (dimenticare uno stato deve
   essere un -Wswitch, non un bug silenzioso -- vedi il commento su AppMode).
   Ritorna true SOLO quando il gioco deve chiudersi (ExitConfirm/Conferma con
   contesto "uscita dal gioco", l'unico modo per uscire: niente piu' scorciatoie
   dirette come il vecchio APP_MENU/Q). 'input' e' gia' raccolto UNA volta per
   frame dal chiamante (AppInputCollect in app.c) -- UpdateApp stesso non
   chiama mai IsKeyPressed, cosi' src/tests/game_tests.c puo' esercitare ogni
   transizione con eventi sintetici, senza finestra ne' tastiera vera
   (--states-test). Il click del mouse sulle voci di menu (DEC-057: ammesso
   nei menu, tastiera via primaria) e' un'eccezione dichiarata: UpdateApp lo
   legge da solo con IsMouseButtonPressed/GetMousePosition e interroga
   RendererMenuItemAt (src/render, fonte unica della geometria delle voci) --
   non e' una "chiave" delle nove sopra, e sotto test (mouse mai premuto in
   Xvfb) non interferisce mai con gli scenari sintetici. */
bool UpdateApp(Game *game, AppMode *mode, AppGen *gen, AppUi *ui, const AppInput *input);

#endif
