# 05 — Dataset e licenze

## Risposta breve

Per raggiungere una qualità vicina a Retro Diffusion non basta scaricare “un dataset pixel art”. Serve un corpus coerente, annotato per ruolo e costruito con provenienza dimostrabile.

Ordine consigliato:

1. asset propri o commissionati;
2. raccolte CC0 da fonti ufficiali;
3. immagini sintetiche procedurali create dal progetto;
4. singoli asset con licenze diverse soltanto dopo audit;
5. nessuno scraping di gallerie o sprite di giochi.

Questa nota è una strategia prudenziale, non un parere legale.

## Il dataset migliore: quello del progetto

Il nucleo dello stile dovrebbe essere creato o supervisionato da un pixel artist. Nel contratto di commissione vanno autorizzati esplicitamente:

- training e fine-tuning;
- uso commerciale degli output;
- creazione di opere derivate;
- distribuzione delle LoRA o dei pesi, se prevista;
- trasformazioni, crop, palette e annotazioni;
- esclusione di personaggi, marchi o asset di terzi.

“Royalty-free” o “commercial use” non significano automaticamente “utilizzabile per addestrare un modello”.

## Fonti CC0 ad alta priorità

### Kenney

La [FAQ ufficiale Kenney](https://kenney.nl/support) dichiara CC0 gli asset pubblicati nelle pagine asset, utilizzabili anche commercialmente senza attribuzione obbligatoria. Il logo Kenney va escluso.

Pack utili:

- [Tiny Dungeon](https://kenney.nl/assets/tiny-dungeon): forme dungeon 16×16;
- [Micro Roguelike](https://kenney.nl/assets/micro-roguelike): forme molto piccole;
- [1-Bit Pack](https://kenney.nl/assets/1-bit-pack): silhouette e leggibilità;
- [Pixel UI Pack](https://kenney.nl/assets/pixel-ui-pack): icone e UI;
- [Top-down Shooter](https://kenney.nl/assets/top-down-shooter): top-down e proiettili;
- [Pixel Shmup](https://kenney.nl/assets/pixel-shmup): proiettili, VFX e pattern.

Il numero di file non è il numero di concetti indipendenti. Varianti colore e frame adiacenti devono essere deduplicati o raggruppati.

### Urizen 1Bit

[Urizen 1Bit Tileset](https://vurmux.itch.io/urizen-onebit-tileset) è dichiarato CC0 e contiene migliaia di tile. È utile per categorie e forme, non per insegnare colore e shading perché è monocromatico.

### Ninja Adventure

[Ninja Adventure](https://pixel-boy.itch.io/ninja-adventure-asset-pack) è dichiarato CC0 e include personaggi, mostri, boss, oggetti, UI ed effetti. È particolarmente utile per:

- cicli top-down;
- coerenza fra personaggio e attacco;
- VFX;
- frame di animazione.

### Superpowers Asset Packs

Il repository [Superpowers Asset Packs](https://github.com/sparklinlabs/superpowers-asset-packs) usa CC0-1.0 e raccoglie pack di più generi. Può sovrapporsi ad altre raccolte: usare SHA-256 e perceptual hash.

### OpenDuelyst

[OpenDuelyst](https://github.com/open-duelyst/duelyst) è un grande corpus CC0 di unità animate, icone ed effetti. Tenerlo separato dal nucleo stilistico e rimuovere:

- logo;
- nome del gioco;
- testi;
- stemmi;
- elementi identificativi.

CC0 riguarda il copyright dichiarato, non concede diritti su marchi.

## OpenGameArt: utile soltanto per singolo asset

OpenGameArt raccoglie licenze diverse. La sua [FAQ ufficiale](https://opengameart.org/node/5571) avverte anche che le immagini di anteprima possono non avere la stessa licenza dei file scaricabili.

Regola:

1. aprire la pagina del singolo asset;
2. verificare licenza, autore e file allegato;
3. scaricare l’archivio, non la preview;
4. conservare una copia datata della pagina/licenza;
5. inserire l’asset nel registro;
6. escluderlo se pagina, archivio e metadati non concordano.

Un mirror “OpenGameArt-CC0” può essere un indice, non una prova sufficiente per addestrare.

## Hugging Face e Kaggle

La [documentazione delle Dataset Card di Hugging Face](https://huggingface.co/docs/hub/datasets-cards) permette di dichiarare licenza, origine e limiti. La presenza di una card, però, non dimostra che ogni immagine sia stata raccolta correttamente.

Accettare un dataset soltanto se:

- la licenza è esplicita;
- la provenienza per immagine è disponibile;
- il creatore del dataset aveva il diritto di redistribuire i file;
- la licenza permette l’uso commerciale previsto;
- non contiene fan art, screenshot o sprite estratti;
- si può ricostruire un manifest.

Un tag “license: cc0” senza provenienza per file non è sufficiente per il corpus principale.

## Fonti da evitare

- The Spriters Resource e siti simili;
- ROM e sprite estratti;
- screenshot e wiki di videogiochi;
- asset di The Binding of Isaac;
- fan art;
- Pinterest e Google Images;
- ArtStation, DeviantArt, PixelJoint e gallerie Lospec;
- pack Unity Asset Store usati come training set;
- CC-NC per un prodotto commerciale;
- CC-ND;
- CC-BY-SA in un modello proprietario senza valutazione legale;
- raccolte con licenze contraddittorie;
- output Retro Diffusion come dataset senza autorizzazione scritta.

Il sito Retro Diffusion afferma che gli output possono essere usati commercialmente, ma “posso usare l’immagine nel mio gioco” non equivale automaticamente a “posso usarla per addestrare un sistema concorrente”. Per il training serve una conferma contrattuale specifica; i [termini ufficiali](https://www.retrodiffusion.ai/assets/terms-8312c717.pdf) vanno controllati alla data dell’uso.

## Registro obbligatorio

Per ogni file:

    asset_id
    sha256
    perceptual_hash
    original_url
    archive_url
    author
    license_id
    license_url
    license_snapshot_date
    source_pack
    role
    dimensions
    animation_group
    exclusions
    transformations
    caption
    split
    reviewer

Conservare anche la licenza accanto al dataset, non soltanto il link.

## Struttura del dataset visivo

### Style Core

Insegna:

- spessore dei contorni;
- palette;
- shading;
- direzione della luce;
- livello di dettaglio;
- prospettiva top-down;
- proporzioni.

### Item

Annotazioni:

- pool;
- rarità;
- forma;
- materiale;
- funzione semantica;
- silhouette;
- palette;
- vista;
- presenza di sfondo.

### Projectile e VFX

Annotazioni:

- corpo;
- superficie;
- movimento;
- trail;
- impatto;
- elemento;
- fase dell’animazione;
- direzione.

### Player Mutation

Annotazioni:

- regione del corpo;
- socket;
- tipo di morph;
- tratto meccanico sorgente;
- intensità;
- compatibilità con frame e direzione;
- maschera alpha.

### Animazione

Non trattare i frame come immagini indipendenti. Conservare:

- animation_id;
- ordine;
- durata del frame;
- azione;
- direzione;
- pivot;
- socket;
- contatto a terra;
- evento di attacco.

## Quante immagini

Stime di progetto, non soglie scientifiche:

| Obiettivo | Quantità iniziale utile |
|---|---:|
| Smoke test di una LoRA | 50–100 immagini molto pulite |
| LoRA di ruolo v0 | 100–300 esempi diversi |
| Proof of concept complessiva | 300–800 asset puliti |
| Style Core più robusto | 500–1.500 immagini curate |
| Ruolo produttivo ampio | 500–2.000 esempi per famiglia |
| Animazione coerente | centinaia di sequenze, non frame mescolati |

La diversità indipendente conta più del totale. Dieci animazioni da otto frame non equivalgono a ottanta soggetti.

## Split corretto

Non dividere casualmente i frame della stessa animazione fra train e validation. Separare per:

- autore o pack;
- personaggio;
- animation_id;
- famiglia visiva.

Altrimenti la validation misura memoria quasi identica, non generalizzazione.

## Dataset generato dal gioco

Per Qwen, il corpus più importante nasce durante lo sviluppo:

- richiesta;
- JSON proposto;
- grafo normalizzato;
- correzioni;
- errori;
- risultato della simulazione;
- valutazione umana.

Per SD, conservare:

- prompt strutturato;
- seed;
- checkpoint e LoRA;
- parametri;
- output grezzo;
- output ripulito;
- motivo di accettazione/rifiuto;
- confronto in-engine.

Non usare automaticamente gli output accettati per riaddestrare: prima vanno revisionati, deduplicati e separati dagli asset di origine.

