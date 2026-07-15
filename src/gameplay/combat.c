#include "gameplay/combat.h"

#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "gameplay/synergies.h"
#include "script/script_items.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Shot.hitMask e' una maschera a 64 bit, un bit per slot di Game.enemies: se
   MAX_ENEMIES crescesse oltre 64, i nemici dagli slot 64 in su sarebbero
   colpibili all'infinito dallo stesso colpo (uno spostamento di 1ull oltre la
   larghezza del tipo e' comportamento indefinito, in pratica un wrap). Meglio
   non compilare affatto che scoprirlo a runtime. */
#if MAX_ENEMIES > 64
#error "Shot.hitMask e' a 64 bit: MAX_ENEMIES non puo' superare 64 (vedi core/game_types.h)"
#endif

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
        /* Step C (fortuna): la probabilita' di rubare vita non e' piu' un 18%
           fisso, e' 18% + 3 punti per ogni punto di fortuna, clampata in
           [0, 60]. E' lo schema lineare di Isaac ("chance = base + luck*incr",
           vedi docs/references/formule-statistiche.md), il primo consumatore
           della nuova statistica: con fortuna 0 il comportamento e' identico a
           prima di questa fase, con fortuna al massimo (15) si arriva al tetto
           del 60% -- alto, mai garantito. */
        if ((traits & TRAIT_VAMP) && game->player.hp < game->player.maxHp)
        {
            int chance = (int)GameMathClampFloat(18.0f + 3.0f*game->player.luck, 0.0f, 60.0f);
            if (GameRngRange(&game->rng, 0, 100) < chance) game->player.hp++;
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

/* Step C: cuce un tipo di colpo (inventato dal modello, vedi core/shot_type.h)
   su un colpo APPENA creato. I moltiplicatori continui su danno/velocita'/raggio
   sono gia' stati applicati agli ARGOMENTI di EntitiesAddShot dal chiamante (sono
   parametri di costruzione); qui restano i campi che esistono solo dopo la
   nascita del colpo: la forma con cui il renderer lo disegna, i salti di catena
   che gli restano, la perforazione in piu' e la vita moltiplicata.
   'shot' NULL (array dei colpi pieno) e' un no-op: non e' un errore. */
static void CombatApplyShotType(Shot *shot, const ShotTypeDef *type)
{
    if (!shot || !type || !type->active) return;
    shot->form = type->form;
    shot->chain = type->chain;
    shot->pierce += type->pierceBonus;
    shot->life *= type->lifeMul;
}

/* Il nemico attivo piu' vicino a 'pos' ENTRO 'maxDist', escluso 'exceptIndex'
   (il nemico appena colpito: una catena che rimbalzasse su chi ha appena preso
   il colpo non sarebbe una catena, sarebbe danno doppio). Ritorna -1 se non ce
   n'e' nessuno: in quel caso la catena semplicemente non scatta, senza penalita'
   ne' effetti collaterali. */
static int CombatNearestEnemyExcept(Game *game, Vector2 pos, int exceptIndex, float maxDist)
{
    int best = -1;
    float bestD = maxDist*maxDist;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (i == exceptIndex) continue;
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        float d = GameMathLengthSquared(GameMathSubtract(e->pos, pos));
        if (d < bestD)
        {
            bestD = d;
            best = i;
        }
    }
    return best;
}

/* Portata di un salto di catena. Abbastanza larga da collegare due nemici di un
   gruppo, troppo corta per attraversare l'intera stanza (876 px): la catena
   premia chi combatte fra i nemici raggruppati, non chi spara a caso. */
#define COMBAT_CHAIN_RANGE 220.0f
/* Il danno che un salto porta con se'. La perdita e' cio' che rende la catena un
   SIDEGRADE e non un moltiplicatore gratuito (ed e' esattamente il peso con cui
   ShotTypePower la conta, vedi core/shot_type.c). */
#define COMBAT_CHAIN_DAMAGE_FALLOFF 0.65f

/* Il salto di catena vero e proprio (step C, manopola 'chain' di un tipo di
   colpo): dal punto d'impatto parte un colpo NUOVO verso un altro nemico
   vicino, con un salto in meno di quelli che restavano. Un colpo nuovo invece
   di deviare quello vecchio, per due motivi: il colpo vecchio deve poter morire
   (o perforare) secondo le sue regole normali, e MAX_SHOTS resta l'unico tetto
   di sicurezza -- se l'array e' pieno, EntitiesAddShot torna NULL e la catena
   semplicemente si ferma, senza casi speciali. Nasce SPOSTATO fuori dal nemico
   appena colpito (raggio del nemico + raggio del colpo), altrimenti scatterebbe
   subito una seconda collisione con lui: danno doppio invece di una catena. */
static void CombatChainShot(Game *game, const Shot *shot, int hitEnemyIndex)
{
    int targetIndex = CombatNearestEnemyExcept(game, shot->pos, hitEnemyIndex, COMBAT_CHAIN_RANGE);
    if (targetIndex < 0) return;

    Enemy *hitEnemy = &game->enemies[hitEnemyIndex];
    Enemy *target = &game->enemies[targetIndex];
    Vector2 dir = GameMathNormalize(GameMathSubtract(target->pos, hitEnemy->pos));
    if (GameMathLengthSquared(dir) <= 0.0001f) return;

    /* L'origine si misura dal CENTRO DEL NEMICO colpito, non dalla posizione del
       colpo (correzione da review). Al momento dell'impatto il colpo si trova un
       raggio-nemico PRIMA del centro (e' li' che scatta la collisione): partendo
       da li' e spostandosi di "raggio del nemico + raggio del colpo" si finiva
       ancora DENTRO il nemico appena colpito. */
    float hitRadius = hitEnemy->radius;
    Vector2 origin = GameMathAdd(hitEnemy->pos, GameMathScale(dir, hitRadius + shot->radius + 2.0f));
    float speed = sqrtf(GameMathLengthSquared(shot->vel));

    Shot *spawned = EntitiesAddShot(game, true, origin, dir, speed, shot->damage*COMBAT_CHAIN_DAMAGE_FALLOFF,
                                    shot->radius, shot->traits & ~(unsigned int)TRAIT_SPLIT, shot->color);
    if (!spawned) return;
    spawned->form = shot->form;
    spawned->chain = shot->chain - 1;
    spawned->scriptDepth = shot->scriptDepth;
    spawned->synergized = shot->synergized;   /* un salto di catena resta lo stesso colpo, anche a vedersi */
    /* La vera garanzia che la catena non torni indietro non e' la geometria sopra
       (che dipende dai raggi e da dove sono i nemici): e' la MASCHERA dei gia'
       colpiti. Il colpo di catena eredita quella del colpo che l'ha generato, piu'
       il nemico appena colpito. Senza, il salto nasceva a un pelo dal bersaglio
       precedente e lo ricolpiva nel frame successivo, bruciando subito il primo
       salto: una catena da 1 non arrivava MAI a un secondo nemico, e una da 2 ne
       colpiva uno solo. Il test U non se ne accorgeva perche' usa chain=2, e col
       salto sprecato gliene restava comunque uno buono. */
    spawned->hitMask = shot->hitMask | (1ull << hitEnemyIndex);
}

void CombatFirePlayer(Game *game, Vector2 dir)
{
    Player *p = &game->player;
    unsigned int traits = p->traits;
    /* Step C: il tipo di colpo del giocatore (ricalcolato da zero insieme alle
       statistiche, vedi ScriptItemsRecomputeStats) MODULA il colpo base, non lo
       sostituisce: i suoi moltiplicatori si applicano DOPO le statistiche vere e
       DOPO gli aggiustamenti dei trait (GIANT), cosi' il sistema delle cache
       resta l'unica fonte di verita' delle statistiche e un tipo di colpo non
       puo' scavalcarlo. Senza tipo di colpo (type->active falso, il caso di ogni
       run senza oggetti che ne portino uno) ogni fattore vale 1 e il codice qui
       sotto e' identico a quello di prima di questa fase. */
    const ShotTypeDef *type = &p->shotType;
    bool typed = type->active;
    float radius = (p->shotRadius + ((traits & TRAIT_GIANT) ? 5.0f : 0.0f))*(typed ? type->radiusMul : 1.0f);
    float damage = (p->damage + ((traits & TRAIT_GIANT) ? 4.0f : 0.0f))*(typed ? type->damageMul : 1.0f);
    float speed = p->shotSpeed*((traits & TRAIT_GIANT) ? 0.88f : 1.0f)*(typed ? type->speedMul : 1.0f);
    Color color = typed ? p->shotColor : game->theme.accent2;
    /* I pallettoni del tipo di colpo si SOMMANO a quello che TRAIT_SPLIT gia'
       dava (non lo sostituiscono): un tipo a tre pallettoni su un giocatore con
       split spara 4 colpi, non 3. Tetto a 5 perche' oltre il ventaglio diventa
       un muro e MAX_SHOTS si consuma in un attimo. */
    /* Sinergie, CANALE B (step D, docs/references/design-sinergie.md 4.3): il
       punto di innesto naturale e' proprio qui, subito dopo la creazione del
       colpo -- e' l'equivalente delle "tear flags" di Isaac. La maschera e' gia'
       in cache (ricalcolata da zero insieme alle statistiche, vedi
       ScriptItemsRecomputeStats): qui non si rileva nulla, si applica soltanto. */
    int pellets = ((traits & TRAIT_SPLIT) ? 2 : 1) + (typed ? type->pellets - 1 : 0) + SynergiesExtraPellets(p->synergies);
    if (pellets > 5) pellets = 5;
    float angle = atan2f(dir.y, dir.x);
    for (int i = 0; i < pellets; i++)
    {
        float offset = ((float)i - (float)(pellets - 1)*0.5f)*0.18f;
        Shot *spawned = EntitiesAddShot(game, true, p->pos, (Vector2){ cosf(angle + offset), sinf(angle + offset) }, speed, damage, radius, traits, color);
        CombatApplyShotType(spawned, type);
        SynergiesApplyToShot(p, p->synergies, spawned);
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

/* ============================================================
   Fase 3b: i cinque MOVIMENTI e i quattro modi di SPARARE che il motore mette a
   disposizione, e che il modello combina inventando i nemici (core/enemy_type.h).
   Il motore non sa nulla di "quel nemico li'": sa muovere e sparare in questi modi,
   e basta. Un nemico SENZA tipo (zero-default) passa dal ramo storico, identico a
   prima di questa fase.
   ============================================================ */

/* La direzione in cui il nemico si vuole muovere, secondo il suo movimento. */
static Vector2 CombatEnemyMoveDir(Enemy *e, Vector2 dir, float dist, float dt)
{
    switch (e->type.move)
    {
        case ENEMY_MOVE_KITE:
            /* Si tiene a mezza distanza: se sei lontano avanza, se sei addosso
               indietreggia. E' il tiratore di sempre, generalizzato. */
            if (dist < 210.0f) return GameMathScale(dir, -0.65f);
            return dir;

        case ENEMY_MOVE_ORBIT:
        {
            /* Gira attorno al giocatore a distanza fissa: si avvicina/allontana per
               tenere il raggio, e intanto scorre di lato. Non usa 'phase': la
               tangente viene dalla direzione verso il giocatore, non da un
               accumulatore. */
            float radial = (dist - 190.0f)*0.012f;
            radial = GameMathClampFloat(radial, -1.0f, 1.0f);
            Vector2 tangent = GameMathPerpendicular(dir);
            return GameMathNormalize(GameMathAdd(GameMathScale(dir, radial), GameMathScale(tangent, 0.9f)));
        }

        case ENEMY_MOVE_ZIGZAG:
        {
            /* Verso il giocatore, ma serpeggiando: la fase iniziale e' casuale per
               nemico (EntitiesAddEnemyTyped), cosi' un gruppo non ondeggia
               all'unisono come un banco di pesci. */
            e->phase += dt*4.5f;
            Vector2 side = GameMathScale(GameMathPerpendicular(dir), sinf(e->phase)*0.85f);
            return GameMathNormalize(GameMathAdd(dir, side));
        }

        case ENEMY_MOVE_CHARGE:
        {
            /* Si ferma, prende la mira, poi scatta. chargeTimer scandisce le due
               fasi: negativo = sta caricando (fermo), positivo = sta scattando. */
            e->chargeTimer -= dt;
            if (e->chargeTimer <= 0.0f)
            {
                e->chargeTimer = 1.6f;         /* pronto a un nuovo ciclo */
                e->phase = 0.45f;              /* durata dello scatto */
                e->vel = GameMathScale(dir, e->speed*2.2f);   /* lo scatto vero e' un impulso: si smorza da solo */
            }
            if (e->phase > 0.0f) { e->phase -= dt; return (Vector2){ 0.0f, 0.0f }; }   /* durante lo scatto comanda l'impulso */
            return GameMathScale(dir, 0.15f);   /* fra uno scatto e l'altro si trascina appena */
        }

        case ENEMY_MOVE_CHASE:
        default:
            return dir;
    }
}

/* Il fuoco del nemico, secondo il suo modo di sparare. */
static void CombatEnemyFire(Game *game, Enemy *e, Vector2 dir)
{
    float speed = 245.0f + 14.0f*(float)game->floor;
    float radius = e->type.boss ? 7.0f : 6.0f;
    Color color = e->type.boss ? game->theme.boss : game->theme.enemy;

    switch (e->type.fire)
    {
        case ENEMY_FIRE_SINGLE:
            EntitiesAddShot(game, false, e->pos, dir, speed, 1.0f, radius, 0, color);
            break;

        case ENEMY_FIRE_SPREAD:
        {
            /* Un ventaglio VERSO il giocatore: apertura fissa, il numero di colpi lo
               ha scelto il modello (clampato e ribilanciato da EnemyTypeBalance). */
            int pellets = e->type.pellets;
            float base = atan2f(dir.y, dir.x);
            for (int s = 0; s < pellets; s++)
            {
                float offset = ((float)s - (float)(pellets - 1)*0.5f)*0.22f;
                EntitiesAddShot(game, false, e->pos, (Vector2){ cosf(base + offset), sinf(base + offset) }, speed, 1.0f, radius, 0, color);
            }
            break;
        }

        case ENEMY_FIRE_RING:
        {
            /* Una corona in tutte le direzioni, con la fase che ruota di raffica in
               raffica: e' il boss di sempre, generalizzato a qualunque nemico. */
            int pellets = e->type.pellets;
            e->firePhase += 0.22f;   /* campo separato da 'phase': vedi Enemy in game_types.h */
            for (int s = 0; s < pellets; s++)
            {
                float a = (float)s*PI_F*2.0f/(float)pellets + e->firePhase;
                EntitiesAddShot(game, false, e->pos, (Vector2){ cosf(a), sinf(a) }, speed*0.88f, 1.0f, radius, 0, color);
            }
            break;
        }

        case ENEMY_FIRE_NONE:
        default:
            break;   /* fa danno solo al contatto */
    }
}

void CombatUpdateEnemies(Game *game, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        Vector2 toPlayer = GameMathSubtract(game->player.pos, e->pos);
        Vector2 dir = GameMathNormalize(toPlayer);
        float dist = sqrtf(GameMathLengthSquared(toPlayer));
        float slow = e->slowTimer > 0.0f ? 0.45f : 1.0f;
        if (e->slowTimer > 0.0f) e->slowTimer -= dt;

        Vector2 move;
        if (e->type.active)
        {
            move = CombatEnemyMoveDir(e, dir, dist, dt);   /* fase 3b: il movimento inventato dal modello */
        }
        else
        {
            /* Nessun tipo: i nemici storici, invariati (manifest vecchio, o nessun
               manifest). */
            move = dir;
            if (e->kind == ENEMY_SHOOTER)
            {
                if (dist < 210.0f) move = GameMathScale(dir, -0.65f);
            }
            else if (e->kind == ENEMY_BOSS)
            {
                move = GameMathNormalize(GameMathAdd(GameMathScale(dir, 0.5f), GameMathScale(GameMathPerpendicular(dir), sinf((float)GetTime()*1.4f)*0.75f)));
            }
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

        if (e->type.active)
        {
            /* Fase 3b: un solo "istante di azione" per ciclo, scandito dal cooldown
               -- e SOLO qui lo si riarma. Tenere le due cose (sparo e rinforzo del
               boss) dentro lo stesso 'ready' e' necessario, non estetico: se lo
               sparo riarmasse il cooldown per conto suo, il controllo del rinforzo
               subito sotto lo troverebbe sempre > 0 e il boss finale non
               chiamerebbe MAI nessuno. */
            bool ready = e->cooldown <= 0.0f;
            if (ready)
            {
                /* fireRate 0 = non spara mai: EnemyTypeClamp garantisce che in quel
                   caso 'fire' sia NONE, quindi qui basta guardare 'fire'. */
                if (e->type.fire != ENEMY_FIRE_NONE) CombatEnemyFire(game, e, dir);

                /* Il boss dell'ultimo piano continua a chiamare rinforzi: e' una
                   regola di STRUTTURA della run (l'ultimo scontro deve essere
                   l'ultimo scontro), non un comportamento del tipo -- quindi resta
                   in C e non fra le manopole che il modello puo' toccare. Il
                   rinforzo, pero', e' un nemico DEL PIANO: uno di quelli che il
                   modello ha inventato, non l'inseguitore hardcoded di sempre. */
                if (e->kind == ENEMY_BOSS && game->floor == FLOOR_COUNT &&
                    GameRngRange(&game->rng, 0, 100) < 45)
                {
                    const FloorContent *fc = &game->content.floors[game->floor - 1];
                    const EnemyTypeDef *reinforcement = fc->enemies[0].active ? &fc->enemies[0] : NULL;
                    EntitiesAddEnemyTyped(game, ENEMY_CHASER, EntitiesRandomRoomPosition(&game->rng, 60.0f), reinforcement);
                }

                /* 1.2s per un nemico che non spara: e' comunque il suo battito, e
                   serve al boss finale per i rinforzi. */
                e->cooldown = (e->type.fireRate > 0.0f) ? 1.0f/e->type.fireRate : 1.2f;
            }
        }
        else if ((e->kind == ENEMY_SHOOTER || e->kind == ENEMY_TANK) && e->cooldown <= 0.0f)
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
                /* Un colpo non colpisce mai due volte lo stesso nemico (step C,
                   vedi Shot.hitMask in core/game_types.h): e' cio' che fa
                   funzionare davvero la perforazione, invece di far consumare
                   tutti i suoi passaggi sul primo nemico che attraversa. */
                if (s->hitMask & (1ull << e)) continue;
                float r = s->radius + enemy->radius;
                if (GameMathLengthSquared(GameMathSubtract(s->pos, enemy->pos)) >= r*r) continue;
                s->hitMask |= (1ull << e);
                /* ScriptItemsOnHit PRIMA di CombatDamageEnemy (a differenza
                   della mini-VM sotto, che e' sempre girata dopo): uno script
                   Lua deve poter leggere lo stato del nemico com'era AL
                   MOMENTO dell'impatto (es. enemy_hp(id) prima di questo
                   colpo), non un handle che CombatDamageEnemy potrebbe aver
                   gia' disattivato (nemico ucciso da questo stesso colpo). */
                ScriptItemsOnHit(game, i, e);
                CombatDamageEnemy(game, enemy, s->damage, s->traits);
                ScriptVmExecutePlayer(game, SCRIPT_ON_HIT, s->pos, GameMathNormalize(s->vel), s->damage, s->traits, s->scriptDepth);
                /* Catena (step C): PRIMA di far esplodere/sdoppiare/morire questo
                   colpo -- il salto parte dall'impatto, quindi deve leggere il
                   colpo com'e' adesso (posizione, danno, forma), non dopo che
                   pierce gli ha gia' ridotto il danno qui sotto. */
                if (s->chain > 0) CombatChainShot(game, s, e);
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
    /* Sinergie: si guarda la maschera PRIMA e DOPO il ricalcolo del pickup, cosi'
       si annuncia solo cio' che questo oggetto ha davvero SBLOCCATO (mai le
       coppie che c'erano gia'). Il confronto vive qui e non dentro il ricalcolo
       apposta: ScriptItemsRecomputeStats deve restare PURA e idempotente (gira
       piu' volte per frame, e ogni volta annuncerebbe di nuovo la stessa
       sinergia). */
    unsigned int synergiesBefore = p->synergies;
    ScriptItemsOnAcquire(game, itemIndex);
    ScriptItemsProcessDirty(game);
    unsigned int unlocked = p->synergies & ~synergiesBefore;

    /* Guarigione completa al pickup: e' un bonus UNA TANTUM del momento
       dell'acquisizione (il giocatore "sente" subito il cuore in piu'), non
       una statistica ricalcolabile: se vivesse nel sistema delle cache,
       ogni ricalcolo (es. l'acquisizione di un oggetto successivo)
       guarirebbe di nuovo il giocatore gratis. p->maxHp e' gia' aggiornato
       dalla ScriptItemsProcessDirty qui sopra. */
    if (item.slot == SLOT_BODY) p->hp = p->maxHp;

    char msg[160];
    /* Una sinergia appena sbloccata e' l'evento piu' importante che possa
       capitare a una build: si prende il messaggio (e una fiammata di particelle),
       davanti perfino al cambio di tipo di colpo. "Le sinergie non si notano" era
       il feedback: qui si notano. */
    if (unlocked != 0u)
    {
        int first = 0;
        while (first < 31 && !(unlocked & (1u << first))) first++;
        snprintf(msg, sizeof(msg), "SINERGIA: %s -- %s!", SynergyName(first), SynergyDescription(first));
        EntitiesAddParticle(game, p->pos, item.color, 34);
    }
    /* Step C: se l'oggetto cambia il MODO di sparare, il messaggio lo dice --
       e' l'evento piu' vistoso che possa capitare a una run, e finora il
       giocatore lo avrebbe scoperto solo guardando i proiettili. */
    else if (item.shotType.active)
    {
        snprintf(msg, sizeof(msg), "Oggetto: %s (%s). Ora spari: %s.", item.name, ItemFirstTraitName(item.traits), item.shotType.name);
    }
    else
    {
        snprintf(msg, sizeof(msg), "Oggetto: %s (%s).", item.name, ItemFirstTraitName(item.traits));
    }
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
