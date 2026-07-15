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

bool GameMathResolveCircleRect(Vector2 *pos, float radius, Rectangle rect)
{
    /* Punto del rettangolo piu' vicino al centro del cerchio. */
    float nx = GameMathClampFloat(pos->x, rect.x, rect.x + rect.width);
    float ny = GameMathClampFloat(pos->y, rect.y, rect.y + rect.height);
    float dx = pos->x - nx;
    float dy = pos->y - ny;
    float d2 = dx*dx + dy*dy;

    if (d2 > radius*radius) return false;   /* non tocca */

    if (d2 > 0.0001f)
    {
        /* Il centro e' FUORI dal rettangolo ma il cerchio lo sfiora: spingi lungo la
           normale (dal punto piu' vicino al centro) fino a staccare. */
        float d = sqrtf(d2);
        float push = radius - d;
        pos->x += (dx/d)*push;
        pos->y += (dy/d)*push;
        return true;
    }

    /* Il centro e' DENTRO il rettangolo: esci dal lato piu' vicino (la penetrazione
       minima fra i quattro lati). */
    float left = pos->x - rect.x;
    float right = (rect.x + rect.width) - pos->x;
    float top = pos->y - rect.y;
    float bottom = (rect.y + rect.height) - pos->y;
    float minH = left < right ? left : right;
    float minV = top < bottom ? top : bottom;
    if (minH < minV)
    {
        pos->x += (left < right) ? -(left + radius) : (right + radius);
    }
    else
    {
        pos->y += (top < bottom) ? -(top + radius) : (bottom + radius);
    }
    return true;
}

bool GameMathSegmentHitsRect(Vector2 a, Vector2 b, Rectangle rect)
{
    /* Estremi gia' dentro: intersezione ovvia. */
    if (a.x >= rect.x && a.x <= rect.x + rect.width && a.y >= rect.y && a.y <= rect.y + rect.height) return true;
    if (b.x >= rect.x && b.x <= rect.x + rect.width && b.y >= rect.y && b.y <= rect.y + rect.height) return true;

    /* Clipping di Liang-Barsky del segmento contro il rettangolo: se resta un tratto
       parametrico valido, il segmento attraversa. */
    float x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    float dx = x1 - x0, dy = y1 - y0;
    float p[4] = { -dx, dx, -dy, dy };
    float q[4] = { x0 - rect.x, (rect.x + rect.width) - x0, y0 - rect.y, (rect.y + rect.height) - y0 };
    float t0 = 0.0f, t1 = 1.0f;
    for (int i = 0; i < 4; i++)
    {
        if (p[i] == 0.0f)
        {
            if (q[i] < 0.0f) return false;   /* parallelo e fuori */
        }
        else
        {
            float t = q[i]/p[i];
            if (p[i] < 0.0f) { if (t > t1) return false; if (t > t0) t0 = t; }
            else             { if (t < t0) return false; if (t < t1) t1 = t; }
        }
    }
    return t0 <= t1;
}
