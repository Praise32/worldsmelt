/* WP15a -- LE ARENE DI SFIDA DEL PIANO 0
   (docs/design/systems/floor-zero.md; DEC-004/047/055/092/093/094/095;
   systems/special-rooms.md, Scenario 2; ui/hud.md DEC-169).

   Come GameEconomyTest/GameTrialsTest, gira dopo InitWindow e usa 'game' per
   davvero (GameResetRunWithSeed chiama AssetsLoad) ma non disegna nulla.
   Tredici blocchi indipendenti; ciascuno riparte da un Piano 0 pulito
   (FloorZeroEnter) cosi' un fallimento non trascina stato sporco nel
   successivo.

   Il blocco centrale e' (c): il RIPRISTINO INTEGRALE. Confronta il Player
   PRIMA e DOPO la simulazione **byte per byte** (memcmp sull'intera struct)
   oltre ai campi nominati uno per uno: un campo aggiunto domani al Player e
   dimenticato nel ripristino fa fallire questo test senza che nessuno debba
   ricordarsi di aggiungerlo qui -- il difetto peggiore possibile per questo
   archetipo e' proprio quello silenzioso. */

#include "tests/game_tests.h"

#include "app/app_internal.h"
#include "content/run_catalog.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "game/trials.h"
#include "gameplay/combat.h"
#include "render/game_renderer.h"
#include "world/floor_zero.h"
#include "world/floor_zero_arena.h"
#include "world/world.h"

#include <stdio.h>
#include <string.h>

#define ARENA_TEST_CATALOG_DIR "logs/arena-hub-test-catalog"

static AppInput ArenaInputNone(void)  { AppInput in = { 0 }; return in; }
static AppInput ArenaInputBack(void)  { AppInput in = { 0 }; in.back = true; return in; }
static AppInput ArenaInputPause(void) { AppInput in = { 0 }; in.pause = true; return in; }

/* La piazzola del tema 'theme', o NULL. */
static Pickup *ArenaTestFindGate(Game *game, int theme)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_TRIAL_GATE &&
            game->pickups[i].value == theme) return &game->pickups[i];
    return NULL;
}

/* Porta il giocatore ESATTAMENTE sulla piazzola e preme il tasto di
   interazione come farebbe CombatUpdatePlayer: latcha 'interactQueued' e
   consuma il latch attraverso la stessa funzione che il gameplay chiama, mai
   scrivendo floorZeroTrialRequest a mano. */
static bool ArenaTestPressGate(Game *game, int theme)
{
    Pickup *gate = ArenaTestFindGate(game, theme);
    if (!gate) return false;
    game->player.pos = gate->pos;
    return FloorZeroArenaQueueEntry(game);
}

/* Un catalogo sintetico con due run costruite APPOSTA per isolare il criterio
   "best-of": la run SCONFITTA e' quella arrivata piu' lontano (piano 5, boss
   abbattuto) e la run VITTORIOSA e' quella corta (piano 1, nessun boss). Ogni
   altro termine della formula di qualita' tira quindi dalla parte SBAGLIATA:
   se il best-of smettesse di pesare l'esito, pescherebbe dalla sconfitta e il
   test se ne accorgerebbe. Stesso pattern (RunCatalogSetTestPath) dei test di
   catalogo esistenti. */
static bool ArenaTestWriteSyntheticCatalog(void)
{
    if (!DirectoryExists(ARENA_TEST_CATALOG_DIR) && MakeDirectory(ARENA_TEST_CATALOG_DIR) != 0)
    {
        fprintf(stderr, "ArenaHubTest: impossibile creare %s\n", ARENA_TEST_CATALOG_DIR);
        return false;
    }

    /* La run PEGGIORE per esito ma MIGLIORE per ogni altro termine: sconfitta,
       ma arrivata al piano 5 con un boss abbattuto. */
    FILE *loss = fopen(ARENA_TEST_CATALOG_DIR "/run-1-sconfitta-p5-1.txt", "w");
    if (!loss) return false;
    fprintf(loss, "catalogSchema=1\nseed=1\nsource=local:test\noutcome=sconfitta\nfloorReached=5\nfloor.count=1\n");
    fprintf(loss, "floor1.enemy1.name=Scarto Perdente\nfloor1.enemy1.form=blob\nfloor1.enemy1.move=chase\n");
    fprintf(loss, "floor1.enemy1.fire=none\nfloor1.enemy1.hp=1.000\nfloor1.enemy1.speed=1.000\n");
    fprintf(loss, "floor1.enemy1.size=1.000\nfloor1.enemy1.rate=0.000\nfloor1.enemy1.pellets=0\n");
    fprintf(loss, "floor1.boss.name=Sconfitta Illustre\nfloor1.boss.form=blob\nfloor1.boss.move=chase\n");
    fprintf(loss, "floor1.boss.fire=none\nfloor1.boss.hp=1.000\nfloor1.boss.speed=1.000\n");
    fprintf(loss, "floor1.boss.size=1.000\nfloor1.boss.rate=0.000\nfloor1.boss.pellets=0\n");
    fprintf(loss, "floor1.boss.outcome=sconfitto\n");
    fclose(loss);

    /* La run MIGLIORE: vittoria, ma corta -- piano 1, nessun boss registrato
       oltre a quello del piano stesso. */
    FILE *win = fopen(ARENA_TEST_CATALOG_DIR "/run-2-vittoria-p1-1.txt", "w");
    if (!win) return false;
    fprintf(win, "catalogSchema=1\nseed=2\nsource=local:test\noutcome=vittoria\nfloorReached=1\nfloor.count=1\n");
    fprintf(win, "floor1.enemy1.name=Campione di Bronzo\nfloor1.enemy1.form=spiky\nfloor1.enemy1.move=kite\n");
    fprintf(win, "floor1.enemy1.fire=single\nfloor1.enemy1.hp=1.200\nfloor1.enemy1.speed=1.100\n");
    fprintf(win, "floor1.enemy1.size=1.000\nfloor1.enemy1.rate=1.000\nfloor1.enemy1.pellets=1\n");
    fprintf(win, "floor1.boss.name=Colosso Fuso\nfloor1.boss.form=armored\nfloor1.boss.move=charge\n");
    fprintf(win, "floor1.boss.fire=ring\nfloor1.boss.hp=1.500\nfloor1.boss.speed=0.800\n");
    fprintf(win, "floor1.boss.size=1.400\nfloor1.boss.rate=0.800\nfloor1.boss.pellets=6\n");
    fprintf(win, "floor1.boss.outcome=incontrato\n");
    fclose(win);
    return true;
}

static void ArenaTestRemoveSyntheticCatalog(void)
{
    remove(ARENA_TEST_CATALOG_DIR "/run-1-sconfitta-p5-1.txt");
    remove(ARENA_TEST_CATALOG_DIR "/run-2-vittoria-p1-1.txt");
}

/* Vero se almeno un nemico attivo porta questo nome. */
static bool ArenaTestHasEnemyNamed(const Game *game, const char *name)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (game->enemies[i].active && strcmp(game->enemies[i].type.name, name) == 0) return true;
    return false;
}

static int ArenaTestActiveEnemies(const Game *game)
{
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active) n++;
    return n;
}

bool GameArenaHubTest(Game *game)
{
    /* Il catalogo REALE del giocatore resta fuori da questo test per intero
       (stesso pattern RunCatalogSetTestPath dei test di catalogo esistenti):
       il best-of leggerebbe altrimenti run vere e il blocco (h), che chiede un
       catalogo VUOTO, dipenderebbe da quante partite ha fatto chi lancia la
       suite. La cartella si crea vuota qui e i due blocchi che vogliono dei
       record ce li scrivono e poi li tolgono. */
    if (!DirectoryExists(ARENA_TEST_CATALOG_DIR) && MakeDirectory(ARENA_TEST_CATALOG_DIR) != 0)
    {
        fprintf(stderr, "ArenaHubTest: impossibile creare %s\n", ARENA_TEST_CATALOG_DIR);
        return false;
    }
    ArenaTestRemoveSyntheticCatalog();   /* residui di un giro precedente interrotto */
    RunCatalogSetTestPath(ARENA_TEST_CATALOG_DIR);

    /* --- (a) le piazzole esistono, una per tema, dentro la stanza e mai
       sotto i piedi del giocatore che nasce al centro. --- */
    FloorZeroEnter(game);
    int gates = 0;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_TRIAL_GATE) gates++;
    if (gates != FLOOR_ZERO_TRIAL_COUNT)
    {
        fprintf(stderr, "ArenaHubTest: (a) piazzole trovate=%d, attese %d\n", gates, FLOOR_ZERO_TRIAL_COUNT);
        return false;
    }
    for (int t = 0; t < FLOOR_ZERO_TRIAL_COUNT; t++)
    {
        Pickup *gate = ArenaTestFindGate(game, t);
        if (!gate) { fprintf(stderr, "ArenaHubTest: (a) manca la piazzola del tema %d\n", t); return false; }
        if (gate->pos.x < ROOM_X || gate->pos.x > ROOM_X + ROOM_W ||
            gate->pos.y < ROOM_Y || gate->pos.y > ROOM_Y + ROOM_H)
        {
            fprintf(stderr, "ArenaHubTest: (a) piazzola %d fuori dalla stanza\n", t);
            return false;
        }
        float dx = gate->pos.x - (ROOM_X + ROOM_W*0.5f);
        float dy = gate->pos.y - (ROOM_Y + ROOM_H*0.5f);
        if (dx*dx + dy*dy < 60.0f*60.0f)
        {
            fprintf(stderr, "ArenaHubTest: (a) piazzola %d addosso al punto di nascita\n", t);
            return false;
        }
    }
    if (game->floorZeroTrialActive)
    {
        fprintf(stderr, "ArenaHubTest: (a) simulazione gia' attiva appena entrati nel Piano 0\n");
        return false;
    }
    /* DEC-169, meta' "nascosto": fuori da una prova l'HUD resta invisibile. */
    if (HudCombatShouldDraw(APP_FLOOR_ZERO, game->floorZeroTrialActive))
    {
        fprintf(stderr, "ArenaHubTest: (a) HUD di combattimento visibile nell'hub fuori da una prova\n");
        return false;
    }

    /* --- (b) il TOCCO non entra, il tasto LONTANO non entra, il tasto A
       CONTATTO latcha (conferma esplicita, come arena e Pourhouse). --- */
    Pickup *gate0 = ArenaTestFindGate(game, FLOOR_ZERO_TRIAL_MOVE);
    game->player.pos = gate0->pos;
    CombatUpdatePickups(game);   /* il solo tocco, dallo stesso punto del gioco vero */
    if (game->floorZeroTrialRequest != 0 || game->floorZeroTrialActive || !gate0->active)
    {
        fprintf(stderr, "ArenaHubTest: (b) il solo tocco della piazzola ha fatto qualcosa\n");
        return false;
    }
    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    if (FloorZeroArenaQueueEntry(game))
    {
        fprintf(stderr, "ArenaHubTest: (b) il tasto premuto lontano dalla piazzola e' entrato lo stesso\n");
        return false;
    }
    if (!ArenaTestPressGate(game, FLOOR_ZERO_TRIAL_MOVE) ||
        game->floorZeroTrialRequest != FLOOR_ZERO_TRIAL_MOVE + 1)
    {
        fprintf(stderr, "ArenaHubTest: (b) il tasto a contatto non ha latchato la richiesta\n");
        return false;
    }
    game->floorZeroTrialRequest = 0;

    /* --- (c) IL RIPRISTINO INTEGRALE (DEC-092), compreso un danno subito
       dentro la simulazione. --- */
    FloorZeroEnter(game);
    /* Uno stato d'ingresso RICCO, non quello vergine: risorse, salute
       temporanea e punteggio devono tornare tutti esattamente com'erano. */
    game->player.coins = 17;
    game->player.bombs = 3;
    game->player.keys = 2;
    game->player.flux = 1;
    game->player.tempHp = 2;
    game->score = 4242;
    unsigned int rngBefore = game->rng = 0xC0FFEEu;
    /* Ci si porta sulla piazzola PRIMA di fotografare lo stato: e' esattamente
       il momento in cui la simulazione lo cattura, posizione compresa -- e la
       posizione DEVE tornare quella, non il centro dell'arena. */
    if (!ArenaTestPressGate(game, FLOOR_ZERO_TRIAL_MOVE)) { fprintf(stderr, "ArenaHubTest: (c) ingresso non latchato\n"); return false; }
    game->floorZeroTrialRequest = 0;
    Player before = game->player;
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    if (!game->floorZeroTrialActive) { fprintf(stderr, "ArenaHubTest: (c) la simulazione non e' partita\n"); return false; }
    if (!HudCombatShouldDraw(APP_FLOOR_ZERO, game->floorZeroTrialActive))
    {
        fprintf(stderr, "ArenaHubTest: (c) HUD di combattimento nascosto DENTRO una prova (DEC-169)\n");
        return false;
    }
    if (game->inRealRun || game->runElapsedSeconds != 0.0f)
    {
        fprintf(stderr, "ArenaHubTest: (c) il timer di run corre dentro una simulazione del Piano 0\n");
        return false;
    }
    if (ArenaTestActiveEnemies(game) <= 0) { fprintf(stderr, "ArenaHubTest: (c) simulazione senza nemici\n"); return false; }

    /* Danno vero, dentro. */
    game->player.invuln = 0.0f;
    CombatDamagePlayer(game, 1, "una prova");
    game->player.invuln = 0.0f;
    CombatDamagePlayer(game, 1, "una prova");
    if (game->player.tempHp == before.tempHp && game->player.hp == before.hp)
    {
        fprintf(stderr, "ArenaHubTest: (c) il danno dentro la simulazione non ha avuto effetto: la prova non prova nulla\n");
        return false;
    }
    /* E qualcosa di raccolto, dentro. */
    game->player.coins += 99;
    game->player.bombs += 5;
    game->score += 300;
    game->rng = 0xDEADBEEFu;

    FloorZeroArenaExit(game, false);
    if (game->floorZeroTrialActive) { fprintf(stderr, "ArenaHubTest: (c) la simulazione non si e' chiusa\n"); return false; }

    if (memcmp(&before, &game->player, sizeof(Player)) != 0)
    {
        /* Diagnostica campo per campo: dice SUBITO quale valore e' rimasto
           indietro, invece di un "sono diversi" che non aiuta nessuno. */
        fprintf(stderr, "ArenaHubTest: (c) il Player NON e' tornato quello d'ingresso\n");
        fprintf(stderr, "  hp %d->%d  tempHp %d->%d  maxHp %d->%d  hpCap %d->%d\n",
                before.hp, game->player.hp, before.tempHp, game->player.tempHp,
                before.maxHp, game->player.maxHp, before.hpCap, game->player.hpCap);
        fprintf(stderr, "  coins %d->%d  bombs %d->%d  keys %d->%d  flux %d->%d  items %d->%d\n",
                before.coins, game->player.coins, before.bombs, game->player.bombs,
                before.keys, game->player.keys, before.flux, game->player.flux,
                before.itemCount, game->player.itemCount);
        fprintf(stderr, "  pos %.2f,%.2f -> %.2f,%.2f  invuln %.3f->%.3f  damage %.3f->%.3f\n",
                (double)before.pos.x, (double)before.pos.y,
                (double)game->player.pos.x, (double)game->player.pos.y,
                (double)before.invuln, (double)game->player.invuln,
                (double)before.damage, (double)game->player.damage);
        return false;
    }
    if (game->score != 4242) { fprintf(stderr, "ArenaHubTest: (c) punteggio=%d, atteso 4242\n", game->score); return false; }
    if (game->rng != rngBefore) { fprintf(stderr, "ArenaHubTest: (c) lo stream RNG della run e' stato spostato dalla simulazione\n"); return false; }
    if (ArenaTestActiveEnemies(game) != 0) { fprintf(stderr, "ArenaHubTest: (c) nemici sopravvissuti all'uscita\n"); return false; }
    if (!ArenaTestFindGate(game, FLOOR_ZERO_TRIAL_MOVE)) { fprintf(stderr, "ArenaHubTest: (c) le piazzole non sono tornate\n"); return false; }
    if (game->obstacleCount <= 0) { fprintf(stderr, "ArenaHubTest: (c) l'arredo del crogiolo non e' tornato\n"); return false; }
    if (HudCombatShouldDraw(APP_FLOOR_ZERO, game->floorZeroTrialActive))
    {
        fprintf(stderr, "ArenaHubTest: (c) HUD ancora visibile dopo l'uscita dalla prova (DEC-169)\n");
        return false;
    }

    /* --- (d) morte dentro la simulazione: MAI PHASE_GAME_OVER. --- */
    FloorZeroEnter(game);
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    Player beforeDeath = game->player;
    for (int i = 0; i < 40 && game->player.hp > 0; i++)
    {
        game->player.invuln = 0.0f;
        CombatDamagePlayer(game, 1, "la simulazione");
    }
    if (game->phase == PHASE_GAME_OVER)
    {
        fprintf(stderr, "ArenaHubTest: (d) PHASE_GAME_OVER dentro una simulazione a rischio zero\n");
        return false;
    }
    if (!game->floorZeroTrialDefeated)
    {
        fprintf(stderr, "ArenaHubTest: (d) la sconfitta nella simulazione non e' stata latchata\n");
        return false;
    }
    if (game->deathCause[0])
    {
        fprintf(stderr, "ArenaHubTest: (d) causa di morte scritta per una sconfitta in simulazione\n");
        return false;
    }
    FloorZeroArenaExit(game, true);
    if (game->phase != PHASE_PLAY) { fprintf(stderr, "ArenaHubTest: (d) fase != PHASE_PLAY dopo la sconfitta in simulazione\n"); return false; }
    if (memcmp(&beforeDeath, &game->player, sizeof(Player)) != 0)
    {
        fprintf(stderr, "ArenaHubTest: (d) il Player non e' tornato integro dopo la sconfitta (hp %d->%d)\n",
                beforeDeath.hp, game->player.hp);
        return false;
    }
    if (game->floorZeroTrialDefeated) { fprintf(stderr, "ArenaHubTest: (d) il latch di sconfitta non e' stato consumato\n"); return false; }

    /* --- (e) nessuna valuta/pickup sopravvive: si RACCOGLIE davvero, dentro,
       passando da CombatPickup come nel gioco vero. --- */
    FloorZeroEnter(game);
    Player beforeLoot = game->player;
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_RESOURCES, false);
    int collected = 0;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *p = &game->pickups[i];
        if (!p->active) continue;
        if (p->kind != PICKUP_COIN && p->kind != PICKUP_BOMB && p->kind != PICKUP_KEY) continue;
        /* Raccolta VERA: ci si cammina sopra e si lascia fare a
           CombatUpdatePickups, lo stesso percorso del gioco. */
        game->player.pos = p->pos;
        CombatUpdatePickups(game);
        collected++;
    }
    if (collected == 0) { fprintf(stderr, "ArenaHubTest: (e) la prova RISORSE non offre nulla da raccogliere\n"); return false; }
    if (game->player.coins == beforeLoot.coins && game->player.bombs == beforeLoot.bombs && game->player.keys == beforeLoot.keys)
    {
        fprintf(stderr, "ArenaHubTest: (e) la raccolta dentro la simulazione non ha avuto effetto: la prova non prova nulla\n");
        return false;
    }
    FloorZeroArenaExit(game, false);
    if (memcmp(&beforeLoot, &game->player, sizeof(Player)) != 0)
    {
        fprintf(stderr, "ArenaHubTest: (e) risorse raccolte nella simulazione sopravvissute all'uscita (coins %d->%d, bombs %d->%d, keys %d->%d)\n",
                beforeLoot.coins, game->player.coins, beforeLoot.bombs, game->player.bombs,
                beforeLoot.keys, game->player.keys);
        return false;
    }

    /* --- (f) le PROVE della run (DEC-042, WP16) non avanzano dentro una
       simulazione -- guardia esplicita, non un effetto collaterale. --- */
    FloorZeroEnter(game);
    memset(game->trials, 0, sizeof(game->trials));
    game->trialCount = 1;
    game->trials[0].kind = TRIAL_FUSE_ITEM;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->trials[0].bonus = 10;
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_FUSION, false);
    TrialsOnFusionPerformed(game);
    TrialsOnSecretFound(game);
    TrialsOnRoomCleared(game, ROOM_ARENA);
    if (game->trials[0].state != TRIAL_IN_PROGRESS)
    {
        fprintf(stderr, "ArenaHubTest: (f) una prova della run e' avanzata DENTRO una simulazione del Piano 0\n");
        return false;
    }
    FloorZeroArenaExit(game, false);
    TrialsOnFusionPerformed(game);   /* fuori dalla simulazione la stessa chiamata deve invece contare */
    if (game->trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "ArenaHubTest: (f) la guardia ha spento le prove anche FUORI dalla simulazione\n");
        return false;
    }
    memset(game->trials, 0, sizeof(game->trials));
    game->trialCount = 0;

    /* --- (g) BEST-OF da un catalogo sintetico: la run migliore vince. --- */
    if (!ArenaTestWriteSyntheticCatalog()) { fprintf(stderr, "ArenaHubTest: (g) catalogo sintetico non scritto\n"); return false; }

    EnemyTypeDef bestOf[8];
    int bestCount = RunCatalogBestOfEnemies(bestOf, 8);
    if (bestCount != 2)
    {
        fprintf(stderr, "ArenaHubTest: (g) tipi best-of=%d, attesi 2 dalla run di vittoria\n", bestCount);
        ArenaTestRemoveSyntheticCatalog();
        return false;
    }
    bool sawLoser = false;
    bool sawWinner = false;
    for (int i = 0; i < bestCount; i++)
    {
        if (strcmp(bestOf[i].name, "Scarto Perdente") == 0 || strcmp(bestOf[i].name, "Sconfitta Illustre") == 0) sawLoser = true;
        if (strcmp(bestOf[i].name, "Campione di Bronzo") == 0) sawWinner = true;
    }
    if (!sawWinner)
    {
        fprintf(stderr, "ArenaHubTest: (g) il best-of non ha pescato dalla run di vittoria\n");
        ArenaTestRemoveSyntheticCatalog();
        return false;
    }
    if (sawLoser)
    {
        fprintf(stderr, "ArenaHubTest: (g) il best-of ha pescato dalla run peggiore\n");
        ArenaTestRemoveSyntheticCatalog();
        return false;
    }

    FloorZeroEnter(game);
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    bool sawBestOf = ArenaTestHasEnemyNamed(game, "Campione di Bronzo") || ArenaTestHasEnemyNamed(game, "Colosso Fuso");
    /* Un boss del catalogo entra come nemico normale: la simulazione non e'
       la prova dal museo (DEC-040), che non esiste ancora nel motore. */
    bool anyBoss = false;
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active && game->enemies[i].type.boss) anyBoss = true;
    FloorZeroArenaExit(game, false);
    ArenaTestRemoveSyntheticCatalog();
    if (!sawBestOf)
    {
        fprintf(stderr, "ArenaHubTest: (g) l'arena non usa i contenuti best-of del catalogo\n");
        return false;
    }
    if (anyBoss)
    {
        fprintf(stderr, "ArenaHubTest: (g) un boss del catalogo e' entrato come boss nella simulazione\n");
        return false;
    }

    /* --- (h) FALLBACK obbligatorio: catalogo VUOTO, l'arena funziona lo
       stesso (caso limite dichiarato di floor-zero.md, DEC-087/094/153). --- */
    EnemyTypeDef empty[4];
    if (RunCatalogBestOfEnemies(empty, 4) != 0)
    {
        fprintf(stderr, "ArenaHubTest: (h) il catalogo di prova non e' vuoto\n");
        return false;
    }
    FloorZeroEnter(game);
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    int fallbackEnemies = ArenaTestActiveEnemies(game);
    FloorZeroArenaExit(game, false);
    if (fallbackEnemies <= 0)
    {
        fprintf(stderr, "ArenaHubTest: (h) senza catalogo l'arena resta vuota: il fallback curato non ha funzionato\n");
        return false;
    }

    /* --- (i-bis) VITTORIA: si annuncia, NON chiude la prova (DEC-095: le
       prove sono illimitate, e chiudere d'ufficio taglierebbe corta la lezione
       della piazzola FUSIONE). --- */
    FloorZeroEnter(game);
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_FUSION, false);
    if (game->floorZeroTrialWon) { fprintf(stderr, "ArenaHubTest: (i-bis) prova gia' vinta all'ingresso\n"); return false; }
    if (FloorZeroArenaCleared(game)) { fprintf(stderr, "ArenaHubTest: (i-bis) prova dichiarata vinta con i nemici ancora vivi\n"); return false; }
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active) game->enemies[i].active = false;
    /* Attraverso UpdateApp, non chiamando l'annuncio a mano: e' src/app/app.c a
       decidere cosa fare di una prova vinta, ed e' li' che la tentazione di
       chiuderla d'ufficio vivrebbe. */
    {
        AppUi vui;
        AppGen vgen;
        AppMode vmode = APP_FLOOR_ZERO;
        memset(&vui, 0, sizeof(vui));
        memset(&vgen, 0, sizeof(vgen));
        for (int i = 0; i < 3; i++) { AppInput in = ArenaInputNone(); UpdateApp(game, &vmode, &vgen, &vui, &in); }
        if (vmode != APP_FLOOR_ZERO) { fprintf(stderr, "ArenaHubTest: (i-bis) la vittoria ha cambiato stato dell'app\n"); return false; }
    }
    if (!game->floorZeroTrialWon) { fprintf(stderr, "ArenaHubTest: (i-bis) la vittoria non e' stata registrata\n"); return false; }
    if (!game->floorZeroTrialActive)
    {
        fprintf(stderr, "ArenaHubTest: (i-bis) la vittoria ha chiuso la prova da sola: DEC-095 chiede l'opposto\n");
        return false;
    }
    /* Gli attrezzi della lezione restano a terra: e' il punto della piazzola. */
    {
        int leftovers = 0;
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (game->pickups[i].active && (game->pickups[i].kind == PICKUP_ITEM || game->pickups[i].kind == PICKUP_FLUX)) leftovers++;
        if (leftovers == 0)
        {
            fprintf(stderr, "ArenaHubTest: (i-bis) la piazzola FUSIONE non lascia nulla con cui fondere\n");
            return false;
        }
    }
    FloorZeroArenaExit(game, false);
    if (game->floorZeroTrialWon) { fprintf(stderr, "ArenaHubTest: (i-bis) il flag di vittoria non e' stato ripulito all'uscita\n"); return false; }

    /* --- (i) DETERMINISMO: due ingressi nella stessa piazzola compongono la
       stessa ondata, e la simulazione non sposta mai gli stream della run. --- */
    FloorZeroEnter(game);
    game->rng = 0x12345678u;
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    Vector2 firstPositions[MAX_ENEMIES];
    int firstCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active) firstPositions[firstCount++] = game->enemies[i].pos;
    FloorZeroArenaExit(game, false);
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    int secondCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active) continue;
        if (secondCount >= firstCount ||
            game->enemies[i].pos.x != firstPositions[secondCount].x ||
            game->enemies[i].pos.y != firstPositions[secondCount].y)
        {
            fprintf(stderr, "ArenaHubTest: (i) la composizione della simulazione non e' deterministica\n");
            return false;
        }
        secondCount++;
    }
    if (secondCount != firstCount)
    {
        fprintf(stderr, "ArenaHubTest: (i) nemici al secondo ingresso=%d, al primo=%d\n", secondCount, firstCount);
        return false;
    }
    /* Prove ILLIMITATE (DEC-095): il secondo ingresso e' avvenuto davvero. */
    FloorZeroArenaExit(game, false);
    if (game->rng != 0x12345678u)
    {
        fprintf(stderr, "ArenaHubTest: (i) due simulazioni hanno spostato lo stream RNG della run\n");
        return false;
    }

    /* --- (j) il varco verso il piano 1 non si attraversa dentro una
       simulazione: l'attraversamento azzererebbe lo snapshot da restituire. --- */
    FloorZeroEnter(game);
    game->floorZeroExitOpen = true;
    FloorZeroArenaEnter(game, FLOOR_ZERO_TRIAL_MOVE, false);
    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + 2.0f };
    WorldHandleTransitions(game, (Vector2){ 0.0f, -1.0f });
    if (game->floorZeroExitCrossed)
    {
        fprintf(stderr, "ArenaHubTest: (j) il varco si e' attraversato durante una simulazione\n");
        return false;
    }
    FloorZeroArenaExit(game, false);
    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + 2.0f };
    WorldHandleTransitions(game, (Vector2){ 0.0f, -1.0f });
    if (!game->floorZeroExitCrossed)
    {
        fprintf(stderr, "ArenaHubTest: (j) fuori dalla simulazione il varco non si attraversa piu'\n");
        return false;
    }
    game->floorZeroExitCrossed = false;

    /* --- (k) TUTORIAL alla PRIMA visita e mai piu' (DEC-047), piu' la
       consultazione dal Piano 0 con il comando di pausa (domanda aperta 22).
       Qui si passa da UpdateApp per davvero: il "gia' visto" vive su AppUi. --- */
    {
        AppUi ui;
        AppGen gen;
        AppMode mode = APP_FLOOR_ZERO;
        memset(&ui, 0, sizeof(ui));
        memset(&gen, 0, sizeof(gen));
        /* ui.catalogWritesEnabled resta falso (memset sopra): nessuna
           scrittura nel catalogo reale da questo test. */

        FloorZeroEnter(game);
        if (!ArenaTestPressGate(game, FLOOR_ZERO_TRIAL_RESOURCES)) { fprintf(stderr, "ArenaHubTest: (k) ingresso non latchato\n"); return false; }
        { AppInput in = ArenaInputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!game->floorZeroTrialActive) { fprintf(stderr, "ArenaHubTest: (k) UpdateApp non ha aperto la simulazione\n"); return false; }
        if (!game->floorZeroTrialHint[0]) { fprintf(stderr, "ArenaHubTest: (k) nessun cartello alla PRIMA visita\n"); return false; }
        if (!ui.floorZeroTrialTutorialSeen[FLOOR_ZERO_TRIAL_RESOURCES]) { fprintf(stderr, "ArenaHubTest: (k) la prima visita non e' stata registrata\n"); return false; }

        /* ESC chiude la simulazione, non apre ExitConfirm. */
        { AppInput in = ArenaInputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_FLOOR_ZERO || game->floorZeroTrialActive)
        {
            fprintf(stderr, "ArenaHubTest: (k) ESC dentro la simulazione non riporta nell'hub (mode=%d, attiva=%d)\n",
                    (int)mode, (int)game->floorZeroTrialActive);
            return false;
        }

        /* Seconda visita alla STESSA piazzola: nessun cartello. */
        if (!ArenaTestPressGate(game, FLOOR_ZERO_TRIAL_RESOURCES)) { fprintf(stderr, "ArenaHubTest: (k) secondo ingresso non latchato\n"); return false; }
        { AppInput in = ArenaInputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!game->floorZeroTrialActive) { fprintf(stderr, "ArenaHubTest: (k) la seconda visita non e' partita (DEC-095: prove illimitate)\n"); return false; }
        if (game->floorZeroTrialHint[0])
        {
            fprintf(stderr, "ArenaHubTest: (k) il cartello e' tornato alla seconda visita: '%s'\n", game->floorZeroTrialHint);
            return false;
        }
        /* Una piazzola MAI vista mostra invece il suo cartello: il "gia' visto"
           e' per piazzola, non globale. */
        { AppInput in = ArenaInputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!ArenaTestPressGate(game, FLOOR_ZERO_TRIAL_FUSION)) { fprintf(stderr, "ArenaHubTest: (k) ingresso FUSIONE non latchato\n"); return false; }
        { AppInput in = ArenaInputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (!game->floorZeroTrialHint[0])
        {
            fprintf(stderr, "ArenaHubTest: (k) nessun cartello alla prima visita di un'ALTRA piazzola\n");
            return false;
        }
        { AppInput in = ArenaInputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }

        /* --- (l) domanda aperta 22: il comando di pausa apre PauseMenu dal
           Piano 0 e "Riprendi" riporta nel Piano 0, non in Gameplay. --- */
        { AppInput in = ArenaInputPause(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_PAUSE_MENU || !ui.pauseFromFloorZero)
        {
            fprintf(stderr, "ArenaHubTest: (l) il comando di pausa non apre PauseMenu dal Piano 0 (mode=%d)\n", (int)mode);
            return false;
        }
        if (game->floor != 0)
        {
            fprintf(stderr, "ArenaHubTest: (l) il riquadro di consultazione dell'HUD dipende da floor==0, e floor=%d\n", game->floor);
            return false;
        }
        { AppInput in = ArenaInputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_FLOOR_ZERO)
        {
            fprintf(stderr, "ArenaHubTest: (l) uscendo dalla pausa non si torna nel Piano 0 (mode=%d)\n", (int)mode);
            return false;
        }
        /* ESC nell'hub, fuori da ogni prova, resta ExitConfirm (DEC-074). */
        { AppInput in = ArenaInputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
        if (mode != APP_EXIT_CONFIRM)
        {
            fprintf(stderr, "ArenaHubTest: (l) ESC nell'hub non apre piu' ExitConfirm (mode=%d)\n", (int)mode);
            return false;
        }
    }

    RunCatalogSetTestPath(NULL);   /* il catalogo reale torna quello di default */
    return true;
}
