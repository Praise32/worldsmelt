#include "app/app.h"
#include "app/app_internal.h"

#include "assets/art_atlas.h"
#include "audio/audio.h"
#include "content/character_proposal.h"
#include "content/character_roster.h"
#include "content/run_catalog.h"
#include "content/run_content.h"
#include "core/game_math.h"
#include "game/game.h"
#include "game/game_internal.h"
#include "gameplay/fusion.h"
#include "gen/gen_runner.h"
#include "render/game_renderer.h"
#include "script/script_items.h"
#include "tests/game_tests.h"
#include "world/floor_zero.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    const char *kArgs[] = {
        "--from-json", "generated/current_run.json",
        "--resume",
        "--out", "generated",
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
 * esatto, non uno diverso deciso all'ultimo momento.
 * NOTA (M1b): questa funzione non tocca piu' 'game' -- FloorZeroEnter (vedi
 * AppEnterFloorZero sotto) prepara il Piano 0 indipendentemente dall'esito
 * della generazione, e GameResetRun (che carica manifest/atlas nuovi) arriva
 * solo all'attraversamento del varco, mai qui. */
static bool AppStartGeneration(AppGen *gen, unsigned int seed)
{
    AppStopLazyGeneration(gen);   /* mai due melting-gen insieme: vedi AppStopLazyGeneration */
    /* M5: proposeRunner e' gia' finito per costruzione quando si arriva qui
       (la generazione completa parte solo alla scelta della carta, e le
       carte non sono selezionabili finche' proposeRunner non e' terminale) --
       ma la cancellazione resta un no-op sicuro se per qualche motivo fosse
       ancora RUNNING, stessa difesa di AppStopLazyGeneration sopra. */
    GenRunnerCancel(&gen->proposeRunner);
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
    /* --theme-file (M5, requisiti 3/5/6): SEMPRE passato -- la generazione
       completa parte solo dopo una scelta (AppConfirmThemeChoice l'ha gia'
       scritto su disco, tmp+rename, PRIMA di chiamare questa funzione). Se il
       file per qualche motivo non c'e' (disco pieno...), melting-gen degrada
       da solo: --theme-file su un percorso illeggibile equivale a nessun
       tema scelto (vedi GenLoadChosenTheme), mai un errore fatale. */
    const char *kArgs[] = {
        "--lua-first", "1",
        "--theme-file", "generated/chosen_theme.txt",
        NULL
    };
    return GenRunnerStartWithArgs(&gen->runner, gen->command, seed, 420.0,
                                  "generated/gen_progress.txt", kArgs);
}

static bool AppStartSpritesGeneration(AppGen *gen)
{
    unsigned int seed = NextGenSeed(0x5F3759DFu);
    return GenRunnerStartWithArgs(&gen->spritesRunner, gen->spritesCommand, seed, 240.0,
                                  "generated/gen_progress.txt", NULL);
}

/* Il messaggio STABILE dell'indicatore del Piano 0 (M1b, ui/generation-status.md):
 * mappa 1:1 con gli "Stati visibili al giocatore" della KB -- MAI il
 * phase/message grezzo di GenRunner.progress (quello resta un dettaglio
 * tecnico, buono al massimo per DECIDERE fra i tre messaggi qui sotto, mai
 * per essere mostrato com'e': niente percentuali, niente frasi diverse a
 * ogni chiamata di melting-gen). 'game->floorZeroExitOpen' e' gia' la
 * verita' su "pipeline terminale" (la scrive AppOpenFloorZeroExit sotto),
 * quindi basta leggerla invece di ricalcolare la terminalita' da capo qui.
 *
 * M5 (DEC-005), requisito 10: NUOVO stato "in attesa della scelta del
 * mondo" fra "le proposte sono pronte" e "il tema e' scelto" -- il gioco non
 * genera piu' nulla in quella finestra (la generazione completa parte solo
 * DOPO la scelta), quindi non c'e' nessun altro messaggio onesto da dare:
 * mai un errore tecnico, sempre descrittivo e stabile, come richiesto dalla
 * KB. Messaggio descrittivo stabile: game->themeCardCount>0 e'
 * gia' la verita' su "le carte sono pronte" (le scrive AppLoadThemeCards/
 * AppUseFallbackThemeCards, mai a meta'). */
static const char *AppFloorZeroStatusText(const Game *game, const AppGen *gen)
{
    if (game->floorZeroExitOpen) return "Primo piano pronto -- l'uscita e' aperta.";
    if (game->themeChosenIndex < 0)
    {
        if (game->themeCardCount > 0) return "In attesa della scelta del mondo -- TAB per le carte.";
        return "Il crogiolo prepara il mondo...";
    }
    if (gen->inSpritesStage) return "Il mondo prende forma...";
    return "Il crogiolo prepara il mondo...";
}

/* Apertura dell'uscita verso il piano 1 (M1b): evento visibile e distinto
 * (KB floor-zero.md, "Feedback": mai un semplice cambio di stato silenzioso)
 * -- messaggio di gioco + il varco che cambia aspetto (DrawFloorZeroExitGate,
 * src/render/game_renderer.c, legge game->floorZeroExitOpen) + un burst di
 * particelle nello stesso punto. Idempotente: se la pipeline resta
 * "terminale" per piu' frame di fila (il caso normale: GenRunnerUpdate su un
 * runner gia' SUCCEEDED/FAILED e' un no-op, vedi gen_runner.c), la seconda
 * chiamata non deve ripetere messaggio/particelle. Vale ANCHE per un
 * fallimento della generazione (fallback silenzioso, DEC-002/DEC-020: il
 * gioco e' sempre avviabile) -- chi chiama non distingue i due casi, ed e'
 * voluto: il giocatore non deve mai vedere la differenza. */
static void AppOpenFloorZeroExit(Game *game)
{
    if (game->floorZeroExitOpen) return;
    game->floorZeroExitOpen = true;
    GameSetMessage(game, "L'uscita verso il piano 1 si apre.");
    Vector2 gate = { ROOM_X + ROOM_W*0.5f, ROOM_Y };
    EntitiesAddParticle(game, gate, game->theme.accent2, 26);
}

/* Annulla i runner di primo piano ancora attivi (ESC/ExitConfirm dal Piano 0,
 * "abbandona la preparazione"): SOLO quelli, mai il lazyRunner (che a questo
 * punto non e' ancora partito -- parte solo all'attraversamento vero, vedi il
 * case APP_FLOOR_ZERO sotto). GenRunnerCancel e' gia' un no-op se il runner
 * non e' RUNNING, quindi chiamarli entrambi senza controllare quale dei due
 * e' quello attivo e' sicuro e piu' semplice che tenere traccia a parte.
 * M5, requisito 10: anche proposeRunner -- un abbandono durante "propongo i
 * temi" (ancora prima che le carte compaiano) deve fermare pure quel
 * processo, non solo runner/spritesRunner che a quel punto non sono ancora
 * partiti. */
static void AppCancelFloorZeroGeneration(AppGen *gen)
{
    GenRunnerCancel(&gen->proposeRunner);
    GenRunnerCancel(&gen->runner);
    GenRunnerCancel(&gen->spritesRunner);
}

/* Passo FISSO della simulazione (spec appunti 01/03: sim a 60 Hz, rendering a
   frequenza propria). Un frame video normale contiene 1 passo; un frame lento
   ne recupera fino a APP_SIM_MAX_STEPS; oltre, il tempo in eccesso si butta
   (anti "spirale della morte": frame lento -> piu' passi -> frame ancora piu'
   lento). Il gameplay smette cosi' di dipendere dal framerate. */
#define APP_SIM_DT (1.0f / 60.0f)
#define APP_SIM_MAX_STEPS 5

/* Indice della riga "Indietro" in APP_OPTIONS (MenuItemCountForMode(APP_OPTIONS)
   in game_renderer.c ritorna 4: le tre barre 0..2, "Indietro" alla 3). Un solo
   simbolo condiviso fra il blocco click generico qui sopra e il case
   APP_OPTIONS sotto, cosi' il confine "riga-slider vs Indietro" non puo'
   disallinearsi fra i due punti che lo controllano (W9 correzione round 0). */
#define APP_OPTIONS_ROW_BACK 3

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
    input.left = IsKeyPressed(KEY_LEFT);    /* M5: pannello di scelta del tema in FloorZero, vedi il commento su AppInput */
    input.right = IsKeyPressed(KEY_RIGHT);
    input.tab = IsKeyPressed(KEY_TAB);
    input.reroll = IsKeyPressed(KEY_R);
    input.quit = IsKeyPressed(KEY_Q);
    input.bomb = IsKeyPressed(KEY_SPACE);
    input.useActive = IsKeyPressed(KEY_E);
    input.dropGraft = IsKeyPressed(KEY_G);
    input.fuse = IsKeyPressed(KEY_F);
    input.toggleStats = IsKeyPressed(KEY_C);
    return input;
}

/* M5 (DEC-005), requisito 1/8: avvia il processo "proponi 3 temi" (early-exit
 * come il passo testo, ma un ordine di grandezza piu' corto: ~8-12s misurati,
 * docs/engineering/benchmarks.md). Stesso comando di gen->command (bin/melting-gen o il
 * finto sostituto dei test): --propose-themes non e' un binario a parte. */
static bool AppStartProposeThemes(AppGen *gen, unsigned int seed)
{
    const char *kArgs[] = { "--propose-themes", "3", NULL };
    return GenRunnerStartWithArgs(&gen->proposeRunner, gen->command, seed, 45.0,
                                  "generated/gen_progress_propose.txt", kArgs);
}

/* M5, requisito 8: legge generated/theme_proposals.json SENZA cJSON (il
 * gioco non lo linka mai, AGENTS.md) -- schema fisso e charset ASCII senza
 * virgolette/backslash interni (propose.gbnf lo garantisce sia sul percorso
 * modello sia su quello procedurale di melting-gen, vedi il commento su
 * GenWriteThemeProposals in tools/melting-gen/gen_manifest.c), quindi basta
 * cercare "name":" e "blurb":" in ordine e leggere fino alla virgoletta di
 * chiusura successiva. Ritorna false (game->themeCards non toccato) se il
 * file manca, e' vuoto, o non contiene ALMENO una coppia valida -- il
 * chiamante ricade su AppUseFallbackThemeCards (mai meno di una carta,
 * requisito 11). */
static bool AppLoadThemeCards(Game *game, const char *path)
{
    char *text = LoadFileText(path);
    if (!text) return false;

    int count = 0;
    const char *cursor = text;
    while (count < THEME_CARD_MAX)
    {
        const char *nameKey = strstr(cursor, "\"name\":\"");
        if (!nameKey) break;
        const char *nameStart = nameKey + strlen("\"name\":\"");
        const char *nameEnd = strchr(nameStart, '"');
        if (!nameEnd) break;
        const char *blurbKey = strstr(nameEnd, "\"blurb\":\"");
        if (!blurbKey) break;
        const char *blurbStart = blurbKey + strlen("\"blurb\":\"");
        const char *blurbEnd = strchr(blurbStart, '"');
        if (!blurbEnd) break;

        ThemeCard *card = &game->themeCards[count];
        size_t nameLen = (size_t)(nameEnd - nameStart);
        if (nameLen >= sizeof(card->name)) nameLen = sizeof(card->name) - 1;
        memcpy(card->name, nameStart, nameLen);
        card->name[nameLen] = '\0';

        size_t blurbLen = (size_t)(blurbEnd - blurbStart);
        if (blurbLen >= sizeof(card->blurb)) blurbLen = sizeof(card->blurb) - 1;
        memcpy(card->blurb, blurbStart, blurbLen);
        card->blurb[blurbLen] = '\0';

        count++;
        cursor = blurbEnd + 1;
    }
    UnloadFileText(text);

    if (count < 1) return false;
    game->themeCardCount = count;
    game->themeCardFocus = 0;
    return true;
}

/* M5, requisito 8/11: carte curate lato gioco, SUBITO -- nessun processo,
 * mai meno di un'opzione selezionabile (DEC-002/requisito 11). Idempotente
 * (mai chiamata due volte sulla stessa permanenza in FloorZero): un tema gia'
 * scelto o carte gia' pronte significano che questa chiamata arriva tardi
 * (proposeRunner e' appena riuscito nel frattempo), e non deve sovrascrivere
 * nulla. */
static void AppUseFallbackThemeCards(Game *game, unsigned int seed)
{
    if (game->themeChosenIndex >= 0 || game->themeCardCount > 0) return;
    RunContentMakeFallbackThemeCards(seed, game->themeCards, THEME_CARD_MAX);
    game->themeCardCount = THEME_CARD_MAX;
    game->themeCardFocus = 0;
}

/* M5, requisito 3/6: il tema scelto viaggia al processo di generazione per
 * FILE (tmp+rename, come ogni altro output atomico di questo progetto), mai
 * per riga di comando -- nome/blurb possono contenere spazi/apostrofi che
 * andrebbero citati con cura su una argv costruita a mano (gen_runner.c).
 * Formato IDENTICO a quello che GenLoadChosenTheme (tools/melting-gen/
 * gen_util.c) si aspetta e a quanto provenance.txt scrive in chosenTheme=:
 * "<name> -- <blurb>", una riga sola. "generated/" e' gia' garantita
 * esistente dal target del Makefile (mkdir -p bin logs generated, vedi il
 * commento su ScriptSandboxLogLine in src/script/script_sandbox.c): nessun
 * mkdir a runtime qui, coerente col resto di src/. Silenzioso su ogni
 * fallimento (fopen fallita): mai bloccare la scelta del tema per un file
 * diagnostico, melting-gen degrada gia' da solo su --theme-file assente o
 * illeggibile. */
static void AppWriteChosenThemeFile(const ThemeCard *card)
{
    const char *tmpPath = "generated/chosen_theme.txt.tmp";
    const char *finalPath = "generated/chosen_theme.txt";
    FILE *f = fopen(tmpPath, "w");
    if (!f) return;
    fprintf(f, "%s -- %s\n", card->name, card->blurb);
    fclose(f);
    rename(tmpPath, finalPath);
}

/* M5, requisito 9/10: la scelta VERA del tema (confermata dal pannello di
 * carte, mai dal solo focus che ci passa sopra) -- scrive il file per
 * melting-gen e SOLO ORA avvia la generazione completa (mai prima, a
 * differenza di prima di M5: AppEnterFloorZero non chiama piu'
 * AppStartGeneration all'ingresso). Idempotente: un secondo confirm mentre
 * il tema e' gia' scelto (es. tasto tenuto premuto) non deve riavviare
 * un'altra generazione sopra quella in corso. */
/* Vedi il commento sulla dichiarazione in app_internal.h (la regola sta la',
   accanto alla firma che i test usano). */
int AppFloorZeroCardToConfirm(int cardHit, bool clicked, bool confirmKey, int focus)
{
    if (clicked && cardHit >= 0) return cardHit;
    if (confirmKey) return focus;
    return -1;
}

static void AppConfirmThemeChoice(Game *game, AppGen *gen, int index)
{
    if (game->themeChosenIndex >= 0) return;
    if (index < 0 || index >= game->themeCardCount) return;
    game->themeChosenIndex = index;
    game->themeCardsPanelOpen = false;
    AppWriteChosenThemeFile(&game->themeCards[index]);
    if (gen->enabled && AppStartGeneration(gen, gen->pendingGenSeed)) return;   /* il case APP_FLOOR_ZERO sondera' il progresso ai prossimi frame */
    AppOpenFloorZeroExit(game);
}

/* M6a (DEC-030/033), requisito 2/3: la scelta VERA del personaggio, sezione
 * PERSONAGGI del pannello combinato -- a differenza di AppConfirmThemeChoice
 * NON e' guardata da "gia' scelto": il personaggio resta modificabile per
 * tutta la permanenza nel Piano 0 (floor-zero.md, riga del Selettore
 * personaggio, "Abilitato quando: Sempre"), quindi ogni confirm nella
 * sezione PERSONAGGI deve poter cambiare scelta di nuovo. Applica le
 * statistiche SUBITO (GamePlayerResetBaseStatsFor + ScriptItemsInit, come
 * FloorZeroEnter): il giocatore SENTE la differenza nell'hub dallo stesso
 * frame in cui conferma, requisito 2 della spec. Idempotente per
 * costruzione: riconfermare lo stesso indice rideriva esattamente gli
 * stessi numeri (ScriptItemsInit riparte sempre da zero, sistema delle
 * cache). Non tocca il pannello (resta aperto, a differenza della scelta
 * del tema): la sezione PERSONAGGI invita a confrontare le tre schede senza
 * richiudersi ad ogni conferma.
 * M6b-1 (DEC-014): 'index' puo' ora arrivare anche a CHARACTER_COUNT (il
 * quarto slot, il personaggio generato) -- GameResolveCharacterDef e' l'UNICO
 * punto che sa interpretarlo, quindi la guardia diventa "risolve a qualcosa"
 * invece di "sta dentro la rosa": un CHARACTER_COUNT senza una proposta
 * valida (generatedCharacterValid falso) risolve a NULL e la conferma non fa
 * nulla, coerente con "carta assente, nessuna selezione possibile". */
static void AppConfirmCharacterChoice(Game *game, int index)
{
    const CharacterDef *character = GameResolveCharacterDef(game, index);
    if (!character) return;
    game->characterChosenIndex = index;
    GamePlayerResetBaseStatsFor(&game->player, character);
    ScriptItemsInit(game, character);
}

/* M6b-1 (DEC-014, prima fetta): legge generated/character_proposal.json in
 * una CharacterDef "generata" (RunContentLoadCharacterProposal, src/content/
 * character_proposal.c -- scanner a mano, seconda rete di clamp) e la
 * pubblica nel canale dati dinamico del quarto slot (Game.generatedCharacter
 * + generatedCharacterValid), stesso schema di AppLoadThemeCards. Chiamata
 * SOLO quando proposeRunner e' terminale (vedi il case APP_FLOOR_ZERO), sia
 * su successo (il file potrebbe comunque non esistere: la generazione del
 * personaggio e' indipendente da quella dei temi, e il suo fallback canonico
 * e' l'assenza della carta, mai un errore) sia su fallimento del propose
 * (stesso identico trattamento: nessun file, nessuna carta -- ma tentare
 * comunque il caricamento e' innocuo e piu' semplice che duplicare la
 * guardia in due punti diversi del chiamante). Nessun effetto se il file
 * manca o non valida: game->generatedCharacterValid resta quello che era
 * (falso, per costruzione: FloorZeroEnter l'ha appena azzerato). */
static void AppLoadCharacterProposal(Game *game)
{
    CharacterDef proposal;
    if (RunContentLoadCharacterProposal("generated/character_proposal.json", &proposal))
    {
        game->generatedCharacter = proposal;
        game->generatedCharacterValid = true;
    }
}

/* Ingresso canonico in FloorZero (RunSetup/Avvia, Gameplay/reroll con
   generazione, RunResults/"Nuova run subito"): SEMPRE lo stesso cammino,
   "niente scorciatoie" (spec M1a/M1b). 'seed' e' gia' deciso dal chiamante
   (il seed di RunSetup, o un NextGenSeed fresco per gli altri due ingressi).
   FloorZeroEnter prepara SUBITO la sala d'attesa giocabile (M1b: mai piu' un
   overlay bloccante) -- il giocatore ci si muove da questo stesso frame.
   M5 (DEC-005): la generazione completa NON parte piu' qui -- parte alla
   scelta della carta (AppConfirmThemeChoice). Qui si avvia SOLO il processo
   "proponi 3 temi"; se la generazione e' disabilitata, o quel processo non
   parte nemmeno (fork fallita), le carte curate lato gioco compaiono SUBITO
   (AppUseFallbackThemeCards, DEC-002: il gioco resta sempre avviabile). */
static void AppEnterFloorZero(Game *game, AppGen *gen, AppMode *mode, unsigned int seed)
{
    *mode = APP_FLOOR_ZERO;
    FloorZeroEnter(game);
    gen->pendingGenSeed = seed;
    if (gen->enabled && AppStartProposeThemes(gen, seed)) return;   /* il case APP_FLOOR_ZERO sondera' le proposte ai prossimi frame */
    AppUseFallbackThemeCards(game, seed);
}

/* ============================================================
   LA FUSIONE dentro BuildScreen (systems/item-fusion.md, DEC-022/023/143/162/171
   + ui/inventory-and-synergy-screen.md, riga "Fusioni possibili").

   Dove vive il flusso, e perche' qui: la meccanica sta tutta in
   src/gameplay/fusion.h (composizione, budget, consumo, inventario); questo
   e' solo il pezzo di INTERFACCIA, cioe' quello che src/app possiede per
   contratto (AGENTS.md). Nessuna regola di fusione e' scritta in questo file.

   DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019), da far confermare al
   proprietario: item-fusion.md pone la fusione nella STANZA DI FUSIONE
   (systems/special-rooms.md), che nel motore non esiste ancora -- non c'e'
   un ROOM_FUSION, e piazzarlo e' lavoro di un altro blocco (vedi la matrice
   di copertura, sezione "Stanze speciali"). Finche' quella stanza non c'e',
   l'innesco vive nell'unico posto che il design gia' assegna alla fusione:
   la sezione "Fusioni possibili" di BuildScreen. Quando ROOM_FUSION
   arrivera' bastera' aggiungere UNA condizione qui sotto (la stanza corrente
   e' di fusione) -- il resto del flusso non cambia. La deviazione e'
   dichiarata in ui/inventory-and-synergy-screen.md (nota di implementazione)
   e la domanda per il proprietario e' registrata insieme alle altre della
   sessione.
   ============================================================ */

static void AppFusionClearSelection(AppUi *ui)
{
    ui->fusionSourceA = FUSION_UI_NONE;
    ui->fusionSourceB = FUSION_UI_NONE;
}

/* Entrare in BuildScreen: la selezione riparte SEMPRE vuota, perche' i due
   campi sono indici dentro items[] e l'inventario puo' essere cambiato
   mentre la schermata era chiusa (un oggetto raccolto, un Innesto sganciato,
   un attivo scambiato sul piedistallo). Messaggio ed esito dell'ultima
   fusione restano invece visibili: sono un RISULTATO, non una selezione. */
static void AppEnterBuildScreen(Game *game, AppUi *ui, AppMode from)
{
    ui->openedFrom = from;
    ui->returnFocus = ui->focus;
    ui->focus = 0;
    AppFusionClearSelection(ui);
    int count = game->player.itemCount;
    if (count > MAX_ITEMS) count = MAX_ITEMS;
    if (ui->buildItemFocus >= count) ui->buildItemFocus = count > 0 ? count - 1 : 0;
    if (ui->buildItemFocus < 0) ui->buildItemFocus = 0;
    /* W9 correzione round 1: l'ancora di scorrimento della lista riparte dalla
       riga a fuoco (AppUi.buildItemScroll -- la finestra visibile dipende da
       lei, non piu' dal focus). Senza questa riga il primo frame della
       schermata userebbe l'ancora della visita precedente, che l'inventario
       cambiato nel frattempo puo' aver reso insensata. Il valore esatto (quante
       righe stanno nella finestra, quindi quanto l'ancora va tirata indietro)
       lo sistema AppBuildScrollFollowFocus, che gira all'inizio di ogni frame
       di BuildScreen: qui non si puo' chiedere al renderer, questa funzione la
       chiamano anche cammini senza finestra. */
    ui->buildItemScroll = ui->buildItemFocus;
}

/* Seleziona/deseleziona l'oggetto a fuoco. L'ORDINE conta (e' il tie-break di
   DEC-143 e del punto 4 di "Priorita' e conflitti"), quindi: la prima scelta
   resta ferma finche' non la si toglie, e una terza scelta sostituisce la
   SECONDA -- si tiene l'ancora e si cambia il candidato, che e' il modo in
   cui si ragiona su una fusione. */
static void AppFusionToggle(AppUi *ui, int slot)
{
    int a = FUSION_UI_SLOT(ui->fusionSourceA);
    int b = FUSION_UI_SLOT(ui->fusionSourceB);
    if (slot == a)
    {
        /* Togliere la prima promuove la seconda: mai un buco davanti a una
           scelta gia' fatta. */
        ui->fusionSourceA = ui->fusionSourceB;
        ui->fusionSourceB = FUSION_UI_NONE;
        return;
    }
    if (slot == b) { ui->fusionSourceB = FUSION_UI_NONE; return; }
    if (a < 0) { ui->fusionSourceA = FUSION_UI_FIELD(slot); return; }
    ui->fusionSourceB = FUSION_UI_FIELD(slot);
}

static void AppFusionConfirm(Game *game, AppUi *ui)
{
    int a = FUSION_UI_SLOT(ui->fusionSourceA);
    int b = FUSION_UI_SLOT(ui->fusionSourceB);
    Item fused;
    FusionStatus status = FusionPerform(game, a, b, &fused);
    snprintf(ui->fusionMessage, sizeof(ui->fusionMessage), "%s", FusionStatusText(status));
    if (status != FUSION_OK) return;

    /* DEC-118: la fusione ha il segnale sonoro piu' riconoscibile del gioco,
       priorita' massima -- vedi docs/design/content/audio-and-feedback.md. */
    AudioPlaySfx(AUDIO_SFX_FUSION_COMPLETE);
    snprintf(ui->fusionResultName, sizeof(ui->fusionResultName), "%s", fused.name);
    snprintf(ui->fusionResultImage, sizeof(ui->fusionResultImage), "%s", fused.imagePath);
    snprintf(ui->fusionResultImageId, sizeof(ui->fusionResultImageId), "%s", fused.imageId);   /* W8: primo gradino della priorita' */
    /* Precisioni esplicite: la riga deve stare in fusionMessage[96] qualunque
       nome abbiano i tre oggetti coinvolti (troncare un nome lunghissimo e'
       corretto, perdere il messaggio no). */
    snprintf(ui->fusionMessage, sizeof(ui->fusionMessage), "Fuso: %.30s (da %.22s + %.22s).",
             fused.name, fused.fusedFrom[0], fused.fusedFrom[1]);
    /* Lo stesso annuncio anche nel log di gioco: la fusione e' un evento
       della run, non solo di questa schermata (item-fusion.md, "Feedback":
       un momento dedicato, non un popup di drop). */
    GameSetMessage(game, ui->fusionMessage);
    AppFusionClearSelection(ui);
    /* Due oggetti sono usciti e uno e' entrato: il fuoco della lista va
       riportato dentro l'inventario nuovo, sul risultato (l'ultimo slot),
       che e' anche cio' che il giocatore vuole guardare adesso. */
    int count = game->player.itemCount;
    if (count > MAX_ITEMS) count = MAX_ITEMS;
    ui->buildItemFocus = count > 0 ? count - 1 : 0;
}

/* Il ramo "fusione" di APP_BUILD_SCREEN: su/giu' scorrono gli oggetti,
   conferma seleziona (ui/inventory-and-synergy-screen.md, riga "Lista oggetti
   acquisiti"), F conferma la fusione (riga "Conferma fusione" di
   item-fusion.md). Nessun'altra transizione di stato: uscire da qui e' gia'
   gestito dal chiamante. */
static void AppUpdateBuildScreen(Game *game, AppUi *ui, const AppInput *input)
{
    int count = game->player.itemCount;
    if (count > MAX_ITEMS) count = MAX_ITEMS;
    if (count <= 0) { ui->buildItemFocus = 0; return; }
    if (ui->buildItemFocus >= count) ui->buildItemFocus = count - 1;
    if (ui->buildItemFocus < 0) ui->buildItemFocus = 0;

    if (input->up || input->down)
    {
        ui->buildItemFocus = (ui->buildItemFocus + count + (input->down ? 1 : -1))%count;
        return;
    }
    if (input->confirm) { AppFusionToggle(ui, ui->buildItemFocus); return; }
    if (input->fuse) AppFusionConfirm(game, ui);
}

/* W9 correzione round 1 (BOCCIATO, "l'anello di retroazione della lista
   scorrevole"): tiene l'ANCORA di scorrimento della lista OGGETTI PRESI
   ('ui->buildItemScroll', l'indice del primo oggetto mostrato) allineata al
   focus, scorrendo del MINIMO indispensabile -- e SOLO quando il focus e'
   uscito dalla finestra. Sostituisce la vecchia derivazione "first = focus -
   maxShow + 1" che stava dentro la geometria del renderer: la' rendeva la
   mappatura "punto dello schermo -> indice di oggetto" dipendente dal focus,
   quindi l'hover del mouse (che scrive il focus) alimentava se stesso e faceva
   scorrere la lista di uno step per ogni frame di MOVIMENTO, annullando la
   rotellina. Adesso l'hover, che per definizione cade su una riga GIA' dentro
   la finestra, non muove mai l'ancora: l'anello e' rotto per costruzione.
   Quante righe stanno nella finestra lo dice il renderer
   (RendererBuildItemRowsVisible: STESSA misura del disegno, mai una copia --
   dipende dall'altezza della finestra e dalle sinergie attive, cose che questo
   file non conosce). Chiamata all'INIZIO del frame di BuildScreen (cosi' il
   hit-test lavora sulla finestra che il giocatore ha DAVVERO davanti) e alla
   FINE (cosi' il disegno di questo stesso frame mostra il focus appena
   spostato da tastiera/rotellina): idempotente, senza cambi di focus la seconda
   chiamata non fa nulla. */
static void AppBuildScrollFollowFocus(Game *game, AppUi *ui)
{
    int count = GameMathClampInt(game->player.itemCount, 0, MAX_ITEMS);
    if (count <= 0) { ui->buildItemScroll = 0; return; }
    int maxShow = RendererBuildItemRowsVisible(game);
    if (maxShow < 1) maxShow = 1;
    int lastAnchor = (count > maxShow) ? count - maxShow : 0;
    int focus = GameMathClampInt(ui->buildItemFocus, 0, count - 1);
    int scroll = GameMathClampInt(ui->buildItemScroll, 0, lastAnchor);
    if (focus < scroll) scroll = focus;
    else if (focus > scroll + maxShow - 1) scroll = focus - maxShow + 1;
    ui->buildItemScroll = GameMathClampInt(scroll, 0, lastAnchor);
}

/* M7 (DEC-015/041/045/069, substrato del catalogo persistente): l'hook di
   scrittura, UNA funzione sola per i TRE chiamanti che possono chiudere una
   run (spec, punto 3) -- vittoria/sconfitta in APP_GAMEPLAY, abbandono
   confermato in APP_EXIT_CONFIRM (Piano 0 incluso: la guardia "floor<1" sotto
   lo esclude gia' da sola, nessun bisogno di leggere ui->openedFrom qui), e
   il reroll in APP_GAMEPLAY (che oggi sfuggiva a ogni hook -- coperto
   chiamando questa funzione PRIMA di AppEnterFloorZero/game->resetQueued, mai
   dopo: entrambi i cammini toccano/azzerano campi di Game che questa
   funzione legge, vedi FloorZeroEnter/GameResetRun).
   Guardie, in ordine:
   - 'catalogWritesEnabled' (AppUi, zero-default): la guardia test-safe (spec
     punto 3, "i game test... non devono scrivere file"). OGNI test C che
     chiama UpdateApp direttamente costruisce la propria AppUi con "{0}"
     (src/tests/game_tests.c e affini), quindi questa funzione ritorna subito
     per costruzione in tutta quella suite, senza bisogno di una condizione
     esplicita in ognuno. I due soli punti che l'accendono: AppRun piu' sotto
     (il gioco vero) e --catalog-test (src/tests/catalog_tests.c), che la
     accende a mano sulla propria AppUi locale per esercitare la scrittura
     vera attraverso UpdateApp, esattamente come ogni altro *Test di
     game_tests.c esercita UpdateApp per davvero.
   - 'game->floor < 1': nessun piano davvero giocato (Piano 0, o un abbandono
     prima di attraversare il varco) -- niente da registrare.
   Il resto (contenuto DAVVERO generato o no, categorie con qualcosa da
   scrivere o no) e' responsabilita' di RunCatalogWriteRun (src/content/
   run_catalog.c): questa funzione non lo anticipa, solo whether-to-call. MAI
   blocca la transizione che la chiama: il chiamante non controlla mai
   'game->catalogRecordsWritten' per decidere se procedere, solo per il
   feedback di RunResults (DrawRunResultsOverlay, src/render/game_renderer.c). */
static void AppWriteRunCatalog(Game *game, AppUi *ui, const char *outcome)
{
    game->catalogRecordsWritten = 0;
    if (!ui->catalogWritesEnabled) return;
    if (game->floor < 1) return;
    game->catalogRecordsWritten = RunCatalogWriteRun(game, ui->seed, outcome);
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

    /* W9 correzione round 0 (BOCCIATO): 'mouseMoved' e' l'unico gate che i tre
       punti di questo file che leggono il mouse in modo CONTINUO (il passo
       hover generico qui sotto, le righe di BuildScreen, le carte/schedine del
       Piano 0) devono rispettare prima di scrivere un focus -- vedi il
       commento su AppUi.mouseTracked in game_types.h per il perche'. I CLICK
       (IsMouseButtonPressed) restano eventi discreti e NON passano da questo
       gate: un click va sempre onorato, si sia mosso o no il mouse in quel
       frame. 'lastMousePos'/'mouseTracked' si aggiornano qui, una volta sola
       per frame, PRIMA di ogni ramo che li legge. */
    Vector2 mousePos = GetMousePosition();
    bool mouseMoved = !ui->mouseTracked ||
                       mousePos.x != ui->lastMousePos.x || mousePos.y != ui->lastMousePos.y;
    ui->lastMousePos = mousePos;
    ui->mouseTracked = true;

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
    /* Vero SOLO quando il confirm sintetico sotto viene da un CLICK su una
       voce di menu. Serve a BuildScreen, l'unico stato in cui "conferma" ha
       due significati diversi (la riga "Indietro" chiude la schermata, ma
       ENTER/SPAZIO sulla lista oggetti selezionano per la fusione): senza
       questo, un click su "Indietro" selezionerebbe un oggetto invece di
       uscire. Gli altri stati non lo leggono affatto. */
    bool menuClick = false;
    /* M8: la vista Catalogo (DENTRO APP_MAIN_MENU, nessun nuovo AppMode) ha
       una geometria propria (categorie/righe, DrawCatalogOverlay), NON quella
       delle 4 voci di menu che RendererMenuItemAt conosce per questo mode --
       un click qui sotto interrogherebbe i rettangoli SBAGLIATI (quelli del
       menu che non e' nemmeno disegnato in questo momento), potendo
       corrompere ui->focus/simulare un confirm a caso. v1 resta quindi
       tastiera-solo DENTRO la vista (parita' tastiera comunque garantita,
       DEC-057: il mouse resta "ammesso", mai l'unica via); la voce "Catalogo"
       stessa, nel menu, resta cliccabile come ogni altra voce quando la vista
       e' chiusa. */
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !(*mode == APP_MAIN_MENU && ui->catalogOpen))
    {
        int clicked = RendererMenuItemAt(*mode, mousePos);
        if (clicked >= 0)
        {
            ui->focus = clicked;
            /* W9 correzione round 0 (MINORE): in Options un click su una
               riga-slider (indice < APP_OPTIONS_ROW_BACK) sposta il focus e
               apre il trascinamento (vedi APP_OPTIONS sotto), ma NON e' un
               "confirm" -- ENTER su una riga-slider non ha significato
               (docs/design/ui/options-and-accessibility.md), quindi il click
               non deve sintetizzarne uno: niente sfx UI_CONFIRM ad ogni
               inizio di trascinamento, e nessun rischio di attraversare per
               sbaglio il ramo "back" del case sotto. La riga "Indietro"
               (indice APP_OPTIONS_ROW_BACK) e ogni voce di menu negli altri
               stati restano un confirm pieno, come sempre. */
            bool isOptionsSliderClick = (*mode == APP_OPTIONS && clicked < APP_OPTIONS_ROW_BACK);
            if (!isOptionsSliderClick)
            {
                effective.confirm = true;
                menuClick = true;
            }
        }
    }

    /* W9 (playtest round 1, "mouse ovunque"): il solo PASSAGGIO del mouse
       (nessun click) sposta gia' il focus su qualunque voce di menu STANDARD
       sotto il puntatore -- stessa geometria del click appena sopra
       (RendererMenuItemAt e' la fonte unica), cosi' "in evidenza" e
       "cliccabile per attivarla" restano sempre la stessa voce. SOLO quando
       'mouseMoved' (W9 correzione round 0, BOCCIATO): un puntatore lasciato
       fermo su una voce -- la situazione normale dopo un click, o quando il
       giocatore e' tornato a navigare da tastiera/pad -- non deve piu'
       riscrivere il focus ad ogni frame (rompeva DEC-057: tastiera/pad
       smettevano di funzionare per la sola presenza del mouse, vedi il
       commento su AppUi.mouseTracked). Non tocca 'effective': un hover da
       solo non e' mai un confirm sintetico. Sotto test resta un no-op non per
       caso ma per costruzione: le suite che pilotano UpdateApp con eventi
       sintetici parcheggiano il cursore virtuale in (2,2) prima del primo
       frame (GameStatesTest/GameMouseHoverFocusTest, src/tests/game_tests.c),
       punto fuori da ogni riquadro di menu -- che sono CENTRATI -- quindi
       nessuna geometria risponde e nessun focus viene riscritto, qualunque
       dimensione abbia la finestra di Xvfb. Stessa esclusione della vista Catalogo del blocco sopra
       (altra geometria, RendererMenuItemAt risponderebbe con gli indici del
       menu sottostante). Le geometrie AGGIUNTIVE di BuildScreen (righe
       oggetti) e FloorZero (carte/schedine/fumetto del pannello, DEC-075)
       vivono nei rispettivi case dello switch sotto, con le loro funzioni
       dedicate (RendererBuildItemRowAt/RendererFloorZeroCardAt e affini). */
    if (mouseMoved && !(*mode == APP_MAIN_MENU && ui->catalogOpen))
    {
        int hovered = RendererMenuItemAt(*mode, mousePos);
        if (hovered >= 0) ui->focus = hovered;
    }

    /* Feedback sonoro generico di navigazione/conferma/annulla nei menu
       (docs/design/content/audio-and-feedback.md): le frecce/ENTER/ESC di
       'effective' pilotano SOLO la navigazione a menu in ogni stato tranne
       APP_GAMEPLAY, dove il movimento vero passa da IsKeyDown in combat.c,
       mai da qui (vedi il commento su AppInput in app_internal.h) -- una
       sola classificazione qui evita di instrumentare ogni ramo dello
       switch sotto. In APP_FLOOR_ZERO le stesse frecce muovono il pannello
       COMBINATO MONDI/PERSONAGGI solo quando e' aperto (altrimenti sono
       lettere morte per lo switch sotto): la guardia sul pannello evita un
       blip udibile senza alcun effetto visibile mentre si gira nell'hub. */
    bool menuNavContext = (*mode != APP_GAMEPLAY) && (*mode != APP_FLOOR_ZERO || game->themeCardsPanelOpen);
    if (menuNavContext)
    {
        if (effective.up || effective.down || effective.left || effective.right) AudioPlaySfx(AUDIO_SFX_UI_MOVE);
        else if (effective.confirm) AudioPlaySfx(AUDIO_SFX_UI_CONFIRM);
        else if (effective.back) AudioPlaySfx(AUDIO_SFX_UI_CANCEL);
    }

    switch (*mode)
    {
        case APP_MAIN_MENU:
        {
            /* M8 (DEC-045): la vista Catalogo vive DENTRO questo stato (nessun
               nuovo AppMode, nota architetturale della spec M8) -- un ramo
               SEPARATO, controllato PRIMA di tutto il resto: quando e' aperta,
               su/giu/sinistra/destra/back muovono la VISTA, mai le voci del
               menu sottostante (ui->focus non viene mai toccato qui, resta
               fermo su 1/"Catalogo" per tutta la permanenza nella vista). v1 e'
               SOLO consultazione (spec M8, "SOLO l'enciclopedia"): confirm non
               fa nulla, non c'e' ancora un'azione da compiere su una voce. */
            if (ui->catalogOpen)
            {
                if (effective.back) { ui->catalogOpen = false; break; }   /* torna a MainMenu, focus gia' su "Catalogo" */
                if (effective.left || effective.right)
                {
                    ui->catalogCategory = (ui->catalogCategory + RUN_CATALOG_CATEGORY_COUNT + (effective.right ? 1 : -1)) % RUN_CATALOG_CATEGORY_COUNT;
                    ui->catalogItemFocus = 0;   /* mai un indice che punta a una voce di un'altra lista */
                    break;
                }
                if (effective.up || effective.down)
                {
                    int count = ui->catalog.entryCount[ui->catalogCategory];
                    if (count > 0) ui->catalogItemFocus = (ui->catalogItemFocus + count + (effective.down ? 1 : -1)) % count;
                    break;   /* categoria vuota: su/giu' non fa nulla, mai una divisione per zero */
                }
                break;
            }

            if (effective.up || effective.down) { ui->focus = (ui->focus + 4 + (effective.down ? 1 : -1)) % 4; break; }
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
                else if (ui->focus == 1)   /* Catalogo (M8, DEC-045): aggregazione ON-DEMAND, mai per-frame */
                {
                    RunCatalogAggregate(&ui->catalog);
                    ui->catalogOpen = true;
                    ui->catalogCategory = 0;
                    ui->catalogItemFocus = 0;
                }
                else if (ui->focus == 2)   /* Opzioni */
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
            /* M5 (DEC-005), requisito 1/8: finche' il tema non e' scelto E
               le carte non sono ancora pronte, si sonda proposeRunner --
               successo -> le carte del JSON (o il ripiego, se il JSON non
               valida: mai un errore visibile, requisito 11); fallimento ->
               le carte curate lato gioco, SUBITO. Una volta che
               themeCardCount>0 questo blocco non fa piu' nulla per il resto
               della permanenza nel Piano 0 (stesso schema idempotente del
               blocco "pipeline principale" sotto). */
            if (game->themeChosenIndex < 0 && game->themeCardCount == 0 &&
                gen->proposeRunner.state == GEN_RUNNER_RUNNING)
            {
                GenRunnerUpdate(&gen->proposeRunner);
                if (gen->proposeRunner.state == GEN_RUNNER_SUCCEEDED)
                {
                    if (!AppLoadThemeCards(game, "generated/theme_proposals.json"))
                        AppUseFallbackThemeCards(game, gen->pendingGenSeed);
                }
                else if (gen->proposeRunner.state == GEN_RUNNER_FAILED)
                {
                    AppUseFallbackThemeCards(game, gen->pendingGenSeed);
                }
                /* M6b-1 (DEC-014): il personaggio generato viaggia nella
                   STESSA chiamata --propose-themes dei temi (mai un secondo
                   processo/modello), ma la sua riuscita e' INDIPENDENTE da
                   quella dei temi -- tentare il caricamento qui, su
                   qualunque esito TERMINALE del runner (successo O
                   fallimento), e' corretto: character_proposal.json o c'e'
                   ed e' valido, o non c'e' affatto (fallback canonico =
                   carta assente, mai un errore), indipendentemente da come
                   sono andati i temi. La guardia 'state != RUNNING' (invece
                   di richiamarla ad ogni frame mentre il propose e' ancora
                   in corso) e' cio' che evita di leggere ripetutamente un
                   file residuo di un propose PRECEDENTE prima che quello di
                   QUESTA run abbia anche solo iniziato a scrivere. */
                if (gen->proposeRunner.state != GEN_RUNNER_RUNNING) AppLoadCharacterProposal(game);
            }

            /* M5/M6a, requisito 9/3: il pannello COMBINATO MONDI/PERSONAGGI --
               un tasto dedicato (TAB) lo apre/chiude, cosi' le frecce dentro
               non "rubano" i controlli di movimento (WASD) del Piano 0
               giocabile quando il pannello e' chiuso (il caso normale: il
               giocatore gira nell'hub mentre aspetta le proposte). Disponibile
               solo da quando le carte-mondo sono pronte (stessa condizione di
               M5: 'themeCardCount>0' -- i personaggi curati sarebbero gia'
               pronti prima, ma legare le due sezioni alla stessa condizione
               evita un secondo timing da testare per un guadagno che nessuno
               nota, dato quanto e' breve l'attesa delle proposte). A
               differenza di M5, il pannello NON sparisce da solo quando il
               tema e' scelto: la sezione PERSONAGGI resta interattiva per
               tutta la permanenza nel Piano 0 (requisito 1: "sempre
               modificabile finche' non si attraversa l'uscita"); solo
               AppConfirmThemeChoice (guardata/idempotente) fa si' che
               confermare di nuovo un mondo, dopo il primo, non abbia effetto.
               Su/giu' cambia sezione (con wrap fra le due, mai un terzo
               stato); sinistra/destra e conferma agiscono SOLO dentro la
               sezione col focus. */
            if (game->themeCardCount > 0)
            {
                if (effective.tab) game->themeCardsPanelOpen = !game->themeCardsPanelOpen;
                /* W9 (playtest round 1, DEC-075): il click sul fumetto "TAB --
                   mondo e personaggio" apre il pannello esattamente come TAB
                   -- il Piano 0 conta come menu ai fini del mouse, e questo
                   fumetto e' l'unico invito visibile a pannello chiuso. */
                if (!game->themeCardsPanelOpen && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                    RendererFloorZeroHintChipAt(game, mousePos))
                    game->themeCardsPanelOpen = true;

                if (game->themeCardsPanelOpen)
                {
                    if (effective.up || effective.down)
                        game->floorZeroPanelSection = (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS)
                                                       ? FLOOR_ZERO_PANEL_CHARACTERS : FLOOR_ZERO_PANEL_WORLDS;

                    /* W9: le due schedine MONDI/PERSONAGGI sono cliccabili,
                       stessa azione di su/giu' da tastiera. */
                    int tabHit = RendererFloorZeroSectionTabAt(game, mousePos);
                    if (tabHit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        game->floorZeroPanelSection = (tabHit == 0) ? FLOOR_ZERO_PANEL_WORLDS : FLOOR_ZERO_PANEL_CHARACTERS;

                    /* W9: hover su una carta della sezione ATTIVA sposta il
                       focus (come una voce di menu), un click sopra la
                       conferma -- stesso effetto di sinistra/destra + conferma
                       da tastiera (DEC-075: il Piano 0 conta come menu).
                       W9 correzione round 0 (BLOCCANTE): l'assegnazione del
                       focus e' gated da 'mouseMoved' -- altrimenti un mouse
                       lasciato fermo su una carta ruba per sempre la
                       selezione a sinistra/destra da tastiera/pad, rendendo
                       la scelta del mondo (irreversibile: avvia la
                       generazione) quella sotto il puntatore invece di quella
                       scelta col pad. Il click resta un evento discreto, non
                       passa dal gate. */
                    int cardHit = RendererFloorZeroCardAt(game, mousePos);
                    bool cardClicked = (cardHit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
                    /* W9 correzione round 1 (BLOCCANTE): il click sposta il
                       focus ANCHE se il mouse non si e' mosso in questo frame
                       -- come fa da sempre il blocco generico dei menu qui
                       sopra ("ui->focus = clicked", non condizionato). Senza
                       questo, un puntatore lasciato fermo su una carta con il
                       focus altrove (pannello aperto con TAB, o focus spostato
                       da tastiera: 'mouseMoved' falso, nessun hover) faceva
                       confermare la carta SBAGLIATA -- e la scelta del mondo e'
                       IRREVERSIBILE (avvia la generazione). */
                    if (cardHit >= 0 && (mouseMoved || cardClicked))
                    {
                        if (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS) game->themeCardFocus = cardHit;
                        else game->characterCardFocus = cardHit;
                    }

                    if (game->floorZeroPanelSection == FLOOR_ZERO_PANEL_WORLDS)
                    {
                        if (effective.left)
                            game->themeCardFocus = (game->themeCardFocus + game->themeCardCount - 1)%game->themeCardCount;
                        if (effective.right)
                            game->themeCardFocus = (game->themeCardFocus + 1)%game->themeCardCount;
                        /* W9 correzione round 1 (BLOCCANTE): il click conferma
                           la carta DAVVERO cliccata ('cardHit'), non il focus
                           corrente -- che le due frecce qui sopra possono
                           persino aver spostato in questo stesso frame. La
                           scelta del mondo avvia la generazione e non si torna
                           indietro: qui non ci sono margini per un'indirezione
                           in piu'. La regola vive in
                           AppFloorZeroCardToConfirm (nucleo puro, testabile
                           senza click veri: vedi app_internal.h). */
                        int themeChoice = AppFloorZeroCardToConfirm(cardHit, cardClicked, effective.confirm, game->themeCardFocus);
                        if (themeChoice >= 0) AppConfirmThemeChoice(game, gen, themeChoice);
                    }
                    else
                    {
                        /* M6b-1: il conteggio e' DINAMICO (CHARACTER_COUNT
                           piu' uno quando il quarto slot generato e' valido,
                           vedi GameCharacterCardCount) -- se il focus era
                           gia' sul quarto slot e la carta sparisce (non
                           dovrebbe succedere dentro una permanenza nel Piano
                           0, ma resta una difesa a buon mercato), il modulo
                           lo riporta comunque dentro banda. */
                        int cardCount = GameCharacterCardCount(game);
                        if (game->characterCardFocus >= cardCount) game->characterCardFocus = cardCount - 1;
                        if (effective.left)
                            game->characterCardFocus = (game->characterCardFocus + cardCount - 1)%cardCount;
                        if (effective.right)
                            game->characterCardFocus = (game->characterCardFocus + 1)%cardCount;
                        /* Stessa regola per le carte-personaggio (W9 correzione
                           round 1), stessa funzione: conferma quella cliccata.
                           'cardHit' viene da RendererFloorZeroCardAt, che nella
                           sezione PERSONAGGI conta esattamente le carte
                           disegnate (GameCharacterCardCount, la stessa rosa
                           dinamica di 'cardCount' qui sopra), quindi e' sempre
                           un indice valido per AppConfirmCharacterChoice. */
                        int characterChoice = AppFloorZeroCardToConfirm(cardHit, cardClicked, effective.confirm, game->characterCardFocus);
                        if (characterChoice >= 0) AppConfirmCharacterChoice(game, characterChoice);
                    }
                }
            }

            /* M1b: la sala d'attesa giocabile. La generazione corre in
               sottofondo, MAI bloccante -- il giocatore continua a muoversi
               (AppSimStep tratta questo stato come Gameplay per la
               simulazione). Finche' l'uscita non e' aperta si sonda il
               runner di primo piano attivo (testo, poi se pianificato lo
               sprite) e si apre l'uscita non appena la pipeline diventa
               TERMINALE -- anche in caso di fallimento: i contenuti gia' su
               disco sono la riserva, il gioco resta sempre avviabile
               (DEC-002/DEC-020, fallback SILENZIOSO: qui non si distingue
               successo da fallback, vedi AppOpenFloorZeroExit). Una volta
               aperta, questo blocco non fa piu' nulla (game->floorZeroExitOpen
               resta vero per il resto della permanenza nel Piano 0).
               M5, requisito 10: la pipeline principale non e' nemmeno
               partita finche' il tema non e' scelto (gen->runner.state resta
               GEN_RUNNER_IDLE, mai SUCCEEDED/FAILED), quindi la guardia
               'game->themeChosenIndex >= 0' qui sotto e' ridondante per
               costruzione con quello stato -- la si scrive comunque, esplicita:
               "l'uscita si apre solo se tema scelto E pipeline terminale" e'
               un requisito di design, non solo un effetto collaterale
               dell'ordine delle chiamate. */
            if (game->themeChosenIndex >= 0 && !game->floorZeroExitOpen)
            {
                GenRunner *active = gen->inSpritesStage ? &gen->spritesRunner : &gen->runner;
                GenRunnerUpdate(active);
                if (active->state == GEN_RUNNER_SUCCEEDED)
                {
                    if (!gen->inSpritesStage && gen->spritesPlannedThisRun)
                    {
                        /* Passo testo riuscito e modelli SD presenti: si passa
                           al passo sprite invece di aprire subito l'uscita
                           (l'indicatore passa a "Il mondo prende forma...",
                           vedi AppFloorZeroStatusText). */
                        if (AppStartSpritesGeneration(gen)) gen->inSpritesStage = true;
                        /* fork() fallita (rarissimo): pipeline comunque
                           conclusa, si gioca con l'atlas gia' scritto dal
                           passo testo -- mai bloccare l'uscita per un secondo
                           passo che non e' nemmeno partito. */
                        else AppOpenFloorZeroExit(game);
                    }
                    else AppOpenFloorZeroExit(game);
                }
                else if (active->state == GEN_RUNNER_FAILED) AppOpenFloorZeroExit(game);
            }

            /* Attraversamento (segnalato da WorldHandleTransitions, world.c,
               quando il giocatore preme contro il varco APERTO): SOLO qui
               scattano GameResetRun (carica manifest/atlas nuovi, o quelli
               di riserva) e la ripresa in sottofondo dei piani 2-5 -- stessa
               condizione di sempre (Step B2): solo se il passo TESTO e'
               andato a buon fine, perche' la ripresa rilegge
               generated/current_run.json con lo STESSO seed di quella run. */
            if (game->floorZeroExitCrossed)
            {
                game->floorZeroExitCrossed = false;
                /* M6a, requisito 2/4c: GameResetRun azzera l'INTERO Game con
                   un memset (compreso characterChosenIndex), quindi l'indice
                   scelto va catturato PRIMA di chiamarla e riapplicato SUBITO
                   dopo -- la run parte con le statistiche del personaggio
                   scelto nel Piano 0, non con lo storico "nessun personaggio"
                   che GameResetRun applicherebbe da sola (comportamento
                   invariato per ogni ALTRO chiamante di GameResetRun, es. i
                   test che la chiamano direttamente senza mai passare dal
                   Piano 0: vedi il commento su GamePlayerResetBaseStatsFor in
                   game.c). ScriptItemsInit rideriva damage/fireDelay/... dai
                   nuovi base* (0 oggetti posseduti a inizio piano 1, come
                   sempre).
                   M6b-1 (DEC-014): se il personaggio scelto e' quello
                   GENERATO (chosenCharacter == CHARACTER_COUNT), l'indice da
                   solo non basta piu' -- il memset di GameResetRun cancella
                   ANCHE Game.generatedCharacter/generatedCharacterValid, e
                   GameResolveCharacterDef non troverebbe piu' nulla da
                   applicare. Si cattura quindi una COPIA della def generata
                   PRIMA del memset e la si riscrive subito dopo, esattamente
                   come si fa per l'indice -- la run generata sopravvive
                   all'attraversamento tanto quanto quella curata.
                   DEC-141: GameResetRunWithSeed, non piu' GameResetRun --
                   'gen->pendingGenSeed' e' il seed che AppEnterFloorZero ha
                   gia' deciso per QUESTA run (RunSetup/reroll/RunResults,
                   vedi il commento sul campo in app_internal.h), lo stesso
                   passato a melting-gen: il gameplay (spawn/drop/combattimento)
                   ora deriva da quel seed quanto il contenuto generato, non
                   piu' dall'orologio. */
                int chosenCharacter = game->characterChosenIndex;
                bool chosenIsGenerated = (chosenCharacter == CHARACTER_COUNT && game->generatedCharacterValid);
                CharacterDef savedGenerated = chosenIsGenerated ? game->generatedCharacter : (CharacterDef){ 0 };
                GameResetRunWithSeed(game, gen->pendingGenSeed);
                game->characterChosenIndex = chosenCharacter;
                if (chosenIsGenerated)
                {
                    game->generatedCharacter = savedGenerated;
                    game->generatedCharacterValid = true;
                }
                const CharacterDef *chosenDef = GameResolveCharacterDef(game, chosenCharacter);
                if (chosenDef) GamePlayerResetBaseStatsFor(&game->player, chosenDef);
                ScriptItemsInit(game, chosenDef);
                *mode = APP_GAMEPLAY;
                if (gen->runner.state == GEN_RUNNER_SUCCEEDED) AppStartLazyGeneration(gen);
                break;
            }

            if (effective.back)
            {
                /* Azione distruttiva: passa da ExitConfirm come ogni altro
                   abbandono (DEC-057), mai una cancellazione diretta -- il
                   contesto "abbandona la preparazione" lo sceglie
                   DrawExitConfirmOverlay da ui->openedFrom. La generazione
                   NON viene toccata qui: continua in sottofondo finche' non
                   si conferma davvero (AppCancelFloorZeroGeneration, sotto). */
                ui->openedFrom = APP_FLOOR_ZERO;
                ui->returnFocus = 0;
                ui->exitAbandonsRun = true;
                *mode = APP_EXIT_CONFIRM;
                ui->focus = 1;   /* default: "Annulla" */
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
                AppWriteRunCatalog(game, ui, game->phase == PHASE_WIN ? RUN_CATALOG_OUTCOME_WIN : RUN_CATALOG_OUTCOME_LOSS);
                *mode = APP_RUN_RESULTS;
                ui->focus = 0;
                break;
            }
            if (effective.pause || effective.back) { *mode = APP_PAUSE_MENU; ui->focus = 0; break; }
            if (effective.tab)
            {
                AppEnterBuildScreen(game, ui, APP_GAMEPLAY);
                *mode = APP_BUILD_SCREEN;
                break;
            }
            if (effective.reroll)
            {
                /* M7: terzo chiamante dell'hook (spec, punto 3) -- il reroll
                   ABBANDONA la run corrente per rigenerarne una nuova, esattamente
                   come una conferma di abbandono, quindi registra PRIMA di
                   toccare 'game' in qualunque modo (sia AppEnterFloorZero, che
                   azzera player/rooms via FloorZeroEnter, sia resetQueued, che lo
                   fara' al prossimo GameUpdate): un ordine invertito perderebbe
                   esattamente gli oggetti/nemici incontrati che il catalogo deve
                   registrare. Se la run era fallback (source=fallback, es. gen
                   disabilitata) RunCatalogWriteRun non scrive nulla da sola,
                   nessuna guardia aggiuntiva serve qui. */
                AppWriteRunCatalog(game, ui, RUN_CATALOG_OUTCOME_ABANDON);
                /* Con generazione: nuova run con seed nuovo, stesso cammino
                   canonico di RunSetup/Avvia (niente scorciatoie). Senza: il
                   reset rapido dev di sempre, latchato per il passo che lo
                   consuma (vedi il commento in game_types.h). */
                if (gen->enabled) AppEnterFloorZero(game, gen, mode, NextGenSeed(0u));
                else game->resetQueued = true;
                break;
            }
            if (effective.bomb) game->bombQueued = true;
            /* Slot funzionali: solo dentro Gameplay, come la bomba -- in un
               menu E e G non devono fare nulla. */
            if (effective.useActive) game->useActiveQueued = true;
            if (effective.dropGraft) game->dropGraftQueued = true;
            /* DEC-184: solo un toggle di visibilita' sull'HUD, nessun effetto
               sulla simulazione -- vive su 'ui' (sopravvive a GameResetRun),
               mai su 'game'. */
            if (effective.toggleStats) ui->hudStatsHidden = !ui->hudStatsHidden;
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
                    AppEnterBuildScreen(game, ui, APP_PAUSE_MENU);
                    *mode = APP_BUILD_SCREEN;
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
            /* W8 (chiude la parte UI del difetto noto 9, categoria "audio" di
               ui/options-and-accessibility.md): tre volumi piu' "Indietro".
               Su/giu' scelgono la riga, sinistra/destra cambiano il valore
               della riga scelta, ENTER/ESC escono -- la stessa grammatica di
               ogni altro menu, quindi la parita' tastiera/controller di
               DEC-057 vale senza codice dedicato.
               ESC esce SEMPRE; ENTER esce solo dalla riga "Indietro": su una
               riga-slider un ENTER non ha significato (non c'e' nulla da
               confermare, il valore e' gia' applicato) e chiudere la schermata
               sarebbe una sorpresa mentre si sta regolando. */
            const int OPTIONS_ROWS = 4;
            const int OPTIONS_ROW_BACK = APP_OPTIONS_ROW_BACK;
            /* W9 (playtest round 1, "mouse ovunque"): le tre barre diventano
               TRASCINABILI col mouse -- il click iniziale su una barra applica
               subito il valore sotto il puntatore e apre il trascinamento
               ('ui->optionsDragging'), che prosegue finche' il tasto resta
               premuto (anche se il mouse esce dalla riga in verticale: solo la
               X conta durante il trascinamento, come ci si aspetta da uno
               slider), e si chiude al rilascio. RendererMenuItemAt(APP_OPTIONS,
               ...) e' la STESSA query del blocco generico sopra (gia' chiamata
               per il click/hover della riga): qui si aggiunge solo cosa fare
               quando la riga colpita e' una delle tre barre (indice <
               OPTIONS_ROW_BACK), non "Indietro". */
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                int hitRow = RendererMenuItemAt(APP_OPTIONS, mousePos);
                /* Il trascinamento si apre SOLO dalla barra (RendererOptions-
                   SliderHit): un click sull'etichetta o sulle frecce della
                   riga e' navigazione (l'hover generico sopra ha gia' spostato
                   il focus) e non deve toccare il volume -- senza il cancello,
                   il clamp di ValueAt trasformava quei click in 0%/100%. */
                if (hitRow >= 0 && hitRow < OPTIONS_ROW_BACK &&
                    RendererOptionsSliderHit(hitRow, mousePos))
                {
                    ui->optionsDragging = true;
                    ui->optionsDraggingIndex = hitRow;
                }
            }
            if (ui->optionsDragging)
            {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                {
                    float v = RendererOptionsSliderValueAt(ui->optionsDraggingIndex, mousePos.x);
                    if (ui->optionsDraggingIndex == 0) AudioSetMasterVolume(v);
                    else if (ui->optionsDraggingIndex == 1) AudioSetMusicVolume(v);
                    else AudioSetSfxVolume(v);
                }
                else ui->optionsDragging = false;   /* rilasciato: fine del trascinamento */
            }
            if (effective.up || effective.down)
            {
                ui->focus = (ui->focus + OPTIONS_ROWS + (effective.down ? 1 : -1))%OPTIONS_ROWS;
                break;
            }
            if ((effective.left || effective.right) && ui->focus < OPTIONS_ROW_BACK)
            {
                float delta = effective.right ? OPTIONS_VOLUME_STEP : -OPTIONS_VOLUME_STEP;
                /* Nessun clamp qui: le tre AudioSet* clampano gia' in [0,1]
                   per contratto (audio.h), ed e' giusto che il clamp viva in un
                   solo posto -- quello che possiede il valore. */
                if (ui->focus == 0) AudioSetMasterVolume(AudioGetMasterVolume() + delta);
                else if (ui->focus == 1) AudioSetMusicVolume(AudioGetMusicVolume() + delta);
                else AudioSetSfxVolume(AudioGetSfxVolume() + delta);
                /* Il suono di navigazione FA da anteprima del volume appena
                   scelto: e' l'unico modo di sentire l'effetto di uno slider
                   SFX senza uscire dal menu. */
                AudioPlaySfx(AUDIO_SFX_UI_MOVE);
                break;
            }
            if (effective.back || (effective.confirm && ui->focus == OPTIONS_ROW_BACK))
            {
                /* W9 correzione round 0 (MINORE): 'optionsDragging' si chiude
                   anche qui, non solo al rilascio del tasto sopra -- altrimenti
                   un'uscita (ESC, o back sintetico da click su "Indietro")
                   mentre il tasto e' ancora premuto lascerebbe il flag vero, e
                   riprenderebbe da solo alla visita successiva di Options se
                   il tasto risultasse ancora premuto in quel momento. */
                ui->optionsDragging = false;
                *mode = ui->openedFrom;
                ui->focus = ui->returnFocus;
            }
            break;
        }

        case APP_BUILD_SCREEN:
        {
            /* W9 (playtest round 1, "mouse ovunque"): le righe della lista
               OGGETTI PRESI sono cliccabili per scegliere le sorgenti della
               fusione (DEC-057/DEC-143) -- hover sposta 'buildItemFocus'
               come una voce di menu qualunque, click seleziona/deseleziona
               esattamente come ENTER da tastiera (AppFusionToggle, la stessa
               funzione che AppUpdateBuildScreen sotto usa per il tasto).
               Geometria SEPARATA dal 'menuClick' generico calcolato sopra
               (che per questo stato significa SOLO "Indietro", l'unica voce
               che RendererMenuItemAt conosce qui): un click su una riga
               oggetto non deve mai essere scambiato per un'uscita dalla
               schermata. RendererBuildItemRowAt ritorna -1 se il punto non
               cade su nessuna riga DAVVERO disegnata in questo momento
               (finestra scorrevole), quindi fuori dalla lista non fa nulla.
               W9 correzione round 1: la finestra visibile dipende ORA
               dall'ancora 'ui->buildItemScroll', non piu' dal focus -- il
               hit-test qui sotto e' quindi indipendente dal campo che l'hover
               scrive (nessun anello di retroazione, vedi
               AppBuildScrollFollowFocus). Prima riga di questo case: allineare
               l'ancora, cosi' la geometria interrogata dal mouse e' quella che
               il giocatore ha davvero davanti anche se l'inventario o la
               finestra sono cambiati mentre la schermata era chiusa. */
            AppBuildScrollFollowFocus(game, ui);
            int hoveredBuildRow = RendererBuildItemRowAt(game, ui, mousePos);
            if (hoveredBuildRow >= 0)
            {
                /* W9 correzione round 0 (BLOCCANTE): il focus si sposta SOLO
                   quando il mouse si e' mosso davvero -- un puntatore lasciato
                   fermo su una riga non deve rubare la selezione a
                   tastiera/pad (DEC-057, vedi il commento su
                   AppUi.mouseTracked in game_types.h). Il CLICK resta un
                   evento discreto e non passa da questo gate: agisce sulla
                   riga DAVVERO sotto il puntatore ('hoveredBuildRow'), non sul
                   focus, quindi non dipende dall'hover che potrebbe non essere
                   girato. */
                if (mouseMoved) ui->buildItemFocus = hoveredBuildRow;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) AppFusionToggle(ui, hoveredBuildRow);
            }
            /* W9 correzione round 1 (MINORE): la riga di stato della fascia
               FUSIONE e' cliccabile e vale [F] -- senza questa, nessun
               percorso col SOLO mouse portava a termine una fusione (le
               sorgenti si scelgono col click, ma la conferma era solo da
               tastiera). Stessa funzione del tasto (AppFusionConfirm), quindi
               anche lo stesso messaggio di esito quando la fusione non si puo'
               fare. */
            /* Feedback di hover della riga di conferma (correzione di Fable):
               la riga e' un bersaglio di click, quindi deve dirlo anche al
               passaggio del mouse, come ogni altra superficie cliccabile. */
            ui->fusionConfirmHover = (hoveredBuildRow < 0) &&
                                     RendererFusionConfirmAt(game, mousePos);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ui->fusionConfirmHover)
                AppFusionConfirm(game, ui);
            /* W9: la lista e' una finestra SCORREVOLE (RendererBuildItemRowAt/
               BuildScreenItemListLayoutFor in game_renderer.c: solo le righe
               DAVVERO disegnate in questo momento sono cliccabili) -- senza la
               rotellina, un giocatore SOLO mouse non potrebbe mai raggiungere
               un oggetto oltre la finestra iniziale (l'hover da solo non
               sposta mai 'buildItemFocus' fuori da quello che gia' vede).
               La rotellina muove il focus come su/giu' da tastiera e l'ancora
               lo segue (AppBuildScrollFollowFocus in fondo al case): verso
               l'alto (valore positivo di GetMouseWheelMove) avvicina la cima
               della lista, esattamente come il tasto SU. */
            {
                float wheel = GetMouseWheelMove();
                if (wheel != 0.0f)
                {
                    int count = GameMathClampInt(game->player.itemCount, 0, MAX_ITEMS);
                    if (count > 0)
                        ui->buildItemFocus = GameMathClampInt(ui->buildItemFocus - (int)wheel, 0, count - 1);
                }
            }

            /* Uscita: ESC, TAB (DEC-139, arco diretto verso Gameplay) o il
               click sulla riga "Indietro". Il confirm da TASTIERA non chiude
               piu' la schermata: qui dentro significa "seleziona questo
               oggetto" (ui/inventory-and-synergy-screen.md, riga "Lista
               oggetti acquisiti"), primo passo della fusione. Le due vie di
               uscita documentate (TAB e "Indietro") restano entrambe. */
            if (effective.back || effective.tab || menuClick)
            {
                *mode = ui->openedFrom;
                ui->focus = ui->returnFocus;
                AppFusionClearSelection(ui);
                break;
            }
            AppUpdateBuildScreen(game, ui, &effective);
            /* W9 correzione round 1: su/giu' da tastiera, la rotellina o
               l'esito di una fusione possono aver portato il focus FUORI dalla
               finestra visibile -- l'ancora lo segue subito, prima che il
               renderer disegni questo stesso frame, cosi' la riga a fuoco e'
               sempre visibile (la garanzia della vecchia derivazione, senza il
               suo anello di retroazione). */
            AppBuildScrollFollowFocus(game, ui);
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
                    if (ui->exitAbandonsRun)
                    {
                        /* M7: secondo chiamante dell'hook (spec, punto 3) --
                           l'abbandono confermato. Un unico punto per ENTRAMBE le
                           origini di questo dialogo (openedFrom FLOOR_ZERO o
                           PAUSE_MENU): niente da distinguere qui, la guardia
                           "game->floor < 1" dentro AppWriteRunCatalog esclude gia'
                           da sola l'abbandono della preparazione nel Piano 0
                           (floor resta 0 li', WorldStartFloor non e' mai girata). */
                        AppWriteRunCatalog(game, ui, RUN_CATALOG_OUTCOME_ABANDON);
                        /* Abbandono dalla preparazione (M1b): i runner di primo
                           piano attivi vanno cancellati SOLO qui, alla conferma
                           vera -- non al semplice ESC che apre il dialogo (la
                           generazione continua in sottofondo mentre il
                           giocatore decide, vedi il case APP_FLOOR_ZERO). */
                        if (ui->openedFrom == APP_FLOOR_ZERO) AppCancelFloorZeroGeneration(gen);
                        *mode = APP_MAIN_MENU; ui->focus = 0;
                    }
                    else return true;   /* uscita dall'applicazione: l'UNICO modo, niente piu' scorciatoie dirette */
                }
                else { *mode = ui->openedFrom; ui->focus = ui->returnFocus; }   /* Annulla */
            }
            break;
        }
    }
    return false;
}

/* Un passo di simulazione a dt FISSO: il gioco vero in Gameplay E in
   FloorZero (M1b: la sala d'attesa e' giocabile, il giocatore ci si muove e
   ci spara mentre la generazione corre in sottofondo -- vedi
   FloorZeroEnter/WorldHandleTransitions), le sole particelle cosmetiche in
   tutti gli altri stati (menu, pausa, build, risultati...). Il mouse viene
   mappato una volta per frame dal chiamante: dentro lo stesso frame non
   cambia. */
static void AppSimStep(Game *game, AppMode mode, UiLayout layout)
{
    if (mode == APP_GAMEPLAY || mode == APP_FLOOR_ZERO)
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
    bool roomShapesScreenshotTest = false;
    bool shotFormsScreenshotTest = false;
    bool hudStatsScreenshotTest = false;
    bool scriptSandboxTest = false;
    bool scriptDeterminismTest = false;
    bool scriptItemsTest = false;
    bool scriptCharacterTest = false;
    bool statesTest = false;
    bool floorZeroTest = false;
    bool floorZeroScreenshotTest = false;
    bool roomsTest = false;
    /* W9 (playtest round 1, "mouse ovunque"): come --rooms-test, gira DOPO
       InitWindow senza bisogno di una finestra VISIBILE, ma serve il font di
       default di raylib gia' caricato (RendererMouseHitTestSelfTest misura
       BuildScreen con DrawBuildBlock, che chiama MeasureText) -- non puo'
       girare prima di InitWindow come --layout-test. Vedi
       RendererMouseHitTestSelfTest in src/render/game_renderer.c. */
    bool mouseHitTest = false;
    bool rngSeedTest = false;
    bool itemPoolTest = false;
    bool economyTest = false;
    bool fusionTest = false;
    bool fusionScreenshotTest = false;
    /* DEC-065/131/152/159/169: come --economy-test, gira dopo InitWindow
       senza bisogno vero della finestra (GameDiscoveryTest non disegna
       nulla, vedi src/tests/discovery_tests.c). */
    bool discoveryTest = false;
    /* DEC-172: come --economy-test, gira dopo InitWindow (il device audio
       reale e' gia' stato tentato da AudioInit poco sopra, qui sotto CI
       headless: nessun bisogno di una finestra visibile per il test, vedi
       src/tests/audio_tests.c). */
    bool audioTest = false;
    /* W5b (DEC-153): come --audio-test, gira dopo InitWindow senza bisogno
       vero della finestra (GameCuratedContentTest non disegna nulla, vedi
       src/tests/curated_content_tests.c). */
    bool curatedContentTest = false;
    bool artAtlasTest = false;
    bool catalogTest = false;
    /* M8 (DEC-045, vista Catalogo v1): in make test, come --catalog-test --
       gira DOPO InitWindow (esercita davvero UpdateApp/RendererDrawApp). */
    bool catalogScreenTest = false;
    /* SOLO manuale (mai in make test, stessa tradizione di
       --floor-zero-screenshot-test): apre la vista Catalogo su un catalogo
       sintetico popolato e scatta logs/worldsmelt-catalog-screen.png. */
    bool catalogScreenshotTest = false;
    bool layoutTest = false;
    /* M4, SOLO manuale (mai in make test, come *ScreenshotTest sopra): a
       differenza di TUTTI gli altri flag *Test qui sopra, questo NON mette
       smokeTest a true -- vuole davvero la finestra a dimensione del monitor
       (il ramo "!smokeTest" piu' sotto, lo stesso avvio fullscreen del gioco
       vero), non la finestra grande di test APP_WINDOW_WIDTH/HEIGHT. */
    bool fullscreenScreenshotTest = false;
    /* DEC-137, SOLO manuale: come fullscreenScreenshotTest (stessa finestra a
       dimensione del monitor, NON smokeTest), ma scatta l'HUD in overlay in
       APP_GAMEPLAY -- vedi GameOverlayScreenshotTest. */
    bool overlayScreenshotTest = false;
    bool artScreensScreenshotTest = false;
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
        /* DEC-184 (ui/hud.md, "Blocco statistiche"), SOLO manuale (mai in make
           test, come --rarity-screenshot-test/--room-shapes-screenshot-test):
           il blocco compatto sotto salute/risorse con valori non-default,
           una volta visibile e una volta nascosto dal toggle C. Vedi
           GameHudStatsScreenshotTest. */
        if (strcmp(argv[i], "--hud-stats-screenshot-test") == 0)
        {
            smokeTest = true;
            hudStatsScreenshotTest = true;
        }
        if (strcmp(argv[i], "--screenshot-test") == 0)
        {
            smokeTest = true;
            screenshotTest = true;
        }
        /* DEC-170, SOLO manuale: le stanze multi-cella viste davvero (la
           telecamera che segue e sbatte contro il clamp, l'angolo mancante di
           una forma a L). Vedi GameRoomShapesScreenshotTest. */
        if (strcmp(argv[i], "--room-shapes-screenshot-test") == 0)
        {
            smokeTest = true;
            roomShapesScreenshotTest = true;
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
        /* M1b: la sala d'attesa giocabile del Piano 0 (vedi GameFloorZeroTest
           in src/tests/game_tests.c). Come --states-test, gira dopo InitWindow
           ma prima del loop principale. */
        if (strcmp(argv[i], "--floor-zero-test") == 0)
        {
            smokeTest = true;
            floorZeroTest = true;
        }
        /* SOLO manuale (non in make test, tradizione del progetto: vedi
           --rarity-screenshot-test/--shot-forms-screenshot-test sopra):
           finestra GRANDE, entra nel Piano 0 con gen disabilitata e scatta
           logs/worldsmelt-floorzero-screen.png. */
        if (strcmp(argv[i], "--floor-zero-screenshot-test") == 0)
        {
            smokeTest = true;
            floorZeroScreenshotTest = true;
        }
        /* DEC-170/DEC-009: come --states-test, gira dopo InitWindow ma senza
           bisogno vero della finestra (GameRoomsTest non disegna nulla, vedi
           src/tests/game_tests.c). */
        if (strcmp(argv[i], "--rooms-test") == 0)
        {
            smokeTest = true;
            roomsTest = true;
        }
        /* W9: vedi il commento sul flag qui sopra. */
        if (strcmp(argv[i], "--mouse-hit-test") == 0)
        {
            smokeTest = true;
            mouseHitTest = true;
        }
        /* DEC-141: come --rooms-test, gira dopo InitWindow ma senza bisogno
           vero della finestra (GameRngSeedTest non disegna nulla, vedi
           src/tests/game_tests.c). */
        if (strcmp(argv[i], "--rng-seed-test") == 0)
        {
            smokeTest = true;
            rngSeedTest = true;
        }
        /* DEC-144/DEC-145: come --rng-seed-test, gira dopo InitWindow ma
           senza bisogno vero della finestra (GameItemPoolTest non disegna
           nulla, vedi src/tests/game_tests.c). */
        if (strcmp(argv[i], "--item-pool-test") == 0)
        {
            smokeTest = true;
            itemPoolTest = true;
        }
        /* DEC-167: come --item-pool-test, gira dopo InitWindow senza bisogno
           vero della finestra (GameEconomyTest non disegna nulla, vedi
           src/tests/game_tests.c). */
        if (strcmp(argv[i], "--economy-test") == 0)
        {
            smokeTest = true;
            economyTest = true;
        }
        /* DEC-022/023/143/162/171 (la fusione): come --economy-test, gira
           dopo InitWindow senza bisogno vero della finestra (GameFusionTest
           non disegna nulla, vedi src/tests/game_tests.c). */
        if (strcmp(argv[i], "--fusion-test") == 0)
        {
            smokeTest = true;
            fusionTest = true;
        }
        /* SOLO manuale (non in make test, tradizione del progetto): finestra
           grande, BuildScreen con la fascia FUSIONE viva, scatto in
           logs/worldsmelt-fusion-screen.png. */
        if (strcmp(argv[i], "--fusion-screenshot-test") == 0)
        {
            smokeTest = true;
            fusionScreenshotTest = true;
        }
        /* DEC-065/131/152/159/169 (card di scoperta, HUD del Piano 0, causa
           della sconfitta): come --economy-test, gira dopo InitWindow senza
           bisogno vero della finestra. */
        if (strcmp(argv[i], "--discovery-test") == 0)
        {
            smokeTest = true;
            discoveryTest = true;
        }
        /* DEC-172: come --discovery-test sopra. */
        if (strcmp(argv[i], "--audio-test") == 0)
        {
            smokeTest = true;
            audioTest = true;
        }
        /* W5b (DEC-153): come --audio-test, gira dopo InitWindow senza
           bisogno vero della finestra. Vedi GameCuratedContentTest in
           src/tests/curated_content_tests.c. */
        if (strcmp(argv[i], "--curated-content-test") == 0)
        {
            smokeTest = true;
            curatedContentTest = true;
        }
        /* W8: come --curated-content-test, gira dopo InitWindow -- gli scenari
           di caricamento creano texture vere, quindi serve un contesto grafico
           (il parser e l'animatore sarebbero puri, vedi art_atlas_tests.c). */
        if (strcmp(argv[i], "--art-atlas-test") == 0)
        {
            smokeTest = true;
            artAtlasTest = true;
        }
        /* M7 (substrato del catalogo): come --states-test/--rooms-test, gira
           DOPO InitWindow (GameCatalogTest chiama UpdateApp) ma con la SUA
           PROPRIA AppUi locale per ogni scenario (mai la 'appUi' costruita
           piu' sotto per il main loop): l'accensione della guardia test-safe
           avviene dentro il test stesso, non qui. */
        if (strcmp(argv[i], "--catalog-test") == 0)
        {
            smokeTest = true;
            catalogTest = true;
        }
        /* M8 (DEC-045, vista Catalogo v1): come --catalog-test, gira DOPO
           InitWindow con la SUA PROPRIA AppUi locale per ogni scenario. Vedi
           GameCatalogScreenTest in src/tests/catalog_tests.c. */
        if (strcmp(argv[i], "--catalog-screen-test") == 0)
        {
            smokeTest = true;
            catalogScreenTest = true;
        }
        /* SOLO manuale (mai in make test): finestra GRANDE (vedi
           compactTestWindow piu' sotto, stessa tradizione di
           --floor-zero-screenshot-test), percorso DEDICATO -- non passa dal
           ciclo principale/screenshotTest generico, vedi GameCatalogScreenshotTest
           in src/tests/catalog_tests.c. */
        if (strcmp(argv[i], "--catalog-screenshot-test") == 0)
        {
            smokeTest = true;
            catalogScreenshotTest = true;
        }
        if (strcmp(argv[i], "--gen-test") == 0) genTest = true;
        /* M4: matematica pura come --gen-test (nessuna InitWindow/GetScreenWidth
           dentro UiLayoutSelfTest, vedi src/render/game_renderer.c), quindi gira
           nello stesso punto, PRIMA di InitWindow. */
        if (strcmp(argv[i], "--layout-test") == 0) layoutTest = true;
        if (strcmp(argv[i], "--fullscreen-screenshot-test") == 0) fullscreenScreenshotTest = true;
        if (strcmp(argv[i], "--overlay-screenshot-test") == 0) overlayScreenshotTest = true;
        /* W8: come --overlay-screenshot-test (finestra a schermo pieno, un
           warmup di scambi di buffer prima dello scatto a freddo), ma uno scatto
           per SCHERMATA invece di uno solo. */
        if (strcmp(argv[i], "--art-screens-screenshot-test") == 0) artScreensScreenshotTest = true;
        if (strcmp(argv[i], "--script-sandbox-test") == 0) scriptSandboxTest = true;
        if (strcmp(argv[i], "--script-determinism-test") == 0) scriptDeterminismTest = true;
        if (strcmp(argv[i], "--script-items-test") == 0) scriptItemsTest = true;
        if (strcmp(argv[i], "--script-character-test") == 0) scriptCharacterTest = true;
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

    /* M4: matematica pura (src/render/game_renderer.c, UiLayoutSelfTest) come
       GenRunnerSelfTest sopra -- nessuna finestra, gira su risoluzioni
       sintetiche fisse. 19: il primo codice di uscita libero (vedi gli altri
       test sopra e sotto, l'ultimo era --rooms-test=18). */
    if (layoutTest)
    {
        bool ok = UiLayoutSelfTest();
        printf("Layout test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 19;
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
    /* M6b-2 (DEC-037): come sopra, il runtime del trait del personaggio
       (src/script/script_character.c) non tocca raylib direttamente, quindi
       gira prima di InitWindow. Codice di uscita 21: il primo libero fra
       TUTTI i codici gia' usati in questo file (20 e' gia' di
       --fullscreen-screenshot-test, piu' in basso). */
    if (scriptCharacterTest)
    {
        bool ok = ScriptCharacterSelfTest();
        printf("Script character test: %s\n", ok ? "ok" : "failed");
        return ok ? 0 : 21;
    }
    /* --rarity-screenshot-test vuole la finestra GRANDE (non compatta) come
       --screenshot-test: a differenza di --layer-test (dove il personaggio
       equipaggiato e' l'unica cosa da vedere), qui serve anche leggere per
       intero il pannello "GIOCATORE" a destra (bordo + nome della rarita'),
       che nella finestra compatta 960x640 finisce in parte sotto il
       riquadro "GAME VIEW" (overlap gia' presente anche in --layer-test:
       vedi logs/melting-run-layers-screen.png). */
    bool compactTestWindow = smokeTest && !screenshotTest && !rarityScreenshotTest && !shotFormsScreenshotTest && !hudStatsScreenshotTest && !floorZeroScreenshotTest && !catalogScreenshotTest && !roomShapesScreenshotTest;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    /* Titolo della finestra WORLDSMELT (DEC-071): il repo conserva il nome
       storico solo in locale (percorsi, binari, cartelle) -- vedi CLAUDE.md. */
    InitWindow(compactTestWindow ? SCREEN_WIDTH : APP_WINDOW_WIDTH, compactTestWindow ? SCREEN_HEIGHT : APP_WINDOW_HEIGHT, "WORLDSMELT");
    SetExitKey(KEY_NULL);
    /* Modulo audio (DEC-172): DOPO la finestra, come ogni altro sotto-sistema
       raylib qui sopra. Chiamata anche nei rami *Test che seguono (nessuno la
       chiude prima di uscire, stessa convenzione di questi rami per
       GenRunner/AppGen: il processo termina subito dopo comunque) -- e'
       esattamente cio' che esercita il fallback "senza device" sotto Xvfb
       (make test gira headless, senza backend audio reale): vedi
       AudioSelfTest, src/tests/audio_tests.c, --audio-test. */
    AudioInit();
    if (!smokeTest)
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
        /* M4: SetWindowSize/ToggleFullscreen aggiornano SUBITO la contabilita'
           interna di raylib (GetScreenWidth/Height/GetRenderWidth/Height, quindi
           UiComputeLayout, gia' corretti al frame successivo) -- ma il vero
           framebuffer GLX della finestra (il drawable X11 su cui Mesa disegna
           davvero) si ridimensiona SOLO alla prima glXSwapBuffers dopo la
           richiesta, non alla richiesta stessa (osservato sotto Xvfb SENZA window
           manager: nessun ConfigureNotify a confermare, quindi raylib/GLFW
           aggiornano le proprie variabili in modo ottimistico prima che Mesa
           riallochi davvero il backing store). Il gioco vero non se ne accorge
           MAI (il primo vero frame del game loop, subito sotto, scambia i buffer
           comunque prima che il giocatore veda niente) -- questo warmup serve
           SOLO a --fullscreen-screenshot-test, che scatta uno screenshot A FREDDO
           in zero frame reali: senza qualche scambio di buffer esplicito prima,
           TakeScreenshot prenderebbe un frame ancora alla vecchia risoluzione
           fisica pur riportando gia' le dimensioni nuove. Guardia dietro
           'fullscreenScreenshotTest' apposta: il gioco vero non deve pagare
           qualche frame nero in piu' per un problema che non ha. */
        if (fullscreenScreenshotTest || overlayScreenshotTest || artScreensScreenshotTest)
        {
            for (int warmup = 0; warmup < 5; warmup++)
            {
                BeginDrawing();
                ClearBackground(BLACK);
                EndDrawing();
            }
        }
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
    if (statesTest)
    {
        bool ok = GameStatesTest(&game);
        printf("States test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 15;   /* 15: il primo codice di uscita libero (vedi gli altri test sopra) */
    }
    if (floorZeroTest)
    {
        bool ok = GameFloorZeroTest(&game);
        printf("Floor zero test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 16;
    }
    if (floorZeroScreenshotTest)
    {
        bool ok = GameFloorZeroScreenshotTest(&game);
        printf("Floor zero screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 17;
    }
    /* M4, SOLO manuale: la finestra e' gia' a dimensione del monitor (il ramo
       "!smokeTest" qui sopra, mai eseguito per gli altri *Test) -- stesso
       schema di GameFloorZeroScreenshotTest, percorso del file diverso. 20:
       il primo codice di uscita libero (l'ultimo era --layout-test=19). */
    if (fullscreenScreenshotTest)
    {
        bool ok = GameFullscreenScreenshotTest(&game);
        printf("Fullscreen screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 20;
    }
    /* DEC-137, SOLO manuale: come --fullscreen-screenshot-test (finestra a
       dimensione del monitor), ma scatta l'HUD in overlay in Gameplay. 25: il
       primo codice di uscita libero (l'ultimo era --catalog-screen-test=24). */
    if (overlayScreenshotTest)
    {
        bool ok = GameOverlayScreenshotTest(&game);
        printf("Overlay screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 25;
    }
    if (artScreensScreenshotTest)
    {
        bool ok = GameArtScreensScreenshotTest(&game);
        printf("Art screens screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        ArtAtlasShutdown();
        CloseWindow();
        return ok ? 0 : 36;   /* 36: il primo codice libero dopo --art-atlas-test=35 */
    }
    if (mouseHitTest)
    {
        bool ok = RendererMouseHitTestSelfTest(&game) && GameMouseHoverFocusTest(&game);
        printf("Mouse hit-test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 38;   /* 38: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --art-screens-screenshot-test=36) */
    }
    if (roomsTest)
    {
        bool ok = GameRoomsTest(&game);
        printf("Rooms test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 18;   /* 18: il primo codice di uscita libero (vedi gli altri test sopra) */
    }
    if (rngSeedTest)
    {
        bool ok = GameRngSeedTest(&game);
        printf("Rng seed test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 26;   /* 26: il primo codice di uscita libero (vedi gli altri test sopra) */
    }
    if (itemPoolTest)
    {
        bool ok = GameItemPoolTest(&game);
        printf("Item pool test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 28;   /* 28: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --room-shapes-screenshot-test=27) */
    }
    if (economyTest)
    {
        bool ok = GameEconomyTest(&game);
        printf("Economy test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 29;   /* 29: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --item-pool-test=28) */
    }
    if (fusionScreenshotTest)
    {
        bool ok = GameFusionScreenshotTest(&game);
        printf("Fusion screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 31;   /* 31: il primo codice di uscita libero */
    }
    if (fusionTest)
    {
        bool ok = GameFusionTest(&game);
        printf("Fusion test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 30;   /* 30: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --economy-test=29) */
    }
    if (discoveryTest)
    {
        bool ok = GameDiscoveryTest(&game);
        printf("Discovery test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 32;   /* 32: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --fusion-screenshot-test=31) */
    }
    if (audioTest)
    {
        bool ok = GameAudioTest(&game);
        printf("Audio test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        ArtAtlasShutdown();
        AudioShutdown();
        CloseWindow();
        return ok ? 0 : 33;   /* 33: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --discovery-test=32) */
    }
    if (curatedContentTest)
    {
        bool ok = GameCuratedContentTest(&game);
        printf("Curated content test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 34;   /* 34: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --audio-test=33) */
    }
    if (artAtlasTest)
    {
        bool ok = GameArtAtlasTest(&game);
        printf("Art atlas test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        ArtAtlasShutdown();
        CloseWindow();
        return ok ? 0 : 35;   /* 35: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --curated-content-test=34) */
    }
    if (catalogTest)
    {
        bool ok = GameCatalogTest(&game);
        printf("Catalog test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 22;   /* 22: il primo codice di uscita libero (vedi gli altri test sopra) */
    }
    /* M8 (DEC-045, vista Catalogo v1): come --catalog-test, gira DOPO
       InitWindow (esercita UpdateApp/RendererDrawApp per davvero). 23: il
       primo codice di uscita libero. */
    if (catalogScreenTest)
    {
        bool ok = GameCatalogScreenTest(&game);
        printf("Catalog screen test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 23;
    }
    /* SOLO manuale (mai in make test): scrive un catalogo sintetico popolato,
       apre la vista e scatta logs/worldsmelt-catalog-screen.png, poi ripulisce
       i propri file (stessa pulizia snapshot-based degli altri test del
       catalogo). 24: il primo codice di uscita libero. */
    if (catalogScreenshotTest)
    {
        bool ok = GameCatalogScreenshotTest(&game);
        printf("Catalog screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 24;
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
    /* DEC-170, SOLO manuale (mai in make test): scatti delle taglie
       multi-cella per il giudizio di gusto sulla telecamera. 27: il primo
       codice di uscita libero (vedi gli altri test sopra). */
    if (roomShapesScreenshotTest)
    {
        bool ok = GameRoomShapesScreenshotTest(&game);
        printf("Room shapes screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 27;
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
    if (hudStatsScreenshotTest)
    {
        bool ok = GameHudStatsScreenshotTest(&game);
        printf("HUD stats screenshot test: %s\n", ok ? "ok" : "failed");
        GameUnloadAssets(&game);
        CloseWindow();
        return ok ? 0 : 37;   /* 37: il primo codice di uscita libero (vedi gli altri test sopra, l'ultimo era --art-atlas-test=36) */
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
    /* M7 (substrato del catalogo): accesa SOLO per il gioco vero. Ogni *Test
       che raggiunge QUESTO main loop invece di uscire prima (--smoke-test,
       --screenshot-test, --menu-screenshot-test -- gli unici tre: ogni altro
       *Test esce con un return sopra, prima di questo punto) resta
       silenzioso sul catalogo esattamente come i test sintetici di
       game_tests.c, stesso principio "test-safe di default" (spec M7, punto
       3b). Non serve controllare menuScreenshotTest/screenshotTest a parte:
       sono entrambi un caso di smokeTest vero (vedi il parsing di argv
       sopra). */
    appUi.catalogWritesEnabled = !smokeTest;
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
        /* Musica in streaming (DEC-172): UNA volta per frame di finestra, come
           il resto del ciclo qui sopra -- 'frameDt' e' lo stesso passo gia'
           clampato poco sopra, non un nuovo GetFrameTime. No-op sicuro se il
           device audio non e' pronto (vedi audio.c). */
        AudioSyncMusic(&game, appMode, frameDt);
        GenProgress floorZeroStatus = { 0 };
        if (appMode == APP_FLOOR_ZERO)
            snprintf(floorZeroStatus.message, sizeof(floorZeroStatus.message), "%s", AppFloorZeroStatusText(&game, &gen));
        RendererDrawApp(&game, gameCanvas, appMode, &appUi, screenshotTest && !screenshotDone,
                        appMode == APP_FLOOR_ZERO ? &floorZeroStatus : NULL, "logs/melting-run-screen.png");
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
    /* W8: le texture del pacchetto artistico (src/assets/art_atlas.c) non
       seguono il ciclo di vita di Game -- sono asset statici, non contenuto di
       run -- quindi si rilasciano qui, accanto ad AudioShutdown e per la stessa
       ragione: e' il punto in cui il gioco VERO esce. I percorsi di test qui
       sopra escono subito dopo e lasciano la pulizia al teardown del contesto
       OpenGL di CloseWindow, esattamente come fanno gia' col device audio. */
    ArtAtlasShutdown();
    AudioShutdown();
    CloseWindow();
    return 0;
}
