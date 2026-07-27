# HANDOFF

Stato sintetico del lavoro. La cronologia completa delle sessioni passate è in
`docs/archive/handoffs/` (integrale: `docs/archive/handoffs/HANDOFF-2026-07-19.md`).

## Stato al 2026-07-27

- **Branch**: `main` (tutto committato e pushato; policy: ogni cambiamento verificato va
  subito su main).
- **Ultimo lavoro**: **audit documentale completo**. Batch DEC-144..DEC-169 registrato nel
  decision-log (169 decisioni totali: 168 `approved`, 1 `superseded` — DEC-003) e propagato
  nella KB di design/engineering/ai-production nello stesso lavoro. Punti salienti: coda
  unica delle domande aperte (DEC-147, code parallele chiuse e archiviate, 22 voci in
  `docs/design/governance/open-questions.md`, l'ultima aperta da DEC-169 sul comando che apre
  il menu di pausa dal Piano 0); `docs/_meta/DOC-CONFLICTS.md` e
  `docs/_meta/DOC-CODE-DRIFT.md`
  diventano registri vivi con campo Stato per voce e DEC/commit di chiusura (DEC-150);
  DEC-019 promossa da `draft` ad `approved` (DEC-154, nessuna decisione in stato draft nel
  registro); pipeline immagini confermata su SD1.5 con Style LoRA su base vanilla e dataset
  definitivi affidati al proprietario (DEC-148); mechanics-lab sbloccato, gate di DEC-138
  soddisfatto (DEC-165); comparison modelli chiusa il 23/07 e spostata in
  `docs/plans/completed/model-comparison.md` (DEC-157).
- **Test**: invariati rispetto alla sessione precedente. `make test-script`, `test-gen`,
  `test-sprites` verdi. `make test` resta **rosso su `--states-test` quando `catalog/`
  contiene run locali** (difetto preesistente, `docs/engineering/known-issues.md` #1); verde
  a catalog vuoto. `test-llm` flaky noto ~25% col modello 1.5B (known-issues.md #2).
- **WIP / blocchi**: nessuno sul codice. Nessuna decisione in stato `draft` nel decision-log.
- **Prossimo passo naturale**: **backlog implementativo** aperto dai batch DEC-141..DEC-143
  (2026-07-25) e DEC-144..DEC-169 (design
  deciso, codice non ancora scritto):
  - RNG di gameplay derivato dal seed di run, prerequisito bloccante di DEC-141 per la
    Classificata a stesso seed (`docs/engineering/known-issues.md` #3);
  - floor minimo di almeno un oggetto per rarità nel pool curato (DEC-144);
  - correzione di fortuna estesa esplicitamente a tutti i pool, boss e negozio inclusi
    (DEC-145), e proxy di leggibilità visiva sulla percentuale di schermo coperta (DEC-146);
  - `RunResults` dichiara la causa della sconfitta (DEC-159);
  - budget di potenza dedicato per il risultato di sinergie e fusioni (DEC-162);
  - card di scoperta scartate a morte o cambio stanza (DEC-152); Innesto sganciato che resta
    a terra, recuperabile nella stanza (DEC-160); HUD nascosto nel Piano 0, consultabile in
    pausa e visibile nelle prove (DEC-169); valuta da qualunque stanza completata secondo la
    propria condizione (DEC-167);
  - in parallelo, fuori dal motore C: **training della Style LoRA su Kaggle** (DEC-168,
    runbook RunPod resta fallback a pagamento) **coi dataset definitivi del proprietario**
    (DEC-148 — i dataset attuali, incluso il Kaggle ~89k immagini, non sono definitivi).

## Orientarsi

- **Avvio**: `make run-demo` (demo curata senza modelli: gira in `build/demo` con `generated/` vuota, così nessun artefatto di una vecchia generazione entra nella demo); `make run` (usa `generated/` della repo: ultimo manifest o fallback); `make run-gen` / `make run-gen-fast` (pipeline completa con generazione); vedi README.md per i dettagli.
- Implementazione: `CLAUDE.md` (scala agenti) + `AGENTS.md` (regole moduli).
- Documentazione/design: `docs/CLAUDE.md` + `docs/design/README.md`.
- Indice generale: `docs/INDEX.md` (rigenera con `make docs-index`).
