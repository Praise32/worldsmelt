#ifndef MELTING_GEN_H
#define MELTING_GEN_H

#include <stddef.h>

#define GEN_FLOORS 5
#define GEN_ITEMS 3
#define GEN_MAX_OPS 3

typedef struct GenScriptOp {
    char trigger[10];   /* "on_fire" | "on_hit" */
    char op[12];        /* "burst" | "projectile" | "area" | "heal" */
    double a;
    double b;
    char trait[10];     /* uno dei GEN_TRAITS oppure "none" */
} GenScriptOp;

typedef struct GenItem {
    char name[48];      /* stesso limite di Item.name in game_types.h */
    char slot[8];       /* uno dei GEN_SLOTS */
    char traits[2][10];
    int traitCount;     /* 1..2 */
    char color[8];      /* "#rrggbb" */
    GenScriptOp ops[GEN_MAX_OPS];
    int opCount;        /* 1..3 */
} GenItem;

typedef struct GenFloor {
    char theme[64];
    char style[48];
    char boss[64];
    char bg[8], floorColor[8], wall[8], accent[8], accent2[8], enemy[8], bossColor[8];
    GenItem items[GEN_ITEMS];
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
void GenProgressWrite(const char *outDir, const char *phase, int percent, const char *message);
void GenLogLine(const char *fmt, ...);
extern const char *GEN_SLOTS[6];
extern const char *GEN_TRAITS[9];
const GenTraitRule *GenTraitRuleFor(const char *trait);   /* NULL se sconosciuto */

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

#endif
