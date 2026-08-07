---
id: aiprod-retro-diffusion-letter
title: Lettera ad Astropulse — permesso scritto per il dataset LoRA
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Lettera pronta da inviare ad Astropulse (Retro Diffusion) per chiedere per iscritto il
  permesso di usare una quota curata di output Cloud come dataset per la Worldsmelt LoRA
  (base DreamShaper 8/SD1.5, non il checkpoint Retro Diffusion) e la conferma dei diritti
  di uso commerciale degli output gia' presenti nel gioco, piu' un terzo punto facoltativo
  (preventivo per un modello su commissione, punto 4 dell'ordine di lavoro del dossier).
  Adattata al piano operativo RD-PREP del 07/08/2026: benchmark/asset curati via Cloud,
  LoRA interna non concorrente.
last_reviewed: 2026-08-07
topics: [retro-diffusion, licenza, dataset, lora, astropulse, permesso-scritto]
related: [aiprod-dataset-readme]
supersedes: []
source_files: [scripts/rd_dataset_gen.py, docs/ai-production/dataset/rd-dataset-plan.json]
---

# Lettera ad Astropulse — permesso scritto per il dataset LoRA

## Contesto (per chi la invia)

Punto di partenza: il dossier del proprietario del 04/08/2026 (strategia Retro Diffusion,
riassunto in `02-STACK-MODELLI.md`/decisione del 04/08 archiviata insieme a questo
lavoro) e il censimento tecnico verificato il 07/08/2026 su
`github.com/Retro-Diffusion/api-examples` e sul ToS PDF del 19/08/2025.

**Perche' questa lettera e non quella originale del dossier.** La bozza del 04/08
chiedeva permesso per un ventaglio ampio di usi del *checkpoint* Retro Diffusion
(caricarlo fuori da Aseprite, farci LoRA/fine-tuning, distillarlo LCM/Hyper-SD,
quantizzarlo, ospitarlo, distribuirlo col gioco). Da quando la strategia si e' chiusa su
DreamShaper 8/SD1.5 come base della Worldsmelt LoRA (`03-PIANO-DREAMSHAPER-WORLDSMELT.md`
del dossier), **le domande 1-7 non servono piu'**: non useremo mai il modello o il
checkpoint di Retro Diffusion come base, quindi non c'e' nulla da chiedere su
caricamento/distillazione/quantizzazione/hosting di QUEL modello.

**Corrispondenza esplicita con le 10 domande del dossier**, cosi' che nessuna cada in
silenzio (una prima stesura di questa lettera ne lasciava fuori due senza dirlo):

| Dossier | Esito qui | Perche' |
|---|---|---|
| 1-7 (caricare/allenare/distillare/quantizzare/distribuire/ospitare il checkpoint RD) | **ritirate** | la base della LoRA e' DreamShaper 8/SD1.5, il checkpoint RD non entra mai nella pipeline |
| 8 (output come dati di training di un modello separato) | **punto 2** della lettera | e' l'uso che il piano attuale fa davvero |
| 9 (modello Worldsmelt su commissione, con diritti di modifica/distillazione/distribuzione/hosting) | **punto 3** della lettera | e' il punto 4 dell'ordine di lavoro del dossier ("chiedere preventivo per modello personalizzato"): non si puo' rispondere da soli, e' una domanda di prezzo |
| 10 (codice di training, configurazione, provenienza del dataset) | **punto 3** della lettera | ha senso solo insieme alla 9: un modello commissionato senza provenienza del dataset non e' verificabile |

Il punto 3 e' l'unico **facoltativo**: apre una trattativa commerciale che oggi non e'
finanziata (`RD_API_KEY` non e' ancora arrivata, nessun credito speso). Chi invia puo'
cancellarlo senza toccare il resto — i punti 1 e 2 restano una lettera completa. Va
cancellato **consapevolmente**, non dimenticato: e' il motivo per cui e' scritto qui.

Le tre cose che questa versione chiede:

1. **Conferma scritta dei diritti sugli output** che `scripts/rd_dataset_gen.py` gia'
   genera come asset **curati** (tier Plus/Pro) ed embedda staticamente nel gioco --
   probabilmente gia' coperto dai ToS pubblici (verdetto sotto), ma vale la pena
   ottenerlo per iscritto prima di spendere credito reale.
2. **Permesso esplicito per l'uso come dataset di training**: una quota limitata e
   curata di output Cloud (mai la maggioranza del dataset, vedi
   `docs/ai-production/dataset/README.md` regola d'oro 1) usata per addestrare la
   **Worldsmelt LoRA**, un modello **separato**, su base **DreamShaper 8/SD1.5**, **non
   concorrente** (mai offerto come servizio di generazione a terzi: gira solo dentro il
   gioco, offline, per produrre varianti di contenuto del gioco stesso a runtime).
3. **Preventivo per un modello Worldsmelt su commissione** (facoltativo): costo e
   condizioni di un modello addestrato per il progetto, con diritti di modifica,
   distillazione, quantizzazione, distribuzione e hosting, e con codice di training,
   configurazione e provenienza del dataset. E' una domanda di **prezzo e condizioni**,
   non di permesso: nessun ToS la copre, e la motivazione dei punti 1-2 (uso non
   concorrente di output gia' pagati) non vale qui.

**Verdetto ToS verificato il 07/08** (dettagli in
`docs/ai-production/experiments/teacher-bench-2026-08-06.md`, sezione "Percorso Retro
Diffusion"): il ToS pubblico afferma la piena proprieta' degli output da parte
dell'utente e non menziona esplicitamente il training su di essi -- **favorevole**, ma
un'assenza di divieto non e' un permesso esplicito. Da qui la lettera, non un'assunzione.

**Dove inviarla** (verificato il 07/08 sulla pagina ufficiale
`astropulse.itch.io/retrodiffusion`, sezione contatti):

- **Discord** (canale preferito dallo sviluppatore stesso: *"The best place to reach me
  is by joining the Retro Diffusion Discord server"*) -- `https://discord.gg/retrodiffusion`.
  Piu' veloce, ma la risposta puo' arrivare come messaggio informale.
- **Email** -- `support@retrodiffusion.com`. Piu' lenta, ma lascia una traccia scritta
  formale: **preferibile per QUESTA lettera**, visto che l'oggetto e' proprio "conferma
  scritta". Se dopo qualche giorno non arriva risposta, il Discord e' il modo giusto per
  sollecitare.

Placeholder da compilare prima dell'invio: `[NOME]`, `[STUDIO/PROGETTO]`, `[EMAIL DI
RISPOSTA]`. Il nome del gioco (Worldsmelt) e' gia' nel testo: e' il nome pubblico del
prodotto, non un dato riservato.

---

## English (send this version)

**Subject: Written permission request — training data use of Retro Diffusion Cloud outputs (Worldsmelt)**

Hello,

I'm [NOME], developing *Worldsmelt*, a commercial game ([STUDIO/PROGETTO]). I'm using
the Retro Diffusion Cloud API to generate curated, pre-rendered sprite assets that ship
statically inside the game (no runtime generation, no end-user access to your API or
models). Separately, I'm training a small in-house LoRA on my own dataset, on top of
DreamShaper 8 / Stable Diffusion 1.5 (not your model or checkpoint), to run fully
offline inside the game for generating game-content variations at runtime. I read your
public Terms of Service (the version dated 2025-08-19) and want to confirm a few
specific points in writing before scaling up either use:

1. **Output ownership / commercial use** — Can you confirm that images I generate
   through the Cloud API under my account, and ship as static assets inside a
   commercial game, are owned by me and free to use commercially, with no separate
   license or attribution required beyond your standard terms?

2. **Training data use (the one point your public ToS doesn't address explicitly)** —
   May I use a limited, curated subset of images I generate through the Cloud API as
   training data for the small in-house LoRA described above? To be clear about what
   this is *not*: it is not a checkpoint or fine-tune of your model, it is not offered
   as a generation service to anyone, and Retro Diffusion outputs would never be more
   than a minority slice of a mixed dataset (the rest being original art and verified
   CC0 sources). If useful, I'm happy to share the exact intended proportion once I have
   a training run scheduled.

3. **Commissioned model — a quote, if this is something you do at all** — Would you
   quote a Worldsmelt-specific model trained by you for this project? What I would need
   it to include: the right to modify, distil, quantise, ship it with the game and host
   it on my own backend, plus the training code, configuration and dataset provenance.
   This is a commercial question rather than a licensing one, and "we don't do that" is
   a perfectly useful answer — it just isn't something I can work out on my own.

If it's easier, a short "yes" or "no" per point is genuinely enough — I mainly need
something in writing I can keep on file. If any answer is "it depends," I'd
appreciate knowing what it depends on (volume caps, attribution, a specific tier, a
separate agreement) so I can plan around it.

Thank you for your time, and for building a tool this useful for small teams.

[NOME]
[EMAIL DI RISPOSTA]

---

## Italiano (traduzione di cortesia / riferimento interno)

**Oggetto: richiesta di permesso scritto — uso come dati di training degli output di Retro Diffusion Cloud (Worldsmelt)**

Salve,

sono [NOME], sto sviluppando *Worldsmelt*, un gioco commerciale ([STUDIO/PROGETTO]). Uso
l'API Retro Diffusion Cloud per generare asset sprite curati e pre-renderizzati, che
entrano nel gioco come file statici (nessuna generazione a runtime, nessun accesso
dell'utente finale alla vostra API o ai vostri modelli). Separatamente, sto addestrando
una piccola LoRA interna sul mio dataset, sopra DreamShaper 8 / Stable Diffusion 1.5 (non
il vostro modello o checkpoint), per girare completamente offline dentro il gioco e
generare varianti di contenuto a runtime. Ho letto i vostri Termini di Servizio pubblici
(versione datata 19/08/2025) e vorrei chiarire per iscritto alcuni punti specifici prima
di intensificare l'uno o l'altro uso:

1. **Proprieta' degli output / uso commerciale** — Potete confermare che le immagini che
   genero tramite la Cloud API col mio account, e che distribuisco come asset statici
   dentro un gioco commerciale, sono di mia proprieta' e libere da usare commercialmente,
   senza licenza o attribuzione separate oltre ai vostri termini standard?

2. **Uso come dati di training (l'unico punto che i ToS pubblici non trattano
   esplicitamente)** — Posso usare un sottoinsieme limitato e curato delle immagini che
   genero tramite la Cloud API come dati di training per la piccola LoRA interna
   descritta sopra? Per chiarezza su cosa NON e': non e' un checkpoint ne' un
   fine-tuning del vostro modello, non viene offerta come servizio di generazione a
   nessuno, e gli output di Retro Diffusion non supererebbero mai una quota minoritaria
   di un dataset misto (il resto e' arte originale e fonti CC0 verificate). Se utile,
   sono disponibile a condividere la proporzione esatta prevista non appena avro' un
   training programmato.

3. **Modello su commissione — un preventivo, se e' una cosa che fate** — Fareste un
   preventivo per un modello specifico per Worldsmelt, addestrato da voi per questo
   progetto? Cosa dovrebbe comprendere: il diritto di modificarlo, distillarlo,
   quantizzarlo, distribuirlo col gioco e ospitarlo su un backend mio, piu' codice di
   training, configurazione e provenienza del dataset. E' una domanda commerciale, non di
   licenza, e "non lo facciamo" e' una risposta perfettamente utile — semplicemente non e'
   qualcosa che possa decidere da solo.

Se e' piu' comodo, basta davvero un "si" o un "no" per punto — mi serve principalmente
qualcosa per iscritto da conservare. Se una risposta e' "dipende", apprezzerei sapere da
cosa dipende (limiti di volume, attribuzione, un tier specifico, un accordo a parte) cosi'
posso pianificare di conseguenza.

Grazie per il tempo, e per aver costruito uno strumento cosi' utile per i team piccoli.

[NOME]
[EMAIL DI RISPOSTA]
