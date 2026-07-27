---
id: gd-content-audio-feedback
title: Audio and Feedback
domain: design
status: draft
authority: canonical
owner: design
summary: "Feedback per azioni, rischi e sinergie; elenco eventi prioritari con fusione, scelta del tema e generazione completata nel Piano 0. L'audio è uno dei quattro assi dell'escalation leggibile del tema per piano (DEC-024). Dal 22/07 la via primaria è generativa: Stable Audio Small in locale, con il pacchetto curato/statico come fallback garantito (DEC-109, catena semplificata a due livelli da DEC-178 — rFXGen rimosso); ogni evento critico mantiene comunque un suono curato o di fallback. La demo attuale usa un pacchetto pre-generato offline, non la generazione a runtime (DEC-172)."
last_reviewed: 2026-07-28
topics: [audio, feedback, eventi prioritari, DEC-024, DEC-036, DEC-109, stable-audio, DEC-172, DEC-178, demo]
related: []
supersedes: []
source_files: []
---

# Audio and Feedback

## Ogni evento importante definisce

- segnale anticipatorio;
- conferma dell'azione;
- impatto;
- stato persistente, se necessario;
- priorità rispetto ad altri suoni.

## Eventi prioritari

- danno ricevuto;
- attacco nemico pericoloso;
- stanza completata;
- risorsa insufficiente;
- oggetto acquisito;
- sinergia attivata;
- boss in nuova fase;
- generazione o fallback che richiede intervento del giocatore;
- **fusione**: evento della meccanica-firma, quando il giocatore completa una fusione
  esplicita nella stanza di fusione (vedi `../systems/item-fusion.md`) — ha **priorità
  massima dedicata**: il segnale più riconoscibile del gioco, interrompe/attenua gli altri
  (DEC-118);
- **scelta del tema nel Piano 0**: quando il giocatore sceglie uno dei 2-3 temi proposti
  dall'IA (vedi `../systems/floor-zero.md`);
- **generazione completata**: quando l'indicatore di generazione del Piano 0 segnala che il
  piano successivo è pronto e l'uscita si apre (vedi `../systems/floor-zero.md`).

## Regola

Il feedback generato deve appartenere a una libreria o grammatica coerente, non essere rumore
arbitrario.

## Escalation del tema tra piani (DEC-024)

L'audio è uno dei quattro assi su cui il tema di una run si intensifica piano dopo piano
(vedi [Difficulty and Progression](../07-difficulty-and-progression.md) per il principio
generale, non riformulato qui). Anche nei piani più avanzati, l'intensificazione della
libreria/grammatica sonora deve restare **ascoltabile**: l'audio non deve mai degradare in
rumore indistinguibile, indipendentemente da quanto il tema si sia intensificato.

## Audio generativo con catena di fallback (DEC-109, sostituisce la parte «futuro» di DEC-036; catena semplificata da DEC-178)

Dal 22/07 la via primaria per musica e SFX è **generativa**: **Stable Audio Small in
locale** — checkpoint `music` per musica/ambience, checkpoint `sfx` per gli effetti, sia
semplici sia complessi — con **audio curato/statico** come fallback garantito. Dal 28/07
(DEC-178) la catena è a **due soli livelli** (checkpoint sfx → curato): **rFXGen è uscito
dalla pipeline**, non è mai stato installato né usato. La garanzia storica di DEC-036
sopravvive come rete: ogni evento critico ha sempre un suono curato o di fallback, e la
modalità solo-curato resta completa e dignitosa. Vincoli architetturali: nessuna
generazione durante il combattimento; il modello audio si carica in sequenza con il
modello di testo attivo e SD (mai insieme nei 6 GB di riferimento); cache e pubblicazione
atomica (pipeline tecnica in `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`; licenza:
DEC-113).

## Audio della demo: pacchetto pre-generato offline (DEC-172)

Nella demo attuale l'audio è un **pacchetto pre-generato offline**: musica/ambience e SFX
generati con **Stable Audio 3 Small** (checkpoint music e sfx già in `models/` — nessun
effetto procedurale rFXGen, uscito dalla pipeline con DEC-178), tutto integrato nel gioco
come **asset statici**. Il motore acquisisce un **modulo audio** (raylib) che **legge solo
file locali già pronti** — coerente
con la regola di `AGENTS.md` sull'indipendenza del motore dai modelli AI. **Nessuna
generazione audio gira a runtime nella demo.** DEC-109 (pipeline generativa a runtime)
**resta la destinazione finale**: questa è l'**istanza demo** del fallback curato sempre
garantito già previsto da DEC-036/DEC-109, non una nuova pipeline. Dettaglio tecnico e nota
gemella: [Pipeline audio](../../ai-production/16-AUDIO-GENERATION-PIPELINE.md).

## Stato di implementazione — modulo audio del motore (2026-07-28)

Il **modulo audio** promesso da DEC-172 è implementato in `src/audio/audio.{h,c}` (prefisso
`Audio*`, coppia .h/.c come da `AGENTS.md`): legge **solo** il pacchetto statico di
`assets/audio/` tramite una tabella di percorsi fissa in C (mai `manifest.json`, che resta
per gli umani — regola `AGENTS.md`, niente parsing JSON nel motore). `InitAudioDevice` gira
subito dopo `InitWindow` (`src/app/app.c`, `AppRun`); se il device manca, o un singolo file
non carica, il modulo resta **silenziosamente spento** — nessuna funzione va mai in crash,
verificato con `--audio-test`/`make test` sotto Xvfb (nessun backend audio reale in
quell'ambiente: è lo scenario headless vero, non solo simulato).

Musica in streaming per stato (crossfade breve, **default proposto** stile DEC-019, nessun
documento fissa la durata): `MainMenu`/`RunSetup` condividono un tema, `FloorZero` il suo,
`Gameplay` due (piano 1-2 / 3-5, asse "audio" dell'escalation DEC-024 — **soglia di piano
proposta dall'implementazione**, da confermare col playtest), la stanza boss uno dedicato
(vince sempre, a qualunque piano), `RunResults` il suo. `PauseMenu` abbassa la musica
sottostante (duck, **default proposto**) invece di cambiarla; `Options`/`BuildScreen`/
`ExitConfirm` non hanno una traccia propria e non la toccano.

I dieci SFX a evento sono agganciati nei punti reali: sparo del giocatore (UNA volta per
sparo, non per pallettone — `CombatFirePlayer`), colpo a nemico (`CombatDamageEnemy`), danno
al giocatore (`CombatDamagePlayer`), oggetto/valuta/Flux raccolti (`CombatPickup`), porta che
si apre (`WorldTryEnterRoom`), **fusione completata con priorità massima dedicata** (DEC-118,
`AppFusionConfirm`), card di scoperta mostrata (`GameQueueDiscoveryCard`), navigazione/
conferma/annulla nei menu (`UpdateApp`, un solo punto di aggancio generico — vedi il commento
in `app.c` per il perimetro esatto). Volumi master/musica/SFX esposti
(`AudioSetMasterVolume`/`MusicVolume`/`SfxVolume`, default 1.0, clampati) ma **senza ancora
una voce in `Options`**: questo documento non ne fissava lo slider, `Options` resta la
schermata minima di M1a (vedi `ui/options-and-accessibility.md`) — costanti raggiungibili
solo da codice per ora, domanda registrata per il proprietario.

**Lacuna dichiarata**: la famiglia sonora del Piano 0 a due voci (DEC-121, "scelta del
tema"/"generazione completata") **non ha ancora asset dedicati** nel pacchetto pre-generato —
nessun hook è stato aggiunto per non riciclare un SFX semanticamente scorrelato su quei due
eventi. Vedi `docs/engineering/known-issues.md` voce 9.

## Non-obiettivi

Questo documento non definisce suoni, asset o implementazione tecnica del feedback: elenca
solo quali eventi meritano priorità di progettazione. La grammatica audio/visiva completa
resta stato draft e non è definita nel dettaglio qui.

La pipeline tecnica della generazione audio (modelli, formati, budget) non vive qui ma in
`docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` (DEC-109).

## Famiglia sonora del Piano 0 (DEC-121)

Gli eventi del Piano 0 («scelta del tema», «generazione completata») condividono una
stessa **famiglia sonora del crogiolo**, con due segnali riconoscibili al suo interno:
coerenza d'insieme, momenti distinguibili. Si integra col set di simboli di DEC-120.

## Domande aperte residue

- Nessuna: priorità della fusione risolta da DEC-118, famiglia del Piano 0 da DEC-121.

## Scenari

**Scenario: fusione completata nella stanza di fusione**
- Given il giocatore ha due oggetti idonei e il catalizzatore di fusione necessario,
- When completa la fusione esplicita nella stanza di fusione,
- Then il gioco emette il segnale prioritario di fusione, distinto dagli eventi di sinergia
  implicita, a conferma della meccanica-firma.

**Scenario: scelta del tema nel Piano 0**
- Given l'IA ha proposto 2-3 temi nel Piano 0,
- When il giocatore ne sceglie uno,
- Then il gioco emette un feedback prioritario di conferma della scelta, prima di procedere
  con la run.

**Scenario: generazione completata e uscita del Piano 0 disponibile**
- Given il piano 1 è in fase di generazione mentre il giocatore si trova ancora nel Piano 0,
- When la generazione del piano 1 si completa con successo (o tramite fallback-usato, vedi
  `../systems/generated-content-validation.md`),
- Then l'indicatore di generazione lo segnala e il gioco emette il feedback prioritario di
  generazione completata, e l'uscita verso il piano 1 si apre.

**Scenario: l'audio si intensifica ma resta ascoltabile nel piano più avanzato**
- Given una run arrivata al piano 5 con l'audio del tema intensificato al massimo previsto,
- When più eventi sonori del piano suonano in sequenza ravvicinata,
- Then l'audio resta ascoltabile e riconoscibile come parte della stessa grammatica sonora,
  senza degradare in rumore indistinguibile (DEC-024).

**Scenario: la generazione audio fallisce e la catena di fallback tiene**
- Given una run con audio generato da Stable Audio Small per il tema corrente,
- When la generazione di un suono fallisce o produce un output non valido,
- Then il gioco ripiega senza interruzioni sul suono curato equivalente (fallback garantito,
  DEC-178): l'evento critico ha comunque il suo segnale (DEC-109; garanzia ereditata da
  DEC-036).

**Scenario: la demo usa il pacchetto audio pre-generato**
- Given il giocatore gioca la build demo attuale,
- When un evento sonoro qualunque scatta durante la partita,
- Then il suono proviene da un asset statico del pacchetto pre-generato offline (Stable
  Audio 3 Small usato in produzione, non a runtime; nessun rFXGen, uscito dalla pipeline con
  DEC-178), letto dal modulo audio raylib del motore senza alcun modello caricato (DEC-172).
