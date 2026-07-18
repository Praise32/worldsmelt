---
id: gd-core-loop
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Ciclo principale di una run: Piano 0 (con tutorial integrato nella primissima visita, DEC-047), cinque piani, fusione esplicita e ciclo museo/catalogo/sblocchi. La vittoria al boss del piano 5 chiude la run (DEC-031); la prosecuzione in piani extra resta un'idea futura. La valuta principale si guadagna da nemici e stanze ripulite e il negozio ricompra oggetti indesiderati (DEC-048)."
---

# Core Loop

## Loop di Piano 0 (approved, DEC-004)

1. Il giocatore entra nell'hub sempre giocabile del Piano 0. Alla primissima visita, questo
   passaggio è guidato: le arene opzionali insegnano le meccaniche base (movimento, sparo,
   risorse, fusione) con cartelli e prove pratiche, senza un tutorial separato (DEC-047;
   dettaglio in [Floor Zero](systems/floor-zero.md)).
2. Consulta il museo delle creazioni migliori (contenuti "best-of" già validati da run
   passate) e può affrontare arene di sfida opzionali che li riusano.
3. Sceglie uno tra i 2–3 temi che l'IA propone per la run (DEC-005).
4. Sceglie il personaggio: quello base sempre disponibile, oppure il personaggio
   alternativo generato per questa run (DEC-014; vedi [Characters](systems/characters.md)).
5. Osserva l'indicatore di generazione mentre il piano 1 viene preparato in background.
6. Quando il piano 1 è pronto, l'uscita verso il piano 1 si apre.

## Loop principale (piani 1–5)

1. Entrare in una stanza.
2. Leggere minacce, ostacoli e opportunità.
3. Combattere o risolvere la stanza.
4. Ricevere risorse per funzione (salute, valuta principale — guadagnata sconfiggendo
   nemici e ripulendo stanze, DEC-048 — strumento di breccia, strumento di apertura,
   catalizzatore di fusione), informazioni o una scelta (vedi
   [Health and Resources](systems/health-and-resources.md)).
5. Decidere dove andare e cosa spendere, incluso rivendere al negozio oggetti o Innesti
   indesiderati a prezzo ridotto rispetto all'acquisto (DEC-048; dettaglio in
   [Rewards and Economy](systems/rewards-and-economy.md)).
6. Modificare la build: raccogliere oggetti, attivare sinergie implicite, oppure — nella
   stanza di fusione, quando si dispone del catalizzatore di fusione — consumare due
   oggetti per ottenere un oggetto di fusione generato dall'IA che eredita comportamento e
   presentazione da entrambi (DEC-012; meccanica-firma del progetto; vedi
   [Item Fusion](systems/item-fusion.md)).
7. Affrontare una sfida più complessa, mentre il tema della run evolve o degenera.
8. Sconfiggere il boss del piano.
9. Accedere al piano successivo.
10. Al boss del piano 5: la run si chiude con vittoria, valida per classifiche (DEC-006,
    aggiornata da DEC-031: la prosecuzione in piani extra non è implementata ora e resta
    un'idea futura, vedi DEC-018 nel [decision log](governance/decision-log.md)). In
    qualunque piano, salute a zero = run persa, permadeath.

## Loop di apprendimento

1. Incontrare un contenuto nuovo.
2. Osservarne segnali e comportamento.
3. Formulare una previsione.
4. Verificarla in combattimento.
5. Riutilizzare la conoscenza in contenuti futuri con tag simili.

## Loop metagioco: museo, catalogo, punti sblocco (approved, DEC-015)

1. Concludere o fallire una run.
2. Registrare nel catalogo persistente tutti i contenuti generati durante la run: alla
   vittoria come alla sconfitta, il catalogo si aggiorna comunque con le creazioni
   incontrate (DEC-041).
3. I contenuti migliori entrano nel museo del Piano 0, dove diventano visibili e riusabili
   nelle arene di sfida.
4. I punti guadagnati in singleplayer si accumulano e possono essere spesi per sbloccare
   contenuti generati nei pool delle run future. Alla sconfitta i punti maturati restano ma
   in misura ridotta rispetto alla vittoria (DEC-041); nessun oggetto sopravvive comunque
   alla run (permadeath, DEC-006). Nessun potenziamento permanente del personaggio è
   previsto: gli sblocchi ampliano la varietà, non la potenza di base.
5. Gli sblocchi sono disattivati nelle modalità competitive (vedi
   [Multiplayer and Competition](08-multiplayer-and-competition.md)).
6. Preparare una nuova run: tornare al Piano 0 e scegliere un nuovo tema e, se disponibile,
   un nuovo personaggio generato.

## Scenari

- **Dato** che il giocatore è nel Piano 0 e il piano 1 non è ancora pronto, **quando**
  osserva l'indicatore di generazione, **allora** può comunque muoversi, consultare il
  museo e scegliere tema e personaggio senza restare bloccato in attesa.
- **Dato** che il giocatore possiede il catalizzatore di fusione e due oggetti compatibili,
  **quando** entra nella stanza di fusione e conferma la fusione, **allora** i due oggetti
  vengono consumati e il giocatore riceve un oggetto nuovo con comportamento e
  presentazione visibilmente derivati da entrambi.
- **Dato** che il giocatore sconfigge il boss del piano 5, **quando** la run si chiude,
  **allora** il risultato è registrato come vittoria valida per classifiche e la run finisce
  lì, senza alcuna scelta di prosecuzione in piani extra (DEC-031).
- **Dato** che il giocatore muore (permadeath) prima del boss del piano 5, **quando** la run
  si chiude con sconfitta, **allora** i punti sblocco maturati restano ma in misura ridotta
  rispetto alla vittoria, e il catalogo si aggiorna comunque con le creazioni incontrate
  (DEC-041).
- **Dato** che una run è terminata, **quando** il giocatore torna al Piano 0, **allora** i
  contenuti generati in quella run sono già registrati nel catalogo e i migliori sono
  consultabili nel museo, senza richiedere un'azione manuale di salvataggio.
- **Dato** che il giocatore entra nel Piano 0 per la primissima volta, **quando** esplora le
  arene opzionali, **allora** trova cartelli e prove pratiche che insegnano movimento,
  sparo, risorse e fusione, senza alcun tutorial separato dal resto del gioco (DEC-047).
- **Dato** che il giocatore possiede oggetti o Innesti che non vuole più tenere, **quando**
  li porta al negozio, **allora** può rivenderli per valuta principale a un prezzo ridotto
  rispetto al valore di acquisto (DEC-048).

## Fallback

Se un contenuto generato necessario al loop (piano, oggetto di fusione, tema) non supera la
validazione, si applica la regola unica di fallback descritta in
[Generated Content Validation](systems/generated-content-validation.md); questo documento
non la ripete.

## Non-obiettivi

- Il loop non richiede che il giocatore usi la fusione per completare una run.
- Il loop non introduce potenziamenti permanenti del personaggio tra una run e l'altra.

## Domande aperte residue

- Numero e cadenza esatti dei punti sblocco per contenuto del catalogo (vedi
  [Difficulty and Progression](07-difficulty-and-progression.md), DEC-019).
- Quanti contenuti "best-of" il museo del Piano 0 può mostrare contemporaneamente.
