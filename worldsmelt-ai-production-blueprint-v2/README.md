# Worldsmelt — AI Asset & Training Blueprint

Pacchetto di decisioni tecniche e operative ricavato da:

- conversazione di progettazione su Stable Diffusion, LoRA, Kaggle e animazioni;
- lettura di `docs/`, `roguelike-ai-appunti/` e della knowledge base del progetto;
- ispezione dei moduli correnti `tools/melting-sprites`, `src/assets`, `src/render`, `src/gen`;
- verifica delle fonti ufficiali disponibili al 20 luglio 2026.

## Obiettivo

Costruire una pipeline locale e vendibile che:

1. usa **Stable Diffusion 1.5** come base leggera;
2. addestra **LoRA specialistiche**, non checkpoint completi;
3. usa **Kaggle Notebook** per il training;
4. consente a **Codex CLI o Claude Code** di preparare, avviare e analizzare gli esperimenti tramite Kaggle MCP/CLI;
5. genera asset pixel-art durante il Piano 0 o in sviluppo;
6. anima gli asset con **raylib e rig deterministici**, senza generazione durante il combattimento;
7. mantiene modalità solo-curato, cache, validazione e fallback;
8. separa chiaramente esperimenti di ricerca da asset/modelli commercialmente puliti.

## Ordine di lettura

1. `00-DECISIONI-CANONICHE.md`
2. `01-AUDIT-DEL-PROGETTO.md`
3. `02-STACK-MODELLI.md`
4. `03-PIANO-LORA.md`
5. `04-DATASET-LICENZE.md`
6. `05-KAGGLE-TRAINING-RUNBOOK.md`
7. `06-AGENTI-KAGGLE-MCP.md`
8. `07-ARCHITETTURA-RUNTIME.md`
9. `08-PIPELINE-SPRITE-ANIMAZIONI.md`
10. `09-NEMICI-BODY-PLAN-RIG.md`
11. `10-PIANO-INTEGRAZIONE-C.md`
12. `11-PROTOCOLLO-ESPERIMENTI.md`
13. `12-ROADMAP.md`
14. `13-PROMPT-AGENTI.md`
15. `14-FONTI.md`

La cartella `templates/` contiene documenti riutilizzabili. La cartella
`agent-config/` contiene regole da copiare o adattare per Codex/Claude.

## Principio architetturale

> L'IA propone aspetto e contenuto; il motore decide struttura, sicurezza, animazione,
> collisioni, budget e fallback.

Il progetto non deve dipendere dalla capacità di Stable Diffusion di creare uno
spritesheet completo e temporalmente coerente per ogni mostro immaginabile. La varietà
visiva può essere molto grande, mentre il numero di rig e famiglie corporee resta finito,
testabile e deterministico.
## Estensioni v2

Questa versione aggiunge:

- pipeline UI/GUI con Penpot, token, 9-slice e renderer raylib;
- pipeline audio ibrida con rFXGen e proposta Stable Audio 3 Small;
- sistema candidate → curated → fallback;
- Piano 0 costruito anche dalle migliori generazioni storiche;
- INDEX e topic router;
- coda di domande decisionali;
- session protocol;
- path orchestrator;
- agenti Claude Code specializzati;
- appendix Codex;
- proposte di aggiornamento della knowledge base;
- template per decisioni, domande, UI, audio e review.
