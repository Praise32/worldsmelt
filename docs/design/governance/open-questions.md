---
id: design-open-questions
title: Open Questions
domain: design
status: draft
authority: canonical
owner: design
summary: >-
  Coda ufficiale e unica delle domande ancora aperte (21 voci attive su 22 numerate; la 12 è chiusa da DEC-176) dopo DEC-001..DEC-176: economia, valori numerici da playtest, personaggi, multiplayer, produzione, interfaccia, distribuzione e produzione AI/asset.
last_reviewed: 2026-07-28
last_verified_commit: d30890b
topics: [open-questions, governance, domande aperte, playtest, backlog design, interfaccia, distribuzione, produzione ai, DEC-174, DEC-176]
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
(DEC-112), licenza Stability Community accettata (DEC-113). Un'ulteriore sessione di audit
del 2026-07-25 ha risolto tre domande residue emerse dalla revisione documentale, anch'esse
fuori dalla numerazione 1-10 sotto: il fix del RNG di gameplay come prerequisito bloccante
della Classificata a stesso seed (DEC-141), i requisiti hardware minimi espressi in numeri
misurati anziché in nomi di modello (DEC-142), e la categoria ereditata da una fusione
cross-categoria — vince la sorgente dominante per rarità (DEC-143). Nessuna delle 10 domande
numerate sotto era chiusa da quella sessione.

La sessione del 2026-07-27 ha registrato il batch DEC-144...DEC-169, che chiude le domande
residue dell'audit documentale: pool curato con almeno un oggetto per rarità (DEC-144),
Fortuna che riduce la soglia della correzione di fortuna su tutti i pool (DEC-145), proxy
operativo della leggibilità visiva (DEC-146), pipeline immagini su SD1.5 con Style LoRA su
base vanilla (DEC-148), card di scoperta scartate a morte e cambio stanza (DEC-152),
fallback curato come stato base del gioco (DEC-153), Innesto sganciato recuperabile nella
stanza (DEC-160), conflitti di categoria risolti con l'RNG del seed di run (DEC-161),
budget dedicato al risultato di sinergie e fusioni (DEC-162), lettore di schermo
circoscritto ai menu testuali (DEC-166), valuta da qualunque stanza completata secondo la
propria condizione (DEC-167), HUD nascosto nel Piano 0 (DEC-169). La stessa sessione ha
promosso DEC-019 ad `approved` (DEC-154) **senza** chiudere la domanda 5: la promozione
riguarda l'impianto, non i valori, che restano da playtest. Nessuna delle 10 domande
numerate sotto è quindi chiusa; la 10 resta aperta solo sui numeri, dopo che DEC-144 ha
fissato il vincolo di copertura per rarità.

Una sessione successiva, la sera del 2026-07-27, ha registrato il via all'implementazione
della demo con altre tre decisioni — DEC-170 (taglie multiple stile Isaac e telecamera a
zoom fisso nelle stanze più grandi, con nota di supersessione parziale su DEC-009), DEC-171
(la demo copre tutti i sistemi documentati; le immagini del contenuto curato vengono dal
dataset CC0 come ponte provvisorio fino al training della Style LoRA) e DEC-172 (l'audio
della demo è un pacchetto pre-generato offline, non la pipeline generativa a runtime di
DEC-109) — senza chiudere nessuna delle 22 domande numerate sotto: sono decisioni di
implementazione della demo, non risposte a domande già in coda. La domanda 1 riceve solo un
rimando a DEC-170 nel suo testo, perché DEC-170 non fissa le dimensioni esatte di una
cella/taglia.

Con DEC-147 questa pagina diventa la **coda ufficiale e unica** delle domande aperte del
progetto: il questionario, ora in `docs/archive/superseded/19-DECISION-QUESTIONNAIRE.md`, e
il piano, ora in `docs/plans/cancelled/aiprod-proposed-kb-updates.md`, sono stati chiusi e
tolti dalle cartelle vive (il piano segue la regola dei piani di DEC-157, il questionario
quella dell'archivio). Le loro
domande già risolte sono state chiuse citando la decisione corrispondente — audio
generativo (DEC-109), licenza Stability (DEC-113), identità del Piano 0 (DEC-004/DEC-063/
DEC-085), direzioni di mira (DEC-007), input obbligatori (DEC-057), generazione nel
prodotto e scelta binaria al primo avvio (DEC-070/DEC-086/DEC-111), formulazione dei
requisiti hardware minimi (DEC-142), autorità degli agenti e regole di commit
(`CLAUDE.md`, `docs/ai-production/18-AGENT-ORCHESTRATION.md`, DEC-164) — mentre le domande
davvero residue sono state **trasferite qui** come voci 11-21, con la provenienza citata
voce per voce.

Restano quindi le domande davvero aperte: la numerazione originale 1-10 dell'audit di
design, le voci 11-21 trasferite e la 22, aperta dalla sessione stessa (DEC-169 indica il
menu di pausa come luogo di consultazione dell'HUD nel Piano 0 senza fissare il comando che
lo apre). La 22 sta nella sezione «Interfaccia» pur essendo fuori sequenza: i numeri sono
identificatori stabili, le sezioni raggruppano per tema. Domande aperte più locali vivono anche nelle sezioni
"Domande aperte" dei singoli documenti di sistema (es. `systems/grafts.md`,
`systems/item-fusion.md`): quelle non sono duplicate qui.

## Economia e stanze

1. Quali sono le grandezze minime e massime delle stanze, in pixel per ciascuna taglia? (DEC-009 fissa solo la variabilità e una grandezza minima garantita, senza valori; DEC-170 aggiunge le taglie multiple in classi discrete 1x1/1x2/2x1/2x2/L e il comportamento della telecamera, senza fissare le dimensioni esatte di una cella né quale taglia ricevano la stanza boss e la stanza di partenza.) **Aggiornamento 27/07:** l'implementazione di DEC-170 ha adottato dei **default proposti** (cella = 876×458 px, cioè il canvas logico; partenza 1x1; boss 2x2 quando entra nella griglia; tesoro/negozio 1x1; distribuzione 55/15/15/8/7) — vedi `systems/rooms-and-floor-generation.md`, sezione «Default proposti dall'implementazione (DEC-170)». La domanda **resta aperta**: sono default di implementazione, non una decisione di design.
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

10. I numeri della tabella del pool curato minimo per categoria (DEC-087: 3 temi, 5 boss, 12 nemici, 20 oggetti, 6 colpi) sono confermati man mano che i contenuti curati vengono prodotti, o vanno corretti? (DEC-087 approva il principio; i valori sono default proposti stile DEC-019. DEC-144 ha fissato il vincolo di copertura — almeno 1 oggetto per rarità, eccedenza sottratta alle rarità più comuni — e l'esempio derivato 11/6/2/1: restano da confermare i numeri, non il vincolo.)

## Interfaccia

11. Qual è la risoluzione logica canonica dell'interfaccia e con quale regola di scaling? La proposta ricorrente è **640×360 con scaling intero**, presente negli appunti e nei template ma **mai approvata**: DEC-156 la apre esplicitamente come domanda aperta e fa marcare il valore come non approvato in `docs/ai-production/templates/UI-SKIN-SPEC.md`. (Provenienza: `Q-UI-002` del questionario ai-production archiviato, priorità BLOCKING per l'implementazione UI.) **Resta aperta (28/07, DEC-174):** il proprietario ha scelto di non deciderla ora; l'HUD in pixel art della demo si disegna nel frattempo per il **canvas logico attuale, 960×640** (lo stesso di DEC-170), non per fissare implicitamente questa domanda. Si decide dopo la demo.
12. ~~Qual è la scala dei pixel del gioco — pixel nativi molto grandi, pixel medi, dettaglio alto con pixel snapping — e vale la stessa scala per il mondo e per l'interfaccia?~~ (Provenienza: `Q-UI-003`, BLOCKING per l'art bible.) **Chiusa (28/07, DEC-176):** il proprietario ha scelto, al checkpoint CP1 della produzione pixel-art, la **scala base 24px** per personaggi/nemici/oggetti (boss più grandi); le **icone HUD seguono la propria griglia**, indipendente da questa scala — nessun obbligo di scala condivisa fra mondo e interfaccia. Vedi `content/visual-language.md`, sezione «Stile pixel-art ufficiale e scala base sprite».
13. Serve uno strumento di design come fonte dell'interfaccia (Penpot canonico, Penpot solo per mockup, un altro strumento, oppure file Markdown più PNG/SVG senza strumento), e in quale forma (cloud, self-host, nessuna integrazione)? (Provenienza: `Q-UI-001` e `Q-UI-005`.)
22. Con quale comando il menu di pausa si apre dal Piano 0? DEC-169 indica il menu di pausa come luogo in cui l'HUD di combattimento resta consultabile durante il Piano 0, ma ESC è già assegnato a `ExitConfirm` (DEC-074) e le condizioni di ingresso di `PauseMenu` prevedono oggi la sola provenienza da `Gameplay` (`ui/pause-menu.md`, `05-game-states-and-flow.md`). Manca il comando — o lo stato — che rende operativa la consultazione. (Provenienza: gap aperto da DEC-169 nella sessione del 2026-07-27; registrata anche in `ui/pause-menu.md` e `systems/floor-zero.md`.)

## Distribuzione

La **proprietà** di questo tema è assegnata al dominio ai-production da DEC-158, che non decide nessuna delle domande qui sotto.

14. Quali sono le piattaforme di destinazione della prima release: Linux, Windows, Steam Deck, macOS? Il repository conserva Windows, ma Linux è la piattaforma principale di sviluppo. (Provenienza: `Q-DIST-001`, priorità BLOCKING.)
15. Qual è il formato della **AI disclosure** e dove vive (pagina negozio, primo avvio, crediti in gioco), e quali requisiti deve soddisfare la pagina negozio? (DEC-158 assegna il tema ad ai-production proprio perché quel dominio possiede licenze e provenienza dei contenuti — DEC-113, DEC-140, DEC-148, `docs/ai-production/licenze.md` — su cui la disclosure poggia.)
16. Come arrivano i modelli all'utente: installer separato, download al primo avvio, strumento o DLC di piattaforma, oppure nessun modello distribuito? (DEC-070/DEC-086 fissano la scelta binaria al primo avvio e DEC-113/DEC-140 stabiliscono che i pesi non sono mai ridistribuiti col gioco — li scarica l'utente —, ma il meccanismo concreto di consegna non è deciso. Provenienza: `Q-DIST-002`.)
17. Il giocatore può importare contenuti propri — modelli, LoRA, prompt pack, rig, AudioSpec, skin dell'interfaccia — e con quali garanzie di validazione e originalità? (Provenienza: `Q-DIST-003`, priorità LATER, più la parte residua di `Q-IMG-001` sull'import di LoRA dell'utente; la sorte delle LoRA del progetto è invece decisa da DEC-148.)

## Produzione AI e asset

18. Quali body plan si realizzano per primi e con quale approccio si anima il personaggio giocante (rig modulare con skin, spritesheet completo, ibrido, stickman fino alla vertical slice)? (`docs/ai-production/09-NEMICI-BODY-PLAN-RIG.md` propone senza decidere. Provenienza: `Q-ANIM-001` e `Q-ANIM-002`.)
19. Quante immagini originali il proprietario può realisticamente produrre e rivedere per i **dataset definitivi**, quale regime di review manuale vale per gli asset (approvazione umana sempre, solo per la release, a campione) e serve uno strumento interno di approvazione/rifiuto già nella pre-alpha? (DEC-148 stabilisce che i dataset definitivi li crea il proprietario, non la loro ampiezza né il regime di review. Provenienza: `Q-IMG-003`, `Q-IMG-004`, `Q-F0-002`.)
20. Il primo esperimento audio riguarda solo SFX, solo musica, o entrambi con due milestone separate? (DEC-109 fissa pipeline e catena di fallback, non l'ordine degli esperimenti. Provenienza: `Q-AUD-002`.)
21. Qual è il budget cloud disponibile e quante ore settimanali il proprietario può dedicare a domande, review di codice e asset, ascolto audio e playtest? (DEC-168 porta il training sulle 30 ore settimanali gratuite di Kaggle e declassa il runbook RunPod a fallback a pagamento, senza impegnare alcun budget. Serve a dimensionare i batch di lavoro. Provenienza: `Q-BUD-001` e `Q-BUD-002`.)
