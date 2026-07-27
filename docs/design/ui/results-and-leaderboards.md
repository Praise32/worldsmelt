---
id: gd-ui-results-leaderboards
title: Results and Leaderboards (RunResults)
domain: design
status: approved
authority: canonical
owner: design
summary: "Vittoria chiusa al boss del piano 5 (DEC-031); alla sconfitta restano i punti sblocco maturati in misura ridotta (DEC-041), con un campo esplicito che dichiara la causa della sconfitta (ultimo colpo o nemico letale, DEC-159). L'abbandono confermato (ExitConfirm da PauseMenu) e il reroll da Gameplay contano entrambi come sconfitta ai fini dei punti sblocco (DEC-082), ma la presentazione differisce: l'abbandono passa da RunResults con i punti ridotti visibili lì, il reroll salta i risultati e accredita in silenzio, consultabile poi nel Catalogo (DEC-089); riprova non classificata. Schermata risultati completa con timeline, scoperte e confronto con le run passate (DEC-056); classifiche divise per metrica, tempo e punteggio separati (DEC-062). Se la run appartiene alla Classificata giornaliera, i risultati mostrano la medaglia/cornice cosmetica guadagnata, fuori dall'economia dei punti (DEC-064)."
last_reviewed: 2026-07-27
last_verified_commit: 0ec60d0
topics: [run-results, classifiche, vittoria-sconfitta, punti-sblocco, daily, causa-sconfitta, DEC-041, DEC-056, DEC-062, DEC-159]
related: []
supersedes: []
source_files: []
---

# Results and Leaderboards (RunResults)

> Aggiunta del 22/07 (DEC-132): `RunResults` include una **riga discreta e aggregata** sui
> fallback della generazione, nel registro ironico del crogiolo (es. «il crogiolo ha
> attinto due volte alla riserva») — accanto al conteggio delle creazioni entrate nel
> Catalogo. Niente tecnicismi; il dettaglio per-scheda vive nel Catalogo (DEC-103).

## Intento

Comunicare l'esito della run in modo chiaro — vittoria al boss del piano 5 o sconfitta — e
dare accesso alle classifiche coerenti con DEC-016.

## Condizioni di ingresso

Da `Gameplay`, quando la run termina: vittoria sul boss del piano 5, morte in un piano
qualunque (permadeath), o abbandono confermato da `PauseMenu`. L'abbandono confermato da
`PauseMenu` porta a `RunResults` (DEC-089): la tensione che c'era in precedenza con
`ui/pause-menu.md` e `ui/navigation-map.md` è sanata in favore di questo documento. Il
reroll da `Gameplay`, invece, **non** è un ingresso in `RunResults`: salta i risultati e va
dritto alla nuova run (vedi [Alla sconfitta](#alla-sconfitta-dec-041)).

## Vittoria e sconfitta (DEC-006, aggiornata da DEC-031)

- Sconfiggere il boss del piano 5 chiude la run con **vittoria**, valida per le classifiche.
  La run finisce lì: la prosecuzione in piani extra non è implementata in questa fase del
  gioco e resta un'idea futura parcheggiata (DEC-018, DEC-031), non un'opzione presentata in
  `RunResults`.
- La salute a zero, in qualunque piano, è run persa (permadeath): non esiste un "continua"
  dopo la morte.

## Causa della sconfitta (DEC-159)

Quando la run termina per morte, `RunResults` espone un **campo esplicito con la causa
della sconfitta**: **l'ultimo colpo o nemico letale**. Nessuna telemetria, nessun grafico
né riepilogo esteso degli ultimi secondi di combattimento: una dichiarazione leggibile di
cosa ha chiuso la run, coerente col registro ironico-leggero del crogiolo (DEC-105) e
accanto alla riga già prevista sui fallback aggregati (DEC-132). Questo campo è il
presidio concreto della promessa di chiarezza «perché è morto» descritta in
[Player Experience](../02-player-experience.md) (rimando, non riformulato qui). Il campo
non compare in caso di vittoria o di abbandono volontario, dove non c'è un colpo letale da
dichiarare.

## Alla sconfitta (DEC-041)

Alla sconfitta restano i punti sblocco maturati durante la run, ma in **misura ridotta**
rispetto alla vittoria; il catalogo si aggiorna comunque con le creazioni incontrate e con le
statistiche della run. Nessun oggetto sopravvive alla run in nessun caso (permadeath,
DEC-006).

L'**abbandono volontario di una run in corso** (`ExitConfirm` da `PauseMenu`) e il **reroll
da `Gameplay`** contano entrambi come **sconfitta** ai fini dei punti sblocco (DEC-082):
punti ridotti standard su quanto maturato fino a quel momento, nessuna categoria intermedia
rispetto a vittoria/sconfitta. La **presentazione** dei due casi differisce (DEC-089):
l'**abbandono confermato passa da `RunResults`**, come una sconfitta, con i punti ridotti
visibili lì; il **reroll salta la schermata dei risultati** e va dritto alla nuova run, con
i punti ridotti **accreditati in silenzio** e consultabili poi nel Catalogo. Il catalogo si
aggiorna comunque con le creazioni incontrate, come per qualunque run interrotta a metà
(regola già esistente, vedi [Save and Meta
Progression](../systems/save-and-meta-progression.md)). L'abbandono della sola
**preparazione nel Piano 0** (`ExitConfirm` da `FloorZero`, DEC-074) avviene prima che la
run giocata cominci: non conta come sconfitta e resta fuori da questa contabilità.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Esito | Sempre | — (sola lettura) | Nessuna | — | Vittoria (boss piano 5), sconfitta, o abbandono |
| Causa della sconfitta (DEC-159) | La run termina per sconfitta (morte) | — (sola lettura) | Nessuna | — | Dichiarazione leggibile dell'ultimo colpo o nemico letale, nel registro del crogiolo (DEC-105) |
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
8. **Given** il giocatore abbandona volontariamente la run tramite `ExitConfirm` da `PauseMenu` e conferma l'abbandono, **when** la run si chiude, **then** entra in `RunResults` con i punti sblocco maturati mostrati in misura ridotta, come per qualunque sconfitta, senza categoria intermedia, e il catalogo risulta comunque aggiornato con le creazioni incontrate (DEC-082, DEC-089).
9. **Given** il giocatore effettua un reroll da `Gameplay`, **when** la run in corso termina, **then** nessuna schermata `RunResults` viene mostrata: i punti sblocco maturati fino a quel momento si accreditano in silenzio, con la stessa riduzione della sconfitta, e restano consultabili poi nel Catalogo (DEC-082, DEC-089).
10. **Given** il giocatore muore per un colpo o un nemico letale, **when** entra in `RunResults`, **then** trova un campo esplicito con la causa della sconfitta (ultimo colpo o nemico letale), distinto dalla riga aggregata sui fallback (DEC-159).
