---
id: aiprod-agent-orchestration
title: Orchestrazione degli agenti
domain: ai-production
status: approved
authority: supporting
owner: ai-production
summary: >-
  Path Orchestrator che classifica i task in 7 percorsi (design/technical/ML/UI/audio/curation/implementation) e regola quando porre domande bloccanti.
last_reviewed: 2026-07-22
last_verified_commit: 892911a
topics: [orchestrazione, path-orchestrator, agenti, claude-code, codex, decision-facilitator]
related: []
supersedes: []
source_files: [.claude/agents/]
---
# Orchestrazione degli agenti

> **Approvato il 2026-07-22** con una precisazione: per i ruoli di implementazione e
> giudizio prevale sempre la scala di implementazione di `CLAUDE.md` (root); questo
> documento la integra per l'orchestrazione dei domini di produzione IA, non la sostituisce.

## Obiettivo

Evitare che Codex o Claude prendano decisioni di prodotto o architettura in silenzio.

Il sistema deve distinguere:

1. decisione di design;
2. decisione tecnica reversibile;
3. esperimento;
4. implementazione;
5. verifica;
6. curation.

## Path Orchestrator

L'agente principale non implementa automaticamente. Prima sceglie il percorso.

```text
richiesta
  ↓
consulta root AGENTS/CLAUDE
  ↓
consulta KB INDEX + blueprint INDEX
  ↓
classifica il task
  ↓
controlla stato e domande
  ├── conflitto/blocco → Decision Facilitator → domande all'utente
  └── pronto → piano → specialista → verifier → report
```

## Percorsi

### DESIGN_PATH

Per:

- comportamento visibile;
- flussi;
- UI;
- audio percepito;
- regole del Piano 0;
- contenuto;
- difficoltà.

Fonte canonica: game-design knowledge base.

### TECHNICAL_PATH

Per:

- formati;
- moduli;
- tool;
- cache;
- processi;
- IPC;
- manifest;
- integrazione C;
- backend.

Fonte: `docs/technical/`, blueprint e codice.

### ML_EXPERIMENT_PATH

Per:

- dataset;
- LoRA;
- Kaggle;
- prompt;
- benchmark;
- modelli;
- post-processing.

Richiede policy GPU e protocollo esperimenti.

### UI_PATH

Richiede:

- documento UI specifico;
- visual language;
- token;
- accessibilità;
- Penpot/export;
- renderer;
- verifier visivo.

### AUDIO_PATH

Richiede:

- `audio-and-feedback.md`;
- decisione su DEC-036;
- AudioSpec;
- licenza;
- ascolto umano;
- fallback.

### CURATION_PATH

Richiede:

- provenienza;
- manifest;
- review;
- promozione;
- catalogo.

### IMPLEMENTATION_PATH

Usa la scala già definita nel root `CLAUDE.md`:

- implementer;
- verifier un gradino sopra;
- escalation dopo bocciatura o due fallimenti.

## Regola delle domande

L'orchestrator domanda soltanto quando:

- esiste un conflitto fra documenti;
- manca una scelta che cambia comportamento o costi;
- non esiste un default autorizzato;
- una decisione è difficile da invertire;
- licenza o distribuzione dipendono dalla scelta;
- il task cambierebbe un documento approved.

Non deve chiedere per:

- nomi locali;
- dettagli meccanici reversibili;
- implementazione già specificata;
- informazioni presenti nei documenti;
- scelte coperte da un default approvato.

## Batch

Massimo sette domande per batch. Ogni domanda contiene:

- ID;
- contesto;
- cosa blocca;
- opzioni;
- raccomandazione;
- default temporaneo;
- conseguenze.

Dopo la risposta:

1. aggiornare il documento canonico;
2. aggiungere decision record;
3. rimuovere o chiudere la domanda;
4. aggiornare indice/manifest;
5. soltanto dopo avviare implementazione.

## Claude Code

Il pacchetto contiene agenti custom da copiare in `.claude/agents/`. Il main agent può
delegare ai subagent. Agent Teams può essere valutato per task paralleli, ma non è
necessario per la prima versione.

## Codex

Codex usa lo stesso protocollo attraverso:

- root `AGENTS.md`;
- indice;
- topic router;
- question queue;
- prompt di sessione;
- verifier indipendente o seconda sessione.

Non dipendere da funzionalità specifiche di un solo agente per la correttezza del processo.

## Tracciabilità

Ogni sessione produce:

```text
docs/plans/<date>-<task>.md
docs/decisions/<id>.md, se necessario
experiments/<id>/report.md, se ML
git diff
test output
verdict
```
