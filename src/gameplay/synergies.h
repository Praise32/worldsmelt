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
   SynergyId i-esima attiva. 'runSeed' e' Game.runSeed (DEC-141): quando piu' di
   un oggetto posseduto porta lo stesso segnale, DEC-161 lo usa per scegliere fra
   i candidati, vedi il commento su SynergyConflictAPrevails sotto. */
unsigned int SynergiesDetect(const Player *player, unsigned int runSeed);

/* Canale A. 'mask' e' quella appena calcolata da SynergiesDetect (il chiamante ce
   l'ha gia': non la si ricalcola). Stesso 'runSeed' di SynergiesDetect: RuleActive
   viene rivalutata qui dentro (vedi il commento nell'implementazione) e deve
   scegliere fra candidati multipli allo stesso modo, o la rarita' usata per
   scalare la potenza (SynergyRarityScale) potrebbe non corrispondere a quella
   vista da SynergiesDetect. */
SynergyStatBonus SynergiesStatBonus(const Player *player, unsigned int mask, unsigned int runSeed);

/* Canale B, parte "per colpo": accende i trait e alza pierce/bounces/chain del
   colpo appena creato. Idempotente sui bit (un OR), e i contributi numerici sono
   piccoli e limitati dalla tavola. Stesso 'runSeed' di sopra, stesso motivo. */
void SynergiesApplyToShot(const Player *player, unsigned int mask, unsigned int runSeed, Shot *shot);

/* DEC-161 (docs/design/systems/synergies.md, "Priorita'"): quando un conflitto
 * non e' risolto ne' da una trasformazione esplicita di fusione ne' da una
 * regola di sinergia implicita gia' definita -- qui, in pratica, la scelta fra
 * PIU' oggetti posseduti che portano lo STESSO segnale, per cui non esiste una
 * regola di design che dica quale "conta" per la coppia -- non esiste un ordine
 * fisso di priorita': l'esito si decide con un numero pseudo-casuale derivato
 * dal seed di run PIU' una chiave stabile della coppia in conflitto.
 *
 * Deliberatamente NON e' un consumo di 'game->rng' (quello stream avanza a ogni
 * lettura: usarlo qui darebbe un esito diverso ad ogni query nella STESSA run,
 * il contrario della stabilita' richiesta -- vedi Test Y in
 * script_items_tests.c, che pretende risultati identici su 100 ricalcoli di
 * fila). E' invece un hash puro (splitmix64, come GameplayRngSeedFromRunSeed in
 * src/game/game.c ma con una costante di dominio propria): a runSeed/keyA/keyB
 * fissi risponde sempre uguale, quante volte lo si chiami.
 *
 * Simmetrica: SynergyConflictAPrevails(seed, A, B) == !SynergyConflictAPrevails(seed, B, A)
 * per costruzione (la chiave di coppia ordina keyA/keyB prima di mescolare, poi
 * confronta il vincitore mescolato con l'argomento letterale 'keyA' ricevuto).
 * Ritorna true se 'keyA' prevale su 'keyB'. */
bool SynergyConflictAPrevails(unsigned int runSeed, int keyA, int keyB);

/* Canale B, parte "per sparo": pallettoni in piu' (non e' una proprieta' del
   singolo colpo, e' quanti colpi nascono). */
int SynergiesExtraPellets(unsigned int mask);

/* Nome e descrizione brevi, per la GUI (pannello LOG) e per il messaggio che
   annuncia una sinergia appena sbloccata. Indice fuori range -> stringa vuota. */
const char *SynergyName(int id);
const char *SynergyDescription(int id);

#endif
