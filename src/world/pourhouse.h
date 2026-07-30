#ifndef MELTING_RUN_POURHOUSE_H
#define MELTING_RUN_POURHOUSE_H

#include "core/game_types.h"

/* WP7 -- la Pourhouse, «Casa della Colata» (DEC-136): lo scambio ad alto
   rischio di docs/design/systems/special-rooms.md, l'UNICO luogo dove sono
   ammessi patti a costo salute (DEC-026).
 *
 * Questo modulo possiede LA PUNTATA (DEC-044): la composizione deterministica
 * della coppia offerta/prezzo, la sua validazione contro lo stato del
 * giocatore, i testi leggibili e l'applicazione atomica all'accettazione. La
 * stanza in se' (piazzamento, arredo, messaggi d'ingresso) resta di
 * src/world/world.c, che chiama qui.
 *
 * NIENTE MODELLO A RUNTIME (DEC-171): nella demo nessun modello gira mentre si
 * gioca, quindi "generata dall'IA" si realizza come COMPOSIZIONE deterministica
 * dal seed di run + piano + cella -- la stessa disciplina di FusionKey e delle
 * sinergie (DEC-161). Due run con lo stesso seme, giocate allo stesso modo,
 * trovano la stessa puntata.
 *
 * ============================================================
 * TABELLA DI EQUIVALENZA (il "budget di equita'" di DEC-044)
 * ============================================================
 * Tutto si misura in PUNTI DI EQUITA', ancorati alla valuta principale:
 * 1 Ingot = 1 punto. Gli altri valori sono ancorati ai prezzi che il negozio
 * gia' pratica (src/gameplay/item_traits.c e src/world/world.c), cosi' la
 * Pourhouse non inventa una seconda economia parallela:
 *
 *   | voce                                   | punti |
 *   |----------------------------------------|-------|
 *   | 1 Ingot (valuta principale)            |     1 |
 *   | 1 punto di salute immediata            |     4 |
 *   | 1 punto di salute MASSIMA (il tetto)   |    14 |
 *   | 1 punto di Crust                       |    12 |
 *   | 1 strumento di breccia (Blast Charge)  |     4 |
 *   | 1 strumento di apertura (Cast Key)     |     5 |
 *   | 1 catalizzatore di fusione (Flux)      |    30 |
 *   | 1 oggetto, per rarita'                 | 8/16/28/45 |
 *
 * La riga degli oggetti NON e' un numero nuovo: e' esattamente
 * ItemShopCostForRarity (8/16/28/45), l'unica fonte dei prezzi per fascia di
 * rarita' (DEC-026). La salute MASSIMA vale piu' del triplo della salute
 * immediata perche' il tetto non si ricompra: e' l'unica voce davvero
 * irreversibile della tavola.
 *
 * DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE (stile DEC-019): DEC-044 fissa il
 * PRINCIPIO del budget di equita' e le categorie ammesse, non i numeri --
 * registrati in docs/design/systems/rewards-and-economy.md e
 * docs/design/governance/open-questions.md, da confermare al playtest.
 * ============================================================ */

#define POURHOUSE_VALUE_COIN     1
#define POURHOUSE_VALUE_HP       4
#define POURHOUSE_VALUE_MAX_HP  14
#define POURHOUSE_VALUE_CRUST   12
#define POURHOUSE_VALUE_BOMB     4
#define POURHOUSE_VALUE_KEY      5
#define POURHOUSE_VALUE_FLUX    30

/* Il budget di equita' vero e proprio: |valore offerta - valore prezzo| deve
   restare entro questa tolleranza, altrimenti la coppia e' respinta e non
   viene MAI proposta al giocatore (special-rooms.md, "Regole per contenuti
   generati" + "Casi limite"). Percentuale sul valore dell'offerta, con un
   minimo assoluto perche' sulle offerte piccole una percentuale sola sarebbe
   piu' stretta della granularita' delle risorse (un punto di salute vale 4,
   quindi sotto i 20 punti nessun prezzo in salute passerebbe mai). */
#define POURHOUSE_EQUITY_TOLERANCE_PERCENT 20
#define POURHOUSE_EQUITY_TOLERANCE_MIN      4

/* Il tetto di salute non scende MAI sotto un cuore (2 punti vita: la
   granularita' dei cuori dell'HUD, DrawHearts in src/render/game_renderer.c).
   E' il limite duro chiesto dalla specifica del lavoro, applicato in
   validazione -- una puntata che lo sforerebbe non viene proposta, non viene
   "corretta" al momento di pagare. */
#define POURHOUSE_MIN_BASE_MAX_HP 2

/* Compone la puntata della Pourhouse in (roomX,roomY) per lo stato ATTUALE del
   giocatore e la scrive in 'out' (mai un'uscita a meta': 'out' e' sempre
   completamente scritto). 'out->valid' falso significa che nessuna delle
   coppie candidate e' insieme equa E pagabile: la stanza offre allora la sola
   uscita libera.
   PURA rispetto a Game (non lo modifica): il chiamante decide se e quando
   registrarne la firma. */
void WorldComposePourhouseWager(const Game *game, int roomX, int roomY, PourhouseWager *out);

/* Vero se il prezzo di 'w' e' pagabile dal giocatore ADESSO. Ri-controllato
   all'accettazione, non solo alla composizione: fra i due momenti il giocatore
   puo' aver speso, perso salute o scambiato l'oggetto richiesto. */
bool WorldPourhousePricePayable(const Game *game, const PourhouseWager *w);
/* Vero se l'offerta di 'w' e' consegnabile INTERAMENTE adesso (inventario non
   pieno per un oggetto, Crust che non sfora il proprio tetto). Serve
   all'atomicita': meglio non concludere che incassare un prezzo per
   un'offerta consegnata a meta'. */
bool WorldPourhouseOfferDeliverable(const Game *game, const PourhouseWager *w);

/* I due testi leggibili della puntata, scritti per esteso -- DEC-058 vieta di
   affidare un'informazione al solo colore, e qui il giocatore deve poter
   leggere COSA riceve e COSA versa PRIMA di accettare. Scrivono sempre una
   stringa terminata, anche per una puntata non valida. */
void WorldPourhouseOfferText(const PourhouseWager *w, char *out, int cap);
void WorldPourhousePriceText(const PourhouseWager *w, char *out, int cap);

/* La firma compatta di una puntata (categorie + quantita'): due puntate con la
   stessa firma sono "la stessa puntata" ai fini dello Scenario 8. 0 per una
   puntata non valida. */
unsigned int WorldPourhouseSignature(const PourhouseWager *w);

/* Prepara la stanza corrente (che DEVE essere una ROOM_POURHOUSE): compone la
   puntata se serve, materializza il banco e sceglie il messaggio d'ingresso.
   Chiamata da WorldSpawnRoomContents ad OGNI ingresso. */
void WorldPourhousePrepareRoom(Game *game);

/* La conferma esplicita della puntata (Game.interactQueued, consumato da
   CombatUpdatePlayer). Vero SOLO se la puntata e' stata davvero accettata:
   stanza ROOM_POURHOUSE, puntata valida e non gia' accettata, giocatore a
   contatto col banco, prezzo ancora pagabile e offerta ancora consegnabile.
   In ogni altro caso torna falso e NON tocca nulla del giocatore: rifiutare o
   fallire non costa mai niente (special-rooms.md, Scenario 3). */
bool WorldTryAcceptPourhouseWager(Game *game);

#endif
