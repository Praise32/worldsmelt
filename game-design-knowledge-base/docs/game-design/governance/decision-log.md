# Decision Log

Usare una voce per ogni decisione che cambia il comportamento del gioco.

## Template

### DEC-000 — Titolo

- **Data:** YYYY-MM-DD
- **Stato:** proposed | approved | superseded
- **Contesto:**
- **Decisione:**
- **Alternative considerate:**
- **Conseguenze:**
- **Documenti aggiornati:**

---

### DEC-001 — Run standard iniziale di cinque piani

- **Data:** 2026-07-17
- **Stato:** proposed
- **Contesto:** La visione iniziale descrive una run base articolata in cinque piani.
- **Decisione:** Usare cinque piani come struttura di design iniziale.
- **Alternative considerate:** Durata dinamica; più percorsi; run senza numero fisso.
- **Conseguenze:** Tutte le curve di contenuto e difficoltà iniziali devono riferirsi a cinque piani.
- **Documenti aggiornati:** `04-run-structure.md`

### DEC-002 — Primo piano sempre disponibile

- **Data:** 2026-07-17
- **Stato:** proposed
- **Contesto:** Il gioco deve iniziare anche mentre l'IA prepara contenuti futuri.
- **Decisione:** Il primo piano usa contenuti curati o già validati e deve essere giocabile senza attendere la generazione degli altri piani.
- **Conseguenze:** Serve un pool di fallback sufficiente e una transizione sicura.
- **Documenti aggiornati:** `04-run-structure.md`, `systems/rooms-and-floor-generation.md`
