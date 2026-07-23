---
id: ref-meccaniche-callback-e-cache
title: Il modello a callback e il sistema delle cache (da Isaac, per Melting Run)
domain: references
status: approved
authority: supporting
owner: design
summary: >-
  Mappa il modello MC_* di Isaac sulle callback Lua esistenti (on_evaluate/on_fire/on_hit/on_tick) e propone tre nuove callback (on_enemy_update, on_room_enter, on_synergy) con firme e punti di invocazione.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [callback-lua, evaluate-cache, cacheflag, recompute-from-zero, on-enemy-update, on-room-enter, isaac-reference]
related: []
supersedes: []
source_files: []
---

# Il modello a callback e il sistema delle cache (da Isaac, per Melting Run)

> Documento di riferimento. Mappa il modello a callback della modding API di
> *The Binding of Isaac: Repentance* (fonte: wofsauge IsaacDocs,
> <https://wofsauge.github.io/IsaacDocs/rep/>) sull'architettura reale di
> Melting Run: le callback Lua sandboxate `on_evaluate` / `on_fire` / `on_hit`
> / `on_tick` (`src/script/script_items.{h,c}`, `src/script/script_api.h`) e il
> sistema di ricalcolo delle statistiche da zero
> (`ScriptItemsRecomputeStats`). Guida le fasi **3a-synergy** e **3b**.
>
> Nota IP (ripetuta per esteso in fondo): qui si documentano **meccaniche,
> algoritmi e firme di callback** — che sono fatti e liberamente
> reimplementabili — con nomi **generici e originali**. Non si copiano nomi
> di oggetti/nemici, testi, sprite o tabelle di ID di Isaac.

---

## 1. Perché un modello a callback

Un motore roguelite deve poter aggiungere comportamento *senza* toccare il
codice del motore: ogni oggetto, ogni nemico, ogni sinergia è un piccolo
frammento di logica che il motore chiama nei momenti giusti del suo ciclo di
vita. Isaac risolve questo con un registro di **mod callbacks**: la mod
registra una funzione su una costante `MC_*` e il motore la invoca quando quel
momento si verifica. La logica non "gira da sola", viene *chiamata* dal motore
in un punto ben definito e con argomenti ben definiti.

Melting Run usa esattamente questo modello, con due vincoli in più che Isaac
non ha:

1. **La logica è generata da un LLM 7B locale** (llama.cpp) e quindi non è
   fidata: può sbagliare i conti, produrre `NaN`, o tentare accessi illegali.
2. **La logica gira in una sandbox Lua 5.5** con budget di memoria/istruzioni
   e un *kill-switch* (il "patto di sicurezza"): una callback che solleva un
   errore Lua o sfora il budget viene disabilitata in modo permanente
   (`ScriptSandboxIsDisabled`), e dal frame successivo l'oggetto ripiega sulla
   mini-VM dichiarativa. Il motore C resta sempre il padrone: valida gli
   handle, fa il clamp dei risultati, decide cosa applicare.

Il modello a callback è proprio ciò che rende sicuro tutto questo: ogni
callback ha una **firma stretta** (argomenti numerici o handle, mai
puntatori) e un **punto di invocazione unico** nel motore, quindi la
superficie che il codice non fidato può toccare è piccola e verificabile.

---

## 2. Le callback importanti di Isaac e a cosa servono

Elenco dei `MC_*` rilevanti per Melting Run, con la costante esatta (fatto,
citabile) e lo scopo. Fonte: IsaacDocs, pagine *ModCallbacks* e *CacheFlag*.

| Costante Isaac | Quando viene chiamata | A cosa serve |
|---|---|---|
| `MC_EVALUATE_CACHE` | Una o più volte quando le statistiche del giocatore vengono ri-valutate (es. dopo aver preso un oggetto). Riceve `(player, CacheFlag)`. | Applicare modifiche alle statistiche, familiari, volo, arma. **È il cuore del sistema delle cache** (sezione 4). |
| `MC_POST_PEFFECT_UPDATE` | Ogni frame, per ogni giocatore, dopo la valutazione degli effetti che richiedono controllo continuo. | Effetti "persistenti" che vanno riconfermati ogni frame (aure, contatori, trigger a soglia). |
| `MC_POST_PLAYER_UPDATE` | 60 volte al secondo per ogni giocatore. | Logica continua legata al giocatore (posizione, input derivato, timer). |
| `MC_POST_FIRE_TEAR` | Quando il giocatore spara un proiettile. | Modificare/aumentare i colpi appena creati (split, homing, effetti sul proiettile). |
| `MC_ENTITY_TAKE_DMG` | *Prima* che un danno venga applicato a un'entità. Ritornando `false` il danno viene annullato. | Intercettare/ridurre/annullare danni (scudi, riflessioni, on-hit del giocatore o del nemico). |
| `MC_NPC_UPDATE` | Dopo l'aggiornamento di un NPC (non durante l'animazione di comparsa). | Il "cervello" per-frame di un nemico: movimento, scelta dell'attacco, pattern. |
| `MC_POST_NPC_INIT` | Subito dopo l'inizializzazione di un NPC. | Configurare hp/varianti/stato iniziale del nemico appena creato. |
| `MC_POST_NEW_ROOM` | Dopo essere entrati in una stanza (sempre prima di `MC_POST_NEW_LEVEL`). | Reagire al cambio stanza: reset di stato, effetti "all'ingresso", ricalcoli legati alla stanza. |

Costanti citate per completezza ma **non** al centro di questo documento:
`MC_POST_RENDER` (disegno per-frame), `MC_POST_UPDATE` (update globale 30/s),
`MC_USE_ITEM` (uso di un oggetto attivo), `MC_PRE_ENTITY_SPAWN` (modifica di
un'entità prima dello spawn).

---

## 3. Mappatura sulle callback esistenti di Melting Run

Melting Run oggi espone **quattro** callback Lua per oggetto (definite come
funzioni globali nello script; `script_items.c` ne mette in cache il
riferimento con `luaL_ref` all'acquisto in `ScriptItemsOnAcquire`). Ecco la
corrispondenza con i `MC_*` di Isaac e la loro invocazione reale nel motore.

### 3.1 Callback già presenti

| Callback Melting Run | Firma | Invocata da (C) | Analogo Isaac | Ruolo |
|---|---|---|---|---|
| `on_evaluate(stats)` | tabella `stats` mutabile: `damage`, `fire_delay`, `shot_speed`, `shot_radius`, `speed`, `max_hp` | `ScriptItemsRecomputeStats` → `ScriptItemsCallEvaluate` | `MC_EVALUATE_CACHE` | Modificatori di statistiche. È il **cuore della cache** (sezione 4). |
| `on_fire(x, y, dx, dy)` | posizione e direzione del colpo | `ScriptItemsOnFire` | `MC_POST_FIRE_TEAR` | Comportamento allo sparo (spawn di colpi extra via `spawn_shot`, split, deviazioni). Solo per `ITEM_ACTIVE` (mai per `ITEM_STATUP`). |
| `on_hit(shot_id, enemy_id)` | handle del colpo e del nemico colpiti | `ScriptItemsOnHit` | `MC_ENTITY_TAKE_DMG` (lato attaccante) | Effetti on-hit (danno extra via `damage_enemy`, rallentamento via `set_enemy_velocity`, burn/slow). Solo `ITEM_ACTIVE`. |
| `on_tick(dt)` | delta tempo del frame | `ScriptItemsOnTick` | `MC_POST_PEFFECT_UPDATE` / `MC_POST_PLAYER_UPDATE` | Logica continua per-frame dell'oggetto (aure, contatori, timer). Solo `ITEM_ACTIVE`. |

Dettagli di sicurezza già implementati che il lettore deve conoscere prima di
estendere il sistema:

- **Gli handle non sono puntatori.** `on_hit` riceve `shot_id`/`enemy_id` come
  numeri (indice + generazione impacchettati, `ScriptApiPackShotHandle` /
  `ScriptApiPackEnemyHandle`). Lua non vede mai un `Enemy*`, quindi non può
  causare use-after-free su un nemico che nel frattempo è morto: l'handle
  viene rivalidato a ogni uso lato C.
- **Difesa in profondità sulla tassonomia.** Per un oggetto `ITEM_STATUP`,
  `script_items.c` **non** mette mai in cache `on_fire`/`on_hit`/`on_tick`,
  anche se lo script Lua le definisse (manifest manomesso). Solo `on_evaluate`
  è ammessa per uno stat-up. "Gli stat-up non hanno comportamento" resta vero
  anche bypassando il generatore.
- **Ogni chiamata passa dal budget.** `ScriptItemsCallCachedVoid` e
  `ScriptItemsCallEvaluate` invocano sempre `ScriptSandboxProtectedCall`: se lo
  script sfora o solleva errore, viene disabilitato e saltato dal frame dopo.

### 3.2 Callback da AGGIUNGERE (fasi 3a-synergy e 3b)

Le tre callback seguenti **non esistono ancora**. Questo documento ne fissa la
firma e il punto di invocazione perché le fasi successive le implementino in
modo coerente con quanto sopra.

#### `on_enemy_update(enemy_id, dt)` — analogo di `MC_NPC_UPDATE`

Il "cervello" per-frame di un nemico o di una **parte** di boss (Part System,
`docs/APPUNTI.md` §3). Oggi i nemici (`ENEMY_CHASER`, `ENEMY_SHOOTER`,
`ENEMY_TANK`, `ENEMY_BOSS`) sono guidati interamente da C in `combat.c`. La
callback permette all'LLM di scrivere pattern di attacco generati (es. il
`script_attacco_lua` di una `MonsterPart`).

- **Firma:** `on_enemy_update(enemy_id, dt)`. `enemy_id` è un handle
  (stesso schema di `on_hit`), `dt` un numero.
- **Invocazione:** in `combat.c`, nel loop di update dei nemici, dopo il
  movimento C di base e prima della risoluzione delle collisioni. Un nemico
  con `script_attacco_lua` non vuoto chiama la sua callback; gli altri restano
  100% C.
- **API di gioco leggibile:** `player_x/y`, `enemy_x/y(id)`, `nearest_enemy`,
  `rng`; **scrivibile:** `spawn_shot`, `set_enemy_velocity`, `damage_enemy`.
  Nessun nuovo diritto rispetto a quanto la sandbox già espone
  (`script_api.h`): riusa la stessa superficie, così la validazione non cambia.
- **Sicurezza:** stesso patto — un `on_enemy_update` che sfora disabilita solo
  *quella* parte/nemico, che ripiega su un movimento C banale (es. inseguire il
  giocatore). Un boss non diventa mai ingiocabile o invulnerabile per colpa di
  uno script rotto.

#### `on_room_enter(room_kind)` — analogo di `MC_POST_NEW_ROOM`

Reagire all'ingresso in una stanza (`ROOM_COMBAT`, `ROOM_TREASURE`,
`ROOM_SHOP`, `ROOM_BOSS`, ...). Utile per effetti "all'ingresso" (cura in stanza
tesoro, buff temporaneo in stanza boss, contatori che si azzerano a stanza).

- **Firma:** `on_room_enter(room_kind)`, con `room_kind` passato come numero
  (il valore dell'enum `RoomKind`, `game_types.h`).
- **Invocazione:** in `world.c`, nella transizione di stanza, **dopo** che la
  nuova `RoomState` è attiva ma **prima** del primo frame di combat, così ogni
  effetto legato alla stanza è già applicato quando parte il gameplay. Isaac
  garantisce lo stesso ordinamento (`MC_POST_NEW_ROOM` sempre prima di
  `MC_POST_NEW_LEVEL`): replichiamo la garanzia — se aggiungeremo un futuro
  `on_floor_enter`, `on_room_enter` verrà **prima**.
- **Attenzione alla cache:** se un `on_room_enter` cambia una statistica, deve
  farlo impostando `game->statsDirty = true` (non scrivendo `player.damage`
  direttamente), così il ricalcolo da zero della sezione 4 resta l'unica
  strada verso le statistiche. Vedi §4.5.

#### `on_synergy(item_ids)` / hook di sinergia — nessun `MC_*` diretto

Questa non ha un analogo 1:1 in Isaac (in Isaac le sinergie sono per lo più
codificate a mano nel motore). È la callback centrale della **fase 3a-synergy**:
data la lista degli oggetti posseduti, produrre modificatori *combinati* che
nessun singolo oggetto potrebbe esprimere da solo. Corrisponde all'idea di
`onEvaluateSynergy(player)` abbozzata in `docs/APPUNTI.md` §6C.

- **Modello consigliato — NON una quinta callback separata, ma una fase di
  `on_evaluate`.** La sinergia è, matematicamente, ancora un modificatore di
  statistiche: appartiene alla stessa passata di ricalcolo. La proposta è:
  1. Ogni script può definire `on_synergy(stats, has)` dove `has` è una
     funzione/tabella che risponde a "il giocatore possiede un oggetto con il
     trait/archetipo X?" (astrazione **generica**: si interroga per *trait* o
     *archetipo*, mai per nome copiato).
  2. `ScriptItemsRecomputeStats` esegue `on_synergy` **dopo** tutti gli
     `on_evaluate` individuali, in una passata finale, così può leggere lo
     stato "quasi finito" e aggiungere il bonus di combinazione.
  3. Il risultato passa comunque per `ScriptItemsClampStats` (tetto globale) e,
     dove ha senso, per il budget per-rarità.
- **Perché così e non una callback a evento:** tenerla dentro il ricalcolo da
  zero preserva l'**idempotenza** (sezione 4.4). Una sinergia calcolata "una
  volta al pickup" andrebbe in deriva quando si aggiunge/rimuove un oggetto;
  ricalcolata da zero ogni volta, no.

---

## 4. Il sistema delle cache: ricalcolo da zero, in profondità

Questa è la sezione centrale. Melting Run implementa già una versione semplice
del pattern di `MC_EVALUATE_CACHE`; qui la si spiega a fondo e si indicano i
punti di estensione per le fasi future.

### 4.1 L'idea di Isaac: statistiche = pura funzione degli oggetti

In Isaac ogni statistica del giocatore (danno, fire delay, shot speed, range,
velocità, luck, ...) **non** viene mutata in modo incrementale quando prendi un
oggetto. Invece, quando qualcosa cambia, il motore **azzera la statistica al
suo valore base e riapplica da capo il contributo di ogni oggetto/effetto**.
Questo giro si chiama *evaluate cache*.

Per non ricalcolare *tutto* ogni volta, Isaac marca *quali* statistiche sono
"sporche" con un bitmask, l'enum **`CacheFlag`** (fatto, citabile):

| Flag Isaac | Bit | Statistica |
|---|---|---|
| `CACHE_DAMAGE` | `1<<0` | danno |
| `CACHE_FIREDELAY` | `1<<1` | cadenza di fuoco |
| `CACHE_SHOTSPEED` | `1<<2` | velocità del colpo |
| `CACHE_RANGE` | `1<<3` | gittata |
| `CACHE_SPEED` | `1<<4` | velocità di movimento |
| `CACHE_TEARFLAG` | `1<<5` | proprietà del colpo |
| `CACHE_TEARCOLOR` | `1<<6` | aspetto del colpo |
| `CACHE_FLYING` | `1<<7` | volo |
| `CACHE_WEAPON` | `1<<8` | tipo di arma |
| `CACHE_FAMILIARS` | `1<<9` | familiari |
| `CACHE_LUCK` | `1<<10` | fortuna |
| `CACHE_SIZE` | `1<<11` | dimensione del personaggio |
| `CACHE_COLOR` | `1<<12` | colore del personaggio |
| `CACHE_PICKUP_VISION` | `1<<13` | previsione dei drop |
| `CACHE_ALL` | `(1<<16)-1` | tutte le statistiche |
| `CACHE_TWIN_SYNC` | `1<<31` | sincronizzazione gemelli |

La callback `MC_EVALUATE_CACHE` riceve `(player, CacheFlag)` e viene invocata
**una volta per flag sporco**. La mod controlla *quale* flag è impostato e
modifica *solo* la statistica corrispondente:

```lua
-- schema del pattern Isaac (nomi generici)
function mod:onEvaluateCache(player, flag)
    if flag == CACHE_DAMAGE   then player.Damage   = player.Damage + 1.0 end
    if flag == CACHE_FIREDELAY then /* modifica MaxFireDelay */ end
end
```

Due proprietà rendono il pattern robusto:

1. **Riparte sempre dal valore base**, quindi l'ordine in cui prendi gli
   oggetti non produce deriva.
2. **È idempotente:** rieseguire la valutazione con lo stesso set di oggetti dà
   sempre lo stesso risultato.

### 4.2 Come Melting Run lo implementa oggi

`ScriptItemsRecomputeStats` (`src/script/script_items.c`) è la nostra
`MC_EVALUATE_CACHE`. La struttura è deliberatamente più semplice di Isaac —
**non abbiamo (ancora) i bit di `CacheFlag`**, ricalcoliamo *tutte* e sei le
statistiche in un colpo — ma la logica di fondo è identica:

```
acc = { player.base* }                     // 1. parti dal BASE (0 oggetti)
per ogni oggetto i posseduto, in ordine di acquisizione:
    ScriptItemsApplyBuiltin(&acc, item)    // 2a. modificatori "built-in" (trait/slot)
    ScriptItemsClampStats(&acc)            //     clamp ai confini globali
    se l'oggetto ha on_evaluate Lua vivo:
        pre = acc
        acc = on_evaluate(acc)             // 2b. lo script muta la tabella stats
        se ITEM_STATUP: clamp del DELTA per-rarità rispetto a pre
        ScriptItemsClampStats(&acc)        //     clamp globale di nuovo
    altrimenti se ITEM_STATUP senza Lua:
        ScriptItemsApplyStatUpFallback     // 2c. "mai un dud": bonus fisso
        clamp delta + clamp globale
player.damage/fireDelay/... = acc          // 3. scrivi il risultato in blocco
if player.hp > player.maxHp: player.hp = player.maxHp
```

Il ricalcolo è pilotato da un flag globale `game->statsDirty`: chi cambia
l'inventario (`ScriptItemsOnAcquire`) lo alza; `ScriptItemsProcessDirty`, una
volta per frame in cima a `GameUpdate`, lo consuma e chiama il ricalcolo
**prima** che `CombatUpdatePlayer` legga `player.damage` ecc. `statsDirty` è il
nostro equivalente "grosso" del bitmask `CacheFlag`: un unico bit "qualcosa è
cambiato" invece di un bit per statistica.

### 4.3 Ordine di applicazione (e perché conta)

L'ordine dentro il loop **non è arbitrario**, ed è parte del contratto che gli
script generati devono poter assumere:

1. **Base prima di tutto.** `acc` parte da `player.base*` (danno 8, fireDelay
   0.23, shotSpeed 520, shotRadius 5, speed 224, maxHp 6). Nessuno stato
   residuo dal frame precedente.
2. **Built-in prima di Lua, per ogni oggetto.** Prima si applicano i
   modificatori "duri" derivati da trait/slot (`ScriptItemsApplyBuiltin`,
   moltiplicatori e addendi fissi), poi lo script Lua di *quello stesso*
   oggetto vede il risultato e può raffinarlo. Così un `on_evaluate` generato
   lavora su valori già sensati, non su un base "grezzo".
3. **Clamp dopo ogni passo, non solo alla fine.** `ScriptItemsClampStats`
   viene chiamato dopo il built-in **e** dopo il Lua di ogni oggetto. Questo
   impedisce a un oggetto intermedio malfatto di far esplodere `acc` a valori
   che poi, moltiplicati da un oggetto successivo, uscirebbero comunque
   dall'intervallo anche dopo un clamp finale.
4. **Oggetti in ordine di acquisizione.** Il loop scorre `player.items[0..n]`.
   L'ordine è deterministico (ordine di raccolta), quindi due run con lo stesso
   seed producono lo stesso risultato. Poiché ogni passo riparte da `acc` e fa
   clamp, l'ordine influenza il risultato **solo** per operazioni non
   commutative (moltiplicazioni miste ad addizioni con clamp intermedio) — ed
   è per questo che l'ordine dev'essere deterministico.
5. **La passata di sinergia va per ultima** (fase 3a-synergy, §3.2): legge lo
   stato quasi finito e aggiunge il bonus di combinazione, poi un ultimo clamp.

### 4.4 Perché è idempotente e sicuro per modificatori generati

Questa è la ragione per cui il pattern è la scelta giusta proprio *perché* i
modificatori li scrive un 7B non fidato.

- **Idempotenza = niente accumulo.** Uno script sbagliato che facesse
  `stats.damage = stats.damage + 5` NON accumula run dopo run: a ogni ricalcolo
  `stats.damage` riparte dal valore di *questo* giro (base + built-in + oggetti
  precedenti), mai dal totale del frame prima. Il peggio che quello script può
  fare è aggiungere 5 *una volta per ricalcolo*, non 5 per frame all'infinito.
  Un modello incrementale (mutare direttamente `player.damage`) non avrebbe
  questa protezione: ogni frame lo script sommerebbe altri 5.
- **Aggiungere/rimuovere un oggetto è privo di deriva.** Poiché si riparte
  sempre da zero, togliere un oggetto (o disabilitarne lo script per il patto
  di sicurezza) rimuove *esattamente* il suo contributo, senza residui. Non
  serve una logica di "undo" per oggetto — che con codice generato sarebbe
  impossibile da fidarsi.
- **Il confine Lua→C è presidiato due volte.** In `ScriptItemsCallEvaluate`:
  (a) i campi riletti dalla tabella passano un test `isfinite` — `NaN`/`±inf`
  (che `lua_isnumber` accetta come "numeri" Lua) vengono **scartati**, e `acc`
  tiene il valore che aveva *prima* della chiamata; (b) subito dopo,
  `GameMathClampFloat` è reso NaN-safe. Due reti indipendenti.
- **Rollback implicito su errore.** Se `on_evaluate` viene ucciso a metà
  chiamata, `ScriptItemsCallEvaluate` ritorna `false` e `acc` **non viene
  toccato**: qualunque scrittura parziale che lo script avesse fatto sulla
  tabella condivisa non raggiunge mai le statistiche vere. La tabella di
  scratch è riusata (una `luaL_ref` per oggetto), ma viene riscritta da C con i
  valori correnti *prima* di ogni chiamata, quindi lo stato sporco di un giro
  fallito non sopravvive al giro dopo.
- **Doppio tetto: globale + per-oggetto.** `ScriptItemsClampStats` è un tetto
  *globale* sull'intero giocatore (es. `damage ∈ [0.5, 200]`). In più, per gli
  `ITEM_STATUP`, `ScriptItemsClampItemDelta` limita *quanto un singolo oggetto*
  può spostare una statistica in un colpo, come frazione del **base**
  (`baseDamage`, non del valore corrente), scalata per rarità:

  ```c
  SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION = {
      0.15f,  /* RARITY_COMMON     */
      0.25f,  /* RARITY_UNCOMMON   */
      0.40f,  /* RARITY_RARE       */
      0.60f,  /* RARITY_LEGENDARY  */
  };
  ```

  Relativizzare al base (e non al corrente) impedisce che il decimo oggetto
  raccolto possa spostare la statistica più del primo solo perché i nove
  precedenti l'hanno già gonfiata. Il tetto globale gira **sempre** dopo,
  qualunque sia la rarità.
- **"Mai un dud".** Un `ITEM_STATUP` il cui script non è mai stato generato,
  è stato bocciato dalla validazione, o è stato ucciso a runtime, prende
  comunque un bonus fisso e prevedibile via `ScriptItemsApplyStatUpFallback`,
  scelto in base al suo trait (nessuna RNG: stesso trait → stesso bonso),
  scalato per rarità e poi clampato. Un premio di boss non è mai vuoto anche se
  l'LLM ha fallito completamente.

### 4.5 Regola d'oro per le nuove callback

**Nessuna callback deve scrivere `player.damage`/`fireDelay`/... direttamente.**
L'unica strada verso le statistiche è il ricalcolo da zero. Una callback che
vuole cambiare una statistica (una sinergia, un buff `on_room_enter`, un effetto
`on_tick`) deve:

1. registrare la sua intenzione in uno stato che il ricalcolo legge (un
   modificatore in `on_evaluate`/`on_synergy`, oppure un flag/contatore di
   stato dell'oggetto), e
2. alzare `game->statsDirty = true`.

Così il ricalcolo resta l'unico produttore di statistiche, e tutte le
protezioni della sezione 4.4 (idempotenza, clamp, isfinite, rollback)
continuano a valere *automaticamente* anche per le callback che aggiungeremo.
Scrivere direttamente la statistica scavalcherebbe tutte queste reti — ed è
esattamente l'errore che il modello a cache di Isaac esiste per prevenire.

### 4.6 Estensione futura: introdurre i veri `CacheFlag`

Oggi `statsDirty` è un unico bit "ricalcola tutto". Se il profiling mostrerà
che il ricalcolo completo pesa (improbabile con ≤ ~pochi oggetti e sei
statistiche, ma la porta resta aperta), il passo naturale è replicare
`CacheFlag`: sostituire il `bool statsDirty` con un bitmask `dirtyStats`, far
sì che `on_evaluate` dichiari quali campi tocca, e ricalcolare solo le
statistiche marcate. La forma dei dati (tabella `stats` con sei campi nominati)
è già compatibile: basta associare a ciascun campo un bit e ricalcolare per
sottoinsieme. Finché non serve, l'approccio "ricalcola tutte e sei" è più
semplice e altrettanto corretto — l'idempotenza vale identica.

---

## 5. Riepilogo della mappatura

| Isaac (`MC_*`) | Melting Run oggi | Da aggiungere | Punto di invocazione C |
|---|---|---|---|
| `MC_EVALUATE_CACHE` | `on_evaluate(stats)` | passata `on_synergy` finale | `ScriptItemsRecomputeStats` |
| `MC_POST_FIRE_TEAR` | `on_fire(x,y,dx,dy)` | — | `ScriptItemsOnFire` |
| `MC_ENTITY_TAKE_DMG` | `on_hit(shot_id,enemy_id)` | — | `ScriptItemsOnHit` |
| `MC_POST_PEFFECT_UPDATE` / `MC_POST_PLAYER_UPDATE` | `on_tick(dt)` | — | `ScriptItemsOnTick` |
| `MC_NPC_UPDATE` | — | `on_enemy_update(enemy_id, dt)` | loop nemici in `combat.c` |
| `MC_POST_NEW_ROOM` | — | `on_room_enter(room_kind)` | transizione stanza in `world.c` |
| (nessuno) | — | `on_synergy(stats, has)` | fine di `ScriptItemsRecomputeStats` |

---

## 6. Confini di IP

- **Cosa possiamo riusare (fatti):** il *modello a callback* (registrare
  logica su punti di invocazione del motore), le *firme* e gli *scopi* delle
  callback, e in particolare l'**algoritmo del "recompute-from-zero" / evaluate
  cache** con i suoi bit di `CacheFlag` — sono meccaniche, regole e algoritmi,
  liberamente reimplementabili. I nomi delle costanti `MC_*` e `CACHE_*` sono
  citati qui **come riferimento tecnico alla documentazione pubblica**, non
  copiati dentro il codice del gioco: nel motore usiamo nomi nostri
  (`on_evaluate`, `statsDirty`, `ScriptItemsRecomputeStats`, ecc.).
- **Cosa NON possiamo copiare:** nomi di oggetti, personaggi, nemici o
  familiari di Isaac; testi di flavour; prosa delle wiki; sprite/asset; tabelle
  di ID numeriche prese di peso. Le sinergie e i comportamenti generati devono
  usare descrizioni **generiche e originali** (per *trait*/*archetipo*, non per
  nome di marchio): "un colpo che rimbalza e poi insegue il nemico più vicino",
  non il nome commerciale dell'oggetto che fa quella cosa in Isaac.
- **Regola pratica per il generatore LLM:** il prompt deve chiedere
  comportamenti descritti per meccanica (formula, trait, effetto) e nomi
  inventati; mai chiedere "riproduci l'oggetto X di Isaac". Le formule e i
  passi algoritmici sono fatti e vanno inclusi con precisione, citando la fonte
  (IsaacDocs); i nomi restano nostri.

**Fonte:** wofsauge IsaacDocs (Repentance), pagine *ModCallbacks* e
*CacheFlag* — <https://wofsauge.github.io/IsaacDocs/rep/>.
