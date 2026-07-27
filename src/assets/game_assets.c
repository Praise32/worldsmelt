#include "assets/game_assets.h"

#include "content/curated_images.h"
#include "script/script_items.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void GameUnloadAssets(Game *game)
{
    if (game->atlasLoaded)
    {
        UnloadTexture(game->atlas);
        game->atlasLoaded = false;
        game->atlas.id = 0;
    }
    memset(game->atlasCellPresent, 0, sizeof(game->atlasCellPresent));

    /* DEC-171: le texture curate seguono l'atlas, stesso ciclo di vita e
       stesso punto di rilascio (vedi AssetsCuratedTexture piu' sotto). */
    for (int i = 0; i < game->curatedTextures.count && i < MAX_CURATED_TEXTURES; i++)
    {
        if (game->curatedTextures.textures[i].id != 0) UnloadTexture(game->curatedTextures.textures[i]);
    }
    memset(&game->curatedTextures, 0, sizeof(game->curatedTextures));

    /* Ogni ScriptSandbox viva (una per oggetto con Lua, vedi
       core/game_types.h, Game.itemScripts) va chiusa qui, non lasciata al
       memset di GameResetRun: quel memset azzererebbe i puntatori senza mai
       chiamare lua_close, perdendo la memoria di Lua (fuori dal heap C
       normale, mai vista da free()). GameUnloadAssets e' il punto giusto
       perche' e' GIA' la prima riga di GameResetRun (game.c) e l'ultima
       chiamata di ogni percorso di uscita in src/app/app.c: un solo punto
       di pulizia per tutto cio' che Game possiede, invece di doverlo
       ricordare in ogni call site. */
    ScriptItemsShutdown(game);
}

static bool IsAtlasKeyPixel(Color c, Color key)
{
    int dr = abs((int)c.r - (int)key.r);
    int dg = abs((int)c.g - (int)key.g);
    int db = abs((int)c.b - (int)key.b);
    bool nearKey = (dr + dg + db) < 42 && c.r < 45 && c.g < 55 && c.b < 55;
    bool nearBlack = c.r < 18 && c.g < 22 && c.b < 24;
    return nearKey || nearBlack;
}

/* Vero se image ha gia' un canale alpha "vero" (almeno un pixel non del
   tutto opaco). L'atlas PNG di melting-sprites lo ha sempre: il ritaglio a
   flood fill lascia trasparenti lo sfondo e (per costruzione, vedi
   SpritesAtlasNew) tutte le celle dell'atlas 8x8 non usate dalle 12 note.
   I vecchi spritesheet (API, pre fase-2) erano invece PNG completamente
   opachi, senza alcuna informazione di trasparenza: per quelli il
   chroma-key sotto resta l'unica rete di sicurezza. */
static bool ImageHasRealAlpha(const Image *image)
{
    const Color *pixels = (const Color *)image->data;
    int total = image->width*image->height;
    for (int i = 0; i < total; i++) if (pixels[i].a != 255) return true;
    return false;
}

/* Scandisce le SPR_COUNT celle note dell'atlas (stesso layout usato per il
   disegno: colonna cell%ATLAS_COLS, riga cell/ATLAS_COLS) e registra quali
   hanno abbastanza pixel opachi da essere sprite veri. Deve girare DOPO ogni
   eventuale chroma-key, cosi' riflette l'alpha finale che verra' disegnato.
   Per un atlas BMP procedurale (senza canale alpha: ImageFormat lo riempie
   tutto a 255) ogni cella risulta sempre "presente", cioe' il comportamento
   di oggi non cambia. */
static void ScanAtlasCells(const Image *image, bool *cellPresent)
{
    const Color *pixels = (const Color *)image->data;
    for (int cell = 0; cell < SPR_COUNT; cell++)
    {
        cellPresent[cell] = false;
        int ox = (cell%ATLAS_COLS)*ATLAS_CELL;
        int oy = (cell/ATLAS_COLS)*ATLAS_CELL;
        if (ox + ATLAS_CELL > image->width || oy + ATLAS_CELL > image->height) continue;
        int opaque = 0;
        for (int y = 0; y < ATLAS_CELL && opaque < ATLAS_CELL_MIN_OPAQUE; y++)
        {
            const Color *row = pixels + (size_t)(oy + y)*image->width + ox;
            for (int x = 0; x < ATLAS_CELL; x++) if (row[x].a > 0) opaque++;
        }
        cellPresent[cell] = opaque >= ATLAS_CELL_MIN_OPAQUE;
    }
}

static Texture2D LoadAtlasTexture(const char *path, bool *cellPresent)
{
    Image image = LoadImage(path);
    if (!image.data) return (Texture2D){ 0 };
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    /* Il chroma-key sul quasi-nero serve solo da rete per i PNG SENZA un
       canale alpha vero (i vecchi spritesheet API). Il nuovo atlas locale ha
       gia' un alpha vero e affidabile (flood fill + KEY_FLOOR nel tool):
       applicargli comunque il chroma-key rischierebbe di mangiare pixel
       opachi dello sprite (KEY_FLOOR garantisce solo che il canale piu'
       chiaro di ogni pixel sia >=16, non che tutti e tre i canali superino
       le soglie per-canale usate qui sotto). */
    bool isPng = strstr(path, ".png") != NULL;
    bool hasRealAlpha = ImageHasRealAlpha(&image);
    if (isPng && !hasRealAlpha)
    {
        Color *pixels = (Color *)image.data;
        Color key = pixels[0];
        int total = image.width*image.height;
        for (int i = 0; i < total; i++)
        {
            if (IsAtlasKeyPixel(pixels[i], key)) pixels[i].a = 0;
        }
    }

    ScanAtlasCells(&image, cellPresent);

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

const Texture2D *AssetsCuratedTexture(Game *game, const char *relativePath)
{
    if (!game || !relativePath || !relativePath[0]) return NULL;

    CuratedTextureCache *cache = &game->curatedTextures;
    for (int i = 0; i < cache->count && i < MAX_CURATED_TEXTURES; i++)
    {
        if (strcmp(cache->paths[i], relativePath) == 0)
            return cache->textures[i].id != 0 ? &cache->textures[i] : NULL;
    }
    if (cache->count >= MAX_CURATED_TEXTURES) return NULL;

    char full[192];
    snprintf(full, sizeof(full), "%s%s", CURATED_IMAGE_DIR, relativePath);
    if (!FileExists(full)) return NULL;

    /* Lo slot si occupa PRIMA di sapere se il caricamento riesce: cosi' un
       file rotto viene tentato una volta sola e non ad ogni frame di
       disegno (id 0 nello slot = "gia' provato, non c'e' nulla da
       disegnare"). Stessa filosofia di atlasCellPresent per l'atlas. */
    int slot = cache->count++;
    snprintf(cache->paths[slot], sizeof(cache->paths[slot]), "%s", relativePath);
    cache->textures[slot] = LoadTexture(full);
    if (cache->textures[slot].id == 0) return NULL;
    /* Pixel art: nessun filtro, come per l'atlas. */
    SetTextureFilter(cache->textures[slot], TEXTURE_FILTER_POINT);
    return &cache->textures[slot];
}

void AssetsLoad(Game *game)
{
    memset(game->atlasCellPresent, 0, sizeof(game->atlasCellPresent));
    if (!game->content.atlasPath[0] || !FileExists(game->content.atlasPath)) return;
    game->atlas = LoadAtlasTexture(game->content.atlasPath, game->atlasCellPresent);
    game->atlasLoaded = game->atlas.id != 0;
    if (game->atlasLoaded) SetTextureFilter(game->atlas, TEXTURE_FILTER_POINT);
    else memset(game->atlasCellPresent, 0, sizeof(game->atlasCellPresent));
}
