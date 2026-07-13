#include "melting_gen.h"

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
    GenProgressWrite(args->outDir, "scrivo", 85, "scrivo manifest e atlas");
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

        static char json[65536];
        /* Limite tentativi legato al timeout del genitore: src/app/app.c
         * (AppStartGeneration) manda SIGTERM a questo processo dopo 180s. Un
         * tentativo costa fino a ~76s (load 2,6s + nPredict=2048 token a
         * 28,1 tok/s, misurati sul 7B a ngl=99 in docs/BENCHMARKS.md), quindi
         * 2 tentativi restano a ~152s, con margine per la scrittura finale
         * del fallback deterministico che segue se anche il secondo fallisce.
         * Con 3 tentativi (~228s) il genitore ucciderebbe questo processo
         * PRIMA che possa arrivare a GenFallbackRun, lasciando il giocatore
         * con la run stantia precedente invece di quella nuova garantita dal
         * design: se uno di questi due numeri cambia, va ricontrollato anche
         * l'altro. */
        for (int attempt = 0; modelPath && attempt < 2 && !haveRun; attempt++)
        {
            GenLlmConfig cfg = {
                .modelPath = modelPath,
                .nGpuLayers = args.ngl,
                .nPredict = args.nPredict,
                .temp = args.temp,
                .seed = args.seed + (unsigned int)attempt*7919u,
                .outDir = args.outDir,
                .promptsDir = args.promptsDir,
                .grammarPath = args.grammarPath,
            };
            double loadSecs = 0, genSecs = 0;
            int tokens = 0;
            if (GenLlmGenerate(&cfg, json, sizeof(json), &loadSecs, &genSecs, &tokens) != 0)
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
            GenProgressWrite(args.outDir, "valido", 94, "valido e normalizzo");
            GenNormalizeRun(root, args.seed, &run);
            cJSON_Delete(root);
            const char *base = strrchr(modelPath, '/');
            snprintf(run.source, sizeof(run.source), "local:%s", base ? base + 1 : modelPath);
            GenLogLine("ok: model=%s ngl=%d load=%.1fs gen=%.1fs token=%d (%.1f tok/s)",
                       modelPath, args.ngl, loadSecs, genSecs, tokens,
                       genSecs > 0 ? tokens/genSecs : 0.0);
            haveRun = 1;
        }
    }
    if (!haveRun) GenFallbackRun(&run, args.seed);
    return WriteOutputs(&run, &args);
}
