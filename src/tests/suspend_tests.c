/* WP17 (DEC-050, docs/design/systems/save-and-meta-progression.md
   "Sospensione della run e ripresa"; ui/main-menu.md "Continua";
   ui/pause-menu.md "Sospendi e esci"): LA SOSPENSIONE DELLA RUN.

   Sette blocchi:
   (a) andata e ritorno su una run ricca al piano 3 -- confronto CAMPO PER
       CAMPO dello stato ricostruito in un Game NUOVO (inventario con sorgente
       Lua, risorse, Crust, prove con stato, tempo, fusioni, stanze
       visitate/ripulite/premiate, segreta aperta, distruttibili distrutti,
       Innesto a terra, puntata della Pourhouse, incontri) piu' la MAPPA del
       piano, che non e' serializzata e deve tornare identica dal seed;
   (b) la stanza corrente riparte DALL'INGRESSO coi nemici ripristinati;
   (c) determinismo della ripresa: due riprese dallo stesso file danno lo
       stesso identico stato, stream RNG compreso;
   (d) file corrotto/di versione diversa -> nessuna voce "Continua", nessun
       crash, Game intatto;
   (e) la sospensione si CONSUMA alla ripresa;
   (f) il flusso vero attraverso UpdateApp: "Sospendi e esci" da PauseMenu ->
       MainMenu con "Continua" a fuoco -> ripresa in Gameplay;
   (g) le tre vie che CANCELLANO la sospensione: abbandono (DEC-089), reroll
       (DEC-114) e "Nuova run" con conferma (ui/main-menu.md).

   Come GameEconomyTest, gira dopo InitWindow e usa 'game' per davvero
   (GameResetRunWithSeed chiama AssetsLoad) ma non disegna nulla. Il file di
   sospensione vive in una cartella temporanea (RunSuspendSetTestPath): questo
   test non tocca mai suspend/ vero, come GameCatalogTest non tocca catalog/. */

#include "tests/game_tests.h"

#include "app/app.h"
#include "app/app_internal.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "game/run_suspend.h"
#include "render/game_renderer.h"
#include "script/script_items.h"
#include "world/world.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define SUSPEND_SEED 20260731u

static bool g_fail = false;

#define SUSPEND_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameSuspendTest: %s\n", (msg)); g_fail = true; goto cleanup; } } while (0)

static AppInput InputNone(void)    { AppInput in = { 0 }; return in; }
static AppInput InputConfirm(void) { AppInput in = { 0 }; in.confirm = true; return in; }
static AppInput InputDown(void)    { AppInput in = { 0 }; in.down = true; return in; }

/* Copia privata di CreateTempCatalogTestDir (src/tests/catalog_tests.c): stessa
   convenzione del progetto, ogni modulo di test si porta la propria. */
static char *CreateTempDir(char *pathBuf, size_t pathBufSize, const char *namePrefix)
{
    const char *base = getenv("TMPDIR");
#ifdef _WIN32
    if (!base) base = getenv("TEMP");
    if (!base) base = getenv("TMP");
    if (!base) base = ".";
    for (int attempt = 0; attempt < 64; attempt++)
    {
        snprintf(pathBuf, pathBufSize, "%s\\%s-%d-%d", base, namePrefix, rand(), attempt);
        if (_mkdir(pathBuf) == 0) return pathBuf;
    }
    return NULL;
#else
    if (!base) base = "/tmp";
    snprintf(pathBuf, pathBufSize, "%s/%s-XXXXXX", base, namePrefix);
    return mkdtemp(pathBuf);
#endif
}

static void RemoveTempDir(const char *path)
{
#ifdef _WIN32
    _rmdir(path);
#else
    rmdir(path);
#endif
}

/* ---- fixture: lo stato "ricco" che la sospensione deve riportare intero ---- */

/* Un oggetto costruito a mano, con sorgente Lua vera: e' cio' che prova che
   l'inventario non si ricostruisce "per nome dal manifest" (un oggetto FUSO
   non e' in nessun manifest) ma dal record stesso. */
static Item MakeSuspendItem(const char *name, ItemKind kind, Rarity rarity, const char *lua)
{
    Item it = { 0 };
    it.active = true;
    snprintf(it.name, sizeof(it.name), "%s", name);
    it.slot = SLOT_HAND;
    it.kind = kind;
    it.rarity = rarity;
    it.traits = 0x5u;
    it.color = (Color){ 200, 120, 40, 255 };
    it.shape = 3;
    snprintf(it.script, sizeof(it.script), "on_fire,damage,+1");
    if (lua) snprintf(it.luaSource, sizeof(it.luaSource), "%s", lua);
    it.charges = 4;
    it.chargeNow = 2;
    it.chargeGainRoom = 1;
    it.chargeGainEnergy = 1;
    snprintf(it.imageId, sizeof(it.imageId), "item.test.%s", name);
    snprintf(it.fusedFrom[0], sizeof(it.fusedFrom[0]), "Sorgente A");
    snprintf(it.fusedFrom[1], sizeof(it.fusedFrom[1]), "Sorgente B");
    it.shotType.active = true;
    snprintf(it.shotType.name, sizeof(it.shotType.name), "Scheggia di prova");
    it.shotType.form = SHOT_FORM_SPIKE;
    it.shotType.speedMul = 1.2f;
    it.shotType.damageMul = 0.9f;
    it.shotType.radiusMul = 0.8f;
    it.shotType.lifeMul = 1.1f;
    it.shotType.pierceBonus = 1;
    return it;
}

static void GrantSuspendItem(Game *game, Item item)
{
    int index = game->player.itemCount;
    if (index >= MAX_ITEMS) return;
    game->player.items[index] = item;
    game->player.itemCount = index + 1;
    ScriptItemsOnAcquire(game, index);
}

/* Trova la cella di STATO di una stanza dell'archetipo dato sul piano
   corrente, o -1/-1. */
static bool FindRoomOfKind(const Game *game, RoomKind kind, int *outX, int *outY)
{
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            if (!game->rooms[y][x].exists) continue;
            const RoomState *state = WorldRoomAt(game, x, y);
            if (state != &game->rooms[y][x]) continue;
            if (state->kind == kind) { *outX = x; *outY = y; return true; }
        }
    return false;
}

/* Costruisce la run "ricca" al piano 3 con seed fisso: la stessa strada del
   gioco vero (GameResetRunWithSeed + WorldStartFloor, esattamente quello che
   fa combat.c a ogni cambio piano), poi lo stato che il documento elenca. */
static bool BuildRichRun(Game *game)
{
    GameResetRunWithSeed(game, SUSPEND_SEED);   /* la run parte davvero dal piano 1 */
    /* I piani 1 e 2 si ATTRAVERSANO per davvero, con simulazione in mezzo,
       prima di arrivare al 3. Non e' scenografia: e' quello che fa AVANZARE lo
       stream di gioco fra un ingresso di piano e il successivo. Saltando dritti
       al piano 3 dopo il reset, lo stato dell'RNG all'ingresso del piano
       coinciderebbe per caso con quello che una ripresa otterrebbe senza
       rimettere 'rng' al valore d'ingresso salvato -- e il confronto della
       mappa non potrebbe piu' accorgersi di quella dimenticanza (verificato:
       senza questo attraversamento la mutazione sopravvive). */
    for (int f = 1; f <= 2; f++)
    {
        WorldStartFloor(game, f);
        for (int step = 0; step < 90; step++)
        {
            /* Il giocatore non deve morire durante l'avanzamento: qui interessa
               solo che la simulazione consumi lo stream, non l'esito. */
            game->player.hp = game->player.maxHp > 0 ? game->player.maxHp : 6;
            game->player.tempHp = 0;
            game->phase = PHASE_PLAY;
            GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
        }
        game->phase = PHASE_PLAY;
    }
    WorldStartFloor(game, 3);

    game->player.coins = 41;
    game->player.bombs = 5;
    game->player.keys = 3;
    game->player.flux = 2;
    game->player.tempHp = 3;   /* Crust (DEC-008) */
    game->player.activeSlotCount = 2;
    game->player.graftSlotCount = 2;
    game->player.activeSelected = 1;

    GrantSuspendItem(game, MakeSuspendItem("Lente Fusa", ITEM_PASSIVE, RARITY_RARE,
                                           "function on_tick(dt) end\n"));
    GrantSuspendItem(game, MakeSuspendItem("Innesto Provato", ITEM_GRAFT, RARITY_UNCOMMON, NULL));
    ScriptItemsRecomputeStats(game);
    game->player.hp = 2;   /* dopo il ricalcolo: maxHp e' derivato, hp no */

    game->fusionCount = 3;
    game->runElapsedSeconds = 137.5f;
    game->floorEntryElapsedSeconds = 92.25f;
    game->score = 1234;
    game->roomNumber = 17;
    game->treasureLuckStreak = 2;
    game->shopLuckStreak = 1;
    game->timedRoomEverGenerated = true;
    game->secretRoomEverGenerated = true;
    game->arenaRoomEverGenerated = true;
    game->currentBossFightDamaged = true;
    game->pourhouseLastSignature = 0xBEEF01u;

    /* Prova SUPERATA: lo stato non si ricava dal seed, solo l'assegnazione. */
    if (game->trialCount <= 0) return false;
    game->trials[0].state = TRIAL_PASSED;

    /* Stanze: la partenza e' gia' visitata+ripulita; si marca a mano una
       seconda cella come ripulita+premiata e una terza come segreta aperta,
       piu' un distruttibile fatto saltare. */
    int marked = 0;
    for (int y = 0; y < GRID_SIZE && marked < 3; y++)
        for (int x = 0; x < GRID_SIZE && marked < 3; x++)
        {
            RoomState *r = &game->rooms[y][x];
            if (!r->exists || r->kind == ROOM_START) continue;
            if (WorldRoomAt(game, x, y) != r) continue;
            if (marked == 0) { r->visited = true; r->cleared = true; r->rewardTaken = true; }
            if (marked == 1) { r->visited = true; r->secretOpened = true; }
            if (marked == 2) { r->arenaActive = true; game->destroyedObstacleMask[y][x] = 0x25u; }
            marked++;
        }
    if (marked < 3) return false;

    /* Innesto lasciato a terra (DEC-183). */
    game->droppedGrafts[0].active = true;
    game->droppedGrafts[0].roomX = game->roomX;
    game->droppedGrafts[0].roomY = game->roomY;
    game->droppedGrafts[0].pos = (Vector2){ 111.0f, 222.0f };
    game->droppedGrafts[0].item = MakeSuspendItem("Innesto a Terra", ITEM_GRAFT, RARITY_LEGENDARY, NULL);

    /* Puntata della Pourhouse (DEC-044), offerta compresa. */
    game->pourhouse.composed = true;
    game->pourhouse.valid = true;
    game->pourhouse.accepted = false;
    game->pourhouse.roomX = 1;
    game->pourhouse.roomY = 2;
    game->pourhouse.offerKind = POURHOUSE_OFFER_ITEM;
    game->pourhouse.offerAmount = 0;
    game->pourhouse.offerItem = MakeSuspendItem("Offerta del Banco", ITEM_ACTIVE, RARITY_RARE, NULL);
    game->pourhouse.priceKind = POURHOUSE_PRICE_MAX_HP;
    game->pourhouse.priceAmount = 1;
    snprintf(game->pourhouse.priceItemName, sizeof(game->pourhouse.priceItemName), "Lente Fusa");
    game->pourhouse.priceItemRarity = RARITY_RARE;
    game->pourhouse.offerValue = 28;
    game->pourhouse.priceValue = 14;

    game->enemyEncountered[2][0] = true;
    game->bossEncountered[0] = true;
    game->bossDefeated[0] = true;
    return true;
}

/* Confronto CAMPO PER CAMPO fra la run sospesa e quella ricostruita. Ogni
   riga e' una riga del documento ("cosa si salva"): una mutazione che
   dimentichi un campo fa fallire esattamente la sua. */
/* 'exactEncounters' distingue i due usi. Fra la run SOSPESA e quella RIPRESA,
   i flag "incontrato" del piano CORRENTE possono legittimamente crescere: la
   stanza riparte dall'ingresso, quindi i suoi nemici vengono spawnati di nuovo
   e chi non era ancora comparso compare adesso. Cio' che non deve mai
   succedere e' PERDERE un incontro gia' registrato (il Catalogo lo scrivera' a
   fine run), e per gli altri piani vale l'uguaglianza secca. Fra DUE riprese
   dello stesso file, invece, tutto deve combaciare esattamente. */
static bool CompareRestored(const Game *a, const Game *b, bool exactEncounters)
{
    if (a->runSeed != b->runSeed) { fprintf(stderr, "GameSuspendTest: runSeed non ricostruito\n"); return false; }
    if (a->floor != b->floor) { fprintf(stderr, "GameSuspendTest: piano non ricostruito\n"); return false; }
    if (a->roomX != b->roomX || a->roomY != b->roomY) { fprintf(stderr, "GameSuspendTest: stanza corrente non ricostruita\n"); return false; }
    if (a->floorEntryRng != b->floorEntryRng) { fprintf(stderr, "GameSuspendTest: floorEntryRng non ricostruito\n"); return false; }
    if (a->characterChosenIndex != b->characterChosenIndex) { fprintf(stderr, "GameSuspendTest: personaggio scelto non ricostruito\n"); return false; }
    if (a->fusionCount != b->fusionCount) { fprintf(stderr, "GameSuspendTest: fusioni fatte non ricostruite\n"); return false; }
    if (a->score != b->score) { fprintf(stderr, "GameSuspendTest: punteggio non ricostruito\n"); return false; }
    if (a->roomNumber != b->roomNumber) { fprintf(stderr, "GameSuspendTest: numero di stanze non ricostruito\n"); return false; }
    if (a->treasureLuckStreak != b->treasureLuckStreak || a->shopLuckStreak != b->shopLuckStreak)
    { fprintf(stderr, "GameSuspendTest: contatori di correzione di fortuna non ricostruiti\n"); return false; }
    if (a->timedRoomEverGenerated != b->timedRoomEverGenerated ||
        a->secretRoomEverGenerated != b->secretRoomEverGenerated ||
        a->arenaRoomEverGenerated != b->arenaRoomEverGenerated)
    { fprintf(stderr, "GameSuspendTest: flag di archetipo generato non ricostruiti\n"); return false; }
    if (a->currentBossFightDamaged != b->currentBossFightDamaged)
    { fprintf(stderr, "GameSuspendTest: tentativo di boss senza danno non ricostruito\n"); return false; }
    if (a->pourhouseLastSignature != b->pourhouseLastSignature)
    { fprintf(stderr, "GameSuspendTest: firma della Pourhouse non ricostruita\n"); return false; }
    if (a->runElapsedSeconds < b->runElapsedSeconds - 0.01f || a->runElapsedSeconds > b->runElapsedSeconds + 0.01f)
    { fprintf(stderr, "GameSuspendTest: tempo di run non ricostruito\n"); return false; }
    if (a->floorEntryElapsedSeconds < b->floorEntryElapsedSeconds - 0.01f ||
        a->floorEntryElapsedSeconds > b->floorEntryElapsedSeconds + 0.01f)
    { fprintf(stderr, "GameSuspendTest: istante di ingresso nel piano non ricostruito\n"); return false; }

    const Player *pa = &a->player;
    const Player *pb = &b->player;
    if (pa->hp != pb->hp || pa->tempHp != pb->tempHp)
    { fprintf(stderr, "GameSuspendTest: salute o Crust non ricostruiti\n"); return false; }
    if (pa->coins != pb->coins || pa->bombs != pb->bombs || pa->keys != pb->keys || pa->flux != pb->flux)
    { fprintf(stderr, "GameSuspendTest: risorse non ricostruite\n"); return false; }
    if (pa->activeSlotCount != pb->activeSlotCount || pa->graftSlotCount != pb->graftSlotCount ||
        pa->activeSelected != pb->activeSelected)
    { fprintf(stderr, "GameSuspendTest: slot funzionali non ricostruiti\n"); return false; }
    if (pa->maxHp != pb->maxHp)
    { fprintf(stderr, "GameSuspendTest: tetto di salute non ricostruito\n"); return false; }
    if (pa->itemCount != pb->itemCount)
    { fprintf(stderr, "GameSuspendTest: numero di oggetti non ricostruito\n"); return false; }
    for (int i = 0; i < pa->itemCount; i++)
    {
        const Item *ia = &pa->items[i];
        const Item *ib = &pb->items[i];
        if (strcmp(ia->name, ib->name) != 0 || ia->kind != ib->kind || ia->rarity != ib->rarity ||
            ia->slot != ib->slot || ia->traits != ib->traits || ia->shape != ib->shape ||
            ia->charges != ib->charges || ia->chargeNow != ib->chargeNow)
        { fprintf(stderr, "GameSuspendTest: oggetto %d non ricostruito\n", i); return false; }
        if (strcmp(ia->luaSource, ib->luaSource) != 0)
        { fprintf(stderr, "GameSuspendTest: sorgente Lua dell'oggetto %d non ricostruita\n", i); return false; }
        if (strcmp(ia->imageId, ib->imageId) != 0 || strcmp(ia->fusedFrom[0], ib->fusedFrom[0]) != 0)
        { fprintf(stderr, "GameSuspendTest: identita' visiva/origine dell'oggetto %d non ricostruita\n", i); return false; }
        if (ia->shotType.active != ib->shotType.active || ia->shotType.form != ib->shotType.form ||
            strcmp(ia->shotType.name, ib->shotType.name) != 0)
        { fprintf(stderr, "GameSuspendTest: tipo di colpo dell'oggetto %d non ricostruito\n", i); return false; }
    }

    if (a->trialCount != b->trialCount) { fprintf(stderr, "GameSuspendTest: numero di prove non ricostruito\n"); return false; }
    for (int i = 0; i < a->trialCount; i++)
    {
        if (a->trials[i].kind != b->trials[i].kind || a->trials[i].state != b->trials[i].state ||
            a->trials[i].param != b->trials[i].param || a->trials[i].bonus != b->trials[i].bonus ||
            strcmp(a->trials[i].text, b->trials[i].text) != 0)
        { fprintf(stderr, "GameSuspendTest: prova %d non ricostruita\n", i); return false; }
    }

    for (int i = 0; i < FLOOR_COUNT; i++)
    {
        bool loose = !exactEncounters && (i == a->floor - 1);
        bool same = a->enemyEncountered[i][0] == b->enemyEncountered[i][0] &&
                    a->enemyEncountered[i][1] == b->enemyEncountered[i][1] &&
                    a->bossEncountered[i] == b->bossEncountered[i] &&
                    a->bossDefeated[i] == b->bossDefeated[i];
        bool keptAll = (!a->enemyEncountered[i][0] || b->enemyEncountered[i][0]) &&
                       (!a->enemyEncountered[i][1] || b->enemyEncountered[i][1]) &&
                       (!a->bossEncountered[i] || b->bossEncountered[i]) &&
                       (!a->bossDefeated[i] || b->bossDefeated[i]);
        if (loose ? !keptAll : !same)
        { fprintf(stderr, "GameSuspendTest: incontri del piano %d non ricostruiti\n", i + 1); return false; }
    }

    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
        {
            const RoomState *ra = &a->rooms[y][x];
            const RoomState *rb = &b->rooms[y][x];
            /* La MAPPA non e' serializzata: deve tornare identica dal seed. */
            if (ra->exists != rb->exists || ra->kind != rb->kind || ra->cells != rb->cells ||
                ra->originX != rb->originX || ra->originY != rb->originY ||
                memcmp(ra->doors, rb->doors, sizeof(ra->doors)) != 0)
            { fprintf(stderr, "GameSuspendTest: la mappa del piano non e' stata rigenerata identica in (%d,%d)\n", x, y); return false; }
            /* Lo STATO mutabile e' invece proprio quello che il file porta. */
            if (ra->visited != rb->visited || ra->cleared != rb->cleared ||
                ra->rewardTaken != rb->rewardTaken || ra->arenaActive != rb->arenaActive ||
                ra->secretOpened != rb->secretOpened)
            { fprintf(stderr, "GameSuspendTest: stato della stanza (%d,%d) non ricostruito\n", x, y); return false; }
            if (a->destroyedObstacleMask[y][x] != b->destroyedObstacleMask[y][x])
            { fprintf(stderr, "GameSuspendTest: distruttibili distrutti in (%d,%d) non ricostruiti\n", x, y); return false; }
        }

    int graftsA = 0, graftsB = 0;
    for (int i = 0; i < MAX_DROPPED_GRAFTS; i++)
    {
        if (a->droppedGrafts[i].active) graftsA++;
        if (b->droppedGrafts[i].active) graftsB++;
    }
    if (graftsA != graftsB) { fprintf(stderr, "GameSuspendTest: Innesti a terra non ricostruiti\n"); return false; }
    if (graftsA > 0 && strcmp(a->droppedGrafts[0].item.name, b->droppedGrafts[0].item.name) != 0)
    { fprintf(stderr, "GameSuspendTest: l'Innesto a terra ha perso identita'\n"); return false; }

    const PourhouseWager *wa = &a->pourhouse;
    const PourhouseWager *wb = &b->pourhouse;
    if (wa->composed != wb->composed || wa->valid != wb->valid || wa->accepted != wb->accepted ||
        wa->roomX != wb->roomX || wa->roomY != wb->roomY ||
        wa->offerKind != wb->offerKind || wa->priceKind != wb->priceKind ||
        wa->priceAmount != wb->priceAmount || wa->offerValue != wb->offerValue ||
        wa->priceValue != wb->priceValue ||
        strcmp(wa->priceItemName, wb->priceItemName) != 0 ||
        strcmp(wa->offerItem.name, wb->offerItem.name) != 0)
    { fprintf(stderr, "GameSuspendTest: puntata della Pourhouse non ricostruita\n"); return false; }
    return true;
}

/* Scrive un file di sospensione ARBITRARIO nel percorso di test: serve al
   blocco (d) (schema sbagliato, testo troncato) e ai blocchi che hanno bisogno
   di una sospensione "gia' li'" senza costruire una run. */
static void WriteRawSuspendFile(const char *dir, const char *content)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/current.txt", dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fputs(content, f);
    fclose(f);
}

static bool SuspendFileExists(const char *dir)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/current.txt", dir);
    return FileExists(path);
}

/* Copia il file di sospensione in memoria e lo rimette a posto: il blocco (c)
   ne ha bisogno perche' la ripresa lo CONSUMA. */
static char *SnapshotSuspendFile(const char *dir)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/current.txt", dir);
    return LoadFileText(path);
}

bool GameSuspendTest(Game *game)
{
    g_fail = false;
    char dirBuf[256] = { 0 };
    char *dir = CreateTempDir(dirBuf, sizeof(dirBuf), "melting-test-suspend");
    if (!dir)
    {
        fprintf(stderr, "GameSuspendTest: impossibile creare la cartella temporanea\n");
        return false;
    }
    RunSuspendSetTestPath(dir);
    SetMousePosition(2, 2);   /* come GameStatesTest: fuori da ogni geometria di menu */

    Game *restored = (Game *)calloc(1, sizeof(Game));
    Game *restored2 = (Game *)calloc(1, sizeof(Game));
    char *snapshot = NULL;
    if (!restored || !restored2)
    {
        fprintf(stderr, "GameSuspendTest: allocazione dei Game di prova fallita\n");
        g_fail = true;
        goto cleanup;
    }

    /* ---- (a) andata e ritorno su una run ricca al piano 3 ---- */
    SUSPEND_CHECK(BuildRichRun(game), "la run di prova non e' stata costruita (prove o stanze insufficienti)");
    SUSPEND_CHECK(game->floor == 3, "la run di prova non e' al piano 3");
    SUSPEND_CHECK(game->player.itemCount == 2, "la run di prova non ha i due oggetti attesi");
    SUSPEND_CHECK(game->floorEntryRng != 0u, "WorldStartFloor non ha catturato floorEntryRng");

    /* CONTROPROVA che il blocco (a) non sia tautologico: una ricostruzione
       INGENUA -- solo GameResetRunWithSeed, senza rimettere 'rng' al valore
       d'ingresso nel piano -- arriverebbe al piano 3 con uno stream diverso.
       Se questi due valori coincidessero, il confronto della mappa piu' sotto
       non potrebbe distinguere una ripresa corretta da una che ignora
       floorEntryRng. 'restored2' e' ancora libero qui: verra' comunque
       riscritto per intero dalla ripresa del blocco (c). */
    GameResetRunWithSeed(restored2, SUSPEND_SEED);
    SUSPEND_CHECK(restored2->rng != game->floorEntryRng,
                  "la fixture e' tautologica: lo stream ingenuo coincide con quello d'ingresso nel piano");

    /* Nemici azzerati PRIMA della sospensione: al rientro devono tornare, e'
       la regola "la stanza riparte dall'ingresso coi nemici ripristinati". */
    {
        int combatX = 0, combatY = 0;
        SUSPEND_CHECK(FindRoomOfKind(game, ROOM_COMBAT, &combatX, &combatY),
                      "il piano di prova non ha una stanza di combattimento");
        game->roomX = combatX;
        game->roomY = combatY;
        RoomState *cur = WorldCurrentRoomMutable(game);
        cur->cleared = false;   /* stanza NON ripulita: i nemici devono ricomparire */
        game->droppedGrafts[0].roomX = combatX;
        game->droppedGrafts[0].roomY = combatY;
        /* Ingresso VERO nella stanza (la stessa chiamata di WorldTryEnterRoom):
           marca 'visited' e registra i tipi di nemico incontrati, cosi' lo stato
           sospeso e' quello di una run davvero giocata, non uno costruito a
           mano che il confronto poi non potrebbe reggere. */
        WorldSpawnRoomContents(game);
        game->roomNumber = 17;   /* dopo l'ingresso: il contatore deve tornare ESATTAMENTE questo */
        /* Nemici azzerati e giocatore a meta' stanza: nessuno dei due si salva,
           entrambi devono essere ricostruiti dalla ri-materializzazione. */
        EntitiesClear(game);
        game->player.pos = (Vector2){ 12.0f, 34.0f };
    }

    SUSPEND_CHECK(RunSuspendWrite(game), "RunSuspendWrite non ha scritto la sospensione");
    SUSPEND_CHECK(SuspendFileExists(dir), "il file di sospensione non esiste dopo la scrittura");
    SUSPEND_CHECK(RunSuspendIsAvailable(), "RunSuspendIsAvailable non riconosce la sospensione appena scritta");

    snapshot = SnapshotSuspendFile(dir);
    SUSPEND_CHECK(snapshot != NULL, "impossibile rileggere il file di sospensione appena scritto");

    /* RIFERIMENTO per il default proposto "si salva game->rng, cosi' la
       sequenza continua identica": la stanza che la ripresa deve
       ri-materializzare e' ESATTAMENTE quella che la run sospesa otterrebbe
       rientrandoci adesso, cioe' spawnata dallo stesso stato di stream. La si
       spawna qui, DOPO aver scritto il file, per avere il riferimento vero da
       confrontare -- rimettendo a posto il solo contatore di stanze, che un
       ingresso vero incrementa. Senza questo confronto, una ripresa che
       ripartisse dallo stream d'ingresso nel piano invece che da quello della
       sospensione passerebbe inosservata. */
    {
        int roomNumberBefore = game->roomNumber;
        game->player.pos = WorldRoomCenter(game);
        WorldSpawnRoomContents(game);
        game->roomNumber = roomNumberBefore;
    }

    SUSPEND_CHECK(RunSuspendResume(restored), "RunSuspendResume ha rifiutato una sospensione valida");
    SUSPEND_CHECK(CompareRestored(game, restored, true), "lo stato ricostruito diverge da quello sospeso");
    SUSPEND_CHECK(restored->rng == game->rng,
                  "lo stream RNG non riprende dal punto della sospensione (default proposto WP17)");
    {
        int aliveRef = 0, aliveGot = 0;
        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (game->enemies[i].active) aliveRef++;
            if (restored->enemies[i].active) aliveGot++;
        }
        SUSPEND_CHECK(aliveRef > 0, "la stanza di riferimento non ha spawnato nemici (fixture inutile)");
        SUSPEND_CHECK(aliveRef == aliveGot,
                      "la stanza ri-materializzata non coincide con quella della run sospesa");
        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            SUSPEND_CHECK(game->enemies[i].active == restored->enemies[i].active &&
                          game->enemies[i].pos.x == restored->enemies[i].pos.x &&
                          game->enemies[i].pos.y == restored->enemies[i].pos.y,
                          "un nemico ri-materializzato non e' nella stessa posizione della run sospesa");
        }
    }

    /* ---- (b) la stanza corrente riparte DALL'INGRESSO, nemici ripristinati ---- */
    {
        Vector2 entrance = WorldRoomCenter(restored);
        SUSPEND_CHECK(restored->player.pos.x == entrance.x && restored->player.pos.y == entrance.y,
                      "alla ripresa il giocatore non riparte dall'ingresso della stanza");
        SUSPEND_CHECK(restored->player.pos.x != 12.0f || restored->player.pos.y != 34.0f,
                      "alla ripresa e' stata ripristinata la posizione di meta' stanza (DEC-050 lo vieta)");
        int alive = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) if (restored->enemies[i].active) alive++;
        SUSPEND_CHECK(alive > 0, "alla ripresa i nemici della stanza non sono stati ripristinati");
        SUSPEND_CHECK(restored->phase == PHASE_PLAY, "la run ripresa non e' in PHASE_PLAY");
        SUSPEND_CHECK(restored->inRealRun, "la run ripresa non e' marcata come run vera");
    }

    /* ---- (e) la sospensione si CONSUMA ---- */
    SUSPEND_CHECK(!SuspendFileExists(dir), "il file di sospensione sopravvive alla ripresa (deve consumarsi)");
    SUSPEND_CHECK(!RunSuspendIsAvailable(), "RunSuspendIsAvailable resta vera dopo il consumo della sospensione");

    /* ---- (c) determinismo: due riprese dallo stesso file sono identiche ---- */
    WriteRawSuspendFile(dir, snapshot);
    SUSPEND_CHECK(RunSuspendResume(restored2), "la seconda ripresa dallo stesso file e' fallita");
    SUSPEND_CHECK(restored->rng == restored2->rng, "due riprese dello stesso file divergono nello stream RNG");
    {
        int aliveA = 0, aliveB = 0;
        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (restored->enemies[i].active) aliveA++;
            if (restored2->enemies[i].active) aliveB++;
        }
        SUSPEND_CHECK(aliveA == aliveB, "due riprese dello stesso file spawnano un numero diverso di nemici");
        SUSPEND_CHECK(CompareRestored(restored, restored2, true), "due riprese dello stesso file danno stati diversi");
    }

    /* ---- (d) file corrotto o di versione diversa ---- */
    {
        AppUi ui = { 0 };
        ui.suspendEnabled = true;

        WriteRawSuspendFile(dir, "suspendSchema=99\nrunSeed=7\nfloor=2\nroomX=1\nroomY=1\nrngNow=5\nrngFloorEntry=6\n");
        SUSPEND_CHECK(!RunSuspendIsAvailable(), "una sospensione di schema diverso viene accettata");
        ui.suspendAvailable = RunSuspendIsAvailable();
        SUSPEND_CHECK(!RendererMainMenuHasContinueRow(&ui), "la voce Continua compare con uno schema incompatibile");

        WriteRawSuspendFile(dir, "suspendSchema=1\nrunSeed=7\nfloo");   /* troncato a meta' */
        SUSPEND_CHECK(!RunSuspendIsAvailable(), "una sospensione troncata viene accettata");

        WriteRawSuspendFile(dir, "non e' nemmeno un formato\nchiave=valore\n");
        SUSPEND_CHECK(!RunSuspendIsAvailable(), "un file senza riga di schema viene accettato");

        WriteRawSuspendFile(dir, "suspendSchema=1\nrunSeed=7\nfloor=9\nroomX=1\nroomY=1\nrngNow=5\nrngFloorEntry=6\n");
        SUSPEND_CHECK(!RunSuspendIsAvailable(), "una sospensione con un piano fuori banda viene accettata");

        /* Nessuno di questi tocca il Game: la ripresa fallisce senza crash e
           senza lasciare uno stato a meta'. */
        unsigned int seedBefore = restored2->runSeed;
        int floorBefore = restored2->floor;
        SUSPEND_CHECK(!RunSuspendResume(restored2), "RunSuspendResume ha accettato un file non valido");
        SUSPEND_CHECK(restored2->runSeed == seedBefore && restored2->floor == floorBefore,
                      "un file non valido ha comunque toccato il Game");
        RunSuspendClear();
    }

    /* ---- (f) il flusso vero attraverso UpdateApp ---- */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_PAUSE_MENU;
        ui.suspendEnabled = true;
        SUSPEND_CHECK(BuildRichRun(game), "la run di prova per il flusso UpdateApp non e' stata costruita");

        SUSPEND_CHECK(RendererPauseMenuHasSuspendRow(game, &ui),
                      "la riga 'Sospendi e esci' non esiste in una run vera");
        SUSPEND_CHECK(RendererMenuCtxFor(game, &ui).pauseSuspend,
                      "il contesto di menu non riporta la riga 'Sospendi e esci'");

        /* Riprendi -> ... -> Sospendi e esci: cinque "giu'" dall'indice 0. */
        for (int i = 0; i < 5; i++) { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(ui.focus == 5, "la navigazione non arriva alla riga 'Sospendi e esci' (indice 5)");
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_MAIN_MENU, "'Sospendi e esci' non riporta a MainMenu");
        SUSPEND_CHECK(ui.suspendAvailable, "'Sospendi e esci' non registra la sospensione su AppUi");
        SUSPEND_CHECK(SuspendFileExists(dir), "'Sospendi e esci' non ha scritto il file di sospensione");
        SUSPEND_CHECK(ui.focus == 0, "il focus in MainMenu non e' sulla voce 'Continua'");
        SUSPEND_CHECK(RendererMainMenuHasContinueRow(&ui), "la voce 'Continua' non compare con una sospensione valida");

        /* MainMenu/Continua -> Gameplay, sospensione consumata. */
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_GAMEPLAY, "'Continua' non rientra in Gameplay");
        SUSPEND_CHECK(!ui.suspendAvailable, "'Continua' lascia la sospensione disponibile");
        SUSPEND_CHECK(!SuspendFileExists(dir), "'Continua' non consuma il file di sospensione");
        SUSPEND_CHECK(game->runSeed == SUSPEND_SEED, "la run ripresa non ha lo stesso seed di quella sospesa");
        SUSPEND_CHECK(game->floor == 3, "la run ripresa non e' al piano salvato");
        SUSPEND_CHECK(ui.seed == SUSPEND_SEED, "AppUi.seed non segue il seed della run ripresa");

        /* Senza sospensione la voce sparisce e MainMenu torna a quattro voci:
           il focus 0 e' di nuovo "Nuova run". */
        SUSPEND_CHECK(!RendererMainMenuHasContinueRow(&ui), "la voce 'Continua' resta dopo il consumo");

        /* (g1) "R dopo la ripresa" resta la STESSA run, stesso seed. */
        game->resetQueued = true;
        GameUpdate(game, 1.0f/60.0f, (Vector2){ 0.0f, 0.0f }, false);
        SUSPEND_CHECK(game->runSeed == SUSPEND_SEED, "il reset rapido dopo la ripresa cambia il seed della run");
        SUSPEND_CHECK(game->floor == 1, "il reset rapido dopo la ripresa non riparte dal piano 1");
    }

    /* ---- (g) le tre vie che cancellano la sospensione ---- */
    {
        AppGen gen = { 0 };
        AppUi ui = { 0 };
        AppMode mode = APP_MAIN_MENU;
        ui.suspendEnabled = true;

        /* (g2) "Nuova run" con una sospensione: conferma esplicita, poi il file
           sparisce e si apre RunSetup. */
        SUSPEND_CHECK(BuildRichRun(game), "la run di prova per l'abbandono non e' stata costruita");
        SUSPEND_CHECK(RunSuspendWrite(game), "scrittura della sospensione fallita nel blocco (g)");
        ui.suspendAvailable = true;
        ui.focus = 1;   /* con "Continua" all'indice 0, "Nuova run" e' l'indice 1 */
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_EXIT_CONFIRM, "'Nuova run' con una sospensione non chiede conferma");
        SUSPEND_CHECK(ui.exitDropsSuspendedRun, "il contesto di ExitConfirm non e' quello della sospensione");
        SUSPEND_CHECK(!ExitConfirmIsLightModalFor(ui.openedFrom, ui.exitDropsSuspendedRun),
                      "il dialogo di rinuncia alla sospensione e' stato marcato leggero (DEC-090)");
        SUSPEND_CHECK(ui.focus == 1, "il default di questa conferma non e' 'Annulla'");
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* Annulla */
        SUSPEND_CHECK(mode == APP_MAIN_MENU, "annullare la rinuncia non torna a MainMenu");
        SUSPEND_CHECK(SuspendFileExists(dir), "annullare la rinuncia ha comunque cancellato la sospensione");
        ui.focus = 1;
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* di nuovo "Nuova run" */
        { AppInput in = InputNone(); UpdateApp(game, &mode, &gen, &ui, &in); }
        ui.focus = 0;   /* "Conferma" */
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_RUN_SETUP, "confermare la rinuncia non apre RunSetup");
        SUSPEND_CHECK(!SuspendFileExists(dir), "confermare la rinuncia non cancella la sospensione");
        SUSPEND_CHECK(!ui.suspendAvailable, "confermare la rinuncia lascia la sospensione disponibile su AppUi");

        /* (g3) l'abbandono di una run vera (DEC-089) cancella la sospensione. */
        SUSPEND_CHECK(BuildRichRun(game), "la run di prova per l'abbandono non e' stata ricostruita");
        SUSPEND_CHECK(RunSuspendWrite(game), "scrittura della sospensione fallita prima dell'abbandono");
        ui.suspendAvailable = true;
        ui.exitDropsSuspendedRun = false;
        mode = APP_PAUSE_MENU;
        ui.focus = 6;   /* "Abbandona run", che con "Sospendi e esci" scala a 6 */
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_EXIT_CONFIRM && ui.exitAbandonsRun, "'Abbandona run' non apre la propria conferma");
        ui.focus = 0;
        { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
        SUSPEND_CHECK(mode == APP_RUN_RESULTS, "l'abbandono di una run vera non porta a RunResults (DEC-089)");
        SUSPEND_CHECK(!SuspendFileExists(dir), "l'abbandono non cancella la sospensione");
        SUSPEND_CHECK(!ui.suspendAvailable, "l'abbandono lascia la sospensione disponibile su AppUi");
    }

    /* Il Piano 0 non e' sospendibile in questa fetta (limite dichiarato). */
    {
        AppUi ui = { 0 };
        ui.suspendEnabled = true;
        Game *hub = restored2;
        hub->floor = 0;
        SUSPEND_CHECK(!RunSuspendWrite(hub), "il Piano 0 e' stato sospeso (limite dichiarato: non deve)");
        SUSPEND_CHECK(!RendererPauseMenuHasSuspendRow(hub, &ui),
                      "la riga 'Sospendi e esci' compare nel Piano 0");
    }

cleanup:
    if (snapshot) UnloadFileText(snapshot);
    RunSuspendClear();
    if (restored) { GameUnloadAssets(restored); free(restored); }
    if (restored2) { GameUnloadAssets(restored2); free(restored2); }
    RunSuspendSetTestPath(NULL);
    RemoveTempDir(dir);
    return !g_fail;
}
