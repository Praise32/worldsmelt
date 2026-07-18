#ifndef MELTING_RUN_CHARACTER_ROSTER_H
#define MELTING_RUN_CHARACTER_ROSTER_H

#include "core/game_types.h"

/* M6a (DEC-030/033/049): la rosa base CURATA di CHARACTER_COUNT personaggi
 * (vedi il commento su CharacterDef in core/game_types.h). Ritorna sempre un
 * puntatore valido: un indice fuori range (difesa in profondita', mai
 * dovrebbe succedere: ogni chiamante clampa gia' i propri indici con un
 * modulo CHARACTER_COUNT) ricade sul personaggio 0 (Wayfinder, il
 * preselezionato di default), mai un crash o un puntatore nullo. */
const CharacterDef *CharacterRosterGet(int index);

#endif
