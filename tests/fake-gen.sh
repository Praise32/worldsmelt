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
      # M6b-1 (DEC-014): il personaggio generato per-run viaggia nella STESSA
      # chiamata --propose-themes (mai un secondo processo) -- FAKE_GEN_CHARACTER_MODE
      # e' un interruttore SEPARATO da FAKE_GEN_PROPOSE_MODE (come quest'ultimo lo e'
      # da FAKE_GEN_MODE sopra): i test hanno bisogno di far succedere i temi e
      # variare indipendentemente l'esito del personaggio (assente/in banda/fuori
      # banda) nello STESSO scenario. "none" non scrive il file (fallback canonico
      # del personaggio generato = carta assente, characters.md); "outofband" scrive
      # stats deliberatamente fuori dalle bande (per il test del clamp alla lettura,
      # lato gioco); il default "ok" scrive una proposta valida e in banda.
      case "${FAKE_GEN_CHARACTER_MODE:-ok}" in
        none)
          rm -f "$out/character_proposal.json"
          ;;
        outofband)
          cat > "$out/character_proposal.json" <<'EOF'
{"name":"Fake Overclock","blurb":"A test blurb for an out-of-band fake character proposal.","stats":{"damage":99,"fireDelay":0.05,"shotSpeed":900,"speed":50,"maxHp":40,"luck":9},"palette":"#ff00aa","source":"local:fake-model.gguf"}
EOF
          ;;
        *)
          cat > "$out/character_proposal.json" <<'EOF'
{"name":"Fake Ember Twin","blurb":"A test blurb for the fake generated character in this scenario.","stats":{"damage":9,"fireDelay":0.22,"shotSpeed":520,"speed":215,"maxHp":7,"luck":0.8},"palette":"#cc7733","source":"local:fake-model.gguf"}
EOF
          ;;
      esac
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
