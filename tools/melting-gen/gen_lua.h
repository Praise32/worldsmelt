#ifndef MELTING_GEN_LUA_H
#define MELTING_GEN_LUA_H

/* gen_lua.{h,c}: fase 3a-L3 (vedi
   docs/superpowers/specs/2026-07-13-lua-sandbox-design.md, sezioni 6 e 9).
   Chiude il cerchio aperto dalla spec: fino a qui l'LLM sceglieva SOLO fra
   le quattro operazioni della mini-VM (script CSV); da qui puo' scrivere
   Lua vero per il comportamento speciale di un oggetto, opzionale, validato
   PRIMA che il gioco lo veda mai (sezione 9, "nessuno script puo' rompere
   il gioco" si applica anche qui, non solo a runtime).

   Due responsabilita' distinte in questo modulo:
   - GenLuaValidate: sintassi + dry-run nella sandbox VERA del gioco
     (src/script/script_sandbox.c, compilata anche dentro melting-gen, vedi
     Makefile) con un'API di gioco FINTA (stub, sotto in gen_lua.c). Non
     serve un modello: e' quello che esercita il corpus di
     tests/melting-gen/lua/ tramite il flag --lua-check, e quello che il
     ciclo di generazione sotto chiama ad ogni tentativo.
   - GenLuaGenerateForRun: il ciclo prompt -> modello -> valida -> ritenta
     (fino a 2 volte, con l'errore rimandato al modello) per i 20 oggetti di
     una run (3 attivi + 1 stat-up per piano, fase 3), riusando la sessione
     LLM gia' aperta per il JSON. */

#include "melting_gen.h"

#include <stdbool.h>

typedef struct GenLuaStats {
    int firstTry;       /* script valido al primo tentativo */
    int afterRetry;      /* script valido dopo 1 o 2 ritenti */
    int optedOut;         /* il modello ha scelto esplicitamente "nessun comportamento speciale" (script sintatticamente valido ma senza nessuna callback) */
    int fellBack;          /* nessuno script valido entro i tentativi: l'oggetto resta sulla sola mini-VM */
    int skippedBudget;      /* mai tentato: budget di tempo della fase Lua esaurito (vedi GEN_LUA_PHASE_BUDGET_SEC) */
} GenLuaStats;

/* Genera, quando possibile, lo script Lua di ciascuno dei 20 oggetti di
   'run' (5 piani x (3 oggetti attivi + 1 oggetto stat-up del boss), fase 3):
   per i tre attivi il prompt prompts/lua_user.txt (UN solo effetto semplice,
   vedi la vision doc sezione 2), per il bossItem il prompt dedicato
   prompts/lua_statup_user.txt (validato con statUpOnly=true, vedi
   GenLuaValidate). Sessione LLM 'sess' GIA' APERTA (riusata dalla
   generazione JSON: vedi main.c). Non tocca il filesystem: il chiamante
   (gen_manifest.c) scrive i file .lua e la riga di manifest solo per gli
   oggetti con .lua non vuoto. Scrive 'stats' con il riepilogo per il log
   (vedi make test-llm). */
void GenLuaGenerateForRun(GenLlmSession *sess, GenRun *run, const char *promptsDir,
                           const char *outDir, double deadline, GenLuaStats *stats);

/* Validazione pura, senza alcun modello: carica 'source' in una
   ScriptSandbox NUOVA con lo stesso allowlist/tetto di memoria/budget di
   istruzioni del gioco (script/script_sandbox.h), la registra con un'API di
   gioco finta (stub: nessun Game reale, melting-gen non lo linka mai, vedi
   il commento in cima a gen_lua.c) e chiama una volta ciascuna delle
   callback (on_evaluate/on_fire/on_hit/on_tick) che lo script definisce, con
   argomenti plausibili. Ritorna false su qualunque fallimento (sintassi,
   tetto di memoria, budget di istruzioni, errore a runtime, handle non
   valido) e riempie 'err' (puo' essere NULL/errSize 0). 'anyCallback' (puo'
   essere NULL) viene impostato a true se lo script compila E definisce
   almeno una delle quattro callback: uno script sintatticamente valido ma
   che non ne definisce nessuna e' trattato dal chiamante come "il modello
   ha scelto di non proporre nulla", non come un fallimento.
   'statUpOnly' (fase 3, vedi la vision doc sezione 1: "gli oggetti stat-up
   sono puri aumenti di statistiche, nessun comportamento nuovo") quando vero
   rifiuta uno script che definisce on_fire/on_hit/on_tick: un oggetto
   ricompensa del boss puo' SOLO ricalcolare statistiche (on_evaluate) o non
   fare nulla di scriptato. Non e' un cambiamento della sandbox (che resta
   libera quanto prima, vedi script_sandbox.c): e' un gate di dominio in
   PIU', qui in melting-gen, sullo stesso modello dei tanti altri gia'
   presenti in questo file (OpAllowsTrait e affini in gen_validate.c). Falso
   per un oggetto attivo: nessun cambiamento di comportamento rispetto a
   prima di questa fase. */
bool GenLuaValidate(const char *source, unsigned int seed, bool statUpOnly, bool *anyCallback, char *err, size_t errSize);

#endif
