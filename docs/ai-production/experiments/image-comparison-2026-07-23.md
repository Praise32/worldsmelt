---
id: aiprod-exp-image-comparison-2026-07-23
title: Comparison dei checkpoint immagine — 23/07/2026
domain: ai-production
status: implemented
authority: supporting
owner: ai-production
summary: >-
  Suite immagini su 3 checkpoint SD1.5 (15 coppie tema/stile della baseline x 2 seed,
  90 atlas totali salvati per revisione umana): tempi, celle scartate dal quality gate e
  incidente dei 9 timeout transitori documentato. Artefatti in
  logs/model-comparison/20260723-164914/images/.
last_reviewed: 2026-07-23
last_verified_commit: a737aba
topics: [comparison, immagini, sd15, checkpoint, atlas, baseline]
related: [aiprod-exp-model-comparison-testo-2026-07-23]
supersedes: []
source_files: [scripts/image-comparison.sh]
---

# Comparison dei checkpoint immagine (23/07/2026)

Tre checkpoint SD1.5 sulle **15 coppie tema/stile congelate** della baseline
«Esperimento 0» × 2 seed (5, 17), stessa pipeline del gioco (melting-sprites, LCM-LoRA,
quality gate delle celle). **Il giudizio di qualità è visivo e spetta all'utente**: gli
atlas sono in `logs/model-comparison/20260723-164914/images/<checkpoint>/`
(`atlas-NN-seedS.png`, indice leggibile in `index.txt` per cartella).

## Tabella

| Checkpoint | imgS bench (s) | load (s) | warmup (s) | Atlas | Timeout 1a corsa | Rigenerati | s/atlas medio | Celle scartate |
|---|---|---|---|---|---|---|---|---|
| pixel-baseline | 5.63 | 1.34 | 15.13 | 30/30 | 9 | 0 | 88s | 0 su 21 misurate |
| pixelart-alt | 5.68 | 1.28 | 23.97 | 30/30 | 0 | 0 | 87s | 0 su 30 misurate |
| sd15-vanilla | 5.68 | 0.13 | 14.82 | 30/30 | 0 | 0 | 124s | 161 su 30 misurate |

- **pixel-baseline** = `Public-Prompts-Pixel-Model.ckpt` (il checkpoint attuale del gioco).
- **sd15-vanilla** = SD1.5 puro: riferimento senza fine-tune pixel-art.
- **pixelart-alt** = fine-tune pixel-art alternativo (spritesheet-generator).

## Incidente documentato: 9 timeout transitori

Nella prima corsa, `pixel-baseline` ha avuto 9 generazioni consecutive in timeout
(coppie 03–07, ~300s l'una) in una finestra in cui la macchina scaricava i modelli
audio (6.6 GB) e preparava il venv torch: **rigenerate tutte e 9 a GPU libera in
77–84s l'una, zero fallimenti** — contesa transitoria di risorse, non un problema del
checkpoint. Le voci rigenerate sono marcate «rigenerato a GPU libera» in `index.txt`:
i loro tempi non sono confrontabili con quelli della corsa originale.

## Come confrontare

Aprire le tre cartelle affiancate ordinando per nome: stessa coppia e stesso seed hanno
lo stesso nome file nei tre checkpoint. Nessuna LoRA di stile è ancora in gioco (le
LoRA arriveranno con la campagna dataset): questo è il confronto **baseline** fra
checkpoint di partenza.
