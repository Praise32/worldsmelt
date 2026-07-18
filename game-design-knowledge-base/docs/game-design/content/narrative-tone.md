---
id: gd-content-narrative-tone
status: draft
owner: design
last_reviewed: 2026-07-18
summary: "Tono narrativo generale ancora non deciso; nome di lavoro provvisorio (DEC-003) e meccanismo di tema per-run (DEC-005) documentati separatamente. La lingua primaria dei contenuti generati è l'inglese, con l'italiano lingua di sviluppo e test (DEC-052); gap noto: la pipeline attuale genera in italiano."
---

# Narrative Tone

## Stato

Il tono narrativo generale non è ancora stato specificato. È una decisione ad alta priorità
perché guida nomi, sprite, stanze, boss e descrizioni generate. Questo documento resta
esplicitamente in stato draft: non definisce un tono, per non anticipare una decisione non
ancora presa dal proprietario del progetto.

## Nome di lavoro provvisorio (DEC-003)

"Melting Run" è un titolo di lavoro **provvisorio**. Il nome definitivo del gioco sarà scelto
dal proprietario del progetto; resta una domanda aperta e non va risolta in questo documento
né altrove nella KB.

Il campo semantico fusione/scioglimento è disponibile come base di vocabolario ed estetica —
coerente sia con il nome provvisorio sia con la meccanica-firma di fusione (vedi
`../systems/item-fusion.md`) — ma resta un'**opzione**, non un tono narrativo deciso. Non va
trattato come se fosse già il tono ufficiale del gioco.

## Tema per-run (DEC-005) — distinto dal tono narrativo generale

Il tema di ogni singola run è un meccanismo di variazione **per run**, distinto dal tono
narrativo generale del gioco (che resta non deciso):

- l'IA propone 2-3 temi generati nel Piano 0 (vedi `../systems/floor-zero.md`);
- il giocatore ne sceglie uno;
- il tema evolve/degenera piano dopo piano fino al boss del piano 5.

Questo meccanismo è approvato e definito operativamente in `../systems/floor-zero.md`; qui si
registra solo la sua relazione con il tono narrativo generale: un tema di run non equivale a
una dichiarazione di tono complessivo del gioco, che resta da definire.

## Lingua dei contenuti generati (DEC-052)

La lingua primaria del gioco — e quindi della generazione IA di nomi, descrizioni e temi —
è l'**inglese**. L'italiano resta la lingua di sviluppo e di test del progetto, incluso il
tono e il vocabolario ancora provvisori descritti in questo documento.

Questa decisione riguarda la lingua dei contenuti generati per il giocatore, non la lingua
della KB: la knowledge base resta scritta in italiano, perché è la lingua di lavoro del
team, non la lingua del gioco.

**Gap noto rispetto al codice:** la pipeline di generazione attuale produce contenuti in
italiano. Questo è un requisito di design non ancora implementato — il codice dovrà
adeguarsi a questa regola, non viceversa (stesso trattamento di gap dato a DEC-009 in
[Rooms and Floor Generation](../systems/rooms-and-floor-generation.md)).

## Deve ancora definire

- mondo e premessa;
- ruolo del protagonista;
- significato diegetico della generazione;
- tono generale: tragico, ironico, inquietante, surreale o altro;
- limiti di contenuto;
- simboli ricorrenti;
- lessico per nomi e descrizioni (compreso il nome definitivo del gioco, DEC-003).

## Regola temporanea

Finché il tono generale non è approvato, usare placeholder funzionali e non produrre grandi
quantità di lore definitiva. Il campo semantico fusione/scioglimento può essere usato come
opzione di lavoro per placeholder coerenti col nome provvisorio, senza che questo costituisca
una decisione di tono.

## Non-obiettivi

Questo documento non sceglie un tono narrativo, non propone un nome definitivo per il gioco e
non risolve la domanda aperta di DEC-003: registra solo cosa è deciso (il meccanismo di tema
per-run, DEC-005) e cosa resta aperto (tono generale, nome definitivo).

## Domande aperte residue

- Nome definitivo del gioco (DEC-003, decisione del proprietario del progetto, non di questa
  KB).
- Tono narrativo generale (tragico, ironico, inquietante, surreale o altro): nessuna scelta
  ancora fatta.
- Se il campo semantico fusione/scioglimento diventerà il tono ufficiale o resterà solo
  un'opzione di vocabolario tra altre.
- Tempistica e percorso di migrazione della pipeline di generazione dall'italiano
  all'inglese (DEC-052 fissa la regola, non il piano di implementazione).

## Scenari

**Scenario: il giocatore sceglie un tema tra quelli proposti nel Piano 0**
- Given l'IA genera 2-3 temi nel Piano 0 per la run in corso,
- When il giocatore ne sceglie uno,
- Then quel tema per-run guida la variazione tematica dei contenuti generati nei piani
  successivi, senza costituire una dichiarazione di tono narrativo generale del gioco.

**Scenario: il tema degenera fino al boss del piano 5**
- Given un tema per-run è stato scelto all'inizio della run,
- When il giocatore avanza piano dopo piano fino al piano 5,
- Then il tema evolve/degenera progressivamente fino a culminare nel boss del piano 5.

**Scenario: il nome provvisorio non implica un tono deciso**
- Given la KB usa "Melting Run" come titolo di lavoro provvisorio (DEC-003),
- When un documento fa riferimento al campo semantico fusione/scioglimento per vocabolario o
  estetica,
- Then quel riferimento resta un'opzione di lavoro e non deve essere interpretato come una
  decisione definitiva sul tono narrativo generale del gioco.

**Scenario: la lingua di generazione è l'inglese, la KB resta in italiano**
- Given un documento di design che descrive un contenuto generato dall'IA (nome,
  descrizione, tema),
- When si osserva la lingua prevista per quel contenuto nel gioco finito,
- Then è l'inglese, mentre questo stesso documento di design resta scritto in italiano,
  perché la KB è in una lingua di lavoro distinta da quella del gioco (DEC-052).
