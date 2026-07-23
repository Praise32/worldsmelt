#ifndef MELTING_RUN_SCRIPT_API_H
#define MELTING_RUN_SCRIPT_API_H

/* ScriptApi: l'API di gioco esposta dentro l'_ENV di una ScriptSandbox (spec,
   docs/engineering/specs/2026-07-13-lua-sandbox-design.md, sezione 5). Vive
   in src/script/ (non src/gameplay/): e' l'unico modulo, insieme a
   script_items.c, che tocca sia lua_State sia i tipi di gioco (Game, Enemy,
   Shot...) nello stesso file. src/gameplay/ non include mai lua.h (vedi
   AGENTS.md): chiama ScriptItems* (src/script/script_items.h), che non
   espone alcun tipo Lua nella propria firma pubblica.

   Decisione di design chiave (spec, sezione 5): Lua non vede MAI un
   puntatore. Riceve un HANDLE = indice nell'array C + generazione,
   impacchettati in un unico numero (i numeri Lua sono double, esatti fino a
   2^53: indice in 16 bit bassi, generazione nei bit sopra, ampio margine).
   Ogni funzione che riceve un handle lo valida PRIMA di toccare qualunque
   cosa (ScriptApiCheckEnemy/ScriptApiCheckShot sotto); un handle non valido
   solleva luaL_error, che il chiamante (ScriptSandboxCallVoid/
   ScriptSandboxProtectedCall) intercetta e traduce nell'uccisione
   permanente della sandbox (patto di sicurezza, spec sezione 9): non serve
   nessuna logica di "uccisione" qui dentro, basta luaL_error. */

#include "core/game_types.h"
#include "script/script_sandbox.h"

/* Registra tutte le funzioni di gioco nell'_ENV di 'sb' (lightuserdata
   'game' come unico upvalue di ciascuna, coerente con ScriptSandboxLuaRng
   per rng() in script_sandbox.c). Il puntatore Game* e' un lightuserdata
   VOLUTAMENTE, a differenza di enemy/shot: Game vive per l'intera run,
   allocato una sola volta dal chiamante (src/app/app.c) e mai spostato ne'
   liberato finche' la run non finisce, quindi non c'e' alcun rischio di
   use-after-free ad usarlo come puntatore grezzo (e' esattamente il
   controesempio che la spec, sezione 5, chiarisce: il pericolo del
   lightuserdata sono i puntatori a entita' che MUOIONO durante la run, non
   un singleton stabile). Da chiamare una volta sola, subito dopo
   ScriptSandboxCreate e PRIMA di ScriptSandboxLoad (cosi' il corpo di primo
   livello dello script puo' gia' vedere l'API, se vuole usarla li'). */
void ScriptApiRegister(ScriptSandbox *sb, Game *game);

/* Impacchetta/spacchetta un handle enemy/shot. Usate anche da
   script_items.c per costruire gli argomenti di on_hit(shot_id, enemy_id).
   Ritornano un numero non negativo; ScriptApiUnpackHandle torna false se il
   valore non puo' essere un handle valido (negativo, frazionario, o fuori
   range) SENZA sollevare errori Lua: e' compito del chiamante (le funzioni
   Check* sotto) decidere se questo e' un'emergenza da segnalare con
   luaL_error. */
double ScriptApiPackEnemyHandle(const Game *game, int index);
double ScriptApiPackShotHandle(const Game *game, int index);

#endif
