---
id: eng-spec-step-c-shottype
title: Step C: tipi di colpo inventati e bilanciati
domain: engineering
status: implemented
authority: canonical
owner: engineering
summary: >-
  Tipi di colpo senza enum fisso: ShotForm+manopole inventati dal modello, ShotTypeBalance riporta ogni tipo a potenza ~1.0 (sidegrade mai dud), curve alla Isaac e fortuna.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [spec, implementazione, fase-storica]
related: []
supersedes: []
source_files: []
---

# Step C — Bilanciamento alla Isaac + tipi di colpo generati dall'AI

Riferimenti: `docs/engineering/specs/2026-07-14-feedback-roadmap.md` (punto 3),
`docs/references/formule-statistiche.md`, `docs/references/design-sinergie.md`.

Feedback che ha rifatto questa spec (2026-07-14, durante la run notturna):

> «I tipi di colpi nuovi devono sempre essere creati dai modelli AI — i tre che hai
> creato (chiodi, laser, elettricità) sono solo esempi.»

Quindi: **il motore C non ha un menu fisso di tipi di colpo**. Il C espone un
sistema *parametrico* (una forma di resa + manopole di comportamento, tutte
clampate e auto-bilanciate); il modello inventa nome, forma e numeri di ogni tipo
nel JSON della run. "Chiodi/laser/scarica" restano SOLO due cose: un esempio nel
prompt, e il ripiego procedurale in C quando il modello non c'è (stessa filosofia
di temi/oggetti/Lua: il C dà i mattoni, l'AI compone).

## C1 — Curve e confini alla Isaac (statistiche)

In `src/script/script_items.c` (il sistema delle cache) e `src/game/game.c`:

1. **Pavimento pratico della cadenza**: `SCRIPT_ITEMS_FIRE_DELAY_MIN` 0.05 → **0.10**
   (0.05 s = 20 colpi/s, fuori scala per qualunque nemico) e
   `SCRIPT_ITEMS_FIRE_DELAY_MAX` 2.0 → **1.2** (oltre, l'oggetto è un *dud*: un
   colpo ogni due secondi non è un compromesso, è una run rovinata).
2. **Pavimento pratico della velocità dei colpi**: `SCRIPT_ITEMS_SHOT_SPEED_MIN`
   60 → **260** (= 0.5 × base 520): un colpo che striscia non attraversa la stanza.
3. **Rendimenti decrescenti sul danno** (Isaac, `formule-statistiche.md`): il danno
   resta lineare fino a 2 × base, poi viene compresso con una radice:
   `d > 2b  ->  d' = 2b * sqrt(d / 2b)`. Applicato UNA volta, in fondo al
   ricalcolo, quindi resta idempotente (il ricalcolo riparte sempre da zero).
   Effetto: 5 leggendari impilati (32) → 22.6; due oggetti da +2/+3 (13) restano
   intatti. Nessun oggetto singolo diventa inutile, ma impilare non raddoppia più.
4. **Nuova statistica `luck`** (base 0, banda [-5, +15]): pilota la probabilità del
   trait VAMP (`18 + 3*luck` %, clampata 0..60). Leggibile/scrivibile da
   `on_evaluate` come `stats.luck`.

## C2 — Tipi di colpo parametrici, inventati dal modello

### Il vocabolario che il C mette a disposizione

`src/core/shot_type.h` (nuovo, **senza raylib**: lo includono sia il gioco sia
melting-gen, così la definizione è UNA sola e non può divergere):

```c
typedef enum ShotForm {          /* la RESA: come appare il colpo */
    SHOT_FORM_ORB = 0,           /* zero-default: la palla di sempre */
    SHOT_FORM_SPIKE,             /* proiettile allungato (chiodo, dardo, scheggia) */
    SHOT_FORM_BEAM,              /* raggio sottile e lungo */
    SHOT_FORM_ARC,               /* arco a zig-zag (scarica, fulmine) */
    SHOT_FORM_BLADE,             /* lama che ruota */
    SHOT_FORM_COUNT
} ShotForm;

typedef struct ShotTypeDef {
    bool active;                 /* false = nessun tipo: il colpo base di sempre */
    char name[32];               /* inventato dal modello, mostrato nella GUI */
    ShotForm form;
    float speedMul, damageMul, radiusMul, lifeMul;   /* manopole continue */
    int pierceBonus, chain, pellets;                 /* manopole discrete */
} ShotTypeDef;
```

Bande (`ShotTypeClamp`): speed 0.5–2.0, damage 0.3–2.0, radius 0.4–2.5,
life 0.5–2.0, pierce 0–3, chain 0–3, pellets 1–3.

### Bilanciamento automatico: nessun tipo può essere né un dud né rotto

`ShotTypePower()` stima il budget di potenza di un tipo:

```
power = damageMul * pellets^0.85 * (1 + 0.55*pierce) * (1 + 0.45*chain)
        * (0.75 + 0.25*speedMul) * (0.80 + 0.20*radiusMul) * (0.85 + 0.15*lifeMul)
```

`ShotTypeBalance()` clampa, poi **risolve `damageMul` perché `power` valga 1.0**;
se il risultato uscirebbe dalla banda di damageMul, taglia le manopole discrete
(chain, poi pierce, poi pellets) finché il potere rientra. Garanzia verificabile:
qualunque cosa inventi il modello, `power ∈ [0.75, 1.25]`. I tipi diventano
*sidegrade* alla Isaac (chiodi veloci e deboli ≈ laser che perfora ≈ scarica che
salta fra nemici), mai un upgrade secco.

Gira in due punti indipendenti: in melting-gen (il manifest è già bilanciato e
ispezionabile) e in `run_content.c` al caricamento (difesa in profondità contro un
manifest scritto a mano).

### Chi porta un tipo di colpo

Un tipo per piano, attaccato a **uno dei tre oggetti attivi** (quale lo decide il
modello: campo `shotItem` 1..3 nel JSON). Gli oggetti stat-up del boss non ne hanno
mai (sono solo numeri). Il tipo di colpo del giocatore è **ricalcolato da zero**
insieme alle statistiche (`ScriptItemsRecomputeStats`): vince l'ULTIMO oggetto
raccolto che ne porta uno (alla Isaac: l'ultima *tear replacement* vince), e
toglierlo fa tornare il precedente senza deriva.

### Comportamento (non solo estetica)

`CombatFirePlayer`/`CombatUpdateShots`: `pellets` (ventaglio), `pierceBonus`
(attraversa), `chain` (all'impatto salta al nemico più vicino entro 220 px con
danno 0.65×, `chain-1`), moltiplicatori su velocità/danno/raggio/vita. La forma
guida il disegno (`game_renderer.c`): orb = due cerchi (oggi), spike = rettangolo
allungato orientato dalla velocità, beam = scia lunga e sottile, arc = spezzata a
zig-zag, blade = quadrato che ruota.

### Lato generatore

* `run.gbnf`: il piano acquista `"shot": {name, form, speed, damage, size, life,
  pierce, chain, pellets}` e `"shotItem": 1|2|3` (ordine chiavi fisso, come sempre).
* `prompts/system.txt` + `user.txt`: descrizione del campo + **chiodi/laser/scarica
  come ESEMPI**, con la frase esplicita "inventane di tuoi, non copiare questi".
* `gen_fallback.c`: tipo di colpo procedurale dal seed (mai un piano senza).
* `gen_validate.c`: normalizza + `ShotTypeBalance`.
* `gen_manifest.c`: righe `floorN.itemM.shot*=` sull'oggetto che lo porta.
* `run_content.c`: le rilegge con lo schema per-chiave (riga assente = nessun tipo
  → back-compat totale con i manifest vecchi).

## Criteri di successo (test)

1. `ShotTypeBalance` su una griglia di combinazioni estreme: `power ∈ [0.75, 1.25]`
   sempre (dud e rotto entrambi corretti).
2. Round-trip testo↔enum delle forme (blocca la sincronia gen↔gioco, come per rarity).
3. L'ultimo oggetto con tipo di colpo vince; rimuoverlo ripristina il precedente.
4. `chain` produce davvero un colpo verso un secondo nemico all'impatto.
5. `pierceBonus` fa sopravvivere il colpo al primo nemico.
6. Curva del danno: +2/+3 invariati, 5 leggendari compressi ma monotoni.
7. `make test-gen`: un tipo di colpo per piano nel manifest, la grammatica accetta
   il JSON del writer, golden rigenerato deliberatamente.
