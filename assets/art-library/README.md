# Worldsmelt Art Library

Libreria di reference `.aseprite` per la definizione dello stile e la produzione
del nuovo pacchetto grafico (piano del 31/07/2026: rifare tutta la grafica della
demo). Struttura e protocollo dalla ricerca del proprietario
(`Worldsmelt_ricerca_riferimenti_Aseprite.md` + manifest JSON, upload del 31/07).

## Regole

1. Ogni pack in `10_references/_downloads/<id>/` ha `source_url.txt` e
   `license.txt` (snapshot alla data di download — **ricontrollare upstream
   prima di ogni uso nuovo**).
2. Le reference si usano SOLO per principi astratti (body plan, cluster,
   shading, timing): mai copiare design, personaggi o pixel riconoscibili.
3. Se una licenza cita divieti su AI training/input → il pack va in
   `study-only/` e non viene mai aperto dagli agenti né usato per LoRA.
4. Le card in `20_reference_cards/` sono l'output della Fase A; i voti finali
   (1-5 per proprietà) li dà il proprietario. Soglia: 4-5 → approved,
   3 → study-only, 1-2 → rejected.
5. Il gold set (40_worldsmelt_gold_set/) resta SOLO materiale originale
   Worldsmelt approvato dal proprietario (DEC-201).

## Stato pack (31/07/2026)

| Pack | Licenza dichiarata | Sorgenti .aseprite | Stato |
|---|---|---|---|
| debts_in_the_depths | CC0 | sì (creature, VFX, ambienti) | scaricato, Fase A |
| good_and_evil | CC0 | sì (personaggi, mostri, world, UI) | scaricato, Fase A |
| isometric_character_supernova | CC0 | sì (4 animazioni) | scaricato, Fase A |
| isometric_character_template_intellikat | CC0 | sì (~58 file .ase) | scaricato, Fase A |
| pixel_patterns | CC0 | sì (sheet 16/32) | scaricato, Fase A |
| industrial_punk | CC0 | sì (tileset 480 tile) | scaricato, Fase A |
| medieval_fantasy_items | CC0 | sì (sheet 16px) | scaricato, Fase A |
| weaponry_tools_kaenine | CC0 | sì (8 oggetti, RAR) | scaricato, Fase A |
| skull_enemy | CC0 | **no** (solo PNG nel tier gratuito; sorgenti a pagamento) | study-only |
| topdown_dungeon_character | CC0 | n/d | **download manuale richiesto** (layout pagina diverso dal flusso automatico) |
| pixel_monster_pack_1 (rvros) | CC0 | a pagamento | da acquistare se serve (molto alto per animazione) |
| architecture_shop (Readient) | vieta uso AI | — | VIETATO agli agenti (study-only umano, mai scaricato qui) |
