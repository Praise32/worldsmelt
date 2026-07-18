#ifndef MELTING_RUN_GAME_INTERNAL_H
#define MELTING_RUN_GAME_INTERNAL_H

#include "game/game.h"

void GameSetMessage(Game *game, const char *message);

/* Valori di PARTENZA del Player (radius/hp/coins/bombs/keys + i base* del
   sistema delle cache), estratti da GameResetRun perche' FloorZeroEnter
   (src/world/floor_zero.c) ne ha bisogno anche lei per preparare un Player
   pulito nel Piano 0 -- SENZA passare da GameResetRun, che leggerebbe
   generated/ (vietato li', vedi il commento su FloorZeroEnter). Una sola
   fonte dei numeri: se cambiano qui, cambiano per entrambe senza rischio che
   divergano in silenzio. Non tocca ScriptItemsInit ne' i campi derivati
   (damage/fireDelay/.../maxHp): quello resta a carico del chiamante, DOPO
   aver chiamato questa funzione (stesso ordine di GameResetRun). */
void GamePlayerResetBaseStats(Player *player);
/* M6a: la stessa cosa, ma per il personaggio APPLICATO (NULL = comportamento
   storico, vedi il commento nell'implementazione in game.c). */
void GamePlayerResetBaseStatsFor(Player *player, const CharacterDef *character);

/* M6b-1 (DEC-014, prima fetta): risolve un indice di
   Game.characterChosenIndex (0..CHARACTER_COUNT-1 = rosa curata,
   CHARACTER_COUNT = il personaggio generato per QUESTA run) nella
   CharacterDef giusta -- l'UNICO punto che deve sapere che quel valore
   speciale esiste, cosi' app.c/game.c/game_renderer.c non duplicano la
   stessa scelta if/else in tre posti diversi. Ritorna NULL quando index e'
   fuori range, o quando e' CHARACTER_COUNT ma game->generatedCharacterValid
   e' falso (proposta non ancora arrivata, o scartata): il chiamante deve
   trattarlo come "nessun personaggio applicato" (lo stesso -1 storico),
   MAI come un accesso alla rosa fuori banda -- CharacterRosterGet da sola
   ricadrebbe silenziosamente su Wayfinder per un indice cosi', che sarebbe
   sbagliato qui. */
const CharacterDef *GameResolveCharacterDef(const Game *game, int index);

/* M6b-1: quante carte mostra/naviga la sezione PERSONAGGI del pannello del
   Piano 0 in QUESTO momento -- CHARACTER_COUNT (la rosa curata) piu' UNO
   quando game->generatedCharacterValid e' vero (il quarto slot dinamico).
   Fonte unica di questo conteggio: la naviga app.c (wrap del focus), la
   disegna game_renderer.c (DrawCharacterCards) -- due copie della stessa
   formula avrebbero potuto divergere in silenzio se una delle due
   dimenticasse il +1. */
int GameCharacterCardCount(const Game *game);

RoomState *WorldCurrentRoomMutable(Game *game);
bool WorldNoEnemiesActive(const Game *game);
void WorldStartFloor(Game *game, int floor);
void WorldSpawnRoomContents(Game *game);
/* Fase 3b: la stanza di combattimento non spawna "N nemici", SPENDE un budget di
   difficolta' (ogni nemico costa la propria potenza). Non piu' static: il test AG
   (src/tests/script_items_tests.c) verifica proprio quella garanzia. */
void WorldSpawnCombatRoom(Game *game);
void WorldTryEnterRoom(Game *game, int direction);
void WorldHandleTransitions(Game *game, Vector2 move);
void WorldCheckRoomClear(Game *game);

void EntitiesClear(Game *game);
/* M2: 'room' e' il rettangolo dentro cui pescare (di norma
   WorldCurrentRoomRect(game), ma il chiamante decide -- questo file non
   sa nulla di "stanza corrente"). */
Vector2 EntitiesRandomRoomPosition(unsigned int *rng, Rectangle room, float padding);
void EntitiesAddParticle(Game *game, Vector2 position, Color color, int count);
void EntitiesAddEnemy(Game *game, EnemyKind kind, Vector2 position);
/* Fase 3b: il nemico con un TIPO inventato dal modello (core/enemy_type.h).
   'type' NULL/non attivo = esattamente EntitiesAddEnemy (i nemici storici). */
void EntitiesAddEnemyTyped(Game *game, EnemyKind kind, Vector2 position, const EnemyTypeDef *type);
Shot *EntitiesAddShot(
    Game *game,
    bool fromPlayer,
    Vector2 position,
    Vector2 direction,
    float speed,
    float damage,
    float radius,
    unsigned int traits,
    Color color
);
void EntitiesAddPickup(Game *game, PickupKind kind, Vector2 position, int value, int cost);
int EntitiesCountActivePickups(const Game *game, PickupKind kind);
void EntitiesAddItemPickup(Game *game, Vector2 position, Item item, int cost);

void AssetsLoad(Game *game);

void CombatDamagePlayer(Game *game, int amount);
void CombatDamageEnemy(Game *game, Enemy *enemy, float damage, unsigned int traits);
void CombatExplodeAt(Game *game, Vector2 position, float radius, float damage);
void CombatSplitShot(Game *game, const Shot *shot);
void CombatFirePlayer(Game *game, Vector2 direction);
void CombatUpdatePlayer(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame);
void CombatUpdateEnemies(Game *game, float dt);
void CombatUpdateShots(Game *game, float dt);
void CombatUpdatePickups(Game *game);
void CombatUpdateBombs(Game *game, float dt);

void ScriptVmExecutePlayer(
    Game *game,
    ScriptTrigger trigger,
    Vector2 position,
    Vector2 direction,
    float damage,
    unsigned int traits,
    int scriptDepth
);

#endif
