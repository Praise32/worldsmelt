---
id: design-open-questions
title: Open Questions
domain: design
status: draft
authority: canonical
owner: design
summary: >-
  Coda ufficiale e unica delle domande ancora aperte (35 voci attive su 36 numerate; la 12 è chiusa da DEC-176) dopo DEC-001..DEC-176: economia, valori numerici da playtest, personaggi, multiplayer, produzione, interfaccia, distribuzione e produzione AI/asset.
last_reviewed: 2026-07-30
last_updated_from_session: 2026-07-30-wp-int-art-hookup
last_verified_commit: bf0fde8
topics: [open-questions, governance, domande aperte, playtest, backlog design, interfaccia, distribuzione, produzione ai, DEC-174, DEC-176, DEC-051, DEC-008, DEC-043, WP4, WP-INT]
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
3. Quali sono i valori esatti di soglia (tempo) e ricompensa delle stanze a tempo nei piani avanzati, e cosa succede se il giocatore le raggiunge dopo la soglia (nessuna ricompensa, ricompensa ridotta, o comportamento diverso)? (DEC-051 fissa solo il principio, da playtest come DEC-019.) **Aggiornamento 30/07 (WP5):** esiste ora un default proposto e implementato — soglia `40s + 6s × celle vere del piano`, misurata dall'ingresso nel piano (`Game.floorEntryElapsedSeconds`, mai dall'inizio della run); ricompensa 6 Ingots (`WORLD_ROOM_CURRENCY_TIMED`) SOLO entro soglia; oltre soglia **nessuna ricompensa** (non ridotta: zero), stanza comunque sempre percorribile — vedi `systems/special-rooms.md`, sezione "Stato di implementazione: la stanza a tempo", e la voce 32 sotto per la frequenza/i piani ammessi. La domanda resta aperta: sono default di implementazione, non una decisione di design.
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
12. ~~Qual è la scala dei pixel del gioco — pixel nativi molto grandi, pixel medi, dettaglio alto con pixel snapping — e vale la stessa scala per il mondo e per l'interfaccia?~~ (Provenienza: `Q-UI-003`, BLOCKING per l'art bible.) **Chiusa (28/07, DEC-176; valore rettificato da DEC-177):** il proprietario ha scelto, al checkpoint CP1 della produzione pixel-art, la scala base per personaggi/nemici/oggetti — fissata a 24px da DEC-176 e **corretta lo stesso giorno a 32px** da DEC-177 per allinearsi alla pipeline SD1.5/LoRA (512/32 = 16 esatto); i boss possono superarla e le **icone HUD seguono la propria griglia**, indipendente da questa scala — nessun obbligo di scala condivisa fra mondo e interfaccia. Vedi `content/visual-language.md`, sezione «Stile pixel-art ufficiale e scala base sprite».
13. Serve uno strumento di design come fonte dell'interfaccia (Penpot canonico, Penpot solo per mockup, un altro strumento, oppure file Markdown più PNG/SVG senza strumento), e in quale forma (cloud, self-host, nessuna integrazione)? (Provenienza: `Q-UI-001` e `Q-UI-005`.)
22. Con quale comando il menu di pausa si apre dal Piano 0? DEC-169 indica il menu di pausa come luogo in cui l'HUD di combattimento resta consultabile durante il Piano 0, ma ESC è già assegnato a `ExitConfirm` (DEC-074) e le condizioni di ingresso di `PauseMenu` prevedono oggi la sola provenienza da `Gameplay` (`ui/pause-menu.md`, `05-game-states-and-flow.md`). Manca il comando — o lo stato — che rende operativa la consultazione. (Provenienza: gap aperto da DEC-169 nella sessione del 2026-07-27; registrata anche in `ui/pause-menu.md` e `systems/floor-zero.md`.)

## Distribuzione

La **proprietà** di questo tema è assegnata al dominio ai-production da DEC-158, che non decide nessuna delle domande qui sotto.

14. Quali sono le piattaforme di destinazione della prima release: Linux, Windows, Steam Deck, macOS? Il repository conserva Windows, ma Linux è la piattaforma principale di sviluppo. (Provenienza: `Q-DIST-001`, priorità BLOCKING.)
15. Qual è il formato della **AI disclosure** e dove vive (pagina negozio, primo avvio, crediti in gioco), e quali requisiti deve soddisfare la pagina negozio? (DEC-158 assegna il tema ad ai-production proprio perché quel dominio possiede licenze e provenienza dei contenuti — DEC-113, DEC-140, DEC-148, `docs/ai-production/licenze.md` — su cui la disclosure poggia.)
16. Come arrivano i modelli all'utente: installer separato, download al primo avvio, strumento o DLC di piattaforma, oppure nessun modello distribuito? (DEC-070/DEC-086 fissano la scelta binaria al primo avvio e DEC-113/DEC-140 stabiliscono che i pesi non sono mai ridistribuiti col gioco — li scarica l'utente —, ma il meccanismo concreto di consegna non è deciso. Provenienza: `Q-DIST-002`.)
17. Il giocatore può importare contenuti propri — modelli, LoRA, prompt pack, rig, AudioSpec, skin dell'interfaccia — e con quali garanzie di validazione e originalità? (Provenienza: `Q-DIST-003`, priorità LATER, più la parte residua di `Q-IMG-001` sull'import di LoRA dell'utente; la sorte delle LoRA del progetto è invece decisa da DEC-148.)

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
19. Quante immagini originali il proprietario può realisticamente produrre e rivedere per i **dataset definitivi**, quale regime di review manuale vale per gli asset (approvazione umana sempre, solo per la release, a campione) e serve uno strumento interno di approvazione/rifiuto già nella pre-alpha? (DEC-148 stabilisce che i dataset definitivi li crea il proprietario, non la loro ampiezza né il regime di review. Provenienza: `Q-IMG-003`, `Q-IMG-004`, `Q-F0-002`.)
20. Il primo esperimento audio riguarda solo SFX, solo musica, o entrambi con due milestone separate? (DEC-109 fissa pipeline e catena di fallback, non l'ordine degli esperimenti. Provenienza: `Q-AUD-002`.)
21. Qual è il budget cloud disponibile e quante ore settimanali il proprietario può dedicare a domande, review di codice e asset, ascolto audio e playtest? (DEC-168 porta il training sulle 30 ore settimanali gratuite di Kaggle e declassa il runbook RunPod a fallback a pagamento, senza impegnare alcun budget. Serve a dimensionare i batch di lavoro. Provenienza: `Q-BUD-001` e `Q-BUD-002`.)

---

## Consumo del pacchetto artistico nel motore (W8, 2026-07-30)

Cinque domande aperte dallo stesso giro di lavoro sul consumo del pacchetto W8: quattro
dall'aggancio degli asset di `assets/art/` al motore, una dal gating del timer di run
(DEC-051) chiuso nello stesso lavoro in `known-issues.md` voce 10 punto 5 (WP1). Ognuna ha
già un **default proposto** implementato (stile DEC-019): il gioco funziona, ma il numero
o la politica non sono canone e vanno confermati o spostati.

23. A quale piano scatta la variante di **escalation** del tileset? DEC-024 chiede che il
    tema si intensifichi piano dopo piano sull'asse aspetto, e il contratto d'arte emette
    tre ruoli dedicati (`floor_deg`/`wall_deg`/`void_deg`, "crepe di brace",
    `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md`), ma nessun documento fissa la
    soglia. *Default proposto e implementato*: dal **piano 3**, cioè lo stesso confine
    della seconda traccia di gameplay (`AUDIO_GAMEPLAY_1_MAX_FLOOR`) e del passaggio dei
    boss a due fasi (DEC-028/106) — far coincidere i tre assi dell'escalation su un solo
    confine è l'ipotesi più leggibile per il giocatore. Da confermare al playtest.
    (`ROOM_TILESET_DEGRADED_FROM_FLOOR`, `src/render/game_renderer.c`.)
24. I **volumi audio devono persistere** fra un avvio e l'altro, e in quale forma? W8
    espone i tre slider in `Options` ma il gioco non ha un file di configurazione:
    inventarne uno avrebbe voluto dire decidere da soli percorso, formato e politica di
    migrazione. *Default proposto e implementato*: nessuna persistenza, si riparte da 1.0
    ad ogni avvio. La domanda vera è più ampia dei volumi — serve un file di
    preferenze del giocatore, e se sì dove vive (accanto a `catalog/`?) e con quale
    schema versionato. (`docs/design/ui/options-and-accessibility.md` elenca "audio" fra
    le categorie minime senza fissare né slider né persistenza.)
25. Passo, etichette e ordine degli **slider di volume** sono canone o solo un default?
    *Default proposto e implementato*: tre righe nell'ordine `Volume generale` / `Musica`
    / `Effetti`, passo del 10% su dieci caselle, valore mostrato anche in percentuale
    (DEC-058). Le altre quattro categorie minime del documento (video, controlli,
    accessibilità, gameplay) restano da scrivere e W8 non le ha inventate.
26. La **card di scoperta** va in alto al centro o in basso al centro? `ui/hud.md` dice
    "un riquadro in alto al centro (fuori dai quattro angoli)", ma quella formulazione
    descriveva l'HUD a quattro cluster con riquadro, che il layout V3 approvato al CP2 ha
    sostituito — e in V3 la quota alta a destra è occupata dalla riga piano/mondo.
    *Default proposto e implementato*: **basso al centro**, come il mock V3. Da
    confermare, e in ogni caso `ui/hud.md` va allineato alla scelta.
27. Il **timer di run non deve correre nel Piano 0**? DEC-051 fissa il timer sempre
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
    `src/world/floor_zero.c`.)

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

31. Con `ROOM_FUSION` ora presente nel motore, dove deve vivere la CONFERMA della fusione:
    resta un'azione di `BuildScreen` (come oggi), o si sposta integralmente nella stanza di
    fusione, coerente col modello canonico descritto in
    [Inventory and Synergy Screen](../ui/inventory-and-synergy-screen.md) ("la fusione
    *eseguita* nella stanza di fusione... questa schermata come sola consultazione")?
    *Default proposto e implementato (WP4, 30/07)*: la conferma resta in `BuildScreen`,
    raggiungibile da TRE porte d'ingresso equivalenti — il crogiolo della stanza di fusione
    (che apre `BuildScreen` già pronta alla fusione), TAB da `Gameplay`, e la voce dedicata
    nel `PauseMenu` — nessuna delle tre è stata rimossa. Da confermare al playtest.
    (`systems/item-fusion.md`, "Domande aperte residue"; `ui/inventory-and-synergy-screen.md`,
    nota di implementazione.)

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

35. Con `props/spuntoni` e `props/cassa` ora agganciati al motore, quale ALTERNANZA
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
    "Default proposti dall'implementazione".)

36. Con tre spritesheet di personaggio ora agganciati al motore (`character/fonditrice`/
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
    (`systems/characters.md`, sezione "Default proposti dall'implementazione".)
