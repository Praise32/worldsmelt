/* Test del pacchetto artistico originale (W8): il parser dei manifest di
   assets/art/ (src/assets/art_atlas.h), l'animatore deterministico, la
   risoluzione a priorita' degli image-id e il degrado quando un asset manca o
   e' corrotto.
 *
   Le fixture vivono in una cartella TEMPORANEA (mkdtemp), MAI in assets/art/:
   quella cartella e' il consegnato della sessione artistica, cambia a ogni
   giro di produzione, e un test che vi si appoggiasse si romperebbe al
   prossimo sprite ridisegnato -- oltre a poter leggere un file che un'altra
   sessione sta scrivendo proprio ora. Stessa disciplina di
   curated_content_tests.c (CuratedCatalogSetTestDir) e catalog_tests.c.
 *
   Gran parte di questo test NON ha bisogno della finestra: ArtAtlasParseManifest
   e ArtAnimFrameAt/ArtAnimDone sono pure per costruzione (nessuna chiamata
   raylib), ed e' proprio per poterle esercitare cosi' che il confine
   assets/render e' stato tagliato dove e' stato tagliato. Gli scenari che
   caricano una texture vera girano comunque dopo InitWindow, come il resto
   della suite. */

#include "tests/game_tests.h"

#include "assets/art_atlas.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>   /* mkdir */
#include <sys/types.h>
#include <unistd.h>     /* mkdtemp */
#endif

#define ART_CHECK(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "GameArtAtlasTest: %s\n", (msg)); return false; } } while (0)

/* Come CREATE_TEMP_* negli altri file di test del progetto: copia privata,
   nessuna dipendenza fra moduli di test. */
static bool CreateTempArtDir(char *outPath, int outSize)
{
#ifdef _WIN32
    static int counter = 0;
    snprintf(outPath, (size_t)outSize, "art-test-%d-%d", (int)GetTime(), counter++);
    return _mkdir(outPath) == 0;
#else
    char templatePath[] = "/tmp/worldsmelt-art-XXXXXX";
    char *made = mkdtemp(templatePath);
    if (!made) return false;
    snprintf(outPath, (size_t)outSize, "%s", made);
    return true;
#endif
}

static bool WriteTextFile(const char *dir, const char *name, const char *text)
{
    char path[800];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fputs(text, f);
    fclose(f);
    return true;
}

static void RemoveFile(const char *dir, const char *name)
{
    char path[800];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    remove(path);
}

/* Scrive uno spritesheet FINTO ma valido: una striscia di 'cols' x 'rows'
   fotogrammi da 'frame' pixel, tutta opaca. Serve solo a far riuscire
   LoadTexture; nessuno scenario guarda i pixel. */
static bool WritePngSheet(const char *dir, const char *name, int frame, int cols, int rows)
{
    Image image = GenImageColor(frame*cols, frame*rows, (Color){ 200, 120, 60, 255 });
    if (!image.data) return false;
    char path[800];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    bool ok = ExportImage(image, path);
    UnloadImage(image);
    return ok;
}

/* --- Scenario 1: il contratto BASE degli spritesheet ---------------------- */
static bool ScenarioParseSpriteSheet(void)
{
    /* Il manifest e' scritto ESATTAMENTE come la pipeline lo emette (una riga,
       nessuno spazio superfluo): se il parser reggesse solo la versione
       indentata, passerebbe qui e fallirebbe su assets/art/ vero. */
    const char *text =
        "{\"frame_w\":32,\"frame_h\":32,\"anchor\":[16,28],\"anims\":{"
        "\"walk_down\":{\"row\":0,\"frames\":4,\"fps\":8,\"loop\":true},"
        "\"idle\":{\"row\":4,\"frames\":2,\"fps\":2,\"loop\":true},"
        "\"death\":{\"row\":6,\"frames\":4,\"fps\":8,\"loop\":false}}}";
    ArtSheet sheet;
    ART_CHECK(ArtAtlasParseManifest(text, &sheet), "manifest spritesheet valido rifiutato");
    ART_CHECK(sheet.frameW == 32 && sheet.frameH == 32, "frame_w/frame_h letti male");
    ART_CHECK(sheet.anchorX == 16 && sheet.anchorY == 28, "anchor letta male");
    ART_CHECK(sheet.animCount == 3, "numero di animazioni inatteso");

    const ArtAnim *walk = ArtSheetAnim(&sheet, "walk_down");
    ART_CHECK(walk != NULL, "walk_down non trovata");
    ART_CHECK(walk->row == 0 && walk->frames == 4 && walk->fps == 8 && walk->loop,
              "campi di walk_down letti male");
    const ArtAnim *death = ArtSheetAnim(&sheet, "death");
    ART_CHECK(death != NULL, "death non trovata");
    ART_CHECK(death->row == 6 && death->frames == 4 && !death->loop, "campi di death letti male");
    ART_CHECK(ArtSheetAnim(&sheet, "attack") == NULL, "un'animazione assente non deve essere inventata");

    /* Il contratto dice che la mappa anims e' APERTA e che il motore ignora le
       animazioni che non conosce: si verifica il rovescio, cioe' che una
       CHIAVE sconosciuta al parser (a qualunque livello) non gli faccia
       perdere il resto del file. */
    const char *extended =
        "{\"version\":3,\"frame_w\":48,\"frame_h\":48,\"sockets\":{\"mano\":[10,20]},"
        "\"anchor\":[24,45],\"hitbox\":[1,2,3,4],"
        "\"anims\":{\"walk\":{\"row\":0,\"frames\":4,\"fps\":9,\"loop\":true,\"events\":{\"1\":[\"telegraph\"]}}}}";
    ART_CHECK(ArtAtlasParseManifest(extended, &sheet), "manifest ESTESO rifiutato: il parser non e' tollerante");
    ART_CHECK(sheet.frameW == 48 && sheet.anchorY == 45, "chiavi note perse dopo una chiave sconosciuta");
    ART_CHECK(sheet.animCount == 1 && ArtSheetAnim(&sheet, "walk") != NULL,
              "animazione persa dopo una chiave sconosciuta annidata");
    return true;
}

/* --- Scenario 2: le tre ESTENSIONI del contratto -------------------------- */
static bool ScenarioParseExtensions(void)
{
    ArtSheet sheet;

    /* 9-patch: la chiave 'slice' in piu' su uno spritesheet normale. */
    const char *panel =
        "{\"frame_w\":24,\"frame_h\":24,\"anchor\":[0,0],\"slice\":[6,6,6,6],"
        "\"anims\":{\"idle\":{\"row\":0,\"frames\":1,\"fps\":1,\"loop\":true}}}";
    ART_CHECK(ArtAtlasParseManifest(panel, &sheet), "manifest 9-patch rifiutato");
    ART_CHECK(sheet.sliceL == 6 && sheet.sliceT == 6 && sheet.sliceR == 6 && sheet.sliceB == 6,
              "slice letta male");

    /* Tileset: nessun frame_w/anims, ma tile_w/tile_h/grid/tiles. */
    const char *tileset =
        "{\"tile_w\":32,\"tile_h\":32,\"grid\":[8,5],\"tiles\":{"
        "\"floor\":[0,0],\"wall_n\":[4,0],\"door_e_chiusa\":[6,2],\"void_deg\":[4,4]}}";
    ART_CHECK(ArtAtlasParseManifest(tileset, &sheet), "manifest tileset rifiutato");
    ART_CHECK(sheet.tileW == 32 && sheet.tileH == 32, "tile_w/tile_h letti male");
    ART_CHECK(sheet.gridCols == 8 && sheet.gridRows == 5, "grid letta male");
    ART_CHECK(sheet.roleCount == 4, "numero di ruoli inatteso");
    ART_CHECK(sheet.animCount == 0, "un tileset non ha animazioni");
    /* Senza texture, ArtSheetTileRect non puo' validare i limiti: qui si
       verifica solo che i ruoli siano in tabella con le celle giuste. */
    bool foundDoor = false;
    for (int i = 0; i < sheet.roleCount; i++)
    {
        if (strcmp(sheet.roles[i].name, "door_e_chiusa") != 0) continue;
        foundDoor = true;
        ART_CHECK(sheet.roles[i].col == 6 && sheet.roles[i].row == 2, "cella di door_e_chiusa letta male");
    }
    ART_CHECK(foundDoor, "ruolo door_e_chiusa non trovato");

    /* Font: un terzo insieme di chiavi, compresa baseline_y (presente nei file
       reali e non ancora elencata nel documento di contratto). I nomi dei
       glifi sono caratteri singoli, alcuni dei quali sono anche punteggiatura
       JSON ('/', ':', ','): e' il caso che un parser a strstr sbaglierebbe. */
    const char *font =
        "{\"glyph_h\":5,\"baseline_y\":1,\"space_w\":3,\"letter_spacing\":1,\"glyphs\":{"
        "\"A\":{\"x\":0,\"w\":3},\"M\":{\"x\":48,\"w\":5},\":\":{\"x\":150,\"w\":1},"
        "\"/\":{\"x\":152,\"w\":3},\",\":{\"x\":182,\"w\":1}}}";
    ART_CHECK(ArtAtlasParseManifest(font, &sheet), "manifest font rifiutato");
    ART_CHECK(sheet.glyphH == 5 && sheet.baselineY == 1, "glyph_h/baseline_y letti male");
    ART_CHECK(sheet.spaceW == 3 && sheet.letterSpacing == 1, "space_w/letter_spacing letti male");
    ART_CHECK(sheet.glyphCount == 5, "numero di glifi inatteso");
    const ArtGlyph *m = ArtSheetGlyph(&sheet, 'M');
    ART_CHECK(m != NULL && m->x == 48 && m->w == 5, "glifo M letto male");
    const ArtGlyph *slash = ArtSheetGlyph(&sheet, '/');
    ART_CHECK(slash != NULL && slash->x == 152 && slash->w == 3,
              "glifo '/' letto male: un nome-glifo di punteggiatura confonde il parser");
    const ArtGlyph *comma = ArtSheetGlyph(&sheet, ',');
    ART_CHECK(comma != NULL && comma->x == 182, "glifo ',' letto male (la virgola e' anche separatore JSON)");
    ART_CHECK(ArtSheetGlyph(&sheet, 'Z') == NULL, "un glifo assente non deve essere inventato");
    return true;
}

/* --- Scenario 3: manifest ROTTI ------------------------------------------ */
static bool ScenarioParseBroken(void)
{
    ArtSheet sheet;
    /* Ognuno di questi deve tornare false SENZA leggere fuori dal buffer e
       senza girare a vuoto: e' l'unico modo di garantire che un PNG/JSON
       corrotto sul disco del giocatore non blocchi il gioco. */
    const char *broken[] = {
        "",
        "{",
        "}",
        "non e' nemmeno json",
        "{\"frame_w\":32",
        "{\"frame_w\":32,\"frame_h\":32,\"anchor\":[16",
        "{\"frame_w\":32,\"frame_h\":32,\"anims\":{",
        "{\"frame_w\":32,\"frame_h\":32,\"anims\":{\"walk\":{\"row\":0}}}",   /* frames assente: animazione scartata */
        "{\"frame_w\":0,\"frame_h\":0,\"anims\":{\"walk\":{\"row\":0,\"frames\":4,\"fps\":8,\"loop\":true}}}",
        "{\"tiles\":{\"floor\":[0,0]}}",                                       /* tile_w/tile_h assenti */
        "{\"glyphs\":{\"A\":{\"x\":0,\"w\":3}}}",                              /* glyph_h assente */
        NULL
    };
    for (int i = 0; broken[i]; i++)
    {
        if (ArtAtlasParseManifest(broken[i], &sheet))
        {
            fprintf(stderr, "GameArtAtlasTest: manifest rotto #%d accettato: \"%s\"\n", i, broken[i]);
            return false;
        }
        ART_CHECK(!sheet.manifestOk, "manifestOk deve restare falso su un manifest rotto");
    }
    ART_CHECK(!ArtAtlasParseManifest(NULL, &sheet), "testo NULL deve essere rifiutato");
    ART_CHECK(!ArtAtlasParseManifest("{}", &sheet), "un oggetto vuoto non e' un manifest usabile");
    return true;
}

/* --- Scenario 4: l'ANIMATORE, deterministico ----------------------------- */
static bool ScenarioAnimator(void)
{
    ArtAnim loop = { "walk", 0, 4, 8, true };      /* 4 fotogrammi a 8 fps: 0.125 s ciascuno */
    ArtAnim once = { "death", 3, 4, 8, false };

    /* Confini esatti: il fotogramma cambia ALLO scadere di 1/fps, non prima. */
    ART_CHECK(ArtAnimFrameAt(&loop, 0.0f) == 0, "t=0 deve dare il primo fotogramma");
    ART_CHECK(ArtAnimFrameAt(&loop, 0.124f) == 0, "poco prima di 1/fps si resta sul primo fotogramma");
    ART_CHECK(ArtAnimFrameAt(&loop, 0.125f) == 1, "a 1/fps esatti si passa al secondo fotogramma");
    ART_CHECK(ArtAnimFrameAt(&loop, 0.375f) == 3, "a 3/fps si e' sull'ultimo fotogramma");
    ART_CHECK(ArtAnimFrameAt(&loop, 0.5f) == 0, "un'animazione che cicla riparte da zero dopo l'ultimo");
    ART_CHECK(ArtAnimFrameAt(&loop, 1.125f) == 1, "il ciclo si ripete identico al giro dopo");

    /* Determinismo: la stessa 'elapsed' deve dare lo stesso fotogramma sempre,
       ed e' questo che rende riproducibile uno screenshot test. */
    for (int i = 0; i < 40; i++)
    {
        float t = (float)i*0.05f;
        ART_CHECK(ArtAnimFrameAt(&loop, t) == ArtAnimFrameAt(&loop, t),
                  "ArtAnimFrameAt non e' deterministica");
    }

    /* Chi non cicla si ferma sull'ultimo e ci resta, per sempre. */
    ART_CHECK(ArtAnimFrameAt(&once, 0.5f) == 3, "un'animazione che non cicla si ferma sull'ultimo fotogramma");
    ART_CHECK(ArtAnimFrameAt(&once, 60.0f) == 3, "e ci resta anche molto dopo");
    ART_CHECK(!ArtAnimDone(&once, 0.49f), "ArtAnimDone non deve scattare prima della fine");
    ART_CHECK(ArtAnimDone(&once, 0.5f), "ArtAnimDone deve scattare a 4/8 secondi esatti");
    ART_CHECK(!ArtAnimDone(&loop, 1000.0f), "un'animazione che cicla non finisce mai");

    /* Difese: nessuna divisione per zero, nessun indice negativo. */
    ArtAnim zeroFps = { "x", 0, 4, 0, true };
    ArtAnim zeroFrames = { "x", 0, 0, 8, true };
    ART_CHECK(ArtAnimFrameAt(&zeroFps, 5.0f) == 0, "fps 0 deve dare uno sprite fermo");
    ART_CHECK(ArtAnimFrameAt(&zeroFrames, 5.0f) == 0, "frames 0 deve dare uno sprite fermo");
    ART_CHECK(ArtAnimFrameAt(&loop, -3.0f) == 0, "un tempo negativo deve dare il primo fotogramma");
    ART_CHECK(ArtAnimFrameAt(NULL, 1.0f) == 0, "anim NULL non deve essere un crash");
    ART_CHECK(ArtAnimDone(NULL, 1.0f), "ArtAnimDone(NULL) deve dire 'finita', non toccare memoria");
    return true;
}

/* --- Scenario 5: caricamento vero, cache e priorita' degli image-id ------- */
static bool ScenarioLoadAndPriority(const char *dir)
{
    char itemsDir[640], enemiesDir[640];
    snprintf(itemsDir, sizeof(itemsDir), "%s/items", dir);
    snprintf(enemiesDir, sizeof(enemiesDir), "%s/enemies", dir);
#ifdef _WIN32
    _mkdir(itemsDir); _mkdir(enemiesDir);
#else
    if (mkdir(itemsDir, 0700) != 0 || mkdir(enemiesDir, 0700) != 0) return false;
#endif

    const char *itemManifest =
        "{\"frame_w\":32,\"frame_h\":32,\"anchor\":[16,26],\"anims\":{"
        "\"idle\":{\"row\":0,\"frames\":1,\"fps\":1,\"loop\":true},"
        "\"glow\":{\"row\":1,\"frames\":2,\"fps\":4,\"loop\":true}}}";
    const char *enemyManifest =
        "{\"frame_w\":32,\"frame_h\":32,\"anchor\":[16,30],\"anims\":{"
        "\"walk\":{\"row\":0,\"frames\":4,\"fps\":8,\"loop\":true}}}";
    ART_CHECK(WriteTextFile(itemsDir, "prova-oggetto.json", itemManifest), "fixture: json oggetto non scritto");
    ART_CHECK(WritePngSheet(itemsDir, "prova-oggetto.png", 32, 2, 2), "fixture: png oggetto non scritto");
    ART_CHECK(WriteTextFile(enemiesDir, "prova-nemico.json", enemyManifest), "fixture: json nemico non scritto");
    ART_CHECK(WritePngSheet(enemiesDir, "prova-nemico.png", 32, 4, 1), "fixture: png nemico non scritto");
    /* Un manifest ROTTO col suo PNG a fianco: e' il caso "asset corrotto" del
       requisito di robustezza -- deve degradare, non far cadere il gioco. */
    ART_CHECK(WriteTextFile(itemsDir, "rotto.json", "{\"frame_w\":32,"), "fixture: json rotto non scritto");
    ART_CHECK(WritePngSheet(itemsDir, "rotto.png", 32, 1, 1), "fixture: png di rotto non scritto");
    /* Un manifest valido SENZA il suo PNG: l'altra meta' dello stesso caso. */
    ART_CHECK(WriteTextFile(itemsDir, "senza-png.json", itemManifest), "fixture: json senza-png non scritto");

    ArtAtlasSetTestDir(dir);

    const ArtSheet *item = ArtAtlasGet("items/prova-oggetto");
    ART_CHECK(item != NULL, "sheet valido non caricato dalla fixture");
    ART_CHECK(item->frameW == 32 && item->anchorY == 26, "manifest della fixture letto male");
    ART_CHECK(item->textureOk && item->texture.id != 0, "texture della fixture non caricata");
    /* La cache deve restituire LO STESSO oggetto, non ricaricare: se
       ricaricasse, ogni frame di disegno aprirebbe due file. */
    int cachedAfterFirst = ArtAtlasCachedCount();
    ART_CHECK(ArtAtlasGet("items/prova-oggetto") == item, "la cache non restituisce lo stesso sheet");
    ART_CHECK(ArtAtlasCachedCount() == cachedAfterFirst, "un secondo ArtAtlasGet ha aggiunto una voce in cache");

    /* Il rettangolo sorgente: griglia coerente col PNG, e clamp (non modulo)
       fuori dai limiti. */
    Rectangle r0 = ArtSheetFrameRect(item, 1, 1);
    ART_CHECK(r0.x == 32.0f && r0.y == 32.0f && r0.width == 32.0f, "ArtSheetFrameRect sbagliato");
    Rectangle rOut = ArtSheetFrameRect(item, 9, 9);
    ART_CHECK(rOut.x == 32.0f && rOut.y == 32.0f,
              "un fotogramma fuori dal PNG deve essere CLAMPATO all'ultimo valido, non riavvolto");

    /* Priorita': un image-id si risolve scandendo le categorie, e uno
       inesistente non trova nulla (chi chiama scende al gradino successivo). */
    ART_CHECK(ArtAtlasFindByImageId("prova-oggetto") == item, "image-id di un oggetto non risolto");
    const ArtSheet *enemy = ArtAtlasFindByImageId("prova-nemico");
    ART_CHECK(enemy != NULL && enemy != item, "image-id di un nemico non risolto");
    ART_CHECK(ArtSheetAnim(enemy, "walk") != NULL, "l'animazione del nemico risolto non c'e'");
    ART_CHECK(ArtAtlasFindByImageId("mai-disegnato") == NULL,
              "un image-id senza originale deve tornare NULL, non un altro sprite");
    ART_CHECK(ArtAtlasFindByImageId("") == NULL, "image-id vuoto deve tornare NULL");
    ART_CHECK(ArtAtlasFindByImageId(NULL) == NULL, "image-id NULL deve tornare NULL");

    /* Degrado: manifest rotto, PNG assente, chiave con ".." (un id storpiato
       non deve poter comporre un percorso fuori da assets/art/). */
    ART_CHECK(ArtAtlasGet("items/rotto") == NULL, "un manifest rotto deve dare NULL");
    ART_CHECK(ArtAtlasGet("items/senza-png") == NULL, "un manifest senza PNG deve dare NULL");
    ART_CHECK(ArtAtlasGet("items/../../etc/passwd") == NULL, "una chiave con '..' deve essere rifiutata");
    ART_CHECK(ArtAtlasGet("items/mai-esistito") == NULL, "una chiave inesistente deve dare NULL");
    /* Il fallimento si RICORDA: un secondo tentativo non deve riaprire i file
       (voce negativa in cache). */
    int cachedAfterFailures = ArtAtlasCachedCount();
    ART_CHECK(ArtAtlasGet("items/rotto") == NULL, "un manifest rotto deve dare NULL anche al secondo giro");
    ART_CHECK(ArtAtlasCachedCount() == cachedAfterFailures,
              "un secondo tentativo su un asset rotto ha aggiunto una voce: il fallimento non e' ricordato");

    /* ArtAtlasShutdown svuota tutto, e dopo si puo' ricaricare da zero: e' il
       ciclo che i percorsi di uscita del gioco esercitano. */
    ArtAtlasShutdown();
    ART_CHECK(ArtAtlasCachedCount() == 0, "ArtAtlasShutdown non ha svuotato la cache");
    ART_CHECK(ArtAtlasGet("items/prova-oggetto") != NULL, "dopo lo shutdown non si ricarica piu' nulla");

    /* Cartella di test rimossa: si deve tornare al comportamento "nessun
       pacchetto artistico", cioe' NULL per tutto. */
    ArtAtlasSetTestDir(NULL);
    ART_CHECK(ArtAtlasCachedCount() == 0, "ArtAtlasSetTestDir deve svuotare la cache");

    ArtAtlasSetTestDir(dir);
    RemoveFile(itemsDir, "prova-oggetto.png");
    ArtAtlasShutdown();
    ART_CHECK(ArtAtlasGet("items/prova-oggetto") == NULL,
              "cancellato il PNG, lo sheet deve tornare assente (nessuna cache stantia dopo lo shutdown)");
    ArtAtlasSetTestDir(NULL);

    /* Pulizia della fixture: nessun file lasciato in /tmp. */
    RemoveFile(itemsDir, "prova-oggetto.json");
    RemoveFile(itemsDir, "rotto.json");
    RemoveFile(itemsDir, "rotto.png");
    RemoveFile(itemsDir, "senza-png.json");
    RemoveFile(enemiesDir, "prova-nemico.json");
    RemoveFile(enemiesDir, "prova-nemico.png");
    remove(itemsDir);
    remove(enemiesDir);
    return true;
}

/* --- Scenario 6: il pacchetto VERO, se c'e' ------------------------------ */
static bool ScenarioRealPackage(void)
{
    /* Questo scenario NON e' vincolante: assets/art/ puo' non esistere in un
       checkout parziale, e in quel caso il test deve passare (e' esattamente il
       caso di degrado che il resto del file verifica). Quando c'e', si controlla
       che il contratto reale sia leggibile da questo parser -- cioe' che il
       consegnato della sessione artistica e il motore non siano andati alla
       deriva. */
    ArtAtlasShutdown();
    if (!FileExists(ART_ATLAS_DIR "character/fonditrice.json")) return true;

    const ArtSheet *player = ArtAtlasGet("character/fonditrice");
    ART_CHECK(player != NULL, "il pacchetto artistico c'e' ma character/fonditrice non si carica");
    static const char *const REQUIRED_PLAYER_ANIMS[] = {
        "walk_down", "walk_up", "walk_left", "walk_right", "idle", "hit", "death", NULL
    };
    for (int i = 0; REQUIRED_PLAYER_ANIMS[i]; i++)
    {
        if (ArtSheetAnim(player, REQUIRED_PLAYER_ANIMS[i])) continue;
        fprintf(stderr, "GameArtAtlasTest: character/fonditrice non dichiara \"%s\"\n", REQUIRED_PLAYER_ANIMS[i]);
        return false;
    }

    const ArtSheet *font = ArtAtlasGet("ui/font-5px");
    ART_CHECK(font != NULL, "ui/font-5px non si carica");
    ART_CHECK(font->glyphH > 0 && font->glyphCount > 20, "ui/font-5px senza glifi");
    const ArtSheet *panel = ArtAtlasGet("ui/panel-9patch");
    ART_CHECK(panel != NULL && panel->sliceL > 0, "ui/panel-9patch senza slice");

    /* CAPIENZA DEL REGISTRO. Non e' un dettaglio: la scansione a priorita'
       (ArtAtlasFindByImageId) lascia una voce NEGATIVA per ogni categoria che
       non contiene l'id cercato, e quelle voci contano nel limite ART_SHEET_MAX
       come quelle vere. Si risolve quindi OGNI image-id del catalogo curato
       (image-map.txt, la lista vera che il gioco userà) e si verifica che alla
       fine il registro non sia pieno -- se lo fosse, gli ultimi contenuti della
       run ricadrebbero in silenzio sulle primitive, e nessun altro assert se ne
       accorgerebbe. Il conteggio non e' intuitivo -- risolvere i 44 id della
       mappa costa 71 voci, non 44 -- ed e' proprio per questo che va misurato
       da un test invece che stimato a mano. */
    char *mapText = FileExists("assets/curated-content/image-map.txt")
                    ? LoadFileText("assets/curated-content/image-map.txt") : NULL;
    if (mapText)
    {
        int resolved = 0;
        for (const char *line = mapText; line && *line; )
        {
            const char *eol = strchr(line, '\n');
            if (*line != '#' && *line != '\n' && *line != '\r')
            {
                const char *eq = strchr(line, '=');
                if (eq && (!eol || eq < eol))
                {
                    char id[64];
                    const char *v = eq + 1;
                    while (*v == ' ' || *v == '\t') v++;
                    int n = 0;
                    while (v[n] && v[n] != '\n' && v[n] != '\r' && v[n] != ' ' && n < (int)sizeof(id) - 1) n++;
                    memcpy(id, v, (size_t)n);
                    id[n] = '\0';
                    if (id[0] && ArtAtlasFindByImageId(id)) resolved++;
                }
            }
            line = eol ? eol + 1 : NULL;
        }
        UnloadFileText(mapText);
        /* Si chiede un MARGINE, non solo "non e' pieno": un registro saturo al
           95% passerebbe oggi e si romperebbe alla prossima voce aggiunta al
           catalogo, in silenzio (gli ultimi contenuti ricadrebbero sulle
           primitive e nessun altro assert se ne accorgerebbe). Il numero
           osservato al 2026-07-30 e' 71 voci su 160 per le 44 voci della mappa,
           quindi il tetto all'85% e' largo per il catalogo di oggi e scatta
           prima del guaio quando il catalogo raddoppia. */
        int used = ArtAtlasCachedCount();
        if (used > ART_SHEET_MAX*85/100)
        {
            fprintf(stderr, "GameArtAtlasTest: registro quasi saturo (%d/%d) dopo aver risolto %d "
                            "image-id del catalogo curato: alzare ART_SHEET_MAX in "
                            "src/assets/art_atlas.h\n", used, ART_SHEET_MAX, resolved);
            return false;
        }
        printf("  art: %d image-id del catalogo curato risolti, %d/%d voci di registro usate\n",
               resolved, used, ART_SHEET_MAX);
        /* Contro-prova che la scansione stia davvero risolvendo qualcosa: se
           l'assert sopra passasse perche' NESSUN id si risolve, non proverebbe
           nulla. Il catalogo curato ha 44 voci con uno sprite disegnato; si
           chiede solo che ne risolva la maggioranza, per non legare il test al
           conteggio esatto di una ricurazione futura. */
        ART_CHECK(resolved >= 20, "il catalogo curato risolve troppi pochi image-id: la priorita' non funziona");
    }

    /* Tileset: si verificano i ruoli che il renderer usa DAVVERO, uno per
       famiglia, compresa la variante di escalation. Un tileset che ne perdesse
       uno lascerebbe un buco a schermo senza che nulla lo segnali. */
    const ArtSheet *tiles = ArtAtlasGet("tiles/lunar-forge");
    if (tiles)
    {
        static const char *const REQUIRED_ROLES[] = {
            "floor", "floor_var1", "wall_n", "wall_e", "wall_s", "wall_w",
            "corner_nw", "corner_ne", "corner_se", "corner_sw", "l_block", "void",
            "door_n_aperta", "door_s_chiusa", "door_e_bloccata",
            "obst_pillar", "obst_corridor", "obst_arena", "obst_scatter",
            "floor_deg", "wall_deg", "void_deg", NULL
        };
        Rectangle src;
        for (int i = 0; REQUIRED_ROLES[i]; i++)
        {
            if (ArtSheetTileRect(tiles, REQUIRED_ROLES[i], &src)) continue;
            fprintf(stderr, "GameArtAtlasTest: tiles/lunar-forge non dichiara il ruolo \"%s\"\n", REQUIRED_ROLES[i]);
            return false;
        }
    }
    ArtAtlasShutdown();
    return true;
}

bool GameArtAtlasTest(Game *game)
{
    (void)game;
    if (!ScenarioParseSpriteSheet()) return false;
    if (!ScenarioParseExtensions()) return false;
    if (!ScenarioParseBroken()) return false;
    if (!ScenarioAnimator()) return false;

    char dir[512];
    if (!CreateTempArtDir(dir, (int)sizeof(dir)))
    {
        fprintf(stderr, "GameArtAtlasTest: impossibile creare la cartella temporanea della fixture\n");
        return false;
    }
    bool ok = ScenarioLoadAndPriority(dir);
    /* Si ripristina SEMPRE la cartella di produzione, anche sui rami di
       errore: uno scenario fallito non deve lasciare il resto della suite a
       leggere una fixture cancellata (stessa disciplina di
       CuratedCatalogSetTestDir in curated_content_tests.c). */
    ArtAtlasSetTestDir(NULL);
    remove(dir);
    if (!ok) return false;

    return ScenarioRealPackage();
}
