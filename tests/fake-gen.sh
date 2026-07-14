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
