---
id: ai-prod-integration
status: active
owner: engineering
last_reviewed: 2026-07-20
summary: "Come copiare il pacchetto nel repository e attivare indici, agenti e sessioni."
---

# Integrazione nel repository

## Posizione

Copiare la cartella come:

```text
docs/worldsmelt-ai-production-blueprint/
```

## Primo commit

Non modificare codice. Aggiungere soltanto la documentazione e verificare i link.

## Claude Code

Copiare gli agenti da:

```text
agent-config/claude/agents/
```

a:

```text
.claude/agents/
```

Non sovrascrivere gli agenti esistenti. I nuovi agenti si affiancano a:

- `melting-implementer`;
- `melting-verifier`;
- `melting-content-designer`.

Aggiungere al root `CLAUDE.md`:

```text
Per task AI/UI/audio/curation, il punto d'ingresso è
docs/worldsmelt-ai-production-blueprint/INDEX.md.
Usare worldsmelt-path-orchestrator per classificare il task.
Non implementare una scelta presente in 19-DECISION-QUESTIONNAIRE.md come BLOCKING
senza risposta e aggiornamento della fonte canonica.
```

## Codex

Aggiungere al root `AGENTS.md` un riferimento equivalente, oppure copiare:

```text
agent-config/codex/AGENTS-AI-PRODUCTION-APPENDIX.md
```

nel documento principale, adattandolo.

## Prima sessione consigliata

Usare il prompt:

```text
Avvia una Decision Session per la milestone "AI Production Foundation".

Leggi:
- root AGENTS.md e CLAUDE.md;
- game-design-knowledge-base/docs/game-design/INDEX.md;
- game-design-knowledge-base/docs/game-design/governance/open-questions.md;
- docs/worldsmelt-ai-production-blueprint/INDEX.md;
- docs/worldsmelt-ai-production-blueprint/19-DECISION-QUESTIONNAIRE.md.

Seleziona al massimo sette domande BLOCKING o SOON necessarie per:
- SD1.5 e distribuzione LoRA;
- UI;
- audio;
- Piano 0;
- autorità degli agenti.

Non modificare codice.
Per ogni domanda mostra opzioni, raccomandazione e cosa blocca.
Dopo le mie risposte prepara le patch documentali, non l'implementazione.
```

## Seconda sessione

Dopo le risposte:

```text
Aggiorna la knowledge base, il decision log, gli open questions e la blueprint.
Poi prepara docs/plans/AI-PRODUCTION-FOUNDATION.md con milestone, file, test e gate.
Non avviare training.
```

## Terza sessione

Implementare soltanto Milestone 0:

- directory ML;
- policy;
- validator;
- notebook sottile;
- nessun training completo;
- nessuna nuova dipendenza nel gioco.
