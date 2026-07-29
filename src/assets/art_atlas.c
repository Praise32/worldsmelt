#include "assets/art_atlas.h"

#include <stdio.h>
#include <string.h>

/* ============================================================
   Scanner sequenziale del manifest (vedi il perche' in art_atlas.h).
 *
 * Non e' un parser JSON generale e non deve diventarlo: riconosce ESATTAMENTE
 * la forma che la pipeline scrive -- un oggetto radice le cui chiavi sono
 * numeri, array di numeri o oggetti di secondo livello -- e salta in silenzio
 * tutto cio' che non riconosce. Le tre garanzie che rendono lecita questa
 * scelta (documentate in 08-PIPELINE-SPRITE-ANIMAZIONI.md): solo ASCII,
 * nessun escape, profondita' massima due.
 * Il cursore avanza SEMPRE (nessun ramo che lo lascia fermo): un file
 * troncato o con graffe spaiate termina la scansione, non la fa girare a
 * vuoto.
   ============================================================ */

static const char *SkipBlanks(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
    return p;
}

/* Legge la stringa fra virgolette che comincia in '*cursor' (dopo gli spazi) e
   la copia in 'out'. Ritorna false se li' non c'e' una stringa chiusa: in quel
   caso il cursore resta dove era e chi chiama termina la scansione. */
static bool ReadString(const char **cursor, char *out, int outSize)
{
    const char *p = SkipBlanks(*cursor);
    if (*p != '"') return false;
    p++;
    const char *end = strchr(p, '"');
    if (!end) return false;
    int len = (int)(end - p);
    if (len >= outSize) len = outSize - 1;
    if (out)
    {
        memcpy(out, p, (size_t)len);
        out[len] = '\0';
    }
    *cursor = end + 1;
    return true;
}

/* Consuma i due punti che separano chiave e valore. */
static bool ReadColon(const char **cursor)
{
    const char *p = SkipBlanks(*cursor);
    if (*p != ':') return false;
    *cursor = p + 1;
    return true;
}

/* Legge un intero (anche negativo) o un booleano. I manifest non contengono
   frazionari: gli fps sono interi per contratto, e leggerli come tali evita di
   dover portare in giro un float per un valore che poi si userebbe come passo
   discreto. Un eventuale "8.5" viene letto come 8 -- piu' lento, mai piu'
   veloce del dichiarato: il fallimento cade dalla parte innocua. */
static bool ReadNumberOrBool(const char **cursor, int *out)
{
    const char *p = SkipBlanks(*cursor);
    if (strncmp(p, "true", 4) == 0) { *out = 1; *cursor = p + 4; return true; }
    if (strncmp(p, "false", 5) == 0) { *out = 0; *cursor = p + 5; return true; }

    bool negative = false;
    if (*p == '-') { negative = true; p++; }
    if (*p < '0' || *p > '9') return false;
    int value = 0;
    while (*p >= '0' && *p <= '9')
    {
        if (value < 100000) value = value*10 + (*p - '0');   /* tetto: un manifest non dichiara mai numeri grandi, e senza tetto un file spazzatura potrebbe far straripare l'int */
        p++;
    }
    if (*p == '.') { p++; while (*p >= '0' && *p <= '9') p++; }   /* parte frazionaria letta e scartata, vedi sopra */
    *out = negative ? -value : value;
    *cursor = p;
    return true;
}

/* Legge "[a, b, ...]" in 'out' (fino a 'maxOut' valori) e ritorna quanti ne ha
   letti, 0 se li' non c'e' un array. */
static int ReadIntArray(const char **cursor, int *out, int maxOut)
{
    const char *p = SkipBlanks(*cursor);
    if (*p != '[') return 0;
    p++;
    int count = 0;
    while (1)
    {
        p = SkipBlanks(p);
        if (*p == ']') { p++; break; }
        if (*p == '\0') return 0;   /* array non chiuso: il manifest e' troncato */
        int value = 0;
        const char *before = p;
        if (!ReadNumberOrBool(&p, &value)) return 0;
        if (p == before) return 0;
        if (count < maxOut) out[count] = value;
        count++;
    }
    *cursor = p;
    return count;
}

/* Salta il valore che comincia in '*cursor', qualunque sia (numero, stringa,
   array, oggetto): e' la via d'uscita per ogni chiave che questo modulo non
   conosce, e quindi la ragione per cui un manifest ESTESO domani continua a
   caricarsi oggi. Conta le parentesi per saltare un valore composto intero. */
static void SkipValue(const char **cursor)
{
    const char *p = SkipBlanks(*cursor);
    if (*p == '"') { ReadString(&p, NULL, 0); *cursor = p; return; }
    if (*p == '[' || *p == '{')
    {
        int depth = 0;
        while (*p)
        {
            if (*p == '[' || *p == '{') depth++;
            else if (*p == ']' || *p == '}') { depth--; if (depth == 0) { p++; break; } }
            else if (*p == '"') { const char *s = p; if (!ReadString(&s, NULL, 0)) { p++; continue; } p = s; continue; }
            p++;
        }
        *cursor = p;
        return;
    }
    int ignored = 0;
    if (!ReadNumberOrBool(&p, &ignored)) p++;   /* carattere inatteso: avanza di uno, mai fermo */
    *cursor = p;
}

/* Entra nell'oggetto che comincia in '*cursor'. false se li' non c'e' una
   graffa aperta. */
static bool EnterObject(const char **cursor)
{
    const char *p = SkipBlanks(*cursor);
    if (*p != '{') return false;
    *cursor = p + 1;
    return true;
}

/* Vero (consumando la graffa) se l'oggetto corrente e' finito. */
static bool LeaveObjectIfClosed(const char **cursor)
{
    const char *p = SkipBlanks(*cursor);
    if (*p == '}') { *cursor = p + 1; return true; }
    return *p == '\0';
}

/* Un'animazione: l'oggetto {"row":..,"frames":..,"fps":..,"loop":..}. Le quattro
   chiavi si leggono per NOME e non per posizione (il contratto le elenca in
   ordine, ma legarsi all'ordine renderebbe illeggibile un manifest riscritto da
   un tool che riordina le chiavi). */
static void ParseAnimBody(const char **cursor, ArtAnim *anim)
{
    if (!EnterObject(cursor)) { SkipValue(cursor); return; }
    while (!LeaveObjectIfClosed(cursor))
    {
        char key[24];
        if (!ReadString(cursor, key, (int)sizeof(key))) { SkipValue(cursor); continue; }
        if (!ReadColon(cursor)) { SkipValue(cursor); continue; }
        int value = 0;
        if (strcmp(key, "row") == 0 && ReadNumberOrBool(cursor, &value)) anim->row = value;
        else if (strcmp(key, "frames") == 0 && ReadNumberOrBool(cursor, &value)) anim->frames = value;
        else if (strcmp(key, "fps") == 0 && ReadNumberOrBool(cursor, &value)) anim->fps = value;
        else if (strcmp(key, "loop") == 0 && ReadNumberOrBool(cursor, &value)) anim->loop = (value != 0);
        else SkipValue(cursor);
    }
}

static void ParseAnims(const char **cursor, ArtSheet *out)
{
    if (!EnterObject(cursor)) { SkipValue(cursor); return; }
    while (!LeaveObjectIfClosed(cursor))
    {
        char name[ART_ANIM_NAME_LEN];
        if (!ReadString(cursor, name, (int)sizeof(name))) { SkipValue(cursor); continue; }
        if (!ReadColon(cursor)) { SkipValue(cursor); continue; }
        if (out->animCount >= ART_ANIM_MAX) { SkipValue(cursor); continue; }
        ArtAnim anim = { { 0 }, 0, 0, 0, false };
        snprintf(anim.name, sizeof(anim.name), "%s", name);
        ParseAnimBody(cursor, &anim);
        /* Un'animazione senza fotogrammi non e' un'animazione: si scarta qui
           invece di lasciarla in tabella, cosi' ArtSheetAnim non torna mai
           qualcosa che ArtAnimFrameAt dovrebbe poi difendere. */
        if (anim.frames > 0) out->anims[out->animCount++] = anim;
    }
}

static void ParseTiles(const char **cursor, ArtSheet *out)
{
    if (!EnterObject(cursor)) { SkipValue(cursor); return; }
    while (!LeaveObjectIfClosed(cursor))
    {
        char name[ART_ROLE_NAME_LEN];
        if (!ReadString(cursor, name, (int)sizeof(name))) { SkipValue(cursor); continue; }
        if (!ReadColon(cursor)) { SkipValue(cursor); continue; }
        int cell[2] = { 0, 0 };
        if (ReadIntArray(cursor, cell, 2) < 2) { SkipValue(cursor); continue; }
        if (out->roleCount >= ART_ROLE_MAX) continue;
        ArtTileRole role = { { 0 }, cell[0], cell[1] };
        snprintf(role.name, sizeof(role.name), "%s", name);
        out->roles[out->roleCount++] = role;
    }
}

static void ParseGlyphs(const char **cursor, ArtSheet *out)
{
    if (!EnterObject(cursor)) { SkipValue(cursor); return; }
    while (!LeaveObjectIfClosed(cursor))
    {
        char name[8];
        if (!ReadString(cursor, name, (int)sizeof(name))) { SkipValue(cursor); continue; }
        if (!ReadColon(cursor)) { SkipValue(cursor); continue; }
        int x = 0, w = 0;
        bool haveX = false, haveW = false;
        if (!EnterObject(cursor)) { SkipValue(cursor); continue; }
        while (!LeaveObjectIfClosed(cursor))
        {
            char key[8];
            if (!ReadString(cursor, key, (int)sizeof(key))) { SkipValue(cursor); continue; }
            if (!ReadColon(cursor)) { SkipValue(cursor); continue; }
            int value = 0;
            if (strcmp(key, "x") == 0 && ReadNumberOrBool(cursor, &value)) { x = value; haveX = true; }
            else if (strcmp(key, "w") == 0 && ReadNumberOrBool(cursor, &value)) { w = value; haveW = true; }
            else SkipValue(cursor);
        }
        /* Un glifo e' un CARATTERE: una chiave piu' lunga di uno (o vuota) non
           puo' essere un glifo di questo font, e ignorarla e' meglio che
           mapparne il primo carattere su una casella sbagliata. */
        if (name[0] == '\0' || name[1] != '\0' || !haveX || !haveW || w <= 0) continue;
        if (out->glyphCount >= ART_GLYPH_MAX) continue;
        ArtGlyph glyph = { name[0], (short)x, (short)w };
        out->glyphs[out->glyphCount++] = glyph;
    }
}

bool ArtAtlasParseManifest(const char *text, ArtSheet *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    if (!text) return false;

    const char *cursor = text;
    if (!EnterObject(&cursor)) return false;

    while (!LeaveObjectIfClosed(&cursor))
    {
        char key[24];
        if (!ReadString(&cursor, key, (int)sizeof(key))) { SkipValue(&cursor); continue; }
        if (!ReadColon(&cursor)) { SkipValue(&cursor); continue; }

        int value = 0;
        int pair[4] = { 0, 0, 0, 0 };
        if (strcmp(key, "frame_w") == 0 && ReadNumberOrBool(&cursor, &value)) out->frameW = value;
        else if (strcmp(key, "frame_h") == 0 && ReadNumberOrBool(&cursor, &value)) out->frameH = value;
        else if (strcmp(key, "tile_w") == 0 && ReadNumberOrBool(&cursor, &value)) out->tileW = value;
        else if (strcmp(key, "tile_h") == 0 && ReadNumberOrBool(&cursor, &value)) out->tileH = value;
        else if (strcmp(key, "glyph_h") == 0 && ReadNumberOrBool(&cursor, &value)) out->glyphH = value;
        else if (strcmp(key, "baseline_y") == 0 && ReadNumberOrBool(&cursor, &value)) out->baselineY = value;
        else if (strcmp(key, "space_w") == 0 && ReadNumberOrBool(&cursor, &value)) out->spaceW = value;
        else if (strcmp(key, "letter_spacing") == 0 && ReadNumberOrBool(&cursor, &value)) out->letterSpacing = value;
        else if (strcmp(key, "anchor") == 0)
        {
            if (ReadIntArray(&cursor, pair, 2) >= 2) { out->anchorX = pair[0]; out->anchorY = pair[1]; }
            else SkipValue(&cursor);
        }
        else if (strcmp(key, "grid") == 0)
        {
            if (ReadIntArray(&cursor, pair, 2) >= 2) { out->gridCols = pair[0]; out->gridRows = pair[1]; }
            else SkipValue(&cursor);
        }
        else if (strcmp(key, "slice") == 0)
        {
            if (ReadIntArray(&cursor, pair, 4) >= 4)
            {
                out->sliceL = pair[0]; out->sliceT = pair[1]; out->sliceR = pair[2]; out->sliceB = pair[3];
            }
            else SkipValue(&cursor);
        }
        else if (strcmp(key, "anims") == 0) ParseAnims(&cursor, out);
        else if (strcmp(key, "tiles") == 0) ParseTiles(&cursor, out);
        else if (strcmp(key, "glyphs") == 0) ParseGlyphs(&cursor, out);
        else SkipValue(&cursor);
    }

    bool sheetOk = out->frameW > 0 && out->frameH > 0 && out->animCount > 0;
    bool tilesetOk = out->tileW > 0 && out->tileH > 0 && out->roleCount > 0;
    bool fontOk = out->glyphH > 0 && out->glyphCount > 0;
    out->manifestOk = sheetOk || tilesetOk || fontOk;
    return out->manifestOk;
}

/* ============================================================
   Registro delle texture.
 *
 * STATICO al modulo e non dentro Game (a differenza di Game.atlas/
 * curatedTextures): gli asset di assets/art/ non sono contenuto di RUN. Game
 * viene azzerato e ricostruito a ogni GameResetRun (piu' volte per partita:
 * ingresso nel Piano 0, nuova run, rigenerazione con R), e appenderci un
 * registro di 73 spritesheet avrebbe significato ricaricare da disco l'intero
 * pacchetto artistico ad ogni azzeramento -- per asset che non cambiano mai.
   ============================================================ */

typedef struct ArtRegistry {
    ArtSheet sheets[ART_SHEET_MAX];
    int count;
} ArtRegistry;

static ArtRegistry g_registry;
static const char *g_testDir = NULL;

void ArtAtlasSetTestDir(const char *dir)
{
    ArtAtlasShutdown();
    g_testDir = dir;
}

static const char *ArtDir(void)
{
    return g_testDir ? g_testDir : ART_ATLAS_DIR;
}

void ArtAtlasShutdown(void)
{
    for (int i = 0; i < g_registry.count && i < ART_SHEET_MAX; i++)
    {
        if (g_registry.sheets[i].textureOk && g_registry.sheets[i].texture.id != 0)
            UnloadTexture(g_registry.sheets[i].texture);
    }
    memset(&g_registry, 0, sizeof(g_registry));
}

int ArtAtlasCachedCount(void)
{
    return g_registry.count;
}

/* Misura il centro del 9-patch: uniforme o no, e con quale colore. No-op per
   uno sheet che non dichiara 'slice' (la grande maggioranza) e per un centro
   degenere (bordi che coprono tutto il fotogramma). */
static void MeasureSliceCenter(const Image *image, ArtSheet *sheet)
{
    sheet->sliceCenterUniform = false;
    if (sheet->sliceL <= 0 || sheet->sliceT <= 0 || sheet->sliceR <= 0 || sheet->sliceB <= 0) return;
    if (sheet->frameW <= 0 || sheet->frameH <= 0) return;
    if (!image->data || image->format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) return;

    int x0 = sheet->sliceL, y0 = sheet->sliceT;
    int x1 = sheet->frameW - sheet->sliceR, y1 = sheet->frameH - sheet->sliceB;
    if (x1 <= x0 || y1 <= y0) return;
    if (x1 > image->width || y1 > image->height) return;

    const Color *pixels = (const Color *)image->data;
    Color first = pixels[(size_t)y0*image->width + x0];
    for (int y = y0; y < y1; y++)
    {
        const Color *row = pixels + (size_t)y*image->width;
        for (int x = x0; x < x1; x++)
        {
            const Color *c = &row[x];
            if (c->r != first.r || c->g != first.g || c->b != first.b || c->a != first.a) return;
        }
    }
    sheet->sliceCenterUniform = true;
    sheet->sliceCenterColor = first;
}

const ArtSheet *ArtAtlasGet(const char *key)
{
    if (!key || !key[0]) return NULL;
    if (strlen(key) >= ART_KEY_LEN) return NULL;
    /* Nessun ".." nella chiave: le chiavi nascono da tabelle interne e da
       image-id di file curati a mano, ma un id storpiato non deve poter
       comporre un percorso fuori da assets/art/. */
    if (strstr(key, "..")) return NULL;

    for (int i = 0; i < g_registry.count && i < ART_SHEET_MAX; i++)
    {
        if (strcmp(g_registry.sheets[i].key, key) == 0)
        {
            /* Voce NEGATIVA (manifest o texture mancanti): si ricorda, cosi'
               un asset assente non ritenta due file a ogni frame di disegno.
               Stessa disciplina della cache di AssetsCuratedTexture. */
            const ArtSheet *sheet = &g_registry.sheets[i];
            return (sheet->manifestOk && sheet->textureOk) ? sheet : NULL;
        }
    }
    if (g_registry.count >= ART_SHEET_MAX)
    {
        /* Una volta sola: se il registro si riempie, ogni disegno passerebbe da
           qui e allagherebbe stderr. Il gioco continua col percorso precedente,
           ma il messaggio dice DOVE guardare (ART_SHEET_MAX, art_atlas.h). */
        static bool warned = false;
        if (!warned)
        {
            warned = true;
            fprintf(stderr, "ArtAtlas: registro pieno (%d voci, comprese quelle negative): "
                            "\"%s\" e i successivi ricadono sul percorso precedente. "
                            "Alzare ART_SHEET_MAX in src/assets/art_atlas.h.\n", ART_SHEET_MAX, key);
        }
        return NULL;
    }

    int slot = g_registry.count++;
    ArtSheet *sheet = &g_registry.sheets[slot];
    memset(sheet, 0, sizeof(*sheet));
    snprintf(sheet->key, sizeof(sheet->key), "%s", key);

    /* Il separatore si aggiunge SOLO se la radice non ce l'ha gia'.
       ART_ATLAS_DIR finisce con '/' (e' la convenzione del progetto, come
       CURATED_IMAGE_DIR), ma una cartella di fixture arriva da mkdtemp, che non
       lo mette: senza questa riga il percorso diventerebbe
       "/tmp/worldsmelt-art-XXXitems/..." e il test cercherebbe file che non
       esistono -- difetto trovato eseguendolo, non ragionandoci. */
    const char *root = ArtDir();
    size_t rootLen = strlen(root);
    const char *sep = (rootLen > 0 && root[rootLen - 1] == '/') ? "" : "/";
    char manifestPath[400];
    char texturePath[400];
    snprintf(manifestPath, sizeof(manifestPath), "%s%s%s.json", root, sep, key);
    snprintf(texturePath, sizeof(texturePath), "%s%s%s.png", root, sep, key);
    if (!FileExists(manifestPath) || !FileExists(texturePath)) return NULL;

    char *text = LoadFileText(manifestPath);
    if (!text) return NULL;
    /* Il parser azzera 'out': la chiave va riscritta dopo, non prima. */
    bool parsed = ArtAtlasParseManifest(text, sheet);
    UnloadFileText(text);
    snprintf(sheet->key, sizeof(sheet->key), "%s", key);
    if (!parsed) return NULL;

    /* Si passa da un Image e non direttamente da LoadTexture perche' il centro
       del 9-patch va misurato sui PIXEL, e in GPU non si guardano piu' (vedi
       ArtSheet.sliceCenterUniform). Per gli sheet che non sono 9-patch il
       passaggio in piu' e' un memcpy, non un costo osservabile: si caricano una
       volta sola per processo. */
    Image image = LoadImage(texturePath);
    if (!image.data) return NULL;
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    MeasureSliceCenter(&image, sheet);
    sheet->texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (sheet->texture.id == 0) return NULL;
    /* Pixel art: nessun filtro, come per l'atlas e le immagini curate. */
    SetTextureFilter(sheet->texture, TEXTURE_FILTER_POINT);
    sheet->textureOk = true;
    return sheet;
}

/* Le categorie di assets/art/, nell'ordine di scansione di
   ArtAtlasFindByImageId. "character" per ultimo: e' l'unica categoria a voce
   singola (il personaggio non e' contenuto pescabile) e non verra' mai
   raggiunta da un image-id di contenuto. */
static const char *const ART_CATEGORIES[] = { "items", "enemies", "bosses", "props", "shots", "character", NULL };

const ArtSheet *ArtAtlasFindByImageId(const char *imageId)
{
    if (!imageId || !imageId[0]) return NULL;
    for (int i = 0; ART_CATEGORIES[i]; i++)
    {
        char key[ART_KEY_LEN];
        int written = snprintf(key, sizeof(key), "%s/%s", ART_CATEGORIES[i], imageId);
        if (written <= 0 || written >= (int)sizeof(key)) return NULL;
        const ArtSheet *sheet = ArtAtlasGet(key);
        if (sheet) return sheet;
    }
    return NULL;
}

const ArtAnim *ArtSheetAnim(const ArtSheet *sheet, const char *name)
{
    if (!sheet || !name || !name[0]) return NULL;
    for (int i = 0; i < sheet->animCount && i < ART_ANIM_MAX; i++)
    {
        if (strcmp(sheet->anims[i].name, name) == 0) return &sheet->anims[i];
    }
    return NULL;
}

const ArtAnim *ArtSheetAnimAny(const ArtSheet *sheet, const char *const *names)
{
    if (!sheet || sheet->animCount <= 0) return NULL;
    for (int i = 0; names && names[i]; i++)
    {
        const ArtAnim *anim = ArtSheetAnim(sheet, names[i]);
        if (anim) return anim;
    }
    return &sheet->anims[0];
}

Rectangle ArtSheetFrameRect(const ArtSheet *sheet, int row, int frame)
{
    if (!sheet || sheet->frameW <= 0 || sheet->frameH <= 0) return (Rectangle){ 0.0f, 0.0f, 0.0f, 0.0f };
    int cols = sheet->texture.width/sheet->frameW;
    int rows = sheet->texture.height/sheet->frameH;
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    /* Clamp e non modulo: un manifest che dichiara 6 fotogrammi dove il PNG ne
       contiene 4 deve mostrare il quarto (l'ultimo disegnato), non tornare al
       primo -- il difetto si vede come "l'animazione si blocca", che e'
       diagnosticabile, invece che come uno sfarfallio. */
    if (frame < 0) frame = 0;
    if (frame >= cols) frame = cols - 1;
    if (row < 0) row = 0;
    if (row >= rows) row = rows - 1;
    return (Rectangle){ (float)(frame*sheet->frameW), (float)(row*sheet->frameH),
                        (float)sheet->frameW, (float)sheet->frameH };
}

bool ArtSheetTileRect(const ArtSheet *sheet, const char *role, Rectangle *outSrc)
{
    if (!sheet || !role || !role[0] || !outSrc) return false;
    if (sheet->tileW <= 0 || sheet->tileH <= 0) return false;
    for (int i = 0; i < sheet->roleCount && i < ART_ROLE_MAX; i++)
    {
        if (strcmp(sheet->roles[i].name, role) != 0) continue;
        float x = (float)(sheet->roles[i].col*sheet->tileW);
        float y = (float)(sheet->roles[i].row*sheet->tileH);
        /* Un ruolo che punta FUORI dal PNG (griglia dichiarata piu' grande
           dell'immagine) e' come un ruolo assente: meglio il colore piatto di
           sempre che un rettangolo di pixel trasparenti o ripetuti. */
        if (x + (float)sheet->tileW > (float)sheet->texture.width) return false;
        if (y + (float)sheet->tileH > (float)sheet->texture.height) return false;
        *outSrc = (Rectangle){ x, y, (float)sheet->tileW, (float)sheet->tileH };
        return true;
    }
    return false;
}

const ArtGlyph *ArtSheetGlyph(const ArtSheet *sheet, char ch)
{
    if (!sheet) return NULL;
    for (int i = 0; i < sheet->glyphCount && i < ART_GLYPH_MAX; i++)
    {
        if (sheet->glyphs[i].ch == ch) return &sheet->glyphs[i];
    }
    return NULL;
}

int ArtAnimFrameAt(const ArtAnim *anim, float elapsed)
{
    if (!anim || anim->frames <= 1) return 0;
    if (anim->fps <= 0 || elapsed <= 0.0f) return 0;
    /* Il conteggio passa da un intero e non da fmodf: e' quello che rende la
       funzione riproducibile bit a bit fra piattaforme (il test dell'animatore
       confronta indici esatti) e fa scattare il fotogramma esattamente al
       confine 1/fps. */
    int step = (int)(elapsed*(float)anim->fps);
    if (step < 0) return 0;   /* elapsed enorme: l'int e' straripato, si torna al primo fotogramma invece di indicizzare a caso */
    if (anim->loop) return step % anim->frames;
    return (step >= anim->frames) ? anim->frames - 1 : step;
}

bool ArtAnimDone(const ArtAnim *anim, float elapsed)
{
    /* Nessuna animazione = niente da scorrere, quindi "finita": chi chiama usa
       questa funzione per decidere quando SMETTERE di mostrare qualcosa, e su
       un'animazione che non esiste la risposta giusta e' "smetti subito" --
       false lascerebbe un effetto appeso per sempre. Stessa logica del caso
       frames/fps a zero qui sotto. */
    if (!anim) return true;
    if (anim->loop) return false;
    if (anim->frames <= 0 || anim->fps <= 0) return true;
    return elapsed >= (float)anim->frames/(float)anim->fps;
}
