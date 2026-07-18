#include "script/script_api.h"

#include "core/game_math.h"
#include "game/game_internal.h"

#include "lauxlib.h"
#include "lua.h"

#include <limits.h>
#include <math.h>
#include <string.h>

/* ============================================================
   Handle: indice + generazione impacchettati in un unico numero
   (spec, sezione 5). Vedi il commento lungo in script_api.h.
   ============================================================ */

#define SCRIPT_API_INDEX_BITS 16
#define SCRIPT_API_INDEX_MASK ((1u << SCRIPT_API_INDEX_BITS) - 1)

/* Tetto per ScriptApiUnpackHandle sotto: un handle vero e' "generazione (32
   bit) << 16 | indice (16 bit)", quindi non supera mai 2^48. Qualunque
   valore Lua oltre questo tetto e' gia' un handle invalido di suo (non
   corrisponde a nessuna combinazione indice/generazione producibile da
   ScriptApiPackEnemyHandle/ScriptApiPackShotHandle sopra): rifiutarlo qui,
   PRIMA del cast a unsigned long long piu' sotto, non esclude nessun
   handle legittimo. Un double rappresenta ogni intero fino a 2^53 in modo
   esatto, quindi il confronto sotto e' preciso, non approssimato. */
#define SCRIPT_API_HANDLE_MAX ((double)(1ull << 48))

double ScriptApiPackEnemyHandle(const Game *game, int index)
{
    unsigned long long packed = ((unsigned long long)game->enemyGen[index] << SCRIPT_API_INDEX_BITS)
                                 | ((unsigned int)index & SCRIPT_API_INDEX_MASK);
    return (double)packed;
}

double ScriptApiPackShotHandle(const Game *game, int index)
{
    unsigned long long packed = ((unsigned long long)game->shotGen[index] << SCRIPT_API_INDEX_BITS)
                                 | ((unsigned int)index & SCRIPT_API_INDEX_MASK);
    return (double)packed;
}

/* Spacchetta SENZA sollevare errori: un numero negativo o non intero non e'
   mai un handle valido, ma decidere se questo e' un'emergenza (luaL_error,
   patto di sicurezza) spetta al chiamante (ScriptApiCheckEnemy/Shot sotto),
   non a questa funzione pura. */
static bool ScriptApiUnpackHandle(double handle, int *indexOut, unsigned int *genOut)
{
    /* Il cast (unsigned long long)handle qui sotto e' undefined behaviour
       (C99 6.3.1.4) per qualunque double che il tipo di destinazione non
       possa rappresentare: enemy_x(1e300) supera sia "handle < 0" sia
       "floor(handle)==handle" (1e300 e' gia' un intero in virgola mobile),
       arrivando al cast senza altre difese. Sulla toolchain x86-64 di oggi
       l'effetto osservato e' innocuo (il valore risultante fallisce
       comunque i controlli di range del chiamante), ma UB resta UB: niente
       lo garantisce su un'altra architettura o un'altra versione del
       compilatore. Il controllo handle >= SCRIPT_API_HANDLE_MAX rifiuta
       questi valori PRIMA del cast, senza costare nessun handle vero. */
    if (handle < 0.0 || handle != floor(handle) || handle >= SCRIPT_API_HANDLE_MAX) return false;
    unsigned long long packed = (unsigned long long)handle;
    *indexOut = (int)(packed & SCRIPT_API_INDEX_MASK);
    *genOut = (unsigned int)(packed >> SCRIPT_API_INDEX_BITS);
    return true;
}

static Game *ScriptApiGame(lua_State *L)
{
    return (Game *)lua_touserdata(L, lua_upvalueindex(1));
}

/* Valida un handle nemico e ritorna il puntatore, oppure MAI: luaL_error fa
   longjmp fuori da questa funzione (non ritorna mai in caso di errore), che
   lua_pcall (dentro ScriptSandboxCallVoid/ScriptSandboxProtectedCall)
   intercetta come un qualunque errore a runtime -> la sandbox viene
   disabilitata in modo permanente (patto di sicurezza, spec sezione 9): "un
   handle non valido" e' letteralmente uno dei quattro motivi elencati li'.
   E' cosi' che TUTTE le funzioni sotto che ricevono un handle applicano la
   regola, senza duplicare la logica di uccisione. */
static Enemy *ScriptApiCheckEnemy(lua_State *L, Game *game, int argIdx)
{
    double handle = luaL_checknumber(L, argIdx);
    /* Inizializzati anche se ScriptApiUnpackHandle fallisce e non li tocca:
       luaL_error sotto non ritorna mai (longjmp), ma gcc non lo sa (nel
       nostro lauxlib.h vendorizzato luaL_error e' dichiarata "int", non
       _Noreturn) e segnalerebbe altrimenti un falso "maybe-uninitialized"
       sulla return finale. */
    int index = -1; unsigned int gen = 0;
    if (!ScriptApiUnpackHandle(handle, &index, &gen) || index < 0 || index >= MAX_ENEMIES
        || !game->enemies[index].active || game->enemyGen[index] != gen)
    {
        luaL_error(L, "handle nemico non valido");
    }
    return &game->enemies[index];
}

static Shot *ScriptApiCheckShot(lua_State *L, Game *game, int argIdx)
{
    double handle = luaL_checknumber(L, argIdx);
    int index = -1; unsigned int gen = 0;   /* vedi il commento in ScriptApiCheckEnemy sopra */
    if (!ScriptApiUnpackHandle(handle, &index, &gen) || index < 0 || index >= MAX_SHOTS
        || !game->shots[index].active || game->shotGen[index] != gen)
    {
        luaL_error(L, "handle colpo non valido");
    }
    return &game->shots[index];
}

/* ============================================================
   Letture
   ============================================================ */

/* Non nell'elenco letterale della task brief, ma un'aggiunta necessaria:
   senza di lei on_tick(dt) (nessun argomento di posizione) non avrebbe
   alcun modo di sapere DOVE si trova il giocatore (es. per un'aura
   periodica centrata su di lui), mentre on_fire(x,y,dx,dy) la posizione la
   riceve gia' come argomento.

   player_x/player_y (e allo stesso modo enemy_x/enemy_y, shot_x/shot_y
   sotto) sono DUE funzioni a un solo valore di ritorno ciascuna, non UNA
   funzione a due valori (player_pos() -> x, y come nella prima versione di
   questa API). Il motivo e' una trappola reale di Lua, non estetica: un
   valore multiplo si espande SOLO nell'ultima posizione di una chiamata
   (`f(player_pos())` espande entrambi), in QUALSIASI altra posizione viene
   troncato al primo valore senza errore ne' avviso
   (`f(player_pos(), 0)` passa SOLO px, py sparisce silenziosamente). Un
   modello da 7B scrive `spawn_shot(player_pos(), 0, 0, 1, 75, 3, 4, ...)"
   in modo del tutto naturale (osservato per davvero in un run generato:
   spara con y=0, il bordo alto della stanza, non la posizione del
   giocatore) e quello script e' Lua valido, passa il validatore, non va mai
   in crash: sbaglia in silenzio. Rendere ogni getter a un solo valore rende
   l'errore IRRAPPRESENTABILE piuttosto che documentarlo e sperare che il
   modello legga il cheat-sheet (prompts/lua_system.txt) con abbastanza
   attenzione da evitarlo. */
static int ScriptApiPlayerX(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.pos.x);
    return 1;
}

static int ScriptApiPlayerY(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.pos.y);
    return 1;
}

static int ScriptApiPlayerHp(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.hp);
    return 1;
}

static int ScriptApiPlayerMaxHp(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.maxHp);
    return 1;
}

static int ScriptApiPlayerDamage(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.damage);
    return 1;
}

static int ScriptApiPlayerItemCount(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)ScriptApiGame(L)->player.itemCount);
    return 1;
}

static int ScriptApiPlayerHasItem(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    Game *game = ScriptApiGame(L);
    bool found = false;
    for (int i = 0; i < game->player.itemCount; i++)
    {
        if (strcmp(game->player.items[i].name, name) == 0) { found = true; break; }
    }
    lua_pushboolean(L, found);
    return 1;
}

static int ScriptApiEnemyX(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Enemy *e = ScriptApiCheckEnemy(L, game, 1);
    lua_pushnumber(L, (lua_Number)e->pos.x);
    return 1;
}

static int ScriptApiEnemyY(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Enemy *e = ScriptApiCheckEnemy(L, game, 1);
    lua_pushnumber(L, (lua_Number)e->pos.y);
    return 1;
}

static int ScriptApiEnemyHp(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Enemy *e = ScriptApiCheckEnemy(L, game, 1);
    lua_pushnumber(L, (lua_Number)e->hp);
    return 1;
}

static int ScriptApiShotX(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Shot *s = ScriptApiCheckShot(L, game, 1);
    lua_pushnumber(L, (lua_Number)s->pos.x);
    return 1;
}

static int ScriptApiShotY(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Shot *s = ScriptApiCheckShot(L, game, 1);
    lua_pushnumber(L, (lua_Number)s->pos.y);
    return 1;
}

static int ScriptApiNearestEnemy(lua_State *L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    Game *game = ScriptApiGame(L);
    int best = -1;
    float bestD = 0.0f;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!game->enemies[i].active) continue;
        float dx = game->enemies[i].pos.x - x;
        float dy = game->enemies[i].pos.y - y;
        float d = dx*dx + dy*dy;
        if (best < 0 || d < bestD) { best = i; bestD = d; }
    }
    if (best < 0) { lua_pushnil(L); return 1; }
    lua_pushnumber(L, ScriptApiPackEnemyHandle(game, best));
    return 1;
}

/* room_bounds() -> x_min, y_min, x_max, y_max aveva la STESSA trappola di
   player_pos() sopra (uno script che scrive spawn_shot(room_bounds(), dx,
   dy, ...) tronca silenziosamente a room_left soltanto), quindi stessa
   cura: quattro getter a un solo valore invece di uno a quattro. Non basta
   "usarla solo da sola" come alternativa: un modello da 7B che ha appena
   imparato a chiamare player_x()/player_y() separatamente generalizza
   naturalmente lo stesso schema qui, mentre un'eccezione ("questa singola
   funzione fa eccezione ed e' ok chiamarla con piu' ritorni, le altre no")
   e' esattamente il tipo di regola sottile che un 7B non tiene a mente in
   modo affidabile. */
static int ScriptApiRoomLeft(lua_State *L)
{
    (void)L;
    lua_pushnumber(L, (lua_Number)ROOM_X);
    return 1;
}

static int ScriptApiRoomTop(lua_State *L)
{
    (void)L;
    lua_pushnumber(L, (lua_Number)ROOM_Y);
    return 1;
}

static int ScriptApiRoomRight(lua_State *L)
{
    (void)L;
    lua_pushnumber(L, (lua_Number)ROOM_RIGHT);
    return 1;
}

static int ScriptApiRoomBottom(lua_State *L)
{
    (void)L;
    lua_pushnumber(L, (lua_Number)ROOM_BOTTOM);
    return 1;
}

/* ============================================================
   Scritture: OGNI valore numerico e' bloccato agli stessi confini
   che la mini-VM gia' impone (ItemTraitsFromText, script_vm.c,
   MAX_SHOTS/MAX_ENEMIES), cosi' uno script Lua non puo' ottenere un
   proiettile/un danno/una velocita' che il gioco non produrrebbe
   gia' con i quattro operatori CSV di oggi. Vedi il task brief,
   sezione 1: "a script asking for 10^6 shots must get the same cap
   the mini-VM would apply, not a crash".
   ============================================================ */

#define SCRIPT_API_SHOT_SPEED_MIN 60.0f
#define SCRIPT_API_SHOT_SPEED_MAX 900.0f
#define SCRIPT_API_SHOT_DAMAGE_MIN 0.1f
#define SCRIPT_API_SHOT_DAMAGE_MAX 60.0f
#define SCRIPT_API_SHOT_RADIUS_MIN 2.0f
#define SCRIPT_API_SHOT_RADIUS_MAX 40.0f
#define SCRIPT_API_DAMAGE_ENEMY_MAX 500.0f
#define SCRIPT_API_HEAL_MAX 12.0f
#define SCRIPT_API_KNOCKBACK_SPEED_MAX 900.0f

/* Solo i bit di trait realmente definiti (core/game_types.h): uno script
   non puo' impostare pattern indefiniti passando un numero a caso. */
#define SCRIPT_API_TRAIT_MASK (TRAIT_BOUNCE | TRAIT_HOMING | TRAIT_EXPLODE | TRAIT_SPLIT | \
                                TRAIT_PIERCE | TRAIT_RAPID | TRAIT_GIANT | TRAIT_SLOW | TRAIT_VAMP)

/* Converte un double arbitrario in unsigned int in modo SEMPRE definito.
   luaL_optnumber() non impone alcun limite superiore (a differenza dei
   parametri che passano per GameMathClampFloat, che restano float e non
   scattano mai in UB), e (unsigned int)x per un double fuori dal range
   rappresentabile e' undefined behaviour (C99 6.3.1.4) esattamente come il
   cast in ScriptApiUnpackHandle sopra, qui per traits/colore invece che per
   un handle: spawn_shot(..., 1e300) o add_particle(x, y, 1e300) ci
   arriverebbero senza nessun'altra difesa. A differenza dell'handle pero'
   non ha senso rifiutare lo script (traits e colore non indicizzano nulla:
   un valore fuori range e' solo "sbagliato", non pericoloso), quindi si
   CLAMPA nel range rappresentabile invece di uccidere la sandbox - stesso
   spirito dei clamp GameMathClampFloat gia' usati in questo file. */
static unsigned int ScriptApiClampToUint(double v)
{
    if (!(v > 0.0)) return 0u;   /* copre anche NaN (ogni confronto con NaN e' falso) */
    if (v >= (double)UINT_MAX) return UINT_MAX;
    return (unsigned int)v;
}

static int ScriptApiSpawnShot(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    /* M2: room_left/top/right/bottom (sopra) restano il rettangolo MASSIMO
       per compatibilita' col catalogo gia' generato -- ma il clamp VERO deve
       tenere lo script dentro la stanza CORRENTE, che puo' essere piu'
       piccola: uno script che scrive fuori stanza va comunque riportato
       dentro, garanzia invariata (vedi il task brief M2, sezione 3). */
    Rectangle room = WorldCurrentRoomRect(game);
    float x = GameMathClampFloat((float)luaL_checknumber(L, 1), room.x, room.x + room.width);
    float y = GameMathClampFloat((float)luaL_checknumber(L, 2), room.y, room.y + room.height);
    float dx = (float)luaL_checknumber(L, 3);
    float dy = (float)luaL_checknumber(L, 4);
    float speed = GameMathClampFloat((float)luaL_checknumber(L, 5), SCRIPT_API_SHOT_SPEED_MIN, SCRIPT_API_SHOT_SPEED_MAX);
    float damage = GameMathClampFloat((float)luaL_checknumber(L, 6), SCRIPT_API_SHOT_DAMAGE_MIN, SCRIPT_API_SHOT_DAMAGE_MAX);
    float radius = GameMathClampFloat((float)luaL_checknumber(L, 7), SCRIPT_API_SHOT_RADIUS_MIN, SCRIPT_API_SHOT_RADIUS_MAX);
    unsigned int traits = ScriptApiClampToUint(luaL_optnumber(L, 8, 0.0)) & SCRIPT_API_TRAIT_MASK;

    /* EntitiesAddShot torna NULL sia se la direzione e' nulla sia se
       MAX_SHOTS e' gia' pieno: in ENTRAMBI i casi il cap esistente
       dell'array (lo stesso della mini-VM, non uno nuovo) e' gia' rispettato
       per costruzione, niente da clampare qui sopra oltre ai parametri
       numerici. Uno script che chiama questa funzione 10^6 volte in un
       ciclo esaurisce comunque il budget di istruzioni di frame (10^4)
       molto prima di avvicinarsi a 10^6 iterazioni; anche se non lo
       esaurisse, il numero di colpi vivi resta comunque <= MAX_SHOTS. */
    Shot *spawned = EntitiesAddShot(game, true, (Vector2){ x, y }, (Vector2){ dx, dy }, speed, damage, radius, traits, game->theme.accent2);
    if (spawned == NULL) { lua_pushnil(L); return 1; }
    lua_pushnumber(L, ScriptApiPackShotHandle(game, (int)(spawned - game->shots)));
    return 1;
}

static int ScriptApiDamageEnemy(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Enemy *e = ScriptApiCheckEnemy(L, game, 1);
    float amount = GameMathClampFloat((float)luaL_checknumber(L, 2), 0.0f, SCRIPT_API_DAMAGE_ENEMY_MAX);
    CombatDamageEnemy(game, e, amount, 0);
    return 0;
}

static int ScriptApiHealPlayer(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    float amount = GameMathClampFloat((float)luaL_checknumber(L, 1), 0.0f, SCRIPT_API_HEAL_MAX);
    game->player.hp = GameMathClampInt(game->player.hp + (int)amount, 0, game->player.maxHp);
    return 0;
}

static int ScriptApiSetEnemyVelocity(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    Enemy *e = ScriptApiCheckEnemy(L, game, 1);
    float vx = (float)luaL_checknumber(L, 2);
    float vy = (float)luaL_checknumber(L, 3);
    float len = sqrtf(vx*vx + vy*vy);
    if (len > SCRIPT_API_KNOCKBACK_SPEED_MAX && len > 0.0001f)
    {
        float scale = SCRIPT_API_KNOCKBACK_SPEED_MAX/len;
        vx *= scale;
        vy *= scale;
    }
    /* e->vel e' un impulso di spinta, sommato alla posizione (e smorzato
       esponenzialmente) da CombatUpdateEnemies ad ogni frame, oltre al
       movimento della sua IA: stesso schema di decadimento gia' usato per
       Particle.vel in GameUpdateParticles. Non sostituisce l'IA (quella e'
       il prossimo sotto-ciclo, on_enemy_update, spec sezione 10), la
       spinge. */
    e->vel.x = vx;
    e->vel.y = vy;
    return 0;
}

static int ScriptApiAddParticle(lua_State *L)
{
    Game *game = ScriptApiGame(L);
    /* M2: stesso motivo di ScriptApiSpawnShot sopra -- clamp alla stanza
       CORRENTE, non al rettangolo massimo. */
    Rectangle room = WorldCurrentRoomRect(game);
    float x = GameMathClampFloat((float)luaL_checknumber(L, 1), room.x, room.x + room.width);
    float y = GameMathClampFloat((float)luaL_checknumber(L, 2), room.y, room.y + room.height);
    unsigned int packed = ScriptApiClampToUint(luaL_optnumber(L, 3, 0.0));   /* stesso cast UB di traits sopra: vedi ScriptApiClampToUint */
    Color color = { (unsigned char)((packed >> 16) & 0xFFu), (unsigned char)((packed >> 8) & 0xFFu), (unsigned char)(packed & 0xFFu), 255 };
    EntitiesAddParticle(game, (Vector2){ x, y }, color, 1);
    return 0;
}

/* ============================================================
   Registrazione
   ============================================================ */

static void ScriptApiRegisterFn(lua_State *L, Game *game, const char *name, lua_CFunction fn)
{
    lua_pushlightuserdata(L, game);
    lua_pushcclosure(L, fn, 1);
    lua_setfield(L, -2, name);
}

static void ScriptApiRegisterTraitConstant(lua_State *L, const char *name, unsigned int value)
{
    lua_pushnumber(L, (lua_Number)value);
    lua_setfield(L, -2, name);
}

void ScriptApiRegister(ScriptSandbox *sb, Game *game)
{
    lua_State *L = ScriptSandboxRawState(sb);
    if (L == NULL) return;
    lua_pushglobaltable(L);

    ScriptApiRegisterFn(L, game, "player_x", ScriptApiPlayerX);
    ScriptApiRegisterFn(L, game, "player_y", ScriptApiPlayerY);
    ScriptApiRegisterFn(L, game, "player_hp", ScriptApiPlayerHp);
    ScriptApiRegisterFn(L, game, "player_max_hp", ScriptApiPlayerMaxHp);
    ScriptApiRegisterFn(L, game, "player_damage", ScriptApiPlayerDamage);
    ScriptApiRegisterFn(L, game, "player_item_count", ScriptApiPlayerItemCount);
    ScriptApiRegisterFn(L, game, "player_has_item", ScriptApiPlayerHasItem);
    ScriptApiRegisterFn(L, game, "enemy_x", ScriptApiEnemyX);
    ScriptApiRegisterFn(L, game, "enemy_y", ScriptApiEnemyY);
    ScriptApiRegisterFn(L, game, "enemy_hp", ScriptApiEnemyHp);
    ScriptApiRegisterFn(L, game, "shot_x", ScriptApiShotX);
    ScriptApiRegisterFn(L, game, "shot_y", ScriptApiShotY);
    ScriptApiRegisterFn(L, game, "nearest_enemy", ScriptApiNearestEnemy);
    ScriptApiRegisterFn(L, game, "room_left", ScriptApiRoomLeft);
    ScriptApiRegisterFn(L, game, "room_top", ScriptApiRoomTop);
    ScriptApiRegisterFn(L, game, "room_right", ScriptApiRoomRight);
    ScriptApiRegisterFn(L, game, "room_bottom", ScriptApiRoomBottom);
    ScriptApiRegisterFn(L, game, "spawn_shot", ScriptApiSpawnShot);
    ScriptApiRegisterFn(L, game, "damage_enemy", ScriptApiDamageEnemy);
    ScriptApiRegisterFn(L, game, "heal_player", ScriptApiHealPlayer);
    ScriptApiRegisterFn(L, game, "set_enemy_velocity", ScriptApiSetEnemyVelocity);
    ScriptApiRegisterFn(L, game, "add_particle", ScriptApiAddParticle);

    /* Costanti dei trait (core/game_types.h), cosi' uno script passa
       TRAIT_HOMING invece di scrivere "2" a memoria. rng() e' gia' presente
       nell'_ENV: e' la sandbox stessa a metterla li' (ScriptSandboxLuaRng in
       script_sandbox.c), non serve ripeterla qui. */
    ScriptApiRegisterTraitConstant(L, "TRAIT_BOUNCE", TRAIT_BOUNCE);
    ScriptApiRegisterTraitConstant(L, "TRAIT_HOMING", TRAIT_HOMING);
    ScriptApiRegisterTraitConstant(L, "TRAIT_EXPLODE", TRAIT_EXPLODE);
    ScriptApiRegisterTraitConstant(L, "TRAIT_SPLIT", TRAIT_SPLIT);
    ScriptApiRegisterTraitConstant(L, "TRAIT_PIERCE", TRAIT_PIERCE);
    ScriptApiRegisterTraitConstant(L, "TRAIT_RAPID", TRAIT_RAPID);
    ScriptApiRegisterTraitConstant(L, "TRAIT_GIANT", TRAIT_GIANT);
    ScriptApiRegisterTraitConstant(L, "TRAIT_SLOW", TRAIT_SLOW);
    ScriptApiRegisterTraitConstant(L, "TRAIT_VAMP", TRAIT_VAMP);

    lua_pop(L, 1);   /* toglie la tabella globale dallo stack esplorativo */
}
