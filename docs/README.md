---
id: docs-readme
title: Documentazione di Worldsmelt — mappa dei domini
domain: meta
status: approved
authority: canonical
owner: meta
summary: >-
  Pagina d'ingresso di docs/: la knowledge base unica del progetto, organizzata per domini
  (design, engineering, ai-production, plans, references, archive, _meta) con indici
  generati e verifica automatica.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [documentazione, indice, domini]
related: []
supersedes: []
source_files: [scripts/docs/build_knowledge_index.py]
---

# Documentazione di Worldsmelt

Tutta la conoscenza del progetto vive qui, in un'unica knowledge base per domini.
L'indice completo generato è [INDEX.md](INDEX.md) (`make docs-index`); le regole del
formato sono in [_meta/DOCUMENT-STANDARDS.md](_meta/DOCUMENT-STANDARDS.md); il router
task→dominio è [_meta/TOPIC-ROUTER.md](_meta/TOPIC-ROUTER.md).

| Dominio | Risponde a | Punto d'ingresso |
|---|---|---|
| [`design/`](design/README.md) | Che cosa deve essere e far vivere il gioco? (**canonico**) | `design/README.md`, decision-log |
| [`engineering/`](engineering/README.md) | Come funziona davvero il progetto nel codice? | `engineering/README.md` |
| [`ai-production/`](ai-production/README.md) | Come si scelgono/allenano/integrano modelli, LoRA, asset? | `ai-production/README.md` |
| `plans/` | Piani di lavoro (active / completed / cancelled) | `plans/active/` |
| `references/` | Ricerca e materiale esterno (mai canonico) | `references/research/` |
| `archive/` | Storia: appunti superati, vecchi handoff, doc sostituiti | escluso dagli indici |
| `_meta/` | Standard, router, audit, inventario, report generati | `_meta/DOCUMENT-STANDARDS.md` |

In caso di conflitto fra fonti vale la gerarchia definita nello standard (il decision-log
di design vince su tutto). Per gli agenti: profilo di implementazione in `CLAUDE.md` a
root, profilo di documentazione in [CLAUDE.md](CLAUDE.md) qui accanto.

`README.md` resta alla radice del repo come pagina iniziale GitHub.
