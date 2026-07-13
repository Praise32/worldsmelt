/* Costruzione dei prompt dai template su disco (tools/melting-sprites/prompts/),
   stesso schema di tools/melting-gen/prompts/: testo editabile con segnaposto,
   niente ricompilazione per cambiare un prompt. */
#include "melting_sprites.h"

#include <stdlib.h>
#include <string.h>

/* Stesso ordine di AtlasSprite in src/core/game_types.h. */
const char *SPRITE_CELL_NAMES[SPRITE_CELLS] = {
    "player", "enemy_chaser", "enemy_shooter", "enemy_tank", "boss", "item",
    "heart", "coin", "bomb", "key", "exit", "shot"
};

static char *TrimTrailingWhitespace(char *s)
{
    if (!s) return s;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = '\0';
    return s;
}

char *SpritesExpandTemplate(const char *templateText, const char *theme, const char *style)
{
    if (!templateText) return NULL;
    const char *t = theme ? theme : "";
    const char *s = style ? style : "";
    size_t themeLen = strlen(t), styleLen = strlen(s);

    /* Sovradimensiona invece di calcolare la lunghezza esatta: ogni occorrenza
       di un segnaposto (7 byte, "{THEME}" e "{STYLE}" hanno la stessa
       lunghezza) e' gia' contata nella lunghezza del template, quindi
       aggiungere anche themeLen/styleLen per occorrenza riserva sempre
       abbastanza spazio, mai troppo poco. */
    size_t outCap = strlen(templateText) + 1;
    for (const char *p = templateText; (p = strstr(p, "{THEME}")) != NULL; p += 7) outCap += themeLen;
    for (const char *p = templateText; (p = strstr(p, "{STYLE}")) != NULL; p += 7) outCap += styleLen;

    char *out = malloc(outCap);
    if (!out) return NULL;
    size_t used = 0;
    const char *p = templateText;
    while (*p)
    {
        if (strncmp(p, "{THEME}", 7) == 0)
        {
            memcpy(out + used, t, themeLen);
            used += themeLen;
            p += 7;
        }
        else if (strncmp(p, "{STYLE}", 7) == 0)
        {
            memcpy(out + used, s, styleLen);
            used += styleLen;
            p += 7;
        }
        else out[used++] = *p++;
    }
    out[used] = '\0';
    return out;
}

char *SpritesLoadCellPrompt(const char *promptsDir, int cellIndex, const char *theme, const char *style)
{
    if (cellIndex < 0 || cellIndex >= SPRITE_CELLS) return NULL;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.txt", promptsDir, SPRITE_CELL_NAMES[cellIndex]);
    char *tmpl = SpritesReadFile(path);
    if (!tmpl) return NULL;
    TrimTrailingWhitespace(tmpl);
    if (!tmpl[0])
    {
        /* File presente ma vuoto (o di soli spazi/a-capo, azzerati dal trim
           sopra): stesso trattamento di un file mancante, come documentato
           in melting_sprites.h. Senza questo controllo un prompt vuoto
           arriverebbe comunque al modello, che genererebbe un'immagine non
           guidata invece di lasciare la cella trasparente. */
        free(tmpl);
        return NULL;
    }
    char *out = SpritesExpandTemplate(tmpl, theme, style);
    free(tmpl);
    return out;
}

char *SpritesLoadNegativePrompt(const char *promptsDir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/negative.txt", promptsDir);
    return TrimTrailingWhitespace(SpritesReadFile(path));
}
