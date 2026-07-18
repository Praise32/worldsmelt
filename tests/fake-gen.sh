#!/usr/bin/env bash
# Finto melting-gen per collaudare src/gen/gen_runner senza modelli.
# FAKE_GEN_MODE: ok (default) | fail | hang | args.  FAKE_GEN_OUT: cartella output.
# "args" (step B2): scrive la propria riga di comando in $FAKE_GEN_OUT/fake-gen-args.txt
# e basta. Serve a verificare che GenRunnerStartWithArgs consegni davvero al figlio
# gli argomenti in piu' (--from-json/--resume/--lua-first): senza questo controllo,
# una argv costruita male produrrebbe un melting-gen che ignora la ripresa e
# rigenera una run DIVERSA da quella che si sta giocando, in silenzio.
out="${FAKE_GEN_OUT:-generated}"
mkdir -p "$out"
prog() {
  printf '%s|%s|%s\n' "$1" "$2" "$3" > "$out/gen_progress.tmp"
  mv "$out/gen_progress.tmp" "$out/gen_progress.txt"
}

# M5 (DEC-005): --propose-themes e' un comando DIVERSO dalla generazione
# completa (stesso binario/finto script, argomenti diversi -- vedi
# AppStartProposeThemes in src/app/app.c) -- controllato SEPARATAMENTE da
# FAKE_GEN_PROPOSE_MODE (ok default | fail | hang), mai da FAKE_GEN_MODE:
# i test hanno bisogno di far succedere il propose e fallire/appendere la
# generazione completa (o viceversa) nello STESSO scenario.
is_propose=0
for a in "$@"; do
  if [ "$a" = "--propose-themes" ]; then is_propose=1; fi
done
if [ "$is_propose" = "1" ]; then
  case "${FAKE_GEN_PROPOSE_MODE:-ok}" in
    fail)
      prog errore 100 "finte proposte fallite"
      exit 3
      ;;
    hang)
      prog avvio 5 "finto propose infinito"
      sleep 30
      exit 0
      ;;
    *)
      cat > "$out/theme_proposals.json" <<'EOF'
{"proposals":[{"name":"Fake Orchard","blurb":"A test blurb for the first fake proposal in this scenario."},{"name":"Fake Cistern","blurb":"A test blurb for the second fake proposal in this scenario."},{"name":"Fake Belltower","blurb":"A test blurb for the third fake proposal in this scenario."}],"source":"fallback"}
EOF
      prog fine 100 "finte proposte pronte"
      exit 0
      ;;
  esac
fi

case "${FAKE_GEN_MODE:-ok}" in
  args)
    printf '%s\n' "$*" > "$out/fake-gen-args.txt"
    prog fine 100 "argomenti registrati"
    exit 0
    ;;
  hang)
    prog carico-modello 10 "finto caricamento infinito"
    sleep 30
    exit 0
    ;;
  fail)
    prog errore 100 "errore simulato"
    exit 3
    ;;
  *)
    prog carico-modello 30 "finto caricamento"
    sleep 0.2
    prog genero 70 "finta generazione"
    sleep 0.2
    prog fine 100 "finto manifest pronto"
    exit 0
    ;;
esac
