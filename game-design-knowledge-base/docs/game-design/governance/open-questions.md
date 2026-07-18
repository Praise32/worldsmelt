# Open Questions

Le risposte vanno trasferite nei documenti pertinenti e registrate in `decision-log.md`.
Le domande risolte dalle decisioni DEC-001...DEC-020 del 2026-07-17, dalle decisioni
DEC-025/DEC-026 del 2026-07-18, e dalla decisione DEC-032 del 2026-07-18, sono state
rimosse da questo elenco; restano solo le domande davvero aperte. Le decisioni
DEC-055...DEC-062 del 2026-07-18 non hanno risolto alcuna domanda preesistente di questo
elenco, ma ne hanno aperte di nuove (vedi sotto). Le decisioni DEC-063...DEC-066 del
2026-07-18 risolvono parzialmente la domanda sulla Daily (le ricompense dedicate sono
ora definite come cosmetiche, DEC-064): resta aperto solo l'orario esatto di rotazione (vedi
sotto). La decisione DEC-067 del 2026-07-18 approva una cornice narrativa minima ("il
crogiolo dei mondi"): il tono narrativo generale non è più completamente aperto, resta
aperto solo il tono specifico (tragico, ironico, ecc.), i simboli ricorrenti e i limiti di
contenuto — dettagli registrati in `content/narrative-tone.md`, non duplicati qui. Le
decisioni DEC-069 e DEC-070 del 2026-07-18 non risolvono domande preesistenti di questo
elenco, ma ne aprono due nuove (vedi sotto). Le decisioni DEC-071 e DEC-072 del 2026-07-18
risolvono per intero la sezione "Nome e vocabolario" che comparirebbe qui (il nome
definitivo del gioco è Worldsmelt, e i nomi definitivi di valuta principale, strumento di
breccia, strumento di apertura, catalizzatore di fusione, Innesto e Veterano hanno ora il
rispettivo nome inglese in-game, vedi `governance/glossary.md`): la sezione e le sue due
domande sono state rimosse da questo elenco, che ora inizia dalla sezione Economia e stanze.

## Economia e stanze

1. Quali sono le grandezze minime e massime delle stanze? (DEC-009 fissa solo la variabilità e una grandezza minima garantita, senza valori.)
2. Qual è l'economia esatta dei punti di meta-progressione: tasso di guadagno, costo degli sblocchi, contenuto iniziale del pool sbloccabile? (DEC-015 fissa il principio, DEC-027 fissa la struttura a doppio canale — punti base più bonus da prove specifiche — non i numeri esatti.)
3. Quali sono i valori esatti di soglia (tempo) e ricompensa delle stanze a tempo nei piani avanzati, e cosa succede se il giocatore le raggiunge dopo la soglia (nessuna ricompensa, ricompensa ridotta, o comportamento diverso)? (DEC-051 fissa solo il principio, da playtest come DEC-019.)
4. Qual è il bilanciamento fine del punteggio composito multi-percorso: peso relativo di tempo, prove/sfide, esplorazione, scoperte, eliminazioni e Veterani, e come si equivalgono esattamente un percorso rapido/efficiente e un percorso lento/esaustivo? (DEC-060 fissa le fonti e il vincolo di competitività tra percorsi, non i numeri; da playtest.)

## Valori numerici da playtest

5. I valori proposti in DEC-019 (pesi rarità {55,30,12,3}, pesi boss {0,0,70,30}, bande di potenza colpi/nemici/boss, 4 rarità) sono confermati dal playtest o vanno corretti?
6. Quali sono le bande min/max dei tetti di salute dei personaggi? (DEC-033 fissa il principio che ogni personaggio ha il proprio tetto di salute base come parte delle sue statistiche; i valori delle bande, soprattutto per il personaggio generato per run, restano da validare col playtest. Per la rosa base i tetti non sono bande ma valori FISSI curati — 8/12/16, default proposti dall'implementazione M6a, vedi `systems/characters.md` — le bande restano una domanda aperta solo per il personaggio generato per run, DEC-014/M6b.)

## Personaggi

7. Composizione esatta della rosa dei personaggi base: nomi, ruoli precisi oltre alle indicazioni offensivo/difensivo/esploratore, e condizioni esatte di sblocco di ciascuno (DEC-030 fissa solo il principio di una rosa di 2-3 personaggi curati, sbloccabili presto). L'implementazione M6a propone dei default (Wayfinder/Ashblade/Bulwark, vedi `systems/characters.md`, blocco "Default proposti dall'implementazione"): nomi, ruoli e statistiche sono un punto di partenza giocabile, non ancora approvati dal design; la domanda resta aperta.

## Multiplayer

8. Quali dettagli restano da definire nel multiplayer asincrono oltre a DEC-016/DEC-021/DEC-062: gestione delle disconnessioni, metriche di classifica oltre a tempo e punteggio, regole di parità e di validità della run pubblicata, e soprattutto il criterio di normalizzazione della difficoltà per la Classificata a seed diversi?
9. Qual è l'orario esatto di rotazione del cambio giornaliero della Classificata giornaliera pubblica ("Daily")? (DEC-062 fissa che la Daily esiste, usa lo stesso seed per tutti i giocatori e cambia ogni giorno; DEC-064 fissa che premia con medaglie/cornici cosmetiche legate a piazzamenti e streak, risolvendo la parte "ricompense dedicate?" di questa domanda. Resta aperto solo l'orario esatto di rotazione.)

## Produzione

10. Quali contenuti curati minimi devono esistere per garantire una run di fallback completa (Piano 0 + 5 piani) senza alcuna generazione IA disponibile?
11. Qual è il minimo gioco base da completare prima di espandere la generazione IA?

## Primo avvio e migrazione del catalogo

12. Dettagli dell'interfaccia della scelta binaria completo/solo-curato al primo avvio (schermata dedicata, overlay, punto esatto di rientro se annullata) e dove/come si riattiva la generazione per chi ha scelto solo curato inizialmente. (DEC-070 fissa solo il principio e il punto in cui avviene la scelta, non l'interfaccia esatta.)
13. Un contenuto promosso al museo del Piano 0 (per metriche o come preferito, DEC-063) che diventa una Reliquia dopo un aggiornamento (DEC-069): resta esposto nel museo o ne esce automaticamente?

## Stati e flusso

14. Abbandono del Piano 0: la mappa canonica non prevede un arco FloorZero → MainMenu; l'implementazione attuale usa ESC → ExitConfirm (abbandona la preparazione). Va sancito o sostituito?

## Piano 0 e scelta del tema (M5, 18/07/2026)

15. Il codice breve di condivisione run (DEC-066: seed più versione di gioco) non porta la scelta del tema né quella del personaggio fatte dal giocatore nel Piano 0 — solo il seed, da cui si rigenerano contenuti proposti, non necessariamente la STESSA scelta. Come va esteso (se va esteso) perché chi riceve un codice breve possa rigiocare esattamente la stessa run, tema e personaggio inclusi, e non solo "una run con lo stesso seed"?
16. Il Piano 0 conta come "menu" ai fini dell'ammissione del mouse (DEC-057, "il mouse è ammesso solo nei menu")? La scelta del tema nel Piano 0 (carte selezionabili) è implementata solo con tastiera/pad per non prendere questa decisione in silenzio durante l'implementazione (M5): resta da stabilire se le carte tema debbano diventare cliccabili come le voci di menu degli altri stati, o se il Piano 0 resti un'eccezione dove il mouse non è ammesso.
17. Quante carte tema curate di fallback mostrare quando nessuna proposta dell'IA supera la validazione: vedi la stessa domanda, con lo stesso default proposto (3), in `systems/floor-zero.md`, sezione "Domande aperte residue".
