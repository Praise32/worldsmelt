#include "gameplay/entities.h"

#include "core/game_math.h"

#include <math.h>
#include <string.h>

void EntitiesClear(Game *game)
{
    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->shots, 0, sizeof(game->shots));
    memset(game->pickups, 0, sizeof(game->pickups));
    memset(game->bombs, 0, sizeof(game->bombs));
    memset(game->particles, 0, sizeof(game->particles));
}

Vector2 EntitiesRandomRoomPosition(unsigned int *rng, float pad)
{
    return (Vector2){
        GameRngFloat(rng, ROOM_X + pad, ROOM_RIGHT - pad),
        GameRngFloat(rng, ROOM_Y + pad, ROOM_BOTTOM - pad)
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

void EntitiesAddEnemy(Game *game, EnemyKind kind, Vector2 pos)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (e->active) continue;
        e->active = true;
        e->kind = kind;
        e->pos = pos;
        e->cooldown = GameRngFloat(&game->rng, 0.45f, 1.35f);
        float scale = 1.0f + (float)(game->floor - 1)*0.20f;

        if (kind == ENEMY_CHASER)
        {
            e->radius = 15.0f;
            e->hp = 18.0f*scale;
            e->speed = 98.0f + 7.0f*(float)game->floor;
        }
        else if (kind == ENEMY_SHOOTER)
        {
            e->radius = 17.0f;
            e->hp = 24.0f*scale;
            e->speed = 68.0f + 4.0f*(float)game->floor;
        }
        else if (kind == ENEMY_TANK)
        {
            e->radius = 23.0f;
            e->hp = 45.0f*scale;
            e->speed = 48.0f + 3.0f*(float)game->floor;
        }
        else
        {
            e->radius = (game->floor == FLOOR_COUNT) ? 52.0f : 42.0f;
            e->hp = 150.0f + 52.0f*(float)game->floor + ((game->floor == FLOOR_COUNT) ? 150.0f : 0.0f);
            e->speed = 54.0f + 4.0f*(float)game->floor;
            e->cooldown = 0.65f;
        }
        e->maxHp = e->hp;
        return;
    }
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
        p->radius = 22.0f;
        return;
    }
}
