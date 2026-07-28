#include "content/curated_images.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>

/* Cerca "<key>" a partire da 'cursor', salta i due punti e lo spazio, e copia
   la stringa fra virgolette che segue in 'out'. Ritorna il puntatore SUBITO
   dopo la virgoletta di chiusura (per la ricerca successiva), o NULL se la
   chiave non c'e' o il valore non e' una stringa chiusa.
   Nessun unescape: il manifest lo scrive scripts/curated-pack.py con id e
   percorsi ASCII senza virgolette ne' backslash interni (vedi il commento in
   curated_images.h). Un valore che li contenesse verrebbe troncato al primo
   apice, mai letto oltre il buffer. */
static const char *ReadJsonString(const char *cursor, const char *key, char *out, int outSize)
{
    char pattern[32];
    int written = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (written <= 0 || written >= (int)sizeof(pattern)) return NULL;

    const char *at = strstr(cursor, pattern);
    if (!at) return NULL;
    const char *p = at + written;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '"') return NULL;
    p++;
    const char *end = strchr(p, '"');
    if (!end) return NULL;

    int len = (int)(end - p);
    if (len >= outSize) len = outSize - 1;
    memcpy(out, p, (size_t)len);
    out[len] = '\0';
    return end + 1;
}

/* Legge la voce che comincia da 'cursor' (le tre chiavi in ordine fisso:
   id, file, category) e avanza il cursore. false = niente altre voci. */
static bool ReadEntry(const char **cursor, CuratedImage *out)
{
    const char *p = ReadJsonString(*cursor, "id", out->id, (int)sizeof(out->id));
    if (!p) return false;
    p = ReadJsonString(p, "file", out->file, (int)sizeof(out->file));
    if (!p) return false;
    p = ReadJsonString(p, "category", out->category, (int)sizeof(out->category));
    if (!p) return false;
    *cursor = p;
    return out->id[0] != '\0' && out->file[0] != '\0';
}

bool CuratedImageMaskGet(const unsigned char *mask, int maskBytes, int index)
{
    if (!mask || index < 0 || index/8 >= maskBytes) return true;   /* fuori range = "gia' usata": mai pescabile */
    return (mask[index/8] & (unsigned char)(1u << (index%8))) != 0;
}

void CuratedImageMaskSet(unsigned char *mask, int maskBytes, int index)
{
    if (!mask || index < 0 || index/8 >= maskBytes) return;
    mask[index/8] |= (unsigned char)(1u << (index%8));
}

int CuratedImagesCount(const char *manifestPath)
{
    const char *path = manifestPath ? manifestPath : CURATED_MANIFEST_PATH;
    if (!FileExists(path)) return 0;
    char *text = LoadFileText(path);
    if (!text) return 0;

    int count = 0;
    const char *cursor = text;
    CuratedImage entry;
    while (count < CURATED_IMAGE_MAX && ReadEntry(&cursor, &entry)) count++;
    UnloadFileText(text);
    return count;
}

bool CuratedImagesFindByIdInText(const char *manifestText, const char *id, CuratedImage *out, int *outIndex)
{
    if (outIndex) *outIndex = -1;
    if (!out || !id || !id[0] || !manifestText) return false;

    int count = 0;
    const char *cursor = manifestText;
    CuratedImage entry;
    while (count < CURATED_IMAGE_MAX && ReadEntry(&cursor, &entry))
    {
        if (strcmp(entry.id, id) == 0)
        {
            *out = entry;
            if (outIndex) *outIndex = count;
            return true;
        }
        count++;
    }
    return false;
}

bool CuratedImagesFindById(const char *manifestPath, const char *id, CuratedImage *out, int *outIndex)
{
    if (outIndex) *outIndex = -1;
    if (!out) return false;
    const char *path = manifestPath ? manifestPath : CURATED_MANIFEST_PATH;
    if (!FileExists(path)) return false;
    char *text = LoadFileText(path);
    if (!text) return false;

    bool found = CuratedImagesFindByIdInText(text, id, out, outIndex);
    UnloadFileText(text);
    return found;
}

/* Percorso del manifest per i test (vedi il commento in curated_images.h):
   stessa convenzione di g_testCatalogDir in curated_catalog.c e di
   g_testCatalogPath in run_catalog.c -- il puntatore resta di chi chiama,
   che lo rimette a NULL appena finito. */
static const char *g_testManifestPath = NULL;

void CuratedImagesSetTestManifestPath(const char *path)
{
    g_testManifestPath = path;
}

const char *CuratedImagesGetTestManifestPath(void)
{
    return g_testManifestPath;
}

/* Un solo passaggio non basta: per pescare la k-esima voce LIBERA bisogna
   prima sapere quante ce ne sono. Si legge il file una volta sola e lo si
   scorre due volte (il testo e' gia' in memoria), invece di aprirlo due
   volte. */
static bool PickFrom(const char *text, unsigned int roll, const char *category,
                     const unsigned char *usedMask, int maskBytes,
                     CuratedImage *out, int *outIndex)
{
    CuratedImage entry;
    int available = 0;
    int index = 0;
    const char *cursor = text;
    while (index < CURATED_IMAGE_MAX && ReadEntry(&cursor, &entry))
    {
        if (!CuratedImageMaskGet(usedMask, maskBytes, index) &&
            (!category || strcmp(entry.category, category) == 0)) available++;
        index++;
    }
    if (available <= 0) return false;

    int wanted = (int)(roll % (unsigned int)available);
    int seen = 0;
    index = 0;
    cursor = text;
    while (index < CURATED_IMAGE_MAX && ReadEntry(&cursor, &entry))
    {
        if (!CuratedImageMaskGet(usedMask, maskBytes, index) &&
            (!category || strcmp(entry.category, category) == 0))
        {
            if (seen == wanted)
            {
                *out = entry;
                if (outIndex) *outIndex = index;
                return true;
            }
            seen++;
        }
        index++;
    }
    return false;   /* irraggiungibile: 'wanted' e' sempre < available */
}

bool CuratedImagesPickUnused(const char *manifestPath, unsigned int roll, const char *category,
                             const unsigned char *usedMask, int maskBytes,
                             CuratedImage *out, int *outIndex)
{
    if (!out) return false;
    const char *path = manifestPath ? manifestPath : CURATED_MANIFEST_PATH;
    if (!FileExists(path)) return false;
    char *text = LoadFileText(path);
    if (!text) return false;

    bool ok = PickFrom(text, roll, category, usedMask, maskBytes, out, outIndex);
    /* La categoria e' una PREFERENZA, non un requisito: quando quella
       famiglia e' esaurita si pesca comunque dal resto del pacchetto --
       "fra le immagini non ancora usate nella run corrente" (DEC-171) non
       parla di categorie. */
    if (!ok && category) ok = PickFrom(text, roll, NULL, usedMask, maskBytes, out, outIndex);
    UnloadFileText(text);
    return ok;
}
