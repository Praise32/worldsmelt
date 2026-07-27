# assets/curated/ — pacchetto immagini curate della demo

Contenuto curato provvisorio della demo (DEC-171, `docs/design/governance/decision-log.md`):
finché la Style LoRA di SD1.5 non è addestrata (DEC-148), gli sprite di oggetti, nemici,
boss e prop della demo sono immagini **selezionate e copiate** dal corpus CC0 già
registrato in `dataset-raw/` — non generate. Nessun modello immagine gira per produrre
questo pacchetto.

## Provenienza e licenza

Tutte le immagini vengono da due famiglie di pacchetti **CC0-1.0** già presenti in
`dataset-raw/` e già registrate nel ledger di provenienza:

- pacchetti **Kenney** (kenney-tiny-dungeon, kenney-micro-roguelike, kenney-pixel-shmup,
  kenney-top-down-shooter) — CC0, vedi `dataset-raw/kenney-*/extracted/License.txt`;
- **superpowers-asset-packs** — CC0-1.0, vedi
  `dataset-raw/superpowers-asset-packs/LICENSE.txt` e `README.md`.

Il registro di provenienza per-file è `docs/ai-production/dataset/ledger.jsonl` (uno
`sha256` + licenza per ogni file di `dataset-raw/`), verificabile con:

```
python3 scripts/dataset_ledger.py check
```

Ogni riga di `manifest.json` porta un campo `"source"` nella forma
`<pacchetto>/<file-di-origine-relativo-a-dataset-raw>`: è sempre possibile risalire
dall'immagine curata al file originale e da lì alla riga del ledger. Non è stato aggiunto
nessun file nuovo al ledger: tutte le sorgenti usate qui erano già registrate (verificato
in fase di build, 189/189 presenti).

Fonte di design: `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md` (DEC-171) e
`docs/ai-production/04-DATASET-LICENZE.md`.

## Come si rigenera

```
python3 scripts/curated-pack.py            # ricostruisce assets/curated/{items,enemies,bosses,props}
python3 scripts/curated-pack.py --dry-run   # solo conteggi, non scrive nulla
```

Lo script ripulisce e ricrea le quattro cartelle di categoria a ogni esecuzione: non è
incrementale, la lista di selezione vive interamente in `scripts/curated-pack.py`
(tabelle `ITEMS` / `ENEMIES` / `BOSSES` / `PROPS`). Per aggiungere o togliere uno sprite si
modifica quella tabella e si rilancia lo script.

### Unica trasformazione ammessa: ritaglio del primo fotogramma

La curation qui dentro è selezione + copia integrale, con una sola eccezione dichiarata:
8 sprite di `rpg-battle-system/monster/*` (bat, dino, ghost, slime, dragon, mushroom,
reptile, snake) esistono nel corpus solo come **sprite-sheet multi-fotogramma** (il nome
del file incorpora la dimensione del singolo fotogramma, es.
`sprite-sheet-258x209.png` su un foglio 1290×1045 = griglia 5×5). Per questi otto lo
script ritaglia il **primo fotogramma** (in alto a sinistra, la posa "idle" per
convenzione dello stesso autore, che fornisce già così boar/chest/giant/octopus/yeti come
singoli). È un ritaglio meccanico di pixel già esistenti, non contenuto nuovo: il campo
`"transform"` del manifest lo dichiara esplicitamente per ogni voce interessata. Nessun
altro file in questo pacchetto è stato modificato: tutti gli altri sono copie byte-per-byte
dell'originale.

## Struttura

```
assets/curated/
├── items/     85 sprite (pozioni, armi, gemme, gadget...)
├── enemies/   49 sprite (nemici comuni)
├── bosses/    15 sprite (boss o mini-boss, sprite più grandi o dal ruolo climatico)
├── props/     40 sprite (ostacoli/decorazioni di stanza: casse, barili, tombe, alberi...)
└── manifest.json
```

`manifest.json` contiene, per ogni immagine: `id`, `file`, `category`
(`item|enemy|boss|prop`), `tags`, `size` ([w, h] in pixel), `source` e, quando applicabile,
`transform`. I `tags` derivano dal nome del file di origine e dal giudizio visivo di
curation (contact sheet in `logs/curated-pack/`, vedi sotto): servono al matching con il
contenuto generato (es. un oggetto "potion"+"red"+"health" generato a runtime può cercare
un'immagine curata con quei tag come riferimento/fallback).

### Pool "riserva fusione" (`tags` contiene `fusion-reserve`)

Il compito chiedeva anche ~10 immagini dedicate alle fusioni. DEC-171 stabilisce che
l'immagine di un oggetto nato da fusione si pesca dal dataset **generale** (immagini non
ancora usate nella run corrente), non da un sotto-pacchetto dedicato — quindi qui non c'è
una quinta cartella. Per dare comunque un punto di partenza dichiaratamente "neutro" (un
oggetto che *sembra* già una fusione di più concetti, non uno specifico oggetto esistente),
10 immagini in `items/` portano il tag `fusion-reserve`: le 8 icone elementali astratte di
`medieval-fantasy/items/element/` (fuoco, acqua, fulmine, terra, natura, vento, luce,
oscurità) più due orbi generici di `space-shooter/items/` (`bubble-shield`, `orb-red-ball`).
Sono scelte deliberate di chi cura, non un requisito del dataset.

## Verifica visiva a campione

Contact sheet generati dal contenuto **effettivamente copiato** (non dalle sorgenti) in
`logs/curated-pack/contact-{items,enemies,bosses,props}.png`, guardati uno per uno prima di
chiudere il lavoro. Esito: uno scarto (4 "teste mostro" 8×8 di
`kenney-micro-roguelike`, illeggibili una volta ingrandite: si leggono come macchie a croce,
non come creature) e una correzione (due file di `medieval-fantasy/background-elements`
erano stati scambiati per rilievi murali "orc guardian" quando in realtà sono alberi; i
volti da gargoyle corretti vengono invece da `kenney-tiny-dungeon/Tiles/tile_0019.png` e
`tile_0020.png`). Dettaglio in `questions-night.md` del run che ha prodotto questo
pacchetto.

## Budget dimensione

Contenuto effettivo copiato: **~0.47 MiB** (189 file PNG + manifest), ben sotto il tetto di
5 MB. Nessuna immagine è stata ridimensionata: le dimensioni originali variano da 6×10 px
(un'icona di ninja-adventure) a 260×143 px (`boss-giant`), coerente con l'eterogeneità dei
pacchetti sorgente — non è stata forzata un'unica risoluzione.
