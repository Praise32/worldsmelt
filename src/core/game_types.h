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
/* Sorgente Lua opzionale di un oggetto (fase 3a-L2, vedi
   docs/superpowers/specs/2026-07-13-lua-sandbox-design.md sezioni 5-9).
   Vuota ("") per un oggetto che usa solo la mini-VM (tutti gli oggetti
   generati oggi, dato che tools/melting-gen non scrive ancora Lua: e'
   deliberatamente fuori scopo per questo task, vedi il task brief). Quando
   non vuota, l'oggetto la eseguisce al posto del suo `script` mini-VM
   finche' resta valida (vedi src/script/script_items.c); se lo script Lua
   viene disabilitato dal patto di sicurezza, l'oggetto ripiega su `script`
   dallo stesso frame in poi, senza bisogno di alcuno switch esplicito. */
#define SCRIPT_LUA_LEN 2048
#define ATLAS_CELL 128
#define ATLAS_COLS 8

/* Soglia minima di pixel opachi perche' una cella dell'atlas sia considerata
   uno sprite vero e non una cella vuota. melting-sprites scarta una cella
   generata sotto il 5% di pixel opachi (819 su 16384 per una cella 128x128,
   vedi tools/melting-sprites/main.c, CellPassesQualityGate) e in quel caso la
   azzera per intero con memset: una cella scartata ha quindi SEMPRE zero
   pixel opachi, mai "quasi zero". 32 sta ben sotto quella soglia (non serve
   un margine stretto) ma ben sopra zero, cosi' qualche pixel opaco isolato
   (rumore residuo, un futuro cambio di pipeline) non fa scambiare una cella
   davvero rotta per uno sprite valido. */
#define ATLAS_CELL_MIN_OPAQUE 32

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

/* Tassonomia degli oggetti (fase 3, docs/superpowers/specs/2026-07-13-items-synergy-vision.md
   sezioni 1,2,5): ITEM_ACTIVE modifica come spari o ti muovi (i mattoni delle
   sinergie: stanze tesoro e negozio), ITEM_STATUP e' un puro aumento di
   statistiche (ricompensa del boss, nessun comportamento nuovo). ITEM_ACTIVE
   vale 0 di proposito: un Item azzerato con "{0}" (il pattern usato in tutto
   il codice, vedi CombatApplyItem/i test) o un manifest vecchio senza alcuna
   riga "kind=" restano attivi di default, mai stat-up per sbaglio. */
typedef enum ItemKind {
    ITEM_ACTIVE,
    ITEM_STATUP
} ItemKind;

/* Rarita' (fase 3b, docs/superpowers/specs/2026-07-13-pools-rarity-design.md
   sezioni 1-3): determina SIA la potenza (il tetto per-oggetto scalato per
   rarita', vedi src/script/script_items.c, SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION)
   SIA la frequenza di drop (le tabelle di peso per pool, vedi
   tools/melting-gen/gen_util.c lato generatore e src/content/run_content.c
   lato gioco). Niente campo "pool" a parte: il pool di un oggetto e' gia'
   la sua POSIZIONE (items[0..2] = tesoro/negozio, bossItem = boss, vedi
   FloorContent sotto), esattamente come "kind" gia' non ha bisogno di un
   campo aggiuntivo per sapere se e' un oggetto del piano o la ricompensa
   del boss. RARITY_COMMON vale 0 di proposito, stesso motivo di ITEM_ACTIVE
   sopra: un Item azzerato con "{0}" o un manifest senza una riga
   "rarity=" (vecchio, scritto prima di questa fase) restano comuni, mai una
   rarita' piu' alta per sbaglio (vedi RarityFromText in run_content.c). */
typedef enum Rarity {
    RARITY_COMMON,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_LEGENDARY
} Rarity;

typedef enum GamePhase {
    PHASE_PLAY,
    PHASE_GAME_OVER,
    PHASE_WIN
} GamePhase;

typedef enum AppMode {
    APP_MENU,
    APP_PLAY,
    APP_PAUSE,
    APP_GENERATING
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
    SPR_SHOT,
    SPR_COUNT   /* non e' una cella: conta le celle note, per dimensionare array */
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
    ItemKind kind;   /* ITEM_ACTIVE di default (vedi il commento sopra): mai stat-up senza che qualcuno lo imposti esplicitamente */
    Rarity rarity;   /* RARITY_COMMON di default (vedi il commento sopra): letta dal renderer (fase 3b VISIVA, task parallelo) per il colore del bordo, e da script_items.c/world.c per il tetto di potenza/costo negozio */
    Color color;
    int shape;
    char script[SCRIPT_TEXT_LEN];
    char luaSource[SCRIPT_LUA_LEN];   /* vedi il commento su SCRIPT_LUA_LEN sopra */
} Item;

typedef struct FloorContent {
    Theme theme;
    Item items[3];   /* oggetti ATTIVI del piano: stanza tesoro e negozio pescano da qui (world.c) */
    /* Oggetto STAT-UP del piano, campo esplicito e non un quarto slot di
       items[] (scelta deliberata, vedi il report di fase: docs/superpowers/sdd/
       phase3-items-report.md): src/render/game_renderer.c gia' itera
       "items[3]" con un letterale "3" per l'anteprima del piano (fuori scopo
       di questo task, di proprieta' di un lavoro parallelo sulla grafica) -
       crescere items[] a 4 avrebbe silenziosamente infilato l'oggetto del
       boss in quella anteprima "oggetti del piano" senza toccare quel file.
       Un campo a parte rende impossibile quel bug per costruzione e non
       richiede alcuna modifica al renderer. E' SEMPRE la ricompensa del boss
       del piano (world.c, WorldSpawnRoomContents/WorldSpawnRoomReward), mai
       pescato a caso come items[0..2]. */
    Item bossItem;
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
    /* Valori di PARTENZA (prima di qualunque oggetto), da cui
       ScriptItemsRecomputeStats riparte OGNI VOLTA che ricalcola: e' il
       sistema delle cache "alla Isaac" (spec, sezione 7). damage/fireDelay/
       shotSpeed/shotRadius/speed/maxHp sopra sono invece il risultato
       dell'ultimo ricalcolo, mai mutati direttamente altrove (vedi
       src/script/script_items.c, ScriptItemsRecomputeStats): permette di
       rimuovere/aggiungere un oggetto senza deriva, e rende idempotente
       ripetere lo stesso passaggio piu' volte. */
    float baseDamage;
    float baseFireDelay;
    float baseShotSpeed;
    float baseShotRadius;
    float baseSpeed;
    int baseMaxHp;
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

/* Stato di runtime Lua per l'oggetto nello slot i-esimo di Player.items[]
   (fase 3a-L2). Vive qui, in "core", non in src/script/, perche' Game deve
   restare un dato POD semplice (array fissi, zero allocazioni fuori da Lua
   stesso, coerente con lo stile del resto del file) che game.c puo'
   azzerare con un memset in GameResetRun -- ma SOLO se qualcuno ha gia'
   distrutto le sandbox vive prima di quel memset (vedi ScriptItemsShutdown,
   chiamata da GameResetRun come GameUnloadAssets). 'sandbox' e' un
   ScriptSandbox* volutamente tipizzato void*: game_types.h e' "core" e non
   deve dipendere da src/script/ (vedi AGENTS.md); solo script_items.c lo
   interpreta davvero, con un cast. I quattro *Ref e statsTableRef sono
   riferimenti luaL_ref nel registro DI QUELLA sandbox (creazione pigra al
   caricamento riuscito, vedi script_items.c): -1 = nessun riferimento. */
typedef struct ScriptItemRuntime {
    void *sandbox;
    int evalRef;
    int fireRef;
    int hitRef;
    int tickRef;
    int statsTableRef;
} ScriptItemRuntime;

typedef struct Game {
    RunContent content;
    Theme theme;
    Texture2D atlas;
    bool atlasLoaded;
    /* Per ciascuna delle SPR_COUNT celle note: vero se contiene abbastanza
       pixel opachi da essere uno sprite vero (vedi ATLAS_CELL_MIN_OPAQUE).
       Una cella rimasta vuota (gate di qualita' di melting-sprites fallito)
       ha questo flag falso, e DrawAtlasCell ripiega sulla forma geometrica
       di riserva SOLO per quella cella, non per l'intero atlas. */
    bool atlasCellPresent[SPR_COUNT];
    RoomState rooms[GRID_SIZE][GRID_SIZE];
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Shot shots[MAX_SHOTS];
    Pickup pickups[MAX_PICKUPS];
    Bomb bombs[MAX_BOMBS];
    Particle particles[MAX_PARTICLES];
    /* Contatori di generazione per l'API a handle di Lua (spec, sezione 5):
       incrementati in EntitiesAddEnemy/EntitiesAddShot ogni volta che uno
       slot viene (ri)assegnato. Un handle e' indice+generazione impacchettati
       (vedi src/script/script_api.c): se lo slot e' stato riusato da
       un'altra entita' nel frattempo, la generazione non combacia piu' e la
       chiamata viene rifiutata (script ucciso, vedi il patto di sicurezza)
       invece di leggere/scrivere l'entita' sbagliata. Array separati da
       enemies/shots (non un campo dentro Enemy/Shot) cosi' EntitiesClear
       (che azzera quegli array con un memset) non li tocca: la generazione
       deve continuare a crescere anche attraverso una pulizia di stanza,
       altrimenti un handle catturato prima di EntitiesClear e uno catturato
       dopo, sullo stesso indice, sarebbero indistinguibili. */
    unsigned int enemyGen[MAX_ENEMIES];
    unsigned int shotGen[MAX_SHOTS];
    /* Runtime Lua per ciascuno slot di player.items[] (stesso indice). Vedi
       il commento su ScriptItemRuntime sopra. */
    ScriptItemRuntime itemScripts[MAX_ITEMS];
    /* Bandiera sporca del sistema delle cache (spec, sezione 7): impostata
       da CombatApplyItem quando un oggetto viene acquisito, consumata una
       volta per frame da GameUpdate (ScriptItemsProcessDirty), che chiama
       ScriptItemsRecomputeStats solo se davvero necessario invece che ad
       ogni frame. */
    bool statsDirty;
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

/* Progresso del generatore esterno (melting-gen), letto da gen_progress.txt. */
typedef struct GenProgress {
    char phase[32];
    int percent;
    char message[96];
} GenProgress;

#endif
