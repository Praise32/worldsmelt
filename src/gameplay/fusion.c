#include "gameplay/fusion.h"

#include "content/curated_images.h"
#include "core/game_math.h"
#include "core/shot_type.h"
#include "gameplay/item_slots.h"
#include "script/script_items.h"

#include <stdio.h>
#include <string.h>

/* Perche' questo file puo' includere src/content: il pacchetto di immagini
   curate e' CONTENUTO della run (AGENTS.md, "src/content: manifest e
   contenuti della run"), letto in sola lettura, esattamente come
   src/world/world.c legge content/run_content.h per gli oggetti del piano.
   Cio' che NON entra mai qui e' raylib-per-disegnare: questo modulo non crea
   texture, non apre immagini e gira identico nei test senza finestra. */

const char *FusionStatusText(FusionStatus status)
{
    switch (status)
    {
        case FUSION_OK:              return "Fusione riuscita.";
        case FUSION_ERR_NO_CATALYST: return "Serve un catalizzatore (Flux).";
        case FUSION_ERR_NEED_TWO:    return "Servono almeno due oggetti idonei.";
        case FUSION_ERR_SAME_ITEM:   return "Scegli due oggetti diversi.";
        /* Copre due casi che per il giocatore sono lo stesso: non ha ancora
           scelto le due sorgenti, oppure una delle due non e' piu' valida
           (l'inventario e' cambiato). In entrambi la mossa da fare e' la
           stessa, quindi il testo dice quella invece di spiegare il codice. */
        case FUSION_ERR_NOT_ELIGIBLE:return "Scegli due oggetti da fondere (INVIO).";
    }
    return "Scegli due oggetti da fondere (INVIO).";
}

bool FusionItemEligible(const Item *item)
{
    return item != NULL && item->active;
}

int FusionEligibleCount(const Player *p)
{
    if (!p) return 0;
    int count = GameMathClampInt(p->itemCount, 0, MAX_ITEMS);
    int eligible = 0;
    for (int i = 0; i < count; i++) if (FusionItemEligible(&p->items[i])) eligible++;
    return eligible;
}

FusionStatus FusionCheck(const Player *p, int indexA, int indexB)
{
    if (!p) return FUSION_ERR_NOT_ELIGIBLE;
    /* Ordine dei controlli = ordine in cui il giocatore li incontra
       nell'interfaccia (item-fusion.md, tabella "Input/azioni"): prima "ho
       abbastanza oggetti?", poi "ne ho scelti due diversi e validi?", e per
       ULTIMO il catalizzatore, che e' la condizione che disabilita la sola
       CONFERMA (la selezione resta possibile senza, ed e' proprio lo
       scenario "catalizzatore mancante"). */
    if (FusionEligibleCount(p) < 2) return FUSION_ERR_NEED_TWO;
    int count = GameMathClampInt(p->itemCount, 0, MAX_ITEMS);
    if (indexA < 0 || indexA >= count || indexB < 0 || indexB >= count) return FUSION_ERR_NOT_ELIGIBLE;
    if (indexA == indexB) return FUSION_ERR_SAME_ITEM;
    if (!FusionItemEligible(&p->items[indexA]) || !FusionItemEligible(&p->items[indexB])) return FUSION_ERR_NOT_ELIGIBLE;
    if (p->flux < 1) return FUSION_ERR_NO_CATALYST;
    return FUSION_OK;
}

int FusionDominant(const Item *a, const Item *b)
{
    if (!a || !b) return 0;
    /* "vince la categoria dell'oggetto sorgente di rarita' piu' alta; a
       parita' di rarita' vince l'oggetto selezionato per primo dal
       giocatore" (DEC-143 + punto 4 di "Priorita' e conflitti"). Il ">"
       stretto e' esattamente il tie-break: a parita' resta 'a'. */
    return (b->rarity > a->rarity) ? 1 : 0;
}

/* Gruppi di trait che competono sulla STESSA proprieta' del colpo
   (item-fusion.md, "Priorita' e conflitti": "per esempio entrambi
   modificano la traiettoria del proiettile"). Il raggruppamento e' un
   default proposto dall'implementazione (stile DEC-019) -- il catalogo dei
   trait del motore e' una maschera piatta di 9 bit, nessun documento li
   raggruppa -- ed e' cio' che rende la regola 4 applicabile davvero: senza
   gruppi, "vince il tratto della sorgente piu' rara" non avrebbe alcun
   conflitto da risolvere e la fusione sarebbe una semplice unione. */
static const unsigned int FUSION_TRAIT_GROUPS[] = {
    TRAIT_BOUNCE | TRAIT_HOMING,                   /* traiettoria */
    TRAIT_EXPLODE | TRAIT_SPLIT | TRAIT_PIERCE,    /* cosa succede all'impatto */
    TRAIT_RAPID | TRAIT_GIANT,                     /* corpo e cadenza del colpo */
    TRAIT_SLOW | TRAIT_VAMP                        /* effetto sul bersaglio */
};

/* Ordine di priorita' dei trait, lo STESSO di ItemFirstTraitName
   (src/gameplay/item_traits.c): quando il risultato ne ha piu' di
   FUSION_MAX_TRAITS, si tengono i primi in questo ordine -- prima quelli che
   arrivano dalla sorgente dominante. */
static const unsigned int FUSION_TRAIT_ORDER[] = {
    TRAIT_BOUNCE, TRAIT_HOMING, TRAIT_EXPLODE, TRAIT_SPLIT, TRAIT_PIERCE,
    TRAIT_RAPID, TRAIT_GIANT, TRAIT_SLOW, TRAIT_VAMP
};

static int FusionTraitCount(unsigned int traits)
{
    int n = 0;
    for (size_t i = 0; i < sizeof(FUSION_TRAIT_ORDER)/sizeof(FUSION_TRAIT_ORDER[0]); i++)
        if (traits & FUSION_TRAIT_ORDER[i]) n++;
    return n;
}

static unsigned int FusionComposeTraits(unsigned int dominant, unsigned int other)
{
    unsigned int merged = 0;
    for (size_t g = 0; g < sizeof(FUSION_TRAIT_GROUPS)/sizeof(FUSION_TRAIT_GROUPS[0]); g++)
    {
        unsigned int group = FUSION_TRAIT_GROUPS[g];
        unsigned int fromDominant = dominant & group;
        /* Conflitto sulla stessa proprieta': vince il dominante e l'altro
           NON entra. Nessun conflitto (solo uno dei due dichiara qualcosa su
           questa proprieta'): entra chi lo dichiara -- e' l'"eredita tratti
           da entrambi" del documento. */
        merged |= fromDominant ? fromDominant : (other & group);
    }
    if (FusionTraitCount(merged) <= FUSION_MAX_TRAITS) return merged;

    unsigned int capped = 0;
    int kept = 0;
    for (int pass = 0; pass < 2 && kept < FUSION_MAX_TRAITS; pass++)
    {
        for (size_t i = 0; i < sizeof(FUSION_TRAIT_ORDER)/sizeof(FUSION_TRAIT_ORDER[0]) && kept < FUSION_MAX_TRAITS; i++)
        {
            unsigned int flag = FUSION_TRAIT_ORDER[i];
            if (!(merged & flag)) continue;
            bool isDominant = (dominant & flag) != 0;
            if ((pass == 0) != isDominant) continue;   /* primo giro: solo i trait del dominante */
            capped |= flag;
            kept++;
        }
    }
    return capped;
}

/* Copia la prima (o l'ultima) parola di 'name' in 'out'. Una parola e' cio'
   che sta fra spazi: nomi di una sola parola danno la stessa cosa in
   entrambi i casi, ed e' voluto. */
static void FusionCopyToken(const char *name, bool last, char *out, int outSize)
{
    out[0] = '\0';
    if (!name || !name[0] || outSize <= 1) return;

    const char *start = name;
    const char *end = name + strlen(name);
    if (last)
    {
        const char *space = strrchr(name, ' ');
        if (space && space[1] != '\0') start = space + 1;
    }
    else
    {
        const char *space = strchr(name, ' ');
        if (space && space != name) end = space;
    }

    int len = (int)(end - start);
    if (len < 0) len = 0;
    if (len >= outSize) len = outSize - 1;
    memcpy(out, start, (size_t)len);
    out[len] = '\0';
}

/* Il nome del composto: la prima parola del DOMINANTE piu' una parola
   dell'altro (quale, lo decide la chiave -- vedi FusionKey). Nessun elenco
   di parole inventate dal C: il nome nasce SEMPRE dai due genitori, cosi'
   il motore non possiede un catalogo di nomi (la stessa regola per cui non
   possiede un catalogo di tipi di colpo, vedi src/core/shot_type.h). Lo
   stadio 2 (IA) lo sostituira' con un nome dedicato quando esistera'. */
static void FusionComposeName(unsigned int key, const Item *dominant, const Item *other, char *out, int outSize)
{
    char head[28];
    char tail[28];
    FusionCopyToken(dominant->name, false, head, (int)sizeof(head));
    FusionCopyToken(other->name, (key & 1u) != 0u, tail, (int)sizeof(tail));
    if (!head[0]) snprintf(head, sizeof(head), "Fuso");
    if (!tail[0]) snprintf(tail, sizeof(tail), "Composto");

    snprintf(out, (size_t)outSize, "%s %s", head, tail);
    /* Un composto che si chiama ESATTAMENTE come uno dei due genitori
       nasconderebbe al giocatore che e' successo qualcosa: in quel caso si
       usa la forma col trattino, che nessun contenuto generato produce. */
    if (strcmp(out, dominant->name) == 0 || strcmp(out, other->name) == 0)
        snprintf(out, (size_t)outSize, "%s-%s", head, tail);
}

static unsigned int FusionHashText(unsigned int hash, const char *text)
{
    /* FNV-1a a 32 bit: stabile, senza tabelle, uguale ovunque. */
    for (const char *p = text; p && *p; p++)
    {
        hash ^= (unsigned int)(unsigned char)*p;
        hash *= 16777619u;
    }
    return hash;
}

unsigned int FusionKey(unsigned int runSeed, int fusionOrdinal, const Item *a, const Item *b)
{
    /* Stesso schema di GameplayRngSeedFromRunSeed (src/game/game.c): uno
       splitmix64 con una costante di DOMINIO propria, cosi' la sequenza
       della fusione non e' mai la stessa di quella del gameplay o della
       generazione anche partendo dallo stesso seed di run. */
    const unsigned long long domain = 0x465553494F4E0001ULL;   /* 'FUSION' + costante di dominio */
    unsigned int mix = 2166136261u;
    mix = FusionHashText(mix, a ? a->name : "");
    mix = FusionHashText(mix, b ? b->name : "");
    mix ^= (unsigned int)fusionOrdinal*2654435761u;

    unsigned long long state = (((unsigned long long)runSeed << 32) ^ (unsigned long long)mix ^ domain) + 0x9E3779B97F4A7C15ULL;
    unsigned long long z = state;
    z = (z ^ (z >> 30))*0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27))*0x94D049BB133111EBULL;
    z ^= (z >> 31);
    return (unsigned int)(z >> 32) ^ (unsigned int)z;
}

void FusionCompose(unsigned int key, const Item *a, const Item *b, Item *out)
{
    if (!out || !a || !b) return;

    int dominantIsB = FusionDominant(a, b);
    const Item *dominant = dominantIsB ? b : a;
    const Item *other = dominantIsB ? a : b;

    /* Si parte dalla COPIA della sorgente dominante, non da un Item vuoto:
       cosi' tutto cio' che il documento non nomina esplicitamente (slot
       visivo, script mini-VM, sorgente Lua, dichiarazione di ricarica di un
       attivo) segue automaticamente la regola 4 -- "vince il tratto
       dell'oggetto di rarita' piu' alta" -- invece di sparire. La CATEGORIA
       arriva da qui, ed e' esattamente DEC-143. */
    *out = *dominant;
    out->active = true;

    /* DEC-162, primo canale del budget dedicato: la rarita' del risultato
       sale di un gradino rispetto alla dominante. Non e' cosmesi -- la
       rarita' E' il budget di potenza per-oggetto del motore (il tetto di
       delta per oggetto, SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION in
       src/script/script_items.c), quindi un gradino in piu' significa
       letteralmente "budget piu' alto di quello del singolo oggetto
       sorgente". Default proposto dall'implementazione (stile DEC-019): il
       documento fissa che il budget e' piu' alto, non di quanto. */
    out->rarity = (Rarity)GameMathClampInt((int)dominant->rarity + 1, RARITY_COMMON, RARITY_LEGENDARY);

    out->traits = FusionComposeTraits(dominant->traits, other->traits);

    /* Tipo di colpo. Se solo l'altro ne ha uno, e' suo che il risultato
       eredita (non c'e' conflitto da risolvere); se ce l'hanno entrambi
       vince il dominante, ma UNA manopola del perdente entra comunque nel
       risultato -- e' la parte "eredita ... da ENTRAMBI" di DEC-023, ed e'
       anche cio' che impedisce alla fusione di essere una ricetta
       prevedibile (item-fusion.md, Non-obiettivi). */
    if (!out->shotType.active && other->shotType.active) out->shotType = other->shotType;
    else if (out->shotType.active && other->shotType.active)
    {
        switch ((key >> 2) & 3u)
        {
            case 0:  out->shotType.speedMul  = other->shotType.speedMul;  break;
            case 1:  out->shotType.radiusMul = other->shotType.radiusMul; break;
            case 2:  out->shotType.lifeMul   = other->shotType.lifeMul;   break;
            default: out->shotType.form      = other->shotType.form;      break;   /* eredita' puramente visiva */
        }
    }
    if (out->shotType.active)
    {
        /* DEC-162, secondo canale: il colpo del fuso si bilancia verso la
           banda DEDICATA (piu' alta di quella del singolo oggetto), non
           verso 1.0. La verifica di leggibilita' resta quella di sempre
           (DEC-146 non si allarga per la fusione): se il tipo composto la
           sfonda si ricade sul tipo di colpo di un genitore, che e' gia'
           passato dalla validazione quando e' entrato nella run -- la
           "normale catena di fallback" invece di una limatura sul posto. */
        ShotTypeDef balanced = out->shotType;
        ShotTypeBalanceTo(&balanced, SHOT_TYPE_FUSION_POWER_TARGET,
                          SHOT_TYPE_FUSION_POWER_MIN, SHOT_TYPE_FUSION_POWER_MAX);
        if (ShotTypeReadabilityOk(&balanced)) out->shotType = balanced;
        else out->shotType = dominant->shotType.active ? dominant->shotType : other->shotType;
    }

    /* Strati visivi da entrambi (item-fusion.md, "Regole per contenuti
       generati": una fusione deve cambiare SIA il comportamento SIA la
       presentazione). Il colore e' la miscela dei due, la silhouette e'
       dell'uno o dell'altro secondo la chiave, l'ancora sul personaggio
       (slot) resta quella del dominante. Con l'immagine curata che
       FusionPerform aggiunge (DEC-171), il risultato non somiglia mai
       esattamente a nessuno dei due genitori. */
    out->color = GameColorLerp(dominant->color, other->color, 0.5f);
    out->shape = ((key >> 1) & 1u) ? other->shape : dominant->shape;

    FusionComposeName(key, dominant, other, out->name, (int)sizeof(out->name));

    /* I genitori si dichiarano nell'ordine di SELEZIONE (a, b), non di
       dominanza: e' quello che il giocatore ha visto fare. */
    /* La precisione esplicita (39 = sizeof-1) e' il modo di dire al
       compilatore che il troncamento e' VOLUTO: un nome di genitore piu'
       lungo del campo si taglia, non si perde la fusione. */
    snprintf(out->fusedFrom[0], sizeof(out->fusedFrom[0]), "%.39s", a->name);
    snprintf(out->fusedFrom[1], sizeof(out->fusedFrom[1]), "%.39s", b->name);
    out->imagePath[0] = '\0';   /* la pesca FusionPerform: dipende da cosa la run ha gia' usato */

    if (out->kind == ITEM_ACTIVE)
    {
        /* Un attivo deve dichiarare uno fra cariche e cooldown
           (systems/active-items.md). Se il dominante non dichiara nulla, si
           prende la dichiarazione dell'altro invece di lasciare che il
           risultato ricada sul cooldown di riserva del motore. */
        if (out->charges <= 0 && out->cooldown <= 0.0f)
        {
            out->charges = other->charges;
            out->cooldown = other->cooldown;
            out->chargeGainRoom = other->chargeGainRoom;
            out->chargeGainEnergy = other->chargeGainEnergy;
        }
        ItemActiveResetCharge(out);   /* un fuso nasce carico, come un attivo appena trovato */
    }
}

FusionStatus FusionPerform(Game *game, int indexA, int indexB, Item *outFused)
{
    if (!game) return FUSION_ERR_NOT_ELIGIBLE;
    Player *p = &game->player;

    /* Nessuna scrittura prima di questa riga: un tentativo che fallisce non
       deve mai consumare un oggetto o un catalizzatore (item-fusion.md,
       casi limite). */
    FusionStatus status = FusionCheck(p, indexA, indexB);
    if (status != FUSION_OK) return status;

    /* Copie: appena il primo sorgente esce da items[], gli indici scorrono. */
    Item a = p->items[indexA];
    Item b = p->items[indexB];

    unsigned int key = FusionKey(game->runSeed, game->fusionCount, &a, &b);
    Item fused;
    FusionCompose(key, &a, &b, &fused);

    /* DEC-171 (ponte provvisorio della demo): sprite pescato dal pacchetto
       curato fra le immagini NON ancora usate in questa run, in modo
       deterministico dal seed. Un pacchetto assente o esaurito non e' un
       errore: l'oggetto resta senza immagine e si disegna con la forma
       geometrica di sempre. */
    CuratedImage image;
    int imageIndex = -1;
    if (CuratedImagesPickUnused(CURATED_MANIFEST_PATH, key >> 8, "item",
                                game->curatedImageUsed, CURATED_IMAGE_MASK_BYTES, &image, &imageIndex))
    {
        snprintf(fused.imagePath, sizeof(fused.imagePath), "%s", image.file);
        CuratedImageMaskSet(game->curatedImageUsed, CURATED_IMAGE_MASK_BYTES, imageIndex);
    }

    /* Consumo. Si rimuove prima l'indice PIU' ALTO: ScriptItemsRemoveItem
       compatta items[] verso il basso, quindi togliere prima il basso
       sposterebbe l'altro sotto i piedi. */
    int high = (indexA > indexB) ? indexA : indexB;
    int low  = (indexA > indexB) ? indexB : indexA;
    ScriptItemsRemoveItem(game, high);
    ScriptItemsRemoveItem(game, low);
    p->flux--;
    game->fusionCount++;

    /* Inserimento: c'e' sempre posto (due usciti, uno entra), ma il clamp
       resta per un itemCount corrotto da un chiamante.
       Nessun rischio di sfondare gli slot funzionali: il risultato ha la
       categoria della sorgente DOMINANTE, cioe' di uno dei due oggetti
       appena consumati -- il numero di attivi (o di Innesti) posseduti non
       puo' quindi crescere con una fusione, al massimo cala. Per questo qui
       non serve la logica di scambio col piedistallo di CombatPickup. */
    int slot = GameMathClampInt(p->itemCount, 0, MAX_ITEMS - 1);
    p->items[slot] = fused;
    p->itemCount = slot + 1;
    ScriptItemsOnAcquire(game, slot);
    ScriptItemsProcessDirty(game);   /* il fuso e' gia' nelle statistiche nello stesso istante */

    if (outFused) *outFused = p->items[slot];
    /* Qui aggancerebbe lo STADIO 2 (rifinitura IA in sottofondo): vedi
       FUSION_STAGE_2_HOOK in fusion.h. Nella demo non parte nulla, per
       scelta -- non e' un TODO dimenticato. */
    return FUSION_OK;
}
