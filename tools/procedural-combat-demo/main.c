#include "raylib.h"
#include "raymath.h"

#include "demo_script_api.h"
#include "script/script_sandbox.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* WP4 (spec docs/engineering/specs/2026-08-05-combat-lab-design.md, sezione
 * 2): melting-gen gira come PROCESSO FIGLIO, mai linkato (ADR-002). La build
 * Linux e' l'unica viva per l'arena (AGENTS.md), quindi questi header POSIX
 * entrano diretti, senza guardia di piattaforma: gli script .ps1 restano per
 * la prova storica a sei scene su Windows, non questo file. */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

/* --------------------------------------------------------------------------
 * v3 (spec sezione "v3", playtest 08/08): budget di bilanciamento HOST-SIDE.
 * Il feedback del proprietario e' stato "nemici e armi generati lanciano
 * tantissimi proiettili": l'API v2 (demo_script_api.h) e' CONGELATA (firme,
 * costanti pubbliche, semantica), quindi questi tetti vivono qui, nel
 * consumo dei comandi (DemoConsumeCommands), non nella sandbox. Un comando
 * oltre budget non e' un errore Lua e non disabilita nulla: si tronca o si
 * scarta in silenzio, la sandbox resta viva e lo script continua a girare.
 * -------------------------------------------------------------------------- */

/* Colpi ATTIVI contemporaneamente per proprietario (somma su DemoProjectile
 * con lo stesso flag hostile). Oltre il tetto, EMIT_RING/ORBIT/la mezzaluna
 * di EMIT_ARC/RELEASE_ECHOES si fermano a meta' burst invece di allocare
 * senza limite nei 640 slot condivisi. Tarati sui picchi reali dei 5 script
 * curati (mezzaluna del ragno: 13 colpi in un colpo solo; ring+orbit della
 * falena: 13) con ampio margine per il contenuto generato. */
#define DEMO_ENEMY_SHOT_CAP  56
#define DEMO_WEAPON_SHOT_CAP 40

/* Token bucket proiettili/secondo per entita': ricarica continua a "rate"
 * token/s fino al tetto "rate" (capacita' == rate), si parte a META' capacita'
 * cosi' il primissimo tick non puo' mai scaricare un burst pieno. Tarati sopra
 * la media reale dei curati (falena/ragno ~6-7 colpi/s in scoppio, il resto
 * del ciclo e' fermo) con ampio margine: il bucket serve a tagliare la CODA
 * lunga di Gemma (uno script che spara ogni tick), non i burst dei curati. */
#define DEMO_ENEMY_SHOT_RATE_PER_SEC  30.0f
#define DEMO_WEAPON_SHOT_RATE_PER_SEC 25.0f

/* Cooldown minimo fra due emissioni ACCETTATE della stessa categoria per la
 * stessa entita' (nemico o arma, non il singolo tipo di comando: l'orbit
 * conta come ring). Il gate confronta col timestamp dell'ultima emissione
 * accettata della categoria, aggiornato UNA SOLA VOLTA a fine tick
 * (DemoThrottleCommitTick) e non ad ogni comando: cosi' piu' emissioni della
 * stessa categoria nello STESSO tick (es. emit_ring+emit_orbit nella stessa
 * transizione di fase) si confrontano tutte col valore "vecchio" e passano
 * insieme -- solo lo spam FRA un tick e l'altro viene tagliato. Nessuna
 * categoria per emit_arc da solo (i colpi della mezzaluna sono gia' limitati
 * dal bucket proiettili/s; l'unico rischio reale osservato e'
 * ring/orbit/echoes, gia' coperto).
 *
 * I TELEGRAFI NON ENTRANO IN NESSUN GATE (correzione 08/08): telegraph_arc e
 * telegraph_beam non spawnano niente e non fanno danno, quindi non consumano
 * ne' bucket ne' cooldown. Condividere la categoria beam fra telegraph_beam e
 * emit_beam rendeva INERTE ogni pattern sano con windup < 0.35s fra l'annuncio
 * e il colpo (il telegrafo prendeva il cooldown, l'emit lo trovava chiuso e il
 * raggio che fa danno non nasceva mai: 30s simulati, 18 cicli, zero danno) --
 * ed era peggio col telegrafo ridisegnato ogni tick, che ricaricava il
 * cooldown all'infinito. I telegrafi restano soggetti alle sole quote per-tick
 * dell'API (demo_script_api.h, congelata: 64 comandi visuali per tick). */
#define DEMO_COOLDOWN_BEAM_SECONDS           0.35f
#define DEMO_COOLDOWN_MELEE_SWEEP_SECONDS    0.25f
#define DEMO_COOLDOWN_CAPTURE_RADIUS_SECONDS 0.8f
#define DEMO_COOLDOWN_RING_SECONDS           0.12f

/* Sentinella "mai emesso": abbastanza nel passato che la primissima emissione
 * di ogni categoria, subito dopo un reset/spawn, non venga MAI scartata per
 * cooldown (un memset a zero lascerebbe 0.0, che con globalTime=0 leggerebbe
 * "appena emesso" e bloccherebbe il primo colpo del gioco). */
#define DEMO_COOLDOWN_NEVER (-1000.0f)

/* WP4: generatore esterno (spec sezione 2, ADR-002 -- bin/melting-gen come
 * processo figlio, mai linkato). Percorsi relativi alla CWD, come
 * DEMO_*_GENERATED_DIR sopra: la demo si lancia dalla radice del repo. */
#define DEMO_GEN_BIN "bin/melting-gen"
#define DEMO_GEN_OUT_DIR "generated"
#define DEMO_GEN_BRIEF_PATH "generated/combat-lab/brief.txt"
#define DEMO_GEN_LOG_DIR "logs"
#define DEMO_GEN_LOG_PATH "logs/combat-lab-gen.log"
/* Valore dei tasti G/H (task brief WP4, requisito 8: la funzione di spawn
 * riceve il conteggio come PARAMETRO apposta, cosi' l'harness di verifica
 * puo' passare 1 senza toccare questa costante). */
#define DEMO_GEN_KEY_ATTACK_COUNT 3
/* Quanto restano visibili "lotto pronto"/"fallita" prima che la riga GEN
 * torni al segnaposto "GEN: --" (stesso ruolo del vecchio
 * DEMO_INFO_MESSAGE_SECONDS, ora dentro DemoGenState invece che nel mondo:
 * deve sopravvivere a DemoResetArena, vedi il commento su DemoGenState). */
#define DEMO_GEN_STATUS_SECONDS 5.0f
#define DEMO_GEN_STATUS_TEXT_MAX 192
#define DEMO_GEN_BRIEF_LINE_MAX 72
/* argv massimi verso execv: bin + 8 fissi (--attacks/--attack-count/--seed/
 * --out) + 2 opzionali (--attack-brief <path>) + NULL = 12; 16 lascia
 * margine senza dover ricontare ogni volta che si aggiunge un flag. */
#define DEMO_GEN_MAX_ARGV 16

static const Rectangle DEMO_ROOM = { 58.0f, 88.0f, 1164.0f, 566.0f };

/* Bande del debug UI (correzione 08/08). La geometria dell'arena NON si tocca
 * -- DEMO_ROOM e' la visuale di gioco originale, a cui il proprietario tiene --
 * quindi tutto il debug vive FUORI da quel rettangolo: banda alta
 * 0..DEMO_HUD_TOP_HEIGHT (esattamente DEMO_ROOM.y: l'ultima riga coperta e' la
 * 87, la prima riga d'arena e' la 88) per i due pannelli NEMICO/ARMA, banda
 * bassa da DEMO_HUD_BOTTOM_Y (sotto il bordo inferiore dell'arena, y=654) a
 * fine finestra per contatori, riga GEN e legenda dei tasti. Il banner
 * temporaneo prende la striscia DEMO_HUD_BANNER_Y..DEMO_HEIGHT, cioe' copre la
 * legenda e nient'altro. Prima la barra pannelli era alta 130px: tagliava lo
 * sprite del nemico (bordo alto y~103) e nascondeva del tutto la sua barra
 * vita (y~91). */
#define DEMO_HUD_TOP_HEIGHT 88
#define DEMO_HUD_BOTTOM_Y   658
#define DEMO_HUD_BANNER_Y   693

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

typedef enum DemoGenKind {
    DEMO_GEN_KIND_NONE = 0,
    DEMO_GEN_KIND_ENEMY,
    DEMO_GEN_KIND_WEAPON
} DemoGenKind;

/* Stato del generatore esterno (WP4). Vive A FIANCO di DemoWorld (variabile
 * locale a se' in main(), come enemyPool/weaponPool), MAI dentro: un lotto in
 * corso o il toggle del brief non devono sparire ad ogni morte del player o
 * pressione di R, che passano DemoWorld per un memset a zero
 * (DemoResetArena). */
typedef struct DemoGenState {
    pid_t pid;      /* 0 = nessun figlio vivo: UNA generazione alla volta (requisito 1) */
    DemoGenKind kind; /* kind del figlio vivo, o dell'ultimo lotto concluso */
    /* Riga "GEN: ..." mostrata in HUD. Fonte di verita' unica: chi la scrive
     * (spawn/poll) decide anche se e' permanente o a tempo via statusTimer,
     * il disegno la stampa e basta, senza logica propria. */
    char statusText[DEMO_GEN_STATUS_TEXT_MAX];
    /* <=0 = permanente (in corso / gia' in corso / idle); >0 = countdown, a
     * zero DemoGenDecayStatus la riporta al segnaposto "GEN: --". */
    float statusTimer;
    bool useBrief;      /* toggle tasto B (default: acceso se il file non e' vuoto all'avvio) */
    bool briefPresent;  /* brief.txt esiste e non e' vuoto (fotografia dell'ultima DemoGenReloadBrief) */
    char briefFirstLine[DEMO_GEN_BRIEF_LINE_MAX]; /* prima riga (troncata) per l'HUD */
    bool loggedPathOnce; /* la riga col percorso del log compare in HUD una sola volta (requisito 2) */
} DemoGenState;

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

/* Categorie di cooldown v3 (vedi il blocco di #define sopra): un indice per
 * ognuna, cosi' DemoThrottleState porta un array invece di quattro campi
 * ripetuti a mano. */
typedef enum DemoCooldownCategory {
    DEMO_COOLDOWN_BEAM = 0,
    DEMO_COOLDOWN_MELEE_SWEEP,
    DEMO_COOLDOWN_CAPTURE_RADIUS,
    DEMO_COOLDOWN_RING,
    DEMO_COOLDOWN_CATEGORY_COUNT
} DemoCooldownCategory;

/* Budget v3 per UNA entita' (nemico o arma player): bucket proiettili/s +
 * cooldown per categoria + il contatore "scartati" che l'HUD legge. Vive
 * dentro DemoWorld (due istanze, enemyThrottle/weaponThrottle) ma NON si puo'
 * azzerare con un semplice memset: DemoThrottleInit e' l'unico modo corretto
 * di riportarlo a uno stato iniziale (vedi il commento sulla sentinella
 * DEMO_COOLDOWN_NEVER sopra). */
typedef struct DemoThrottleState {
    float shotBucket;
    int throttledWindowCount;  /* eventi scartati nella finestra APERTA (ultimo secondo in corso) */
    int throttledLastSecond;   /* valore congelato per l'HUD: la finestra chiusa precedente */
    float windowTimer;
    float cooldownLast[DEMO_COOLDOWN_CATEGORY_COUNT];
    bool cooldownUsedThisTick[DEMO_COOLDOWN_CATEGORY_COUNT];
} DemoThrottleState;

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

    DemoThrottleState enemyThrottle;
    DemoThrottleState weaponThrottle;

    float globalTime;
    int storedEchoes;
    int capturedTotal;
    int hitsDealt;
    int lastEnemyCommandCount;
    int lastWeaponCommandCount;
    uint32_t cosmeticRng;
} DemoWorld;

/* Stato UI transitorio v3 (playtest 08/08: "non si capisce il cambio arma/
 * nemico", "a volte le entita' sembra che non si vedano"). Vive A FIANCO di
 * DemoWorld in main(), come DemoGenState: un banner o l'avviso asset non
 * devono sparire ad ogni DemoResetArena (memset di world), che non tocca
 * ne' il pool ne' cosa il proprietario ha appena cambiato. */
typedef struct DemoUiState {
    char bannerText[160];
    Color bannerColor;
    float bannerTimer;      /* >0 mentre il banner e' visibile, vedi DEMO_UI_BANNER_SECONDS */
    bool assetWarningShown; /* un asset stock e' risultato invalido almeno una volta: riga sticky in HUD */
} DemoUiState;

#define DEMO_UI_BANNER_SECONDS 2.5f

static const Color DEMO_TAG_CURATED_COLOR   = { 126, 224, 255, 255 }; /* stesso azzurro "accent" gia' in uso nell'HUD */
static const Color DEMO_TAG_GENERATED_COLOR = { 255, 181, 71, 255 };  /* stesso arancio di VIS_GRAVITY: "attenzione, e' Gemma" */
static const Color DEMO_TAG_BASE_COLOR      = { 188, 201, 222, 255 }; /* pistola base: ne' curato ne' generato */

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

/* Garanzia di visibilita' v3 (playtest 08/08: "a volte le entita' sembra che
 * non si vedano"): qualunque colore visuale, presente o futuro, resta
 * leggibile sul fondo quasi nero dell'arena. I 6 colori curati sotto sono
 * gia' tutti chiari (non tocca niente in pratica); e' rete di sicurezza per
 * un VIS_* aggiunto in seguito con una tinta scura, o per DemoVisualColor
 * chiamata su un id non ancora mappato ma diverso da 0/default. */
static Color DemoEnsureVisibleColor(Color color)
{
    const int MIN_LUMINANCE = 150;
    int maxChannel = color.r;
    if (color.g > maxChannel) maxChannel = color.g;
    if (color.b > maxChannel) maxChannel = color.b;
    if (maxChannel >= MIN_LUMINANCE) return color;

    /* Nero puro: non c'e' nessuna tinta da preservare, l'unica risposta
     * possibile e' un grigio esattamente sulla soglia. Senza questo ramo la
     * "garanzia" saltava proprio nel caso peggiore, l'unico per cui esiste. */
    if (maxChannel <= 0) return (Color){ MIN_LUMINANCE, MIN_LUMINANCE, MIN_LUMINANCE, color.a };

    /* Scala i tre canali dello stesso fattore finche' il piu' alto vale
     * esattamente MIN_LUMINANCE: e' un floor vero (non un mix verso il bianco,
     * che asintoticamente non lo raggiungeva mai) e conserva la tinta, perche'
     * i rapporti fra i canali restano identici. Nessun canale puo' sforare:
     * per costruzione sono tutti <= maxChannel. Il +0.5f e' l'arrotondamento,
     * altrimenti il troncamento del cast fermerebbe il massimo a 149. */
    float scale = (float)MIN_LUMINANCE/(float)maxChannel;
    return (Color){
        (unsigned char)DemoClamp((float)color.r*scale + 0.5f, 0.0f, 255.0f),
        (unsigned char)DemoClamp((float)color.g*scale + 0.5f, 0.0f, 255.0f),
        (unsigned char)DemoClamp((float)color.b*scale + 0.5f, 0.0f, 255.0f),
        color.a
    };
}

static Color DemoVisualColorRaw(int visualId)
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

static Color DemoVisualColor(int visualId)
{
    return DemoEnsureVisibleColor(DemoVisualColorRaw(visualId));
}

/* --------------------------------------------------------------------------
 * v3: budget di bilanciamento. Bucket proiettili/s + cooldown per categoria,
 * per UNA entita' (DemoThrottleState). Vedi il blocco di #define in cima al
 * file per i valori e il razionale di taratura.
 * -------------------------------------------------------------------------- */

/* Azzera e riporta a uno stato iniziale SICURO: bucket a meta' capacita' (mai
 * pieno al primo tick), cooldown tutti alla sentinella "mai emesso" (mai un
 * memset a zero, che leggerebbe "appena emesso" a globalTime=0 e bloccherebbe
 * la primissima emissione del gioco). Chiamata all'avvio e ad ogni (ri)carica
 * dello script proprietario: uno script nuovo parte sempre con un budget
 * fresco, mai a meta' consumato da quello precedente. */
static void DemoThrottleInit(DemoThrottleState *throttle, float ratePerSecond)
{
    memset(throttle, 0, sizeof *throttle);
    throttle->shotBucket = ratePerSecond*0.5f;
    for (int i = 0; i < DEMO_COOLDOWN_CATEGORY_COUNT; i++) throttle->cooldownLast[i] = DEMO_COOLDOWN_NEVER;
}

/* Ricarica continua del bucket (una volta per tick fisso, MAI per comando: la
 * ricarica e' nel tempo, non nel numero di emissioni) + rotazione della
 * finestra "scartati/s" mostrata in HUD. La finestra si chiude ogni secondo
 * VERO (windowTimer), non ogni tick fisso: a 60 tick/s il numero cambierebbe
 * visibilmente ogni frame invece di leggersi come "al secondo". */
static void DemoThrottleRefill(DemoThrottleState *throttle, float ratePerSecond, float dt)
{
    throttle->shotBucket = fminf(ratePerSecond, throttle->shotBucket + ratePerSecond*dt);
    throttle->windowTimer += dt;
    if (throttle->windowTimer >= 1.0f)
    {
        throttle->windowTimer -= 1.0f;
        throttle->throttledLastSecond = throttle->throttledWindowCount;
        throttle->throttledWindowCount = 0;
    }
}

/* true se la categoria puo' emettere ORA: confronta col timestamp
 * dell'ULTIMA emissione ACCETTATA, che DemoThrottleCommitTick aggiorna una
 * volta sola a fine tick (vedi il commento sul gate nel blocco #define). */
static bool DemoThrottleCooldownReady(const DemoThrottleState *throttle, DemoCooldownCategory category,
                                      float tickTime, float cooldownSeconds)
{
    return (tickTime - throttle->cooldownLast[category]) >= cooldownSeconds;
}

static void DemoThrottleMarkCooldownUsed(DemoThrottleState *throttle, DemoCooldownCategory category)
{
    throttle->cooldownUsedThisTick[category] = true;
}

/* Commit di fine tick (chiamata una volta, alla fine di DemoConsumeCommands):
 * sposta avanti SOLO i timestamp delle categorie usate in QUESTO tick, tutte
 * insieme, cosi' due emissioni della stessa categoria nello stesso tick non
 * si bloccano a vicenda (si confrontano entrambe col valore vecchio prima di
 * questo commit). */
static void DemoThrottleCommitTick(DemoThrottleState *throttle, float tickTime)
{
    for (int i = 0; i < DEMO_COOLDOWN_CATEGORY_COUNT; i++)
    {
        if (throttle->cooldownUsedThisTick[i])
        {
            throttle->cooldownLast[i] = tickTime;
            throttle->cooldownUsedThisTick[i] = false;
        }
    }
}

/* Il nome dei file generati e' sempre "NNN_seed<seed>.lua" (WriteAttackScript
 * in tools/melting-gen/gen_attacks.c): estrae il seed per l'HUD leggendo
 * quella convenzione in UN solo punto, cosi' se mai cambiasse basta toccare
 * qui. Ritorna 0 per un nome che non rispetta il pattern (o per un'entrata
 * curata, che non ha un seed di generazione). */
static unsigned int DemoPoolEntrySeed(const DemoPoolEntry *entry)
{
    if (entry->source != DEMO_POOL_SOURCE_GENERATED) return 0;
    const char *marker = strstr(entry->fileName, "_seed");
    if (marker == NULL) return 0;
    return (unsigned int)strtoul(marker + 5, NULL, 10);
}

static void DemoStripLuaExtension(char *destination, size_t size, const char *fileName)
{
    snprintf(destination, size, "%s", fileName);
    size_t length = strlen(destination);
    if (length > 4 && strcmp(destination + length - 4, ".lua") == 0) destination[length - 4] = '\0';
}

static Color DemoSourceTagColor(DemoPoolSource source)
{
    return source == DEMO_POOL_SOURCE_CURATED ? DEMO_TAG_CURATED_COLOR : DEMO_TAG_GENERATED_COLOR;
}

static const char *DemoSourceTagLabel(DemoPoolSource source)
{
    return source == DEMO_POOL_SOURCE_CURATED ? "CURATO" : "GEMMA";
}

/* Banner grande temporaneo (v3, requisito HUD "il cambio arma/nemico e' poco
 * chiaro"): un solo setter, usato per il cambio pool E per il lotto atterrato.
 * Sovrascrive un banner ancora visibile invece di accodarsi: l'ultimo evento
 * vince, mai una coda di messaggi. */
static void DemoUiSetBanner(DemoUiState *ui, Color color, const char *text)
{
    snprintf(ui->bannerText, sizeof ui->bannerText, "%s", text);
    ui->bannerColor = color;
    ui->bannerTimer = DEMO_UI_BANNER_SECONDS;
}

static void DemoUiAnnounceEnemy(DemoUiState *ui, const DemoPool *enemyPool)
{
    if (enemyPool->count <= 0) return;
    const DemoPoolEntry *entry = &enemyPool->entries[enemyPool->current];
    char name[DEMO_POOL_NAME_MAX];
    DemoStripLuaExtension(name, sizeof name, entry->fileName);
    if (entry->source == DEMO_POOL_SOURCE_GENERATED)
        DemoUiSetBanner(ui, DEMO_TAG_GENERATED_COLOR,
                       TextFormat("NEMICO -> GEMMA: %s (seed %u)", name, DemoPoolEntrySeed(entry)));
    else
        DemoUiSetBanner(ui, DEMO_TAG_CURATED_COLOR, TextFormat("NEMICO -> CURATO: %s", name));
}

static void DemoUiAnnounceWeapon(DemoUiState *ui, const DemoPool *weaponPool)
{
    if (weaponPool->current == 0)
    {
        DemoUiSetBanner(ui, DEMO_TAG_BASE_COLOR, "ARMA -> BASE: pistola");
        return;
    }
    const DemoPoolEntry *entry = &weaponPool->entries[weaponPool->current - 1];
    char name[DEMO_POOL_NAME_MAX];
    DemoStripLuaExtension(name, sizeof name, entry->fileName);
    if (entry->source == DEMO_POOL_SOURCE_GENERATED)
        DemoUiSetBanner(ui, DEMO_TAG_GENERATED_COLOR,
                       TextFormat("ARMA -> GEMMA: %s (seed %u)", name, DemoPoolEntrySeed(entry)));
    else
        DemoUiSetBanner(ui, DEMO_TAG_CURATED_COLOR, TextFormat("ARMA -> CURATO: %s", name));
}

/* Secondo innesco del banner (v3: "... o quando ATTERRA una generazione"),
 * indipendente dal primo: un lotto puo' finire senza che il proprietario
 * cambi subito lo slot attivo. L'annuncio segue l'ESITO del figlio, non il
 * delta di conteggio del pool (correzione 08/08: con il pool gia' a
 * DEMO_POOL_MAX_ENTRIES il delta e' 0 anche per una generazione perfettamente
 * riuscita, e il lotto atterrava in silenzio). newCount resta per distinguere
 * "ci sono voci nuove" da "il pool e' pieno, quei file li vedrai al prossimo
 * avvio": due messaggi diversi, entrambi meglio di nessun banner. Un lotto
 * FALLITO non fa banner: resta sulla riga GEN, che dice anche l'exit code. */
static void DemoUiAnnounceGenReady(DemoUiState *ui, DemoGenKind kind, int newCount, bool poolFull)
{
    const char *kindLabel = (kind == DEMO_GEN_KIND_ENEMY) ? "nemico" : "arma";
    if (newCount > 0)
        DemoUiSetBanner(ui, DEMO_TAG_GENERATED_COLOR,
                       TextFormat("GEMMA: lotto %s pronto (+%d)", kindLabel, newCount));
    else if (poolFull)
        DemoUiSetBanner(ui, DEMO_TAG_GENERATED_COLOR,
                       TextFormat("GEMMA: lotto %s pronto ma pool pieno (%d voci): riavvia per vederlo",
                                  kindLabel, DEMO_POOL_MAX_ENTRIES));
    else
        DemoUiSetBanner(ui, DEMO_TAG_GENERATED_COLOR,
                       TextFormat("GEMMA: lotto %s concluso, nessuno script nuovo sul disco", kindLabel));
}

static void DemoUiTick(DemoUiState *ui, float frameTime)
{
    if (ui->bannerTimer > 0.0f) ui->bannerTimer = fmaxf(0.0f, ui->bannerTimer - frameTime);
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
 * ancora. logs/ (WP4) si aggiunge qui per lo stesso motivo: DemoGenSpawn ci
 * apre il log del figlio con O_CREAT, che fallisce silenziosamente se la
 * cartella manca (un checkout appena clonato non la porta, e' in
 * .gitignore). */
static void DemoEnsureGeneratedDirs(void)
{
    if (!DirectoryExists(DEMO_ENEMY_GENERATED_DIR)) MakeDirectory(DEMO_ENEMY_GENERATED_DIR);
    if (!DirectoryExists(DEMO_WEAPON_GENERATED_DIR)) MakeDirectory(DEMO_WEAPON_GENERATED_DIR);
    if (!DirectoryExists(DEMO_GEN_LOG_DIR)) MakeDirectory(DEMO_GEN_LOG_DIR);
}

/* ------------------------------------------------------------------------
 * WP4: generatore esterno come processo figlio (ADR-002). Tre responsabilita'
 * separate come in src/gen/gen_runner.c (che qui NON si puo' riusare: quello
 * e' per il gioco vero, dietro guardie Windows/Linux che main.c non deve
 * portarsi dietro per una demo solo-Linux) -- spawn non bloccante, poll a
 * WNOHANG una volta per frame, lettura del brief per l'HUD.
 * ---------------------------------------------------------------------- */

/* Riporta la riga GEN al segnaposto originale: usata sia all'avvio sia dal
 * decadimento del timer (DemoGenDecayStatus). statusTimer <=0 qui e' apposta
 * "permanente": l'idle non deve mai scomparire da solo. */
static void DemoGenResetStatus(DemoGenState *gen)
{
    snprintf(gen->statusText, sizeof gen->statusText, "GEN: --");
    gen->statusTimer = -1.0f;
}

/* Rilegge generated/combat-lab/brief.txt (tasto B, spec sezione 4: "ricarica
 * brief.txt"). Un file assente o fatto di soli spazi/a-capo conta come vuoto:
 * altrimenti il default "acceso se il file esiste non vuoto" (requisito 5 del
 * task WP4) si accenderebbe su un file bianco che il proprietario non ha
 * ancora scritto per davvero. Non tocca useBrief -- quello lo decide SOLO la
 * pressione di B (vedi il chiamante in main()) -- qui si aggiorna solo cosa
 * il file CONTIENE, cosi' l'HUD e il prossimo spawn vedono la stessa foto. */
static void DemoGenReloadBrief(DemoGenState *gen)
{
    gen->briefPresent = false;
    gen->briefFirstLine[0] = '\0';

    char *text = LoadFileText(DEMO_GEN_BRIEF_PATH);
    if (text == NULL) return;

    size_t length = strlen(text);
    size_t start = 0;
    while (start < length && isspace((unsigned char)text[start])) start++;
    if (start < length)
    {
        size_t end = start;
        while (end < length && text[end] != '\n' && text[end] != '\r') end++;
        size_t copyLength = end - start;
        bool truncated = false;
        /* -4: margine per il "..." (3 byte) + terminatore, cosi' la copia
         * sotto non deborda mai anche sulla riga piu' lunga possibile. */
        if (copyLength > sizeof gen->briefFirstLine - 4)
        {
            copyLength = sizeof gen->briefFirstLine - 4;
            truncated = true;
        }
        memcpy(gen->briefFirstLine, text + start, copyLength);
        gen->briefFirstLine[copyLength] = '\0';
        if (truncated) strcat(gen->briefFirstLine, "...");
        gen->briefPresent = true;
    }
    UnloadFileText(text);
}

/* Stato iniziale (main(), una volta sola): idle in HUD, brief riletto da
 * disco, toggle acceso solo se il file c'e' davvero (requisito 5). */
static void DemoGenInit(DemoGenState *gen)
{
    memset(gen, 0, sizeof *gen);
    DemoGenResetStatus(gen);
    DemoGenReloadBrief(gen);
    gen->useBrief = gen->briefPresent;
}

/* Testo della riga "brief: ..." in HUD -- separata da statusText perche' e'
 * uno stato persistente (il toggle B), non un messaggio a tempo. */
static const char *DemoGenBriefLabel(const DemoGenState *gen)
{
    if (!gen->useBrief) return "off";
    if (!gen->briefPresent) return "(vuoto)";
    return gen->briefFirstLine;
}

/* Spawna bin/melting-gen come figlio non bloccante (ADR-002: la demo non
 * linka mai llama.cpp). 'count' e' un PARAMETRO apposta (non
 * DEMO_GEN_KEY_ATTACK_COUNT direttamente): l'harness di verifica del task
 * brief passa 1 per un giro reale rapido, i tasti G/H passano sempre la
 * costante. UNA generazione alla volta (requisito 1): se un figlio e' gia'
 * vivo la richiesta e' rifiutata SENZA toccare lo stato del lotto in corso --
 * l'harness verifica anche questo chiamando la funzione due volte di fila
 * mentre il primo figlio e' ancora vivo. */
static bool DemoGenSpawn(DemoGenState *gen, DemoGenKind kind, int count, unsigned int seed)
{
    if (gen->pid != 0)
    {
        snprintf(gen->statusText, sizeof gen->statusText, "GEN: gia' in corso");
        gen->statusTimer = -1.0f;   /* resta finche' il figlio vivo non finisce (DemoGenPollTick) */
        return false;
    }

    const char *kindText = (kind == DEMO_GEN_KIND_ENEMY) ? "enemy" : "weapon";
    char countText[16];
    char seedText[16];
    snprintf(countText, sizeof countText, "%d", count);
    snprintf(seedText, sizeof seedText, "%u", seed);
    /* Il brief entra negli argomenti solo se ATTIVO E il file e' davvero non
     * vuoto (requisito 2): DemoGenReloadBrief e' l'unica a sapere quale delle
     * due condizioni vale, fotografata in briefPresent all'ultima rilettura. */
    bool briefActive = gen->useBrief && gen->briefPresent;

    pid_t pid = fork();
    if (pid < 0)
    {
        snprintf(gen->statusText, sizeof gen->statusText, "GEN: fork fallita");
        gen->statusTimer = DEMO_GEN_STATUS_SECONDS;
        return false;
    }
    if (pid == 0)
    {
        /* Figlio: stdout/stderr del generatore in append sul log condiviso
         * (requisito 2, "il terminale della demo resta pulito"). Se l'open
         * fallisce (es. logs/ non scrivibile nonostante DemoEnsureGeneratedDirs)
         * si lascia che il figlio erediti gli stream del genitore invece di
         * morire: un lotto rumoroso nel terminale e' meglio di nessun lotto. */
        int logFd = open(DEMO_GEN_LOG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (logFd >= 0)
        {
            dup2(logFd, STDOUT_FILENO);
            dup2(logFd, STDERR_FILENO);
            close(logFd);
        }

        char *argv[DEMO_GEN_MAX_ARGV];
        int argc = 0;
        argv[argc++] = (char *)DEMO_GEN_BIN;
        argv[argc++] = (char *)"--attacks";
        argv[argc++] = (char *)kindText;
        argv[argc++] = (char *)"--attack-count";
        argv[argc++] = countText;
        argv[argc++] = (char *)"--seed";
        argv[argc++] = seedText;
        argv[argc++] = (char *)"--out";
        argv[argc++] = (char *)DEMO_GEN_OUT_DIR;
        if (briefActive)
        {
            argv[argc++] = (char *)"--attack-brief";
            argv[argc++] = (char *)DEMO_GEN_BRIEF_PATH;
        }
        argv[argc] = NULL;

        execv(DEMO_GEN_BIN, argv);
        {
            /* L'HUD del padre rimanda al log su exit 127: una riga nel log
             * (fd 2 e' gia' dup2-ato li' sopra, e write e' async-signal-safe)
             * o il rimando indica l'output del lotto precedente. */
            static const char failMessage[] = "combat-lab: execv bin/melting-gen fallita (binario assente o non eseguibile?)\n";
            ssize_t ignored = write(STDERR_FILENO, failMessage, sizeof failMessage - 1);
            (void)ignored;
        }
        _exit(127);   /* requisito 2: execv fallito -> uscita 127, mai un return dal figlio */
    }

    gen->pid = pid;
    gen->kind = kind;
    if (!gen->loggedPathOnce)
    {
        /* Riga col percorso del log (requisito 2): compare una volta sola,
         * poi il messaggio "in corso" resta compatto. */
        snprintf(gen->statusText, sizeof gen->statusText, "GEN: %s in corso... (output -> %s)",
                 kindText, DEMO_GEN_LOG_PATH);
        gen->loggedPathOnce = true;
    }
    else
    {
        snprintf(gen->statusText, sizeof gen->statusText, "GEN: %s in corso...", kindText);
    }
    gen->statusTimer = -1.0f;   /* permanente: resta finche' DemoGenPollTick non lo sostituisce */
    return true;
}

/* Poll UNA VOLTA PER FRAME (requisito 3: "fuori dal loop a passo fisso, come
 * l'input"): WNOHANG non blocca mai, quindi e' sicuro chiamarla anche quando
 * pid==0. Quando il figlio e' morto lo stato uscita si raccoglie SEMPRE:
 * niente zombie (requisito 3, "waitpid raccoglie sempre"). */
static void DemoGenPollTick(DemoGenState *gen, DemoPool *enemyPool, DemoPool *weaponPool, DemoUiState *ui)
{
    if (gen->pid == 0) return;

    int status = 0;
    pid_t done = waitpid(gen->pid, &status, WNOHANG);
    if (done == 0) return;   /* ancora vivo */
    if (done < 0)
    {
        /* waitpid -1 (es. ECHILD se qualcosa ha gia' raccolto il figlio):
         * trattarlo come "ancora vivo" bloccherebbe G/H per sempre. L'esito
         * del lotto e' perso ma i file no (tmp+rename): la scansione sotto
         * raccoglie comunque cio' che e' stato scritto. */
        snprintf(gen->statusText, sizeof gen->statusText,
                 "GEN: esito perso (waitpid fallita), pool riscansionato");
        DemoPoolScanGenerated(enemyPool, DEMO_ENEMY_GENERATED_DIR);
        DemoPoolScanGenerated(weaponPool, DEMO_WEAPON_GENERATED_DIR);
        gen->statusTimer = DEMO_GEN_STATUS_SECONDS;
        gen->pid = 0;
        return;
    }

    const char *kindText = (gen->kind == DEMO_GEN_KIND_ENEMY) ? "enemy" : "weapon";
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        snprintf(gen->statusText, sizeof gen->statusText, "GEN: lotto %s pronto", kindText);
        /* Scansione extra SUBITO (requisito 3): senza questo il file nuovo
         * aspetterebbe il prossimo tick di DemoPoolPollTick (~1s) invece di
         * comparire nello stesso frame in cui il figlio ha finito. newCount
         * (v3, banner "atterra una generazione") e' quanti file lo scan ha
         * aggiunto in QUESTO poll, indipendente dal cambio di slot attivo. */
        DemoPool *targetPool = (gen->kind == DEMO_GEN_KIND_ENEMY) ? enemyPool : weaponPool;
        int beforeCount = targetPool->count;
        DemoPoolScanGenerated(enemyPool, DEMO_ENEMY_GENERATED_DIR);
        DemoPoolScanGenerated(weaponPool, DEMO_WEAPON_GENERATED_DIR);
        DemoUiAnnounceGenReady(ui, gen->kind, targetPool->count - beforeCount,
                               targetPool->count >= DEMO_POOL_MAX_ENTRIES);
    }
    else
    {
        int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        snprintf(gen->statusText, sizeof gen->statusText,
                 "GEN: fallita (exit %d), vedi %s", exitCode, DEMO_GEN_LOG_PATH);
    }
    gen->statusTimer = DEMO_GEN_STATUS_SECONDS;   /* da qui in poi il messaggio e' a tempo */
    gen->pid = 0;
}

/* Decadimento del messaggio a tempo (chiamata una volta per frame, come
 * DemoGenPollTick): statusTimer<=0 e' il sentinel "permanente" (in corso /
 * gia' in corso / idle), quindi qui non scende mai sotto zero da solo. */
static void DemoGenDecayStatus(DemoGenState *gen, float frameTime)
{
    if (gen->statusTimer <= 0.0f) return;
    gen->statusTimer -= frameTime;
    if (gen->statusTimer <= 0.0f) DemoGenResetStatus(gen);
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
    /* v3: sprite stabile per script (mai piu' un ciclo ad ogni respawn --
     * playtest 08/08, "non si capisce quando cambia il nemico") e budget
     * fresco per il pattern che comincia ORA, mai a meta' consumato da quello
     * precedente (unico punto di ricarica: copre spawn iniziale, respawn dopo
     * la morte, N/SHIFT+N, e l'avanzamento automatico della cattura headless).
     *
     * Lo sprite si sceglie con l'INDICE della voce nel pool, non con un hash
     * del nome (correzione 08/08: djb2 & 3 mandava spider_arc,
     * snail_calligrapher e squid_reload sullo stesso sprite -- due nemici
     * curati su tre con la stessa faccia, e premere N non si percepiva). Con
     * l'indice, voci ADIACENTI hanno per costruzione sprite diversi, che e' la
     * proprieta' che serve davvero mentre si scorre il pool; resta stabile per
     * script perche' l'ordine del pool e' deterministico (curati nell'ordine
     * cablato, generati appesi in coda in ordine alfabetico e mai riordinati,
     * vedi DemoPoolScanGenerated). */
    world->enemy.spriteKind = enemyPool->current & 3;
    DemoThrottleInit(&world->enemyThrottle, DEMO_ENEMY_SHOT_RATE_PER_SEC);
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

/* Colpi vivi in QUESTO momento con lo stesso flag hostile (la codifica di
 * "proprietario" gia' usata in tutto il file: hostile=true puo' ferire il
 * player -- nemico -- hostile=false puo' ferire il nemico -- player). Costa
 * una scansione dei 640 slot: chiamata solo dall'HUD (una volta a frame) e da
 * DemoNewProjectile (una volta per colpo spawnato, al massimo una manciata a
 * tick), mai in un punto caldo. */
static int DemoCountActiveShots(const DemoWorld *world, bool hostile)
{
    int count = 0;
    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
        if (world->projectiles[i].active && world->projectiles[i].hostile == hostile) count++;
    return count;
}

/* Punto unico di allocazione di un proiettile (spawn diretto, ring, orbit,
 * mezzaluna, echi, pistola base: tutti passano da qui): v3 applica qui,
 * SENZA eccezioni, il cap di colpi attivi e il token bucket proiettili/s del
 * proprietario "hostile". Un rifiuto per cap o per bucket e' identico dal
 * punto di vista del chiamante (ritorna NULL, come quando lo slot pool e'
 * pieno): non e' un errore, e' un troncamento silenzioso -- lo stesso
 * contatore "scartati" dell'HUD copre entrambi i casi, il proprietario non ha
 * bisogno di sapere QUALE dei due tetti ha fermato il colpo, solo che Gemma
 * sta sforando. */
static DemoProjectile *DemoNewProjectile(DemoWorld *world, bool hostile)
{
    DemoThrottleState *throttle = hostile ? &world->enemyThrottle : &world->weaponThrottle;
    int cap = hostile ? DEMO_ENEMY_SHOT_CAP : DEMO_WEAPON_SHOT_CAP;

    if (DemoCountActiveShots(world, hostile) >= cap)
    {
        throttle->throttledWindowCount++;
        return NULL;
    }
    if (throttle->shotBucket < 1.0f)
    {
        throttle->throttledWindowCount++;
        return NULL;
    }

    for (int i = 0; i < DEMO_MAX_PROJECTILES; i++)
    {
        if (!world->projectiles[i].active)
        {
            memset(&world->projectiles[i], 0, sizeof world->projectiles[i]);
            world->projectiles[i].active = true;
            throttle->shotBucket -= 1.0f;
            return &world->projectiles[i];
        }
    }
    return NULL;   /* pool globale (640 slot) esaurito: non e' un budget v3, resta inalterato */
}

/* Quanti colpi di un burst CIRCOLARE possono davvero nascere in questo istante
 * (cap dei colpi attivi + token bucket), contando subito come "scartati" quelli
 * che non nasceranno. Serve perche' un cerchio troncato riempiendo gli indici
 * dal primo in poi degenera in un ventaglio da un lato -- chiedendo prima
 * quanti ne passano, il burst si ridistribuisce sull'intero giro e un ring
 * troncato resta un ring, solo piu' rado. */
static int DemoShotAllowance(DemoWorld *world, bool hostile, int wanted)
{
    DemoThrottleState *throttle = hostile ? &world->enemyThrottle : &world->weaponThrottle;
    int cap = hostile ? DEMO_ENEMY_SHOT_CAP : DEMO_WEAPON_SHOT_CAP;
    int room = cap - DemoCountActiveShots(world, hostile);
    int tokens = (int)floorf(throttle->shotBucket);
    int allowed = wanted;
    if (allowed > room) allowed = room;
    if (allowed > tokens) allowed = tokens;
    if (allowed < 0) allowed = 0;
    throttle->throttledWindowCount += wanted - allowed;
    return allowed;
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
    DemoProjectile *shot = DemoNewProjectile(world, hostile);
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
    /* Come il ring: si chiede PRIMA quanti colpi passano, poi si distribuiscono
     * su tutto il giro (una corona rada resta una corona, un troncamento a
     * meta' del ciclo lascerebbe una mezzaluna di orbitanti). */
    int count = DemoShotAllowance(world, hostile, command->count);
    for (int i = 0; i < count; i++)
    {
        DemoProjectile *shot = DemoNewProjectile(world, hostile);
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
    int count = DemoShotAllowance(world, hostile, command->count);
    for (int i = 0; i < count; i++)
    {
        float angle = phase + DEMO_TAU*(float)i/(float)count;
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
 * decide se un arco/proiettile puo' ferire il player invece del nemico. E'
 * anche la chiave del budget v3 (throttle): "nemico" e "arma" hanno bucket e
 * cooldown separati, scelti qui in base a QUALE sandbox ha emesso il comando
 * -- non in base a chi il colpo risultante puo' ferire (quella e' la scelta
 * di DemoNewProjectile, sul flag hostile del singolo colpo: per RELEASE_ECHOES
 * i due possono divergere, vedi il commento li' sotto). */
static void DemoConsumeCommands(DemoWorld *world, DemoScriptRuntime *runtime, bool hostile,
                                int *lastCommandCount)
{
    const DemoScriptCommand *commands = DemoScriptApiCommands(&runtime->api);
    size_t commandCount = DemoScriptApiCommandCount(&runtime->api);
    *lastCommandCount = (int)commandCount;

    DemoThrottleState *throttle = hostile ? &world->enemyThrottle : &world->weaponThrottle;
    float tickTime = world->globalTime;

    for (size_t i = 0; i < commandCount; i++)
    {
        const DemoScriptCommand *command = &commands[i];
        switch (command->type)
        {
            case DEMO_CMD_TELEGRAPH_ARC:
            case DEMO_CMD_EMIT_ARC:
            case DEMO_CMD_MELEE_SWEEP:
            {
                /* v3: cooldown solo su MELEE_SWEEP (il vero colpo corpo a
                 * corpo). telegraph_arc non paga NESSUN budget per principio
                 * (non spawna e non ferisce, vedi il blocco #define);
                 * emit_arc non ha categoria propria -- nessuno dei curati ne
                 * abusa, e i colpi che genera (mezzaluna) sono gia' limitati
                 * dal bucket proiettili/s in DemoNewProjectile. */
                if (command->type == DEMO_CMD_MELEE_SWEEP &&
                    !DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_MELEE_SWEEP, tickTime,
                                               DEMO_COOLDOWN_MELEE_SWEEP_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
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
                if (command->type == DEMO_CMD_MELEE_SWEEP)
                {
                    if (!hostile) DemoApplyMelee(world, command);
                    DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_MELEE_SWEEP);
                }
            } break;

            case DEMO_CMD_EMIT_RING:
                if (!DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_RING, tickTime, DEMO_COOLDOWN_RING_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
                DemoSpawnRing(world, command, hostile);
                DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_RING);
                break;

            case DEMO_CMD_EMIT_ORBIT:
                /* l'orbit conta come ring (stessa categoria di cooldown v3):
                 * uno script che alterna ring/orbit per aggirare un
                 * cooldown-per-tipo non guadagna nulla. */
                if (!DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_RING, tickTime, DEMO_COOLDOWN_RING_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
                DemoSpawnOrbit(world, command, hostile);
                DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_RING);
                break;

            case DEMO_CMD_TELEGRAPH_BEAM:
            case DEMO_CMD_EMIT_BEAM:
            {
                /* Solo emit_beam paga il cooldown: il telegrafo e' un disegno,
                 * non un'emissione (vedi il blocco #define sui budget). */
                bool isRealBeam = command->type == DEMO_CMD_EMIT_BEAM;
                if (isRealBeam &&
                    !DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_BEAM, tickTime, DEMO_COOLDOWN_BEAM_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
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
                if (isRealBeam) DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_BEAM);
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
                if (!DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_CAPTURE_RADIUS, tickTime,
                                               DEMO_COOLDOWN_CAPTURE_RADIUS_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
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
                DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_CAPTURE_RADIUS);
            } break;

            case DEMO_CMD_RELEASE_ECHOES:
            {
                /* Stessa categoria RING del bucket cooldown, sul lato che ha
                 * EMESSO il comando (throttle/hostile qui sopra). I colpi
                 * ereditano lo stesso 'hostile' (correzione 08/08: prima
                 * uscivano sempre con hostile=false, cioe' un nemico che
                 * chiamava release_echoes sparava colpi del PLAYER e li faceva
                 * pagare al budget dell'ARMA -- il gate diceva "nemico", lo
                 * spawn diceva "arma", e il pannello ARMA vedeva salire gli
                 * scartati per colpa del nemico).
                 * La riserva di echi e' del player: la riempie solo un
                 * capture_radius dell'ARMA (DemoUpdateCaptureFields) e la
                 * consuma solo un release_echoes dell'arma. Un nemico non la
                 * legge e non la azzera: spara il suo cono e basta. */
                if (!DemoThrottleCooldownReady(throttle, DEMO_COOLDOWN_RING, tickTime, DEMO_COOLDOWN_RING_SECONDS))
                {
                    throttle->throttledWindowCount++;
                    break;
                }
                int available = hostile ? 0 : world->storedEchoes;
                /* Il nemico non ha riserva, quindi il suo release_echoes vale
                 * per quello che e' -- il ventaglio dichiarato dallo script.
                 * Farlo cadere a un colpo solo lo renderebbe un dud silenzioso
                 * (i prompt lo elencano fra gli attacchi veri di un nemico e il
                 * validatore lo accetta), lo stesso difetto del cooldown beam
                 * condiviso col telegrafo. L'ARMA invece scarica la riserva
                 * accumulata con capture_radius: un eco per colpo, uno solo se
                 * non ha catturato niente. */
                int releaseCount = hostile ? command->count
                                           : (available > 0 ? available : 1);
                if (releaseCount > command->count) releaseCount = command->count;
                for (int shot = 0; shot < releaseCount; shot++)
                {
                    float t = releaseCount > 1 ? (float)shot/(float)(releaseCount - 1) : 0.5f;
                    float angle = command->angle + (t - 0.5f)*command->spread;
                    DemoSpawnShot(world, (Vector2){ command->x, command->y }, angle,
                                  command->speed, 7.0f + 0.7f*(float)available,
                                  command->damage + 0.5f*(float)available,
                                  command->life, command->visualId, hostile);
                }
                if (!hostile) world->storedEchoes = 0;
                DemoThrottleMarkCooldownUsed(throttle, DEMO_COOLDOWN_RING);
            } break;
        }
    }

    DemoThrottleCommitTick(throttle, tickTime);
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
    /* spriteKind non si cicla piu' qui: DemoEnemyBeginPattern (dentro
     * DemoEnemySpawn) lo deriva dal nome del nuovo script, v3. */
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
                /* Solo un campo dell'ARMA alimenta la riserva di echi: e' una
                 * risorsa del player, e release_echoes di un nemico non la
                 * legge (vedi DEMO_CMD_RELEASE_ECHOES). Un campo nemico
                 * assorbe comunque il colpo e conta fra i "catturati". */
                if (field->playerOwned) world->storedEchoes++;
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

/* Punto unico per (ri)equipaggiare l'arma allo slot weaponPool->current
 * (0 = pistola base): usato da R/scadenza KO (DemoResetArena, announce=false,
 * "stessi script") e dai tasti M/SHIFT+M in main() (announce=true, "il
 * proprietario ha appena cambiato arma"). v3: azzera SEMPRE il budget
 * dell'arma qui, cosi' un'arma appena equipaggiata (o la pistola base appena
 * ripristinata) parte con un tetto fresco, mai a meta' consumato da quella
 * precedente. ui puo' essere NULL quando announce e' false (DemoResetArena
 * non ha e non deve toccare lo stato del banner). */
static void DemoWeaponEquip(DemoWorld *world, const DemoPool *weaponPool, DemoUiState *ui, bool announce)
{
    DemoThrottleInit(&world->weaponThrottle, DEMO_WEAPON_SHOT_RATE_PER_SEC);
    if (weaponPool->current == 0)
        DemoScriptUnload(&world->weaponScript);
    else
        DemoScriptLoad(&world->weaponScript, &weaponPool->entries[weaponPool->current - 1],
                       DEMO_WEAPON_GENERATED_DIR, true,
                       0x77A1u + (unsigned int)weaponPool->current*53u);
    if (announce && ui != NULL) DemoUiAnnounceWeapon(ui, weaponPool);
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
    DemoWeaponEquip(world, weaponPool, NULL, false);
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

    if (world->player.dead)
    {
        world->player.koTimer -= dt;
        if (world->player.koTimer <= 0.0f) DemoResetArena(world, enemyPool, weaponPool);
        return;
    }

    world->player.invulnerability = fmaxf(0.0f, world->player.invulnerability - dt);
    world->player.statusTime = fmaxf(0.0f, world->player.statusTime - dt);

    /* v3: ricarica dei due bucket proiettili/s una volta per tick fisso,
     * PRIMA che nemico/arma emettano comandi in questo stesso tick -- mai
     * dentro DemoConsumeCommands, che gira una volta a testa e non deve
     * ricaricare due volte lo stesso secondo. */
    DemoThrottleRefill(&world->enemyThrottle, DEMO_ENEMY_SHOT_RATE_PER_SEC, dt);
    DemoThrottleRefill(&world->weaponThrottle, DEMO_WEAPON_SHOT_RATE_PER_SEC, dt);

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

/* Rete di sicurezza v3 (playtest 08/08: "a volte le entita' sembra che non si
 * vedano"): un asset stock mancante o corrotto non risulta MAI in un disegno
 * vuoto. Il rettangolo magenta e' deliberatamente vistoso -- nessuno lo
 * scambia per uno sprite vero -- cosi' il problema si nota subito invece di
 * passare per un bug di gameplay. Il caso "texture assente" e' gia' comune
 * (DemoLoadAssets non blocca piu' l'avvio su un singolo asset rotto, vedi
 * main()): questo e' il punto che lo rende innocuo a runtime. */
static void DemoDrawTextureOrPlaceholder(DemoUiState *ui, Texture2D texture, Vector2 position,
                                         float width, float height, float rotation, Color tint)
{
    if (DemoTextureValid(texture))
    {
        DemoDrawTextureCentered(texture, position, width, height, rotation, tint);
        return;
    }
    Rectangle rect = { position.x, position.y, width, height };
    Vector2 origin = { width*0.5f, height*0.5f };
    DrawRectanglePro(rect, origin, rotation, (Color){ 255, 0, 255, 220 });
    DrawRectangleLinesEx((Rectangle){ position.x - width*0.5f, position.y - height*0.5f, width, height },
                         2.0f, WHITE);
    ui->assetWarningShown = true;  /* sticky: l'HUD lo mostra da qui in poi (vedi DemoDrawWorldHud) */
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
    /* v3 (playtest 08/08, "a volte le entita' sembra che non si vedano"):
     * raggio e alpha minimi di DISEGNO, mai quelli fisici -- un colpo generato
     * con radius/damage minuscoli (range valido ma spinto al limite basso dal
     * prompt) resta comunque leggibile. shot->radius (la hitbox vera) non
     * cambia: solo cio' che finisce sullo schermo. */
    float drawRadius = fmaxf(3.0f, shot->radius);
    float lifeAlpha = fmaxf(0.55f, DemoClamp(shot->life*4.0f, 0.0f, 1.0f));

    for (int i = shot->trailCount - 1; i > 0; i--)
    {
        float alpha = (float)(shot->trailCount - i)/(float)shot->trailCount;
        float width = mode == DEMO_RENDER_PIXEL ? fmaxf(1.0f, drawRadius*0.55f)
                                                 : fmaxf(1.0f, drawRadius*(0.35f + alpha));
        DrawLineEx(shot->trail[i], shot->trail[i - 1], width, Fade(color, 0.06f + alpha*0.22f));
    }

    if (mode != DEMO_RENDER_PIXEL)
    {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(shot->position, drawRadius*3.4f,
                           Fade(color, 0.21f*lifeAlpha), BLANK);
        EndBlendMode();
    }

    if (shot->visualId == DEMO_VIS_VIOLET_CUT)
    {
        float degrees = shot->rotation*RAD2DEG;
        DrawRing(shot->position, drawRadius*0.85f, drawRadius*1.8f,
                 degrees - 72.0f, degrees + 72.0f,
                 mode == DEMO_RENDER_PIXEL ? 8 : 20, Fade(color, lifeAlpha));
        DrawRingLines(shot->position, drawRadius*0.85f, drawRadius*1.8f,
                      degrees - 72.0f, degrees + 72.0f,
                      mode == DEMO_RENDER_PIXEL ? 8 : 20, Fade(WHITE, lifeAlpha*0.78f));
    }
    else if (shot->visualId == DEMO_VIS_GLASS_PRISM)
    {
        Vector2 forward = DemoDirection(shot->rotation);
        Vector2 side = { -forward.y, forward.x };
        Vector2 a = DemoAddScaled(shot->position, forward, drawRadius*1.7f);
        Vector2 b = DemoAddScaled(shot->position, side, drawRadius);
        Vector2 c = DemoAddScaled(shot->position, side, -drawRadius);
        DrawTriangle(a, b, c, Fade(color, lifeAlpha));
        DrawTriangleLines(a, b, c, WHITE);
    }
    else if (shot->visualId == DEMO_VIS_CALLIGRAPHY_INK)
    {
        DrawCircleV(shot->position, drawRadius*1.2f, Fade(color, lifeAlpha));
        DrawCircleV((Vector2){ shot->position.x - drawRadius*0.45f, shot->position.y - drawRadius*0.4f },
                    drawRadius*0.45f, Fade((Color){ 8, 20, 28, 255 }, lifeAlpha));
    }
    else
    {
        if (mode == DEMO_RENDER_PIXEL)
            DrawRectangle((int)(shot->position.x - drawRadius), (int)(shot->position.y - drawRadius),
                          (int)fmaxf(2.0f, drawRadius*2.0f), (int)fmaxf(2.0f, drawRadius*2.0f),
                          Fade(color, lifeAlpha));
        else DrawCircleV(shot->position, drawRadius, Fade(color, lifeAlpha));
        DrawCircleV(shot->position, fmaxf(1.5f, drawRadius*0.34f), WHITE);
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
 * aperto. Lo sprite e' solo cosmetico, stabile per script (v3: hash del nome
 * file in DemoEnemyBeginPattern, non piu' ciclato ad ogni respawn). */
static void DemoDrawEnemy(DemoUiState *ui, const DemoAssets *assets, const DemoWorld *world)
{
    const DemoEnemy *enemy = &world->enemy;
    if (!enemy->alive && enemy->corpseFade <= 0.0f) return;

    float bob = enemy->alive ? sinf(world->globalTime*4.2f)*3.0f : 0.0f;
    Vector2 position = { enemy->position.x, enemy->position.y + bob };
    float fadeAlpha = enemy->alive ? 1.0f
                                   : DemoClamp(enemy->corpseFade/DEMO_ENEMY_CORPSE_FADE_SECONDS, 0.0f, 1.0f);
    Color tint = enemy->hitFlash > 0.0f ? WHITE : Fade((Color){ 248, 248, 255, 255 }, fadeAlpha);

    DrawEllipse((int)position.x, (int)(position.y + 28.0f), 38.0f, 11.0f, Fade(BLACK, 0.42f*fadeAlpha));
    /* v3 (playtest 08/08, "il nemico a volte sembra sparire"): durante il
     * fade del cadavere il contorno resta ad alpha minima invece di svanire
     * insieme allo sprite, stesso principio del pavimento minimo sui
     * proiettili in DemoDrawProjectile. */
    if (!enemy->alive)
        DrawCircleLines((int)position.x, (int)position.y, 30.0f, Fade(BLACK, fmaxf(0.30f, fadeAlpha)));
    DemoDrawTextureOrPlaceholder(ui, DemoEnemyTexture(assets, enemy->spriteKind), position, 56.0f, 56.0f, 0.0f, tint);

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
static void DemoDrawPlayerWeapon(DemoUiState *ui, const DemoAssets *assets, const DemoWorld *world,
                                 bool baseWeaponActive)
{
    if (!baseWeaponActive) return;
    float degrees = world->player.aimAngle*RAD2DEG;
    Vector2 gunPosition = DemoAddScaled(world->player.position, DemoDirection(world->player.aimAngle), 24.0f);
    DemoDrawTextureOrPlaceholder(ui, assets->handgun, gunPosition, 58.0f, 58.0f, degrees, (Color){ 198, 214, 222, 255 });
}

static void DemoDrawPlayer(DemoUiState *ui, const DemoAssets *assets, const DemoWorld *world, bool baseWeaponActive)
{
    Vector2 position = world->player.position;
    float bob = sinf(world->globalTime*8.0f)*1.5f;
    Color tint = (world->player.invulnerability > 0.0f && fmodf(world->globalTime, 0.10f) < 0.05f)
                     ? Fade(WHITE, 0.35f) : WHITE;
    DrawEllipse((int)position.x, (int)(position.y + 19.0f), 17.0f, 6.0f, Fade(BLACK, 0.44f));
    DemoDrawPlayerWeapon(ui, assets, world, baseWeaponActive);
    DemoDrawTextureOrPlaceholder(ui, assets->player, (Vector2){ position.x, position.y + bob },
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

/* Spinner testuale della riga GEN: v3, "indicatore generazione in corso ben
 * visibile" (playtest 08/08: la riga passava quasi inosservata). GetTime() e'
 * il tempo reale del processo, non world->globalTime: e' puramente cosmetico
 * e va avanti anche in pausa, che e' proprio il momento in cui il
 * proprietario guarda l'HUD invece dell'azione. */
static const char *DemoGenSpinnerFrame(void)
{
    static const char *frames[4] = { "|", "/", "-", "\\" };
    return frames[((int)(GetTime()*8.0)) & 3];
}

/* Un pannello di debug (nemico o arma): nome script, fonte con tag colorato,
 * seed se generato, posizione nel pool, stato Lua, contatori di budget v3.
 * Stessa forma per i due lati (playtest 08/08: "non si capisce cosa si sta
 * guardando") -- differiscono solo nei valori passati dal chiamante.
 *
 * Tutte le righe stanno DENTRO la banda alta DEMO_HUD_TOP_HEIGHT (= DEMO_ROOM.y),
 * l'ultima chiude a y=85: il debug non puo' invadere l'arena, dove il nemico
 * nasce col bordo alto a y~103 e la barra vita a y~91 (correzione 08/08: la
 * barra opaca da 130px tagliava il nemico e nascondeva del tutto la sua barra
 * vita per i primi secondi). Chi aggiunge una riga qui accorcia le altre, non
 * la banda. */
static void DemoDrawEntityPanel(float x, const char *ownerLabel, int poolPos, int poolCount,
                                bool hasSource, const char *scriptName, DemoPoolSource source, unsigned int seed,
                                bool scriptExists, bool scriptReady, const char *notReadyLabel,
                                int cmdPerTick, const char *errorText,
                                int activeShots, int shotCap, int throttledPerSecond)
{
    Color tagColor = hasSource ? DemoSourceTagColor(source) : DEMO_TAG_BASE_COLOR;
    const char *tagLabel = hasSource ? DemoSourceTagLabel(source) : "BASE";

    DrawText(TextFormat("%s %d/%d", ownerLabel, poolPos, poolCount), (int)x, 4, 17, (Color){ 239, 244, 252, 255 });

    char tagText[16];
    snprintf(tagText, sizeof tagText, "[%s]", tagLabel);
    DrawText(tagText, (int)x, 24, 14, tagColor);
    int tagWidth = MeasureText(tagText, 14);
    if (hasSource && source == DEMO_POOL_SOURCE_GENERATED)
        DrawText(TextFormat("%s  seed %u", scriptName, seed), (int)x + tagWidth + 8, 24, 14, (Color){ 210, 218, 232, 255 });
    else
        DrawText(scriptName, (int)x + tagWidth + 8, 24, 14, (Color){ 210, 218, 232, 255 });

    if (!scriptExists)
        DrawText("nessuna sandbox", (int)x, 42, 13, (Color){ 151, 166, 190, 255 });
    else
    {
        DrawText(TextFormat("Lua: %s  cmd/tick %d", scriptReady ? "ON" : notReadyLabel, cmdPerTick),
                 (int)x, 42, 13, scriptReady ? (Color){ 118, 255, 178, 255 } : (Color){ 255, 104, 116, 255 });
        if (!scriptReady && errorText != NULL && errorText[0] != '\0')
            DrawText(errorText, (int)x, 58, 11, (Color){ 255, 158, 165, 255 });
    }

    /* Contatori di budget v3 (requisito HUD: "il proprietario VEDE quando
     * Gemma sfora i budget"): colore neutro finche' scartati/s resta a zero,
     * arancio (lo stesso di [GEMMA]) appena il token bucket o un cooldown
     * comincia a tagliare qualcosa. */
    Color throttleColor = throttledPerSecond > 0 ? DEMO_TAG_GENERATED_COLOR : (Color){ 188, 201, 222, 255 };
    DrawText(TextFormat("colpi %d/%d   scartati: %d/s", activeShots, shotCap, throttledPerSecond),
             (int)x, 72, 13, throttleColor);
}

/* Banner grande temporaneo (v3, requisito HUD "il cambio arma/nemico e' poco
 * chiaro"), colore coerente col tag dell'evento annunciato
 * (DemoUiAnnounceEnemy/Weapon/GenReady). Vive nella BANDA BASSA, come una
 * striscia a tutta larghezza che copre la legenda dei tasti finche' e' visibile
 * (correzione 08/08: prima stava a y=140, cioe' esattamente sopra il nemico
 * appena comparso -- copriva la cosa che annunciava). La legenda e' l'unica
 * informazione che il proprietario puo' perdere per 2.5s senza danno: e' fissa
 * e la sa gia' a memoria dopo il primo minuto. Disegnata SEMPRE (anche in
 * split/cattura): bannerTimer resta a 0 se nessuno l'ha mai innescata, quindi
 * e' innocua li' dove non serve. */
static void DemoDrawUiBanner(const DemoUiState *ui)
{
    if (ui->bannerTimer <= 0.0f) return;
    float alpha = fminf(1.0f, ui->bannerTimer/0.4f);   /* dissolvenza solo negli ultimi 0.4s */
    int size = 20;
    int width = MeasureText(ui->bannerText, size);
    int x = (DEMO_WIDTH - width)/2;
    if (x < 8) x = 8;
    DrawRectangle(0, DEMO_HUD_BANNER_Y, DEMO_WIDTH, DEMO_HEIGHT - DEMO_HUD_BANNER_Y,
                  Fade((Color){ 6, 9, 16, 245 }, alpha));
    DrawRectangle(0, DEMO_HUD_BANNER_Y, DEMO_WIDTH, 2, Fade(ui->bannerColor, alpha));
    DrawText(ui->bannerText, x, DEMO_HUD_BANNER_Y + 4, size, Fade(ui->bannerColor, alpha));
}

static void DemoDrawWorldHud(const DemoWorld *world, const DemoPool *enemyPool,
                             const DemoPool *weaponPool, DemoRenderMode mode,
                             const DemoGenState *gen, const DemoUiState *ui, bool interactive)
{
    Color accent = (Color){ 126, 224, 255, 255 };
    /* Banda alta: esattamente DEMO_HUD_TOP_HEIGHT, mai un pixel dentro l'arena
     * (vedi il commento sul blocco di #define). Ci stanno le 4-5 righe dei due
     * pannelli, compattate; la riga GEN e l'avviso asset sono andati altrove
     * proprio per non farla crescere. */
    DrawRectangle(0, 0, DEMO_WIDTH, DEMO_HUD_TOP_HEIGHT, (Color){ 7, 10, 18, 255 });
    DrawRectangle(0, DEMO_HUD_TOP_HEIGHT - 2, DEMO_WIDTH, 2, accent);

    char enemyName[DEMO_POOL_NAME_MAX];
    if (enemyPool->count > 0)
        DemoStripLuaExtension(enemyName, sizeof enemyName, enemyPool->entries[enemyPool->current].fileName);
    else
        snprintf(enemyName, sizeof enemyName, "--");
    DemoDrawEntityPanel(24.0f, "NEMICO", enemyPool->current + 1, enemyPool->count,
                        enemyPool->count > 0, enemyName,
                        enemyPool->count > 0 ? enemyPool->entries[enemyPool->current].source : DEMO_POOL_SOURCE_CURATED,
                        enemyPool->count > 0 ? DemoPoolEntrySeed(&enemyPool->entries[enemyPool->current]) : 0,
                        true, world->enemyScript.ready, "KO -> nemico inerte", world->lastEnemyCommandCount,
                        world->enemyScript.error, DemoCountActiveShots(world, true), DEMO_ENEMY_SHOT_CAP,
                        world->enemyThrottle.throttledLastSecond);

    bool weaponHasSource = weaponPool->current != 0;
    char weaponName[DEMO_POOL_NAME_MAX];
    DemoPoolSource weaponSource = DEMO_POOL_SOURCE_CURATED;
    unsigned int weaponSeed = 0;
    if (weaponHasSource)
    {
        const DemoPoolEntry *entry = &weaponPool->entries[weaponPool->current - 1];
        DemoStripLuaExtension(weaponName, sizeof weaponName, entry->fileName);
        weaponSource = entry->source;
        weaponSeed = DemoPoolEntrySeed(entry);
    }
    else snprintf(weaponName, sizeof weaponName, "pistola base");
    DemoDrawEntityPanel(660.0f, "ARMA", weaponPool->current, weaponPool->count,
                        weaponHasSource, weaponName, weaponSource, weaponSeed,
                        weaponHasSource, world->weaponScript.ready, "FALLBACK -> pistola base",
                        world->lastWeaponCommandCount, world->weaponScript.error,
                        DemoCountActiveShots(world, false), DEMO_WEAPON_SHOT_CAP,
                        world->weaponThrottle.throttledLastSecond);

    /* Avviso asset: allineato a destra sulla PRIMA riga della banda alta, dove
     * i due pannelli hanno solo il titolo corto ("NEMICO 1/3", "ARMA 0/2"), e
     * abbreviato -- il messaggio per esteso resta su stderr all'avvio. Prima
     * stava a (420,108), esattamente addosso alla riga GEN a (24,108). */
    if (ui->assetWarningShown)
    {
        const char *warning = "ASSET STOCK KO: placeholder magenta (vedi stderr all'avvio)";
        DrawText(warning, DEMO_WIDTH - MeasureText(warning, 12) - 16, 6, 12,
                 (Color){ 255, 158, 165, 255 });
    }

    /* Banda bassa: contatori, riga GEN e legenda, tutto sotto il bordo
     * inferiore dell'arena (y=654). */
    DrawRectangle(0, DEMO_HUD_BOTTOM_Y - 2, DEMO_WIDTH, 2, accent);
    DrawRectangle(0, DEMO_HUD_BOTTOM_Y, DEMO_WIDTH, DEMO_HEIGHT - DEMO_HUD_BOTTOM_Y,
                  (Color){ 7, 10, 18, 245 });

    /* Riga GEN: sempre lo stato vero, anche sotto --capture -- un PNG dello
     * smoke test che dichiara una UI diversa da quella del gioco non e' piu'
     * una regressione, e' una bugia (chiusura WP5 del mustFix del giudice
     * WP4). Il determinismo della cattura non dipende da questa riga ma dallo
     * stato: in capture nessun figlio parte mai (gen->pid resta 0, quindi lo
     * spinner/pulsazione v3 sotto non scatta mai li') e il brief viene
     * forzato off all'avvio (vedi main()), cosi' il testo non varia col
     * contenuto di generated/combat-lab/brief.txt sul disco. */
    Color genColor = (Color){ 151, 166, 190, 255 };
    /* statusText (DEMO_GEN_STATUS_TEXT_MAX=192) + " | brief: " + briefLine
     * (DEMO_GEN_BRIEF_LINE_MAX=72) + spinner: 300 lascia margine reale,
     * niente piu' -Wformat-truncation. */
    char genLine[300];
    if (gen->pid != 0)
    {
        float pulse = 0.55f + 0.45f*sinf((float)GetTime()*10.0f);
        genColor = Fade(DEMO_TAG_GENERATED_COLOR, pulse);
        snprintf(genLine, sizeof genLine, "%s %s | brief: %s", DemoGenSpinnerFrame(), gen->statusText,
                 DemoGenBriefLabel(gen));
    }
    else snprintf(genLine, sizeof genLine, "%s | brief: %s", gen->statusText, DemoGenBriefLabel(gen));

    /* Prima riga della banda bassa: vita del player e contatori globali. */
    DrawRectangle(22, 662, 156, 12, (Color){ 29, 35, 49, 255 });
    DrawRectangle(24, 664, (int)(152.0f*DemoClamp(world->player.hp/DEMO_PLAYER_MAX_HP, 0.0f, 1.0f)), 8,
                  (Color){ 255, 86, 108, 255 });
    DrawText(TextFormat("HP %.1f/%.0f", world->player.hp, DEMO_PLAYER_MAX_HP), 188, 663, 13,
             (Color){ 206, 216, 232, 255 });
    DrawText(TextFormat("proiettili %d", DemoActiveProjectileCount(world)), 290, 663, 13,
             (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("echi %d", world->storedEchoes), 420, 663, 13, accent);
    DrawText(TextFormat("hit %d", world->hitsDealt), 520, 663, 13, accent);
    DrawText(TextFormat("nemico HP %.0f/%.0f", world->enemy.hp, DEMO_ENEMY_MAX_HP), 610, 663, 13,
             (Color){ 188, 201, 222, 255 });
    DrawText(TextFormat("catturati %d", world->capturedTotal), 780, 663, 13,
             (Color){ 188, 201, 222, 255 });
    const char *renderName = mode == DEMO_RENDER_PIXEL ? "PIXEL" : mode == DEMO_RENDER_SMOOTH ? "SMOOTH" : "IBRIDO";
    DrawText(TextFormat("RENDER %s", renderName), 920, 662, 14, accent);

    /* Seconda riga: la riga GEN, da sola -- e' la piu' lunga di tutte
     * (statusText fino a 192 byte piu' il brief), quindi non condivide la riga
     * con nient'altro. */
    DrawText(genLine, 24, 678, 13, genColor);

    /* La terza riga della banda bassa (da DEMO_HUD_BANNER_Y in giu') e' la
     * legenda dei tasti, disegnata pero' sul frame COMPOSTO (DemoComposeFrame):
     * li' e' leggibile anche in split, dove questo HUD finisce rimpicciolito
     * dentro un pannello. E' anche l'unica cosa che il banner temporaneo puo'
     * coprire. */
    (void)interactive;
}

static void DemoDrawWorld(DemoUiState *ui, DemoAssets *assets, const DemoWorld *world, const DemoPool *enemyPool,
                          const DemoPool *weaponPool, DemoRenderMode mode,
                          const DemoGenState *gen, bool interactive)
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

    /* v3 (playtest 08/08, "player + handgun sopra gli effetti"): nemico e
     * player disegnati DOPO proiettili/archi/raggi/particelle, non prima, cosi'
     * non spariscono mai sotto un accumulo di colpi. Il nemico usa comunque
     * l'ombra/contorno con alpha minima durante il fade (DemoDrawEnemy) per
     * restare percepibile anche mentre il cadavere svanisce. */
    DemoDrawEnemy(ui, assets, world);
    DemoDrawPlayer(ui, assets, world, baseWeaponActive);

    DemoDrawHitboxLegend(world);
    DemoDrawKoOverlay(world);
    DemoDrawWorldHud(world, enemyPool, weaponPool, mode, gen, ui, interactive);
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

static void DemoRenderWorldToTarget(DemoUiState *ui, DemoAssets *assets, const DemoWorld *world,
                                    const DemoPool *enemyPool, const DemoPool *weaponPool, DemoRenderMode mode,
                                    RenderTexture2D target, const DemoGenState *gen, bool interactive)
{
    float scale = (float)target.texture.width/(float)DEMO_WIDTH;
    Camera2D camera = { 0 };
    camera.zoom = scale;
    BeginTextureMode(target);
    BeginMode2D(camera);
    DemoDrawWorld(ui, assets, world, enemyPool, weaponPool, mode, gen, interactive);
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

static void DemoComposeFrame(DemoRenderer *renderer, DemoUiState *ui, DemoAssets *assets, const DemoWorld *world,
                             const DemoPool *enemyPool, const DemoPool *weaponPool,
                             DemoRenderMode mode, bool split, bool showControls,
                             const DemoGenState *gen)
{
    /* showControls fa gia' esattamente la distinzione che serve qui
     * (false solo nella cattura headless, true nel gioco vero, vedi i due
     * punti di chiamata in main()): riusarla come "interactive" per la riga
     * GEN dentro DemoDrawWorldHud evita un secondo booleano ridondante. */
    if (split || mode == DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(ui, assets, world, enemyPool, weaponPool, DEMO_RENDER_PIXEL, renderer->pixelTarget,
                                gen, showControls);
    if (split || mode != DEMO_RENDER_PIXEL)
        DemoRenderWorldToTarget(ui, assets, world, enemyPool, weaponPool,
                                split ? DEMO_RENDER_HYBRID : mode, renderer->highTarget,
                                gen, showControls);

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

    /* Legenda dei tasti: unica copia, nella banda bassa del frame composto
     * (fuori dall'arena per costruzione, vedi i #define DEMO_HUD_*). Sta qui e
     * non dentro DemoDrawWorldHud perche' in split quell'HUD viene rimpicciolito
     * dentro i pannelli e la legenda diventerebbe illeggibile (e doppia). */
    if (showControls)
    {
        DrawRectangle(0, DEMO_HUD_BANNER_Y, DEMO_WIDTH, DEMO_HEIGHT - DEMO_HUD_BANNER_Y,
                      (Color){ 3, 5, 10, 232 });
        DrawText("WASD mouse | clic fuoco | N/M cambia . SHIFT+N/M indietro | G/H genera | R reset | B brief | 1/2/3 render | TAB split | SPAZIO pausa",
                 24, DEMO_HUD_BANNER_Y + 6, 12, (Color){ 178, 191, 213, 255 });
    }

    /* Banner v3 disegnato UNA volta sul frame composto, mai dentro
     * DemoDrawWorld: in split quel percorso gira due volte (pixel + ibrido) e
     * un banner per pannello sarebbe duplicato/disallineato. Va DOPO la
     * legenda: la copre apposta finche' e' vivo. bannerTimer resta a 0 nella
     * cattura headless (nessun tasto N/M/G/H li', vedi main()), quindi qui non
     * cambia mai un PNG dello smoke test. */
    DemoDrawUiBanner(ui);
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
    /* A fianco di world/enemyPool/weaponPool, non dentro DemoWorld (vedi il
     * commento su DemoGenState): un lotto in corso deve sopravvivere ai
     * memset di DemoResetArena. Stessa ragione per DemoUiState (banner,
     * avviso asset, v3): non e' gameplay, non deve sparire ad ogni reset. */
    DemoGenState gen;
    DemoUiState ui = { 0 };

    /* v3 (playtest 08/08, "a volte le entita' sembra che non si vedano"): un
     * asset CC0 mancante o corrotto non spegne piu' l'intera demo. Ogni
     * disegno che usa una texture stock passa da DemoDrawTextureOrPlaceholder,
     * che sostituisce un rettangolo magenta e alza ui.assetWarningShown
     * (riga sticky in HUD) invece di lasciare l'entita' invisibile. */
    if (!DemoLoadAssets(&assets))
        fprintf(stderr, "ATTENZIONE: uno o piu' asset CC0 mancanti/corrotti accanto all'eseguibile; "
                        "ripiego su placeholder magenta a runtime.\n");
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
    DemoGenInit(&gen);
    if (capture)
    {
        /* L'HUD della cattura disegna la riga GEN vera (niente piu' testo
         * congelato): perche' lo smoke test resti deterministico il brief va
         * forzato off, o il PNG cambierebbe a seconda che brief.txt esista
         * sul disco di chi lancia la cattura. */
        gen.useBrief = false;
        gen.briefPresent = false;
        gen.briefFirstLine[0] = '\0';
    }
    if (!capture)
    {
        DemoPoolScanGenerated(&enemyPool, DEMO_ENEMY_GENERATED_DIR);
        DemoPoolScanGenerated(&weaponPool, DEMO_WEAPON_GENERATED_DIR);
    }

    world.cosmeticRng = 0xC0FFEE11u;
    world.player.hp = DEMO_PLAYER_MAX_HP;
    world.player.position = (Vector2){ 640.0f, 532.0f };
    /* enemyThrottle si inizializza da solo dentro DemoEnemyBeginPattern (sotto
     * DemoEnemySpawn); weaponThrottle no, perche' allo start non c'e' nessuna
     * chiamata a DemoWeaponEquip (lo slot 0/pistola base e' lo stato zero di
     * DemoWorld, niente da caricare) -- va inizializzato qui a mano, o il
     * bucket resterebbe a 0.0 dal memset di `world = {0}` sopra. */
    DemoThrottleInit(&world.weaponThrottle, DEMO_WEAPON_SHOT_RATE_PER_SEC);
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
            DemoComposeFrame(&renderer, &ui, &assets, &world, &enemyPool, &weaponPool,
                             DEMO_RENDER_SMOOTH, false, false, &gen);
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

            /* v3: SHIFT+N/SHIFT+M scorrono il pool all'indietro, stesso
             * meccanismo di N/M ma in senso opposto (playtest 08/08, "il
             * cambio arma/nemico e' poco chiaro" -- includeva "vorrei poter
             * tornare indietro senza girare tutto il pool in avanti"). Ogni
             * cambio di slot annuncia il banner (DemoUiAnnounceEnemy/Weapon):
             * l'ultimo evento vince su un banner ancora visibile. */
            bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (IsKeyPressed(KEY_N) && enemyPool.count > 0)
            {
                enemyPool.current = shiftHeld ? (enemyPool.current + enemyPool.count - 1)%enemyPool.count
                                              : (enemyPool.current + 1)%enemyPool.count;
                DemoEnemyBeginPattern(&world, &enemyPool);
                DemoUiAnnounceEnemy(&ui, &enemyPool);
            }
            if (IsKeyPressed(KEY_M))
            {
                int slotCount = weaponPool.count + 1;
                weaponPool.current = shiftHeld ? (weaponPool.current + slotCount - 1)%slotCount
                                               : (weaponPool.current + 1)%slotCount;
                DemoWeaponEquip(&world, &weaponPool, &ui, true);
            }
            /* G/H: spawna un lotto (ADR-002, processo figlio non bloccante --
             * DemoGenSpawn rifiuta da sola una seconda pressione mentre il
             * primo figlio e' vivo, requisito 1). Seme fresco ad ogni
             * pressione: time(NULL) mescolato alla RNG cosmetica, MAI lo
             * stesso a due pressioni ravvicinate (requisito 2) -- basta
             * DemoNextRandom perche' avanza lo stato ad ogni chiamata, quindi
             * differenzia anche due G premuti nello stesso secondo. */
            if (IsKeyPressed(KEY_G))
                DemoGenSpawn(&gen, DEMO_GEN_KIND_ENEMY, DEMO_GEN_KEY_ATTACK_COUNT,
                            (uint32_t)time(NULL) ^ DemoNextRandom(&world));
            if (IsKeyPressed(KEY_H))
                DemoGenSpawn(&gen, DEMO_GEN_KIND_WEAPON, DEMO_GEN_KEY_ATTACK_COUNT,
                            (uint32_t)time(NULL) ^ DemoNextRandom(&world));
            /* B: toggle "usa brief" on/off, e SEMPRE ricarica il file (spec
             * sezione 4, "ricarica brief.txt"): anche premuto per spegnere,
             * l'owner potrebbe aver appena scritto il file per la prossima
             * volta che lo riaccende. */
            if (IsKeyPressed(KEY_B))
            {
                gen.useBrief = !gen.useBrief;
                DemoGenReloadBrief(&gen);
            }

            /* Poll UNA volta per frame, fuori dal loop a passo fisso (requisito
             * 3), come DemoPoolPollTick qui sotto: waitpid con WNOHANG non deve
             * mai finire dentro un ciclo che puo' girare piu' volte nello
             * stesso frame. */
            DemoGenPollTick(&gen, &enemyPool, &weaponPool, &ui);
            DemoGenDecayStatus(&gen, frameTime);
            DemoUiTick(&ui, frameTime);

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

            DemoComposeFrame(&renderer, &ui, &assets, &world, &enemyPool, &weaponPool, mode, split, true, &gen);
            BeginDrawing();
            ClearBackground(BLACK);
            DemoDrawFinalToWindow(renderer.finalTarget);
            EndDrawing();
        }
    }

    /* Requisito 4: NON uccidere un figlio ancora vivo all'uscita. Gli script
     * si scrivono con tmp+rename (gen_attacks.c), quindi un lotto interrotto
     * a meta' non lascia mai un .lua corrotto sul pool -- lasciarlo finire in
     * background e' innocuo, e il proprietario probabilmente lo rivuole
     * completo alla prossima apertura della demo. waitpid qui bloccherebbe
     * l'uscita per tutta la durata del lotto (fino a ~90s con Gemma): il
     * processo orfano passa al reaper di sistema (init/subreaper), che lo
     * raccoglie comunque quando termina. */
    if (gen.pid != 0)
        fprintf(stderr, "combat-lab: generazione %s ancora in corso (pid %ld), continua in background\n",
                gen.kind == DEMO_GEN_KIND_ENEMY ? "enemy" : "weapon", (long)gen.pid);

    DemoScriptUnload(&world.enemyScript);
    DemoScriptUnload(&world.weaponScript);
    DemoRendererUnload(&renderer);
    DemoUnloadAssets(&assets);
    CloseWindow();
    return 0;
}
