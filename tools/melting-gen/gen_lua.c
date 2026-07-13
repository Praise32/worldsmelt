/* Vedi gen_lua.h per la panoramica. Compila dentro melting-gen anche
   src/script/script_sandbox.c (stessa sandbox del gioco: allowlist, tetto
   di memoria, budget di istruzioni, TUTTI identici, vedi Makefile) e usa
   qui un'API di gioco FINTA invece di src/script/script_api.c: quel file
   prende un Game* vero (src/game/game_internal.h), che trascinerebbe dentro
   melting-gen l'intero stato di gioco/raylib runtime solo per un dry-run
   usa-e-getta. Le funzioni sotto hanno gli STESSI NOMI e la STESSA ARITA'
   di script_api.c (e la STESSA politica di validazione degli handle: indice
   non valido -> luaL_error, che il patto di sicurezza gia' implementato in
   script_sandbox.c traduce in "sandbox disabilitata"), ma rispondono con
   dati plausibili e fissi invece di leggere/scrivere un mondo di gioco
   vero. E' esattamente il "no-op che registra di essere stato chiamato"
   suggerito dal task brief: qui "registrare" e' implicito nel fatto che la
   chiamata non fa esplodere nulla, che e' tutto cio' che il dry-run deve
   verificare. */

#include "gen_lua.h"

#include "core/game_types.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
   API di gioco finta: un solo handle "valido" per enemy e per shot,
   cosi' uno script che usa CORRETTAMENTE gli argomenti delle sue callback
   (es. l'id restituito da nearest_enemy(), o shot_id/enemy_id ricevuti da
   on_hit) valida senza errori, mentre uno script che si inventa un numero
   (es. damage_enemy(999999, 10), il caso "handle stantio" del corpus di
   test) viene respinto esattamente come lo respingerebbe il gioco vero.
   ============================================================ */

#define GEN_LUA_FAKE_INDEX 1u
#define GEN_LUA_FAKE_GEN   1u
#define GEN_LUA_INDEX_BITS 16   /* stessa codifica "indice + generazione" di script_api.c: coincidenza voluta per restare plausibile, ma questa e' una codifica INDIPENDENTE, valida solo dentro questa sandbox usa-e-getta (melting-gen non vede mai gli handle veri che il gioco assegnera' a runtime). */

static double GenLuaFakeHandle(void)
{
    return (double)(((unsigned long long)GEN_LUA_FAKE_GEN << GEN_LUA_INDEX_BITS) | GEN_LUA_FAKE_INDEX);
}

static bool GenLuaHandleValid(double handle)
{
    return handle == GenLuaFakeHandle();
}

static double GenLuaCheckHandle(lua_State *L, int argIdx, const char *what)
{
    double handle = luaL_checknumber(L, argIdx);
    if (!GenLuaHandleValid(handle)) luaL_error(L, "handle %s non valido", what);
    return handle;
}

static int GenLuaStubPlayerX(lua_State *L)        { lua_pushnumber(L, 500.0); return 1; }
static int GenLuaStubPlayerY(lua_State *L)        { lua_pushnumber(L, 300.0); return 1; }
static int GenLuaStubPlayerHp(lua_State *L)       { lua_pushnumber(L, 6.0); return 1; }
static int GenLuaStubPlayerMaxHp(lua_State *L)    { lua_pushnumber(L, 6.0); return 1; }
static int GenLuaStubPlayerDamage(lua_State *L)   { lua_pushnumber(L, 8.0); return 1; }
static int GenLuaStubPlayerItemCount(lua_State *L){ lua_pushnumber(L, 3.0); return 1; }

static int GenLuaStubPlayerHasItem(lua_State *L)
{
    luaL_checkstring(L, 1);
    lua_pushboolean(L, 0);
    return 1;
}

static int GenLuaStubEnemyX(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "nemico");
    lua_pushnumber(L, 520.0);
    return 1;
}

static int GenLuaStubEnemyY(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "nemico");
    lua_pushnumber(L, 300.0);
    return 1;
}

static int GenLuaStubEnemyHp(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "nemico");
    lua_pushnumber(L, 10.0);
    return 1;
}

static int GenLuaStubShotX(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "colpo");
    lua_pushnumber(L, 500.0);
    return 1;
}

static int GenLuaStubShotY(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "colpo");
    lua_pushnumber(L, 300.0);
    return 1;
}

static int GenLuaStubNearestEnemy(lua_State *L)
{
    luaL_checknumber(L, 1);
    luaL_checknumber(L, 2);
    /* Sempre "trovato" (a differenza del gioco vero, dove puo' tornare nil):
       cosi' il dry-run esercita il ramo "id ~= nil", quello dove uno script
       tipicamente chiama enemy_x/enemy_y/enemy_hp/damage_enemy, che e' il ramo
       che vale davvero la pena validare. */
    lua_pushnumber(L, GenLuaFakeHandle());
    return 1;
}

static int GenLuaStubRoomLeft(lua_State *L)   { lua_pushnumber(L, (lua_Number)ROOM_X); return 1; }
static int GenLuaStubRoomTop(lua_State *L)    { lua_pushnumber(L, (lua_Number)ROOM_Y); return 1; }
static int GenLuaStubRoomRight(lua_State *L)  { lua_pushnumber(L, (lua_Number)ROOM_RIGHT); return 1; }
static int GenLuaStubRoomBottom(lua_State *L) { lua_pushnumber(L, (lua_Number)ROOM_BOTTOM); return 1; }

static int GenLuaStubSpawnShot(lua_State *L)
{
    for (int i = 1; i <= 7; i++) luaL_checknumber(L, i);
    luaL_optnumber(L, 8, 0.0);   /* traits, opzionale */
    lua_pushnumber(L, GenLuaFakeHandle());
    return 1;
}

static int GenLuaStubDamageEnemy(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "nemico");
    luaL_checknumber(L, 2);
    return 0;
}

static int GenLuaStubHealPlayer(lua_State *L)
{
    luaL_checknumber(L, 1);
    return 0;
}

static int GenLuaStubSetEnemyVelocity(lua_State *L)
{
    GenLuaCheckHandle(L, 1, "nemico");
    luaL_checknumber(L, 2);
    luaL_checknumber(L, 3);
    return 0;
}

static int GenLuaStubAddParticle(lua_State *L)
{
    luaL_checknumber(L, 1);
    luaL_checknumber(L, 2);
    luaL_optnumber(L, 3, 0.0);
    return 0;
}

static void GenLuaRegisterFn(lua_State *L, const char *name, lua_CFunction fn)
{
    lua_pushcfunction(L, fn);
    lua_setfield(L, -2, name);
}

static void GenLuaRegisterConst(lua_State *L, const char *name, unsigned int value)
{
    lua_pushnumber(L, (lua_Number)value);
    lua_setfield(L, -2, name);
}

/* Stessa lista di funzioni/costanti di ScriptApiRegister (script_api.c):
   se una manca qui, un modello che la usa fallirebbe la validazione anche
   se il gioco vero gliela concederebbe (falso negativo, innocuo ma
   fuorviante); se ce n'e' una IN PIU', il modello imparerebbe a fidarsi di
   una funzione che il gioco vero non ha (falso positivo, pericoloso: per
   questo l'elenco va tenuto sincronizzato a mano con script_api.c, non
   generato). */
static void GenLuaStubRegister(ScriptSandbox *sb)
{
    lua_State *L = ScriptSandboxRawState(sb);
    if (!L) return;
    lua_pushglobaltable(L);

    GenLuaRegisterFn(L, "player_x", GenLuaStubPlayerX);
    GenLuaRegisterFn(L, "player_y", GenLuaStubPlayerY);
    GenLuaRegisterFn(L, "player_hp", GenLuaStubPlayerHp);
    GenLuaRegisterFn(L, "player_max_hp", GenLuaStubPlayerMaxHp);
    GenLuaRegisterFn(L, "player_damage", GenLuaStubPlayerDamage);
    GenLuaRegisterFn(L, "player_item_count", GenLuaStubPlayerItemCount);
    GenLuaRegisterFn(L, "player_has_item", GenLuaStubPlayerHasItem);
    GenLuaRegisterFn(L, "enemy_x", GenLuaStubEnemyX);
    GenLuaRegisterFn(L, "enemy_y", GenLuaStubEnemyY);
    GenLuaRegisterFn(L, "enemy_hp", GenLuaStubEnemyHp);
    GenLuaRegisterFn(L, "shot_x", GenLuaStubShotX);
    GenLuaRegisterFn(L, "shot_y", GenLuaStubShotY);
    GenLuaRegisterFn(L, "nearest_enemy", GenLuaStubNearestEnemy);
    GenLuaRegisterFn(L, "room_left", GenLuaStubRoomLeft);
    GenLuaRegisterFn(L, "room_top", GenLuaStubRoomTop);
    GenLuaRegisterFn(L, "room_right", GenLuaStubRoomRight);
    GenLuaRegisterFn(L, "room_bottom", GenLuaStubRoomBottom);
    GenLuaRegisterFn(L, "spawn_shot", GenLuaStubSpawnShot);
    GenLuaRegisterFn(L, "damage_enemy", GenLuaStubDamageEnemy);
    GenLuaRegisterFn(L, "heal_player", GenLuaStubHealPlayer);
    GenLuaRegisterFn(L, "set_enemy_velocity", GenLuaStubSetEnemyVelocity);
    GenLuaRegisterFn(L, "add_particle", GenLuaStubAddParticle);

    GenLuaRegisterConst(L, "TRAIT_BOUNCE", TRAIT_BOUNCE);
    GenLuaRegisterConst(L, "TRAIT_HOMING", TRAIT_HOMING);
    GenLuaRegisterConst(L, "TRAIT_EXPLODE", TRAIT_EXPLODE);
    GenLuaRegisterConst(L, "TRAIT_SPLIT", TRAIT_SPLIT);
    GenLuaRegisterConst(L, "TRAIT_PIERCE", TRAIT_PIERCE);
    GenLuaRegisterConst(L, "TRAIT_RAPID", TRAIT_RAPID);
    GenLuaRegisterConst(L, "TRAIT_GIANT", TRAIT_GIANT);
    GenLuaRegisterConst(L, "TRAIT_SLOW", TRAIT_SLOW);
    GenLuaRegisterConst(L, "TRAIT_VAMP", TRAIT_VAMP);
    /* rng() e' gia' nell'_ENV: la mette ScriptSandboxCreate stesso (vedi
       script_sandbox.c, ScriptSandboxLuaRng), identica a quella del gioco. */

    lua_pop(L, 1);
}

/* ============================================================
   Validazione: sintassi (ScriptSandboxLoad) + dry-run di ogni callback
   definita, una volta, con argomenti plausibili.
   ============================================================ */

bool GenLuaValidate(const char *source, unsigned int seed, bool *anyCallback, char *err, size_t errSize)
{
    if (err && errSize) err[0] = '\0';
    if (anyCallback) *anyCallback = false;
    if (!source || !source[0])
    {
        if (err) snprintf(err, errSize, "script vuoto");
        return false;
    }
    if (strlen(source) >= (size_t)GEN_LUA_LEN - 1)
    {
        if (err) snprintf(err, errSize, "script troppo lungo (limite %d caratteri)", GEN_LUA_LEN - 2);
        return false;
    }

    ScriptSandbox *sb = ScriptSandboxCreate(seed ? seed : 1u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb)
    {
        if (err) snprintf(err, errSize, "impossibile creare la sandbox di validazione (memoria?)");
        return false;
    }
    GenLuaStubRegister(sb);

    if (!ScriptSandboxLoad(sb, "item-lua", source, err, errSize))
    {
        ScriptSandboxDestroy(sb);
        return false;
    }

    bool hasEval = ScriptSandboxHasFunction(sb, "on_evaluate");
    bool hasFire = ScriptSandboxHasFunction(sb, "on_fire");
    bool hasHit  = ScriptSandboxHasFunction(sb, "on_hit");
    bool hasTick = ScriptSandboxHasFunction(sb, "on_tick");
    if (anyCallback) *anyCallback = hasEval || hasFire || hasHit || hasTick;

    bool ok = true;
    if (hasEval)
    {
        /* Stessi valori di partenza di GameResetRun/MakeBaseGame (vedi
           src/tests/script_items_tests.c): plausibili, non i valori VERI di
           una run reale (che melting-gen non conosce), ma nello stesso
           ordine di grandezza che uno script scritto per questo gioco si
           aspetta di leggere. */
        lua_State *L = ScriptSandboxRawState(sb);
        lua_getglobal(L, "on_evaluate");
        lua_newtable(L);
        lua_pushnumber(L, 8.0);   lua_setfield(L, -2, "damage");
        lua_pushnumber(L, 0.23);  lua_setfield(L, -2, "fire_delay");
        lua_pushnumber(L, 520.0); lua_setfield(L, -2, "shot_speed");
        lua_pushnumber(L, 5.0);   lua_setfield(L, -2, "shot_radius");
        lua_pushnumber(L, 224.0); lua_setfield(L, -2, "speed");
        lua_pushnumber(L, 6.0);   lua_setfield(L, -2, "max_hp");
        ok = ScriptSandboxProtectedCall(sb, 1, 0);
    }
    if (ok && hasFire) ok = ScriptSandboxCallVoid(sb, "on_fire", 4, 480.0, 300.0, 1.0, 0.0);
    if (ok && hasHit)  ok = ScriptSandboxCallVoid(sb, "on_hit", 2, GenLuaFakeHandle(), GenLuaFakeHandle());
    if (ok && hasTick) ok = ScriptSandboxCallVoid(sb, "on_tick", 1, 0.016);

    if (!ok && err) snprintf(err, errSize, "%s", ScriptSandboxDisabledReason(sb));
    ScriptSandboxDestroy(sb);
    return ok;
}

/* ============================================================
   Generazione: prompt (cheat-sheet + few-shot + scheda oggetto) -> modello
   (senza grammatica, spec sezione 6) -> estrazione -> valida -> ritenta.
   ============================================================ */

#define GEN_LUA_MAX_ATTEMPTS 3   /* tentativo iniziale + 2 ritenti, vedi il task brief */
#define GEN_LUA_N_PREDICT 384    /* uno script piccolo: abbondante per "una sola sinergia" (vedi il prompt), tiene la fase dentro GEN_LUA_PHASE_BUDGET_SEC */
#define GEN_LUA_TEMP 0.6f        /* piu' bassa di quella del JSON (di norma 0.8): qui conta la correttezza sintattica/semantica, non la varieta' */

static void TrimWhitespace(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == '\t')) s[--len] = '\0';
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\n' || s[start] == '\r' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

/* I modelli da 7B tendono a incorniciare il codice in un blocco markdown
   nonostante l'istruzione esplicita di non farlo (vedi lua_system.txt):
   qui si estrae il contenuto fra i due delimitatori ``` (con o senza
   l'etichetta "lua" subito dopo il primo), altrimenti si usa l'intera
   risposta cosi' com'e'. */
static void ExtractLuaCode(const char *raw, char *out, size_t outCap)
{
    const char *fence = strstr(raw, "```");
    if (fence)
    {
        const char *body = fence + 3;
        if (strncmp(body, "lua", 3) == 0) body += 3;
        while (*body == '\n' || *body == '\r') body++;
        const char *closeFence = strstr(body, "```");
        size_t len = closeFence ? (size_t)(closeFence - body) : strlen(body);
        if (len >= outCap) len = outCap - 1;
        memcpy(out, body, len);
        out[len] = '\0';
    }
    else
    {
        /* Non snprintf(out, outCap, "%s", raw): 'raw' e' un buffer piu'
           grande di 'out' (4096 contro GEN_LUA_LEN), quindi il troncamento
           e' atteso e sicuro, ma -Wformat-truncation non puo' saperlo (vede
           solo le due dimensioni dichiarate) e lo segnala come se fosse un
           bug. Una copia bounded esplicita ottiene lo stesso troncamento
           sicuro senza il falso positivo. */
        size_t rawLen = strlen(raw);
        size_t copyLen = (rawLen < outCap - 1) ? rawLen : outCap - 1;
        memcpy(out, raw, copyLen);
        out[copyLen] = '\0';
    }
    TrimWhitespace(out);
}

static char *BuildLuaPrompt(const char *promptsDir, const char *floorTheme, const GenItem *item, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/lua_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    snprintf(path, sizeof(path), "%s/lua_user.txt", promptsDir);
    char *userTpl = GenReadFile(path);
    if (!sys || !userTpl) { free(sys); free(userTpl); return NULL; }

    char traitsText[64];
    traitsText[0] = '\0';
    size_t used = 0;
    for (int t = 0; t < item->traitCount; t++)
    {
        int n = snprintf(traitsText + used, sizeof(traitsText) - used, "%s%s", t > 0 ? ", " : "", item->traits[t]);
        if (n < 0 || (size_t)n >= sizeof(traitsText) - used) break;
        used += (size_t)n;
    }

    char *step1 = GenReplaceAll(userTpl, "{ITEM_NAME}", item->name);
    free(userTpl);
    char *step2 = step1 ? GenReplaceAll(step1, "{ITEM_SLOT}", item->slot) : NULL;
    free(step1);
    char *step3 = step2 ? GenReplaceAll(step2, "{ITEM_TRAITS}", traitsText) : NULL;
    free(step2);
    char *userFinal = step3 ? GenReplaceAll(step3, "{FLOOR_THEME}", floorTheme) : NULL;
    free(step3);
    if (!userFinal) { free(sys); return NULL; }

    if (prevError && prevError[0])
    {
        size_t cap = strlen(userFinal) + strlen(prevError) + 256;
        char *withRetry = malloc(cap);
        if (withRetry)
        {
            snprintf(withRetry, cap,
                "%s\n\nIl tuo script precedente non ha superato la validazione:\n%s\n"
                "Correggi SOLO il problema segnalato e riscrivi lo script Lua completo da capo, "
                "seguendo le stesse regole del cheat-sheet sopra.",
                userFinal, prevError);
            free(userFinal);
            userFinal = withRetry;
        }
    }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

void GenLuaGenerateForRun(GenLlmSession *sess, GenRun *run, const char *promptsDir,
                           const char *outDir, double deadline, GenLuaStats *stats)
{
    memset(stats, 0, sizeof(*stats));
    if (!sess) return;

    int itemNum = 0;
    const int totalItems = GEN_FLOORS*GEN_ITEMS;
    for (int f = 0; f < GEN_FLOORS; f++)
    {
        GenFloor *floor = &run->floors[f];
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            itemNum++;
            GenItem *item = &floor->items[i];
            item->lua[0] = '\0';

            if (GenNowSeconds() >= deadline)
            {
                stats->skippedBudget++;
                GenLogLine("lua: oggetto %d/%d (%s) saltato: budget di tempo della fase Lua esaurito",
                           itemNum, totalItems, item->name);
                continue;
            }

            char err[192];
            err[0] = '\0';
            char code[GEN_LUA_LEN];
            code[0] = '\0';
            bool success = false;
            bool optedOut = false;
            int attempt = 0;

            for (attempt = 0; attempt < GEN_LUA_MAX_ATTEMPTS && !success && !optedOut; attempt++)
            {
                if (GenNowSeconds() >= deadline) break;

                char *prompt = BuildLuaPrompt(promptsDir, floor->theme, item, attempt > 0 ? err : NULL);
                if (!prompt)
                {
                    snprintf(err, sizeof(err), "prompt Lua non costruibile (file mancanti in %s?)", promptsDir);
                    break;
                }

                char msg[112];
                snprintf(msg, sizeof(msg), "scrivo il Lua di %s (tentativo %d/%d)", item->name, attempt + 1, GEN_LUA_MAX_ATTEMPTS);
                GenProgressWrite(outDir, "lua", 92 + (6*itemNum)/totalItems, msg);

                char raw[4096];
                unsigned int callSeed = run->seed + (unsigned int)(itemNum*131u + (unsigned int)attempt*17u + 1u);
                int rc = GenLlmComplete(sess, prompt, NULL, GEN_LUA_N_PREDICT, GEN_LUA_TEMP, callSeed,
                                         outDir, "lua", 92 + (6*itemNum)/totalItems, 1,
                                         raw, sizeof(raw), NULL);
                free(prompt);
                if (rc != 0)
                {
                    snprintf(err, sizeof(err), "generazione fallita (decodifica o token troncati)");
                    continue;
                }

                char extracted[GEN_LUA_LEN];
                ExtractLuaCode(raw, extracted, sizeof(extracted));

                bool anyCallback = false;
                bool valid = GenLuaValidate(extracted, run->seed + (unsigned int)itemNum, &anyCallback, err, sizeof(err));
                if (valid && !anyCallback)
                {
                    optedOut = true;   /* sintassi ok, ma nessuna callback: il modello ha scelto di non proporre nulla */
                    break;
                }
                if (valid)
                {
                    snprintf(code, sizeof(code), "%s", extracted);
                    success = true;
                }
            }

            if (success)
            {
                snprintf(item->lua, sizeof(item->lua), "%s", code);
                if (attempt <= 1) stats->firstTry++; else stats->afterRetry++;
                GenLogLine("lua: oggetto %d/%d (%s) ok al tentativo %d/%d",
                           itemNum, totalItems, item->name, attempt, GEN_LUA_MAX_ATTEMPTS);
            }
            else if (optedOut)
            {
                stats->optedOut++;
                GenLogLine("lua: oggetto %d/%d (%s) nessun comportamento proposto dal modello",
                           itemNum, totalItems, item->name);
            }
            else
            {
                stats->fellBack++;
                GenLogLine("lua: oggetto %d/%d (%s) fallito dopo %d tentativi, ripiego mini-VM: %s",
                           itemNum, totalItems, item->name, attempt, err);
            }
        }
    }
    GenLogLine("lua: riepilogo run - %d/%d primo tentativo, %d dopo retry, %d nessun comportamento, %d ripiegati, %d saltati per budget",
               stats->firstTry, totalItems, stats->afterRetry, stats->optedOut, stats->fellBack, stats->skippedBudget);
}
