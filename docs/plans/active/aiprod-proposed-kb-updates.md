---
id: plans-aiprod-proposed-kb-updates
title: Aggiornamenti proposti alla knowledge base
domain: plans
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Elenco di modifiche da trasferire nella KB canonica (audio, visual language, floor zero, validazione contenuti, run manifest) solo dopo decisione umana.
last_reviewed: 2026-07-22
topics: [kb-updates, audio, floor-zero, run-manifest, visual-language]
related: []
supersedes: []
source_files: []
---
# Aggiornamenti proposti alla knowledge base

Questo file non modifica automaticamente il design canonico.

## Audio

Documento:

```text
game-design-knowledge-base/docs/game-design/content/audio-and-feedback.md
```

Conflitto:

- stato attuale: audio curato/statico, generazione futura;
- proposta: pipeline generativa opzionale o di produzione.

Azione:

1. risolvere `Q-AUD-001`;
2. aggiungere una decisione;
3. aggiornare summary, sezione DEC-036, non-obiettivi e scenari;
4. mantenere fallback curato.

## Visual Language

Aggiunte possibili:

- UI costruita nello stesso linguaggio pixel-art;
- componenti modulari;
- compatibilità 9-slice;
- token semantici;
- vincoli per asset AI senza testo.

Richiede design review, non è solo implementazione.

## Floor Zero

Aggiunta possibile:

- struttura curata;
- libreria costruita dalle migliori generazioni storiche;
- generazione runtime soltanto per contenuto della run;
- identità del Piano 0 non dipendente dal successo dei modelli.

Risolvere `Q-F0-001`.

## Generated Content Validation

Aggiungere, se approvato:

- stati candidate/curated/quarantine;
- validazione UI/audio;
- promozione umana;
- non-contaminazione fra research e commercial.

## Run Manifest

Aggiungere contratti:

- `EnemySpec`;
- `SpriteBundle`;
- `UISkinSpec`;
- `AudioSpec`;
- hash e versioni;
- fallback.

Soltanto i campi percepiti dal giocatore appartengono alla KB; dettagli di serializzazione
restano tecnici.

## Open Questions

Non duplicare:

- controllo e camera;
- piattaforme;
- pool curato;
- valori da playtest.

Le questioni tecniche/produttive restano nel questionario della blueprint; quelle che
cambiano l'esperienza vanno promosse nella KB.
