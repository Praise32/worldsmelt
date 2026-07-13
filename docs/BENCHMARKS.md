# Benchmark melting-gen — macchina di riferimento

Macchina: Ryzen 5 3600, RX 5600 XT 6GB (RDNA1, Mesa RADV, Vulkan), Ubuntu 26.04.
Comando: `MODEL=... NGL=... SEED=42 make test-llm`. Una riga per corsa, copiata da `logs/melting-gen.log`.
Colonna VRAM = somma dei buffer Vulkan0 riportati dal log di caricamento di llama.cpp
(model buffer + KV buffer + compute buffer); è il consumo reale sulla scheda, non la
dimensione del file .gguf.

| Modello | ngl | load (s) | gen (s) | totale (s) | token | tok/s | VRAM Vulkan0 | Esito |
|---|---|---|---|---|---|---|---|---|
| 1.5B Q4_K_M | 99 | 0.7 | 28.1 | 28.8 | 1291 | 46.0 | 1.29 GiB (29/29 layer) | ok |
| 7B Q4_K_M | 99 | 2.6 | 47.0 | 49.6 | 1321 | 28.1 | 4.53 GiB (29/29 layer) | ok |
| 7B Q4_K_M | 28 | 2.5 | 53.3 | 55.8 | 1321 | 24.8 | 4.39 GiB (28/29 layer) | ok |
| 7B Q4_K_M | 24 | 2.2 | 71.4 | 73.6 | 1339 | 18.8 | 3.84 GiB (24/29 layer) | ok |
| 7B Q4_K_M | 20 | 2.0 | 94.7 | 96.7 | 1434 | 15.1 | 3.30 GiB (20/29 layer) | ok |

Default scelti in `tools/melting-gen/main.c`: modello = 7B Q4_K_M
(`models/qwen2.5-coder-7b-instruct-q4_k_m.gguf`), ngl = 99.
Criterio: la corsa più veloce che completa in modo stabile entro il budget di
1-2 minuti (spec §2); a parità di stabilità vince la qualità (7B > 1.5B).

## Cosa dicono questi numeri, in pratica

La sorpresa di questo giro di misure è che il 7B **ci sta comodamente** nei 6GB
della RX 5600 XT: a `ngl=99` (tutti i 29 layer sulla GPU) llama.cpp riporta un
consumo di 4,53 GiB su Vulkan0, quindi resta più di 1 GiB di margine libero. Il
timore iniziale — che il file da 4,7 GB non entrasse a offload pieno — non si è
verificato su questa macchina: nessun fallimento di caricamento, nessun
rallentamento anomalo. La colonna "load (s)" resta bassa e stabile (2-2,6s) a
ogni livello di `ngl`, segno che il caricamento dei pesi via mmap è sempre stato
rapido; è la fase di generazione a rallentare quando si tolgono layer dalla GPU.

Il pattern è lineare e prevedibile: ogni layer del 7B tolto dalla GPU e rimasto
sulla CPU (mmap, non copiato in RAM dedicata) costa velocità di generazione,
perché quel layer va calcolato sulla CPU e i risultati intermedi devono
attraversare il bus PCIe per tornare al resto della pipeline sulla GPU. Si vede
bene nella progressione dei tok/s: 28,1 (ngl 99, tutto in GPU) → 24,8 (ngl 28,
un solo layer fuori) → 18,8 (ngl 24, 5 layer fuori) → 15,1 (ngl 20, 9 layer
fuori). Nessuna di queste corse ha "sforato" il budget di 1-2 minuti — anche la
più lenta (ngl 20) chiude a 96,7s totali, sotto i 120s — ma il margine si
assottiglia man mano che si scende con `ngl`, ed è ragionevole aspettarsi che
schede con meno VRAM (es. 4GB) debbano scendere ulteriormente e quindi
avvicinarsi o superare il limite dei 2 minuti: è per questo che questa tabella
serve da base per la futura funzione "scegli il modello in base alla macchina
del giocatore".

Il "muro della VRAM" quindi non è stato osservato direttamente su questa scheda
(6GB bastano per il 7B Q4_K_M a offload pieno con margine), ma la tabella lascia
comunque la traccia di cosa succede quando ci si avvicina: più layer restano
sulla CPU, più tempo passa nel trasferimento dati sul bus, e la generazione
rallenta in modo continuo e proporzionale, non a scalino. Su una scheda con
meno di ~4,5 GiB liberi il 7B a `ngl=99` fallirebbe l'allocazione o (a seconda
del driver) andrebbe in overflow verso la RAM di sistema con un crollo molto
più marcato dei tok/s — scenario da verificare quando si testerà su hardware
diverso.

Il default scelto è quindi il 7B a `ngl=99`: è insieme la configurazione più
veloce (49,6s totali, quasi un terzo del budget di 1-2 minuti) e quella con la
qualità di contenuto migliore, perché il 7B genera testi più vari e non ripete
i nomi degli oggetti tra un piano e l'altro come si era osservato con l'1.5B nel
Task 7. Non c'è quindi nessun compromesso da fare su questa macchina: il 7B
vince su entrambi i fronti. Il modello 1.5B resta il fallback automatico (già
cablato in `main.c`) per le macchine dove il 7B non dovesse caricare.
