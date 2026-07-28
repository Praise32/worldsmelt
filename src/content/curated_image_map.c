#include "content/curated_image_map.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>

/* Toglie spazi/tab iniziali e spazi/tab/CR finali sul posto. Nessuna copia:
   il chiamante passa sempre un buffer locale che puo' accorciare. */
static void Trim(char *s)
{
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s + start, strlen(s + start) + 1);

    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                        s[len - 1] == '\r' || s[len - 1] == '\n')) s[--len] = '\0';
}

bool CuratedImageMapResolveInText(const char *mapText, const char *contentId, char *outImageId, int outSize)
{
    if (!outImageId || outSize <= 0) return false;
    outImageId[0] = '\0';
    if (!mapText || !contentId || !contentId[0]) return false;

    bool found = false;
    const char *lineStart = mapText;
    while (*lineStart)
    {
        const char *lineEnd = strchr(lineStart, '\n');
        int lineLen = lineEnd ? (int)(lineEnd - lineStart) : (int)strlen(lineStart);

        char line[192];
        int copyLen = (lineLen < (int)sizeof(line) - 1) ? lineLen : (int)sizeof(line) - 1;
        memcpy(line, lineStart, (size_t)copyLen);
        line[copyLen] = '\0';

        if (line[0] != '#')
        {
            char *eq = strchr(line, '=');
            if (eq)
            {
                *eq = '\0';
                char *key = line;
                char *value = eq + 1;
                Trim(key);
                Trim(value);
                if (key[0] && strcmp(key, contentId) == 0)
                {
                    snprintf(outImageId, (size_t)outSize, "%s", value);
                    found = true;
                    break;
                }
            }
        }

        lineStart = lineEnd ? lineEnd + 1 : lineStart + lineLen;
    }

    return found && outImageId[0] != '\0';
}

bool CuratedImageMapResolve(const char *mapPath, const char *contentId, char *outImageId, int outSize)
{
    if (!outImageId || outSize <= 0) return false;
    outImageId[0] = '\0';
    if (!mapPath) return false;
    if (!FileExists(mapPath)) return false;

    char *text = LoadFileText(mapPath);
    if (!text) return false;

    bool found = CuratedImageMapResolveInText(text, contentId, outImageId, outSize);
    UnloadFileText(text);
    return found;
}
