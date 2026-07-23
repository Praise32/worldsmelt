---
id: gd-content-audio-feedback
title: Audio and Feedback
domain: design
status: draft
authority: canonical
owner: design
summary: "Feedback per azioni, rischi e sinergie; elenco eventi prioritari aggiornato con fusione, scelta del tema e generazione completata nel Piano 0. L'audio è uno dei quattro assi dell'escalation leggibile del tema per piano (DEC-024). Per ora musica e suoni sono curati e statici; la generazione audio a tema resta un'idea futura (DEC-036)."
last_reviewed: 2026-07-18
topics: [audio, feedback, eventi prioritari, DEC-024, DEC-036]
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
  esplicita nella stanza di fusione (vedi `../systems/item-fusion.md`);
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

## Audio curato per ora, generazione futura (DEC-036)

Per ora musica e suoni sono **curati e statici**: non generati dall'IA. L'asse audio
dell'escalation del tema (DEC-024, sopra) si realizza oggi con i mezzi curati disponibili —
selezione e mix di suoni già pronti — senza alcuna generazione audio a tema. La
parametrizzazione o generazione audio a tema è un'**idea futura**, parcheggiata insieme alle
altre idee non ancora requisiti (vedi DEC-018 nel
[decision log](../governance/decision-log.md)).

## Non-obiettivi

Questo documento non definisce suoni, asset o implementazione tecnica del feedback: elenca
solo quali eventi meritano priorità di progettazione. La grammatica audio/visiva completa
resta stato draft e non è definita nel dettaglio qui.

Non copre la generazione o parametrizzazione audio a tema: quella resta un'idea futura non
progettata (DEC-036).

## Domande aperte residue

- Se l'evento "fusione" debba avere una priorità sonora superiore agli altri eventi
  prioritari, data la sua natura di meccanica-firma (non deciso).
- Se "generazione completata" nel Piano 0 debba avere un segnale distinto da "scelta del tema"
  o condividere una stessa famiglia sonora (non deciso).

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

**Scenario: intensificazione audio con mezzi curati, senza generazione**
- Given una run che degenera verso i piani più avanzati,
- When il sistema intensifica l'asse audio del tema secondo DEC-024,
- Then lo fa componendo selezione e mix di suoni già curati, senza alcuna generazione audio
  a tema: la generazione resta un'idea futura non ancora implementata (DEC-036).
