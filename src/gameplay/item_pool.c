#include "gameplay/item_pool.h"

#include "core/game_math.h"

#include <math.h>

const int ItemPoolWeightsStandard[ITEM_POOL_RARITY_COUNT] = { 55, 30, 12, 3 };
const int ItemPoolWeightsBoss[ITEM_POOL_RARITY_COUNT]     = {  0,  0, 70, 30 };

void ItemPoolMinimumCounts(int poolSize, const int weights[ITEM_POOL_RARITY_COUNT], int outCounts[ITEM_POOL_RARITY_COUNT])
{
    if (!weights || !outCounts) return;
    if (poolSize < 0) poolSize = 0;

    int total = 0;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++) if (weights[r] > 0) total += weights[r];

    int counts[ITEM_POOL_RARITY_COUNT] = { 0, 0, 0, 0 };
    if (total <= 0)
    {
        /* Tabella senza alcun peso utile: nessuna garanzia ha senso, tutto
           il pool ricade su RARITY_COMMON (ripiego sicuro, come
           ItemPoolRollRarity sotto). */
        counts[RARITY_COMMON] = poolSize;
        for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++) outCounts[r] = counts[r];
        return;
    }

    int sum = 0;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
    {
        if (weights[r] > 0) counts[r] = (poolSize * weights[r]) / total;   /* arrotondamento per difetto */
        sum += counts[r];
    }
    int leftover = poolSize - sum;   /* residuo dell'arrotondamento, DEC-144 */

    /* Garanzia: ogni rarita' a peso > 0 compare almeno una volta. Il
       residuo si assegna prima alle rarita' PIU' RARE (si scende da
       leggendaria verso comune): sono quelle che l'arrotondamento per
       difetto svuota per prime, ed e' li' che la garanzia serve davvero. */
    for (int r = ITEM_POOL_RARITY_COUNT - 1; r >= 0 && leftover > 0; r--)
    {
        if (weights[r] > 0 && counts[r] == 0)
        {
            counts[r] = 1;
            leftover--;
        }
    }
    /* Se il residuo non bastava, il deficit si toglie alle rarita' PIU'
       COMUNI ancora capienti (si parte dalla comune, poi si risale):
       l'eccedenza non si aggiunge mai al totale (DEC-087, pool sempre di
       'poolSize' oggetti). */
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
    {
        if (weights[r] > 0 && counts[r] == 0)
        {
            int donor = 0;
            while (donor < ITEM_POOL_RARITY_COUNT && (donor == r || counts[donor] <= 0)) donor++;
            if (donor < ITEM_POOL_RARITY_COUNT)
            {
                counts[donor]--;
                counts[r] = 1;
            }
            /* Nessun donatore capiente (pool degenere, es. poolSize troppo
               piccolo per il numero di rarita' a peso > 0): la rarita'
               resta a 0, e' il ripiego meno peggio possibile senza
               inventare un oggetto in piu' rispetto a 'poolSize'. */
        }
    }
    /* Residuo ancora positivo (poolSize non perfettamente proporzionale ai
       pesi, garanzie gia' tutte soddisfatte): va alla rarita' PIU' COMUNE
       FRA QUELLE A PESO > 0 -- MAI a una rarita' a peso 0 (sul pool boss
       comune/non-comune hanno peso 0 per costruzione, non devono comparire
       per un residuo di arrotondamento). E' la fascia che assorbe
       l'aggiustamento senza alterare la gerarchia percepita, stesso
       principio della sottrazione sopra. */
    if (leftover > 0)
    {
        int r = 0;
        while (r < ITEM_POOL_RARITY_COUNT - 1 && weights[r] <= 0) r++;
        counts[r] += leftover;
    }

    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++) outCounts[r] = counts[r];
}

Rarity ItemPoolRollRarity(unsigned int *rng, const int weights[ITEM_POOL_RARITY_COUNT])
{
    int total = 0;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++) if (weights[r] > 0) total += weights[r];
    if (total <= 0) return RARITY_COMMON;

    int roll = GameRngRange(rng, 0, total - 1);
    int acc = 0;
    for (int r = 0; r < ITEM_POOL_RARITY_COUNT; r++)
    {
        if (weights[r] <= 0) continue;
        acc += weights[r];
        if (roll < acc) return (Rarity)r;
    }
    return RARITY_COMMON;   /* mai raggiunto se i pesi sono coerenti col totale, ripiego difensivo */
}

int ItemPoolLuckThreshold(float luck)
{
    int reduction = (int)floorf(luck / ITEM_POOL_LUCK_DIVISOR);
    int n = ITEM_POOL_LUCK_THRESHOLD_BASE - reduction;
    if (n < 1) n = 1;
    if (n > ITEM_POOL_LUCK_THRESHOLD_BASE) n = ITEM_POOL_LUCK_THRESHOLD_BASE;
    return n;
}

bool ItemPoolLuckCorrectionActive(int streak, float luck)
{
    return streak >= ItemPoolLuckThreshold(luck);
}

/* Conta gli ammessi secondo 'restrictToNonCommon' (correzione di fortuna) e
   'applyWeightFilter' (esclude i candidati la cui rarita' ha peso <= 0 nella
   tabella, difesa contro un pool malformato -- MAI una vera pesatura, vedi
   il commento sopra ItemPoolDrawIndex in item_pool.h). */
static int ItemPoolCountAdmitted(const Rarity *rarities, int count, const int weights[ITEM_POOL_RARITY_COUNT],
                                  bool restrictToNonCommon, bool applyWeightFilter)
{
    int admitted = 0;
    for (int i = 0; i < count; i++)
    {
        if (restrictToNonCommon && rarities[i] == RARITY_COMMON) continue;
        if (applyWeightFilter && weights && weights[rarities[i]] <= 0) continue;
        admitted++;
    }
    return admitted;
}

int ItemPoolDrawIndex(unsigned int *rng, const Rarity *rarities, int count,
                       const int weights[ITEM_POOL_RARITY_COUNT], float luck, int *streak)
{
    if (count <= 0 || !rarities || !rng) return -1;

    int fallbackStreak = 0;
    if (!streak) streak = &fallbackStreak;   /* chiamante senza stato di correzione: non deterministico fra chiamate, ma non crasha */

    bool corrected = ItemPoolLuckCorrectionActive(*streak, luck);
    bool hasNonCommon = false;
    if (corrected)
    {
        for (int i = 0; i < count; i++) if (rarities[i] != RARITY_COMMON) { hasNonCommon = true; break; }
    }
    bool restrictToNonCommon = corrected && hasNonCommon;

    /* Scelta UNIFORME fra i candidati ammessi -- NON piu' pesata per rarita'
       (vedi il commento sopra la dichiarazione in item_pool.h: i candidati
       arrivano GIA' pesati secondo DEC-019 da chi li ha generati -- melting-
       gen per una run vera, ItemPoolMinimumCounts per il ripiego -- pesare
       di nuovo qui applicherebbe la tabella una SECONDA volta, elevando la
       distribuzione al quadrato invece che rispettarla. Una scelta uniforme
       fra k campioni gia' distribuiti secondo i pesi standard riproduce
       ESATTAMENTE quei pesi sul candidato estratto, qualunque sia k: e' un
       fatto di probabilita' (media della funzione indicatrice sui k
       campioni), verificato empiricamente su 2.000.000 di estrazioni prima
       di questa correzione). 'weights' resta come parametro solo per il
       filtro di ammissione difensivo sotto (mai per pesare la scelta): un
       candidato la cui rarita' ha peso <= 0 nella tabella passata non e'
       mai scelto, a meno che escluderlo lasci l'insieme vuoto. */
    bool applyWeightFilter = true;
    int admitted = ItemPoolCountAdmitted(rarities, count, weights, restrictToNonCommon, applyWeightFilter);
    if (admitted <= 0)
    {
        /* Filtro dei pesi troppo aggressivo (tabella malformata: nessun
           candidato ammesso ha peso > 0): lo si abbandona, resta solo la
           restrizione della correzione di fortuna. */
        applyWeightFilter = false;
        admitted = ItemPoolCountAdmitted(rarities, count, weights, restrictToNonCommon, applyWeightFilter);
    }
    if (admitted <= 0)
    {
        /* Ancora vuoto: non dovrebbe accadere (hasNonCommon sopra garantisce
           un candidato non-comune quando restrictToNonCommon e' vero), ma un
           ripiego totale -- nessuna restrizione -- evita comunque un indice
           fuori banda invece di crashare. */
        restrictToNonCommon = false;
        admitted = count;
    }

    int ordinal = GameRngRange(rng, 0, admitted - 1);
    int picked = count - 1;
    for (int i = 0; i < count; i++)
    {
        if (restrictToNonCommon && rarities[i] == RARITY_COMMON) continue;
        if (applyWeightFilter && weights && weights[rarities[i]] <= 0) continue;
        if (ordinal == 0) { picked = i; break; }
        ordinal--;
    }

    if (rarities[picked] == RARITY_COMMON) (*streak)++;
    else *streak = 0;

    return picked;
}
