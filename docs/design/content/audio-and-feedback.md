---
id: gd-content-audio-feedback
title: Audio and Feedback
domain: design
status: draft
authority: canonical
owner: design
summary: "Feedback per azioni, rischi e sinergie; elenco eventi prioritari con fusione, scelta del tema e generazione completata nel Piano 0. L'audio è uno dei quattro assi dell'escalation leggibile del tema per piano (DEC-024). Dal 22/07 la via primaria è generativa: Stable Audio Small in locale con catena di fallback rFXGen → curato (DEC-109); ogni evento critico mantiene comunque un suono curato o di fallback."
last_reviewed: 2026-07-27
topics: [audio, feedback, eventi prioritari, DEC-024, DEC-036, DEC-109, stable-audio, rfxgen]
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

## Audio generativo con catena di fallback (DEC-109, sostituisce la parte «futuro» di DEC-036)

Dal 22/07 la via primaria per musica e SFX è **generativa**: **Stable Audio Small in
locale**, con catena di fallback obbligatoria **rFXGen** (SFX procedurali) → **audio
curato/statico**. La garanzia storica di DEC-036 sopravvive come rete: ogni evento critico
ha sempre un suono curato o di fallback, e la modalità solo-curato resta completa e
dignitosa. Vincoli architetturali: nessuna generazione durante il combattimento; il modello
audio si carica in sequenza con il modello di testo attivo e SD (mai insieme nei 6 GB di
riferimento); cache e pubblicazione atomica (pipeline tecnica in
`docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md`; licenza: DEC-113).

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
- Then il gioco ripiega senza interruzioni sulla catena di fallback (rFXGen, poi il suono
  curato equivalente): l'evento critico ha comunque il suo segnale (DEC-109; garanzia
  ereditata da DEC-036).
