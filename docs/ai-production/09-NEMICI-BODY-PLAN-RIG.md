---
id: aiprod-nemici-body-plan-rig
title: Nemici: body plan e rig
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Catalogo di 11 body plan e 10 ruoli meccanici per i nemici, formato EnemySpec, regole di validazione e vincoli di roster per run.
last_reviewed: 2026-07-22
topics: [body-plan, rig, enemy-spec, roster, nemici]
related: []
supersedes: []
source_files: []
---
# Nemici: body plan e rig

## Problema

Un modello di diffusione non può garantire spritesheet coerenti per qualunque creatura
immaginabile. La soluzione è separare:

- varietà estetica, molto ampia;
- grammatica corporea, finita;
- comportamento, parametrico o sandboxato;
- animazione, deterministica.

## Catalogo iniziale

| Body plan | Componenti | Movimento |
|---|---|---|
| biped | body, head, arms, legs, weapon | clip base + socket |
| quadruped | torso, 4 legs, head, tail | cycle procedurale o 4–6 frame |
| mounted | mount + rider + weapon | due rig sincronizzati |
| crawler | body + leg pair | fase alternata delle zampe |
| blob | body | squash/stretch |
| serpentine | head + N segmenti + tail | catena con ritardo |
| tentacled | core + segment + tip | catene sinusoidali |
| flying | body + shadow | bob e drift |
| turret | base + rotating head | rotazione e recoil |
| swarm | unit sprite | steering di gruppo |
| multipart_boss | core + parti | rig gerarchico e fasi |

## Ruoli meccanici

Il body plan non è il ruolo. Ruoli iniziali:

- chaser;
- shooter;
- charger;
- space_controller;
- support;
- summoner;
- obstacle;
- splitter;
- bomber;
- phased_boss.

Qwen sceglie una combinazione valida da una matrice di compatibilità.

## EnemySpec

```json
{
  "id": "marrow_oracle",
  "origin": "nuovo",
  "role": "space_controller",
  "body_plan": "tentacled",
  "rig": "tentacle_chain_v1",
  "size": "large",
  "locomotion": "stationary",
  "material": ["bone", "flesh"],
  "palette_tags": ["ivory", "dark_red", "violet"],
  "parts": {
    "tentacle_count": 6,
    "segments_per_tentacle": 5
  },
  "attack": {
    "pattern": "radial_delayed_bursts",
    "telegraph": "tip_glow",
    "cooldown_band": "slow"
  },
  "required_assets": [
    "body",
    "tentacle_segment",
    "tentacle_tip",
    "projectile"
  ],
  "fallback": "curated_tentacle_controller_01"
}
```

## Validazione

- body plan presente nel registry;
- rig compatibile;
- numero di parti entro budget;
- ruolo supportato;
- attacco supportato;
- telegraph definito;
- footprint e hitbox entro limiti;
- asset richiesti noti;
- fallback esistente.

## Esempi

### Cavaliere a cavallo

Asset:

- mount body;
- set gambe;
- rider torso/head;
- arma.

Rig:

- saddle;
- rider hand;
- weapon tip;
- ground pivot.

### Tentacolare

Asset:

- core;
- segmento;
- punta.

Il numero di tentacoli e segmenti è deciso dal manifest entro budget. Non si generano tutte
le combinazioni.

### Blob

Un solo sprite può bastare. Animazioni:

- squash;
- stretch;
- flash;
- ombra;
- frammenti.

### Flying

Un solo sprite + ombra:

- bob;
- roll;
- opacity;
- trail;
- recoil.

## Roster per run

La varietà globale può essere grande, ma la singola run deve avere un roster leggibile:

- circa 6–8 archetipi principali;
- varianti veterane;
- boss;
- palette e tema coerenti.

Il giocatore impara pattern; il modello cambia forma, materiale e combinazioni.
