---
id: aiprod-regole-agenti-ml
title: Regole per gli agenti nei task ML
domain: ai-production
status: approved
authority: canonical
owner: ai-production
summary: >-
  Regole vincolanti per qualunque agente (Claude Code, Codex) che tocca training, dataset,
  modelli o asset generati. Fusione delle appendici ML della blueprint-v2
  (AGENTS-ML-APPENDIX, CLAUDE-ML-APPENDIX, appendice Codex).
last_reviewed: 2026-07-27
last_verified_commit: fe27f6d
topics: [agenti, ml, training, gpu, dataset, regole]
related: []
supersedes: []
source_files: [scripts/dataset_ledger.py, scripts/download-models.sh]
---

# Regole per gli agenti nei task ML

Integrano — non sostituiscono — `AGENTS.md` e `CLAUDE.md` a root. Prima di un task ML
leggere: `CLAUDE.md`, `AGENTS.md`, `docs/design/README.md`,
`docs/ai-production/00-DECISIONI-CANONICHE.md` e l'eventuale issue corrente.
Vale la scala di implementazione del progetto (CLAUDE.md): un task di training non è
completato quando il processo termina — servono validation, griglia comparativa, report e
una decisione registrata.

## Divieti assoluti

1. Non avviare un training GPU completo senza autorizzazione esplicita
   (`approved_gpu_run: true` nell'issue o richiesta diretta dell'utente).
2. Non pubblicare notebook, dataset o pesi. Non distribuire pesi col gioco.
3. Non mischiare provenienza incerta e ramo commerciale: un asset `research` non diventa
   `commercial-clean` per decisione estetica; la promozione richiede verifica di licenza.
4. Non inserire chiavi, token o cookie nel repository.
5. Non eliminare dataset, checkpoint o artifact; non rimuovere fallback.
6. Nessuna inferenza durante il combattimento; il modello di testo attivo (oggi
   Gemma-3-4B-IT Q4, DEC-140), SD e audio caricati in sequenza sul target da 6 GB.
7. Non scegliere un checkpoint solo dalla loss; non usare bypass globali dei permessi.

## Metodo sperimentale

1. Prima di ogni training: dataset validation e smoke test breve obbligatori.
2. Massimo due variabili (iperparametri) per esperimento; prompt e seed congelati.
3. Ogni esperimento produce: config, log, hash, griglia comparativa (o ascolti per
   l'audio), report — vedi `templates/EXPERIMENT-REPORT.md` e
   `11-PROTOCOLLO-ESPERIMENTI.md`.
4. Non ritentare automaticamente più di una volta.
5. Registrare ogni file del dataset nel ledger (`scripts/dataset_ledger.py`), con origine,
   licenza, hash e trasformazioni; mai dividere frame dello stesso soggetto fra train e
   validation.
6. Le licenze si verificano alla revisione corrente (upstream), non per sentito dire:
   fonte canonica [licenze.md](licenze.md).
7. Preservare atlas e fallback geometrici in ogni migrazione; eseguire le suite indicate
   da `AGENTS.md` quando si tocca il C.
8. Concludere ogni task con: diff, test eseguiti, rischi, artifact prodotti.

## Confini

- SD1.5 resta la baseline immagini finché una decisione non la sostituisce; LoRA prima dei
  checkpoint completi.
- L'audio generativo è adottato (DEC-109: Stable Audio Small, fallback rFXGen → curato;
  licenza Community accettata, DEC-113): ogni integrazione deve preservare la catena di
  fallback e caricare il modello in sequenza, mai in combattimento.
- Per l'implementazione C delegare a `melting-implementer`; ogni modifica passa dal
  giudice del gradino superiore (`melting-verifier` o su).
