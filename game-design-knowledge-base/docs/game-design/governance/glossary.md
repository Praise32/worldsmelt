# Glossary

## Struttura e stati

- **Piano 0:** hub di caricamento giocabile, sempre disponibile; rifugio sicuro più arene di sfida opzionali con contenuti "best-of" già validati; contiene museo, scelta del tema, scelta del personaggio e indicatore di generazione (DEC-004). Assorbe la vecchia schermata separata di generazione.
- **Floor / Piano:** gruppo di stanze con progressione interna e boss. Una run standard è Piano 0 + 5 piani (DEC-001).
- **Run:** sessione completa dall'avvio alla vittoria (boss del piano 5) o alla sconfitta (permadeath). Una run può proseguire oltre il piano 5 in piani extra non ufficiali (DEC-006).
- **Manifest di run:** descrizione stabile dei contenuti e delle regole di una run (seed compreso), usata per riproducibilità e classifiche.
- **Build:** insieme corrente di oggetti, statistiche e sinergie del giocatore.

## Oggetti e slot

- **Innesto** (placeholder, sostituisce il termine vietato "trinket"): oggetto piccolo, situazionale, sostituibile; 1 slot iniziale, espandibile con oggetti o eventi rari. Rosa di nomi alternativi da valutare quando si sceglierà il nome del gioco: *Scaglia*, *Residuo*, *Sigillo*.
- **Trinket:** termine esterno, non usare. Sostituito ovunque da **Innesto**.
- **Oggetto attivo:** oggetto con azione volontaria e ricarica; 1 slot iniziale, espandibile.
- **Oggetto passivo:** oggetto con effetto continuo; nessun limite di slot.
- **Stat-up:** incremento diretto di una statistica; nessun limite di slot.
- **Fusione:** meccanica-firma del gioco (DEC-012). Nella stanza di fusione il giocatore consuma due oggetti e ottiene un oggetto nuovo generato dall'IA che eredita comportamento e presentazione da entrambi.
- **Sinergia (implicita):** interazione automatica tra due o più componenti compatibili della build, senza consumo di oggetti; distinta dalla fusione esplicita.

## Risorse

- **Salute stratificata:** salute base più salute temporanea/protettiva, visivamente distinguibili; ordine di consumo: prima la temporanea, poi la base (DEC-008).
- **Valuta principale:** risorsa spendibile per acquisti in run (nome definitivo da assegnare).
- **Strumento di breccia:** risorsa consumabile con funzione equivalente alle "bombe" (nome definitivo da assegnare).
- **Strumento di apertura:** risorsa consumabile con funzione equivalente alle "chiavi" (nome definitivo da assegnare).
- **Catalizzatore di fusione:** risorsa che abilita e paga la fusione esplicita (DEC-012, DEC-013).

## Nemici

- **Veterano** (placeholder, sostituisce "élite"): nemico potenziato non-boss.

## Generazione e validazione

- **Contenuto curato:** contenuto creato e approvato manualmente.
- **Contenuto generato:** contenuto prodotto o composto dall'IA locale.
- **Origine del contenuto:** tassonomia unica a 4 valori usata dai template con il campo `origin:` — `curato | composto | variato | nuovo`. Sostituisce ogni altra classificazione informale dell'origine.
- **Fallback:** contenuto sicuro usato quando una proposta generata non è disponibile o valida. Fonte unica delle regole di fallback: `systems/generated-content-validation.md`.
- **Correzione di fortuna** (sostituisce il termine informale "pity"): garanzia che, dopo N estrazioni sfortunate, la qualità minima del contenuto offerto sale.
- **Stati di validazione del contenuto generato** (6, in italiano): *proposto*, *strutturalmente-valido*, *simulato*, *approvato-per-run*, *respinto*, *fallback-usato*. Sostituiscono gli aggettivi informali "validato" e "fortemente validato". Fonte dei controlli: `systems/generated-content-validation.md`.

## Pool ed economia degli oggetti

- **Pool:** insieme pesato di contenuti candidati per un contesto.
- **Peso:** valore relativo che determina la probabilità di un contenuto all'interno di un pool.
- **Rarità:** classe di frequenza, non necessariamente sinonimo di potenza.
- **Budget:** quantità massima spendibile di un attributo entro cui la generazione o la composizione di un contenuto deve restare. Varianti in uso: *budget di potenza*, *budget di pericolo*, *budget di novità*, *budget di leggibilità*, *budget di difficoltà della stanza*.
- **Stacking:** effetto di più istanze dello stesso componente (oggetto, effetto, sinergia) che si sommano o si combinano secondo una regola dichiarata.
- **Incompatibilità** (termine unico: "esclusioni" non si usa più): relazione dichiarata tra due componenti che impedisce la loro convivenza nella stessa build o pool.
- **Tag:** proprietà semantica usata per regole, generazione e presentazione.

## Presentazione e leggibilità

- **Telegraph:** segnale anticipatorio di un attacco o evento.
- **Leggibilità:** vincolo di chiarezza visiva delle minacce e degli effetti attivi; fonte unica del "budget di leggibilità": `systems/combat-and-projectiles.md`.
