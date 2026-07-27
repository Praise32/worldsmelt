---
id: plans-mechanics-lab
title: Mechanics-lab — scoprire le primitive minime dei colpi componibili
domain: plans
status: approved
authority: supporting
owner: engineering
summary: >-
  Esperimento isolato (DEC-138) per scoprire le primitive fisiche minime che permettono
  al modello di COMPORRE classi di colpo nuove (laser, catena, orbita e oltre) senza
  primitive dedicate; le vincitrici entrano poi nel motore con le garanzie di sempre.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [mechanics-lab, colpi, primitive, lua, esperimento, DEC-138, DEC-165]
related: [eng-espressivita-colpi]
supersedes: []
source_files: [src/script/script_sandbox.c, src/script/script_api.c]
---

# Mechanics-lab (DEC-138)

Obiettivo: capire **quali primitive minime** rendono il modello capace di inventare
classi fisiche di colpo che non abbiamo previsto — senza trasformare l'API in un motore
duplicato e senza perdere le garanzie (clamp, budget, determinismo, fallback).
Base fattuale: `docs/engineering/espressivita-colpi.md`. Origine dell'idea: research pack
21/07 (`docs/references/research/worldsmelt-research-pack-2026-07-21/10-decisioni-e-domande-aperte.md`,
domande 4-5 e «prossimo esperimento consigliato»).

## La build

`tools/mechanics-lab/` (eseguibile separato, mai linkato nel gioco):

1. **La stessa sandbox del gioco** (`src/script/script_sandbox.c`, come già fa
   melting-gen per il dry-run): stessi limiti di memoria/istruzioni/determinismo.
2. **Arena 2D minima**: una stanza, un player fantoccio, N bersagli statici/mobili;
   simulazione a passo fisso, rendering minimo (o headless con dump di traiettorie).
3. **Primitive candidate** esposte al Lua, ognuna con clamp e costo dichiarato:
   - query a segmento (`segment_query(x1,y1,x2,y2)` → primo bersaglio sulla retta);
   - controllo del colpo posseduto: `set_shot_velocity(id,vx,vy)` / `anchor_shot(id,x,y)`
     con budget di manipolazioni per frame;
   - vita parametrica (`spawn_shot(..., life)` clampata);
   - forze/steering (`steer_shot(id, tx, ty, forza)`);
   - emissione dal colpo (`shot_emit(id, ...)`: un colpo che spara);
   - timer/fase per script (`elapsed()`, già deterministico).
4. **Command queue e beam rendering** di servizio per visualizzare cosa succede.

## Il protocollo

- **20 prompt «impossibili»** per l'API attuale (laser continuo, torretta, mina a
  prossimità, orbita, boomerang, muro di colpi, catena elastica, raggio riflesso, …),
  congelati con seed fissi.
- Per ogni combinazione di primitive abilitate: il modello (7B baseline; poi i candidati
  della comparison) scrive lo script; si misura: compila? fa la cosa chiesta (giudizio
  visivo/dump)? resta nei budget? è bilanciabile (danno totale/secondo confrontabile)?
- **Criterio di successo (DEC-138):** laser, catena e orbita generati SENZA primitive
  dedicate a «laser», «catena» o «orbita» — solo componendo le primitive minime.
- Report: `logs/mechanics-lab/<timestamp>/report.md` — per ogni prompt: primitive usate,
  esito, script campione; classifica delle primitive per «potere espressivo per unità di
  rischio».

## Dopo l'esperimento

Le primitive vincenti entrano nel motore vero con: clamp e budget in `script_api.c`,
bilanciamento esteso (`ShotTypeBalance` deve pesare anche le nuove classi), grammatica e
cheat-sheet aggiornati, test di garanzia per ogni via di abuso (colpo eterno, danno
infinito da beam, orbita-scudo permanente), e i vincoli fermi di DEC-138: floor
anti-«colpo che striscia» per i colpi in volo, mini-VM come rete, nessuna inferenza in
combattimento.

## Quando

**Gate soddisfatto (DEC-165, 2026-07-27): il piano è sbloccato e può partire.** Le due
condizioni poste da DEC-138 si sono chiuse entrambe — il refactor GUI (DEC-137) è concluso
e la comparison dei modelli è chiusa il 23/07/2026, con i report congelati in
`docs/ai-production/experiments/` (i candidati migliori, a partire dal modello di testo
attivo, diventano i modelli di prova del lab). Restano invariati il criterio di successo e
i vincoli fissati da DEC-138: generare laser, catena e orbita **senza** primitive dedicate
a laser, catena o orbita; niente inferenza in combattimento; primitive vincenti ammesse nel
motore solo con clamp, budget, `ShotTypeBalance` e fallback. Scala: gradino 3 (implementa
opus, giudica Fable).
