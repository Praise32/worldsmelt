#include "melting_gen.h"

#include "gen_lua.h"

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
    int ngl;
    float temp;
    int nPredict;
    const char *promptsDir;
    const char *grammarPath;
} GenArgs;

static int ParseArgs(int argc, char **argv, GenArgs *args)
{
    args->fallback = 0;
    args->emitLlmJson = 0;
    args->seed = (unsigned int)time(NULL);
    args->outDir = "generated";
    args->fromJson = NULL;
    /* Calibrati nel Task 8 su Ryzen 5 3600 + RX 5600 XT 6GB (dettagli in
     * docs/BENCHMARKS.md): a ngl=99 il 7B occupa ~4.53 GiB di VRAM e completa
     * in 49.6s totali, la corsa piu' veloce E con la qualita' migliore
     * dell'intera matrice misurata (batte anche il fallback 1.5B). */
    args->model = "models/qwen2.5-coder-7b-instruct-q4_k_m.gguf";
    args->modelFallback = "models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf";
    args->ngl = 99;
    args->temp = 0.8f;
    args->nPredict = 2048;
    args->promptsDir = "tools/melting-gen/prompts";
    args->grammarPath = "tools/melting-gen/run.gbnf";
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
        else if (strcmp(argv[i], "--ngl") == 0 && i + 1 < argc) args->ngl = atoi(argv[++i]);
        else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) args->temp = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--n-predict") == 0 && i + 1 < argc) args->nPredict = atoi(argv[++i]);
        else if (strcmp(argv[i], "--prompts") == 0 && i + 1 < argc) args->promptsDir = argv[++i];
        else if (strcmp(argv[i], "--grammar") == 0 && i + 1 < argc) args->grammarPath = argv[++i];
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
        else
        {
            fprintf(stderr, "melting-gen: opzione sconosciuta: %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

static int WriteOutputs(const GenRun *run, const GenArgs *args)
{
    /* 99, non 85: la fase Lua (fase 3a-L3, quando c'e' un modello) scrive
     * progresso fino al 98% (vedi GenLuaGenerateForRun in gen_lua.c), la
     * barra deve restare monotona crescente fino a "fine" a 100. */
    GenProgressWrite(args->outDir, "scrivo", 99, "scrivo manifest e atlas");
    if (GenWriteAtlasBmp(run, args->outDir) != 0 || GenWriteRunFiles(run, args->outDir) != 0)
    {
        GenProgressWrite(args->outDir, "errore", 100, "scrittura file fallita");
        return 3;
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

int main(int argc, char **argv)
{
    double processStart = GenNowSeconds();
    GenArgs args;
    if (ParseArgs(argc, argv, &args) != 0) return 2;
    if (GenEnsureDir(args.outDir) != 0)
    {
        fprintf(stderr, "melting-gen: impossibile creare %s\n", args.outDir);
        return 3;
    }
    GenProgressWrite(args.outDir, "avvio", 0, "melting-gen avviato");

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
        GenRun run;
        GenProgressWrite(args.outDir, "valido", 60, "valido e normalizzo il JSON");
        GenNormalizeRun(root, args.seed, &run);
        cJSON_Delete(root);
        snprintf(run.source, sizeof(run.source), "from-json");
        return WriteOutputs(&run, &args);
    }

    GenRun run;
    int haveRun = 0;
    if (!args.fallback)
    {
        const char *modelPath = NULL;
        if (GenFileExists(args.model)) modelPath = args.model;
        else if (GenFileExists(args.modelFallback))
        {
            modelPath = args.modelFallback;
            GenLogLine("modello principale assente, ripiego su %s", modelPath);
        }
        else GenLogLine("nessun modello in models/: uso il fallback deterministico");

        /* Sessione condivisa (fase 3a-L3): il modello si carica UNA VOLTA
         * sola per l'intero processo e serve sia i tentativi JSON sotto sia,
         * a valle, i 15 script Lua (GenLuaGenerateForRun). Limite tentativi
         * JSON legato al timeout del genitore: src/app/app.c
         * (AppStartGeneration) manda SIGTERM a questo processo dopo 420s
         * (alzato da 180s proprio per fare posto alla fase Lua, vedi il
         * commento li'). Un tentativo JSON costa fino a ~76s (nPredict=2048
         * token a 28,1 tok/s sul 7B, docs/BENCHMARKS.md); 2 tentativi
         * restano a ~152s, lasciando margine sia per GEN_LUA_PHASE_BUDGET_SEC
         * (300s assoluti dall'avvio del processo, vedi melting_gen.h) sia
         * per la scrittura finale se anche il secondo tentativo fallisce. */
        static char json[65536];
        if (modelPath)
        {
            GenLlmSession *sess = GenLlmSessionOpen(modelPath, args.ngl, args.outDir);
            if (sess)
            {
                char *grammar = GenReadFile(args.grammarPath);
                for (int attempt = 0; attempt < 2 && !haveRun && grammar; attempt++)
                {
                    unsigned int attemptSeed = args.seed + (unsigned int)attempt*7919u;
                    char *prompt = GenLlmBuildJsonPrompt(args.promptsDir, attemptSeed);
                    if (!prompt)
                    {
                        GenLogLine("tentativo %d: prompt JSON non costruibile (file mancanti in %s?)", attempt + 1, args.promptsDir);
                        continue;
                    }
                    double t0 = GenNowSeconds();
                    int tokens = 0;
                    int rc = GenLlmComplete(sess, prompt, grammar, args.nPredict, args.temp, attemptSeed,
                                             args.outDir, "genero", 62, 30, json, sizeof(json), &tokens);
                    double genSecs = GenNowSeconds() - t0;
                    free(prompt);
                    if (rc != 0)
                    {
                        GenLogLine("tentativo %d: generazione fallita", attempt + 1);
                        continue;
                    }
                    cJSON *root = cJSON_Parse(json);
                    if (!root)
                    {
                        GenLogLine("tentativo %d: JSON troncato o non parsabile (%d token)", attempt + 1, tokens);
                        continue;
                    }
                    GenProgressWrite(args.outDir, "valido", 92, "valido e normalizzo");
                    GenNormalizeRun(root, args.seed, &run);
                    cJSON_Delete(root);
                    const char *base = strrchr(modelPath, '/');
                    snprintf(run.source, sizeof(run.source), "local:%s", base ? base + 1 : modelPath);
                    GenLogLine("ok: model=%s ngl=%d gen=%.1fs token=%d (%.1f tok/s)",
                               modelPath, args.ngl, genSecs, tokens, genSecs > 0 ? tokens/genSecs : 0.0);
                    haveRun = 1;
                }
                free(grammar);

                if (haveRun)
                {
                    double luaDeadline = processStart + GEN_LUA_PHASE_BUDGET_SEC;
                    GenLuaStats luaStats;
                    GenLuaGenerateForRun(sess, &run, args.promptsDir, args.outDir, luaDeadline, &luaStats);
                    GenLogLine("lua: %d/15 primo tentativo, %d dopo retry, %d senza comportamento, %d ripiegati su mini-VM, %d saltati per budget",
                               luaStats.firstTry, luaStats.afterRetry, luaStats.optedOut, luaStats.fellBack, luaStats.skippedBudget);
                }
                GenLlmSessionClose(sess);
            }
        }
    }
    if (!haveRun) GenFallbackRun(&run, args.seed);
    return WriteOutputs(&run, &args);
}
