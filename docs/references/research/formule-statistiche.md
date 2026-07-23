---
id: ref-formule-statistiche
title: Le formule delle statistiche (cadenza, danno, velocita, gittata, fortuna)
domain: references
status: approved
authority: supporting
owner: design
summary: >-
  Formule esatte di Isaac (cadenza a radice, danno a radice, shot speed, gittata, fortuna) mappate sui campi Player/clamp attuali, con raccomandazioni adottare-vs-adattare; piu' raccomandazioni sono gia' state implementate (floor fireDelay, luck).
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [formule, bilanciamento, cadenza-di-tiro, danno, luck, clamp, isaac-reference]
related: []
supersedes: []
source_files: []
---

# Le formule delle statistiche (cadenza, danno, velocita, gittata, fortuna)

Documento di riferimento per bilanciare gli oggetti generati di **Melting Run**
contro una curva collaudata. Le formule qui sotto sono i **fatti numerici**
(algoritmi e costanti) del sistema di statistiche di *The Binding of Isaac:
Rebirth/Repentance*, riscritti con la nostra terminologia e mappati sulle
`struct Player` / `ScriptItemsStatsAccum` di `src/core/game_types.h` e
`src/script/script_items.c`.

> Nota di lettura. Isaac ragiona per **frame** (30 fps interni) e per **tile**.
> Melting Run ragiona per **secondi** e per **pixel** (`SCREEN_WIDTH 960`,
> `ROOM_W 876`). Ogni formula viene quindi data prima nella sua forma
> originale (il fatto), poi convertita nelle nostre unita'. Le costanti dei
> nostri clamp (`SCRIPT_ITEMS_*_MIN/MAX`) sono citate dal codice attuale.

---

## 1. Cadenza di tiro (fire rate) — la piu' importante

### 1.1 Il fatto: dalla statistica "Tears" al tear delay

Isaac non conserva direttamente "colpi al secondo": conserva un **tear delay**
(ritardo, in frame) e mostra al giocatore una statistica derivata **Tears**.
La conversione dalla statistica Tears `T` al ritardo, nel ramo normale
(`0 <= T <= Tmax`), e':

```
tear_delay = 16 - 6 * sqrt(1.3 * T + 1)
```

Costanti esatte: `16`, `6`, `1.3`, `1`. La radice quadrata e' il cuore del
comportamento: rende ogni "Tears Up" successivo **meno efficace** del
precedente (rendimenti decrescenti), impedendo che sommare tanti oggetti porti
a una cadenza infinita.

Forma a tratti completa (Repentance+):

```
tear_delay = 5                              se T >  Tmax
tear_delay = 16 - 6 * sqrt(1.3 * T + 1)     se 0 <= T <= Tmax
             (rami speciali per T < 0)       se il valore va sotto zero
Tmax ~= 1.816239316   (cap pratico della statistica Tears)
```

Al cap `Tmax` il ritardo tocca il suo minimo di circa `5` frame; oltre, viene
bloccato. Esistono oggetti che spingono il ritardo sotto zero (fino a un tetto
di fire rate interno di `120`, corrispondente a un tear delay di `-0.75`), ma
sono l'eccezione, non la curva di base.

### 1.2 Il fatto: dal tear delay ai colpi al secondo

```
fire_rate (colpi/sec) = 30 / (tear_delay + 1)
```

Interpretazione: il tear delay e' il **ritardo aggiuntivo oltre 1/30 di
secondo**, misurato in unita' da 1/30 s. Esempio dal wiki: `tear_delay = 7`
significa `8/30` s tra un colpo e l'altro, cioe' `30/8 = 3.75` colpi/sec.

Secondi tra due colpi (ci serve per la conversione):

```
secondi_tra_colpi = (tear_delay + 1) / 30
```

### 1.3 Mappatura su Melting Run

Da noi la cadenza NON e' un tear delay in frame: e' `Player.fireDelay`, un
**cooldown in secondi** (vedi `fireTimer` in `combat.c`), con partenza
`baseFireDelay = 0.23` e clamp `[SCRIPT_ITEMS_FIRE_DELAY_MIN 0.05,
SCRIPT_ITEMS_FIRE_DELAY_MAX 2.0]`. Il nostro `fireDelay` e' quindi
l'equivalente diretto di `secondi_tra_colpi` di Isaac. Ponte tra i due mondi:

```
fireDelay (s)      = (tear_delay + 1) / 30
tear_delay_equiv   = fireDelay * 30 - 1
T_equivalente      solve: 16 - 6*sqrt(1.3*T + 1) = tear_delay_equiv
```

Dove cade il nostro default `baseFireDelay = 0.23`:

- `tear_delay_equiv = 0.23 * 30 - 1 = 5.9` frame
- risolvendo la 1.1: `T_equivalente ~= 1.42` Tears
- cadenza: `1 / 0.23 ~= 4.35` colpi/sec

Quindi partiamo gia' vicini al **cap** della curva Isaac (Tears ~1.42 su un
Tmax ~1.82; cadenza 4.35 contro le ~5 tps del cap "solo statistica Tears").
Implicazioni pratiche per il bilanciamento:

- **Il nostro floor di clamp `0.05` s = 20 colpi/sec e' molto oltre Isaac.**
  Isaac considera ~5 tps il tetto raggiungibile con la sola statistica Tears
  (tear delay minimo ~5), e riserva i valori piu' alti a pochi oggetti
  eccezionali. Se vogliamo restare su una curva provata, il floor pratico per
  gli oggetti generati dovrebbe essere ~`0.10` s (10 tps), lasciando `0.05`
  solo come rete di sicurezza assoluta (che gia' e', vedi il commento in
  `script_items.c`). **Da adattare, non adottare tale e quale.**
- **Rendimenti decrescenti.** Oggi i nostri stat-up muovono `fireDelay` in
  modo lineare/moltiplicativo: `TRAIT_RAPID` fa `fireDelay *= 0.92`
  (`ScriptItemsApplyBuiltin`) e il fallback fa `fireDelay -= 0.03*scale`
  (`ScriptItemsApplyStatUpFallback`). Il `*= 0.92` moltiplicativo e' gia'
  auto-limitante (ogni oggetto toglie il 8% del *residuo*, mai lo stesso
  assoluto), quindi imita bene lo spirito della radice di Isaac. Il
  `-= 0.03` additivo invece **no**: N oggetti tolgono `N*0.03` lineare. Per
  gli oggetti stat-up di boss conviene esprimere il buff di cadenza in
  `on_evaluate` come frazione del residuo (`fire_delay = fire_delay * k`),
  non come sottrazione fissa, cosi' da riprodurre i rendimenti decrescenti.
- Il nostro tetto per-oggetto scalato per rarita' (`0.15/0.25/0.40/0.60` di
  `baseFireDelay`, cioe' al massimo `-0.60*0.23 = -0.138` s per un leggendario)
  limita gia' bene il singolo oggetto; e' l'accumulo di molti oggetti additivi
  il rischio, non il singolo.

---

## 2. Danno

### 2.1 Il fatto: la formula con radice

```
EffectiveDamage = ( CharBaseDmg * sqrt(1.2 * TotalDmgUps + 1) + FlatDmgUps ) * Multipliers
```

Dove:

- `CharBaseDmg` = danno base del personaggio (costante di riferimento **3.50**)
  moltiplicato per l'eventuale moltiplicatore del personaggio.
- `TotalDmgUps` = somma dei "danno +X" normali. Entra **sotto radice**
  (`sqrt(1.2 * TotalDmgUps + 1)`): stessa logica della cadenza, ogni "danno +"
  successivo rende meno del precedente.
- `FlatDmgUps` = i pochi bonus danno esclusi dalla radice (additivi puri, si
  sommano dopo).
- `Multipliers` = prodotto dei moltiplicatori di danno; la maggior parte si
  moltiplica tra loro, ma alcuni `1.5x` non si sommano tra loro e si fermano
  a `1.5x` (cap di categoria).

Costanti esatte: base `3.5`, coefficiente `1.2` dentro la radice, `+1` dentro
la radice.

### 2.2 Mappatura su Melting Run

Da noi il danno e' `Player.damage`, `baseDamage = 8`, clamp
`[SCRIPT_ITEMS_DAMAGE_MIN 0.5, SCRIPT_ITEMS_DAMAGE_MAX 200]`. Oggi lo muoviamo
in modo **puramente additivo** (`ScriptItemsApplyBuiltin`: `TRAIT_GIANT
damage += 1.6`, `TRAIT_PIERCE damage += 0.8`, `SLOT_HAND damage += 1.0`;
fallback `damage += 1.5*scale` ecc.). Non c'e' radice: N oggetti danno
`N * incremento`, crescita lineare.

Cosa adottare vs adattare:

- **Adottare la forma a radice per la scalata di lungo periodo.** Se un giorno
  contiamo "quanti stat-up di danno ha preso il giocatore" e vogliamo evitare
  la fuga esponenziale su 5 piani, la formula giusta e':

  ```
  damage = baseDamage * sqrt(1.2 * n_dmg_ups + 1)
  ```

  Implementabile in `on_evaluate` di un oggetto stat-up di danno, oppure in C
  se introduciamo un contatore. Con `baseDamage = 8`: `n=1 -> 8*1.48 = 11.9`;
  `n=4 -> 8*2.41 = 19.3`; `n=9 -> 8*3.44 = 27.5`. Curva morbida, "sempre
  meglio ma mai fuga".
- **Il nostro modello attuale (additivo + tetto per-oggetto per rarita') e'
  gia' contenuto** dal fatto che c'e' al massimo un `bossItem` stat-up per
  piano (5 in tutta la run). Il commento in `script_items.c` calcola: cinque
  leggendari consecutivi su `base=8` con tetto `0.60` arrivano a
  `8 + 5*4.8 = 32`, ben dentro `[0.5, 200]`. Quindi additivo va bene finche'
  restiamo a "un oggetto stat-up per piano". Se in futuro moltiplicheremo le
  fonti di stat-up, passare alla radice.
- **Attenzione a mescolare additivo e moltiplicativo.** In Isaac gli additivi
  vanno sotto radice e i moltiplicatori fuori; da noi tenere una regola sola
  per categoria evita sorprese. I nostri trait comportamentali (`TRAIT_GIANT`
  etc.) restano additivi; se introduciamo moltiplicatori, applicarli **dopo**
  gli additivi, come `Multipliers` in 2.1.

---

## 3. Velocita' dei colpi (shot speed)

### 3.1 Il fatto

- Valore base: **1.0**. Minimo: **0.6** (gli oggetti che la abbassano non
  possono scendere sotto). Nessun massimo documentato.
- A shot speed `1.0` un colpo attraversa **un tile in 8 frame** (`0.133` s),
  cioe' **7.5 tile/sec**; il resto scala **linearmente**.
- Pre-Repentance la shot speed influenzava la gittata effettiva (meno shot
  speed = meno gittata, perche' la gittata era *tempo* in aria, vedi sez. 4);
  in Repentance questo legame e' stato rimosso.

### 3.2 Mappatura su Melting Run

Da noi e' `Player.shotSpeed` in **pixel/sec**, `baseShotSpeed = 520`, clamp
`[SCRIPT_ITEMS_SHOT_SPEED_MIN 60, SCRIPT_ITEMS_SHOT_SPEED_MAX 1400]`. E' gia'
lineare come Isaac (un semplice modulo di velocita' del proiettile). Note:

- Non abbiamo tile; se ci servisse un'intuizione "quanti attraversamenti di
  stanza al secondo", `ROOM_W = 876` px, quindi `520 px/s` attraversa la
  stanza in `~1.68` s. E' un valore comodo e leggibile; non serve toccarlo.
- **Adottare l'idea del floor, non il numero.** Il nostro floor `60` px/s e'
  molto lento (colpi quasi fermi); va bene come rete di sicurezza. Isaac usa
  `0.6` del base come floor pratico: l'equivalente da noi sarebbe `~312` px/s.
  Per gli oggetti generati conviene non scendere sotto `~0.6*base = 312`, cosi'
  i colpi restano sempre "usabili".
- I built-in oggi spingono la shot speed con `SLOT_EYES shotSpeed += 25` e i
  fallback `TRAIT_HOMING/BOUNCE shotSpeed += 60*scale`: additivi, coerenti con
  la linearita' di Isaac. Nessun cambiamento necessario.

---

## 4. Gittata / altezza del colpo (range / tear height)

### 4.1 Il fatto

Isaac **non pubblica una formula chiusa** con costanti per la gittata. Quel che
e' documentato:

- Pre-Repentance: "Range" determina il **tempo** che un colpo resta in aria,
  non la distanza. La distanza effettiva dipende da tempo in aria + shot speed
  + velocita' di caduta del colpo. Internamente la statistica si chiama **Tear
  Height** e regola anche l'altezza da cui il colpo parte.
- Repentance+: "Range" e' stato reinterpretato come **numero di tile** che il
  colpo percorre.
- Limiti: la gittata non puo' scendere sotto `5.0` (pre-Repentance) o `1.0`
  (Repentance+); **nessun limite superiore**.

Fatto operativo: senza costanti pubbliche, la gittata di Isaac non e' una
formula da copiare, e' un **modello** ("un colpo vive un tot e poi cade").

### 4.2 Mappatura su Melting Run

Noi **non abbiamo una statistica di gittata dedicata**. Il ruolo e' svolto da
`Shot.life` (tempo di vita del proiettile, in `combat.c`) combinato con
`shotSpeed`. Se e quando vorremo una statistica "gittata":

- Il modello piu' semplice e fedele e' **tempo di vita**: `Shot.life`
  proporzionale a una futura `Player.range`, esattamente come il "tempo in
  aria" pre-Repentance. Distanza percorsa `= shotSpeed * life`.
- Adottare **il floor, non lo zero**: come Isaac impedisce gittata sotto `1.0`,
  noi dovremmo impedire `life` cosi' bassa che il colpo muoia prima di uscire
  dal giocatore.
- Tenere `shotRadius` (`Player.shotRadius`, base `5`, clamp `[2, 40]`)
  **separato** dalla gittata: e' la dimensione/collisione del colpo (l'analogo
  del "tear size"/scala di Isaac), non la distanza. Oggi i built-in la muovono
  con `TRAIT_GIANT shotRadius += 0.8` e i fallback `+= 1.0*scale`: coerente.

---

## 5. Fortuna (luck)

### 5.1 Il fatto

- La fortuna non modifica danno o cadenza: **regola le probabilita'** degli
  effetti a chance (esplosioni, veleno, tiri all'indietro, drop, ecc.).
- Pattern tipico: la probabilita' sale con la fortuna fino a **garantire**
  l'effetto a una certa soglia. Il wiki lo esprime per esempi:
  - esplosione del colpo: 100% a **13** di fortuna;
  - colpi avvelenati: 100% a **12**;
  - rallentamento: 100% a **18**;
  - tiri all'indietro: 100% a **5** (versione Repentance).
- La maggior parte degli effetti sui colpi scala **esponenzialmente** con la
  fortuna (i primi punti si sentono poco), alcuni **linearmente**.
- Per certi drop la fortuna e' **limitata tra 0 e 10**. La fortuna base della
  maggior parte dei personaggi e' `0` e puo' essere negativa o frazionaria.

Forma lineare generica (quella facile da riprodurre e da bilanciare):

```
chance = min( base + luck * incremento, 1.0 )
```

Se vogliamo "garantito a `L` di fortuna" partendo da `base`:

```
incremento = (1 - base) / L
```

Esempio: effetto al 5% base, garantito a `L = 10`: `incremento = 0.95/10 =
0.095` per punto di fortuna.

### 5.2 Mappatura su Melting Run

Oggi **non abbiamo una statistica luck** nel `Player`, e i nostri trait
comportamentali (`TRAIT_HOMING`, `TRAIT_EXPLODE`, `TRAIT_SPLIT`, `TRAIT_VAMP`,
`TRAIT_BOUNCE`) sono **binari** (o l'oggetto ce l'ha o no), non probabilistici.
La fortuna sarebbe il modo naturale per rendere questi trait *graduali*:

- Aggiungere un campo `float luck` (e `baseLuck`) al `Player`, dentro il
  sistema di cache recompute-from-zero come le altre sei statistiche (nuovo
  campo in `ScriptItemsStatsAccum`, nuovo clamp, esposto a `on_evaluate` come
  `stats.luck`).
- Usarlo negli hook Lua tramite l'API a handle (`rng` di `script_api`): un
  `on_hit(shot_id, enemy_id)` che innesca l'esplosione **solo** se
  `rng() < min(base + luck*incr, 1)`. Cosi' un oggetto "colpi esplosivi" a
  bassa fortuna esplode di rado, e diventa garantito accumulando fortuna —
  esattamente la curva di Isaac.
- **Adottare il pattern lineare `min(base + luck*incr, 1)`** (semplice,
  testabile, deterministico con la nostra RNG seminata) piuttosto che la
  scala esponenziale di Isaac, a meno che non vogliamo esplicitamente
  nascondere i primi punti di fortuna.
- **Adottare l'idea del cap sulla fortuna** (Isaac la limita 0..10 per certi
  drop): un clamp `[SCRIPT_ITEMS_LUCK_MIN, SCRIPT_ITEMS_LUCK_MAX]` p.es.
  `[-5, 15]` eviterebbe proc garantiti troppo presto e fortune negative
  patologiche.

---

## 6. Tabella riassuntiva: adottare vs adattare

| Statistica | Formula-fatto di Isaac | Campo Melting Run | Adottare / Adattare |
|---|---|---|---|
| Cadenza | `tear_delay = 16 - 6*sqrt(1.3*T+1)`; `fire_rate = 30/(tear_delay+1)`; cap `Tmax~1.82`, delay min ~5 | `fireDelay` (s), base `0.23`, clamp `[0.05, 2.0]` | Adattare: convertire in secondi `fireDelay=(td+1)/30`; alzare il floor pratico a ~0.10 s; usare buff moltiplicativi (residuo) per i rendimenti decrescenti |
| Danno | `Dmg = (Base*sqrt(1.2*ups+1) + flat) * mult`, base `3.5` | `damage`, base `8`, clamp `[0.5, 200]` | Adottare la **radice** per la scalata su piu' oggetti; l'additivo attuale va bene finche' resta 1 stat-up/piano |
| Shot speed | base `1.0`, floor `0.6`, `1.0 = 7.5 tile/s`, lineare | `shotSpeed` (px/s), base `520`, clamp `[60, 1400]` | Adottare il concetto di floor (~0.6*base = 312 px/s); modello lineare gia' corretto |
| Gittata | nessuna formula pubblica; "tempo in aria" o "tile"; floor `1.0`, nessun cap | (assente) — svolta da `Shot.life`+`shotSpeed`; `shotRadius` e' la *taglia*, non la gittata | Adattare: se serve, statistica = tempo di vita del colpo con un floor; tenere `shotRadius` separato |
| Fortuna | `chance = min(base + luck*incr, 1)`; garantito a soglia; cap 0..10 | (assente) | Adottare il pattern lineare + cap; introdurre `luck` per rendere graduali i trait binari via `rng()` negli hook Lua |

---

## 7. Confini di IP

Quanto sopra e' deliberatamente **solo fatti**: formule, costanti e algoritmi.
Formule e passi algoritmici **sono fatti** e li possiamo reimplementare
liberamente (`16 - 6*sqrt(1.3*T+1)`, `30/(delay+1)`, la radice del danno, i
cap numerici, il pattern lineare della fortuna): sono citati sotto con la
fonte.

Restano **fuori** da cio' che possiamo copiare, e in questo documento non
compaiono di proposito:

- **nomi** protetti di oggetti, personaggi o nemici di Isaac (qui abbiamo usato
  solo descrizioni generiche: "esplosione del colpo", "tiri all'indietro",
  ecc., mai il nome commerciale dell'oggetto che produce l'effetto);
- **testi di flavour**, prosa del wiki, descrizioni redazionali;
- **sprite**, asset grafici, suoni;
- **tabelle di ID numerici** copiate in blocco (le nostre statistiche e i
  nostri clamp sono valori originali, tarati sui default di Melting Run
  `damage 8 / fireDelay 0.23 / shotSpeed 520 / shotRadius 5 / speed 224`).

In breve: le **curve** (cadenza a radice, danno a radice, fortuna lineare) le
adottiamo come matematica; i **nomi e gli asset** che le vestono in Isaac no,
li sostituiamo con roba nostra e generica.

---

## Fonti

- The Binding of Isaac: Rebirth Wiki — *Tears*:
  https://bindingofisaacrebirth.wiki.gg/wiki/Tears
- The Binding of Isaac Wiki — *Stats/Tears* (formula tear delay a radice):
  https://bindingofisaac.fandom.com/wiki/Stats/Tears
- The Binding of Isaac: Rebirth Wiki — *Damage*:
  https://bindingofisaacrebirth.wiki.gg/wiki/Damage
- The Binding of Isaac: Rebirth Wiki — *Shot Speed*:
  https://bindingofisaacrebirth.wiki.gg/wiki/Shot_Speed
- The Binding of Isaac: Rebirth Wiki — *Range*:
  https://bindingofisaacrebirth.wiki.gg/wiki/Range
- The Binding of Isaac: Rebirth Wiki — *Luck*:
  https://bindingofisaacrebirth.wiki.gg/wiki/Luck
</content>
</invoke>
