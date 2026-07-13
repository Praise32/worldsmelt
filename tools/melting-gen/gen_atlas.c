#include "melting_gen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATLAS_W 1024

typedef struct Rgb { int r, g, b; } Rgb;

static Rgb HexToRgb(const char *hex)
{
    Rgb c = { 255, 255, 255 };
    if (hex && hex[0] == '#' && strlen(hex) >= 7)
    {
        unsigned int v = (unsigned int)strtoul(hex + 1, NULL, 16);
        c.r = (int)((v >> 16) & 0xFFu);
        c.g = (int)((v >> 8) & 0xFFu);
        c.b = (int)(v & 0xFFu);
    }
    return c;
}

static int ClampByte(double v) { return v < 0 ? 0 : (v > 255 ? 255 : (int)lround(v)); }

static Rgb Shade(Rgb c, double factor)
{
    return (Rgb){ ClampByte(c.r*factor), ClampByte(c.g*factor), ClampByte(c.b*factor) };
}

static void PutPixel(unsigned char *px, int x, int y, Rgb c)
{
    if (x < 0 || y < 0 || x >= ATLAS_W || y >= ATLAS_W) return;
    size_t idx = ((size_t)y*ATLAS_W + (size_t)x)*4;
    px[idx + 0] = (unsigned char)c.b;   /* BMP: ordine BGRA */
    px[idx + 1] = (unsigned char)c.g;
    px[idx + 2] = (unsigned char)c.r;
    px[idx + 3] = 255;
}

static void FillRect(unsigned char *px, int x, int y, int w, int h, Rgb c)
{
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++) PutPixel(px, xx, yy, c);
}

static void FillCircle(unsigned char *px, int cx, int cy, int radius, Rgb c)
{
    int r2 = radius*radius;
    for (int y = cy - radius; y <= cy + radius; y++)
        for (int x = cx - radius; x <= cx + radius; x++)
        {
            int dx = x - cx, dy = y - cy;
            if (dx*dx + dy*dy <= r2) PutPixel(px, x, y, c);
        }
}

static void DrawLineT(unsigned char *px, int x0, int y0, int x1, int y1, int thickness, Rgb c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int steps = dx > dy ? dx : dy;
    if (steps < 1) steps = 1;
    for (int i = 0; i <= steps; i++)
    {
        double t = (double)i/(double)steps;
        FillCircle(px, (int)lround(x0 + (x1 - x0)*t), (int)lround(y0 + (y1 - y0)*t), thickness, c);
    }
}

static void FillDiamond(unsigned char *px, int cx, int cy, int radius, Rgb c)
{
    for (int y = -radius; y <= radius; y++)
    {
        int span = radius - (y < 0 ? -y : y);
        FillRect(px, cx - span, cy + y, span*2 + 1, 1, c);
    }
}

static void DrawCell(unsigned char *px, int cell, const GenRun *run)
{
    int x = (cell%8)*128;
    int y = (cell/8)*128;
    int cx = x + 64;
    int cy = y + 64;
    const GenFloor *floor = &run->floors[cell%GEN_FLOORS];
    Rgb accent = HexToRgb(floor->accent);
    Rgb accent2 = HexToRgb(floor->accent2);
    Rgb enemy = HexToRgb(floor->enemy);
    Rgb boss = HexToRgb(floor->bossColor);
    Rgb wall = HexToRgb(floor->wall);
    Rgb dark = { 7, 9, 12 };
    Rgb line = Shade(wall, 1.35);

    if (cell == 0)
    {
        FillCircle(px, cx, cy - 28, 14, accent2);
        FillRect(px, cx - 5, cy - 14, 10, 42, accent2);
        DrawLineT(px, cx - 25, cy - 2, cx + 25, cy - 2, 3, accent2);
        DrawLineT(px, cx - 5, cy + 27, cx - 20, cy + 54, 4, accent2);
        DrawLineT(px, cx + 5, cy + 27, cx + 20, cy + 54, 4, accent2);
        FillCircle(px, cx - 5, cy - 30, 3, dark);
        FillCircle(px, cx + 6, cy - 30, 3, dark);
    }
    else if (cell >= 1 && cell <= 4)
    {
        Rgb body = cell == 4 ? boss : enemy;
        int r = cell == 4 ? 42 : (cell == 3 ? 32 : 27);
        FillCircle(px, cx, cy, r, body);
        if (cell == 2)
        {
            FillRect(px, cx + 10, cy - 7, 38, 14, body);
            FillCircle(px, cx + 47, cy, 8, accent);
        }
        if (cell == 3)
        {
            FillRect(px, cx - 34, cy + 21, 68, 12, Shade(body, 0.7));
            FillRect(px, cx - 38, cy - 5, 12, 26, Shade(body, 0.8));
            FillRect(px, cx + 26, cy - 5, 12, 26, Shade(body, 0.8));
        }
        if (cell == 4)
        {
            for (int i = 0; i < 6; i++)
            {
                double a = 3.14159265358979*2.0*i/6.0;
                DrawLineT(px, cx, cy, cx + (int)lround(cos(a)*55), cy + (int)lround(sin(a)*55), 5, Shade(body, 0.8));
            }
            FillCircle(px, cx, cy, 17, accent);
        }
        FillCircle(px, cx - 12, cy - 8, 5, dark);
        FillCircle(px, cx + 12, cy - 8, 5, dark);
    }
    else if (cell == 5)
    {
        FillDiamond(px, cx, cy, 36, accent);
        FillDiamond(px, cx, cy, 21, accent2);
        FillCircle(px, cx, cy, 8, dark);
    }
    else if (cell == 6)
    {
        FillCircle(px, cx - 14, cy - 8, 19, boss);
        FillCircle(px, cx + 14, cy - 8, 19, boss);
        FillDiamond(px, cx, cy + 14, 31, boss);
        FillCircle(px, cx, cy + 1, 10, accent2);
    }
    else if (cell == 7)
    {
        FillCircle(px, cx, cy, 32, accent);
        FillCircle(px, cx, cy, 22, accent2);
        FillRect(px, cx - 5, cy - 23, 10, 46, accent);
    }
    else if (cell == 8)
    {
        FillCircle(px, cx, cy + 8, 32, Shade(wall, 1.55));
        DrawLineT(px, cx + 13, cy - 20, cx + 28, cy - 43, 3, accent);
        FillCircle(px, cx + 30, cy - 46, 5, boss);
    }
    else if (cell == 9)
    {
        FillCircle(px, cx - 23, cy, 17, accent2);
        FillCircle(px, cx - 23, cy, 8, dark);
        FillRect(px, cx - 7, cy - 4, 50, 8, accent2);
        FillRect(px, cx + 24, cy + 4, 8, 14, accent2);
        FillRect(px, cx + 38, cy + 4, 8, 22, accent2);
    }
    else if (cell == 10)
    {
        FillCircle(px, cx, cy, 43, accent);
        FillCircle(px, cx, cy, 32, dark);
        FillCircle(px, cx, cy, 24, accent2);
        FillCircle(px, cx, cy, 15, dark);
    }
    else if (cell == 11)
    {
        DrawLineT(px, cx - 34, cy + 8, cx + 29, cy - 9, 5, accent);
        FillCircle(px, cx + 35, cy - 11, 12, accent2);
        FillCircle(px, cx + 47, cy - 14, 5, boss);
    }
    else
    {
        int variant = cell%8;
        if (variant < 2)
        {
            FillRect(px, cx - 28, cy + 16, 56, 14, wall);
            DrawLineT(px, cx, cy + 16, cx, cy - 28, 4, accent2);
            FillCircle(px, cx - 14, cy - 15, 12, enemy);
            FillCircle(px, cx + 17, cy - 22, 13, accent);
        }
        else if (variant < 4)
        {
            FillDiamond(px, cx, cy, 33, line);
            FillDiamond(px, cx, cy, 23, wall);
            FillCircle(px, cx, cy, 7, accent2);
        }
        else if (variant < 6)
        {
            FillRect(px, cx - 31, cy - 22, 62, 44, wall);
            FillRect(px, cx - 22, cy - 13, 44, 26, Shade(wall, 1.45));
            DrawLineT(px, cx - 26, cy + 28, cx + 26, cy + 28, 3, accent);
        }
        else
        {
            FillCircle(px, cx - 17, cy + 12, 20, line);
            FillCircle(px, cx + 18, cy + 5, 27, wall);
            FillCircle(px, cx + 23, cy - 2, 7, accent2);
        }
    }
}

static void WriteU16(unsigned char *p, unsigned int v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
static void WriteU32(unsigned char *p, unsigned int v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

int GenWriteAtlasBmp(const GenRun *run, const char *outDir)
{
    size_t pixelBytes = (size_t)ATLAS_W*ATLAS_W*4;
    unsigned char *px = calloc(pixelBytes, 1);
    if (!px) return -1;
    for (int cell = 0; cell < 64; cell++) DrawCell(px, cell, run);

    unsigned char header[54] = { 0 };
    header[0] = 'B'; header[1] = 'M';
    WriteU32(header + 2, (unsigned int)(54 + pixelBytes));
    WriteU32(header + 10, 54);
    WriteU32(header + 14, 40);
    WriteU32(header + 18, ATLAS_W);
    WriteU32(header + 22, (unsigned int)(-ATLAS_W));   /* altezza negativa: righe dall'alto */
    WriteU16(header + 26, 1);
    WriteU16(header + 28, 32);
    WriteU32(header + 34, (unsigned int)pixelBytes);

    /* Come per il manifest (gen_manifest.c): file temporaneo + rename atomico,
       cosi' un SIGTERM o un disco pieno a meta' scrittura non lasciano un
       current_atlas.bmp troncato al posto di quello valido di prima. */
    char tmpPath[512], finalPath[512];
    snprintf(finalPath, sizeof(finalPath), "%s/current_atlas.bmp", outDir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_atlas.bmp.tmp", outDir);
    FILE *f = fopen(tmpPath, "wb");
    if (!f) { free(px); return -1; }
    int ok = fwrite(header, 1, 54, f) == 54 && fwrite(px, 1, pixelBytes, f) == pixelBytes;
    free(px);
    if (!ok)
    {
        fclose(f);
        remove(tmpPath);
        return -1;
    }
    return GenPublishFile(f, tmpPath, finalPath);
}
