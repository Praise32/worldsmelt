---
id: design-open-questions
title: Open Questions
domain: design
status: draft
authority: canonical
owner: design
summary: >-
  Coda ufficiale e unica delle domande ancora aperte (37 voci attive su 58 numerate; la voce 12 già chiusa in precedenza da DEC-176/177, e il batch DEC-185..DEC-204 del 31/07 chiude altre 20 voci, aggiornando senza chiudere le voci 9 e 17) dopo DEC-001..DEC-204: economia, valori numerici da playtest, personaggi, multiplayer, produzione, interfaccia, distribuzione, produzione AI/asset, stanze speciali nel motore inclusa la stanza segreta a due livelli (WP8), il catalogo delle prove specifiche della run (WP16), l'abbandono di una run in corso (WP19), il tasto rapido R del reroll nel motore (WP21) e l'oscuramento del dialogo leggero ExitConfirm/MainMenu (WP22).
last_reviewed: 2026-07-31
last_updated_from_session: 2026-07-31-decision-batch-dec185-204
last_verified_commit: 4d7a410
topics: [open-questions, governance, domande aperte, playtest, backlog design, interfaccia, distribuzione, produzione ai, DEC-174, DEC-176, DEC-051, DEC-008, DEC-043, DEC-010, DEC-022, DEC-025, DEC-127, DEC-042, DEC-027, DEC-090, DEC-114, WP4, WP5, WP6, WP7, WP8, WP-INT, WP16, WP19, WP21, WP22, DEC-185, DEC-204]
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

**Batch decisionale del 2026-07-31 (DEC-185...DEC-204).** Il proprietario ha risposto a un
batch di 22 domande poste da Fable, in gran parte default proposti dall'implementazione in
attesa di conferma. Venti risposte chiudono altrettante voci con una DEC dedicata ciascuna:
22 (DEC-185, comando di pausa dal Piano 0), 26 (DEC-186, card di scoperta in basso al
centro), 31 (DEC-187, conferma fusione in `BuildScreen`), 55 (DEC-188, `R` comando di
sviluppo), 24 (DEC-189, file di preferenze `prefs/settings.txt`), 25 (DEC-190, slider
volumi canone), 23 (DEC-191, escalation tileset dal piano 3), 27 (DEC-192, timer fermo nel
Piano 0), 14 (DEC-193, piattaforme Linux+Windows), 15 (DEC-194, AI disclosure su negozio e
crediti), 16 (DEC-195, modelli via download al primo avvio), 20 (DEC-196, primo esperimento
audio SFX+musica insieme), 13 (DEC-197, strumento UI Markdown+PNG, niente Penpot), 35
(DEC-198, spuntoni temporizzati), 36 (DEC-199, personaggio generato su sheet Fonditrice),
11 (DEC-200, risoluzione logica 640×360, migrazione a CP4), 19 (DEC-201, regime di review
sempre esplicito per curato/LoRA definitivi), 58 (DEC-202, default sospensione confermati
in blocco), 54 (DEC-203, abbandono finalizza le prove come fine run), 21 (DEC-204, 10+ ore
settimanali del proprietario). Due risposte **aggiornano ma non chiudono** la voce, con un
rinvio esplicito registrato nel testo, senza una DEC di merito: 17 (import di contenuti
utente rimandato a dopo la release) e 9 (dettagli del multiplayer asincrono rimandati a una
sessione dedicata dopo il playtest della demo).

## Economia e stanze

1. Quali sono le grandezze minime e massime delle stanze, in pixel per ciascuna taglia? (DEC-009 fissa solo la variabilità e una grandezza minima garantita, senza valori; DEC-170 aggiunge le taglie multiple in classi discrete 1x1/1x2/2x1/2x2/L e il comportamento della telecamera, senza fissare le dimensioni esatte di una cella né quale taglia ricevano la stanza boss e la stanza di partenza.) **Aggiornamento 27/07:** l'implementazione di DEC-170 ha adottato dei **default proposti** (cella = 876×458 px, cioè il canvas logico; partenza 1x1; boss 2x2 quando entra nella griglia; tesoro/negozio 1x1; distribuzione 55/15/15/8/7) — vedi `systems/rooms-and-floor-generation.md`, sezione «Default proposti dall'implementazione (DEC-170)». La domanda **resta aperta**: sono default di implementazione, non una decisione di design.
2. Qual è l'economia esatta dei punti di meta-progressione: tasso di guadagno, costo degli sblocchi, contenuto iniziale del pool sbloccabile? (DEC-015 fissa il principio, DEC-027 fissa la struttura a doppio canale — punti base più bonus da prove specifiche — non i numeri esatti.)
3. Quali sono i valori esatti di soglia (tempo) e ricompensa delle stanze a tempo nei piani avanzati, e cosa succede se il giocatore le raggiunge dopo la soglia (nessuna ricompensa, ricompensa ridotta, o comportamento diverso)? (DEC-051 fissa solo il principio, da playtest come DEC-019.) **Aggiornamento 30/07 (WP5):** esiste ora un default proposto e implementato — soglia `40s + 6s × celle vere del piano`, misurata dall'ingresso nel piano (`Game.floorEntryElapsedSeconds`, mai dall'inizio della run); ricompensa 6 Ingots (`WORLD_ROOM_CURRENCY_TIMED`) SOLO entro soglia; oltre soglia **nessuna ricompensa** (non ridotta: zero), stanza comunque sempre percorribile — vedi `systems/special-rooms.md`, sezione "Stato di implementazione: la stanza a tempo", e la voce 32 sotto per la frequenza/i piani ammessi. La domanda resta aperta: sono default di implementazione, non una decisione di design.
4. Qual è il bilanciamento fine del punteggio composito multi-percorso: peso relativo di tempo, prove/sfide, esplorazione, scoperte, eliminazioni e Veterani, e come si equivalgono esattamente un percorso rapido/efficiente e un percorso lento/esaustivo? (DEC-060 fissa le fonti e il vincolo di competitività tra percorsi, non i numeri; da playtest.)

## Valori numerici da playtest

5. I valori proposti in DEC-019 (pesi rarità {55,30,12,3}, pesi boss {0,0,70,30}, bande di potenza colpi/nemici/boss, 4 rarità) sono confermati dal playtest o vanno corretti?
6. Quali sono le bande min/max dei tetti di salute dei personaggi? (DEC-033 fissa il principio che ogni personaggio ha il proprio tetto di salute base come parte delle sue statistiche; i valori delle bande, soprattutto per il personaggio generato per run, restano da validare col playtest. Per la rosa base i tetti non sono bande ma valori FISSI curati — 8/12/16, default proposti dall'implementazione M6a, vedi `systems/characters.md` — le bande restano una domanda aperta solo per il personaggio generato per run, DEC-014/M6b. L'implementazione M6b-1 propone un default anche per queste bande — damage/fireDelay/shotSpeed/speed/maxHp/luck e la regola hpCap=2×maxHp clampato [6,18] — vedi `systems/characters.md`, blocco "Default proposti dall'implementazione": punto di partenza giocabile, non ancora approvato dal design.)
7. Il valore del fattore di compressione delle bande per il colpo firmato (0.6, default proposto M6b-3) è confermato dal playtest o va corretto? (DEC-078 fissa il criterio — compressione fissa delle bande — non il valore.)

## Personaggi

8. Quali sono le statistiche esatte della rosa base Wayfinder/Ashblade/Bulwark? (DEC-080 approva nomi e ruoli; DEC-100 fissa gli sblocchi — Wayfinder subito, Ashblade alla prima run conclusa, Bulwark al primo boss abbattuto; le statistiche restano default proposti dall'implementazione M6a, da playtest.)

## Multiplayer

9. Quali dettagli restano da definire nel multiplayer asincrono oltre a DEC-016/DEC-021/DEC-062: gestione delle disconnessioni, metriche di classifica oltre a tempo e punteggio, regole di parità e di validità della run pubblicata, e i valori esatti dei vincoli di budget della Classificata a seed diversi? (Il criterio di normalizzazione è ora fissato: budget di generazione vincolato, DEC-096; l'orario di rotazione della Daily è 00:00 UTC, DEC-081.) **Aggiornamento 31/07 — RIMANDATA, non decisa qui:** il proprietario ha scelto esplicitamente di rimandare i dettagli del multiplayer asincrono a **dopo il playtest della demo**, in una sessione dedicata separata. Nessuna DEC di merito registra questa voce: è un rinvio, non una risposta.

## Produzione

10. I numeri della tabella del pool curato minimo per categoria (DEC-087: 3 temi, 5 boss, 12 nemici, 20 oggetti, 6 colpi) sono confermati man mano che i contenuti curati vengono prodotti, o vanno corretti? (DEC-087 approva il principio; i valori sono default proposti stile DEC-019. DEC-144 ha fissato il vincolo di copertura — almeno 1 oggetto per rarità, eccedenza sottratta alle rarità più comuni — e l'esempio derivato 11/6/2/1: restano da confermare i numeri, non il vincolo.)

## Interfaccia

~~11. Qual è la risoluzione logica canonica dell'interfaccia e con quale regola di scaling? La proposta ricorrente è **640×360 con scaling intero**, presente negli appunti e nei template ma **mai approvata**: DEC-156 la apre esplicitamente come domanda aperta e fa marcare il valore come non approvato in `docs/ai-production/templates/UI-SKIN-SPEC.md`. (Provenienza: `Q-UI-002` del questionario ai-production archiviato, priorità BLOCKING per l'implementazione UI.) **Resta aperta (28/07, DEC-174):** il proprietario ha scelto di non deciderla ora; l'HUD in pixel art della demo si disegna nel frattempo per il **canvas logico attuale, 960×640** (lo stesso di DEC-170), non per fissare implicitamente questa domanda. Si decide dopo la demo.~~ **Chiusa (31/07, DEC-200):** la risoluzione logica canonica è **640×360** (16:9 nativo, scala intera ×3 = 1920×1080). La migrazione della GUI avviene nella sessione **CP4**; fino ad allora la demo resta sul canvas 960×640 (DEC-174, invariata). Sul 21:9: pillarbox a scala intera nella prima release, vista ultrawide nativa eventuale estensione futura non decisa. Vedi `docs/ai-production/templates/UI-SKIN-SPEC.md`.
12. ~~Qual è la scala dei pixel del gioco — pixel nativi molto grandi, pixel medi, dettaglio alto con pixel snapping — e vale la stessa scala per il mondo e per l'interfaccia?~~ (Provenienza: `Q-UI-003`, BLOCKING per l'art bible.) **Chiusa (28/07, DEC-176; valore rettificato da DEC-177):** il proprietario ha scelto, al checkpoint CP1 della produzione pixel-art, la scala base per personaggi/nemici/oggetti — fissata a 24px da DEC-176 e **corretta lo stesso giorno a 32px** da DEC-177 per allinearsi alla pipeline SD1.5/LoRA (512/32 = 16 esatto); i boss possono superarla e le **icone HUD seguono la propria griglia**, indipendente da questa scala — nessun obbligo di scala condivisa fra mondo e interfaccia. Vedi `content/visual-language.md`, sezione «Stile pixel-art ufficiale e scala base sprite».
~~13. Serve uno strumento di design come fonte dell'interfaccia (Penpot canonico, Penpot solo per mockup, un altro strumento, oppure file Markdown più PNG/SVG senza strumento), e in quale forma (cloud, self-host, nessuna integrazione)? (Provenienza: `Q-UI-001` e `Q-UI-005`.)~~ **Chiusa (31/07, DEC-197):** lo strumento è **Markdown + PNG nel repository**, mock riproducibili via script/Aseprite. **Niente Penpot**: la proposta di `docs/ai-production/15-UI-DESIGN-PIPELINE.md` (documento `approved`) è superata su questo punto specifico — vedi la nota di supersessione parziale in testa a quel documento.
~~22. Con quale comando il menu di pausa si apre dal Piano 0? DEC-169 indica il menu di pausa come luogo in cui l'HUD di combattimento resta consultabile durante il Piano 0, ma ESC è già assegnato a `ExitConfirm` (DEC-074) e le condizioni di ingresso di `PauseMenu` prevedono oggi la sola provenienza da `Gameplay` (`ui/pause-menu.md`, `05-game-states-and-flow.md`). Manca il comando — o lo stato — che rende operativa la consultazione. (Provenienza: gap aperto da DEC-169 nella sessione del 2026-07-27; registrata anche in `ui/pause-menu.md` e `systems/floor-zero.md`.) **Aggiornamento 30/07 (WP15a) — DEFAULT PROPOSTO DALL'IMPLEMENTAZIONE, la domanda RESTA APERTA:** dal Piano 0 il menu di pausa si apre con il **comando di pausa**, lo stesso di `Gameplay`, e "Riprendi"/ESC riportano nel Piano 0 invece che in `Gameplay` (`AppUi.pauseFromFloorZero`, zero-default falso = comportamento storico invariato). Scelto per esclusione motivata, non per gusto: ESC è `ExitConfirm` (DEC-074) e TAB è già il pannello mondi/personaggi del Piano 0 (M5/M6a, con l'invito "TAB per le carte" scritto a schermo, quindi riusarlo romperebbe un'affordance esistente); il comando di pausa era l'unico tasto libero nel Piano 0 ed è anche l'unico che significhi già "fermati e guarda lo stato" per chi ha giocato una run. Il riquadro di consultazione non è cambiato: `DrawPauseMenuFloorZeroConsult` era già disegnato, condizionato solo a `game->floor == 0`. Verificato da `--arena-hub-test`. Resta un default di implementazione: la scelta è del proprietario.~~ **Chiusa (31/07, DEC-185):** il default WP15a è promosso a canone. Dal Piano 0 il menu di pausa si apre col comando di pausa, in sola consultazione; "Riprendi" torna nell'hub; ESC resta `ExitConfirm` (DEC-074), invariato.

## Distribuzione

La **proprietà** di questo tema è assegnata al dominio ai-production da DEC-158, che non decide nessuna delle domande qui sotto.

~~14. Quali sono le piattaforme di destinazione della prima release: Linux, Windows, Steam Deck, macOS? Il repository conserva Windows, ma Linux è la piattaforma principale di sviluppo. (Provenienza: `Q-DIST-001`, priorità BLOCKING.)~~ **Chiusa (31/07, DEC-193):** **Linux e Windows** per la prima release. Steam Deck riceve una verifica dedicata in seguito, non è un requisito della release 1.
15. ~~Qual è il formato della **AI disclosure** e dove vive (pagina negozio, primo avvio, crediti in gioco), e quali requisiti deve soddisfare la pagina negozio? (DEC-158 assegna il tema ad ai-production proprio perché quel dominio possiede licenze e provenienza dei contenuti — DEC-113, DEC-140, DEC-148, `docs/ai-production/licenze.md` — su cui la disclosure poggia.)~~ **Chiusa (31/07, DEC-194):** la disclosure vive sulla **pagina negozio** e nei **crediti in gioco** (voce dedicata nel `MainMenu` col dettaglio di modelli, licenze Stability/Gemma, dataset CC0). Il dettaglio operativo esatto resta lavoro del dominio ai-production.
16. ~~Come arrivano i modelli all'utente: installer separato, download al primo avvio, strumento o DLC di piattaforma, oppure nessun modello distribuito? (DEC-070/DEC-086 fissano la scelta binaria al primo avvio e DEC-113/DEC-140 stabiliscono che i pesi non sono mai ridistribuiti col gioco — li scarica l'utente —, ma il meccanismo concreto di consegna non è deciso. Provenienza: `Q-DIST-002`.)~~ **Chiusa (31/07, DEC-195):** **download dal gioco al primo avvio**, per chi sceglie l'esperienza completa; chi sceglie "solo curato" non scarica nulla. Fonte e verifica dei pesi dichiarate dal gioco; mai ridistribuiti col gioco (DEC-113/DEC-140 invariate).
17. Il giocatore può importare contenuti propri — modelli, LoRA, prompt pack, rig, AudioSpec, skin dell'interfaccia — e con quali garanzie di validazione e originalità? (Provenienza: `Q-DIST-003`, priorità LATER, più la parte residua di `Q-IMG-001` sull'import di LoRA dell'utente; la sorte delle LoRA del progetto è invece decisa da DEC-148.) **Aggiornamento 31/07 — RIMANDATA, non decisa qui:** l'import di contenuti utente è rimandato esplicitamente a **dopo la release**, quando la pipeline generativa definitiva esisterà. Nessuna DEC di merito registra questa voce: è un rinvio, non una risposta.

## Revisione finali della maratona di implementazione (2026-07-28)

La sessione di revisione finale ha registrato le 10 domande accumulate durante l'implementazione della demo notturna (2026-07-27…28), provenienti da `scratchpad/questions-night.md`. Tutte le 10 risultano già risolte dalle DEC citate nel loro stesso testo:
- DEC-141 (reset rapido R: stessa run vs. run nuova) — approved, default proposto registrato in known-issues.md voce 3
- DEC-170 (5 domande: dimensione cella, distribuzione taglie, taglia boss, quantità piano, telecamera L) — approved, default proposti registrati in systems/rooms-and-floor-generation.md
- DEC-167 (2 domande: importi valuta, negozio alla visita) — approved, default proposti registrati in systems/rewards-and-economy.md e systems/shops-and-merchants.md
- DEC-161 (conflitto sinergie) — approved, default proposto registrato in systems/synergies.md
- DEC-162 (budget dedicato risultato) — approved, default proposto registrato in systems/synergies.md

**Risultato:** 0 nuove domande aperte dalla sessione; tutte le 10 sono duplicate già chiuse. Le domande veramente aperte restano 21 (voci 1-11, 13-22), come sotto.

---

## Produzione AI e asset

18. Quali body plan si realizzano per primi e con quale approccio si anima il personaggio giocante (rig modulare con skin, spritesheet completo, ibrido, stickman fino alla vertical slice)? (`docs/ai-production/09-NEMICI-BODY-PLAN-RIG.md` propone senza decidere. Provenienza: `Q-ANIM-001` e `Q-ANIM-002`.)
19. ~~Quante immagini originali il proprietario può realisticamente produrre e rivedere per i **dataset definitivi**, quale regime di review manuale vale per gli asset (approvazione umana sempre, solo per la release, a campione) e serve uno strumento interno di approvazione/rifiuto già nella pre-alpha? (DEC-148 stabilisce che i dataset definitivi li crea il proprietario, non la loro ampiezza né il regime di review. Provenienza: `Q-IMG-003`, `Q-IMG-004`, `Q-F0-002`.)~~ **Chiusa sul regime di review (31/07, DEC-201):** tutto ciò che entra nel curato definitivo e nei dataset LoRA passa **sempre** dall'approvazione esplicita del proprietario; gli asset provvisori della demo si giudicano a posteriori al playtest e **non entrano mai** nei dataset. La quantità di immagini producibili e l'eventuale strumento interno di approvazione/rifiuto restano dettagli operativi non fissati da questa decisione.
20. ~~Il primo esperimento audio riguarda solo SFX, solo musica, o entrambi con due milestone separate? (DEC-109 fissa pipeline e catena di fallback, non l'ordine degli esperimenti. Provenienza: `Q-AUD-002`.)~~ **Chiusa (31/07, DEC-196):** **SFX e musica insieme, un'unica milestone** — scelta esplicita del proprietario, diversa dalla raccomandazione di default "SFX prima", motivata dal voler valutare in un solo giro la coerenza dell'intera pipeline Stable Audio Small.
21. ~~Qual è il budget cloud disponibile e quante ore settimanali il proprietario può dedicare a domande, review di codice e asset, ascolto audio e playtest? (DEC-168 porta il training sulle 30 ore settimanali gratuite di Kaggle e declassa il runbook RunPod a fallback a pagamento, senza impegnare alcun budget. Serve a dimensionare i batch di lavoro. Provenienza: `Q-BUD-001` e `Q-BUD-002`.)~~ **Chiusa (31/07, DEC-204):** il budget cloud restava già fissato da DEC-168 (30 ore gratuite Kaggle/settimana, nessun budget a pagamento impegnato); il proprietario dedica **10 o più ore a settimana** a review, verdetti, playtest e decisioni. I batch di lavoro si dimensionano su questo ritmo pieno.

---

## Consumo del pacchetto artistico nel motore (W8, 2026-07-30)

Cinque domande aperte dallo stesso giro di lavoro sul consumo del pacchetto W8: quattro
dall'aggancio degli asset di `assets/art/` al motore, una dal gating del timer di run
(DEC-051) chiuso nello stesso lavoro in `known-issues.md` voce 10 punto 5 (WP1). Ognuna ha
già un **default proposto** implementato (stile DEC-019): il gioco funziona, ma il numero
o la politica non sono canone e vanno confermati o spostati.

~~23. A quale piano scatta la variante di **escalation** del tileset? DEC-024 chiede che il
    tema si intensifichi piano dopo piano sull'asse aspetto, e il contratto d'arte emette
    tre ruoli dedicati (`floor_deg`/`wall_deg`/`void_deg`, "crepe di brace",
    `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md`), ma nessun documento fissa la
    soglia. *Default proposto e implementato*: dal **piano 3**, cioè lo stesso confine
    della seconda traccia di gameplay (`AUDIO_GAMEPLAY_1_MAX_FLOOR`) e del passaggio dei
    boss a due fasi (DEC-028/106) — far coincidere i tre assi dell'escalation su un solo
    confine è l'ipotesi più leggibile per il giocatore. Da confermare al playtest.
    (`ROOM_TILESET_DEGRADED_FROM_FLOOR`, `src/render/game_renderer.c`.)~~ **Chiusa (31/07,
    DEC-191):** confermato dal piano 3 — i tre assi dell'escalation (aspetto, audio,
    nemici/boss) coincidono di proposito sullo stesso confine.
24. ~~I **volumi audio devono persistere** fra un avvio e l'altro, e in quale forma? W8
    espone i tre slider in `Options` ma il gioco non ha un file di configurazione:
    inventarne uno avrebbe voluto dire decidere da soli percorso, formato e politica di
    migrazione. *Default proposto e implementato*: nessuna persistenza, si riparte da 1.0
    ad ogni avvio. La domanda vera è più ampia dei volumi — serve un file di
    preferenze del giocatore, e se sì dove vive (accanto a `catalog/`?) e con quale
    schema versionato. (`docs/design/ui/options-and-accessibility.md` elenca "audio" fra
    le categorie minime senza fissare né slider né persistenza.)~~ **Chiusa (31/07,
    DEC-189):** il file di preferenze si fa — `prefs/settings.txt`, accanto a `catalog/`,
    chiave=valore con campo versione, disciplina zero-default come `catalog/`/`suspend/`.
    Primo contenuto: i tre volumi audio. L'implementazione concreta resta un gap dichiarato.
25. ~~Passo, etichette e ordine degli **slider di volume** sono canone o solo un default?
    *Default proposto e implementato*: tre righe nell'ordine `Volume generale` / `Musica`
    / `Effetti`, passo del 10% su dieci caselle, valore mostrato anche in percentuale
    (DEC-058). Le altre quattro categorie minime del documento (video, controlli,
    accessibilità, gameplay) restano da scrivere e W8 non le ha inventate.~~ **Chiusa
    (31/07, DEC-190):** i default sono canone.
26. ~~La **card di scoperta** va in alto al centro o in basso al centro? `ui/hud.md` dice
    "un riquadro in alto al centro (fuori dai quattro angoli)", ma quella formulazione
    descriveva l'HUD a quattro cluster con riquadro, che il layout V3 approvato al CP2 ha
    sostituito — e in V3 la quota alta a destra è occupata dalla riga piano/mondo.
    *Default proposto e implementato*: **basso al centro**, come il mock V3. Da
    confermare, e in ogni caso `ui/hud.md` va allineato alla scelta.~~ **Chiusa (31/07,
    DEC-186):** basso al centro, layout V3. `ui/hud.md` allineato — "alto al centro"
    resta accurata solo per il ripiego integrale senza pacchetto artistico.
27. ~~Il **timer di run non deve correre nel Piano 0**? DEC-051 fissa il timer sempre
    visibile durante il gameplay ma non si pronuncia sul crogiolo, che nel motore usa la
    stessa `PHASE_PLAY` del gameplay vero per restare esplorabile mentre le proposte
    girano in sottofondo (M1b). *Default proposto e implementato*: **il cronometro viene
    azzerato all'ingresso nel Piano 0 e resta fermo a zero** per tutta la permanenza —
    `FloorZeroEnter` spegne `game->inRealRun` (che blocca l'accumulo) e riporta
    `runElapsedSeconds` a zero (che cancella il tempo della run precedente: senza
    quest'ultimo il crogiolo mostrerebbe congelato il tempo dell'ultima run dalla seconda
    visita in poi). Il conteggio riparte da zero solo con `GameResetRunWithSeed`, cioè
    dopo l'attraversamento del varco verso il piano 1. Da confermare al playtest —
    l'alternativa scartata (farlo accumulare anche nell'hub) penalizzerebbe chi si ferma a
    leggere le carte-proposta prima di scegliere. (`src/game/game.c`,
    `src/world/floor_zero.c`.)~~ **Chiusa (31/07, DEC-192):** confermato — fermo per tutta
    la permanenza nel Piano 0, incluse le simulazioni delle arene.

## Salute temporanea/protettiva nel motore (WP2, 2026-07-30)

28. Quali sono il **tetto** e la **fonte in-run** della salute temporanea/protettiva
    (Crust, DEC-008)? Il documento fissa composizione (salute base + Crust) e ordine di
    consumo (prima il Crust, poi la base) ma non un valore numerico di tetto né una fonte
    concreta — entrambi restavano "draft" prima di WP2. *Default proposto e implementato*:
    tetto GLOBALE `PLAYER_TEMP_HP_CAP = 4` (4 icone `heart_temp` nell'HUD, un punto di
    `tempHp` per icona, non variabile per personaggio come il tetto di salute base
    DEC-033, perché nessuna DEC chiede quella variazione per questo strato); fonte
    scelta per la demo il **negozio** — 40% di probabilità per piano di tenere Crust in
    banco (stessa tecnica hash-based del Flux, DEC-022, mai `game->rng`), costo 25
    monete per 2 punti di `tempHp` (2 icone). Da confermare al playtest o da
    promuovere a decisione — nessuna delle altre fonti previste da DEC-008 per la salute
    base (stanze/oggetti/eventi) è stata esclusa, solo non ancora scelta per il Crust.
    (`src/core/game_types.h`, `src/gameplay/combat.c`, `src/world/world.c`; vedi
    `systems/health-and-resources.md`, sezione "Default proposti dall'implementazione".)

## Ostacoli distruttibili e pericoli passivi nel motore (WP3, 2026-07-30)

Una domanda aperta dall'implementazione degli ostacoli a tema di
`systems/secrets-and-obstacles.md` (famiglie solido/distruttibile/pericolo, DEC-043): il
documento fissa il principio del budget di difficoltà condiviso e la richiesta di telegraph
leggibile, ma lascia esplicitamente aperti (sezione "Domande aperte residue" dello stesso
documento) sia la proporzione blocchi/pericoli sia i valori numerici del budget condiviso.
Ha già un **default proposto** implementato (stile DEC-019): il gioco funziona, ma i numeri
non sono canone.

29. Qual è la **proporzione esatta** fra ostacoli solidi, distruttibili e pericoli passivi
    telegrafati nella generazione a tema, e quale il **costo esatto** che ogni ostacolo
    sottrae al budget nemici condiviso (DEC-043)? Il documento fissa solo il principio (due
    famiglie generate a tema oltre al solido, più aggressive nei piani alti; budget
    condiviso fra ostacoli e nemici), non i numeri. *Default proposto e implementato*:
    probabilità **piatta** (non scalata col piano) del 35% che un blocco generato diventi
    distruttibile; probabilità di pericolo che parte all'8% al piano 1 e cresce del 5% per
    piano (DEC-024, degenerazione del tema), fino a un tetto del 40%; il resto resta solido
    (comportamento di sempre, zero-default). Costo nemici: 0.18 punti di budget per ogni
    ostacolo della stanza (di qualunque famiglia, celle-buco di una L escluse), mai sotto
    0.35 punti residui — la stessa soglia minima di costo di un singolo nemico, così il
    budget non azzera mai la presenza di nemici (`secrets-and-obstacles.md`, "Casi limite").
    Danno di contatto di un pericolo: 1 punto, dentro gli i-frames esistenti; i nemici lo
    ignorano (interpretazione scelta fra le due ammesse dal documento, "ignorano o evitano"
    — nessuna euristica di pathing per "evitare" esiste ancora nel motore). Da confermare al
    playtest. (`OBSTACLE_DESTRUCTIBLE_CHANCE`, `OBSTACLE_HAZARD_CHANCE_BASE/PER_FLOOR/MAX`,
    `WORLD_OBSTACLE_ENEMY_BUDGET_COST/FLOOR`, `src/world/world.c`, `src/gameplay/combat.c`;
    vedi `systems/secrets-and-obstacles.md`, sezione "Default proposti
    dall'implementazione".)

    **Aggiornamento 30/07 (revisione WP3):** due precisazioni ulteriori, stesso stato di
    default proposto. (1) La persistenza dei distruttibili spaccati è oggi infrastruttura
    non ancora osservabile in gioco (una stanza di combattimento perde comunque tutti i suoi
    ostacoli quando si ripulisce, e la porta resta bloccata finché non si ripulisce): vedi
    `docs/engineering/known-issues.md` voce 11. (2) `CombatExplodeAt` apre i distruttibili
    per qualunque esplosione di origine GIOCATORE (bomba, colpo/attivo con trait Esplosivo),
    non solo per la bomba in senso stretto — un parametro `breach` esplicito impedisce
    comunque che un'ipotetica esplosione di origine nemica apra un varco.

## Stanza di fusione nel motore (WP4, 2026-07-30)

Una domanda aperta da `systems/special-rooms.md`, che lascia esplicitamente in coda
("Domande aperte residue") la frequenza esatta di ciascun archetipo speciale per piano —
qui solo per l'archetipo appena piazzabile nel motore, la stanza di fusione.

30. Qual è la **frequenza esatta** con cui la stanza di fusione (e gli altri archetipi
    speciali) compare per piano? Il documento fissa solo che l'archetipo esiste, non quante
    volte per piano o con quale probabilità. *Default proposto e implementato*: un solo
    tentativo di piazzamento per piano, stesso algoritmo di tesoro/negozio
    (`WorldPlaceSpecialRoom`, `src/world/world.c`) — non garantito (nessun piazzamento se la
    griglia è satura o se ogni cella libera tocca solo la stanza boss), mai adiacente alla
    stanza boss (DEC-182), deterministico dal seed del piano. Misurato su 120 piani generati
    (5 piani × 24 semi, `--rooms-test`): piazzata in 119 casi su 120. L'accesso globale
    storico (TAB da Gameplay, voce dal PauseMenu) resta comunque sempre disponibile come rete
    di sicurezza, indipendentemente da questa frequenza. Da confermare al playtest.
    (`systems/special-rooms.md`, sezione "Default proposti dall'implementazione".)

~~31. Con `ROOM_FUSION` ora presente nel motore, dove deve vivere la CONFERMA della fusione:
    resta un'azione di `BuildScreen` (come oggi), o si sposta integralmente nella stanza di
    fusione, coerente col modello canonico descritto in
    [Inventory and Synergy Screen](../ui/inventory-and-synergy-screen.md) ("la fusione
    *eseguita* nella stanza di fusione... questa schermata come sola consultazione")?
    *Default proposto e implementato (WP4, 30/07)*: la conferma resta in `BuildScreen`,
    raggiungibile da TRE porte d'ingresso equivalenti — il crogiolo della stanza di fusione
    (che apre `BuildScreen` già pronta alla fusione), TAB da `Gameplay`, e la voce dedicata
    nel `PauseMenu` — nessuna delle tre è stata rimossa. Da confermare al playtest.
    (`systems/item-fusion.md`, "Domande aperte residue"; `ui/inventory-and-synergy-screen.md`,
    nota di implementazione.)~~ **Chiusa (31/07, DEC-187):** confermato — la conferma resta
    in `BuildScreen`, con le tre porte d'ingresso equivalenti; il crogiolo della stanza di
    fusione è segnale/tema, non l'unico varco.

## Stanza a tempo nel motore (WP5, 2026-07-30)

Due domande aperte da `systems/special-rooms.md`/`systems/rooms-and-floor-generation.md`
(frequenza per piano, già in coda per la fusione alla voce 30) e da
`governance/open-questions.md` voce 3 sopra (soglia/ricompensa) — qui il pezzo specifico
del QUINTO archetipo appena piazzabile nel motore, la stanza a tempo (DEC-051).

32. A quale piano MINIMO compare la stanza a tempo, e con quale frequenza? Il documento
    fissa solo "piani avanzati" (DEC-051), non un numero. *Default proposto e implementato*:
    **dal piano 3** (`WORLD_TIMED_ROOM_MIN_FLOOR`, `src/world/world.h`) — stesso confine già
    scelto per l'escalation del tileset (voce 23 sopra) e il passaggio dei boss a due fasi
    (DEC-028/106): allineare i tre assi su un solo confine è l'ipotesi più leggibile. Un solo
    tentativo di piazzamento per piano (stesso algoritmo di tesoro/negozio/fusione,
    `WorldPlaceSpecialRoom`), non garantito, mai adiacente alla stanza boss (DEC-182),
    deterministico dal seed del piano. Misurato su 120 piani generati (5 piani × 24 semi,
    `--rooms-test`; solo i piani 3-5, 72 tentativi, sono candidati): piazzata in 69 casi su
    72. Da confermare al playtest. (`systems/special-rooms.md`, sezione "Default proposti
    dall'implementazione".)

33. La soglia si misura dall'ingresso nel piano o dall'inizio della run? DEC-051 non lo
    fissa esplicitamente — il timer di run sempre visibile (voce 27 sopra, WP1) accumula
    dall'inizio della run, non del piano, ed è la fonte più ovvia da cui partire senza
    guardarci due volte. *Default proposto e implementato*: **dall'ingresso nel piano**
    (`Game.floorEntryElapsedSeconds`, catturato da `WorldStartFloor` come istantanea di
    `Game.runElapsedSeconds`) — l'alternativa scartata (dall'inizio della run) avrebbe reso
    la soglia via via più facile da mancare piano dopo piano per la sola durata dei piani
    precedenti, indipendentemente da quanto in fretta si gioca QUESTO piano: la soglia
    misurerebbe la run intera, non la stanza. Da confermare al playtest.
    (`systems/special-rooms.md`, sezione "Stato di implementazione: la stanza a tempo".)

34. Il timer di run mostrato nell'HUD (DEC-051, sempre visibile e cumulativo
    dall'inizio della run) NON permette di valutare la soglia PER PIANO della
    stanza a tempo (voce 33 sopra): il tempo trascorso nel piano corrente e la
    soglia numerica compaiono solo DENTRO la stanza a tempo, dopo che l'esito
    è già deciso, mai prima di entrarci. `ui/hud.md` (sezione "Timer di run
    sempre visibile") descriveva il timer di run come "il segnale con cui il
    giocatore valuta se raggiungere in tempo" l'archetipo — un'affermazione
    scritta prima che l'archetipo esistesse nel motore, e non più accurata
    così com'era. Serve un indicatore leggibile PRIMA dell'ingresso (tempo
    trascorso nel piano e/o soglia, non solo il timer cumulativo), o va
    accettato esplicitamente che l'unico segnale pre-ingresso resti quello
    visivo/posizionale (icona dedicata sulla minimappa, `known-issues.md`
    voce 12) e che soglia/tempo restino leggibili solo a stanza raggiunta?
    Non deciso qui: la nota di implementazione WP5 in `ui/hud.md` registra il
    gap senza sceglierlo. (`ui/hud.md`, sezione "Timer di run sempre visibile
    (DEC-051)"; `systems/special-rooms.md`, sezione "Stato di implementazione:
    la stanza a tempo".)

## Consumo del pacchetto artistico nel motore, seconda tranche (WP-INT, 2026-07-30)

Il lavoro che aggancia gli asset di CP4/CP5 (props di pickup/ostacoli, i tre sheet di
personaggio, l'estensione del font) chiude i buchi di `known-issues.md` voce 10 (#10.1/
#10.2/#10.3), ma introduce due scelte visive che nessun documento di design fissa.

~~35. Con `props/spuntoni` e `props/cassa` ora agganciati al motore, quale ALTERNANZA
    visiva mostra il pericolo passivo (`OBSTACLE_HAZARD`) fra i tag "retratti" ed "estesi",
    e quale prop veste il distruttibile (`OBSTACLE_DESTRUCTIBLE`) fra `props/vaso` e
    `props/cassa`, entrambi consegnati? `secrets-and-obstacles.md` fissa solo che il danno
    di contatto è costante ("nessun windup a tempo") e che il telegraph è sempre visibile
    dal primo frame, non la resa dei due tag del prop. *Default proposto e implementato
    (rivisto in seconda istanza, 30/07: la prima versione faceva alternare "estesi" e
    "retratti" nel tempo — bocciata perché mostrava "retratti" per 1 s ogni 2.4 s mentre
    il danno resta costante, la stessa finestra di sicurezza fasulla che la motivazione
    originale diceva di voler evitare)*: **SEMPRE "estesi"**, mai "retratti" — il tag
    "retratti" resta consegnato nell'asset ma inutilizzato in questo WP, riservato a una
    futura variante di pericolo davvero temporizzata (una trappola con una finestra di
    sicurezza reale), che oggi non esiste nel motore (`CombatResolveHazards` non ha alcun
    gate temporale). Il vero telegraph resta SEMPRE la sovrapposizione a bande di
    `DrawObstacleFamilyOverlay`, disegnata incondizionatamente. Per il distruttibile:
    **`props/cassa`**, non `props/vaso` — un contenitore di legno si legge come
    "distruttibile" in qualunque ambientazione del gioco (industriale, naturale, anomala)
    senza dipendere dal tema, mentre un vaso presuppone un arredo domestico/decorativo che
    non tutti i temi condividono. Da confermare al playtest. (`DrawObstacleFamilyProp`,
    `src/render/game_renderer.c`; vedi `systems/secrets-and-obstacles.md`, sezione
    "Default proposti dall'implementazione".)~~ **Chiusa sull'alternanza (31/07, DEC-198):**
    il proprietario vuole la variante davvero temporizzata SUBITO, non "sempre estesi" —
    cicli retratti/estesi con danno solo da estesi, animazione e danno sincronizzati. Questo
    SUPERA il default "sempre estesi"; l'implementazione (gate temporale su
    `CombatResolveHazards`) resta da fare, in coda. La scelta del prop distruttibile
    (`props/cassa`) non è toccata da questa decisione.

36. ~~Con tre spritesheet di personaggio ora agganciati al motore (`character/fonditrice`/
    `ashblade`/`bulwark`), quale sheet mostra il personaggio GENERATO per-run
    (DEC-014/DEC-037)? Nessun documento fissa una veste dedicata per il personaggio
    generato, e produrne una richiederebbe un quarto sheet mai realizzato. *Default
    proposto e implementato*: il personaggio generato mostra sempre **`character/
    fonditrice`** (`CharacterSheetKey`, `src/render/game_renderer.c`) — la stessa veste
    del personaggio 0 curato (Wayfinder). Solo l'alfa della tinta resta distintiva (il
    lampeggio di invulnerabilità): la palette del personaggio generato non colora lo
    sprite, stessa limitazione già vera per la rosa curata prima di WP-INT (uno sprite
    disegnato ha la sua palette dentro, moltiplicarla per un colore la sporcherebbe).
    Resta aperta la domanda più ampia se il personaggio generato debba avere in futuro
    una veste propria — richiederebbe generazione procedurale dello sprite, fuori scope
    di questo lavoro. Da confermare al playtest o da promuovere a decisione.
    (`systems/characters.md`, sezione "Default proposti dall'implementazione".)~~ **Chiusa
    (31/07, DEC-199):** confermato — sheet della Fonditrice, provvisorio dichiarato finché
    non esiste la Style LoRA, nessun palette-swap.

## Arena di sfida nel motore (WP6, 2026-07-30)

Il lavoro che porta nel motore la versione dell'arena di sfida **incontrata nel piano**
(`ROOM_ARENA`; l'accesso "best-of" dal Piano 0, DEC-004, resta fuori) apre quattro
domande che nessun documento chiude: `systems/special-rooms.md` fissa l'archetipo
("combattimento più impegnativo in cambio di ricompensa maggiore") e il caso limite
("mai un passaggio obbligato"), non i numeri né l'input.

37. A quale piano MINIMO compare l'arena di sfida, con quale frequenza e di quale taglia?
    Il documento non fissa nulla di tutto questo. *Default proposto e implementato*:
    **dal piano 2** (`WORLD_ARENA_ROOM_MIN_FLOOR`, `src/world/world.h`) — il piano 1 resta
    il primo contatto col mondo generato, l'arena è un'escalation volontaria che comincia
    subito dopo; confine deliberatamente DIVERSO da quello della stanza a tempo (piano 3),
    perché lì "dai piani avanzati" è parte della decisione DEC-051 e qui è solo frequenza.
    Un solo tentativo di piazzamento per piano, non garantito, **mai 1x1** (si provano
    2x2 → L → 1x2/2x1 e si rinuncia se nessuna entra: una 1x1 stretta mortificherebbe un
    combattimento maggiorato) e **sempre foglia del grafo** di adiacenza come la stanza
    boss (DEC-182), che è il modo strutturale di garantire "mai un passaggio obbligato".
    Misurato su 120 piani generati (5 piani × 24 semi, `--rooms-test`; solo i piani 2-5,
    96 tentativi, sono candidati): piazzata in 82 casi su 96. Effetto collaterale
    dichiarato e misurato: piazzando l'arena PRIMA delle speciali 1x1 (l'unica che chiede
    celle contigue) la stanza di fusione scende da 119/120 a 101/120 e la stanza a tempo
    da 69/72 a 40/72 — l'ordine inverso salverebbe quei due numeri ma ridurrebbe l'arena
    a 17/96. Da confermare al playtest. (`systems/special-rooms.md`, sezione "Stato di
    implementazione: l'arena di sfida nel piano".)

38. Quanto vale il "combattimento più impegnativo" dell'arena? Il documento dice solo
    "più impegnativo", non di quanto. *Default proposto e implementato*: budget nemici
    della stanza **×1.5** (`WORLD_ARENA_BUDGET_MULTIPLIER`, applicato dopo la scala per
    celle di DEC-170 e prima della riduzione per ostacoli di DEC-043, così resta un
    moltiplicatore della difficoltà che quella stanza avrebbe avuto come combattimento
    normale) e tipi di nemico portati alla **fascia alta della banda di potenza
    dichiarata** (`ENEMY_TYPE_POWER_MAX`, cioè dove `systems/enemies.md` colloca il
    Veterano — mai fuori dalla banda draft [0.7–1.35] di DEC-019). Tutto deterministico
    dal seed. Limite dichiarato: senza tipi generati (manifest vecchio o assente) l'arena
    sale di sola quantità, perché i quattro nemici storici non hanno manopole da alzare.
    Da confermare al playtest. (`systems/enemies.md`, "Stato di implementazione (WP6)";
    `systems/special-rooms.md`, "Default proposti dall'implementazione".)

39. Quanto vale la "ricompensa maggiore" dell'arena? `rewards-and-economy.md` fissa solo
    "superiore alla media di una stanza di combattimento equivalente non a rischio"
    (Scenario 2). *Default proposto e implementato*, su tre canali: **8 Ingots** di valuta
    di completamento (il doppio del combattimento, meno del boss); **l'oggetto di rarità
    migliore fra i tre candidati del piano** — la forma concreta scelta per "rarità minima
    alzata", preferita a un'estrazione pesata perché una sfida vinta non deve poter pagare
    un oggetto comune quando nel pool c'è una rara; **catalizzatore di fusione al 50%**,
    più del 35% del drop di boss perché l'arena è un rischio scelto — e questo chiude la
    terza delle tre fonti di Flux dichiarate da DEC-022, che nel motore ne aveva solo due.
    Tutto SOLO a sfida accettata e vinta: attraversare l'arena non è completarla (DEC-167).
    Da confermare al playtest. (`systems/rewards-and-economy.md`, "Pattern
    rischio/ricompensa dell'arena di sfida"; `systems/special-rooms.md`.)

40. Con quale TASTO si conferma la sfida dell'arena? Il documento chiede una "conferma
    esplicita prima di un'azione irreversibile" ma nessun documento fissa i tasti — stessa
    situazione già registrata per `E` (usa attivo), `G` (sgancia Innesto), `F` (fondi) e
    `C` (statistiche HUD). *Default proposto e implementato*: **X**, premuto **a contatto**
    col segnale della stanza (`PICKUP_ARENA_ALTAR`); premuto altrove non fa nulla, e il
    solo TOCCO del segnale non basta (camminarci sopra non è una conferma, e la sfida è
    irreversibile). Non si può riusare "conferma" (ENTER/SPAZIO) perché in Gameplay SPAZIO
    è già la bomba: una sfida senza ritorno non deve poter partire premendo il tasto con
    cui si spara. Resta aperta la domanda più ampia se questi cinque tasti debbano
    diventare una mappatura rivedibile dal giocatore. (`app/app_internal.h`, `AppInput`;
    `systems/special-rooms.md`, "Stato di implementazione: l'arena di sfida nel piano".)

## Pourhouse / scambio ad alto rischio nel motore (WP7, 2026-07-30)

Il lavoro che porta nel motore lo **scambio ad alto rischio** (`ROOM_POURHOUSE`, in-game
Pourhouse — DEC-136) apre tre domande che nessun documento chiude: `systems/special-rooms.md`
fissa l'archetipo, le categorie ammesse di prezzo e offerta e il principio del budget di
equità (DEC-044), non i numeri né la politica di rifiuto.

41. Con quale FREQUENZA e da quale piano compare la Pourhouse? Il documento dice che è un
    archetipo speciale, non quanto sia raro. *Default proposto e implementato*: **dal piano
    2** (`WORLD_POURHOUSE_ROOM_MIN_FLOOR`, `src/world/world.h`) — come l'arena, perché prima
    di possedere qualcosa la risposta della Pourhouse sarebbe quasi sempre «la colata è
    fredda» — e **non a ogni piano**: un tentativo di piazzamento solo quando l'estrazione
    del piano lo concede (`WORLD_POURHOUSE_ROOM_CHANCE_PERCENT` = 70%, tiratura dell'RNG del
    piano, quindi deterministica dal seed di run). Il 70% è la probabilità del *tentativo*,
    non del risultato: piazzandosi per ultima su una griglia 5x5 già occupata da boss, arena
    e quattro speciali 1x1, la stanza trova posto nel 44% dei casi anche con l'estrazione
    forzata al 100%. Misurato su 120 piani generati (`--rooms-test`; solo i piani 2-5, 96
    candidati): **piazzata in 27 casi su 96**, circa un piano candidato su quattro, ~73%
    delle run ne incontra almeno una. Taglia sempre 1x1, quinta chiamante di
    `WorldPlaceSpecialRoom`, mai adiacente a boss o arena. Da confermare al playtest.
    (`systems/special-rooms.md`, "Stato di implementazione: la Pourhouse";
    `systems/rooms-and-floor-generation.md`, "Quantità di piano".)

42. Quali sono i VALORI del budget di equità della puntata (DEC-044)? La decisione fissa il
    principio — offerta e prezzo devono equivalersi dentro un budget dichiarato — e le
    categorie ammesse, non i numeri. *Default proposto e implementato*: una **tabella di
    valori equivalenti** in punti di equità, ancorata alla valuta principale (1 Ingot = 1
    punto) e ai prezzi che il negozio già pratica (DEC-026), fonte unica in
    `systems/rewards-and-economy.md` — salute immediata 4, salute massima 14, Crust 12,
    strumento di breccia 4, strumento di apertura 5, Flux 30, oggetti 8/16/28/45 — con la
    regola simmetrica `|offerta − prezzo| ≤ max(4 punti, 20% dell'offerta)`. Una coppia
    fuori tolleranza è respinta e non viene mai proposta. Il rischio dell'archetipo non sta
    quindi in uno sconto ma in **cosa** si versa (salute, il tetto, un pezzo della build):
    l'equità nominale resta pari, l'irreversibilità no. Nella stessa voce rientrano due
    regole di contorno adottate dall'implementazione: una puntata non baratta mai una
    risorsa con sé stessa (Ingots per Ingots, Flux per Flux, oggetto per oggetto di pari
    valore), e **il Crust non paga mai un prezzo di salute** (DEC-008: è protezione, non
    valuta — l'ordine di consumo vale per il danno subito, non per un patto volontario;
    registrato anche in `systems/health-and-resources.md`). Da confermare al playtest.

43. Cosa succede alla puntata quando il giocatore la RIFIUTA: resta disponibile per un
    ritorno successivo o si brucia? Il documento dice solo che il rifiuto non deve avere
    penalità. *Default proposto e implementato*: **resta disponibile**. Non esiste un tasto
    «rifiuta» — si esce dalla porta, senza alcun costo — e tornando nella stessa stanza si
    ritrova la stessa identica puntata: solo l'accettazione la consuma. Bruciare l'occasione
    per aver esitato punirebbe l'esplorazione, e la conferma esplicita di DEC-058 serve a
    proteggere dall'azione irreversibile, non a trasformare l'indecisione in una. Corollario
    della stessa scelta: una puntata **fredda** (nessuna coppia pagabile, Scenario 3) si
    ricompone invece a ogni ingresso, così chi torna con qualcosa da versare trova il banco
    acceso. Da confermare al playtest. (`systems/special-rooms.md`, "Default proposti
    dall'implementazione" della Pourhouse.)

## Stanza segreta a due livelli nel motore (WP8, 2026-07-30)

Il lavoro che porta nel motore l'ultimo dei cinque archetipi speciali — la **stanza
segreta** a due livelli (`ROOM_SECRET`, DEC-025) — apre tre domande che nessun documento
chiude: `systems/secrets-and-obstacles.md` fissa i due livelli e la grammatica di scoperta,
`systems/special-rooms.md` colloca l'archetipo nella tassonomia, nessuno dei due fissa
numeri, geometria o ricompensa.

44. Con quale FREQUENZA e da quale piano compaiono i due livelli? DEC-025 dice che la
    super-segreta è più difficile da trovare, non quanto sia più rara. *Default proposto e
    implementato*: la segreta **normale** si tenta a **ogni piano dal piano 1**
    (`WORLD_SECRET_ROOM_MIN_FLOOR`, `src/world/world.h`) — è l'archetipo che insegna a
    leggere il mondo, una crepa in una parete, e ha senso incontrare quella lezione subito;
    la **super-segreta** dal **piano 2** e solo a estrazione concessa
    (`WORLD_SECRET_SUPER_MIN_FLOOR`, `WORLD_SECRET_SUPER_CHANCE_PERCENT` = 50%). Misura su
    120 piani generati (`--rooms-test`): normale in **36 casi su 120**, super-segreta in
    **13 su 96** candidati. Nella stessa voce rientra l'**ordine di piazzamento** e il suo
    prezzo, dichiarato e misurato: le segrete si piazzano dopo boss/arena/tesoro/negozio e
    prima di fusione/stanza a tempo/Pourhouse, perché il loro vincolo di posizione è il più
    stretto di tutte (serve una cella libera con una sola cella vicina) — piazzate per
    ultime la normale scendeva a 23/120 e la super a **0/96**, cioè un intero livello di
    DEC-025 non sarebbe mai esistito nel gioco vero. Il prezzo lo pagano le tre 1x1 che
    vengono dopo: fusione da 101/120 a 95/120, stanza a tempo da 40/72 a 30/72, Pourhouse
    da 27/96 a 17/96. Nessuna delle tre è necessaria a completare un piano. Da confermare
    al playtest. (`systems/special-rooms.md`, "Stato di implementazione: la stanza segreta";
    `systems/rooms-and-floor-generation.md`, "Quantità di piano".)

45. Quanto deve essere VICINA l'esplosione alla parete perché il varco si apra? Il
    documento dice che la segreta normale «si apre con lo strumento di breccia», non con
    quale precisione. *Default proposto e implementato*: la fascia sbrecciabile è larga
    esattamente quanto una porta (`DOOR_HALF*2`, centrata sul lato della cella — dove la
    porta comparirà davvero) e profonda **56 px** dentro la stanza
    (`WORLD_SECRET_BREACH_DEPTH`, `src/world/world.h`); col raggio di esplosione attuale
    (74 px) significa **addosso al muro giusto**, non «verso quella parete». L'alternativa
    scartata — accettare qualunque esplosione dentro la stanza — avrebbe reso l'indizio
    decorativo (bastava una bomba a caso per stanza) e banale la super-segreta, che
    dell'assenza di indizio fa tutta la sua definizione. Corollario della stessa scelta,
    dalla stessa riga di codice: il varco si apre **solo** da un'esplosione di ORIGINE
    GIOCATORE, sotto lo stesso parametro `breach` già dichiarato dal WP3 per i
    distruttibili. Da confermare al playtest. (`systems/secrets-and-obstacles.md`,
    "Default proposti dall'implementazione".)

46. Qual è la RICOMPENSA «degna del segreto», e in che cosa la super-segreta paga di più?
    I documenti chiedono una ricompensa proporzionata e una superiore per il livello 2,
    senza fissarle. *Default proposto e implementato*: **entrambi** i livelli lasciano un
    oggetto del pool del piano con la **rarità minima alzata** — il migliore dei tre
    candidati, **senza estrazione**, quindi identico a ogni rientro (è così che il
    contenuto risulta «assegnato una sola volta» senza consumarlo al primo ingresso, cosa
    che lo farebbe perdere a chi esce senza raccoglierlo) — più **6 Ingots** di «stanza
    segreta trovata» (DEC-167, `WORLD_ROOM_CURRENCY_SECRET`). La **super-segreta** aggiunge
    **1 catalizzatore di fusione** (`WORLD_SECRET_SUPER_FLUX`), versato direttamente al
    primo ingresso e non come pickup a terra: un pickup sarebbe stato raccoglibile
    all'infinito rientrando in una stanza che si può riattraversare quanto si vuole, o
    perdibile uscendo senza prenderlo. La superiorità del livello 2 è quindi un **salto di
    categoria** (la risorsa più rara del gioco, DEC-022) e non un numero di Ingots più
    grande. Nella stessa voce rientra la scelta di **scope** del lavoro: i rivelatori di
    DEC-127 (Innesti «sensore») NON sono stati implementati insieme alla stanza — il
    contenuto curato di ripiego non contiene oggi nessun Innesto, e introdurne la prima
    categoria avrebbe toccato ledger e test del contenuto curato (DEC-171), garanzie che
    con i segreti non c'entrano. La super-segreta resta trovabile per intuizione estrema,
    l'altra via che DEC-025 ammette esplicitamente; limite registrato in
    `docs/engineering/known-issues.md`, voce 14. Da confermare al playtest.
    (`systems/rewards-and-economy.md`, "Fonti canoniche della valuta principale";
    `systems/special-rooms.md`, "Default proposti dall'implementazione" della stanza
    segreta.)

## Prove specifiche della run nel motore (WP16, 2026-07-30)

Il canale bonus di DEC-027, presentato secondo DEC-042, apre tre domande che nessun
documento chiude: `systems/rewards-and-economy.md` fissa la struttura a doppio canale e
qualche esempio ("boss senza danni", "2 stanze segrete", "arena completata"), non un
catalogo chiuso né i numeri.

47. Quale CATALOGO di prove è verificabile col motore attuale, e con quale testo? Il
    documento dà solo esempi sparsi. *Default proposto e implementato*: un catalogo
    **curato e deterministico** di **otto** tipi (`TrialKind`, `src/core/game_types.h`) —
    boss senza danno, stanza segreta trovata, arena vinta, piano sotto soglia di tempo, fine
    run con almeno N Ingots, una fusione riuscita, stanza a tempo entro soglia, mai comprare
    al negozio — scelto perché ciascuno ha già un evento del motore che lo decide senza
    ambiguità (nessuna prova "quasi verificabile"). Nessuna prova GENERATA: il documento
    ammette "fisse o generate", qui solo la parte fissa, coerente col principio del work
    package di restare dentro ciò che il motore attuale garantisce. Testo in italiano,
    ironico-leggero (DEC-105), senza accentate (coerenza con lo stile esistente dei testi di
    gioco, non più un limite del font). Da confermare al playtest.
    (`systems/rewards-and-economy.md`, "Stato di implementazione: le prove specifiche".)

48. QUANTE prove per run, e come si scelgono? DEC-027 non fissa un numero.
    *Default proposto e implementato*: **2 o 3**, estratto anche questo da uno stream locale
    derivato dal seed di RUN (mai `game->rng`) — così anche il CONTEGGIO è deterministico dal
    seed come i tipi. Le prove assegnate sono sempre di tipo DIVERSO fra loro (mai due prove
    identiche nella stessa run); il piano bersaglio di `TRIAL_BOSS_NO_DAMAGE`/
    `TRIAL_FLOOR_UNDER_TIME` è estratto uniformemente in `[1, FLOOR_COUNT]`, che esiste sempre
    per costruzione (ogni piano generato ha sempre una stanza boss) — per questi due tipi
    l'esclusione dei parametri non verificabili che il documento chiede (Caso limite: "una
    prova ... impossibile ... va scartata") non ha oggi alcun caso su cui attivarsi AL
    MOMENTO DELL'ASSEGNAZIONE, ma resta un vincolo scritto esplicitamente nel codice perché è
    un requisito della decisione, non un effetto collaterale dell'assenza di casi limite di
    oggi. **Aggiornamento 30/07 (seconda tornata), correzione di un'affermazione precedente
    FALSA**: per `TRIAL_SECRET_FOUND`/`TRIAL_ARENA_WON`/`TRIAL_TIMED_ROOM_WITHIN_THRESHOLD` il
    caso ESISTE davvero (nessuno dei tre archetipi è garantito per costruzione, misure di
    `--rooms-test` in `docs/engineering/known-issues.md` voce 15 — la stanza a tempo manca in
    circa 1 piano su 5 fra i candidati, la segreta normale in circa 1 su 10), semplicemente
    non è verificabile AL MOMENTO DELL'ASSEGNAZIONE (generazione pigra dei piani: i piani 2-5
    non esistono ancora quando `TrialsAssignForRun` gira). L'esclusione per questi tre tipi si
    applica invece A FINE RUN (`TrialsFinalizeAtRunEnd`): una prova ancora in corso il cui
    archetipo non è mai comparso in nessun piano della run si scarta (nuovo stato
    `TRIAL_VOID`, "annullata"), non fallisce. Da confermare al playtest.
    (`systems/rewards-and-economy.md`, "Stato di implementazione: le prove specifiche";
    verificato da `--trials-test`, test (a), (k), (n).)

49. Quali sono i BONUS punti e le SOGLIE numeriche di ciascuna prova (Ingots richiesti,
    secondi per completare un piano)? Nessun documento fissa numeri. *Default proposto e
    implementato*: bonus da **10 a 25 punti**, alla stessa scala della valuta di stanza
    (`WORLD_ROOM_CURRENCY_*` va da 2 a 12) ma più alti, perché una prova vincola l'INTERA run
    e non un solo evento — boss senza danno il massimo (25, la più difficile: un solo colpo
    la chiude per sempre, senza un secondo tentativo sullo stesso piano); mai comprare al
    negozio (20) più delle altre prove "passive" perché è la più facile da rovinare per
    distrazione e la più difficile da recuperare, senza una seconda occasione. Soglia
    Ingots: **30**, poco sopra un acquisto comune (8, DEC-026) più margine, per chiedere di
    arrivare a fine run con una riserva vera. Soglia di tempo per piano: `90s + 25s × N`
    (piano N), più larga della soglia della stanza a tempo (WP5) perché qui si chiede di
    completare l'INTERO piano, non di raggiungere una sola stanza; non dipende dalla taglia
    vera del piano bersaglio perché al momento dell'assegnazione — l'ingresso nel piano 1 — i
    piani successivi non sono ancora generati. Tabella completa in
    `systems/rewards-and-economy.md`, "Stato di implementazione: le prove specifiche". Da
    confermare al playtest.

## Arene di sfida del Piano 0 nel motore (WP15a, 2026-07-30)

Il lavoro che porta nel motore le **arene di sfida opzionali del Piano 0** (DEC-004, con il
tutorial integrato di DEC-047, il rischio zero di DEC-055/092, l'assenza di economia di
DEC-093, il pool curato di DEC-087/094 e le prove illimitate di DEC-095) apre tre domande che
nessun documento chiude: `systems/floor-zero.md` fissa l'archetipo, le garanzie e i casi
limite, mai i numeri né gli input.

50. Quale criterio rende una run passata "migliore" ai fini dei contenuti **best-of** di
    un'arena, e quanti contenuti si pescano? Il documento dice solo "contenuti best-of già
    validati nelle run passate" e "basta un solo contenuto valido perché un'arena si apra"
    (DEC-094). *Default proposto e implementato*: una funzione di qualità chiusa e
    deterministica su ciascun record del catalogo —
    `10000 × (esito vittoria) + 100 × piano raggiunto + 50 × boss davvero sconfitti`
    (`RunCatalogBestOfEnemiesFromPath`, `src/content/run_catalog.c`) — e si pesca da **una
    sola** run, la migliore, non da un miscuglio: un'arena "best-of" deve sapere di
    qualcosa, non essere una macedonia di run diverse. L'esito domina di proposito su ogni
    altro termine: una vittoria corta è comunque un risultato migliore di una sconfitta
    lontana. Spareggio sul nome del file, mai sull'ordine di enumerazione della cartella,
    altrimenti la composizione dell'arena smetterebbe di essere deterministica. Un boss del
    catalogo entra come nemico normale: riaffrontare un boss è la prova dal **museo**
    (DEC-040), che non esiste ancora nel motore. Da confermare al playtest.
    (`systems/floor-zero.md`, "Stato di implementazione: le arene di sfida del Piano 0".)

51. QUANTE arene, con quali TEMI e con quale taglia d'ondata? DEC-047 elenca ciò che le
    arene devono insegnare — "movimento, sparo, risorse e fusione" — senza dire quante
    siano; DEC-004 non fissa nulla. *Default proposto e implementato*: **tre** piazzole
    segnalate nel crogiolo (`FloorZeroTrialTheme`, `src/core/game_types.h`) — movimento e
    tiro insieme perché sono lo stesso gesto continuo, risorse e bombe insieme perché la
    bomba *è* una risorsa spendibile, fusione da sola con due oggetti e un catalizzatore a
    terra perché la sua lezione si deve poter **compiere**, non solo leggere. Ondata di 3/2/1
    nemici (`FLOOR_ZERO_ARENA_ENEMIES_*`), volutamente piccola: l'arena del Piano 0 insegna,
    non mette alla prova come quella incontrata nel piano (budget ×1.5 e nemici in fascia
    alta, voce 38). Nessuna maggiorazione sui tipi: un contenuto best-of si riaffronta
    com'era. Da confermare al playtest.
    (`systems/floor-zero.md`, "Default proposti dall'implementazione".)

52. Con quali TASTI si entra e si esce da una simulazione del Piano 0? Nessun documento
    fissa i tasti — stessa situazione già registrata per `E`/`G`/`F`/`C` e per la conferma
    dell'arena del piano (voce 40). *Default proposto e implementato*: si **entra** con `X`
    a contatto con la piazzola, lo stesso tasto e lo stesso gesto dell'arena del piano e
    della Pourhouse — per il giocatore è sempre "accetto ciò che questo posto propone" — e
    il solo TOCCO non basta: entrare non è irreversibile, ma girando per l'hub non si deve
    finire dentro una simulazione per averci camminato sopra. Si **esce** con ESC, che
    dentro una prova significa "torna nell'hub" invece di aprire `ExitConfirm`; fuori dalla
    prova ESC resta `ExitConfirm` (DEC-074), invariato. Dentro una prova TAB apre la fucina
    (`BuildScreen`) come in `Gameplay`, non il pannello mondi/personaggi. Resta aperta la
    domanda più ampia se questi tasti debbano diventare una mappatura rivedibile dal
    giocatore. (`systems/floor-zero.md`; `app/app_internal.h`, `AppInput`.)

## Abbandono di una run in corso nel motore (WP19, 2026-07-30)

Il lavoro che corregge il flusso di abbandono (DEC-082/089, `ui/pause-menu.md`,
`ui/results-and-leaderboards.md`, `05-game-states-and-flow.md`): l'abbandono confermato di
una run VERA (piani 1-5) passa ora da `RunResults` come sconfitta, non più da `MainMenu`
diretto. Il **routing** e la **finalizzazione delle prove** sono la lettura diretta delle
regole già canoniche (DEC-082/089/WP16), non un default; la sola cosa che nessun documento
fissa è **come si presenta la causa** sulla schermata dei risultati.

53. Con quale meccanismo distinguere, in `RunResults`, "sconfitta per morte" (DEC-159,
    causa = ultimo colpo o nemico letale) da "sconfitta per abbandono volontario"? La
    tabella di `ui/results-and-leaderboards.md` elenca "Vittoria, sconfitta, o abbandono"
    come tre testi di esito possibili, ma DEC-159 dice esplicitamente che il suo campo
    "non compare... in caso di abbandono volontario, dove non c'è un colpo letale da
    dichiarare" — nessun documento dice se l'abbandono debba avere un titolo/esito TERZO
    e distinto ("ABBANDONO") o restare "SCONFITTA" con una causa dichiarata a parte.
    *Default proposto e implementato*: il titolo resta quello già esistente per ogni
    sconfitta ("SCONFITTA", derivato da `game->phase != PHASE_WIN` — l'abbandono non
    tocca mai `phase`, che resta `PHASE_PLAY`), e una riga dedicata indipendente,
    "Causa: abbandono volontario.", si aggiunge SOLO quando il nuovo campo
    `Game.runAbandoned` è vero (`src/core/game_types.h`, scritto da `APP_EXIT_CONFIRM`
    quando `game->floor >= 1`, letto da `DrawRunResultsOverlay`,
    `src/render/game_renderer.c`) — mai insieme alla riga DEC-159 (`deathCause` resta
    vuota per costruzione su questo percorso, i due campi si escludono a vicenda per
    come sono scritti). Scelto per continuità con l'impianto esistente (una riga "Causa:
    ..." già c'è per la morte, qui si aggiunge la stessa forma per l'abbandono) invece di
    un terzo valore di titolo, che avrebbe richiesto toccare ogni altro punto del codice
    che legge `game->phase == PHASE_WIN` per dedurre il titolo. Resta un default
    d'implementazione: la scelta esatta del testo e se meriti un titolo distinto è del
    proprietario. Verificato da `--states-test` (`src/tests/game_tests.c`),
    `--catalog-test` (`src/tests/catalog_tests.c`, test G) e `--arena-hub-test`
    (`src/tests/floor_zero_arena_tests.c`, blocco (m)).
    (`ui/results-and-leaderboards.md`, `ui/pause-menu.md`, `05-game-states-and-flow.md`.)

~~54. Riusare `TrialsFinalizeAtRunEnd` per un abbandono chiude le prove ancora in corso
    esattamente come a fine run vera — **anche in positivo**. Un giocatore che abbandona
    con `Player.coins >= 30` vede `TRIAL_END_WITH_INGOTS` (+10) diventare `TRIAL_PASSED`;
    uno che non ha mai comprato nulla vede `TRIAL_NO_SHOP_PURCHASE` (+20) diventare
    `TRIAL_PASSED` — due prove il cui testo chiede esplicitamente di **finire** la run,
    mostrate come superate a chi ha invece mollato. Nessun documento dice se questo sia
    accettabile o se l'abbandono debba invece far fallire SEMPRE queste due prove
    (indipendentemente dalla soglia/dall'acquisto). *Default proposto e implementato*:
    accettato — la disciplina "nessuna prova resta `TRIAL_IN_PROGRESS` per sempre" (WP16)
    prevale, e riusare la stessa funzione di finalizzazione evita un secondo percorso di
    chiusura da mantenere in parallelo. **Impatto dichiarato oggi: di sola presentazione**
    — nessun sistema di punti sblocco esiste ancora nel motore
    (`systems/save-and-meta-progression.md`), quindi il bonus mostrato in `RunResults` non
    si traduce ancora in un vantaggio spendibile. Se in futuro le prove alimenteranno la
    meta-progressione, questa combinazione (abbandono + soglia già raggiunta) andrà
    rivalutata come possibile incentivo a mollare la run invece di finirla. Resta un
    default d'implementazione: la scelta finale è del proprietario.
    (`systems/rewards-and-economy.md`.)~~ **Chiusa (31/07, DEC-203):** confermato —
    l'abbandono finalizza le prove come una fine run, anche in positivo.

## Il reroll a nuovo seed nel motore (WP21, 2026-07-31)

Il lavoro che chiude il gap dichiarato da DEC-114 ("oggi il tasto `R` rigenera
direttamente"): la voce "Rigenera la run" di `PauseMenu`, con `ExitConfirm` come conferma
esplicita, è oggi l'UNICA via per un reroll a seed nuovo — riusa la stessa
`AppEnterFloorZero` di `RunSetup`/Avvia e `RunResults`/"Nuova run subito", nessuna
generazione duplicata. Il tasto `R` in `Gameplay` chiama oggi sempre e soltanto
`game->resetQueued`, mai un seed nuovo: il gap è chiuso.

~~55. Il tasto `R` in `Gameplay` resta, invariato da prima di questo lavoro (fuori dal
    mandato di WP21), un reset rapido della STESSA run allo STESSO seed: butta via
    l'intero progresso della run corrente (piani percorsi, oggetti raccolti, Innesti)
    SENZA alcuna conferma. Nessuna decisione approvata autorizza oggi questo
    comportamento come funzione di gioco rivolta al giocatore finale — anzi DEC-114 (che
    parla del reroll a nuovo seed, non di questo) dice il principio opposto: "nessun
    tasto rapido diretto: buttare una run per un tasto sbagliato è il caso peggiore". Il
    commento che lo introduceva nel codice, rimosso da questo lavoro, lo chiamava "il
    reset rapido dev di sempre" — un residuo dei comandi di sviluppo, non una funzione
    di gioco mai formalizzata da una DEC. *Default proposto dall'implementazione (stile
    DEC-019), non canone*: `R` resta un'eccezione dichiarata alla regola "nessuna azione
    distruttiva immediata" (vedi `ui/pause-menu.md`, sezione "Regole"), documentata come
    tale invece di rimossa o portata dietro conferma, per continuità con tutta la suite
    di test esistente che la usa come scorciatoia sintetica (`--rng-seed-test` e il resto
    di `src/tests/game_tests.c`) e perché WP21 ha mandato esplicito solo sul reroll a
    nuovo seed. Il proprietario decide se: (a) formalizzare `R` come funzione di gioco
    invariata (serve una DEC che lo autorizzi esplicitamente senza conferma, in deroga al
    principio di DEC-114); (b) portarlo anch'esso dietro conferma; (c) rimuoverlo dalla
    build di gioco e tenerlo solo come comando di sviluppo/debug (es. dietro un flag di
    build, fuori da `AppInputCollect`). Nessuna delle tre è ancora scelta.
    (`ui/pause-menu.md`, DEC-114, DEC-141, `docs/engineering/known-issues.md`.)~~ **Chiusa
    (31/07, DEC-188):** opzione (c) — `R` è un comando di sviluppo, escluso dalla build di
    gioco finale, e resta attivo nella demo per il playtest. Il reroll di gioco è solo
    "Rigenera la run" dal `PauseMenu` con conferma (DEC-114). Escludere `R` dalla build
    finale resta un gap di implementazione da fare.

## Tre rifiniture UI nel motore (WP22, 2026-07-31)

ExitConfirm è stato corretto in una seconda e in una terza passata lo stesso giorno, dopo
due bocciature del giudice (dettagli sotto).

Lavoro che chiude tre gap piccoli e circoscritti, uno per dominio (`ui-cornice` G9/G10,
`ui-gioco` G8): il dialogo `ExitConfirm` aperto da `MainMenu` (chiusura del gioco) è ora un
overlay leggero DAVVERO — `MainMenu` resta disegnato sotto (con l'ultimo focus reale, non
quello di `ExitConfirm`, passato come parametro diretto e non più come copia dell'intera
`AppUi`, 337KB misurati inutilmente duplicati/copiati a ogni frame nella prima passata) —
mentre gli **altri tre** contesti (abbandono della preparazione nel Piano 0, abbandono di
una run in corso da `PauseMenu`, rigenerazione della run di WP21/DEC-114) restano a schermo
pieno e con la larghezza di pannello di sempre, come DEC-090 richiede esplicitamente; la
riga "Modalità: Standard" di `RunSetup` risultava già disegnata (non selezionabile, dalla
M1a del 18/07) ma **si sovrapponeva alla riga "Seed"** ed era **priva di qualunque test** —
la terza passata l'ha spostata in una quota libera e coperta con
`--run-setup-mode-line-test`; il focus iniziale di `BuildScreen` va ora davvero sull'ultimo
oggetto acquisito (`AppEnterBuildScreen` impostava solo un clamp, non un vero "vai
all'ultimo"), verificato con un pickup reale (`CombatUpdatePickups`), coerente col testo
già canonico di `ui/inventory-and-synergy-screen.md` — nessuna nuova domanda lì, solo un
bug corretto.

La **prima** passata di ExitConfirm era stata bocciata (due difetti misurati): (a) il
MainMenu ridisegnato sotto usava comunque il proprio velo a schermo pieno (190/255) SOPRA
cui si sommava il velo leggero di ExitConfirm (90/255), risultando in un composito
(213/255) **più scuro** del 190/255 di prima di WP22, l'opposto dell'intento; (b) il box
di ExitConfirm riusava la STESSA geometria di MainMenu (`MenuBoxForModeFor` restituiva 600
di larghezza per entrambi), quindi il riquadro di conferma copriva il menu per intero e
nessun punto dello schermo restava leggibile. La seconda passata corregge entrambi:
`DrawMainMenuOverlay` accetta ora un parametro `dimBackground` e, quando disegna se stesso
come sfondo di ExitConfirm, lo passa `false` (nessun velo proprio: l'unico velo applicato
sull'intero schermo resta quello, più chiaro, di ExitConfirm sopra); `MenuBoxForModeFor`
restituisce 460 di larghezza invece di 600 — il margine risparmiato (140, 70 per lato)
supera il margine orizzontale delle righe di menu (60), quindi un bordo di ciascuna riga
del MainMenu resta visibile ai due lati del dialogo di conferma più stretto. Verificato non
più solo dal nucleo puro `ExitConfirmIsLightModalFor` (`--layout-test`/`--states-test`, che
non dicono nulla sul frame vero) ma anche da `--exit-confirm-light-modal-test`
(`GameExitConfirmLightModalTest`, `src/tests/game_tests.c`): campiona pixel di
`RendererDrawApp` REALI per verificare che il velo fuori dal box sia più chiaro nel
contesto leggero che in quello a schermo pieno sulla stessa scena, che il colore dentro il
box di MainMenu ma fuori da quello di ExitConfirm sia nettamente diverso da quanto ci
sarebbe senza pannello dietro (croma soppressa dal pannello quasi neutro, non solo dal
velo), e — nucleo puro via `RendererMenuItemAt` — che una riga di MainMenu resti
geometricamente fuori da ogni voce di ExitConfirm.

Anche la **seconda** passata è stata bocciata, per tre residui, chiusi dalla **terza**:

1. **Geometria fuori scopo.** I 460 si applicavano ad `APP_EXIT_CONFIRM` in tutti e
   **quattro** i contesti, non solo al dialogo leggero. Nei tre a schermo pieno — che
   DEC-090 vuole invariati — la domanda, disegnata da una sola `UiText` senza andare a
   capo, sconfinava dal pannello molto più di prima: misure col font reale
   (`assets/art/ui/font-5px.json`, `UiFontScale(16)=3`, `uiScale` 1) 765 px ("Abbandonare
   la run in corso?…"), 849 px ("Abbandonare la preparazione?…") e 864 px ("Rigenerare la
   run con un nuovo seed?…") contro 380 px di spazio utile in un box da 460 (erano 520 in
   uno da 600). Corretto così: `MenuBoxForModeFor` prende un parametro `exitConfirmLight`
   (falso = geometria di sempre, il valore più innocuo) che viaggia insieme a `mode` per
   tutta la catena della geometria fino a `RendererMenuItemAt`, così **disegno e hit-test
   del mouse restano la stessa geometria**; i tre contesti a schermo pieno tornano a 600 e
   solo il dialogo leggero resta a 460. In più la domanda va ora **a capo** con
   `WrapTextLines` (fino a tre righe, quota 52, passo 20, tutte sopra la prima voce a 110):
   a 1920x1080 il testo del contesto a schermo pieno arriva a 519 px dal bordo sinistro del
   pannello e finisce **dentro** il riquadro, contro i ~304 px di sbordo che aveva anche
   prima di WP22.
2. **Affermazioni false.** Il commento di `MenuBoxForModeFor` sosteneva che il testo più
   lungo degli altri contesti "non ha bisogno di più larghezza": falso e misurabile come
   falso, riscritto con i numeri veri. Le note in `05-game-states-and-flow.md` e la voce 56
   qui sotto dicevano che gli "altri due" contesti restavano invariati: i contesti sono
   quattro e la loro geometria era cambiata — entrambe corrette.
3. **Test mancante.** La riga "Modalità: Standard" di `RunSetup`, richiesta dalla specifica
   del work package, non aveva alcuna copertura (`grep -rn 'Modalit' src/tests/` = 0
   riscontri): cancellare quella `UiText` lasciava `make test` interamente verde. Ora
   `--run-setup-mode-line-test` (`GameRunSetupModeLineTest`) fallisce sia se la riga
   sparisce (conta i pixel chiari della sua fascia in un frame vero, contro una fascia
   vuota di controllo) sia se diventa selezionabile o torna a sovrapporsi a una voce
   (`RendererMenuItemAt` su tutta la fascia), e `--layout-test` (voce `h`) ripete la
   verifica geometrica come nucleo puro su sette risoluzioni.

56. Quanto deve scurirsi lo sfondo dietro il dialogo leggero `ExitConfirm`/`MainMenu`
    perché il menu sotto resti "visibile/leggibile" (DEC-090) e non solo "intuibile"? Il
    documento (`05-game-states-and-flow.md`) non fissa alcun numero, solo l'intento
    descrittivo. *Default proposto dall'implementazione (stile DEC-019), non canone*: un
    velo a 90/255 di opacità (contro il 190/255 usato da ogni altro overlay di menu a
    schermo pieno, incluse le altre TRE presentazioni di `ExitConfirm`), applicato UNA SOLA
    VOLTA sull'intero schermo (il MainMenu ridisegnato sotto non aggiunge un proprio velo,
    vedi sopra) — e, **solo per questo contesto**, una geometria del box DEDICATA e più
    stretta (460 contro i 600 di MainMenu, `MenuBoxForModeFor`): la "leggibilità dietro"
    che questo offre è quindi reale sia nel contorno dello schermo attorno al box (dove
    prima c'era un nero quasi pieno, ora un velo più chiaro) sia in un margine di ciascuna
    riga di MainMenu ai due lati del riquadro di conferma più stretto, non solo nel grado
    di trasparenza del riquadro stesso. Gli altri tre contesti (abbandono dal Piano 0,
    abbandono di una run in corso, rigenerazione della run di DEC-114) **conservano i 600
    di sempre**: DEC-090 li vuole invariati, e la seconda passata di questo lavoro li aveva
    stretti anche loro, facendo sconfinare la domanda dal pannello. *Secondo default
    proposto (stessa natura, aggiunto dalla terza passata)*: la domanda di `ExitConfirm` va
    **a capo** dentro il pannello (`WrapTextLines`, larghezza utile `box.width - 80`, fino
    a tre righe a partire da 52 con passo 20, tutte sopra la prima voce a 110) invece di
    restare una riga sola che usciva dal riquadro in tutti e quattro i contesti — difetto
    presente anche prima di WP22, non introdotto da esso. Il documento non fissa requisiti
    pixel-per-pixel più precisi (es. quante righe intere di MainMenu debbano restare
    visibili, non solo un margine; né se la domanda debba essere centrata invece che
    allineata a sinistra): un layout "a finestra" con più righe intere visibili attorno al
    dialogo resterebbe un affinamento ulteriore, non richiesto qui. Verificato da
    `--layout-test` (nucleo puro: `ExitConfirmIsLightModalFor`, geometria delle voci in
    entrambe le larghezze, box leggero più stretto di quello di MainMenu e box a schermo
    pieno uguale ad esso), `--states-test` (i contesti restano distinti nel routing reale)
    e `--exit-confirm-light-modal-test` (frame vero campionato a pixel,
    `GameExitConfirmLightModalTest`, `src/tests/game_tests.c`) — quest'ultimo è quello che
    verifica davvero il valore di opacità, la geometria e il fatto che la domanda resti
    dentro il bordo destro del pannello, non solo il nucleo puro.
    (`05-game-states-and-flow.md`, DEC-090, `src/render/game_renderer.c`.)

57. Dove va la riga informativa "Modalità: Standard" di `RunSetup`? `ui/run-setup.md` la
    elenca fra gli elementi visibili ("Sempre", non selezionabile) ma non ne fissa la
    posizione. Fino alla seconda passata di WP22 era disegnata a `box.y + 142`, cioè
    **dentro** la fascia della voce "Seed" (110..150): si sovrapponeva al bordo inferiore
    di una riga selezionabile, e nessun documento lo autorizzava. *Default proposto
    dall'implementazione (stile DEC-019), non canone*: la riga sta a `box.y + 78`, nella
    fascia libera fra il filetto del titolo (che finisce a 30) e la prima voce (110) —
    sopra le tre voci, non in mezzo — con 48px di margine sopra e 17px sotto a ogni
    risoluzione. Il proprietario resta libero di volerla altrove (per esempio subito sotto
    la riga "Seed", che però richiederebbe di allontanare le voci fra loro: oggi il passo
    fra due righe è 52 e lascia solo 12px liberi). Verificato da `--run-setup-mode-line-test`
    (frame vero campionato a pixel più `RendererMenuItemAt` su tutta la fascia) e da
    `--layout-test` (voce `h`, nucleo puro su sette risoluzioni).
    (`ui/run-setup.md`, DEC-038, `src/render/game_renderer.c`.)

~~58. **La sospensione della run (DEC-050): percorso del file, stato dell'RNG salvato, che
    cosa significa "l'ingresso" della stanza, e nessun salvataggio automatico.**~~ **Chiusa
    (31/07, DEC-202):** i cinque default WP17 sono confermati in blocco, senza eccezioni —
    vedi il testo originale sotto per il dettaglio di ciascuno.

    DEC-050 e
    `systems/save-and-meta-progression.md` fissano il COMPORTAMENTO (si sospende in
    qualunque momento; al rientro la stanza corrente riparte dall'ingresso coi nemici
    ripristinati, il resto della run riprende com'era) ma nessun dettaglio tecnico. WP17
    (2026-07-31) ne implementa cinque come *default proposti dall'implementazione (stile
    DEC-019), NON canone*:
    - **Percorso e molteplicità**: un solo file, `suspend/current.txt`, accanto a
      `catalog/` e con le stesse regole (dati del giocatore, mai versionato, tmp+rename
      atomico, formato chiave=valore con `suspendSchema=1` come campo versione). Una sola
      sospensione per profilo, che è esattamente quello che la voce "Continua" sa
      esprimere: una voce, non una lista di salvataggi. **Nota**: questo è un SALVATAGGIO
      DI RUN, dominio di `systems/save-and-meta-progression.md` — la domanda aperta 24 sul
      file di PREFERENZE resta aperta e distinta, e questo lavoro non la tocca.
    - **Si salva lo stato dell'RNG di gioco**: `game->rng` al momento della sospensione,
      più `Game.floorEntryRng` (il valore d'ingresso nel piano, nuovo campo scritto da
      `WorldStartFloor`). Senza il secondo la mappa del piano corrente non si rigenera
      identica; senza il primo la sequenza di gioco divergerebbe alla ripresa.
      L'alternativa — accettare la divergenza e dichiararla — era più semplice ma avrebbe
      reso una run ripresa non più confrontabile con la stessa run giocata di fila
      (`systems/run-manifest-and-reproducibility.md`, DEC-141).
    - **"L'ingresso" della stanza è il suo baricentro** (`WorldRoomCenter`), lo stesso
      punto in cui il giocatore compare entrando in un piano. La porta da cui era entrato
      non è salvata, ed è l'unica cosa che permetterebbe di scegliere uno dei quattro punti
      d'ingresso laterali di `WorldTryEnterRoom`.
    - **Collocazione e conferma della voce**: "Sospendi e esci" è la sesta riga di
      `PauseMenu` (fra "Rigenera la run" e "Abbandona run"), **senza** passare da
      `ExitConfirm` — è l'unica uscita del menu che non perde nulla, e chiederne conferma
      la metterebbe sullo stesso piano delle due distruttive che le stanno accanto.
    - **Nessun salvataggio automatico** alla chiusura della finestra: il documento non lo
      chiede esplicitamente, il ciclo applicativo non ha un gancio di terminazione, e uno
      che scrivesse a ogni chiusura (compresa quella dopo una morte) è una decisione di
      comportamento, non un dettaglio tecnico. Limite dichiarato in
      `docs/engineering/known-issues.md`.

    Due **limiti dichiarati** accompagnano i default, entrambi in `known-issues.md`: il
    **Piano 0 non è sospendibile** in questa fetta (il documento lo prevede come stato
    salvato, ma quello stato comprende la generazione in corso e le carte-proposta) e i
    **semi delle sandbox Lua dei singoli oggetti non si salvano** (si ri-estraggono dallo
    stream di ricostruzione: deterministico dal file, non identico alla run originale).
    Verificato da `--suspend-test` (`GameSuspendTest`, `src/tests/suspend_tests.c`) e da
    `--layout-test` (le voci condizionali di `MainMenu`/`PauseMenu` restano dentro il
    riquadro). (`systems/save-and-meta-progression.md`, `ui/main-menu.md`,
    `ui/pause-menu.md`, `05-game-states-and-flow.md`, DEC-050, `src/game/run_suspend.c`.)
