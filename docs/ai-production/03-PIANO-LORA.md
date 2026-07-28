---
id: aiprod-piano-lora
title: Piano LoRA
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Gerarchia delle LoRA da addestrare (style, enemies, items, environments, vfx, identità), caption, configurazione baseline e criteri per un eventuale checkpoint completo. Struttura del dataset definitivo per famiglie in dataset/worldsmelt-style/, alimentato dagli sprite originali Aseprite che sono insieme asset di gioco e dataset LoRA (DEC-175).
last_reviewed: 2026-07-28
topics: [lora, training, caption, stable-diffusion, dataset-split, dataset-worldsmelt-style, aseprite, DEC-175]
related: []
supersedes: []
source_files: []
---
# Piano LoRA

## Principio

La LoRA modifica un numero ridotto di parametri e produce un adattatore piccolo. Il
checkpoint completo viene rimandato finché non esiste una ragione misurabile.

## Base e luogo del training (DEC-148, DEC-168)

Le LoRA si addestrano su **SD1.5 vanilla**, non sul checkpoint `pixel-baseline` di terze
parti: provenienza pulita e risultato portabile. A validazione avvenuta, la Style LoRA
può essere fusa nella base per ottenere un checkpoint proprietario, candidato a
sostituire `pixel-baseline` nel runtime previa asset review; fino ad allora il runtime
resta su `pixel-baseline`.

Il training si fa **su Kaggle** (`05-KAGGLE-TRAINING-RUNBOOK.md`, runbook primario, fino
a 30 ore di GPU gratuite a settimana). Il runbook RunPod (`dataset/TRAINING-RUNBOOK.md`)
resta come fallback a pagamento per quando Kaggle non basta.

**Nota (DEC-179, 28/07):** la risoluzione di lavoro **SD1.5 @ 512 è confermata** anche dopo
un confronto esplicito con SDXL @ 1024 (scartata: budget runtime 6 GB, informazione persa
dal downscale alla scala sprite di DEC-177, costo Kaggle circa triplo). Dettaglio e porte
aperte (checkpoint fuso di qualità, famiglie illustrative offline) nel decision-log.

## Struttura del dataset definitivo per famiglie (DEC-175)

DEC-148(e) affidava al proprietario del progetto la creazione dei dataset definitivi,
lasciando il corpus CC0 di `dataset/ledger.jsonl` come base solo per i primi esperimenti.
DEC-175 (28/07) fissa **come**: gli **sprite originali** prodotti con Aseprite (HUD,
personaggio, nemici, boss, oggetti, colpi, prop, con le animazioni nel formato a contratto
fisso di `08-PIPELINE-SPRITE-ANIMAZIONI.md`) sono **allo stesso tempo asset di gioco e
dataset definitivo delle LoRA**: non due produzioni separate, la stessa immagine serve
entrambi gli scopi.

Il dataset vive in `dataset/worldsmelt-style/`, organizzato per famiglia:

```text
dataset/worldsmelt-style/
├── _general/       # aggregato per la Style LoRA di base (worldsmelt-style)
├── character/      # per worldsmelt-identità/character-role LoRA dedicate
├── enemies/
├── bosses/
├── items/
├── shots/
├── ui/
└── props/
```

`_general/` raccoglie il sottoinsieme trasversale usato per la Style LoRA di base (sezione
`1. worldsmelt-style` della Gerarchia sotto); le altre cartelle alimentano le LoRA per
famiglia della stessa Gerarchia (`worldsmelt-enemies`, `worldsmelt-items`, LoRA di
identità, ecc.), non un rifacimento di quella gerarchia.

Regole:

- **Un file caption per immagine** (stesso nome base, estensione `.txt`), secondo il
  vocabolario della sezione `Caption` sotto e le caption obbligatorie per famiglia già
  definite sopra (es. `worldsmelt-enemies`).
- **Registrazione nel ledger** (`docs/ai-production/dataset/ledger.jsonl`,
  fonte unica dello schema in `dataset/README.md`) con `license_id: own` — gli sprite
  originali sono `own/original`, non CC0: provenienza propria, non di terze parti.
- I **189 sprite curati CC0** rimappati alla palette ufficiale «Fucina di Worldsmelt»
  (DEC-173) possono **integrare `_general/`**, ma con **provenienza distinta** nel ledger
  (`license_id: cc0`, non `own`): non si riscrive la loro provenienza originale solo
  perché sono stati rimappati di palette.

Questa struttura **non sostituisce** il corpus CC0 esistente (resta valido per gli
esperimenti già fatti, vedi `dataset/README.md`): lo affianca come dataset definitivo,
in costruzione via Aseprite.

## Gerarchia

### 1. `worldsmelt-style`

Impara:

- scala apparente dei pixel;
- contorni;
- palette;
- quantità di colori;
- shading;
- contrasto;
- prospettiva top-down three-quarter;
- proporzioni;
- ombra a terra;
- sfondo piatto;
- leggibilità della silhouette.

Non deve specializzarsi troppo su una sola categoria.

### 2. `worldsmelt-enemies`

Impara:

- body plan;
- forme organiche;
- materiali;
- ruoli visivi;
- silhouette;
- telegraph estetici;
- mostri e creature.

Caption obbligatorie:

```text
body_plan
role
locomotion
size
view
material
limb_count
attack_visual
```

### 3. `worldsmelt-items`

Impara:

- oggetti singoli centrati;
- icone;
- armi;
- pickup;
- equipaggiamento;
- leggibilità a dimensioni piccole.

### 4. `worldsmelt-environments`

Impara:

- pavimenti;
- muri;
- porte;
- ostacoli;
- decorazioni;
- tile e moduli ambientali.

Tenere separata dagli sprite trasparenti.

### 5. `worldsmelt-vfx`

Impara:

- proiettili;
- esplosioni;
- aura;
- particelle;
- telegraph;
- hit effects.

### 6. LoRA di identità

Una per personaggio importante, soltanto quando la Style LoRA e la pipeline pose sono
stabili.

## Prima configurazione

Baseline iniziale:

```yaml
base_model: stable-diffusion-v1-5/stable-diffusion-v1-5
resolution: 512
rank: 8
alpha: 8
learning_rate: 0.0001
train_unet: true
train_text_encoder: false
mixed_precision: fp16
batch_size: 1
gradient_accumulation: 4
max_train_steps: 1500
checkpointing_steps: 250
seed: 20260720
```

Fare un secondo run da 3000 step soltanto dopo la review dei checkpoint intermedi.

## Caption

Trigger consigliato:

```text
wsmeltpx
```

Esempio:

```text
wsmeltpx, top-down three-quarter single enemy sprite,
tentacled space controller, bone and flesh material,
large readable silhouette, flat background, game asset
```

Non usare soltanto `2.5D`: può produrre render tridimensionali, voxel o isometria.

## Split

Separare per identità:

- personaggio;
- pack/autore;
- animation_id;
- famiglia visiva;
- soggetto quasi duplicato.

Tutti i frame di una stessa animazione vanno nello stesso split.

## Quando considerare un checkpoint completo

Soltanto se:

- esistono migliaia di immagini pulite e coerenti;
- più LoRA non raggiungono la qualità richiesta;
- il modello deve perdere gran parte del comportamento generico;
- il costo di distribuzione è accettabile;
- la derivazione e la licenza sono documentate;
- il miglioramento è misurato in-engine.

## Cosa non fare

- Addestrare SD1.5 da zero su 89.000 sprite.
- Mischiare UI, tile, personaggi, VFX e nemici senza caption affidabili.
- Usare JPEG per il dataset finale.
- Allenare subito il text encoder.
- Valutare solo loss o immagini isolate.
- Scegliere l'ultimo checkpoint per principio.
- Riciclare automaticamente output generati nel training set.
