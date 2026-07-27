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

#include "gen_corpus.h"

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

/* Core condiviso fra GenLuaValidate (oggetti, sotto) e
   GenLuaValidateCharacterTrait (il trait del personaggio, M6b-2, piu' in
   basso): crea la sandbox, applica lo stub API, compila (ScriptSandboxLoad)
   e rileva quali delle quattro callback lo script definisce. NON applica
   nessun gate di dominio (quello resta specifico di ogni chiamante: gli
   oggetti hanno due categorie con regole opposte, il trait ne ha una terza,
   "esattamente una fra le quattro") e NON fa il dry-run (idem: un chiamante
   deve poter rifiutare uno script PRIMA di eseguirlo, se il suo gate lo
   boccia). Ritorna NULL su qualunque fallimento di caricamento (err gia'
   riempito), altrimenti la sandbox CARICATA (mai distrutta qui: il
   chiamante decide quando, dopo il proprio gate/dry-run). */
static ScriptSandbox *GenLuaValidateLoad(const char *source, unsigned int seed,
                                          bool *hasEval, bool *hasFire, bool *hasHit, bool *hasTick,
                                          char *err, size_t errSize)
{
    *hasEval = *hasFire = *hasHit = *hasTick = false;
    if (err && errSize) err[0] = '\0';
    if (!source || !source[0])
    {
        if (err) snprintf(err, errSize, "empty script");
        return NULL;
    }
    if (strlen(source) >= (size_t)GEN_LUA_LEN - 1)
    {
        if (err) snprintf(err, errSize, "script too long (limit %d characters)", GEN_LUA_LEN - 2);
        return NULL;
    }

    ScriptSandbox *sb = ScriptSandboxCreate(seed ? seed : 1u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb)
    {
        if (err) snprintf(err, errSize, "could not create the validation sandbox (out of memory?)");
        return NULL;
    }
    GenLuaStubRegister(sb);

    if (!ScriptSandboxLoad(sb, "item-lua", source, err, errSize))
    {
        ScriptSandboxDestroy(sb);
        return NULL;
    }

    *hasEval = ScriptSandboxHasFunction(sb, "on_evaluate");
    *hasFire = ScriptSandboxHasFunction(sb, "on_fire");
    *hasHit  = ScriptSandboxHasFunction(sb, "on_hit");
    *hasTick = ScriptSandboxHasFunction(sb, "on_tick");
    return sb;
}

/* Dry-run delle sole callback che il chiamante lascia passare (hasX=false =
   "non chiamarla", che sia perche' lo script non la definisce o perche' il
   gate del chiamante l'ha gia' scartata): stessi argomenti plausibili per
   ognuna, condivisi fra GenLuaValidate e GenLuaValidateCharacterTrait cosi'
   una futura modifica ai valori di prova (es. i sei campi di 'stats') non
   deve essere tenuta sincronizzata a mano in due punti. Non distrugge 'sb':
   il chiamante lo fa, dopo aver letto 'err' se ok e' falso. */
static bool GenLuaDryRun(ScriptSandbox *sb, bool hasEval, bool hasFire, bool hasHit, bool hasTick,
                          char *err, size_t errSize)
{
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
        lua_pushnumber(L, 0.0);   lua_setfield(L, -2, "luck");   /* step C: stessa tabella che il gioco passa davvero (script_items.c) */
        ok = ScriptSandboxProtectedCall(sb, 1, 0);
    }
    if (ok && hasFire) ok = ScriptSandboxCallVoid(sb, "on_fire", 4, 480.0, 300.0, 1.0, 0.0);
    if (ok && hasHit)  ok = ScriptSandboxCallVoid(sb, "on_hit", 2, GenLuaFakeHandle(), GenLuaFakeHandle());
    if (ok && hasTick) ok = ScriptSandboxCallVoid(sb, "on_tick", 1, 0.016);

    if (!ok && err) snprintf(err, errSize, "%s", ScriptSandboxDisabledReason(sb));
    return ok;
}

bool GenLuaValidate(const char *source, unsigned int seed, bool statUpOnly, bool *anyCallback, char *err, size_t errSize)
{
    if (anyCallback) *anyCallback = false;

    bool hasEval, hasFire, hasHit, hasTick;
    ScriptSandbox *sb = GenLuaValidateLoad(source, seed, &hasEval, &hasFire, &hasHit, &hasTick, err, errSize);
    if (!sb) return false;

    if (anyCallback) *anyCallback = hasEval || hasFire || hasHit || hasTick;

    /* Gate di dominio per gli oggetti stat-up (fase 3, vedi gen_lua.h sopra):
       NON e' una fuga della sandbox ne' un errore di sintassi, e' una regola
       di melting-gen su COSA puo' scrivere il modello per questa categoria
       di oggetto. Controllato PRIMA di eseguire alcunche' (dry-run incluso),
       cosi' un modello che ignora l'istruzione del prompt e propone
       comunque un comportamento viene sempre rimandato indietro con
       l'errore, non silenziosamente accettato. */
    if (statUpOnly && (hasFire || hasHit || hasTick))
    {
        ScriptSandboxDestroy(sb);
        if (err) snprintf(err, errSize, "a stat-up item can only define on_evaluate, no on_fire/on_hit/on_tick");
        return false;
    }

    /* Gate inverso (review, "active items can still write stat-scaling,
       dodging the per-item budget"): un oggetto ATTIVO che definisce
       on_evaluate valida oggi (nessun controllo lo impediva), ma a runtime
       (ScriptItemsRecomputeStats, src/script/script_items.c) un oggetto
       ATTIVO passa SOLO dal tetto GLOBALE, non dal tetto PER-OGGETTO
       (ScriptItemsClampItemDelta, riservato a ITEM_STATUP): puo' quindi
       spostare una statistica fino al doppio del budget di un boss reward
       in un colpo solo. Il modello lo fa per davvero quando ignora il
       cheat-sheet (lua_system.txt dice gia' "on_evaluate e' VIETATO per un
       oggetto attivo", ma e' solo testo nel prompt): l'ultima run reale ha
       spedito floor5_item2.lua = "stats.damage = stats.damage * 1.5" come
       oggetto del negozio. Qui si rifiuta lo script PRIMA del dry-run,
       rimandando l'errore al ciclo di retry (GenLuaGenerateOneItem sopra),
       cosi' il modello riscrive un comportamento vero invece di uno
       stat-up travestito. Solo lato generatore: la sandbox del gioco resta
       libera quanto prima (vedi il commento sopra
       SCRIPT_ITEMS_ITEM_DELTA_FRACTION in script_items.c), uno script
       scritto a mano puo' ancora usare on_evaluate su un oggetto attivo per
       una sinergia piu' ricca. */
    if (!statUpOnly && hasEval)
    {
        ScriptSandboxDestroy(sb);
        if (err) snprintf(err, errSize, "an active item cannot define on_evaluate (it's reserved for stat-up items): define EXACTLY one real behavior with on_fire/on_hit/on_tick, or no callback at all");
        return false;
    }

    bool ok = GenLuaDryRun(sb, hasEval, hasFire, hasHit, hasTick, err, errSize);
    ScriptSandboxDestroy(sb);
    return ok;
}

/* M6b-2 (DEC-037): il trait UNICO del personaggio generato -- STESSA
   pipeline di validazione degli oggetti (GenLuaValidateLoad/GenLuaDryRun
   sopra, la sandbox VERA del gioco, l'API stub, nessun ampliamento
   dell'allowlist _ENV), ma con un gate di dominio DIVERSO da statUpOnly
   vero/falso: la KB (characters.md, DEC-037) descrive il trait come "una
   sola fra on_fire/on_hit/on_tick, OPPURE un on_evaluate moderato" -- ne'
   il gate "solo on_evaluate" (statUpOnly=true, pensato per un bossItem che
   NON puo' mai avere un comportamento vero) ne' il suo opposto
   (statUpOnly=false, che vieta on_evaluate del tutto) esprimono questa
   forma: servono ENTRAMBE le famiglie, ma MAI piu' di una callback insieme
   (il trait e' "UNICO", non una lista). Il gate piu' adatto e' quindi un
   terzo, dedicato: "conta quante delle quattro callback lo script
   definisce, accetta solo esattamente 1" -- ne' 0 (un personaggio generato
   ha SEMPRE un tratto, a differenza di un oggetto: "nessuna idea buona,
   nessuna callback" non e' un'opzione valida qui) ne' 2+ (una combinazione
   sarebbe "un piccolo elenco di piccoli oggetti", esattamente cio' che la
   KB vuole evitare per il personaggio). */
bool GenLuaValidateCharacterTrait(const char *source, unsigned int seed, bool *anyCallback, char *err, size_t errSize)
{
    if (anyCallback) *anyCallback = false;

    bool hasEval, hasFire, hasHit, hasTick;
    ScriptSandbox *sb = GenLuaValidateLoad(source, seed, &hasEval, &hasFire, &hasHit, &hasTick, err, errSize);
    if (!sb) return false;

    int callbackCount = (hasEval ? 1 : 0) + (hasFire ? 1 : 0) + (hasHit ? 1 : 0) + (hasTick ? 1 : 0);
    if (anyCallback) *anyCallback = callbackCount > 0;
    if (callbackCount != 1)
    {
        ScriptSandboxDestroy(sb);
        if (err) snprintf(err, errSize, callbackCount == 0
            ? "the character trait must define exactly one behavior (on_evaluate, or exactly one of on_fire/on_hit/on_tick): none was found"
            : "the character trait must define exactly ONE behavior, never a combination (on_evaluate, or exactly one of on_fire/on_hit/on_tick)");
        return false;
    }

    bool ok = GenLuaDryRun(sb, hasEval, hasFire, hasHit, hasTick, err, errSize);
    ScriptSandboxDestroy(sb);
    return ok;
}

/* ============================================================
   Generazione: prompt (cheat-sheet + few-shot + scheda oggetto) -> modello
   (senza grammatica, spec sezione 6) -> estrazione -> valida -> ritenta.
   ============================================================ */

#define GEN_LUA_MAX_ATTEMPTS 3   /* tentativo iniziale + 2 ritenti, vedi il task brief */
/* GEN_LUA_N_PREDICT: vedi gen_lua.h (spostata li' in fase 3b review perche'
   GEN_LUA_PROMPT_BYTE_CEILING, la guardia byte-budget del prompt, la usa per
   calcolare quanti token restano al prompt dentro GEN_LLM_SESSION_N_CTX). */
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

/* 'isStatUp' sceglie SOLO il template utente (lua_user.txt vs
   lua_statup_user.txt, vedi prompts/): il cheat-sheet di sistema
   (lua_system.txt) resta condiviso, perche' l'API della sandbox e le
   funzioni proibite sono le stesse per ogni oggetto, attivo o stat-up (vedi
   la vision doc sezione 2). E' il TASK che cambia (un solo effetto semplice
   contro un ricalcolo di 1-2 statistiche), non le regole della sandbox.

   Estratta da BuildLuaPrompt/BuildLuaSuffix sotto (fase 3b step B1): SOLO la
   parte utente sostituita (placeholder + eventuale errore di retry), SENZA
   alcun wrapping ChatML -- helper condiviso dalle due viste dello stesso
   identico testo (il prompt combinato per il guard byte-budget, il solo
   suffisso per il percorso KV-riusato), per non doverle tenere sincronizzate
   a mano. Buffer malloc, NULL su file mancante o fallimento di
   sostituzione. */
static char *BuildLuaUserFinal(const char *promptsDir, const char *floorTheme, const GenItem *item, bool isStatUp, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", promptsDir, isStatUp ? "lua_statup_user.txt" : "lua_user.txt");
    char *userTpl = GenReadFile(path);
    if (!userTpl) return NULL;

    char traitsText[64];
    traitsText[0] = '\0';
    size_t used = 0;
    for (int t = 0; t < item->traitCount; t++)
    {
        int n = snprintf(traitsText + used, sizeof(traitsText) - used, "%s%s", t > 0 ? ", " : "", item->traits[t]);
        if (n < 0 || (size_t)n >= sizeof(traitsText) - used) break;
        used += (size_t)n;
    }

    /* Rarita' come intensita' (fase 3b design doc, sezione 2): l'indice
       torna -1 solo se item->rarity contiene un testo sconosciuto (non
       dovrebbe mai succedere, e' sempre GenRollRarity/GenNormalizeRun a
       scriverlo), in quel caso si ricade sulla frase COMUNE -- un prompt
       che spinge verso numeri piccoli e' l'errore piu' sicuro da fare. */
    int rarityIdx = GenRarityIndexFromText(item->rarity);
    const char *rarityHint = GEN_RARITY_PROMPT_HINTS[rarityIdx >= 0 ? rarityIdx : 0];

    char *step1 = GenReplaceAll(userTpl, "{ITEM_NAME}", item->name);
    free(userTpl);
    char *step2 = step1 ? GenReplaceAll(step1, "{ITEM_SLOT}", item->slot) : NULL;
    free(step1);
    char *step3 = step2 ? GenReplaceAll(step2, "{ITEM_TRAITS}", traitsText) : NULL;
    free(step2);
    char *step4 = step3 ? GenReplaceAll(step3, "{FLOOR_THEME}", floorTheme) : NULL;
    free(step3);
    char *userFinal = step4 ? GenReplaceAll(step4, "{ITEM_RARITY}", rarityHint) : NULL;
    free(step4);
    if (!userFinal) return NULL;

    if (prevError && prevError[0])
    {
        size_t cap = strlen(userFinal) + strlen(prevError) + 256;
        char *withRetry = malloc(cap);
        if (withRetry)
        {
            snprintf(withRetry, cap,
                "%s\n\nYour previous script failed validation:\n%s\n"
                "Fix ONLY the reported problem and rewrite the complete Lua script from scratch, "
                "following the same rules from the cheat-sheet above.",
                userFinal, prevError);
            free(userFinal);
            userFinal = withRetry;
        }
    }
    return userFinal;
}

/* Prompt ChatML combinato (prefisso+suffisso in un solo pezzo): usato oggi
   SOLO da GenLuaPromptBudgetCheck sotto (il guard "quanto puo' pesare al
   massimo il prompt Lua", che deve misurare la stessa cosa che il modello
   vede per davvero). Il percorso di generazione vero (GenLuaGenerateOneItem)
   NON lo chiama piu' (fase 3b step B1): usa BuildLuaPrefix + BuildLuaSuffix
   sotto, cosi' il prefisso (system+cheat-sheet, condiviso da tutti e 20 gli
   oggetti) si puo' decodificare una volta sola nella KV cache invece che ad
   ogni chiamata. Le tre funzioni condividono BuildLuaUserFinal sopra: questa
   resta l'esatta concatenazione byte-per-byte di BuildLuaPrefix(promptsDir)
   e BuildLuaSuffix(...) con gli stessi argomenti (GenChatMlWrap produce
   "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
   esattamente il prefisso seguito dal suffisso), quindi il guard byte-budget
   resta valido senza modifiche. */
static char *BuildLuaPrompt(const char *promptsDir, const char *floorTheme, const GenItem *item, bool isStatUp, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/lua_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    if (!sys) return NULL;

    char *userFinal = BuildLuaUserFinal(promptsDir, floorTheme, item, isStatUp, prevError);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

/* Prefisso ChatML condiviso da OGNI oggetto Lua della run (fase 3b step B1):
   "<|im_start|>system\n{lua_system.txt}<|im_end|>\n<|im_start|>user\n" --
   SOLO il cheat-sheet di sistema, mai un template utente (quello e'
   per-oggetto, vedi BuildLuaSuffix sotto). Uguale per i 20 oggetti di una
   run (lua_system.txt non ha placeholder), quindi GenLuaGenerateForRun lo
   costruisce e lo decodifica (GenLlmPrefixPrime) UNA volta sola, non per
   ogni oggetto. Buffer malloc, NULL su file mancante. */
static char *BuildLuaPrefix(const char *promptsDir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/lua_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    if (!sys) return NULL;

    size_t cap = strlen(sys) + 48;
    char *prefix = malloc(cap);
    if (prefix) snprintf(prefix, cap, "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n", sys);
    free(sys);
    return prefix;
}

/* Suffisso ChatML per-oggetto (fase 3b step B1): "{scheda dell'oggetto,
   BuildLuaUserFinal sopra}<|im_end|>\n<|im_start|>assistant\n" -- la parte
   che GenLlmCompleteFromPrefix decodifica a partire dalla posizione nPrefix
   gia' in cache (vedi GenLuaGenerateOneItem). Buffer malloc, NULL su file
   mancante o fallimento di sostituzione (stessi motivi di BuildLuaUserFinal,
   che fa il lavoro vero qui sotto). */
static char *BuildLuaSuffix(const char *promptsDir, const char *floorTheme, const GenItem *item, bool isStatUp, const char *prevError)
{
    char *userFinal = BuildLuaUserFinal(promptsDir, floorTheme, item, isStatUp, prevError);
    if (!userFinal) return NULL;

    size_t cap = strlen(userFinal) + 32;
    char *suffix = malloc(cap);
    if (suffix) snprintf(suffix, cap, "%s<|im_end|>\n<|im_start|>assistant\n", userFinal);
    free(userFinal);
    return suffix;
}

/* ============================================================
   M6b-2 (DEC-037): il prompt del trait del personaggio -- STESSO cheat-sheet
   di sistema (lua_system.txt, condiviso con gli oggetti: l'API della
   sandbox e le funzioni proibite sono le stesse), ma un template utente
   NUOVO e dedicato (prompts/lua_character_user.txt, placeholder
   {CHAR_NAME}/{CHAR_BLURB}) -- il trait non e' "un oggetto ATTIVO" ne' "un
   oggetto STAT-UP", e' il personaggio stesso, sempre attivo dall'inizio
   della run senza bisogno di raccoglierlo. Un solo oggetto alla volta (mai
   una run intera come per gli item Lua), quindi niente ottimizzazione
   prefisso/suffisso in KV-cache qui: GenLuaGenerateCharacterTrait sotto usa
   GenLlmComplete "a prompt intero", come RunProposeCharacter fa gia' per il
   JSON del personaggio (main.c) -- stesso stile del resto della fase
   propose, che non e' quella che genera i 20 script Lua di una run intera. */
static char *BuildCharacterTraitUserFinal(const char *promptsDir, const char *charName, const char *charBlurb, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/lua_character_user.txt", promptsDir);
    char *userTpl = GenReadFile(path);
    if (!userTpl) return NULL;

    char *step1 = GenReplaceAll(userTpl, "{CHAR_NAME}", charName);
    free(userTpl);
    char *userFinal = step1 ? GenReplaceAll(step1, "{CHAR_BLURB}", charBlurb) : NULL;
    free(step1);
    if (!userFinal) return NULL;

    /* Stesso schema di ritenti di BuildLuaUserFinal sopra (l'errore
       dell'ultimo tentativo rimandato al modello, invece di un secondo
       prompt duplicato da tenere sincronizzato). */
    if (prevError && prevError[0])
    {
        size_t cap = strlen(userFinal) + strlen(prevError) + 256;
        char *withRetry = malloc(cap);
        if (withRetry)
        {
            snprintf(withRetry, cap,
                "%s\n\nYour previous script failed validation:\n%s\n"
                "Fix ONLY the reported problem and rewrite the complete Lua script from scratch, "
                "following the same rules from the cheat-sheet above.",
                userFinal, prevError);
            free(userFinal);
            userFinal = withRetry;
        }
    }
    return userFinal;
}

/* Prompt ChatML combinato (system+cheat-sheet+scheda del personaggio): usato
   sia da GenLuaGenerateCharacterTrait sotto (un prompt intero per tentativo,
   vedi il commento sopra) sia da GenLuaPromptBudgetCheck (guardia
   byte-budget, worst-case). */
static char *BuildCharacterTraitPrompt(const char *promptsDir, const char *charName, const char *charBlurb, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/lua_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    if (!sys) return NULL;

    char *userFinal = BuildCharacterTraitUserFinal(promptsDir, charName, charBlurb, prevError);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

/* Vedi gen_lua.h per la spiegazione completa (perche' esiste, il bug reale
   che previene, la derivazione del ceiling). Riusa BuildLuaPrompt sopra
   (la STESSA funzione della generazione vera, non una reimplementazione)
   due volte -- una per categoria, attivo e stat-up, i due template utente
   possibili -- con un GenItem sintetico che rappresenta il caso peggiore
   su OGNI dimensione che puo' davvero variare:
   - rarity: l'hint PIU' LUNGO fra i quattro di GEN_RARITY_PROMPT_HINTS
     (gen_util.c) -- si scorre l'array invece di assumere quale sia oggi il
     piu' lungo, cosi' la guardia resta corretta anche se l'ordine di
     lunghezza cambiasse in futuro;
   - traits: i due piu' lunghi (non un trait qualunque): fino a 2 traits per
     oggetto (item->traitCount 1..2), uniti da BuildLuaPrompt con ", ";
   - name/floorTheme: NON i limiti di campo estremi (item->name e' 48 byte,
     ben oltre quanto un nome plausibile usa davvero: name/tema arrivano da
     JSON generato dal modello con la grammatica run.gbnf, namechar{3,40},
     o dal ripiego procedurale C, mai vicini al limite del campo), ma un
     contesto rappresentativo un po' sopra la media osservata nel golden
     file di ripiego (tests/melting-gen/golden-fallback-seed12345.txt) --
     "rappresentativo", come da task brief, non uno stress-test dei campi. */
bool GenLuaPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize)
{
    if (err && errSize) err[0] = '\0';

    int longestHint = 0;
    for (int i = 1; i < 4; i++)
    {
        if (strlen(GEN_RARITY_PROMPT_HINTS[i]) > strlen(GEN_RARITY_PROMPT_HINTS[longestHint])) longestHint = i;
    }

    GenItem worst;
    memset(&worst, 0, sizeof(worst));
    /* Nomi ri-tarati in inglese (DEC-052, 18/07): stessa taglia in byte dei
       precedenti worst-case italiani ("Lente del Vulcano Radioattivo" 29,
       "Laboratorio di Zucchero Radioattivo" 35), non un caso a se' -- vedi
       il commento sopra la funzione per il "perche'" di questa taglia. */
    snprintf(worst.name, sizeof(worst.name), "%s", "Lens of the Radioactive Volcano");
    snprintf(worst.slot, sizeof(worst.slot), "%s", "aura");
    snprintf(worst.traits[0], sizeof(worst.traits[0]), "%s", "explode");
    snprintf(worst.traits[1], sizeof(worst.traits[1]), "%s", "bounce");
    worst.traitCount = 2;
    snprintf(worst.rarity, sizeof(worst.rarity), "%s", GEN_RARITIES[longestHint]);
    const char *floorTheme = "Laboratory of the Radioactive Sugar";

    size_t worstBytes = 0;
    for (int statUp = 0; statUp <= 1; statUp++)
    {
        char *prompt = BuildLuaPrompt(promptsDir, floorTheme, &worst, statUp != 0, NULL);
        if (!prompt)
        {
            if (err) snprintf(err, errSize, "prompt Lua non costruibile (file mancanti in %s?)", promptsDir);
            return false;
        }
        size_t len = strlen(prompt);
        free(prompt);
        if (len > worstBytes) worstBytes = len;
    }

    /* M6b-2: worst-case anche per il trait del personaggio (nuovo template
       lua_character_user.txt, stesso cheat-sheet condiviso lua_system.txt
       di sopra) -- stesso principio del blocco appena sopra: un nome/blurb
       rappresentativo, non i limiti di campo estremi
       (CHARACTER_GEN_NAME_LEN=32/CHARACTER_GEN_BLURB_LEN=160, core/
       character_type.h). */
    char *charPrompt = BuildCharacterTraitPrompt(promptsDir,
        "The Radioactive Sugar Cartographer",
        "Grew up mapping molten caves nobody else dared enter, and still hums old work songs mid-fight.",
        NULL);
    if (!charPrompt)
    {
        if (err) snprintf(err, errSize, "prompt Lua del personaggio non costruibile (file mancanti in %s?)", promptsDir);
        return false;
    }
    size_t charBytes = strlen(charPrompt);
    free(charPrompt);
    if (charBytes > worstBytes) worstBytes = charBytes;

    if (worstBytes > (size_t)GEN_LUA_PROMPT_BYTE_CEILING)
    {
        if (err) snprintf(err, errSize,
            "prompt Lua composto = %zu byte, oltre il ceiling di %d (vedi GEN_LUA_PROMPT_BYTE_CEILING in gen_lua.h)",
            worstBytes, GEN_LUA_PROMPT_BYTE_CEILING);
        return false;
    }
    return true;
}

/* M6b-2 (DEC-037): il ciclo prompt->modello->valida->ritenta per il trait
   UNICO del personaggio generato -- stesso schema di
   GenLuaGenerateOneItem sotto (fino a GEN_LUA_MAX_ATTEMPTS tentativi,
   errore rimandato al modello ad ogni ritento), ma per UN solo script (non
   20 per una run intera), quindi GenLlmComplete "a prompt intero" invece
   della coppia prefix-prime/complete-from-prefix (vedi il commento sopra
   BuildCharacterTraitPrompt) -- e senza alcun ripiego C: se lo script non
   valida entro i tentativi, o il budget di tempo scade, la funzione ritorna
   false e 'outLua' resta vuoto -- il chiamante (RunProposeCharacter, main.c)
   NON scrive character_proposal.json in quel caso (KB: trait invalido =
   personaggio invalido, carta assente).

   Corpus (fase 3, gen_corpus.h): NON registrato qui, di proposito, stesso
   motivo per cui l'intera fase propose non lo fa (vedi il commento su
   RunProposeThemes in main.c, "MAI corpus/manifest/atlas/provenienza"):
   propose-themes esiste per essere leggero e non-bloccante mentre il
   giocatore gira nell'hub, non per popolare il corpus di training. */
bool GenLuaGenerateCharacterTrait(GenLlmSession *sess, const char *promptsDir, unsigned int seed,
                                   const char *charName, const char *charBlurb, double deadline,
                                   char *outLua, size_t outLuaSize, GenLuaStats *stats)
{
    memset(stats, 0, sizeof(*stats));
    if (outLuaSize) outLua[0] = '\0';
    if (!sess) return false;

    char err[192];
    err[0] = '\0';
    for (int attempt = 0; attempt < GEN_LUA_MAX_ATTEMPTS; attempt++)
    {
        if (GenNowSeconds() >= deadline)
        {
            stats->skippedBudget++;
            GenLogLine("lua-character: budget di fase esaurito al tentativo %d/%d, nessun trait", attempt + 1, GEN_LUA_MAX_ATTEMPTS);
            return false;
        }

        char *prompt = BuildCharacterTraitPrompt(promptsDir, charName, charBlurb, attempt > 0 ? err : NULL);
        if (!prompt)
        {
            snprintf(err, sizeof(err), "prompt Lua del personaggio non costruibile (file mancanti in %s?)", promptsDir);
            break;
        }

        char raw[4096];
        int tokens = 0;
        unsigned int callSeed = seed + (unsigned int)attempt*17u + 1u;
        int rc = GenLlmComplete(sess, prompt, NULL, GEN_LUA_N_PREDICT, GEN_LUA_TEMP, callSeed,
                                 NULL, NULL, 0, 0, raw, sizeof(raw), &tokens);
        free(prompt);
        if (rc != 0)
        {
            snprintf(err, sizeof(err), "generazione fallita (decodifica o token troncati)");
            GenLogLine("lua-character: tentativo %d/%d fallito: %s", attempt + 1, GEN_LUA_MAX_ATTEMPTS, err);
            continue;
        }

        char extracted[GEN_LUA_LEN];
        ExtractLuaCode(raw, extracted, sizeof(extracted));

        bool anyCallback = false;
        bool valid = GenLuaValidateCharacterTrait(extracted, seed, &anyCallback, err, sizeof(err));
        if (valid)
        {
            snprintf(outLua, outLuaSize, "%s", extracted);
            if (attempt == 0) stats->firstTry++; else stats->afterRetry++;
            GenLogLine("lua-character: trait ok al tentativo %d/%d", attempt + 1, GEN_LUA_MAX_ATTEMPTS);
            return true;
        }
        GenLogLine("lua-character: tentativo %d/%d respinto: %s", attempt + 1, GEN_LUA_MAX_ATTEMPTS, err);
    }

    stats->fellBack++;
    GenLogLine("lua-character: nessun trait valido dopo %d tentativi (%s): nessuna carta questa run", GEN_LUA_MAX_ATTEMPTS, err);
    return false;
}

/* Corpo del ciclo prompt->modello->valida->ritenta per UN oggetto (attivo o
   stat-up): prima viveva inline dentro il doppio for di GenLuaGenerateForRun,
   estratto qui perche' ora va eseguito 4 volte per piano (3 attivi + il
   bossItem) invece di 3, con solo il template utente/il gate statUpOnly che
   cambiano fra le due categorie (vedi BuildLuaPrompt/GenLuaValidate sopra).

   'nPrefix' (fase 3b step B1): il prefisso ChatML condiviso (system+cheat-
   sheet) e' gia' stato decodificato UNA volta nella KV cache da
   GenLuaGenerateForRun (GenLlmPrefixPrime) prima di chiamare questa funzione
   per il primo oggetto. Qui si costruisce e decodifica SOLO il suffisso
   per-oggetto (BuildLuaSuffix + GenLlmCompleteFromPrefix, invece di
   BuildLuaPrompt + GenLlmComplete: quest'ultima coppia ridecodificherebbe da
   capo anche il prefisso, esattamente il costo che questa fase vuole
   evitare) e si riavvolge la cache (GenLlmRewindToPrefix) dopo OGNI
   tentativo -- successo, fallimento di validazione o fallimento di
   decodifica non importa: il prossimo tentativo (retry con l'errore
   rimandato indietro) o il prossimo oggetto deve sempre ripartire dallo
   stesso identico prefisso, mai da quello allungato dal tentativo appena
   fatto. */
/* Step B2: gli script gia' presenti su disco da una generazione precedente. Il
   percorso e' lo STESSO che scrive gen_manifest.c (WriteItemLua): se cambia li',
   va cambiato anche qui -- sono le due meta' della stessa convenzione. */
int GenLuaLoadExisting(GenRun *run, const char *outDir)
{
    int loaded = 0;
    if (!run || !outDir) return 0;

    for (int f = 0; f < GEN_FLOORS; f++)
    {
        for (int i = 0; i <= GEN_ITEMS; i++)   /* <= : l'ultimo giro e' il bossItem */
        {
            GenItem *item = (i < GEN_ITEMS) ? &run->floors[f].items[i] : &run->floors[f].bossItem;
            char path[512];
            if (i < GEN_ITEMS) snprintf(path, sizeof(path), "%s/scripts/floor%d_item%d.lua", outDir, f + 1, i + 1);
            else snprintf(path, sizeof(path), "%s/scripts/floor%d_bossItem.lua", outDir, f + 1);

            char *text = GenReadFile(path);
            if (!text) continue;
            snprintf(item->lua, sizeof(item->lua), "%s", text);
            free(text);
            loaded++;
        }
    }
    return loaded;
}

static void GenLuaGenerateOneItem(GenLlmSession *sess, const char *promptsDir, const char *outDir,
                                   double deadline, int nPrefix, unsigned int runSeed, const char *floorTheme,
                                   GenItem *item, bool isStatUp, int itemNum, int totalItems, GenLuaStats *stats)
{
    /* Step B2: script gia' presente (caricato da disco da GenLuaLoadExisting in
       una ripresa) -> non si rigenera. Non e' solo un'ottimizzazione: e' cio' che
       rende la ripresa IDEMPOTENTE, e che impedisce a un secondo processo di
       sovrascrivere con un tentativo peggiore uno script gia' validato. */
    if (item->lua[0] != '\0')
    {
        stats->alreadyDone++;
        return;
    }

    if (GenNowSeconds() >= deadline)
    {
        stats->skippedBudget++;
        GenLogLine("lua: oggetto %d/%d (%s) saltato: budget di tempo della fase Lua esaurito",
                   itemNum, totalItems, item->name);
        return;
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

        char *suffix = BuildLuaSuffix(promptsDir, floorTheme, item, isStatUp, attempt > 0 ? err : NULL);
        if (!suffix)
        {
            snprintf(err, sizeof(err), "prompt Lua non costruibile (file mancanti in %s?)", promptsDir);
            break;
        }

        char msg[112];
        snprintf(msg, sizeof(msg), "scrivo il Lua di %s (tentativo %d/%d)", item->name, attempt + 1, GEN_LUA_MAX_ATTEMPTS);
        GenProgressWrite(outDir, "lua", 92 + (6*itemNum)/totalItems, msg);

        char raw[4096];
        unsigned int callSeed = runSeed + (unsigned int)(itemNum*131u + (unsigned int)attempt*17u + 1u);
        int rc = GenLlmCompleteFromPrefix(sess, nPrefix, suffix, GEN_LUA_N_PREDICT, GEN_LUA_TEMP, callSeed,
                                           outDir, "lua", 92 + (6*itemNum)/totalItems, 1,
                                           raw, sizeof(raw), NULL);
        free(suffix);
        /* Riavvolgi SEMPRE, prima di guardare rc: il prossimo tentativo (o
           il prossimo oggetto) deve ripartire dal solo prefisso condiviso
           indipendentemente da come e' andato questo. */
        GenLlmRewindToPrefix(sess, nPrefix);
        if (rc != 0)
        {
            snprintf(err, sizeof(err), "generazione fallita (decodifica o token troncati)");
            GenCorpusRecordLua(floorTheme, item->name, isStatUp, attempt + 1, "decode-failed", err, NULL);
            continue;
        }

        char extracted[GEN_LUA_LEN];
        ExtractLuaCode(raw, extracted, sizeof(extracted));

        bool anyCallback = false;
        bool valid = GenLuaValidate(extracted, runSeed + (unsigned int)itemNum, isStatUp, &anyCallback, err, sizeof(err));
        GenCorpusRecordLua(floorTheme, item->name, isStatUp, attempt + 1,
                            valid ? (anyCallback ? "ok" : "opted-out") : "invalid",
                            valid ? NULL : err, extracted);
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
        GenLogLine("lua: oggetto %d/%d (%s%s) ok al tentativo %d/%d",
                   itemNum, totalItems, item->name, isStatUp ? ", stat-up" : "", attempt, GEN_LUA_MAX_ATTEMPTS);
    }
    else if (optedOut)
    {
        stats->optedOut++;
        GenLogLine("lua: oggetto %d/%d (%s%s) nessun comportamento proposto dal modello",
                   itemNum, totalItems, item->name, isStatUp ? ", stat-up" : "");
    }
    else
    {
        stats->fellBack++;
        GenLogLine("lua: oggetto %d/%d (%s%s) fallito dopo %d tentativi, ripiego mini-VM/fallback C: %s",
                   itemNum, totalItems, item->name, isStatUp ? ", stat-up" : "", attempt, err);
    }
}

void GenLuaGenerateForRun(GenLlmSession *sess, GenRun *run, const char *promptsDir,
                           const char *outDir, double deadline, int firstFloors,
                           bool publishPerFloor, GenLuaStats *stats)
{
    memset(stats, 0, sizeof(*stats));
    if (!sess) return;
    if (firstFloors < 0) firstFloors = 0;
    if (firstFloors > GEN_FLOORS) firstFloors = GEN_FLOORS;

    const int totalItems = firstFloors*(GEN_ITEMS + 1);   /* +1: il bossItem stat-up di ogni piano (fase 3) */
    if (totalItems == 0) return;

    /* Fase 3b step B1: il prefisso ChatML condiviso (system+cheat-sheet,
       ~3700 token, quasi il n_ctx=4096 della sessione) si decodifica QUI,
       UNA volta sola per l'intera run, invece che dentro ogni tentativo di
       ogni oggetto (era la causa misurata di ~9.6s dei circa 11s spesi per
       oggetto: vedi il log "llm: completamento fase=lua" prima di questo
       cambio, prompt~9.6s contro generazione~0.5-2s). Se anche solo questa
       decodifica fallisce (file di prompt mancanti, o il prefisso da solo
       satura n_ctx) nessun oggetto di questa run puo' avere uno script Lua:
       tutti restano sulla sola mini-VM, come se 'sess' fosse NULL, ma qui e'
       un fallimento vero quindi lo si conta nel riepilogo (fellBack per
       tutti) invece di restare silenzioso. */
    char *prefix = BuildLuaPrefix(promptsDir);
    int nPrefix = 0;
    if (!prefix || GenLlmPrefixPrime(sess, prefix, &nPrefix) != 0)
    {
        GenLogLine("lua: prefisso condiviso non decodificabile (file mancanti in %s, o prompt troppo grande): nessuno script Lua per questa run", promptsDir);
        free(prefix);
        stats->fellBack = totalItems;
        return;
    }
    free(prefix);

    int itemNum = 0;
    for (int f = 0; f < firstFloors; f++)
    {
        GenFloor *floor = &run->floors[f];
        for (int i = 0; i < GEN_ITEMS; i++)
        {
            itemNum++;
            /* Task "4 categorie": items[] non e' piu' sempre "attivo" nel
               senso del prompt Lua (on_fire/on_hit/on_tick) -- un oggetto di
               questo slot puo' essere kind=statup (DEC-035, "gli stat-up
               compaiono anche nei pool normali"), che nel dominio del
               cheat-sheet Lua e' l'ALTRA categoria (solo on_evaluate, mai un
               comportamento): instradarlo comunque sul template/gate
               "attivo" gli farebbe scrivere on_fire/on_hit/on_tick per un
               oggetto la cui riga ".script=" WriteManifest non scrivera' mai
               (gen_manifest.c decide in base al kind, non a opCount -- vedi
               il commento li'), sprecando il tentativo su un comportamento
               che il giocatore non vedra' comunque. */
            bool isStatUpSlot = strcmp(floor->items[i].kind, "statup") == 0;
            GenLuaGenerateOneItem(sess, promptsDir, outDir, deadline, nPrefix, run->seed, floor->theme,
                                   &floor->items[i], isStatUpSlot, itemNum, totalItems, stats);
        }
        itemNum++;
        GenLuaGenerateOneItem(sess, promptsDir, outDir, deadline, nPrefix, run->seed, floor->theme,
                               &floor->bossItem, true, itemNum, totalItems, stats);

        /* Step B2: il piano e' completo -> lo si PUBBLICA subito (manifest +
           file .lua, scrittura atomica tmp+rename come sempre) invece di
           aspettare la fine di tutti e cinque. Il gioco, che sta gia' girando,
           raccoglie gli script di questo piano appena ci entra
           (RunContentRefreshFloorScripts). L'atlas.path esistente viene
           PRESERVATO: melting-sprites potrebbe averlo gia' fatto puntare al PNG,
           e ricostruirlo alla cieca riporterebbe la run all'atlas BMP di riserva
           (vedi GenWriteRunFilesResume in gen_manifest.c). */
        if (publishPerFloor)
        {
            if (GenWriteRunFilesResume(run, outDir) == 0)
            {
                GenLogLine("lua: piano %d pubblicato (manifest aggiornato, il gioco puo' raccoglierlo)", f + 1);
            }
            else
            {
                GenLogLine("lua: piano %d generato ma la pubblicazione del manifest e' fallita", f + 1);
            }
        }
    }
    GenLogLine("lua: riepilogo - %d/%d primo tentativo, %d dopo retry, %d nessun comportamento, %d ripiegati, %d saltati per budget, %d gia' fatti",
               stats->firstTry, totalItems, stats->afterRetry, stats->optedOut, stats->fellBack, stats->skippedBudget, stats->alreadyDone);
}
