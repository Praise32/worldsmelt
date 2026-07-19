---
id: gd-content-narrative-tone
status: approved
owner: design
last_reviewed: 2026-07-19
summary: "Cornice narrativa minima canonica approvata: 'il crogiolo dei mondi' (DEC-067) — dà senso a hub, museo, temi e fusione senza imporre lore fissa ai temi generati. Il titolo definitivo del gioco è Worldsmelt (DEC-071, risolve DEC-003) e la nomenclatura inglese in-game è fissata da DEC-072 (fonte unica: governance/glossary.md). Il tono narrativo è ora deciso: ironico-leggero (DEC-105), voce del crogiolo — non del mondo generato, coerente con DEC-073. Restano draft/aperti: limiti di contenuto, simboli ricorrenti. La lingua primaria dei contenuti generati è l'inglese, con l'italiano lingua di sviluppo e test (DEC-052); gap chiuso (M3, 18/07/2026): la pipeline genera ora in inglese."
---

# Narrative Tone

## Cornice narrativa: "il crogiolo dei mondi" (DEC-067)

Questo documento smette di essere un placeholder: registra la cornice narrativa minima
canonica del gioco, approvata dal proprietario del progetto.

Il Piano 0 è un **crogiolo**: un luogo-fucina fuori dal tempo dove i mondi generati dall'IA
nascono, si fondono e infine si sciolgono. Il giocatore è chi si immerge nel crogiolo per
esplorare questi mondi prima che collassino. Il museo del Piano 0 (vedi
`../systems/floor-zero.md`) è la memoria di ciò che il giocatore ha scelto di salvare da
quel collasso.

Questa è la cornice minima canonica: dà senso concettuale a hub, museo, temi e fusione, senza
imporre una lore fissa ai temi generati (che restano liberi, vedi "Vincolo sui temi generati"
sotto). La cornice si appoggia a meccaniche già approvate altrove nella KB, senza
riformularle:

- la **degenerazione dei piani** (DEC-024, fonte unica
  `../07-difficulty-and-progression.md`) è il collasso progressivo del mondo che il
  giocatore sta visitando in quella run;
- la **fusione esplicita** (DEC-012, fonte unica `../systems/item-fusion.md`) è la pratica
  stessa del crogiolo: fondere due cose per crearne una nuova da ciò che si scioglie.

## Campo semantico

Il campo semantico fusione/scioglimento/crogiolo — già disponibile in precedenza come
opzione di vocabolario ed estetica, coerente anche col titolo definitivo del gioco,
Worldsmelt (DEC-071, vedi sotto) — è ora il campo semantico della cornice narrativa canonica
(DEC-067): nomi, descrizioni e presentazione dei contenuti generati possono attingervi
liberamente.

## Vincolo sui temi generati

I temi proposti dall'IA nel Piano 0 (DEC-005, vedi `../systems/floor-zero.md`) restano
**liberi**: la cornice del crogiolo non impone loro un'ambientazione fissa. L'unico vincolo è
che nessun tema generato può **contraddire** la cornice — per esempio negando che il Piano 0
sia un luogo di passaggio fuori dal tempo, o che i mondi visitati siano destinati a sciogliersi.
Dentro questo unico vincolo, la varietà dei temi resta interamente affidata alla generazione.

## Titolo definitivo (DEC-071, risolve DEC-003)

Il titolo definitivo del gioco è **Worldsmelt**, scelto dal proprietario del progetto e
verificato libero da collisioni con giochi esistenti (DEC-071). "Melting Run" resta solo il
nome storico di questo repository e il titolo di lavoro citato nel
[registro delle decisioni](../governance/decision-log.md); non è più usato per riferirsi al
gioco negli altri documenti vivi della KB. Questo chiude la domanda aperta lasciata da
DEC-003. La cornice del crogiolo (DEC-067) restava una cornice narrativa minima indipendente
dal nome: DEC-071 non la modifica, si limita a fissare il titolo.

## Nomenclatura di interfaccia (DEC-072), rimando

I nomi inglesi in-game dei termini di lavoro legati alla cornice del crogiolo (Piano 0,
Fusione, Innesto, Veterano, valuta, ecc.) sono definiti nella mappa bilingue del
[Glossary](../governance/glossary.md) (DEC-072, rimando, non riformulato qui).

## Tono narrativo: ironico-leggero (DEC-105)

Il registro narrativo di Worldsmelt è **ironico-leggero**: il crogiolo ha coscienza di sé e
un filo di humour asciutto nei testi che rivolge al giocatore — card di scoperta (DEC-065,
fonte unica `../ui/hud.md`), testi del museo (fonte unica `../systems/floor-zero.md`) e
schede del Catalogo (DEC-045, fonte unica `../systems/save-and-meta-progression.md`) — senza
rompere l'atmosfera né scadere nella parodia.

L'ironia sta nella **voce del crogiolo**, non nel **mondo generato**: coerente con DEC-073
(fonte unica `../06-ai-content-generation-model.md`), la voce dell'interfaccia/cornice non
contamina i prompt di generazione dei contenuti dei World. Un tema di run può restare cupo,
epico o straniante quanto la generazione lo produce; è il crogiolo che lo racconta — non il
tema stesso — a portare il filo di ironia asciutta.

Questo chiude la domanda residua sul tono specifico registrata più sotto in questo
documento. Restano aperti limiti di contenuto e simboli ricorrenti (vedi "Deve ancora
definire" e "Domande aperte residue").

## Tema per-run (DEC-005) — distinto dal tono narrativo

Il tema di ogni singola run è un meccanismo di variazione **per run**, distinto dal tono
narrativo del gioco (ora fissato come ironico-leggero, DEC-105, sopra): il tono è la voce
con cui il crogiolo si rivolge al giocatore, il tema è il contenuto generato di quella
specifica run:

- l'IA propone 2-3 temi generati nel Piano 0 (vedi `../systems/floor-zero.md`);
- il giocatore ne sceglie uno;
- il tema evolve/degenera piano dopo piano fino al boss del piano 5.

Questo meccanismo è approvato e definito operativamente in `../systems/floor-zero.md`; qui si
registra solo la sua relazione con la cornice narrativa e col tono narrativo (DEC-105, sopra):
un tema di run resta dentro il vincolo del crogiolo (sopra) ma non equivale a una dichiarazione
di tono — il tono è fissato una volta per tutto il gioco, il tema varia run per run.

## Lingua dei contenuti generati (DEC-052)

La lingua primaria del gioco — e quindi della generazione IA di nomi, descrizioni e temi —
è l'**inglese**. L'italiano resta la lingua di sviluppo e di test del progetto, incluso il
vocabolario di limiti di contenuto e simboli ricorrenti ancora provvisori descritti in questo
documento.

Questa decisione riguarda la lingua dei contenuti generati per il giocatore, non la lingua
della KB: la knowledge base resta scritta in italiano, perché è la lingua di lavoro del
team, non la lingua del gioco.

**Stato rispetto al codice (aggiornato 18/07/2026):** gap chiuso. La pipeline di
generazione (prompt, esempi few-shot, pool di nomi curati/fallback) produce ora contenuti
in inglese, coerente con questa regola. Restava un gap di implementazione (la pipeline
produceva contenuti in italiano) fino all'implementazione di M3 (stesso tipo di
gap-di-implementazione dato a DEC-009 in
[Rooms and Floor Generation](../systems/rooms-and-floor-generation.md), che ha un proprio
stato non toccato da questo lavoro).

## Deve ancora definire

La cornice narrativa minima (DEC-067) è decisa, così come il titolo del gioco (DEC-071), la
nomenclatura di interfaccia (DEC-072) e il tono narrativo (DEC-105, ironico-leggero). Restano
ancora da definire, e restano quindi in stato **draft**:

- limiti di contenuto;
- simboli ricorrenti;
- lessico per nomi e descrizioni dei singoli contenuti generati (oggetti, nemici, boss,
  World) caso per caso — distinto dal nome del gioco, dalla nomenclatura di interfaccia e dal
  tono narrativo, ormai fissati.

## Regola temporanea

Finché limiti di contenuto e simboli ricorrenti non sono approvati, usare placeholder
funzionali coerenti con la cornice del crogiolo (DEC-067) e col tono ironico-leggero
(DEC-105), e non produrre grandi quantità di lore definitiva. Il campo semantico
fusione/scioglimento/crogiolo è ora la base di vocabolario approvata (non più una semplice
opzione), e ha ora anche un tono narrativo definitivo (DEC-105); restano provvisori solo i
limiti di contenuto e i simboli ricorrenti.

## Non-obiettivi

Questo documento registra la cornice narrativa minima approvata (DEC-067), il titolo
definitivo del gioco deciso dal proprietario (DEC-071, rimando), il tono narrativo
ironico-leggero (DEC-105) e il meccanismo di tema per-run (DEC-005). Non fissa limiti di
contenuto né simboli ricorrenti, che restano ancora da definire.

## Domande aperte residue

- Limiti di contenuto e simboli ricorrenti: non ancora definiti.
- ~~Tono specifico del gioco (tragico, ironico, inquietante, surreale o altro), dentro la
  cornice del crogiolo (DEC-067)~~: risolto da DEC-105, registro ironico-leggero — vedi
  "Tono narrativo: ironico-leggero (DEC-105)" sopra.
- ~~Tempistica e percorso di migrazione della pipeline di generazione dall'italiano
  all'inglese~~: risolta con M3 (18/07/2026), vedi la nota di stato sopra (DEC-052).

## Scenari

**Scenario: il giocatore sceglie un tema tra quelli proposti nel Piano 0**
- Given l'IA genera 2-3 temi nel Piano 0 per la run in corso,
- When il giocatore ne sceglie uno,
- Then quel tema per-run guida la variazione tematica dei contenuti generati nei piani
  successivi, senza costituire una dichiarazione di tono specifico del gioco.

**Scenario: il tema degenera fino al boss del piano 5**
- Given un tema per-run è stato scelto all'inizio della run,
- When il giocatore avanza piano dopo piano fino al piano 5,
- Then il tema evolve/degenera progressivamente fino a culminare nel boss del piano 5,
  restando coerente con la cornice del crogiolo (DEC-067): la degenerazione è il collasso
  progressivo del mondo visitato.

**Scenario: il titolo definitivo non impone da solo il tono narrativo**
- Given la KB registra "Worldsmelt" come titolo definitivo del gioco (DEC-071),
- When un documento fa riferimento al campo semantico fusione/scioglimento/crogiolo per
  vocabolario o estetica,
- Then quel riferimento resta coerente con la cornice narrativa approvata (DEC-067) e col
  tono ironico-leggero (DEC-105), ma il titolo da solo non è la fonte del tono: il tono è
  fissato separatamente da DEC-105.

**Scenario: il filo di humour del crogiolo non contamina il mondo generato**
- Given una card di scoperta, un testo del museo o una scheda del Catalogo che il crogiolo
  rivolge al giocatore (DEC-105),
- When quel testo viene scritto con un filo di humour asciutto e la coscienza di sé del
  crogiolo,
- Then l'ironia resta nella voce dell'interfaccia/cornice e non entra nei prompt di
  generazione dei contenuti dei World (sprite, nemici, boss, oggetti, stanze, temi), coerente
  con DEC-073: il mondo generato può restare cupo, epico o straniante quanto la generazione
  lo produce.

**Scenario: un tema generato non contraddice la cornice del crogiolo**
- Given l'IA propone un tema per una run nel Piano 0,
- When il tema viene validato prima di comparire come proposta selezionabile,
- Then il tema resta libero in ogni altro aspetto, ma non nega che il Piano 0 sia un
  luogo-fucina fuori dal tempo né che i mondi visitati siano destinati a sciogliersi
  (DEC-067).

**Scenario: la lingua di generazione è l'inglese, la KB resta in italiano**
- Given un documento di design che descrive un contenuto generato dall'IA (nome,
  descrizione, tema),
- When si osserva la lingua prevista per quel contenuto nel gioco finito,
- Then è l'inglese, mentre questo stesso documento di design resta scritto in italiano,
  perché la KB è in una lingua di lavoro distinta da quella del gioco (DEC-052).
