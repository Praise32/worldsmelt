---
id: meta-document-audit
title: Audit documentale completo — 2026-07-22
domain: meta
status: approved
authority: supporting
owner: meta
summary: >-
  Fotografia pre-migrazione dell'intera documentazione (180 documenti, 8 contenitori),
  con verdetti dei giudici, 46 conflitti, 35 drift doc-codice reali e il piano di promozione
  verso docs/. Prodotto dal workflow ws-doc-audit (68 agenti) e arbitrato da Fable.
last_reviewed: 2026-07-22
last_verified_commit: a2d5df7
topics: [audit, inventario, migrazione]
related: [meta-doc-conflicts, meta-doc-code-drift, meta-document-standards]
supersedes: []
source_files: [scripts/docs/build_knowledge_index.py]
---

# Audit documentale — 2026-07-22

Metodo: 15 domini d'inventario (analista sonnet → giudice opus), 9 topic di conflitto e 10
verifiche di drift doc↔codice (sonnet con Codebase Memory → giudice opus), arbitrato finale
Fable sui conflitti trasversali. Nessuna BOCCIA: 68/68 agenti completati.
Dettaglio per documento: [DOCUMENT-INVENTORY.json](DOCUMENT-INVENTORY.json).
Conflitti: [DOC-CONFLICTS.md](DOC-CONFLICTS.md). Drift: [DOC-CODE-DRIFT.md](DOC-CODE-DRIFT.md).

## Numeri

180 documenti analizzati, 746 decisioni/contenuti unici censiti nei key_decisions.

| Contenitore | Documenti |
|---|---|
| KB design | 68 |
| blueprint-v2 | 50 |
| appunti | 12 |
| research-pack | 12 |
| docs-legacy | 10 |
| superpowers | 9 |
| root | 8 |
| references-legacy | 6 |
| agenti | 3 |
| dataset | 2 |

| Azione proposta | Documenti |
|---|---|
| `keep` | 68 |
| `promote` | 66 |
| `needs-human-decision` | 26 |
| `archive` | 14 |
| `split` | 4 |
| `merge` | 2 |

| Tipo | Documenti |
|---|---|
| `canonical-design` | 51 |
| `reference` | 40 |
| `template` | 20 |
| `agent-config` | 17 |
| `canonical-ai-production` | 16 |
| `historical-note` | 11 |
| `generated-index` | 8 |
| `canonical-engineering` | 6 |
| `handoff` | 4 |
| `active-plan` | 4 |
| `experiment` | 1 |
| `completed-plan` | 1 |
| `cancelled-plan` | 1 |

Verdetti dei giudici (opus): inventario {'APPROVA': 4, 'APPROVA_CON_CORREZIONI': 11}, conflitti {'APPROVA': 3, 'APPROVA_CON_CORREZIONI': 6}, drift {'APPROVA_CON_CORREZIONI': 7, 'APPROVA': 3}.

## Baseline delle suite (registrata PRIMA della migrazione)

- `make test`: **rosso su `--states-test`** con dati di run locali in `catalog/`
  («su/giù su una categoria vuota ha spostato il focus voce»), **verde a catalog vuoto**.
  Difetto preesistente della schermata Catalogo (M8, `f8055bd`), dipendente dallo stato
  locale; il codice del gioco NON viene toccato da questa migrazione.
- `make test-script`, `make test-gen`, `make test-sprites`: verdi.
- `make test-llm`: flaky noto ~25% col 1.5B (HANDOFF 19/07), non eseguito in baseline.

## Esito dell'arbitrato (conflitti needs-human)

Nessuna decisione approvata modificata. Sei conflitti risolti con default reversibile +
open question (dettaglio e verdetti in DOC-CONFLICTS.md): audio generativo vs DEC-036;
soglia Enterprise Stability AI; etichetta licenza LCM-LoRA (fix fattuale in
`scripts/download-models.sh`); director-per-stile vs DEC-038; tier hardware S/A/B/C vs
DEC-070; preset lowspec automatico vs DEC-070.

## Le dieci divergenze doc↔codice più rilevanti

1. `docs/ARCHITECTURE.md`: dichiara la mini-VM come comportamento predefinito; la sandbox
   Lua è il percorso primario (DEC-037, `src/script/`). → riscritto in engineering.
2. `README.md`: quickstart Windows/OpenAI in testa e «sprite locali / sandbox Lua» come
   fasi future: sono implementate. → riscritto (percorso Linux/locale di riferimento).
3. `docs/superpowers/plans/2026-07-13-linux-local-llm.md`: 81 step «[ ]» ma fasi 0-1 e
   roadmap complete. → archiviato come piano storico completato.
4. `docs/DESIGN_NOTES.md`: sandbox Lua presentata come ipotesi non scelta (pre-DEC-037).
   → archiviato.
5. `docs/OPENAI_SETUP.md` + `llm/`: pipeline sidecar OpenAI/Node superata dal percorso
   locale; `current_atlas.png` non è più esclusivo di quel percorso. → archiviato.
6. `docs/BENCHMARKS.md`: numeri senza data/macchina/commit e «tok/s» ambiguo fra due
   misure. → promosso in engineering con contesto e disambiguazione.
7. Spec con header «proposta/da approvare» citate dal codice come fonte vincolante
   (lua-sandbox-design & co.). → promosse in engineering con status reale.
8. `src/core/room_layout.h` cita una spec inesistente (2026-07-16-step-3c-rooms.md);
   commenti citano `docs/superpowers/sdd/` (cartella non tracciata). → commenti corretti.
9. Vincoli duri Vulkan-only/niente ROCm/niente flash-attention mai promossi. → ADR-001.
10. Fase 5 del piano (benchmark al primo avvio) differita di proposito ma mai registrata
    come piano attivo. → `docs/plans/active/`.

## Piano di promozione (eseguito nei batch successivi)

- KB → `docs/design/` (struttura preservata; INDEX+README fusi nel README curato;
  `docs/technical/multiplayer-steam.md` → engineering; template piani → `docs/plans/`).
- `worldsmelt_sintesi_strategica.md` → `docs/design/vision/` (supporting, non canonico).
- Stato tecnico verificato → `docs/engineering/` (+ `adr/`, `specs/`, `img/`).
- blueprint-v2 → `docs/ai-production/` (+ `templates/`; 16-AUDIO bloccato da DEC-036;
  19-QUESTIONARIO coda viva; 24-PROPOSED → `docs/plans/active/`); appendici ML fuse in
  `docs/ai-production/regole-agenti-ml.md`; agent-config → `.claude/agents/worldsmelt-*`.
- `docs/dataset/` → `docs/ai-production/dataset/` (con aggiornamento dei percorsi in
  `scripts/dataset_ledger.py`, `scripts/sprite-baseline.sh`, Makefile).
- Ricerca → `docs/references/research/` (5 note di genere + research pack verbatim);
  `LOCAL_REFERENCES.md` → `docs/references/external/`.
- Storia → `docs/archive/` (appunti, OPENAI_SETUP, DESIGN_NOTES, ISSUE_NOTES, APPUNTI,
  piano linux-local-llm, manifest KB, HANDOFF completo; stub breve al posto di HANDOFF.md).
- Poi: profili (CLAUDE.md root + docs/CLAUDE.md), puntatori (~60 commenti nel codice,
  AGENTS.md, README, memoria agente), `make docs-index && make docs-check` verdi,
  reindicizzazione Codebase Memory, e SOLO ALLA FINE l'eliminazione dei contenitori vuoti.
