/* Utilita' condivise: file, progresso, log, VRAM. Duplica deliberatamente
   gen_util.c di tools/melting-gen invece di condividerlo: i due tool
   vendorizzano due ggml incompatibili e devono restare eseguibili separati
   (vedi il commento sul Makefile e in cima a sprite_sd.c). */
#include "melting_sprites.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int SpritesEnsureDir(const char *path)
{
    if (mkdir(path, 0755) == 0) return 0;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

char *SpritesReadFile(const char *path)
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

int SpritesFileExists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

double SpritesNowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}

void SpritesProgressWrite(const char *outDir, const char *phase, int percent, const char *message)
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

int SpritesPublishFile(FILE *f, const char *tmpPath, const char *finalPath)
{
    /* ferror() va controllato PRIMA di fclose(): dopo la chiusura lo stream
       non e' piu' valido (stesso ragionamento di GenPublishFile in
       tools/melting-gen/gen_util.c). */
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

void SpritesLogLine(const char *fmt, ...)
{
    SpritesEnsureDir("logs");
    FILE *f = fopen("logs/melting-sprites.log", "a");
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

long long SpritesReadVramUsedBytes(void)
{
    char path[96];
    for (int card = 0; card < 8; card++)
    {
        snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/mem_info_vram_used", card);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        long long bytes = -1;
        int got = fscanf(f, "%lld", &bytes);
        fclose(f);
        if (got == 1) return bytes;
    }
    return -1;
}
