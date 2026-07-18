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
#include <stddef.h>

/* Quanti token il modello genera per lo script Lua di UN oggetto (spostata
   qui da gen_lua.c in fase 3b review: GEN_LUA_PROMPT_BYTE_CEILING sotto ne
   ha bisogno per calcolare quanti token restano al PROMPT dentro
   GEN_LLM_SESSION_N_CTX, vedi melting_gen.h). Uno script piccolo: abbondante
   per "una sola sinergia" (vedi il prompt), tiene la fase dentro
   GEN_LUA_PHASE_BUDGET_SEC. */
#define GEN_LUA_N_PREDICT 384

typedef struct GenLuaStats {
    int firstTry;       /* script valido al primo tentativo */
    int afterRetry;      /* script valido dopo 1 o 2 ritenti */
    int optedOut;         /* il modello ha scelto esplicitamente "nessun comportamento speciale" (script sintatticamente valido ma senza nessuna callback) */
    int fellBack;          /* nessuno script valido entro i tentativi: l'oggetto resta sulla sola mini-VM */
    int skippedBudget;      /* mai tentato: budget di tempo della fase Lua esaurito (vedi GEN_LUA_PHASE_BUDGET_SEC) */
    int alreadyDone;         /* step B2: script gia' presente su disco (ripresa), saltato senza rigenerarlo */
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
/* 'firstFloors' (step B2, generazione pigra dei piani, roadmap punto 2): quanti
   piani generare, a partire dal primo. GEN_FLOORS = tutti (il comportamento di
   sempre). 1 = solo il piano che il giocatore giochera' SUBITO, cosi' la run
   parte dopo 4 script invece di 20 -- gli altri 16 li scrive un secondo processo
   in sottofondo mentre si gioca (vedi --resume in main.c).
   'publishPerFloor': se vero, il manifest viene PUBBLICATO (atomicamente) dopo
   ogni piano completato, invece che solo alla fine. E' cio' che permette al gioco,
   gia' in partita, di raccogliere gli script di un piano appena questo e' pronto
   (RunContentRefreshFloorScripts, src/content/run_content.c) invece di aspettare
   la fine dell'intera fase.
   Gli oggetti che hanno GIA' uno script (item->lua non vuoto, tipicamente caricato
   da disco con GenLuaLoadExisting sotto) vengono SALTATI: e' cio' che rende la
   ripresa idempotente e non fa rigenerare quello che c'e' gia'. */
void GenLuaGenerateForRun(GenLlmSession *sess, GenRun *run, const char *promptsDir,
                           const char *outDir, double deadline, int firstFloors,
                           bool publishPerFloor, GenLuaStats *stats);

/* Carica in 'run' gli script Lua GIA' scritti su disco da una generazione
   precedente (<outDir>/scripts/floorN_itemM.lua e floorN_bossItem.lua). Serve alla
   ripresa in sottofondo (step B2): il secondo processo deve (a) non rigenerare
   cio' che il primo ha gia' fatto, e (b) non PERDERLO -- se ricostruisse il
   manifest senza questi script, le righe ".lua=" del piano 1 sparirebbero e il
   gioco tornerebbe alla mini-VM per oggetti che avevano gia' il loro Lua.
   Ritorna quanti script ha caricato. */
int GenLuaLoadExisting(GenRun *run, const char *outDir);

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
   presenti in questo file (OpAllowsTrait e affini in gen_validate.c).
   'statUpOnly' falso (oggetto ATTIVO, il caso di --lua-check senza flag
   aggiuntivi) applica il gate INVERSO: rifiuta uno script che definisce
   on_evaluate. Non e' simmetria per il gusto della simmetria: a runtime
   (ScriptItemsRecomputeStats, src/script/script_items.c) un oggetto ATTIVO
   con on_evaluate passa SOLO dal tetto GLOBALE, mai dal tetto PER-OGGETTO
   riservato a ITEM_STATUP, quindi puo' spostare una statistica ben oltre il
   budget di un boss reward -- esattamente lo stat-up travestito da oggetto
   attivo che il cheat-sheet (lua_system.txt) gia' vieta a parole ma che,
   prima di questo gate, nessun controllo respingeva davvero (vedi il
   corpus tests/melting-gen/lua/active-item-on-evaluate.lua). Solo lato
   generatore: uno script scritto a mano resta libero di usare on_evaluate
   su un oggetto attivo. */
bool GenLuaValidate(const char *source, unsigned int seed, bool statUpOnly, bool *anyCallback, char *err, size_t errSize);

/* ============================================================
   Guardia byte-budget del prompt Lua (fase 3b review, "un guard automatico
   contro una futura re-inflazione"): vedi il commento sopra
   GEN_RARITY_PROMPT_HINTS in tools/melting-gen/gen_util.c per il bug REALE
   che questa guardia previene -- durante lo sviluppo della fase 3b, hint di
   rarita' scritti come frasi intere (invece delle frasi brevi di oggi)
   hanno fatto sforare GEN_LLM_SESSION_N_CTX per OGNI singolo prompt Lua di
   una run, mandando in fallback silenzioso tutti e 20 gli script (0 Lua):
   `make test-gen` restava verde lo stesso (non tocca mai il modello vero),
   solo un giro reale di `make test-llm` lo mostrava nel log. Questa guardia
   non serve il modello: compone il prompt Lua PIU' GRANDE che melting-gen
   possa costruire oggi con la stessa BuildLuaPrompt della generazione vera
   (gen_lua.c, non una sua reimplementazione: se BuildLuaPrompt cambia, la
   guardia la segue) e lo confronta a un ceiling in BYTE, cosi'
   `scripts/test-gen.sh` (senza modello) puo' farla fallire in CI.

   Perche' un ceiling in BYTE e non in token: contare i token veri richiede
   il tokenizer del modello (vocabolario GGUF), che scripts/test-gen.sh non
   carica di proposito (deve restare "senza modello", vedi il commento in
   cima allo script) -- caricare anche solo il vocabolario per ogni run di
   `make test-gen` e' un costo che qui non vale la pena pagare per una
   guardia cosi' economica. Un ceiling in byte, tarato una tantum a mano
   sotto, e' una stima sufficiente: falso-negativo occasionale su un prompt
   che cresce di poche decine di byte, ma cattura senza ambiguita' il tipo
   di regressione REALE del bug sopra (una frase intera al posto di un
   hint di poche parole).

   Derivazione del ceiling: GEN_LLM_SESSION_N_CTX (melting_gen.h, la
   sessione condivisa, 8192) meno GEN_LUA_N_PREDICT sopra (384) = 7808
   token che il PROMPT puo' occupare senza intaccare il budget riservato
   all'output generato -- lo stesso confine che GenLlmComplete gia' impone a
   runtime (gen_llm.c: "prompt+nPredict <= n_ctx", che pero' degrada sempre
   con un log+fallback, mai un crash: quella resta l'ultima rete di
   sicurezza, questa guardia serve solo a scoprirlo PRIMA, senza il
   modello). Convertiti in byte con ~3.2 caratteri/token: non e' un numero a
   caso, e' vicino al rapporto misurato UNA TANTUM (non ad ogni build, serve
   il modello vero) tokenizzando per davvero il prompt Lua piu' grande di
   oggi col vocabolario di Qwen2.5-Coder (vocab-only, ~5ms, nessuna
   inferenza):

     deps/llama.cpp/build/bin/test-tokenizer-0 \
       models/qwen2.5-coder-7b-instruct-q4_k_m.gguf <prompt-composto>.txt

   -> ri-misurato il 18/07/2026 dopo M3/DEC-052 (contenuto oggi inglese+Lua,
   non piu' italiano+Lua): 11662 byte = 3711 token reali (~3.14
   caratteri/token, praticamente invariato rispetto alla misura italiana
   precedente -- lua_system.txt, il cheat-sheet condiviso in inglese fin
   dalla fase 3a-L3, pesa la maggior parte del prompt piu' grande, ed e' lui
   a dominare il rapporto, non i pochi campi tradotti). Il margine sotto il
   ceiling in token oggi (7808-3711 = 4097 token, ampio) e' pero' un fatto
   di GEN_LLM_SESSION_N_CTX=8192: con l'n_ctx piu' piccolo di una fase
   precedente (4096, quindi budget 3712) lo stesso prompt sarebbe stato a
   UN SOLO token dal tetto -- non e' una scoperta di questa guardia, e' lo
   stesso motivo per cui GEN_RARITY_PROMPT_HINTS sono "VOLUTAMENTE brevi"
   (vedi gen_util.c). Il *16/5 sotto (= *3.2, interi per evitare float in
   una #define) resta scelto leggermente PIU' GENEROSO del rapporto
   misurato apposta, cosi' la guardia passa senza sfarfallare a ogni piccola
   modifica cosmetica del prompt qualunque sia l'n_ctx del momento, restando
   comunque ben sotto l'intero n_ctx e capace di intercettare senza
   ambiguita' una vera re-inflazione (centinaia di byte in piu', non
   decine). */
#define GEN_LUA_PROMPT_BYTE_CEILING (((GEN_LLM_SESSION_N_CTX) - (GEN_LUA_N_PREDICT)) * 16 / 5)

/* Compone il prompt Lua piu' grande possibile (entrambe le categorie,
   attivo/stat-up: usa il template piu' grande delle due; hint di rarita'
   piu' lungo fra i quattro di GEN_RARITY_PROMPT_HINTS; un contesto oggetto
   rappresentativo, non i limiti di campo estremi) leggendo i prompt da
   'promptsDir', e fallisce se supera GEN_LUA_PROMPT_BYTE_CEILING. Nessun
   modello coinvolto (vedi il commento sopra). 'err' (puo' essere NULL/
   errSize 0) riceve il motivo del fallimento (file mancanti o ceiling
   superato, con le due dimensioni per il log). Ritorna true se il prompt
   composto sta dentro il ceiling. */
bool GenLuaPromptBudgetCheck(const char *promptsDir, char *err, size_t errSize);

#endif
