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

/* Step B2 (generazione pigra dei piani, roadmap punto 2): ricarica dal manifest
 * SOLO gli script Lua del piano dato (indice 0..FLOOR_COUNT-1), lasciando intatto
 * tutto il resto del contenuto gia' caricato.
 *
 * Serve perche' con la generazione pigra il gioco parte quando e' pronto il solo
 * piano 1, e un secondo processo melting-gen scrive gli script dei piani 2-5 in
 * sottofondo mentre si gioca, ripubblicando il manifest dopo ogni piano. Il gioco
 * chiama questa funzione quando ENTRA in un piano (WorldStartFloor): se nel
 * frattempo gli script di quel piano sono arrivati, li raccoglie; se non sono
 * ancora pronti, non cambia nulla e gli oggetti di quel piano restano sulla
 * mini-VM, esattamente come degradano oggi.
 *
 * NON e' RunContentLoad: quella ricostruirebbe l'intero RunContent (temi, oggetti,
 * colori) ripartendo dal contenuto procedurale e da un seed -- a meta' partita
 * sarebbe un disastro. Questa tocca UNA cosa sola: la sorgente Lua degli oggetti
 * di quel piano. Nessun RNG, nessuna riallocazione, nessun altro campo. */
void RunContentRefreshFloorScripts(RunContent *content, int floorIndex);

/* M5 (DEC-005), requisito 8: carte-proposta di tema deterministiche sul seed,
 * usate SOLO quando la generazione e' disabilitata o bin/melting-gen non c'e'
 * (DEC-002 -- il gioco resta sempre avviabile senza il tool): niente
 * processo, carte pronte nello stesso frame in cui si entra nel Piano 0.
 * 'out' deve avere almeno 'count' slot (1..THEME_CARD_MAX, clampato dentro).
 * Vedi AppUseFallbackThemeCards in src/app/app.c per il chiamante. */
void RunContentMakeFallbackThemeCards(unsigned int seed, ThemeCard *out, int count);

#endif
