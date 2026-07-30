#include "gameplay/combat.h"

#include "audio/audio.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_slots.h"
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

/* Fase 3c: spinge un cerchio (giocatore o nemico) fuori da TUTTI gli ostacoli
   solidi della stanza corrente. Un solo giro basta nella pratica (gli ostacoli non
   si toccano fra loro, la croce centrale li tiene separati), ma se un cerchio
   grosso stesse a cavallo di due blocchi adiacenti un secondo giro lo sistema:
   due giri sono un tetto abbondante e a costo nullo (gli ostacoli veri sono ~4
   per cella, DEC-170 ne ammette fino a 10 per cella piu' l'eventuale
   cella-buco di una forma a L). Ritorna true se ha toccato qualcosa (serve ai
   colpi, sotto, per sapere se rimbalzare). */
static bool CombatResolveObstacles(Game *game, Vector2 *pos, float radius)
{
    bool touched = false;
    for (int pass = 0; pass < 2; pass++)
    {
        bool any = false;
        for (int i = 0; i < game->obstacleCount; i++)
        {
            Obstacle *o = &game->obstacles[i];
            Rectangle r = { o->x, o->y, o->w, o->h };
            if (GameMathResolveCircleRect(pos, radius, r)) { any = true; touched = true; }
        }
        if (!any) break;
    }
    /* Ri-clamp ai bordi DOPO la risoluzione (correzione da review): il chiamante ha
       gia' clampato prima, ma la spinta fuori da un ostacolo attaccato al muro puo'
       riportare il centro OLTRE il bordo -- e nessuno ri-clampava, quindi l'entita'
       finiva col bordo dentro il muro (2px per il giocatore, fino a ~20px per un
       corazzato grosso in una strozzatura d'angolo). Meglio che l'entita' resti
       incollata al muro con l'hitbox che sfiora appena il blocco (invisibile) che
       bucare il muro (visibile). Fatto qui, dentro la funzione, cosi' vale per
       giocatore e nemici -- entrambi la chiamano subito dopo il proprio clamp.
       DEC-170: il bordo e' il riquadro della stanza CORRENTE (una o piu'
       celle) -- gli ostacoli vivono comunque solo li'. */
    if (touched) WorldClampToRoom(game, pos, radius);
    return touched;
}

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

void CombatDamagePlayer(Game *game, int amount, const char *cause)
{
    if (game->player.invuln > 0.0f || game->phase != PHASE_PLAY) return;
    AudioPlaySfx(AUDIO_SFX_HIT_PLAYER);
    game->player.hp -= amount;
    game->player.invuln = 0.85f;
    EntitiesAddParticle(game, game->player.pos, RED, 22);
    if (game->player.hp <= 0)
    {
        game->phase = PHASE_GAME_OVER;
        GameSetMessage(game, "Run finita. Premi R.");
        /* DEC-159: la causa della sconfitta, mostrata da DrawRunResultsOverlay
           SOLO quando game->phase == PHASE_GAME_OVER (mai a vittoria/abbandono,
           dove deathCause resta la stringa vuota dello zero-default). */
        snprintf(game->deathCause, sizeof(game->deathCause), "%s", cause ? cause : "un colpo");
        /* DEC-152: le card di scoperta ancora IN CODA a morte si scartano
           silenziosamente -- mai enemyEncountered/bossEncountered, gia' scritti
           al momento della scoperta (vedi il commento su GameDiscardPendingDiscoveries). */
        GameDiscardPendingDiscoveries(game);
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

    AudioPlaySfx(AUDIO_SFX_HIT_ENEMY);
    enemy->hp -= damage;
    /* W8: apre la finestra dell'animazione 'hit' (vedi Enemy.hitFlash). 0.12 s
       e' un fotogramma di quella riga a 10 fps piu' un margine: abbastanza per
       vedersi, abbastanza poco per non nascondere la camminata di un nemico
       colpito a raffica. */
    enemy->hitFlash = 0.12f;
    if (traits & TRAIT_SLOW) enemy->slowTimer = 1.6f;
    EntitiesAddParticle(game, enemy->pos, game->theme.accent2, 4);
    if (enemy->hp <= 0.0f)
    {
        enemy->active = false;
        /* W8: la riga 'death' dello spritesheet continua a scorrere dopo che il
           nemico e' uscito di scena (ArtFx, core/game_types.h). La chiave si
           compone qui e non in src/render perche' e' qui che si sa QUALE nemico
           e' morto -- un istante dopo, l'Enemy e' uno slot riciclabile. Non e'
           un percorso di file: e' l'image-id, e la risoluzione a spritesheet
           resta di src/assets (DEC-175(b)).
           La durata e' un tetto generoso per la riga 'death' del contratto
           d'arte (4 fotogrammi a 8-10 fps per un nemico, 6 a 7 fps per un
           boss); superarla e' innocuo, vedi EntitiesAddArtFx. La scala e' la
           stessa che il renderer usa per il nemico vivo, espressa qui come
           larghezza voluta in pixel (raggio*3.3) perche' src/gameplay non sa
           quanti pixel abbia il fotogramma: la converte ArtScaleForWidth. */
        EntitiesAddArtFx(game, enemy->type.imageId, "death", enemy->pos,
                         enemy->radius*3.3f, (enemy->kind == ENEMY_BOSS) ? 0.95f : 0.55f,
                         enemy->vel.x < -1.0f, WHITE);
        game->score += (enemy->kind == ENEMY_BOSS) ? 300 : 30;
        EntitiesAddParticle(game, enemy->pos, enemy->kind == ENEMY_BOSS ? game->theme.boss : game->theme.enemy, 22);
        /* Step C (fortuna): la probabilita' di rubare vita non e' piu' un 18%
           fisso, e' 18% + 3 punti per ogni punto di fortuna, clampata in
           [0, 60]. E' lo schema lineare di Isaac ("chance = base + luck*incr",
           vedi docs/references/research/formule-statistiche.md), il primo consumatore
           della nuova statistica: con fortuna 0 il comportamento e' identico a
           prima di questa fase, con fortuna al massimo (15) si arriva al tetto
           del 60% -- alto, mai garantito. */
        if ((traits & TRAIT_VAMP) && game->player.hp < game->player.maxHp)
        {
            int chance = (int)GameMathClampFloat(18.0f + 3.0f*game->player.luck, 0.0f, 60.0f);
            if (GameRngRange(&game->rng, 0, 100) < chance) game->player.hp++;
        }
        /* DEC-059, secondo canale di ricarica: l'energia droppata dai nemici.
           Cade SOLO se serve davvero a qualcuno (un attivo a cariche
           posseduto e non pieno): senza quella condizione sarebbe rumore a
           schermo per la maggior parte delle run, in cui nessun attivo e'
           ancora stato raccolto. Resta l'unico drop per-NEMICO del gioco --
           tutte le altre ricompense sono per-STANZA (WorldSpawnRoomReward) --
           e l'eccezione e' voluta: il canale che DEC-059 descrive e' proprio
           "energia droppata dai nemici", non "energia a fine stanza" (quello
           e' gia' il primo canale). Probabilita' fissa, materia di playtest
           come il resto del dosaggio. */
        if (ItemActivesWantEnergy(&game->player) && GameRngRange(&game->rng, 0, 100) < 30)
        {
            EntitiesAddPickup(game, PICKUP_ENERGY, enemy->pos, 1, 0);
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
    /* UNA volta per sparo, non per pallettone: la cadenza vera e' gia' quella
       di p->fireDelay (fireTimer sotto), il pool di alias di AudioPlaySfx
       (audio.c) copre le sovrapposizioni fra spari ravvicinati -- ma un
       ventaglio di piu' pallettoni nello STESSO sparo (TRAIT_SPLIT, un tipo
       di colpo generato, Sciame) deve restare UN SOLO evento sonoro, non
       N sovrapposti sullo stesso frame. */
    AudioPlaySfx(AUDIO_SFX_SHOT);
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
    /* Sinergie, CANALE B (step D, docs/references/research/design-sinergie.md 4.3): il
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
        SynergiesApplyToShot(p, p->synergies, game->runSeed, spawned);
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

/* ============================================================
   Oggetti ATTIVI e INNESTI: uso, sgancio, scambio sul piedistallo
   (docs/design/systems/active-items.md, systems/grafts.md).
   ============================================================ */

/* Ripiego in C dell'attivazione, per un attivo che non ha un 'on_use' Lua
   utilizzabile. E' la stessa promessa "mai un dud" degli stat-up
   (ScriptItemsApplyStatUpFallback, src/script/script_items.c) applicata agli
   attivi: un oggetto che il giocatore ha scelto di tenere in un solo slot e
   che ha aspettato di ricaricare non puo' non fare NIENTE quando lo si usa.
   Niente RNG: stesso trait -> sempre lo stesso effetto, prevedibile e
   testabile come il resto. UN SOLO trait guida un solo effetto (stessa forma
   a catena di ScriptItemsApplyStatUpFallback), ma l'ordine NON e' quello di
   ItemFirstTraitName: qui vengono prima i tre trait che descrivono un'AZIONE
   istantanea sensata da attivare a comando (vampirismo -> cura, esplosione/
   gigante -> deflagrazione attorno, rallentamento -> tutta la stanza
   rallenta), mentre i trait "di colpo" (rimbalzo, guida, divisione,
   perforazione, cadenza) non descrivono un'azione ma il modo in cui un
   proiettile vola, e finiscono percio' tutti nel caso finale, dove sono i
   proiettili della corona a portarseli addosso. I numeri sono un default
   proposto dall'implementazione, materia di playtest come il resto del
   bilanciamento. */
static void CombatActiveFallbackEffect(Game *game, const Item *item, Vector2 dir)
{
    Player *p = &game->player;
    float power = 1.0f + 0.35f*(float)item->rarity;   /* comune 1.0 ... leggendario ~2.05 */

    if (item->traits & TRAIT_VAMP)
    {
        int before = p->hp;
        p->hp = GameMathClampInt(p->hp + 1 + (int)item->rarity/2, 0, p->maxHp);
        EntitiesAddParticle(game, p->pos, item->color, 24);
        if (p->hp == before) GameSetMessage(game, "Attivo usato: gia' al massimo della salute.");
        return;
    }
    if (item->traits & (unsigned int)(TRAIT_EXPLODE | TRAIT_GIANT))
    {
        CombatExplodeAt(game, p->pos, 120.0f, (18.0f + 6.0f*(float)game->floor)*power);
        return;
    }
    if (item->traits & TRAIT_SLOW)
    {
        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (!game->enemies[i].active) continue;
            game->enemies[i].slowTimer = 2.5f*power;
        }
        EntitiesAddParticle(game, p->pos, item->color, 20);
        return;
    }
    /* Nessun trait riconosciuto (o uno "di colpo": rimbalzo, guida,
       divisione, perforazione, cadenza): una corona di colpi attorno al
       giocatore. E' l'effetto piu' neutro possibile -- usa i mattoni gia'
       esistenti, non ne inventa uno -- e non e' mai nullo. Il primo colpo
       parte nella direzione di mira, cosi' anche una corona si sente
       "puntata" invece che sparata a caso. */
    const int pellets = 8;
    float base = atan2f(dir.y, dir.x);
    for (int i = 0; i < pellets; i++)
    {
        float a = base + (float)i*(2.0f*3.14159265f/(float)pellets);
        EntitiesAddShot(game, true, p->pos, (Vector2){ cosf(a), sinf(a) },
                        380.0f, p->damage*0.6f*power, p->shotRadius, item->traits, item->color);
    }
}

void CombatUseActive(Game *game, Vector2 dir)
{
    Player *p = &game->player;
    int index = ItemSelectedActiveIndex(p);
    if (index < 0)
    {
        GameSetMessage(game, "Nessun oggetto attivo equipaggiato.");
        return;
    }

    Item *item = &p->items[index];
    char msg[160];
    if (!ItemActiveIsReady(item))
    {
        /* active-items.md, caso limite: attivazione in ricarica -> nessun
           effetto, nessuna carica persa, feedback che non lascia dubbi su
           QUANTO manca. */
        if (ItemActiveIsChargeBased(item))
            snprintf(msg, sizeof(msg), "%s: in ricarica (%d/%d cariche).", item->name, item->chargeNow, ItemActiveChargeCapacity(item));
        else
            snprintf(msg, sizeof(msg), "%s: in ricarica (%.1fs).", item->name, (double)item->cooldownTimer);
        GameSetMessage(game, msg);
        return;
    }

    /* La direzione di mira puo' essere nulla (il giocatore non sta mirando):
       un attivo deve funzionare comunque, quindi si ripiega su "verso
       destra" invece di rifiutare l'uso. */
    if (GameMathLengthSquared(dir) <= 0.01f) dir = (Vector2){ 1.0f, 0.0f };

    /* Lo stato di ricarica si consuma PRIMA dell'effetto: l'effetto puo'
       spawnare colpi, uccidere nemici e far ripulire la stanza nello stesso
       frame -- cioe' far scattare il canale di ricarica "stanza completata"
       (DEC-059) -- e consumare dopo cancellerebbe quella ricarica. */
    if (ItemActiveIsChargeBased(item)) item->chargeNow--;
    else item->cooldownTimer = ItemActiveCooldownSeconds(item);

    Item used = *item;   /* copia: il ripiego in C legge l'oggetto mentre l'effetto puo' toccare items[] */
    /* Il messaggio generico si scrive PRIMA dell'effetto (conferma
       immediata dell'attivazione, active-items.md "Feedback"): cosi' un
       effetto che ha qualcosa di piu' preciso da dire -- "gia' al massimo
       della salute" -- lo sovrascrive, invece di essere sovrascritto. */
    snprintf(msg, sizeof(msg), "Attivo: %s.", used.name);
    GameSetMessage(game, msg);
    if (!ScriptItemsOnUse(game, index, p->pos, dir)) CombatActiveFallbackEffect(game, &used, dir);
}

void CombatDropGraft(Game *game)
{
    Player *p = &game->player;
    /* Con piu' slot Innesto si sgancia l'ULTIMO equipaggiato: e' l'unico che
       il giocatore ha scelto di recente, e senza una UI di selezione degli
       slot (fuori da questo passo) e' la scelta meno sorprendente. */
    int owned = ItemCountOfKind(p, ITEM_GRAFT);
    if (owned <= 0)
    {
        GameSetMessage(game, "Nessun Innesto equipaggiato.");
        return;
    }
    int index = ItemIndexOfKind(p, ITEM_GRAFT, owned - 1);
    if (index < 0) return;

    /* DEC-115 + DEC-183: lo slot si libera e l'Innesto resta A TERRA, nella
       stanza in cui e' stato sganciato, recuperabile in QUALSIASI momento
       della run -- non solo restando nella visita corrente (DEC-183 supera
       la clausola "uscendo si perde" di DEC-160). La persistenza vive in un
       record di Game.droppedGrafts (vedi il commento li': una lista, non un
       campo singolo per stanza, perche' in una stessa stanza possono
       coesistere piu' Innesti a terra insieme -- il difetto bloccante che
       questa versione chiude). WorldSpawnRoomContents ri-materializza OGNI
       record della stanza corrente come pickup a ogni ingresso, finche' non
       viene ripreso davvero (CombatPickup, guardato da
       Pickup.isPersistedGraft/droppedGraftSlot). */
    Item dropped = p->items[index];
    ScriptItemsRemoveItem(game, index);
    ScriptItemsProcessDirty(game);   /* lo slot si libera SUBITO, come la raccolta si vede subito */
    int slot = -1;
    for (int i = 0; i < MAX_DROPPED_GRAFTS; i++)
    {
        if (!game->droppedGrafts[i].active) { slot = i; break; }
    }
    if (slot >= 0)
    {
        DroppedGraftRecord *rec = &game->droppedGrafts[slot];
        rec->active = true;
        rec->roomX = game->roomX;
        rec->roomY = game->roomY;
        rec->item = dropped;
        rec->pos = p->pos;
    }
    else
    {
        /* Non raggiungibile con il contenuto attuale (vedi il commento su
           MAX_DROPPED_GRAFTS): se capitasse comunque, l'Innesto resta a
           terra solo per la visita corrente (comportamento pre-DEC-183)
           invece di corrompere un altro record o rifiutare lo sgancio. */
        fprintf(stderr, "CombatDropGraft: Game.droppedGrafts esaurito (%d), '%s' non sara' persistente\n",
                MAX_DROPPED_GRAFTS, dropped.name);
    }
    Pickup *ground = EntitiesAddItemPickup(game, p->pos, dropped, 0);
    if (ground && slot >= 0)
    {
        ground->isPersistedGraft = true;
        ground->droppedGraftSlot = slot;
    }
    /* Nato sotto i piedi del giocatore: senza il blocco verrebbe riraccolto
       il frame successivo e sganciare sarebbe impossibile. Si blocca ogni
       pickup di oggetto che il giocatore sta gia' toccando, non solo quello
       appena creato: se si sgancia stando addosso a un piedistallo, anche
       quello non deve scattare per la sovrapposizione gia' in corso. */
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        Pickup *pk = &game->pickups[i];
        if (!pk->active || pk->kind != PICKUP_ITEM) continue;
        float r = pk->radius + p->radius;
        if (GameMathLengthSquared(GameMathSubtract(pk->pos, p->pos)) < r*r) pk->locked = true;
    }
    char msg[160];
    snprintf(msg, sizeof(msg), "Innesto sganciato: %s. Resta qui per tutta la run.", dropped.name);
    GameSetMessage(game, msg);
}

/* Quale oggetto gia' posseduto deve lasciare il posto a un oggetto della
   categoria 'kind' appena raccolto, oppure -1 se c'e' uno slot libero (o se
   la categoria non ha slot: passivi e stat-up si accumulano e basta).
   DEC-117 per gli attivi: quello che finisce sul piedistallo e' l'attivo
   ATTUALMENTE SELEZIONATO, non un attivo qualsiasi. */
static int CombatSlotToSwapFor(const Player *p, ItemKind kind)
{
    if (kind == ITEM_ACTIVE)
    {
        if (ItemCountOfKind(p, ITEM_ACTIVE) < ItemActiveSlotCount(p)) return -1;
        return ItemSelectedActiveIndex(p);
    }
    if (kind == ITEM_GRAFT)
    {
        int owned = ItemCountOfKind(p, ITEM_GRAFT);
        if (owned < ItemGraftSlotCount(p)) return -1;
        return ItemIndexOfKind(p, ITEM_GRAFT, owned - 1);
    }
    return -1;
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
    /* W8: lo stato di ANIMAZIONE del personaggio (Player.animFacing/walkTime,
       core/game_types.h) si scrive QUI e in nessun altro punto -- e' l'unico
       posto che sa se il giocatore si sta muovendo e verso dove, perche' il
       movimento e' un delta applicato e dimenticato (non esiste una
       Player.vel). L'asse VERTICALE vince sull'orizzontale in diagonale: le
       quattro camminate dello spritesheet sono ortogonali, una diagonale deve
       scegliere, e la scelta verticale rende leggibile "sto salendo/scendendo",
       che e' l'informazione che conta in una stanza vista dall'alto.
       'walkTime' si azzera appena si sta fermi, cosi' la camminata ricomincia
       sempre dal primo fotogramma (un passo che riprende a meta' si vede) e
       'walkTime == 0' significa esattamente "fermo" per il renderer. */
    if (move.y < -0.001f) p->animFacing = DIR_UP;
    else if (move.y > 0.001f) p->animFacing = DIR_DOWN;
    else if (move.x < -0.001f) p->animFacing = DIR_LEFT;
    else if (move.x > 0.001f) p->animFacing = DIR_RIGHT;
    if (GameMathLengthSquared(move) > 0.0001f) p->walkTime += dt;
    else p->walkTime = 0.0f;
    p->pos = GameMathAdd(p->pos, GameMathScale(move, p->speed*dt));
    /* DEC-170: il bordo e' quello della stanza corrente, che ora puo' valere
       piu' celle -- dentro ci si cammina senza transizioni (l'angolo mancante
       di una forma a L e' un ostacolo, lo risolve la riga sotto). */
    WorldClampToRoom(game, &p->pos, p->radius);
    CombatResolveObstacles(game, &p->pos, p->radius);   /* fase 3c: non si passa attraverso i muri */
    WorldHandleTransitions(game, move);

    if (p->invuln > 0.0f) p->invuln -= dt;
    if (p->fireTimer > 0.0f) p->fireTimer -= dt;
    ItemActivesTickCooldown(p, dt);   /* attivi a cooldown: il loro canale di ricarica e' il tempo */
    ScriptItemsOnTick(game, dt);

    Vector2 aim = { 0.0f, 0.0f };
    /* DEC-170: 'mouseGame' e' un punto del CANVAS (960x640); con la telecamera
       il mondo puo' essere traslato sotto di esso, quindi la mira va convertita
       prima di diventare una direzione. Per una stanza 1x1 la conversione e'
       l'identita': mira invariata. */
    if (mouseInsideGame && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) aim = GameMathSubtract(WorldCanvasToWorld(game, mouseGame), p->pos);
    if (GameMathLengthSquared(aim) <= 0.01f)
    {
        if (IsKeyDown(KEY_UP)) aim.y -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) aim.y += 1.0f;
        if (IsKeyDown(KEY_LEFT)) aim.x -= 1.0f;
        if (IsKeyDown(KEY_RIGHT)) aim.x += 1.0f;
    }
    aim = GameMathNormalize(aim);
    if (p->fireTimer <= 0.0f && GameMathLengthSquared(aim) > 0.01f) CombatFirePlayer(game, aim);

    if (game->bombQueued)
    {
        game->bombQueued = false;   /* consumato: un evento = una bomba, anche su frame a 2 passi */
        CombatPlaceBomb(game);
    }
    if (game->useActiveQueued)
    {
        game->useActiveQueued = false;   /* stessa disciplina della bomba: un evento = un uso */
        CombatUseActive(game, aim);
    }
    if (game->dropGraftQueued)
    {
        game->dropGraftQueued = false;
        CombatDropGraft(game);
    }
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
        /* W8: la finestra dell'animazione 'hit' si chiude qui, nello stesso
           ciclo che consuma slowTimer -- un solo posto in cui i timer di un
           nemico avanzano. */
        if (e->hitFlash > 0.0f) e->hitFlash -= dt;

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
        /* Bordo della stanza corrente (DEC-170: riquadro multi-cella), non il
           massimo fisso -- lo si ricalcola per ogni nemico (banale: la stanza
           non cambia durante questo ciclo, ma il compilatore non ha comunque
           motivo di lamentarsi di una chiamata cosi' leggera dentro il ciclo). */
        WorldClampToRoom(game, &e->pos, e->radius);
        CombatResolveObstacles(game, &e->pos, e->radius);   /* fase 3c: i nemici non passano attraverso i muri */
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
                    EntitiesAddEnemyTyped(game, ENEMY_CHASER, EntitiesRandomRoomPosition(&game->rng, WorldCurrentRoomRect(game), 60.0f), reinforcement);
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
                EntitiesAddEnemy(game, ENEMY_CHASER, EntitiesRandomRoomPosition(&game->rng, WorldCurrentRoomRect(game), 60.0f));
            }
            e->cooldown = (game->floor == FLOOR_COUNT) ? 0.85f : 1.18f;
        }

        float touch = e->radius + game->player.radius;
        if (GameMathLengthSquared(GameMathSubtract(e->pos, game->player.pos)) < touch*touch)
        {
            /* DEC-159: nome dichiarato dal modello se questo nemico ha un tipo
               generato (fase 3b), altrimenti il nome storico del suo 'kind' --
               mai un identificatore tecnico a schermo (registro del crogiolo,
               DEC-105). */
            const char *enemyName = (e->type.active && e->type.name[0]) ? e->type.name
                : (e->kind == ENEMY_BOSS ? "il boss"
                   : (e->kind == ENEMY_SHOOTER ? "un tiratore"
                      : (e->kind == ENEMY_TANK ? "un corazzato" : "un inseguitore")));
            char cause[64];
            snprintf(cause, sizeof(cause), "contatto con %s", enemyName);
            CombatDamagePlayer(game, 1, cause);
        }
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

        Vector2 prevPos = s->pos;
        s->pos = GameMathAdd(s->pos, GameMathScale(s->vel, dt));
        s->life -= dt;
        bool wall = false;
        /* I colpi vivono e rimbalzano nel riquadro della stanza CORRENTE
           (DEC-170: puo' valere piu' celle); l'angolo mancante di una forma a
           L li ferma come un ostacolo, nel ciclo qui sotto. */
        Rectangle shotRoom = WorldCurrentRoomRect(game);
        float shotRoomRight = shotRoom.x + shotRoom.width;
        float shotRoomBottom = shotRoom.y + shotRoom.height;
        if (s->pos.x < shotRoom.x + s->radius || s->pos.x > shotRoomRight - s->radius)
        {
            s->vel.x *= -1.0f;
            s->pos.x = GameMathClampFloat(s->pos.x, shotRoom.x + s->radius, shotRoomRight - s->radius);
            wall = true;
        }
        if (s->pos.y < shotRoom.y + s->radius || s->pos.y > shotRoomBottom - s->radius)
        {
            s->vel.y *= -1.0f;
            s->pos.y = GameMathClampFloat(s->pos.y, shotRoom.y + s->radius, shotRoomBottom - s->radius);
            wall = true;
        }
        /* Fase 3c: gli ostacoli fermano i colpi come i muri. Si usa il SEGMENTO del
           movimento (prevPos -> pos), non il solo punto d'arrivo: un colpo veloce
           puo' scavalcare un ostacolo sottile in un frame. Un colpo con rimbalzi
           spende un rimbalzo e riparte dal punto prima dell'ostacolo, invertendo la
           componente di velocita' del lato colpito; altrimenti muore. Vale per i
           colpi del giocatore E dei nemici: un muro e' un muro per entrambi. */
        if (!wall && game->obstacleCount > 0)
        {
            for (int oi = 0; oi < game->obstacleCount; oi++)
            {
                Obstacle *o = &game->obstacles[oi];
                Rectangle r = { o->x - s->radius, o->y - s->radius, o->w + s->radius*2.0f, o->h + s->radius*2.0f };
                if (!GameMathSegmentHitsRect(prevPos, s->pos, r)) continue;
                /* Da che lato e' entrato: si guarda dove stava PRIMA rispetto al
                   rettangolo gonfiato, per decidere quale componente invertire. */
                bool fromSide = prevPos.x <= r.x || prevPos.x >= r.x + r.width;
                bool fromTopBottom = prevPos.y <= r.y || prevPos.y >= r.y + r.height;
                s->pos = prevPos;   /* torna al punto sicuro prima dell'ostacolo */
                if (fromSide && !fromTopBottom) s->vel.x *= -1.0f;
                else if (fromTopBottom && !fromSide) s->vel.y *= -1.0f;
                else { s->vel.x *= -1.0f; s->vel.y *= -1.0f; }   /* angolo: inverti entrambe */
                wall = true;
                break;
            }
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
                /* DEC-159: 'Shot' non porta con se' l'identita' di chi lo ha
                   sparato (nessun campo owner/nome, vedi core/game_types.h) --
                   un colpo puo' ancora essere in volo quando il nemico che lo
                   ha sparato e' gia' morto o fuori stanza. Causa generica per
                   ora: gap noto, non una nuova decisione di design (colmarlo
                   vorrebbe dire aggiungere un identificatore a Shot, che tocca
                   entities.c/script_api.c/script_vm.c -- fuori perimetro di
                   questo lavoro). */
                CombatDamagePlayer(game, 1, "un colpo nemico");
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
    /* Lo stato di ricarica NON si tocca qui: l'oggetto arriva gia' carico se
       il gioco lo ha appena offerto (EntitiesAddItemPickup), e arriva con le
       cariche che aveva se torna da un piedistallo dopo uno scambio
       (DEC-117). Azzerarlo qui sarebbe una ricarica gratis a ogni
       scambio-e-riscambio. */
    /* p->traits NON si aggiorna piu' qui con un OR: lo ricalcola da zero
       ScriptItemsRecomputeStats (chiamata sotto da ScriptItemsProcessDirty),
       come ogni altra statistica. Un OR qui tornerebbe a essere monotono, e
       sganciare un Innesto o scambiare un attivo lascerebbe i suoi trait sui
       colpi per il resto della run. */

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
    /* "Oggetto/valuta/Flux raccolti" (audio-and-feedback.md): un solo evento
       sonoro condiviso da ogni pickup con un effetto reale, tranne
       PICKUP_EXIT -- quello apre il piano successivo (o vince la run), non
       e' "una raccolta" nel senso dell'evento sonoro. */
    if (pickup->kind != PICKUP_EXIT) AudioPlaySfx(AUDIO_SFX_PICKUP);
    if (pickup->kind == PICKUP_HEART)
    {
        game->player.hp = GameMathClampInt(game->player.hp + pickup->value, 0, game->player.maxHp);
        GameSetMessage(game, "Cuore raccolto.");
    }
    else if (pickup->kind == PICKUP_COIN) game->player.coins += pickup->value;
    else if (pickup->kind == PICKUP_BOMB) game->player.bombs += pickup->value;
    else if (pickup->kind == PICKUP_KEY) game->player.keys += pickup->value;
    else if (pickup->kind == PICKUP_ENERGY)
    {
        /* DEC-059, secondo canale: l'energia ricarica gli attivi a cariche
           secondo il dosaggio dichiarato da ciascun oggetto. Non e' una
           risorsa che si accumula in tasca (nessun campo in Player): si
           raccoglie e si converte subito, altrimenti sarebbe una sesta
           valuta senza un documento che la definisca. */
        int touched = ItemActivesGainEnergyCharge(&game->player);
        GameSetMessage(game, touched > 0 ? "Energia: attivo ricaricato." : "Energia raccolta.");
    }
    else if (pickup->kind == PICKUP_FLUX)
    {
        /* Catalizzatore di fusione (DEC-022). A differenza dell'energia qui
           sopra si ACCUMULA in tasca (Player.flux) e senza alcun cap
           (DEC-129): il limite e' la rarita' delle fonti, non un tetto.
           Il messaggio dice cosa farne, perche' la fusione non e' un'azione
           ovvia come "hai una bomba in piu'". */
        game->player.flux += pickup->value > 0 ? pickup->value : 1;
        GameSetMessage(game, "Flux raccolto: apri la build (TAB) per fondere due oggetti.");
    }
    else if (pickup->kind == PICKUP_ITEM)
    {
        Item taken = pickup->item;
        int swapIndex = CombatSlotToSwapFor(&game->player, taken.kind);
        /* DEC-167: il tesoro conta come "ripulito" quando si APRE, cioe'
           quando il suo oggetto viene preso per la prima volta -- catturato
           PRIMA di scrivere 'rewardTaken' sotto, cosi' uno scambio successivo
           sullo stesso piedistallo (o rientrare nella stanza) non paga una
           seconda volta. Il negozio passa da qui per lo STESSO ramo di
           codice (l'acquisto di un oggetto e' anche lui un PICKUP_ITEM), ma
           la sua valuta di completamento e' gia' assegnata alla visita
           (WorldSpawnRoomContents): il controllo sul kind sotto evita di
           contarla due volte. */
        RoomState *room = WorldCurrentRoomMutable(game);
        bool firstReward = !room->rewardTaken;
        /* DEC-183: catturati PRIMA che 'pickup' possa essere riscritto sotto
           (lo scambio riusa la STESSA Pickup per il vecchio oggetto) -- veri
           solo per il pickup che rappresenta un record di
           Game.droppedGrafts (vedi il commento su Pickup.isPersistedGraft,
           core/game_types.h), mai per un Innesto qualunque offerto da
           tesoro/negozio. 'persistedSlot' e' -1 quando wasPersistedGraft e'
           falso (mai letto in quel caso, ma niente indice spazzatura in
           giro). */
        bool wasPersistedGraft = pickup->isPersistedGraft;
        int persistedSlot = wasPersistedGraft ? pickup->droppedGraftSlot : -1;
        bool persistedSlotValid = persistedSlot >= 0 && persistedSlot < MAX_DROPPED_GRAFTS;
        if (swapIndex >= 0)
        {
            /* DEC-117 (attivi) e grafts.md (Innesti): a slot pieni la
               raccolta e' uno SCAMBIO col piedistallo -- l'oggetto che
               possedevi non sparisce, resta li' dove hai preso il nuovo, e
               lo scambio e' reversibile finche' non lasci la stanza (i
               pickup muoiono con EntitiesClear al cambio stanza). */
            Item previous = game->player.items[swapIndex];
            ScriptItemsRemoveItem(game, swapIndex);
            CombatApplyItem(game, taken);
            pickup->active = true;    /* il piedistallo non resta vuoto: ci finisce il vecchio */
            pickup->item = previous;
            pickup->cost = 0;         /* gia' pagato una volta: riprendersi il proprio non si paga */
            pickup->locked = true;    /* niente scambio a ripetizione restando fermi sul piedistallo */
            /* DEC-183: se il pickup era un Innesto persistente, dopo lo
               scambio e' 'previous' (l'Innesto appena tolto dallo slot) a
               restare a terra al suo posto -- il record di
               Game.droppedGrafts si aggiorna di conseguenza (stessa
               posizione/stanza, solo l'Item cambia), invece di restare
               agganciato all'oggetto ormai ripreso. 'isPersistedGraft'/
               'droppedGraftSlot' restano invariati su questo stesso Pickup,
               quindi resta comunque tracciato se il giocatore esce senza
               riprenderselo. */
            if (wasPersistedGraft && persistedSlotValid && previous.kind == ITEM_GRAFT)
            {
                game->droppedGrafts[persistedSlot].item = previous;
            }
        }
        else CombatApplyItem(game, taken);
        /* DEC-183: riprendere un Innesto persistente NON e' un premio della
           stanza -- e' l'oggetto del giocatore che torna al giocatore.
           Toccare 'rewardTaken'/la valuta di completamento qui bruciava un
           tesoro mai aperto (DEC-167) se lo si sganciava prima di toccare il
           piedistallo: quel ramo resta riservato a un Innesto/oggetto
           offerto DAVVERO da tesoro/negozio. */
        if (!wasPersistedGraft)
        {
            room->rewardTaken = true;
            if (firstReward && room->kind == ROOM_TREASURE) WorldAwardRoomCompletionCurrency(game, ROOM_TREASURE);
        }
        /* DEC-183: raccolta DIRETTA (nessuno scambio) di un Innesto
           persistente -- niente resta a terra al suo posto, il record si
           libera: la stanza non ha piu' nulla da ri-materializzare per
           QUESTO Innesto al prossimo ingresso. */
        if (wasPersistedGraft && swapIndex < 0 && persistedSlotValid)
        {
            game->droppedGrafts[persistedSlot].active = false;
        }
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
        bool overlap = GameMathLengthSquared(GameMathSubtract(p->pos, game->player.pos)) < r*r;
        /* Il blocco di uno scambio/sgancio si scioglie quando il giocatore si
           ALLONTANA, non dopo un tempo: un timer scadrebbe mentre il
           giocatore e' ancora fermo li' sopra e lo scambio ripartirebbe da
           solo (vedi Pickup.locked in core/game_types.h). */
        if (!overlap) { p->locked = false; continue; }
        if (p->locked) continue;
        CombatPickup(game, p);
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
    /* W8: gli effetti grafici scorrono con lo stesso orologio delle particelle
       (un solo passo di frame per il gioco intero) e si spengono da soli quando
       l'animazione e' finita. Nessun altro modulo li legge: non hanno
       collisione, danno ne' punteggio. */
    for (int i = 0; i < MAX_ART_FX; i++)
    {
        ArtFx *fx = &game->artFx[i];
        if (!fx->active) continue;
        fx->elapsed += dt;
        if (fx->elapsed >= fx->duration) fx->active = false;
    }
}
