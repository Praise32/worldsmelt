/* Vedi gen_attacks.h per la panoramica. Compila dentro melting-gen anche
   src/script/script_sandbox.c (la stessa sandbox del gioco, vedi Makefile) e
   tools/procedural-combat-demo/demo_script_api.c (l'API VERA della demo, non
   uno stub: a differenza di gen_lua.c non serve inventarne una finta, quella
   della demo non tocca mai src/game/ ne' raylib). ADR-002 resta rispettato:
   e' sempre e solo il GENERATORE a linkarla per un dry-run usa-e-getta, il
   binario del gioco (bin/melting_run_gpu) non la vede mai, e la demo
   (bin/combat-lab) non linka mai llama.cpp (chiede la generazione a
   melting-gen come processo figlio, vedi WP4). */

#include "gen_attacks.h"

#include "demo_script_api.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Lunghezza massima di uno script d'attacco: piu' generosa di GEN_LUA_LEN
   (2000, melting_gen.h) perche' una macchina a fasi con 2-3 stati e
   commenti e' tipicamente piu' lunga di "un solo effetto semplice". Non
   deve stare dentro nessun campo del lato gioco (questi file non sono
   ancora letti dal runtime, l'integrazione e' WP4/fuori scope di questa
   spec): il tetto e' scelto solo per contenere n_predict=768 token con
   ampio margine, stesso spirito del limite in gen_lua.c. */
#define GEN_ATTACK_LEN 4000

/* Buffer del testo grezzo generato dal modello, prima dell'estrazione del
   fence markdown: 768 token a ~16 byte/token di margine (molto piu' largo
   del rapporto reale misurato per il Lua, vedi gen_lua.h) tiene comodo
   anche un modello verboso. */
#define GEN_ATTACK_RAW_CAP 12288

/* Nome del chunk Lua: compare come prefisso di OGNI messaggio d'errore
   ("attack-lua:12: ..."), quindi deve essere lo stesso nella passata di
   validazione e nel replay diagnostico, altrimenti il modello vedrebbe due
   provenienze diverse per lo stesso errore. */
#define GEN_ATTACK_CHUNK_NAME "attack-lua"

#define GEN_ATTACK_MAX_ATTEMPTS 3   /* tentativo iniziale + 2 ritenti, come GEN_LUA_MAX_ATTEMPTS in gen_lua.c */
#define GEN_ATTACK_TEMP 0.6f        /* stessa temperatura della fase Lua (GEN_LUA_TEMP in gen_lua.c): qui conta la correttezza, non la varieta' del testo */

/* ============================================================
   Validazione: sintassi + dry-run di 120 tick con l'API VERA della demo.
   ============================================================ */

static bool GenAttackIsAttackCommand(DemoScriptCommandType type)
{
    /* Sottoinsieme di comandi "gameplay" (demo_script_api.c, DemoAppend)
       che rappresenta davvero un ATTACCO: set_velocity e add_status sono
       gameplay=true anche loro (consumano la stessa quota), ma da soli
       descrivono un nemico che si muove o si segna uno stato, non un
       nemico che colpisce -- per questo NON bastano (spec sezione 5,
       requisito 4 del task brief). I due telegraph_* sono solo visuali
       (DemoAppend li registra con gameplay=false), quindi non compaiono
       nemmeno nell'enum sotto per costruzione: telegraph_arc/telegraph_beam
       non finiscono mai nel command buffer come comando "gameplay". */
    switch (type)
    {
        case DEMO_CMD_EMIT_ARC:
        case DEMO_CMD_EMIT_RING:
        case DEMO_CMD_EMIT_ORBIT:
        case DEMO_CMD_EMIT_BEAM:
        case DEMO_CMD_MELEE_SWEEP:
        case DEMO_CMD_CAPTURE_RADIUS:
        case DEMO_CMD_RELEASE_ECHOES:
            return true;
        default:
            return false;
    }
}

/* Lo stato che l'host consegna alla sandbox per UN tick del dry-run. Estratto
   in una struttura invece che calcolato inline dentro il ciclo perche' ora ha
   DUE lettori che devono vedere gli stessi identici numeri: la simulazione
   vera (GenAttackSimulateTicks) e il replay diagnostico
   (GenAttackRefineRuntimeError piu' sotto), che senza le stesse posizioni
   riprodurrebbe un errore diverso da quello da spiegare. */
typedef struct GenAttackFrame {
    float playerX, playerY, selfX, selfY, aimAngle;
    bool fireHeld, specialPressed;
} GenAttackFrame;

/* Posizioni plausibili IN MOVIMENTO per il tick 'tick' (task brief,
   requisito 4): non serve realismo di gioco, solo che self/player si muovano
   davvero (un nemico o un'arma che valida sempre e solo con posizioni ferme
   non eserciterebbe i rami di uno script che, per esempio, calcola una
   direzione a ogni tick). Le posizioni NON vengono clampate qui:
   DemoScriptApiBeginFrame lo fa gia' da sola (demo_script_api.c), quindi
   bastano coordinate "nell'ordine di grandezza giusto", non esattamente
   dentro l'arena.
   - nemico: fermo in alto-centro (un boss/nemico tipico della demo non
     insegue mai il player da solo, si sposta solo dentro il proprio script,
     vedi spider_arc.lua), il player orbita nella meta' bassa dell'arena,
     aim = verso il player (calcolato qui lato host: e' cio' che
     aim_at_player() farebbe lato script, ma BeginFrame vuole gia' un
     currentAimAngle pronto).
   - arma: self = player, che gira lentamente per l'arena (self e player
     COINCIDONO: e' la stessa entita', vedi il commento su DemoScriptApiState
     in demo_script_api.h), aim ruota lentamente e indipendentemente dal
     movimento (un'arma che spazza l'arena mentre il player si sposta).
     fireHeld/specialPressed restano falsi per un nemico (spec sezione 3:
     per un nemico quelle due funzioni esistono ma tornano sempre false,
     nessun ramo speciale nel prompt): fireHeld vero fra i tick 10 e 90 (una
     finestra di 80 tick, ampia abbastanza da coprire qualunque cooldown
     ragionevole di un'arma generata), specialPressed vero SOLO ai tick 30 e
     70 (una pressione singola, non un livello). */
static GenAttackFrame GenAttackFrameFor(int tick, bool isWeapon)
{
    GenAttackFrame f;
    memset(&f, 0, sizeof(f));
    float t = (float)tick;

    if (isWeapon)
    {
        float cx = (GEN_ATTACK_ROOM_LEFT + GEN_ATTACK_ROOM_RIGHT)*0.5f;
        float cy = (GEN_ATTACK_ROOM_TOP + GEN_ATTACK_ROOM_BOTTOM)*0.5f;
        float orbitAngle = t*0.05f;
        f.selfX = cx + cosf(orbitAngle)*260.0f;
        f.selfY = cy + sinf(orbitAngle)*160.0f;
        f.playerX = f.selfX;
        f.playerY = f.selfY;
        f.aimAngle = t*0.03f;
        f.fireHeld = (tick >= 10 && tick <= 90);
        f.specialPressed = (tick == 30 || tick == 70);
    }
    else
    {
        f.selfX = (GEN_ATTACK_ROOM_LEFT + GEN_ATTACK_ROOM_RIGHT)*0.5f;
        f.selfY = GEN_ATTACK_ROOM_TOP + 60.0f;
        float lowerCx = (GEN_ATTACK_ROOM_LEFT + GEN_ATTACK_ROOM_RIGHT)*0.5f;
        float lowerCy = GEN_ATTACK_ROOM_TOP + (GEN_ATTACK_ROOM_BOTTOM - GEN_ATTACK_ROOM_TOP)*0.75f;
        float orbitAngle = t*0.06f;
        f.playerX = lowerCx + cosf(orbitAngle)*220.0f;
        f.playerY = lowerCy + sinf(orbitAngle)*110.0f;
        f.aimAngle = atan2f(f.playerY - f.selfY, f.playerX - f.selfX);
    }
    return f;
}

/* Prepara il tick 'tick' sulla sandbox: BeginFrame (che azzera anche i due
   flag di input) e, solo per un'arma, SetInput -- l'ordine e la condizione
   sono quelli di DemoRunEnemyScript/DemoRunWeaponScript nella demo. */
static void GenAttackBeginTick(DemoScriptApiState *api, int tick, bool isWeapon)
{
    GenAttackFrame f = GenAttackFrameFor(tick, isWeapon);
    DemoScriptApiBeginFrame(api, f.playerX, f.playerY, f.selfX, f.selfY, f.aimAngle);
    if (isWeapon) DemoScriptApiSetInput(api, f.fireHeld, f.specialPressed);
}

/* Simula GEN_ATTACK_DRY_RUN_TICKS tick a dt fisso 1/60. '*failTick' riceve
   l'indice del tick che ha ucciso la sandbox (-1 se il fallimento non e' a
   runtime: nessun comando d'attacco in 120 tick), cosi' il chiamante puo'
   ritentare quel solo tick per estrarne il messaggio Lua vero (vedi
   GenAttackRefineRuntimeError). */
static bool GenAttackSimulateTicks(ScriptSandbox *sb, DemoScriptApiState *api,
                                    uint64_t selfHandle, bool isWeapon, int *failTick,
                                    char *err, size_t errSize)
{
    bool sawAttack = false;
    *failTick = -1;
    for (int tick = 0; tick < GEN_ATTACK_DRY_RUN_TICKS; tick++)
    {
        GenAttackBeginTick(api, tick, isWeapon);

        bool ok = ScriptSandboxCallVoid(sb, "on_tick", 2, 1.0/60.0, (double)selfHandle);
        if (!ok || ScriptSandboxIsDisabled(sb))
        {
            *failTick = tick;
            if (err) snprintf(err, errSize, "on_tick failed at tick %d/%d: %s",
                               tick + 1, GEN_ATTACK_DRY_RUN_TICKS, ScriptSandboxDisabledReason(sb));
            return false;
        }

        /* DemoScriptApiBeginFrame azzera commandCount a OGNI chiamata (il
           PROSSIMO giro del ciclo, non questo): il buffer letto qui e'
           ancora quello riempito dalla on_tick appena eseguita, va letto
           PRIMA che il prossimo BeginFrame lo cancelli. */
        if (!sawAttack)
        {
            const DemoScriptCommand *cmds = DemoScriptApiCommands(api);
            size_t n = DemoScriptApiCommandCount(api);
            for (size_t i = 0; i < n; i++)
            {
                if (GenAttackIsAttackCommand(cmds[i].type)) { sawAttack = true; break; }
            }
        }
    }

    if (!sawAttack)
    {
        /* Messaggio tenuto corto apposta: e' il testo che il ritento
           rimanda al modello, e la parte che conta ("questi non contano")
           sta in fondo -- con un buffer stretto era la prima a sparire. */
        if (err) snprintf(err, errSize,
            "ran %d ticks (two seconds) with no attack: the attacks are emit_arc/emit_ring/emit_orbit/"
            "emit_beam/melee_sweep/capture_radius/release_echoes, while set_velocity/add_status/telegraph_* "
            "are not. Fire the first one sooner (shorter wind-up), and check that your phase counter really advances",
            GEN_ATTACK_DRY_RUN_TICKS);
        return false;
    }
    return true;
}

/* Traduzioni brevi dei messaggi d'errore che l'API della demo solleva piu'
   spesso. demo_script_api.c e' CONGELATA e parla italiano; il prompt (e
   quindi il modello) parla inglese per DEC-052, e un errore rimandato
   indietro in una lingua diversa da quella del cheat-sheet vale poco. Il
   suggerimento non SOSTITUISCE il messaggio Lua (che porta anche il numero
   di riga, l'informazione piu' utile di tutte): lo accompagna.
   ORDINE SIGNIFICATIVO: "self_handle non valido" e "target_handle non
   valido" contengono entrambi "handle non valido" come sottostringa, quindi
   i due casi specifici vanno prima di quello generico. */
static const struct { const char *needle; const char *hint; } GEN_ATTACK_ERROR_HINTS[] = {
    { "deve essere intero",       " [whole number required there: recount the call arguments against its signature, or wrap the value in math.floor()]" },
    { "self_handle non valido",   " [the first argument of that call must be SELF_HANDLE]" },
    { "target_handle non valido", " [add_status accepts only SELF_HANDLE or PLAYER_HANDLE]" },
    { "handle non valido",        " [pass SELF_HANDLE, never a bare number]" },
    { "visual_id non ammesso",    " [the visual_id argument must be one of the VIS_* constants]" },
    { "status_id non ammesso",    " [the status_id argument must be one of the STATUS_* constants]" },
    { "non finito",               " [that argument is NaN or infinite: a division by zero?]" },
    { "got no value",             " [too few arguments in that call: count them against its signature]" },
    { "arithmetic on a nil value (global", " [that global has no starting value: assign one ABOVE on_tick, at the top of the script]" },
};

/* La firma ESATTA di ogni funzione dell'alfabeto, indicizzata dal nome come
   compare in una chiamata. Serve a rispondere alla domanda che il modello
   sbaglia piu' spesso: "quanti argomenti vuole QUESTA chiamata". Il
   messaggio di Lua non lo dice mai (parla di uno slot, non della funzione),
   e il cheat-sheet lo dice a 300 righe di distanza, dove evidentemente non
   basta: rimandarglielo insieme alla riga colpevole e' l'unica forma in cui
   arriva davvero. Va tenuta allineata a demo_script_api.c a mano, come ogni
   altra copia della stessa verita' in questo repo. */
static const struct { const char *call; const char *sig; } GEN_ATTACK_SIGNATURES[] = {
    { "telegraph_arc(",  "telegraph_arc(x, y, angle, radius, width, sweep, duration, visual_id): 8 arguments, no handle and NO damage" },
    { "telegraph_beam(", "telegraph_beam(x, y, angle, length, width, duration, visual_id): 7 arguments, no handle and NO damage" },
    { "emit_arc(",       "emit_arc(self_handle, x, y, angle, radius, width, sweep, damage, duration, visual_id): 10 arguments" },
    { "melee_sweep(",    "melee_sweep(self_handle, x, y, angle, radius, width, sweep, damage, duration, visual_id): 10 arguments" },
    { "emit_beam(",      "emit_beam(self_handle, x, y, angle, length, width, damage, duration, visual_id): 9 arguments" },
    { "emit_ring(",      "emit_ring(self_handle, x, y, count, speed, damage, shot_radius, life, visual_id): 9 arguments, NO angle" },
    { "emit_orbit(",     "emit_orbit(self_handle, x, y, count, orbit_radius, angular_speed, damage, shot_radius, life, visual_id): 10 arguments, NO angle" },
    { "release_echoes(", "release_echoes(self_handle, x, y, angle, count, speed, damage, spread, life, visual_id): 10 arguments" },
    { "capture_radius(", "capture_radius(self_handle, x, y, radius, pull_strength, max_targets, duration, visual_id): 8 arguments" },
    { "set_velocity(",   "set_velocity(self_handle, vx, vy): 3 arguments" },
    { "add_status(",     "add_status(target_handle, status_id, strength, duration): 4 arguments" },
};

/* La firma della PRIMA chiamata dell'alfabeto che compare nella riga (la
   piu' a sinistra, non la prima dell'elenco): una riga puo' annidare piu'
   chiamate -- self_x(), math.cos() -- ma quella che l'errore riguarda e'
   sempre l'esterna, cioe' quella che comincia prima. "" se la riga non
   chiama nessuna funzione dell'alfabeto. */
static const char *GenAttackSignatureFor(const char *lineText)
{
    if (!lineText) return "";
    const char *best = NULL;
    const char *bestSig = "";
    for (size_t i = 0; i < sizeof(GEN_ATTACK_SIGNATURES)/sizeof(GEN_ATTACK_SIGNATURES[0]); i++)
    {
        const char *at = strstr(lineText, GEN_ATTACK_SIGNATURES[i].call);
        if (at && (!best || at < best)) { best = at; bestSig = GEN_ATTACK_SIGNATURES[i].sig; }
    }
    return bestSig;
}

/* Il suggerimento si sceglie guardando PRIMA la riga colpevole e poi il
   messaggio. Non e' un dettaglio di stile: la riga sa cose che il messaggio
   non dice. Il caso che conta e' uno solo, ed e' il piu' frequente di tutti
   nei giri veri -- un self_handle passato a telegraph_arc/telegraph_beam,
   che NON lo prendono: Lua non se ne accorge (sono tutti numeri), sposta
   ogni argomento di uno slot e l'errore che ne esce parla dell'ULTIMO
   argomento ("argomento 8 deve essere intero"), cioe' di un punto della riga
   che col vero sbaglio non c'entra niente. Rimandare indietro "controlla il
   conteggio degli argomenti" per quel caso e' inutile: bisogna dire QUALE
   argomento e' di troppo. */
static const char *GenAttackErrorHint(const char *msg, const char *lineText)
{
    bool onTelegraph = lineText && (strstr(lineText, "telegraph_arc(") || strstr(lineText, "telegraph_beam("));
    if (onTelegraph && strstr(lineText, "self_handle"))
    {
        return " [telegraph_arc and telegraph_beam take NO self_handle: their first argument is x. Drop it and every other argument falls back into its right slot]";
    }
    for (size_t i = 0; i < sizeof(GEN_ATTACK_ERROR_HINTS)/sizeof(GEN_ATTACK_ERROR_HINTS[0]); i++)
    {
        if (strstr(msg, GEN_ATTACK_ERROR_HINTS[i].needle)) return GEN_ATTACK_ERROR_HINTS[i].hint;
    }
    return "";
}

/* Un messaggio Lua comincia sempre con "[string \"attack-lua\"]:31: ...":
   il numero di riga c'e', ma da solo non basta a un modello che deve
   ricordare a memoria cosa aveva scritto alla riga 31 (i giri veli del
   06/08 lo mostrano: rimandargli l'errore "argomento 4 deve essere intero"
   con la sola riga produce tre volte di fila lo stesso errore). Qui si
   separa il numero dal testo, cosi' il chiamante puo' rimandargli anche LA
   RIGA, presa da 'source'. Ritorna 0 se il prefisso non c'e' (messaggio non
   posizionale: allora resta solo il testo). */
static int GenAttackSplitLuaMessage(const char *msg, const char **bodyOut)
{
    *bodyOut = msg;
    const char *close = strstr(msg, "]:");
    if (!close) return 0;
    const char *p = close + 2;
    int line = 0;
    while (*p >= '0' && *p <= '9') { line = line*10 + (*p - '0'); p++; }
    if (line == 0 || *p != ':') return 0;
    p++;
    while (*p == ' ') p++;
    *bodyOut = p;
    return line;
}

/* Riga 'line' (1-based) di 'source', senza rientro e troncata: e' cio' che
   il modello deve rileggere per capire l'errore. 'out' resta vuoto se la
   riga non esiste. */
static void GenAttackSourceLine(const char *source, int line, char *out, size_t outCap)
{
    out[0] = '\0';
    if (line <= 0) return;

    const char *p = source;
    for (int i = 1; i < line && p; i++)
    {
        const char *nl = strchr(p, '\n');
        p = nl ? nl + 1 : NULL;
    }
    if (!p) return;

    while (*p == ' ' || *p == '\t') p++;
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    while (len > 0 && (p[len-1] == '\r' || p[len-1] == ' ')) len--;
    if (len >= outCap) len = outCap - 1;
    memcpy(out, p, len);
    out[len] = '\0';
}

/* Recupera il messaggio Lua VERO del tick che ha ucciso la sandbox.
   ScriptSandboxCallVoid lo legge (script_sandbox.c) ma lo consuma dentro
   ScriptSandboxKill, che conserva solo la CLASSIFICAZIONE ("errore a
   runtime"): da fuori non resta nulla di utilizzabile, e src/script/ non si
   tocca da qui (e' la sandbox del gioco, non del generatore). Quindi si
   rigioca la scena: sandbox NUOVA con lo stesso seed, stesso stato dell'API,
   stessi tick fino a 'failTick' (il dry-run e' deterministico -- rng() nasce
   dal seed, le posizioni da GenAttackFrameFor -- quindi l'errore si
   ripresenta identico), e il tick colpevole viene richiamato a mano con
   lua_pcall per leggere il messaggio prima che sparisca.
   Il pcall a mano NON installa l'hook di conteggio istruzioni della sandbox
   (quello e' interno a script_sandbox.c): non serve e non e' un buco: la
   passata fedele qui sopra ha gia' dimostrato che questo tick termina entro
   SCRIPT_SANDBOX_FRAME_BUDGET istruzioni -- se non l'avesse fatto la
   classificazione sarebbe "budget di istruzioni superato" e questa funzione
   non verrebbe nemmeno chiamata (vedi il chiamante).
   'err' viene riscritto solo se il replay riproduce davvero l'errore;
   altrimenti resta il messaggio con la classificazione, come prima. */
static void GenAttackRefineRuntimeError(const char *source, unsigned int seed,
                                         uint64_t selfHandle, bool isWeapon, int failTick,
                                         char *err, size_t errSize)
{
    ScriptSandbox *sb = ScriptSandboxCreate(seed ? seed : 1u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb) return;

    DemoScriptApiState api;
    DemoScriptApiInit(&api, selfHandle, GEN_ATTACK_PLAYER_HANDLE,
                       GEN_ATTACK_ROOM_LEFT, GEN_ATTACK_ROOM_TOP,
                       GEN_ATTACK_ROOM_RIGHT, GEN_ATTACK_ROOM_BOTTOM);
    if (!DemoScriptApiRegister(sb, &api) || !ScriptSandboxLoad(sb, GEN_ATTACK_CHUNK_NAME, source, NULL, 0))
    {
        ScriptSandboxDestroy(sb);
        return;
    }

    for (int tick = 0; tick < failTick; tick++)
    {
        GenAttackBeginTick(&api, tick, isWeapon);
        if (!ScriptSandboxCallVoid(sb, "on_tick", 2, 1.0/60.0, (double)selfHandle))
        {
            ScriptSandboxDestroy(sb);   /* il replay diverge: si tiene il messaggio di prima */
            return;
        }
    }

    lua_State *L = ScriptSandboxRawState(sb);
    if (!L)
    {
        ScriptSandboxDestroy(sb);
        return;
    }
    GenAttackBeginTick(&api, failTick, isWeapon);
    lua_getglobal(L, "on_tick");
    lua_pushnumber(L, (lua_Number)(1.0/60.0));
    lua_pushnumber(L, (lua_Number)selfHandle);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        const char *msg = lua_tostring(L, -1);
        if (msg)
        {
            /* Ogni pezzo ha il suo tetto (%.N) invece di affidarsi al
               troncamento finale di snprintf: cosi' la coda -- il
               suggerimento in inglese, l'unica parte che il modello puo'
               tradurre in una correzione -- non sparisce mai per colpa di un
               messaggio Lua lungo. */
            const char *body = msg;
            int line = GenAttackSplitLuaMessage(msg, &body);
            char lineText[160];
            GenAttackSourceLine(source, line, lineText, sizeof(lineText));
            if (lineText[0])
            {
                const char *sig = GenAttackSignatureFor(lineText);
                snprintf(err, errSize,
                         "on_tick died at tick %d/%d, line %d: %.90s%s -- that line reads: %.140s%s%.120s",
                         failTick + 1, GEN_ATTACK_DRY_RUN_TICKS, line, body,
                         GenAttackErrorHint(msg, lineText), lineText,
                         sig[0] ? " -- the correct signature is " : "", sig);
            }
            else
            {
                snprintf(err, errSize, "on_tick died at tick %d/%d: %.160s%s",
                         failTick + 1, GEN_ATTACK_DRY_RUN_TICKS, msg, GenAttackErrorHint(msg, NULL));
            }
        }
        lua_pop(L, 1);
    }
    ScriptSandboxDestroy(sb);
}

bool GenAttackValidate(const char *source, const char *kind, unsigned int seed, char *err, size_t errSize)
{
    if (err && errSize) err[0] = '\0';
    if (!source || !source[0])
    {
        if (err) snprintf(err, errSize, "empty script");
        return false;
    }
    if (strlen(source) >= (size_t)GEN_ATTACK_LEN - 1)
    {
        if (err) snprintf(err, errSize, "script too long (limit %d characters)", GEN_ATTACK_LEN - 2);
        return false;
    }

    bool isWeapon;
    uint64_t selfHandle;
    if (kind && strcmp(kind, "enemy") == 0) { isWeapon = false; selfHandle = GEN_ATTACK_ENEMY_SELF_HANDLE; }
    else if (kind && strcmp(kind, "weapon") == 0) { isWeapon = true; selfHandle = GEN_ATTACK_WEAPON_SELF_HANDLE; }
    else
    {
        if (err) snprintf(err, errSize, "unknown kind '%s' (expected 'enemy' or 'weapon')", kind ? kind : "(null)");
        return false;
    }

    ScriptSandbox *sb = ScriptSandboxCreate(seed ? seed : 1u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (!sb)
    {
        if (err) snprintf(err, errSize, "could not create the validation sandbox (out of memory?)");
        return false;
    }

    /* DemoScriptApiState LOCALE a questo stack frame: deve restare allo
       stesso indirizzo finche' la sandbox vive (demo_script_api.h, in
       cima), e questo frame non ritorna prima di ScriptSandboxDestroy
       sotto -- nessuna copia, nessun puntatore che scappa di qui. */
    DemoScriptApiState api;
    DemoScriptApiInit(&api, selfHandle, GEN_ATTACK_PLAYER_HANDLE,
                       GEN_ATTACK_ROOM_LEFT, GEN_ATTACK_ROOM_TOP,
                       GEN_ATTACK_ROOM_RIGHT, GEN_ATTACK_ROOM_BOTTOM);
    /* Registrata PRIMA di ScriptSandboxLoad, come richiede il contratto in
       demo_script_api.h. */
    if (!DemoScriptApiRegister(sb, &api))
    {
        ScriptSandboxDestroy(sb);
        if (err) snprintf(err, errSize, "could not register the demo API in the sandbox");
        return false;
    }

    if (!ScriptSandboxLoad(sb, GEN_ATTACK_CHUNK_NAME, source, err, errSize))
    {
        ScriptSandboxDestroy(sb);
        return false;
    }

    if (!ScriptSandboxHasFunction(sb, "on_tick"))
    {
        ScriptSandboxDestroy(sb);
        if (err) snprintf(err, errSize, "the script must define on_tick(dt, self_handle): none found");
        return false;
    }

    int failTick = -1;
    bool ok = GenAttackSimulateTicks(sb, &api, selfHandle, isWeapon, &failTick, err, errSize);
    ScriptSandboxDestroy(sb);

    /* Solo per la classificazione generica: "tetto di memoria superato" e
       "budget di istruzioni superato" dicono gia' al modello cosa ha
       sbagliato, "errore a runtime" no -- ed e' proprio quella che serve
       spiegare (un argomento non intero, un handle sbagliato, un nil). Il
       riconoscimento passa dal testo perche' la sandbox non espone la
       classificazione in altro modo (ScriptSandboxDisabledReason e' gia'
       finita dentro 'err' qui sopra, ed e' l'unica forma in cui esiste). */
    if (!ok && failTick >= 0 && err && errSize && strstr(err, "errore a runtime"))
    {
        GenAttackRefineRuntimeError(source, seed, selfHandle, isWeapon, failTick, err, errSize);
    }
    return ok;
}

/* ============================================================
   Gate anti-copia (vedi gen_attacks.h, GenAttackGenerate): uno script
   "nuovo" che e' il few-shot ricopiato passa la validazione -- e' uno
   script curato, ovviamente valido -- ma non aggiunge NIENTE al pool. E'
   successo per davvero (giro reale 05/08: 8 script su 8 identici a
   spider_arc.lua), quindi il controllo sta nel codice, non solo nel prompt.
   ============================================================ */

#define GEN_ATTACK_COPY_MIN_LINE   10   /* sotto i 10 caratteri una riga e' "end"/"else"/"local a = 0": comune a qualunque script, non prova niente */
#define GEN_ATTACK_COPY_MIN_LINES   3   /* con meno di 3 righe significative il rapporto non dice nulla; resta il confronto letterale */
#define GEN_ATTACK_COPY_RATIO_PCT  85   /* quante righe significative devono ricomparire nel riferimento perche' sia "la stessa cosa riscritta" */

/* Normalizza per il confronto: via i commenti (`--` fino a fine riga e i
   blocchi `--[[ ]]`), via rientri, spazi ripetuti e righe vuote. Restano le
   sole righe di codice, una per riga, con gli spazi interni collassati a
   uno: due script che differiscono solo per commenti, indentazione o righe
   vuote normalizzano identici, che e' esattamente la forma di "copia" da
   riconoscere (l'header che WriteAttackScript aggiunge in cima ai file del
   pool e' un commento, quindi sparisce qui senza trattamenti speciali).
   Approssimazione nota e accettata: un `--` dentro una stringa Lua verrebbe
   scambiato per un commento -- questa e' un'euristica di somiglianza, non
   un parser, e l'alfabeto della demo e' tutto numerico (uno script
   d'attacco non ha motivo di contenere stringhe). Buffer malloc, NULL su
   OOM. */
static char *GenAttackNormalize(const char *src)
{
    size_t n = strlen(src);
    char *out = malloc(n + 2);
    if (!out) return NULL;

    size_t w = 0;
    bool lineHasText = false;
    bool pendingSpace = false;
    for (size_t i = 0; i < n; )
    {
        char c = src[i];
        if (c == '-' && i + 1 < n && src[i+1] == '-')
        {
            if (i + 3 < n && src[i+2] == '[' && src[i+3] == '[')
            {
                const char *close = strstr(src + i + 4, "]]");
                i = close ? (size_t)(close - src) + 2 : n;
            }
            else
            {
                while (i < n && src[i] != '\n') i++;
            }
            continue;
        }
        if (c == '\n' || c == '\r')
        {
            if (lineHasText) out[w++] = '\n';
            lineHasText = false;
            pendingSpace = false;
            i++;
            continue;
        }
        if (c == ' ' || c == '\t')
        {
            if (lineHasText) pendingSpace = true;
            i++;
            continue;
        }
        if (pendingSpace) { out[w++] = ' '; pendingSpace = false; }
        out[w++] = c;
        lineHasText = true;
        i++;
    }
    if (lineHasText) out[w++] = '\n';
    out[w] = '\0';
    return out;
}

/* Due gradi di "copia", entrambi su testo gia' normalizzato:
   1. LETTERALE: il candidato compare tale e quale dentro il riferimento.
      Col cheat-sheet come riferimento questo cattura in un colpo solo i due
      few-shot E gli snippet "RIGHT" (sono tutti dentro quel file), senza
      dover sapere dove ognuno inizia e finisce -- nessun marcatore da
      mantenere nel prompt.
   2. RISCRITTURA: la stessa macchina a fasi con qualche numero cambiato --
      il confronto letterale non la vedrebbe. Si contano le righe
      significative del candidato che ricompaiono nel riferimento: sopra
      l'85% e' lo stesso script travestito. Le righe corte non contano (vedi
      GEN_ATTACK_COPY_MIN_LINE): "end" e "function on_tick(dt, self_handle)"
      ci sono in ogni script, farle pesare respingerebbe anche gli script
      buoni. */
static bool GenAttackLooksCopied(const char *normCandidate, const char *normRef)
{
    if (!normCandidate || !normRef || !normCandidate[0] || !normRef[0]) return false;
    if (strstr(normRef, normCandidate) != NULL) return true;

    int significant = 0;
    int matched = 0;
    const char *line = normCandidate;
    while (*line)
    {
        const char *nl = strchr(line, '\n');
        size_t len = nl ? (size_t)(nl - line) : strlen(line);
        if (len >= GEN_ATTACK_COPY_MIN_LINE)
        {
            significant++;
            char buf[256];
            size_t copyLen = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
            memcpy(buf, line, copyLen);
            buf[copyLen] = '\0';
            if (strstr(normRef, buf) != NULL) matched++;
        }
        line = nl ? nl + 1 : line + len;
    }
    if (significant < GEN_ATTACK_COPY_MIN_LINES) return false;
    return matched*100 >= significant*GEN_ATTACK_COPY_RATIO_PCT;
}

/* true se 'source' e' gia' nel pool in un modo o nell'altro: uno dei pezzi
   di codice del cheat-sheet (i due few-shot, gli snippet "RIGHT") oppure un
   file gia' scritto in <outDir>/combat-lab/<kind>/ -- questo secondo
   confronto e' cio' che tiene diversi anche i tre script dello stesso lotto,
   visto che ognuno viene scritto subito dopo essere stato accettato.
   'err' riceve un messaggio pensato per il MODELLO (e' quello che il ritento
   gli rimanda), non per un log. Su OOM o file illeggibili non blocca nulla:
   un gate di novita' che fallisce in chiuso fermerebbe la generazione per un
   problema che non c'entra col contenuto. */
static bool GenAttackIsRehash(const char *source, const char *promptsDir, const char *outDir,
                               const char *kind, char *err, size_t errSize)
{
    char *norm = GenAttackNormalize(source);
    if (!norm) return false;

    bool copied = false;
    char sysPath[512];
    snprintf(sysPath, sizeof(sysPath), "%s/attack_system.txt", promptsDir);
    char *sys = GenReadFile(sysPath);
    if (sys)
    {
        char *normSys = GenAttackNormalize(sys);
        free(sys);
        if (normSys)
        {
            if (GenAttackLooksCopied(norm, normSys))
            {
                copied = true;
                if (err) snprintf(err, errSize,
                    "this is one of the cheat-sheet examples again, and Combat Lab already has it in its pool: "
                    "write a DIFFERENT pattern (another primitive, another number of phases, your own timings)");
            }
            free(normSys);
        }
    }

    char kindDir[560];
    snprintf(kindDir, sizeof(kindDir), "%s/combat-lab/%s", outDir, kind);
    DIR *dir = copied ? NULL : opendir(kindDir);
    if (dir)
    {
        struct dirent *entry;
        while (!copied && (entry = readdir(dir)) != NULL)
        {
            const char *dot = strrchr(entry->d_name, '.');
            if (!dot || strcmp(dot, ".lua") != 0) continue;

            char path[1100];
            snprintf(path, sizeof(path), "%s/%s", kindDir, entry->d_name);
            char *existing = GenReadFile(path);
            if (!existing) continue;
            char *normExisting = GenAttackNormalize(existing);
            free(existing);
            if (!normExisting) continue;

            if (GenAttackLooksCopied(norm, normExisting))
            {
                copied = true;
                /* Precisione esplicita sul nome del file: d_name puo'
                   arrivare a 255 byte e il compilatore non sa che qui i nomi
                   sono sempre "NNN_seedS.lua" (li scrive WriteAttackScript
                   poche righe piu' sotto). */
                if (err) snprintf(err, errSize,
                    "this is the same script as %.64s, already in Combat Lab's pool: "
                    "write a DIFFERENT pattern (another primitive, another number of phases, your own timings)",
                    entry->d_name);
            }
            free(normExisting);
        }
        closedir(dir);
    }

    free(norm);
    return copied;
}

/* ============================================================
   Generazione: prompt (cheat-sheet + due few-shot + scheda kind/brief/seed)
   -> modello (senza grammatica) -> estrazione -> valida -> ritenta.
   ============================================================ */

/* Forma d'attacco principale richiesta a QUESTO script, scelta dal seed.
   Non e' un vezzo: e' la leva che rende diversi due script generati con due
   semi diversi. Senza, il modello ricadeva sempre sull'unico pattern che il
   cheat-sheet mostra per intero (il few-shot nemico), e il gate anti-copia
   qui sopra si limitava a respingere tentativo dopo tentativo. Con una forma
   imposta la copia non e' nemmeno una risposta possibile alla richiesta.
   emit_arc NON e' in elenco di proposito: e' l'unica forma di cui il
   cheat-sheet mostra una realizzazione completa (spider_arc), quindi
   chiederla e' esattamente cio' che produceva le copie -- ed e' anche la
   sola gia' rappresentata nel pool curato. Le altre sei coprono tutto il
   resto dell'alfabeto. Lista unica per nemici e armi: l'alfabeto e' lo
   stesso, cambia solo cosa fa scattare l'attacco (fasi contro input), e
   quello lo dice gia' il cheat-sheet. */
static const char *GEN_ATTACK_SHAPES[] = {
    "emit_ring -- a burst of projectiles spreading outward from one point",
    "emit_beam -- a long damaging line, announced by a telegraph_beam first",
    "emit_orbit -- projectiles that circle a point instead of flying away",
    "release_echoes -- a cone of projectiles along a locked aim direction",
    "melee_sweep -- a short swing that only threatens whoever stands close",
    "capture_radius, then one emit_* -- pull first, punish right after",
};

static const char *GenAttackShapeForSeed(unsigned int seed)
{
    const unsigned int count = (unsigned int)(sizeof(GEN_ATTACK_SHAPES)/sizeof(GEN_ATTACK_SHAPES[0]));
    return GEN_ATTACK_SHAPES[seed % count];
}

static void TrimWhitespace(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == '\t')) s[--len] = '\0';
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\n' || s[start] == '\r' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

/* Stessa estrazione fence markdown di ExtractLuaCode in gen_lua.c (duplicata
   qui invece di esportata da li': gen_lua.c e' solo lettura per WP3, vedi il
   task brief). I modelli incorniciano spesso il codice in un blocco
   ```lua ... ``` nonostante l'istruzione esplicita di non farlo (vedi
   prompts/attack_system.txt). */
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
        size_t rawLen = strlen(raw);
        size_t copyLen = (rawLen < outCap - 1) ? rawLen : outCap - 1;
        memcpy(out, raw, copyLen);
        out[copyLen] = '\0';
    }
    TrimWhitespace(out);
}

/* Prima riga di 'text' (fino a \n/\r), copiata in 'out' senza superarne la
   capacita': usata sia per l'header del file scritto (spec sezione 6,
   "brief: <prima riga o '-'>") sia come testo vuoto quando 'text' e' NULL
   (nessun --attack-brief passato). */
static void FirstLine(const char *text, char *out, size_t outCap)
{
    out[0] = '\0';
    if (!text) return;
    size_t i = 0;
    while (text[i] && text[i] != '\n' && text[i] != '\r' && i + 1 < outCap) { out[i] = text[i]; i++; }
    out[i] = '\0';
}

/* Parte utente del prompt (kind/brief/seed/forma sostituiti in
   prompts/attack_user.txt), condivisa fra BuildAttackPrompt sotto e
   GenAttackPromptBudgetCheck -- stesso schema di BuildLuaUserFinal in
   gen_lua.c: il retry (prevError non vuoto) accoda l'errore dell'ultimo
   tentativo.
   Lo script fallito NON viene rimandato indietro insieme all'errore, anche
   se sarebbe la cosa "ovvia": provato nel giro reale del 06/08, Gemma lo
   ricopia e basta (9 tentativi su 9 hanno restituito lo stesso identico
   script con lo stesso identico errore, riga compresa). Quello che serve al
   modello non e' rivedere il proprio testo, e' sapere ESATTAMENTE dove ha
   sbagliato: per questo l'errore porta gia' con se' il numero di riga e la
   riga stessa (vedi GenAttackRefineRuntimeError). */
static char *BuildAttackUserFinal(const char *promptsDir, const char *kind, const char *briefText,
                                   unsigned int seed, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/attack_user.txt", promptsDir);
    char *tpl = GenReadFile(path);
    if (!tpl) return NULL;

    char *step1 = GenReplaceAll(tpl, "{KIND}", kind);
    free(tpl);
    if (!step1) return NULL;

    /* {BRIEF_LINE}: riga omessa (stringa vuota) quando non c'e' brief, mai
       un placeholder vuoto lasciato a meta' frase -- stesso trattamento di
       {EVITA} in gen_llm.c (GenLlmBuildJsonPrompt). */
    char briefLine[700];
    if (briefText && briefText[0]) snprintf(briefLine, sizeof(briefLine), "Brief: %s\n", briefText);
    else briefLine[0] = '\0';
    char *step2 = GenReplaceAll(step1, "{BRIEF_LINE}", briefLine);
    free(step1);
    if (!step2) return NULL;

    char seedText[32];
    snprintf(seedText, sizeof(seedText), "%u", seed);
    char *step3 = GenReplaceAll(step2, "{SEED}", seedText);
    free(step2);
    if (!step3) return NULL;

    /* {SHAPE} dallo STESSO seed della variazione: due semi diversi chiedono
       (quasi sempre) due forme diverse, e un ritento sullo stesso seed
       ripete la stessa richiesta -- deve cambiare il tentativo, non il
       compito. */
    char *userFinal = GenReplaceAll(step3, "{SHAPE}", GenAttackShapeForSeed(seed));
    free(step3);
    if (!userFinal) return NULL;

    if (prevError && prevError[0])
    {
        size_t cap = strlen(userFinal) + strlen(prevError) + 256;
        char *withRetry = malloc(cap);
        if (withRetry)
        {
            snprintf(withRetry, cap,
                "%s\n\nYour previous script was rejected:\n%s\n"
                "Write the script again from scratch, avoiding that mistake this time, "
                "following the same rules from the cheat-sheet above.",
                userFinal, prevError);
            free(userFinal);
            userFinal = withRetry;
        }
    }
    return userFinal;
}

static char *BuildAttackPrompt(const char *promptsDir, const char *kind, const char *briefText,
                                unsigned int seed, const char *prevError)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/attack_system.txt", promptsDir);
    char *sys = GenReadFile(path);
    if (!sys) return NULL;

    char *userFinal = BuildAttackUserFinal(promptsDir, kind, briefText, seed, prevError);
    if (!userFinal) { free(sys); return NULL; }

    char *prompt = GenChatMlWrap(sys, userFinal);
    free(sys);
    free(userFinal);
    return prompt;
}

bool GenAttackPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize)
{
    if (err && errSize) err[0] = '\0';

    /* Worst-case: "weapon" (6 char, piu' lungo di "enemy") e un brief
       PRESENTE e rappresentativo -- non il limite di campo estremo (il
       brief e' testo libero del proprietario, senza una grammatica che ne
       fissi un tetto, quindi "rappresentativo" e' l'unica misura sensata
       qui, stesso ragionamento del commento su GenLuaPromptBudgetCheck in
       gen_lua.c). Si scorrono TUTTE le forme d'attacco (il seed che sceglie
       {SHAPE} le tocca tutte, e non sono lunghe uguali) invece di fidarsi di
       un seed qualunque: la guardia deve misurare il prompt piu' grande che
       la generazione vera possa comporre, non uno a caso. */
    static const char *worstBrief =
        "The boss should feel like a living glacier that cracks the floor before it charges, "
        "with a clear gap the player can dash through to punish the recovery.";

    /* Il caso peggiore vero e' un RITENTO, non il primo tentativo: li' il
       prompt porta in coda anche l'errore che ha respinto lo script
       precedente. Ha un tetto duro (GEN_ATTACK_ERR_CAP), quindi qui si
       misura proprio quel tetto riempito di caratteri qualunque: e' l'unico
       modo perche' la guardia copra il ramo che consuma piu' contesto invece
       che quello piu' comodo. */
    char worstError[GEN_ATTACK_ERR_CAP];
    memset(worstError, 'x', sizeof(worstError) - 1);
    worstError[sizeof(worstError) - 1] = '\0';

    size_t worstBytes = 0;
    const unsigned int shapeCount = (unsigned int)(sizeof(GEN_ATTACK_SHAPES)/sizeof(GEN_ATTACK_SHAPES[0]));
    for (unsigned int s = 0; s < shapeCount; s++)
    {
        char *prompt = BuildAttackPrompt(promptsDir, "weapon", worstBrief, s, worstError);
        if (!prompt)
        {
            if (err) snprintf(err, errSize, "prompt attacchi non costruibile (file mancanti in %s?)", promptsDir);
            return false;
        }
        size_t len = strlen(prompt);
        free(prompt);
        if (len > worstBytes) worstBytes = len;
    }

    if (worstBytes > (size_t)GEN_ATTACK_PROMPT_BYTE_CEILING)
    {
        if (err) snprintf(err, errSize,
            "prompt attacchi = %zu byte, oltre il ceiling di %d (vedi GEN_ATTACK_PROMPT_BYTE_CEILING in gen_attacks.h)",
            worstBytes, GEN_ATTACK_PROMPT_BYTE_CEILING);
        return false;
    }
    return true;
}

/* Indice massimo GIA' presente in 'dirPath' (nomi "NNN_seed....lua"), +1:
   atoi si ferma al primo carattere non cifra, quindi "007_seed123.lua" ->
   7, e un nome senza cifre iniziali (".", "..", un file estraneo) -> 0,
   ignorato nel massimo. Cartella assente o appena creata -> si parte da 1,
   nessun errore: non e' una precondizione, e' pulizia opportunistica dello
   stesso spirito di GenRemoveOldScripts in gen_util.c. */
static int NextIndex(const char *dirPath)
{
    DIR *dir = opendir(dirPath);
    if (!dir) return 1;
    int maxIndex = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        int idx = atoi(entry->d_name);
        if (idx > maxIndex) maxIndex = idx;
    }
    closedir(dir);
    return maxIndex + 1;
}

/* Timestamp ISO 8601 in UTC (spec sezione 6: "<data ISO>" nell'header del
   file scritto) -- UTC invece del localtime_r di GenLogLine (gen_util.c)
   apposta: uno script generato puo' finire condiviso/incollato altrove, un
   orario senza fuso sarebbe ambiguo. */
static void IsoTimestamp(char *buf, size_t bufSize)
{
    time_t now = time(NULL);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    strftime(buf, bufSize, "%Y-%m-%dT%H:%M:%SZ", &tmv);
}

/* Scrive 'source' (gia' validato dal chiamante) in
   <outDir>/combat-lab/<kind>/NNN_seed<seedI>.lua con l'header commentato
   della spec (sezione 6), tmp+rename come ogni altro file di questo tool
   (GenPublishFile). Crea le directory mancanti con GenEnsureDir (un livello
   alla volta: outDir esiste gia' -- lo crea main.c all'avvio -- ma
   outDir/combat-lab e outDir/combat-lab/<kind> no). Ritorna false su
   qualunque fallimento di I/O. */
static bool WriteAttackScript(const char *outDir, const char *kind, const char *source,
                               unsigned int seedI, const char *briefFirstLine)
{
    char combatLabDir[512];
    snprintf(combatLabDir, sizeof(combatLabDir), "%s/combat-lab", outDir);
    if (GenEnsureDir(combatLabDir) != 0) return false;

    char kindDir[560];
    snprintf(kindDir, sizeof(kindDir), "%s/combat-lab/%s", outDir, kind);
    if (GenEnsureDir(kindDir) != 0) return false;

    int idx = NextIndex(kindDir);
    char finalPath[600], tmpPath[600];
    snprintf(finalPath, sizeof(finalPath), "%s/%03d_seed%u.lua", kindDir, idx, seedI);
    snprintf(tmpPath, sizeof(tmpPath), "%s/%03d_seed%u.lua.tmp", kindDir, idx, seedI);

    FILE *f = fopen(tmpPath, "w");
    if (!f) return false;

    char stamp[32];
    IsoTimestamp(stamp, sizeof(stamp));
    fprintf(f, "-- melting-gen --attacks | kind=%s | seed=%u | %s | brief: %s\n",
            kind, seedI, stamp, (briefFirstLine && briefFirstLine[0]) ? briefFirstLine : "-");
    fputs(source, f);
    size_t srcLen = strlen(source);
    if (srcLen == 0 || source[srcLen - 1] != '\n') fputc('\n', f);

    return GenPublishFile(f, tmpPath, finalPath) == 0;
}

int GenAttackGenerate(GenLlmSession *sess, const char *promptsDir, const char *outDir,
                       const char *kind, int count, const char *briefPath, unsigned int seed)
{
    if (!sess || !kind) return 0;
    if (count < 1) count = 1;
    if (count > 8) count = 8;

    char *briefFull = (briefPath && briefPath[0]) ? GenReadFile(briefPath) : NULL;
    if (briefFull) TrimWhitespace(briefFull);
    char briefFirstLine[200];
    FirstLine(briefFull, briefFirstLine, sizeof(briefFirstLine));

    int written = 0;
    for (int i = 0; i < count; i++)
    {
        unsigned int seedI = seed + (unsigned int)i*97u;
        char err[GEN_ATTACK_ERR_CAP];
        err[0] = '\0';
        char code[GEN_ATTACK_LEN];
        code[0] = '\0';
        bool success = false;

        for (int attempt = 0; attempt < GEN_ATTACK_MAX_ATTEMPTS && !success; attempt++)
        {
            char *prompt = BuildAttackPrompt(promptsDir, kind, briefFull, seedI, attempt > 0 ? err : NULL);
            if (!prompt)
            {
                snprintf(err, sizeof(err), "attack prompt not buildable (missing files in %s?)", promptsDir);
                printf("melting-gen: attacks %s %d/%d attempt %d/%d: %s\n",
                       kind, i + 1, count, attempt + 1, GEN_ATTACK_MAX_ATTEMPTS, err);
                break;
            }

            static char raw[GEN_ATTACK_RAW_CAP];
            unsigned int callSeed = seedI + (unsigned int)attempt*17u + 1u;
            int rc = GenLlmComplete(sess, prompt, NULL, GEN_ATTACK_N_PREDICT, GEN_ATTACK_TEMP, callSeed,
                                     NULL, NULL, 0, 0, raw, sizeof(raw), NULL);
            free(prompt);
            if (rc != 0)
            {
                snprintf(err, sizeof(err), "generation failed (decode or token limit)");
                printf("melting-gen: attacks %s %d/%d attempt %d/%d: %s\n",
                       kind, i + 1, count, attempt + 1, GEN_ATTACK_MAX_ATTEMPTS, err);
                continue;
            }

            char extracted[GEN_ATTACK_LEN];
            ExtractLuaCode(raw, extracted, sizeof(extracted));

            bool valid = GenAttackValidate(extracted, kind, seedI, err, sizeof(err));
            /* Il gate anti-copia viene DOPO la validazione, non prima:
               "questo script esiste gia'" ha senso solo per uno script che
               sarebbe altrimenti scrivibile, e l'errore di validazione e'
               sempre quello piu' utile da rimandare al modello. */
            if (valid && GenAttackIsRehash(extracted, promptsDir, outDir, kind, err, sizeof(err))) valid = false;
            printf("melting-gen: attacks %s %d/%d attempt %d/%d: %s\n",
                   kind, i + 1, count, attempt + 1, GEN_ATTACK_MAX_ATTEMPTS, valid ? "OK" : err);
            if (valid)
            {
                snprintf(code, sizeof(code), "%s", extracted);
                success = true;
            }
        }

        if (success)
        {
            if (WriteAttackScript(outDir, kind, code, seedI, briefFirstLine))
            {
                written++;
                GenLogLine("attacks: %s %d/%d ok (seed=%u)", kind, i + 1, count, seedI);
            }
            else
            {
                GenLogLine("attacks: %s %d/%d validated but write failed (seed=%u)", kind, i + 1, count, seedI);
            }
        }
        else
        {
            GenLogLine("attacks: %s %d/%d failed after %d attempts (seed=%u): %s",
                       kind, i + 1, count, GEN_ATTACK_MAX_ATTEMPTS, seedI, err);
        }
    }

    free(briefFull);
    return written;
}
