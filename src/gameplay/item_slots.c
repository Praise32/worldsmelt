#include "gameplay/item_slots.h"

#include "core/game_math.h"
#include "core/game_types.h"

#include <stddef.h>

/* Banda di sicurezza sulla capienza in cariche: un contenuto generato o un
   manifest scritto a mano non deve poter chiedere 0 cariche (attivo mai
   usabile) ne' centinaia (barra della ricarica illeggibile e un attivo che
   di fatto non si scarica mai). Stessa filosofia dei clamp di
   script_items.c: larghi ma finiti. */
#define ITEM_ACTIVE_CHARGES_MIN 1
#define ITEM_ACTIVE_CHARGES_MAX 12

/* Idem per il cooldown: sotto mezzo secondo un "attivo" e' un secondo tasto
   di fuoco, sopra il minuto e mezzo e' un oggetto che in una run non si
   riusa mai. */
#define ITEM_ACTIVE_COOLDOWN_MIN 0.5f
#define ITEM_ACTIVE_COOLDOWN_MAX 90.0f

int ItemActiveSlotCount(const Player *p)
{
    if (!p) return 1;
    if (p->activeSlotCount <= 0) return 1;
    return (p->activeSlotCount > MAX_ACTIVE_SLOTS) ? MAX_ACTIVE_SLOTS : p->activeSlotCount;
}

int ItemGraftSlotCount(const Player *p)
{
    if (!p) return 1;
    if (p->graftSlotCount <= 0) return 1;
    return (p->graftSlotCount > MAX_GRAFT_SLOTS) ? MAX_GRAFT_SLOTS : p->graftSlotCount;
}

/* itemCount e' clampato qui e non solo dai chiamanti: questo modulo lo
   leggono anche il renderer e i test, che possono ricevere un Game
   costruito a mano. */
static int ItemSlotsUsableCount(const Player *p)
{
    if (!p) return 0;
    if (p->itemCount < 0) return 0;
    return (p->itemCount > MAX_ITEMS) ? MAX_ITEMS : p->itemCount;
}

int ItemCountOfKind(const Player *p, ItemKind kind)
{
    int count = 0;
    int n = ItemSlotsUsableCount(p);
    for (int i = 0; i < n; i++) if (p->items[i].kind == kind) count++;
    return count;
}

int ItemIndexOfKind(const Player *p, ItemKind kind, int n)
{
    if (n < 0) return -1;
    int seen = 0;
    int count = ItemSlotsUsableCount(p);
    for (int i = 0; i < count; i++)
    {
        if (p->items[i].kind != kind) continue;
        if (seen == n) return i;
        seen++;
    }
    return -1;
}

int ItemSelectedActiveIndex(const Player *p)
{
    int owned = ItemCountOfKind(p, ITEM_ACTIVE);
    if (owned <= 0) return -1;
    int ordinal = p->activeSelected;
    if (ordinal < 0) ordinal = 0;
    if (ordinal >= owned) ordinal = owned - 1;
    return ItemIndexOfKind(p, ITEM_ACTIVE, ordinal);
}

bool ItemActiveIsChargeBased(const Item *item)
{
    return item != NULL && item->kind == ITEM_ACTIVE && item->charges > 0;
}

bool ItemActiveIsCooldownBased(const Item *item)
{
    if (item == NULL || item->kind != ITEM_ACTIVE) return false;
    /* Le cariche vincono (vedi il commento in item_slots.h): un oggetto che
       dichiara entrambi non e' anche a cooldown. */
    return item->charges <= 0;
}

int ItemActiveChargeCapacity(const Item *item)
{
    if (!ItemActiveIsChargeBased(item)) return 0;
    return GameMathClampInt(item->charges, ITEM_ACTIVE_CHARGES_MIN, ITEM_ACTIVE_CHARGES_MAX);
}

float ItemActiveCooldownSeconds(const Item *item)
{
    if (!ItemActiveIsCooldownBased(item)) return 0.0f;
    if (item->cooldown <= 0.0f) return ITEM_ACTIVE_DEFAULT_COOLDOWN;
    return GameMathClampFloat(item->cooldown, ITEM_ACTIVE_COOLDOWN_MIN, ITEM_ACTIVE_COOLDOWN_MAX);
}

bool ItemActiveIsReady(const Item *item)
{
    if (item == NULL || item->kind != ITEM_ACTIVE) return false;
    if (ItemActiveIsChargeBased(item)) return item->chargeNow >= 1;
    return item->cooldownTimer <= 0.0f;
}

void ItemActiveResetCharge(Item *item)
{
    if (item == NULL || item->kind != ITEM_ACTIVE) return;
    item->chargeNow = ItemActiveChargeCapacity(item);
    item->cooldownTimer = 0.0f;
}

/* I due canali di DEC-059 differiscono solo per il campo di dosaggio: la
   meccanica e' la stessa, quindi lo e' anche il codice. Un dosaggio non
   dichiarato (0, cioe' lo zero-default di ogni oggetto scritto prima di
   questa fase) vale 1: un attivo a cariche deve poter ricaricare comunque,
   altrimenti "cariche" significherebbe "usi totali per run". */
static int ItemActivesGain(Player *p, bool fromRoom)
{
    int touched = 0;
    int count = ItemSlotsUsableCount(p);
    for (int i = 0; i < count; i++)
    {
        Item *item = &p->items[i];
        if (!ItemActiveIsChargeBased(item)) continue;
        int cap = ItemActiveChargeCapacity(item);
        if (item->chargeNow >= cap) continue;
        int gain = fromRoom ? item->chargeGainRoom : item->chargeGainEnergy;
        if (gain <= 0) gain = 1;
        item->chargeNow = GameMathClampInt(item->chargeNow + gain, 0, cap);
        touched++;
    }
    return touched;
}

int ItemActivesGainRoomCharge(Player *p) { return ItemActivesGain(p, true); }
int ItemActivesGainEnergyCharge(Player *p) { return ItemActivesGain(p, false); }

bool ItemActivesWantEnergy(const Player *p)
{
    int count = ItemSlotsUsableCount(p);
    for (int i = 0; i < count; i++)
    {
        const Item *item = &p->items[i];
        if (!ItemActiveIsChargeBased(item)) continue;
        if (item->chargeNow < ItemActiveChargeCapacity(item)) return true;
    }
    return false;
}

void ItemActivesTickCooldown(Player *p, float dt)
{
    int count = ItemSlotsUsableCount(p);
    for (int i = 0; i < count; i++)
    {
        Item *item = &p->items[i];
        if (!ItemActiveIsCooldownBased(item)) continue;
        if (item->cooldownTimer <= 0.0f) { item->cooldownTimer = 0.0f; continue; }
        item->cooldownTimer -= dt;
        if (item->cooldownTimer < 0.0f) item->cooldownTimer = 0.0f;
    }
}

const char *ItemKindLabel(ItemKind kind)
{
    switch (kind)
    {
        case ITEM_STATUP: return "STAT-UP";
        case ITEM_ACTIVE: return "ATTIVO";
        case ITEM_GRAFT: return "INNESTO";
        case ITEM_PASSIVE: default: return "PASSIVO";
    }
}
