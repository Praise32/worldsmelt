---
id: ref-pack-modello-linguistico-unico
title: Modello linguistico unico per generare codice e run
domain: references
status: proposed
authority: supporting
owner: design
summary: >-
  Definisce baseline (Qwen2.5-Coder-7B) e candidato (Qwen3-4B-Instruct-2507), criteri di benchmark Worldsmelt-specifico e requisiti di riproducibilita' cross-hardware.
last_reviewed: 2026-07-22
topics: [scelta-modello-llm, qwen, benchmark, riproducibilita, quantizzazione]
related: []
supersedes: []
source_files: []
---

# Modello linguistico unico per generare codice e run

## Il compito reale

Il modello deve combinare cinque capacità:

1. comprendere il tema e la direzione della run;
2. progettare una meccanica nuova;
3. scrivere Lua corretto usando solo l’API concessa;
4. reagire agli errori del compilatore/dry-run;
5. rispettare budget, bilanciamento e leggibilità.

Non basta essere bravo nei benchmark di coding generico. Deve essere bravo nel **micro-linguaggio operativo Worldsmelt**.

## Baseline già provata

**Qwen2.5-Coder-7B-Instruct Q4_K_M** è la baseline di qualità:

- già integrato;
- Apache 2.0;
- 4,53 GiB di VRAM con offload completo sulla RX 5600 XT;
- circa 49,6 secondi per il benchmark attuale;
- migliore varietà rispetto al 1.5B documentata nella repository.

Non va abbandonato finché un modello più piccolo non lo eguaglia sul benchmark specifico.

## Candidato più promettente per la distribuzione

**Qwen3-4B-Instruct-2507 Q4** è il candidato da misurare per primo:

- circa 4B parametri;
- non-thinking, utile per output vincolati e latenza;
- instruction following e coding;
- licenza Apache 2.0;
- dimensione e memoria sensibilmente inferiori al 7B;
- maggiore plausibilità su Steam Deck.

Il fatto che non si chiami “Coder” non è decisivo. La qualità va misurata sui task Worldsmelt.

## Modelli di controllo

Per evitare una scelta per marca, una suite iniziale può confrontare senza addestramento:

- Qwen2.5-Coder-7B Q4_K_M — baseline;
- Qwen3-4B-Instruct-2507 Q4 — candidato principale;
- Phi-4-mini-instruct Q4 — controllo 3,8B;
- eventuale piccolo modello recente soltanto come esperimento.

Alla fine si distribuisce un solo modello.

## Perché non scegliere automaticamente il più piccolo

Un modello piccolo può:

- compilare più spesso ma produrre archetipi banali;
- copiare gli esempi del prompt;
- fallire logica con stato;
- usare funzioni inesistenti;
- non comprendere sinergie tra più sistemi;
- richiedere più retry, annullando il vantaggio di velocità.

La metrica corretta è:

> **meccaniche nuove e accettate per minuto totale, inclusi retry e simulazione.**

## Benchmark Worldsmelt consigliato

Creare 150–200 casi, raggruppati per difficoltà:

### Base

- colpo extra condizionale;
- knockback;
- timer;
- targeting;
- stat-up valido.

### Composizione

- terzo colpo con comportamento differente;
- effetto dopo N rimbalzi;
- attivazione sotto soglia vita;
- bersaglio più vicino con fallback;
- interazione tra due oggetti.

### Primitive nuove

- laser/raycast;
- catena tra nemici;
- orbita;
- area geometrica mobile;
- clone temporaneo;
- campo che devia i proiettili;
- scambio posizione controllato.

### Nemici e boss

- pattern a fasi;
- risposta alla distanza;
- telegraph prima dell’attacco;
- memoria di eventi;
- adattamento agli oggetti posseduti.

## Punteggio

| Criterio | Peso suggerito |
|---|---:|
| Compila e supera dry-run | 20% |
| Produce l’effetto richiesto | 25% |
| Novità rispetto agli esempi | 20% |
| Sicurezza/costo runtime | 15% |
| Bilanciamento e leggibilità | 10% |
| Latenza totale con retry | 10% |

Registrare anche:

- numero di retry;
- funzioni allucinate;
- fallback;
- istruzioni eseguite;
- entità create;
- picchi di memoria;
- tempo di prefill e decode;
- similarità con esempi del prompt;
- percentuale di codice morto.

## Stessa qualità su hardware diverso

Congelare:

- repository e revision del modello;
- SHA-256 del GGUF;
- ricetta di quantizzazione;
- commit del runtime;
- tokenizer;
- chat template;
- system prompt;
- GBNF/schema;
- sampling;
- versione API Lua.

Per run condivisibili, salvare e condividere il `RunBundle` finale. Non affidarsi alla rigenerazione bit-identica tra backend differenti: floating point e sampling possono divergere tra CPU, Vulkan e GPU diverse.

## Ottimizzazioni senza addestramento multiplo

- contesto reale 4–8K, non 100K;
- prompt più corto e API più idiomatica;
- KV cache del prefisso di sistema;
- più output in una chiamata solo quando l’affidabilità non cala;
- retry mirato con errore breve;
- grammar per wrapper e metadati, Lua validato dal compilatore;
- custom imatrix sul corpus Worldsmelt;
- quantizzazione mista verificata;
- scaricamento completo del modello prima del gameplay.
