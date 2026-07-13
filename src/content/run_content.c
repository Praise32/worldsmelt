#include "content/run_content.h"

#include "core/game_math.h"
#include "gameplay/item_traits.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int HexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return 0;
}

static Color ParseHexColor(const char *text, Color fallback)
{
    if (!text || text[0] != '#' || strlen(text) < 7) return fallback;
    return (Color){
        (unsigned char)(HexDigit(text[1])*16 + HexDigit(text[2])),
        (unsigned char)(HexDigit(text[3])*16 + HexDigit(text[4])),
        (unsigned char)(HexDigit(text[5])*16 + HexDigit(text[6])),
        255
    };
}

static void ReadManifestValue(const char *text, const char *key, char *out, int outSize)
{
    if (!text || !key || !out || outSize <= 0) return;
    const char *start = strstr(text, key);
    if (!start) return;
    start += strlen(key);
    int i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
}

static ItemSlot SlotFromText(const char *text)
{
    if (strstr(text, "eyes")) return SLOT_EYES;
    if (strstr(text, "hand")) return SLOT_HAND;
    if (strstr(text, "back")) return SLOT_BACK;
    if (strstr(text, "body")) return SLOT_BODY;
    if (strstr(text, "aura")) return SLOT_AURA;
    return SLOT_HAT;
}

/* Fase 3 (vedi ItemKind in core/game_types.h): "statup" e' l'UNICO testo che
   produce ITEM_STATUP, qualunque altra cosa (mancante, vuoto, "active",
   refuso) ricade su ITEM_ACTIVE. Il chiamante decide se invocarla affatto:
   quando la chiave del manifest e' assente (run vecchia, oggetto senza
   riga "kind=") il campo NON va toccato qui, resta quello gia' impostato dal
   contenuto di ripiego (vedi RunContentLoad sotto, stesso schema "per-key
   fallback" di ogni altro campo di questo file). */
static ItemKind ItemKindFromText(const char *text)
{
    return (text && strcmp(text, "statup") == 0) ? ITEM_STATUP : ITEM_ACTIVE;
}

static const char *FallbackScriptForTrait(unsigned int trait)
{
    if (trait & TRAIT_BOUNCE) return "on_fire:burst,2,0.25,bounce";
    if (trait & TRAIT_HOMING) return "on_hit:projectile,2,260,homing";
    if (trait & TRAIT_EXPLODE) return "on_hit:area,58,0.48,explode";
    if (trait & TRAIT_SPLIT) return "on_fire:burst,3,0.36,split";
    if (trait & TRAIT_PIERCE) return "on_hit:projectile,1,420,pierce";
    if (trait & TRAIT_RAPID) return "on_fire:burst,2,0.16,rapid";
    if (trait & TRAIT_GIANT) return "on_hit:area,44,0.34,giant";
    if (trait & TRAIT_SLOW) return "on_hit:area,54,0.22,slow";
    if (trait & TRAIT_VAMP) return "on_hit:heal,18,1,vamp";
    return "on_hit:projectile,1,300,none";
}

static unsigned int RandomTrait(unsigned int *rng)
{
    static const unsigned int traits[] = {
        TRAIT_BOUNCE, TRAIT_HOMING, TRAIT_EXPLODE, TRAIT_SPLIT, TRAIT_PIERCE,
        TRAIT_RAPID, TRAIT_GIANT, TRAIT_SLOW, TRAIT_VAMP
    };
    return traits[GameRngRange(rng, 0, (int)(sizeof(traits)/sizeof(traits[0])) - 1)];
}

static Item MakeFallbackItem(unsigned int *rng, const Theme *theme, int index)
{
    static const char *names[] = {
        "Cappello Rimbalzino", "Occhiali Guidati", "Guanto Bomba",
        "Mantello Split", "Medaglia Rapida", "Corona Gigante"
    };
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_ACTIVE;
    snprintf(item.name, sizeof(item.name), "%s", names[(index + GameRngRange(rng, 0, 5))%6]);
    item.slot = (ItemSlot)GameRngRange(rng, 0, 5);
    item.traits = RandomTrait(rng);
    if (GameRngRange(rng, 0, 100) < 28) item.traits |= RandomTrait(rng);
    item.color = GameColorLerp(theme->accent, ColorFromHSV(GameRngFloat(rng, 0.0f, 360.0f), 0.75f, 0.95f), 0.45f);
    item.shape = GameRngRange(rng, 0, 4);
    snprintf(item.script, sizeof(item.script), "%s", FallbackScriptForTrait(item.traits));
    return item;
}

/* Oggetto stat-up di ripiego (fase 3, ricompensa del boss): stesso stile
   procedurale di MakeFallbackItem sopra, ma senza alcuno script mini-VM
   (item.script resta "" -- un oggetto stat-up non ha comportamento, solo
   statistiche, vedi ScriptItemsRecomputeStats/ScriptItemsApplyStatUpFallback
   in src/script/script_items.c) e con un solo trait (usato SOLO come
   etichetta per il ripiego C se non c'e' un on_evaluate Lua valido, mai per
   pilotare la mini-VM). */
static Item MakeFallbackBossItem(unsigned int *rng, const Theme *theme, int floorIdx)
{
    static const char *names[] = {
        "Reliquia Possente", "Nucleo Ardente", "Sigillo Vitale",
        "Cristallo Rapido", "Totem Solido", "Anima Grande"
    };
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_STATUP;
    snprintf(item.name, sizeof(item.name), "%s", names[(floorIdx + GameRngRange(rng, 0, 5))%6]);
    item.slot = (ItemSlot)GameRngRange(rng, 0, 5);
    item.traits = RandomTrait(rng);
    item.color = GameColorLerp(theme->accent2, ColorFromHSV(GameRngFloat(rng, 0.0f, 360.0f), 0.65f, 0.92f), 0.5f);
    item.shape = GameRngRange(rng, 0, 4);
    item.script[0] = '\0';
    return item;
}

static Theme MakeFallbackTheme(unsigned int *rng, int floor)
{
    static const char *themes[] = {
        "Cantina Neon", "Biblioteca Muffita", "Fucina Lunare", "Acquario Radioattivo", "Cattedrale di Zucchero"
    };
    static const char *styles[] = {
        "pixel semplice", "toon scuro", "arcade secco", "inchiostro piatto", "low-fi fantasy"
    };
    float hue = GameRngFloat(rng, 0.0f, 360.0f);
    Theme theme = { 0 };
    snprintf(theme.name, sizeof(theme.name), "%s", themes[(floor - 1)%5]);
    snprintf(theme.style, sizeof(theme.style), "%s", styles[GameRngRange(rng, 0, 4)]);
    snprintf(theme.bossName, sizeof(theme.bossName), "Custode Piano %d", floor);
    theme.bg = ColorFromHSV(hue, 0.30f, 0.12f);
    theme.floor = ColorFromHSV(fmodf(hue + 20.0f, 360.0f), 0.38f, 0.22f);
    theme.wall = ColorFromHSV(fmodf(hue + 52.0f, 360.0f), 0.55f, 0.45f);
    theme.accent = ColorFromHSV(fmodf(hue + 100.0f, 360.0f), 0.62f, 0.86f);
    theme.accent2 = ColorFromHSV(fmodf(hue + 172.0f, 360.0f), 0.70f, 0.94f);
    theme.enemy = ColorFromHSV(fmodf(hue + 235.0f, 360.0f), 0.58f, 0.82f);
    theme.boss = ColorFromHSV(fmodf(hue + 300.0f, 360.0f), 0.75f, 0.88f);
    return theme;
}

static void GenerateFallbackContent(RunContent *content, unsigned int seed)
{
    unsigned int rng = seed ^ 0xBAD51DEu;
    content->loaded = false;
    snprintf(content->atlasPath, sizeof(content->atlasPath), "generated/current_atlas.bmp");
    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        content->floors[f].theme = MakeFallbackTheme(&rng, f + 1);
        for (int i = 0; i < 3; i++)
        {
            content->floors[f].items[i] = MakeFallbackItem(&rng, &content->floors[f].theme, i);
        }
        content->floors[f].bossItem = MakeFallbackBossItem(&rng, &content->floors[f].theme, f);
    }
}

/* melting-gen scrive sempre "atlas.path=generated/current_atlas.bmp" nel
   manifest (vedi gen_manifest.c): e' melting-sprites, se e quando il passo
   sprite va a buon fine, a riscrivere quella riga puntando al PNG (vedi
   SpritesUpdateManifestAtlasPath in tools/melting-sprites/sprite_manifest.c).
   Quella riscrittura pero' non e' atomica insieme alla scrittura del PNG: un
   ESC o un timeout che uccide melting-sprites fra le due scritture
   lascerebbe un PNG completo (rename() atomico l'ha gia' pubblicato) ma un
   manifest che dichiara ancora il BMP. Il gioco decide quindi da solo,
   confrontando le date dei file su disco invece di fidarsi di quel campo:
   se il PNG esiste ed e' piu' recente (o della stessa epoca, risoluzione a
   1s) del manifest appena letto, lo si preferisce. Se il PNG non esiste, o
   e' piu' vecchio del manifest (run precedente, passo sprite mai partito o
   saltato con --no-sprites), resta il BMP che il manifest dichiara: stesso
   comportamento di oggi, degrada correttamente senza bisogno di riscrivere
   il manifest da questo lato. */
static void PreferPngAtlasIfFresh(RunContent *content)
{
    static const char *MANIFEST_PATH = "generated/current_run.txt";
    static const char *PNG_PATH = "generated/current_atlas.png";
    if (!FileExists(PNG_PATH)) return;
    if (GetFileModTime(PNG_PATH) >= GetFileModTime(MANIFEST_PATH))
        snprintf(content->atlasPath, sizeof(content->atlasPath), "%s", PNG_PATH);
}

void RunContentLoad(RunContent *content, unsigned int seed)
{
    GenerateFallbackContent(content, seed);

    char *text = LoadFileText("generated/current_run.txt");
    if (!text)
    {
        PreferPngAtlasIfFresh(content);
        return;
    }

    char value[SCRIPT_TEXT_LEN];
    bool loadedSomething = false;

    value[0] = '\0';
    ReadManifestValue(text, "atlas.path=", value, sizeof(value));
    if (value[0]) snprintf(content->atlasPath, sizeof(content->atlasPath), "%s", value);

    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        FloorContent *floor = &content->floors[f];
        int n = f + 1;
        char key[80];

        snprintf(key, sizeof(key), "floor%d.theme=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0])
        {
            snprintf(floor->theme.name, sizeof(floor->theme.name), "%s", value);
            loadedSomething = true;
        }

        snprintf(key, sizeof(key), "floor%d.style=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) snprintf(floor->theme.style, sizeof(floor->theme.style), "%s", value);

        snprintf(key, sizeof(key), "floor%d.boss=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) snprintf(floor->theme.bossName, sizeof(floor->theme.bossName), "%s", value);

        snprintf(key, sizeof(key), "floor%d.bg=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.bg = ParseHexColor(value, floor->theme.bg);

        snprintf(key, sizeof(key), "floor%d.floor=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.floor = ParseHexColor(value, floor->theme.floor);

        snprintf(key, sizeof(key), "floor%d.wall=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.wall = ParseHexColor(value, floor->theme.wall);

        snprintf(key, sizeof(key), "floor%d.accent=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.accent = ParseHexColor(value, floor->theme.accent);

        snprintf(key, sizeof(key), "floor%d.accent2=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.accent2 = ParseHexColor(value, floor->theme.accent2);

        snprintf(key, sizeof(key), "floor%d.enemy=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.enemy = ParseHexColor(value, floor->theme.enemy);

        snprintf(key, sizeof(key), "floor%d.bossColor=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) floor->theme.boss = ParseHexColor(value, floor->theme.boss);

        for (int i = 0; i < 3; i++)
        {
            Item *item = &floor->items[i];
            snprintf(key, sizeof(key), "floor%d.item%d.name=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) snprintf(item->name, sizeof(item->name), "%s", value);

            snprintf(key, sizeof(key), "floor%d.item%d.slot=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) item->slot = SlotFromText(value);

            snprintf(key, sizeof(key), "floor%d.item%d.traits=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) item->traits = ItemTraitsFromText(value);

            snprintf(key, sizeof(key), "floor%d.item%d.color=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) item->color = ParseHexColor(value, item->color);

            /* Fase 3: riga assente (manifest scritto prima di questo task, o
               un vecchio golden file) -> item->kind resta ITEM_ACTIVE, gia'
               impostato da MakeFallbackItem sopra (stesso schema "per-key
               fallback" di ogni altro campo qui). */
            snprintf(key, sizeof(key), "floor%d.item%d.kind=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) item->kind = ItemKindFromText(value);

            snprintf(key, sizeof(key), "floor%d.item%d.script=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) snprintf(item->script, sizeof(item->script), "%s", value);

            /* Fase 3a-L3: sorgente Lua opzionale, in un file a parte (vedi
               tools/melting-gen/gen_lua.c e gen_manifest.c). item->luaSource
               resta vuota (mini-VM soltanto) di default: una riga assente
               (oggetto senza Lua valido), o presente ma che punta a un file
               mancante/illeggibile (run copiata a meta', disco esterno
               scollegato...), degrada silenziosamente allo stesso modo,
               MAI un errore fatale per il caricamento del manifest. */
            item->luaSource[0] = '\0';
            snprintf(key, sizeof(key), "floor%d.item%d.lua=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0])
            {
                char *luaText = LoadFileText(value);
                if (luaText)
                {
                    snprintf(item->luaSource, sizeof(item->luaSource), "%s", luaText);
                    UnloadFileText(luaText);
                }
            }
            item->active = true;
        }

        /* Oggetto stat-up del piano (fase 3, ricompensa del boss): stesso
           schema chiave=valore/per-key-fallback di items[] sopra, ma col
           prefisso "bossItem" e SENZA ".script=" (nessuna riga da leggere:
           un manifest scritto da questa fase non la scrive mai, vedi
           WriteManifest in gen_manifest.c, quindi floor->bossItem.script
           resta "" -- gia' cosi' dal contenuto di ripiego). Una run
           generata PRIMA di questo task non ha nessuna di queste chiavi:
           l'intero bossItem resta quello di MakeFallbackBossItem sopra
           (kind=ITEM_STATUP incluso), mai un oggetto vuoto. */
        Item *boss = &floor->bossItem;
        snprintf(key, sizeof(key), "floor%d.bossItem.name=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) snprintf(boss->name, sizeof(boss->name), "%s", value);

        snprintf(key, sizeof(key), "floor%d.bossItem.slot=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) boss->slot = SlotFromText(value);

        snprintf(key, sizeof(key), "floor%d.bossItem.traits=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) boss->traits = ItemTraitsFromText(value);

        snprintf(key, sizeof(key), "floor%d.bossItem.color=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) boss->color = ParseHexColor(value, boss->color);

        snprintf(key, sizeof(key), "floor%d.bossItem.kind=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) boss->kind = ItemKindFromText(value);

        boss->luaSource[0] = '\0';
        snprintf(key, sizeof(key), "floor%d.bossItem.lua=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0])
        {
            char *luaText = LoadFileText(value);
            if (luaText)
            {
                snprintf(boss->luaSource, sizeof(boss->luaSource), "%s", luaText);
                UnloadFileText(luaText);
            }
        }
        boss->active = true;
    }

    content->loaded = loadedSomething;
    UnloadFileText(text);
    PreferPngAtlasIfFresh(content);
}
