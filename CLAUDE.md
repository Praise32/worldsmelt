# Worldsmelt — istruzioni di progetto (cartella principale)

Il gioco si chiama **Worldsmelt** (il repo conserva il nome storico in locale).
Regole tecniche dei moduli: vedi `AGENTS.md`. La fonte canonica del game design è
`game-design-knowledge-base/` (108 decisioni in `docs/game-design/governance/decision-log.md`):
prima di implementare comportamento visibile al giocatore, consultare l'INDEX della KB.
La cartella della KB ha un proprio CLAUDE.md per le sessioni di design: queste istruzioni
valgono per il lavoro di implementazione nella cartella principale.

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
- Il verifier esegue davvero le suite (`make test`, `test-gen`, `test-script`,
  `test-sprites`) e dà verdetto APPROVA/BOCCIA; nessun commit prima del verdetto.
- Ogni cambiamento verificato va committato e pushato subito su `main`.
