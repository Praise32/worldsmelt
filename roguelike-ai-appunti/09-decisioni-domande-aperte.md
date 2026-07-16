# 09 — Decisioni e domande aperte

## Decisioni già prese

- un solo PC Ubuntu, RX 5600 XT, Ryzen 5, 16 GB DDR4;
- llama.cpp e Stable Diffusion già configurati;
- target minimo 60 FPS;
- primo piano circa 10 minuti;
- 20 oggetti sufficienti per la vertical slice;
- sono accettabili soltanto i tratti visivi dominanti;
- DSL molto espressiva;
- interesse reale per fine-tuning;
- multiplayer con modalità sia a mondo uguale sia a mondi diversi.

## Default proposti fino a prova contraria

- canvas 640×360;
- player 64×64;
- atlas 96×96;
- tre direzioni disegnate più mirroring;
- animazione 6–15 FPS dentro render 60 FPS;
- rig a socket e maschere, non scheletro deformabile complesso;
- Qwen2.5-Coder 7B Q4_K_M, contesto 4k;
- Qwen e SD sequenziali;
- una Style LoRA più LoRA di ruolo;
- nessun training Qwen nella prima fase;
- 4090 24 GB per LoRA;
- A6000/A40 48 GB se serve ControlNet;
- Mirror Race come prima modalità equa.

## Domande bloccanti per il prossimo documento tecnico

### Gameplay

1. Il controllo è twin-stick, quattro direzioni di sparo, otto direzioni o mira libera con mouse?
2. La stanza resta tutta visibile come in Isaac o la camera segue il personaggio?
3. Quanti nemici e proiettili massimi vuoi vedere nella situazione peggiore?
4. Vuoi contatto fisico col terreno per i proiettili “rimbalzanti” o è una metafora visiva in vista top-down?
5. Il primo personaggio ha una forma narrativa precisa o può essere progettato attorno alla modularità?

### Stile

6. Quali tre riferimenti visivi descrivono il tono desiderato senza copiare un artista o un gioco?
7. Preferisci contorni netti, morbidi, sporchi o quasi assenti?
8. Palette per run da 16, 24 o 32 colori?
9. Il corpo può deformarsi molto o deve restare sempre riconoscibile?
10. Quanto horror/grottesco è accettabile?

### Ambiente Ubuntu

11. Modello esatto del Ryzen 5?
12. Versione Ubuntu e kernel?
13. Versione/commit di llama.cpp?
14. Backend usato da llama.cpp: Vulkan, CPU o altro?
15. Quale interfaccia/programma avvia Stable Diffusion?
16. Quale checkpoint SD 1.5 e quali LoRA sono già installati?
17. Quanto impiegano oggi un output Qwen da 512 token e un’immagine 512×512?
18. Quanto spazio libero c’è sull’SSD?

### Dataset e budget

19. Puoi creare o commissionare 100–300 asset originali supervisionati?
20. Le LoRA resteranno private o vuoi distribuirle col gioco?
21. Qual è il budget massimo per la prima campagna cloud: $50, $150 o $300?
22. Sei disposto a fare review manuale di centinaia di output?

### Distribuzione

23. Target iniziale soltanto Linux o anche Windows/Steam?
24. I modelli saranno inclusi nel gioco o selezionati dall’utente?
25. La modalità senza GPU generativa deve esistere?
26. Vuoi che il giocatore possa importare modelli/LoRA propri?

## Tre risposte da ottenere per prime

Le prossime decisioni tecniche dipendono soprattutto da:

1. controllo e camera;
2. benchmark reale di Qwen e SD sul PC Ubuntu;
3. riferimento visivo e capacità di creare/commissionare dataset originale.

Il resto può usare i default proposti.

## Benchmark da riportare

Per evitare di scambiare ipotesi per fatti, raccogliere:

| Test | Valore |
|---|---|
| Modello Qwen e quantizzazione | |
| Contesto | |
| Backend | |
| Prompt token | |
| Output token | |
| Prefill tok/s | |
| Decode tok/s | |
| Picco RAM/VRAM | |
| Checkpoint SD | |
| Risoluzione | |
| Step e sampler | |
| Secondi/immagine | |
| Picco RAM/VRAM | |
| Tempo scarico Qwen → carico SD | |

Con questi numeri si potrà scrivere un piano preciso di scheduling per i dieci minuti del primo piano.

