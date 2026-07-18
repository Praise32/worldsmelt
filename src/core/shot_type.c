#include "core/shot_type.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Vedi shot_type.h per il perche' di questo modulo (i tipi di colpo li inventa
   il modello, il C fornisce solo il vocabolario e la garanzia di equilibrio) e
   per il motivo per cui NON include raylib ne' game_types.h. */

static const char *SHOT_FORM_TEXT[SHOT_FORM_COUNT] = {
    "orb", "spike", "beam", "arc", "blade"
};

ShotForm ShotFormFromText(const char *text)
{
    if (!text) return SHOT_FORM_ORB;
    for (int i = 0; i < (int)SHOT_FORM_COUNT; i++)
    {
        if (strcmp(SHOT_FORM_TEXT[i], text) == 0) return (ShotForm)i;
    }
    return SHOT_FORM_ORB;   /* testo sconosciuto: la palla di sempre, mai una forma esotica per sbaglio */
}

const char *ShotFormName(ShotForm form)
{
    if (form < 0 || form >= SHOT_FORM_COUNT) return SHOT_FORM_TEXT[SHOT_FORM_ORB];
    return SHOT_FORM_TEXT[form];
}

static float ClampF(float v, float min, float max)
{
    /* NaN-safe come GameMathClampFloat (core/game_math.c): un NaN che arrivasse
       da un JSON malfatto non deve passare indenne dentro le statistiche di
       gioco, deve diventare il minimo. */
    if (!(v > min)) return min;
    if (v > max) return max;
    return v;
}

static int ClampI(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

void ShotTypeClamp(ShotTypeDef *type)
{
    if (!type) return;
    type->speedMul    = ClampF(type->speedMul,  SHOT_TYPE_SPEED_MIN,  SHOT_TYPE_SPEED_MAX);
    type->damageMul   = ClampF(type->damageMul, SHOT_TYPE_DAMAGE_MIN, SHOT_TYPE_DAMAGE_MAX);
    type->radiusMul   = ClampF(type->radiusMul, SHOT_TYPE_RADIUS_MIN, SHOT_TYPE_RADIUS_MAX);
    type->lifeMul     = ClampF(type->lifeMul,   SHOT_TYPE_LIFE_MIN,   SHOT_TYPE_LIFE_MAX);
    type->pierceBonus = ClampI(type->pierceBonus, 0, SHOT_TYPE_PIERCE_MAX);
    type->chain       = ClampI(type->chain,       0, SHOT_TYPE_CHAIN_MAX);
    type->pellets     = ClampI(type->pellets,     1, SHOT_TYPE_PELLETS_MAX);
    if (type->form < 0 || type->form >= SHOT_FORM_COUNT) type->form = SHOT_FORM_ORB;
}

/* I tre contributi DISCRETI, isolati perche' ShotTypeBalance deve sapere quale
   pesa di piu' per decidere cosa tagliare per primo (vedi sotto). */
static float PowerPellets(const ShotTypeDef *type)
{
    /* Sublineare: tre pallettoni non fanno tre volte il danno di uno (mancano
       piu' spesso, e il ventaglio si apre). 0.85 e' l'esponente scelto. */
    return powf((float)type->pellets, 0.85f);
}

static float PowerPierce(const ShotTypeDef *type)
{
    /* Ogni nemico attraversato in piu' e' un bersaglio in piu' colpito, ma solo
       quando i nemici sono allineati: vale meno di un colpo intero. */
    return 1.0f + 0.55f*(float)type->pierceBonus;
}

static float PowerChain(const ShotTypeDef *type)
{
    /* La catena colpisce un secondo bersaglio a danno ridotto (0.65x, vedi
       combat.c) e solo se c'e' un nemico vicino: vale meno della perforazione. */
    return 1.0f + 0.45f*(float)type->chain;
}

static float PowerContinuous(const ShotTypeDef *type)
{
    /* Velocita', raggio e vita non moltiplicano il danno: rendono piu' facile
       COLPIRE. Pesati sublinearmente attorno a 1 (un tipo lento/piccolo/corto
       resta giocabile, uno veloce/grande/lungo non diventa un'altra categoria). */
    float speed  = 0.75f + 0.25f*type->speedMul;
    float radius = 0.80f + 0.20f*type->radiusMul;
    float life   = 0.85f + 0.15f*type->lifeMul;
    return speed*radius*life;
}

/* Tutto tranne damageMul: e' il termine per cui ShotTypeBalance risolve. */
static float ShotTypeRestPower(const ShotTypeDef *type)
{
    return PowerPellets(type)*PowerPierce(type)*PowerChain(type)*PowerContinuous(type);
}

float ShotTypePower(const ShotTypeDef *type)
{
    if (!type || !type->active) return 1.0f;   /* nessun tipo = esattamente il colpo base */
    return type->damageMul*ShotTypeRestPower(type);
}

/* Taglia di UNA tacca la manopola discreta che contribuisce di piu' alla
   potenza. Tagliare "la piu' grossa" invece che sempre la stessa preserva
   l'identita' del tipo il piu' a lungo possibile: un tipo che punta TUTTO sulla
   catena la conserva (non ha altro da tagliare), mentre uno che ha chiesto tutto
   al massimo viene sfoltito in modo uniforme invece che svuotato di una sola
   caratteristica. Ritorna false se non c'e' piu' niente da tagliare (tutte le
   manopole discrete sono gia' al minimo). */
static bool ShotTypeCutStrongestKnob(ShotTypeDef *type)
{
    float pellets = (type->pellets > 1)     ? PowerPellets(type) : 0.0f;
    float pierce  = (type->pierceBonus > 0) ? PowerPierce(type)  : 0.0f;
    float chain   = (type->chain > 0)       ? PowerChain(type)   : 0.0f;

    if (pellets <= 0.0f && pierce <= 0.0f && chain <= 0.0f) return false;

    if (pellets >= pierce && pellets >= chain) type->pellets--;
    else if (pierce >= chain) type->pierceBonus--;
    else type->chain--;
    return true;
}

void ShotTypeExample(ShotTypeDef *out, int index)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->active = true;

    /* Tre esempi che coprono tre STRATEGIE diverse (non tre estetiche diverse):
       perforare, arrivare lontano, saltare fra i nemici. Sono anche i tre esempi
       citati nel prompt del generatore (tools/melting-gen/prompts/system.txt), che
       dice esplicitamente al modello di NON copiarli ma di inventarne di propri. */
    switch (((index % SHOT_TYPE_EXAMPLE_COUNT) + SHOT_TYPE_EXAMPLE_COUNT) % SHOT_TYPE_EXAMPLE_COUNT)
    {
        case 1:   /* raggio: velocissimo e sottile, lunga gittata, perfora molto */
            snprintf(out->name, sizeof(out->name), "Beam");
            out->form = SHOT_FORM_BEAM;
            out->speedMul = 1.9f; out->damageMul = 0.6f; out->radiusMul = 0.5f; out->lifeMul = 1.6f;
            out->pierceBonus = 2; out->chain = 0; out->pellets = 1;
            break;
        case 2:   /* scarica: lenta e grossa, ma salta di nemico in nemico */
            snprintf(out->name, sizeof(out->name), "Jolt");
            out->form = SHOT_FORM_ARC;
            out->speedMul = 0.8f; out->damageMul = 0.9f; out->radiusMul = 1.2f; out->lifeMul = 0.9f;
            out->pierceBonus = 0; out->chain = 2; out->pellets = 1;
            break;
        default:  /* chiodo: veloce, piccolo, perfora un nemico */
            snprintf(out->name, sizeof(out->name), "Spikes");
            out->form = SHOT_FORM_SPIKE;
            out->speedMul = 1.45f; out->damageMul = 0.75f; out->radiusMul = 0.65f; out->lifeMul = 1.0f;
            out->pierceBonus = 1; out->chain = 0; out->pellets = 1;
            break;
    }

    ShotTypeBalance(out);
}

void ShotTypeBalance(ShotTypeDef *type)
{
    if (!type) return;
    ShotTypeClamp(type);
    if (!type->active) return;

    /* Al massimo 3+3+2 = 8 tagli possibili (chain, pierce, pellets dal loro
       massimo al minimo), piu' un giro finale: 16 e' un tetto abbondante che
       rende impossibile un ciclo infinito anche se qualcuno cambiasse le bande
       sopra senza ripensare a questo ciclo. */
    for (int guard = 0; guard < 16; guard++)
    {
        float rest = ShotTypeRestPower(type);
        if (rest <= 0.0f) return;   /* non puo' succedere (ogni fattore e' > 0), ma mai una divisione per zero */

        float power = type->damageMul*rest;
        /* Il tipo che il modello ha inventato e' GIA' in banda: si rispetta la
           sua scelta di danno invece di riscriverla (il modello ha un'idea di
           quanto deve picchiare "un chiodo" contro "una lama"; se e' equilibrata
           non c'e' motivo di sovrascriverla). */
        if (power >= SHOT_TYPE_POWER_MIN && power <= SHOT_TYPE_POWER_MAX) return;

        /* Fuori banda: si risolve damageMul per centrare il bersaglio. */
        type->damageMul = ClampF(SHOT_TYPE_POWER_TARGET/rest, SHOT_TYPE_DAMAGE_MIN, SHOT_TYPE_DAMAGE_MAX);
        power = type->damageMul*rest;
        if (power >= SHOT_TYPE_POWER_MIN && power <= SHOT_TYPE_POWER_MAX) return;

        /* Ancora fuori banda: puo' succedere SOLO verso l'alto (un tipo con
           troppe manopole discrete al massimo resta rotto anche col danno
           minimo). Verso il basso non e' possibile: anche il tipo piu' fiacco
           possibile (tutto al minimo) ha rest ~0.71, e damageMul puo' salire
           fino a 2.0, quindi il bersaglio 1.0 e' sempre raggiungibile. */
        if (power < SHOT_TYPE_POWER_MIN) return;
        if (!ShotTypeCutStrongestKnob(type)) return;
    }
}
