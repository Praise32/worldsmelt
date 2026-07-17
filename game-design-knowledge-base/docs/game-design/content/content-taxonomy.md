---
id: gd-content-taxonomy
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Vocabolario condiviso per contenuti e tag; fonte unica dei 4 valori di origine del contenuto (curato, composto, variato, nuovo)."
---

# Content Taxonomy

## Scopo

Permettere a persone e agenti di descrivere contenuti con termini coerenti.

## Famiglie di tag iniziali

- **Ruolo:** danno, difesa, mobilità, economia, controllo, rischio.
- **Elemento:** fuoco, freddo, elettrico, corrosivo, fisico, psichico o equivalenti originali.
- **Forma:** punta, onda, raggio, orbita, esplosione, frammentazione.
- **Bersaglio:** singolo, area, catena, automatico.
- **Frequenza:** comune, non comune, raro, eccezionale.
- **Complessità:** base, combinato, avanzato.

## Origine del contenuto — fonte unica

Questa sezione è la fonte UNICA della tassonomia di origine per l'intera knowledge base.
Esattamente 4 valori, in questo ordine e con questi nomi esatti: `curato`, `composto`,
`variato`, `nuovo`. I template dei contenuti (es. `item-spec-template.md`) usano il campo
`origin:` con esattamente questi 4 valori. Nessun altro documento deve introdurre una
tassonomia di origine diversa o in conflitto con questa.

- **curato**: scritto a mano da un designer umano. Non è generato dall'IA in alcuna forma.
- **composto**: assemblato dall'IA combinando elementi curati già esistenti, senza inventare
  parametri nuovi al di fuori di quelli forniti dai componenti curati.
- **variato**: l'IA modifica parametri o produce varianti di un contenuto curato o composto
  esistente, restando dentro le bande di garanzia dichiarate per quella categoria.
- **nuovo**: l'IA inventa un contenuto parametrico nuovo dentro bande di garanzia (il caso
  descritto in `../systems/generated-content-validation.md`, DEC-020) — non deriva da un
  singolo contenuto curato preesistente come punto di partenza diretto, ma da un modello di
  generazione libero all'interno dei limiti dichiarati per la categoria.

Ogni contenuto generato deve dichiarare la propria origine tra questi 4 valori, indipendente
dal suo stato di validazione (vedi `../systems/generated-content-validation.md` per i sei
stati: proposto, strutturalmente-valido, simulato, approvato-per-run, respinto,
fallback-usato). Origine e stato di validazione sono due assi indipendenti: un contenuto
"nuovo" può essere respinto, e un contenuto "curato" è per definizione già affidabile e in
genere usato proprio come riserva nei fallback.

## Regola

Ogni tag deve avere significato meccanico o di presentazione documentato. Evitare sinonimi
duplicati.

## Casi limite

- Un contenuto composto che viene poi variato dall'IA in una run successiva mantiene
  l'origine dichiarata al momento della sua creazione (`composto`), non diventa retroattivamente
  `variato`; una nuova variante generata a partire da esso è un contenuto distinto con propria
  origine `variato`.
- Un contenuto curato usato come fallback (vedi
  `../systems/generated-content-validation.md`) resta di origine `curato` anche quando compare
  al posto di un contenuto generato che è stato respinto.

## Non-obiettivi

Questo documento non definisce i controlli di validazione o gli stati che un contenuto
attraversa prima di comparire in gioco (vedi `../systems/generated-content-validation.md`);
definisce solo da dove il contenuto proviene concettualmente.

## Domande aperte residue

- Se un contenuto `variato` derivato da un contenuto `nuovo` debba ereditare eventuali vincoli
  aggiuntivi del genitore oltre alle bande di garanzia generali della categoria.

## Scenari

**Scenario: un colpo scritto dal team è curato**
- Given un designer scrive a mano un tipo di colpo per il pool di base,
- When quel colpo entra nella KB e nei pool del gioco,
- Then la sua origine è `curato`.

**Scenario: l'IA assembla un oggetto da componenti curati**
- Given due effetti curati esistenti nel pool,
- When l'IA li combina in un nuovo oggetto senza inventare parametri fuori da quelli forniti
  dai due componenti,
- Then l'origine dell'oggetto risultante è `composto`.

**Scenario: l'IA varia un nemico curato dentro le bande garantite**
- Given un nemico curato esistente con statistiche di base note,
- When l'IA ne genera una variante con parametri diversi ma dentro le bande di garanzia
  dichiarate per quella categoria di nemico,
- Then l'origine del nemico risultante è `variato`.

**Scenario: l'IA inventa un tipo di colpo interamente nuovo**
- Given una categoria di contenuto con bande di garanzia dichiarate (es. tipi di colpo),
- When l'IA genera un contenuto parametrico che non deriva da un singolo contenuto curato
  preesistente ma resta dentro quelle bande,
- Then l'origine del contenuto risultante è `nuovo` (DEC-020).
