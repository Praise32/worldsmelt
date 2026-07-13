/* CLI e orchestrazione di melting-sprites.

   Dalla fase S3 in poi il tool collega davvero stable-diffusion.cpp (vedi
   sprite_sd.c): --dry-run resta disponibile e usato dai test (sintetizza
   celle di prova senza modello, vedi SynthesizeCell piu' sotto), ma il
   comportamento di default e' generare le 12 celle con Stable Diffusion, dal
   tema/stile del manifest (sprite_manifest.c) e dai template di
   tools/melting-sprites/prompts/ (sprite_prompt.c). Se il modello non c'e' o
   il caricamento fallisce, il tool esce con codice diverso da zero SENZA
   scrivere l'atlas ne' toccare il manifest (vedi RunGenerate piu' sotto):
   le celle sintetiche di --dry-run restano disponibili solo dietro quel
   flag esplicito, mai come ripiego silenzioso (vedi
   docs/superpowers/specs/2026-07-13-local-sprites-design.md). */
#include "melting_sprites.h"

#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPRITE_CELL_PIXELS (SPRITE_CELL*SPRITE_CELL)

typedef struct SpritesArgs {
    const char *outDir;
    unsigned int seed;
    int dryRun;
    int check;
    int cells;
    const char *model;
    const char *lora;
    int useTaesd;
    int steps;
    float cfg;
    const char *promptsDir;
    int dryRunBadBorder;   /* --dry-run-bad-border: solo per test, vedi sotto */
} SpritesArgs;

static int ParseArgs(int argc, char **argv, SpritesArgs *args)
{
    args->outDir = "generated";
    args->seed = (unsigned int)time(NULL);
    args->dryRun = 0;
    args->check = 0;
    args->cells = SPRITE_CELLS;
    /* Misurati nello spike (docs/SPRITES-SPIKE.md): non ridiscussi qui. */
    args->model = "models/Public-Prompts-Pixel-Model.ckpt";
    args->lora = "models/lcm-lora-sdv1-5.safetensors";
    args->useTaesd = 0;   /* di default la VAE reale: piu' nitida, ~1s in piu' per cella */
    args->steps = 8;
    /* cfg 1.8: a 1.5 il modello ignorava il soggetto (la chiave diventava un
       televisore); a 2.5 obbedisce ma lo sfondo esce a bande e il ritaglio le
       lascerebbe dentro. 1.8 e' il punto in cui il soggetto si legge e lo sfondo
       resta piatto. Misurato, non scelto a caso. */
    args->cfg = 1.8f;
    args->promptsDir = "tools/melting-sprites/prompts";
    args->dryRunBadBorder = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) args->outDir = argv[++i];
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) args->seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--dry-run") == 0) args->dryRun = 1;
        else if (strcmp(argv[i], "--dry-run-bad-border") == 0) { args->dryRun = 1; args->dryRunBadBorder = 1; }
        else if (strcmp(argv[i], "--check") == 0) args->check = 1;
        else if (strcmp(argv[i], "--cells") == 0 && i + 1 < argc) args->cells = atoi(argv[++i]);
        else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) args->model = argv[++i];
        else if (strcmp(argv[i], "--lora") == 0 && i + 1 < argc) args->lora = argv[++i];
        else if (strcmp(argv[i], "--taesd") == 0) args->useTaesd = 1;
        else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) args->steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cfg") == 0 && i + 1 < argc) args->cfg = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--prompts") == 0 && i + 1 < argc) args->promptsDir = argv[++i];
        else
        {
            fprintf(stderr, "melting-sprites: opzione sconosciuta: %s\n", argv[i]);
            return -1;
        }
    }
    if (args->cells < 1) args->cells = 1;
    if (args->cells > SPRITE_CELLS) args->cells = SPRITE_CELLS;
    if (args->steps < 1) args->steps = 1;
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

/* Variante di SynthesizeCell usata SOLO da --dry-run-bad-border (test): il
   disco e' centrato a 8px dal bordo sinistro della cella invece che al
   centro, cosi' il flood fill non ha margine di sfondo su quel lato e il
   riquadro dei pixel opachi finisce per toccare il bordo dopo il ritaglio --
   la stessa firma di un crop di Stable Diffusion fallito. E' l'unico modo,
   senza un modello vero, di far scattare il ramo "il riquadro opaco tocca
   il bordo" di CellPassesQualityGate (vedi il gap di copertura in
   scripts/test-sprites.sh): --dry-run normale sintetizza sempre celle con
   margine di sfondo su tutti i lati, quindi non lo esercita mai. */
static void SynthesizeCellBadBorder(unsigned int seed, int cellIndex, unsigned char *src512)
{
    unsigned int rng = seed ^ ((unsigned int)cellIndex*0x9E3779B1u) ^ 0xC2B2AE35u;
    for (int i = 0; i < 4; i++) DryRngNext(&rng);

    unsigned char bg[3], body[3];
    DryPickColor(&rng, 170, 230, bg);
    DryPickColor(&rng, 30, 110, body);

    int cx = 8;                    /* deliberatamente vicino al bordo sinistro */
    int cy = SPRITE_CELL/2;

    unsigned char logical[SPRITE_CELL*SPRITE_CELL*3];
    for (int y = 0; y < SPRITE_CELL; y++)
    for (int x = 0; x < SPRITE_CELL; x++)
    {
        int dx = x - cx, dy = y - cy, d2 = dx*dx + dy*dy;
        unsigned char c[3];
        if (d2 > DRY_R_OUT*DRY_R_OUT) memcpy(c, bg, 3);
        else if (d2 >= DRY_R_IN*DRY_R_IN) { c[0] = c[1] = c[2] = 0; }
        else memcpy(c, body, 3);
        memcpy(logical + (size_t)(y*SPRITE_CELL + x)*3, c, 3);
    }

    const int f = SPRITE_DOWNSCALE_F;
    for (int y = 0; y < SPRITE_SRC; y++)
    for (int x = 0; x < SPRITE_SRC; x++)
    {
        int lx = x/f, ly = y/f;
        memcpy(src512 + ((size_t)y*SPRITE_SRC + (size_t)x)*3, logical + (size_t)(ly*SPRITE_CELL + lx)*3, 3);
    }
}

/* ---------------------------------------------------------------------
 * Gate di qualita' (vedi spec, sezione 4): una cella generata da SD viene
 * scartata se e' implausibile. Ogni soglia e' quella misurata/decisa nello
 * spike, non ridiscussa qui. */
static int CellPassesQualityGate(const SpritePostStats *stats, const unsigned char *cellRgba, const char **reason)
{
    double ratio = (double)stats->opaquePixels / (double)SPRITE_CELL_PIXELS;
    if (ratio < 0.05) { *reason = "pixel opachi < 5%"; return 0; }
    if (ratio > 0.70) { *reason = "pixel opachi > 70%"; return 0; }
    if (SpritesOpaqueTouchesBorder(cellRgba)) { *reason = "il riquadro opaco tocca il bordo (ritaglio fallito)"; return 0; }
    return 1;
}

/* Genera e post-processa UNA cella con Stable Diffusion, col gate di qualita'
 * e un solo tentativo di ritocco del seed (vedi spec). Se anche il secondo
 * tentativo fallisce il gate, cellBuf resta una cella trasparente: il gioco
 * disegna la sua forma geometrica di riserva per quell'entita', nessuna cella
 * rotta puo' rompere il gioco. Ritorna 0 se una generazione e' stata accettata,
 * 1 se la cella resta trasparente (non e' un errore fatale del processo). */
static int GenerateCellReal(SpriteSdCtx *sdCtx, const SpritesArgs *args, const SpriteManifest *manifest,
                            int cellIndex, unsigned char *cellBuf, unsigned char *src512, double *genSecsOut)
{
    *genSecsOut = 0;
    char *prompt = SpritesLoadCellPrompt(args->promptsDir, cellIndex, manifest->theme, manifest->style);
    char *neg = SpritesLoadNegativePrompt(args->promptsDir);
    if (!prompt)
    {
        SpritesLogLine("cella %s: prompt mancante in %s, resta trasparente",
                       SPRITE_CELL_NAMES[cellIndex], args->promptsDir);
        free(neg);
        memset(cellBuf, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
        return 1;
    }
    SpritesLogLine("cella %s: prompt=\"%s\"", SPRITE_CELL_NAMES[cellIndex], prompt);

    unsigned int seed0 = args->seed + (unsigned int)cellIndex*104729u;
    int ok = 0;
    for (int attempt = 0; attempt < 2 && !ok; attempt++)
    {
        unsigned int seed = attempt == 0 ? seed0 : (seed0 ^ 0x9E3779B9u) + 1u;
        double genSecs = 0;
        if (SpriteSdGenerate(sdCtx, prompt, neg ? neg : "", seed, src512, &genSecs) != 0)
        {
            *genSecsOut += genSecs;
            SpritesLogLine("cella %s: generazione fallita (tentativo %d, seed=%u)",
                           SPRITE_CELL_NAMES[cellIndex], attempt + 1, seed);
            continue;
        }
        *genSecsOut += genSecs;
        SpritePostStats stats;
        SpritesPostProcessCell(src512, cellBuf, SPRITE_PALETTE_COLORS, &stats);
        const char *reason = NULL;
        if (CellPassesQualityGate(&stats, cellBuf, &reason)) { ok = 1; break; }
        SpritesLogLine("cella %s: SCARTATA (tentativo %d, seed=%u): %s (opachi=%d/%d)",
                       SPRITE_CELL_NAMES[cellIndex], attempt + 1, seed, reason,
                       stats.opaquePixels, SPRITE_CELL_PIXELS);
    }
    free(prompt);
    free(neg);
    if (!ok)
    {
        memset(cellBuf, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
        SpritesLogLine("cella %s: nessun tentativo valido, resta trasparente (riserva geometrica in gioco)",
                       SPRITE_CELL_NAMES[cellIndex]);
    }
    return ok ? 0 : 1;
}

/* ---------------------------------------------------------------------
 * Generazione: con Stable Diffusion di default, SOLO --dry-run esplicito
 * ripiega sulle celle sintetiche. Se il modello manca, il LoRA e' assente
 * (sd.cpp lo gestisce da solo, vedi sprite_sd.c) non e' un caso da coprire
 * qui, o il caricamento fallisce, NON si scrive piu' un atlas placeholder:
 * si esce con codice diverso da zero senza toccare atlas ne' manifest (vedi
 * fix di revisione: prima il fallback silenzioso pubblicava dodici dischi
 * pastello quasi identici sopra l'atlas BMP procedurale gia' buono su
 * disco, mentre l'interfaccia continuava a dichiarare "Stable Diffusion").
 * Il gioco gestisce gia' bene un passo sprite che esce con errore (tiene
 * l'atlas BMP, vedi src/gen/gen_runner.c + docs/.../local-sprites-design.md
 * sezione 5): un errore esplicito e' quindi una via gia' pronta, non un
 * caso nuovo da inventare.
 */
static int RunGenerate(const SpritesArgs *args)
{
    if (SpritesEnsureDir(args->outDir) != 0)
    {
        fprintf(stderr, "melting-sprites: impossibile creare %s\n", args->outDir);
        return 3;
    }
    SpritesEnsureDir("logs");

    double tRunStart = SpritesNowSeconds();

    int useDryRun = args->dryRun;
    if (!useDryRun && !SpritesFileExists(args->model))
    {
        SpritesLogLine("modello assente (%s): nessun atlas scritto (passare --dry-run per le celle sintetiche)", args->model);
        fprintf(stderr, "melting-sprites: modello assente (%s), nessun atlas scritto\n", args->model);
        return 4;
    }

    SpriteManifest manifest = {0};
    if (!useDryRun)
    {
        SpritesLoadManifest(args->outDir, &manifest);
        if (!manifest.loaded)
            SpritesLogLine("manifest %s/current_run.txt assente o senza floor1.theme: uso un tema generico", args->outDir);
    }

    SpriteSdCtx *sdCtx = NULL;
    double loadSecs = 0;
    if (!useDryRun)
    {
        SpriteSdConfig sdCfg;
        sdCfg.modelPath = args->model;
        sdCfg.loraPath = args->lora;
        sdCfg.taesdPath = args->useTaesd ? "models/taesd.safetensors" : NULL;
        sdCfg.steps = args->steps;
        sdCfg.cfg = args->cfg;
        sdCfg.outDir = args->outDir;

        long long vramBefore = SpritesReadVramUsedBytes();
        sdCtx = SpriteSdLoad(&sdCfg, &loadSecs);
        if (!sdCtx)
        {
            SpritesLogLine("caricamento del modello fallito: nessun atlas scritto (passare --dry-run per le celle sintetiche)");
            fprintf(stderr, "melting-sprites: caricamento del modello fallito, nessun atlas scritto\n");
            SpritesProgressWrite(args->outDir, "errore", 100, "caricamento del modello fallito");
            return 4;
        }
        else
        {
            /* Preferisci la cifra che sd.cpp stesso logga durante il caricamento
               ("total params memory size ... VRAM ...MB"): e' esatta, non una
               differenza di campionamento. Il delta da /sys/class/drm resta un
               ripiego "a miglior sforzo" se quella riga non arriva. */
            double vramMB = SpriteSdVramMB(sdCtx);
            long long vramAfter = SpritesReadVramUsedBytes();
            if (vramMB > 0.0)
                SpritesLogLine("caricamento modello: %.1fs, VRAM %.0f MB (da sd.cpp)", loadSecs, vramMB);
            else if (vramBefore >= 0 && vramAfter >= 0)
                SpritesLogLine("caricamento modello: %.1fs, VRAM ~%.0f MB (da /sys/class/drm)",
                               loadSecs, (double)(vramAfter - vramBefore)/1048576.0);
            else
                SpritesLogLine("caricamento modello: %.1fs (VRAM non disponibile su questo sistema)", loadSecs);
        }
    }

    unsigned char *atlas = SpritesAtlasNew();
    unsigned char *src512 = malloc((size_t)SPRITE_SRC*SPRITE_SRC*3);
    unsigned char *cellBuf = malloc((size_t)SPRITE_CELL*SPRITE_CELL*4);
    if (!atlas || !src512 || !cellBuf)
    {
        fprintf(stderr, "melting-sprites: memoria esaurita\n");
        free(atlas); free(src512); free(cellBuf);
        if (sdCtx) SpriteSdFree(sdCtx);
        return 3;
    }

    double totalGenSecs = 0;
    int rejectedCells = 0;
    for (int i = 0; i < args->cells; i++)
    {
        int pct = 15 + (int)((95.0 - 15.0)*i/args->cells);
        char msg[96];
        snprintf(msg, sizeof(msg), "genero sprite %d/%d (%s)", i + 1, args->cells, SPRITE_CELL_NAMES[i]);
        SpritesProgressWrite(args->outDir, "genero", pct, msg);

        int isShotCell = strcmp(SPRITE_CELL_NAMES[i], "shot") == 0;

        if (useDryRun && args->dryRunBadBorder)
        {
            /* Solo per test (vedi scripts/test-sprites.sh): a differenza del
               --dry-run normale, qui la cella sintetica e' costruita apposta
               per fallire il controllo "il riquadro opaco tocca il bordo" e
               passa dallo stesso gate/ritentativo della generazione vera
               (CellPassesQualityGate + un secondo tentativo), cosi' quel
               ramo -- altrimenti mai esercitato da un --dry-run normale --
               ha copertura senza bisogno di un modello vero. */
            SpritePostStats stats = {0};
            int ok = 0;
            for (int attempt = 0; attempt < 2 && !ok; attempt++)
            {
                SynthesizeCellBadBorder(args->seed + (unsigned int)attempt, i, src512);
                SpritesPostProcessCell(src512, cellBuf, SPRITE_PALETTE_COLORS, &stats);
                const char *reason = NULL;
                if (CellPassesQualityGate(&stats, cellBuf, &reason)) { ok = 1; break; }
                SpritesLogLine("cella %s: SCARTATA (dry-run-bad-border, tentativo %d): %s (opachi=%d/%d)",
                               SPRITE_CELL_NAMES[i], attempt + 1, reason, stats.opaquePixels, SPRITE_CELL_PIXELS);
            }
            if (!ok)
            {
                memset(cellBuf, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
                rejectedCells++;
                SpritesLogLine("cella %s: nessun tentativo valido (dry-run-bad-border), resta trasparente",
                               SPRITE_CELL_NAMES[i]);
            }
            fprintf(stderr, "melting-sprites: cella %2d/%d (%-13s) dry-run-bad-border opachi=%5d%s\n",
                    i + 1, args->cells, SPRITE_CELL_NAMES[i], stats.opaquePixels, ok ? "" : " [TRASPARENTE]");
        }
        else if (useDryRun)
        {
            /* --dry-run resta invariato anche per la cella "shot": serve a
               testare il post-processing su tutte e 12 le celle (vedi
               scripts/test-sprites.sh), non a rispecchiare l'atlas finale
               di una run vera. */
            SynthesizeCell(args->seed, i, src512);
            SpritePostStats stats;
            SpritesPostProcessCell(src512, cellBuf, SPRITE_PALETTE_COLORS, &stats);
            fprintf(stderr, "melting-sprites: cella %2d/%d (%-13s) tagliati=%5d opachi=%5d a-rischio=%d\n",
                    i + 1, args->cells, SPRITE_CELL_NAMES[i], stats.cutPixels, stats.opaquePixels, stats.keyRiskPixels);
        }
        else if (isShotCell)
        {
            /* Il colpo (del giocatore e dei nemici) e' sempre disegnato come
               cerchio, mai come sprite: DrawAtlasCell non viene mai chiamato
               con SPR_SHOT (vedi il loop degli shot in
               src/render/game_renderer.c). Generarlo con Stable Diffusion
               costerebbe ~5.7s di GPU per ogni run senza che nessuno lo veda
               mai: si salta, la cella resta vuota (il layout 8x8 dell'atlas
               non cambia, la posizione resta riservata mai occupata). */
            memset(cellBuf, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
            SpritesLogLine("cella %s: saltata, mai disegnata in gioco (~5.7s di GPU risparmiati)", SPRITE_CELL_NAMES[i]);
            fprintf(stderr, "melting-sprites: cella %2d/%d (%-13s) saltata (mai disegnata, GPU risparmiata)\n",
                    i + 1, args->cells, SPRITE_CELL_NAMES[i]);
        }
        else
        {
            double cellSecs = 0;
            int rc = GenerateCellReal(sdCtx, args, &manifest, i, cellBuf, src512, &cellSecs);
            totalGenSecs += cellSecs;
            if (rc > 0) rejectedCells++;
            fprintf(stderr, "melting-sprites: cella %2d/%d (%-13s) %.1fs%s\n",
                    i + 1, args->cells, SPRITE_CELL_NAMES[i], cellSecs, rc > 0 ? " [TRASPARENTE]" : "");
        }
        SpritesComposeAtlas(atlas, i, cellBuf);
    }

    if (sdCtx) SpriteSdFree(sdCtx);

    SpritesProgressWrite(args->outDir, "scrivo", 96, "scrivo l'atlas");
    int rc = SpritesWriteAtlasPng(atlas, args->outDir);
    free(atlas); free(src512); free(cellBuf);
    if (rc != 0)
    {
        SpritesProgressWrite(args->outDir, "errore", 100, "scrittura PNG fallita");
        fprintf(stderr, "melting-sprites: scrittura PNG fallita\n");
        return 3;
    }

    SpritesUpdateManifestAtlasPath(args->outDir);
    SpritesProgressWrite(args->outDir, "fine", 100, "atlas pronto");

    double totalSecs = SpritesNowSeconds() - tRunStart;
    SpritesLogLine("run: modo=%s celle=%d scartate=%d caricamento=%.1fs generazione=%.1fs totale=%.1fs seed=%u",
                   useDryRun ? "dry-run" : "stable-diffusion", args->cells, rejectedCells,
                   loadSecs, totalGenSecs, totalSecs, args->seed);

    printf("melting-sprites: atlas scritto in %s/current_atlas.png (%d celle, seed=%u, modo=%s, %.1fs totali)\n",
           args->outDir, args->cells, args->seed, useDryRun ? "dry-run" : "stable-diffusion", totalSecs);
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

    return RunGenerate(&args);
}
