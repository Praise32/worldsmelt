/* Test del substrato del catalogo persistente v1 (M7, DEC-015/041/045/069,
   src/content/run_catalog.c + l'hook AppWriteRunCatalog in src/app/app.c).
   Stesso stile di src/tests/script_character_tests.c: costruisce una run
   sintetica su uno Game locale (memset, nessun GameResetRun/AssetsLoad --
   questo test non tocca mai l'atlas/le sandbox Lua, solo i campi dati e i
   file di testo che RunCatalogWriteRun legge davvero) e guida UpdateApp con
   AppInput sintetici (mai IsKeyPressed), esattamente come GameStatesTest:
   gira DOPO InitWindow per lo stesso motivo (UpdateApp legge
   GetScreenWidth/Height per il mouse dei menu).

   Ogni scenario scrive PRIMA i propri file fixture in generated/ (lo stesso
   manifest/chosen_theme.txt/character_trait.lua che una run vera lascia sul
   disco) cosi' RunCatalogWriteRun legge esattamente cio' che il test si
   aspetta, non lo stato ambientale del checkout. */

#include "tests/game_tests.h"

#include "app/app.h"
#include "app/app_internal.h"
#include "content/run_catalog.h"
#include "render/game_renderer.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CATALOG_TEST_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameCatalogTest: %s\n", (msg)); return false; } } while (0)

static AppInput InputNone(void)    { AppInput in = { 0 }; return in; }
static AppInput InputConfirm(void) { AppInput in = { 0 }; in.confirm = true; return in; }
static AppInput InputReroll(void)  { AppInput in = { 0 }; in.reroll = true; return in; }
/* M8 (DEC-045, vista Catalogo v1): le quattro direzioni + back, usate SOLO
   dai due scenari di GameCatalogScreenTest sotto -- copia privata locale,
   stessa convenzione gia' in uso in questo file (InputNone/InputConfirm/
   InputReroll sopra) e in src/tests/game_tests.c. */
static AppInput InputBack(void)  { AppInput in = { 0 }; in.back = true; return in; }
static AppInput InputUp(void)    { AppInput in = { 0 }; in.up = true; return in; }
static AppInput InputDown(void)  { AppInput in = { 0 }; in.down = true; return in; }
static AppInput InputLeft(void)  { AppInput in = { 0 }; in.left = true; return in; }
static AppInput InputRight(void) { AppInput in = { 0 }; in.right = true; return in; }

/* ---- fixture su disco: gli stessi tre file che una run vera lascia in
   generated/ e che RunCatalogWriteRun rilegge (mai provenance.txt: la
   guardia principale usa SOLO current_run.txt, spec M7). ---- */

static void WriteManifestSource(const char *source)
{
    MakeDirectory("generated");
    FILE *f = fopen("generated/current_run.txt", "w");
    if (!f) return;
    fprintf(f, "# GameCatalogTest fixture\nsource=%s\nseed=999\n", source);
    fclose(f);
}

static void RemoveManifest(void) { remove("generated/current_run.txt"); }

static void WriteChosenTheme(const char *name, const char *blurb)
{
    MakeDirectory("generated");
    FILE *f = fopen("generated/chosen_theme.txt", "w");
    if (!f) return;
    fprintf(f, "%s -- %s\n", name, blurb);
    fclose(f);
}

static void WriteCharacterTraitLua(const char *source)
{
    MakeDirectory("generated");
    MakeDirectory("generated/scripts");
    FILE *f = fopen("generated/scripts/character_trait.lua", "w");
    if (!f) return;
    fputs(source, f);
    fclose(f);
}

/* ---- lettura del record scritto: stesso schema "strstr fino a fine riga"
   di ReadManifestValue (run_catalog.c/run_content.c), una copia privata di
   test come ogni altro modulo che legge i propri file. ---- */

static void ReadValue(const char *text, const char *key, char *out, int outSize)
{
    out[0] = '\0';
    if (!text || !key) return;
    const char *start = strstr(text, key);
    if (!start) return;
    start += strlen(key);
    int i = 0;
    while (start[i] && start[i] != '\r' && start[i] != '\n' && i < outSize - 1) { out[i] = start[i]; i++; }
    out[i] = '\0';
}

/* ---- snapshot/diff di catalog/: individua il file NUOVO comparso fra due
   istanti, senza dover pre-calcolare il progressivo (che dipende da quanto
   gia' presente in catalog/ da run precedenti di questo stesso eseguibile:
   la cartella non viene mai ripulita fra un'invocazione e l'altra, e' dato
   del giocatore come generated/, mai ripristinata da un test). ---- */

#define CATALOG_TEST_MAX_FILES 128
typedef struct { char names[CATALOG_TEST_MAX_FILES][256]; int count; } CatalogSnapshot;

static void SnapshotCatalog(CatalogSnapshot *snap)
{
    snap->count = 0;
    if (!DirectoryExists("catalog")) return;
    FilePathList files = LoadDirectoryFilesEx("catalog", ".txt", false);
    for (unsigned int i = 0; i < files.count && snap->count < CATALOG_TEST_MAX_FILES; i++)
        snprintf(snap->names[snap->count++], sizeof(snap->names[0]), "%s", GetFileName(files.paths[i]));
    UnloadDirectoryFiles(files);
}

static bool FindNewFile(const CatalogSnapshot *before, const CatalogSnapshot *after, char *outName, int outSize)
{
    for (int i = 0; i < after->count; i++)
    {
        bool seen = false;
        for (int j = 0; j < before->count; j++)
            if (strcmp(after->names[i], before->names[j]) == 0) { seen = true; break; }
        if (!seen) { snprintf(outName, (size_t)outSize, "%s", after->names[i]); return true; }
    }
    return false;
}

static int CatalogTmpFileCount(void)
{
    if (!DirectoryExists("catalog")) return 0;
    FilePathList files = LoadDirectoryFilesEx("catalog", ".tmp", false);
    int count = (int)files.count;
    UnloadDirectoryFiles(files);
    return count;
}

/* Pulizia a fine test: RunCatalogWriteRun scrive nella STESSA dir catalog/
   che il gioco vero usa per i dati del giocatore (non sa distinguere una run
   sintetica da una vera), quindi il test deve rimuovere ESATTAMENTE i file
   comparsi durante la sua esecuzione -- mai i record reali gia' presenti nel
   checkout di chi lancia le suite -- altrimenti ogni `make test` accumula
   record fittizi ('Testforge', seed 4242/7...) che una futura schermata
   Catalogo o la riconvalida DEC-069 rileggerebbe come contenuti davvero
   incontrati. 'before' e' lo snapshot preso PRIMA di qualunque scenario:
   togliamo dalla dir solo i .txt (e gli eventuali .tmp residui, difensivo)
   che NON c'erano allora. Se la dir non esisteva affatto prima del test la
   rimuoviamo del tutto (remove() su una dir e' rmdir: riesce solo se vuota,
   cioe' se nessun record reale la abitava). */
static void CleanupCatalog(const CatalogSnapshot *before, bool dirExistedBefore)
{
    if (DirectoryExists("catalog"))
    {
        CatalogSnapshot after; SnapshotCatalog(&after);
        for (int i = 0; i < after.count; i++)
        {
            bool preexisting = false;
            for (int j = 0; j < before->count; j++)
                if (strcmp(after.names[i], before->names[j]) == 0) { preexisting = true; break; }
            if (preexisting) continue;
            char path[300];
            snprintf(path, sizeof(path), "catalog/%s", after.names[i]);
            remove(path);
        }
        FilePathList tmp = LoadDirectoryFilesEx("catalog", ".tmp", false);
        for (unsigned int i = 0; i < tmp.count; i++) remove(tmp.paths[i]);
        UnloadDirectoryFiles(tmp);
    }
    if (!dirExistedBefore && DirectoryExists("catalog")) remove("catalog");
}

/* Una run sintetica "ricca": 2 piani raggiunti, un oggetto preso con Lua e
   tipo di colpo, un nemico e un boss incontrati per piano (boss del piano 1
   solo incontrato, boss del piano 2 sconfitto -- copre entrambi gli esiti
   nello stesso record), un personaggio generato scelto con trait Lua e
   colpo firmato attivo. 'floor' resta parametrico: i test di guardia
   (FloorZero, nessun piano) passano 0. */
static void BuildSyntheticGame(Game *game, int floor)
{
    memset(game, 0, sizeof(*game));
    game->phase = PHASE_PLAY;
    game->floor = floor;
    if (floor < 1) return;   /* FloorZero/nessun piano: il resto non conta, la guardia lo scarta comunque */

    for (int f = 0; f < floor && f < FLOOR_COUNT; f++)
    {
        FloorContent *fc = &game->content.floors[f];
        snprintf(fc->theme.name, sizeof(fc->theme.name), "Test Theme %d", f + 1);
        snprintf(fc->theme.style, sizeof(fc->theme.style), "test style");
        snprintf(fc->theme.bossName, sizeof(fc->theme.bossName), "Test Guardian %d", f + 1);
        fc->theme.bg = (Color){ 10, 20, 30, 255 };
        fc->theme.floor = (Color){ 40, 50, 60, 255 };
        fc->theme.wall = (Color){ 70, 80, 90, 255 };
        fc->theme.accent = (Color){ 100, 110, 120, 255 };
        fc->theme.accent2 = (Color){ 130, 140, 150, 255 };
        fc->theme.enemy = (Color){ 160, 170, 180, 255 };
        fc->theme.boss = (Color){ 190, 200, 210, 255 };

        fc->roomLayout.active = true;
        snprintf(fc->roomLayout.name, sizeof(fc->roomLayout.name), "Test Layout %d", f + 1);
        fc->roomLayout.form = ROOM_LAYOUT_PILLARS;
        fc->roomLayout.density = 0.5f;

        fc->enemies[0].active = true;
        snprintf(fc->enemies[0].name, sizeof(fc->enemies[0].name), "Test Enemy %d", f + 1);
        fc->enemies[0].form = ENEMY_FORM_SPIKY;
        fc->enemies[0].move = ENEMY_MOVE_ZIGZAG;
        fc->enemies[0].fire = ENEMY_FIRE_SPREAD;
        fc->enemies[0].hpMul = 1.1f;
        fc->enemies[0].speedMul = 0.9f;
        fc->enemies[0].sizeMul = 1.0f;
        fc->enemies[0].fireRate = 1.2f;
        fc->enemies[0].pellets = 3;
        game->enemyEncountered[f][0] = true;
        /* fc->enemies[1] resta inattivo/non incontrato: verifica che la
           categoria salti gli slot non incontrati senza scrivere nulla. */

        fc->bossType.active = true;
        fc->bossType.boss = true;
        snprintf(fc->bossType.name, sizeof(fc->bossType.name), "Test Boss %d", f + 1);
        fc->bossType.form = ENEMY_FORM_ARMORED;
        fc->bossType.move = ENEMY_MOVE_CHARGE;
        fc->bossType.fire = ENEMY_FIRE_RING;
        fc->bossType.hpMul = 2.0f;
        fc->bossType.speedMul = 0.8f;
        fc->bossType.sizeMul = 1.6f;
        fc->bossType.fireRate = 0.9f;
        fc->bossType.pellets = 8;
        game->bossEncountered[f] = true;
    }
    /* Boss del piano 1 solo incontrato, boss dell'ULTIMO piano registrato
       sconfitto: copre i due esiti "incontrato"/"sconfitto" nello stesso
       record (spec M7, test dedicato). */
    if (floor >= 1) game->bossDefeated[floor - 1] = (floor >= 2);

    Item *it = &game->player.items[0];
    it->active = true;
    snprintf(it->name, sizeof(it->name), "Test Blade");
    it->slot = SLOT_HAND;
    it->kind = ITEM_ACTIVE;
    it->rarity = RARITY_RARE;
    it->traits = TRAIT_PIERCE | TRAIT_RAPID;
    it->color = (Color){ 5, 6, 7, 255 };
    it->shape = 2;
    snprintf(it->script, sizeof(it->script), "on_hit:projectile,1,300,none");
    snprintf(it->luaSource, sizeof(it->luaSource), "function on_fire(ctx)\n  local x = 1\n  return x\nend\n");
    it->shotType.active = true;
    snprintf(it->shotType.name, sizeof(it->shotType.name), "Test Floor Shot");
    it->shotType.form = SHOT_FORM_BEAM;
    it->shotType.speedMul = 1.1f;
    it->shotType.damageMul = 0.9f;
    it->shotType.radiusMul = 1.0f;
    it->shotType.lifeMul = 1.2f;
    it->shotType.pierceBonus = 1;
    it->shotType.chain = 0;
    it->shotType.pellets = 1;
    game->player.itemCount = 1;
    game->player.shotType = it->shotType;   /* "adottato": e' quello attivo del giocatore ORA */

    game->characterChosenIndex = CHARACTER_COUNT;   /* il personaggio GENERATO */
    game->generatedCharacterValid = true;
    CharacterDef *c = &game->generatedCharacter;
    snprintf(c->name, sizeof(c->name), "Testforge");
    snprintf(c->role, sizeof(c->role), "FORGED THIS RUN");
    snprintf(c->blurb, sizeof(c->blurb), "A character built for GameCatalogTest.");
    c->baseDamage = 8.0f;
    c->baseFireDelay = 0.2f;
    c->baseShotSpeed = 500.0f;
    c->baseShotRadius = 5.0f;
    c->baseSpeed = 220.0f;
    c->baseMaxHp = 6;
    c->hpCap = 12;
    c->baseLuck = 0.1f;
    c->palette = (Color){ 200, 100, 50, 255 };
    snprintf(c->traitHook, sizeof(c->traitHook), "on_evaluate");
    c->signatureShot.active = true;
    snprintf(c->signatureShot.name, sizeof(c->signatureShot.name), "Test Signature Bolt");
    c->signatureShot.form = SHOT_FORM_SPIKE;
    c->signatureShot.speedMul = 1.0f;
    c->signatureShot.damageMul = 1.0f;
    c->signatureShot.radiusMul = 1.0f;
    c->signatureShot.lifeMul = 1.0f;
    c->signatureShot.pierceBonus = 0;
    c->signatureShot.chain = 1;
    c->signatureShot.pellets = 1;
    game->player.characterShotType = c->signatureShot;   /* "attivo" sul giocatore */

    WriteCharacterTraitLua("function on_evaluate(stats)\n  stats.max_hp = stats.max_hp + 1\nend\n");
}

/* ============================================================
   Test A: run interamente fallback -> nessun file scritto (spec M7, punto
   2, default v1 sulla domanda "fallback-usato conta?": NO).
   ============================================================ */
static bool TestFallbackWritesNothing(void)
{
    WriteManifestSource("fallback");
    Game game;
    BuildSyntheticGame(&game, 2);

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    ui.catalogWritesEnabled = true;
    ui.seed = 999;
    AppMode mode = APP_GAMEPLAY;
    CatalogSnapshot before; SnapshotCatalog(&before);

    game.phase = PHASE_WIN;
    { AppInput in = InputNone(); UpdateApp(&game, &mode, &gen, &ui, &in); }

    CatalogSnapshot after; SnapshotCatalog(&after);
    char newFile[256];
    bool wroteNothing = !FindNewFile(&before, &after, newFile, sizeof(newFile));
    CATALOG_TEST_CHECK(wroteNothing, "source=fallback ha comunque scritto un file di catalogo");
    CATALOG_TEST_CHECK(game.catalogRecordsWritten == 0, "source=fallback ha valorizzato catalogRecordsWritten");
    return true;
}

/* ============================================================
   Test B: nessun manifest sul disco (mai girato melting-gen) -> stesso
   trattamento del fallback, niente file.
   ============================================================ */
static bool TestMissingManifestWritesNothing(void)
{
    RemoveManifest();
    Game game;
    BuildSyntheticGame(&game, 2);

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    ui.catalogWritesEnabled = true;
    ui.seed = 999;
    AppMode mode = APP_GAMEPLAY;
    CatalogSnapshot before; SnapshotCatalog(&before);

    game.phase = PHASE_GAME_OVER;
    { AppInput in = InputNone(); UpdateApp(&game, &mode, &gen, &ui, &in); }

    CatalogSnapshot after; SnapshotCatalog(&after);
    char newFile[256];
    bool wroteNothing = !FindNewFile(&before, &after, newFile, sizeof(newFile));
    CATALOG_TEST_CHECK(wroteNothing, "manifest assente ha comunque scritto un file di catalogo");
    return true;
}

/* ============================================================
   Test C: FloorZero/nessun piano giocato -> niente file, anche con
   contenuto generato vero e la guardia test-safe disattivata (source=
   local:*, catalogWritesEnabled=true): la guardia "floor<1" da sola basta.
   Esercita il SECONDO chiamante dell'hook (ExitConfirm, abbandono dalla
   preparazione: openedFrom=APP_FLOOR_ZERO).
   ============================================================ */
static bool TestFloorZeroWritesNothing(void)
{
    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game game;
    BuildSyntheticGame(&game, 0);   /* floor=0: Piano 0, nessun piano giocato */

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    ui.catalogWritesEnabled = true;
    ui.seed = 999;
    ui.exitAbandonsRun = true;
    ui.openedFrom = APP_FLOOR_ZERO;
    ui.focus = 0;
    AppMode mode = APP_EXIT_CONFIRM;
    CatalogSnapshot before; SnapshotCatalog(&before);

    { AppInput in = InputConfirm(); UpdateApp(&game, &mode, &gen, &ui, &in); }

    CatalogSnapshot after; SnapshotCatalog(&after);
    char newFile[256];
    bool wroteNothing = !FindNewFile(&before, &after, newFile, sizeof(newFile));
    CATALOG_TEST_CHECK(mode == APP_MAIN_MENU, "abbandono dal Piano 0 non torna a MainMenu");
    CATALOG_TEST_CHECK(wroteNothing, "abbandono dal Piano 0 (floor=0) ha comunque scritto un file di catalogo");
    CATALOG_TEST_CHECK(game.catalogRecordsWritten == 0, "abbandono dal Piano 0 ha valorizzato catalogRecordsWritten");
    return true;
}

/* Verifica comune ai tre scenari di scrittura vera sotto: il file appena
   comparso ha schema=1, seed/outcome/floorReached corretti, e le categorie
   attese (incluso il round-trip del sorgente Lua). 'expectedFloor' e' il
   numero di piani che BuildSyntheticGame ha popolato. */
static bool VerifyWrittenRecord(const char *path, unsigned int expectedSeed, const char *expectedOutcome, int expectedFloor)
{
    char *text = LoadFileText(path);
    CATALOG_TEST_CHECK(text != NULL, "il file di catalogo appena scritto non si rilegge");

    bool ok = true;
    ok = ok && (strncmp(text, "catalogSchema=1", 15) == 0);
    if (!ok) fprintf(stderr, "GameCatalogTest: la prima riga non e' catalogSchema=1 (%s)\n", path);

    char value[256];
    ReadValue(text, "seed=", value, sizeof(value));
    bool seedOk = (unsigned int)strtoul(value, NULL, 10) == expectedSeed;
    ok = ok && seedOk;
    if (!seedOk) fprintf(stderr, "GameCatalogTest: seed=%s (atteso %u)\n", value, expectedSeed);

    ReadValue(text, "outcome=", value, sizeof(value));
    bool outcomeOk = strcmp(value, expectedOutcome) == 0;
    ok = ok && outcomeOk;
    if (!outcomeOk) fprintf(stderr, "GameCatalogTest: outcome=%s (atteso %s)\n", value, expectedOutcome);

    ReadValue(text, "floorReached=", value, sizeof(value));
    bool floorOk = atoi(value) == expectedFloor;
    ok = ok && floorOk;
    if (!floorOk) fprintf(stderr, "GameCatalogTest: floorReached=%s (atteso %d)\n", value, expectedFloor);

    ok = ok && (strstr(text, "world.name=Test World") != NULL);
    ok = ok && (strstr(text, "world.blurb=A blurb for the test world.") != NULL);
    ok = ok && (strstr(text, "floor1.theme.name=Test Theme 1") != NULL);
    ok = ok && (strstr(text, "floor1.room.form=pillars") != NULL);
    ok = ok && (strstr(text, "floor1.enemy1.name=Test Enemy 1") != NULL);
    ok = ok && (strstr(text, "floor1.enemy2.name=") == NULL);   /* slot 2 mai incontrato: nessuna riga */
    ok = ok && (strstr(text, "floor1.boss.name=Test Boss 1") != NULL);
    ok = ok && (strstr(text, "floor1.boss.outcome=incontrato") != NULL);   /* piano 1: solo incontrato */
    if (expectedFloor >= 2)
    {
        ok = ok && (strstr(text, "floor2.boss.outcome=sconfitto") != NULL);   /* ultimo piano: sconfitto */
    }
    ok = ok && (strstr(text, "item1.name=Test Blade") != NULL);
    ok = ok && (strstr(text, "item1.rarity=rare") != NULL);
    ok = ok && (strstr(text, "item1.traits=pierce,rapid") != NULL);
    ok = ok && (strstr(text, "item1.shotType.name=Test Floor Shot") != NULL);
    ok = ok && (strstr(text, "shot.floor.active=1") != NULL);
    ok = ok && (strstr(text, "shot.character.active=1") != NULL);
    ok = ok && (strstr(text, "character.active=1") != NULL);
    ok = ok && (strstr(text, "character.name=Testforge") != NULL);
    ok = ok && (strstr(text, "character.traitHook=on_evaluate") != NULL);
    ok = ok && (strstr(text, "character.signatureShot.name=Test Signature Bolt") != NULL);
    if (!ok) fprintf(stderr, "GameCatalogTest: una o piu' categorie attese mancano in %s\n", path);

    /* Round-trip del sorgente Lua dell'oggetto (autosufficienza, spec M7 --
       "porta la definizione COMPLETA... il sorgente Lua dove esiste"): il
       campo e' su una riga sola (escaped), va scandito con ReadValue e poi
       invertito con RunCatalogUnescapeText, non trovato con un semplice
       strstr multi-riga (che fallirebbe per costruzione). */
    ReadValue(text, "item1.lua=", value, sizeof(value));
    char unescaped[SCRIPT_LUA_LEN];
    RunCatalogUnescapeText(value, unescaped, sizeof(unescaped));
    bool luaOk = strcmp(unescaped, "function on_fire(ctx)\n  local x = 1\n  return x\nend\n") == 0;
    ok = ok && luaOk;
    if (!luaOk) fprintf(stderr, "GameCatalogTest: round-trip luaSource fallito, riletto: [%s]\n", unescaped);

    ReadValue(text, "character.traitLua=", value, sizeof(value));
    RunCatalogUnescapeText(value, unescaped, sizeof(unescaped));
    bool traitLuaOk = strcmp(unescaped, "function on_evaluate(stats)\n  stats.max_hp = stats.max_hp + 1\nend\n") == 0;
    ok = ok && traitLuaOk;
    if (!traitLuaOk) fprintf(stderr, "GameCatalogTest: round-trip character.traitLua fallito, riletto: [%s]\n", unescaped);

    UnloadFileText(text);
    return ok;
}

/* ============================================================
   Test D/E/F: scrittura vera per i tre esiti (spec M7, punto 5: "invoca la
   scrittura con i tre esiti"). D e E passano dal PRIMO chiamante
   (PHASE_WIN/PHASE_GAME_OVER in APP_GAMEPLAY); F dal reroll (TERZO
   chiamante, quello che "oggi sfugge a ogni hook" -- verifica che sia
   davvero coperto, con gen disabilitata: il ramo resetQueued NON deve
   impedire la scrittura, che avviene PRIMA che GameUpdate consumi il
   flag). Ognuno controlla anche l'assenza di residui .tmp (scrittura
   atomica, spec M7 punto 5).
   ============================================================ */
static bool RunRealWriteScenario(const char *label, int expectedFloor, const char *expectedOutcome,
                                  void (*drive)(Game *game, AppGen *gen, AppUi *ui, AppMode *mode))
{
    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game game;
    BuildSyntheticGame(&game, expectedFloor);

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    ui.catalogWritesEnabled = true;
    ui.seed = 4242;
    AppMode mode = APP_GAMEPLAY;

    CatalogSnapshot before; SnapshotCatalog(&before);
    drive(&game, &gen, &ui, &mode);
    CatalogSnapshot after; SnapshotCatalog(&after);

    char newFile[256];
    bool wroteOne = FindNewFile(&before, &after, newFile, sizeof(newFile));
    if (!wroteOne) { fprintf(stderr, "GameCatalogTest: %s non ha scritto alcun file nuovo in catalog/\n", label); return false; }
    CATALOG_TEST_CHECK(game.catalogRecordsWritten > 0, "catalogRecordsWritten non valorizzato dopo una scrittura vera");

    char fullPath[300];
    snprintf(fullPath, sizeof(fullPath), "catalog/%s", newFile);
    bool recordOk = VerifyWrittenRecord(fullPath, ui.seed, expectedOutcome, expectedFloor);
    CATALOG_TEST_CHECK(recordOk, "il record scritto non passa la verifica dei campi");
    CATALOG_TEST_CHECK(CatalogTmpFileCount() == 0, "un file .tmp e' rimasto residuo in catalog/ (scrittura non atomica)");
    return true;
}

static void DriveWin(Game *game, AppGen *gen, AppUi *ui, AppMode *mode)
{
    game->phase = PHASE_WIN;
    AppInput in = InputNone();
    UpdateApp(game, mode, gen, ui, &in);
}

static void DriveLoss(Game *game, AppGen *gen, AppUi *ui, AppMode *mode)
{
    game->phase = PHASE_GAME_OVER;
    AppInput in = InputNone();
    UpdateApp(game, mode, gen, ui, &in);
}

static void DriveReroll(Game *game, AppGen *gen, AppUi *ui, AppMode *mode)
{
    gen->enabled = false;   /* ramo "resetQueued": non deve impedire la scrittura, che precede il flag */
    AppInput in = InputReroll();
    UpdateApp(game, mode, gen, ui, &in);
}

static bool TestWinWritesRecord(void)    { return RunRealWriteScenario("vittoria",  2, RUN_CATALOG_OUTCOME_WIN,     DriveWin); }
static bool TestLossWritesRecord(void)   { return RunRealWriteScenario("sconfitta", 2, RUN_CATALOG_OUTCOME_LOSS,    DriveLoss); }
static bool TestRerollWritesRecord(void) { return RunRealWriteScenario("abbandono", 2, RUN_CATALOG_OUTCOME_ABANDON, DriveReroll); }

/* ============================================================
   Test G: abbandono confermato da PauseMenu (SECONDO chiamante, questa
   volta con un piano davvero giocato) -> registra, esattamente come il
   reroll.
   ============================================================ */
static bool TestPauseMenuAbandonWritesRecord(void)
{
    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game game;
    BuildSyntheticGame(&game, 1);

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    ui.catalogWritesEnabled = true;
    ui.seed = 4242;
    ui.exitAbandonsRun = true;
    ui.openedFrom = APP_PAUSE_MENU;
    ui.focus = 0;
    AppMode mode = APP_EXIT_CONFIRM;

    CatalogSnapshot before; SnapshotCatalog(&before);
    { AppInput in = InputConfirm(); UpdateApp(&game, &mode, &gen, &ui, &in); }
    CatalogSnapshot after; SnapshotCatalog(&after);

    char newFile[256];
    bool wroteOne = FindNewFile(&before, &after, newFile, sizeof(newFile));
    CATALOG_TEST_CHECK(mode == APP_MAIN_MENU, "abbandono da PauseMenu non torna a MainMenu");
    CATALOG_TEST_CHECK(wroteOne, "abbandono da PauseMenu (floor=1) non ha scritto alcun file in catalog/");
    CATALOG_TEST_CHECK(game.catalogRecordsWritten > 0, "catalogRecordsWritten non valorizzato dopo l'abbandono da PauseMenu");
    return true;
}

/* ============================================================
   Test H: due run -> due file distinti (progressivo, spec M7 punto 5).
   ============================================================ */
static bool TestTwoRunsGiveTwoFiles(void)
{
    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");

    Game gameA; BuildSyntheticGame(&gameA, 1);
    Game gameB; BuildSyntheticGame(&gameB, 1);

    AppGen genA = { 0 }, genB = { 0 };
    AppUi uiA = { 0 }, uiB = { 0 };
    uiA.catalogWritesEnabled = uiB.catalogWritesEnabled = true;
    uiA.seed = uiB.seed = 7;
    AppMode modeA = APP_GAMEPLAY, modeB = APP_GAMEPLAY;

    CatalogSnapshot before; SnapshotCatalog(&before);
    gameA.phase = PHASE_WIN;
    { AppInput in = InputNone(); UpdateApp(&gameA, &modeA, &genA, &uiA, &in); }
    CatalogSnapshot mid; SnapshotCatalog(&mid);
    gameB.phase = PHASE_WIN;
    { AppInput in = InputNone(); UpdateApp(&gameB, &modeB, &genB, &uiB, &in); }
    CatalogSnapshot after; SnapshotCatalog(&after);

    char firstFile[256], secondFile[256];
    bool gotFirst = FindNewFile(&before, &mid, firstFile, sizeof(firstFile));
    bool gotSecond = FindNewFile(&mid, &after, secondFile, sizeof(secondFile));
    CATALOG_TEST_CHECK(gotFirst && gotSecond, "due run con lo stesso seed/esito/piano non hanno scritto due file");
    CATALOG_TEST_CHECK(strcmp(firstFile, secondFile) != 0, "due run distinte hanno prodotto lo STESSO nome file (progressivo non avanzato)");
    return true;
}

bool GameCatalogTest(Game *game)
{
    (void)game;   /* ogni scenario costruisce il proprio Game locale (BuildSyntheticGame): isolamento totale dallo stato che AppRun ha gia' caricato */

    /* Snapshot PRIMA di ogni scenario: fotografa gli eventuali record reali
       gia' in catalog/ (chi lancia le suite potrebbe averci giocato davvero)
       cosi' la pulizia finale tocchi SOLO i file scritti dal test. Se la dir
       nemmeno esiste, il test la creera' e CleanupCatalog la rimuovera'. */
    bool dirExistedBefore = DirectoryExists("catalog");
    CatalogSnapshot before; SnapshotCatalog(&before);

    struct { const char *label; bool (*fn)(void); } tests[] = {
        { "A (source=fallback -> nessun file)", TestFallbackWritesNothing },
        { "B (manifest assente -> nessun file)", TestMissingManifestWritesNothing },
        { "C (FloorZero/floor=0, abbandono dalla preparazione -> nessun file)", TestFloorZeroWritesNothing },
        { "D (PHASE_WIN -> record vittoria completo)", TestWinWritesRecord },
        { "E (PHASE_GAME_OVER -> record sconfitta completo)", TestLossWritesRecord },
        { "F (reroll in Gameplay, terzo chiamante -> record abbandono)", TestRerollWritesRecord },
        { "G (abbandono confermato da PauseMenu, floor>=1 -> record abbandono)", TestPauseMenuAbandonWritesRecord },
        { "H (due run -> due file distinti, progressivo)", TestTwoRunsGiveTwoFiles },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        printf("-- test %s --\n", tests[i].label);
        bool ok = tests[i].fn();
        if (!ok) { fprintf(stderr, "  FALLITO: %s\n", tests[i].label); allOk = false; }
    }

    RemoveManifest();
    CleanupCatalog(&before, dirExistedBefore);   /* il --catalog-test scrive E pulisce: catalog/ resta com'era (o inesistente) */
    return allOk;
}

/* ============================================================
   M8 (DEC-045, vista Catalogo v1): test della schermata (--catalog-screen-
   test, aggregazione + navigazione dentro APP_MAIN_MENU). Stesso stile del
   blocco sopra: sotto-scenari con la propria etichetta, pulizia snapshot-
   based UNA volta sola a livello della funzione esposta (GameCatalogScreenTest
   in fondo), mai per singolo scenario -- l'invariante che ha causato la prima
   bocciatura della scala di implementazione (vedi CLAUDE.md): catalog/ deve
   restare esattamente com'era dopo la suite.
   ============================================================ */

#define CATALOG_SCREEN_TEST_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameCatalogScreenTest: %s\n", (msg)); return false; } } while (0)

/* Ricerca lineare in sola lettura -- copia privata di test dello stesso
   schema di FindOrCreateCatalogEntry (run_catalog.c), qui solo per le
   asserzioni. NULL se non trovata: i chiamanti lo trattano come un
   fallimento esplicito, mai un puntatore sporco. */
static const RunCatalogEntry *FindCatalogEntry(const RunCatalogSummary *sum, RunCatalogCategory cat, const char *name)
{
    for (int i = 0; i < sum->entryCount[cat]; i++)
        if (strcmp(sum->entries[cat][i].name, name) == 0) return &sum->entries[cat][i];
    return NULL;
}

/* Scenario 1 (spec M8: "catalogo vuoto -> messaggio senza crash"): catalog/
   spostata TEMPORANEAMENTE fuori dai piedi (rename, mai una cancellazione --
   ripristinata subito dopo, qualunque record reale del checkout resta
   intatto) cosi' l'aggregato e' DETERMINISTICAMENTE vuoto, indipendentemente
   da quante run vere chi lancia le suite ha gia' giocato su questo stesso
   checkout. Disegna anche un frame vero (RendererDrawApp, come GameLayerTest)
   perche' "senza crash" deve valere per davvero, non solo per l'aggregato. */
static bool CatalogScreenEmptyScenario(Game *game)
{
    bool hadDir = DirectoryExists("catalog");
    if (hadDir && rename("catalog", "catalog.GameCatalogScreenTest.bak") != 0)
    {
        fprintf(stderr, "GameCatalogScreenTest: impossibile spostare catalog/ da parte, scenario 'vuoto' saltato\n");
        return true;   /* non un fallimento del test: solo impossibile isolare l'ambiente qui */
    }

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;
    ui.focus = 1;   /* Catalogo */

    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_MAIN_MENU || !ui.catalogOpen)
    {
        fprintf(stderr, "GameCatalogScreenTest: (vuoto) confirm su Catalogo non apre la vista dentro APP_MAIN_MENU\n");
        if (hadDir) rename("catalog.GameCatalogScreenTest.bak", "catalog");
        return false;
    }
    int total = 0;
    for (int c = 0; c < RUN_CATALOG_CATEGORY_COUNT; c++) total += ui.catalog.entryCount[c];
    bool ok = (total == 0);
    if (!ok) fprintf(stderr, "GameCatalogScreenTest: (vuoto) l'aggregato non e' vuoto su catalog/ assente\n");

    /* "senza crash" per davvero: un frame vero, non solo i dati. */
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, mode, &ui, false, NULL, NULL);
    UnloadRenderTexture(canvas);

    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    if (mode != APP_MAIN_MENU || ui.catalogOpen || ui.focus != 1)
    {
        fprintf(stderr, "GameCatalogScreenTest: (vuoto) ESC non torna a MainMenu con focus su Catalogo\n");
        ok = false;
    }

    if (hadDir) rename("catalog.GameCatalogScreenTest.bak", "catalog");
    return ok;
}

/* Scenario 2 (spec M8: aggregazione, boss sconfitto, navigazione con wrap,
   file corrotto -> ignorato senza crash): due run sintetiche VERE (stessa
   BuildSyntheticGame/DriveWin/DriveLoss di GameCatalogTest sopra) piu' un
   file corrotto scritto a mano. Le due run condividono lo STESSO mondo/
   oggetto/personaggio (fixture fissa di BuildSyntheticGame) apposta: e' cio'
   che permette di verificare runCount/encounterCount aggregati su PIU' di un
   file, non solo "un file si legge". La pulizia di catalog/ (snapshot-based)
   e' responsabilita' della funzione esposta in fondo al file, NON di questo
   scenario -- scrive soltanto. */
static bool CatalogScreenPopulatedScenario(Game *game)
{
    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game gameA;
    BuildSyntheticGame(&gameA, 2);
    {
        AppGen genA = { 0 };
        AppUi uiA = { 0 };
        uiA.catalogWritesEnabled = true;
        uiA.seed = 5001;
        AppMode modeA = APP_GAMEPLAY;
        DriveWin(&gameA, &genA, &uiA, &modeA);
    }

    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game gameB;
    BuildSyntheticGame(&gameB, 1);
    {
        AppGen genB = { 0 };
        AppUi uiB = { 0 };
        uiB.catalogWritesEnabled = true;
        uiB.seed = 5002;
        AppMode modeB = APP_GAMEPLAY;
        DriveLoss(&gameB, &genB, &uiB, &modeB);
    }

    /* Un file corrotto (nessun catalogSchema=1 valido): RunCatalogAggregate lo
       deve ignorare con una riga di log, mai un crash (vedi il commento su
       RunCatalogAggregate in src/content/run_catalog.h). */
    FILE *corrupted = fopen("catalog/GameCatalogScreenTest-corrupted.txt", "w");
    if (corrupted)
    {
        fputs("questo file non e' un record di catalogo valido\nseed=42\n", corrupted);
        fclose(corrupted);
    }
    else fprintf(stderr, "GameCatalogScreenTest: impossibile scrivere il file corrotto di fixture\n");

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;
    ui.focus = 1;   /* Catalogo */
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    CATALOG_SCREEN_TEST_CHECK(ui.catalogOpen, "confirm su Catalogo non apre la vista");
    CATALOG_SCREEN_TEST_CHECK(ui.catalog.filesRead >= 2, "l'aggregato non ha letto almeno i due file scritti da questo scenario");

    const RunCatalogSummary *sum = &ui.catalog;

    /* Aggregazione corretta: conteggi su un tema/oggetto/personaggio comuni
       alle due run VS uno presente in una sola. */
    const RunCatalogEntry *themeCommon = FindCatalogEntry(sum, RUN_CATALOG_CAT_WORLD, "Test Theme 1");
    CATALOG_SCREEN_TEST_CHECK(themeCommon && themeCommon->runCount == 2 && themeCommon->encounterCount == 2,
                               "conteggi errati per il tema comune alle due run (Test Theme 1)");
    const RunCatalogEntry *themeSolo = FindCatalogEntry(sum, RUN_CATALOG_CAT_WORLD, "Test Theme 2");
    CATALOG_SCREEN_TEST_CHECK(themeSolo && themeSolo->runCount == 1, "conteggi errati per il tema della sola run A (Test Theme 2)");

    /* Boss sconfitto (spec M8): Test Boss 1 e' incontrato in ENTRAMBE le run
       ma mai sconfitto; Test Boss 2 e' sconfitto nella run A. */
    const RunCatalogEntry *bossNeverDown = FindCatalogEntry(sum, RUN_CATALOG_CAT_BOSS, "Test Boss 1");
    CATALOG_SCREEN_TEST_CHECK(bossNeverDown && bossNeverDown->runCount == 2 && !bossNeverDown->bossDefeated,
                               "Test Boss 1 (mai sconfitto) risulta sconfitto o coi conteggi sbagliati");
    const RunCatalogEntry *bossDown = FindCatalogEntry(sum, RUN_CATALOG_CAT_BOSS, "Test Boss 2");
    CATALOG_SCREEN_TEST_CHECK(bossDown && bossDown->runCount == 1 && bossDown->bossDefeated,
                               "Test Boss 2 (sconfitto nella run A) non risulta sconfitto");

    /* Oggetto: slot/rarita'/tratti nel dettaglio (spec M8). */
    const RunCatalogEntry *item = FindCatalogEntry(sum, RUN_CATALOG_CAT_ITEM, "Test Blade");
    CATALOG_SCREEN_TEST_CHECK(item && item->runCount == 2 && strstr(item->detail, "rare") && strstr(item->detail, "pierce"),
                               "l'oggetto comune alle due run ha conteggi o dettaglio sbagliati");

    /* Voce personaggio con trait (spec M8: "voce personaggio con trait"). */
    const RunCatalogEntry *character = FindCatalogEntry(sum, RUN_CATALOG_CAT_CHARACTER, "Testforge");
    CATALOG_SCREEN_TEST_CHECK(character && character->runCount == 2 && strstr(character->detail, "on_evaluate"),
                               "il personaggio generato non ha il trait hook nel dettaglio, o i conteggi sono sbagliati");

    /* Navigazione categorie con wrap (7 categorie, spec M8). */
    int startCategory = ui.catalogCategory;
    for (int step = 0; step < RUN_CATALOG_CATEGORY_COUNT; step++)
    {
        AppInput in = InputRight();
        UpdateApp(game, &mode, &gen, &ui, &in);
    }
    CATALOG_SCREEN_TEST_CHECK(ui.catalogCategory == startCategory, "RUN_CATALOG_CATEGORY_COUNT 'destra' non tornano alla categoria di partenza (wrap rotto)");
    { AppInput in = InputLeft(); UpdateApp(game, &mode, &gen, &ui, &in); }
    CATALOG_SCREEN_TEST_CHECK(ui.catalogCategory == (startCategory + RUN_CATALOG_CATEGORY_COUNT - 1) % RUN_CATALOG_CATEGORY_COUNT,
                               "sinistra dalla prima categoria non fa wrap sull'ultima");

    /* Navigazione voci con wrap, dentro la categoria Boss (2 voci: Test Boss
       1/2, gia' verificate sopra -- il caso giusto per un wrap non banale). */
    while (ui.catalogCategory != RUN_CATALOG_CAT_BOSS)
    {
        AppInput in = InputRight();
        UpdateApp(game, &mode, &gen, &ui, &in);
    }
    CATALOG_SCREEN_TEST_CHECK(ui.catalog.entryCount[RUN_CATALOG_CAT_BOSS] == 2, "la categoria Boss non ha le due voci attese per la navigazione");
    CATALOG_SCREEN_TEST_CHECK(ui.catalogItemFocus == 0, "il cambio categoria non riparte dalla prima voce");
    { AppInput in = InputUp(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* su dalla voce 0 -> wrap sull'ultima (1) */
    CATALOG_SCREEN_TEST_CHECK(ui.catalogItemFocus == 1, "su' dalla prima voce non fa wrap sull'ultima");
    { AppInput in = InputDown(); UpdateApp(game, &mode, &gen, &ui, &in); }   /* giu' dall'ultima -> wrap sulla prima (0) */
    CATALOG_SCREEN_TEST_CHECK(ui.catalogItemFocus == 0, "giu' dall'ultima voce non fa wrap sulla prima");

    /* ESC torna a MainMenu con focus su Catalogo (spec M8). */
    { AppInput in = InputBack(); UpdateApp(game, &mode, &gen, &ui, &in); }
    CATALOG_SCREEN_TEST_CHECK(mode == APP_MAIN_MENU && !ui.catalogOpen && ui.focus == 1,
                               "ESC dal Catalogo popolato non torna a MainMenu con focus su Catalogo");

    return true;
}

bool GameCatalogScreenTest(Game *game)
{
    /* Stesso schema di GameCatalogTest sopra: snapshot PRIMA di qualunque
       scenario, pulizia UNA volta sola alla fine -- catalog/ resta esattamente
       com'era (o inesistente) qualunque cosa gli scenari abbiano scritto. */
    bool dirExistedBefore = DirectoryExists("catalog");
    CatalogSnapshot before;
    SnapshotCatalog(&before);

    struct { const char *label; bool (*fn)(Game *); } tests[] = {
        { "1 (catalogo vuoto -> messaggio senza crash)", CatalogScreenEmptyScenario },
        { "2 (catalogo popolato -> aggregazione/boss sconfitto/personaggio con trait/navigazione con wrap/file corrotto)", CatalogScreenPopulatedScenario },
    };
    bool allOk = true;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        printf("-- test %s --\n", tests[i].label);
        bool ok = tests[i].fn(game);
        if (!ok) { fprintf(stderr, "  FALLITO: %s\n", tests[i].label); allOk = false; }
    }

    RemoveManifest();
    CleanupCatalog(&before, dirExistedBefore);
    return allOk;
}

/* ============================================================
   SOLO manuale (mai in make test, stessa tradizione di
   GameFloorZeroScreenshotTest): stessa fixture di
   CatalogScreenPopulatedScenario sopra (due run sintetiche + un file
   corrotto, cosi' lo screenshot mostra la vista con piu' di una voce per
   categoria), ma disegna e scatta DAVVERO invece di solo asserire sui dati.
   Pulizia snapshot-based come ogni altro test del catalogo: catalog/ resta
   com'era anche dopo una corsa manuale.
   ============================================================ */
bool GameCatalogScreenshotTest(Game *game)
{
    bool dirExistedBefore = DirectoryExists("catalog");
    CatalogSnapshot before;
    SnapshotCatalog(&before);

    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game gameA;
    BuildSyntheticGame(&gameA, 2);
    {
        AppGen genA = { 0 };
        AppUi uiA = { 0 };
        uiA.catalogWritesEnabled = true;
        uiA.seed = 6001;
        AppMode modeA = APP_GAMEPLAY;
        DriveWin(&gameA, &genA, &uiA, &modeA);
    }

    WriteManifestSource("local:test-model");
    WriteChosenTheme("Test World", "A blurb for the test world.");
    Game gameB;
    BuildSyntheticGame(&gameB, 1);
    {
        AppGen genB = { 0 };
        AppUi uiB = { 0 };
        uiB.catalogWritesEnabled = true;
        uiB.seed = 6002;
        AppMode modeB = APP_GAMEPLAY;
        DriveLoss(&gameB, &genB, &uiB, &modeB);
    }

    FILE *corrupted = fopen("catalog/GameCatalogScreenshotTest-corrupted.txt", "w");
    if (corrupted)
    {
        fputs("questo file non e' un record di catalogo valido\nseed=42\n", corrupted);
        fclose(corrupted);
    }

    AppGen gen = { 0 };
    AppUi ui = { 0 };
    AppMode mode = APP_MAIN_MENU;
    ui.focus = 1;
    { AppInput in = InputConfirm(); UpdateApp(game, &mode, &gen, &ui, &in); }
    bool ok = ui.catalogOpen;
    if (!ok) fprintf(stderr, "GameCatalogScreenshotTest: confirm su Catalogo non apre la vista\n");
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    RendererDrawApp(game, canvas, mode, &ui, true, NULL, "logs/worldsmelt-catalog-screen.png");
    UnloadRenderTexture(canvas);

    RemoveManifest();
    CleanupCatalog(&before, dirExistedBefore);
    return ok;
}
