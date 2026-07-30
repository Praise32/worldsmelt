#include "game/trials.h"

#include "core/game_math.h"
#include "game/game_internal.h"

#include <string.h>
#include <stdio.h>

/* WP16 -- vedi il commento di apertura in trials.h per il contratto di
   ciascuna funzione. Qui solo il "come". */

/* Riempie una prova con testo/parametro/bonus secondo 'kind'. 'floorPick' e'
   il piano bersaglio gia' estratto dallo stream locale del chiamante (sempre
   dentro [1, FLOOR_COUNT], vedi il commento su TrialsAssignForRun in
   trials.h): i tipi che non usano un piano lo ignorano semplicemente. */
static void TrialsFillOne(Trial *t, TrialKind kind, int floorPick)
{
    memset(t, 0, sizeof(*t));
    t->kind = kind;
    t->state = TRIAL_IN_PROGRESS;
    switch (kind)
    {
        case TRIAL_BOSS_NO_DAMAGE:
            t->param = floorPick;
            t->bonus = TRIAL_BONUS_BOSS_NO_DAMAGE;
            snprintf(t->text, sizeof(t->text),
                     "Sconfiggi il boss del piano %d senza incassare un colpo.", floorPick);
            return;
        case TRIAL_SECRET_FOUND:
            t->bonus = TRIAL_BONUS_SECRET_FOUND;
            snprintf(t->text, sizeof(t->text), "%s",
                     "Trova una stanza segreta da qualche parte nel crogiolo.");
            return;
        case TRIAL_ARENA_WON:
            t->bonus = TRIAL_BONUS_ARENA_WON;
            snprintf(t->text, sizeof(t->text), "%s",
                     "Vinci una sfida dell'arena, se hai il fegato di accettarla.");
            return;
        case TRIAL_FLOOR_UNDER_TIME:
        {
            t->param = floorPick;
            t->bonus = TRIAL_BONUS_FLOOR_UNDER_TIME;
            int seconds = (int)(TRIAL_FLOOR_TIME_BASE_SECONDS
                               + TRIAL_FLOOR_TIME_PER_FLOOR_SECONDS*(float)floorPick);
            snprintf(t->text, sizeof(t->text),
                     "Completa il piano %d in meno di %ds, dall'ingresso nel piano.", floorPick, seconds);
            return;
        }
        case TRIAL_END_WITH_INGOTS:
            t->param = TRIAL_END_INGOTS_TARGET;
            t->bonus = TRIAL_BONUS_END_WITH_INGOTS;
            snprintf(t->text, sizeof(t->text),
                     "Finisci la run con almeno %d Ingots in tasca.", TRIAL_END_INGOTS_TARGET);
            return;
        case TRIAL_FUSE_ITEM:
            t->bonus = TRIAL_BONUS_FUSE_ITEM;
            snprintf(t->text, sizeof(t->text), "%s", "Fondi almeno un oggetto nel crogiolo.");
            return;
        case TRIAL_TIMED_ROOM_WITHIN_THRESHOLD:
            t->bonus = TRIAL_BONUS_TIMED_ROOM;
            snprintf(t->text, sizeof(t->text), "%s",
                     "Supera una stanza a tempo entro la soglia richiesta.");
            return;
        case TRIAL_NO_SHOP_PURCHASE:
            t->bonus = TRIAL_BONUS_NO_SHOP_PURCHASE;
            snprintf(t->text, sizeof(t->text), "%s",
                     "Non comprare mai nulla al negozio, tieni le mani in tasca.");
            return;
        case TRIAL_KIND_COUNT:
            break;
    }
}

void TrialsAssignForRun(Game *game)
{
    game->trialCount = 0;
    memset(game->trials, 0, sizeof(game->trials));
    game->currentBossFightDamaged = false;
    /* Game.timedRoomEverGenerated/secretRoomEverGenerated/arenaRoomEverGenerated
       (core/game_types.h) NON si toccano qui: sono gia' stati azzerati dal
       memset(game, 0, ...) di GameResetRunWithSeed e possono essere GIA'
       scritti a questo punto (WorldStartFloor(1), chiamata subito prima di
       questa funzione, li scrive per il piano 1 -- vedi il commento
       sull'ordine di chiamata in game.c). Azzerarli di nuovo qui
       cancellerebbe quell'informazione appena scritta. Solo
       TrialsFinalizeAtRunEnd li legge, a fine run. */

    /* Stream LOCALE derivato dal seed di RUN con una costante di dominio
       propria ('TRIA'), mai game->rng -- stessa disciplina di
       WorldComposePourhouseWager/WorldPlaceSecretRoom (src/world):
       questa funzione non deve spostare di un bit il flusso di gameplay, e
       la stessa run con lo stesso seme deve assegnare sempre le stesse
       prove indipendentemente da quanto game->rng e' gia' stato consumato
       quando viene chiamata. */
    unsigned int state = game->runSeed ^ 0x54524941u;   /* 'TRIA' */

    /* 2 o 3 prove per run (DEC-042 non fissa il numero esatto): estratto
       anche questo dallo stream locale, cosi' il CONTEGGIO stesso e'
       deterministico dal seed come i tipi/parametri sotto. */
    int wanted = 2 + (int)(GameRngNext(&state)%2u);

    bool usedKind[TRIAL_KIND_COUNT];
    memset(usedKind, 0, sizeof(usedKind));
    int assigned = 0;
    /* Guardia contro un guasto teorico (mai raggiunta: TRIAL_KIND_COUNT=8 >=
       wanted<=3, quindi il ciclo trova sempre abbastanza tipi non ancora
       usati) -- disciplina "mai un ciclo senza uscita", come WorldGenerateFloorMap. */
    int guard = TRIAL_KIND_COUNT*8;
    while (assigned < wanted && guard-- > 0)
    {
        int idx = (int)(GameRngNext(&state)%(unsigned int)TRIAL_KIND_COUNT);
        if (usedKind[idx]) continue;
        usedKind[idx] = true;
        int floorPick = 1 + (int)(GameRngNext(&state)%(unsigned int)FLOOR_COUNT);
        TrialsFillOne(&game->trials[assigned], (TrialKind)idx, floorPick);
        assigned++;
    }
    game->trialCount = assigned;

    /* Presentazione (floor-zero.md, "al passaggio dal Piano 0 al piano 1"):
       una card di scoperta per prova, lo stesso componente di sistema gia'
       usato per boss/nemici incontrati (DEC-065/131/152) -- niente overlay
       dedicato nuovo. Restano comunque consultabili per tutta la run da
       PauseMenu/BuildScreen indipendentemente da quanto a lungo la card resta
       visibile o viene scartata (stessa disciplina di ogni altra card:
       GameDiscardPendingDiscoveries non tocca mai lo STATO della prova, solo
       la coda di notifica). */
    for (int i = 0; i < game->trialCount; i++)
        GameQueueDiscoveryCardWithImage(game, "Prova", game->trials[i].text, NULL);
}

void TrialsOnBossRoomEntered(Game *game)
{
    game->currentBossFightDamaged = false;
}

void TrialsOnPlayerDamaged(Game *game)
{
    const RoomState *room = WorldCurrentRoomMutable(game);
    if (room->kind == ROOM_BOSS && !room->cleared) game->currentBossFightDamaged = true;
}

void TrialsOnRoomCleared(Game *game, RoomKind kind)
{
    for (int i = 0; i < game->trialCount; i++)
    {
        Trial *t = &game->trials[i];
        if (t->state != TRIAL_IN_PROGRESS) continue;

        if (kind == ROOM_BOSS && t->kind == TRIAL_BOSS_NO_DAMAGE && t->param == game->floor)
        {
            /* 't->param == game->floor' NON e' ridondante: senza questa
               guardia il boss di un piano QUALUNQUE deciderebbe l'esito di
               una prova che parla di un piano bersaglio diverso (es. la
               prova dice "piano 3" e questo e' il boss del piano 1 appena
               ripulito) -- verificato da trials-test, TrialsTestFloorParamGuard.
               Questo E' il tentativo del boss del piano bersaglio: da qui in
               poi quel boss e' sconfitto, non ci sara' mai un secondo
               tentativo per lo stesso piano -- l'esito si decide ORA, per
               sempre (superata se pulito, fallita altrimenti: mai "ancora in
               corso" dopo questo momento). */
            t->state = game->currentBossFightDamaged ? TRIAL_FAILED : TRIAL_PASSED;
        }
        if (kind == ROOM_BOSS && t->kind == TRIAL_FLOOR_UNDER_TIME && t->param == game->floor)
        {
            /* Stessa guardia sul piano bersaglio di sopra, stessa ragione:
               il boss di un piano diverso da quello che la prova chiede non
               deve toccarla. */
            float threshold = TRIAL_FLOOR_TIME_BASE_SECONDS
                             + TRIAL_FLOOR_TIME_PER_FLOOR_SECONDS*(float)game->floor;
            float elapsed = game->runElapsedSeconds - game->floorEntryElapsedSeconds;
            t->state = (elapsed <= threshold) ? TRIAL_PASSED : TRIAL_FAILED;
        }
        if (kind == ROOM_ARENA && t->kind == TRIAL_ARENA_WON)
        {
            /* WorldCheckRoomClear chiama qui SOLO quando l'arena e' stata
               'cleared' con la sfida davvero accettata (room->arenaActive
               vero): attraversarla senza accettare non passa mai da qui. */
            t->state = TRIAL_PASSED;
        }
    }
}

void TrialsOnSecretFound(Game *game)
{
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].kind == TRIAL_SECRET_FOUND && game->trials[i].state == TRIAL_IN_PROGRESS)
            game->trials[i].state = TRIAL_PASSED;
}

void TrialsOnTimedRoomWithinThreshold(Game *game)
{
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].kind == TRIAL_TIMED_ROOM_WITHIN_THRESHOLD && game->trials[i].state == TRIAL_IN_PROGRESS)
            game->trials[i].state = TRIAL_PASSED;
}

void TrialsOnFusionPerformed(Game *game)
{
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].kind == TRIAL_FUSE_ITEM && game->trials[i].state == TRIAL_IN_PROGRESS)
            game->trials[i].state = TRIAL_PASSED;
}

void TrialsOnShopPurchase(Game *game)
{
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].kind == TRIAL_NO_SHOP_PURCHASE && game->trials[i].state == TRIAL_IN_PROGRESS)
            game->trials[i].state = TRIAL_FAILED;
}

void TrialsFinalizeAtRunEnd(Game *game)
{
    for (int i = 0; i < game->trialCount; i++)
    {
        Trial *t = &game->trials[i];
        if (t->state != TRIAL_IN_PROGRESS) continue;
        if (t->kind == TRIAL_END_WITH_INGOTS)
            t->state = (game->player.coins >= t->param) ? TRIAL_PASSED : TRIAL_FAILED;
        else if (t->kind == TRIAL_NO_SHOP_PURCHASE)
            /* Mai fallita finora (TrialsOnShopPurchase l'avrebbe gia' chiusa
               subito): la run e' finita senza un solo acquisto, superata. */
            t->state = TRIAL_PASSED;
        /* WP16, seconda tornata (rewards-and-economy.md, "Casi limite": "una
           prova ... risulta impossibile ... va scartata"): questi tre tipi
           dipendono da un archetipo NON garantito per costruzione
           (--rooms-test: la stanza a tempo manca in circa 1 piano su 5 fra i
           candidati, la segreta normale in circa 1 su 10, l'arena e' rara --
           vedi docs/engineering/known-issues.md voce 15). Se il suo
           Game.*EverGenerated e' rimasto falso, l'archetipo non e' MAI
           comparso in nessun piano di questa run: la prova non ha mai avuto
           un'occasione vera, quindi si scarta (TRIAL_VOID) invece di fallire
           -- non deve mai negare i punti gia' maturati dalle altre prove
           (stesso caso limite). Se invece l'archetipo E' comparso ma la
           prova e' rimasta IN_PROGRESS fino a qui, e' un tentativo mancato
           per davvero: TRIAL_FAILED, come ogni altro tipo sotto. */
        else if (t->kind == TRIAL_SECRET_FOUND)
            t->state = game->secretRoomEverGenerated ? TRIAL_FAILED : TRIAL_VOID;
        else if (t->kind == TRIAL_ARENA_WON)
            t->state = game->arenaRoomEverGenerated ? TRIAL_FAILED : TRIAL_VOID;
        else if (t->kind == TRIAL_TIMED_ROOM_WITHIN_THRESHOLD)
            t->state = game->timedRoomEverGenerated ? TRIAL_FAILED : TRIAL_VOID;
        else
            /* Ogni altro tipo ancora IN_PROGRESS (boss senza danno, piano
               sotto soglia): la run e' finita, nessun evento futuro potra'
               piu' soddisfarlo -- qui l'impossibilita' e' "la run stessa e'
               finita", il caso limite piu' semplice di tutti (il piano
               bersaglio esiste sempre per costruzione, vedi il commento su
               TrialsAssignForRun in trials.h: non e' mai il caso "mai
               offerta" gestito sopra). */
            t->state = TRIAL_FAILED;
    }
}

int TrialsPassedCount(const Game *game)
{
    int n = 0;
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].state == TRIAL_PASSED) n++;
    return n;
}

int TrialsBonusTotal(const Game *game)
{
    int total = 0;
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].state == TRIAL_PASSED) total += game->trials[i].bonus;
    return total;
}

int TrialsCountedTotal(const Game *game)
{
    int voided = 0;
    for (int i = 0; i < game->trialCount; i++)
        if (game->trials[i].state == TRIAL_VOID) voided++;
    return game->trialCount - voided;
}

const char *TrialStateLabel(TrialState state)
{
    switch (state)
    {
        case TRIAL_IN_PROGRESS: return "in corso";
        case TRIAL_PASSED:      return "superata";
        case TRIAL_FAILED:      return "fallita";
        case TRIAL_VOID:        return "annullata";
    }
    return "in corso";
}
