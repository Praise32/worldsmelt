# Issue notes

Queste note servono a non perdere i problemi incontrati durante lo sviluppo.
Non sono tutte issue da aprire su GitHub: alcune sono configurazioni locali,
altre sono limiti del prototipo.

## Risolte in questo progetto

- Gli script `.bat` di generazione potevano perdere l'errorlevel reale di Node
  dopo `popd`. Risolto salvando `%ERRORLEVEL%` prima di uscire.
- Il fallback atlas BMP aveva sfondo opaco dentro ogni cella, quindi il player
  appariva dentro un quadrato scuro. Risolto lasciando trasparenti i pixel non
  disegnati.
- Il portale boss poteva sparire cambiando stanza. Regression test presente:
  `bin\melting_run_gpu.exe --portal-test`.
- Gli spritesheet IA potevano essere belli ma non tagliabili in celle perfette.
  Migliorato usando un prompt tecnico molto vincolato, il PNG Image API come
  atlas principale e un chroma-key runtime per rimuovere lo sfondo quasi nero.
  Il BMP locale resta disponibile con `--local-atlas`.
- Alcuni script LLM erano validi nel JSON ma non nel runtime, per esempio
  `on_fire:projectile`. Risolto normalizzando trigger, operazioni e trait prima
  di scrivere il manifest.

## Configurazione locale, non issue della repository

- Senza `OPENAI_API_KEY`, la generazione OpenAI non parte. Il progetto usa il
  fallback locale e non consuma crediti.
- La Image API puo' fallire per quota, rete, modello non disponibile o
  verifica organizzazione OpenAI non completata. In questi casi va controllato
  l'account OpenAI prima di aprire issue sul codice.
- Lo screenshot test puo' mostrare `0 FPS` perche' cattura subito il primo frame.
  Durante il gameplay il contatore FPS si aggiorna.

## Da verificare prima di aprire issue

- I PNG della Image API possono ancora avere celle non perfette, anche con un
  prompt tecnico. Possibile miglioramento: validatore visuale prima di
  accettare la PNG come atlas giocabile.
- I PNG generati da Image API possono avere sfondo opaco. Possibile
  miglioramento: chroma-key configurabile o richiesta di background coerente.
- Le sinergie sandboxate sono sicure ma ancora semplici. Possibile
  miglioramento: aggiungere piu' operazioni dichiarative senza passare subito a
  Lua.
