/* Un test per ciascuna delle fughe elencate nella spec (sezioni 2 e 4 di
   docs/engineering/specs/2026-07-13-lua-sandbox-design.md): ognuno esegue
   DAVVERO lo snippet ostile attraverso l'API pubblica di ScriptSandbox e
   verifica che sia stato fermato, non che una stringa sia assente da un
   file. Il test di determinismo (escape 9) e' separato: richiede due
   PROCESSI diversi, e vive in ScriptSandboxDeterminismProbe, chiamata due
   volte da scripts/test-script.sh con lo stesso seme. */

#include "tests/game_tests.h"

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

/* RSS del processo corrente in KB, da /proc/self/status. Usata SOLO
   dall'escape 2 (bomba di memoria) per verificare che il processo del
   gioco non si gonfi davvero, non solo che la contabilita' interna della
   sandbox dica di essersi fermata: sono due prove indipendenti, e la
   ricerca a monte della spec insiste che il rischio vero e' proprio la RAM
   reale del giocatore. */
static long ReadRssKb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (f == NULL) return -1;
    char line[256];
    long kb = -1;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0) { sscanf(line + 6, "%ld", &kb); break; }
    }
    fclose(f);
    return kb;
}

/* Escape 1: ciclo infinito senza nessuna chiamata, nessuna allocazione.
   L'unica cosa che puo' fermarlo e' l'hook di conteggio istruzioni. Prima
   che questa sandbox esistesse, uno script cosi' (generato per errore da
   un modello che non ha chiuso una condizione) avrebbe bloccato per sempre
   il thread di gioco: nessun limite esisteva. */
static bool TestEscape1InstructionCap(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(101u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) { printf("  [1] impossibile creare la sandbox\n"); return false; }

    char err[256];
    double t0 = NowSeconds();
    bool loaded = ScriptSandboxLoad(sb, "escape1", "while true do end", err, sizeof(err));
    double elapsed = NowSeconds() - t0;
    bool disabled = ScriptSandboxIsDisabled(sb);
    bool reasonOk = strstr(ScriptSandboxDisabledReason(sb), "istruzioni") != NULL;

    printf("  [1] 'while true do end' -> %s in %.3fs, motivo=\"%s\"\n",
           loaded ? "CARICATO (sbagliato!)" : "bloccato", elapsed, err);
    ScriptSandboxDestroy(sb);

    bool ok = !loaded && disabled && reasonOk && elapsed < 3.0;
    if (!ok) printf("      FALLITO: atteso un fallimento rapido per budget di istruzioni superato\n");
    return ok;
}

/* Escape 2: bomba di memoria costruita SOLO con while/tabelle/aritmetica
   (nessuna string.rep: string non e' nemmeno nell'allowlist, vedi
   script_sandbox.c). Ogni iterazione crea una tabella nuova, cosi' il
   costo in memoria per istruzione eseguita e' alto e il tetto di memoria
   scatta con largo margine prima del budget di istruzioni (verificato: la
   'reasonOk' sotto controlla proprio che sia stato il tetto di memoria, e
   non quello di istruzioni, a fermare lo script). */
static bool TestEscape2MemoryCap(void)
{
    const char *bomb =
        "local t = {}\n"
        "local i = 0\n"
        "while true do\n"
        "  i = i + 1\n"
        "  t[i] = { i, i, i }\n"
        "end\n";

    long rssBefore = ReadRssKb();
    bool allOk = true;
    for (int i = 0; i < 20; i++)
    {
        ScriptSandbox *sb = ScriptSandboxCreate((unsigned int)(200 + i), SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
        if (sb == NULL) { allOk = false; break; }
        char err[256];
        bool loaded = ScriptSandboxLoad(sb, "escape2", bomb, err, sizeof(err));
        bool disabled = ScriptSandboxIsDisabled(sb);
        bool reasonOk = strstr(ScriptSandboxDisabledReason(sb), "memoria") != NULL;
        size_t used = ScriptSandboxMemoryUsed(sb);
        if (i == 0)
        {
            printf("  [2] tabella che cresce all'infinito -> %s, motivo=\"%s\", memUsed=%zu/%u\n",
                   loaded ? "CARICATA (sbagliato!)" : "bloccata", err, used, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
        }
        if (loaded || !disabled || !reasonOk || used > SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP) allOk = false;
        ScriptSandboxDestroy(sb);
    }
    long rssAfter = ReadRssKb();
    long grew = (rssBefore >= 0 && rssAfter >= 0) ? (rssAfter - rssBefore) : -1;
    printf("  [2] RSS del processo: %ld KB -> %ld KB (crescita %ld KB per 20 bombe con tetto %u byte l'una)\n",
           rssBefore, rssAfter, grew, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    /* Soglia ampiamente generosa: 20 sandbox con tetto ~1 MB l'una fanno al
       massimo qualche MB di contabilita' onesta. Una bomba NON fermata
       farebbe crescere l'RSS di centinaia di MB (o esaurirebbe la RAM)
       in questi stessi 20 tentativi. */
    if (grew > 64L*1024L)
    {
        printf("      FALLITO: RSS cresciuta di %ld KB, ben oltre il margine atteso\n", grew);
        allOk = false;
    }
    if (!allOk) printf("      FALLITO: la bomba di memoria non e' stata fermata dal tetto\n");
    return allOk;
}

/* Escape 3: coroutine.wrap bypassa gli hook di debug (per-thread, non
   ereditati da una coroutine nuova). Qui 'coroutine' deve essere nil: lo
   script fallisce PRIMA ancora di creare la coroutine, non dopo un ciclo
   che gira senza freno. */
static bool TestEscape3CoroutineAbsent(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(103u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) return false;

    char err[256];
    double t0 = NowSeconds();
    bool loaded = ScriptSandboxLoad(sb, "escape3",
        "coroutine.wrap(function() while true do end end)()", err, sizeof(err));
    double elapsed = NowSeconds() - t0;
    bool disabled = ScriptSandboxIsDisabled(sb);
    bool nilRef = strstr(err, "nil value") != NULL;

    printf("  [3] coroutine.wrap(...) -> %s in %.3fs: %s\n",
           loaded ? "CARICATO (sbagliato!)" : "bloccato", elapsed, err);
    ScriptSandboxDestroy(sb);

    bool ok = !loaded && disabled && nilRef && elapsed < 1.0;
    if (!ok) printf("      FALLITO: 'coroutine' deve essere nil, fallimento immediato\n");
    return ok;
}

/* Escape 4: pcall si mangerebbe l'errore sollevato dal nostro hook. Qui
   'pcall' deve essere nil: il ciclo fallisce al primo giro. */
static bool TestEscape4PcallAbsent(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(104u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) return false;

    char err[256];
    double t0 = NowSeconds();
    bool loaded = ScriptSandboxLoad(sb, "escape4",
        "while true do pcall(function() while true do end end) end", err, sizeof(err));
    double elapsed = NowSeconds() - t0;
    bool disabled = ScriptSandboxIsDisabled(sb);
    bool nilRef = strstr(err, "nil value") != NULL;

    printf("  [4] 'while true do pcall(...) end' -> %s in %.3fs: %s\n",
           loaded ? "CARICATO (sbagliato!)" : "bloccato", elapsed, err);
    ScriptSandboxDestroy(sb);

    bool ok = !loaded && disabled && nilRef && elapsed < 1.0;
    if (!ok) printf("      FALLITO: 'pcall' deve essere nil, fallimento immediato\n");
    return ok;
}

/* Escape 5: getmetatable("").__index e' - in un interprete Lua aperto con
   luaL_openlibs - la libreria string VERA, condivisa da ogni stringa dello
   stato. Questa sandbox non chiama mai luaopen_string (vedi il commento
   lungo in script_sandbox.c), quindi 'getmetatable' e' gia' nil di suo: lo
   script ostile fallisce prima ancora di poter tentare l'avvelenamento.
   Il secondo script, DIVERSO e in una sandbox DIVERSA, deve comportarsi in
   modo normale: prova che non c'e' nessuno stato condiviso da avvelenare
   (ogni ScriptSandbox e' un lua_State indipendente, vedi lo header). */
static bool TestEscape5StringMetatable(void)
{
    ScriptSandbox *attacker = ScriptSandboxCreate(105u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (attacker == NULL) return false;

    char err[256];
    bool loaded = ScriptSandboxLoad(attacker, "escape5-attacker",
        "getmetatable(\"\").__index.rep = function() return \"pwned\" end", err, sizeof(err));
    bool disabled = ScriptSandboxIsDisabled(attacker);
    bool nilRef = strstr(err, "nil value") != NULL;
    printf("  [5] getmetatable(\"\").__index.rep = ... -> %s: %s\n",
           loaded ? "CARICATO (sbagliato!)" : "bloccato", err);
    ScriptSandboxDestroy(attacker);

    bool attackOk = !loaded && disabled && nilRef;

    /* Un secondo script, in una sandbox nuova, che si comporta bene: se il
       tentativo sopra avesse davvero avvelenato qualcosa di globale (o se
       ci fosse un bug che condivide stato fra sandbox), qui si vedrebbe. */
    ScriptSandbox *victim = ScriptSandboxCreate(106u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    bool victimLoaded = (victim != NULL) && ScriptSandboxLoad(victim, "escape5-victim",
        "result = tostring(1 + 1)", err, sizeof(err));
    char out[32] = { 0 };
    bool victimOk = victimLoaded && ScriptSandboxGetGlobalString(victim, "result", out, sizeof(out))
                     && strcmp(out, "2") == 0 && !ScriptSandboxIsDisabled(victim);
    printf("  [5] script pulito successivo: result=\"%s\" (%s)\n", out, victimOk ? "ok" : "FALLITO");
    ScriptSandboxDestroy(victim);

    return attackOk && victimOk;
}

/* Escape 6: cinque tentativi separati di raggiungere il mondo esterno.
   Nessuna di queste globali esiste nell'_ENV costruito da zero: ognuno
   deve fallire come un qualunque accesso a un nome inesistente. */
static bool TestEscape6ForbiddenGlobals(void)
{
    static const struct { const char *label; const char *src; } cases[] = {
        { "io.open",           "io.open(\"/etc/passwd\")" },
        { "os.execute",        "os.execute(\"id\")" },
        { "require",           "require(\"os\")" },
        { "load",              "load(\"return 1\")" },
        { "debug.getregistry", "debug.getregistry()" },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++)
    {
        ScriptSandbox *sb = ScriptSandboxCreate((unsigned int)(600 + i), SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
        if (sb == NULL) { allOk = false; continue; }
        char err[256];
        bool loaded = ScriptSandboxLoad(sb, cases[i].label, cases[i].src, err, sizeof(err));
        bool disabled = ScriptSandboxIsDisabled(sb);
        bool nilRef = strstr(err, "nil value") != NULL;
        printf("  [6] %-18s -> %s: %s\n", cases[i].label,
               loaded ? "CARICATO (sbagliato!)" : "bloccato", err);
        if (loaded || !disabled || !nilRef) allOk = false;
        ScriptSandboxDestroy(sb);
    }
    if (!allOk) printf("      FALLITO: almeno uno degli accessi vietati non e' fallito come atteso\n");
    return allOk;
}

/* Escape 7: le funzioni di pattern matching di string girano in C e non
   fanno mai scattare l'hook di conteggio istruzioni: un pattern patologico
   bloccherebbe il gioco senza modo di fermarlo. Qui l'intera libreria
   string non e' mai installata (vedi script_sandbox.c), quindi la sintassi
   a metodo su una stringa letterale fallisce per mancanza di metatabella,
   PRIMA ancora che l'assenza delle sole funzioni di pattern conti qualcosa:
   chiude la fuga in un colpo solo, invece che funzione per funzione. */
static bool TestEscape7StringPatternsAbsent(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(107u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) return false;

    char err[256];
    double t0 = NowSeconds();
    bool loaded = ScriptSandboxLoad(sb, "escape7", "local s = (\"x\"):rep(1000000)", err, sizeof(err));
    double elapsed = NowSeconds() - t0;
    bool disabled = ScriptSandboxIsDisabled(sb);
    bool indexErr = strstr(err, "index") != NULL;

    printf("  [7] (\"x\"):rep(1e6) -> %s in %.3fs: %s\n",
           loaded ? "CARICATO (sbagliato!)" : "bloccato", elapsed, err);
    ScriptSandboxDestroy(sb);

    bool ok = !loaded && disabled && indexErr && elapsed < 1.0;
    if (!ok) printf("      FALLITO: le stringhe non devono avere alcun metodo (nessuna libreria string caricata)\n");
    return ok;
}

/* Escape 8 (non ostile, ma va comunque provato): una sorgente rotta a
   livello sintattico deve essere rifiutata in modo pulito, con un
   messaggio utile, non con un crash o un comportamento indefinito. */
static bool TestEscape8SyntaxError(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(108u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) return false;

    char err[256];
    bool loaded = ScriptSandboxLoad(sb, "escape8", "function broken( ( end", err, sizeof(err));
    bool disabled = ScriptSandboxIsDisabled(sb);
    bool hasMessage = err[0] != '\0';

    printf("  [8] sorgente sintatticamente rotta -> %s: \"%s\"\n",
           loaded ? "CARICATA (sbagliato!)" : "rifiutata", err);
    ScriptSandboxDestroy(sb);

    bool ok = !loaded && disabled && hasMessage;
    if (!ok) printf("      FALLITO: atteso rifiuto pulito con un messaggio non vuoto\n");
    return ok;
}

/* Escape 10 (in realta' il test "positivo"): la sandbox non deve essere
   cosi' stretta da rendere impossibile scrivere qualcosa di reale. Lo
   script usa function/for/pairs/tabelle/math.sqrt/assert/rng: tutto
   allowlisted, niente di ostile. Il risultato e' verificato con
   un'uguaglianza esatta (non solo "non e' andato in crash"). */
static bool TestEscape10WellBehaved(void)
{
    const char *src =
        "result = 0\n"
        "function update(dt)\n"
        "  local values = {}\n"
        "  for i = 1, 5 do values[i] = i * i end\n"
        "  local sum = 0\n"
        "  for _, v in pairs(values) do sum = sum + v end\n"
        "  local r = rng()\n"
        "  assert(r >= 0 and r < 1, \"rng fuori range\")\n"
        "  result = result + dt * math.sqrt(sum)\n"
        "end\n";

    ScriptSandbox *sb = ScriptSandboxCreate(110u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) return false;

    char err[256];
    bool loaded = ScriptSandboxLoad(sb, "escape10", src, err, sizeof(err));
    /* HasFunction non e' esercitata da nessun altro escape: la si prova qui,
       sia sul caso vero (la funzione che lo script definisce davvero) sia
       sul caso falso (un hook opzionale che questo script non implementa,
       come sarebbe on_hit per un item che non lo usa). */
    bool hasUpdate = loaded && ScriptSandboxHasFunction(sb, "update");
    bool lacksOnHit = loaded && !ScriptSandboxHasFunction(sb, "on_hit");
    bool called = loaded && ScriptSandboxCallVoid(sb, "update", 1, 2.0);
    double result = 0.0;
    bool got = called && ScriptSandboxGetGlobalNumber(sb, "result", &result);
    double expected = 2.0*sqrt(55.0);   /* 1+4+9+16+25 = 55 */
    bool closeEnough = got && fabs(result - expected) < 1e-6;
    bool stillAlive = !ScriptSandboxIsDisabled(sb);

    printf("  [10] script ben educato -> loaded=%d hasUpdate=%d lacksOnHit=%d called=%d result=%.6f atteso=%.6f disabilitata=%d\n",
           loaded, hasUpdate, lacksOnHit, called, result, expected, ScriptSandboxIsDisabled(sb));
    if (!loaded || !called) printf("       errore: %s\n", err);
    ScriptSandboxDestroy(sb);

    bool ok = loaded && hasUpdate && lacksOnHit && called && got && closeEnough && stillAlive;
    if (!ok) printf("      FALLITO: uno script innocuo deve girare e produrre il risultato atteso\n");
    return ok;
}

/* Escape 11 (revisione di sicurezza finale, non a tavolino): table.move gira
   in un ciclo TUTTO IN C (deps/lua-5.5.0/src/ltablib.c, tmove: "for (i = 0;
   i < n; i++) { lua_geti(...); lua_seti(...); }" con n = e - f + 1, dove f/e
   sono interi passati dallo script e SVINCOLATI dalla dimensione vera della
   tabella: lua_geti/lua_seti fuori range restituiscono/creano
   silenziosamente nil, non sollevano mai un errore che fermerebbe il
   ciclo). L'hook LUA_MASKCOUNT (spec, sezione 4, barriera 3) conta SOLO
   istruzioni della VM Lua e non scatta MAI dentro una funzione C: e'
   esattamente la fuga 5 di sopra (pattern matching di string, "gira in C,
   senza limiti, inarrestabile"), rimasta aperta per errore su table.move
   mentre era gia' stata chiusa per string. Il revisore l'ha verificata per
   davvero: table.move(a, 1, 300000000, 1, a) ha girato 6.3s ininterrotti
   senza che la sandbox si disattivasse (4.2s se lanciato dentro on_tick sotto
   il budget di frame, comunque mai fermato), e con math.maxinteger
   (~9.2*10^18, gia' esposto da 'math': vedi lmathlib.c) il ciclo non
   finisce mai in pratica, senza nemmeno far crescere la memoria (il tetto
   di memoria, barriera 2, non interviene: non alloca nulla di nuovo, legge
   e riscrive celle gia' esistenti).

   Questo test passa DAVVERO per il percorso di chiamata di frame
   (ScriptSandboxCallVoid su on_tick, budget stretto SCRIPT_SANDBOX_FRAME_BUDGET),
   lo stesso che il gioco userebbe 60 volte al secondo, non una scorciatoia.
   PRIMA della correzione (table.move ancora presente in _ENV) questa
   chiamata non ritorna MAI: e' bloccata dentro tmove, e l'unica cosa che la
   ferma e' il timeout esterno (`timeout -s KILL 30` in
   scripts/test-script.sh), che trasforma l'intero processo di test in un
   fallimento (SIGKILL), non in un "FALLITO" pulito stampato da qui sotto.
   DOPO la correzione (table.move nil in _ENV) la chiamata a un valore nil
   solleva un errore Lua immediato ("attempt to call a nil value"),
   catturato da lua_pcall come qualunque altro errore a runtime, e la
   sandbox si disabilita in modo pulito (patto di sicurezza, spec sezione
   9): questo prima/dopo (hang forzatamente ucciso dall'esterno contro kill
   pulito e verificato qui) e' la prova che la correzione chiude davvero la
   fuga, non solo "sembra chiuderla". */
static bool TestEscape11TableMoveUnbounded(void)
{
    ScriptSandbox *sb = ScriptSandboxCreate(111u, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) { printf("  [11] impossibile creare la sandbox\n"); return false; }

    char err[256];
    bool loaded = ScriptSandboxLoad(sb, "escape11",
        "local a = {1, 2, 3}\n"
        "function on_tick()\n"
        "  table.move(a, 1, math.maxinteger, 1, a)\n"
        "end\n",
        err, sizeof(err));
    if (!loaded)
    {
        printf("  [11] script di preparazione non caricato (inatteso): %s\n", err);
        ScriptSandboxDestroy(sb);
        return false;
    }

    /* ATTENZIONE a chi rilancia questo test su un binario NON corretto: la
       riga sotto (ScriptSandboxCallVoid) non ritorna mai in quel caso (vedi
       il commento sopra). Non e' un bug del test: e' la prova stessa della
       fuga, ed e' esattamente cosi' che questo test e' stato verificato
       (fallito/appeso prima della correzione, passato dopo). */
    double t0 = NowSeconds();
    bool called = ScriptSandboxCallVoid(sb, "on_tick", 0);
    double elapsed = NowSeconds() - t0;
    bool disabled = ScriptSandboxIsDisabled(sb);
    const char *reason = ScriptSandboxDisabledReason(sb);
    bool runtimeErr = strstr(reason, "runtime") != NULL;

    printf("  [11] on_tick con table.move(a,1,math.maxinteger,1,a) -> %s in %.3fs, motivo=\"%s\"\n",
           called ? "ESEGUITO (sbagliato!)" : "bloccato", elapsed, reason);
    ScriptSandboxDestroy(sb);

    bool ok = !called && disabled && runtimeErr && elapsed < 3.0;
    if (!ok) printf("      FALLITO: 'table.move' deve essere nil, fallimento immediato (kill switch)\n");
    return ok;
}

bool ScriptSandboxSelfTest(void)
{
    struct { const char *label; bool (*fn)(void); } tests[] = {
        { "1 (tetto istruzioni)",           TestEscape1InstructionCap },
        { "2 (tetto memoria)",              TestEscape2MemoryCap },
        { "3 (coroutine assente)",          TestEscape3CoroutineAbsent },
        { "4 (pcall assente)",              TestEscape4PcallAbsent },
        { "5 (metatabella stringhe)",       TestEscape5StringMetatable },
        { "6 (globali vietate)",            TestEscape6ForbiddenGlobals },
        { "7 (pattern string assenti)",     TestEscape7StringPatternsAbsent },
        { "8 (errore di sintassi)",         TestEscape8SyntaxError },
        { "10 (script ben educato)",        TestEscape10WellBehaved },
        { "11 (table.move senza limiti)",   TestEscape11TableMoveUnbounded },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        printf("-- escape %s --\n", tests[i].label);
        if (!tests[i].fn()) allOk = false;
    }
    return allOk;
}

bool ScriptSandboxDeterminismProbe(unsigned int seed, char *out, size_t outSize)
{
    /* Escape 9: stesso seed -> stessa sequenza di rng() E stesso ordine di
       pairs() su un tavolo con chiavi stringa (l'hash seed di Lua 5.5,
       passato a lua_newstate, e' esattamente li' per questo: vedi
       script_sandbox.c e la spec, sezione 3). Il risultato e' scritto in
       una stringa e restituito al chiamante (src/app/app.c, flag
       --script-determinism-test), che lo stampa su stdout: la prova vera
       richiede di confrontare l'output di DUE PROCESSI separati con lo
       stesso seme, e vive in scripts/test-script.sh. */
    if (out == NULL || outSize == 0) return false;
    out[0] = '\0';

    ScriptSandbox *sb = ScriptSandboxCreate(seed, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) { snprintf(out, outSize, "ERRORE:create"); return false; }

    const char *src =
        "local order = \"\"\n"
        "local t = { zeta = 1, alpha = 2, mid = 3, beta = 4, omega = 5 }\n"
        "for k, _ in pairs(t) do order = order .. k .. \",\" end\n"
        "local nums = \"\"\n"
        "for i = 1, 8 do nums = nums .. tostring(rng()) .. \",\" end\n"
        "result = order .. \"|\" .. nums\n";

    char err[256];
    bool loaded = ScriptSandboxLoad(sb, "determinism", src, err, sizeof(err));
    bool got = loaded && ScriptSandboxGetGlobalString(sb, "result", out, outSize);
    if (!got) snprintf(out, outSize, "ERRORE:%s", err);
    ScriptSandboxDestroy(sb);
    return got;
}

#else /* _WIN32: la generazione in-game (e con essa questa suite) non esiste su Windows, vedi AGENTS.md */

bool ScriptSandboxSelfTest(void)
{
    return true;
}

bool ScriptSandboxDeterminismProbe(unsigned int seed, char *out, size_t outSize)
{
    (void)seed;
    if (out != NULL && outSize > 0) out[0] = '\0';
    return true;
}

#endif
