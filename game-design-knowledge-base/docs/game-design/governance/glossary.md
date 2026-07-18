# Glossary

## Struttura e stati

- **Piano 0** (in-game: **The Crucible**, DEC-072): hub di caricamento giocabile, sempre disponibile; rifugio sicuro più arene di sfida opzionali con contenuti "best-of" già validati; contiene museo, scelta del tema, scelta del personaggio e indicatore di generazione (DEC-004). Assorbe la vecchia schermata separata di generazione.
- **Floor / Piano:** gruppo di stanze con progressione interna e boss. Una run standard è Piano 0 + 5 piani (DEC-001).
- **Tema della run** (in-game: **World**, DEC-072): identità tematica scelta dal giocatore tra 2-3 proposte generate dall'IA nel Piano 0; evolve/degenera piano dopo piano fino al boss del piano 5 (DEC-005). Fonte unica: `systems/floor-zero.md`.
- **Run:** sessione completa dall'avvio alla vittoria (boss del piano 5) o alla sconfitta (permadeath). La vittoria al boss del piano 5 chiude la run (DEC-006, DEC-031); una prosecuzione oltre il piano 5 resta solo un'idea futura non implementata (DEC-018).
- **Manifest di run:** descrizione stabile dei contenuti e delle regole di una run (seed compreso), usata per riproducibilità e classifiche.
- **Codice run:** codice breve testuale (seed più versione di gioco) che permette di condividere e rigenerare localmente una run fuori dalle classifiche; alternativa più leggera al file RunBundle esportato per la condivisione di una run (DEC-066). Fonte unica: `08-multiplayer-and-competition.md`.
- **RunBundle:** file esportato di una run, con verifica d'integrità già esistente nel progetto; via completa e verificabile per condividere o archiviare una run fuori dalle classifiche (DEC-066), adatta a gare private e archivio. Fonte unica: `08-multiplayer-and-competition.md`.
- **Build:** insieme corrente di oggetti, statistiche e sinergie del giocatore.

## Cornice narrativa

- **Crogiolo (cornice)** (in-game: **The Crucible**, stesso nome in-game della voce Piano 0
  sopra, DEC-072) (DEC-067): cornice narrativa minima canonica del gioco. Il Piano 0 è
  un luogo-fucina fuori dal tempo dove i mondi generati nascono, si fondono e si sciolgono; il
  giocatore è chi vi si immerge per esplorarli prima che collassino; il museo è la memoria di
  ciò che ha salvato. Dà senso a hub, museo, temi e fusione senza imporre lore fissa ai temi
  generati, che restano liberi. Fonte unica: `content/narrative-tone.md`.

## Oggetti e slot

- **Innesto** (in-game: **Graft**, DEC-072; sostituisce il termine vietato "trinket"): oggetto piccolo, situazionale, sostituibile; 1 slot iniziale, espandibile con oggetti o eventi rari.
- **Trinket:** termine esterno, non usare. Sostituito ovunque da **Innesto**.
- **Oggetto attivo:** oggetto con azione volontaria e ricarica; 1 slot iniziale, espandibile.
- **Oggetto passivo:** oggetto con effetto continuo; nessun limite di slot.
- **Stat-up:** incremento diretto di una statistica; nessun limite di slot.
- **Fusione** (in-game: **Smelting**, DEC-072): meccanica-firma del gioco (DEC-012). Nella stanza di fusione il giocatore consuma due oggetti e ottiene un oggetto nuovo generato dall'IA che eredita comportamento e presentazione da entrambi.
- **Stanza di fusione** (in-game: **Smeltery**, DEC-072): archetipo speciale di stanza dove si pratica la fusione esplicita (DEC-010, DEC-012). Fonte unica: `systems/item-fusion.md`, `systems/special-rooms.md`.
- **Sinergia (implicita):** interazione automatica tra due o più componenti compatibili della build, senza consumo di oggetti; distinta dalla fusione esplicita.

## Risorse

- **Salute stratificata:** salute base più salute temporanea/protettiva (in-game: **Crust** per la sola componente temporanea/protettiva, DEC-072), visivamente distinguibili; ordine di consumo: prima la temporanea, poi la base (DEC-008).
- **Valuta principale** (in-game: **Ingots**, DEC-072): risorsa spendibile per acquisti in run.
- **Strumento di breccia** (in-game: **Blast Charges**, DEC-072): risorsa consumabile con funzione equivalente alle "bombe".
- **Strumento di apertura** (in-game: **Cast Keys**, DEC-072): risorsa consumabile con funzione equivalente alle "chiavi".
- **Catalizzatore di fusione** (in-game: **Flux**, DEC-072): risorsa che abilita e paga la fusione esplicita (DEC-012, DEC-013).
- **Punti sblocco (meta)** (in-game: **Embers**, DEC-072): risorsa meta, fuori dalla run, guadagnata in singleplayer a fine run e spendibile nel Catalogo per sbloccare contenuti generati nei pool delle run future (DEC-015, DEC-027). Fonte unica: `systems/save-and-meta-progression.md`.
- **Energia** (in-game: **Heat**, DEC-072): risorsa droppata dai nemici sconfitti che ricarica gli oggetti attivi; è uno dei due canali di ricarica di base degli attivi (l'altro è il completamento delle stanze), estensibile da oggetti che aggiungono ulteriori modi di ricarica (DEC-059). Fonte unica: `systems/active-items.md`.

## Nemici

- **Veterano** (in-game: **Tempered**, DEC-072; sostituisce "élite"): nemico potenziato non-boss.

## Personaggi e colpi

- **Colpo firmato** (in-game: **Signature Shot**, DEC-072) (DEC-068): tipo di colpo generato (forma più comportamento) che il
  personaggio alternativo generato per la run può avere, a volte, come possibilità del
  generatore; è parte del budget del personaggio — chi lo riceve ha statistiche più caute. I
  personaggi della rosa base usano sempre colpi standard curati, mai un colpo firmato. Fonte
  unica: `systems/characters.md`.

## Generazione e validazione

- **Contenuto curato:** contenuto creato e approvato manualmente.
- **Contenuto generato:** contenuto prodotto o composto dall'IA locale.
- **Origine del contenuto:** tassonomia unica a 4 valori usata dai template con il campo `origin:` — `curato | composto | variato | nuovo`. Sostituisce ogni altra classificazione informale dell'origine.
- **Fallback:** contenuto sicuro usato quando una proposta generata non è disponibile o valida. Fonte unica delle regole di fallback: `systems/generated-content-validation.md`.
- **Correzione di fortuna** (sostituisce il termine informale "pity"): garanzia che, dopo N estrazioni sfortunate, la qualità minima del contenuto offerto sale.
- **Stati di validazione del contenuto generato** (6, in italiano): *proposto*, *strutturalmente-valido*, *simulato*, *approvato-per-run*, *respinto*, *fallback-usato*. Sostituiscono gli aggettivi informali "validato" e "fortemente validato". Fonte dei controlli: `systems/generated-content-validation.md`.
- **Reliquie** (in-game: **Relics**, DEC-072) (DEC-069): sezione del Catalogo che raccoglie i contenuti che non superano la
  riconvalida a un aggiornamento del gioco: scheda consultabile, ma non più giocabile né
  sbloccabile nei pool. La memoria del giocatore non si perde mai. Fonte unica:
  `systems/save-and-meta-progression.md`.
- **Solo curato** (modalità, DEC-070): modalità di gioco legittima e permanente, scelta al
  primo avvio (o attivabile in seguito), in cui nessun modello IA è attivo: si gioca con
  contenuti curati e fallback procedurale. Distinta dal fallback per singolo contenuto. Fonte
  unica: `06-ai-content-generation-model.md` (regola), `systems/floor-zero.md` (scelta al
  primo avvio).

## Pool ed economia degli oggetti

- **Pool:** insieme pesato di contenuti candidati per un contesto.
- **Peso:** valore relativo che determina la probabilità di un contenuto all'interno di un pool.
- **Rarità:** classe di frequenza, non necessariamente sinonimo di potenza.
- **Budget:** quantità massima spendibile di un attributo entro cui la generazione o la composizione di un contenuto deve restare. Varianti in uso: *budget di potenza*, *budget di pericolo*, *budget di novità*, *budget di leggibilità*, *budget di difficoltà della stanza*.
- **Stacking:** effetto di più istanze dello stesso componente (oggetto, effetto, sinergia) che si sommano o si combinano secondo una regola dichiarata.
- **Incompatibilità** (termine unico: "esclusioni" non si usa più): relazione dichiarata tra due componenti che impedisce la loro convivenza nella stessa build o pool.
- **Tag:** proprietà semantica usata per regole, generazione e presentazione.

## Multiplayer e classifiche

- **Daily (Classificata giornaliera pubblica)** (in-game: **Daily Smelt**, DEC-072): una delle tre istanze della modalità Classificata (DEC-021, DEC-062); una run generata scelta dallo sviluppatore, con lo stesso seed per tutti i giocatori, che cambia ogni giorno; ha una classifica globale giornaliera, divisa per metrica (tempo e punteggio separati); premia con medaglie/cornici cosmetiche legate a piazzamento e streak di partecipazione, fuori dall'economia dei punti sblocco (DEC-064). Fonte unica: `08-multiplayer-and-competition.md`.

## Presentazione e leggibilità

- **Telegraph:** segnale anticipatorio di un attacco o evento.
- **Leggibilità:** vincolo di chiarezza visiva delle minacce e degli effetti attivi; fonte unica del "budget di leggibilità": `systems/combat-and-projectiles.md`.
- **Card di scoperta (breve)** (in-game: **Discovery Card**, DEC-072): annuncio non bloccante nell'HUD alla prima occorrenza di un contenuto generato mai visto (oggetto, nemico, boss, sinergia/fusione); mostra sprite, nome e una riga; non mette in pausa né blocca l'input; una sola card alla volta, le altre si accodano (DEC-065). Fonte unica: `ui/hud.md`.
