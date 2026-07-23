---
id: design-open-questions
title: Open Questions
domain: design
status: draft
authority: canonical
owner: design
summary: >-
  Coda ufficiale delle domande di design ancora aperte (10 voci, quasi tutte valori da playtest) dopo DEC-001..DEC-140; economia, valori numerici, personaggi, multiplayer, produzione.
last_reviewed: 2026-07-22
topics: [open-questions, governance, domande aperte, playtest, backlog design]
related: []
supersedes: []
source_files: []
---

# Open Questions

Le risposte vanno trasferite nei documenti pertinenti e registrate in `decision-log.md`.
Le domande risolte dalle decisioni DEC-001...DEC-020 (2026-07-17) e dalle decisioni del
2026-07-18 (DEC-025/026, DEC-032, DEC-063...DEC-067, DEC-069...DEC-072) erano già state
rimosse da questo elenco. La sessione di design del 2026-07-19 (DEC-074...DEC-088) ha
risolto le domande emerse dall'implementazione M5→M8: abbandono del Piano 0 (DEC-074),
mouse nel Piano 0 (DEC-075), carte tema di fallback (DEC-076), contenuto del codice di
condivisione (DEC-077), criterio dello sconto del colpo firmato (DEC-078), colpo firmato
mai scartato (DEC-079), nomi e ruoli della rosa base (DEC-080), orario della Daily
(DEC-081), punti di abbandono e reroll (DEC-082), categorie del Catalogo (DEC-083),
Catalogo come vista interna (DEC-084), Reliquie nel museo (DEC-085), interfaccia del primo
avvio (DEC-086), pool curato minimo (DEC-087) e soglia del minimo gioco base (DEC-088).
Un secondo giro nella stessa giornata (DEC-089...DEC-108) ha risolto venti domande residue
dei documenti di sistema — flusso di abbandono e reroll (DEC-089), dialogo di uscita
(DEC-090), scelte modificabili nel Piano 0 (DEC-091), arene come simulazioni senza economia
propria (DEC-092/093), soglia best-of e prove illimitate (DEC-094/095), budget vincolato per
la Classificata a seed diversi (DEC-096), rifiuto ripensabile, varietà del trait,
sostituibilità del colpo firmato e sblocchi della rosa (DEC-097...100), fusione libera e
ri-fusione (DEC-101/102), curati nel Catalogo (DEC-103), roster estendibile (DEC-104), tono
ironico-leggero (DEC-105), boss piano 2 a fase singola (DEC-106), piega-regole solo
leggendaria (DEC-107), proposte identiche in gara (DEC-108). La sessione decisionale del
2026-07-22 ha risolto la 11 (reroll dal PauseMenu con conferma, DEC-114) e le cinque domande
nate dall'audit documentale: audio generativo con fallback (DEC-109), rimozione del preset
lowspec (DEC-110), scelta binaria confermata (DEC-111), director-per-stile parcheggiato
(DEC-112), licenza Stability Community accettata (DEC-113). Restano le domande davvero
aperte, con la numerazione originale 1-10. Domande aperte più locali vivono anche nelle sezioni "Domande
aperte" dei singoli documenti di sistema (es. `systems/grafts.md`, `systems/item-fusion.md`):
quelle non sono duplicate qui.

## Economia e stanze

1. Quali sono le grandezze minime e massime delle stanze? (DEC-009 fissa solo la variabilità e una grandezza minima garantita, senza valori.)
2. Qual è l'economia esatta dei punti di meta-progressione: tasso di guadagno, costo degli sblocchi, contenuto iniziale del pool sbloccabile? (DEC-015 fissa il principio, DEC-027 fissa la struttura a doppio canale — punti base più bonus da prove specifiche — non i numeri esatti.)
3. Quali sono i valori esatti di soglia (tempo) e ricompensa delle stanze a tempo nei piani avanzati, e cosa succede se il giocatore le raggiunge dopo la soglia (nessuna ricompensa, ricompensa ridotta, o comportamento diverso)? (DEC-051 fissa solo il principio, da playtest come DEC-019.)
4. Qual è il bilanciamento fine del punteggio composito multi-percorso: peso relativo di tempo, prove/sfide, esplorazione, scoperte, eliminazioni e Veterani, e come si equivalgono esattamente un percorso rapido/efficiente e un percorso lento/esaustivo? (DEC-060 fissa le fonti e il vincolo di competitività tra percorsi, non i numeri; da playtest.)

## Valori numerici da playtest

5. I valori proposti in DEC-019 (pesi rarità {55,30,12,3}, pesi boss {0,0,70,30}, bande di potenza colpi/nemici/boss, 4 rarità) sono confermati dal playtest o vanno corretti?
6. Quali sono le bande min/max dei tetti di salute dei personaggi? (DEC-033 fissa il principio che ogni personaggio ha il proprio tetto di salute base come parte delle sue statistiche; i valori delle bande, soprattutto per il personaggio generato per run, restano da validare col playtest. Per la rosa base i tetti non sono bande ma valori FISSI curati — 8/12/16, default proposti dall'implementazione M6a, vedi `systems/characters.md` — le bande restano una domanda aperta solo per il personaggio generato per run, DEC-014/M6b. L'implementazione M6b-1 propone un default anche per queste bande — damage/fireDelay/shotSpeed/speed/maxHp/luck e la regola hpCap=2×maxHp clampato [6,18] — vedi `systems/characters.md`, blocco "Default proposti dall'implementazione": punto di partenza giocabile, non ancora approvato dal design.)
7. Il valore del fattore di compressione delle bande per il colpo firmato (0.6, default proposto M6b-3) è confermato dal playtest o va corretto? (DEC-078 fissa il criterio — compressione fissa delle bande — non il valore.)

## Personaggi

8. Quali sono le statistiche esatte della rosa base Wayfinder/Ashblade/Bulwark? (DEC-080 approva nomi e ruoli; DEC-100 fissa gli sblocchi — Wayfinder subito, Ashblade alla prima run conclusa, Bulwark al primo boss abbattuto; le statistiche restano default proposti dall'implementazione M6a, da playtest.)

## Multiplayer

9. Quali dettagli restano da definire nel multiplayer asincrono oltre a DEC-016/DEC-021/DEC-062: gestione delle disconnessioni, metriche di classifica oltre a tempo e punteggio, regole di parità e di validità della run pubblicata, e i valori esatti dei vincoli di budget della Classificata a seed diversi? (Il criterio di normalizzazione è ora fissato: budget di generazione vincolato, DEC-096; l'orario di rotazione della Daily è 00:00 UTC, DEC-081.)

## Produzione

10. I numeri della tabella del pool curato minimo per categoria (DEC-087: 3 temi, 5 boss, 12 nemici, 20 oggetti, 6 colpi) sono confermati man mano che i contenuti curati vengono prodotti, o vanno corretti? (DEC-087 approva il principio; i valori sono default proposti stile DEC-019.)
