---
id: ref-sintesi-strategica
title: Worldsmelt — Sintesi strategica
domain: design
status: proposed
authority: supporting
owner: design
summary: >-
  Analisi esterna/consulenziale su rischi, posizionamento, vertical slice e priorita' operative del progetto; non ancora discussa/approvata.
last_reviewed: 2026-07-19
topics: [posizionamento, vertical-slice, mechanics-lab, rischi, roadmap, marketing]
related: []
supersedes: []
source_files: []
---

# Worldsmelt — Sintesi strategica

## Verdetto

**Worldsmelt ha una possibilità reale di successo come indie di nicchia o cult game**, soprattutto se viene presentato come un action roguelite con una meccanica di fusione generativa, non come un gioco “interamente creato dall’AI”.

La tecnologia è interessante e l’architettura appare più matura di un semplice prototipo. Il rischio principale, però, non è tecnico: il progetto deve prima di tutto essere un ottimo gioco.

> **La proposta più forte:** un action roguelite offline in cui il giocatore fonde due reliquie e il gioco forgia una nuova regola di combattimento, completa di comportamento, identità e codice condivisibile.

---

## Il cuore del progetto

Worldsmelt combina:

1. gameplay action roguelite tradizionale;
2. generazione AI locale;
3. meccaniche e comportamenti descritti tramite Lua;
4. validazione, simulazione e fallback;
5. catalogo persistente delle creazioni;
6. RunBundle esportabili e condivisibili.

La caratteristica davvero distintiva è la **fusione degli oggetti**:

- il giocatore sceglie due oggetti;
- la fusione produce una nuova meccanica;
- il risultato modifica concretamente il combattimento;
- il contenuto viene validato prima dell’uso;
- la creazione può essere conservata e condivisa.

L’AI non deve sostituire il game design: deve trasformare una scelta del giocatore in una conseguenza sorprendente ma controllata.

---

## Punti di forza

### 1. AI locale e offline

L’esecuzione locale offre:

- assenza di costi cloud per ogni partita;
- funzionamento offline;
- maggiore privacy;
- indipendenza da servizi esterni;
- possibilità di vendere il gioco senza costi ricorrenti di inferenza.

### 2. Architettura prudente

Il motore mantiene il controllo di:

- collisioni;
- danni;
- economia;
- navigazione;
- limiti numerici;
- stato della run;
- budget di esecuzione;
- caricamento e fallback.

L’AI lavora entro un perimetro ristretto. Questa separazione è corretta e vicina alla direzione seguita dall’industria nei sistemi di AI integrati nei videogiochi.

### 3. Validazione e fallback

La pipeline proposta — compilazione, dry-run, simulazione, approvazione o fallback — è uno degli elementi più maturi del progetto.

Worldsmelt non presume che il modello generi sempre contenuti validi: tratta il fallimento come una condizione normale e recuperabile.

### 4. RunBundle e provenienza

Seed, versioni, hash e contenuti esportabili possono diventare:

- codici di condivisione;
- daily challenge;
- replay verificabili;
- strumenti di debugging;
- contenuti per streamer;
- base per classifiche e competizioni asincrone;
- dataset per migliorare il sistema.

### 5. Posizionamento originale

La maggior parte dei progetti AI nel gaming si concentra su NPC conversazionali, companion vocali e narrativa.

Worldsmelt esplora invece la **generazione controllata di regole di combattimento**. È una nicchia più rischiosa, ma anche più distintiva.

---

## Rischi principali

### 1. Il gioco potrebbe non essere abbastanza divertente

L’AI non può compensare:

- movimento poco preciso;
- combattimento senza impatto;
- boss deboli;
- scarsa leggibilità;
- stanze ripetitive;
- progressione piatta;
- interfaccia confusa.

Il gioco deve essere divertente anche con un solo tema, pochi nemici e nessuna generazione visiva.

### 2. Tempi di generazione troppo lunghi

Tempi nell’ordine di decine di secondi o minuti sono accettabili per un prototipo, ma difficili da sostenere nel prodotto finale.

Possibili mitigazioni:

- cache;
- generazione anticipata;
- pool di contenuti già validati;
- preparazione tra un piano e l’altro;
- generazione della meccanica prima della grafica;
- riutilizzo di animazioni e body plan;
- Floor Zero come attività giocabile durante la preparazione.

### 3. Frammentazione hardware

Non tutti i giocatori dispongono di abbastanza VRAM per modelli e grafica contemporaneamente.

Occorre testare almeno:

- 4 GB VRAM;
- 6 GB;
- 8 GB;
- 12 GB;
- GPU integrate;
- modalità CPU;
- Steam Deck;
- GPU NVIDIA, AMD e Intel;
- Windows e Linux/Proton.

### 4. Scope eccessivo

Il progetto comprende contemporaneamente:

- gioco;
- runtime AI;
- pipeline grafica;
- sistema di animazione;
- training;
- catalogo;
- multiplayer;
- classifiche;
- supporto multipiattaforma;
- audio generativo.

Per un piccolo team è troppo.

Da rinviare:

- multiplayer sincrono;
- audio generativo;
- runtime proprietario;
- console;
- animazioni interamente generate;
- personaggi completamente generati;
- training esteso prima della validazione del gameplay.

### 5. Identità visiva debole

La correttezza tecnica degli sprite non basta. Gli screenshot devono apparire intenzionali, coerenti e immediatamente riconoscibili.

La generazione dovrebbe produrre variazioni dentro una direzione artistica curata, non sostituire la direzione artistica.

### 6. Comunicazione centrata troppo sull’AI

La frase “tutto generato dall’AI” rischia di attirare diffidenza e di posizionare il progetto tra prodotti percepiti come poco curati.

Il messaggio dovrebbe partire dalla fantasia del giocatore e dalla fusione, non dalla tecnologia.

---

## Posizionamento consigliato

### Formula consigliata

> **Un action roguelite offline in cui fondi due reliquie per forgiare nuove regole di combattimento, che puoi collezionare e condividere.**

### Da evitare

> Un roguelite interamente generato dall’intelligenza artificiale.

### Per publisher e investitori

La demo deve mostrare rapidamente:

1. due oggetti comprensibili;
2. la scelta della fusione;
3. la nuova reliquia;
4. un cambiamento evidente del combattimento;
5. un nemico o boss che interagisce con la nuova regola;
6. la registrazione nel catalogo;
7. l’esportazione o condivisione del RunBundle;
8. un fallback senza interruzione del gioco.

---

## Vertical slice consigliata

La prima versione dimostrativa dovrebbe contenere:

- Floor Zero essenziale;
- due piani;
- un personaggio;
- un solo tema visivo molto curato;
- sei nemici;
- un boss;
- circa venti oggetti base;
- una stanza di fusione;
- otto-dodici famiglie di comportamento;
- massimo due fusioni per run;
- catalogo minimo;
- importazione ed esportazione RunBundle;
- arte principale curata;
- generazione grafica limitata e controllata.

La domanda da validare è:

> **Le fusioni sono abbastanza sorprendenti, leggibili e divertenti da far desiderare un’altra run?**

---

## Mechanics Lab

Prima di ampliare il gioco conviene costruire un’arena di test dedicata alle meccaniche generate.

Per ogni fusione sarebbe utile registrare:

- compilazione;
- validazione;
- risultato osservato;
- leggibilità;
- novità;
- potenza;
- bug;
- tempo di generazione;
- numero di retry;
- valutazione umana del divertimento.

Il vero vantaggio competitivo non sarà il modello utilizzato, ma il dataset di meccaniche generate, testate e valutate.

---

## Priorità operative

### Priorità immediata

1. rendere il combattimento divertente senza AI;
2. completare il Mechanics Lab;
3. ridurre o nascondere i tempi di generazione;
4. definire una direzione artistica forte;
5. rendere la fusione comprensibile in pochi secondi;
6. consolidare il RunBundle;
7. testare il gioco con persone esterne.

### Documentazione da mantenere

Creare tre fonti autorevoli:

- `CURRENT_PRODUCT.md` — che cosa è oggi il gioco;
- `CURRENT_TECH.md` — che cosa funziona realmente;
- `CURRENT_LIMITS.md` — limiti, problemi e hardware non verificato.

Ogni altra proposta dovrebbe essere marcata come:

- ipotesi;
- approvata;
- implementata;
- misurata;
- superata;
- abbandonata.

---

## Criteri di successo della vertical slice

Il progetto è promettente se, durante test ciechi con giocatori esterni:

- il funzionamento della fusione viene compreso senza spiegazioni lunghe;
- la maggioranza desidera iniziare una seconda run;
- le creazioni vengono ricordate e raccontate;
- il fallback è raro o quasi invisibile;
- nessuno rimane bloccato ad attendere la generazione;
- il combattimento è apprezzato anche senza considerare l’AI;
- alcuni giocatori condividono spontaneamente bundle o screenshot;
- l’AI viene percepita come parte del gioco, non come espediente promozionale.

---

## Vantaggio competitivo reale

I modelli open source, Stable Diffusion e i runtime di inferenza sono accessibili anche ad altri sviluppatori.

Il vantaggio difendibile di Worldsmelt è la combinazione di:

- grammatica delle meccaniche;
- API Lua sicura;
- simulatore;
- validatore;
- corpus di fusioni;
- classificazione del divertimento;
- normalizzazione;
- fallback;
- RunBundle;
- catalogo;
- dati raccolti dalle scelte dei giocatori;
- identità creativa del gioco.

> **Worldsmelt non deve costruire il modello migliore. Deve costruire la migliore fonderia capace di trasformare output imperfetti in meccaniche giocabili.**

---

## Conclusione

Il progetto merita di continuare, ma con uno scope molto più concentrato.

La strada più credibile è:

- meno nuove pipeline;
- più combat feel;
- più test con giocatori reali;
- più qualità visiva;
- più misurazione delle fusioni;
- meno dipendenza da una singola piattaforma hardware;
- RunBundle canonici invece di un unico modello obbligatorio;
- AI presentata come strumento della meccanica, non come sostituto del gioco.

Worldsmelt potrebbe fallire come roguelite generico riempito di contenuti AI.

Può invece riuscire come **roguelite di fusione sistemica, offline e condivisibile, nel quale l’AI è il crogiolo ma il game designer conserva il martello**.
