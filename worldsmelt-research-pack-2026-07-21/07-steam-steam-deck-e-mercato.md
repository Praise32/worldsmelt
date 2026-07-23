# Steam, Steam Deck, compatibilità e mercato

## Il PC di sviluppo come benchmark

Il Ryzen 5 3600 e 16 GB di RAM sono ancora rappresentativi di molti desktop da gioco. La RX 5600 XT da 6 GB è utile come **target desktop AI di riferimento**.

Tuttavia, il fatto che una pipeline funzioni su questa macchina non dimostra automaticamente:

- compatibilità con GPU integrate;
- Windows e driver differenti;
- NVIDIA/Intel;
- portatili con memoria condivisa;
- Steam Deck;
- coesistenza tra rendering e inferenza;
- tempo di una run completa, non di un singolo manifest.

Prima di pubblicare requisiti minimi occorre una matrice esterna.

## Cosa dicono i dati Steam

Nel sondaggio Steam consultato durante l’analisi:

- 16 GB è una configurazione RAM molto comune;
- 6 e 8 core sono categorie CPU dominanti;
- 8, 12 e 16 GB di VRAM hanno quote importanti;
- esiste comunque una parte significativa di utenti con 4–6 GB o grafica integrata;
- 1080p resta la risoluzione principale, con 1440p in crescita.

Le categorie RAM e VRAM non sono una distribuzione congiunta. Non è corretto moltiplicarle e dichiarare una percentuale precisa di utenti compatibili.

## Steam Deck

Specifiche rilevanti:

- CPU Zen 2, 4 core/8 thread;
- GPU RDNA2 integrata, 8 CU;
- APU 4–15 W;
- 16 GB di memoria condivisa;
- 1280×800.

### Valutazione

- Il gioco C 2D può essere ottimizzato per Deck.
- Un 7B Q4 può forse entrare in memoria, ma la latenza della generazione completa non è dimostrata.
- Un 4B Q4 è molto più plausibile.
- LLM e diffusione devono essere sequenziali.
- Nessuna inferenza pesante durante il combattimento.
- Il modello deve essere provato su un Deck reale prima di dichiarare supporto.

### Obiettivo Deck

```text
1280×800
controller completo
30 FPS minimi, 60 target
nessun launcher obbligatorio
UI leggibile
modello caricato solo nella fase di generazione
RunBundle pronto prima del gameplay
```

## Distribuzione Steam con modelli inclusi

La build deve contenere il modello canonico se la generazione locale è la caratteristica principale. Possibile struttura depot:

```text
common depot     = dati comuni, modello GGUF, prompt, grammar, licenze
Windows depot    = eseguibile e runtime Windows
Linux depot      = eseguibile e runtime Linux/SteamOS
```

Non usare pacchetti qualitativi differenti. Eventuali depot possono separare piattaforme, non qualità.

Il gioco deve:

- non scaricare pesi da Hugging Face al primo avvio;
- verificare hash e integrità;
- mostrare spazio richiesto chiaramente;
- mantenere cache e RunBundle in directory utente;
- avviarsi con un bundle di emergenza se la generazione fallisce;
- registrare log diagnostici senza dati personali.

## AI disclosure su Steam

Gli asset generati durante lo sviluppo rientrano nella dichiarazione di AI pre-generata. Il codice, testo, immagini o audio prodotti mentre il gioco è in esecuzione rientrano nella categoria live-generated AI.

La descrizione per Valve dovrebbe spiegare:

- modello locale e offline;
- API Lua allowlist;
- nessun filesystem/rete;
- memoria e instruction budget;
- compilazione e dry-run;
- normalizzazione;
- fallback;
- filtri testuali;
- sistema di segnalazione output problematico.

## Test a budget ridotto

### Simulazioni sul PC esistente

- limitare core con `taskset`;
- limitare memoria con cgroup/systemd;
- variare offload GPU;
- testare 720p, 800p, 1080p, 1440p;
- forzare software renderer;
- interrompere il processo generatore;
- corrompere il modello o il RunBundle;
- testare directory non scrivibili;
- testare controller e cambio risoluzione.

### Hardware esterno

- Steam Playtest con tester selezionati;
- branch beta protetta;
- richiesta Steam Deck dev kit quando idonei;
- prestito/acquisto usato di un Deck vicino alla demo;
- build Windows nativa oltre ai test Proton.

## Posizionamento di mercato

Non vendere il titolo come semplice “AI game”. Enfatizzare:

- meccaniche realmente nuove per run;
- offline e privacy;
- seed/RunBundle condivisibili;
- azione arcade immediata;
- trasparenza dei sistemi;
- generazione validata, non caos incontrollato.

Rischi:

- diffidenza verso asset AI;
- attese troppo lunghe;
- meccaniche illeggibili o sbilanciate;
- percezione di contenuto superficiale;
- download del modello molto grande;
- crash attribuiti all’AI.

Contromisure:

- mostrare la meccanica generata durante la preparazione;
- iniziare il gameplay soltanto con bundle valido;
- cache di run locali non ancora giocate;
- fallback raro ma solido;
- demo con controller e Deck verificati;
- comunicazione precisa su cosa viene generato.
