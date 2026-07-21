---
id: meta-document-standards
title: Standard documentale di Worldsmelt
domain: meta
status: approved
authority: canonical
owner: meta
summary: >-
  Regole vincolanti per tutta la documentazione sotto docs/: albero dei domini,
  front matter obbligatorio, stati, autorita', gerarchia delle fonti, formati
  DOC-CONFLICT/DOC-CODE-DRIFT e strumenti make docs-check/docs-index/docs-audit.
last_reviewed: 2026-07-22
last_verified_commit: 75c8ab2
topics: [standard, front-matter, governance, indice]
related: [meta-topic-router]
supersedes: []
source_files: [scripts/docs/build_knowledge_index.py]
---

# Standard documentale di Worldsmelt

La documentazione canonica del progetto vive **tutta** sotto `docs/`. La lingua canonica è
l'**italiano**; front matter e identificatori tecnici restano in inglese. Ogni indice e
manifest è un **livello derivato** dai Markdown canonici: si rigenera con gli strumenti qui
sotto e non si modifica a mano.

## 1. Albero dei domini

```text
docs/
├── README.md          → pagina d'ingresso umana
├── CLAUDE.md          → profilo documentazione per gli agenti (vedi CLAUDE.md a root)
├── INDEX.md           → indice GENERATO (make docs-index)
├── design/            → che cosa deve essere e far vivere il gioco (fonte canonica di design)
├── engineering/       → come funziona davvero il progetto nel codice (+ adr/)
├── ai-production/     → modelli, LoRA, dataset, training, asset generati
├── plans/             → active/ completed/ cancelled/
├── references/        → external/ research/ (materiale di consultazione, mai canonico)
├── archive/           → legacy-notes/ superseded/ handoffs/ historical-plans/
└── _meta/             → standard, router, inventario, audit, report generati
```

Regole di collocazione: design valido → `design/`; stato tecnico reale verificato →
`engineering/` (decisioni architetturali datate → `engineering/adr/`); training, modelli,
dataset e cura degli asset → `ai-production/`; piani → `plans/{active,completed,cancelled}/`;
ricerca e materiale esterno → `references/`; tutto ciò che è superato → `archive/` (mai
cancellare un documento le cui decisioni valide non siano già state promosse in fonte canonica).

## 2. Front matter obbligatorio

Ogni documento **vivo** (tutto fuorché `docs/archive/`, i file generati e i `CLAUDE.md`)
inizia con questo front matter YAML:

```yaml
---
id: design-floor-zero            # univoco in tutto docs/, kebab-case, prefisso di dominio
title: Piano 0 — hub giocabile
domain: design                   # design | engineering | ai-production | plans | references | archive | meta
status: approved                 # draft | proposed | experimental | approved | implemented | superseded | deprecated | archived
authority: canonical             # canonical | supporting | historical | generated
owner: design                    # chi risponde del documento: design | engineering | ai-production | meta
summary: >-
  Una-due righe in italiano: che cosa copre il documento.
last_reviewed: 2026-07-22        # data dell'ultima revisione umana/di sessione
last_verified_commit: 75c8ab2    # obbligatorio per status approved/implemented
topics: [floor-zero, hub, generazione]
related: [design-run-structure]  # id di altri documenti
supersedes: []                   # id dei documenti che questo sostituisce
source_files: [src/world/floor_zero.c]  # file di codice citati (devono esistere)
---
```

Semantica dei campi delicati:

- **status** riflette lo stato REALE: un piano già realizzato è `implemented` (o `superseded`
  se sostituito), mai `approved`-per-sempre. `approved`/`implemented` richiedono
  `last_verified_commit` (il commit contro cui il contenuto è stato verificato l'ultima volta).
- **authority**: `canonical` = fa fede in caso di conflitto nel suo dominio; `supporting` =
  utile ma non vincolante; `historical` = conservato per memoria, **escluso dagli indici di
  default**; `generated` = prodotto da uno strumento, non modificare a mano.
- **supersedes**: se A supersede B, B deve avere status `superseded`/`deprecated`/`archived`
  e non può restare `canonical` (violazione = conflitto di authority, `docs-check` fallisce).

## 3. Gerarchia delle fonti (precedenza in caso di conflitto)

1. decision-log approvato (`docs/design/governance/decision-log.md`);
2. documento `approved` in `design/`;
3. ADR approvato in `engineering/adr/`;
4. documentazione engineering verificata contro il codice;
5. documentazione ai-production approvata;
6. piano attivo; 7. esperimento; 8. riferimento; 9. archivio storico; 10. output generato.

Conflitti fra fonti dello stesso rango non si risolvono da soli: si apre una domanda in
`docs/design/governance/open-questions.md`. Il codice **non** è fonte canonica di game
design; è la fonte necessaria dello stato tecnico reale (via Codebase Memory + verifica
diretta). Distinzione obbligatoria nelle analisi: **INTENDED** (design/decisioni) vs
**DOCUMENTED AS-IS** (ciò che un doc dichiara) vs **OBSERVED AS-IS** (ciò che il codice mostra).

## 4. Formato dei conflitti e del drift

- **DOC-CONFLICT-NNN** (in `docs/_meta/DOCUMENT-AUDIT.md` o file dedicati): fonti coinvolte,
  dominio, decisione coinvolta (DEC-NNN), evidenza, raccomandazione secondo la gerarchia,
  rischio, eventuale domanda umana, proponente, verificatore, verdetto.
- **DOC-CODE-DRIFT-NNN**: documento, affermazione (DOCUMENTED AS-IS), evidenza Codebase
  Memory, conferma diretta nel codice (file:riga), classificazione
  (`doc-superato | doc-impreciso | codice-avanti | codice-indietro | allineato | incerto`),
  azione raccomandata.

## 5. Strumenti

```bash
make docs-index   # rigenera KNOWLEDGE_MANIFEST.json, docs/INDEX.md e gli INDEX di dominio
make docs-check   # verifica vincolante: exit 1 su qualunque violazione
make docs-audit   # rigenera docs/_meta/LINK-REPORT.md e docs/_meta/STALE-DOCUMENTS.md
```

`docs-check` fallisce su: front matter mancante o incompleto nei doc vivi; `id` duplicati;
link interni rotti; `related`/`supersedes` verso id inesistenti; `source_files` inesistenti;
`canonical` senza `owner`; `approved`/`implemented` senza `last_verified_commit`; conflitto
di authority (doc superseded ancora canonical); manifest/INDEX non rigenerati dopo una
modifica; puntatori rotti nei file di profilo a root (`README.md`, `AGENTS.md`, `CLAUDE.md`,
`HANDOFF.md`, `.claude/agents/*.md`). `docs/archive/` è escluso da tutti i controlli (i suoi
file possono restare senza front matter); i file generati portano il marcatore
`<!-- GENERATED -->` e non vanno mai editati a mano.

## 6. Regole di aggiornamento

- Chi cambia comportamento del gioco aggiorna **nello stesso lavoro** il documento di design
  pertinente (o apre una open question) e almeno uno scenario verificabile.
- Chi verifica un documento contro il codice aggiorna `last_reviewed` e
  `last_verified_commit`.
- Un documento si archivia spostandolo in `docs/archive/...` (status `archived`), **solo
  dopo** che le sue decisioni ancora valide sono state promosse nella fonte canonica.
- Prima di ogni commit che tocca `docs/`: `make docs-index && make docs-check`.
