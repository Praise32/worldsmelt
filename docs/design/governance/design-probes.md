---
id: design-probes-qa
title: Design Probes
domain: design
status: approved
authority: supporting
owner: design
summary: >-
  Otto domande-risposta usate per verificare che un agente recuperi correttamente il design (Piano 0, fusione, personaggi, meta-punti, sinergia visiva, casualità, classifica, ambiguità).
last_reviewed: 2026-07-19
last_verified_commit: 0ec60d0
topics: [design-probes, QA, recupero design, governance, agenti]
related: []
supersedes: []
source_files: []
---

# Design Probes

Queste domande verificano se un agente riesce a recuperare correttamente il design.

## Piano 0

**Domanda:** Perché il gioco è sempre avviabile anche se l'IA non ha ancora generato tutti i contenuti?

**Risposta attesa:** Il Piano 0 è un hub giocabile (rifugio più arene di sfida opzionali) che fa da spazio di attesa mentre l'IA genera i piani successivi; finché non esistono asset dedicati, una versione statica curata del Piano 0 fa da sala d'attesa. L'uscita verso il piano 1 si apre solo quando il piano 1 è pronto. Lo stato di generazione è un indicatore dentro il Piano 0, non una schermata separata.

**Fonti:** `04-run-structure.md`, `systems/floor-zero.md`, `ui/generation-status.md`, `ui/navigation-map.md`.

## Fusione esplicita

**Domanda:** Due Innesti compatibili nella build producono automaticamente una fusione?

**Risposta attesa:** No. La sinergia implicita è automatica quando due componenti compatibili convivono nella build. La fusione esplicita è un'azione volontaria: richiede la stanza di fusione, il consumo di due oggetti e un catalizzatore di fusione. Il risultato è un oggetto nuovo generato dall'IA che eredita comportamento e presentazione da entrambi; ogni fusione deve cambiare sia il comportamento sia la presentazione visiva.

**Fonti:** `systems/synergies.md`, `systems/item-fusion.md`, `systems/special-rooms.md`, `ui/inventory-and-synergy-screen.md`.

## Personaggi

**Domanda:** Il giocatore può costruire liberamente le statistiche del proprio personaggio prima della run?

**Risposta attesa:** No. Esiste un personaggio base sempre disponibile. Per ogni run l'IA genera un personaggio alternativo con un trait unico e statistiche casuali entro bande garantite: il giocatore lo prende o lo lascia. La scelta avviene nel Piano 0, non in Run Setup.

**Fonte:** `systems/floor-zero.md`.

## Meta-punti

**Domanda:** Vincere run in singleplayer garantisce potenziamenti permanenti al personaggio?

**Risposta attesa:** No. Persistono il catalogo di tutti i contenuti generati, il museo del Piano 0 (i migliori) e punti guadagnati in singleplayer, spendibili per sbloccare contenuti generati nei pool delle run future. Non esistono potenziamenti permanenti del personaggio. Gli sblocchi sono disattivati nelle modalità competitive.

**Fonte:** `systems/save-and-meta-progression.md`.

## Sinergia visiva

**Domanda:** Due effetti che rendono un proiettile appuntito e incendiario devono limitarsi a sommare il danno?

**Risposta attesa:** No. La sinergia deve definire comportamento e fusione visiva sul proiettile e, quando appropriato, sul personaggio, rispettando leggibilità e priorità.

**Fonti:** `systems/synergies.md`, `content/visual-language.md`.

## Casualità controllata

**Domanda:** Gli oggetti possono essere estratti senza pool, peso o limiti?

**Risposta attesa:** No, non nella modalità standard. Gli oggetti appartengono a pool e possiedono rarità, peso, budget, tag, prerequisiti e incompatibilità dichiarate.

**Fonte:** `systems/items-pools-and-rarity.md`.

## Classifica

**Domanda:** Una gara classificata può usare run non confrontabili senza registrare le differenze?

**Risposta attesa:** No. Il multiplayer è asincrono sulla stessa run (stesso seed/manifest); le condizioni devono essere riproducibili o verificabilmente equivalenti e legate a versione e manifest; i pool sbloccati sono esclusi dalla competizione.

**Fonti:** `08-multiplayer-and-competition.md`, `systems/run-manifest-and-reproducibility.md`.

## Ambiguità

**Domanda:** Se un dettaglio non è documentato, l'agente può inventarlo e aggiornare il codice?

**Risposta attesa:** No. Deve registrare l'assunzione o la domanda in `governance/open-questions.md` e non trattarla come requisito approvato.

**Fonte:** `AGENTS.md`.
