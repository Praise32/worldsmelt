#include "gameplay/entities.h"

#include "core/game_math.h"
#include "gameplay/item_slots.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void EntitiesClear(Game *game)
{
    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->shots, 0, sizeof(game->shots));
    memset(game->pickups, 0, sizeof(game->pickups));
    memset(game->bombs, 0, sizeof(game->bombs));
    memset(game->particles, 0, sizeof(game->particles));
    /* W8: gli effetti grafici seguono le particelle -- stesso ciclo di vita,
       stesso punto di azzeramento (un cambio di stanza non deve mostrare la
       morte di un nemico della stanza precedente). */
    memset(game->artFx, 0, sizeof(game->artFx));
}

/* M2: il rettangolo lo decide il chiamante (WorldCurrentRoomRect o simili) --
   questo file non include world/world.h apposta (src/gameplay non deve sapere
   COME si calcola "la stanza corrente", solo dentro quale rettangolo pescare
   un punto). */
Vector2 EntitiesRandomRoomPosition(unsigned int *rng, Rectangle room, float pad)
{
    return (Vector2){
        GameRngFloat(rng, room.x + pad, room.x + room.width - pad),
        GameRngFloat(rng, room.y + pad, room.y + room.height - pad)
    };
}

void EntitiesAddParticle(Game *game, Vector2 pos, Color color, int count)
{
    for (int n = 0; n < count; n++)
    {
        for (int i = 0; i < MAX_PARTICLES; i++)
        {
            Particle *p = &game->particles[i];
            if (p->active) continue;
            float a = GameRngFloat(&game->rng, 0.0f, PI_F*2.0f);
            float s = GameRngFloat(&game->rng, 35.0f, 190.0f);
            p->active = true;
            p->pos = pos;
            p->vel = (Vector2){ cosf(a)*s, sinf(a)*s };
            p->life = GameRngFloat(&game->rng, 0.18f, 0.55f);
            p->radius = GameRngFloat(&game->rng, 2.0f, 5.0f);
            p->color = color;
            break;
        }
    }
}

/* W8: accoda un'animazione puramente GRAFICA (vedi ArtFx in
   core/game_types.h). 'imageId' vuoto o slot tutti occupati = non succede
   nulla: un effetto perso e' invisibile, mai un errore. 'duration' la decide
   chi chiama, perche' questo modulo non legge i manifest degli spritesheet (li
   legge src/assets/art_atlas.c) e quindi non puo' sapere quanti fotogrammi
   abbia la riga 'death' di quel nemico. Una durata piu' LUNGA del dovuto e'
   innocua: l'animazione non cicla, quindi resta ferma sull'ultimo fotogramma
   per il tempo che avanza, mai su un fotogramma sbagliato. */
void EntitiesAddArtFx(Game *game, const char *imageId, const char *anim, Vector2 pos,
                      float wantedWidth, float duration, bool flipX, Color tint)
{
    if (!imageId || !imageId[0] || !anim || !anim[0] || duration <= 0.0f) return;
    for (int i = 0; i < MAX_ART_FX; i++)
    {
        ArtFx *fx = &game->artFx[i];
        if (fx->active) continue;
        memset(fx, 0, sizeof(*fx));
        fx->active = true;
        snprintf(fx->imageId, sizeof(fx->imageId), "%s", imageId);
        snprintf(fx->anim, sizeof(fx->anim), "%s", anim);
        fx->pos = pos;
        fx->wantedWidth = wantedWidth;
        fx->duration = duration;
        fx->flipX = flipX;
        fx->tint = tint;
        return;
    }
}

/* Le BASI del motore, da cui i moltiplicatori del tipo (fase 3b) partono. Sono
   quelle storiche dell'inseguitore/boss: un nemico senza tipo (zero-default) resta
   quindi identico a prima di questa fase. MODIFICA QUI per ribilanciare la
   difficolta' di fondo del gioco, non nei tipi (quelli li inventa il modello). */
#define ENEMY_BASE_HP      24.0f
#define ENEMY_BASE_SPEED   98.0f
#define ENEMY_BASE_RADIUS  15.0f
#define ENEMY_BOSS_BASE_HP     150.0f
#define ENEMY_BOSS_BASE_SPEED   54.0f
#define ENEMY_BOSS_BASE_RADIUS  42.0f

/* Il nemico TIPIZZATO (fase 3b): stesse basi scalate per piano di sempre, modulate
   dai moltiplicatori del tipo che il modello ha inventato. 'type' NULL o non attivo
   -> il nemico storico corrispondente a 'kind', identico a prima. */
void EntitiesAddEnemyTyped(Game *game, EnemyKind kind, Vector2 pos, const EnemyTypeDef *type)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (e->active) continue;
        memset(e, 0, sizeof(*e));
        game->enemyGen[i]++;
        for (int s = 0; s < MAX_SHOTS; s++) game->shots[s].hitMask &= ~(1ull << i);

        e->active = true;
        e->kind = kind;
        e->pos = pos;
        e->cooldown = GameRngFloat(&game->rng, 0.45f, 1.35f);
        e->phase = GameRngFloat(&game->rng, 0.0f, PI_F*2.0f);   /* orbita/zig-zag: fasi diverse, cosi' non si muovono all'unisono */
        if (type && type->active) e->type = *type;

        bool isBoss = (kind == ENEMY_BOSS);
        float floorScale = 1.0f + (float)(game->floor - 1)*0.20f;
        float baseHp     = isBoss ? (ENEMY_BOSS_BASE_HP + 52.0f*(float)game->floor + ((game->floor == FLOOR_COUNT) ? 150.0f : 0.0f)) : ENEMY_BASE_HP*floorScale;
        float baseSpeed  = isBoss ? (ENEMY_BOSS_BASE_SPEED + 4.0f*(float)game->floor) : (ENEMY_BASE_SPEED + 7.0f*(float)game->floor);
        float baseRadius = isBoss ? ((game->floor == FLOOR_COUNT) ? 52.0f : ENEMY_BOSS_BASE_RADIUS) : ENEMY_BASE_RADIUS;

        if (e->type.active)
        {
            e->hp = baseHp*e->type.hpMul;
            e->speed = baseSpeed*e->type.speedMul;
            e->radius = baseRadius*e->type.sizeMul;
        }
        else
        {
            /* Nessun tipo: i quattro nemici storici, invariati. */
            e->hp = baseHp;
            e->speed = baseSpeed;
            e->radius = baseRadius;
            if (kind == ENEMY_SHOOTER) { e->radius = 17.0f; e->hp = 24.0f*floorScale; e->speed = 68.0f + 4.0f*(float)game->floor; }
            else if (kind == ENEMY_TANK) { e->radius = 23.0f; e->hp = 45.0f*floorScale; e->speed = 48.0f + 3.0f*(float)game->floor; }
            else if (kind == ENEMY_CHASER) { e->hp = 18.0f*floorScale; }
        }
        if (isBoss) e->cooldown = 0.65f;
        e->maxHp = e->hp;
        return;
    }
}

void EntitiesAddEnemy(Game *game, EnemyKind kind, Vector2 pos)
{
    EntitiesAddEnemyTyped(game, kind, pos, NULL);
}

Shot *EntitiesAddShot(Game *game, bool fromPlayer, Vector2 pos, Vector2 dir, float speed, float damage, float radius, unsigned int traits, Color color)
{
    dir = GameMathNormalize(dir);
    if (GameMathLengthSquared(dir) <= 0.0001f) return NULL;
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        Shot *s = &game->shots[i];
        if (s->active) continue;
        memset(s, 0, sizeof(*s));
        /* Stessa generazione di EntitiesAddEnemy sopra, per Game.shotGen. */
        game->shotGen[i]++;
        s->active = true;
        s->fromPlayer = fromPlayer;
        s->pos = pos;
        s->vel = GameMathScale(dir, speed);
        s->radius = radius;
        s->damage = damage;
        s->life = fromPlayer ? 1.15f : 2.6f;
        s->traits = traits;
        s->bounces = (traits & TRAIT_BOUNCE) ? 2 : 0;
        s->pierce = (traits & TRAIT_PIERCE) ? 2 : 0;
        s->color = color;
        return s;
    }
    return NULL;
}

void EntitiesAddPickup(Game *game, PickupKind kind, Vector2 pos, int value, int cost)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *p = &game->pickups[i];
        if (p->active) continue;
        p->active = true;
        p->kind = kind;
        p->pos = pos;
        p->value = value;
        p->cost = cost;
        p->locked = false;   /* uno slot riciclato non eredita il blocco di uno scambio precedente */
        p->radius = (kind == PICKUP_ITEM) ? 20.0f : 14.0f;
        return;
    }
}

int EntitiesCountActivePickups(const Game *game, PickupKind kind)
{
    int count = 0;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (game->pickups[i].active && game->pickups[i].kind == kind) count++;
    }
    return count;
}

void EntitiesAddItemPickup(Game *game, Vector2 pos, Item item, int cost)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *p = &game->pickups[i];
        if (p->active) continue;
        p->active = true;
        p->kind = PICKUP_ITEM;
        p->pos = pos;
        p->item = item;
        p->cost = cost;
        p->locked = false;
        p->radius = 22.0f;
        /* Un oggetto OFFERTO dal gioco arriva carico: e' qui che un attivo
           passa da "scheda di contenuto" (FloorContent, che non ha stato di
           ricarica) a "oggetto vero nel mondo". Farlo invece al momento della
           raccolta sarebbe una ricarica gratis: lo scambio col piedistallo
           (DEC-117) rimette in circolo l'attivo del giocatore scrivendo
           direttamente Pickup.item, senza passare di qui, proprio perche' le
           sue cariche consumate devono restare consumate. */
        ItemActiveResetCharge(&p->item);
        return;
    }
}
