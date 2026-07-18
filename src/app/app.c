#include "app/app.h"
#include "app/app_internal.h"

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

/* Lettura "chiave=valore" per riga: stesso schema/stessa reimplementazione
 * locale di ReadManifestValue in src/content/run_content.c e
 * tools/melting-sprites/sprite_manifest.c (vedi il commento li' sul perche'
 * non e' condivisa: moduli diversi, ognuno coi propri file da leggere -- qui
 * e' logs/benchmark.txt, vedi AppReadBenchmarkPreset sotto). */
static void ReadAppManifestValue(const char *text, const char *key, char *out, size_t outSize)
{
    out[0] = '\0';
    if (!text || !key || outSize == 0) return;
    const char *start = strstr(text, key);
    if (!start) return;
    start += strlen(key);
    size_t i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
}

void AppReadBenchmarkPreset(const char *path, bool manualLowSpec, bool manualFullSpec,
                             bool *lowSpecOut, char *msgOut, size_t msgCap)
{
    if (msgCap > 0) msgOut[0] = '\0';
    /* Override manuale: l'utente ha gia' scelto (--low-spec o --full-spec),
     * il benchmark si ignora del tutto -- ne' preset ne' messaggio. */
    if (manualLowSpec || manualFullSpec) return;

    char *text = LoadFileText(path);
    if (!text) return;   /* nessun benchmark.txt: comportamento di sempre */

    char schema[8] = { 0 };
    char tier[16] = { 0 };
    ReadAppManifestValue(text, "benchSchema=", schema, sizeof(schema));
    ReadAppManifestValue(text, "tier=", tier, sizeof(tier));
    UnloadFileText(text);

    /* benchSchema diverso da "1" (assente, o un formato futuro che non
     * riconosciamo): meglio non toccare nulla che fidarsi di un file che non
     * capiamo. */
    if (strcmp(schema, "1") != 0) return;

    if (strcmp(tier, "lowspec") == 0)
    {
        if (lowSpecOut) *lowSpecOut = true;
        snprintf(msgOut, msgCap, "preset low-spec dal benchmark");
    }
    else if (strcmp(tier, "unsupported") == 0)
    {
        /* NON blocca niente (il fallback procedurale esiste sempre): solo un
         * avviso, mai un impedimento a giocare. */
        snprintf(msgOut, msgCap, "Hardware sotto la soglia minima misurata: generazione IA lenta o assente, si gioca con i contenuti di riserva");
    }
    /* tier=full (o un valore sconosciuto): nessuna azione, nessun messaggio -- e' il comportamento di sempre. */
}

/* Modello LLM piccolo del preset --low-spec (vedi AppGen.lowSpec, ora in
 * app_internal.h: AppGen si e' spostato li' perche' src/tests/game_tests.c
 * deve poterne costruire uno per --states-test). Estratto in
 * una costante condivisa perche' DUE punti lo devono passare con lo stesso
 * valore: il passo bloccante (AppStartGeneration) e la ripresa in sottofondo
 * (AppStartLazyGeneration). La ripresa DEVE usare lo stesso modello del passo
 * bloccante -- stesso ragionamento del seed (vedi AppGen.lastGenSeed): se i due
 * divergessero, i 16 script Lua dei piani 2-5 uscirebbero da un modello diverso
 * dai 4 del piano 1, e la run cambierebbe contenuto a meta'; peggio, su hardware
 * sotto la scheda di riferimento (il caso d'uso del flag) caricare il 7B in
 * sottofondo mentre si gioca e' proprio il carico che --low-spec deve evitare. */
#define APP_LOW_SPEC_MODEL "models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf"

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

/* Step B2: il processo di ripresa in sottofondo. Va fermato PRIMA di avviare una
 * generazione nuova (due melting-gen insieme = due modelli da ~4.5 GiB nella
 * VRAM della scheda di riferimento: il secondo fallirebbe, o peggio farebbero
 * thrashing entrambi) e prima di uscire dal gioco. No-op se non sta girando. */
static void AppStopLazyGeneration(AppGen *gen)
{
    if (!gen->lazyRunning) return;
    GenRunnerCancel(&gen->lazyRunner);
    gen->lazyRunning = false;
}

/* Avvia la ripresa dei piani 2-5 in sottofondo. Chiamata quando la partita
 * comincia davvero (mai prima: vedi il commento su AppGen.lazyRunner). Silenziosa
 * su ogni fallimento -- e' un miglioramento opportunistico, non un requisito: se
 * non parte, i piani 2-5 restano sulla mini-VM, che e' esattamente la
 * degradazione gia' prevista quando il modello non c'e'. */
static void AppStartLazyGeneration(AppGen *gen)
{
    if (!gen->enabled || gen->lazyRunning) return;
    /* Serve la run gia' scritta su disco: e' da li' che la ripresa la rilegge,
       invece di inventarne un'altra (stesso seed + stesso JSON = stessa run). */
    if (!FileExists("generated/current_run.json")) return;

    /* Non e' un array static: come in AppStartGeneration, gli ultimi due slot
       dipendono da gen->lowSpec e vanno valutati a ogni chiamata. Con lowSpec
       false quei due slot sono NULL e GenRunnerStartWithArgs si ferma al primo
       NULL (vedi gen_runner.c), quindi senza il flag il comportamento resta
       IDENTICO a prima. Con lowSpec true passa lo STESSO modello del passo
       bloccante (APP_LOW_SPEC_MODEL): vedi il commento sulla costante. */
    const char *kArgs[] = {
        "--from-json", "generated/current_run.json",
        "--resume",
        "--out", "generated",
        gen->lowSpec ? "--model" : NULL,
        gen->lowSpec ? APP_LOW_SPEC_MODEL : NULL,
        NULL
    };
    /* Progresso su un file DIVERSO da quello della generazione bloccante: la barra
       dell'overlay legge generated/gen_progress.txt, e questo processo non deve
       riscriverla mentre si gioca. */
    if (GenRunnerStartWithArgs(&gen->lazyRunner, gen->command, gen->lastGenSeed, 900.0,
                               "generated/gen_progress_lazy.txt", kArgs))
    {
        gen->lazyRunning = true;
    }
}

/* 'seed' e' SEMPRE quello scelto dal chiamante (AppUi.seed di RunSetup, o un
 * NextGenSeed fresco per "Nuova run subito"/il reroll di Gameplay, vedi
 * AppEnterFloorZero piu' sotto) -- non lo si genera piu' QUI dentro con
 * NextGenSeed(0u): RunSetup e' lo stato canonico che possiede il seed della
 * run (spec M1a, ui/run-setup.md), e AppStartGeneration deve usare quello
 * esatto, non uno diverso deciso all'ultimo momento. */
static bool AppStartGeneration(AppGen *gen, unsigned int seed)
{
    AppStopLazyGeneration(gen);   /* mai due melting-gen insieme: vedi AppStopLazyGeneration */
    gen->inSpritesStage = false;
    gen->spritesPlannedThisRun = !gen->noSprites && SpritesModelsPresent();
    gen->lastGenSeed = seed;
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
    /* Step B2: il passo bloccante genera il Lua del SOLO piano 1. E' cio' che
       taglia l'attesa iniziale: 4 script invece di 20 (a ~3-5s l'uno con la cache
       del prefisso condiviso, step B1, sono ~50-80s risparmiati). Gli altri 16 li
       scrive AppStartLazyGeneration in sottofondo, quando la partita e' gia'
       cominciata. */
    /* --low-spec (preset manuale, vedi AppGen.lowSpec): forza il modello
       piccolo invece del default 7B. Non e' un array static: il terzo/quarto
       slot dipendono da gen->lowSpec, valutato a ogni chiamata. Con lowSpec
       false kArgs[2] e' NULL e GenRunnerStartWithArgs si ferma li' (vedi
       gen_runner.c), quindi il comportamento senza il flag resta IDENTICO a
       prima. */
    const char *kArgs[] = {
        "--lua-first", "1",
        gen->lowSpec ? "--model" : NULL,
        gen->lowSpec ? APP_LOW_SPEC_MODEL : NULL,
        NULL
    };
    return GenRunnerStartWithArgs(&gen->runner, gen->command, seed, 420.0,
                                  "generated/gen_progress.txt", kArgs);
}

static bool AppStartSpritesGeneration(AppGen *gen)
{
    unsigned int seed = NextGenSeed(0x5F3759DFu);
    /* --low-spec: --gen-size 256 invece del default 512 (vedi
       tools/melting-sprites/main.c). Stesso schema di AppStartGeneration
       sopra: senza il flag kArgs[0] e' NULL, nessun argomento in piu'. */
    const char *kArgs[] = {
        gen->lowSpec ? "--gen-size" : NULL,
        gen->lowSpec ? "256" : NULL,
        NULL
    };
    return GenRunnerStartWithArgs(&gen->spritesRunner, gen->spritesCommand, seed, 240.0,
                                  "generated/gen_progress.txt", kArgs);
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

/* Passo FISSO della simulazione (spec appunti 01/03: sim a 60 Hz, rendering a
   frequenza propria). Un frame video normale contiene 1 passo; un frame lento
   ne recupera fino a APP_SIM_MAX_STEPS; oltre, il tempo in eccesso si butta
   (anti "spirale della morte": frame lento -> piu' passi -> frame ancora piu'
   lento). Il gameplay smette cosi' di dipendere dal framerate. */
#define APP_SIM_DT (1.0f / 60.0f)
#define APP_SIM_MAX_STEPS 5

/* L'unico punto che chiama IsKeyPressed per queste chiavi (vedi AppInput in
   app_internal.h): riempito UNA volta per frame di finestra, prima di
   UpdateApp. F11 resta fuori (gestito a parte, subito sotto in UpdateApp):
   non e' un evento di navigazione fra stati. */
static AppInput AppInputCollect(void)
{
    AppInput input = { 0 };
    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    input.back = IsKeyPressed(KEY_ESCAPE);
    input.pause = IsKeyPressed(KEY_P);
    input.up = IsKeyPressed(KEY_UP);
    input.down = IsKeyPressed(KEY_DOWN);
    input.tab = IsKeyPressed(KEY_TAB);
    input.reroll = IsKeyPressed(KEY_R);
    input.quit = IsKeyPressed(KEY_Q);
    input.bomb = IsKeyPressed(KEY_SPACE);
    return input;
}

/* Ingresso canonico in FloorZero (RunSetup/Avvia, Gameplay/reroll con
   generazione, RunResults/"Nuova run subito"): SEMPRE lo stesso cammino,
   "niente scorciatoie" (spec M1a). 'seed' e' gia' deciso dal chiamante (il
   seed di RunSetup, o un NextGenSeed fresco per gli altri due ingressi).
   Con generazione disabilitata o AppStartGeneration fallita, la transizione
   a Gameplay avviene SUBITO, nello stesso update: il flusso e' comunque
   passato per FloorZero (*mode lo ha attraversato un istante prima), che e'
   quanto la spec richiede per M1a (la sala d'attesa giocabile arriva in
   M1b). */
static void AppEnterFloorZero(Game *game, AppGen *gen, AppMode *mode, unsigned int seed)
{
    *mode = APP_FLOOR_ZERO;
    if (gen->enabled && AppStartGeneration(gen, seed)) return;   /* il case APP_FLOOR_ZERO sondera' il progresso ai prossimi frame */

    GameResetRun(game);
    if (gen->enabled) GameSetMessage(game, "melting-gen non disponibile: contenuti esistenti");
    *mode = APP_GAMEPLAY;
}

/* La macchina a stati canonica (9 stati, vedi UpdateApp in app_internal.h per
   il contratto). Regola generale di focus condivisa da Options/BuildScreen/
   ExitConfirm (ui/navigation-map.md, "il focus torna sull'elemento che ha
   aperto la schermata"): chi le apre scrive SEMPRE
   ui->returnFocus = ui->focus (il proprio, PRIMA di cambiare *mode), cosi' un
   "back" identico in tutt'e tre le sappia restituire senza un caso per
   ciascuna provenienza. */
bool UpdateApp(Game *game, AppMode *mode, AppGen *gen, AppUi *ui, const AppInput *input)
{
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    /* Step B2: sondato SEMPRE, non solo durante Gameplay (diverso da
       pre-M1a) -- cosi' il processo di ripresa in sottofondo viene raccolto
       (niente zombie, vedi GenRunnerUpdate) anche se il giocatore resta a
       lungo in PauseMenu/Options/BuildScreen invece di tornare subito in
       gioco. Non e' legato alla simulazione (che invece SI ferma fuori da
       Gameplay): e' un processo di sistema indipendente. */
    if (gen->lazyRunning)
    {
        GenRunnerUpdate(&gen->lazyRunner);
        if (gen->lazyRunner.state != GEN_RUNNER_RUNNING) gen->lazyRunning = false;
    }

    /* Click del mouse su una voce di menu (DEC-057: il mouse e' ammesso nei
       menu, la tastiera resta la via primaria): tradotto in un confirm
       sintetico sulla voce cliccata, su una copia locale di 'input' -- cosi'
       lo switch sotto non deve sapere nulla del mouse, e --states-test (che
       passa AppInput sintetici e non muove mai il mouse vero) non ne risente
       mai. RendererMenuItemAt e' la fonte unica della geometria delle voci
       (la stessa usata per disegnarle, src/render/game_renderer.c): una query,
       non una copia duplicata del layout. */
    AppInput effective = *input;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        int clicked = RendererMenuItemAt(*mode, GetMousePosition());
        if (clicked >= 0)
        {
            ui->focus = clicked;
            effective.confirm = true;
        }
    }

    switch (*mode)
    {
        case APP_MAIN_MENU:
        {
            if (effective.up || effective.down) { ui->focus = (ui->focus + 3 + (effective.down ? 1 : -1)) % 3; break; }
            if (effective.quit || effective.back)
            {
                /* Azione distruttiva: passa SEMPRE da ExitConfirm, mai un'uscita
                   diretta (regola KB, niente piu' scorciatoie come il vecchio Q). */
                ui->openedFrom = APP_MAIN_MENU;
                ui->returnFocus = ui->focus;
                ui->exitAbandonsRun = false;
                *mode = APP_EXIT_CONFIRM;
                ui->focus = 1;   /* default: "Annulla", l'opzione non distruttiva */
                break;
            }
            if (effective.confirm)
            {
                if (ui->focus == 0)   /* Nuova run */
                {
                    ui->seed = NextGenSeed(0u);   /* seed PROPOSTO all'ingresso in RunSetup: il giocatore puo' solo rerollarlo o accettarlo */
                    *mode = APP_RUN_SETUP;
                    ui->focus = 0;
                }
                else if (ui->focus == 1)   /* Opzioni */
                {
                    ui->openedFrom = APP_MAIN_MENU;
                    ui->returnFocus = ui->focus;
                    *mode = APP_OPTIONS;
                    ui->focus = 0;
                }
                else   /* Esci */
                {
                    ui->openedFrom = APP_MAIN_MENU;
                    ui->returnFocus = ui->focus;
                    ui->exitAbandonsRun = false;
                    *mode = APP_EXIT_CONFIRM;
                    ui->focus = 1;
                }
            }
            break;
        }

        case APP_RUN_SETUP:
        {
            if (effective.up || effective.down) { ui->focus = (ui->focus + 3 + (effective.down ? 1 : -1)) % 3; break; }
            if (effective.back) { *mode = APP_MAIN_MENU; ui->focus = 0; break; }
            if (effective.reroll) { ui->seed = NextGenSeed(0u); break; }   /* R rigenera il seed a prescindere dal focus */
            if (effective.confirm)
            {
                if (ui->focus == 0) ui->seed = NextGenSeed(0u);              /* Seed: confirm equivale a reroll */
                else if (ui->focus == 1) AppEnterFloorZero(game, gen, mode, ui->seed);   /* Avvia */
                else { *mode = APP_MAIN_MENU; ui->focus = 0; }               /* Indietro */
            }
            break;
        }

        case APP_FLOOR_ZERO:
        {
            /* M1a: la generazione resta BLOCCANTE, ospitata qui come overlay
               (comportamento equivalente a pre-M1a, solo riposizionato nello
               stato canonico: la sala d'attesa giocabile arriva in M1b). */
            GenRunner *active = gen->inSpritesStage ? &gen->spritesRunner : &gen->runner;
            GenRunnerUpdate(active);
            if (effective.back)
            {
                GenRunnerCancel(active);
                *mode = APP_MAIN_MENU;
                ui->focus = 0;
                break;
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
                        *mode = APP_GAMEPLAY;
                        AppStartLazyGeneration(gen);   /* il passo TESTO e' comunque riuscito: i piani 2-5 si possono scrivere */
                    }
                }
                else
                {
                    GameResetRun(game);
                    if (gen->inSpritesStage) GameSetMessage(game, "Sprite generati: run pronta");
                    *mode = APP_GAMEPLAY;
                    /* Step B2: la partita comincia ORA -> parte la ripresa dei piani
                       2-5 in sottofondo. Solo se il passo TESTO e' andato a buon fine:
                       la ripresa rilegge generated/current_run.json e ha bisogno dello
                       STESSO seed di quella run (gen->lastGenSeed) -- se il passo testo
                       fosse fallito, quel JSON sarebbe di una run precedente e la
                       ripresa ricostruirebbe un contenuto DIVERSO da quello che si sta
                       giocando. */
                    if (gen->runner.state == GEN_RUNNER_SUCCEEDED) AppStartLazyGeneration(gen);
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
                *mode = APP_GAMEPLAY;
                /* Correzione da review (pre-M1a): se a fallire e' stato il passo
                   SPRITE (timeout a 240s, o fork fallita), il passo TESTO era
                   comunque riuscito -- la run che si sta per giocare e' quella
                   nuova, e i piani 2-5 vanno scritti lo stesso. */
                if (gen->runner.state == GEN_RUNNER_SUCCEEDED) AppStartLazyGeneration(gen);
            }
            break;
        }

        case APP_GAMEPLAY:
        {
            /* Fine run (boss del piano 5 sconfitto, o salute a zero): rilevata
               osservando la fase terminale del Game (combat.c la scrive), non
               un evento a parte. Controllata PRIMA di ogni altro input: una run
               finita non deve piu' reagire a pausa/tab/reroll dello stesso frame. */
            if (game->phase == PHASE_WIN || game->phase == PHASE_GAME_OVER)
            {
                *mode = APP_RUN_RESULTS;
                ui->focus = 0;
                break;
            }
            if (effective.pause || effective.back) { *mode = APP_PAUSE_MENU; ui->focus = 0; break; }
            if (effective.tab)
            {
                ui->openedFrom = APP_GAMEPLAY;
                ui->returnFocus = ui->focus;
                *mode = APP_BUILD_SCREEN;
                ui->focus = 0;
                break;
            }
            if (effective.reroll)
            {
                /* Con generazione: nuova run con seed nuovo, stesso cammino
                   canonico di RunSetup/Avvia (niente scorciatoie). Senza: il
                   reset rapido dev di sempre, latchato per il passo che lo
                   consuma (vedi il commento in game_types.h). */
                if (gen->enabled) AppEnterFloorZero(game, gen, mode, NextGenSeed(0u));
                else game->resetQueued = true;
                break;
            }
            if (effective.bomb) game->bombQueued = true;
            break;
        }

        case APP_PAUSE_MENU:
        {
            if (effective.up || effective.down) { ui->focus = (ui->focus + 4 + (effective.down ? 1 : -1)) % 4; break; }
            if (effective.back || effective.pause) { *mode = APP_GAMEPLAY; break; }   /* "Riprendi" e' anche ESC/P diretto */
            if (effective.confirm)
            {
                if (ui->focus == 0) *mode = APP_GAMEPLAY;   /* Riprendi */
                else if (ui->focus == 1)   /* Build e sinergie */
                {
                    ui->openedFrom = APP_PAUSE_MENU;
                    ui->returnFocus = ui->focus;
                    *mode = APP_BUILD_SCREEN;
                    ui->focus = 0;
                }
                else if (ui->focus == 2)   /* Opzioni */
                {
                    ui->openedFrom = APP_PAUSE_MENU;
                    ui->returnFocus = ui->focus;
                    *mode = APP_OPTIONS;
                    ui->focus = 0;
                }
                else   /* Abbandona run */
                {
                    ui->openedFrom = APP_PAUSE_MENU;
                    ui->returnFocus = ui->focus;
                    ui->exitAbandonsRun = true;
                    *mode = APP_EXIT_CONFIRM;
                    ui->focus = 1;
                }
            }
            break;
        }

        case APP_OPTIONS:
        {
            /* Schermata minima (M1a): una sola voce, "Indietro" -- back o
               confirm fanno la stessa cosa. Le opzioni vere arrivano con
               ui/options-and-accessibility.md, fuori scope qui. */
            if (effective.back || effective.confirm)
            {
                *mode = ui->openedFrom;
                ui->focus = ui->returnFocus;
            }
            break;
        }

        case APP_BUILD_SCREEN:
        {
            if (effective.back || effective.tab || effective.confirm)
            {
                *mode = ui->openedFrom;
                ui->focus = ui->returnFocus;
            }
            break;
        }

        case APP_RUN_RESULTS:
        {
            if (effective.up || effective.down) { ui->focus = (ui->focus + 2 + (effective.down ? 1 : -1)) % 2; break; }
            if (effective.confirm)
            {
                if (ui->focus == 0) AppEnterFloorZero(game, gen, mode, NextGenSeed(0u));   /* Nuova run subito */
                else { *mode = APP_MAIN_MENU; ui->focus = 0; }                             /* Menu principale */
            }
            break;
        }

        case APP_EXIT_CONFIRM:
        {
            if (effective.up || effective.down) { ui->focus = (ui->focus + 2 + (effective.down ? 1 : -1)) % 2; break; }
            if (effective.back) { *mode = ui->openedFrom; ui->focus = ui->returnFocus; break; }
            if (effective.confirm)
            {
                if (ui->focus == 0)   /* Conferma */
                {
                    if (ui->exitAbandonsRun) { *mode = APP_MAIN_MENU; ui->focus = 0; }
                    else return true;   /* uscita dall'applicazione: l'UNICO modo, niente piu' scorciatoie dirette */
                }
                else { *mode = ui->openedFrom; ui->focus = ui->returnFocus; }   /* Annulla */
            }
            break;
        }
    }
    return false;
}

/* Un passo di simulazione a dt FISSO: il gioco vero solo in Gameplay, le sole
   particelle cosmetiche in tutti gli altri stati (menu, generazione, pausa,
   build, risultati...). Il mouse viene mappato una volta per frame dal
   chiamante: dentro lo stesso frame non cambia. */
static void AppSimStep(Game *game, AppMode mode, UiLayout layout)
{
    if (mode == APP_GAMEPLAY)
    {
        Vector2 mouseGame = { 0.0f, 0.0f };
        bool mouseInsideGame = UiScreenToGameMouse(layout, &mouseGame);
        GameUpdate(game, APP_SIM_DT, mouseGame, mouseInsideGame);
    }
    else GameUpdateParticles(game, APP_SIM_DT);
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
    bool rarityScreenshotTest = false;
    bool shotFormsScreenshotTest = false;
    bool scriptSandboxTest = false;
    bool scriptDeterminismTest = false;
    bool scriptItemsTest = false;
    bool benchPresetTest = false;
    bool statesTest = false;
    unsigned int scriptSeed = 12345u;
    /* --full-spec: override manuale gemello di --low-spec (vedi
       AppReadBenchmarkPreset in app.h/qui sopra) -- forza il preset di
       default anche quando logs/benchmark.txt dice tier=lowspec, ignorando
       il benchmark del tutto. Non e' un campo di AppGen: non serve dopo
       l'avvio, a differenza di gen.lowSpec che i due AppStart* rileggono a
       ogni chiamata. */
    bool fullSpec = false;
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
        /* Fase 3b VISIVA: come --layer-test, ma equipaggia/piazza un oggetto
           per ciascuna delle quattro rarita' invece di un mix di slot, per
           verificare a schermo RarityColor/RarityName (src/render/
           rarity_style.h). Vedi GameRarityScreenshotTest. */
        if (strcmp(argv[i], "--rarity-screenshot-test") == 0)
        {
            smokeTest = true;
            rarityScreenshotTest = true;
        }
        /* Step C: le cinque forme di colpo, una accanto all'altra (vedi
           GameShotFormsScreenshotTest). Finestra GRANDE come le altre due sopra:
           serve leggere anche la riga "Colpo" del pannello "GIOCATORE". */
        if (strcmp(argv[i], "--shot-forms-screenshot-test") == 0)
        {
            smokeTest = true;
            shotFormsScreenshotTest = true;
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
        /* M1a: come --portal-test, gira dopo InitWindow ma prima del loop
           principale; chiama UpdateApp direttamente con AppInput sintetici
           (vedi GameStatesTest in src/tests/game_tests.c). */
        if (strcmp(argv[i], "--states-test") == 0)
        {
            smokeTest = true;
            statesTest = true;
        }
        if (strcmp(argv[i], "--gen-test") == 0) genTest = true;
        if (strcmp(argv[i], "--script-sandbox-test") == 0) scriptSandboxTest = true;
        if (strcmp(argv[i], "--script-determinism-test") == 0) scriptDeterminismTest = true;
        if (strcmp(argv[i], "--script-items-test") == 0) scriptItemsTest = true;
        if (strcmp(argv[i], "--bench-preset-test") == 0) benchPresetTest = true;
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) scriptSeed = (unsigned int)strtoul(argv[++i], NULL, 10);
        if (strcmp(argv[i], "--generate") == 0) gen.enabled = true;
        if (strcmp(argv[i], "--no-sprites") == 0) gen.noSprites = true;
        /* Preset manuale per hardware sotto la scheda di riferimento (vedi
           il commento su AppGen.lowSpec): melting-gen col modello 1.5B,
           melting-sprites a 256px. Nessun effetto sui default se assente. */
        if (strcmp(argv[i], "--low-spec") == 0) gen.lowSpec = true;
        /* Override manuale gemello: ignora il benchmark, forza il preset di
           default anche se logs/benchmark.txt dice tier=lowspec (vedi
           AppReadBenchmarkPreset). */
        if (strcmp(argv[i], "--full-spec") == 0) fullSpec = true;
        if (strcmp(argv[i], "--gen-cmd") == 0 && i + 1 < argc) gen.command = argv[++i];
        if (strcmp(argv[i], "--sprites-cmd") == 0 && i + 1 < argc) gen.spritesCommand = argv[++i];
    }

    /* Piano strategico 16/07/2026, sezione tier: solo con --generate, solo se
       l'utente non ha gia' scelto (--low-spec/--full-spec, vedi sopra). Prima
       di InitWindow: e' solo I/O di file, il messaggio (se c'e') si applica
       al Game piu' sotto, appena esiste (GameResetRun). NIENTE esecuzione
       automatica del benchmark qui (v1): si legge solo logs/benchmark.txt se
       gia' scritto da "make benchmark" (scripts/benchmark.sh); se manca, il
       comportamento resta quello di sempre. */
    char benchMsg[160] = { 0 };
    if (gen.enabled)
    {
        bool lowSpecFromBench = gen.lowSpec;
        AppReadBenchmarkPreset("logs/benchmark.txt", gen.lowSpec, fullSpec, &lowSpecFromBench, benchMsg, sizeof(benchMsg));
        gen.lowSpec = lowSpecFromBench;
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
    /* Piano strategico 16/07/2026, sezione tier: AppReadBenchmarkPreset e'
       solo I/O di file (nessuna finestra), stessa famiglia dei tre test sopra. */
    if (benchPresetTest)
    {
        bool ok = AppBenchmarkPresetSelfTest();
        printf("Benchmark preset test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 14;   /* 14: il primo codice di uscita libero (vedi gli altri test sopra) */
    }

    /* --rarity-screenshot-test vuole la finestra GRANDE (non compatta) come
       --screenshot-test: a differenza di --layer-test (dove il personaggio
       equipaggiato e' l'unica cosa da vedere), qui serve anche leggere per
       intero il pannello "GIOCATORE" a destra (bordo + nome della rarita'),
       che nella finestra compatta 960x640 finisce in parte sotto il
       riquadro "GAME VIEW" (overlap gia' presente anche in --layer-test:
       vedi logs/melting-run-layers-screen.png). */
    bool compactTestWindow = smokeTest && !screenshotTest && !rarityScreenshotTest && !shotFormsScreenshotTest;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    /* Titolo della finestra WORLDSMELT (DEC-071): il repo conserva il nome
       storico solo in locale (percorsi, binari, cartelle) -- vedi CLAUDE.md. */
    InitWindow(compactTestWindow ? SCREEN_WIDTH : APP_WINDOW_WIDTH, compactTestWindow ? SCREEN_HEIGHT : APP_WINDOW_HEIGHT, "WORLDSMELT");
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
    /* Messaggio del preset da benchmark (se c'e', vedi AppReadBenchmarkPreset
       piu' sopra): il Game esiste solo da qui in poi, prima non c'era nulla a
       cui passare GameSetMessage. */
    if (benchMsg[0]) GameSetMessage(&game, benchMsg);
    if (portalTest)
    {
        bool ok = GamePortalRespawnTest(&game);
        printf("Portal test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 2;
    }
    if (statesTest)
    {
        bool ok = GameStatesTest(&game);
        printf("States test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 15;   /* 15: il primo codice di uscita libero (vedi gli altri test sopra) */
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
    if (rarityScreenshotTest)
    {
        bool ok = GameRarityScreenshotTest(&game);
        printf("Rarity screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 12;
    }
    if (shotFormsScreenshotTest)
    {
        bool ok = GameShotFormsScreenshotTest(&game);
        printf("Shot forms screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 13;   /* 13: il primo codice di uscita libero (vedi gli altri test sopra) */
    }

    RenderTexture2D gameCanvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    /* POINT, non BILINEAR: gli sprite sono pixel art a 16 colori e il filtro
       bilineare li sfocava nello scaling del canvas. La scala del layout e'
       agganciata a passi di 1/8 (UiComputeLayout) apposta per rendere
       regolare la cadenza dei pixel raddoppiati con questo filtro. */
    SetTextureFilter(gameCanvas.texture, TEXTURE_FILTER_POINT);
    AppMode appMode = (smokeTest && !menuScreenshotTest) ? APP_GAMEPLAY : APP_MAIN_MENU;
    /* AppUi vive per l'intera sessione (posseduta da AppRun, spec M1a):
       zero-inizializzata, il che mette il focus a 0 su qualunque stato --
       esattamente cio' che serve per APP_MAIN_MENU (focus iniziale "Nuova
       run", indice 0) e per --smoke-test che parte in Gameplay (dove il
       focus non e' letto da nessuno). */
    AppUi appUi = { 0 };
    int frames = smokeTest ? 10 : -1;
    bool screenshotDone = false;
    float simAccum = 0.0f;
    while (!WindowShouldClose())
    {
        /* Nei test a frame contati il tempo e' forzato a un passo esatto:
           10 frame = 10 passi di simulazione, qualunque cosa faccia il clock
           sotto Xvfb (il primo GetFrameTime dopo l'init puo' valere centinaia
           di ms). In gioco normale, un frame vicino al passo nominale viene
           AGGANCIATO al passo: il jitter del vsync (16.6ms +/- decimi) non
           deve accumulare resti che ogni tanto producono frame a 0 o 2 passi
           (micro-scatto visibile senza interpolazione). */
        float frameDt = smokeTest ? APP_SIM_DT : GetFrameTime();
        if (frameDt > APP_SIM_DT*(float)APP_SIM_MAX_STEPS) frameDt = APP_SIM_DT*(float)APP_SIM_MAX_STEPS;
        if (frameDt > APP_SIM_DT - 0.002f && frameDt < APP_SIM_DT + 0.002f) frameDt = APP_SIM_DT;
        simAccum += frameDt;
        UiLayout layout = UiComputeLayout();
        AppInput input = AppInputCollect();
        if (UpdateApp(&game, &appMode, &gen, &appUi, &input)) break;
        while (simAccum >= APP_SIM_DT)
        {
            AppSimStep(&game, appMode, layout);
            simAccum -= APP_SIM_DT;
        }
        GenProgress combinedProgress = { 0 };
        if (appMode == APP_FLOOR_ZERO) combinedProgress = AppCombinedProgress(&gen);
        RendererDrawApp(&game, gameCanvas, appMode, &appUi, screenshotTest && !screenshotDone,
                        appMode == APP_FLOOR_ZERO ? &combinedProgress : NULL, "logs/melting-run-screen.png");
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
    /* Step B2: e anche il processo di ripresa in sottofondo, che a differenza
       degli altri due e' vivo proprio MENTRE si gioca -- quindi e' quello che ha
       piu' probabilita' di essere ancora in piedi quando si chiude il gioco. */
    AppStopLazyGeneration(&gen);

    UnloadRenderTexture(gameCanvas);
    GameUnloadAssets(&game);
    CloseWindow();
    return 0;
}
