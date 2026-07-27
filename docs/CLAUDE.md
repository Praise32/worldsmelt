# Worldsmelt — profilo di DOCUMENTAZIONE (docs/)

Questo profilo governa le sessioni di documentazione, design e knowledge base sotto
`docs/`. Per il lavoro sul codice vale il profilo gemello **`CLAUDE.md` a root**
(scala agenti, regole dei moduli in `AGENTS.md`). Erede del CLAUDE.md/AGENTS.md della
vecchia `game-design-knowledge-base/`.

## Prima di tutto

Quando una richiesta riguarda comportamento, interfaccia, contenuti, bilanciamento o
flussi di gioco: consultare **prima** `docs/design/README.md` (percorso curato) e il
decision log (`docs/design/governance/decision-log.md`, DEC-001..172), **poi** proporre
modifiche. Il router task→dominio è `docs/_meta/TOPIC-ROUTER.md`.

## Regole vincolanti

1. **Standard documentale**: `docs/_meta/DOCUMENT-STANDARDS.md` — albero dei domini,
   front matter obbligatorio, stati, autorità. Prima di ogni commit che tocca `docs/`:
   `make docs-index && make docs-check` (devono essere verdi).
2. **Gerarchia delle fonti** (conflitti): 1 decision-log; 2 design `approved`; 3 ADR;
   4 engineering verificato contro il codice; 5 ai-production approvato; 6 piano attivo;
   7 esperimento; 8 riferimento; 9 archivio; 10 generato. Conflitti allo stesso rango:
   aprire una domanda in `docs/design/governance/open-questions.md`, non scegliere da soli.
3. **Niente regole inventate in silenzio**: distinguere `approved` / `draft` /
   `experimental`; le open questions non sono requisiti; non modificare un documento
   `approved` per adattarlo al codice esistente.
4. **Aggiornamento obbligatorio**: ogni cambiamento al comportamento del gioco aggiorna
   nello stesso lavoro il documento di design pertinente e almeno uno scenario
   verificabile; chi verifica un doc contro il codice aggiorna `last_reviewed` e
   `last_verified_commit`.
5. **Drift doc↔codice**: per lo stato tecnico usare Codebase Memory
   (`search_graph`, `trace_path`, `get_code_snippet`) + conferma diretta nel codice.
   Distinguere sempre INTENDED / DOCUMENTED AS-IS / OBSERVED AS-IS; registrare le
   divergenze nel formato DOC-CODE-DRIFT (`docs/_meta/DOC-CODE-DRIFT.md`). Codebase
   Memory è un indice derivato: MAI fonte canonica di design, licenze, roadmap.
6. **Archivio**: un documento si archivia (mai si cancella) solo dopo che le sue
   decisioni ancora valide sono state promosse in fonte canonica; `docs/archive/` è
   escluso da indici e verifiche.
7. **Decisioni**: le decisioni di design si registrano SOLO nel decision-log (DEC-NNN
   progressive); niente registri paralleli. Le risposte alle open questions vanno
   trasferite nei documenti pertinenti e registrate come decisione.
8. **Lingua**: italiano per i contenuti; inglese per front matter e identificatori.

## Generazione IA e originalità

I contenuti generati non sono automaticamente validi: devono rispettare contratti di
design, limiti di difficoltà, tassonomie, pool, rarità, leggibilità e fallback
(`docs/design/systems/generated-content-validation.md`). Mai copiare contenuti
identificabili da giochi esistenti.

## Scala agenti per il lavoro documentale

Stessa scala del profilo root (nessuna gerarchia parallela): letture meccaniche e
inventari al gradino 1-2, sintesi e conflitti al gradino 2-3, arbitraggi trasversali a
Fable. Il giudice sta sempre un gradino sopra chi produce. Specialisti utili:
`worldsmelt-knowledge-librarian` (ricerca/indici), `worldsmelt-decision-facilitator`
(batch di domande decisionali e registrazione decisioni).
