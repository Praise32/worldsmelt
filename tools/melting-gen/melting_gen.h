#ifndef MELTING_GEN_H
#define MELTING_GEN_H

/* Tipi di colpo (step C): la STESSA definizione e la STESSA funzione di
 * bilanciamento del gioco (src/core/shot_type.h, compilato dentro melting-gen
 * via GEN_EXTRA_SRC nel Makefile). Quell'header non include raylib ne'
 * game_types.h apposta, proprio per poter vivere anche qui dentro: e' l'unico
 * pezzo di src/core/ che melting-gen include, e la ragione e' che un tipo di
 * colpo lo inventa il MODELLO -- se generatore e gioco avessero due definizioni
 * separate (come e' successo per rarity/kind, due elenchi di stringhe da tenere
 * sincronizzati a mano) un giorno divergerebbero in silenzio. */
#include "core/character_type.h"
#include "core/enemy_type.h"
#include "core/room_layout.h"
#include "core/shot_type.h"

#include <stddef.h>
#include <stdio.h>

#define GEN_FLOORS 5
#define GEN_ITEMS 3
#define GEN_MAX_OPS 3

/* Penalita' sulle ripetizioni del campionamento (vedi gen_llm.c).
   La finestra deve coprire PIU' DI UN PIANO di JSON, altrimenti il modello non
   viene mai penalizzato per ricopiare il piano precedente -- ed e' esattamente
   cio' che fa: con 256 token e piani da ~370 (misurati col 7B dopo l'aggiunta dei
   tipi di colpo, step C: 1851 token per 5 piani) una generazione su alcuni seed
   produceva CINQUE PIANI FOTOCOPIA, stesso tema, stesso boss, stessi oggetti.
   Non e' una regressione teorica: si vedeva in una run vera (seed 20260714).
   2048 copre l'INTERA generazione (1851 token misurati), non "abbastanza": provato
   prima a 1024 (~tre piani) e non bastava -- il quinto piano ricopiava il PRIMO, che
   a quel punto era gia' fuori finestra. Resta comunque solo la prima delle due
   difese: la seconda, che e' quella che GARANTISCE, e' la rete anti-fotocopia in
   gen_validate.c (DedupeFloors), perche' il campionamento si puo' rendere
   improbabile, mai impossibile. Il
   valore della penalita' resta mite: penalizza anche i token STRUTTURALI del JSON
   (virgolette, parentesi), ma quelli la grammatica li impone comunque -- a quel
   punto della sequenza sono gli unici token legali, quindi la penalita' non puo'
   fare danno, puo' solo spostare la scelta dei CONTENUTI. */
#define GEN_PENALTY_LAST_N 2048
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
   entrambi, quindi resta l'unico posto senza dipendenze in piu'.

   8192 (era 4096 prima dello step C, 6144 dopo). Ogni fase che aggiunge contenuto
   inventato dal modello fa crescere il JSON: i tipi di colpo (+25%), poi i nemici
   della fase 3b (due nemici + un boss per piano). Misurato col 7B: 2796 token per
   una run completa. Con nPredict=3072 il margine era di 276 token -- di nuovo
   troppo poco, e un troncamento non e' un degrado ma un BUCO (la grammatica rende
   il JSON troncato non parsabile, e la run finisce sul ripiego procedurale in
   silenzio). Ora nPredict=4096 e il prompt ~2300: 6400 su 8192, con margine vero.
   Il costo in VRAM e' misurato, non stimato: la KV cache di questo modello e'
   ~57 KB/token, quindi 8192 token = ~467 MiB. Col modello (4.53 GiB) e il buffer di
   calcolo (~300 MiB) si resta sotto i 6 GiB della scheda di riferimento -- ed e'
   verificato con una generazione vera, non solo con l'aritmetica.
   La ragione originale del primo aumento (dopo lo step C): i tipi di colpo hanno fatto crescere il
   JSON del ~25% (1851 token misurati col 7B contro i ~1450 di prima), e con
   nPredict=2048 il margine era sceso a ~200 token -- un tema dai nomi lunghi
   avrebbe troncato il JSON, che la grammatica rende non parsabile, mandando la run
   sul ripiego procedurale IN SILENZIO. Alzare n_ctx costa poco (la KV cache di
   questo modello e' ~57 KB/token: 2048 token in piu' = ~117 MiB, su una scheda che
   ne ha 6144) e paga due volte: da' respiro a nPredict (ora 2560, vedi main.c) e
   rilassa il budget di byte del prompt Lua, che era arrivato a ~14 token di
   margine (vedi GEN_LUA_PROMPT_BYTE_CEILING in gen_lua.h -- e' proprio l'"alzare
   n_ctx" che HANDOFF.md indicava come la via d'uscita quando quel tetto fosse
   diventato stretto). */
#define GEN_LLM_SESSION_N_CTX   8192
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
    /* Tipo di colpo del piano (step C, spec 2026-07-14-step-c-shottype-balance.md):
     * A DIFFERENZA di kind/rarity (decisioni di bilanciamento, sempre prese in C,
     * mai dal modello) questo lo scrive IL MODELLO -- fa parte della grammatica
     * JSON (run.gbnf, chiave "shot"), perche' e' contenuto creativo puro: nome,
     * forma e numeri di un modo nuovo di sparare. Il C non ne giudica il gusto,
     * ne garantisce solo l'equilibrio (ShotTypeBalance, core/shot_type.c) e la
     * sicurezza (le bande di ShotTypeClamp).
     * 'shotItem' (1..3, chiave "shotItem" del JSON) e' QUALE dei tre oggetti
     * attivi del piano lo conferisce: lo sceglie il modello, cosi' puo' dare il
     * tipo di colpo all'oggetto il cui nome ha piu' senso ("Guanto di Chiodi" ->
     * spara chiodi). Mai il bossItem: uno stat-up e' solo numeri. */
    ShotTypeDef shot;
    int shotItem;
    /* Tipi di nemico del piano (fase 3b): due nemici normali + il boss. Come il
     * tipo di colpo, li scrive IL MODELLO (fanno parte della grammatica JSON): sono
     * contenuto creativo, non bilanciamento. Il C ne garantisce solo l'equilibrio
     * (EnemyTypeBalance) e, in gioco, il budget di difficolta' della stanza. */
    EnemyTypeDef enemies[2];
    EnemyTypeDef bossType;
    /* Layout delle stanze del piano (fase 3c): lo scrive il modello (grammatica
     * JSON, chiave "room"). Il C garantisce solo densita' e forma valide
     * (RoomLayoutClamp); la giocabilita' vera la garantisce RoomLayoutBuild lato
     * gioco. */
    RoomLayoutDef roomLayout;
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

/* M5 (DEC-005, scelta del tema nel Piano 0): dimensioni condivise fra le
 * proposte di tema (GenThemeProposal, scritte in generated/theme_proposals.json)
 * e il tema SCELTO (GenChosenTheme, letto da --theme-file): stesso "name"
 * (3-40 char, il charset di 'name' in propose.gbnf/run.gbnf) e stesso
 * "blurb" (una riga ASCII, vedi propose.gbnf). GEN_THEME_PROPOSALS e' sia il
 * numero di proposte che la grammatica genera SEMPRE (root fissa a 3, vedi
 * propose.gbnf) sia la capacita' massima degli array di questo modulo -- N
 * (2..3, il parametro di --propose-themes) sceglie solo quante di quelle 3
 * il chiamante vuole vedere scritte su disco, vedi RunProposeThemes in main.c. */
#define GEN_THEME_PROPOSALS 3
#define GEN_THEME_NAME_LEN 48
#define GEN_THEME_BLURB_LEN 200

typedef struct GenThemeProposal {
    char name[GEN_THEME_NAME_LEN];
    char blurb[GEN_THEME_BLURB_LEN];
} GenThemeProposal;

/* Il tema scelto dal giocatore, letto da --theme-file (GenLoadChosenTheme,
 * gen_util.c): 'raw' e' il testo INTERO cosi' com'e' nel file ("<name> --
 * <blurb>", stesso formato di provenance.txt chosenTheme=), usato alla
 * lettera per {CHOSEN_THEME} (gen_llm.c) e per provenance.txt; 'name'/'blurb'
 * sono la stessa coppia spezzata, usata da GenFallbackRun (solo 'name': il
 * blurb non entra mai nel contenuto procedurale, e' materiale di prompt).
 * name[0]=='\0' = nessun tema (equivalente a un puntatore NULL per chi legge
 * solo 'name', ma 'raw' resta la fonte di verita' per il testo completo). */
typedef struct GenChosenTheme {
    char name[GEN_THEME_NAME_LEN];
    char blurb[GEN_THEME_BLURB_LEN];
    char raw[GEN_THEME_NAME_LEN + GEN_THEME_BLURB_LEN + 8];
} GenChosenTheme;

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
/* Step B2 (correzione da review): cancella gli script Lua di una generazione
 * precedente. Da chiamare all'inizio di ogni generazione NON di ripresa -- vedi il
 * commento nel .c per il bug silenzioso che chiude (una run nuova che adotta gli
 * script della run di ieri). */
void GenRemoveOldScripts(const char *outDir);
/* M5: legge 'path' (il file scritto dal gioco, generated/chosen_theme.txt,
 * o qualunque altro passato a --theme-file) e lo spezza in name/blurb/raw.
 * Formato atteso: UNA riga "<name> -- <blurb>" (lo stesso separatore " -- "
 * usato da provenance.txt chosenTheme= e dal gioco, src/app/app.c
 * AppWriteChosenThemeFile), newline finale opzionale. Ritorna false (out non
 * garantito valido) se il file manca, e' vuoto, o non contiene " -- ": il
 * chiamante (main.c) tratta questo esattamente come --theme-file assente,
 * mai un errore fatale -- coerente con "se il flag manca, comportamento
 * attuale" del requisito 3 della spec M5. */
bool GenLoadChosenTheme(const char *path, GenChosenTheme *out);

/* FNV-1a 64 bit (RunBundle v1, roadmap 16/07/2026 settimana 4): NON e' un
 * hash crittografico, e' una checksum veloce. Usata SOLO per la provenienza
 * (generated/provenance.txt, chiave promptsFnv=...): capire "con quali
 * prompt e' nata questa run", non per verificarne l'integrita' -- quella la
 * fa scripts/bundle-export.sh/-import.sh con sha256sum, molto piu' robusto
 * contro corruzione o manomissione. 'hash' e' lo stato corrente
 * (GEN_FNV1A64_OFFSET per iniziare una sequenza nuova); incatenando piu'
 * chiamate su pezzi consecutivi si ottiene lo STESSO risultato di un'unica
 * chiamata sul buffer concatenato (e' la proprieta' che rende FNV-1a
 * "streamabile": lo sfrutta GenPromptsFnv sotto per non dover concatenare i
 * file dei prompt in un unico buffer malloc). */
#define GEN_FNV1A64_OFFSET 14695981039346656037ULL
unsigned long long GenFnv1a64(unsigned long long hash, const void *data, size_t len);
/* Concatena, in ORDINE ALFABETICO di nome file (requisito del formato di
 * provenance.txt: la stessa cartella prompts/ deve produrre sempre lo stesso
 * hash, indipendentemente dall'ordine con cui il filesystem restituisce
 * readdir()), il contenuto di ogni file REGOLARE dentro 'promptsDir' e ne
 * calcola l'FNV-1a 64 con GenFnv1a64 sopra. Scrive il risultato in '*out'.
 * Ritorna 0 su successo, -1 se la cartella non esiste, e' vuota, o un file
 * non si legge. */
int GenPromptsFnv(const char *promptsDir, unsigned long long *out);
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

/* gen_fallback.c. 'chosen' (M5): NULL = comportamento di sempre, IDENTICO
 * byte-per-byte (stesso stream RNG: il golden file di regressione non deve
 * cambiare, vedi il commento sul parametro dentro il .c). Non-NULL:
 * floor[0].theme = chosen->name; i piani 2-5 sono lo stesso nome piu' un
 * suffisso di stadio curato (stageSuffixes, sotto), 4 estratti DISTINTI da
 * un RNG DEDICATO che non tocca affatto lo stream usato quando chosen e'
 * NULL. */
void GenFallbackRun(GenRun *run, unsigned int seed, const GenChosenTheme *chosen);

/* Proposte di tema deterministiche (M5, fallback permanente DEC-039/DEC-070):
 * pesca 'count' (1..GEN_THEME_PROPOSALS) coppie name/blurb DISTINTE dal pool
 * curato -- il "name" dal pool themeWords x weirdWords (lo stesso di
 * GenFallbackRun quando chosen e' NULL, mai una lista a parte), il "blurb"
 * dai 32 blurb curati dal content designer (logs/m5-content-notes.md,
 * scritti per accompagnare QUALSIASI combinazione). RNG dedicato, seed-only:
 * non tocca ne' e' toccato dallo stream di GenFallbackRun (stessa garanzia
 * di 'chosen' sopra). 'out' deve avere almeno GEN_THEME_PROPOSALS slot
 * (i primi 'count' vengono scritti, gli altri restano intoccati). */
void GenFallbackThemeProposals(unsigned int seed, int count, GenThemeProposal out[GEN_THEME_PROPOSALS]);

/* Budget di tempo della fase Lua in RIPRESA (step B2, processo in sottofondo
 * mentre si gioca): piu' largo di GEN_LUA_PHASE_BUDGET_SEC perche' qui nessuno
 * sta aspettando davanti a una barra di caricamento -- il giocatore sta gia'
 * giocando, e il costo di arrendersi troppo presto e' un piano che resta senza
 * script Lua (mini-VM) per il resto della run. Resta comunque un tetto: il
 * processo non deve vivere per sempre se il modello si impianta. */
#define GEN_LUA_RESUME_BUDGET_SEC 600.0

/* gen_manifest.c */
int GenWriteRunFiles(const GenRun *run, const char *outDir);
int GenWriteRunFilesResume(const GenRun *run, const char *outDir);   /* step B2: vedi il commento nel .c */
int GenWriteLlmJson(const GenRun *run, const char *path);
/* generated/provenance.txt (RunBundle v1, roadmap 16/07/2026 settimana 4):
 * scrittura atomica (tmp+rename via GenPublishFile) come tutti gli altri
 * file di outDir. Il chiamante (main.c) la invoca SOLO a fine di una
 * generazione NORMALE o FALLBACK, MAI in --resume: la ripresa appartiene
 * alla STESSA run del processo che l'ha aperta, provenance.txt esiste gia'
 * (scritto la prima volta) e NON va toccato -- vedi il blocco --resume in
 * main.c, che semplicemente non chiama questa funzione.
 * 'modelJsonField'/'modelLuaField' sono gia' risolti dal chiamante (main.c
 * e' l'unico punto che sa quale ramo -- sessione unica, esperimento
 * due-modelli, --from-json, o ripiego procedurale -- ha prodotto la run):
 * il percorso del modello .gguf usato, il percorso del file --from-json
 * quando il JSON viene da li' e non da un modello vivo in questo processo,
 * oppure i letterali "fallback" (modelJson, nessun modello: GenFallbackRun)
 * o "-" (modelLua, nessuna fase Lua eseguita in questo processo).
 * 'promptsDir' e' la cartella di cui si calcola promptsFnv (GenPromptsFnv
 * sopra): se il calcolo fallisce (cartella prompt mancante) si scrive
 * comunque il file, con promptsFnv=0000000000000000, piuttosto che far
 * fallire un'intera generazione per un dettaglio diagnostico -- e' loggato
 * (GenLogLine), non silenzioso. 'chosenThemeField' (M5, requisito 6): il
 * testo intero del tema scelto (GenChosenTheme.raw) o NULL quando
 * --theme-file non e' stato passato -- scritto sempre come riga
 * "chosenTheme=<...>", col letterale "none" quando NULL (stessa scelta di
 * modelJson/modelLua sopra: mai una riga assente, sempre un valore da
 * fare grep). Ritorna 0 su successo, -1 su errore di scrittura (outDir non
 * creabile, disco pieno...). */
int GenWriteProvenance(const GenRun *run, const char *outDir, const char *promptsDir,
                        const char *modelJsonField, const char *modelLuaField,
                        const char *chosenThemeField);

/* M5: scrive generated/theme_proposals.json (tmp+rename, come ogni altro
 * output di questo modulo) con le prime 'count' proposte di 'proposals' e il
 * campo "source" (letterale "local:<modello>" o "fallback", stesso
 * vocabolario di GenRun.source). Ritorna 0 su successo, -1 su errore di
 * scrittura. */
int GenWriteThemeProposals(const GenThemeProposal *proposals, int count, const char *source, const char *outDir);

/* M6b-1 (DEC-014, prima fetta): scrive generated/character_proposal.json
 * (tmp+rename, come ogni altro output di questo modulo) col personaggio
 * alternativo per-run gia' CLAMPATO (CharacterGenDefClamp, prima rete di
 * sicurezza -- la seconda e' src/content/character_proposal.c alla lettura)
 * e il campo "source" (stesso vocabolario di GenRun.source/
 * GenWriteThemeProposals: "local:<modello>"). Ritorna 0 su successo, -1 su
 * errore di scrittura. Chiamata SOLO su successo della generazione: il
 * fallback canonico e' l'ASSENZA del file (characters.md, "Fallback"), mai
 * un personaggio curato di riserva -- vedi RunProposeCharacter in main.c. */
int GenWriteCharacterProposal(const CharacterGenDef *def, const char *source, const char *outDir);

/* gen_atlas.c */
int GenWriteAtlasBmp(const GenRun *run, const char *outDir);

/* gen_validate.c (Task 6). 'chosen' (M5): passato pari pari a GenFallbackRun
 * (sotto forma del ripiego 'fb' interno usato per il riempimento per-campo):
 * un JSON del modello che manca/rifiuta il tema su un piano ricade cosi'
 * sul ripiego GIA' coerente col tema scelto, invece che su un nome
 * procedurale slegato. */
struct cJSON;
void GenNormalizeRun(const struct cJSON *raw, unsigned int seed, const GenChosenTheme *chosen, GenRun *out);

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
 * prompts/user.txt da 'promptsDir', sostituisce {SEED}/{ISPIRAZIONI}/{EVITA}
 * e, da M5, {CHOSEN_THEME} (prompts/user.txt riga 7): 'chosen' NULL o
 * chosen->raw vuoto sostituisce con la frase di degrado "not chosen this
 * time -- invent one yourself..."; non-NULL con "<name> -- <blurb>. Stay
 * inside this world...". Il placeholder e' SEMPRE sostituito, mai lasciato
 * nel prompt (vedi logs/m5-content-notes.md per il testo esatto dei due
 * rami, scritto dal content designer). Buffer malloc, NULL su fallimento
 * (file mancanti). */
char *GenLlmBuildJsonPrompt(const char *promptsDir, unsigned int seed, const GenChosenTheme *chosen);

/* M5: prompt ChatML per --propose-themes, legge prompts/propose_system.txt e
 * prompts/propose_user.txt (stesso schema di placeholder di
 * GenLlmBuildJsonPrompt: {SEED}/{ISPIRAZIONI}/{EVITA}, MAI {CHOSEN_THEME} --
 * qui non c'e' ancora un tema scelto, e' proprio questo prompt che ne
 * propone). Buffer malloc, NULL su fallimento (file mancanti). */
char *GenLlmBuildProposePrompt(const char *promptsDir, unsigned int seed);

/* M6b-1 (DEC-014, prima fetta): prompt ChatML per il personaggio alternativo
 * per-run, legge prompts/propose_character_system.txt e prompts/
 * propose_character_user.txt -- SOLO {SEED} (niente {ISPIRAZIONI}/{EVITA}/
 * {CHOSEN_THEME}: il personaggio non dipende dal tema, si genera PRIMA
 * della scelta, vedi RunProposeThemes in main.c). Buffer malloc, NULL su
 * fallimento (file mancanti). */
char *GenLlmBuildCharacterPrompt(const char *promptsDir, unsigned int seed);

/* ============================================================
 * Riuso del prefisso condiviso nella KV cache (fase 3b step B1, misurato:
 * ~9.6s di ogni ~10-11s per oggetto Lua sono il RIPROCESSAMENTO del
 * cheat-sheet di sistema (~3700 token, quasi il n_ctx=4096 della sessione),
 * identico per i 20 oggetti di una run, e GenLlmComplete lo rifaceva da capo
 * ad ogni chiamata (llama_memory_clear in testa). Le tre funzioni sotto
 * spezzano quel lavoro: il prefisso si decodifica UNA SOLA VOLTA
 * (GenLlmPrefixPrime), ogni oggetto decodifica SOLO il suo pezzetto
 * (GenLlmCompleteFromPrefix) a partire da li', e si riavvolge la cache
 * (GenLlmRewindToPrefix) prima del prossimo. Solo per il percorso Lua
 * (gen_lua.c): il percorso JSON (grammatica GBNF, un solo prompt a run)
 * resta su GenLlmComplete sopra, invariato. Verificato (non solo assunto)
 * che questo produca la STESSA tokenizzazione e la STESSA sequenza di token
 * generati del prompt combinato di prima: vedi il commento sopra
 * GenLlmPrefixPrime nel .c. */

/* Decodifica 'prefixPrompt' (il prefisso ChatML condiviso: system+cheat-
 * sheet+"<|im_start|>user\n", vedi gen_lua.c BuildLuaPrefix) UNA VOLTA sola
 * nella KV cache della sessione (azzera prima la cache, come faceva
 * GenLlmComplete: e' comunque l'inizio di una conversazione nuova). Scrive
 * in '*nPrefixOut' quanti token occupa (>0) -- il chiamante lo tiene per
 * tutta la fase Lua e lo passa, invariato, a GenLlmCompleteFromPrefix e
 * GenLlmRewindToPrefix per ciascuno dei 20 oggetti. Ritorna 0 su successo,
 * -1 su errore (gia' loggato: file/tokenizzazione, o prefisso che da solo
 * supera n_ctx). */
int GenLlmPrefixPrime(GenLlmSession *sess, const char *prefixPrompt, int *nPrefixOut);

/* Come GenLlmComplete, ma NON azzera la cache KV: assume che
 * GenLlmPrefixPrime l'abbia gia' riempita con 'nPrefix' token di prefisso
 * condiviso (posizioni 0..nPrefix-1) e decodifica SOLO 'suffix' (la scheda
 * per-oggetto + "<|im_end|>\n<|im_start|>assistant\n", vedi gen_lua.c
 * BuildLuaSuffix) a partire dalla posizione nPrefix, poi campiona come
 * GenLlmComplete (stesso sampler chain: penalita'+temp+dist, MAI una
 * grammatica -- il percorso Lua non ne usa, spec sezione 6, quindi qui non
 * c'e' parametro grammarText). 'suffix' e' tokenizzato con add_special=false
 * (e' la continuazione della stessa sequenza del prefisso, non l'inizio: mai
 * un secondo BOS). Il chiamante DEVE richiamare GenLlmRewindToPrefix dopo
 * (successo o fallimento non importa), altrimenti il prossimo oggetto
 * troverebbe la cache allungata di questo tentativo invece del solo
 * prefisso. Stesso significato di ritorno/uscita di GenLlmComplete. */
int GenLlmCompleteFromPrefix(GenLlmSession *sess, int nPrefix, const char *suffix,
                              int nPredict, float temp, unsigned int seed,
                              const char *outDir, const char *progressPhase, int progressBase, int progressSpan,
                              char *out, size_t outCap, int *tokensOut);

/* Rimuove dalla KV cache tutto cio' che segue la posizione 'nPrefix'
 * (llama_memory_seq_rm su tutta la sequenza 0, [nPrefix, inf)): dopo la
 * chiamata la sessione e' di nuovo nello stato "solo prefisso condiviso",
 * pronta per il prossimo GenLlmCompleteFromPrefix. No-op se nPrefix <= 0 o
 * la sessione non e' valida. */
void GenLlmRewindToPrefix(GenLlmSession *sess, int nPrefix);

#endif
