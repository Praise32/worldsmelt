# Worldsmelt AI Production Blueprint — Index

Questa cartella è la guida tecnica e produttiva per immagini, animazioni, UI, audio,
training, agenti e curation. Non sostituisce la game-design knowledge base.

## Inizio

- [README](README.md)
- [Decisioni canoniche](00-DECISIONI-CANONICHE.md)
- [Audit del progetto](01-AUDIT-DEL-PROGETTO.md)
- [Topic Router](22-TOPIC-ROUTER.md)
- [Questionario decisionale](19-DECISION-QUESTIONNAIRE.md)
- [Istruzioni di integrazione](23-INTEGRATION-INSTRUCTIONS.md)

## Immagini e modelli

- [Stack modelli](02-STACK-MODELLI.md)
- [Piano LoRA](03-PIANO-LORA.md)
- [Dataset e licenze](04-DATASET-LICENZE.md)
- [Kaggle Training Runbook](05-KAGGLE-TRAINING-RUNBOOK.md)
- [Agenti e Kaggle MCP](06-AGENTI-KAGGLE-MCP.md)
- [Protocollo esperimenti](11-PROTOCOLLO-ESPERIMENTI.md)

## Runtime, sprite e animazioni

- [Architettura runtime](07-ARCHITETTURA-RUNTIME.md)
- [Pipeline sprite e animazioni](08-PIPELINE-SPRITE-ANIMAZIONI.md)
- [Nemici, body plan e rig](09-NEMICI-BODY-PLAN-RIG.md)
- [Piano integrazione C](10-PIANO-INTEGRAZIONE-C.md)

## UI e audio

- [Pipeline UI e GUI](15-UI-DESIGN-PIPELINE.md)
- [Pipeline audio](16-AUDIO-GENERATION-PIPELINE.md)
- [Curation e Piano 0](17-ASSET-CURATION-AND-FLOOR-ZERO.md)

## Agenti e governance

- [Orchestrazione agenti](18-AGENT-ORCHESTRATION.md)
- [Questionario decisionale](19-DECISION-QUESTIONNAIRE.md)
- [Protocollo delle sessioni](20-SESSION-PROTOCOL.md)
- [Governance documenti](21-DOCUMENT-GOVERNANCE.md)
- [Topic Router](22-TOPIC-ROUTER.md)
- [Istruzioni di integrazione](23-INTEGRATION-INSTRUCTIONS.md)
- [Aggiornamenti KB proposti](24-PROPOSED-KB-UPDATES.md)
- [Prompt agenti](13-PROMPT-AGENTI.md)

## Roadmap e fonti

- [Roadmap](12-ROADMAP.md)
- [Fonti](14-FONTI.md)

## Template

- [Experiment Report](templates/EXPERIMENT-REPORT.md)
- [Dataset Ledger](templates/DATASET-LEDGER.md)
- [EnemySpec](templates/ENEMY-SPEC.md)
- [SpriteBundle](templates/SPRITE-BUNDLE.md)
- [Weekly Issue](templates/WEEKLY-ISSUE.md)
- [Decision Record](templates/DECISION-RECORD.md)
- [Question Batch](templates/QUESTION-BATCH.md)
- [Asset Review](templates/ASSET-REVIEW.md)
- [AudioSpec](templates/AUDIO-SPEC.md)
- [UISkinSpec](templates/UI-SKIN-SPEC.md)

## Configurazione agenti

- [Claude agent setup](agent-config/claude/README.md)
- [Path Orchestrator](agent-config/claude/agents/worldsmelt-path-orchestrator.md)
- [Decision Facilitator](agent-config/claude/agents/worldsmelt-decision-facilitator.md)
- [Knowledge Librarian](agent-config/claude/agents/worldsmelt-knowledge-librarian.md)
- [ML Pipeline Architect](agent-config/claude/agents/worldsmelt-ml-pipeline-architect.md)
- [UI Systems Designer](agent-config/claude/agents/worldsmelt-ui-systems-designer.md)
- [Audio Systems Designer](agent-config/claude/agents/worldsmelt-audio-systems-designer.md)
- [Asset Curator](agent-config/claude/agents/worldsmelt-asset-curator.md)
- [Codex appendix](agent-config/codex/AGENTS-AI-PRODUCTION-APPENDIX.md)

## Regola di precedenza

1. decision log della game-design knowledge base;
2. documento di game design approved;
3. decisione tecnica approvata;
4. blueprint;
5. piano;
6. esperimento;
7. output generato.

Un conflitto non viene risolto automaticamente.
