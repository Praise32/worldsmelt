---
id: aiprod-roadmap
title: Roadmap operativa
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Roadmap in 9 milestone (repository ML -> Style LoRA -> multi-LoRA -> SpriteBundle -> body plan -> Enemy LoRA -> Piano 0 -> umanoide -> produttizzazione) con gate e ordine da non invertire.
last_reviewed: 2026-07-27
topics: [roadmap, milestone, piano-0, lora, produttizzazione]
related: []
supersedes: []
source_files: [tools/melting-sprites]
---
# Roadmap operativa

## Stato al 27/07

La Milestone 0 è in gran parte già coperta al di fuori della struttura `ml/` qui
descritta: il registro di provenienza (`scripts/dataset_ledger.py`, `dataset/ledger.jsonl`),
la baseline congelata (`dataset/baseline-prompts.txt`, `logs/sprite-baseline/`) e il tool
CLI (`tools/melting-sprites`) esistono già. La comparison di seconda generazione del 23/07
ha confermato SD1.5 come base immagini (DEC-148) e ha reso Gemma-3-4B-IT Q4 il modello di
testo di riferimento (DEC-140). La Milestone 1 (Style LoRA v0) non è ancora avviata: il
training si farà su Kaggle, runbook primario (DEC-168).

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

- il modello di testo attivo produce EnemySpec;
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

- downloader modelli;
- NOTICE;
- modalità solo-curato;
- pulizia cache;
- crash recovery;
- telemetria locale opzionale.

Nota (DEC-110, DEC-142): niente tier di qualità né auto-run del benchmark al primo avvio —
il preset `--low-spec` è stato rimosso, il piano `benchmark-primo-avvio` è annullato, e il
requisito hardware minimo si esprime in numeri misurati (VRAM/RAM/OS), non in nomi di
modello o in tier. Resta valida l'informazione sull'hardware mostrata al primo avvio
prevista da DEC-086.

## Ordine da non invertire

Non fare prima:

- checkpoint completo;
- ControlNet custom;
- AnimateDiff finale;
- migliaia di nemici;
- fine-tuning del modello di testo;
- UI definitiva.

Prima dimostrare una singola pipeline completa e riproducibile.
