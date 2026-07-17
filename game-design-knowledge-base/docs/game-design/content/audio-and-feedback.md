---
id: gd-content-audio-feedback
status: draft
owner: design
last_reviewed: 2026-07-17
summary: "Feedback per azioni, rischi e sinergie; elenco eventi prioritari aggiornato con fusione, scelta del tema e generazione completata nel Piano 0."
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

## Non-obiettivi

Questo documento non definisce suoni, asset o implementazione tecnica del feedback: elenca
solo quali eventi meritano priorità di progettazione. La grammatica audio/visiva completa
resta stato draft e non è definita nel dettaglio qui.

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
