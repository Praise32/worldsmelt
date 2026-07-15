#ifndef MELTING_RUN_GAME_MATH_H
#define MELTING_RUN_GAME_MATH_H

#include "core/game_types.h"

float GameMathClampFloat(float value, float minimum, float maximum);
int GameMathClampInt(int value, int minimum, int maximum);
float GameMathLengthSquared(Vector2 value);
Vector2 GameMathAdd(Vector2 a, Vector2 b);
Vector2 GameMathSubtract(Vector2 a, Vector2 b);
Vector2 GameMathScale(Vector2 value, float scalar);
Vector2 GameMathPerpendicular(Vector2 value);
Vector2 GameMathNormalize(Vector2 value);
unsigned int GameRngNext(unsigned int *state);
int GameRngRange(unsigned int *state, int minimum, int maximum);
float GameRngFloat(unsigned int *state, float minimum, float maximum);
Color GameColorWithAlpha(Color color, unsigned char alpha);
Color GameColorLerp(Color a, Color b, float amount);

/* Collisione cerchio-vs-rettangolo (fase 3c, ostacoli delle stanze). Se il cerchio
   di centro *pos e raggio 'radius' penetra il rettangolo 'rect', spinge *pos fuori
   lungo la via d'uscita piu' corta e ritorna true; altrimenti lascia *pos com'e' e
   ritorna false. Risoluzione standard "closest point on AABB": funziona sia quando
   il centro e' dentro il rettangolo sia quando e' fuori ma il cerchio lo sfiora. */
bool GameMathResolveCircleRect(Vector2 *pos, float radius, Rectangle rect);

/* Vero se il segmento dal punto 'a' al punto 'b' interseca il rettangolo 'rect'.
   Serve ai colpi: un colpo veloce puo' saltare oltre un ostacolo sottile in un
   frame, quindi non basta il test punto-dentro-rettangolo, serve il segmento del
   suo spostamento. */
bool GameMathSegmentHitsRect(Vector2 a, Vector2 b, Rectangle rect);

#endif
