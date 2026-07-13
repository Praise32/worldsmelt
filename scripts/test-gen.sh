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
trap 'rm -rf "$TMP"' EXIT

echo "-- il binario del gioco non deve linkare llama.cpp o cJSON --"
! nm bin/melting_run_gpu | grep -qi -e llama -e cJSON

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

echo "TEST-GEN: OK"
