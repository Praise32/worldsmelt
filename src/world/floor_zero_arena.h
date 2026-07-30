#ifndef MELTING_RUN_FLOOR_ZERO_ARENA_H
#define MELTING_RUN_FLOOR_ZERO_ARENA_H

#include "core/game_types.h"

/* WP15a -- LE ARENE DI SFIDA DEL PIANO 0
   (docs/design/systems/floor-zero.md; DEC-004, DEC-047, DEC-055, DEC-092,
   DEC-093, DEC-094, DEC-095; systems/special-rooms.md, Scenario 2).

   Simulazioni OPZIONALI a rischio zero dentro il crogiolo: si entra da una
   piazzola segnalata, si combatte contro contenuti "best-of" delle run passate
   e si esce con ESATTAMENTE lo stato d'ingresso -- vittoria, sconfitta o
   abbandono che sia (DEC-092). Non e' una stanza del piano: il Piano 0 ha una
   sola cella (ROOM_HUB, src/world/floor_zero.c) e la simulazione riusa quella,
   sostituendone temporaneamente il contenuto.

   Le tre garanzie che questo modulo possiede, e che i test verificano:

   1. RIPRISTINO INTEGRALE (DEC-092). L'ingresso cattura una
      FloorZeroTrialSnapshot -- il Player copiato INTERO, piu' punteggio,
      stream RNG e messaggio -- e l'uscita la riapplica. Il Player si copia
      per intero e non campo per campo apposta: e' l'unico modo per cui una
      statistica aggiunta domani non possa restare fuori dal ripristino.
   2. NESSUNA ECONOMIA (DEC-093). Dentro non si guadagna e non si perde nulla
      che sopravviva: la stanza hub non ha una condizione di "ripulita"
      (ROOM_HUB non compare in WorldCheckRoomClear), quindi nessuna valuta di
      completamento e nessuna ricompensa vengono mai assegnate, e tutto cio'
      che il giocatore raccoglie DENTRO la simulazione (le risorse di pratica
      del tema RISORSE, gli oggetti del tema FUSIONE) sparisce col ripristino.
   3. MAI UN GAME OVER (DEC-055). La salute a zero dentro una simulazione
      scrive Game.floorZeroTrialDefeated invece di PHASE_GAME_OVER
      (CombatDamagePlayer, src/gameplay/combat.c); UpdateApp lo consuma e
      chiude la simulazione con un messaggio, senza mai chiudere la run.

   Il modulo NON conosce AppMode: l'ingresso passa da un latch
   (Game.floorZeroTrialRequest, scritto dal tasto di interazione e consumato da
   src/app/app.c), lo stesso schema di Game.fusionRoomTriggered. */

/* Piazza le piazzole d'arena nel crogiolo. Chiamata da FloorZeroEnter dopo
   l'arredo, e di nuovo da FloorZeroArenaExit quando la simulazione finisce.
   Deterministica e senza RNG: posizioni fisse sulla croce centrale libera
   (garanzia di RoomLayoutBuild), mai sopra un ostacolo dell'arredo. */
void FloorZeroArenaPlaceGates(Game *game);

/* Il tema della piazzola con cui il giocatore e' A CONTATTO, o -1. */
int FloorZeroArenaGateAtPlayer(const Game *game);

/* La CONFERMA esplicita di ingresso (Game.interactQueued, consumato da
   CombatUpdatePlayer). Vero SOLO se ha davvero latchato una richiesta: Piano 0,
   nessuna simulazione gia' in corso e giocatore a contatto con una piazzola.
   Non entra da sola nella simulazione -- scrive Game.floorZeroTrialRequest,
   che src/app/app.c consuma: e' li' che vive la memoria "questa piazzola l'ho
   gia' vista" del tutorial di DEC-047. */
bool FloorZeroArenaQueueEntry(Game *game);

/* Apre la simulazione del tema 'theme'. 'tutorial' vero mostra il cartello
   della PRIMA visita (DEC-047); falso lascia l'arena giocabile ma muta, come
   il documento chiede per le visite successive. */
void FloorZeroArenaEnter(Game *game, FloorZeroTrialTheme theme, bool tutorial);

/* Chiude la simulazione e ripristina lo stato d'ingresso. 'defeated' vero solo
   quando la salute e' scesa a zero DENTRO la simulazione (messaggio ironico,
   mai un game over). Sicura da chiamare quando nessuna simulazione e' in
   corso: non fa nulla. */
void FloorZeroArenaExit(Game *game, bool defeated);

/* Vero quando la simulazione in corso e' stata VINTA: aveva nemici e non ne
   resta nessuno. Falso quando nessuna simulazione e' in corso o quando la
   simulazione non aveva nemici da abbattere -- senza questa seconda guardia
   un'arena senza nemici si dichiarerebbe vinta al primo frame. */
bool FloorZeroArenaCleared(const Game *game);

/* Registra la vittoria e la annuncia UNA volta sola. NON chiude la prova: le
   prove del Piano 0 sono illimitate (DEC-095) e l'uscita e' sempre nelle mani
   del giocatore -- chiudere d'ufficio taglierebbe corta la lezione della
   piazzola FUSIONE, dove i nemici sono il contorno e la fucina e' il punto.
   Idempotente: chiamarla ad ogni frame non ripete l'annuncio. */
void FloorZeroArenaNoteVictory(Game *game);

/* L'etichetta breve del tema, per la piazzola e per i messaggi. Mai NULL. */
const char *FloorZeroArenaThemeLabel(FloorZeroTrialTheme theme);

#endif
