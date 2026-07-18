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

/* Fase 3b (vedi Rarity in core/game_types.h): "common"/"uncommon"/"rare"/
   "legendary" sono gli UNICI testi riconosciuti, qualunque altra cosa
   (refuso, valore inatteso) ricade su RARITY_COMMON. Stesso schema
   "per-key fallback" di ItemKindFromText sopra: il chiamante non invoca
   questa funzione affatto quando la chiave "rarity=" e' assente dal
   manifest (manifest vecchio, scritto prima di questa fase), il campo
   resta quello gia' impostato dal contenuto di ripiego (RARITY_COMMON per
   un oggetto normale; RARITY_RARE per il bossItem dalla fase 3b review "il
   boss non delude mai", vedi MakeFallbackBossItem sotto).

   NON piu' static (fase 3b review, "lock the rarity enum/text sync"): questi
   quattro testi devono restare sincronizzati A MANO con GEN_RARITIES
   (tools/melting-gen/gen_util.c) e con l'ordine dell'enum Rarity
   (core/game_types.h) -- un mismatch silenzioso (es. "uncommon" ->
   RARITY_RARE) passerebbe ogni altro test esistente. Esposta in
   run_content.h SOLO per farla raggiungere dal test di round-trip in
   src/tests/script_items_tests.c (TestRarityTextRoundTrip): nessun altro
   chiamante fuori da questo file. */
Rarity RarityFromText(const char *text)
{
    if (text && strcmp(text, "uncommon") == 0) return RARITY_UNCOMMON;
    if (text && strcmp(text, "rare") == 0) return RARITY_RARE;
    if (text && strcmp(text, "legendary") == 0) return RARITY_LEGENDARY;
    return RARITY_COMMON;
}

/* Tipo di colpo di RIPIEGO per un piano (step C). LEGGERE IL COMMENTO IN CIMA A
   src/core/shot_type.h: i tipi di colpo di una run vera li INVENTA IL MODELLO,
   sempre. Questo e' il ripiego per il caso degenere in cui non esiste alcun
   manifest sul disco (melting-gen non e' mai girato), lo stesso ruolo che
   MakeFallbackItem/MakeFallbackTheme hanno per oggetti e temi. Gli esempi veri
   vivono in core/shot_type.c (ShotTypeExample), condivisi con il ripiego di
   melting-gen: un solo elenco, mai due copie da tenere allineate. */
static ShotTypeDef MakeFallbackShotType(unsigned int *rng)
{
    ShotTypeDef type;
    ShotTypeExample(&type, GameRngRange(rng, 0, SHOT_TYPE_EXAMPLE_COUNT - 1));
    return type;
}

/* Tipi di nemico di RIPIEGO (fase 3b). Come per i tipi di colpo: il motore non ha
   un catalogo di nemici, li inventa il modello. Questi sono i tre nemici storici
   riscritti nel vocabolario nuovo, usati SOLO quando non c'e' nessun manifest.
   Vivono in core/enemy_type.c (EnemyTypeExample), condivisi col ripiego di
   melting-gen: un solo elenco. */
static void MakeFallbackEnemies(FloorContent *floor, unsigned int *rng)
{
    for (int i = 0; i < 2; i++)
    {
        EnemyTypeExample(&floor->enemies[i], GameRngRange(rng, 0, ENEMY_TYPE_EXAMPLE_COUNT - 1));
    }
    EnemyTypeExampleBoss(&floor->bossType);
}

/* Un tipo di nemico dal manifest: "floorN.enemyM.*" (M = 1,2) oppure
   "floorN.bossType.*". La chiave "name" fa da sentinella -- se manca, il piano non
   ha quel tipo e il gioco usa i nemici storici (back-compat totale con ogni
   manifest scritto prima di questa fase). Passa sempre da EnemyTypeBalance: e' la
   seconda rete indipendente dopo quella di melting-gen, perche' un manifest e' un
   file di testo che chiunque puo' modificare a mano. */
static void ReadEnemyType(const char *text, const char *prefix, bool isBoss, EnemyTypeDef *out)
{
    char key[96];
    char value[SCRIPT_TEXT_LEN];

    snprintf(key, sizeof(key), "%s.name=", prefix);
    value[0] = '\0';
    ReadManifestValue(text, key, value, sizeof(value));
    if (!value[0]) return;   /* niente tipo: resta quello che c'era (zero = nemici storici) */

    EnemyTypeDef type;
    memset(&type, 0, sizeof(type));
    type.active = true;
    type.boss = isBoss;
    snprintf(type.name, sizeof(type.name), "%s", value);

    snprintf(key, sizeof(key), "%s.form=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.form = EnemyFormFromText(value);
    snprintf(key, sizeof(key), "%s.move=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.move = EnemyMoveFromText(value);
    snprintf(key, sizeof(key), "%s.fire=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.fire = EnemyFireFromText(value);

    snprintf(key, sizeof(key), "%s.hp=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.hpMul = value[0] ? (float)atof(value) : 1.0f;
    snprintf(key, sizeof(key), "%s.speed=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.speedMul = value[0] ? (float)atof(value) : 1.0f;
    snprintf(key, sizeof(key), "%s.size=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.sizeMul = value[0] ? (float)atof(value) : 1.0f;
    snprintf(key, sizeof(key), "%s.rate=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.fireRate = value[0] ? (float)atof(value) : 0.0f;
    snprintf(key, sizeof(key), "%s.pellets=", prefix);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    type.pellets = value[0] ? atoi(value) : 1;

    EnemyTypeBalance(&type);
    *out = type;
}

/* Il layout della stanza del piano dal manifest (fase 3c). Chiave "name" come
   sentinella: se manca, il piano ha stanze VUOTE (back-compat totale con i manifest
   scritti prima di questa fase). Passa da RoomLayoutClamp (la garanzia vera --
   stanza sempre giocabile -- e' in RoomLayoutBuild, che gira solo lato gioco). */
static void ReadRoomLayout(const char *text, int floorNum, RoomLayoutDef *out)
{
    char key[64];
    char value[SCRIPT_TEXT_LEN];

    snprintf(key, sizeof(key), "floor%d.room.name=", floorNum);
    value[0] = '\0';
    ReadManifestValue(text, key, value, sizeof(value));
    if (!value[0]) return;

    RoomLayoutDef def;
    memset(&def, 0, sizeof(def));
    def.active = true;
    snprintf(def.name, sizeof(def.name), "%s", value);

    snprintf(key, sizeof(key), "floor%d.room.form=", floorNum);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    def.form = RoomFormFromText(value);

    snprintf(key, sizeof(key), "floor%d.room.density=", floorNum);
    value[0] = '\0'; ReadManifestValue(text, key, value, sizeof(value));
    def.density = value[0] ? (float)atof(value) : 0.5f;

    RoomLayoutClamp(&def);
    /* Un layout OPEN col nome non e' un layout: se il modello ha detto "open", il
       piano resta a stanze vuote (active falso), non un layout attivo che non
       disegna nulla ma occupa il campo. */
    if (def.form == ROOM_LAYOUT_OPEN) return;
    *out = def;
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
        "Bouncy Hat", "Homing Goggles", "Bomb Glove",
        "Split Cloak", "Swift Medal", "Giant Crown"
    };
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_ACTIVE;
    /* Fase 3b: questo ripiego "puro" (nessun manifest sul disco, il caso
       degenere di prima ancora che melting-gen sia mai girato) resta
       deliberatamente comune (il valore zero, gia' impostato da "{0}"
       sopra): non tira dalla tabella di pesi per pool (quella vive nel
       ripiego DI melting-gen, tools/melting-gen/gen_fallback.c, che scrive
       un manifest vero e testabile). Riga esplicita, come "item.kind"
       sopra, solo per leggibilita' -- stesso valore che "{0}" darebbe gia'. */
    item.rarity = RARITY_COMMON;
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
        "Mighty Relic", "Blazing Core", "Vital Seal",
        "Swift Crystal", "Solid Totem", "Great Soul"
    };
    Item item = { 0 };
    item.active = true;
    item.kind = ITEM_STATUP;
    /* Fase 3b review ("il boss non delude mai", decisione del proprietario):
       A DIFFERENZA di MakeFallbackItem sopra (che resta comune di proposito
       -- un oggetto tesoro/negozio di ripiego puro NON deve fingersi raro),
       il bossItem NO. La promessa "il boss da' sempre raro o leggendario"
       (design doc, sezione 3) vale gia' per il contenuto DAVVERO generato
       (il manifest scritto da melting-gen, che tira sempre da
       GEN_RARITY_WEIGHTS_BOSS = {0,0,70,30}, mai comune/non-comune, vedi
       tools/melting-gen/gen_fallback.c): qui si estende lo STESSO principio
       al caso degenere -- ripiego "puro" senza alcun manifest, o un
       manifest VECCHIO (scritto prima della fase 3b) a cui manca la riga
       "bossItem.rarity=" -- cosi' che "il boss non delude mai" valga
       SEMPRE, non solo quando melting-gen e' gia' girato almeno una volta.
       Resta invece invariato il back-compat per un oggetto NORMALE (righe
       "itemN.rarity=" mancanti -> RARITY_COMMON, vedi RarityFromText sopra
       e MakeFallbackItem): quella promessa era solo "niente rarita' alta
       per sbaglio", non "il boss non delude mai", e non riguarda il boss. */
    item.rarity = RARITY_RARE;
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
        "Neon Cellar", "Moldy Library", "Lunar Forge", "Radioactive Aquarium", "Cathedral of Sugar"
    };
    static const char *styles[] = {
        "simple pixel", "dark toon", "stark arcade", "flat ink", "low-fi fantasy"
    };
    float hue = GameRngFloat(rng, 0.0f, 360.0f);
    Theme theme = { 0 };
    snprintf(theme.name, sizeof(theme.name), "%s", themes[(floor - 1)%5]);
    snprintf(theme.style, sizeof(theme.style), "%s", styles[GameRngRange(rng, 0, 4)]);
    snprintf(theme.bossName, sizeof(theme.bossName), "Floor %d Guardian", floor);
    theme.bg = ColorFromHSV(hue, 0.30f, 0.12f);
    theme.floor = ColorFromHSV(fmodf(hue + 20.0f, 360.0f), 0.38f, 0.22f);
    theme.wall = ColorFromHSV(fmodf(hue + 52.0f, 360.0f), 0.55f, 0.45f);
    theme.accent = ColorFromHSV(fmodf(hue + 100.0f, 360.0f), 0.62f, 0.86f);
    theme.accent2 = ColorFromHSV(fmodf(hue + 172.0f, 360.0f), 0.70f, 0.94f);
    theme.enemy = ColorFromHSV(fmodf(hue + 235.0f, 360.0f), 0.58f, 0.82f);
    theme.boss = ColorFromHSV(fmodf(hue + 300.0f, 360.0f), 0.75f, 0.88f);
    return theme;
}

/* M5 (DEC-005), requisito 8: carte-proposta di tema quando la generazione e'
 * disabilitata o bin/melting-gen non c'e' (DEC-002, il gioco resta sempre
 * avviabile SENZA il tool) -- niente processo, niente attesa, le carte sono
 * gia' pronte nello stesso frame in cui si entra nel Piano 0. 5 nomi (lo
 * stesso pool di MakeFallbackTheme sopra, coerenza col resto del contenuto
 * di riserva) + 5 blurb curati dal content designer (logs/m5-content-notes.md
 * (c)-2, gli stessi 5 primi del pool piu' ampio scritto per melting-gen: qui
 * ne bastano 5, un pool a parte in questo modulo sarebbe solo duplicazione).
 * Salt diverso da GenerateFallbackContent sopra (0xBAD51DE): questa funzione
 * NON deve consumare o essere consumata dallo stream della run vera. */
void RunContentMakeFallbackThemeCards(unsigned int seed, ThemeCard *out, int count)
{
    static const char *names[] = {
        "Neon Cellar", "Moldy Library", "Lunar Forge", "Radioactive Aquarium", "Cathedral of Sugar"
    };
    static const char *blurbs[] = {
        "It looks abandoned, until something in the dark decides to notice you.",
        "Every hallway loops back to somewhere it should not.",
        "Something here remembers you, and it is not friendly.",
        "The air hums like it is counting down to something.",
        "Nothing moves until you stop looking directly at it.",
    };
    int poolSize = (int)(sizeof(names)/sizeof(names[0]));   /* == sizeof(blurbs)/sizeof(blurbs[0]) */
    if (count < 1) count = 1;
    if (count > THEME_CARD_MAX) count = THEME_CARD_MAX;
    if (count > poolSize) count = poolSize;

    unsigned int rng = seed ^ 0xF10A7EEDu;
    /* Punto di partenza casuale sul seed, poi 'count' voci CONSECUTIVE
       (ciclando sul pool): distinte per costruzione senza bisogno di
       rejection sampling -- count <= poolSize per il clamp sopra. Nome e
       blurb usano basi INDIPENDENTI (due tiri separati) cosi' la coppia
       nome<->blurb non e' sempre la stessa a ogni run. */
    int nameBase = GameRngRange(&rng, 0, poolSize - 1);
    int blurbBase = GameRngRange(&rng, 0, poolSize - 1);
    for (int i = 0; i < count; i++)
    {
        snprintf(out[i].name, sizeof(out[i].name), "%s", names[(nameBase + i)%poolSize]);
        snprintf(out[i].blurb, sizeof(out[i].blurb), "%s", blurbs[(blurbBase + i)%poolSize]);
    }
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
        /* Step C: UN tipo di colpo per piano, su UNO dei tre oggetti attivi (mai
           sul bossItem: uno stat-up e' solo numeri, vedi la vision doc). Stessa
           regola che segue il contenuto generato dal modello (campo "shotItem"
           del JSON): il ripiego non e' una modalita' di gioco diversa, e' lo
           stesso gioco con contenuti procedurali invece che inventati. */
        int shotOwner = GameRngRange(&rng, 0, 2);
        content->floors[f].items[shotOwner].shotType = MakeFallbackShotType(&rng);
        MakeFallbackEnemies(&content->floors[f], &rng);   /* fase 3b */
        RoomLayoutExample(&content->floors[f].roomLayout, GameRngRange(&rng, 0, ROOM_LAYOUT_EXAMPLE_COUNT - 1));   /* fase 3c */
    }
}

/* Un numero del tipo di colpo dal manifest ("floorN.itemM.<field>="), col suo
   ripiego neutro se la riga manca (1.0 per un moltiplicatore, 0 per una manopola
   discreta): un manifest a cui manca una riga produce un tipo di colpo piu'
   BLANDO, mai uno piu' forte. */
static float ReadShotNumber(const char *text, int floorNum, int itemNum, const char *field, float fallback)
{
    char key[80];
    char value[64];
    snprintf(key, sizeof(key), "floor%d.item%d.%s=", floorNum, itemNum, field);
    value[0] = '\0';
    ReadManifestValue(text, key, value, sizeof(value));
    if (!value[0]) return fallback;
    return (float)atof(value);
}

/* Tipo di colpo di un oggetto dal manifest (step C). La chiave "shotName=" e' la
   sentinella: se manca, l'oggetto NON porta alcun tipo di colpo e la funzione non
   tocca nulla (un manifest scritto prima di questa fase resta valido e produce
   esattamente il gioco di prima -- back-compat, stesso schema "per-key fallback"
   di kind/rarity). Se c'e', il tipo viene ricostruito e passato per
   ShotTypeBalance: e' la SECONDA rete indipendente dopo quella di melting-gen
   (la terza e' ScriptItemsRecomputeStats), perche' un manifest e' un file di
   testo che chiunque puo' modificare a mano. */
static void ReadItemShotType(const char *text, int floorNum, int itemNum, Item *item)
{
    char key[80];
    char value[SCRIPT_TEXT_LEN];

    snprintf(key, sizeof(key), "floor%d.item%d.shotName=", floorNum, itemNum);
    value[0] = '\0';
    ReadManifestValue(text, key, value, sizeof(value));
    if (!value[0]) return;

    ShotTypeDef type;
    memset(&type, 0, sizeof(type));
    type.active = true;
    snprintf(type.name, sizeof(type.name), "%s", value);

    snprintf(key, sizeof(key), "floor%d.item%d.shotForm=", floorNum, itemNum);
    value[0] = '\0';
    ReadManifestValue(text, key, value, sizeof(value));
    type.form = ShotFormFromText(value);   /* testo mancante o sconosciuto -> SHOT_FORM_ORB */

    type.speedMul    = ReadShotNumber(text, floorNum, itemNum, "shotSpeed",  1.0f);
    type.damageMul   = ReadShotNumber(text, floorNum, itemNum, "shotDamage", 1.0f);
    type.radiusMul   = ReadShotNumber(text, floorNum, itemNum, "shotSize",   1.0f);
    type.lifeMul     = ReadShotNumber(text, floorNum, itemNum, "shotLife",   1.0f);
    type.pierceBonus = (int)ReadShotNumber(text, floorNum, itemNum, "shotPierce",  0.0f);
    type.chain       = (int)ReadShotNumber(text, floorNum, itemNum, "shotChain",   0.0f);
    type.pellets     = (int)ReadShotNumber(text, floorNum, itemNum, "shotPellets", 1.0f);

    ShotTypeBalance(&type);
    item->shotType = type;
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

/* Vedi run_content.h per il perche' esiste (step B2). Rilegge dal manifest le sole
   righe "floorN.itemM.lua=" / "floorN.bossItem.lua=" del piano dato e ricarica i
   file che referenziano. Un file mancante o una riga assente NON azzerano lo
   script gia' in memoria: gli script arrivano nel tempo, e questa funzione puo'
   girare mentre il generatore sta ancora scrivendo -- il caso "non c'e' ancora"
   deve essere un no-op, mai una regressione a mini-VM di qualcosa che funzionava. */
void RunContentRefreshFloorScripts(RunContent *content, int floorIndex)
{
    if (!content || floorIndex < 0 || floorIndex >= FLOOR_COUNT) return;

    char *text = LoadFileText("generated/current_run.txt");
    if (!text) return;

    FloorContent *floor = &content->floors[floorIndex];
    int n = floorIndex + 1;
    char key[80];
    char value[SCRIPT_TEXT_LEN];

    for (int i = 0; i <= 3; i++)   /* <= 3: l'ultimo giro e' il bossItem */
    {
        Item *item = (i < 3) ? &floor->items[i] : &floor->bossItem;
        if (i < 3) snprintf(key, sizeof(key), "floor%d.item%d.lua=", n, i + 1);
        else snprintf(key, sizeof(key), "floor%d.bossItem.lua=", n);

        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (!value[0]) continue;

        char *luaText = LoadFileText(value);
        if (!luaText) continue;
        snprintf(item->luaSource, sizeof(item->luaSource), "%s", luaText);
        UnloadFileText(luaText);
    }

    UnloadFileText(text);
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

        /* Step C, invariante "un solo tipo di colpo per piano": il manifest e'
           l'AUTORITA' sui tipi di colpo dei piani che descrive. Si azzerano
           quindi i tipi che il ripiego procedurale (GenerateFallbackContent, gia'
           girato sopra) aveva assegnato, PRIMA di leggere quelli del manifest --
           altrimenti un manifest che mette il tipo di colpo sull'oggetto 2 e un
           ripiego che l'aveva messo sull'oggetto 1 lascerebbero DUE oggetti con
           un tipo di colpo sullo stesso piano.
           Conseguenza voluta: un manifest VECCHIO (scritto prima di questa fase,
           senza righe "shot*") produce un piano SENZA tipi di colpo, cioe'
           esattamente il gioco di prima -- back-compat pieno. Non e' il caso
           "mai un dud": un manifest vecchio non ha contenuti mancanti, ha
           contenuti di una versione precedente, e inventarci sopra un tipo di
           colpo che il suo autore non ha mai scritto sarebbe peggio che non
           averlo. Il "mai un dud" vive nel ripiego SENZA manifest (sopra) e in
           melting-gen (che scrive sempre un tipo per piano, anche nel proprio
           ripiego procedurale). */
        for (int i = 0; i < 3; i++) memset(&floor->items[i].shotType, 0, sizeof(ShotTypeDef));

        /* Fase 3b: i tipi di nemico del piano. Stessa regola dei tipi di colpo -- il
           manifest e' l'AUTORITA': si azzerano quelli del ripiego procedurale e si
           leggono i suoi. Un manifest VECCHIO (senza righe enemy*) lascia quindi il
           piano SENZA tipi, e il gioco usa i nemici storici: esattamente il gioco di
           prima, che e' la cosa giusta (inventare nemici che l'autore di quel
           manifest non ha mai scritto sarebbe peggio che non averli). */
        memset(floor->enemies, 0, sizeof(floor->enemies));
        memset(&floor->bossType, 0, sizeof(floor->bossType));
        for (int i = 0; i < 2; i++)
        {
            snprintf(key, sizeof(key), "floor%d.enemy%d", n, i + 1);
            ReadEnemyType(text, key, false, &floor->enemies[i]);
        }
        snprintf(key, sizeof(key), "floor%d.bossType", n);
        ReadEnemyType(text, key, true, &floor->bossType);

        /* Fase 3c: il layout delle stanze del piano. Azzerato prima (il manifest e'
           l'autorita', come per nemici/tipi di colpo). */
        memset(&floor->roomLayout, 0, sizeof(floor->roomLayout));
        ReadRoomLayout(text, n, &floor->roomLayout);

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

            /* Fase 3b: riga assente (manifest scritto prima di questa fase)
               -> item->rarity resta RARITY_COMMON, gia' impostato da
               MakeFallbackItem sopra (stesso schema "per-key fallback" di
               ogni altro campo qui, vedi il commento su RarityFromText). */
            snprintf(key, sizeof(key), "floor%d.item%d.rarity=", n, i + 1);
            value[0] = '\0';
            ReadManifestValue(text, key, value, sizeof(value));
            if (value[0]) item->rarity = RarityFromText(value);

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

            /* Step C: il tipo di colpo che questo oggetto conferisce, se il
               manifest gliene assegna uno (vedi ReadItemShotType sopra). */
            ReadItemShotType(text, n, i + 1, item);
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

        /* Fase 3b review ("il boss non delude mai"): stesso schema "per-key
           fallback" di sopra -- riga assente -> resta quello che
           MakeFallbackBossItem ha gia' impostato, ora RARITY_RARE (non piu'
           RARITY_COMMON: vedi il commento li' sopra). A differenza degli
           oggetti normali (dove "riga assente -> comune" resta back-compat
           voluto), qui la riga puo' mancare per due motivi ben distinti --
           nessun manifest ancora (ripiego puro) o un manifest VECCHIO
           (scritto prima di questa fase) -- ed entrambi ora ricadono su
           "raro", mai piu' su "comune": il bossItem non deve MAI deludere,
           nemmeno quando melting-gen non ha ancora scritto nulla. */
        snprintf(key, sizeof(key), "floor%d.bossItem.rarity=", n);
        value[0] = '\0';
        ReadManifestValue(text, key, value, sizeof(value));
        if (value[0]) boss->rarity = RarityFromText(value);

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
        /* Step C, invariante di tassonomia (difesa in profondita', come il gate
           su on_fire/on_hit/on_tick per gli stat-up in script_items.c): un
           oggetto stat-up NON cambia mai il modo di sparare -- e' solo numeri
           (vision doc). Non si legge quindi nessuna riga "shot*" per bossItem, e
           qui si azzera il campo esplicitamente: nemmeno un manifest scritto a
           mano puo' dare un tipo di colpo alla ricompensa del boss. */
        memset(&boss->shotType, 0, sizeof(ShotTypeDef));
        boss->active = true;
    }

    content->loaded = loadedSomething;
    UnloadFileText(text);
    PreferPngAtlasIfFresh(content);
}
