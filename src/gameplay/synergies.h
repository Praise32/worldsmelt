#ifndef MELTING_RUN_SYNERGIES_H
#define MELTING_RUN_SYNERGIES_H

#include "core/game_types.h"

/* Sinergie IMPLICITE alla Isaac (step D, docs/references/research/design-sinergie.md,
   sezione 6 "la prima versione minima"; decisione di alto livello gia' presa
   nella vision doc, sezione 4: modello A, implicito).
 *
 * COSA VUOL DIRE: gli oggetti restano separati nell'inventario (non si fondono,
 * non si consumano). Quando l'inventario contiene una COPPIA compatibile, il
 * gioco AGGIUNGE un effetto combinato. La profondita' non nasce dal singolo
 * oggetto -- che resta semplice e leggibile, un solo effetto -- ma dalle
 * combinazioni man mano che la build cresce.
 *
 * PERCHE' E' POCO CODICE: il motore aveva gia' entrambi i meccanismi che in Isaac
 * rendono le sinergie componibili per costruzione (vedi il design doc, sezione 3):
 *   1. il ricalcolo-da-zero delle statistiche (ScriptItemsRecomputeStats) = la
 *      "stats cache" di Isaac. Una sinergia e' solo UN CONTRIBUTO CONDIZIONALE IN
 *      PIU' dentro quel ricalcolo, quindi e' idempotente e senza deriva per
 *      costruzione: togliere un oggetto della coppia la spegne pulita, senza
 *      alcuna contabilita' da disfare.
 *   2. i trait bitmask sui colpi (Shot.traits) = le "tear flags". Accendere un bit
 *      su un colpo appena creato = accendere una sinergia comportamentale.
 * Questo modulo e' quindi soprattutto CABLAGGIO fra due punti che esistevano gia'.
 *
 * I DUE CANALI (design doc, sezione 4.3):
 *   Canale A -- STATISTICO: SynergiesStatBonus(), applicato dentro
 *     ScriptItemsRecomputeStats DOPO i contributi dei singoli oggetti e PRIMA dei
 *     clamp finali. Passa quindi per gli stessi tetti di sicurezza di tutto il
 *     resto: nemmeno una sinergia sbagliata puo' portare il giocatore fuori banda.
 *   Canale B -- COMPORTAMENTALE: SynergiesApplyToShot()/SynergiesExtraPellets(),
 *     applicati alla creazione del colpo del giocatore in combat.c.
 *
 * PERCHE' NON C'E' Item.archetype (deviazione consapevole dal design doc, sezione
 * 4.1): quel campo era stato proposto per colmare un buco -- "l'archetipo di un
 * oggetto vive solo nel prompt, a runtime non e' persistito". Lo step C quel buco
 * l'ha gia' chiuso da un'altra parte: oggi un oggetto porta i suoi 'traits'
 * (segnale primario, scritto dal modello, gia' persistito) E il suo ShotTypeDef
 * (forma + manopole, scritto dal modello, persistito nel manifest). Le condizioni
 * qui sotto sono espresse su QUESTI due segnali, che sono reali e generati, invece
 * che su un terzo campo derivato che nessuno saprebbe riempire meglio. Se un
 * giorno le sinergie avranno bisogno di distinguere "famiglio" da "esplosivo" (che
 * i trait non catturano), l'archetipo tornera' utile: il punto di applicazione
 * (questo modulo) e' gia' pronto e non cambia. */

/* Le sinergie canoniche di questa prima versione. L'ordine e' l'indice del bit
   nella maschera (Player.synergies): SYNERGY_COUNT <= 32. MODIFICA QUI (e nella
   tavola in synergies.c) per aggiungerne una: e' una riga, come per le tavole di
   rarita'. */
typedef enum SynergyId {
    SYNERGY_PIERCING_FLIGHT = 0,   /* inseguimento + perforazione */
    SYNERGY_UNSTABLE_BOUNCE,       /* rimbalzo + esplosione */
    SYNERGY_SWARM,                 /* divisione + cadenza */
    SYNERGY_ETERNAL_FROST,         /* rallentamento + cadenza */
    SYNERGY_RAVENOUS_BITE,         /* furto di vita + colpi giganti */
    SYNERGY_VOLTAIC_ARC,           /* un tipo di colpo che SALTA + rallentamento */
    SYNERGY_COUNT
} SynergyId;

/* Contributo STATISTICO di tutte le sinergie attive (canale A). I moltiplicatori
   valgono 1.0 e luckAdd 0.0 quando non c'e' alcuna sinergia: applicarli e' sempre
   sicuro, anche a mani vuote. */
typedef struct SynergyStatBonus {
    float damageMul;
    float fireDelayMul;
    float shotSpeedMul;
    float luckAdd;
} SynergyStatBonus;

/* La maschera delle sinergie attive per l'inventario di 'player', calcolata da
   ZERO sugli oggetti posseduti (mai da Player.traits, che e' un OR monotono e non
   si spegne mai: usarlo qui renderebbe impossibile spegnere una sinergia
   togliendo un oggetto -- il criterio 5 dei test del design doc). Bit i-esimo =
   SynergyId i-esima attiva. */
unsigned int SynergiesDetect(const Player *player);

/* Canale A. 'mask' e' quella appena calcolata da SynergiesDetect (il chiamante ce
   l'ha gia': non la si ricalcola). */
SynergyStatBonus SynergiesStatBonus(const Player *player, unsigned int mask);

/* Canale B, parte "per colpo": accende i trait e alza pierce/bounces/chain del
   colpo appena creato. Idempotente sui bit (un OR), e i contributi numerici sono
   piccoli e limitati dalla tavola. */
void SynergiesApplyToShot(const Player *player, unsigned int mask, Shot *shot);

/* Canale B, parte "per sparo": pallettoni in piu' (non e' una proprieta' del
   singolo colpo, e' quanti colpi nascono). */
int SynergiesExtraPellets(unsigned int mask);

/* Nome e descrizione brevi, per la GUI (pannello LOG) e per il messaggio che
   annuncia una sinergia appena sbloccata. Indice fuori range -> stringa vuota. */
const char *SynergyName(int id);
const char *SynergyDescription(int id);

#endif
