# Worldsmelt

Action roguelite top-down in C99 + raylib in cui **i contenuti di ogni run li inventa
un'IA locale**: temi dei piani, oggetti (con comportamento scritto in **Lua sandboxato**),
tipi di colpo, nemici, boss, stanze e perfino un personaggio "forgiato" per la run.
Niente rete, niente chiave API: `tools/melting-gen` (llama.cpp su Vulkan, modello
Qwen2.5-Coder GGUF) genera un JSON vincolato da grammatica GBNF e lo valida;
`tools/melting-sprites` (stable-diffusion.cpp, SD1.5 pixel-art) disegna gli sprite del
tema. Se un modello manca o sbaglia, un generatore deterministico con seed e i fallback
geometrici prendono il suo posto: la run parte comunque, sempre.

> Il repository conserva il nome storico *melting-run-gpu*; il gioco è **Worldsmelt**
> (DEC-071). Il design canonico vive in [`docs/design/`](docs/design/README.md).

## Avvio rapido (Linux, percorso di riferimento)

```bash
scripts/setup-deps.sh          # una tantum: apt + raylib + llama.cpp + Lua + sd.cpp
make                           # compila gioco + melting-gen + melting-sprites
scripts/download-models.sh     # scarica i modelli GGUF/safetensors (riprendibile)
make run-gen                   # nuova run generata in locale (testo + sprite)
make run-gen-fast              # solo testo: salta gli ~85s di sprite
make run                       # gioca con l'ultimo manifest (o il fallback interno)
```

Il flusso: menu **WORLDSMELT** → *Nuova run* → scegli il seed → entri nel **Piano 0**
(l'hub giocabile) e ti muovi mentre la generazione lavora in sottofondo → scegli il tema
fra tre proposte e il personaggio → l'uscita verso il piano 1 si apre da sola quando è
pronto. ESC apre sempre una conferma; **R** rigenera.

## Test

```bash
make test            # suite del gioco (usa xvfb se installato: nessuna finestra visibile)
make test-gen        # generatore di testo, senza modello (veloce)
make test-script     # sandbox Lua: fughe note, determinismo, API a handle, cache
make test-sprites    # post-processing sprite, senza modello (--dry-run)
make test-llm        # generazione reale col modello (~1 min; flaky noto ~25% col 1.5B)
make docs-check      # verifica della knowledge base documentale
```

Difetti noti e loro stato: [`docs/engineering/known-issues.md`](docs/engineering/known-issues.md).

## Come funziona (pipeline locale, di serie)

```text
gioco (C + raylib, linka solo Lua statica)
  -> bin/melting-gen   (processo figlio: llama.cpp/Vulkan + GBNF + validatore
                        + dry-run del Lua nella STESSA sandbox del gioco;
                        fallback deterministico con seed su ogni errore)
       -> generated/current_run.{json,txt} + generated/scripts/*.lua
  -> bin/melting-sprites (processo figlio: stable-diffusion.cpp, SD1.5 pixel-art;
                        celle scartate -> fallback geometrico, mai un crash)
       -> generated/current_atlas.png
  -> il gioco rilegge il manifest e riparte; i piani 2-5 si generano MENTRE giochi
```

I due generatori non sono mai linkati nel binario del gioco e non stanno mai insieme in
VRAM (6 GB di riferimento): si alternano e ognuno libera tutto quando esce. Dettagli:
[`docs/engineering/architecture.md`](docs/engineering/architecture.md) e gli ADR in
`docs/engineering/adr/`.

## Script sandboxati (il cuore della faccenda)

L'IA scrive **vero codice Lua** per il comportamento degli oggetti («ogni terzo colpo si
sdoppia e i frammenti inseguono il nemico più vicino, ma solo se hai meno di tre cuori»),
eseguito in una sandbox blindata: allowlist di `_ENV`, tetto di memoria, budget di
istruzioni, mai bytecode. Ogni script fa un giro di prova dentro melting-gen prima che il
gioco lo veda; se fallisce, l'oggetto ripiega sulla mini-VM dichiarativa storica (quattro
operazioni sicure). Spec: [`docs/engineering/specs/2026-07-13-lua-sandbox-design.md`](docs/engineering/specs/2026-07-13-lua-sandbox-design.md).

## Controlli

- `WASD` movimento; `mouse + click` o frecce per sparare.
- `SPACE` bomba, `TAB` pannello build/personaggi (nel Piano 0), `R` nuova run, `ESC` conferma di uscita.

## Stato del gioco (fase 1 completa)

9 stati canonici (menu, run setup, Piano 0, gameplay, pausa, opzioni, build, risultati,
conferma d'uscita); 5 piani con stanze di taglia variabile; scelta del tema e rosa di
personaggi (3 curati + 1 generato per run con trait Lua e colpo firmato); tipi di colpo,
nemici, boss e stanze inventati dal modello dentro bande di bilanciamento garantite dal
motore (`ShotTypeBalance`, `EnemyTypeBalance`, budget di difficoltà); sinergie implicite
visibili; resa 2.5D; catalogo persistente delle creazioni incontrate; punteggi e daily
in arrivo. La roadmap vive in [`docs/plans/`](docs/plans/) e nel
[decision log](docs/design/governance/decision-log.md) (108 decisioni).

## Percorso storico Windows / OpenAI

Gli script `.bat` (MinGW) e il sidecar Node per OpenAI (`llm/`) restano nel repo come
percorso storico funzionante ma **non sono più il riferimento**: la documentazione vive in
[`docs/archive/superseded/openai-setup.md`](docs/archive/superseded/openai-setup.md).
La chiave API, se usata, resta in `.env.local` (mai nel C, mai in git).

## Documentazione

Tutta sotto [`docs/`](docs/README.md), per domini: `design/` (che cosa deve essere il
gioco — canonico), `engineering/` (come è fatto davvero), `ai-production/` (modelli, LoRA,
dataset, licenze), `plans/`, `references/`, `archive/`. Indice generato:
[`docs/INDEX.md`](docs/INDEX.md); regole: [`docs/_meta/DOCUMENT-STANDARDS.md`](docs/_meta/DOCUMENT-STANDARDS.md).
Licenze di modelli e dipendenze: [`docs/ai-production/licenze.md`](docs/ai-production/licenze.md).

## Limiti intenzionali

- Nessuna inferenza durante il combattimento: l'IA lavora nel Piano 0 e fra un piano e
  l'altro.
- Niente audio per ora (DEC-036: arriverà con mezzi curati; l'audio generativo è una
  proposta aperta).
- Il motore non dipende mai da rete, chiavi o modelli: modalità solo-curato sempre
  disponibile e dignitosa.
- Niente dipendenze npm nel percorso di riferimento.
