---
id: aiprod-decisioni-canoniche
title: Decisioni canoniche
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Sintesi delle decisioni tecniche proposte su modelli, LoRA, training, dataset, generazione in-game, animazione, agenti e licenze; stato 'proposta consolidata'.
last_reviewed: 2026-07-23
topics: [decisioni, stable-diffusion, lora, kaggle, dataset, licenze, animazione]
related: []
supersedes: []
source_files: []
---
# Decisioni canoniche

Stato: proposta consolidata da adottare come baseline tecnica.

## Modelli

- Base immagini: **Stable Diffusion 1.5 vanilla**.
- Non addestrare un checkpoint completo nella prima fase.
- Addestrare LoRA separate per stile e ruolo.
- Modello testuale locale: **Gemma-3-4B-IT Q4_K_M** dal 23/07 (DEC-140, scelto con la suite di comparison; fallback su errore di caricamento: Qwen2.5-Coder 1.5B; il 7B resta selezionabile con `--model`).
- Qwen e Stable Diffusion restano processi separati e caricati in sequenza.
- LCM-LoRA è un acceleratore di inferenza, non la LoRA di stile.
- `stable-diffusion.cpp` resta il backend runtime locale preferito perché il progetto ha
  già misure reali su Vulkan/RX 5600 XT.

## LoRA previste

1. `worldsmelt-style`
2. `worldsmelt-enemies`
3. `worldsmelt-items`
4. `worldsmelt-environments`
5. `worldsmelt-vfx`
6. LoRA di identità per specifici personaggi, solo quando necessarie

Non unire subito tutto in una LoRA generale. La Style LoRA deve imparare il linguaggio
grafico; le LoRA di ruolo devono imparare categorie e soggetti.

## Training

- Training su Kaggle Notebook.
- Primo run: 150–300 immagini curate e coerenti.
- Training iniziale UNet-only.
- Text encoder congelato.
- Rank iniziale 8.
- Confronto con seed e prompt congelati.
- Smoke test breve obbligatorio prima del run completo.
- Ogni esperimento produce LoRA, config, log, griglia e report.

## Dataset

- Separare `research-unknown-provenance` e `commercial-clean`.
- Il dataset Kaggle da circa 89.000 immagini può essere usato per imparare e fare ricerca,
  ma non è una base commercialmente prudente: la provenienza è descritta soltanto come
  immagini raccolte da un non identificato gioco online.
- Per il ramo commerciale usare asset propri, commissionati con cessione chiara, CC0 o
  altra licenza verificata.
- Conservare un ledger con origine, licenza, hash e trasformazioni.
- Mai dividere frame dello stesso personaggio/animazione fra train e validation.

## Generazione nel gioco

- Nessuna inferenza durante il combattimento.
- Generazione nel Piano 0 o fra piani.
- Cache per hash di modello, LoRA, prompt, seed e versione pipeline.
- Pubblicazione atomica degli asset.
- Modalità solo-curato permanente e dignitosa.
- Asset generati non validi vengono sostituiti da fallback curati o geometrici.

## Animazione

- Umanoidi: key frame controllati, tre direzioni generate e sinistra specchiata.
- Creature: immagine o componenti generati + rig procedurale.
- Non richiedere un dataset infinito di spritesheet.
- Definire un catalogo finito di body plan.
- Collisioni, pivot, socket e telegraph sono dati del motore, non decisioni libere del
  modello.
- AnimateDiff/video sono strumenti di riferimento, non la sorgente finale degli sprite.

## Agenti

- Codex CLI o Claude Code lavorano sul repository.
- Kaggle viene usato come esecutore GPU remoto.
- Collegamento preferito: Kaggle MCP ufficiale.
- Fallback: Kaggle CLI ufficiale.
- L'agente non deve avviare run GPU completi senza una policy esplicita.
- L'agente dentro il Notebook è ammesso per debugging, non come architettura principale.

## Licenze

- SD1.5 usa CreativeML OpenRAIL-M: è un modello open-weight con restrizioni, non una
  licenza permissiva OSI come MIT/Apache.
- Qwen2.5-Coder-7B-Instruct dichiara Apache 2.0.
- Pixel Art Fixer open source dichiara MIT.
- Il ramo commerciale deve conservare NOTICE, licenze, hash e provenienza.
- Distribuire soltanto PNG/JSON è più semplice che distribuire i pesi, ma non sostituisce
  la verifica dei diritti sui dati e sugli output.
