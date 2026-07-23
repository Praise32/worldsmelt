<!-- GENERATED: make docs-index -- non modificare a mano -->

# Indice `docs/ai-production/`

- [Decisioni canoniche](00-DECISIONI-CANONICHE.md) — Sintesi delle decisioni tecniche proposte su modelli, LoRA, training, dataset, generazione in-game, animazione, agenti e licenze; stato 'proposta consolidata'. `[proposed/supporting]`
- [Stack dei modelli](02-STACK-MODELLI.md) — Motiva la scelta di SD1.5 come base immagini, il ruolo di LCM-LoRA e TAESD, i compiti/limiti di Qwen2.5-Coder e i criteri per valutare modelli alternativi. `[proposed/supporting]`
- [Piano LoRA](03-PIANO-LORA.md) — Gerarchia delle LoRA da addestrare (style, enemies, items, environments, vfx, identità), caption, configurazione baseline e criteri per un eventuale checkpoint completo. `[proposed/supporting]`
- [Dataset e licenze](04-DATASET-LICENZE.md) — Strategia prudente su provenienza del dataset Kaggle 89k immagini, separazione research/commercial, ledger minimo e obblighi di licenza di SD1.5/Qwen/Pixel Art Fixer. `[proposed/supporting]`
- [Kaggle Training Runbook — SD1.5 LoRA](05-KAGGLE-TRAINING-RUNBOOK.md) — Runbook operativo per il training LoRA su Kaggle Notebook: struttura ml/, preflight, smoke test, config v0, output obbligatori e comando di riferimento. `[proposed/supporting]`
- [Codex/Claude + Kaggle MCP](06-AGENTI-KAGGLE-MCP.md) — Architettura e configurazione per collegare Codex CLI/Claude Code a Kaggle via MCP o CLI fallback, policy di autorizzazione GPU e flusso settimanale. `[proposed/supporting]`
- [Architettura runtime della generazione](07-ARCHITETTURA-RUNTIME.md) — Definisce i 4 tier di generazione (solo curato -> grafica completa), scheduling Qwen/SD, chiave di cache, pubblicazione atomica e failure policy. `[proposed/supporting]`
- [Pipeline sprite e animazioni](08-PIPELINE-SPRITE-ANIMAZIONI.md) — Pipeline dalla EnemySpec allo SpriteBundle animato: guide/ControlNet, Pixel Art Fixer, validazione automatica, formato eventi/animazioni e struct raylib. `[proposed/supporting]`
- [Nemici: body plan e rig](09-NEMICI-BODY-PLAN-RIG.md) — Catalogo di 11 body plan e 10 ruoli meccanici per i nemici, formato EnemySpec, regole di validazione e vincoli di roster per run. `[proposed/supporting]`
- [Piano di integrazione nel codice C](10-PIANO-INTEGRAZIONE-C.md) — Piano in 8 fasi (A-H) per portare multi-LoRA, SpriteBundle, registry dei rig, animator ed EnemySpec esteso nel motore C, con migrazione graduale dell'atlas. `[proposed/supporting]`
- [Protocollo degli esperimenti](11-PROTOCOLLO-ESPERIMENTI.md) — Metodologia per esperimenti LoRA: una domanda per run, massimo due variabili, baseline obbligatoria, metriche automatiche, review umana e regola di promozione. `[proposed/supporting]`
- [Roadmap operativa](12-ROADMAP.md) — Roadmap in 9 milestone (repository ML -> Style LoRA -> multi-LoRA -> SpriteBundle -> body plan -> Enemy LoRA -> Piano 0 -> umanoide -> produttizzazione) con gate e ordine da non invertire. `[proposed/supporting]`
- [Prompt per agenti](13-PROMPT-AGENTI.md) — Cinque prompt di esempio per sessioni agente: preparazione esperimento, analisi output Kaggle, multi-LoRA, primo SpriteBundle blob, review licenze. `[proposed/supporting]`
- [Fonti verificate](14-FONTI.md) — Elenco di link esterni verificati il 20/07/2026 (SD1.5, Qwen, Kaggle, agenti, pixel art, audio) piu' riferimenti a documenti interni del repo. `[draft/supporting]`
- [Pipeline UI e GUI](15-UI-DESIGN-PIPELINE.md) — Proposta di pipeline UI: Penpot come sorgente design, token a 3 livelli, componenti minimi, 9-slice, moduli src/ui/, raygui solo per tool interni. `[approved/supporting]`
- [Pipeline audio](16-AUDIO-GENERATION-PIPELINE.md) — Pipeline ibrida rFXGen + Stable Audio 3 Small per SFX/musica, in esplicito conflitto con DEC-036 che considera l'audio generativo futuro. `[approved/supporting]`
- [Curation degli asset e Piano 0](17-ASSET-CURATION-AND-FLOOR-ZERO.md) — Pipeline di curation (candidate->curated->fallback) con stati, directory art/, manifest e proposta di Piano 0 ibrido curato+generato. `[approved/supporting]`
- [Orchestrazione degli agenti](18-AGENT-ORCHESTRATION.md) — Path Orchestrator che classifica i task in 7 percorsi (design/technical/ML/UI/audio/curation/implementation) e regola quando porre domande bloccanti. `[approved/supporting]`
- [Questionario decisionale](19-DECISION-QUESTIONNAIRE.md) — Coda di 24 domande (audio, UI, immagini, animazioni, Piano 0, agenti, distribuzione, budget) con priorita' BLOCKING/SOON/LATER da risolvere prima di implementare. `[proposed/supporting]`
- [Protocollo delle sessioni](20-SESSION-PROTOCOL.md) — Definisce 6 tipi di sessione (decision, planning, implementation, ML experiment, curation, release audit), gate di ingresso e formato di handoff. `[proposed/supporting]`
- [AI production — modelli, LoRA, dataset e asset generati](README.md) — Punto d'ingresso del dominio ai-production: come si selezionano, allenano e integrano modelli, LoRA, dataset e asset generati. Erede dei contenuti canonici della worldsmelt-ai-production-blueprint-v2. `[approved/canonical]`
- [Licenze dello stack](licenze.md) — Analisi non legale delle licenze di codice (raylib, llama.cpp, stable-diffusion.cpp, Lua, cJSON) e modelli (Qwen, pixel model OpenRAIL-M, LCM-LoRA, TAESD, Stable Audio Small con Stability Community License, DEC-113). `[approved/canonical]`
- [Regole per gli agenti nei task ML](regole-agenti-ml.md) — Regole vincolanti per qualunque agente (Claude Code, Codex) che tocca training, dataset, modelli o asset generati. Fusione delle appendici ML della blueprint-v2 (AGENTS-ML-APPENDIX, CLAUDE-ML-APPENDIX, appendice Codex). `[approved/canonical]`

## dataset/

- [Dataset — regole d'oro e registro di provenienza](dataset/README.md) — Regole del dataset per le LoRA (solo CC0 verificate, asset propri o commissioni con cessione chiara), fonti candidate e registro di provenienza ledger.jsonl gestito da scripts/dataset_ledger.py. `[approved/canonical]`
- [Runbook: prima campagna Style LoRA su RunPod](dataset/TRAINING-RUNBOOK.md) — Runbook passo-passo per addestrare la prima Style LoRA su RunPod: prerequisiti, scelta pod, dataset, comandi kohya_ss, sweep iperparametri, valutazione cieca. `[approved/supporting]`

## experiments/

- [Primo benchmark audio — Stable Audio 3 Small su CPU (23/07/2026)](experiments/audio-benchmark-2026-07-23.md) — Prima esecuzione reale di DEC-109: Stable Audio 3 Small (sfx+music) su CPU — SFX 4s in ~7.7s, musica 20s in ~13.6s (0.68x realtime), 5 GB RAM; 20 clip salvate per revisione umana in logs/model-comparison/audio-20260723-172702/. Ricetta ambiente in scripts/. `[implemented/supporting]`
- [Comparison dei modelli di testo — 23/07/2026](experiments/model-comparison-testo-2026-07-23.md) — Suite di comparison su 11 modelli GGUF (3 seed fissi): gemma-3-4b-it Q4 migliore complessivo (84.9), Coder 1.5B Q4 miglior rapporto e minimo accettabile; base di DEC-140. Artefatti e valori grezzi in logs/model-comparison/20260723-125614/. `[implemented/supporting]`
- [Spike: sprite generati in locale con Stable Diffusion](experiments/sprites-spike.md) — Spike del 13/07 su generazione locale sprite con stable-diffusion.cpp su RX 5600 XT: misure di performance e tecnica di chroma-key con flood fill. `[implemented/supporting]`

## templates/

- [Asset Review](templates/ASSET-REVIEW.md) — Modulo di revisione asset: metadata, checklist tecnica, punteggi in-engine (leggibilità, coerenza, ruolo, originalità), decisione finale con motivazione. `[draft/supporting]`
- [AudioSpec Template](templates/AUDIO-SPEC.md) — Schema JSON per AudioSpec: musica, famiglie di suoni, generation_policy (curated/rfxgen/stable-audio/hybrid), fallback_pack, license_branch. `[draft/supporting]`
- [Dataset Ledger Specification](templates/DATASET-LEDGER.md) — Formato JSONL per il ledger dei dataset: provenance, licenza, split, caption, hash originale/processato; regole su provenance_status e split_group. `[draft/supporting]`
- [DEC-AI-XXX — Titolo](templates/DECISION-RECORD.md) — Frontmatter e struttura per registrare decisioni AI-production (DEC-AI-XXX): contesto, decisione, alternative, motivazione, conseguenze, migrazione, test. `[draft/supporting]`
- [EnemySpec Template](templates/ENEMY-SPEC.md) — Schema JSON tecnico per nemici: origin, role, body_plan, rig, stats_band, attacco/telegraph, asset richiesti, prompt di generazione, checklist di validazione. `[draft/supporting]`
- [Experiment Report](templates/EXPERIMENT-REPORT.md) — Report esperimento ML completo: metadata, domanda unica, variabili, preflight, tabella metriche baseline/candidate, review umana, decisione, artifact. `[draft/supporting]`
- [Decision Session — Question Batch](templates/QUESTION-BATCH.md) — Modulo per sessioni decisionali: fino a 7 domande con priorità BLOCKING/SOON/LATER, opzioni, raccomandazione, default temporaneo, sezione di chiusura. `[draft/supporting]`
- [SpriteBundle Template](templates/SPRITE-BUNDLE.md) — Schema JSON per bundle sprite: rig, canvas/pivot/hitbox, texture, sockets, animazioni, metadata di generazione (seed, steps, cfg, LoRA), validazione e fallback. `[draft/supporting]`
- [UISkinSpec Template](templates/UI-SKIN-SPEC.md) — Schema JSON per skin UI: risoluzione logica 640x360, integer scale, token, componenti (panel/button/card/slot/tooltip/focus), input supportati, fallback_skin. `[draft/supporting]`
- [Weekly ML/Generation Issue](templates/WEEKLY-ISSUE.md) — Template per issue settimanale ML/generazione: obiettivo, scope, baseline, variabili ammesse, checklist, autorizzazione GPU, criteri di accettazione. `[draft/supporting]`

_1 documenti historical esclusi dall'indice._
