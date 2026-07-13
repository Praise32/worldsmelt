#ifndef MELTING_SPRITES_H
#define MELTING_SPRITES_H

#include <stddef.h>

/* Dimensioni fisse della pipeline, misurate nello spike (docs/SPRITES-SPIKE.md):
   Stable Diffusion genera a 512x512, il gioco usa celle da 128x128 in un
   atlas 1024x1024 a 8 colonne (stesso layout di AtlasSprite in
   src/core/game_types.h: cella i alla colonna i%8, riga i/8). */
#define SPRITE_SRC 512
#define SPRITE_CELL 128
#define SPRITE_DOWNSCALE_F (SPRITE_SRC / SPRITE_CELL)
#define SPRITE_ATLAS_COLS 8
#define SPRITE_ATLAS_W (SPRITE_ATLAS_COLS * SPRITE_CELL)
/* Le celle note dell'atlas (vedi AtlasSprite in src/core/game_types.h):
   player, 3 nemici, boss, oggetto, cuore, moneta, bomba, chiave, portale, colpo. */
#define SPRITE_CELLS 12

/* Colori nella palette finale di ogni sprite (riduzione con exoquant). */
#define SPRITE_PALETTE_COLORS 16

/* Il gioco fa chroma-key sul quasi-nero (src/assets/game_assets.c): nessun
   pixel opaco dello sprite puo' avere max(r,g,b) sotto questa soglia, o il
   caricamento lo tratterebbe come sfondo trasparente. Vedi "Le due cose
   imparate" in docs/SPRITES-SPIKE.md. */
#define SPRITE_KEY_FLOOR 16

/* Tolleranze del ritaglio a flood fill: la prima per il flood fill vero e
   proprio dai bordi, la seconda (piu' larga) per i due passaggi di "halo"
   che ripuliscono l'anti-aliasing fra sprite e sfondo. Valori dello spike. */
#define SPRITE_CUT_TOL 40
#define SPRITE_CUT_TOL_HALO 80

typedef struct SpritePostStats {
    int cutPixels;      /* pixel di sfondo ritagliati dal flood fill */
    int opaquePixels;   /* pixel opachi rimasti nello sprite finale */
    int keyRiskPixels;  /* pixel opachi con max(r,g,b) < SPRITE_KEY_FLOOR (deve essere 0) */
} SpritePostStats;

/* sprite_post.c: downscale modale, ritaglio dello sfondo, riduzione palette.
   Porting diretto della pipeline provata su hardware reale (vedi
   docs/SPRITES-SPIKE.md). Operano su un buffer di cella SPRITE_CELL x
   SPRITE_CELL, RGBA a meno che sia specificato altrimenti. */

/* src: SPRITE_SRC*SPRITE_SRC*3 (RGB). dst: SPRITE_CELL*SPRITE_CELL*3 (RGB). */
void SpritesModalDownscale(const unsigned char *src, unsigned char *dst);

/* cellRgba: SPRITE_CELL*SPRITE_CELL*4, alpha gia' impostato a 255 in ingresso.
   Ritaglia lo sfondo (colore preso dal bordo dell'immagine) con un flood
   fill dai bordi: un pixel scuro dentro lo sprite non e' raggiungibile senza
   attraversare lo sprite, quindi sopravvive. Ritorna il numero di pixel
   trasformati in trasparenti. */
int SpritesCutBackground(unsigned char *cellRgba, int tol, int tolHalo);

/* Riduce i pixel opachi di cellRgba a ncolors colori (exoquant) e applica
   SPRITE_KEY_FLOOR ai colori troppo scuri della palette risultante. */
void SpritesQuantize(unsigned char *cellRgba, int ncolors);

/* Orchestrazione dei tre passi sopra: da sorgente 512x512 RGB a cella
   128x128 RGBA pronta per l'atlas. outCellRgba deve avere spazio per
   SPRITE_CELL*SPRITE_CELL*4 byte. statsOut puo' essere NULL. */
void SpritesPostProcessCell(const unsigned char *src512Rgb, unsigned char *outCellRgba,
                             int ncolors, SpritePostStats *statsOut);

/* sprite_atlas.c: composizione dell'atlas 1024x1024 RGBA e scrittura PNG. */

/* Alloca e azzera (trasparente) un atlas SPRITE_ATLAS_W x SPRITE_ATLAS_W
   RGBA. Il chiamante libera con free(). NULL su memoria esaurita. */
unsigned char *SpritesAtlasNew(void);

/* Ricentra lo sprite dentro la sua cella usando il riquadro dei pixel
   opachi: dopo il ritaglio lo sprite potrebbe non essere al centro esatto
   della cella. Cella vuota (nessun pixel opaco): non fa nulla. */
void SpritesCenterInCell(unsigned char *cellRgba);

/* Ricentra (vedi sopra) e copia cellRgba (SPRITE_CELL*SPRITE_CELL*4) nella
   cella cellIndex dell'atlas (colonna cellIndex%SPRITE_ATLAS_COLS, riga
   cellIndex/SPRITE_ATLAS_COLS). atlas deve avere spazio per
   SPRITE_ATLAS_W*SPRITE_ATLAS_W*4 byte. */
void SpritesComposeAtlas(unsigned char *atlas, int cellIndex, unsigned char *cellRgba);

/* Scrive atlas come "<outDir>/current_atlas.png" (file temporaneo + rename
   atomico, come i file scritti da melting-gen). 0 se ok, -1 su errore. */
int SpritesWriteAtlasPng(const unsigned char *atlas, const char *outDir);

#endif
