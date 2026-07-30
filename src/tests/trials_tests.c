/* WP16 (DEC-042/DEC-027, docs/design/systems/rewards-and-economy.md "Prove
   specifiche della run"; docs/design/systems/floor-zero.md "Presentazione
   delle prove"). Come GameEconomyTest (src/tests/game_tests.c), gira dopo
   InitWindow e usa 'game' per davvero (GameResetRunWithSeed chiama
   AssetsLoad) ma non disegna nulla.

   Le verifiche funzionali (boss senza danno, arena vinta, stanza segreta
   trovata, stanza a tempo, fusione, acquisto al negozio) impiantano a mano
   la prova che vogliono esercitare (game->trials[0] = ...) invece di
   sperare che TrialsAssignForRun la estragga dal catalogo per un dato seme:
   la stessa tecnica con cui RoomsTestPourhouseInteraction/RoomsTestArena-
   Interaction (game_tests.c) costruiscono lo stato che vogliono isolare.
   L'ASSEGNAZIONE vera e propria (determinismo dal seed, 2-3 prove distinte)
   ha invece il proprio blocco dedicato, che chiama TrialsAssignForRun per
   davvero attraverso GameResetRunWithSeed. */

#include "tests/game_tests.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "game/trials.h"
#include "gameplay/fusion.h"
#include "script/script_items.h"
#include "world/world.h"

#include <stdio.h>
#include <string.h>

/* Semi di prova per gli archetipi PROBABILISTICI (segreta/arena/a tempo):
   nessuno di questi e' garantito per costruzione (WorldGenerateFloorMap,
   src/world/world.c), quindi si scansiona una manciata di semi/piani come
   fa gia' RoomsTestArenaInteraction/RoomsTestFindSecret in game_tests.c. */
static const unsigned int kTrialsTestSeeds[] = {
    20260727u, 1001u, 2002u, 3003u, 4004u, 5005u, 6006u, 7007u, 8008u, 424242u, 90210u, 5150u
};
#define TRIALS_TEST_SEED_COUNT ((int)(sizeof(kTrialsTestSeeds)/sizeof(kTrialsTestSeeds[0])))

/* Cerca 'kind' su un piano GIA' generato (game->floor corrente): usata per
   gli archetipi SEMPRE presenti (boss, negozio) sul piano 1 del seme fisso
   kTrialsTestSeeds[0], lo stesso gia' verificato da GameEconomyTest. */
static bool TrialsTestFindOnCurrentFloor(Game *game, RoomKind kind, int *outX, int *outY)
{
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!game->rooms[y][x].exists) continue;
            const RoomState *state = WorldRoomAt(game, x, y);
            if (state != &game->rooms[y][x]) continue;   /* solo la cella di STATO della stanza */
            if (state->kind == kind) { *outX = x; *outY = y; return true; }
        }
    return false;
}

/* Genera il piano 'floor' con seed 'seed' su un Game LOCALE azzerato sullo
   stack (come RoomsTestGenerateFloor in game_tests.c): nessun AssetsLoad --
   WorldSpawnRoomContents/WorldCheckRoomClear/WorldTryStartArenaChallenge
   operano su stato puro, mai su texture, esattamente come gia' verificato da
   RoomsTestArenaInteraction/RoomsTestSecretRooms/RoomsTestTimedRoomInteraction
   (game_tests.c) per gli stessi tre archetipi. */
static void TrialsTestGenerateFloor(unsigned int seed, int floor, Game *out)
{
    memset(out, 0, sizeof(*out));
    out->rng = seed;
    out->phase = PHASE_PLAY;
    WorldStartFloor(out, floor);
}

/* Cerca 'kind' provando i semi di kTrialsTestSeeds dal piano 'minFloor' in
   su, su un Game LOCALE leggero (TrialsTestGenerateFloor sopra): nessuno di
   questi tre archetipi (segreta/arena/a tempo) e' garantito per costruzione,
   quindi si scansiona come fa gia' RoomsTestFindSecret/RoomsTestArenaInteraction. */
static bool TrialsTestFindAnywhere(Game *out, RoomKind kind, int minFloor, int *outX, int *outY)
{
    for (int si = 0; si < TRIALS_TEST_SEED_COUNT; si++)
        for (int floor = minFloor; floor <= FLOOR_COUNT; floor++)
        {
            TrialsTestGenerateFloor(kTrialsTestSeeds[si], floor, out);
            if (TrialsTestFindOnCurrentFloor(out, kind, outX, outY)) return true;
        }
    return false;
}

/* ============================================================
   (a) Assegnazione: deterministica dal seed, 2-3 prove DISTINTE, R
   riassegna le IDENTICHE prove con lo stato ripulito, semi diversi -> di
   norma prove diverse.
   ============================================================ */
static bool TrialsTestAssignment(void)
{
    bool ok = true;
    Game game;
    memset(&game, 0, sizeof(game));
    const unsigned int seedA = 555001u;

    GameResetRunWithSeed(&game, seedA);
    int firstCount = game.trialCount;
    Trial firstRun[TRIAL_SLOTS_MAX];
    memcpy(firstRun, game.trials, sizeof(firstRun));

    if (firstCount < 2 || firstCount > 3)
    {
        fprintf(stderr, "GameTrialsTest: (a) numero di prove fuori range [2,3]: %d\n", firstCount);
        ok = false;
    }
    for (int i = 0; i < firstCount; i++)
        for (int j = i + 1; j < firstCount; j++)
            if (firstRun[i].kind == firstRun[j].kind)
            {
                fprintf(stderr, "GameTrialsTest: (a) due prove dello stesso tipo nella stessa run (indici %d/%d)\n", i, j);
                ok = false;
            }

    /* Sporca lo stato a mano (come farebbe una run giocata) prima del
       "reset rapido" simulato sotto: se il refresh non lo ripulisse
       davvero, questo test lo vedrebbe. */
    for (int i = 0; i < firstCount; i++)
        game.trials[i].state = (i%2 == 0) ? TRIAL_PASSED : TRIAL_FAILED;

    /* R = GameResetRunWithSeed con lo STESSO seed (src/game/game.c,
       GameUpdate/resetQueued): stessa chiamata, qui diretta. */
    GameResetRunWithSeed(&game, seedA);
    if (game.trialCount != firstCount)
    {
        fprintf(stderr, "GameTrialsTest: (a) stesso seed, conteggio diverso dopo il reset (%d -> %d)\n", firstCount, game.trialCount);
        ok = false;
    }
    for (int i = 0; i < firstCount && i < game.trialCount; i++)
    {
        const Trial *a = &firstRun[i];
        const Trial *b = &game.trials[i];
        if (a->kind != b->kind || a->param != b->param || a->bonus != b->bonus || strcmp(a->text, b->text) != 0)
        {
            fprintf(stderr, "GameTrialsTest: (a) stesso seed, la prova %d e' cambiata dopo il reset\n", i);
            ok = false;
        }
        if (b->state != TRIAL_IN_PROGRESS)
        {
            fprintf(stderr, "GameTrialsTest: (a) stesso seed, la prova %d non riparte con stato pulito (stato %d)\n", i, (int)b->state);
            ok = false;
        }
    }

    /* Semi diversi -> di norma prove diverse: non deve capitare che TUTTI i
       semi di prova producano lo stesso identico set (sintomo di uno stream
       non davvero derivato dal seed). */
    static const unsigned int kSeeds[] = { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u };
    Trial snapshots[8][TRIAL_SLOTS_MAX];
    int counts[8];
    for (int i = 0; i < 8; i++)
    {
        GameResetRunWithSeed(&game, kSeeds[i]);
        counts[i] = game.trialCount;
        memcpy(snapshots[i], game.trials, sizeof(snapshots[i]));
    }
    bool anyDifferent = false;
    for (int i = 1; i < 8 && !anyDifferent; i++)
    {
        if (counts[i] != counts[0]) { anyDifferent = true; break; }
        for (int k = 0; k < counts[0]; k++)
            if (snapshots[i][k].kind != snapshots[0][k].kind || snapshots[i][k].param != snapshots[0][k].param)
            { anyDifferent = true; break; }
    }
    if (!anyDifferent)
    {
        fprintf(stderr, "GameTrialsTest: (a) otto semi diversi hanno prodotto TUTTI le stesse prove\n");
        ok = false;
    }

    if (ok)
        printf("  [trials-a] %d prove assegnate dal seme %u, distinte, stesso seme -> stesse prove con stato pulito dopo reset, semi diversi -> prove diverse -> ok\n",
               firstCount, seedA);
    GameUnloadAssets(&game);
    return ok;
}

/* ============================================================
   (b) TRIAL_BOSS_NO_DAMAGE: simulazione vera sul boss del piano 1 (sempre
   presente) -- pulita = superata, danneggiata = fallita SUBITO (quel boss e'
   ormai sconfitto, non ci sara' un secondo tentativo).
   ============================================================ */
static bool TrialsTestBossNoDamage(Game *game)
{
    bool ok = true;
    const unsigned int seed = kTrialsTestSeeds[0];

    /* --- tentativo pulito: nessun colpo incassato --- */
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_BOSS_NO_DAMAGE;
    game->trials[0].param = 1;
    game->trials[0].bonus = TRIAL_BONUS_BOSS_NO_DAMAGE;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    int bx = -1, by = -1;
    if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
    {
        fprintf(stderr, "GameTrialsTest: (b) nessuna stanza boss nel piano 1 col seed %u\n", seed);
        return false;
    }
    game->roomX = bx; game->roomY = by;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);   /* TrialsOnBossRoomEntered azzera currentBossFightDamaged */
    if (game->currentBossFightDamaged)
    {
        fprintf(stderr, "GameTrialsTest: (b) currentBossFightDamaged non parte pulito all'ingresso\n");
        ok = false;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    if (game->trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (b) boss sconfitto senza danno ma la prova non e' superata (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    /* --- tentativo danneggiato: un colpo prima della vittoria --- */
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_BOSS_NO_DAMAGE;
    game->trials[0].param = 1;
    game->trials[0].bonus = TRIAL_BONUS_BOSS_NO_DAMAGE;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
    {
        fprintf(stderr, "GameTrialsTest: (b) nessuna stanza boss nel piano 1 col seed %u (secondo tentativo)\n", seed);
        return false;
    }
    game->roomX = bx; game->roomY = by;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    CombatDamagePlayer(game, 1, "colpo di prova");
    if (!game->currentBossFightDamaged)
    {
        fprintf(stderr, "GameTrialsTest: (b) CombatDamagePlayer dentro la stanza boss non ha segnato currentBossFightDamaged\n");
        ok = false;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (b) boss sconfitto CON danno ma la prova non e' fallita (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    if (ok) printf("  [trials-b] TRIAL_BOSS_NO_DAMAGE: pulito -> superata, con un colpo -> fallita subito -> ok\n");
    return ok;
}

/* ============================================================
   (c) TRIAL_FLOOR_UNDER_TIME: entro soglia -> superata, oltre soglia ->
   fallita, misurata dall'ingresso nel piano come la stanza a tempo di WP5.
   ============================================================ */
static bool TrialsTestFloorUnderTime(Game *game)
{
    bool ok = true;
    const unsigned int seed = kTrialsTestSeeds[0];

    for (int within = 1; within >= 0; within--)
    {
        GameResetRunWithSeed(game, seed);
        game->trialCount = 1;
        memset(&game->trials[0], 0, sizeof(game->trials[0]));
        game->trials[0].kind = TRIAL_FLOOR_UNDER_TIME;
        game->trials[0].param = 1;
        game->trials[0].bonus = TRIAL_BONUS_FLOOR_UNDER_TIME;
        game->trials[0].state = TRIAL_IN_PROGRESS;

        int bx, by;
        if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
        {
            fprintf(stderr, "GameTrialsTest: (c) nessuna stanza boss nel piano 1 col seed %u\n", seed);
            return false;
        }
        game->roomX = bx; game->roomY = by;
        EntitiesClear(game);
        WorldSpawnRoomContents(game);
        game->floorEntryElapsedSeconds = 0.0f;
        game->runElapsedSeconds = within ? 1.0f : 9999.0f;
        for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
        WorldCheckRoomClear(game);
        TrialState expected = within ? TRIAL_PASSED : TRIAL_FAILED;
        if (game->trials[0].state != expected)
        {
            fprintf(stderr, "GameTrialsTest: (c) piano %s soglia: attesa %d, ottenuta %d\n",
                    within ? "entro" : "oltre", (int)expected, (int)game->trials[0].state);
            ok = false;
        }
    }
    if (ok) printf("  [trials-c] TRIAL_FLOOR_UNDER_TIME: entro soglia -> superata, oltre soglia -> fallita -> ok\n");
    return ok;
}

/* ============================================================
   (d) TRIAL_SECRET_FOUND: trovare (= primo ingresso) una stanza segreta
   qualunque la supera.
   ============================================================ */
static bool TrialsTestSecretFound(void)
{
    Game probe;
    int sx = -1, sy = -1;
    if (!TrialsTestFindAnywhere(&probe, ROOM_SECRET, WORLD_SECRET_ROOM_MIN_FLOOR, &sx, &sy))
    {
        fprintf(stderr, "GameTrialsTest: (d) nessuna stanza segreta nei semi di prova: verifica non eseguita\n");
        return false;
    }
    probe.trialCount = 1;
    memset(&probe.trials[0], 0, sizeof(probe.trials[0]));
    probe.trials[0].kind = TRIAL_SECRET_FOUND;
    probe.trials[0].bonus = TRIAL_BONUS_SECRET_FOUND;
    probe.trials[0].state = TRIAL_IN_PROGRESS;

    probe.roomX = sx; probe.roomY = sy;
    WorldSpawnRoomContents(&probe);
    bool ok = true;
    if (probe.trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (d) stanza segreta trovata ma la prova non e' superata (stato %d)\n", (int)probe.trials[0].state);
        ok = false;
    }
    /* Rientrare non deve mai regredire uno stato gia' deciso. */
    WorldSpawnRoomContents(&probe);
    if (probe.trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (d) rientrare nella stanza segreta ha cambiato lo stato della prova\n");
        ok = false;
    }
    if (ok) printf("  [trials-d] TRIAL_SECRET_FOUND: stanza segreta trovata -> superata, stabile a rientro -> ok\n");
    return ok;
}

/* ============================================================
   (e) TRIAL_ARENA_WON: sfida accettata e vinta la supera; attraversare
   senza accettare non la tocca (resta in corso).
   ============================================================ */
static bool TrialsTestArenaWon(void)
{
    Game probe;
    int ax = -1, ay = -1;
    if (!TrialsTestFindAnywhere(&probe, ROOM_ARENA, WORLD_ARENA_ROOM_MIN_FLOOR, &ax, &ay))
    {
        fprintf(stderr, "GameTrialsTest: (e) nessuna arena di sfida nei semi di prova: verifica non eseguita\n");
        return false;
    }
    probe.trialCount = 1;
    memset(&probe.trials[0], 0, sizeof(probe.trials[0]));
    probe.trials[0].kind = TRIAL_ARENA_WON;
    probe.trials[0].bonus = TRIAL_BONUS_ARENA_WON;
    probe.trials[0].state = TRIAL_IN_PROGRESS;

    probe.roomX = ax; probe.roomY = ay;
    WorldSpawnRoomContents(&probe);

    Pickup *altar = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (probe.pickups[i].active && probe.pickups[i].kind == PICKUP_ARENA_ALTAR) { altar = &probe.pickups[i]; break; }
    if (!altar)
    {
        fprintf(stderr, "GameTrialsTest: (e) nessun segnale d'arena nella stanza trovata\n");
        return false;
    }

    /* Attraversare senza accettare: la prova resta in corso. */
    if (probe.trials[0].state != TRIAL_IN_PROGRESS)
    {
        fprintf(stderr, "GameTrialsTest: (e) entrare nell'arena senza accettare ha gia' toccato la prova\n");
        return false;
    }

    probe.player.pos = altar->pos;
    probe.interactQueued = true;
    if (!WorldTryStartArenaChallenge(&probe))
    {
        fprintf(stderr, "GameTrialsTest: (e) la conferma sul segnale non fa partire la sfida\n");
        return false;
    }
    bool ok = true;
    for (int i = 0; i < MAX_ENEMIES; i++) probe.enemies[i].active = false;
    WorldCheckRoomClear(&probe);
    if (probe.trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (e) arena vinta ma la prova non e' superata (stato %d)\n", (int)probe.trials[0].state);
        ok = false;
    }
    if (ok) printf("  [trials-e] TRIAL_ARENA_WON: sfida accettata e vinta -> superata -> ok\n");
    return ok;
}

/* ============================================================
   (f) TRIAL_TIMED_ROOM_WITHIN_THRESHOLD: raggiunta entro soglia la supera.
   ============================================================ */
static bool TrialsTestTimedRoom(void)
{
    Game probe;
    int tx = -1, ty = -1;
    if (!TrialsTestFindAnywhere(&probe, ROOM_TIMED, WORLD_TIMED_ROOM_MIN_FLOOR, &tx, &ty))
    {
        fprintf(stderr, "GameTrialsTest: (f) nessuna stanza a tempo nei semi di prova: verifica non eseguita\n");
        return false;
    }
    probe.trialCount = 1;
    memset(&probe.trials[0], 0, sizeof(probe.trials[0]));
    probe.trials[0].kind = TRIAL_TIMED_ROOM_WITHIN_THRESHOLD;
    probe.trials[0].bonus = TRIAL_BONUS_TIMED_ROOM;
    probe.trials[0].state = TRIAL_IN_PROGRESS;

    probe.roomX = tx; probe.roomY = ty;
    probe.floorEntryElapsedSeconds = 0.0f;
    probe.runElapsedSeconds = 0.0f;   /* ben entro qualunque soglia */
    WorldSpawnRoomContents(&probe);
    bool ok = true;
    if (probe.trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (f) stanza a tempo raggiunta entro soglia ma la prova non e' superata (stato %d)\n", (int)probe.trials[0].state);
        ok = false;
    }
    if (ok) printf("  [trials-f] TRIAL_TIMED_ROOM_WITHIN_THRESHOLD: raggiunta entro soglia -> superata -> ok\n");
    return ok;
}

/* ============================================================
   (g) TRIAL_FUSE_ITEM: una fusione riuscita la supera.
   ============================================================ */
static bool TrialsTestFuseItem(Game *game)
{
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    ScriptItemsInit(game, NULL);
    game->player.itemCount = 0;

    Item a = { 0 }; a.active = true; snprintf(a.name, sizeof(a.name), "%s", "Prova A");
    a.kind = ITEM_PASSIVE; a.rarity = RARITY_COMMON; a.slot = SLOT_HAND;
    Item b = { 0 }; b.active = true; snprintf(b.name, sizeof(b.name), "%s", "Prova B");
    b.kind = ITEM_PASSIVE; b.rarity = RARITY_UNCOMMON; b.slot = SLOT_HAND;

    game->player.items[0] = a; game->player.itemCount = 1; ScriptItemsOnAcquire(game, 0);
    game->player.items[1] = b; game->player.itemCount = 2; ScriptItemsOnAcquire(game, 1);
    ScriptItemsProcessDirty(game);
    game->player.flux = 3;

    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_FUSE_ITEM;
    game->trials[0].bonus = TRIAL_BONUS_FUSE_ITEM;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    Item fused;
    if (FusionPerform(game, 0, 1, &fused) != FUSION_OK)
    {
        fprintf(stderr, "GameTrialsTest: (g) la fusione di prova non e' riuscita\n");
        return false;
    }
    bool ok = true;
    if (game->trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (g) fusione riuscita ma la prova non e' superata (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }
    if (ok) printf("  [trials-g] TRIAL_FUSE_ITEM: fusione riuscita -> superata -> ok\n");
    return ok;
}

/* ============================================================
   (h) TRIAL_NO_SHOP_PURCHASE: un acquisto al negozio la fa fallire SUBITO,
   e resta fallita anche a fine run (mai "recuperata").
   ============================================================ */
static bool TrialsTestShopPurchase(Game *game)
{
    const unsigned int seed = kTrialsTestSeeds[0];
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_NO_SHOP_PURCHASE;
    game->trials[0].bonus = TRIAL_BONUS_NO_SHOP_PURCHASE;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    int sx, sy;
    if (!TrialsTestFindOnCurrentFloor(game, ROOM_SHOP, &sx, &sy))
    {
        fprintf(stderr, "GameTrialsTest: (h) nessun negozio nel piano 1 col seed %u\n", seed);
        return false;
    }
    game->roomX = sx; game->roomY = sy;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);

    Pickup *buy = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_ITEM && game->pickups[i].cost > 0)
        { buy = &game->pickups[i]; break; }
    if (!buy)
    {
        fprintf(stderr, "GameTrialsTest: (h) nessun oggetto a pagamento nel negozio trovato\n");
        return false;
    }
    game->player.coins = buy->cost + 10;
    game->player.pos = buy->pos;
    CombatUpdatePickups(game);

    bool ok = true;
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (h) un acquisto al negozio non ha fatto fallire subito la prova (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }
    /* La run finisce: la prova resta fallita, mai "recuperata" a posteriori. */
    TrialsFinalizeAtRunEnd(game);
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (h) la finalizzazione di fine run ha cambiato una prova gia' fallita\n");
        ok = false;
    }
    if (ok) printf("  [trials-h] TRIAL_NO_SHOP_PURCHASE: un acquisto -> fallita subito, mai recuperata a fine run -> ok\n");
    return ok;
}

/* ============================================================
   (i) TRIAL_NO_SHOP_PURCHASE: MAI comprato per tutta la run -> superata
   solo a fine run (non prima: nessun evento la marca superata mentre la
   run e' ancora in corso, per costruzione -- solo TrialsFinalizeAtRunEnd
   la chiude in positivo).
   ============================================================ */
static bool TrialsTestNoShopPurchasePassesAtRunEnd(Game *game)
{
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_NO_SHOP_PURCHASE;
    game->trials[0].bonus = TRIAL_BONUS_NO_SHOP_PURCHASE;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    TrialsFinalizeAtRunEnd(game);
    bool ok = true;
    if (game->trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (i) nessun acquisto per tutta la run ma la prova non risulta superata a fine run (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }
    if (ok) printf("  [trials-i] TRIAL_NO_SHOP_PURCHASE: mai comprato per tutta la run -> superata a fine run -> ok\n");
    return ok;
}

/* ============================================================
   (j) TrialsFinalizeAtRunEnd: TRIAL_END_WITH_INGOTS confrontata con la
   soglia, ogni altra prova ancora in corso diventa fallita (la run e'
   finita), una prova gia' decisa non viene mai ritoccata, i bonus (derivati
   da TrialsBonusTotal) restano invariati chiamando la funzione una seconda
   volta -- "arrivano una volta sola".
   ============================================================ */
static bool TrialsTestFinalizeAndBonus(Game *game)
{
    bool ok = true;
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    game->trialCount = 3;
    memset(game->trials, 0, sizeof(game->trials));
    game->trials[0].kind = TRIAL_END_WITH_INGOTS;
    game->trials[0].param = TRIAL_END_INGOTS_TARGET;
    game->trials[0].bonus = TRIAL_BONUS_END_WITH_INGOTS;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->trials[1].kind = TRIAL_FUSE_ITEM;
    game->trials[1].bonus = TRIAL_BONUS_FUSE_ITEM;
    game->trials[1].state = TRIAL_PASSED;   /* gia' superata PRIMA della fine della run */
    game->trials[2].kind = TRIAL_SECRET_FOUND;
    game->trials[2].bonus = TRIAL_BONUS_SECRET_FOUND;
    game->trials[2].state = TRIAL_IN_PROGRESS;   /* mai successa: deve fallire a fine run */

    game->player.coins = TRIAL_END_INGOTS_TARGET;   /* esattamente la soglia: >= deve bastare */
    TrialsFinalizeAtRunEnd(game);

    if (game->trials[0].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (j) Ingots esattamente alla soglia ma TRIAL_END_WITH_INGOTS non e' superata\n");
        ok = false;
    }
    if (game->trials[1].state != TRIAL_PASSED)
    {
        fprintf(stderr, "GameTrialsTest: (j) una prova gia' superata prima della fine e' stata ritoccata\n");
        ok = false;
    }
    if (game->trials[2].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (j) una prova mai successa non e' fallita alla fine della run\n");
        ok = false;
    }
    int passedOnce = TrialsPassedCount(game);
    int bonusOnce = TrialsBonusTotal(game);
    int expectedBonus = TRIAL_BONUS_END_WITH_INGOTS + TRIAL_BONUS_FUSE_ITEM;
    if (passedOnce != 2 || bonusOnce != expectedBonus)
    {
        fprintf(stderr, "GameTrialsTest: (j) conteggio/bonus inattesi dopo la finalizzazione: %d superate, +%d punti (attesi 2, +%d)\n",
                passedOnce, bonusOnce, expectedBonus);
        ok = false;
    }

    /* Idempotenza: una seconda finalizzazione non cambia nulla, quindi il
       bonus (sempre DERIVATO da TrialsBonusTotal, mai sommato una volta per
       tutte in un campo a parte) non puo' mai "arrivare due volte". */
    TrialsFinalizeAtRunEnd(game);
    if (TrialsBonusTotal(game) != expectedBonus || TrialsPassedCount(game) != passedOnce)
    {
        fprintf(stderr, "GameTrialsTest: (j) una seconda finalizzazione ha cambiato bonus/conteggio\n");
        ok = false;
    }

    /* Sotto soglia: TRIAL_END_WITH_INGOTS fallisce. */
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_END_WITH_INGOTS;
    game->trials[0].param = TRIAL_END_INGOTS_TARGET;
    game->trials[0].bonus = TRIAL_BONUS_END_WITH_INGOTS;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->player.coins = TRIAL_END_INGOTS_TARGET - 1;
    TrialsFinalizeAtRunEnd(game);
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (j) un Ingot sotto la soglia ma TRIAL_END_WITH_INGOTS non e' fallita\n");
        ok = false;
    }

    if (ok) printf("  [trials-j] TrialsFinalizeAtRunEnd: soglia Ingots, prove mai successe fallite, gia' decise intatte, bonus stabile a due chiamate -> ok\n");
    return ok;
}

/* ============================================================
   (k) Isolamento da game->rng (seconda tornata, bocciatura del giudice): la
   garanzia piu' ripetuta del lavoro -- "stream locale, mai game->rng" -- non
   era verificata da nulla. Chiamata DIRETTAMENTE (non attraverso
   GameResetRunWithSeed, che azzera game->rng da sola prima di ogni chiamata
   e nasconderebbe il difetto): game->rng deve restare bit-identico
   prima/dopo, e uno stato di PARTENZA arbitrario di game->rng (diverso, o
   gia' avanzato di un numero qualunque di estrazioni, come farebbe
   WorldStartFloor(1) chiamato PRIMA di questa funzione in
   GameResetRunWithSeed) non deve cambiare le prove assegnate a parita' di
   runSeed. Verificato: prima di questa correzione, sostituire lo stream
   locale con GameRngNext(&game->rng) nelle tre estrazioni lasciava verdi
   tutte le altre suite (nessuna le esercitava).
   ============================================================ */
static bool TrialsTestRngIsolation(void)
{
    bool ok = true;
    const unsigned int seed = 777001u;

    /* (a) game->rng bit-identico prima e dopo la chiamata. */
    Game a;
    memset(&a, 0, sizeof(a));
    a.runSeed = seed;
    a.rng = 0x1234ABCDu;
    unsigned int rngBefore = a.rng;
    TrialsAssignForRun(&a);
    if (a.rng != rngBefore)
    {
        fprintf(stderr, "GameTrialsTest: (k) TrialsAssignForRun ha toccato game->rng (era %u, ora %u)\n", rngBefore, a.rng);
        ok = false;
    }

    /* (b) stesso runSeed, game->rng arbitrario e DIVERSO da 'a' -> stesse prove. */
    Game b;
    memset(&b, 0, sizeof(b));
    b.runSeed = seed;
    b.rng = 0xDEADBEEFu;
    TrialsAssignForRun(&b);

    /* (c) stesso runSeed, game->rng ottenuto CONSUMANDO un numero arbitrario
       di estrazioni prima della chiamata -> stesse prove anche qui. */
    Game c;
    memset(&c, 0, sizeof(c));
    c.runSeed = seed;
    c.rng = 555u;
    for (int i = 0; i < 37; i++) GameRngNext(&c.rng);
    TrialsAssignForRun(&c);

    if (a.trialCount != b.trialCount || a.trialCount != c.trialCount)
    {
        fprintf(stderr, "GameTrialsTest: (k) stesso runSeed, game->rng diverso -> conteggio di prove diverso (%d/%d/%d)\n",
                a.trialCount, b.trialCount, c.trialCount);
        ok = false;
    }
    int shortest = a.trialCount;
    if (b.trialCount < shortest) shortest = b.trialCount;
    if (c.trialCount < shortest) shortest = c.trialCount;
    for (int i = 0; i < shortest; i++)
    {
        if (a.trials[i].kind != b.trials[i].kind || a.trials[i].param != b.trials[i].param ||
            a.trials[i].bonus != b.trials[i].bonus || strcmp(a.trials[i].text, b.trials[i].text) != 0 ||
            a.trials[i].kind != c.trials[i].kind || a.trials[i].param != c.trials[i].param ||
            a.trials[i].bonus != c.trials[i].bonus || strcmp(a.trials[i].text, c.trials[i].text) != 0)
        {
            fprintf(stderr, "GameTrialsTest: (k) stesso runSeed, game->rng diverso -> la prova %d e' cambiata (TrialsAssignForRun leggerebbe game->rng invece dello stream locale)\n", i);
            ok = false;
        }
    }

    if (ok) printf("  [trials-k] TrialsAssignForRun non tocca game->rng e ignora il suo stato di partenza a parita' di runSeed -> ok\n");
    return ok;
}

/* ============================================================
   (l) TrialsFinalizeAtRunEnd risolve DA SOLA a fine run vera, senza bisogno
   di chiamarla a mano: game over via CombatDamagePlayer, vittoria via
   CombatUpdatePickups sul portale d'uscita del piano finale. Verificato:
   prima di questa correzione, rimuovere ENTRAMBE le chiamate a
   TrialsFinalizeAtRunEnd da src/gameplay/combat.c lasciava verdi tutte le
   altre suite.
   ============================================================ */
static bool TrialsTestRunEndAutoFinalize(Game *game)
{
    bool ok = true;
    const unsigned int seed = kTrialsTestSeeds[0];

    /* --- game over: CombatDamagePlayer porta a PHASE_GAME_OVER da sola --- */
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_FUSE_ITEM;   /* mai innescata: deve fallire SOLO a fine run */
    game->trials[0].bonus = TRIAL_BONUS_FUSE_ITEM;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->player.hp = 1;
    CombatDamagePlayer(game, 999, "colpo di prova");
    if (game->phase != PHASE_GAME_OVER)
    {
        fprintf(stderr, "GameTrialsTest: (l) CombatDamagePlayer non ha portato a PHASE_GAME_OVER\n");
        return false;
    }
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (l) game over ma la prova ancora in corso non e' stata risolta senza chiamare TrialsFinalizeAtRunEnd a mano (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    /* --- vittoria: CombatUpdatePickups sul portale del piano finale porta a PHASE_WIN da sola --- */
    GameResetRunWithSeed(game, seed);
    WorldStartFloor(game, FLOOR_COUNT);
    int bx = -1, by = -1;
    if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
    {
        fprintf(stderr, "GameTrialsTest: (l) nessuna stanza boss nel piano finale col seed %u\n", seed);
        return false;
    }
    game->roomX = bx; game->roomY = by;
    RoomState *boss = WorldCurrentRoomMutable(game);
    boss->cleared = true;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);   /* boss cleared: ripiazza il portale d'uscita */

    Pickup *exitPickup = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_EXIT) { exitPickup = &game->pickups[i]; break; }
    if (!exitPickup)
    {
        fprintf(stderr, "GameTrialsTest: (l) nessun portale d'uscita nella stanza boss ripulita del piano finale\n");
        return false;
    }

    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_FUSE_ITEM;
    game->trials[0].bonus = TRIAL_BONUS_FUSE_ITEM;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->player.pos = exitPickup->pos;
    CombatUpdatePickups(game);

    if (game->phase != PHASE_WIN)
    {
        fprintf(stderr, "GameTrialsTest: (l) toccare il portale d'uscita del piano finale non ha portato a PHASE_WIN\n");
        ok = false;
    }
    if (game->trials[0].state != TRIAL_FAILED)
    {
        fprintf(stderr, "GameTrialsTest: (l) vittoria ma la prova ancora in corso non e' stata risolta senza chiamare TrialsFinalizeAtRunEnd a mano (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    if (ok) printf("  [trials-l] game over e vittoria risolvono davvero le prove ancora in corso senza chiamare TrialsFinalizeAtRunEnd a mano -> ok\n");
    return ok;
}

/* ============================================================
   (m) Guardia sul piano bersaglio: TRIAL_BOSS_NO_DAMAGE/TRIAL_FLOOR_UNDER_TIME
   non devono muoversi quando si ripulisce il boss di un piano DIVERSO da
   quello che la prova chiede (t->param != game->floor). Verificato: prima di
   questa correzione, neutralizzare 't->param == game->floor' in
   TrialsOnRoomCleared lasciava verdi tutte le altre suite.
   ============================================================ */
static bool TrialsTestFloorParamGuard(Game *game)
{
    bool ok = true;
    const unsigned int seed = kTrialsTestSeeds[0];

    /* TRIAL_BOSS_NO_DAMAGE con bersaglio il piano 3, ma si ripulisce (senza
       danno) il boss del piano 1: la prova non deve muoversi. */
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_BOSS_NO_DAMAGE;
    game->trials[0].param = 3;
    game->trials[0].bonus = TRIAL_BONUS_BOSS_NO_DAMAGE;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    int bx, by;
    if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
    {
        fprintf(stderr, "GameTrialsTest: (m) nessuna stanza boss nel piano 1 col seed %u\n", seed);
        return false;
    }
    game->roomX = bx; game->roomY = by;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    if (game->trials[0].state != TRIAL_IN_PROGRESS)
    {
        fprintf(stderr, "GameTrialsTest: (m) TRIAL_BOSS_NO_DAMAGE per il piano 3 e' stata toccata ripulendo il boss del piano 1 (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    /* Stesso principio per TRIAL_FLOOR_UNDER_TIME. */
    GameResetRunWithSeed(game, seed);
    game->trialCount = 1;
    memset(&game->trials[0], 0, sizeof(game->trials[0]));
    game->trials[0].kind = TRIAL_FLOOR_UNDER_TIME;
    game->trials[0].param = 3;
    game->trials[0].bonus = TRIAL_BONUS_FLOOR_UNDER_TIME;
    game->trials[0].state = TRIAL_IN_PROGRESS;

    if (!TrialsTestFindOnCurrentFloor(game, ROOM_BOSS, &bx, &by))
    {
        fprintf(stderr, "GameTrialsTest: (m) nessuna stanza boss nel piano 1 col seed %u (secondo controllo)\n", seed);
        return false;
    }
    game->roomX = bx; game->roomY = by;
    EntitiesClear(game);
    WorldSpawnRoomContents(game);
    game->floorEntryElapsedSeconds = 0.0f;
    game->runElapsedSeconds = 1.0f;   /* ben entro qualunque soglia, per non confondere la causa */
    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i].active = false;
    WorldCheckRoomClear(game);
    if (game->trials[0].state != TRIAL_IN_PROGRESS)
    {
        fprintf(stderr, "GameTrialsTest: (m) TRIAL_FLOOR_UNDER_TIME per il piano 3 e' stata toccata ripulendo il piano 1 (stato %d)\n", (int)game->trials[0].state);
        ok = false;
    }

    if (ok) printf("  [trials-m] TRIAL_BOSS_NO_DAMAGE/TRIAL_FLOOR_UNDER_TIME ignorano il boss di un piano diverso dal bersaglio -> ok\n");
    return ok;
}

/* ============================================================
   (n) TRIAL_SECRET_FOUND/TRIAL_ARENA_WON/TRIAL_TIMED_ROOM_WITHIN_THRESHOLD:
   se il rispettivo archetipo non e' MAI comparso in nessun piano della run
   (Game.*EverGenerated rimasto falso), la prova ancora in corso a fine run
   si SCARTA (TRIAL_VOID) invece di fallire, ed e' esclusa dal denominatore
   mostrato al giocatore (TrialsCountedTotal). Se invece l'archetipo E'
   comparso ma la prova resta comunque in corso, fallisce per davvero
   (TRIAL_FAILED), come ogni altro tipo.
   ============================================================ */
static bool TrialsTestVoidWhenArchetypeNeverGenerated(Game *game)
{
    bool ok = true;

    /* --- mai comparso: la prova si scarta, non fallisce --- */
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    game->timedRoomEverGenerated = false;
    game->secretRoomEverGenerated = false;
    game->arenaRoomEverGenerated = false;
    game->trialCount = 3;
    memset(game->trials, 0, sizeof(game->trials));
    game->trials[0].kind = TRIAL_SECRET_FOUND;
    game->trials[0].bonus = TRIAL_BONUS_SECRET_FOUND;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->trials[1].kind = TRIAL_ARENA_WON;
    game->trials[1].bonus = TRIAL_BONUS_ARENA_WON;
    game->trials[1].state = TRIAL_IN_PROGRESS;
    game->trials[2].kind = TRIAL_TIMED_ROOM_WITHIN_THRESHOLD;
    game->trials[2].bonus = TRIAL_BONUS_TIMED_ROOM;
    game->trials[2].state = TRIAL_IN_PROGRESS;

    TrialsFinalizeAtRunEnd(game);
    for (int i = 0; i < 3; i++)
        if (game->trials[i].state != TRIAL_VOID)
        {
            fprintf(stderr, "GameTrialsTest: (n) archetipo mai comparso ma la prova %d non e' TRIAL_VOID (stato %d)\n", i, (int)game->trials[i].state);
            ok = false;
        }
    if (TrialsCountedTotal(game) != 0)
    {
        fprintf(stderr, "GameTrialsTest: (n) TrialsCountedTotal non esclude le prove annullate (%d, attese 0)\n", TrialsCountedTotal(game));
        ok = false;
    }
    if (TrialsPassedCount(game) != 0 || TrialsBonusTotal(game) != 0)
    {
        fprintf(stderr, "GameTrialsTest: (n) una prova annullata conta come superata o paga un bonus\n");
        ok = false;
    }

    /* --- comparso davvero ma non soddisfatta: fallisce per davvero --- */
    GameResetRunWithSeed(game, kTrialsTestSeeds[0]);
    game->timedRoomEverGenerated = true;
    game->secretRoomEverGenerated = true;
    game->arenaRoomEverGenerated = true;
    game->trialCount = 3;
    memset(game->trials, 0, sizeof(game->trials));
    game->trials[0].kind = TRIAL_SECRET_FOUND;
    game->trials[0].bonus = TRIAL_BONUS_SECRET_FOUND;
    game->trials[0].state = TRIAL_IN_PROGRESS;
    game->trials[1].kind = TRIAL_ARENA_WON;
    game->trials[1].bonus = TRIAL_BONUS_ARENA_WON;
    game->trials[1].state = TRIAL_IN_PROGRESS;
    game->trials[2].kind = TRIAL_TIMED_ROOM_WITHIN_THRESHOLD;
    game->trials[2].bonus = TRIAL_BONUS_TIMED_ROOM;
    game->trials[2].state = TRIAL_IN_PROGRESS;

    TrialsFinalizeAtRunEnd(game);
    for (int i = 0; i < 3; i++)
        if (game->trials[i].state != TRIAL_FAILED)
        {
            fprintf(stderr, "GameTrialsTest: (n) archetipo comparso ma non soddisfatta, la prova %d non e' TRIAL_FAILED (stato %d)\n", i, (int)game->trials[i].state);
            ok = false;
        }
    if (TrialsCountedTotal(game) != 3)
    {
        fprintf(stderr, "GameTrialsTest: (n) TrialsCountedTotal esclude prove FALLITE per davvero (%d, attese 3)\n", TrialsCountedTotal(game));
        ok = false;
    }

    if (ok) printf("  [trials-n] archetipo mai comparso -> TRIAL_VOID (scartata, esclusa dal conteggio); archetipo comparso ma non soddisfatta -> TRIAL_FAILED per davvero -> ok\n");
    return ok;
}

/* (o) PRESENTAZIONE (bocciatura del giudice, 30/07): le prove assegnate
   devono FINIRE DAVVERO nella coda delle card di scoperta -- cancellare il
   ciclo di presentazione in TrialsAssignForRun lasciava l'intera suite
   verde, cioe' il giocatore poteva non vedere mai le sue prove senza che un
   test se ne accorgesse. Gira PRIMA di qualunque GameDiscardPendingDiscoveries
   e su un Game azzerato: esattamente lo stato del reset vero.
   In piu' la garanzia di SILENZIO al boot: dopo il solo reset nessuna card
   e' ancora ATTIVA (visibile) -- il suono vive alla promozione dentro
   GameUpdate (vedi game.c), che il boot a menu vuoto non chiama mai. */
static bool TrialsTestPresentation(void)
{
    bool ok = true;
    Game game;
    memset(&game, 0, sizeof(game));
    GameResetRunWithSeed(&game, kTrialsTestSeeds[0]);

    if (game.discoveryQueueCount != game.trialCount)
    {
        fprintf(stderr, "GameTrialsTest: (o) coda di presentazione %d card per %d prove (attese uguali)\n",
                game.discoveryQueueCount, game.trialCount);
        ok = false;
    }
    for (int i = 0; i < game.trialCount && i < game.discoveryQueueCount; i++)
    {
        const DiscoveryCard *card = &game.discoveryQueue[i];
        if (strcmp(card->name, "Prova") != 0)
        {
            fprintf(stderr, "GameTrialsTest: (o) card %d della presentazione con name '%s' (atteso 'Prova')\n", i, card->name);
            ok = false;
        }
        if (strcmp(card->line, game.trials[i].text) != 0)
        {
            fprintf(stderr, "GameTrialsTest: (o) card %d con testo diverso dalla prova corrispondente\n", i);
            ok = false;
        }
    }
    if (game.discoveryActiveValid)
    {
        fprintf(stderr, "GameTrialsTest: (o) una card e' gia' ATTIVA dopo il solo reset: al boot suonerebbe a menu vuoto\n");
        ok = false;
    }
    if (ok) printf("  [trials-o] presentazione: %d card 'Prova' in coda col testo delle prove, nessuna attiva prima di GameUpdate -> ok\n", game.trialCount);
    return ok;
}

bool GameTrialsTest(Game *game)
{
    bool ok = true;
    if (!TrialsTestAssignment()) ok = false;
    if (!TrialsTestPresentation()) ok = false;
    if (!TrialsTestBossNoDamage(game)) ok = false;
    if (!TrialsTestFloorUnderTime(game)) ok = false;
    if (!TrialsTestSecretFound()) ok = false;
    if (!TrialsTestArenaWon()) ok = false;
    if (!TrialsTestTimedRoom()) ok = false;
    if (!TrialsTestFuseItem(game)) ok = false;
    if (!TrialsTestShopPurchase(game)) ok = false;
    if (!TrialsTestNoShopPurchasePassesAtRunEnd(game)) ok = false;
    if (!TrialsTestFinalizeAndBonus(game)) ok = false;
    if (!TrialsTestRngIsolation()) ok = false;
    if (!TrialsTestRunEndAutoFinalize(game)) ok = false;
    if (!TrialsTestFloorParamGuard(game)) ok = false;
    if (!TrialsTestVoidWhenArchetypeNeverGenerated(game)) ok = false;
    return ok;
}
