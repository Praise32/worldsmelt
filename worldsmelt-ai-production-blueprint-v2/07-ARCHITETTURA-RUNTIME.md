# Architettura runtime della generazione

## Obiettivo

Permettere a più computer possibili di giocare, mantenendo la generazione locale come
funzione scalabile e non come requisito assoluto.

## Tier

### Tier 0 — Solo curato

- nessun modello;
- asset inclusi;
- RunManifest curato/procedurale;
- partenza immediata;
- esperienza completa e supportata.

### Tier 1 — Testo generativo, grafica curata

- Qwen genera contenuto e comportamento;
- sprite curati o composti;
- adatto a GPU con poca memoria o CPU.

### Tier 2 — Grafica leggera

- Qwen crea le specifiche;
- SD genera una reference o pochi componenti per archetipo;
- rig raylib anima;
- LCM e 256 px opzionali;
- cache aggressiva.

### Tier 3 — Grafica completa

- più candidati;
- 512 px;
- più LoRA;
- frame chiave controllati;
- ControlNet quando necessario;
- variazioni per piano.

## Scheduling

```text
1. Qwen carica
2. genera e valida RunManifest/EnemySpec
3. Qwen si scarica
4. SD carica
5. genera asset minimi
6. SD si scarica
7. motore valida e pubblica SpriteBundle
8. gioco entra nel piano
```

Qwen e SD non devono coesistere in VRAM sulla macchina da 6 GB.

## Priorità di generazione

1. anteprime delle carte tema;
2. asset indispensabili al Piano 1;
3. boss Piano 1;
4. oggetti/proiettili indispensabili;
5. decorazioni;
6. piani successivi;
7. varianti cosmetiche.

## Cache

Chiave:

```text
pipeline_version
model_sha256
lora_sha256[]
prompt_hash
negative_prompt_hash
seed
generation_size
steps
cfg
postprocess_version
asset_role
rig_version
```

Un asset valido già in cache non viene rigenerato.

## Pubblicazione atomica

Ogni bundle viene scritto in una cartella temporanea:

```text
generated/tmp/<bundle-id>/
```

Dopo validazione completa:

```text
rename -> generated/bundles/<bundle-id>/
```

Il manifest della run viene aggiornato soltanto dopo la pubblicazione.

## Failure policy

- modello assente: Tier inferiore;
- caricamento fallito: fallback curato;
- cella vuota: fallback geometrico;
- asset tocca bordo: rigenerazione o fallback;
- alpha errato: postprocess;
- silhouette non valida: altro seed;
- timeout: interrompere, non bloccare;
- run incompleta: usare bundle precompilato.

## Nessuna inferenza in combattimento

Durante il combattimento sono ammessi:

- caricamento texture;
- animazione;
- compositing;
- palette swap;
- particelle;
- shader;
- rig procedurali;
- lettura cache.

Non sono ammessi:

- Stable Diffusion;
- Qwen;
- download;
- compilazione di nuovi script;
- validazione pesante.

## Benchmark al primo avvio

Misurare separatamente:

- Qwen token/s;
- SD secondi per immagine;
- VRAM;
- tempo load/unload;
- memoria sistema;
- spazio disco.

Il benchmark propone un tier, ma il giocatore può cambiarlo.
