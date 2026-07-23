---
id: ref-pack-visione-prodotto-e-posizionamento
title: Visione del prodotto e posizionamento
domain: references
status: proposed
authority: supporting
owner: design
summary: >-
  Riposiziona Worldsmelt come arcade action roguelite AI-native; confronta con NVIDIA NVIGI e AI Roguelite 2D; ribadisce modello unico per tutti.
last_reviewed: 2026-07-22
topics: [posizionamento, marketing, modello-unico, visione-prodotto, confronto-competitor]
related: []
supersedes: []
source_files: []
---

# Visione del prodotto e posizionamento

## Identità corretta

Worldsmelt è un **arcade action roguelite AI-native**. L’AI locale non è un’opzione cosmetica e non è soltanto uno strumento di sviluppo: è il meccanismo che produce la varietà sostanziale delle run.

La promessa non dovrebbe essere genericamente “contenuto infinito” o “gioco creato dall’AI”. È più precisa:

> Ogni run viene progettata localmente da un modello AI che scrive e valida nuove meccaniche di combattimento, oggetti, sinergie e comportamenti in una sandbox. Tutto funziona offline.

## Cosa deve significare “nuova meccanica”

Non basta:

- cambiare il colore di un nemico;
- aumentare il numero dei proiettili;
- moltiplicare danno o velocità;
- scegliere un tratto da una lista chiusa;
- ricombinare soltanto quattro operazioni note.

Deve essere possibile esprimere programmi come:

- un raggio continuo che colpisce lungo un segmento;
- un colpo che si ancora a un nemico e gli ruota attorno;
- frammenti che si attivano dopo il terzo rimbalzo;
- una cura che crea un anello offensivo soltanto sotto una condizione;
- un boss che cambia pattern in base alle sinergie del giocatore;
- proiettili che memorizzano una traiettoria o reagiscono a eventi precedenti;
- una sinergia tra due script che produce un effetto non scritto esplicitamente nel C.

Il modello inventa il programma, ma non può inventare capacità che il motore non espone. Il motore deve offrire **primitive generiche**, non archetipi completi.

## Posizionamento rispetto a progetti esistenti

Non è prudente affermare in senso assoluto che nessuno abbia mai integrato LLM e gameplay generato.

Esempi rilevanti:

- **AI Roguelite 2D** dichiara generazione AI di contenuti e meccaniche durante il gioco.
- **NVIDIA NVIGI Code Agent Lua Dungeon Crawler** usa un LLM locale per generare Lua eseguibile che controlla un compagno.
- Esistono demo e ricerche su agenti che trasformano comandi in codice o DSL di gioco.

La combinazione specifica di Worldsmelt resta però rara:

- open-weight model incluso nella distribuzione;
- offline e locale;
- action roguelite arcade in tempo reale;
- generazione per run, non solo dialogo NPC;
- Lua realmente eseguito;
- sandbox con limiti forti;
- oggetti, proiettili, sinergie, nemici e boss;
- supporto cross-vendor e obiettivo Steam Deck;
- fallback e validazione automatica.

Formula di marketing difendibile:

> **Un roguelite offline in cui un modello AI locale scrive e collauda nuove regole di combattimento per ogni run.**

## Un solo modello e parità qualitativa

La scelta dichiarata è corretta:

- stesso modello per tutti;
- stessa quantizzazione;
- stesso prompt e versione API;
- stessa pipeline di validazione;
- hardware più potente = attesa minore;
- hardware meno potente = attesa maggiore, non contenuti peggiori.

Il prodotto può avere fallback in caso di errore, ma non livelli commerciali “lite/quality” con differenti capacità creative.

## Conseguenza progettuale

Il requisito minimo del gioco non è soltanto “60 FPS”. Comprende anche un tempo di generazione accettabile. Per questo la progettazione deve trasformare l’attesa in parte del loop:

1. preparazione della run o del piano;
2. generazione e retry;
3. compilazione Lua;
4. dry-run e simulazione;
5. pubblicazione atomica del `RunBundle`;
6. scaricamento del modello;
7. gameplay senza inferenza nel frame critico.

La generazione può avvenire prima della run o tra piani, ma non dovrebbe contendere GPU e memoria al combattimento.
