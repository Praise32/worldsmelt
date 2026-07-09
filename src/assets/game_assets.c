#include "assets/game_assets.h"

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

static Texture2D LoadAtlasTexture(const char *path)
{
    Image image = LoadImage(path);
    if (!image.data) return (Texture2D){ 0 };
    bool chromaKey = strstr(path, ".png") != NULL;
    if (chromaKey)
    {
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color *pixels = (Color *)image.data;
        Color key = pixels[0];
        int total = image.width*image.height;
        for (int i = 0; i < total; i++)
        {
            if (IsAtlasKeyPixel(pixels[i], key)) pixels[i].a = 0;
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

void AssetsLoad(Game *game)
{
    if (!game->content.atlasPath[0] || !FileExists(game->content.atlasPath)) return;
    game->atlas = LoadAtlasTexture(game->content.atlasPath);
    game->atlasLoaded = game->atlas.id != 0;
    if (game->atlasLoaded) SetTextureFilter(game->atlas, TEXTURE_FILTER_POINT);
}
