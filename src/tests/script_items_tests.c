/* Test dell'API di gioco a handle e delle callback degli oggetti (fase
   3a-L2, src/script/script_api.c e src/script/script_items.c). Vedi la
   spec, docs/superpowers/specs/2026-07-13-lua-sandbox-design.md, sezioni
   5-9, e il task brief (criteri di successo).

   Come src/tests/script_sandbox_tests.c, ogni test costruisce un Game LOCALE
   sullo stack (mai tramite GameResetRun: non serve ne' raylib ne' un atlas,
   solo i campi che i moduli sotto test leggono davvero) e chiama le API
   pubbliche vere di combat.c/script_items.c: e' un'esecuzione reale, non una
   simulazione della logica. */

#include "tests/game_tests.h"

#include "game/game_internal.h"
#include "script/script_items.h"
#include "script/script_sandbox.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

/* Un Game minimo ma "vero": stessi valori di base di GameResetRun
   (src/game/game.c), senza atlas/finestra/mondo (nessuno dei test qui sotto
   ne ha bisogno). ScriptItemsInit deriva gia' player.damage/fireDelay/... dai
   base* con zero oggetti, esattamente come farebbe GameResetRun. */
static Game MakeBaseGame(unsigned int seed)
{
    Game game;
    memset(&game, 0, sizeof(game));
    game.phase = PHASE_PLAY;
    game.rng = seed;
    game.player.pos = (Vector2){ (ROOM_X + ROOM_RIGHT)*0.5f, (ROOM_Y + ROOM_BOTTOM)*0.5f };
    game.player.radius = 14.0f;
    game.player.hp = 6;
    game.player.baseDamage = 8.0f;
    game.player.baseFireDelay = 0.23f;
    game.player.baseShotSpeed = 520.0f;
    game.player.baseShotRadius = 5.0f;
    game.player.baseSpeed = 224.0f;
    game.player.baseMaxHp = 6;
    ScriptItemsInit(&game);
    return game;
}

/* Riproduce SOLO la parte di CombatApplyItem (src/gameplay/combat.c, static)
   che questi test devono esercitare: assegnare lo slot e passare dalla
   stessa coppia ScriptItemsOnAcquire/ScriptItemsProcessDirty che il gioco
   vero chiama al pickup. Non replica il messaggio a schermo ne' il bonus di
   guarigione SLOT_BODY: nessuno dei due e' rilevante per l'API a handle o il
   sistema delle cache. */
static void TestAddItem(Game *game, Item item)
{
    int idx = game->player.itemCount;
    if (idx >= MAX_ITEMS) idx = MAX_ITEMS - 1; else game->player.itemCount++;
    game->player.items[idx] = item;
    game->player.traits |= item.traits;
    ScriptItemsOnAcquire(game, idx);
    ScriptItemsProcessDirty(game);
}

static int CountActiveShots(const Game *game)
{
    int count = 0;
    for (int i = 0; i < MAX_SHOTS; i++) if (game->shots[i].active) count++;
    return count;
}

static int CountActiveShotsWithTrait(const Game *game, unsigned int trait)
{
    int count = 0;
    for (int i = 0; i < MAX_SHOTS; i++) if (game->shots[i].active && (game->shots[i].traits & trait)) count++;
    return count;
}

/* ============================================================
   Test A (il "headline test" della task brief): "ogni terzo colpo
   si sdoppia e i frammenti inseguono il nemico piu' vicino, ma solo
   se il giocatore ha meno di tre cuori" -- qualcosa che la mini-VM a
   quattro operazioni non puo' esprimere (nessun contatore persistente,
   nessuna condizione sull'hp, nessun bersaglio dinamico).
   ============================================================ */

/* Il primo frammento punta SUBITO al nemico piu' vicino (usa nearest_enemy/
   enemy_x/enemy_y: l'API a handle di lettura); il secondo mantiene la direzione
   di sparo originale e si affida INTERAMENTE alla fisica homing gia'
   esistente in CombatUpdateShots (src/gameplay/combat.c) per raggiungere il
   bersaglio nei frame successivi. Verificato SUL SERIO (vedi sotto) che sia
   proprio quest'ultimo a girare nel tempo: e' la prova che "inseguono"
   parla della fisica per-frame, non di una mira una tantum al momento dello
   sparo. */
static const char *HEADLINE_LUA =
    "shot_count = 0\n"
    "function on_fire(x, y, dx, dy)\n"
    "  shot_count = shot_count + 1\n"
    "  if shot_count % 3 == 0 and player_hp() < 3 then\n"
    "    local hx, hy = dx, dy\n"
    "    local id = nearest_enemy(x, y)\n"
    "    if id ~= nil then\n"
    "      local ex, ey = enemy_x(id), enemy_y(id)\n"
    "      local ddx, ddy = ex - x, ey - y\n"
    "      local len = math.sqrt(ddx*ddx + ddy*ddy)\n"
    "      if len > 0.0001 then\n"
    "        hx = ddx/len\n"
    "        hy = ddy/len\n"
    "      end\n"
    "    end\n"
    "    spawn_shot(x, y, hx, hy, 380, 3, 4, TRAIT_HOMING)\n"
    "    spawn_shot(x, y, dx, dy, 380, 3, 4, TRAIT_HOMING)\n"
    "  end\n"
    "end\n";

static bool TestHeadlineSplitHoming(void)
{
    Game game = MakeBaseGame(4242u);
    Item item = { 0 };
    item.active = true;
    snprintf(item.name, sizeof(item.name), "Corona Frantumante");
    item.slot = SLOT_HAND;
    snprintf(item.script, sizeof(item.script), "on_hit:projectile,1,300,none");   /* rete di sicurezza, non usata qui */
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", HEADLINE_LUA);
    TestAddItem(&game, item);

    bool loaded = ScriptItemsHasActiveLua(&game, 0);
    printf("  [A] script caricato: %s\n", loaded ? "si" : "NO");

    /* Vita piena (hp=6, >=3 cuori): il terzo colpo NON deve sdoppiarsi. */
    game.player.hp = 6;
    int before = CountActiveShots(&game);
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });   /* shot_count arriva a 3 */
    int createdFullHp = CountActiveShots(&game) - before;
    bool noSplitAtFullHp = createdFullHp == 3;
    printf("  [A] hp pieno (6): 3 spari -> %d colpi attivi (attesi 3: nessuno sdoppiato)\n", createdFullHp);

    /* Vita bassa (hp=2, <3 cuori): il PROSSIMO terzo colpo (shot_count=6)
       deve sdoppiarsi in due frammenti con TRAIT_HOMING. Il nemico e'
       piazzato a ~150 gradi dalla direzione di sparo (1,0): abbastanza
       "dietro" da rendere la direzione originale chiaramente sbagliata
       (dot iniziale atteso ~ -0.87), ma DELIBERATAMENTE non esattamente
       opposta (180 gradi). Il motivo e' un dettaglio reale della formula di
       homing di CombatUpdateShots (media pesata poi rinormalizzata,
       90% direzione attuale + 10% direzione desiderata): quando le due
       direzioni sono ESATTAMENTE opposte la media pesata e' ancora
       esattamente opposta dopo la rinormalizzazione (0.9*(-d)+0.1*d =
       -0.8*d, che normalizzato torna ad essere -d) -- un punto di
       equilibrio instabile in cui il colpo non gira MAI, verificato
       empiricamente proprio scrivendo questo test. 150 gradi e' ben lontano
       da quel punto e lascia comunque una direzione di partenza chiaramente
       "sbagliata" da correggere. */
    game.player.hp = 2;
    EntitiesAddEnemy(&game, ENEMY_CHASER, (Vector2){ game.player.pos.x - 260.0f, game.player.pos.y + 150.0f });
    int enemyIdx = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) if (game.enemies[i].active) { enemyIdx = i; break; }
    float enemyHpBefore = enemyIdx >= 0 ? game.enemies[enemyIdx].hp : 0.0f;

    int before2 = CountActiveShots(&game);
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });   /* shot_count=4 */
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });   /* shot_count=5 */
    int homingBefore6th = CountActiveShotsWithTrait(&game, TRAIT_HOMING);
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });   /* shot_count=6: sdoppia! */
    int createdLowHp = CountActiveShots(&game) - before2;
    int homingNew = CountActiveShotsWithTrait(&game, TRAIT_HOMING) - homingBefore6th;
    bool splitAtLowHp = createdLowHp == 5 && homingNew == 2;   /* 3 normali + 2 frammenti */
    printf("  [A] hp basso (2): 3 spari -> %d colpi attivi (attesi 5: 3 normali + 2 frammenti), nuovi homing=%d (attesi 2)\n",
           createdLowHp, homingNew);

    /* I frammenti inseguono per davvero: prendi quello nato mantenendo la
       direzione di sparo originale (1,0) -- vel.x nettamente positivo,
       l'altro frammento (mirato subito al nemico dallo script) ha invece
       vel.x negativo -- e verifica che la fisica homing gia' esistente in
       CombatUpdateShots lo faccia girare verso il nemico nel tempo, oppure
       che colpisca davvero il nemico (segno ancora piu' forte di successo). */
    int homingIdx = -1;
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        if (game.shots[i].active && (game.shots[i].traits & TRAIT_HOMING) && game.shots[i].vel.x > 300.0f)
        {
            homingIdx = i;
            break;
        }
    }
    float initialDot = -2.0f;   /* fuori range [-1,1]: sentinella "non calcolato" */
    if (homingIdx >= 0 && enemyIdx >= 0)
    {
        Vector2 toEnemy = { game.enemies[enemyIdx].pos.x - game.shots[homingIdx].pos.x,
                             game.enemies[enemyIdx].pos.y - game.shots[homingIdx].pos.y };
        Vector2 v = game.shots[homingIdx].vel;
        float tlen = sqrtf(toEnemy.x*toEnemy.x + toEnemy.y*toEnemy.y);
        float vlen = sqrtf(v.x*v.x + v.y*v.y);
        if (tlen > 0.001f && vlen > 0.001f) initialDot = (toEnemy.x*v.x + toEnemy.y*v.y)/(tlen*vlen);
    }
    bool homed = false;
    if (homingIdx >= 0 && enemyIdx >= 0)
    {
        for (int step = 0; step < 60; step++) CombatUpdateShots(&game, 1.0f/60.0f);   /* 1.0s, sotto la vita di 1.15s del colpo */
        if (game.shots[homingIdx].active)
        {
            Vector2 toEnemy = { game.enemies[enemyIdx].pos.x - game.shots[homingIdx].pos.x,
                                 game.enemies[enemyIdx].pos.y - game.shots[homingIdx].pos.y };
            Vector2 v = game.shots[homingIdx].vel;
            float tlen = sqrtf(toEnemy.x*toEnemy.x + toEnemy.y*toEnemy.y);
            float vlen = sqrtf(v.x*v.x + v.y*v.y);
            float finalDot = (tlen > 0.001f && vlen > 0.001f) ? (toEnemy.x*v.x + toEnemy.y*v.y)/(tlen*vlen) : -2.0f;
            homed = finalDot > 0.6f;   /* da "punta ~150 gradi nella direzione sbagliata" a "punta chiaramente verso il bersaglio" */
            printf("  [A] frammento homing ancora attivo dopo 60 step (1.0s): dot iniziale=%.3f -> finale=%.3f (atteso finale > 0.6)\n",
                   initialDot, finalDot);
        }
        else
        {
            bool enemyHit = game.enemies[enemyIdx].active && game.enemies[enemyIdx].hp < enemyHpBefore;
            homed = enemyHit || !game.enemies[enemyIdx].active;   /* colpito (hp sceso o nemico morto): homing riuscito */
            printf("  [A] frammento non piu' attivo dopo 50 step (probabile impatto): nemico colpito=%s\n", homed ? "si" : "no");
        }
    }
    else printf("  [A] FALLITO: nessun frammento homing con direzione iniziale opposta trovato\n");

    ScriptItemsShutdown(&game);
    bool ok = loaded && noSplitAtFullHp && splitAtLowHp && homingIdx >= 0 && homed;
    if (!ok) printf("      FALLITO: split-and-home condizionato all'hp non si comporta come atteso\n");
    return ok;
}

/* ============================================================
   Test B/C: il sistema delle cache (spec, sezione 7).
   ============================================================ */

static const char *ADD_DAMAGE_2_LUA = "function on_evaluate(stats)\n  stats.damage = stats.damage + 2\nend\n";
static const char *ADD_DAMAGE_3_LUA = "function on_evaluate(stats)\n  stats.damage = stats.damage + 3\nend\n";

static bool TestRecomputeNoDrift(void)
{
    Game game = MakeBaseGame(555u);
    float base = game.player.baseDamage;

    Item item1 = { 0 }; item1.active = true; item1.slot = SLOT_HAT;
    snprintf(item1.name, sizeof(item1.name), "Piu' Due");
    snprintf(item1.luaSource, sizeof(item1.luaSource), "%s", ADD_DAMAGE_2_LUA);
    TestAddItem(&game, item1);

    Item item2 = { 0 }; item2.active = true; item2.slot = SLOT_HAT;
    snprintf(item2.name, sizeof(item2.name), "Piu' Tre");
    snprintf(item2.luaSource, sizeof(item2.luaSource), "%s", ADD_DAMAGE_3_LUA);
    TestAddItem(&game, item2);

    float withBoth = game.player.damage;
    bool bothOk = fabsf(withBoth - (base + 5.0f)) < 1e-4f;
    printf("  [B] due oggetti (+2, +3): damage=%.4f (atteso %.4f)\n", withBoth, base + 5.0f);

    /* "Rimozione": il gioco non ha ancora una funzione per scartare un
       oggetto (fuori scopo di questo task), ma il ricalcolo da zero e'
       esattamente cio' che la rende banale il giorno che arrivera': basta
       escludere l'oggetto dal conteggio e ricalcolare, senza contabilita'
       incrementale da disfare. */
    game.player.itemCount = 1;
    ScriptItemsRecomputeStats(&game);
    float withOne = game.player.damage;
    bool removedOk = fabsf(withOne - (base + 2.0f)) < 1e-4f;
    printf("  [B] rimosso il secondo oggetto: damage=%.4f (atteso %.4f, NESSUNA deriva)\n", withOne, base + 2.0f);

    ScriptItemsShutdown(&game);
    bool ok = bothOk && removedOk;
    if (!ok) printf("      FALLITO: il ricalcolo da zero non corrisponde ai valori attesi\n");
    return ok;
}

static bool TestRecomputeIdempotent(void)
{
    Game game = MakeBaseGame(777u);
    Item item = { 0 }; item.active = true; item.slot = SLOT_HAT;
    snprintf(item.name, sizeof(item.name), "Idempotente");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", ADD_DAMAGE_2_LUA);
    TestAddItem(&game, item);

    float first = game.player.damage;
    bool allSame = true;
    for (int i = 0; i < 100; i++)
    {
        ScriptItemsRecomputeStats(&game);
        if (fabsf(game.player.damage - first) > 1e-6f) { allSame = false; break; }
    }
    printf("  [C] on_evaluate applicato 100 volte: damage sempre %.6f? %s\n", (double)first, allSame ? "si" : "NO");

    ScriptItemsShutdown(&game);
    if (!allSame) printf("      FALLITO: il ricalcolo ripetuto ha prodotto un risultato diverso (deriva)\n");
    return allSame;
}

/* ============================================================
   Test D: 10^6 spawn_shot -> clamp a MAX_SHOTS, mai un blocco.
   ============================================================ */

static const char *SPAWN_MANY_LUA =
    "function on_tick(dt)\n"
    "  local px, py = player_x(), player_y()\n"
    "  local i = 0\n"
    "  while i < 2000000 do\n"
    "    spawn_shot(px, py, 1, 0, 300, 1, 3, 0)\n"
    "    i = i + 1\n"
    "  end\n"
    "end\n";

static bool TestMillionShotsClamped(void)
{
    Game game = MakeBaseGame(2024u);
    Item item = { 0 }; item.active = true; item.slot = SLOT_HAT;
    snprintf(item.name, sizeof(item.name), "Cannone Impossibile");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", SPAWN_MANY_LUA);
    TestAddItem(&game, item);

    double t0 = NowSeconds();
    ScriptItemsOnTick(&game, 1.0f/60.0f);
    double elapsed = NowSeconds() - t0;

    int active = CountActiveShots(&game);
    printf("  [D] script che chiede 2*10^6 spawn_shot -> %d colpi attivi (limite MAX_SHOTS=%d) in %.4fs, sandbox uccisa=%s\n",
           active, MAX_SHOTS, elapsed, ScriptItemsHasActiveLua(&game, 0) ? "no" : "si (atteso: budget di istruzioni)");

    ScriptItemsShutdown(&game);
    bool ok = active <= MAX_SHOTS && elapsed < 3.0;
    if (!ok) printf("      FALLITO: il gioco deve restare entro MAX_SHOTS e rispondere in tempo limitato, mai un crash/blocco\n");
    return ok;
}

/* ============================================================
   Test E: handle invalido -> no-op sul gioco, script ucciso,
   ripiego sulla mini-VM nello STESSO frame.
   ============================================================ */

static const char *BAD_HANDLE_LUA = "function on_fire(x, y, dx, dy)\n  damage_enemy(999999, 10)\nend\n";

static bool TestInvalidHandleKillsAndFallsBack(void)
{
    Game game = MakeBaseGame(31337u);
    Item item = { 0 }; item.active = true; item.slot = SLOT_HAND;
    snprintf(item.name, sizeof(item.name), "Bastone Difettoso");
    snprintf(item.script, sizeof(item.script), "on_fire:burst,2,0.30,none");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", BAD_HANDLE_LUA);
    TestAddItem(&game, item);

    bool loadedOk = ScriptItemsHasActiveLua(&game, 0);   /* sintassi valida: carica bene */

    int before = CountActiveShots(&game);
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });   /* on_fire chiama un handle invalido -> uccisa */
    int createdFirst = CountActiveShots(&game) - before;
    bool killedNow = !ScriptItemsHasActiveLua(&game, 0);

    const ScriptSandbox *sb = (const ScriptSandbox *)game.itemScripts[0].sandbox;
    bool disabled = sb != NULL && ScriptSandboxIsDisabled(sb);
    printf("  [E] caricato=%s ucciso_dopo_1_colpo=%s motivo=\"%s\"\n",
           loadedOk ? "si" : "no", killedNow ? "si" : "no", sb != NULL ? ScriptSandboxDisabledReason(sb) : "?");

    /* Ripiego mini-VM GIA' nello stesso frame: 1 pallino normale + 2 di
       burst = 3 colpi, zero danno reale a chiunque (l'handle invalido non
       ha toccato nulla). */
    bool fallbackFired = createdFirst == 3;
    printf("  [E] colpi creati al primo sparo (con ripiego mini-VM): %d (attesi 3)\n", createdFirst);

    /* Il gioco continua: un secondo sparo funziona ancora (mini-VM, Lua
       resta disabilitata), nessun crash. */
    int before2 = CountActiveShots(&game);
    CombatFirePlayer(&game, (Vector2){ 0.0f, 1.0f });
    int createdSecond = CountActiveShots(&game) - before2;
    bool stillRunning = createdSecond == 3 && game.phase == PHASE_PLAY;
    printf("  [E] secondo sparo dopo l'uccisione: %d colpi creati (attesi 3), gioco ancora in PHASE_PLAY=%s\n",
           createdSecond, game.phase == PHASE_PLAY ? "si" : "no");

    ScriptItemsShutdown(&game);
    bool ok = loadedOk && killedNow && disabled && fallbackFired && stillRunning;
    if (!ok) printf("      FALLITO: handle invalido deve uccidere lo script e far ripiegare sulla mini-VM, senza fermare il gioco\n");
    return ok;
}

/* ============================================================
   Test F: sintassi Lua rotta -> ripiego mini-VM da subito
   (nessun on_fire deve nemmeno tentare di girare).
   ============================================================ */

static bool TestBrokenLuaFallsBack(void)
{
    Game game = MakeBaseGame(2222u);
    Item item = { 0 }; item.active = true; item.slot = SLOT_HAND;
    snprintf(item.name, sizeof(item.name), "Oggetto Rotto");
    snprintf(item.script, sizeof(item.script), "on_fire:burst,2,0.30,none");
    snprintf(item.luaSource, sizeof(item.luaSource), "function broken( ( end");
    TestAddItem(&game, item);

    /* Il fallimento e' di COMPILAZIONE: ScriptItemsOnAcquire lo scopre
       subito (dentro ScriptSandboxLoad), niente aspetta il primo on_fire. */
    bool neverActive = !ScriptItemsHasActiveLua(&game, 0);
    printf("  [F] sintassi invalida -> mai attiva, gia' subito dopo l'acquisizione: %s\n", neverActive ? "si" : "NO");

    int before = CountActiveShots(&game);
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    int created = CountActiveShots(&game) - before;
    bool fallbackFired = created == 3;
    printf("  [F] mini-VM di ripiego attiva dal primo sparo: %d colpi creati (attesi 3)\n", created);

    ScriptItemsShutdown(&game);
    bool ok = neverActive && fallbackFired && game.phase == PHASE_PLAY;
    if (!ok) printf("      FALLITO: uno script Lua rotto deve ripiegare sulla mini-VM da subito, senza crash\n");
    return ok;
}

/* ============================================================
   Test G: determinismo (stesso seed -> stessi colpi/danni,
   byte per byte; seed diverso -> risultato diverso).
   ============================================================ */

static const char *DETERMINISTIC_LUA =
    "function on_evaluate(stats)\n"
    "  stats.damage = stats.damage + 1\n"
    "end\n"
    "function on_fire(x, y, dx, dy)\n"
    "  local jitter = (rng() - 0.5)*0.4\n"
    "  spawn_shot(x, y, dx + jitter, dy, 300, 5, 4, 0)\n"
    "end\n";

static void RunDeterminismScenario(Game *game, unsigned int seed)
{
    *game = MakeBaseGame(seed);
    Item item = { 0 }; item.active = true; item.slot = SLOT_HAND;
    snprintf(item.name, sizeof(item.name), "Item Determinismo");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", DETERMINISTIC_LUA);
    TestAddItem(game, item);
    for (int i = 0; i < 5; i++) CombatFirePlayer(game, (Vector2){ 1.0f, 0.0f });
}

static bool GamesEqualEnough(const Game *a, const Game *b)
{
    if (fabsf(a->player.damage - b->player.damage) > 1e-6f) return false;
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        const Shot *sa = &a->shots[i];
        const Shot *sb = &b->shots[i];
        if (sa->active != sb->active) return false;
        if (!sa->active) continue;
        if (fabsf(sa->pos.x - sb->pos.x) > 1e-6f || fabsf(sa->pos.y - sb->pos.y) > 1e-6f) return false;
        if (fabsf(sa->vel.x - sb->vel.x) > 1e-6f || fabsf(sa->vel.y - sb->vel.y) > 1e-6f) return false;
        if (fabsf(sa->damage - sb->damage) > 1e-6f) return false;
    }
    return true;
}

static bool TestDeterminism(void)
{
    Game gameA, gameB, gameC;
    RunDeterminismScenario(&gameA, 90125u);
    RunDeterminismScenario(&gameB, 90125u);
    bool same = GamesEqualEnough(&gameA, &gameB);
    printf("  [G] stesso seed, due Game separati nello stesso processo -> %s\n", same ? "risultati identici" : "DIVERSI");

    RunDeterminismScenario(&gameC, 90126u);
    bool differs = !GamesEqualEnough(&gameA, &gameC);
    printf("  [G] seed diverso -> risultati %s (atteso: diversi)\n", differs ? "diversi" : "UGUALI");

    ScriptItemsShutdown(&gameA);
    ScriptItemsShutdown(&gameB);
    ScriptItemsShutdown(&gameC);
    bool ok = same && differs;
    if (!ok) printf("      FALLITO: stesso seed deve produrre lo stesso risultato, seed diverso un risultato diverso\n");
    return ok;
}

/* ============================================================
   Test H: costo reale delle callback con una stanza piena
   (spec, criterio di successo 6: sotto l'1%% di un frame a 60 FPS).
   ============================================================ */

static const char *PERF_LUA =
    "function on_fire(x, y, dx, dy)\n"
    "  local id = nearest_enemy(x, y)\n"
    "  if id ~= nil then\n"
    "    local ex, ey = enemy_x(id), enemy_y(id)\n"
    "    local d = (ex - x)*(ex - x) + (ey - y)*(ey - y)\n"
    "  end\n"
    "end\n"
    "function on_hit(shot_id, enemy_id)\n"
    "  local hp = enemy_hp(enemy_id)\n"
    "  if hp ~= nil then\n"
    "    local dummy = hp*0.5\n"
    "  end\n"
    "end\n"
    "function on_tick(dt)\n"
    "  local hp = player_hp()\n"
    "  local dummy = hp + dt\n"
    "end\n";

#define SCRIPT_ITEMS_PERF_ITEM_COUNT   4
#define SCRIPT_ITEMS_PERF_ENEMY_COUNT  60
#define SCRIPT_ITEMS_PERF_FRAMES       300

static bool TestPerformance(void)
{
    Game game = MakeBaseGame(1234u);
    for (int i = 0; i < SCRIPT_ITEMS_PERF_ENEMY_COUNT; i++)
    {
        EntitiesAddEnemy(&game, ENEMY_CHASER,
            (Vector2){ ROOM_X + 30.0f + (float)(i%20)*30.0f, ROOM_Y + 30.0f + (float)(i/20)*40.0f });
    }
    Shot *sampleShot = EntitiesAddShot(&game, true, game.player.pos, (Vector2){ 1.0f, 0.0f }, 300.0f, 5.0f, 5.0f, 0, WHITE);
    int shotIndex = sampleShot != NULL ? (int)(sampleShot - game.shots) : 0;

    for (int k = 0; k < SCRIPT_ITEMS_PERF_ITEM_COUNT; k++)
    {
        Item item = { 0 }; item.active = true; item.slot = SLOT_HAT;
        snprintf(item.name, sizeof(item.name), "PerfItem%d", k);
        snprintf(item.luaSource, sizeof(item.luaSource), "%s", PERF_LUA);
        TestAddItem(&game, item);
    }
    bool allLoaded = true;
    for (int k = 0; k < SCRIPT_ITEMS_PERF_ITEM_COUNT; k++) if (!ScriptItemsHasActiveLua(&game, k)) allLoaded = false;
    printf("  [H] %d oggetti Lua caricati correttamente: %s\n", SCRIPT_ITEMS_PERF_ITEM_COUNT, allLoaded ? "si" : "NO");

    double t0 = NowSeconds();
    for (int f = 0; f < SCRIPT_ITEMS_PERF_FRAMES; f++)
    {
        ScriptItemsOnFire(&game, game.player.pos, (Vector2){ 1.0f, 0.0f });
        ScriptItemsOnTick(&game, 1.0f/60.0f);
        for (int e = 0; e < SCRIPT_ITEMS_PERF_ENEMY_COUNT; e++) ScriptItemsOnHit(&game, shotIndex, e);
    }
    double elapsed = NowSeconds() - t0;
    double perFrameUs = (elapsed/(double)SCRIPT_ITEMS_PERF_FRAMES)*1e6;
    double budgetUs = (1000.0/60.0)*1000.0*0.01;   /* 1% di 16.667ms, in microsecondi */
    int callsPerFrame = SCRIPT_ITEMS_PERF_ITEM_COUNT*(1 + 1 + SCRIPT_ITEMS_PERF_ENEMY_COUNT);

    printf("  [H] %d oggetti Lua x (on_fire+on_tick+%d on_hit) = %d callback/frame simulato, %d frame -> %.2f us/frame (budget 1%% di 16.667ms = %.2f us)\n",
           SCRIPT_ITEMS_PERF_ITEM_COUNT, SCRIPT_ITEMS_PERF_ENEMY_COUNT, callsPerFrame, SCRIPT_ITEMS_PERF_FRAMES, perFrameUs, budgetUs);

    bool noneKilled = true;
    for (int k = 0; k < SCRIPT_ITEMS_PERF_ITEM_COUNT; k++) if (!ScriptItemsHasActiveLua(&game, k)) noneKilled = false;

    ScriptItemsShutdown(&game);
    bool ok = allLoaded && noneKilled && perFrameUs < budgetUs;
    if (!ok) printf("      FALLITO (o oltre budget): perFrameUs=%.2f budgetUs=%.2f allLoaded=%d noneKilled=%d\n",
                     perFrameUs, budgetUs, allLoaded, noneKilled);
    return ok;
}

bool ScriptItemsSelfTest(void)
{
    struct { const char *label; bool (*fn)(void); } tests[] = {
        { "A (headline: ogni 3 colpi sdoppia e insegue, solo con hp<3)", TestHeadlineSplitHoming },
        { "B (ricalcolo da zero: rimuovere un oggetto non lascia deriva)", TestRecomputeNoDrift },
        { "C (idempotenza: 100 ricalcoli, sempre lo stesso risultato)", TestRecomputeIdempotent },
        { "D (10^6 spawn_shot: clamp a MAX_SHOTS, mai un blocco)", TestMillionShotsClamped },
        { "E (handle invalido: uccisa + ripiego mini-VM nello stesso frame)", TestInvalidHandleKillsAndFallsBack },
        { "F (Lua rotto: ripiego mini-VM da subito)", TestBrokenLuaFallsBack },
        { "G (determinismo: stesso seed -> stesso risultato)", TestDeterminism },
        { "H (prestazioni: costo reale delle callback, stanza piena)", TestPerformance },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        printf("-- test %s --\n", tests[i].label);
        if (!tests[i].fn()) allOk = false;
    }
    return allOk;
}

#else /* _WIN32: vedi lo stesso commento in script_sandbox_tests.c */

bool ScriptItemsSelfTest(void)
{
    return true;
}

#endif
