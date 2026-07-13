#ifndef MELTING_RUN_RUN_CONTENT_H
#define MELTING_RUN_RUN_CONTENT_H

#include "core/game_types.h"

void RunContentLoad(RunContent *content, unsigned int seed);

/* Fase 3b review ("lock the rarity enum/text sync"): "common"/"uncommon"/
 * "rare"/"legendary" -> Rarity, refuso o testo sconosciuto -> RARITY_COMMON.
 * Esposta (non piu' static) SOLO perche' src/tests/script_items_tests.c la
 * usa per un test di round-trip contro GEN_RARITIES (tools/melting-gen/
 * gen_util.c, sincronizzato a mano con questa funzione): vedi il commento
 * sulla sua definizione in run_content.c.
 */
Rarity RarityFromText(const char *text);

#endif
