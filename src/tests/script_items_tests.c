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

#include "content/run_content.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "gameplay/synergies.h"
#include "script/script_items.h"
#include "script/script_sandbox.h"

#include <limits.h>
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
   Test I/J/K (fase 3, docs/superpowers/specs/2026-07-13-items-synergy-vision.md
   sezioni 1,2,5 + il task brief "items synergy vision"): oggetti stat-up
   (ITEM_STATUP), il budget di potenza per-oggetto e il ripiego C "mai un
   dud". Stesso pattern degli altri test qui sopra: Game locale, TestAddItem
   riproduce il pickup vero.
   ============================================================ */

/* Test I: on_evaluate ambizioso (+1000 danno) -> clampato al budget
   per-oggetto, MAI ai 1000 richiesti. E' il test esplicito del task brief:
   "feed a script bumping damage by +1000, assert the player's damage lands
   at the per-item cap, not 1000". Rarita' esplicita RARITY_UNCOMMON: e' la
   riga della tavola SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION (src/script/
   script_items.c) rimasta 0.25, lo stesso valore del vecchio tetto flat
   pre-fase-3b, cosi' questo test isola "il tetto per-oggetto esiste ed e'
   rispettato" dalla scalatura per rarita' (quella e' il test M sotto). */
static const char *GREEDY_DAMAGE_LUA =
    "function on_evaluate(stats)\n"
    "  stats.damage = stats.damage + 1000\n"
    "end\n";

static bool TestStatUpClampedToPerItemCap(void)
{
    Game game = MakeBaseGame(9001u);
    float base = game.player.baseDamage;
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_STATUP;
    item.rarity = RARITY_UNCOMMON;
    item.slot = SLOT_HAT;
    snprintf(item.name, sizeof(item.name), "Nucleo Ingordo");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", GREEDY_DAMAGE_LUA);
    TestAddItem(&game, item);

    float perItemCap = base*0.25f;   /* deve combaciare con la riga RARITY_UNCOMMON della tavola */
    float expected = base + perItemCap;
    float got = game.player.damage;
    bool ok = fabsf(got - expected) < 1e-3f;
    printf("  [I] on_evaluate chiede +1000 danno (non comune) -> damage=%.4f (atteso %.4f = base %.1f + tetto per-oggetto %.2f, MAI base+1000)\n",
           got, expected, base, perItemCap);
    ScriptItemsShutdown(&game);
    if (!ok) printf("      FALLITO: il tetto di potenza per-oggetto (fase 3) non ha limitato uno script avido\n");
    return ok;
}

/* Test M (fase 3b, design doc sezione 2 + il task brief: "a LEGENDARY
   stat-up pushing +1000 damage lands higher than a COMMON one pushing
   +1000, proving the per-rarity cap differs"): stesso script avido di
   sopra, due Game separati, un oggetto COMUNE e uno LEGGENDARIO. Numeri
   concreti (base=8, tavola in src/script/script_items.c): comune ->
   8 + 0.15*8 = 9.2, leggendario -> 8 + 0.60*8 = 12.8. Entrambi ben dentro
   la banda globale [SCRIPT_ITEMS_DAMAGE_MIN, SCRIPT_ITEMS_DAMAGE_MAX] =
   [0.5, 200]. */
static bool TestRarityCapDiffersFromCommonToLegendary(void)
{
    Game commonGame = MakeBaseGame(5001u);
    float base = commonGame.player.baseDamage;
    Item commonItem = { 0 };
    commonItem.active = true;
    commonItem.kind = ITEM_STATUP;
    commonItem.rarity = RARITY_COMMON;
    commonItem.slot = SLOT_HAT;
    snprintf(commonItem.name, sizeof(commonItem.name), "Nucleo Comune");
    snprintf(commonItem.luaSource, sizeof(commonItem.luaSource), "%s", GREEDY_DAMAGE_LUA);
    TestAddItem(&commonGame, commonItem);
    float commonDamage = commonGame.player.damage;
    float expectedCommon = base + base*0.15f;

    Game legendaryGame = MakeBaseGame(5002u);
    Item legendaryItem = { 0 };
    legendaryItem.active = true;
    legendaryItem.kind = ITEM_STATUP;
    legendaryItem.rarity = RARITY_LEGENDARY;
    legendaryItem.slot = SLOT_HAT;
    snprintf(legendaryItem.name, sizeof(legendaryItem.name), "Nucleo Leggendario");
    snprintf(legendaryItem.luaSource, sizeof(legendaryItem.luaSource), "%s", GREEDY_DAMAGE_LUA);
    TestAddItem(&legendaryGame, legendaryItem);
    float legendaryDamage = legendaryGame.player.damage;
    float expectedLegendary = base + base*0.60f;

    bool commonOk = fabsf(commonDamage - expectedCommon) < 1e-3f;
    bool legendaryOk = fabsf(legendaryDamage - expectedLegendary) < 1e-3f;
    bool legendaryHigher = legendaryDamage > commonDamage;
    bool bothInBand = commonDamage >= 0.5f && commonDamage <= 200.0f && legendaryDamage >= 0.5f && legendaryDamage <= 200.0f;

    printf("  [M] on_evaluate chiede +1000 danno -> comune=%.4f (atteso %.4f), leggendario=%.4f (atteso %.4f); leggendario>comune=%s, entrambi in banda [0.5,200]=%s\n",
           commonDamage, expectedCommon, legendaryDamage, expectedLegendary,
           legendaryHigher ? "si" : "NO", bothInBand ? "si" : "NO");

    ScriptItemsShutdown(&commonGame);
    ScriptItemsShutdown(&legendaryGame);
    bool ok = commonOk && legendaryOk && legendaryHigher && bothInBand;
    if (!ok) printf("      FALLITO: il tetto per-oggetto deve scalare per rarita' (comune < leggendario), entrambi dentro banda\n");
    return ok;
}

/* Test N (fase 3b, design doc sezione 2 + il task brief: "five legendaries
   stacked keep the player inside the band, not unplayable"): cinque
   oggetti stat-up LEGGENDARI, ciascuno con lo stesso script avido
   (+1000 danno). Il tetto per-oggetto e' relativo a player.baseDamage
   (FISSO, non al valore corrente prima di questo oggetto, vedi il
   commento su ScriptItemsClampItemDelta): lo spostamento massimo per
   oggetto resta 0.60*8=4.8 anche per il quinto oggetto, quindi la somma
   GREZZA e' 8 + 5*4.8 = 32.0.
   STEP C (curve alla Isaac): quella somma grezza non e' piu' il danno finale.
   ScriptItemsDamageCurve la comprime sopra il ginocchio (2*base = 16):
   16*sqrt(32/16) = 22.63. Il valore atteso qui sotto e' quindi calcolato con la
   STESSA formula invece che scritto a mano -- il test continua a verificare cio'
   che ha sempre verificato (il tetto per-oggetto regge, cinque leggendari
   restano dentro la banda globale [0.5,200], il giocatore resta giocabile) senza
   diventare un doppione del test W, che e' quello che verifica la curva in se'
   (compressione + monotonia). */
static bool TestFiveLegendariesStayInBand(void)
{
    Game game = MakeBaseGame(5005u);
    float base = game.player.baseDamage;
    for (int i = 0; i < 5; i++)
    {
        Item item = { 0 };
        item.active = true;
        item.kind = ITEM_STATUP;
        item.rarity = RARITY_LEGENDARY;
        item.slot = SLOT_HAT;
        snprintf(item.name, sizeof(item.name), "Nucleo Leggendario %d", i + 1);
        snprintf(item.luaSource, sizeof(item.luaSource), "%s", GREEDY_DAMAGE_LUA);
        TestAddItem(&game, item);
    }
    float raw = base + 5.0f*(base*0.60f);   /* la somma grezza dei cinque tetti per-oggetto: 32.0 */
    float knee = 2.0f*base;
    float expected = raw > knee ? knee*sqrtf(raw/knee) : raw;
    float got = game.player.damage;
    bool matchesExpected = fabsf(got - expected) < 1e-2f;
    bool inBand = got >= 0.5f && got <= 200.0f;
    printf("  [N] cinque oggetti stat-up leggendari (+1000 danno ciascuno) -> damage=%.4f (atteso %.4f = somma grezza %.1f compressa dalla curva), dentro banda [0.5,200]: %s\n",
           got, expected, raw, inBand ? "si" : "NO");
    ScriptItemsShutdown(&game);
    bool ok = matchesExpected && inBand;
    if (!ok) printf("      FALLITO: cinque leggendari devono restare dentro la banda globale (mai ingiocabile)\n");
    return ok;
}

/* Test O (fase 3b, design doc sezione 4: "il costo del negozio scala con
   la rarita': un leggendario costa piu' monete di un comune"). Non
   esercita world.c/combat.c (nessun Game/stanza necessari: e' una tavola
   pura, src/gameplay/item_traits.c), ma vive qui insieme al resto della
   suite di bilanciamento della rarita' (task brief, sezione 5: "extend
   scripts/test-gen.sh AND the --script-items-test suite"). Verifica sia
   l'ordine stretto (comune < non-comune < raro < leggendario) sia il
   valore storico invariato per il comune (8, lo stesso letterale hardcoded
   prima di questa fase in world.c). */
static bool TestShopCostScalesWithRarity(void)
{
    int common = ItemShopCostForRarity(RARITY_COMMON);
    int uncommon = ItemShopCostForRarity(RARITY_UNCOMMON);
    int rare = ItemShopCostForRarity(RARITY_RARE);
    int legendary = ItemShopCostForRarity(RARITY_LEGENDARY);
    printf("  [O] costo negozio per rarita': comune=%d non-comune=%d raro=%d leggendario=%d\n",
           common, uncommon, rare, legendary);
    bool strictlyIncreasing = common < uncommon && uncommon < rare && rare < legendary;
    bool commonUnchanged = common == 8;
    bool ok = strictlyIncreasing && commonUnchanged;
    if (!ok) printf("      FALLITO: il costo deve crescere strettamente con la rarita', comune deve restare 8\n");
    return ok;
}

/* Test P (fase 3b review, "lock the rarity enum/text sync"): GEN_RARITIES
   (tools/melting-gen/gen_util.c, lato generatore), l'enum Rarity
   (core/game_types.h, lato gioco) e RarityFromText (run_content.c, il
   parser che li mette in comunicazione) vanno tenuti sincronizzati A MANO,
   stesso ordine e stessi quattro testi letterali. Un mismatch silenzioso
   (es. "uncommon" -> RARITY_RARE) passerebbe ogni altro test di oggi senza
   che nessuno se ne accorga: qui si verifica che ciascuno dei 4 testi
   CANONICI (identici a GEN_RARITIES: se li cambi la', cambiali anche qui)
   mappi sul livello atteso e che i quattro livelli restino DISTINTI. */
static bool TestRarityTextRoundTrip(void)
{
    static const char *kTexts[4]    = { "common", "uncommon", "rare", "legendary" };
    static const Rarity kExpected[4] = { RARITY_COMMON, RARITY_UNCOMMON, RARITY_RARE, RARITY_LEGENDARY };
    bool ok = true;
    for (int i = 0; i < 4; i++)
    {
        Rarity got = RarityFromText(kTexts[i]);
        if (got != kExpected[i])
        {
            printf("      FALLITO: RarityFromText(\"%s\") = %d, atteso %d\n", kTexts[i], (int)got, (int)kExpected[i]);
            ok = false;
        }
        for (int j = 0; j < i; j++)
        {
            if (kExpected[j] == kExpected[i]) { printf("      FALLITO: due testi sullo stesso livello\n"); ok = false; }
        }
    }
    return ok;
}

/* Test Q (fase 3b review, "il boss non delude mai", decisione del
   proprietario): il ripiego DETERMINISTICO puro (RunContentLoad quando
   generated/current_run.txt non esiste ancora, MakeFallbackBossItem in
   run_content.c) deve dare comunque un bossItem raro o leggendario per
   ciascuno dei 5 piani, mai comune -- lo stesso principio che gia' vale per
   il manifest VERO generato da melting-gen (GEN_RARITY_WEIGHTS_BOSS, vedi
   scripts/test-gen.sh), esteso qui al caso degenere "nessun manifest
   ancora". Sposta via il manifest PRIMA di RunContentLoad (rename, non
   remove: un giro precedente di make test-gen puo' averne lasciato uno vero
   in generated/, e questo test non e' la sede giusta per distruggerlo) per
   essere sicuri di esercitare il ramo di ripiego puro, poi lo rimette al
   suo posto subito dopo, che il test passi o fallisca. */
static bool TestFallbackBossItemIsRare(void)
{
    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.rarity-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    RunContent content = { 0 };
    RunContentLoad(&content, 777u);

    bool ok = true;
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        Rarity r = content.floors[f].bossItem.rarity;
        if (r != RARITY_RARE && r != RARITY_LEGENDARY)
        {
            printf("      FALLITO: floor %d bossItem.rarity = %d, atteso raro o leggendario (ripiego puro, mai comune)\n", f, (int)r);
            ok = false;
        }
    }

    if (hadManifest) rename(kBackup, kManifest);
    return ok;
}

/* ============================================================
   Test R-W (step C, docs/superpowers/specs/2026-07-14-step-c-shottype-balance.md):
   i tipi di colpo INVENTATI DAL MODELLO e le curve alla Isaac. Il punto di
   questi test e' che il motore non ha un elenco di tipi di colpo da verificare
   (non esiste: li inventa il modello a ogni run) -- si verifica quindi la
   PROMESSA che il motore fa al modello: "scrivi quello che vuoi, io garantisco
   che sia bilanciato, che si comporti come dichiarato, e che non rompa nulla".
   ============================================================ */

/* Un tipo di colpo costruito a mano (come lo costruirebbe il parser dal
   manifest, ma senza toccare il disco). Volutamente NON passa da
   ShotTypeBalance: sono i test a decidere quando applicarlo. */
static ShotTypeDef MakeShotType(ShotForm form, float speed, float damage, float radius, float life,
                                int pierce, int chain, int pellets)
{
    ShotTypeDef type;
    memset(&type, 0, sizeof(type));
    type.active = true;
    snprintf(type.name, sizeof(type.name), "Prova");
    type.form = form;
    type.speedMul = speed;
    type.damageMul = damage;
    type.radiusMul = radius;
    type.lifeMul = life;
    type.pierceBonus = pierce;
    type.chain = chain;
    type.pellets = pellets;
    return type;
}

/* Test R (il test centrale di questa fase): QUALUNQUE cosa il modello inventi,
   ShotTypeBalance la riporta dentro la banda di potenza. Si esplora l'intero
   spazio delle manopole ai suoi estremi (2^4 combinazioni dei quattro
   moltiplicatori al minimo/massimo x 4 valori di pierce x 4 di chain x 3 di
   pellets = 768 tipi), piu' i due casi patologici espliciti -- il "dud" (tutto
   al minimo: deve essere RINFORZATO, altrimenti sarebbe un tipo di colpo che
   peggiora la run) e il "rotto" (tutto al massimo: deve essere TAGLIATO). Se
   questo test passa, nessun tipo di colpo scritto da un 7B puo' rompere il
   gioco, e non serve fidarsi del modello su nulla. */
static bool TestShotTypeAlwaysBalanced(void)
{
    static const float kSpeeds[2]  = { SHOT_TYPE_SPEED_MIN,  SHOT_TYPE_SPEED_MAX };
    static const float kDamages[2] = { SHOT_TYPE_DAMAGE_MIN, SHOT_TYPE_DAMAGE_MAX };
    static const float kRadii[2]   = { SHOT_TYPE_RADIUS_MIN, SHOT_TYPE_RADIUS_MAX };
    static const float kLives[2]   = { SHOT_TYPE_LIFE_MIN,   SHOT_TYPE_LIFE_MAX };

    bool ok = true;
    int checked = 0;
    float worstLow = 999.0f, worstHigh = 0.0f;

    for (int s = 0; s < 2; s++)
    for (int d = 0; d < 2; d++)
    for (int r = 0; r < 2; r++)
    for (int l = 0; l < 2; l++)
    for (int pierce = 0; pierce <= SHOT_TYPE_PIERCE_MAX; pierce++)
    for (int chain = 0; chain <= SHOT_TYPE_CHAIN_MAX; chain++)
    for (int pellets = 1; pellets <= SHOT_TYPE_PELLETS_MAX; pellets++)
    {
        ShotTypeDef type = MakeShotType(SHOT_FORM_SPIKE, kSpeeds[s], kDamages[d], kRadii[r], kLives[l], pierce, chain, pellets);
        ShotTypeBalance(&type);
        float power = ShotTypePower(&type);
        checked++;
        if (power < worstLow) worstLow = power;
        if (power > worstHigh) worstHigh = power;
        if (power < SHOT_TYPE_POWER_MIN || power > SHOT_TYPE_POWER_MAX) ok = false;

        /* Idempotenza: ribilanciare un tipo gia' bilanciato non lo cambia (il
           gioco lo fa DAVVERO tre volte -- melting-gen, run_content, recompute --
           quindi non e' un dettaglio teorico). */
        ShotTypeDef again = type;
        ShotTypeBalance(&again);
        if (fabsf(ShotTypePower(&again) - power) > 1e-4f) ok = false;
    }

    ShotTypeDef dud = MakeShotType(SHOT_FORM_ORB, SHOT_TYPE_SPEED_MIN, SHOT_TYPE_DAMAGE_MIN, SHOT_TYPE_RADIUS_MIN, SHOT_TYPE_LIFE_MIN, 0, 0, 1);
    float dudBefore = ShotTypePower(&dud);
    ShotTypeBalance(&dud);
    float dudAfter = ShotTypePower(&dud);

    ShotTypeDef broken = MakeShotType(SHOT_FORM_BLADE, SHOT_TYPE_SPEED_MAX, SHOT_TYPE_DAMAGE_MAX, SHOT_TYPE_RADIUS_MAX, SHOT_TYPE_LIFE_MAX,
                                      SHOT_TYPE_PIERCE_MAX, SHOT_TYPE_CHAIN_MAX, SHOT_TYPE_PELLETS_MAX);
    float brokenBefore = ShotTypePower(&broken);
    ShotTypeBalance(&broken);
    float brokenAfter = ShotTypePower(&broken);

    printf("  [R] %d tipi di colpo estremi bilanciati: potere in [%.3f, %.3f] (banda ammessa [%.2f, %.2f])\n",
           checked, (double)worstLow, (double)worstHigh, (double)SHOT_TYPE_POWER_MIN, (double)SHOT_TYPE_POWER_MAX);
    printf("  [R] dud (tutto al minimo): potere %.3f -> %.3f (rinforzato); rotto (tutto al massimo): %.3f -> %.3f (tagliato)\n",
           (double)dudBefore, (double)dudAfter, (double)brokenBefore, (double)brokenAfter);

    bool dudFixed = dudAfter > dudBefore && dudAfter >= SHOT_TYPE_POWER_MIN && dudAfter <= SHOT_TYPE_POWER_MAX;
    bool brokenFixed = brokenAfter < brokenBefore && brokenAfter <= SHOT_TYPE_POWER_MAX && brokenAfter >= SHOT_TYPE_POWER_MIN;
    ok = ok && dudFixed && brokenFixed;
    if (!ok) printf("      FALLITO: un tipo di colpo inventato dal modello e' finito fuori banda (o il ribilanciamento non e' idempotente)\n");
    return ok;
}

/* Test S: round-trip testo<->enum delle forme, come il test P per la rarita'.
   Stessa ragione: i testi ("orb", "spike", ...) attraversano tre confini
   diversi -- la grammatica GBNF che il modello segue (run.gbnf), il manifest di
   testo (gen_manifest.c) e il parser del gioco (run_content.c) -- e un
   disallineamento silenzioso (es. "beam" letto come SHOT_FORM_ORB) non farebbe
   fallire nessun altro test: i colpi si disegnerebbero solo... male. */
static bool TestShotFormTextRoundTrip(void)
{
    static const char *kTexts[SHOT_FORM_COUNT] = { "orb", "spike", "beam", "arc", "blade" };
    bool ok = true;
    for (int i = 0; i < (int)SHOT_FORM_COUNT; i++)
    {
        ShotForm got = ShotFormFromText(kTexts[i]);
        const char *back = ShotFormName((ShotForm)i);
        if (got != (ShotForm)i || strcmp(back, kTexts[i]) != 0)
        {
            printf("      FALLITO: forma \"%s\": ShotFormFromText=%d (atteso %d), ShotFormName=\"%s\"\n",
                   kTexts[i], (int)got, i, back);
            ok = false;
        }
    }
    /* Un testo sconosciuto NON deve mai produrre una forma esotica. */
    if (ShotFormFromText("chiodo-arcobaleno") != SHOT_FORM_ORB || ShotFormFromText(NULL) != SHOT_FORM_ORB)
    {
        printf("      FALLITO: un testo sconosciuto/NULL deve ricadere su SHOT_FORM_ORB\n");
        ok = false;
    }
    printf("  [S] %d forme: testo <-> enum coerenti in entrambe le direzioni, ignoto -> orb\n", (int)SHOT_FORM_COUNT);
    return ok;
}

/* Test T (il sistema delle cache applicato ai tipi di colpo): raccogliere un
   secondo oggetto con un tipo di colpo SOSTITUISCE il primo (alla Isaac:
   l'ultima "tear replacement" vince, non si sommano), e "rimuovere" quel
   secondo oggetto fa tornare il PRIMO senza alcuna deriva -- esattamente come
   per le statistiche (test B). E' la prova che il tipo di colpo e' ricalcolato
   da zero e non accumulato. */
static bool TestShotTypeLastItemWinsAndReverts(void)
{
    Game game = MakeBaseGame(6001u);

    Item nails = { 0 };
    nails.active = true;
    nails.slot = SLOT_HAND;
    snprintf(nails.name, sizeof(nails.name), "Guanto di Chiodi");
    nails.shotType = MakeShotType(SHOT_FORM_SPIKE, 1.45f, 0.75f, 0.65f, 1.0f, 1, 0, 1);
    TestAddItem(&game, nails);
    bool firstOk = game.player.shotType.active && game.player.shotType.form == SHOT_FORM_SPIKE;

    Item arc = { 0 };
    arc.active = true;
    arc.slot = SLOT_AURA;
    snprintf(arc.name, sizeof(arc.name), "Aura Scarica");
    arc.shotType = MakeShotType(SHOT_FORM_ARC, 0.8f, 0.9f, 1.2f, 0.9f, 0, 2, 1);
    TestAddItem(&game, arc);
    bool secondWins = game.player.shotType.form == SHOT_FORM_ARC && game.player.shotType.chain == 2;

    /* "Rimozione": come nel test B, il gioco non ha ancora una funzione per
       scartare un oggetto -- il ricalcolo da zero e' esattamente cio' che la
       rende banale il giorno che arrivera'. */
    game.player.itemCount = 1;
    ScriptItemsRecomputeStats(&game);
    bool reverted = game.player.shotType.form == SHOT_FORM_SPIKE && game.player.shotType.pierceBonus == 1;

    /* E senza alcun oggetto: nessun tipo di colpo, il colpo base di sempre. */
    game.player.itemCount = 0;
    ScriptItemsRecomputeStats(&game);
    bool cleared = !game.player.shotType.active;

    printf("  [T] primo tipo attivo=%s, il secondo vince=%s, rimosso il secondo torna il primo=%s, senza oggetti nessun tipo=%s\n",
           firstOk ? "si" : "NO", secondWins ? "si" : "NO", reverted ? "si" : "NO", cleared ? "si" : "NO");

    ScriptItemsShutdown(&game);
    bool ok = firstOk && secondWins && reverted && cleared;
    if (!ok) printf("      FALLITO: il tipo di colpo non e' ricalcolato da zero (vince l'ultimo, la rimozione ripristina il precedente)\n");
    return ok;
}

/* Test U: la manopola 'chain' non e' decorativa -- all'impatto nasce DAVVERO un
   colpo verso un secondo nemico vicino. Due nemici a portata di catena, si
   spara sul primo, si aggiornano i colpi, e si verifica che ne compaia uno
   nuovo che si muove verso il secondo. Verifica anche che la catena NON colpisca
   di nuovo lo stesso nemico (sarebbe danno doppio travestito da catena). */
static bool TestShotTypeChainJumpsToSecondEnemy(void)
{
    Game game = MakeBaseGame(6002u);

    Item arcItem = { 0 };
    arcItem.active = true;
    arcItem.slot = SLOT_HAND;
    snprintf(arcItem.name, sizeof(arcItem.name), "Bobina Saltellante");
    arcItem.shotType = MakeShotType(SHOT_FORM_ARC, 1.0f, 1.0f, 1.0f, 1.0f, 0, 2, 1);
    TestAddItem(&game, arcItem);

    /* Primo nemico davanti al giocatore, secondo poco piu' in la' ma dentro la
       portata della catena (220 px, vedi COMBAT_CHAIN_RANGE in combat.c). */
    Vector2 p = game.player.pos;
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 90.0f, p.y });
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 90.0f, p.y - 120.0f });

    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    int shotsAfterFire = CountActiveShots(&game);

    /* Abbastanza frame perche' il colpo raggiunga il primo nemico. */
    bool chained = false;
    for (int step = 0; step < 40 && !chained; step++)
    {
        CombatUpdateShots(&game, 1.0f/60.0f);
        for (int i = 0; i < MAX_SHOTS; i++)
        {
            const Shot *s = &game.shots[i];
            /* Il colpo di catena si riconosce cosi': va VERSO L'ALTO (il secondo
               nemico e' sopra), mentre quello sparato dal giocatore andava a
               destra. Nessun altro meccanismo di questo test crea colpi. */
            if (s->active && s->fromPlayer && s->vel.y < -50.0f) chained = true;
        }
    }

    bool secondEnemyDamaged = false;
    for (int step = 0; step < 60; step++) CombatUpdateShots(&game, 1.0f/60.0f);
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game.enemies[i];
        if (e->active && e->pos.y < p.y - 100.0f && e->hp < e->maxHp) secondEnemyDamaged = true;
    }

    printf("  [U] catena: %d colpo sparato, colpo di catena verso il secondo nemico=%s, secondo nemico danneggiato=%s\n",
           shotsAfterFire, chained ? "si" : "NO", secondEnemyDamaged ? "si" : "NO");

    ScriptItemsShutdown(&game);
    bool ok = chained && secondEnemyDamaged;
    if (!ok) printf("      FALLITO: 'chain' deve creare un colpo verso un ALTRO nemico all'impatto, che poi lo colpisce davvero\n");
    return ok;
}

/* Test V: la manopola 'pierce' fa sopravvivere il colpo al primo nemico (senza,
   il colpo muore all'impatto: vedi CombatUpdateShots). Due nemici in fila sulla
   traiettoria; un solo colpo deve danneggiarli entrambi. */
static bool TestShotTypePierceSurvivesFirstEnemy(void)
{
    Game game = MakeBaseGame(6003u);

    Item spike = { 0 };
    spike.active = true;
    spike.slot = SLOT_HAND;
    snprintf(spike.name, sizeof(spike.name), "Dardo Passante");
    spike.shotType = MakeShotType(SHOT_FORM_SPIKE, 1.0f, 1.0f, 1.0f, 1.0f, 2, 0, 1);
    TestAddItem(&game, spike);

    Vector2 p = game.player.pos;
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 80.0f, p.y });
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 190.0f, p.y });

    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    for (int step = 0; step < 60; step++) CombatUpdateShots(&game, 1.0f/60.0f);

    int damaged = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game.enemies[i];
        if (e->active && e->hp < e->maxHp) damaged++;
        else if (!e->active && e->maxHp > 0.0f) damaged++;   /* morto: danneggiato eccome */
    }
    printf("  [V] perforazione: nemici in fila danneggiati da UN solo colpo = %d (attesi 2)\n", damaged);

    ScriptItemsShutdown(&game);
    bool ok = damaged >= 2;
    if (!ok) printf("      FALLITO: 'pierce' deve far attraversare il primo nemico e colpire il secondo\n");
    return ok;
}

/* Test W (curve alla Isaac, step C): la curva dei rendimenti decrescenti sul
   danno (ScriptItemsDamageCurve, src/script/script_items.c) deve
   (1) NON toccare la zona normale di gioco -- due oggetti da +2/+3 su base 8
       danno ancora esattamente 13, come prima di questa fase (e' il test B, qui
       si verifica esplicitamente il confine);
   (2) comprimere l'impilamento estremo -- cinque leggendari avidi (che sommati
       darebbero 32) restano sotto 32;
   (3) restare MONOTONA -- cinque oggetti fanno comunque piu' danno di quattro:
       un oggetto in piu' non deve mai far male, deve solo rendere meno.
   Senza (3) la curva sarebbe una punizione, non un bilanciamento. */
static bool TestDamageCurveDiminishesButNeverHurts(void)
{
    Game linear = MakeBaseGame(6004u);
    float base = linear.player.baseDamage;

    Item plus2 = { 0 }; plus2.active = true; plus2.slot = SLOT_HAT;
    snprintf(plus2.name, sizeof(plus2.name), "Piu' Due");
    snprintf(plus2.luaSource, sizeof(plus2.luaSource), "%s", ADD_DAMAGE_2_LUA);
    TestAddItem(&linear, plus2);
    Item plus3 = { 0 }; plus3.active = true; plus3.slot = SLOT_HAT;
    snprintf(plus3.name, sizeof(plus3.name), "Piu' Tre");
    snprintf(plus3.luaSource, sizeof(plus3.luaSource), "%s", ADD_DAMAGE_3_LUA);
    TestAddItem(&linear, plus3);
    float normalZone = linear.player.damage;
    bool normalUntouched = fabsf(normalZone - (base + 5.0f)) < 1e-4f;
    ScriptItemsShutdown(&linear);

    /* Quattro e cinque leggendari avidi: la somma grezza sarebbe 8+4*4.8=27.2 e
       8+5*4.8=32, entrambe sopra il ginocchio della curva (2*8=16). */
    float damageWith[6] = { 0 };
    for (int count = 4; count <= 5; count++)
    {
        Game game = MakeBaseGame(6005u + (unsigned int)count);
        for (int i = 0; i < count; i++)
        {
            Item item = { 0 };
            item.active = true;
            item.kind = ITEM_STATUP;
            item.rarity = RARITY_LEGENDARY;
            item.slot = SLOT_HAT;
            snprintf(item.name, sizeof(item.name), "Nucleo %d", i + 1);
            snprintf(item.luaSource, sizeof(item.luaSource), "%s", GREEDY_DAMAGE_LUA);
            TestAddItem(&game, item);
        }
        damageWith[count] = game.player.damage;
        ScriptItemsShutdown(&game);
    }

    bool compressed = damageWith[5] < 32.0f;                 /* la somma grezza sarebbe 32 */
    bool monotonic = damageWith[5] > damageWith[4];          /* ma il quinto oggetto aggiunge comunque qualcosa */
    printf("  [W] zona normale (+2,+3): %.2f (atteso %.2f, INTATTA) | 4 leggendari: %.2f | 5 leggendari: %.2f (somma grezza 32, compressa=%s, monotona=%s)\n",
           (double)normalZone, (double)(base + 5.0f), (double)damageWith[4], (double)damageWith[5],
           compressed ? "si" : "NO", monotonic ? "si" : "NO");

    bool ok = normalUntouched && compressed && monotonic;
    if (!ok) printf("      FALLITO: la curva del danno deve lasciare intatta la zona normale, comprimere l'impilamento, e restare monotona\n");
    return ok;
}

/* ============================================================
   Test X-Z (step D, docs/references/design-sinergie.md, sezione 6 punto 5: i
   criteri di successo della prima versione delle sinergie implicite). Le
   sinergie sono coppie: nessun oggetto sa dell'altro, ma il gioco riconosce la
   coppia e aggiunge UN effetto leggibile.
   ============================================================ */

/* Un oggetto attivo minimale con un trait e una rarita' dati. */
static Item MakeTraitItem(const char *name, unsigned int traits, Rarity rarity, ItemSlot slot)
{
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_ACTIVE;
    item.rarity = rarity;
    item.slot = slot;
    item.traits = traits;
    snprintf(item.name, sizeof(item.name), "%s", name);
    return item;
}

/* Test X (criteri 1 e 4 del design doc): la coppia ACCENDE la sinergia, e
   togliere uno dei due oggetti la SPEGNE pulita. Verifica entrambi i canali:
   il canale B (il colpo sparato riceve davvero la perforazione in piu') e la
   maschera in cache. E' il test che dimostra che la sinergia si calcola dagli
   oggetti posseduti ORA e non da Player.traits (che e' un OR monotono: se la
   rilevazione passasse da li', togliere l'oggetto non spegnerebbe nulla e
   questo test fallirebbe). */
static bool TestSynergyPairTogglesOnAndOff(void)
{
    Game game = MakeBaseGame(7001u);

    /* Un solo oggetto: nessuna coppia, nessuna sinergia. */
    TestAddItem(&game, MakeTraitItem("Occhio Rapace", TRAIT_HOMING, RARITY_UNCOMMON, SLOT_EYES));
    bool aloneOff = (game.player.synergies & (1u << SYNERGY_PIERCING_FLIGHT)) == 0u;

    /* Il secondo oggetto chiude la coppia inseguimento+perforazione. */
    TestAddItem(&game, MakeTraitItem("Punteruolo", TRAIT_PIERCE, RARITY_UNCOMMON, SLOT_HAND));
    bool pairOn = (game.player.synergies & (1u << SYNERGY_PIERCING_FLIGHT)) != 0u;

    /* Canale B: il colpo nasce con la perforazione del trait (2) PIU' quella
       della sinergia (2, scala non-comune = 1.0). */
    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    int pierceOnShot = -1;
    bool ringed = false;
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        if (!game.shots[i].active || !game.shots[i].fromPlayer) continue;
        pierceOnShot = game.shots[i].pierce;
        ringed = game.shots[i].synergized;
        break;
    }

    /* "Rimozione" (come nei test B/T: il ricalcolo da zero e' cio' che la rende
       banale il giorno che il gioco avra' un modo di scartare un oggetto). */
    game.player.itemCount = 1;
    ScriptItemsRecomputeStats(&game);
    bool removedOff = (game.player.synergies & (1u << SYNERGY_PIERCING_FLIGHT)) == 0u;

    printf("  [X] un oggetto solo: sinergia spenta=%s | coppia completa: accesa=%s, pierce sul colpo=%d (atteso 4 = 2 del trait + 2 della sinergia), anello visivo=%s | tolto un oggetto: spenta=%s\n",
           aloneOff ? "si" : "NO", pairOn ? "si" : "NO", pierceOnShot, ringed ? "si" : "NO", removedOff ? "si" : "NO");

    ScriptItemsShutdown(&game);
    bool ok = aloneOff && pairOn && pierceOnShot == 4 && ringed && removedOff;
    if (!ok) printf("      FALLITO: la coppia deve accendere la sinergia (canale B incluso) e toglierne un oggetto deve spegnerla pulita\n");
    return ok;
}

/* Test Y (criterio 2): il canale statistico e' IDEMPOTENTE. E' la proprieta' che
   rende una sinergia "solo un altro modificatore" del ricalcolo-da-zero: 100
   ricalcoli di fila devono dare esattamente lo stesso giocatore. Se qualcuno un
   giorno spostasse il contributo delle sinergie fuori dal ricalcolo (in un
   "applica una tantum al pickup"), questo test fallirebbe subito. */
static bool TestSynergyStatChannelIdempotent(void)
{
    Game game = MakeBaseGame(7002u);
    TestAddItem(&game, MakeTraitItem("Brina Lenta", TRAIT_SLOW, RARITY_RARE, SLOT_AURA));
    TestAddItem(&game, MakeTraitItem("Molla a Scatto", TRAIT_RAPID, RARITY_RARE, SLOT_HAND));

    bool active = (game.player.synergies & (1u << SYNERGY_ETERNAL_FROST)) != 0u;
    float damage = game.player.damage;
    float fireDelay = game.player.fireDelay;

    bool stable = true;
    for (int i = 0; i < 100; i++)
    {
        ScriptItemsRecomputeStats(&game);
        if (fabsf(game.player.damage - damage) > 1e-5f) stable = false;
        if (fabsf(game.player.fireDelay - fireDelay) > 1e-6f) stable = false;
        if (game.player.synergies != (unsigned int)(1u << SYNERGY_ETERNAL_FROST)) stable = false;
    }

    printf("  [Y] gelo perpetuo (rallentamento+cadenza) attivo=%s -> danno %.4f, cadenza %.4fs; 100 ricalcoli identici=%s\n",
           active ? "si" : "NO", damage, fireDelay, stable ? "si" : "NO");

    ScriptItemsShutdown(&game);
    bool ok = active && stable;
    if (!ok) printf("      FALLITO: il contributo statistico di una sinergia deve essere idempotente sotto ricalcoli ripetuti\n");
    return ok;
}

/* Test Z (criterio 3): la potenza di una sinergia scala con la rarita' MINIMA
   della coppia -- due leggendari spingono piu' di due comuni -- ma entrambe
   restano dentro la banda globale. E' il punto in cui le sinergie riusano il
   sistema di bilanciamento gia' esistente (la tavola dei tetti per rarita')
   invece di introdurne un secondo. */
static bool TestSynergyPowerScalesWithRarity(void)
{
    Game commonGame = MakeBaseGame(7003u);
    TestAddItem(&commonGame, MakeTraitItem("Brina Scialba", TRAIT_SLOW, RARITY_COMMON, SLOT_AURA));
    TestAddItem(&commonGame, MakeTraitItem("Molla Scialba", TRAIT_RAPID, RARITY_COMMON, SLOT_HAND));
    float commonDamage = commonGame.player.damage;

    Game legendaryGame = MakeBaseGame(7004u);
    TestAddItem(&legendaryGame, MakeTraitItem("Brina Eterna", TRAIT_SLOW, RARITY_LEGENDARY, SLOT_AURA));
    TestAddItem(&legendaryGame, MakeTraitItem("Molla Eterna", TRAIT_RAPID, RARITY_LEGENDARY, SLOT_HAND));
    float legendaryDamage = legendaryGame.player.damage;

    /* Coppia MISTA: comanda la rarita' MINIMA (un leggendario non "traina" un
       comune), quindi deve valere come la coppia di comuni, non come quella di
       leggendari. */
    Game mixedGame = MakeBaseGame(7005u);
    TestAddItem(&mixedGame, MakeTraitItem("Brina Scialba", TRAIT_SLOW, RARITY_COMMON, SLOT_AURA));
    TestAddItem(&mixedGame, MakeTraitItem("Molla Eterna", TRAIT_RAPID, RARITY_LEGENDARY, SLOT_HAND));
    float mixedDamage = mixedGame.player.damage;

    bool scales = legendaryDamage > commonDamage;
    bool minRarityWins = fabsf(mixedDamage - commonDamage) < 1e-3f;
    bool inBand = commonDamage >= 0.5f && legendaryDamage <= 200.0f;

    printf("  [Z] gelo perpetuo: due comuni -> danno %.4f | due leggendari -> %.4f (piu' forte=%s) | coppia mista -> %.4f (comanda la rarita' minima=%s), entrambe in banda=%s\n",
           commonDamage, legendaryDamage, scales ? "si" : "NO", mixedDamage, minRarityWins ? "si" : "NO", inBand ? "si" : "NO");

    ScriptItemsShutdown(&commonGame);
    ScriptItemsShutdown(&legendaryGame);
    ScriptItemsShutdown(&mixedGame);
    bool ok = scales && minRarityWins && inBand;
    if (!ok) printf("      FALLITO: la potenza di una sinergia deve scalare con la rarita' minima della coppia, restando in banda\n");
    return ok;
}

/* ============================================================
   Test AA-AC: le tre regressioni trovate dalla review a freddo dei commit della
   notte. Ognuno di questi tre bug era passato in mezzo a tutti i test di sopra --
   e il motivo per cui passavano e' documentato dentro ciascun test, perche' e' la
   parte che vale piu' del test stesso.
   ============================================================ */

/* Test AA (il bug piu' grave): il ricalcolo delle statistiche LEGGEVA IL PROPRIO
   OUTPUT PRECEDENTE. Le sinergie che condizionano sul tipo di colpo ("Arco
   Voltaico": un tipo che salta + un oggetto che rallenta) venivano rilevate
   PRIMA che il tipo di colpo di questo stesso ricalcolo fosse scritto su
   player.shotType -- quindi leggevano quello del giro prima.
   Due conseguenze, entrambe silenziose:
   1. ORDINE: la stessa identica coppia di oggetti dava o non dava la sinergia a
      seconda dell'ordine in cui li avevi raccolti.
   2. IDEMPOTENZA: due ricalcoli di fila davano risultati diversi.
   Cioe' esattamente le due promesse su cui e' costruito il sistema delle cache.
   Il test dell'idempotenza (Y) non se ne accorgeva perche' usa una coppia
   trait+trait, che non passa dal tipo di colpo. Questo test prova ENTRAMBI gli
   ordini di raccolta e pretende lo stesso risultato. */
static bool TestSynergyShotTypeOrderIndependent(void)
{
    /* Ordine A: prima l'oggetto che rallenta, poi quello col tipo di colpo che salta. */
    Game slowFirst = MakeBaseGame(8001u);
    TestAddItem(&slowFirst, MakeTraitItem("Brina Lenta", TRAIT_SLOW, RARITY_UNCOMMON, SLOT_AURA));
    Item arcItemA = MakeTraitItem("Bobina Saltellante", 0u, RARITY_UNCOMMON, SLOT_HAND);
    arcItemA.shotType = MakeShotType(SHOT_FORM_ARC, 1.0f, 1.0f, 1.0f, 1.0f, 0, 2, 1);
    TestAddItem(&slowFirst, arcItemA);
    bool onA = (slowFirst.player.synergies & (1u << SYNERGY_VOLTAIC_ARC)) != 0u;

    /* Ordine B: gli stessi due oggetti, raccolti al contrario. */
    Game arcFirst = MakeBaseGame(8002u);
    Item arcItemB = MakeTraitItem("Bobina Saltellante", 0u, RARITY_UNCOMMON, SLOT_HAND);
    arcItemB.shotType = MakeShotType(SHOT_FORM_ARC, 1.0f, 1.0f, 1.0f, 1.0f, 0, 2, 1);
    TestAddItem(&arcFirst, arcItemB);
    TestAddItem(&arcFirst, MakeTraitItem("Brina Lenta", TRAIT_SLOW, RARITY_UNCOMMON, SLOT_AURA));
    bool onB = (arcFirst.player.synergies & (1u << SYNERGY_VOLTAIC_ARC)) != 0u;

    /* Idempotenza: ricalcolare non deve cambiare nulla (era il secondo sintomo). */
    unsigned int before = arcFirst.player.synergies;
    ScriptItemsRecomputeStats(&arcFirst);
    ScriptItemsRecomputeStats(&arcFirst);
    bool stable = arcFirst.player.synergies == before;

    /* Un oggetto SOLO non fa una sinergia, nemmeno se porta entrambi i segnali:
       una coppia e' fra DUE oggetti (era il terzo bug: il tipo di colpo veniva
       trattato come un segnale "senza padrone", quindi l'oggetto che lo portava
       poteva fare da entrambe le meta'). */
    Game selfGame = MakeBaseGame(8003u);
    Item both = MakeTraitItem("Bobina Gelida", TRAIT_SLOW, RARITY_UNCOMMON, SLOT_HAND);
    both.shotType = MakeShotType(SHOT_FORM_ARC, 1.0f, 1.0f, 1.0f, 1.0f, 0, 2, 1);
    TestAddItem(&selfGame, both);
    bool noSelfSynergy = (selfGame.player.synergies & (1u << SYNERGY_VOLTAIC_ARC)) == 0u;

    printf("  [AA] arco voltaico: rallenta-poi-salta=%s | salta-poi-rallenta=%s (devono coincidere) | idempotente=%s | un solo oggetto NON sinergizza con se' stesso=%s\n",
           onA ? "acceso" : "SPENTO", onB ? "acceso" : "SPENTO", stable ? "si" : "NO", noSelfSynergy ? "si" : "NO");

    ScriptItemsShutdown(&slowFirst);
    ScriptItemsShutdown(&arcFirst);
    ScriptItemsShutdown(&selfGame);
    bool ok = onA && onB && stable && noSelfSynergy;
    if (!ok) printf("      FALLITO: il ricalcolo deve essere indipendente dall'ordine di raccolta e idempotente, e una sinergia richiede DUE oggetti\n");
    return ok;
}

/* Test AB: la catena bruciava il PRIMO salto. Il colpo di catena nasceva misurato
   dalla posizione del COLPO (che all'impatto e' gia' addosso al nemico) invece che
   dal centro del NEMICO: partiva quindi ancora DENTRO il bersaglio appena colpito e
   lo ricolpiva nel frame successivo, consumando subito un salto.
   DUE cose rendono questo test capace di prendere il bug, e sono la ragione per cui
   il test U non ci riusciva:
   1. chain=1, non 2. Con due salti, quello bruciato lasciava comunque l'altro
      buono: il secondo nemico veniva colpito lo stesso e il test restava verde.
   2. Il secondo nemico e' IN LINEA DIETRO il primo, non di lato. La geometria
      conta: se il salto va di traverso, lo spostamento sbagliato basta comunque a
      uscire dal nemico appena colpito (l'ho verificato: con il bug rimesso, una
      disposizione perpendicolare passava lo stesso). Il bug morde quando la catena
      prosegue nella STESSA direzione, che e' anche il caso piu' comune in gioco:
      un gruppetto di nemici in fila. */
static bool TestShotTypeChainOneJumpReachesSecondEnemy(void)
{
    Game game = MakeBaseGame(8004u);

    Item arcItem = MakeTraitItem("Bobina Singola", 0u, RARITY_UNCOMMON, SLOT_HAND);
    arcItem.shotType = MakeShotType(SHOT_FORM_ARC, 1.0f, 1.0f, 1.0f, 1.0f, 0, 1, 1);   /* UN solo salto */
    TestAddItem(&game, arcItem);

    Vector2 p = game.player.pos;
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 90.0f, p.y });    /* primo: lo prende il colpo */
    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ p.x + 200.0f, p.y });   /* secondo: in fila dietro, dentro la portata (110 px < 220) */

    CombatFirePlayer(&game, (Vector2){ 1.0f, 0.0f });
    for (int step = 0; step < 90; step++) CombatUpdateShots(&game, 1.0f/60.0f);

    /* Il secondo nemico e' quello piu' lontano: deve aver preso danno dal SALTO
       (il colpo sparato non lo puo' raggiungere da solo -- non ha perforazione,
       muore sul primo). */
    bool secondDamaged = false;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game.enemies[i];
        if (e->maxHp <= 0.0f) continue;
        if (e->pos.x > p.x + 150.0f && (e->hp < e->maxHp || !e->active)) secondDamaged = true;
    }
    printf("  [AB] catena con UN SOLO salto, nemici in fila: il secondo e' stato colpito=%s (col bug, il salto si bruciava sul primo)\n",
           secondDamaged ? "si" : "NO");

    ScriptItemsShutdown(&game);
    if (!secondDamaged) printf("      FALLITO: un salto di catena deve raggiungere un ALTRO nemico, non ricolpire quello appena colpito\n");
    return secondDamaged;
}

/* Test AC: un nemico nuovo ereditava lo stato del morto che occupava il suo slot.
   EntitiesAddEnemy ripopolava lo slot senza azzerarlo, lasciando intatti 'vel' (la
   spinta di uno script Lua) e 'slowTimer' (il rallentamento di TRAIT_SLOW): un
   nemico appena nato poteva partire gia' rallentato al 45%, o scivolando. Bug
   preesistente e invisibile -- somiglia troppo a "un nemico un po' lento". */
static bool TestEnemySlotDoesNotInheritState(void)
{
    Game game = MakeBaseGame(8005u);

    EntitiesAddEnemy(&game, ENEMY_CHASER, (Vector2){ 400.0f, 300.0f });
    game.enemies[0].slowTimer = 1.6f;                       /* rallentato... */
    game.enemies[0].vel = (Vector2){ 250.0f, -180.0f };     /* ...e spinto da uno script */
    game.enemies[0].active = false;                          /* muore, lo slot si libera */

    EntitiesAddEnemy(&game, ENEMY_TANK, (Vector2){ 500.0f, 200.0f });   /* nasce nello stesso slot */
    bool clean = game.enemies[0].slowTimer <= 0.0f &&
                 fabsf(game.enemies[0].vel.x) < 0.001f && fabsf(game.enemies[0].vel.y) < 0.001f;
    bool reused = game.enemies[0].active && game.enemies[0].kind == ENEMY_TANK;

    printf("  [AC] nemico nato in uno slot riciclato: slowTimer=%.2f vel=(%.1f,%.1f) -> pulito=%s\n",
           game.enemies[0].slowTimer, game.enemies[0].vel.x, game.enemies[0].vel.y, clean ? "si" : "NO");

    ScriptItemsShutdown(&game);
    bool ok = clean && reused;
    if (!ok) printf("      FALLITO: uno slot nemico riciclato non deve ereditare rallentamento/spinta del nemico morto\n");
    return ok;
}

/* Test J: un oggetto stat-up SENZA alcuno script Lua (mai generato: il caso
   piu' comune quando il modello fallisce/opta per non proporre nulla) non
   deve mai restare senza effetto ("so a boss reward is never a dud", task
   brief): il ripiego C (ScriptItemsApplyStatUpFallback) scatta al posto di
   on_evaluate, sommandosi al bonus "built-in" per trait che ogni oggetto
   gia' riceve (ScriptItemsApplyBuiltin, invariato da fase 3a-L2). */
static bool TestStatUpFallbackWhenNoLua(void)
{
    Game game = MakeBaseGame(4444u);
    int baseMaxHp = game.player.baseMaxHp;
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_STATUP;
    item.slot = SLOT_HAT;
    item.traits = TRAIT_VAMP;
    snprintf(item.name, sizeof(item.name), "Sigillo Vitale");
    /* luaSource vuota di proposito: nessun on_evaluate, il ripiego C deve bastare da solo. */
    TestAddItem(&game, item);

    bool noLua = !ScriptItemsHasActiveLua(&game, 0);   /* luaSource vuota: nessuna sandbox creata */
    int expected = baseMaxHp + 1 /* ApplyBuiltin, TRAIT_VAMP */ + 1 /* ripiego stat-up, TRAIT_VAMP */;
    bool ok = noLua && game.player.maxHp == expected;
    printf("  [J] oggetto stat-up SENZA Lua (trait vamp) -> maxHp=%d (atteso %d = base %d + builtin 1 + ripiego 1: mai un dud)\n",
           game.player.maxHp, expected, baseMaxHp);
    ScriptItemsShutdown(&game);
    if (!ok) printf("      FALLITO: il ripiego C per un oggetto stat-up senza Lua non e' scattato come atteso\n");
    return ok;
}

/* Test K: due oggetti stat-up si compongono (nessuna sinergia speciale,
   solo somma via il sistema delle cache) e la rimozione ricalcola senza
   deriva, esattamente come gia' verificato per gli oggetti generici in
   TestRecomputeNoDrift/TestRecomputeIdempotent: qui si estende lo stesso
   principio a ITEM_STATUP (task brief, make test-script: "extend it to the
   stat-up kind"). +1 vita ciascuno, ben sotto il tetto per-oggetto di
   maxHp (0.25*6=1.5), cosi' il test isola la composizione dal clamp. */
static const char *STATUP_ADD_MAXHP_1_LUA = "function on_evaluate(stats)\n  stats.max_hp = stats.max_hp + 1\nend\n";

static bool TestStatUpComposeAndRecompute(void)
{
    Game game = MakeBaseGame(555u);
    int baseMaxHp = game.player.baseMaxHp;

    Item item1 = { 0 }; item1.active = true; item1.kind = ITEM_STATUP; item1.slot = SLOT_HAT;
    snprintf(item1.name, sizeof(item1.name), "Cuore Piu' Uno");
    snprintf(item1.luaSource, sizeof(item1.luaSource), "%s", STATUP_ADD_MAXHP_1_LUA);
    TestAddItem(&game, item1);

    Item item2 = { 0 }; item2.active = true; item2.kind = ITEM_STATUP; item2.slot = SLOT_HAT;
    snprintf(item2.name, sizeof(item2.name), "Cuore Ancora Uno");
    snprintf(item2.luaSource, sizeof(item2.luaSource), "%s", STATUP_ADD_MAXHP_1_LUA);
    TestAddItem(&game, item2);

    int withBoth = game.player.maxHp;
    bool composeOk = withBoth == baseMaxHp + 2;
    printf("  [K] due oggetti stat-up (+1 vita ciascuno) -> maxHp=%d (atteso %d)\n", withBoth, baseMaxHp + 2);

    game.player.itemCount = 1;
    ScriptItemsRecomputeStats(&game);
    int withOne = game.player.maxHp;
    bool removedOk = withOne == baseMaxHp + 1;
    printf("  [K] rimosso il secondo oggetto stat-up: maxHp=%d (atteso %d, NESSUNA deriva)\n", withOne, baseMaxHp + 1);

    ScriptItemsShutdown(&game);
    bool ok = composeOk && removedOk;
    if (!ok) printf("      FALLITO: composizione/ricalcolo degli oggetti stat-up non corrisponde\n");
    return ok;
}

/* Test L (review CRITICO, "NaN poisons player stats through both clamps"):
   un on_evaluate che scrive 0/0 (NaN, aritmetica pura, permessa dalla
   sandbox e accettata dal dry-run del generatore) in due campi non deve mai
   raggiungere player.damage/player.maxHp. Prima della correzione,
   GameMathClampFloat (src/core/game_math.c) lasciava passare NaN intatto
   (entrambi i confronti < e > sono falsi su NaN) e ScriptItemsCallEvaluate
   (sopra) rileggeva il campo con lua_isnumber, che e' vero anche per NaN:
   damage=NaN avrebbe reso i nemici immortali (mai danno reale), e
   "p->maxHp = (int)(acc.maxHp + 0.5f)" con acc.maxHp=NaN e' un
   comportamento indefinito (su x86/SSE tipicamente INT_MIN). Qui si verifica
   che entrambi i campi restino finiti e dentro la banda di sicurezza
   (SCRIPT_ITEMS_DAMAGE_MIN/MAX, SCRIPT_ITEMS_MAX_HP_MIN/MAX), non solo
   "non-NaN": un valore finito ma fuori banda sarebbe comunque un buco. */
static const char *NAN_POISON_LUA =
    "function on_evaluate(stats)\n"
    "  stats.damage = 0/0\n"
    "  stats.max_hp = 0/0\n"
    "end\n";

static bool TestStatUpNaNPoisonClamped(void)
{
    Game game = MakeBaseGame(31416u);
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_STATUP;
    item.slot = SLOT_HAT;
    snprintf(item.name, sizeof(item.name), "Nucleo Instabile");
    snprintf(item.luaSource, sizeof(item.luaSource), "%s", NAN_POISON_LUA);
    TestAddItem(&game, item);

    float damage = game.player.damage;
    int maxHp = game.player.maxHp;
    bool damageFinite = isfinite(damage) != 0;
    bool damageInBand = damageFinite && damage >= 0.5f && damage <= 200.0f;
    /* maxHp != INT_MIN esclude specificamente l'esito da manuale del task
       brief ((int)(NaN+0.5f) su x86/SSE); il controllo di banda sotto e' il
       vero criterio (finito E dentro [1,12], non solo "non e' INT_MIN"). */
    bool maxHpNotUB = maxHp != INT_MIN;
    bool maxHpInBand = maxHpNotUB && maxHp >= 1 && maxHp <= 12;
    printf("  [L] on_evaluate scrive stats.damage=0/0, stats.max_hp=0/0 (NaN) -> damage=%f maxHp=%d "
           "(attesi entrambi finiti e dentro banda: damage in [0.5,200], maxHp in [1,12])\n",
           (double)damage, maxHp);

    ScriptItemsShutdown(&game);
    bool ok = damageInBand && maxHpInBand;
    if (!ok) printf("      FALLITO: un on_evaluate che scrive NaN deve restare dentro banda finita "
                     "(serve GameMathClampFloat NaN-safe + isfinite al confine Lua->C in ScriptItemsCallEvaluate)\n");
    return ok;
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
        { "I (stat-up: budget per-oggetto, +1000 danno clampato)", TestStatUpClampedToPerItemCap },
        { "J (stat-up: ripiego C quando non c'e' Lua, mai un dud)", TestStatUpFallbackWhenNoLua },
        { "K (stat-up: due oggetti si compongono, rimozione senza deriva)", TestStatUpComposeAndRecompute },
        { "L (stat-up: NaN da on_evaluate resta clampato, mai NaN/INT_MIN)", TestStatUpNaNPoisonClamped },
        { "M (rarita': il tetto per-oggetto scala, leggendario > comune, entrambi in banda)", TestRarityCapDiffersFromCommonToLegendary },
        { "N (rarita': cinque leggendari impilati restano dentro la banda globale)", TestFiveLegendariesStayInBand },
        { "O (rarita': il costo del negozio cresce con la rarita')", TestShopCostScalesWithRarity },
        { "P (rarita': RarityFromText round-trip sui 4 testi canonici, sincronizzato con GEN_RARITIES)", TestRarityTextRoundTrip },
        { "Q (rarita': il ripiego puro senza manifest da' comunque un boss raro/leggendario, mai comune)", TestFallbackBossItemIsRare },
        { "R (tipi di colpo: qualunque cosa inventi il modello resta in banda di potenza)", TestShotTypeAlwaysBalanced },
        { "S (tipi di colpo: round-trip testo<->enum delle forme, sincronizzato con run.gbnf)", TestShotFormTextRoundTrip },
        { "T (tipi di colpo: vince l'ultimo raccolto, rimuoverlo ripristina il precedente)", TestShotTypeLastItemWinsAndReverts },
        { "U (tipi di colpo: 'chain' salta davvero su un secondo nemico all'impatto)", TestShotTypeChainJumpsToSecondEnemy },
        { "V (tipi di colpo: 'pierce' fa attraversare il primo nemico)", TestShotTypePierceSurvivesFirstEnemy },
        { "W (curve Isaac: rendimenti decrescenti sul danno, zona normale intatta, monotona)", TestDamageCurveDiminishesButNeverHurts },
        { "X (sinergie: la coppia accende, togliere un oggetto spegne pulito)", TestSynergyPairTogglesOnAndOff },
        { "Y (sinergie: il canale statistico e' idempotente su 100 ricalcoli)", TestSynergyStatChannelIdempotent },
        { "Z (sinergie: la potenza scala con la rarita' minima della coppia, in banda)", TestSynergyPowerScalesWithRarity },
        { "AA (review: ricalcolo indipendente dall'ordine di raccolta, idempotente, niente auto-sinergia)", TestSynergyShotTypeOrderIndependent },
        { "AB (review: un salto di catena raggiunge un ALTRO nemico, non ricolpisce quello gia' colpito)", TestShotTypeChainOneJumpReachesSecondEnemy },
        { "AC (review: uno slot nemico riciclato non eredita rallentamento/spinta del morto)", TestEnemySlotDoesNotInheritState },
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
