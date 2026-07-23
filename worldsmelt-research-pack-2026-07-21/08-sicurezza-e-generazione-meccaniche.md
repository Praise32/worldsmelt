# Sicurezza e kernel di generazione delle meccaniche

## Principio

Il modello non è l’autorità. Propone codice. Il motore:

- compila;
- valida;
- simula;
- limita;
- accetta o rifiuta;
- esegue attraverso un’interfaccia controllata.

## Architettura consigliata

```text
LLM
  ↓ Lua + metadati
parser/loader solo testo
  ↓
sandbox Lua
  ↓ command queue
validator C
  ↓
world/gameplay C
```

## Command queue

Invece di permettere modifiche arbitrarie immediate, molte azioni dovrebbero diventare comandi:

```text
SPAWN_PROJECTILE
APPLY_DAMAGE
APPLY_FORCE
APPLY_STATUS
DRAW_BEAM
CREATE_TIMER
DESTROY_ENTITY
```

Ogni callback produce al massimo un numero fisso di comandi. Il C li valida e li applica dopo il ritorno dello script.

Vantaggi:

- niente mutazioni a metà iterazione;
- limiti globali semplici;
- replay e debug;
- telemetria per bilanciamento;
- possibilità di rifiutare comandi individuali;
- migliore determinismo.

## Primitive da aggiungere

### Query

- `query_circle`;
- `query_segment`;
- `raycast`;
- `nearest_entity` con filtri;
- `entities_with_tag`;
- query di proiettili ostili/alleati.

### Azioni

- `spawn_projectile` parametrico;
- `spawn_entity` da archetipi minimi sicuri;
- `apply_force`;
- `apply_status`;
- `schedule_timer`;
- `cancel_timer`;
- `destroy_entity`;
- `set_target`;
- `emit_event` con allowlist.

### Visualizzazione

- `draw_line`/beam temporaneo;
- `draw_arc`;
- `draw_ring`;
- `add_particle` con palette controllata;
- telegraph separato dall’hitbox.

### Stato

- key/value numerico o tabella limitata per script;
- timer nominati;
- contatori di eventi;
- handle generazionali, mai puntatori diretti.

## Budget

Esempio iniziale da calibrare:

- memoria per sandbox/script;
- istruzioni per callback;
- profondità massima;
- massimo comandi per callback;
- massimo spawn per frame e per stanza;
- massimo query geometriche;
- massimo timer;
- massimo particelle/VFX;
- massimo tempo wall-clock di validazione.

I valori vanno misurati, non scelti soltanto a intuito.

## Pipeline di accettazione

1. output delimitato correttamente;
2. scansione simboli vietati;
3. caricamento solo testo;
4. compilazione Lua;
5. verifica callback e firma;
6. dry-run con stub API;
7. simulazione accelerata in scenari sintetici;
8. limiti di danno, spawn ed entità;
9. test di non-vacuità;
10. hash e serializzazione;
11. pubblicazione atomica nel RunBundle.

## Simulazione comportamentale

Il dry-run singolo non basta. Servono piccole arene deterministiche:

- nessun nemico;
- un nemico vicino;
- più nemici allineati;
- nemici ai bordi;
- giocatore con poca vita;
- molti proiettili;
- callback ripetuta per 10–60 secondi simulati.

Metriche:

- DPS;
- numero spawn;
- durata script;
- comandi rifiutati;
- crescita dello stato;
- entità vive;
- copertura delle callback;
- comportamento dopo handle invalidi.

## Sinergie

Le sinergie non dovrebbero dipendere da uno script che conosce internamente ogni altro oggetto. Meglio eventi e tag semantici:

```text
on_projectile_spawned
on_damage_dealt
on_heal
on_enemy_killed
on_bounce
on_status_applied
```

Ogni oggetto ascolta o emette eventi entro un budget. Il motore ordina gli eventi e impedisce ricorsioni infinite.

Questo permette emergenza reale:

- oggetto A converte cure in evento energia;
- oggetto B reagisce a energia creando un’orbita;
- oggetto C modifica i proiettili creati da un’orbita;

senza una funzione C scritta appositamente per la tripla.

## Determinismo e condivisione

- RNG soltanto del gioco;
- niente `math.random`;
- ordine eventi definito;
- handle generazionali;
- serializzazione canonica;
- seed del runtime Lua;
- niente indirizzi convertiti in stringa;
- RunBundle finale condiviso per evitare divergenze tra backend.

## Fuzzing e CI

- ASan/UBSan sulle build di test;
- corpus di script validi e malevoli;
- loop, ricorsione, bombe memoria;
- metatable e bytecode;
- NaN, infinito e overflow;
- handle stale;
- pattern di eventi ricorsivi;
- confronto hash dopo replay;
- test su CPU e Vulkan.
