#ifndef MELTING_GEN_H
#define MELTING_GEN_H

#include <stddef.h>
#include <stdio.h>

#define GEN_FLOORS 5
#define GEN_ITEMS 3
#define GEN_MAX_OPS 3

/* Penalita' sulle ripetizioni del campionamento (vedi gen_llm.c).
   La finestra copre circa un piano e mezzo di JSON, cosi' il modello "vede" i nomi
   che ha appena usato; il valore e' volutamente mite, perche' penalizza anche i
   token strutturali del JSON (virgolette, parentesi) che pero' la grammatica
   protegge comunque. */
#define GEN_PENALTY_LAST_N 256
#define GEN_PENALTY_REPEAT 1.08f

/* n_ctx/n_batch della sessione condivisa (fase 3a-L3, gen_llm.c): fissi per
   l'intero processo, devono coprire la chiamata piu' grande fra le due che
   la sessione serve. JSON: prompt di poche centinaia di token + nPredict
   fino a 2048 (args.nPredict di default). Lua: prompt piu' grande
   (cheat-sheet + few-shot, vedi tools/melting-gen/prompts/lua_system.txt) ma
   nPredict molto piu' corto (GEN_LUA_N_PREDICT in gen_lua.h). 4096 tiene
   comodamente entrambe con margine.

   n_batch = n_ctx (non piu' 2048, fase 3): GenLlmComplete sottomette l'intero
   prompt in un colpo solo con llama_batch_get_one (vedi gen_llm.c), e
   llama_decode ha un'asserzione interna "n_tokens_all <= cparams.n_batch"
   che NON e' la stessa cosa del controllo "prompt+nPredict <= n_ctx" gia'
   presente in GenLlmComplete (quello logga un errore e ritorna -1, questo
   fa un ggml_abort() e uccide l'intero processo). Scoperto in fase 3
   (docs/superpowers/sdd/phase3-items-report.md): il cheat-sheet Lua e' cresciuto
   (tavolozza di archetipi per gli oggetti attivi + il prompt dedicato agli
   oggetti stat-up) fino a superare 2048 token pur restando ben sotto n_ctx,
   e la sessione condivisa e' andata in crash A META' RUN (dopo il JSON,
   durante il primo item Lua), perdendo l'intera generazione. n_batch = n_ctx
   rende il controllo "prompt+nPredict <= n_ctx" gia' esistente l'UNICO
   vincolo binding: qualunque prompt che lo supera degrada gia' con un
   log+fallback (mai un crash), qualunque prompt che lo rispetta non puo'
   piu' far scattare l'asserzione di llama_decode. Il costo in VRAM e'
   trascurabile: il buffer di calcolo riservato da llama.cpp e' dimensionato
   su n_ubatch (default 512, non toccato qui), non su n_batch.

   Spostate qui da gen_llm.c in fase 3b review (guardia byte-budget del
   prompt Lua, gen_lua.h): GEN_LUA_PROMPT_BYTE_CEILING li' deriva da
   GEN_LLM_SESSION_N_CTX e da gen_lua.h non si puo' includere llama.h (lo fa
   solo gen_llm.c) solo per due #define. melting_gen.h e' incluso da
   entrambi, quindi resta l'unico posto senza dipendenze in piu'. */
#define GEN_LLM_SESSION_N_CTX   4096
#define GEN_LLM_SESSION_N_BATCH GEN_LLM_SESSION_N_CTX

/* Budget di tempo ASSOLUTO (secondi dall'avvio del processo, confrontato con
 * GenNowSeconds()) per la fase Lua (fase 3a-L3, gen_lua.c): oltre questa
 * soglia gli oggetti non ancora tentati restano senza Lua (mini-VM, nessun
 * errore) invece di continuare a provare. Esiste perche' src/app/app.c
 * (AppStartGeneration) manda SIGTERM al processo dopo un tetto fisso (vedi
 * il commento li': alzato da 180s a 420s proprio per questa fase) e la run
 * generata finora (JSON + atlas + quello che di Lua e' gia' pronto) va
 * comunque scritta su disco PRIMA che quel SIGTERM arrivi, non persa. 300s
 * lascia 120s di margine sotto i 420s per: gli ultimi item Lua in corso,
 * la scrittura di manifest/atlas/script, e un margine di sicurezza. */
#define GEN_LUA_PHASE_BUDGET_SEC 300.0

typedef struct GenScriptOp {
    char trigger[10];   /* "on_fire" | "on_hit" */
    char op[12];        /* "burst" | "projectile" | "area" | "heal" */
    double a;
    double b;
    char trait[10];     /* uno dei GEN_TRAITS oppure "none" */
} GenScriptOp;

/* Sorgente Lua opzionale dell'oggetto (fase 3a-L3, vedi
 * docs/superpowers/specs/2026-07-13-lua-sandbox-design.md sezioni 6,9 e
 * gen_lua.h). Deve restare comodamente sotto SCRIPT_LUA_LEN=2048 di
 * core/game_types.h (il campo del lato gioco che la ospita): GenLuaValidate
 * rifiuta qui in melting-gen qualunque script che sfori questo margine,
 * cosi' il gioco non vede mai un file troncato a meta'. Vuota ("") per un
 * oggetto che resta sulla sola mini-VM (il caso di oggi, e il ripiego di
 * ogni tentativo Lua fallito). */
#define GEN_LUA_LEN 2000

/* "active" | "statup" (vedi GEN_KINDS in gen_util.c): mai scritto dal
 * modello (non fa parte della grammatica JSON, run.gbnf), sempre deciso in C
 * -- "active" per ogni GenItem dentro items[] (assegnato in
 * GenNormalizeRun/GenFallbackRun), "statup" per bossItem sotto. Esiste
 * comunque come campo testuale (non un bool) per lo stesso motivo di
 * slot/traits: e' cio' che gen_manifest.c scrive alla lettera nel manifest,
 * e cio' che run_content.c rilegge con lo stesso schema chiave=valore. */
typedef struct GenItem {
    char name[48];      /* stesso limite di Item.name in game_types.h */
    char slot[8];       /* uno dei GEN_SLOTS */
    char traits[2][10];
    int traitCount;     /* 1..2 */
    char color[8];      /* "#rrggbb" */
    char kind[8];        /* uno dei GEN_KINDS */
    /* Rarita' (fase 3b, docs/superpowers/specs/2026-07-13-pools-rarity-design.md):
     * uno dei GEN_RARITIES sotto. Stesso trattamento testuale di 'kind' e
     * per lo stesso motivo: mai scritta dal modello (non fa parte della
     * grammatica JSON, run.gbnf), sempre tirata in C da una tabella di pesi
     * PER POOL (GenRollRarity in gen_util.c) -- items[] tira dalla tabella
     * tesoro/negozio, bossItem SEMPRE dalla tabella boss (mai comune/non-
     * comune, vedi FallbackBossItem in gen_fallback.c). */
    char rarity[12];
    GenScriptOp ops[GEN_MAX_OPS];
    int opCount;        /* 1..3; sempre 0 per un oggetto stat-up (nessun comportamento mini-VM, vedi bossItem) */
    char lua[GEN_LUA_LEN];
} GenItem;

typedef struct GenFloor {
    char theme[64];
    char style[48];
    char boss[64];
    char bg[8], floorColor[8], wall[8], accent[8], accent2[8], enemy[8], bossColor[8];
    GenItem items[GEN_ITEMS];   /* oggetti ATTIVI: la stessa grammatica JSON di sempre (run.gbnf), il modello li scrive */
    /* Oggetto STAT-UP del piano, ricompensa del boss (fase 3, vedi
     * docs/superpowers/specs/2026-07-13-items-synergy-vision.md sezioni
     * 1,2,5): campo esplicito, non un quarto elemento di items[] (stessa
     * scelta e stessa motivazione del lato gioco, vedi FloorContent in
     * core/game_types.h). Nome/slot/colore/trait sono generati
     * DETERMINISTICAMENTE dal seed della run (GenFallbackRun, riusato anche
     * da GenNormalizeRun quando il JSON del modello non lo prevede: non fa
     * parte della grammatica, per non dover ritoccare run.gbnf/system.txt/
     * user.txt in questa fase): stessa qualita' procedurale di ogni altro
     * contenuto di ripiego, MAI un doppione degli oggetti attivi. Cio' che e'
     * davvero scritto dal modello, quando c'e', e' il suo comportamento
     * on_evaluate (campo 'lua' sotto), con un prompt dedicato (vedi
     * gen_lua.c, prompts/lua_statup_user.txt): "mai un doppione" del
     * comportamento, solo numeri, bilanciati in C (src/script/script_items.c). */
    GenItem bossItem;
} GenFloor;

typedef struct GenRun {
    char source[96];
    unsigned int seed;
    GenFloor floors[GEN_FLOORS];
} GenRun;

typedef struct GenTraitRule {
    const char *trait;
    const char *trigger;
    const char *op;
    double a;
    double b;
} GenTraitRule;

/* gen_util.c */
unsigned int GenRngNext(unsigned int *state);
int GenRngRange(unsigned int *state, int min, int max);
void GenHsvToHex(double h, double s, double v, char out[8]);
int GenEnsureDir(const char *path);
char *GenReadFile(const char *path);   /* buffer malloc terminato da zero, NULL su errore */
int GenFileExists(const char *path);
void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message);
void GenLogLine(const char *fmt, ...);
/* Orologio monotono in secondi (CLOCK_MONOTONIC): usato sia per i tempi di
 * generazione (gia' prima di questa fase) sia per il budget di tempo della
 * fase Lua (fase 3a-L3, vedi gen_lua.h e GEN_LUA_PHASE_BUDGET_SEC sotto). */
double GenNowSeconds(void);
/* Sostituisce OGNI occorrenza di 'find' in 'src' con 'repl' in un nuovo
 * buffer malloc (mai in place: 'src' e' spesso lo stesso template riusato
 * per piu' chiamate, es. i placeholder di gen_lua.c). 'find' vuoto o NULL
 * ritorna una copia invariata di 'src'. NULL su fallimento di allocazione o
 * se 'src' e' NULL. */
char *GenReplaceAll(const char *src, const char *find, const char *repl);
/* Prompt ChatML di Qwen2.5 (stesso formato gia' usato per il JSON dei
 * piani): "<|im_start|>system\n...<|im_end|>\n<|im_start|>user\n...<|im_end|>\n<|im_start|>assistant\n".
 * Buffer malloc, NULL su fallimento o argomenti NULL. */
char *GenChatMlWrap(const char *sys, const char *user);
/* Pubblica atomicamente un file di output "definitivo" (manifest, JSON di
 * gioco, atlas BMP): f e' gia' stato aperto su tmpPath e scritto dal
 * chiamante. Controlla gli errori di scrittura, chiude il file, e solo se
 * tutto e' andato bene fa rename() su finalPath. Su qualunque errore rimuove
 * tmpPath e non tocca finalPath (che quindi resta quello valido di prima).
 * Stesso pattern tmp+rename di GenProgressWrite qui sopra. */
int GenPublishFile(FILE *f, const char *tmpPath, const char *finalPath);
extern const char *GEN_SLOTS[6];
extern const char *GEN_TRAITS[9];
extern const char *GEN_KINDS[2];   /* "active", "statup": vedi il commento su GenItem.kind sopra */
const GenTraitRule *GenTraitRuleFor(const char *trait);   /* NULL se sconosciuto */

/* Rarita' (fase 3b design doc, sezioni 1-3): MODIFICA QUI (gen_util.c) per
 * ribilanciare/espandere. GEN_RARITIES e' l'ordine canonico (indice 0..3 =
 * comune..leggendario, stesso ordine dell'enum Rarity in core/game_types.h,
 * vedi RarityFromText in run_content.c che deve restare sincronizzato a
 * mano). GEN_RARITY_PROMPT_HINTS e' la frase di intensita' (in italiano) che
 * gen_lua.c inietta nel prompt per-oggetto: un comune chiede numeri piccoli,
 * un leggendario numeri grandi, MAI un secondo effetto (vedi i prompt in
 * tools/melting-gen/prompts/). */
extern const char *GEN_RARITIES[4];
extern const char *GEN_RARITY_PROMPT_HINTS[4];
/* Tira una rarita' pesata (design sezione 3): isBoss=0 pesca dalla tabella
 * tesoro/negozio (55/30/12/3), isBoss!=0 dalla tabella boss (0/0/70/30,
 * SEMPRE raro o leggendario). Ritorna un indice 0..3 in GEN_RARITIES
 * (Rarity-compatibile, ma qui e' un int semplice: GenItem tiene solo il
 * testo, vedi il commento su GenItem.rarity in melting_gen.h). Stesso
 * algoritmo RNG di GenRngRange sopra: deterministico, stesso seed -> stessa
 * rarita' (vedi scripts/test-gen.sh). */
int GenRollRarity(unsigned int *rng, int isBoss);
/* Indice 0..3 in GEN_RARITIES per il testo dato, -1 se sconosciuto (usato da
 * gen_lua.c per scegliere la frase giusta in GEN_RARITY_PROMPT_HINTS). */
int GenRarityIndexFromText(const char *text);

/* gen_fallback.c */
void GenFallbackRun(GenRun *run, unsigned int seed);

/* gen_manifest.c */
int GenWriteRunFiles(const GenRun *run, const char *outDir);
int GenWriteLlmJson(const GenRun *run, const char *path);

/* gen_atlas.c */
int GenWriteAtlasBmp(const GenRun *run, const char *outDir);

/* gen_validate.c (Task 6) */
struct cJSON;
void GenNormalizeRun(const struct cJSON *raw, unsigned int seed, GenRun *out);

/* gen_llm.c: sessione = modello + contesto caricati UNA SOLA VOLTA per
 * l'intero processo (fase 3a-L3). Prima di questa fase melting-gen faceva
 * un caricamento per ogni tentativo di generazione JSON (fino a 2): con
 * l'aggiunta dei 15 script Lua per run, ricaricare il modello ad ogni
 * generazione indipendente costerebbe ~2.6s x 15 sul 7B (docs/BENCHMARKS.md),
 * tempo tolto al budget di 180s che il gioco concede al processo (vedi
 * src/app/app.c, AppStartGeneration) prima di mandargli SIGTERM. Una
 * sessione condivisa paga quel costo una volta sola. */
typedef struct GenLlmSession GenLlmSession;

/* Carica il modello e crea un contesto con n_ctx/n_batch abbastanza larghi
 * da coprire sia il prompt JSON (grammatica GBNF, nPredict fino a 2048) sia
 * i prompt Lua (cheat-sheet + few-shot, nPredict piu' corto): vedi le
 * costanti GEN_LLM_SESSION_* in gen_llm.c. 'outDir' e' solo per il progress
 * callback di caricamento (percentuale 0-60, come prima). Ritorna NULL su
 * fallimento (gia' loggato). */
GenLlmSession *GenLlmSessionOpen(const char *modelPath, int nGpuLayers, const char *outDir);
void GenLlmSessionClose(GenLlmSession *sess);

/* Un completamento indipendente: azzera la cache KV prima di generare (ogni
 * chiamata e' una conversazione a se', mai il turno successivo di quella
 * prima), poi campiona fino a 'nPredict' token o fino a un token di fine
 * sequenza. 'grammarText' NULL = campionamento libero (percorso Lua, spec
 * sezione 6: una grammatica GBNF per Lua completo non si puo' esprimere,
 * vedi gen_lua.c); non-NULL = stesso percorso a grammatica del JSON di oggi.
 * 'outDir'/'progressPhase' possono essere NULL per non scrivere progresso.
 * Ritorna 0 su successo, -1 su errore (gia' loggato); 'out' e' sempre
 * terminata da zero (stringa vuota su errore). */
int GenLlmComplete(GenLlmSession *sess, const char *prompt, const char *grammarText,
                    int nPredict, float temp, unsigned int seed,
                    const char *outDir, const char *progressPhase, int progressBase, int progressSpan,
                    char *out, size_t outCap, int *tokensOut);

/* Prompt ChatML per il JSON dei piani: legge prompts/system.txt e
 * prompts/user.txt da 'promptsDir', sostituisce {SEED}. Buffer malloc, NULL
 * su fallimento (file mancanti). */
char *GenLlmBuildJsonPrompt(const char *promptsDir, unsigned int seed);

#endif
