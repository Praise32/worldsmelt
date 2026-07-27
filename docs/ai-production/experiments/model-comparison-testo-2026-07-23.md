---
id: aiprod-exp-model-comparison-testo-2026-07-23
title: Comparison dei modelli di testo — 23/07/2026
domain: ai-production
status: implemented
authority: supporting
owner: ai-production
summary: >-
  Suite di comparison su 11 modelli GGUF (3 seed fissi): gemma-3-4b-it Q4 migliore
  complessivo (84.9), Coder 1.5B Q4 miglior rapporto e minimo accettabile; base di DEC-140.
  Artefatti e valori grezzi in logs/model-comparison/20260723-125614/.
last_reviewed: 2026-07-27
last_verified_commit: 6ba3f40
topics: [comparison, modelli, benchmark, gemma, qwen, DEC-140]
related: []
supersedes: []
source_files: [scripts/model-comparison.sh, scripts/model_comparison_report.py]
---


# Report suite di comparazione modelli

Cartella: `logs/model-comparison/20260723-125614` -- vedi anche `report.csv` (stesse colonne, valori grezzi, per riordinare in un foglio di calcolo).

## Tabella comparativa

| Modello | Dim. (GiB) | tok/s (bench) | Tempo/run (s) | JSON 1° colpo | Lua 1° colpo | Lua valido (tot.) | Varietà (jaccard, ↓ meglio) | Aderenza tema | Guardia EN | Fotocopie | Punteggio | Note |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gemma-3-4b-it-q4_k_m | 2.32 | 52.6 | 272 | 100% | 93% | 100% | 0.00 | 15% | 0 hit | 0/3 | 84.9 | OK |
| qwen2.5-coder-7b-instruct-q5_k_m | 5.07 | 41.0 | 262 | 100% | 88% | 88% | 0.00 | 13% | 0 hit | 0/3 | 82.9 | OK |
| qwen2.5-coder-7b-instruct-q6_k | 5.82 | 33.5 | 302 | 100% | 81% | 81% | 0.00 | 10% | 0 hit | 0/3 | 79.8 | OK |
| qwen2.5-coder-7b-instruct-q4_k_m | 4.36 | 43.6 | 142 | 100% | 73% | 75% | 0.00 | 8% | 0 hit | 0/3 | 76.9 | OK |
| qwen2.5-coder-1.5b-instruct-q8_0 | 1.76 | 55.0 | 137 | 100% | 63% | 75% | 0.00 | 28% | 0 hit | 0/3 | 76.3 | OK |
| qwen2.5-coder-1.5b-instruct-q4_k_m | 1.04 | 103.0 | 118 | 100% | 67% | 86% | 0.00 | 5% | 0 hit | 0/3 | 74.2 | OK |
| qwen2.5-coder-3b-instruct-q8_0 | 3.37 | 53.8 | 140 | 100% | 62% | 93% | 0.00 | 9% | 0 hit | 0/3 | 73.0 | OK |
| qwen2.5-coder-3b-instruct-q4_k_m | 1.96 | 71.0 | 123 | 100% | 52% | 90% | 0.00 | 15% | 0 hit | 0/3 | 70.3 | OK |
| qwen3-4b-instruct-2507-q4_k_m | 2.33 | 58.4 | 232 | 100% | 0% | 0% | 0.00 | 18% | 0 hit | 0/3 | 52.8 | OK |
| phi-4-mini-instruct-q4_k_m | 2.32 | 61.0 | 305 | 100% | 6% | 39% | 0.00 | 4% | 0 hit | 0/3 | 52.6 | OK |
| qwen2.5-7b-instruct-q4_k_m | 4.36 | 40.5 | 209 | 100% | 2% | 68% | 0.09 | 12% | 0 hit | 0/3 | 50.3 | OK |

## Giudizio automatico

Soglie (piano, `docs/plans/completed/model-comparison.md`): Lua validi (primo colpo + dopo retry) >= 70%, JSON valido al primo tentativo su TUTTE le run campionate (nessun ripiego procedurale), zero run con piani fotocopia (< 5 temi distinti su 5 piani).

- **Migliore complessivo**: `gemma-3-4b-it-q4_k_m` (punteggio 84.9/100).
- **Migliore rapporto qualità/dimensione**: `qwen2.5-coder-1.5b-instruct-q4_k_m` (71.3 punti/GiB).
- **Più piccolo accettabile**: `qwen2.5-coder-1.5b-instruct-q4_k_m` (1.04 GiB, sopra tutte e tre le soglie).

## Dettaglio per modello

### gemma-3-4b-it-q4_k_m

- Dimensione file: 2.32 GiB
- Bench: tok/s=52.61, caricamento=5.73s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 272s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 93%, valido comunque 100%, ripiegato su mini-VM 0%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 15%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Vineyard of Velvet | Furnace of Cracking Glass | Lagoon of Eclipse | Cave of Amber | Tower of Glass
  - seed 4343: Foundry of Cracking Glass | Ossuary of Frozen Mist | Press House of Bottomless Sand | Printworks of Eternal Autumn | Crypt of Glowing Mold

### phi-4-mini-instruct-q4_k_m

- Dimensione file: 2.32 GiB
- Bench: tok/s=61.05, caricamento=4.36s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 305s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (36 oggetti totali): primo colpo 6%, valido comunque 39%, ripiegato su mini-VM 50%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 4%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Foundry of Cracking Glass | Vineyard of Velvet Ashes | Lagoon in Eclipse | Cavern of Amber Luminescence | Cellar of Neon
  - seed 4343: Presshouse of Steam | Printworks in Eternal Autumn | Crypt beneath Glowing Mold | Ossuary at Midnight Snowfall | Presshouse of Melted Ice

### qwen2.5-7b-instruct-q4_k_m

- Dimensione file: 4.36 GiB
- Bench: tok/s=40.55, caricamento=10.30s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 209s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 2%, valido comunque 68%, ripiegato su mini-VM 32%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.04, colpi 0.17, nemici 0.14, boss 0.11, stanze 0.06, oggetti 0.01
- Aderenza al tema (euristica lessicale): 12%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Tower of Velvet | Lagoon of Eclipse | Cave of Amber | Lagoon of Amber | Lagoon of Fire
  - seed 4343: Volcano of Sand | Aquarium of Radiation | Library of Mold | Forge of the Moon | Cathedral of the Moon

### qwen2.5-coder-1.5b-instruct-q4_k_m

- Dimensione file: 1.04 GiB
- Bench: tok/s=102.97, caricamento=0.62s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 118s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (58 oggetti totali): primo colpo 67%, valido comunque 86%, ripiegato su mini-VM 10%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.03
- Aderenza al tema (euristica lessicale): 5%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Foundry of Cold | Forge of Rust | Cave of Shadows | Tower of Fire | Vineyard of Leaves
  - seed 4343: Volcano Desert | Steam City Forge | Crypt of Bones | Ossuary of Stars | None

### qwen2.5-coder-1.5b-instruct-q8_0

- Dimensione file: 1.76 GiB
- Bench: tok/s=55.03, caricamento=4.63s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 137s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 63%, valido comunque 75%, ripiegato su mini-VM 10%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 28%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Forest of Shadows | Volcano of Ashes | Grotto of Bones | Iron Forge of Steel | Fairy Forest of Light
  - seed 4343: Volcano Peak | Press Room | Printworks | Crypt | Ossuary

### qwen2.5-coder-3b-instruct-q4_k_m

- Dimensione file: 1.96 GiB
- Bench: tok/s=70.95, caricamento=4.73s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 123s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 52%, valido comunque 90%, ripiegato su mini-VM 8%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 15%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Tower of Shadows | Vineyard of Amber | Lagoon of Eclipse | Furnace of Cracking Glass | Ossuary of Shattering Stone
  - seed 4343: Volcano Abyss | Press House Breach | Printworks Crack | Crypt Quake | Ossuary Collapse

### qwen2.5-coder-3b-instruct-q8_0

- Dimensione file: 3.37 GiB
- Bench: tok/s=53.78, caricamento=8.23s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 140s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 62%, valido comunque 93%, ripiegato su mini-VM 0%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 9%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Vineyard of Velvet | Tower of Eclipse | Cave of Flame | Lagoon of Amber | Furnace of Glass
  - seed 4343: Volcano Abyss | Press House | Printworks | Crypt of Shadows | Ossuary of Shadows

### qwen2.5-coder-7b-instruct-q4_k_m

- Dimensione file: 4.36 GiB
- Bench: tok/s=43.57, caricamento=11.33s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 142s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 73%, valido comunque 75%, ripiegato su mini-VM 0%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 8%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Ironworks of Twisted Metal | Ironworks of Shattered Metal | Ironworks of Corrupted Metal | Ironworks of Ruined Metal | Ironworks of Desolate Metal
  - seed 4343: Press of Eternal Autumn | Crypt of Shattering Glass | Ossuary of Cracking Glass | Forges of Cracking Glass | Volcanos of Cracking Glass

### qwen2.5-coder-7b-instruct-q5_k_m

- Dimensione file: 5.07 GiB
- Bench: tok/s=41.02, caricamento=12.71s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 262s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 88%, valido comunque 88%, ripiegato su mini-VM 0%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 13%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Temple of Fire | Vineyard of Eclipse | Lagoon of Cracking Glass | Cave of Shattering Glass | Tower of Crackling Glass
  - seed 4343: Printworks of Eternautumn | Press House of Midnight | Volcano of Sand | Ossuary of Cracking Glass | Crypt of Shattering Glass

### qwen2.5-coder-7b-instruct-q6_k

- Dimensione file: 5.82 GiB
- Bench: tok/s=33.51, caricamento=14.52s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 302s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (42 oggetti totali): primo colpo 81%, valido comunque 81%, ripiegato su mini-VM 0%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 10%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Temple of Fire | Forge of Steel | Cave of Echoes | Vineyard of Silence | Tower of Twilight
  - seed 4343: Volcano of Lava | Press House of Shadows | Printworks of Enigma | Crypt of Midnight | Ossuary of Corruption

### qwen3-4b-instruct-2507-q4_k_m

- Dimensione file: 2.33 GiB
- Bench: tok/s=58.35, caricamento=5.60s
- Run campionate: 3 (3 dal modello, 0 in ripiego procedurale, 0 fallite/timeout)
- Tempo medio per run completa: 232s
- JSON: primo colpo 100%, riuscito comunque 100%, tentativi medi 1.00
- Lua (60 oggetti totali): primo colpo 0%, valido comunque 0%, ripiegato su mini-VM 100%
- Varietà per categoria (jaccard medio fra run, ↓ meglio): temi 0.00, colpi 0.00, nemici 0.00, boss 0.00, stanze 0.00, oggetti 0.00
- Aderenza al tema (euristica lessicale): 18%
- Guardia inglese: 0 parole-funzione italiane trovate (0 atteso, DEC-052)
- Fotocopie: 0/3 run con < 5 temi distinti
- Campione temi (giudicare l'inglese a occhio):
  - seed 4242: Tower of Velvet | Tower of Eclipse | Tower of Amber Flame | Tower of Fractured Velvet | Tower of Hollow Light
  - seed 4343: Volcano of Glowing Mold | Volcano of Glowing Mold - Rotting | Volcano of Glowing Mold - Boiling | Volcano of Glowing Mold - Fissure | Volcano of Glowing Mold - Final Rift
