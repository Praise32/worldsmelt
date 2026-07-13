#include "script/script_items.h"

#include "core/game_math.h"
#include "script/script_api.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#define SCRIPT_ITEMS_NO_REF (-1)

/* ============================================================
   Ciclo di vita per-slot
   ============================================================ */

static void ScriptItemsResetSlot(ScriptItemRuntime *rt)
{
    rt->sandbox = NULL;
    rt->evalRef = SCRIPT_ITEMS_NO_REF;
    rt->fireRef = SCRIPT_ITEMS_NO_REF;
    rt->hitRef = SCRIPT_ITEMS_NO_REF;
    rt->tickRef = SCRIPT_ITEMS_NO_REF;
    rt->statsTableRef = SCRIPT_ITEMS_NO_REF;
}

void ScriptItemsInit(Game *game)
{
    for (int i = 0; i < MAX_ITEMS; i++) ScriptItemsResetSlot(&game->itemScripts[i]);
    game->statsDirty = false;
    ScriptItemsRecomputeStats(game);   /* 0 oggetti -> player.* = player.base* */
}

void ScriptItemsShutdown(Game *game)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        if (rt->sandbox != NULL) ScriptSandboxDestroy((ScriptSandbox *)rt->sandbox);
        ScriptItemsResetSlot(rt);
    }
}

static int ScriptItemsCacheGlobalRef(lua_State *L, const char *name)
{
    lua_getglobal(L, name);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return SCRIPT_ITEMS_NO_REF; }
    return luaL_ref(L, LUA_REGISTRYINDEX);   /* fa il pop da solo */
}

void ScriptItemsOnAcquire(Game *game, int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= MAX_ITEMS) return;
    ScriptItemRuntime *rt = &game->itemScripts[itemIndex];
    if (rt->sandbox != NULL) ScriptSandboxDestroy((ScriptSandbox *)rt->sandbox);
    ScriptItemsResetSlot(rt);

    const Item *item = &game->player.items[itemIndex];
    if (item->luaSource[0] == '\0') { game->statsDirty = true; return; }   /* solo mini-VM */

    /* Seed dedicato per questo script, tratto dalla RNG di gioco (gia'
       seminata una volta sola da GameResetRun) e avanzato: il prossimo
       consumatore (nemici, particelle...) resta nella stessa sequenza, e
       due run con lo stesso seed di gioco producono la stessa sequenza di
       semi per gli script -> stesso comportamento (spec, sezione 9,
       criterio 5). */
    unsigned int seed = GameRngNext(&game->rng);
    ScriptSandbox *sb = ScriptSandboxCreate(seed, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) { game->statsDirty = true; return; }   /* niente memoria: resta sulla mini-VM */

    ScriptApiRegister(sb, game);

    char err[160];
    if (!ScriptSandboxLoad(sb, item->name, item->luaSource, err, sizeof(err)))
    {
        /* Gia' loggato da ScriptSandboxKill dentro ScriptSandboxLoad. Si
           tiene comunque la sandbox (disabilitata): ScriptItemsHasActiveLua
           tornera' falso, l'oggetto ripiega sulla sua mini-VM da subito. La
           si tiene viva solo per coerenza diagnostica (DisabledReason), non
           costa il tetto di memoria Lua: lo stato Lua e' gia' inutilizzabile
           ma non viene chiuso finche' non arriva un nuovo ScriptItemsOnAcquire
           su questo slot o ScriptItemsShutdown. */
        rt->sandbox = sb;
        game->statsDirty = true;
        return;
    }

    lua_State *L = ScriptSandboxRawState(sb);
    rt->sandbox = sb;
    rt->evalRef = ScriptItemsCacheGlobalRef(L, "on_evaluate");
    rt->fireRef = ScriptItemsCacheGlobalRef(L, "on_fire");
    rt->hitRef = ScriptItemsCacheGlobalRef(L, "on_hit");
    rt->tickRef = ScriptItemsCacheGlobalRef(L, "on_tick");

    /* Tabella di scratch per on_evaluate, creata una volta e riusata ad ogni
       ricalcolo invece che allocata per chiamata (spec, sezione 5). Solo se
       lo script implementa davvero on_evaluate: altrimenti non serve mai. */
    if (rt->evalRef != SCRIPT_ITEMS_NO_REF)
    {
        lua_newtable(L);
        rt->statsTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    game->statsDirty = true;
}

bool ScriptItemsHasActiveLua(const Game *game, int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= MAX_ITEMS) return false;
    const ScriptSandbox *sb = (const ScriptSandbox *)game->itemScripts[itemIndex].sandbox;
    return sb != NULL && !ScriptSandboxIsDisabled(sb);
}

/* ============================================================
   Callback per-evento: funzione Lua cache (luaL_ref) + argomenti
   numerici, tutti passati attraverso ScriptSandboxProtectedCall
   (stesso budget/kill-switch di ScriptSandboxCallVoid, vedi
   script_sandbox.h).
   ============================================================ */

static void ScriptItemsCallCachedVoid(ScriptSandbox *sb, int ref, const double *args, int nargs)
{
    if (ref == SCRIPT_ITEMS_NO_REF) return;   /* hook non implementato: non e' un errore */
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    for (int i = 0; i < nargs; i++) lua_pushnumber(L, (lua_Number)args[i]);
    ScriptSandboxProtectedCall(sb, nargs, 0);
}

void ScriptItemsOnFire(Game *game, Vector2 pos, Vector2 dir)
{
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[4] = { (double)pos.x, (double)pos.y, (double)dir.x, (double)dir.y };
        ScriptItemsCallCachedVoid(sb, rt->fireRef, args, 4);
    }
}

void ScriptItemsOnHit(Game *game, int shotIndex, int enemyIndex)
{
    if (shotIndex < 0 || shotIndex >= MAX_SHOTS || enemyIndex < 0 || enemyIndex >= MAX_ENEMIES) return;
    double shotHandle = ScriptApiPackShotHandle(game, shotIndex);
    double enemyHandle = ScriptApiPackEnemyHandle(game, enemyIndex);
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[2] = { shotHandle, enemyHandle };
        ScriptItemsCallCachedVoid(sb, rt->hitRef, args, 2);
    }
}

void ScriptItemsOnTick(Game *game, float dt)
{
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[1] = { (double)dt };
        ScriptItemsCallCachedVoid(sb, rt->tickRef, args, 1);
    }
}

/* ============================================================
   Il sistema delle cache: recompute-from-zero (spec, sezione 7)
   ============================================================ */

typedef struct ScriptItemsStatsAccum
{
    float damage;
    float fireDelay;
    float shotSpeed;
    float shotRadius;
    float speed;
    float maxHp;   /* float per condividere lo stesso clamp delle altre; arrotondata a int solo alla fine */
} ScriptItemsStatsAccum;

/* Confini di sicurezza (spec, sezione 3 della task brief: "C clamps every
   field to sane bounds after each item's pass"). Scelti larghi ma finiti
   attorno ai valori di partenza di GameResetRun (damage 8, fireDelay 0.23,
   shotSpeed 520, shotRadius 5, speed 224, maxHp 6, tetto 12 gia' usato
   altrove per maxHp, vedi lo storico CombatApplyItem/TRAIT_VAMP): nessun
   oggetto, built-in o Lua, puo' produrre un giocatore che non spara piu',
   non si muove piu', o e' immortale/istantaneamente morto. */
#define SCRIPT_ITEMS_DAMAGE_MIN      0.5f
#define SCRIPT_ITEMS_DAMAGE_MAX      200.0f
#define SCRIPT_ITEMS_FIRE_DELAY_MIN  0.05f
#define SCRIPT_ITEMS_FIRE_DELAY_MAX  2.0f
#define SCRIPT_ITEMS_SHOT_SPEED_MIN  60.0f
#define SCRIPT_ITEMS_SHOT_SPEED_MAX  1400.0f
#define SCRIPT_ITEMS_SHOT_RADIUS_MIN 2.0f
#define SCRIPT_ITEMS_SHOT_RADIUS_MAX 40.0f
#define SCRIPT_ITEMS_SPEED_MIN       60.0f
#define SCRIPT_ITEMS_SPEED_MAX       600.0f
#define SCRIPT_ITEMS_MAX_HP_MIN      1.0f
#define SCRIPT_ITEMS_MAX_HP_MAX      12.0f

static void ScriptItemsClampStats(ScriptItemsStatsAccum *acc)
{
    acc->damage     = GameMathClampFloat(acc->damage,     SCRIPT_ITEMS_DAMAGE_MIN,      SCRIPT_ITEMS_DAMAGE_MAX);
    acc->fireDelay  = GameMathClampFloat(acc->fireDelay,  SCRIPT_ITEMS_FIRE_DELAY_MIN,  SCRIPT_ITEMS_FIRE_DELAY_MAX);
    acc->shotSpeed  = GameMathClampFloat(acc->shotSpeed,  SCRIPT_ITEMS_SHOT_SPEED_MIN,  SCRIPT_ITEMS_SHOT_SPEED_MAX);
    acc->shotRadius = GameMathClampFloat(acc->shotRadius, SCRIPT_ITEMS_SHOT_RADIUS_MIN, SCRIPT_ITEMS_SHOT_RADIUS_MAX);
    acc->speed      = GameMathClampFloat(acc->speed,      SCRIPT_ITEMS_SPEED_MIN,       SCRIPT_ITEMS_SPEED_MAX);
    acc->maxHp      = GameMathClampFloat(acc->maxHp,      SCRIPT_ITEMS_MAX_HP_MIN,      SCRIPT_ITEMS_MAX_HP_MAX);
}

/* La stessa matematica che prima viveva UNA TANTUM (al pickup) in
   CombatApplyItem, ora ricalcolata da zero ad ogni passaggio: e' quello che
   rende l'aggiunta/rimozione di un oggetto priva di deriva (spec, sezione
   7). Valori invariati rispetto alla versione precedente di CombatApplyItem. */
static void ScriptItemsApplyBuiltin(ScriptItemsStatsAccum *acc, const Item *item)
{
    if (item->traits & TRAIT_RAPID) acc->fireDelay *= 0.92f;
    if (item->traits & TRAIT_GIANT)
    {
        acc->damage += 1.6f;
        acc->shotRadius += 0.8f;
    }
    if (item->traits & TRAIT_PIERCE) acc->damage += 0.8f;
    if (item->traits & TRAIT_VAMP) acc->maxHp += 1.0f;
    if (item->slot == SLOT_BODY) acc->maxHp += 1.0f;
    if (item->slot == SLOT_HAND) acc->damage += 1.0f;
    if (item->slot == SLOT_EYES) acc->shotSpeed += 25.0f;
}

/* Chiama on_evaluate(stats) sulla tabella di scratch riusata (statsTableRef),
   scrivendo i valori CORRENTI di 'acc' prima della chiamata e rileggendoli
   dopo. Ritorna false se lo script e' stato ucciso durante la chiamata: in
   quel caso 'acc' NON viene toccato (i campi si leggono solo dopo un
   successo), quindi qualunque scrittura la tabella avesse ricevuto prima
   dell'errore (Lua non fa rollback delle mutazioni su una tabella
   condivisa) non raggiunge mai le statistiche vere del giocatore. */
static bool ScriptItemsCallEvaluate(ScriptItemRuntime *rt, ScriptItemsStatsAccum *acc)
{
    if (rt->evalRef == SCRIPT_ITEMS_NO_REF || rt->statsTableRef == SCRIPT_ITEMS_NO_REF) return true;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->evalRef);          /* funzione: arg 1 di lua_pcall */
    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);    /* tabella di scratch: unico argomento */
    lua_pushnumber(L, (lua_Number)acc->damage);     lua_setfield(L, -2, "damage");
    lua_pushnumber(L, (lua_Number)acc->fireDelay);  lua_setfield(L, -2, "fire_delay");
    lua_pushnumber(L, (lua_Number)acc->shotSpeed);  lua_setfield(L, -2, "shot_speed");
    lua_pushnumber(L, (lua_Number)acc->shotRadius); lua_setfield(L, -2, "shot_radius");
    lua_pushnumber(L, (lua_Number)acc->speed);      lua_setfield(L, -2, "speed");
    lua_pushnumber(L, (lua_Number)acc->maxHp);      lua_setfield(L, -2, "max_hp");

    if (!ScriptSandboxProtectedCall(sb, 1, 0)) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);
    int t = lua_gettop(L);
    lua_getfield(L, t, "damage");      if (lua_isnumber(L, -1)) acc->damage     = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, t, "fire_delay");  if (lua_isnumber(L, -1)) acc->fireDelay  = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, t, "shot_speed");  if (lua_isnumber(L, -1)) acc->shotSpeed  = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, t, "shot_radius"); if (lua_isnumber(L, -1)) acc->shotRadius = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, t, "speed");       if (lua_isnumber(L, -1)) acc->speed      = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, t, "max_hp");      if (lua_isnumber(L, -1)) acc->maxHp      = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);   /* la tabella stessa */
    return true;
}

void ScriptItemsProcessDirty(Game *game)
{
    if (!game->statsDirty) return;
    ScriptItemsRecomputeStats(game);
    game->statsDirty = false;
}

void ScriptItemsRecomputeStats(Game *game)
{
    Player *p = &game->player;
    ScriptItemsStatsAccum acc = {
        p->baseDamage, p->baseFireDelay, p->baseShotSpeed, p->baseShotRadius, p->baseSpeed, (float)p->baseMaxHp
    };

    for (int i = 0; i < p->itemCount; i++)
    {
        const Item *item = &p->items[i];
        ScriptItemsApplyBuiltin(&acc, item);
        ScriptItemsClampStats(&acc);

        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb != NULL && !ScriptSandboxIsDisabled(sb))
        {
            ScriptItemsCallEvaluate(rt, &acc);
            ScriptItemsClampStats(&acc);   /* di nuovo: anche dopo un fallimento, per sicurezza in profondita' */
        }
    }

    p->damage = acc.damage;
    p->fireDelay = acc.fireDelay;
    p->shotSpeed = acc.shotSpeed;
    p->shotRadius = acc.shotRadius;
    p->speed = acc.speed;
    p->maxHp = (int)(acc.maxHp + 0.5f);
    if (p->hp > p->maxHp) p->hp = p->maxHp;
}
