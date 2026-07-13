/* Composizione dell'atlas 1024x1024 RGBA (8x8 celle da 128) e scrittura PNG. */
#include "melting_sprites.h"

#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *SpritesAtlasNew(void)
{
    return calloc((size_t)SPRITE_ATLAS_W*SPRITE_ATLAS_W*4, 1);
}

void SpritesCenterInCell(unsigned char *cellRgba)
{
    int minX = SPRITE_CELL, minY = SPRITE_CELL, maxX = -1, maxY = -1;
    for (int y = 0; y < SPRITE_CELL; y++)
        for (int x = 0; x < SPRITE_CELL; x++)
        {
            if (cellRgba[(y*SPRITE_CELL + x)*4 + 3] == 0) continue;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
        }
    if (maxX < 0) return;   /* cella vuota: niente da centrare */

    int dx = SPRITE_CELL/2 - (minX + maxX)/2;
    int dy = SPRITE_CELL/2 - (minY + maxY)/2;
    if (dx == 0 && dy == 0) return;

    unsigned char *tmp = calloc((size_t)SPRITE_CELL*SPRITE_CELL*4, 1);
    if (!tmp) return;
    for (int y = 0; y < SPRITE_CELL; y++)
        for (int x = 0; x < SPRITE_CELL; x++)
        {
            if (cellRgba[(y*SPRITE_CELL + x)*4 + 3] == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= SPRITE_CELL || ny < 0 || ny >= SPRITE_CELL) continue;
            memcpy(tmp + (size_t)(ny*SPRITE_CELL + nx)*4, cellRgba + (size_t)(y*SPRITE_CELL + x)*4, 4);
        }
    memcpy(cellRgba, tmp, (size_t)SPRITE_CELL*SPRITE_CELL*4);
    free(tmp);
}

void SpritesComposeAtlas(unsigned char *atlas, int cellIndex, unsigned char *cellRgba)
{
    SpritesCenterInCell(cellRgba);
    int col = cellIndex % SPRITE_ATLAS_COLS;
    int row = cellIndex / SPRITE_ATLAS_COLS;
    int ox = col*SPRITE_CELL, oy = row*SPRITE_CELL;
    for (int y = 0; y < SPRITE_CELL; y++)
        memcpy(atlas + ((size_t)(oy + y)*SPRITE_ATLAS_W + (size_t)ox)*4,
               cellRgba + (size_t)y*SPRITE_CELL*4, (size_t)SPRITE_CELL*4);
}

int SpritesWriteAtlasPng(const unsigned char *atlas, const char *outDir)
{
    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/current_atlas.png", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_atlas.png.tmp", outDir);
    if (!stbi_write_png(tmpPath, SPRITE_ATLAS_W, SPRITE_ATLAS_W, 4, atlas, SPRITE_ATLAS_W*4))
        return -1;
    if (rename(tmpPath, finalPath) != 0) { remove(tmpPath); return -1; }
    return 0;
}
