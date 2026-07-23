#!/usr/bin/env bash
# Test di melting-sprites senza modello Stable Diffusion (solo post-processing,
# via --dry-run). Vedi docs/ai-production/experiments/sprites-spike.md per il perche' degli algoritmi.
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

echo "-- senza modello ne' --dry-run, si esce con errore invece di pubblicare un atlas placeholder --"
# Prima di questo fix il tool ripiegava in silenzio sulle celle sintetiche di
# --dry-run, scriveva current_atlas.png e ripuntava li' il manifest: il gioco
# mostrava dodici dischi pastello quasi identici dichiarando "Stable
# Diffusion", mentre l'atlas BMP procedurale (strettamente migliore) era gia'
# su disco. Ora si esce con codice diverso da zero, SENZA scrivere l'atlas ne'
# toccare il manifest: il gioco tiene gia' bene un passo sprite fallito
# (mantiene l'atlas BMP, vedi src/gen/gen_runner.c).
mkdir -p "$TMP/nomodel"
printf 'atlas.path=generated/current_atlas.bmp\nfloor1.theme=Prova\n' > "$TMP/nomodel/current_run.txt"
set +e
"$SPR" --out "$TMP/nomodel" --model "$TMP/nessun-modello.ckpt" --seed 1 >/dev/null 2>&1
rc=$?
set -e
[ "$rc" -ne 0 ]
[ ! -f "$TMP/nomodel/current_atlas.png" ]
grep -q '^atlas.path=generated/current_atlas.bmp$' "$TMP/nomodel/current_run.txt"

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

echo "-- --gen-size 256 (preset --low-spec, roadmap 16/07/2026) produce comunque un atlas 1024x1024 con celle 128x128 --"
# La cella finale nell'atlas NON deve cambiare: solo il downscale modale
# interno cambia fattore (genSize/128, oggi 2 invece di 4). Stesso controllo
# IHDR grezzo usato sopra per il default 512, cosi' un eventuale regressione
# che facesse trapelare genSize nella dimensione dell'atlas o della cella
# verrebbe presa qui.
"$SPR" --dry-run --seed 12345 --gen-size 256 --out "$TMP/e" >/dev/null
[ -f "$TMP/e/current_atlas.png" ]
read -r width256 height256 bitdepth256 colortype256 < <(od -An -tu1 -j16 -N10 "$TMP/e/current_atlas.png" | awk '{
  w = $1*16777216 + $2*65536 + $3*256 + $4
  h = $5*16777216 + $6*65536 + $7*256 + $8
  print w, h, $9, $10
}')
[ "$width256" -eq 1024 ]
[ "$height256" -eq 1024 ]
[ "$bitdepth256" -eq 8 ]
[ "$colortype256" -eq 6 ]
"$SPR" --check --out "$TMP/e" --cells 12 > "$TMP/check256.txt"
[ "$(wc -l < "$TMP/check256.txt")" -eq 12 ]
if grep -qv "eyeOpaque=1" "$TMP/check256.txt"; then
  echo "FALLITO (gen-size 256): un pixel nero interno e' stato mangiato dal ritaglio"; exit 1
fi
if grep -qv "keyRisk=0" "$TMP/check256.txt"; then
  echo "FALLITO (gen-size 256): pixel opachi con max(r,g,b) < 16 sopravvivono alla quantizzazione"; exit 1
fi

echo "-- --gen-size 300 e' rifiutato (solo 256 o 512 sono ammessi) --"
set +e
"$SPR" --dry-run --seed 1 --gen-size 300 --out "$TMP/badgensize" >/dev/null 2>"$TMP/badgensize.err"
rc=$?
set -e
[ "$rc" -ne 0 ]
[ ! -f "$TMP/badgensize/current_atlas.png" ]
grep -q -- "--gen-size deve essere 256 o 512" "$TMP/badgensize.err"

echo "-- semi diversi = atlas diversi --"
"$SPR" --dry-run --seed 999 --out "$TMP/c" >/dev/null
if cmp -s "$TMP/a/current_atlas.png" "$TMP/c/current_atlas.png"; then
  echo "FALLITO: due semi diversi hanno prodotto lo stesso atlas"; exit 1
fi

echo "-- --cells limita il numero di celle generate (per iterare in fretta) --"
"$SPR" --dry-run --seed 1 --cells 3 --out "$TMP/d" >/dev/null
"$SPR" --check --out "$TMP/d" --cells 3 > "$TMP/check3.txt"
[ "$(wc -l < "$TMP/check3.txt")" -eq 3 ]

echo "-- il gate di qualita' scarta una cella il cui riquadro opaco tocca il bordo --"
# --dry-run normale (sopra) sintetizza sempre celle con margine di sfondo su
# tutti i lati (vedi SynthesizeCell): il ramo "il riquadro opaco tocca il
# bordo" di CellPassesQualityGate, il secondo tentativo con un altro seed, e
# la cella lasciata trasparente quando anche il secondo fallisce non erano
# MAI esercitati da nessun test. --dry-run-bad-border sintetizza invece una
# cella deliberatamente vicina al bordo sinistro (vedi SynthesizeCellBadBorder
# in main.c): stessa firma di un crop di Stable Diffusion fallito.
: > logs/melting-sprites.log
"$SPR" --dry-run-bad-border --seed 42 --cells 1 --out "$TMP/badborder" >/dev/null
"$SPR" --check --out "$TMP/badborder" --cells 1 > "$TMP/badborder_check.txt"
grep -q '^cell=0 opaque=0 ' "$TMP/badborder_check.txt"
grep -q 'SCARTATA (dry-run-bad-border, tentativo 1): il riquadro opaco tocca il bordo' logs/melting-sprites.log
grep -q 'SCARTATA (dry-run-bad-border, tentativo 2): il riquadro opaco tocca il bordo' logs/melting-sprites.log
grep -q 'nessun tentativo valido (dry-run-bad-border), resta trasparente' logs/melting-sprites.log

echo "-- SpritesUpdateManifestAtlasPath riscrive SOLO atlas.path, il resto del manifest sopravvive --"
# Prima d'ora questa funzione non aveva alcuna copertura di test.
mkdir -p "$TMP/manifest"
printf 'floor1.theme=Tema di Prova\nfloor1.style=pixel semplice\natlas.path=generated/current_atlas.bmp\nextra.campo=invariato\n' > "$TMP/manifest/current_run.txt"
"$SPR" --dry-run --seed 5 --cells 1 --out "$TMP/manifest" >/dev/null
printf 'floor1.theme=Tema di Prova\nfloor1.style=pixel semplice\natlas.path=generated/current_atlas.png\nextra.campo=invariato\n' > "$TMP/manifest_expected.txt"
diff "$TMP/manifest_expected.txt" "$TMP/manifest/current_run.txt"

echo "-- --bench senza modello: exit 1, la cartella --out resta byte per byte identica --"
# Piano strategico 16/07/2026, sezione tier: melting-sprites non ha un modello
# di ripiego (a differenza di melting-gen), quindi un solo --model su un
# percorso inesistente basta a esercitare il ramo "nessun modello disponibile".
mkdir -p "$TMP/bench-nomodel"
echo marker > "$TMP/bench-nomodel/marker.txt"
before=$(find "$TMP/bench-nomodel" -type f -exec sha256sum {} \; | sort)
set +e
"$SPR" --bench --model "$TMP/nonexistent.ckpt" --out "$TMP/bench-nomodel" \
       >"$TMP/bench-nomodel.out" 2>"$TMP/bench-nomodel.err"
rc=$?
set -e
[ "$rc" -eq 1 ]
[ -z "$(cat "$TMP/bench-nomodel.out")" ]   # niente riga "bench: ..." su stdout quando fallisce
after=$(find "$TMP/bench-nomodel" -type f -exec sha256sum {} \; | sort)
[ "$before" = "$after" ]
grep -q "modello assente" "$TMP/bench-nomodel.err"

echo "TEST-SPRITES: OK"
