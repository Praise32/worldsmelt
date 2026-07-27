---
id: design-decision-log
title: Decision Log
domain: design
status: approved
authority: canonical
owner: design
summary: >-
  Registro delle 175 decisioni di design (DEC-001..DEC-175) che cambiano il comportamento del gioco: 174 approved e 1 superseded (DEC-003, sostituita da DEC-071); fonte canonica di rango massimo nella gerarchia.
last_reviewed: 2026-07-28
last_verified_commit: d30890b
topics: [decision-log, governance, worldsmelt, design canonico, DEC-001..175]
related: []
supersedes: []
source_files: []
---

# Decision Log

Usare una voce per ogni decisione che cambia il comportamento del gioco.

## Template

### DEC-000 — Titolo

- **Data:** YYYY-MM-DD
- **Stato:** proposed | approved | superseded
- **Contesto:**
- **Decisione:**
- **Alternative considerate:**
- **Conseguenze:**
- **Documenti aggiornati:**

---

### DEC-001 — Una run è Piano 0 + cinque piani

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** La visione iniziale descriveva una run di cinque piani senza un hub distinto.
- **Decisione:** Una run standard è composta dal Piano 0 (hub) più cinque piani generati.
- **Alternative considerate:** Cinque piani senza hub dedicato; hub non giocabile.
- **Conseguenze:** La struttura di run, gli stati di navigazione e l'HUD devono trattare il Piano 0 come stato distinto da Gameplay.
- **Documenti aggiornati:** `04-run-structure.md`, `05-game-states-and-flow.md`, `ui/navigation-map.md`

### DEC-002 — Il Piano 0 è lo spazio di attesa giocabile

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Il gioco deve restare avviabile anche mentre l'IA genera i contenuti dei piani successivi.
- **Decisione:** Il gioco è sempre avviabile; il Piano 0 fa da spazio di attesa giocabile durante la generazione; finché non esistono asset dedicati, una versione statica curata del Piano 0 fa da sala d'attesa.
- **Alternative considerate:** Schermata di caricamento separata e non giocabile; blocco totale dell'input durante la generazione.
- **Conseguenze:** L'indicatore di generazione vive dentro il Piano 0 e non è più una schermata a sé stante.
- **Documenti aggiornati:** `04-run-structure.md`, `ui/generation-status.md`, `ui/navigation-map.md`

### DEC-003 — Il nome del gioco è provvisorio

- **Data:** 2026-07-17
- **Stato:** superseded (sostituita da DEC-071)
- **Contesto:** "Melting Run" è un titolo di lavoro, non il nome definitivo del progetto.
- **Decisione:** Il nome resta provvisorio; il nome definitivo lo sceglierà il proprietario (domanda aperta); il vocabolario può comunque appoggiarsi al campo semantico fusione/scioglimento.
- **Alternative considerate:** Fissare subito un nome definitivo.
- **Conseguenze:** I nomi placeholder di risorse, Innesto e Veterano dipendono da questa scelta futura.
- **Documenti aggiornati:** `governance/open-questions.md`, `governance/glossary.md`
- **Nota (2026-07-18): risolta da DEC-071.** Il nome definitivo del gioco è "Worldsmelt"; "Melting Run" resta il nome storico del repository e il titolo di lavoro citato in questo registro. La domanda aperta sul nome è chiusa.
- **Nota (2026-07-27):** stato portato a `superseded` dal retrofit di DEC-155 — unica decisione del registro **integralmente** sostituita: il suo contenuto operativo (nome provvisorio, scelta rimandata al proprietario) è interamente esaurito da DEC-071, che dichiara essa stessa «risolve DEC-003». Il testo storico resta invariato.

### DEC-004 — Il Piano 0 è un hub ibrido

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve un luogo unico che colleghi meta-progressione, preparazione della run e attesa della generazione.
- **Decisione:** Il Piano 0 è un hub ibrido: rifugio sicuro più arene di sfida opzionali che usano contenuti "best-of" già validati delle run passate. Contiene: museo delle creazioni migliori, scelta del tema della run, scelta del personaggio, indicatore di generazione. L'uscita verso il piano 1 si apre quando il piano 1 è pronto.
- **Alternative considerate:** Hub minimale senza arene; scelta di tema e personaggio dentro Run Setup.
- **Conseguenze:** Run Setup si restringe a seed e modalità; generation-status e main-menu rimandano al Piano 0.
- **Documenti aggiornati:** `ui/run-setup.md`, `ui/generation-status.md`, `ui/main-menu.md`, `04-run-structure.md`, `systems/floor-zero.md`

### DEC-005 — Il tema della run è proposto dall'IA nel Piano 0

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** La run deve avere un'identità tematica riconoscibile e non ripetitiva.
- **Decisione:** L'IA propone 2-3 temi generati nel Piano 0, il giocatore ne sceglie uno; il tema evolve o degenera piano dopo piano fino al boss del piano 5.
- **Alternative considerate:** Tema fisso per tutte le run; tema scelto da un menu statico in Run Setup.
- **Conseguenze:** La scelta del tema si sposta dal Run Setup al Piano 0.
- **Documenti aggiornati:** `systems/floor-zero.md`, `ui/run-setup.md`

### DEC-006 — Vittoria al boss del piano 5 e permadeath

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve un confine chiaro tra la run valida per classifica e la prosecuzione libera oltre la vittoria.
- **Decisione:** Sconfiggere il boss del piano 5 chiude la run ufficiale (valida per classifiche); il giocatore può scegliere di proseguire in piani extra sempre più degenerati finché non muore. Salute a zero significa run persa, permadeath.
- **Alternative considerate:** Nessun limite ufficiale di piani; vittoria che chiude definitivamente la run senza possibilità di proseguire.
- **Conseguenze:** I risultati devono distinguere la run ufficiale dalla prosecuzione extra ai fini di classifica.
- **Documenti aggiornati:** `ui/results-and-leaderboards.md`, `04-run-structure.md`
- **Nota (2026-07-18):** la prosecuzione in piani extra descritta qui è stata spostata tra le idee future da DEC-031: non va implementata ora. La vittoria al boss del piano 5 chiude la run e basta; la parte di questa decisione sulla prosecuzione resta di riferimento storico, superata nella pratica da DEC-031.
- **Nota (2026-07-27):** verificata dal retrofit di DEC-155 e classificata come **sostituzione parziale**: resta `approved`. DEC-031 supera solo la prosecuzione in piani extra; la chiusura della run alla vittoria sul boss del piano 5 e il **permadeath** restano canone di questa decisione, citato da `03-core-loop.md`, `systems/bosses.md` e DEC-045.

### DEC-007 — Controlli: movimento libero, sparo a 4 direzioni

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve uno schema di controllo semplice, leggibile e rimappabile.
- **Decisione:** Movimento libero, sparo nelle 4 direzioni cardinali.
- **Alternative considerate:** Mira libera analogica; sparo automatico verso il nemico più vicino.
- **Conseguenze:** Le opzioni di rimappatura e accessibilità devono rispettare questo schema di base.
- **Documenti aggiornati:** `systems/player.md`, `ui/options-and-accessibility.md`

### DEC-008 — Salute stratificata

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve distinguere la protezione temporanea dalla salute permanente.
- **Decisione:** Salute base più salute temporanea/protettiva; ordine di consumo: prima la temporanea, poi la base.
- **Alternative considerate:** Salute unica senza strati; scudo separato senza priorità di consumo definita.
- **Conseguenze:** L'HUD deve mostrare i due strati in modo distinguibile a colpo d'occhio.
- **Documenti aggiornati:** `systems/health-and-resources.md`, `ui/hud.md`

### DEC-009 — Stanze di numero e grandezza variabili

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Il codice attuale genera stanze tutte uguali su una griglia fissa.
- **Decisione:** Griglia fissa, numero di stanze variabile per piano, stanze di grandezze tutte diverse tra loro con una grandezza minima garantita. È un requisito di design; il codice si adeguerà.
- **Alternative considerate:** Mantenere stanze di dimensione uniforme.
- **Conseguenze:** `rooms-and-floor-generation.md` deve introdurre variabilità dimensionale e una grandezza minima garantita.
- **Documenti aggiornati:** `systems/rooms-and-floor-generation.md`
- **Nota (2026-07-27):** superata parzialmente da DEC-170 — le stanze passano a **taglie multiple in classi discrete** stile Isaac (1x1/1x2/2x1/2x2/L): la clausola «grandezze tutte diverse tra loro» non vale più, più stanze dello stesso piano possono condividere la stessa taglia. Restano validi il principio di variabilità dimensionale del piano e la grandezza minima garantita (ora la taglia 1x1).

### DEC-010 — Tassonomia delle stanze

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve un set comune di tipi di stanza per generazione e level design.
- **Decisione:** Tipi canonici: partenza, combattimento, tesoro, negozio, boss, più archetipi speciali: stanza di fusione, stanza segreta, arena di sfida, scambio ad alto rischio (ri-tematizzato in modo originale, non "devil room").
- **Alternative considerate:** Mantenere nomi di stanza riconoscibili da altri giochi.
- **Conseguenze:** `special-rooms.md` deve introdurre la stanza di fusione e ridenominare lo scambio ad alto rischio.
- **Documenti aggiornati:** `systems/special-rooms.md`, `systems/rooms-and-floor-generation.md`
- **Nota (2026-07-18):** integrata da DEC-051 — aggiunto un quinto archetipo speciale, "stanza a tempo", nei piani avanzati (dettaglio in `systems/special-rooms.md` e `systems/rooms-and-floor-generation.md`).

### DEC-011 — Tassonomia oggetti e slot espandibili

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve una categorizzazione completa degli oggetti per pool, HUD e schermata build.
- **Decisione:** Tassonomia completa: attivi, passivi, stat-up, Innesti (nome placeholder). Slot espandibili: si parte con 1 attivo + 1 Innesto; oggetti o eventi rari aggiungono slot durante la run. Passivi e stat-up senza limite.
- **Alternative considerate:** Slot fissi per tutta la run.
- **Conseguenze:** `items-pools-and-rarity.md`, `grafts.md` e l'HUD devono riflettere gli slot espandibili e il termine Innesto.
- **Documenti aggiornati:** `systems/items-pools-and-rarity.md`, `systems/grafts.md`, `ui/hud.md`

### DEC-012 — Sinergie a doppio binario: implicite e fusione esplicita

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Le sole sinergie implicite non bastano a dare al gioco una meccanica-firma riconoscibile.
- **Decisione:** Doppio binario: (a) sinergie implicite/automatiche quando due oggetti compatibili convivono (le 6 attuali del codice più quelle generate); (b) fusione esplicita, meccanica-firma: nella stanza di fusione il giocatore consuma due oggetti e ottiene un oggetto nuovo generato dall'IA che eredita comportamento e presentazione da entrambi. Ogni sinergia/fusione deve cambiare sia il comportamento sia la presentazione visiva.
- **Alternative considerate:** Solo sinergie implicite; fusione come semplice somma di statistiche.
- **Conseguenze:** Serve una stanza di fusione dedicata e un documento di sistema per la fusione esplicita.
- **Documenti aggiornati:** `systems/synergies.md`, `systems/item-fusion.md`, `systems/special-rooms.md`, `ui/inventory-and-synergy-screen.md`

### DEC-013 — Risorse ri-tematizzate e definite per funzione

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Il set di risorse attuale cita esplicitamente il set cuori/monete/bombe/chiavi di un altro gioco.
- **Decisione:** Risorse definite per funzione, nomi placeholder finché non esiste il nome del gioco: salute (stratificata, DEC-008), valuta principale, strumento di breccia (funzione delle "bombe"), strumento di apertura (funzione delle "chiavi"), catalizzatore di fusione (risorsa nuova che abilita/paga la fusione esplicita). Via la citazione all'altro gioco.
- **Alternative considerate:** Mantenere i nomi originali del genere.
- **Conseguenze:** `health-and-resources.md`, `09-originality-guardrails.md` e l'HUD devono usare i nomi per funzione, mai i nomi presi in prestito.
- **Documenti aggiornati:** `systems/health-and-resources.md`, `09-originality-guardrails.md`, `ui/hud.md`
- **Nota (2026-07-18):** nomenclatura fissata da DEC-072. I nomi placeholder di valuta principale, strumento di breccia, strumento di apertura e catalizzatore di fusione hanno ora il rispettivo nome inglese in-game (Ingots, Blast Charges, Cast Keys, Flux); i termini di lavoro italiani restano quelli usati in questo documento e nella KB.
- **Nota (2026-07-27):** verificata dal retrofit di DEC-155 e classificata come **sostituzione parziale**: resta `approved`. DEC-072 fissa solo i nomi in-game dei placeholder; la definizione delle risorse **per funzione** e il divieto dei nomi presi in prestito restano canone di questa decisione, citato da `systems/rewards-and-economy.md` e `systems/health-and-resources.md`.

### DEC-014 — Personaggio base più personaggio alternativo generato

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve varietà di run senza introdurre potenziamenti permanenti del personaggio.
- **Decisione:** Esiste un personaggio base sempre disponibile; ogni run l'IA genera un personaggio alternativo (trait unico più statistiche casuali entro bande): prendere o lasciare, si sceglie nel Piano 0.
- **Alternative considerate:** Roster fisso di personaggi sbloccabili in modo permanente.
- **Conseguenze:** La scelta del personaggio si sposta da Run Setup al Piano 0.
- **Documenti aggiornati:** `systems/floor-zero.md`, `ui/run-setup.md`
- **Nota (2026-07-18):** integrata da DEC-030 — il personaggio base non è un singolo personaggio ma una piccola rosa fissa di 2-3 personaggi curati con ruoli distinti; il personaggio generato per run descritto qui si aggiunge a quella rosa nella scelta del Piano 0, non la sostituisce.

### DEC-015 — Meta-progressione senza potenziamenti permanenti

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve una progressione a lungo termine che non comprometta l'equilibrio delle singole run.
- **Decisione:** Persistono il catalogo di tutti i contenuti generati, il museo del Piano 0 (i migliori) e punti guadagnati in singleplayer spendibili per sbloccare contenuti generati nei pool delle run future. Niente potenziamenti permanenti del personaggio. Sblocchi disattivati nelle modalità competitive.
- **Alternative considerate:** Potenziamenti permanenti del personaggio tra una run e l'altra.
- **Conseguenze:** Il menu principale deve esporre una voce Catalogo/Museo.
- **Documenti aggiornati:** `systems/save-and-meta-progression.md`, `ui/main-menu.md`

### DEC-016 — Multiplayer: gare asincrone sulla stessa run

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve una visione minima e realistica per la competizione multigiocatore.
- **Decisione:** Visione fissata: gare asincrone sulla stessa run (stesso seed/manifest, il determinismo esiste già), classifiche a tempo/punteggio, pool sbloccati esclusi. Ogni altro dettaglio resta `experimental`.
- **Alternative considerate:** Gara simultanea online in tempo reale.
- **Conseguenze:** La lobby multiplayer diventa selezione di una run/seed pubblicata, non una lobby live con partecipanti in attesa.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `ui/multiplayer-lobby.md`, `ui/results-and-leaderboards.md`
- **Nota (2026-07-25):** corretta da DEC-141 — la premessa «il determinismo esiste già» non regge per il gameplay: l'RNG di spawn/drop/combattimento è ancora seedato con `time(NULL)` (known-issues.md #3), non dal seed di run. Il determinismo vale oggi solo per il contenuto generato (tema, layout, oggetti); il fix del RNG di gameplay è prerequisito bloccante di qualunque gara a stesso seed.

### DEC-017 — Durata obiettivo 30-45 minuti

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve un riferimento di ritmo per contenuti, difficoltà e pacing di generazione.
- **Decisione:** Durata obiettivo di una run completa vinta (Piano 0 + 5 piani): 30-45 minuti.
- **Alternative considerate:** Durata libera senza riferimento condiviso.
- **Conseguenze:** Il pacing di generazione e difficoltà deve rientrare in questa finestra.
- **Documenti aggiornati:** `04-run-structure.md`, `07-difficulty-and-progression.md`

### DEC-018 — Idee future parcheggiate

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Alcune idee sono interessanti ma non sono requisiti immediati del gioco base.
- **Decisione:** Parcheggiate in una sezione dedicata di idee future, non requisiti: modalità caos, obiettivi di vittoria legati al tema, salute "integrità melting" (il personaggio si scioglie), lobby online custom coi contenuti sbloccati, nome definitivo del gioco.
- **Alternative considerate:** Includerle subito come requisiti del gioco base.
- **Conseguenze:** Run Setup rimuove la modalità caos come opzione attiva.
- **Documenti aggiornati:** `ui/run-setup.md`, `governance/open-questions.md`
- **Nota (2026-07-18):** integrata da DEC-045 — si aggiunge alla lista delle idee future anche portare le funzioni del Catalogo (enciclopedia, preferiti, spesa punti) dentro il Piano 0/museo.
- **Nota (2026-07-18):** integrata da DEC-031 — si aggiunge alle idee future la prosecuzione in piani extra oltre il boss del piano 5, non implementata ora.
- **Nota (2026-07-18):** integrata da DEC-036 — si aggiunge alle idee future la parametrizzazione/generazione audio a tema.

### DEC-019 — Valori numerici attuali come default draft

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Il codice contiene già pesi e bande numeriche non ancora validate col playtest.
- **Decisione:** Pesi rarità {55, 30, 12, 3}, pesi boss {0, 0, 70, 30}, bande di potenza colpi [0.75-1.25] / nemici [0.7-1.35] / boss [1.4-3.2], 4 rarità (comune, non-comune, rara, leggendaria) sono i default attuali dell'implementazione. Vanno documentati come "default proposto", non come decisione presa, e restano da validare col playtest.
- **Alternative considerate:** Fissarli subito come valori definitivi.
- **Conseguenze:** `governance/open-questions.md` registra la validazione di questi valori come domanda aperta.
- **Documenti aggiornati:** `systems/items-pools-and-rarity.md`, `systems/bosses.md`, `systems/combat-and-projectiles.md`, `governance/open-questions.md`
- **Nota (2026-07-27):** promossa da DEC-154; valori esatti da playtest. La promozione riguarda l'impianto (quattro rarità, esistenza dei pesi di rarità e del pool boss, esistenza delle bande di potenza per colpi, nemici e boss), non i numeri: nei documenti di sistema restano marcati «default proposto, da validare col playtest» e le domande aperte corrispondenti restano aperte. Verificata l'assenza di conflitti con le decisioni che poggiano su di essa (DEC-087 — con il vincolo «almeno 1 per rarità» ora fissato da DEC-144 —, DEC-107, DEC-133, DEC-145).

### DEC-020 — Il modello di generazione reale è canone concettuale

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve chiarire che l'IA non sceglie da un menu chiuso ma inventa contenuti dentro garanzie di design.
- **Decisione:** L'IA non "sceglie da un menu" ma inventa contenuti parametrici dentro bande di garanzia (tipi di colpo, nemici, layout stanze) e scrive comportamenti (oggetti) validati in sandbox con fallback curato sempre presente; mai un contenuto rotto, mai un blocco della partita.
- **Alternative considerate:** Generazione per sola selezione da liste chiuse pre-scritte.
- **Conseguenze:** `ui/generation-status.md` rimanda a `06-ai-content-generation-model.md` senza duplicare dettagli tecnici.
- **Documenti aggiornati:** `06-ai-content-generation-model.md`, `systems/generated-content-validation.md`, `ui/generation-status.md`

### DEC-021 — Menu multiplayer a due assi

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-016 fissava solo la gara classificata a stesso seed; il proprietario vuole un menu multiplayer completo.
- **Decisione:** Il menu multiplayer offre due scelte indipendenti: Modalità = Leggera (non classificata) o Classificata; Tipo di gara = Stesso seed (stessa run esatta per tutti) o Seed diversi (una run per giocatore). Tutte e quattro le combinazioni esistono. Le classifiche valgono solo per la Classificata.
- **Alternative considerate:** Solo classificata a stesso seed (DEC-016 originale); modalità uniche separate ("Shared Run Race"/"Unique Run Duel", nomi eliminati).
- **Conseguenze:** La Classificata a seed diversi richiede un criterio di normalizzazione della difficoltà (open question); la gara resta asincrona (DEC-016).
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `ui/multiplayer-lobby.md`
- **Nota (2026-07-18):** estesa da DEC-062 — dentro la Modalità Classificata, il "Tipo di gara" passa da due a tre istanze (stesso seed, seed diversi, Classificata giornaliera pubblica "Daily"); la Modalità Leggera resta a due istanze. DEC-062 aggiunge inoltre che le classifiche sono divise per metrica (tempo e punteggio separati, mai combinati).

### DEC-022 — Catalizzatore di fusione raro

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** la fusione è la meccanica-firma del gioco; senza un vincolo di rarità sul catalizzatore rischia di diventare un'azione ripetibile senza peso.
- **Decisione:** Il catalizzatore di fusione è raro (drop da boss o arene, oppure acquisto costoso); attese 1-2 fusioni a run. Ogni fusione è un momento memorabile e una scommessa strategica.
- **Alternative considerate:** Catalizzatore comune e liberamente accumulabile; fusione illimitata.
- **Conseguenze:** `item-fusion.md` e `health-and-resources.md` devono trattare il catalizzatore come risorsa rara con fonti dedicate.
- **Documenti aggiornati:** `systems/item-fusion.md`

### DEC-023 — Risultato della fusione a doppio stadio

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** generare subito un oggetto interamente IA rischia di bloccare il giocatore in attesa nel momento più importante della run (la fusione).
- **Decisione:** Alla fusione il gioco compone SUBITO il risultato con regole deterministiche (eredita trait e strati visivi dai due genitori); l'IA in sottofondo genera nome, comportamento e sprite dedicati e li applica appena pronti. Mai un'attesa per il giocatore; se la generazione non arriva, la composizione deterministica resta valida (fallback naturale).
- **Alternative considerate:** Attendere sempre la generazione IA completa prima di consegnare l'oggetto; fusione puramente deterministica senza rifinitura IA.
- **Conseguenze:** `item-fusion.md` descrive il doppio stadio come parte del risultato, non solo come fallback d'emergenza.
- **Documenti aggiornati:** `systems/item-fusion.md`

### DEC-024 — Degenerazione come escalation leggibile del tema

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** serve un principio comune che leghi come il tema si intensifica piano dopo piano su più sistemi, senza produrre distorsioni arbitrarie o illeggibili.
- **Decisione:** Piano dopo piano il tema si intensifica su quattro assi (aspetto, nemici, regole di stanza, audio), ma deve sempre rispettare uno schema visivo comprensibile e un audio ascoltabile (il budget di leggibilità vale su tutti i piani). Progressione dentro il tema, non distorsione arbitraria: esempio canonico del proprietario, tema "fantasy medievale", piano 1 cavalieri di grado infimo, piano 5 cavalieri esperti, gameplay che cambia di conseguenza, con eventuali elementi di degrado ambientale. Nei piani avanzati sono ammessi modificatori di stanza generati (dentro le garanzie di giocabilità); Veterani più frequenti nei piani alti.
- **Alternative considerate:** Degenerazione libera senza vincolo di leggibilità; tema fisso senza intensificazione piano dopo piano.
- **Conseguenze:** `07-difficulty-and-progression.md`, `enemies.md`, `rooms-and-floor-generation.md`, `visual-language.md` e `audio-and-feedback.md` devono riflettere i quattro assi e il vincolo di leggibilità.
- **Documenti aggiornati:** `07-difficulty-and-progression.md`, `systems/enemies.md`, `systems/rooms-and-floor-generation.md`, `content/visual-language.md`, `content/audio-and-feedback.md`

### DEC-025 — Segrete a due livelli

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il metodo di scoperta delle stanze segrete era una domanda di design aperta.
- **Decisione:** Stanze segrete "normali" con indizi visivi leggibili (crepe, anomalie del tema) apribili con lo strumento di breccia; "super-segrete" senza indizi, trovabili solo con oggetti/Innesti rivelatori o intuizione estrema.
- **Alternative considerate:** Un solo livello di segretezza; scoperta puramente casuale.
- **Conseguenze:** `secrets-and-obstacles.md` e `special-rooms.md` descrivono i due livelli; la domanda aperta sul metodo di scoperta è chiusa.
- **Documenti aggiornati:** `systems/secrets-and-obstacles.md`, `systems/special-rooms.md`, `governance/open-questions.md`

### DEC-026 — Negozio a prezzi fissi più offerta speciale

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** prezzi dinamici o costi in salute nel negozio erano una domanda di design aperta.
- **Decisione:** Prezzi base fissi per fascia di rarità (il giocatore impara il valore delle cose); ogni negozio ha 1 offerta speciale generata (sconto, pacchetto, oggetto del tema). I "patti" a costo salute non esistono nel negozio: restano esclusivi dello scambio ad alto rischio.
- **Alternative considerate:** Prezzi dinamici legati alla build; costi in salute nel negozio.
- **Conseguenze:** `rewards-and-economy.md` e `special-rooms.md` devono fissare il confine tra negozio e scambio ad alto rischio.
- **Documenti aggiornati:** `systems/rewards-and-economy.md`, `systems/special-rooms.md`, `governance/open-questions.md`

### DEC-027 — Punti sblocco a doppio canale

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-015 fissava solo il principio dei punti singleplayer, non la struttura di guadagno.
- **Decisione:** Punti base assegnati a fine run in funzione del risultato (piani completati, boss, scoperte) più bonus da prove specifiche (fisse o generate: es. boss senza danni, 2 segrete trovate, arena completata). Solo singleplayer (DEC-015 invariata).
- **Nota (2026-07-18):** il testo originale citava anche la "prosecuzione" tra i fattori: rimossa per coerenza con DEC-031 (prosecuzione parcheggiata tra le idee future).
- **Alternative considerate:** Un unico canale di punti senza bonus da prove; bonus disponibili anche in competitivo.
- **Conseguenze:** `rewards-and-economy.md` e `save-and-meta-progression.md` devono riflettere il doppio canale.
- **Documenti aggiornati:** `systems/rewards-and-economy.md`, `systems/save-and-meta-progression.md`

### DEC-028 — Boss a escalation col piano

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** serve coerenza tra la complessità dei boss e l'escalation generale del tema (DEC-024).
- **Decisione:** Boss del piano 1 a fase singola e leggibile; dal piano 3 due fasi con cambio di comportamento; il boss del piano 5 è il più complesso.
- **Alternative considerate:** Stessa complessità di boss per tutti i piani; complessità libera senza soglie dichiarate.
- **Conseguenze:** `bosses.md` deve descrivere la progressione di fasi per piano.
- **Documenti aggiornati:** `systems/bosses.md`

### DEC-029 — Arene del Piano 0 con piccola dote

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** le arene di sfida del Piano 0 non davano alcun beneficio alla run in preparazione.
- **Decisione:** Completare un'arena dà un piccolo vantaggio iniziale per la run che sta per cominciare (risorse o un oggetto comune); disattivato nelle run in modalità Classificata (coerenza con DEC-016/DEC-021).
- **Alternative considerate:** Nessun beneficio dalle arene sulla run in preparazione; dote sempre attiva anche in Classificata.
- **Conseguenze:** `floor-zero.md` deve descrivere la dote e la sua disattivazione in Classificata.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-030 — Rosa di personaggi base

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** un solo personaggio base rischiava di rendere la scelta d'identità povera rispetto all'unica alternativa generata.
- **Decisione:** i personaggi base sono una piccola rosa di 2-3 personaggi FISSI e curati con ruoli distinti (indicativamente: offensivo, difensivo, esploratore — nomi/dettagli da definire), sbloccabili presto. Il personaggio generato per-run (DEC-014) si aggiunge alla rosa nella scelta del Piano 0, non la sostituisce.
- **Alternative considerate:** Un solo personaggio base (versione originale di DEC-014); roster ampio e liberamente configurabile.
- **Conseguenze:** DEC-014 viene annotata come integrata da questa decisione; `systems/characters.md` deve descrivere la rosa e la sua relazione con il personaggio generato.
- **Documenti aggiornati:** `systems/characters.md`, `systems/player.md`, `governance/decision-log.md` (annotazione DEC-014)

### DEC-031 — Prosegui-oltre parcheggiato tra le idee future

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** la prosecuzione in piani extra dopo il boss del piano 5 (DEC-006) non è una priorità di implementazione in questa fase del progetto.
- **Decisione:** la vittoria al boss del piano 5 chiude la run e basta; la prosecuzione in piani extra non va implementata ora ed entra tra le idee future (lista DEC-018). DEC-006 resta nel decision log ma viene annotata di conseguenza.
- **Alternative considerate:** Mantenere la prosecuzione in piani extra come requisito attivo; rimuovere del tutto l'idea invece di parcheggiarla.
- **Conseguenze:** `04-run-structure.md`, `ui/results-and-leaderboards.md`, `03-core-loop.md` e `systems/bosses.md` (dove citava il proseguimento) descrivono ora la vittoria come fine della run, con il proseguimento solo come idea futura.
- **Documenti aggiornati:** `04-run-structure.md`, `ui/results-and-leaderboards.md`, `03-core-loop.md`, `systems/bosses.md`, `governance/decision-log.md` (annotazione DEC-006)

### DEC-032 — Densità oggetti per piano

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il numero medio di oggetti offerti per piano era una domanda aperta.
- **Decisione:** in media 3-5 oggetti per piano, indicativamente circa 20 a run completa (5 piani).
- **Alternative considerate:** Densità fissa per piano senza variazione; densità lasciata interamente al playtest senza un default indicativo.
- **Conseguenze:** Risolve la domanda aperta sul numero medio di oggetti per piano, che viene rimossa da `governance/open-questions.md`.
- **Documenti aggiornati:** `systems/items-pools-and-rarity.md`, `governance/open-questions.md`

### DEC-033 — Cap di salute per personaggio

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** serviva stabilire se la salute base avesse un limite massimo e se fosse uguale per tutti i personaggi.
- **Decisione:** ogni personaggio (base o generato) ha il proprio tetto di salute base come parte delle sue statistiche: personaggi-vetro (tetto basso) e personaggi-roccia (tetto alto) esistono per design. I contenitori di salute crescono con stat-up e oggetti fino al tetto del personaggio specifico. Le bande min/max entro cui possono variare i tetti — soprattutto per i personaggi generati — sono valori di default da playtest, come DEC-019. La salute temporanea resta regolata da DEC-008.
- **Alternative considerate:** Nessun tetto massimo di salute base; un tetto assoluto unico e uguale per tutti i personaggi (versione iniziale della decisione, corretta dal proprietario in questa stessa sessione).
- **Conseguenze:** `health-and-resources.md`, `player.md` e `characters.md` devono trattare il tetto come parte delle statistiche del personaggio, non come valore globale unico; il legame con la rosa base (DEC-030) e col personaggio generato (DEC-014) va esplicitato.
- **Documenti aggiornati:** `systems/health-and-resources.md`, `systems/player.md`, `systems/characters.md`

### DEC-034 — Innesti a doppia natura

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** gli Innesti rischiavano di restare solo modificatori minori, senza una progressione di profondità legata alla rarità.
- **Decisione:** Innesti comuni = piccoli modificatori situazionali passivi, scambiabili al volo senza costo; Innesti rari = "piega-regole" che alterano in piccolo UNA regola del gioco (es. le offerte del negozio, il comportamento delle segrete). La rarità decide la profondità dell'effetto.
- **Alternative considerate:** Tutti gli Innesti con la stessa natura di modificatore situazionale, indipendentemente dalla rarità.
- **Conseguenze:** `grafts.md` deve descrivere le due nature e il vincolo "una sola regola alterata" per i piega-regole.
- **Documenti aggiornati:** `systems/grafts.md`

### DEC-035 — Stat-up diffusi e oggetti ibridi

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** gli stat-up e gli oggetti con comportamento erano trattati come categorie separate senza sovrapposizione.
- **Decisione:** gli stat-up compaiono anche nei pool normali (tesoro/negozio), non solo come ricompensa boss; inoltre gli oggetti con comportamento possono includere modifiche alle statistiche, positive o NEGATIVE, coerenti con ciò che fanno (oggetti ibridi comportamento+statistiche).
- **Alternative considerate:** Stat-up riservati a contesti specifici; nessuna modifica di statistica ammessa su oggetti attivi/passivi.
- **Conseguenze:** `items-pools-and-rarity.md` aggiorna la tassonomia senza rompere DEC-011 (restano 4 categorie; gli ibridi compongono, non aggiungono una categoria).
- **Documenti aggiornati:** `systems/items-pools-and-rarity.md`

### DEC-036 — Audio curato, generazione futura

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** l'asse audio dell'escalation del tema (DEC-024) non specificava se i suoni fossero curati o generati.
- **Decisione:** per ora musica e suoni sono curati e statici; la parametrizzazione/generazione audio a tema è un'idea futura (lista DEC-018). L'asse audio della degenerazione (DEC-024) si applica per ora con i mezzi curati disponibili (selezione/mix), senza generazione.
- **Alternative considerate:** Generazione audio a tema fin da subito.
- **Conseguenze:** `content/audio-and-feedback.md` chiarisce che l'asse audio dell'escalation è oggi curato, non generato.
- **Documenti aggiornati:** `content/audio-and-feedback.md`
- **Nota (2026-07-27):** superata parzialmente da DEC-109 — la generazione audio non è più «futura»: DEC-109 adotta l'audio generativo (Stable Audio Small) con fallback curato sempre garantito. Restano validi il fallback curato e l'applicazione dell'asse audio di DEC-024. Classificazione parziale verificata dal retrofit di DEC-155.

### DEC-037 — Comportamenti Lua anche per trait e tipi di colpo

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** la pipeline comportamentale validata in sandbox era riservata agli oggetti; il proprietario vuole estenderla ad altri contenuti per aumentare la varietà.
- **Decisione:** il trait unico del personaggio generato è un comportamento Lua generato e validato in sandbox (stessa pipeline degli oggetti). Anche i tipi di colpo evolvono verso comportamenti Lua generati con grande varietà: le manopole parametriche attuali restano come garanzia di bilanciamento e fallback, ma il comportamento del colpo può essere scriptato.
- **Alternative considerate:** Mantenere tipi di colpo e trait personaggio solo come generazione parametrica entro bande, senza comportamenti scriptati.
- **Conseguenze:** `systems/characters.md`, `systems/combat-and-projectiles.md` e `06-ai-content-generation-model.md` devono riflettere la pipeline comportamentale estesa e il ruolo di garanzia/fallback delle manopole parametriche per i colpi.
- **Documenti aggiornati:** `systems/characters.md`, `systems/combat-and-projectiles.md`, `06-ai-content-generation-model.md`

### DEC-038 — Difficoltà unica

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** serviva decidere se il gioco offrisse livelli di difficoltà selezionabili.
- **Decisione:** nessun livello di difficoltà selezionabile; la curva è quella dei 5 piani, uguale per tutti. Classifiche immediatamente confrontabili.
- **Alternative considerate:** Livelli di difficoltà selezionabili in `RunSetup`; difficoltà adattiva basata sulle prestazioni del giocatore.
- **Conseguenze:** `07-difficulty-and-progression.md` e `ui/run-setup.md` devono escludere esplicitamente un selettore di difficoltà.
- **Documenti aggiornati:** `07-difficulty-and-progression.md`, `ui/run-setup.md`

### DEC-039 — Scelta tema con anteprima visiva

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** le carte tema del Piano 0 mostravano solo nome e descrizione, senza alcun elemento visivo.
- **Decisione:** ognuna delle 2-3 proposte di tema nel Piano 0 mostra nome, breve descrizione e un'anteprima visiva già generata (es. uno sprite campione di un nemico del tema); la generazione delle anteprime ha priorità altissima a inizio run. Fallback: se un'anteprima non è pronta, nome+descrizione.
- **Alternative considerate:** Anteprime visive opzionali o a bassa priorità di generazione; nessuna anteprima visiva, solo testo.
- **Conseguenze:** `systems/floor-zero.md` descrive il comportamento completo; `06-ai-content-generation-model.md` registra una nota sulla priorità di generazione.
- **Documenti aggiornati:** `systems/floor-zero.md`, `06-ai-content-generation-model.md`

### DEC-040 — Museo con interazione

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il museo del Piano 0 era solo consultazione passiva delle creazioni migliori.
- **Decisione:** il museo del Piano 0 è una galleria delle creazioni migliori (oggetti, nemici, boss, fusioni, personaggi) con nome e storia, e permette di provarle: testare un oggetto in una saletta, riaffrontare un boss in arena (si collega alle arene di sfida di DEC-004/DEC-029).
- **Alternative considerate:** Museo puramente consultativo, senza possibilità di provare le creazioni.
- **Conseguenze:** `systems/floor-zero.md` descrive l'interazione col museo e il collegamento con le arene di sfida.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-041 — Alla sconfitta

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era chiaro cosa restasse al giocatore in caso di sconfitta, rispetto alla vittoria.
- **Decisione:** alla sconfitta restano i punti sblocco maturati in misura RIDOTTA rispetto alla vittoria, il catalogo aggiornato con le creazioni incontrate e le statistiche. Nessun oggetto sopravvive alla run (permadeath, DEC-006).
- **Alternative considerate:** Nessun punto sblocco alla sconfitta; punti sblocco identici a vittoria e sconfitta.
- **Conseguenze:** `ui/results-and-leaderboards.md` e i documenti di struttura della run devono distinguere l'esito ridotto della sconfitta da quello della vittoria.
- **Documenti aggiornati:** `ui/results-and-leaderboards.md`, `04-run-structure.md`, `03-core-loop.md`, `systems/bosses.md`

### DEC-042 — Presentazione delle prove

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-027 introduce bonus da prove specifiche (fisse o generate) che danno punti sblocco, ma non era definito dove e quando il giocatore le vede prima e durante la run.
- **Decisione:** le prove specifiche della run vengono presentate al giocatore al passaggio dal Piano 0 al piano 1 (momento di ingresso nella run vera) e restano sempre consultabili dal menu di pausa e dalla schermata build.
- **Alternative considerate:** Prove nascoste fino al completamento, senza presentazione anticipata; prove visibili solo a fine run nella schermata risultati.
- **Conseguenze:** `systems/floor-zero.md` deve descrivere la presentazione delle prove al momento dell'ingresso nel piano 1; `ui/pause-menu.md` e `ui/inventory-and-synergy-screen.md` devono esporre una voce "Prove" sempre consultabile; `systems/rewards-and-economy.md` rimanda senza ripetere.
- **Documenti aggiornati:** `systems/floor-zero.md`, `ui/pause-menu.md`, `ui/inventory-and-synergy-screen.md`, `systems/rewards-and-economy.md`

### DEC-043 — Ostacoli generati a tema

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** gli ostacoli ambientali (vedi `secrets-and-obstacles.md`) non avevano un legame esplicito col tema di run né un budget dichiarato condiviso con i nemici.
- **Decisione:** gli ostacoli ambientali sono un tipo di contenuto generato dal tema (forma + comportamento semplice), dentro le garanzie di giocabilità: croce centrale libera, telegraph leggibili. Comprendono blocchi e pericoli passivi telegrafati. La degenerazione (DEC-024) li rende più insidiosi nei piani alti. Vincolo esplicito del proprietario: giusto bilanciamento tra ostacoli e nemici — il budget di difficoltà della stanza copre ENTRAMBI (spendere in ostacoli riduce i nemici e viceversa).
- **Alternative considerate:** Ostacoli come contenuto puramente curato, senza generazione a tema; budget separati e indipendenti per ostacoli e nemici.
- **Conseguenze:** `systems/secrets-and-obstacles.md` deve descrivere la generazione a tema degli ostacoli e le sue garanzie; `systems/rooms-and-floor-generation.md` deve fissare il budget di stanza condiviso tra ostacoli e nemici.
- **Documenti aggiornati:** `systems/secrets-and-obstacles.md`, `systems/rooms-and-floor-generation.md`

### DEC-044 — Scambio ad alto rischio a puntata generata

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** `special-rooms.md` ammette scambi a costo salute nella stanza di scambio ad alto rischio (DEC-026), ma non definiva come offerta e prezzo vengono generati né la garanzia di equità.
- **Decisione:** nella stanza di scambio l'IA genera sia l'offerta sia il prezzo dentro un budget di equità; il prezzo può essere salute (immediata o massima), un oggetto o Innesto posseduto, valuta, perfino un catalizzatore di fusione. Ogni scambio è diverso; l'equità è garantita dal budget, la validazione segue le regole di `generated-content-validation.md`.
- **Alternative considerate:** Offerte fisse curate senza generazione; prezzo limitato solo a salute o valuta principale.
- **Conseguenze:** `systems/special-rooms.md` deve descrivere la generazione di offerta e prezzo dentro il budget di equità; `systems/rewards-and-economy.md` rimanda e conferma che il negozio resta invariato (DEC-026: nessun patto a costo salute nel negozio).
- **Documenti aggiornati:** `systems/special-rooms.md`, `systems/rewards-and-economy.md`

### DEC-045 — Catalogo completo

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** `ui/main-menu.md` e `systems/save-and-meta-progression.md` trattavano il Catalogo/Museo come voce di menu senza una definizione completa delle sue funzioni.
- **Decisione:** il Catalogo (dal menu principale) è: enciclopedia consultabile di tutto il generato incontrato (schede con nome, sprite, storia, statistiche d'uso) più preferiti che pesano leggermente sulle proposte future dell'IA più il luogo dove si spendono i punti per sbloccare contenuti nei pool (DEC-015, DEC-027). Idea futura (aggiunta alla lista DEC-018): portare queste funzioni anche dentro il Piano 0/museo.
- **Alternative considerate:** Catalogo puramente consultativo, senza preferiti né spesa punti; le tre funzioni divise tra più schermate separate.
- **Conseguenze:** `ui/main-menu.md` e `systems/save-and-meta-progression.md` devono descrivere le tre funzioni del Catalogo; la lista di idee future di DEC-018 si arricchisce dell'estensione al Piano 0/museo.
- **Documenti aggiornati:** `ui/main-menu.md`, `systems/save-and-meta-progression.md`, `governance/decision-log.md` (annotazione DEC-018)

### DEC-046 — Pixel art come linguaggio canonico totale

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il gioco si appoggia già a sprite generati e a una resa a campionamento a punto negli esempi tecnici, ma non esisteva una regola di design esplicita sulla pixel art come linguaggio dell'intero gioco, UI compresa.
- **Decisione:** tutto il gioco si basa sulla pixel art, INCLUSA la UI (menu, HUD, font, indicatori): la UI è custom e in pixel art anch'essa, non un linguaggio pulito non-pixel separato. Le risoluzioni di riferimento attuali (atlas generati, resa a campionamento a punto) restano default dell'implementazione in stile DEC-019.
- **Alternative considerate:** Pixel art solo per gli elementi di gioco (nemici, oggetti, ambiente) con una UI in stile pulito non-pixel separato (scartata dal proprietario: la UI è pixel art anch'essa).
- **Conseguenze:** `content/visual-language.md` diventa la fonte unica della regola per l'intero gioco, UI compresa; `ui/hud.md` rimanda con una riga senza riformulare.
- **Documenti aggiornati:** `content/visual-language.md`, `ui/hud.md`

### DEC-047 — Il Piano 0 è il tutorial

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** mancava una risposta chiara su dove e come il gioco insegna le meccaniche base al giocatore.
- **Decisione:** la primissima visita al Piano 0 è guidata; le arene opzionali insegnano movimento, sparo, risorse e fusione con cartelli e prove pratiche. Nessun tutorial separato. Le visite successive non ripropongono la guida (le arene restano riutilizzabili per allenarsi).
- **Alternative considerate:** un tutorial separato prima del Piano 0; nessuna guida esplicita, apprendimento solo per tentativi.
- **Conseguenze:** `systems/floor-zero.md` e `03-core-loop.md` devono descrivere la primissima visita guidata.
- **Documenti aggiornati:** `systems/floor-zero.md`, `03-core-loop.md`

### DEC-048 — Valuta: drop + ricompra

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** le fonti canoniche della valuta principale e il destino degli oggetti indesiderati non erano definiti.
- **Decisione:** la valuta principale si guadagna da nemici sconfitti e stanze ripulite; il negozio ricompra oggetti e Innesti indesiderati a prezzo ridotto rispetto al valore di acquisto. Nessun'altra fonte canonica per ora.
- **Alternative considerate:** fonti aggiuntive di valuta (es. esplorazione, eventi); nessuna ricompra nel negozio, oggetti indesiderati senza uso economico.
- **Conseguenze:** `systems/rewards-and-economy.md` e `03-core-loop.md` devono descrivere fonti e ricompra.
- **Documenti aggiornati:** `systems/rewards-and-economy.md`, `03-core-loop.md`

### DEC-049 — Sprite dei personaggi

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito se i personaggi avessero sprite curati o generati, né come si comportassero gli oggetti equipaggiati sull'uno o sull'altro.
- **Decisione:** i 2-3 personaggi base (DEC-030) hanno sprite pixel art curati a mano; il personaggio generato per-run ha sprite generato dalla pipeline sprite esistente (come i nemici). I 6 slot visivi degli oggetti si sovrappongono a tutti i personaggi, base e generati.
- **Alternative considerate:** sprite generato anche per la rosa base; slot visivi diversi tra personaggi curati e generati.
- **Conseguenze:** `systems/characters.md` e `content/visual-language.md` devono descrivere la distinzione curato/generato e i 6 slot visivi comuni.
- **Documenti aggiornati:** `systems/characters.md`, `content/visual-language.md`

### DEC-050 — Sospensione ovunque con reset stanza

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era chiaro cosa succedesse alla stanza corrente quando una run veniva sospesa e ripresa.
- **Decisione:** si può sospendere la run in qualsiasi momento; al rientro la stanza corrente riparte dall'ingresso con i nemici ripristinati (niente snapshot di metà combattimento). Il resto della run (piani, build, risorse) riprende esattamente com'era.
- **Alternative considerate:** snapshot esatto di metà combattimento; sospensione consentita solo in punti prestabiliti.
- **Conseguenze:** `systems/save-and-meta-progression.md` deve descrivere la regola di ripristino della stanza, coerente con "Continua" in `05-game-states-and-flow.md` e `ui/navigation-map.md`.
- **Documenti aggiornati:** `systems/save-and-meta-progression.md`

### DEC-051 — Timer visibile + stanze-premio a tempo

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il tempo di run era mostrato solo nelle modalità competitive; mancava un meccanismo di ricompensa legato alla velocità nei piani avanzati.
- **Decisione:** il tempo di run è sempre visibile nell'HUD, in ogni modalità (il gioco si dichiara una corsa). Nei piani avanzati esistono inoltre stanze fisse che, se raggiunte abbastanza in fretta, danno ricompense, bilanciate col gameplay (soglie e ricompense esatte da playtest, stile DEC-019). È un quinto archetipo di stanza speciale, aggiunto ai quattro di DEC-010: "stanza a tempo".
- **Alternative considerate:** timer visibile solo in competitivo (comportamento precedente); nessuna ricompensa legata al tempo nei piani.
- **Conseguenze:** `ui/hud.md` deve mostrare il timer sempre; `systems/rooms-and-floor-generation.md` e `systems/special-rooms.md` devono introdurre l'archetipo "stanza a tempo"; `systems/rewards-and-economy.md` descrive la ricompensa; `governance/decision-log.md` annota DEC-010.
- **Documenti aggiornati:** `ui/hud.md`, `systems/rooms-and-floor-generation.md`, `systems/special-rooms.md`, `systems/rewards-and-economy.md`, `governance/decision-log.md` (annotazione DEC-010)

### DEC-052 — Inglese-first

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** la lingua primaria del gioco e della generazione IA non era mai stata fissata esplicitamente, mentre la pipeline attuale genera in italiano.
- **Decisione:** la lingua primaria del gioco (e della generazione IA: nomi, descrizioni, temi) è l'inglese; l'italiano resta lingua di sviluppo e test. La pipeline attuale genera contenuti in italiano: è un gap di implementazione esplicito. I documenti KB restano scritti in italiano, lingua di lavoro della KB, non del gioco.
- **Alternative considerate:** italiano come lingua primaria definitiva del gioco; bilinguismo nativo fin da subito.
- **Conseguenze:** `content/narrative-tone.md` e `06-ai-content-generation-model.md` devono registrare la regola e il gap noto.
- **Documenti aggiornati:** `content/narrative-tone.md`, `06-ai-content-generation-model.md`

### DEC-053 — Roster nemici compatto

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito quanti tipi di nemici distinti un tema generasse per l'intera run, con il rischio di un flusso illeggibile di nemici sempre nuovi.
- **Decisione:** il tema genera 6-8 tipi di nemici per l'intera run, distribuiti sui piani e potenziati dalla degenerazione (gradi crescenti, DEC-024; Veterani): l'apprendimento dei pattern è parte del design (coerente con la difficoltà unica, DEC-038).
- **Alternative considerate:** numero illimitato di tipi di nemici generati per run; roster fisso identico per ogni tema.
- **Conseguenze:** `systems/enemies.md` deve descrivere il roster compatto e la sua relazione con i ruoli tattici e la degenerazione.
- **Documenti aggiornati:** `systems/enemies.md`

### DEC-054 — Boss tutti generati

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era esplicito se alcuni boss potessero essere fissi/curati come identità ricorrente del gioco, a differenza della rosa di personaggi base.
- **Decisione:** tutti e 5 i boss della run sono generati dal tema e validati nelle bande boss, con l'escalation di DEC-028 (piano 1 fase singola, dal 3 due fasi, il 5 il più complesso). Nessun boss fisso del gioco; il fallback curato resta comunque una rete di sicurezza in caso di generazione non valida, non un'eccezione a questa regola.
- **Alternative considerate:** uno o più boss fissi e curati come identità ricorrente del gioco (es. il boss del piano 5 sempre lo stesso).
- **Conseguenze:** `systems/bosses.md` deve chiarire che tutti i boss sono generati e distinguere questa regola dal fallback curato.
- **Documenti aggiornati:** `systems/bosses.md`

### DEC-055 — Arene del Piano 0 senza conseguenze

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era esplicito cosa perdesse il giocatore sconfitto in un'arena di sfida del Piano 0, rischiando di introdurre rischio percepito in quello che deve restare un rifugio sicuro.
- **Decisione:** la sconfitta in un'arena del Piano 0 non ha alcun costo — l'arena è una simulazione: il giocatore esce sconfitto ma illeso, perde solo la dote (DEC-029) di quell'arena. Il Piano 0 resta a rischio zero.
- **Alternative considerate:** penalità sulla run in preparazione alla sconfitta in arena; perdita di risorse o di tempo oltre alla sola dote mancata.
- **Conseguenze:** `systems/floor-zero.md` deve descrivere l'assenza di costo alla sconfitta in arena.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-056 — Schermata risultati completa

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** `RunResults` mostrava solo tempo, punteggio e punti sblocco, senza un quadro completo della run appena conclusa.
- **Decisione:** i risultati di fine run mostrano, oltre a tempo e punteggio: (1) la timeline della run per piano (oggetti presi, fusioni, boss, tempi parziali); (2) le nuove scoperte entrate nel catalogo, con i candidati al museo evidenziati; (3) il riepilogo punti (base + prove) con scorciatoia diretta alla spesa nel Catalogo; (4) il confronto con le run passate (miglior tempo, medie, record personali per tema e personaggio).
- **Alternative considerate:** schermata risultati minimale, solo esito/tempo/punteggio; confronto con le run passate spostato in una schermata separata del Catalogo.
- **Conseguenze:** `ui/results-and-leaderboards.md` deve descrivere i quattro elementi aggiuntivi della schermata risultati.
- **Documenti aggiornati:** `ui/results-and-leaderboards.md`

### DEC-057 — Parità rigorosa di input

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non esisteva un vincolo esplicito che garantisse la stessa esperienza a tastiera e a controller su tutte le schermate.
- **Decisione:** ogni scelta di design deve funzionare in modo identico su tastiera e controller (vincolo esplicito nei documenti UI); il mouse è ammesso solo nei menu. Nessuna meccanica può richiedere un dispositivo specifico.
- **Alternative considerate:** parità solo raccomandata, non vincolante; mouse ammesso anche in `Gameplay` per meccaniche opzionali.
- **Conseguenze:** `ui/options-and-accessibility.md` diventa fonte unica della regola; `ui/navigation-map.md` vi rimanda con una riga.
- **Documenti aggiornati:** `ui/options-and-accessibility.md`, `ui/navigation-map.md`

### DEC-058 — Garanzie di accessibilità canoniche

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** l'accessibilità era elencata solo come lista di voci da progettare, senza garanzie approvate e vincolanti.
- **Decisione:** tre garanzie canoniche: (1) rimappatura totale di ogni input su tastiera e pad; (2) nessuna informazione di gioco affidata al solo colore (forme e pattern distinti, si aggancia al budget di leggibilità); (3) opzione di riduzione effetti (particelle, scuotimenti, lampi, anche per fotosensibilità) che non altera le informazioni di gioco. La "modalità assistita" (riduzione difficoltà) non è nel canone.
- **Alternative considerate:** accessibilità come insieme di raccomandazioni non vincolanti; modalità assistita di riduzione difficoltà inclusa nel canone.
- **Conseguenze:** `ui/options-and-accessibility.md` deve distinguere le tre garanzie canoniche dalle voci ancora da progettare, ed escludere esplicitamente la modalità assistita.
- **Documenti aggiornati:** `ui/options-and-accessibility.md`

### DEC-059 — Ricarica degli attivi a doppio canale

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il metodo di ricarica degli attivi a cariche era lasciato generico ("una fonte esterna"), senza canali di base garantiti.
- **Decisione:** gli oggetti attivi si ricaricano sia completando stanze sia raccogliendo energia droppata dai nemici (entrambi i canali attivi di base; il dosaggio è parte del budget dell'oggetto). Alcuni oggetti possono aggiungere modi di ricarica ulteriori (sistema estensibile per design, anche dai contenuti generati, dentro validazione).
- **Alternative considerate:** un solo canale di ricarica di base; ricarica esclusivamente a tempo (cooldown) per tutti gli attivi a cariche.
- **Conseguenze:** `systems/active-items.md` deve descrivere i due canali di base e l'estensibilità; `governance/glossary.md` introduce il termine "energia".
- **Documenti aggiornati:** `systems/active-items.md`, `governance/glossary.md`

### DEC-060 — Punteggio composito multi-percorso

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il punteggio era citato solo come voce generica di ricompensa, senza una composizione dichiarata né un vincolo di equità tra stili di gioco diversi.
- **Decisione:** il punteggio somma piccoli bonus da: tempo, prove/sfide, esplorazione, scoperte, eliminazioni e Veterani. Vincolo di bilanciamento esplicito del proprietario: percorsi diversi devono restare competitivi — chi completa i 5 piani con il minor numero di stanze visitate e nel minor tempo riceve un bonus di efficienza; chi esplora tutto e impiega più tempo accumula comunque bonus per tutto ciò che fa. Il bilanciamento fine è da playtest (open question).
- **Alternative considerate:** punteggio basato solo sul tempo; punteggio che penalizza l'esplorazione rispetto alla velocità, senza bonus di efficienza dedicato al percorso rapido.
- **Conseguenze:** `systems/rewards-and-economy.md` diventa fonte del punteggio composito; `08-multiplayer-and-competition.md` e `ui/results-and-leaderboards.md` vi rimandano per la metrica punteggio delle classifiche.
- **Documenti aggiornati:** `systems/rewards-and-economy.md`, `08-multiplayer-and-competition.md`, `ui/results-and-leaderboards.md`

### DEC-061 — Danno da contatto dichiarato dalla forma

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito se tutti i nemici infliggessero danno al solo contatto fisico, indipendentemente dal loro aspetto.
- **Decisione:** solo i nemici la cui forma lo telegrafa (spine, corpi ustionanti, ecc.) feriscono al contatto; gli altri al contatto spingono ma non feriscono. La lettura visiva decide: si aggancia al vocabolario delle forme dei nemici e al budget di leggibilità.
- **Alternative considerate:** tutti i nemici infliggono danno al contatto indipendentemente dalla forma; danno da contatto dichiarato da un tag invisibile, non dalla silhouette.
- **Conseguenze:** `systems/enemies.md` deve descrivere la regola e la sua validazione in generazione.
- **Documenti aggiornati:** `systems/enemies.md`

### DEC-062 — Tre istanze di Classificata + classifiche divise per metrica

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-021 fissava solo due istanze di gara (stesso seed / seed diversi) per ciascuna Modalità, senza una classifica pubblica giornaliera né una regola esplicita su come si combinano tempo e punteggio in classifica.
- **Decisione:** estende DEC-021. La modalità Classificata esiste in tre istanze: (a) sfida a stesso seed — i contendenti affrontano la stessa run generata; (b) sfida a seed diversi — run generate casualmente, diverse per ciascuno, con componente dichiarata di casualità; (c) Classificata giornaliera pubblica ("Daily") — una run generata scelta dallo sviluppatore, che cambia ogni giorno, stesso seed per tutti i giocatori, con classifica globale giornaliera. Le classifiche della Classificata sono divise per metrica: una graduatoria per il tempo e una per il punteggio, non un punteggio combinato.
- **Alternative considerate:** solo due istanze di gara come in DEC-021, senza Daily; classifica unica con punteggio combinato di tempo e punteggio.
- **Conseguenze:** la Modalità Leggera resta a due istanze; la Daily esiste solo in Classificata. `08-multiplayer-and-competition.md` diventa fonte unica delle tre istanze e della divisione per metrica; `ui/multiplayer-lobby.md` e `ui/results-and-leaderboards.md` vi rimandano.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `ui/multiplayer-lobby.md`, `ui/results-and-leaderboards.md`, `governance/decision-log.md` (annotazione DEC-021)

### DEC-063 — Museo curato in modo misto

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito il criterio di ingresso nel museo del Piano 0 oltre alla generica idea di "creazioni migliori".
- **Decisione:** le creazioni entrano nel museo del Piano 0 per promozione automatica basata su metriche (uso, sopravvivenza col giocatore, contributo alle vittorie) E per scelta del giocatore: i contenuti marcati come preferiti nel Catalogo (DEC-045) hanno la precedenza e non escono mai dal museo. Il museo è metà specchio delle metriche, metà curatela del giocatore.
- **Alternative considerate:** museo interamente automatico per metriche, senza intervento del giocatore; museo interamente curato a mano dal giocatore, senza promozione automatica.
- **Conseguenze:** `systems/floor-zero.md` e `systems/save-and-meta-progression.md` devono descrivere il criterio misto e il legame coi preferiti del Catalogo; risolve la domanda aperta su cosa mostri il museo (intero catalogo o sottoinsieme curato).
- **Documenti aggiornati:** `systems/floor-zero.md`, `systems/save-and-meta-progression.md`

### DEC-064 — Ricompense della Daily: cosmetici e riconoscimenti

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-062 introduceva la Classificata giornaliera pubblica ("Daily") senza definire se avesse ricompense dedicate oltre alla classifica.
- **Decisione:** la Daily (DEC-062) premia con medaglie/cornici visibili nel profilo e nel museo, legate a piazzamenti e streak di partecipazione. NON assegna punti sblocco né tocca l'economia dei punti (DEC-015/DEC-027 restano intatte: sblocchi solo singleplayer). Risolve la parte "ricompense dedicate?" della domanda aperta sulla Daily; l'orario di rotazione resta aperto.
- **Alternative considerate:** nessuna ricompensa oltre alla classifica; ricompense che assegnano punti sblocco anche in competitivo (scartata, romperebbe DEC-015).
- **Conseguenze:** `08-multiplayer-and-competition.md` diventa fonte unica delle ricompense della Daily; `systems/save-and-meta-progression.md` e `ui/results-and-leaderboards.md` registrano la persistenza cosmetica nel profilo/risultati; `ui/multiplayer-lobby.md` e `governance/open-questions.md` si aggiornano di conseguenza.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `systems/save-and-meta-progression.md`, `ui/results-and-leaderboards.md`, `ui/multiplayer-lobby.md`, `governance/open-questions.md`

### DEC-065 — Card di scoperta breve

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito come il gioco comunica al giocatore l'incontro con un contenuto generato mai visto prima.
- **Decisione:** alla prima occorrenza di un contenuto generato mai visto (oggetto, nemico, boss, sinergia/fusione) il gioco mostra una card rapida — sprite, nome, una riga — che NON mette in pausa e non blocca l'input; i dettagli completi vivono nella scheda del Catalogo. Una sola card alla volta; se ne arrivano più insieme, si accodano senza invadere lo schermo (casi limite da specificare).
- **Alternative considerate:** nessun annuncio dedicato, solo la voce nel Catalogo a fine run; card che mette in pausa il gioco per essere letta con calma (scartata, romperebbe il ritmo dichiarato dal timer sempre visibile, DEC-051).
- **Conseguenze:** `ui/hud.md` diventa fonte unica dell'elemento; `06-ai-content-generation-model.md` vi rimanda con una riga.
- **Documenti aggiornati:** `ui/hud.md`, `06-ai-content-generation-model.md`

### DEC-066 — Condivisione run a due vie

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** la condivisione di una run tra giocatori era menzionata solo genericamente ("identificatore condivisibile" in RunResults, "codice/manifest" in RunSetup) senza distinguere le forme possibili né la loro classificabilità.
- **Decisione:** una run è condivisibile (fuori dalle classifiche) in due modi: (a) codice breve testuale (seed più versione di gioco) da incollare in Run Setup — richiede che chi riceve possa rigenerare i contenuti; (b) file RunBundle esportato (formato con verifica d'integrità già esistente nel progetto) — via completa e verificabile, adatta a gare private e archivio. Le run condivise così sono sempre non classificate (coerenza DEC-062: la Classificata passa dalle gare pubblicate/Daily).
- **Alternative considerate:** un'unica forma di condivisione (solo codice o solo bundle); permettere che una run condivisa possa comunque entrare in classifica.
- **Conseguenze:** `08-multiplayer-and-competition.md` diventa fonte unica della regola; `ui/run-setup.md`, `systems/run-manifest-and-reproducibility.md` e `ui/multiplayer-lobby.md` vi rimandano.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `ui/run-setup.md`, `systems/run-manifest-and-reproducibility.md`, `ui/multiplayer-lobby.md`

### DEC-067 — Cornice narrativa: "il crogiolo dei mondi"

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** `content/narrative-tone.md` era un placeholder esplicito: senza una cornice narrativa minima, hub, museo, temi e fusione restavano senza un senso concettuale comune, pur essendo già tutti approvati singolarmente.
- **Decisione:** il Piano 0 è un luogo-fucina fuori dal tempo dove i mondi generati nascono, si fondono e si sciolgono; il giocatore è chi vi si immerge per esplorarli prima che collassino; il museo è la memoria di ciò che ha salvato. La degenerazione dei piani (DEC-024) è il collasso progressivo del mondo visitato; la fusione (DEC-012) è la pratica stessa del crogiolo. Questa è la cornice minima canonica: dà senso a hub, museo, temi e fusione senza imporre lore fissa ai temi generati, che restano liberi. `content/narrative-tone.md` smette di essere un placeholder: registra la cornice, il campo semantico (fusione/scioglimento/crogiolo) e il vincolo che i temi generati non devono contraddirla.
- **Alternative considerate:** nessuna cornice narrativa esplicita, lasciando hub/museo/fusione senza un senso concettuale comune; una lore completa e definitiva fin da subito.
- **Conseguenze:** `content/narrative-tone.md` passa da placeholder puro a documento approvato per la cornice, con tono specifico e nome definitivo ancora draft/aperti; `00-vision.md` e `systems/floor-zero.md` richiamano la cornice con una riga.
- **Documenti aggiornati:** `content/narrative-tone.md`, `00-vision.md`, `systems/floor-zero.md`

### DEC-068 — Colpo firmato "a volte" per il personaggio generato

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il personaggio alternativo generato per run (DEC-014, DEC-030) aveva già un trait unico comportamentale (DEC-037), ma non era definito se potesse avere anche un proprio tipo di colpo dedicato, distinto dal colpo standard.
- **Decisione:** il personaggio generato per-run PUÒ avere un tipo di colpo proprio generato (forma più comportamento, "colpo firmato"): è una possibilità del generatore, parte del budget del personaggio — chi ha il colpo firmato ha statistiche più caute, chi non ce l'ha usa il colpo standard. I personaggi base usano sempre colpi standard curati. Si aggancia a DEC-037 (comportamenti Lua per i colpi) e alle bande di bilanciamento dei personaggi (DEC-033).
- **Alternative considerate:** colpo firmato garantito per ogni personaggio generato; colpo firmato riservato anche ai personaggi della rosa base.
- **Conseguenze:** `systems/characters.md` diventa fonte unica del meccanismo e del suo effetto sul budget del personaggio; `systems/combat-and-projectiles.md` vi rimanda con una riga.
- **Documenti aggiornati:** `systems/characters.md`, `systems/combat-and-projectiles.md`

### DEC-069 — Migrazione del catalogo tra versioni: le Reliquie

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** il catalogo persistente (DEC-015) non aveva una regola per cosa succede ai contenuti generati quando il gioco si aggiorna e le regole di validazione cambiano.
- **Decisione:** a ogni aggiornamento del gioco il catalogo viene riconvalidato con la pipeline di validazione (stessi sei stati di `generated-content-validation.md`): ciò che passa resta giocabile e sbloccabile; ciò che fallisce viene archiviato in una sezione "Reliquie" del Catalogo, solo consultabile (scheda visibile, non giocabile, non sbloccabile nei pool). La memoria del giocatore non si perde mai; il termine "Reliquie" è coerente con la cornice del crogiolo (DEC-067).
- **Alternative considerate:** eliminare del tutto dal catalogo i contenuti che non superano la riconvalida; non riconvalidare mai il catalogo tra versioni.
- **Conseguenze:** `systems/save-and-meta-progression.md` diventa fonte unica delle Reliquie; `ui/main-menu.md` e `systems/generated-content-validation.md` vi rimandano con una riga.
- **Documenti aggiornati:** `systems/save-and-meta-progression.md`, `ui/main-menu.md`, `systems/generated-content-validation.md`

### DEC-070 — Primo avvio: benchmark più scelta binaria

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** non era definito come il gioco introduce per la prima volta la generazione IA, né cosa succede se l'hardware del giocatore non la regge.
- **Decisione:** al primo avvio il gioco misura l'hardware (il benchmark esiste già nel progetto) e propone una scelta binaria: esperienza completa (scarica/attiva i modelli IA — un solo set di modelli, non esistono alternative) oppure solo curato (nessun modello: si gioca con i contenuti curati e il fallback procedurale, la generazione può essere attivata in seguito). Nessun tier intermedio di download. Se l'hardware non regge la generazione, il gioco lo dice chiaramente e consiglia "solo curato" (coerente con la garanzia: il gioco è sempre giocabile, DEC-002/DEC-020). La scelta avviene al primissimo avvio, prima della visita guidata al Piano 0 (DEC-047).
- **Alternative considerate:** tier intermedi di download/qualità dei modelli; nessuna scelta esplicita, generazione sempre attiva di default indipendentemente dall'hardware.
- **Conseguenze:** `systems/floor-zero.md` diventa fonte unica di dove e quando avviene la scelta; `06-ai-content-generation-model.md` registra "solo curato" come stato legittimo e permanente, non un fallback temporaneo.
- **Documenti aggiornati:** `systems/floor-zero.md`, `06-ai-content-generation-model.md`

### DEC-071 — Il nome del gioco è "Worldsmelt"

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** DEC-003 lasciava il nome del gioco come titolo di lavoro provvisorio, in attesa di una scelta del proprietario del progetto.
- **Decisione:** il titolo definitivo del gioco è **Worldsmelt**, scelto dal proprietario e verificato libero da collisioni con giochi esistenti (ricerca web del 18/07/2026; scartati per collisione "Forgefall" ed "Everforge"; nessun match trovato per "Worldsmelt"). "Melting Run" resta solo il nome storico di questo repository e il titolo di lavoro citato in questo registro delle decisioni; nei documenti vivi della KB il gioco è Worldsmelt.
- **Alternative considerate:** "Forgefall" (scartato, collisione con giochi esistenti); "Everforge" (scartato, collisione con giochi esistenti); mantenere il nome provvisorio a tempo indeterminato.
- **Conseguenze:** risolve DEC-003 e la domanda aperta sul nome definitivo del gioco. Apre la strada alla nomenclatura inglese in-game dei termini di lavoro (vedi DEC-072).
- **Documenti aggiornati:** `README.md`, `PROJECT_BRIEF.md`, `00-vision.md`, `content/narrative-tone.md`, `governance/open-questions.md`, `governance/decision-log.md` (annotazione DEC-003)

### DEC-072 — Nomenclatura inglese di gioco

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** con il titolo definitivo fissato (DEC-071) e la cornice del crogiolo già approvata (DEC-067), i termini di lavoro placeholder della KB (risorse, Innesto, Veterano, ecc.) potevano finalmente ricevere un nome inglese in-game coerente.
- **Decisione:** confermata dal proprietario una tabella canonica di nomenclatura inglese in-game, coerente con la cornice del crogiolo: Piano 0/crogiolo → The Crucible; Tema della run → World; Fusione esplicita → Smelting; Stanza di fusione → Smeltery; Catalizzatore di fusione → Flux; Valuta principale → Ingots; Punti sblocco (meta) → Embers; Energia di ricarica attivi → Heat; Salute temporanea → Crust; Strumento di breccia → Blast Charges; Strumento di apertura → Cast Keys; Innesto → Graft; Veterano → Tempered; Reliquie → Relics; Colpo firmato → Signature Shot; Card di scoperta → Discovery Card; Daily → Daily Smelt. La KB resta scritta in italiano (DEC-052): i termini di lavoro italiani restano in uso nei documenti; il glossario diventa la mappa bilingue, con il nome inglese accanto a ogni termine di lavoro interessato. I nomi placeholder smettono di essere placeholder: la domanda aperta sui nomi definitivi di risorse/Innesto/Veterano è risolta.
- **Alternative considerate:** lasciare i nomi di interfaccia in italiano anche in gioco; rimandare la nomenclatura a un secondo momento, oltre il lancio del nome del gioco.
- **Conseguenze:** `governance/glossary.md` diventa la fonte unica della mappa bilingue; `ui/hud.md` e `systems/health-and-resources.md` mostrano i nomi in-game accanto ai termini di lavoro, senza duplicare la mappa.
- **Documenti aggiornati:** `governance/glossary.md`, `content/narrative-tone.md`, `ui/hud.md`, `systems/health-and-resources.md`, `governance/open-questions.md`, `governance/decision-log.md` (annotazione DEC-013)

### DEC-073 — La nomenclatura non contamina la generazione dei mondi

- **Data:** 2026-07-18
- **Stato:** approved
- **Contesto:** con la nomenclatura di interfaccia fissata (DEC-072), serviva chiarire il confine tra il vocabolario di fonderia dell'interfaccia/cornice e il contenuto generato per i World di ogni run, per evitare che i prompt di generazione venissero contaminati da termini che appartengono solo al Crucible.
- **Decisione:** due regole. (a) I nomi di gioco della nomenclatura (Smelting, Flux, Tempered, ecc.) non entrano nei prompt di generazione dei contenuti dei World (sprite, nemici, boss, oggetti, stanze): quei prompt descrivono funzione e tema del World scelto, non il vocabolario di fonderia dell'interfaccia. (b) Le risorse fisse (Ingots, Cast Keys, Blast Charges, Crust, Flux, Heat) hanno una silhouette iconica stabile tra le run: la variazione per-World è ammessa solo in palette e dettagli, dentro il budget di leggibilità (coerente con DEC-058: mai informazione dal solo colore). Nota gap: il codice attuale genera le icone di valuta/chiave/bomba/cuore per-run con il tema, senza silhouette stabile — registrato come gap di implementazione esplicito (stile DEC-009/DEC-052).
- **Alternative considerate:** lasciare che i prompt dei World includano liberamente il vocabolario di fonderia dell'interfaccia; icone delle risorse fisse completamente rigenerate a ogni World senza vincolo di silhouette.
- **Conseguenze:** `06-ai-content-generation-model.md` fissa la regola (a) per i prompt dei World; `content/visual-language.md` fissa la regola (b) e registra il gap di implementazione.
- **Documenti aggiornati:** `06-ai-content-generation-model.md`, `content/visual-language.md`

### DEC-074 — L'abbandono del Piano 0 passa da ExitConfirm

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la mappa canonica dei 9 stati non prevedeva un arco FloorZero → MainMenu; l'implementazione M5 aveva realizzato l'abbandono con ESC → ExitConfirm senza copertura di design, registrandolo come domanda aperta (ex domanda 14).
- **Decisione:** sancito ESC → ExitConfirm come arco canonico di abbandono del Piano 0: la conferma protegge la preparazione già fatta (tema e personaggio scelti, generazione in corso) ed è coerente con l'uscita confermata dagli altri stati. Confermato l'abbandono interrompe la preparazione e riporta al menu principale.
- **Alternative considerate:** arco diretto FloorZero → MainMenu senza conferma; nessun abbandono possibile dal Piano 0 (solo entrare in run o chiudere il gioco).
- **Conseguenze:** la mappa canonica degli stati acquisisce l'arco FloorZero → ExitConfirm; l'implementazione M5 è già conforme.
- **Documenti aggiornati:** `05-game-states-and-flow.md`, `ui/navigation-map.md`, `systems/floor-zero.md`

### DEC-075 — Il Piano 0 conta come menu per il mouse

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-057 ammette il mouse solo nei menu; l'implementazione M5 aveva lasciato le carte tema selezionabili solo con tastiera/pad proprio per non decidere in silenzio se il Piano 0 fosse un "menu" (ex domanda 16).
- **Decisione:** il Piano 0 conta come menu ai fini dell'ammissione del mouse: gli elementi di interfaccia selezionabili del Piano 0 (carte tema, schede personaggio, pannelli) sono cliccabili come le voci di menu degli altri stati. Il movimento del personaggio nel Piano 0 e ogni meccanica giocata restano su tastiera/controller con parità rigorosa: il mouse non muove il personaggio e non è mai richiesto (DEC-057 resta intatta nel suo nucleo).
- **Alternative considerate:** Piano 0 eccezione senza mouse (sancire l'implementazione attuale); mouse ammesso solo nei pannelli overlay ma non sulle carte.
- **Conseguenze:** `ui/options-and-accessibility.md` (fonte unica di DEC-057) chiarisce il perimetro della regola; `systems/floor-zero.md` aggiorna input/azioni e scenari. Gap di implementazione esplicito (stile DEC-009/DEC-052): le carte tema e le schede personaggio di M5/M6a vanno rese cliccabili.
- **Documenti aggiornati:** `ui/options-and-accessibility.md`, `systems/floor-zero.md`

### DEC-076 — Tre carte tema curate di fallback

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** quando nessuna proposta di tema dell'IA supera la validazione, l'implementazione M5 mostrava 3 carte curate come default proposto (ex domanda 17).
- **Decisione:** il numero canonico di carte tema curate di fallback è **3**, come le proposte ordinarie: il giocatore non percepisce differenza di forma tra proposta generata e fallback curato.
- **Alternative considerate:** 2 carte (fallback più sobrio); più di 3 (l'intero pool curato disponibile).
- **Conseguenze:** il default proposto M5 diventa canone; vincola il pool curato minimo dei temi (almeno 3, vedi DEC-087).
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-077 — Il codice di condivisione porta anche tema e personaggio

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** il codice breve di condivisione run (DEC-066) codificava solo seed più versione di gioco: chi lo riceveva rigiocava "una run con lo stesso seed", non necessariamente con lo stesso tema e personaggio scelti nel Piano 0 (ex domanda 15).
- **Decisione:** il codice breve si estende: codifica **seed, versione di gioco, tema scelto e personaggio scelto**, così chi lo riceve può rigiocare esattamente la stessa run. Le scelte trasportate arrivano nel Piano 0 come preselezione; il giocatore resta libero di cambiarle (a quel punto sta giocando una run con lo stesso seed, non più la stessa run — il Piano 0 resta il luogo della scelta). La run importata resta sempre non classificata (DEC-066/DEC-062 invariate). Il RunBundle, che trasporta già il manifest completo, resta la via completa e verificabile.
- **Alternative considerate:** solo seed come design voluto (ognuno fa le proprie scelte sulla stessa base); due formati di codice (corto solo-seed e completo).
- **Conseguenze:** `systems/run-manifest-and-reproducibility.md` (fonte unica del codice breve) aggiorna il contenuto del codice; glossario e documenti che citano "seed più versione di gioco" si allineano. Gap di implementazione esplicito: il codice attuale non trasporta ancora tema e personaggio.
- **Documenti aggiornati:** `systems/run-manifest-and-reproducibility.md`, `08-multiplayer-and-competition.md`, `governance/glossary.md`, `ui/run-setup.md`

### DEC-078 — Lo sconto del colpo firmato è una compressione fissa delle bande

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-068 chiedeva "statistiche più caute" per il personaggio con colpo firmato senza quantificare; l'implementazione M6b-3 usava una compressione fissa delle bande a fattore 0.6 come default proposto (ex domanda 18).
- **Decisione:** il criterio canonico dello sconto è la **compressione fissa delle bande**: danno/salute/fortuna compressi verso il bordo cauto, cadenza compressa verso il lato lento, velocità del colpo e del movimento intatte (il colpo firmato paga il proprio vantaggio offensivo, non la mobilità). Il **valore** del fattore (0.6 attuale) resta un default proposto da validare col playtest, stile DEC-019.
- **Alternative considerate:** sconto proporzionale al budget di potenza del colpo firmato (stile ShotTypePower); ibrido fisso più correzione sul budget.
- **Conseguenze:** `systems/characters.md` promuove il criterio a canone; il valore del fattore entra nell'elenco dei valori numerici da playtest in `governance/open-questions.md`.
- **Documenti aggiornati:** `systems/characters.md`, `governance/open-questions.md`

### DEC-079 — Il colpo firmato non si scarta mai: si normalizza

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la ex domanda 19 chiedeva se, al fallback dal colpo firmato al colpo standard, le statistiche caute restassero o tornassero alle bande piene (doppia penalità contro colpo standard "gratis"). L'implementazione M6b-3 normalizza già sempre il colpo in banda con la stessa doppia rete dei colpi di run, senza mai scartarlo.
- **Decisione:** sancito: il colpo firmato attraversa sempre la doppia rete di bilanciamento, che **normalizza e non scarta mai** — un colpo fuori banda viene riportato in banda, mai sostituito dal colpo standard. Le statistiche caute non vengono mai ricalcolate retroattivamente. Un personaggio generato con colpo firmato ha sempre il suo colpo firmato: il dilemma della doppia penalità sparisce alla radice.
- **Alternative considerate:** fallback al colpo standard con ritorno alle bande piene; fallback al colpo standard con statistiche caute mantenute.
- **Conseguenze:** il blocco "colpo che non valida" di `systems/characters.md` passa da default proposto a canone; la ex domanda 19 è chiusa.
- **Documenti aggiornati:** `systems/characters.md`

### DEC-080 — La rosa base ha nomi e ruoli canonici

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** l'implementazione M6a proponeva tre personaggi come default (ex domanda 7, che chiedeva la composizione esatta della rosa DEC-030).
- **Decisione:** nomi e ruoli approvati come canone, in nomenclatura inglese coerente con DEC-072: **Wayfinder** (esploratore, personaggio di partenza e default: il feel storico del progetto, più fortuna), **Ashblade** (offensivo di vetro: danno alto, tetto vita basso), **Bulwark** (difensivo di roccia: lento, tetto vita alto). Le statistiche esatte restano default proposti da playtest; le condizioni di sblocco di Ashblade e Bulwark restano da definire. La domanda aperta si restringe a statistiche e sblocchi.
- **Alternative considerate:** tenerli provvisori; approvare subito anche le condizioni di sblocco; rivedere nomi o ruoli.
- **Conseguenze:** `systems/characters.md` promuove nomi e ruoli a canone; il glossario registra i tre nomi.
- **Documenti aggiornati:** `systems/characters.md`, `systems/floor-zero.md`, `governance/glossary.md`

### DEC-081 — La Daily ruota a mezzanotte UTC

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-062 fissava che la Classificata giornaliera pubblica ("Daily") usa lo stesso seed per tutti e cambia ogni giorno, senza orario (ex domanda 9).
- **Decisione:** la rotazione della Daily avviene alle **00:00 UTC**, un istante globale unico per tutti i giocatori (lo standard dei roguelike con daily run). La lobby mostra il conto alla rovescia al prossimo cambio.
- **Alternative considerate:** mezzanotte locale (seed per data di calendario del giocatore); orario fisso europeo (es. 10:00 CET).
- **Conseguenze:** `08-multiplayer-and-competition.md` (fonte unica della Daily) registra l'orario; `ui/multiplayer-lobby.md` aggiorna il feedback del conto alla rovescia.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `ui/multiplayer-lobby.md`

### DEC-082 — Abbandono e reroll contano come sconfitta per i punti

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-041 fissava cosa resta al giocatore solo per vittoria (punti pieni) e sconfitta (punti ridotti); l'abbandono volontario (ExitConfirm) e il reroll da Gameplay non erano classificati (ex domanda 20, emersa dal substrato del catalogo M7).
- **Decisione:** abbandono volontario e reroll contano entrambi come **sconfitta** ai fini dei punti sblocco: punti ridotti standard su quanto maturato fino a quel momento, nessuna categoria intermedia. Catalogo e statistiche si aggiornano comunque con quanto incontrato (regola già esistente: una run interrotta a metà non corrompe il profilo). Il farming è mitigato dal fatto che i punti maturano giocando: chi abbandona presto ha maturato poco. Perimetro: la regola riguarda una run in corso (`ExitConfirm` da `PauseMenu`, reroll da `Gameplay`); l'abbandono della sola preparazione nel Piano 0 (`ExitConfirm` da `FloorZero`, DEC-074) avviene prima che la run giocata cominci, non conta come sconfitta e resta fuori da questa contabilità.
- **Alternative considerate:** nessun punto (fuori dall'economia); categoria propria con punti ulteriormente ridotti; trattare abbandono e reroll in modo diverso tra loro.
- **Conseguenze:** `ui/results-and-leaderboards.md` (dettaglio di DEC-041) e `systems/save-and-meta-progression.md` registrano la regola.
- **Documenti aggiornati:** `ui/results-and-leaderboards.md`, `systems/save-and-meta-progression.md`

### DEC-083 — Le sette categorie del Catalogo

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la vista Catalogo M8 esponeva sette categorie consultabili come scelta di implementazione (ex domanda 21: layout stanza e tipi di colpo meritano una scheda o restano record interni?).
- **Decisione:** approvate le **sette categorie consultabili**: Oggetti, Nemici, Boss, Personaggi, Mondi, Layout, Colpi. Anche i layout stanza e i tipi di colpo sono creazioni del crogiolo che il giocatore ha incontrato: hanno una scheda propria come il resto.
- **Alternative considerate:** cinque categorie (Layout e Colpi solo record interni); sei categorie (Colpi consultabili, Layout interno).
- **Conseguenze:** `systems/save-and-meta-progression.md` (fonte unica del catalogo) registra le categorie; `ui/main-menu.md` le riflette.
- **Documenti aggiornati:** `systems/save-and-meta-progression.md`, `ui/main-menu.md`

### DEC-084 — Il Catalogo è una vista interna del menu principale

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** M8 aveva collocato la schermata Catalogo dentro lo stato MainMenu per non introdurre un decimo stato senza decisione esplicita (ex domanda 22); `ui/main-menu.md` la chiamava "schermata", linguaggio da stato proprio, creando un'incoerenza da risolvere.
- **Decisione:** la mappa canonica resta a **9 stati**; il Catalogo vive dentro MainMenu come **vista interna**. Nei documenti, "schermata" descrive l'esperienza del giocatore, non uno stato dell'applicazione: la scelta M8 è sancita.
- **Alternative considerate:** promuovere il Catalogo a decimo stato canonico con archi propri (MainMenu ⇄ Catalogo).
- **Conseguenze:** `05-game-states-and-flow.md` annota la vista interna; `ui/main-menu.md` chiarisce il linguaggio; `ui/navigation-map.md` resta coerente.
- **Documenti aggiornati:** `05-game-states-and-flow.md`, `ui/main-menu.md`, `ui/navigation-map.md`

### DEC-085 — Reliquie nel museo: la curatela vince, le metriche decadono

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** un contenuto promosso al museo del Piano 0 (per metriche o come preferito, DEC-063) può diventare una Reliquia dopo un aggiornamento (DEC-069); non era definito se restasse esposto (ex domanda 13).
- **Decisione:** se il contenuto è nel museo **come preferito del giocatore, resta esposto** — la curatela non esce mai finché marcata (coerente con DEC-063); la scheda museale segnala che è una Reliquia, e la prova in arena (DEC-040) non è più disponibile, perché le Reliquie non sono giocabili (DEC-069). Se era promosso **solo per metriche, esce automaticamente**: la promozione automatica riguarda contenuti ancora in circolazione.
- **Alternative considerate:** resta sempre esposto (il museo come pura memoria); esce sempre (le Reliquie vivono solo nel Catalogo).
- **Conseguenze:** `systems/floor-zero.md` (museo) e `systems/save-and-meta-progression.md` (Reliquie) registrano la regola con uno scenario.
- **Documenti aggiornati:** `systems/floor-zero.md`, `systems/save-and-meta-progression.md`

### DEC-086 — Il primo avvio è una schermata dedicata; la riattivazione vive in Impostazioni

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-070 fissava principio e momento della scelta binaria completo/solo-curato (dopo il benchmark, prima della visita guidata al Piano 0), non l'interfaccia né il punto di riattivazione della generazione (ex domanda 12).
- **Decisione:** la scelta è una **schermata dedicata a due carte** (esperienza completa / solo curato) al primissimo avvio, dopo il benchmark. Nessun default silenzioso: se il giocatore annulla o chiude il gioco senza scegliere, al rientro la schermata si ripresenta finché una scelta non è fatta. La **riattivazione** della generazione per chi ha scelto solo curato vive in **Impostazioni**, accompagnata dalla stessa informazione del benchmark sull'hardware.
- **Alternative considerate:** overlay sopra il menu principale; rimandare la decisione sull'interfaccia.
- **Conseguenze:** `systems/floor-zero.md` (fonte unica di dove/quando avviene la scelta, DEC-070) dettaglia l'interfaccia; `ui/options-and-accessibility.md` registra la voce di riattivazione; `06-ai-content-generation-model.md` vi rimanda.
- **Documenti aggiornati:** `systems/floor-zero.md`, `ui/options-and-accessibility.md`, `06-ai-content-generation-model.md`

### DEC-087 — Pool curato minimo per categoria (run di fallback completa)

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la ex domanda 10 chiedeva quali contenuti curati minimi debbano esistere per garantire una run completa (Piano 0 + 5 piani) senza alcuna generazione IA disponibile.
- **Decisione:** approvato il principio del **pool curato minimo per categoria**: per ogni categoria di contenuto che il gioco propone esiste un minimo curato che garantisce run completa e fallback in ogni punto di scelta. I valori della tabella sono default proposti stile DEC-019, da confermare: 3 temi/World curati (= carte fallback, DEC-076); 5 boss (uno per piano, legati alle bande di difficoltà); 12 nemici comuni (distribuiti sulle bande dei 5 piani); 20 oggetti (distribuiti sulle 4 rarità con i pesi DEC-019, nessuna rarità vuota); 6 tipi di colpo curati (inclusi i colpi standard della rosa base); la rosa base come pool personaggi (3, DEC-080; in solo-curato non esiste il personaggio generato). I layout stanza sono generati proceduralmente senza IA (DEC-009) e non richiedono un minimo curato.
- **Alternative considerate:** minimo assoluto di sola completabilità (1 tema, 1 boss per piano, un pugno di nemici/oggetti); minimi alti da "gioco vero" che regge più run senza ripetersi.
- **Conseguenze:** `systems/generated-content-validation.md` diventa fonte unica della tabella del pool minimo; `06-ai-content-generation-model.md` e `content/content-taxonomy.md` vi rimandano. La domanda aperta si restringe alla conferma dei numeri.
- **Documenti aggiornati:** `systems/generated-content-validation.md`, `06-ai-content-generation-model.md`, `content/content-taxonomy.md`, `governance/open-questions.md`

### DEC-088 — Il minimo gioco base è la fase 1 più l'economia dei punti

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la ex domanda 11 chiedeva quale sia il minimo gioco base da completare prima di espandere la generazione IA.
- **Decisione:** il minimo gioco base è la **fase 1 già implementata (M1–M8) più l'economia dei punti sblocco** (DEC-027: tasso di guadagno, costi, doppio canale) **e la spesa dei punti nel Catalogo** (DEC-045, terza funzione, oggi mancante). Raggiunto questo, la priorità passa all'espansione della generazione IA.
- **Alternative considerate:** la fase 1 attuale basta già; servono anche il pool curato minimo completo (DEC-087) e una run di fallback senza IA prima di espandere.
- **Conseguenze:** chiude la ex domanda 11 e orienta i piani di implementazione; nessun comportamento visibile al giocatore cambia, quindi nessun documento di sistema da aggiornare oltre alla governance.
- **Documenti aggiornati:** `governance/open-questions.md`, `governance/decision-log.md`

### DEC-089 — L'abbandono passa da RunResults, il reroll no

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** applicando DEC-082 era emersa un'incoerenza (ex domanda 11): `ui/results-and-leaderboards.md` elencava l'abbandono tra gli ingressi di `RunResults`, mentre `ui/pause-menu.md` e `ui/navigation-map.md` documentavano il ritorno diretto a `MainMenu`; non era chiaro dove il giocatore vedesse i punti ridotti.
- **Decisione:** l'**abbandono di una run in corso passa da `RunResults`** come una sconfitta: chiusura della run e punti ridotti visibili lì. Il **reroll salta i risultati** e va dritto alla nuova run: i punti ridotti si accreditano in silenzio e restano consultabili nel Catalogo. L'incoerenza tra i tre documenti si sana in favore di `results-and-leaderboards.md` per l'abbandono.
- **Alternative considerate:** entrambi passano da RunResults; entrambi diretti con accredito silenzioso.
- **Conseguenze:** `ui/pause-menu.md` e `ui/navigation-map.md` correggono la destinazione dell'abbandono confermato (RunResults, non MainMenu diretto); resta aperta solo la collocazione UI esatta del reroll (comando, eventuale conferma).
- **Documenti aggiornati:** `ui/results-and-leaderboards.md`, `ui/pause-menu.md`, `ui/navigation-map.md`, `05-game-states-and-flow.md`, `systems/save-and-meta-progression.md`

### DEC-090 — ExitConfirm da MainMenu è un dialogo modale

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `05-game-states-and-flow.md`: la conferma di chiusura del gioco merita una schermata dedicata o un dialogo?
- **Decisione:** da `MainMenu`, `ExitConfirm` (chiusura del gioco) è un **dialogo modale leggero** sopra il menu, non una schermata dedicata. Negli altri usi (abbandono di run o preparazione) la presentazione resta quella già documentata.
- **Alternative considerate:** schermata dedicata uniforme per ogni uso di ExitConfirm.
- **Conseguenze:** chiude la domanda residua di `05-game-states-and-flow.md`.
- **Documenti aggiornati:** `05-game-states-and-flow.md`

### DEC-091 — Tema e personaggio modificabili fino all'uscita del Piano 0

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/floor-zero.md`: il riepilogo di tema/personaggio è modificabile prima di attraversare l'uscita, o la scelta è definitiva appena confermata?
- **Decisione:** il Piano 0 è il luogo della scelta: **tema e personaggio si possono cambiare finché non si attraversa l'uscita** verso il piano 1 (coerente con la preselezione modificabile di DEC-077). Cambiare il tema **riavvia la generazione** dei piani: l'uscita torna chiusa finché il piano 1 del nuovo tema non è pronto.
- **Alternative considerate:** scelta definitiva alla conferma; personaggio libero ma tema definitivo.
- **Conseguenze:** `systems/floor-zero.md` registra la regola e il costo del cambio tema; gap di implementazione esplicito: M5 tratta la conferma del tema come definitiva.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-092 — Le arene ripristinano lo stato d'ingresso

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `04-run-structure.md`: cosa succede a salute/stato del giocatore se muore in un'arena del Piano 0.
- **Decisione:** l'arena è una **simulazione pura**: uscendone (vittoria o sconfitta) il giocatore ha esattamente la salute e lo stato con cui era entrato. Il rischio zero di DEC-055 vale alla lettera; l'unico effetto persistente resta la dote (DEC-029) in caso di vittoria.
- **Alternative considerate:** la salute persa nell'arena resta persa.
- **Conseguenze:** chiude la domanda residua di `04-run-structure.md`; `systems/floor-zero.md` allinea la descrizione delle arene.
- **Documenti aggiornati:** `04-run-structure.md`, `systems/floor-zero.md`

### DEC-093 — Le arene non hanno un'economia propria

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/floor-zero.md`: oltre alla dote (DEC-029) e alla meta-progressione, le arene premiano l'attività nel Piano 0 stesso?
- **Decisione:** **no**: le arene restano preparazione e allenamento. Le uniche ricompense sono la dote iniziale per la run (DEC-029) e gli eventuali bonus di meta-progressione; il Piano 0 non è un posto dove si "farma".
- **Alternative considerate:** piccole ricompense cosmetiche di attività; economia propria del Piano 0.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/floor-zero.md`.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-094 — Un'arena si apre con un solo best-of, seminata dal curato

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/floor-zero.md`: quanti "best-of" minimi servono perché un'arena compaia come disponibile; il tutorial (DEC-047) è integrato nelle arene, che quindi devono esistere dal primissimo avvio.
- **Decisione:** basta **un** contenuto valido perché un'arena si apra; al primissimo avvio (e ogni volta che i best-of mancano) le arene sono **seminate dal pool curato minimo** (DEC-087), così il tutorial funziona sempre.
- **Alternative considerate:** soglia di 3 best-of; arene sempre aperte senza soglia.
- **Conseguenze:** chiude la domanda residua corrispondente; lega le arene al pool curato di DEC-087.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-095 — Le prove dal museo sono illimitate

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/floor-zero.md` (DEC-040): quanti tentativi per riaffrontare un boss in arena per sessione, e se la saletta di prova oggetto ha limiti.
- **Decisione:** **nessun limite** di tentativi, tempo o usi: le prove dal museo sono simulazioni a rischio zero (DEC-055, DEC-092), il museo è un parco giochi della memoria, non una risorsa da dosare.
- **Alternative considerate:** tetto per sessione; boss illimitati ma saletta a tempo.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/floor-zero.md`.
- **Documenti aggiornati:** `systems/floor-zero.md`

### DEC-096 — Classificata a seed diversi: budget di generazione vincolato

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la parte principale della domanda 9 di `governance/open-questions.md`: come si rende equa una gara Classificata dove ogni partecipante gioca un seed diverso.
- **Decisione:** l'equità è **by-design**: i seed della stessa gara generano dentro gli **stessi vincoli di budget** (difficoltà di nemici e boss, rarità disponibili, numero di stanze in banda stretta). Nessuna correzione statistica a posteriori: il giocatore capisce cosa sta gareggiando. I valori esatti dei vincoli sono da playtest, stile DEC-019.
- **Alternative considerate:** nessuna normalizzazione (gara di adattabilità pura); correzione statistica del punteggio a posteriori.
- **Conseguenze:** `08-multiplayer-and-competition.md` registra il criterio; la domanda 9 si restringe a disconnessioni, metriche extra, regole di parità/validità e valori dei vincoli.
- **Documenti aggiornati:** `08-multiplayer-and-competition.md`, `governance/open-questions.md`

### DEC-097 — Il rifiuto dell'alternativa è ripensabile fino all'uscita

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/characters.md`: il giocatore che rifiuta il personaggio alternativo generato può tornare a selezionarlo nello stesso Piano 0?
- **Decisione:** **sì**: la carta del personaggio generato resta nel selettore e si può scegliere finché non si attraversa l'uscita, coerente con DEC-091. Il "prendere-o-lasciare" si consuma solo all'uscita dal Piano 0.
- **Alternative considerate:** rifiuto definitivo con sparizione della carta.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/characters.md`.
- **Documenti aggiornati:** `systems/characters.md`, `systems/floor-zero.md`

### DEC-098 — Varietà leggera anti-fotocopia del trait

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/characters.md`: il trait unico del personaggio generato può ripetersi tra run diverse?
- **Decisione:** il generatore applica una **varietà leggera anti-fotocopia**: evita di riproporre trait identici a quelli delle run recenti usando il catalogo (stessa filosofia della rete anti-fotocopia dei temi); la ripetizione occasionale a distanza resta ammessa, nessun divieto assoluto.
- **Alternative considerate:** nessun vincolo di varietà; mai ripetere finché esistono alternative.
- **Conseguenze:** chiude la domanda residua corrispondente; il meccanismo riusa il catalogo (DEC-015) come memoria.
- **Documenti aggiornati:** `systems/characters.md`

### DEC-099 — Il colpo firmato è sostituibile come ogni colpo di partenza

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** la lettura M6b-3 (colpo firmato = colpo di partenza, sostituito dagli oggetti-colpo con la regola "vince l'ultimo raccolto", ripristinato se l'oggetto viene concettualmente tolto) era un default proposto mai discusso esplicitamente dal design.
- **Decisione:** la lettura è **sancita**: il colpo firmato è il colpo di partenza e gli oggetti-colpo lo sostituiscono esattamente come sostituirebbero un colpo standard; togliendo l'oggetto torna il colpo firmato (mai il colpo standard, DEC-079). Nessun meccanismo di "colpo protetto".
- **Alternative considerate:** colpo firmato protetto che nessun oggetto può sovrascrivere.
- **Conseguenze:** il blocco "Sostituibilità" di `systems/characters.md` passa da default proposto a canone.
- **Documenti aggiornati:** `systems/characters.md`

### DEC-100 — Sblocchi della rosa base

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** DEC-080 aveva approvato nomi e ruoli della rosa base lasciando aperte le condizioni di sblocco (DEC-030 chiede sblocchi "presto").
- **Decisione:** **Wayfinder** è disponibile da subito (personaggio di partenza); **Ashblade** si sblocca alla **prima run conclusa** (qualunque esito, anche sconfitta o abbandono); **Bulwark** si sblocca al **primo boss abbattuto**. Traguardi naturali che arrivano presto, come vuole DEC-030.
- **Alternative considerate:** tutti sbloccati da subito; sblocco a punti nell'economia DEC-027.
- **Conseguenze:** la domanda 8 di `governance/open-questions.md` si restringe alle sole statistiche (da playtest).
- **Documenti aggiornati:** `systems/characters.md`, `systems/floor-zero.md`, `governance/open-questions.md`

### DEC-101 — La fusione è libera tra categorie

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/item-fusion.md`: la fusione esplicita (DEC-022) è ammessa tra categorie diverse (attivo, passivo, stat-up, Innesto)?
- **Decisione:** **sì, libera tra categorie**: è la meccanica-firma, e fondere un attivo con un passivo è il tipo di sorpresa che la rende memorabile. L'oggetto risultante **dichiara la propria categoria** e resta dentro il budget e le regole di validazione dei contenuti generati (`generated-content-validation.md`).
- **Alternative considerate:** solo stessa categoria; libera tranne gli Innesti.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/item-fusion.md`.
- **Documenti aggiornati:** `systems/item-fusion.md`

### DEC-102 — Un oggetto fuso può essere ri-fuso

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/item-fusion.md`: l'oggetto nato da una fusione può essere sorgente di una fusione successiva?
- **Decisione:** **sì, nessun limite concettuale**: la cadenza attesa di 1-2 fusioni per run (DEC-022, limitata dalla disponibilità di Flux) rende già rarissima la catena; vietarla sarebbe una regola in più senza necessità.
- **Alternative considerate:** oggetto fuso terminale.
- **Conseguenze:** chiude la domanda residua corrispondente; resta aperta (non decisa qui) l'esistenza di un limite rigido al numero totale di fusioni per run.
- **Documenti aggiornati:** `systems/item-fusion.md`

### DEC-103 — I contenuti curati incontrati entrano nel Catalogo

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/generated-content-validation.md` e `systems/save-and-meta-progression.md`: i contenuti curati usati come fallback durante una run entrano nel catalogo persistente?
- **Decisione:** **sì**: il Catalogo è l'enciclopedia completa del crogiolo. I contenuti curati incontrati compaiono marcati con la loro **origine `curato`** (tassonomia già esistente in `content/content-taxonomy.md`), accanto a composto/variato/nuovo. Coerente con la rosa base già visibile tra i Personaggi.
- **Alternative considerate:** catalogo riservato ai soli contenuti generati.
- **Conseguenze:** chiude le domande residue corrispondenti nei due documenti; la vista Catalogo mostra l'origine.
- **Documenti aggiornati:** `systems/save-and-meta-progression.md`, `systems/generated-content-validation.md`

### DEC-104 — Il roster nemici è estendibile nei piani avanzati

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/enemies.md`: il roster di 6-8 tipi per run (DEC-053) è fisso dall'inizio o può crescere?
- **Decisione:** il roster parte fisso (6-8 tipi, DEC-053) ma **nei piani avanzati può essere esteso** da **1-2 tipi "best-of"** provenienti dal catalogo: più sorpresa a run inoltrata, quando il giocatore ha già assimilato le sagome di base. I nuovi ingressi rispettano il budget di leggibilità e le bande di potenza del piano. Valori esatti (da quale piano, quanti tipi) come default proposti da playtest, stile DEC-019: punto di partenza 1 tipo dal piano 3, un secondo dal piano 4.
- **Alternative considerate:** roster fisso per l'intera run (leggibilità pura).
- **Conseguenze:** `systems/enemies.md` registra principio e default; si integra con l'escalation del tema (DEC-024) senza sostituirla.
- **Documenti aggiornati:** `systems/enemies.md`

### DEC-105 — Il tono è ironico-leggero

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** dentro la cornice del crogiolo (DEC-067) il tono specifico restava aperto (`content/narrative-tone.md`).
- **Decisione:** il registro di Worldsmelt è **ironico-leggero**: il crogiolo ha coscienza di sé e un filo di humour asciutto nei testi (card di scoperta, museo, schede del Catalogo), senza rompere l'atmosfera né scadere nella parodia. L'ironia sta nella voce del crogiolo, non nel mondo generato.
- **Alternative considerate:** malinconico-contemplativo; meravigliato-alchemico; epico-solenne.
- **Conseguenze:** `content/narrative-tone.md` registra il registro; restano aperti limiti di contenuto e simboli ricorrenti.
- **Documenti aggiornati:** `content/narrative-tone.md`

### DEC-106 — Il boss del piano 2 è a fase singola

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/bosses.md`: DEC-028 non specifica se il boss del piano 2 abbia una o due fasi.
- **Decisione:** i boss dei piani **1 e 2 sono a fase singola**; le due fasi arrivano dal piano 3 (dove DEC-028 le fissa). Rampa dolce: il salto di complessità coincide con la metà della run.
- **Alternative considerate:** due fasi già dal piano 2.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/bosses.md`.
- **Documenti aggiornati:** `systems/bosses.md`

### DEC-107 — Piega-regole solo alla rarità leggendaria

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/grafts.md`: a quale soglia di rarità un Innesto diventa piega-regole (DEC-034).
- **Decisione:** solo alla rarità **leggendaria** (peso 3 su {55,30,12,3}, DEC-019): piegare le regole del gioco è il massimo effetto possibile e resta un evento raro e memorabile. Gli Innesti di rarità inferiore restano potenti ma dentro le regole.
- **Alternative considerate:** piega-regole anche alla rarità rara, con effetti minori.
- **Conseguenze:** chiude la domanda residua corrispondente di `systems/grafts.md`.
- **Documenti aggiornati:** `systems/grafts.md`

### DEC-108 — Nelle gare le proposte sono identiche, la scelta è libera

- **Data:** 2026-07-19
- **Stato:** approved
- **Contesto:** domanda residua di `systems/characters.md`: come funziona la scelta del personaggio nelle modalità competitive asincrone (Classificata stesso seed, Daily).
- **Decisione:** lo stesso seed genera **le stesse proposte per tutti i partecipanti** (stesso personaggio alternativo generato, stesso tema proposto): la **scelta resta libera e strategica**, l'equità è garantita dal determinismo della generazione dal seed. Nessun personaggio imposto dalla gara, nessuna esclusione del personaggio generato. Perimetro (chiarito il 19/07 dopo verifica incrociata con DEC-100): la **rosa base disponibile resta quella sbloccata dal singolo giocatore** (DEC-100), anche in gara — la parità riguarda il contenuto generato dal seed, non la progressione personale; gli sblocchi arrivano presto per costruzione, quindi lo svantaggio di un giocatore nuovo è effimero e non merita eccezioni alle regole di sblocco. Gap di implementazione esplicito: il determinismo completo delle proposte dal seed non è ancora garantito dalla pipeline (backlog noto: RNG di gioco su `time(NULL)`, inferenza non deterministica).
- **Alternative considerate:** personaggio fissato dalla gara; solo rosa base in Classificata.
- **Conseguenze:** `systems/characters.md` e `08-multiplayer-and-competition.md` registrano la regola; si appoggia a `systems/run-manifest-and-reproducibility.md` per la riproducibilità.
- **Documenti aggiornati:** `systems/characters.md`, `08-multiplayer-and-competition.md`


---

### DEC-109 — L'audio diventa generativo: Stable Audio Small con catena di fallback

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** DEC-036 (18/07) fissava musica e suoni curati/statici con la generazione audio come idea futura. La blueprint ai-production proponeva una pipeline generativa, rimasta bloccata dall'audit documentale come open question 12. L'utente ha sciolto la riserva nella sessione decisionale del 22/07.
- **Decisione:** la via primaria per musica e SFX è **Stable Audio Small in locale**, con catena di fallback obbligatoria: **rFXGen** (SFX procedurali, ecosistema raylib) → **audio curato/statico**. La garanzia di DEC-036 sopravvive come rete: ogni evento critico ha sempre un suono curato o di fallback, e la modalità solo-curato resta completa e dignitosa. Vincoli architetturali invariati: nessuna generazione durante il combattimento; il modello audio si carica **in sequenza** con Qwen e SD (mai insieme nei 6 GB di riferimento); cache e pubblicazione atomica; nessun peso ridistribuito col gioco. **Sostituisce la parte «generazione = futuro» di DEC-036.**
- **Alternative considerate:** confermare DEC-036 (solo curato); solo rFXGen senza modelli generativi.
- **Conseguenze:** `content/audio-and-feedback.md` aggiornato; `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` sbloccata e promossa; licenza del modello → DEC-113; open question 12 chiusa.
- **Documenti aggiornati:** `content/audio-and-feedback.md`, `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`, `docs/ai-production/licenze.md`, `docs/ai-production/regole-agenti-ml.md`
- **Nota (2026-07-28):** superata parzialmente da DEC-178 — rFXGen esce dalla catena di fallback: rFXGen non è mai stato installato né usato, il generatore degli SFX è il checkpoint sfx di Stable Audio Small (semplici e complessi), e il fallback garantito è direttamente il pacchetto curato/statico (catena a due livelli, non più tre). Restano invariate la scelta di Stable Audio Small come via primaria e la garanzia del fallback curato sempre disponibile.

---

### DEC-110 — Niente preset lowspec: i requisiti minimi sono i modelli di riferimento

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** DEC-070 dichiara «un solo set di modelli, nessun tier intermedio», ma il codice applicava in automatico un preset `--low-spec` (testo 1.5B, sprite 256px) deciso dal benchmark della macchina (open question 13 dell'audit).
- **Decisione:** il preset di qualità viene **rimosso dal codice**. I requisiti minimi del gioco completo sono quelli necessari a far girare i modelli di riferimento (Qwen 7B + SD1.5); hardware più potente significa solo attese più brevi, mai qualità diversa. `make benchmark` resta come strumento diagnostico manuale, ma il gioco non ne legge più l'esito. Restano intatti i fallback di robustezza (1.5B quando il 7B non si carica, generatore deterministico, solo-curato): sono reti su errore, non tier di qualità.
- **Alternative considerate:** chiarire DEC-070 ammettendo il preset come dettaglio interno; renderlo visibile al giocatore.
- **Conseguenze:** rimozione implementata via scala (gradino 2); il piano `benchmark-primo-avvio` è annullato; open question 13 chiusa.
- **Documenti aggiornati:** `docs/engineering/benchmarks.md`, `docs/plans/cancelled/benchmark-primo-avvio.md`
- **Nota (2026-07-25):** superata parzialmente da DEC-142 — dopo DEC-140 (Gemma-3-4B-IT Q4 sostituisce Qwen 7B come modello testuale di riferimento) l'ancoraggio del requisito minimo ai **nomi dei modelli di riferimento** non è più la formulazione canonica: il requisito minimo si esprime ora in **numeri misurati** (VRAM/RAM/sistema operativo) sulla macchina di riferimento. Il principio di questa decisione (un solo set di riferimento, nessun tier di qualità) resta valido; cambia solo come il requisito viene espresso.

---

### DEC-111 — Confermata la scelta binaria: nessun fallback granulare per hardware debole

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** gli appunti storici proponevano tier hardware S/A/B/C (fino a «logic-only con sprite da libreria» e pool di bundle pre-generati); l'audit li aveva segnalati come possibile estensione di DEC-070 (open question 14).
- **Decisione:** DEC-070 e DEC-086 sono confermate nella lettura stretta: al primo avvio la scelta è **binaria** — esperienza completa o solo-curato — senza livelli intermedi né modalità degradate. La tabella dei tier resta solo memoria storica in `docs/archive/legacy-notes/roguelike-ai-appunti/`.
- **Alternative considerate:** parcheggiare i tier in DEC-018; esplorarli subito per Steam Deck.
- **Conseguenze:** open question 14 chiusa.
- **Documenti aggiornati:** `governance/open-questions.md`

---

### DEC-112 — Il director-per-stile è parcheggiato fra le idee future

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** gli appunti 04 proponevano un adattamento dei contenuti allo *stile* di gioco osservato (mai alla difficoltà: DEC-038 resta intatta in ogni caso); la KB non l'aveva mai recepito (open question 15).
- **Decisione:** l'idea entra nell'elenco delle **idee parcheggiate di DEC-018** (come caos, obiettivi a tema, integrità melting, lobby custom): nessun lavoro previsto, nessuna promessa, ripresa possibile con una decisione futura.
- **Alternative considerate:** scarto definitivo; design probe immediata.
- **Conseguenze:** open question 15 chiusa; DEC-038 (difficoltà unica) non toccata.
- **Documenti aggiornati:** `governance/open-questions.md`

---

### DEC-113 — Accettata la Stability AI Community License per Stable Audio Small

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** l'adozione di Stable Audio Small (DEC-109) rende operativa la domanda di licenza che il research pack dava per scontata senza alcuna decisione (open question 16): i modelli Stability recenti usano la Community License, gratuita per uso commerciale fino a 1M$ di ricavi annui, oltre i quali serve la licenza Enterprise.
- **Decisione:** si **accettano i termini della Stability AI Community License** per Stable Audio Small. La soglia Enterprise si rivaluta solo se i ricavi si avvicinassero a 1M$/anno. Invarianti confermate: i pesi non vengono mai ridistribuiti col gioco (li scarica l'utente); la verifica puntuale della licenza si fa alla revisione corrente dell'upstream al momento dell'integrazione.
- **Alternative considerate:** rifiutare Stability (avrebbe annullato DEC-109); rimandare al momento dell'integrazione.
- **Conseguenze:** open question 16 chiusa; registro licenze aggiornato.
- **Documenti aggiornati:** `docs/ai-production/licenze.md`

---

### DEC-114 — Il reroll vive nel menu di pausa, con conferma

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** DEC-089 fissava il flusso del reroll da run in corso (salta i risultati, accredita i punti in silenzio) ma non la collocazione UI (open question 11).
- **Decisione:** il reroll si attiva **solo dal PauseMenu**, come voce dedicata con **dialogo di conferma modale** (coerente con DEC-090: ogni azione distruttiva passa da una conferma). Nessun tasto rapido diretto: buttare una run per un tasto sbagliato è il caso peggiore. Gap di implementazione esplicito: oggi `R` rigenera direttamente, da adeguare.
- **Alternative considerate:** tasto rapido `R` con conferma; `R` diretto senza conferma; rinvio al playtest.
- **Conseguenze:** open question 11 chiusa; `ui/pause-menu.md` registra la voce.
- **Documenti aggiornati:** `ui/pause-menu.md`, `governance/open-questions.md`

---

### DEC-115 — Gli Innesti si possono sganciare volontariamente

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda aperta residua di `systems/grafts.md`: il drop volontario di un Innesto equipaggiato è permesso?
- **Decisione:** sì: un Innesto equipaggiato può essere **sganciato volontariamente**, liberando lo slot. È coerente con la natura situazionale degli Innesti (si agganciano e sganciano secondo il contesto). Il destino dell'Innesto sganciato (resta a terra recuperabile nella stanza o si perde) è un dettaglio da definire con l'implementazione.
- **Alternative considerate:** Innesto fisso fino a sostituzione; rinvio al playtest.
- **Conseguenze:** `systems/grafts.md` aggiornato; la domanda esce dalle residue.
- **Documenti aggiornati:** `systems/grafts.md`

---

### DEC-116 — Gli Innesti valgono per tutta la run

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda aperta residua di `systems/grafts.md`: gli Innesti persistono tra i piani o sono legati a condizioni locali?
- **Decisione:** gli Innesti raccolti **persistono per tutta la run**, come ogni altro oggetto della build (stessa regola già scritta per gli attivi): semplice, prevedibile, coerente con l'inventario.
- **Alternative considerate:** Innesti legati al piano (risorse tattiche locali).
- **Conseguenze:** `systems/grafts.md` aggiornato.
- **Documenti aggiornati:** `systems/grafts.md`

---

### DEC-117 — Gli attivi si scambiano sui piedistalli

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda aperta residua di `systems/active-items.md` sul riordino/scambio volontario degli attivi. L'utente ha risposto con un design più preciso.
- **Decisione:** gli oggetti attivi usano un **sistema a piedistalli**: un attivo trovato (es. nella stanza del tesoro) **fluttua su un piedistallo**; raccogliendolo, il nuovo attivo entra nell'inventario (visibile nella sezione UI dedicata agli attivi) e **l'attivo che possedevi finisce sul piedistallo al suo posto**. Lo scambio è quindi sempre reversibile finché resti nella stanza: il vecchio attivo non sparisce, fluttua lì. Con uno slot libero la raccolta riempie lo slot senza scambio. Perimetro (chiarito nello stesso giro): con più slot pieni va sul piedistallo **l'attivo attualmente selezionato per l'attivazione** — quello «in mano».
- **Alternative considerate:** riordino libero fra slot fuori combattimento; ordine fisso di raccolta.
- **Conseguenze:** `systems/active-items.md` registra il sistema; gap di implementazione esplicito (oggi la raccolta non ha piedistalli di scambio).
- **Documenti aggiornati:** `systems/active-items.md`

---

### DEC-118 — L'audio della fusione ha priorità massima dedicata

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda aperta residua di `content/audio-and-feedback.md`: la fusione (meccanica-firma, DEC-023) merita una priorità sonora superiore agli altri eventi prioritari?
- **Decisione:** sì: l'evento sonoro della fusione ha **priorità massima dedicata** — il segnale più riconoscibile del gioco, che interrompe/attenua gli altri suoni mentre suona. Il momento-firma deve sentirsi tale.
- **Alternative considerate:** stessa famiglia degli altri eventi prioritari; rinvio al playtest.
- **Conseguenze:** `content/audio-and-feedback.md` aggiornato.
- **Documenti aggiornati:** `content/audio-and-feedback.md`

---

### DEC-119 — Limiti di contenuto: fantasy dark stilizzato (~16+)

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** `content/narrative-tone.md` lasciava i limiti di contenuto per i contenuti generati fra le cose «da definire». Il tono della voce dell'interfaccia è ironico-leggero (DEC-105); mancavano i confini su violenza e temi sensibili per ciò che il modello può inventare.
- **Decisione:** i contenuti generati (mondi, nemici, nomi, descrizioni) possono essere **dark (~16+)**: gore stilizzato da pixel-art e temi cupi/disturbanti sono ammessi. Restano **fuori**: contenuti sessuali espliciti e riferimenti al mondo reale (religioni, politica, etnie, persone). Non contraddice DEC-105: il registro ironico-leggero riguarda la *voce del crogiolo* (interfaccia), i World generati possono essere cupi — l'ironia del crogiolo può anzi fare da contrappunto. I limiti vanno tradotti in vincoli nei prompt di generazione e nel validatore.
- **Alternative considerate:** fantasy stilizzato ~PEGI 12; family ~PEGI 7; lista scritta da rivedere.
- **Conseguenze:** `content/narrative-tone.md` definisce i limiti; i prompt di melting-gen andranno allineati (gap di implementazione).
- **Documenti aggiornati:** `content/narrative-tone.md`

---

### DEC-120 — Il set di simboli ricorrenti del crogiolo

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** `content/narrative-tone.md` lasciava i simboli ricorrenti «da definire»; la cornice è il Crogiolo dei mondi (DEC-067) con nomenclatura inglese (DEC-072) mai imposta ai World generati (DEC-073).
- **Decisione:** esiste un **piccolo set fisso di simboli ricorrenti della forgia** che lega le run: **fuoco/colata, metallo e crogiolo, lingotti (Ingots), scintille (Embers)**. Vive solo nella voce dell'interfaccia, nel Piano 0 e nei testi del crogiolo; i World generati restano liberi (coerente con DEC-067/072/073).
- **Alternative considerate:** nessun simbolismo oltre la nomenclatura; proposta scritta successiva.
- **Conseguenze:** `content/narrative-tone.md` registra il set; il visual language del Piano 0 potrà attingervi.
- **Documenti aggiornati:** `content/narrative-tone.md`

---

### DEC-121 — Una famiglia sonora per il Piano 0, con due voci

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** ultima domanda residua di `content/audio-and-feedback.md`: «scelta del tema» e «generazione completata» nel Piano 0 devono avere segnali distinti o condividere una famiglia?
- **Decisione:** gli eventi del Piano 0 condividono **una stessa famiglia sonora «del crogiolo»**, con **due segnali riconoscibili** al suo interno: coerenza d'insieme e distinguibilità dei momenti. Si integra naturalmente col set di simboli di DEC-120.
- **Alternative considerate:** due segnali indipendenti; rinvio al playtest.
- **Conseguenze:** `content/audio-and-feedback.md` chiude le sue domande residue.
- **Documenti aggiornati:** `content/audio-and-feedback.md`

---

### DEC-122 — Gli Innesti si combinano fra loro

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/grafts.md`: due Innesti equipaggiati (con più slot) possono avere stacking?
- **Decisione:** sì: gli effetti di più Innesti equipaggiati **coesistono e si combinano** naturalmente, dentro i soliti clamp e budget del motore (mai oltre le bande). Più profondità di build, coerente con la filosofia delle sinergie.
- **Alternative considerate:** Innesti isolati; rinvio al playtest.
- **Conseguenze:** `systems/grafts.md` aggiornato; il bilanciamento fine resta materia di playtest.
- **Documenti aggiornati:** `systems/grafts.md`

---

### DEC-123 — Gli slot Innesto aggiuntivi valgono solo per la run

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/grafts.md`: gli slot extra da eventi rari entrano nella meta-progressione?
- **Decisione:** no: gli slot Innesto aggiuntivi sono **fortuna della run**, come il resto della build. La meta-progressione resta **sblocco di contenuti, mai di potenza** (DEC-015/DEC-027): anche l'equilibrio delle gare a parità di seed ne dipende.
- **Alternative considerate:** slot permanente sbloccabile con gli Embers.
- **Conseguenze:** `systems/grafts.md` aggiornato; nessun impatto su `save-and-meta-progression.md` (che resta sblocco-contenuti).
- **Documenti aggiornati:** `systems/grafts.md`

---

### DEC-124 — I contenuti «variati» si rivalidano sempre da zero

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `content/content-taxonomy.md`: un contenuto `variato` derivato da un `nuovo` già validato eredita le prove superate dal genitore?
- **Decisione:** no: **ogni contenuto passa sempre l'intera validazione**, anche se deriva da un genitore valido. Nessuna scorciatoia: «i contenuti generati non sono automaticamente validi» vale anche per le variazioni (una variazione può rompere esattamente ciò che il genitore garantiva).
- **Alternative considerate:** eredità dei vincoli con rivalidazione del solo delta; decisione rinviata a una misura dei costi.
- **Conseguenze:** `content/content-taxonomy.md` aggiornato; coerente con `systems/generated-content-validation.md`.
- **Documenti aggiornati:** `content/content-taxonomy.md`

---

### DEC-125 — Nessun limite rigido al numero di fusioni per run

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/item-fusion.md`: oltre alla cadenza attesa di 1-2 fusioni (DEC-022), serve un tetto rigido?
- **Decisione:** no: il limite è già **nell'economia** (catalizzatore raro, stanza dedicata), non serve un tetto artificiale. Se una run fortunata concede una terza fusione, è festa, non un bug. La cadenza attesa 1-2 resta il riferimento di bilanciamento delle fonti del catalizzatore.
- **Alternative considerate:** tetto rigido a 2; rinvio al playtest.
- **Conseguenze:** `systems/item-fusion.md` aggiornato; il cap del catalizzatore resta domanda numerica da playtest.
- **Documenti aggiornati:** `systems/item-fusion.md`

---

### DEC-126 — I bersagli dei piega-regole: lista chiusa curata

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/grafts.md`: quali regole del gioco, oltre a offerte del negozio e stanze segrete, sono bersagli ammissibili per un Innesto piega-regole (DEC-034, solo leggendari per DEC-107)?
- **Decisione:** i bersagli ammissibili sono una **lista chiusa curata** di quattro domini: **economia** (offerte, prezzi, valute), **stanze** (segrete, tesoro, ricompense), **drop e rarità** (pesi dei pool), **risorse** (cap e ricariche). **Esclusioni dure**: mai la difficoltà (DEC-038), mai le regole competitive/classifiche, mai la validazione e i fallback. Il modello inventa piega-regole dentro questa lista; il validatore la fa rispettare.
- **Alternative considerate:** solo economia e stanze; lista aperta con sole esclusioni; rinvio al playtest.
- **Conseguenze:** `systems/grafts.md` registra la lista; i prompt e il validatore dei contenuti generati andranno allineati (gap di implementazione).
- **Documenti aggiornati:** `systems/grafts.md`

---

### DEC-127 — I rivelatori delle super-segrete: Innesti sensore + oggetti rari

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/secrets-and-obstacles.md`: come esistono i rivelatori delle stanze super-segrete (DEC-025)?
- **Decisione:** la rivelazione passa da una **categoria di Innesti «sensore» dedicati** più **qualche oggetto raro** che la offre come effetto secondario. Rari nei pool, ma con **almeno un'occasione realistica per run**. I tassi esatti restano materia di playtest (open question numerica).
- **Alternative considerate:** solo Innesti dedicati; solo indizi ambientali senza oggetti.
- **Conseguenze:** `systems/secrets-and-obstacles.md` aggiornato; i pool dei contenuti dovranno prevedere la categoria.
- **Documenti aggiornati:** `systems/secrets-and-obstacles.md`

---

### DEC-128 — Lo strumento di breccia sono le bombe, universali

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/secrets-and-obstacles.md`: lo strumento di breccia ha un solo tipo di bersaglio o più categorie di ostacolo con costi diversi?
- **Decisione:** lo strumento di breccia **è la bomba** (Blast Charges, DEC-013/DEC-072): nessun oggetto separato, nessuna categoria a costo diverso. **Universale**: un uso = un'esplosione, e la variabilità sta **nel bersaglio** — che cosa è colpito dall'esplosione e che cosa no (dichiarato e leggibile prima dell'uso, mai una lotteria). Un ostacolo o è breccia-bile o non lo è.
- **Alternative considerate:** categorie di ostacolo con costi diversi.
- **Conseguenze:** `systems/secrets-and-obstacles.md` e `systems/health-and-resources.md` allineati.
- **Documenti aggiornati:** `systems/secrets-and-obstacles.md`, `systems/health-and-resources.md`

---

### DEC-129 — Il Flux non ha cap

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/item-fusion.md` e `systems/health-and-resources.md`: il catalizzatore di fusione (Flux) ha un cap massimo trasportabile?
- **Decisione:** **nessun cap**: l'accumulo è libero, il limite è già nella rarità delle fonti (DEC-022). Stessa filosofia niente-tetti-artificiali di DEC-125: se una run generosa regala Flux, la festa è legittima.
- **Alternative considerate:** cap 1 con conversione in valuta; cap piccolo 2-3; rinvio al playtest.
- **Conseguenze:** `systems/health-and-resources.md` registra il cap; resta aperto solo il comportamento a fine piano/fine run (già domanda del documento).
- **Documenti aggiornati:** `systems/item-fusion.md`, `systems/health-and-resources.md`

---

### DEC-130 — Invulnerabilità breve dopo il danno, con lampeggio

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/player.md`: esistono gli i-frames?
- **Decisione:** sì: dopo aver subito danno il personaggio ha una **breve invulnerabilità** con **feedback di lampeggio**. Evita le morti-frullatore contro i gruppi, standard leggibile del genere. La **durata esatta** è materia di playtest.
- **Alternative considerate:** nessun i-frame (hardcore); rinvio totale al playtest.
- **Conseguenze:** `systems/player.md` registra il principio; resta aperta solo la durata.
- **Documenti aggiornati:** `systems/player.md`

---

### DEC-131 — La coda delle card di scoperta ha un cap piccolo

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** caso limite di DEC-065 (`ui/hud.md`): con molte scoperte ravvicinate la coda delle card può crescere.
- **Decisione:** la coda è **limitata** (ordine di ~5 card, valore esatto da playtest): quando trabocca, le card **più vecchie escono senza essere mostrate**. Nessuna perdita reale: ogni scoperta resta comunque registrata nel Catalogo con la sua scheda. L'HUD resta pulito.
- **Alternative considerate:** coda illimitata con scorrimento accelerato.
- **Conseguenze:** `ui/hud.md` aggiornato.
- **Documenti aggiornati:** `ui/hud.md`

---

### DEC-132 — I fallback si raccontano a fine run, con discrezione

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/generated-content-validation.md`: comunicare al giocatore, in forma aggregata e non tecnica, quante volte è scattato un fallback?
- **Decisione:** sì, con **una riga discreta in `RunResults`**, nel registro ironico-leggero del crogiolo (DEC-105) — ad es. «il crogiolo ha attinto due volte alla riserva». Trasparenza senza tecnicismi; durante la run il fallback resta invisibile. Si integra con RunResults che già conta le creazioni entrate nel Catalogo e con l'origine registrata per scheda (DEC-103).
- **Alternative considerate:** origine visibile solo nel Catalogo; silenzio totale.
- **Conseguenze:** `systems/generated-content-validation.md` e `ui/results-and-leaderboards.md` registrano la riga.
- **Documenti aggiornati:** `systems/generated-content-validation.md`, `ui/results-and-leaderboards.md`

---

### DEC-133 — Le bande di potenza restano invariate per tutta la run

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/combat-and-projectiles.md`: le bande [0.75–1.25] valgono uguali ovunque o si allargano coi piani?
- **Decisione:** le bande sono **invariate per tutta la run**: la difficoltà cresce col **budget di stanza** e con l'escalation leggibile del tema (DEC-024/DEC-043), mai gonfiando i singoli contenuti. Ogni tipo di colpo/nemico resta un sidegrade leggibile in qualunque piano: è la garanzia già implementata da `ShotTypeBalance`/`EnemyTypeBalance` e questa decisione la rende canonica.
- **Alternative considerate:** bande che si allargano/spostano nei piani avanzati.
- **Conseguenze:** `systems/combat-and-projectiles.md` aggiornato; nessun cambio al codice (già conforme).
- **Documenti aggiornati:** `systems/combat-and-projectiles.md`

---

### DEC-134 — Il danno da contatto respinge: knockback breve

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `systems/player.md` sull'interazione fisica con ostacoli e danno da contatto.
- **Decisione:** il danno da contatto **respinge brevemente** il personaggio (knockback): più fisicità e leggibilità del colpo subito. Lavora insieme all'invulnerabilità breve (DEC-130): la respinta avviene dentro la finestra di i-frames, quindi niente rimbalzi-frullatore. Distanza e durata della respinta sono da playtest; gli ostacoli solidi restano bloccanti nel movimento normale (lo scivolamento lungo le pareti in diagonale resta il comportamento atteso, dettaglio implementativo).
- **Alternative considerate:** blocco semplice senza knockback; rinvio al playtest.
- **Conseguenze:** `systems/player.md` aggiornato; il tuning fine è materia di playtest.
- **Documenti aggiornati:** `systems/player.md`

---

### DEC-135 — La validazione strutturale dei layout è una categoria dedicata

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** domanda residua di `06-ai-content-generation-model.md`: i layout di stanza generati richiedono una categoria di validazione strutturale oltre alla croce centrale libera già garantita dal motore?
- **Decisione:** sì: il contratto dei contenuti generati include una **categoria di validazione strutturale dei layout**: croce centrale libera (già implementata), **raggiungibilità garantita** di porte e ricompense, **spazio minimo di manovra**. Un layout che non la supera segue la normale catena di fallback. Gap di implementazione: la croce libera esiste già nel motore; la verifica formale di raggiungibilità e spazio va aggiunta al validatore.
- **Alternative considerate:** ritenere sufficiente la croce libera; rinvio al playtest.
- **Conseguenze:** `06-ai-content-generation-model.md` e `systems/generated-content-validation.md` registrano la categoria.
- **Documenti aggiornati:** `06-ai-content-generation-model.md`, `systems/generated-content-validation.md`

---

### DEC-136 — Lo scambio ad alto rischio si chiama Pourhouse

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** `systems/special-rooms.md` lasciava aperti nome e presentazione dello scambio ad alto rischio. Rosa di cinque proposte del content designer (con verifica di originalità: un candidato scartato perché collideva con un'area di un gioco esistente), scelta dell'utente.
- **Decisione:** il nome in-game è **Pourhouse** (resa italiana per il glossario bilingue: **Casa della Colata**). Doppio registro nella stessa parola: il luogo dove si versa il metallo fuso (simboli del crogiolo, DEC-120) e quasi-omofono di *poorhouse* — chi entra rischia di uscirne più povero, che è la meccanica (DEC-044). Presentazione canonica nella voce del crogiolo (DEC-105): «Non è un negozio, è una colata: quello che porti dentro si scioglie, quello che esce non lo scegli tu.» Il termine funzionale «scambio ad alto rischio» resta invariato nei documenti di design.
- **Alternative considerate:** Cindermonger, Quenchbroker, Emberlien, Cinderpit (sconsigliata per prossimità a un titolo esistente).
- **Conseguenze:** glossario DEC-072 esteso; la domanda residua di `special-rooms.md` si chiude.
- **Documenti aggiornati:** `governance/glossary.md`, `systems/special-rooms.md`

---

### DEC-137 — Una sola schermata: la game view a tutto schermo con GUI in overlay

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** il layout attuale affianca il canvas di gioco a pannelli laterali dedicati (due zone distinte dello schermo). L'utente vuole un'unica superficie.
- **Decisione:** esiste **una sola schermata**: la **game view occupa tutto lo schermo** e la GUI vive **in overlay sopra di essa** — HUD sovrapposto ai bordi, pannelli (build, log, minimappa) come overlay adattivi/a comparsa, mai come colonne che sottraggono spazio al mondo. Restano validi: pixel-art per tutta la UI (DEC-046), uiScale adattivo (M4), leggibilità degli overlay sopra il gioco (contrasto/pannelli semitrasparenti da definire con l'implementazione), stati canonici invariati.
- **Alternative considerate:** mantenere il layout a pannelli laterali.
- **Conseguenze:** refactor del renderer (gradino 3 della scala); `ui/hud.md` e gli screenshot di riferimento andranno aggiornati con l'implementazione.
- **Documenti aggiornati:** `ui/hud.md` (nota), implementazione in corso

---

### DEC-138 — Le classi fisiche dei colpi le comporrà il modello: prima il mechanics-lab

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** l'indagine tecnica del 22/07 (`docs/engineering/espressivita-colpi.md`) ha accertato che il vocabolario attuale (5 forme × 7 manopole + trait + Lua) permette solo variazioni sullo stesso scheletro fisico — proiettile puntiforme, moto rettilineo, vita breve. Laser veri, colpi stazionari e orbite non sono esprimibili; la catena fra nemici sì (già nativa). L'utente vuole di più: **il gioco deve poter generare classi fisiche nuove**, non scegliere da un menu.
- **Decisione:** l'espansione dell'espressività NON passa da un elenco fisso di classi cablate (un enum «laser/torretta/orbita» sarebbe di nuovo un menu): passa da **primitive fisiche minime e componibili** che il modello combina per inventare classi che non abbiamo previsto. Per scoprire QUALI primitive servono si costruisce prima il **mechanics-lab** (piano attivo dedicato): build isolata con la stessa sandbox Lua del gioco, arena minima, primitive candidate (query a segmento, ancoraggio/velocità dei colpi, vita parametrica, forze/steering), prompt «impossibili» per l'API attuale, report comparativo. **Criterio di successo:** generare un laser, una catena e un'orbita **senza** primitive ad alto livello dedicate a «laser», «catena» o «orbita». Le primitive vincenti entrano poi nel motore con le garanzie di sempre (clamp, budget, ShotTypeBalance, fallback) e il vocabolario GBNF/Lua si estende. Restano fermi: il floor anti-«colpo che striscia» per i colpi in volo (una classe stazionaria, se nascerà, sarà una classe distinta, non un colpo lento); nessuna inferenza in combattimento; la mini-VM/parametrico come rete di sicurezza.
- **Alternative considerate:** aggiungere subito 3 classi fisse nel motore (più rapido, ma resta un menu); esporre al Lua la manipolazione libera dei colpi senza esperimento (garanzie fragili).
- **Conseguenze:** piano `docs/plans/active/mechanics-lab.md`; le domande 4-5 del research pack (primitive minime, composizione senza loop) trovano qui il loro veicolo; implementazione dopo il refactor GUI in corso.
- **Documenti aggiornati:** `docs/engineering/espressivita-colpi.md` (nuovo), `docs/plans/active/mechanics-lab.md` (nuovo)

---

### DEC-139 — TAB apre la build anche in Gameplay

- **Data:** 2026-07-22
- **Stato:** approved
- **Contesto:** con l'overlay di DEC-137 il pannello build completo vive in `BuildScreen`, raggiungibile da Pausa → «Build e sinergie». L'utente vuole la consultazione immediata.
- **Decisione:** **TAB in `Gameplay` apre/chiude `BuildScreen`** direttamente (la simulazione si ferma come già accade in `BuildScreen`): coerente col TAB del Piano 0 che apre le carte. La via da Pausa resta.
- **Alternative considerate:** solo da Pausa.
- **Conseguenze:** verificato in scala (gradino 1) che il binding **esiste già dal M1a** (`f019cd2`, con test in GameStatesTest: Gameplay→TAB→BuildScreen→ritorno): la decisione rende canonico il comportamento implementato, nessun cambio di codice necessario. `ui/inventory-and-synergy-screen.md` registra l'ingresso rapido; la mappa canonica dei 9 stati non cambia.
- **Documenti aggiornati:** `ui/inventory-and-synergy-screen.md`

---

### DEC-140 — Il modello testuale di riferimento diventa Gemma-3-4B-IT (Q4_K_M)

- **Data:** 2026-07-23
- **Stato:** approved
- **Contesto:** la suite di comparison del 23/07 (11 modelli × 3 seed fissi, report congelato in `docs/ai-production/experiments/model-comparison-testo-2026-07-23.md`) ha misurato gemma-3-4b-it Q4_K_M sopra il baseline Qwen2.5-Coder-7B Q4_K_M: punteggio 84.9 vs 76.9, Lua valido al primo colpo 93% vs 73% (100% dopo retry), JSON 100%, metà del peso (2.3 vs 4.4 GiB), +21% tok/s. Decisione dell'utente.
- **Decisione:** `gemma-3-4b-it-q4_k_m.gguf` diventa il **modello testuale di riferimento** di melting-gen (default). Il fallback su errore di caricamento resta il Coder 1.5B Q4 (più piccolo accettabile: sopra tutte le soglie). Il 7B resta scaricabile e selezionabile con `--model`, quindi la scelta è **reversibile con un flag**. Licenza: Gemma è distribuito sotto i **Gemma Terms of Use** di Google (uso commerciale consentito con condizioni; i pesi non vengono comunque mai ridistribuiti col gioco) — registrata in `licenze.md`. Nota di rigore: il campione era di 3 seed; una **validazione estesa** (10+ seed via `make gen-metrics`) resta raccomandata come follow-up.
- **Alternative considerate:** restare sul 7B salendo a Q5_K_M (+15 punti di Lua sul Q4); Coder 1.5B per il minimo assoluto.
- **Conseguenze:** default aggiornato in tools/melting-gen e in `scripts/download-models.sh` (scala, gradino 2, con `make test-llm` sul nuovo default); `00-DECISIONI-CANONICHE.md` e `licenze.md` aggiornati.
- **Documenti aggiornati:** `docs/ai-production/00-DECISIONI-CANONICHE.md`, `docs/ai-production/licenze.md`, `docs/ai-production/experiments/model-comparison-testo-2026-07-23.md`

---

### DEC-141 — Il RNG di gameplay deterministico è prerequisito bloccante della Classificata a stesso seed

- **Data:** 2026-07-25
- **Stato:** approved
- **Contesto:** l'audit documentale del 25/07 ha verificato nel codice (`src/game/game.c:99-103`, `GameResetRun`) che l'RNG di gameplay (spawn, drop, combattimento) è seedato con `time(NULL)`, non con un seed derivato dal seed di run — difetto già registrato in `docs/engineering/known-issues.md` voce 3. `systems/run-manifest-and-reproducibility.md` (riga ~70) affermava però il determinismo del gameplay come già garantito: un drift doc↔codice fra la premessa di DEC-016 (gare asincrone sullo stesso seed/manifest, "il determinismo esiste già") e lo stato reale. Domanda: la premessa regge ancora per abilitare gare Classificate a stesso seed?
- **Decisione:** **fix prima della Classificata.** Nessuna gara a stesso seed viene abilitata finché l'RNG di gameplay non deriva dal seed di run: il fix è **prerequisito bloccante** della modalità Classificata a stesso seed (DEC-021). Il determinismo oggi garantito riguarda solo il contenuto generato (tema, layout, oggetti dal seed di `melting-gen`), non spawn/drop/combattimento durante la run: due giocatori con lo stesso seed possono affrontare gli stessi piani ma vivere un combattimento diverso. Fino al fix, la Classificata a stesso seed resta non offerta o segnalata come non garantita.
- **Alternative considerate:** abilitare comunque la Classificata a stesso seed accettando la divergenza di gameplay come rumore tollerato; derivare l'RNG di gameplay dal seed solo come palliativo lato client senza correggere `GameResetRun`.
- **Conseguenze:** `systems/run-manifest-and-reproducibility.md` corregge l'affermazione di determinismo del gameplay già garantito, dichiarando lo stato reale e il prerequisito; `docs/engineering/multiplayer-steam.md` registra che le classifiche a stesso seed dipendono dal fix; `docs/engineering/known-issues.md` voce 3 rimanda a questa decisione. Il fix stesso (derivare `game->rng` dal seed di run in `GameResetRun`) resta backlog aperto: nessun cambio di codice in questo lavoro.
- **Documenti aggiornati:** `docs/design/systems/run-manifest-and-reproducibility.md`, `docs/engineering/multiplayer-steam.md`, `docs/engineering/known-issues.md`

---

### DEC-142 — I requisiti hardware minimi si esprimono in numeri misurati, non in nomi di modello

- **Data:** 2026-07-25
- **Stato:** approved
- **Contesto:** DEC-110 ancora i requisiti minimi del gioco completo ai "modelli di riferimento (Qwen 7B + SD1.5)"; DEC-140 (23/07) ha però sostituito Qwen 7B con Gemma-3-4B-IT Q4_K_M come modello testuale di riferimento, lasciando ambiguo a quale nome di modello sia oggi ancorato il requisito minimo dichiarato.
- **Decisione:** **esprimerlo in numeri.** Il requisito hardware minimo dichiarato del gioco completo si esprime in **VRAM, RAM e sistema operativo misurati sulla macchina di riferimento**, non in nomi di modello: un nome di modello cambia a ogni comparison (come è appena successo con DEC-140), un numero misurato no. Questa decisione registra la **policy di formulazione**; le misure concrete (nuovo benchmark sulla macchina di riferimento coi modelli di DEC-140) sono un'attività successiva, fuori da questo lavoro. Limite esplicito: l'allineamento più ampio di `docs/engineering/benchmarks.md` e `docs/engineering/dependencies.md` a DEC-140 è materia di un'altra domanda dello stesso batch di audit, non ancora chiusa — quei due documenti **non vengono riscritti ora**.
- **Alternative considerate:** continuare ad ancorare il requisito ai nomi di modello, aggiornandoli a ogni cambio di default; esprimere il requisito come fascia qualitativa ("fascia media") senza numeri.
- **Conseguenze:** supera **parzialmente** DEC-110 nella parte che definisce il requisito minimo per nome di modello: il principio di DEC-110 (un solo set di riferimento, nessun tier di qualità intermedio, hardware migliore = solo attese più brevi) resta valido, cambia solo la sua **espressione**, che diventa numerica. Nessun documento oltre al decision-log viene aggiornato in questo lavoro; la nota di sostituzione parziale è registrata su DEC-110 stesso.
- **Documenti aggiornati:** nessuno (nota di sostituzione parziale su DEC-110 in questo registro); `docs/engineering/benchmarks.md` e `docs/engineering/dependencies.md` restano da allineare in lavoro successivo.

---

### DEC-143 — Fusione cross-categoria: la categoria la eredita la sorgente dominante

- **Data:** 2026-07-25
- **Stato:** approved
- **Contesto:** DEC-101 ammette la fusione libera tra categorie diverse (es. attivo + Innesto) e stabilisce che l'oggetto risultante "dichiara la propria categoria", ma non specifica **quale** categoria erediti né, di conseguenza, quali campi obbligatori (`systems/items-pools-and-rarity.md`) la validazione (`systems/generated-content-validation.md`) deve pretendere. Il caso limite era già annotato in `systems/item-fusion.md` senza una regola di risoluzione.
- **Decisione:** **vince la sorgente dominante.** L'oggetto risultante eredita la categoria dell'oggetto sorgente di **rarità più alta**; a parità di rarità si applica la stessa regola di priorità già usata da `item-fusion.md` per i tratti in conflitto sulla stessa proprietà (punto 4 di "Priorità e conflitti": vince l'oggetto selezionato per primo dal giocatore). La validazione applica i campi obbligatori della categoria così ereditata.
- **Alternative considerate:** categoria decisa dal modello caso per caso senza regola fissa; sempre la categoria del primo oggetto selezionato indipendentemente dalla rarità; una categoria ibrida dedicata alle fusioni cross-categoria.
- **Conseguenze:** `systems/item-fusion.md` (sezione "Casi limite") recepisce la regola, con rimando alla priorità già definita per i tratti; nessun punto aperto corrispondente in `systems/active-items.md` o `systems/generated-content-validation.md` (non marcavano la domanda come aperta, nessuna modifica lì).
- **Documenti aggiornati:** `docs/design/systems/item-fusion.md`

---

### DEC-144 — Il pool curato minimo garantisce almeno un oggetto per rarità

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** il pool curato minimo di **20 oggetti** (DEC-087) distribuiti coi pesi di rarità di DEC-019 `{55, 30, 12, 3}` produrrebbe **0,6 oggetti leggendari**: la tabella del pool minimo pretende «nessuna rarità resta vuota», ma l'aritmetica dei pesi non lo garantisce. Serviva la regola che risolve la contraddizione.
- **Decisione:** nel pool curato minimo ogni rarità ha **almeno 1 oggetto**. L'eccedenza necessaria a rispettare il vincolo si **sottrae alle rarità più comuni**, non si aggiunge al totale: il totale minimo resta quello di DEC-087. L'esempio derivato onesto (11 comuni / 6 non-comuni / 2 rari / 1 leggendario = 20) va marcato esplicitamente come **derivato**, non come nuova tabella canonica: i valori esatti restano materia di playtest, in stile DEC-019.
- **Alternative considerate:** alzare il totale del pool minimo oltre 20 per far quadrare i pesi; accettare la rarità leggendaria vuota nella modalità solo-curato; distribuire per soli pesi senza garanzia di copertura.
- **Conseguenze:** `systems/generated-content-validation.md` (tabella del pool minimo) esplicita il vincolo «almeno 1 per rarità» e la regola di sottrazione, con l'esempio marcato come derivato; `systems/items-pools-and-rarity.md` vi rimanda senza duplicare. Le domande aperte sui numeri di DEC-019 e di DEC-087 restano aperte.
- **Documenti aggiornati:** `docs/design/systems/generated-content-validation.md`, `docs/design/systems/items-pools-and-rarity.md` (aggiornati in questo stesso lavoro)

---

### DEC-145 — La Fortuna riduce la soglia della correzione di fortuna, che vale su tutti i pool

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** la **correzione di fortuna** (`governance/glossary.md`, sostituisce il termine informale «pity») garantisce che dopo N estrazioni consecutive senza risultati sopra il comune la qualità minima salga, ma non era definito né come la statistica **Fortuna** del personaggio (`systems/player.md`) interagisca con N, né su quali pool la garanzia si applichi.
- **Decisione:** la Fortuna **riduce N**: più Fortuna significa protezione che scatta prima, non una probabilità gonfiata altrove. La correzione vale su **tutti i pool**, inclusi il pool ricompense del boss e il negozio. Dove un pool ha già una **garanzia strutturale superiore** — è il caso dei pesi boss `{0, 0, 70, 30}` di DEC-019, dove nessun risultato può essere comune o non-comune — la correzione è **soddisfatta per costruzione e non aggiunge nulla**: va detto esplicitamente nel documento, per non far credere a un doppio effetto cumulativo. Il valore base di N resta un default da playtest, in stile DEC-019.
- **Alternative considerate:** Fortuna che alza la qualità minima garantita invece di abbassare la soglia; correzione limitata ai soli drop di combattimento, con boss e negozio esclusi; correzione applicata solo al pool oggetti generale.
- **Conseguenze:** `systems/items-pools-and-rarity.md` resta fonte unica della correzione di fortuna e registra sia l'effetto della Fortuna su N sia l'estensione a tutti i pool, con la nota sui pool già garantiti per costruzione; `systems/player.md` vi rimanda con una riga dalla statistica Fortuna. Il valore di N resta da playtest.
- **Documenti aggiornati:** `docs/design/systems/items-pools-and-rarity.md`, `docs/design/systems/player.md` (aggiornati in questo stesso lavoro)

---

### DEC-146 — Il proxy della leggibilità visiva è la percentuale massima di schermo coperta

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** fra i controlli obbligatori della validazione dei contenuti generati compare la «leggibilità visiva», senza alcun criterio operativo: così com'era, nessun validatore automatico poteva applicarla e nessun agente poteva dire se un contenuto la superasse.
- **Decisione:** il **proxy primario** della leggibilità visiva è la **percentuale massima di schermo coperta** contemporaneamente da effetti, proiettili e telegraph. Un contenuto che la supera non passa la validazione e segue la normale catena di fallback. Accanto al proxy primario sono **ammessi controlli complementari** (per esempio contrasto minimo o numero di elementi simultanei), che non lo sostituiscono. Le soglie numeriche sono **provvisorie** in stile DEC-019: la conferma — e l'eventuale sistemazione — spetta ai playtest.
- **Alternative considerate:** lasciare la leggibilità al giudizio umano caso per caso; rinunciare a un criterio automatico e trattarla come sola linea guida per i prompt; adottare subito soglie definitive senza passare dal playtest.
- **Conseguenze:** `systems/generated-content-validation.md` registra il proxy come criterio operativo del controllo obbligatorio, con le soglie marcate provvisorie; `systems/combat-and-projectiles.md`, fonte unica del budget di leggibilità, vi rimanda con una riga.
- **Documenti aggiornati:** `docs/design/systems/generated-content-validation.md`, `docs/design/systems/combat-and-projectiles.md` (aggiornati in questo stesso lavoro)

---

### DEC-147 — Una sola coda di domande aperte: le code parallele si chiudono e si archiviano

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `docs/ai-production/19-DECISION-QUESTIONNAIRE.md` (24 domande con priorità BLOCKING/SOON/LATER) e il piano `docs/plans/active/aiprod-proposed-kb-updates.md` continuavano a marcare come BLOCKING domande già decise nel decision-log — audio generativo (DEC-109), licenza Stability (DEC-113), identità del Piano 0 (DEC-004/DEC-063/DEC-085), direzioni di mira (DEC-007), scelta binaria al primo avvio (DEC-070/DEC-086/DEC-111). Erano di fatto una seconda coda decisionale in concorrenza con `governance/open-questions.md`.
- **Decisione:** le domande già risolte si **chiudono citando la DEC** che le risolve; le domande **davvero residue** si **trasferiscono** in `governance/open-questions.md` come voci numerate, con citazione della provenienza; entrambi i file vengono poi **archiviati**. L'unica coda ufficiale delle domande aperte del progetto è `governance/open-questions.md`, coerentemente con la regola «niente registri paralleli» di `docs/CLAUDE.md`.
- **Alternative considerate:** mantenere il questionario come coda tecnica separata da quella di design; cancellare i due file invece di archiviarli; lasciare i marcatori BLOCKING e limitarsi ad annotarli.
- **Conseguenze:** `governance/open-questions.md` cresce delle domande residue trasferite (interfaccia, distribuzione, produzione AI e asset); i due file sorgente escono dalle cartelle vive, ciascuno per la strada del proprio tipo — il questionario in `docs/archive/superseded/`, il piano in `docs/plans/cancelled/` secondo la regola dei piani di DEC-157; `docs/_meta/TOPIC-ROUTER.md` guadagna la riga che indirizza qualunque domanda non risolta alla sola coda ufficiale.
- **Documenti aggiornati:** `docs/design/governance/open-questions.md`, `docs/_meta/TOPIC-ROUTER.md`, spostamento di `docs/ai-production/19-DECISION-QUESTIONNAIRE.md` in `docs/archive/superseded/` e di `docs/plans/active/aiprod-proposed-kb-updates.md` in `docs/plans/cancelled/` (aggiornati in questo stesso lavoro)

---

### DEC-148 — Pipeline immagini: SD1.5 confermato, Style LoRA su base vanilla, merge dopo validazione, dataset definitivi del proprietario

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** la comparison di seconda generazione del 23/07 (SD3.5 / SDXL / Flux contro SD1.5 su 6 GB) ha confermato che nessun modello moderno batte SD1.5 nel vincolo hardware del progetto, ma **nessuna decisione lo registrava**: la base immagini restava un fatto di fatto. Restavano inoltre senza risposta la base di partenza del training della Style LoRA, il luogo del training, il destino della LoRA una volta validata e la sorte dei dataset attuali.
- **Decisione:** cinque punti. **(a) Base immagini: SD1.5**, confermato come modello di riferimento per le immagini nel vincolo dei 6 GB. **(b)** La **Style LoRA si addestra su SD1.5 vanilla**, non sul checkpoint `pixel-baseline`: provenienza pulita e portabilità del risultato. **(c)** Il **training si fa su Kaggle** (dettaglio operativo in DEC-168). **(d)** A **validazione avvenuta**, la LoRA può essere **fusa nella base** per ottenere il **checkpoint proprietario del progetto**; quel checkpoint **candida la sostituzione di `pixel-baseline` nel runtime**, previa **asset review**. Fino ad allora il runtime **resta su `pixel-baseline`**. **(e)** I **dataset attuali non sono definitivi**, incluso il dataset Kaggle da 89k: i **dataset definitivi li creerà il proprietario**. Questa decisione **corregge affermazioni mai adottate** di `00-DECISIONI-CANONICHE.md`: la base vanilla, che lì era data per assunta, viene adottata **davvero adesso**; il piano dataset a due rami imperniato sul Kaggle 89k è **sostituito** dal piano dataset proprietario.
- **Alternative considerate:** salire a un modello di generazione più recente accettando un requisito VRAM superiore; addestrare la LoRA direttamente su `pixel-baseline` (più vicino alla resa attuale, ma provenienza opaca e non portabile); tenere la LoRA sempre separata dalla base senza mai fondere; considerare definitivo il dataset Kaggle 89k.
- **Conseguenze:** `docs/ai-production/00-DECISIONI-CANONICHE.md` registra la base SD1.5 e corregge le affermazioni non adottate; `docs/ai-production/03-PIANO-LORA.md` e `docs/ai-production/04-DATASET-LICENZE.md` recepiscono base vanilla, merge post-validazione e piano dataset proprietario; `docs/ai-production/dataset/README.md` segnala che i dataset attuali non sono definitivi. Il runtime non cambia in questo lavoro.
- **Documenti aggiornati:** `docs/ai-production/00-DECISIONI-CANONICHE.md`, `docs/ai-production/03-PIANO-LORA.md`, `docs/ai-production/04-DATASET-LICENZE.md`, `docs/ai-production/dataset/README.md` (aggiornati in questo stesso lavoro)

---

### DEC-149 — I benchmark testuali del 13/07 si congelano come misura storica

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `docs/engineering/benchmarks.md` presenta ancora la tabella del 13/07 con il 7B come modello testuale di default, superato da DEC-140 (Gemma-3-4B-IT Q4_K_M). Un lettore che arriva oggi legge come stato attuale una misura che non lo è più.
- **Decisione:** la tabella del 13/07 **si congela come misura storica**, con un **banner esplicito** in testa che dichiara data, modelli misurati e il fatto che non descrive il default attuale; accanto si aggiunge un **paragrafo sul default attuale** (DEC-140). **Nessuna rimisurazione ora**: rimisurare è un'attività successiva, fuori da questo lavoro.
- **Alternative considerate:** rimisurare subito tutti i modelli e riscrivere la tabella; cancellare la tabella superata; lasciarla com'è annotando solo la data.
- **Conseguenze:** `docs/engineering/benchmarks.md` diventa leggibile senza indurre in errore, senza perdere il dato storico. Resta aperto — e non chiuso da questa decisione — il nuovo benchmark sulla macchina di riferimento coi modelli attuali, già indicato da DEC-142 come attività successiva.
- **Documenti aggiornati:** `docs/engineering/benchmarks.md` (aggiornato in questo stesso lavoro)

---

### DEC-150 — DOC-CONFLICTS e DOC-CODE-DRIFT sono registri vivi con stato di chiusura per voce

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `docs/_meta/DOC-CONFLICTS.md` e `docs/_meta/DOC-CODE-DRIFT.md` erano fermi al 22/07, con risoluzioni proposte e mai applicate e nessun modo di sapere, voce per voce, se un conflitto fosse ancora aperto o già risolto altrove.
- **Decisione:** i due registri **restano documenti vivi**, non fotografie di una sessione. Ogni voce riceve un **campo di stato** — *aperta* / *applicata* / *superata* — accompagnato dalla **DEC o dal commit** che l'ha chiusa. Il `docs/_meta/TOPIC-ROUTER.md` li **nomina esplicitamente**, così chi cerca conflitti o drift sa dove guardare senza scoprirli per caso.
- **Alternative considerate:** archiviarli come istantanee del 22/07 e riaprirne di nuovi a ogni audit; fonderli in un unico registro; tenerli senza stato, verificandoli a mano ogni volta.
- **Conseguenze:** `docs/_meta/DOC-CONFLICTS.md` e `docs/_meta/DOC-CODE-DRIFT.md` acquistano il campo di stato su ogni voce esistente; `docs/_meta/TOPIC-ROUTER.md` li elenca fra le destinazioni del router.
- **Documenti aggiornati:** `docs/_meta/DOC-CONFLICTS.md`, `docs/_meta/DOC-CODE-DRIFT.md`, `docs/_meta/TOPIC-ROUTER.md` (aggiornati in questo stesso lavoro)

---

### DEC-151 — Nella KB il modello di testo si cita con una formula neutra

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** dopo DEC-140 la knowledge base è rimasta punteggiata di riferimenti puntuali a «Qwen», scritti quando quello era il default. Ogni cambio di modello costringerebbe a rincorrere gli stessi riferimenti in decine di documenti.
- **Decisione:** nei documenti si usa la **formula neutra** «il modello di testo attivo (oggi Gemma-3-4B-IT Q4, DEC-140)», perché il modello **può ancora cambiare**. I **nomi puntuali** restano solo dove sono il contenuto stesso del documento: `docs/ai-production/licenze.md` e questo decision-log. La sezione **Licenze** di `00-DECISIONI-CANONICHE.md` va integrata con i **Gemma Terms of Use** e la **Stability Community License** (DEC-113), oggi assenti.
- **Alternative considerate:** aggiornare i nomi puntuali a ogni cambio di default; introdurre un alias tecnico unico da sostituire in fase di build della documentazione.
- **Conseguenze:** i documenti che citavano «Qwen» come default passano alla formula neutra; `docs/ai-production/00-DECISIONI-CANONICHE.md` completa la sezione Licenze; `docs/ai-production/licenze.md` resta la fonte dei nomi e dei termini puntuali. Oltre a `licenze.md` e a questo registro, il **nome puntuale resta legittimo** anche in tre casi: le voci di licenza e i link bibliografici (`04-DATASET-LICENZE.md`, `14-FONTI.md`), i nomi di file dei pesi e il fallback selezionabile 1.5B/7B (`02-STACK-MODELLI.md`, `docs/engineering/dependencies.md`), e i documenti **storici o congelati** che riportano una misura fatta con quel modello (`01-AUDIT-DEL-PROGETTO.md`, `superseded/historical`; `docs/engineering/benchmarks.md` congelato da DEC-149; le specifiche del 13/07 e il research pack). Riscriverli falsificherebbe una misura: la formula neutra vale per chi descrive il presente, non per chi registra il passato.
- **Documenti aggiornati:** `docs/ai-production/00-DECISIONI-CANONICHE.md`, `docs/ai-production/02-STACK-MODELLI.md`, `docs/ai-production/07-ARCHITETTURA-RUNTIME.md`, `docs/ai-production/09-NEMICI-BODY-PLAN-RIG.md`, `docs/ai-production/10-PIANO-INTEGRAZIONE-C.md`, `docs/ai-production/README.md`, `docs/ai-production/regole-agenti-ml.md`, `docs/design/content/audio-and-feedback.md`, `docs/engineering/dependencies.md` (aggiornati in questo stesso lavoro)

---

### DEC-152 — Le card di scoperta in coda si scartano alla morte o al cambio stanza

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-131 regola solo il **cap** della coda delle card di scoperta e il suo **overflow** (le più vecchie escono senza essere mostrate). Restava indefinito cosa accade alle card ancora in coda quando il giocatore muore o cambia stanza, e `ui/hud.md` era internamente contraddittorio sul punto.
- **Decisione:** se il giocatore **muore** o **cambia stanza**, le card non ancora mostrate vengono **scartate silenziosamente**: nessuna coda che insegue il giocatore nella stanza successiva, nessun recupero differito. La **scoperta resta comunque registrata nel Catalogo permanente** con la sua scheda, esattamente come nell'overflow di DEC-131: la card è la notifica, non il contenuto.
- **Alternative considerate:** trasportare la coda nella stanza successiva; mostrare le card residue in un riepilogo al cambio stanza o a fine run.
- **Conseguenze:** `ui/hud.md` registra la regola e **sana la propria contraddizione interna**; nessuna perdita di informazione per il giocatore, che ritrova tutto nel Catalogo.
- **Documenti aggiornati:** `docs/design/ui/hud.md` (aggiornato in questo stesso lavoro)

---

### DEC-153 — Il contenuto curato di fallback è lo stato base del gioco, sempre pronto

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** alcuni documenti descrivevano il fallback curato come qualcosa che «diventa disponibile», arrivando a ipotizzare un caso limite in cui l'uscita dal Piano 0 resta disabilitata senza limite di tempo perché *nemmeno il fallback è pronto*.
- **Decisione:** il **contenuto curato di fallback è lo stato base del gioco**: precaricato e disponibile **per costruzione**, mai in attesa, mai in caricamento. Il caso limite «uscita dal Piano 0 disabilitata senza limite perché nemmeno il fallback è pronto» **non esiste** e va rimosso dai documenti che lo ipotizzano. È la lettura coerente con DEC-002 (il gioco è sempre avviabile) e DEC-020 (mai un blocco della partita).
- **Alternative considerate:** trattare il fallback come risorsa caricata su richiesta con un proprio stato di attesa; definire un timeout oltre il quale l'uscita si sblocca comunque.
- **Conseguenze:** `systems/generated-content-validation.md`, fonte unica della regola di fallback, registra il fallback come stato base sempre pronto; `systems/floor-zero.md` e `ui/generation-status.md` rimuovono il caso limite e descrivono il Piano 0 curato come pronto senza attesa. `06-ai-content-generation-model.md` non è stato toccato: parlava già di «fallback curato sempre presente», nessuna formulazione da correggere lì.
- **Documenti aggiornati:** `docs/design/systems/generated-content-validation.md`, `docs/design/systems/floor-zero.md`, `docs/design/ui/generation-status.md` (aggiornati in questo stesso lavoro)

---

### DEC-154 — DEC-019 passa ad approved

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-019 («valori numerici attuali come default draft») era l'unica decisione del registro rimasta in stato `draft`, pur essendo la **base numerica citata da almeno cinque decisioni approved** (DEC-087, DEC-107, DEC-133 e le tabelle di `bosses.md`/`enemies.md`) e da una decina di documenti canonici. Uno stato `draft` che regge tanto canone è una contraddizione di governance.
- **Decisione:** DEC-019 viene **promossa ad approved**, previa verifica di assenza di conflitti con le altre decisioni. La promozione riguarda l'**impianto** — quattro rarità, esistenza dei pesi di rarità e di pool boss, esistenza delle bande di potenza per colpi, nemici e boss — **non i valori esatti**, che restano materia di playtest come già registrato nelle domande aperte. I documenti che citano quei numeri continuano a marcarli «default proposto, da validare col playtest».
- **Alternative considerate:** lasciare DEC-019 in `draft` fino al playtest, accettando che decisioni approved poggino su una decisione draft; approvare anche i valori esatti chiudendo le domande aperte relative.
- **Conseguenze:** la promozione è applicata sul campo **Stato** di DEC-019 in questo registro, con nota datata; le domande aperte sui valori numerici **restano aperte**; nessun documento di sistema cambia i propri numeri.
- **Documenti aggiornati:** `docs/design/governance/decision-log.md` (campo Stato di DEC-019, in questo stesso lavoro)

---

### DEC-155 — Le decisioni integralmente sostituite ricevono lo stato superseded

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** il registro prevede lo stato `superseded` nel proprio template, ma non l'ha mai usato: le sostituzioni sono state finora annotate solo in prosa, nelle note in calce. Chi legge una voce vecchia non distingue a colpo d'occhio lo storico dal canone attuale. La **modalità** del retrofit era delegata al coordinatore.
- **Decisione:** retrofit del **campo strutturato** `Stato: superseded (sostituita da DEC-NNN)` sulle decisioni **integralmente sostituite**, da individuare verificando le note in calce. Le **sostituzioni parziali restano `approved` con nota**: se una parte della decisione è ancora canone, la voce non è superata. Il **testo storico non si riscrive**: cambia solo il campo Stato. Così lo storico resta leggibile ma non si confonde con il canone attuale.
- **Alternative considerate:** marcare `superseded` anche le sostituzioni parziali; spostare le decisioni superate in un archivio separato; lasciare tutto alle note in prosa.
- **Conseguenze:** applicato in questo stesso lavoro. Verifica delle note in calce: l'unica decisione **integralmente** sostituita è **DEC-003** (il nome provvisorio del gioco), il cui contenuto operativo è interamente esaurito da DEC-071 — che dichiara essa stessa «risolve DEC-003». Le altre annotazioni del registro sono **integrazioni o sostituzioni parziali** e restano `approved` con nota: DEC-006 (solo la prosecuzione in piani extra è superata da DEC-031; vittoria al piano 5 e permadeath restano canone citato dai documenti), DEC-013 (solo i nomi placeholder sono fissati da DEC-072; le risorse per funzione restano canone), DEC-010 (integrata da DEC-051), DEC-014 (integrata da DEC-030), DEC-016 (corretta da DEC-141), DEC-018 (integrata da DEC-031/DEC-036/DEC-045), DEC-021 (estesa da DEC-062), DEC-036 (sostituita solo nella parte «generazione = futuro» da DEC-109), DEC-110 (superata parzialmente da DEC-142).
- **Documenti aggiornati:** `docs/design/governance/decision-log.md` (campo Stato di DEC-003 e note di classificazione, in questo stesso lavoro)

---

### DEC-156 — DEC-137 e DEC-139 entrano nei flussi UI; la risoluzione logica resta una domanda aperta

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-137 (una sola schermata, GUI in overlay) e DEC-139 (TAB apre `BuildScreen` dal Gameplay) erano registrate ma non ancora recepite nei documenti che descrivono navigazione e stati. In parallelo, i template e gli appunti di interfaccia propongono una risoluzione logica di **640×360 con scaling intero** che **nessuna decisione ha mai approvato**.
- **Decisione:** **recepire subito** DEC-137 e DEC-139 in `ui/navigation-map.md` e `05-game-states-and-flow.md`. La **risoluzione logica 640×360 con scaling intero non è approvata**: si apre come **domanda aperta numerata** in `governance/open-questions.md`, e nel template `UI-SKIN-SPEC` va **marcata esplicitamente come non approvata**, così che nessuna implementazione la prenda per canone.
- **Alternative considerate:** approvare 640×360 contestualmente al recepimento dei flussi; rimandare anche il recepimento di DEC-137/DEC-139 all'implementazione del refactor.
- **Conseguenze:** i due documenti di flusso descrivono la schermata unica con overlay e l'ingresso rapido alla build; `governance/open-questions.md` guadagna la domanda sulla risoluzione logica; `docs/ai-production/templates/UI-SKIN-SPEC.md` marca il valore come proposta non approvata.
- **Documenti aggiornati:** `docs/design/ui/navigation-map.md`, `docs/design/05-game-states-and-flow.md`, `docs/design/governance/open-questions.md`, `docs/ai-production/templates/UI-SKIN-SPEC.md` (aggiornati in questo stesso lavoro)

---

### DEC-157 — Chi chiude un piano lo sposta fuori da plans/active nello stesso commit

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `docs/plans/active/` conteneva piani di fatto conclusi (la comparison modelli, conclusa il 23/07) perché nessuna regola diceva **chi** e **quando** sposta un piano fuori dagli attivi. La modalità era **decisione delegata al coordinatore**.
- **Decisione:** **chi completa il lavoro che chiude un piano lo sposta** in `docs/plans/completed/` (o `docs/plans/cancelled/`) **nello stesso commit** che chiude il lavoro. La **responsabilità di ultima istanza** resta al **coordinatore di sessione**, che a fine sessione verifica che `plans/active/` contenga solo piani davvero attivi. **Applicazione immediata:** `docs/plans/active/model-comparison.md` passa in `completed/`.
- **Alternative considerate:** revisione periodica dedicata dei piani attivi; spostamento a carico del solo coordinatore; nessuna regola, con `plans/active/` inteso come archivio cronologico.
- **Conseguenze:** la regola entra in `docs/_meta/DOCUMENT-STANDARDS.md` (§ «Regole di aggiornamento»), accanto alle altre regole di ciclo di vita dei documenti, e non in un README di cartella: `docs/plans/` non ha un README e non ne serve uno per una regola sola. `model-comparison.md` viene spostato in `docs/plans/completed/`.
- **Documenti aggiornati:** `docs/_meta/DOCUMENT-STANDARDS.md`, spostamento di `docs/plans/active/model-comparison.md` in `docs/plans/completed/` (aggiornati in questo stesso lavoro)

---

### DEC-158 — Il tema della distribuzione appartiene al dominio ai-production

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** piattaforme di destinazione, requisiti della pagina negozio e **AI disclosure** non avevano un proprietario: comparivano di sfuggita in `docs/engineering/multiplayer-steam.md` e nel questionario ai-production, senza che nessun dominio se ne facesse carico. La proprietà era **decisione delegata al coordinatore**.
- **Decisione:** il tema **distribuzione** è di proprietà del dominio **ai-production**, che già possiede licenze e provenienza dei contenuti — le due materie da cui la AI disclosure dipende — e vive in un **documento dedicato** del dominio. `docs/engineering/multiplayer-steam.md` **rimanda** a quel documento senza duplicarne il contenuto. Le **decisioni concrete** (quali piattaforme, quale formato di disclosure, quali requisiti di pagina) **restano domande aperte**: questa decisione assegna la proprietà, non le risposte.
- **Alternative considerate:** assegnare la distribuzione a engineering, insieme all'integrazione Steam; creare un dominio «produzione/publishing» dedicato; lasciare il tema senza proprietario fino alla prima decisione concreta.
- **Conseguenze:** nasce `docs/ai-production/21-DISTRIBUZIONE.md`; `docs/engineering/multiplayer-steam.md` vi rimanda e dichiara che le proprie voci restano solo come vincoli tecnici dell'integrazione Steamworks; `docs/_meta/TOPIC-ROUTER.md` guadagna la riga «Distribuzione» che indirizza al nuovo documento; le domande concrete entrano in `governance/open-questions.md` (vedi DEC-147).
- **Documenti aggiornati:** `docs/ai-production/21-DISTRIBUZIONE.md` (nuovo), `docs/engineering/multiplayer-steam.md`, `docs/_meta/TOPIC-ROUTER.md`, `docs/design/governance/open-questions.md` (aggiornati in questo stesso lavoro)

---

### DEC-159 — RunResults dichiara la causa della sconfitta

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `02-player-experience.md` promette che alla morte il giocatore deve «capire perché è morto», ma nessun elemento di interfaccia presidiava quella promessa: `RunResults` mostrava risultato, punteggio e creazioni, non la causa. La forma del presidio era **decisione delegata al coordinatore**.
- **Decisione:** `RunResults` espone un **campo esplicito con la causa della sconfitta** — **ultimo colpo o nemico letale** — mostrato alla morte. Nessuna telemetria, nessun grafico: una dichiarazione leggibile di cosa ha chiuso la run, coerente col registro del crogiolo (DEC-105) e accanto alle righe già previste dalla schermata (DEC-132).
- **Alternative considerate:** un riepilogo esteso degli ultimi secondi di combattimento; nessun campo dedicato, affidando la comprensione al solo feedback in gioco; una ripetizione video del momento della morte.
- **Conseguenze:** `ui/results-and-leaderboards.md` registra il campo fra i contenuti canonici di `RunResults`; `02-player-experience.md` collega la propria promessa al campo che la presidia.
- **Documenti aggiornati:** `docs/design/ui/results-and-leaderboards.md`, `docs/design/02-player-experience.md` (aggiornati in questo stesso lavoro)

---

### DEC-160 — L'Innesto sganciato resta a terra, recuperabile nella stanza

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-115 permette di sganciare volontariamente un Innesto equipaggiato, ma lasciava esplicitamente indefinito il destino dell'Innesto sganciato: resta a terra o si perde?
- **Decisione:** l'Innesto sganciato **resta a terra ed è recuperabile finché si resta nella stanza**; **uscendo dalla stanza si perde**. È la stessa **reversibilità locale** già approvata per lo scambio degli attivi sui piedistalli (DEC-117): la scelta si può ripensare finché sei lì, non per tutta la run.
- **Alternative considerate:** Innesto sganciato perso immediatamente; Innesto che rientra in un inventario di riserva trasportabile; recupero possibile per tutta la run.
- **Conseguenze:** `systems/grafts.md` chiude il punto lasciato aperto da DEC-115 e registra la simmetria con i piedistalli degli attivi; `systems/active-items.md` non cambia.
- **Documenti aggiornati:** `docs/design/systems/grafts.md` (aggiornato in questo stesso lavoro)

---

### DEC-161 — I conflitti di categoria senza priorità esplicita si risolvono a caso, con l'RNG del seed di run

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** la gerarchia di risoluzione delle sinergie definisce le priorità caso per caso, ma non dice cosa accade quando due effetti di **categorie diverse** entrano in conflitto e **nessuna regola esplicita** stabilisce chi vince. Senza risposta, il comportamento resta a discrezione dell'implementazione.
- **Decisione:** **non esiste un ordine fisso fra le categorie**. Quando la priorità non è esplicitata dalle regole, il conflitto si risolve in modo **casuale**, ma con **RNG derivato dal seed di run**: deterministico e stabile dentro la stessa run — stessa run, stesso esito — coerente con DEC-016 e col prerequisito di DEC-141 (l'RNG di gameplay deve derivare dal seed di run; finché il fix non c'è, la garanzia vale come requisito, non come stato attuale).
- **Alternative considerate:** un ordine fisso di precedenza fra le categorie; vince sempre l'effetto acquisito per ultimo; nessuna risoluzione, con entrambi gli effetti applicati.
- **Conseguenze:** `systems/synergies.md` registra la regola nella gerarchia di risoluzione, con il rimando esplicito al seed di run; `systems/item-fusion.md` mantiene le proprie priorità già definite (DEC-143), che restano esplicite e quindi prevalgono su questa clausola residuale.
- **Documenti aggiornati:** `docs/design/systems/synergies.md` (aggiornato in questo stesso lavoro)

---

### DEC-162 — Il risultato di sinergie e fusioni ha un budget di potenza dedicato

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** il budget di potenza è definito per il **singolo oggetto**, ma una sinergia implicita o una fusione esplicita producono un risultato che vale più della somma delle parti: senza un tetto proprio, il risultato o viene schiacciato sul budget del singolo (e la meccanica-firma perde senso) o non viene verificato affatto.
- **Decisione:** oltre al budget del singolo oggetto esiste un **budget dedicato al risultato** di una sinergia implicita o di una fusione esplicita, **più alto di quello del singolo** e **verificato in validazione** come tutti gli altri budget. Il **valore esatto è draft**, da playtest, in stile DEC-019: questa decisione fissa l'esistenza del budget dedicato e il fatto che sia più alto, non il numero.
- **Alternative considerate:** applicare al risultato lo stesso budget del singolo oggetto; non verificare affatto il risultato, fidandosi dei budget dei genitori; tetto proporzionale alla somma dei genitori senza un budget proprio.
- **Conseguenze:** `systems/generated-content-validation.md` registra il budget dedicato fra i controlli; `systems/synergies.md` e `systems/item-fusion.md` vi rimandano; il valore resta materia di playtest.
- **Documenti aggiornati:** `docs/design/systems/generated-content-validation.md`, `docs/design/systems/synergies.md`, `docs/design/systems/item-fusion.md` (aggiornati in questo stesso lavoro)

---

### DEC-163 — I template di contenuto aprono con una riga di vincoli che rimanda al contratto

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** i template di contenuto (boss, nemico, oggetto, stanza, sinergia, schermata UI) non ricordavano i vincoli che ogni contenuto deve rispettare: chi compilava un template doveva sapere a memoria che esistono limiti di contenuto, tono e tassonomia dei tag. La forma del richiamo era **decisione delegata al coordinatore**, con criterio dichiarato: **massima leggibilità per umani e agenti**.
- **Decisione:** ogni template di contenuto apre con una **riga standard «Vincoli di contenuto»** che **rimanda a `templates/generated-content-contract.md`**, senza riformulare i vincoli sul posto. Il **contratto centralizza esplicitamente**: i limiti di contenuto dark ~16+ (DEC-119), il tono di `content/narrative-tone.md` e le **sei famiglie di tag** di `content/content-taxonomy.md`. Inoltre la sezione **Tag** di `item-spec-template.md`, oggi vuota, va **compilata con le sei famiglie**.
- **Alternative considerate:** ripetere i vincoli per esteso in ogni template (ridondante e destinato a divergere); non ricordarli affatto, affidandosi alla validazione a valle; un solo documento di vincoli senza riga nei template.
- **Conseguenze:** i sei template acquistano la riga standard; `templates/generated-content-contract.md` diventa il punto unico dove i vincoli sono elencati per esteso; `item-spec-template.md` ha la sezione Tag compilata.
- **Documenti aggiornati:** `docs/design/templates/generated-content-contract.md`, `docs/design/templates/boss-spec-template.md`, `docs/design/templates/enemy-spec-template.md`, `docs/design/templates/item-spec-template.md`, `docs/design/templates/room-spec-template.md`, `docs/design/templates/synergy-spec-template.md`, `docs/design/templates/ui-screen-template.md` (aggiornati in questo stesso lavoro)

---

### DEC-164 — I protocolli 11 e 20 passano ad approved con nota di precedenza; il protocollo esperimenti copre anche i bake-off

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `11-PROTOCOLLO-ESPERIMENTI.md` e `20-SESSION-PROTOCOL.md` erano documenti `proposed` pur essendo di fatto la procedura seguita dagli agenti; in più il protocollo esperimenti descrive l'esperimento singolo, mentre i report del 23/07 sono **comparison su più modelli** e non trovavano una forma prevista. La modalità di promozione era **decisione delegata al coordinatore**.
- **Decisione:** `11-PROTOCOLLO-ESPERIMENTI.md` viene **esteso con una variante «comparison/bake-off»** — che legittima il formato dei report del 23/07 — e **promosso ad approved** con **nota di precedenza di `CLAUDE.md`**, sullo stesso modello già adottato per `18-AGENT-ORCHESTRATION.md`. `20-SESSION-PROTOCOL.md` viene **promosso ad approved** con la **stessa nota di precedenza**. Dove i due protocolli e `CLAUDE.md` divergono, vale `CLAUDE.md`.
- **Alternative considerate:** lasciarli `proposed` e trattarli come raccomandazioni; promuoverli senza nota di precedenza (rischio di conflitto con la scala agenti di `CLAUDE.md`); creare un protocollo separato per i bake-off.
- **Conseguenze:** i due documenti cambiano `status` nel front matter e guadagnano la nota di precedenza; `11-PROTOCOLLO-ESPERIMENTI.md` guadagna la sezione sulla variante comparison/bake-off; i report di comparison esistenti risultano conformi.
- **Documenti aggiornati:** `docs/ai-production/11-PROTOCOLLO-ESPERIMENTI.md`, `docs/ai-production/20-SESSION-PROTOCOL.md` (aggiornati in questo stesso lavoro)

---

### DEC-165 — Il gate di DEC-138 è soddisfatto: il mechanics-lab è sbloccato

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-138 subordinava l'avvio del mechanics-lab a due condizioni: la conclusione del refactor GUI di DEC-137 e la conclusione della comparison modelli. Entrambe si sono chiuse (refactor concluso; comparison chiusa il 23/07, con i report congelati in `docs/ai-production/experiments/`).
- **Decisione:** il **gate è soddisfatto**: il piano `docs/plans/active/mechanics-lab.md` è **sbloccato e può partire**. Restano invariati il criterio di successo e i vincoli fissati da DEC-138 — generare laser, catena e orbita **senza** primitive dedicate a laser, catena o orbita; niente inferenza in combattimento; primitive vincenti ammesse nel motore solo con clamp, budget, `ShotTypeBalance` e fallback.
- **Alternative considerate:** attendere anche la validazione estesa del modello testuale raccomandata da DEC-140; rinviare il mechanics-lab dopo il lavoro sulla pipeline immagini.
- **Conseguenze:** `docs/plans/active/mechanics-lab.md` registra il gate come soddisfatto e passa a piano eseguibile; nessun cambio ai criteri di DEC-138.
- **Documenti aggiornati:** `docs/plans/active/mechanics-lab.md` (aggiornato in questo stesso lavoro)

---

### DEC-166 — Il lettore di schermo resta un obiettivo dei soli menu testuali

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** `ui/main-menu.md` accennava a un supporto per lettore di schermo senza perimetro né stato, in una zona grigia rispetto alle tre garanzie canoniche di accessibilità di DEC-058 (rimappatura totale, nessuna informazione affidata al solo colore, riduzione effetti).
- **Decisione:** il supporto al lettore di schermo resta un **obiettivo**, non una garanzia, ed è **circoscritto a `MainMenu` e ai menu testuali semplici**. **Non è esteso al gameplay** e **non entra fra le garanzie canoniche di DEC-058**. `ui/main-menu.md` va circoscritto di conseguenza; `ui/options-and-accessibility.md`, fonte unica dell'accessibilità, lo registra fra gli **obiettivi non-garanzia**, distinti dalle tre garanzie.
- **Alternative considerate:** promuoverlo a quarta garanzia canonica estesa a tutto il gioco; rimuovere del tutto l'accenno; lasciarlo generico senza perimetro.
- **Conseguenze:** `ui/options-and-accessibility.md` distingue le tre garanzie dagli obiettivi non-garanzia e accoglie il lettore di schermo fra i secondi; `ui/main-menu.md` circoscrive l'accenno e rimanda alla fonte unica. DEC-058 resta invariata.
- **Documenti aggiornati:** `docs/design/ui/options-and-accessibility.md`, `docs/design/ui/main-menu.md` (aggiornati in questo stesso lavoro)

---

### DEC-167 — La valuta arriva da qualunque stanza completata secondo la propria condizione

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** DEC-048 indica «nemici sconfitti e **stanze ripulite**» come uniche fonti canoniche della valuta principale, ma «ripulita» è un verbo da stanza di combattimento: restava ambiguo se una stanza del tesoro, un negozio o una segreta contino come fonte.
- **Decisione:** «stanza ripulita» significa **qualunque stanza completata secondo la propria condizione di completamento**: combattimento vinto, tesoro aperto, negozio visitato, segreto trovato. Non solo le stanze di combattimento. La formulazione di DEC-048 resta valida, chiarita nel suo significato: le fonti canoniche restano due (nemici sconfitti e stanze completate), non si aggiungono fonti nuove.
- **Alternative considerate:** limitare la fonte alle sole stanze di combattimento; introdurre valori di valuta differenziati per tipo di stanza come parte di questa decisione (rinviato al playtest).
- **Conseguenze:** `systems/rewards-and-economy.md` chiarisce la formulazione delle fonti canoniche; `systems/rooms-and-floor-generation.md` e `systems/special-rooms.md` rimandano alla condizione di completamento propria di ciascun archetipo. Gli importi restano materia di playtest.
- **Documenti aggiornati:** `docs/design/systems/rewards-and-economy.md` (aggiornato in questo stesso lavoro)

---

### DEC-168 — Il training della Style LoRA si fa su Kaggle; il runbook RunPod è fallback a pagamento

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** il progetto aveva **due runbook di training** con stati incoerenti: `dataset/TRAINING-RUNBOOK.md` (RunPod, a pagamento) era `approved`, mentre `05-KAGGLE-TRAINING-RUNBOOK.md` non lo era — l'opposto dell'intenzione, visto che Kaggle offre **30 ore di GPU gratuite a settimana**.
- **Decisione:** il training della Style LoRA si fa **su Kaggle**. `05-KAGGLE-TRAINING-RUNBOOK.md` diventa il **runbook primario**, aggiornato alla base **SD1.5 vanilla** (DEC-148). `dataset/TRAINING-RUNBOOK.md` passa da `approved` a **`proposed`**, nel ruolo di **fallback a pagamento**. I due runbook si **citano a vicenda**, così che chi apre l'uno sappia dell'altro e del suo ruolo.
- **Alternative considerate:** restare su RunPod per prevedibilità e assenza di limiti di sessione; mantenere entrambi i runbook allo stesso rango; eliminare il runbook RunPod.
- **Conseguenze:** i due documenti scambiano stato e ruolo e acquistano i rimandi reciproci; `03-PIANO-LORA.md` indica Kaggle come via primaria; il budget cloud resta una domanda aperta (nessuna spesa impegnata da questa decisione).
- **Documenti aggiornati:** `docs/ai-production/05-KAGGLE-TRAINING-RUNBOOK.md`, `docs/ai-production/dataset/TRAINING-RUNBOOK.md`, `docs/ai-production/03-PIANO-LORA.md` (aggiornati in questo stesso lavoro)

---

### DEC-169 — Nel Piano 0 l'HUD è nascosto, consultabile in pausa, visibile nelle prove

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** il Piano 0 è un hub (DEC-004) dove non si combatte, ma l'HUD di combattimento — salute, risorse, slot — vi compariva comunque, senza che nessuna decisione dicesse se debba esserci. Il punto era rimasto nel backlog dell'implementazione.
- **Decisione:** durante il Piano 0 l'**HUD di combattimento è nascosto**; resta **consultabile su richiesta dal menu di pausa**, per chi vuole controllare salute, risorse e build senza uscire dall'hub; **ricompare quando si entra nelle prove**, cioè nelle attività di combattimento e tutorial del Piano 0 (DEC-047). Il Piano 0 resta uno spazio di preparazione con lo schermo pulito, senza togliere informazione a chi la cerca.
- **Alternative considerate:** HUD sempre visibile anche nell'hub; HUD sempre nascosto, prove comprese; versione ridotta permanente dell'HUD nel Piano 0.
- **Conseguenze:** `ui/hud.md` registra la regola di visibilità per stato; `systems/floor-zero.md` la applica al Piano 0 e `ui/pause-menu.md` colloca la consultazione su richiesta. Le «prove» del Piano 0 di questa decisione sono le arene di sfida e il tutorial integrato (DEC-047), **non** le prove specifiche della run di DEC-042: i tre documenti lo dicono esplicitamente. Questa decisione **non fissa il comando** con cui il menu di pausa si apre dal Piano 0, dove ESC è già assegnato a `ExitConfirm` (DEC-074): il punto diventa la domanda aperta 22.
- **Documenti aggiornati:** `docs/design/ui/hud.md`, `docs/design/systems/floor-zero.md`, `docs/design/ui/pause-menu.md`, `docs/design/governance/open-questions.md` (aggiornati in questo stesso lavoro)

---

### DEC-170 — Le stanze hanno taglie multiple stile Isaac; la telecamera segue a zoom fisso nelle stanze più grandi

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** l'avvio dell'implementazione della demo ha reso necessario fissare la forma esatta della variabilità dimensionale delle stanze lasciata aperta da DEC-009 («grandezze tutte diverse tra loro», senza un modello di riferimento) e il comportamento dell'inquadratura quando una stanza supera lo schermo singolo — punto mai deciso, dato che l'implementazione M2 attuale non prevede alcuna telecamera (l'intera stanza sta sempre nel rettangolo fisso del canvas).
- **Decisione:** le stanze hanno **taglie multiple in classi discrete stile Isaac**: 1x1 (base), 1x2, 2x1, 2x2, più **forme a L** (tre celle contigue di un blocco 2x2 con un angolo mancante). Più stanze dello stesso piano possono condividere la stessa taglia — la clausola di DEC-009 «grandezze tutte diverse tra loro» **non si applica più** in questa forma: la variabilità ora è di **classe**, non di misura continua univoca per stanza. La **grandezza minima garantita** di DEC-009 resta la taglia 1x1. Sulla **telecamera**: le stanze **1x1** restano inquadrate per intero con **camera fissa** (comportamento invariato); nelle taglie **maggiori** (1x2, 2x1, 2x2, L) il giocatore cammina dentro uno spazio più ampio dello schermo e la **telecamera lo segue a zoom fisso** — **nessuno zoom dinamico** — **clampata ai bordi della stanza** (non mostra mai area fuori dal rettangolo occupato dalla stanza). La generazione dei piani assegna le taglie alle stanze; il modo in cui i layout di ostacoli esistenti (`ROOM_LAYOUT_*`) si applicano alle stanze multi-cella — per singola cella o estesi sull'intera stanza — resta un **dettaglio di implementazione**, non fissato da questa decisione.
- **Alternative considerate:** mantenere il modello attuale di taglie continue tutte diverse tra loro senza telecamera (avrebbe impedito le stanze davvero grandi stile Isaac); zoom dinamico che si adatta alla taglia della stanza invece di zoom fisso; camera libera non clampata ai bordi della stanza.
- **Conseguenze:** `systems/rooms-and-floor-generation.md` sostituisce il modello di taglie continue con le classi discrete e aggiunge la sezione sul comportamento della telecamera; DEC-009 riceve una nota di supersessione parziale (resta approved: il principio di variabilità e il minimo garantito restano validi, la clausola «tutte diverse tra loro» è superata dalla forma a classi). Nessun rimando aggiunto a `ui/hud.md`, che non parla di inquadratura.
- **Documenti aggiornati:** `docs/design/systems/rooms-and-floor-generation.md`, `docs/design/governance/decision-log.md` (nota su DEC-009) (aggiornati in questo stesso lavoro)

---

### DEC-171 — La demo copre tutti i sistemi documentati; il contenuto curato usa le immagini del dataset CC0 come ponte provvisorio

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** l'avvio dell'implementazione della demo ha reso necessario fissare l'obiettivo della demo stessa — copertura dei sistemi o run rifinita — e la fonte delle immagini del contenuto curato: la pipeline immagini definitiva (Style LoRA su SD1.5 vanilla, DEC-148) non è ancora addestrata, ma la demo non può restare senza sprite.
- **Decisione:** l'obiettivo della demo è **implementare tutti i sistemi documentati** (fusioni, sinergie, correzione di fortuna, economia, ecc.) **con priorità alla copertura del design** rispetto a una run end-to-end rifinita. Il contenuto curato della demo usa le **immagini del dataset di training** (`dataset-raw/`, pacchetti Kenney più `superpowers-asset-packs`, licenza **CC0**, **~2.567 PNG**, già registrate nel ledger CC0 di `docs/ai-production/04-DATASET-LICENZE.md`). L'immagine di un oggetto **nato da fusione** — che non ha un'immagine curata propria — si **pesca dal dataset fra le immagini non ancora usate nella run corrente**, con **scelta deterministica dal seed di run**. **Nessun modello immagine gira a runtime nella demo.** Questa soluzione è **esplicitamente provvisoria**: serve solo a permettere il playtest delle meccaniche finché SD non è stato addestrato con la Style LoRA. **DEC-148 resta invariata**: questa decisione è il ponte provvisorio, non la pipeline definitiva.
- **Alternative considerate:** rimandare la demo fino al training della Style LoRA (avrebbe bloccato il playtest delle meccaniche per settimane); generare le immagini della demo con SD1.5 vanilla a runtime (avrebbe introdotto un modello immagine a runtime, contro l'indipendenza del motore, e prodotto sprite di qualità peggiore del dataset curato); costruire una run end-to-end rifinita di pochi sistemi invece di coprire tutti i sistemi documentati.
- **Conseguenze:** `systems/item-fusion.md` registra la fonte provvisoria dell'immagine di un oggetto fuso nella demo; `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md` registra il dataset Kenney CC0 come fonte curata della demo, con nota di licenza. Nessun cambiamento alla pipeline definitiva di DEC-148.
- **Documenti aggiornati:** `docs/design/systems/item-fusion.md`, `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md` (aggiornati in questo stesso lavoro)

---

### DEC-172 — L'audio della demo è un pacchetto pre-generato offline; il motore acquisisce un modulo audio che legge solo asset statici

- **Data:** 2026-07-27
- **Stato:** approved
- **Contesto:** l'avvio dell'implementazione della demo ha reso necessario decidere come la demo produce l'audio, dato che DEC-109 fissa Stable Audio Small come via primaria a runtime ma la regola di `AGENTS.md` («motore indipendente dai modelli AI»: il runtime legge solo file locali già validati) impone che la demo non dipenda da modelli AI a runtime.
- **Decisione:** la demo usa un **pacchetto audio pre-generato offline**: musica/ambience per i temi con **Stable Audio 3 Small** (i checkpoint music e sfx già presenti in `models/`), effetti procedurali con **rFXGen**, tutto **integrato nel gioco come asset statici** prodotti prima della build. Il motore acquisisce un **modulo audio** (raylib) che **legge solo file locali già pronti** — coerente con la regola di `AGENTS.md` sull'indipendenza del motore dai modelli AI. **DEC-109 resta la destinazione finale** (pipeline generativa a runtime): questa decisione è l'**istanza demo** del fallback curato sempre garantito già previsto da DEC-036/DEC-109, non una nuova pipeline.
- **Alternative considerate:** generare audio a runtime con Stable Audio anche nella demo (avrebbe richiesto integrare il modello nel binario di gioco, contro la regola di indipendenza del motore); lasciare la demo senza audio fino all'integrazione della pipeline generativa a runtime.
- **Conseguenze:** `content/audio-and-feedback.md` registra che la demo usa il pacchetto pre-generato; `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` registra la nota demo (generazione offline, runtime statico). Nessun cambiamento alla pipeline definitiva di DEC-109.
- **Documenti aggiornati:** `docs/design/content/audio-and-feedback.md`, `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` (aggiornati in questo stesso lavoro)
- **Nota (2026-07-28):** superata parzialmente da DEC-178 — «effetti procedurali con rFXGen» non descriveva la realtà: rFXGen non è mai stato installato né usato, gli SFX della demo vengono (e sono sempre venuti) dal checkpoint sfx di Stable Audio Small. Restano invariati il resto della decisione (pacchetto pre-generato offline, modulo audio che legge solo asset statici, DEC-109 come destinazione finale).

---

### DEC-173 — Palette ufficiale «Fucina di Worldsmelt»: 31 colori, esplicitamente non-neon

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** il gioco non aveva mai una palette ufficiale documentata: il look attuale è di fatto neon, senza che nessuna decisione lo avesse scelto. Con l'avvio della produzione pixel-art in Aseprite serviva fissare i colori prima di disegnare HUD e sprite originali e prima di rimappare i sprite curati CC0 già in uso.
- **Decisione:** il proprietario ha scelto (28/07, dopo un confronto visivo su 4 proposte con sprite rimappati) la palette custom **«Fucina di Worldsmelt»**: **31 colori** — bronzo, brace, cenere, verderame, con accenti ardesia e prugna — **esplicitamente NON neon**. Vale per **HUD nuovo**, **sprite originali** e **remap batch dei 189 sprite curati CC0**. La fonte operativa per Aseprite resta il file `.gpl` (`~/tools/aseprite-workspace/worldsmelt-fucina.gpl`, fuori dal repository); `content/visual-language.md` ne copia i valori RGB/hex come riferimento di design.
- **Alternative considerate:** **Endesga 32**, **Resurrect 64** e **Apollo** (palette canoniche di terze parti, scartate a favore di una palette custom con identità propria); il mantenimento del look **neon** attuale, esplicitamente non gradito dal proprietario.
- **Conseguenze:** `content/visual-language.md` guadagna la sezione «Palette ufficiale» con i 31 colori, i loro nomi italiani, hex/RGB e il ruolo di ciascuna famiglia (bronzo/metallo, brace/fiamma, cenere/neutri, verderame, ardesia, prugna); la palette vincola anche la variazione per-World già ammessa da DEC-073b, che resta dentro questi 31 colori. Nessun documento approved viene sostituito: prima di questa decisione non esisteva una palette ufficiale canonica, solo un look neon di fatto, mai formalizzato.
- **Documenti aggiornati:** `docs/design/content/visual-language.md` (aggiornato in questo stesso lavoro)

---

### DEC-174 — L'HUD della demo si disegna per il canvas logico attuale 960×640; la domanda aperta 11 resta aperta

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** l'HUD in pixel art della demo va disegnato ora, ma la risoluzione logica canonica dell'interfaccia resta la domanda aperta 11 (proposta ricorrente 640×360 con scaling intero, mai approvata). Serviva sapere se disegnare l'HUD blocca o meno quella scelta, dato che DEC-170 ha già fissato le stanze multi-taglia e la telecamera sul canvas 960×640 in uso oggi.
- **Decisione:** il proprietario ha scelto di **non decidere ora** la risoluzione logica definitiva: la **domanda aperta 11 resta aperta**, si decide dopo la demo. Nel frattempo, l'**HUD in pixel art nasce per il canvas 960×640** in uso oggi, lo stesso su cui sono costruite le stanze multi-taglia e la telecamera (DEC-170). Questa scelta non fissa implicitamente la domanda aperta 11: è il canvas di lavoro corrente, non un valore di design definitivo, stesso trattamento dei default proposti stile DEC-019.
- **Alternative considerate:** fissare subito 640×360 (scartata: avrebbe richiesto rifare canvas/celle/camera prima di poter disegnare l'HUD, bloccando il lavoro in corso); costruire elementi di HUD indipendenti dalla risoluzione come via principale (scartata: resta buona pratica per i componenti a 9-patch, ma non la via principale per non rimandare la disegnazione concreta dell'HUD).
- **Conseguenze:** `ui/hud.md` registra il canvas 960×640 come riferimento della demo, con rimando esplicito a DEC-170 e alla domanda aperta 11; `governance/open-questions.md` aggiunge una nota alla domanda 11 che segnala che resta aperta e che la demo procede a 960×640. Nessun documento approved viene sostituito: DEC-170 resta invariata, questa decisione ne applica il canvas all'HUD.
- **Documenti aggiornati:** `docs/design/ui/hud.md`, `docs/design/governance/open-questions.md` (aggiornati in questo stesso lavoro)

---

### DEC-175 — Pipeline artistica: gli sprite Aseprite sono insieme asset di gioco e dataset LoRA; layer di indirezione contenuto→image-id

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** DEC-148(e) aveva affidato al proprietario del progetto la creazione dei dataset definitivi delle LoRA, senza dire come; nel frattempo la toolchain Aseprite è stata compilata e verificata (27/07: binario locale + server MCP registrato in `.mcp.json`; EULA rispettata, nessuna ridistribuzione del binario), rendendo necessario fissare il ruolo di questa produzione rispetto al catalogo curato (testi/parametri) e alla demo (DEC-171).
- **Decisione:** tre punti. **(a)** Il **lavoro immagini della demo esce dallo scope del catalogo curato** — che si limita a **testi/parametri** con un **auto-mapping provvisorio alle immagini CC0 per tag/categoria** (il meccanismo di DEC-171) — e **passa alla produzione pixel-art con Aseprite**. Il ponte provvisorio DEC-171 resta comunque il fallback attivo finché una categoria non ha ancora uno sprite originale. **(b)** Fra contenuto e immagini esiste un **layer di indirezione contenuto→image-id**: il contenuto non referenzia mai un file immagine direttamente, solo un `id` (il campo `id` del manifest di `17-ASSET-CURATION-AND-FLOOR-ZERO.md`) che si risolve in un file concreto; questo permette di sostituire in blocco le immagini provvisorie con gli sprite definitivi senza toccare i testi. **(c)** Gli **sprite originali** prodotti (HUD, personaggio, nemici, boss, oggetti, colpi, prop, animazioni come spritesheet a **contratto fisso**) sono **anche i dataset definitivi delle LoRA** (DEC-148/168), organizzati per famiglia in `dataset/worldsmelt-style/` (`_general` aggregato per la Style LoRA di base, più `character/enemies/bosses/items/shots/ui/props` per LoRA dedicate), **un file caption per immagine**, registrati nel ledger come **`own`** (non CC0). I **189 sprite curati CC0** rimappati alla palette «Fucina di Worldsmelt» (DEC-173) possono integrare `_general`, con provenienza distinta nel ledger.
- **Alternative considerate:** mantenere il ponte CC0 di DEC-171 come unica via a tempo indeterminato, rimandando sia la produzione artistica reale sia il dataset definitivo; tenere separate la produzione degli asset di gioco e quella del dataset LoRA (doppio sforzo, provenienza divergente fra le due copie della stessa immagine); far referenziare al contenuto i file immagine direttamente (avrebbe impedito la sostituzione in blocco delle immagini provvisorie senza toccare i testi).
- **Conseguenze:** `docs/ai-production/03-PIANO-LORA.md` guadagna la sezione sulla struttura del dataset per famiglie; `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md` registra il contratto spritesheet condiviso e il ruolo di Aseprite; `docs/design/systems/item-fusion.md` collega il suo meccanismo di pesca CC0 al layer di indirezione generale; `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md` registra lo spostamento di scope del lavoro immagini della demo e il campo `id` del manifest come image-id. DEC-148, DEC-168 e DEC-171 restano invariate: questa decisione ne dettaglia l'attuazione, non le sostituisce.
- **Documenti aggiornati:** `docs/ai-production/03-PIANO-LORA.md`, `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md`, `docs/design/systems/item-fusion.md`, `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md` (aggiornati in questo stesso lavoro)

---

### DEC-176 — Stile pixel-art ufficiale «S1 – outline nero» e scala base sprite 24px

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** il gioco non aveva mai uno stile pixel-art ufficiale né una scala base per gli sprite: la domanda aperta 12 chiedeva esplicitamente la scala dei pixel del gioco e se valga la stessa scala per mondo e interfaccia, e DEC-046 lasciava le risoluzioni di riferimento come default di implementazione, non come valore di design. Con la palette «Fucina di Worldsmelt» fissata (DEC-173) e la toolchain Aseprite pronta (DEC-175), il checkpoint CP1 della sessione di produzione pixel-art ha prodotto cinque prove di stile sugli stessi soggetti (goblin, pozione, cuore HUD, cornice slot) — sorgenti in `assets/art-src/style-tests/` (commit `3752eef`) — più un confronto di scala 16/24/32 sul goblin, sottoponendo entrambe le scelte al proprietario.
- **Decisione:** due punti. **(a) Stile:** il proprietario ha scelto lo stile **S1 «outline nero»** fra le cinque prove (S1 outline nero; S2 outline colorato selettivo + cluster shading; S3 no-outline silhouette + rim-light; S4 2-bit alto contrasto; S5 dithering leggero retro): **outline nero 1px** (colore `slag-nero` della palette Fucina) attorno alla silhouette di ogni sprite, **shading piatto a 2 toni per materiale** (tono base + un solo tono di ombra, nessuna banda intermedia), **niente dithering**. Riferimento canonico: `assets/art-src/style-tests/S1-outline-nero.aseprite`, con preview a ingrandimento ×8 in `assets/art-src/style-tests/preview/S1-outline-nero-x8.png`. Dettagli piccoli e ad alto contrasto (occhi, punte luminose, bagliori) possono restare **senza outline interno** quando l'outline li renderebbe illeggibili sotto i 24px: l'outline nero è obbligatorio solo sul perimetro esterno della silhouette. **(b) Scala:** la **scala base è 24px** per personaggi, nemici e oggetti; i **boss** possono superarla; le **icone HUD seguono la propria griglia**, indipendente da questa scala (nessun obbligo che mondo e interfaccia condividano la stessa taglia). Lo stile S1 e la scala 24px valgono per **sprite originali nuovi** e per il **remap batch dei 189 sprite curati CC0**, la stessa copertura già fissata per la palette da DEC-173.
- **Alternative considerate:** per lo stile, gli altri quattro (S2/S3/S4/S5), scartati a favore della leggibilità e semplicità produttiva di S1 su un catalogo di volume; per la scala, **16px** (troppo poco dettaglio per comunicare a colpo d'occhio i 7 strati di trasformazione visiva di `content/visual-language.md`, in particolare silhouette e materiale) e **32px** (dettaglio maggiore ma costo di produzione più alto sul volume richiesto dal catalogo curato e dal dataset LoRA), entrambe scartate a favore del compromesso a 24px.
- **Conseguenze:** `content/visual-language.md` guadagna la sezione «Stile pixel-art ufficiale e scala base sprite» con le regole di outline/shading e la scala, e corregge la riga sulla collocazione del file `.gpl` della palette (ora canonico nel repository, non più «fuori da questo repository» — nota di correzione a DEC-173, che resta approved: cambia solo la collocazione del file, non la scelta dei 31 colori). Il paragrafo di DEC-046 su «risoluzioni di riferimento attuali... default dell'implementazione» resta valido per gli aspetti tecnici non coperti qui (es. rendering a campionamento a punto), ma la **scala base degli sprite pixel-art è ora un valore di design approvato**, non più solo un default proposto. La domanda aperta 12 si **chiude**: scala dei pixel del gioco = 24px base per mondo/personaggio (boss eccettuati), griglia propria per le icone HUD, senza obbligo di scala condivisa fra mondo e interfaccia. La domanda aperta 11 (risoluzione logica dell'interfaccia, 960×640 vs 640×360) **resta aperta**: questa decisione non la tocca, coerente con DEC-174.
- **Documenti aggiornati:** `docs/design/content/visual-language.md`, `docs/design/governance/open-questions.md` (chiusura domanda 12) (aggiornati in questo stesso lavoro)
- **Nota (2026-07-28):** il punto **(b) Scala** è superato da **DEC-177** — la scala base passa da 24px a 32px per allinearsi alla pipeline SD1.5/LoRA. Il punto **(a) Stile S1** resta invariato e approved.

---

### DEC-177 — La scala base sprite passa da 24px a 32px per allinearsi alla pipeline SD1.5/LoRA

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** DEC-176, lo stesso giorno, aveva fissato la scala base a 24px dopo un confronto visivo diretto 16/24/32 al checkpoint CP1. L'analisi tecnica successiva su implementazione e generazione ha verificato l'allineamento della scala con la risoluzione di lavoro di SD1.5 (512px) usata dalla pipeline Style LoRA (DEC-148/168/175): 512/24 = 21,33 (non intero), mentre **512/32 = 16 esatto**. Un rapporto non intero introduce jitter nell'upscale/downscale del Pixel Art Fixer della pipeline; un rapporto intero lo elimina.
- **Decisione:** la **scala base passa a 32px** per personaggi, nemici e oggetti. **Invariati** rispetto a DEC-176: i **boss** possono superarla e le **icone HUD** seguono la propria griglia indipendente; lo **stile S1 «outline nero»** non è rimesso in discussione. A 32px il dettaglio per frame utile alla Style LoRA è **~1,8×** quello a 24px. Il proprietario accetta esplicitamente tre costi: **più tempo di produzione** per sprite (più pixel da disegnare e rivedere sul volume del catalogo curato), **stanze più affollate/dense** a parità di area di cella (più sprite più grandi nello stesso spazio), e un **ritocco futuro dei raggi di collisione** (circa **+2px**) per restare coerenti con la silhouette più grande — non eseguito da questa decisione, registrato come lavoro di implementazione da fare.
- **Alternative considerate:** mantenere **24px** (scartato: rapporto non intero con la risoluzione SD1.5, jitter di upscale/downscale, meno dettaglio per frame per la Style LoRA); **16px** (già scartato da DEC-176 per dettaglio insufficiente a comunicare i 7 strati di trasformazione visiva, motivo che vale ancora); **64px** (rapporto intero altrettanto pulito con 512, 512/64=8, ma dettaglio e costo di produzione/collisione eccessivi per il volume richiesto dal catalogo curato e dal dataset LoRA).
- **Conseguenze:** `content/visual-language.md` aggiorna la sezione «Stile pixel-art ufficiale e scala base sprite» (24px → 32px, con la motivazione di allineamento pipeline) e lo scenario Given/When/Then pertinente; `governance/open-questions.md` aggiorna il valore riportato nella nota di chiusura della domanda 12 (resta chiusa, cambia solo il valore). Il ritocco dei raggi di collisione resta un **gap di implementazione esplicito** (stile DEC-009/DEC-052): il codice dovrà adeguarsi al nuovo ingombro visivo, non ancora applicato in questo lavoro.
- **Documenti aggiornati:** `docs/design/content/visual-language.md`, `docs/design/governance/open-questions.md`, `docs/design/governance/decision-log.md` (nota su DEC-176) (aggiornati in questo stesso lavoro)

---

### DEC-178 — Gli SFX si generano con Stable Audio Small FX; rFXGen esce dalla pipeline audio

- **Data:** 2026-07-28
- **Stato:** approved
- **Contesto:** DEC-109 (22/07) elencava rFXGen come anello di fallback della catena SFX e DEC-172 (27/07) lo citava come motore degli effetti procedurali della demo, ma nella pratica rFXGen non è mai stato installato né usato: il pacchetto SFX della demo è già al 100% generato dal checkpoint `models/stable-audio-3-small-sfx` (vedi `assets/audio/README.md`, che registra esplicitamente «rFXGen non è presente né nel repo né nell'ambiente di produzione»). Il proprietario ha sciolto la riserva il 28/07: «Gli effetti FX sono tutti generati con Stable Audio Small FX, non con rFXGen».
- **Decisione:** il **generatore degli SFX** è il **checkpoint sfx di Stable Audio Small**, per effetti sia semplici sia complessi. **rFXGen è rimosso dalla pipeline audio**: non è più un anello di fallback (DEC-109) né uno strumento di produzione della demo (DEC-172). Il **fallback garantito** resta il **pacchetto audio curato/statico**, già esistente e versionato — catena a **due livelli** (checkpoint sfx → curato) al posto dei tre precedenti (checkpoint → rFXGen → curato). **Nota di contesto onesta**: il primo lotto di SFX generato con la ricetta veloce del benchmark del 23/07 (steps=8, cfg_scale=1.0) è stato **bocciato all'ascolto dal proprietario**; è in corso la rigenerazione con una **ricetta di qualità** (steps≥25, cfg 6-7). Questa decisione riguarda **la pipeline** (quale generatore, quale catena di fallback): **non approva** il lotto SFX specifico già presente in `assets/audio/sfx/`, che resta da rigenerare.
- **Alternative considerate:** installare davvero rFXGen e mantenerlo come secondo anello procedurale reale (scartata: nessuna necessità dimostrata — il checkpoint sfx copre già l'intera produzione — e tenere un anello mai esercitato nella catena rischia di far credere che esista un fallback procedurale disponibile quando il fallback reale è sempre stato il pacchetto curato); lasciare la documentazione invariata trattando le menzioni di rFXGen come imprecisione innocua (scartata: chi legge la pipeline resterebbe convinto dell'esistenza di un secondo generatore mai adottato).
- **Conseguenze:** `content/audio-and-feedback.md` e `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` aggiornano ogni menzione di rFXGen nella catena SFX, nel diagramma di architettura e nei fallback elencati. DEC-109 e DEC-172 restano `approved` ma ricevono ciascuna una nota datata di sostituzione parziale (stesso trattamento già usato per DEC-036/DEC-176): la scelta di Stable Audio Small come via primaria e la garanzia del fallback curato sempre disponibile restano invariate, cambia solo l'anello intermedio rFXGen, ora rimosso. Il lotto SFX attuale in `assets/audio/sfx/` resta segnalato come da rigenerare con la ricetta di qualità: non è oggetto di questa decisione.
- **Documenti aggiornati:** `docs/design/content/audio-and-feedback.md`, `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`, `docs/design/governance/decision-log.md` (note su DEC-109 e DEC-172) (aggiornati in questo stesso lavoro)
