---
id: aiprod-exp-audio-benchmark-2026-07-23
title: Primo benchmark audio — Stable Audio 3 Small su CPU (23/07/2026)
domain: ai-production
status: implemented
authority: supporting
owner: ai-production
summary: >-
  Prima esecuzione reale di DEC-109: Stable Audio 3 Small (sfx+music) su CPU — SFX 4s in
  ~7.7s, musica 20s in ~13.6s (0.68x realtime), 5 GB RAM; 20 clip salvate per revisione
  umana in logs/model-comparison/audio-20260723-172702/. Ricetta ambiente in scripts/.
last_reviewed: 2026-07-23
last_verified_commit: 8c5aec8
topics: [audio, stable-audio-3, benchmark, cpu, DEC-109, DEC-113]
related: []
supersedes: []
source_files: [scripts/audio_benchmark.py, scripts/audio-benchmark.sh]
---

# Benchmark audio -- Stable Audio 3 Small (DEC-109/DEC-113)

Generato: 2026-07-23 17:30:46
Ricetta: steps=8, cfg_scale=1.0, sampler_type="pingpong", device=cpu (float32)
Caricamento: pesi letti direttamente da models/stable-audio-3-small-{sfx,music}/ (model_config.json + model.safetensors + text-encoder t5gemma-b-b-ul2/), nessuna chiamata di rete (HF_HUB_OFFLINE=1/TRANSFORMERS_OFFLINE=1).

## SFX (models/stable-audio-3-small-sfx)

- Modello: `/home/meri/progetti/melting-run-gpu/models/stable-audio-3-small-sfx`
- Caricamento: 6.4s
- Warmup (prima clip, scartata dalla media): sfx-fusion-completed seed=4242 -> 8.1s (2.02x la durata)
- A regime (11 clip, warmup escluso): 7.7s/clip medio, rapporto medio 1.93x la durata richiesta
- RAM di picco (resource.getrusage, processo intero): 4996 MiB

| prompt | seed | durata (s) | tempo gen. (s) | rapporto | warmup | file |
|---|---|---|---|---|---|---|
| fusion completed, metallic pour and sizzle | 4242 | 4.0 | 8.1 | 2.02x | si | `sfx-fusion-completed-seed4242.wav` |
| fusion completed, metallic pour and sizzle | 4343 | 4.0 | 7.9 | 1.97x |  | `sfx-fusion-completed-seed4343.wav` |
| player takes damage, dull metallic hit | 4242 | 4.0 | 8.3 | 2.07x |  | `sfx-damage-taken-seed4242.wav` |
| player takes damage, dull metallic hit | 4343 | 4.0 | 8.1 | 2.01x |  | `sfx-damage-taken-seed4343.wav` |
| room cleared, short triumphant chime | 4242 | 4.0 | 8.0 | 2.00x |  | `sfx-room-cleared-seed4242.wav` |
| room cleared, short triumphant chime | 4343 | 4.0 | 8.2 | 2.05x |  | `sfx-room-cleared-seed4343.wav` |
| item pickup, bright metallic tick | 4242 | 4.0 | 7.3 | 1.82x |  | `sfx-item-pickup-seed4242.wav` |
| item pickup, bright metallic tick | 4343 | 4.0 | 7.6 | 1.89x |  | `sfx-item-pickup-seed4343.wav` |
| boss enters new phase, deep ominous impact | 4242 | 4.0 | 7.3 | 1.82x |  | `sfx-boss-phase-seed4242.wav` |
| boss enters new phase, deep ominous impact | 4343 | 4.0 | 7.4 | 1.86x |  | `sfx-boss-phase-seed4343.wav` |
| bomb explosion, stylized blast | 4242 | 4.0 | 7.4 | 1.86x |  | `sfx-bomb-explosion-seed4242.wav` |
| bomb explosion, stylized blast | 4343 | 4.0 | 7.5 | 1.87x |  | `sfx-bomb-explosion-seed4343.wav` |

## Music (models/stable-audio-3-small-music)

- Modello: `/home/meri/progetti/melting-run-gpu/models/stable-audio-3-small-music`
- Caricamento: 9.2s
- Warmup (prima clip, scartata dalla media): music-dark-fantasy-dungeon seed=4242 -> 13.7s (0.69x la durata)
- A regime (7 clip, warmup escluso): 13.6s/clip medio, rapporto medio 0.68x la durata richiesta
- RAM di picco (resource.getrusage, processo intero): 5057 MiB

| prompt | seed | durata (s) | tempo gen. (s) | rapporto | warmup | file |
|---|---|---|---|---|---|---|
| dark fantasy dungeon theme, ominous and rhythmic | 4242 | 20.0 | 13.7 | 0.69x | si | `music-dark-fantasy-dungeon-seed4242.wav` |
| dark fantasy dungeon theme, ominous and rhythmic | 4343 | 20.0 | 13.9 | 0.69x |  | `music-dark-fantasy-dungeon-seed4343.wav` |
| the Crucible hub, warm forge ambience with distant hammering | 4242 | 20.0 | 13.5 | 0.68x |  | `music-crucible-hub-seed4242.wav` |
| the Crucible hub, warm forge ambience with distant hammering | 4343 | 20.0 | 13.3 | 0.67x |  | `music-crucible-hub-seed4343.wav` |
| cavern ambience with dripping water and low drones | 4242 | 20.0 | 13.8 | 0.69x |  | `music-cavern-ambience-seed4242.wav` |
| cavern ambience with dripping water and low drones | 4343 | 20.0 | 13.7 | 0.68x |  | `music-cavern-ambience-seed4343.wav` |
| final floor theme, intense dark orchestral loop | 4242 | 20.0 | 13.7 | 0.69x |  | `music-final-floor-theme-seed4242.wav` |
| final floor theme, intense dark orchestral loop | 4343 | 20.0 | 13.6 | 0.68x |  | `music-final-floor-theme-seed4343.wav` |

## Note ambiente

Misura esterna (/usr/bin/time -v, processo intero):

- 	Percent of CPU this job got: 555%
- 	Elapsed (wall clock) time (h:mm:ss or m:ss): 3:41.83
- 	Maximum resident set size (kbytes): 5178284

Warning unici osservati nel log (vedi logs/model-comparison/audio-20260723-172702/run.log per il contesto completo):

- /home/meri/venvs/stable-audio/lib/python3.12/site-packages/torch/nn/utils/weight_norm.py:143: FutureWarning: `torch.nn.utils.weight_norm` is deprecated in favor of `torch.nn.utils.parametrizations.weight_norm`.
