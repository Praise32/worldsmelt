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

#endif
