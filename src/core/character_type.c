#include "core/character_type.h"

#include <string.h>

/* Vedi character_type.h per il perche' di questo modulo e per il motivo per
   cui NON include raylib ne' game_types.h -- stesso principio di shot_type.c,
   ClampF/ClampI reimplementate qui invece di riusare GameMathClampFloat/Int
   (core/game_math.h) proprio per non trascinarsi dietro quella dipendenza. */

static float ClampF(float v, float min, float max)
{
    /* NaN-safe come ClampF di shot_type.c: un NaN da un JSON malfatto non
       deve passare indenne dentro le statistiche di gioco, deve diventare
       il minimo (!(v > min) e' vero anche quando v e' NaN, a differenza di
       v < min). */
    if (!(v > min)) return min;
    if (v > max) return max;
    return v;
}

static int ClampI(int v, int min, int max)
{
    return v < min ? min : (v > max ? max : v);
}

/* M6b-3 (DEC-068): il massimo EFFETTIVO di una banda "piu' alto = piu'
   forte" quando il colpo firmato e' presente -- vedi il commento su
   CHARACTER_SHOT_CAUTION_FRACTION in character_type.h. */
static float CautiousMax(float bandMin, float bandMax)
{
    return bandMin + CHARACTER_SHOT_CAUTION_FRACTION*(bandMax - bandMin);
}

/* Lo stesso budget, ma per una banda "piu' BASSO = piu' forte" (fireDelay:
   un tempo di ricarica piu' corto spara piu' veloce): qui e' il MINIMO
   effettivo che sale, mai il massimo che scende. */
static float CautiousMin(float bandMin, float bandMax)
{
    return bandMax - CHARACTER_SHOT_CAUTION_FRACTION*(bandMax - bandMin);
}

void CharacterGenDefClamp(CharacterGenDef *def)
{
    if (!def) return;

    /* M6b-3 (DEC-068): il budget cauto sceglie i CONFINI passati a ClampF/
       ClampI sotto -- coi confini di sempre quando def->hasShot e' falso
       (comportamento IDENTICO a prima di questa fetta, nessuna regressione
       per un personaggio senza colpo firmato), coi confini compressi
       (CautiousMax/CautiousMin sopra) quando e' vero. shotSpeed e speed
       (movimento) restano SEMPRE sui confini di sempre, col o senza colpo
       firmato -- vedi il commento su CHARACTER_SHOT_CAUTION_FRACTION: il
       colpo firmato compra il proprio vantaggio offensivo/difensivo, non la
       mobilita' del personaggio. maxHp e' int: il tetto cauto (un float, es.
       6.6) viene TRONCATO verso il basso -- scelta deliberatamente
       conservativa (mai un intero che sfori il tetto cauto per un
       arrotondamento all'insu'), coerente col resto della fetta ("cauto"
       vuol dire non superare mai il budget, non avvicinarcisi il piu'
       possibile). */
    float damageMax = def->hasShot ? CautiousMax(CHARACTER_DAMAGE_MIN, CHARACTER_DAMAGE_MAX) : CHARACTER_DAMAGE_MAX;
    float fireDelayMin = def->hasShot ? CautiousMin(CHARACTER_FIRE_DELAY_MIN, CHARACTER_FIRE_DELAY_MAX) : CHARACTER_FIRE_DELAY_MIN;
    float luckMax = def->hasShot ? CautiousMax(CHARACTER_LUCK_MIN, CHARACTER_LUCK_MAX) : CHARACTER_LUCK_MAX;
    int maxHpMax = def->hasShot
        ? (int)CautiousMax((float)CHARACTER_MAX_HP_MIN, (float)CHARACTER_MAX_HP_MAX)
        : CHARACTER_MAX_HP_MAX;

    def->damage    = ClampF(def->damage,    CHARACTER_DAMAGE_MIN,     damageMax);
    def->fireDelay = ClampF(def->fireDelay, fireDelayMin,             CHARACTER_FIRE_DELAY_MAX);
    def->shotSpeed = ClampF(def->shotSpeed, CHARACTER_SHOT_SPEED_MIN, CHARACTER_SHOT_SPEED_MAX);
    def->speed     = ClampF(def->speed,     CHARACTER_SPEED_MIN,      CHARACTER_SPEED_MAX);
    def->maxHp     = ClampI(def->maxHp,     CHARACTER_MAX_HP_MIN,     maxHpMax);
    def->luck      = ClampF(def->luck,      CHARACTER_LUCK_MIN,       luckMax);
    /* Derivato, MAI letto dal file/dal modello: una settima manopola libera
       renderebbe il tetto di salute indipendente da maxHp, che e' proprio
       cio' che DEC-033 vuole evitare (il tetto e' parte della statistica,
       non un numero a se'). Deriva dal maxHp GIA' compresso sopra quando
       hasShot: un personaggio col colpo firmato e maxHp cauto ha anche un
       hpCap piu' cauto per costruzione, senza bisogno di una regola a
       parte. */
    def->hpCap = ClampI(def->maxHp * 2, CHARACTER_HP_CAP_MIN, CHARACTER_HP_CAP_MAX);

    /* M6b-3 (DEC-068): il colpo firmato stesso -- 'active' SEGUE 'hasShot'
       (mai letto per conto suo, vedi il commento sul campo in
       character_type.h), poi ShotTypeBalance/Clamp COSI' COME SONO (riuso
       puro, spec vincoli): stessa doppia rete di ShotTypeDef nel resto del
       gioco (tool prima di scrivere, gioco alla lettura), qui raggiunta
       gratis chiamando semplicemente CharacterGenDefClamp da entrambi i lati
       (vedi main.c/character_proposal.c). Senza colpo firmato, azzerato per
       intero: un personaggio senza colpo firmato non deve MAI portarsi
       dietro un residuo (nome/numeri di un colpo scartato che sopravvive a
       meta'), esattamente come i personaggi della rosa base che non toccano
       mai questo campo. */
    if (def->hasShot)
    {
        def->signatureShot.active = true;
        ShotTypeBalance(&def->signatureShot);
    }
    else
    {
        memset(&def->signatureShot, 0, sizeof(def->signatureShot));
    }
}
