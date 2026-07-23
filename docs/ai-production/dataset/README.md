---
id: aiprod-dataset-readme
title: Dataset — regole d'oro e registro di provenienza
domain: ai-production
status: approved
authority: canonical
owner: ai-production
summary: >-
  Regole del dataset per le LoRA (solo CC0 verificate, asset propri o commissioni con
  cessione chiara), fonti candidate e registro di provenienza ledger.jsonl gestito da
  scripts/dataset_ledger.py.
last_reviewed: 2026-07-22
last_verified_commit: fe27f6d
topics: [dataset, licenze, provenienza, ledger, cc0]
related: []
supersedes: []
source_files: [scripts/dataset_ledger.py]
---

# Registro di provenienza del dataset

Roadmap 16/07/2026, settimana 2. Prepara il terreno per la futura Style
LoRA (vedi `roguelike-ai-appunti/05-dataset-e-licenze.md` e
`roguelike-ai-appunti/06-training-hardware-costi.md`): prima ancora di
raccogliere un solo file, il progetto ha bisogno di un modo per dimostrare
DA DOVE viene ogni immagine e CON CHE LICENZA puo' essere usata per
addestrare un modello.

## Scopo

Un dataset "vicino a Retro Diffusion" nasce da un corpus coerente,
annotato per ruolo e con provenienza dimostrabile (05, "Risposta breve").
Senza un registro:

- non si puo' dimostrare, in caso di dubbio, che un'immagine era
  davvero CC0/propria al momento dell'uso;
- non si possono deduplicare varianti dello stesso asset finite in
  raccolte diverse (05 lo dice esplicitamente per Superpowers Asset
  Packs e Kenney: "il numero di file non e' il numero di concetti
  indipendenti");
- non si puo' fare uno split train/validation corretto (per pack o
  autore, mai per frame della stessa animazione).

Il registro vive in `docs/ai-production/dataset/ledger.jsonl` (JSON Lines: una riga =
un file), scritto e verificato da `scripts/dataset_ledger.py`. Non e'
un parere legale: e' una misura prudenziale, come la nota 05 da cui
nasce.

## Regole d'oro (da 05-dataset-e-licenze.md)

1. **Ordine di priorita' delle fonti**: asset propri o commissionati
   prima di tutto; poi raccolte CC0 da fonti ufficiali verificate; poi
   immagini sintetiche procedurali del progetto; solo dopo audit,
   singoli asset con licenze diverse. Mai scraping di gallerie o
   sprite di giochi (Spriters Resource, ROM estratte, screenshot, wiki,
   fan art, Pinterest/Google Images, ArtStation/DeviantArt/PixelJoint/
   Lospec, pack Unity Asset Store come training set).
2. **Solo CC0 verificate o asset propri**: una licenza va accettata
   solo se e' esplicita, la provenienza per immagine e' disponibile, chi
   distribuisce il dataset aveva il diritto di farlo, la licenza copre
   l'uso commerciale previsto e non contiene fan art/screenshot/sprite
   estratti. Un tag "license: cc0" senza provenienza per file NON basta
   per il corpus principale (vale anche per Hugging Face/Kaggle). Per
   OpenGameArt: si apre la pagina del singolo asset, si verifica
   licenza/autore/file allegato, si scarica l'archivio (mai la
   preview), si conserva una copia datata della licenza — un mirror
   "OpenGameArt-CC0" e' un indice, non una prova.
3. **Niente output Retro Diffusion senza permesso scritto**: "posso
   usare l'immagine nel mio gioco" non equivale a "posso usarla per
   addestrare un sistema concorrente". Serve una conferma contrattuale
   specifica, non i soli termini d'uso pubblici.
4. **Niente scraping**: nessuna raccolta automatica da gallerie, wiki o
   motori di ricerca immagini. Vale anche per CC-NC (non commerciale,
   incompatibile con un prodotto commerciale), CC-ND e CC-BY-SA senza
   valutazione legale dedicata, e per qualunque raccolta con licenze
   contraddittorie fra pagina/archivio/metadati.
5. **Dedup con sha256 + perceptual hash**: lo sha256 individua i file
   identici byte per byte (varianti ricaricate, duplicati fra pack);
   il perceptual hash (campo del registro, non ancora calcolato in
   automatico dallo script) serve per le varianti quasi identiche
   (ricolorazioni, crop leggermente diversi) che raccolte come Kenney o
   Superpowers Asset Packs tendono a ripetere fra loro.
6. **Split per pack/autore, MAI per frame**: non dividere a caso i
   frame della stessa animazione fra train e validation, altrimenti la
   validation misura memoria quasi identica invece che
   generalizzazione. Si separa per autore, pack, `animation_group` o
   famiglia visiva.

## Fonti CC0 candidate (da 05-dataset-e-licenze.md)

| Fonte | URL | Cosa insegnerebbe |
|---|---|---|
| Kenney — Tiny Dungeon | https://kenney.nl/assets/tiny-dungeon | Forme dungeon a 16x16: silhouette semplici e leggibili a scala piccola |
| Kenney — Micro Roguelike | https://kenney.nl/assets/micro-roguelike | Forme ancora piu' piccole: leggibilita' al limite della risoluzione |
| Kenney — 1-Bit Pack | https://kenney.nl/assets/1-bit-pack | Silhouette pure e leggibilita' in bianco e nero, senza shading a distrarre |
| Kenney — Pixel Shmup | https://kenney.nl/assets/pixel-shmup | Proiettili, VFX e pattern: il ruolo Projectile/VFX della nota 05 |
| Kenney — Top-down Shooter | https://kenney.nl/assets/top-down-shooter | Prospettiva e composizione top-down, coerenza fra corpo e proiettili |
| Ninja Adventure | https://pixel-boy.itch.io/ninja-adventure-asset-pack | Personaggi, mostri, boss, oggetti, UI ed effetti: cicli top-down completi, coerenza fra personaggio e attacco, frame di animazione coerenti |
| Urizen 1Bit Tileset | https://vurmux.itch.io/urizen-onebit-tileset | Categorie e forme in scala; e' monocromatico, quindi NON insegna colore ne' shading |
| OpenDuelyst | https://github.com/open-duelyst/duelyst | Corpus ampio di unita' animate, icone ed effetti — da tenere separato dal nucleo stilistico e ripulito da logo/nome del gioco/testi/stemmi prima dell'uso |
| Superpowers Asset Packs | https://github.com/sparklinlabs/superpowers-asset-packs | Pack di piu' generi sotto CC0-1.0; puo' sovrapporsi ad altre raccolte, va deduplicato con sha256+phash |

Kenney dichiara CC0 gli asset pubblicati nelle pagine asset (vedi la
FAQ ufficiale: https://kenney.nl/support), logo escluso. Anche per le
altre fonti la licenza dichiarata va verificata e "fotografata"
(licenza + data) al momento dell'uso, non solo linkata: vedi la
colonna `license_snapshot_date` del registro qui sotto.

## Il registro (`docs/ai-production/dataset/ledger.jsonl`)

Formato JSON Lines: una riga per file, secondo l'elenco di campi della
nota 05 ("Registro obbligatorio"):

    asset_id, sha256, perceptual_hash, original_url, archive_url,
    author, license_id, license_url, license_snapshot_date,
    source_pack, role, dimensions, animation_group, exclusions,
    transformations, caption, split, reviewer

`scripts/dataset_ledger.py` compila in automatico `sha256`,
`dimensions` (solo per PNG, leggendo l'header IHDR) e
`license_snapshot_date`; i campi presi dagli argomenti della riga di
comando sono `original_url`, `author`, `license_id`, `license_url`,
`role` e (opzionale) `notes`. I campi restanti (`perceptual_hash`,
`archive_url`, `source_pack`, `animation_group`, `exclusions`,
`transformations`, `caption`, `split`, `reviewer`) restano `null` fino
a una compilazione manuale o a uno strumento futuro: il registro e'
pensato per crescere, non per essere completo dal primo giorno.

## Uso dello script

Aggiungere un file o una cartella intera (ricorsivo) al registro:

    python3 scripts/dataset_ledger.py add percorso/asset-o-cartella \
      --source-url "https://kenney.nl/assets/tiny-dungeon" \
      --license "CC0" \
      --license-url "https://kenney.nl/support" \
      --author "Kenney" \
      --role "item" \
      --notes "batch 1, solo i tile 16x16 di terreno"

Se un file ha uno sha256 gia' presente nel registro, `add` avvisa e
NON lo riscrive (idempotente: si puo' rilanciare `add` sulla stessa
cartella dopo averci aggiunto altri file senza duplicare le righe
vecchie).

Verificare l'integrita' del registro (duplicati, campi obbligatori
mancanti, licenze fuori whitelist CC0/own/commissioned):

    python3 scripts/dataset_ledger.py check

Contare le voci per ruolo, licenza e fonte:

    python3 scripts/dataset_ledger.py stats

Lo script e' Python 3 standard library, nessuna dipendenza esterna
(niente Pillow: le dimensioni PNG si leggono a mano dal chunk IHDR).
