# Stack dei modelli

## Scelta principale: Stable Diffusion 1.5

Motivi:

- ecosistema LoRA maturo;
- risoluzione nativa 512×512 coerente con la pipeline esistente;
- peso e memoria inferiori ai modelli moderni più grandi;
- supporto già funzionante in stable-diffusion.cpp;
- backend CPU, CUDA, Vulkan, Metal e altri;
- supporto LoRA, LCM/LCM-LoRA, TAESD e ControlNet SD1.5;
- benchmark già raccolti sul PC di sviluppo.

SD1.5 non è scelto perché produce la qualità massima assoluta, ma perché offre il miglior
rapporto fra:

- dimensioni;
- compatibilità;
- addestrabilità su Kaggle;
- inferenza locale;
- ecosistema;
- costo d'integrazione.

## Base consigliata

Per il ramo canonico:

```text
stable-diffusion-v1-5/stable-diffusion-v1-5
```

Conservare:

- repository esatto;
- revisione/commit;
- hash dei file;
- licenza;
- data di download.

## Checkpoint pixel-art di terze parti

Utilizzarli come benchmark, non come fondamento automatico del ramo commerciale.

Problemi possibili:

- dataset non documentato;
- derivazione non chiara;
- stili e personaggi memorizzati;
- trigger proprietari;
- compatibilità non garantita;
- licenza della pagina diversa dai diritti reali sui dati.

Confronto corretto:

```text
A: SD1.5 vanilla + Worldsmelt Style LoRA
B: checkpoint pixel di terze parti + Worldsmelt Style LoRA
```

Vince il sistema che ottiene migliori asset in-engine, non l'immagine più appariscente in
isolamento.

## LCM-LoRA

Uso:

- accelerare l'inferenza a pochi step;
- permettere una modalità rapida;
- ridurre attesa nel Piano 0.

Non deve sostituire la Style LoRA.

Stack previsto:

```text
SD1.5
+ worldsmelt-style
+ worldsmelt-enemies/items/...
+ lcm-lora-sdv1-5
```

Serve benchmark A/B perché la somma degli adattatori può cambiare:

- palette;
- silhouette;
- adesione al prompt;
- pulizia dello sfondo.

## TAESD

TAESD può ridurre memoria e latenza del decoding, ma la pipeline corrente ha osservato una
resa più nitida con la VAE reale. Mantenerlo come tier low-spec, non come default
automatico.

## Qwen2.5-Coder

Ruolo:

- generazione di RunManifest;
- specifiche di nemici;
- comportamento Lua limitato;
- temi, nomi e descrizioni;
- selezione di body plan e rig;
- richiesta degli asset necessari.

Non deve:

- decidere collisioni arbitrarie;
- inventare API;
- modificare regole fondamentali;
- generare codice senza schema e validazione;
- produrre una descrizione grafica non traducibile in un rig supportato.

## Modelli più piccoli

Dopo la Style LoRA funzionante si può valutare un modello SD compresso, ma solo con test
separati. Modifiche architetturali dell'UNet possono rendere incompatibili LoRA addestrate
sulla base SD1.5 standard.

Criterio di adozione:

```text
qualità in-engine >= soglia
tempo <= target
VRAM <= tier
LoRA compatibili o riaddestrabili
licenza/provenienza documentate
```

## Modelli moderni più grandi

FLUX, SDXL e modelli analoghi possono essere utili come strumenti di concept art o
benchmark esterni. Non sono la baseline runtime per Worldsmelt finché:

- aumentano il requisito hardware;
- non migliorano abbastanza il tasso di asset validi;
- complicano training e distribuzione;
- non rispettano il target local-first.
