#include "game/run_suspend.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "script/script_items.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WP17 (DEC-050) -- vedi il commento di apertura in run_suspend.h per il
   contratto e per il formato. Qui solo il "come". */

static const char *g_testSuspendPath = NULL;

void RunSuspendSetTestPath(const char *path) { g_testSuspendPath = path; }
const char *RunSuspendGetTestPath(void) { return g_testSuspendPath; }

static const char *SuspendDir(void)
{
    return g_testSuspendPath ? g_testSuspendPath : "suspend";
}

/* DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019): un solo file, sempre
   lo stesso nome -- una sospensione per profilo, che e' esattamente quello che
   "Continua" sa esprimere (una voce sola, nessuna lista di salvataggi). */
static void SuspendFilePath(char *out, int outSize)
{
    snprintf(out, (size_t)outSize, "%s/current.txt", SuspendDir());
}

/* Maschera dei flag MUTABILI di una cella di stato (RoomState): tutto il resto
   -- forma, porte, archetipo, livello della segreta -- si rigenera dal seed e
   non viene mai scritto qui. Bit in coda come ogni altro enum del motore. */
#define SUSPEND_ROOM_VISITED       0x01u
#define SUSPEND_ROOM_CLEARED       0x02u
#define SUSPEND_ROOM_REWARD_TAKEN  0x04u
#define SUSPEND_ROOM_ARENA_ACTIVE  0x08u
#define SUSPEND_ROOM_SECRET_OPENED 0x10u

/* ============================================================
   Scrittura: un valore per riga, sempre.
   ============================================================ */

/* Un valore di sospensione e' SEMPRE una riga sola (lo stesso schema
   chiave=valore/fino-a-fine-riga di ogni manifest del progetto): il sorgente
   Lua di un oggetto non lo e' quasi mai, e nemmeno un nome forgiato a mano.
   Copia PRIVATA di WriteEscapedValue (src/content/run_catalog.c): la
   convenzione del progetto e' una copia per modulo, ognuno coi propri file da
   leggere (vedi il commento su ParseHexColor in character_proposal.c). */
static void WriteEscaped(FILE *f, const char *key, const char *text)
{
    fputs(key, f);
    fputc('=', f);
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); *p; p++)
    {
        if (*p == '\\') fputs("\\\\", f);
        else if (*p == '\n') fputs("\\n", f);
        else if (*p == '\r') continue;
        else fputc((int)*p, f);
    }
    fputc('\n', f);
}

static void WriteShotType(FILE *f, const char *prefix, const ShotTypeDef *s)
{
    char key[96];
    snprintf(key, sizeof(key), "%s.active", prefix);   fprintf(f, "%s=%d\n", key, s->active ? 1 : 0);
    if (!s->active) return;
    snprintf(key, sizeof(key), "%s.name", prefix);     WriteEscaped(f, key, s->name);
    fprintf(f, "%s.form=%d\n", prefix, (int)s->form);
    fprintf(f, "%s.speedMul=%.6f\n", prefix, (double)s->speedMul);
    fprintf(f, "%s.damageMul=%.6f\n", prefix, (double)s->damageMul);
    fprintf(f, "%s.radiusMul=%.6f\n", prefix, (double)s->radiusMul);
    fprintf(f, "%s.lifeMul=%.6f\n", prefix, (double)s->lifeMul);
    fprintf(f, "%s.pierceBonus=%d\n", prefix, s->pierceBonus);
    fprintf(f, "%s.chain=%d\n", prefix, s->chain);
    fprintf(f, "%s.pellets=%d\n", prefix, s->pellets);
}

/* L'oggetto INTERO, sorgente Lua compresa. Non "per nome dal manifest": un
   oggetto FUSO (DEC-023) non esiste in nessun manifest -- nasce a runtime da
   FusionPerform -- quindi una ricostruzione per nome lo perderebbe in
   silenzio, che e' il difetto peggiore possibile per un inventario. */
static void WriteItemFields(FILE *f, const char *prefix, const Item *it)
{
    char key[96];
    snprintf(key, sizeof(key), "%s.name", prefix);      WriteEscaped(f, key, it->name);
    fprintf(f, "%s.slot=%d\n", prefix, (int)it->slot);
    fprintf(f, "%s.kind=%d\n", prefix, (int)it->kind);
    fprintf(f, "%s.rarity=%d\n", prefix, (int)it->rarity);
    fprintf(f, "%s.traits=%u\n", prefix, it->traits);
    fprintf(f, "%s.color=%u,%u,%u,%u\n", prefix,
            (unsigned)it->color.r, (unsigned)it->color.g, (unsigned)it->color.b, (unsigned)it->color.a);
    fprintf(f, "%s.shape=%d\n", prefix, it->shape);
    snprintf(key, sizeof(key), "%s.script", prefix);    WriteEscaped(f, key, it->script);
    snprintf(key, sizeof(key), "%s.lua", prefix);       WriteEscaped(f, key, it->luaSource);
    fprintf(f, "%s.charges=%d\n", prefix, it->charges);
    fprintf(f, "%s.cooldown=%.6f\n", prefix, (double)it->cooldown);
    fprintf(f, "%s.chargeGainRoom=%d\n", prefix, it->chargeGainRoom);
    fprintf(f, "%s.chargeGainEnergy=%d\n", prefix, it->chargeGainEnergy);
    fprintf(f, "%s.chargeNow=%d\n", prefix, it->chargeNow);
    fprintf(f, "%s.cooldownTimer=%.6f\n", prefix, (double)it->cooldownTimer);
    snprintf(key, sizeof(key), "%s.imagePath", prefix); WriteEscaped(f, key, it->imagePath);
    snprintf(key, sizeof(key), "%s.imageId", prefix);   WriteEscaped(f, key, it->imageId);
    snprintf(key, sizeof(key), "%s.fusedFrom0", prefix); WriteEscaped(f, key, it->fusedFrom[0]);
    snprintf(key, sizeof(key), "%s.fusedFrom1", prefix); WriteEscaped(f, key, it->fusedFrom[1]);
    char shotPrefix[96];
    snprintf(shotPrefix, sizeof(shotPrefix), "%s.shotType", prefix);
    WriteShotType(f, shotPrefix, &it->shotType);
}

bool RunSuspendWrite(const Game *game)
{
    if (!game) return false;
    /* Il Piano 0 non e' sospendibile in questa fetta: LIMITE DICHIARATO
       (docs/engineering/known-issues.md), non un requisito scartato -- il
       documento prevede anche `FloorZero` come stato salvato, ma quello stato
       comprende la generazione in corso e le carte-proposta, che nessun file
       di run puo' ricostruire da solo. Stessa soglia "floor >= 1" gia' usata
       da WP19 per distinguere una run vera dalla sola preparazione. */
    if (game->floor < 1 || game->floor > FLOOR_COUNT) return false;

    const char *dir = SuspendDir();
    if (!DirectoryExists(dir) && MakeDirectory(dir) != 0)
    {
        fprintf(stderr, "RunSuspend: impossibile creare %s, run non sospesa\n", dir);
        return false;
    }

    char finalPath[192];
    char tmpPath[208];
    SuspendFilePath(finalPath, sizeof(finalPath));
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", finalPath);

    FILE *f = fopen(tmpPath, "w");
    if (!f)
    {
        fprintf(stderr, "RunSuspend: impossibile scrivere %s, run non sospesa\n", tmpPath);
        return false;
    }

    /* Intestazione: la riga di VERSIONE per prima, cosi' un lettore puo'
       rifiutare il file senza analizzare altro. */
    fprintf(f, "suspendSchema=%d\n", RUN_SUSPEND_SCHEMA);
    fprintf(f, "runSeed=%u\n", game->runSeed);
    fprintf(f, "rngNow=%u\n", game->rng);
    fprintf(f, "rngFloorEntry=%u\n", game->floorEntryRng);
    fprintf(f, "floor=%d\n", game->floor);
    fprintf(f, "roomX=%d\n", game->roomX);
    fprintf(f, "roomY=%d\n", game->roomY);
    fprintf(f, "roomNumber=%d\n", game->roomNumber);
    fprintf(f, "score=%d\n", game->score);
    fprintf(f, "fusionCount=%d\n", game->fusionCount);
    fprintf(f, "runTime=%.6f\n", (double)game->runElapsedSeconds);
    fprintf(f, "floorEntryTime=%.6f\n", (double)game->floorEntryElapsedSeconds);
    fprintf(f, "treasureLuckStreak=%d\n", game->treasureLuckStreak);
    fprintf(f, "shopLuckStreak=%d\n", game->shopLuckStreak);
    fprintf(f, "timedRoomEverGenerated=%d\n", game->timedRoomEverGenerated ? 1 : 0);
    fprintf(f, "secretRoomEverGenerated=%d\n", game->secretRoomEverGenerated ? 1 : 0);
    fprintf(f, "arenaRoomEverGenerated=%d\n", game->arenaRoomEverGenerated ? 1 : 0);
    fprintf(f, "currentBossFightDamaged=%d\n", game->currentBossFightDamaged ? 1 : 0);
    fprintf(f, "pourhouseLastSignature=%u\n", game->pourhouseLastSignature);

    /* Personaggio: l'indice basta per la rosa curata (i tre CharacterDef sono
       costanti del binario), la definizione COMPLETA serve solo per il quarto
       slot generato per la run, che vive in generated/ e potrebbe non esserci
       piu' al rientro. */
    fprintf(f, "characterIndex=%d\n", game->characterChosenIndex);
    bool generatedChosen = (game->characterChosenIndex == CHARACTER_COUNT && game->generatedCharacterValid);
    fprintf(f, "character.valid=%d\n", generatedChosen ? 1 : 0);
    if (generatedChosen)
    {
        const CharacterDef *c = &game->generatedCharacter;
        WriteEscaped(f, "character.name", c->name);
        WriteEscaped(f, "character.role", c->role);
        WriteEscaped(f, "character.blurb", c->blurb);
        fprintf(f, "character.baseDamage=%.6f\n", (double)c->baseDamage);
        fprintf(f, "character.baseFireDelay=%.6f\n", (double)c->baseFireDelay);
        fprintf(f, "character.baseShotSpeed=%.6f\n", (double)c->baseShotSpeed);
        fprintf(f, "character.baseShotRadius=%.6f\n", (double)c->baseShotRadius);
        fprintf(f, "character.baseSpeed=%.6f\n", (double)c->baseSpeed);
        fprintf(f, "character.baseMaxHp=%d\n", c->baseMaxHp);
        fprintf(f, "character.hpCap=%d\n", c->hpCap);
        fprintf(f, "character.baseLuck=%.6f\n", (double)c->baseLuck);
        fprintf(f, "character.palette=%u,%u,%u,%u\n",
                (unsigned)c->palette.r, (unsigned)c->palette.g, (unsigned)c->palette.b, (unsigned)c->palette.a);
        WriteEscaped(f, "character.traitHook", c->traitHook);
        WriteShotType(f, "character.signatureShot", &c->signatureShot);
    }

    const Player *p = &game->player;
    fprintf(f, "player.hp=%d\n", p->hp);
    fprintf(f, "player.tempHp=%d\n", p->tempHp);
    fprintf(f, "player.coins=%d\n", p->coins);
    fprintf(f, "player.bombs=%d\n", p->bombs);
    fprintf(f, "player.keys=%d\n", p->keys);
    fprintf(f, "player.flux=%d\n", p->flux);
    fprintf(f, "player.activeSlotCount=%d\n", p->activeSlotCount);
    fprintf(f, "player.graftSlotCount=%d\n", p->graftSlotCount);
    fprintf(f, "player.activeSelected=%d\n", p->activeSelected);
    fprintf(f, "player.baseDamage=%.6f\n", (double)p->baseDamage);
    fprintf(f, "player.baseFireDelay=%.6f\n", (double)p->baseFireDelay);
    fprintf(f, "player.baseShotSpeed=%.6f\n", (double)p->baseShotSpeed);
    fprintf(f, "player.baseShotRadius=%.6f\n", (double)p->baseShotRadius);
    fprintf(f, "player.baseSpeed=%.6f\n", (double)p->baseSpeed);
    fprintf(f, "player.baseMaxHp=%d\n", p->baseMaxHp);
    fprintf(f, "player.baseLuck=%.6f\n", (double)p->baseLuck);
    fprintf(f, "player.hpCap=%d\n", p->hpCap);

    int itemCount = GameMathClampInt(p->itemCount, 0, MAX_ITEMS);
    fprintf(f, "item.count=%d\n", itemCount);
    for (int i = 0; i < itemCount; i++)
    {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "item%d", i + 1);
        WriteItemFields(f, prefix, &p->items[i]);
    }

    /* DEC-183: gli Innesti lasciati a terra restano recuperabili per tutta la
       run. Senza questi record una sospensione li perderebbe (la lista si
       azzera in WorldGenerateFloorMap, che la ripresa richiama per rigenerare
       il piano): un oggetto posseduto che sparisce in silenzio. */
    int graftCount = 0;
    for (int i = 0; i < MAX_DROPPED_GRAFTS; i++) if (game->droppedGrafts[i].active) graftCount++;
    fprintf(f, "graft.count=%d\n", graftCount);
    int graftWritten = 0;
    for (int i = 0; i < MAX_DROPPED_GRAFTS; i++)
    {
        const DroppedGraftRecord *g = &game->droppedGrafts[i];
        if (!g->active) continue;
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "graft%d", ++graftWritten);
        fprintf(f, "%s.roomX=%d\n", prefix, g->roomX);
        fprintf(f, "%s.roomY=%d\n", prefix, g->roomY);
        fprintf(f, "%s.posX=%.6f\n", prefix, (double)g->pos.x);
        fprintf(f, "%s.posY=%.6f\n", prefix, (double)g->pos.y);
        char itemPrefix[48];
        snprintf(itemPrefix, sizeof(itemPrefix), "%s.item", prefix);
        WriteItemFields(f, itemPrefix, &g->item);
    }

    /* WP7 (DEC-044): la puntata per INTERO, offerta compresa. Salvare solo
       "accettata si'/no" non basterebbe -- WorldPourhousePrepareRoom non
       ricompone una puntata gia' valida, quindi il banco mostrerebbe
       un'offerta azzerata. */
    const PourhouseWager *w = &game->pourhouse;
    fprintf(f, "pourhouse.composed=%d\n", w->composed ? 1 : 0);
    fprintf(f, "pourhouse.valid=%d\n", w->valid ? 1 : 0);
    fprintf(f, "pourhouse.accepted=%d\n", w->accepted ? 1 : 0);
    fprintf(f, "pourhouse.roomX=%d\n", w->roomX);
    fprintf(f, "pourhouse.roomY=%d\n", w->roomY);
    fprintf(f, "pourhouse.offerKind=%d\n", (int)w->offerKind);
    fprintf(f, "pourhouse.offerAmount=%d\n", w->offerAmount);
    fprintf(f, "pourhouse.offerKeys=%d\n", w->offerKeys);
    fprintf(f, "pourhouse.priceKind=%d\n", (int)w->priceKind);
    fprintf(f, "pourhouse.priceAmount=%d\n", w->priceAmount);
    WriteEscaped(f, "pourhouse.priceItemName", w->priceItemName);
    fprintf(f, "pourhouse.priceItemRarity=%d\n", (int)w->priceItemRarity);
    fprintf(f, "pourhouse.offerValue=%d\n", w->offerValue);
    fprintf(f, "pourhouse.priceValue=%d\n", w->priceValue);
    WriteItemFields(f, "pourhouse.offerItem", &w->offerItem);

    /* WP16 (DEC-042): le prove si riassegnano identiche dal seed
       (TrialsAssignForRun), ma il loro STATO no -- e si scrive tutto lo stesso,
       cosi' il record resta autosufficiente e verificabile campo per campo. */
    int trialCount = GameMathClampInt(game->trialCount, 0, TRIAL_SLOTS_MAX);
    fprintf(f, "trial.count=%d\n", trialCount);
    for (int i = 0; i < trialCount; i++)
    {
        const Trial *t = &game->trials[i];
        char key[48];
        fprintf(f, "trial%d.kind=%d\n", i + 1, (int)t->kind);
        fprintf(f, "trial%d.state=%d\n", i + 1, (int)t->state);
        fprintf(f, "trial%d.param=%d\n", i + 1, t->param);
        fprintf(f, "trial%d.bonus=%d\n", i + 1, t->bonus);
        snprintf(key, sizeof(key), "trial%d.text", i + 1);
        WriteEscaped(f, key, t->text);
    }

    /* M7: chi e' stato DAVVERO incontrato -- e' cio' che il catalogo scrivera'
       a fine run, quindi una sospensione che lo perdesse cancellerebbe scoperte
       gia' fatte. */
    for (int i = 0; i < FLOOR_COUNT; i++)
    {
        fprintf(f, "enc%d.enemy1=%d\n", i + 1, game->enemyEncountered[i][0] ? 1 : 0);
        fprintf(f, "enc%d.enemy2=%d\n", i + 1, game->enemyEncountered[i][1] ? 1 : 0);
        fprintf(f, "enc%d.boss=%d\n", i + 1, game->bossEncountered[i] ? 1 : 0);
        fprintf(f, "enc%d.bossDefeated=%d\n", i + 1, game->bossDefeated[i] ? 1 : 0);
    }

    /* Stato mutabile della griglia del piano CORRENTE: solo le celle con
       qualcosa da dire (una cella assente vale zero, cioe' "mai vista, non
       ripulita, premio non preso" -- lo zero-default piu' innocuo). */
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            const RoomState *r = &game->rooms[y][x];
            unsigned int mask = 0u;
            if (r->visited) mask |= SUSPEND_ROOM_VISITED;
            if (r->cleared) mask |= SUSPEND_ROOM_CLEARED;
            if (r->rewardTaken) mask |= SUSPEND_ROOM_REWARD_TAKEN;
            if (r->arenaActive) mask |= SUSPEND_ROOM_ARENA_ACTIVE;
            if (r->secretOpened) mask |= SUSPEND_ROOM_SECRET_OPENED;
            if (mask != 0u) fprintf(f, "room.%d.%d=%u\n", y, x, mask);
            if (game->destroyedObstacleMask[y][x] != 0)
                fprintf(f, "destroyed.%d.%d=%u\n", y, x, (unsigned)game->destroyedObstacleMask[y][x]);
        }

    fclose(f);
    if (rename(tmpPath, finalPath) != 0)
    {
        fprintf(stderr, "RunSuspend: rename fallita per %s, run non sospesa\n", finalPath);
        remove(tmpPath);
        return false;
    }
    return true;
}

/* ============================================================
   Lettura: stesso schema "chiave ANCORATA a inizio riga" di
   ReadCatalogLineValue (src/content/run_catalog.c) -- necessario perche' in
   questo formato alcune chiavi sono il suffisso letterale di un'altra
   ("item1.name=" dentro "graft1.item1..." non esiste, ma "character.name="
   e' un suffisso di "pourhouse.character.name=" in ogni estensione futura):
   ancorare a "\n" rende impossibile per costruzione che un valore o una
   chiave piu' lunga rispondano al posto di quella giusta. La primissima riga
   ("suspendSchema=") e' l'unica senza '\n' davanti, letta a parte.
   ============================================================ */

static void ReadRawAt(const char *start, char *out, int outSize)
{
    int i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1)
    {
        out[i] = start[i];
        i++;
    }
    out[i] = '\0';
}

/* Valore GREZZO (ancora escapato) della chiave, o stringa vuota se assente. */
static void ReadRaw(const char *text, const char *key, char *out, int outSize)
{
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!text || !key) return;
    char anchored[160];
    snprintf(anchored, sizeof(anchored), "\n%s=", key);
    const char *start = strstr(text, anchored);
    if (!start) return;
    ReadRawAt(start + strlen(anchored), out, outSize);
}

/* Inverso di WriteEscaped: "\\n" -> newline vero, "\\\\" -> backslash. */
static void Unescape(const char *escaped, char *out, int outSize)
{
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!escaped) return;
    int w = 0;
    for (const char *p = escaped; *p && w < outSize - 1; p++)
    {
        if (p[0] == '\\' && p[1] == 'n') { out[w++] = '\n'; p++; }
        else if (p[0] == '\\' && p[1] == '\\') { out[w++] = '\\'; p++; }
        else out[w++] = *p;
    }
    out[w] = '\0';
}

/* Stringa gia' de-escapata. Il buffer intermedio e' grande quanto il piu'
   lungo valore del formato (il sorgente Lua di un oggetto, SCRIPT_LUA_LEN,
   che escapato puo' raddoppiare). */
static void ReadStr(const char *text, const char *key, char *out, int outSize)
{
    static char raw[SCRIPT_LUA_LEN*2 + 8];
    ReadRaw(text, key, raw, (int)sizeof(raw));
    Unescape(raw, out, outSize);
}

static bool HasKey(const char *text, const char *key)
{
    char anchored[160];
    snprintf(anchored, sizeof(anchored), "\n%s=", key);
    return text && strstr(text, anchored) != NULL;
}

static int ReadInt(const char *text, const char *key, int fallback)
{
    char raw[64];
    ReadRaw(text, key, raw, (int)sizeof(raw));
    if (!raw[0]) return fallback;
    return atoi(raw);
}

static unsigned int ReadUInt(const char *text, const char *key, unsigned int fallback)
{
    char raw[64];
    ReadRaw(text, key, raw, (int)sizeof(raw));
    if (!raw[0]) return fallback;
    return (unsigned int)strtoul(raw, NULL, 10);
}

static float ReadFloat(const char *text, const char *key, float fallback)
{
    char raw[64];
    ReadRaw(text, key, raw, (int)sizeof(raw));
    if (!raw[0]) return fallback;
    return (float)atof(raw);
}

static bool ReadBool(const char *text, const char *key)
{
    return ReadInt(text, key, 0) != 0;
}

static Color ReadColor(const char *text, const char *key)
{
    char raw[64];
    ReadRaw(text, key, raw, (int)sizeof(raw));
    unsigned int r = 0, g = 0, b = 0, a = 0;
    if (sscanf(raw, "%u,%u,%u,%u", &r, &g, &b, &a) != 4) return (Color){ 0, 0, 0, 0 };
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
}

static void ReadShotType(const char *text, const char *prefix, ShotTypeDef *out)
{
    char key[112];
    memset(out, 0, sizeof(*out));
    snprintf(key, sizeof(key), "%s.active", prefix);
    if (!ReadBool(text, key)) return;
    out->active = true;
    snprintf(key, sizeof(key), "%s.name", prefix);        ReadStr(text, key, out->name, (int)sizeof(out->name));
    snprintf(key, sizeof(key), "%s.form", prefix);        out->form = (ShotForm)GameMathClampInt(ReadInt(text, key, 0), 0, SHOT_FORM_COUNT - 1);
    snprintf(key, sizeof(key), "%s.speedMul", prefix);    out->speedMul = ReadFloat(text, key, 1.0f);
    snprintf(key, sizeof(key), "%s.damageMul", prefix);   out->damageMul = ReadFloat(text, key, 1.0f);
    snprintf(key, sizeof(key), "%s.radiusMul", prefix);   out->radiusMul = ReadFloat(text, key, 1.0f);
    snprintf(key, sizeof(key), "%s.lifeMul", prefix);     out->lifeMul = ReadFloat(text, key, 1.0f);
    snprintf(key, sizeof(key), "%s.pierceBonus", prefix); out->pierceBonus = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.chain", prefix);       out->chain = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.pellets", prefix);     out->pellets = ReadInt(text, key, 0);
    ShotTypeClamp(out);   /* un file forgiato a mano non deve poter produrre un colpo fuori banda */
}

static void ReadItemFields(const char *text, const char *prefix, Item *out)
{
    char key[112];
    memset(out, 0, sizeof(*out));
    snprintf(key, sizeof(key), "%s.name", prefix);   ReadStr(text, key, out->name, (int)sizeof(out->name));
    snprintf(key, sizeof(key), "%s.slot", prefix);   out->slot = (ItemSlot)GameMathClampInt(ReadInt(text, key, 0), 0, (int)SLOT_AURA);
    snprintf(key, sizeof(key), "%s.kind", prefix);   out->kind = (ItemKind)GameMathClampInt(ReadInt(text, key, 0), 0, (int)ITEM_GRAFT);
    snprintf(key, sizeof(key), "%s.rarity", prefix); out->rarity = (Rarity)GameMathClampInt(ReadInt(text, key, 0), 0, (int)RARITY_LEGENDARY);
    snprintf(key, sizeof(key), "%s.traits", prefix); out->traits = ReadUInt(text, key, 0u);
    snprintf(key, sizeof(key), "%s.color", prefix);  out->color = ReadColor(text, key);
    snprintf(key, sizeof(key), "%s.shape", prefix);  out->shape = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.script", prefix); ReadStr(text, key, out->script, (int)sizeof(out->script));
    snprintf(key, sizeof(key), "%s.lua", prefix);    ReadStr(text, key, out->luaSource, (int)sizeof(out->luaSource));
    snprintf(key, sizeof(key), "%s.charges", prefix);          out->charges = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.cooldown", prefix);         out->cooldown = ReadFloat(text, key, 0.0f);
    snprintf(key, sizeof(key), "%s.chargeGainRoom", prefix);   out->chargeGainRoom = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.chargeGainEnergy", prefix); out->chargeGainEnergy = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.chargeNow", prefix);        out->chargeNow = ReadInt(text, key, 0);
    snprintf(key, sizeof(key), "%s.cooldownTimer", prefix);    out->cooldownTimer = ReadFloat(text, key, 0.0f);
    snprintf(key, sizeof(key), "%s.imagePath", prefix); ReadStr(text, key, out->imagePath, (int)sizeof(out->imagePath));
    snprintf(key, sizeof(key), "%s.imageId", prefix);   ReadStr(text, key, out->imageId, (int)sizeof(out->imageId));
    snprintf(key, sizeof(key), "%s.fusedFrom0", prefix); ReadStr(text, key, out->fusedFrom[0], (int)sizeof(out->fusedFrom[0]));
    snprintf(key, sizeof(key), "%s.fusedFrom1", prefix); ReadStr(text, key, out->fusedFrom[1], (int)sizeof(out->fusedFrom[1]));
    char shotPrefix[96];
    snprintf(shotPrefix, sizeof(shotPrefix), "%s.shotType", prefix);
    ReadShotType(text, shotPrefix, &out->shotType);
    out->active = true;
}

/* Carica il file e ne verifica la compatibilita'. Ritorna il testo (da
   liberare con UnloadFileText) o NULL: schema assente/diverso, piano o stanza
   fuori banda, stati RNG nulli (GameRngNext non produce MAI zero, quindi uno
   zero li' significa file forgiato o troncato). "Mai un crash, mai una
   ricostruzione parziale": o il file e' buono, o non esiste. */
static char *LoadValidSuspendText(void)
{
    char path[192];
    SuspendFilePath(path, sizeof(path));
    if (!FileExists(path)) return NULL;
    char *text = LoadFileText(path);
    if (!text) return NULL;

    char schema[32];
    schema[0] = '\0';
    if (strncmp(text, "suspendSchema=", 14) == 0) ReadRawAt(text + 14, schema, (int)sizeof(schema));
    if (atoi(schema) != RUN_SUSPEND_SCHEMA)
    {
        fprintf(stderr, "RunSuspend: %s ha schema \"%s\" invece di %d, sospensione ignorata\n",
                path, schema, RUN_SUSPEND_SCHEMA);
        UnloadFileText(text);
        return NULL;
    }

    int floor = ReadInt(text, "floor", 0);
    int roomX = ReadInt(text, "roomX", -1);
    int roomY = ReadInt(text, "roomY", -1);
    unsigned int rngNow = ReadUInt(text, "rngNow", 0u);
    unsigned int rngFloorEntry = ReadUInt(text, "rngFloorEntry", 0u);
    bool ok = floor >= 1 && floor <= FLOOR_COUNT
           && roomX >= 0 && roomX < GRID_SIZE
           && roomY >= 0 && roomY < GRID_SIZE
           && rngNow != 0u && rngFloorEntry != 0u
           && HasKey(text, "runSeed");
    if (!ok)
    {
        fprintf(stderr, "RunSuspend: %s incompleto o fuori banda, sospensione ignorata\n", path);
        UnloadFileText(text);
        return NULL;
    }
    return text;
}

bool RunSuspendIsAvailable(void)
{
    char *text = LoadValidSuspendText();
    if (!text) return false;
    UnloadFileText(text);
    return true;
}

void RunSuspendClear(void)
{
    char path[192];
    char tmpPath[208];
    SuspendFilePath(path, sizeof(path));
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
    remove(path);
    remove(tmpPath);   /* un tmp rimasto da una scrittura interrotta non deve sopravvivere alla cancellazione */
}

bool RunSuspendResume(Game *game)
{
    if (!game) return false;
    char *text = LoadValidSuspendText();
    if (!text) return false;

    unsigned int runSeed = ReadUInt(text, "runSeed", 0u);
    int floor = GameMathClampInt(ReadInt(text, "floor", 1), 1, FLOOR_COUNT);
    int roomX = GameMathClampInt(ReadInt(text, "roomX", 0), 0, GRID_SIZE - 1);
    int roomY = GameMathClampInt(ReadInt(text, "roomY", 0), 0, GRID_SIZE - 1);
    unsigned int rngNow = ReadUInt(text, "rngNow", 0u);
    unsigned int rngFloorEntry = ReadUInt(text, "rngFloorEntry", 0u);

    /* (1) La run si RICOSTRUISCE dal seed, esattamente come un attraversamento
       del varco del Piano 0: contenuti, atlas, prove e piano 1. Nulla di
       quello che segue inventa un mondo -- lo stato salvato si limita ad
       applicarsi sopra. */
    GameResetRunWithSeed(game, runSeed);

    /* (2) Il personaggio scelto, con la stessa cattura/riapplicazione che
       src/app/app.c fa all'attraversamento: GameResetRunWithSeed ha appena
       azzerato l'intero Game, quindi la def generata (che non vive in nessuna
       tabella const) va riscritta a mano prima di risolverla. */
    int characterIndex = ReadInt(text, "characterIndex", -1);
    if (ReadBool(text, "character.valid"))
    {
        CharacterDef c = { 0 };
        ReadStr(text, "character.name", c.name, (int)sizeof(c.name));
        ReadStr(text, "character.role", c.role, (int)sizeof(c.role));
        ReadStr(text, "character.blurb", c.blurb, (int)sizeof(c.blurb));
        c.baseDamage = ReadFloat(text, "character.baseDamage", 8.0f);
        c.baseFireDelay = ReadFloat(text, "character.baseFireDelay", 0.23f);
        c.baseShotSpeed = ReadFloat(text, "character.baseShotSpeed", 520.0f);
        c.baseShotRadius = ReadFloat(text, "character.baseShotRadius", 5.0f);
        c.baseSpeed = ReadFloat(text, "character.baseSpeed", 224.0f);
        c.baseMaxHp = ReadInt(text, "character.baseMaxHp", 6);
        c.hpCap = ReadInt(text, "character.hpCap", 12);
        c.baseLuck = ReadFloat(text, "character.baseLuck", 0.0f);
        c.palette = ReadColor(text, "character.palette");
        ReadStr(text, "character.traitHook", c.traitHook, (int)sizeof(c.traitHook));
        ReadShotType(text, "character.signatureShot", &c.signatureShot);
        /* Rete di sicurezza minima su un file che il giocatore puo' forgiare a
           mano: i due soli campi che, a zero, renderebbero il personaggio non
           giocabile. Il resto e' gia' passato da CharacterGenDefClamp quando la
           proposta fu caricata (content/character_proposal.c), e il colpo
           firmato e' appena passato da ShotTypeClamp qui sopra. */
        if (c.baseMaxHp < 1) c.baseMaxHp = 1;
        if (c.hpCap < c.baseMaxHp) c.hpCap = c.baseMaxHp;
        game->generatedCharacter = c;
        game->generatedCharacterValid = true;
    }
    game->characterChosenIndex = characterIndex;
    const CharacterDef *chosen = GameResolveCharacterDef(game, characterIndex);
    if (chosen) GamePlayerResetBaseStatsFor(&game->player, chosen);
    ScriptItemsInit(game, chosen);

    /* (3) La MAPPA del piano corrente: si rimette 'rng' al valore che aveva
       all'ingresso in quel piano e si rigenera. E' l'unico modo per ottenere
       la STESSA mappa -- WorldGenerateFloorMap pesca da game->rng, che il
       combattimento ha fatto avanzare da allora. Vale anche per il piano 1
       (dove GameResetRunWithSeed ha gia' generato una mappa): rigenerarlo con
       lo stesso stato d'ingresso e' idempotente, e tenere UNA sola strada per
       tutti e cinque i piani vale piu' del passo risparmiato. */
    game->rng = rngFloorEntry;
    WorldStartFloor(game, floor);

    /* (4) Il resto dello stato scalare della run. Lo stream di gioco si rimette
       al valore della sospensione soltanto al passo (9), IMMEDIATAMENTE prima
       di ri-materializzare la stanza: fra qui e li' la ricostruzione
       dell'inventario consuma qualche estrazione (ScriptItemsOnAcquire pesca da
       'game->rng' il seme della sandbox Lua di ogni oggetto, src/script/
       script_items.c), e farlo dopo aver gia' rimesso 'rng' sposterebbe la
       stanza rispetto a quella che la run sospesa avrebbe trovato rientrandoci.
       LIMITE DICHIARATO (docs/engineering/known-issues.md): i semi delle
       sandbox Lua degli oggetti NON sono salvati, quindi si ri-estraggono dallo
       stream di ricostruzione -- una funzione Lua che chiami rng() riparte da
       un punto diverso della PROPRIA sequenza. Resta deterministico dal file
       (due riprese dello stesso file danno gli stessi semi), non identico alla
       run originale. */
    game->roomNumber = ReadInt(text, "roomNumber", 0);
    game->score = ReadInt(text, "score", 0);
    game->fusionCount = ReadInt(text, "fusionCount", 0);
    game->runElapsedSeconds = ReadFloat(text, "runTime", 0.0f);
    game->floorEntryElapsedSeconds = ReadFloat(text, "floorEntryTime", 0.0f);
    game->treasureLuckStreak = ReadInt(text, "treasureLuckStreak", 0);
    game->shopLuckStreak = ReadInt(text, "shopLuckStreak", 0);
    game->timedRoomEverGenerated = ReadBool(text, "timedRoomEverGenerated");
    game->secretRoomEverGenerated = ReadBool(text, "secretRoomEverGenerated");
    game->arenaRoomEverGenerated = ReadBool(text, "arenaRoomEverGenerated");
    game->currentBossFightDamaged = ReadBool(text, "currentBossFightDamaged");
    game->pourhouseLastSignature = ReadUInt(text, "pourhouseLastSignature", 0u);

    /* (5) Inventario. Ogni oggetto torna nel suo slot con lo stato di ricarica
       che aveva, e ScriptItemsOnAcquire ricrea la sandbox Lua dal sorgente
       salvato: e' la STESSA via del pickup, quindi un oggetto ripreso da una
       sospensione non e' mai "meno vivo" di uno appena raccolto. */
    Player *p = &game->player;
    p->baseDamage = ReadFloat(text, "player.baseDamage", p->baseDamage);
    p->baseFireDelay = ReadFloat(text, "player.baseFireDelay", p->baseFireDelay);
    p->baseShotSpeed = ReadFloat(text, "player.baseShotSpeed", p->baseShotSpeed);
    p->baseShotRadius = ReadFloat(text, "player.baseShotRadius", p->baseShotRadius);
    p->baseSpeed = ReadFloat(text, "player.baseSpeed", p->baseSpeed);
    p->baseMaxHp = ReadInt(text, "player.baseMaxHp", p->baseMaxHp);
    p->baseLuck = ReadFloat(text, "player.baseLuck", p->baseLuck);
    p->hpCap = ReadInt(text, "player.hpCap", p->hpCap);
    p->coins = ReadInt(text, "player.coins", 0);
    p->bombs = ReadInt(text, "player.bombs", 0);
    p->keys = ReadInt(text, "player.keys", 0);
    p->flux = ReadInt(text, "player.flux", 0);
    p->activeSlotCount = ReadInt(text, "player.activeSlotCount", 1);
    p->graftSlotCount = ReadInt(text, "player.graftSlotCount", 1);
    p->activeSelected = ReadInt(text, "player.activeSelected", 0);

    int itemCount = GameMathClampInt(ReadInt(text, "item.count", 0), 0, MAX_ITEMS);
    p->itemCount = 0;
    for (int i = 0; i < itemCount; i++)
    {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "item%d", i + 1);
        ReadItemFields(text, prefix, &p->items[i]);
        p->itemCount = i + 1;
        ScriptItemsOnAcquire(game, i);
    }
    ScriptItemsRecomputeStats(game);   /* maxHp/damage/... derivano SEMPRE da base* + oggetti, mai dal file */

    /* La salute si applica DOPO il ricalcolo, che e' l'unica fonte di maxHp. */
    int savedHp = ReadInt(text, "player.hp", p->maxHp);
    p->hp = GameMathClampInt(savedHp, 1, p->maxHp > 0 ? p->maxHp : 1);
    p->tempHp = GameMathClampInt(ReadInt(text, "player.tempHp", 0), 0, PLAYER_TEMP_HP_CAP);

    /* (6) Innesti a terra (DEC-183). */
    int graftCount = GameMathClampInt(ReadInt(text, "graft.count", 0), 0, MAX_DROPPED_GRAFTS);
    for (int i = 0; i < graftCount; i++)
    {
        char prefix[32];
        char key[64];
        snprintf(prefix, sizeof(prefix), "graft%d", i + 1);
        DroppedGraftRecord *g = &game->droppedGrafts[i];
        memset(g, 0, sizeof(*g));
        snprintf(key, sizeof(key), "%s.roomX", prefix); g->roomX = GameMathClampInt(ReadInt(text, key, 0), 0, GRID_SIZE - 1);
        snprintf(key, sizeof(key), "%s.roomY", prefix); g->roomY = GameMathClampInt(ReadInt(text, key, 0), 0, GRID_SIZE - 1);
        snprintf(key, sizeof(key), "%s.posX", prefix);  g->pos.x = ReadFloat(text, key, 0.0f);
        snprintf(key, sizeof(key), "%s.posY", prefix);  g->pos.y = ReadFloat(text, key, 0.0f);
        char itemPrefix[48];
        snprintf(itemPrefix, sizeof(itemPrefix), "%s.item", prefix);
        ReadItemFields(text, itemPrefix, &g->item);
        g->active = true;
    }

    /* (7) Puntata della Pourhouse e prove della run. */
    PourhouseWager *w = &game->pourhouse;
    memset(w, 0, sizeof(*w));
    w->composed = ReadBool(text, "pourhouse.composed");
    w->valid = ReadBool(text, "pourhouse.valid");
    w->accepted = ReadBool(text, "pourhouse.accepted");
    w->roomX = GameMathClampInt(ReadInt(text, "pourhouse.roomX", 0), 0, GRID_SIZE - 1);
    w->roomY = GameMathClampInt(ReadInt(text, "pourhouse.roomY", 0), 0, GRID_SIZE - 1);
    w->offerKind = (PourhouseOfferKind)GameMathClampInt(ReadInt(text, "pourhouse.offerKind", 0), 0, (int)POURHOUSE_OFFER_COUNT - 1);
    w->offerAmount = ReadInt(text, "pourhouse.offerAmount", 0);
    w->offerKeys = ReadInt(text, "pourhouse.offerKeys", 0);
    w->priceKind = (PourhousePriceKind)GameMathClampInt(ReadInt(text, "pourhouse.priceKind", 0), 0, (int)POURHOUSE_PRICE_COUNT - 1);
    w->priceAmount = ReadInt(text, "pourhouse.priceAmount", 0);
    ReadStr(text, "pourhouse.priceItemName", w->priceItemName, (int)sizeof(w->priceItemName));
    w->priceItemRarity = (Rarity)GameMathClampInt(ReadInt(text, "pourhouse.priceItemRarity", 0), 0, (int)RARITY_LEGENDARY);
    w->offerValue = ReadInt(text, "pourhouse.offerValue", 0);
    w->priceValue = ReadInt(text, "pourhouse.priceValue", 0);
    ReadItemFields(text, "pourhouse.offerItem", &w->offerItem);

    int trialCount = GameMathClampInt(ReadInt(text, "trial.count", 0), 0, TRIAL_SLOTS_MAX);
    game->trialCount = trialCount;
    for (int i = 0; i < trialCount; i++)
    {
        char key[48];
        Trial *t = &game->trials[i];
        memset(t, 0, sizeof(*t));
        snprintf(key, sizeof(key), "trial%d.kind", i + 1);  t->kind = (TrialKind)GameMathClampInt(ReadInt(text, key, 0), 0, (int)TRIAL_KIND_COUNT - 1);
        snprintf(key, sizeof(key), "trial%d.state", i + 1); t->state = (TrialState)GameMathClampInt(ReadInt(text, key, 0), 0, (int)TRIAL_VOID);
        snprintf(key, sizeof(key), "trial%d.param", i + 1); t->param = ReadInt(text, key, 0);
        snprintf(key, sizeof(key), "trial%d.bonus", i + 1); t->bonus = ReadInt(text, key, 0);
        snprintf(key, sizeof(key), "trial%d.text", i + 1);  ReadStr(text, key, t->text, (int)sizeof(t->text));
    }

    for (int i = 0; i < FLOOR_COUNT; i++)
    {
        char key[48];
        snprintf(key, sizeof(key), "enc%d.enemy1", i + 1);       game->enemyEncountered[i][0] = ReadBool(text, key);
        snprintf(key, sizeof(key), "enc%d.enemy2", i + 1);       game->enemyEncountered[i][1] = ReadBool(text, key);
        snprintf(key, sizeof(key), "enc%d.boss", i + 1);         game->bossEncountered[i] = ReadBool(text, key);
        snprintf(key, sizeof(key), "enc%d.bossDefeated", i + 1); game->bossDefeated[i] = ReadBool(text, key);
    }

    /* (8) Stato mutabile della griglia: il file e' AUTORITATIVO, quindi si
       azzerano prima tutti e cinque i flag su ogni cella (compresi quelli che
       WorldGenerateFloorMap ha appena scritto sulla stanza di partenza) e poi
       si applica cio' che il file dice. Al contrario, applicare solo le celle
       presenti lascerebbe in piedi lo stato di una mappa appena generata. */
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            char key[48];
            RoomState *r = &game->rooms[y][x];
            snprintf(key, sizeof(key), "room.%d.%d", y, x);
            unsigned int mask = ReadUInt(text, key, 0u);
            r->visited = (mask & SUSPEND_ROOM_VISITED) != 0u;
            r->cleared = (mask & SUSPEND_ROOM_CLEARED) != 0u;
            r->rewardTaken = (mask & SUSPEND_ROOM_REWARD_TAKEN) != 0u;
            r->arenaActive = (mask & SUSPEND_ROOM_ARENA_ACTIVE) != 0u;
            r->secretOpened = (mask & SUSPEND_ROOM_SECRET_OPENED) != 0u;
            snprintf(key, sizeof(key), "destroyed.%d.%d", y, x);
            game->destroyedObstacleMask[y][x] = (unsigned short)ReadUInt(text, key, 0u);
        }

    /* (9) La stanza corrente RIPARTE DALL'INGRESSO (DEC-050): il giocatore vi
       rientra come vi entrerebbe da una porta -- posizione di partenza della
       stanza, nemici ri-materializzati da WorldSpawnRoomContents. Non esiste
       uno snapshot di meta' combattimento, quindi non c'e' nulla da rimettere
       in piedi oltre alla stanza stessa.
       DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019): "l'ingresso" e' il
       BARICENTRO della stanza (WorldRoomCenter), lo stesso punto in cui
       WorldStartFloor deposita il giocatore all'ingresso in un piano -- la
       porta da cui era entrato non e' salvata, e sarebbe l'unico modo per
       scegliere uno dei quattro offset di WorldTryEnterRoom. */
    game->roomX = roomX;
    game->roomY = roomY;
    /* Lo stream di gioco riprende ESATTAMENTE dal punto della sospensione (vedi
       il passo (4)): la stanza che si ri-materializza qui sotto e' cosi' la
       stessa che la run sospesa avrebbe trovato rientrandoci, e da li' in poi
       la sequenza continua identica. */
    game->rng = rngNow;
    game->player.pos = WorldRoomCenter(game);
    WorldSnapCamera(game);
    int roomNumberBefore = game->roomNumber;
    WorldSpawnRoomContents(game);
    /* WorldSpawnRoomContents conta l'ingresso: qui pero' non e' un ingresso
       nuovo, e' la STESSA visita che era in corso. Il contatore torna al
       valore salvato, cosi' la ripresa non gonfia le statistiche della run. */
    game->roomNumber = roomNumberBefore;

    UnloadFileText(text);

    /* (10) Una sospensione SI CONSUMA: morire dopo la ripresa non deve poter
       riportare al punto di salvataggio (systems/save-and-meta-progression.md,
       "permadeath"). Cancellata solo a ricostruzione riuscita. */
    RunSuspendClear();
    GameSetMessage(game, "Run ripresa: la stanza riparte dall ingresso.");
    return true;
}
