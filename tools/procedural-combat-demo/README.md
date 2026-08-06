# Worldsmelt Procedural Combat Lab

Demo autonoma Raylib 6.0 che mostra attacchi e animazioni di gameplay generati
da Lua senza spritesheet. Usa la **stessa `ScriptSandbox` del gioco**, non una
reimplementazione semplificata, e asset statici CC0 esterni a Worldsmelt.

La demo non modifica il runtime del gioco. Serve a verificare visivamente e
funzionalmente il percorso di integrazione proposto per nemici, armi e sinergie
non convenzionali.

## Risultato visibile

Non più sei scene dimostrative a ciclo: una **arena continua** di debug del
combattimento (spec `docs/engineering/specs/2026-08-05-combat-lab-design.md`).
Un player che si muove e mira col mouse, un nemico alla volta con HP visibile e
respawn col pattern successivo, due sandbox Lua vive insieme — quella del
nemico e quella dell'arma equipaggiata — con quote e kill switch indipendenti.

I cinque script della prova originale restano come **pool curato** in
`scripts/curated/`, copiato accanto al binario dal Makefile. Gli script
generati si aggiungono a caldo (polling ~1 s) da `generated/combat-lab/enemy/`
e `generated/combat-lab/weapon/`, percorsi **relativi alla directory da cui si
lancia la demo**: da qui l'avvio dalla radice del repository.

La cattura headless `--capture <dir>` esporta 450 frame PNG col player in
autopilota, i soli script curati e il pattern nemico che avanza ogni 5 secondi:
è lo smoke test della demo, non un trailer.

La camera mostra sempre l'intera arena. Il player è alto 34 px su una viewport
720p (circa 4,7% dell'altezza), quindi lo zoom è intenzionalmente più ampio di
quello del prototipo attuale.

## Avvio

Su Linux, dalla radice del repository:

```bash
make combat-lab
make run-combat-lab
```

Gli script `build.ps1`, `capture.ps1` e `run-demo.bat` restano per la build
MinGW storica su Windows (prova originale a sei scene).

Controlli:

- `WASD` o frecce: movimento; mouse: mira; click sinistro tenuto: fuoco;
- `N` / `M`: pattern nemico / arma successivi nel pool (slot 0 dell'arma =
  pistola base, sempre disponibile anche se la sandbox dell'arma muore);
- `G` / `H` / `B`: generazione lotto nemici / armi e ricarica del brief (in
  arrivo con l'integrazione del generatore: per ora solo un avviso in HUD);
- `1`, `2`, `3`: pixel, smooth, ibrido (default smooth);
- `Tab`: confronto affiancato pixel vs ibrido (si mira nel pannello sotto il
  cursore);
- `Spazio`: pausa;
- `R`: reset dell'arena senza cambiare gli script correnti.

Per rigenerare i frame dello smoke test:

```bash
./build/combat-lab/combat-lab --capture build/combat-lab/frames
```

## Come Gemma compone l'attacco

Gemma non genera C, GLSL o accessi Raylib diretti. Genera una piccola funzione
Lua `on_tick(dt, self_handle)` usando un alfabeto di verbi semantici:

```lua
locked_aim = aim_snapshot()
telegraph_arc(self_x(), self_y(), locked_aim,
              112, 24, 2.35, 0.34, VIS_GRAVITY)

melee_sweep(self_handle, self_x(), self_y(), locked_aim,
            112, 24, 2.35, 12, 0.18, VIS_GRAVITY)
capture_radius(self_handle, self_x(), self_y(),
               132, 2.2, 8, 0.46, VIS_GRAVITY)
release_echoes(self_handle, self_x(), self_y(), locked_aim,
               5, 360, 5, 1.15, 1.6, VIS_VOID_ECHO)
```

Il C valida ogni comando, applica collisioni e danno e lo invia al renderer.
Lo stesso comando viene poi disegnato in modalità pixel, smooth o ibrida.
Cambiare renderer non cambia la simulazione.

Alfabeto esposto dalla demo:

| Letture | Geometria/attacchi | Stato e sinergie |
| --- | --- | --- |
| `player_x/y` | `telegraph_arc` | `set_velocity` |
| `self_x/y` | `emit_arc` | `add_status` |
| `aim_at_player` | `emit_ring` | `melee_sweep` |
| `aim_snapshot` | `emit_orbit` | `capture_radius` |
|  | `telegraph_beam` | `release_echoes` |
|  | `emit_beam` |  |

Non sono template per classi di arma: sono operatori geometrici riutilizzabili.
Una falce, un ragno, un'alabarda e una ricarica organica possono combinarli in
sequenze differenti senza aggiungere una nuova classe C per ogni invenzione.

## Confine di sicurezza verificato

La demo usa direttamente:

- `ScriptSandboxCreate` con tetto di memoria;
- `_ENV` ad allowlist e RNG deterministica;
- caricamento esclusivamente testuale;
- budget di istruzioni per caricamento e callback;
- `ScriptSandboxCallVoid` per `on_tick`;
- kill switch per sandbox, non globale.

`demo_script_api.c` registra closure C come fa già il dry-run di
`melting-gen`. Lua non riceve puntatori, texture, shader, percorsi o accesso al
filesystem. Per fixed tick, ogni sandbox può emettere al massimo 96 comandi
totali, di cui 48 gameplay e 64 visuali. Handle, ID, posizioni, velocità,
conteggi, raggi, danno e durate sono validati o clampati dal C.

Verifiche eseguite:

- 5/5 script caricati nella sandbox reale;
- 100 tick di smoke per script senza sandbox disabilitate;
- build MinGW con `-Wall -Wextra` senza warning;
- 450/450 frame esportati;
- GIF, WebP e contact sheet creati;
- collisioni, catture, HP e hit sono visibili nell'HUD della cattura.

## Cosa è fisso e cosa deve adattarsi

Può essere fisso per quasi qualunque sprite statico, se il contenuto fornisce
un pivot e una hitbox:

- bob/camminata del corpo intero, squash, recoil, knockback;
- dash con afterimage, hurt flash, invulnerabilità, morte/dissolvenza;
- rotazione di arma rigida verso la mira;
- telegraph di area, beam e arco;
- aura di status, ombra, trail e particelle cosmetiche.

Deve essere parametrico o generato in Lua quando cambia la decisione di gioco:

- traiettoria e forma dei proiettili;
- ordine e finestre di una ricarica inventata;
- thrust, sweep, giro completo e combo di una nuova arma melee;
- assorbimento, rilascio, split, orbita, rimbalzo e status;
- fasi e pattern di un nemico/boss.

Non servono frame SD aggiuntivi: il renderer muove lo sprite statico come corpo
rigido e costruisce intorno ad esso arma, hitbox, scie e VFX. Servono però
metadati minimi per contenuto (`pivot`, `tip`, `muzzle`, raggio hitbox), perché
Raylib non può dedurre in modo affidabile dove sia l'impugnatura da un PNG
arbitrario.

## Pixel, smooth o shader

La scelta raccomandata è **ibrida**:

- sprite, nucleo del proiettile, telegraph e bordo della hitbox restano
  pixel/nearest e ad alto contrasto;
- glow, scie, shockwave e particelle secondarie possono essere smooth;
- HUD e informazioni critiche vengono disegnati sopra gli effetti;
- un preset Pixel/Reduced FX resta fallback per hardware o backend senza
  shader.

Il semplice filtro bilineare su uno sprite pixel lo sfoca: non crea dettaglio.
Gli shader sono più utili su geometria procedurale, maschere emissive e
compositing. La demo usa un singolo shader didattico; una pipeline production
userebbe emissive mask e blur separabile a risoluzione ridotta.

## Limite attuale di Worldsmelt

Fatto verificato: oggi il runtime del gioco non espone ancora
`on_enemy_update`, archi/beam con collisione né un'API visuale Lua. I nemici
attuali sono combinazioni di enum C per movimento e fuoco. Questa cartella
dimostra il minimo layer corretto da integrare; non dichiara che tali attacchi
siano già disponibili nel gioco principale.

Per l'integrazione reale servono inoltre validatori di dominio pre-run:

- densità massima e tempo minimo di telegraph;
- almeno un corridoio/spazio sicuro nei pattern obbligatori;
- budget di danno, velocità, vita e numero di entità;
- transcript deterministico dei comandi a stesso seed;
- RNG gameplay separata dalla RNG cosmetica;
- fallback per la singola entità quando uno script fallisce.

## Riferimenti di design e rendering

- [Tiny Rogues, pagina ufficiale Steam](https://store.steampowered.com/app/2088570/Tiny_Rogues/)
- [The Binding of Isaac: gameplay explained, Edmund McMillen](https://edmundmcmillen.blogspot.com/2011/09/binding-of-isaac-gameplay-explained.html)
- [Enter the Gungeon, pagina ufficiale Steam](https://store.steampowered.com/app/311690/Enter_the_Gungeon/)
- [Enter the Gungeon: intervista di design](https://www.gamedeveloper.com/design/q-a-the-guns-and-dungeons-of-i-enter-the-gungeon-i-)
- [Sinergie di Enter the Gungeon](https://enterthegungeon.wiki.gg/wiki/Synergies)
- [raylib 6.0: post-processing](https://github.com/raysan5/raylib/blob/6.0/examples/shaders/shaders_postprocessing.c)
- [raylib 6.0: particle blending](https://github.com/raysan5/raylib/blob/6.0/examples/textures/textures_particles_blending.c)
- [raylib 6.0: texture waves](https://github.com/raysan5/raylib/blob/6.0/examples/shaders/shaders_texture_waves.c)

La provenienza degli sprite è registrata in `assets/ASSET_PROVENANCE.md`.
