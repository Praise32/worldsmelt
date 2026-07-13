#!/usr/bin/env bash
# Test di melting-sprites senza modello Stable Diffusion (solo post-processing,
# via --dry-run). Vedi docs/SPRITES-SPIKE.md per il perche' degli algoritmi.
set -euo pipefail
cd "$(dirname "$0")/.."

SPR=bin/melting-sprites
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

echo "-- fase 3: melting-sprites linka stable-diffusion.cpp ma MAI llama.cpp (due ggml incompatibili, vedi Makefile) --"
# nm scrive su file invece di andare diretto in pipe a grep -q: con pipefail,
# grep -q che chiude presto la pipe manda SIGPIPE a nm e il suo exit status
# (141) vince quello di grep nella pipeline, anche se il pattern e' stato
# trovato. Su file non c'e' pipe da rompere.
NM_OUT="$TMP/nm.txt"
nm "$SPR" 2>/dev/null > "$NM_OUT"
grep -qiE 'new_sd_ctx|generate_image' "$NM_OUT"
! grep -qiE 'llama_model_load|llama_decode|llama_sampler' "$NM_OUT"

echo "-- senza modello ne' --dry-run, si ripiega su celle sintetiche invece di andare in crash --"
"$SPR" --out "$TMP/nomodel" --model "$TMP/nessun-modello.ckpt" --seed 1 >/dev/null
[ -f "$TMP/nomodel/current_atlas.png" ]

echo "-- --dry-run produce un PNG 1024x1024 RGBA a 8 bit per canale --"
"$SPR" --dry-run --seed 12345 --out "$TMP/a" >/dev/null
[ -f "$TMP/a/current_atlas.png" ]
# IHDR (RFC PNG): firma (8 byte) + lunghezza chunk (4) + "IHDR" (4) +
# width(4) + height(4) + bitdepth(1) + colortype(1). Si leggono i byte grezzi,
# niente tool esterni di decodifica immagini.
read -r width height bitdepth colortype < <(od -An -tu1 -j16 -N10 "$TMP/a/current_atlas.png" | awk '{
  w = $1*16777216 + $2*65536 + $3*256 + $4
  h = $5*16777216 + $6*65536 + $7*256 + $8
  print w, h, $9, $10
}')
[ "$width" -eq 1024 ]
[ "$height" -eq 1024 ]
[ "$bitdepth" -eq 8 ]
[ "$colortype" -eq 6 ]   # 6 = truecolore con alpha (RGBA)

echo "-- il post-processing supera i casi difficili sintetizzati da --dry-run --"
# melting-sprites --check decodifica il PNG vero con stb_image (gia' linkato
# nel binario) e stampa, per ogni cella, statistiche verificabili qui: non
# serve un decoder PNG in bash per ispezionare i pixel.
REPORT="$TMP/check.txt"
"$SPR" --check --out "$TMP/a" --cells 12 > "$REPORT"
[ "$(wc -l < "$REPORT")" -eq 12 ]

echo "   lo sfondo e' tagliato: i pixel trasparenti dominano il bordo di ogni cella"
if awk -F'borderCutRatio=' '{split($2,a," "); if (a[1]+0 < 0.8) { print $0; bad=1 } } END{exit bad?1:0}' "$REPORT"; then
  :
else
  echo "FALLITO: almeno una cella ha il bordo non tagliato (sopra righe)"; exit 1
fi

echo "   il pixel nero dentro lo sprite (l'occhio) sopravvive al ritaglio dello sfondo"
if grep -qv "eyeOpaque=1" "$REPORT"; then
  echo "FALLITO: un pixel nero interno e' stato mangiato dal ritaglio (regressione a soglia di luminosita'?)"
  exit 1
fi

echo "   la macchia color-sfondo racchiusa nel corpo NON viene tagliata (non raggiungibile dal bordo)"
if grep -qv "patchOpaque=1" "$REPORT"; then
  echo "FALLITO: una macchia color-sfondo racchiusa nel corpo e' stata tagliata"
  exit 1
fi

echo "   nessun pixel opaco cade sotto KEY_FLOOR (rischio chroma-key in gioco)"
if grep -qv "keyRisk=0" "$REPORT"; then
  echo "FALLITO: pixel opachi con max(r,g,b) < 16 sopravvivono alla quantizzazione"
  exit 1
fi

echo "-- determinismo: stesso seed = atlas identico byte per byte --"
"$SPR" --dry-run --seed 12345 --out "$TMP/b" >/dev/null
cmp "$TMP/a/current_atlas.png" "$TMP/b/current_atlas.png"

echo "-- semi diversi = atlas diversi --"
"$SPR" --dry-run --seed 999 --out "$TMP/c" >/dev/null
if cmp -s "$TMP/a/current_atlas.png" "$TMP/c/current_atlas.png"; then
  echo "FALLITO: due semi diversi hanno prodotto lo stesso atlas"; exit 1
fi

echo "-- --cells limita il numero di celle generate (per iterare in fretta) --"
"$SPR" --dry-run --seed 1 --cells 3 --out "$TMP/d" >/dev/null
"$SPR" --check --out "$TMP/d" --cells 3 > "$TMP/check3.txt"
[ "$(wc -l < "$TMP/check3.txt")" -eq 3 ]

echo "TEST-SPRITES: OK"
