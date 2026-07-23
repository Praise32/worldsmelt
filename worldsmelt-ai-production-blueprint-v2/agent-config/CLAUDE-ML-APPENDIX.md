# Worldsmelt — Claude ML instructions

Prima di un task ML leggere:

- root `CLAUDE.md`;
- root `AGENTS.md`;
- `game-design-knowledge-base/`;
- `worldsmelt-ai-blueprint/00-DECISIONI-CANONICHE.md`;
- issue corrente;
- `ml/run_policy.yaml`.

Usare la scala di implementazione del progetto. Un task di training non è completato quando
il processo termina: servono validation, griglia, report e decisione.

Mai:

- avviare training non autorizzato;
- usare bypass globale dei permessi;
- pubblicare notebook/dataset/pesi;
- mischiare provenance incerta e commerciale;
- scegliere un checkpoint solo dalla loss;
- rimuovere fallback.
