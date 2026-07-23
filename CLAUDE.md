# Worldsmelt — profilo di IMPLEMENTAZIONE (root)

Il gioco si chiama **Worldsmelt** (il repo conserva il nome storico in locale).
Questo profilo governa il lavoro sul codice (motore raylib, tools, sandbox Lua, script).
Per le sessioni di documentazione/design vale il profilo gemello **`docs/CLAUDE.md`**.

Regole tecniche dei moduli: vedi `AGENTS.md`. La fonte canonica del game design è
**`docs/design/`** (140 decisioni in `docs/design/governance/decision-log.md`): prima di
implementare comportamento visibile al giocatore, consultare `docs/design/README.md`
(percorso curato) o `docs/design/INDEX.md` (indice generato). Il router
task→dominio è `docs/_meta/TOPIC-ROUTER.md`. Per i task ML valgono anche le regole di
`docs/ai-production/regole-agenti-ml.md`.

## Scala di implementazione (delega ed escalation)

Chi implementa parte dal gradino più basso adatto al task; **il giudice sta sempre un
gradino sopra chi ha implementato**. Una bocciatura del giudice (o un task che non regge
dopo 2 tentativi sullo stesso gradino) fa salire il task di un gradino.

| Gradino | Implementa | Giudica | Quando si parte da qui |
|---|---|---|---|
| 1 | `melting-implementer` (**haiku**, default) | `melting-verifier` (**sonnet**, default) | Task semplici, meccanici, ben specificati (rinomine, piccoli fix, test aggiuntivi, modifiche localizzate) |
| 2 | `melting-implementer` con `model: sonnet` | `melting-verifier` con `model: opus` | Task medi (feature circoscritte, refactoring di un modulo) o escalation dal gradino 1 |
| 3 | `melting-implementer` con `model: opus` | **Fable direttamente** (diff + test, nessun sottoagente) | Task difficili/trasversali o escalation dal gradino 2 |
| 4 | **Fable direttamente** | — | Caso estremo: fallito anche con opus, o decisione architetturale delicata |

- La scala vale anche dentro i Workflow (`opts.model` / `opts.effort` per agente).
- `melting-content-designer` (prompt e contenuti) resta su sonnet, giudicato da opus.
- Gli specialisti `worldsmelt-*` (`.claude/agents/`: path-orchestrator, knowledge-librarian,
  decision-facilitator, ml-pipeline-architect, ui-systems-designer, audio-systems-designer,
  asset-curator) NON sostituiscono la scala: preparano piani e materiale nei loro domini;
  l'implementazione C passa comunque da `melting-implementer` + giudice.
- Il verifier esegue davvero le suite (`make test`, `test-gen`, `test-script`,
  `test-sprites`) e dà verdetto APPROVA/BOCCIA; nessun commit prima del verdetto.
- Ogni cambiamento verificato va committato e pushato subito su `main`.

## Regole di chiusura di un task

1. Chi cambia comportamento del gioco aggiorna nello stesso lavoro il documento di design
   pertinente (o apre una open question in `docs/design/governance/open-questions.md`).
2. Chi tocca `docs/` esegue `make docs-index && make docs-check` prima del commit.
3. Difetti noti e baseline dei test: `docs/engineering/known-issues.md` (non mascherarli).
4. Stato sintetico del lavoro per chi arriva dopo: `HANDOFF.md` (breve; la storia sta in
   `docs/archive/handoffs/`).
