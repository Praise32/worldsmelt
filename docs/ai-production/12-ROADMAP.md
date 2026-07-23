---
id: aiprod-roadmap
title: Roadmap operativa
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Roadmap in 9 milestone (repository ML -> Style LoRA -> multi-LoRA -> SpriteBundle -> body plan -> Enemy LoRA -> Piano 0 -> umanoide -> produttizzazione) con gate e ordine da non invertire.
last_reviewed: 2026-07-22
topics: [roadmap, milestone, piano-0, lora, produttizzazione]
related: []
supersedes: []
source_files: [tools/melting-sprites]
---
# Roadmap operativa

## Milestone 0 — Repository ML

- creare `ml/`;
- config YAML;
- validator;
- experiment registry;
- prompt congelati;
- run policy;
- kernel Kaggle sottile;
- nessun training.

Gate: smoke test locale/import del dataset.

## Milestone 1 — Style LoRA v0

- 150–300 immagini clean;
- rank 8;
- 1500 step;
- checkpoint 250;
- griglie;
- report.

Gate: miglioramento cieco rispetto a SD1.5 base.

## Milestone 2 — Multi-LoRA runtime

- modificare `melting-sprites`;
- LCM + Style;
- test CLI;
- recipe;
- benchmark.

Gate: stessa pipeline legacy con due adattatori.

## Milestone 3 — Primo SpriteBundle

Nemico blob:

- un'immagine;
- pivot;
- hitbox;
- squash/stretch;
- fallback.

Gate: import e animazione raylib.

## Milestone 4 — Body plan

Aggiungere:

- flying;
- tentacled;
- biped.

Gate: quattro famiglie con spec validata.

## Milestone 5 — Enemy LoRA v0

- dataset statico per body plan;
- caption strutturate;
- prompt di ruolo;
- valutazione silhouette/role.

Gate: miglioramento sui quattro rig.

## Milestone 6 — Piano 0

- Qwen produce EnemySpec;
- registry assegna rig;
- SD genera asset minimi;
- barra stato;
- cache;
- pubblicazione atomica;
- fallback.

Gate: Piano 1 sempre avviabile.

## Milestone 7 — Umanoide animato

- reference canonica;
- tre direzioni;
- pose guide;
- frame chiave;
- ControlNet esistente se necessario;
- socket.

Gate: idle/walk/attack coerenti.

## Milestone 8 — Produttizzazione

- benchmark primo avvio;
- tier;
- downloader modelli;
- NOTICE;
- modalità solo-curato;
- pulizia cache;
- crash recovery;
- telemetria locale opzionale.

## Ordine da non invertire

Non fare prima:

- checkpoint completo;
- ControlNet custom;
- AnimateDiff finale;
- migliaia di nemici;
- Qwen fine-tuning;
- UI definitiva.

Prima dimostrare una singola pipeline completa e riproducibile.
