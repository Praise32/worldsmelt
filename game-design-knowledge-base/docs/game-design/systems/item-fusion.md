---
id: gd-system-item-fusion
status: approved
owner: design
last_reviewed: 2026-07-17
summary: "Meccanica-firma: consumare due oggetti e un catalizzatore di fusione per ottenere un oggetto nuovo generato dall'IA."
---

# Item Fusion

## Intento per il giocatore

La fusione è la meccanica-firma del gioco: trasformare deliberatamente due oggetti già
posseduti in un oggetto nuovo, mai visto prima in quella forma, che porta con sé tratti
riconoscibili di entrambi. È una decisione di build a rischio controllato, non un bonus
automatico.

## Condizioni di ingresso

- Il giocatore deve trovarsi nella stanza di fusione (vedi [Special Rooms](special-rooms.md)).
- Il giocatore deve possedere almeno due oggetti idonei alla fusione.
- Il giocatore deve possedere almeno un catalizzatore di fusione, la risorsa dedicata che
  abilita l'operazione (vedi [Health and Resources](health-and-resources.md)).

## Input/azioni

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Slot oggetto sorgente 1 | Nella stanza di fusione | Il giocatore possiede almeno un oggetto idoneo | Selezionare il primo oggetto da fondere | L'oggetto viene messo in attesa di conferma | L'oggetto selezionato si evidenzia nello slot |
| Slot oggetto sorgente 2 | Nella stanza di fusione | Il giocatore possiede almeno un secondo oggetto idoneo, diverso dal primo | Selezionare il secondo oggetto da fondere | La coppia è pronta per la fusione | Anteprima dei tratti ereditabili dai due oggetti |
| Conferma fusione | Due oggetti sorgente selezionati | Il giocatore possiede almeno un catalizzatore di fusione | Confermare l'operazione | I due oggetti sorgente e un catalizzatore vengono consumati; un nuovo oggetto viene generato e assegnato al giocatore | Sequenza di fusione visibile, presentazione del nuovo oggetto |
| Annulla selezione | Almeno un oggetto sorgente selezionato | Sempre, prima della conferma | Svuotare gli slot | Nessuna risorsa viene consumata | Gli slot tornano vuoti |

## Risultato

I due oggetti sorgente e un catalizzatore di fusione vengono consumati. Il giocatore
riceve un oggetto nuovo, generato dall'IA, che eredita comportamento e presentazione da
entrambi gli oggetti sorgente. L'oggetto risultante entra nell'inventario nello slot
corrispondente alla sua categoria.

## Feedback

- La sequenza di fusione mostra chiaramente quali due oggetti sono stati consumati.
- Il nuovo oggetto viene presentato con un momento dedicato (non un semplice popup di
  drop), coerente con il suo ruolo di meccanica-firma.
- La scheda del nuovo oggetto indica da quali due oggetti sorgente deriva, senza esporre
  dettagli tecnici della generazione.

## Interazioni

- [Special Rooms](special-rooms.md): la stanza di fusione è l'unico luogo in cui questa
  meccanica è disponibile.
- [Health and Resources](health-and-resources.md): il catalizzatore di fusione è definito
  lì come risorsa (fonti, cap, visibilità in HUD).
- [Items, Pools and Rarity](items-pools-and-rarity.md): l'oggetto generato rispetta gli
  stessi campi obbligatori di ogni altro oggetto del gioco.
- [Synergies](synergies.md): la fusione esplicita è il secondo binario delle sinergie,
  complementare a quelle implicite/automatiche descritte in quel documento.

## Regole per contenuti generati

- Ogni fusione deve cambiare sia il comportamento sia la presentazione visiva
  dell'oggetto risultante: non è ammessa una fusione che cambi solo le statistiche o solo
  l'aspetto.
- Per la presentazione visiva del nuovo oggetto si usano gli strati definiti in
  [Visual Language](../content/visual-language.md) (silhouette, materiale, colore
  funzionale, particelle, animazione, segnale d'impatto, indicatore sul personaggio).
- Il numero di strati e di effetti aggiunti dalla fusione rientra nel budget di
  leggibilità definito in [Combat and Projectiles](combat-and-projectiles.md).
- L'oggetto generato dichiara la propria origine come `nuovo` o `composto`, secondo la
  tassonomia unica di origine del contenuto.

## Priorità e conflitti

Quando i due oggetti sorgente hanno tratti che competono sulla stessa proprietà (per
esempio entrambi modificano la traiettoria del proiettile), si applica in ordine:

1. una regola di fusione esplicita già definita per quella coppia di tratti, se esiste nel
   catalogo dei tratti;
2. il tratto dichiarato dominante da uno dei due oggetti sorgente, se dichiarato;
3. una terza variante generata dall'IA che fonde i due tratti in un comportamento coerente,
   quando la generazione produce un risultato strutturalmente valido;
4. in assenza delle precedenti, vince il tratto dell'oggetto sorgente di rarità più alta;
   a parità di rarità, vince il tratto dell'oggetto selezionato per primo dal giocatore.

## Casi limite

- Il giocatore ha due oggetti idonei ma nessun catalizzatore di fusione: la conferma resta
  disabilitata e l'interfaccia lo segnala.
- I due oggetti selezionati appartengono a categorie diverse (per esempio un attivo e un
  Innesto): l'ammissibilità di fusioni tra categorie diverse non è ancora decisa (vedi
  domande aperte).
- La generazione del nuovo oggetto fallisce o non supera la validazione dopo che il
  giocatore ha confermato: si applica il fallback (vedi sotto); i due oggetti sorgente e il
  catalizzatore non devono andare persi senza che il giocatore riceva comunque un oggetto
  utilizzabile.

## Fallback

Se il risultato generato non è disponibile o non è valido, si applica la regola di
fallback unica definita in
[Generated Content Validation](generated-content-validation.md).

## Non-obiettivi

- La fusione non è un sistema di crafting con ricette fisse e prevedibili: il risultato
  esatto non è mai conoscibile in anticipo dal giocatore.
- La fusione non permette di annullare un'operazione già confermata.
- La fusione non è obbligatoria per completare una run.

## Domande aperte residue

- La fusione è ammessa tra oggetti di categorie diverse (attivo, passivo, stat-up,
  Innesto), o solo tra oggetti della stessa categoria?
- Esiste un limite al numero di fusioni per run, oltre alla disponibilità di catalizzatori?
- Il nuovo oggetto generato può a sua volta essere usato come sorgente per una fusione
  successiva?
- Come si ottiene e quale cap ha il catalizzatore di fusione (valore numerico da definire
  in [Health and Resources](health-and-resources.md))?

## Scenari

**Scenario: fusione riuscita**
- Given il giocatore è nella stanza di fusione con due oggetti idonei e un catalizzatore
  di fusione
- When il giocatore seleziona i due oggetti e conferma la fusione
- Then i due oggetti sorgente e il catalizzatore vengono consumati e il giocatore riceve
  un nuovo oggetto generato che eredita comportamento e presentazione da entrambi

**Scenario: catalizzatore mancante**
- Given il giocatore è nella stanza di fusione con due oggetti idonei ma senza
  catalizzatore di fusione
- When il giocatore prova a confermare la fusione
- Then la conferma resta disabilitata e l'interfaccia segnala l'assenza del catalizzatore

**Scenario: tratti in conflitto sulla stessa proprietà**
- Given i due oggetti sorgente modificano entrambi la stessa proprietà del proiettile
- When la fusione viene confermata
- Then si applica l'ordine di priorità definito, e il risultato dichiara quale regola ha
  determinato l'esito

**Scenario: generazione non valida dopo conferma**
- Given il giocatore ha confermato una fusione
- When il nuovo oggetto generato non supera la validazione
- Then si applica il fallback definito in Generated Content Validation e il giocatore
  riceve comunque un oggetto utilizzabile, senza perdere la fusione a vuoto
