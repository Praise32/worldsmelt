---
id: eng-espressivita-colpi
title: Espressività dei tipi di colpo — cosa può inventare davvero il modello oggi
domain: engineering
status: approved
authority: canonical
owner: engineering
summary: >-
  Verifica puntuale (file:riga) di cosa il vocabolario attuale dei colpi permette al
  modello: variazioni sullo stesso scheletro fisico sì (catena inclusa, già nativa),
  laser/stazionari/orbite no. Base fattuale di DEC-138 (mechanics-lab).
last_reviewed: 2026-07-22
last_verified_commit: bd2cf04
topics: [colpi, shot-type, lua, espressivita, mechanics-lab, DEC-138]
related: [eng-spec-step-c-shottype]
supersedes: []
source_files: [src/core/shot_type.h, src/gameplay/combat.c, src/script/script_api.c, tools/melting-gen/run.gbnf]
---

# Espressività dei tipi di colpo (verificata sul codice, 22/07/2026)

Domanda: *la struttura attuale supporta veri colpi nuovi? Il gioco può creare laser,
colpi immobili che fluttuano, o qualsiasi cosa?* Risposta breve: **no** — oggi il modello
inventa **variazioni comportamentali sullo stesso scheletro fisico** (proiettile
puntiforme, moto rettilineo, vita breve, danno a impatto), non nuove classi di fisica.
La direzione per superarlo è DEC-138 (mechanics-lab, primitive componibili).

## Il vocabolario attuale

- **`ShotForm`** (`src/core/shot_type.h:31-38`): 5 forme — orb, spike, beam, arc, blade —
  **puramente visive** (`src/render/game_renderer.c:719-799`). «Beam» è una scia dietro un
  proiettile normale, non un raggio.
- **7 manopole** (`shot_type.h:46-57`) con clamp (`:62-72`): speedMul 0.5-2.0,
  damageMul 0.3-2.0, radiusMul 0.4-2.5, lifeMul 0.5-2.0, pierce 0-3, chain 0-3,
  pellets 1-3. `speedMul` **non scende mai a 0**.
- **`ShotTypeBalance`** (`shot_type.c:161-196`): riporta la potenza in [0.75, 1.25] —
  bilancia numeri, non conosce i comportamenti.
- **Proiettile runtime `Shot`** (`src/core/game_types.h`): pos, vel, radius, damage,
  life (hard-coded 1.15s giocatore / 2.6s nemico, `src/gameplay/entities.c:131`),
  traits, bounces/pierce/chain, form. Update: moto rettilineo puro
  (`src/gameplay/combat.c:602` `pos += vel*dt`), collisione punto-cerchio (`:660-700`).
  Nessun campo per accelerazioni, orbite (centro/fase), o raggi.
- **API Lua** (`src/script/script_api.c:437-458`): letture (player/enemy/shot x-y-hp,
  `nearest_enemy`, bordi stanza) e scritture clampate — `spawn_shot` con **speed
  [60, 900]** (`:293,340`), `damage_enemy` [0,500], `heal_player` [0,12],
  `set_enemy_velocity` (solo nemici, mai colpi), `add_particle`. **Non esiste** alcuna
  funzione per muovere/ancorare un colpo già sparato, né vita parametrica, né raycast.
- **Grammatica del generatore** (`tools/melting-gen/run.gbnf:26-27`): il modello può
  scrivere ESATTAMENTE le 5 forme e le 7 manopole — nessun campo in più è generabile.

## Cosa è già esprimibile (verificato)

- **Catena fra nemici**: nativa — `ShotTypeDef.chain` + `CombatChainShot`
  (`combat.c:218-253`, salto al nemico più vicino entro 220px, danno ×0.65); l'esempio
  «Jolt» del fallback la usa (`shot_type.c:144-149`).
- Sidegrade su forma+manopole (dardo perforante veloce, ventaglio lento, ecc.).
- Da Lua: proiettili extra con trait (bounce/homing/explode/split/…), «familiare» che
  spara periodicamente verso `nearest_enemy` in `on_tick`, respinte con
  `set_enemy_velocity` — gli esempi del cheat-sheet (`prompts/lua_system.txt:93-150`).

## Cosa NON è esprimibile e perché

| Classe | Barriera (verificata) |
|---|---|
| **Laser/beam continuo** | Nessuna collisione a segmento/raycast (solo punto-cerchio, `combat.c:660-700`); nessun danno-per-tick lungo una direzione; `SHOT_FORM_BEAM` è solo grafica. |
| **Stazionario (torretta/mina)** | Velocità ~0 bloccata su TRE livelli indipendenti: `SHOT_TYPE_SPEED_MIN=0.5` (`shot_type.h:62`), `SCRIPT_API_SHOT_SPEED_MIN=60` (`script_api.c:293`), floor 260 sulla base (`script_items.c:237`) — scelta deliberata anti-«colpo che striscia» (`docs/engineering/specs/2026-07-14-step-c-shottype-balance.md:44`). In più `Shot.life` è hard-coded (max ×2 via lifeMul). |
| **Orbitante** | Nessun campo fase/centro su `Shot`, nessuna primitiva Lua per riposizionare un colpo. Nota: la formula tangenziale esiste già **per i nemici** (`ENEMY_MOVE_ORBIT`, `combat.c:382-392`, campo `phase` su `Enemy`) — mai portata sui colpi. |

## Conseguenza (DEC-138)

Aggiungere queste classi richiede modifiche al C (campi su `Shot`/`ShotTypeDef`, rami
in `CombatUpdateShots`, primitive Lua, grammatica, prompt, test): non si ottengono
scrivendo solo JSON/Lua dentro i binari attuali. La via scelta NON è un menu di classi
fisse ma **primitive minime componibili** scoperte con il mechanics-lab
(`docs/plans/active/mechanics-lab.md`): il criterio di successo è generare laser, catena
e orbita **senza** primitive dedicate a «laser», «catena» o «orbita».
