---
id: gd-system-characters
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Piccola rosa di 2-3 personaggi base fissi con ruoli distinti, più un personaggio alternativo generato per ogni run che si aggiunge alla rosa nella scelta del Piano 0 (DEC-030); il trait unico del personaggio generato è un comportamento Lua validato in sandbox (DEC-037). Sprite: curati a mano per la rosa base, generati dalla pipeline sprite esistente (come i nemici) per il personaggio alternativo (DEC-049). Il personaggio alternativo può avere, a volte, un colpo firmato generato: parte del suo budget, con statistiche più caute in cambio (DEC-068); i personaggi base usano sempre colpi standard curati."
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
  salvi gli sblocchi previsti da DEC-030, vedi sotto).
- Il personaggio alternativo generato per la run si **aggiunge** alla rosa base nella scelta
  del Piano 0: esiste solo se la generazione per quella run lo ha prodotto e validato; in
  caso contrario non compare come opzione (vedi "Fallback").

## Rosa di personaggi base (DEC-030)

I personaggi base non sono un singolo personaggio ma una **piccola rosa fissa e curata di
2-3 personaggi**, ciascuno con un ruolo distinto (indicativamente: un ruolo offensivo, uno
difensivo, uno da esploratore). Nomi e dettagli esatti dei personaggi della rosa restano da
definire (vedi Domande aperte residue). I personaggi della rosa sono **sbloccabili presto**:
non tutti devono essere disponibili fin dal primo avvio (dettagli dello sblocco non
definiti, vedi Domande aperte residue).

Il ruolo di ciascun personaggio della rosa base si riflette anche nel proprio **tetto di
salute base** (DEC-033): un ruolo difensivo può avere un tetto alto ("personaggio-roccia"),
un ruolo più aggressivo o mobile un tetto più basso ("personaggio-vetro"), come parte
curata delle sue statistiche — non un valore unico condiviso da tutta la rosa. Il dettaglio
del meccanismo del tetto vive in
[Health and Resources](health-and-resources.md) (rimando, non riformulato qui).

Il personaggio alternativo generato per run (vedi sotto) non sostituisce la rosa base: la
scelta nel Piano 0 avviene tra i personaggi della rosa (quelli già sbloccati) più
l'eventuale alternativa generata per quella run.

### Default proposti dall'implementazione (stile DEC-019, M6a)

Come i valori numerici di DEC-019 (pesi rarità, bande di potenza), questi sono **default
proposti in fase di implementazione** per rendere la rosa giocabile subito, non ancora
numeri approvati dal design: restano da validare col playtest (vedi Domande aperte residue
e `../governance/open-questions.md` punto 7).

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
0: un'assunzione dichiarata dell'implementazione, legata alla open question residua sulla
definitività della scelta (vedi sotto) — così "nessuno dei tre elementi [mondo, pipeline,
personaggio]" del Risultato del Piano 0 resta mai indefinito, anche senza una conferma
esplicita del giocatore. La scelta resta comunque modificabile fino all'attraversamento
dell'uscita, esattamente come per un personaggio scelto attivamente.

Per ora l'**intera rosa base è disponibile da subito**, senza alcuna condizione: gli
"sbloccabili presto" di DEC-030 restano un principio approvato, ma le condizioni di sblocco
sono ancora una open question (vedi sopra e punto 7 di `open-questions.md`) — finché
restano indefinite, non c'è nulla da bloccare per davvero, e bloccare una scheda senza una
regola definita sarebbe un comportamento inventato in silenzio (vietato da `AGENTS.md`
della KB).

## Sprite dei personaggi (DEC-049)

Nota implementativa (M6a): finché il modello immagini resta provvisorio (vedi
`../06-ai-content-generation-model.md`), lo stickman a palette già in uso per il
personaggio base (uno stesso disegno, tinto col colore proprio di ciascun personaggio della
rosa) **È** lo sprite curato placeholder di cui parla questo paragrafo — non un sostituto
temporaneo fuori standard. Il gap verso pixel art dedicata per personaggio resta esplicito e
noto, non risolto da questo default.

I 2-3 personaggi della rosa base (DEC-030) hanno sprite pixel art **curati a mano**: sono
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

## Personaggio generato: stato dell'implementazione (M6b-1, M6b-2)

Nota di stato della fetta (stile delle note di gap già presenti in questo documento e nella
KB): l'implementazione M6b-1 ha coperto nome/blurb, statistiche in bande e palette del
personaggio alternativo generato per run, più la sua carta nel Piano 0 (quarto slot dinamico
accanto alla rosa base). Da M6b-2 il **trait unico come comportamento Lua (DEC-037)** è
implementato: generato e validato in sandbox nella stessa sessione modello della proposta,
attivo dalla selezione del personaggio generato. Resta il **colpo firmato (DEC-068, M6b-3)**
come gap di implementazione esplicito: fino ad allora il personaggio generato ha statistiche,
palette e trait propri ma usa sempre il colpo standard, esattamente come un personaggio della
rosa base senza colpo firmato
(DEC-068, "un personaggio alternativo senza colpo firmato non è penalizzato").

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
dettaglio del meccanismo). Il margine tra 18 e 24 è dichiarato: serve al colpo firmato
(DEC-068, M6b-3), la cui quantificazione esatta delle "statistiche più caute" resta una domanda
aperta (`../governance/open-questions.md`, sezione "Personaggio generato per-run").

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Schede personaggi della rosa base | Sempre, nel Piano 0 | Per ciascuna scheda: quando quel personaggio della rosa è già sbloccato | Selezionare uno dei personaggi della rosa | Il personaggio scelto, col suo ruolo distinto, diventa il personaggio della run | Evidenziazione della scheda selezionata, ruolo messo in risalto |
| Scheda personaggio alternativo | Quando la generazione per la run ha prodotto un'alternativa valida | Sempre, se visibile | Selezionare il personaggio alternativo | Il personaggio alternativo, con il suo trait unico, diventa il personaggio della run | Evidenziazione della scheda, trait unico messo in risalto |
| Rifiuto dell'alternativa | Quando è presente una scheda alternativa | Sempre, se visibile | Non selezionare l'alternativa (lasciare) | Un personaggio della rosa base resta quello attivo | Nessun cambiamento visibile oltre alla non-selezione |

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
- Il giocatore rifiuta l'alternativa: non viene proposta una seconda alternativa nella
  stessa run (vedi domande aperte per il caso di rigenerazione).
- Il trait Lua generato per il personaggio alternativo non supera la validazione in sandbox:
  si applica il fallback, vedi sotto.
- Un personaggio della rosa base non è ancora sbloccato: la sua scheda non è selezionabile
  nel Piano 0 (dettagli dello sblocco da definire, vedi domande aperte).
- Lo sprite generato del personaggio alternativo non supera la validazione: si applica il
  fallback definito in [Generated Content Validation](generated-content-validation.md), e la
  scheda del personaggio alternativo non compare (coerente con il caso limite già descritto
  sopra per la generazione dell'alternativa non disponibile).
- Il generatore non produce un colpo firmato per il personaggio alternativo di questa run:
  non è un errore, è lo stato più comune del generatore (DEC-068); il personaggio alternativo
  usa il colpo standard, con le statistiche non caute della sua banda garantita.
- Il colpo firmato generato non supera la validazione in sandbox: si applica il fallback
  verso il colpo standard curato equivalente (vedi
  [Generated Content Validation](generated-content-validation.md) e
  [Combat and Projectiles](combat-and-projectiles.md), rimando, non riformulato qui), senza
  che il giocatore veda alcun errore.

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
  di 2-3 personaggi (DEC-030) più l'unica alternativa generata per quella run.

## Domande aperte residue

- Quali sono i valori esatti delle bande garantite per le statistiche casuali del
  personaggio alternativo, incluse le bande min/max del suo tetto di salute base (DEC-033)?
- Il giocatore può rifiutare l'alternativa e poi tornare a valutarla più tardi nello stesso
  Piano 0, o il rifiuto è definitivo per quella run?
- Il trait unico del personaggio alternativo può ripetersi tra run diverse, o è garantita
  varietà rispetto alle run precedenti (relazione con il catalogo di
  [Save and Meta Progression](save-and-meta-progression.md))?
- Come cambia, se cambia, la scelta del personaggio nelle modalità competitive asincrone
  (vedi vincoli generali in [Multiplayer and Competition](../08-multiplayer-and-competition.md))?
- Composizione esatta della rosa dei personaggi base (DEC-030): nomi, ruoli precisi oltre
  alle indicazioni offensivo/difensivo/esploratore, e condizioni esatte di sblocco di
  ciascuno (vedi `../governance/open-questions.md`).
- Se lo sprite curato della rosa base condivide lo stesso atlas/risoluzione di riferimento
  dello sprite generato del personaggio alternativo (vedi
  [Visual Language](../content/visual-language.md), valori draft DEC-046).
- Con quale frequenza il generatore assegna un colpo firmato rispetto al colpo standard, e di
  quanto sono esattamente più caute le statistiche compensative del personaggio che lo riceve
  (DEC-068 fissa solo il principio; valori da playtest, stile DEC-019).

## Scenari

**Scenario: scelta di un personaggio della rosa base**
- Given il giocatore è nel Piano 0, con la rosa dei personaggi base sbloccati visibile e
  un'alternativa generata disponibile
- When il giocatore seleziona uno dei personaggi della rosa base (es. il ruolo offensivo)
- Then quel personaggio, col suo ruolo distinto, diventa il personaggio della run e
  l'alternativa non ha alcun effetto

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
- When il giocatore lo rifiuta e conferma un personaggio della rosa base
- Then la run prosegue con quel personaggio della rosa base e l'alternativa generata non
  viene più riproposta in quella run

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
