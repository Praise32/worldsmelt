# Runtime di inferenza specializzato — lezione di DS4

## Che cos’è DS4 oggi

`antirez/ds4`, oggi chiamato DwarfStar nel README, è un runtime nativo deliberatamente ristretto. Non è un runner GGUF generico.

Supporta un piccolo insieme di modelli/layout verificati, tra cui:

- DeepSeek V4 Flash;
- DeepSeek V4 PRO su macchine molto grandi;
- GLM 5.2 in configurazioni previste dal progetto.

Backend dichiarati:

- Metal su Mac ad alta memoria;
- CUDA, compresi sistemi multi-GPU/DGX Spark;
- ROCm su sistemi specifici come AMD Strix Halo;
- CPU soprattutto come riferimento/diagnostica.

## Da dove viene il vantaggio

DS4 ottimizza insieme:

- layout tensoriale specifico;
- grafo del modello;
- kernel specifici;
- prompt renderer;
- tool calling;
- KV cache;
- server e agente;
- quantizzazione;
- streaming SSD;
- distribuzione su più dispositivi.

### Quantizzazione asimmetrica

DeepSeek V4 Flash è Mixture-of-Experts. DS4 sfrutta il fatto che gli esperti instradati occupano la maggior parte dello spazio:

- gli esperti routed vengono quantizzati molto aggressivamente;
- componenti sensibili come shared experts, proiezioni, routing e output restano a precisione più alta;
- una importance matrix guida la quantizzazione;
- la qualità viene controllata con test e continuation vectors.

Questa specializzazione è più intelligente di “mettere tutto a 2 bit”.

### Streaming degli esperti da SSD

Quando il modello non entra in memoria:

- i pesi non routed restano residenti;
- una cache mantiene gli esperti routed più usati;
- gli esperti mancanti vengono letti dal GGUF su SSD;
- il caricamento viene sovrapposto al calcolo quando possibile.

È una tecnica particolarmente adatta ai MoE, perché a ogni token si attiva soltanto una parte degli esperti.

### KV cache e sessioni

DS4 include:

- riuso del prefisso KV vivo;
- checkpoint KV su disco;
- ripresa delle sessioni;
- replay esatto dei tool call;
- contesti lunghi;
- modalità distribuite e tensor parallel.

## Perché il confronto con Worldsmelt non è diretto

DS4 è nato per modelli enormi su macchine con 64–512 GB e architetture MoE. Worldsmelt punta probabilmente a un modello denso 4–7B Q4 su:

- Windows e Linux;
- AMD, NVIDIA, Intel e CPU;
- 4–8 GB di VRAM o memoria condivisa;
- Steam Deck;
- budget di distribuzione e assistenza da piccolo team.

Su un modello denso:

- quasi tutti i pesi vengono letti per ogni token;
- non esistono migliaia di esperti da tenere in cache selettiva;
- lo streaming SSD per token tende a essere molto più penalizzante;
- il vantaggio di una quantizzazione “routed experts only” non esiste.

Il fattore dominante tende a essere la banda con cui si leggono i pesi e l’efficienza dei kernel matrix-vector/matrix-matrix. `llama.cpp` ha già anni di ottimizzazioni cross-platform in quest’area.

## È folle creare un runtime proprietario?

**No come progetto di ricerca; sì come prima scelta commerciale per un singolo sviluppatore con budget limitato.**

Un runtime specializzato può dare vantaggi reali:

- binario e API più piccoli;
- nessun ramo per modelli non usati;
- memoria pianificata esattamente;
- prompt e tokenizer fissi;
- KV prefix precompilato;
- kernel scelti per un solo shape;
- quantizzazione costruita sul corpus Worldsmelt;
- avvio e percorso di esecuzione più prevedibili.

Ma il costo nascosto comprende:

- loader GGUF/safetensors;
- tokenizer corretto;
- RoPE/attention/KV cache;
- quantizzazione e dequantizzazione;
- sampling e grammar;
- kernel CPU;
- kernel Vulkan per AMD/NVIDIA/Intel;
- sincronizzazione, buffer e memory planner;
- Windows, Linux e SteamOS;
- differenze driver;
- test numerici contro un riferimento;
- regressioni silenziose di qualità;
- aggiornamento a ogni nuovo modello.

Ridurre il runtime di decine di MB non cambia molto se il modello pesa 2,5–5 GB. Il guadagno deve arrivare da **prestazioni, memoria o qualità della quantizzazione**, non dalla sola dimensione dell’eseguibile.

## Strategia raccomandata in quattro livelli

### Livello 1 — build minimale di llama.cpp

Congelare un commit e compilare soltanto:

- una architettura modello;
- CPU + Vulkan;
- una o due quantizzazioni;
- batch 1;
- contesto 4–8K;
- grammar necessaria;
- nessun server, UI, esempi, backends inutili.

Questa è già una “runtime edition Worldsmelt”, senza riscrivere la matematica.

### Livello 2 — fork specializzato

Rimuovere o fissare:

- auto-detection non necessaria;
- chat template dinamici;
- model router;
- allocazioni variabili;
- formati non usati;
- sampling non usato;
- contesti enormi;
- multi-sessione.

Aggiungere:

- prompt prefix KV precaricato;
- API diretta `GenerateRunBundle`;
- metriche integrate;
- timeout e cancellazione;
- impostazioni hardware validate;
- cache del prefisso di sistema;
- freeze di hash e metadati.

### Livello 3 — quantizzazione specifica Worldsmelt

Senza addestrare altri modelli è possibile:

1. raccogliere un corpus di prompt, API Lua, retry ed esempi Worldsmelt;
2. generare una importance matrix;
3. provare ricette miste Q4/Q5/Q6 per tensor sensibili;
4. misurare compilazione, novità e correttezza, non solo perplexity;
5. distribuire un singolo GGUF verificato.

Questa è la lezione di DS4 più trasferibile: **specializzare la quantizzazione e la validazione prima di riscrivere tutto il runtime**.

### Livello 4 — runtime vero e proprio

Considerarlo soltanto se:

- il modello finale è congelato;
- la build minimale è ancora troppo lenta o pesante;
- il profiler individua colli di bottiglia precisi;
- esiste una suite di equivalenza numerica e qualitativa;
- il vantaggio atteso giustifica mantenere CPU/Vulkan/Windows/Linux/Deck.

Una via intermedia è usare GGML come backend tensoriale ma costruire un grafo e un’API Worldsmelt specifici. È molto meno rischiosa di scrivere da zero kernel e quantizzazione.

## Decisione

Non iniziare da un clone di DS4. Costruire prima **Worldsmelt Inference Runtime v1** come fork minimale e congelato di `llama.cpp`.

Obiettivo:

```text
un modello + un tokenizer + un prompt + un grammar + CPU/Vulkan
```

Solo i benchmark possono autorizzare il passaggio a un motore completamente indipendente.
