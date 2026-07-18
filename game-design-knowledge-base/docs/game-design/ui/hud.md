---
id: gd-ui-hud
status: approved
owner: design
last_reviewed: 2026-07-18
summary: "Salute stratificata, risorse per funzione, slot attivo e Innesto. Stile pixel art come tutta la UI (DEC-046, fonte unica in content/visual-language.md)."
---

# HUD

## Intento

Mostrare durante `Gameplay` solo le informazioni necessarie a decisioni immediate di
sopravvivenza e di gestione delle risorse.

## Condizioni di ingresso

Sempre visibile durante `Gameplay`; nascosto o attenuato durante `PauseMenu` e `BuildScreen`.

## Elementi interattivi

| Elemento | Visibile quando | Abilitato quando | Azione | Risultato | Feedback |
|---|---|---|---|---|---|
| Salute base | Sempre | — (sola lettura) | Nessuna | — | Rappresentazione distinta dalla salute temporanea |
| Salute temporanea/protettiva | Il giocatore ne possiede | — (sola lettura) | Nessuna | — | Livello visivamente separato dalla salute base; si consuma per prima (DEC-008) |
| Valuta principale | Sempre | — (sola lettura) | Nessuna | — | Contatore numerico |
| Strumento di breccia | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Strumento di apertura | Sempre | Disponibile se quantità > 0 | Uso dedicato fuori HUD | — | Contatore numerico |
| Catalizzatore di fusione | Sempre | Disponibile se quantità > 0 | Nessuna diretta (si spende in stanza di fusione) | — | Contatore numerico, evidenziato quando sufficiente per una fusione |
| Slot attivo | Sempre | Un oggetto attivo è equipaggiato e carico | Attiva l'oggetto attivo | Effetto dell'oggetto attivo | Indicatore di carica/cooldown |
| Slot Innesto | Sempre | Un Innesto è equipaggiato | Passiva, nessuna azione diretta dall'HUD | — | Icona dell'Innesto attivo |
| Piano e stanza | Sempre | — (sola lettura) | Nessuna | — | Indicatore di progressione |
| Stato competitivo essenziale | Modalità competitiva attiva | — (sola lettura) | Nessuna | — | Indicatore minimo (es. tempo trascorso) |

## Principio

L'HUD mostra informazioni necessarie a decisioni immediate. Dettagli complessi delle
sinergie e della fusione appartengono a `BuildScreen` (vedi
`ui/inventory-and-synergy-screen.md`).

## Salute stratificata

La salute base e la salute temporanea/protettiva devono essere distinguibili a colpo
d'occhio (colore, forma o strato separato); l'ordine di consumo è sempre: prima la
temporanea, poi la base (DEC-008). Fonte di sistema: `systems/health-and-resources.md`.

## Risorse per funzione

Valuta principale, strumento di breccia, strumento di apertura e catalizzatore di fusione
sono mostrati con nomi placeholder per funzione (DEC-013): nessun riferimento al set
cuori/monete/bombe/chiavi di altri giochi. Il catalizzatore di fusione è una risorsa nuova,
distinta dalle altre tre.

## Slot attivo e Innesto

Si parte con 1 slot attivo e 1 slot Innesto; oggetti o eventi rari possono aggiungere slot
durante la run (DEC-011). L'HUD mostra sempre lo stato corrente degli slot posseduti, non
il numero massimo teorico.

## Stile visivo (DEC-046, rimando)

L'HUD, come tutta l'interfaccia del gioco, è pixel art: fonte unica della regola è
[Visual Language](../content/visual-language.md), non riformulata qui.

## Priorità visiva

1. sopravvivenza (salute base e temporanea);
2. minacce e cooldown;
3. risorse spendibili (valuta, breccia, apertura, catalizzatore di fusione);
4. progressione della run (piano e stanza);
5. informazioni competitive.

## Non-obiettivi

- Non mostra formule interne, dettagli tecnici o prompt dell'IA (fonte unica: `06-ai-content-generation-model.md`).
- Non sostituisce `BuildScreen` per la spiegazione delle sinergie.

## Domande aperte residue

- I nomi definitivi delle risorse e degli slot dipendono dal nome del gioco (vedi `governance/open-questions.md`).

## Scenari verificabili

1. **Given** il giocatore ha sia salute base sia salute temporanea, **when** subisce danno, **then** la salute temporanea si riduce per prima e resta visivamente distinta da quella base.
2. **Given** il giocatore possiede catalizzatore di fusione sufficiente per una fusione, **when** osserva l'HUD, **then** l'indicatore del catalizzatore appare evidenziato rispetto allo stato "insufficiente".
3. **Given** il giocatore raccoglie un oggetto raro che aggiunge uno slot Innesto, **when** l'HUD si aggiorna, **then** compare un secondo slot Innesto vuoto.
4. **Given** una modalità competitiva è attiva, **when** il giocatore gioca in `Gameplay`, **then** l'HUD mostra lo stato competitivo essenziale senza rivelare informazioni tecniche della generazione.
