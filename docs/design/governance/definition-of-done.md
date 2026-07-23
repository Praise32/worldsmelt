---
id: design-definition-of-done
title: Definition of Done — Comportamento di Gioco
domain: design
status: approved
authority: canonical
owner: design
summary: >-
  Checklist dei criteri che rendono completa una modifica al comportamento di gioco: coerenza col design, test, vocabolario canonico, fallback e documentazione aggiornata.
last_reviewed: 2026-07-19
last_verified_commit: 0ec60d0
topics: [definition-of-done, governance, processo, qualità, vocabolario canonico]
related: []
supersedes: []
source_files: []
---

# Definition of Done — Comportamento di Gioco

Una modifica è completa quando:

- il comportamento implementato corrisponde al documento approvato;
- gli scenari verificabili sono coperti da test o verifica esplicita;
- menu, focus, annullamento ed errori sono gestiti;
- il contenuto generato dispone di validazione e fallback;
- la leggibilità visiva è stata controllata;
- il documento di design è aggiornato;
- le decisioni nuove sono registrate;
- non sono stati introdotti riferimenti copiati da altre proprietà;
- il vocabolario canonico di `governance/glossary.md` è rispettato (nomi degli stati, "Innesto" non "trinket", "incompatibilità" non "esclusioni", nessun termine vietato);
- la modalità classificata registra versione e manifest, quando coinvolta.
