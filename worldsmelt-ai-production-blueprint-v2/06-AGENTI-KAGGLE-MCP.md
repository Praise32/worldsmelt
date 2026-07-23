# Codex/Claude + Kaggle MCP

Verificato sulle documentazioni ufficiali disponibili al 20 luglio 2026.

## Architettura consigliata

```text
Codex CLI o Claude Code
        |
        | modifica/test repository
        v
GitHub / working tree
        |
        | Kaggle MCP oppure Kaggle CLI
        v
Kaggle Notebook GPU
        |
        | output, log, checkpoint, griglie
        v
agente analizza e prepara patch/report
```

L'agente non deve consumare la sessione GPU per attività di ragionamento che può svolgere
sul computer locale o nel proprio ambiente.

## Kaggle MCP

Endpoint ufficiale:

```text
https://www.kaggle.com/mcp
```

### Codex

La configurazione più esplicita è project-scoped:

```toml
# .codex/config.toml
[mcp_servers.kaggle]
url = "https://www.kaggle.com/mcp"
auth = "oauth"
default_tools_approval_mode = "writes"
tool_timeout_sec = 120
```

Poi:

```bash
codex mcp login kaggle
codex mcp list
```

L'impostazione `writes` richiede approvazione per operazioni non read-only.

### Claude Code

```bash
claude mcp add --transport http kaggle \
  --scope project https://www.kaggle.com/mcp

claude mcp login kaggle
claude mcp list
```

In alternativa, aprire Claude e usare `/mcp`.

## Kaggle CLI fallback

La CLI ufficiale supporta push, status e output dei kernel:

```bash
kaggle kernels push -p kaggle/worldsmelt-style-lora
kaggle kernels status USER/worldsmelt-style-lora
kaggle kernels output USER/worldsmelt-style-lora \
  -p artifacts/kaggle/style-lora-v0
```

Il kernel metadata decide il file, le sorgenti e la visibilità; l'acceleratore può essere
passato quando disponibile.

## Policy di autorizzazione

File canonico:

```yaml
approved_gpu_run: false
max_training_steps: 3000
max_automatic_retries: 1
require_smoke_test: true
require_dataset_validation: true
allow_dataset_download: true
allow_model_upload: false
allow_public_notebook: false
```

L'agente può preparare tutto senza autorizzazione, ma non avvia il training completo quando
`approved_gpu_run` è falso.

## Codex non interattivo

Esempio concettuale:

```bash
codex exec \
  "Leggi AGENTS.md e ml/run_policy.yaml. Valida il dataset, esegui lo smoke test e prepara il run Kaggle. Non avviare il training completo senza approvazione."
```

Usare sandbox e permessi coerenti con la propria installazione. Evitare bypass globali.

## Claude non interattivo

```bash
claude -p \
  "Leggi CLAUDE.md e ml/run_policy.yaml. Prepara il prossimo esperimento Kaggle, esegui le verifiche consentite e produci un report." \
  --allowedTools "Read,Edit,Bash"
```

La modalità `--bare` non carica automaticamente CLAUDE.md o MCP: usarla soltanto passando
esplicitamente configurazione e contesto.

## Flusso settimanale

1. Creare una issue con un solo obiettivo.
2. L'agente prepara branch, config, test e report baseline.
3. Il verifier esegue dataset validation e smoke test.
4. Il proprietario imposta `approved_gpu_run: true`.
5. L'agente avvia Kaggle.
6. L'agente controlla stato e scarica output.
7. L'agente genera report comparativo.
8. Il proprietario approva o rifiuta.
9. Un solo vincitore viene promosso.

## Regole anti-spreco

- non cambiare più di due iperparametri per run;
- non rilanciare automaticamente più di una volta;
- non cancellare checkpoint;
- non rendere pubblici dataset o pesi;
- non usare segreti nelle celle;
- non addestrare senza baseline;
- non promuovere un run senza griglia in-engine;
- non usare il dataset Kaggle incerto nel ramo commerciale.
