#include "core/character_type.h"

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

void CharacterGenDefClamp(CharacterGenDef *def)
{
    if (!def) return;
    def->damage    = ClampF(def->damage,    CHARACTER_DAMAGE_MIN,     CHARACTER_DAMAGE_MAX);
    def->fireDelay = ClampF(def->fireDelay, CHARACTER_FIRE_DELAY_MIN, CHARACTER_FIRE_DELAY_MAX);
    def->shotSpeed = ClampF(def->shotSpeed, CHARACTER_SHOT_SPEED_MIN, CHARACTER_SHOT_SPEED_MAX);
    def->speed     = ClampF(def->speed,     CHARACTER_SPEED_MIN,      CHARACTER_SPEED_MAX);
    def->maxHp     = ClampI(def->maxHp,     CHARACTER_MAX_HP_MIN,     CHARACTER_MAX_HP_MAX);
    def->luck      = ClampF(def->luck,      CHARACTER_LUCK_MIN,       CHARACTER_LUCK_MAX);
    /* Derivato, MAI letto dal file/dal modello: una settima manopola libera
       renderebbe il tetto di salute indipendente da maxHp, che e' proprio
       cio' che DEC-033 vuole evitare (il tetto e' parte della statistica,
       non un numero a se'). */
    def->hpCap = ClampI(def->maxHp * 2, CHARACTER_HP_CAP_MIN, CHARACTER_HP_CAP_MAX);
}
