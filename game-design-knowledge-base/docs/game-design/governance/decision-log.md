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
- **Stato:** approved
- **Contesto:** "Melting Run" è un titolo di lavoro, non il nome definitivo del progetto.
- **Decisione:** Il nome resta provvisorio; il nome definitivo lo sceglierà il proprietario (domanda aperta); il vocabolario può comunque appoggiarsi al campo semantico fusione/scioglimento.
- **Alternative considerate:** Fissare subito un nome definitivo.
- **Conseguenze:** I nomi placeholder di risorse, Innesto e Veterano dipendono da questa scelta futura.
- **Documenti aggiornati:** `governance/open-questions.md`, `governance/glossary.md`
- **Nota (2026-07-18): risolta da DEC-071.** Il nome definitivo del gioco è "Worldsmelt"; "Melting Run" resta il nome storico del repository e il titolo di lavoro citato in questo registro. La domanda aperta sul nome è chiusa.

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
- **Stato:** draft
- **Contesto:** Il codice contiene già pesi e bande numeriche non ancora validate col playtest.
- **Decisione:** Pesi rarità {55, 30, 12, 3}, pesi boss {0, 0, 70, 30}, bande di potenza colpi [0.75-1.25] / nemici [0.7-1.35] / boss [1.4-3.2], 4 rarità (comune, non-comune, rara, leggendaria) sono i default attuali dell'implementazione. Vanno documentati come "default proposto", non come decisione presa, e restano da validare col playtest.
- **Alternative considerate:** Fissarli subito come valori definitivi.
- **Conseguenze:** `governance/open-questions.md` registra la validazione di questi valori come domanda aperta.
- **Documenti aggiornati:** `systems/items-pools-and-rarity.md`, `systems/bosses.md`, `systems/combat-and-projectiles.md`, `governance/open-questions.md`

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

