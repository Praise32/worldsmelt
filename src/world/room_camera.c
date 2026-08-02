#include "world/room_camera.h"

#include "core/game_math.h"

#include <math.h>

/* Vedi room_camera.h per il principio (zoom fisso, clamp ai bordi, 1x1 come
   caso degenere e non come eccezione). */

Rectangle WorldCameraBoundsFromRoom(Rectangle roomRect)
{
    /* DEC-200: la cornice viene da ROOM_FRAME_W/H, non piu' dal canvas -- le
       due cose erano lo stesso 960x640 fino a DEC-174 e si sono separate con
       la migrazione a 640x360. Derivarla ancora dal canvas darebbe qui una
       cornice NEGATIVA a destra e in basso (640-918, 360-562): il rettangolo
       di clamp collasserebbe sulla vista e la telecamera mostrerebbe sempre
       l'angolo in alto a sinistra della stanza, col giocatore libero di
       uscire dall'inquadratura. */
    const float left = ROOM_X;
    const float top = ROOM_Y;
    const float right = ROOM_FRAME_W - ROOM_RIGHT;
    const float bottom = ROOM_FRAME_H - ROOM_BOTTOM;
    return (Rectangle){ roomRect.x - left, roomRect.y - top,
                        roomRect.width + left + right, roomRect.height + top + bottom };
}

Vector2 WorldCameraClampTarget(Rectangle bounds, Vector2 focus, float viewW, float viewH)
{
    Vector2 out = focus;
    float halfW = viewW*0.5f;
    float halfH = viewH*0.5f;
    /* Se la vista e' piu' larga del limite, il minimo supera il massimo: si
       centra invece di clampare (GameMathClampFloat con min>max darebbe il
       minimo, cioe' un'inquadratura sbilenca). E' il caso 1x1 su entrambi gli
       assi, e quello di una 1x2 sull'asse corto. */
    if (bounds.width <= viewW) out.x = bounds.x + bounds.width*0.5f;
    else out.x = GameMathClampFloat(focus.x, bounds.x + halfW, bounds.x + bounds.width - halfW);
    if (bounds.height <= viewH) out.y = bounds.y + bounds.height*0.5f;
    else out.y = GameMathClampFloat(focus.y, bounds.y + halfH, bounds.y + bounds.height - halfH);
    return out;
}

Vector2 WorldCameraApproach(Vector2 current, Vector2 desired, float dt, float rate)
{
    if (!(dt > 0.0f) || !(rate > 0.0f)) return desired;
    float t = 1.0f - expf(-rate*dt);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Vector2 out;
    out.x = current.x + (desired.x - current.x)*t;
    out.y = current.y + (desired.y - current.y)*t;
    /* Aggancio finale: sotto il mezzo pixel l'inseguimento esponenziale non
       arriverebbe mai esattamente a destinazione, e una stanza 1x1 (dove il
       bersaglio e' costante) deve avere una telecamera ESATTAMENTE ferma, non
       "quasi ferma": il canvas e' campionato POINT, mezzo pixel di deriva si
       vedrebbe come tremolio. */
    if (fabsf(out.x - desired.x) < 0.5f) out.x = desired.x;
    if (fabsf(out.y - desired.y) < 0.5f) out.y = desired.y;
    return out;
}
