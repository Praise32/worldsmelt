# Design: oggetti semplici, sinergie, stat-up, personaggio a strati

Data: 2026-07-13
Fase: 3 (redirezione della roadmap dopo la tua descrizione della visione)
Stato: proposta — da rivedere e correggere quando torni

Questo documento nasce dalla tua descrizione a voce e dai tuoi `APPUNTI.md`
(sezioni 4 e 6: "la tela vuota", i layer, le sinergie visive e logiche). Serve a
mettere per iscritto la direzione prima di scriverci troppo codice sopra.

## 1. La filosofia degli oggetti (il cuore della tua visione)

**Un oggetto singolo fa una cosa sola, semplice e chiara.** Rimbalzo dei proiettili.
Un laser. Evoca una lingua che colpisce. Non oggetti complicati: oggetti *leggibili*.

**La profondita' del gioco nasce dalle SINERGIE, non dal singolo oggetto.** Man mano
che raccogli il terzo, il quarto, il quinto oggetto, le combinazioni fra loro creano
qualcosa di nuovo — a livello di comportamento e a livello visivo. Il gioco vero e'
la costruzione di una "build".

**Due famiglie di oggetti, distinte:**

- **Oggetti attivi** (stanze tesoro, negozio): modificano *come spari o come ti muovi*.
  Semplici e unici. Sono i mattoni delle sinergie.
- **Oggetti stat-up** (ricompensa del boss): puri aumenti di statistiche — piu' vita,
  piu' velocita', piu' danno, piu' cadenza. Nessun comportamento nuovo, solo numeri.
  Devono essere **equilibrati**: un budget di potenza per oggetto, non bonus arbitrari.

## 2. Come si realizza, sul motore che abbiamo gia'

Buona notizia: l'infrastruttura della fase 3a serve quasi tutta.

- **Oggetti attivi** → una callback Lua semplice (`on_fire`/`on_hit`/`on_tick`), UN solo
  effetto per oggetto. Il sandbox e l'API a handle sono gia' pronti e blindati.
- **Oggetti stat-up** → la callback `on_evaluate` col sistema a cache che ho gia'
  costruito: si riparte dalle statistiche base e si riapplicano tutti i modificatori.
  E' idempotente, quindi un oggetto stat-up non puo' accumulare danno all'infinito, e
  l'equilibrio si impone con un **clamp per campo** (il C limita ogni statistica a un
  intervallo sano dopo ogni oggetto).
- **Sinergie** → una nuova callback, `on_synergy(inventory)`, che scatta quando il tuo
  inventario contiene una combinazione. E' il seme del sistema di merge (sezione 4).

**Vincolo di generazione (nuovo).** Finora l'LLM poteva scrivere Lua abbastanza libero.
D'ora in poi il prompt gli chiede **un solo effetto semplice per oggetto**, scelto da una
piccola tavolozza di archetipi (rimbalzo, perforazione, laser/raggio, evocazione,
orbitale, rallentamento…), leggermente parametrizzato. Piu' semplice = piu' bilanciabile
= piu' affidabile con un 7B. La bravura del modello si sposta sulle **sinergie**, non sul
singolo oggetto.

## 3. Il personaggio a strati (la "tela vuota")

**Decisione (tua delega esplicita: "scegli tu il modo ottimale").**

Il personaggio base e' **minimale e FISSO**, non generato. Uno stickman pulito con punti
di aggancio noti: testa, occhi, mani, schiena, corpo, aura — gli slot che il gioco ha
gia' (`ItemSlot` in `game_types.h`). Perche' fisso e non generato:

- **Affidabilita' degli agganci.** Se il personaggio base cambia a ogni run, non so piu'
  dove attaccare il cappello o gli occhiali. Con una base fissa, ogni layer ha una
  posizione garantita.
- **Massima semplicita'.** E' esattamente cio' che chiedi: il personaggio non deve
  rubare la scena agli oggetti che ci metti sopra.
- **Zero costo di generazione** e nessun rischio che SD sbagli il personaggio.

**Gli oggetti si vedono sopra il personaggio, a strati (layer).** Ogni oggetto ha uno
slot e un colore; viene disegnato come un layer sopra la base, allo slot giusto, con lo
Z-index giusto. Il gioco *gia' fa questo* con forme geometriche (`DrawEquipment` in
`game_renderer.c`): cappelli impilati, occhiali, oggetti in mano, mantelli, aure. Lo
rendiamo l'architettura ufficiale e la puliamo.

**La sorgente visiva di un layer e' pluggabile**, e questa e' la chiave per far evolvere
l'architettura senza rifarla:

- **Adesso:** forma geometrica colorata (come oggi). Zero costo, sempre disponibile.
- **Dopo:** uno sprite generato da SD per quell'oggetto (128x128, sfondo trasparente,
  disegnato per il suo slot). Si aggancia allo stesso punto della forma geometrica.

Cosi' "gli oggetti si fondono sempre con gli sprite" (come dici tu) diventa vero appena
gli sprite ci sono, senza dover cambiare il sistema dei layer.

## 4. Il merge / le sinergie (la parte da decidere con te)

Questa e' la parte su cui ho **domande vere** (in fondo). Metto qui la mia proposta, ma
la decisione e' tua.

Tre modi possibili di intendere "facendo il merge dei due oggetti si creano nuovi oggetti":

- **A — Sinergia implicita (stile Isaac).** Gli oggetti restano separati nell'inventario;
  quando ne tieni una coppia compatibile, il gioco *aggiunge* un effetto combinato (e un
  tocco visivo). Non "fondi" due oggetti in uno.
- **B — Fusione esplicita.** Combini attivamente due oggetti in un terzo, nuovo (nuovo
  sprite, nuovo comportamento), consumando i due.
- **C — Arma che evolve.** Tutti gli oggetti che prendi confluiscono in un'unica arma che
  cresce, visivamente e nel comportamento.

**La mia proposta per iniziare: A (implicita), con la fusione VISIVA fatta a strati.**
Motivo: si sposa con l'architettura a layer (i due sprite degli oggetti si sovrappongono,
e per i conflitti di slot si puo' generare uno sprite ibrido come dici in APPUNTI sez. 6),
e con il vincolo "l'LLM lavora prima della run, non durante". Le sinergie fra i 15 oggetti
noti a inizio run si possono pre-calcolare: 15 oggetti danno un numero gestibile di coppie,
e la *logica* di sinergia (una callback Lua) e' economica anche a decine di combinazioni.
La fusione esplicita (B) e l'arma che evolve (C) sono piu' ambiziose e le terrei per dopo.

**Perche' non generare gli sprite fusi durante il gioco:** l'LLM/SD occupano la VRAM e i
due modelli non convivono; farli girare mentre giochi romperebbe i 60 FPS. Quindi la
fusione visiva a runtime e' **composizione di sprite gia' pronti** (strati sovrapposti),
non generazione nuova. Solo gli ibridi per i conflitti di slot, se li vuoi, si
pre-generano a inizio run.

## 5. Cosa costruisco ORA (non dipende dalle domande aperte)

1. **Tassonomia degli oggetti**: un campo `kind` (attivo | stat-up). Le stanze boss
   lasciano cadere oggetti **stat-up**; tesoro e negozio danno oggetti **attivi**.
2. **Oggetti stat-up** via `on_evaluate` + clamp di equilibrio per statistica.
3. **Vincolo di semplicita'** nella generazione: un effetto per oggetto attivo, da una
   tavolozza di archetipi, nel prompt di melting-gen.
4. **Personaggio base minimale + layer degli oggetti visibili**, con la sorgente del layer
   pluggabile (geometria ora, sprite dopo).

## 6. Cosa NON costruisco finche' non mi rispondi

- Il meccanismo di merge (A/B/C della sezione 4).
- La generazione degli sprite per-oggetto e degli ibridi di conflitto.
- Il bilanciamento fine (serve giocarci; adesso metto solo i budget di potenza).

## 7. Le domande per te (rispondi quando torni, io intanto vado avanti sul resto)

1. **Il merge**: quale dei tre (A implicita / B fusione esplicita / C arma che evolve)?
   La mia proposta e' A per iniziare.
2. **La visione della fusione**: strati di sprite gia' pronti sovrapposti (economico,
   istantaneo) va bene per iniziare, o vuoi da subito sprite ibridi generati per le coppie?
3. **Il personaggio base**: confermi fisso e minimale (la mia scelta, per agganci
   affidabili), o lo vuoi generato da SD ma semplice?
4. **Gli archetipi di effetto**: ti va che gli oggetti attivi peschino da una tavolozza
   curata di effetti semplici (piu' bilanciabile), o preferisci che l'LLM li inventi piu'
   liberamente (piu' vari, meno prevedibili)?
