#!/usr/bin/env bash
# RunBundle v1 (roadmap 16/07/2026, settimana 4 anticipata): impacchetta
# generated/ (o una cartella equivalente, vedi l'argomento sotto) in un
# tar.gz autocontenuto con verifica d'integrita' -- serve a condividere una
# run fra macchine, allegarla a un bug per QA riproducibile e, in futuro, alla
# daily challenge. generated/ e' gia' un bundle de-facto (current_run.json col
# seed, current_run.txt, scripts/*.lua, atlas, scritture atomiche): qui si
# aggiunge solo l'incarto e l'hash, non si cambia il formato dei file dentro.
#
# Uso:      scripts/bundle-export.sh [srcDir]     (default: generated)
# Output:   bundles/melting-bundle-seed<seed>-<hash8>.tar.gz
#
# Dentro il tar.gz: tutti i file di srcDir ESCLUSO gen_progress.txt (stato
# effimero della barra di avanzamento, vedi GenProgressWrite in
# tools/melting-gen/gen_util.c: non fa parte della run) + un
# BUNDLE_MANIFEST.txt con bundleSchema, una riga sha256sum per OGNI file del
# bundle e bundleHash = sha256 della lista ordinata di quelle righe.
# <hash8> nel nome file sono i primi 8 esadecimali di bundleHash.
#
# Rifiuta di esportare se srcDir manca dei file essenziali di una run
# giocabile (current_run.txt, current_run.json) o della provenienza
# (provenance.txt, scritta da tools/melting-gen a fine generazione normale/
# fallback -- vedi GenWriteProvenance): un bundle senza provenienza non si
# potrebbe mai far risalire a "con quale seed/modello/prompt e' nato".
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$PWD"

# LC_ALL=C: l'ordine del sort qui sotto decide l'ordine delle righe di
# BUNDLE_MANIFEST.txt e quindi bundleHash. Con la locale di sistema
# (es. it_IT.UTF-8) due nomi che differiscono per maiuscola/minuscola si
# ordinano in modo OPPOSTO rispetto a C: lo stesso contenuto esportato su due
# macchine con locale diverse produrrebbe due bundleHash diversi -- trovato
# dalla verifica adversariale con controesempio riproducibile. Stessa
# disciplina byte-wise gia' usata dal lato C (GenPromptsFnv usa strcmp).
export LC_ALL=C

SRC="${1:-generated}"

[ -d "$SRC" ] || { echo "bundle-export: cartella sorgente mancante: $SRC" >&2; exit 1; }
for req in current_run.txt current_run.json provenance.txt; do
  [ -f "$SRC/$req" ] || {
    echo "bundle-export: $SRC/$req mancante -- non esporto un bundle incompleto" >&2
    exit 1
  }
done

# Copia in una cartella di lavoro temporanea, MAI dentro srcDir: srcDir puo'
# essere la vera generated/ (letta/scritta dal gioco e da melting-gen in
# sottofondo durante --resume), e questo script non deve lasciarci ne'
# toccarci nulla.
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
PAYLOAD="$STAGE/payload"
mkdir -p "$PAYLOAD"

# Elenco file (percorsi relativi a srcDir), ESCLUSO gen_progress.txt, in
# ordine alfabetico: l'ordine e' quello che finisce, di riga in riga, dentro
# BUNDLE_MANIFEST.txt, e deve restare lo stesso a ogni esportazione dello
# stesso contenuto perche' bundleHash sia deterministico. -print0/mapfile -d
# per restare corretti anche su nomi con spazi (nessuno oggi, ma e' gratis).
mapfile -d '' -t files < <(cd "$SRC" && find . -type f ! -name gen_progress.txt -print0 | sort -z)
if [ "${#files[@]}" -eq 0 ]; then
  echo "bundle-export: $SRC non contiene nessun file da esportare" >&2
  exit 1
fi

for rel in "${files[@]}"; do
  rel="${rel#./}"
  mkdir -p "$PAYLOAD/$(dirname "$rel")"
  cp -p "$SRC/$rel" "$PAYLOAD/$rel"
done

cd "$PAYLOAD"
rels=()
for rel in "${files[@]}"; do rels+=("${rel#./}"); done

# Una riga sha256sum per file (formato "<hash>  <percorso>", lo stesso che
# `sha256sum -c` sa verificare direttamente: bundle-import.sh lo sfrutta).
hashLines=$(sha256sum -- "${rels[@]}")

# bundleHash = sha256 della lista ORDINATA delle righe sha256 sopra (non dei
# singoli file uno per uno): un file spostato, rinominato o mancante cambia
# l'insieme delle righe e quindi bundleHash, non solo il contenuto.
bundleHash=$(printf '%s\n' "$hashLines" | sha256sum | cut -d' ' -f1)
hash8="${bundleHash:0:8}"

{
  echo "bundleSchema=1"
  printf '%s\n' "$hashLines"
  echo "bundleHash=$bundleHash"
} > BUNDLE_MANIFEST.txt

seed=$(sed -n 's/^seed=//p' provenance.txt | head -1)
[ -n "$seed" ] || { echo "bundle-export: provenance.txt senza riga seed=" >&2; exit 1; }

mkdir -p "$ROOT/bundles"
outFile="$ROOT/bundles/melting-bundle-seed${seed}-${hash8}.tar.gz"

# --sort=name/--owner/--group/mtime fissi: due esportazioni dello STESSO
# contenuto payload devono produrre lo STESSO tar.gz byte-a-byte (utile per i
# test e per non far scattare falsi diff quando un bundle finisce versionato
# a mano da qualche parte). GNU tar: se non disponibile su un'altra
# piattaforma questo script resta comunque corretto, solo non bit-a-bit
# riproducibile.
tar --sort=name --owner=0 --group=0 --numeric-owner --mtime='@0' \
    -czf "$outFile" -- "${rels[@]}" BUNDLE_MANIFEST.txt

echo "bundle-export: scritto $outFile"
