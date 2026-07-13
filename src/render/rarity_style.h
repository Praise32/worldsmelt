#ifndef MELTING_RUN_RARITY_STYLE_H
#define MELTING_RUN_RARITY_STYLE_H

#include "core/game_types.h"

/* Fase 3b VISIVA (docs/superpowers/specs/2026-07-13-pools-rarity-design.md,
   sezioni 1 e 6): unica fonte di verita' per come la rarita' si vede sullo
   schermo -- colore "classico" e nome in italiano. Ogni punto del renderer
   che mostra la rarita' (bordo/glow del pickup in DrawPickup, bordo +
   etichetta del pannello in DrawItemPreview, entrambi in game_renderer.c)
   chiama SOLO queste due funzioni invece di reinventare la tavolozza:
   ribilanciare uno schema di colori o rinominare un livello significa
   toccare SOLO rarity_style.c. Coppia .h/.c a parte (invece di due funzioni
   private dentro game_renderer.c) perche' la rarita' e' un concetto di
   dominio a se' -- stessa idea di item_traits.{h,c} in src/gameplay -- non
   un dettaglio implementativo del renderer, e potrebbe servire domani anche
   fuori da game_renderer.c (es. DrawItemPreview in un secondo file, un
   tooltip, un log). */

/* Colore "classico" della rarita' (design doc, sezione 1): Comune
   bianco/grigio, Non-comune verde, Raro blu, Leggendario arancione/oro.
   Una Rarity fuori range (dato corrotto, mai dovrebbe succedere: l'enum ha
   solo 4 valori e RarityFromText in src/content/run_content.c ricade sempre
   su RARITY_COMMON per un testo sconosciuto) ricade sul colore di RARITY_COMMON,
   stessa difesa in profondita' di ScriptItemsRarityFraction
   (src/script/script_items.c) e ItemShopCostForRarity (src/gameplay/item_traits.c). */
Color RarityColor(Rarity rarity);

/* Nome della rarita' in italiano, per il pannello (design doc, sezione 6:
   "il pannello... mostra rarita' (nome + colore)"). Stessa difesa in
   profondita' di RarityColor sopra. */
const char *RarityName(Rarity rarity);

#endif
