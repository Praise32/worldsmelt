# Claude agents

Copiare i file in `.claude/agents/` senza rimuovere gli agenti già presenti.

Punto d'ingresso:

```text
worldsmelt-path-orchestrator
```

Il path orchestrator:

- non sostituisce `melting-implementer`;
- non sostituisce `melting-verifier`;
- usa il decision facilitator in caso di domande;
- usa specialisti per preparare piani;
- richiede sempre verifica separata.

I custom subagent sono la modalità base. Agent Teams è opzionale per task davvero
parallelizzabili.
