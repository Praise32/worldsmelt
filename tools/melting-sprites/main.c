/* CLI e orchestrazione di melting-sprites.

   In questa fase (S1/S2 della roadmap di fase 2) il tool non collega ancora
   Stable Diffusion: solo --dry-run e' implementato, per rendere testabile
   tutta la pipeline di post-processing senza modello (vedi
   docs/superpowers/specs/2026-07-13-local-sprites-design.md). Un task
   successivo aggiunge la generazione vera. */
#include "melting_sprites.h"

#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef struct SpritesArgs {
    const char *outDir;
    unsigned int seed;
    int dryRun;
    int check;
    int cells;
} SpritesArgs;

static int EnsureDir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

static int ParseArgs(int argc, char **argv, SpritesArgs *args)
{
    args->outDir = "generated";
    args->seed = (unsigned int)time(NULL);
    args->dryRun = 0;
    args->check = 0;
    args->cells = SPRITE_CELLS;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) args->outDir = argv[++i];
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) args->seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--dry-run") == 0) args->dryRun = 1;
        else if (strcmp(argv[i], "--check") == 0) args->check = 1;
        else if (strcmp(argv[i], "--cells") == 0 && i + 1 < argc) args->cells = atoi(argv[++i]);
        else
        {
            fprintf(stderr, "melting-sprites: opzione sconosciuta: %s\n", argv[i]);
            return -1;
        }
    }
    if (args->cells < 1) args->cells = 1;
    if (args->cells > SPRITE_CELLS) args->cells = SPRITE_CELLS;
    return 0;
}

/* ---------------------------------------------------------------------
 * Sintesi delle celle di prova per --dry-run.
 *
 * Costruisce direttamente in memoria una sorgente 512x512 "difficile" per
 * ogni cella, senza toccare il modello: contorno nero, un pixel nero puro
 * dentro lo sprite (l'"occhio"), un'ombreggiatura quasi nera, una macchia
 * del colore esatto di sfondo racchiusa nel corpo, e uno sfondo rumoroso
 * (non piatto). Sono esattamente i casi che lo spike ha misurato rompere
 * una soglia di luminosita' globale (docs/SPRITES-SPIKE.md, "Le due cose
 * imparate"): se il ritaglio regredisse a quell'approccio, questi test
 * fallirebbero.
 *
 * Il disco pieno (corpo+contorno) e' generato a risoluzione logica 128x128
 * e poi replicato 4x4 sulla sorgente 512x512: ogni tassello che il downscale
 * modale vedra' e' quindi perfettamente uniforme, e il giro di andata e
 * ritorno e' esatto. Il rumore si aggiunge SOLO dopo, sui pixel di sfondo.
 *
 * Le feature "difficili" sono posizionate come offset fissi rispetto al
 * centro del disco (che puo' variare da cella a cella): dato che
 * SpritesCenterInCell ricentra sempre il riquadro del disco esattamente sul
 * centro della cella (128/2, 128/2) -- il disco e' simmetrico, quindi il suo
 * riquadro coincide esattamente col suo centro, senza alcun arrotondamento
 * -- la posizione FINALE di ogni feature nell'atlas composto e' sempre la
 * stessa, nota, indipendente dal centro scelto in fase di sintesi. E'
 * questo che rende --check verificabile senza dover condividere lo stato
 * della sintesi. */

#define DRY_R_OUT 30
#define DRY_R_IN  27
#define DRY_EYE_DX (-14)
#define DRY_EYE_DY (-14)
#define DRY_EYE_HALF 2        /* blocco 5x5 */
#define DRY_PATCH_DX 14
#define DRY_PATCH_DY (-2)
#define DRY_PATCH_HALF 3      /* blocco 7x7 */
#define DRY_SHADE_X 14
#define DRY_SHADE_Y0 16
#define DRY_SHADE_Y1 20

static unsigned int DryRngNext(unsigned int *s)
{
    unsigned int x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 0x9E3779B9u;
    *s = x;
    return x;
}

static int DryRngRange(unsigned int *s, int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(DryRngNext(s) % (unsigned int)(hi - lo + 1));
}

static void DryPickColor(unsigned int *rng, int lo, int hi, unsigned char out[3])
{
    for (int k = 0; k < 3; k++) out[k] = (unsigned char)DryRngRange(rng, lo, hi);
}

/* src512: buffer SPRITE_SRC*SPRITE_SRC*3 (RGB) gia' allocato dal chiamante. */
static void SynthesizeCell(unsigned int seed, int cellIndex, unsigned char *src512)
{
    unsigned int rng = seed ^ ((unsigned int)cellIndex*0x9E3779B1u) ^ 0xC2B2AE35u;
    for (int i = 0; i < 4; i++) DryRngNext(&rng);   /* mescola un seme debole */

    unsigned char bg[3], body[3];
    /* Intervalli disgiunti: sfondo sempre chiaro, corpo sempre piu' scuro,
       cosi' la distanza di colore fra i due resta ampia per costruzione e
       il flood fill non puo' mai mangiare lo sprite intero. */
    DryPickColor(&rng, 170, 230, bg);
    DryPickColor(&rng, 30, 110, body);

    int cx = SPRITE_CELL/2 + DryRngRange(&rng, -10, 10);
    int cy = SPRITE_CELL/2 + DryRngRange(&rng, -10, 10);

    unsigned char logical[SPRITE_CELL*SPRITE_CELL*3];
    for (int y = 0; y < SPRITE_CELL; y++)
    for (int x = 0; x < SPRITE_CELL; x++)
    {
        int dx = x - cx, dy = y - cy, d2 = dx*dx + dy*dy;
        unsigned char c[3];
        if (d2 > DRY_R_OUT*DRY_R_OUT) { memcpy(c, bg, 3); }
        else if (d2 >= DRY_R_IN*DRY_R_IN) { c[0] = c[1] = c[2] = 0; }   /* contorno nero */
        else
        {
            int ex = x - (cx + DRY_EYE_DX), ey = y - (cy + DRY_EYE_DY);
            int qx = x - (cx + DRY_PATCH_DX), qy = y - (cy + DRY_PATCH_DY);
            if (ex >= -DRY_EYE_HALF && ex <= DRY_EYE_HALF && ey >= -DRY_EYE_HALF && ey <= DRY_EYE_HALF)
                { c[0] = c[1] = c[2] = 0; }                             /* occhio: nero puro */
            else if (qx >= -DRY_PATCH_HALF && qx <= DRY_PATCH_HALF && qy >= -DRY_PATCH_HALF && qy <= DRY_PATCH_HALF)
                { memcpy(c, bg, 3); }                                   /* macchia color-sfondo racchiusa */
            else if (y >= cy + DRY_SHADE_Y0 && y <= cy + DRY_SHADE_Y1 && x >= cx - DRY_SHADE_X && x <= cx + DRY_SHADE_X)
                { c[0] = 10; c[1] = 9; c[2] = 8; }                      /* ombreggiatura quasi nera */
            else memcpy(c, body, 3);
        }
        memcpy(logical + (size_t)(y*SPRITE_CELL + x)*3, c, 3);
    }

    const int f = SPRITE_DOWNSCALE_F;
    for (int y = 0; y < SPRITE_SRC; y++)
    for (int x = 0; x < SPRITE_SRC; x++)
    {
        int lx = x/f, ly = y/f;
        const unsigned char *c = logical + (size_t)(ly*SPRITE_CELL + lx)*3;
        int dx = lx - cx, dy = ly - cy;
        unsigned char out[3];
        if (dx*dx + dy*dy > DRY_R_OUT*DRY_R_OUT)
        {
            /* Sfondo rumoroso: un vero output di SD non e' mai piatto. */
            for (int k = 0; k < 3; k++)
            {
                int v = (int)c[k] + DryRngRange(&rng, -10, 10);
                out[k] = (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
            }
        }
        else memcpy(out, c, 3);
        memcpy(src512 + ((size_t)y*SPRITE_SRC + (size_t)x)*3, out, 3);
    }
}

/* ---------------------------------------------------------------------
 * Generazione (solo --dry-run in questa fase).
 */
static int RunGenerate(const SpritesArgs *args)
{
    if (EnsureDir(args->outDir) != 0)
    {
        fprintf(stderr, "melting-sprites: impossibile creare %s\n", args->outDir);
        return 3;
    }

    unsigned char *atlas = SpritesAtlasNew();
    unsigned char *src512 = malloc((size_t)SPRITE_SRC*SPRITE_SRC*3);
    unsigned char *cellBuf = malloc((size_t)SPRITE_CELL*SPRITE_CELL*4);
    if (!atlas || !src512 || !cellBuf)
    {
        fprintf(stderr, "melting-sprites: memoria esaurita\n");
        free(atlas); free(src512); free(cellBuf);
        return 3;
    }

    for (int i = 0; i < args->cells; i++)
    {
        SynthesizeCell(args->seed, i, src512);
        SpritePostStats stats;
        SpritesPostProcessCell(src512, cellBuf, SPRITE_PALETTE_COLORS, &stats);
        SpritesComposeAtlas(atlas, i, cellBuf);
        fprintf(stderr, "melting-sprites: cella %2d/%d tagliati=%5d opachi=%5d a-rischio=%d\n",
                i + 1, args->cells, stats.cutPixels, stats.opaquePixels, stats.keyRiskPixels);
    }
    free(src512); free(cellBuf);

    int rc = SpritesWriteAtlasPng(atlas, args->outDir);
    free(atlas);
    if (rc != 0)
    {
        fprintf(stderr, "melting-sprites: scrittura PNG fallita\n");
        return 3;
    }

    printf("melting-sprites: atlas scritto in %s/current_atlas.png (%d celle, seed=%u)\n",
           args->outDir, args->cells, args->seed);
    return 0;
}

/* ---------------------------------------------------------------------
 * Verifica dell'atlas scritto (usato da scripts/test-sprites.sh): decodifica
 * il PNG vero con stb_image e stampa, per ogni cella, statistiche verificabili
 * da uno script. Non serve un decoder PNG in bash: il tool e' gia' li'. */
static int RunCheck(const SpritesArgs *args)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/current_atlas.png", args->outDir);
    int w, h, ch;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px)
    {
        fprintf(stderr, "melting-sprites: impossibile leggere %s\n", path);
        return 1;
    }
    if (w != SPRITE_ATLAS_W || h != SPRITE_ATLAS_W)
    {
        fprintf(stderr, "melting-sprites: atteso %dx%d, trovato %dx%d\n", SPRITE_ATLAS_W, SPRITE_ATLAS_W, w, h);
        stbi_image_free(px);
        return 1;
    }

    for (int i = 0; i < args->cells; i++)
    {
        int col = i % SPRITE_ATLAS_COLS, row = i / SPRITE_ATLAS_COLS;
        int ox = col*SPRITE_CELL, oy = row*SPRITE_CELL;
        long opaque = 0, borderTotal = 0, borderTransparent = 0, keyRisk = 0;
        for (int y = 0; y < SPRITE_CELL; y++)
        for (int x = 0; x < SPRITE_CELL; x++)
        {
            const unsigned char *p = px + ((size_t)(oy + y)*(size_t)w + (size_t)(ox + x))*4;
            int isBorder = (x == 0 || y == 0 || x == SPRITE_CELL-1 || y == SPRITE_CELL-1);
            if (isBorder) { borderTotal++; if (p[3] == 0) borderTransparent++; }
            if (p[3] == 0) continue;
            opaque++;
            int mx = p[0] > p[1] ? (p[0] > p[2] ? p[0] : p[2]) : (p[1] > p[2] ? p[1] : p[2]);
            if (mx < SPRITE_KEY_FLOOR) keyRisk++;
        }
        int eyeX = ox + SPRITE_CELL/2 + DRY_EYE_DX, eyeY = oy + SPRITE_CELL/2 + DRY_EYE_DY;
        int patchX = ox + SPRITE_CELL/2 + DRY_PATCH_DX, patchY = oy + SPRITE_CELL/2 + DRY_PATCH_DY;
        int eyeOpaque = px[((size_t)eyeY*(size_t)w + (size_t)eyeX)*4 + 3] != 0;
        int patchOpaque = px[((size_t)patchY*(size_t)w + (size_t)patchX)*4 + 3] != 0;
        double borderRatio = borderTotal ? (double)borderTransparent/(double)borderTotal : 0.0;
        printf("cell=%d opaque=%ld borderCutRatio=%.3f keyRisk=%ld eyeOpaque=%d patchOpaque=%d\n",
               i, opaque, borderRatio, keyRisk, eyeOpaque, patchOpaque);
    }
    stbi_image_free(px);
    (void)ch;
    return 0;
}

int main(int argc, char **argv)
{
    SpritesArgs args;
    if (ParseArgs(argc, argv, &args) != 0) return 2;

    if (args.check) return RunCheck(&args);

    if (!args.dryRun)
    {
        fprintf(stderr,
            "melting-sprites: la generazione con Stable Diffusion non e' ancora collegata "
            "in questa fase; usa --dry-run per testare la pipeline di post-processing.\n");
        return 1;
    }

    return RunGenerate(&args);
}
