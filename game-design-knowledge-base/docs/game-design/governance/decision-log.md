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

### DEC-014 — Personaggio base più personaggio alternativo generato

- **Data:** 2026-07-17
- **Stato:** approved
- **Contesto:** Serve varietà di run senza introdurre potenziamenti permanenti del personaggio.
- **Decisione:** Esiste un personaggio base sempre disponibile; ogni run l'IA genera un personaggio alternativo (trait unico più statistiche casuali entro bande): prendere o lasciare, si sceglie nel Piano 0.
- **Alternative considerate:** Roster fisso di personaggi sbloccabili in modo permanente.
- **Conseguenze:** La scelta del personaggio si sposta da Run Setup al Piano 0.
- **Documenti aggiornati:** `systems/floor-zero.md`, `ui/run-setup.md`

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
