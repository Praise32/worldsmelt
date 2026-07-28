/* Test del pool curato di contenuto (W5b, DEC-153) e del layer di
   indirezione immagini: content/curated_catalog.h (formato + loader),
   content/curated_image_map.h (content-id -> image-id), content/
   curated_images.h (CuratedImagesFindById, image-id -> file), e la
   precedenza "curated-content -> fallback deterministico" dentro
   RunContentLoad (content/run_content.c).

   Ogni fixture vive in una cartella TEMPORANEA (mkdtemp), MAI in
   assets/curated-content/: quella cartella e' condivisa col resto del
   progetto (un'altra sessione potrebbe starci scrivendo contenuto vero
   proprio ora) e questo test non deve ne' leggerla ne' scriverla. Lo
   scenario di integrazione motore usa CuratedCatalogSetTestDir (stesso
   schema di RunCatalogSetTestPath in run_catalog.c) per far leggere a
   RunContentLoad la fixture invece di CURATED_CATALOG_DIR reale, e lo
   ripristina SEMPRE a NULL prima di ritornare, anche sui rami di errore. */

#include "tests/game_tests.h"

#include "content/curated_catalog.h"
#include "content/curated_image_map.h"
#include "content/curated_images.h"
#include "content/run_content.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>   /* _mkdir/_rmdir: mkdtemp() non esiste ne' in UCRT ne' nel runtime MinGW-w64 */
#else
#include <unistd.h>   /* mkdtemp */
#endif

#define CURATED_CONTENT_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameCuratedContentTest: %s\n", (msg)); return false; } } while (0)

/* Copia privata di CreateTempCatalogTestDir (src/tests/catalog_tests.c):
   stessa convenzione del progetto, un modulo diverso ha la propria copia
   invece di una funzione condivisa (vedi il commento su ReadManifestValue
   in content/run_catalog.c). Crea una directory temporanea univoca e vuota,
   o NULL se la creazione fallisce. */
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

static void WriteFixtureFile(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fputs(text, f);
    fclose(f);
}

/* --- Scenario 1: il floor DEC-144 (rispettato / violato) ---------------- */

static bool TestFloorRespectedWithOnePerRarity(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-floor-ok"), "impossibile creare la dir temporanea");

    char itemsPath[560];
    snprintf(itemsPath, sizeof(itemsPath), "%s/items.txt", dir);
    WriteFixtureFile(itemsPath,
        "item1.id=test-common\n"
        "item1.name=Test Common\n"
        "item1.kind=passive\n"
        "item1.rarity=common\n"
        "item1.slot=hat\n"
        "item1.traits=bounce\n"
        "item1.color=#112233\n"
        "item1.script=on_hit:projectile,1,300,none\n"
        "item2.id=test-uncommon\n"
        "item2.name=Test Uncommon\n"
        "item2.kind=passive\n"
        "item2.rarity=uncommon\n"
        "item2.slot=hand\n"
        "item2.traits=homing\n"
        "item2.color=#223344\n"
        "item3.id=test-rare\n"
        "item3.name=Test Rare\n"
        "item3.kind=active\n"
        "item3.rarity=rare\n"
        "item3.slot=back\n"
        "item3.traits=explode\n"
        "item3.color=#334455\n"
        "item3.charges=3\n"
        "item4.id=test-legendary\n"
        "item4.name=Test Legendary\n"
        "item4.kind=statup\n"
        "item4.rarity=legendary\n"
        "item4.slot=body\n"
        "item4.color=#445566\n"
        "item4.script=deve-sparire-perche-statup\n");

    CuratedCatalogPool pool;
    bool loaded = CuratedCatalogLoad(dir, &pool);
    remove(itemsPath);
    RemoveTempDir(dir);

    CURATED_CONTENT_CHECK(loaded, "il caricamento con 4 voci (una per rarita') doveva riuscire");
    CURATED_CONTENT_CHECK(pool.itemCount == 4, "attese esattamente 4 voci nel pool");
    CURATED_CONTENT_CHECK(CuratedCatalogValidateFloor(&pool), "il floor DEC-144 doveva risultare rispettato (una voce per rarita')");

    /* Verifiche di lettura sui campi, di riflesso (il loader riusa
       ItemKindFromText/RarityFromText -- questo e' un test del round-trip
       testo->struttura, non una riscoperta di quelle funzioni). */
    CURATED_CONTENT_CHECK(pool.items[2].item.kind == ITEM_ACTIVE, "item3 dichiara charges=3: doveva risolvere a ITEM_ACTIVE, non al passivo storico");
    CURATED_CONTENT_CHECK(pool.items[3].item.kind == ITEM_STATUP, "item4.kind=statup doveva risolvere a ITEM_STATUP");
    CURATED_CONTENT_CHECK(pool.items[3].item.script[0] == '\0', "uno stat-up non deve mai portare uno script mini-VM, anche se il file lo dichiarava");

    return true;
}

static bool TestFloorViolationIsDetected(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-floor-bad"), "impossibile creare la dir temporanea");

    char itemsPath[560];
    snprintf(itemsPath, sizeof(itemsPath), "%s/items.txt", dir);
    /* Solo 3 voci, NESSUNA leggendaria: ItemPoolMinimumCounts(3, standard)
       da' {0,1,1,1} (stesso esempio normativo di ItemPoolTestMinimumCounts,
       src/tests/game_tests.c) -- la leggendaria attesa e' 1, ma il pool non
       ne ha nessuna: violazione. */
    WriteFixtureFile(itemsPath,
        "item1.id=v-common\n"
        "item1.name=V Common\n"
        "item1.kind=passive\n"
        "item1.rarity=common\n"
        "item2.id=v-uncommon\n"
        "item2.name=V Uncommon\n"
        "item2.kind=passive\n"
        "item2.rarity=uncommon\n"
        "item3.id=v-rare\n"
        "item3.name=V Rare\n"
        "item3.kind=passive\n"
        "item3.rarity=rare\n");

    CuratedCatalogPool pool;
    bool loaded = CuratedCatalogLoad(dir, &pool);
    remove(itemsPath);
    RemoveTempDir(dir);

    CURATED_CONTENT_CHECK(loaded, "il caricamento con 3 voci valide doveva comunque riuscire");
    CURATED_CONTENT_CHECK(pool.itemCount == 3, "attese esattamente 3 voci nel pool");
    CURATED_CONTENT_CHECK(!CuratedCatalogValidateFloor(&pool), "un pool di 3 oggetti senza legendary doveva violare il floor DEC-144");

    return true;
}

/* --- Scenario 2: l'indirezione immagini si risolve ----------------------- */

static bool TestImageIndirectionResolves(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-image-map-ok"), "impossibile creare la dir temporanea");

    char mapPath[560], manifestPath[560];
    snprintf(mapPath, sizeof(mapPath), "%s/image-map.txt", dir);
    snprintf(manifestPath, sizeof(manifestPath), "%s/manifest.json", dir);

    WriteFixtureFile(mapPath,
        "# commento, ignorato\n"
        "\n"
        "test-content-id = test-image-id\n"
        "other-content-id=other-image-id\n");
    WriteFixtureFile(manifestPath,
        "{\n"
        "  \"images\": [\n"
        "    { \"id\": \"test-image-id\", \"file\": \"items/test-image.png\", \"category\": \"item\" },\n"
        "    { \"id\": \"other-image-id\", \"file\": \"items/other.png\", \"category\": \"item\" }\n"
        "  ]\n"
        "}\n");

    char imageId[64];
    bool resolvedMap = CuratedImageMapResolve(mapPath, "test-content-id", imageId, sizeof(imageId));
    CuratedImage image;
    int imageIdx = -1;
    bool resolvedName = false;
    if (resolvedMap) resolvedName = CuratedImagesFindById(manifestPath, imageId, &image, &imageIdx);

    remove(mapPath);
    remove(manifestPath);
    RemoveTempDir(dir);

    CURATED_CONTENT_CHECK(resolvedMap, "la risoluzione content-id -> image-id doveva riuscire");
    CURATED_CONTENT_CHECK(strcmp(imageId, "test-image-id") == 0, "image-id risolto inatteso");
    CURATED_CONTENT_CHECK(resolvedName, "la ricerca per id nel manifest fixture doveva riuscire");
    CURATED_CONTENT_CHECK(strcmp(image.file, "items/test-image.png") == 0, "il file risolto non e' quello atteso");
    CURATED_CONTENT_CHECK(imageIdx == 0, "'test-image-id' e' la prima voce del manifest fixture: outIndex atteso 0");

    return true;
}

/* --- Scenario 3: catalogo assente -> il fallback procedurale resta -------
   sia al livello del solo loader, sia -- integrazione vera -- dentro
   RunContentLoad quando nessun override di test dir e' attivo: in questo
   checkout assets/curated-content/ non esiste ancora (il passo successivo
   del lavoro), quindi questo e' anche il comportamento REALE di oggi.

   Correzione round 0 (bloccante, stessa fragilita' di
   TestEngineAppliesCuratedPoolOverFallback sotto, qui piu' blanda perche' le
   verifiche sono generiche): RunContentLoad legge ANCHE
   generated/current_run.txt se esiste (stato normalissimo dopo 'make gen' o
   dopo una run giocata), e quel ramo e' l'AUTORITA' su nomi/kind sopra il
   ripiego procedurale -- questo test vuole esercitare il ripiego DETERMINISTICO
   puro, non un manifest vero che capiti di essere li'. Stesso schema di
   TestFallbackBossItemIsRare (src/tests/script_items_tests.c) e
   GameItemPoolFallbackCoverageTest (src/tests/game_tests.c): rename via
   PRIMA di RunContentLoad, rimesso a posto subito dopo, che il test passi o
   fallisca. */
static bool TestCatalogAbsentFallsBackToProcedural(void)
{
    CuratedCatalogPool pool;
    bool loaded = CuratedCatalogLoad("/percorso/di-sicuro/inesistente-melting-run", &pool);
    CURATED_CONTENT_CHECK(!loaded, "una cartella assente deve far fallire il caricamento");
    CURATED_CONTENT_CHECK(pool.itemCount == 0 && pool.enemyCount == 0 && pool.bossCount == 0,
                           "il pool deve restare azzerato quando il caricamento fallisce, mai a meta'");

    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.curated-absent-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    CuratedCatalogSetTestDir(NULL);   /* nessun override: esattamente il cammino di produzione */
    RunContent content;
    memset(&content, 0, sizeof(content));
    RunContentLoad(&content, 4242u);

    if (hadManifest) rename(kBackup, kManifest);

    CURATED_CONTENT_CHECK(content.floors[0].items[0].active, "il ripiego procedurale deve comunque popolare 3 oggetti attivi sul piano 1");
    CURATED_CONTENT_CHECK(content.floors[0].bossItem.kind == ITEM_STATUP, "il bossItem di ripiego resta sempre uno stat-up (fase 3b review)");
    return true;
}

/* --- Scenario 4: un id immagine mancante ricade sulla resa geometrica ---- */

static bool TestMissingImageIdFallsBack(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-image-map-miss"), "impossibile creare la dir temporanea");

    char mapPath[560], manifestPath[560];
    snprintf(mapPath, sizeof(mapPath), "%s/image-map.txt", dir);
    snprintf(manifestPath, sizeof(manifestPath), "%s/manifest.json", dir);

    /* La mappa referenzia un'immagine che il manifest NON ha (pacchetto
       immagini disallineato dal pool curato, o non ancora aggiornato). */
    WriteFixtureFile(mapPath, "orphan-content = ghost-image\n");
    WriteFixtureFile(manifestPath,
        "{\n  \"images\": [\n    { \"id\": \"real-image\", \"file\": \"items/real.png\", \"category\": \"item\" }\n  ]\n}\n");

    char imageId[64];
    bool resolvedMap = CuratedImageMapResolve(mapPath, "orphan-content", imageId, sizeof(imageId));
    CuratedImage image;
    int imageIdx = 99;   /* valore-sentinella: doveva tornare a -1 sul fallimento */
    bool resolvedName = resolvedMap && CuratedImagesFindById(manifestPath, imageId, &image, &imageIdx);

    char unusedId[64];
    unusedId[0] = 'X'; unusedId[1] = '\0';   /* valore-sentinella per verificare che la funzione lo azzeri sul fallimento */
    bool resolvedUnknownContent = CuratedImageMapResolve(mapPath, "never-declared-content-id", unusedId, sizeof(unusedId));

    remove(mapPath);
    remove(manifestPath);
    RemoveTempDir(dir);

    CURATED_CONTENT_CHECK(resolvedMap, "la mappa doveva comunque risolvere l'image-id: il buco e' nel manifest, non nella mappa");
    CURATED_CONTENT_CHECK(!resolvedName, "un image-id assente dal manifest non deve trovare nessuna voce (fallback immagine)");
    CURATED_CONTENT_CHECK(imageIdx == -1, "outIndex deve tornare a -1 quando la ricerca per id fallisce, mai il valore precedente");
    CURATED_CONTENT_CHECK(!resolvedUnknownContent, "un content-id assente dalla mappa non deve risolvere nulla");
    CURATED_CONTENT_CHECK(unusedId[0] == '\0', "outImageId deve restare azzerata quando la risoluzione fallisce, mai il valore precedente");

    return true;
}

/* --- Scenario 5: integrazione motore, il pool curato SOSTITUISCE i campi
   del contenuto procedurale rispettando la rarita' richiesta per slot ------ */

static bool TestEngineAppliesCuratedPoolOverFallback(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-engine-ok"), "impossibile creare la dir temporanea");

    char itemsPath[560], enemiesPath[560];
    snprintf(itemsPath, sizeof(itemsPath), "%s/items.txt", dir);
    snprintf(enemiesPath, sizeof(enemiesPath), "%s/enemies.txt", dir);

    WriteFixtureFile(itemsPath,
        "item1.id=eng-common\n"
        "item1.name=Engine Common\n"
        "item1.kind=passive\n"
        "item1.rarity=common\n"
        "item1.slot=hat\n"
        "item1.traits=bounce\n"
        "item1.color=#010101\n"
        "item1.script=on_hit:projectile,1,300,none\n"
        "item2.id=eng-uncommon\n"
        "item2.name=Engine Uncommon\n"
        "item2.kind=passive\n"
        "item2.rarity=uncommon\n"
        "item2.slot=hand\n"
        "item2.color=#020202\n"
        "item3.id=eng-rare\n"
        "item3.name=Engine Rare\n"
        "item3.kind=passive\n"
        "item3.rarity=rare\n"
        "item3.slot=back\n"
        "item3.color=#030303\n"
        "item4.id=eng-legendary\n"
        "item4.name=Engine Legendary\n"
        "item4.kind=passive\n"
        "item4.rarity=legendary\n"
        "item4.slot=body\n"
        "item4.color=#040404\n");

    WriteFixtureFile(enemiesPath,
        "enemy1.id=eng-enemy-1\n"
        "enemy1.name=Engine Enemy\n"
        "enemy1.form=spiky\n"
        "enemy1.move=kite\n"
        "enemy1.fire=single\n");

    /* Correzione round 0 (bloccante): RunContentLoad legge ANCHE
       generated/current_run.txt se esiste (stato normalissimo dopo 'make gen'
       o dopo una run giocata) -- quel ramo e' l'AUTORITA' sopra tutto quello
       che questo test vuole verificare (il pool curato passato via
       CuratedCatalogSetTestDir), quindi lo sposta via PRIMA di caricare e lo
       rimette a posto SUBITO dopo, che il test passi o fallisca. Stesso
       schema di TestFallbackBossItemIsRare (src/tests/script_items_tests.c)
       e GameItemPoolFallbackCoverageTest (src/tests/game_tests.c). */
    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.curated-engine-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    CuratedCatalogSetTestDir(dir);
    RunContent content;
    memset(&content, 0, sizeof(content));
    RunContentLoad(&content, 99u);
    CuratedCatalogSetTestDir(NULL);   /* ripristinato SUBITO, prima di ogni verifica che potrebbe uscire presto */
    if (hadManifest) rename(kBackup, kManifest);

    remove(itemsPath);
    remove(enemiesPath);
    RemoveTempDir(dir);

    for (int f = 0; f < FLOOR_COUNT; f++)
    {
        for (int i = 0; i < 3; i++)
        {
            const char *name = content.floors[f].items[i].name;
            if (strncmp(name, "Engine ", 7) != 0)
            {
                fprintf(stderr, "GameCuratedContentTest: piano %d oggetto %d atteso dal pool curato (\"Engine ...\"), trovato \"%s\"\n", f + 1, i + 1, name);
                return false;
            }
        }
    }
    CURATED_CONTENT_CHECK(strcmp(content.floors[0].enemies[0].name, "Engine Enemy") == 0,
                           "il tipo di nemico 1 del piano 1 doveva venire dal pool curato di enemies.txt");
    CURATED_CONTENT_CHECK(content.floors[0].enemies[0].form == ENEMY_FORM_SPIKY, "la forma del nemico curato non e' stata letta correttamente");

    return true;
}

/* --- Scenario 5b: correzione round 0 (bloccante) -- un oggetto curato
   kind=statup non deve MAI portare il tipo di colpo del piano ------------
   Riproduce esattamente il pool che ha fatto scattare il difetto in
   revisione (comune/non-comune statup, raro/leggendario passivi): con
   ItemPoolMinimumCounts(15, standard) gli slot comuni e non-comuni sono la
   maggioranza dei 15 dell'intera run, quindi il pool curato di 4 voci
   finisce ripetutamente su una posizione statup -- se quella posizione e'
   anche lo shotOwner scelto da GenerateFallbackContent (un piano su tre,
   in media), l'invariante "un tipo di colpo per piano, mai su uno stat-up"
   si romperebbe senza il fix. Copre piu' semi (non solo 99u come lo
   scenario 5) proprio perche' il difetto originale dipendeva dal seed:
   23 piani su 25 nella riproduzione con l'harness. */

static bool TestCuratedStatupNeverCarriesShotType(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-statup-shot"), "impossibile creare la dir temporanea");

    char itemsPath[560];
    snprintf(itemsPath, sizeof(itemsPath), "%s/items.txt", dir);
    WriteFixtureFile(itemsPath,
        "item1.id=su-common\n"
        "item1.name=SU Common\n"
        "item1.kind=statup\n"
        "item1.rarity=common\n"
        "item2.id=su-uncommon\n"
        "item2.name=SU Uncommon\n"
        "item2.kind=statup\n"
        "item2.rarity=uncommon\n"
        "item3.id=pv-rare\n"
        "item3.name=PV Rare\n"
        "item3.kind=passive\n"
        "item3.rarity=rare\n"
        "item3.traits=explode\n"
        "item3.script=on_hit:projectile,1,300,none\n"
        "item4.id=pv-legendary\n"
        "item4.name=PV Legendary\n"
        "item4.kind=passive\n"
        "item4.rarity=legendary\n"
        "item4.traits=bounce\n"
        "item4.script=on_hit:projectile,1,300,none\n");

    bool ok = true;
    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.curated-statup-shot-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    for (unsigned int seed = 1; seed <= 25 && ok; seed++)
    {
        CuratedCatalogSetTestDir(dir);
        RunContent content;
        memset(&content, 0, sizeof(content));
        RunContentLoad(&content, seed);
        CuratedCatalogSetTestDir(NULL);

        for (int f = 0; f < FLOOR_COUNT && ok; f++)
        {
            int activeCount = 0;
            for (int i = 0; i < 3; i++)
            {
                if (!content.floors[f].items[i].shotType.active) continue;
                activeCount++;
                if (content.floors[f].items[i].kind == ITEM_STATUP)
                {
                    fprintf(stderr,
                            "GameCuratedContentTest: seed %u piano %d oggetto %d e' kind=statup e porta un tipo di colpo (shotType.active), vietato dalla tassonomia\n",
                            seed, f + 1, i + 1);
                    ok = false;
                }
            }
            if (activeCount > 1)
            {
                fprintf(stderr, "GameCuratedContentTest: seed %u piano %d ha %d tipi di colpo attivi, atteso al massimo 1\n", seed, f + 1, activeCount);
                ok = false;
            }
        }
    }

    if (hadManifest) rename(kBackup, kManifest);
    remove(itemsPath);
    RemoveTempDir(dir);

    return ok;
}

/* --- Scenario 5c: correzione round 1 (bloccante) -- l'indirezione immagini
   si risolve DENTRO ApplyCuratedCatalog (non solo nelle funzioni isolate
   dello Scenario 2), su ENTRAMBI i rami: il pool curato (imagePath/
   curatedImageIdx risolti e coerenti col manifest immagini) e il manifest di
   una run generata (che deve azzerarli di nuovo, mai ereditare l'immagine di
   un content-id curato che non c'entra piu' nulla -- il difetto bloccante
   del round 0). L'isolamento e' completo: la mappa si compone dalla stessa
   cartella del catalogo (nessun bisogno di un override a parte), il
   manifest immagini usa CuratedImagesSetTestManifestPath -- nessuno dei due
   rami tocca mai assets/curated-content/ o assets/curated/ reali. */

/* Il pool ha ESATTAMENTE una voce per rarita': ItemPoolMinimumCounts(15,
   standard) garantisce almeno uno slot per rarita' su 15 (DEC-144), quindi
   ogni slot della run risolve SEMPRE a una delle 4 voci qui sotto (roll % 1
   e' sempre 0) -- il test non dipende dal seed ne' deve indovinare quale
   slot ottiene quale rarita', gli basta leggere content->floors[f].items[i].rarity
   di ritorno per sapere quale file/indice aspettarsi. */
static void ExpectedImageForRarity(Rarity rarity, const char **outFile, int *outIdx)
{
    switch (rarity)
    {
        case RARITY_COMMON:    *outFile = "items/e2e-common.png";    *outIdx = 0; return;
        case RARITY_UNCOMMON:  *outFile = "items/e2e-uncommon.png";  *outIdx = 1; return;
        case RARITY_RARE:      *outFile = "items/e2e-rare.png";      *outIdx = 2; return;
        case RARITY_LEGENDARY: *outFile = "items/e2e-legendary.png"; *outIdx = 3; return;
        default:               *outFile = "";                       *outIdx = -1; return;
    }
}

static bool TestEngineResolvesImageIndirectionEndToEnd(void)
{
    char dir[512];
    CURATED_CONTENT_CHECK(CreateTempDir(dir, sizeof(dir), "curated-image-e2e"), "impossibile creare la dir temporanea");

    char itemsPath[560], mapPath[560], manifestPath[560];
    snprintf(itemsPath, sizeof(itemsPath), "%s/items.txt", dir);
    snprintf(mapPath, sizeof(mapPath), "%s/%s", dir, CURATED_IMAGE_MAP_FILE);
    snprintf(manifestPath, sizeof(manifestPath), "%s/manifest.json", dir);

    WriteFixtureFile(itemsPath,
        "item1.id=e2e-common\n"
        "item1.name=E2E Common\n"
        "item1.kind=passive\n"
        "item1.rarity=common\n"
        "item1.slot=hat\n"
        "item1.color=#0a0a0a\n"
        "item2.id=e2e-uncommon\n"
        "item2.name=E2E Uncommon\n"
        "item2.kind=passive\n"
        "item2.rarity=uncommon\n"
        "item2.slot=hand\n"
        "item2.color=#0b0b0b\n"
        "item3.id=e2e-rare\n"
        "item3.name=E2E Rare\n"
        "item3.kind=passive\n"
        "item3.rarity=rare\n"
        "item3.slot=back\n"
        "item3.color=#0c0c0c\n"
        "item4.id=e2e-legendary\n"
        "item4.name=E2E Legendary\n"
        "item4.kind=passive\n"
        "item4.rarity=legendary\n"
        "item4.slot=body\n"
        "item4.color=#0d0d0d\n");
    WriteFixtureFile(mapPath,
        "e2e-common = e2e-common-img\n"
        "e2e-uncommon = e2e-uncommon-img\n"
        "e2e-rare = e2e-rare-img\n"
        "e2e-legendary = e2e-legendary-img\n");
    WriteFixtureFile(manifestPath,
        "{\n  \"images\": [\n"
        "    { \"id\": \"e2e-common-img\", \"file\": \"items/e2e-common.png\", \"category\": \"item\" },\n"
        "    { \"id\": \"e2e-uncommon-img\", \"file\": \"items/e2e-uncommon.png\", \"category\": \"item\" },\n"
        "    { \"id\": \"e2e-rare-img\", \"file\": \"items/e2e-rare.png\", \"category\": \"item\" },\n"
        "    { \"id\": \"e2e-legendary-img\", \"file\": \"items/e2e-legendary.png\", \"category\": \"item\" }\n"
        "  ]\n}\n");

    static const char *kManifest = "generated/current_run.txt";
    static const char *kBackup = "generated/current_run.txt.curated-image-e2e-test-bak";
    bool hadManifest = (rename(kManifest, kBackup) == 0);

    bool ok = true;

    /* Fase A: SOLO il ramo curato (nessun generated/current_run.txt) --
       ogni slot deve risolvere all'immagine giusta per la sua rarita'. */
    CuratedCatalogSetTestDir(dir);
    CuratedImagesSetTestManifestPath(manifestPath);
    RunContent contentA;
    memset(&contentA, 0, sizeof(contentA));
    RunContentLoad(&contentA, 12345u);
    CuratedCatalogSetTestDir(NULL);
    CuratedImagesSetTestManifestPath(NULL);

    for (int f = 0; f < FLOOR_COUNT && ok; f++)
    {
        for (int i = 0; i < 3 && ok; i++)
        {
            const char *expectFile; int expectIdx;
            ExpectedImageForRarity(contentA.floors[f].items[i].rarity, &expectFile, &expectIdx);
            if (strcmp(contentA.floors[f].items[i].imagePath, expectFile) != 0)
            {
                fprintf(stderr, "GameCuratedContentTest: e2e fase A piano %d oggetto %d imagePath \"%s\", atteso \"%s\"\n",
                        f + 1, i + 1, contentA.floors[f].items[i].imagePath, expectFile);
                ok = false;
            }
            if (contentA.floors[f].curatedImageIdx[i] != expectIdx)
            {
                fprintf(stderr, "GameCuratedContentTest: e2e fase A piano %d oggetto %d curatedImageIdx %d, atteso %d\n",
                        f + 1, i + 1, contentA.floors[f].curatedImageIdx[i], expectIdx);
                ok = false;
            }
        }
    }

    /* Fase B: un generated/current_run.txt esiste (il caso normale dopo
       'make gen') -- il ramo manifest e' l'AUTORITA' e deve azzerare
       imagePath/curatedImageIdx su OGNI oggetto, anche quelli che il pool
       curato ha appena risolto in fase A: e' esattamente il bloccante del
       round 0 (l'immagine di un content-id curato diverso appiccicata a un
       oggetto generato). Il contenuto del manifest non serve a nulla oltre
       a esistere: il reset e' incondizionato, per costruzione. */
    WriteFixtureFile(kManifest, "atlas.path=generated/current_atlas.png\n");

    CuratedCatalogSetTestDir(dir);
    CuratedImagesSetTestManifestPath(manifestPath);
    RunContent contentB;
    memset(&contentB, 0, sizeof(contentB));
    RunContentLoad(&contentB, 12345u);
    CuratedCatalogSetTestDir(NULL);
    CuratedImagesSetTestManifestPath(NULL);

    remove(kManifest);
    if (hadManifest) rename(kBackup, kManifest);

    for (int f = 0; f < FLOOR_COUNT && ok; f++)
    {
        for (int i = 0; i < 3 && ok; i++)
        {
            if (contentB.floors[f].items[i].imagePath[0] != '\0')
            {
                fprintf(stderr, "GameCuratedContentTest: e2e fase B piano %d oggetto %d imagePath \"%s\" non azzerato dal ramo manifest\n",
                        f + 1, i + 1, contentB.floors[f].items[i].imagePath);
                ok = false;
            }
            if (contentB.floors[f].curatedImageIdx[i] != -1)
            {
                fprintf(stderr, "GameCuratedContentTest: e2e fase B piano %d oggetto %d curatedImageIdx %d, atteso -1 dopo il ramo manifest\n",
                        f + 1, i + 1, contentB.floors[f].curatedImageIdx[i]);
                ok = false;
            }
        }
    }

    remove(itemsPath);
    remove(mapPath);
    remove(manifestPath);
    RemoveTempDir(dir);

    return ok;
}

/* --- Scenario 6: CuratedCatalogPickItem, la sola logica di estrazione --- */

static bool TestPickItemRespectsRarityAndDegradesToNull(void)
{
    CuratedCatalogPool pool;
    memset(&pool, 0, sizeof(pool));
    pool.itemCount = 2;
    snprintf(pool.items[0].id, sizeof(pool.items[0].id), "a");
    pool.items[0].item.rarity = RARITY_COMMON;
    snprintf(pool.items[0].item.name, sizeof(pool.items[0].item.name), "A");
    snprintf(pool.items[1].id, sizeof(pool.items[1].id), "b");
    pool.items[1].item.rarity = RARITY_COMMON;
    snprintf(pool.items[1].item.name, sizeof(pool.items[1].item.name), "B");

    const CuratedCatalogItem *pick0 = CuratedCatalogPickItem(&pool, RARITY_COMMON, 0u);
    const CuratedCatalogItem *pick1 = CuratedCatalogPickItem(&pool, RARITY_COMMON, 1u);
    CURATED_CONTENT_CHECK(pick0 != NULL && pick1 != NULL, "un pool con 2 voci comuni deve sempre pescare qualcosa per RARITY_COMMON");
    CURATED_CONTENT_CHECK(strcmp(pick0->id, "a") == 0 && strcmp(pick1->id, "b") == 0,
                           "roll % count deve scorrere le voci disponibili in ordine deterministico");

    const CuratedCatalogItem *missing = CuratedCatalogPickItem(&pool, RARITY_LEGENDARY, 0u);
    CURATED_CONTENT_CHECK(missing == NULL, "un pool senza nessuna voce leggendaria deve tornare NULL, mai un indice inventato");

    return true;
}

bool GameCuratedContentTest(Game *game)
{
    (void)game;   /* le fixture di questo test sono file di testo indipendenti dallo Game: nessun bisogno di GameResetRun/AssetsLoad */
    bool ok = true;

    if (!TestFloorRespectedWithOnePerRarity()) ok = false;
    if (!TestFloorViolationIsDetected()) ok = false;
    if (!TestImageIndirectionResolves()) ok = false;
    if (!TestCatalogAbsentFallsBackToProcedural()) ok = false;
    if (!TestMissingImageIdFallsBack()) ok = false;
    if (!TestEngineAppliesCuratedPoolOverFallback()) ok = false;
    if (!TestEngineResolvesImageIndirectionEndToEnd()) ok = false;
    if (!TestCuratedStatupNeverCarriesShotType()) ok = false;
    if (!TestPickItemRespectsRarityAndDegradesToNull()) ok = false;

    return ok;
}
