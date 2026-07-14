# Fase 3b — Nemici e boss inventati dall'AI

Data: 2026-07-14 (dopo C/D/E/B2 e la review)
Stato: in esecuzione

Stesso principio dello step C, che il proprietario ha imposto e che ormai è la regola della
casa: **il motore non ha un catalogo di nemici**. Espone un vocabolario parametrico (forme,
movimenti, modi di sparare, manopole clampate) e **il modello inventa i nemici a ogni run**.
I quattro nemici di oggi (chaser/shooter/tank/boss) restano solo come ripiego procedurale.

## Cosa il C mette a disposizione

`src/core/enemy_type.h` (senza raylib, condiviso con melting-gen come `shot_type.h`):

```c
typedef enum EnemyForm  { BLOB=0, SPIKY, ARMORED, FLOATER }        /* come APPARE */
typedef enum EnemyMove  { CHASE=0, KITE, ORBIT, ZIGZAG, CHARGE }   /* come SI MUOVE */
typedef enum EnemyFire  { NONE=0, SINGLE, SPREAD, RING }           /* come SPARA */

typedef struct EnemyTypeDef {
    bool active;  char name[32];
    EnemyForm form;  EnemyMove move;  EnemyFire fire;
    float hpMul, speedMul, sizeMul;   /* moltiplicatori delle basi del C */
    float fireRate;                   /* colpi al secondo (0 = solo contatto) */
    int pellets;                      /* per SPREAD/RING */
    bool boss;
} EnemyTypeDef;
```

Basi del motore (scalate per piano, come oggi): hp 24, velocità 80 px/s, raggio 17.
Zero-default ovunque: `{0}` = blob che insegue e non spara, cioè il nemico più innocuo.

## La garanzia: nessun nemico può rompere la run

Due reti, entrambe in C, entrambe testabili:

1. **`EnemyTypeBalance()`** — clampa le manopole e riporta la potenza del singolo nemico in
   una banda (come `ShotTypeBalance`): un nemico non può essere né un sacco da boxe né un
   muro che spara a raffica. Risolve `hpMul` per centrare il bersaglio, e se non basta taglia
   le manopole offensive.
2. **Budget di difficoltà della stanza** — la stanza non spawna «N nemici», spende un
   **budget di punti** (`3 + piano`), e ogni nemico costa la propria potenza. Un piano con
   nemici cattivi ne spawna quindi **meno**. È la rete che rende irrilevante quanto il modello
   sia stato generoso: la difficoltà della stanza è decisa dal C, non da lui.

## Cosa scrive il modello

Nel JSON, per ogni piano: **2 tipi di nemico + 1 tipo di boss**. Costo misurato: ~90 token per
piano, ~450 per run — sta dentro il `nPredict` di 2560 già allargato (il JSON di oggi ne usa
~1850). Non serve toccare di nuovo `n_ctx`.

```json
"enemies": [ {"name":"...","form":"spiky","move":"kite","fire":"spread","hp":1.2,"speed":0.9,"size":1.0,"rate":0.8,"pellets":3}, {...} ],
"bossType": { ... stessi campi ... }
```

Il nome del boss resta la chiave `boss` che c'è già (è il nome mostrato a schermo); `bossType`
è il suo **comportamento**.

## Criteri di successo

1. Qualunque combinazione di manopole → potenza in banda (griglia esaustiva, come il test R).
2. Il budget della stanza regge: con nemici massimi, la stanza ne spawna pochi; con nemici
   minimi, di più — e in entrambi i casi la somma dei costi resta sotto il budget.
3. Round-trip testo↔enum di forme/movimenti/spari (blocca la sincronia gen↔gioco).
4. I cinque movimenti e i quattro modi di sparare fanno cose **osservabilmente diverse**.
5. Manifest vecchio senza righe `enemy*` → i quattro nemici di sempre (back-compat).
6. `make test-llm`: nomi di nemici distinti per piano, come per i tipi di colpo.
