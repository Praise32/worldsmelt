#include "world/pourhouse.h"

#include "audio/audio.h"
#include "core/game_math.h"
#include "game/game_internal.h"
#include "gameplay/item_traits.h"
#include "world/world.h"

#include <stdio.h>
#include <string.h>

/* WP7 -- lo scambio ad alto rischio (Pourhouse, DEC-136/DEC-044). Il perche'
   della tabella di equivalenza e dei limiti sta tutto in pourhouse.h: qui c'e'
   solo il come. */

/* ============================================================
   Valori in punti di equita'
   ============================================================ */

static int PourhouseItemValue(Rarity rarity)
{
    /* Nessun numero nuovo: il valore di un oggetto E' il prezzo che il negozio
       gia' pratica per la sua fascia di rarita' (DEC-026, fonte unica
       ItemShopCostForRarity). Se quei prezzi cambiano, la Pourhouse li segue
       da sola invece di divergere in silenzio. */
    return ItemShopCostForRarity(rarity);
}

/* La rarita' migliore fra i tre candidati del piano: e' cio' che rende
   "oggetto di rarita' superiore" un fatto e non una promessa (stessa scelta
   gia' fatta per la ricompensa dell'arena, WorldSpawnRoomReward). A parita'
   vince l'indice piu' basso, stabile a parita' di seed. */
static int PourhouseBestFloorItemIndex(const Game *game)
{
    if (game->floor < 1 || game->floor > FLOOR_COUNT) return 0;
    const FloorContent *fc = &game->content.floors[game->floor - 1];
    int best = 0;
    for (int i = 1; i < 3; i++) if ((int)fc->items[i].rarity > (int)fc->items[best].rarity) best = i;
    return best;
}

/* ============================================================
   Il catalogo delle OFFERTE candidate
   ------------------------------------------------------------
   Tabella fissa e piccola invece di un'estrazione libera di quantita': il
   budget di equita' deve restare verificabile a mano (i valori in punti sono
   scritti accanto a ciascuna riga), e una quantita' pescata a caso avrebbe
   solo aggiunto rumore senza aggiungere scelte per il giocatore.
   ============================================================ */

typedef struct PourhouseOfferSpec {
    PourhouseOfferKind kind;
    int amount;   /* Ingots / Crust / Flux, oppure strumenti di BRECCIA per SUPPLIES */
    int keys;     /* SUPPLIES: strumenti di apertura */
} PourhouseOfferSpec;

static const PourhouseOfferSpec kPourhouseOffers[] = {
    { POURHOUSE_OFFER_COINS,    18, 0 },   /* 18 punti */
    { POURHOUSE_OFFER_COINS,    26, 0 },   /* 26 */
    { POURHOUSE_OFFER_COINS,    34, 0 },   /* 34 */
    { POURHOUSE_OFFER_COINS,    45, 0 },   /* 45 */
    { POURHOUSE_OFFER_SUPPLIES,  3, 2 },   /* 3x4 + 2x5 = 22 */
    { POURHOUSE_OFFER_SUPPLIES,  4, 3 },   /* 4x4 + 3x5 = 31 */
    { POURHOUSE_OFFER_SUPPLIES,  2, 4 },   /* 2x4 + 4x5 = 28 */
    { POURHOUSE_OFFER_CRUST,     2, 0 },   /* 24 */
    { POURHOUSE_OFFER_CRUST,     3, 0 },   /* 36 */
    { POURHOUSE_OFFER_FLUX,      1, 0 },   /* 30 */
    { POURHOUSE_OFFER_ITEM,      1, 0 },   /* 8/16/28/45 secondo la rarita' migliore del piano */
};
#define POURHOUSE_OFFER_SPEC_COUNT ((int)(sizeof(kPourhouseOffers)/sizeof(kPourhouseOffers[0])))

static int PourhouseOfferValue(const Game *game, const PourhouseOfferSpec *spec)
{
    switch (spec->kind)
    {
        case POURHOUSE_OFFER_COINS:    return spec->amount*POURHOUSE_VALUE_COIN;
        case POURHOUSE_OFFER_SUPPLIES: return spec->amount*POURHOUSE_VALUE_BOMB + spec->keys*POURHOUSE_VALUE_KEY;
        case POURHOUSE_OFFER_CRUST:    return spec->amount*POURHOUSE_VALUE_CRUST;
        case POURHOUSE_OFFER_FLUX:     return spec->amount*POURHOUSE_VALUE_FLUX;
        case POURHOUSE_OFFER_ITEM:
        {
            const FloorContent *fc = &game->content.floors[(game->floor >= 1 && game->floor <= FLOOR_COUNT) ? game->floor - 1 : 0];
            return PourhouseItemValue(fc->items[PourhouseBestFloorItemIndex(game)].rarity);
        }
        case POURHOUSE_OFFER_COUNT: break;
    }
    return 0;
}

/* Un'offerta puo' essere fuori gioco a prescindere dal prezzo: il Crust che
   sforerebbe il proprio tetto e l'oggetto che non entra in un inventario pieno
   sarebbero consegnati a META', ed e' esattamente cio' che l'atomicita' vieta.
   Si scartano gia' in composizione, cosi' il giocatore non legge nemmeno una
   promessa che il motore non potrebbe mantenere. */
static bool PourhouseOfferSpecDeliverable(const Game *game, const PourhouseOfferSpec *spec)
{
    switch (spec->kind)
    {
        case POURHOUSE_OFFER_CRUST: return game->player.tempHp + spec->amount <= PLAYER_TEMP_HP_CAP;
        case POURHOUSE_OFFER_ITEM:  return game->player.itemCount < MAX_ITEMS;
        case POURHOUSE_OFFER_COINS:
        case POURHOUSE_OFFER_SUPPLIES:
        case POURHOUSE_OFFER_FLUX:  return true;
        case POURHOUSE_OFFER_COUNT: break;
    }
    return false;
}

bool WorldPourhouseOfferDeliverable(const Game *game, const PourhouseWager *w)
{
    PourhouseOfferSpec spec = { w->offerKind, w->offerAmount, w->offerKeys };
    return PourhouseOfferSpecDeliverable(game, &spec);
}

/* ============================================================
   I PREZZI candidati
   ============================================================ */

static int PourhousePriceUnitValue(PourhousePriceKind kind)
{
    switch (kind)
    {
        case POURHOUSE_PRICE_COINS:  return POURHOUSE_VALUE_COIN;
        case POURHOUSE_PRICE_HP:     return POURHOUSE_VALUE_HP;
        case POURHOUSE_PRICE_MAX_HP: return POURHOUSE_VALUE_MAX_HP;
        case POURHOUSE_PRICE_FLUX:   return POURHOUSE_VALUE_FLUX;
        case POURHOUSE_PRICE_ITEM:   return 0;   /* non ha unita': il valore e' quello dell'oggetto scelto */
        case POURHOUSE_PRICE_COUNT:  break;
    }
    return 0;
}

/* Il budget di equita': |offerta - prezzo| dentro la tolleranza dichiarata. */
static bool PourhouseWithinEquityBudget(int offerValue, int priceValue)
{
    int tolerance = offerValue*POURHOUSE_EQUITY_TOLERANCE_PERCENT/100;
    if (tolerance < POURHOUSE_EQUITY_TOLERANCE_MIN) tolerance = POURHOUSE_EQUITY_TOLERANCE_MIN;
    int diff = offerValue - priceValue;
    if (diff < 0) diff = -diff;
    return diff <= tolerance;
}

/* Un baratto ha senso solo fra risorse DIVERSE: chiedere Ingots per dare
   Ingots (o Flux per Flux, o un oggetto per un oggetto di pari valore -- che
   e' l'unico che il budget di equita' lascerebbe passare) non e' una puntata,
   e' un giro a vuoto. Default proposto dall'implementazione: il documento non
   lo vieta esplicitamente perche' non gli e' venuto in mente che qualcuno lo
   proponesse. */
static bool PourhouseSameResource(PourhouseOfferKind offer, PourhousePriceKind price)
{
    if (offer == POURHOUSE_OFFER_COINS && price == POURHOUSE_PRICE_COINS) return true;
    if (offer == POURHOUSE_OFFER_FLUX && price == POURHOUSE_PRICE_FLUX) return true;
    if (offer == POURHOUSE_OFFER_ITEM && price == POURHOUSE_PRICE_ITEM) return true;
    return false;
}

bool WorldPourhousePricePayable(const Game *game, const PourhouseWager *w)
{
    const Player *p = &game->player;
    if (w->priceAmount < 1 && w->priceKind != POURHOUSE_PRICE_ITEM) return false;
    switch (w->priceKind)
    {
        case POURHOUSE_PRICE_COINS:
            return p->coins >= w->priceAmount;
        case POURHOUSE_PRICE_HP:
            /* Mai letale: si versa sangue, non la run. La salute
               temporanea/protettiva NON entra in questo conto (DEC-008: il
               Crust e' protezione, non valuta -- non paga mai un prezzo di
               salute), quindi si guarda solo 'hp'. */
            return p->hp - w->priceAmount >= 1;
        case POURHOUSE_PRICE_MAX_HP:
            /* Il tetto non scende mai sotto un cuore, e non si puo' chiedere
               piu' tetto di quanto il giocatore ne abbia: sono la stessa
               disuguaglianza (caso limite di special-rooms.md). */
            return p->baseMaxHp - w->priceAmount >= POURHOUSE_MIN_BASE_MAX_HP;
        case POURHOUSE_PRICE_FLUX:
            return p->flux >= w->priceAmount;
        case POURHOUSE_PRICE_ITEM:
        {
            if (!w->priceItemName[0]) return false;
            for (int i = 0; i < p->itemCount && i < MAX_ITEMS; i++)
                if (strncmp(p->items[i].name, w->priceItemName, sizeof(p->items[i].name)) == 0) return true;
            return false;
        }
        case POURHOUSE_PRICE_COUNT: break;
    }
    return false;
}

/* Riempie 'w' con la coppia (offerta, prezzo) candidata, o torna falso se
   quella coppia non esiste/non e' equa/non e' pagabile. Non tocca lo stato del
   gioco: e' una funzione di prova, chiamata fino a 55 volte dal ciclo sotto. */
static bool PourhouseBuildCandidate(const Game *game, const PourhouseOfferSpec *offer,
                                    PourhousePriceKind priceKind, unsigned int itemPickOffset,
                                    PourhouseWager *w)
{
    if (PourhouseSameResource(offer->kind, priceKind)) return false;
    if (!PourhouseOfferSpecDeliverable(game, offer)) return false;

    int offerValue = PourhouseOfferValue(game, offer);
    if (offerValue <= 0) return false;

    memset(w, 0, sizeof(*w));
    w->offerKind = offer->kind;
    w->offerAmount = offer->amount;
    w->offerKeys = offer->keys;
    w->offerValue = offerValue;
    if (offer->kind == POURHOUSE_OFFER_ITEM)
    {
        const FloorContent *fc = &game->content.floors[(game->floor >= 1 && game->floor <= FLOOR_COUNT) ? game->floor - 1 : 0];
        w->offerItem = fc->items[PourhouseBestFloorItemIndex(game)];
        w->offerAmount = 1;
    }

    w->priceKind = priceKind;
    if (priceKind == POURHOUSE_PRICE_ITEM)
    {
        /* Si cerca fra gli oggetti POSSEDUTI quello il cui valore entra nel
           budget di equita', partendo da un offset deterministico cosi' due
           Pourhouse diverse non chiedono sempre il primo oggetto raccolto. */
        int owned = game->player.itemCount;
        if (owned > MAX_ITEMS) owned = MAX_ITEMS;
        if (owned <= 0) return false;
        for (int k = 0; k < owned; k++)
        {
            int idx = (int)((itemPickOffset + (unsigned int)k)%(unsigned int)owned);
            const Item *candidate = &game->player.items[idx];
            if (!candidate->name[0]) continue;
            int value = PourhouseItemValue(candidate->rarity);
            if (!PourhouseWithinEquityBudget(offerValue, value)) continue;
            snprintf(w->priceItemName, sizeof(w->priceItemName), "%s", candidate->name);
            w->priceItemRarity = candidate->rarity;
            w->priceAmount = 1;
            w->priceValue = value;
            break;
        }
        if (!w->priceItemName[0]) return false;
    }
    else
    {
        int unit = PourhousePriceUnitValue(priceKind);
        if (unit <= 0) return false;
        /* Arrotondamento al piu' vicino: la quantita' che avvicina di piu' il
           prezzo all'offerta, mai un troncamento verso il basso (regalerebbe
           sistematicamente al giocatore e renderebbe il budget di equita' una
           formalita'). */
        int amount = (offerValue + unit/2)/unit;
        if (amount < 1) amount = 1;
        w->priceAmount = amount;
        w->priceValue = amount*unit;
    }

    if (!PourhouseWithinEquityBudget(w->offerValue, w->priceValue)) return false;
    if (!WorldPourhousePricePayable(game, w)) return false;

    w->composed = true;
    w->valid = true;
    return true;
}

unsigned int WorldPourhouseSignature(const PourhouseWager *w)
{
    if (!w->valid) return 0u;
    /* Categoria + quantita' di entrambi i lati: due puntate che differiscono
       anche solo nella quantita' offerta sono gia' due puntate diverse per il
       giocatore, e devono esserlo anche qui. Il +1 sulle categorie evita che
       la firma di una puntata valida possa valere 0 (che significa "nessuna"). */
    unsigned int sig = (unsigned int)(w->offerKind + 1);
    sig = sig*61u + (unsigned int)(w->offerAmount + 1);
    sig = sig*61u + (unsigned int)(w->offerKeys + 1);
    sig = sig*61u + (unsigned int)(w->priceKind + 1);
    sig = sig*61u + (unsigned int)(w->priceAmount + 1);
    return sig;
}

/* Gli scorrimenti ammessi per il giro sulle 55 coppie candidate: devono essere
   COPRIMI con 55 (= 5 x 11), altrimenti il giro non visiterebbe tutte le
   coppie ma solo un sottoinsieme ciclico -- e una coppia mai visitata sarebbe
   una puntata che non puo' mai uscire. Nessuno di questi e' divisibile per 5
   o per 11. */
static const int kPourhouseStrides[] = { 3, 7, 9, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53 };

void WorldComposePourhouseWager(const Game *game, int roomX, int roomY, PourhouseWager *out)
{
    memset(out, 0, sizeof(*out));
    out->composed = true;
    out->roomX = roomX;
    out->roomY = roomY;

    /* Stream LOCALE derivato dal seed di RUN, dal piano e dalla cella: mai
       game->rng. Due motivi, entrambi necessari: (1) questa funzione puo'
       girare piu' volte per la stessa stanza (finche' non esiste una puntata
       valida, vedi WorldPourhousePrepareRoom) e non deve spostare il flusso di
       gioco; (2) la stessa run con lo stesso seme deve comporre la stessa
       puntata a parita' di stato del giocatore, che e' il senso stesso di
       "deterministica dal seed". */
    unsigned int state = game->runSeed
                       ^ ((unsigned int)game->floor*2654435761u)
                       ^ ((unsigned int)(roomX + 1)*40503u)
                       ^ ((unsigned int)(roomY + 1)*2246822519u)
                       ^ 0x50594F55u;   /* 'PYOU' */

    const int total = POURHOUSE_OFFER_SPEC_COUNT*(int)POURHOUSE_PRICE_COUNT;
    int start = (int)(GameRngNext(&state)%(unsigned int)total);
    int stride = kPourhouseStrides[GameRngNext(&state)%(unsigned int)(sizeof(kPourhouseStrides)/sizeof(kPourhouseStrides[0]))];
    unsigned int itemPickOffset = GameRngNext(&state);

    /* Due passate (Scenario 8, "ogni scambio e' diverso"): la prima scarta
       ogni coppia la cui firma coincide con l'ULTIMA puntata composta nella
       run; la seconda accetta anche quella. Cosi' due Pourhouse nella stessa
       run propongono puntate diverse quando e' possibile, ma un giocatore che
       possiede una cosa sola non resta mai davanti a un banco muto solo perche'
       l'unica puntata pagabile e' quella di prima. */
    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < total; i++)
        {
            int combo = (start + i*stride)%total;
            const PourhouseOfferSpec *offer = &kPourhouseOffers[combo/(int)POURHOUSE_PRICE_COUNT];
            PourhousePriceKind priceKind = (PourhousePriceKind)(combo%(int)POURHOUSE_PRICE_COUNT);

            PourhouseWager candidate;
            if (!PourhouseBuildCandidate(game, offer, priceKind, itemPickOffset, &candidate)) continue;
            if (pass == 0 && game->pourhouseLastSignature != 0u &&
                WorldPourhouseSignature(&candidate) == game->pourhouseLastSignature) continue;

            candidate.roomX = roomX;
            candidate.roomY = roomY;
            *out = candidate;
            return;
        }
    }
    /* Nessuna coppia equa e pagabile: 'valid' resta falso e la stanza offrira'
       la sola uscita libera (special-rooms.md, Scenario 3). Non e' un errore
       ne' un fallback: e' uno stato previsto del contratto dell'archetipo. */
}

/* ============================================================
   Testi leggibili (DEC-058: mai un'informazione affidata al solo colore)
   ============================================================ */

static const char *PourhouseRarityWord(Rarity rarity)
{
    switch (rarity)
    {
        case RARITY_COMMON:    return "comune";
        case RARITY_UNCOMMON:  return "non-comune";
        case RARITY_RARE:      return "raro";
        case RARITY_LEGENDARY: return "leggendario";
    }
    return "comune";
}

void WorldPourhouseOfferText(const PourhouseWager *w, char *out, int cap)
{
    if (cap <= 0) return;
    if (!w->valid) { snprintf(out, (size_t)cap, "%s", "niente"); return; }
    switch (w->offerKind)
    {
        case POURHOUSE_OFFER_COINS:
            snprintf(out, (size_t)cap, "%d Ingots", w->offerAmount);
            return;
        case POURHOUSE_OFFER_SUPPLIES:
            if (w->offerAmount > 0 && w->offerKeys > 0)
                snprintf(out, (size_t)cap, "%d Blast Charges e %d Cast Keys", w->offerAmount, w->offerKeys);
            else if (w->offerKeys > 0) snprintf(out, (size_t)cap, "%d Cast Keys", w->offerKeys);
            else snprintf(out, (size_t)cap, "%d Blast Charges", w->offerAmount);
            return;
        case POURHOUSE_OFFER_CRUST:
            snprintf(out, (size_t)cap, "%d Crust", w->offerAmount);
            return;
        case POURHOUSE_OFFER_FLUX:
            snprintf(out, (size_t)cap, "%d Flux", w->offerAmount);
            return;
        case POURHOUSE_OFFER_ITEM:
            snprintf(out, (size_t)cap, "%s, oggetto %s",
                     w->offerItem.name[0] ? w->offerItem.name : "un oggetto del piano",
                     PourhouseRarityWord(w->offerItem.rarity));
            return;
        case POURHOUSE_OFFER_COUNT: break;
    }
    snprintf(out, (size_t)cap, "%s", "niente");
}

void WorldPourhousePriceText(const PourhouseWager *w, char *out, int cap)
{
    if (cap <= 0) return;
    if (!w->valid) { snprintf(out, (size_t)cap, "%s", "niente"); return; }
    switch (w->priceKind)
    {
        case POURHOUSE_PRICE_COINS:
            snprintf(out, (size_t)cap, "%d Ingots", w->priceAmount);
            return;
        case POURHOUSE_PRICE_HP:
            snprintf(out, (size_t)cap, "%d di salute", w->priceAmount);
            return;
        case POURHOUSE_PRICE_MAX_HP:
            snprintf(out, (size_t)cap, "%d di salute MASSIMA", w->priceAmount);
            return;
        case POURHOUSE_PRICE_FLUX:
            snprintf(out, (size_t)cap, "%d Flux", w->priceAmount);
            return;
        case POURHOUSE_PRICE_ITEM:
            snprintf(out, (size_t)cap, "il tuo %s, %s", w->priceItemName, PourhouseRarityWord(w->priceItemRarity));
            return;
        case POURHOUSE_PRICE_COUNT: break;
    }
    snprintf(out, (size_t)cap, "%s", "niente");
}

/* ============================================================
   Ingresso nella stanza e accettazione
   ============================================================ */

void WorldPourhousePrepareRoom(Game *game)
{
    PourhouseWager *w = &game->pourhouse;
    bool sameRoom = w->composed && w->roomX == game->roomX && w->roomY == game->roomY;
    /* Si ricompone quando la puntata e' di un'ALTRA stanza (mai ereditarla) e
       anche quando questa stanza non e' ancora riuscita a proporne una valida:
       una colata fredda non e' una puntata, e un giocatore che torna con
       qualcosa da versare deve poterla trovare accesa. Una puntata VALIDA
       invece non si ri-tira mai -- una sola puntata per stanza per run, che
       resta la stessa finche' non viene accettata (default proposto
       dall'implementazione). */
    if (!sameRoom || (!w->valid && !w->accepted))
    {
        WorldComposePourhouseWager(game, game->roomX, game->roomY, w);
        if (w->valid) game->pourhouseLastSignature = WorldPourhouseSignature(w);
    }

    int bankState = w->accepted ? 2 : (w->valid ? 1 : 0);
    Vector2 center = WorldRoomCenter(game);
    EntitiesAddPickup(game, PICKUP_POURHOUSE_BANK, center, bankState, 0);

    if (w->accepted)
    {
        GameSetMessage(game, "Pourhouse: la colata e' gia' stata versata.");
    }
    else if (w->valid)
    {
        /* Buffer volutamente piu' stretti di quelli del banco (96): la riga di
           messaggio ha 160 caratteri in tutto, e un nome di oggetto lunghissimo
           deve troncare QUI, non far sparire la seconda meta' del contratto.
           Il testo integrale resta comunque leggibile sul banco, che e' la
           fonte vera (DrawPickup, PICKUP_POURHOUSE_BANK). */
        char offerText[56], priceText[56], msg[160];
        WorldPourhouseOfferText(w, offerText, (int)sizeof(offerText));
        WorldPourhousePriceText(w, priceText, (int)sizeof(priceText));
        snprintf(msg, sizeof(msg), "Pourhouse: dai %s -> prendi %s. X per accettare.", priceText, offerText);
        GameSetMessage(game, msg);
    }
    else
    {
        /* Scenario 3: niente da cedere, nessuna penalita', nessun blocco. */
        GameSetMessage(game, "Pourhouse: la colata e' fredda, non hai nulla da versare. Esci quando vuoi.");
    }
}

bool WorldTryAcceptPourhouseWager(Game *game)
{
    RoomState *room = WorldCurrentRoomMutable(game);
    if (room->kind != ROOM_POURHOUSE) return false;
    PourhouseWager *w = &game->pourhouse;
    if (!w->valid || w->accepted) return false;
    /* La puntata deve essere DI QUESTA stanza. Oggi non puo' essere altrimenti
       (una sola Pourhouse per piano, e WorldPourhousePrepareRoom ricompone
       quando le coordinate non coincidono), ma la guardia costa una riga e
       chiude per costruzione la peggiore forma di difetto possibile qui:
       accettare in una stanza una puntata composta per un'altra. */
    if (w->roomX != game->roomX || w->roomY != game->roomY) return false;

    /* Il banco deve essere sotto i piedi: la stessa geometria di contatto di
       ogni altro pickup (CombatUpdatePickups), come per il segnale dell'arena.
       Premuto in mezzo alla stanza il tasto non fa nulla. */
    Pickup *bank = NULL;
    for (int i = 0; i < MAX_PICKUPS; i++)
    {
        if (game->pickups[i].active && game->pickups[i].kind == PICKUP_POURHOUSE_BANK) { bank = &game->pickups[i]; break; }
    }
    if (!bank) return false;
    float r = bank->radius + game->player.radius;
    if (GameMathLengthSquared(GameMathSubtract(bank->pos, game->player.pos)) > r*r) return false;

    /* ATOMICITA': si controlla TUTTO prima di toccare qualunque cosa. Fra la
       composizione e questo momento il giocatore puo' aver speso il prezzo
       altrove o riempito l'inventario -- e mezza puntata (prezzo pagato,
       offerta mai arrivata, o viceversa) sarebbe il difetto peggiore che
       questo archetipo possa avere. Un fallimento qui non costa niente:
       rifiutare o non poter pagare non e' mai una penalita'. */
    if (!WorldPourhousePricePayable(game, w))
    {
        AudioPlaySfx(AUDIO_SFX_UI_CANCEL);
        GameSetMessage(game, "Non puoi piu' pagare questa puntata. Niente versato, nessuna penalita'.");
        return false;
    }
    if (!WorldPourhouseOfferDeliverable(game, w))
    {
        AudioPlaySfx(AUDIO_SFX_UI_CANCEL);
        GameSetMessage(game, "Non c'e' posto per l'offerta. Niente versato, nessuna penalita'.");
        return false;
    }

    /* --- il prezzo --- */
    Player *p = &game->player;
    switch (w->priceKind)
    {
        case POURHOUSE_PRICE_COINS:
            p->coins -= w->priceAmount;
            break;
        case POURHOUSE_PRICE_HP:
            /* Direttamente su 'hp', mai via CombatDamagePlayer: un patto non
               e' un colpo subito (niente i-frame, niente suono di danno,
               niente causa di morte) e soprattutto DEC-008 -- il Crust e'
               protezione, non valuta, e non paga mai un prezzo di salute. */
            p->hp -= w->priceAmount;
            if (p->hp < 1) p->hp = 1;
            break;
        case POURHOUSE_PRICE_MAX_HP:
            /* Il tetto vero e' 'baseMaxHp': 'maxHp' e' un valore DERIVATO che
               ScriptItemsRecomputeStats ricalcola da zero ad ogni passaggio
               (sistema delle cache), quindi ridurre solo quello verrebbe
               annullato al primo ricalcolo. CombatReducePlayerMaxHp fa
               entrambe le cose e riclampa 'hp' al nuovo tetto. */
            CombatReducePlayerMaxHp(game, w->priceAmount);
            break;
        case POURHOUSE_PRICE_FLUX:
            p->flux -= w->priceAmount;
            break;
        case POURHOUSE_PRICE_ITEM:
            CombatRemovePlayerItemByName(game, w->priceItemName);
            break;
        case POURHOUSE_PRICE_COUNT:
            break;
    }

    /* --- l'offerta --- */
    switch (w->offerKind)
    {
        case POURHOUSE_OFFER_COINS:
            p->coins += w->offerAmount;
            break;
        case POURHOUSE_OFFER_SUPPLIES:
            p->bombs += w->offerAmount;
            p->keys += w->offerKeys;
            break;
        case POURHOUSE_OFFER_CRUST:
            p->tempHp = GameMathClampInt(p->tempHp + w->offerAmount, 0, PLAYER_TEMP_HP_CAP);
            break;
        case POURHOUSE_OFFER_FLUX:
            p->flux += w->offerAmount;
            break;
        case POURHOUSE_OFFER_ITEM:
            CombatGrantPlayerItem(game, w->offerItem);
            break;
        case POURHOUSE_OFFER_COUNT:
            break;
    }

    w->accepted = true;
    bank->value = 2;
    AudioPlaySfx(AUDIO_SFX_UI_CONFIRM);

    char offerText[56], priceText[56], msg[160];   /* stessa ragione dei buffer di WorldPourhousePrepareRoom */
    WorldPourhouseOfferText(w, offerText, (int)sizeof(offerText));
    WorldPourhousePriceText(w, priceText, (int)sizeof(priceText));
    snprintf(msg, sizeof(msg), "Colata versata: hai dato %s -> ricevuto %s.", priceText, offerText);
    GameSetMessage(game, msg);
    return true;
}
