---
id: aiprod-session-protocol
title: Protocollo delle sessioni
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Definisce 6 tipi di sessione (decision, planning, implementation, ML experiment, curation, release audit), gate di ingresso e formato di handoff.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [sessioni, gate, handoff, processo, esperimenti]
related: []
supersedes: []
source_files: []
---
# Protocollo delle sessioni

> **Approvato il 2026-07-27 (DEC-164)** con una precisazione: per i ruoli di
> implementazione e giudizio, e per ogni punto in cui questo protocollo divergesse dalla
> scala di implementazione o dalle regole di chiusura task, prevale sempre `CLAUDE.md`
> (root), nello stesso spirito della nota di precedenza di
> [18-AGENT-ORCHESTRATION.md](18-AGENT-ORCHESTRATION.md). Questo documento resta il
> metodo per condurre le sessioni, non lo sostituisce.

## Tipi

### 1. Decision Session

Scopo: ottenere risposte umane e aggiornare la fonte canonica.

Input:

- milestone;
- INDEX;
- open questions;
- conflitti.

Output:

- massimo sette domande;
- risposte;
- decision record;
- documenti aggiornati;
- lista dei task ora pronti.

Non implementa codice.

### 2. Planning Session

Scopo: trasformare requisiti approved in un piano tecnico.

Output:

- file coinvolti;
- milestone;
- test;
- rischi;
- dipendenze;
- gate;
- stima S/M/L.

Non cambia comportamento di design.

### 3. Implementation Session

Scopo: eseguire un task Ready.

Flusso:

```text
orchestrator
→ implementer
→ verifier
→ fix/escalation
→ report
```

### 4. ML Experiment Session

Scopo: un solo esperimento.

Richiede:

- domanda;
- baseline;
- config;
- dataset;
- policy;
- smoke test;
- autorizzazione GPU;
- report.

### 5. Curation Session

Scopo: revisionare candidate.

Output:

- accepted/rejected/quarantine;
- tag;
- note;
- asset promossi;
- aggiornamento catalogo.

### 6. Release Audit Session

Scopo:

- licenze;
- NOTICE;
- modelli;
- dataset;
- fallback;
- piattaforme;
- riproducibilità;
- privacy;
- dimensioni e download.

## Apertura di una sessione

L'agente deve dichiarare internamente:

```text
session_type
task_id
canonical_docs
open_questions
write_scope
commands_allowed
gpu_allowed
network_allowed
acceptance_criteria
```

## Gate

### Design Ready

- comportamento documentato;
- domande bloccanti chiuse;
- scenari;
- fallback;
- originalità.

### Technical Ready

- moduli;
- contratto;
- migrazione;
- test;
- rollback.

### Experiment Ready

- dataset;
- licenze;
- baseline;
- policy;
- artifact path.

### Curation Ready

- candidate validi;
- provenance;
- scala in-engine;
- criteri.

## Formato domanda

```text
Q-ID — Titolo
Perché serve:
Cosa blocca:
Opzioni:
Raccomandazione:
Default temporaneo:
Risposta:
```

## Regola di stop

L'agente si ferma e domanda quando la decisione:

- modifica un documento approved;
- cambia licenza/distribuzione;
- aumenta requisito minimo;
- elimina fallback;
- introduce rete nel gioco;
- introduce inferenza in combattimento;
- rende irreversibile un dataset/modello;
- cambia il flusso visibile.

## Handoff

Ogni sessione termina con:

- stato;
- decisioni;
- file;
- test;
- artifact;
- rischi;
- prossima sessione consigliata;
- domande rimaste.
