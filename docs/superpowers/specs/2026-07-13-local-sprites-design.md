# Spec: sprite generati in locale (fase 2)

Data: 2026-07-13
Ciclo: 2 di 5 (vedi la roadmap in `2026-07-13-local-llm-linux-design.md`)
Stato: progettata sui numeri dello spike (`docs/SPRITES-SPIKE.md`), da approvare dall'autore

## 1. Obiettivo

Oggi il gioco disegna sprite procedurali: l'atlas BMP che `melting-gen` genera con
primitive geometriche in C. Questa fase lo sostituisce con **sprite veri, generati in
locale da Stable Diffusion**, coerenti col tema della run appena inventata dall'LLM.

Fuori scope: sprite per piano (l'atlas resta uno per run), animazioni, sprite degli
oggetti equipaggiati sul personaggio (restano i layer geometrici attuali).

## 2. Fattibilita', gia' misurata

Lo spike (`docs/SPRITES-SPIKE.md`) ha misurato sulla macchina di riferimento:
**~5,3 s per sprite**, **~75 s per i 12 sprite** dell'atlas, **2,0 GB di VRAM**.
Il post-processing costa ~70 ms. La qualita' a 128x128 e' buona (immagini nel documento).

## 3. Vincolo che decide l'architettura

llama.cpp e stable-diffusion.cpp **vendorizzano due ggml diversi e incompatibili**
(sd.cpp usa il fork `leejet/ggml`). Linkarli nello stesso binario e' un conflitto di
simboli. Sono quindi **due eseguibili separati**, il che e' anche cio' che vogliamo per
la VRAM: 4,5 GB (LLM) e 2,0 GB (SD) non coesistono nei 6 GB della scheda, ma i due
processi si alternano e ognuno libera tutto quando esce.

```text
INIZIO RUN (schermata di caricamento)

  gioco
   ├─ 1) lancia melting-gen   → testo (Qwen 7B, ~50 s) → current_run.txt → esce, VRAM libera
   └─ 2) lancia melting-sprites → 12 sprite (SD1.5+LCM, ~75 s) → current_atlas.png → esce
        (legge il manifest del passo 1: temi, stile, palette guidano i prompt)

  il gioco carica manifest + atlas → si gioca
```

Se i modelli SD non sono presenti, il passo 2 si salta e resta l'atlas BMP procedurale:
il gioco funziona comunque, come oggi.

## 4. melting-sprites

Nuovo eseguibile C99 (`tools/melting-sprites/`), che linka stable-diffusion.cpp
(tag fissato `master-775-b5d8120`, build Vulkan) e vendorizza `stb_image`,
`stb_image_write` (dominio pubblico) ed `exoquant` (MIT).

1. Legge `generated/current_run.txt` per tema, stile e palette del piano 1.
2. Per ognuna delle 12 celle dell'atlas (player, 3 nemici, boss, oggetto, cuore, moneta,
   bomba, chiave, portale, colpo) costruisce un prompt: soggetto fisso + tema/stile della
   run + il trigger `pixelsprite` + la LoRA LCM.
3. Genera a 512x512 (8 passi, LCM, cfg 1.5), riusando lo stesso contesto SD per tutti e 12.
4. Post-processing per sprite: downscale modale 4x a 128x128, ritaglio dello sfondo con
   flood fill dai bordi, riduzione a 16 colori, `KEY_FLOOR` sui colori troppo scuri.
5. Compone l'atlas 1024x1024 RGBA (8x8 celle da 128) e lo scrive come PNG.
6. Aggiorna `atlas.path` nel manifest a `generated/current_atlas.png`.

**Perche' non lo sfondo nero** (lo spike lo ha dimostrato): la pixel art ha contorni neri,
e su sfondo nero il ritaglio li mangia. Si chiede uno sfondo piatto **senza nominarne il
colore** (nominarlo colora il soggetto), lo si riconosce dal bordo dell'immagine e lo si
rimuove col flood fill, che non puo' raggiungere i neri *interni* allo sprite.

Qualita': se uno sprite esce implausibile (meno del 5% o piu' del 70% di pixel opachi, o
il contorno tocca il bordo della cella) si rigenera con un altro seed, al massimo una
volta. Ogni cella che fallisce comunque resta vuota e il gioco disegna la sua forma
geometrica di riserva per quell'entita': **nessuna cella rotta puo' rompere il gioco**.

Flag: `--out DIR`, `--model`, `--lora`, `--steps`, `--seed`, `--cells N` (per test rapidi),
`--dry-run` (salta SD, produce celle di prova: rende testabile tutto il post-processing
senza modello).

## 5. Integrazione nel gioco

`src/gen/gen_runner` gia' sa gestire un processo figlio. Il gioco ora ne esegue **due in
sequenza** nello stato `APP_GENERATING`: la barra mostra 0-50% per il testo e 50-100% per
gli sprite, leggendo lo stesso file di progresso (che il secondo processo sovrascrive).

- Il passo sprite parte solo se il passo testo e' andato a buon fine e i modelli SD
  esistono in `models/`.
- ESC annulla il passo in corso e torna al menu.
- Se il passo sprite fallisce o va in timeout, si gioca comunque: l'atlas BMP e' gia' li'.
- Il timeout del secondo passo e' 240 s (75 s attesi, con margine per macchine lente).
- Flag `--no-sprites` per saltarlo (utile per iterare in fretta sul testo).

Il gioco carica gia' i PNG con chroma-key (`src/assets/game_assets.c`); poiche' ora l'atlas
ha un vero canale alpha, il caricamento usa quello e il chroma-key resta solo come rete.

## 6. Test

- **Senza modelli SD** (`make test-sprites`): `--dry-run` genera celle sintetiche che
  contengono tutti i casi difficili (contorno nero, pixel nero *dentro* lo sprite, ombra
  sfumata, macchia del colore di sfondo racchiusa nel corpo). Si verifica: lo sfondo viene
  tagliato, i neri interni sopravvivono, nessun pixel opaco finirebbe nel chroma-key, il
  PNG e' 1024x1024 RGBA con 8x8 celle.
- **Determinismo:** stesso seed e stesso manifest → atlas identico byte per byte.
- **Con modelli** (`make test-sprites-sd`): generazione vera di 2 celle, tempi e VRAM a log.
- **Regressione:** `make test` e `make test-gen` restano verdi; il gioco parte anche senza
  modelli SD.

## 7. Licenze

Il modello dello spike (All-In-One-Pixel-Model) e' **CreativeML OpenRAIL-M**: le immagini
generate sono tue e vendibili, ma se un giorno **ridistribuisci i pesi** col gioco devi
propagare le restrizioni d'uso della licenza. Esiste un'alternativa **Apache 2.0**
(`SD_PixelArt_SpriteSheet_Generator`), leggermente peggiore in resa. Lo script di download
prende il primo di default e documenta il secondo: la scelta e' dell'autore, e va fatta
prima di distribuire il gioco, non prima di giocarci.

## 8. Criteri di successo

1. `make run-gen` genera testo **e** sprite, e la run e' giocabile con sprite coerenti col tema.
2. Senza modelli SD, il comportamento e' quello di oggi (atlas BMP), senza errori.
3. `make test-sprites` verde senza scaricare nulla.
4. Nessuno sprite rotto puo' rompere il gioco: ogni cella ha la sua riserva geometrica.
5. Al termine, nessun processo residuo e VRAM libera.
