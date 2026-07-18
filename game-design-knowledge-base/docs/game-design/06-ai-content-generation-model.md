---
id: gd-ai-content-model
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Modello reale di generazione: invenzione parametrica entro bande di garanzia e comportamenti validati in sandbox con fallback curato. La pipeline comportamentale copre anche il trait del personaggio generato e i tipi di colpo, con le manopole parametriche come garanzia/fallback (DEC-037). La lingua primaria di generazione è l'inglese; gap noto: la pipeline attuale genera in italiano (DEC-052). La modalità 'solo curato', scelta al primo avvio, è uno stato legittimo e permanente, non un fallback temporaneo (DEC-070). La nomenclatura di interfaccia del crogiolo (DEC-072) non entra nei prompt di generazione dei World: quei prompt descrivono solo funzione e tema (DEC-073a)."
---

# AI Content Generation Model

## Principio (DEC-020)

L'IA locale non "sceglie da un menu": **inventa** contenuti. Il modo in cui inventa dipende
dal tipo di contenuto:

1. Per contenuti **parametrici** (tipi di colpo, nemici, layout di stanze), l'IA genera
   valori e combinazioni nuovi ma vincolati dentro **bande di garanzia** numeriche definite
   dal game design (vedi ad esempio le bande di potenza in
   [Difficulty and Progression](07-difficulty-and-progression.md)).
2. Per contenuti **comportamentali** (oggetti, sinergie, fusioni), l'IA scrive un
   comportamento che viene **validato in sandbox** prima di entrare in run, con un
   **fallback curato sempre presente** in caso di rifiuto.

In entrambi i casi vale la stessa garanzia: mai un contenuto rotto arriva al giocatore, mai
la generazione blocca la partita.

## Tipi di contenuto

- Tipi di colpo (parametrico entro bande, come garanzia e fallback; anche comportamentale,
  scritto in Lua e validato in sandbox con grande varietà — DEC-037, vedi
  [Combat and Projectiles](systems/combat-and-projectiles.md)).
- Nemici e boss (parametrico per statistiche/pattern, entro bande).
- Layout di stanze (parametrico, entro bande e vincoli strutturali).
- Oggetti, inclusi gli oggetti di fusione (comportamentale, validato in sandbox; vedi
  [Item Fusion](systems/item-fusion.md)).
- Trait unico del personaggio alternativo generato per run (comportamentale, scritto in Lua
  e validato in sandbox con la stessa pipeline degli oggetti — DEC-037, vedi
  [Characters](systems/characters.md)).
- Sinergie tra effetti (comportamentale).
- Temi di piano e la loro evoluzione/degenerazione (composizione + variazione).
- Aspetto e composizione degli sprite.
- Nomi e descrizioni coerenti con la tassonomia.

## Origine del contenuto — tassonomia unica

Ogni contenuto generato dichiara un'origine tra esattamente **quattro valori**. Questa è la
fonte unica della tassonomia di origine; altri documenti (template, glossario, tassonomia
dei contenuti) devono usare questi stessi quattro valori, senza sinonimi:

| Origine | Significato |
|---|---|
| `curato` | Contenuto creato e approvato manualmente, nessuna generazione. |
| `composto` | Nuova combinazione di moduli già validati e conosciuti. |
| `variato` | Contenuto esistente con parametri modificati entro bande di garanzia sicure. |
| `nuovo` | Introduce una regola o un archetipo non ancora visto; richiede validazione più severa. |

## Vincoli obbligatori

Ogni contenuto generato deve avere:

- identità univoca nella run;
- categoria e tag;
- origine dichiarata (uno dei quattro valori sopra);
- budget di potenza o pericolo, rispettato entro le bande di garanzia applicabili;
- segnali visivi e audio;
- descrizione comprensibile;
- dipendenze dichiarate;
- incompatibilità dichiarate;
- fallback curato;
- test di giocabilità (simulazione o validazione in sandbox per i contenuti
  comportamentali).

## Processo per contenuti comportamentali (oggetti, sinergie, fusioni)

1. L'IA scrive il comportamento proposto.
2. Il comportamento passa attraverso gli stati di validazione definiti in
   [Generated Content Validation](systems/generated-content-validation.md) (fonte unica per
   quel processo; questo documento non lo ripete).
3. Se il contenuto non supera la validazione, il fallback curato prende il suo posto senza
   interrompere la run.

## Processo per contenuti parametrici (colpi, nemici, layout)

1. L'IA genera valori e combinazioni nuovi dentro le bande di garanzia numeriche vigenti
   (rarità, potenza, densità, dimensione minima di stanza).
2. Non è richiesta una sandbox comportamentale separata: il rispetto delle bande è di per sé
   la garanzia di sicurezza, verificata da controlli strutturali minimi.

## Comportamenti Lua per oggetti, trait di personaggio e tipi di colpo (DEC-037)

La pipeline comportamentale, nata per gli oggetti, si estende ad altri due contenuti:

- il **trait unico del personaggio alternativo** generato per ogni run (vedi
  [Characters](systems/characters.md));
- i **tipi di colpo** (per il giocatore e per i nemici), che il proprietario vuole vedere
  evolvere verso una grande varietà di comportamenti scriptati (vedi
  [Combat and Projectiles](systems/combat-and-projectiles.md)).

In entrambi i casi il comportamento è scritto dall'IA e validato in sandbox con la stessa
pipeline già usata per i comportamenti degli oggetti (vedi "Processo per contenuti
comportamentali" sopra). Per i tipi di colpo, le manopole parametriche descritte nel
processo per contenuti parametrici **restano attive** come garanzia di bilanciamento e come
fallback: un colpo scriptato in Lua che non supera la validazione ricade su una versione
parametrica curata equivalente. Questo documento non ridefinisce i dettagli tecnici del
linguaggio o della sandbox: quei dettagli, quando esistono, vivono nella documentazione
tecnica del progetto, fuori da questa KB di game design.

## Trasparenza al giocatore (fonte unica)

Questa è la fonte unica della regola: il gioco può comunicare che la run è stata generata,
ma **non deve mostrare prompt, errori interni o altri dettagli tecnici** durante
l'esperienza normale. Ogni altro documento che tocchi questo argomento (ad esempio
[Run Structure](04-run-structure.md) o [Generation Status](ui/generation-status.md)) rimanda
qui invece di riformulare la regola.

## Modalità "solo curato" (DEC-070)

Il giocatore può scegliere, al primo avvio del gioco (dettaglio completo in
[Floor Zero](systems/floor-zero.md), rimando, non riformulato qui), di giocare in modalità
**solo curato**: nessun modello IA attivo, solo contenuti curati e fallback procedurale.
Questa modalità NON è un fallback d'emergenza temporaneo: è uno stato legittimo e
**permanente** del gioco, tanto quanto la modalità con generazione attiva. Il giocatore può
riattivare la generazione in un secondo momento.

Questa distinzione è concettualmente separata dal fallback per singolo contenuto descritto in
[Generated Content Validation](systems/generated-content-validation.md): quel fallback
sostituisce un contenuto specifico non validato all'interno di una run con generazione
attiva; la modalità solo curato disattiva la generazione stessa, per scelta esplicita del
giocatore, non per un errore di validazione.

## Limite

L'IA non può modificare arbitrariamente regole fondamentali come input, condizioni di
vittoria, significato delle risorse o segnali di pericolo senza una modalità esplicitamente
dedicata.

## Nota — priorità delle anteprime visive dei temi (DEC-039)

Tra le generazioni richieste a inizio run, le anteprime visive dei 2-3 temi proposti nel
Piano 0 (es. uno sprite campione di un nemico del tema) hanno **priorità altissima**: la
scelta del tema dipende da queste anteprime. Il dettaglio completo del comportamento e del
fallback (nome+descrizione senza anteprima) vive in [Floor Zero](systems/floor-zero.md)
(rimando, non riformulato qui).

## Nota — card di scoperta breve (DEC-065), rimando

Alla prima occorrenza di un contenuto generato mai visto, il gioco mostra una card di
scoperta breve, non bloccante; l'elemento vive nell'HUD ed è descritto in
[HUD](ui/hud.md#card-di-scoperta-breve-dec-065) (rimando, non riformulato qui).

## Nota — lingua della generazione (DEC-052), gap di implementazione

La lingua primaria del gioco, inclusi i contenuti generati dall'IA (nomi, descrizioni,
temi), è l'inglese; l'italiano resta la lingua di sviluppo e di test del progetto (dettaglio
completo in [Narrative Tone](content/narrative-tone.md), non riformulato qui).

**Stato:** regola di design approved. **Gap noto rispetto al codice:** la pipeline di
generazione attuale produce contenuti in italiano. È un requisito di design non ancora
implementato — il codice dovrà adeguarsi a questa regola, non viceversa (stesso trattamento
del gap già registrato per DEC-009 in
[Rooms and Floor Generation](systems/rooms-and-floor-generation.md)).

## Nota — la nomenclatura non contamina i prompt dei World (DEC-073a)

I nomi di gioco della nomenclatura di interfaccia (Smelting, Flux, Tempered, ecc. — fonte
unica: [Glossary](governance/glossary.md), DEC-072) NON entrano nei prompt di generazione dei
contenuti dei World (sprite, nemici, boss, oggetti, stanze): quei prompt descrivono funzione
e tema del World scelto dal giocatore, non il vocabolario di fonderia. Il vocabolario di
fonderia appartiene all'interfaccia e alla cornice del Crucible (DEC-067, vedi
[Narrative Tone](content/narrative-tone.md)), non ai mondi generati.

## Casi limite

- Un contenuto `variato` supera la banda di garanzia per un errore di generazione: viene
  trattato come `rejected` nel processo di validazione e sostituito dal fallback curato.
- Un contenuto `nuovo` introduce una regola che confligge con un vincolo fondamentale (es.
  altera il significato di una risorsa): viene respinto a prescindere dal risultato della
  simulazione.

## Non-obiettivi

- Questo documento non definisce gli stati di validazione né i controlli minimi: quelli
  sono in [Generated Content Validation](systems/generated-content-validation.md).
- Non definisce i valori numerici delle bande di garanzia: quelli sono default
  d'implementazione `draft`, vedi [Difficulty and Progression](07-difficulty-and-progression.md)
  (DEC-019).

## Domande aperte residue

- Se i layout di stanza generati richiedano una categoria di validazione strutturale
  dedicata, separata dai controlli minimi generali.

## Scenari

- **Dato** che l'IA genera una variazione di un tipo di colpo esistente, **quando** i suoi
  parametri restano dentro la banda di garanzia definita, **allora** il contenuto è
  utilizzabile in run senza passare per la sandbox comportamentale.
- **Dato** che l'IA scrive il comportamento di un nuovo oggetto di fusione, **quando** il
  comportamento non supera la validazione in sandbox, **allora** il fallback curato prende
  il suo posto e il giocatore non vede alcun errore né dettaglio tecnico.
- **Dato** che un contenuto dichiara origine `nuovo`, **quando** entra nel processo di
  validazione, **allora** è soggetto a controlli più severi rispetto a un contenuto
  `variato` o `composto`.
- **Dato** che il gioco comunica che la run è stata generata, **quando** mostra
  quell'informazione al giocatore, **allora** non include mai prompt, log o messaggi di
  errore interni.
- **Dato** che l'IA scrive il trait unico del personaggio alternativo o un tipo di colpo
  come comportamento Lua (DEC-037), **quando** il comportamento non supera la validazione in
  sandbox, **allora** il fallback curato prende il suo posto — per i tipi di colpo, la
  versione parametrica dentro le bande di garanzia — senza che il giocatore veda alcun
  errore.
- **Dato** che la pipeline di generazione produce oggi contenuti in italiano, **quando** si
  confronta con la regola di design (DEC-052), **allora** il documento registra questo come
  un gap di implementazione noto, non come comportamento canonico da preservare.
- **Dato** che un giocatore ha scelto la modalità "solo curato" al primo avvio, **quando**
  gioca una run in quella modalità, **allora** incontra solo contenuti curati e fallback
  procedurali, senza che questo venga trattato come un errore o una condizione temporanea da
  correggere (DEC-070).
- **Dato** che l'IA genera il prompt per un nemico o un oggetto del World scelto per la run,
  **quando** quel prompt viene composto, **allora** descrive solo funzione e tema del World,
  senza includere termini della nomenclatura di interfaccia come Smelting, Flux o Tempered
  (DEC-073a).
