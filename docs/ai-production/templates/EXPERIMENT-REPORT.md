---
id: aiprod-experiment-report
title: Experiment Report
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Report esperimento ML completo: metadata, domanda unica, variabili, preflight, tabella metriche baseline/candidate, review umana, decisione, artifact.
last_reviewed: 2026-07-22
topics: [template, esperimento, ML, metriche, report]
related: []
supersedes: []
source_files: []
---
# Experiment Report

## Metadata

- Experiment ID:
- Date:
- Owner:
- Git commit:
- Base model:
- Base revision/hash:
- Dataset:
- Dataset hash:
- Config hash:
- Kaggle kernel:
- GPU:
- Runtime:

## Question

Una sola domanda verificabile.

## Changes

- Variable 1:
- Variable 2:

## Configuration

```yaml
```

## Preflight

- [ ] ledger valid
- [ ] duplicate check
- [ ] split check
- [ ] caption check
- [ ] smoke test
- [ ] resume test

## Results

| Metric | Baseline | Candidate | Delta |
|---|---:|---:|---:|
| valid asset rate | | | |
| fallback rate | | | |
| seconds/image | | | |
| VRAM | | | |
| palette pass | | | |
| silhouette pass | | | |

## Human Review

| Criterion | Score 1–5 | Notes |
|---|---:|---|
| readability | | |
| style | | |
| role recognition | | |
| animation feasibility | | |
| originality check | | |
| in-engine result | | |

## Failures

## Decision

- [ ] accept
- [ ] reject
- [ ] repeat with one change
- [ ] quarantine

Motivazione:

## Artifacts

- LoRA:
- Checkpoints:
- Grid:
- Logs:
- Recipe:
