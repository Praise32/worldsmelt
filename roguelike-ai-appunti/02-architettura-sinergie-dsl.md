# 02 — Architettura, sinergie e DSL

## Raccomandazione principale

La DSL non deve essere un elenco di oggetti predefiniti. Deve essere un’algebra di effetti: pochi concetti generali, tipizzati e componibili. È inevitabile che imponga confini, ma quei confini sono ciò che rende possibile validare, bilanciare, replicare e spedire il gioco.

La libertà utile non è “eseguire qualunque programma”. È poter collegare in modi nuovi:

- eventi;
- condizioni;
- selettori di entità;
- formule limitate;
- geometrie;
- moto;
- collisioni;
- payload;
- stati;
- costi e rischi.

## Moduli concettuali

~~~mermaid
flowchart LR
    A["Telemetria e seed"] --> B["RunBible"]
    B --> C["Qwen: candidati JSON"]
    C --> D["Schema e type checker"]
    D --> E["Effect Graph"]
    E --> F["Simulatore headless"]
    F --> G["Bilanciamento e novelty check"]
    G --> H["Appearance Contributions"]
    H --> I["SD e libreria componenti"]
    I --> J["RunBundle congelato"]
    J --> K["Engine a 60 Hz"]
~~~

### RunBible

È il contratto creativo del piano:

- tema;
- palette;
- tono;
- famiglie di forme;
- meccaniche favorite e vietate;
- pool;
- curva di difficoltà;
- budget di proiettili e spawn;
- profilo del giocatore usato;
- versione delle capacità disponibili.

Impedisce che ogni oggetto sembri provenire da un gioco diverso.

### RunBundle

È il contenuto generato già congelato:

- RunBible;
- mappe e stanze;
- definizioni normalizzate degli oggetti;
- Effect Graph compilati;
- definizioni di nemici e boss consentite;
- sprite, atlas e metadati;
- seed separati per sistema;
- versioni di schema, bilanciamento, prompt e modello;
- hash degli asset;
- hash canonico del bundle.

Il modello crea il bundle; durante il gioco il bundle è l’autorità.

### Effect Graph

Una pipeline minima per un proiettile può essere pensata così:

    Emitter
      -> Pattern
      -> Projectile Body
      -> Motion
      -> Collision
      -> Payload
      -> Event Reactions
      -> Lifetime

Due oggetti non devono conoscere direttamente l’uno l’altro. Aggiungono o trasformano nodi della stessa struttura.

## Esempio: palla rimbalzante più spine

Oggetto A:

    mechanics:
      add_motion: gravity_arc
      add_collision_response: floor_bounce
      max_bounces: 3
    appearance:
      projectile_shape: sphere
      player_morph:
        roundness: 0.25

Oggetto B:

    mechanics:
      add_payload: piercing
      add_surface: spikes
    appearance:
      projectile_surface: spikes
      player_attachment:
        role: body_spikes

Il compilatore non chiede a SD di “immaginare la sinergia” al momento del pickup. Produce:

- corpo sferico;
- superficie con spine;
- traiettoria ad arco;
- rimbalzo;
- penetrazione;
- contributo visivo rotondo e spinoso sul personaggio.

Se occorre un nuovo asset, viene generato prima del piano o sostituito da un componente certificato.

## DSL a tre livelli

### Livello 1 — Modificatori dichiarativi

Per effetti semplici:

- danno, cadenza e velocità;
- dimensione e colore;
- numero di proiettili;
- rimbalzi e penetrazione;
- durata;
- probabilità con limite;
- costo in vita, valuta o risorsa.

Limite iniziale consigliato: 8 nodi.

### Livello 2 — Typed Effect Graph

È la fonte principale di novità. Include:

- OnFire, OnHit, OnBounce, OnKill, OnRoomEnter e timer;
- condizioni e confronti tipizzati;
- selettori spaziali;
- trasformazioni vettoriali;
- spawn con quote;
- status e payload;
- esecuzione in sequenza o parallelo;
- formule con clamp;
- adapter tra tipi compatibili.

Limite iniziale consigliato: 24 nodi e profondità evento massima 4.

### Livello 3 — Bounded State Machine

Per oggetti signature e leggi del piano:

- massimo quattro variabili locali tipizzate;
- timer e contatori;
- stati;
- transizioni su eventi;
- nessun loop arbitrario;
- nessuna ricorsione;
- massimo 48 nodi;
- massimo otto entità figlie per evento;
- almeno un tick di cooldown per una catena che può riattivarsi.

Esempio:

> Dopo tre rimbalzi il proiettile diventa una mina. Quando un nemico entra nel raggio, la mina emette spine e torna allo stato iniziale.

## La DSL limita la creatività?

Sì, come qualunque motore. La risposta corretta è renderla evolutiva.

### Percorso di espansione

1. Qwen prova a comporre con il Capability Registry corrente.
2. Se una proposta non è rappresentabile emette una Capability Request strutturata.
3. Il gioco usa un fallback sicuro e conserva la richiesta.
4. Le richieste ricorrenti vengono raggruppate.
5. Lo sviluppatore implementa una nuova primitiva generale.
6. La nuova capacità entra in una versione successiva dello schema.

In questo modo l’AI aiuta a scoprire quali primitive mancano, senza poter scavalcare il motore.

### Ruolo futuro di Lua

Il piano originale attribuisce molto valore a Lua. Lua resta utile, ma la raccomandazione è:

- vertical slice: solo DSL tipizzata;
- laboratorio interno: Qwen può tradurre una proposta in Lua dentro una sandbox e un simulatore;
- produzione successiva: un pattern Lua ripetutamente valido diventa una primitiva ufficiale;
- eventuali “anomalie” Lua runtime solo dopo sicurezza, determinismo, replay e fuzzing.

Questo evita che la prima prova di gameplay diventi prima di tutto un progetto di sicurezza.

## Regole di fusione

Il compilatore deve avere precedenze esplicite:

1. regole fondamentali del motore;
2. trasformazioni signature;
3. modificatori di forma e traiettoria;
4. payload e status;
5. moltiplicatori numerici;
6. cosmetica.

Quando due effetti confliggono, non se ne cancella uno in silenzio. Il risultato deve essere uno fra:

- composizione;
- trasformazione con adapter;
- dominanza dichiarata;
- alternativa selezionata e descritta;
- rifiuto del candidato.

Ogni decisione produce un Contribution Trace consultabile nel tooltip di debug.

## Guardrail di runtime

- budget globale di nodi eseguiti per tick;
- cap separati per proiettili, nemici e particelle;
- clamp su velocità, danno, scala e durata;
- divieto di NaN e infinito;
- pool di entità preallocati;
- nessun caricamento di modello dentro il loop;
- nessuna allocazione incontrollata da un nodo;
- fallback deterministico quando un riferimento è mancante;
- grafica indipendente da hitbox e statistiche.

## Anti-ripetizione

Un Novelty Ledger conserva:

- tag meccanici;
- topologia normalizzata del grafo;
- parametri quantizzati;
- descrizione compilata;
- esito e valutazione umana.

Un candidato troppo simile alle ultime run può essere mutato o scartato. La similarità non deve però premiare la stranezza fine a se stessa: un oggetto nuovo ma illeggibile non è migliore di uno familiare e ben combinato.

## Test minimi sui 20 oggetti

- tutte le 190 coppie;
- almeno 1.000 triple campionate;
- 100 seed per gli oggetti con probabilità;
- test di terminazione per ogni macchina a stati;
- stress test al cap di proiettili;
- test che ogni effetto dichiarato produca un evento osservabile;
- test che il tooltip corrisponda al grafo normalizzato;
- test che il tratto visivo mantenga una Contribution Trace;
- golden bundle rigiocabili dopo ogni modifica di schema.

