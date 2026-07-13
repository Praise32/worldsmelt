#include "script/script_items.h"

#include "core/game_math.h"
#include "script/script_api.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <math.h>

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

/* ============================================================
   Budget di potenza PER OGGETTO (fase 3, vision doc sezione 1: "un budget di
   potenza per oggetto, non bonus arbitrari"). SCRIPT_ITEMS_CLAMP_STATS sopra
   e' un tetto GLOBALE, indipendente da quanti oggetti hai: protegge il
   giocatore nel suo insieme (mai piu' veloce di X, mai piu' fragile di Y),
   ma da solo NON impedisce che un singolo oggetto malfatto (un 7B che sbaglia
   i conti, o semplicemente uno script troppo generoso) spinga UNA statistica
   fin quasi al tetto in un colpo solo. Qui sotto si aggiunge un secondo
   limite, per-oggetto: quanto puo' SPOSTARE quella statistica un singolo
   on_evaluate, a prescindere da cosa il tetto globale permetterebbe ancora.

   La percentuale e' relativa a player.base* (il valore di PARTENZA della
   run, fisso), non al valore corrente prima di questo oggetto: un oggetto
   raccolto per decimo non deve poter spostare la statistica piu' di uno
   raccolto per primo solo perche' i nove precedenti l'hanno gia' gonfiata.
   25% e' la stessa cifra per tutte le sei statistiche: abbastanza per
   sentire davvero un oggetto stat-up (su damage=8 sono +2, un incremento
   del 25% e' ben percepibile), abbastanza poco perche' anche il singolo
   oggetto peggio riuscito nell'intera run (al massimo 5 oggetti stat-up,
   uno per piano) non possa mai avvicinarsi da solo al tetto globale sopra
   (0.25*8=2 contro un tetto [0.5,200]; anche sommando tutti e 5 i piani al
   loro massimo assoluto restano ben dentro banda).

   Applicato SOLO agli oggetti ITEM_STATUP (task brief, fase 3: "cap how
   much a single stat-up item may shift any stat"), non a ogni on_evaluate:
   un oggetto ATTIVO puo' gia' definire on_evaluate oggi (fase 3a-L2, prima
   di questo task) per esprimere sinergie piu' ricche di un semplice
   modificatore statico (vedi src/tests/script_items_tests.c, test B/C, che
   sommano +2/+3 danno da due oggetti attivi ben oltre il 25% di un singolo
   oggetto) -- resta cosi', la sandbox non perde liberta'. Il budget
   per-oggetto e' la promessa di equilibrio specifica dei BOSS REWARD, non
   una restrizione nuova sull'intera libreria di oggetti attivi gia'
   esistente. */
#define SCRIPT_ITEMS_ITEM_DELTA_FRACTION 0.25f

static float ScriptItemsClampItemDeltaField(float post, float pre, float base)
{
    float cap = fabsf(base)*SCRIPT_ITEMS_ITEM_DELTA_FRACTION;
    float delta = GameMathClampFloat(post - pre, -cap, cap);
    return pre + delta;
}

/* Applica il tetto per-oggetto a TUTTE le statistiche che 'post' porta
   rispetto a 'pre' (i valori subito prima di chiamare on_evaluate per
   QUESTO oggetto), scrivendo il risultato clampato in 'post' stesso. Il
   tetto globale (ScriptItemsClampStats) va comunque richiamato DOPO questa
   funzione dal chiamante: sono due reti distinte, non una alternativa
   all'altra. */
static void ScriptItemsClampItemDelta(ScriptItemsStatsAccum *post, const ScriptItemsStatsAccum *pre, const Player *p)
{
    post->damage     = ScriptItemsClampItemDeltaField(post->damage,     pre->damage,     p->baseDamage);
    post->fireDelay  = ScriptItemsClampItemDeltaField(post->fireDelay,  pre->fireDelay,  p->baseFireDelay);
    post->shotSpeed  = ScriptItemsClampItemDeltaField(post->shotSpeed,  pre->shotSpeed,  p->baseShotSpeed);
    post->shotRadius = ScriptItemsClampItemDeltaField(post->shotRadius, pre->shotRadius, p->baseShotRadius);
    post->speed      = ScriptItemsClampItemDeltaField(post->speed,      pre->speed,      p->baseSpeed);
    post->maxHp      = ScriptItemsClampItemDeltaField(post->maxHp,      pre->maxHp,      (float)p->baseMaxHp);
}

/* Ripiego fisso e sicuro (fase 3, task brief: "so a boss reward is never a
   dud"): un oggetto stat-up SENZA un on_evaluate Lua funzionante (mai
   generato, o generato ma bocciato dalla validazione/ucciso a runtime, vedi
   ScriptItemsRecomputeStats sotto) prende comunque UN bonus, piccolo ma
   reale, deciso qui in C e scelto in base al suo trait (lo stesso trait che
   nel manifest serve solo da "etichetta", vedi tools/melting-gen/gen_fallback.c
   FallbackBossItem): niente RNG, stesso trait -> sempre lo stesso bonus,
   cosi' il ripiego resta prevedibile e testabile quanto il resto del
   sistema delle cache. Passa comunque per ScriptItemsClampItemDelta subito
   dopo (vedi il chiamante), quindi anche questi valori "piccoli a mano"
   restano sotto lo stesso tetto per-oggetto di un on_evaluate scritto da un
   modello. Ordine di priorita' identico a ItemFirstTraitName
   (src/gameplay/item_traits.c): un solo trait guida un solo bonus. */
static void ScriptItemsApplyStatUpFallback(ScriptItemsStatsAccum *acc, const Item *item)
{
    if (item->traits & TRAIT_VAMP)         { acc->maxHp     += 1.0f;  return; }
    if (item->traits & TRAIT_GIANT)        { acc->damage    += 1.5f;  return; }
    if (item->traits & TRAIT_RAPID)        { acc->fireDelay -= 0.03f; return; }
    if (item->traits & TRAIT_PIERCE)       { acc->damage    += 1.0f;  return; }
    if (item->traits & TRAIT_HOMING)       { acc->shotSpeed += 60.0f; return; }
    if (item->traits & TRAIT_BOUNCE)       { acc->shotSpeed += 60.0f; return; }
    if (item->traits & TRAIT_EXPLODE)      { acc->shotRadius += 1.0f; return; }
    if (item->traits & TRAIT_SPLIT)        { acc->shotRadius += 1.0f; return; }
    if (item->traits & TRAIT_SLOW)         { acc->speed      += 20.0f; return; }
    acc->maxHp += 1.0f;   /* nessun trait riconosciuto: un cuore extra, sempre sicuro */
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
        bool sandboxUsable = sb != NULL && !ScriptSandboxIsDisabled(sb);
        bool isStatUp = item->kind == ITEM_STATUP;

        /* on_evaluate, quando c'e' ed e' ancora vivo: per un oggetto
           ITEM_STATUP anche il budget per-oggetto (vedi
           ScriptItemsClampItemDelta sopra), SEMPRE seguito dal tetto
           globale. Un oggetto ATTIVO passa SOLO dal tetto globale, come
           prima di questa fase (vedi il commento sopra la macro). */
        bool ranLuaEval = false;
        if (sandboxUsable && rt->evalRef != SCRIPT_ITEMS_NO_REF)
        {
            ScriptItemsStatsAccum pre = acc;
            if (ScriptItemsCallEvaluate(rt, &acc))
            {
                if (isStatUp) ScriptItemsClampItemDelta(&acc, &pre, p);
                ranLuaEval = true;
            }
            ScriptItemsClampStats(&acc);   /* di nuovo: anche dopo un fallimento, per sicurezza in profondita' */
        }

        /* Ripiego "mai un dud" (task brief, fase 3): un oggetto STAT-UP
           senza un on_evaluate Lua riuscito in QUESTO ricalcolo (mai
           acquisito con Lua, sandbox disabilitata dal patto di sicurezza, o
           script che non definisce on_evaluate) prende comunque il bonus
           fisso di ScriptItemsApplyStatUpFallback. Un oggetto ATTIVO senza
           on_evaluate resta invece esattamente come oggi: solo
           ScriptItemsApplyBuiltin, nessun bonus in piu' inventato qui. */
        if (!ranLuaEval && isStatUp)
        {
            ScriptItemsStatsAccum pre = acc;
            ScriptItemsApplyStatUpFallback(&acc, item);
            ScriptItemsClampItemDelta(&acc, &pre, p);
            ScriptItemsClampStats(&acc);
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
