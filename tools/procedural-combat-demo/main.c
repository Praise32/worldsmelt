#include "raylib.h"
#include "raymath.h"

#include "demo_script_api.h"
#include "script/script_sandbox.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEMO_WIDTH 1280
#define DEMO_HEIGHT 720
#define DEMO_PIXEL_WIDTH 640
#define DEMO_PIXEL_HEIGHT 360
#define DEMO_CAPTURE_FPS 15
#define DEMO_CAPTURE_FRAMES 450
#define DEMO_FIXED_DT (1.0f/60.0f)
/* Nella cattura headless non c'e' un tasto N: il pattern nemico avanza da
 * solo, cosi' lo smoke test attraversa comunque piu' di uno script. */
#define DEMO_CAPTURE_ENEMY_CYCLE_SECONDS 5.0f

#define DEMO_MAX_PROJECTILES 640
#define DEMO_MAX_ARCS 96
#define DEMO_MAX_BEAMS 48
#define DEMO_MAX_CAPTURE_FIELDS 16
#define DEMO_MAX_PARTICLES 768
#define DEMO_TRAIL_POINTS 10

#define DEMO_PI 3.14159265358979323846f
#define DEMO_TAU (2.0f*DEMO_PI)

/* Pool nemico/arma: array fissi, niente allocazione dinamica. Un nome file
 * oltre DEMO_POOL_NAME_MAX-1 caratteri o una voce oltre DEMO_POOL_MAX_ENTRIES
 * vengono semplicemente ignorati dal polling. */
#define DEMO_POOL_MAX_ENTRIES 64
#define DEMO_POOL_NAME_MAX 64
#define DEMO_POOL_POLL_SECONDS 1.0f
#define DEMO_ENEMY_GENERATED_DIR  "generated/combat-lab/enemy"
#define DEMO_WEAPON_GENERATED_DIR "generated/combat-lab/weapon"

#define DEMO_PLAYER_MAX_HP 6.0f
#define DEMO_PLAYER_KO_SECONDS 1.5f
#define DEMO_ENEMY_MAX_HP 60.0f
#define DEMO_ENEMY_RESPAWN_SECONDS 1.2f
#define DEMO_ENEMY_CORPSE_FADE_SECONDS 0.6f
#define DEMO_BASE_WEAPON_COOLDOWN 0.22f
/* HUD: quanto resta visibile il messaggio provvisorio di G/H/B prima di
 * tornare al placeholder "GEN: --" che WP4 riempira' con lo stato vero. */
#define DEMO_INFO_MESSAGE_SECONDS 2.4f

static const Rectangle DEMO_ROOM = { 58.0f, 88.0f, 1164.0f, 566.0f };

/* Pannelli del confronto A/B (Tab): in split il mondo non copre piu' l'intero
 * frame ma viene disegnato due volte qui dentro. Sono costanti condivise fra
 * il disegno (DemoComposeFrame) e l'inversa della mira (DemoWindowToLogic):
 * se divergessero il puntatore smetterebbe di corrispondere all'arena. */
static const Rectangle DEMO_SPLIT_LEFT = { 14.0f, 46.0f, 616.0f, 600.0f };
static const Rectangle DEMO_SPLIT_RIGHT = { 650.0f, 46.0f, 616.0f, 600.0f };

typedef enum DemoRenderMode {
    DEMO_RENDER_PIXEL = 0,
    DEMO_RENDER_SMOOTH,
    DEMO_RENDER_HYBRID
} DemoRenderMode;

typedef struct DemoAssets {
    Texture2D spider;
    Texture2D spook;
    Texture2D gelatine;
    Texture2D stareyes;
    Texture2D player;
    Texture2D handgun;
    bool ready;
} DemoAssets;

typedef struct DemoScriptRuntime {
    ScriptSandbox *sandbox;
    DemoScriptApiState api;
    bool ready;
    bool playerOwned;
    char error[256];
    char fileName[64];
} DemoScriptRuntime;

/* Una voce del pool e' o uno script curato (tools/procedural-combat-demo/
 * scripts/curated/, caricato relativo alla cartella dell'eseguibile come gli
 * asset, perche' il Makefile lo copia li' accanto al binario) o uno script
 * generato (generated/combat-lab/{enemy,weapon}/, relativo alla CWD: e'
 * contenuto scritto a runtime dal generatore esterno e la demo si lancia
 * dalla radice del repo con `make run-combat-lab`, MAI copiato accanto al
 * binario). Lo slot 0 dell'arma non ha una voce qui: e' la pistola base
 * cablata in C, mai un file su disco (vedi DemoBaseWeaponActive). */
typedef enum DemoPoolSource {
    DEMO_POOL_SOURCE_CURATED = 0,
    DEMO_POOL_SOURCE_GENERATED
} DemoPoolSource;

typedef struct DemoPoolEntry {
    char fileName[DEMO_POOL_NAME_MAX];
    DemoPoolSource source;
} DemoPoolEntry;

typedef struct DemoPool {
    DemoPoolEntry entries[DEMO_POOL_MAX_ENTRIES];
    int count;
    int current;
    float pollTimer;
} DemoPool;

typedef struct DemoProjectile {
    bool active;
    bool hostile;
    bool orbiting;
    bool splitOnDeath;
    Vector2 position;
    Vector2 velocity;
    Vector2 orbitCenter;
    Vector2 trail[DEMO_TRAIL_POINTS];
    int trailCount;
    float trailTimer;
    float radius;
    float damage;
    float life;
    float totalLife;
    float rotation;
    float orbitRadius;
    float orbitAngle;
    float angularSpeed;
    int visualId;
} DemoProjectile;

typedef struct DemoArcEffect {
    bool active;
    bool hostile;
    bool telegraph;
    bool melee;
    /* Come nei beam: il settore resta sensibile per tutta la vita dell'arco,
     * ma il danno al player si applica al massimo UNA volta. Il flag va alzato
     * solo quando il danno e' entrato davvero (i-frame compresi), altrimenti
     * l'arco diventa un hitscan del primo tick e la schivata non si legge. */
    bool damageApplied;
    Vector2 position;
    float angle;
    float radius;
    float width;
    float sweep;
    float damage;
    float life;
    float totalLife;
    int visualId;
} DemoArcEffect;

typedef struct DemoBeamEffect {
    bool active;
    bool hostile;
    bool telegraph;
    bool damageApplied;
    Vector2 position;
    float angle;
    float length;
    float width;
    float damage;
    float life;
    float totalLife;
    int visualId;
} DemoBeamEffect;

typedef struct DemoCaptureField {
    bool active;
    /* true se il campo viene dalla sandbox arma (self coincide col player):
     * in quel caso segue il player tick per tick, come nella prova originale.
     * Un campo nemico invece resta fermo dove e' stato creato. */
    bool playerOwned;
    Vector2 position;
    float radius;
    float strength;
    int remaining;
    float life;
    float totalLife;
    int visualId;
} DemoCaptureField;

typedef struct DemoParticle {
    bool active;
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float life;
    float totalLife;
} DemoParticle;

typedef struct DemoEnemy {
    bool alive;
    Vector2 position;
    Vector2 velocity;
    float hp;
    float hitFlash;
    float corpseFade;    /* >0 mentre il cadavere sfuma dopo la morte */
    float respawnTimer;  /* >0 mentre si aspetta il respawn */
    int spriteKind;       /* 0..3: spider/spook/gelatine/stareyes, ciclato ad ogni respawn */
} DemoEnemy;

typedef struct DemoPlayer {
    Vector2 position;
    float hp;
    float invulnerability;
    float statusTime;
    float statusStrength;
    bool dead;
    float koTimer;
    float aimAngle;
    float weaponCooldown; /* solo la pistola base: le armi generate gestiscono il proprio ritmo in Lua */
} DemoPlayer;

/* Input del frame, campionato UNA sola volta per frame nel main loop e poi
 * consumato dai passi a dt fisso. Leggere raylib dentro il loop a passo fisso
 * sarebbe sbagliato: i fronti (IsKeyPressed/IsMouseButtonPressed) li ricalcola
 * PollInputEvents una volta per frame, quindi un frame con piu' passi
 * ripeterebbe la stessa pressione su ogni tick e un frame senza passi la
 * perderebbe. specialPressed resta pendente finche' un tick non lo consuma,
 * cosi' rispetta il contratto di special_pressed() ("vero solo nel tick della
 * pressione") senza perdere click. */
typedef struct DemoFrameInput {
    bool interactive;    /* false sotto --capture: autopilota, nessun mouse/tastiera */
    Vector2 aim;         /* mouse gia' riportato nello spazio logico 1280x720 */
    Vector2 move;        /* asse WASD/frecce, -1..1 per componente */
    bool fireHeld;
    bool specialPressed;
} DemoFrameInput;

typedef struct DemoWorld {
    DemoScriptRuntime enemyScript;
    DemoScriptRuntime weaponScript;

    DemoEnemy enemy;
    DemoPlayer player;

    DemoProjectile projectiles[DEMO_MAX_PROJECTILES];
    DemoArcEffect arcs[DEMO_MAX_ARCS];
    DemoBeamEffect beams[DEMO_MAX_BEAMS];
    DemoCaptureField captureFields[DEMO_MAX_CAPTURE_FIELDS];
    DemoParticle particles[DEMO_MAX_PARTICLES];

    float globalTime;
    int storedEchoes;
    int capturedTotal;
    int hitsDealt;
    int lastEnemyCommandCount;
    int lastWeaponCommandCount;
    float infoMessageTimer;
    uint32_t cosmeticRng;
} DemoWorld;

typedef struct DemoRenderer {
    RenderTexture2D pixelTarget;
    RenderTexture2D highTarget;
    RenderTexture2D finalTarget;
    Shader hybridShader;
    bool shaderReady;
    int timeLocation;
    int modeLocation;
    int pixelSizeLocation;
} DemoRenderer;

static float DemoClamp(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static float DemoEaseOutCubic(float value)
{
    float t = DemoClamp(value, 0.0f, 1.0f);
    float inv = 1.0f - t;
    return 1.0f - inv*inv*inv;
}

static float DemoAngleDifference(float a, float b)
{
    float result = fmodf(a - b + DEMO_PI, DEMO_TAU);
    if (result < 0.0f) result += DEMO_TAU;
    return result - DEMO_PI;
}

static Vector2 DemoDirection(float angle)
{
    return (Vector2){ cosf(angle), sinf(angle) };
}

static Vector2 DemoAddScaled(Vector2 origin, Vector2 direction, float amount)
{
    return (Vector2){ origin.x + direction.x*amount, origin.y + direction.y*amount };
}

static float DemoPointSegmentDistance(Vector2 point, Vector2 a, Vector2 b)
{
    Vector2 ab = Vector2Subtract(b, a);
    float lengthSquared = Vector2LengthSqr(ab);
    if (lengthSquared <= 0.0001f) return Vector2Distance(point, a);
    float t = Vector2DotProduct(Vector2Subtract(point, a), ab)/lengthSquared;
    t = DemoClamp(t, 0.0f, 1.0f);
    return Vector2Distance(point, Vector2Add(a, Vector2Scale(ab, t)));
}

static uint32_t DemoNextRandom(DemoWorld *world)
{
    uint32_t x = world->cosmeticRng;
    if (x == 0) x = 0xA341316Cu;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    world->cosmeticRng = x;
    return x;
}

static float DemoRandomUnit(DemoWorld *world)
{
    return (float)(DemoNextRandom(world) & 0x00ffffffu)/(float)0x01000000u;
}

static Color DemoVisualColor(int visualId)
{
    switch (visualId)
    {
        case DEMO_VIS_VIOLET_CUT: return (Color){ 198, 92, 255, 255 };
        case DEMO_VIS_CALLIGRAPHY_INK: return (Color){ 52, 218, 196, 255 };
        case DEMO_VIS_GLASS_PRISM: return (Color){ 126, 224, 255, 255 };
        case DEMO_VIS_GRAVITY: return (Color){ 255, 181, 71, 255 };
        case DEMO_VIS_VOID_ECHO: return (Color){ 145, 107, 255, 255 };
        case DEMO_VIS_RELOAD_ORBIT: return (Color){ 255, 102, 153, 255 };
        default: return WHITE;
    }
}

static void DemoBuildPath(char *destination, size_t size, const char *relative)
{
    const char *base = GetApplicationDirectory();
    size_t length = strlen(base);
    const char *separator = (length > 0 && (base[length - 1] == '/' || base[length - 1] == '\\')) ? "" : "/";
    snprintf(destination, size, "%s%s%s", base, separator, relative);
}

static bool DemoTextureValid(Texture2D texture)
{
    return IsTextureValid(texture) && texture.width > 0 && texture.height > 0;
}

static bool DemoLoadAssets(DemoAssets *assets)
{
    char path[1024];
    memset(assets, 0, sizeof *assets);

    DemoBuildPath(path, sizeof path, "assets/spider_lord.png");
    assets->spider = LoadTexture(path);
    DemoBuildPath(path, sizeof path, "assets/spook.png");
    assets->spook = LoadTexture(path);
    DemoBuildPath(path, sizeof path, "assets/gelatine.png");
    assets->gelatine = LoadTexture(path);
    DemoBuildPath(path, sizeof path, "assets/stareyes.png");
    assets->stareyes = LoadTexture(path);
    DemoBuildPath(path, sizeof path, "assets/horseman.png");
    assets->player = LoadTexture(path);
    DemoBuildPath(path, sizeof path, "assets/handgun.png");
    assets->handgun = LoadTexture(path);

    assets->ready = DemoTextureValid(assets->spider) && DemoTextureValid(assets->spook) &&
                    DemoTextureValid(assets->gelatine) && DemoTextureValid(assets->stareyes) &&
                    DemoTextureValid(assets->player) && DemoTextureValid(assets->handgun);
    return assets->ready;
}

static void DemoUnloadAssets(DemoAssets *assets)
{
    if (DemoTextureValid(assets->spider)) UnloadTexture(assets->spider);
    if (DemoTextureValid(assets->spook)) UnloadTexture(assets->spook);
    if (DemoTextureValid(assets->gelatine)) UnloadTexture(assets->gelatine);
    if (DemoTextureValid(assets->stareyes)) UnloadTexture(assets->stareyes);
    if (DemoTextureValid(assets->player)) UnloadTexture(assets->player);
    if (DemoTextureValid(assets->handgun)) UnloadTexture(assets->handgun);
    memset(assets, 0, sizeof *assets);
}

static Texture2D DemoEnemyTexture(const DemoAssets *assets, int kind)
{
    switch (kind & 3)
    {
        case 0: return assets->spider;
        case 1: return assets->spook;
        case 2: return assets->gelatine;
        default: return assets->stareyes;
    }
}

/* ------------------------------------------------------------------------
 * Pool nemico/arma: inizializzazione curata, scansione di generated/, e
 * risoluzione del percorso di caricamento di una voce.
 * ---------------------------------------------------------------------- */

static void DemoPoolInitCurated(DemoPool *pool, const char *const *names, int count)
{
    memset(pool, 0, sizeof *pool);
    for (int i = 0; i < count && pool->count < DEMO_POOL_MAX_ENTRIES; i++)
    {
        snprintf(pool->entries[pool->count].fileName, DEMO_POOL_NAME_MAX, "%s", names[i]);
        pool->entries[pool->count].source = DEMO_POOL_SOURCE_CURATED;
        pool->count++;
    }
}

static bool DemoPoolContains(const DemoPool *pool, const char *fileName)
{
    for (int i = 0; i < pool->count; i++)
        if (strcmp(pool->entries[i].fileName, fileName) == 0) return true;
    return false;
}

static int DemoPoolNameCompare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

/* Scansiona una cartella generated/combat-lab/{enemy,weapon} (CWD, non
 * application directory) e appende in coda, in ordine alfabetico di nome, i
 * soli file .lua mai visti prima: e' il polling ~1 s della spec (sezione 2,
 * protocollo file). Il pool e' un array fisso: oltre DEMO_POOL_MAX_ENTRIES gli
 * altri file restano ignorati fino al prossimo riavvio. */
static void DemoPoolScanGenerated(DemoPool *pool, const char *directory)
{
    if (pool->count >= DEMO_POOL_MAX_ENTRIES) return;
    if (!DirectoryExists(directory)) return;

    FilePathList list = LoadDirectoryFiles(directory);
    char candidates[DEMO_POOL_MAX_ENTRIES][DEMO_POOL_NAME_MAX];
    int candidateCount = 0;
    for (unsigned int i = 0; i < list.count && candidateCount < DEMO_POOL_MAX_ENTRIES; i++)
    {
        const char *name = GetFileName(list.paths[i]);
        if (!IsFileExtension(name, ".lua")) continue;
        if (strlen(name) >= DEMO_POOL_NAME_MAX) continue;
        if (DemoPoolContains(pool, name)) continue;
        snprintf(candidates[candidateCount], DEMO_POOL_NAME_MAX, "%s", name);
        candidateCount++;
    }
    UnloadDirectoryFiles(list);

    qsort(candidates, (size_t)candidateCount, DEMO_POOL_NAME_MAX, DemoPoolNameCompare);
    for (int i = 0; i < candidateCount && pool->count < DEMO_POOL_MAX_ENTRIES; i++)
    {
        /* memcpy, non snprintf("%s",...): dopo il qsort gcc perde la
         * dimensione statica della riga (passata come void* a qsort) e
         * -Wformat-truncation assume una sorgente illimitata. candidates[i]
         * e' gia' una riga da DEMO_POOL_NAME_MAX byte terminata a dovere
         * dallo snprintf qui sopra, quindi una copia a blocco fisso e'
         * sia corretta sia silenziosa per l'analisi statica. */
        memcpy(pool->entries[pool->count].fileName, candidates[i], DEMO_POOL_NAME_MAX);
        pool->entries[pool->count].source = DEMO_POOL_SOURCE_GENERATED;
        pool->count++;
    }
}

static void DemoPoolPollTick(DemoPool *pool, const char *directory, float dt)
{
    pool->pollTimer += dt;
    if (pool->pollTimer < DEMO_POOL_POLL_SECONDS) return;
    pool->pollTimer = 0.0f;
    DemoPoolScanGenerated(pool, directory);
}

/* generated/combat-lab/{enemy,weapon}: create all'avvio se mancano.
 * MakeDirectory di raylib crea l'intera catena di cartelle richieste, quindi
 * basta una chiamata a testa anche se "generated/combat-lab" non esiste
 * ancora. */
static void DemoEnsureGeneratedDirs(void)
{
    if (!DirectoryExists(DEMO_ENEMY_GENERATED_DIR)) MakeDirectory(DEMO_ENEMY_GENERATED_DIR);
    if (!DirectoryExists(DEMO_WEAPON_GENERATED_DIR)) MakeDirectory(DEMO_WEAPON_GENERATED_DIR);
}

static char *DemoPoolLoadSource(const DemoPoolEntry *entry, const char *generatedDirectory)
{
    char path[1024];
    if (entry->source == DEMO_POOL_SOURCE_CURATED)
        DemoBuildPath(path, sizeof path, TextFormat("scripts/curated/%s", entry->fileName));
    else
        snprintf(path, sizeof path, "%s/%s", generatedDirectory, entry->fileName);
    return LoadFileText(path);
}

static void DemoScriptUnload(DemoScriptRuntime *runtime)
{
    if (runtime->sandbox != NULL) ScriptSandboxDestroy(runtime->sandbox);
    memset(runtime, 0, sizeof *runtime);
}

static bool DemoScriptLoad(DemoScriptRuntime *runtime, const DemoPoolEntry *entry,
                           const char *generatedDirectory, bool playerOwned, unsigned int seed)
{
    char *source = NULL;
    uint64_t selfHandle = playerOwned ? 100u : 200u;
    uint64_t playerHandle = 1u;

    DemoScriptUnload(runtime);
    runtime->playerOwned = playerOwned;
    snprintf(runtime->fileName, sizeof runtime->fileName, "%s", entry->fileName);
    DemoScriptApiInit(&runtime->api, selfHandle, playerHandle,
                      DEMO_ROOM.x, DEMO_ROOM.y,
                      DEMO_ROOM.x + DEMO_ROOM.width,
                      DEMO_ROOM.y + DEMO_ROOM.height);

    runtime->sandbox = ScriptSandboxCreate(seed, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (runtime->sandbox == NULL)
    {
        snprintf(runtime->error, sizeof runtime->error, "creazione sandbox fallita");
        return false;
    }
    if (!DemoScriptApiRegister(runtime->sandbox, &runtime->api))
    {
        snprintf(runtime->error, sizeof runtime->error, "registrazione API fallita");
        return false;
    }

    source = DemoPoolLoadSource(entry, generatedDirectory);
    if (source == NULL)
    {
        snprintf(runtime->error, sizeof runtime->error, "script assente: %s", entry->fileName);
        return false;
    }
    runtime->ready = ScriptSandboxLoad(runtime->sandbox, entry->fileName, source,
                                       runtime->error, sizeof runtime->error);
    UnloadFileText(source);
    if (runtime->ready && !ScriptSandboxHasFunction(runtime->sandbox, "on_tick"))
    {
        snprintf(runtime->error, sizeof runtime->error, "on_tick assente");
        runtime->ready = false;
    }
    return runtime->ready;
}

/* Ricarica SOLO la sandbox nemico col pattern a enemyPool->current (tasto N e
 * avanzamento automatico dopo la morte): posizione/HP del nemico non
 * cambiano, cambia solo il "cervello". */
static void DemoEnemyBeginPattern(DemoWorld *world, const DemoPool *enemyPool)
{
    const DemoPoolEntry *entry = &enemyPool->entries[enemyPool->current];
    DemoScriptLoad(&world->enemyScript, entry, DEMO_ENEMY_GENERATED_DIR, false,
                   0x51A7u + (unsigned int)enemyPool->current*97u);
}

/* Fa nascere/rinascere il nemico con enemyPool->current SENZA avanzare
 * l'indice: usato sia dal reset iniziale/arena (che deve restare sullo
 * stesso pattern) sia da DemoEnemyRespawn (che avanza l'indice PRIMA di
 * chiamare questa funzione). */
static void DemoEnemySpawn(DemoWorld *world, const DemoPool *enemyPool)
{
    world->enemy.alive = true;
    world->enemy.hp = DEMO_ENEMY_MAX_HP;
    world->enemy.hitFlash = 0.0f;
    world->enemy.corpseFade = 0.0f;
    world->enemy.respawnTimer = 0.0f;
    world->enemy.velocity = (Vector2){ 0.0f, 0.0f };
    world->enemy.position = (Vector2){ DEMO_ROOM.x + DEMO_ROOM.width*0.5f, DEMO_ROOM.y + 46.0f };
    DemoEnemyBeginPattern(world, enemyPool);
}

static void DemoEnemyKill(DemoWorld *world)
{
    if (!world->enemy.alive) return;
    world->enemy.alive = false;
    world->enemy.corpseFade = DEMO_ENEMY_CORPSE_FADE_SECONDS;
    world->enemy.respawnTimer = DEMO_ENEMY_RESPAWN_SECONDS;
}

static DemoProjectile *DemoNewProjectile(DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
    {
        if (!world->projectiles[i].active)
        {
            memset(&world->projectiles[i], 0, sizeof world->projectiles[i]);
            world->projectiles[i].active = true;
            return &world->projectiles[i];
        }
    }
    return NULL;
}

static DemoArcEffect *DemoNewArc(DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_ARCS; i++)
    {
        if (!world->arcs[i].active)
        {
            memset(&world->arcs[i], 0, sizeof world->arcs[i]);
            world->arcs[i].active = true;
            return &world->arcs[i];
        }
    }
    return NULL;
}

static DemoBeamEffect *DemoNewBeam(DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
    {
        if (!world->beams[i].active)
        {
            memset(&world->beams[i], 0, sizeof world->beams[i]);
            world->beams[i].active = true;
            return &world->beams[i];
        }
    }
    return NULL;
}

static DemoCaptureField *DemoNewCaptureField(DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_CAPTURE_FIELDS; i++)
    {
        if (!world->captureFields[i].active)
        {
            memset(&world->captureFields[i], 0, sizeof world->captureFields[i]);
            world->captureFields[i].active = true;
            return &world->captureFields[i];
        }
    }
    return NULL;
}

static void DemoSpawnParticles(DemoWorld *world, Vector2 position, Color color, int count, float speed)
{
    for (int n = 0; n < count; n++)
    {
        DemoParticle *particle = NULL;
        for (int i = 0; i < DEMO_MAX_PARTICLES; i++)
        {
            if (!world->particles[i].active)
            {
                particle = &world->particles[i];
                break;
            }
        }
        if (particle == NULL) return;
        float angle = DemoRandomUnit(world)*DEMO_TAU;
        float magnitude = speed*(0.25f + 0.75f*DemoRandomUnit(world));
        memset(particle, 0, sizeof *particle);
        particle->active = true;
        particle->position = position;
        particle->velocity = Vector2Scale(DemoDirection(angle), magnitude);
        particle->color = color;
        particle->size = 2.0f + 4.0f*DemoRandomUnit(world);
        particle->life = particle->totalLife = 0.25f + 0.45f*DemoRandomUnit(world);
    }
}

static void DemoSpawnShot(DemoWorld *world, Vector2 position, float angle, float speed,
                          float radius, float damage, float life, int visualId, bool hostile)
{
    DemoProjectile *shot = DemoNewProjectile(world);
    if (shot == NULL) return;
    shot->position = position;
    shot->velocity = Vector2Scale(DemoDirection(angle), speed);
    shot->radius = radius;
    shot->damage = damage;
    shot->life = shot->totalLife = life;
    shot->rotation = angle;
    shot->visualId = visualId;
    shot->hostile = hostile;
    shot->trail[0] = position;
    shot->trailCount = 1;
}

static void DemoSpawnOrbit(DemoWorld *world, const DemoScriptCommand *command, bool hostile)
{
    int count = command->count;
    for (int i = 0; i < count; i++)
    {
        DemoProjectile *shot = DemoNewProjectile(world);
        if (shot == NULL) return;
        float phase = DEMO_TAU*(float)i/(float)count;
        shot->orbiting = true;
        shot->hostile = hostile;
        shot->orbitCenter = (Vector2){ command->x, command->y };
        shot->orbitRadius = command->radius;
        shot->orbitAngle = phase;
        shot->angularSpeed = command->angularSpeed;
        shot->position = DemoAddScaled(shot->orbitCenter, DemoDirection(phase), command->radius);
        shot->radius = command->projectileRadius;
        shot->damage = command->damage;
        shot->life = shot->totalLife = command->life;
        shot->visualId = command->visualId;
        shot->splitOnDeath = hostile && command->visualId == DEMO_VIS_CALLIGRAPHY_INK;
        shot->trail[0] = shot->position;
        shot->trailCount = 1;
    }
}

static void DemoSpawnRing(DemoWorld *world, const DemoScriptCommand *command, bool hostile)
{
    float phase = 0.23f*world->globalTime;
    for (int i = 0; i < command->count; i++)
    {
        float angle = phase + DEMO_TAU*(float)i/(float)command->count;
        DemoSpawnShot(world, (Vector2){ command->x, command->y }, angle,
                      command->speed, command->projectileRadius, command->damage,
                      command->life, command->visualId, hostile);
    }
}

static void DemoSpawnCrescentWall(DemoWorld *world, const DemoScriptCommand *command)
{
    const int count = 13;
    for (int i = 0; i < count; i++)
    {
        float sideT;
        float offset;
        if (i < 6)
        {
            sideT = (float)i/5.0f;
            offset = -command->sweep*0.5f + sideT*(command->sweep*0.36f);
        }
        else
        {
            sideT = (float)(i - 6)/6.0f;
            offset = command->sweep*0.14f + sideT*(command->sweep*0.36f);
        }
        float angle = command->angle + offset;
        Vector2 start = DemoAddScaled((Vector2){ command->x, command->y }, DemoDirection(angle), command->radius*0.36f);
        DemoSpawnShot(world, start, angle, 156.0f, 7.0f, command->damage*0.38f,
                      2.8f, command->visualId, true);
    }
}

/* Ritorna true solo se il danno e' entrato davvero: archi e raggi ci
 * appoggiano il proprio "colpito una volta sola", cosi' un contatto assorbito
 * dagli i-frame non consuma il loro unico colpo. */
static bool DemoDamagePlayer(DemoWorld *world, float amount, Vector2 hitPosition)
{
    if (world->player.invulnerability > 0.0f || world->player.dead) return false;
    world->player.hp = fmaxf(0.0f, world->player.hp - amount);
    world->player.invulnerability = 0.38f;
    DemoSpawnParticles(world, hitPosition, (Color){ 255, 93, 110, 255 }, 14, 90.0f);
    if (world->player.hp <= 0.0f)
    {
        world->player.dead = true;
        world->player.koTimer = DEMO_PLAYER_KO_SECONDS;
        DemoSpawnParticles(world, world->player.position, (Color){ 255, 210, 90, 255 }, 30, 160.0f);
    }
    return true;
}

/* Danno generico al nemico. grantsEcho distingue le due sole fonti di
 * storedEchoes previste dalla spec (melee del player) da tutte le altre
 * (proiettili): capture_radius alimenta storedEchoes per conto suo in
 * DemoUpdateCaptureFields. */
static void DemoEnemyApplyDamage(DemoWorld *world, float amount, Vector2 hitPosition,
                                 Color particleColor, bool grantsEcho)
{
    DemoEnemy *enemy = &world->enemy;
    if (!enemy->alive) return;
    enemy->hp = fmaxf(0.0f, enemy->hp - amount);
    enemy->hitFlash = 0.2f;
    world->hitsDealt++;
    if (grantsEcho && world->storedEchoes < 8) world->storedEchoes++;
    DemoSpawnParticles(world, hitPosition, particleColor, grantsEcho ? 18 : 14, grantsEcho ? 120.0f : 100.0f);
    if (enemy->hp <= 0.0f) DemoEnemyKill(world);
}

/* Melee del player sul nemico vero (spec sezione 3: "un attore alla volta"):
 * prima colpiva un array di manichini, ora c'e' un solo bersaglio reale. */
static void DemoApplyMelee(DemoWorld *world, const DemoScriptCommand *command)
{
    DemoEnemy *enemy = &world->enemy;
    if (!enemy->alive) return;
    Vector2 delta = Vector2Subtract(enemy->position, (Vector2){ command->x, command->y });
    float distance = Vector2Length(delta);
    float angle = atan2f(delta.y, delta.x);
    if (distance <= command->radius + command->width*0.5f &&
        fabsf(DemoAngleDifference(angle, command->angle)) <= command->sweep*0.5f)
    {
        DemoEnemyApplyDamage(world, command->damage, enemy->position, DemoVisualColor(command->visualId), true);
    }
}

/* Consuma il command buffer di UNA sandbox (nemico o arma). hostile
 * distingue le due, per chi guarda solo il buffer risultante: e' cio' che
 * decide se un arco/proiettile puo' ferire il player invece del nemico. */
static void DemoConsumeCommands(DemoWorld *world, DemoScriptRuntime *runtime, bool hostile,
                                int *lastCommandCount)
{
    const DemoScriptCommand *commands = DemoScriptApiCommands(&runtime->api);
    size_t commandCount = DemoScriptApiCommandCount(&runtime->api);
    *lastCommandCount = (int)commandCount;

    for (size_t i = 0; i < commandCount; i++)
    {
        const DemoScriptCommand *command = &commands[i];
        switch (command->type)
        {
            case DEMO_CMD_TELEGRAPH_ARC:
            case DEMO_CMD_EMIT_ARC:
            case DEMO_CMD_MELEE_SWEEP:
            {
                DemoArcEffect *arc = DemoNewArc(world);
                if (arc != NULL)
                {
                    arc->hostile = hostile;
                    arc->telegraph = command->type == DEMO_CMD_TELEGRAPH_ARC;
                    arc->melee = command->type == DEMO_CMD_MELEE_SWEEP;
                    arc->position = (Vector2){ command->x, command->y };
                    arc->angle = command->angle;
                    arc->radius = command->radius;
                    arc->width = command->width;
                    arc->sweep = command->sweep;
                    arc->damage = command->damage;
                    arc->life = arc->totalLife = command->duration;
                    arc->visualId = command->visualId;
                }
                if (command->type == DEMO_CMD_EMIT_ARC && hostile) DemoSpawnCrescentWall(world, command);
                if (command->type == DEMO_CMD_MELEE_SWEEP && !hostile) DemoApplyMelee(world, command);
            } break;

            case DEMO_CMD_EMIT_RING:
                DemoSpawnRing(world, command, hostile);
                break;

            case DEMO_CMD_EMIT_ORBIT:
                DemoSpawnOrbit(world, command, hostile);
                break;

            case DEMO_CMD_TELEGRAPH_BEAM:
            case DEMO_CMD_EMIT_BEAM:
            {
                DemoBeamEffect *beam = DemoNewBeam(world);
                if (beam != NULL)
                {
                    beam->hostile = hostile;
                    beam->telegraph = command->type == DEMO_CMD_TELEGRAPH_BEAM;
                    beam->position = (Vector2){ command->x, command->y };
                    beam->angle = command->angle;
                    beam->length = command->length;
                    beam->width = command->width;
                    beam->damage = command->damage;
                    beam->life = beam->totalLife = command->duration;
                    beam->visualId = command->visualId;
                }
            } break;

            case DEMO_CMD_SET_VELOCITY:
                /* Solo il nemico si muove per script: il "self" dell'arma
                 * coincide sempre col player, che si sposta solo con
                 * WASD/frecce (vedi DemoUpdatePlayerMovement). Un'arma
                 * generata che chiama set_velocity viene quindi ignorata,
                 * mai lascia che uno script sposti il giocatore. */
                if (hostile) world->enemy.velocity = (Vector2){ command->vx, command->vy };
                break;

            case DEMO_CMD_ADD_STATUS:
                if (command->targetHandle == runtime->api.playerHandle)
                {
                    world->player.statusTime = fmaxf(world->player.statusTime, command->duration);
                    world->player.statusStrength = command->strength;
                }
                break;

            case DEMO_CMD_CAPTURE_RADIUS:
            {
                DemoCaptureField *field = DemoNewCaptureField(world);
                if (field != NULL)
                {
                    field->position = (Vector2){ command->x, command->y };
                    field->radius = command->radius;
                    field->strength = command->strength;
                    field->remaining = command->count;
                    field->life = field->totalLife = command->duration;
                    field->visualId = command->visualId;
                    field->playerOwned = !hostile;
                }
            } break;

            case DEMO_CMD_RELEASE_ECHOES:
            {
                int available = world->storedEchoes;
                int releaseCount = available > 0 ? available : 1;
                if (releaseCount > command->count) releaseCount = command->count;
                for (int shot = 0; shot < releaseCount; shot++)
                {
                    float t = releaseCount > 1 ? (float)shot/(float)(releaseCount - 1) : 0.5f;
                    float angle = command->angle + (t - 0.5f)*command->spread;
                    DemoSpawnShot(world, (Vector2){ command->x, command->y }, angle,
                                  command->speed, 7.0f + 0.7f*(float)available,
                                  command->damage + 0.5f*(float)available,
                                  command->life, command->visualId, false);
                }
                world->storedEchoes = 0;
            } break;
        }
    }
}

static void DemoRunEnemyScript(DemoWorld *world, float dt)
{
    DemoScriptRuntime *runtime = &world->enemyScript;
    /* Niente IA mentre il cadavere sfuma o si aspetta il respawn: lo script
     * riprende da capo (DemoEnemyBeginPattern) solo alla rinascita. */
    if (!world->enemy.alive) return;
    if (!runtime->ready || runtime->sandbox == NULL) return;

    float aimToPlayer = atan2f(world->player.position.y - world->enemy.position.y,
                               world->player.position.x - world->enemy.position.x);
    DemoScriptApiBeginFrame(&runtime->api,
                            world->player.position.x, world->player.position.y,
                            world->enemy.position.x, world->enemy.position.y,
                            aimToPlayer);
    if (!ScriptSandboxCallVoid(runtime->sandbox, "on_tick", 2,
                               (double)dt, (double)runtime->api.selfHandle))
    {
        if (ScriptSandboxIsDisabled(runtime->sandbox))
        {
            snprintf(runtime->error, sizeof runtime->error, "%s",
                     ScriptSandboxDisabledReason(runtime->sandbox));
            runtime->ready = false;
            world->lastEnemyCommandCount = 0;
        }
        return;
    }
    DemoConsumeCommands(world, runtime, true, &world->lastEnemyCommandCount);
}

static void DemoRunWeaponScript(DemoWorld *world, float dt, bool fireHeld, bool specialPressed)
{
    DemoScriptRuntime *runtime = &world->weaponScript;
    if (!runtime->ready || runtime->sandbox == NULL) return;

    /* L'arma non ha una posizione propria: "self" e' sempre il player. */
    DemoScriptApiBeginFrame(&runtime->api,
                            world->player.position.x, world->player.position.y,
                            world->player.position.x, world->player.position.y,
                            world->player.aimAngle);
    DemoScriptApiSetInput(&runtime->api, fireHeld, specialPressed);
    if (!ScriptSandboxCallVoid(runtime->sandbox, "on_tick", 2,
                               (double)dt, (double)runtime->api.selfHandle))
    {
        if (ScriptSandboxIsDisabled(runtime->sandbox))
        {
            snprintf(runtime->error, sizeof runtime->error, "%s",
                     ScriptSandboxDisabledReason(runtime->sandbox));
            runtime->ready = false;
            /* Il conteggio comandi di una sandbox morta non deve restare
             * congelato sull'ultimo valore: l'HUD mostrerebbe attivita' dove
             * non ce n'e' piu'. */
            world->lastWeaponCommandCount = 0;
        }
        return;
    }
    DemoConsumeCommands(world, runtime, false, &world->lastWeaponCommandCount);
}

/* Avanza il pattern nemico E fa ripartire il nemico da capo (spec sezione 3:
 * "alla morte respawn con il pattern successivo del pool"): usata alla morte
 * e dalla cattura headless (avanzamento a tempo, niente tasto N). */
static void DemoEnemyRespawn(DemoWorld *world, DemoPool *enemyPool)
{
    if (enemyPool->count > 0) enemyPool->current = (enemyPool->current + 1)%enemyPool->count;
    world->enemy.spriteKind = (world->enemy.spriteKind + 1) & 3;
    DemoEnemySpawn(world, enemyPool);
}

static void DemoUpdateEnemy(DemoWorld *world, DemoPool *enemyPool, float dt)
{
    DemoEnemy *enemy = &world->enemy;
    enemy->hitFlash = fmaxf(0.0f, enemy->hitFlash - dt);

    if (!enemy->alive)
    {
        enemy->corpseFade = fmaxf(0.0f, enemy->corpseFade - dt);
        enemy->respawnTimer -= dt;
        if (enemy->respawnTimer <= 0.0f) DemoEnemyRespawn(world, enemyPool);
        return;
    }

    enemy->position = Vector2Add(enemy->position, Vector2Scale(enemy->velocity, dt));
    enemy->position.x = DemoClamp(enemy->position.x, DEMO_ROOM.x + 70.0f, DEMO_ROOM.x + DEMO_ROOM.width - 70.0f);
    enemy->position.y = DemoClamp(enemy->position.y, DEMO_ROOM.y + 55.0f, DEMO_ROOM.y + DEMO_ROOM.height*0.60f);
}

static void DemoUpdateCaptureFields(DemoWorld *world, float dt)
{
    for (int i = 0; i < DEMO_MAX_CAPTURE_FIELDS; i++)
    {
        DemoCaptureField *field = &world->captureFields[i];
        if (!field->active) continue;
        field->life -= dt;
        if (field->playerOwned) field->position = world->player.position;

        for (int shotIndex = 0; shotIndex < DEMO_MAX_PROJECTILES && field->remaining > 0; shotIndex++)
        {
            DemoProjectile *shot = &world->projectiles[shotIndex];
            if (!shot->active || !shot->hostile || shot->orbiting) continue;
            Vector2 delta = Vector2Subtract(field->position, shot->position);
            float distance = Vector2Length(delta);
            if (distance > field->radius) continue;
            if (distance > 0.001f)
                shot->velocity = Vector2Add(shot->velocity,
                    Vector2Scale(delta, field->strength*520.0f*dt/distance));
            if (distance < fmaxf(24.0f, field->radius*0.70f))
            {
                shot->active = false;
                field->remaining--;
                world->storedEchoes++;
                world->capturedTotal++;
                DemoSpawnParticles(world, shot->position, DemoVisualColor(field->visualId), 9, 75.0f);
            }
        }
        if (field->life <= 0.0f) field->active = false;
    }
}

static void DemoUpdateProjectileTrail(DemoProjectile *shot, float dt)
{
    shot->trailTimer -= dt;
    if (shot->trailTimer > 0.0f) return;
    if (shot->trailCount < DEMO_TRAIL_POINTS) shot->trailCount++;
    for (int i = shot->trailCount - 1; i > 0; i--) shot->trail[i] = shot->trail[i - 1];
    shot->trail[0] = shot->position;
    shot->trailTimer = 0.035f;
}

static void DemoUpdateProjectiles(DemoWorld *world, float dt)
{
    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
    {
        DemoProjectile *shot = &world->projectiles[i];
        if (!shot->active) continue;
        shot->life -= dt;
        if (shot->orbiting)
        {
            shot->orbitAngle += shot->angularSpeed*dt;
            shot->position = DemoAddScaled(shot->orbitCenter, DemoDirection(shot->orbitAngle), shot->orbitRadius);
            shot->rotation = shot->orbitAngle + DEMO_PI*0.5f;
        }
        else
        {
            shot->position = Vector2Add(shot->position, Vector2Scale(shot->velocity, dt));
            shot->rotation = atan2f(shot->velocity.y, shot->velocity.x);
        }
        DemoUpdateProjectileTrail(shot, dt);

        if (shot->hostile && CheckCollisionCircles(shot->position, shot->radius, world->player.position, 12.0f))
        {
            DemoDamagePlayer(world, fmaxf(0.2f, shot->damage*0.08f), shot->position);
            shot->active = false;
            continue;
        }
        if (!shot->hostile && world->enemy.alive &&
            CheckCollisionCircles(shot->position, shot->radius, world->enemy.position, 18.0f))
        {
            DemoEnemyApplyDamage(world, shot->damage, shot->position, DemoVisualColor(shot->visualId), false);
            shot->active = false;
        }

        if (shot->life <= 0.0f)
        {
            bool split = shot->splitOnDeath;
            Vector2 position = shot->position;
            int visualId = shot->visualId;
            float baseAngle = shot->rotation;
            shot->active = false;
            if (split)
            {
                DemoSpawnShot(world, position, baseAngle - 0.45f, 145.0f, 3.5f, 2.0f, 1.2f, visualId, true);
                DemoSpawnShot(world, position, baseAngle + 0.45f, 145.0f, 3.5f, 2.0f, 1.2f, visualId, true);
            }
            continue;
        }

        float margin = 130.0f;
        if (!shot->orbiting &&
            (shot->position.x < DEMO_ROOM.x - margin ||
             shot->position.x > DEMO_ROOM.x + DEMO_ROOM.width + margin ||
             shot->position.y < DEMO_ROOM.y - margin ||
             shot->position.y > DEMO_ROOM.y + DEMO_ROOM.height + margin))
            shot->active = false;
    }
}

static void DemoUpdateArcsAndBeams(DemoWorld *world, float dt)
{
    for (int i = 0; i < DEMO_MAX_ARCS; i++)
    {
        DemoArcEffect *arc = &world->arcs[i];
        if (!arc->active) continue;
        arc->life -= dt;
        /* Archi ostili non-telegraph (emit_arc/melee_sweep del nemico)
         * danneggiano il player al contatto col settore, in aggiunta a
         * qualunque effetto fisico gia' generato (es. la mezzaluna di
         * proiettili di emit_arc). Il test va rifatto ad ogni tick finche'
         * l'arco vive: entrare nel settore mentre la spazzata e' a meta' deve
         * costare, uscirne subito dopo la nascita deve salvare. */
        if (arc->hostile && !arc->telegraph && !arc->damageApplied)
        {
            Vector2 delta = Vector2Subtract(world->player.position, arc->position);
            float distance = Vector2Length(delta);
            float angleToPlayer = atan2f(delta.y, delta.x);
            float innerBound = arc->radius - arc->width*0.5f - 11.0f;
            float outerBound = arc->radius + arc->width*0.5f + 11.0f;
            if (distance >= innerBound && distance <= outerBound &&
                fabsf(DemoAngleDifference(angleToPlayer, arc->angle)) <= arc->sweep*0.5f)
                arc->damageApplied = DemoDamagePlayer(world, fmaxf(0.25f, arc->damage*0.08f),
                                                      world->player.position);
        }
        if (arc->life <= 0.0f) arc->active = false;
    }

    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
    {
        DemoBeamEffect *beam = &world->beams[i];
        if (!beam->active) continue;
        beam->life -= dt;
        /* Stessa forma degli archi: il raggio resta pericoloso per tutta la
         * sua durata, ma toglie vita una volta sola. */
        if (beam->hostile && !beam->telegraph && !beam->damageApplied)
        {
            Vector2 end = DemoAddScaled(beam->position, DemoDirection(beam->angle), beam->length);
            if (DemoPointSegmentDistance(world->player.position, beam->position, end) <= beam->width*0.5f + 11.0f)
                beam->damageApplied = DemoDamagePlayer(world, fmaxf(0.25f, beam->damage*0.08f),
                                                       world->player.position);
        }
        if (beam->life <= 0.0f) beam->active = false;
    }
}

static void DemoUpdateParticles(DemoWorld *world, float dt)
{
    for (int i = 0; i < DEMO_MAX_PARTICLES; i++)
    {
        DemoParticle *particle = &world->particles[i];
        if (!particle->active) continue;
        particle->life -= dt;
        particle->position = Vector2Add(particle->position, Vector2Scale(particle->velocity, dt));
        particle->velocity = Vector2Scale(particle->velocity, 1.0f - DemoClamp(dt*3.6f, 0.0f, 0.9f));
        if (particle->life <= 0.0f) particle->active = false;
    }
}

static Vector2 DemoAutoPlayerPosition(const DemoWorld *world)
{
    float t = world->globalTime;
    return (Vector2){ 640.0f + sinf(t*1.2f)*260.0f, 500.0f + cosf(t*0.95f)*96.0f };
}

/* input->interactive distingue il gioco vero dalla cattura headless
 * (--capture, niente finestra/tastiera reale sotto xvfb): li' il player si
 * muove da solo perche' lo smoke test deve attraversare l'arena senza mani.
 * A differenza della vecchia demo a scene fisse, nel gioco il player fermo
 * NON va alla deriva. */
static void DemoUpdatePlayerMovement(DemoWorld *world, float dt, const DemoFrameInput *frameInput)
{
    Vector2 input = frameInput->interactive ? frameInput->move : (Vector2){ 0.0f, 0.0f };

    if (Vector2LengthSqr(input) > 0.0f)
    {
        float speed = 235.0f;
        if (world->player.statusTime > 0.0f) speed *= DemoClamp(1.0f - world->player.statusStrength*0.45f, 0.35f, 1.0f);
        input = Vector2Scale(Vector2Normalize(input), speed*dt);
        world->player.position = Vector2Add(world->player.position, input);
    }
    else if (!frameInput->interactive)
    {
        Vector2 target = DemoAutoPlayerPosition(world);
        float follow = 1.0f - powf(0.0008f, dt);
        world->player.position = Vector2Lerp(world->player.position, target, follow);
    }

    world->player.position.x = DemoClamp(world->player.position.x, DEMO_ROOM.x + 24.0f,
                                         DEMO_ROOM.x + DEMO_ROOM.width - 24.0f);
    world->player.position.y = DemoClamp(world->player.position.y, DEMO_ROOM.y + 24.0f,
                                         DEMO_ROOM.y + DEMO_ROOM.height - 24.0f);
}

/* Inversa esatta di DemoDrawFinalToWindow: finestra -> rettangolo letterbox
 * -> spazio logico 1280x720. Serve a tradurre il mouse reale in coordinate
 * di gioco indipendentemente da resize/fullscreen della finestra. Con lo
 * split A/B attivo il mondo non riempie piu' il frame: dopo il letterbox
 * serve un secondo passaggio dal pannello (deformato, la resa e' stirata) allo
 * spazio dell'arena, altrimenti il puntatore punta a tutt'altro. */
static Vector2 DemoWindowToLogic(Vector2 windowPosition, bool split)
{
    float scaleX = (float)GetScreenWidth()/(float)DEMO_WIDTH;
    float scaleY = (float)GetScreenHeight()/(float)DEMO_HEIGHT;
    float scale = fminf(scaleX, scaleY);
    if (scale <= 0.0001f) return (Vector2){ DEMO_WIDTH*0.5f, DEMO_HEIGHT*0.5f };
    float destinationX = ((float)GetScreenWidth() - (float)DEMO_WIDTH*scale)*0.5f;
    float destinationY = ((float)GetScreenHeight() - (float)DEMO_HEIGHT*scale)*0.5f;
    Vector2 logic = { (windowPosition.x - destinationX)/scale, (windowPosition.y - destinationY)/scale };
    if (!split) return logic;

    /* Meta' schermo per pannello, anche fuori dai bordi del riquadro: il
     * cursore resta agganciato all'arena piu' vicina invece di saltare. */
    Rectangle panel = logic.x < (float)DEMO_WIDTH*0.5f ? DEMO_SPLIT_LEFT : DEMO_SPLIT_RIGHT;
    return (Vector2){ (logic.x - panel.x)*(float)DEMO_WIDTH/panel.width,
                      (logic.y - panel.y)*(float)DEMO_HEIGHT/panel.height };
}

/* La pistola base e' "sempre disponibile" (spec sezione 3): l'arma generata la
 * sostituisce solo finche' la sua sandbox e' viva. Se lo script non e' mai
 * partito o e' stato ucciso a meta' partita (budget di istruzioni, errore Lua)
 * il click torna alla pistola, altrimenti un'arma generata male lascerebbe il
 * player disarmato fino al prossimo M o R. */
static bool DemoBaseWeaponActive(const DemoWorld *world, const DemoPool *weaponPool)
{
    return weaponPool->current == 0 || !world->weaponScript.ready;
}

/* Mira e fuoco del player. Sotto --capture non esiste un mouse reale
 * (xvfb): si mira automaticamente il nemico e non si spara mai, cosi' il
 * fuoco resta deterministico e non dipende da cio' che l'X server finto
 * riporta come posizione del cursore. */
static void DemoUpdatePlayerAimAndWeapon(DemoWorld *world, const DemoPool *weaponPool, float dt,
                                         const DemoFrameInput *frameInput, bool specialPressed)
{
    Vector2 aimTarget = frameInput->interactive ? frameInput->aim : world->enemy.position;
    world->player.aimAngle = atan2f(aimTarget.y - world->player.position.y,
                                    aimTarget.x - world->player.position.x);

    bool fireHeld = frameInput->interactive && frameInput->fireHeld;

    if (DemoBaseWeaponActive(world, weaponPool))
    {
        world->player.weaponCooldown = fmaxf(0.0f, world->player.weaponCooldown - dt);
        if (fireHeld && world->player.weaponCooldown <= 0.0f)
        {
            DemoSpawnShot(world, world->player.position, world->player.aimAngle,
                         340.0f, 6.0f, 6.0f, 2.5f, DEMO_VIS_GLASS_PRISM, false);
            world->player.weaponCooldown = DEMO_BASE_WEAPON_COOLDOWN;
        }
    }
    else
    {
        DemoRunWeaponScript(world, dt, fireHeld, specialPressed);
    }
}

/* Reset arena "leggero": ricrea player/nemico/proiettili/effetti usando gli
 * indici pool CORRENTI, senza avanzarli (tasto R e scadenza del KO). */
static void DemoResetArena(DemoWorld *world, const DemoPool *enemyPool, const DemoPool *weaponPool)
{
    DemoScriptUnload(&world->enemyScript);
    DemoScriptUnload(&world->weaponScript);
    uint32_t rng = world->cosmeticRng ^ 0x9E3779B9u;
    if (rng == 0) rng = 0xC0FFEE11u;

    memset(world, 0, sizeof *world);
    world->cosmeticRng = rng;
    world->player.hp = DEMO_PLAYER_MAX_HP;
    world->player.position = (Vector2){ 640.0f, 532.0f };

    DemoEnemySpawn(world, enemyPool);
    if (weaponPool->current != 0)
        DemoScriptLoad(&world->weaponScript, &weaponPool->entries[weaponPool->current - 1],
                       DEMO_WEAPON_GENERATED_DIR, true,
                       0x77A1u + (unsigned int)weaponPool->current*53u);
}

static void DemoUpdateWorld(DemoWorld *world, DemoPool *enemyPool, DemoPool *weaponPool,
                            float dt, DemoFrameInput *frameInput)
{
    /* Il fronte del click destro vale per UN solo tick: il primo passo fisso
     * del frame se lo prende e lo azzera, gli eventuali passi successivi dello
     * stesso frame vedono gia' false. */
    bool specialPressed = frameInput->specialPressed;
    frameInput->specialPressed = false;

    world->globalTime += dt;
    world->infoMessageTimer = fmaxf(0.0f, world->infoMessageTimer - dt);

    if (world->player.dead)
    {
        world->player.koTimer -= dt;
        if (world->player.koTimer <= 0.0f) DemoResetArena(world, enemyPool, weaponPool);
        return;
    }

    world->player.invulnerability = fmaxf(0.0f, world->player.invulnerability - dt);
    world->player.statusTime = fmaxf(0.0f, world->player.statusTime - dt);

    DemoUpdatePlayerMovement(world, dt, frameInput);
    DemoUpdatePlayerAimAndWeapon(world, weaponPool, dt, frameInput, specialPressed);
    DemoRunEnemyScript(world, dt);
    DemoUpdateEnemy(world, enemyPool, dt);
    DemoUpdateCaptureFields(world, dt);
    DemoUpdateProjectiles(world, dt);
    DemoUpdateArcsAndBeams(world, dt);
    DemoUpdateParticles(world, dt);
}

static int DemoActiveProjectileCount(const DemoWorld *world)
{
    int count = 0;
    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++) if (world->projectiles[i].active) count++;
    return count;
}

static void DemoSetTextureFiltering(DemoAssets *assets, DemoRenderMode mode)
{
    int filter = (mode == DEMO_RENDER_SMOOTH) ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT;
    SetTextureFilter(assets->spider, filter);
    SetTextureFilter(assets->spook, filter);
    SetTextureFilter(assets->gelatine, filter);
    SetTextureFilter(assets->stareyes, filter);
    SetTextureFilter(assets->player, filter);
    SetTextureFilter(assets->handgun, filter);
}

static void DemoDrawTextureCentered(Texture2D texture, Vector2 position, float width, float height,
                                    float rotation, Color tint)
{
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle destination = { position.x, position.y, width, height };
    Vector2 origin = { width*0.5f, height*0.5f };
    DrawTexturePro(texture, source, destination, origin, rotation, tint);
}

static void DemoDrawArenaBackdrop(const DemoWorld *world, DemoRenderMode mode)
{
    Color outside = mode == DEMO_RENDER_PIXEL ? (Color){ 7, 10, 18, 255 } : (Color){ 5, 8, 16, 255 };
    Color floor = mode == DEMO_RENDER_PIXEL ? (Color){ 22, 28, 42, 255 } : (Color){ 18, 25, 39, 255 };
    Color tile = mode == DEMO_RENDER_PIXEL ? (Color){ 29, 37, 53, 255 } : (Color){ 27, 36, 54, 255 };
    ClearBackground(outside);

    DrawRectangleRec(DEMO_ROOM, floor);
    for (int y = (int)DEMO_ROOM.y; y < (int)(DEMO_ROOM.y + DEMO_ROOM.height); y += 48)
    {
        for (int x = (int)DEMO_ROOM.x; x < (int)(DEMO_ROOM.x + DEMO_ROOM.width); x += 48)
        {
            if ((((x - (int)DEMO_ROOM.x)/48) + ((y - (int)DEMO_ROOM.y)/48)) & 1)
                DrawRectangle(x, y, 48, 48, Fade(tile, 0.34f));
        }
    }
    DrawRectangleLinesEx(DEMO_ROOM, 4.0f, (Color){ 75, 91, 121, 255 });
    DrawRectangleLinesEx((Rectangle){ DEMO_ROOM.x + 9, DEMO_ROOM.y + 9,
                                     DEMO_ROOM.width - 18, DEMO_ROOM.height - 18 },
                         1.0f, (Color){ 50, 65, 91, 180 });

    for (int i = 0; i < 4; i++)
    {
        float x = DEMO_ROOM.x + 170.0f + (float)i*276.0f;
        float pulse = 0.55f + 0.45f*sinf(world->globalTime*1.7f + (float)i);
        DrawCircleV((Vector2){ x, DEMO_ROOM.y + 22.0f }, 3.0f, Fade((Color){ 116, 136, 176, 255 }, pulse));
        DrawCircleV((Vector2){ x, DEMO_ROOM.y + DEMO_ROOM.height - 22.0f }, 3.0f,
                    Fade((Color){ 116, 136, 176, 255 }, pulse));
    }
}

static void DemoDrawArcEffect(const DemoArcEffect *arc, DemoRenderMode mode, float time)
{
    Color color = DemoVisualColor(arc->visualId);
    float progress = 1.0f - arc->life/fmaxf(arc->totalLife, 0.001f);
    float start = (arc->angle - arc->sweep*0.5f)*RAD2DEG;
    float end = (arc->angle + arc->sweep*0.5f)*RAD2DEG;
    float alpha = arc->telegraph ? (0.32f + 0.26f*sinf(time*18.0f)) : (1.0f - progress)*0.88f;
    float inner = fmaxf(1.0f, arc->radius - arc->width*0.5f);
    float outer = arc->radius + arc->width*0.5f;

    if (arc->telegraph)
    {
        int segments = mode == DEMO_RENDER_PIXEL ? 9 : 16;
        float step = (end - start)/(float)segments;
        for (int i = 0; i < segments; i += 2)
            DrawRing(arc->position, inner, outer, start + step*(float)i,
                     start + step*(float)(i + 1), 8, Fade(color, alpha));
        DrawLineEx(arc->position, DemoAddScaled(arc->position, DemoDirection(arc->angle), outer),
                   mode == DEMO_RENDER_PIXEL ? 2.0f : 1.2f, Fade(color, 0.72f));
    }
    else
    {
        float sweepHead = arc->melee ? DemoEaseOutCubic(progress) : 1.0f;
        float visibleEnd = start + (end - start)*sweepHead;
        if (mode != DEMO_RENDER_PIXEL)
        {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRing(arc->position, fmaxf(1.0f, inner - 8.0f), outer + 10.0f,
                     start, visibleEnd, 32, Fade(color, 0.18f*(1.0f - progress)));
            EndBlendMode();
        }
        DrawRing(arc->position, inner, outer, start, visibleEnd,
                 mode == DEMO_RENDER_PIXEL ? 14 : 40, Fade(color, alpha));
        DrawRingLines(arc->position, inner, outer, start, visibleEnd,
                      mode == DEMO_RENDER_PIXEL ? 14 : 40, Fade(WHITE, alpha*0.74f));
    }
}

static void DemoDrawBeamEffect(const DemoBeamEffect *beam, DemoRenderMode mode, float time)
{
    Color color = DemoVisualColor(beam->visualId);
    Vector2 direction = DemoDirection(beam->angle);
    Vector2 end = DemoAddScaled(beam->position, direction, beam->length);
    float progress = 1.0f - beam->life/fmaxf(beam->totalLife, 0.001f);
    if (beam->telegraph)
    {
        int segments = 14;
        for (int i = 0; i < segments; i += 2)
        {
            Vector2 a = Vector2Lerp(beam->position, end, (float)i/(float)segments);
            Vector2 b = Vector2Lerp(beam->position, end, (float)(i + 1)/(float)segments);
            DrawLineEx(a, b, fmaxf(2.0f, beam->width*0.32f),
                       Fade(color, 0.38f + 0.25f*sinf(time*20.0f)));
        }
    }
    else
    {
        if (mode != DEMO_RENDER_PIXEL)
        {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawLineEx(beam->position, end, beam->width*2.7f,
                       Fade(color, 0.13f*(1.0f - progress)));
            DrawLineEx(beam->position, end, beam->width*1.55f,
                       Fade(color, 0.24f*(1.0f - progress)));
            EndBlendMode();
        }
        DrawLineEx(beam->position, end, beam->width, Fade(color, 0.94f));
        DrawLineEx(beam->position, end, fmaxf(1.0f, beam->width*0.25f), WHITE);
    }
}

static void DemoDrawProjectile(const DemoProjectile *shot, DemoRenderMode mode)
{
    Color color = DemoVisualColor(shot->visualId);
    float lifeAlpha = DemoClamp(shot->life*4.0f, 0.0f, 1.0f);

    for (int i = shot->trailCount - 1; i > 0; i--)
    {
        float alpha = (float)(shot->trailCount - i)/(float)shot->trailCount;
        float width = mode == DEMO_RENDER_PIXEL ? fmaxf(1.0f, shot->radius*0.55f)
                                                 : fmaxf(1.0f, shot->radius*(0.35f + alpha));
        DrawLineEx(shot->trail[i], shot->trail[i - 1], width, Fade(color, 0.06f + alpha*0.22f));
    }

    if (mode != DEMO_RENDER_PIXEL)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(shot->position, shot->radius*3.4f,
                           Fade(color, 0.21f*lifeAlpha), BLANK);
        EndBlendMode();
    }

    if (shot->visualId == DEMO_VIS_VIOLET_CUT)
    {
        float degrees = shot->rotation*RAD2DEG;
        DrawRing(shot->position, shot->radius*0.85f, shot->radius*1.8f,
                 degrees - 72.0f, degrees + 72.0f,
                 mode == DEMO_RENDER_PIXEL ? 8 : 20, Fade(color, lifeAlpha));
        DrawRingLines(shot->position, shot->radius*0.85f, shot->radius*1.8f,
                      degrees - 72.0f, degrees + 72.0f,
                      mode == DEMO_RENDER_PIXEL ? 8 : 20, Fade(WHITE, lifeAlpha*0.78f));
    }
    else if (shot->visualId == DEMO_VIS_GLASS_PRISM)
    {
        Vector2 forward = DemoDirection(shot->rotation);
        Vector2 side = { -forward.y, forward.x };
        Vector2 a = DemoAddScaled(shot->position, forward, shot->radius*1.7f);
        Vector2 b = DemoAddScaled(shot->position, side, shot->radius);
        Vector2 c = DemoAddScaled(shot->position, side, -shot->radius);
        DrawTriangle(a, b, c, Fade(color, lifeAlpha));
        DrawTriangleLines(a, b, c, WHITE);
    }
    else if (shot->visualId == DEMO_VIS_CALLIGRAPHY_INK)
    {
        DrawCircleV(shot->position, shot->radius*1.2f, Fade(color, lifeAlpha));
        DrawCircleV((Vector2){ shot->position.x - shot->radius*0.45f, shot->position.y - shot->radius*0.4f },
                    shot->radius*0.45f, Fade((Color){ 8, 20, 28, 255 }, lifeAlpha));
    }
    else
    {
        if (mode == DEMO_RENDER_PIXEL)
            DrawRectangle((int)(shot->position.x - shot->radius), (int)(shot->position.y - shot->radius),
                          (int)fmaxf(2.0f, shot->radius*2.0f), (int)fmaxf(2.0f, shot->radius*2.0f),
                          Fade(color, lifeAlpha));
        else DrawCircleV(shot->position, shot->radius, Fade(color, lifeAlpha));
        DrawCircleV(shot->position, fmaxf(1.5f, shot->radius*0.34f), WHITE);
    }
}

static void DemoDrawCaptureField(const DemoWorld *world, const DemoCaptureField *field,
                                 DemoRenderMode mode)
{
    Color color = DemoVisualColor(field->visualId);
    float progress = 1.0f - field->life/fmaxf(field->totalLife, 0.001f);
    float pulse = 0.86f + sinf(world->globalTime*22.0f)*0.05f;
    float radius = field->radius*pulse;
    DrawRingLines(field->position, radius - 2.0f, radius + 2.0f,
                  0.0f, 360.0f, mode == DEMO_RENDER_PIXEL ? 32 : 72, Fade(color, 0.72f));
    DrawRingLines(field->position, radius*0.60f, radius*0.63f,
                  progress*240.0f, progress*240.0f + 260.0f,
                  mode == DEMO_RENDER_PIXEL ? 28 : 64, Fade(color, 0.48f));

    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
    {
        const DemoProjectile *shot = &world->projectiles[i];
        if (!shot->active || !shot->hostile) continue;
        float distance = Vector2Distance(shot->position, field->position);
        if (distance < field->radius)
            DrawLineEx(shot->position, field->position, mode == DEMO_RENDER_PIXEL ? 1.0f : 2.0f,
                       Fade(color, 0.18f));
    }
}

static void DemoDrawParticles(const DemoWorld *world, DemoRenderMode mode)
{
    if (mode != DEMO_RENDER_PIXEL) BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < DEMO_MAX_PARTICLES; i++)
    {
        const DemoParticle *particle = &world->particles[i];
        if (!particle->active) continue;
        float alpha = DemoClamp(particle->life/fmaxf(particle->totalLife, 0.001f), 0.0f, 1.0f);
        float size = particle->size*(0.45f + alpha*0.55f);
        if (mode == DEMO_RENDER_PIXEL)
            DrawRectangle((int)(particle->position.x - size*0.5f), (int)(particle->position.y - size*0.5f),
                          (int)fmaxf(1.0f, size), (int)fmaxf(1.0f, size), Fade(particle->color, alpha));
        else DrawCircleV(particle->position, size, Fade(particle->color, alpha*0.68f));
    }
    if (mode != DEMO_RENDER_PIXEL) EndBlendMode();
}

/* Disegno nemico generico (ombra + sprite + flash + barra vita): niente piu'
 * disegni cuciti sulla singola scena curata, il pattern ora arriva da un pool
 * aperto. Lo sprite e' solo cosmetico, ciclato ad ogni respawn. */
static void DemoDrawEnemy(const DemoAssets *assets, const DemoWorld *world)
{
    const DemoEnemy *enemy = &world->enemy;
    if (!enemy->alive && enemy->corpseFade <= 0.0f) return;

    float bob = enemy->alive ? sinf(world->globalTime*4.2f)*3.0f : 0.0f;
    Vector2 position = { enemy->position.x, enemy->position.y + bob };
    float fadeAlpha = enemy->alive ? 1.0f
                                   : DemoClamp(enemy->corpseFade/DEMO_ENEMY_CORPSE_FADE_SECONDS, 0.0f, 1.0f);
    Color tint = enemy->hitFlash > 0.0f ? WHITE : Fade((Color){ 248, 248, 255, 255 }, fadeAlpha);

    DrawEllipse((int)position.x, (int)(position.y + 28.0f), 38.0f, 11.0f, Fade(BLACK, 0.42f*fadeAlpha));
    DemoDrawTextureCentered(DemoEnemyTexture(assets, enemy->spriteKind), position, 56.0f, 56.0f, 0.0f, tint);

    if (enemy->alive && enemy->hp > 0.0f)
    {
        float ratio = enemy->hp/DEMO_ENEMY_MAX_HP;
        DrawRectangle((int)position.x - 26, (int)position.y - 40, 52, 5, (Color){ 9, 12, 19, 220 });
        DrawRectangle((int)position.x - 25, (int)position.y - 39, (int)(50.0f*ratio), 3,
                      (Color){ 255, 101, 116, 255 });
    }
}

/* La pistola base e' l'unica arma con uno sprite dedicato in mano al player:
 * le armi generate non hanno un modello proprio, si vedono solo attraverso i
 * comandi visuali che emettono. Se il player ricade sulla pistola perche' la
 * sandbox arma e' morta, l'arma in mano ricompare da sola. */
static void DemoDrawPlayerWeapon(const DemoAssets *assets, const DemoWorld *world, bool baseWeaponActive)
{
    if (!baseWeaponActive) return;
    float degrees = world->player.aimAngle*RAD2DEG;
    Vector2 gunPosition = DemoAddScaled(world->player.position, DemoDirection(world->player.aimAngle), 24.0f);
    DemoDrawTextureCentered(assets->handgun, gunPosition, 58.0f, 58.0f, degrees, (Color){ 198, 214, 222, 255 });
}

static void DemoDrawPlayer(const DemoAssets *assets, const DemoWorld *world, bool baseWeaponActive)
{
    Vector2 position = world->player.position;
    float bob = sinf(world->globalTime*8.0f)*1.5f;
    Color tint = (world->player.invulnerability > 0.0f && fmodf(world->globalTime, 0.10f) < 0.05f)
                     ? Fade(WHITE, 0.35f) : WHITE;
    DrawEllipse((int)position.x, (int)(position.y + 19.0f), 17.0f, 6.0f, Fade(BLACK, 0.44f));
    DemoDrawPlayerWeapon(assets, world, baseWeaponActive);
    DemoDrawTextureCentered(assets->player, (Vector2){ position.x, position.y + bob },
                            34.0f, 34.0f, 0.0f, tint);

    if (world->player.statusTime > 0.0f)
        DrawRingLines(position, 18.0f, 21.0f, world->globalTime*90.0f,
                      world->globalTime*90.0f + 250.0f, 24,
                      Fade(DemoVisualColor(DEMO_VIS_VOID_ECHO), 0.68f));
}

static void DemoDrawHitboxLegend(const DemoWorld *world)
{
    DrawCircleLines((int)world->player.position.x, (int)world->player.position.y, 12.0f,
                    Fade((Color){ 255, 255, 255, 255 }, 0.38f));
    DrawCircleV(world->player.position, 2.0f, (Color){ 255, 255, 255, 220 });
}

static void DemoDrawKoOverlay(const DemoWorld *world)
{
    if (!world->player.dead) return;
    DrawRectangle(0, 0, DEMO_WIDTH, DEMO_HEIGHT, Fade(BLACK, 0.55f));
    const char *text = "KO - reset";
    int size = 48;
    int width = MeasureText(text, size);
    DrawText(text, (DEMO_WIDTH - width)/2, DEMO_HEIGHT/2 - size/2, size, (Color){ 255, 104, 116, 255 });
}

static const char *DemoWeaponDisplayName(const DemoPool *weaponPool)
{
    if (weaponPool->current == 0) return "pistola base";
    return weaponPool->entries[weaponPool->current - 1].fileName;
}

static void DemoDrawWorldHud(const DemoWorld *world, const DemoPool *enemyPool,
                             const DemoPool *weaponPool, DemoRenderMode mode)
{
    Color accent = (Color){ 126, 224, 255, 255 };
    DrawRectangle(0, 0, DEMO_WIDTH, 92, (Color){ 7, 10, 18, 255 });
    DrawRectangle(0, 90, DEMO_WIDTH, 2, accent);

    DrawText(TextFormat("enemy %d/%d: %s", enemyPool->current + 1, enemyPool->count,
                        world->enemyScript.fileName[0] != '\0' ? world->enemyScript.fileName : "--"),
             24, 14, 18, (Color){ 239, 244, 252, 255 });
    /* Il nemico non ha ripiego: senza sandbox resta fermo e inoffensivo, e
     * l'HUD deve dirlo invece di parlare di un fallback inesistente. */
    DrawText(TextFormat("Lua: %s  cmd/tick: %d",
                        world->enemyScript.ready ? "ON" : "KO -> nemico inerte",
                        world->lastEnemyCommandCount),
             24, 36, 14, world->enemyScript.ready ? (Color){ 118, 255, 178, 255 } : (Color){ 255, 104, 116, 255 });
    if (!world->enemyScript.ready && world->enemyScript.error[0] != '\0')
        DrawText(world->enemyScript.error, 24, 56, 12, (Color){ 255, 158, 165, 255 });

    DrawText(TextFormat("weapon %d/%d: %s", weaponPool->current, weaponPool->count,
                        DemoWeaponDisplayName(weaponPool)),
             660, 14, 18, (Color){ 239, 244, 252, 255 });
    if (weaponPool->current == 0)
        DrawText("pistola base: nessuna sandbox", 660, 36, 14, (Color){ 151, 166, 190, 255 });
    else
    {
        /* "FALLBACK" solo dove il ripiego esiste davvero: sandbox arma morta
         * significa che il click torna alla pistola base, non che il player
         * resta senz'arma. */
        DrawText(TextFormat("Lua: %s  cmd/tick: %d",
                            world->weaponScript.ready ? "ON" : "FALLBACK -> pistola base",
                            world->lastWeaponCommandCount),
                 660, 36, 14, world->weaponScript.ready ? (Color){ 118, 255, 178, 255 } : (Color){ 255, 104, 116, 255 });
        if (!world->weaponScript.ready && world->weaponScript.error[0] != '\0')
            DrawText(world->weaponScript.error, 660, 56, 12, (Color){ 255, 158, 165, 255 });
    }

    DrawText(world->infoMessageTimer > 0.0f ? "GEN: generazione: arriva con WP4" : "GEN: --",
             24, 74, 15, (Color){ 151, 166, 190, 255 });

    DrawRectangle(0, 660, DEMO_WIDTH, 60, (Color){ 7, 10, 18, 245 });
    DrawRectangle(22, 676, 156, 12, (Color){ 29, 35, 49, 255 });
    DrawRectangle(24, 678, (int)(152.0f*DemoClamp(world->player.hp/DEMO_PLAYER_MAX_HP, 0.0f, 1.0f)), 8,
                  (Color){ 255, 86, 108, 255 });
    DrawText(TextFormat("HP %.1f/%.0f", world->player.hp, DEMO_PLAYER_MAX_HP), 22, 694, 13,
             (Color){ 206, 216, 232, 255 });
    DrawText(TextFormat("proiettili %d", DemoActiveProjectileCount(world)), 212, 676, 14,
             (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("echi %d", world->storedEchoes), 212, 697, 14, accent);
    DrawText(TextFormat("hit %d", world->hitsDealt), 372, 676, 14, accent);
    DrawText(TextFormat("nemico HP %.0f/%.0f", world->enemy.hp, DEMO_ENEMY_MAX_HP), 372, 697, 14,
             (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("catturati %d", world->capturedTotal), 560, 676, 14,
             (Color){ 188, 201, 222, 255 });

    const char *renderName = mode == DEMO_RENDER_PIXEL ? "PIXEL" : mode == DEMO_RENDER_SMOOTH ? "SMOOTH" : "IBRIDO";
    DrawText(TextFormat("RENDER %s", renderName), 900, 676, 15, accent);
    DrawText("WASD mouse | G/H genera (WP4) | N/M cicla | B brief | R reset | 1/2/3 render | TAB split | SPAZIO pausa",
             505, 697, 13, (Color){ 173, 186, 208, 255 });
}

static void DemoDrawWorld(DemoAssets *assets, const DemoWorld *world, const DemoPool *enemyPool,
                          const DemoPool *weaponPool, DemoRenderMode mode)
{
    bool baseWeaponActive = DemoBaseWeaponActive(world, weaponPool);
    DemoSetTextureFiltering(assets, mode);
    DemoDrawArenaBackdrop(world, mode);

    for (int i = 0; i < DEMO_MAX_ARCS; i++)
        if (world->arcs[i].active && world->arcs[i].telegraph)
            DemoDrawArcEffect(&world->arcs[i], mode, world->globalTime);
    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
        if (world->beams[i].active && world->beams[i].telegraph)
            DemoDrawBeamEffect(&world->beams[i], mode, world->globalTime);

    DemoDrawEnemy(assets, world);
    DemoDrawPlayer(assets, world, baseWeaponActive);

    for (int i = 0; i < DEMO_MAX_CAPTURE_FIELDS; i++)
        if (world->captureFields[i].active) DemoDrawCaptureField(world, &world->captureFields[i], mode);
    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
        if (world->projectiles[i].active) DemoDrawProjectile(&world->projectiles[i], mode);
    for (int i = 0; i < DEMO_MAX_ARCS; i++)
        if (world->arcs[i].active && !world->arcs[i].telegraph)
            DemoDrawArcEffect(&world->arcs[i], mode, world->globalTime);
    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
        if (world->beams[i].active && !world->beams[i].telegraph)
            DemoDrawBeamEffect(&world->beams[i], mode, world->globalTime);
    DemoDrawParticles(world, mode);
    DemoDrawHitboxLegend(world);
    DemoDrawKoOverlay(world);
    DemoDrawWorldHud(world, enemyPool, weaponPool, mode);
}

static bool DemoRendererInit(DemoRenderer *renderer)
{
    char shaderPath[1024];
    memset(renderer, 0, sizeof *renderer);
    renderer->pixelTarget = LoadRenderTexture(DEMO_PIXEL_WIDTH, DEMO_PIXEL_HEIGHT);
    renderer->highTarget = LoadRenderTexture(DEMO_WIDTH, DEMO_HEIGHT);
    renderer->finalTarget = LoadRenderTexture(DEMO_WIDTH, DEMO_HEIGHT);
    if (!IsRenderTextureValid(renderer->pixelTarget) ||
        !IsRenderTextureValid(renderer->highTarget) ||
        !IsRenderTextureValid(renderer->finalTarget)) return false;

    SetTextureFilter(renderer->pixelTarget.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(renderer->highTarget.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(renderer->finalTarget.texture, TEXTURE_FILTER_BILINEAR);

    DemoBuildPath(shaderPath, sizeof shaderPath, "shaders/hybrid.fs");
    renderer->hybridShader = LoadShader(NULL, shaderPath);
    renderer->shaderReady = IsShaderValid(renderer->hybridShader);
    if (renderer->shaderReady)
    {
        renderer->timeLocation = GetShaderLocation(renderer->hybridShader, "time");
        renderer->modeLocation = GetShaderLocation(renderer->hybridShader, "mode");
        renderer->pixelSizeLocation = GetShaderLocation(renderer->hybridShader, "pixelSize");
    }
    return true;
}

static void DemoRendererUnload(DemoRenderer *renderer)
{
    if (renderer->shaderReady) UnloadShader(renderer->hybridShader);
    if (IsRenderTextureValid(renderer->pixelTarget)) UnloadRenderTexture(renderer->pixelTarget);
    if (IsRenderTextureValid(renderer->highTarget)) UnloadRenderTexture(renderer->highTarget);
    if (IsRenderTextureValid(renderer->finalTarget)) UnloadRenderTexture(renderer->finalTarget);
    memset(renderer, 0, sizeof *renderer);
}

static void DemoRenderWorldToTarget(DemoAssets *assets, const DemoWorld *world, const DemoPool *enemyPool,
                                    const DemoPool *weaponPool, DemoRenderMode mode, RenderTexture2D target)
{
    float scale = (float)target.texture.width/(float)DEMO_WIDTH;
    Camera2D camera = { 0 };
    camera.zoom = scale;
    BeginTextureMode(target);
    BeginMode2D(camera);
    DemoDrawWorld(assets, world, enemyPool, weaponPool, mode);
    EndMode2D();
    EndTextureMode();
}

static void DemoDrawTarget(DemoRenderer *renderer, Texture2D texture, Rectangle destination,
                           DemoRenderMode mode, float time)
{
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)-texture.height };
    Vector2 origin = { 0.0f, 0.0f };
    if (renderer->shaderReady)
    {
        float shaderMode = (float)mode;
        float pixelSize = mode == DEMO_RENDER_HYBRID ? 2.0f : 1.0f;
        if (renderer->timeLocation >= 0)
            SetShaderValue(renderer->hybridShader, renderer->timeLocation, &time, SHADER_UNIFORM_FLOAT);
        if (renderer->modeLocation >= 0)
            SetShaderValue(renderer->hybridShader, renderer->modeLocation, &shaderMode, SHADER_UNIFORM_FLOAT);
        if (renderer->pixelSizeLocation >= 0)
            SetShaderValue(renderer->hybridShader, renderer->pixelSizeLocation, &pixelSize, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(renderer->hybridShader);
        DrawTexturePro(texture, source, destination, origin, 0.0f, WHITE);
        EndShaderMode();
    }
    else DrawTexturePro(texture, source, destination, origin, 0.0f, WHITE);
}

static void DemoComposeFrame(DemoRenderer *renderer, DemoAssets *assets, const DemoWorld *world,
                             const DemoPool *enemyPool, const DemoPool *weaponPool,
                             DemoRenderMode mode, bool split, bool showControls)
{
    if (split || mode == DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(assets, world, enemyPool, weaponPool, DEMO_RENDER_PIXEL, renderer->pixelTarget);
    if (split || mode != DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(assets, world, enemyPool, weaponPool,
                                split ? DEMO_RENDER_HYBRID : mode, renderer->highTarget);

    BeginTextureMode(renderer->finalTarget);
    ClearBackground((Color){ 5, 8, 15, 255 });
    if (split)
    {
        DrawText("PIXEL PURO vs IBRIDO: stessa arena, stesse hitbox (si mira nel pannello sotto il cursore)",
                 24, 14, 20, (Color){ 239, 244, 252, 255 });
        DemoDrawTarget(renderer, renderer->pixelTarget.texture, DEMO_SPLIT_LEFT,
                       DEMO_RENDER_PIXEL, world->globalTime);
        DemoDrawTarget(renderer, renderer->highTarget.texture, DEMO_SPLIT_RIGHT,
                       DEMO_RENDER_HYBRID, world->globalTime);
    }
    else
    {
        Texture2D source = mode == DEMO_RENDER_PIXEL ? renderer->pixelTarget.texture : renderer->highTarget.texture;
        DemoDrawTarget(renderer, source, (Rectangle){ 0, 0, DEMO_WIDTH, DEMO_HEIGHT }, mode, world->globalTime);
    }

    if (showControls)
    {
        DrawRectangle(0, DEMO_HEIGHT - 20, DEMO_WIDTH, 20, (Color){ 3, 5, 10, 232 });
        DrawText("WASD mouse | click fuoco | G/H genera | N/M cicla | B brief | R reset | 1/2/3 render | TAB split | SPAZIO pausa",
                 18, DEMO_HEIGHT - 17, 12, (Color){ 178, 191, 213, 255 });
    }
    EndTextureMode();
}

static void DemoDrawFinalToWindow(RenderTexture2D target)
{
    float scaleX = (float)GetScreenWidth()/(float)DEMO_WIDTH;
    float scaleY = (float)GetScreenHeight()/(float)DEMO_HEIGHT;
    float scale = fminf(scaleX, scaleY);
    Rectangle source = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
    Rectangle destination = {
        ((float)GetScreenWidth() - (float)DEMO_WIDTH*scale)*0.5f,
        ((float)GetScreenHeight() - (float)DEMO_HEIGHT*scale)*0.5f,
        (float)DEMO_WIDTH*scale,
        (float)DEMO_HEIGHT*scale
    };
    DrawTexturePro(target.texture, source, destination, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

static bool DemoExportFrame(RenderTexture2D target, const char *directory, int frame)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/frame-%04d.png", directory, frame);
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    bool success = ExportImage(image, path);
    UnloadImage(image);
    return success;
}

static const char *DemoArgValue(int argc, char **argv, const char *name)
{
    for (int i = 1; i + 1 < argc; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static bool DemoHasArg(int argc, char **argv, const char *name)
{
    for (int i = 1; i < argc; i++) if (strcmp(argv[i], name) == 0) return true;
    return false;
}

int main(int argc, char **argv)
{
    bool capture = DemoHasArg(argc, argv, "--capture");
    const char *captureDirectory = DemoArgValue(argc, argv, "--capture");
    if (capture && (captureDirectory == NULL || captureDirectory[0] == '\0')) captureDirectory = "frames";

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | (capture ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(DEMO_WIDTH, DEMO_HEIGHT, "Worldsmelt - Lua Procedural Combat Lab");
    SetWindowMinSize(800, 450);
    SetTargetFPS(60);

    DemoAssets assets;
    DemoRenderer renderer;
    DemoWorld world = { 0 };
    DemoPool enemyPool = { 0 };
    DemoPool weaponPool = { 0 };

    if (!DemoLoadAssets(&assets))
    {
        fprintf(stderr, "ERRORE: asset CC0 mancanti accanto all'eseguibile.\n");
        CloseWindow();
        return 2;
    }
    if (!DemoRendererInit(&renderer))
    {
        fprintf(stderr, "ERRORE: creazione RenderTexture fallita.\n");
        DemoUnloadAssets(&assets);
        CloseWindow();
        return 3;
    }

    /* Pool nemico/arma: curati subito, poi la cartella generated/ — che sotto
     * --capture NON si legge, cosi' lo smoke test headless resta deterministico
     * qualunque cosa il generatore abbia lasciato su disco. Lo slot 0 dell'arma
     * (pistola base) e' implicito: weaponPool.current parte a 0 senza voce. */
    static const char *const enemyCurated[] = { "spider_arc.lua", "snail_calligrapher.lua", "glass_moth.lua" };
    static const char *const weaponCurated[] = { "halberd_gravity.lua", "squid_reload.lua" };
    DemoPoolInitCurated(&enemyPool, enemyCurated, 3);
    DemoPoolInitCurated(&weaponPool, weaponCurated, 2);
    DemoEnsureGeneratedDirs();
    if (!capture)
    {
        DemoPoolScanGenerated(&enemyPool, DEMO_ENEMY_GENERATED_DIR);
        DemoPoolScanGenerated(&weaponPool, DEMO_WEAPON_GENERATED_DIR);
    }

    world.cosmeticRng = 0xC0FFEE11u;
    world.player.hp = DEMO_PLAYER_MAX_HP;
    world.player.position = (Vector2){ 640.0f, 532.0f };
    DemoEnemySpawn(&world, &enemyPool);

    if (capture)
    {
        if (!DirectoryExists(captureDirectory) && MakeDirectory(captureDirectory) != 0)
        {
            fprintf(stderr, "ERRORE: impossibile creare la cartella frame: %s\n", captureDirectory);
            DemoScriptUnload(&world.enemyScript);
            DemoScriptUnload(&world.weaponScript);
            DemoRendererUnload(&renderer);
            DemoUnloadAssets(&assets);
            CloseWindow();
            return 4;
        }

        float enemyCycleTimer = 0.0f;
        /* Cattura headless: nessun input reale, il player va in autopilota. */
        DemoFrameInput autopilot = { 0 };
        for (int frame = 0; frame < DEMO_CAPTURE_FRAMES; frame++)
        {
            for (int step = 0; step < 4; step++)
            {
                DemoUpdateWorld(&world, &enemyPool, &weaponPool, DEMO_FIXED_DT, &autopilot);
                enemyCycleTimer += DEMO_FIXED_DT;
                if (enemyCycleTimer >= DEMO_CAPTURE_ENEMY_CYCLE_SECONDS && enemyPool.count > 0)
                {
                    enemyCycleTimer -= DEMO_CAPTURE_ENEMY_CYCLE_SECONDS;
                    enemyPool.current = (enemyPool.current + 1)%enemyPool.count;
                    DemoEnemyBeginPattern(&world, &enemyPool);
                }
            }
            DemoComposeFrame(&renderer, &assets, &world, &enemyPool, &weaponPool,
                             DEMO_RENDER_SMOOTH, false, false);
            if (!DemoExportFrame(renderer.finalTarget, captureDirectory, frame))
            {
                fprintf(stderr, "ERRORE: export fallito al frame %d.\n", frame);
                DemoScriptUnload(&world.enemyScript);
                DemoScriptUnload(&world.weaponScript);
                DemoRendererUnload(&renderer);
                DemoUnloadAssets(&assets);
                CloseWindow();
                return 5;
            }
        }
        printf("Cattura completata: %d frame PNG a %d fps in %s\n",
               DEMO_CAPTURE_FRAMES, DEMO_CAPTURE_FPS, captureDirectory);
    }
    else
    {
        DemoRenderMode mode = DEMO_RENDER_SMOOTH;
        bool split = false;
        bool paused = false;
        float accumulator = 0.0f;
        /* Dichiarato fuori dal loop apposta: specialPressed sopravvive ai frame
         * che non eseguono nessun passo fisso (accumulator sotto 1/60 col
         * jitter del vsync) e viene consumato dal primo tick utile. */
        DemoFrameInput input = { 0 };
        input.interactive = true;
        while (!WindowShouldClose())
        {
            float frameTime = fminf(GetFrameTime(), 0.10f);
            if (IsKeyPressed(KEY_ONE)) mode = DEMO_RENDER_PIXEL;
            if (IsKeyPressed(KEY_TWO)) mode = DEMO_RENDER_SMOOTH;
            if (IsKeyPressed(KEY_THREE)) mode = DEMO_RENDER_HYBRID;
            if (IsKeyPressed(KEY_TAB)) split = !split;
            if (IsKeyPressed(KEY_SPACE)) paused = !paused;
            if (IsKeyPressed(KEY_R)) DemoResetArena(&world, &enemyPool, &weaponPool);

            if (IsKeyPressed(KEY_N) && enemyPool.count > 0)
            {
                enemyPool.current = (enemyPool.current + 1)%enemyPool.count;
                DemoEnemyBeginPattern(&world, &enemyPool);
            }
            if (IsKeyPressed(KEY_M))
            {
                weaponPool.current = (weaponPool.current + 1)%(weaponPool.count + 1);
                if (weaponPool.current == 0) DemoScriptUnload(&world.weaponScript);
                else DemoScriptLoad(&world.weaponScript, &weaponPool.entries[weaponPool.current - 1],
                                    DEMO_WEAPON_GENERATED_DIR, true,
                                    0x77A1u + (unsigned int)weaponPool.current*53u);
            }
            /* G/H/B: nessun processo figlio in questo WP, solo il segnaposto
             * HUD che WP4 sostituira' con lo stato vero del generatore. */
            if (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_H) || IsKeyPressed(KEY_B))
                world.infoMessageTimer = DEMO_INFO_MESSAGE_SECONDS;

            DemoPoolPollTick(&enemyPool, DEMO_ENEMY_GENERATED_DIR, frameTime);
            DemoPoolPollTick(&weaponPool, DEMO_WEAPON_GENERATED_DIR, frameTime);

            /* Campionamento dell'input: una volta per frame, mai dentro il
             * loop a passo fisso. La mira dipende da `split` perche' in
             * confronto A/B il mondo vive dentro un pannello, non a schermo
             * pieno. */
            input.aim = DemoWindowToLogic(GetMousePosition(), split);
            input.move = (Vector2){ 0.0f, 0.0f };
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.move.x -= 1.0f;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.move.x += 1.0f;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.move.y -= 1.0f;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.move.y += 1.0f;
            input.fireHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) input.specialPressed = true;
            /* In pausa non gira nessun tick: un click destro non deve restare
             * in canna per scattare alla ripresa. */
            if (paused) input.specialPressed = false;

            if (!paused) accumulator += frameTime;
            while (accumulator >= DEMO_FIXED_DT)
            {
                DemoUpdateWorld(&world, &enemyPool, &weaponPool, DEMO_FIXED_DT, &input);
                accumulator -= DEMO_FIXED_DT;
            }

            DemoComposeFrame(&renderer, &assets, &world, &enemyPool, &weaponPool, mode, split, true);
            BeginDrawing();
            ClearBackground(BLACK);
            DemoDrawFinalToWindow(renderer.finalTarget);
            EndDrawing();
        }
    }

    DemoScriptUnload(&world.enemyScript);
    DemoScriptUnload(&world.weaponScript);
    DemoRendererUnload(&renderer);
    DemoUnloadAssets(&assets);
    CloseWindow();
    return 0;
}
