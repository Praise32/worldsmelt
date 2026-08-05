#ifndef WORLDSMELT_PROCEDURAL_COMBAT_DEMO_SCRIPT_API_H
#define WORLDSMELT_PROCEDURAL_COMBAT_DEMO_SCRIPT_API_H

/*
 * API Lua della demo procedurale.
 *
 * Questo non e' un catalogo di armi o classi di nemico. E' un piccolo
 * alfabeto di primitive geometriche e di simulazione: archi, anelli, orbite,
 * raggi, velocita', stati e tre operazioni generiche per una sequenza melee/
 * cattura/rilascio. Lo script decide quando e come combinarle; il C resta
 * autorevole per collisioni, clamp, quote e rendering.
 *
 * La demo deve creare una VERA ScriptSandbox con ScriptSandboxCreate(), poi
 * chiamare DemoScriptApiRegister() PRIMA di ScriptSandboxLoad(). Il puntatore
 * DemoScriptApiState vive soltanto come upvalue di closure C: Lua non riceve
 * mai un puntatore, un Texture2D, uno Shader o un altro oggetto Raylib. La
 * struttura deve quindi restare allo stesso indirizzo fino alla distruzione
 * della sandbox (non copiarla/spostarla dopo la registrazione).
 *
 * Ogni fixed tick:
 *   1. DemoScriptApiBeginFrame(...)
 *   2. callback Lua on_tick(dt, self_handle) sotto
 *      ScriptSandboxProtectedCall/ScriptSandboxCallVoid
 *   3. il motore legge DemoScriptApiCommands() e valida/applica i comandi
 *   4. il renderer disegna lo stato risultante senza richiamare Lua
 *
 * Gli handle sono interi rappresentabili esattamente da un lua_Number
 * (double, < 2^53). Ogni comando che agisce sul proprietario accetta soltanto
 * self_handle; add_status accetta soltanto self_handle o PLAYER_HANDLE.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ScriptSandbox;

#define DEMO_SCRIPT_API_VERSION 1

/* Quote per UNA sandbox e UN fixed tick. Un comando gameplay che possiede
 * anche una rappresentazione visuale consuma entrambe le quote. La saturazione
 * e' normale (la funzione Lua restituisce false), non uccide la sandbox. */
#define DEMO_SCRIPT_MAX_COMMANDS          96
#define DEMO_SCRIPT_MAX_GAMEPLAY_COMMANDS 48
#define DEMO_SCRIPT_MAX_VISUAL_COMMANDS   64

/* ID visuali scelti dal contenuto ma risolti esclusivamente dal renderer C.
 * Nessun percorso file o shader arbitrario attraversa il confine Lua. */
typedef enum DemoVisualId {
    DEMO_VIS_INVALID = 0,
    DEMO_VIS_VIOLET_CUT,
    DEMO_VIS_CALLIGRAPHY_INK,
    DEMO_VIS_GLASS_PRISM,
    DEMO_VIS_GRAVITY,
    DEMO_VIS_VOID_ECHO,
    DEMO_VIS_RELOAD_ORBIT,
    DEMO_VIS_COUNT
} DemoVisualId;

/* Stati ammessi dalla demo. Il motore decide la semantica esatta e applica
 * comunque durata/intensita' clampate. */
typedef enum DemoStatusId {
    DEMO_STATUS_INVALID = 0,
    DEMO_STATUS_SLOW,
    DEMO_STATUS_INKED,
    DEMO_STATUS_GLASS_MARK,
    DEMO_STATUS_GRAVITY,
    DEMO_STATUS_COUNT
} DemoStatusId;

typedef enum DemoScriptCommandType {
    DEMO_CMD_TELEGRAPH_ARC = 0,
    DEMO_CMD_EMIT_ARC,
    DEMO_CMD_EMIT_RING,
    DEMO_CMD_EMIT_ORBIT,
    DEMO_CMD_TELEGRAPH_BEAM,
    DEMO_CMD_EMIT_BEAM,
    DEMO_CMD_SET_VELOCITY,
    DEMO_CMD_ADD_STATUS,
    DEMO_CMD_MELEE_SWEEP,
    DEMO_CMD_CAPTURE_RADIUS,
    DEMO_CMD_RELEASE_ECHOES
} DemoScriptCommandType;

/* Record volutamente piatto: il consumer fa switch(type) e usa soltanto i
 * campi pertinenti. I campi non usati sono sempre zero perche' ogni wrapper
 * parte da un record azzerato.
 *
 * mapping principale:
 * - ARC/MELEE: x,y,angle,radius,width,sweep,damage,duration,visualId
 * - RING: x,y,count,speed,damage,projectileRadius,life,visualId
 * - ORBIT: x,y,count,radius,angularSpeed,damage,projectileRadius,life,visualId
 * - BEAM: x,y,angle,length,width,damage,duration,visualId
 * - VELOCITY: vx,vy
 * - STATUS: targetHandle,statusId,strength,duration
 * - CAPTURE: x,y,radius,strength,count,duration,visualId
 * - ECHOES: x,y,angle,count,speed,damage,spread,life,visualId
 */
typedef struct DemoScriptCommand {
    DemoScriptCommandType type;
    uint64_t sourceHandle;
    uint64_t targetHandle;
    int visualId;
    int statusId;
    int count;
    float x;
    float y;
    float angle;
    float radius;
    float projectileRadius;
    float width;
    float sweep;
    float length;
    float speed;
    float angularSpeed;
    float damage;
    float duration;
    float life;
    float strength;
    float spread;
    float vx;
    float vy;
} DemoScriptCommand;

typedef struct DemoScriptApiState {
    uint64_t selfHandle;
    uint64_t playerHandle;

    float roomLeft;
    float roomTop;
    float roomRight;
    float roomBottom;

    float playerX;
    float playerY;
    float selfX;
    float selfY;
    /* Direzione di mira fornita dall'host. Per un nemico puo' coincidere con
     * aim_at_player(); per un player e' la mira mouse/stick. */
    float currentAimAngle;
    float lastAimSnapshot;
    bool hasAimSnapshot;

    DemoScriptCommand commands[DEMO_SCRIPT_MAX_COMMANDS];
    size_t commandCount;
    size_t gameplayCommandCount;
    size_t visualCommandCount;
} DemoScriptApiState;

/* Inizializza lo stato persistente di una singola sandbox/entita'. I due
 * handle devono essere interi < 2^53 e diversi da zero; in caso contrario
 * vengono sostituiti con valori sicuri (1 per self, 2 per player). */
void DemoScriptApiInit(DemoScriptApiState *api,
                       uint64_t selfHandle, uint64_t playerHandle,
                       float roomLeft, float roomTop,
                       float roomRight, float roomBottom);

/* Aggiorna il contesto letto da Lua e azzera SOLO il command buffer del tick.
 * Lo stato Lua e lastAimSnapshot restano vivi fra i frame. */
void DemoScriptApiBeginFrame(DemoScriptApiState *api,
                             float playerX, float playerY,
                             float selfX, float selfY,
                             float currentAimAngle);

/* Registra nell'_ENV della sandbox le closure seguenti:
 *
 * letture (un solo valore di ritorno):
 *   player_x(), player_y(), self_x(), self_y()
 *   aim_at_player()   -- angolo live self -> player
 *   aim_snapshot()    -- fotografa e ritorna currentAimAngle dell'host
 *
 * comandi (ritornano true se accodati, false se una quota e' piena):
 *   telegraph_arc(x,y,angle,radius,width,sweep,duration,visual_id)
 *   emit_arc(self,x,y,angle,radius,width,sweep,damage,duration,visual_id)
 *   emit_ring(self,x,y,count,speed,damage,shot_radius,life,visual_id)
 *   emit_orbit(self,x,y,count,orbit_radius,angular_speed,damage,shot_radius,life,visual_id)
 *   telegraph_beam(x,y,angle,length,width,duration,visual_id)
 *   emit_beam(self,x,y,angle,length,width,damage,duration,visual_id)
 *   set_velocity(self,vx,vy)
 *   add_status(target,status_id,strength,duration)
 *   melee_sweep(self,x,y,angle,radius,width,sweep,damage,duration,visual_id)
 *   capture_radius(self,x,y,radius,pull_strength,max_targets,duration,visual_id)
 *   release_echoes(self,x,y,angle,count,speed,damage,spread,life,visual_id)
 *
 * Costanti: SELF_HANDLE, PLAYER_HANDLE, VIS_*, STATUS_*, DEMO_API_VERSION.
 */
bool DemoScriptApiRegister(struct ScriptSandbox *sandbox, DemoScriptApiState *api);

const DemoScriptCommand *DemoScriptApiCommands(const DemoScriptApiState *api);
size_t DemoScriptApiCommandCount(const DemoScriptApiState *api);
bool DemoScriptApiVisualIdValid(int visualId);
bool DemoScriptApiStatusIdValid(int statusId);

#endif
