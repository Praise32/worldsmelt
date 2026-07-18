#include "script/script_items.h"

#include "core/game_math.h"
#include "gameplay/synergies.h"
#include "script/script_api.h"
#include "script/script_character.h"
#include "script/script_sandbox.h"

#include "lauxlib.h"
#include "lua.h"

#include <math.h>
#include <string.h>

#define SCRIPT_ITEMS_NO_REF (-1)

/* ============================================================
   Ciclo di vita per-slot
   ============================================================ */

static void ScriptItemsResetSlot(ScriptItemRuntime *rt)
{
    rt->sandbox = NULL;
    rt->evalRef = SCRIPT_ITEMS_NO_REF;
    rt->fireRef = SCRIPT_ITEMS_NO_REF;
    rt->hitRef = SCRIPT_ITEMS_NO_REF;
    rt->tickRef = SCRIPT_ITEMS_NO_REF;
    rt->statsTableRef = SCRIPT_ITEMS_NO_REF;
}

void ScriptItemsInit(Game *game, const CharacterDef *character)
{
    for (int i = 0; i < MAX_ITEMS; i++) ScriptItemsResetSlot(&game->itemScripts[i]);
    game->statsDirty = false;
    /* M6b-2 (DEC-037), facciata: applica/scarica il trait Lua del
       personaggio PRIMA del ricalcolo, cosi' la prima ScriptItemsRecomputeStats
       qui sotto lo vede gia' attivo (0 oggetti posseduti, ma il trait conta:
       vedi il commento su quella funzione). */
    ScriptCharacterSetActive(game, character);
    ScriptItemsRecomputeStats(game);   /* 0 oggetti -> player.* = player.base* + trait */
}

void ScriptItemsShutdown(Game *game)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        if (rt->sandbox != NULL) ScriptSandboxDestroy((ScriptSandbox *)rt->sandbox);
        ScriptItemsResetSlot(rt);
    }
    ScriptCharacterShutdown(game);   /* M6b-2, facciata: stessa vita/morte degli oggetti */
}

static int ScriptItemsCacheGlobalRef(lua_State *L, const char *name)
{
    lua_getglobal(L, name);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return SCRIPT_ITEMS_NO_REF; }
    return luaL_ref(L, LUA_REGISTRYINDEX);   /* fa il pop da solo */
}

void ScriptItemsOnAcquire(Game *game, int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= MAX_ITEMS) return;
    ScriptItemRuntime *rt = &game->itemScripts[itemIndex];
    if (rt->sandbox != NULL) ScriptSandboxDestroy((ScriptSandbox *)rt->sandbox);
    ScriptItemsResetSlot(rt);

    const Item *item = &game->player.items[itemIndex];
    if (item->luaSource[0] == '\0') { game->statsDirty = true; return; }   /* solo mini-VM */

    /* Seed dedicato per questo script, tratto dalla RNG di gioco (gia'
       seminata una volta sola da GameResetRun) e avanzato: il prossimo
       consumatore (nemici, particelle...) resta nella stessa sequenza, e
       due run con lo stesso seed di gioco producono la stessa sequenza di
       semi per gli script -> stesso comportamento (spec, sezione 9,
       criterio 5). */
    unsigned int seed = GameRngNext(&game->rng);
    ScriptSandbox *sb = ScriptSandboxCreate(seed, SCRIPT_SANDBOX_DEFAULT_MEMORY_CAP);
    if (sb == NULL) { game->statsDirty = true; return; }   /* niente memoria: resta sulla mini-VM */

    ScriptApiRegister(sb, game);

    char err[160];
    if (!ScriptSandboxLoad(sb, item->name, item->luaSource, err, sizeof(err)))
    {
        /* Gia' loggato da ScriptSandboxKill dentro ScriptSandboxLoad. Si
           tiene comunque la sandbox (disabilitata): ScriptItemsHasActiveLua
           tornera' falso, l'oggetto ripiega sulla sua mini-VM da subito. La
           si tiene viva solo per coerenza diagnostica (DisabledReason), non
           costa il tetto di memoria Lua: lo stato Lua e' gia' inutilizzabile
           ma non viene chiuso finche' non arriva un nuovo ScriptItemsOnAcquire
           su questo slot o ScriptItemsShutdown. */
        rt->sandbox = sb;
        game->statsDirty = true;
        return;
    }

    lua_State *L = ScriptSandboxRawState(sb);
    rt->sandbox = sb;
    rt->evalRef = ScriptItemsCacheGlobalRef(L, "on_evaluate");
    /* Difesa in profondita' di tassonomia (review, "game-side taxonomy
       defense-in-depth"): per un oggetto ITEM_STATUP non si mettono MAI in
       cache i riferimenti a on_fire/on_hit/on_tick, anche se lo script Lua
       li definisse (es. un manifest modificato a mano che assegna un
       comportamento a un oggetto stat-up, aggirando GenLuaValidate, che gia'
       lo vieta lato generatore -- vedi statUpOnly sopra in gen_lua.c). Un
       solo guardiano qui basta: fireRef/hitRef/tickRef restano
       SCRIPT_ITEMS_NO_REF (gia' impostato da ScriptItemsResetSlot in cima a
       questa funzione), quindi ScriptItemsOnFire/OnHit/OnTick non chiamano
       mai nulla per questo slot, qualunque cosa lo script definisca. "gli
       stat-up non hanno comportamento" resta vero anche bypassando il
       generatore. */
    if (item->kind != ITEM_STATUP)
    {
        rt->fireRef = ScriptItemsCacheGlobalRef(L, "on_fire");
        rt->hitRef = ScriptItemsCacheGlobalRef(L, "on_hit");
        rt->tickRef = ScriptItemsCacheGlobalRef(L, "on_tick");
    }

    /* Tabella di scratch per on_evaluate, creata una volta e riusata ad ogni
       ricalcolo invece che allocata per chiamata (spec, sezione 5). Solo se
       lo script implementa davvero on_evaluate: altrimenti non serve mai. */
    if (rt->evalRef != SCRIPT_ITEMS_NO_REF)
    {
        lua_newtable(L);
        rt->statsTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    game->statsDirty = true;
}

bool ScriptItemsHasActiveLua(const Game *game, int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= MAX_ITEMS) return false;
    const ScriptSandbox *sb = (const ScriptSandbox *)game->itemScripts[itemIndex].sandbox;
    return sb != NULL && !ScriptSandboxIsDisabled(sb);
}

/* ============================================================
   Callback per-evento: funzione Lua cache (luaL_ref) + argomenti
   numerici, tutti passati attraverso ScriptSandboxProtectedCall
   (stesso budget/kill-switch di ScriptSandboxCallVoid, vedi
   script_sandbox.h).
   ============================================================ */

static void ScriptItemsCallCachedVoid(ScriptSandbox *sb, int ref, const double *args, int nargs)
{
    if (ref == SCRIPT_ITEMS_NO_REF) return;   /* hook non implementato: non e' un errore */
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    for (int i = 0; i < nargs; i++) lua_pushnumber(L, (lua_Number)args[i]);
    ScriptSandboxProtectedCall(sb, nargs, 0);
}

void ScriptItemsOnFire(Game *game, Vector2 pos, Vector2 dir)
{
    ScriptCharacterOnFire(game, pos, dir);   /* M6b-2, facciata: il trait prima o dopo gli oggetti non importa, sono chiamate indipendenti */
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[4] = { (double)pos.x, (double)pos.y, (double)dir.x, (double)dir.y };
        ScriptItemsCallCachedVoid(sb, rt->fireRef, args, 4);
    }
}

void ScriptItemsOnHit(Game *game, int shotIndex, int enemyIndex)
{
    if (shotIndex < 0 || shotIndex >= MAX_SHOTS || enemyIndex < 0 || enemyIndex >= MAX_ENEMIES) return;
    ScriptCharacterOnHit(game, shotIndex, enemyIndex);   /* M6b-2, facciata */
    double shotHandle = ScriptApiPackShotHandle(game, shotIndex);
    double enemyHandle = ScriptApiPackEnemyHandle(game, enemyIndex);
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[2] = { shotHandle, enemyHandle };
        ScriptItemsCallCachedVoid(sb, rt->hitRef, args, 2);
    }
}

void ScriptItemsOnTick(Game *game, float dt)
{
    ScriptCharacterOnTick(game, dt);   /* M6b-2, facciata */
    for (int i = 0; i < game->player.itemCount; i++)
    {
        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        if (sb == NULL || ScriptSandboxIsDisabled(sb)) continue;
        double args[1] = { (double)dt };
        ScriptItemsCallCachedVoid(sb, rt->tickRef, args, 1);
    }
}

/* ============================================================
   Il sistema delle cache: recompute-from-zero (spec, sezione 7)
   ============================================================ */

typedef struct ScriptItemsStatsAccum
{
    float damage;
    float fireDelay;
    float shotSpeed;
    float shotRadius;
    float speed;
    float maxHp;   /* float per condividere lo stesso clamp delle altre; arrotondata a int solo alla fine */
    float luck;    /* step C: vedi Player.luck in core/game_types.h */
} ScriptItemsStatsAccum;

/* Confini di sicurezza (spec, sezione 3 della task brief: "C clamps every
   field to sane bounds after each item's pass"). Scelti larghi ma finiti
   attorno ai valori di partenza di GameResetRun (damage 8, fireDelay 0.23,
   shotSpeed 520, shotRadius 5, speed 224, maxHp 6, tetto 12 gia' usato
   altrove per maxHp, vedi lo storico CombatApplyItem/TRAIT_VAMP): nessun
   oggetto, built-in o Lua, puo' produrre un giocatore che non spara piu',
   non si muove piu', o e' immortale/istantaneamente morto. */
#define SCRIPT_ITEMS_DAMAGE_MIN      0.5f
#define SCRIPT_ITEMS_DAMAGE_MAX      200.0f
/* Step C (curve alla Isaac, docs/references/formule-statistiche.md + spec
   2026-07-14-step-c-shottype-balance.md): la banda della cadenza non e' piu'
   [0.05, 2.0] ma [0.10, 1.2], e sono PAVIMENTI PRATICI, non solo confini di
   sicurezza. 0.05 s significa 20 colpi al secondo: nessun nemico di questo gioco
   e' tarato per reggerlo, ed e' il tipo di valore che un oggetto Lua ambizioso
   raggiungeva davvero impilando cadenza. All'altro estremo, un colpo ogni 2
   secondi non e' "un compromesso interessante": e' una run rovinata da un
   oggetto, esattamente il *dud* che questa fase deve rendere impossibile. Isaac
   fa la stessa cosa: la sua cadenza ha un tetto duro (il "tear delay" non
   scende sotto ~5 tick) e un pavimento sotto cui nessun oggetto ti spinge. */
#define SCRIPT_ITEMS_FIRE_DELAY_MIN  0.10f
#define SCRIPT_ITEMS_FIRE_DELAY_MAX  1.2f
/* Stesso ragionamento: 60 px/s sono 15 secondi per attraversare la stanza (876
   px). Il pavimento pratico e' meta' della velocita' di partenza (520/2 = 260):
   un tipo di colpo o un oggetto possono rendere i colpi LENTI, mai inutili. */
#define SCRIPT_ITEMS_SHOT_SPEED_MIN  260.0f
#define SCRIPT_ITEMS_SHOT_SPEED_MAX  1400.0f
#define SCRIPT_ITEMS_SHOT_RADIUS_MIN 2.0f
#define SCRIPT_ITEMS_SHOT_RADIUS_MAX 40.0f
#define SCRIPT_ITEMS_SPEED_MIN       60.0f
#define SCRIPT_ITEMS_SPEED_MAX       600.0f
#define SCRIPT_ITEMS_MAX_HP_MIN      1.0f
/* M6a (DEC-033): il tetto vero di maxHp non e' piu' un assoluto -- e'
   Player.hpCap, proprio di ciascun personaggio (vedi il commento su quel
   campo in core/game_types.h). Questo resta il valore STORICO, usato SOLO
   quando hpCap non e' impostato (<=0: un Player azzerato con memset, come
   fanno ancora molti test costruiti a mano senza passare da
   GamePlayerResetBaseStatsFor), vedi ScriptItemsHpCap sotto: e' cio' che
   garantisce che nessun test esistente cambi risultato. */
#define SCRIPT_ITEMS_MAX_HP_MAX      12.0f
/* Guardia di motore ASSOLUTA (M6a): a prescindere da quanto generoso sia
   hpCap di un personaggio -- curato oggi (rosa base, DEC-030), generato
   domani entro bande di default da playtest (personaggio alternativo per
   run, DEC-014/033, M6b) -- nessun personaggio puo' mai superare QUESTO
   valore. E' la rete di sicurezza del motore (protegge da un hpCap
   corrotto o da bande di generazione future troppo larghe), non un valore
   di design: quello resta hpCap, deciso per-personaggio. */
#define SCRIPT_ITEMS_MAX_HP_ABSOLUTE_MAX 24.0f
/* Fortuna (step C): banda alla Isaac. Puo' andare in negativo (un oggetto puo'
   costare fortuna in cambio d'altro) ma non sotto -5, e non oltre +15: oltre
   quella soglia ogni effetto a probabilita' sarebbe di fatto garantito. */
#define SCRIPT_ITEMS_LUCK_MIN        (-5.0f)
#define SCRIPT_ITEMS_LUCK_MAX        15.0f

/* M6a (DEC-033): tetto EFFETTIVO di maxHp per QUESTO player -- p->hpCap se
   impostato (>0), altrimenti il tetto storico assoluto (nessun personaggio
   applicato, vedi il commento sul campo). Passa comunque dalla guardia di
   motore assoluta: un hpCap corrotto o fuori banda non puo' mai superarla. */
static float ScriptItemsHpCap(const Player *p)
{
    float cap = (p->hpCap > 0) ? (float)p->hpCap : SCRIPT_ITEMS_MAX_HP_MAX;
    if (cap > SCRIPT_ITEMS_MAX_HP_ABSOLUTE_MAX) cap = SCRIPT_ITEMS_MAX_HP_ABSOLUTE_MAX;
    return cap;
}

static void ScriptItemsClampStats(ScriptItemsStatsAccum *acc, float hpCap)
{
    acc->damage     = GameMathClampFloat(acc->damage,     SCRIPT_ITEMS_DAMAGE_MIN,      SCRIPT_ITEMS_DAMAGE_MAX);
    acc->fireDelay  = GameMathClampFloat(acc->fireDelay,  SCRIPT_ITEMS_FIRE_DELAY_MIN,  SCRIPT_ITEMS_FIRE_DELAY_MAX);
    acc->shotSpeed  = GameMathClampFloat(acc->shotSpeed,  SCRIPT_ITEMS_SHOT_SPEED_MIN,  SCRIPT_ITEMS_SHOT_SPEED_MAX);
    acc->shotRadius = GameMathClampFloat(acc->shotRadius, SCRIPT_ITEMS_SHOT_RADIUS_MIN, SCRIPT_ITEMS_SHOT_RADIUS_MAX);
    acc->speed      = GameMathClampFloat(acc->speed,      SCRIPT_ITEMS_SPEED_MIN,       SCRIPT_ITEMS_SPEED_MAX);
    acc->maxHp      = GameMathClampFloat(acc->maxHp,      SCRIPT_ITEMS_MAX_HP_MIN,      hpCap);
    acc->luck       = GameMathClampFloat(acc->luck,       SCRIPT_ITEMS_LUCK_MIN,        SCRIPT_ITEMS_LUCK_MAX);
}

/* Rendimenti decrescenti sul danno (step C, la curva alla Isaac). In Isaac il
   danno non e' la somma dei bonus: passa per una radice
   (base*sqrt(1.2*ups + 1)), cosi' il primo oggetto che trovi si sente eccome e
   il decimo aggiunge ancora qualcosa, ma impilarne dieci non ti da' dieci volte
   il danno. Qui la stessa idea, adattata al nostro sistema (che accumula un
   VALORE, non un numero di "ups"): il danno resta INTATTO fino al doppio del
   valore di partenza -- la zona dove vive il 95% delle run, quindi nessun
   oggetto perde mordente e nessuno dei test esistenti cambia -- e sopra quella
   soglia viene compresso con una radice attorno ad essa.
   Concretamente, con base 8: 13 resta 13; 32 (cinque leggendari avidi impilati,
   vedi il test N) diventa 22.6; 200 (il tetto assoluto) diventa 63.2. La curva
   e' continua nel punto di raccordo (a d = 2b vale esattamente 2b), monotona
   crescente (un oggetto in piu' non fa MAI male) e senza limite superiore
   (nessun oggetto diventa mai del tutto inutile: aggiunge solo sempre meno).
   Applicata UNA VOLTA in fondo al ricalcolo, non dentro il ciclo: il ricalcolo
   riparte sempre da zero, quindi resta idempotente (test C) e i tetti
   per-oggetto (che confrontano pre/post DENTRO il ciclo) continuano a misurare
   il contributo grezzo dell'oggetto, non uno gia' compresso. */
static float ScriptItemsDamageCurve(float damage, float baseDamage)
{
    float knee = 2.0f*baseDamage;
    if (!(damage > knee) || knee <= 0.0f) return damage;   /* !(>) e non <=: NaN-safe, come GameMathClampFloat */
    return knee*sqrtf(damage/knee);
}

/* ============================================================
   Budget di potenza PER OGGETTO (fase 3, vision doc sezione 1: "un budget di
   potenza per oggetto, non bonus arbitrari"). SCRIPT_ITEMS_CLAMP_STATS sopra
   e' un tetto GLOBALE, indipendente da quanti oggetti hai: protegge il
   giocatore nel suo insieme (mai piu' veloce di X, mai piu' fragile di Y),
   ma da solo NON impedisce che un singolo oggetto malfatto (un 7B che sbaglia
   i conti, o semplicemente uno script troppo generoso) spinga UNA statistica
   fin quasi al tetto in un colpo solo. Qui sotto si aggiunge un secondo
   limite, per-oggetto: quanto puo' SPOSTARE quella statistica un singolo
   on_evaluate, a prescindere da cosa il tetto globale permetterebbe ancora.

   La percentuale e' relativa a player.base* (il valore di PARTENZA della
   run, fisso), non al valore corrente prima di questo oggetto: un oggetto
   raccolto per decimo non deve poter spostare la statistica piu' di uno
   raccolto per primo solo perche' i nove precedenti l'hanno gia' gonfiata.
   25% e' la stessa cifra per tutte le sei statistiche: abbastanza per
   sentire davvero un oggetto stat-up (su damage=8 sono +2, un incremento
   del 25% e' ben percepibile), abbastanza poco perche' anche il singolo
   oggetto peggio riuscito nell'intera run (al massimo 5 oggetti stat-up,
   uno per piano) non possa mai avvicinarsi da solo al tetto globale sopra
   (0.25*8=2 contro un tetto [0.5,200]; anche sommando tutti e 5 i piani al
   loro massimo assoluto restano ben dentro banda).

   Applicato SOLO agli oggetti ITEM_STATUP (task brief, fase 3: "cap how
   much a single stat-up item may shift any stat"), non a ogni on_evaluate:
   un oggetto ATTIVO puo' gia' definire on_evaluate oggi (fase 3a-L2, prima
   di questo task) per esprimere sinergie piu' ricche di un semplice
   modificatore statico (vedi src/tests/script_items_tests.c, test B/C, che
   sommano +2/+3 danno da due oggetti attivi ben oltre il 25% di un singolo
   oggetto) -- resta cosi', la sandbox non perde liberta'. Il budget
   per-oggetto e' la promessa di equilibrio specifica dei BOSS REWARD, non
   una restrizione nuova sull'intera libreria di oggetti attivi gia'
   esistente.

   MODIFICA QUI per ribilanciare (fase 3b, design doc, sezione 2): la
   frazione non e' piu' un'unica costante, e' una tavola indicizzata per
   Rarity (core/game_types.h). Un oggetto piu' raro puo' spostare una
   statistica di piu': comune 15%, non-comune 25% (la stessa cifra di prima
   di questa fase: e' il punto di riferimento "invariato" usato anche sotto
   per scalare il ripiego C), raro 40%, leggendario 60%. Il tetto GLOBALE
   (ScriptItemsClampStats) resta INVARIATO e gira SEMPRE dopo questo,
   qualunque sia la rarita': con base=8 (damage) il tetto per-oggetto piu'
   largo (leggendario, 60%) sposta al massimo 4.8 a botta, quindi anche
   cinque oggetti leggendari consecutivi (il massimo possibile: un solo
   bossItem per piano, 5 piani) restano a 8+5*4.8=32, ben dentro la banda
   assoluta [0.5,200] -- vedi src/tests/script_items_tests.c, test M/N, che
   verificano questi numeri per davvero. */
static const float SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION[4] = {
    0.15f,   /* RARITY_COMMON */
    0.25f,   /* RARITY_UNCOMMON -- invariata rispetto al vecchio tetto flat */
    0.40f,   /* RARITY_RARE */
    0.60f,   /* RARITY_LEGENDARY */
};

/* Difesa in profondita': una Rarity fuori range (non dovrebbe mai succedere,
   l'enum ha solo 4 valori e RarityFromText/GenNormalizeRun ricadono sempre
   su RARITY_COMMON per un testo sconosciuto) ricade sul tetto piu' stretto,
   mai sul piu' largo -- un dato corrotto deve rendere un oggetto PIU'
   debole, mai piu' forte del previsto. */
static float ScriptItemsRarityFraction(Rarity rarity)
{
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION[RARITY_COMMON];
    return SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION[rarity];
}

static float ScriptItemsClampItemDeltaField(float post, float pre, float base, float fraction)
{
    float cap = fabsf(base)*fraction;
    float delta = GameMathClampFloat(post - pre, -cap, cap);
    return pre + delta;
}

/* Applica il tetto per-oggetto SCALATO PER RARITA' a TUTTE le statistiche
   che 'post' porta rispetto a 'pre' (i valori subito prima di chiamare
   on_evaluate per QUESTO oggetto), scrivendo il risultato clampato in
   'post' stesso. Il tetto globale (ScriptItemsClampStats) va comunque
   richiamato DOPO questa funzione dal chiamante: sono due reti distinte,
   non una alternativa all'altra. */
/* La fortuna parte da ZERO (Player.baseLuck), quindi il tetto per-oggetto
   "frazione x valore di partenza" (che vale per tutte le altre statistiche)
   darebbe qui zero: nessun oggetto potrebbe mai dare un punto di fortuna. Serve
   quindi una base VIRTUALE, solo per il calcolo del tetto: 4 punti di fortuna e'
   la scala su cui la statistica ha senso (la banda utile e' [-5, +15]), quindi
   il tetto per-oggetto diventa comune 0.6, non-comune 1.0, raro 1.6, leggendario
   2.4 punti -- un leggendario "fortunato" sposta la probabilita' di VAMP di ~7
   punti percentuali (vedi CombatDamageEnemy), sensibile ma mai decisivo da solo. */
#define SCRIPT_ITEMS_LUCK_CAP_BASE 4.0f

static void ScriptItemsClampItemDelta(ScriptItemsStatsAccum *post, const ScriptItemsStatsAccum *pre, const Player *p, Rarity rarity)
{
    float fraction = ScriptItemsRarityFraction(rarity);
    post->damage     = ScriptItemsClampItemDeltaField(post->damage,     pre->damage,     p->baseDamage,     fraction);
    post->fireDelay  = ScriptItemsClampItemDeltaField(post->fireDelay,  pre->fireDelay,  p->baseFireDelay,  fraction);
    post->shotSpeed  = ScriptItemsClampItemDeltaField(post->shotSpeed,  pre->shotSpeed,  p->baseShotSpeed,  fraction);
    post->shotRadius = ScriptItemsClampItemDeltaField(post->shotRadius, pre->shotRadius, p->baseShotRadius, fraction);
    post->speed      = ScriptItemsClampItemDeltaField(post->speed,      pre->speed,      p->baseSpeed,      fraction);
    post->maxHp      = ScriptItemsClampItemDeltaField(post->maxHp,      pre->maxHp,      (float)p->baseMaxHp, fraction);
    post->luck       = ScriptItemsClampItemDeltaField(post->luck,       pre->luck,       SCRIPT_ITEMS_LUCK_CAP_BASE, fraction);
}

/* Ripiego fisso e sicuro (fase 3, task brief: "so a boss reward is never a
   dud"): un oggetto stat-up SENZA un on_evaluate Lua funzionante (mai
   generato, o generato ma bocciato dalla validazione/ucciso a runtime, vedi
   ScriptItemsRecomputeStats sotto) prende comunque UN bonus, piccolo ma
   reale, deciso qui in C e scelto in base al suo trait (lo stesso trait che
   nel manifest serve solo da "etichetta", vedi tools/melting-gen/gen_fallback.c
   FallbackBossItem): niente RNG, stesso trait -> sempre lo stesso bonus di
   base, cosi' il ripiego resta prevedibile e testabile quanto il resto del
   sistema delle cache.

   Fase 3b (design doc, sezione 2: "il ripiego 'mai un buco' rispetta la
   rarita' dell'oggetto"): il bonus di base viene scalato dalla STESSA
   tavola di frazioni usata dal tetto sopra, relativa alla frazione
   NON-COMUNE (0.25, il valore storico "invariato" di questa fase): un
   oggetto comune prende un bonus piu' piccolo (0.15/0.25=60%), uno raro o
   leggendario uno piu' grande (160%/240%). Passa comunque per
   ScriptItemsClampItemDelta subito dopo (vedi il chiamante) con lo stesso
   tetto per-rarita', quindi anche uno scalato resta dentro il budget del
   suo livello. Ordine di priorita' identico a ItemFirstTraitName
   (src/gameplay/item_traits.c): un solo trait guida un solo bonus. */
static void ScriptItemsApplyStatUpFallback(ScriptItemsStatsAccum *acc, const Item *item, Rarity rarity)
{
    float scale = ScriptItemsRarityFraction(rarity)/SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION[RARITY_UNCOMMON];
    if (item->traits & TRAIT_VAMP)         { acc->maxHp     += 1.0f*scale;  return; }
    if (item->traits & TRAIT_GIANT)        { acc->damage    += 1.5f*scale;  return; }
    if (item->traits & TRAIT_RAPID)        { acc->fireDelay -= 0.03f*scale; return; }
    if (item->traits & TRAIT_PIERCE)       { acc->damage    += 1.0f*scale;  return; }
    if (item->traits & TRAIT_HOMING)       { acc->shotSpeed += 60.0f*scale; return; }
    if (item->traits & TRAIT_BOUNCE)       { acc->shotSpeed += 60.0f*scale; return; }
    if (item->traits & TRAIT_EXPLODE)      { acc->shotRadius += 1.0f*scale; return; }
    if (item->traits & TRAIT_SPLIT)        { acc->shotRadius += 1.0f*scale; return; }
    if (item->traits & TRAIT_SLOW)         { acc->speed      += 20.0f*scale; return; }
    acc->maxHp += 1.0f*scale;   /* nessun trait riconosciuto: un cuore extra, sempre sicuro */
}

/* La stessa matematica che prima viveva UNA TANTUM (al pickup) in
   CombatApplyItem, ora ricalcolata da zero ad ogni passaggio: e' quello che
   rende l'aggiunta/rimozione di un oggetto priva di deriva (spec, sezione
   7). Valori invariati rispetto alla versione precedente di CombatApplyItem. */
static void ScriptItemsApplyBuiltin(ScriptItemsStatsAccum *acc, const Item *item)
{
    if (item->traits & TRAIT_RAPID) acc->fireDelay *= 0.92f;
    if (item->traits & TRAIT_GIANT)
    {
        acc->damage += 1.6f;
        acc->shotRadius += 0.8f;
    }
    if (item->traits & TRAIT_PIERCE) acc->damage += 0.8f;
    if (item->traits & TRAIT_VAMP) acc->maxHp += 1.0f;
    if (item->slot == SLOT_BODY) acc->maxHp += 1.0f;
    if (item->slot == SLOT_HAND) acc->damage += 1.0f;
    if (item->slot == SLOT_EYES) acc->shotSpeed += 25.0f;
    /* Step C: lo slot AURA da' fortuna, come HAND da' danno e EYES velocita' dei
       colpi. E' cio' che rende la nuova statistica REALE in ogni run, anche senza
       un solo script Lua che la tocchi: senza questa riga, luck resterebbe a zero
       per sempre in una run in cui il modello non ha generato nulla, e sarebbe
       una statistica finta. */
    if (item->slot == SLOT_AURA) acc->luck += 0.5f;
}

/* Chiama on_evaluate(stats) sulla tabella di scratch riusata (statsTableRef),
   scrivendo i valori CORRENTI di 'acc' prima della chiamata e rileggendoli
   dopo. Ritorna false se lo script e' stato ucciso durante la chiamata: in
   quel caso 'acc' NON viene toccato (i campi si leggono solo dopo un
   successo), quindi qualunque scrittura la tabella avesse ricevuto prima
   dell'errore (Lua non fa rollback delle mutazioni su una tabella
   condivisa) non raggiunge mai le statistiche vere del giocatore. */
static bool ScriptItemsCallEvaluate(ScriptItemRuntime *rt, ScriptItemsStatsAccum *acc)
{
    if (rt->evalRef == SCRIPT_ITEMS_NO_REF || rt->statsTableRef == SCRIPT_ITEMS_NO_REF) return true;
    ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->evalRef);          /* funzione: arg 1 di lua_pcall */
    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);    /* tabella di scratch: unico argomento */
    lua_pushnumber(L, (lua_Number)acc->damage);     lua_setfield(L, -2, "damage");
    lua_pushnumber(L, (lua_Number)acc->fireDelay);  lua_setfield(L, -2, "fire_delay");
    lua_pushnumber(L, (lua_Number)acc->shotSpeed);  lua_setfield(L, -2, "shot_speed");
    lua_pushnumber(L, (lua_Number)acc->shotRadius); lua_setfield(L, -2, "shot_radius");
    lua_pushnumber(L, (lua_Number)acc->speed);      lua_setfield(L, -2, "speed");
    lua_pushnumber(L, (lua_Number)acc->maxHp);      lua_setfield(L, -2, "max_hp");
    lua_pushnumber(L, (lua_Number)acc->luck);       lua_setfield(L, -2, "luck");

    if (!ScriptSandboxProtectedCall(sb, 1, 0)) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rt->statsTableRef);
    int t = lua_gettop(L);
    /* isfinite, non solo lua_isnumber: lua_isnumber torna vero anche per
       NaN/+-inf (sono "numeri" Lua a tutti gli effetti), quindi da solo non
       basta a fermare "stats.damage = 0/0" o "stats.speed = math.huge*0"
       (aritmetica pura, permessa dalla sandbox, che passa anche il dry-run
       del generatore). Un campo non finito viene scartato: 'acc' mantiene
       il valore che aveva PRIMA di questa chiamata (gia' dentro banda dal
       giro precedente), non un valore inventato qui. Questo e' il presidio
       al confine Lua->C; l'altro, indipendente, e' GameMathClampFloat reso
       NaN-safe (game_math.c) subito dopo, nel chiamante. Protegge anche il
       percorso pre-esistente degli oggetti ATTIVI con on_evaluate (vedi il
       commento sopra SCRIPT_ITEMS_ITEM_DELTA_FRACTION), non solo gli
       stat-up. */
    float v;
    lua_getfield(L, t, "damage");      if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->damage     = v; } lua_pop(L, 1);
    lua_getfield(L, t, "fire_delay");  if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->fireDelay  = v; } lua_pop(L, 1);
    lua_getfield(L, t, "shot_speed");  if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->shotSpeed  = v; } lua_pop(L, 1);
    lua_getfield(L, t, "shot_radius"); if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->shotRadius = v; } lua_pop(L, 1);
    lua_getfield(L, t, "speed");       if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->speed      = v; } lua_pop(L, 1);
    lua_getfield(L, t, "max_hp");      if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->maxHp      = v; } lua_pop(L, 1);
    lua_getfield(L, t, "luck");        if (lua_isnumber(L, -1)) { v = (float)lua_tonumber(L, -1); if (isfinite(v)) acc->luck       = v; } lua_pop(L, 1);
    lua_pop(L, 1);   /* la tabella stessa */
    return true;
}

void ScriptItemsProcessDirty(Game *game)
{
    if (!game->statsDirty) return;
    ScriptItemsRecomputeStats(game);
    game->statsDirty = false;
}

void ScriptItemsRecomputeStats(Game *game)
{
    Player *p = &game->player;
    float hpCap = ScriptItemsHpCap(p);   /* M6a (DEC-033): per-personaggio, vedi il commento sopra */
    ScriptItemsStatsAccum acc = {
        p->baseDamage, p->baseFireDelay, p->baseShotSpeed, p->baseShotRadius, p->baseSpeed, (float)p->baseMaxHp,
        p->baseLuck
    };

    /* M6b-2 (DEC-037): il trait del personaggio generato, applicato SUBITO
       dopo i base*, PRIMA di qualunque oggetto -- e' "parte del personaggio",
       non un oggetto raccolto durante la run (il commento su
       ScriptCharacterEvaluate in script_character.h spiega perche' i due
       moduli non condividono ScriptItemsStatsAccum). Nessun budget
       per-oggetto qui (quello e' riservato agli ITEM_STATUP, vedi
       SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION sopra): il trait passa SOLO
       dal tetto GLOBALE, come un oggetto ATTIVO con on_evaluate -- stesso
       principio (e' generato una volta sola con la STESSA pipeline di
       validazione degli oggetti, mai in corsa contro un budget di run
       intera). Nessun effetto se il trait non c'e'/non e' attivo/fallisce:
       'acc' resta quello che era, esattamente come un oggetto senza
       on_evaluate. */
    ScriptCharacterEvaluate(game, &acc.damage, &acc.fireDelay, &acc.shotSpeed, &acc.shotRadius,
                             &acc.speed, &acc.maxHp, &acc.luck);
    ScriptItemsClampStats(&acc, hpCap);

    /* Tipo di colpo (step C): stesso identico principio delle statistiche --
       si riparte da ZERO (nessun tipo = il colpo base) e si riscorrono gli
       oggetti in ordine di acquisizione. Vince l'ULTIMO che ne porta uno (alla
       Isaac: raccogliere una nuova "tear replacement" sostituisce la precedente,
       non si sommano), quindi togliere quell'oggetto fa automaticamente tornare
       il tipo di quello prima, senza alcuna contabilita' incrementale da
       disfare. */
    ShotTypeDef shotType;
    memset(&shotType, 0, sizeof(shotType));
    Color shotColor = (Color){ 0, 0, 0, 0 };
    /* QUALE oggetto ha dato il tipo di colpo (-1 = nessuno). Serve alle sinergie
       (correzione da review): una sinergia e' fra DUE oggetti diversi, quindi una
       regola che condiziona sul tipo di colpo deve sapere a chi attribuirlo --
       altrimenti un oggetto che porta un tipo di colpo che salta ED e' anche
       l'oggetto che rallenta sinergizzerebbe con se' stesso. */
    int shotTypeItem = -1;

    for (int i = 0; i < p->itemCount; i++)
    {
        const Item *item = &p->items[i];
        ScriptItemsApplyBuiltin(&acc, item);
        ScriptItemsClampStats(&acc, hpCap);

        if (item->shotType.active)
        {
            shotType = item->shotType;
            shotColor = item->color;
            shotTypeItem = i;
        }

        ScriptItemRuntime *rt = &game->itemScripts[i];
        ScriptSandbox *sb = (ScriptSandbox *)rt->sandbox;
        bool sandboxUsable = sb != NULL && !ScriptSandboxIsDisabled(sb);
        bool isStatUp = item->kind == ITEM_STATUP;

        /* on_evaluate, quando c'e' ed e' ancora vivo: per un oggetto
           ITEM_STATUP anche il budget per-oggetto (vedi
           ScriptItemsClampItemDelta sopra), SEMPRE seguito dal tetto
           globale. Un oggetto ATTIVO passa SOLO dal tetto globale, come
           prima di questa fase (vedi il commento sopra la macro). */
        bool ranLuaEval = false;
        if (sandboxUsable && rt->evalRef != SCRIPT_ITEMS_NO_REF)
        {
            ScriptItemsStatsAccum pre = acc;
            if (ScriptItemsCallEvaluate(rt, &acc))
            {
                if (isStatUp) ScriptItemsClampItemDelta(&acc, &pre, p, item->rarity);
                ranLuaEval = true;
            }
            ScriptItemsClampStats(&acc, hpCap);   /* di nuovo: anche dopo un fallimento, per sicurezza in profondita' */
        }

        /* Ripiego "mai un dud" (task brief, fase 3): un oggetto STAT-UP
           senza un on_evaluate Lua riuscito in QUESTO ricalcolo (mai
           acquisito con Lua, sandbox disabilitata dal patto di sicurezza, o
           script che non definisce on_evaluate) prende comunque il bonus
           fisso di ScriptItemsApplyStatUpFallback. Un oggetto ATTIVO senza
           on_evaluate resta invece esattamente come oggi: solo
           ScriptItemsApplyBuiltin, nessun bonus in piu' inventato qui. */
        if (!ranLuaEval && isStatUp)
        {
            ScriptItemsStatsAccum pre = acc;
            ScriptItemsApplyStatUpFallback(&acc, item, item->rarity);
            ScriptItemsClampItemDelta(&acc, &pre, p, item->rarity);
            ScriptItemsClampStats(&acc, hpCap);
        }
    }

    /* Il tipo di colpo si PUBBLICA QUI, prima delle sinergie, non in fondo
       (correzione da review). Motivo, e non e' un dettaglio di ordine: alcune
       sinergie condizionano sul tipo di colpo attivo (oggi "Arco Voltaico": un
       tipo che SALTA piu' un oggetto che rallenta, vedi synergies.c). Se le
       sinergie si rilevassero prima di questa scrittura, leggerebbero il tipo di
       colpo del ricalcolo PRECEDENTE -- cioe' ScriptItemsRecomputeStats leggerebbe
       il proprio output precedente, e smetterebbe di essere una funzione pura dei
       soli (oggetti, statistiche di base).
       Le conseguenze erano due, entrambe silenziose: la stessa identica coppia di
       oggetti dava o non dava la sinergia A SECONDA DELL'ORDINE in cui li avevi
       raccolti, e due ricalcoli di fila davano risultati diversi -- cioe' proprio
       le due promesse (nessuna deriva, idempotenza) su cui e' costruito tutto il
       sistema delle cache. Il test dell'idempotenza non se ne accorgeva perche'
       usa una coppia trait+trait, che non passa da qui.
       Difesa in profondita' (terza rete, dopo melting-gen e run_content.c): il tipo
       di colpo che finisce davvero nelle mani del giocatore e' SEMPRE ribilanciato,
       qualunque strada abbia preso per arrivare qui (un manifest modificato a mano,
       un test che costruisce un Item a mano). Idempotente: un tipo gia' in banda
       esce identico. */
    if (shotType.active) ShotTypeBalance(&shotType);
    p->shotType = shotType;
    p->shotColor = shotColor;
    p->shotTypeItem = shotTypeItem;

    /* Sinergie, CANALE A (step D, docs/references/design-sinergie.md sezione 4.3):
       il contributo statistico delle coppie attive si applica QUI -- dopo i
       contributi di tutti i singoli oggetti, prima dei clamp finali. E' l'intero
       motivo per cui questo passo e' poco codice: una sinergia e' solo "un altro
       modificatore nella stessa lista" del ricalcolo-da-zero, quindi e'
       idempotente per costruzione (test C) e passa per gli stessi tetti di
       sicurezza di tutto il resto (nemmeno una sinergia sbagliata puo' portare il
       giocatore fuori banda).
       La maschera si calcola DAGLI OGGETTI POSSEDUTI ORA, mai da p->traits (che e'
       un OR monotono e non si spegne piu'): e' cio' che fa spegnere pulita una
       sinergia quando togli uno dei due oggetti. */
    unsigned int synergies = SynergiesDetect(p);
    SynergyStatBonus bonus = SynergiesStatBonus(p, synergies);
    acc.damage    *= bonus.damageMul;
    acc.fireDelay *= bonus.fireDelayMul;
    acc.shotSpeed *= bonus.shotSpeedMul;
    acc.luck      += bonus.luckAdd;
    ScriptItemsClampStats(&acc, hpCap);

    /* La curva dei rendimenti decrescenti (step C) va QUI, dopo l'ultimo oggetto
       (e dopo le sinergie: anche il loro contributo e' danno, e deve rispettare
       la stessa curva) e prima della scrittura: vedi il commento su
       ScriptItemsDamageCurve. Il clamp globale gira comunque un'ultima volta dopo
       di lei -- la curva puo' solo ABBASSARE il danno, quindi non puo' sfondare il
       tetto, ma il pavimento va comunque garantito per un baseDamage patologico
       (0 o negativo) che nessuno dovrebbe mai impostare. */
    acc.damage = ScriptItemsDamageCurve(acc.damage, p->baseDamage);
    ScriptItemsClampStats(&acc, hpCap);

    p->damage = acc.damage;
    p->fireDelay = acc.fireDelay;
    p->shotSpeed = acc.shotSpeed;
    p->shotRadius = acc.shotRadius;
    p->speed = acc.speed;
    p->luck = acc.luck;
    p->synergies = synergies;
    p->maxHp = (int)(acc.maxHp + 0.5f);
    if (p->hp > p->maxHp) p->hp = p->maxHp;

}
