#include "gameplay/synergies.h"

#include "core/game_math.h"

#include <math.h>

/* Vedi synergies.h per il disegno complessivo (i due canali, perche' e' poco
   codice, perche' non c'e' Item.archetype). Qui c'e' la TAVOLA. */

/* Un "segnale" e' cio' su cui una regola di sinergia condiziona. Sono tutti dati
   REALI, scritti dal modello e persistiti nel manifest: i trait dell'oggetto
   (segnale primario, design doc sezione 4.1) e le proprieta' del tipo di colpo
   attivo (step C). Nessun segnale derivato o indovinato. */
typedef enum SynergySignal {
    SIG_TRAIT_BOUNCE,
    SIG_TRAIT_HOMING,
    SIG_TRAIT_EXPLODE,
    SIG_TRAIT_SPLIT,
    SIG_TRAIT_PIERCE,
    SIG_TRAIT_RAPID,
    SIG_TRAIT_GIANT,
    SIG_TRAIT_SLOW,
    SIG_TRAIT_VAMP,
    SIG_SHOT_CHAIN,    /* il tipo di colpo ATTIVO salta di nemico in nemico */
    SIG_SHOT_PIERCE    /* il tipo di colpo ATTIVO perfora */
} SynergySignal;

/* Una regola = due segnali + cosa aggiunge, sui due canali. I campi del canale A
   sono moltiplicatori (1.0 = niente) e un additivo (0 = niente); quelli del
   canale B sono bit e piccoli interi. Una riga puo' usare entrambi i canali (e'
   il caso del rimbalzo instabile), ma resta UNA cosa comprensibile: "la coppia
   aggiunge una cosa leggibile, non un fuoco d'artificio" (design doc, 4.5). */
typedef struct SynergyRule {
    const char *name;
    const char *description;
    SynergySignal a;
    SynergySignal b;
    /* Canale A */
    float damageMul;
    float fireDelayMul;
    float shotSpeedMul;
    float luckAdd;
    /* Canale B */
    unsigned int grantTraits;
    int pierceBonus;
    int bounceBonus;
    int chainBonus;
    int pelletBonus;
} SynergyRule;

/* MODIFICA QUI per aggiungere/ribilanciare una sinergia: e' una riga, e va tenuta
   allineata all'enum SynergyId in synergies.h (stesso ordine).
   Le coppie sono i pattern generici del design doc, sezione 5, con nomi NOSTRI
   (mai i nomi di Isaac: vedi "Confini di IP", sezione 7). */
static const SynergyRule SYNERGY_RULES[SYNERGY_COUNT] = {
    /* Inseguimento + perforazione: il colpo curva verso il nemico E lo trapassa,
       continuando verso il prossimo. I due trait, da soli, gia' si sommano sul
       colpo (e' l'OR di bit, gratis): la SINERGIA e' che la perforazione diventa
       profonda -- non attraversi un nemico, attraversi la fila. */
    {
        "Volo Infilzante", "i colpi curvano e attraversano la fila",
        SIG_TRAIT_HOMING, SIG_TRAIT_PIERCE,
        1.0f, 1.0f, 1.0f, 0.0f,
        0u, 2, 0, 0, 0
    },
    /* Rimbalzo + esplosione: rimbalza piu' a lungo e ogni impatto e' piu' cattivo. */
    {
        "Rimbalzo Instabile", "rimbalzi piu' lunghi, impatti piu' cattivi",
        SIG_TRAIT_BOUNCE, SIG_TRAIT_EXPLODE,
        1.10f, 1.0f, 1.0f, 0.0f,
        0u, 0, 2, 0, 0
    },
    /* Divisione + cadenza: un pallettone in piu' a ogni sparo. */
    {
        "Sciame", "un colpo in piu' a ogni sparo",
        SIG_TRAIT_SPLIT, SIG_TRAIT_RAPID,
        1.0f, 1.0f, 1.0f, 0.0f,
        0u, 0, 0, 0, 1
    },
    /* Rallentamento + cadenza: tanti colpi deboli che tengono i nemici sempre
       lenti. Premiata sul canale statistico (il comportamento c'e' gia'). */
    {
        "Gelo Perpetuo", "tanti colpi, nemici sempre lenti",
        SIG_TRAIT_SLOW, SIG_TRAIT_RAPID,
        1.12f, 0.95f, 1.0f, 0.0f,
        0u, 0, 0, 0, 0
    },
    /* Furto di vita + colpi giganti: colpi enormi che rubano vita piu' spesso (la
       fortuna alza proprio la probabilita' di VAMP, vedi CombatDamageEnemy). */
    {
        "Morso Vorace", "colpi enormi che rubano vita piu' spesso",
        SIG_TRAIT_VAMP, SIG_TRAIT_GIANT,
        1.10f, 1.0f, 1.0f, 2.0f,
        0u, 0, 0, 0, 0
    },
    /* Il tipo di colpo del piano SALTA (step C: chain > 0) e hai un oggetto che
       rallenta: la scarica salta a un nemico in piu' e li lascia tutti lenti.
       E' la prova che le sinergie non sono solo trait+trait: il contenuto nuovo
       inventato dal modello (il tipo di colpo) partecipa alle coppie. */
    {
        "Arco Voltaico", "la scarica salta a un nemico in piu'",
        SIG_SHOT_CHAIN, SIG_TRAIT_SLOW,
        1.0f, 1.0f, 1.0f, 0.0f,
        TRAIT_SLOW, 0, 0, 1, 0
    },
};

/* DEC-161: vedi il commento completo in synergies.h. splitmix64 (Steele/Lea/
   Flood) con una costante di dominio propria ('SYNCONF'), diversa da quella di
   GameplayRngSeedFromRunSeed (src/game/game.c, 'GMPLAY') e da quella (se mai
   ce ne sara' una) di un futuro terzo consumatore del seed di run: stream
   diversi non devono mai correlare per costruzione. Le chiavi si ordinano
   PRIMA di entrare nel mix (min/max), cosi' la chiave di coppia e' la stessa
   indipendentemente da quale dei due venga passato come 'keyA': cio' che
   distingue "A prevale" da "B prevale" e' SOLO il confronto finale fra il
   vincitore mescolato e l'argomento letterale 'keyA'. */
bool SynergyConflictAPrevails(unsigned int runSeed, int keyA, int keyB)
{
    int lo = (keyA < keyB) ? keyA : keyB;
    int hi = (keyA < keyB) ? keyB : keyA;
    const unsigned long long domain = 0x53594E434F4E4601ULL;   /* 'SYNCONF' + costante di dominio */
    unsigned long long state = ((unsigned long long)runSeed ^ domain)
        + ((unsigned long long)(unsigned int)lo << 32) + (unsigned long long)(unsigned int)hi
        + 0x9E3779B97F4A7C15ULL;
    unsigned long long z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= (z >> 31);
    bool loWins = (z & 1u) == 0u;   /* un solo bit basta: e' una scelta binaria fra due candidati */
    int winner = loWins ? lo : hi;
    return winner == keyA;
}

static unsigned int SignalTraitMask(SynergySignal sig)
{
    switch (sig)
    {
        case SIG_TRAIT_BOUNCE:  return TRAIT_BOUNCE;
        case SIG_TRAIT_HOMING:  return TRAIT_HOMING;
        case SIG_TRAIT_EXPLODE: return TRAIT_EXPLODE;
        case SIG_TRAIT_SPLIT:   return TRAIT_SPLIT;
        case SIG_TRAIT_PIERCE:  return TRAIT_PIERCE;
        case SIG_TRAIT_RAPID:   return TRAIT_RAPID;
        case SIG_TRAIT_GIANT:   return TRAIT_GIANT;
        case SIG_TRAIT_SLOW:    return TRAIT_SLOW;
        case SIG_TRAIT_VAMP:    return TRAIT_VAMP;
        default:                return 0u;   /* i segnali del tipo di colpo non sono trait */
    }
}

/* Cerca UN oggetto che porti il segnale, saltando 'excludeItem' (l'oggetto gia'
   usato per l'altro meta' della coppia: una sinergia e' fra DUE oggetti diversi,
   non un oggetto che sinergizza con se' stesso). Scrive in *outItem l'indice
   trovato (-1 per un segnale del tipo di colpo, che non appartiene a un oggetto
   specifico dell'inventario) e in *outRarity la rarita' da usare per scalare la
   potenza. Ritorna false se il segnale non c'e'.
   DEC-161: se PIU' di un oggetto posseduto porta lo stesso segnale, nessuna
   regola di design dice quale dei due "conta" per la coppia -- fin qui il
   codice prendeva semplicemente il PRIMO trovato nell'ordine dell'inventario,
   una priorita' di fatto (l'ordine di raccolta) mai dichiarata da nessun
   documento. Sostituita da un mini-torneo: ogni candidato successivo sfida il
   vincitore corrente con SynergyConflictAPrevails(runSeed, ...), chiave =
   (segnale, indice) di ciascuno. Con UN solo candidato il torneo non gira
   nemmeno (nessun cambio di comportamento sul caso comune); con piu' di uno la
   scelta e' deterministica per (runSeed, segnale, composizione della build) --
   stessa run, stessa build -> stesso vincitore ogni volta (idempotenza, Test Y
   in script_items_tests.c) -- e diversa in run con seed diverso. Non cambia
   MAI se la sinergia si forma (basta un candidato qualunque), solo quale
   oggetto ne detta la rarita'. */
static bool SignalPresent(const Player *player, SynergySignal sig, int excludeItem, unsigned int runSeed, int *outItem, Rarity *outRarity)
{
    if (sig == SIG_SHOT_CHAIN || sig == SIG_SHOT_PIERCE)
    {
        if (!player->shotType.active) return false;
        bool present = (sig == SIG_SHOT_CHAIN) ? (player->shotType.chain > 0) : (player->shotType.pierceBonus > 0);
        if (!present) return false;
        /* Il tipo di colpo APPARTIENE all'oggetto che l'ha conferito
           (player->shotTypeItem, ricalcolato in ScriptItemsRecomputeStats), non
           "a nessuno" (correzione da review): se lo si trattasse come un segnale
           senza padrone, l'oggetto che porta il tipo di colpo potrebbe fare da
           ENTRAMBE le meta' della coppia -- un solo oggetto che rallenta e porta
           un tipo di colpo che salta accenderebbe "Arco Voltaico" da solo, contro
           la regola base di questo modulo (una sinergia e' fra DUE oggetti). Con
           il padrone dichiarato, excludeItem lo esclude come qualunque altro. */
        if (player->shotTypeItem < 0 || player->shotTypeItem >= player->itemCount) return false;
        if (player->shotTypeItem == excludeItem) return false;
        *outItem = player->shotTypeItem;
        /* La rarita' della coppia e' quella dell'OGGETTO che porta il tipo di
           colpo, come per ogni altro segnale: un tipo di colpo su un oggetto
           leggendario vale piu' dello stesso tipo su un comune. */
        *outRarity = player->items[player->shotTypeItem].rarity;
        return true;
    }

    unsigned int mask = SignalTraitMask(sig);
    if (mask == 0u) return false;
    int candidates[MAX_ITEMS];
    int candidateCount = 0;
    for (int i = 0; i < player->itemCount && i < MAX_ITEMS; i++)
    {
        if (i == excludeItem) continue;
        /* Solo gli oggetti ATTIVI partecipano alle coppie: uno stat-up e' solo
           numeri, non ha un comportamento con cui sinergizzare (design doc,
           sezione 4.1, riga "Tipo"). I suoi trait esistono solo come etichetta
           per il ripiego C (vedi ScriptItemsApplyStatUpFallback). */
        const Item *item = &player->items[i];
        if (item->kind == ITEM_STATUP) continue;
        if (item->traits & mask) candidates[candidateCount++] = i;
    }
    if (candidateCount == 0) return false;

    int chosen = candidates[0];
    for (int k = 1; k < candidateCount; k++)
    {
        /* Chiave di coppia = (segnale, indice): il segnale distingue il
           torneo di un trait da quello di un altro (due tornei diversi non
           devono mai condividere lo stesso esito solo perche' due indici
           coincidono), l'indice distingue i candidati fra loro DENTRO lo
           stesso torneo. */
        int keyChallenger = (int)sig*1000 + candidates[k];
        int keyChampion = (int)sig*1000 + chosen;
        if (SynergyConflictAPrevails(runSeed, keyChallenger, keyChampion)) chosen = candidates[k];
    }
    *outItem = chosen;
    *outRarity = player->items[chosen].rarity;
    return true;
}

/* Una regola e' attiva se entrambi i segnali sono presenti su DUE oggetti
   diversi. Si prova in entrambi gli ordini: se il segnale A e il segnale B
   trovassero per primo lo STESSO oggetto (uno che porta entrambi i trait),
   provare solo "A poi B-escluso-A" fallirebbe anche quando esiste un secondo
   oggetto valido -- che sarebbe un falso negativo dipendente dall'ordine
   dell'inventario. */
static bool RuleActive(const Player *player, const SynergyRule *rule, unsigned int runSeed, Rarity *outMinRarity)
{
    int itemA = -2, itemB = -2;
    Rarity rarityA = RARITY_COMMON, rarityB = RARITY_COMMON;

    if (SignalPresent(player, rule->a, -2, runSeed, &itemA, &rarityA) &&
        SignalPresent(player, rule->b, itemA, runSeed, &itemB, &rarityB))
    {
        *outMinRarity = (rarityA < rarityB) ? rarityA : rarityB;
        return true;
    }
    if (SignalPresent(player, rule->b, -2, runSeed, &itemB, &rarityB) &&
        SignalPresent(player, rule->a, itemB, runSeed, &itemA, &rarityA))
    {
        *outMinRarity = (rarityA < rarityB) ? rarityA : rarityB;
        return true;
    }
    return false;
}

unsigned int SynergiesDetect(const Player *player, unsigned int runSeed)
{
    unsigned int mask = 0u;
    if (!player) return 0u;
    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        Rarity minRarity = RARITY_COMMON;
        if (RuleActive(player, &SYNERGY_RULES[i], runSeed, &minRarity)) mask |= (1u << i);
    }
    return mask;
}

/* Potenza scalata per la rarita' MINIMA della coppia (design doc, sezione 4.5:
   "la sinergia di due comuni e' piccola; due leggendari danno il colpo grosso").
   Riusa la STESSA scala dei tetti per-oggetto (script_items.c,
   SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION: 0.15/0.25/0.40/0.60, normalizzata sul
   livello non-comune) invece di introdurre un secondo sistema di bilanciamento:
   una sinergia e' "un oggetto in piu'" col suo budget. Comune 0.6x, non-comune
   1.0x, raro 1.6x, leggendario 2.4x. */
static float SynergyRarityScale(Rarity rarity)
{
    static const float kFraction[4] = { 0.15f, 0.25f, 0.40f, 0.60f };
    if (rarity < RARITY_COMMON || rarity > RARITY_LEGENDARY) rarity = RARITY_COMMON;
    return kFraction[rarity]/kFraction[RARITY_UNCOMMON];
}

SynergyStatBonus SynergiesStatBonus(const Player *player, unsigned int mask, unsigned int runSeed)
{
    SynergyStatBonus bonus = { 1.0f, 1.0f, 1.0f, 0.0f };
    if (!player || mask == 0u) return bonus;

    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        if (!(mask & (1u << i))) continue;
        const SynergyRule *rule = &SYNERGY_RULES[i];
        Rarity minRarity = RARITY_COMMON;
        if (!RuleActive(player, rule, runSeed, &minRarity)) continue;   /* la maschera e' vecchia: non fidarsi */
        float scale = SynergyRarityScale(minRarity);

        /* Lo SCOSTAMENTO da 1.0 e' cio' che scala con la rarita', non il
           moltiplicatore intero: 1.10 con scala 2.4 diventa 1.24, non 2.64. */
        bonus.damageMul    *= 1.0f + (rule->damageMul - 1.0f)*scale;
        bonus.fireDelayMul *= 1.0f + (rule->fireDelayMul - 1.0f)*scale;
        bonus.shotSpeedMul *= 1.0f + (rule->shotSpeedMul - 1.0f)*scale;
        bonus.luckAdd      += rule->luckAdd*scale;
    }
    return bonus;
}

/* I bonus DISCRETI del canale B scalano anch'essi con la rarita' (stessa scala),
   arrotondati per eccesso a un minimo di 1: una coppia di comuni deve comunque
   dare qualcosa di percepibile (mai zero: sarebbe una sinergia annunciata dalla
   GUI e invisibile in gioco), una di leggendari deve dare il colpo grosso. */
static int ScaledKnob(int base, float scale)
{
    if (base <= 0) return 0;
    int scaled = (int)floorf((float)base*scale + 0.5f);
    return scaled < 1 ? 1 : scaled;
}

void SynergiesApplyToShot(const Player *player, unsigned int mask, unsigned int runSeed, Shot *shot)
{
    if (!player || !shot || mask == 0u) return;

    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        if (!(mask & (1u << i))) continue;
        const SynergyRule *rule = &SYNERGY_RULES[i];
        Rarity minRarity = RARITY_COMMON;
        if (!RuleActive(player, rule, runSeed, &minRarity)) continue;
        float scale = SynergyRarityScale(minRarity);

        shot->traits |= rule->grantTraits;   /* OR: idempotente, mai un cumulo esplosivo */
        shot->pierce += ScaledKnob(rule->pierceBonus, scale);
        shot->bounces += ScaledKnob(rule->bounceBonus, scale);
        shot->chain += ScaledKnob(rule->chainBonus, scale);
    }
    /* Marchio visivo: il renderer disegna un anello in piu' attorno a un colpo
       "sinergico" (game_renderer.c, DrawShot). La sinergia si DEVE vedere: era il
       punto del feedback ("le sinergie non si notano"). */
    shot->synergized = true;
}

/* I pallettoni NON scalano con la rarita' (a differenza di pierce/bounce/chain
   sopra): ogni pallettone e' un colpo intero in piu', quindi il piu' potente dei
   bonus discreti -- una coppia di leggendari che ne desse tre trasformerebbe il
   giocatore in un fucile a pompa, ben oltre "una cosa leggibile in piu'". Resta
   quindi al valore piatto della tavola. */
int SynergiesExtraPellets(unsigned int mask)
{
    int extra = 0;
    for (int i = 0; i < (int)SYNERGY_COUNT; i++)
    {
        if (mask & (1u << i)) extra += SYNERGY_RULES[i].pelletBonus;
    }
    return extra;
}

const char *SynergyName(int id)
{
    if (id < 0 || id >= (int)SYNERGY_COUNT) return "";
    return SYNERGY_RULES[id].name;
}

const char *SynergyDescription(int id)
{
    if (id < 0 || id >= (int)SYNERGY_COUNT) return "";
    return SYNERGY_RULES[id].description;
}
