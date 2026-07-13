#include "melting_gen.h"

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
} GenArgs;

static int ParseArgs(int argc, char **argv, GenArgs *args)
{
    args->fallback = 0;
    args->emitLlmJson = 0;
    args->seed = (unsigned int)time(NULL);
    args->outDir = "generated";
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

    if (!args.fallback)
    {
        fprintf(stderr, "melting-gen: la generazione LLM arriva con i task successivi; usa --fallback\n");
        GenProgressWrite(args.outDir, "errore", 100, "modalita' non disponibile");
        return 2;
    }

    GenRun run;
    GenFallbackRun(&run, args.seed);
    return WriteOutputs(&run, &args);
}
