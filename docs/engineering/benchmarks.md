---
id: eng-benchmarks
title: Benchmark melting-gen e melting-sprites — macchina di riferimento
domain: engineering
status: implemented
authority: supporting
owner: engineering
summary: >-
  Misure di velocità di generazione (testo e sprite) sulla macchina di riferimento,
  contesto della misura, disambiguazione tra i due meccanismi di benchmark del
  repo e la storia del meccanismo di tier automatico, rimosso da DEC-110 (il gioco non legge più logs/benchmark.txt).
  La tabella testuale è congelata come misura storica del 13/07 (DEC-149); il modello
  di testo attivo oggi è citato con la formula neutra di DEC-151 (Gemma-3-4B-IT Q4, DEC-140).
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [benchmark, performance, tier, vulkan, gpu, DEC-149, DEC-151]
related: []
supersedes: []
source_files: [scripts/benchmark.sh, scripts/test-llm.sh, tools/melting-gen/main.c, tools/melting-sprites/main.c, src/app/app.c]
---

# Benchmark melting-gen e melting-sprites — macchina di riferimento

> **Misura storica, congelata (DEC-149).** La tabella e l'analisi qui sotto sono state
> misurate il **13/07/2026** (commit `b836e96`) sui modelli allora di default
> (`qwen2.5-coder-1.5b-instruct-q4_k_m.gguf` e `qwen2.5-coder-7b-instruct-q4_k_m.gguf`):
> **non descrivono il default attuale di `melting-gen`**. Restano fedeli come dato storico
> e come spiegazione del meccanismo di tier automatico (rimosso da DEC-110): non sono state
> rimisurate né riscritte. Il paragrafo subito sotto dice qual è il modello di testo attivo
> oggi; la rimisurazione sulla macchina di riferimento coi modelli attuali resta
> un'attività successiva (DEC-142), fuori da questo lavoro.

## Il modello di testo attivo oggi

Il **modello di testo attivo (oggi Gemma-3-4B-IT Q4, DEC-140)** ha sostituito il 7B Coder
Q4_K_M come default di `tools/melting-gen` il 23/07/2026: la comparison su 11 modelli × 3
seed fissi (`docs/ai-production/experiments/model-comparison-testo-2026-07-23.md`) lo ha
misurato sopra il vecchio default su punteggio, Lua valido al primo colpo e tok/s, a metà
del peso su disco. Questa formula neutra (DEC-151) è quella da usare in tutta la KB, perché
il modello può ancora cambiare a una prossima comparison; il nome puntuale e i termini di
licenza (Gemma Terms of Use, diversi da Apache/MIT) restano in
`docs/ai-production/licenze.md` e nel decision-log. Il fallback automatico su errore di
caricamento resta il Coder 1.5B Q4 (invariato da DEC-140), lo stesso citato come tale nella
tabella storica sotto. **Questa sostituzione di default non è ancora stata ribenchmarkata**
sulla tabella `make test-llm` qui sotto: i numeri di quella tabella restano quelli del 7B
Coder del 13/07, per il motivo spiegato nel banner sopra.

## Contesto della misura

- **Macchina di riferimento**: Ryzen 5 3600, **AMD RX 5600 XT 6 GB VRAM** (RDNA1, Mesa RADV,
  backend **Vulkan**), Ubuntu 26.04.
- **Periodo delle misure**: la tabella sotto (`make test-llm`, sweep manuale per modello/ngl)
  risale al **13/07/2026**, commit `b836e96` ("calibrate default model and gpu offload from
  measured benchmarks" — unico commit che ha toccato questo file, `git log --follow`). Le
  misure del meccanismo di tier automatico (`logs/benchmark.txt`, sezione più sotto) sono
  successive, del **17/07/2026** (introduzione del tier automatico, vedi sotto).
- **Commit di riferimento di questo documento**: `fe27f6d` (HEAD al momento della stesura).
- Il file originale (`docs/archive/superseded/benchmarks-2026-07-13.md`, non aveva front matter) non registrava data né
  commit: entrambi sono stati ricavati da `git log` per questa migrazione.

## Tabella misurata — sweep manuale (`make test-llm`)

Comando: `MODEL=... NGL=... SEED=42 make test-llm`. Una riga per corsa, copiata da
`logs/melting-gen.log`. Colonna VRAM = somma dei buffer Vulkan0 riportati dal log di
caricamento di llama.cpp (model buffer + KV buffer + compute buffer); è il consumo reale
sulla scheda, non la dimensione del file .gguf.

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

### Cosa dicono questi numeri, in pratica

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
serviva da base per il vecchio meccanismo di tier automatico (rimosso da DEC-110, vedi sotto).

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
vince su entrambi i fronti. Il modello 1.5B resta il fallback automatico su errore di
caricamento del 7B (`modelFallback` in `tools/melting-gen/main.c`): fallback di
robustezza, tuttora attivo e indipendente dal vecchio preset di qualità (DEC-110).

## Disambiguazione: due "tok/s" diversi nel repo

Il repo contiene **due meccanismi di misura distinti**, che producono entrambi un
numero in tok/s (o img/s) ma **non misurano la stessa cosa**. Confonderli porta a
confrontare mele con pere.

### `make test-llm` (la tabella sopra) — generazione reale col prompt del gioco

`scripts/test-llm.sh` invoca `bin/melting-gen --model ... --ngl ... --seed ... --out generated`
**senza** `--bench`: è la stessa identica strada di produzione che il gioco percorre a
`--generate`. Genera l'intera run (5 piani, oggetti, script Lua per oggetto, eventuale
personaggio proposto), usando i prompt reali in `prompts/` e la grammatica GBNF del gioco
(`run.gbnf`). Il tok/s riportato in tabella viene letto a posteriori dall'ultima riga
`ok: model=...` di `logs/melting-gen.log`: è quindi il throughput di una generazione vera,
comprensiva del costo reale del prompt di produzione e della grammatica vincolata.
`test-llm.sh` verifica anche varietà dei contenuti (5 temi/tipi di colpo distinti, guardia
"anti-fotocopia") e assenza di italiano nei campi generati — non è un benchmark puro, è il
test end-to-end che *incidentalmente* riporta anche i tempi.

### `melting-gen --bench` / `melting-sprites --bench` — throughput puro, isolato dai prompt di produzione

`scripts/benchmark.sh` invoca invece `bin/melting-gen --bench` e `bin/melting-sprites --bench`
in sequenza (mai insieme: i due modelli non convivono nella VRAM di riferimento). Qui il
codice (`tools/melting-gen/main.c`, funzione `RunBench`, righe 341-410) è esplicito:

> "Carica lo STESSO modello di una generazione vera... genera `GEN_BENCH_TOKENS` token da un
> prompt FISSO hardcoded... nessuna dipendenza da `prompts/`... NON tocca `generated/`."

Cioè `--bench` misura **la velocità pura del modello** (caricamento + tok/s su un prompt
fisso, breve e sempre uguale, non i prompt di produzione del gioco), apposta per essere
confrontabile fra macchine e non inquinato dal costo della grammatica GBNF del JSON di
produzione (il commento nel codice nota esplicitamente che il costo del prompt fisso è
trascurabile rispetto ai token generati, e che non vale la pena separare le due fasi "per
soglie grossolane come quelle del tier"). Il gemello lato sprite (`tools/melting-sprites/main.c`,
funzione `RunBench`) genera sempre a 512px (indipendente da `--gen-size`) perché misura "la
pipeline di riferimento", non il preset alleggerito.

In sintesi: **la tabella di questo documento** (da `make test-llm`) è la calibrazione una
tantum dei default di produzione in `tools/melting-gen/main.c` (modello e `ngl` di default);
**`--bench`** alimenta il report diagnostico di `make benchmark` descritto sotto. Sono complementari, non intercambiabili: uno misura "quanto è buona/veloce una
generazione vera", l'altro "quanto è veloce il modello nudo su questa macchina".

## Meccanismo del tier automatico

`make benchmark` (target che esegue `scripts/benchmark.sh`) orchestra i due `--bench` in
sequenza e scrive `logs/benchmark.txt` in formato chiave=valore:

```
benchSchema=1
tokS=42.29
imgS=5.62
tier=full
measuredAt=1784275965
```

(valori reali osservati in `logs/benchmark.txt` in questo repo).

Soglie (`scripts/benchmark.sh`, righe 56-74), misurate non dedotte:

- `tokS >= 12` **e** `imgS <= 8` → tier `full` (hardware alla pari/sopra la scheda di
  riferimento);
- altrimenti `tokS >= 6` → tier `lowspec` (testo comunque utilizzabile anche senza SD);
- sotto → tier `unsupported` (si può comunque giocare, fallback procedurale sempre presente,
  ma la generazione IA sarà lenta o assente).

**Dal 22/07/2026 (DEC-110) il gioco NON legge più questo file.** Il preset automatico
`--low-spec` (testo 1.5B + sprite 256px), i flag `--low-spec`/`--full-spec` e il test
`--bench-preset-test` sono stati rimossi dal codice: nessun tier di qualità automatico, i
requisiti minimi del gioco completo sono quelli per far girare i modelli di riferimento
(7B + SD1.5) e l'hardware migliore significa solo attese più brevi. `make benchmark` resta
uno **strumento diagnostico manuale**: misura la macchina e scrive il report qui sopra
(tier compreso, come pura informazione), ma nulla nel gioco lo rilegge. Restano intatti i
fallback di robustezza: 1.5B quando il 7B non si carica (`tools/melting-gen/main.c`,
`modelFallback`), generatore deterministico, modalità solo-curato.

### Il warmup Vulkan della prima immagine si scarta

`melting-sprites --bench` (`tools/melting-sprites/main.c`, funzione `RunBench`, righe
639-664) genera **due** immagini di prova e tiene solo la seconda:

> "DUE generazioni: la prima paga il warmup Vulkan (compilazione delle pipeline/shader) e
> viene scartata, la misura è la SECONDA, a regime... Misurato sulla scheda di riferimento:
> prima immagine ~14.9s, seconda ~5.7s — contare il warmup mandava una 5600 XT (che regge il
> full) nel tier lowspec."

Il valore effettivamente scritto in `logs/benchmark.txt` (`imgS=5.62` nella misura osservata
in questo repo) è quindi il tempo della seconda generazione, non della prima: se si contasse
il warmup (~14,9s) come tempo "di regime", la soglia `imgS <= 8` del tier `full` fallirebbe
anche su hardware che in realtà lo supera comodamente a regime.

## Nota storica: l'ambiguità con DEC-070 (risolta da DEC-110)

DEC-070 stabiliva la scelta binaria «nessun tier intermedio», ma il codice applicava in
silenzio il preset `--low-spec`: l'audit documentale del 22/07 aveva registrato la tensione
come open question 13. **Risolta lo stesso giorno da DEC-110**: il preset è stato rimosso
(lettura stretta di DEC-070 confermata); questo documento conserva la descrizione storica
del meccanismo qui sopra come contesto delle misure.
