---
id: gd-ui-results-leaderboards
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Vittoria chiusa al boss del piano 5 (DEC-031); alla sconfitta restano i punti sblocco maturati in misura ridotta (DEC-041); riprova non classificata. Schermata risultati completa con timeline, scoperte e confronto con le run passate (DEC-056); classifiche divise per metrica, tempo e punteggio separati (DEC-062). Se la run appartiene alla Classificata giornaliera, i risultati mostrano la medaglia/cornice cosmetica guadagnata, fuori dall'economia dei punti (DEC-064)."
---

# Results and Leaderboards (RunResults)

## Intento

Comunicare l'esito della run in modo chiaro — vittoria al boss del piano 5 o sconfitta — e
dare accesso alle classifiche coerenti con DEC-016.

## Condizioni di ingresso

Da `Gameplay`, quando la run termina: vittoria sul boss del piano 5, morte in un piano
qualunque (permadeath), o abbandono confermato da `PauseMenu`.

## Vittoria e sconfitta (DEC-006, aggiornata da DEC-031)

- Sconfiggere il boss del piano 5 chiude la run con **vittoria**, valida per le classifiche.
  La run finisce lì: la prosecuzione in piani extra non è implementata in questa fase del
  gioco e resta un'idea futura parcheggiata (DEC-018, DEC-031), non un'opzione presentata in
  `RunResults`.
- La salute a zero, in qualunque piano, è run persa (permadeath): non esiste un "continua"
  dopo la morte.

## Alla sconfitta (DEC-041)

Alla sconfitta restano i punti sblocco maturati durante la run, ma in **misura ridotta**
rispetto alla vittoria; il catalogo si aggiorna comunque con le creazioni incontrate e con le
statistiche della run. Nessun oggetto sopravvive alla run in nessun caso (permadeath,
DEC-006).

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Esito | Sempre | — (sola lettura) | Nessuna | — | Vittoria (boss piano 5), sconfitta, o abbandono |
| Tempo e piano raggiunto | Sempre | — (sola lettura) | Nessuna | — | — |
| Build finale e sinergie notevoli | Sempre | — (sola lettura) | Consulta dettagli | Apre una vista sola-lettura della build | — |
| Timeline della run per piano (DEC-056) | Sempre | — (sola lettura) | Consulta la timeline | Mostra, piano per piano, oggetti presi, fusioni, boss affrontati e tempi parziali | Ordine cronologico, raggruppato per piano |
| Nuove scoperte entrate nel catalogo (DEC-056) | Sempre | — (sola lettura) | Consulta le scoperte | Elenco dei contenuti generati incontrati per la prima volta in questa run | I candidati al museo sono evidenziati distintamente dal resto |
| Punteggio | Sempre | — (sola lettura) | Nessuna | — | Punteggio composito, vedi [Rewards and Economy](../systems/rewards-and-economy.md), DEC-060 (rimando) |
| Riepilogo punti (DEC-056) | Sempre | — (sola lettura) | Segui la scorciatoia | Mostra punti base + bonus da prove (DEC-027) e apre direttamente la spesa nel Catalogo | Scorciatoia diretta verso la spesa punti |
| Punti sblocco maturati | Sempre | — (sola lettura) | Nessuna | — | Valore intero alla vittoria, ridotto alla sconfitta (DEC-041) |
| Medaglia/cornice della Daily (DEC-064) | La run appartiene alla Classificata giornaliera pubblica (Daily) | — (sola lettura) | Nessuna | — | Mostra la medaglia/cornice cosmetica guadagnata dal piazzamento o dalla streak di partecipazione, se applicabile; nessun punto sblocco aggiuntivo |
| Confronto con le run passate (DEC-056) | Sempre | — (sola lettura) | Consulta il confronto | Mostra miglior tempo, medie e record personali per tema e personaggio | — |
| Identificatore condivisibile della run | Sempre | Sempre | Copia/condividi identificatore | Permette ad altri di scegliere la stessa run/seed | Conferma di copia |
| Nuova run | Sempre | Sempre | Apre `RunSetup` | Entra in `RunSetup` senza vincoli dalla run appena conclusa | — |
| Riprova la stessa run | Sempre | Sempre | Apre `RunSetup` con lo stesso seed/manifest precompilato | Riavvia la stessa run, ma **non classificata** | Etichetta esplicita "non classificata" |
| Torna al menu | Sempre | Sempre | Chiude `RunResults` | Entra in `MainMenu` | — |

## Decisione approvata: riprova non classificata

"Riprova la stessa run" è permesso ma la nuova esecuzione non è classificata: rigiocare
lo stesso seed non deve poter essere usato per manipolare una classifica basata su tempo o
punteggio (approved).

## Schermata risultati completa (DEC-056)

Oltre a tempo e punteggio, `RunResults` mostra sempre:

1. la **timeline della run per piano**: oggetti presi, fusioni, boss affrontati, tempi
   parziali, in ordine cronologico e raggruppati per piano;
2. le **nuove scoperte** entrate nel catalogo durante questa run, con i **candidati al
   museo** evidenziati distintamente dal resto (fonte del museo:
   [Floor Zero](../systems/floor-zero.md), DEC-040, rimando);
3. il **riepilogo punti** (punti base + bonus da prove, DEC-027) con una **scorciatoia
   diretta** alla spesa nel Catalogo;
4. il **confronto con le run passate**: miglior tempo, medie e record personali per tema
   e personaggio.

Questi quattro elementi sono sempre presenti nella schermata risultati, a vittoria come a
sconfitta (fatta salva la riduzione dei punti sblocco alla sconfitta, DEC-041).

## Classifica

Ogni voce di classifica è legata a modalità, versione delle regole e run/stagione (DEC-016).
Le classifiche sono **divise per metrica** (DEC-062): una graduatoria per il tempo e una
per il punteggio, **non** un punteggio combinato; la metrica punteggio usa il punteggio
composito descritto in [Rewards and Economy](../systems/rewards-and-economy.md) (DEC-060,
rimando). I pool sbloccati dalla meta-progressione sono esclusi dal calcolo competitivo.
La modalità Classificata esiste in tre istanze — stesso seed, seed diversi, e la
Classificata giornaliera pubblica ("Daily") — descritte come fonte unica in
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md) (DEC-062,
rimando, non riformulate qui).

## Ricompense cosmetiche della Daily (DEC-064)

Quando la run conclusa appartiene alla Classificata giornaliera pubblica ("Daily", DEC-062),
`RunResults` mostra anche la medaglia o cornice cosmetica guadagnata dal piazzamento e/o dalla
streak di partecipazione (DEC-064): fonte unica della Daily e delle sue regole è
[08-multiplayer-and-competition.md](../08-multiplayer-and-competition.md#ricompense-della-daily-cosmetici-dec-064)
(rimando, non riformulato qui). Queste ricompense sono puramente cosmetiche, persistono nel
profilo e nel museo del Piano 0 (vedi
[Save and Meta Progression](../systems/save-and-meta-progression.md#ricompense-cosmetiche-della-classificata-giornaliera-dec-064));
**non** assegnano punti sblocco e non toccano l'economia dei punti (DEC-015, DEC-027).

## Fallback rilevanti

Se un fallback ha sostituito contenuti durante la run, e ciò incide sulla validità
competitiva, va segnalato qui in modo sintetico, senza dettagli tecnici. Fonte unica delle
regole di fallback: `systems/generated-content-validation.md`.

## Non-obiettivi

- Non ridefinisce le condizioni di vittoria: rimanda a `04-run-structure.md` e DEC-006.
- Non gestisce la scelta del prossimo tema o personaggio: quella avviene nel Piano 0 della run successiva.

## Domande aperte residue

- Metriche di classifica oltre a tempo e punteggio restano `experimental` (vedi `governance/open-questions.md`).

## Scenari verificabili

1. **Given** il giocatore sconfigge il boss del piano 5, **when** entra in `RunResults`, **then** l'esito mostra "vittoria" e la run risulta conclusa lì, senza alcuna opzione di prosecuzione (DEC-031).
2. **Given** il giocatore muore in un piano qualunque, **when** entra in `RunResults`, **then** l'esito mostra sconfitta e nessuna opzione di "continua" è disponibile (permadeath).
3. **Given** il giocatore sceglie "Riprova la stessa run", **when** la nuova esecuzione parte, **then** è etichettata come non classificata fin dall'inizio.
4. **Given** il giocatore muore (permadeath) prima del boss del piano 5, **when** entra in `RunResults`, **then** i punti sblocco maturati sono mostrati in misura ridotta rispetto a quanto avrebbe ottenuto con una vittoria, e il catalogo risulta comunque aggiornato con le creazioni incontrate (DEC-041).
5. **Given** il giocatore conclude una run, **when** entra in `RunResults`, **then** vede la timeline della run per piano, le nuove scoperte con i candidati al museo evidenziati, il riepilogo punti con la scorciatoia al Catalogo e il confronto con le run passate, oltre a tempo e punteggio (DEC-056).
6. **Given** una gara in modalità Classificata è conclusa, **when** il giocatore consulta la classifica, **then** trova una graduatoria per il tempo e una separata per il punteggio, mai un punteggio combinato (DEC-062).
7. **Given** il giocatore conclude una run della Classificata giornaliera (Daily) con un piazzamento che merita una medaglia, **when** entra in `RunResults`, **then** vede la medaglia/cornice cosmetica guadagnata, senza alcun punto sblocco aggiuntivo (DEC-064).
