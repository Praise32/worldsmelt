---
id: meta-topic-router
title: Topic router — da task a dominio documentale
domain: meta
status: approved
authority: canonical
owner: meta
summary: >-
  Mappa ogni tipo di task alla parte giusta della knowledge base: quale dominio
  consultare, con quale strumento, e chi implementa/giudica secondo la scala agenti.
last_reviewed: 2026-07-22
last_verified_commit: 75c8ab2
topics: [router, navigazione, agenti]
related: [meta-document-standards]
supersedes: []
source_files: [scripts/docs/build_knowledge_index.py]
---

# Topic router

Prima regola: se il task tocca comportamento visibile al giocatore, si parte SEMPRE dal
design canonico (`docs/design/`), poi si scende nel tecnico. Il percorso curato di lettura
del design è `docs/design/README.md`; l'indice completo generato è `docs/design/INDEX.md`.

| Task | Dove guardare | Strumenti/note |
|---|---|---|
| Gameplay, regole, contenuti, bilanciamento, flussi | `docs/design/` (systems/, content/, governance/decision-log.md) | Le 108+ decisioni DEC-NNN fanno fede; dubbi → `governance/open-questions.md` |
| UI / schermate / HUD | `docs/design/ui/` + `docs/design/05-game-states-and-flow.md` | Stati canonici in `src/app` |
| Audio | `docs/design/content/audio-and-feedback.md` (DEC-109, fallback da DEC-036) + `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` | Conflitti audio → decision-log prima di tutto |
| Modifica a un modulo C | `docs/engineering/` + Codebase Memory (`search_graph`, `trace_path`, `get_code_snippet`) | Poi `AGENTS.md` per i confini dei moduli |
| Architettura / dipendenze / build | `docs/engineering/architecture.md`, `Makefile`, `AGENTS.md` | Decisioni datate in `docs/engineering/adr/` |
| Sandbox Lua / sicurezza script | `docs/engineering/specs/2026-07-13-lua-sandbox-design.md` + `src/script/` | Mai ampliare l'allowlist senza barriera + test |
| Modelli, LoRA, SD, Kaggle, training, dataset | `docs/ai-production/` | Licenze e separazione research/commercial-clean incluse |
| Sprite / asset generati / Piano 0 curato | `docs/ai-production/` + `docs/design/systems/floor-zero.md` | |
| Piani di lavoro | `docs/plans/active/` (poi completed/cancelled) | Un piano finito si sposta, non si riscrive |
| Ricerca esterna, benchmark di mercato | `docs/references/research/` | Mai canonico: informa, non decide |
| Pulizia/riorganizzazione documentazione | `docs/_meta/` + `make docs-check` | Standard in `_meta/DOCUMENT-STANDARDS.md` |
| Storia del progetto, vecchi appunti | `docs/archive/` | Escluso da indici e ricerca di default |

## Scala agenti (implementa/giudica)

Definita in `CLAUDE.md` (root). In sintesi: si parte dal gradino più basso adatto
(haiku→sonnet→opus→Fable), il giudice sta sempre un gradino sopra chi ha implementato,
una bocciatura fa salire di gradino. Per l'implementazione: `melting-implementer` +
`melting-verifier`; per i contenuti generativi: `melting-content-designer`.

## Prima di implementare comportamento

1. `docs/design/README.md` → documento pertinente → decision-log.
2. Codebase Memory per l'impatto (`trace_path`, impact analysis).
3. Implementa secondo la scala; il verifier esegue le suite vere.
4. Aggiorna il doc di design/engineering nello stesso lavoro (`last_reviewed`).
5. `make docs-index && make docs-check` se hai toccato `docs/`.
