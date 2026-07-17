#!/usr/bin/env bash
# RunBundle v1 (roadmap 16/07/2026, settimana 4 anticipata): importa un
# tar.gz scritto da scripts/bundle-export.sh, verifica OGNI sha256 elencato
# in BUNDLE_MANIFEST.txt e il bundleHash complessivo, e SOLO se tutto torna
# sostituisce atomicamente la cartella di destinazione (default generated/).
# Qualunque mismatch (un file corrotto, un bundleHash che non torna, un
# manifest mancante o mal formato) e' un messaggio chiaro + exit 1 SENZA
# toccare la destinazione -- ne' scritture parziali, ne' un generated/ a
# meta' sostituito.
#
# Uso:      scripts/bundle-import.sh <bundle.tar.gz> [destDir]
#           (destDir default: generated)
#
# Dopo un import riuscito il gioco deve poter partire senza rigenerare nulla:
# non si tocca il CONTENUTO dei file, solo l'incarto (vedi bundle-export.sh).
set -euo pipefail

usage() { echo "uso: scripts/bundle-import.sh <bundle.tar.gz> [destDir]" >&2; }

if [ $# -lt 1 ]; then usage; exit 1; fi

# Percorsi risolti rispetto alla cwd DI CHI CHIAMA (non a quella del repo:
# questo script, a differenza di bundle-export.sh, non fa 'cd' prima di
# leggere gli argomenti), cosi' "scripts/bundle-import.sh ../foo.tar.gz"
# funziona da qualunque directory di lavoro.
ORIG_PWD="$PWD"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

ARCHIVE_INPUT="$1"
DEST_INPUT="${2:-generated}"

case "$ARCHIVE_INPUT" in
  /*) ARCHIVE="$ARCHIVE_INPUT" ;;
  *)  ARCHIVE="$ORIG_PWD/$ARCHIVE_INPUT" ;;
esac
[ -f "$ARCHIVE" ] || { echo "bundle-import: archivio non trovato: $ARCHIVE_INPUT" >&2; exit 1; }

case "$DEST_INPUT" in
  /*) DEST="$DEST_INPUT" ;;
  *)  DEST="$ORIG_PWD/$DEST_INPUT" ;;
esac

# La cartella di lavoro temporanea vive DENTRO bundles/ (gia' nel .gitignore
# del repo, vedi bundle-export.sh) e non sotto il /tmp di sistema: cosi'
# resta quasi sempre sullo STESSO filesystem di destDir (il caso comune,
# destDir sotto la radice del repo), che e' cio' che serve perche' il 'mv'
# finale sia una rename() atomica invece di una copia+cancellazione. Se
# destDir vive altrove (un altro disco/filesystem) il 'mv' resta corretto ma
# non piu' atomico: e' un limite noto di 'mv' fra filesystem diversi, non di
# questo script.
mkdir -p "$ROOT/bundles"
STAGE=$(mktemp -d "$ROOT/bundles/.import-tmp.XXXXXX")
trap 'rm -rf "$STAGE"' EXIT
EXTRACT="$STAGE/extract"
mkdir -p "$EXTRACT"

# Difesa ESPLICITA contro il path traversal ("../evil.txt" o percorsi
# assoluti fra i membri), PRIMA di estrarre. Il GNU tar installato qui li
# rifiuta gia' di suo in estrazione, ma quella e' una proprieta' del binario
# di sistema, non di questo script: se un domani l'invocazione cambiasse (un
# -P aggiunto per altri motivi) o girasse su un tar/libarchive con default
# diversi, la difesa sparirebbe in silenzio. Trovato dalla verifica
# adversariale: la garanzia va posseduta qui, non presa in prestito.
while IFS= read -r member; do
  case "/$member/" in
    *"/../"*|//*)
      echo "bundle-import: membro con percorso pericoloso nell'archivio: $member -- rifiutato, $DEST NON toccato" >&2
      exit 1 ;;
  esac
done < <(tar -tzf "$ARCHIVE")

tar -xzf "$ARCHIVE" -C "$EXTRACT"

# Difesa contro i membri NON regolari (RunBundle istruzione #1: "un link
# simbolico! ... l'import deve rifiutarli o comunque non scrivere fuori dalla
# dir di destinazione"). La verifica sha256 NON basta da sola: 'sha256sum'
# SEGUE i symlink, quindi un membro `evil -> /etc/hostname` (o un qualunque
# file dal contenuto NOTO/prevedibile sulla macchina bersaglio) supera sia
# 'sha256sum -c' sia il bundleHash aggregato -- l'attaccante calcola l'hash del
# bersaglio in anticipo e lo mette nel manifest. Il symlink finirebbe cosi'
# vivo in destDir, puntando FUORI, e un successivo bundle-export.sh lo
# dereferenzierebbe (cp -p segue i link) impacchettandone il CONTENUTO: una
# catena di exfiltrazione, non solo un file spurio.
#
# Quindi: PRIMA di verificare gli hash e PRIMA di toccare destDir si rifiuta
# QUALUNQUE membro che non sia un file regolare o una directory -- symlink,
# device, fifo, socket (`! -type f ! -type d`) e anche gli hardlink verso
# regolari (un file regolare con piu' di un link: -type f -links +1; un bundle
# legittimo non ne contiene mai). La dir di estrazione e' fresca e sotto
# bundles/: se rifiutiamo qui, destDir non e' mai stata sfiorata.
badMembers=$(find "$EXTRACT" -mindepth 1 \( \( ! -type f ! -type d \) -o \( -type f -links +1 \) \) -print)
if [ -n "$badMembers" ]; then
  echo "bundle-import: il bundle contiene membri non regolari (symlink/hardlink/device/fifo/socket) -- rifiutato, $DEST NON toccato:" >&2
  while IFS= read -r bad; do
    [ -n "$bad" ] && echo "  ${bad#"$EXTRACT"/}" >&2
  done <<< "$badMembers"
  exit 1
fi

MANIFEST="$EXTRACT/BUNDLE_MANIFEST.txt"
[ -f "$MANIFEST" ] || {
  echo "bundle-import: BUNDLE_MANIFEST.txt mancante nell'archivio -- non e' un bundle RunBundle v1, $DEST NON toccato" >&2
  exit 1
}

schemaLine=$(grep -m1 '^bundleSchema=' "$MANIFEST" || true)
if [ "$schemaLine" != "bundleSchema=1" ]; then
  echo "bundle-import: bundleSchema mancante o non supportato (${schemaLine:-assente}), $DEST NON toccato" >&2
  exit 1
fi

bundleHashLine=$(grep -m1 '^bundleHash=' "$MANIFEST" || true)
if [ -z "$bundleHashLine" ]; then
  echo "bundle-import: manca la riga bundleHash= nel manifest, $DEST NON toccato" >&2
  exit 1
fi
expectedHash="${bundleHashLine#bundleHash=}"

# Le righe sha256 sono TUTTO cio' che sta fra bundleSchema= e bundleHash= nel
# manifest (vedi bundle-export.sh: e' esattamente il blocco che ha prodotto
# bundleHash la' -- stessi byte, stesso ordine, si ricalcola qui allo stesso
# modo per costruzione, non serve riordinare nulla).
grep -Ev '^(bundleSchema=|bundleHash=)' "$MANIFEST" > "$STAGE/hashlines.txt"
if [ ! -s "$STAGE/hashlines.txt" ]; then
  echo "bundle-import: manifest senza righe sha256 -- bundle vuoto o corrotto, $DEST NON toccato" >&2
  exit 1
fi

recomputedHash=$(sha256sum < "$STAGE/hashlines.txt" | cut -d' ' -f1)
if [ "$recomputedHash" != "$expectedHash" ]; then
  echo "bundle-import: bundleHash non corrisponde (atteso $expectedHash, calcolato $recomputedHash) -- bundle corrotto o manomesso, $DEST NON toccato" >&2
  exit 1
fi

# Verifica OGNI file, uno per uno: bundleHash sopra protegge l'INSIEME delle
# righe (un file aggiunto/tolto/rinominato lo fa cambiare), 'sha256sum -c'
# qui protegge il CONTENUTO di ciascun file elencato (usa lo stesso formato
# "<hash>  <percorso>" che sha256sum scrive e legge, vedi bundle-export.sh).
if ! (cd "$EXTRACT" && sha256sum -c --quiet -- "$STAGE/hashlines.txt"); then
  echo "bundle-import: uno o piu' file non corrispondono al proprio sha256 -- bundle corrotto o manomesso, $DEST NON toccato" >&2
  exit 1
fi

echo "bundle-import: bundle verificato (bundleHash=$expectedHash)"

epoch=$(date +%s)
if [ -e "$DEST" ]; then
  backup="${DEST}.bak-${epoch}"
  mv -- "$DEST" "$backup"
  echo "bundle-import: cartella precedente conservata in $backup"
fi

# Sostituzione ATOMICA: 'mv' su file gia' esistenti sullo stesso filesystem e'
# una rename(), non una copia -- destDir non e' mai visto a meta' sostituito
# da chi lo legge nel frattempo (il gioco, un processo di generazione).
mv -- "$EXTRACT" "$DEST"

echo "bundle-import: fatto. $DEST e' ora il bundle importato (nessuna rigenerazione necessaria)."
