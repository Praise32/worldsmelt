---
id: aiprod-dataset-licenze
title: Dataset e licenze
domain: ai-production
status: proposed
authority: supporting
owner: ai-production
summary: >-
  Strategia prudente su provenienza del dataset Kaggle 89k immagini, separazione research/commercial, ledger minimo e obblighi di licenza di SD1.5/Qwen/Pixel Art Fixer.
last_reviewed: 2026-07-22
topics: [dataset, licenze, kaggle, provenienza, openrail-m]
related: []
supersedes: []
source_files: []
---
# Dataset e licenze

Questo documento è una strategia tecnica prudente, non un parere legale.

## Dataset Kaggle `ebrahimelgazar/pixel-art`

La pagina dichiara:

- circa 89.000 immagini;
- personaggi e oggetti pixel-art;
- immagini raccolte originariamente da un "online game";
- licenza indicata sulla pagina Kaggle.

Il problema è la catena dei diritti: la pagina non identifica chiaramente il gioco, gli
autori originali o l'autorizzazione a rilicenziare gli sprite.

## Decisione operativa

Usi accettabili:

- imparare il training;
- smoke test;
- ricerca interna;
- confronto di iperparametri;
- prototipo non distribuito;
- studio con provenienza dichiarata.

Uso sconsigliato:

- modello principale del ramo commerciale;
- fusione irreversibile con dataset pulito;
- distribuzione dei pesi come "commercial-safe";
- dichiarazione che la licenza Kaggle risolve la provenienza.

## Separazione obbligatoria

```text
datasets/
├── research-unknown-provenance/
└── commercial-clean/

models/
├── research/
└── commercial/
```

Non promuovere automaticamente un peso da `research` a `commercial`.

## Dataset commerciale

Priorità:

1. asset creati direttamente;
2. asset commissionati con accordo scritto;
3. CC0 verificato;
4. licenza permissiva che consente modifiche, training e uso commerciale;
5. output generati e revisionati, ma solo dopo aver verificato la base e aver escluso
   duplicati/memorizzazione.

## Ledger minimo

Ogni file deve registrare:

```json
{
  "asset_id": "sha256...",
  "path": "dataset/style/image_001.png",
  "source_url": "...",
  "author": "...",
  "source_pack": "...",
  "license": "CC0-1.0",
  "license_url": "...",
  "downloaded_at": "2026-07-20",
  "allowed_commercial": true,
  "allowed_derivatives": true,
  "training_allowed_explicit": true,
  "transformations": ["crop", "nearest_upscale"],
  "split_group": "character_knight_01",
  "notes": ""
}
```

## Regole tecniche

- PNG o formato lossless.
- Alpha conservato.
- Upscale nearest-neighbor.
- Nessun bilinear.
- Deduplicazione percettiva.
- Ispezione di sprite sheet e recolor.
- Caption verificata.
- Gruppi di split.
- Hash del file originale e trasformato.
- Copia locale della licenza o snapshot della pagina.

## Stable Diffusion 1.5

La base dichiara CreativeML OpenRAIL-M.

Implicazioni operative:

- uso commerciale possibile rispettando le restrizioni;
- non descriverla come licenza MIT/Apache;
- quando si distribuiscono pesi o derivati bisogna conservare gli obblighi applicabili;
- il licenziante non assegna automaticamente diritti su eventuali marchi, personaggi o
  contenuti di terzi;
- gli output devono essere controllati per somiglianze e contenuti non desiderati.

## Qwen

Qwen2.5-Coder-7B-Instruct dichiara Apache 2.0. Conservare LICENSE e NOTICE pertinenti nel
pacchetto che distribuisce il modello o il downloader.

## Pixel Art Fixer

Il repository open source dichiara MIT. È elaborazione classica e non richiede un modello.

La versione neurale descritta dal sito è un servizio separato ed esclusivo del sito; non
va confusa con il codice MIT.

## Checklist pre-release

- [ ] lista completa di modelli e LoRA;
- [ ] hash dei pesi;
- [ ] licenza di ogni modello;
- [ ] provenance del dataset;
- [ ] NOTICE;
- [ ] controllo somiglianze;
- [ ] contenuti vietati filtrati;
- [ ] termini di distribuzione dei modelli;
- [ ] opzione solo-curato;
- [ ] documentazione del downloader;
- [ ] revisione legale prima della vendita.
