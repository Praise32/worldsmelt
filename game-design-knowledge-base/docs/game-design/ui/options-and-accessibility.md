---
id: gd-ui-options-accessibility
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Impostazioni e accessibilità, incluso lo schema di controllo approvato."
---

# Options and Accessibility

## Intento

Dare al giocatore controllo su comodità, accessibilità e chiarezza visiva senza alterare
in modo nascosto l'equilibrio competitivo.

## Condizioni di ingresso

Da `MainMenu` o da `PauseMenu`; al ritorno, il focus torna alla voce che ha aperto `Options`
(vedi `ui/navigation-map.md`, `ui/pause-menu.md`, `ui/main-menu.md`).

## Categorie minime

- audio;
- video;
- controlli (rimappatura di movimento libero e sparo a 4 direzioni, DEC-007);
- accessibilità;
- gameplay;
- privacy e online, se applicabile.

## Accessibilità da progettare

- rimappatura controlli;
- supporto controller e tastiera;
- riduzione flash e particelle;
- contrasto di proiettili e minacce (coerente col budget di leggibilità, fonte unica `systems/combat-and-projectiles.md`);
- dimensione testi;
- alternative ai soli colori;
- velocità o assistenze in modalità non classificata;
- descrizioni leggibili degli oggetti (Innesti compresi).

## Regola

Le opzioni che alterano la difficoltà competitiva devono essere dichiarate e gestite dalle
regole della classifica (vedi `ui/results-and-leaderboards.md`, DEC-016).

## Non-obiettivi

- Non ridefinisce lo schema di controllo di base (movimento libero, sparo a 4 direzioni): solo la sua rimappatura.

## Domande aperte residue

- Nessuna specifica; le assistenze esatte che rendono una run non classificata restano da dettagliare in `governance/open-questions.md` (sezione Multiplayer).

## Scenari verificabili

1. **Given** il giocatore apre `Options` da `PauseMenu`, **when** torna indietro, **then** il focus ritorna sull'elemento "Opzioni" di `PauseMenu`.
2. **Given** il giocatore attiva un'assistenza dichiarata come non classificante, **when** avvia una run competitiva, **then** la run viene etichettata come non classificata.
3. **Given** il giocatore aumenta il contrasto di proiettili e minacce, **when** rientra in `Gameplay`, **then** il budget di leggibilità applicato resta coerente con `systems/combat-and-projectiles.md`.
