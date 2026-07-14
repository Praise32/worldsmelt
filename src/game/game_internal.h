#ifndef MELTING_RUN_GAME_INTERNAL_H
#define MELTING_RUN_GAME_INTERNAL_H

#include "game/game.h"

void GameSetMessage(Game *game, const char *message);

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
Vector2 EntitiesRandomRoomPosition(unsigned int *rng, float padding);
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
