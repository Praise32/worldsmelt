---
id: gd-content-narrative-tone
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Cornice narrativa minima canonica approvata: 'il crogiolo dei mondi' (DEC-067) — dà senso a hub, museo, temi e fusione senza imporre lore fissa ai temi generati. Il titolo definitivo del gioco è Worldsmelt (DEC-071, risolve DEC-003) e la nomenclatura inglese in-game è fissata da DEC-072 (fonte unica: governance/glossary.md). Restano draft/aperti: tono specifico (tragico, ironico, ecc.), simboli ricorrenti, limiti di contenuto. La lingua primaria dei contenuti generati è l'inglese, con l'italiano lingua di sviluppo e test (DEC-052); gap noto: la pipeline attuale genera in italiano."
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

## Tema per-run (DEC-005) — distinto dal tono narrativo specifico

Il tema di ogni singola run è un meccanismo di variazione **per run**, distinto dal tono
narrativo specifico del gioco (tragico, ironico, inquietante, surreale o altro), che resta
non deciso:

- l'IA propone 2-3 temi generati nel Piano 0 (vedi `../systems/floor-zero.md`);
- il giocatore ne sceglie uno;
- il tema evolve/degenera piano dopo piano fino al boss del piano 5.

Questo meccanismo è approvato e definito operativamente in `../systems/floor-zero.md`; qui si
registra solo la sua relazione con la cornice narrativa e col tono specifico ancora aperto: un
tema di run resta dentro il vincolo del crogiolo (sopra) ma non equivale a una dichiarazione di
tono specifico del gioco.

## Lingua dei contenuti generati (DEC-052)

La lingua primaria del gioco — e quindi della generazione IA di nomi, descrizioni e temi —
è l'**inglese**. L'italiano resta la lingua di sviluppo e di test del progetto, incluso il
tono e il vocabolario specifico ancora provvisori descritti in questo documento.

Questa decisione riguarda la lingua dei contenuti generati per il giocatore, non la lingua
della KB: la knowledge base resta scritta in italiano, perché è la lingua di lavoro del
team, non la lingua del gioco.

**Gap noto rispetto al codice:** la pipeline di generazione attuale produce contenuti in
italiano. Questo è un requisito di design non ancora implementato — il codice dovrà
adeguarsi a questa regola, non viceversa (stesso trattamento di gap dato a DEC-009 in
[Rooms and Floor Generation](../systems/rooms-and-floor-generation.md)).

## Deve ancora definire

La cornice narrativa minima (DEC-067) è decisa, così come il titolo del gioco (DEC-071) e la
nomenclatura di interfaccia (DEC-072). Restano ancora da definire, e restano quindi in stato
**draft**:

- tono specifico: tragico, ironico, inquietante, surreale o altro, dentro la cornice del
  crogiolo;
- limiti di contenuto;
- simboli ricorrenti;
- lessico per nomi e descrizioni dei singoli contenuti generati (oggetti, nemici, boss,
  World) caso per caso — distinto dal nome del gioco e dalla nomenclatura di interfaccia,
  ormai fissati.

## Regola temporanea

Finché il tono specifico non è approvato, usare placeholder funzionali coerenti con la
cornice del crogiolo (DEC-067) e non produrre grandi quantità di lore definitiva. Il campo
semantico fusione/scioglimento/crogiolo è ora la base di vocabolario approvata (non più una
semplice opzione), ma resta comunque priva di un tono specifico definitivo.

## Non-obiettivi

Questo documento non sceglie un tono narrativo specifico: registra la cornice narrativa
minima approvata (DEC-067), il titolo definitivo del gioco deciso dal proprietario (DEC-071,
rimando) e il meccanismo di tema per-run (DEC-005). Il tono specifico resta ancora da
definire.

## Domande aperte residue

- Tono specifico del gioco (tragico, ironico, inquietante, surreale o altro), dentro la
  cornice del crogiolo (DEC-067): nessuna scelta ancora fatta.
- Limiti di contenuto e simboli ricorrenti: non ancora definiti.
- Tempistica e percorso di migrazione della pipeline di generazione dall'italiano
  all'inglese (DEC-052 fissa la regola, non il piano di implementazione).

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

**Scenario: il titolo definitivo non impone da solo un tono specifico**
- Given la KB registra "Worldsmelt" come titolo definitivo del gioco (DEC-071),
- When un documento fa riferimento al campo semantico fusione/scioglimento/crogiolo per
  vocabolario o estetica,
- Then quel riferimento resta coerente con la cornice narrativa approvata (DEC-067), ma non
  deve essere interpretato come una decisione definitiva sul tono narrativo specifico del
  gioco, che resta aperto.

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
