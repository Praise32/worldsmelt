#ifndef MELTING_RUN_ITEM_TRAITS_H
#define MELTING_RUN_ITEM_TRAITS_H

#include "core/game_types.h"

unsigned int ItemTraitsFromText(const char *text);
const char *ItemFirstTraitName(unsigned int traits);

/* Fase 3b (design doc, docs/superpowers/specs/2026-07-13-pools-rarity-design.md,
   sezione 4): "il costo del negozio scala con la rarita'". Usata da
   src/world/world.c quando piazza l'oggetto attivo del negozio (l'unico
   pool a pagamento: il tesoro costa una chiave, non monete). */
int ItemShopCostForRarity(Rarity rarity);

#endif
