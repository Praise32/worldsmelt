#include "script/script_sandbox.h"

#include "core/game_math.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ScriptSandbox
{
    lua_State *L;
    size_t memCap;
    size_t memUsed;
    unsigned int rngState;
    bool disabled;
    char disabledReason[160];
    char name[64];         /* nome dell'ultima sorgente caricata, per i log */
};

/* ============================================================
   Barriera 2 (spec, sezione 3): allocatore con tetto di memoria
   ============================================================ */

static void *ScriptSandboxAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    ScriptSandbox *sb = (ScriptSandbox *)ud;

    /* TRAPPOLA (verificata nella ricerca a monte della spec, non a memoria):
       quando ptr == NULL, 'osize' NON e' la dimensione di un blocco
       precedente (che non esiste) ma un CODICE DI TIPO (LUA_TSTRING,
       LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA o LUA_TTHREAD) che dice
       all'allocatore quale genere di oggetto GC Lua sta per creare (vedi il
       manuale ufficiale, voce lua_Alloc: "quando ptr e' NULL, osize codifica
       il tipo di oggetto che Lua sta allocando"). Sottrarre quel valore
       dalla contabilita' "cosi' com'e'" e' il bug classico di questo
       pattern: sottrarrebbe 3-8 byte inventati ad ogni singola stringa,
       tabella, funzione, userdata o thread creati, un errore piccolo ma
       sistematico che nel tempo fa credere alla sandbox di avere piu'
       margine di budget di quanto ne abbia davvero (memUsed scende sotto il
       valore reale). La dimensione vecchia vera, quando ptr == NULL, e'
       sempre 0: non c'era alcun blocco prima. */
    size_t oldSize = (ptr != NULL) ? osize : 0;

    if (nsize == 0)
    {
        /* Contratto di lua_Alloc: frealloc(ud, p, x, 0) libera 'p' (anche se
           p e' gia' NULL, nel qual caso non fa nulla, come free(NULL)). */
        free(ptr);
        if (ptr != NULL) sb->memUsed -= oldSize;
        return NULL;
    }

    /* Rifiuta PRIMA di toccare realloc: una singola richiesta piu' grande
       dell'intero tetto non deve nemmeno tentare l'allocazione reale (e non
       deve nemmeno rischiare l'overflow calcolando memUsed + delta quando
       delta e' gia' enorme). Restituire NULL qui e' sicuro: Lua tenta una
       garbage collection d'emergenza e ci richiama una seconda volta con
       gli stessi argomenti; se falliamo di nuovo solleva un errore Lua vero
       (LUA_ERRMEM), catturabile da lua_pcall come qualunque altro errore. */
    if (nsize > sb->memCap) return NULL;
    if (nsize > oldSize && sb->memUsed + (nsize - oldSize) > sb->memCap) return NULL;

    void *newPtr = realloc(ptr, nsize);
    if (newPtr == NULL) return NULL;

    sb->memUsed = sb->memUsed - oldSize + nsize;
    return newPtr;
}

/* Ultima istanza, difesa in profondita': non dovrebbe MAI scattare, perche'
   ogni singola chiamata a codice Lua in questo file (compilazione a parte,
   che non esegue nulla) passa da lua_pcall, che intercetta qualunque errore
   prima che arrivi qui. La si installa comunque: se un giorno una fuga non
   ancora scoperta facesse arrivare un errore non protetto, meglio un log
   con l'ultima parola che un crash silenzioso. */
static int ScriptSandboxPanic(lua_State *L)
{
    const char *msg = lua_tostring(L, -1);
    ScriptSandboxLogLine("PANIC (errore Lua sfuggito a lua_pcall, non dovrebbe accadere): %s",
                         msg ? msg : "?");
    return 0;
}

/* ============================================================
   Barriera 3 (spec, sezione 3): hook di conteggio istruzioni
   ============================================================ */

/* Identico nello spirito a lstop() nell'interprete di riferimento lua.c:
   toglie l'hook (per non richiamarsi da solo mentre srotola l'errore) e
   solleva un errore Lua con luaL_error, che lua_pcall cattura come un
   qualunque errore a runtime. */
static void ScriptSandboxCountHook(lua_State *L, lua_Debug *ar)
{
    (void)ar;
    lua_sethook(L, NULL, 0, 0);
    luaL_error(L, "budget di istruzioni superato");
}

/* ============================================================
   La RNG del gioco esposta come funzione Lua (sostituisce
   math.random/randomseed, esclusi per nondeterminismo)
   ============================================================ */

static int ScriptSandboxLuaRng(lua_State *L)
{
    ScriptSandbox *sb = (ScriptSandbox *)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushnumber(L, (lua_Number)GameRngFloat(&sb->rngState, 0.0f, 1.0f));
    return 1;
}

/* ============================================================
   Costruzione di _ENV da zero (spec, sezione 4)
   ============================================================ */

/* Le luaopen_* di Lua non falliscono mai in condizioni normali (creano solo
   una tabella e ci infilano dentro delle funzioni C), ma la chiamata resta
   comunque protetta da lua_pcall per lo stesso motivo per cui lo e' tutto
   il resto in questo file: qui dentro "non dovrebbe fallire" non e' una
   licenza per saltare la protezione. */
static bool ScriptSandboxOpenLib(lua_State *L, lua_CFunction openf)
{
    lua_pushcfunction(L, openf);
    return lua_pcall(L, 0, 1, 0) == LUA_OK;
}

/* Installa nell'_ENV vero (indice 'envIdx' dello stato 'L') SOLO le
   funzioni della libreria base elencate in 'allow', usando uno stato Lua
   SEPARATO e usa-e-getta per aprire luaopen_base.

   Il motivo per cui serve un intero stato a parte, e non semplicemente
   "apri in una tabella e copia i nomi buoni" come per math/table piu'
   sotto, e' un dettaglio dell'implementazione di luaopen_base che e' stato
   scoperto DAVVERO facendo girare il test dell'escape 4 (non a tavolino):
   a differenza di luaopen_math/luaopen_table (che creano una tabella
   fresca con luaL_newlib), luaopen_base fa
     lua_pushglobaltable(L); luaL_setfuncs(L, base_funcs, 0);
   cioe' installa OGNI funzione (pcall, load, setmetatable, rawset,
   collectgarbage, tutto) DIRETTAMENTE nella tabella globale corrente,
   perche' deve anche poter far puntare _G alla vera tabella globale.
   Chiamarla sul nostro stato reale significherebbe popolare il nostro
   _ENV vero con l'intera libreria base PRIMA ancora di poter scegliere
   cosa copiare: "copiare solo i nomi buoni dopo" non toglie le funzioni
   cattive gia' scritte li' dentro dalla chiamata stessa. La prima
   versione di questo file aveva esattamente questo bug: l'escape 4
   ("while true do pcall(f) end") girava all'infinito, perche' pcall
   esisteva davvero nell'_ENV. Aprendo invece luaopen_base in uno stato
   usa-e-getta, "la tabella globale corrente" e' quella dello stato usa-e-
   getta, irrilevante per noi: si estraggono solo i puntatori a funzione C
   dell'allowlist (un lua_CFunction e' codice puro, senza upvalue proprie,
   perfettamente riusabile in un altro lua_State) e si chiude subito lo
   stato temporaneo. */
static void ScriptSandboxInstallBaseSubset(lua_State *L, int envIdx, const char *const *allow)
{
    lua_State *scratch = luaL_newstate();
    if (scratch == NULL) return;   /* niente memoria nemmeno per lo stato usa-e-getta: base resta assente, non e' un crash */

    lua_pushcfunction(scratch, luaopen_base);
    if (lua_pcall(scratch, 0, 1, 0) == LUA_OK)
    {
        int baseIdx = lua_gettop(scratch);
        for (int i = 0; allow[i] != NULL; i++)
        {
            lua_getfield(scratch, baseIdx, allow[i]);
            lua_CFunction fn = lua_tocfunction(scratch, -1);
            lua_pop(scratch, 1);
            if (fn != NULL)
            {
                lua_pushcfunction(L, fn);
                lua_setfield(L, envIdx, allow[i]);
            }
        }
    }
    lua_close(scratch);
}

static void ScriptSandboxBuildEnv(ScriptSandbox *sb)
{
    lua_State *L = sb->L;

    /* lua_newstate crea GIA' una tabella (vuota) all'indice
       LUA_RIDX_GLOBALS del registro, indipendentemente da quali librerie
       vengano aperte: e' cosi' che _ENV di ogni chunk caricato viene
       risolto automaticamente (lua_load imposta il primo upvalue del
       chunk su questa stessa tabella, vedi lapi.c). Non serve quindi
       nessun trucco con lua_setupvalue: basta popolare QUESTA tabella con
       esattamente cio' che vogliamo esporre, e ogni script la vedra' come
       il proprio _ENV, "vuoto" tranne per quello che mettiamo qui. */
    lua_pushglobaltable(L);
    int envIdx = lua_gettop(L);

    /* base: a differenza di math/table, la libreria base non ha un
       sottoinsieme "gia' sicuro": mescola in un'unica tabella piatta
       funzioni innocue (ipairs, type...) e funzioni pericolosissime
       (pcall, load, setmetatable, rawset, collectgarbage...). Si apre
       l'intera libreria in una tabella SCARTATA e si copiano nell'_ENV
       vero SOLO i nomi dell'allowlist: e' l'inverso del trattamento di
       math/table (li' si toglie il poco di cattivo, qui si prende il
       poco di buono). Elenco completo di cio' che resta fuori e perche',
       spec sezione 4:
       - collectgarbage: espone dettagli del GC e puo' forzare una
         collezione fuori dal nostro controllo del budget.
       - dofile/loadfile/load: caricano ed eseguono codice arbitrario da
         file o stringa, bypassando ogni controllo "solo testo" fatto qui.
       - getmetatable/setmetatable/rawget/rawset/rawequal/rawlen: bypassano
         qualunque protezione futura basata su metatabelle (il layer di
         gioco, prossimo task, user usera' metatabelle per gli handle) e
         permettono di leggere/scrivere la metatabella condivisa delle
         stringhe (vedi il commento piu' sotto sul perche' string non
         compare affatto in questo file).
       - pcall/xpcall: intercettano l'errore sollevato dal nostro hook di
         conteggio istruzioni (spec sezione 2, punto 4): uno script con
         "while true do pcall(f) end" sopravvivrebbe per sempre al budget.
       - print/warn: I/O verso stdout/stderr, non necessario a uno script
         di gameplay e comunque fuori dall'allowlist esplicita.
       - next: pairs() lo copre gia'; esporlo a parte non aggiunge nulla e
         allarga la superficie senza motivo. */
    static const char *const baseAllow[] = {
        "ipairs", "pairs", "type", "tonumber", "tostring", "select", "error", "assert", NULL
    };
    ScriptSandboxInstallBaseSubset(L, envIdx, baseAllow);

    /* math: libreria gia' innocua, tranne random/randomseed (fonte di
       nondeterminismo: due run con lo stesso seed di gioco darebbero
       sequenze diverse). Qui il sottoinsieme sicuro e' "tutto tranne due
       chiavi", quindi si tiene l'intera tabella (fresca, mai condivisa con
       nient'altro: a differenza delle stringhe, math non ha un tipo Lua
       primitivo con una metatabella globale da difendere) e si azzerano
       solo le due chiavi pericolose. */
    if (ScriptSandboxOpenLib(L, luaopen_math))
    {
        lua_pushnil(L); lua_setfield(L, -2, "random");
        lua_pushnil(L); lua_setfield(L, -2, "randomseed");
        lua_setfield(L, envIdx, "math");   /* pop della tabella math */
    }
    else lua_settop(L, envIdx);

    /* table: sottoinsieme sicuro. ESCLUSO table.sort, di proposito, oltre
       a quanto gia' richiesto dalla spec: la sua difesa anti-caso-
       patologico (randomizzare il pivot quando l'input sembra costruito
       ad arte per far degenerare il quicksort, vedi ltablib.c,
       l_randomizePivot) chiama luaL_makeseed(), che pesca entropia da
       orologio di sistema e indirizzi di memoria. Per un interprete Lua
       generico e' un'ottima difesa; per QUESTA sandbox e' l'esatto
       contrario di cio' che serve: una sorgente di nondeterminismo
       nascosta dentro una libreria altrimenti innocua. Il numero di
       confronti (e quindi l'ordine osservabile delle chiamate, se il
       comparatore ha effetti collaterali) potrebbe differire fra due run
       con lo stesso seed di gioco, che e' esattamente la proprieta' che
       tutta questa sandbox esiste per garantire (spec sezione 3). Le
       restanti funzioni di table sono pure manipolazioni dato-a-dato,
       senza alcuna sorgente di entropia propria. */
    if (ScriptSandboxOpenLib(L, luaopen_table))
    {
        lua_pushnil(L); lua_setfield(L, -2, "sort");
        lua_setfield(L, envIdx, "table");
    }
    else lua_settop(L, envIdx);

    /* Nessuna libreria 'string' viene mai aperta, nemmeno per un
       sottoinsieme filtrato. Motivo (spec sezione 2, punto 2 e sezione 4):
       luaopen_string, oltre a restituire la tabella delle funzioni,
       installa come EFFETTO COLLATERALE INTERNO una metatabella condivisa
       su TUTTI i valori stringa dello stato (createmetatable in lstrlib.c fa
       metatable.__index = la tabella string COMPLETA, quella vera, non un
       nostro sottoinsieme filtrato). Un chunk potrebbe raggiungere quella
       tabella completa (find/gsub/match/gmatch compresi: pattern matching
       che gira in C, invisibile all'hook di conteggio istruzioni, spec
       sezione 2 punto 5) tramite la sintassi a metodo ("x"):qualcosa(...),
       indipendentemente da quali funzioni esponiamo come globale 'string'.
       Filtrare DOPO non basterebbe: bisognerebbe rimpiazzare la
       metatabella condivisa stessa, e a quel punto la superficie di
       attacco (getmetatable/setmetatable, esclusi qui sopra) andrebbe
       riaperta solo per poterla difendere. Piu' semplice e piu' sicuro non
       aprire affatto la libreria: niente viene mai installato, quindi non
       c'e' nulla da avvelenare (verificato dal test dell'escape 5). Le
       stringhe restano comunque utilizzabili come valore primitivo (".."
       per la concatenazione, "#" per la lunghezza, confronti, chiavi di
       tabella, argomenti di tostring()): sono tutte operazioni della VM,
       non della libreria. */

    /* rng: la RNG del gioco (src/core/game_math.c, xorshift a 32 bit),
       seminata con lo stesso 'seed' passato a lua_newstate (hash seed di
       Lua: stesso seed -> stesso ordine di pairs() -> stessa sequenza di
       rng(), spec sezione 3 e sezione 9, criterio 5). Sostituisce
       math.random/randomseed. */
    lua_pushlightuserdata(L, sb);
    lua_pushcclosure(L, ScriptSandboxLuaRng, 1);
    lua_setfield(L, envIdx, "rng");

    lua_pop(L, 1);   /* toglie _ENV dallo stack esplorativo: resta comunque in registry[LUA_RIDX_GLOBALS] */
}

/* ============================================================
   Interruttore di emergenza (spec, sezione 5 e 9)
   ============================================================ */

static void ScriptSandboxKill(ScriptSandbox *sb, const char *reason)
{
    sb->disabled = true;
    snprintf(sb->disabledReason, sizeof(sb->disabledReason), "%s", reason);
    ScriptSandboxLogLine("[%s] disabilitata per il resto della run: %s",
                         sb->name[0] ? sb->name : "?", reason);
}

static const char *ScriptSandboxClassifyError(int rc, const char *msg)
{
    if (rc == LUA_ERRMEM) return "tetto di memoria superato";
    if (msg != NULL && strstr(msg, "budget di istruzioni") != NULL) return "budget di istruzioni superato";
    return "errore a runtime";
}

/* ============================================================
   API pubblica
   ============================================================ */

ScriptSandbox *ScriptSandboxCreate(unsigned int seed, size_t memoryCapBytes)
{
    ScriptSandbox *sb = calloc(1, sizeof *sb);
    if (sb == NULL) return NULL;
    sb->memCap = memoryCapBytes;
    sb->memUsed = 0;
    /* GameRngNext si autocorregge se lo stato scende a 0 (vedi
       game_math.c), ma partire gia' non-zero evita di affidarsi a quel
       dettaglio implementativo per il primissimo numero estratto. */
    sb->rngState = (seed != 0) ? seed : 0xA341316Cu;
    sb->disabled = false;

    /* Il seed passato qui e' lo stesso hash seed che decide l'ordine di
       pairs() su chiavi stringa (spec sezione 3): e' l'intero motivo per
       cui il progetto ha scelto Lua 5.5 invece di 5.4 (che mescola ASLR e
       time() internamente, rendendo due run con lo stesso seed di gioco
       comunque diverse). */
    sb->L = lua_newstate(ScriptSandboxAlloc, sb, seed);
    if (sb->L == NULL)
    {
        ScriptSandboxLogLine("ScriptSandboxCreate: lua_newstate ha fallito (tetto di memoria %zu troppo basso per la sola inizializzazione?)",
                             memoryCapBytes);
        free(sb);
        return NULL;
    }
    lua_atpanic(sb->L, ScriptSandboxPanic);
    ScriptSandboxBuildEnv(sb);
    return sb;
}

void ScriptSandboxDestroy(ScriptSandbox *sb)
{
    if (sb == NULL) return;
    if (sb->L != NULL) lua_close(sb->L);
    free(sb);
}

bool ScriptSandboxLoad(ScriptSandbox *sb, const char *name, const char *source, char *err, size_t errSize)
{
    if (err != NULL && errSize > 0) err[0] = '\0';
    if (sb == NULL || source == NULL) return false;
    if (sb->disabled)
    {
        if (err != NULL) snprintf(err, errSize, "sandbox gia' disabilitata: %s", sb->disabledReason);
        return false;
    }
    snprintf(sb->name, sizeof(sb->name), "%s", (name != NULL) ? name : "?");

    lua_State *L = sb->L;

    /* SOLO testo ("t"): questa e' l'UNICA funzione di caricamento usata in
       tutto questo file, e deve restare cosi'. luaL_loadbuffer,
       luaL_loadstring e luaL_dostring caricano di default in modalita' "bt"
       (testo O bytecode): Lua non verifica l'integrita' di un chunk
       binario (lundump.c si fida ciecamente dell'header), quindi un chunk
       malformato o costruito ad arte crasha l'interprete invece di
       fallire in modo pulito con un errore catturabile. Un chunk binario
       e' comunque legato a una singola build (versione, endianness,
       dimensioni di int/size_t) e non ha senso per contenuto generato al
       volo. Il target 'test' del Makefile fallisce la build se una di
       quelle tre funzioni compare in src/: vedi il grep li' dentro. */
    int rc = luaL_loadbufferx(L, source, strlen(source), sb->name, "t");
    if (rc != LUA_OK)
    {
        const char *msg = lua_tostring(L, -1);
        if (err != NULL) snprintf(err, errSize, "errore di compilazione: %s", (msg != NULL) ? msg : "?");
        lua_pop(L, 1);
        ScriptSandboxKill(sb, "errore di compilazione (sintassi)");
        return false;
    }

    /* Budget generoso (spec sezione 3): qui non gira solo codice inerte, il
       corpo di primo livello dello script VIENE ESEGUITO (tipicamente
       definisce le sue funzioni globali on_xxx e inizializza tabelle). */
    lua_sethook(L, ScriptSandboxCountHook, LUA_MASKCOUNT, SCRIPT_SANDBOX_LOAD_BUDGET);
    rc = lua_pcall(L, 0, 0, 0);
    lua_sethook(L, NULL, 0, 0);

    if (rc != LUA_OK)
    {
        const char *msg = lua_tostring(L, -1);
        const char *reason = ScriptSandboxClassifyError(rc, msg);
        if (err != NULL) snprintf(err, errSize, "%s: %s", reason, (msg != NULL) ? msg : "?");
        lua_pop(L, 1);
        ScriptSandboxKill(sb, reason);
        return false;
    }
    return true;
}

bool ScriptSandboxCallVoid(ScriptSandbox *sb, const char *fn, int nargs, ...)
{
    if (sb == NULL || sb->disabled || fn == NULL) return false;
    lua_State *L = sb->L;

    lua_getglobal(L, fn);
    if (!lua_isfunction(L, -1))
    {
        /* Non e' una fuga ne' un errore dello script: e' normale che un
           oggetto non implementi un hook opzionale (es. un oggetto che non
           tocca mai on_hit). Non disabilita la sandbox. */
        lua_pop(L, 1);
        return false;
    }

    va_list ap;
    va_start(ap, nargs);
    for (int i = 0; i < nargs; i++) lua_pushnumber(L, (lua_Number)va_arg(ap, double));
    va_end(ap);

    /* Budget stretto (spec sezione 3): questa e' la chiamata che il gioco
       ripete ad ogni frame, fino a 60 volte al secondo. */
    lua_sethook(L, ScriptSandboxCountHook, LUA_MASKCOUNT, SCRIPT_SANDBOX_FRAME_BUDGET);
    int rc = lua_pcall(L, nargs, 0, 0);
    lua_sethook(L, NULL, 0, 0);

    if (rc != LUA_OK)
    {
        const char *msg = lua_tostring(L, -1);
        const char *reason = ScriptSandboxClassifyError(rc, msg);
        lua_pop(L, 1);
        ScriptSandboxKill(sb, reason);
        return false;
    }
    return true;
}

lua_State *ScriptSandboxRawState(ScriptSandbox *sb)
{
    if (sb == NULL || sb->disabled) return NULL;
    return sb->L;
}

bool ScriptSandboxProtectedCall(ScriptSandbox *sb, int nargs, int nresults)
{
    if (sb == NULL || sb->disabled) return false;
    lua_State *L = sb->L;

    /* Stesso budget stretto di ScriptSandboxCallVoid: questa e' la stessa
       "chiamata di frame", solo con argomenti/risultati non numerici
       costruiti dal chiamante invece che dalla variadica qui sotto. */
    lua_sethook(L, ScriptSandboxCountHook, LUA_MASKCOUNT, SCRIPT_SANDBOX_FRAME_BUDGET);
    int rc = lua_pcall(L, nargs, nresults, 0);
    lua_sethook(L, NULL, 0, 0);

    if (rc != LUA_OK)
    {
        const char *msg = lua_tostring(L, -1);
        const char *reason = ScriptSandboxClassifyError(rc, msg);
        lua_pop(L, 1);
        ScriptSandboxKill(sb, reason);
        return false;
    }
    return true;
}

bool ScriptSandboxHasFunction(const ScriptSandbox *sb, const char *fn)
{
    if (sb == NULL || sb->disabled || fn == NULL) return false;
    lua_State *L = sb->L;
    lua_getglobal(L, fn);
    bool isFn = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return isFn;
}

bool ScriptSandboxGetGlobalNumber(const ScriptSandbox *sb, const char *name, double *out)
{
    if (sb == NULL || sb->disabled || name == NULL || out == NULL) return false;
    lua_State *L = sb->L;
    /* Nessuna metatabella e' MAI installata sulle tabelle di questa
       sandbox (ne' su _ENV, ne' su nessun'altra): lua_getglobal su un
       nome inesistente o su un tipo qualunque e' un semplice accesso
       diretto, non puo' sollevare un errore Lua. Nessun pcall serve qui. */
    lua_getglobal(L, name);
    bool ok = (lua_type(L, -1) == LUA_TNUMBER);
    if (ok) *out = (double)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return ok;
}

bool ScriptSandboxGetGlobalString(const ScriptSandbox *sb, const char *name, char *out, size_t outSize)
{
    if (sb == NULL || sb->disabled || name == NULL || out == NULL || outSize == 0) return false;
    lua_State *L = sb->L;
    lua_getglobal(L, name);
    bool ok = (lua_type(L, -1) == LUA_TSTRING);   /* solo stringhe vere: niente coercizione silenziosa dei numeri */
    if (ok)
    {
        size_t len = 0;
        const char *s = lua_tolstring(L, -1, &len);
        size_t copyLen = (len < outSize - 1) ? len : outSize - 1;
        memcpy(out, s, copyLen);
        out[copyLen] = '\0';
    }
    lua_pop(L, 1);
    return ok;
}

size_t ScriptSandboxMemoryUsed(const ScriptSandbox *sb)
{
    return (sb != NULL) ? sb->memUsed : 0;
}

bool ScriptSandboxIsDisabled(const ScriptSandbox *sb)
{
    return (sb != NULL) ? sb->disabled : true;
}

const char *ScriptSandboxDisabledReason(const ScriptSandbox *sb)
{
    if (sb == NULL || !sb->disabled) return "";
    return sb->disabledReason;
}

void ScriptSandboxLogLine(const char *fmt, ...)
{
    /* logs/ e' creata dal target del Makefile che produce il binario del
       gioco (mkdir -p bin logs generated), esattamente come si affida a
       quella stessa garanzia game_renderer.c per TakeScreenshot: nessun
       mkdir a runtime qui, coerente col resto di src/. */
    FILE *f = fopen("logs/script-sandbox.log", "a");
    if (f == NULL) return;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s] ", stamp);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}
