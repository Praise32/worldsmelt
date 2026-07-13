# Repository e riferimenti locali

## Dentro la repo (`deps/`, ignorata da git, creata da scripts/setup-deps.sh)

- `deps/raylib` — raylib 6.0 a tag fissato, build statica linkata dal gioco.
- `deps/llama.cpp` — llama.cpp b9979 con backend Vulkan, linkata SOLO da
  `tools/melting-gen`. La build include `build/bin/test-gbnf-validator`,
  usato da `make test-gen` per la grammatica.

## Vendorate (committate nella repo)

- `tools/melting-gen/vendor/cJSON.{c,h}` — cJSON v1.7.19 (MIT).

## Riferimenti esterni (da consultare, non dipendenze)

- Raygui — https://github.com/raysan5/raygui — candidata per la fase 4 (UI).
  Se integrata: `RAYGUI_IMPLEMENTATION` in un solo `.c` in un futuro `src/ui`.
- IsaacDocs — https://wofsauge.github.io/IsaacDocs/rep/ — riferimento
  concettuale per callback e pattern della futura sandbox Lua (fase 3).
  Non è codice da copiare né una specifica del nostro motore.
- Generazione dungeon di Isaac — articolo di BorisTheBrave:
  https://www.boristhebrave.com/2020/09/12/dungeon-generation-in-binding-of-isaac/
  Algoritmi da reinterpretare per il modello dati di `src/world`.
- gguf-tools — https://github.com/antirez/gguf-tools — SOLO ispezione e
  manipolazione di file GGUF (niente inferenza); utile come codice leggibile
  per capire il formato. L'inferenza è llama.cpp.
