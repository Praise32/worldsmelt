#ifndef MELTING_RUN_ROOM_CAMERA_H
#define MELTING_RUN_ROOM_CAMERA_H

#include "core/game_types.h"

/* Telecamera delle stanze (DEC-170, docs/design/systems/rooms-and-floor-generation.md,
 * sezione "Taglie multiple e telecamera").
 *
 * QUATTRO RIGHE DI PRINCIPIO, perche' e' tutto qui:
 *   1. lo ZOOM E' FISSO, sempre 1. Una cella (ROOM_W x ROOM_H) piu' la sua
 *      cornice di muro occupa ROOM_FRAME_W x ROOM_FRAME_H: nessuno zoom
 *      dinamico che si adatti alla taglia della stanza o all'azione;
 *   2. la stanza 1x1 aveva una sola inquadratura possibile finche' il canvas
 *      era grande quanto la cella inquadrata (960x640): il clamp sotto la
 *      determinava da solo, senza un caso speciale scritto a mano. DEC-200
 *      porta il canvas a 640x360 e quella coincidenza cade -- la vista e' ora
 *      piu' piccola della cella, quindi anche una 1x1 scorre, con lo STESSO
 *      clamp e senza righe in piu'. Se la si rivuole a inquadratura fissa la
 *      leva e' la taglia della cella (ROOM_W/ROOM_H), non questo file: e' una
 *      decisione di design, vedi governance/open-questions.md;
 *   3. la telecamera non mostra MAI area fuori dalla stanza: si clampa il
 *      centro dell'inquadratura, non la si lascia libera e poi si nasconde il
 *      fuori;
 *   4. queste funzioni sono PURE (nessun Game, nessuna raylib oltre ai tipi):
 *      il clamp e' esattamente cio' che un test puo' verificare senza aprire
 *      una finestra -- vedi --rooms-test in src/tests/game_tests.c.
 *
 * Il "rettangolo di clamp" NON e' il rettangolo di gioco della stanza: e'
 * quello allargato della cornice (WorldCameraBoundsFromRoom), cioe' lo spazio
 * che si mostrava gia' attorno alla stanza prima di DEC-170 (i muri, la fascia
 * dell'HUD in alto). Senza quell'allargamento una stanza 1x1 non mostrerebbe
 * piu' i propri muri ne' le proprie porte. */

/* Allarga il rettangolo di gioco (l'INTERA stanza, DEC-180: anche per le forme
   a L e' sempre il riquadro dell'intero blocco 2x2, mai una singola cella)
   della cornice fissa della cella: a sinistra/destra ROOM_X, in alto ROOM_Y,
   in basso quel che resta. Per una cella singola il risultato e' esattamente
   (0, 0, ROOM_FRAME_W, ROOM_FRAME_H). */
Rectangle WorldCameraBoundsFromRoom(Rectangle roomRect);

/* Il punto del mondo da mettere al centro del canvas per inquadrare 'focus'
   (di norma il giocatore) senza uscire da 'bounds'. Se 'bounds' e' piu' piccolo
   della vista su un asse, su quell'asse si centra (e' il caso 1x1: una sola
   inquadratura possibile, la telecamera resta ferma). */
Vector2 WorldCameraClampTarget(Rectangle bounds, Vector2 focus, float viewW, float viewH);

/* Avvicinamento esponenziale, indipendente dal passo: e' cio' che rende
   "morbido" l'inseguimento del giocatore in tutte le taglie maggiori (DEC-180:
   L compresa, che ora segue in continuo sul riquadro intero, senza piu' salti
   di cella). */
Vector2 WorldCameraApproach(Vector2 current, Vector2 desired, float dt, float rate);

/* Velocita' di avvicinamento (1/s). 12 = ~0.25s per coprire il 95% di uno
   scarto: abbastanza morbido da leggersi come movimento, abbastanza rapido da
   non far uscire il giocatore dall'inquadratura mentre corre. */
#define WORLD_CAMERA_RATE 12.0f

#endif
