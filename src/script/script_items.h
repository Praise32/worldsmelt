#ifndef MELTING_RUN_SCRIPT_ITEMS_H
#define MELTING_RUN_SCRIPT_ITEMS_H

/* ScriptItems: il confine pulito fra src/gameplay/ (combat.c) e la sandbox
   Lua (spec, docs/engineering/specs/2026-07-13-lua-sandbox-design.md,
   sezioni 5-9). QUESTO header non include mai lua.h ne' script_sandbox.h:
   Game.itemScripts (vedi core/game_types.h, ScriptItemRuntime) tiene il
   puntatore alla sandbox come void*, cosi' combat.c puo' chiamare le
   funzioni sotto senza mai vedere un tipo Lua (AGENTS.md: "la codice di
   combattimento chiama ScriptItems*, non tocca Lua"). */

#include "core/game_types.h"

/* Da chiamare ogni volta che il personaggio applicato al player e'
   (ri)deciso, DOPO che player.base* sono gia' impostati per quel personaggio
   (GamePlayerResetBaseStatsFor(player, character), vedi il chiamante):
   azzera i riferimenti cache di ogni slot oggetto, applica/scarica dietro la
   facciata il trait Lua del personaggio (M6b-2, DEC-037 --
   ScriptCharacterSetActive, src/script/script_character.h: 'character' NULL
   o senza trait = nessun trait attivo) e deriva player.damage/fireDelay/
   shotSpeed/shotRadius/speed/maxHp dai base* (0 oggetti posseduti, ma il
   trait SI' che gira, vedi ScriptItemsRecomputeStats sotto). 'character'
   deve essere lo STESSO passato a GamePlayerResetBaseStatsFor appena prima
   (NULL quando nessun personaggio e' applicato, come in GameResetRun). */
void ScriptItemsInit(Game *game, const CharacterDef *character);

/* Distrugge ogni ScriptSandbox viva, oggetti E il trait del personaggio
   (M6b-2: facciata su ScriptCharacterShutdown). Chiamata da GameUnloadAssets
   (vedi src/assets/game_assets.c): GameResetRun la richiama gia' come
   primissima riga, PRIMA del memset che azzererebbe altrimenti i puntatori
   senza liberare la memoria di Lua. Sicura da chiamare piu' volte/su uno
   stato gia' vuoto. */
void ScriptItemsShutdown(Game *game);

/* Da chiamare da CombatApplyItem subito dopo aver copiato l'oggetto in
   player.items[itemIndex]: se l'oggetto porta una sorgente Lua
   (item->luaSource[0] != '\0'), crea la sua sandbox, la registra con
   l'API di gioco (script_api.h) e prova a caricarla. Un fallimento e'
   silenzioso da qui (gia' loggato da script_sandbox.c): l'oggetto resta
   semplicemente "non attivo" (ScriptItemsHasActiveLua tornera' falso) e usa
   la sua mini-VM. Se lo slot aveva gia' una sandbox (oggetti in
   sovrannumero che riusano l'ultimo slot, vedi CombatApplyItem), la
   distrugge prima di sostituirla. Imposta Game.statsDirty. */
void ScriptItemsOnAcquire(Game *game, int itemIndex);

/* Vero se l'oggetto nello slot 'itemIndex' ha uno script Lua attualmente
   caricato con successo e non ancora disabilitato. Usata da
   src/gameplay/script_vm.c per SALTARE la mini-VM di quell'oggetto quando
   Lua la sta gia' gestendo: appena una sandbox viene disabilitata (patto di
   sicurezza) questa funzione torna falso dal frame successivo, e la mini-VM
   riprende da sola, senza alcuno switch esplicito da scrivere altrove. */
bool ScriptItemsHasActiveLua(const Game *game, int itemIndex);

/* Le tre callback per-evento (spec, sezione 7, "on_fire"/"on_hit"/
   "on_tick"): chiamano l'omonima funzione Lua di OGNI oggetto con Lua
   attivo (chi non la definisce viene saltato, non e' un errore), E -- M6b-2,
   facciata -- l'omonima callback del trait del personaggio, se attivo:
   combat.c non chiama mai ScriptCharacter* direttamente, solo queste tre
   funzioni. Chiamate ADDITIVAMENTE dal codice che gia' chiama
   ScriptVmExecutePlayer per la mini-VM (vedi src/gameplay/combat.c): non
   sostituiscono quella chiamata, la mini-VM continua a girare per gli
   oggetti che non hanno Lua attivo. shotIndex/enemyIndex sono indici grezzi
   nell'array C di Game: questo modulo li impacchetta in handle
   (script_api.h) prima di passarli allo script, cosi' combat.c non deve mai
   conoscere quell'encoding. */
void ScriptItemsOnFire(Game *game, Vector2 pos, Vector2 dir);
void ScriptItemsOnHit(Game *game, int shotIndex, int enemyIndex);
void ScriptItemsOnTick(Game *game, float dt);

/* Consuma Game.statsDirty: se e' vera chiama ScriptItemsRecomputeStats e la
   rimette a falso, altrimenti non fa nulla. Da chiamare una volta per
   frame, in cima a GameUpdate, PRIMA che CombatUpdatePlayer legga
   player.damage/fireDelay/... (vedi src/game/game.c). */
void ScriptItemsProcessDirty(Game *game);

/* Il sistema delle cache (spec, sezione 7, l'idea rubata a Isaac
   MC_EVALUATE_CACHE): riparte SEMPRE da player.base*, applica SUBITO -- M6b-2
   -- l'on_evaluate del trait del personaggio (se attivo: "parte del
   personaggio", prima di qualunque oggetto raccolto durante la run), poi
   riapplica, in ordine, per ciascun oggetto posseduto, prima i suoi
   modificatori "built-in" (trait/slot, la stessa matematica che prima viveva
   una tantum in CombatApplyItem) poi, se l'oggetto ha Lua attivo con
   on_evaluate, la tabella delle statistiche che lo script puo' modificare.
   Ogni passo e' seguito da un clamp ai confini di sicurezza, cosi' nessun
   oggetto - built-in o generato da un 7B che sbaglia i conti - puo' produrre
   un giocatore non giocabile, e nessuno script puo' far "accumulare" un
   modificatore ricalcolo dopo ricalcolo. Non 'static': i test dedicati del
   sistema delle cache la chiamano direttamente. */
void ScriptItemsRecomputeStats(Game *game);

#endif
