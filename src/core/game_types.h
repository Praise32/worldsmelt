#ifndef MELTING_RUN_GAME_TYPES_H
#define MELTING_RUN_GAME_TYPES_H

#include "raylib.h"

/* Tipi di colpo (step C): il vocabolario parametrico con cui il MODELLO inventa
   i tipi di colpo di ogni run (mai un menu fisso in C, vedi il commento in cima
   a shot_type.h). Vive in un header a parte, senza raylib, perche' lo include
   anche melting-gen: una sola definizione, impossibile da far divergere fra
   generatore e gioco. */
#include "core/enemy_type.h"
#include "core/room_layout.h"
#include "core/shot_type.h"

#include <stdbool.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640
#define APP_WINDOW_WIDTH 1600
#define APP_WINDOW_HEIGHT 900

#define HUD_H 82
#define FOOTER_H 38
/* DEC-170 (taglie multiple + telecamera; supera il lattice in pixel di M2):
   questi sono la taglia e l'origine di UNA CELLA della griglia del piano --
   la stanza 1x1, cioe' la grandezza minima garantita da DEC-009. Una stanza
   vera occupa 1..4 celle contigue (vedi RoomState/RoomSize sotto), quindi il
   suo rettangolo e' un MULTIPLO di questo, mai un valore intermedio.
   Le coordinate di gioco sono LOCALI alla stanza corrente: il riquadro di
   ogni stanza parte sempre da (ROOM_X, ROOM_Y) e cresce verso destra/basso di
   una cella per volta (si simula una stanza per volta, le stanze non
   coesistono mai nello stesso spazio). La telecamera (src/world/room_camera.h)
   traduce quel mondo nel canvas 960x640: per la 1x1 la traduzione e'
   l'identita' -- l'inquadratura fissa di sempre, cornice di muro compresa.
   L'UNICO modo corretto di leggere il rettangolo di una stanza resta
   WorldRoomRect/WorldCurrentRoomRect (src/world/world.h). */
#define ROOM_X 42.0f
#define ROOM_Y 104.0f
#define ROOM_W 876.0f
#define ROOM_H 458.0f
#define ROOM_RIGHT (ROOM_X + ROOM_W)
#define ROOM_BOTTOM (ROOM_Y + ROOM_H)
#define DOOR_HALF 50.0f

#define FLOOR_COUNT 5
#define GRID_SIZE 5

#define MAX_ENEMIES 64
#define MAX_SHOTS 220
#define MAX_PICKUPS 28
#define MAX_BOMBS 8
#define MAX_ITEMS 18
/* Tetto di MOTORE agli slot funzionali (systems/items-pools-and-rarity.md,
   "Slot": si parte con 1 slot attivo + 1 slot Innesto, oggetti/eventi rari
   possono aggiungerne). Il documento lascia aperto il numero MASSIMO
   ottenibile in una run: questi non sono quel numero, sono il limite oltre
   il quale il motore rifiuta di crescere -- una fonte di slot generata male
   non puo' comunque sfondare gli array. Il valore iniziale (1 e 1) lo scrive
   GamePlayerResetBaseStatsFor, non queste costanti. */
#define MAX_ACTIVE_SLOTS 4
#define MAX_GRAFT_SLOTS 4
/* DEC-171 (ponte provvisorio della demo): quante voci al massimo il motore
   legge da assets/curated/manifest.json e, di conseguenza, quanti bit ha la
   maschera "immagine gia' usata in questa run" (Game.curatedImageUsed). Il
   pacchetto curato ne ha 189 oggi: 256 lascia margine senza costare nulla
   (32 byte di maschera). Un manifest piu' lungo NON e' un errore -- le voci
   oltre il tetto semplicemente non vengono mai pescate, vedi
   src/content/curated_images.h. */
#define CURATED_IMAGE_MAX 256
#define CURATED_IMAGE_MASK_BYTES (CURATED_IMAGE_MAX/8)
/* Quante texture curate il gioco tiene caricate insieme (le immagini degli
   oggetti FUSI di questa run, DEC-171). La cadenza attesa e' 1-2 fusioni per
   run (DEC-022), quindi 8 e' gia' abbondante; oltre, il motore smette di
   caricare e il risultato ricade sulla forma geometrica di sempre -- una
   degradazione visiva, mai un errore. */
#define MAX_CURATED_TEXTURES 8
#define MAX_PARTICLES 128
#define MAX_SCRIPT_OPS 4
#define SCRIPT_TEXT_LEN 256
/* Sorgente Lua opzionale di un oggetto (fase 3a-L2, vedi
   docs/engineering/specs/2026-07-13-lua-sandbox-design.md sezioni 5-9).
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
    ROOM_BOSS,
    /* M1b: la sala hub del Piano 0 (src/world/floor_zero.c). Aggiunta IN CODA,
       come SPR_ENEMY_FLOATER sotto: RoomKind non e' mai usato come indice di
       un array (solo confronti/switch, vedi world.c/game_renderer.c), quindi
       non c'e' un vincolo di posizione, ma restare in coda evita comunque di
       spostare per errore il significato implicito di un valore salvato da
       qualche parte in futuro. */
    ROOM_HUB
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
    PICKUP_EXIT,
    /* Secondo canale di ricarica degli attivi a cariche (DEC-059): l'energia
       che lasciano i nemici. In CODA all'enum, mai in mezzo, per lo stesso
       motivo di SPR_ENEMY_FLOATER piu' sotto -- questi valori finiscono in
       switch e tabelle indicizzate sparse per il codice. Non ha una cella
       d'atlas propria (aggiungerne una invaliderebbe ogni atlas gia'
       generato): si disegna con una forma geometrica, vedi DrawPickup. */
    PICKUP_ENERGY,
    /* Catalizzatore di fusione (in-game: Flux, DEC-072) -- la risorsa RARA
       che paga una fusione (DEC-022, systems/health-and-resources.md
       "Catalizzatore di fusione"). In coda all'enum come PICKUP_ENERGY sopra
       e per lo stesso motivo; nessuna cella d'atlas propria, forma
       geometrica in DrawPickup. Raccoglierlo non da' un oggetto: incrementa
       Player.flux, che non ha alcun cap (DEC-129). */
    PICKUP_FLUX
} PickupKind;

typedef enum ItemSlot {
    SLOT_HAT,
    SLOT_EYES,
    SLOT_HAND,
    SLOT_BACK,
    SLOT_BODY,
    SLOT_AURA
} ItemSlot;

/* Tassonomia degli oggetti a QUATTRO categorie
   (docs/design/systems/items-pools-and-rarity.md, "Categorie": attivo,
   passivo, stat-up, Innesto). Fino a questa fase il codice ne aveva due, e
   la prima si chiamava ITEM_ACTIVE pur significando "passivo" nel senso del
   design (effetto sempre presente una volta ottenuto: modifica come spari o
   ti muovi). Il nome mentiva: qui e' rinominata ITEM_PASSIVE, che e' quello
   che ha sempre fatto, e ITEM_ACTIVE torna a significare cio' che il design
   intende -- un oggetto che si ATTIVA volontariamente, con cariche o
   cooldown e uno slot dedicato (systems/active-items.md).

   ITEM_PASSIVE vale 0 di proposito, per lo stesso motivo per cui ci stava
   prima il vecchio ITEM_ACTIVE: un Item azzerato con "{0}" (il pattern usato
   in tutto il codice, vedi CombatApplyItem/i test), un manifest vecchio
   senza riga "kind=", un record di catalogo scritto prima di oggi restano
   PASSIVI -- cioe' esattamente la semantica che avevano -- e non diventano
   mai per sbaglio uno stat-up, un attivo usabile o un Innesto.
   La mappatura dei testi del manifest (compresa la riga storica
   "kind=active", che significava passivo) vive in ItemKindFromText,
   src/content/run_content.c: e' l'unico punto che la conosce. */
typedef enum ItemKind {
    ITEM_PASSIVE,   /* effetto sempre presente, nessuno slot, si accumula senza limite */
    ITEM_STATUP,    /* solo numeri, nessun comportamento (ricompensa del boss) */
    ITEM_ACTIVE,    /* si attiva a comando: cariche o cooldown, slot dedicato */
    ITEM_GRAFT      /* Innesto: piccolo, situazionale, sganciabile, slot dedicato */
} ItemKind;

/* Rarita' (fase 3b, docs/engineering/specs/2026-07-13-pools-rarity-design.md
   sezioni 1-3): determina SIA la potenza (il tetto per-oggetto scalato per
   rarita', vedi src/script/script_items.c, SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION)
   SIA la frequenza di drop (le tabelle di peso per pool, vedi
   tools/melting-gen/gen_util.c lato generatore e src/content/run_content.c
   lato gioco). Niente campo "pool" a parte: il pool di un oggetto e' gia'
   la sua POSIZIONE (items[0..2] = tesoro/negozio, bossItem = boss, vedi
   FloorContent sotto), esattamente come "kind" gia' non ha bisogno di un
   campo aggiuntivo per sapere se e' un oggetto del piano o la ricompensa
   del boss. RARITY_COMMON vale 0 di proposito, stesso motivo di ITEM_PASSIVE
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

/* Stati canonici del gioco (M1a, docs/design/
   05-game-states-and-flow.md, fonte unica dei NOMI: qualunque altro documento
   di design che parli di navigazione deve usare esattamente questi nomi).
   L'ordine e APP_MAIN_MENU=0 sono voluti (spec M1a): src/app/app.c li scrive
   in uno switch esplicito su UpdateApp, senza "default", cosi' dimenticare un
   caso e' un -Wswitch, non un silenzio a runtime. FloorZero assorbe la
   vecchia schermata di generazione (mai uno stato a parte, vedi
   ui/generation-status.md): in M1a la generazione resta bloccante e vive
   dentro quello stato come overlay. */
typedef enum AppMode {
    APP_MAIN_MENU,
    APP_RUN_SETUP,
    APP_FLOOR_ZERO,
    APP_GAMEPLAY,
    APP_PAUSE_MENU,
    APP_OPTIONS,
    APP_BUILD_SCREEN,
    APP_RUN_RESULTS,
    APP_EXIT_CONFIRM
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
    /* Fase 3b: la quarta FORMA di nemico (medusa che fluttua). Aggiunta IN CODA,
       mai in mezzo: l'indice di una cella e' la sua posizione nell'atlas, quindi
       inserirla fra le altre sposterebbe tutte le celle successive e ogni atlas
       gia' generato (o gia' su disco) diventerebbe sbagliato di colpo. */
    SPR_ENEMY_FLOATER,
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

/* M5 (DEC-005, scelta del tema nel Piano 0): una carta-proposta, letta da
 * generated/theme_proposals.json (AppLoadThemeCards, src/app/app.c) o
 * generata SUL POSTO dal ripiego lato gioco (RunContentMakeFallbackThemeCards,
 * src/content/run_content.c) quando la generazione e' disabilitata o
 * bin/melting-gen non c'e' (DEC-002: il gioco resta sempre avviabile). Stessi
 * limiti di GenThemeProposal (tools/melting-gen/melting_gen.h): name 48 byte
 * (3-40 char usati per davvero, il resto e' margine), blurb 200 -- charset
 * ASCII puro (DEC-052), mai bisogno di escape quando si spezza il JSON a
 * mano (il gioco non linka cJSON, AGENTS.md). */
#define THEME_CARD_MAX 3
typedef struct ThemeCard {
    char name[48];
    char blurb[200];
} ThemeCard;

/* M6a (DEC-030/033/049, rosa base di personaggi): scheda di un personaggio.
 * La rosa fissa (2-3, CURATA a mano) usa questa stessa forma per i suoi
 * CHARACTER_COUNT elementi, che vivono in src/content/character_roster.c,
 * NON qui -- questo header resta solo la FORMA condivisa (come Theme/Item
 * sopra), coerente con AGENTS.md ("costanti/tipi condivisi in core, i dati
 * in un modulo dedicato"). Da M6b-1 (DEC-014, prima fetta), la STESSA forma
 * ospita anche il personaggio alternativo GENERATO per-run (Game.
 * generatedCharacter, src/content/character_proposal.c la costruisce dal
 * json su disco): nome/blurb/stats/palette sono generati, il trait Lua
 * unico (DEC-037) e il colpo firmato (DEC-068) restano gap di
 * implementazione espliciti, non ancora campi di questa struct (vedi
 * docs/design/systems/characters.md, nota
 * di stato della fetta). 'hpCap' e' il tetto di salute BASE proprio di
 * questo personaggio (DEC-033): ScriptItemsRecomputeStats lo usa al posto
 * del tetto globale storico [1,12], che resta il ripiego SOLO quando nessun
 * personaggio e' stato applicato (vedi il commento su Player.hpCap sotto).
 * 'palette' e' il colore con cui DrawBaseStickman tinge lo stickman di
 * questo personaggio (DEC-058-friendly: mai l'unico segnale di selezione,
 * solo un tocco identitario in piu' -- il focus/la selezione nel pannello
 * restano bordo+scala+indicatore, vedi DrawFloorZeroPanel; per il
 * personaggio generato l'origine e' anche testo esplicito nel campo 'role',
 * vedi RunContentLoadCharacterProposal). Nomi in inglese (DEC-052), niente
 * termine riservato della nomenclatura in-game (vedi
 * governance/glossary.md). */
#define CHARACTER_COUNT 3
/* Le due sezioni del pannello combinato del Piano 0 (M6a, requisito 3):
 * su/giu' passa dall'una all'altra, sinistra/destra sposta il focus DENTRO
 * la sezione attiva. Interi semplici (non un enum) perche' Game.
 * floorZeroPanelSection deve restare un campo POD azzerabile con memset
 * come il resto di Game, e 0 (MONDI) e' gia' il valore giusto di default
 * all'ingresso in FloorZero, senza bisogno di uno zero-default diverso da
 * "il primo dei due". */
#define FLOOR_ZERO_PANEL_WORLDS 0
#define FLOOR_ZERO_PANEL_CHARACTERS 1
typedef struct CharacterDef {
    char name[32];
    char role[24];
    char blurb[160];
    float baseDamage;
    float baseFireDelay;
    float baseShotSpeed;
    float baseShotRadius;
    float baseSpeed;
    int baseMaxHp;
    int hpCap;
    float baseLuck;
    Color palette;
    /* M6b-2 (DEC-037): SOLO per il personaggio GENERATO -- il nome della
       callback che il suo trait Lua definisce ("on_fire"/"on_hit"/"on_tick"/
       "on_evaluate"), letto per-testo dal file .lua stesso al momento del
       caricamento della proposta (RunContentLoadCharacterProposal, src/
       content/character_proposal.c), MAI inventato: stringa vuota se non
       c'e' nessun trait (rosa curata, proposta senza campo "lua", o file
       assente/illeggibile -- stesso fallback silenzioso di ogni altro punto
       di questa fetta, vedi il commento su ScriptCharacterSetActive in src/
       script/script_character.h). Usato SOLO per la riga onesta e sobria
       sulla carta (DrawCharacterCards, src/render/game_renderer.c): il
       caricamento VERO del comportamento rilegge lo stesso file per conto
       suo quando il personaggio viene selezionato, indipendentemente da
       questo campo (due letture separate, mai una a cascata dell'altra). */
    char traitHook[16];
    /* M6b-3 (DEC-068): il colpo firmato OPZIONALE del personaggio GENERATO
       -- 'active' falso (lo zero-default di un memset, e quindi SEMPRE il
       caso per la rosa curata: character_roster.c non lo imposta mai) =
       nessun colpo firmato, il personaggio usa il colpo standard come
       prima di questa fetta. Gia' bilanciato quando arriva qui
       (RunContentLoadCharacterProposal -> CharacterGenDefClamp, la seconda
       rete lato gioco): questo campo non viene mai clampato per conto suo,
       solo copiato. Consumato da GamePlayerResetBaseStatsFor (src/game/
       game.c), che lo trasferisce su Player.characterShotType/
       characterShotColor -- vedi il commento li' per il perche' non basta
       tenerlo solo qui (ScriptItemsRecomputeStats non vede mai una
       CharacterDef, solo il Player). */
    ShotTypeDef signatureShot;
} CharacterDef;

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
    ItemKind kind;   /* ITEM_PASSIVE di default (vedi il commento sopra): mai un'altra categoria senza che qualcuno la imposti esplicitamente */
    Rarity rarity;   /* RARITY_COMMON di default (vedi il commento sopra): letta dal renderer (fase 3b VISIVA, task parallelo) per il colore del bordo, e da script_items.c/world.c per il tetto di potenza/costo negozio */
    Color color;
    int shape;
    char script[SCRIPT_TEXT_LEN];
    char luaSource[SCRIPT_LUA_LEN];   /* vedi il commento su SCRIPT_LUA_LEN sopra */
    /* Tipo di colpo che questo oggetto conferisce, se ne conferisce uno (step C,
       docs/engineering/specs/2026-07-14-step-c-shottype-balance.md). 'active'
       falso (lo zero-default di "{0}", di un memset e di ogni manifest scritto
       prima di questa fase) = l'oggetto NON cambia il modo di sparare: e' il
       caso della grande maggioranza degli oggetti, e di TUTTI gli stat-up del
       boss (che sono solo numeri, mai comportamento). Il tipo viaggia DENTRO
       l'Item (per valore, non un indice in una tabella a parte) perche' un Item
       viene copiato per valore ovunque -- FloorContent -> Pickup -> Player.items
       -- e un riferimento indiretto si romperebbe silenziosamente a ogni copia. */
    ShotTypeDef shotType;
    /* Ricarica di un oggetto ATTIVO (systems/active-items.md, "Come si
       attivano e ricaricano"): il design impone che ogni attivo dichiari UNO
       fra cariche e cooldown, mai nessuno dei due e mai entrambi. Qui i due
       modi convivono come campi perche' il motore deve poter leggere anche un
       contenuto malfatto: chi decide quale vale e' ItemActiveIsChargeBased/
       ItemActiveIsCooldownBased (src/gameplay/item_slots.h), con le cariche
       che vincono se per errore ci fossero entrambi -- il modo piu' avaro dei
       due, cosi' un dato sbagliato rende l'oggetto piu' debole, mai piu'
       forte (stessa regola di ScriptItemsRarityFraction).
       Tutti zero = nessuno dei due dichiarato: l'attivo ricade sul cooldown
       di riserva del motore (ITEM_ACTIVE_DEFAULT_COOLDOWN), mai su "usabile a
       ogni frame".
       'chargeGainRoom'/'chargeGainEnergy' sono il dosaggio dei DUE canali di
       base di DEC-059 (stanza completata / energia droppata dai nemici):
       restano per-oggetto, come vuole il documento, e valgono 1 quando
       l'oggetto non dichiara nulla. */
    int charges;             /* capienza in cariche (>0 = attivo a cariche) */
    float cooldown;          /* secondi di attesa dopo l'uso (>0 = attivo a cooldown) */
    int chargeGainRoom;      /* DEC-059, canale 1: cariche per stanza completata */
    int chargeGainEnergy;    /* DEC-059, canale 2: cariche per raccolta di energia */
    /* Stato VIVO della ricarica. Vive dentro l'Item, non in una tabella
       parallela indicizzata per slot, esattamente per il motivo scritto sopra
       su ShotTypeDef: un Item viaggia per VALORE (FloorContent -> Pickup ->
       Player.items -> di nuovo Pickup quando lo si scambia sul piedistallo,
       DEC-117). Cosi' lo scambio col piedistallo conserva le cariche che
       l'oggetto aveva, e riprenderlo indietro lo restituisce com'era: se lo
       stato vivesse in una tabella per-slot, uno scambio lo azzererebbe in
       silenzio. */
    int chargeNow;           /* cariche accumulate ORA */
    float cooldownTimer;     /* secondi che mancano al prossimo uso */
    /* --- Fusione (systems/item-fusion.md, DEC-023 passo 1 + DEC-171) -------
       'imagePath' e' il percorso RELATIVO a assets/curated/ dell'immagine
       curata di questo oggetto; vuoto = nessuna, e il gioco ricade sulla resa
       geometrica di sempre (lo zero-default di "{0}" vale "come prima di
       questa fase"). Due sorgenti, entrambe legittime: la pesca di
       FusionPerform per un oggetto COMPOSTO (DEC-171), e -- da W5b -- il
       layer di indirezione content-id -> image-id del pool CURATO, per gli
       oggetti dei piani (ApplyCuratedCatalog, content/run_content.c). Resta
       vuoto per ogni oggetto puramente procedurale o GENERATO: melting-gen
       non scrive mai un'immagine per gli oggetti dei piani, e il ramo
       manifest di RunContentLoad azzera questo campo proprio per non far
       ereditare a un oggetto generato l'immagine risolta per un contenuto
       curato diverso. Vive dentro l'Item, non in una
       tabella indicizzata a parte, per lo stesso motivo di ShotTypeDef e
       dello stato di ricarica sopra: un Item viaggia per VALORE
       (FloorContent -> Pickup -> Player.items -> di nuovo Pickup), e un
       riferimento indiretto si romperebbe in silenzio a ogni copia.
       'fusedFrom' sono i nomi dei DUE oggetti sorgente consumati (troncati):
       la scheda del risultato deve dichiarare da chi deriva (item-fusion.md,
       "Feedback"), e fusedFrom[0][0] != '\0' e' anche il modo canonico di
       chiedere "questo oggetto e' nato da una fusione?" -- serve alla
       ri-fusione (DEC-102: ammessa, nessun limite) e all'etichetta di
       origine 'composto'. */
    char imagePath[64];
    /* W8: l'IMAGE-ID risolto per questo oggetto (DEC-175(b)), accanto al file
       curato e non al posto suo. I due campi sono i due gradini della stessa
       priorita', non due alternative: 'imageId' e' la chiave con cui si cerca
       PRIMA l'originale animato in assets/art/ (ArtAtlasFindByImageId,
       src/assets/art_atlas.h), 'imagePath' e' il ripiego CC0 di DEC-171 quando
       quell'originale non esiste ancora. Tenere solo l'id avrebbe voluto dire
       ricostruire il percorso curato riaprendo il manifest a ogni frame di
       disegno; tenere solo il percorso avrebbe voluto dire indovinare l'id dal
       nome del file, cioe' scavalcare in silenzio proprio il layer di
       indirezione che DEC-175(b) impone. Scritto dagli STESSI due punti che
       scrivono imagePath (ApplyCuratedCatalog in content/run_content.c e
       FusionPerform in gameplay/fusion.c) e azzerato con lui. */
    char imageId[40];
    char fusedFrom[2][40];
} Item;

/* DEC-171: le texture delle immagini curate gia' caricate in questa run,
   indicizzate per PERCORSO (lo stesso Item.imagePath), non per indice di
   manifest: chi disegna ha in mano un Item, non un indice, e un indice
   memorizzato qui si disallineerebbe al primo manifest ricostruito
   (scripts/curated-pack.py). Vive dentro Game come l'atlas, ed e' liberata
   dallo stesso punto (GameUnloadAssets), cosi' non esiste un secondo ciclo
   di vita da ricordare. Cache STRETTAMENTE del renderer: il motore di
   fusione non la tocca mai (nessun modulo di gameplay apre file di
   immagine), vedi src/assets/game_assets.h. */
typedef struct CuratedTextureCache {
    int count;
    char paths[MAX_CURATED_TEXTURES][64];
    Texture2D textures[MAX_CURATED_TEXTURES];
} CuratedTextureCache;

typedef struct FloorContent {
    Theme theme;
    /* I due tipi di nemico del piano e il tipo del boss (fase 3b), inventati dal
       modello. Se non sono attivi (manifest vecchio, nessun manifest) il gioco usa
       i nemici storici: back-compat totale, vedi RunContentLoad. */
    EnemyTypeDef enemies[2];
    EnemyTypeDef bossType;
    /* Layout delle stanze di combattimento del piano (fase 3c), inventato dal
       modello. Non attivo (manifest vecchio, nessun manifest) = stanze vuote come
       prima: back-compat totale. */
    RoomLayoutDef roomLayout;
    Item items[3];   /* oggetti ATTIVI del piano: stanza tesoro e negozio pescano da qui (world.c) */
    /* Indice di manifest (content/curated_images.h) dell'immagine curata
       risolta per items[i] dal pool curato (content/run_content.c,
       ApplyCuratedCatalog), o -1 se items[i] non ha nessuna immagine curata
       (contenuto procedurale, o id/mappa mancanti). RunContentLoad lo lascia
       SEMPRE a -1 di suo conto (0 e' un indice di manifest valido, mai
       usabile come "assente"): chi carica una nuova run (GameResetRunWithSeed,
       game/game.c) lo legge SUBITO dopo per marcare Game.curatedImageUsed,
       cosi' FusionPerform (gameplay/fusion.c) non ripesca in questa run
       un'immagine gia' assegnata come stato base (DEC-171: "fra le immagini
       non ancora usate nella run corrente"). Non serve per bossItem/enemies/
       bossType: solo items[] risolve un'immagine curata oggi. */
    int curatedImageIdx[3];
    /* Oggetto STAT-UP del piano, campo esplicito e non un quarto slot di
       items[] (scelta deliberata, vedi il report di fase locale
       .superpowers/sdd/phase3-items-report.md): src/render/game_renderer.c gia' itera
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

/* DEC-170: le taglie di stanza sono CLASSI DISCRETE stile Isaac, non piu' un
   lattice di rettangoli in pixel (M2). Ogni stanza occupa 1..4 celle CONTIGUE
   della griglia del piano; una cella e' grande esattamente ROOM_W x ROOM_H
   (il canvas logico di sempre), cosi' la taglia 1x1 -- il minimo garantito di
   DEC-009 -- resta l'inquadratura fissa di sempre.
   I nomi seguono il documento di design (rooms-and-floor-generation.md,
   "Taglie multiple e telecamera"): 1x2 e' la coppia ORIZZONTALE, 2x1 quella
   VERTICALE. */
typedef enum RoomSize {
    ROOM_SIZE_1X1 = 0,   /* una cella */
    ROOM_SIZE_1X2,       /* due celle in fila orizzontale */
    ROOM_SIZE_2X1,       /* due celle in colonna verticale */
    ROOM_SIZE_2X2,       /* blocco di quattro celle */
    ROOM_SIZE_L,         /* tre celle di un blocco 2x2 (un angolo mancante) */
    ROOM_SIZE_COUNT
} RoomSize;

/* Maschera delle celle occupate dentro il riquadro 2x2 di una stanza, relativa
   alla cella di ORIGINE (l'angolo in alto a sinistra del riquadro). Un solo
   byte descrive tutte e cinque le classi di DEC-170. */
#define ROOM_CELL_BIT(dx, dy) ((unsigned char)(1u << ((dy)*2 + (dx))))

/* DEC-170: una cella della griglia del piano; PIU' celle possono appartenere
   alla STESSA stanza.
 *
 * INVARIANTE (l'unica cosa da ricordare leggendo questa struttura): lo stato
 * MUTABILE di una stanza -- visited/cleared/rewardTaken -- e il suo 'kind'
 * vivono in UNA sola cella, la cella di STATO (la prima cella occupata in
 * ordine di lettura dentro il riquadro, vedi WorldRoomAt in src/world/world.h).
 * Le altre celle della stessa stanza li lasciano a zero: leggerli direttamente
 * da game->rooms[y][x] e' sbagliato per costruzione, si passa SEMPRE da
 * WorldRoomAt/WorldRoomAtMutable/GameCurrentRoom. Sono invece validi su OGNI
 * cella della stanza: 'exists', 'originX/originY', 'cells' e 'doors[]' (una
 * porta e' un fatto del LATO di UNA cella, non della stanza intera).
 *
 * 'cells' == 0 significa "cella mai passata dal generatore": si comporta come
 * una stanza 1x1 ancorata a se stessa (il hub del Piano 0, un Game di test
 * costruito a mano) -- stesso spirito del vecchio w/h <= 0. */
typedef struct RoomState {
    bool exists;
    bool visited;
    bool cleared;
    bool rewardTaken;
    RoomKind kind;
    bool doors[4];
    unsigned char cells;   /* maschera 2x2 (ROOM_CELL_BIT), relativa a originX/originY */
    int originX;           /* cella in alto a sinistra del RIQUADRO della stanza */
    int originY;
    /* DEC-183: gli Innesti sganciati volontariamente in QUESTA stanza (cella
       di STATO, CombatDropGraft in src/gameplay/combat.c) NON vivono qui --
       un campo singolo per stanza non basterebbe (in una stessa stanza
       possono coesistere piu' Innesti a terra: uno sganciato e uno lasciato
       da un piedistallo, o due sganci successivi prima di riprenderne
       nessuno) e replicare un array per RoomState moltiplicherebbe il costo
       di ogni Item (~2.6KB) per GRID_SIZE*GRID_SIZE=25 celle. Vivono invece
       in una lista GLOBALE al piano, Game.droppedGrafts (vedi
       DroppedGraftRecord sotto), ciascun record con un riferimento a QUALE
       cella di stato appartiene -- stesso azzeramento (memset di
       GameResetRun/GameResetRunWithSeed e di WorldGenerateFloorMap ad ogni
       nuovo piano), stessa semantica "non segue il giocatore tra i piani". */
} RoomState;

/* DEC-183: un Innesto lasciato a terra -- sganciato volontariamente
   (CombatDropGraft) o rimasto al posto di quello appena ripreso in uno
   scambio a slot pieni (CombatPickup, grafts.md "Drop e persistenza") --
   resta recuperabile per TUTTA LA RUN, nella stanza (cella di STATO,
   RoomState) in cui giace, anche se il giocatore esce e ripulisce altre
   stanze nel frattempo. WorldSpawnRoomContents ri-materializza un pickup per
   OGNI record 'active' la cui (roomX,roomY) coincide con la stanza corrente,
   ad OGNI ingresso (EntitiesClear la svuota comunque a ogni ingresso: senza
   questo l'Innesto sparirebbe, il gap che DEC-183 chiude). CombatPickup
   libera il record ('active' a falso) quando il giocatore lo riprende
   DAVVERO (nessuno scambio), o lo aggiorna con l'Innesto che resta al suo
   posto se lo riprende tramite scambio -- vedi Pickup.isPersistedGraft/
   droppedGraftSlot, che legano un pickup a terra al SUO record specifico
   (niente ambiguita' quando ce ne sono piu' di uno nella stessa stanza).
   MAX_DROPPED_GRAFTS e' un tetto comodo, non stretto: ogni cella di stato
   puo' concedere al massimo un oggetto come premio (ROOM_TREASURE/ROOM_SHOP,
   WorldSpawnRoomContents), quindi il numero di Innesti che possono ESISTERE
   su un piano (equipaggiati o a terra) non supera mai
   GRID_SIZE*GRID_SIZE=25; il tetto resta ben sopra quella soglia per
   lasciare margine. Se mai si esaurisse (difetto altrove, non raggiungibile
   con il contenuto attuale) CombatDropGraft ripiega sul comportamento
   pre-DEC-183 per QUEL sgancio soltanto (a terra solo per la visita
   corrente, invece di rifiutare l'azione o corrompere un altro record) e lo
   segnala con un fprintf(stderr, ...) -- stessa convenzione di
   ArtAtlas/RunCatalog per una condizione anomala non fatale. */
#define MAX_DROPPED_GRAFTS 32
typedef struct DroppedGraftRecord {
    bool active;
    int roomX;
    int roomY;
    Item item;
    Vector2 pos;
} DroppedGraftRecord;

typedef struct Player {
    Vector2 pos;
    float radius;
    float speed;
    int hp;
    int maxHp;
    int coins;
    int bombs;
    int keys;
    /* Catalizzatore di fusione (in-game: Flux, DEC-072): la risorsa che paga
       una fusione (DEC-022). Sta accanto alle altre tre scorte perche' e'
       una risorsa come loro -- si raccoglie, si spende, si mostra nell'HUD --
       e NON ha alcun cap (DEC-129: accumulo libero, il limite e' la rarita'
       delle fonti). Zero-default: un Player azzerato non puo' fondere, che e'
       esattamente la condizione d'ingresso di systems/item-fusion.md. */
    int flux;
    float damage;
    float fireDelay;
    float shotSpeed;
    float shotRadius;
    /* Fortuna (step C, curve alla Isaac): statistica come le altre -- parte da
       baseLuck, la ricalcola ScriptItemsRecomputeStats, e' clampata, e un
       on_evaluate Lua la vede come stats.luck. Oggi pilota la probabilita' del
       trait VAMP (vedi CombatDamageEnemy); e' il gancio per ogni futuro effetto
       "a probabilita'" (critici, drop, catena) senza doverne inventare uno nuovo
       ogni volta, esattamente come in Isaac. */
    float luck;
    float fireTimer;
    float invuln;
    /* W8: la direzione verso cui il personaggio GUARDA (una Direction, DIR_*),
       e da quanto tempo si sta muovendo senza interruzioni.
     *
       Servono perche' lo spritesheet del personaggio ha quattro camminate
       separate (walk_down/walk_up/walk_right/walk_left) piu' un idle, e
       nessuna delle due informazioni e' derivabile dalla sola posizione: la
       velocita' del giocatore non e' memorizzata (il movimento e' un delta
       applicato e dimenticato, vedi GameUpdatePlayer), e "fermo" va distinto
       da "si muove" per scegliere fra idle e camminata. Li scrive UN SOLO
       punto (GameUpdatePlayer, src/game/game.c) e il renderer li legge
       soltanto.
       Zero-default: DIR_UP con 'walkTime' 0 -- un Player azzerato guarda in su
       e sta fermo, cioe' mostra l'idle, che e' il fotogramma piu' innocuo da
       mostrare per un personaggio appena creato. */
    int animFacing;
    float walkTime;
    /* Maschera di trait che CombatFirePlayer mette sui colpi. Scritta da UN
       SOLO punto, ScriptItemsRecomputeStats, che la ricalcola da ZERO dagli
       oggetti posseduti come ogni altra statistica -- non piu' un OR
       accumulato al pickup. Il cambio e' obbligato dalle categorie con slot:
       un Innesto si sgancia (DEC-115/DEC-160) e un attivo si scambia sul
       piedistallo (DEC-117), e un OR monotono non si spegne mai, quindi i
       trait di un oggetto rimosso resterebbero sui colpi per il resto della
       run. Le sinergie continuano a NON derivare da questo campo (vedono gli
       oggetti direttamente, vedi il commento su Player.synergies sotto). */
    unsigned int traits;
    /* Tipo di colpo ATTIVO del giocatore (step C): NON e' un'unione dei tipi
       posseduti -- vince l'ULTIMO oggetto raccolto che ne porta uno (alla Isaac:
       l'ultima "tear replacement" vince), e ScriptItemsRecomputeStats lo
       ricalcola da zero come ogni altra statistica, quindi togliere quell'oggetto
       fa tornare automaticamente il tipo precedente, senza deriva.
       shotType.active falso = il colpo base di sempre. shotColor e' il colore
       dell'OGGETTO che ha dato il tipo (cosi' il colpo si vede a colpo d'occhio
       come "quello di quell'oggetto"); vale solo quando shotType.active e' vero,
       altrimenti il colpo resta sul theme.accent2 di sempre. */
    ShotTypeDef shotType;
    Color shotColor;
    /* Indice, dentro items[], dell'oggetto che ha dato il tipo di colpo attivo
       (-1 = nessuno). Ricalcolato da zero come tutto il resto. Serve alle sinergie
       (correzione da review): una sinergia e' fra DUE oggetti DIVERSI, quindi una
       regola che condiziona sul tipo di colpo deve sapere a chi attribuirlo --
       senza questo, un oggetto che porta un tipo di colpo "che salta" ed e' anche
       l'oggetto che "rallenta" sinergizzerebbe con SE' STESSO. */
    int shotTypeItem;
    /* M6b-3 (DEC-068): il colpo firmato del personaggio APPLICATO (copiato da
       CharacterDef.signatureShot da GamePlayerResetBaseStatsFor, {0}/'active'
       falso se il personaggio non ne ha uno o se nessun personaggio e'
       applicato). ScriptItemsRecomputeStats riparte da QUESTO invece che da
       "nessun tipo" ad ogni ricalcolo (esattamente come baseDamage ecc. sono
       il punto di partenza delle statistiche): il colpo firmato fa da BASE al
       posto del colpo base del motore, e un oggetto-colpo raccolto durante la
       run lo sostituisce ESATTAMENTE come sostituirebbe il colpo base (stessa
       riga di codice nel ciclo degli oggetti, nessun ramo speciale) --
       togliere quell'oggetto fa tornare il colpo firmato, non "nessun colpo",
       perche' il ricalcolo riparte sempre da qui. 'characterShotColor' e' il
       colore con cui questo colpo (quando attivo E non ancora sovrascritto da
       un oggetto) si disegna: la palette del personaggio stesso (non il
       colore di un oggetto, che qui non c'e'), scelta cosi' il colpo firmato
       si legge visivamente come "suo" fin dal primo sparo. */
    ShotTypeDef characterShotType;
    Color characterShotColor;
    /* Sinergie attive (step D, src/gameplay/synergies.h): un bit per SynergyId.
       E' una statistica come le altre -- ricalcolata da ZERO da
       ScriptItemsRecomputeStats sugli oggetti posseduti ORA, mai accumulata --
       quindi togliere un oggetto della coppia spegne la sinergia pulita, senza
       alcuna contabilita' da disfare. NON derivarla mai da Player.traits: quello
       e' un OR monotono che non si spegne piu'. */
    unsigned int synergies;
    Item items[MAX_ITEMS];
    int itemCount;
    /* Slot funzionali (systems/items-pools-and-rarity.md, "Slot"). NON sono
       un secondo inventario: attivi e Innesti restano dentro items[] come
       ogni altro oggetto -- "equipaggiato" significa esattamente "posseduto",
       e sganciare un Innesto significa toglierlo da items[]
       (ScriptItemsRemoveItem). E' la ragione per cui questi campi contano
       CAPIENZA e non contengono indici: un indice memorizzato qui si
       disallineerebbe alla prima rimozione, e un Game azzerato con memset
       (quello che fanno GameResetRun e mezza suite di test) lo lascerebbe
       puntato all'oggetto 0 invece che a "nessuno". Quale oggetto occupa
       quale slot lo deriva sempre src/gameplay/item_slots.h scorrendo
       items[] per categoria, in ordine di acquisizione.
       Valore <= 0 significa "il minimo di design", cioe' 1 (vedi
       ItemActiveSlotCount/ItemGraftSlotCount): cosi' un Player azzerato ha
       comunque il suo slot attivo e il suo slot Innesto, senza dipendere da
       chi lo ha costruito. Gli slot in piu' valgono SOLO per la run in corso
       (DEC-123): vivono qui dentro Player, che GameResetRun azzera. */
    int activeSlotCount;
    int graftSlotCount;
    /* Quale degli attivi posseduti risponde al tasto d'uso -- e quindi anche
       quale finisce sul piedistallo in uno scambio (DEC-117: "con piu' slot
       pieni, quello attualmente selezionato per l'attivazione"). E' un
       ORDINALE fra gli attivi posseduti (0 = il primo raccolto), non un
       indice in items[]: sopravvive intatto a qualunque rimozione, e lo zero
       di un memset e' gia' la scelta giusta. Chi lo legge lo clampa
       (ItemSelectedActiveIndex): un ordinale oltre il numero di attivi
       posseduti ricade sull'ultimo. */
    int activeSelected;
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
    float baseLuck;   /* 0 di partenza (step C): la fortuna e' un bonus, mai un requisito */
    /* M6a (DEC-033): tetto di salute BASE proprio del personaggio applicato
       (GamePlayerResetBaseStatsFor, src/game/game.c) -- ScriptItemsRecomputeStats
       clampa maxHp a [SCRIPT_ITEMS_MAX_HP_MIN, hpCap] invece del vecchio tetto
       assoluto [1,12]. <=0 significa "nessun personaggio applicato" (un Player
       azzerato con memset, come fanno ancora molti test che costruiscono un
       Game a mano senza passare da GamePlayerResetBaseStatsFor): in quel caso
       script_items.c ripiega sul tetto STORICO 12, cosi' nessun test esistente
       cambia esito. Non e' un tetto per-oggetto (quelli restano relativi a
       baseMaxHp, invariati) ne' tocca la salute temporanea/protettiva
       (Crust, DEC-008), che non e' rappresentata da un campo separato in
       questa versione del motore e quindi non e' mai soggetta a QUESTO clamp. */
    int hpCap;
} Player;

typedef struct Enemy {
    bool active;
    /* EnemyKind resta, ma ora dice UNA COSA SOLA: se e' un boss (punteggio,
       stanza, cella dell'atlas). Il COMPORTAMENTO non viene piu' da qui -- viene
       dal tipo, inventato dal modello (fase 3b, core/enemy_type.h). Tenerlo
       significa che ogni chiamante storico (il punteggio in CombatDamageEnemy, la
       scelta dello sprite in DrawEnemy, il portale del boss in world.c) continua a
       funzionare senza sapere nulla dei tipi. */
    EnemyKind kind;
    Vector2 pos;
    Vector2 vel;
    float radius;
    float hp;
    float maxHp;
    float speed;
    float cooldown;
    float slowTimer;
    /* Il tipo inventato dal modello (fase 3b). 'active' falso = i nemici storici,
       cioe' lo zero-default: un Enemy azzerato con memset si comporta ESATTAMENTE
       come prima di questa fase (insegue e non spara). Copiato per valore dentro
       l'Enemy, come ShotTypeDef dentro l'Item e per lo stesso motivo: il contenuto
       del piano puo' cambiare sotto i piedi (generazione pigra in sottofondo), un
       nemico gia' in campo no. */
    EnemyTypeDef type;
    /* Stato per-nemico dei movimenti a fasi (zig-zag, scatto): un angolo/fase che
       avanza da solo. Zero-default innocuo. 'firePhase' e' TENUTO SEPARATO da
       'phase' (correzione da review): il fuoco a corona (RING) ruota di raffica in
       raffica, e il movimento a scatto (CHARGE) usa 'phase' come timer di stato --
       condividere lo stesso campo faceva singhiozzare la deriva fra uno scatto e
       l'altro di un nemico che carica E spara a corona (una combinazione che il
       modello puo' inventare: movimento e fuoco sono manopole indipendenti). */
    float phase;
    float firePhase;
    float chargeTimer;
    /* W8: secondi che restano all'animazione 'hit' (la riga del colpo subito
       negli spritesheet di assets/art/enemies|bosses). Vive sull'Enemy e non
       nel renderer perche' "sono stato colpito ADESSO" e' un evento del
       gameplay, non uno stato derivabile da una posizione: CombatDamageEnemy e'
       l'unico a scriverlo, il ciclo dei nemici lo consuma, e il renderer lo
       legge soltanto. Zero-default (nessun colpo in corso): un nemico azzerato
       con memset mostra la camminata di sempre. */
    float hitFlash;
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
    /* Step C: la FORMA con cui il renderer disegna questo colpo (zero-default
       SHOT_FORM_ORB: ogni colpo creato prima di questa fase, ogni colpo nemico e
       ogni colpo generato da uno script Lua resta la palla di sempre senza che
       nessuno debba impostare nulla) e i salti di catena che gli restano
       (0 = nessuno). Sono sul COLPO e non sul giocatore perche' un colpo vive
       oltre lo sparo: deve ricordare come disegnarsi e se puo' ancora saltare
       anche dopo che il giocatore ha cambiato tipo di colpo. */
    ShotForm form;
    int chain;
    /* Vero se una sinergia ha toccato questo colpo (step D): il renderer gli
       disegna un anello in piu' attorno. Le sinergie DEVONO vedersi -- "non si
       notano" era esattamente il feedback che ha aperto questo passo. */
    bool synergized;
    /* Nemici che QUESTO colpo ha gia' colpito, un bit per slot di Game.enemies
       (step C). Esiste perche' senza di lei "perforare" non perforava: un colpo
       con pierce resta sovrapposto al nemico che ha appena attraversato per
       diversi frame (raggio del nemico ~23 px, il colpo ne percorre ~9 per
       frame), e il ciclo delle collisioni lo ricolpiva ad ogni frame -- bruciando
       tutta la perforazione sul PRIMO nemico e morendo prima di raggiungere il
       secondo. Con la maschera, pierce significa quello che promette: attraversi
       N nemici DIVERSI (la stessa semantica di Isaac).
       Zero-default = nessun nemico colpito, quindi ogni colpo nasce "pulito"
       senza che EntitiesAddShot debba fare nulla. Limite noto e accettato: gli
       slot dei nemici vengono riusati, quindi un colpo ancora vivo (vita massima
       ~1.2 s) potrebbe rifiutarsi di colpire un nemico NUOVO nato in uno slot che
       aveva gia' colpito -- un colpo mancato, mai un danno doppio: il fallimento
       cade sempre dalla parte sicura. */
    unsigned long long hitMask;
} Shot;

typedef struct Pickup {
    bool active;
    PickupKind kind;
    Vector2 pos;
    float radius;
    int value;
    int cost;
    Item item;
    /* "Il giocatore ci sta ancora sopra": vero quando questo pickup e' appena
       nato SOTTO il giocatore -- l'attivo che uno scambio ha appena lasciato
       sul piedistallo (DEC-117) o l'Innesto appena sganciato a terra
       (DEC-160). Senza, la raccolta scatterebbe di nuovo il frame successivo,
       perche' CombatUpdatePickups guarda solo la sovrapposizione dei raggi:
       lo scambio si ripeterebbe a ogni frame finche' il giocatore non si
       sposta, e sganciare un Innesto sarebbe impossibile.
       Non e' un timer (un timer si esaurisce mentre il giocatore e' ancora
       fermo li' sopra, e il problema torna): si spegne quando la
       sovrapposizione FINISCE, cioe' quando ci si allontana. Zero-default
       falso = raccoglibile subito, quindi ogni pickup gia' esistente e ogni
       test che ne costruisce uno a mano restano invariati. */
    bool locked;
    /* DEC-183: vero SOLO per il pickup che rappresenta un record di
       Game.droppedGrafts (sganciato via CombatDropGraft, o lasciato su un
       piedistallo da uno scambio successivo dello STESSO Innesto
       persistente) -- mai per un Innesto offerto da tesoro/negozio.
       Distingue "questo pickup deve tenere sincronizzato il SUO record in
       Game.droppedGrafts quando viene preso" da qualunque altro pickup di
       categoria Innesto, cosi' CombatPickup non tocca stato persistente per
       un Innesto che non c'entra (vedi il commento su CombatPickup,
       src/gameplay/combat.c). Zero-default falso: ogni pickup che non passa
       esplicitamente da qui resta un Innesto "normale", invariato. */
    bool isPersistedGraft;
    /* DEC-183: indice in Game.droppedGrafts del record che QUESTO pickup
       rappresenta -- significativo SOLO quando isPersistedGraft e' vero (in
       una stessa stanza possono coesistere piu' Innesti persistenti, uno per
       record: senza un riferimento diretto CombatPickup non saprebbe QUALE
       record aggiornare/liberare). -1 = nessuno (zero-default esplicito,
       stesso schema di isPersistedGraft: mai letto quando isPersistedGraft e'
       falso, ma esplicito per non affidarsi al caso). */
    int droppedGraftSlot;
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

/* W8: un'animazione di MORTE che continua a scorrere dopo che l'entita' non
 * c'e' piu'.
 *
 * Perche' non basta l'entita' stessa: un nemico morto esce di scena nello
 * STESSO istante in cui perde l'ultimo punto vita (enemy->active = false in
 * CombatDamageEnemy) -- e' l'invariante su cui poggiano punteggio, drop,
 * conteggio "stanza pulita" e la maschera hitMask dei colpi. Tenerlo "attivo
 * ma morente" per far scorrere i 4-6 fotogrammi della riga 'death' del suo
 * spritesheet avrebbe voluto dire rivedere ogni "if (e->active)" del motore:
 * un cambio di semantica grosso, per un effetto puramente visivo. Questi
 * effetti sono invece un secondo insieme, SOLO grafico -- niente collisione,
 * niente danno, niente punteggio, nessuna lettura da parte del gameplay --
 * con lo stesso ciclo di vita delle particelle (nascono da un evento, scorrono
 * a tempo, si spengono da soli, EntitiesClear li azzera).
 *
 * 'imageId' e' l'IMAGE-ID di chi e' morto (DEC-175(b)), non una chiave di file
 * ne' un puntatore: chi genera l'effetto sta in src/gameplay, che non conosce
 * la spartizione in categorie di assets/art/ e non deve cominciare a
 * conoscerla -- la risoluzione a spritesheet resta di src/assets
 * (ArtAtlasFindByImageId), letta dal renderer. Un puntatore, oltretutto, si
 * romperebbe al primo ArtAtlasShutdown, e Game deve restare POD azzerabile con
 * memset. Vuoto = nessuno sprite: l'effetto non disegna nulla (un nemico senza
 * originale artistico non lascia animazione di morte, come prima di W8). */
#define MAX_ART_FX 12
typedef struct ArtFx {
    bool active;
    char imageId[40];
    char anim[20];
    Vector2 pos;      /* il punto in cui l'ancora dello sprite va appoggiata (i "piedi") */
    /* La larghezza VOLUTA a schermo, in pixel, non un fattore di scala: chi
       genera l'effetto sta in src/gameplay e non sa quanti pixel abbia il
       fotogramma di quello spritesheet. La conversione in scala la fa il
       renderer (ArtScaleForWidth), che il manifest lo ha letto. */
    float wantedWidth;
    float elapsed;    /* secondi dall'inizio: e' l'argomento di ArtAnimFrameAt */
    float duration;   /* durata totale dell'animazione, oltre la quale si spegne */
    bool flipX;
    Color tint;
} ArtFx;

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
    /* Quinta callback, solo per gli oggetti ITEM_ACTIVE (l'attivazione
       volontaria di systems/active-items.md). Resta -1 per ogni altra
       categoria anche se lo script la definisse: vedi la difesa di
       tassonomia in ScriptItemsOnAcquire. Aggiunta QUI e non in
       ScriptCharacterRuntime di proposito -- il trait del personaggio non
       occupa lo slot attivo e non si "usa". */
    int useRef;
    int statsTableRef;
} ScriptItemRuntime;

/* M6b-2 (DEC-037): stato di runtime Lua per il trait UNICO del personaggio
   GENERATO -- stessa idea/stessi campi di ScriptItemRuntime sopra, ma per
   UNA sola sandbox indipendente dagli oggetti: il trait non occupa uno slot
   di Player.items[], non ha layer visivi di build screen, e vive/muore con
   la SELEZIONE del personaggio (vedi src/script/script_character.h), non
   con l'acquisto di un oggetto durante la run. Tipo separato invece di
   riusare ScriptItemRuntime apposta: quel tipo e' documentato "per l'oggetto
   nello slot i-esimo", e un campo Game.characterTrait di quel tipo avrebbe
   confuso chi legge (un solo trait non e' "uno slot d'inventario in piu'").
   'sandbox' void* per lo stesso motivo di ScriptItemRuntime.sandbox:
   game_types.h resta "core", solo script_character.c lo interpreta con un
   cast. */
typedef struct ScriptCharacterRuntime {
    void *sandbox;
    int evalRef;
    int fireRef;
    int hitRef;
    int tickRef;
    int statsTableRef;
} ScriptCharacterRuntime;

/* DEC-065/131/152 (ui/hud.md): una card di scoperta breve -- sprite (non
 * ancora, v1 e' solo testo: nessun campo sprite finche' l'HUD pixel art di
 * W7 non ne ha bisogno), nome e una riga di descrizione. La card e' solo
 * l'ANNUNCIO: la scheda completa vive nel Catalogo (RunCatalogEntry sopra),
 * e la REGISTRAZIONE li' (i flag Game.enemyEncountered/bossEncountered)
 * avviene al momento della scoperta vera (WorldSpawnCombatRoom/
 * WorldSpawnRoomContents), mai qui -- questa struct e mai letta da
 * RunCatalogWriteRun. */
typedef struct DiscoveryCard {
    char name[48];
    char line[160];
    /* W8: l'IMAGE-ID dello sprite da mostrare nella card (DEC-175(b)), che il
       documento chiede da sempre ("sprite, nome, una riga di descrizione") e
       che la v1 a solo testo non aveva. Vuoto = nessuno sprite, e la casella
       della card resta vuota: e' il caso di ogni scoperta il cui contenuto non
       passi dal layer di indirezione (un nemico inventato dal modello). */
    char imageId[40];
} DiscoveryCard;

/* DEC-131: cap della coda, valore provvisorio da playtest (~5, come da
 * hud.md, "Domande aperte residue"). Oltre il cap, GameQueueDiscoveryCard
 * scarta la piu' vecchia SENZA mostrarla, mai un errore o un troncamento
 * silenzioso di altro tipo. */
#define DISCOVERY_QUEUE_MAX 5

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
    /* DEC-171: cache delle texture curate gia' caricate (vedi
       CuratedTextureCache sopra). Come 'atlas', appartiene a Game e muore in
       GameUnloadAssets. */
    CuratedTextureCache curatedTextures;
    /* DEC-171: quali voci di assets/curated/manifest.json sono gia' state
       usate in QUESTA run -- un bit per indice di manifest. E' cio' che
       rende vera la clausola "fra le immagini NON ancora usate nella run
       corrente": azzerata dal memset di GameResetRunWithSeed insieme a tutto
       il resto, quindi ogni run riparte dal pacchetto intero. */
    unsigned char curatedImageUsed[CURATED_IMAGE_MASK_BYTES];
    /* Quante fusioni sono state completate in questa run: entra nella chiave
       deterministica del risultato (src/gameplay/fusion.h) cosi' fondere la
       STESSA coppia due volte nella stessa run non da' due volte lo stesso
       oggetto -- "il risultato esatto non e' mai conoscibile in anticipo"
       (item-fusion.md, Non-obiettivi) senza smettere di essere deterministico
       dal seed. Nessun tetto (DEC-125: nessun limite rigido di fusioni per
       run, il limite e' l'economia del catalizzatore). */
    int fusionCount;
    RoomState rooms[GRID_SIZE][GRID_SIZE];
    /* DEC-183: lista GLOBALE al piano degli Innesti lasciati a terra,
       persistenti per tutta la run -- vedi il commento su DroppedGraftRecord
       sopra. Azzerata da un memset ESPLICITO e SEPARATO in
       WorldGenerateFloorMap (subito dopo quello di game->rooms, stesso
       spirito ma campo diverso: non un effetto collaterale di essere vicino
       a 'rooms' in memoria) ad ogni nuovo piano, e dal memset che
       GameResetRun/GameResetRunWithSeed applicano a tutto Game. */
    DroppedGraftRecord droppedGrafts[MAX_DROPPED_GRAFTS];
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Shot shots[MAX_SHOTS];
    Pickup pickups[MAX_PICKUPS];
    Bomb bombs[MAX_BOMBS];
    Particle particles[MAX_PARTICLES];
    /* W8: le animazioni di morte ancora in scorrimento (vedi ArtFx sopra).
       Accanto alle particelle perche' hanno lo stesso ciclo di vita e lo
       stesso aggiornamento (GameUpdateParticles), non perche' siano entita'. */
    ArtFx artFx[MAX_ART_FX];
    /* Ostacoli solidi della stanza CORRENTE (fase 3c): ricostruiti da
       WorldSpawnRoomContents ogni volta che si entra in una stanza, a partire dal
       RoomLayoutDef del piano. Vuoto (obstacleCount 0) per le stanze senza layout e
       per quelle che non ne hanno (boss/tesoro/negozio: servono spazio).
       DEC-170: i PRIMI 'obstacleHoleCount' ostacoli non vengono da un layout --
       sono le celle del riquadro che NON appartengono alla stanza (l'angolo
       mancante di una forma a L). Sono solide come qualunque altro ostacolo
       (giocatore, nemici e colpi le trattano gia' cosi', senza una riga in
       piu' in combat.c), ma il renderer le salta perche' DrawRoom le disegna
       gia' come muro vero della stanza. Esistono per OGNI tipo di stanza, non
       solo per quelle di combattimento: un buco deve essere solido sempre. */
    Obstacle obstacles[MAX_OBSTACLES];
    int obstacleCount;
    int obstacleHoleCount;
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
    /* M6b-2 (DEC-037): runtime Lua del trait UNICO del personaggio generato
       -- vedi il commento su ScriptCharacterRuntime sopra. Vive/muore con la
       selezione del personaggio generato (ScriptItemsInit lo pilota dietro
       la facciata quando riceve un CharacterDef con traitHook non vuoto),
       non con l'inventario: per questo NON e' dentro itemScripts[]. Distrutto
       da ScriptItemsShutdown insieme alle sandbox degli oggetti (stesso
       ordine "prima del memset" di GameResetRun/FloorZeroEnter). */
    ScriptCharacterRuntime characterTrait;
    /* Bandiera sporca del sistema delle cache (spec, sezione 7): impostata
       da CombatApplyItem quando un oggetto viene acquisito, consumata una
       volta per frame da GameUpdate (ScriptItemsProcessDirty), che chiama
       ScriptItemsRecomputeStats solo se davvero necessario invece che ad
       ogni frame. */
    bool statsDirty;
    /* Input a EVENTO latchato dal livello applicativo (src/app), una volta per
       frame di finestra, e consumato dal primo passo di simulazione che lo
       legge. Con la simulazione a passo fisso (60 Hz) un frame video puo'
       contenere 0 o 2 passi: leggere IsKeyPressed() dentro la simulazione
       perderebbe l'evento nel primo caso e lo raddoppierebbe nel secondo.
       Gli input CONTINUI (IsKeyDown per movimento/mira) restano letti
       direttamente dentro la simulazione: rileggerli a ogni passo e' corretto. */
    bool bombQueued;    /* SPACE: piazzare una bomba al prossimo passo */
    bool resetQueued;   /* R (senza melting-gen): reset rapido della run */
    /* Stessa disciplina di bombQueued: sono eventi (IsKeyPressed), quindi il
       livello applicativo li latcha una volta per frame di finestra e il
       primo passo di simulazione che li legge li consuma. Un uso = una
       carica, anche su un frame che contiene due passi. */
    bool useActiveQueued;   /* E: usare l'oggetto attivo selezionato */
    bool dropGraftQueued;   /* G: sganciare l'Innesto equipaggiato (DEC-115/DEC-160) */
    /* Piano 0 sala d'attesa (M1b, systems/floor-zero.md + ui/generation-status.md):
       scritto SOLO da src/app (che possiede lo stato della generazione), letto
       da world/render. 'floorZeroExitOpen' diventa vero quando la pipeline di
       generazione del primo piano e' TERMINALE (successo o fallback, mai un
       blocco: DEC-002/DEC-020) e resta vero per il resto della permanenza nel
       Piano 0 -- riapre il varco nel muro (render dedicato, non una porta
       normale) e permette l'attraversamento. 'floorZeroExitCrossed' e' il
       segnale opposto, scritto SOLO da world (WorldHandleTransitions, quando
       il giocatore preme contro il varco aperto) e consumato da UpdateApp
       nello stesso frame in cui lo legge true (poi lo rimette a false): SOLO
       quel consumo puo' far scattare GameResetRun e il passaggio a Gameplay,
       mai l'apertura da sola (il giocatore puo' restare nel Piano 0 quanto
       vuole dopo che l'uscita si e' aperta). */
    bool floorZeroExitOpen;
    bool floorZeroExitCrossed;
    /* M5 (DEC-005): le carte-proposta del Piano 0, scritte SOLO da src/app
       (che possiede la generazione, esattamente come floorZeroExitOpen
       sopra), lette da world/render. 'themeCardCount' 0 = proposte non
       ancora pronte (mai carte a meta': FloorZeroEnter/AppLoadThemeCards/
       RunContentMakeFallbackThemeCards scrivono sempre un conteggio
       completo o niente). 'themeCardFocus' e' l'indice con la selezione da
       tastiera/pad DENTRO il pannello di scelta (requisito 9: sinistra/
       destra + conferma; DEC-075/W9: il mouse ci entra allo stesso modo,
       hover sposta questo indice e click conferma, RendererFloorZeroCardAt in
       src/render/game_renderer.c); 'themeCardsPanelOpen' e' quel pannello,
       apribile con TAB (o un click sul fumetto quando e' chiuso) SOLO finche'
       il tema non e' scelto -- non ruba i controlli di movimento (WASD) del
       Piano 0 giocabile (M1b).
       'themeChosenIndex' -1 = nessun tema scelto: la generazione completa
       (AppStartGeneration) parte SOLO alla scelta, mai prima (a differenza
       di prima di M5), e l'uscita del Piano 0 resta chiusa finche' non e'
       vero ENTRAMBE le cose, tema scelto E pipeline terminale (vedi
       AppOpenFloorZeroExit/il case APP_FLOOR_ZERO in src/app/app.c). */
    ThemeCard themeCards[THEME_CARD_MAX];
    int themeCardCount;
    int themeCardFocus;
    /* M6a: 'themeCardsPanelOpen' apre/chiude ora il pannello COMBINATO
       MONDI/PERSONAGGI (stesso tasto TAB, requisito 3 della spec M6a:
       "nessun tasto nuovo") -- il nome resta quello di M5 per non muovere
       tutti i punti che gia' lo leggono/scrivono, ma dal M6a in poi governa
       entrambe le sezioni. 'floorZeroPanelSection' (FLOOR_ZERO_PANEL_WORLDS/
       _CHARACTERS sopra) dice quale sezione ha il focus da tastiera in
       QUESTO momento: su/giu' la cambia, sinistra/destra e conferma
       agiscono SOLO dentro quella attiva. A differenza del tema (una scelta
       sola, mai piu' modificabile dopo AppConfirmThemeChoice), il pannello
       resta apribile e la sezione PERSONAGGI resta interattiva anche DOPO
       che il mondo e' stato scelto -- il personaggio e' "sempre" scegliebile
       finche' non si attraversa l'uscita (floor-zero.md, riga del
       Selettore personaggio: "Abilitato quando: Sempre"). */
    bool themeCardsPanelOpen;
    int themeChosenIndex;
    int floorZeroPanelSection;
    /* M6a (DEC-030/033): il personaggio scelto nel Piano 0, indice dentro
       la rosa curata (CharacterRosterGet, src/content/character_roster.h) --
       0..CHARACTER_COUNT-1. Da M6b-1 (DEC-014), il valore CHARACTER_COUNT
       (il quarto slot, subito dopo l'ultimo indice della rosa) significa
       invece "il personaggio GENERATO per questa run e' quello scelto":
       vedi GameResolveCharacterDef (src/game/game_internal.h), l'UNICO punto
       che deve interpretare questo campo -- ogni altro chiamante lo passa a
       quella funzione, mai a CharacterRosterGet direttamente, perche' un
       indice CHARACTER_COUNT passato li' ricadrebbe silenziosamente su
       Wayfinder (vedi il commento su CharacterRosterGet). A differenza di
       themeChosenIndex, NON parte da -1: FloorZeroEnter lo preseleziona
       SUBITO a 0 (Wayfinder, l'indice piu' vicino allo storico, vedi il
       commento li') cosi' "nessuno dei tre elementi [mondo, pipeline,
       personaggio] resta indefinito" resta vero anche senza una conferma
       esplicita (assunzione dichiarata, la open question sulla definitivita'
       della scelta resta aperta) -- la rosa base non ha un equivalente del
       "rifiuta e resta indefinito" del tema. 'characterCardFocus' e'
       l'indice col focus da tastiera DENTRO la sezione PERSONAGGI (0..
       GameCharacterCardCount(game)-1, stesso schema di themeCardFocus).
       Applicato SUBITO alla selezione (nell'hub, GamePlayerResetBaseStatsFor
       via AppConfirmCharacterChoice, src/app/app.c) e di nuovo dopo
       GameResetRun all'attraversamento (che azzera tutto il resto di Game
       con un memset: il case APP_FLOOR_ZERO in app.c cattura l'indice PRIMA
       di quella chiamata e lo riapplica subito dopo, vedi il commento li').
       Quando il personaggio scelto e' quello generato, l'indice da solo NON
       basta piu' dopo un memset (generatedCharacter sotto sparirebbe con
       lui): app.c/game.c catturano anche una COPIA della def generata prima
       di azzerare Game, e la riscrivono in generatedCharacter subito dopo. */
    int characterChosenIndex;
    int characterCardFocus;
    /* M6b-1 (DEC-014, prima fetta): il personaggio alternativo generato per
       QUESTA run -- canale dati DINAMICO, separato dalla rosa const
       (CharacterRosterGet), stesso schema di themeCards/themeCardCount
       sopra: 'generatedCharacterValid' e' la verita' su "la proposta e'
       arrivata ed e' valida" (la scrive AppLoadCharacterProposal, src/app/
       app.c, quando RunContentLoadCharacterProposal ha successo), mai un
       campo a meta'. Puo' diventare valido DOPO che il pannello PERSONAGGI
       e' gia' aperto (il propose finisce mentre il giocatore guarda le
       carte): il quarto slot compare quando arriva, senza spostare il focus
       da dove si trovava (DrawCharacterCards/il case APP_FLOOR_ZERO non
       toccano characterCardFocus quando la carta compare, solo quando la
       naviga il giocatore). Azzerato ad ogni nuovo ingresso in FloorZero
       (FloorZeroEnter, azzeramento MIRATO come i themeCards) e mai
       ripristinato da GameResetRun (che lo azzera con tutto il resto):
       resta vivo attraverso un attraversamento SOLO se il case
       APP_FLOOR_ZERO lo ricopia qui subito dopo, esattamente come fa per
       characterChosenIndex. */
    CharacterDef generatedCharacter;
    bool generatedCharacterValid;
    /* M7 (DEC-015/041/045/069, substrato del catalogo persistente): quali TIPI
       di nemico/boss il giocatore ha DAVVERO incontrato in questa run, per
       piano -- non un contatore di uccisioni, un flag "questo tipo e'
       comparso mentre il giocatore era nella stanza". Lo spawn avviene
       SEMPRE da WorldSpawnRoomContents/WorldSpawnCombatRoom (src/world/
       world.c), chiamate SOLO quando il giocatore entra in una stanza: "il
       giocatore e' presente" e' quindi garantito per costruzione, nessun
       controllo aggiuntivo necessario li' dentro. Indicizzato [piano-1][0|1]
       per i due slot nemico del piano (vedi FloorContent.enemies), azzerato
       a inizio run da GameResetRun col resto di Game (memset).
       'bossEncountered'/'bossDefeated' distinguono "il boss e' comparso" da
       "il boss e' stato sconfitto" (RunCatalog registra l'esito separato,
       vedi src/content/run_catalog.c): defeated implica sempre encountered
       nella pratica (WorldCheckRoomClear lo marca solo DOPO lo spawn), ma il
       campo resta indipendente, nessuna deduzione implicita in chi legge. */
    bool enemyEncountered[FLOOR_COUNT][2];
    bool bossEncountered[FLOOR_COUNT];
    bool bossDefeated[FLOOR_COUNT];
    /* DEC-065/131/152 (ui/hud.md): coda delle card di scoperta non ancora
       mostrate -- 'discoveryQueueCount' voci valide in
       'discoveryQueue[0..discoveryQueueCount-1]', FIFO (GameUpdate promuove
       sempre l'indice 0 in 'discoveryActive' quando questa e' libera). Push
       SOLO da GameQueueDiscoveryCard (mai un accesso diretto all'array): e'
       lei che applica il cap DISCOVERY_QUEUE_MAX (DEC-131). 'discoveryActive'/
       'discoveryActiveValid'/'discoveryActiveTimer' sono la card CORRENTEMENTE
       mostrata (non bloccante, stesso ritmo di 'message'/'messageTimer' sotto):
       a differenza della coda, NON si scarta a morte/cambio stanza (DEC-152
       tocca solo le voci ancora in attesa, mai quella gia' annunciata).
       GameDiscardPendingDiscoveries (chiamata da CombatDamagePlayer alla morte
       e da WorldTryEnterRoom al cambio stanza) azzera SOLO 'discoveryQueueCount':
       mai enemyEncountered/bossEncountered sopra, che restano scritti al
       momento della scoperta -- la scoperta resta registrata nel Catalogo
       anche quando la card che l'annunciava viene scartata qui. */
    DiscoveryCard discoveryQueue[DISCOVERY_QUEUE_MAX];
    int discoveryQueueCount;
    DiscoveryCard discoveryActive;
    bool discoveryActiveValid;
    float discoveryActiveTimer;
    /* DEC-169 (systems/floor-zero.md): hook pronto per le PROVE del Piano 0
       (arene di sfida opzionali e tutorial integrato, DEC-004/047) -- non
       ancora implementate nel motore (matrice di copertura, sezione 17):
       nessun codice imposta mai questo flag a vero oggi, resta sempre falso
       per costruzione (zero-default del memset di GameResetRun*). Quando le
       arene arriveranno, l'entrata/uscita dalla prova scrivera' qui vero/falso
       e l'HUD di combattimento ricomparira' nel Piano 0 automaticamente (vedi
       HudCombatShouldDraw in src/render/game_renderer.c), senza dover
       ritoccare la logica di disegno. Gap esplicito stile DEC-009/052. */
    bool floorZeroTrialActive;
    /* DEC-159 (ui/results-and-leaderboards.md): l'ultimo colpo o nemico
       letale, scritto SOLO da CombatDamagePlayer quando la salute scende a
       zero (mai a vittoria, mai ad abbandono: in quei casi resta la stringa
       vuota dello zero-default). Letto da DrawRunResultsOverlay per la riga
       "Causa: ...", SOLO quando game->phase == PHASE_GAME_OVER. */
    char deathCause[96];
    /* M7: quanti record il catalogo ha scritto per l'ULTIMA chiamata di
       AppWriteRunCatalog (src/app/app.c) -- 0 se non si e' scritto nulla
       (run fallback, nessun piano giocato, guardia test attiva). Letto SOLO
       da DrawRunResultsOverlay (src/render/game_renderer.c) per la riga
       "Creazioni registrate nel catalogo: N" (05-game-states-and-flow.md,
       righe 83-85): non serve altrove, quindi non e' protetto da alcuna
       logica di sopravvivenza a un memset come characterChosenIndex sopra --
       torna a 0 con ogni nuova run, esattamente come deve. */
    int catalogRecordsWritten;
    GamePhase phase;
    /* DEC-141: il seed della run corrente (quello scelto/condiviso in
       RunSetup, o un valore orologio quando nessuna generazione l'ha
       ancora deciso -- vedi GameResetRun/GameResetRunWithSeed in
       src/game/game.c). 'rng' sotto NON e' questo valore: e' derivato da
       'runSeed' con uno splitmix64 a costante di dominio, cosi' il flusso
       di gameplay non condivide mai lo stream con la generazione (che
       riceve 'runSeed' grezzo, vedi RunContentLoad). Sopravvive al reset
       rapido R esattamente come characterChosenIndex (capture/restore in
       GameUpdate), cosi' la stessa run riparte con la stessa sequenza. */
    unsigned int runSeed;
    unsigned int rng;
    /* DEC-145 -- correzione di fortuna: estrazioni consecutive di rarita'
       comune viste da ciascun pool, indipendenti fra loro (la stessa
       sequenza sfortunata sul tesoro non deve consumare la soglia del
       negozio). Azzerate dal memset di GameResetRunWithSeed come ogni altro
       campo di stato della run; aggiornate da ItemPoolDrawIndex
       (src/gameplay/item_pool.h) ai due punti di estrazione (tesoro/negozio,
       src/world/world.c, WorldSpawnRoomContents). Nessun contatore per il
       pool boss: quel pool non contiene mai rarita' comune per costruzione
       (pesi DEC-019 {0,0,70,30}), la correzione vi resta definita ma non
       puo' mai attivarsi -- vedi il commento su ItemPoolLuckCorrectionActive. */
    int treasureLuckStreak;
    int shopLuckStreak;
    int floor;
    /* DEC-170: UNA cella della stanza corrente -- per convenzione la sua cella
       di STATO (vedi RoomState sopra), quella che porta kind/visited/cleared.
       Con le stanze multi-cella NON e' piu' "la cella in cui cammina il
       giocatore": quella si ricava dalla posizione (WorldPlayerCell,
       src/world/world.h) e cambia senza cambiare stanza. */
    int roomX;
    int roomY;
    /* DEC-170: punto del MONDO inquadrato al centro del canvas. Per una stanza
       1x1 vale sempre il centro della cella (telecamera ferma, inquadratura di
       sempre); nelle taglie maggiori insegue il giocatore a zoom fisso,
       clampato dai bordi (src/world/room_camera.h). Aggiornato dalla
       simulazione (WorldUpdateCamera), non dal renderer: e' stato di gioco,
       cosi' resta identico a parita' di passi simulati. */
    Vector2 cameraTarget;
    int score;
    int roomNumber;
    char message[160];
    float messageTimer;
} Game;

typedef struct UiLayout {
    /* DEC-137: una sola superficie. La game view (il canvas 960x640 campionato
       POINT) riempie TUTTO lo schermo -- niente piu' colonne riservate alla UI,
       che ora vive in overlay sopra il canvas (DrawOuterUi, ancorata ai bordi di
       gameRect). Di quel layout a pannelli restano solo le tre grandezze del
       canvas: dove sta, quanto e' scalato, e la scala del chrome sovrapposto. I
       vecchi leftPanel/rightPanel/bottomPanel sono spariti col layout a colonne. */
    Rectangle gameRect;
    float gameScale;
    /* M4 (fullscreen-first): fattore di scala dell'INTERFACCIA in overlay (HUD,
       font, overlay dei menu) -- MAI del canvas di gioco, che resta 960x640 sempre
       e usa la sua scala indipendente 'gameScale' sopra. Derivato dalla sola
       altezza dello schermo (UiComputeLayoutFor, src/render/game_renderer.c) e
       quantizzato a passi di 0.25 per restare coerente col gusto "a scatti" del
       resto del progetto (gameScale a passi di 1/8, minimappa a taglie discrete).
       1.0 per qualunque finestra <=900px di altezza: e' cio' che garantisce che le
       finestre di test compatte (960x640, smoke test) e la finestra grande di
       riferimento (1600x900, screenshot test) restino bit-per-bit identiche a
       prima di M4. */
    float uiScale;
} UiLayout;

/* Progresso del generatore esterno (melting-gen), letto da gen_progress.txt. */
typedef struct GenProgress {
    char phase[32];
    int percent;
    char message[96];
} GenProgress;

/* M8 (DEC-045, vista Catalogo v1 -- enciclopedia consultabile dal MainMenu):
 * la FORMA di una voce aggregata e del suo contenitore, letta on-demand da
 * TUTTI i file .txt di catalog/ (RunCatalogAggregate, src/content/run_catalog.c)
 * -- vive qui in core, non in src/content, per lo STESSO motivo di ThemeCard
 * sopra: sia AppUi (che la possiede, vedi il campo 'catalog' sotto) sia
 * src/render (che la disegna) ne hanno bisogno, e core e' l'unico modulo che
 * entrambi gia' includono senza creare una dipendenza all'indietro (content
 * dipende da core, mai il contrario, vedi AGENTS.md). L'ordine dei sette
 * valori e' quello con cui la vista elenca le categorie (sinistra/destra):
 * mondi/temi, layout, oggetti, tipi di colpo, nemici, boss, personaggi
 * generati -- lo stesso ordine della spec M8. */
typedef enum RunCatalogCategory {
    RUN_CATALOG_CAT_WORLD,
    RUN_CATALOG_CAT_LAYOUT,
    RUN_CATALOG_CAT_ITEM,
    RUN_CATALOG_CAT_SHOT,
    RUN_CATALOG_CAT_ENEMY,
    RUN_CATALOG_CAT_BOSS,
    RUN_CATALOG_CAT_CHARACTER,
    RUN_CATALOG_CATEGORY_COUNT   /* non e' una categoria vera: conta quelle sopra, per dimensionare gli array */
} RunCatalogCategory;

/* Tetto DICHIARATO per categoria (spec M8, stile "buffer fissi" del
 * progetto): oltre questo numero di voci DISTINTE in una categoria, le
 * occorrenze aggiuntive contano solo in RunCatalogSummary.overflowCount,
 * mai un'allocazione dinamica ne' una scrittura fuori banda. 256 e' larghezza
 * di margine per una collezione che cresce per run intere (un file per run,
 * mai per singolo oggetto): anche giocando centinaia di run, oggetti/nemici/
 * boss distinti restano ben sotto -- il tetto esiste per la garanzia, non
 * perche' ci si aspetti di raggiungerlo davvero. */
#define RUN_CATALOG_ENTRY_MAX 256

/* Una voce aggregata: TUTTE le occorrenze con lo stesso (categoria, nome) in
 * TUTTI i file .txt di catalog/ confluiscono qui. 'detail' e' testo gia' pronto
 * per la UI (mai ricostruito a ogni frame di disegno) -- la PRIMA descrizione
 * vista per questo nome, mai sovrascritta dalle successive (spec M8: slot/
 * rarita'/tratti per gli oggetti, forma/movimento per i nemici, ruolo/trait
 * hook/colpo firmato per i personaggi -- RunCatalogAggregate la compone per
 * categoria, vedi run_catalog.c). 'encounterCount' conta OGNI occorrenza
 * (anche piu' volte nella stessa run, es. lo stesso nemico su due piani);
 * 'runCount' conta le run DISTINTE in cui e' comparso (al massimo una volta
 * per file, indipendentemente da quante volte compare dentro). 'bossDefeated'
 * ha significato SOLO per RUN_CATALOG_CAT_BOSS: vero se sconfitto in ALMENO
 * una delle run aggregate (spec M8: "per i boss: sconfitto si'/no" -- un
 * singolo bool basta, la vista non deve raccontare la storia run-per-run). */
typedef struct RunCatalogEntry {
    char name[40];
    char detail[128];
    int encounterCount;
    int runCount;
    unsigned int firstSeed;
    unsigned int lastSeed;
    bool bossDefeated;
} RunCatalogEntry;

/* Il contenitore intero: un array FISSO di voci per ciascuna delle sette
 * categorie sopra, mai una lista dinamica (stile del progetto, AGENTS.md).
 * 'entryCount[cat]' e' quante voci sono davvero popolate in
 * 'entries[cat][0..entryCount[cat]-1]' (il resto dell'array resta a zero,
 * mai letto); 'overflowCount[cat]' e' quante occorrenze IN PIU' (oltre le
 * prime RUN_CATALOG_ENTRY_MAX voci distinte) RunCatalogAggregate ha dovuto
 * scartare per quella categoria -- la vista le mostra come "e altre N",
 * mai silenziosamente. 'filesRead'/'filesSkipped' sono la controprova
 * osservabile della robustezza (spec M8: "file corrotti/troncati -> voce
 * saltata in silenzio, log stderr, mai crash") -- utili soprattutto ai test,
 * la vista non li mostra. */
typedef struct RunCatalogSummary {
    RunCatalogEntry entries[RUN_CATALOG_CATEGORY_COUNT][RUN_CATALOG_ENTRY_MAX];
    int entryCount[RUN_CATALOG_CATEGORY_COUNT];
    int overflowCount[RUN_CATALOG_CATEGORY_COUNT];
    int filesRead;
    int filesSkipped;
} RunCatalogSummary;

/* Stato di navigazione UI (M1a): posseduto e mutato SOLO da UpdateApp
   (src/app/app.c), ma vive qui in "core" -- non in un header di src/app --
   perche' src/render lo deve LEGGERE per disegnare (voce col focus
   evidenziata, valore del seed in RunSetup, testo di contesto in
   ExitConfirm): stessa ragione per cui AppMode e GenProgress stanno gia' qui
   invece che in src/app. AppInput (gli eventi grezzi da tastiera/mouse di UN
   frame) resta invece un tipo INTERNO di src/app: il renderer non ne ha mai
   bisogno, vedi src/app/app_internal.h.
   'focus' e' l'indice della voce con la selezione da tastiera nella
   schermata ATTIVA (il significato dell'indice e' locale a ciascuno stato,
   deciso in UpdateApp e rispecchiato SOLO nel disegno di quello stato in
   game_renderer.c: 0 e' sempre la prima voce elencata nel documento UI
   corrispondente). 'openedFrom'/'returnFocus' sono la coppia che implementa
   la regola "il focus torna sull'elemento che ha aperto la schermata"
   (ui/navigation-map.md): chi apre Options/BuildScreen/ExitConfirm scrive
   SEMPRE returnFocus = il proprio focus corrente PRIMA di cambiare stato, cosi'
   un singolo "back" generico funziona per tutti e tre senza sapere da dove
   viene. 'exitAbandonsRun' distingue i due contesti di ExitConfirm (DEC-057:
   MainMenu/Esci = uscita dal gioco, PauseMenu/Abbandona = abbandono run).
   'seed' e' il seed scelto/proposto in RunSetup: e' la fonte di verita' che
   AppStartGeneration usa per avviare la run (mai piu' un seed generato al
   volo dentro AppStartGeneration, vedi il commento li'). */
typedef struct AppUi {
    int focus;
    AppMode openedFrom;
    int returnFocus;
    bool exitAbandonsRun;
    unsigned int seed;
    /* M7 (substrato del catalogo persistente): guardia test-safe per
       AppWriteRunCatalog (src/app/app.c) -- vive qui, non su AppGen, perche'
       AppUi e' l'UNICO dei parametri di UpdateApp che sopravvive intatto a
       GameResetRun/FloorZeroEnter (Game viene azzerato a meta' run in piu'
       punti, vedi i commenti su characterChosenIndex/generatedCharacter
       sopra) ed e' gia' uno dei tre argomenti dell'hook per spec. Zero-default
       (falso): OGNI test C che chiama UpdateApp direttamente costruisce la
       propria "AppUi ui = {0}" (src/tests/game_tests.c e affini), quindi il
       catalogo resta disattivato per costruzione in tutta quella suite senza
       bisogno di una condizione esplicita in ognuno. I due soli punti che lo
       accendono: AppRun (src/app/app.c, il gioco vero: sempre tranne che nei
       vari *Test raggiunti dal SUO stesso main loop, vedi il commento li') e
       il test dedicato --catalog-test (src/tests/catalog_tests.c), che lo
       accende a mano sulla propria AppUi locale per esercitare la scrittura
       vera con UpdateApp. */
    bool catalogWritesEnabled;
    /* M8 (DEC-045, vista Catalogo v1): la vista vive DENTRO APP_MAIN_MENU
       (nessun nuovo AppMode, nota architetturale della spec M8) -- questi
       campi la governano esattamente come 'focus' governa le voci del menu,
       ma sono un ramo SEPARATO nel case APP_MAIN_MENU di UpdateApp (vedi
       src/app/app.c): quando 'catalogOpen' e' vero, su/giu/sinistra/destra
       muovono la vista, mai le voci del menu sottostante. 'catalogOpen'
       false = si disegna DrawMainMenuOverlay come sempre; vero =
       DrawCatalogOverlay lo sostituisce (game_renderer.c), col menu (e
       ui->focus, gia' fermo su 1/"Catalogo" da quando la vista si e'
       aperta) intatto sotto, cosi' un ESC torna li' senza ricalcolare nulla.
       'catalogCategory' e' l'indice (RunCatalogCategory) con la selezione
       sinistra/destra; 'catalogItemFocus' l'indice su/giu' DENTRO quella
       categoria -- azzerato ad ogni cambio di categoria (mai un indice che
       sopravvive a una lista diversa da quella per cui e' stato scelto).
       'catalog' e' l'aggregato letto ON-DEMAND (RunCatalogAggregate,
       src/content/run_catalog.c) alla CONFERMA della voce "Catalogo", mai
       per-frame: catalog/ e' dati del giocatore su disco, non cambia
       mentre la vista resta aperta (a differenza di 'catalogWritesEnabled'
       sopra, che governa la SCRITTURA a fine run, questo campo governa
       solo la LETTURA per la consultazione -- due percorsi indipendenti). */
    bool catalogOpen;
    int catalogCategory;
    int catalogItemFocus;
    RunCatalogSummary catalog;
    /* --- Fusione dentro BuildScreen (ui/inventory-and-synergy-screen.md,
       "Fusioni possibili"; systems/item-fusion.md) ------------------------
       Stessa scelta dei campi del Catalogo qui sopra: la fusione non e' un
       decimo stato applicativo, e' un RAMO dentro APP_BUILD_SCREEN, quindi
       il suo stato di interfaccia vive qui accanto a 'focus' e non in Game
       (che viene azzerato a meta' run in piu' punti).
       'buildItemFocus' e' l'indice in Player.items[] con la selezione da
       tastiera (su/giu'); 'fusionSourceA'/'fusionSourceB' sono i due slot
       sorgente scelti, nell'ORDINE in cui il giocatore li ha scelti --
       l'ordine conta, e' il tie-break di DEC-143 e del punto 4 di
       "Priorita' e conflitti". I due slot sono memorizzati come "indice + 1"
       (FUSION_UI_SLOT/FUSION_UI_FIELD, src/gameplay/fusion.h): cosi' lo ZERO
       di un AppUi azzerato significa "nessuna sorgente scelta" e non "il
       primo oggetto dell'inventario", la stessa disciplina zero-default di
       ITEM_PASSIVE/RARITY_COMMON. 'fusionMessage' e' l'esito leggibile
       dell'ultimo tentativo (errore o riuscita), 'fusionResultName'/
       'fusionResultImage' il risultato da mostrare (nome + immagine curata,
       DEC-171). Tutti azzerati da "{0}", che e' anche lo stato giusto
       all'ingresso: nessuna sorgente scelta, nessun messaggio. */
    int buildItemFocus;
    /* W9 correzione round 1 (BOCCIATO, "l'anello di retroazione della lista
       scorrevole"): l'ANCORA di scorrimento della lista OGGETTI PRESI --
       l'indice del PRIMO oggetto mostrato nella finestra visibile -- vive qui,
       SEPARATA da 'buildItemFocus'. Prima era DERIVATA dal focus ("first =
       focus - maxShow + 1"): con l'hover del mouse che scrive il focus, la
       mappatura punto->riga dipendeva dal proprio risultato e la lista
       scorreva da sola di uno step per frame di movimento (12 oggetti,
       finestra da 3: schizzava in cima in ~5 frame, annullando la rotellina).
       Adesso la finestra dipende SOLO da questo campo, quindi la mappatura
       "punto dello schermo -> indice di oggetto" e' indipendente dal focus e
       l'anello e' rotto per costruzione. Chi muove il focus (su/giu',
       rotellina, esito di una fusione) lo tiene visibile chiamando
       AppBuildScrollFollowFocus in src/app/app.c: l'ancora si sposta SOLO
       quando il focus e' uscito dalla finestra, mai per un hover (che per
       definizione cade su una riga gia' visibile). Zero-default (0) = finestra
       in cima, lo stato giusto all'ingresso come per i campi qui accanto. */
    int buildItemScroll;
    int fusionSourceA;
    int fusionSourceB;
    char fusionMessage[96];
    char fusionResultName[48];
    char fusionResultImage[64];
    /* W8: l'IMAGE-ID del risultato, accanto al percorso curato e per lo stesso
       motivo di Item.imageId (vedi il commento li'): la fascia FUSIONE deve
       mostrare l'originale animato quando esiste, il ponte CC0 altrimenti.
       Copiato dal fuso da AppFusionConfirm, insieme a nome e percorso. */
    char fusionResultImageId[40];
    /* DEC-184 (ui/hud.md, "Blocco statistiche"): preferenza di VISIBILITA'
       del blocco compatto danno/cadenza/vel.colpo/vel.movimento/raggio/Fortuna
       nell'HUD di Gameplay, non stato di run -- vive qui apposta, come
       'catalogWritesEnabled'/'buildItemFocus' sopra, cosi' sopravvive intatta
       a GameResetRun/FloorZeroEnter (un giocatore che nasconde il blocco non
       se lo ritrova acceso alla run successiva). Zero-default (falso) =
       blocco VISIBILE: il documento chiede "visibile di default", quindi il
       flag e' l'inverso ("nascosto"), non "visibile", per restare nella
       disciplina zero-default del resto di questo struct (vedi il commento
       su fusionSourceA/B sopra) senza dover inizializzare nulla a mano nei
       tanti "AppUi ui = { 0 }" dei test. Tasto di toggle: C (default proposto
       dall'implementazione, stile DEC-019 -- vedi AppInput.toggleStats). */
    bool hudStatsHidden;
    /* W9 (playtest round 1, "mouse ovunque"): stato del trascinamento di una
       barra di Opzioni -- vive qui come 'buildItemFocus'/'hudStatsHidden'
       sopra, campo di INTERFACCIA, mai di run. 'optionsDragging' zero-default
       (falso) = nessun trascinamento in corso, cosi' ogni "AppUi ui = {0}" dei
       test resta innocuo per costruzione, come sempre. 'optionsDraggingIndex'
       (0..2: generale/musica/effetti) si legge SOLO quando 'optionsDragging'
       e' vero -- UpdateApp lo scrive al MOUSE_BUTTON_LEFT premuto su una
       barra e lo rilascia quando il tasto torna su, in APP_OPTIONS. */
    bool optionsDragging;
    int optionsDraggingIndex;
    /* W9, correzione di Fable: la riga di conferma della fascia FUSIONE e'
       cliccabile ma non aveva alcun feedback al passaggio del mouse (unica
       superficie cliccabile senza: le voci di menu e le righe oggetto lo
       hanno tramite il focus spostato dall'hover). UpdateApp lo scrive ogni
       frame in APP_BUILD_SCREEN, il renderer lo legge per evidenziare la
       riga. Zero-default falso: nessun hover. */
    bool fusionConfirmHover;
    /* W9 correzione round 0 (BOCCIATO, "il puntatore fermo uccide tastiera/
       pad"): l'hover generico e le due geometrie aggiuntive (righe di
       BuildScreen, carte/schedine del Piano 0) devono spostare il focus SOLO
       quando il puntatore si e' spostato DAVVERO da un frame all'altro --
       altrimenti un puntatore semplicemente lasciato fermo su una voce (la
       situazione normale dopo un click, o quando il giocatore torna a
       tastiera/pad) lo riscriverebbe ad ogni frame, cancellando ogni
       navigazione da tastiera/pad (rompe DEC-057) e persino il default
       "Annulla" di ExitConfirm. 'mouseTracked' zero-default (falso) = "mai
       osservato ancora": il PRIMO frame in cui UpdateApp legge il mouse
       applica comunque l'hover una volta (stesso comportamento di sempre,
       richiesto da GameMouseHoverFocusTest: ogni scenario costruisce una
       "AppUi ui = {0}" fresca e si aspetta l'hover al primissimo sguardo) --
       da li' in poi l'hover si applica SOLO se 'lastMousePos' e' cambiata.
       Un solo confronto in cima a UpdateApp copre tutti e tre i punti che
       leggono il mouse in modo continuo (il passo generico, BuildScreen,
       Piano 0): scrivono tutti nella stessa 'lastMousePos', letta una volta
       sola per frame. I CLICK restano eventi discreti e non passano da
       questo gate (IsMouseButtonPressed va sempre onorato, si muova o no il
       mouse in quel frame). */
    bool mouseTracked;
    Vector2 lastMousePos;
} AppUi;

#endif
