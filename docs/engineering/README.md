---
id: eng-readme
title: Engineering di Worldsmelt — stato tecnico reale
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Punto d'ingresso del dominio engineering: cosa contiene la cartella, il
  principio "stato tecnico verificato contro il codice" e come si mantiene
  allineata con gli strumenti docs-check/docs-index/docs-audit.
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [indice, engineering, navigazione, adr]
related: [eng-adr-001, eng-adr-002, eng-adr-003, meta-document-standards]
supersedes: []
source_files: [Makefile, scripts/setup-deps.sh]
---

# Engineering di Worldsmelt

Questa cartella è **lo stato tecnico reale** del progetto: come funziona davvero il
codice oggi, verificato contro `src/`, `tools/`, `scripts/` e il `Makefile` — non
come dovrebbe funzionare secondo un'idea o un piano. Il design (che cosa deve fare
il gioco, dal punto di vista del giocatore) vive interamente in
[`docs/design/`](../design/README.md), fonte canonica separata con il proprio
[Decision Log](../design/governance/decision-log.md) (DEC-001..DEC-108). Le due
cartelle non si contraddicono per costruzione: `docs/design/` decide *cosa* deve
succedere, `docs/engineering/` documenta *come* il codice lo fa succedere davvero
oggi; quando un documento engineering descrive un comportamento visibile al
giocatore, deve essere coerente con la decisione di design corrispondente, mai
inventarne una nuova.

## Cosa contiene

- **[`architecture.md`](architecture.md)** — mappa verificata dei moduli C, dei
  processi esterni (`melting-gen`, `melting-sprites`) e dei confini di
  sicurezza (sandbox Lua, mini-VM di ripiego).
- **[`benchmarks.md`](benchmarks.md)** — misure di velocità di generazione
  (testo e sprite) sulla macchina di riferimento.
- **[`dependencies.md`](dependencies.md)** — librerie native vendorizzate in
  `deps/` (versioni pinnate da `scripts/setup-deps.sh`, vedi anche ADR-001) con
  ruolo e binario che le linka, più i repository locali di sola consultazione.
- **[`known-issues.md`](known-issues.md)** — difetti e limiti tecnici NOTI e
  verificati nel codice reale, con sintomo, evidenza (file:riga) e stato
  attuale; non è un elenco di idee o un backlog di design.
- **[`multiplayer-steam.md`](multiplayer-steam.md)** — nota tecnica non
  decisionale (`status: proposed`, `authority: supporting`): valuta Steamworks
  per il multiplayer asincrono già approvato in design (DEC-016, DEC-021), senza
  ridefinire alcun comportamento di gioco.
- **[`specs/`](specs/)** — le spec storiche di implementazione citate dai commenti
  del codice (es. `src/script/script_sandbox.h`, `tools/melting-gen/gen_lua.c`
  rimandano a `specs/2026-07-13-lua-sandbox-design.md`). Sono verificate contro
  il codice reale (front matter `status: implemented` quando la fase descritta è
  completa) e restano la fonte di dettaglio dietro le decisioni più sintetiche
  registrate negli ADR. Le spec risalgono al ciclo di design del 2026-07-13/14 e
  vivono qui perché il codice le cita direttamente per nome di file.
- **[`adr/`](adr/)** — decisioni architetturali datate (Architecture Decision
  Record), nel formato Contesto / Decisione / Conseguenze / Fonti. Sono il livello
  di massima autorità (`canonical`) di questo dominio: registrano scelte già
  in produzione, con le fonti che le motivano, così un futuro agente non le
  reintroduce/rimuove per errore. Oggi: [ADR-001](adr/ADR-001-backend-vulkan-only.md)
  (backend Vulkan obbligatorio, mai ROCm/flash-attention su RDNA1),
  [ADR-002](adr/ADR-002-generatori-processi-esterni.md) (generatori come processi
  esterni, mai linkati nel gioco), [ADR-003](adr/ADR-003-lua-sandbox-non-dsl.md)
  (script generati in Lua 5.5 sandboxato, niente DSL tipizzata).

## Il principio

**Qui sta lo stato tecnico REALE, verificato contro il codice; il design sta in
`docs/design/`.** Ogni affermazione tecnica in questi documenti deve essere
riscontrabile in `src/`, `tools/`, `scripts/` o nella configurazione di build
(`Makefile`, `scripts/setup-deps.sh`) al momento in cui il documento viene
scritto o rivisto — non un'intenzione, non un piano, non ciò che "dovrebbe"
essere vero. Un documento engineering che descrive un comportamento visibile al
giocatore (non solo interno al codice) resta comunque subordinato al
[Decision Log](../design/governance/decision-log.md) in caso di conflitto: la
gerarchia delle fonti (`docs/_meta/DOCUMENT-STANDARDS.md`, §3) mette gli ADR
approvati (rango 3) sotto il decision-log approvato (rango 1) e i documenti
`approved` di design (rango 2), ma sopra la documentazione engineering generica
verificata contro il codice (rango 4).

Distinzione da mantenere in ogni analisi tecnica: **DOCUMENTED AS-IS** (ciò che
un documento qui dichiara) contro **OBSERVED AS-IS** (ciò che il codice mostra
davvero, via lettura diretta o Codebase Memory). Le divergenze già accertate fra
le due cose sono registrate in [`../_meta/DOC-CODE-DRIFT.md`](../_meta/DOC-CODE-DRIFT.md);
le decisioni d'arbitrato fra fonti in conflitto sono in
[`../_meta/DOC-CONFLICTS.md`](../_meta/DOC-CONFLICTS.md) (es. `DOC-CONFLICT-038`,
l'origine di ADR-001).

## Manutenzione

Ogni documento vivo di questa cartella segue lo standard documentale comune
(`docs/_meta/DOCUMENT-STANDARDS.md`): front matter obbligatorio, `status`/
`authority` coerenti, `last_verified_commit` per i documenti `approved`/
`implemented`, `source_files` che esistono davvero nel repo. Prima di ogni
commit che tocca `docs/`:

```bash
make docs-index   # rigenera KNOWLEDGE_MANIFEST.json, docs/INDEX.md e gli INDEX di dominio
make docs-check   # verifica vincolante: exit 1 su qualunque violazione
```

`docs/engineering/INDEX.md` è **generato** (`<!-- GENERATED -->`, non editare a
mano): elenca ogni documento della cartella con stato e autorità, rigenerato da
`make docs-index` a ogni modifica.
