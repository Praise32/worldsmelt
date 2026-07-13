# Design delle sinergie implicite (il prossimo passo)

Data: 2026-07-13
Fase: 3c (dopo tassonomia oggetti, personaggio a strati, rarita'/pool)
Stato: documento di riferimento — la decisione di alto livello e' gia' presa
(sinergie IMPLICITE alla Isaac, vedi
`docs/superpowers/specs/2026-07-13-pools-rarity-design.md` sez. 7 e
`2026-07-13-items-synergy-vision.md` sez. 4). Qui progettiamo il *come*.

Questo file spiega prima come funzionano concettualmente le sinergie implicite
in un roguelike alla Isaac (con le fonti), poi le mappa **concretamente** sul
motore che abbiamo gia' (la cache di ricalcolo delle statistiche, i trait bitmask
sui colpi, la sandbox Lua a handle), e infine propone la **prima versione minima**
da costruire.

---

## 1. Cosa vuol dire "sinergia implicita"

Abbiamo scelto il modello **A — implicito** (vedi la visione oggetti, sez. 4):

- Gli oggetti restano **separati** nell'inventario. Non si fondono, non si
  consumano.
- Quando l'inventario contiene una **coppia compatibile**, il gioco *aggiunge*
  un effetto combinato (e, dopo, un tocco visivo a strati).
- La profondita' non nasce dal singolo oggetto (che resta semplice e leggibile,
  un solo effetto) ma dalle **combinazioni** man mano che la build cresce.

E' il modello del "Binding of Isaac": nessun oggetto sa dell'altro, ma i loro
effetti si sommano sullo stesso colpo/sulla stessa statistica, e da questa somma
emerge un comportamento nuovo.

---

## 2. Come funziona, concettualmente, in Isaac (i fatti)

Due meccanismi rendono le sinergie di Isaac *componibili per costruzione*. Sono
fatti di design, li citiamo perche' li reimplementiamo (non copiamo nomi/asset —
vedi "Confini di IP" in fondo).

### 2.1 La cache di valutazione delle statistiche (`MC_EVALUATE_CACHE`)

Il fatto chiave: **le statistiche si ricalcolano SEMPRE da zero.** Quando il gioco
rileva che una statistica *potrebbe* essere cambiata (hai preso un oggetto, un
effetto e' scaduto…), rifa' l'intero calcolo di quella statistica: azzera al
valore base e poi **riapplica in ordine il modificatore di ogni oggetto
posseduto**.

> "Since previously applied stat changes are reset at the start of each stat
> evaluation, you should reapply them again each time." — Isaac Blueprints,
> *Stats Cache*.

Il callback `MC_EVALUATE_CACHE` gira subito dopo una valutazione, ricevendo il
giocatore e un `CacheFlag` (un bitfield che dice *quale* statistica sta venendo
ricalcolata: danno, cadenza, velocita', numero di colpi…). L'ordine di
applicazione dei modificatori e' fisso:

1. modifiche "normali" (additive di base);
2. conversione della statistica interna nella sua versione mostrata;
3. moltiplicatori "NormBreak";
4. modificatori flat;
5. moltiplicatori.

Perche' questo abilita le sinergie: siccome ogni oggetto **riapplica** il suo
contributo a ogni ricalcolo, aggiungere/togliere un oggetto non lascia residui e
non fa "accumulare" nulla. Il risultato e' sempre la somma pulita di tutti i
contributi presenti *ora*. E' esattamente la proprieta' che serve perche' una
sinergia (un contributo condizionale "se hai A e B") sia solo **un modificatore
in piu' nella stessa lista**.

Fonti:
- Isaac Blueprints, *Stats Cache* — https://isaacblueprints.com/tutorials/basics/stats_cache/
- BoI Lua API Docs, *ModCallbacks* (`MC_EVALUATE_CACHE`) — https://wofsauge.github.io/IsaacDocs/rep/enums/ModCallbacks.html

### 2.2 I flag sui proiettili (tear flags) — bit indipendenti sullo stesso colpo

L'attacco base e' un proiettile che porta un **insieme di flag bitmask**
indipendenti: inseguimento, perforazione, rimbalzo, attraversamento dei muri
("spettrale"), esplosione, veleno, bruciatura, rallentamento, divisione… Ogni
oggetto-modificatore accende uno o piu' di questi bit sul colpo.

La sinergia e' *emergente*: siccome i bit sono indipendenti e vivono sullo stesso
proiettile, tenere due modificatori diversi produce un colpo che ha **entrambi**
i comportamenti — senza che nessuno abbia scritto codice per quella coppia
specifica. Esempi generici (descritti come pattern, non come asset di marca):

- **inseguimento + perforazione**: un colpo che curva verso il nemico piu' vicino
  *e* lo trapassa continuando verso il prossimo.
- **fuoco/bruciatura + rimbalzo**: un colpo che rimbalza sulle pareti lasciando
  danno nel tempo a ogni bersaglio toccato.
- **esplosione + divisione**: al primo impatto il colpo si spezza in piu' colpi,
  ciascuno dei quali esplode.
- **rallentamento + cadenza alta**: molti colpi deboli che tengono i nemici
  costantemente lenti.

Alcune coppie non sono semplice unione di bit ma **override con priorita'**: un
modificatore "forte" (es. quello che trasforma i colpi in un raggio continuo) puo'
sostituire il tipo di attacco invece di sommarsi, e la sua interazione con un
altro modificatore forte segue una gerarchia definita a mano. Questo e' il caso
che nel nostro motore modelleremo come **regola esplicita**, non come semplice OR
di bit (vedi sez. 4.3).

Fonti (categorie/esempi di sinergia, in parole nostre):
- Platinum God, *Item Synergies* — https://tboi.com/synergies
- The Binding of Isaac: Rebirth Wiki, *Synergies* — https://thebindingofisaacrebirth.wiki/items/synergies/

---

## 3. Perche' il NOSTRO motore e' gia' pronto per questo

Buona notizia (gia' anticipata nelle spec): abbiamo, senza volerlo, ricostruito
entrambi i meccanismi di Isaac. Le sinergie sono per lo piu' *cablaggio*, non
motore nuovo.

### 3.1 La nostra cache di ricalcolo = `MC_EVALUATE_CACHE`

`ScriptItemsRecomputeStats` (`src/script/script_items.c`,
firma in `script_items.h`) fa **esattamente** il ricalcolo-da-zero di Isaac:

- riparte sempre dai `player.base*` (`baseDamage`, `baseFireDelay`,
  `baseShotSpeed`, `baseShotRadius`, `baseSpeed`, `baseMaxHp` in
  `core/game_types.h`);
- riapplica, in ordine, per ogni oggetto posseduto, prima i suoi modificatori
  built-in (trait/slot) e poi — se ha Lua attivo — la sua `on_evaluate(stats)`;
- **clampa per campo** dopo ogni passo (i confini di sicurezza), cosi' nessuno
  script puo' produrre un giocatore ingiocabile;
- e' pilotato dalla bandiera `Game.statsDirty`, consumata una volta per frame da
  `ScriptItemsProcessDirty` — il nostro equivalente del "rileva un possibile
  cambiamento e ricalcola".

Questa idempotenza e' precisamente cio' che serve: **una sinergia e' solo un
contributo condizionale in piu' dentro questo ricalcolo.**

### 3.2 I nostri trait = le tear flags

`Shot.traits` (`core/game_types.h`) e' gia' un bitmask indipendente:
`TRAIT_BOUNCE`, `TRAIT_HOMING`, `TRAIT_EXPLODE`, `TRAIT_SPLIT`, `TRAIT_PIERCE`,
`TRAIT_RAPID`, `TRAIT_GIANT`, `TRAIT_SLOW`, `TRAIT_VAMP`. Il colpo trasporta i
suoi flag (`Shot.bounces`, `Shot.pierce`, `Shot.splitDone`…), la pipeline di
combattimento in `combat.c` li applica in modo indipendente, e le stesse costanti
sono gia' esposte a Lua (`ScriptApiRegisterTraitConstant`, `src/script/script_api.c`).
Aggiungere un bit a un colpo appena creato = accendere una sinergia sul colpo,
identico a Isaac.

### 3.3 L'API di gioco a handle, per gli effetti attivi

Le funzioni gia' registrate nell'`_ENV` della sandbox (`script_api.c`) bastano a
esprimere quasi ogni effetto di sinergia:
`player_x/y`, `player_hp/max_hp/damage`, `player_item_count`,
**`player_has_item`**, `enemy_x/y/hp`, `nearest_enemy`, `shot_x/y`,
`room_left/top/right/bottom`, `spawn_shot`, `damage_enemy`, `heal_player`,
`set_enemy_velocity`, `add_particle`, piu' `rng()` e le costanti `TRAIT_*`.

`player_has_item` e' la primitiva di rilevamento delle coppie gia' pronta.

---

## 4. Il design del nostro sistema di sinergie implicite

### 4.1 Rilevare una coppia compatibile dai metadati

Una sinergia scatta quando l'inventario contiene due (o piu') oggetti che
combaciano su una **regola**. I metadati su cui decidere ci sono gia' quasi tutti
in `Item` (`core/game_types.h`):

| Segnale | Campo | Ruolo nella regola di sinergia |
|---|---|---|
| Tipo | `kind` (`ITEM_ACTIVE`/`ITEM_STATUP`) | filtra: le sinergie interessanti sono fra **attivi**; uno stat-up e' solo numeri. |
| Rarita' | `rarity` (4 livelli) | scala la **potenza** della sinergia (sez. 4.5) e puo' essere un pre-requisito. |
| Trait | `traits` (bitmask) | il segnale **primario** e persistente: "oggetto-rimbalzo" = `traits & TRAIT_BOUNCE`. |
| Slot visivo | `slot` | per la fusione visiva a strati (fase successiva). |
| Archetipo | *(oggi non persistito)* | vedi sotto. |

**Buco da chiudere: l'archetipo non e' un campo di `Item`.** Oggi l'archetipo
(rimbalzo, inseguimento, laser, famiglio, perforazione, rallentamento…) vive solo
nel *prompt* di generazione (`tools/melting-gen/prompts/lua_system.txt`, la
"tavolozza di archetipi") e si riflette a runtime solo indirettamente nei
`traits` e nella callback scelta. Per far decidere una sinergia in modo
**deterministico e testabile**, conviene persistere l'archetipo come piccolo enum
dichiarato sull'oggetto:

```c
typedef enum ItemArchetype {
    ARCH_NONE = 0,     /* default: oggetti vecchi/azzerati "{0}" restano neutri */
    ARCH_BOUNCE,
    ARCH_HOMING,
    ARCH_BEAM,         /* laser/raggio */
    ARCH_FAMILIAR,     /* famiglio/orbitale periodico */
    ARCH_PIERCE,
    ARCH_SLOW,
    ARCH_BURN,         /* danno nel tempo */
    ARCH_COUNT
} ItemArchetype;
```

Con `ARCH_NONE = 0` seguiamo la stessa convenzione difensiva gia' usata per
`kind` e `rarity`: un `Item` azzerato con `{0}` o un manifest vecchio senza riga
`archetype=` resta **neutro**, mai una sinergia per sbaglio. Round-trip nel
manifest come rarita'/kind. In pratica **due segnali coprono il 90% dei casi**:
`traits` (che gia' esiste) e questo `archetype` dichiarato. La regola tipica e'
*"archetipo/trait X presente" AND "archetipo/trait Y presente"*.

### 4.2 Dove vivono le sinergie: tre opzioni

Sono le tre risposte alla domanda "chi conosce la coppia A+B e cosa fa".

**Opzione 1 — Tavola di regole in C.** Una tabella dichiarativa (nello stile
delle tabelle di rarita'/pesi gia' presenti) del tipo:
`{condizione su archetype/traits, effetto}`. Il C, dopo `RecomputeStats` e nella
pipeline dei colpi, controlla le condizioni e applica l'effetto.

- Pro: deterministica, testabile, zero rischio sandbox, sempre disponibile, costo
  quasi nullo. Perfetta per un set curato di sinergie "canoniche" (rimbalzo+fuoco,
  inseguimento+perforazione…).
- Contro: le sinergie sono **fisse**, non nascono dagli oggetti specifici che
  l'LLM ha inventato per *questa* run.

**Opzione 2 — Callback Lua `on_synergy(inventory)` per-oggetto.** Come le altre
callback (`on_evaluate`/`on_fire`/`on_hit`/`on_tick`): un oggetto puo' definire
`on_synergy`, che il C chiama passando (una vista sicura del)l'inventario;
l'oggetto controlla `player_has_item(...)` e, se la coppia c'e', accende il suo
effetto extra.

- Pro: riusa **tutto** l'impianto sandbox (handle, clamp, patto di sicurezza,
  ripiego). L'LLM puo' scrivere la sinergia *insieme* all'oggetto.
- Contro: l'oggetto A deve "sapere" il nome/archetipo di B. Con oggetti generati a
  ogni run i nomi non sono noti a priori — meglio far condizionare su
  **archetipo/trait** (che sono noti) piuttosto che su nomi specifici.

**Opzione 3 — Script di sinergia generati dall'LLM a inizio run.** All'inizio
della run l'LLM conosce **tutti e 15** gli oggetti del piano/della run
(`FloorContent.items[3]` x 5 piani + i `bossItem`). Gli si chiede di produrre un
piccolo insieme di **script di sinergia** per le coppie compatibili che vede,
riusando la stessa sandbox. E' l'idea gia' anticipata nelle spec ("le sinergie fra
i 15 oggetti noti a inizio run si possono pre-calcolare… la logica e' economica
anche a decine di combinazioni").

- Pro: sinergie **su misura** degli oggetti realmente in gioco; sfrutta il vincolo
  "l'LLM lavora prima della run, non durante" (niente VRAM/inferenza a 60 FPS).
- Contro: piu' complesso; dipende dalla qualita' del 7B; serve validazione e
  ripiego robusti.

**Raccomandazione (vedi anche sez. 6): non e' un aut-aut, e' una scala.**
Costruire **prima l'Opzione 1** (tavola C su archetype/traits), che da' subito
sinergie sicure e testabili col minimo codice; poi, quando quella regge, salire
all'Opzione 3 (LLM a inizio run) che riusa lo **stesso punto di applicazione**.
L'Opzione 2 e' il meccanismo di *esecuzione* dell'Opzione 3 (gli script generati
sono, tecnicamente, delle `on_synergy`), non una terza strada separata.

### 4.3 Come applicare una sinergia (i due canali)

Una sinergia produce sempre uno di due tipi di effetto, e ciascuno ha gia' il suo
canale nel motore:

**Canale A — statistico (nella cache di ricalcolo).** La sinergia e' un
contributo condizionale in `ScriptItemsRecomputeStats`, applicato **dopo** i
contributi dei singoli oggetti e **prima** dei clamp finali. Esempio: "se hai un
oggetto-perforazione e uno-inseguimento, +20% danno". Concretamente: dopo aver
riapplicato tutti gli oggetti, il C (Opzione 1) o una `on_synergy(stats,...)`
(Opzione 2/3) legge le condizioni e modifica la `stats` table, poi si clampa.
Idempotente per costruzione: e' solo un altro passo del ricalcolo-da-zero.

**Canale B — comportamentale (nella pipeline dei colpi).** La sinergia accende
**trait sul colpo** o inietta un effetto negli eventi esistenti. Il punto di
innesto naturale e' subito dopo la creazione del colpo del giocatore in
`combat.c`: se le condizioni di sinergia sono attive, si fa l'**OR** dei trait
sul nuovo `Shot.traits` (esattamente la meccanica "tear flags" di Isaac). In Lua,
lo stesso si esprime dentro `on_fire`/`on_hit`/`on_tick` con
`spawn_shot(..., traits)`.

Per le sinergie **override** (sez. 2.2 — due modificatori "forti" che non si
sommano ma seguono una gerarchia) l'unica strada pulita e' l'**Opzione 1**: una
riga esplicita nella tavola C con la priorita' decisa a mano. I bit non bastano a
descrivere "A vince su B": serve una regola.

### 4.4 La firma di `on_synergy` (se/quando andiamo su Lua)

Coerente con le callback esistenti e col patto "Lua non vede mai un puntatore":

```
-- chiamata dal C dentro il ricalcolo (canale A) e/o per-frame (canale B),
-- con la stessa 'stats' table di on_evaluate quando serve modificare statistiche.
function on_synergy(stats)
    -- rileva la coppia SOLO tramite segnali noti a priori:
    --   player_has_item(name)  -> per nome (utile per l'Opzione 3, LLM conosce i nomi)
    --   oppure un futuro player_has_archetype(ARCH_*) / has_trait(TRAIT_*) helper
    if player_has_item("Bracciale del rimbalzo")
       and player_has_item("Lacrima incandescente") then
        stats.damage = stats.damage * 1.15   -- canale A, poi il C clampa
    end
end
```

Per l'Opzione 1 non serve nessuna nuova API: la condizione la valuta il C sui
campi `Item.traits`/`archetype`, e l'effetto lo applica direttamente sulla `stats`
struct del ricalcolo o sui `Shot.traits`.

### 4.5 Tenere le sinergie BILANCIATE (rarity-aware)

Le sinergie devono restare dentro la stessa filosofia di equilibrio gia' scelta
(spec rarita', sez. 2):

- **La sinergia ha un budget, come un oggetto.** Il suo canale statistico passa
  per gli **stessi clamp per-campo** di `RecomputeStats`: anche una sinergia
  sbagliata (o un 7B che esagera) non puo' portare fuori fascia il giocatore. Il
  tetto **globale** su ogni statistica resta il guardiano finale.
- **Potenza scalata per rarita'.** La forza della sinergia scala con la rarita'
  **minima** della coppia (la sinergia di due comuni e' piccola; due leggendari
  danno il colpo grosso), riusando la tavola dei tetti per-oggetto gia' esistente
  (`SCRIPT_ITEMS_RARITY_ITEM_DELTA_FRACTION`). Cosi' non introduciamo un secondo
  sistema di bilanciamento: la sinergia e' "un oggetto in piu'" col suo budget.
- **Una sinergia per coppia, un effetto leggibile.** Come il vincolo "un effetto
  per oggetto": la coppia aggiunge **una** cosa comprensibile, non un fuoco
  d'artificio. La profondita' viene dal numero di coppie possibili, non dalla
  complessita' della singola.
- **Niente cumulo esplosivo.** Il canale comportamentale accende **bit** (OR
  idempotente): tenere due volte lo stesso trait non lo raddoppia. I contributi
  numerici passano dai clamp. Entrambe le vie sono a prova di runaway.

### 4.6 Tenere le sinergie SICURE (sandbox + ripiego)

- **Opzione 1 (C):** nessuna superficie sandbox. E' la piu' sicura per definizione.
- **Opzione 2/3 (Lua):** riusa **integralmente** il patto di sicurezza gia' in
  vigore (spec Lua sandbox, sez. 9). Uno `on_synergy` e' una callback come le
  altre: gira in sandbox senza accesso a OS/filesystem, vede solo handle validati,
  e qualunque `luaL_error` (handle non valido, tipo sbagliato, budget di
  istruzioni) **disabilita permanentemente quella sandbox** senza toccare il
  resto. Se lo script di sinergia muore, l'oggetto ripiega sui suoi effetti
  singoli, esattamente come oggi un oggetto ripiega sulla mini-VM quando il suo
  Lua si disabilita (`ScriptItemsHasActiveLua` torna falso dal frame dopo).
- **Ripiego "mai un buco".** Come per gli oggetti, se lo script di sinergia
  fallisce al caricamento, la sinergia semplicemente **non c'e'** (nessun crash,
  nessuna mezza-applicazione). Con l'Opzione 1 come base, c'e' sempre almeno il
  set di sinergie canoniche sicure sotto.
- **Validazione a inizio run (Opzione 3).** Gli script di sinergia generati
  dall'LLM passano per la stessa validazione/caricamento degli script-oggetto
  (fase 3a-L3): quelli che non caricano vengono scartati, gli altri diventano
  sandbox vive. Nessuna inferenza durante il gameplay.

---

## 5. Tre-quattro sinergie di esempio, nella NOSTRA API Lua

Effetti descritti come **pattern generici** (nessun nome/asset di marca). I nomi
degli oggetti sono segnaposto: nella pratica la condizione dovrebbe guardare
**archetipo/trait**, non il nome, quando gli oggetti sono generati.

### Esempio 1 — Inseguimento + Perforazione → "colpo che infila la fila"
Canale B (trait sul colpo). Se possiedi sia un oggetto-inseguimento sia uno-
perforazione, i tuoi colpi curvano *e* trapassano. Reso come OR di bit nel punto
di creazione del colpo; qui la versione Lua dentro `on_fire`:

```lua
-- oggetto "inseguimento", ma con consapevolezza della sinergia
function on_fire(x, y, dx, dy)
    local t = TRAIT_HOMING
    if player_has_item("Punteruolo")  -- l'oggetto-perforazione
    then
        t = t | TRAIT_PIERCE          -- la coppia aggiunge la perforazione
    end
    spawn_shot(x, y, dx, dy, t)
end
```

### Esempio 2 — Rimbalzo + Fuoco → "rimbalzo incendiario"
Canale B. Un oggetto-rimbalzo e uno-fuoco: i colpi rimbalzano lasciando danno nel
tempo. Espresso in `on_hit`, applicando il danno-nel-tempo come tick quando un
colpo con rimbalzo colpisce (usando gli handle validati):

```lua
function on_hit(shot_id, enemy_id)
    if player_has_item("Braciere tascabile") then   -- l'oggetto-fuoco
        -- piccola scottatura immediata + la coppia e' "leggibile": brucia a ogni tocco
        damage_enemy(enemy_id, player_damage() * 0.25)
        add_particle(enemy_x(enemy_id), enemy_y(enemy_id))
    end
end
```

### Esempio 3 — Rallentamento + Cadenza alta → statistica (canale A)
Canale A (cache). Un oggetto-rallentamento e uno-cadenza: molti colpi deboli che
tengono i nemici lenti; premiamo la combo con un filo di danno. Dentro
`on_synergy`/`on_evaluate`, modifica la `stats` table; il C poi clampa:

```lua
function on_synergy(stats)
    if player_has_item("Ghiaccio secco")      -- rallentamento
       and player_has_item("Molla a scatto")  -- cadenza
    then
        stats.damage    = stats.damage * 1.10  -- piccolo, poi clampato
        stats.fireDelay = stats.fireDelay * 0.95
    end
end
```

### Esempio 4 — Famiglio periodico + Rallentamento → "orbita gelida" (canale B, on_tick)
Canale B via `on_tick`: se hai il famiglio e il rallentamento, il famiglio ogni
tot secondi rallenta il nemico piu' vicino invece di limitarsi a sparare.

```lua
local acc = 0.0
function on_tick(dt)
    if not player_has_item("Alito polare") then return end  -- il rallentamento
    acc = acc + dt
    if acc < 0.8 then return end
    acc = 0.0
    local e = nearest_enemy()          -- handle validato dall'API
    if e >= 0 then
        set_enemy_velocity(e, 0.0, 0.0)  -- pin/rallenta; il C clampa e valida l'handle
        add_particle(enemy_x(e), enemy_y(e))
    end
end
```

In tutti e quattro: nessun puntatore, solo handle; ogni effetto passa per i clamp
di sicurezza; se lo script muore, l'oggetto torna al suo effetto singolo.

---

## 6. La prima versione minima da costruire (la raccomandazione)

Costruire il **meno possibile** che dia sinergie vere, sicure e testabili, e che
sia il fondamento per la versione LLM dopo:

1. **Persistere l'archetipo** come `Item.archetype` (enum sopra, `ARCH_NONE = 0`),
   con round-trip nel manifest e ripiego "neutro" — cosi' come gia' fatto per
   `kind`/`rarity`. E' l'unico dato mancante per decidere una sinergia in modo
   deterministico.
2. **Una tavola di sinergie in C** (Opzione 1), dichiarativa e ben marcata come le
   tabelle di rarita': poche righe `{archetipo/trait A, archetipo/trait B,
   effetto, canale}`, con 4-6 sinergie canoniche (quelle della sez. 5). Aggiungere
   una sinergia = aggiungere una riga.
3. **Due punti di applicazione, riusando cio' che c'e':**
   - canale A dentro `ScriptItemsRecomputeStats`, **dopo** i contributi degli
     oggetti e **prima** dei clamp;
   - canale B subito dopo la creazione del colpo del giocatore in `combat.c`
     (OR dei trait), e/o via le callback esistenti.
4. **Potenza scalata per rarita' minima della coppia**, riusando la tavola dei
   tetti gia' esistente; tutto passa dai clamp per-campo.
5. **Test**: presenza/assenza della coppia accende/spegne l'effetto; l'effetto
   statistico e' idempotente sotto ricalcoli ripetuti; una coppia di leggendari
   spinge piu' di una di comuni ma resta in fascia; togliere un oggetto della
   coppia spegne pulito la sinergia.

**Fuori da questa prima versione** (cicli successivi, ora che il punto di
applicazione esiste):
- la callback `on_synergy` in sandbox e gli **script di sinergia generati
  dall'LLM a inizio run** (Opzione 3), che si agganciano agli **stessi** due
  canali;
- la **fusione visiva a strati** delle coppie (sprite sovrapposti; ibridi
  pre-generati per i conflitti di slot, APPUNTI sez. 6);
- gli helper API `player_has_archetype(ARCH_*)` / `has_trait(TRAIT_*)` per far
  condizionare gli script sui metadati invece che sui nomi.

Motivo della scala: la tavola C da' subito valore giocabile con rischio zero ed e'
il banco di prova dei **due canali di applicazione**. Quando quei canali sono
solidi e testati, aggiungere l'LLM sopra e' cablaggio, non un motore nuovo — la
stessa strategia che ha funzionato per gli oggetti (prima mini-VM, poi Lua, poi
LLM che scrive il Lua).

---

## 7. Confini di IP (cosa possiamo e non possiamo riusare)

- **Possiamo** reimplementare liberamente i **meccanismi**: il ricalcolo-da-zero
  delle statistiche a ogni cambiamento (l'idea del `MC_EVALUATE_CACHE`), i **flag
  bitmask indipendenti sui proiettili** che si combinano per OR, l'ordine di
  applicazione dei modificatori, la gerarchia di priorita' per i modificatori
  "forti". Sono **fatti di design e algoritmi**, non protetti, e li citiamo con le
  fonti (sez. 2).
- **Possiamo** reimplementare i **tipi** di sinergia come pattern generici:
  "inseguimento+perforazione", "rimbalzo+fuoco", "esplosione+divisione",
  "rallentamento+cadenza" descritti come comportamenti, con nomi **nostri**.
- **NON possiamo** copiare: i **nomi** di oggetti/personaggi/nemici di Isaac, i
  testi di flavour, la prosa delle wiki, gli **sprite/asset**, o tabelle di ID
  numerici prese di peso. Tutti i nomi in questo documento
  ("Punteruolo", "Braciere tascabile", "Ghiaccio secco", "Alito polare", …) sono
  segnaposto **originali/generici**, da sostituire coi nomi che l'LLM genera per la
  run. Descriviamo il **pattern**, mai l'asset di marca.

Fonti citate:
- Isaac Blueprints — *Stats Cache*: https://isaacblueprints.com/tutorials/basics/stats_cache/
- BoI Lua API Docs — *ModCallbacks* (`MC_EVALUATE_CACHE`): https://wofsauge.github.io/IsaacDocs/rep/enums/ModCallbacks.html
- Platinum God — *Item Synergies*: https://tboi.com/synergies
- The Binding of Isaac: Rebirth Wiki — *Synergies*: https://thebindingofisaacrebirth.wiki/items/synergies/
