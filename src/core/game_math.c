#include "core/game_math.h"

#include <math.h>

float GameMathClampFloat(float v, float lo, float hi)
{
    /* "!(v > lo)" invece di "v < lo": su NaN ENTRAMBI i confronti < e >
       sono falsi (IEEE 754), quindi la forma naive (if v<lo return lo; if
       v>hi return hi; return v;) lascia passare NaN intatto invece di
       clamparlo. "!(v > lo)" cattura sia "v < lo" sia "v e' NaN" nello
       stesso ramo, mappando NaN sul bound basso: e' la scelta di sicurezza
       corretta ovunque questa funzione sia gia' usata (statistiche del
       giocatore, colore, ecc.), non solo per il chiamante che ha scoperto
       il buco (script_items.c, ScriptItemsClampStats/ClampItemDeltaField).
       Vedi anche il secondo presidio indipendente in
       ScriptItemsCallEvaluate (script_items.c): quel controllo scarta NaN
       PRIMA che arrivi qui, questo e' la rete di sicurezza di riserva. */
    if (!(v > lo)) return lo;
    if (v > hi) return hi;
    return v;
}

int GameMathClampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float GameMathLengthSquared(Vector2 v)
{
    return v.x*v.x + v.y*v.y;
}

Vector2 GameMathAdd(Vector2 a, Vector2 b)
{
    return (Vector2){ a.x + b.x, a.y + b.y };
}

Vector2 GameMathSubtract(Vector2 a, Vector2 b)
{
    return (Vector2){ a.x - b.x, a.y - b.y };
}

Vector2 GameMathScale(Vector2 v, float s)
{
    return (Vector2){ v.x*s, v.y*s };
}

Vector2 GameMathPerpendicular(Vector2 v)
{
    return (Vector2){ -v.y, v.x };
}

Vector2 GameMathNormalize(Vector2 v)
{
    float d = GameMathLengthSquared(v);
    if (d <= 0.0001f) return (Vector2){ 0.0f, 0.0f };
    float inv = 1.0f/sqrtf(d);
    return (Vector2){ v.x*inv, v.y*inv };
}

unsigned int GameRngNext(unsigned int *state)
{
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x ? x : 0xA341316Cu;
    return *state;
}

int GameRngRange(unsigned int *state, int minValue, int maxValue)
{
    if (maxValue <= minValue) return minValue;
    return minValue + (int)(GameRngNext(state)%((unsigned int)(maxValue - minValue + 1)));
}

float GameRngFloat(unsigned int *state, float minValue, float maxValue)
{
    float t = (float)(GameRngNext(state)&0xFFFFFF)/(float)0xFFFFFF;
    return minValue + (maxValue - minValue)*t;
}

Color GameColorWithAlpha(Color c, unsigned char a)
{
    c.a = a;
    return c;
}

Color GameColorLerp(Color a, Color b, float t)
{
    t = GameMathClampFloat(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r)*t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g)*t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b)*t),
        (unsigned char)((float)a.a + ((float)b.a - (float)a.a)*t)
    };
}
