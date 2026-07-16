#include "gen_corpus.h"

#include "melting_gen.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Singleton di processo, aperto pigramente alla prima riga: il processo di
   generazione e quello di ripresa (--resume) convivono, ciascuno scrive il
   proprio file (pid nel nome). Ogni riga viene flushata subito: il genitore
   puo' mandare SIGTERM in qualunque momento (timeout di gen_runner.c) e le
   righe gia' scritte devono sopravvivere. */
static FILE *sFile = NULL;
static bool sTried = false;
static unsigned int sSeed = 0;
static char sMode[16] = "gen";

void GenCorpusConfigure(unsigned int seed, const char *mode)
{
    sSeed = seed;
    snprintf(sMode, sizeof(sMode), "%s", mode ? mode : "gen");
}

static FILE *CorpusFile(void)
{
    if (sTried) return sFile;
    sTried = true;
    if (getenv("MELTING_GEN_NO_CORPUS")) return NULL;
    mkdir("logs", 0755);
    mkdir("logs/gen-corpus", 0755);
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char path[256];
    snprintf(path, sizeof(path), "logs/gen-corpus/%04d%02d%02d-%02d%02d%02d-seed%u-%s-pid%d.jsonl",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
             sSeed, sMode, (int)getpid());
    sFile = fopen(path, "w");
    if (sFile) GenLogLine("corpus: registro le generazioni in %s", path);
    return sFile;
}

/* Prende POSSESSO di rec (lo libera sempre). I campi comuni a ogni riga
   (seed, modalita', ora) vengono aggiunti qui, una volta sola. */
static void CorpusWrite(cJSON *rec)
{
    FILE *f = CorpusFile();
    if (!f)
    {
        cJSON_Delete(rec);
        return;
    }
    cJSON_AddNumberToObject(rec, "seed", (double)sSeed);
    cJSON_AddStringToObject(rec, "mode", sMode);
    cJSON_AddNumberToObject(rec, "time", (double)time(NULL));
    char *line = cJSON_PrintUnformatted(rec);
    if (line)
    {
        fputs(line, f);
        fputc('\n', f);
        fflush(f);
        free(line);
    }
    cJSON_Delete(rec);
}

void GenCorpusRecordSession(const char *modelPath, int ngl)
{
    cJSON *rec = cJSON_CreateObject();
    if (!rec) return;
    cJSON_AddStringToObject(rec, "kind", "session");
    cJSON_AddStringToObject(rec, "model", modelPath ? modelPath : "");
    cJSON_AddNumberToObject(rec, "ngl", ngl);
    CorpusWrite(rec);
}

void GenCorpusRecordJson(int attempt, bool ok, const char *reason,
                          double seconds, int tokens, const char *raw)
{
    cJSON *rec = cJSON_CreateObject();
    if (!rec) return;
    cJSON_AddStringToObject(rec, "kind", "manifest");
    cJSON_AddNumberToObject(rec, "attempt", attempt);
    cJSON_AddBoolToObject(rec, "ok", ok);
    if (reason) cJSON_AddStringToObject(rec, "reason", reason);
    cJSON_AddNumberToObject(rec, "secs", seconds);
    cJSON_AddNumberToObject(rec, "tokens", tokens);
    /* Il JSON grezzo va registrato ANCHE quando non parsa: gli errori del
       modello sono la meta' interessante del dataset. */
    if (raw) cJSON_AddStringToObject(rec, "raw", raw);
    CorpusWrite(rec);
}

void GenCorpusRecordLua(const char *floorTheme, const char *itemName, bool statUp,
                         int attempt, const char *outcome, const char *error,
                         const char *script)
{
    cJSON *rec = cJSON_CreateObject();
    if (!rec) return;
    cJSON_AddStringToObject(rec, "kind", "lua");
    cJSON_AddStringToObject(rec, "theme", floorTheme ? floorTheme : "");
    cJSON_AddStringToObject(rec, "item", itemName ? itemName : "");
    cJSON_AddBoolToObject(rec, "statUp", statUp);
    cJSON_AddNumberToObject(rec, "attempt", attempt);
    cJSON_AddStringToObject(rec, "outcome", outcome ? outcome : "");
    if (error && error[0]) cJSON_AddStringToObject(rec, "error", error);
    if (script && script[0]) cJSON_AddStringToObject(rec, "script", script);
    CorpusWrite(rec);
}

void GenCorpusRecordFallback(const char *reason, bool explicitRequest)
{
    cJSON *rec = cJSON_CreateObject();
    if (!rec) return;
    cJSON_AddStringToObject(rec, "kind", "fallback");
    cJSON_AddStringToObject(rec, "reason", reason ? reason : "");
    cJSON_AddBoolToObject(rec, "explicit", explicitRequest);
    CorpusWrite(rec);
}
