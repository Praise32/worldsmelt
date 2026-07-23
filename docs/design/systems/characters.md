---
id: gd-system-characters
title: Characters
domain: design
status: approved
authority: canonical
owner: design
summary: "Rosa base canonica di 3 personaggi con nomi e ruoli fissi — Wayfinder (esploratore, personaggio di partenza e default), Ashblade (offensivo di vetro), Bulwark (difensivo di roccia) (DEC-080) — più un personaggio alternativo generato per ogni run che si aggiunge alla rosa nella scelta del Piano 0 (DEC-030); il trait unico del personaggio generato è un comportamento Lua validato in sandbox (DEC-037), con varietà anti-fotocopia rispetto alle run recenti via catalogo (DEC-098). Sblocchi della rosa base ora canonici (DEC-100): Wayfinder da subito, Ashblade alla prima run conclusa (qualunque esito), Bulwark al primo boss abbattuto — solo le statistiche restano da playtest. Il rifiuto dell'alternativa nel Piano 0 è ripensabile fino all'attraversamento dell'uscita (DEC-097). Sprite: curati a mano per la rosa base, generati dalla pipeline sprite esistente (come i nemici) per il personaggio alternativo (DEC-049). Il personaggio alternativo può avere, a volte, un colpo firmato generato: parte del suo budget, con statistiche compresse verso il bordo cauto delle bande in cambio, criterio canonico ma fattore ancora da playtest (DEC-068, DEC-078); il colpo firmato non si scarta mai, si normalizza in banda (DEC-079); è sempre il colpo di partenza, sostituito e ripristinato come ogni altro colpo di partenza, canone (DEC-099); i personaggi base usano sempre colpi standard curati. Nelle gare a stesso seed (Classificata stesso seed, Daily) le proposte di personaggio sono identiche per tutti i partecipanti, scelta libera (DEC-108)."
last_reviewed: 2026-07-19
last_verified_commit: 0ec60d0
topics: [personaggi, rosa base, personaggio generato, colpo firmato, DEC-080, DEC-100, trait Lua]
related: []
supersedes: []
source_files: []
---

# Characters

## Intento per il giocatore

Ogni run offre una decisione d'identità semplice: scegliere uno dei personaggi curati della
rosa base — pochi, distinti, pensati per ruoli di gioco diversi — oppure accettare
un'alternativa generata apposta per quella run, con un trait unico e statistiche diverse.
Per l'alternativa resta una scelta "prendere o lasciare"; per la rosa base è una vera scelta
tra opzioni curate.

## Condizioni di ingresso

- La scelta avviene nel Piano 0, prima di attraversare l'uscita verso il piano 1 (vedi
  [Floor Zero](floor-zero.md)).
- La rosa dei personaggi base è sempre disponibile, in ogni run, senza condizioni (fatti
  salvi gli sblocchi previsti da DEC-030 e fissati in dettaglio da DEC-100, vedi sotto).
- Il personaggio alternativo generato per la run si **aggiunge** alla rosa base nella scelta
  del Piano 0: esiste solo se la generazione per quella run lo ha prodotto e validato; in
  caso contrario non compare come opzione (vedi "Fallback").

## Rosa di personaggi base (DEC-030, DEC-080, DEC-100)

I personaggi base non sono un singolo personaggio ma una **piccola rosa fissa e curata di
3 personaggi**, con nomi e ruoli ora **canonici** (DEC-080): **Wayfinder** (esploratore,
personaggio di partenza e default), **Ashblade** (offensivo di vetro: danno alto, tetto vita
basso), **Bulwark** (difensivo di roccia: lento, tetto vita alto). Le condizioni di sblocco
sono ora **canoniche** (DEC-100): **Wayfinder** è disponibile da subito, è il personaggio di
partenza; **Ashblade** si sblocca alla **prima run conclusa**, qualunque esito (vittoria,
sconfitta o abbandono); **Bulwark** si sblocca al **primo boss abbattuto**. Sono traguardi
naturali e precoci, come volevano gli "sbloccabili presto" di DEC-030: resta da playtest solo
la statistica esatta di ciascun personaggio (vedi Domande aperte residue e punto 8 di
`../governance/open-questions.md`).

Il ruolo di ciascun personaggio della rosa base si riflette anche nel proprio **tetto di
salute base** (DEC-033): il ruolo difensivo (Bulwark) ha un tetto alto ("personaggio-roccia"),
il ruolo offensivo (Ashblade) un tetto più basso ("personaggio-vetro"), come parte
curata delle sue statistiche — non un valore unico condiviso da tutta la rosa. Il dettaglio
del meccanismo del tetto vive in
[Health and Resources](health-and-resources.md) (rimando, non riformulato qui).

Il personaggio alternativo generato per run (vedi sotto) non sostituisce la rosa base: la
scelta nel Piano 0 avviene tra i personaggi della rosa (quelli già sbloccati) più
l'eventuale alternativa generata per quella run.

### Statistiche: default proposti dall'implementazione (stile DEC-019, M6a)

Nomi e ruoli della rosa qui sopra sono canone (DEC-080); restano **default proposti in fase
di implementazione**, come i valori numerici di DEC-019 (pesi rarità, bande di potenza), solo
le **statistiche esatte** della tabella sotto, per rendere la rosa giocabile subito senza
ancora numeri approvati dal design: restano da validare col playtest (vedi Domande aperte
residue e `../governance/open-questions.md` punto 8).

| | Wayfinder | Ashblade | Bulwark |
|---|---|---|---|
| Ruolo | Explorer | Offensive | Defensive |
| Danno base | 8.0 | 10.0 | 7.0 |
| Cadenza base | 0.23s | 0.21s | 0.26s |
| Velocità base | 240 | 230 | 204 |
| Salute base / tetto | 6 / 12 | 4 / 8 | 8 / 16 |
| Fortuna base | +0.5 | 0 | 0 |
| Palette | teal/verde | rosso/arancio caldo | blu acciaio |

Wayfinder (indice 0) è il personaggio **preselezionato di default** all'ingresso nel Piano
0: un'assunzione dichiarata dell'implementazione — così "nessuno dei tre elementi [mondo,
pipeline, personaggio]" del Risultato del Piano 0 resta mai indefinito, anche senza una
conferma esplicita del giocatore. La scelta resta comunque modificabile fino all'attraversamento
dell'uscita, esattamente come per un personaggio scelto attivamente (coerente con la
modificabilità generale di tema e personaggio nel Piano 0, DEC-091).

## Sprite dei personaggi (DEC-049)

Nota implementativa (M6a): finché il modello immagini resta provvisorio (vedi
`../06-ai-content-generation-model.md`), lo stickman a palette già in uso per il
personaggio base (uno stesso disegno, tinto col colore proprio di ciascun personaggio della
rosa) **È** lo sprite curato placeholder di cui parla questo paragrafo — non un sostituto
temporaneo fuori standard. Il gap verso pixel art dedicata per personaggio resta esplicito e
noto, non risolto da questo default.

I 3 personaggi della rosa base (DEC-030, DEC-080) hanno sprite pixel art **curati a mano**: sono
contenuto `curato`, non generato, come il resto della rosa. Il personaggio alternativo
generato per la run ha invece uno sprite **generato dalla pipeline di generazione sprite già
esistente**, la stessa usata per i nemici (vedi
[AI Content Generation Model](../06-ai-content-generation-model.md)); questo documento non
ripete il dettaglio tecnico di quella pipeline.

Indipendentemente dall'origine dello sprite — curato per la rosa base, generato per
l'alternativa — i 6 slot visivi degli oggetti equipaggiati si sovrappongono allo stesso modo
a **tutti** i personaggi: fonte unica del dettaglio di quegli strati è
[Visual Language](../content/visual-language.md), non riformulato qui.

## Colpo firmato (DEC-068)

Il personaggio alternativo generato per la run può, **a volte**, avere un proprio tipo di
colpo generato (forma più comportamento) invece di usare il colpo standard: è una
**possibilità del generatore**, non una garanzia — non ogni personaggio generato riceve un
colpo firmato.

Il colpo firmato è parte del **budget** del personaggio generato (vedi
[glossario](../governance/glossary.md) per il concetto generale di budget): un personaggio
che riceve un colpo firmato ha statistiche più **caute** rispetto a un personaggio
alternativo equivalente senza colpo firmato, per compensare il vantaggio di un colpo
dedicato.

- Un personaggio alternativo **senza** colpo firmato usa il colpo standard del giocatore, lo
  stesso disponibile a qualunque personaggio della rosa base.
- I personaggi della rosa base (DEC-030) usano **sempre** colpi standard curati: non hanno
  mai un colpo firmato, che resta una possibilità esclusiva del personaggio alternativo
  generato per run.
- Quando presente, il colpo firmato del personaggio alternativo è generato con la stessa
  pipeline dei tipi di colpo descritta in
  [Combat and Projectiles](combat-and-projectiles.md) e può essere un comportamento Lua
  validato in sandbox, con le manopole parametriche come garanzia e fallback (DEC-037,
  rimando, non riformulato qui).

## Personaggio generato: stato dell'implementazione (M6b-1, M6b-2, M6b-3)

Nota di stato della fetta (stile delle note di gap già presenti in questo documento e nella
KB): l'implementazione M6b-1 ha coperto nome/blurb, statistiche in bande e palette del
personaggio alternativo generato per run, più la sua carta nel Piano 0 (quarto slot dinamico
accanto alla rosa base). Da M6b-2 il **trait unico come comportamento Lua (DEC-037)** è
implementato: generato e validato in sandbox nella stessa sessione modello della proposta,
attivo dalla selezione del personaggio generato. Da M6b-3 il **colpo firmato (DEC-068)** è
implementato: il personaggio generato per run è ora **completo** (statistiche + trait + colpo
firmato opzionale), chiudendo il gap dichiarato dalle fette precedenti. Un personaggio
alternativo senza colpo firmato (lo stato più comune del generatore) continua a usare sempre
il colpo standard, esattamente come prima di questa fetta e come ogni personaggio della rosa
base (DEC-068, "un personaggio alternativo senza colpo firmato non è penalizzato").

### Default proposti dall'implementazione (stile DEC-019, M6b-1)

Come i default della rosa base sopra, questi sono **valori proposti in fase di
implementazione**, non ancora numeri approvati dal design: restano da validare col playtest
(vedi Domande aperte residue e `../governance/open-questions.md` punto 6). Il centro di ogni
banda è vicino alla rosa base curata (sopra), cosí il personaggio generato resta un'alternativa
credibile alla stessa rosa, mai un caso anomalo fuori scala.

| Statistica | Banda min | Banda max |
|---|---|---|
| Danno base | 6.0 | 11.0 |
| Cadenza base (secondi) | 0.19 | 0.28 |
| Velocità colpo base | 480 | 560 |
| Velocità movimento base | 190 | 260 |
| Salute base (maxHp) | 3 | 9 |
| Fortuna base | 0 | 1.5 |

Il tetto di salute (DEC-033) non è una settima banda indipendente: si **deriva** sempre da
maxHp con la regola `hpCap = 2 × maxHp`, poi clampato alla propria banda `[6, 18]` — mai una
manopola libera che il generatore possa scegliere a parte. 18 resta ben sotto la guardia
assoluta di motore (24, indipendente dal personaggio: vedi `health-and-resources.md` per il
dettaglio del meccanismo). Il margine tra 18 e 24 non è riservato al colpo firmato (vedi
sotto: quella fetta comprime maxHp verso il basso, non lo fa mai crescere oltre la banda) —
resta un cuscinetto generico verso la guardia assoluta.

### Budget del colpo firmato: criterio canonico (DEC-078) e valore in default proposto (M6b-3)

Il **criterio** della compressione è ora canone (DEC-078): un personaggio con colpo firmato
ha damage/maxHp/luck/fortuna compressi verso il **bordo cauto** della banda e cadenza
compressa verso il **lato lento** (più lenta, mai più veloce del bordo cauto) — velocità del
colpo base e velocità di movimento **non** sono mai compresse: il colpo firmato paga il
proprio vantaggio offensivo, non la mobilità del personaggio. hpCap segue sempre `2 × maxHp`
come sopra, quindi eredita automaticamente la compressione applicata a maxHp. La compressione
si applica **solo** quando il personaggio ha un colpo firmato: senza, le bande sono quelle
intere della tabella sopra, come prima di M6b-3.

Il **valore** esatto del fattore resta invece, come i valori numerici di DEC-019, un
**default proposto in fase di implementazione**, non ancora un numero approvato dal design:
la conferma o correzione del fattore resta una domanda aperta (vedi punto 7 di
`../governance/open-questions.md`), questo è solo il numero scelto per renderla giocabile
subito e verificabile. Il fattore attuale è 0.6: damage/maxHp/luck/fortuna compressi verso
`bandMin + 0.6 × (bandMax − bandMin)` (il 60% inferiore della banda, non un dimezzamento
secco), cadenza compressa verso `bandMax − 0.6 × (bandMax − bandMin)`.

**Sostituibilità (canone, DEC-099):** il colpo firmato, quando c'è, è il colpo di
**partenza** del personaggio — attivo dalla prima stanza, non un lucchetto che si sblocca. Un
oggetto-colpo raccolto durante la run lo **sostituisce** esattamente come sostituirebbe il
colpo standard di qualunque altro personaggio (stessa regola "vince l'ultimo raccolto" del
resto del sistema dei tipi di colpo, vedi
[Combat and Projectiles](combat-and-projectiles.md)); se quell'oggetto viene concettualmente
"tolto" (ricalcolo da zero, come per ogni altra statistica), il colpo firmato torna ad essere
quello attivo — mai il colpo standard. Questa lettura, prima un default proposto in fase di
implementazione (M6b-3), è ora **sancita dal design** (DEC-099): nessun meccanismo di "colpo
protetto" che nessun oggetto possa sovrascrivere.

**Colpo che non valida dopo la generazione (canone, DEC-079):** sancito che il colpo firmato
attraversa sempre la doppia rete di bilanciamento dei tipi di colpo di run, che **normalizza
e non scarta mai** — un colpo fuori banda viene riportato in banda, **mai sostituito dal
colpo standard**. In pratica il tool scrive la proposta SOLO dopo che il colpo firmato ha già
attraversato quella stessa doppia rete, quindi il caso concreto di un colpo fuori banda è solo
un `character_proposal.json` forgiato a mano fuori dal percorso normale del generatore — quel
caso viene comunque riportato in banda alla lettura (stessa rete): il personaggio resta con il
suo colpo firmato e le statistiche caute già generate, senza alcun rebalance retroattivo delle
altre statistiche se il colpo cambia. Un personaggio generato con colpo firmato ha quindi
sempre il suo colpo firmato: il dilemma della doppia penalità contro il colpo standard
"gratis" (statistiche caute più eventuale ritorno al colpo standard) sparisce alla radice.

## Modalità competitive: proposte identiche a parità di seed (DEC-108)

Nelle istanze di Classificata **a stesso seed** e nella **Daily** (vedi
[Multiplayer and Competition](../08-multiplayer-and-competition.md) per la distinzione delle
istanze di Classificata, rimando, non riformulata qui) lo stesso seed genera **le stesse
proposte generate per tutti i partecipanti**: lo stesso personaggio alternativo generato
(stesso trait, stesse statistiche, eventuale stesso colpo firmato) e lo stesso tema
proposto. La **rosa base disponibile resta quella sbloccata dal singolo giocatore**
(DEC-100): la parità riguarda il contenuto generato dal seed, non la progressione personale
— gli sblocchi arrivano presto per costruzione, quindi la differenza tra partecipanti è
effimera. La **scelta tra le proposte resta libera**, esattamente come in singleplayer:
nessun personaggio è imposto dalla gara, il personaggio alternativo non viene mai escluso.
L'equità della gara viene dal determinismo della generazione a partire dal seed, non da un
vincolo sulla scelta del giocatore — un caso distinto dalla Classificata a **seed diversi**,
dove l'equità passa invece dai vincoli di budget della generazione (DEC-096).

Gap di implementazione esplicito: il determinismo completo delle proposte di personaggio a
partire dal seed **non è ancora garantito** dalla pipeline attuale (backlog noto: RNG di
gioco su `time(NULL)`, inferenza non deterministica). Il dettaglio della riproducibilità
resta in [Run Manifest and Reproducibility](run-manifest-and-reproducibility.md), non
riformulato qui.

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Schede personaggi della rosa base | Sempre, nel Piano 0 | Per ciascuna scheda: quando quel personaggio della rosa è già sbloccato | Selezionare uno dei personaggi della rosa | Il personaggio scelto, col suo ruolo distinto, diventa il personaggio della run | Evidenziazione della scheda selezionata, ruolo messo in risalto |
| Scheda personaggio alternativo | Quando la generazione per la run ha prodotto un'alternativa valida | Sempre, se visibile | Selezionare il personaggio alternativo | Il personaggio alternativo, con il suo trait unico, diventa il personaggio della run | Evidenziazione della scheda, trait unico messo in risalto |
| Rifiuto dell'alternativa | Quando è presente una scheda alternativa | Sempre, se visibile | Non selezionare l'alternativa (lasciare) | Un personaggio della rosa base resta quello attivo; la scheda alternativa resta comunque nel selettore e selezionabile finché non si attraversa l'uscita (DEC-097) | Nessun cambiamento visibile oltre alla non-selezione |

## Risultato

Il personaggio scelto nel Piano 0 (uno della rosa base o l'alternativo generato) è quello
con cui si gioca l'intera run, dal piano 1 fino alla fine. La scelta non cambia durante la
run.

## Feedback

- La scheda del personaggio alternativo mostra il trait unico e le statistiche in modo
  leggibile prima della scelta, non dopo.
- Le statistiche del personaggio alternativo sono presentate come valori entro una banda
  garantita, non come numeri arbitrari: il giocatore deve poter capire che non ci sono
  sorprese fuori controllo.
- Ogni scheda della rosa base comunica chiaramente il proprio ruolo (offensivo, difensivo,
  esploratore o equivalente), così la scelta tra i personaggi curati resta leggibile quanto
  quella verso l'alternativa generata.
- Il personaggio scelto resta visibile nel riepilogo del Piano 0, insieme al tema della run
  (vedi [Floor Zero](floor-zero.md)).
- La scheda del personaggio alternativo indica chiaramente se ha un colpo firmato generato,
  distinto dal colpo standard, insieme al trait unico (DEC-068).

## Interazioni

- [Player](player.md): responsabilità e statistiche base condivise da ogni personaggio,
  base o alternativo.
- [Floor Zero](floor-zero.md): la scelta del personaggio avviene lì, insieme alla scelta
  del tema della run.
- [Run Manifest and Reproducibility](run-manifest-and-reproducibility.md): il personaggio
  scelto entra nel manifest della run.
- [AI Content Generation Model](../06-ai-content-generation-model.md): la pipeline di
  generazione sprite usata per il personaggio alternativo, condivisa coi nemici (DEC-049).
- [Visual Language](../content/visual-language.md): i 6 slot visivi degli oggetti
  equipaggiati, condivisi da personaggi curati e generati (DEC-049).
- [Combat and Projectiles](combat-and-projectiles.md): il colpo firmato del personaggio
  alternativo usa la stessa pipeline di generazione dei tipi di colpo, inclusi i
  comportamenti Lua validati in sandbox (DEC-037, DEC-068).
- [Multiplayer and Competition](../08-multiplayer-and-competition.md): le istanze di
  Classificata a stesso seed e la Daily propongono lo stesso personaggio a tutti i
  partecipanti, con scelta libera (DEC-108).

## Regole per contenuti generati

- Il personaggio alternativo è generato una sola volta per run: un trait unico più
  statistiche casuali entro bande garantite (i valori esatti delle bande sono da definire,
  vedi domande aperte). Tra queste statistiche c'è anche il proprio tetto di salute base
  (DEC-033), generato entro bande min/max di default da playtest, come i valori di DEC-019:
  il personaggio generato può quindi risultare più "vetro" o più "roccia" di un personaggio
  della rosa base, ma sempre dentro limiti garantiti, mai arbitrario.
- **Trait unico come comportamento Lua (DEC-037):** il trait unico del personaggio
  alternativo generato è un comportamento scritto dall'IA e validato in sandbox, con la
  stessa pipeline usata per i comportamenti degli oggetti (vedi
  [AI Content Generation Model](../06-ai-content-generation-model.md) e
  [Generated Content Validation](generated-content-validation.md), rimando, non riformulare
  qui). Deve inoltre rispettare gli stessi vincoli di leggibilità di qualunque altro
  contenuto generato.
- **Varietà anti-fotocopia del trait (DEC-098):** il generatore evita di riproporre trait
  unici identici a quelli delle run recenti, consultando il catalogo persistente come memoria
  (vedi [Save and Meta Progression](save-and-meta-progression.md), rimando, non riformulato
  qui) — stessa filosofia già usata per evitare la fotocopia dei temi generati. La
  ripetizione occasionale a distanza resta ammessa: non è un divieto assoluto.
- Il personaggio alternativo dichiara la propria origine come `nuovo` o `variato`, secondo
  la tassonomia unica di origine del contenuto.
- I personaggi della rosa base (DEC-030) sono `curato`: non sono generati, restano fissi tra
  una run e l'altra fino a un eventuale aggiornamento curato del gioco.
- Il colpo firmato del personaggio alternativo, quando il generatore lo produce, è parte del
  suo budget e comporta statistiche più caute per il resto del personaggio (DEC-068); un
  personaggio alternativo senza colpo firmato non è penalizzato: usa semplicemente il colpo
  standard, senza compensazione né svantaggio.

## Casi limite

- La generazione del personaggio alternativo per la run non produce un risultato valido:
  nel Piano 0 compare solo la rosa dei personaggi base già sbloccati, senza errore visibile
  al giocatore.
- Il giocatore rifiuta l'alternativa (non la seleziona): la sua scheda resta comunque nel
  selettore e resta selezionabile finché non si attraversa l'uscita del Piano 0 (DEC-097) —
  non viene proposta una seconda alternativa diversa nella stessa run.
- Il trait Lua generato per il personaggio alternativo non supera la validazione in sandbox:
  si applica il fallback, vedi sotto.
- Un personaggio della rosa base non è ancora sbloccato: la sua scheda non è selezionabile
  nel Piano 0 (condizioni di sblocco ora canoniche, DEC-100: vedi "Rosa di personaggi base"
  sopra).
- Lo sprite generato del personaggio alternativo non supera la validazione: si applica il
  fallback definito in [Generated Content Validation](generated-content-validation.md), e la
  scheda del personaggio alternativo non compare (coerente con il caso limite già descritto
  sopra per la generazione dell'alternativa non disponibile).
- Il generatore non produce un colpo firmato per il personaggio alternativo di questa run:
  non è un errore, è lo stato più comune del generatore (DEC-068); il personaggio alternativo
  usa il colpo standard, con le statistiche non caute della sua banda garantita.
- Il comportamento Lua del colpo firmato non supera la validazione in sandbox: il colpo
  firmato ripiega sulla propria forma parametrica — le manopole parametriche sono la
  garanzia e il fallback di ogni comportamento Lua (DEC-037, vedi
  [Generated Content Validation](generated-content-validation.md) e
  [Combat and Projectiles](combat-and-projectiles.md), rimando, non riformulato qui) — e
  **mai** sul colpo standard: il personaggio con colpo firmato conserva sempre il suo colpo
  firmato (DEC-079), senza che il giocatore veda alcun errore.

## Fallback

Se il personaggio alternativo generato per la run non è disponibile o non è valido, si
applica la regola di fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- Non è un sistema di creazione o personalizzazione libera del personaggio.
- Non esistono potenziamenti permanenti del personaggio tra una run e l'altra: la
  meta-progressione riguarda contenuti sbloccati nei pool, non il personaggio stesso (vedi
  [Save and Meta Progression](save-and-meta-progression.md)).
- Non è un roster ampio o liberamente configurabile: resta una piccola rosa fissa e curata
  di 3 personaggi (DEC-030, DEC-080) più l'unica alternativa generata per quella run.

## Domande aperte residue

- Quali sono i valori esatti delle bande garantite per le statistiche casuali del
  personaggio alternativo, incluse le bande min/max del suo tetto di salute base (DEC-033)?
- Statistiche esatte della rosa base Wayfinder/Ashblade/Bulwark (DEC-080 fissa nomi e ruoli
  come canone, DEC-100 fissa gli sblocchi — Wayfinder subito, Ashblade alla prima run
  conclusa, Bulwark al primo boss abbattuto; resta aperta solo la statistica esatta di
  ciascuno, da playtest, vedi punto 8 di `../governance/open-questions.md`).
- Se lo sprite curato della rosa base condivide lo stesso atlas/risoluzione di riferimento
  dello sprite generato del personaggio alternativo (vedi
  [Visual Language](../content/visual-language.md), valori draft DEC-046).
- Con quale frequenza il generatore assegna un colpo firmato rispetto al colpo standard, e
  qual è il valore esatto del fattore di compressione delle bande (DEC-078 fissa il
  **criterio** della compressione come canone — bordo cauto per danno/salute/fortuna, lato
  lento per la cadenza — non il valore; il fattore attuale, 0.6, resta un default proposto da
  playtest, stile DEC-019, vedi punto 7 di `../governance/open-questions.md`). Default
  proposto dall'implementazione (M6b-3, RESTA da validare): il prompt istruisce "circa metà
  delle volte" senza imporlo (nessuna garanzia di frequenza, coerente con "a volte" di
  DEC-068).

## Scenari

**Scenario: scelta di un personaggio della rosa base**
- Given il giocatore è nel Piano 0, con la rosa dei personaggi base sbloccati visibile e
  un'alternativa generata disponibile
- When il giocatore seleziona uno dei personaggi della rosa base (es. il ruolo offensivo)
- Then quel personaggio, col suo ruolo distinto, diventa il personaggio della run e
  l'alternativa non ha alcun effetto

**Scenario: sblocco di Ashblade e Bulwark (DEC-100)**
- Given un giocatore ha solo Wayfinder disponibile nella rosa base
- When conclude la sua prima run, qualunque esito, e in una run successiva abbatte il suo
  primo boss
- Then Ashblade risulta sbloccato subito dopo la prima run conclusa e Bulwark risulta
  sbloccato subito dopo il primo boss abbattuto, entrambi selezionabili nel Piano 0 delle
  run seguenti

**Scenario: scelta del personaggio alternativo**
- Given il giocatore è nel Piano 0 e viene proposta un'alternativa generata con trait
  unico e statistiche entro banda garantita
- When il giocatore seleziona l'alternativa
- Then il personaggio alternativo, con il suo trait unico, diventa il personaggio della
  run fino alla fine

**Scenario: generazione dell'alternativa non disponibile**
- Given la generazione del personaggio alternativo per questa run non ha prodotto un
  risultato valido
- When il giocatore entra nel Piano 0
- Then compare solo la rosa dei personaggi base già sbloccati, senza alcun errore visibile

**Scenario: rifiuto dell'alternativa**
- Given è disponibile un personaggio alternativo generato per la run
- When il giocatore non la seleziona e attraversa l'uscita del Piano 0 con un personaggio
  della rosa base
- Then la run prosegue con quel personaggio della rosa base: il Piano 0 di quella run è
  concluso e l'alternativa generata non viene più riproposta

**Scenario: il rifiuto dell'alternativa è ripensabile fino all'uscita (DEC-097)**
- Given il giocatore ha inizialmente ignorato la scheda del personaggio alternativo e
  selezionato un personaggio della rosa base, ma non ha ancora attraversato l'uscita del
  Piano 0
- When, nello stesso Piano 0, seleziona la scheda del personaggio alternativo
- Then il personaggio alternativo diventa quello attivo per la run: la scheda alternativa
  resta nel selettore e selezionabile finché l'uscita non viene attraversata, coerente con
  la modificabilità generale delle scelte del Piano 0 (DEC-091)

**Scenario: il generatore evita un trait già visto di recente (DEC-098)**
- Given il catalogo persistente registra i trait unici già generati nelle run recenti
- When il generatore prepara il personaggio alternativo per una nuova run
- Then evita di riproporre un trait identico a quelli delle run recenti, pur restando
  ammessa una ripetizione occasionale a distanza (nessun divieto assoluto)

**Scenario: trait Lua generato per l'alternativa non supera la validazione**
- Given il trait unico del personaggio alternativo è stato scritto dall'IA come
  comportamento Lua
- When il comportamento non supera la validazione in sandbox
- Then si applica il fallback definito in
  [Generated Content Validation](generated-content-validation.md) e nel Piano 0 compare solo
  la rosa dei personaggi base già sbloccati

**Scenario: sprite curato per la rosa base, generato per l'alternativa**
- Given un giocatore che confronta nel Piano 0 la scheda di un personaggio della rosa base e
  la scheda del personaggio alternativo generato
- When osserva i due sprite
- Then lo sprite della rosa base è pixel art curata a mano, mentre lo sprite
  dell'alternativa è generato dalla stessa pipeline usata per i nemici, ma entrambi
  condividono gli stessi 6 slot visivi per gli oggetti equipaggiati (DEC-049)

**Scenario: personaggio alternativo con colpo firmato**
- Given il generatore produce per questa run un personaggio alternativo con un colpo firmato
  proprio
- When il giocatore consulta la sua scheda nel Piano 0
- Then la scheda mostra il colpo firmato distinto dal colpo standard, insieme a statistiche
  più caute rispetto a un personaggio alternativo equivalente senza colpo firmato (DEC-068)

**Scenario: personaggio alternativo senza colpo firmato usa il colpo standard**
- Given il generatore produce per questa run un personaggio alternativo senza colpo firmato
- When il giocatore lo seleziona e inizia la run
- Then il personaggio usa il colpo standard, lo stesso disponibile ai personaggi della rosa
  base, senza alcuna compensazione o svantaggio di statistiche (DEC-068)

**Scenario: colpo firmato fuori banda viene normalizzato, mai scartato (DEC-079)**
- Given il personaggio alternativo di questa run ha un colpo firmato i cui parametri
  risultano fuori dalla banda garantita (es. un `character_proposal.json` forgiato fuori dal
  percorso normale del generatore)
- When il gioco legge la proposta del personaggio
- Then il colpo firmato viene riportato in banda dalla stessa doppia rete di bilanciamento
  dei tipi di colpo di run, senza mai essere sostituito dal colpo standard: il personaggio
  mantiene il proprio colpo firmato e le statistiche caute già generate, senza alcun
  rebalance retroattivo delle altre statistiche

**Scenario: un oggetto-colpo raccolto sostituisce il colpo firmato (DEC-099)**
- Given il giocatore ha scelto il personaggio alternativo col suo colpo firmato, attivo fin
  dalla prima stanza come colpo di partenza
- When raccoglie durante la run un oggetto che porta il proprio tipo di colpo
- Then il colpo dell'oggetto sostituisce il colpo firmato esattamente come sostituirebbe il
  colpo standard di qualunque altro personaggio — e se quell'oggetto viene concettualmente
  tolto, torna attivo il colpo firmato, non il colpo standard

**Scenario: stesso seed, stesse proposte di personaggio in gara (DEC-108)**
- Given due giocatori partecipano alla stessa gara Classificata a stesso seed (o alla stessa
  Daily del giorno)
- When entrano nel Piano 0 delle rispettive run
- Then vedono lo stesso personaggio alternativo generato (stesso trait, stesse statistiche)
  e lo stesso tema proposto — ciascuno con la propria rosa base sbloccata (DEC-100) — e
  restano liberi di scegliere personaggi diversi tra loro
