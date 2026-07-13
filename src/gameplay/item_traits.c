#include "gameplay/item_traits.h"

#include "core/game_types.h"

#include <string.h>

unsigned int ItemTraitsFromText(const char *text)
{
    unsigned int traits = 0;
    if (!text) return traits;

    if (strstr(text, "bounce")) traits |= TRAIT_BOUNCE;
    if (strstr(text, "homing")) traits |= TRAIT_HOMING;
    if (strstr(text, "explode")) traits |= TRAIT_EXPLODE;
    if (strstr(text, "split")) traits |= TRAIT_SPLIT;
    if (strstr(text, "pierce")) traits |= TRAIT_PIERCE;
    if (strstr(text, "rapid")) traits |= TRAIT_RAPID;
    if (strstr(text, "giant")) traits |= TRAIT_GIANT;
    if (strstr(text, "slow")) traits |= TRAIT_SLOW;
    if (strstr(text, "vamp")) traits |= TRAIT_VAMP;
    return traits;
}

const char *ItemFirstTraitName(unsigned int traits)
{
    if (traits & TRAIT_BOUNCE) return "bounce";
    if (traits & TRAIT_HOMING) return "homing";
    if (traits & TRAIT_EXPLODE) return "explode";
    if (traits & TRAIT_SPLIT) return "split";
    if (traits & TRAIT_PIERCE) return "pierce";
    if (traits & TRAIT_RAPID) return "rapid";
    if (traits & TRAIT_GIANT) return "giant";
    if (traits & TRAIT_SLOW) return "slow";
    if (traits & TRAIT_VAMP) return "vamp";
    return "plain";
}

/* Costo in monete dell'oggetto del negozio, per rarita' (fase 3b, design
   doc sezione 4: "il costo del negozio scala con la rarita': un
   leggendario costa piu' monete di un comune"). MODIFICA QUI per
   ribilanciare l'economia. Indicizzata come Rarity (core/game_types.h):
   COMUNE resta l'8 monete gia' in uso prima di questa fase (nessuna
   sorpresa per chi gia' gioca), le altre tre righe salgono abbastanza da
   rendere il negozio una scelta vera ("spendo tutto per l'oggetto forte, o
   prendo due cose economiche?", vedi il design doc). */
static const int ITEM_SHOP_COST_BY_RARITY[4] = {
    8,    /* RARITY_COMMON -- valore storico, invariato */
    16,
    28,
    45,   /* RARITY_LEGENDARY */
};

int ItemShopCostForRarity(Rarity rarity)
{
    /* Difesa in profondita' (dato corrotto -> il costo piu' basso, mai il
       piu' alto): stessa scelta di ScriptItemsRarityFraction in
       src/script/script_items.c. */
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) return ITEM_SHOP_COST_BY_RARITY[RARITY_COMMON];
    return ITEM_SHOP_COST_BY_RARITY[rarity];
}
