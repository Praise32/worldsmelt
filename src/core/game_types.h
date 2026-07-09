#ifndef MELTING_RUN_GAME_TYPES_H
#define MELTING_RUN_GAME_TYPES_H

#include "raylib.h"

#include <stdbool.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640
#define APP_WINDOW_WIDTH 1600
#define APP_WINDOW_HEIGHT 900

#define HUD_H 82
#define FOOTER_H 38
#define ROOM_X 42.0f
#define ROOM_Y 104.0f
#define ROOM_W 876.0f
#define ROOM_H 458.0f
#define ROOM_RIGHT (ROOM_X + ROOM_W)
#define ROOM_BOTTOM (ROOM_Y + ROOM_H)
#define DOOR_HALF 50.0f

#define FLOOR_COUNT 5
#define ROOMS_PER_FLOOR 5
#define GRID_SIZE 5

#define MAX_ENEMIES 64
#define MAX_SHOTS 220
#define MAX_PICKUPS 28
#define MAX_BOMBS 8
#define MAX_ITEMS 18
#define MAX_PARTICLES 128
#define MAX_SCRIPT_OPS 4
#define SCRIPT_TEXT_LEN 256
#define ATLAS_CELL 128
#define ATLAS_COLS 8

#define PI_F 3.14159265359f

typedef enum RoomKind {
    ROOM_EMPTY,
    ROOM_START,
    ROOM_COMBAT,
    ROOM_TREASURE,
    ROOM_SHOP,
    ROOM_BOSS
} RoomKind;

typedef enum Direction {
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} Direction;

typedef enum EnemyKind {
    ENEMY_CHASER,
    ENEMY_SHOOTER,
    ENEMY_TANK,
    ENEMY_BOSS
} EnemyKind;

typedef enum PickupKind {
    PICKUP_HEART,
    PICKUP_COIN,
    PICKUP_BOMB,
    PICKUP_KEY,
    PICKUP_ITEM,
    PICKUP_EXIT
} PickupKind;

typedef enum ItemSlot {
    SLOT_HAT,
    SLOT_EYES,
    SLOT_HAND,
    SLOT_BACK,
    SLOT_BODY,
    SLOT_AURA
} ItemSlot;

typedef enum GamePhase {
    PHASE_PLAY,
    PHASE_GAME_OVER,
    PHASE_WIN
} GamePhase;

typedef enum AppMode {
    APP_MENU,
    APP_PLAY,
    APP_PAUSE
} AppMode;

typedef enum ScriptTrigger {
    SCRIPT_ON_FIRE,
    SCRIPT_ON_HIT
} ScriptTrigger;

typedef enum ScriptOpKind {
    SCRIPT_OP_NONE,
    SCRIPT_OP_BURST,
    SCRIPT_OP_PROJECTILE,
    SCRIPT_OP_AREA,
    SCRIPT_OP_HEAL
} ScriptOpKind;

typedef enum AtlasSprite {
    SPR_PLAYER,
    SPR_ENEMY_CHASER,
    SPR_ENEMY_SHOOTER,
    SPR_ENEMY_TANK,
    SPR_BOSS,
    SPR_ITEM,
    SPR_HEART,
    SPR_COIN,
    SPR_BOMB,
    SPR_KEY,
    SPR_EXIT,
    SPR_SHOT
} AtlasSprite;

enum {
    TRAIT_BOUNCE  = 1u << 0,
    TRAIT_HOMING  = 1u << 1,
    TRAIT_EXPLODE = 1u << 2,
    TRAIT_SPLIT   = 1u << 3,
    TRAIT_PIERCE  = 1u << 4,
    TRAIT_RAPID   = 1u << 5,
    TRAIT_GIANT   = 1u << 6,
    TRAIT_SLOW    = 1u << 7,
    TRAIT_VAMP    = 1u << 8
};

typedef struct Theme {
    char name[64];
    char style[48];
    char bossName[64];
    Color bg;
    Color floor;
    Color wall;
    Color accent;
    Color accent2;
    Color enemy;
    Color boss;
} Theme;

typedef struct Item {
    bool active;
    char name[48];
    ItemSlot slot;
    unsigned int traits;
    Color color;
    int shape;
    char script[SCRIPT_TEXT_LEN];
} Item;

typedef struct FloorContent {
    Theme theme;
    Item items[3];
} FloorContent;

typedef struct RunContent {
    bool loaded;
    char atlasPath[128];
    FloorContent floors[FLOOR_COUNT];
} RunContent;

typedef struct RoomState {
    bool exists;
    bool visited;
    bool cleared;
    bool rewardTaken;
    RoomKind kind;
    bool doors[4];
} RoomState;

typedef struct Player {
    Vector2 pos;
    float radius;
    float speed;
    int hp;
    int maxHp;
    int coins;
    int bombs;
    int keys;
    float damage;
    float fireDelay;
    float shotSpeed;
    float shotRadius;
    float fireTimer;
    float invuln;
    unsigned int traits;
    Item items[MAX_ITEMS];
    int itemCount;
} Player;

typedef struct Enemy {
    bool active;
    EnemyKind kind;
    Vector2 pos;
    Vector2 vel;
    float radius;
    float hp;
    float maxHp;
    float speed;
    float cooldown;
    float slowTimer;
} Enemy;

typedef struct Shot {
    bool active;
    bool fromPlayer;
    Vector2 pos;
    Vector2 vel;
    float radius;
    float damage;
    float life;
    unsigned int traits;
    int bounces;
    int pierce;
    bool splitDone;
    int scriptDepth;
    Color color;
} Shot;

typedef struct Pickup {
    bool active;
    PickupKind kind;
    Vector2 pos;
    float radius;
    int value;
    int cost;
    Item item;
} Pickup;

typedef struct Bomb {
    bool active;
    Vector2 pos;
    float timer;
    float radius;
} Bomb;

typedef struct Particle {
    bool active;
    Vector2 pos;
    Vector2 vel;
    float life;
    float radius;
    Color color;
} Particle;

typedef struct Game {
    RunContent content;
    Theme theme;
    Texture2D atlas;
    bool atlasLoaded;
    RoomState rooms[GRID_SIZE][GRID_SIZE];
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Shot shots[MAX_SHOTS];
    Pickup pickups[MAX_PICKUPS];
    Bomb bombs[MAX_BOMBS];
    Particle particles[MAX_PARTICLES];
    GamePhase phase;
    unsigned int rng;
    int floor;
    int roomX;
    int roomY;
    int score;
    int roomNumber;
    char message[160];
    float messageTimer;
} Game;

typedef struct UiLayout {
    Rectangle gameRect;
    Rectangle leftPanel;
    Rectangle rightPanel;
    Rectangle bottomPanel;
    float gameScale;
} UiLayout;

#endif
