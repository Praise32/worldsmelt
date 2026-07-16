# 06 — Training dei modelli, hardware e costi GPU

## Decisione

### SD 1.5

Sì al fine-tuning leggero, in modo progressivo:

1. Style LoRA;
2. Item LoRA;
3. Projectile/VFX LoRA;
4. Player Mutation LoRA;
5. ControlNet o modello di animazione soltanto se i benchmark dimostrano che serve.

### Qwen

No, non subito. Prima usare il modello base con:

- prompt brevi e strutturati;
- GBNF o JSON Schema;
- esempi selezionati;
- Capability Registry;
- compilatore;
- retry con errore preciso;
- dataset di proposte accettate e rifiutate.

Il fine-tuning di Qwen serve in seguito a ridurre retry e uniformare la DSL. Non crea capacità che l’engine non possiede.

## RX 5600 XT: ruolo realistico

La RX 5600 XT ha 6 GB di VRAM; AMD la documenta come scheda GDDR6 da 6 GB nella [pagina di lancio ufficiale](https://www.amd.com/en/newsroom/press-releases/2020-1-6-amd-unveils-four-new-desktop-and-mobile-gpus-incl.html). Non compare nelle matrici Radeon attualmente supportate da [ROCm su Linux](https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityrad/native_linux/native_linux_compatibility.html).

Conseguenze:

- inferenza tramite backend già funzionante, preferibilmente Vulkan/ggml;
- nessun affidamento su training PyTorch ROCm ufficialmente supportato;
- smoke test locali piccoli possibili soltanto come esperimento;
- training ripetibile su CUDA a noleggio;
- Qwen e SD caricati in sequenza;
- 16 GB di RAM impongono contesto e cache prudenti.

Non conviene spendere settimane per forzare ROCm sulla 5600 XT se una campagna LoRA costa decine di dollari.

## Qwen con llama.cpp

Configurazione iniziale:

- Qwen2.5-Coder-7B-Instruct GGUF;
- Q4_K_M come baseline;
- contesto 4k, da aumentare soltanto se necessario;
- prompt che contengono solo RunBible, schema, capacità e pochi esempi;
- output vincolato;
- una richiesta produce più candidati compatti;
- KV/prefix cache per la parte comune;
- modello scaricato prima del batch SD.

La [documentazione Qwen per llama.cpp](https://qwen.readthedocs.io/en/v2.5/run_locally/llama.cpp.html) supporta GGUF ufficiali e quantizzazione. Le [grammatiche di llama.cpp](https://github.com/ggml-org/llama.cpp/blob/master/grammars/README.md) sono il primo strumento da provare per vincolare l’output.

Prima di addestrare, misurare:

- validità JSON al primo tentativo;
- percentuale di grafi compilabili;
- retry medi;
- tempo per candidato;
- novelty;
- valutazione umana;
- differenza fra 3B e 7B sui prompt reali.

## SD 1.5 con stable-diffusion.cpp

Configurazione iniziale:

- una base SD 1.5;
- una Style LoRA;
- generazione a 256 o 512 px;
- batch per ruolo;
- LCM-LoRA a pochi step solo dopo confronto qualitativo;
- downscale e palette deterministici;
- niente text encoder training nella prima prova;
- generazione dopo aver liberato la memoria di Qwen.

La [documentazione Diffusers su LoRA](https://huggingface.co/docs/diffusers/main/training/lora) spiega che LoRA riduce i parametri addestrabili e riporta, come riferimento, circa 5 ore per un run completo su RTX 2080 Ti 11 GB. Non è un benchmark della tua pipeline, ma dimostra che SD 1.5 LoRA è un carico da ore, non da settimane.

## Piano di addestramento SD

### Esperimento 0 — Baseline

- nessun training;
- 30 prompt fissi;
- 10 seed per prompt;
- valutazione in-engine;
- misura di silhouette, palette, alpha e coerenza.

### Esperimento 1 — Style LoRA

- 100–300 immagini iniziali;
- rank 4–16;
- UNet soltanto;
- text encoder congelato;
- checkpoint frequenti;
- holdout per pack/autore;
- confronto cieco con baseline.

### Esperimento 2 — LoRA di ruolo

Una alla volta. Prima item, perché è il risultato più semplice da valutare; poi projectile/VFX; infine mutazioni del personaggio.

Per ogni ruolo:

1. definire 30 prompt di test;
2. addestrare più checkpoint;
3. generare gli stessi seed;
4. fare validazione automatica;
5. fare review umana in-engine;
6. tenere un solo vincitore.

### Esperimento 3 — Controllo strutturale

Se SD continua a sbagliare pose, socket o silhouette, valutare ControlNet. La [guida ufficiale ControlNet](https://huggingface.co/docs/diffusers/training/controlnet) indica circa 38 GB nella configurazione standard e descrive ottimizzazioni per 16 GB. Per una campagna affidabile è più semplice noleggiare 48 GB.

Non introdurre ControlNet soltanto perché “suona più avanzato”: deve risolvere un errore misurato.

## Quando addestrare Qwen

Avviare un esperimento QLoRA soltanto quando esistono:

- almeno 1.000–5.000 esempi realmente specifici del gioco;
- output canonici validati;
- motivi di rifiuto;
- validation set bloccato;
- baseline del modello non addestrato;
- metriche di compilazione, retry e qualità.

Dataset:

    input:
      RunBible
      PlayerStyleProfile
      pool
      budget
      Capability Registry

    output:
      ItemDefinition canonica
      Effect Graph
      Appearance Contribution
      tooltip derivabile

Non usare un generico dataset Lua/Coder come obiettivo principale: Qwen sa già programmare. Deve imparare il linguaggio e il gusto di questo gioco.

Il framework indicato nella [documentazione Qwen 2.5, ms-swift](https://qwen.readthedocs.io/en/v2.5/training/RL/ms_swift.html), supporta LoRA e Q-LoRA. I [profili Qwen pubblicati per la precedente generazione 7B](https://github.com/QwenLM/Qwen/blob/main/recipes/finetune/deepspeed/readme.md) sono soltanto indicativi, ma mostrano perché un QLoRA 7B può rientrare in 24 GB con contesti prudenti e perché i contesti lunghi riducono il margine. Per il progetto, 2k–4k token per esempio sono preferibili a 8k.

## Prezzi GPU di riferimento

Prezzi pubblicati il 16 luglio 2026. Disponibilità e regione cambiano.

| GPU | VRAM | Community/da | Secure/on-demand |
|---|---:|---:|---:|
| RTX 4090 | 24 GB | $0,34/h | $0,69/h |
| RTX A6000 | 48 GB | $0,33/h | $0,49/h |
| RTX A5000 | 24 GB | variabile | circa $0,27/h nella pagina Pods |
| RTX 3090 | 24 GB | variabile | circa $0,46/h nella pagina Pods |
| A40 | 48 GB | variabile | circa $0,44/h nella pagina Pods |
| L40S | 48 GB | variabile | circa $0,99/h nella pagina Pods |
| A100 PCIe | 80 GB | variabile | circa $1,39/h nella pagina Pods |

Fonti: [RunPod RTX 4090](https://www.runpod.io/gpu-models/rtx-4090), [RunPod RTX A6000](https://www.runpod.io/gpu-models/rtx-a6000) e [listino Pods](https://www.runpod.io/pricing).

La tariffa più bassa non include necessariamente la stessa affidabilità, banda, CPU, storage o disponibilità. Per il primo training, una 4090 24 GB è la scelta semplice; una A6000/A40 48 GB serve quando il job non entra comodamente.

## Costi effettivi stimati

Formula:

    costo compute = ore GPU × tariffa

Aggiungere:

- disco persistente;
- upload/download;
- CPU e RAM se fatturati;
- tempo di setup;
- istanza inattiva;
- run falliti;
- IVA o imposte applicabili.

### Singola LoRA SD 1.5

Riferimento operativo: 2–8 GPU-ore su 4090 per un esperimento, da verificare.

| Tariffa 4090 | 2 ore | 8 ore |
|---|---:|---:|
| Community $0,34/h | $0,68 | $2,72 |
| Secure $0,69/h | $1,38 | $5,52 |

Dieci run: circa $7–$55 di solo calcolo. Budget prudente con errori e storage: $30–$100.

### Tre LoRA specialistiche

Ipotizzando 30–120 GPU-ore complessive:

- Community 4090: circa $10–$41;
- Secure 4090: circa $21–$83;
- budget operativo prudente: $75–$250.

### ControlNet

Stima ampia: 15–60 GPU-ore per run.

Su A6000:

- Community $0,33/h: circa $5–$20;
- Secure $0,49/h: circa $7–$30.

Con più sweep, dataset grande, validation e run falliti: $100–$400 è un budget più realistico di campagna.

### Qwen2.5-Coder 7B QLoRA

Stime da confermare:

- 1.000 esempi, contesto breve e poche epoche: 6–12 GPU-ore;
- 5.000 esempi: 20–40 GPU-ore;
- singolo run su 4090 Community: circa $2–$14;
- campagna da più esperimenti: circa $30–$150, oltre a storage.

Il costo umano di pulire e valutare gli esempi sarà probabilmente superiore al calcolo.

## Budget raccomandato

### Vertical slice

- $0 per Qwen training;
- $50–$150 per Style LoRA e prime LoRA di ruolo;
- tetto iniziale consigliato: $200.

### Pipeline indie più rifinita

- $200–$600 includendo più specialisti, sweep e un primo ControlNet;
- Qwen QLoRA solo dopo che il dataset esiste.

### “Qualità Retro Diffusion”

Non è un singolo budget di fine-tuning. Il prodotto ufficiale combina generazione, reference, animazione, sprite sheet, palette, rimozione sfondo, tileset ed editor. Tentare di replicare l’intera piattaforma richiederebbe dataset, tooling e lavoro artistico molto oltre la vertical slice.

Obiettivo corretto:

> raggiungere una qualità convincente per un solo stile, un personaggio, tre ruoli visivi e 20 oggetti.

## Checklist prima di noleggiare

- dataset già caricato e validato localmente;
- ambiente in container o script riproducibile;
- checksum del dataset;
- comando di training salvato;
- test da 100 step;
- checkpoint su storage persistente;
- timer o alert di costo;
- download immediato di LoRA, config e log;
- spegnimento e cancellazione dell’istanza quando conclusa;
- registro di GPU, driver, librerie, seed e durata.
