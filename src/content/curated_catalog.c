#include "content/curated_catalog.h"

#include "content/run_content.h"
#include "gameplay/item_pool.h"
#include "gameplay/item_traits.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Percorso del catalogo per i test (vedi il commento in curated_catalog.h):
   stessa convenzione di g_testCatalogPath in run_catalog.c. */
static const char *g_testCatalogDir = NULL;

void CuratedCatalogSetTestDir(const char *dir)
{
    g_testCatalogDir = dir;
}

const char *CuratedCatalogGetTestDir(void)
{
    return g_testCatalogDir;
}

/* Stesso schema "strstr fino a fine riga" di ReadManifestValue in
   src/content/run_content.c e src/content/run_catalog.c: una copia PRIVATA
   per modulo e' la convenzione gia' in uso nel progetto (vedi il commento su
   ReadManifestValue in run_catalog.c -- "moduli diversi, ognuno coi propri
   file da leggere"), non una funzione condivisa. 'out' e' sempre azzerata,
   anche quando la chiave non c'e' (mai un valore stantio del giro precedente). */
static void ReadValue(const char *text, const char *key, char *out, int outSize)
{
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!text || !key) return;
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
        (unsigned char)(HexDigit(text[1]) * 16 + HexDigit(text[2])),
        (unsigned char)(HexDigit(text[3]) * 16 + HexDigit(text[4])),
        (unsigned char)(HexDigit(text[5]) * 16 + HexDigit(text[6])),
        255
    };
}

/* Stessi cinque testi di SlotFromText in run_content.c (copia privata,
   stessa convenzione dichiarata sopra): un refuso o uno slot sconosciuto
   ricade su SLOT_HAT, mai un valore inventato fuori dall'enum. */
static ItemSlot SlotFromText(const char *text)
{
    if (strstr(text, "eyes")) return SLOT_EYES;
    if (strstr(text, "hand")) return SLOT_HAND;
    if (strstr(text, "back")) return SLOT_BACK;
    if (strstr(text, "body")) return SLOT_BODY;
    if (strstr(text, "aura")) return SLOT_AURA;
    return SLOT_HAT;
}

/* Un oggetto del pool ("item<N>.*", 1-based). "item<N>.name=" e' la
   sentinella (stesso schema di ReadEnemyType/ReadRoomLayout in
   run_content.c): se manca, il pool finisce a N-1 voci. Le chiavi di
   ricarica si leggono PRIMA del kind, esattamente come ReadItemRecharge in
   run_content.c: e' l'unica cosa che distingue un attivo vero da un passivo
   che dichiarasse per errore "kind=active" senza cariche ne' cooldown. */
static bool ReadOneItem(const char *text, int index, CuratedCatalogItem *out)
{
    char key[64];
    char value[SCRIPT_TEXT_LEN];

    snprintf(key, sizeof(key), "item%d.name=", index);
    ReadValue(text, key, value, sizeof(value));
    if (!value[0]) return false;

    memset(out, 0, sizeof(*out));
    Item *item = &out->item;
    item->active = true;
    /* Precisione esplicita (47 = sizeof-1), come fusion.c su fusedFrom: dice
       al compilatore che il troncamento e' VOLUTO, non un overflow letto a
       caso da un value molto piu' grande (SCRIPT_TEXT_LEN). */
    snprintf(item->name, sizeof(item->name), "%.47s", value);

    snprintf(key, sizeof(key), "item%d.id=", index);
    ReadValue(text, key, out->id, sizeof(out->id));

    snprintf(key, sizeof(key), "item%d.slot=", index);
    ReadValue(text, key, value, sizeof(value));
    item->slot = value[0] ? SlotFromText(value) : SLOT_HAT;

    snprintf(key, sizeof(key), "item%d.traits=", index);
    ReadValue(text, key, value, sizeof(value));
    item->traits = value[0] ? ItemTraitsFromText(value) : 0u;

    snprintf(key, sizeof(key), "item%d.color=", index);
    ReadValue(text, key, value, sizeof(value));
    item->color = ParseHexColor(value, (Color){ 255, 255, 255, 255 });

    bool declaresRecharge = false;
    snprintf(key, sizeof(key), "item%d.charges=", index);
    ReadValue(text, key, value, sizeof(value));
    if (value[0]) item->charges = atoi(value);
    if (item->charges > 0) declaresRecharge = true;

    snprintf(key, sizeof(key), "item%d.cooldown=", index);
    ReadValue(text, key, value, sizeof(value));
    if (value[0]) item->cooldown = (float)atof(value);
    if (item->cooldown > 0.0f) declaresRecharge = true;

    snprintf(key, sizeof(key), "item%d.chargeRoom=", index);
    ReadValue(text, key, value, sizeof(value));
    item->chargeGainRoom = value[0] ? atoi(value) : 0;

    snprintf(key, sizeof(key), "item%d.chargeEnergy=", index);
    ReadValue(text, key, value, sizeof(value));
    item->chargeGainEnergy = value[0] ? atoi(value) : 0;

    /* Testo assente/sconosciuto -> ITEM_PASSIVE (ItemKindFromText, run_content.c). */
    snprintf(key, sizeof(key), "item%d.kind=", index);
    ReadValue(text, key, value, sizeof(value));
    item->kind = ItemKindFromText(value, declaresRecharge);

    /* Testo assente/sconosciuto -> RARITY_COMMON (RarityFromText, run_content.c). */
    snprintf(key, sizeof(key), "item%d.rarity=", index);
    ReadValue(text, key, value, sizeof(value));
    item->rarity = RarityFromText(value);

    snprintf(key, sizeof(key), "item%d.script=", index);
    ReadValue(text, key, item->script, sizeof(item->script));

    snprintf(key, sizeof(key), "item%d.lua=", index);
    ReadValue(text, key, value, sizeof(value));
    if (value[0])
    {
        char *luaText = LoadFileText(value);
        if (luaText)
        {
            snprintf(item->luaSource, sizeof(item->luaSource), "%s", luaText);
            UnloadFileText(luaText);
        }
    }

    /* Difesa in profondita' di tassonomia (stessa di RunContentLoad,
       run_content.c): uno stat-up non ha mai comportamento mini-VM. */
    if (item->kind == ITEM_STATUP) item->script[0] = '\0';

    return true;
}

/* Un nemico/boss del pool ("<prefix><N>.*", 1-based). Stesso vocabolario di
   ReadEnemyType in run_content.c (EnemyFormFromText/EnemyMoveFromText/
   EnemyFireFromText, core/enemy_type.h), passato per EnemyTypeBalance qui
   dentro cosi' ogni voce del pool esce gia' in banda -- la stessa rete che
   protegge il contenuto generato dal modello protegge anche il contenuto
   curato scritto a mano. */
static bool ReadOneEnemy(const char *text, const char *prefix, int index, bool isBoss, CuratedCatalogEnemy *out)
{
    char key[64];
    char value[SCRIPT_TEXT_LEN];

    snprintf(key, sizeof(key), "%s%d.name=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    if (!value[0]) return false;

    memset(out, 0, sizeof(*out));
    EnemyTypeDef *type = &out->def;
    type->active = true;
    type->boss = isBoss;
    /* Precisione esplicita (31 = sizeof-1), stessa convenzione di sopra. */
    snprintf(type->name, sizeof(type->name), "%.31s", value);

    snprintf(key, sizeof(key), "%s%d.id=", prefix, index);
    ReadValue(text, key, out->id, sizeof(out->id));

    snprintf(key, sizeof(key), "%s%d.form=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->form = EnemyFormFromText(value);

    snprintf(key, sizeof(key), "%s%d.move=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->move = EnemyMoveFromText(value);

    snprintf(key, sizeof(key), "%s%d.fire=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->fire = EnemyFireFromText(value);

    snprintf(key, sizeof(key), "%s%d.hp=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->hpMul = value[0] ? (float)atof(value) : 1.0f;

    snprintf(key, sizeof(key), "%s%d.speed=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->speedMul = value[0] ? (float)atof(value) : 1.0f;

    snprintf(key, sizeof(key), "%s%d.size=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->sizeMul = value[0] ? (float)atof(value) : 1.0f;

    snprintf(key, sizeof(key), "%s%d.rate=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->fireRate = value[0] ? (float)atof(value) : 0.0f;

    snprintf(key, sizeof(key), "%s%d.pellets=", prefix, index);
    ReadValue(text, key, value, sizeof(value));
    type->pellets = value[0] ? atoi(value) : 1;

    EnemyTypeBalance(type);
    return true;
}

static void LoadItemsFile(const char *path, CuratedCatalogPool *out)
{
    char *text = LoadFileText(path);
    if (!text) return;
    for (int i = 1; i <= CURATED_CATALOG_ITEM_MAX; i++)
    {
        CuratedCatalogItem entry;
        if (!ReadOneItem(text, i, &entry)) break;
        out->items[out->itemCount++] = entry;
    }
    UnloadFileText(text);
}

static void LoadEnemiesFile(const char *path, const char *prefix, bool isBoss,
                             CuratedCatalogEnemy *arr, int maxCount, int *count)
{
    char *text = LoadFileText(path);
    if (!text) return;
    for (int i = 1; i <= maxCount; i++)
    {
        CuratedCatalogEnemy entry;
        if (!ReadOneEnemy(text, prefix, i, isBoss, &entry)) break;
        arr[(*count)++] = entry;
    }
    UnloadFileText(text);
}

bool CuratedCatalogLoad(const char *dirPath, CuratedCatalogPool *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    if (!dirPath || !dirPath[0]) return false;

    char path[256];
    snprintf(path, sizeof(path), "%s/items.txt", dirPath);
    LoadItemsFile(path, out);
    if (out->itemCount <= 0)
    {
        memset(out, 0, sizeof(*out));   /* niente pool utilizzabile: mai una struttura a meta' */
        return false;
    }

    snprintf(path, sizeof(path), "%s/enemies.txt", dirPath);
    LoadEnemiesFile(path, "enemy", false, out->enemies, CURATED_CATALOG_ENEMY_MAX, &out->enemyCount);

    snprintf(path, sizeof(path), "%s/bosses.txt", dirPath);
    LoadEnemiesFile(path, "boss", true, out->bosses, CURATED_CATALOG_BOSS_MAX, &out->bossCount);

    return true;
}

bool CuratedCatalogValidateFloor(const CuratedCatalogPool *pool)
{
    if (!pool) return false;

    int expected[ITEM_POOL_RARITY_COUNT];
    ItemPoolMinimumCounts(pool->itemCount, ItemPoolWeightsStandard, expected);

    int actual[ITEM_POOL_RARITY_COUNT] = { 0 };
    for (int i = 0; i < pool->itemCount; i++) actual[(int)pool->items[i].item.rarity]++;

    static const char *names[ITEM_POOL_RARITY_COUNT] = { "comune", "non-comune", "rara", "leggendaria" };
    bool ok = true;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
    {
        if (expected[r] > 0 && actual[r] <= 0)
        {
            fprintf(stderr,
                    "CuratedCatalog: floor DEC-144 violato -- il pool curato di %d oggetti non ne ha nessuno di rarita' %s\n",
                    pool->itemCount, names[r]);
            ok = false;
        }
    }
    return ok;
}

const CuratedCatalogItem *CuratedCatalogPickItem(const CuratedCatalogPool *pool, Rarity rarity, unsigned int roll)
{
    if (!pool || pool->itemCount <= 0) return NULL;

    int indices[CURATED_CATALOG_ITEM_MAX];
    int count = 0;
    for (int i = 0; i < pool->itemCount; i++)
        if (pool->items[i].item.rarity == rarity) indices[count++] = i;
    if (count <= 0) return NULL;

    return &pool->items[indices[roll % (unsigned int)count]];
}
