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
