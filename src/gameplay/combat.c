#include "gameplay/combat.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "script/script_items.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static Enemy *CombatNearestEnemy(Game *game, Vector2 pos)
{
    Enemy *best = NULL;
    float bestD = 9999999.0f;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        float d = GameMathLengthSquared(GameMathSubtract(e->pos, pos));
        if (d < bestD)
        {
            bestD = d;
            best = e;
        }
    }
    return best;
}

void CombatDamagePlayer(Game *game, int amount)
{
    if (game->player.invuln > 0.0f || game->phase != PHASE_PLAY) return;
    game->player.hp -= amount;
    game->player.invuln = 0.85f;
    EntitiesAddParticle(game, game->player.pos, RED, 22);
    if (game->player.hp <= 0)
    {
        game->phase = PHASE_GAME_OVER;
        GameSetMessage(game, "Run finita. Premi R.");
    }
}

void CombatDamageEnemy(Game *game, Enemy *enemy, float damage, unsigned int traits)
{
    /* Guardia contro il doppio credito di morte: ScriptItemsOnHit (chiamata
       PRIMA di questa funzione, vedi il ciclo colpi qui sotto) puo' gia' far
       morire questo stesso nemico se lo script Lua dell'oggetto chiama
       damage_enemy(id, amount) nel suo on_hit (script_api.c,
       ScriptApiDamageEnemy chiama proprio questa funzione). Senza questa
       guardia, la CombatDamageEnemy "incorporata" che segue nel ciclo colpi
       rientrerebbe nel ramo hp<=0 una seconda volta sullo STESSO nemico
       (gia' enemy->active=false, ma questa funzione non lo controllava):
       punteggio +30/+300 due volte, particelle doppie, e un secondo tiro
       indipendente di vamp. Un nemico non attivo e' gia' "morto" a tutti
       gli effetti: nessun danno ulteriore ha senso applicargli. */
    if (!enemy->active) return;

    enemy->hp -= damage;
    if (traits & TRAIT_SLOW) enemy->slowTimer = 1.6f;
    EntitiesAddParticle(game, enemy->pos, game->theme.accent2, 4);
    if (enemy->hp <= 0.0f)
    {
        enemy->active = false;
        game->score += (enemy->kind == ENEMY_BOSS) ? 300 : 30;
        EntitiesAddParticle(game, enemy->pos, enemy->kind == ENEMY_BOSS ? game->theme.boss : game->theme.enemy, 22);
        if ((traits & TRAIT_VAMP) && game->player.hp < game->player.maxHp && GameRngRange(&game->rng, 0, 100) < 18)
        {
            game->player.hp++;
        }
    }
}

void CombatExplodeAt(Game *game, Vector2 pos, float radius, float damage)
{
    EntitiesAddParticle(game, pos, ORANGE, 42);
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        float r = radius + e->radius;
        if (GameMathLengthSquared(GameMathSubtract(e->pos, pos)) < r*r) CombatDamageEnemy(game, e, damage, 0);
    }
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        if (!game->shots[i].fromPlayer && game->shots[i].active && GameMathLengthSquared(GameMathSubtract(game->shots[i].pos, pos)) < radius*radius)
        {
            game->shots[i].active = false;
        }
    }
}

void CombatSplitShot(Game *game, const Shot *shot)
{
    for (int i = 0; i < 4; i++)
    {
        float a = (float)i*PI_F*0.5f + GameRngFloat(&game->rng, -0.14f, 0.14f);
        Shot *spawned = EntitiesAddShot(game, true, shot->pos, (Vector2){ cosf(a), sinf(a) }, 380.0f, shot->damage*0.45f, 4.0f, shot->traits & ~(unsigned int)TRAIT_SPLIT, shot->color);
        if (spawned) spawned->scriptDepth = shot->scriptDepth + 1;
    }
}

void CombatFirePlayer(Game *game, Vector2 dir)
{
    Player *p = &game->player;
    unsigned int traits = p->traits;
    float radius = p->shotRadius + ((traits & TRAIT_GIANT) ? 5.0f : 0.0f);
    float damage = p->damage + ((traits & TRAIT_GIANT) ? 4.0f : 0.0f);
    float speed = p->shotSpeed*((traits & TRAIT_GIANT) ? 0.88f : 1.0f);
    int pellets = (traits & TRAIT_SPLIT) ? 2 : 1;
    float angle = atan2f(dir.y, dir.x);
    for (int i = 0; i < pellets; i++)
    {
        float offset = ((float)i - (float)(pellets - 1)*0.5f)*0.18f;
        EntitiesAddShot(game, true, p->pos, (Vector2){ cosf(angle + offset), sinf(angle + offset) }, speed, damage, radius, traits, game->theme.accent2);
    }
    /* Lua prima, mini-VM dopo: sono a prova reciproca, non in cascata.
       ScriptVmExecutePlayer (src/gameplay/script_vm.c) salta da solo ogni
       oggetto per cui ScriptItemsHasActiveLua e' vero, quindi ogni oggetto
       viene eseguito esattamente una volta, o dall'uno o dall'altro mai da
       entrambi (patto di sicurezza, spec sezione 9: "l'oggetto ripiega sulla
       mini-VM", non "in aggiunta alla mini-VM"). */
    ScriptItemsOnFire(game, p->pos, dir);
    ScriptVmExecutePlayer(game, SCRIPT_ON_FIRE, p->pos, dir, damage, traits, 0);
    p->fireTimer = p->fireDelay*((traits & TRAIT_RAPID) ? 0.62f : 1.0f);
}

static void CombatPlaceBomb(Game *game)
{
    if (game->player.bombs <= 0)
    {
        GameSetMessage(game, "Nessuna bomba.");
        return;
    }
    for (int i = 0; i < MAX_BOMBS; i++)
    {
        Bomb *b = &game->bombs[i];
        if (b->active) continue;
        b->active = true;
        b->pos = game->player.pos;
        b->timer = 1.05f;
        b->radius = 74.0f;
        game->player.bombs--;
        GameSetMessage(game, "Bomba piazzata.");
        return;
    }
}

void CombatUpdatePlayer(Game *game, float dt, Vector2 mouseGame, bool mouseInsideGame)
{
    Player *p = &game->player;
    Vector2 move = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W)) move.y -= 1.0f;
    if (IsKeyDown(KEY_S)) move.y += 1.0f;
    if (IsKeyDown(KEY_A)) move.x -= 1.0f;
    if (IsKeyDown(KEY_D)) move.x += 1.0f;
    move = GameMathNormalize(move);
    p->pos = GameMathAdd(p->pos, GameMathScale(move, p->speed*dt));
    p->pos.x = GameMathClampFloat(p->pos.x, ROOM_X + p->radius, ROOM_RIGHT - p->radius);
    p->pos.y = GameMathClampFloat(p->pos.y, ROOM_Y + p->radius, ROOM_BOTTOM - p->radius);
    WorldHandleTransitions(game, move);

    if (p->invuln > 0.0f) p->invuln -= dt;
    if (p->fireTimer > 0.0f) p->fireTimer -= dt;
    ScriptItemsOnTick(game, dt);

    Vector2 aim = { 0.0f, 0.0f };
    if (mouseInsideGame && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) aim = GameMathSubtract(mouseGame, p->pos);
    if (GameMathLengthSquared(aim) <= 0.01f)
    {
        if (IsKeyDown(KEY_UP)) aim.y -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) aim.y += 1.0f;
        if (IsKeyDown(KEY_LEFT)) aim.x -= 1.0f;
        if (IsKeyDown(KEY_RIGHT)) aim.x += 1.0f;
    }
    aim = GameMathNormalize(aim);
    if (p->fireTimer <= 0.0f && GameMathLengthSquared(aim) > 0.01f) CombatFirePlayer(game, aim);

    if (IsKeyPressed(KEY_SPACE)) CombatPlaceBomb(game);
}

void CombatUpdateEnemies(Game *game, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        Vector2 toPlayer = GameMathSubtract(game->player.pos, e->pos);
        Vector2 dir = GameMathNormalize(toPlayer);
        float slow = e->slowTimer > 0.0f ? 0.45f : 1.0f;
        if (e->slowTimer > 0.0f) e->slowTimer -= dt;

        Vector2 move = dir;
        if (e->kind == ENEMY_SHOOTER)
        {
            float dist = sqrtf(GameMathLengthSquared(toPlayer));
            if (dist < 210.0f) move = GameMathScale(dir, -0.65f);
        }
        else if (e->kind == ENEMY_BOSS)
        {
            move = GameMathNormalize(GameMathAdd(GameMathScale(dir, 0.5f), GameMathScale(GameMathPerpendicular(dir), sinf((float)GetTime()*1.4f)*0.75f)));
        }

        e->pos = GameMathAdd(e->pos, GameMathScale(move, e->speed*slow*dt));
        /* Impulso di spinta (es. ScriptApiSetEnemyVelocity, un contraccolpo
           impostato da on_hit di un oggetto Lua): si somma al movimento
           dell'IA sopra, poi si smorza esponenzialmente, stesso schema di
           Particle.vel in GameUpdateParticles. Resta zero (nessun effetto)
           per qualunque nemico che nessuno script ha mai toccato. */
        e->pos = GameMathAdd(e->pos, GameMathScale(e->vel, dt));
        e->vel = GameMathScale(e->vel, 0.90f);
        e->pos.x = GameMathClampFloat(e->pos.x, ROOM_X + e->radius, ROOM_RIGHT - e->radius);
        e->pos.y = GameMathClampFloat(e->pos.y, ROOM_Y + e->radius, ROOM_BOTTOM - e->radius);
        e->cooldown -= dt;

        if ((e->kind == ENEMY_SHOOTER || e->kind == ENEMY_TANK) && e->cooldown <= 0.0f)
        {
            EntitiesAddShot(game, false, e->pos, dir, 245.0f + 14.0f*(float)game->floor, 1.0f, 6.0f, 0, game->theme.enemy);
            e->cooldown = e->kind == ENEMY_TANK ? 1.4f : 1.0f;
        }
        else if (e->kind == ENEMY_BOSS && e->cooldown <= 0.0f)
        {
            int count = (game->floor == FLOOR_COUNT) ? 14 : 8 + game->floor;
            for (int s = 0; s < count; s++)
            {
                float a = (float)s*PI_F*2.0f/(float)count + (float)GetTime()*0.22f;
                EntitiesAddShot(game, false, e->pos, (Vector2){ cosf(a), sinf(a) }, 215.0f + 10.0f*(float)game->floor, 1.0f, 7.0f, 0, game->theme.boss);
            }
            if (game->floor == FLOOR_COUNT && GameRngRange(&game->rng, 0, 100) < 45)
            {
                EntitiesAddEnemy(game, ENEMY_CHASER, EntitiesRandomRoomPosition(&game->rng, 60.0f));
            }
            e->cooldown = (game->floor == FLOOR_COUNT) ? 0.85f : 1.18f;
        }

        float touch = e->radius + game->player.radius;
        if (GameMathLengthSquared(GameMathSubtract(e->pos, game->player.pos)) < touch*touch) CombatDamagePlayer(game, 1);
    }
}

void CombatUpdateShots(Game *game, float dt)
{
    for (int i = 0; i < MAX_SHOTS; i++)
    {
        Shot *s = &game->shots[i];
        if (!s->active) continue;

        if (s->fromPlayer && (s->traits & TRAIT_HOMING))
        {
            Enemy *target = CombatNearestEnemy(game, s->pos);
            if (target)
            {
                Vector2 desired = GameMathNormalize(GameMathSubtract(target->pos, s->pos));
                Vector2 cur = GameMathNormalize(s->vel);
                Vector2 mix = GameMathNormalize(GameMathAdd(GameMathScale(cur, 0.90f), GameMathScale(desired, 0.10f)));
                float speed = sqrtf(GameMathLengthSquared(s->vel));
                s->vel = GameMathScale(mix, speed);
            }
        }

        s->pos = GameMathAdd(s->pos, GameMathScale(s->vel, dt));
        s->life -= dt;
        bool wall = false;
        if (s->pos.x < ROOM_X + s->radius || s->pos.x > ROOM_RIGHT - s->radius)
        {
            s->vel.x *= -1.0f;
            s->pos.x = GameMathClampFloat(s->pos.x, ROOM_X + s->radius, ROOM_RIGHT - s->radius);
            wall = true;
        }
        if (s->pos.y < ROOM_Y + s->radius || s->pos.y > ROOM_BOTTOM - s->radius)
        {
            s->vel.y *= -1.0f;
            s->pos.y = GameMathClampFloat(s->pos.y, ROOM_Y + s->radius, ROOM_BOTTOM - s->radius);
            wall = true;
        }
        if (wall)
        {
            if (s->fromPlayer && s->bounces > 0) s->bounces--;
            else s->active = false;
        }
        if (!s->active) continue;
        if (s->life <= 0.0f)
        {
            if (s->fromPlayer && (s->traits & TRAIT_EXPLODE)) CombatExplodeAt(game, s->pos, 45.0f, s->damage*0.65f);
            s->active = false;
            continue;
        }

        if (s->fromPlayer)
        {
            for (int e = 0; e < MAX_ENEMIES; e++)
            {
                Enemy *enemy = &game->enemies[e];
                if (!enemy->active) continue;
                float r = s->radius + enemy->radius;
                if (GameMathLengthSquared(GameMathSubtract(s->pos, enemy->pos)) >= r*r) continue;
                /* ScriptItemsOnHit PRIMA di CombatDamageEnemy (a differenza
                   della mini-VM sotto, che e' sempre girata dopo): uno script
                   Lua deve poter leggere lo stato del nemico com'era AL
                   MOMENTO dell'impatto (es. enemy_hp(id) prima di questo
                   colpo), non un handle che CombatDamageEnemy potrebbe aver
                   gia' disattivato (nemico ucciso da questo stesso colpo). */
                ScriptItemsOnHit(game, i, e);
                CombatDamageEnemy(game, enemy, s->damage, s->traits);
                ScriptVmExecutePlayer(game, SCRIPT_ON_HIT, s->pos, GameMathNormalize(s->vel), s->damage, s->traits, s->scriptDepth);
                if ((s->traits & TRAIT_EXPLODE)) CombatExplodeAt(game, s->pos, 50.0f + s->radius*2.0f, s->damage*0.55f);
                if ((s->traits & TRAIT_SPLIT) && !s->splitDone)
                {
                    CombatSplitShot(game, s);
                    s->splitDone = true;
                }
                if (s->pierce > 0)
                {
                    s->pierce--;
                    s->damage *= 0.72f;
                }
                else s->active = false;
                break;
            }
        }
        else
        {
            float r = s->radius + game->player.radius;
            if (GameMathLengthSquared(GameMathSubtract(s->pos, game->player.pos)) < r*r)
            {
                s->active = false;
                CombatDamagePlayer(game, 1);
            }
        }
    }
}

static void CombatApplyItem(Game *game, Item item)
{
    Player *p = &game->player;
    int itemIndex;
    if (p->itemCount < MAX_ITEMS) { itemIndex = p->itemCount; p->items[p->itemCount++] = item; }
    else { itemIndex = MAX_ITEMS - 1; p->items[itemIndex] = item; }
    p->traits |= item.traits;

    /* Il calcolo di damage/fireDelay/shotSpeed/shotRadius/maxHp da
       trait/slot NON vive piu' qui (una tantum, al pickup): vive dentro
       ScriptItemsRecomputeStats (src/script/script_items.c,
       ScriptItemsApplyBuiltin), ricalcolato da zero ad ogni passaggio
       insieme all'eventuale on_evaluate Lua dell'oggetto. ScriptItemsOnAcquire
       carica la sandbox Lua dell'oggetto (se ne ha una) e marca la bandiera
       sporca; ScriptItemsProcessDirty la consuma SUBITO, cosi' il pickup e'
       gia' visibile nello stesso frame (invece di aspettare il prossimo
       GameUpdate, vedi game.c). */
    ScriptItemsOnAcquire(game, itemIndex);
    ScriptItemsProcessDirty(game);

    /* Guarigione completa al pickup: e' un bonus UNA TANTUM del momento
       dell'acquisizione (il giocatore "sente" subito il cuore in piu'), non
       una statistica ricalcolabile: se vivesse nel sistema delle cache,
       ogni ricalcolo (es. l'acquisizione di un oggetto successivo)
       guarirebbe di nuovo il giocatore gratis. p->maxHp e' gia' aggiornato
       dalla ScriptItemsProcessDirty qui sopra. */
    if (item.slot == SLOT_BODY) p->hp = p->maxHp;

    char msg[160];
    snprintf(msg, sizeof(msg), "Oggetto: %s (%s).", item.name, ItemFirstTraitName(item.traits));
    GameSetMessage(game, msg);
}

static void CombatPickup(Game *game, Pickup *pickup)
{
    if (pickup->cost > 0)
    {
        if (game->player.coins < pickup->cost)
        {
            GameSetMessage(game, "Monete insufficienti.");
            return;
        }
        game->player.coins -= pickup->cost;
    }

    pickup->active = false;
    if (pickup->kind == PICKUP_HEART)
    {
        game->player.hp = GameMathClampInt(game->player.hp + pickup->value, 0, game->player.maxHp);
        GameSetMessage(game, "Cuore raccolto.");
    }
    else if (pickup->kind == PICKUP_COIN) game->player.coins += pickup->value;
    else if (pickup->kind == PICKUP_BOMB) game->player.bombs += pickup->value;
    else if (pickup->kind == PICKUP_KEY) game->player.keys += pickup->value;
    else if (pickup->kind == PICKUP_ITEM)
    {
        CombatApplyItem(game, pickup->item);
        WorldCurrentRoomMutable(game)->rewardTaken = true;
    }
    else if (pickup->kind == PICKUP_EXIT)
    {
        if (game->floor >= FLOOR_COUNT)
        {
            game->phase = PHASE_WIN;
            GameSetMessage(game, "Run completata. Premi R per ricominciare.");
        }
        else WorldStartFloor(game, game->floor + 1);
    }
}

void CombatUpdatePickups(Game *game)
{
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *p = &game->pickups[i];
        if (!p->active) continue;
        float r = p->radius + game->player.radius;
        if (GameMathLengthSquared(GameMathSubtract(p->pos, game->player.pos)) < r*r) CombatPickup(game, p);
    }
}

void CombatUpdateBombs(Game *game, float dt)
{
    for (int i = 0; i < MAX_BOMBS; i++)
    {
        Bomb *b = &game->bombs[i];
        if (!b->active) continue;
        b->timer -= dt;
        if (b->timer <= 0.0f)
        {
            CombatExplodeAt(game, b->pos, b->radius, 60.0f + 10.0f*(float)game->floor);
            b->active = false;
        }
    }
}

void GameUpdateParticles(Game *game, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        p->pos = GameMathAdd(p->pos, GameMathScale(p->vel, dt));
        p->vel = GameMathScale(p->vel, 0.90f);
        p->life -= dt;
        if (p->life <= 0.0f) p->active = false;
    }
}
