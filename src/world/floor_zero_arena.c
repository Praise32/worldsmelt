#include "world/floor_zero_arena.h"

#include "audio/audio.h"
#include "content/run_catalog.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "script/script_items.h"
#include "world/floor_zero.h"
#include "world/world.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Vedi il commento in cima a floor_zero_arena.h per il contratto completo
   (DEC-004/047/055/092/093/094/095). Qui sotto solo l'implementazione. */

/* Quanti tipi "best-of" si tengono al massimo per comporre un'ondata. Otto
   basta e avanza: una simulazione di pratica dura meno di una stanza vera, e
   una varieta' maggiore renderebbe il tema della piazzola illeggibile. */
#define FLOOR_ZERO_ARENA_POOL_MAX 8

/* Quanti nemici spawna ciascun tema. DEFAULT PROPOSTI DALL'IMPLEMENTAZIONE
   (stile DEC-019): nessun documento fissa la taglia di un'ondata di pratica.
   Sono pochi di proposito -- l'arena del Piano 0 insegna, non mette alla
   prova come l'arena incontrata nel piano, che ha budget maggiorato e nemici
   in fascia alta (WP6, systems/special-rooms.md). Il tema FUSIONE ne ha meno
   perche' la sua lezione e' la fucina, non il combattimento. */
#define FLOOR_ZERO_ARENA_ENEMIES_MOVE 3
#define FLOOR_ZERO_ARENA_ENEMIES_RESOURCES 2
#define FLOOR_ZERO_ARENA_ENEMIES_FUSION 1

/* Il seme dello stream deterministico della simulazione. NON deriva mai da
   game->rng ne' dall'orologio: la composizione di un'arena del Piano 0 deve
   essere la stessa a ogni ingresso, e soprattutto non deve SPOSTARE gli stream
   della run in preparazione -- FloorZeroArenaEnter salva game->rng nello
   snapshot, ci scrive questo valore e FloorZeroArenaExit rimette l'originale.
   Il Piano 0 non ha ancora un seed di run applicato al gameplay
   (GameResetRunWithSeed gira solo all'attraversamento del varco), quindi una
   costante per tema e' esattamente il "seme di processo" che serve. */
#define FLOOR_ZERO_ARENA_SEED_BASE 0x57534131u   /* 'WSA1' */

const char *FloorZeroArenaThemeLabel(FloorZeroTrialTheme theme)
{
    switch (theme)
    {
        case FLOOR_ZERO_TRIAL_MOVE: return "MOVIMENTO";
        case FLOOR_ZERO_TRIAL_RESOURCES: return "RISORSE";
        case FLOOR_ZERO_TRIAL_FUSION: return "FUSIONE";
        case FLOOR_ZERO_TRIAL_COUNT: break;
    }
    return "PROVA";
}

/* Il cartello della PRIMA visita (DEC-047): una riga breve che spiega i
   comandi del tema, tono ironico-leggero (DEC-105), senza accentate. Non e'
   un tutorial separato -- vive dentro l'arena che il giocatore usera' anche
   dopo per allenarsi, esattamente come chiede il documento. */
static const char *FloorZeroArenaHint(FloorZeroTrialTheme theme)
{
    switch (theme)
    {
        case FLOOR_ZERO_TRIAL_MOVE:
            return "WASD per muoverti, frecce o mouse per sparare. Qui non guarda nessuno.";
        case FLOOR_ZERO_TRIAL_RESOURCES:
            return "SPAZIO lascia una carica. Raccogli il resto: tanto qui non si tiene nulla.";
        case FLOOR_ZERO_TRIAL_FUSION:
            return "Raccogli i due oggetti e il Flux, poi TAB per la fucina.";
        case FLOOR_ZERO_TRIAL_COUNT:
            break;
    }
    return "";
}

/* Quanti nemici per tema (vedi le costanti sopra). */
static int FloorZeroArenaEnemyCount(FloorZeroTrialTheme theme)
{
    switch (theme)
    {
        case FLOOR_ZERO_TRIAL_MOVE: return FLOOR_ZERO_ARENA_ENEMIES_MOVE;
        case FLOOR_ZERO_TRIAL_RESOURCES: return FLOOR_ZERO_ARENA_ENEMIES_RESOURCES;
        case FLOOR_ZERO_TRIAL_FUSION: return FLOOR_ZERO_ARENA_ENEMIES_FUSION;
        case FLOOR_ZERO_TRIAL_COUNT: break;
    }
    return 1;
}

/* Le tre piazzole stanno sulla CROCE CENTRALE della stanza, l'unica zona che
   RoomLayoutBuild (src/core/room_layout.c) garantisce sempre libera: due sui
   bracci orizzontali, una sul braccio verticale in basso. Mai al centro
   esatto, dove nasce il giocatore -- una piazzola sotto i piedi all'ingresso
   sarebbe un invito difficile da distinguere dall'arredo. */
static Vector2 FloorZeroArenaGatePos(FloorZeroTrialTheme theme)
{
    float cx = ROOM_X + ROOM_W*0.5f;
    float cy = ROOM_Y + ROOM_H*0.5f;
    switch (theme)
    {
        case FLOOR_ZERO_TRIAL_MOVE: return (Vector2){ ROOM_X + ROOM_W*0.16f, cy };
        case FLOOR_ZERO_TRIAL_RESOURCES: return (Vector2){ ROOM_X + ROOM_W*0.84f, cy };
        case FLOOR_ZERO_TRIAL_FUSION: return (Vector2){ cx, ROOM_Y + ROOM_H*0.84f };
        case FLOOR_ZERO_TRIAL_COUNT: break;
    }
    return (Vector2){ cx, cy };
}

void FloorZeroArenaPlaceGates(Game *game)
{
    if (!game || game->floor != 0) return;
    for (int i = 0; i < FLOOR_ZERO_TRIAL_COUNT; i++)
        EntitiesAddPickup(game, PICKUP_TRIAL_GATE, FloorZeroArenaGatePos((FloorZeroTrialTheme)i), i, 0);
}

int FloorZeroArenaGateAtPlayer(const Game *game)
{
    if (!game) return -1;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        const Pickup *p = &game->pickups[i];
        if (!p->active || p->kind != PICKUP_TRIAL_GATE) continue;
        float r = p->radius + game->player.radius;
        if (GameMathLengthSquared(GameMathSubtract(p->pos, game->player.pos)) <= r*r)
        {
            /* 'value' e' il tema, mai una quantita' (vedi PickupKind in
               core/game_types.h). Clampato: un valore fuori banda per un
               futuro slot riciclato non deve poter indicizzare nulla. */
            if (p->value < 0 || p->value >= FLOOR_ZERO_TRIAL_COUNT) return FLOOR_ZERO_TRIAL_MOVE;
            return p->value;
        }
    }
    return -1;
}

bool FloorZeroArenaQueueEntry(Game *game)
{
    if (!game || game->floor != 0 || game->floorZeroTrialActive) return false;
    int theme = FloorZeroArenaGateAtPlayer(game);
    if (theme < 0) return false;
    game->floorZeroTrialRequest = theme + 1;
    return true;
}

/* Il pool di tipi con cui comporre l'ondata: prima il "best-of" delle run
   passate (DEC-004: contenuti gia' validati, mai generati sul momento), poi --
   e SOLO se quello e' vuoto -- il ripiego curato, cioe' i nemici del piano 1
   gia' caricati in memoria piu', se mancassero anche quelli, i tipi d'esempio
   del motore. E' il caso limite dichiarato di floor-zero.md: l'arena funziona
   SEMPRE, anche alla primissima esecuzione con un catalogo inesistente
   (DEC-087/094/153). Ritorna il numero di tipi scritti, mai 0. */
static int FloorZeroArenaBuildPool(const Game *game, EnemyTypeDef *pool, int maxPool, bool *outBestOf)
{
    int count = RunCatalogBestOfEnemies(pool, maxPool);
    if (outBestOf) *outBestOf = (count > 0);
    if (count > 0) return count;

    const FloorContent *fc = &game->content.floors[0];
    for (int i = 0; i < 2 && count < maxPool; i++)
        if (fc->enemies[i].active) pool[count++] = fc->enemies[i];

    /* Nemmeno il contenuto curato del piano 1 e' disponibile: restano i tipi
       d'esempio del motore, che esistono per costruzione e non dipendono da
       alcun file. Il giocatore non deve MAI trovare una piazzola vuota. */
    for (int i = 0; i < ENEMY_TYPE_EXAMPLE_COUNT && count < maxPool; i++)
    {
        EnemyTypeDef example;
        EnemyTypeExample(&example, i);
        pool[count++] = example;
    }
    return count;
}

/* Una posizione di spawn deterministica: sul cerchio attorno al centro della
   stanza, a passi regolari piu' un piccolo scarto dallo stream locale. Mai
   game->rng della run (vedi FLOOR_ZERO_ARENA_SEED_BASE sopra) e mai troppo
   vicina al giocatore, che nasce al centro. */
static Vector2 FloorZeroArenaSpawnPos(const Game *game, unsigned int *stream, int index, int total)
{
    float cx = ROOM_X + ROOM_W*0.5f;
    float cy = ROOM_Y + ROOM_H*0.5f;
    float step = (total > 0) ? (6.2831853f/(float)total) : 0.0f;
    float angle = step*(float)index + GameRngFloat(stream, -0.25f, 0.25f);
    float radius = GameRngFloat(stream, 150.0f, 190.0f);
    Vector2 pos = { cx + radius*cosf(angle), cy + radius*sinf(angle)*0.55f };
    /* Il crogiolo e' una cella 1x1: il clamp ai bordi vale gia' per
       costruzione, ma si applica comunque -- una posizione fuori stanza
       sarebbe un nemico incastrato nel muro. */
    WorldClampToRoom(game, &pos, 40.0f);
    return pos;
}

void FloorZeroArenaEnter(Game *game, FloorZeroTrialTheme theme, bool tutorial)
{
    if (!game || game->floor != 0 || game->floorZeroTrialActive) return;

    /* (1) LO STATO D'INGRESSO, prima di toccare qualunque cosa (DEC-092). */
    FloorZeroTrialSnapshot *snap = &game->floorZeroTrialSnapshot;
    memset(snap, 0, sizeof(*snap));
    snap->valid = true;
    snap->player = game->player;
    snap->score = game->score;
    snap->rng = game->rng;
    snprintf(snap->message, sizeof(snap->message), "%s", game->message);
    snap->messageTimer = game->messageTimer;

    /* (2) LA SALETTA. Nessun ostacolo: un'ondata va schivata, e lo stesso
       ragionamento vale gia' per l'arena incontrata nel piano, che non ha
       arredo di layout (WP6, systems/special-rooms.md). */
    EntitiesClear(game);
    game->obstacleCount = 0;
    game->obstacleHoleCount = 0;
    game->player.pos = (Vector2){ ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    game->player.invuln = 0.0f;
    game->bombQueued = false;
    game->useActiveQueued = false;
    game->dropGraftQueued = false;
    game->interactQueued = false;
    /* Il pannello mondi/personaggi non ha senso dentro una simulazione: le
       frecce li' servono a sparare, e src/app/app.c smette comunque di
       ascoltarlo finche' la prova e' aperta -- qui si chiude anche
       visivamente, cosi' non resta appeso sopra l'arena. */
    game->themeCardsPanelOpen = false;
    WorldSnapCamera(game);

    game->floorZeroTrialActive = true;
    game->floorZeroTrialTheme = theme;
    game->floorZeroTrialDefeated = false;
    game->floorZeroTrialWon = false;
    game->floorZeroTrialRequest = 0;
    game->rng = FLOOR_ZERO_ARENA_SEED_BASE ^ ((unsigned int)theme*2654435761u);

    /* (3) I CONTENUTI BEST-OF (DEC-004/094). */
    EnemyTypeDef pool[FLOOR_ZERO_ARENA_POOL_MAX];
    bool bestOf = false;
    int poolCount = FloorZeroArenaBuildPool(game, pool, FLOOR_ZERO_ARENA_POOL_MAX, &bestOf);

    unsigned int stream = game->rng;
    /* FloorZeroArenaBuildPool non torna mai 0 per costruzione (i tipi
       d'esempio del motore chiudono sempre la catena), ma un pool vuoto qui
       significherebbe una divisione per zero due righe sotto: la guardia costa
       una riga e trasforma un guasto futuro in "simulazione senza nemici"
       invece che in un crash. */
    int wanted = (poolCount > 0) ? FloorZeroArenaEnemyCount(theme) : 0;
    int spawned = 0;
    for (int i = 0; i < wanted; i++)
    {
        const EnemyTypeDef *type = &pool[i % poolCount];
        EnemyKind kind = ENEMY_CHASER;
        if (type->form == ENEMY_FORM_SPIKY) kind = ENEMY_SHOOTER;
        else if (type->form == ENEMY_FORM_ARMORED) kind = ENEMY_TANK;
        /* Un boss del catalogo entra come nemico normale della simulazione:
           qui si pratica, non si riaffronta un boss -- quella e' la prova dal
           museo (DEC-040), che non esiste ancora nel motore. */
        EnemyTypeDef copy = *type;
        copy.boss = false;
        EntitiesAddEnemyTyped(game, kind, FloorZeroArenaSpawnPos(game, &stream, i, wanted), &copy);
        spawned++;
    }
    game->floorZeroTrialEnemyGoal = spawned;

    /* (4) GLI ATTREZZI DEL TEMA. Tutto quello che compare qui e' materiale di
       pratica: il ripristino dell'uscita lo cancella comunque, quindi non
       esiste alcun modo di portarlo fuori (DEC-093). */
    Vector2 center = { ROOM_X + ROOM_W*0.5f, ROOM_Y + ROOM_H*0.5f };
    switch (theme)
    {
        case FLOOR_ZERO_TRIAL_MOVE:
            break;   /* solo movimento e tiro: niente da raccogliere */
        case FLOOR_ZERO_TRIAL_RESOURCES:
            EntitiesAddPickup(game, PICKUP_BOMB, (Vector2){ center.x - 90.0f, center.y + 70.0f }, 2, 0);
            EntitiesAddPickup(game, PICKUP_COIN, (Vector2){ center.x, center.y + 70.0f }, 4, 0);
            EntitiesAddPickup(game, PICKUP_KEY, (Vector2){ center.x + 90.0f, center.y + 70.0f }, 1, 0);
            break;
        case FLOOR_ZERO_TRIAL_FUSION:
        {
            /* Due oggetti e un catalizzatore: il minimo esatto che
               systems/item-fusion.md chiede per poter fondere davvero, cosi'
               la lezione si puo' completare invece di solo leggerla. Presi dal
               pool del piano 1 gia' in memoria, senza estrazione: la stessa
               piazzola deve proporre sempre la stessa coppia. */
            const FloorContent *fc = &game->content.floors[0];
            EntitiesAddItemPickup(game, (Vector2){ center.x - 90.0f, center.y + 70.0f }, fc->items[0], 0);
            EntitiesAddItemPickup(game, (Vector2){ center.x + 90.0f, center.y + 70.0f }, fc->items[1], 0);
            EntitiesAddPickup(game, PICKUP_FLUX, (Vector2){ center.x, center.y + 90.0f }, 2, 0);
            break;
        }
        case FLOOR_ZERO_TRIAL_COUNT:
            break;
    }

    /* (5) IL CARTELLO DELLA PRIMA VISITA (DEC-047). */
    if (tutorial) snprintf(game->floorZeroTrialHint, sizeof(game->floorZeroTrialHint), "%s", FloorZeroArenaHint(theme));
    else game->floorZeroTrialHint[0] = '\0';

    AudioPlaySfx(AUDIO_SFX_UI_CONFIRM);
    GameSetMessage(game, bestOf ? "Simulazione avviata: vecchie conoscenze, nessun rischio. ESC per uscire."
                                : "Simulazione avviata: ripiego curato, nessun rischio. ESC per uscire.");
}

bool FloorZeroArenaCleared(const Game *game)
{
    if (!game || !game->floorZeroTrialActive) return false;
    if (game->floorZeroTrialEnemyGoal <= 0) return false;
    for (int i = 0; i < MAX_ENEMIES; i++) if (game->enemies[i].active) return false;
    return true;
}

void FloorZeroArenaNoteVictory(Game *game)
{
    if (!game || !game->floorZeroTrialActive || game->floorZeroTrialWon) return;
    if (!FloorZeroArenaCleared(game)) return;
    game->floorZeroTrialWon = true;
    AudioPlaySfx(AUDIO_SFX_UI_CONFIRM);
    GameSetMessage(game, "Simulazione superata. Resta quanto vuoi: qui non si consuma nulla.");
}

void FloorZeroArenaExit(Game *game, bool defeated)
{
    if (!game || !game->floorZeroTrialActive) return;
    FloorZeroTrialSnapshot *snap = &game->floorZeroTrialSnapshot;
    if (!snap->valid) return;

    bool won = game->floorZeroTrialWon;

    /* (1) Le sandbox Lua create DENTRO la simulazione (il tema FUSIONE fa
       raccogliere oggetti veri, che possono portare uno script) vanno chiuse
       PRIMA di riscrivere il Player: dopo, gli indici di itemScripts[]
       punterebbero a oggetti che non esistono piu' e la memoria di Lua si
       perderebbe senza mai lua_close. Stesso ordine di FloorZeroEnter. */
    ScriptItemsShutdown(game);
    EntitiesClear(game);

    /* (2) IL RIPRISTINO INTEGRALE (DEC-092): il Player torna quello di prima,
       copiato per intero. */
    game->player = snap->player;
    game->score = snap->score;
    game->rng = snap->rng;
    snprintf(game->message, sizeof(game->message), "%s", snap->message);
    game->messageTimer = snap->messageTimer;

    /* (3) Le statistiche derivate si ricalcolano da zero dai base* appena
       ripristinati, come fa ogni altro punto del motore che tocca
       l'inventario -- mai una copia "a mano" dei valori derivati. */
    ScriptItemsInit(game, GameResolveCharacterDef(game, game->characterChosenIndex));

    /* (4) Il crogiolo torna com'era: arredo curato deterministico e piazzole
       di nuovo al loro posto. */
    game->floorZeroTrialActive = false;
    game->floorZeroTrialDefeated = false;
    game->floorZeroTrialWon = false;
    game->floorZeroTrialEnemyGoal = 0;
    game->floorZeroTrialHint[0] = '\0';
    game->floorZeroTrialRequest = 0;
    game->bombQueued = false;
    game->useActiveQueued = false;
    game->dropGraftQueued = false;
    game->interactQueued = false;
    memset(snap, 0, sizeof(*snap));

    FloorZeroBuildDressing(game);
    FloorZeroArenaPlaceGates(game);
    WorldSnapCamera(game);

    if (defeated) GameSetMessage(game, "Simulazione finita male. Nessun graffio vero: era tutta scena.");
    else if (won) GameSetMessage(game, "Simulazione superata. Il crogiolo non paga: paga il piano 1.");
    else GameSetMessage(game, "Simulazione chiusa. Nulla di quel che e' successo li' dentro conta.");
}
