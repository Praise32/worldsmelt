#include "demo_script_api.h"

#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <math.h>
#include <string.h>

#define DEMO_PI       3.14159265358979323846f
#define DEMO_TWO_PI   (2.0f*DEMO_PI)
#define DEMO_HANDLE_MAX_EXCLUSIVE (1ull << 53)

#define DEMO_POS_DEFAULT_RIGHT  960.0f
#define DEMO_POS_DEFAULT_BOTTOM 640.0f

#define DEMO_RADIUS_MIN       4.0f
#define DEMO_RADIUS_MAX       320.0f
#define DEMO_PROJECTILE_MIN   1.0f
#define DEMO_PROJECTILE_MAX   40.0f
#define DEMO_WIDTH_MIN        1.0f
#define DEMO_WIDTH_MAX        120.0f
#define DEMO_LENGTH_MIN       8.0f
#define DEMO_LENGTH_MAX       900.0f
#define DEMO_SWEEP_MIN        0.05f
#define DEMO_DURATION_MIN     0.016f
#define DEMO_DURATION_MAX     6.0f
#define DEMO_LIFE_MIN         0.05f
#define DEMO_LIFE_MAX         10.0f
#define DEMO_SPEED_MAX        1000.0f
#define DEMO_VELOCITY_MAX     650.0f
#define DEMO_ANGULAR_MAX      12.0f
#define DEMO_DAMAGE_MAX       120.0f
#define DEMO_STRENGTH_MAX     4.0f
#define DEMO_COUNT_MAX        24

static float DemoClamp(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static float DemoNormalizeAngle(float angle)
{
    angle = fmodf(angle, DEMO_TWO_PI);
    if (angle > DEMO_PI) angle -= DEMO_TWO_PI;
    if (angle < -DEMO_PI) angle += DEMO_TWO_PI;
    return angle;
}

static bool DemoHandleConfigValid(uint64_t handle)
{
    return handle > 0 && handle < DEMO_HANDLE_MAX_EXCLUSIVE;
}

bool DemoScriptApiVisualIdValid(int visualId)
{
    return visualId > DEMO_VIS_INVALID && visualId < DEMO_VIS_COUNT;
}

bool DemoScriptApiStatusIdValid(int statusId)
{
    return statusId > DEMO_STATUS_INVALID && statusId < DEMO_STATUS_COUNT;
}

void DemoScriptApiInit(DemoScriptApiState *api,
                       uint64_t selfHandle, uint64_t playerHandle,
                       float roomLeft, float roomTop,
                       float roomRight, float roomBottom)
{
    if (api == NULL) return;
    memset(api, 0, sizeof *api);

    api->selfHandle = DemoHandleConfigValid(selfHandle) ? selfHandle : 1u;
    api->playerHandle = DemoHandleConfigValid(playerHandle) ? playerHandle : 2u;
    if (api->playerHandle == api->selfHandle)
        api->playerHandle = (api->selfHandle == 2u) ? 3u : 2u;

    if (!isfinite(roomLeft) || !isfinite(roomTop) || !isfinite(roomRight) ||
        !isfinite(roomBottom) || roomRight <= roomLeft || roomBottom <= roomTop)
    {
        roomLeft = 0.0f;
        roomTop = 0.0f;
        roomRight = DEMO_POS_DEFAULT_RIGHT;
        roomBottom = DEMO_POS_DEFAULT_BOTTOM;
    }
    api->roomLeft = roomLeft;
    api->roomTop = roomTop;
    api->roomRight = roomRight;
    api->roomBottom = roomBottom;
}

void DemoScriptApiBeginFrame(DemoScriptApiState *api,
                             float playerX, float playerY,
                             float selfX, float selfY,
                             float currentAimAngle)
{
    if (api == NULL) return;
    api->playerX = isfinite(playerX) ? DemoClamp(playerX, api->roomLeft, api->roomRight) : api->roomLeft;
    api->playerY = isfinite(playerY) ? DemoClamp(playerY, api->roomTop, api->roomBottom) : api->roomTop;
    api->selfX = isfinite(selfX) ? DemoClamp(selfX, api->roomLeft, api->roomRight) : api->roomLeft;
    api->selfY = isfinite(selfY) ? DemoClamp(selfY, api->roomTop, api->roomBottom) : api->roomTop;
    api->currentAimAngle = isfinite(currentAimAngle) ? DemoNormalizeAngle(currentAimAngle) : 0.0f;
    api->commandCount = 0;
    api->gameplayCommandCount = 0;
    api->visualCommandCount = 0;
}

const DemoScriptCommand *DemoScriptApiCommands(const DemoScriptApiState *api)
{
    return (api != NULL) ? api->commands : NULL;
}

size_t DemoScriptApiCommandCount(const DemoScriptApiState *api)
{
    return (api != NULL) ? api->commandCount : 0;
}

static DemoScriptApiState *DemoApi(lua_State *L)
{
    DemoScriptApiState *api = (DemoScriptApiState *)lua_touserdata(L, lua_upvalueindex(1));
    if (api == NULL) luaL_error(L, "demo API state assente");
    return api;
}

static float DemoCheckFinite(lua_State *L, int arg)
{
    lua_Number value = luaL_checknumber(L, arg);
    if (!isfinite((double)value)) luaL_error(L, "argomento %d non finito", arg);
    return (float)value;
}

static int DemoCheckInteger(lua_State *L, int arg, int lo, int hi)
{
    lua_Number value = luaL_checknumber(L, arg);
    if (!isfinite((double)value) || floor((double)value) != (double)value)
        luaL_error(L, "argomento %d deve essere intero", arg);
    if (value < (lua_Number)lo) return lo;
    if (value > (lua_Number)hi) return hi;
    return (int)value;
}

static uint64_t DemoCheckHandle(lua_State *L, int arg)
{
    lua_Number value = luaL_checknumber(L, arg);
    if (!isfinite((double)value) || value < 1.0 ||
        value >= (lua_Number)DEMO_HANDLE_MAX_EXCLUSIVE ||
        floor((double)value) != (double)value)
    {
        luaL_error(L, "handle non valido");
    }
    return (uint64_t)value;
}

static uint64_t DemoCheckSelfHandle(lua_State *L, DemoScriptApiState *api, int arg)
{
    uint64_t handle = DemoCheckHandle(L, arg);
    if (handle != api->selfHandle) luaL_error(L, "self_handle non valido");
    return handle;
}

static int DemoCheckVisualId(lua_State *L, int arg)
{
    int visualId = DemoCheckInteger(L, arg, DEMO_VIS_INVALID, DEMO_VIS_COUNT);
    if (!DemoScriptApiVisualIdValid(visualId)) luaL_error(L, "visual_id non ammesso");
    return visualId;
}

static int DemoCheckStatusId(lua_State *L, int arg)
{
    int statusId = DemoCheckInteger(L, arg, DEMO_STATUS_INVALID, DEMO_STATUS_COUNT);
    if (!DemoScriptApiStatusIdValid(statusId)) luaL_error(L, "status_id non ammesso");
    return statusId;
}

static float DemoX(lua_State *L, DemoScriptApiState *api, int arg)
{
    return DemoClamp(DemoCheckFinite(L, arg), api->roomLeft, api->roomRight);
}

static float DemoY(lua_State *L, DemoScriptApiState *api, int arg)
{
    return DemoClamp(DemoCheckFinite(L, arg), api->roomTop, api->roomBottom);
}

static bool DemoAppend(DemoScriptApiState *api, const DemoScriptCommand *command,
                       bool gameplay, bool visual)
{
    if (api->commandCount >= DEMO_SCRIPT_MAX_COMMANDS) return false;
    if (gameplay && api->gameplayCommandCount >= DEMO_SCRIPT_MAX_GAMEPLAY_COMMANDS) return false;
    if (visual && api->visualCommandCount >= DEMO_SCRIPT_MAX_VISUAL_COMMANDS) return false;
    api->commands[api->commandCount++] = *command;
    if (gameplay) api->gameplayCommandCount++;
    if (visual) api->visualCommandCount++;
    return true;
}

static int DemoReturnAccepted(lua_State *L, bool accepted)
{
    lua_pushboolean(L, accepted);
    return 1;
}

static int DemoPlayerX(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)DemoApi(L)->playerX);
    return 1;
}

static int DemoPlayerY(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)DemoApi(L)->playerY);
    return 1;
}

static int DemoSelfX(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)DemoApi(L)->selfX);
    return 1;
}

static int DemoSelfY(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)DemoApi(L)->selfY);
    return 1;
}

static int DemoAimAtPlayer(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    float dx = api->playerX - api->selfX;
    float dy = api->playerY - api->selfY;
    float angle = (fabsf(dx) + fabsf(dy) > 0.0001f) ? atan2f(dy, dx) : api->currentAimAngle;
    lua_pushnumber(L, (lua_Number)DemoNormalizeAngle(angle));
    return 1;
}

static int DemoAimSnapshot(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    api->lastAimSnapshot = api->currentAimAngle;
    api->hasAimSnapshot = true;
    lua_pushnumber(L, (lua_Number)api->lastAimSnapshot);
    return 1;
}

static int DemoTelegraphArc(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_TELEGRAPH_ARC;
    c.sourceHandle = api->selfHandle;
    c.x = DemoX(L, api, 1);
    c.y = DemoY(L, api, 2);
    c.angle = DemoNormalizeAngle(DemoCheckFinite(L, 3));
    c.radius = DemoClamp(DemoCheckFinite(L, 4), DEMO_RADIUS_MIN, DEMO_RADIUS_MAX);
    c.width = DemoClamp(DemoCheckFinite(L, 5), DEMO_WIDTH_MIN, DEMO_WIDTH_MAX);
    c.sweep = DemoClamp(fabsf(DemoCheckFinite(L, 6)), DEMO_SWEEP_MIN, DEMO_TWO_PI);
    c.duration = DemoClamp(DemoCheckFinite(L, 7), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    c.visualId = DemoCheckVisualId(L, 8);
    return DemoReturnAccepted(L, DemoAppend(api, &c, false, true));
}

static int DemoEmitArcLike(lua_State *L, DemoScriptCommandType type)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = type;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.angle = DemoNormalizeAngle(DemoCheckFinite(L, 4));
    c.radius = DemoClamp(DemoCheckFinite(L, 5), DEMO_RADIUS_MIN, DEMO_RADIUS_MAX);
    c.width = DemoClamp(DemoCheckFinite(L, 6), DEMO_WIDTH_MIN, DEMO_WIDTH_MAX);
    c.sweep = DemoClamp(fabsf(DemoCheckFinite(L, 7)), DEMO_SWEEP_MIN, DEMO_TWO_PI);
    c.damage = DemoClamp(DemoCheckFinite(L, 8), 0.0f, DEMO_DAMAGE_MAX);
    c.duration = DemoClamp(DemoCheckFinite(L, 9), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    c.visualId = DemoCheckVisualId(L, 10);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static int DemoEmitArc(lua_State *L)
{
    return DemoEmitArcLike(L, DEMO_CMD_EMIT_ARC);
}

static int DemoMeleeSweep(lua_State *L)
{
    return DemoEmitArcLike(L, DEMO_CMD_MELEE_SWEEP);
}

static int DemoEmitRing(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_EMIT_RING;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.count = DemoCheckInteger(L, 4, 1, DEMO_COUNT_MAX);
    c.speed = DemoClamp(DemoCheckFinite(L, 5), 0.0f, DEMO_SPEED_MAX);
    c.damage = DemoClamp(DemoCheckFinite(L, 6), 0.0f, DEMO_DAMAGE_MAX);
    c.projectileRadius = DemoClamp(DemoCheckFinite(L, 7), DEMO_PROJECTILE_MIN, DEMO_PROJECTILE_MAX);
    c.life = DemoClamp(DemoCheckFinite(L, 8), DEMO_LIFE_MIN, DEMO_LIFE_MAX);
    c.visualId = DemoCheckVisualId(L, 9);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static int DemoEmitOrbit(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_EMIT_ORBIT;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.count = DemoCheckInteger(L, 4, 1, DEMO_COUNT_MAX);
    c.radius = DemoClamp(DemoCheckFinite(L, 5), DEMO_RADIUS_MIN, DEMO_RADIUS_MAX);
    c.angularSpeed = DemoClamp(DemoCheckFinite(L, 6), -DEMO_ANGULAR_MAX, DEMO_ANGULAR_MAX);
    c.damage = DemoClamp(DemoCheckFinite(L, 7), 0.0f, DEMO_DAMAGE_MAX);
    c.projectileRadius = DemoClamp(DemoCheckFinite(L, 8), DEMO_PROJECTILE_MIN, DEMO_PROJECTILE_MAX);
    c.life = DemoClamp(DemoCheckFinite(L, 9), DEMO_LIFE_MIN, DEMO_LIFE_MAX);
    c.visualId = DemoCheckVisualId(L, 10);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static int DemoTelegraphBeam(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_TELEGRAPH_BEAM;
    c.sourceHandle = api->selfHandle;
    c.x = DemoX(L, api, 1);
    c.y = DemoY(L, api, 2);
    c.angle = DemoNormalizeAngle(DemoCheckFinite(L, 3));
    c.length = DemoClamp(DemoCheckFinite(L, 4), DEMO_LENGTH_MIN, DEMO_LENGTH_MAX);
    c.width = DemoClamp(DemoCheckFinite(L, 5), DEMO_WIDTH_MIN, DEMO_WIDTH_MAX);
    c.duration = DemoClamp(DemoCheckFinite(L, 6), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    c.visualId = DemoCheckVisualId(L, 7);
    return DemoReturnAccepted(L, DemoAppend(api, &c, false, true));
}

static int DemoEmitBeam(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_EMIT_BEAM;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.angle = DemoNormalizeAngle(DemoCheckFinite(L, 4));
    c.length = DemoClamp(DemoCheckFinite(L, 5), DEMO_LENGTH_MIN, DEMO_LENGTH_MAX);
    c.width = DemoClamp(DemoCheckFinite(L, 6), DEMO_WIDTH_MIN, DEMO_WIDTH_MAX);
    c.damage = DemoClamp(DemoCheckFinite(L, 7), 0.0f, DEMO_DAMAGE_MAX);
    c.duration = DemoClamp(DemoCheckFinite(L, 8), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    c.visualId = DemoCheckVisualId(L, 9);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static int DemoSetVelocity(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_SET_VELOCITY;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.vx = DemoCheckFinite(L, 2);
    c.vy = DemoCheckFinite(L, 3);
    float len = sqrtf(c.vx*c.vx + c.vy*c.vy);
    if (len > DEMO_VELOCITY_MAX && len > 0.0001f)
    {
        float scale = DEMO_VELOCITY_MAX/len;
        c.vx *= scale;
        c.vy *= scale;
    }
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, false));
}

static int DemoAddStatus(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_ADD_STATUS;
    c.sourceHandle = api->selfHandle;
    c.targetHandle = DemoCheckHandle(L, 1);
    if (c.targetHandle != api->selfHandle && c.targetHandle != api->playerHandle)
        luaL_error(L, "target_handle non valido");
    c.statusId = DemoCheckStatusId(L, 2);
    c.strength = DemoClamp(DemoCheckFinite(L, 3), 0.0f, DEMO_STRENGTH_MAX);
    c.duration = DemoClamp(DemoCheckFinite(L, 4), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, false));
}

static int DemoCaptureRadius(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_CAPTURE_RADIUS;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.radius = DemoClamp(DemoCheckFinite(L, 4), DEMO_RADIUS_MIN, DEMO_RADIUS_MAX);
    c.strength = DemoClamp(DemoCheckFinite(L, 5), 0.0f, DEMO_STRENGTH_MAX);
    c.count = DemoCheckInteger(L, 6, 1, DEMO_COUNT_MAX);
    c.duration = DemoClamp(DemoCheckFinite(L, 7), DEMO_DURATION_MIN, DEMO_DURATION_MAX);
    c.visualId = DemoCheckVisualId(L, 8);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static int DemoReleaseEchoes(lua_State *L)
{
    DemoScriptApiState *api = DemoApi(L);
    DemoScriptCommand c = { 0 };
    c.type = DEMO_CMD_RELEASE_ECHOES;
    c.sourceHandle = DemoCheckSelfHandle(L, api, 1);
    c.x = DemoX(L, api, 2);
    c.y = DemoY(L, api, 3);
    c.angle = DemoNormalizeAngle(DemoCheckFinite(L, 4));
    c.count = DemoCheckInteger(L, 5, 1, DEMO_COUNT_MAX);
    c.speed = DemoClamp(DemoCheckFinite(L, 6), 0.0f, DEMO_SPEED_MAX);
    c.damage = DemoClamp(DemoCheckFinite(L, 7), 0.0f, DEMO_DAMAGE_MAX);
    c.spread = DemoClamp(fabsf(DemoCheckFinite(L, 8)), 0.0f, DEMO_TWO_PI);
    c.life = DemoClamp(DemoCheckFinite(L, 9), DEMO_LIFE_MIN, DEMO_LIFE_MAX);
    c.visualId = DemoCheckVisualId(L, 10);
    return DemoReturnAccepted(L, DemoAppend(api, &c, true, true));
}

static void DemoRegisterFn(lua_State *L, DemoScriptApiState *api,
                           const char *name, lua_CFunction fn)
{
    /* Il lightuserdata e' un upvalue inaccessibile allo script: debug,
     * getupvalue e la libreria debug non esistono nella ScriptSandbox. */
    lua_pushlightuserdata(L, api);
    lua_pushcclosure(L, fn, 1);
    lua_setfield(L, -2, name);
}

static void DemoRegisterNumber(lua_State *L, const char *name, lua_Number value)
{
    lua_pushnumber(L, value);
    lua_setfield(L, -2, name);
}

bool DemoScriptApiRegister(struct ScriptSandbox *sandbox, DemoScriptApiState *api)
{
    if (sandbox == NULL || api == NULL) return false;
    lua_State *L = ScriptSandboxRawState((ScriptSandbox *)sandbox);
    if (L == NULL) return false;

    lua_pushglobaltable(L);
    DemoRegisterFn(L, api, "player_x", DemoPlayerX);
    DemoRegisterFn(L, api, "player_y", DemoPlayerY);
    DemoRegisterFn(L, api, "self_x", DemoSelfX);
    DemoRegisterFn(L, api, "self_y", DemoSelfY);
    DemoRegisterFn(L, api, "aim_at_player", DemoAimAtPlayer);
    DemoRegisterFn(L, api, "aim_snapshot", DemoAimSnapshot);
    DemoRegisterFn(L, api, "telegraph_arc", DemoTelegraphArc);
    DemoRegisterFn(L, api, "emit_arc", DemoEmitArc);
    DemoRegisterFn(L, api, "emit_ring", DemoEmitRing);
    DemoRegisterFn(L, api, "emit_orbit", DemoEmitOrbit);
    DemoRegisterFn(L, api, "telegraph_beam", DemoTelegraphBeam);
    DemoRegisterFn(L, api, "emit_beam", DemoEmitBeam);
    DemoRegisterFn(L, api, "set_velocity", DemoSetVelocity);
    DemoRegisterFn(L, api, "add_status", DemoAddStatus);
    DemoRegisterFn(L, api, "melee_sweep", DemoMeleeSweep);
    DemoRegisterFn(L, api, "capture_radius", DemoCaptureRadius);
    DemoRegisterFn(L, api, "release_echoes", DemoReleaseEchoes);

    DemoRegisterNumber(L, "SELF_HANDLE", (lua_Number)api->selfHandle);
    DemoRegisterNumber(L, "PLAYER_HANDLE", (lua_Number)api->playerHandle);
    DemoRegisterNumber(L, "DEMO_API_VERSION", (lua_Number)DEMO_SCRIPT_API_VERSION);

    DemoRegisterNumber(L, "VIS_VIOLET_CUT", DEMO_VIS_VIOLET_CUT);
    DemoRegisterNumber(L, "VIS_CALLIGRAPHY_INK", DEMO_VIS_CALLIGRAPHY_INK);
    DemoRegisterNumber(L, "VIS_GLASS_PRISM", DEMO_VIS_GLASS_PRISM);
    DemoRegisterNumber(L, "VIS_GRAVITY", DEMO_VIS_GRAVITY);
    DemoRegisterNumber(L, "VIS_VOID_ECHO", DEMO_VIS_VOID_ECHO);
    DemoRegisterNumber(L, "VIS_RELOAD_ORBIT", DEMO_VIS_RELOAD_ORBIT);

    DemoRegisterNumber(L, "STATUS_SLOW", DEMO_STATUS_SLOW);
    DemoRegisterNumber(L, "STATUS_INKED", DEMO_STATUS_INKED);
    DemoRegisterNumber(L, "STATUS_GLASS_MARK", DEMO_STATUS_GLASS_MARK);
    DemoRegisterNumber(L, "STATUS_GRAVITY", DEMO_STATUS_GRAVITY);

    lua_pop(L, 1);
    return true;
}
