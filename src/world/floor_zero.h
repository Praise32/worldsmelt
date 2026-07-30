#ifndef MELTING_RUN_FLOOR_ZERO_H
#define MELTING_RUN_FLOOR_ZERO_H

#include "core/game_types.h"

/* Piano 0 (M1b, docs/design/systems/floor-zero.md):
   la sala d'attesa GIOCABILE, non piu' un overlay bloccante (M1a). Prepara
   'game' per una stanza hub statica e curata -- una sola stanza esistente,
   kind ROOM_HUB, nessun nemico/pickup/ostacolo distruttibile -- SENZA MAI
   leggere generated/ (a differenza di GameResetRun/RunContentLoad): l'atlas
   e i contenuti gia' caricati (dell'ultima run vera, o quelli di riserva del
   primissimo avvio) restano quelli usati anche qui, cosi' il Piano 0 e'
   sempre disponibile per costruzione, senza dipendere da cio' che
   melting-gen/melting-sprites stanno ancora scrivendo (DEC-002). Il
   chiamante (src/app/app.c, AppEnterFloorZero) resta l'unico proprietario di
   QUANDO entrare/uscire da questo stato e di quando aprire l'uscita
   (Game.floorZeroExitOpen): questa funzione si limita a preparare il Game
   per l'ingresso. */
void FloorZeroEnter(Game *game);

/* L'arredo curato e STABILE del crogiolo (seme FISSO, mai game->rng: vedi il
   commento sulla definizione in floor_zero.c). Pubblica dal WP15a perche' la
   ricostruisce anche l'uscita da una simulazione d'arena
   (FloorZeroArenaExit, src/world/floor_zero_arena.c), che svuota la stanza per
   fare spazio all'ondata e deve poi rimetterla esattamente com'era --
   deterministica, quindi "com'era" e "ricostruita da zero" sono lo stesso
   arredo, senza doverlo copiare in uno snapshot. */
void FloorZeroBuildDressing(Game *game);

#endif
