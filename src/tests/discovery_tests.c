/* Test di DEC-065/131/152/159/169 (docs/design/ui/hud.md,
   docs/design/ui/pause-menu.md, docs/design/ui/results-and-leaderboards.md,
   docs/design/systems/floor-zero.md): il push REALE di una card dal call site
   vero (WorldSpawnCombatRoom, non solo GameQueueDiscoveryCard chiamata a
   mano) e il cap/drop-oldest della coda (DEC-131); coda delle card di
   scoperta scartata a morte/cambio stanza SENZA toccare il Catalogo
   (DEC-152); flag di visibilita' dell'HUD di combattimento per stato/prova
   del Piano 0 (DEC-169); causa della sconfitta popolata da un game over
   sintetico (DEC-159). Come GameEconomyTest (src/tests/game_tests.c), gira
   dopo InitWindow e usa 'game' per davvero (GameResetRunWithSeed chiama
   AssetsLoad) ma non disegna nulla. */

#include "tests/game_tests.h"

#include "game/game_internal.h"
#include "render/game_renderer.h"

#include <stdio.h>
#include <string.h>

/* (1) DEC-169: HudCombatShouldDraw e' un nucleo puro (nessuna chiamata
   raylib) -- nessun bisogno di 'game' o di InitWindow, ma resta dentro la
   suite qui sotto per un unico punto di ingresso in --discovery-test. */
static bool DiscoveryTestHudFlag(void)
{
    bool ok = true;

    if (!HudCombatShouldDraw(APP_GAMEPLAY, false))
    {
        fprintf(stderr, "DiscoveryTest: HudCombatShouldDraw(APP_GAMEPLAY, false) doveva essere vero\n");
        ok = false;
    }
    if (!HudCombatShouldDraw(APP_GAMEPLAY, true))
    {
        fprintf(stderr, "DiscoveryTest: HudCombatShouldDraw(APP_GAMEPLAY, true) doveva essere vero\n");
        ok = false;
    }
    /* DEC-169: fuori da una prova, il Piano 0 nasconde l'HUD di combattimento. */
    if (HudCombatShouldDraw(APP_FLOOR_ZERO, false))
    {
        fprintf(stderr, "DiscoveryTest: HudCombatShouldDraw(APP_FLOOR_ZERO, false) doveva essere falso\n");
        ok = false;
    }
    /* DEC-169: l'hook pronto per le prove -- quando (in futuro) un'arena lo
       attiva, l'HUD deve ricomparire senza altro codice. */
    if (!HudCombatShouldDraw(APP_FLOOR_ZERO, true))
    {
        fprintf(stderr, "DiscoveryTest: HudCombatShouldDraw(APP_FLOOR_ZERO, true) doveva essere vero\n");
        ok = false;
    }
    /* Ogni altro stato resta nascosto, a prescindere dal flag di prova
       (che ha senso solo nel Piano 0). */
    if (HudCombatShouldDraw(APP_PAUSE_MENU, true) || HudCombatShouldDraw(APP_MAIN_MENU, true) ||
        HudCombatShouldDraw(APP_RUN_RESULTS, true) || HudCombatShouldDraw(APP_BUILD_SCREEN, true))
    {
        fprintf(stderr, "DiscoveryTest: HudCombatShouldDraw doveva restare falso fuori da Gameplay/prova del Piano 0\n");
        ok = false;
    }

    printf("  HUD per stato (DEC-169): Gameplay=si, Piano0 fuori prova=no, Piano0 in prova (hook)=si, altri stati=no: %s\n", ok ? "ok" : "FALLITO");
    return ok;
}

/* (2) DEC-065/152, il push REALE: prima di questo test la suite esercitava
   solo GameQueueDiscoveryCard chiamata a mano -- il call site vero
   (WorldSpawnCombatRoom, src/world/world.c) non era mai raggiunto, quindi una
   regressione che invertisse l'ordine flag/push (o smettesse di pushare del
   tutto) sarebbe passata. Qui si arma un tipo di nemico attivo sul piano
   corrente e si chiama WorldSpawnCombatRoom per davvero: una sola card deve
   accodarsi anche se la stanza spawna piu' nemici dello stesso tipo (il
   flag si scrive alla prima spawn, le successive non ripushano -- vedi il
   commento su WorldSpawnCombatRoom), e una seconda chiamata a stanza gia'
   incontrata (flag rimasto vero) non deve accodare nulla. */
static bool DiscoveryTestRealPushFromCombatRoom(Game *game)
{
    bool ok = true;
    GameResetRunWithSeed(game, 20260728u);

    FloorContent *fc = &game->content.floors[game->floor - 1];
    memset(&fc->enemies[0], 0, sizeof(fc->enemies[0]));
    fc->enemies[0].active = true;
    snprintf(fc->enemies[0].name, sizeof(fc->enemies[0].name), "%s", "Nemico Reale di Prova");
    fc->enemies[1].active = false;   /* un solo slot attivo: la scelta e' deterministica (sempre lo slot 0) */

    game->enemyEncountered[game->floor - 1][0] = false;
    game->discoveryQueueCount = 0;

    WorldSpawnCombatRoom(game);

    bool pushedOnce = (game->discoveryQueueCount == 1);
    bool nameOk = pushedOnce && (strcmp(game->discoveryQueue[0].name, "Nemico Reale di Prova") == 0);
    bool flagSet = game->enemyEncountered[game->floor - 1][0];

    if (!pushedOnce) fprintf(stderr, "DiscoveryTest: WorldSpawnCombatRoom doveva accodare esattamente una card (count=%d)\n", game->discoveryQueueCount);
    if (pushedOnce && !nameOk) fprintf(stderr, "DiscoveryTest: la card accodata da WorldSpawnCombatRoom ha nome '%s', atteso 'Nemico Reale di Prova'\n", game->discoveryQueue[0].name);
    if (!flagSet) fprintf(stderr, "DiscoveryTest: enemyEncountered non e' stato scritto da WorldSpawnCombatRoom\n");

    /* Stanza gia' incontrata (flag vero): un secondo spawn non deve pushare. */
    game->discoveryQueueCount = 0;
    WorldSpawnCombatRoom(game);
    bool noSecondPush = (game->discoveryQueueCount == 0);
    if (!noSecondPush) fprintf(stderr, "DiscoveryTest: un secondo WorldSpawnCombatRoom sullo stesso tipo gia' incontrato ha accodato una card (count=%d)\n", game->discoveryQueueCount);

    printf("  push reale da WorldSpawnCombatRoom (DEC-065): una card=%s, nome corretto=%s, flag scritto=%s, niente doppio push=%s\n",
           pushedOnce ? "si" : "NO", nameOk ? "si" : "NO", flagSet ? "si" : "NO", noSecondPush ? "si" : "NO");

    ok = pushedOnce && nameOk && flagSet && noSecondPush;
    return ok;
}

/* (2bis) DEC-131: cap della coda e drop-oldest. Sei push in fila su una coda
   di capienza DISCOVERY_QUEUE_MAX(=5) devono lasciare le 5 PIU' RECENTI,
   scartando silenziosamente la piu' vecchia (mai la piu' nuova, mai un
   errore). */
static bool DiscoveryTestQueueCapDropOldest(Game *game)
{
    bool ok = true;
    GameResetRunWithSeed(game, 20260728u);
    game->discoveryQueueCount = 0;

    char name[16];
    for (int i = 0; i < DISCOVERY_QUEUE_MAX + 1; i++)
    {
        snprintf(name, sizeof(name), "Card %d", i);
        GameQueueDiscoveryCard(game, name, "riga");
    }

    bool capRespected = (game->discoveryQueueCount == DISCOVERY_QUEUE_MAX);
    /* La piu' vecchia (Card 0) deve essere sparita; la coda va da Card 1 a
       Card 5 in ordine, la piu' nuova (Card 5) in coda. */
    bool oldestDropped = capRespected && (strcmp(game->discoveryQueue[0].name, "Card 1") == 0);
    bool newestKept = capRespected && (strcmp(game->discoveryQueue[DISCOVERY_QUEUE_MAX - 1].name, "Card 5") == 0);

    if (!capRespected) fprintf(stderr, "DiscoveryTest: il cap DISCOVERY_QUEUE_MAX non e' rispettato dopo %d push (count=%d)\n", DISCOVERY_QUEUE_MAX + 1, game->discoveryQueueCount);
    if (capRespected && !oldestDropped) fprintf(stderr, "DiscoveryTest: la card piu' vecchia non e' stata scartata (coda[0]='%s', attesa 'Card 1')\n", game->discoveryQueue[0].name);
    if (capRespected && !newestKept) fprintf(stderr, "DiscoveryTest: la card piu' nuova non e' in coda (coda[%d]='%s', attesa 'Card 5')\n", DISCOVERY_QUEUE_MAX - 1, game->discoveryQueue[DISCOVERY_QUEUE_MAX - 1].name);

    printf("  cap/drop-oldest della coda (DEC-131): cap rispettato=%s, piu' vecchia scartata=%s, piu' nuova in coda=%s\n",
           capRespected ? "si" : "NO", oldestDropped ? "si" : "NO", newestKept ? "si" : "NO");

    ok = capRespected && oldestDropped && newestKept;
    return ok;
}

/* (4) DEC-152: le card ANCORA IN CODA si scartano a morte del giocatore,
   senza toccare la registrazione gia' avvenuta nel Catalogo (qui
   rappresentata dal flag enemyEncountered, esattamente quello che
   RunCatalogWriteRun legge -- vedi src/content/run_catalog.c). DEC-159:
   la stessa chiamata a CombatDamagePlayer che scarta la coda popola anche la
   causa della sconfitta, letta poi da DrawRunResultsOverlay SOLO a
   PHASE_GAME_OVER. */
static bool DiscoveryTestDiscardOnDeath(Game *game)
{
    bool ok = true;
    GameResetRunWithSeed(game, 20260728u);

    /* Scoperta gia' registrata nel Catalogo (simula cio' che
       WorldSpawnCombatRoom scrive PRIMA di accodare la card, vedi il
       commento in src/world/world.c) e una card ancora in attesa. */
    game->enemyEncountered[0][0] = true;
    GameQueueDiscoveryCard(game, "Nemico di prova", "Una riga di prova.");
    bool queuedBeforeDeath = (game->discoveryQueueCount == 1);
    if (!queuedBeforeDeath) fprintf(stderr, "DiscoveryTest: la card di prova non risulta in coda prima della morte (count=%d)\n", game->discoveryQueueCount);

    /* Morte sintetica: hp a 1, un colpo che lo azzera. */
    game->player.hp = 1;
    game->player.invuln = 0.0f;
    CombatDamagePlayer(game, 5, "contatto con Nemico di prova");

    bool phaseOk = (game->phase == PHASE_GAME_OVER);
    bool queueDiscarded = (game->discoveryQueueCount == 0);
    bool catalogIntact = game->enemyEncountered[0][0];
    bool causeOk = (strcmp(game->deathCause, "contatto con Nemico di prova") == 0);

    if (!phaseOk) fprintf(stderr, "DiscoveryTest: la morte non ha portato a PHASE_GAME_OVER\n");
    if (!queueDiscarded) fprintf(stderr, "DiscoveryTest: la coda non e' stata scartata alla morte (count=%d)\n", game->discoveryQueueCount);
    if (!catalogIntact) fprintf(stderr, "DiscoveryTest: enemyEncountered e' stato toccato dallo scarto della coda (DEC-152 non deve farlo)\n");
    if (!causeOk) fprintf(stderr, "DiscoveryTest: game->deathCause e' '%s', atteso 'contatto con Nemico di prova' (DEC-159)\n", game->deathCause);

    printf("  scarto a morte (DEC-152) + causa (DEC-159): coda prima=%s, game over=%s, coda scartata=%s, Catalogo intatto=%s, causa='%s'\n",
           queuedBeforeDeath ? "si" : "NO", phaseOk ? "si" : "NO", queueDiscarded ? "si" : "NO", catalogIntact ? "si" : "NO", game->deathCause);

    ok = queuedBeforeDeath && phaseOk && queueDiscarded && catalogIntact && causeOk;
    return ok;
}

/* (5) DEC-152, il secondo caso: un vero cambio stanza (WorldTryEnterRoom)
   scarta la coda esattamente come la morte, senza toccare il Catalogo. */
static bool DiscoveryTestDiscardOnRoomChange(Game *game)
{
    bool ok = true;
    GameResetRunWithSeed(game, 20260728u);

    /* 'cleared' a vero OVUNQUE (stesso spirito di EconomyEnterRoomOfKind in
       game_tests.c, che forza i flag a mano per isolare cio' che interessa):
       la stanza di arrivo non deve spawnare nulla di suo (WorldSpawnCombatRoom/
       il boss), altrimenti le SUE scoperte vere si mescolerebbero al conteggio
       e questo test non isolerebbe piu' lo scarto della coda al cambio stanza. */
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            game->rooms[y][x].cleared = true;

    int dir = -1;
    for (int d = 0; d < 4; d++)
    {
        if (game->rooms[game->roomY][game->roomX].doors[d]) { dir = d; break; }
    }
    if (dir < 0)
    {
        fprintf(stderr, "DiscoveryTest: la stanza di partenza non ha nessuna porta col seed di prova\n");
        return false;
    }

    game->bossEncountered[0] = true;   /* scoperta gia' registrata, come sopra */
    GameQueueDiscoveryCard(game, "Boss di prova", "Una riga di prova.");
    bool queuedBeforeMove = (game->discoveryQueueCount == 1);
    if (!queuedBeforeMove) fprintf(stderr, "DiscoveryTest: la card di prova non risulta in coda prima del cambio stanza (count=%d)\n", game->discoveryQueueCount);

    WorldTryEnterRoom(game, dir);

    bool queueDiscarded = (game->discoveryQueueCount == 0);
    bool catalogIntact = game->bossEncountered[0];
    if (!queueDiscarded) fprintf(stderr, "DiscoveryTest: la coda non e' stata scartata al cambio stanza (count=%d)\n", game->discoveryQueueCount);
    if (!catalogIntact) fprintf(stderr, "DiscoveryTest: bossEncountered e' stato toccato dallo scarto della coda al cambio stanza (DEC-152 non deve farlo)\n");

    printf("  scarto a cambio stanza (DEC-152): coda prima=%s, coda scartata=%s, Catalogo intatto=%s\n",
           queuedBeforeMove ? "si" : "NO", queueDiscarded ? "si" : "NO", catalogIntact ? "si" : "NO");

    ok = queuedBeforeMove && queueDiscarded && catalogIntact;
    return ok;
}

bool GameDiscoveryTest(Game *game)
{
    bool ok = true;
    if (!DiscoveryTestHudFlag()) ok = false;
    if (!DiscoveryTestRealPushFromCombatRoom(game)) ok = false;
    if (!DiscoveryTestQueueCapDropOldest(game)) ok = false;
    if (!DiscoveryTestDiscardOnDeath(game)) ok = false;
    if (!DiscoveryTestDiscardOnRoomChange(game)) ok = false;
    if (!ok) fprintf(stderr, "GameDiscoveryTest: FALLITO -- vedi i messaggi sopra\n");
    return ok;
}
