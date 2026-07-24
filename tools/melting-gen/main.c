#include "melting_gen.h"

#include "gen_corpus.h"
#include "gen_lua.h"
#include "gen_novelty.h"

#include "cJSON.h"
#include "llama.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct GenArgs {
    int fallback;
    int emitLlmJson;
    unsigned int seed;
    const char *outDir;
    const char *fromJson;
    const char *model;
    const char *modelFallback;
    /* Esperimento due-modelli (roadmap 17/07/2026): un modello Instruct
     * generalista (qwen2.5-7b-instruct, migliore in prosa/nomi italiani del
     * 7B Coder) usato SOLO per i tentativi JSON, in una sessione separata da
     * quella del Coder -- i due non stanno insieme nei 6 GB di VRAM (vedi
     * OpenModelSession sotto e il blocco di generazione in main()).
     * NULL = comportamento di sempre (una sessione sola, il Coder, per
     * JSON+Lua). Riguarda solo il percorso di generazione normale: --resume
     * usa sempre e solo il Coder. */
    const char *modelText;
    int ngl;
    float temp;
    int nPredict;
    const char *promptsDir;
    /* Step B2 (generazione pigra dei piani): quanti piani generare in Lua a
       partire dal primo (GEN_FLOORS = tutti, il comportamento di sempre; 1 = solo
       il piano che si gioca subito). E 'resume': riprendi una run gia' scritta su
       disco, generando SOLO gli script che mancano e pubblicando il manifest dopo
       ogni piano -- e' il processo che gira in sottofondo mentre si gioca. */
    int luaFirst;
    int resume;
    const char *grammarPath;
    /* Semi d'ispirazione (roadmap 16/07/2026): stampa il prompt JSON completo
     * su stdout e basta, nessun modello caricato -- serve ai test per
     * verificare il prompt (placeholder sostituiti, blocco ispirazioni
     * presente, determinismo sul seed) senza aspettare una generazione vera. */
    int printJsonPrompt;
    /* --bench (piano 16/07/2026, sezione tier): misura il throughput REALE
     * della macchina invece di dedurlo dal nome della GPU. Vedi RunBench. */
    int bench;
    /* M5 (DEC-005, scelta del tema nel Piano 0): 0 = non richiesto (comportamento
     * di sempre). >0 = ramo di uscita anticipata --propose-themes N (vedi
     * RunProposeThemes sotto), N clampato 2..3 li' dentro. */
    int proposeThemes;
    /* --theme-file <path>: il tema scelto dal giocatore (letto una volta sola
     * subito dopo ParseArgs, vedi 'chosenTheme'/'chosenPtr' in main()). NULL =
     * comportamento di sempre, nessuna regressione per i test che non lo
     * passano (requisito 3 della spec M5). */
    const char *themeFile;
    /* M6b-1 (DEC-014, prima fetta): 0 = comportamento di sempre (genera
     * ANCHE il personaggio dentro --propose-themes). 1 = --no-character,
     * retro-compat per i test che vogliono solo i temi (scripts/test-gen.sh)
     * -- nessun effetto fuori dal ramo --propose-themes. */
    int noCharacter;
} GenArgs;

static int ParseArgs(int argc, char **argv, GenArgs *args)
{
    args->fallback = 0;
    args->emitLlmJson = 0;
    args->seed = (unsigned int)time(NULL);
    args->outDir = "generated";
    args->fromJson = NULL;
    /* Modello testuale di riferimento (DEC-140, 23/07/2026): la suite di
     * comparazione su 11 modelli (docs/ai-production/experiments/
     * model-comparison-testo-2026-07-23.md, 3 seed fissi) misura gemma-3-4b-it
     * Q4_K_M sopra il precedente default (7B Coder Q4_K_M, calibrato nel Task
     * 8 su Ryzen 5 3600 + RX 5600 XT 6GB): punteggio 84.9 vs 76.9, Lua valido
     * al primo colpo 93% vs 73% (dopo i ritenti 100% vs 75%), JSON 100% su
     * entrambi, meta' del peso su disco (2.32 vs 4.36 GiB) e +21% tok/s (52.6
     * vs 43.6). Il 7B resta scaricabile e selezionabile con --model (scelta
     * reversibile con un flag, vedi scripts/download-models.sh); il ripiego
     * su errore di caricamento resta il Coder 1.5B Q4 sotto, invariato. */
    args->model = "models/gemma-3-4b-it-q4_k_m.gguf";
    args->modelFallback = "models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf";
    args->modelText = NULL;
    args->ngl = 99;
    args->temp = 0.8f;
    /* 2560 (step C review): il JSON di una run e' cresciuto ~25% con i tipi di
     * colpo (1851 token misurati col 7B), e a 2048 il margine era di ~200 token --
     * troppo poco: un troncamento rende il JSON non parsabile e manda la run sul
     * ripiego procedurale senza che nessuno se ne accorga. Rialzato a 3072 con la
     * fase 3b (due nemici + un boss per piano fanno crescere il JSON di altri ~500
     * token): prompt (~2300) + 3072 = ~5400, comodo dentro GEN_LLM_SESSION_N_CTX=6144. */
    args->nPredict = 4096;
    args->promptsDir = "tools/melting-gen/prompts";
    args->grammarPath = "tools/melting-gen/run.gbnf";
    args->luaFirst = GEN_FLOORS;   /* step B2: di default si generano tutti i piani, come sempre */
    args->resume = 0;
    args->printJsonPrompt = 0;
    args->bench = 0;
    args->proposeThemes = 0;
    args->themeFile = NULL;
    args->noCharacter = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            llama_backend_init();
            printf("melting-gen (llama.cpp b9979)\n%s\n", llama_print_system_info());
            llama_backend_free();
            exit(0);
        }
        else if (strcmp(argv[i], "--fallback") == 0) args->fallback = 1;
        else if (strcmp(argv[i], "--emit-llm-json") == 0) args->emitLlmJson = 1;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) args->seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) args->outDir = argv[++i];
        else if (strcmp(argv[i], "--from-json") == 0 && i + 1 < argc) args->fromJson = argv[++i];
        else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) args->model = argv[++i];
        /* --model-fallback: normalmente il ripiego (1.5B) e' fisso, mai scelto a
         * riga di comando -- questa opzione esiste SOLO per poter forzare "nessun
         * modello disponibile" nei test (scripts/test-gen.sh, --bench senza
         * modello), puntandola a un percorso inesistente ANCHE quando i modelli
         * veri sono scaricati in models/ (come nell'ambiente di sviluppo). Senza,
         * --model su un percorso inesistente da solo non basterebbe a testare
         * quel caso: OpenModelSession ripiegherebbe comunque sul modello piccolo
         * vero. */
        else if (strcmp(argv[i], "--model-fallback") == 0 && i + 1 < argc) args->modelFallback = argv[++i];
        else if (strcmp(argv[i], "--model-text") == 0 && i + 1 < argc) args->modelText = argv[++i];
        else if (strcmp(argv[i], "--ngl") == 0 && i + 1 < argc) args->ngl = atoi(argv[++i]);
        else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) args->temp = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--n-predict") == 0 && i + 1 < argc) args->nPredict = atoi(argv[++i]);
        else if (strcmp(argv[i], "--prompts") == 0 && i + 1 < argc) args->promptsDir = argv[++i];
        else if (strcmp(argv[i], "--grammar") == 0 && i + 1 < argc) args->grammarPath = argv[++i];
        else if (strcmp(argv[i], "--lua-first") == 0 && i + 1 < argc) args->luaFirst = atoi(argv[++i]);
        else if (strcmp(argv[i], "--resume") == 0) args->resume = 1;
        else if (strcmp(argv[i], "--print-json-prompt") == 0) args->printJsonPrompt = 1;
        else if (strcmp(argv[i], "--bench") == 0) args->bench = 1;
        else if (strcmp(argv[i], "--propose-themes") == 0 && i + 1 < argc) args->proposeThemes = atoi(argv[++i]);
        else if (strcmp(argv[i], "--theme-file") == 0 && i + 1 < argc) args->themeFile = argv[++i];
        else if (strcmp(argv[i], "--no-character") == 0) args->noCharacter = 1;
        else if (strcmp(argv[i], "--lua-check") == 0 && i + 1 < argc)
        {
            /* Validazione pura, senza modello: usata dal corpus di test
             * senza LLM (tests/melting-gen/lua/, vedi scripts/test-gen.sh) e
             * comoda a mano per controllare uno script prima di incollarlo
             * in un prompt. Stampa VALID/REJECTED su stdout ed esce con
             * 0/1: non tocca outDir, non scrive progresso, non genera. */
            char *src = GenReadFile(argv[++i]);
            if (!src)
            {
                fprintf(stderr, "melting-gen: impossibile leggere %s\n", argv[i]);
                exit(1);
            }
            char err[192];
            bool anyCallback = false;
            bool ok = GenLuaValidate(src, 12345u, false, &anyCallback, err, sizeof(err));
            free(src);
            if (ok)
            {
                printf("VALID%s\n", anyCallback ? "" : " (nessuna callback definita)");
                exit(0);
            }
            printf("REJECTED: %s\n", err);
            exit(1);
        }
        else if (strcmp(argv[i], "--prompt-budget-check") == 0)
        {
            /* Guardia byte-budget del prompt Lua (fase 3b review), senza
             * alcun modello: vedi gen_lua.h (GenLuaPromptBudgetCheck,
             * GEN_LUA_PROMPT_BYTE_CEILING) per il perche' e la derivazione
             * del ceiling. Usata da scripts/test-gen.sh, mai da
             * scripts/test-llm.sh (non serve un modello caricato). Legge i
             * template da args->promptsDir (--prompts per cambiarlo, PRIMA
             * di questo flag sulla riga di comando: stesso ordine di
             * dipendenza di --lua-check sopra), stampa OK/FALLITO su
             * stdout ed esce con 0/1. */
            char err[192];
            bool ok = GenLuaPromptBudgetCheck(args->promptsDir, err, sizeof(err));
            if (ok)
            {
                printf("OK\n");
                exit(0);
            }
            printf("FALLITO: %s\n", err);
            exit(1);
        }
        else if (strcmp(argv[i], "--character-clamp-check") == 0)
        {
            /* M6b-3 (DEC-068): guardia del budget cauto/del clamp del colpo
             * firmato, senza alcun modello -- CharacterGenDefClamp (src/
             * core/character_type.c) e' una funzione PURA, quindi due
             * chiamate con lo stesso input DEVONO produrre lo stesso output
             * byte-per-byte: scripts/test-gen.sh lancia questo ramo due
             * volte e confronta lo stdout con cmp (determinismo), poi legge
             * i numeri e verifica che la sotto-banda cauta
             * (CHARACTER_SHOT_CAUTION_FRACTION) si applichi SOLO quando
             * hasShot e' vero, mai quando e' falso -- lo stesso contratto
             * che main.c/character_proposal.c chiedono a questa funzione,
             * qui esercitato senza dover passare da un json/un modello.
             * Input deliberatamente FUORI banda su ogni manopola (99, 0.01,
             * 999, 1, 999, 99 e, per il colpo, 5/99 su ogni manopola, forma
             * invalida): un clamp che funzionasse solo su input gia' in
             * banda non proverebbe nulla. Stampa "campo=valore", un valore
             * per riga, ordine fisso, ed esce con 0 -- stesso spirito di
             * --prompt-budget-check sopra, nessun outDir/modello toccato. */
            CharacterGenDef noShot;
            memset(&noShot, 0, sizeof(noShot));
            noShot.damage = 99.0f; noShot.fireDelay = 0.01f; noShot.shotSpeed = 999.0f;
            noShot.speed = 1.0f; noShot.maxHp = 999; noShot.luck = 99.0f;
            noShot.hasShot = false;
            CharacterGenDefClamp(&noShot);

            CharacterGenDef withShot;
            memset(&withShot, 0, sizeof(withShot));
            withShot.damage = 99.0f; withShot.fireDelay = 0.01f; withShot.shotSpeed = 999.0f;
            withShot.speed = 1.0f; withShot.maxHp = 999; withShot.luck = 99.0f;
            withShot.hasShot = true;
            snprintf(withShot.signatureShot.name, sizeof(withShot.signatureShot.name), "Selftest Shot");
            withShot.signatureShot.form = (ShotForm)999;   /* fuori enum: deve ricadere su ORB */
            withShot.signatureShot.speedMul = 5.0f;
            withShot.signatureShot.damageMul = 5.0f;
            withShot.signatureShot.radiusMul = 5.0f;
            withShot.signatureShot.lifeMul = 5.0f;
            withShot.signatureShot.pierceBonus = 99;
            withShot.signatureShot.chain = 99;
            withShot.signatureShot.pellets = 99;
            CharacterGenDefClamp(&withShot);

            printf("noshot.damage=%.6f\n", (double)noShot.damage);
            printf("noshot.fireDelay=%.6f\n", (double)noShot.fireDelay);
            printf("noshot.shotSpeed=%.6f\n", (double)noShot.shotSpeed);
            printf("noshot.speed=%.6f\n", (double)noShot.speed);
            printf("noshot.maxHp=%d\n", noShot.maxHp);
            printf("noshot.luck=%.6f\n", (double)noShot.luck);
            printf("noshot.hpCap=%d\n", noShot.hpCap);
            printf("noshot.signature.active=%d\n", noShot.signatureShot.active ? 1 : 0);
            printf("shot.damage=%.6f\n", (double)withShot.damage);
            printf("shot.fireDelay=%.6f\n", (double)withShot.fireDelay);
            printf("shot.shotSpeed=%.6f\n", (double)withShot.shotSpeed);
            printf("shot.speed=%.6f\n", (double)withShot.speed);
            printf("shot.maxHp=%d\n", withShot.maxHp);
            printf("shot.luck=%.6f\n", (double)withShot.luck);
            printf("shot.hpCap=%d\n", withShot.hpCap);
            printf("shot.signature.active=%d\n", withShot.signatureShot.active ? 1 : 0);
            printf("shot.signature.name=%s\n", withShot.signatureShot.name);
            printf("shot.signature.form=%s\n", ShotFormName(withShot.signatureShot.form));
            printf("shot.signature.damageMul=%.6f\n", (double)withShot.signatureShot.damageMul);
            printf("shot.signature.speedMul=%.6f\n", (double)withShot.signatureShot.speedMul);
            printf("shot.signature.radiusMul=%.6f\n", (double)withShot.signatureShot.radiusMul);
            printf("shot.signature.lifeMul=%.6f\n", (double)withShot.signatureShot.lifeMul);
            printf("shot.signature.pierce=%d\n", withShot.signatureShot.pierceBonus);
            printf("shot.signature.chain=%d\n", withShot.signatureShot.chain);
            printf("shot.signature.pellets=%d\n", withShot.signatureShot.pellets);
            exit(0);
        }
        else
        {
            fprintf(stderr, "melting-gen: opzione sconosciuta: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

/* 'modelJsonField'/'modelLuaField': vedi il commento su GenWriteProvenance in
 * melting_gen.h -- gia' risolti dal chiamante (main.c sa quale ramo ha
 * prodotto la run), ignorati del tutto quando args->resume (la provenienza
 * non si scrive in ripresa, vedi sotto). */
static int WriteOutputs(const GenRun *run, const GenArgs *args,
                         const char *modelJsonField, const char *modelLuaField,
                         const GenChosenTheme *chosen)
{
    /* 99, non 85: la fase Lua (fase 3a-L3, quando c'e' un modello) scrive
     * progresso fino al 98% (vedi GenLuaGenerateForRun in gen_lua.c), la
     * barra deve restare monotona crescente fino a "fine" a 100. */
    GenProgressWrite(args->outDir, "scrivo", 99, "scrivo manifest e atlas");
    /* Step B2: una RIPRESA non tocca l'atlas (esiste gia', ed e' magari il PNG
     * degli sprite) e preserva la riga atlas.path del manifest. Vedi
     * GenWriteRunFilesResume in gen_manifest.c per il perche' -- e' il bug che
     * riporterebbe la run all'atlas di riserva pur avendo gli sprite sul disco. */
    int rc = args->resume ? GenWriteRunFilesResume(run, args->outDir)
                          : ((GenWriteAtlasBmp(run, args->outDir) != 0) ? -1 : GenWriteRunFiles(run, args->outDir));
    if (rc != 0)
    {
        GenProgressWrite(args->outDir, "errore", 100, "scrittura file fallita");
        return 3;
    }
    if (!args->resume)
    {
        /* RunBundle v1 (roadmap 16/07/2026 settimana 4): la PROVENIENZA si
         * scrive SOLO a fine di una generazione normale o fallback -- MAI in
         * --resume, che appartiene alla STESSA run del processo che l'ha
         * aperta: provenance.txt esiste gia' e non va toccato. */
        if (GenWriteProvenance(run, args->outDir, args->promptsDir, modelJsonField, modelLuaField,
                                chosen ? chosen->raw : NULL) != 0)
        {
            GenProgressWrite(args->outDir, "errore", 100, "scrittura provenienza fallita");
            return 3;
        }
    }
    if (args->emitLlmJson)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/llm_sample.json", args->outDir);
        if (GenWriteLlmJson(run, path) != 0) return 3;
    }
    GenProgressWrite(args->outDir, "fine", 100, "manifest pronto");
    GenLogLine("source=%s seed=%u out=%s", run->source, run->seed, args->outDir);
    return 0;
}

/* Apertura del modello (principale, poi il ripiego piu' piccolo, poi niente):
 * estratta da main perche' ora la usano DUE percorsi -- la generazione completa e
 * la ripresa in sottofondo dello step B2. Ritorna NULL se non c'e' alcun modello
 * (caso legittimo: si usa il contenuto procedurale). */
static GenLlmSession *OpenModelSession(const GenArgs *args, const char **modelPathOut)
{
    const char *modelPath = NULL;
    if (GenFileExists(args->model)) modelPath = args->model;
    else if (GenFileExists(args->modelFallback))
    {
        modelPath = args->modelFallback;
        GenLogLine("modello principale assente, ripiego su %s", modelPath);
    }
    else
    {
        GenLogLine("nessun modello in models/: uso il fallback deterministico");
        return NULL;
    }
    if (modelPathOut) *modelPathOut = modelPath;
    return GenLlmSessionOpen(modelPath, args->ngl, args->outDir);
}

/* Prompt fisso del bench (piano 16/07/2026, sezione tier): scritto qui, non in
 * un file di prompts/, apposta -- il bench deve misurare la VELOCITA' della
 * macchina, non dipendere dal contenuto o dalla cartella prompts/ (che un
 * utente potrebbe aver modificato, spostato, o non avere affatto se sta solo
 * misurando l'hardware prima ancora di scaricare i prompt di produzione).
 * Stesso schema ChatML di ogni altro prompt di questo tool (GenChatMlWrap). */
static const char *GEN_BENCH_SYSTEM = "Sei un assistente che scrive brevi descrizioni di fantasia in italiano.";
static const char *GEN_BENCH_USER = "Descrivi in due o tre frasi una torre magica misteriosa, come l'inizio di una fiaba.";
#define GEN_BENCH_TOKENS 128
#define GEN_BENCH_SEED 20260717u

/* --bench (piano 16/07/2026, sezione tier: il tier va per THROUGHPUT MISURATO,
 * mai per nome GPU -- la n.1 su Steam e' una 4060 Laptop, ~2x piu' lenta della
 * desktop omonima). Carica lo STESSO modello di una generazione vera
 * (OpenModelSession sopra, rispetta --model/--model-fallback/--ngl), genera
 * GEN_BENCH_TOKENS token da un prompt FISSO hardcoded (vedi sopra: nessuna
 * dipendenza da prompts/) e stampa una riga machine-readable su stdout.
 *
 * NON tocca generated/ (ne' manifest, ne' progresso, ne' corpus): 'benchArgs'
 * e' una copia di 'args' con outDir=NULL, che rende innocuo il progress
 * callback di caricamento del modello (GenProgressWrite/LoadProgressCb, vedi
 * la guardia su outDir NULL in gen_util.c) e ogni chiamata di GenProgressWrite
 * dentro GenLlmComplete (gia' guardata su outDir/progressPhase non-NULL,
 * invariata). Nessuna GenCorpusRecord* viene mai chiamata da questa funzione:
 * il file di corpus si apre pigramente alla prima scrittura (gen_corpus.c),
 * quindi se non scriviamo non si crea nulla.
 *
 * Ritorna 0 e stampa "bench: tok_s=... load_s=...\n" su successo, 1 su
 * qualunque fallimento (nessun modello disponibile, o la generazione stessa
 * fallisce), con un messaggio su stderr in entrambi i casi. */
static int RunBench(const GenArgs *args)
{
    GenArgs benchArgs = *args;
    benchArgs.outDir = NULL;

    double tLoad0 = GenNowSeconds();
    const char *modelPath = NULL;
    GenLlmSession *sess = OpenModelSession(&benchArgs, &modelPath);
    double loadSecs = GenNowSeconds() - tLoad0;
    if (!sess)
    {
        fprintf(stderr, "melting-gen: --bench, nessun modello disponibile (%s)\n", args->model);
        return 1;
    }

    char *prompt = GenChatMlWrap(GEN_BENCH_SYSTEM, GEN_BENCH_USER);
    if (!prompt)
    {
        fprintf(stderr, "melting-gen: --bench, prompt fisso non costruibile\n");
        GenLlmSessionClose(sess);
        return 1;
    }

    static char out[8192];
    int tokens = 0;
    double tGen0 = GenNowSeconds();
    /* Nessuna grammatica (grammarText=NULL, lo stesso percorso a campionamento
     * libero del ramo Lua): il bench misura la velocita' pura del modello, non
     * anche quella del parser della grammatica GBNF del JSON. */
    int rc = GenLlmComplete(sess, prompt, NULL, GEN_BENCH_TOKENS, args->temp, GEN_BENCH_SEED,
                             NULL, NULL, 0, 0, out, sizeof(out), &tokens);
    double genSecs = GenNowSeconds() - tGen0;
    free(prompt);
    GenLlmSessionClose(sess);

    if (rc != 0 || tokens <= 0)
    {
        fprintf(stderr, "melting-gen: --bench, generazione fallita\n");
        return 1;
    }

    /* tok/s misurato sull'INTERA chiamata (decodifica del prompt fisso, breve,
     * piu' i GEN_BENCH_TOKENS generati uno alla volta): il prompt fisso e' cosi'
     * corto (poche decine di token, decodificati in un solo batch parallelo,
     * non uno alla volta come la generazione) che il suo costo e' una frazione
     * trascurabile del tempo totale. Non vale la pena separare le due fasi per
     * soglie grossolane come quelle del tier (12/6 tok/s). */
    double tokS = genSecs > 0.0 ? (double)tokens/genSecs : 0.0;
    printf("bench: tok_s=%.2f load_s=%.2f\n", tokS, loadSecs);
    return 0;
}

/* Tentativi di generazione JSON (fino a 2, grammatica GBNF): estratta da
 * main perche' ora la chiamano DUE percorsi -- la sessione condivisa di
 * sempre e, nell'esperimento due-modelli (--model-text, roadmap 17/07/2026),
 * la sessione SEPARATA aperta solo per il JSON. 'sess' e' gia' aperta,
 * 'modelPath' serve solo per il log e per run->source. Non chiude 'sess':
 * quello resta compito del chiamante (le due sessioni hanno vite diverse a
 * seconda del percorso). Ritorna 1 se un tentativo e' andato a buon fine
 * (run popolato, run->source impostato), 0 altrimenti (*run non toccato). */
static int RunJsonAttempts(GenLlmSession *sess, const char *modelPath, const GenArgs *args,
                            const GenChosenTheme *chosen, GenRun *run, char *json, size_t jsonCap)
{
    int haveRun = 0;
    char *grammar = GenReadFile(args->grammarPath);
    for (int attempt = 0; attempt < 2 && !haveRun && grammar; attempt++)
    {
        unsigned int attemptSeed = args->seed + (unsigned int)attempt*7919u;
        char *prompt = GenLlmBuildJsonPrompt(args->promptsDir, attemptSeed, chosen);
        if (!prompt)
        {
            GenLogLine("tentativo %d: prompt JSON non costruibile (file mancanti in %s?)", attempt + 1, args->promptsDir);
            GenCorpusRecordJson(attempt + 1, false, "prompt non costruibile", 0.0, 0, NULL);
            continue;
        }
        double t0 = GenNowSeconds();
        int tokens = 0;
        int rc = GenLlmComplete(sess, prompt, grammar, args->nPredict, args->temp, attemptSeed,
                                 args->outDir, "genero", 62, 30, json, jsonCap, &tokens);
        double genSecs = GenNowSeconds() - t0;
        free(prompt);
        if (rc != 0)
        {
            GenLogLine("tentativo %d: generazione fallita", attempt + 1);
            GenCorpusRecordJson(attempt + 1, false, "generazione fallita", genSecs, tokens, NULL);
            continue;
        }
        cJSON *root = cJSON_Parse(json);
        if (!root)
        {
            GenLogLine("tentativo %d: JSON troncato o non parsabile (%d token)", attempt + 1, tokens);
            GenCorpusRecordJson(attempt + 1, false, "json non parsabile", genSecs, tokens, json);
            continue;
        }
        GenCorpusRecordJson(attempt + 1, true, NULL, genSecs, tokens, json);
        GenProgressWrite(args->outDir, "valido", 92, "valido e normalizzo");
        GenNormalizeRun(root, args->seed, chosen, run);
        cJSON_Delete(root);
        const char *base = strrchr(modelPath, '/');
        snprintf(run->source, sizeof(run->source), "local:%s", base ? base + 1 : modelPath);
        GenLogLine("ok: model=%s ngl=%d gen=%.1fs token=%d (%.1f tok/s)",
                   modelPath, args->ngl, genSecs, tokens, genSecs > 0 ? tokens/genSecs : 0.0);
        haveRun = 1;
    }
    free(grammar);
    return haveRun;
}

/* Fase Lua (fase 3a-L3): estratta per lo stesso motivo di RunJsonAttempts qui
 * sopra -- nell'esperimento due-modelli gira SEMPRE sulla sessione del Coder
 * (mai sul modello testo, che a quel punto e' gia' chiuso), su una sessione
 * diversa da quella usata per il JSON. */
static void RunLuaPhase(GenLlmSession *sess, GenRun *run, const GenArgs *args, double processStart)
{
    double luaDeadline = processStart + GEN_LUA_PHASE_BUDGET_SEC;
    GenLuaStats luaStats;
    /* Step B2: 'args->luaFirst' piani soltanto (di default tutti e cinque; il
     * gioco passa 1 per far partire la run prima e lasciare i piani 2-5 al
     * processo di ripresa in sottofondo). Nessuna pubblicazione per-piano
     * qui: questo processo blocca ancora la partenza della run, il manifest
     * lo scrive una volta sola alla fine (WriteOutputs). */
    GenLuaGenerateForRun(sess, run, args->promptsDir, args->outDir, luaDeadline,
                          args->luaFirst, false, &luaStats);
    GenLogLine("lua: %d primo tentativo, %d dopo retry, %d senza comportamento, %d ripiegati su mini-VM, %d saltati per budget (piani generati: %d/%d)",
               luaStats.firstTry, luaStats.afterRetry, luaStats.optedOut, luaStats.fellBack, luaStats.skippedBudget,
               args->luaFirst, GEN_FLOORS);
}

/* M5 (DEC-005), requisito 1: percorso della grammatica di --propose-themes.
 * Letterale fisso (come GEN_BENCH_SYSTEM/USER sopra), non configurabile da
 * riga di comando: a differenza di run.gbnf (--grammar) nessun test ha
 * bisogno di puntarla altrove, e propose.gbnf vive accanto a run.gbnf per
 * costruzione (stessa cartella tools/melting-gen). */
#define GEN_PROPOSE_GRAMMAR_PATH "tools/melting-gen/propose.gbnf"
/* nPredict richiesto dalla spec: 3 coppie nome+blurb, output minuscolo. */
#define GEN_PROPOSE_N_PREDICT 320

/* M6b-1 (DEC-014, prima fetta): grammatica/nPredict del personaggio
 * alternativo per-run -- output piccolo (nome+blurb+sei numeri+palette),
 * nPredict piu' generoso di GEN_PROPOSE_N_PREDICT perche' lo "stats" object
 * aggiunge struttura JSON e sei numeri, ma resta comunque un ordine di
 * grandezza sotto il JSON di una run intera.
 * M6b-3 (DEC-068): rialzato da 220 a 320 -- il blocco "shot" opzionale (nome
 * fino a 40 caratteri + forma + sette manopole, come il tipo di colpo di un
 * piano intero) puo' aggiungere ~100 token quando il modello lo include; un
 * troncamento a meta' del blocco "shot" renderebbe il JSON non parsabile e
 * farebbe perdere l'INTERA proposta (character.gbnf non ha un ripiego
 * procedurale, characters.md "Fallback": carta assente), non solo il colpo
 * firmato -- stesso ragionamento del rialzo del nPredict della run intera
 * (args->nPredict qui sopra in ParseArgs, commento sul campo). */
#define GEN_CHARACTER_GRAMMAR_PATH "tools/melting-gen/character.gbnf"
#define GEN_CHARACTER_N_PREDICT 320

/* M6b-2 (DEC-037): budget di TEMPO dedicato al passo trait, contato
 * dall'INIZIO di quel passo (non dall'inizio del processo, a differenza di
 * GEN_LUA_PHASE_BUDGET_SEC che governa i 20 script Lua di una run intera) --
 * la fase propose resta "mentre giri nell'hub", mai bloccante: 60s bastano
 * per GEN_LUA_MAX_ATTEMPTS tentativi di un singolo script piccolo (vedi
 * gen_lua.h) senza rischiare di far percepire il Piano 0 come appeso. */
#define GEN_CHARACTER_TRAIT_BUDGET_SEC 60.0

/* M6b-1 (DEC-014, prima fetta): genera la proposta di personaggio
 * alternativo per questa run, dentro la STESSA sessione/lo stesso processo
 * gia' aperto da RunProposeThemes per i temi -- mai un secondo modello
 * caricato (vedi il commento su AppStopLazyGeneration in src/app/app.c,
 * "mai due melting-gen insieme": qui e' la stessa garanzia, un livello piu'
 * in basso). Il fallback canonico e' l'ASSENZA della carta (characters.md,
 * "Fallback"): questa funzione non scrive MAI un personaggio-curato-di-
 * riserva, solo logga e torna su qualunque fallimento --
 * character_proposal.json resta assente (gia' rimosso in testa a
 * RunProposeThemes, prima di questo tentativo) e il gioco mostrera' solo la
 * rosa base, silenziosamente (DEC-002/DEC-020). */
static void RunProposeCharacter(GenLlmSession *sess, const GenArgs *args, const char *modelPath)
{
    char *prompt = GenLlmBuildCharacterPrompt(args->promptsDir, args->seed);
    char *grammar = GenReadFile(GEN_CHARACTER_GRAMMAR_PATH);
    if (!prompt || !grammar)
    {
        GenLogLine("propose-character: prompt o grammatica non costruibili, nessuna carta");
        free(prompt);
        free(grammar);
        return;
    }

    static char json[2048];
    int tokens = 0;
    /* Seed diverso da quello dei temi (XOR con un letterale fisso): due
     * chiamate indipendenti nella stessa sessione non devono campionare lo
     * stesso identico stream, altrimenti nome/blurb del personaggio
     * rischierebbero di rispecchiare le proposte di tema appena generate
     * (stesso principio del salt fra i due passi in NextGenSeed, src/app/
     * app.c) -- resta comunque deterministico a parita' di args->seed. */
    unsigned int characterSeed = args->seed ^ 0x63484152u;
    int rc = GenLlmComplete(sess, prompt, grammar, GEN_CHARACTER_N_PREDICT, args->temp, characterSeed,
                             NULL, NULL, 0, 0, json, sizeof(json), &tokens);
    free(prompt);
    free(grammar);
    if (rc != 0)
    {
        GenLogLine("propose-character: generazione fallita, nessuna carta");
        return;
    }

    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        GenLogLine("propose-character: JSON non valido, nessuna carta");
        return;
    }

    cJSON *jn = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *jb = cJSON_GetObjectItemCaseSensitive(root, "blurb");
    cJSON *jstats = cJSON_GetObjectItemCaseSensitive(root, "stats");
    cJSON *jpal = cJSON_GetObjectItemCaseSensitive(root, "palette");
    int ok = cJSON_IsString(jn) && jn->valuestring && jn->valuestring[0] &&
             cJSON_IsString(jb) && jb->valuestring && jb->valuestring[0] &&
             cJSON_IsObject(jstats) &&
             cJSON_IsString(jpal) && jpal->valuestring && jpal->valuestring[0] == '#' &&
             strlen(jpal->valuestring) == 7;

    CharacterGenDef def;
    memset(&def, 0, sizeof(def));
    if (ok)
    {
        cJSON *jd = cJSON_GetObjectItemCaseSensitive(jstats, "damage");
        cJSON *jf = cJSON_GetObjectItemCaseSensitive(jstats, "fireDelay");
        cJSON *js = cJSON_GetObjectItemCaseSensitive(jstats, "shotSpeed");
        cJSON *jv = cJSON_GetObjectItemCaseSensitive(jstats, "speed");
        cJSON *jh = cJSON_GetObjectItemCaseSensitive(jstats, "maxHp");
        cJSON *jl = cJSON_GetObjectItemCaseSensitive(jstats, "luck");
        ok = cJSON_IsNumber(jd) && cJSON_IsNumber(jf) && cJSON_IsNumber(js) &&
             cJSON_IsNumber(jv) && cJSON_IsNumber(jh) && cJSON_IsNumber(jl);
        if (ok)
        {
            snprintf(def.name, sizeof(def.name), "%s", jn->valuestring);
            snprintf(def.blurb, sizeof(def.blurb), "%s", jb->valuestring);
            def.damage = (float)jd->valuedouble;
            def.fireDelay = (float)jf->valuedouble;
            def.shotSpeed = (float)js->valuedouble;
            def.speed = (float)jv->valuedouble;
            def.maxHp = (int)jh->valuedouble;
            def.luck = (float)jl->valuedouble;
            snprintf(def.palette, sizeof(def.palette), "%s", jpal->valuestring);

            /* M6b-3 (DEC-068): il colpo firmato OPZIONALE -- character.gbnf
             * lo rende parte della grammatica solo "a volte" ('shotopt'),
             * quindi la sua ASSENZA qui non e' un errore, e' lo stato PIU'
             * COMUNE del generatore (KB, caso limite "il generatore non
             * produce un colpo firmato... non e' un errore"). Se il
             * modello lo scrive, la grammatica ne garantisce la FORMA
             * (tutti i campi presenti coi tipi giusti): il controllo
             * difensivo sotto e' "fidati ma verifica" come il resto di
             * questo file, non sfiducia nella grammatica -- se un campo
             * mancasse comunque (bug futuro nella grammatica), il colpo
             * firmato viene semplicemente IGNORATO (fallback locale, mai
             * la carta intera: spec M6b-3, punto (c)/requisito 3). */
            cJSON *jshot = cJSON_GetObjectItemCaseSensitive(root, "shot");
            if (cJSON_IsObject(jshot))
            {
                cJSON *sn = cJSON_GetObjectItemCaseSensitive(jshot, "name");
                cJSON *sform = cJSON_GetObjectItemCaseSensitive(jshot, "form");
                cJSON *sspeed = cJSON_GetObjectItemCaseSensitive(jshot, "speed");
                cJSON *sdamage = cJSON_GetObjectItemCaseSensitive(jshot, "damage");
                cJSON *ssize = cJSON_GetObjectItemCaseSensitive(jshot, "size");
                cJSON *slife = cJSON_GetObjectItemCaseSensitive(jshot, "life");
                cJSON *spierce = cJSON_GetObjectItemCaseSensitive(jshot, "pierce");
                cJSON *schain = cJSON_GetObjectItemCaseSensitive(jshot, "chain");
                cJSON *spellets = cJSON_GetObjectItemCaseSensitive(jshot, "pellets");
                bool shotOk = cJSON_IsString(sn) && sn->valuestring && sn->valuestring[0] &&
                              cJSON_IsString(sform) && sform->valuestring &&
                              cJSON_IsNumber(sspeed) && cJSON_IsNumber(sdamage) &&
                              cJSON_IsNumber(ssize) && cJSON_IsNumber(slife) &&
                              cJSON_IsNumber(spierce) && cJSON_IsNumber(schain) && cJSON_IsNumber(spellets);
                if (shotOk)
                {
                    memset(&def.signatureShot, 0, sizeof(def.signatureShot));
                    snprintf(def.signatureShot.name, sizeof(def.signatureShot.name), "%s", sn->valuestring);
                    def.signatureShot.form = ShotFormFromText(sform->valuestring);
                    def.signatureShot.speedMul = (float)sspeed->valuedouble;
                    def.signatureShot.damageMul = (float)sdamage->valuedouble;
                    def.signatureShot.radiusMul = (float)ssize->valuedouble;
                    def.signatureShot.lifeMul = (float)slife->valuedouble;
                    def.signatureShot.pierceBonus = (int)spierce->valuedouble;
                    def.signatureShot.chain = (int)schain->valuedouble;
                    def.signatureShot.pellets = (int)spellets->valuedouble;
                    def.hasShot = true;
                }
            }
        }
    }
    cJSON_Delete(root);
    if (!ok)
    {
        GenLogLine("propose-character: schema non valido, nessuna carta");
        return;
    }

    CharacterGenDefClamp(&def);   /* prima rete di clamp (stats caute + colpo firmato): qui, prima di scrivere il json */

    /* M6b-2 (DEC-037), seconda fetta: il trait Lua UNICO del personaggio,
     * STESSA sessione modello gia' aperta (mai un secondo caricamento, vedi
     * il commento in cima a questa funzione), STESSA pipeline degli oggetti
     * (gen_lua.c: sandbox vera, dry-run, ritenti con l'errore rimandato).
     * Budget di fase dedicato, contato da QUI (non dall'inizio del
     * processo): la generazione delle stats sopra e' gia' rapida (un
     * singolo JSON piccolo a grammatica), il tempo che conta per "non
     * bloccare l'hub" e' quello del passo trait.
     * KB (characters.md, DEC-037): trait non valido dopo i ritenti =
     * personaggio INTERO non valido = carta assente, NESSUN
     * character_proposal.json -- si esce qui SENZA scrivere nulla (il file
     * e' gia' stato rimosso in testa a RunProposeThemes, prima di questo
     * tentativo), esattamente come ogni altro fallimento di questa funzione
     * sopra. */
    double traitDeadline = GenNowSeconds() + GEN_CHARACTER_TRAIT_BUDGET_SEC;
    char traitLua[GEN_LUA_LEN];
    GenLuaStats traitStats;
    bool haveTrait = GenLuaGenerateCharacterTrait(sess, args->promptsDir, args->seed, def.name, def.blurb,
                                                   traitDeadline, traitLua, sizeof(traitLua), &traitStats);
    if (!haveTrait)
    {
        GenLogLine("propose-character: trait non validato dopo i tentativi, nessuna carta (DEC-037)");
        return;
    }

    char srcBuf[128];
    const char *base = strrchr(modelPath, '/');
    snprintf(srcBuf, sizeof(srcBuf), "local:%s", base ? base + 1 : modelPath);

    /* Ordine non negoziabile (spec): il .lua PRIMA del .json che lo
     * referenzia, stessa garanzia d'ordine del manifest degli oggetti (vedi
     * il commento su GenWriteCharacterTraitLua in melting_gen.h). Se la
     * scrittura del .lua fallisce (disco pieno, rename impossibile...), il
     * json NON si scrive affatto -- mai un "lua":true che punta a un file
     * che non esiste per davvero (a differenza dell'anomalia di lettura che
     * la spec chiede di tollerare a RUN GIA' INIZIATA, qui e' ancora sotto
     * il nostro controllo, quindi si evita di scriverla). */
    if (GenWriteCharacterTraitLua(traitLua, args->outDir) != 0)
    {
        GenLogLine("propose-character: scrittura del trait fallita, nessuna carta");
        return;
    }

    if (GenWriteCharacterProposal(&def, srcBuf, args->outDir, true) != 0)
        GenLogLine("propose-character: scrittura fallita");
    else
        GenLogLine("propose-character: source=%s name=%s trait=ok shot=%s", srcBuf, def.name,
                    def.hasShot ? def.signatureShot.name : "none");
}

/* --propose-themes N (M5, requisito 1): ramo di uscita anticipata come
 * --print-json-prompt/--bench -- scrive SOLO generated/theme_proposals.json
 * (piu', da M6b-1, generated/character_proposal.json quando il personaggio
 * generato valida: vedi RunProposeCharacter sopra), MAI corpus/manifest/
 * atlas/provenance/ledger di novita' della run (per questo non chiama ne'
 * GenCorpusRecord* ne' GenNoveltyAppend, a differenza del resto di questo
 * file). La grammatica propose.gbnf genera SEMPRE 3 proposte di tema (root
 * fissa, coerente col prompt "Propose 3 possible worlds");
 * 'requestedCount' (clampato 2..3) sceglie solo quante di quelle 3 il
 * chiamante vuole vedere scritte su disco -- mai meno di 2 (requisito 11:
 * mai una sola carta selezionabile). Un solo tentativo per i temi (a
 * differenza dei 2 di RunJsonAttempts): e' un output piccolo e veloce, e il
 * ripiego deterministico copre comunque ogni fallimento (DEC-002) -- il
 * personaggio generato NON ha un ripiego deterministico equivalente
 * (fallback canonico = carta assente, characters.md), quindi
 * 'includeCharacter' governa SOLO se vale la pena tentarlo, mai un secondo
 * tentativo. */
static int RunProposeThemes(const GenArgs *args, int requestedCount, int includeCharacter)
{
    int count = requestedCount;
    if (count < 2) count = 2;
    if (count > GEN_THEME_PROPOSALS) count = GEN_THEME_PROPOSALS;

    /* M6b-1: rimuove SEMPRE un character_proposal.json residuo di una
     * generazione precedente PRIMA di tentare (anche con --no-character,
     * usato dai test che vogliono solo i temi): il fallback canonico del
     * personaggio generato e' l'ASSENZA della carta, mai un personaggio
     * curato di riserva -- un file di ieri lasciato sul disco produrrebbe
     * una carta vecchia travestita da carta di questa run. */
    char characterPath[512];
    snprintf(characterPath, sizeof(characterPath), "%s/character_proposal.json", args->outDir);
    remove(characterPath);
    /* M6b-2 (DEC-037): stesso ragionamento per il trait -- un
     * character_trait.lua di ieri, lasciato sul disco, non deve MAI
     * sopravvivere a una generazione (con o senza modello) che non lo
     * riscrive: senza questa rimozione un fallimento del passo trait di
     * OGGI lascerebbe comunque un file valido di IERI sul disco, e un
     * eventuale "lua":true scritto da un json vecchio residuo (che qui
     * sopra viene gia' rimosso) punterebbe a un trait che non e' di questa
     * run. test-gen (senza modello) verifica proprio l'assenza di residui. */
    char traitPath[512];
    snprintf(traitPath, sizeof(traitPath), "%s/scripts/character_trait.lua", args->outDir);
    remove(traitPath);

    GenThemeProposal proposals[GEN_THEME_PROPOSALS];
    memset(proposals, 0, sizeof(proposals));
    const char *source = "fallback";
    int haveLlm = 0;

    const char *modelPath = NULL;
    GenLlmSession *sess = OpenModelSession(args, &modelPath);
    if (sess)
    {
        char *prompt = GenLlmBuildProposePrompt(args->promptsDir, args->seed);
        char *grammar = GenReadFile(GEN_PROPOSE_GRAMMAR_PATH);
        if (prompt && grammar)
        {
            static char json[8192];
            int tokens = 0;
            /* outDir/progressPhase NULL: --propose-themes non deve toccare
             * generated/gen_progress.txt (che appartiene alla pipeline
             * principale, sequenziale ma distinta -- vedi il commento sul
             * requisito 1 in melting_gen.h). */
            int rc = GenLlmComplete(sess, prompt, grammar, GEN_PROPOSE_N_PREDICT, args->temp, args->seed,
                                     NULL, NULL, 0, 0, json, sizeof(json), &tokens);
            if (rc == 0)
            {
                cJSON *root = cJSON_Parse(json);
                if (root)
                {
                    cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "proposals");
                    if (cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= GEN_THEME_PROPOSALS)
                    {
                        int ok = 1;
                        for (int i = 0; i < GEN_THEME_PROPOSALS && ok; i++)
                        {
                            cJSON *p = cJSON_GetArrayItem(arr, i);
                            cJSON *jn = cJSON_GetObjectItemCaseSensitive(p, "name");
                            cJSON *jb = cJSON_GetObjectItemCaseSensitive(p, "blurb");
                            if (!cJSON_IsString(jn) || !jn->valuestring || !jn->valuestring[0] ||
                                !cJSON_IsString(jb) || !jb->valuestring || !jb->valuestring[0])
                            {
                                ok = 0;
                                break;
                            }
                            snprintf(proposals[i].name, sizeof(proposals[i].name), "%s", jn->valuestring);
                            snprintf(proposals[i].blurb, sizeof(proposals[i].blurb), "%s", jb->valuestring);
                        }
                        if (ok)
                        {
                            haveLlm = 1;
                            static char srcBuf[128];
                            const char *base = strrchr(modelPath, '/');
                            snprintf(srcBuf, sizeof(srcBuf), "local:%s", base ? base + 1 : modelPath);
                            source = srcBuf;
                        }
                    }
                    cJSON_Delete(root);
                }
            }
            if (!haveLlm) GenLogLine("propose-themes: generazione/validazione fallita, ripiego procedurale");
        }
        else GenLogLine("propose-themes: prompt o grammatica non costruibili, ripiego procedurale");
        free(prompt);
        free(grammar);

        /* M6b-1 (DEC-014): il personaggio si genera QUI, nella STESSA
         * sessione gia' aperta per i temi -- indipendente dall'esito dei
         * temi sopra (anche se il JSON dei temi fallisce e si ricade sul
         * ripiego procedurale, la sessione resta valida e vale la pena
         * tentare comunque il personaggio, che ha un prompt/una grammatica
         * completamente separati). */
        if (includeCharacter) RunProposeCharacter(sess, args, modelPath);

        GenLlmSessionClose(sess);
    }

    if (!haveLlm)
    {
        GenFallbackThemeProposals(args->seed, GEN_THEME_PROPOSALS, proposals);
        source = "fallback";
    }

    GenLogLine("propose-themes: source=%s count=%d seed=%u", source, count, args->seed);
    return GenWriteThemeProposals(proposals, count, source, args->outDir) == 0 ? 0 : 3;
}

int main(int argc, char **argv)
{
    double processStart = GenNowSeconds();
    GenArgs args;
    if (ParseArgs(argc, argv, &args) != 0) return 2;

    /* --bench (piano 16/07/2026, sezione tier): esce PRIMA di GenCorpusConfigure/
     * GenEnsureDir/GenProgressWrite -- non deve toccare generated/ in alcun modo
     * (vedi il commento su RunBench sopra). */
    if (args.bench) return RunBench(&args);

    GenCorpusConfigure(args.seed, args.resume ? "resume" : (args.fallback ? "fallback" : "gen"));
    if (GenEnsureDir(args.outDir) != 0)
    {
        fprintf(stderr, "melting-gen: impossibile creare %s\n", args.outDir);
        return 3;
    }
    GenProgressWrite(args.outDir, "avvio", 0, "melting-gen avviato");

    /* M5 (DEC-005): il tema scelto si legge UNA VOLTA sola, qui, prima di
     * ogni ramo che ne ha bisogno (--print-json-prompt, --propose-themes non
     * lo usa, --from-json, la generazione normale, il ripiego). File assente
     * o malformato -> chosenPtr resta NULL, esattamente come --theme-file non
     * passato affatto (requisito 3: nessuna regressione). */
    GenChosenTheme chosenTheme;
    const GenChosenTheme *chosenPtr = NULL;
    if (args.themeFile && GenLoadChosenTheme(args.themeFile, &chosenTheme)) chosenPtr = &chosenTheme;

    /* Semi d'ispirazione: costruisce il prompt JSON e lo stampa, PRIMA di
     * qualunque caricamento modello -- serve ai test per verificare il
     * prompt (placeholder sostituiti, determinismo sul seed) senza aspettare
     * una generazione vera. */
    if (args.printJsonPrompt)
    {
        char *prompt = GenLlmBuildJsonPrompt(args.promptsDir, args.seed, chosenPtr);
        if (!prompt)
        {
            fprintf(stderr, "melting-gen: prompt JSON non costruibile (file mancanti in %s?)\n", args.promptsDir);
            return 6;
        }
        printf("%s", prompt);
        free(prompt);
        return 0;
    }

    /* M5 (DEC-005), requisito 1: altro ramo di uscita anticipata, PRIMA di
     * GenRemoveOldScripts -- --propose-themes non genera una run, non deve
     * toccare gli script Lua di quella in corso. */
    if (args.proposeThemes > 0) return RunProposeThemes(&args, args.proposeThemes, !args.noCharacter);

    /* Step B2 (correzione da review): una generazione NUOVA parte da una cartella
     * scripts/ pulita. Senza, gli script della run precedente restavano sul disco
     * e la RIPRESA in sottofondo (--resume, GenLuaLoadExisting) li adottava,
     * attaccando il comportamento di un oggetto di ieri a un oggetto di oggi. La
     * ripresa, ovviamente, NON deve pulire: quei file sono i suoi. */
    if (!args.resume) GenRemoveOldScripts(args.outDir);

    if (args.fromJson)
    {
        char *text = GenReadFile(args.fromJson);
        cJSON *root = text ? cJSON_Parse(text) : NULL;
        free(text);
        if (!root)
        {
            fprintf(stderr, "melting-gen: JSON non parsabile: %s\n", args.fromJson);
            GenProgressWrite(args.outDir, "errore", 100, "JSON non parsabile");
            return 4;
        }
        /* Step B2: in ripresa il seed NON e' una scelta libera -- deve essere
         * ESATTAMENTE quello della run che si sta giocando, perche' rarita', kind
         * e bossItem sono derivati dal seed (GenNormalizeRun li prende dal ripiego
         * procedurale, che e' deterministico sul seed). Un seed diverso
         * ricostruirebbe un contenuto DIVERSO da quello a schermo. Il JSON che il
         * primo processo ha scritto porta il proprio seed con se': si usa quello,
         * cosi' la cosa e' vera per costruzione e non dipende da chi lancia il
         * comando. */
        cJSON *seedNode = cJSON_GetObjectItemCaseSensitive(root, "seed");
        if (args.resume && cJSON_IsNumber(seedNode))
        {
            unsigned int jsonSeed = (unsigned int)seedNode->valuedouble;
            if (jsonSeed != args.seed)
            {
                GenLogLine("resume: uso il seed del JSON (%u) invece di quello passato (%u)", jsonSeed, args.seed);
                args.seed = jsonSeed;
                GenCorpusConfigure(args.seed, "resume");   /* il corpus riporta il seed VERO della run */
            }
        }

        GenRun run;
        GenProgressWrite(args.outDir, "valido", 60, "valido e normalizzo il JSON");
        GenNormalizeRun(root, args.seed, chosenPtr, &run);
        cJSON_Delete(root);
        snprintf(run.source, sizeof(run.source), "from-json");

        /* Step B2, il processo di RIPRESA (quello che gira in sottofondo mentre
         * si gioca): la run e' gia' stata inventata dal primo processo e sta su
         * disco come JSON -- qui non si tocca nulla di creativo, si scrivono solo
         * gli script Lua che MANCANO. GenLuaLoadExisting carica quelli gia' fatti
         * (il piano 1, di norma) cosi' non vengono ne' rigenerati ne' -- errore
         * ben peggiore -- persi dal manifest che stiamo per riscrivere. */
        if (args.resume)
        {
            int existing = GenLuaLoadExisting(&run, args.outDir);
            GenLogLine("resume: %d script Lua gia' presenti su disco, genero i mancanti", existing);

            const char *modelPath = NULL;
            GenLlmSession *sess = OpenModelSession(&args, &modelPath);
            if (sess)
            {
                GenCorpusRecordSession(modelPath, args.ngl);
                GenLuaStats luaStats;
                double deadline = processStart + GEN_LUA_RESUME_BUDGET_SEC;
                GenLuaGenerateForRun(sess, &run, args.promptsDir, args.outDir, deadline,
                                      GEN_FLOORS, true /* pubblica dopo OGNI piano */, &luaStats);
                GenLlmSessionClose(sess);
                snprintf(run.source, sizeof(run.source), "resume");
            }
            else GenLogLine("resume: nessun modello disponibile, niente da fare");
        }
        /* modelJsonField/modelLuaField contano solo quando !args.resume (vedi
         * WriteOutputs): qui e' il caso --from-json SENZA --resume (usato dai
         * test di normalizzazione, scripts/test-gen.sh) -- nessun modello gira
         * in QUESTO processo, il JSON arriva dal file indicato da --from-json,
         * e nessuna fase Lua viene eseguita (parte solo sotto args.resume qui
         * sopra). Quando args.resume E' vero questi due valori sono ignorati
         * del tutto: provenance.txt esiste gia' e non va toccato. */
        return WriteOutputs(&run, &args, args.fromJson, "-", chosenPtr);
    }

    GenRun run;
    int haveRun = 0;
    /* Provenienza (RunBundle v1): il percorso del modello che ha DAVVERO
     * prodotto il JSON/gli script Lua di questa run, o NULL se nessun
     * modello e' stato usato per quella fase (WriteOutputs sostituisce NULL
     * coi letterali "fallback"/"-"). Popolati mano a mano nei rami sotto,
     * mai retroattivamente: se un ramo fallisce a meta' (es. haveRun resta 0)
     * i valori restano NULL, ed e' la cosa giusta -- la run che si scrive
     * davvero e' quella del ripiego procedurale. */
    const char *provModelJson = NULL;
    const char *provModelLua = NULL;
    if (!args.fallback)
    {
        /* Limite tentativi JSON legato al timeout del genitore: src/app/app.c
         * (AppStartGeneration) manda SIGTERM a questo processo dopo 420s
         * (alzato da 180s proprio per fare posto alla fase Lua, vedi il
         * commento li'). Un tentativo JSON costa fino a ~76s (nPredict=2048
         * token a 28,1 tok/s sul 7B, docs/engineering/benchmarks.md); 2 tentativi
         * restano a ~152s, lasciando margine sia per GEN_LUA_PHASE_BUDGET_SEC
         * (300s assoluti dall'avvio del processo, vedi melting_gen.h) sia
         * per la scrittura finale se anche il secondo tentativo fallisce. */
        static char json[65536];

        /* Esperimento due-modelli (roadmap 17/07/2026): stessa logica di
         * esistenza degli altri modelli (GenFileExists). Un --model-text
         * passato ma mancante e' un avviso, non un errore: si prosegue col
         * comportamento di sempre, una sessione sola per JSON+Lua. */
        int useTextModel = 0;
        if (args.modelText)
        {
            if (GenFileExists(args.modelText)) useTextModel = 1;
            else GenLogLine("--model-text indicato ma assente: %s, uso una sessione sola come sempre", args.modelText);
        }

        if (useTextModel)
        {
            /* Sessione SEPARATA solo per il JSON (modello generalista,
             * migliore in prosa/nomi italiani del 7B Coder): i due modelli
             * non stanno insieme nei 6 GB di VRAM di riferimento, quindi
             * questa sessione si chiude SEMPRE prima di aprire quella del
             * Coder sotto -- anche quando il JSON e' fallito, cosi' il
             * ripiego procedurale non trova due sessioni aperte insieme. */
            GenLlmSession *jsonSess = GenLlmSessionOpen(args.modelText, args.ngl, args.outDir);
            if (jsonSess)
            {
                GenCorpusRecordSession(args.modelText, args.ngl);
                haveRun = RunJsonAttempts(jsonSess, args.modelText, &args, chosenPtr, &run, json, sizeof(json));
                if (haveRun) provModelJson = args.modelText;
                GenLlmSessionClose(jsonSess);
            }
            else GenLogLine("llm: sessione testo non apribile (%s), nessun JSON dal modello", args.modelText);

            if (haveRun)
            {
                /* Fase Lua: SEMPRE il Coder di sempre, mai il modello testo
                 * (gia' chiuso qui sopra). NOTA (dalla review adversariale):
                 * a differenza del percorso a sessione unica — dove un
                 * OpenModelSession fallito fa cadere l'INTERA run sul
                 * fallback procedurale — qui il JSON del modello testo e'
                 * gia' accettato, quindi la run viene scritta con TUTTI gli
                 * oggetti senza script Lua (item->lua vuoto = mini-VM). E'
                 * uno stato lecito ma nuovo: va dichiarato a voce alta nei
                 * log, non lasciato accadere in silenzio. */
                const char *luaModelPath = NULL;
                GenLlmSession *luaSess = OpenModelSession(&args, &luaModelPath);
                if (luaSess)
                {
                    GenCorpusRecordSession(luaModelPath, args.ngl);
                    RunLuaPhase(luaSess, &run, &args, processStart);
                    provModelLua = luaModelPath;
                    GenLlmSessionClose(luaSess);
                }
                else GenLogLine("llm: JSON dal modello testo accettato ma Coder non apribile: "
                                "run scritta SENZA script Lua (tutti gli oggetti su mini-VM)");
            }
        }
        else
        {
            /* Percorso di sempre: sessione condivisa (fase 3a-L3), il
             * modello si carica UNA VOLTA sola per l'intero processo e serve
             * sia i tentativi JSON sotto sia, a valle, i 20 script Lua
             * (GenLuaGenerateForRun). */
            const char *modelPath = NULL;
            GenLlmSession *sess = OpenModelSession(&args, &modelPath);
            if (sess)
            {
                GenCorpusRecordSession(modelPath, args.ngl);
                haveRun = RunJsonAttempts(sess, modelPath, &args, chosenPtr, &run, json, sizeof(json));
                if (haveRun)
                {
                    /* Stessa sessione per JSON e Lua su questo percorso (fase
                     * 3a-L3): un solo modello, quindi la stessa provenienza per
                     * entrambi i campi. */
                    provModelJson = modelPath;
                    RunLuaPhase(sess, &run, &args, processStart);
                    provModelLua = modelPath;
                }
                GenLlmSessionClose(sess);
            }
        }
    }
    if (!haveRun)
    {
        /* Ripiego procedurale: nessun modello ha prodotto questa run.
         * provModelJson/provModelLua sono gia' NULL qui (si valorizzano SOLO
         * dentro i rami "if (haveRun)" sopra, mai fuori), quindi WriteOutputs
         * scrivera' la provenienza coi letterali "fallback"/"-" -- niente da
         * fare qui, e' la cosa giusta per costruzione. */
        GenCorpusRecordFallback(args.fallback ? "richiesto con --fallback" : "modello assente o tentativi JSON esauriti",
                                 args.fallback != 0);
        GenFallbackRun(&run, args.seed, chosenPtr);
    }
    int rc = WriteOutputs(&run, &args, provModelJson, provModelLua, chosenPtr);
    /* Ledger di novita' fra run (gen_novelty.c): SOLO qui, MAI nel ramo
     * --resume/--from-json sopra (che ritorna prima di arrivare fin qui) e
     * MAI quando 'haveRun' e' rimasto 0 (run->source e' "fallback", lo
     * stesso vocabolario procedurale di sempre -- registrarlo avvelenerebbe
     * la lista "da evitare" con parole che il modello non ha mai scelto).
     * Anche il rc di WriteOutputs conta: una scrittura fallita a meta' (disco
     * pieno, permessi) non e' la run che il giocatore vedra' davvero, quindi
     * non deve entrare nella memoria delle run passate. */
    if (rc == 0 && haveRun) GenNoveltyAppend(&run);
    return rc;
}
