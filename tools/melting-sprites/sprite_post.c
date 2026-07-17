/* Post-processing degli sprite: downscale modale, ritaglio dello sfondo a
   flood fill, riduzione della palette con KEY_FLOOR. Porting diretto della
   pipeline provata su hardware reale (vedi docs/SPRITES-SPIKE.md, sezione
   "Le due cose imparate"): non ridisegnare l'algoritmo, e' stato misurato. */
#include "melting_sprites.h"

#include "exoquant.h"

#include <stdlib.h>
#include <string.h>

/* Distanza pesata fra due colori RGB: l'occhio umano e' piu' sensibile al
   verde, meno al blu. Stessa formula dello spike. */
static int ColorDist2(const unsigned char *a, const unsigned char *b)
{
    int dr = a[0] - b[0], dg = a[1] - b[1], db = a[2] - b[2];
    return 2*dr*dr + 4*dg*dg + 3*db*db;
}

void SpritesModalDownscale(const unsigned char *src, unsigned char *dst, int genSize)
{
    /* f e' oggi 4 (genSize=512) o 2 (genSize=256, --gen-size): il fattore
       era un #define fisso prima del preset --low-spec, ora e' derivato a
       runtime da genSize. key[] resta dimensionato al PIU' GRANDE f
       possibile (SPRITE_DOWNSCALE_F_MAX=4) e ne usa solo i primi f*f slot. */
    const int f = genSize / SPRITE_CELL;
    for (int cy = 0; cy < SPRITE_CELL; cy++)
    for (int cx = 0; cx < SPRITE_CELL; cx++)
    {
        /* Inizializzato a zero: con f runtime (non piu' una costante di
           compilazione come prima del preset --gen-size) gcc non riesce a
           provare da solo che i due loop qui sotto riempiono sempre tutti e
           soli i primi f*f slot usati piu' avanti, e segnala un falso
           positivo -Wmaybe-uninitialized. */
        int key[SPRITE_DOWNSCALE_F_MAX*SPRITE_DOWNSCALE_F_MAX] = {0};
        for (int y = 0; y < f; y++)
            for (int x = 0; x < f; x++)
            {
                const unsigned char *p = src + (((size_t)(cy*f + y))*(size_t)genSize + (size_t)(cx*f + x))*3;
                key[y*f + x] = ((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3);
            }
        /* Colore dominante del tassello f*f: meglio del nearest, che
           pescherebbe un pixel a caso (magari di bordo sfumato). */
        int best = key[0], bestc = 0;
        for (int i = 0; i < f*f; i++)
        {
            int c = 0;
            for (int j = 0; j < f*f; j++) if (key[j] == key[i]) c++;
            if (c > bestc) { bestc = c; best = key[i]; }
        }
        long r = 0, g = 0, b = 0, n = 0;
        for (int y = 0; y < f; y++)
            for (int x = 0; x < f; x++)
            {
                const unsigned char *p = src + (((size_t)(cy*f + y))*(size_t)genSize + (size_t)(cx*f + x))*3;
                if ((((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3)) == best)
                { r += p[0]; g += p[1]; b += p[2]; n++; }
            }
        unsigned char *d = dst + ((size_t)(cy*SPRITE_CELL + cx))*3;
        d[0] = (unsigned char)(r/n); d[1] = (unsigned char)(g/n); d[2] = (unsigned char)(b/n);
    }
}

int SpritesCutBackground(unsigned char *cellRgba, int tol, int tolHalo)
{
    /* Il colore di sfondo si prende dal bordo dell'immagine (qualunque
       esso sia: non si chiede mai al modello di nominarlo, vedi lo spike),
       per voto di maggioranza in bucket di 5 bit per canale. */
    unsigned char bg[3];
    {
        static int cnt[32768];
        memset(cnt, 0, sizeof cnt);
        int best = -1, bestc = 0;
        for (int i = 0; i < SPRITE_CELL; i++)
        {
            int idx[4] = { i, (SPRITE_CELL-1)*SPRITE_CELL + i, i*SPRITE_CELL, i*SPRITE_CELL + SPRITE_CELL-1 };
            for (int j = 0; j < 4; j++)
            {
                unsigned char *p = cellRgba + idx[j]*4;
                int k = ((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3);
                if (++cnt[k] > bestc) { bestc = cnt[k]; best = k; }
            }
        }
        bg[0] = (unsigned char)(((best >> 10) & 31) << 3);
        bg[1] = (unsigned char)(((best >> 5) & 31) << 3);
        bg[2] = (unsigned char)((best & 31) << 3);
    }

    /* Flood fill dai bordi: un pixel scuro DENTRO lo sprite non e'
       raggiungibile senza attraversare lo sprite, quindi sopravvive. Una
       soglia globale sulla luminosita' invece lo distruggerebbe. */
    unsigned char *mask = calloc((size_t)SPRITE_CELL*SPRITE_CELL, 1);
    int *stack = malloc(sizeof(int)*(size_t)SPRITE_CELL*SPRITE_CELL);
    if (!mask || !stack)
    {
        /* Contratto del tool: mai un crash. Senza questi due buffer non si
           puo' fare il flood fill: invece di proseguire su un puntatore
           NULL, si lascia la cella del tutto trasparente. Il chiamante
           (GenerateCellReal in main.c) tratta gia' una cella cosi' come uno
           scarto normale del gate di qualita': il gioco disegna la sua
           forma geometrica di riserva per quell'entita'. */
        free(mask); free(stack);
        memset(cellRgba, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
        return SPRITE_CELL*SPRITE_CELL;
    }
    int sp = 0, t2 = tol*tol;
    for (int i = 0; i < SPRITE_CELL; i++)
    {
        int idx[4] = { i, (SPRITE_CELL-1)*SPRITE_CELL + i, i*SPRITE_CELL, i*SPRITE_CELL + SPRITE_CELL-1 };
        for (int j = 0; j < 4; j++)
            if (!mask[idx[j]] && ColorDist2(cellRgba + idx[j]*4, bg) <= t2) { mask[idx[j]] = 1; stack[sp++] = idx[j]; }
    }
    while (sp > 0)
    {
        int p = stack[--sp], x = p % SPRITE_CELL, y = p / SPRITE_CELL;
        int nb[4] = { x > 0 ? p-1 : -1, x < SPRITE_CELL-1 ? p+1 : -1,
                      y > 0 ? p-SPRITE_CELL : -1, y < SPRITE_CELL-1 ? p+SPRITE_CELL : -1 };
        for (int j = 0; j < 4; j++)
        {
            int q = nb[j];
            if (q >= 0 && !mask[q] && ColorDist2(cellRgba + q*4, bg) <= t2) { mask[q] = 1; stack[sp++] = q; }
        }
    }

    /* Due passaggi di "halo" con tolleranza piu' larga: ripuliscono
       l'anti-aliasing fra sprite e sfondo SOLO sui pixel gia' adiacenti
       alla zona tagliata, quindi non possono attraversare un contorno di
       colore ben distinto dallo sfondo (es. nero puro). */
    int h2 = tolHalo*tolHalo;
    for (int pass = 0; pass < 2; pass++)
    {
        unsigned char *add = calloc((size_t)SPRITE_CELL*SPRITE_CELL, 1);
        if (!add)
        {
            free(mask); free(stack);
            memset(cellRgba, 0, (size_t)SPRITE_CELL*SPRITE_CELL*4);
            return SPRITE_CELL*SPRITE_CELL;
        }
        for (int p = 0; p < SPRITE_CELL*SPRITE_CELL; p++)
        {
            if (mask[p]) continue;
            int x = p % SPRITE_CELL, y = p / SPRITE_CELL, touch = 0;
            if (x > 0 && mask[p-1]) touch = 1;
            if (x < SPRITE_CELL-1 && mask[p+1]) touch = 1;
            if (y > 0 && mask[p-SPRITE_CELL]) touch = 1;
            if (y < SPRITE_CELL-1 && mask[p+SPRITE_CELL]) touch = 1;
            if (touch && ColorDist2(cellRgba + p*4, bg) <= h2) add[p] = 1;
        }
        for (int p = 0; p < SPRITE_CELL*SPRITE_CELL; p++) if (add[p]) mask[p] = 1;
        free(add);
    }

    int nbg = 0;
    for (int p = 0; p < SPRITE_CELL*SPRITE_CELL; p++)
        if (mask[p]) { cellRgba[p*4+0] = cellRgba[p*4+1] = cellRgba[p*4+2] = cellRgba[p*4+3] = 0; nbg++; }
    free(mask); free(stack);
    return nbg;
}

void SpritesQuantize(unsigned char *cellRgba, int ncolors)
{
    int n = SPRITE_CELL*SPRITE_CELL, m = 0;
    unsigned char *op = malloc((size_t)n*4);
    int *map = malloc(sizeof(int)*(size_t)n);
    if (!op || !map)
    {
        /* Come in SpritesCutBackground: senza questi buffer non si puo'
           quantizzare, e la cella potrebbe gia' avere un canale alpha
           parzialmente valido dal ritaglio. Meglio azzerarla del tutto
           (trasparente) che lasciarla in uno stato a meta' o andare in
           crash su un puntatore NULL. */
        free(op); free(map);
        memset(cellRgba, 0, (size_t)n*4);
        return;
    }
    for (int p = 0; p < n; p++)
    {
        if (cellRgba[p*4+3] == 0) { map[p] = -1; continue; }
        map[p] = m;
        memcpy(op + (size_t)m*4, cellRgba + (size_t)p*4, 4);
        op[m*4+3] = 255;
        m++;
    }
    if (m == 0) { free(op); free(map); return; }

    exq_data *e = exq_init();
    exq_no_transparency(e);
    exq_feed(e, op, m);
    exq_quantize_hq(e, ncolors);
    unsigned char pal[256*4];
    exq_get_palette(e, pal, ncolors);

    /* Alza i colori troppo scuri della palette: il gioco fa chroma-key sul
       quasi-nero, e mangerebbe i contorni neri dello sprite se restassero
       sotto soglia. Misurato: 0 pixel a rischio su tutti gli sprite provati. */
    for (int i = 0; i < ncolors; i++)
    {
        unsigned char *c = pal + i*4;
        int mx = c[0] > c[1] ? (c[0] > c[2] ? c[0] : c[2]) : (c[1] > c[2] ? c[1] : c[2]);
        if (mx < SPRITE_KEY_FLOOR)
        {
            int add = SPRITE_KEY_FLOOR - mx;
            for (int k = 0; k < 3; k++) { int v = c[k] + add; c[k] = (unsigned char)(v > 255 ? 255 : v); }
        }
    }

    unsigned char *idx = malloc((size_t)m);
    if (!idx)
    {
        exq_free(e); free(op); free(map);
        memset(cellRgba, 0, (size_t)n*4);
        return;
    }
    exq_map_image(e, m, op, idx);
    for (int p = 0; p < n; p++)
    {
        if (map[p] < 0) continue;
        unsigned char *c = pal + idx[map[p]]*4;
        cellRgba[p*4+0] = c[0]; cellRgba[p*4+1] = c[1]; cellRgba[p*4+2] = c[2]; cellRgba[p*4+3] = 255;
    }
    exq_free(e); free(op); free(map); free(idx);
}

void SpritesPostProcessCell(const unsigned char *src512Rgb, unsigned char *outCellRgba,
                             int ncolors, SpritePostStats *statsOut, int genSize)
{
    unsigned char small[SPRITE_CELL*SPRITE_CELL*3];
    SpritesModalDownscale(src512Rgb, small, genSize);

    for (int p = 0; p < SPRITE_CELL*SPRITE_CELL; p++)
    {
        outCellRgba[p*4+0] = small[p*3+0];
        outCellRgba[p*4+1] = small[p*3+1];
        outCellRgba[p*4+2] = small[p*3+2];
        outCellRgba[p*4+3] = 255;
    }

    int cut = SpritesCutBackground(outCellRgba, SPRITE_CUT_TOL, SPRITE_CUT_TOL_HALO);
    SpritesQuantize(outCellRgba, ncolors);

    int opaque = 0, keyRisk = 0;
    for (int p = 0; p < SPRITE_CELL*SPRITE_CELL; p++)
    {
        if (outCellRgba[p*4+3] == 0) continue;
        opaque++;
        unsigned char *c = outCellRgba + p*4;
        int mx = c[0] > c[1] ? (c[0] > c[2] ? c[0] : c[2]) : (c[1] > c[2] ? c[1] : c[2]);
        if (mx < SPRITE_KEY_FLOOR) keyRisk++;
    }
    if (statsOut) { statsOut->cutPixels = cut; statsOut->opaquePixels = opaque; statsOut->keyRiskPixels = keyRisk; }
}
