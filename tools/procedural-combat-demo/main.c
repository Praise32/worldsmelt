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
#define DEMO_SCENE_COUNT 6
#define DEMO_SCENE_SECONDS 5.0f
#define DEMO_FIXED_DT (1.0f/60.0f)

#define DEMO_MAX_PROJECTILES 640
#define DEMO_MAX_ARCS 96
#define DEMO_MAX_BEAMS 48
#define DEMO_MAX_CAPTURE_FIELDS 16
#define DEMO_MAX_PARTICLES 768
#define DEMO_MAX_DUMMIES 6
#define DEMO_TRAIL_POINTS 10

#define DEMO_PI 3.14159265358979323846f
#define DEMO_TAU (2.0f*DEMO_PI)

static const Rectangle DEMO_ROOM = { 58.0f, 88.0f, 1164.0f, 566.0f };

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

typedef struct DemoDummy {
    bool active;
    Vector2 position;
    int textureKind;
    float hp;
    float maxHp;
    float hitFlash;
} DemoDummy;

typedef struct DemoWorld {
    DemoScriptRuntime script;
    DemoProjectile projectiles[DEMO_MAX_PROJECTILES];
    DemoArcEffect arcs[DEMO_MAX_ARCS];
    DemoBeamEffect beams[DEMO_MAX_BEAMS];
    DemoCaptureField captureFields[DEMO_MAX_CAPTURE_FIELDS];
    DemoParticle particles[DEMO_MAX_PARTICLES];
    DemoDummy dummies[DEMO_MAX_DUMMIES];

    int scene;
    float sceneTime;
    float globalTime;
    float ambientTimer;
    float frostTimer;
    float playerHp;
    float playerInvulnerability;
    float playerStatusTime;
    float playerStatusStrength;
    float actorHitFlash;
    int storedEchoes;
    int capturedTotal;
    int hitsDealt;
    int lastCommandCount;

    Vector2 playerPosition;
    Vector2 actorPosition;
    Vector2 actorVelocity;
    Vector2 actorTrail[24];
    int actorTrailCount;
    float actorTrailTimer;
    float aimAngle;
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

static const char *DemoSceneTitle(int scene)
{
    static const char *const titles[DEMO_SCENE_COUNT] = {
        "RAGNO FALCIATORE: PROIETTILI A MEZZALUNA",
        "ALABARDA + NUCLEO GRAVITAZIONALE",
        "FUCILE-SEPPIA: RICARICA CHE ASSORBE COLPI",
        "LUMACA CALLIGRAFA: TRAIL -> LASER -> SCHEGGE",
        "FALENA DI VETRO: ORBITE E ROSA DI PROIETTILI",
        "STESSA SIMULAZIONE: PIXEL PURO vs IBRIDO SHADER"
    };
    return titles[(scene >= 0 && scene < DEMO_SCENE_COUNT) ? scene : 0];
}

static const char *DemoSceneAlphabet(int scene)
{
    static const char *const text[DEMO_SCENE_COUNT] = {
        "Lua: aim_at_player -> telegraph_arc -> emit_arc",
        "Lua: aim_snapshot -> melee_sweep -> capture_radius -> release_echoes",
        "Lua: capture_radius -> emit_orbit -> telegraph_beam -> release_echoes",
        "Lua: set_velocity -> telegraph_beam -> emit_beam -> emit_orbit",
        "Lua: telegraph_beam x2 -> emit_ring -> emit_orbit -> add_status",
        "Una simulazione e una hitbox; cambiano soltanto raster e FX"
    };
    return text[(scene >= 0 && scene < DEMO_SCENE_COUNT) ? scene : 0];
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

static void DemoScriptUnload(DemoScriptRuntime *runtime)
{
    if (runtime->sandbox != NULL) ScriptSandboxDestroy(runtime->sandbox);
    memset(runtime, 0, sizeof *runtime);
}

static bool DemoScriptLoad(DemoScriptRuntime *runtime, const char *fileName,
                           bool playerOwned, unsigned int seed)
{
    char path[1024];
    char *source = NULL;
    uint64_t selfHandle = playerOwned ? 100u : 200u;
    uint64_t playerHandle = 1u;

    DemoScriptUnload(runtime);
    runtime->playerOwned = playerOwned;
    snprintf(runtime->fileName, sizeof runtime->fileName, "%s", fileName);
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

    DemoBuildPath(path, sizeof path, TextFormat("scripts/%s", fileName));
    source = LoadFileText(path);
    if (source == NULL)
    {
        snprintf(runtime->error, sizeof runtime->error, "script assente: %s", fileName);
        return false;
    }
    runtime->ready = ScriptSandboxLoad(runtime->sandbox, fileName, source,
                                       runtime->error, sizeof runtime->error);
    UnloadFileText(source);
    if (runtime->ready && !ScriptSandboxHasFunction(runtime->sandbox, "on_tick"))
    {
        snprintf(runtime->error, sizeof runtime->error, "on_tick assente");
        runtime->ready = false;
    }
    return runtime->ready;
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
    float phase = 0.23f*world->sceneTime;
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

static void DemoDamagePlayer(DemoWorld *world, float amount, Vector2 hitPosition)
{
    if (world->playerInvulnerability > 0.0f) return;
    world->playerHp = fmaxf(0.25f, world->playerHp - amount);
    world->playerInvulnerability = 0.38f;
    DemoSpawnParticles(world, hitPosition, (Color){ 255, 93, 110, 255 }, 14, 90.0f);
}

static void DemoApplyMelee(DemoWorld *world, const DemoScriptCommand *command)
{
    for (int i = 0; i < DEMO_MAX_DUMMIES; i++)
    {
        DemoDummy *dummy = &world->dummies[i];
        if (!dummy->active || dummy->hp <= 0.0f) continue;
        Vector2 delta = Vector2Subtract(dummy->position, (Vector2){ command->x, command->y });
        float distance = Vector2Length(delta);
        float angle = atan2f(delta.y, delta.x);
        if (distance <= command->radius + command->width*0.5f &&
            fabsf(DemoAngleDifference(angle, command->angle)) <= command->sweep*0.5f)
        {
            dummy->hp = fmaxf(0.0f, dummy->hp - command->damage);
            dummy->hitFlash = 0.22f;
            world->hitsDealt++;
            if (world->storedEchoes < 8) world->storedEchoes++;
            DemoSpawnParticles(world, dummy->position, DemoVisualColor(command->visualId), 18, 120.0f);
        }
    }
}

static void DemoConsumeCommands(DemoWorld *world)
{
    const DemoScriptCommand *commands = DemoScriptApiCommands(&world->script.api);
    size_t commandCount = DemoScriptApiCommandCount(&world->script.api);
    bool hostile = !world->script.playerOwned;
    world->lastCommandCount = (int)commandCount;

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
                world->actorVelocity = (Vector2){ command->vx, command->vy };
                break;

            case DEMO_CMD_ADD_STATUS:
                if (command->targetHandle == world->script.api.playerHandle)
                {
                    world->playerStatusTime = fmaxf(world->playerStatusTime, command->duration);
                    world->playerStatusStrength = command->strength;
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

static void DemoSetDummy(DemoWorld *world, int index, Vector2 position, int textureKind, float hp)
{
    if (index < 0 || index >= DEMO_MAX_DUMMIES) return;
    world->dummies[index].active = true;
    world->dummies[index].position = position;
    world->dummies[index].textureKind = textureKind;
    world->dummies[index].hp = hp;
    world->dummies[index].maxHp = hp;
}

static void DemoResetScene(DemoWorld *world, int scene)
{
    static const char *const scripts[DEMO_SCENE_COUNT] = {
        "spider_arc.lua",
        "halberd_gravity.lua",
        "squid_reload.lua",
        "snail_calligrapher.lua",
        "glass_moth.lua",
        "spider_arc.lua"
    };
    static const bool playerOwned[DEMO_SCENE_COUNT] = {
        false, true, true, false, false, false
    };

    DemoScriptUnload(&world->script);
    memset(world, 0, sizeof *world);
    world->scene = (scene + DEMO_SCENE_COUNT)%DEMO_SCENE_COUNT;
    world->playerHp = 6.0f;
    world->playerPosition = (Vector2){ 640.0f, 532.0f };
    world->cosmeticRng = 0xC0FFEE11u ^ (uint32_t)(world->scene*0x9E3779B9u);

    switch (world->scene)
    {
        case 0:
            world->actorPosition = (Vector2){ 640.0f, 210.0f };
            break;
        case 1:
            world->actorPosition = world->playerPosition;
            DemoSetDummy(world, 0, (Vector2){ 465.0f, 284.0f }, 0, 38.0f);
            DemoSetDummy(world, 1, (Vector2){ 682.0f, 210.0f }, 1, 38.0f);
            DemoSetDummy(world, 2, (Vector2){ 875.0f, 330.0f }, 2, 38.0f);
            break;
        case 2:
            world->actorPosition = world->playerPosition;
            DemoSetDummy(world, 0, (Vector2){ 342.0f, 218.0f }, 2, 46.0f);
            DemoSetDummy(world, 1, (Vector2){ 956.0f, 230.0f }, 0, 46.0f);
            break;
        case 3:
            world->actorPosition = (Vector2){ 872.0f, 258.0f };
            break;
        case 4:
            world->actorPosition = (Vector2){ 640.0f, 214.0f };
            break;
        default:
            world->actorPosition = (Vector2){ 640.0f, 218.0f };
            DemoSetDummy(world, 0, (Vector2){ 940.0f, 350.0f }, 1, 80.0f);
            break;
    }

    world->actorTrail[0] = world->actorPosition;
    world->actorTrailCount = 1;
    DemoScriptLoad(&world->script, scripts[world->scene], playerOwned[world->scene],
                   0x51A7u + (unsigned int)world->scene*97u);
}

static Vector2 DemoAutoPlayerPosition(const DemoWorld *world)
{
    float t = world->sceneTime;
    switch (world->scene)
    {
        case 0:
            return (Vector2){ 640.0f + sinf(t*1.45f)*274.0f,
                              510.0f + cosf(t*1.08f)*72.0f };
        case 1:
            return (Vector2){ 640.0f + sinf(t*0.78f)*74.0f,
                              440.0f + cosf(t*0.92f)*42.0f };
        case 2:
            return (Vector2){ 640.0f + sinf(t*0.92f)*104.0f,
                              466.0f + cosf(t*0.71f)*56.0f };
        case 3:
            return (Vector2){ 410.0f + sinf(t*1.18f)*172.0f,
                              474.0f + cosf(t*1.53f)*92.0f };
        case 4:
            return (Vector2){ 640.0f + sinf(t*1.31f)*292.0f,
                              492.0f + cosf(t*1.61f)*88.0f };
        default:
            return (Vector2){ 470.0f + sinf(t*1.38f)*156.0f,
                              492.0f + cosf(t*1.17f)*70.0f };
    }
}

static void DemoUpdatePlayer(DemoWorld *world, float dt, bool allowInput)
{
    Vector2 input = { 0.0f, 0.0f };
    if (allowInput)
    {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.y += 1.0f;
    }

    if (Vector2LengthSqr(input) > 0.0f)
    {
        float speed = 235.0f;
        if (world->playerStatusTime > 0.0f) speed *= DemoClamp(1.0f - world->playerStatusStrength*0.45f, 0.35f, 1.0f);
        input = Vector2Scale(Vector2Normalize(input), speed*dt);
        world->playerPosition = Vector2Add(world->playerPosition, input);
    }
    else
    {
        Vector2 target = DemoAutoPlayerPosition(world);
        float follow = 1.0f - powf(0.0008f, dt);
        world->playerPosition = Vector2Lerp(world->playerPosition, target, follow);
    }

    world->playerPosition.x = DemoClamp(world->playerPosition.x, DEMO_ROOM.x + 24.0f,
                                        DEMO_ROOM.x + DEMO_ROOM.width - 24.0f);
    world->playerPosition.y = DemoClamp(world->playerPosition.y, DEMO_ROOM.y + 24.0f,
                                        DEMO_ROOM.y + DEMO_ROOM.height - 24.0f);
}

static Vector2 DemoAmbientOrigin(const DemoWorld *world, int index)
{
    if (world->scene == 1)
    {
        for (int i = 0; i < DEMO_MAX_DUMMIES; i++)
            if (world->dummies[i].active && i == index%3) return world->dummies[i].position;
    }
    if (world->scene == 2)
    {
        return (index & 1) ? (Vector2){ DEMO_ROOM.x + 42.0f, 250.0f + 92.0f*(float)(index%4) }
                           : (Vector2){ DEMO_ROOM.x + DEMO_ROOM.width - 42.0f, 226.0f + 86.0f*(float)(index%4) };
    }
    return world->actorPosition;
}

static void DemoUpdateAmbientFire(DemoWorld *world, float dt)
{
    if (world->scene != 1 && world->scene != 2 && world->scene != 5) return;
    world->ambientTimer -= dt;
    if (world->ambientTimer > 0.0f) return;

    int volley = (int)(world->sceneTime*10.0f);
    if (world->scene == 1 || world->scene == 2)
    {
        Vector2 origin = DemoAmbientOrigin(world, volley);
        float angle = atan2f(world->playerPosition.y - origin.y, world->playerPosition.x - origin.x);
        int visual = (world->scene == 1) ? DEMO_VIS_GLASS_PRISM : DEMO_VIS_VIOLET_CUT;
        float speed = (world->scene == 1) ? 174.0f : 198.0f;
        DemoSpawnShot(world, origin, angle, speed, 5.0f, 0.35f, 4.2f, visual, true);
        world->ambientTimer = (world->scene == 1) ? 0.24f : 0.17f;
    }
    else
    {
        float angle = atan2f(world->actorPosition.y - world->playerPosition.y,
                             world->actorPosition.x - world->playerPosition.x);
        DemoSpawnShot(world, world->playerPosition, angle, 328.0f, 6.0f, 8.0f,
                      2.4f, DEMO_VIS_GLASS_PRISM, false);
        world->ambientTimer = 0.38f;
    }
}

static void DemoUpdateScript(DemoWorld *world, float dt)
{
    if (!world->script.ready || world->script.sandbox == NULL) return;

    if (world->script.playerOwned) world->actorPosition = world->playerPosition;
    if (world->script.playerOwned)
    {
        Vector2 target = world->scene == 1 ? world->dummies[1].position : (Vector2){ DEMO_ROOM.x + DEMO_ROOM.width, world->playerPosition.y - 80.0f };
        world->aimAngle = atan2f(target.y - world->playerPosition.y, target.x - world->playerPosition.x);
    }
    else
    {
        world->aimAngle = atan2f(world->playerPosition.y - world->actorPosition.y,
                                 world->playerPosition.x - world->actorPosition.x);
    }

    DemoScriptApiBeginFrame(&world->script.api,
                            world->playerPosition.x, world->playerPosition.y,
                            world->actorPosition.x, world->actorPosition.y,
                            world->aimAngle);
    if (!ScriptSandboxCallVoid(world->script.sandbox, "on_tick", 2,
                               (double)dt, (double)world->script.api.selfHandle))
    {
        if (ScriptSandboxIsDisabled(world->script.sandbox))
        {
            snprintf(world->script.error, sizeof world->script.error, "%s",
                     ScriptSandboxDisabledReason(world->script.sandbox));
            world->script.ready = false;
        }
        return;
    }
    DemoConsumeCommands(world);
}

static void DemoUpdateActor(DemoWorld *world, float dt)
{
    if (!world->script.playerOwned)
    {
        world->actorPosition = Vector2Add(world->actorPosition, Vector2Scale(world->actorVelocity, dt));
        world->actorPosition.x = DemoClamp(world->actorPosition.x, DEMO_ROOM.x + 70.0f,
                                           DEMO_ROOM.x + DEMO_ROOM.width - 70.0f);
        world->actorPosition.y = DemoClamp(world->actorPosition.y, DEMO_ROOM.y + 55.0f,
                                           DEMO_ROOM.y + DEMO_ROOM.height*0.60f);
    }
    else world->actorPosition = world->playerPosition;

    world->actorTrailTimer -= dt;
    if (world->actorTrailTimer <= 0.0f)
    {
        int limit = (int)(sizeof world->actorTrail/sizeof world->actorTrail[0]);
        if (world->actorTrailCount < limit) world->actorTrailCount++;
        for (int i = world->actorTrailCount - 1; i > 0; i--) world->actorTrail[i] = world->actorTrail[i - 1];
        world->actorTrail[0] = world->actorPosition;
        world->actorTrailTimer = 0.085f;
    }
}

static void DemoUpdateCaptureFields(DemoWorld *world, float dt)
{
    for (int i = 0; i < DEMO_MAX_CAPTURE_FIELDS; i++)
    {
        DemoCaptureField *field = &world->captureFields[i];
        if (!field->active) continue;
        field->life -= dt;
        if (world->script.playerOwned) field->position = world->playerPosition;

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

        if (shot->hostile && CheckCollisionCircles(shot->position, shot->radius, world->playerPosition, 12.0f))
        {
            DemoDamagePlayer(world, fmaxf(0.2f, shot->damage*0.08f), shot->position);
            shot->active = false;
            continue;
        }
        if (!shot->hostile)
        {
            for (int dummyIndex = 0; dummyIndex < DEMO_MAX_DUMMIES; dummyIndex++)
            {
                DemoDummy *dummy = &world->dummies[dummyIndex];
                if (!dummy->active || dummy->hp <= 0.0f) continue;
                if (CheckCollisionCircles(shot->position, shot->radius, dummy->position, 18.0f))
                {
                    dummy->hp = fmaxf(0.0f, dummy->hp - shot->damage);
                    dummy->hitFlash = 0.18f;
                    world->hitsDealt++;
                    DemoSpawnParticles(world, shot->position, DemoVisualColor(shot->visualId), 12, 95.0f);
                    shot->active = false;
                    break;
                }
            }
            if (world->scene == 5 && shot->active &&
                CheckCollisionCircles(shot->position, shot->radius, world->actorPosition, 38.0f))
            {
                world->actorHitFlash = 0.16f;
                world->hitsDealt++;
                DemoSpawnParticles(world, shot->position, DemoVisualColor(shot->visualId), 12, 105.0f);
                shot->active = false;
            }
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
        if (arc->life <= 0.0f) arc->active = false;
    }

    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
    {
        DemoBeamEffect *beam = &world->beams[i];
        if (!beam->active) continue;
        beam->life -= dt;
        if (beam->hostile && !beam->telegraph && !beam->damageApplied)
        {
            Vector2 end = DemoAddScaled(beam->position, DemoDirection(beam->angle), beam->length);
            if (DemoPointSegmentDistance(world->playerPosition, beam->position, end) <= beam->width*0.5f + 11.0f)
                DemoDamagePlayer(world, fmaxf(0.25f, beam->damage*0.08f), world->playerPosition);
            beam->damageApplied = true;
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

static void DemoUpdateWorld(DemoWorld *world, float dt, bool allowInput)
{
    world->sceneTime += dt;
    world->globalTime += dt;
    world->playerInvulnerability = fmaxf(0.0f, world->playerInvulnerability - dt);
    world->playerStatusTime = fmaxf(0.0f, world->playerStatusTime - dt);
    world->actorHitFlash = fmaxf(0.0f, world->actorHitFlash - dt);
    for (int i = 0; i < DEMO_MAX_DUMMIES; i++)
        world->dummies[i].hitFlash = fmaxf(0.0f, world->dummies[i].hitFlash - dt);

    DemoUpdatePlayer(world, dt, allowInput);
    DemoUpdateScript(world, dt);
    DemoUpdateActor(world, dt);
    DemoUpdateAmbientFire(world, dt);
    DemoUpdateCaptureFields(world, dt);
    DemoUpdateProjectiles(world, dt);
    DemoUpdateArcsAndBeams(world, dt);
    DemoUpdateParticles(world, dt);

    if ((world->scene == 3 || world->scene == 4) && world->actorTrailTimer < dt*0.5f)
        DemoSpawnParticles(world, world->actorPosition,
                           world->scene == 3 ? DemoVisualColor(DEMO_VIS_CALLIGRAPHY_INK)
                                             : DemoVisualColor(DEMO_VIS_GLASS_PRISM),
                           1, 14.0f);
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

static Texture2D DemoDummyTexture(const DemoAssets *assets, int kind)
{
    if (kind == 0) return assets->spook;
    if (kind == 1) return assets->gelatine;
    return assets->stareyes;
}

static void DemoDrawDummies(const DemoAssets *assets, const DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_DUMMIES; i++)
    {
        const DemoDummy *dummy = &world->dummies[i];
        if (!dummy->active) continue;
        float squash = dummy->hp <= 0.0f ? 0.35f : 1.0f + 0.04f*sinf(world->globalTime*5.0f + (float)i);
        Color tint = dummy->hitFlash > 0.0f ? WHITE : (dummy->hp <= 0.0f ? Fade(GRAY, 0.45f) : WHITE);
        DrawEllipse((int)dummy->position.x, (int)(dummy->position.y + 20.0f), 20.0f, 7.0f, Fade(BLACK, 0.36f));
        DemoDrawTextureCentered(DemoDummyTexture(assets, dummy->textureKind), dummy->position,
                                40.0f, 40.0f*squash, 0.0f, tint);
        if (dummy->hp > 0.0f)
        {
            float ratio = dummy->hp/dummy->maxHp;
            DrawRectangle((int)dummy->position.x - 20, (int)dummy->position.y - 29, 40, 4, (Color){ 9, 12, 19, 220 });
            DrawRectangle((int)dummy->position.x - 19, (int)dummy->position.y - 28, (int)(38.0f*ratio), 2,
                          (Color){ 255, 101, 116, 255 });
        }
    }
}

static void DemoDrawActorTrail(const DemoWorld *world, DemoRenderMode mode)
{
    if (world->scene != 3 && world->scene != 4) return;
    Color color = world->scene == 3 ? DemoVisualColor(DEMO_VIS_CALLIGRAPHY_INK)
                                    : DemoVisualColor(DEMO_VIS_GLASS_PRISM);
    for (int i = world->actorTrailCount - 1; i > 0; i--)
    {
        float alpha = 1.0f - (float)i/(float)world->actorTrailCount;
        DrawLineEx(world->actorTrail[i], world->actorTrail[i - 1],
                   mode == DEMO_RENDER_PIXEL ? 3.0f : 4.0f + alpha*4.0f,
                   Fade(color, 0.08f + alpha*0.28f));
        if (world->scene == 3 && (i % 3) == 0)
            DrawCircleV(world->actorTrail[i], mode == DEMO_RENDER_PIXEL ? 3.0f : 5.0f,
                        Fade(color, 0.28f));
    }
}

static void DemoDrawEnemyActor(const DemoAssets *assets, const DemoWorld *world, DemoRenderMode mode)
{
    float bob = sinf(world->globalTime*4.2f)*3.0f;
    Vector2 position = { world->actorPosition.x, world->actorPosition.y + bob };
    Color tint = world->actorHitFlash > 0.0f ? WHITE : (Color){ 248, 248, 255, 255 };
    DrawEllipse((int)position.x, (int)(position.y + 28.0f), 38.0f, 11.0f, Fade(BLACK, 0.42f));

    if (world->scene == 0 || world->scene == 5)
    {
        if (mode != DEMO_RENDER_PIXEL)
        {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient(position, 75.0f,
                               Fade(DemoVisualColor(DEMO_VIS_VIOLET_CUT), 0.13f), BLANK);
            EndBlendMode();
        }
        DemoDrawTextureCentered(assets->spider, position, 100.0f, 62.0f, 0.0f, tint);
    }
    else if (world->scene == 3)
    {
        DrawCircleV(position, 28.0f, (Color){ 38, 70, 75, 255 });
        DrawRing(position, 7.0f, 11.0f, 30.0f, 330.0f, 28, DemoVisualColor(DEMO_VIS_CALLIGRAPHY_INK));
        DemoDrawTextureCentered(assets->gelatine, position, 42.0f, 42.0f, 0.0f, tint);
        DrawLineBezier((Vector2){ position.x - 17, position.y + 14 },
                       (Vector2){ position.x - 34, position.y + 25 }, 4.0f,
                       DemoVisualColor(DEMO_VIS_CALLIGRAPHY_INK));
    }
    else if (world->scene == 4)
    {
        Color glass = Fade(DemoVisualColor(DEMO_VIS_GLASS_PRISM), 0.62f);
        DrawTriangle((Vector2){ position.x - 6, position.y }, (Vector2){ position.x - 61, position.y - 28 },
                     (Vector2){ position.x - 48, position.y + 34 }, glass);
        DrawTriangle((Vector2){ position.x + 6, position.y }, (Vector2){ position.x + 61, position.y - 28 },
                     (Vector2){ position.x + 48, position.y + 34 }, glass);
        DrawTriangleLines((Vector2){ position.x - 6, position.y }, (Vector2){ position.x - 61, position.y - 28 },
                          (Vector2){ position.x - 48, position.y + 34 }, WHITE);
        DrawTriangleLines((Vector2){ position.x + 6, position.y }, (Vector2){ position.x + 61, position.y - 28 },
                          (Vector2){ position.x + 48, position.y + 34 }, WHITE);
        DemoDrawTextureCentered(assets->stareyes, position, 54.0f, 54.0f, 0.0f, tint);
    }
}

static const DemoArcEffect *DemoFindActiveMelee(const DemoWorld *world)
{
    for (int i = 0; i < DEMO_MAX_ARCS; i++)
        if (world->arcs[i].active && world->arcs[i].melee) return &world->arcs[i];
    return NULL;
}

static void DemoDrawPlayerWeapon(const DemoAssets *assets, const DemoWorld *world, DemoRenderMode mode)
{
    if (world->scene == 1)
    {
        const DemoArcEffect *melee = DemoFindActiveMelee(world);
        float angle = world->aimAngle;
        if (melee != NULL)
        {
            float progress = 1.0f - melee->life/fmaxf(melee->totalLife, 0.001f);
            angle = melee->angle - melee->sweep*0.5f + DemoEaseOutCubic(progress)*melee->sweep;
        }
        Vector2 hand = DemoAddScaled(world->playerPosition, DemoDirection(angle), 6.0f);
        Vector2 tip = DemoAddScaled(hand, DemoDirection(angle), 86.0f);
        Vector2 side = { -sinf(angle), cosf(angle) };
        DrawLineEx(hand, tip, mode == DEMO_RENDER_PIXEL ? 5.0f : 6.0f, (Color){ 126, 92, 61, 255 });
        DrawLineEx(DemoAddScaled(tip, DemoDirection(angle), -18.0f), tip, 3.0f, (Color){ 222, 228, 236, 255 });
        DrawTriangle(DemoAddScaled(tip, DemoDirection(angle), 18.0f),
                     DemoAddScaled(tip, side, 14.0f), DemoAddScaled(tip, side, -14.0f),
                     DemoVisualColor(DEMO_VIS_GRAVITY));
        DrawCircleV(hand, 5.0f, DemoVisualColor(DEMO_VIS_GRAVITY));
    }
    else if (world->scene == 2)
    {
        float degrees = world->aimAngle*RAD2DEG;
        Vector2 gunPosition = DemoAddScaled(world->playerPosition, DemoDirection(world->aimAngle), 24.0f);
        DemoDrawTextureCentered(assets->handgun, gunPosition, 58.0f, 58.0f, degrees, (Color){ 198, 214, 222, 255 });
        Color tentacle = DemoVisualColor(DEMO_VIS_RELOAD_ORBIT);
        for (int i = 0; i < 3; i++)
        {
            float offset = ((float)i - 1.0f)*0.42f;
            Vector2 end = DemoAddScaled(world->playerPosition,
                                        DemoDirection(world->aimAngle + DEMO_PI + offset), 32.0f);
            DrawLineBezier(world->playerPosition, end, mode == DEMO_RENDER_PIXEL ? 3.0f : 5.0f,
                           Fade(tentacle, 0.74f));
        }
    }
}

static void DemoDrawPlayer(const DemoAssets *assets, const DemoWorld *world, DemoRenderMode mode)
{
    Vector2 position = world->playerPosition;
    float bob = sinf(world->globalTime*8.0f)*1.5f;
    Color tint = (world->playerInvulnerability > 0.0f && fmodf(world->globalTime, 0.10f) < 0.05f)
                     ? Fade(WHITE, 0.35f) : WHITE;
    DrawEllipse((int)position.x, (int)(position.y + 19.0f), 17.0f, 6.0f, Fade(BLACK, 0.44f));
    DemoDrawPlayerWeapon(assets, world, mode);
    DemoDrawTextureCentered(assets->player, (Vector2){ position.x, position.y + bob },
                            34.0f, 34.0f, 0.0f, tint);

    if (world->playerStatusTime > 0.0f)
        DrawRingLines(position, 18.0f, 21.0f, world->globalTime*90.0f,
                      world->globalTime*90.0f + 250.0f, 24,
                      Fade(DemoVisualColor(world->scene == 4 ? DEMO_VIS_GLASS_PRISM : DEMO_VIS_CALLIGRAPHY_INK), 0.68f));
}

static void DemoDrawHitboxLegend(const DemoWorld *world)
{
    DrawCircleLines((int)world->playerPosition.x, (int)world->playerPosition.y, 12.0f,
                    Fade((Color){ 255, 255, 255, 255 }, 0.38f));
    DrawCircleV(world->playerPosition, 2.0f, (Color){ 255, 255, 255, 220 });
}

static void DemoDrawWorldHud(const DemoWorld *world, DemoRenderMode mode)
{
    Color accent = DemoVisualColor(world->scene == 0 || world->scene == 5 ? DEMO_VIS_VIOLET_CUT :
                                   world->scene == 1 ? DEMO_VIS_GRAVITY :
                                   world->scene == 2 ? DEMO_VIS_RELOAD_ORBIT :
                                   world->scene == 3 ? DEMO_VIS_CALLIGRAPHY_INK : DEMO_VIS_GLASS_PRISM);
    DrawRectangle(0, 0, DEMO_WIDTH, 78, (Color){ 7, 10, 18, 255 });
    DrawRectangle(0, 76, DEMO_WIDTH, 2, accent);
    DrawText(TextFormat("%02d/06", world->scene + 1), 24, 18, 18, accent);
    DrawText(DemoSceneTitle(world->scene), 88, 14, 23, (Color){ 239, 244, 252, 255 });
    DrawText(DemoSceneAlphabet(world->scene), 88, 45, 16, (Color){ 151, 166, 190, 255 });

    DrawText("SANDBOX REALE", 1038, 14, 14, (Color){ 118, 255, 178, 255 });
    DrawText(TextFormat("Lua: %s", world->script.ready ? "ON" : "FALLBACK"), 1038, 34, 14,
             world->script.ready ? (Color){ 118, 255, 178, 255 } : (Color){ 255, 104, 116, 255 });
    DrawText(TextFormat("cmd tick: %d", world->lastCommandCount), 1038, 52, 13, (Color){ 151, 166, 190, 255 });

    DrawRectangle(0, 660, DEMO_WIDTH, 60, (Color){ 7, 10, 18, 245 });
    DrawRectangle(22, 676, 156, 12, (Color){ 29, 35, 49, 255 });
    DrawRectangle(24, 678, (int)(152.0f*DemoClamp(world->playerHp/6.0f, 0.0f, 1.0f)), 8,
                  (Color){ 255, 86, 108, 255 });
    DrawText(TextFormat("HP %.1f/6", world->playerHp), 22, 694, 13, (Color){ 206, 216, 232, 255 });
    DrawText(TextFormat("proiettili %d", DemoActiveProjectileCount(world)), 212, 676, 14, (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("catturati %d", world->capturedTotal), 212, 697, 14, (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("echi %d", world->storedEchoes), 372, 676, 14, accent);
    DrawText(TextFormat("hit %d", world->hitsDealt), 372, 697, 14, accent);

    const char *renderName = mode == DEMO_RENDER_PIXEL ? "PIXEL" : mode == DEMO_RENDER_SMOOTH ? "SMOOTH" : "IBRIDO";
    DrawText(TextFormat("RENDER %s", renderName), 1082, 678, 15, accent);
    DrawText("stesse hitbox", 1082, 699, 13, (Color){ 151, 166, 190, 255 });
    DrawText("C valida: handle, quote, clamp, collisioni, corridoio di fuga", 514, 679, 14,
             (Color){ 173, 186, 208, 255 });
    DrawText("Lua sceglie sequenza e parametri; non vede file, shader o puntatori", 514, 700, 13,
             (Color){ 137, 151, 177, 255 });
}

static void DemoDrawWorld(DemoAssets *assets, const DemoWorld *world, DemoRenderMode mode)
{
    DemoSetTextureFiltering(assets, mode);
    DemoDrawArenaBackdrop(world, mode);
    DemoDrawActorTrail(world, mode);

    for (int i = 0; i < DEMO_MAX_ARCS; i++)
        if (world->arcs[i].active && world->arcs[i].telegraph)
            DemoDrawArcEffect(&world->arcs[i], mode, world->globalTime);
    for (int i = 0; i < DEMO_MAX_BEAMS; i++)
        if (world->beams[i].active && world->beams[i].telegraph)
            DemoDrawBeamEffect(&world->beams[i], mode, world->globalTime);

    if (!world->script.playerOwned) DemoDrawEnemyActor(assets, world, mode);
    DemoDrawDummies(assets, world);
    DemoDrawPlayer(assets, world, mode);

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
    DemoDrawWorldHud(world, mode);
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

static void DemoRenderWorldToTarget(DemoAssets *assets, const DemoWorld *world,
                                    DemoRenderMode mode, RenderTexture2D target)
{
    float scale = (float)target.texture.width/(float)DEMO_WIDTH;
    Camera2D camera = { 0 };
    camera.zoom = scale;
    BeginTextureMode(target);
    BeginMode2D(camera);
    DemoDrawWorld(assets, world, mode);
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

static void DemoDrawComparisonFooter(const DemoWorld *world)
{
    Color accent = DemoVisualColor(DEMO_VIS_GLASS_PRISM);
    DrawText("PIXEL PURO", 34, 46, 19, (Color){ 255, 193, 91, 255 });
    DrawText("IBRIDO: CORE PIXEL + FX SMOOTH", 674, 46, 19, accent);
    DrawRectangleLinesEx((Rectangle){ 14, 76, 616, 347 }, 2.0f, (Color){ 255, 193, 91, 255 });
    DrawRectangleLinesEx((Rectangle){ 650, 76, 616, 347 }, 2.0f, accent);

    DrawText("IDENTICO", 24, 456, 14, (Color){ 118, 255, 178, 255 });
    DrawText("fixed tick, seed, posizioni, collisioni, danno, telegraph e spazio sicuro", 112, 454, 17,
             (Color){ 222, 230, 242, 255 });
    DrawText("PIXEL", 24, 500, 14, (Color){ 255, 193, 91, 255 });
    DrawText("render target 640x360, filtro POINT, palette ridotta, forme a segmenti", 112, 498, 17,
             (Color){ 179, 191, 212, 255 });
    DrawText("IBRIDO", 24, 544, 14, accent);
    DrawText("sprite/core nitidi; scie, glow e color grading in GLSL 330", 112, 542, 17,
             (Color){ 179, 191, 212, 255 });
    DrawText("PER IL GIOCO", 24, 588, 14, (Color){ 212, 153, 255, 255 });
    DrawText("ibrido raccomandato, preset Pixel come fallback e Reduced FX", 148, 586, 17,
             (Color){ 229, 232, 244, 255 });

    DrawRectangle(24, 626, 1232, 62, (Color){ 15, 21, 34, 255 });
    DrawText(TextFormat("Gameplay osservabile: HP %.1f | proiettili %d | hit %d | comandi ultimo tick %d",
                        world->playerHp, DemoActiveProjectileCount(world), world->hitsDealt,
                        world->lastCommandCount),
             42, 642, 17, (Color){ 208, 220, 238, 255 });
    DrawText("Lo shader non decide mai traiettoria o collisione.", 42, 666, 14,
             (Color){ 143, 159, 185, 255 });
}

static void DemoComposeFrame(DemoRenderer *renderer, DemoAssets *assets, const DemoWorld *world,
                             DemoRenderMode mode, bool split, bool showControls)
{
    split = split || world->scene == 5;
    if (split || mode == DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(assets, world, DEMO_RENDER_PIXEL, renderer->pixelTarget);
    if (split || mode != DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(assets, world, split ? DEMO_RENDER_HYBRID : mode, renderer->highTarget);

    BeginTextureMode(renderer->finalTarget);
    ClearBackground((Color){ 5, 8, 15, 255 });
    if (split)
    {
        DrawText(DemoSceneTitle(5), 24, 14, 23, (Color){ 239, 244, 252, 255 });
        DemoDrawTarget(renderer, renderer->pixelTarget.texture, (Rectangle){ 14, 76, 616, 347 },
                       DEMO_RENDER_PIXEL, world->globalTime);
        DemoDrawTarget(renderer, renderer->highTarget.texture, (Rectangle){ 650, 76, 616, 347 },
                       DEMO_RENDER_HYBRID, world->globalTime);
        DemoDrawComparisonFooter(world);
    }
    else
    {
        Texture2D source = mode == DEMO_RENDER_PIXEL ? renderer->pixelTarget.texture : renderer->highTarget.texture;
        DemoDrawTarget(renderer, source, (Rectangle){ 0, 0, DEMO_WIDTH, DEMO_HEIGHT }, mode, world->globalTime);
    }

    if (showControls)
    {
        DrawRectangle(0, DEMO_HEIGHT - 20, DEMO_WIDTH, 20, (Color){ 3, 5, 10, 232 });
        DrawText("F1-F6 scena | F7 auto | 1 pixel  2 smooth  3 ibrido | TAB A/B | WASD muovi | SPAZIO pausa | R reset",
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
    static const DemoRenderMode captureModes[DEMO_SCENE_COUNT] = {
        DEMO_RENDER_HYBRID,
        DEMO_RENDER_HYBRID,
        DEMO_RENDER_SMOOTH,
        DEMO_RENDER_PIXEL,
        DEMO_RENDER_HYBRID,
        DEMO_RENDER_HYBRID
    };
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
    DemoResetScene(&world, 0);

    if (capture)
    {
        if (!DirectoryExists(captureDirectory) && MakeDirectory(captureDirectory) != 0)
        {
            fprintf(stderr, "ERRORE: impossibile creare la cartella frame: %s\n", captureDirectory);
            DemoScriptUnload(&world.script);
            DemoRendererUnload(&renderer);
            DemoUnloadAssets(&assets);
            CloseWindow();
            return 4;
        }

        int currentScene = -1;
        const int framesPerScene = DEMO_CAPTURE_FRAMES/DEMO_SCENE_COUNT;
        for (int frame = 0; frame < DEMO_CAPTURE_FRAMES; frame++)
        {
            int scene = frame/framesPerScene;
            if (scene != currentScene)
            {
                DemoResetScene(&world, scene);
                currentScene = scene;
            }
            for (int step = 0; step < 4; step++) DemoUpdateWorld(&world, DEMO_FIXED_DT, false);
            DemoComposeFrame(&renderer, &assets, &world, captureModes[scene], scene == 5, false);
            if (!DemoExportFrame(renderer.finalTarget, captureDirectory, frame))
            {
                fprintf(stderr, "ERRORE: export fallito al frame %d.\n", frame);
                DemoScriptUnload(&world.script);
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
        DemoRenderMode mode = DEMO_RENDER_HYBRID;
        bool split = false;
        bool paused = false;
        bool automatic = true;
        float accumulator = 0.0f;
        while (!WindowShouldClose())
        {
            float frameTime = fminf(GetFrameTime(), 0.10f);
            if (IsKeyPressed(KEY_ONE)) mode = DEMO_RENDER_PIXEL;
            if (IsKeyPressed(KEY_TWO)) mode = DEMO_RENDER_SMOOTH;
            if (IsKeyPressed(KEY_THREE)) mode = DEMO_RENDER_HYBRID;
            if (IsKeyPressed(KEY_TAB)) split = !split;
            if (IsKeyPressed(KEY_SPACE)) paused = !paused;
            if (IsKeyPressed(KEY_F7)) automatic = true;
            if (IsKeyPressed(KEY_R)) DemoResetScene(&world, world.scene);

            int requestedScene = -1;
            if (IsKeyPressed(KEY_F1)) requestedScene = 0;
            if (IsKeyPressed(KEY_F2)) requestedScene = 1;
            if (IsKeyPressed(KEY_F3)) requestedScene = 2;
            if (IsKeyPressed(KEY_F4)) requestedScene = 3;
            if (IsKeyPressed(KEY_F5)) requestedScene = 4;
            if (IsKeyPressed(KEY_F6)) requestedScene = 5;
            if (requestedScene >= 0)
            {
                automatic = false;
                DemoResetScene(&world, requestedScene);
            }

            if (!paused) accumulator += frameTime;
            while (accumulator >= DEMO_FIXED_DT)
            {
                if (automatic && world.sceneTime >= DEMO_SCENE_SECONDS)
                    DemoResetScene(&world, (world.scene + 1)%DEMO_SCENE_COUNT);
                DemoUpdateWorld(&world, DEMO_FIXED_DT, true);
                accumulator -= DEMO_FIXED_DT;
            }

            DemoComposeFrame(&renderer, &assets, &world, mode, split, true);
            BeginDrawing();
            ClearBackground(BLACK);
            DemoDrawFinalToWindow(renderer.finalTarget);
            EndDrawing();
        }
    }

    DemoScriptUnload(&world.script);
    DemoRendererUnload(&renderer);
    DemoUnloadAssets(&assets);
    CloseWindow();
    return 0;
}
