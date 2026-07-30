#ifndef MELTING_RUN_TRIALS_H
#define MELTING_RUN_TRIALS_H

#include "core/game_types.h"

/* WP16 (DEC-042/DEC-027, docs/design/systems/rewards-and-economy.md "Prove
   specifiche della run", docs/design/systems/floor-zero.md "Presentazione
   delle prove"): catalogo CURATO e deterministico delle prove, la loro
   assegnazione a inizio run e la loro verifica durante il gameplay.

   Bonus punti e soglie numeriche sono DEFAULT PROPOSTI DALL'IMPLEMENTAZIONE
   (stile DEC-019): nessun documento di design fissa questi numeri, solo che
   il canale bonus esiste (DEC-027). Registrati in
   docs/design/systems/rewards-and-economy.md e
   docs/design/governance/open-questions.md. */

/* Bonus punti per tipo di prova, alla stessa scala del resto dell'economia
   (WORLD_ROOM_CURRENCY_* in src/world/world.c va da 2 a 12): le prove
   valgono di piu' di una singola stanza perche' vincolano l'INTERA run, non
   un solo evento. TRIAL_NO_SHOP_PURCHASE vale piu' delle altre perche' e' la
   piu' facile da rovinare per distrazione (un acquisto d'impulso in un
   qualunque negozio di 5 piani) e la piu' difficile da recuperare (nessuna
   seconda occasione, a differenza di trovare UNA stanza segreta fra molte). */
#define TRIAL_BONUS_BOSS_NO_DAMAGE 25
#define TRIAL_BONUS_SECRET_FOUND 15
#define TRIAL_BONUS_ARENA_WON 20
#define TRIAL_BONUS_FLOOR_UNDER_TIME 15
#define TRIAL_BONUS_END_WITH_INGOTS 10
#define TRIAL_BONUS_FUSE_ITEM 10
#define TRIAL_BONUS_TIMED_ROOM 15
#define TRIAL_BONUS_NO_SHOP_PURCHASE 20

/* Soglia di TRIAL_END_WITH_INGOTS: poco sopra un acquisto comune (8 Ingots,
   DEC-026) piu' margine, cosi' la prova chiede di ARRIVARE a fine run con
   una riserva vera, non di sfiorarla per un istante. */
#define TRIAL_END_INGOTS_TARGET 30

/* Soglia di TRIAL_FLOOR_UNDER_TIME, in secondi dall'ingresso nel piano
   (Game.floorEntryElapsedSeconds, come la stanza a tempo di WP5): piu'
   larga della soglia della stanza a tempo (WORLD_TIMED_ROOM_THRESHOLD_*,
   src/world/world.c) perche' qui si chiede di completare l'INTERO piano
   (esplorazione + combattimenti + boss), non di raggiungere una sola
   stanza. Non dipende dalla taglia VERA del piano bersaglio
   (Game.floorCellCount) perche' al momento dell'assegnazione -- l'ingresso
   nel piano 1 -- i piani successivi non sono ancora generati: dipende solo
   dal numero di piano, che e' sempre gia' noto. */
#define TRIAL_FLOOR_TIME_BASE_SECONDS 90.0f
#define TRIAL_FLOOR_TIME_PER_FLOOR_SECONDS 25.0f

/* Assegna 'game->trialCount' prove (2 o 3, DEC-042) al catalogo di
   TrialKind, tutte di tipo DIVERSO, scelte da uno STREAM LOCALE derivato dal
   seed di RUN (game->runSeed), mai da game->rng: stesso seed -> stesse
   prove, sempre, indipendentemente da quante volte il flusso di gameplay ha
   gia' consumato game->rng per generare il piano 1 (chiamata PRIMA o DOPO
   non deve cambiare risultato). Chiamata da GameResetRunWithSeed
   (src/game/game.c), quindi sia il primo ingresso nel piano 1 sia un reset
   rapido R (che passa dalla stessa funzione con lo stesso runSeed)
   riassegnano le IDENTICHE prove, con lo stato ripulito da capo (il memset
   di GameResetRunWithSeed azzera 'trials'/'trialCount' PRIMA di questa
   chiamata).

   Esclusione dei parametri non verificabili (rewards-and-economy.md,
   "Casi limite": "una prova ... risulta impossibile ... va scartata"): per
   TRIAL_BOSS_NO_DAMAGE/TRIAL_FLOOR_UNDER_TIME il catalogo qui sotto sceglie
   solo parametri (numero di piano) dentro [1, FLOOR_COUNT], che esiste
   SEMPRE per costruzione (ogni piano ha sempre una stanza boss,
   WorldGenerateFloorMap non esce mai senza piazzarla) -- per questi due tipi
   il vincolo non ha oggi alcun caso su cui attivarsi QUI, ma resta scritto
   perche' e' un requisito della decisione (DEC-042).
   Per TRIAL_SECRET_FOUND/TRIAL_ARENA_WON/TRIAL_TIMED_ROOM_WITHIN_THRESHOLD
   il caso ESISTE davvero (nessuno dei tre archetipi e' garantito per
   costruzione, misure di --rooms-test in docs/engineering/known-issues.md
   voce 15) ma non e' verificabile QUI: con la generazione pigra dei piani
   (Step B2) questa funzione gira subito dopo WorldStartFloor(1), quando i
   piani 2..FLOOR_COUNT non sono ancora generati -- non esiste modo di sapere
   in anticipo se un archetipo comparira' piu' avanti nella run senza
   generare quei piani per davvero (che sposterebbe game->rng e romperebbe la
   pigrizia della generazione). L'esclusione per questi tre tipi si applica
   quindi A POSTERIORI, a fine run: vedi TrialsFinalizeAtRunEnd sotto e
   Game.timedRoomEverGenerated/secretRoomEverGenerated/arenaRoomEverGenerated
   (core/game_types.h).

   Accoda anche la presentazione (una card di scoperta per prova, lo stesso
   componente di sistema gia' usato per boss/nemici incontrati, DEC-065):
   questa funzione E' il momento della presentazione (floor-zero.md,
   "al passaggio dal Piano 0 al piano 1"), non un passo separato. */
void TrialsAssignForRun(Game *game);

/* Azzera Game.currentBossFightDamaged: chiamata da WorldSpawnRoomContents al
   PRIMO ingresso nella stanza boss del piano corrente, finche' non e'
   ancora stata ripulita -- l'inizio di un tentativo "pulito". */
void TrialsOnBossRoomEntered(Game *game);

/* Segna Game.currentBossFightDamaged se il giocatore e' DENTRO la stanza
   boss del piano corrente e quella stanza non e' ancora ripulita.
   Chiamata da CombatDamagePlayer per OGNI colpo davvero incassato (Crust
   compreso, DEC-159): fuori da un combattimento contro il boss e' un
   no-op. */
void TrialsOnPlayerDamaged(Game *game);

/* Aggiorna lo stato delle prove che dipendono dal completamento di UNA
   stanza specifica (TRIAL_BOSS_NO_DAMAGE, TRIAL_FLOOR_UNDER_TIME per
   ROOM_BOSS; TRIAL_ARENA_WON per ROOM_ARENA). Chiamata da WorldCheckRoomClear
   subito dopo che 'room->cleared' diventa vero, con 'kind' il RoomKind
   appena ripulito. */
void TrialsOnRoomCleared(Game *game, RoomKind kind);

/* Segna TRIAL_SECRET_FOUND superata. Chiamata da WorldSpawnRoomContents al
   PRIMO ingresso in una stanza segreta (normale o super, DEC-025): "trovata"
   e' esattamente questo, la stessa condizione di DEC-167 per la valuta. */
void TrialsOnSecretFound(Game *game);

/* Segna TRIAL_TIMED_ROOM_WITHIN_THRESHOLD superata. Chiamata da
   WorldSpawnRoomContents quando una stanza a tempo viene raggiunta ENTRO
   soglia al primo ingresso (la stessa condizione che paga la valuta di
   DEC-167 per questo archetipo, WP5). */
void TrialsOnTimedRoomWithinThreshold(Game *game);

/* Segna TRIAL_FUSE_ITEM superata. Chiamata da FusionPerform dopo una fusione
   riuscita. */
void TrialsOnFusionPerformed(Game *game);

/* Segna TRIAL_NO_SHOP_PURCHASE fallita DEFINITIVAMENTE: da questo momento
   "non aver mai comprato" e' diventato impossibile per il resto della run,
   qualunque cosa succeda dopo. Chiamata da CombatPickup quando un acquisto a
   pagamento (pickup->cost > 0, che nel motore accade solo nel negozio) va a
   buon fine. */
void TrialsOnShopPurchase(Game *game);

/* Chiamata ai tre punti in cui una run finisce: game->phase diventa
   PHASE_WIN in CombatPickup/PICKUP_EXIT, o PHASE_GAME_OVER in
   CombatDamagePlayer -- oppure (WP19, src/app/app.c, case APP_EXIT_CONFIRM)
   il giocatore conferma l'abbandono di una run VERA (game->floor >= 1): in
   quest'ultimo caso e' l'UNICO dei tre a essere chiamato A MANO invece che da
   dentro combat.c, e l'UNICO in cui game->phase NON diventa terminale
   (resta PHASE_PLAY) -- e' l'abbandono stesso a chiudere la run, non un
   evento di combattimento. Risolve ogni prova ancora TRIAL_IN_PROGRESS --
   TRIAL_END_WITH_INGOTS confrontando Player.coins con la soglia,
   TRIAL_NO_SHOP_PURCHASE come superata (non fallita finora = mai comprato
   per tutta la run). Per TRIAL_SECRET_FOUND/TRIAL_ARENA_WON/
   TRIAL_TIMED_ROOM_WITHIN_THRESHOLD ancora in corso: TRIAL_VOID se il
   relativo Game.*EverGenerated e' rimasto falso (l'archetipo non e' MAI
   comparso in nessun piano di questa run -- mai un'occasione vera,
   rewards-and-economy.md "Casi limite": "va scartata"), altrimenti
   TRIAL_FAILED (l'archetipo c'era, il giocatore non l'ha soddisfatta). Ogni
   altra prova ancora in corso diventa TRIAL_FAILED (la run e' finita: nessun
   evento futuro potra' piu' soddisfarla). Idempotente per costruzione: una
   prova gia' risolta (PASSED/FAILED/VOID) non viene mai toccata due volte,
   quindi chiamare questa funzione piu' volte non ha alcun effetto oltre la
   prima (i bonus, derivati SEMPRE da TrialsBonusTotal sotto, non hanno
   percio' bisogno di una guardia propria: "arrivano una volta sola" perche'
   lo stato sorgente cambia una volta sola). */
void TrialsFinalizeAtRunEnd(Game *game);

/* Quante delle 'game->trialCount' prove sono TRIAL_PASSED in questo momento
   (puo' crescere durante la run, mai diminuire). */
int TrialsPassedCount(const Game *game);

/* Somma dei bonus di tutte le prove TRIAL_PASSED in questo momento: il
   valore che RunResults/BuildScreen mostrano come "+X punti" (vedi il
   commento su TrialsFinalizeAtRunEnd sopra per la garanzia "una volta
   sola"). */
int TrialsBonusTotal(const Game *game);

/* Quante delle 'game->trialCount' prove CONTANO nel denominatore mostrato al
   giocatore ("N/M superate"): 'trialCount' meno le TRIAL_VOID (vedi
   TrialsFinalizeAtRunEnd sopra) -- una prova scartata perche' mai offerta non
   deve abbassare il rapporto visibile, coerente col "non deve mai negare i
   punti base gia' maturati" dello stesso caso limite. Prima della
   finalizzazione nessuna prova e' mai TRIAL_VOID, quindi coincide sempre con
   'trialCount' durante la run: la differenza compare solo a fine run. */
int TrialsCountedTotal(const Game *game);

/* Etichetta italiana breve dello stato ("in corso"/"superata"/"fallita"/
   "annullata"), per le righe di PauseMenu/BuildScreen -- unica fonte del
   testo, cosi' i due punti di consultazione (DEC-042) non possono mai
   divergere. */
const char *TrialStateLabel(TrialState state);

#endif
