#include "core/enemy_type.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Vedi enemy_type.h per il principio (il motore non ha un catalogo di nemici: li
   inventa il modello, il C garantisce l'equilibrio). */

static const char *ENEMY_FORM_TEXT[ENEMY_FORM_COUNT] = { "blob", "spiky", "armored", "floater" };
static const char *ENEMY_MOVE_TEXT[ENEMY_MOVE_COUNT] = { "chase", "kite", "orbit", "zigzag", "charge" };
static const char *ENEMY_FIRE_TEXT[ENEMY_FIRE_COUNT] = { "none", "single", "spread", "ring" };

static int TextIndex(const char *text, const char *const *table, int count)
{
    if (!text) return 0;
    for (int i = 0; i < count; i++)
    {
        if (strcmp(table[i], text) == 0) return i;
    }
    return 0;   /* sconosciuto -> il valore 0, che e' sempre il piu' innocuo */
}

EnemyForm EnemyFormFromText(const char *text) { return (EnemyForm)TextIndex(text, ENEMY_FORM_TEXT, ENEMY_FORM_COUNT); }
EnemyMove EnemyMoveFromText(const char *text) { return (EnemyMove)TextIndex(text, ENEMY_MOVE_TEXT, ENEMY_MOVE_COUNT); }
EnemyFire EnemyFireFromText(const char *text) { return (EnemyFire)TextIndex(text, ENEMY_FIRE_TEXT, ENEMY_FIRE_COUNT); }

const char *EnemyFormName(EnemyForm form)
{
    if (form < 0 || form >= ENEMY_FORM_COUNT) return ENEMY_FORM_TEXT[0];
    return ENEMY_FORM_TEXT[form];
}

const char *EnemyMoveName(EnemyMove move)
{
    if (move < 0 || move >= ENEMY_MOVE_COUNT) return ENEMY_MOVE_TEXT[0];
    return ENEMY_MOVE_TEXT[move];
}

const char *EnemyFireName(EnemyFire fire)
{
    if (fire < 0 || fire >= ENEMY_FIRE_COUNT) return ENEMY_FIRE_TEXT[0];
    return ENEMY_FIRE_TEXT[fire];
}

static float ClampF(float v, float min, float max)
{
    if (!(v > min)) return min;   /* NaN-safe, come GameMathClampFloat */
    if (v > max) return max;
    return v;
}

static int ClampI(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

void EnemyTypeClamp(EnemyTypeDef *type)
{
    if (!type) return;
    type->hpMul    = ClampF(type->hpMul,    ENEMY_TYPE_HP_MIN,    ENEMY_TYPE_HP_MAX);
    type->speedMul = ClampF(type->speedMul, ENEMY_TYPE_SPEED_MIN, ENEMY_TYPE_SPEED_MAX);
    type->sizeMul  = ClampF(type->sizeMul,  ENEMY_TYPE_SIZE_MIN,  ENEMY_TYPE_SIZE_MAX);
    type->fireRate = ClampF(type->fireRate, ENEMY_TYPE_RATE_MIN,  ENEMY_TYPE_RATE_MAX);
    type->pellets  = ClampI(type->pellets,  1, ENEMY_TYPE_PELLETS_MAX);
    if (type->form < 0 || type->form >= ENEMY_FORM_COUNT) type->form = ENEMY_FORM_BLOB;
    if (type->move < 0 || type->move >= ENEMY_MOVE_COUNT) type->move = ENEMY_MOVE_CHASE;
    if (type->fire < 0 || type->fire >= ENEMY_FIRE_COUNT) type->fire = ENEMY_FIRE_NONE;
    /* Coerenza fra 'fire' e 'fireRate': un nemico che dichiara di sparare ma ha
       cadenza zero non spara -- e allora e' ENEMY_FIRE_NONE, non un tiratore che
       sta zitto. Normalizzarlo qui evita che il resto del codice debba controllare
       due campi ogni volta (e che la potenza conti un'offesa che non esiste). */
    if (type->fireRate <= 0.0f) type->fire = ENEMY_FIRE_NONE;
    if (type->fire == ENEMY_FIRE_NONE) { type->fireRate = 0.0f; type->pellets = 1; }
    if (type->fire == ENEMY_FIRE_SINGLE) type->pellets = 1;   /* un colpo mirato e' UN colpo */
}

/* Quanto "offende" un nemico: il contatto (che dipende dalla sua stazza: una massa
   grossa e veloce ti chiude gli spazi) piu' il fuoco (cadenza x colpi per raffica,
   sublineare nei colpi: una corona di 8 proiettili non e' otto volte piu' letale
   di uno). Un nemico che non spara vale comunque piu' di zero: e' un corpo che ti
   viene addosso. */
static float PowerOffense(const EnemyTypeDef *type)
{
    float contact = 0.55f + 0.25f*type->sizeMul + 0.30f*type->speedMul;
    float shots = (type->fire == ENEMY_FIRE_NONE)
        ? 0.0f
        : type->fireRate*powf((float)type->pellets, 0.7f)*(type->fire == ENEMY_FIRE_RING ? 0.55f : 0.85f);
    /* La corona (RING) pesa meno per colpo: spara in tutte le direzioni, quindi solo
       una frazione dei proiettili ti riguarda davvero. */
    return contact + 0.42f*shots;
}

/* Quanto e' DIFFICILE da eliminare: la vita, corretta da quanto e' facile
   colpirlo (grande = piu' facile, quindi vale meno; veloce = piu' difficile). */
static float PowerDurability(const EnemyTypeDef *type)
{
    float hitEase = 0.75f + 0.25f*type->sizeMul;      /* un bersaglio grande si becca piu' colpi */
    float evasion = 0.85f + 0.15f*type->speedMul;     /* uno veloce ne schiva qualcuno */
    return type->hpMul*evasion/hitEase;
}

/* I movimenti non sono tutti ugualmente scomodi da affrontare: uno che ti insegue
   dritto e' pane, uno che ti gira attorno o serpeggia e' un problema. Fattore
   mite: e' un condimento della potenza, non il piatto. */
static float PowerMoveFactor(const EnemyTypeDef *type)
{
    switch (type->move)
    {
        case ENEMY_MOVE_KITE:   return 1.10f;   /* non lo raggiungi mai */
        case ENEMY_MOVE_ORBIT:  return 1.12f;   /* sempre di fianco, mai dove spari */
        case ENEMY_MOVE_ZIGZAG: return 1.15f;   /* difficile da centrare */
        case ENEMY_MOVE_CHARGE: return 1.08f;   /* pericoloso a scatti, ma prevedibile */
        case ENEMY_MOVE_CHASE:
        default:                return 1.0f;
    }
}

float EnemyTypePower(const EnemyTypeDef *type)
{
    if (!type || !type->active) return 1.0f;
    return PowerDurability(type)*PowerOffense(type)*PowerMoveFactor(type);
}

/* Tutto tranne hpMul: il termine per cui EnemyTypeBalance risolve. */
static float RestPower(const EnemyTypeDef *type)
{
    EnemyTypeDef unit = *type;
    unit.hpMul = 1.0f;
    return PowerDurability(&unit)*PowerOffense(&unit)*PowerMoveFactor(&unit);
}

void EnemyTypeBalance(EnemyTypeDef *type)
{
    if (!type) return;
    EnemyTypeClamp(type);
    if (!type->active) return;

    /* Banda diversa per i boss: vedi il commento su ENEMY_TYPE_BOSS_POWER_* in
       enemy_type.h -- usare la banda dei nemici normali sul boss lo INDEBOLIREBBE
       (la rete gli taglierebbe la vita per riportarlo a potenza 1.0). */
    const float target = type->boss ? ENEMY_TYPE_BOSS_POWER_TARGET : ENEMY_TYPE_POWER_TARGET;
    const float bandMin = type->boss ? ENEMY_TYPE_BOSS_POWER_MIN : ENEMY_TYPE_POWER_MIN;
    const float bandMax = type->boss ? ENEMY_TYPE_BOSS_POWER_MAX : ENEMY_TYPE_POWER_MAX;

    for (int guard = 0; guard < 16; guard++)
    {
        float rest = RestPower(type);
        if (rest <= 0.0f) return;

        float power = type->hpMul*rest;
        if (power >= bandMin && power <= bandMax) return;   /* gia' in banda: si rispetta la scelta del modello */

        type->hpMul = ClampF(target/rest, ENEMY_TYPE_HP_MIN, ENEMY_TYPE_HP_MAX);
        power = type->hpMul*rest;
        if (power >= bandMin && power <= bandMax) return;

        /* Ancora fuori banda. Verso il BASSO non si puo' fare altro (il nemico piu'
           fiacco possibile con la vita al massimo e' quello che e': lasciarlo cosi'
           e' giusto -- un nemico debole non rompe niente, e il budget della stanza
           ne fara' spawnare di piu'). Verso l'ALTO si tagliano le manopole
           offensive, in ordine di quanto pesano: prima i colpi per raffica, poi la
           cadenza. */
        if (power < bandMin) return;
        if (type->pellets > 1) type->pellets--;
        else if (type->fireRate > 0.1f) type->fireRate -= 0.25f;
        else if (type->fireRate > 0.0f) { type->fireRate = 0.0f; type->fire = ENEMY_FIRE_NONE; }
        else return;   /* niente piu' da tagliare: e' un corpo grosso e veloce, e va bene cosi' */
        EnemyTypeClamp(type);
    }
}

void EnemyTypeExample(EnemyTypeDef *out, int index)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->active = true;

    /* I tre nemici storici del gioco, riscritti nel vocabolario nuovo. NON sono
       "i nemici del gioco" (vedi il commento in cima all'header): sono il ripiego
       per quando il modello non c'e', e gli esempi che il prompt mostra al modello
       dicendogli di NON copiarli. */
    switch (((index % ENEMY_TYPE_EXAMPLE_COUNT) + ENEMY_TYPE_EXAMPLE_COUNT) % ENEMY_TYPE_EXAMPLE_COUNT)
    {
        case 1:   /* il tiratore: si tiene a distanza e spara colpi mirati */
            snprintf(out->name, sizeof(out->name), "Tiratore");
            out->form = ENEMY_FORM_SPIKY; out->move = ENEMY_MOVE_KITE; out->fire = ENEMY_FIRE_SINGLE;
            out->hpMul = 1.0f; out->speedMul = 0.85f; out->sizeMul = 1.0f; out->fireRate = 1.0f; out->pellets = 1;
            break;
        case 2:   /* il corazzato: lento, grosso, duro, spara ventagli */
            snprintf(out->name, sizeof(out->name), "Corazzato");
            out->form = ENEMY_FORM_ARMORED; out->move = ENEMY_MOVE_CHASE; out->fire = ENEMY_FIRE_SPREAD;
            out->hpMul = 1.9f; out->speedMul = 0.6f; out->sizeMul = 1.35f; out->fireRate = 0.7f; out->pellets = 3;
            break;
        default:  /* l'inseguitore: veloce, fragile, solo contatto */
            snprintf(out->name, sizeof(out->name), "Inseguitore");
            out->form = ENEMY_FORM_BLOB; out->move = ENEMY_MOVE_CHASE; out->fire = ENEMY_FIRE_NONE;
            out->hpMul = 0.75f; out->speedMul = 1.25f; out->sizeMul = 0.9f; out->fireRate = 0.0f; out->pellets = 1;
            break;
    }

    EnemyTypeBalance(out);
}

void EnemyTypeExampleBoss(EnemyTypeDef *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->active = true;
    out->boss = true;
    snprintf(out->name, sizeof(out->name), "Custode");
    out->form = ENEMY_FORM_FLOATER;
    out->move = ENEMY_MOVE_ORBIT;
    out->fire = ENEMY_FIRE_RING;
    out->hpMul = 1.0f; out->speedMul = 0.8f; out->sizeMul = 1.0f; out->fireRate = 1.0f; out->pellets = 8;
    EnemyTypeBalance(out);
}
