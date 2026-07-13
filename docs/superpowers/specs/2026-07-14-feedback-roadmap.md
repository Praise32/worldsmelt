# Roadmap dopo la tua prova (feedback distribuito negli step)

Data: 2026-07-14
Stato: piano d'azione, in esecuzione

Questo documento prende le 5 impressioni della tua prova e le distribuisce negli step di
sviluppo, con l'ordine che hai chiesto. Il cambio d'ordine più importante: **il
bilanciamento degli oggetti va completato PRIMA delle sinergie** (tua indicazione).

## I 5 punti e dove finiscono

### 1. GUI: la vista centrale mostra roba della GUI laterale → **STEP A (subito)**
La vista di gioco centrale ha un HUD soprapposto (`DrawHud` dentro il canvas) che ripete
quello che c'è già nei pannelli laterali: nome, FPS, «Piano X/Y», HP, monete, bombe,
chiavi, tema, e una minimappa. È una duplicazione. Si toglie dal centro (il gioco resta
pulito) e si tiene l'informazione nei pannelli laterali, dove appartiene. Fix contenuto,
non serve ancora raygui.

### 2. Tempi di caricamento troppo lunghi (5-6 min) → **STEP B (subito)**
Diagnosi dai log: gli script Lua sono generati 20 volte, ~11s l'uno = ~3,7 min. La causa
non è la scrittura dello script (corta) ma il **riprocessamento del prompt condiviso**
(cheat-sheet + esempi, ~3700 token) a ogni oggetto. Due leve:
- **B1 — Cache del prefisso condiviso (grosso guadagno, contenuto).** Il prompt di sistema
  + cheat-sheet si processa UNA volta e si riusa la sua KV-cache per tutti e 20 gli
  oggetti; ogni oggetto elabora solo il suo pezzetto specifico. Stima: ~2 minuti risparmiati.
- **B2 — Generazione pigra dei piani (dopo).** Genero solo il piano 1 prima di giocare, i
  piani 2-5 in sottofondo mentre esplori. Riduce ancora l'attesa iniziale. È la «generazione
  in background» già prevista nella roadmap. Più grande, viene dopo B1.

### 3. Bilanciamento oggetti alla Isaac + effetti che cambiano il tipo di attacco → **STEP C (prima delle sinergie)**
Gli oggetti hanno già rarità e pool. Ora vanno bilanciati sul modello di Isaac:
- **Modificatori classici**: danno su, vita su, cadenza, velocità — allineati alla curva di
  Isaac (formule in `docs/references/formule-statistiche.md`).
- **Effetti unici che cambiano il TIPO di attacco**: invece delle solite sfere, sparare
  chiodi, raggi laser, elettricità, ecc. Serve introdurre un concetto di **tipo di colpo**
  (`ShotType`) con aspetto E comportamento diversi, che gli oggetti attivi possono
  cambiare. È il pezzo più sostanzioso: allarga l'API Lua degli oggetti e il sistema dei
  colpi.
Questo va **completato prima** di passare alle sinergie.

### 4. Sinergie: codice + grafica → **STEP D (dopo il bilanciamento)**
L'approccio a codice è già deciso (implicite, alla Isaac; design in
`docs/references/design-sinergie.md`). In più, come hai chiesto, va gestita anche la
**sinergia visiva**: quando due oggetti si combinano, il colpo/effetto deve *vedersi*
diverso (colore, forma, particelle). Con gli effetti «tipo di attacco» dello step C già in
piedi, la sinergia visiva diventa naturale: due tipi che si combinano danno un colpo dal
look combinato.

### 5. Sprite in 2.5D (non solo 2D) → **STEP E (valutato, poi fatto)**
Valutazione (come hai chiesto di fare con attenzione):
- Il 2.5D vero stile Isaac si ottiene quasi tutto **col rendering**, non con più
  generazione: ombre proiettate (ellisse sotto ogni entità), ordinamento per profondità
  (chi sta più in basso è disegnato dopo), pavimento in prospettiva, muri con spessore,
  vignettatura. Tutto questo è già descritto nei tuoi `docs/APPUNTI.md` (sez. 7) e **non
  costa tempo di generazione**.
- Gli sprite restano 128×128, ma il **prompt** chiede una vista a ¾ (top-down leggibile con
  un po' di volume), non un profilo piatto: cambia la resa senza aggiungere secondi.
- Quindi il costo è quasi tutto lavoro di rendering, poco o nullo sul tempo di generazione.
  È un buon affare. Diventa una fase a sé, fatta dopo il bilanciamento e le sinergie (o in
  parallelo sul lato rendering, che non tocca la generazione).

## L'ordine di esecuzione

1. **A — Fix GUI** (vista centrale pulita). *In corso.*
2. **B1 — Cache del prefisso** per la generazione Lua (taglia i tempi). *In corso.*
3. **C — Bilanciamento oggetti + tipi di attacco** (chiodi/laser/elettricità). *Prossimo grande passo, prima delle sinergie.*
4. **D — Sinergie** (codice + visiva).
5. **E — Sprite 2.5D** (rendering: ombre, profondità, prospettiva; prompt a ¾).
6. **B2 — Generazione pigra dei piani** (in background), quando serve limare ancora l'attesa.
7. Poi le fasi già previste: 3b nemici/boss, 3c stanze, raygui (GUI completa), benchmark al primo avvio.

## Note

- Il bilanciamento (C) si appoggia a `docs/references/formule-statistiche.md` e
  `meccaniche-callback-e-cache.md`: reimplementiamo le curve di Isaac come fatti, con nomi e
  numeri nostri.
- La GUI completa con raygui (fase 4) resta separata dal fix rapido A: A pulisce la
  duplicazione ora, raygui rifà i pannelli meglio più avanti.
