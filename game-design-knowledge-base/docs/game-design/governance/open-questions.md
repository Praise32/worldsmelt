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
Restano le domande davvero aperte, rinumerate da 1. Domande aperte più locali vivono anche
nelle sezioni "Domande aperte" dei singoli documenti di sistema (es. `systems/floor-zero.md`
per arene, dote e museo): quelle non sono duplicate qui.

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

8. Quali sono le statistiche esatte e le condizioni di sblocco della rosa base Wayfinder/Ashblade/Bulwark? (DEC-080 approva nomi e ruoli come canone; le statistiche restano default proposti dall'implementazione M6a, da playtest; DEC-030 chiede sblocchi "presto" senza definirli.)

## Multiplayer

9. Quali dettagli restano da definire nel multiplayer asincrono oltre a DEC-016/DEC-021/DEC-062: gestione delle disconnessioni, metriche di classifica oltre a tempo e punteggio, regole di parità e di validità della run pubblicata, e soprattutto il criterio di normalizzazione della difficoltà per la Classificata a seed diversi? (L'orario di rotazione della Daily è ora fissato: 00:00 UTC, DEC-081.)

## Produzione

10. I numeri della tabella del pool curato minimo per categoria (DEC-087: 3 temi, 5 boss, 12 nemici, 20 oggetti, 6 colpi) sono confermati man mano che i contenuti curati vengono prodotti, o vanno corretti? (DEC-087 approva il principio; i valori sono default proposti stile DEC-019.)

## Stati e flusso

11. Flusso dell'abbandono di una run in corso (incoerenza emersa applicando DEC-082): `ui/results-and-leaderboards.md` elenca l'abbandono confermato da `PauseMenu` tra le condizioni di ingresso di `RunResults`, ma `ui/pause-menu.md` e `ui/navigation-map.md` documentano l'abbandono come ritorno diretto a `MainMenu` senza passare da `RunResults`. Va deciso dove il giocatore vede l'accredito dei punti ridotti di DEC-082 in quei flussi — passaggio da `RunResults` anche all'abbandono, o accredito silenzioso col ritorno diretto al menu — e va sanata l'incoerenza tra i documenti. Resta inoltre da documentare la collocazione UI esatta del reroll da `Gameplay` (comando, eventuale conferma).
