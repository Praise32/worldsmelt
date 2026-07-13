# Design: pool degli oggetti e sistema di rarita'

Data: 2026-07-13
Fase: 3 (dopo la visione oggetti; decisioni prese dall'autore)
Stato: proposta — da rivedere quando torni

Le tue decisioni (registrate):
- **Sinergie**: implicite, alla Isaac (gli oggetti restano separati; tenere una coppia
  compatibile aggiunge un effetto). L'impianto di questa fase la prepara; la
  implementazione delle sinergie e' il passo subito dopo.
- **Rarita'**: 4 livelli; la rarita' determina **sia la potenza sia la frequenza**.
- **Pool**: il **luogo** dove trovi l'oggetto ne determina tipo e rarita'.
- **Sprite oggetti**: geometria per ora (gli strati colorati di adesso).

## 1. I quattro livelli di rarita'

| Rarita' | Colore (classico) | Cosa significa |
|---|---|---|
| Comune | bianco/grigio | Effetto piccolo. Si trova spesso. |
| Non-comune | verde | Effetto discreto. |
| Raro | blu | Effetto forte. Si trova di rado. |
| Leggendario | arancione/oro | Effetto molto forte. Raro. |

La rarita' e' scritta su ogni oggetto (`rarity`), gira nel manifest (round-trip, con
ripiego a "comune" per i manifest vecchi), si vede col colore del bordo e col nome nel
pannello, e **guida due cose**: quanto e' potente l'oggetto e quanto spesso appare.

## 2. Rarita' = potenza (il budget bilanciato)

La rarita' scala la **grandezza** dell'effetto, non la sua complessita': un oggetto
resta semplice (la tua richiesta), diventa solo piu' forte. Cosi' il bilanciamento
resta leggibile.

**Oggetti stat-up (ricompensa boss).** Il tetto per-oggetto (gia' esistente, +25% della
base) diventa **scalato per rarita'**:

| Rarita' | Tetto per-oggetto sulla statistica |
|---|---|
| Comune | ±15% della base |
| Non-comune | ±25% |
| Raro | ±40% |
| Leggendario | ±60% |

Il tetto **globale** su ogni statistica resta invariato: anche con cinque leggendari, le
statistiche del giocatore non escono dalla fascia giocabile. Un leggendario e' piu'
forte, mai rotto. Il ripiego "mai un buco" (bump C fisso quando lo script fallisce)
rispetta la rarita' dell'oggetto.

**Oggetti attivi (comportamenti).** La rarita' entra nel prompt come intensita': un
comportamento comune ha numeri piccoli (pochi colpi, poco danno extra), un leggendario
ha numeri grossi. Un solo effetto per oggetto in ogni caso. I clamp del sandbox e
dell'API restano quelli di sempre: la rarita' sposta i default, non alza i tetti di
sicurezza.

## 3. Rarita' = frequenza (le tabelle di drop)

Ogni pool tira la rarita' da una tabella di pesi. Valori iniziali (da bilanciare
giocando):

| | Comune | Non-comune | Raro | Leggendario |
|---|---|---|---|---|
| Tesoro / Negozio | 55% | 30% | 12% | 3% |
| Boss | — | — | 70% | 30% |

Il boss da' **sempre** roba buona (raro o leggendario): la sua ricompensa non delude mai.

## 4. I pool: il luogo determina tipo e rarita'

| Pool | Dove | Tipo | Rarita' | Costo |
|---|---|---|---|---|
| **Tesoro** | stanza tesoro | oggetto **attivo** | mista (tabella sopra) | una chiave |
| **Negozio** | stanza negozio | oggetto **attivo** | mista | **monete, in base alla rarita'** |
| **Boss** | ricompensa boss | oggetto **stat-up** | alta (raro/legg.) | gratis |

Il costo del negozio scala con la rarita' (un leggendario costa piu' monete di un
comune): il negozio diventa una scelta — spendo tutto per l'oggetto forte, o prendo due
cose economiche?

Questo mappa quasi 1:1 sulla struttura attuale (ogni piano genera 3 oggetti attivi +
1 stat-up del boss). Aggiungiamo: la rarita' su ogni oggetto, il tipo/rarita' legati al
pool, e il costo del negozio scalato.

## 5. Espandibilita' (il tuo "si possono espandere con lo sviluppo")

Siccome gli oggetti li **genera l'LLM a ogni run**, un "pool" non e' una lista fissa di
oggetti: e' l'insieme delle **regole e degli archetipi** da cui la generazione pesca. Per
farlo crescere nel tempo, tengo tre cose in punti **dichiarativi e modificabili**, non
sparse nel codice:

1. **La tavolozza degli archetipi di comportamento** (rimbalzo, inseguimento, laser,
   evocazione, rallentamento…) — un file/tavola commentata. Aggiungere un archetipo =
   aggiungere una riga + il suo esempio nel prompt.
2. **Le tabelle di pesi della rarita' per pool** (sezione 3) — una tavola in C ben
   marcata, un numero per cella.
3. **I tetti di potenza per rarita'** (sezione 2) — una tavola in C.

Cosi' quando aggiungi un tipo di stanza, un archetipo o vuoi ribilanciare, tocchi una
tabella, non il motore.

## 6. Cosa vede il giocatore

- Il pickup dell'oggetto ha un **bordo del colore della rarita'**.
- Il pannello "anteprima piano" e "oggetti presi" mostra rarita' (nome + colore).
- Nel negozio, il costo in monete e' visibile e scala con la rarita'.

## 7. Le sinergie (deciso: implicite — impianto ora, implementazione dopo)

Hai scelto le sinergie implicite alla Isaac. Questa fase **non** le implementa ancora, ma
prepara il terreno: gli oggetti ora hanno tipo, rarita' e archetipo dichiarati, che sono
esattamente le informazioni su cui una sinergia decide ("se hai un oggetto-rimbalzo E un
oggetto-fuoco, i colpi rimbalzati bruciano"). L'implementazione delle sinergie
(callback `on_synergy`, o combinazioni pre-calcolate a inizio run) e' il ciclo successivo.

## 8. Cosa costruisco ORA

1. `Rarity` (4 livelli) e `pool` su ogni oggetto: data model, manifest (round-trip +
   ripiego), generazione.
2. Tabelle dichiarative: pesi di rarita' per pool, tetti di potenza per rarita', tavolozza
   archetipi. In punti marcati e modificabili.
3. Generazione: melting-gen tira la rarita' per pool, la scala nel prompt (intensita') e
   nei clamp C (bilanciamento).
4. Distribuzione nel gioco: tesoro/negozio danno attivi, boss da' stat-up alto; costo
   negozio scalato per rarita'.
5. Visivo: bordo colorato per rarita' sul pickup e nel pannello.
6. Test: round-trip di rarita'/pool; il boss da' sempre raro+; i tetti scalano davvero per
   rarita' (un leggendario spinge di piu' di un comune ma resta nella fascia); il costo
   del negozio scala.

## 9. Cosa NON costruisco ora

Le sinergie vere (prossimo ciclo, ora che il meccanismo e' deciso), gli sprite per-oggetto
(hai scelto geometria), i pool per nemici/stanze (fase 3b/3c).

## 10. Criteri di successo

1. Ogni oggetto ha una rarita' e un pool, che sopravvivono a un giro nel manifest.
2. Il boss da' sempre un oggetto raro o leggendario; tesoro e negozio danno la mista.
3. Un oggetto leggendario e' misurabilmente piu' forte di uno comune dello stesso tipo,
   ma nessuna combinazione di oggetti rende il giocatore ingiocabile.
4. Il costo del negozio cresce con la rarita'.
5. Il giocatore vede la rarita' a colpo d'occhio (colore del bordo).
6. Aggiungere un archetipo o ribilanciare una rarita' = modificare una tabella.
