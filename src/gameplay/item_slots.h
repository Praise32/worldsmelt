#ifndef MELTING_RUN_ITEM_SLOTS_H
#define MELTING_RUN_ITEM_SLOTS_H

#include "core/game_types.h"

/* Slot funzionali della tassonomia a 4 categorie
 * (docs/design/systems/items-pools-and-rarity.md "Slot",
 * systems/active-items.md, systems/grafts.md).
 *
 * L'idea che tiene insieme tutto il file: NON esiste un secondo inventario.
 * Attivi e Innesti vivono in Player.items[] come passivi e stat-up --
 * "equipaggiato" e' sinonimo di "posseduto" -- e questo modulo si limita a
 * DERIVARE, scorrendo items[] in ordine di acquisizione, quale oggetto
 * occupa quale slot. Nessun indice memorizzato da tenere allineato, quindi
 * nessun modo di disallinearlo: rimuovere un oggetto
 * (ScriptItemsRemoveItem) non richiede di aggiustare niente qui, ed e'
 * esattamente la stessa scelta di ScriptItemsRecomputeStats -- ricalcolare
 * da zero invece di mantenere una contabilita' incrementale.
 *
 * Conseguenza voluta: passivi e stat-up non hanno slot e non compaiono in
 * questo header. Il documento dice che si accumulano senza limite, ed e'
 * cio' che gia' fanno.
 *
 * Prefisso 'Item' come il modulo gemello item_traits.h (AGENTS.md: prefissi
 * di modulo, niente simboli globali generici). */

/* Cooldown di riserva del motore, in secondi, per un ITEM_ACTIVE che non
 * dichiara NE' cariche NE' cooldown. Il design pretende che un attivo ne
 * dichiari sempre uno (active-items.md); un contenuto generato o curato che
 * non lo fa non deve diventare un attivo usabile a ogni frame -- il caso
 * peggiore possibile -- quindi ricade qui. Scelto lungo: un attivo senza
 * dichiarazione e' un difetto di contenuto, non un premio. */
#define ITEM_ACTIVE_DEFAULT_COOLDOWN 12.0f

/* Quanti slot di ciascun tipo ha questo giocatore. Un valore <= 0 (Player
 * azzerato con memset: GameResetRun, la meta' dei test che costruiscono un
 * Game a mano) vale 1, il minimo di design, mai 0: lo slot iniziale e' una
 * garanzia del documento, non qualcosa che dipende da chi ha costruito il
 * Player. Il risultato e' sempre clampato a MAX_ACTIVE_SLOTS/
 * MAX_GRAFT_SLOTS. */
int ItemActiveSlotCount(const Player *p);
int ItemGraftSlotCount(const Player *p);

/* Quanti oggetti di quella categoria il giocatore possiede ORA. */
int ItemCountOfKind(const Player *p, ItemKind kind);

/* Indice in items[] dell'n-esimo oggetto di quella categoria in ordine di
 * acquisizione (n a base 0), oppure -1 se non ce ne sono abbastanza. */
int ItemIndexOfKind(const Player *p, ItemKind kind, int n);

/* Indice in items[] dell'attivo SELEZIONATO -- quello che risponde al tasto
 * d'uso e quello che uno scambio lascia sul piedistallo (DEC-117). -1 se il
 * giocatore non ha attivi. Player.activeSelected e' un ordinale fra gli
 * attivi posseduti e viene clampato qui: un ordinale rimasto oltre il numero
 * di attivi (l'attivo selezionato e' stato scambiato via) ricade
 * sull'ultimo, mai su -1 quando un attivo c'e'. */
int ItemSelectedActiveIndex(const Player *p);

/* Quale dei due modi di ricarica vale per questo oggetto (active-items.md:
 * ogni attivo ne dichiara esattamente uno). Le cariche VINCONO se un
 * contenuto malfatto dichiara entrambi: e' il modo piu' avaro dei due
 * (servono ricariche esterne, mentre il cooldown si ricarica da solo col
 * tempo), quindi un dato sbagliato rende l'oggetto piu' debole, mai piu'
 * forte -- la stessa regola di ScriptItemsRarityFraction. */
bool ItemActiveIsChargeBased(const Item *item);
bool ItemActiveIsCooldownBased(const Item *item);

/* Capienza in cariche, gia' clampata a una banda sensata. */
int ItemActiveChargeCapacity(const Item *item);

/* Secondi di cooldown effettivi (il valore dichiarato, oppure
 * ITEM_ACTIVE_DEFAULT_COOLDOWN se l'oggetto non dichiara nulla). */
float ItemActiveCooldownSeconds(const Item *item);

/* Vero se l'attivo si puo' usare in questo istante. */
bool ItemActiveIsReady(const Item *item);

/* Porta l'oggetto allo stato "appena trovato": cariche al massimo per un
 * attivo a cariche, nessuna attesa per uno a cooldown. Chiamata quando un
 * attivo entra in gioco da un contenuto (default proposto: un attivo si
 * trova CARICO, cosi' la prima decisione d'uso arriva subito e non dopo una
 * stanza di attesa). Non tocca gli oggetti di altre categorie. */
void ItemActiveResetCharge(Item *item);

/* DEC-059, i due canali di base della ricarica: aggiunge cariche a TUTTI gli
 * attivi a cariche posseduti, secondo il dosaggio dichiarato da ciascun
 * oggetto ('chargeGainRoom' per la stanza completata, 'chargeGainEnergy' per
 * l'energia raccolta). Gli attivi a cooldown non sono toccati: il loro
 * canale e' il tempo, e sommare i due modi renderebbe la dichiarazione
 * dell'oggetto una bugia. Ritorna quanti oggetti hanno davvero guadagnato
 * qualcosa (0 = nessuno, usato da chi decide se droppare energia). */
int ItemActivesGainRoomCharge(Player *p);
int ItemActivesGainEnergyCharge(Player *p);

/* Vero se almeno un attivo posseduto e' a cariche e non e' pieno: e' la
 * condizione che rende sensato far cadere energia da un nemico (DEC-059,
 * canale 2). Senza, il drop sarebbe rumore a schermo. */
bool ItemActivesWantEnergy(const Player *p);

/* Scorre il tempo per gli attivi a cooldown posseduti. */
void ItemActivesTickCooldown(Player *p, float dt);

/* Nome della categoria per l'interfaccia (italiano, come il resto del testo
 * a schermo). Non e' il vocabolario del manifest: quello vive in
 * ItemKindFromText/KindName (src/content/). */
const char *ItemKindLabel(ItemKind kind);

#endif
