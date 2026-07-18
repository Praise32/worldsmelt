#!/usr/bin/env bash
# Generazione reale con modello. Variabili: MODEL, NGL, SEED.
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

MODEL="${MODEL:-models/qwen2.5-coder-1.5b-instruct-q4_k_m.gguf}"
NGL="${NGL:-99}"
SEED="${SEED:-31337}"
[ -f "$MODEL" ] || { echo "Modello mancante: $MODEL — esegui scripts/download-models.sh"; exit 1; }

bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$SEED" --out generated
grep -q "^source=local:" generated/current_run.txt
grep -q "^floor5.item3.script=" generated/current_run.txt

# Guardia di lingua (DEC-052, generazione contenuti inglese-first): l'unico
# test che genera per davvero col modello, quindi l'unico posto dove ha senso
# controllare la lingua VERA prodotta (make test-gen resta "senza modello",
# vedi gen_lua.h). Parole-funzione italiane (preposizioni articolate,
# preposizione semplice "di", congiunzione "e") a CONFINE DI PAROLA sui campi
# generati dal modello (tema/stile/nomi, mai i campi tecnici come colore o
# form): un match e' il modello che e' scivolato in italiano nonostante il
# prompt in inglese. Confine di parola (\b) e' cio' che tiene il falso-
# positivo basso: "del"/"di"/"e" come SOTTOSTRINGA compaiono in nomi inglesi
# legittimi (Divine, Delve, Delight, Emblem, Elemental...) ma mai come parola
# intera separata da spazi -- verificato anche negli esempi few-shot di
# gen_inspire.c (Rotunda of Judgment, Drifting Jellyfish, Hall of Mirrors...:
# nessun falso positivo).
italianWordPattern='\b(del|della|dei|degli|delle|di|e)\b'
generatedFields=$(grep -E '^floor[0-9]+\.(theme|style|boss|enemy[0-9]+\.name|bossType\.name|room\.name|item[0-9]+\.name|item[0-9]+\.shotName|bossItem\.name)=' generated/current_run.txt)
italianHit=$(echo "$generatedFields" | grep -Ei "$italianWordPattern" || true)
if [ -n "$italianHit" ]; then
  echo "FALLITO: parola-funzione italiana a confine di parola in un campo generato dal modello (DEC-052 richiede inglese):"
  echo "$italianHit"
  exit 1
fi
# --manifest-test (fase 3a-L3) carica anche gli script Lua presenti nel
# manifest in una sandbox vera e asserisce che compilino: vedi
# src/tests/game_tests.c, GameManifestTest.
"${GAME_RUN[@]}" bin/melting_run_gpu --manifest-test
# Step C review (bug REALE, trovato con una generazione vera al seed 20260714):
# la finestra della penalita' sulle ripetizioni era piu' CORTA di un piano di JSON
# (256 token contro ~370), quindi ricopiare il piano precedente non costava nulla
# al modello -- e su alcuni seed produceva CINQUE PIANI FOTOCOPIA: stesso tema,
# stesso boss, stessi oggetti, stesso tipo di colpo. Nessun test se ne accorgeva:
# make test-gen non tocca il modello, e il manifest era formalmente PERFETTO.
# Questo e' il posto giusto per la guardia -- l'unico test che il modello vero lo
# usa davvero. Non giudica il gusto (non e' testabile), giudica la VARIETA', che e'
# la promessa minima di un generatore di contenuti.
echo "--- varieta': 5 piani diversi, 5 tipi di colpo diversi (guardia anti-fotocopia) ---"
distinctThemes=$(grep -E '^floor[0-9]\.theme=' generated/current_run.txt | sed 's/.*=//' | sort -u | wc -l)
if [ "$distinctThemes" -lt 5 ]; then
  echo "FALLITO: solo $distinctThemes temi distinti su 5 -- il modello sta ricopiando i piani"
  grep -E '^floor[0-9]\.theme=' generated/current_run.txt
  exit 1
fi
shotNames=$(grep -E '^floor[0-9]\.item[0-9]\.shotName=' generated/current_run.txt | sed 's/.*=//')
shotCount=$(echo "$shotNames" | sed '/^$/d' | wc -l)
distinctShots=$(echo "$shotNames" | sed '/^$/d' | sort -u | wc -l)
if [ "$shotCount" -ne 5 ]; then
  echo "FALLITO: $shotCount tipi di colpo nel manifest (atteso 1 per piano = 5)"; exit 1
fi
if [ "$distinctShots" -lt 5 ]; then
  echo "FALLITO: solo $distinctShots tipi di colpo distinti su 5 -- il modello li sta ricopiando"
  echo "$shotNames"
  exit 1
fi
echo "   temi distinti: $distinctThemes/5 | tipi di colpo distinti: $distinctShots/5"
echo "$shotNames" | sed 's/^/   colpo inventato: /'

echo "--- ultima riga di log (tempi e tok/s) ---"
grep "^\[.*\] ok: model=" logs/melting-gen.log | tail -1

echo "--- riepilogo Lua (fase 3a-L3): quanti dei 15 oggetti hanno preso uno script funzionante ---"
grep "lua: riepilogo run" logs/melting-gen.log | tail -1

echo "--- script Lua scritti in generated/scripts/ ---"
ls -1 generated/scripts/*.lua 2>/dev/null || echo "(nessuno: tutti gli oggetti sono ripiegati sulla mini-VM)"

# M5 (DEC-005), requisito 12: --theme-file con un modello VERO -- floor1.theme
# deriva dal tema scelto, i 5 temi restano distinti (guardia anti-fotocopia
# invariata), la guardia di lingua vale anche qui. Cartella --out SEPARATA
# (mai generated/, gia' verificata dai controlli sopra): un secondo processo
# non deve toccare l'output della prima generazione.
echo "--- M5: --theme-file con un modello vero -- il tema scelto guida i 5 piani ---"
THEME_TMP=$(mktemp -d)
trap 'rm -rf "$THEME_TMP"' EXIT
printf 'Foundry of Glass -- a cathedral of molten glass where the choir never stops singing.' > "$THEME_TMP/chosen-theme.txt"
bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$((SEED + 1))" \
                 --theme-file "$THEME_TMP/chosen-theme.txt" --out "$THEME_TMP/run"
grep -q "^floor1.theme=Foundry of Glass$" "$THEME_TMP/run/current_run.txt" || {
  echo "FALLITO: floor1.theme non e' il tema scelto alla lettera"
  grep '^floor1.theme=' "$THEME_TMP/run/current_run.txt"
  exit 1
}
m5Themes=$(grep -E '^floor[0-9]\.theme=' "$THEME_TMP/run/current_run.txt" | sed 's/.*=//')
m5DistinctThemes=$(echo "$m5Themes" | sort -u | wc -l)
if [ "$m5DistinctThemes" -lt 5 ]; then
  echo "FALLITO: solo $m5DistinctThemes temi distinti su 5 con --theme-file -- il modello sta ricopiando i piani"
  echo "$m5Themes"
  exit 1
fi
m5ItalianHit=$(grep -E '^floor[0-9]+\.(theme|style|boss|enemy[0-9]+\.name|bossType\.name|room\.name|item[0-9]+\.name|item[0-9]+\.shotName|bossItem\.name)=' \
  "$THEME_TMP/run/current_run.txt" | grep -Ei "$italianWordPattern" || true)
if [ -n "$m5ItalianHit" ]; then
  echo "FALLITO: parola-funzione italiana a confine di parola in un campo generato dal modello con --theme-file:"
  echo "$m5ItalianHit"
  exit 1
fi
grep -q "^chosenTheme=Foundry of Glass -- a cathedral of molten glass" "$THEME_TMP/run/provenance.txt" || {
  echo "FALLITO: provenance.txt non porta chosenTheme con --theme-file"
  exit 1
}
echo "   floor1.theme (tema scelto alla lettera): $(grep '^floor1.theme=' "$THEME_TMP/run/current_run.txt" | sed 's/.*=//')"
echo "   temi dei 5 piani (tutti derivati dal tema scelto, tutti distinti):"
echo "$m5Themes" | sed 's/^/     /'
rm -rf "$THEME_TMP"
trap - EXIT

# M6b-1 (DEC-014, prima fetta): --propose-themes con un modello VERO genera
# ANCHE il personaggio alternativo per-run, nella STESSA sessione (mai un
# secondo caricamento) -- l'unico test che campiona una proposta di
# personaggio VERA (make test-gen resta "senza modello", vede solo
# l'assenza della carta). Riusa generated/ (gia' validata sopra dai
# controlli sul JSON dei piani): --propose-themes non tocca current_run.txt.
echo "--- M6b-1: --propose-themes con un modello vero -- personaggio generato campionato ---"
rm -f generated/character_proposal.json
bin/melting-gen --model "$MODEL" --ngl "$NGL" --seed "$((SEED + 2))" --propose-themes 3 --out generated
if [ ! -f generated/character_proposal.json ]; then
  echo "FALLITO: generated/character_proposal.json non scritto da --propose-themes con un modello vero"
  exit 1
fi
python3 - generated/character_proposal.json <<'PYEOF'
import json, sys
d = json.load(open(sys.argv[1]))
assert d["name"], d
assert d["blurb"], d
assert all(ord(c) < 128 for c in d["name"] + d["blurb"]), "personaggio generato non ASCII"
s = d["stats"]
assert 6.0 <= s["damage"] <= 11.0, s
assert 0.19 <= s["fireDelay"] <= 0.28, s
assert 480.0 <= s["shotSpeed"] <= 560.0, s
assert 190.0 <= s["speed"] <= 260.0, s
assert 3 <= s["maxHp"] <= 9, s
assert 0.0 <= s["luck"] <= 1.5, s
pal = d["palette"]
assert pal.startswith("#") and len(pal) == 7, pal
assert d["source"].startswith("local:"), d["source"]
PYEOF
charName=$(python3 -c "import json; print(json.load(open('generated/character_proposal.json'))['name'])")
charBlurb=$(python3 -c "import json; print(json.load(open('generated/character_proposal.json'))['blurb'])")
charHit=$(printf '%s\n%s\n' "$charName" "$charBlurb" | grep -Ei "$italianWordPattern" || true)
if [ -n "$charHit" ]; then
  echo "FALLITO: parola-funzione italiana nel personaggio generato (DEC-052 richiede inglese):"
  echo "$charHit"
  exit 1
fi
echo "   personaggio generato: $charName -- $charBlurb"

echo "TEST-LLM: OK"
