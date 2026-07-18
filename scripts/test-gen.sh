#!/usr/bin/env bash
# Test di melting-gen senza modello LLM.
set -euo pipefail
cd "$(dirname "$0")/.."

# I test che aprono una finestra girano su display virtuale quando disponibile
# (vedi la variabile TEST_RUNNER nel Makefile): su una sessione Wayland bloccata
# il gioco resterebbe appeso al primo SwapBuffers.
XVFB_RUNTIME="$PWD/.xvfb-runtime"
if command -v xvfb-run >/dev/null 2>&1; then
  mkdir -p "$XVFB_RUNTIME" && chmod 700 "$XVFB_RUNTIME"
  GAME_RUN=(env -u WAYLAND_DISPLAY "XDG_RUNTIME_DIR=$XVFB_RUNTIME"
            xvfb-run -a -s "-screen 0 1920x1080x24 +extension GLX +render")
else
  GAME_RUN=()
fi

GEN=bin/melting-gen
TMP=$(mktemp -d)

# Ledger di novita' fra run (gen_novelty.c, piano strategico "check contro le
# ultime ~20 run"): logs/novelty-ledger.txt e' un file PERSISTENTE e condiviso
# con le run vere del giocatore, non uno scratch di processo come gen-corpus/
# (che ha il pid nel nome). I test che lo toccano (piu' sotto) salvano
# l'eventuale ledger preesistente qui e lo ripristinano ALLA FINE DELLA
# SUITE, qualunque sia l'esito (trap EXIT, non un cleanup a fondo pagina):
# altrimenti un test fallito a meta' lascerebbe il ledger sintetico al posto
# di quello vero.
NOVELTY_LEDGER="logs/novelty-ledger.txt"
NOVELTY_LEDGER_BACKUP="$TMP/.novelty-ledger-backup.txt"
[ -f "$NOVELTY_LEDGER" ] && cp "$NOVELTY_LEDGER" "$NOVELTY_LEDGER_BACKUP"
restore_novelty_ledger() {
  if [ -f "$NOVELTY_LEDGER_BACKUP" ]; then
    mkdir -p "$(dirname "$NOVELTY_LEDGER")"
    cp "$NOVELTY_LEDGER_BACKUP" "$NOVELTY_LEDGER"
  else
    rm -f "$NOVELTY_LEDGER"
  fi
}
trap 'restore_novelty_ledger; rm -rf "$TMP"' EXIT

echo "-- corpus delle generazioni: una riga JSONL valida per il ramo fallback --"
# gen_corpus.c scrive in logs/gen-corpus/ una riga per ogni evento del modello
# (tentativi JSON, tentativi Lua, ripieghi). Senza modello si esercita il ramo
# fallback: file creato, JSON valido, campi giusti. La riga di prova viene poi
# rimossa: il corpus di un run di test non e' un dato.
"$GEN" --fallback --seed 424241 --out "$TMP/corpus" > /dev/null
newest="logs/gen-corpus/$(ls -t logs/gen-corpus | head -1)"
python3 - "$newest" <<'PYEOF'
import json, sys
lines = [l for l in open(sys.argv[1]) if l.strip()]
assert lines, "corpus vuoto"
rec = json.loads(lines[0])
assert rec["kind"] == "fallback" and rec["explicit"] is True and rec["seed"] == 424241, rec
PYEOF
rm -f "$newest"

# Semi d'ispirazione (roadmap 16/07/2026, settimana 1): --print-json-prompt
# costruisce il prompt JSON completo e lo stampa su stdout senza caricare
# alcun modello -- il modo giusto per verificare il blocco "Inspirations for
# THIS run" (placeholder sostituito, deterministico sul seed, diverso fra
# semi) senza aspettare una generazione vera. Stringa in inglese dal 18/07
# (DEC-052, generazione contenuti inglese-first): vedi gen_inspire.c.
echo "-- semi d'ispirazione: --print-json-prompt espone il blocco nel prompt --"
"$GEN" --print-json-prompt --seed 4242 --out "$TMP/prompt-a" > "$TMP/prompt-a.txt"
grep -q "Inspirations for THIS run" "$TMP/prompt-a.txt" || {
  echo "FALLITO: il prompt stampato non contiene il blocco ispirazioni"; exit 1; }

echo "-- semi d'ispirazione: stesso seed due volte -> prompt identico --"
"$GEN" --print-json-prompt --seed 4242 --out "$TMP/prompt-b" > "$TMP/prompt-b.txt"
cmp "$TMP/prompt-a.txt" "$TMP/prompt-b.txt" || {
  echo "FALLITO: stesso seed ha prodotto prompt diversi (le ispirazioni non sono deterministiche)"; exit 1; }

echo "-- semi d'ispirazione: semi diversi -> prompt diversi --"
"$GEN" --print-json-prompt --seed 1111 --out "$TMP/prompt-c" > "$TMP/prompt-c.txt"
"$GEN" --print-json-prompt --seed 2222 --out "$TMP/prompt-d" > "$TMP/prompt-d.txt"
if cmp -s "$TMP/prompt-c.txt" "$TMP/prompt-d.txt"; then
  echo "FALLITO: semi diversi (1111 vs 2222) hanno prodotto lo stesso prompt"; exit 1
fi

echo "-- esperimento due-modelli: --model-text con file inesistente non tocca la costruzione del prompt --"
# --model-text riguarda SOLO il caricamento del modello (main.c, il blocco
# 'useTextModel'), mai raggiunto da --print-json-prompt (che ritorna prima di
# qualunque caricamento, vedi il commento in main.c): un percorso inesistente
# deve comportarsi esattamente come nessun flag, exit 0 incluso.
"$GEN" --print-json-prompt --model-text /percorso/inesistente --seed 99 --out "$TMP/prompt-text" > "$TMP/prompt-text.txt"
"$GEN" --print-json-prompt --seed 99 --out "$TMP/prompt-text-ref" > "$TMP/prompt-text-ref.txt"
cmp "$TMP/prompt-text.txt" "$TMP/prompt-text-ref.txt" || {
  echo "FALLITO: --model-text (con file inesistente) ha cambiato il prompt stampato"; exit 1; }

echo "-- semi d'ispirazione: il placeholder {ISPIRAZIONI} non compare mai nel prompt stampato --"
if grep -q '{ISPIRAZIONI}' "$TMP/prompt-a.txt"; then
  echo "FALLITO: il placeholder {ISPIRAZIONI} e' rimasto nel prompt (sostituzione mancata)"; exit 1
fi

# Esempi rotanti nel SYSTEM prompt (contromossa alla misura A/B: "Colonnato
# Sacro" duplicato x2 fra due run era l'esempio letterale fisso di
# system.txt, l'unico ancoraggio residuo dopo le ispirazioni sopra --
# gen_inspire.c). Stessa via di verifica: --print-json-prompt senza modello.
echo "-- esempi rotanti: nessun placeholder {ESEMPIO_*} residuo nel prompt stampato --"
if grep -q '{ESEMPIO_ROOM}\|{ESEMPIO_NEMICI}\|{ESEMPIO_COLPI}' "$TMP/prompt-a.txt"; then
  echo "FALLITO: un placeholder {ESEMPIO_*} e' rimasto nel prompt (sostituzione mancata)"; exit 1
fi

# Marcatori stabili per contare le righe JSON d'esempio senza dipendere dal
# loro contenuto (che cambia per seed): le uniche righe con questi tre campi
# seguiti DA UNA CIFRA sono gli esempi veri -- le righe di schema nel resto
# del prompt usano placeholder fra parentesi angolari ("density":<0.2-1>,
# "hp":<mul>, "pierce":<0-3>), mai una cifra dopo i due punti. Verificato a
# mano su un prompt reale prima di fissare i marcatori.
echo "-- esempi rotanti: ESATTAMENTE 1 esempio room + 2 nemici + 3 colpi --"
nRoom=$(grep -c '"density":[0-9]' "$TMP/prompt-a.txt")
nNemici=$(grep -c '"hp":[0-9]' "$TMP/prompt-a.txt")
nColpi=$(grep -c '"pierce":[0-9]' "$TMP/prompt-a.txt")
[ "$nRoom" = "1" ] || { echo "FALLITO: attesi 1 esempio room, trovati $nRoom"; exit 1; }
[ "$nNemici" = "2" ] || { echo "FALLITO: attesi 2 esempi nemici, trovati $nNemici"; exit 1; }
[ "$nColpi" = "3" ] || { echo "FALLITO: attesi 3 esempi colpi, trovati $nColpi"; exit 1; }

# Il determinismo stesso-seed (cmp prompt-a/prompt-b sopra) copre GIA' anche
# gli esempi rotanti del SYSTEM prompt: --print-json-prompt stampa il prompt
# ChatML intero (system + user), quindi quel cmp confronta i byte del
# system prompt tanto quanto quelli dello user prompt -- non serve un
# secondo cmp dedicato.

# ============================================================
# Ledger di novita' fra RUN (gen_novelty.c, piano strategico "check contro le
# ultime ~20 run"): a differenza dei semi d'ispirazione sopra (che risolvono
# la varieta' DENTRO una run) questo blocco dipende dalla STORIA su disco --
# --print-json-prompt e' di nuovo il modo giusto per verificarlo senza un
# modello vero. Le parole del ledger sintetico qui sotto sono INVENTATE
# apposta (mai una delle liste d'ispirazione di gen_inspire.c, es.
# "cattedrale"/"palude"/"vulcano" sono gia' possibili LUOGHI: userle come
# prova avrebbe reso i controlli di presenza/assenza dei falsi verdi/rossi a
# seconda del seed).
# ============================================================

echo "-- ledger di novita': con un ledger sintetico, {EVITA} mostra solo le parole viste in >=2 run --"
mkdir -p "$(dirname "$NOVELTY_LEDGER")"
cat > "$NOVELTY_LEDGER" <<'EOF'
seed=1001 words=convergenza1 convergenza2 unicauno
seed=1002 words=convergenza1 unicadue
seed=1003 words=convergenza1 convergenza2
EOF
"$GEN" --print-json-prompt --seed 5555 --out "$TMP/prompt-evita" > "$TMP/prompt-evita.txt"
evitaLine=$(grep "Words already seen in your recent runs" "$TMP/prompt-evita.txt") || {
  echo "FALLITO: il blocco EVITA non compare col ledger sintetico presente"; exit 1; }
# convergenza1 e' in 3/3 righe, convergenza2 in 2/3: entrambe convergono
# (viste in ALMENO 2 run diverse), quindi devono comparire, in quest'ordine
# (frequenza decrescente: 3 prima di 2).
echo "$evitaLine" | grep -q "convergenza1.*convergenza2" || {
  echo "FALLITO: 'convergenza1'/'convergenza2' assenti dal blocco EVITA o fuori ordine (atteso per frequenza decrescente): $evitaLine"; exit 1; }
# unicauno e unicadue compaiono ciascuna in UNA sola riga: una parola usata
# una volta sola non e' convergenza, non deve finire nella lista da evitare.
for w in unicauno unicadue; do
  if echo "$evitaLine" | grep -q "$w"; then
    echo "FALLITO: '$w' (vista in una sola run) compare nel blocco EVITA: $evitaLine"; exit 1
  fi
done
if grep -q '{EVITA}' "$TMP/prompt-evita.txt"; then
  echo "FALLITO: il placeholder {EVITA} e' rimasto nel prompt (sostituzione mancata)"; exit 1
fi

echo "-- ledger di novita': senza ledger, nessun blocco EVITA e nessun placeholder residuo --"
rm -f "$NOVELTY_LEDGER"
"$GEN" --print-json-prompt --seed 5555 --out "$TMP/prompt-noevita" > "$TMP/prompt-noevita.txt"
if grep -q "Words already seen" "$TMP/prompt-noevita.txt"; then
  echo "FALLITO: il blocco EVITA compare senza alcun ledger su disco"; exit 1
fi
if grep -q '{EVITA}' "$TMP/prompt-noevita.txt"; then
  echo "FALLITO: il placeholder {EVITA} e' rimasto nel prompt (sostituzione mancata) senza ledger"; exit 1
fi

echo "-- ledger di novita': una generazione --fallback NON scrive nel ledger --"
rm -f "$NOVELTY_LEDGER"
"$GEN" --fallback --seed 9090 --out "$TMP/novelty-fallback" >/dev/null
if [ -f "$NOVELTY_LEDGER" ]; then
  echo "FALLITO: una generazione --fallback ha scritto nel ledger (il vocabolario procedurale avvelenerebbe la lista 'da evitare')"; exit 1
fi

echo "-- ledger di novita': parole lunghissime (31 char), l'ultima non viene troncata a meta' --"
# Caso peggiore del dimensionamento del buffer C dell'elenco EVITA: 24 parole
# di 31 caratteri (il tetto reale degli slot a 32 byte del tally) separate da
# ", " sono ~790 byte, oltre i 768 di un buffer dimensionato "a spanne" a
# MAX*32. Con quel buffer l'ULTIMA parola finiva troncata a meta' e spedita
# spazzatura nel prompt LLM; con GEN_NOVELTY_AVOID_BUF_SIZE (e l'omissione a
# parola intera in GenNoveltyAvoidList) deve arrivare INTERA. Due run che
# condividono le stesse 24 parole: tutte convergono (viste in >=2 run).
rm -f "$NOVELTY_LEDGER"
mkdir -p "$(dirname "$NOVELTY_LEDGER")"
xpad=$(printf '%028d' 0 | tr '0' 'x')   # 28 'x' -> "wNN"(3) + 28 = 31 caratteri
longWords=""
for i in $(seq 0 23); do
  longWords="$longWords w$(printf '%02d' "$i")$xpad"
done
printf 'seed=7001 words=%s\n' "$longWords" >> "$NOVELTY_LEDGER"
printf 'seed=7002 words=%s\n' "$longWords" >> "$NOVELTY_LEDGER"
"$GEN" --print-json-prompt --seed 4242 --out "$TMP/prompt-long" > "$TMP/prompt-long.txt"
evitaLong=$(grep "Words already seen in your recent runs" "$TMP/prompt-long.txt") || {
  echo "FALLITO: il blocco EVITA non compare col ledger di parole lunghe"; exit 1; }
lastLong="w23$xpad"   # l'ultima parola, per intero (31 caratteri)
echo "$evitaLong" | grep -q "$lastLong" || {
  echo "FALLITO: l'ultima parola lunga (31 char) e' stata troncata a meta' o omessa dal blocco EVITA (bug di dimensionamento del buffer): $evitaLong"; exit 1; }

echo "-- ledger di novita': una parola OLTRE il cap (35 char) converge sulla forma troncata --"
# Bug reale trovato dalla verifica adversariale: TallyLine confrontava il
# token GREZZO con la copia gia' troncata a 31 caratteri salvata nel tally,
# quindi una parola oltre il cap non faceva mai match con se stessa e non
# "convergeva" MAI (ledger editato a mano o scritto da una versione con un
# cap diverso). Ora il confronto avviene sulla forma troncata: la stessa
# parola di 35 caratteri ripetuta in 3 run deve comparire nel blocco EVITA
# (nella sua forma a 31 caratteri).
rm -f "$NOVELTY_LEDGER"
overWord="$(printf '%035d' 0 | tr '0' 'z')"   # 35 'z', oltre il cap di 31
printf 'seed=7101 words=%s\nseed=7102 words=%s\nseed=7103 words=%s\n' \
  "$overWord" "$overWord" "$overWord" >> "$NOVELTY_LEDGER"
"$GEN" --print-json-prompt --seed 4242 --out "$TMP/prompt-over" > "$TMP/prompt-over.txt"
cutWord="${overWord:0:31}"
grep "Words already seen in your recent runs" "$TMP/prompt-over.txt" | grep -q "$cutWord" || {
  echo "FALLITO: una parola oltre il cap (35 char, 3 run su 3) non converge nel blocco EVITA"; exit 1; }

# Il ledger sintetico ha fatto il suo lavoro: da qui in giu' non serve piu' (e
# restore_novelty_ledger, gia' agganciato al trap EXIT in cima allo script,
# ripristina comunque il ledger vero del giocatore alla fine della suite).
rm -f "$NOVELTY_LEDGER"

# Da qui in giu' la suite lancia melting-gen decine di volte: il corpus va
# spento o riempirebbe logs/gen-corpus di run finte (vedi gen_corpus.c).
export MELTING_GEN_NO_CORPUS=1

# ============================================================
# RunBundle v1 (roadmap 16/07/2026, settimana 4 anticipata): la provenienza
# scritta a fine generazione (tools/melting-gen/gen_manifest.c,
# GenWriteProvenance) e il bundle esportabile/importabile con verifica
# d'integrita' sha256 (scripts/bundle-export.sh, scripts/bundle-import.sh).
# Nessun modello coinvolto: come il resto di questa suite, si esercita il
# ramo fallback e, per lo script Lua, lo stesso trucco delle prove B2 sopra
# (un file .lua scritto a mano, nessun LLM).
# ============================================================

echo "-- RunBundle: provenance.txt su una generazione fallback --"
"$GEN" --fallback --seed 777 --out "$TMP/bundle-src" >/dev/null
[ -f "$TMP/bundle-src/provenance.txt" ] || {
  echo "FALLITO: provenance.txt non scritto da una generazione fallback"; exit 1; }
grep -q '^bundleSchema=1$' "$TMP/bundle-src/provenance.txt" || {
  echo "FALLITO: bundleSchema=1 mancante in provenance.txt"; exit 1; }
grep -q '^seed=777$' "$TMP/bundle-src/provenance.txt" || {
  echo "FALLITO: seed=777 mancante in provenance.txt"; exit 1; }
grep -q '^source=fallback$' "$TMP/bundle-src/provenance.txt" || {
  echo "FALLITO: source=fallback mancante in provenance.txt"; exit 1; }
fnvLine=$(grep '^promptsFnv=' "$TMP/bundle-src/provenance.txt" || true)
fnvValue="${fnvLine#promptsFnv=}"
[ -n "$fnvValue" ] || { echo "FALLITO: promptsFnv assente in provenance.txt"; exit 1; }
echo "$fnvValue" | grep -Eq '^[0-9a-f]{16}$' || {
  echo "FALLITO: promptsFnv=$fnvValue non e' un hash esadecimale a 16 cifre"; exit 1; }

# Uno script Lua finto (nessun modello in questa suite): serve un file .lua
# VERO dentro il bundle per poterlo corrompere nella prova (c) piu' sotto.
mkdir -p "$TMP/bundle-src/scripts"
printf 'function on_fire(x, y, dx, dy)\n  spawn_shot(x, y, dx, dy, 300, 3, 4, 0)\nend\n' \
  > "$TMP/bundle-src/scripts/floor1_item1.lua"

echo "-- RunBundle: l'export rifiuta una cartella senza i file essenziali --"
mkdir -p "$TMP/bundle-empty"
if scripts/bundle-export.sh "$TMP/bundle-empty" 2>"$TMP/bundle-empty-err.txt"; then
  echo "FALLITO: l'export ha accettato una cartella senza current_run.txt/current_run.json/provenance.txt"; exit 1
fi
grep -qi "manca" "$TMP/bundle-empty-err.txt" || {
  echo "FALLITO: l'export ha rifiutato la cartella incompleta senza un messaggio chiaro"; exit 1; }

echo "-- RunBundle: export + import round-trip, stessi byte --"
rm -rf bundles
scripts/bundle-export.sh "$TMP/bundle-src" >/dev/null
bundleFile=$(ls bundles/melting-bundle-seed777-*.tar.gz 2>/dev/null | head -1)
[ -n "$bundleFile" ] || { echo "FALLITO: bundle-export non ha scritto nessun tar.gz in bundles/"; exit 1; }

scripts/bundle-import.sh "$bundleFile" "$TMP/bundle-dst" >/dev/null

# BUNDLE_MANIFEST.txt esiste SOLO nella destinazione importata (fa parte del
# bundle, non della run originale); gen_progress.txt esiste SOLO nella
# sorgente (escluso di proposito dall'export: e' lo stato effimero della
# barra di avanzamento). Il confronto file-per-file esclude solo questi due,
# tutto il resto deve tornare byte-a-byte.
for f in $(cd "$TMP/bundle-src" && find . -type f ! -name gen_progress.txt | sed 's#^\./##' | sort); do
  cmp "$TMP/bundle-src/$f" "$TMP/bundle-dst/$f" || {
    echo "FALLITO: $f differisce fra sorgente e bundle importato"; exit 1; }
done
[ -f "$TMP/bundle-dst/BUNDLE_MANIFEST.txt" ] || {
  echo "FALLITO: BUNDLE_MANIFEST.txt mancante nella destinazione importata"; exit 1; }

echo "-- RunBundle: l'import rifiuta un bundle con un file .lua corrotto --"
WORK="$TMP/bundle-corrupt-work"
mkdir -p "$WORK"
tar xzf "$bundleFile" -C "$WORK"
# Corrompe UN byte del file .lua gia' estratto, poi ricompone il tar SENZA
# toccare BUNDLE_MANIFEST.txt: il file sul disco non corrisponde piu' al
# proprio sha256 registrato nel manifest.
printf 'X' | dd of="$WORK/scripts/floor1_item1.lua" bs=1 seek=0 count=1 conv=notrunc status=none
corruptFile="$TMP/bundle-corrupt.tar.gz"
tar czf "$corruptFile" -C "$WORK" .

set +e
scripts/bundle-import.sh "$corruptFile" "$TMP/bundle-dst2" >"$TMP/bundle-corrupt-out.txt" 2>&1
rc=$?
set -e
[ "$rc" -eq 1 ] || {
  echo "FALLITO: l'import di un bundle corrotto non e' uscito con exit 1 (rc=$rc)"; exit 1; }
[ ! -e "$TMP/bundle-dst2" ] || {
  echo "FALLITO: l'import di un bundle corrotto ha comunque creato la destinazione"; exit 1; }
grep -qi "sha256\|corrotto" "$TMP/bundle-corrupt-out.txt" || {
  echo "FALLITO: il rifiuto dell'import corrotto non ha un messaggio chiaro"; exit 1; }

echo "-- RunBundle: l'import rifiuta un bundle con un membro symlink --"
# La verifica sha256 da sola NON protegge da un membro symlink: sha256sum SEGUE
# il link, quindi un `evil -> <file dal contenuto noto>` supera sia sha256sum -c
# sia il bundleHash (l'attaccante calcola in anticipo l'hash del bersaglio e lo
# mette nel manifest) e resterebbe vivo in destDir, puntando FUORI -- un
# successivo bundle-export.sh lo dereferenzierebbe, impacchettando il contenuto
# del bersaglio (catena di exfiltrazione). Qui si costruisce ESATTAMENTE quel
# caso: un bundle valido in cui un membro .lua e' sostituito da un symlink verso
# un file ESTERNO col CONTENUTO IDENTICO all'originale -- cosi' lo sha256 nel
# manifest resta corretto e a decidere il rifiuto e' solo la difesa
# anti-symlink, non la verifica di contenuto. L'import deve uscire 1 e NON
# toccare la destinazione.
SYMWORK="$TMP/bundle-sym-work"
mkdir -p "$SYMWORK"
tar xzf "$bundleFile" -C "$SYMWORK"
external="$TMP/bundle-sym-target.lua"
cp "$SYMWORK/scripts/floor1_item1.lua" "$external"
rm "$SYMWORK/scripts/floor1_item1.lua"
ln -s "$external" "$SYMWORK/scripts/floor1_item1.lua"
symFile="$TMP/bundle-sym.tar.gz"
# tar di default NON segue i symlink: li archivia come membri link, che e'
# proprio cio' che serve per riprodurre il bundle malevolo.
tar czf "$symFile" -C "$SYMWORK" .

set +e
scripts/bundle-import.sh "$symFile" "$TMP/bundle-dst-sym" >"$TMP/bundle-sym-out.txt" 2>&1
rc=$?
set -e
[ "$rc" -eq 1 ] || {
  echo "FALLITO: l'import di un bundle con un symlink non e' uscito con exit 1 (rc=$rc)"; cat "$TMP/bundle-sym-out.txt"; exit 1; }
[ ! -e "$TMP/bundle-dst-sym" ] || {
  echo "FALLITO: l'import di un bundle con un symlink ha comunque creato/toccato la destinazione"; exit 1; }
grep -qi "non regolar\|symlink" "$TMP/bundle-sym-out.txt" || {
  echo "FALLITO: il rifiuto del symlink non ha un messaggio chiaro"; cat "$TMP/bundle-sym-out.txt"; exit 1; }

echo "-- RunBundle: l'import rifiuta un membro con path traversal (../) --"
# Il GNU tar di sistema rifiuta gia' da solo i membri con ".." in estrazione,
# ma quella e' una proprieta' del binario installato: bundle-import.sh ha ora
# una guardia PROPRIA (tar -tzf + filtro sui membri) che deve scattare
# QUALUNQUE tar ci sia sotto. Il tar malevolo si costruisce con Python
# (tarfile non sanifica in creazione, a differenza di GNU tar).
travFile="$TMP/bundle-trav.tar.gz"
python3 - "$travFile" <<'PYEOF'
import io, sys, tarfile
with tarfile.open(sys.argv[1], "w:gz") as t:
    data = b"pwned\n"
    info = tarfile.TarInfo(name="../evil-traversal.txt")
    info.size = len(data)
    t.addfile(info, io.BytesIO(data))
PYEOF
set +e
scripts/bundle-import.sh "$travFile" "$TMP/bundle-dst-trav" >"$TMP/bundle-trav-out.txt" 2>&1
rc=$?
set -e
[ "$rc" -eq 1 ] || {
  echo "FALLITO: l'import di un bundle con ../ non e' uscito con exit 1 (rc=$rc)"; cat "$TMP/bundle-trav-out.txt"; exit 1; }
[ ! -e "$TMP/bundle-dst-trav" ] || {
  echo "FALLITO: l'import con path traversal ha comunque creato la destinazione"; exit 1; }
[ ! -e "$TMP/evil-traversal.txt" ] && [ ! -e "bundles/evil-traversal.txt" ] || {
  echo "FALLITO: il membro ../evil-traversal.txt e' stato scritto fuori dalla dir di estrazione"; exit 1; }
grep -q "percorso pericoloso" "$TMP/bundle-trav-out.txt" || {
  echo "FALLITO: il rifiuto del path traversal non ha il messaggio della guardia propria dello script"; cat "$TMP/bundle-trav-out.txt"; exit 1; }

echo "-- RunBundle: due export dello stesso contenuto = stesso tar.gz (LC_ALL=C) --"
# bundleHash deriva dall'ordine delle righe sha256, e quell'ordine deriva dal
# sort dell'export: senza LC_ALL=C due macchine con locale diverse (o la
# stessa macchina in locale it_IT.UTF-8) possono ordinare in modo diverso i
# nomi che differiscono per maiuscole -- trovato dalla verifica adversariale.
# Due export consecutivi (la locale qui e' costante) devono comunque produrre
# lo STESSO file byte-per-byte; il nome contiene hash8, quindi basta un cmp.
exp1=$(LC_ALL=it_IT.UTF-8 scripts/bundle-export.sh "$TMP/bundle-src" | sed -n 's/^bundle-export: scritto //p')
exp2=$(LC_ALL=C scripts/bundle-export.sh "$TMP/bundle-src" | sed -n 's/^bundle-export: scritto //p')
[ "$exp1" = "$exp2" ] || {
  echo "FALLITO: lo stesso contenuto esportato sotto locale diverse ha prodotto file diversi ($exp1 vs $exp2)"; exit 1; }
cmp "$exp1" "$exp2" || {
  echo "FALLITO: due export dello stesso contenuto non sono byte-identici"; exit 1; }

echo "-- RunBundle: --resume NON riscrive provenance.txt (appartiene alla stessa run) --"
# Nessun override di --model qui (stesso stile delle altre prove di ripresa
# di questa suite, vedi step B2 sopra): se un modello e' presente in models/
# la ripresa lo userebbe per gli script Lua mancanti, ma NON per riscrivere
# la provenienza -- e' esattamente quello che questa prova verifica, non
# quanti modelli sono installati sulla macchina che esegue il test.
before=$(md5sum "$TMP/bundle-src/provenance.txt" | cut -d' ' -f1)
"$GEN" --from-json "$TMP/bundle-src/current_run.json" --resume --out "$TMP/bundle-src" >/dev/null
after=$(md5sum "$TMP/bundle-src/provenance.txt" | cut -d' ' -f1)
[ "$before" = "$after" ] || {
  echo "FALLITO: --resume ha riscritto provenance.txt (md5 prima=$before dopo=$after)"; exit 1; }

rm -rf bundles

echo "-- il binario del gioco non deve linkare llama.cpp o cJSON --"
! nm bin/melting_run_gpu | grep -qi -e llama -e cJSON

echo "-- esperimento due-modelli: --model-text non rompe --fallback --"
# --fallback salta a monte l'intero blocco di caricamento modello (main.c:
# 'if (!args.fallback)'), quindi --model-text qui non deve avere ALCUN
# effetto -- ne' un crash, ne' il tentativo di aprire un modello che non
# esiste: exit 0 e manifest scritto, come senza il flag.
"$GEN" --fallback --model-text qualunque --seed 12345 --out "$TMP/model-text-fallback"
grep -q "^floor5.item3.script=" "$TMP/model-text-fallback/current_run.txt" || {
  echo "FALLITO: --fallback --model-text non ha scritto un manifest completo"; exit 1; }

echo "-- determinismo fallback: stesso seed = stessi byte --"
"$GEN" --fallback --seed 12345 --out "$TMP/a"
"$GEN" --fallback --seed 12345 --out "$TMP/b"
cmp "$TMP/a/current_run.txt" "$TMP/b/current_run.txt"
cmp "$TMP/a/current_atlas.bmp" "$TMP/b/current_atlas.bmp"

echo "-- seed diverso = manifest diverso --"
"$GEN" --fallback --seed 999 --out "$TMP/c"
if cmp -s "$TMP/a/current_run.txt" "$TMP/c/current_run.txt"; then
  echo "FALLITO: seed diversi hanno prodotto lo stesso manifest"; exit 1
fi

echo "-- il manifest e' completo --"
grep -q "^floor5.item3.script=" "$TMP/a/current_run.txt"
grep -q "^atlas.path=" "$TMP/a/current_run.txt"

# Fase 3 (tassonomia degli oggetti, docs/superpowers/specs/2026-07-13-items-synergy-vision.md
# sezioni 1,2,5): un campo "kind" per oggetto, gli oggetti attivi vanno in
# items[1..3], l'oggetto stat-up del piano (ricompensa del boss) e' un
# quarto campo esplicito "bossItem". Round-trip attraverso il manifest di
# testo, per ciascuno dei 5 piani.
echo "-- fase 3: kind round-trips attraverso il manifest (attivi=active, boss=statup) --"
for n in 1 2 3 4 5; do
  for i in 1 2 3; do
    grep -q "^floor${n}.item${i}.kind=active$" "$TMP/a/current_run.txt" || {
      echo "FALLITO: floor${n}.item${i}.kind non e' 'active'"; exit 1; }
  done
  grep -q "^floor${n}.bossItem.kind=statup$" "$TMP/a/current_run.txt" || {
    echo "FALLITO: floor${n}.bossItem.kind non e' 'statup'"; exit 1; }
  grep -q "^floor${n}.bossItem.name=" "$TMP/a/current_run.txt" || {
    echo "FALLITO: floor${n}.bossItem.name mancante"; exit 1; }
  # Un oggetto stat-up non ha comportamento mini-VM: mai una riga ".script=".
  if grep -q "^floor${n}.bossItem.script=" "$TMP/a/current_run.txt"; then
    echo "FALLITO: floor${n}.bossItem ha una riga .script= (un oggetto stat-up non ha comportamento)"; exit 1
  fi
done


# Fase 3b (design doc, docs/superpowers/specs/2026-07-13-pools-rarity-design.md
# sezioni 1-3): ogni oggetto porta una rarita', il boss la tira SEMPRE da una
# tabella raro/leggendario (mai comune/non-comune), tesoro/negozio danno la
# mista. Round-trip attraverso il manifest di testo, per ciascuno dei 5 piani
# (stessa run --seed 12345 di sopra, $TMP/a).
echo "-- fase 3b: rarity round-trips attraverso il manifest (uno dei 4 livelli per ogni item, boss sempre raro/leggendario) --"
RARITY_RE='^(common|uncommon|rare|legendary)$'
for n in 1 2 3 4 5; do
  for i in 1 2 3; do
    line=$(grep "^floor${n}.item${i}.rarity=" "$TMP/a/current_run.txt" || true)
    [ -n "$line" ] || { echo "FALLITO: floor${n}.item${i}.rarity mancante"; exit 1; }
    value="${line#*=}"
    echo "$value" | grep -Eq "$RARITY_RE" || {
      echo "FALLITO: floor${n}.item${i}.rarity=$value non e' uno dei 4 livelli"; exit 1; }
  done
  bline=$(grep "^floor${n}.bossItem.rarity=" "$TMP/a/current_run.txt" || true)
  [ -n "$bline" ] || { echo "FALLITO: floor${n}.bossItem.rarity mancante"; exit 1; }
  bvalue="${bline#*=}"
  if [ "$bvalue" != "rare" ] && [ "$bvalue" != "legendary" ]; then
    echo "FALLITO: floor${n}.bossItem.rarity=$bvalue (atteso rare o legendary: il boss non delude mai)"; exit 1
  fi
done

# Su piu' semi (non solo 12345): il boss e' SEMPRE raro/leggendario, mai
# comune/non-comune, e gli oggetti attivi mostrano una MISTA di rarita' (non
# tutti sullo stesso livello) -- la prova che la tabella di pesi tesoro/
# negozio (55/30/12/3) e quella del boss (0/0/70/30) girano per davvero.
echo "-- fase 3b: il boss e' sempre raro/leggendario su piu' semi, gli attivi mostrano una mista --"
ACTIVE_RARITIES_SEEN=""
for seed in 1 2 3 7 42 100 31337; do
  "$GEN" --fallback --seed "$seed" --out "$TMP/rarity-$seed"
  manifest="$TMP/rarity-$seed/current_run.txt"
  bosses=$(grep '^floor[0-9]\.bossItem\.rarity=' "$manifest" | sed 's/.*=//' | sort -u)
  for b in $bosses; do
    if [ "$b" != "rare" ] && [ "$b" != "legendary" ]; then
      echo "FALLITO: seed=$seed ha un bossItem.rarity=$b (atteso sempre rare/legendary)"; exit 1
    fi
  done
  ACTIVE_RARITIES_SEEN="$ACTIVE_RARITIES_SEEN $(grep '^floor[0-9]\.item[0-9]\.rarity=' "$manifest" | sed 's/.*=//')"
done
distinctActive=$(echo "$ACTIVE_RARITIES_SEEN" | tr ' ' '\n' | sed '/^$/d' | sort -u | wc -l)
if [ "$distinctActive" -lt 2 ]; then
  echo "FALLITO: gli oggetti attivi tesoro/negozio mostrano un solo livello di rarita' su 7 semi (atteso una mista)"; exit 1
fi
echo "   rarita' distinte viste negli oggetti attivi su 7 semi: $distinctActive/4"

# Step C (docs/superpowers/specs/2026-07-14-step-c-shottype-balance.md): ogni
# piano porta UN tipo di colpo, su UNO dei tre oggetti attivi (mai sul bossItem:
# uno stat-up e' solo numeri). I tipi li INVENTA IL MODELLO -- qui si esercita il
# ripiego procedurale, che e' quello che il gioco vede quando il modello non c'e',
# ma il formato del manifest e le garanzie sono le stesse.
echo "-- step C: un tipo di colpo per piano, su un solo oggetto attivo, mai sul boss --"
SHOT_FORM_RE='^(orb|spike|beam|arc|blade)$'
for n in 1 2 3 4 5; do
  owners=$(grep -c "^floor${n}\.item[0-9]\.shotName=" "$TMP/a/current_run.txt" || true)
  if [ "$owners" -ne 1 ]; then
    echo "FALLITO: floor${n} ha $owners oggetti con un tipo di colpo (atteso esattamente 1)"; exit 1
  fi
  if grep -q "^floor${n}\.bossItem\.shot" "$TMP/a/current_run.txt"; then
    echo "FALLITO: floor${n}.bossItem ha un tipo di colpo (uno stat-up non cambia il modo di sparare)"; exit 1
  fi
  owner=$(grep "^floor${n}\.item[0-9]\.shotName=" "$TMP/a/current_run.txt" | sed 's/^floor[0-9]\.\(item[0-9]\)\..*/\1/')
  # Tutte le manopole devono esserci sull'oggetto che porta il tipo: un tipo di
  # colpo a meta' (nome ma niente forma, o forma ma niente numeri) verrebbe
  # ricostruito dal gioco coi valori neutri, cioe' sarebbe un colpo base con un
  # nome inventato: il dud che questa fase deve rendere impossibile.
  for field in shotForm shotSpeed shotDamage shotSize shotLife shotPierce shotChain shotPellets; do
    grep -q "^floor${n}\.${owner}\.${field}=" "$TMP/a/current_run.txt" || {
      echo "FALLITO: floor${n}.${owner}.${field} mancante"; exit 1; }
  done
  form=$(grep "^floor${n}\.${owner}\.shotForm=" "$TMP/a/current_run.txt" | sed 's/.*=//')
  echo "$form" | grep -Eq "$SHOT_FORM_RE" || {
    echo "FALLITO: floor${n}.${owner}.shotForm=$form non e' una forma nota"; exit 1; }
done

# Il vero contratto dello step C non e' "il manifest ha le righe giuste": e' che
# QUALUNQUE cosa il modello inventi resti bilanciata. Quel contratto e' verificato
# per davvero (768 combinazioni estreme + i due casi patologici) dal test R della
# suite --script-items-test, che gira in scripts/test-script.sh: qui basta la
# prova che il formato su disco regge il round-trip fino al gioco (il
# --manifest-test qui sotto carica proprio questo manifest).

# Step B2 (generazione pigra dei piani): il processo di RIPRESA gira in sottofondo
# MENTRE si gioca e riscrive il manifest per aggiungere gli script dei piani 2-5.
# Ha due modi non ovvi di rovinare la run, ed entrambi sarebbero silenziosi:
#   1. rimettere atlas.path al BMP di riserva, buttando via il PNG che
#      melting-sprites aveva appena prodotto (il gioco tornerebbe alla grafica
#      procedurale pur avendo gli sprite veri sul disco);
#   2. riscrivere il manifest SENZA le righe .lua= degli script gia' generati dal
#      primo processo (gli oggetti del piano 1 tornerebbero alla mini-VM).
# Qui si simulano entrambe le situazioni (nessun modello coinvolto: la ripresa
# senza modello non genera nulla, ma fa comunque tutta la riscrittura -- che e'
# esattamente la parte che questi due controlli devono proteggere).
echo "-- step B2: la ripresa preserva l'atlas degli sprite e gli script gia' scritti --"
"$GEN" --fallback --seed 4242 --out "$TMP/b2" >/dev/null
sed -i 's|^atlas\.path=.*|atlas.path=generated/current_atlas.png|' "$TMP/b2/current_run.txt"
mkdir -p "$TMP/b2/scripts"
printf 'function on_fire(x, y, dx, dy)\n  spawn_shot(x, y, dx, dy, 300, 3, 4, 0)\nend\n' > "$TMP/b2/scripts/floor1_item1.lua"

"$GEN" --from-json "$TMP/b2/current_run.json" --resume --out "$TMP/b2" >/dev/null

grep -q '^atlas.path=generated/current_atlas.png$' "$TMP/b2/current_run.txt" || {
  echo "FALLITO: la ripresa ha perso l'atlas PNG degli sprite (atlas.path riscritto sul BMP di riserva)"; exit 1; }
grep -q '^floor1.item1.lua=generated/scripts/floor1_item1.lua$' "$TMP/b2/current_run.txt" || {
  echo "FALLITO: la ripresa ha perso la riga .lua= di uno script gia' generato"; exit 1; }
grep -q '^floor5.item3.script=' "$TMP/b2/current_run.txt" || {
  echo "FALLITO: la ripresa ha prodotto un manifest incompleto"; exit 1; }
# La ripresa NON deve toccare i CONTENUTI della run. Il controllo confrontava solo
# "floor1.theme=", che era un falso verde (trovato in review): il tema del piano 1
# e' l'unica riga che la ripresa non potrebbe cambiare NEMMENO SE FOSSE ROTTA (la
# rete anti-fotocopia non sostituisce mai il piano 1, e il seed viene riletto dal
# JSON). Ora si confronta il manifest INTERO -- meno le sole righe che la ripresa
# ha il diritto di aggiungere (.lua=) e la riga "source=", che passa a "resume".
"$GEN" --fallback --seed 12345 --out "$TMP/b2b" >/dev/null
grep -vE '^(source=|floor[0-9]+\.(item[0-9]+|bossItem)\.lua=)' "$TMP/b2b/current_run.txt" > "$TMP/b2b-before.txt"
"$GEN" --from-json "$TMP/b2b/current_run.json" --resume --out "$TMP/b2b" >/dev/null
grep -vE '^(source=|floor[0-9]+\.(item[0-9]+|bossItem)\.lua=)' "$TMP/b2b/current_run.txt" > "$TMP/b2b-after.txt"
if ! diff -u "$TMP/b2b-before.txt" "$TMP/b2b-after.txt" > "$TMP/b2b.diff"; then
  echo "FALLITO: la ripresa ha cambiato i contenuti della run (deve solo AGGIUNGERE righe .lua=):"
  head -20 "$TMP/b2b.diff"
  exit 1
fi

# Una generazione NUOVA (non di ripresa) deve ripulire gli script della run
# PRECEDENTE. Senza, la ripresa della run nuova li adotterebbe (GenLuaLoadExisting):
# il comportamento inventato per un oggetto di ieri finirebbe addosso a un oggetto
# di oggi, con nome/tema/trait che non c'entrano nulla. Silenzioso: lo script e'
# valido, semplicemente non e' il suo.
echo "-- step B2: una generazione nuova non eredita gli script Lua della run precedente --"
mkdir -p "$TMP/b2c/scripts"
printf 'function on_fire(x, y, dx, dy)\n  spawn_shot(x, y, dx, dy, 300, 3, 4, 0)\nend\n' > "$TMP/b2c/scripts/floor3_item2.lua"
"$GEN" --fallback --seed 777 --out "$TMP/b2c" >/dev/null
if [ -f "$TMP/b2c/scripts/floor3_item2.lua" ]; then
  echo "FALLITO: una generazione nuova ha lasciato sul disco lo script Lua di una run precedente"; exit 1
fi
# ...ma una RIPRESA quegli script li deve conservare (sono i suoi).
printf 'function on_fire(x, y, dx, dy)\n  spawn_shot(x, y, dx, dy, 300, 3, 4, 0)\nend\n' > "$TMP/b2c/scripts/floor3_item2.lua"
"$GEN" --from-json "$TMP/b2c/current_run.json" --resume --out "$TMP/b2c" >/dev/null
if [ ! -f "$TMP/b2c/scripts/floor3_item2.lua" ]; then
  echo "FALLITO: la ripresa ha cancellato uno script Lua della propria run"; exit 1
fi

# Fase 3b: ogni piano porta DUE tipi di nemico + il tipo del boss, inventati dal
# modello (qui si esercita il ripiego procedurale, ma il formato e le garanzie sono
# gli stessi). Il contratto vero -- "qualunque nemico inventi il modello resta in
# banda di potenza" -- e' verificato dai test della suite --script-items-test.
echo "-- fase 3b: due tipi di nemico + il boss per piano, con tutte le manopole --"
FOE_FORM_RE='^(blob|spiky|armored|floater)$'
FOE_MOVE_RE='^(chase|kite|orbit|zigzag|charge)$'
FOE_FIRE_RE='^(none|single|spread|ring)$'
for n in 1 2 3 4 5; do
  for who in enemy1 enemy2 bossType; do
    for field in name form move fire hp speed size rate pellets; do
      grep -q "^floor${n}\.${who}\.${field}=" "$TMP/a/current_run.txt" || {
        echo "FALLITO: floor${n}.${who}.${field} mancante"; exit 1; }
    done
    form=$(grep "^floor${n}\.${who}\.form=" "$TMP/a/current_run.txt" | sed 's/.*=//')
    move=$(grep "^floor${n}\.${who}\.move=" "$TMP/a/current_run.txt" | sed 's/.*=//')
    fire=$(grep "^floor${n}\.${who}\.fire=" "$TMP/a/current_run.txt" | sed 's/.*=//')
    echo "$form" | grep -Eq "$FOE_FORM_RE" || { echo "FALLITO: floor${n}.${who}.form=$form sconosciuta"; exit 1; }
    echo "$move" | grep -Eq "$FOE_MOVE_RE" || { echo "FALLITO: floor${n}.${who}.move=$move sconosciuto"; exit 1; }
    echo "$fire" | grep -Eq "$FOE_FIRE_RE" || { echo "FALLITO: floor${n}.${who}.fire=$fire sconosciuto"; exit 1; }
  done
done

# Fase 3c: ogni piano porta un layout di stanza (forma + densita' + nome), inventato
# dal modello. Qui si esercita il ripiego procedurale, ma formato e garanzie sono gli
# stessi. La garanzia vera -- la stanza resta SEMPRE giocabile -- e' verificata dai
# test della suite --script-items-test (croce centrale sempre libera).
echo "-- fase 3c: un layout di stanza per piano (forma nota + densita') --"
ROOM_FORM_RE='^(open|pillars|corridor|arena|scatter)$'
for n in 1 2 3 4 5; do
  # il ripiego procedurale non genera mai "open", quindi tutte le righe ci sono
  for field in name form density; do
    grep -q "^floor${n}\.room\.${field}=" "$TMP/a/current_run.txt" || {
      echo "FALLITO: floor${n}.room.${field} mancante"; exit 1; }
  done
  form=$(grep "^floor${n}\.room\.form=" "$TMP/a/current_run.txt" | sed 's/.*=//')
  echo "$form" | grep -Eq "$ROOM_FORM_RE" || { echo "FALLITO: floor${n}.room.form=$form sconosciuta"; exit 1; }
done

echo "-- il gioco carica il manifest generato --"
"$GEN" --fallback --seed 4242 --out generated
"${GAME_RUN[@]}" bin/melting_run_gpu --manifest-test

echo "-- una generazione riuscita sostituisce davvero i contenuti in generated/ --"
# Non basta che i test siano verdi con una generazione sola: una regressione
# che mettesse in cache i contenuti (o ignorasse il nuovo manifest) lascerebbe
# per sempre la run precedente senza far fallire nessun altro controllo di
# questo script. Si generano due run con semi diversi nella stessa generated/,
# si verifica che il tema del piano 1 cambi, e che il gioco continui a
# caricare correttamente il manifest piu' recente.
"$GEN" --fallback --seed 12345 --out generated
themeA=$(grep '^floor1.theme=' generated/current_run.txt)
"$GEN" --fallback --seed 999 --out generated
themeB=$(grep '^floor1.theme=' generated/current_run.txt)
if [ "$themeA" = "$themeB" ]; then
  echo "FALLITO: due semi diversi hanno prodotto lo stesso floor1.theme (contenuti non aggiornati?)"; exit 1
fi
"${GAME_RUN[@]}" bin/melting_run_gpu --manifest-test

echo "-- coerenza writer C <-> grammatica GBNF --"
GBNF=deps/llama.cpp/build/bin/test-gbnf-validator
"$GEN" --fallback --seed 1 --out "$TMP/g" --emit-llm-json
"$GBNF" tools/melting-gen/run.gbnf "$TMP/g/llm_sample.json" | grep -q "is valid"

echo "-- la grammatica rifiuta un enum sbagliato --"
sed 's/"hat"/"hut"/; s/"eyes"/"eyez"/' "$TMP/g/llm_sample.json" > "$TMP/g/broken.json"
if "$GBNF" tools/melting-gen/run.gbnf "$TMP/g/broken.json" | grep -q "is valid"; then
  echo "FALLITO: la grammatica ha accettato uno slot inesistente"; exit 1
fi

echo "-- corpus JSON rotti: normalizzati senza crash, manifest completo --"
for f in tests/melting-gen/bad/*.json; do
  "$GEN" --from-json "$f" --seed 7 --out "$TMP/bad"
  grep -q "^floor5.item3.script=" "$TMP/bad/current_run.txt" || { echo "FALLITO su $f"; exit 1; }
done

echo "-- normalizzazioni puntuali --"
"$GEN" --from-json tests/melting-gen/bad/wrong-op-pair.json --seed 7 --out "$TMP/n1"
grep -q "^floor1.item1.script=on_fire:burst,2,1.2,homing$" "$TMP/n1/current_run.txt"
"$GEN" --from-json tests/melting-gen/bad/out-of-range.json --seed 7 --out "$TMP/n2"
grep -q "^floor1.item1.script=on_fire:burst,6,1.2,split|on_hit:heal,60,2,vamp$" "$TMP/n2/current_run.txt"

echo "-- JSON non parsabile -> exit 4 --"
set +e
"$GEN" --from-json tests/melting-gen/unparseable.txt --seed 7 --out "$TMP/x"
rc=$?
set -e
[ "$rc" -eq 4 ]

# Golden file di regressione: blocca l'ordine di estrazione dall'RNG del
# fallback (hue -> per-item trait/nome/slot x3 -> theme/weird -> style),
# le liste di parole e gli arrotondamenti. Un cambio accidentale a uno di
# questi tre punti produce un run diverso a parita' di seed senza far
# fallire nessun altro controllo di questo script (che confronta solo
# C-contro-C), quindi qui confrontiamo l'output con un file di riferimento
# generato una volta e committato.
echo "-- golden file: seed 12345 = manifest di riferimento --"
"$GEN" --fallback --seed 12345 --out "$TMP/golden"
cmp "$TMP/golden/current_run.txt" tests/melting-gen/golden-fallback-seed12345.txt

# Fase 3a-L3: il validatore Lua di melting-gen (gen_lua.c), senza alcun
# modello. Il corpus (tests/melting-gen/lua/) copre lo stesso elenco della
# spec (sezione 2): sintassi, ciclo infinito, bomba di memoria, funzione
# proibita (io.open), funzione inesistente, handle stantio. --lua-check
# stampa VALID/REJECTED ed esce 0/1: buono per gli assert qui sotto.
echo "-- corpus Lua: melting-gen accetta lo script buono --"
"$GEN" --lua-check tests/melting-gen/lua/good.lua

echo "-- corpus Lua: melting-gen rifiuta ciascuno degli script rotti --"
for f in tests/melting-gen/lua/*.lua; do
  base=$(basename "$f")
  [ "$base" = "good.lua" ] && continue
  if "$GEN" --lua-check "$f"; then
    echo "FALLITO: $base doveva essere rifiutato dal validatore ed e' stato accettato"; exit 1
  fi
done

# Fase 3b review ("un guard automatico contro una futura re-inflazione dei
# prompt"): stesso motivo del blocco Lua sopra, senza alcun modello. Vedi
# tools/melting-gen/gen_lua.h (GenLuaPromptBudgetCheck, il commento sopra
# GEN_LUA_PROMPT_BYTE_CEILING) per il bug reale che questa guardia previene
# (hint di rarita' scritti come frasi intere hanno fatto sforare n_ctx per
# OGNI prompt Lua di una run, 0/20 script generati, senza che `make test-gen`
# se ne accorgesse: solo un giro reale con `make test-llm` lo mostrava nel
# log). Deve passare con i prompt di oggi (brevi).
echo "-- guardia byte-budget del prompt Lua: il prompt piu' grande di oggi sta sotto il ceiling --"
"$GEN" --prompt-budget-check

# Piano strategico 16/07/2026, sezione tier: --bench senza modello. --model-fallback
# esiste SOLO per questo test (vedi il commento su ParseArgs in main.c): senza,
# un --model su un percorso inesistente ripiegherebbe comunque sul modello
# piccolo vero se e' scaricato in models/ (come nell'ambiente di sviluppo), e il
# test finirebbe per caricare davvero un modello sulla GPU invece di esercitare
# il ramo "nessun modello disponibile".
echo "-- --bench senza modello: exit 1, la cartella --out resta byte per byte identica --"
mkdir -p "$TMP/bench-nomodel"
echo marker > "$TMP/bench-nomodel/marker.txt"
before=$(find "$TMP/bench-nomodel" -type f -exec sha256sum {} \; | sort)
set +e
"$GEN" --bench --model "$TMP/nonexistent-main.gguf" --model-fallback "$TMP/nonexistent-fallback.gguf" \
       --out "$TMP/bench-nomodel" >"$TMP/bench-nomodel.out" 2>"$TMP/bench-nomodel.err"
rc=$?
set -e
[ "$rc" -eq 1 ]
[ -z "$(cat "$TMP/bench-nomodel.out")" ]   # niente riga "bench: ..." su stdout quando fallisce
after=$(find "$TMP/bench-nomodel" -type f -exec sha256sum {} \; | sort)
[ "$before" = "$after" ]
grep -q "nessun modello disponibile" "$TMP/bench-nomodel.err"

# M5 (DEC-005, scelta del tema nel Piano 0): --propose-themes senza modello e'
# deterministico e valido (schema JSON minimo, source=fallback), e stabile su
# un golden file di riferimento -- stesso schema del golden-fallback sopra:
# blocca cambi silenziosi al pool/all'RNG di GenFallbackThemeProposals.
echo "-- --propose-themes: senza modello e' deterministico e produce JSON valido --"
"$GEN" --propose-themes 3 --seed 12345 --model "$TMP/nonexistent-main.gguf" \
       --model-fallback "$TMP/nonexistent-fallback.gguf" --out "$TMP/propose-golden"
cmp "$TMP/propose-golden/theme_proposals.json" tests/melting-gen/golden-theme-proposals-seed12345.txt
python3 - "$TMP/propose-golden/theme_proposals.json" <<'PYEOF'
import json, sys
d = json.load(open(sys.argv[1]))
assert d["source"] == "fallback", d["source"]
props = d["proposals"]
assert len(props) == 3, len(props)
names = [p["name"] for p in props]
assert len(set(n.lower() for n in names)) == 3, names   # 3 nomi distinti
for p in props:
    assert 3 <= len(p["name"]) <= 40, p["name"]
    assert p["blurb"], p
    assert all(ord(c) < 128 for c in p["name"] + p["blurb"])   # ASCII puro (DEC-052)
PYEOF

echo "-- --propose-themes: N=2 scrive solo 2 proposte --"
"$GEN" --propose-themes 2 --seed 12345 --model "$TMP/nonexistent-main.gguf" \
       --model-fallback "$TMP/nonexistent-fallback.gguf" --out "$TMP/propose-n2"
python3 -c "
import json
d = json.load(open('$TMP/propose-n2/theme_proposals.json'))
assert len(d['proposals']) == 2, d['proposals']
"

echo "-- --propose-themes: stesso seed due volte -> proposte identiche --"
"$GEN" --propose-themes 3 --seed 777 --model "$TMP/nonexistent-main.gguf" \
       --model-fallback "$TMP/nonexistent-fallback.gguf" --out "$TMP/propose-a"
"$GEN" --propose-themes 3 --seed 777 --model "$TMP/nonexistent-main.gguf" \
       --model-fallback "$TMP/nonexistent-fallback.gguf" --out "$TMP/propose-b"
cmp "$TMP/propose-a/theme_proposals.json" "$TMP/propose-b/theme_proposals.json"

# --theme-file + --print-json-prompt: entrambi i rami di {CHOSEN_THEME}
# sostituiti (requisito 4), MAI il placeholder grezzo nel prompt stampato.
echo "-- --theme-file: {CHOSEN_THEME} sostituito col tema scelto --"
printf 'Foundry of Glass -- a cathedral of molten glass where the choir never stops singing.' > "$TMP/chosen-theme.txt"
"$GEN" --print-json-prompt --seed 42 --theme-file "$TMP/chosen-theme.txt" --out "$TMP/theme-prompt" \
       > "$TMP/theme-prompt.txt"
grep -q "{CHOSEN_THEME}" "$TMP/theme-prompt.txt" && {
  echo "FALLITO: {CHOSEN_THEME} non sostituito con --theme-file"; exit 1; }
grep -q "This run's world: Foundry of Glass -- a cathedral of molten glass" "$TMP/theme-prompt.txt" || {
  echo "FALLITO: il testo del tema scelto non compare nel prompt"; exit 1; }
grep -q "Stay inside this world" "$TMP/theme-prompt.txt" || {
  echo "FALLITO: manca la frase di rinforzo del tema scelto"; exit 1; }

echo "-- senza --theme-file: {CHOSEN_THEME} degrada in modo pulito --"
"$GEN" --print-json-prompt --seed 42 --out "$TMP/notheme-prompt" > "$TMP/notheme-prompt.txt"
grep -q "{CHOSEN_THEME}" "$TMP/notheme-prompt.txt" && {
  echo "FALLITO: {CHOSEN_THEME} non sostituito senza --theme-file"; exit 1; }
grep -q "not chosen this time" "$TMP/notheme-prompt.txt" || {
  echo "FALLITO: manca il ramo di degrado di {CHOSEN_THEME}"; exit 1; }

echo "-- --theme-file: file inesistente si comporta come flag assente --"
"$GEN" --print-json-prompt --seed 42 --theme-file "$TMP/does-not-exist.txt" --out "$TMP/theme-missing" \
       > "$TMP/theme-missing.txt"
cmp "$TMP/theme-missing.txt" "$TMP/notheme-prompt.txt" || {
  echo "FALLITO: --theme-file su un file mancante ha cambiato il prompt"; exit 1; }

# --fallback con tema scelto: onora il tema su tutti e 5 i piani, con nomi
# distinti (la guardia anti-fotocopia resta valida: 5 STRINGHE diverse anche
# se il "place" e' lo stesso, vedi prompts/user.txt).
echo "-- --fallback con tema scelto: 5 piani, tutti derivati dal tema, tutti distinti --"
"$GEN" --fallback --seed 999 --theme-file "$TMP/chosen-theme.txt" --out "$TMP/theme-fallback"
themes=$(grep -E "^floor[0-9]\.theme=" "$TMP/theme-fallback/current_run.txt" | cut -d= -f2)
echo "$themes" | grep -q "^Foundry of Glass$" || {
  echo "FALLITO: floor1.theme non e' il tema scelto alla lettera"; exit 1; }
[ "$(echo "$themes" | wc -l)" -eq 5 ] || { echo "FALLITO: non ci sono 5 righe floorN.theme"; exit 1; }
[ "$(echo "$themes" | sort -u | wc -l)" -eq 5 ] || {
  echo "FALLITO: i 5 floorN.theme col tema scelto non sono tutti distinti"; exit 1; }
echo "$themes" | tail -n +2 | while IFS= read -r t; do
  case "$t" in
    "Foundry of Glass, "*) ;;
    *) echo "FALLITO: '$t' non porta il place del tema scelto come prefisso"; exit 1 ;;
  esac
done
grep -q "^chosenTheme=Foundry of Glass -- a cathedral of molten glass" "$TMP/theme-fallback/provenance.txt" || {
  echo "FALLITO: provenance.txt non porta chosenTheme"; exit 1; }

echo "-- --fallback SENZA tema: provenance.txt porta chosenTheme=none (nessuna regressione) --"
"$GEN" --fallback --seed 999 --out "$TMP/no-theme-fallback"
grep -q "^chosenTheme=none$" "$TMP/no-theme-fallback/provenance.txt" || {
  echo "FALLITO: provenance.txt senza --theme-file non riporta chosenTheme=none"; exit 1; }

echo "TEST-GEN: OK"
