<!-- GENERATED: make docs-index -- non modificare a mano -->

# Indice della documentazione di Worldsmelt

Rigenerato da `make docs-index`. Punto d'ingresso umano: [README.md](README.md).
Standard e regole: [_meta/DOCUMENT-STANDARDS.md](_meta/DOCUMENT-STANDARDS.md).

- [`design/`](design/INDEX.md) — 61 documenti
- [`engineering/`](engineering/INDEX.md) — 1 documenti
- [`ai-production/`](ai-production/INDEX.md) — 0 documenti
- `plans/`:
  - [Piano: titolo (template)](plans/PLAN_TEMPLATE.md) — Scheletro per piani di implementazione temporanei: obiettivo, documenti consultati, ambiguità, sequenza di lavoro, verifiche.
- `_meta/`:
  - [Registro drift documentazione-codice (DOC-CODE-DRIFT-001..035)](_meta/DOC-CODE-DRIFT.md) — Divergenze fra cio' che i documenti dichiarano e cio' che il codice mostra, verificate con Codebase Memory + conferma diretta (file:riga) e giudice opus. 35 divergenze reali, 43 conferme di allineamento.
  - [Registro dei conflitti documentali (DOC-CONFLICT-001..046)](_meta/DOC-CONFLICTS.md) — 46 conflitti rilevati dall'audit del 2026-07-22 (sonnet propone, opus verifica, Fable arbitra i needs-human), con raccomandazione secondo la gerarchia delle fonti e risoluzione adottata.
  - [Audit documentale completo — 2026-07-22](_meta/DOCUMENT-AUDIT.md) — Fotografia pre-migrazione dell'intera documentazione (180 documenti, 8 contenitori), con verdetti dei giudici, 46 conflitti, 35 drift doc-codice reali e il piano di promozione verso docs/. Prodotto dal workflow ws-doc-audit (68 agenti) e arbitrato da Fable.
  - [Standard documentale di Worldsmelt](_meta/DOCUMENT-STANDARDS.md) — Regole vincolanti per tutta la documentazione sotto docs/: albero dei domini, front matter obbligatorio, stati, autorita', gerarchia delle fonti, formati DOC-CONFLICT/DOC-CODE-DRIFT e strumenti make docs-check/docs-index/docs-audit.
  - [Topic router — da task a dominio documentale](_meta/TOPIC-ROUTER.md) — Mappa ogni tipo di task alla parte giusta della knowledge base: quale dominio consultare, con quale strumento, e chi implementa/giudica secondo la scala agenti.

`archive/` è escluso dagli indici: contiene solo materiale storico.
