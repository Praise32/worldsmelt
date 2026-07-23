---
id: aiprod-prompt-agenti
title: Prompt per agenti
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Cinque prompt di esempio per sessioni agente: preparazione esperimento, analisi output Kaggle, multi-LoRA, primo SpriteBundle blob, review licenze.
last_reviewed: 2026-07-22
topics: [prompt, agenti, kaggle, multi-lora, licenze, sprite-bundle]
related: []
supersedes: []
source_files: [AGENTS.md, CLAUDE.md, tools/melting-sprites]
---
# Prompt per agenti

## Preparazione esperimento

```text
Leggi AGENTS.md, CLAUDE.md, docs/design/ e
worldsmelt-ai-blueprint/.

Obiettivo: preparare l'esperimento indicato nell'issue corrente.

Vincoli:
- non avviare training GPU completo se ml/run_policy.yaml non contiene
  approved_gpu_run: true;
- eseguire dataset validation;
- eseguire smoke test;
- non cambiare più di due variabili;
- non usare dataset research nel ramo commercial;
- non inserire segreti;
- non eliminare artifact;
- generare report e git diff.

Consegna:
- file modificati;
- test eseguiti;
- comando Kaggle;
- rischi;
- decisioni che richiedono review umana.
```

## Analisi output Kaggle

```text
Scarica gli output dell'ultimo run Kaggle autorizzato.

Verifica:
- completamento;
- environment;
- hash;
- loss e NaN;
- checkpoint;
- griglie;
- metriche automatiche;
- tasso di fallback.

Confronta con la baseline usando gli stessi prompt e seed.
Non scegliere il vincitore solo dalla loss.
Genera experiments/<id>/report.md.
Non promuovere il modello senza review in-engine.
```

## Implementazione multi-LoRA

```text
Implementa supporto multi-LoRA in tools/melting-sprites mantenendo compatibilità con
il singolo --lora.

Requisiti:
- massimo 8 LoRA;
- sintassi path:multiplier;
- errori chiari;
- file mancante non deve causare crash;
- test parser;
- test limite;
- l'ordine degli adattatori deve essere conservato;
- recipe JSON con hash;
- nessuna modifica al binario del gioco;
- eseguire make test-sprites e le suite richieste da AGENTS.md.
```

## Primo bundle blob

```text
Implementa il primo SpriteBundle per un nemico body_plan=blob.

Mantieni:
- atlas legacy;
- fallback geometrico;
- rendering corrente degli altri nemici.

Aggiungi:
- loader JSON;
- texture;
- pivot;
- hitbox;
- profilo squash/stretch;
- test bundle invalido;
- screenshot o test visivo riproducibile.
```

## Review licenze

```text
Analizza tutti i nuovi file del dataset.

Per ogni file richiedi:
- autore;
- origine;
- licenza;
- URL;
- hash;
- permesso commerciale;
- gruppo split.

Sposta in quarantine ogni file incompleto.
Non dedurre una licenza dall'estensione o dal nome della cartella.
```
