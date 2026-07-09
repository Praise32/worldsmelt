# Setup OpenAI per Melting Run GPU

Il gioco non contiene la chiave API. La chiave resta nel terminale o in
`.env.local`, che e' ignorato da git.

## 1. Imposta la chiave

In PowerShell:

```powershell
$env:OPENAI_API_KEY="la_tua_chiave"
```

Oppure crea `.env.local`:

```text
OPENAI_API_KEY=la_tua_chiave
OPENAI_MODEL=gpt-5.5
OPENAI_REASONING_EFFORT=medium
OPENAI_IMAGE_MODEL=gpt-image-2
OPENAI_IMAGE_QUALITY=medium
```

`OPENAI_IMAGE_QUALITY=medium` e' il default consigliato per ottenere sprite piu'
leggibili. Puoi abbassarlo a `low` per test rapidi o alzarlo a `high`, ma ogni
generazione immagine puo' costare di piu' e richiedere piu' tempo.

## 2. Scegli una modalita'

Solo testo, economica:

```bat
generate_llm_content.bat
run_gpu.bat
```

Test completamente offline:

```bat
generate_llm_content.bat --fallback --seed=12345
run_gpu.bat
```

Test dinamico completo con testo + spritesheet:

```bat
run_gpu_dynamic.bat --seed=12345 --quality=medium
```

Questa e' la modalita' consigliata: la PNG della Image API viene salvata in
`generated/current_atlas.png` e il gioco la usa direttamente come spritesheet
giocabile, ritagliando celle fisse da 128x128.

Se vuoi forzare il vecchio atlas locale, utile quando la Image API non e'
disponibile o produce una griglia troppo irregolare, usa:

```bat
run_gpu_dynamic.bat --seed=12345 --local-atlas
```

In quel caso il gioco usa `generated/current_atlas.bmp`, generato localmente con
celle garantite.

## 3. File generati

```text
generated/current_run.json
generated/current_run.txt
generated/current_atlas.png
generated/current_atlas.bmp
generated/current_atlas.json
```

Il JSON conserva la risposta strutturata. Il TXT e' il manifest letto dal gioco
C. Il PNG e' lo spritesheet principale letto dal gioco quando usi `--image`. Il
BMP resta come fallback locale o come scelta esplicita con `--local-atlas`.
`current_atlas.json` conserva la mappa delle celle.

## 4. Cosa puo' generare OpenAI

La Responses API genera:

- temi dei 5 piani;
- palette;
- nomi boss;
- oggetti;
- slot visivi degli oggetti;
- tratti meccanici;
- piccoli script sandboxati.

La Image API genera uno spritesheet tecnico 1024x1024. Il prompt chiede una
griglia invisibile 8x8, celle da 128x128, soggetti centrati, padding costante e
sfondo quasi nero. Il gioco applica un chroma-key sul nero, poi ritaglia la
mappa nota: player, nemici, boss, pickup, portale e colpo.

## 5. Cosa non puo' fare OpenAI

- Non puo' eseguire codice dentro il gioco.
- Non puo' accedere a file o rete dal runtime C.
- Non puo' modificare collisioni, hitbox o memoria libera.
- Non puo' inserire nuove funzioni C durante una run.

Le sinergie vengono espresse con istruzioni dichiarative, per esempio:

```text
on_fire:burst,3,0.36,split
on_hit:area,54,0.22,slow
on_hit:heal,18,1,vamp
```

Il C valida e interpreta solo queste operazioni.

## 6. Fonti ufficiali utili

- Responses API: https://api.openai.com/v1/responses
- Image API: https://api.openai.com/v1/images/generations
- Guida immagini: https://developers.openai.com/api/docs/guides/image-generation
- Modello testuale default: https://developers.openai.com/api/docs/guides/latest-model.md
