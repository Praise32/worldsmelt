#include "melting_gen.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

double GenNowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

char *GenReplaceAll(const char *src, const char *find, const char *repl)
{
    if (!src) return NULL;
    if (!find || !find[0]) return strdup(src);
    const char *safeRepl = repl ? repl : "";
    size_t findLen = strlen(find);
    size_t replLen = strlen(safeRepl);

    size_t count = 0;
    for (const char *p = src; (p = strstr(p, find)) != NULL; p += findLen) count++;

    /* Se repl e' piu' corto di find l'output e' SOLO piu' corto di src:
       strlen(src)+1 resta una stima generosa (mai insufficiente) e non vale
       la pena calcolare la dimensione esatta in quel ramo. */
    size_t extra = (replLen > findLen) ? count*(replLen - findLen) : 0;
    char *out = malloc(strlen(src) + extra + 1);
    if (!out) return NULL;

    char *w = out;
    const char *r = src;
    const char *hit;
    while ((hit = strstr(r, find)) != NULL)
    {
        size_t chunk = (size_t)(hit - r);
        memcpy(w, r, chunk); w += chunk;
        memcpy(w, safeRepl, replLen); w += replLen;
        r = hit + findLen;
    }
    strcpy(w, r);
    return out;
}

char *GenChatMlWrap(const char *sys, const char *user)
{
    if (!sys || !user) return NULL;
    size_t total = strlen(sys) + strlen(user) + 128;
    char *prompt = malloc(total);
    if (!prompt) return NULL;
    snprintf(prompt, total,
             "<|im_start|>system\n%s<|im_end|>\n<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n",
             sys, user);
    return prompt;
}

unsigned int GenRngNext(unsigned int *state)
{
    unsigned int s = *state ? *state : 0xA341316Cu;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    if (!s) s = 0xA341316Cu;
    *state = s;
    return s;
}

int GenRngRange(unsigned int *state, int min, int max)
{
    if (max <= min) return min;
    return min + (int)(GenRngNext(state) % (unsigned int)(max - min + 1));
}

void GenHsvToHex(double h, double s, double v, char out[8])
{
    h = fmod(fmod(h, 360.0) + 360.0, 360.0);
    double c = v*s;
    double m = fmod(h/60.0, 2.0) - 1.0;
    double x = c*(1.0 - (m < 0 ? -m : m));
    double base = v - c;
    double r = 0, g = 0, b = 0;
    if (h < 60)       { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else              { r = c; b = x; }
    snprintf(out, 8, "#%02x%02x%02x",
             (int)lround((r + base)*255.0),
             (int)lround((g + base)*255.0),
             (int)lround((b + base)*255.0));
}

int GenEnsureDir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

char *GenReadFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

int GenFileExists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message)
{
    char tmp[512], fin[512];
    snprintf(fin, sizeof(fin), "%s/gen_progress.txt", outDir);
    snprintf(tmp, sizeof(tmp), "%s/gen_progress.tmp", outDir);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "%s|%d|%s\n", phase, percent, message);
    fclose(f);
    rename(tmp, fin);
}

int GenPublishFile(FILE *f, const char *tmpPath, const char *finalPath)
{
    /* ferror() va controllato PRIMA di fclose(): dopo la chiusura lo stream
       non e' piu' valido. Un disco pieno fa fallire le fprintf/fwrite senza
       che il chiamante lo controlli riga per riga: e' qui che l'errore viene
       intercettato prima che il file troncato prenda il posto di quello
       valido. */
    int hadError = ferror(f);
    int closeFailed = (fclose(f) != 0);
    if (hadError || closeFailed)
    {
        remove(tmpPath);
        return -1;
    }
    if (rename(tmpPath, finalPath) != 0)
    {
        remove(tmpPath);
        return -1;
    }
    return 0;
}

void GenLogLine(const char *fmt, ...)
{
    GenEnsureDir("logs");
    FILE *f = fopen("logs/melting-gen.log", "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "[%s] ", stamp);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}

/* Tabelle condivise: stesse regole di TRAIT_SCRIPT_RULES in llm/run_content.mjs. */
const char *GEN_SLOTS[6] = { "hat", "eyes", "hand", "back", "body", "aura" };
const char *GEN_TRAITS[9] = {
    "bounce", "homing", "explode", "split", "pierce", "rapid", "giant", "slow", "vamp"
};
/* Vedi il commento su GenItem.kind in melting_gen.h: mai scelto dal modello,
 * sempre assegnato in C secondo la posizione dell'oggetto (items[] contro
 * bossItem). */
const char *GEN_KINDS[2] = { "active", "statup" };

static const GenTraitRule GEN_TRAIT_RULES[9] = {
    { "bounce",  "on_fire", "burst",       2, 0.25 },
    { "homing",  "on_hit",  "projectile",  2, 260  },
    { "explode", "on_hit",  "area",       58, 0.48 },
    { "split",   "on_fire", "burst",       3, 0.36 },
    { "pierce",  "on_hit",  "projectile",  1, 420  },
    { "rapid",   "on_fire", "burst",       2, 0.16 },
    { "giant",   "on_hit",  "area",       44, 0.34 },
    { "slow",    "on_hit",  "area",       54, 0.22 },
    { "vamp",    "on_hit",  "heal",       18, 1    },
};

const GenTraitRule *GenTraitRuleFor(const char *trait)
{
    for (int i = 0; i < 9; i++)
    {
        if (trait && strcmp(GEN_TRAIT_RULES[i].trait, trait) == 0) return &GEN_TRAIT_RULES[i];
    }
    return NULL;
}
