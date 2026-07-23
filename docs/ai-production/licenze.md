---
id: aiprod-licenze
title: Licenze dello stack
domain: ai-production
status: approved
authority: canonical
owner: ai-production
summary: >-
  Analisi non legale delle licenze di codice (raylib, llama.cpp, stable-diffusion.cpp, Lua, cJSON) e modelli (Qwen, pixel model OpenRAIL-M, LCM-LoRA, TAESD, Stable Audio Small con Stability Community License, DEC-113).
last_reviewed: 2026-07-23
last_verified_commit: fe27f6d
topics: [licenze, OpenRAIL-M, Apache 2.0, zlib, commercializzazione, CREDITS.txt]
related: []
supersedes: []
source_files: [scripts/download-models.sh]
---
# Licenze dello stack

Scritto perche' hai detto «forse un giorno lo vendo». **Non sono un avvocato e questo non e'
un parere legale**: e' una lettura ragionata delle licenze, con i link per verificarle.
Se un giorno vendi davvero, falle guardare a qualcuno che lo fa di mestiere.

## La domanda che conta

Non e' «posso usare questi modelli?» (si', tutti). E': **cosa distribuisci esattamente?**

Il progetto e' costruito in modo che tu **non ridistribuisca mai i pesi dei modelli**.
`scripts/download-models.sh` li scarica da HuggingFace sulla macchina di chi gioca. Tu
distribuisci il gioco; HuggingFace distribuisce i modelli. Questo cambia tutto, e va
mantenuto: **non mettere i `.ckpt` / `.gguf` dentro il pacchetto del gioco.**

## Il codice

| Componente | Licenza | Cosa comporta |
|---|---|---|
| raylib 6.0 | zlib/libpng | Puoi vendere l'eseguibile chiuso. Serve solo il credito all'autore (Ramon Santamaria) in un `CREDITS.txt`. |
| llama.cpp | MIT | Includi il testo della licenza. Nessun altro vincolo. |
| stable-diffusion.cpp | MIT | Idem. |
| Lua 5.5.0 | MIT | Idem. |
| cJSON | MIT | Idem. |
| stb_image / stb_image_write | dominio pubblico | Nessun vincolo. |
| exoquant | MIT | Includi il testo della licenza. |

Tutto il codice e' quindi compatibile con un gioco commerciale a sorgente chiuso. L'unica
cosa che devi davvero fare e' un `CREDITS.txt` che elenchi questi progetti coi loro testi
di licenza.

## I modelli

| Modello | Licenza | Uso commerciale del modello | Uso commerciale degli output | Se ridistribuisci i pesi |
|---|---|---|---|---|
| Qwen2.5-Coder 7B / 1.5B (testo) | Apache 2.0 | si' | si' | nessun vincolo particolare |
| All-In-One-Pixel-Model (sprite) | CreativeML **OpenRAIL-M** | si' | **si', le immagini sono tue** | devi propagare le restrizioni d'uso dell'Attachment A |
| LCM-LoRA SD1.5 | openrail++ | si' | si' | come sopra |
| TAESD | MIT | si' | si' | nessun vincolo |
| Stable Audio 3 Small, varianti sfx e music (audio, DEC-109) | Stability AI **Community License** + componente T5Gemma sotto **Gemma Terms of Use** | si', fino a 1M$/anno di ricavi (DEC-113) | si' | non previsto: i pesi non si ridistribuiscono mai |
| Gemma-3-4B-IT (testo, DEC-140) | **Gemma Terms of Use** (Google) | si', con le condizioni d'uso Google (prohibited use policy) | si' | non previsto: i pesi non si ridistribuiscono mai |

**Il punto che ti preoccupava.** OpenRAIL-M **non e' una licenza non-commerciale**: permette
di usare il modello e di vendere le immagini che produce. L'unico obbligo scatta se
*ridistribuisci i pesi*: in quel caso devi passare a valle le restrizioni d'uso. Siccome il
gioco scarica i pesi da HuggingFace e non li impacchetta, quell'obbligo **non ti tocca**.

Se un giorno volessi comunque impacchettare i pesi (per esempio per una versione offline su
Steam), le strade sono: (a) accettare OpenRAIL-M e propagare l'Attachment A nella EULA, o
(b) cambiare modello.

## Nota sul modello alternativo Apache 2.0

`Onodofthenorth/SD_PixelArt_SpriteSheet_Generator` e' Apache 2.0 (nessun vincolo di nessun
tipo) ed era il candidato «licenza sicura». **L'ho provato e scartato**: e' addestrato per
produrre *spritesheet*, cioe' quattro pose affiancate in una sola immagine. Anche chiedendo
esplicitamente un soggetto singolo continua a disegnarne quattro, e le nostre celle
dell'atlas ne vogliono uno solo. Riutilizzarlo richiederebbe di ritagliare una posa dalle
quattro — fattibile, ma fragile e con sprite piu' piccoli.

Se un giorno cambi idea, i due modi per usarlo sono: ritagliare la prima posa dello
spritesheet, oppure sfruttarlo per quello che sa fare — le **animazioni**, che oggi il gioco
non ha.

## Le meccaniche ispirate a The Binding of Isaac

Le *regole* di un gioco (formule, algoritmi, meccaniche) non sono coperte da copyright: solo
la loro espressione lo e'. Puoi quindi reimplementare da zero le meccaniche di Isaac —
formula della cadenza di tiro, generazione dei piani, sistema delle cache delle statistiche —
studiandole dalla documentazione dei modder.

Cosa **non** puoi fare: copiare sprite, musica, nomi di personaggi e oggetti, testi delle
wiki. E niente «Isaac» nel titolo. La documentazione dei modder (IsaacDocs) non ha una
licenza esplicita, e le wiki sono CC BY-NC-SA (le pagine piu' vecchie): **leggile, non
copiarne il testo**.

## In pratica, prima di vendere

1. Scrivi `CREDITS.txt` con le licenze dei progetti elencati sopra.
2. Tieni i modelli fuori dal pacchetto: falli scaricare al primo avvio.
3. Nomi, sprite e testi tutti tuoi (gia' cosi': li genera l'IA a ogni run).
4. Fai leggere il tutto a un legale, perche' questo documento non lo e'.

## Stability Community License (Stable Audio Small — DEC-109/DEC-113)

Adottando Stable Audio Small per l'audio (DEC-109) entra nello stack la **Stability AI
Community License**: uso commerciale **gratuito fino a 1M$ di ricavi annui**; oltre quella
soglia serve la licenza Enterprise a pagamento. La decisione DEC-113 accetta questi termini;
la soglia si rivaluta solo a ridosso di 1M$/anno. Come per gli altri modelli: i pesi **non
si ridistribuiscono mai col gioco** (li scarica l'utente) e la licenza va riverificata
all'upstream al momento dell'integrazione vera del modello.

Note dalla model card ufficiale (verificate al download del 23/07):

- Il text-encoder e' un **T5Gemma** (`t5gemma-b-b-ul2`) ridistribuito nel repo sotto i
  **Gemma Terms of Use** (file `LICENSE_GEMMA.md` accanto ai pesi): valgono anche le sue
  restrizioni d'uso — stessa famiglia di licenza del modello di testo (DEC-140).
- **Provenienza del training dichiarata**: ~806k registrazioni licenziate da AudioSparx +
  ~472k da Freesound (solo CC-0, CC-BY, CC-Sampling+), con filtro anti-copyright
  documentato. Le attribuzioni Freesound sono pubblicate da Stability:
  https://info.stability.ai/attributions — da citare nel CREDITS.txt se l'audio generato
  finisce nel gioco pubblicato.
