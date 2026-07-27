---
id: gd-player-experience
title: Player Experience
domain: design
status: draft
authority: canonical
owner: design
summary: "Esperienza emotiva, ritmo e chiarezza."
last_reviewed: 2026-07-27
topics: [esperienza-giocatore, ritmo, chiarezza, leggibilità, causa-sconfitta, DEC-159]
related: []
supersedes: []
source_files: []
---

# Player Experience

## Emozioni desiderate

- Curiosità all'inizio della run.
- Comprensione progressiva delle nuove regole.
- Soddisfazione quando una sinergia emerge.
- Tensione crescente verso il boss.
- Desiderio di condividere una build o una run irripetibile.

## Ritmo

1. Piano 0: orientamento, scelta del tema tra le proposte dell'IA, scelta del personaggio,
   attesa percepita ridotta da un hub già giocabile (vedi [Run Structure](04-run-structure.md)).
2. Orientamento e primo combattimento leggibile nel piano 1.
3. Prima scelta significativa, inclusa un'eventuale prima fusione esplicita in stanza di
   fusione.
4. Aumento della complessità, con il tema che evolve o degenera piano dopo piano.
5. Picco di potere o rischio.
6. Boss e ricompensa.
7. Transizione al piano successivo, fino al boss del piano 5: la vittoria chiude la run lì
   (DEC-006, DEC-031); una prosecuzione oltre resta solo un'idea futura non implementata
   (DEC-018).

## Principio di chiarezza

Il giocatore non deve conoscere in anticipo ogni oggetto, ma deve poter dedurre:

- che tipo di effetto produce;
- quale rischio introduce;
- perché è morto (promessa presidiata dal campo esplicito di causa della sconfitta in
  `RunResults` — ultimo colpo o nemico letale, DEC-159; vedi
  [Results and Leaderboards](ui/results-and-leaderboards.md), rimando, non riformulato qui);
- quale scelta ha modificato la build;
- quali componenti della sinergia sono attive;
- quali due oggetti hanno generato un oggetto di fusione, quando presente.

## Evitare

- Effetti visivi che nascondono i proiettili nemici.
- Nomi e descrizioni senza relazione con il comportamento.
- Nemici con pattern non anticipabili.
- Oggetti che annullano una build senza avviso.
- Variazioni puramente cosmetiche presentate come contenuti meccanici nuovi.
