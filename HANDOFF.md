# HANDOFF

Stato sintetico del lavoro. La cronologia completa delle sessioni passate è in
`docs/archive/handoffs/` (integrale: `docs/archive/handoffs/HANDOFF-2026-07-19.md`).

## Stato al 2026-07-22

- **Branch**: `main` (tutto committato e pushato; policy: ogni cambiamento verificato va
  subito su main).
- **Ultimo lavoro**: unificazione documentale completa — un'unica knowledge base sotto
  `docs/` per domini (design/engineering/ai-production/plans/references/archive/_meta),
  standard e verifica in `make docs-check`, audit in `docs/_meta/DOCUMENT-AUDIT.md`.
- **Test**: `make test-script`, `test-gen`, `test-sprites` verdi. `make test` è **rosso su
  `--states-test` quando `catalog/` contiene run locali** (difetto preesistente M8, vedi
  `docs/engineering/known-issues.md`); verde a catalog vuoto. `test-llm` flaky noto ~25%
  col 1.5B.
- **WIP / blocchi**: nessuno sul codice. Cinque open question nuove (12-16) in
  `docs/design/governance/open-questions.md` aspettano il proprietario (audio generativo,
  lowspec vs DEC-070, fallback hardware, director-per-stile, licenze Stability).
- **Prossimo task naturale**: backlog noto in `docs/engineering/known-issues.md` (RNG di
  gameplay ancora `time(NULL)` → gare asincrone; test del Catalogo dipendente dallo stato
  locale) oppure campagna Style LoRA (`docs/ai-production/dataset/TRAINING-RUNBOOK.md`,
  serve il tuo account GPU).

## Orientarsi

- Implementazione: `CLAUDE.md` (scala agenti) + `AGENTS.md` (regole moduli).
- Documentazione/design: `docs/CLAUDE.md` + `docs/design/README.md`.
- Indice generale: `docs/INDEX.md` (rigenera con `make docs-index`).
