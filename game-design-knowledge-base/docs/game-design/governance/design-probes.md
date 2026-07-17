# Design Probes

Queste domande verificano se un agente riesce a recuperare correttamente il design.

## Run iniziale

**Domanda:** Che cosa rende possibile iniziare una run prima che l'IA abbia generato tutto?

**Risposta attesa:** Il primo piano usa contenuti curati o già validati; i piani successivi vengono preparati durante il gioco. Se un piano non è pronto, esiste un fallback.

**Fonti:** `04-run-structure.md`, `systems/generated-content-validation.md`.

## Sinergia visiva

**Domanda:** Due effetti che rendono un proiettile appuntito e incendiario devono limitarsi a sommare il danno?

**Risposta attesa:** No. La sinergia deve definire comportamento e fusione visiva sul proiettile e, quando appropriato, sul personaggio, rispettando leggibilità e priorità.

**Fonti:** `systems/synergies.md`, `content/visual-language.md`.

## Casualità controllata

**Domanda:** Gli oggetti possono essere estratti senza pool, peso o limiti?

**Risposta attesa:** Non nella modalità standard. Gli oggetti appartengono a pool e possiedono rarità, peso, budget, tag, prerequisiti ed esclusioni.

**Fonte:** `systems/items-pools-and-rarity.md`.

## Classifica

**Domanda:** Una gara classificata può usare run non confrontabili senza registrare le differenze?

**Risposta attesa:** No. Le condizioni devono essere riproducibili o verificabilmente equivalenti e legate a versione e manifest.

**Fonti:** `08-multiplayer-and-competition.md`, `systems/run-manifest-and-reproducibility.md`.

## Ambiguità

**Domanda:** Se un dettaglio non è documentato, l'agente può inventarlo e aggiornare il codice?

**Risposta attesa:** No. Deve registrare l'assunzione o la domanda e non trattarla come requisito approvato.

**Fonte:** `AGENTS.md`.
