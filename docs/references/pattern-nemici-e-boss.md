# Pattern di nemici e boss (per la fase 3b)

> Documento di **riferimento**. Cataloga i pattern di comportamento ricorrenti
> dei nemici e la struttura di un boss "a parti", e li mappa concretamente
> sull'architettura di Melting Run (C99 + raylib, sandbox Lua 5.5, contenuto di
> run generato da un LLM locale via llama.cpp).
>
> **Stato**: la fase 3a ha portato in gioco la sandbox Lua e le callback
> *degli oggetti* (`on_evaluate` / `on_fire` / `on_hit` / `on_tick`, vedi
> `src/script/script_items.c`). Le callback *dei nemici* — `on_enemy_update`,
> l'oggetto di questo documento — **non esistono ancora**: sono il "prossimo
> sotto-ciclo" gia' previsto dalla spec e citato testualmente in un commento di
> `src/script/script_api.c` (riga ~387: *"il prossimo sotto-ciclo,
> on_enemy_update, spec sezione 10"*). Qui progettiamo come dovranno essere.
>
> **Fonti**: il modello NPC ufficiale di Isaac (IsaacDocs) come *ispirazione
> architetturale* — vedi la nota sui confini di IP in fondo — piu' i pattern di
> IA nemica classici dei roguelike top-down / twin-stick. Nomi, testi, sprite e
> tabelle numeriche di quel gioco NON sono riusati: descriviamo i *pattern*
> (fatti liberi) con nomi generici.

---

## 1. Da dove partiamo: il ciclo nemici in C oggi

Oggi ogni nemico e' guidato interamente in C, in `CombatUpdateEnemies`
(`src/gameplay/combat.c`). La struttura dati e' minima
(`src/core/game_types.h`):

```c
typedef enum EnemyKind { ENEMY_CHASER, ENEMY_SHOOTER, ENEMY_TANK, ENEMY_BOSS } EnemyKind;

typedef struct Enemy {
    bool active;
    EnemyKind kind;
    Vector2 pos;
    Vector2 vel;      /* impulso di spinta smorzato, NON la velocita' dell'IA */
    float radius;
    float hp, maxHp;
    float speed;
    float cooldown;   /* timer di attacco, conta alla rovescia */
    float slowTimer;
} Enemy;
```

Il loop attuale, sintetizzato, fa gia' tre cose che diventeranno i tre "mattoni"
riusati da ogni pattern:

1. **Movimento** — calcola `dir = normalize(player.pos - e.pos)` e sposta il
   nemico. Ogni `kind` piega questa direzione a modo suo: `ENEMY_CHASER` va
   dritto addosso; `ENEMY_SHOOTER` se `dist < 210` inverte (`-0.65`) per tenere
   la distanza (kiting); `ENEMY_BOSS` mescola avvicinamento e un termine
   perpendicolare `sinf(GetTime()*1.4f)` per oscillare (strafing).
2. **Impulso + smorzamento** — `e.pos += e.vel*dt; e.vel *= 0.90f`. `e.vel` NON
   e' la velocita' dell'IA: e' un contraccolpo che *altri* possono imporre (es.
   `set_enemy_velocity` da un `on_hit` di un oggetto Lua). Decade da solo.
3. **Clamp ai muri** — `pos` e' sempre ricondotto dentro
   `[ROOM_X+radius, ROOM_RIGHT-radius] x [ROOM_Y+radius, ROOM_BOTTOM-radius]`.
4. **Attacco a cooldown** — `e.cooldown -= dt`; quando `<= 0` il nemico spara
   (`EntitiesAddShot`) e riarma il cooldown. Lo `SHOOTER`/`TANK` spara un colpo
   mirato; il `BOSS` spara un anello di `count` proiettili disposti su
   `a = s*2pi/count + GetTime()*0.22f`.

**L'obiettivo della fase 3b**: al posto (o accanto) a questo `switch` sul `kind`
scritto a mano, lasciare che l'LLM generi *lo script di comportamento* di un
archetipo di nemico, eseguito in sandbox, mentre il C resta il padrone assoluto
di sicurezza, cap e posizioni valide. Esattamente il patto gia' in vigore per
gli oggetti.

---

## 2. Il modello NPC di Isaac come ispirazione (non da copiare)

L'API di modding di Isaac (IsaacDocs) e' la nostra bussola *architetturale*. I
concetti-chiave, riscritti con parole nostre:

- **Callback per-frame `MC_NPC_UPDATE(EntityNPC)`**: gira *dopo* che l'update
  interno del nemico e' completato, una volta per frame, e non scatta durante
  l'animazione di comparsa. Esistono anche `MC_PRE_NPC_UPDATE` (puo' *saltare*
  l'IA interna restituendo `true`), `MC_POST_NPC_INIT`, `MC_POST_NPC_RENDER`,
  `MC_PRE_NPC_COLLISION`, `MC_POST_NPC_DEATH`.
- **Macchina a stati sull'entita'**: `State` (stato corrente) + `StateFrame`
  (quanti frame in quello stato). Piu' due interi `I1/I2` e due vettori
  `V1/V2` liberi, che il singolo tipo di nemico usa come gli pare per la sua
  logica di IA (bersaglio memorizzato, fase, contatore di raffica...).
- **Helper di alto livello** citati dai docs: `CalcTargetPosition`,
  `GetPlayerTarget`, `Pathfinder`/`ResetPathFinderTarget`, `FireProjectiles`
  (spara N proiettili, opzionalmente mirati al giocatore), `FireBossProjectiles`
  (traiettorie casuali o che inseguono il giocatore).

**Cosa prendiamo**: la *forma*. Una singola callback per-frame, con lo stato
persistente del nemico esposto allo script, e un pugno di primitive
"spara/muovi/cerca il bersaglio" fornite dal motore. **Cosa NON prendiamo**:
nomi, valori numerici, tabelle di ID, nomi di nemici. Sono fatti liberi solo i
*pattern* (§4).

### 2.1 Mappatura dei concetti Isaac -> Melting Run

| Concetto Isaac (ispirazione) | Equivalente Melting Run |
|---|---|
| `MC_NPC_UPDATE(EntityNPC)` | `on_enemy_update(enemy_id, dt)` (Lua, da introdurre) |
| `EntityNPC` (puntatore) | **handle** `enemy_id` (indice+generazione, mai un puntatore — vedi §3.1) |
| `State` / `StateFrame` | campi di stato del nemico esposti come numeri (§5) |
| `I1/I2`, `V1/V2` (scratch IA) | tabella-stato per-nemico lato Lua / campi scratch in C (§5) |
| `FireProjectiles` / `FireBossProjectiles` | `spawn_shot(x,y,dx,dy,speed,dmg,radius[,traits])` |
| `pos`, `Velocity` | `enemy_x/enemy_y(id)`, `set_enemy_velocity(id,vx,vy)` |
| `GetPlayerTarget` | `player_x() / player_y()`, `nearest_enemy(x,y)` |
| cooldown interni | `cooldown` in `Enemy` + timer nello stato Lua |

---

## 3. L'API di gioco gia' esposta a Lua (e cosa manca)

Tutto quello che segue e' gia' registrato in `ScriptApiRegister`
(`src/script/script_api.c`) e quindi *gia' disponibile* al momento in cui
aggiungeremo `on_enemy_update`. Ogni funzione clampa i suoi argomenti ai
confini di sicurezza: uno script non puo' produrre coordinate o velocita' fuori
scala.

**Lettura del mondo**
- `player_x()`, `player_y()`, `player_hp()`, `player_max_hp()`, `player_damage()`
- `enemy_x(id)`, `enemy_y(id)`, `enemy_hp(id)`
- `shot_x(id)`, `shot_y(id)`
- `nearest_enemy(x, y)` -> handle del nemico piu' vicino a `(x,y)` (usato oggi
  dagli oggetti; per un nemico serve piu' che altro per attacchi "di supporto")
- `room_left()`, `room_top()`, `room_right()`, `room_bottom()` -> i confini
  della stanza (`ROOM_X`, `ROOM_Y`, `ROOM_RIGHT`, `ROOM_BOTTOM`)

**Azione sul mondo**
- `spawn_shot(x, y, dx, dy, speed, damage, radius[, traits])` — genera un
  proiettile. `x,y` sono clampati alla stanza; `speed`, `damage`, `radius`
  sono clampati a `[SCRIPT_API_SHOT_*_MIN, ..._MAX]`; direzione nulla =
  nessun colpo. `traits` accetta le costanti `TRAIT_*` (bounce/homing/explode/
  split/pierce/rapid/giant/slow/vamp).
- `damage_enemy(id, amount)` — infligge danno a un nemico (usato oggi dagli
  oggetti; per i nemici serve per pattern "amico che ferisce amico" o boss che
  danneggia le proprie parti).
- `heal_player(amount)`
- `set_enemy_velocity(id, vx, vy)` — imposta l'**impulso** `Enemy.vel` (poi
  smorzato in C, §1.2). E' la leva giusta per scatti/cariche: dai una spinta
  forte e lascia che decada.
- `add_particle(...)` — effetti visivi.
- `rng()` — gia' iniettata nell'_ENV dalla sandbox stessa
  (`ScriptSandboxLuaRng`), restituisce un numero pseudo-casuale deterministico
  per-run.

**Cosa manca (da aggiungere nella 3b)**
- La callback `on_enemy_update(enemy_id, dt)` e il ciclo C che la invoca per
  ogni nemico attivo (l'analogo per-nemico di `ScriptItemsOnTick`).
- Un modo per il nemico di leggere/scrivere il *proprio* stato persistente
  (fase, timer di raffica, bersaglio memorizzato). Vedi §5.
- Facoltativo ma utile: `set_enemy_target`/lettura `enemy_kind(id)`,
  `enemy_cooldown(id)`, o semplicemente esporre `cooldown`/`slowTimer` in
  lettura.

### 3.1 Regola d'oro degli handle (vale anche per i nemici)

Lua **non vede mai un puntatore**. `enemy_id` e' un handle = indice nell'array
`Game.enemies[]` + un contatore di **generazione** (`Game.enemyGen[]`),
impacchettati in un unico numero (i numeri Lua sono double, esatti fino a
2^53). Ogni funzione che riceve un handle lo valida *prima* di toccare
qualunque cosa; un handle stantio (lo slot e' stato riusato da un altro nemico,
la generazione non combacia) solleva `luaL_error`, che il chiamante traduce
nell'**uccisione permanente della sandbox** (patto di sicurezza). Questo e' il
motivo per cui uno script di nemico che tiene in memoria l'handle di un altro
nemico attraverso piu' frame non puo' mai leggere/scrivere l'entita' sbagliata:
al massimo si autodistrugge, e il C ripiega sul comportamento built-in.

---

## 4. Catalogo dei pattern di comportamento

Per ciascun pattern: **regola di movimento**, **regola di attacco**, e la
**mappatura** su `on_enemy_update(enemy_id, dt)` + API a handle. Gli esempi Lua
sono illustrativi (l'LLM generera' qualcosa di simile su template tipizzato,
§6). Convenzione: `me` = `enemy_id`, `px,py` = posizione giocatore,
`ex,ey` = posizione del nemico, `st` = tabella-stato per-nemico (§5).

### 4.1 Inseguitore diretto (straight chaser)

Il piu' semplice: punta il giocatore e ci va addosso in linea retta. Nessun
attacco a distanza; il danno e' da contatto. E' l'attuale `ENEMY_CHASER`.

- **Movimento**: `dir = normalize(player - me)`, muovi a `speed` costante.
- **Attacco**: nessuno (contatto, gia' gestito in C dal test di collisione
  `radius + player.radius`).

```lua
function on_enemy_update(me, dt)
    local dx, dy = player_x() - enemy_x(me), player_y() - enemy_y(me)
    local len = math.sqrt(dx*dx + dy*dy) + 1e-5
    set_enemy_velocity(me, dx/len * 90, dy/len * 90)  -- spinta verso il player
end
```

> Nota: usare `set_enemy_velocity` come *unica* fonte di moto funziona perche'
> il C somma `e.vel*dt` alla posizione ogni frame; ma `e.vel` decade a `0.90`
> per frame, quindi va *ri-applicata* ogni update (come sopra). In alternativa,
> se in 3b esporremo una `set_enemy_move_dir` che scrive la direzione "di IA"
> (non l'impulso), il chaser diventa un one-liner senza ri-applicazione. Da
> decidere in fase di design della callback.

### 4.2 Mosca vagante (wandering fly)

Si muove di moto quasi-casuale/fluttuante, ignorando o inseguendo blandamente il
giocatore. Debole, fastidiosa, spesso in sciame.

- **Movimento**: direzione che deriva lentamente. Un buon trucco senza stato e'
  il *flow-field* trigonometrico: `angle = base + noise(GetTime())`. Con stato,
  si mantiene una direzione e la si ruota di un piccolo delta casuale ogni tot.
- **Attacco**: nessuno, oppure danno da contatto.

```lua
function on_enemy_update(me, dt)
    st.t = (st.t or rng()*6.28) + dt
    local a = st.t + math.sin(st.t*0.7)*1.5      -- angolo che serpeggia
    set_enemy_velocity(me, math.cos(a)*55, math.sin(a)*55)
end
```

### 4.3 Cecchino stazionario (stationary shooter)

Sta fermo (o si muove poco per tenere la distanza) e spara al giocatore a
intervalli regolari. E' il cuore dell'attuale `ENEMY_SHOOTER` (col kiting) e
`ENEMY_TANK` (fermo, colpo lento e pesante).

- **Movimento**: nullo, oppure *kiting*: se `dist < soglia`, arretra.
- **Attacco**: ogni `cooldown` secondi, `spawn_shot` mirato al giocatore.

```lua
function on_enemy_update(me, dt)
    local ex, ey = enemy_x(me), enemy_y(me)
    local dx, dy = player_x() - ex, player_y() - ey
    local len = math.sqrt(dx*dx + dy*dy) + 1e-5
    if len < 210 then set_enemy_velocity(me, -dx/len*60, -dy/len*60) end  -- kiting
    st.cd = (st.cd or 0) - dt
    if st.cd <= 0 then
        spawn_shot(ex, ey, dx/len, dy/len, 245, 1, 6)
        st.cd = 1.0
    end
end
```

### 4.4 Caricatore / scattante (charger, dasher)

Prende la mira, si ferma un istante, poi scatta velocissimo in linea retta e
supera il giocatore; poi recupera e ripete. Naturale macchina a due stati
(`AIM` -> `DASH` -> recupero).

- **Movimento**: negli stati `AIM`/recupero e' quasi fermo; nel `DASH` una
  grossa spinta una-tantum via `set_enemy_velocity` (che poi decade da sola —
  perfetto per uno scatto).
- **Attacco**: il corpo stesso durante lo scatto (contatto); niente proiettili.

```lua
function on_enemy_update(me, dt)
    st.state = st.state or "aim"
    st.t = (st.t or 0) + dt
    if st.state == "aim" then
        if st.t > 0.6 then
            local dx = player_x() - enemy_x(me)
            local dy = player_y() - enemy_y(me)
            local l = math.sqrt(dx*dx+dy*dy)+1e-5
            set_enemy_velocity(me, dx/l*520, dy/l*520)  -- SCATTO (impulso, decade)
            st.state, st.t = "recover", 0
        end
    elseif st.state == "recover" and st.t > 1.0 then
        st.state, st.t = "aim", 0
    end
end
```

### 4.5 Divisore (splitter)

Alla morte si spezza in due (o piu') nemici piu' piccoli e veloci; quelli
piccoli, morendo, possono sparire o dividersi ancora fino a una taglia minima.
Il pattern-*morte* e' l'analogo del trait `TRAIT_SPLIT` gia' esistente per i
proiettili (`CombatSplitShot` in `combat.c`).

- **Movimento**: uno qualunque dei precedenti (spesso chaser).
- **Attacco/morte**: alla morte, generare 2-3 nemici piu' piccoli. Questa e'
  una reazione a un *evento*, non all'update per-frame: il posto giusto e' una
  callback `on_enemy_death(enemy_id)` (l'analogo per-nemico di `MC_POST_NPC_DEATH`)
  da aggiungere insieme a `on_enemy_update`. Serve inoltre una primitiva
  `spawn_enemy(kind, x, y)` **soggetta a cap** (§7).

```lua
-- da chiamare in on_enemy_death(me) [callback da introdurre in 3b]
function on_enemy_death(me)
    if (st.tier or 0) < 2 then           -- non oltre 2 divisioni
        local ex, ey = enemy_x(me), enemy_y(me)
        for i = 1, 2 do
            local a = rng() * 6.28
            spawn_enemy("fly", ex + math.cos(a)*20, ey + math.sin(a)*20)  -- C: cap MAX_ENEMIES
        end
    end
end
```

> `spawn_enemy` non esiste ancora. E' la primitiva piu' delicata da esporre: va
> **sempre** limitata dal C (numero di nemici vivi, profondita' di divisione,
> posizione valida). Vedi §7.

### 4.6 Bombardiere (bomber)

Si avvicina e lascia cadere un ordigno che esplode dopo un ritardo in un'area
(o si fa esplodere lui stesso — kamikaze). Melting Run ha gia' i tipi `Bomb`
(`MAX_BOMBS`) e il trait `TRAIT_EXPLODE`.

- **Movimento**: avvicinamento (chaser) fino a una distanza di sgancio.
- **Attacco**: a `dist < soglia`, piazza una bomba ritardata. Serve una
  primitiva `spawn_bomb(x, y[, timer])` (oggi le bombe le piazza solo il
  giocatore); in mancanza, un ripiego e' `spawn_shot` lento con `TRAIT_EXPLODE`
  (esplode all'impatto/fine vita) — riusa codice esistente senza nuove API.

```lua
function on_enemy_update(me, dt)
    local ex, ey = enemy_x(me), enemy_y(me)
    local dx, dy = player_x() - ex, player_y() - ey
    local l = math.sqrt(dx*dx+dy*dy)+1e-5
    if l > 90 then
        set_enemy_velocity(me, dx/l*80, dy/l*80)
    else
        st.cd = (st.cd or 0) - dt
        if st.cd <= 0 then
            -- ripiego senza nuove API: proiettile lento esplosivo
            spawn_shot(ex, ey, dx/l, dy/l, 120, 3, 10, TRAIT_EXPLODE)
            st.cd = 2.5
        end
    end
end
```

### 4.7 Sparo ad anello / radiale (ring-shooter)

Sputa proiettili in tutte le direzioni contemporaneamente (anello), spesso con
l'anello che *ruota* di frame in frame per creare una spirale. E' esattamente
il pattern d'attacco dell'attuale `ENEMY_BOSS` in C, promosso a pattern
riutilizzabile anche da nemici normali "elite".

- **Movimento**: tipicamente lento o fermo (o strafing come il boss).
- **Attacco**: ogni `cooldown`, `count` colpi su
  `a = s*2pi/count + fase`, con `fase` che avanza nel tempo per l'effetto
  spirale.

```lua
function on_enemy_update(me, dt)
    st.cd = (st.cd or 0) - dt
    if st.cd <= 0 then
        local ex, ey = enemy_x(me), enemy_y(me)
        local n = 12
        st.phase = (st.phase or 0) + 0.22       -- fa "girare" l'anello
        for s = 0, n - 1 do
            local a = s * 6.2831853 / n + st.phase
            spawn_shot(ex, ey, math.cos(a), math.sin(a), 215, 1, 7)
        end
        st.cd = 1.1
    end
end
```

Varianti gratis: **ventaglio** (solo un arco di angoli verso il giocatore),
**aimed-ring** (l'anello e' centrato sulla direzione del giocatore),
**doppio anello sfasato** (due `for` con `n` diversi).

### 4.8 Mini-boss a fasi (phased mini-boss)

Un nemico grosso con piu' HP che, a soglie di vita, **cambia comportamento**:
per esempio fase 1 = anello lento, fase 2 (sotto il 50% HP) = anello veloce +
scatti, fase 3 (sotto il 25%) = evoca minion. E' la macchina a stati del §2
portata all'estremo, guidata da `enemy_hp(me) / player-side maxHp`.

- **Movimento/attacco**: si seleziona un "sotto-pattern" (una delle §4.x) in
  base alla fascia di HP corrente. Le transizioni sono a senso unico
  (peggiora man mano che scende di vita).

```lua
function on_enemy_update(me, dt)
    local frac = enemy_hp(me) / (st.max_hp or 200)
    local phase = (frac < 0.25) and 3 or (frac < 0.5) and 2 or 1
    if phase ~= st.phase then st.phase, st.cd = phase, 0 end  -- reset al cambio
    st.cd = st.cd - dt
    if st.cd <= 0 then
        local ex, ey = enemy_x(me), enemy_y(me)
        local n = 8 + phase*3
        for s = 0, n-1 do
            local a = s*6.2831853/n + (st.spin or 0)
            spawn_shot(ex, ey, math.cos(a), math.sin(a), 200 + phase*20, 1, 7)
        end
        st.spin = (st.spin or 0) + 0.15*phase
        st.cd = 1.4 - phase*0.35
        if phase == 3 and rng() < 0.3 then spawn_enemy("fly", ex, ey) end
    end
end
```

---

## 5. Lo stato persistente del nemico

Ogni pattern non banale (cecchino, caricatore, mini-boss) ha bisogno di
ricordare qualcosa fra un frame e l'altro: un timer di raffica, la fase
corrente, il bersaglio memorizzato. In Isaac questo e' `State/StateFrame/I1/I2/
V1/V2`. Da noi due opzioni, in ordine di preferenza:

1. **Tabella-stato per-nemico lato Lua** (raccomandato). Come per gli oggetti,
   ogni nemico "vivo con Lua" ha la *sua* sandbox (o una condivisa
   per-archetipo con una tabella indicizzata per handle). La callback riceve
   una `st` che persiste per la vita del nemico. Semplice per l'LLM
   (`st.cd = ... ; st.phase = ...`), zero nuovi campi in C. Costo: gestione
   della vita della sandbox al `active=false` del nemico (analogo a
   `ScriptItemsShutdown`).
2. **Campi scratch in C esposti come numeri**. Aggiungere a `Enemy` un pugno di
   campi generici (`float ai_f[2]; int ai_i[2];`) e esporli con
   `enemy_get/set_scratch(id, slot, val)`. Piu' vicino al modello Isaac,
   ma piu' verboso per lo script e piu' rumore in `Enemy`.

In entrambi i casi vale la regola handle (§3.1): lo stato e' legato allo *slot+
generazione*, quindi quando lo slot viene riusato da un altro nemico lo stato
riparte pulito.

Nota di sicurezza: il `cooldown` in `Enemy` e' gia' decrementato dal C. Se la
callback Lua vive *accanto* al C (non lo sostituisce), meglio che il timer
d'attacco viva **solo** in un posto (o in `st.cd` lato Lua, o in `e.cooldown`
lato C) per non avere due orologi che litigano.

---

## 6. Chi genera cosa: LLM vs. C

Stesso patto della fase 3a per gli oggetti, esteso ai nemici.

**L'LLM genera** (in fase di caricamento della run, tramite il sidecar/il
melting-gen, mai a 60 FPS):
- Lo **script di comportamento per archetipo di nemico**: il corpo di
  `on_enemy_update(enemy_id, dt)` (ed eventualmente `on_enemy_death`), scritto
  su **template fortemente tipizzato** — l'LLM riempie buchi prefissati
  (soglie, count, cooldown, quale sotto-pattern per fase) invece di scrivere
  Lua libero. Questo e' esplicito nelle linee guida di sicurezza in
  `docs/APPUNTI.md` §5 ("il prompt... obbligando l'LLM a compilare template
  prefissati").
- La **scheda tecnica del boss**: l'anatomia a parti (§8), gli offset, quale
  script d'attacco per parte, i cooldown. E' gia' il formato JSON in APPUNTI §3B.

**Il C impone i confini** (invariante, non negoziabile):
- **Cap sulle entita'**: `MAX_ENEMIES=64`, `MAX_SHOTS=220`, `MAX_BOMBS=8`.
  `EntitiesAddShot/AddEnemy` restituiscono `NULL`/no-op se pieno: uno script
  che spamma non fa crescere la memoria.
- **Cap di spawn e profondita' di divisione**: la futura `spawn_enemy` deve
  rifiutare oltre il cap di nemici vivi *e* oltre una profondita' massima di
  divisione (tier), per impedire crescita esponenziale (§4.5, §7).
- **Posizioni valide**: `spawn_shot` clampa `x,y` alla stanza; le posizioni di
  spawn passano da `EntitiesRandomRoomPosition` / clamp ai muri
  (`ROOM_X..ROOM_RIGHT`, `ROOM_Y..ROOM_BOTTOM`).
- **Range di velocita'/danno/raggio**: clamp a `SCRIPT_API_SHOT_*_MIN/MAX`.
- **Patto della sandbox**: qualunque errore (handle stantio, tipo sbagliato,
  budget di istruzioni sforato) **uccide** quella sandbox; il nemico ripiega
  sul comportamento built-in del suo `kind` senza interrompere il gioco.

Regola pratica: **l'LLM decide il "cosa" (la forma dell'attacco/movimento), il
C decide il "quanto" (i tetti)**. Un boss "folle" generato da un 7B che sbaglia
i conti non puo' mai rendere il gioco ingiocabile o farlo crashare.

---

## 7. La primitiva critica: `spawn_enemy`

Splitter (§4.5), bomber che evoca, mini-boss fase 3 (§4.8) e le parti di boss
che rigenerano (§8) hanno tutti bisogno di creare *nemici*. E' l'API piu'
pericolosa da esporre, quindi va progettata con i freni incorporati:

Firma proposta: `spawn_enemy(kind, x, y) -> handle | nil`

Guardie **obbligatorie in C**, controllate *prima* di creare:
1. **Cap globale**: se i nemici attivi sono gia' `>= MAX_ENEMIES`, ritorna
   `nil` (mai crash, mai riallocazione).
2. **Budget di spawn per-frame/per-nemico**: un contatore per impedire che un
   singolo `on_enemy_update` evochi 60 nemici in un frame (es. max N per
   chiamata).
3. **Profondita' di divisione**: il tier va tracciato in C (o nello stato) e la
   creazione rifiutata oltre un massimo, cosi' lo splitter converge.
4. **Posizione**: `x,y` clampati alla stanza (come `spawn_shot`); niente spawn
   dentro i muri o fuori mappa.
5. **`kind` validato**: solo un enum/whitelist noto; un `kind` sconosciuto e'
   un errore di sandbox, non un valore fuori range letto a memoria.

Finche' `spawn_enemy` non esiste, gli splitter/bomber si possono prototipare con
`spawn_shot(..., TRAIT_EXPLODE)` (bomber) e rimandare la vera divisione — cosi'
la 3b puo' partire *senza* subito la primitiva piu' rischiosa.

---

## 8. Il boss "a parti" (part system)

Il proprietario ha progettato il boss come **array di parti**, non come singola
texture, in `docs/APPUNTI.md` §3. Qui lo rendiamo un piano concreto mappato sui
nostri tipi e su Lua.

### 8.1 Le struct C (da APPUNTI, adottate)

```c
typedef enum { PART_CORE, PART_HEAD, PART_ARM, PART_LEG, PART_TENTACLE, PART_WING } PartType;

typedef struct {
    PartType type;
    Texture2D texture;         /* ritagliata dall'atlas 128x128 */
    Vector2 offset_locale;     /* posizione RELATIVA al core del boss */
    float rotazione;
    float timer_attacco;       /* conta alla rovescia, come Enemy.cooldown */
    float cooldown_attacco;
    char script_attacco_lua[64]; /* nome della callback Lua, es. "attack_ring" */
} MonsterPart;

typedef struct {
    Vector2 posizione_mondo;   /* il "core"; le parti sono offset attorno a questo */
    float hp;
    MonsterPart parti[32];     /* array FISSO: sicurezza di memoria, niente alloc */
    int numero_parti;
} Boss;
```

**Coerenza con l'esistente**: `parti[32]` a dimensione fissa e' esattamente lo
stile POD del resto del codice (array fissi in `Game`, zero allocazioni fuori
da Lua). `timer_attacco`/`cooldown_attacco` per parte sono l'analogo del
`cooldown` di `Enemy`. Il core si integra col resto o come un `Enemy` di
`kind = ENEMY_BOSS` promosso, oppure come struct dedicata in `Game` (un solo
boss vivo per volta: un campo `Boss boss; bool bossActive;` e' sufficiente e
non tocca `MAX_ENEMIES`).

### 8.2 Cosa genera l'LLM: la scheda anatomica

All'inizio del piano, il prompt (sidecar Node / melting-gen) produce la scheda
del boss — formato gia' in APPUNTI §3B, riscritto con nomi generici:

```json
{
  "nome": "Colosso Palustre",
  "parti": [
    { "type": "PART_CORE",     "offset": {"x": 0,   "y": 0} },
    { "type": "PART_TENTACLE", "offset": {"x": -30, "y": 10},
      "script_attacco": "attack_sweep", "cooldown": 2.5 },
    { "type": "PART_TENTACLE", "offset": {"x":  30, "y": 10},
      "script_attacco": "attack_ring",  "cooldown": 1.8 }
  ]
}
```

L'LLM assegna: tipo di parte, offset locale, *quale* script d'attacco per parte
(scelto da un catalogo di template — gli §4.x!), e i cooldown. Il C valida:
`numero_parti <= 32`, offset entro un raggio ragionevole, `script_attacco`
appartenente alla whitelist di callback note.

### 8.3 Animazione procedurale (sin/cos su GetTime)

Per non far generare all'LLM centinaia di frame, le appendici si animano con
la matematica, come da APPUNTI §3C. Ogni frame, in C, prima di disegnare:

```c
double t = GetTime();
for (int i = 0; i < boss.numero_parti; i++) {
    MonsterPart *p = &boss.parti[i];
    Vector2 wobble = {0};
    if (p->type == PART_TENTACLE || p->type == PART_WING) {
        wobble.x = sinf((float)t*1.4f + i)*6.0f;   /* oscillazione */
        wobble.y = cosf((float)t*1.1f + i)*4.0f;
        p->rotazione = sinf((float)t*1.4f + i)*0.25f;
    }
    Vector2 world = Vector2Add(boss.posizione_mondo,
                               Vector2Add(p->offset_locale, wobble));
    /* DrawEllipse(ombra) poi Draw della texture della parte a 'world', p->rotazione */
}
```

`sinf/cosf` con fase `+ i` diversa per parte danno un ondeggiamento
sfalsato (tentacoli che non si muovono all'unisono). E' *pura animazione
visiva*, nessuna implicazione di gameplay — vive interamente in C, mai in Lua.

### 8.4 Esecuzione degli attacchi per parte

Nel game loop C, per ogni parte, si decrementa `timer_attacco`; a `<= 0` si
invoca la callback Lua nominata da `script_attacco_lua`, passandole **la
posizione mondo della parte** (core + offset + eventuale wobble) e l'handle del
boss/della parte. La callback e' uno degli §4.x (anello, ventaglio, sweep...),
solo ancorata alla *parte* invece che al centro del nemico:

```c
for (int i = 0; i < boss.numero_parti; i++) {
    MonsterPart *p = &boss.parti[i];
    if (p->script_attacco_lua[0] == '\0') continue;
    p->timer_attacco -= dt;
    if (p->timer_attacco <= 0.0f) {
        Vector2 world = Vector2Add(boss.posizione_mondo, p->offset_locale);
        /* ScriptBossCallAttack(game, p->script_attacco_lua, world, ...) */
        p->timer_attacco = p->cooldown_attacco;
    }
}
```

Lato Lua, `attack_ring` e' letteralmente il §4.7 ma centrato su `world` passato
dal C invece che su `enemy_x/y`:

```lua
function attack_ring(wx, wy)   -- wx,wy = posizione mondo della PARTE
    local n = 12
    for s = 0, n-1 do
        local a = s*6.2831853/n
        spawn_shot(wx, wy, math.cos(a), math.sin(a), 210, 1, 7)
    end
end

function attack_sweep(wx, wy) -- ventaglio spazzante verso il giocatore
    local dx, dy = player_x() - wx, player_y() - wy
    local base = math.atan(dy, dx)
    for k = -2, 2 do
        local a = base + k*0.18
        spawn_shot(wx, wy, math.cos(a), math.sin(a), 200, 1, 7)
    end
end
```

### 8.5 Danno e distruzione delle parti (opzionale, potente)

Il modello a parti abilita boss dove **le appendici hanno HP propri** e vengono
distrutte una a una (ognuna ha il suo `hp` — basta aggiungerlo a `MonsterPart`).
Distruggere una parte spegne il suo `script_attacco_lua` (un attacco in meno) e
puo' innescare una transizione di fase del core (§4.8). Il test di collisione
colpo-boss in C sceglie la parte colpita (bounding circle attorno a
`core+offset`), scala il suo `hp`, e a `hp<=0` la disattiva. Tutto in C:
posizioni, HP e cap restano dominio del motore; Lua fornisce solo i *pattern*
di sparo.

---

## 9. Piano di adozione incrementale per la 3b

1. **Callback + ciclo**: aggiungere `on_enemy_update(enemy_id, dt)` e il loop C
   che la chiama per ogni nemico "con Lua attivo", *accanto* a
   `CombatUpdateEnemies` (i nemici senza Lua restano sul built-in). Riusare lo
   scheletro di `ScriptItemsOnTick`.
2. **Stato per-nemico** (§5): decidere tabella-Lua vs. scratch-C; implementare
   la vita della sandbox al `active=false`.
3. **Pattern senza nuove API**: portare in Lua chaser/wanderer/shooter/charger/
   ring-shooter/phased usando *solo* le API gia' esistenti (§3). Questi coprono
   6 degli 8 pattern senza toccare la sicurezza.
4. **`on_enemy_death` + `spawn_enemy` con cap** (§7): sbloccano splitter e
   bomber "veri". E' il passo piu' delicato — farlo per ultimo, con test di cap.
5. **Boss a parti**: struct C (§8.1), scheda LLM (§8.2), animazione (§8.3),
   esecuzione attacchi (§8.4). HP per parte (§8.5) come estensione finale.
6. **melting-gen**: template tipizzati per gli script di comportamento e per la
   scheda anatomica del boss; validatore che rifiuta callback fuori whitelist,
   `numero_parti > 32`, offset assurdi (stesso spirito del validatore JSON gia'
   in uso per gli oggetti).

---

## 10. Confini di IP (cosa possiamo e non possiamo riusare)

Il proprietario potrebbe **vendere** il gioco: teniamo la linea netta.

**Possiamo (sono fatti liberi)**:
- I **pattern** di movimento e attacco descritti qui (inseguitore, mosca
  vagante, cecchino, caricatore, divisore, bombardiere, sparo ad anello,
  mini-boss a fasi): sono meccaniche/algoritmi, non protetti.
- Le **formule e i passi d'algoritmo**: anello `a = s*2pi/n + fase`, kiting per
  distanza, animazione `sinf/cosf(GetTime())` con fase per-parte, macchina a
  stati per fasce di HP. Sono matematica.
- La **forma architetturale** ispirata all'API di modding di Isaac (una
  callback per-frame sul nemico, stato persistente esposto allo script,
  primitive spara/muovi/cerca-bersaglio). Prendiamo l'*idea di struttura*, non
  il codice.

**NON possiamo**:
- **Nomi** di nemici, oggetti, personaggi o boss di quel gioco; testi di
  flavour; prosa delle wiki copiata; **sprite/asset**; **tabelle di ID
  numerici** prese di peso. Per questo qui i nemici hanno nomi *generici e
  descrittivi* ("cecchino stazionario", "Colosso Palustre") e i numeri sono
  nostri/di esempio, non estratti da tabelle altrui.
- Copiare l'implementazione C di quel gioco. Reimplementiamo i *pattern*
  (fatti) con parole e codice nostri.

Regola operativa per l'LLM (gia' in APPUNTI §5): il prompt e' fortemente
tipizzato e obbliga a **compilare template prefissati** — cosi' non solo la
sandbox e' sicura, ma anche l'output resta descrizione di *pattern generici*,
mai riproduzione di contenuti protetti.

---

### Fonti

- IsaacDocs — Mod Callbacks (`MC_NPC_UPDATE`, `MC_PRE/POST_NPC_UPDATE`,
  `MC_POST_NPC_INIT`, `MC_POST_NPC_DEATH`, `MC_PRE_NPC_COLLISION`):
  <https://wofsauge.github.io/IsaacDocs/rep/enums/ModCallbacks.html>
- IsaacDocs — modello `EntityNPC` (`State`/`StateFrame`, `I1/I2`, `V1/V2`,
  helper `FireProjectiles`/`FireBossProjectiles`/`CalcTargetPosition`/
  `Pathfinder`): <https://wofsauge.github.io/IsaacDocs/rep/EntityNPC.html>
- Codice del progetto: `src/gameplay/combat.c` (`CombatUpdateEnemies`),
  `src/script/script_api.c` (API a handle), `src/script/script_items.c`
  (patto della sandbox e ciclo callback), `src/core/game_types.h` (`Enemy`,
  cap, trait), `docs/APPUNTI.md` §3 (part system del boss).
