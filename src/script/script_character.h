#ifndef MELTING_RUN_SCRIPT_CHARACTER_H
#define MELTING_RUN_SCRIPT_CHARACTER_H

/* ScriptCharacter: la sandbox Lua del trait UNICO del personaggio GENERATO
   per questa run (M6b-2, DEC-037 -- seconda fetta del personaggio generato,
   dopo M6b-1 che ha portato stats+palette+carta). STESSA pipeline degli
   oggetti (script_sandbox.h invariato, nessun ampliamento dell'allowlist
   _ENV, vedi tools/melting-gen/gen_lua.c) e STESSO stile di script_items.c
   (ricalcolo sempre da zero, cache di riferimenti Lua, fallimento
   silenzioso mai un crash), ma con UNA sola sandbox indipendente
   dall'inventario: il trait non e' un oggetto, non occupa uno slot di
   Player.items[], non ha layer visivi di build screen -- vive e muore con
   la SELEZIONE del personaggio (il quarto slot dinamico del pannello
   PERSONAGGI del Piano 0), non con un pickup durante la run.

   QUESTO header non include mai lua.h ne' script_sandbox.h (stesso confine
   di script_items.h): Game.characterTrait (core/game_types.h,
   ScriptCharacterRuntime) tiene il puntatore alla sandbox come void*, cosi'
   i chiamanti di questo modulo non devono mai vedere un tipo Lua.

   Facciata (AGENTS.md, src/gameplay/combat.c chiama SOLO ScriptItems*): le
   funzioni sotto sono chiamate ESCLUSIVAMENTE da src/script/script_items.c
   (mai direttamente da combat.c/game.c/app.c, a parte
   ScriptCharacterSetActive, che src/script/script_items.c invoca a sua
   volta da ScriptItemsInit -- vedi il commento li'), cosi' "il trait sta
   dietro la facciata degli oggetti" resta vero anche per il ciclo di vita,
   non solo per le callback per-evento. */

#include "core/game_types.h"

/* Azzera lo stato del trait (sandbox=NULL, ogni *Ref a "nessun riferimento"):
   NON distrugge nulla, e' per una struttura gia' vuota (es. dopo un memset
   di Game). Il chiamante che ha una sandbox VIVA deve passare da
   ScriptCharacterShutdown sotto, mai direttamente da qui -- altrimenti la
   memoria di quel lua_State non verrebbe mai liberata (stesso patto di
   ScriptItemsInit/ScriptItemsShutdown in script_items.h). */
void ScriptCharacterInit(Game *game);

/* Distrugge la sandbox del trait se viva e azzera lo stato. Sicura da
   chiamare piu' volte/su uno stato gia' vuoto. Chiamata da
   ScriptItemsShutdown (facciata): ogni punto che gia' chiama
   ScriptItemsShutdown (GameUnloadAssets, FloorZeroEnter) libera cosi' ANCHE
   il trait, senza un nuovo call-site da imparare. */
void ScriptCharacterShutdown(Game *game);

/* Applica il trait del personaggio 'character' -- SEMPRE scaricando prima
   qualunque trait precedente (ScriptCharacterShutdown), poi, SOLO se
   'character' non e' NULL e character->traitHook non e' vuoto (il segnale
   che QUESTO personaggio e' quello generato per questa run E ha un trait
   valido: vedi il commento sul campo in core/game_types.h), caricando da
   generated/scripts/character_trait.lua -- lo STESSO percorso fisso per
   ogni run, mai derivato da 'character': un personaggio della rosa curata
   non ha mai traitHook impostato (zero-default degli array const di
   src/content/character_roster.c), quindi non tenta MAI di caricare un
   file che potrebbe ancora esistere sul disco da una run generata
   precedente -- e' esattamente cio' che impedisce a un trait "orfano" di
   attaccarsi al personaggio sbagliato.

   Fallimento di lettura/compilazione (file sparito dopo la proposta, script
   che non compila piu'...): silenziosamente inattivo, mai un crash
   (ScriptCharacterHasActiveLua tornera' falso) -- il giocatore vede al
   massimo un personaggio senza trait, esattamente come un oggetto la cui
   sandbox e' stata disabilitata.

   Chiamata da ScriptItemsInit (facciata, script_items.c): OGNI punto che
   gia' applica GamePlayerResetBaseStatsFor(character) seguito da
   ScriptItemsInit (FloorZeroEnter, AppConfirmCharacterChoice, il case
   APP_FLOOR_ZERO/floorZeroExitCrossed in app.c, GameUpdate/resetQueued in
   game.c) applica cosi' ANCHE il trait, senza un nuovo call-site da
   imparare -- stesso principio di ScriptCharacterShutdown sopra. Sempre
   ricaricato da zero anche per lo STESSO personaggio (mai un confronto
   "e' gia' questo, salto"): coerente col resto del sistema (ricalcolo
   sempre da zero) e rende innocuo un file cambiato sul disco fra due
   selezioni. */
void ScriptCharacterSetActive(Game *game, const CharacterDef *character);

/* Vero se il trait e' attualmente caricato con successo e non ancora
   disabilitato (patto di sicurezza della sandbox). */
bool ScriptCharacterHasActiveLua(const Game *game);

/* Le tre callback per-evento, chiamate ADDITIVAMENTE da ScriptItemsOnFire/
   OnHit/OnTick (script_items.c, facciata): se il trait non definisce quella
   callback, o non e' attivo, non fanno nulla -- non e' un errore, un trait
   puo' benissimo essere un solo on_evaluate (vedi sotto). */
void ScriptCharacterOnFire(Game *game, Vector2 pos, Vector2 dir);
void ScriptCharacterOnHit(Game *game, int shotIndex, int enemyIndex);
void ScriptCharacterOnTick(Game *game, float dt);

/* Esegue on_evaluate(stats) del trait (se presente e attivo) sui SETTE campi
   passati per puntatore, leggendo i valori CORRENTI (scritti dal chiamante
   PRIMA di questa chiamata) e riscrivendoli con l'esito -- stessa tabella di
   scratch riusata/stessa logica NaN-safe di ScriptItemsCallEvaluate
   (script_items.c), duplicata QUI volutamente invece di condivisa: il trait
   ha una sola sandbox indipendente dagli oggetti, non uno slot di un array,
   e i due moduli non condividono ScriptItemsStatsAccum (privata a
   script_items.c) apposta, per non far trapelare quel tipo attraverso il
   confine del modulo -- primitivi in ingresso/uscita, come ogni altra
   funzione di questa facciata (ScriptItemsOnFire/OnHit/OnTick sopra).

   Ritorna false se il trait non ha on_evaluate, non e' attivo, o la
   chiamata e' fallita (sandbox appena disabilitata dal patto di sicurezza):
   in OGNI caso di ritorno falso i sette campi NON vengono toccati, cosi' il
   chiamante (ScriptItemsRecomputeStats) puo' trattare "nessun trait" e "il
   trait e' appena stato ucciso a meta' della sua stessa chiamata"
   esattamente allo stesso modo -- 'acc' resta quello che era prima di
   questa chiamata, pronto per il clamp/il resto del ricalcolo. */
bool ScriptCharacterEvaluate(Game *game, float *damage, float *fireDelay, float *shotSpeed,
                              float *shotRadius, float *speed, float *maxHp, float *luck);

#endif
