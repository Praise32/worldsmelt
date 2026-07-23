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

#endif
