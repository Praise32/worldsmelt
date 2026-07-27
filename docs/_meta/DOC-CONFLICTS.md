---
id: meta-doc-conflicts
title: Registro dei conflitti documentali (DOC-CONFLICT-001..046)
domain: meta
status: approved
authority: supporting
owner: meta
summary: >-
  46 conflitti rilevati dall'audit del 2026-07-22 (sonnet propone, opus verifica,
  Fable arbitra i needs-human), con raccomandazione secondo la gerarchia delle fonti e risoluzione adottata.
  Registro vivo (DEC-150): ogni voce porta oggi un campo Stato (aperta/applicata/superata)
  verificato il 27/07.
last_reviewed: 2026-07-27
last_verified_commit: d30890b
topics: [audit, conflitti, governance, registro-vivo]
related: [meta-document-standards]
supersedes: []
source_files: []
---

# Registro dei conflitti documentali

Formato: fonti, decisione coinvolta, evidenza, raccomandazione (gerarchia delle fonti),
risoluzione adottata nella migrazione, rischio. I 6 conflitti `needs-human` portano il
verdetto dell'arbitro (Fable) con default reversibile + open question: nessuna decisione
di design approvata e' stata modificata.

**Registro vivo (DEC-150).** Ogni voce porta un campo **Stato** — *aperta* / *applicata* /
*superata* — con la DEC o il commit che l'ha chiusa, verificato il 27/07/2026 contro lo
stato reale del repo (non dedotto dalle sole intenzioni di migrazione). **Mappa dei
percorsi pre-migrazione citati nelle voci sotto** (il contenuto e' stato spostato, non
riscritto, salvo dove lo Stato lo segnala esplicitamente):

| Percorso citato nelle voci (pre-migrazione) | Percorso attuale |
|---|---|
| `game-design-knowledge-base/docs/game-design/` | `docs/design/` (stessa struttura di sottocartelle: `systems/`, `ui/`, `content/`, `governance/`) |
| `worldsmelt-ai-production-blueprint-v2/NN-X.md` | `docs/ai-production/NN-X.md` (stessa numerazione) |
| `worldsmelt-ai-production-blueprint-v2/agent-config/.../worldsmelt-*.md` | `.claude/agents/worldsmelt-*.md` |
| `worldsmelt-ai-production-blueprint-v2/19-DECISION-QUESTIONNAIRE.md` | `docs/archive/superseded/19-DECISION-QUESTIONNAIRE.md` (chiuso da DEC-147) |
| `worldsmelt-ai-production-blueprint-v2/24-PROPOSED-KB-UPDATES.md` | `docs/plans/cancelled/aiprod-proposed-kb-updates.md` (chiuso da DEC-147) |
| `roguelike-ai-appunti/0N-X.md`, `piano-roguelike-ai.md` | `docs/archive/legacy-notes/roguelike-ai-appunti/` |
| `worldsmelt-research-pack-2026-07-21/NN-X.md` | `docs/references/research/worldsmelt-research-pack-2026-07-21/NN-X.md` |
| `docs/references/*.md` (livello radice) | `docs/references/research/*.md` |
| `docs/DESIGN_NOTES.md` | `docs/archive/legacy-notes/design-notes.md` |
| `docs/APPUNTI.md` | `docs/archive/legacy-notes/appunti.md` |
| `docs/ISSUE_NOTES.md` | `docs/archive/legacy-notes/issue-notes.md` |
| `docs/OPENAI_SETUP.md` | `docs/archive/superseded/openai-setup.md` |
| `docs/BENCHMARKS.md` | `docs/engineering/benchmarks.md` (versione 13/07 congelata in `docs/archive/superseded/benchmarks-2026-07-13.md`) |
| `docs/SPRITES-SPIKE.md` | `docs/ai-production/experiments/sprites-spike.md` |
| `docs/dataset/README.md`, `docs/dataset/TRAINING-RUNBOOK.md` | `docs/ai-production/dataset/README.md`, `docs/ai-production/dataset/TRAINING-RUNBOOK.md` |
| `docs/superpowers/specs/2026-07-13-*.md` | `docs/engineering/specs/2026-07-13-*.md` |
| `docs/superpowers/plans/2026-07-13-linux-local-llm.md` | `docs/archive/historical-plans/2026-07-13-linux-local-llm.md` |
| `worldsmelt_sintesi_strategica.md` | nessuno: mai tracciato in git (`git log --all` senza risultati) |

`worldsmelt-ai-production-blueprint-v2/00-DECISIONI-CANONICHE.md` a `07-ARCHITETTURA-RUNTIME.md`
seguono la stessa regola generale (numerazione invariata sotto `docs/ai-production/`).

## DOC-CONFLICT-001 — generazione-piani-stanze.md raccomanda una griglia uniforme 9x8 che DEC-009 esplicitamente scarta

- **Stato**: applicata — migrato in `docs/references/research/generazione-piani-stanze.md` (rango 8, non autorevole); DEC-009 intatta, nessuna nuova contraddizione osservata.
- **Fonti**: `docs/references/generazione-piani-stanze.md`, `game-design-knowledge-base/docs/game-design/systems/rooms-and-floor-generation.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md`
- **Decisioni**: DEC-009
- **Rischio**: alto
- **Evidenza**: VERIFICATO. Ref §8.2.1 (righe 236-239): 'Griglia piu' grande... portare GRID_SIZE a 9x8... con 5x5 le piante sono troppo piccole'. DEC-009 (decision-log riga 108): 'Alternative considerate: Mantenere stanze di dimensione uniforme' — scartata; la decisione impone stanze di grandezze diverse con minima garantita. Codice conforme: game_types.h:46 GRID_SIZE 5; world.c:209-218 assegna r->w/r->h per-stanza da un pool di taglie diverse, con guardia difensiva contro il ripiego sulla taglia massima. Nota: il doc stesso (riga 216) ammette gia' '5x5 non 9x8'.
- **Raccomandazione**: Vince la KB (DEC-009, approved, fonte #1) e il codice gia' conforme: la griglia uniforme stile Isaac non va adottata. In migrazione, la parte di proposta §8.2.1 (griglia uniforme piu' grande) e' superata; le sezioni algoritmiche (regole di accettazione BFS, piazzamento speciali nei vicoli ciechi, divisione compiti C/LLM) restano riferimento valido come ispirazione da riadattare a stanze di taglia variabile.
- **Risoluzione**: In migrazione (docs/references/ con nota di cappello, o docs/archive/superseded/ per la sola proposta di griglia), aggiungere l'avviso: 'La proposta di ingrandire GRID_SIZE a griglia uniforme 9x8 e' superata da DEC-009 (dimensione uniforme delle stanze esplicitamente scartata; il codice implementa gia' il lattice a taglie diverse). Le sezioni su ramificazione/BFS/vicoli ciechi restano valide come ispirazione algoritmica.' Non toccare rooms-and-floor-generation.md ne' il codice: nessuna decisione di design cambia.

## DOC-CONFLICT-002 — design-sinergie.md propone un campo Item.archetype mai realizzato: l'implementazione reale usa segnali diversi

- **Stato**: applicata — migrato in `docs/references/research/design-sinergie.md`; DEC-037 e `src/gameplay/synergies.c` restano invariati, nessuna contraddizione residua.
- **Fonti**: `docs/references/design-sinergie.md`, `src/gameplay/synergies.c`, `game-design-knowledge-base/docs/game-design/systems/synergies.md`
- **Rischio**: basso
- **Evidenza**: VERIFICATO. Ref §4.1/§6.1 propone 'typedef enum ItemArchetype { ARCH_NONE=0, ARCH_BOUNCE, ... }' persistito su Item. 'grep -rn ItemArchetype src/' non trova nulla. synergies.c (righe 14-26) usa SynergySignal su Item.traits esistenti + SIG_SHOT_CHAIN/SIG_SHOT_PIERCE; il commento di testata dichiara esplicitamente 'perche' non c'e' Item.archetype'. KB synergies.md rimanda a synergies.c righe 55-106 come esempi canonici delle 6 sinergie, senza prescrivere il meccanismo interno.
- **Raccomandazione**: Nessun conflitto di design: la KB synergies.md prescrive solo il risultato (le 6 sinergie canoniche, realizzate in codice con i nomi della tabella), non il 'come'. L'implementazione reale vince come fonte di verita' tecnica sul meccanismo. Il documento resta valido come racconto del ragionamento e degli esempi; la proposta concreta dell'enum ItemArchetype e' superata e non va piu' riproposta come prossimo passo.
- **Risoluzione**: In migrazione (docs/references/ con nota, o docs/archive/superseded/), annotare a fondo §4.1 e §6: 'Realizzato con un meccanismo diverso: vedi src/gameplay/synergies.c (SynergySignal su traits + tipo di colpo attivo, nessun campo Item.archetype introdotto).' Le sezioni concettuali (fonti, mappatura cache/trait, esempi) restano materiale di riferimento valido.

## DOC-CONFLICT-003 — pattern-nemici-e-boss.md non riflette i vincoli approvati successivamente su roster, Veterano e danno da contatto

- **Stato**: applicata — migrato in `docs/references/research/pattern-nemici-e-boss.md` come catalogo di pattern (non fonte di vincoli); i vincoli restano in `docs/design/systems/enemies.md`.
- **Fonti**: `docs/references/pattern-nemici-e-boss.md`, `game-design-knowledge-base/docs/game-design/systems/enemies.md`
- **Decisioni**: DEC-053, DEC-104, DEC-024, DEC-061, DEC-072
- **Rischio**: medio
- **Evidenza**: VERIFICATO. Il catalogo pattern usa solo i 4 EnemyKind attuali (ENEMY_CHASER/SHOOTER/TANK/BOSS, doc riga 31 = game_types.h:105-110) e cita liberamente il 'danno da contatto' (righe 182, 211) senza la regola DEC-061 'contatto solo se la silhouette lo telegrafa' (enemies.md righe 92-107). Nessuna menzione di roster 6-8 per run (DEC-053), estensione best-of nei piani avanzati (DEC-104), Veterano/Tempered a frequenza crescente. Non c'e' contraddizione diretta ma un gap silenzioso: il doc precede queste decisioni.
- **Raccomandazione**: Non e' una contraddizione diretta ma un gap: chi usa questo catalogo per implementare on_enemy_update deve integrare i vincoli di enemies.md (banda di potenza, roster 6-8, silhouette-contatto) qui assenti. La KB regola i vincoli di generazione; il documento resta utile SOLO come catalogo di pattern di movimento/attacco (fatti algoritmici liberi), non come fonte di vincoli.
- **Risoluzione**: In migrazione verso docs/references/, nota di cappello: 'Catalogo di pattern (fatti algoritmici). I vincoli di generazione (roster 6-8 DEC-053, estensione best-of DEC-104, bande di potenza, danno da contatto dichiarato dalla silhouette DEC-061) sono definiti in game-design-knowledge-base/.../enemies.md e vanno rispettati in ogni implementazione basata su questo catalogo, non riformulati qui.' Nessun cambio di design.

## DOC-CONFLICT-004 — 16-AUDIO-GENERATION-PIPELINE propone audio generativo in pre-alpha contro DEC-036 (audio curato)

- **Stato**: superata (DEC-109, 22/07) — l'audio generativo è stato adottato: `docs/ai-production/16-AUDIO-GENERATION-PIPELINE.md` è sbloccata e promossa, `docs/design/content/audio-and-feedback.md` aggiornato. Nota: README.md 'Limiti intenzionali' non ancora allineato (fuori perimetro di questo pacchetto).
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/16-AUDIO-GENERATION-PIPELINE.md`, `game-design-knowledge-base/docs/game-design/content/audio-and-feedback.md`, `worldsmelt-ai-production-blueprint-v2/19-DECISION-QUESTIONNAIRE.md`
- **Decisioni**: DEC-036, DEC-018, DEC-024
- **Rischio**: alto  |  **needs-human**
- **Evidenza**: VERIFICATO. 16-AUDIO (front matter status: proposed-conflict, owner audio) dichiara esso stesso il conflitto: 'La nuova direzione propone di introdurre una pipeline audio generativa gia nella pre-alpha' con Stable Audio 3 Small nel Piano 0/fra i piani; audio-and-feedback.md (approved, DEC-036 sez. dedicata) afferma 'Per ora musica e suoni sono curati e statici' e 'la generazione audio a tema e un'idea futura'. DEC-036 e approved nel decision-log. Q-AUD-001 e marcata BLOCKING nel questionario e blocca esplicitamente 'modifica DEC-036'. Q-AUD-004 (licenza Stability AI, gratuita sotto 1M$ ricavi) e BLOCKING prima della vendita.
- **Raccomandazione**: Vince decision-log/DEC-036 (gerarchia punto 1) finche il proprietario non approva una nuova decisione. Il blueprint e consapevole del conflitto e non va migrato come design canonico. rFXGen per SFX sintetici semplici e tecnica di produzione compatibile con i 'mezzi curati' di DEC-036 e puo migrare in docs/ai-production/ senza toccare il design; Stable Audio 3 (musica/SFX complessi a runtime nel Piano 0) resta bloccato da DEC-036 e richiede una nuova DEC prima di qualunque implementazione o di modificare audio-and-feedback.md.
- **Risoluzione**: Migrare 16-AUDIO-GENERATION-PIPELINE.md in docs/ai-production/ come piano di produzione, con testata 'proposta non approvata, bloccata da DEC-036' finche Q-AUD-001 non riceve risposta dal proprietario (non da un agente). Se approvata, registrarla come nuova DEC (es. DEC-109) nel decision-log e poi propagarla a content/audio-and-feedback.md, che oggi resta la fonte valida. Preservare la sezione licenza Stable Audio (soglia 1M$, Q-AUD-004) come nota di rischio commerciale in docs/ai-production indipendentemente dall'esito.
- **ARBITRATO (Fable)**: DEC-036 resta intatta. 16-AUDIO migra in docs/ai-production/ con status=proposed e testata 'bloccato da DEC-036/Q-AUD-001'. Open question registrata in governance/open-questions.md. Nessun cambio di design.

## DOC-CONFLICT-005 — 17-ASSET-CURATION e Q-F0-001 trattano l'identita del Piano 0 come aperta, ma e gia risolta da 8+ decisioni approvate

- **Stato**: applicata — Q-F0-001 chiusa; `docs/design/systems/floor-zero.md` resta fonte, `17-ASSET-CURATION-AND-FLOOR-ZERO.md` migrato in `docs/ai-production/17-ASSET-CURATION-AND-FLOOR-ZERO.md`.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/17-ASSET-CURATION-AND-FLOOR-ZERO.md`, `worldsmelt-ai-production-blueprint-v2/19-DECISION-QUESTIONNAIRE.md`, `worldsmelt-ai-production-blueprint-v2/24-PROPOSED-KB-UPDATES.md`, `game-design-knowledge-base/docs/game-design/systems/floor-zero.md`
- **Decisioni**: DEC-004, DEC-029, DEC-040, DEC-063, DEC-070, DEC-085, DEC-086, DEC-087, DEC-094
- **Rischio**: medio
- **Evidenza**: VERIFICATO. 19-DECISION-QUESTIONNAIRE marca Q-F0-001 BLOCKING con raccomandazione 'B+C: struttura curata, libreria costruita dalle migliori generazioni'; 24-PROPOSED sez. Floor Zero chiede di 'Risolvere Q-F0-001'. Ma floor-zero.md (approved, last_reviewed 2026-07-19) gia specifica esattamente B+C con dettaglio piu fine: hub/struttura curata con museo da best-of storici (DEC-040/DEC-063), pool curato minimo quando mancano best-of (DEC-087/DEC-094), fallback statico curato garantito (DEC-004), scelta binaria completo/solo-curato a due carte al primo avvio (DEC-070/DEC-086). Nessun disaccordo di merito: la raccomandazione del blueprint coincide col design gia approvato; e solo un marcatore di domanda rimasto indietro.
- **Raccomandazione**: Vince decision-log: Q-F0-001 e di fatto gia risposta (sostanzialmente B+C, con dettagli molto piu fini di quanto propone il blueprint). Il blueprint e rimasto indietro rispetto alle decisioni del 19/07 (DEC-085..094). Nessuna azione di design richiesta: la parte identita del Piano 0 si chiude senza coinvolgere il proprietario.
- **Risoluzione**: In migrazione, segnare Q-F0-001 come 'gia risolta dal decision-log, vedi DEC-004/DEC-040/DEC-063/DEC-070/DEC-085/DEC-086/DEC-087/DEC-094' o eliminarla da 19-DECISION-QUESTIONNAIRE.md. Il resto di 17-ASSET-CURATION.md (stati candidate/curated/quarantine, manifest JSON, directory art/, tool melting-curator) e materiale di produzione ortogonale al design e non contraddetto: migra in docs/ai-production/ con rimando esplicito a systems/floor-zero.md per il 'cosa', senza riformularlo. Aggiornare 24-PROPOSED-KB-UPDATES.md sez. Floor Zero segnandola chiusa dal design, non 'proposed-conflict'.

## DOC-CONFLICT-006 — 00-DECISIONI-CANONICHE e 24-PROPOSED-KB-UPDATES sono uno snapshot pre-fusione non allineato alle 108 decisioni

- **Stato**: applicata — `00-DECISIONI-CANONICHE.md` migrato in `docs/ai-production/00-DECISIONI-CANONICHE.md`; `24-PROPOSED-KB-UPDATES.md` chiuso da DEC-147 e archiviato in `docs/plans/cancelled/aiprod-proposed-kb-updates.md`; DEC-046 (UI pixel-art) confermata.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/00-DECISIONI-CANONICHE.md`, `worldsmelt-ai-production-blueprint-v2/24-PROPOSED-KB-UPDATES.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md`, `game-design-knowledge-base/docs/game-design/content/visual-language.md`
- **Decisioni**: DEC-046, DEC-036, DEC-002, DEC-020, DEC-070
- **Rischio**: medio
- **Evidenza**: VERIFICATO. 00-DECISIONI-CANONICHE.md ha intestazione 'Stato: proposta consolidata da adottare come baseline tecnica' (mai promossa a decision-log) e non cita alcun numero DEC. 24-PROPOSED sez. Visual Language propone 'UI costruita nello stesso linguaggio pixel-art' e 'token semantici' come se aperto, ma DEC-046 (approved) ha gia reso la pixel art canone totale, UI inclusa (confermato in visual-language.md sez. DEC-046: 'La UI e custom e in pixel art anch'essa'). Le righe di 00 'Nessuna inferenza durante il combattimento' e 'Modalita solo-curato permanente e dignitosa' concordano con DEC-002/DEC-020/DEC-070, non sono proposte aperte. La sez. Audio di 24 rimanda al conflitto audio (bloccato).
- **Raccomandazione**: Il decision-log vince per gerarchia (punto 1). 00 e 24 sono per lo piu contenuti tecnici/produttivi che non toccano il design tranne l'audio (gia coperto sopra, bloccato) e il dettaglio UI, gia risolto da DEC-046: la 'UI pixel-art' non e piu una proposta ma un livello di implementazione, non conflittuale. I dettagli token/9-slice/asset-AI-senza-testo sono aggiunte tecniche nuove, non contraddizioni.
- **Risoluzione**: 00-DECISIONI-CANONICHE.md (modelli, LoRA, training, licenze, agenti Kaggle) migra in docs/ai-production/ come baseline tecnica, rimuovendo lo status 'canoniche' fuorviante (front matter che chiarisce che NON e fonte di design). Annotare che 'nessuna inferenza in combattimento' e 'solo-curato permanente' sono gia confermate da DEC-002/DEC-020/DEC-070, non proposte. 24-PROPOSED va scomposto: Audio resta bloccata (conflitto 1), Floor Zero chiusa (conflitto 2), Visual Language/Generated Content Validation/Run Manifest migrano come proposte tecniche in docs/ai-production/ con rimando alle DEC gia approvate (DEC-046 su UI pixel-art), non come 'aggiornamenti da decidere'.

## DOC-CONFLICT-007 — 15-UI-DESIGN-PIPELINE non e in conflitto con ui/*, ma rischia sovrapposizione silenziosa se migrato senza rimandi

- **Stato**: applicata — `15-UI-DESIGN-PIPELINE.md` migrato in `docs/ai-production/15-UI-DESIGN-PIPELINE.md`; nessun conflitto sostanziale con `docs/design/ui/hud.md` e `ui/options-and-accessibility.md`.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/15-UI-DESIGN-PIPELINE.md`, `game-design-knowledge-base/docs/game-design/ui/hud.md`, `game-design-knowledge-base/docs/game-design/ui/options-and-accessibility.md`
- **Decisioni**: DEC-046, DEC-057, DEC-075
- **Rischio**: basso
- **Evidenza**: VERIFICATO. 15-UI-DESIGN-PIPELINE dichiara il confine corretto ('La knowledge base definisce cosa; questo documento propone come') e istruisce a leggere prima l'INDEX della KB e a non inventare interazioni per adattarsi a un mockup. Coerente con DEC-046 (pixel art anche in UI) e con le garanzie di accessibilita (dimensione testi, rimappatura totale) gia in options-and-accessibility.md (DEC-058). Nessuna cifra o interazione in contraddizione diretta con i documenti ui/*. Non e un conflitto sostanziale ma una nota di rotta per la migrazione.
- **Raccomandazione**: Nessuna fonte 'vince' perche non c'e conflitto: 15 e un documento 'come' legittimo, complementare al 'cosa' della KB. Non richiede decisione del proprietario.
- **Risoluzione**: Migrare 15-UI-DESIGN-PIPELINE.md diviso tra docs/ai-production/ (pipeline Penpot/SD1.5 per asset UI, LoRA worldsmelt-ui) e docs/engineering/ (moduli src/ui/*, renderer raylib, scaling/risoluzione logica), conservando l'avvertenza gia presente di consultare l'INDEX della KB e di non inventare interazioni nuove. Aggiungere rimandi espliciti a ui/hud.md e ui/options-and-accessibility.md per evitare che i componenti proposti (card tema, focus ring, hint controller) reinterpretino in silenzio il 'cosa'. Nessuna azione sui documenti ui/* della KB. Le domande UI del questionario (Q-UI-001..005, risoluzione logica/pixel scale) restano tecniche in docs/engineering, non design.

## DOC-CONFLICT-008 — Il diagramma "pipeline locale, di serie" nel README omette del tutto melting-sprites

- **Stato**: applicata — README.md aggiornato: `bin/melting-sprites` compare oggi nel diagramma della pipeline locale.
- **Fonti**: `README.md (sezione Pipeline dinamica, diagramma 'Percorso locale, di serie')`, `Makefile (run-gen, run-gen-fast, SPRITES_BIN)`, `tools/melting-sprites/`, `src/app/app.c (spritesPlannedThisRun, spritesCommand)`
- **Rischio**: medio
- **Evidenza**: CONFERMATO. README diagramma (righe 113-124) mostra solo bin/melting-gen. Makefile: 'run-gen: --generate' e 'run-gen-fast: --generate --no-sprites' con commento 'Stessa cosa di run-gen ma con --no-sprites' per non pagare melting-sprites ogni run. app.c: spritesPlannedThisRun = !noSprites && SpritesModelsPresent(); spritesCommand='bin/melting-sprites'. Il passo sprite gira di default a --generate.
- **Raccomandazione**: Vince lo stato codice/Makefile (tier 4) sul README non aggiornato. Aggiungere lo stadio melting-sprites (stable-diffusion.cpp) come secondo passo di default e --no-sprites/run-gen-fast come scorciatoia.
- **Risoluzione**: In migrazione aggiornare README.md (Pipeline dinamica + Architettura) per includere tools/melting-sprites. Cronologia confermata: README ultimo tocco eae72dd (13/07 10:07) precede il wiring c40b54d (13/07 12:36). Nessuna decisione di design in gioco.

## DOC-CONFLICT-009 — current_atlas.png descritto come artefatto esclusivo dell'Image API OpenAI, ma e' anche l'output di default di melting-sprites (locale)

- **Stato**: applicata — README.md e `docs/OPENAI_SETUP.md` (oggi `docs/archive/superseded/openai-setup.md`) corretti: `current_atlas.png` non è più presentato come esclusivo del percorso OpenAI.
- **Fonti**: `README.md (tabella file Architettura, riga 257)`, `docs/OPENAI_SETUP.md (sezione 3)`, `src/content/run_content.c (PreferPngAtlasIfFresh, righe 445-467)`, `tools/melting-sprites/sprite_manifest.c (SpritesUpdateManifestAtlasPath)`
- **Rischio**: medio
- **Evidenza**: CONFERMATO. README riga 257: 'current_atlas.png: spritesheet generato dalla Image API di OpenAI (percorso storico)'. run_content.c commento: 'e' melting-sprites... a riscrivere quella riga puntando al PNG'; PreferPngAtlasIfFresh sceglie il PNG se GetFileModTime(PNG) >= GetFileModTime(manifest), cioe' per data del file, non per provenienza. OPENAI_SETUP.md sez.3 elenca il PNG come file OpenAI.
- **Raccomandazione**: Vince il codice (run_content.c): current_atlas.png non e' piu' esclusivo del percorso OpenAI. Correggere README e OPENAI_SETUP.md: il PNG e' oggi prodotto di default da melting-sprites (locale); OpenAI, se usato, scrive lo stesso file, scelto per mtime.
- **Risoluzione**: Correggere la tabella file di README.md e la sezione 3 di OPENAI_SETUP.md (PNG condiviso dai due percorsi, scelta per data non per provenienza); spostare OPENAI_SETUP.md verso docs/references/ o docs/archive/superseded/ con la nota corretta.

## DOC-CONFLICT-010 — docs/DESIGN_NOTES.md presenta la sandbox Lua come ipotesi futura non scelta, ma DEC-037 l'ha gia' decisa ed e' implementata come percorso primario

- **Stato**: applicata — `docs/DESIGN_NOTES.md` archiviato in `docs/archive/legacy-notes/design-notes.md`; DEC-037 e la sandbox Lua restano fonte corrente.
- **Fonti**: `docs/DESIGN_NOTES.md (righe 50-74, 113)`, `game-design-knowledge-base/.../decision-log.md (DEC-037)`, `AGENTS.md (ScriptSandbox/ScriptVm)`, `src/script/ (script_sandbox.c, script_api.c, script_items.c, script_character.c)`, `docs/superpowers/specs/2026-07-13-lua-sandbox-design.md`
- **Decisioni**: DEC-037
- **Rischio**: medio
- **Evidenza**: CONFERMATO. DESIGN_NOTES.md righe 74/113: 'si puo' sostituire con Lua piu' avanti se diventa necessario' / 'Valutare Lua solo quando servono regole davvero piu' espressive'. DEC-037 (2026-07-18, approved): il trait unico e i tipi di colpo sono comportamenti Lua generati e validati in sandbox, manopole parametriche come garanzia/fallback. src/script/ implementa la sandbox Lua; la spec 2026-07-13-lua-sandbox-design.md esiste.
- **Raccomandazione**: Vince il decision-log (tier 1) + codice (tier 4): DESIGN_NOTES.md descrive uno stato pre-DEC-037 superato (l'intero doc e' anche OpenAI-centrico, righe 12/32/102). Archiviare come appunti superati; il razionale corretto e' gia' coperto da DEC-037 e dalla spec Lua.
- **Risoluzione**: Spostare docs/DESIGN_NOTES.md in docs/archive/legacy-notes/ (o superseded/); estrarre eventuali idee valide della sezione 'Potenziamenti futuri sensati' verso docs/plans o docs/references prima di archiviare il resto.

## DOC-CONFLICT-011 — docs/APPUNTI.md elenca come "Obiettivi di Sviluppo (Integrazioni Future)" funzionalita' gia' completate (LLM locale, sandbox Lua)

- **Stato**: applicata — `docs/APPUNTI.md` archiviato in `docs/archive/legacy-notes/appunti.md`.
- **Fonti**: `docs/APPUNTI.md (sezione 2; sezione 8 righe 146-162)`, `tools/melting-gen/ (llama.cpp Vulkan)`, `src/script/ (Lua 5.5)`, `Makefile (test-script, test-gen, test-sprites)`
- **Rischio**: basso
- **Evidenza**: CONFERMATO. APPUNTI.md sez.2 presenta 'Transizione a Sandbox Lua' e 'Integrazione LLM Locale... gguf-tools e llama.cpp' come obiettivi futuri. Entrambe realizzate: tools/melting-gen usa llama.cpp, src/script/ implementa la sandbox Lua, con target make test-gen/test-script/test-sprites. Sez.8 descrive pipeline SDXL+ControlNet+LoRA Pixel Art, mentre il percorso reale e' SD1.5 (tools/melting-sprites/sprite_sd.c).
- **Raccomandazione**: Vince lo stato codice/build (tier 4) sugli appunti iniziali storici. APPUNTI.md va trattato come note iniziali, non come stato o piano attuale.
- **Risoluzione**: Spostare docs/APPUNTI.md in docs/archive/legacy-notes/ con nota di cappello sulle sezioni gia' realizzate; la sezione 8 (SDXL+ControlNet+LoRA Pixel Art) e' superata dal percorso SD1.5 attuale (memoria 'Modello immagini provvisorio', LoRA pixel-art pendente) — se resta un'idea viva estrarla verso un piano/reference, altrimenti archiviarla.

## DOC-CONFLICT-012 — README apre con Quick Start .bat (Windows) e mantiene la sezione OpenAI a pari peso, mentre la spec approvata indica il percorso locale/Linux come riferimento

- **Stato**: applicata — README.md riordinato: Quick Start Linux/locale in testa, sezione OpenAI/.bat retrocessa a 'Percorso storico Windows / OpenAI'.
- **Fonti**: `README.md (sezioni 'Avvio rapido', 'OpenAI API')`, `docs/OPENAI_SETUP.md`, `docs/ISSUE_NOTES.md`, `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`, `AGENTS.md`, `llm/ (run_content.mjs, server.mjs)`
- **Rischio**: basso
- **Evidenza**: CONFERMATO come problema di gerarchia visiva, non di contenuto. README riga 3-12 dichiara il locale 'di serie', ma 'Avvio rapido' (righe 18-61) apre con .bat Windows PRIMA di 'Avvio rapido su Linux'. La spec 2026-07-13 (tier 2) pone come obiettivo la generazione locale 'senza rete e senza OpenAI'. Il README stesso dice che OpenAI 'resta disponibile... e non e' stato rimosso', quindi nessuna contraddizione di sostanza.
- **Raccomandazione**: Nessun conflitto di contenuto: il percorso OpenAI non va cancellato. La spec approvata (tier 2) chiarisce solo che il riferimento e' locale/Linux; e' disallineamento di ordine/enfasi, coerente con la gerarchia.
- **Risoluzione**: Riordinare README.md mettendo la Quick Start Linux/locale in testa ed etichettando la sezione OpenAI/.bat come 'percorso storico opzionale, Windows, richiede rete e chiave API'; spostare le parti OpenAI-specifiche di ISSUE_NOTES.md verso docs/archive o docs/references insieme a OPENAI_SETUP.md, con rimando dal README.

## DOC-CONFLICT-013 — AGGIUNTO: README 'Limiti intenzionali'/roadmap dichiara 'sprite locali' e 'sandbox Lua' come fasi future, ma entrambi sono gia' implementati

- **Stato**: applicata — README.md 'Limiti intenzionali' corretto: sprite locali (SD1.5) e sandbox Lua non più citati come fasi future.
- **Fonti**: `README.md (Limiti intenzionali, righe 272-284)`, `tools/melting-sprites/ (stable-diffusion.cpp, SD1.5)`, `scripts/download-models.sh (MODEL_SD, MODEL_LCM, MODEL_TAESD)`, `src/script/`, `game-design-knowledge-base/.../decision-log.md (DEC-037)`
- **Decisioni**: DEC-037
- **Rischio**: medio
- **Evidenza**: CONFERMATO e piu' netto del conflitto sul diagramma. README riga 272-273: 'Il percorso locale genera solo testo: lo spritesheet resta sempre l'atlas BMP interno (sprite generati in locale sono una fase futura della roadmap)'; righe 282-284 elencano 'sprite locali, sandbox Lua per sinergie uniche' tra 'le fasi future'. Ma melting-sprites genera sprite via SD1.5 (stable-diffusion.cpp) di default a --generate; download-models.sh scarica SD1.5+LCM-LoRA+taesd; src/script/ implementa Lua e DEC-037 lo rende percorso primario.
- **Raccomandazione**: Vince codice/Makefile (tier 4) + decision-log (tier 1) sul testo README non aggiornato. Sono affermazioni oggi false, non semplici omissioni: correggere 'Limiti intenzionali' (lo spritesheet locale non e' piu' solo BMP) e spostare 'sprite locali' e 'sandbox Lua' da roadmap a implementato.
- **Risoluzione**: Aggiornare README.md 'Limiti intenzionali' e la roadmap: rimuovere la frase 'il percorso locale genera solo testo', segnalare sprite locali (SD1.5) e sandbox Lua (DEC-037) come realizzati; coordinare con la correzione del diagramma (conflitto 1).

## DOC-CONFLICT-014 — Line 328 della sintesi ("RunBundle canonici invece di un unico modello obbligatorio") NON contraddice DEC-070: e' un altro asse (distribuzione contenuti, non tier di download modelli)

- **Stato**: applicata — nessuna contraddizione di merito (confermato); `worldsmelt_sintesi_strategica.md` non risulta mai tracciato in git (`git log --all` senza risultati): probabile file di lavoro esterno al repo.
- **Fonti**: `worldsmelt_sintesi_strategica.md:327-328`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-070)`, `worldsmelt-research-pack-2026-07-21/10-decisioni-e-domande-aperte.md (D2)`
- **Decisioni**: DEC-070
- **Rischio**: basso
- **Evidenza**: Riga 328 e' uno slogan conclusivo accanto a riga 327 "meno dipendenza da una singola piattaforma hardware": propone di distribuire RunBundle canonici (contenuti pre-generati condivisibili, gia' previsti da systems/run-manifest-and-reproducibility). DEC-070 riguarda invece i tier di download ("un solo set di modelli, nessun tier intermedio") e stabilisce gia' "solo curato" come stato legittimo e PERMANENTE, cioe' giocare senza modello. Il research pack stesso (D2) ribadisce "un solo modello canonico": non propone piu' modelli. La lettura dell'analista conflava "modello obbligatorio" con "set di modelli".
- **Raccomandazione**: Non e' una contraddizione con una decisione approvata: gerarchia non attivata perche' non c'e' collisione di merito. La preoccupazione "modello non obbligatorio per tutti" e' gia' risolta da DEC-070 (solo curato permanente) + DEC-002/DEC-020 (sempre giocabile). Nessuna modifica a DEC-070 ne' a 06-ai-content-generation-model.md.
- **Risoluzione**: Alla migrazione in docs/references/research/ aggiungere nota neutra: "gli slogan strategici (riga 328) non sono decisioni; la distribuzione di RunBundle e' compatibile con DEC-070, che gia' ammette il gioco senza modello (solo curato). Aprire una open-question SOLO se qualcuno vuole davvero cambiare la politica del set di modelli". Nessuna escalation umana.

## DOC-CONFLICT-015 — 10-decisioni-e-domande-aperte.md si autodefinisce "Decisioni consolidate" (D1-D10): registro parallelo non governato, con D1 in tensione con DEC-070

- **Stato**: applicata — migrato in `docs/references/research/worldsmelt-research-pack-2026-07-21/10-decisioni-e-domande-aperte.md` con front matter `status: proposed`, `authority: supporting` (non più 'decisioni consolidate' senza qualifica); il decision-log resta unica fonte.
- **Fonti**: `worldsmelt-research-pack-2026-07-21/10-decisioni-e-domande-aperte.md:3-45`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-070)`
- **Decisioni**: DEC-070
- **Rischio**: medio
- **Evidenza**: Il documento usa "## Decisioni consolidate" e la numerazione D1..D10 con lo stesso stile autorevole delle DEC-NNN, senza essere passato per la governance. D1 ("non rendere il modello opzionale nella visione del prodotto") e' addirittura in tensione con DEC-070/DEC-002/DEC-020 (solo curato permanente, gioco sempre giocabile). Il rischio e' previsto dal documento stesso: "documentazione contraddittoria: agenti futuri implementano la visione vecchia".
- **Raccomandazione**: decision-log.md resta unica fonte di 'decisioni' (gerarchia 1); un pacchetto di ricerca (gerarchia 7-8) non puo' usare lo stesso lessico. Corretto, non viola la gerarchia. needs_human=false: e' igiene documentale, non un cambio design/hardware/licenza.
- **Risoluzione**: Alla migrazione in docs/references/research/ rinominare la sezione in "Ipotesi di ricerca (non approvate)" e i codici D1..D10 in R1..R10; banner in testa che rimanda a decision-log.md come unica fonte canonica; nota specifica che R1 (ex D1) non prevale su DEC-070 (solo curato resta legittimo e permanente).

## DOC-CONFLICT-016 — 06-modelli-immagine-audio-stability assume come accettata una futura licenza Enterprise Stability AI a pagamento, senza alcuna decisione di progetto

- **Stato**: superata (DEC-113, 22/07) — accettati i termini della Stability AI Community License per Stable Audio Small; soglia Enterprise da rivalutare solo sopra 1M$/anno di ricavi.
- **Fonti**: `worldsmelt-research-pack-2026-07-21/06-modelli-immagine-audio-stability.md:116-121`, `docs/LICENZE.md`
- **Rischio**: medio  |  **needs-human**
- **Evidenza**: Il documento afferma: "Poiche' il passaggio Enterprise e' accettabile per il progetto, la licenza non deve guidare la scelta qualitativa". docs/LICENZE.md e' costruito interamente su licenze senza soglia di ricavo (Apache 2.0, OpenRAIL-M, openrail++, MIT), sul principio di non ridistribuire mai i pesi, e non registra alcuna accettazione di un obbligo Enterprise a pagamento. Adottare SD3.5/Stable Audio (Community License Stability) introdurrebbe una soglia di ricavo Enterprise assente nello stack attuale (SD1.5).
- **Raccomandazione**: docs/LICENZE.md (engineering verificato contro lo stack reale, gerarchia 4) + assenza in decision-log vincono: l'accettazione del tier Enterprise a pagamento e' un'assunzione del documento di ricerca, non una decisione presa. Le licenze sono categoria che richiede revisione umana esplicita.
- **Risoluzione**: Alla migrazione, riformulare la frase "e' accettabile per il progetto" come domanda aperta; aggiungere in docs/LICENZE.md (o governance/open-questions.md): "adottare SD3.5/Stable Audio comporterebbe potenzialmente la soglia di ricavi Enterprise Stability AI: da valutare esplicitamente prima di qualunque switch del modello immagine/audio". Nessuno switch di modello prima di una decisione.
- **ARBITRATO (Fable)**: Il research pack resta verbatim in docs/references/research/ (mai riscritto). La cautela sulla soglia Enterprise Stability AI diventa una open question di licenza in governance/open-questions.md.

## DOC-CONFLICT-017 — Priorita' dell'audio generativo: la sintesi lo mette tra le cose "da rinviare", 06 lo tratta come lavoro operativo prossimo senza cautela di scope

- **Stato**: superata (DEC-109, 22/07) — la priorità è stata decisa: audio generativo adottato subito (Stable Audio Small + fallback rFXGen/curato), non più rinviato.
- **Fonti**: `worldsmelt_sintesi_strategica.md:154-160`, `worldsmelt-research-pack-2026-07-21/06-modelli-immagine-audio-stability.md:74-106`, `worldsmelt-research-pack-2026-07-21/10-decisioni-e-domande-aperte.md (domanda aperta 11)`
- **Rischio**: basso
- **Evidenza**: Sintesi (righe 154-160) elenca "audio generativo" tra le cose "Da rinviare" per scope eccessivo. 06 dedica una sezione operativa dettagliata (Stable Audio 3 Small SFX/Music, Medium) senza cautela di scope. Il research pack stesso pero' hedgea: la domanda aperta 11 di file 10 chiede se Stable Audio "deve essere una fase successiva". Nessuna voce di decision-log arbitra la priorita'.
- **Raccomandazione**: Nessuna fonte ha rango superiore: nessun DEC arbitra. Coerente con la disciplina di scope della sintesi e con la nota di progetto ("non investire in qualita' visiva ora", LoRA da riallenare), il segnale piu' prudente e' rinviare. needs_human=false: e' prioritizzazione di scope, non una delle categorie a revisione umana.
- **Risoluzione**: Alla migrazione in docs/references/research/ marcare le sezioni audio di 06 come "post-vertical-slice", senza promuoverle a docs/plans/active finche' il combat loop non e' validato. Coerente con la domanda aperta 11 del research pack stesso.

## DOC-CONFLICT-018 — Licenza LCM-LoRA SD1.5 dichiarata in modo opposto in due fonti

- **Stato**: applicata — fix eseguito nella migrazione (Fable): `scripts/download-models.sh` etichetta oggi `lcm-lora-sdv1-5.safetensors` come openrail++ (verificato), coerente con `docs/ai-production/licenze.md`.
- **Fonti**: `docs/LICENZE.md:38`, `scripts/download-models.sh:82 (genera models/README.md)`
- **Rischio**: alto  |  **needs-human**
- **Evidenza**: Verificato: docs/LICENZE.md tabella modelli riporta 'LCM-LoRA SD1.5 | openrail++'; l'heredoc di scripts/download-models.sh che scrive models/README.md riporta 'lcm-lora-sdv1-5.safetensors ... (Apache 2.0, salvato con questo nome...)'. Contraddizione reale nel repo; la model card upstream latent-consistency/lcm-lora-sdv1-5 dichiara openrail++.
- **Raccomandazione**: docs/LICENZE.md e' corretto (allineato all'upstream); lo script che genera models/README.md ha l'etichetta sbagliata e va corretto per non far credere il file privo di vincoli d'uso (openrail++ comporta gli stessi obblighi di propagazione dell'Attachment A se un giorno si ridistribuiscono i pesi).
- **Risoluzione**: Correggere la riga dell'heredoc in scripts/download-models.sh (models/README.md) da 'Apache 2.0' a 'openrail++' per lcm-lora-sdv1-5.safetensors, con la nota di propagazione gia' presente per il checkpoint SD1.5. Alla migrazione portare docs/LICENZE.md in docs/engineering (o docs/design) come fonte canonica della tabella licenze modelli. Conferma umana perche' tocca una licenza dichiarata.
- **ARBITRATO (Fable)**: Fix fattuale eseguito in questa migrazione: l'heredoc di scripts/download-models.sh etichetta LCM-LoRA come 'Apache 2.0' ma l'upstream e' openrail++ (LICENZE.md corretto). Correzione dell'etichetta, non cambio di licenza accettata.

## DOC-CONFLICT-019 — Tre schemi diversi e incompatibili per il ledger di provenienza del dataset

- **Stato**: applicata — confrontato con `docs/ai-production/04-DATASET-LICENZE.md` (ex blueprint-v2): lo schema reale di `docs/ai-production/dataset/README.md` resta l'unico applicato; DEC-148 corregge ulteriormente le affermazioni non adottate di 04.
- **Fonti**: `docs/dataset/README.md`, `scripts/dataset_ledger.py`, `docs/dataset/ledger.jsonl`, `worldsmelt-ai-production-blueprint-v2/04-DATASET-LICENZE.md`, `worldsmelt-ai-production-blueprint-v2/templates/DATASET-LEDGER.md`
- **Rischio**: medio
- **Evidenza**: Verificato: dataset_ledger.py REQUIRED_FIELDS e README definiscono lo schema realmente eseguito (sha256, original_url, license_id, license_snapshot_date, role, author...) su ledger.jsonl da 3158 righe. blueprint-v2/04 propone campi inesistenti nel registro (source_url, downloaded_at, allowed_commercial, allowed_derivatives, training_allowed_explicit, split_group); templates/DATASET-LEDGER.md ne aggiunge un terzo (provenance_status, body_plan, view, original_sha256/processed_sha256).
- **Raccomandazione**: docs/dataset/README.md vince: documentazione tecnica verificata contro il codice eseguito e un ledger reale gia' popolato; gli schemi di blueprint-v2 sono proposte non implementate (rank inferiore) e vanno scartati o riscritti per aderenza allo schema reale prima di confluire in docs/ai-production/.
- **Risoluzione**: Alla migrazione portare docs/dataset/README.md + TRAINING-RUNBOOK.md in docs/ai-production/dataset/ come riferimento unico dello schema; archiviare 04-DATASET-LICENZE.md e templates/DATASET-LEDGER.md in docs/archive/superseded/ con nota di rimando allo schema reale, oppure riscrivere il template campo-per-campo su REQUIRED_FIELDS di dataset_ledger.py se si vuole tenere l'idea dei flag come estensione futura.

## DOC-CONFLICT-020 — Separazione research-unknown-provenance / commercial-clean (con dataset Kaggle da 89k immagini) mai adottata nella pipeline reale

- **Stato**: superata (DEC-148, 27/07) — i dataset attuali (incluso il Kaggle ~89k) non sono definitivi; il piano dataset a due rami è sostituito dal piano dataset proprietario.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/00-DECISIONI-CANONICHE.md:41-44`, `worldsmelt-ai-production-blueprint-v2/04-DATASET-LICENZE.md`, `docs/dataset/README.md`, `scripts/dataset_ledger.py:46`
- **Rischio**: medio
- **Evidenza**: Verificato: 00-DECISIONI-CANONICHE propone di separare research-unknown-provenance e commercial-clean e ammette il dataset Kaggle ebrahimelgazar/pixel-art (~89.000 immagini, provenienza non identificata) per ricerca. dataset_ledger.py LICENSE_WHITELIST = {cc0, cc0-1.0, own, commissioned}: nessun livello 'research'. Nel ledger 3158/3158 voci sono CC0 e non c'e' traccia del dataset Kaggle in nessun file del repo.
- **Raccomandazione**: docs/dataset/README.md (verificato contro il codice) vince: la pipeline reale e' a un solo livello (CC0/own/commissioned), piu' conservativa della proposta a due livelli, che non e' mai stata adottata ne' governata da alcun DEC.
- **Risoluzione**: Non portare research-unknown-provenance/commercial-clean e il dataset Kaggle nella migrazione come gia' adottati: archiviare la sezione in docs/archive/superseded/ (o docs/references/research/ se la si vuole tenere come opzione futura valutata), con nota che la pipeline attuale usa solo il registro CC0 a livello singolo di docs/dataset/.

## DOC-CONFLICT-021 — Elenco pack Kenney candidati diverso fra nota di ricerca e registro implementato

- **Stato**: applicata — `roguelike-ai-appunti/05` archiviato in `docs/archive/legacy-notes/roguelike-ai-appunti/05-dataset-e-licenze.md`; `docs/ai-production/dataset/README.md` resta la fonte sui pack Kenney realmente scaricati (Pixel UI Pack mai aggiunto).
- **Fonti**: `roguelike-ai-appunti/05-dataset-e-licenze.md:36-43`, `docs/dataset/README.md:72-82`, `docs/dataset/TRAINING-RUNBOOK.md:100`
- **Rischio**: basso
- **Evidenza**: Verificato: roguelike-ai-appunti/05 elenca 6 pack Kenney incluso Pixel UI Pack; docs/dataset/README.md ne elenca 5 (senza Pixel UI Pack) e TRAINING-RUNBOOK.md parla di '3158 file da 6 pack' (5 Kenney + superpowers). In ledger.jsonl le fonti Kenney distinte sono 5 (tiny-dungeon, micro-roguelike, 1-bit-pack, pixel-shmup, top-down-shooter) e le voci con 'pixel-ui' sono zero.
- **Raccomandazione**: docs/dataset/README.md vince perche' rispecchia il registro realmente popolato; roguelike-ai-appunti/05 e' la nota di ricerca originale del 16/07, superata su questo punto.
- **Risoluzione**: Migrare roguelike-ai-appunti/05-dataset-e-licenze.md in docs/archive/legacy-notes/ con nota 'superato da docs/dataset/README.md sul Pixel UI Pack (mai scaricato)'; se un giorno serve, aggiungerlo al registro con dataset_ledger.py add come le altre fonti Kenney (comunque CC0 secondo la stessa FAQ Kenney).

## DOC-CONFLICT-022 — Checkpoint SD1.5 di base: 'vanilla' dichiarato canonico in blueprint-v2 contro il fine-tune pixel-art realmente scaricato

- **Stato**: superata (DEC-148, 27/07) — base immagini SD1.5 confermata; la Style LoRA si addestra su base vanilla (non su `pixel-baseline`); il runtime resta su `pixel-baseline` fino a validazione.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/00-DECISIONI-CANONICHE.md:7`, `worldsmelt-ai-production-blueprint-v2/02-STACK-MODELLI.md:41-58`, `scripts/download-models.sh:31-32`, `docs/LICENZE.md:37`
- **Rischio**: basso
- **Evidenza**: Verificato: 00-DECISIONI-CANONICHE riga 7 'Base immagini: Stable Diffusion 1.5 vanilla' e 02-STACK-MODELLI tratta i checkpoint pixel di terze parti solo come benchmark; scripts/download-models.sh righe 31-32 scaricano invece PublicPrompts/All-In-One-Pixel-Model.ckpt come base runtime, come conferma docs/LICENZE.md riga 37. Nessuna Style LoRA ancora addestrata.
- **Raccomandazione**: La realta' implementata (download-models.sh + docs/LICENZE.md, rank 4/codice) vince: il checkpoint base in uso e' un fine-tune pixel-art di terze parti, non vanilla. Entrambi restano CreativeML OpenRAIL-M, impatto legale nullo, ma la decisione 'vanilla' del blueprint va corretta prima di confluire in docs/ai-production, altrimenti documenta un'opzione mai scelta.
- **Risoluzione**: Alla migrazione aggiornare 00-DECISIONI-CANONICHE/02-STACK-MODELLI per registrare l'opzione effettivamente adottata (checkpoint pixel-art PublicPrompts/All-In-One-Pixel-Model) come base runtime attuale, citando docs/SPRITES-SPIKE.md come motivazione dello spike; mantenere 'vanilla + Style LoRA' come piano futuro dichiarato, non come stato corrente.

## DOC-CONFLICT-023 — "Melting Run" vs "Worldsmelt" nei prompt degli agenti in .claude/agents/

- **Stato**: applicata — verificato: `.claude/agents/melting-implementer.md:7`, `melting-verifier.md:7`, `melting-content-designer.md:3,7` dicono oggi 'Worldsmelt', non più 'Melting Run'.
- **Fonti**: `.claude/agents/melting-implementer.md`, `.claude/agents/melting-verifier.md`, `.claude/agents/melting-content-designer.md`, `CLAUDE.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-071)`
- **Decisioni**: DEC-071, DEC-003
- **Rischio**: basso
- **Evidenza**: Verificato: melting-implementer.md:7 "Sei l'implementatore di Melting Run"; melting-verifier.md:3 (description) usa "Worldsmelt" ma il corpo r.7 "Sei lo scettico di Melting Run" (incoerenza interna al file confermata); melting-content-designer.md:3 e :7 "Melting Run". DEC-071 (approved, r.729-737): titolo definitivo "Worldsmelt"; "Melting Run resta solo il nome storico di questo repository ... nei documenti vivi il gioco e Worldsmelt". DEC-003 (r.48) annotata come risolta da DEC-071.
- **Raccomandazione**: DEC-071 (decision-log approvato, rango 1) vince; CLAUDE.md root e gia allineato. Correggere i tre file agente: sostituire "Melting Run" riferito AL GIOCO con "Worldsmelt", lasciando invariati i nomi tecnici/storici (melting-implementer, melting-verifier, tools/melting-gen, ecc.) che sono nomi di file/tool, non il titolo del gioco.
- **Risoluzione**: Edit mirato: melting-implementer.md:7, melting-verifier.md:7 e melting-content-designer.md:3,7 sostituendo "Melting Run" con "Worldsmelt" dove indica il gioco. Nessun cambio di design: sola propagazione di DEC-071 gia approvata.

## DOC-CONFLICT-024 — Percorsi INDEX/decisioni-canoniche inesistenti negli agent-config del pacchetto ai-production

- **Stato**: applicata (2026-07-27) — gli agent-config sono migrati in `.claude/agents/worldsmelt-*.md`; i riferimenti puntano oggi a `docs/ai-production/`. Il residuo segnalato lo stesso giorno — `worldsmelt-path-orchestrator.md:19` che citava `docs/ai-production/19-DECISION-QUESTIONNAIRE.md`, archiviato da DEC-147 — è stato chiuso: lo step 5 dell'orchestratore rimanda ora alla sola coda ufficiale `docs/design/governance/open-questions.md` (DEC-147). Il file sta in `.claude/agents/`, fuori da `docs/`, ma `make docs-check` ne verifica i puntatori: lasciarlo rotto avrebbe tenuto il gate rosso.
- **Fonti**: `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-path-orchestrator.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/codex/AGENTS-AI-PRODUCTION-APPENDIX.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/CLAUDE-ML-APPENDIX.md`, `worldsmelt-ai-production-blueprint-v2/INDEX.md (percorso reale odierno)`, `docs/ai-production/INDEX.md (destinazione di migrazione, ora stub generato)`
- **Rischio**: medio
- **Evidenza**: Verificato: path-orchestrator.md:13 e codex/AGENTS-AI-PRODUCTION-APPENDIX.md:5 puntano a "docs/worldsmelt-ai-production-blueprint/INDEX.md"; CLAUDE-ML-APPENDIX.md:8 a "worldsmelt-ai-blueprint/00-DECISIONI-CANONICHE.md" (e :10 a ml/run_policy.yaml). Confermato che docs/worldsmelt-ai-production-blueprint NON esiste. Il pacchetto reale (non tracciato in git) e worldsmelt-ai-production-blueprint-v2/ con INDEX.md e 00-DECISIONI-CANONICHE.md alla radice. Correzione all'analista: docs/ai-production/ NON e piu vuota, contiene INDEX.md ma e uno stub auto-generato ("make docs-index"), non il contenuto migrato del blueprint.
- **Raccomandazione**: Nessuna fonte 'vince': sono refusi/percorsi anticipatori verso una posizione non ancora esistente. Allineare i riferimenti al percorso reale odierno (worldsmelt-ai-production-blueprint-v2/...) finche la migrazione non e fatta, poi a docs/ai-production/ quando il contenuto verra effettivamente spostato.
- **Risoluzione**: Correggere subito le 3 (4 con ml/run_policy.yaml) citazioni di percorso puntandole ai file reali sotto worldsmelt-ai-production-blueprint-v2/; alla migrazione verso docs/ai-production/ aggiornare di nuovo le stesse citazioni in un solo passaggio, altrimenti path-orchestrator (step 3) e l'appendice codex (step 1) falliscono al primo step obbligatorio.

## DOC-CONFLICT-025 — Incoerenza interna nel pacchetto ai-production sul giudice degli specialisti non-codice

- **Stato**: aperta — verificato: `worldsmelt-path-orchestrator.md` restringe ancora il gate a 'Ogni modifica di codice passa da melting-verifier', senza citare output ML/UI/audio/curation; non corretto, fuori perimetro di questo pacchetto (file in `.claude/agents/`).
- **Fonti**: `CLAUDE.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-path-orchestrator.md`, `worldsmelt-ai-production-blueprint-v2/18-AGENT-ORCHESTRATION.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-asset-curator.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-ml-pipeline-architect.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-ui-systems-designer.md`, `worldsmelt-ai-production-blueprint-v2/agent-config/claude/agents/worldsmelt-audio-systems-designer.md`
- **Rischio**: basso
- **Evidenza**: CLAUDE.md: la scala e universale, unica eccezione melting-content-designer. Correzione all'analista: la lacuna NON e completa. 18-AGENT-ORCHESTRATION.md:123-127 estende gia la scala a TUTTI gli specialisti ("Usa la scala gia definita nel root CLAUDE.md; verifier un gradino sopra; escalation dopo bocciatura o due fallimenti") e :39 mostra il flusso "specialista -> verifier -> report". Il difetto residuo e piu ristretto: path-orchestrator.md:35-36 restringe il gate al solo "Ogni modifica DI CODICE passa da melting-verifier", non citando output ML/UI/audio/curation. I 6+1 agenti (incluso decision-facilitator, non elencato dall'analista) hanno model fisso senza rung esplicito nel proprio file.
- **Raccomandazione**: Non e conflitto tra fonti approvate ma incoerenza interna al pacchetto ai-production (non ancora migrato/approvato): 18-AGENT-ORCHESTRATION.md gia dice il giusto, path-orchestrator.md step 9 lo contraddice per restrizione. CLAUDE.md resta fonte di principio. Correggere path-orchestrator.md perche il gate del verifier valga per OGNI output di specialista, non solo per il codice.
- **Risoluzione**: Riformulare path-orchestrator.md step 9 in "Ogni output di specialista (codice, ML, UI, audio, curation) passa da un giudice un gradino sopra secondo root CLAUDE.md", allineandolo a 18-AGENT-ORCHESTRATION.md:123-127; opzionalmente esplicitare in ciascun file agente il gradino/giudice (specialisti sonnet -> opus; librarian haiku -> sonnet). Nessun output promosso senza verdetto.

## DOC-CONFLICT-026 — Commit diretto su main (CLAUDE.md) vs PR/branch raccomandato da Q-AG-002 nel pacchetto ai-production

- **Stato**: applicata — Q-AG-002 chiusa (CLAUDE.md/18-AGENT-ORCHESTRATION/DEC-164 secondo `open-questions.md`); commit/push diretto su main resta la policy attiva.
- **Fonti**: `CLAUDE.md`, `worldsmelt-ai-production-blueprint-v2/19-DECISION-QUESTIONNAIRE.md (Q-AG-002, r.273-287)`, `.claude/agents/melting-implementer.md`
- **Rischio**: medio
- **Evidenza**: Verificato: CLAUDE.md "Ogni cambiamento verificato va committato e pushato subito su main" (coerente con memoria push-direct-to-main del 16/07). Q-AG-002 (BLOCKING, non risolta) raccomanda "C per task ML/UI/audio; A per piccoli fix". melting-implementer.md:19-20 "Non committare mai ... Il commit lo fa il chiamante dopo verifica": ruoli distinti (implementer vs chiamante-che-committa), gia coerenti, non in conflitto tra loro.
- **Raccomandazione**: CLAUDE.md (istruzione di progetto attiva, rango 1 operativo) vince su Q-AG-002, che e una domanda in stato draft in un pacchetto ai-production non approvato. La presunta contraddizione con "l'implementer non committa" e una lettura imprecisa dell'analista dell'orchestratore: e la corretta separazione di ruoli.
- **Risoluzione**: Chiudere Q-AG-002 riaffermando la policy esistente (nessun PR/branch: commit/push diretto su main da chi orchestra, dopo verdetto APPROVA del giudice di gradino appropriato), rimandando a CLAUDE.md. Un eventuale futuro PR/branch per task ML/UI/audio ad alto rischio va aperto come nuova decisione esplicita, non dedotto da un questionario non approvato. Rischio abbassato da 'alto' a 'medio': la gerarchia risolve pulitamente, il residuo e solo il rischio che il questionario draft fuorvii gli agenti.

## DOC-CONFLICT-027 — DSL tipizzata a tre livelli (percorso principale) vs Lua sandboxed generato

- **Stato**: applicata — `docs/engineering/adr/ADR-003-lua-sandbox-non-dsl.md` creato; `roguelike-ai-appunti/02` e la sezione architetturale di `08` archiviati in `docs/archive/legacy-notes/roguelike-ai-appunti/`.
- **Fonti**: `roguelike-ai-appunti/02-architettura-sinergie-dsl.md`, `roguelike-ai-appunti/08-roadmap-vertical-slice.md`, `roguelike-ai-appunti/piano-roguelike-ai.md`, `game-design-knowledge-base/docs/game-design/06-ai-content-generation-model.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-037)`
- **Decisioni**: DEC-037
- **Rischio**: alto
- **Evidenza**: Verificato. 02 §'Ruolo futuro di Lua' (righe 184-193): 'vertical slice: solo DSL tipizzata; laboratorio interno: Qwen puo tradurre... in Lua'. 08 pivota 'da codice Lua generato come percorso principale a DSL tipizzata'. Ma piano-roguelike-ai.md §2 (stessa data 16/07) dice gia il contrario: 'Lua e la tua decisione tecnica piu forte'. DEC-037 (approved, 18/07): trait/colpi sono comportamenti Lua generati e validati in sandbox; alternativa scartata = solo parametrico. Confermato l'Effect Graph/DSL tipizzata come percorso principale non e piu l'architettura corrente.
- **Raccomandazione**: Vince il decision-log (DEC-037) e la decisione tecnica 16/07 gia in memoria e nel codice (src/script sandbox Lua, tools/melting-gen/gen_lua.c, docs/references/design-sinergie.md, docs/superpowers/specs/2026-07-13-lua-sandbox-design.md). Gerarchia rispettata. Nota: e una contraddizione INTERNA agli appunti (02 vs piano), gia risolta a favore di Lua.
- **Risoluzione**: Archiviare 02-architettura-sinergie-dsl.md e la sezione architetturale di 08 in docs/archive/superseded/roguelike-ai-appunti/ con cappello che rimanda a DEC-037 e ai doc Lua correnti. Decisione gia presa e coerente col codice: nessuna nuova DEC.

## DOC-CONFLICT-028 — AI Director con difficolta adattiva alle prestazioni vs 'Difficolta unica' (DEC-038)

- **Stato**: applicata (DEC-112, 22/07) — il director-per-stile è parcheggiato fra le idee future di DEC-018; DEC-038 (difficoltà unica) resta intatta.
- **Fonti**: `roguelike-ai-appunti/04-ai-director-adattamento.md`, `roguelike-ai-appunti/01-visione-e-confini.md`, `game-design-knowledge-base/docs/game-design/07-difficulty-and-progression.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-038)`
- **Decisioni**: DEC-038
- **Rischio**: alto  |  **needs-human**
- **Evidenza**: Verificato. 04 riga 82: 'La difficolta puo usare bande discrete e dichiarate'; §'PerformanceProfile' (righe 79-82) separa stile da abilita ma prevede un director su telemetria. 01 pilastro 4 (riga 37-39) ribadisce l'adattamento. DEC-038 (approved): 'nessun livello di difficolta selezionabile; la curva e quella dei 5 piani, uguale per tutti', alternativa scartata esplicitamente = 'difficolta adattiva basata sulle prestazioni'. La parte difficolta-adattiva e in diretto conflitto e persa dalla KB; il PlayerStyleProfile (peso famiglie meccaniche, contenuti 50/30/20 che valorizzano/sfidano/sorprendono lo stile) non e ne coperto ne escluso.
- **Raccomandazione**: Vince DEC-038 (gerarchia 1) sulla difficolta: curva unica e uguale per tutti, classifiche confrontabili. Scartare PerformanceProfile/bande di difficolta. Il director-per-STILE (non per abilita) resta contenuto unico non catturato dalla KB e mai discusso: e un intero sistema su telemetria assente dalla KB.
- **Risoluzione**: Segnalare al design owner se l'adattamento per STILE (50/30/20, non per difficolta) e ancora desiderato come idea futura (estendere DEC-018 / open-questions.md) o va scartato: tocca il confine di una decisione approvata (DEC-038) e introdurrebbe un director su telemetria mai deciso.
- **ARBITRATO (Fable)**: DEC-038 resta intatta (difficolta' unica). Gli appunti vanno in archive verbatim. Il 'director per stile' (mai per abilita') e' registrato come open question: estendere DEC-018 o scartare.

## DOC-CONFLICT-029 — Linking in-process di llama.cpp/stable-diffusion.cpp vs architettura a processi esterni (MANCANTE nell'analisi)

- **Stato**: applicata — `docs/engineering/adr/ADR-002-generatori-processi-esterni.md` creato; `piano-roguelike-ai.md` archiviato in `docs/archive/legacy-notes/roguelike-ai-appunti/piano-roguelike-ai.md`.
- **Fonti**: `roguelike-ai-appunti/piano-roguelike-ai.md`, `AGENTS.md`, `Makefile`, `src/gen (ciclo di vita processo melting-gen)`
- **Rischio**: medio
- **Evidenza**: Conflitto sfuggito all'analista, trovato controllando fonti non citate. piano-roguelike-ai.md §2 (riga 28): 'llama.cpp + stable-diffusion.cpp linkati in-process... in un gioco Steam un processo unico evita prompt del firewall, conflitti di porta e processi orfani'; il sidecar (processo esterno) e trattato come mero fallback per crash-isolation. L'architettura REALE e opposta: AGENTS.md riga 20 e Makefile righe 54-78 mostrano bin/melting-gen e bin/melting-sprites come binari separati; 'il binario del gioco... linka nessuno dei tre'; src/gen gestisce melting-gen come PROCESSO esterno (avvio, timeout, annullamento). Gerarchia: doc engineering verificata contro il codice (liv. 4) batte il piano (liv. 6).
- **Raccomandazione**: Vince l'architettura corrente a processi esterni (codice + AGENTS.md). La raccomandazione del piano di linkare i modelli in-process e superata: non e piu il design corrente ne va riproposta. Non serve decisione umana: gia risolta e implementata.
- **Risoluzione**: Nell'archiviazione del piano, nota di cappello: la raccomandazione 'linkati in-process' e superata dall'architettura a processi esterni (bin/melting-gen, bin/melting-sprites separati; il gioco non linka mai llama.cpp/SD), rif. AGENTS.md e Makefile. Materiale da archiviare in docs/archive/superseded/, non da migrare come architettura corrente.

## DOC-CONFLICT-030 — Livelli hardware S/A/B/C con configurazioni di modello diverse vs scelta binaria del primo avvio (DEC-070)

- **Stato**: applicata (DEC-111, 22/07) — scelta binaria confermata; la tabella dei tier S/A/B/C resta memoria storica in `docs/archive/legacy-notes/roguelike-ai-appunti/`.
- **Fonti**: `roguelike-ai-appunti/piano-roguelike-ai.md`, `game-design-knowledge-base/docs/game-design/systems/floor-zero.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-070)`
- **Decisioni**: DEC-070, DEC-086
- **Rischio**: alto  |  **needs-human**
- **Evidenza**: Verificato. piano §4 (righe 100-105): tabella Tier S/A/B/C ciascuno con 'Configurazione' diversa (co-residenza; sequenziale; logic-only con sprite da libreria; non supportato con pool di ~500 bundle). DEC-070 (approved): 'un solo set di modelli, non esistono alternative... Nessun tier intermedio di download', alternativa scartata = 'tier intermedi di download/qualita dei modelli'; scelta binaria completo/solo-curato. Il tier misurato dal benchmark (che esiste, DEC-070/DEC-086) contraddice esplicitamente la scelta binaria.
- **Raccomandazione**: Vince il decision-log (DEC-070). Il fallback granulare per hardware debole (es. 'logic-only' senza sprite generati; pool di bundle pre-generati) e una modifica a una decisione approvata. Va segnalato perche tocca fallback e requisito hardware.
- **Risoluzione**: Portare al proprietario: il benchmark propone solo scelta binaria; se si vuole un fallback granulare per hardware debole e una modifica a DEC-070, da registrare come nuova DEC se accolta, altrimenti archiviare la tabella dei tier come idea scartata. Caso hardware/fallback: escalation legittima.
- **ARBITRATO (Fable)**: DEC-070 resta intatta (scelta binaria del primo avvio). La tabella tier S/A/B/C resta negli appunti archiviati. Open question sul fallback granulare per hardware debole.

## DOC-CONFLICT-031 — Rosa di personaggi base: 'nessun personaggio base aggiuntivo in v1' (superato da DEC-030)

- **Stato**: applicata — nessuna azione di design necessaria; `01`/`08` (oggi in `docs/archive/legacy-notes/roguelike-ai-appunti/`) restano superati da DEC-030/080/108.
- **Fonti**: `roguelike-ai-appunti/01-visione-e-confini.md`, `roguelike-ai-appunti/08-roadmap-vertical-slice.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-014, DEC-030, DEC-080, DEC-108)`
- **Decisioni**: DEC-014, DEC-030
- **Rischio**: basso
- **Evidenza**: Verificato. 01 riga 88 'Cosa non includere nella prima versione': 'piu personaggi base'. DEC-030 (18/07): 'i personaggi base sono una piccola rosa di 2-3 personaggi FISSI e curati', annotata come integrazione di DEC-014; ulteriormente dettagliata da DEC-080 (nomi/ruoli) e DEC-108 (sblocchi: Wayfinder da subito, Ashblade/Bulwark presto). La nota degli appunti (16/07) e precedente e superata.
- **Raccomandazione**: Vince il decision-log: rosa di 2-3 personaggi base gia approvata e piu recente (18/07). Non e divergenza aperta ma snapshot superata dagli appunti stessi; la KB ha gia gestito l'evoluzione internamente.
- **Risoluzione**: Nessuna azione di design. Segnare i due paragrafi come superseded all'archiviazione (nota a pie di pagina che rimanda a DEC-030/080/108).

## DOC-CONFLICT-032 — Modalita multiplayer 'Crossed AI Duel' e 'Model Battle' non presenti nella tassonomia approvata

- **Stato**: applicata — `07-multiplayer-classifiche.md` archiviato in `docs/archive/legacy-notes/roguelike-ai-appunti/`; struttura menu reale resta DEC-021/DEC-062; Crossed AI Duel/Model Battle non promosse (verificato: assenti da `open-questions.md`).
- **Fonti**: `roguelike-ai-appunti/07-multiplayer-classifiche.md`, `game-design-knowledge-base/docs/game-design/08-multiplayer-and-competition.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-016, DEC-018, DEC-021, DEC-062)`
- **Decisioni**: DEC-016, DEC-018, DEC-021, DEC-062
- **Rischio**: basso
- **Evidenza**: Verificato. 07 §Modalita (righe 11-17) elenca 5 modalita: Mirror Race (= Classificata stesso seed), Crossed AI Duel (mondi A/B costruiti dal profilo di ciascuno, giocati incrociati), Chaos Race (= 'modalita caos' gia parcheggiata in DEC-018), Daily/Weekly (= Daily DEC-062), Model Battle (bundle da modelli/versioni diverse). Il decision-log fissa 2 assi x 3 istanze (Leggera/Classificata x stesso seed/seed diversi/Daily, DEC-021+DEC-062); Crossed AI Duel e Model Battle non compaiono ne tra le idee future di DEC-018.
- **Raccomandazione**: Vince il decision-log per la struttura del menu (implementata, DEC-021/062). Crossed AI Duel e Model Battle sono contenuto unico non contraddittorio (non escluso) ma non catturato: idee, non design corrente. Chaos Race e gia coperta come idea parcheggiata (DEC-018).
- **Risoluzione**: Se ancora interessanti, aggiungere Crossed AI Duel e Model Battle alle idee future (estendere DEC-018) o a open-questions.md citando la fonte; altrimenti archiviare in docs/archive/legacy-notes/ segnalando che la struttura corrente e DEC-021/DEC-062.

## DOC-CONFLICT-033 — Dataset, licenze e training GPU (05, 06, 10): contenuto di produzione IA non migrato

- **Stato**: applicata — `05`/`06`/`10` di `roguelike-ai-appunti` archiviati; materiale di produzione confrontato con `docs/ai-production/04-DATASET-LICENZE.md` e `03-PIANO-LORA.md`, ulteriormente aggiornati da DEC-148.
- **Fonti**: `roguelike-ai-appunti/05-dataset-e-licenze.md`, `roguelike-ai-appunti/06-training-hardware-costi.md`, `roguelike-ai-appunti/10-fonti-verificate.md`, `worldsmelt-ai-production-blueprint-v2/04-DATASET-LICENZE.md`, `worldsmelt-ai-production-blueprint-v2/03-PIANO-LORA.md`
- **Rischio**: basso
- **Evidenza**: Verificato. Nessuna DEC ne doc di game-design tratta fonti dataset, licenze CC0/OpenRAIL, costi GPU o pipeline di training: correttamente fuori perimetro (la KB e docs/game-design, non produzione). I tre file restano non coperti dalla KB. Esistono gia in repo, piu recenti (21/07): worldsmelt-ai-production-blueprint-v2/04-DATASET-LICENZE.md e 03-PIANO-LORA.md (e piano-roguelike-ai.md §5 copre le licenze in dettaglio).
- **Raccomandazione**: Non e conflitto di design (la KB non doveva trattare questo materiale). Materiale valido di produzione mai migrato secondo docs/ai-production. Da confrontare col blueprint-v2 gia presente e piu aggiornato.
- **Risoluzione**: Confrontare con blueprint-v2/04-DATASET-LICENZE.md e /03-PIANO-LORA.md: se coprono lo stesso terreno con dati aggiornati, archiviare i tre file in docs/archive/legacy-notes/; altrimenti trasferire le parti ancora valide (registro asset, fonti CC0, checklist noleggio GPU) in docs/ai-production/.

## DOC-CONFLICT-034 — Pipeline visiva del personaggio (Visual Dominance Budget, rig a socket, LoRA di ruolo): contenuto tecnico non migrato

- **Stato**: applicata — `03-personaggio-grafica-60fps.md` archiviato; `docs/ai-production/08-PIPELINE-SPRITE-ANIMAZIONI.md` e `09-NEMICI-BODY-PLAN-RIG.md` coprono oggi il terreno tecnico.
- **Fonti**: `roguelike-ai-appunti/03-personaggio-grafica-60fps.md`, `worldsmelt-ai-production-blueprint-v2/08-PIPELINE-SPRITE-ANIMAZIONI.md`, `worldsmelt-ai-production-blueprint-v2/09-NEMICI-BODY-PLAN-RIG.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-046, DEC-049)`
- **Decisioni**: DEC-046, DEC-049
- **Rischio**: basso
- **Evidenza**: Verificato. DEC-046 fissa la pixel art come linguaggio canonico (anche UI); DEC-049 fissa che i personaggi base hanno sprite curati a mano e il generato usa la pipeline sprite esistente. Entrambe restano a livello di principio e rimandano i valori numerici a 'default dell'implementazione', senza definire budget di dominanza (100 punti, canali silhouette/appendici/superficie/palette/aura) ne pipeline SD+LoRA/rig a socket di 03. La KB delega correttamente questi dettagli. Esistono gia blueprint-v2/08 e /09 piu recenti.
- **Raccomandazione**: Non e contraddizione: la KB delega fuori dal proprio perimetro. Il materiale (budget canali, pipeline SD, LoRA di ruolo) e potenzialmente ancora valido come riferimento tecnico/di produzione, non come design.
- **Risoluzione**: Confrontare con blueprint-v2/08-PIPELINE-SPRITE-ANIMAZIONI.md e /09-NEMICI-BODY-PLAN-RIG.md: se sovrapposti, archiviare 03 in docs/archive/legacy-notes/; altrimenti estrarre il Visual Dominance Budget come riferimento in docs/engineering/ (contratto tecnico su come risolvere i conflitti visivi).

## DOC-CONFLICT-035 — Roadmap a fasi (Fase 0-7) con nomenclatura e architettura superate

- **Stato**: applicata — `08-roadmap-vertical-slice.md` e la §8 di `piano-roguelike-ai.md` archiviati; la sequenza reale resta tracciata da CLAUDE.md/git log (milestone M1-M8, poi Fase 1 completa).
- **Fonti**: `roguelike-ai-appunti/08-roadmap-vertical-slice.md`, `roguelike-ai-appunti/piano-roguelike-ai.md`, `CLAUDE.md (repo principale)`
- **Decisioni**: DEC-037
- **Rischio**: basso
- **Evidenza**: Verificato. 08 descrive Fase 0-7 (benchmark, Effect Graph senza AI, Qwen propone, fenotipo visivo, Style LoRA, AI Director, RunBundle/replay, playtest); piano-roguelike-ai.md §8 ha una sua roadmap Fase 0-4. CLAUDE.md: 'Fase 1 di implementazione COMPLETA (M1-M8 su main)', nomenclatura a milestone M1-M8 e senza Effect Graph (superato dal conflitto 1). Piano attivo reale tracciato altrove (git log, milestone).
- **Raccomandazione**: Il piano attivo reale sono le milestone M1-M8 (git log, CLAUDE.md). Le roadmap a Fasi negli appunti sono piani storici non aggiornati con l'architettura Lua e la reale sequenza implementata.
- **Risoluzione**: Spostare 08-roadmap-vertical-slice.md (e la §8 di piano-roguelike-ai.md) in docs/archive/historical-plans/ con nota che rimanda alla sequenza reale M1-M8 (git log, CLAUDE.md) invece delle Fasi basate su Effect Graph/DSL.

## DOC-CONFLICT-036 — RunBundle/replay: dettagli tecnici (hash SHA-256, formato) allineati nel principio ma mai formalizzati come contratto/ADR

- **Stato**: aperta — nessun ADR RunBundle/replay creato (verificato: `docs/engineering/adr/` ha solo ADR-001/002/003, nessuno sul formato RunBundle); azione ancora da fare.
- **Fonti**: `roguelike-ai-appunti/07-multiplayer-classifiche.md`, `roguelike-ai-appunti/piano-roguelike-ai.md`, `game-design-knowledge-base/docs/game-design/systems/run-manifest-and-reproducibility.md`, `worldsmelt-ai-production-blueprint-v2/07-ARCHITETTURA-RUNTIME.md`, `game-design-knowledge-base/docs/game-design/governance/decision-log.md (DEC-066, DEC-077)`
- **Decisioni**: DEC-066, DEC-077
- **Rischio**: basso
- **Evidenza**: Verificato. 07 (§Contenuto RunBundle competitivo/Replay, righe 70-98) e piano §2 descrivono un formato dettagliato (versioni, hash SHA-256, provenienza modello/LoRA, replay con hash di stato a intervalli). run-manifest-and-reproducibility.md conferma il concetto (RunBundle, verifica d'integrita, DEC-066/077) ma dichiara i dettagli tecnici fuori scope del documento di design. docs/engineering/adr/ e VUOTA (verificato): nessun ADR RunBundle esiste. Il blueprint-v2/07-ARCHITETTURA-RUNTIME.md menziona gia RunBundle e potrebbe coprirlo.
- **Raccomandazione**: Nessuna contraddizione: allineamento concettuale, la KB delega correttamente il dettaglio tecnico. Il materiale e un buon punto di partenza per un ADR oggi assente nel repo.
- **Risoluzione**: Verificare prima se blueprint-v2/07-ARCHITETTURA-RUNTIME.md formalizza gia il formato; altrimenti usare le sezioni RunBundle/Replay di 07 e la §2 del piano come base per un ADR in docs/engineering/adr/ (formato, hash, replay), poi archiviare gli originali in docs/archive/legacy-notes/.

## DOC-CONFLICT-037 — Piano linux-local-llm: 81 step ancora "[ ]" ma fase 0-1 e roadmap 2-5 gia' implementate

- **Stato**: applicata — piano spostato in `docs/archive/historical-plans/2026-07-13-linux-local-llm.md`. Residuo cosmetico: le 82 righe `- [ ]` non sono state marcate `[x]` né annotate 'ESEGUITO ED ESTESO'; la collocazione in historical-plans/ segnala comunque lo stato storico.
- **Fonti**: `docs/superpowers/plans/2026-07-13-linux-local-llm.md`, `Makefile`, `tools/melting-gen/`, `scripts/test-gen.sh`, `scripts/benchmark.sh`, `llm/run_content.mjs`
- **Rischio**: medio
- **Evidenza**: Verificato: il piano ha 81 righe "- [ ]" e 0 "- [x]". Makefile espone gia' gen/sprites/test-gen/test-llm/test-sprites/benchmark. tools/melting-gen contiene gen_lua.c, gen_novelty.c, gen_inspire.c, gen_corpus.c, propose.gbnf, character.gbnf, mai citati nel piano. scripts/test-gen.sh = 986 righe (contro le ~30 dello Step originale). docs/plans/{active,completed,cancelled}/ esistono e sono vuote.
- **Raccomandazione**: Il codice (gerarchia punto 4, verificato) vince sullo stato dichiarato dal piano (punto 6). Spostare il piano in docs/plans/completed/, marcare gli step come fatti e aggiungere una nota che rimanda ad AGENTS.md/Makefile per lo stato reale, oggi oltre lo scope originale. Confermato: needs_human=false, si formalizza solo lo stato.
- **Risoluzione**: Nota in testa "ESEGUITO ED ESTESO - vedi AGENTS.md", flag [x] sugli step, spostamento in docs/plans/completed/2026-07-13-linux-local-llm.md. Nessuna decisione di design cambia.

## DOC-CONFLICT-038 — Vincoli hardware duri della spec (Vulkan, mai ROCm, mai flash-attention) non promossi a ADR

- **Stato**: applicata — `docs/engineering/adr/ADR-001-backend-vulkan-only.md` creato (Vulkan, mai ROCm, mai flash-attention, motivazione gfx1010).
- **Fonti**: `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`, `docs/engineering/adr/ (vuota)`, `scripts/setup-deps.sh`
- **Rischio**: alto
- **Evidenza**: Verificato: spec §2 righe 49-55 fissa RX 5600 XT RDNA1/gfx1010, "Backend GPU: Vulkan, mai ROCm", "Niente flash-attention su RDNA1". setup-deps.sh usa -DGGML_VULKAN=ON e -DSD_VULKAN=ON, nessun flag ROCm/flash-attn: vincoli rispettati dal codice. docs/engineering/adr/ e' vuota: nessun ADR li cattura. Stato spec: "approvata a voce sezione per sezione, in attesa di revisione finale".
- **Raccomandazione**: Un ADR (punto 3) batte la spec storica (punto 6) e il silenzio. Estrarre in docs/engineering/adr/ADR-001-backend-vulkan-only.md la decisione (Vulkan, mai ROCm, mai flash-attention, motivazione gfx1010) citando §2, prima di archiviare la spec, cosi' un futuro agente non la reintroduce inseguendo prestazioni. needs_human=false confermato: la scelta hardware e' gia' approvata dall'autore a voce ed e' in produzione; l'ADR la registra, non la decide.
- **Risoluzione**: Creare l'ADR con testo derivato da §2 (citando la fonte), poi spostare la spec in docs/archive/superseded/ con Stato "implementata, formalizzata in ADR-001". Solo formalizzazione di una scelta gia' in produzione.

## DOC-CONFLICT-039 — Regola "fonte di verita' = llm/run_content.mjs" del piano falsa per le funzionalita' post-fase-1

- **Stato**: applicata — lo scoping è implicito nello stato attuale: Lua/novelty/inspire/corpus hanno oggi come fonte il decision-log + il codice C, nessuna pretesa di equivalente in `llm/run_content.mjs`.
- **Fonti**: `docs/superpowers/plans/2026-07-13-linux-local-llm.md`, `llm/run_content.mjs`, `tools/melting-gen/gen_lua.c`, `tools/melting-gen/gen_novelty.c`, `tools/melting-gen/gen_inspire.c`, `tools/melting-gen/gen_corpus.c`
- **Rischio**: basso
- **Evidenza**: Verificato: piano riga 3045 "la fonte di verita' e' llm/run_content.mjs - il C deve replicare il Node". Ma grep su run_content.mjs restituisce 0 occorrenze di lua/novelty/inspire/corpus: queste funzionalita' (script Lua, punteggio novita', corpus d'ispirazione) non hanno alcun equivalente Node da replicare. La regola nel piano e' comunque gia' scoping-limitata al caso "test con valore atteso sbagliato" (Task 1-8).
- **Raccomandazione**: La regola resta valida solo per i campi ereditati dal Node (theme/boss/palette/mini-VM, Task 1-8). Per Lua/novelty/inspire/corpus la fonte di verita' e' il decision-log/KB (punto 1) e il codice C (punto 4). Annotare lo scoping in fase di archiviazione. needs_human=false confermato: nessun cambio di design.
- **Risoluzione**: In archiviazione aggiungere alla nota: "valida solo per i campi ereditati dal Node; Lua, novelty, inspire e corpus non hanno equivalente Node e seguono decision-log + codice".

## DOC-CONFLICT-040 — Spec di design citate come fonte vincolante nel codice hanno ancora header "proposta / da approvare"

- **Stato**: applicata — verificato: le quattro spec sono oggi in `docs/engineering/specs/` con front matter `status` corretto (lua-sandbox e local-sprites: implemented; items-synergy-vision e pools-rarity-design: superseded); il campo prosa 'Stato:' nel corpo non è stato riscritto ovunque (residuo minore).
- **Fonti**: `docs/superpowers/specs/2026-07-13-lua-sandbox-design.md`, `docs/superpowers/specs/2026-07-13-local-sprites-design.md`, `docs/superpowers/specs/2026-07-13-items-synergy-vision.md`, `docs/superpowers/specs/2026-07-13-pools-rarity-design.md`, `src/script/script_sandbox.h`, `src/render/item_layers.h`, `src/gameplay/item_traits.h`, `tools/melting-sprites/main.c`
- **Decisioni**: DEC-037
- **Rischio**: medio
- **Evidenza**: Verificato: ~30 riferimenti nel codice (script_sandbox.h sez.2-9, item_layers.h, item_traits.h, gen_util.c, tools/melting-sprites/main.c) citano queste spec per percorso e sezione come fonte architetturale in produzione, ma gli header dicono ancora "Stato: da rivedere e approvare dall'autore" (lua-sandbox), "da approvare dall'autore" (sprites), "proposta - da rivedere/correggere" (items-synergy, pools-rarity). L'analista aveva omesso pools-rarity-design, che ha lo stesso difetto. feedback-roadmap invece e' gia' "ESEGUITA", nessun conflitto.
- **Raccomandazione**: CORREZIONE alla recommendation originale: la destinazione NON e' uniformemente docs/engineering/ (violerebbe le regole di migrazione). Instradare per contenuto: lua-sandbox (architettura sandbox) -> docs/engineering/; local-sprites (modello SD/training) -> docs/ai-production/; items-synergy-vision e pools-rarity-design (visione/design di gioco) -> docs/design/. Prima correggere il campo Stato a "approvata/implementata" con riferimento ai file che le citano. needs_human=false confermato.
- **Risoluzione**: Per ciascuna spec: aggiornare Stato, spostare nella cartella di dominio corretta, e aggiornare TUTTI i ~30 percorsi citati nei commenti C nello stesso commit per non rompere i riferimenti.

## DOC-CONFLICT-041 — Fase 5 (benchmark al primo avvio + scelta automatica del modello) NON implementata: gap di design da preservare

- **Stato**: superata (DEC-110, 22/07) — il benchmark automatico al primo avvio è stato scartato, non implementato: nessun tier di qualità automatico. Piano annullato: `docs/plans/cancelled/benchmark-primo-avvio.md` (verificato esistente).
- **Fonti**: `docs/superpowers/specs/2026-07-13-local-llm-linux-design.md`, `src/app/app.c`
- **Rischio**: basso
- **Evidenza**: Verificato: spec riga 44 "Fase 5: benchmark al primo avvio e scelta automatica del modello". app.c righe 1277-1284: "NIENTE esecuzione automatica del benchmark qui (v1): si legge solo logs/benchmark.txt se gia' scritto da 'make benchmark'". Richiede comando manuale, non gira al primo avvio: gap reale e deliberato ("v1").
- **Raccomandazione**: NON e' un doc da archiviare come "fatto": e' design ancora aperto (differito v1). Preservarlo esplicitamente come voce attiva in docs/plans/active/, distinto dalla fase 0-1 completa, cosi' non viene silenziato dalla chiusura generale del piano. needs_human=false confermato: e' tracciamento di un gap, non un cambio di decisione.
- **Risoluzione**: Creare docs/plans/active/benchmark-auto-first-run.md che isola il gap (benchmark automatico al primo avvio, oggi manuale via 'make benchmark'), linkando spec e commento app.c come contesto.

## DOC-CONFLICT-042 — AGGIUNTO: riferimento a spec inesistente in src/core/room_layout.h (fonte architetturale dangling)

- **Stato**: applicata — verificato: `src/core/room_layout.h` non cita più `step-3c-rooms.md`; il commento in testa rimanda oggi a `docs/design/systems/rooms-and-floor-generation.md` e dichiara esplicitamente che la spec dedicata non fu mai scritta.
- **Fonti**: `src/core/room_layout.h`, `docs/superpowers/specs/`
- **Rischio**: medio
- **Evidenza**: Missato dall'analista. src/core/room_layout.h riga 6 cita come fonte architetturale in produzione "docs/superpowers/specs/2026-07-16-step-3c-rooms.md", ma quel file NON esiste da nessuna parte nel repo (find su *step-3c* = vuoto). Il riferimento e' gia' rotto oggi, prima di qualsiasi migrazione. Anche step-3b-enemies.md ha Stato "in esecuzione", lievemente stale ma il file esiste.
- **Raccomandazione**: Un riferimento architetturale nel codice a una spec inesistente e' un conflitto reale (codice, punto 4, punta a nulla). Prima di archiviare/migrare la cartella specs, individuare la spec reale delle stanze (rinominata o mai scritta) o correggere/rimuovere la citazione in room_layout.h. needs_human=false: correzione documentale, nessun cambio di design.
- **Risoluzione**: Verificare git history per step-3c-rooms; se assente, aggiornare la citazione in room_layout.h al doc reale (o al decision-log/KB) e ripulire eventuali altri riferimenti dangling durante la migrazione.

## DOC-CONFLICT-043 — "Nessun tier intermedio / nessun livello di qualità" (DEC-070) vs il preset lowspec automatico (1.5B + sprite 256px)

- **Stato**: applicata (DEC-110, 22/07) — il preset lowspec è stato rimosso dal codice; i requisiti minimi sono oggi i modelli di riferimento (poi espressi in numeri da DEC-142).
- **Fonti**: `game-design-knowledge-base/docs/game-design/governance/decision-log.md:724-725`, `game-design-knowledge-base/docs/game-design/systems/floor-zero.md:189-190`, `scripts/benchmark.sh:65-91`, `src/app/app.c:66-70,151-152,228-233`
- **Decisioni**: DEC-070, DEC-047, DEC-086
- **Rischio**: alto  |  **needs-human**
- **Evidenza**: DEC-070 e floor-zero.md:189-190 sono espliciti: "un solo set di modelli; non esistono alternative o livelli di qualità tra cui scegliere"; l'alternativa scartata è "tier intermedi di download/qualità dei modelli". Ma benchmark.sh:65-74 calcola tier full/lowspec/unsupported (soglie 12/6 tok/s) e app.c applica in automatico, senza scelta del giocatore, un secondo set: 1.5B al posto del 7B (app.c:151-152, APP_LOW_SPEC_MODEL) E sprite --gen-size 256 al posto di 512 (app.c:228-233). È un terzo stato di qualità tra "completa" (7B/512px) e "solo curato" (nessun modello), proprio ciò che DEC-070 dichiara inesistente. Comportamento già implementato e testato (--bench-preset-test).
- **Raccomandazione**: Per gerarchia il decision-log (priorità 1) vince su codice ed engineering. Non risolvibile in doc dal giudice: o il proprietario rilegge DEC-070 per ammettere il lowspec come dettaglio implementativo interno alla scelta binaria (una riga di chiarimento in decision-log e floor-zero.md), o il lowspec è il tier di qualità scartato e va rimosso (regressione su benchmark.sh e app.c). Nessuna modifica unilaterale al decision-log né al codice.
- **Risoluzione**: Portare al proprietario: (a) lowspec è dettaglio dentro "esperienza completa" — basta una riga in DEC-070/floor-zero.md — oppure (b) è il tier di qualità scartato e va tolto. Nel frattempo annotare l'ambiguità in docs/BENCHMARKS.md e in un futuro docs/engineering/adr/, senza toccare il codice.
- **ARBITRATO (Fable)**: Nessun tocco al codice. Open question: il preset lowspec automatico (1.5B+256px, mai offerto come scelta al giocatore) e' dettaglio implementativo dentro DEC-070 (aggiungere una riga di chiarimento) o tier da rimuovere? Ambiguita' annotata in docs/engineering/benchmarks.md.

## DOC-CONFLICT-044 — "tok/s" indica due misure incompatibili (make test-llm vs --bench) senza nota di disambiguazione

- **Stato**: applicata — `docs/engineering/benchmarks.md` (già migrato) ha oggi la sezione 'Disambiguazione: due "tok/s" diversi nel repo'.
- **Fonti**: `docs/BENCHMARKS.md:4,11-20`, `tools/melting-gen/main.c:336-408`, `scripts/benchmark.sh:56-74`, `logs/benchmark.txt:2`
- **Rischio**: medio
- **Evidenza**: BENCHMARKS.md:4 usa `make test-llm`: generazione reale ~1321 token JSON+Lua con grammatica GBNF; la tabella dà 28.1 tok/s per 7B ngl=99 (default). RunBench in main.c:336-408 usa un prompt fisso di fiaba, 128 token (GEN_BENCH_TOKENS), nessuna grammatica, seed 20260717u; logs/benchmark.txt (stesso 7B default) riporta tokS=42.29, ~+50%. Le soglie del tier in benchmark.sh (12/6) vivono sulla scala --bench, non su quella della tabella: nessun documento avvisa che i due numeri non sono comparabili, rischio di ricalibrare le soglie sui numeri sbagliati.
- **Raccomandazione**: docs/BENCHMARKS.md (engineering verificata, priorità 4) va aggiornato per primo con una sezione che spieghi che il numero della tabella (make test-llm, con grammatica) e il tokS di make benchmark/logs/benchmark.txt (--bench, prompt corto senza grammatica) misurano cose diverse. Nessuna fonte perde: serve solo la nota di raccordo in entrambi i posti. Nessun cambio di soglie o codice.
- **Risoluzione**: Aggiungere in docs/BENCHMARKS.md un paragrafo "tok/s qui vs tok/s di make benchmark" con rimando a RunBench (main.c:360); aggiungere lo stesso rimando come commento in scripts/benchmark.sh vicino alle soglie 12/6. Nessuna modifica di soglie o di codice.

## DOC-CONFLICT-045 — BENCHMARKS.md senza data/commit non regge lo standard di riproducibilità (il doc gemello SPRITES-SPIKE.md invece ce l'ha)

- **Stato**: applicata — `docs/engineering/benchmarks.md` ha oggi front matter completo (`last_reviewed`, `last_verified_commit`) e la data/commit della misura nel testo ('13/07/2026, commit b836e96').
- **Fonti**: `docs/BENCHMARKS.md:1-8`, `docs/SPRITES-SPIKE.md:3`, `logs/benchmark.txt:1-5`, `.gitignore:15`, `worldsmelt-ai-production-blueprint-v2/11-PROTOCOLLO-ESPERIMENTI.md:93-110`
- **Rischio**: basso
- **Evidenza**: BENCHMARKS.md dichiara la macchina (Ryzen 5 3600, RX 5600 XT, Ubuntu 26.04) ma nessuna data né commit nel testo (desumibile solo dal commit b836e96, 2026-07-13 09:29). Il documento gemello docs/SPRITES-SPIKE.md:3 — stessa campagna di misure — riporta invece "Misure fatte il 2026-07-13 sulla macchina di riferimento": lo standard esiste già nel repo, BENCHMARKS.md è l'unico a non seguirlo. logs/benchmark.txt ha solo measuredAt=1784275965 (epoch, nessun ID macchina) ed è in logs/ (.gitignore:15), rigenerato ad ogni make benchmark: cache locale, non record storico. Il registro del blueprint 11 (priorità 6, dominio LoRA/sprite) richiede sempre git_commit/dataset_hash/config_hash.
- **Raccomandazione**: docs/BENCHMARKS.md (engineering, priorità 4) va integrato con data e commit, allineandolo al formato già usato in SPRITES-SPIKE.md. logs/benchmark.txt resta volutamente locale/effimero (letto da app.c ad ogni --generate) e non deve diventare un record: va solo detto esplicitamente. Il blueprint 11 resta piano (priorità 6) per un dominio diverso, non sostituisce nulla ma fissa lo standard minimo.
- **Risoluzione**: Aggiungere in cima a docs/BENCHMARKS.md "Misurato il 2026-07-13 al commit b836e96" (formato di SPRITES-SPIKE.md); aggiungere un commento in scripts/benchmark.sh che chiarisce che logs/benchmark.txt è una cache locale rigenerata ad ogni run, non uno storico comparabile fra macchine.

## DOC-CONFLICT-046 — research-pack Fase C (confronto multi-modello) non eseguita: il default 7B in BENCHMARKS.md precede il protocollo proposto (nessuna tabella è però superata)

- **Stato**: superata (DEC-140, 23/07) — la Fase C è stata eseguita: comparison su 11 modelli x 3 seed (`docs/ai-production/experiments/model-comparison-testo-2026-07-23.md`); il default è cambiato dal 7B Coder a Gemma-3-4B-IT Q4 di conseguenza.
- **Fonti**: `worldsmelt-research-pack-2026-07-21/09-benchmark-e-roadmap.md:36-52`, `docs/BENCHMARKS.md:11-20`, `tools/melting-gen/main.c:79-80`
- **Rischio**: basso
- **Evidenza**: research-pack 09 Fase C (righe 36-52) propone di confrontare Qwen2.5-Coder-7B, Qwen3-4B-Instruct-2507 e Phi-4-mini-instruct con un CSV (retries/compile_ok/dryrun_ok/sim_ok/novelty/score) prima di eleggere un modello canonico. BENCHMARKS.md ha già scelto e cablato il 7B Coder come default (main.c:79-80) confrontando solo 1.5B vs 7B della stessa famiglia. Non è un conflitto di numeri: è un piano non ancora eseguito rispetto a uno stato reale già implementato — nessuna tabella attuale risulta superata.
- **Raccomandazione**: Il piano (priorità 6, docs/plans) non override lo stato implementato (priorità 4). BENCHMARKS.md resta valido finché Fase C non viene corsa. All'atto della migrazione di 09-benchmark-e-roadmap.md in docs/plans/active/, annotare che l'esito default-model è già preso e che una Fase C eseguita potrebbe in futuro rendere superata la tabella. Nessuna modifica a BENCHMARKS.md ora.
- **Risoluzione**: Migrare 09-benchmark-e-roadmap.md in docs/plans/active/ con nota di stato "Fase A-B già coperte da RunBundle v1/Lua kernel; Fase C non ancora eseguita, default attuale (7B Coder) resta valido finché non gira il confronto". Nessuna modifica a docs/BENCHMARKS.md.
