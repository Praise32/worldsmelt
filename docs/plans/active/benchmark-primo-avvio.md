---
id: plans-benchmark-primo-avvio
title: Benchmark al primo avvio con UI dedicata (differito, v1 manuale)
domain: plans
status: proposed
authority: supporting
owner: engineering
summary: >-
  Gap di design differito di proposito: oggi make benchmark è solo manuale e il gioco
  applica il preset lowspec leggendo logs/benchmark.txt; l'auto-run al primo avvio con UI
  dedicata resta da progettare e costruire.
last_reviewed: 2026-07-22
topics: [benchmark, low-spec, primo-avvio, ui, tier]
related: []
supersedes: []
source_files: [scripts/benchmark.sh, src/app/app.c]
---

# Benchmark al primo avvio (piano attivo, differito)

## Stato attuale (v1, implementato)

- `make benchmark` esegue in sequenza `melting-gen --bench` e `melting-sprites --bench`
  (mai insieme: VRAM) e scrive `logs/benchmark.txt`.
- Alla successiva `--generate` il gioco legge il file e applica da solo il preset
  `--low-spec` (testo 1.5B, sprite 256px) quando serve; i flag espliciti vincono sempre;
  il gioco non si blocca mai, nemmeno su `unsupported`.
- L'auto-run al primo avvio è stato **saltato di proposito** (decisione del 14/07: la
  macchina di riferimento era già tarata; ribadito nel Makefile: «NON parte da solo al
  primo avvio del gioco; l'auto-run arriverà con la UI dedicata»).

## Da fare (quando si riprende)

1. UI dedicata di primo avvio: eseguire il benchmark con progresso visibile e annullabile,
   dopo la schermata a due carte completo/solo-curato (DEC-070/DEC-086).
2. Persistenza e invalidazione: quando rimisurare (cambio hardware/driver)?
3. Integrazione con la open question 13 (lowspec vs DEC-070): la risposta decide se il
   preset resta automatico-silenzioso o va comunicato al giocatore.

## Vincoli

Il benchmark non tocca mai `generated/` (isolato); nessun tier di *download* dei modelli
(DEC-070: un solo set); il fallback non si rimuove.
