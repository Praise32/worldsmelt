#include "script/script_character.h"

#include "core/game_math.h"
#include "script/script_api.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <math.h>
#include <string.h>

#define SCRIPT_CHARACTER_NO_REF (-1)

/* Percorso fisso, UNICO per ogni run (a differenza degli oggetti, che hanno
   un file per floor/slot, vedi WriteItemLua in tools/melting-gen/
   gen_manifest.c): un solo personaggio generato per run, quindi un solo
   file. Stessa scelta di 'RunContentLoadCharacterProposal' per
   generated/character_proposal.json (src/content/character_proposal.c):
   letterale fisso, mai derivato da un parametro. */
#define SCRIPT_CHARACTER_TRAIT_PATH "generated/scripts/character_trait.lua"

static void ScriptCharacterResetRuntime(ScriptCharacterRuntime *rt)
{
    rt->sandbox = NULL;
    rt->evalRef = SCRIPT_CHARACTER_NO_REF;
    rt->fireRef = SCRIPT_CHARACTER_NO_REF;
    rt->hitRef = SCRIPT_CHARACTER_NO_REF;
    rt->tickRef = SCRIPT_CHARACTER_NO_REF;
    rt->statsTableRef = SCRIPT_CHARACTER_NO_REF;
}

void ScriptCharacterInit(Game *game)
{
    ScriptCharacterResetRuntime(&game->characterTrait);
}

void ScriptCharacterShutdown(Game *game)
{
    ScriptCharacterRuntime *rt = &game->characterTrait;
    if (rt->sandbox != NULL) ScriptSandboxDestroy((ScriptSandbox *)rt->sandbox);
    ScriptCharacterResetRuntime(rt);
}

static int ScriptCharacterCacheGlobalRef(lua_State *L, const char *name)
{
    lua_getglobal(L, name);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return SCRIPT_CHARACTER_NO_REF; }
    return luaL_ref(L, LUA_REGISTRYINDEX);   /* fa il pop da solo */
}

/* Carica 'path' nella sandbox del trait (gia' azzerata dal chiamante, vedi
   ScriptCharacterSetActive sotto). Qualunque fallimento -- file assente/
   vuoto (LoadFileText torna NULL/stringa vuota per un percorso inesistente,
   nessuna eccezione raylib da gestire), sandbox non creabile, script che non
   compila -- lascia il trait silenzioso e inattivo (requisito 2 della spec:
   "fallimento di caricamento/compilazione a run gia' iniziata: mai un
   crash"), mai propagato al chiamante come errore. */
static void ScriptCharacterLoad(Game *game, const char *path)
{
    ScriptCharacterRuntime *rt = &game->characterTrait;

    char *source = LoadFileText(path);
    if (!source || !source[0])
    {
        if (source) UnloadFileText(source);
        return;
    }

    /* Seed dedicato dalla RNG di gioco, avanzata (stesso schema di
       ScriptItemsOnAcquire in script_items.c): due run con lo stesso seed
       producono lo stesso seme per il trait. */
    unsigned int seed = GameRngNext(&game->rng);
    ScriptSandbox *sb = ScriptSandboxCreate(seed, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb) { UnloadFileText(source); return; }   /* niente memoria: il personaggio resta senza trait */

    ScriptApiRegister(sb, game);

    char err[160];
    bool loaded = ScriptSandboxLoad(sb, "character-trait", source, err, sizeof(err));
    UnloadFileText(source);
    if (!loaded)
    {
        /* Gia' loggato da ScriptSandboxLoad. Si tiene la sandbox disabilitata
           (come ScriptItemsOnAcquire fa per un oggetto) solo per coerenza
           diagnostica: ScriptCharacterHasActiveLua tornera' comunque falso. */
        rt->sandbox = sb;
        return;
    }

    lua_State *L = ScriptSandboxRawState(sb);
    rt->sandbox = sb;
    rt->evalRef = ScriptCharacterCacheGlobalRef(L, "on_evaluate");
    rt->fireRef = ScriptCharacterCacheGlobalRef(L, "on_fire");
    rt->hitRef  = ScriptCharacterCacheGlobalRef(L, "on_hit");
    rt->tickRef = ScriptCharacterCacheGlobalRef(L, "on_tick");

    if (rt->evalRef != SCRIPT_CHARACTER_NO_REF)
    {
        lua_newtable(L);
        rt->statsTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
}

void ScriptCharacterSetActive(Game *game, const CharacterDef *character)
{
    ScriptCharacterShutdown(game);   /* sempre da zero: mai un trait vecchio dietro uno nuovo */
    if (!character || !character->traitHook[0]) return;   /* rosa curata, o generato senza trait valido */
    ScriptCharacterLoad(game, SCRIPT_CHARACTER_TRAIT_PATH);
}

bool ScriptCharacterHasActiveLua(const Game *game)
{
    const ScriptSandbox *sb = (const ScriptSandbox *)game->characterTrait.sandbox;
    return sb != NULL && !ScriptSandboxIsDisabled(sb);
}

static void ScriptCharacterCallCachedVoid(ScriptSandbox *sb, int ref, const double *args, int nargs)
{
    if (ref == SCRIPT_CHARACTER_NO_REF) return;
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    for (int i = 0; i < nargs; i++) lua_pushnumber(L, (lua_Number)args[i]);
    ScriptSandboxProtectedCall(sb, nargs, 0);
}

void ScriptCharacterOnFire(Game *game, Vector2 pos, Vector2 dir)
{
    ScriptCharacterRuntime *rt = &game->characterTrait;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    if (sb == NULL || ScriptSandboxIsDisabled(sb)) return;
    double args[4] = { (double)pos.x, (double)pos.y, (double)dir.x, (double)dir.y };
    ScriptCharacterCallCachedVoid(sb, rt->fireRef, args, 4);
}

void ScriptCharacterOnHit(Game *game, int shotIndex, int enemyIndex)
{
    if (shotIndex < 0 || shotIndex >= MAX_SHOTS || enemyIndex < 0 || enemyIndex >= MAX_ENEMIES) return;
    ScriptCharacterRuntime *rt = &game->characterTrait;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    if (sb == NULL || ScriptSandboxIsDisabled(sb)) return;
    double args[2] = { ScriptApiPackShotHandle(game, shotIndex), ScriptApiPackEnemyHandle(game, enemyIndex) };
    ScriptCharacterCallCachedVoid(sb, rt->hitRef, args, 2);
}

void ScriptCharacterOnTick(Game *game, float dt)
{
    ScriptCharacterRuntime *rt = &game->characterTrait;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    if (sb == NULL || ScriptSandboxIsDisabled(sb)) return;
    double args[1] = { (double)dt };
    ScriptCharacterCallCachedVoid(sb, rt->tickRef, args, 1);
}

bool ScriptCharacterEvaluate(Game *game, float *damage, float *fireDelay, float *shotSpeed,
                              float *shotRadius, float *speed, float *maxHp, float *luck)
{
    ScriptCharacterRuntime *rt = &game->characterTrait;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    if (sb == NULL || ScriptSandboxIsDisabled(sb)) return false;
    if (rt->evalRef == SCRIPT_CHARACTER_NO_REF || rt->statsTableRef == SCRIPT_CHARACTER_NO_REF) return false;

    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->evalRef);          /* funzione: arg 1 di lua_pcall */
    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);    /* tabella di scratch: unico argomento */
    lua_pushnumber(L, (lua_Number)*damage);     lua_setfield(L, -2, "damage");
    lua_pushnumber(L, (lua_Number)*fireDelay);  lua_setfield(L, -2, "fire_delay");
    lua_pushnumber(L, (lua_Number)*shotSpeed);  lua_setfield(L, -2, "shot_speed");
    lua_pushnumber(L, (lua_Number)*shotRadius); lua_setfield(L, -2, "shot_radius");
    lua_pushnumber(L, (lua_Number)*speed);      lua_setfield(L, -2, "speed");
    lua_pushnumber(L, (lua_Number)*maxHp);      lua_setfield(L, -2, "max_hp");
    lua_pushnumber(L, (lua_Number)*luck);       lua_setfield(L, -2, "luck");

    if (!ScriptSandboxProtectedCall(sb, 1, 0)) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);
    int t = lua_gettop(L);
    /* isfinite, non solo lua_isnumber: stesso presidio anti-NaN/inf di
       ScriptItemsCallEvaluate (script_items.c) -- vedi il commento li' per
       il "perche'" completo (aritmetica pura permessa dalla sandbox puo'
       produrre 0/0 o inf*0, "numeri" Lua a tutti gli effetti). Un campo non
       finito lascia il valore CORRENTE (gia' scritto dal chiamante prima di
       questa chiamata) intatto. */
    float v;
    lua_getfield(L, t, "damage");      if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *damage     = v; } lua_pop(L, 1);
    lua_getfield(L, t, "fire_delay");  if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *fireDelay  = v; } lua_pop(L, 1);
    lua_getfield(L, t, "shot_speed");  if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *shotSpeed  = v; } lua_pop(L, 1);
    lua_getfield(L, t, "shot_radius"); if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *shotRadius = v; } lua_pop(L, 1);
    lua_getfield(L, t, "speed");       if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *speed      = v; } lua_pop(L, 1);
    lua_getfield(L, t, "max_hp");      if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *maxHp      = v; } lua_pop(L, 1);
    lua_getfield(L, t, "luck");        if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) *luck       = v; } lua_pop(L, 1);
    lua_pop(L, 1);   /* la tabella stessa */
    return true;
}
