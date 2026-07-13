/* Lettura di generated/current_run.txt (tema/stile del piano 1) e
   aggiornamento del campo atlas.path dopo aver scritto l'atlas. Stesso
   formato "chiave=valore" per riga scritto da tools/melting-gen/gen_manifest.c
   e letto da src/content/run_content.c: qui la lettura e' reimplementata
   apposta (vedi il commento in cima a sprite_util.c sul perche' i due tool
   non condividono codice). */
#include "melting_sprites.h"

#include <stdlib.h>
#include <string.h>

static void ReadManifestValue(const char *text, const char *key, char *out, size_t outSize)
{
    out[0] = '\0';
    if (!text || !key || outSize == 0) return;
    const char *start = strstr(text, key);
    if (!start) return;
    start += strlen(key);
    size_t i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
}

void SpritesLoadManifest(const char *outDir, SpriteManifest *out)
{
    out->theme[0] = '\0';
    out->style[0] = '\0';
    out->loaded = 0;

    char path[512];
    snprintf(path, sizeof(path), "%s/current_run.txt", outDir);
    char *text = SpritesReadFile(path);
    if (!text) return;

    ReadManifestValue(text, "floor1.theme=", out->theme, sizeof(out->theme));
    ReadManifestValue(text, "floor1.style=", out->style, sizeof(out->style));
    out->loaded = out->theme[0] != '\0';
    free(text);
}

void SpritesUpdateManifestAtlasPath(const char *outDir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/current_run.txt", outDir);
    char *text = SpritesReadFile(path);
    if (!text)
    {
        SpritesLogLine("manifest: %s assente, atlas.path non aggiornato", path);
        return;
    }

    char tmpPath[512];
    snprintf(tmpPath, sizeof(tmpPath), "%s/current_run.txt.tmp", outDir);
    FILE *f = fopen(tmpPath, "w");
    if (!f)
    {
        SpritesLogLine("manifest: impossibile aprire %s in scrittura", tmpPath);
        free(text);
        return;
    }

    /* Percorso fisso, non derivato da outDir: stesso schema di WriteManifest in
       tools/melting-gen/gen_manifest.c (il gioco legge sempre e solo
       "generated/current_run.txt", non un --out qualunque). */
    static const char ATLAS_LINE[] = "atlas.path=generated/current_atlas.png\n";
    int found = 0;
    const char *p = text;
    while (*p)
    {
        const char *nl = strchr(p, '\n');
        size_t lineLen = nl ? (size_t)(nl - p) : strlen(p);
        if (lineLen >= 11 && strncmp(p, "atlas.path=", 11) == 0)
        {
            fputs(ATLAS_LINE, f);
            found = 1;
        }
        else
        {
            fwrite(p, 1, lineLen, f);
            fputc('\n', f);
        }
        p += lineLen;
        if (nl) p++; else break;
    }
    if (!found) fputs(ATLAS_LINE, f);
    free(text);

    if (SpritesPublishFile(f, tmpPath, path) != 0)
        SpritesLogLine("manifest: scrittura di %s fallita, resta la versione precedente", path);
}
