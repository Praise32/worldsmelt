#ifndef MELTING_RUN_ITEM_TRAITS_H
#define MELTING_RUN_ITEM_TRAITS_H

#include "core/game_types.h"

unsigned int ItemTraitsFromText(const char *text);
const char *ItemFirstTraitName(unsigned int traits);

/* M7 (substrato del catalogo, src/content/run_catalog.c): la direzione
 * OPPOSTA di ItemTraitsFromText sopra -- tutti i trait attivi, non solo il
 * primo, uniti da virgola nello stesso vocabolario ("bounce,giant"). Un
 * oggetto senza alcun trait scrive "plain" (stesso testo di riserva di
 * ItemFirstTraitName, cosi' un lettore futuro vede lo stesso vocabolario da
 * entrambe le funzioni). Round-trip garantito con ItemTraitsFromText (che
 * cerca ciascun nome con strstr, indipendente dall'ordine): utile perche' un
 * record di catalogo e' pensato per essere riletto da una futura riconvalida
 * (DEC-069), non solo scritto una volta e dimenticato. 'out' troncato in
 * sicurezza su 'outSize', mai un overflow anche con tutti e nove i trait
 * attivi insieme. */
void ItemTraitsToText(unsigned int traits, char *out, int outSize);

/* Fase 3b (design doc, docs/engineering/specs/2026-07-13-pools-rarity-design.md,
   sezione 4): "il costo del negozio scala con la rarita'". Usata da
   src/world/world.c quando piazza l'oggetto attivo del negozio (l'unico
   pool a pagamento: il tesoro costa una chiave, non monete). */
int ItemShopCostForRarity(Rarity rarity);

#endif
