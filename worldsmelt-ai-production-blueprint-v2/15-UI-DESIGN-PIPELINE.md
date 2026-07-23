---
id: ai-prod-ui-pipeline
status: proposed
owner: art-ui
last_reviewed: 2026-07-20
summary: "Pipeline UI pixel-art: Penpot come sorgente di design, componenti generati da SD1.5, renderer raylib deterministico e raygui riservato agli strumenti."
---

# Pipeline UI e GUI

## Confine canonico

La knowledge base definisce **cosa** deve mostrare l'interfaccia. Questo documento propone
**come** produrla e implementarla.

Prima di implementare una schermata:

1. leggere `game-design-knowledge-base/docs/game-design/INDEX.md`;
2. leggere il documento UI specifico;
3. verificare `governance/open-questions.md`;
4. non inventare nuove interazioni per adattarsi a un mockup;
5. registrare le nuove decisioni di design nella knowledge base.

## Strategia

```text
knowledge base UI
        ↓
Penpot design system
        ↓
token + componenti + stati
        ↓
asset pixel-art modulari
        ↓
export manifest
        ↓
renderer UI raylib
```

Stable Diffusion non deve generare schermate complete con testo. Genera soltanto elementi
modulari:

- pannelli;
- cornici;
- divisori;
- icone;
- cursori;
- bordi;
- barre;
- ornamenti;
- texture di sfondo;
- stati visivi senza testo.

Testo, layout, focus, ridimensionamento e accessibilità restano deterministici.

## Penpot

Penpot è la proposta come sorgente principale del design system perché:

- supporta componenti e varianti;
- supporta design token;
- può essere self-hosted;
- espone un server MCP ufficiale;
- consente all'agente di leggere e modificare file, componenti, token e layer.

L'adozione di Penpot è una decisione tecnica reversibile. Gli export consumati dal gioco
devono essere formati semplici e versionati, non un accesso diretto a Penpot durante il
runtime.

Struttura consigliata:

```text
Worldsmelt UI
├── 00 Foundations
├── 01 Tokens
├── 02 Components
├── 03 Patterns
├── 04 Screens
├── 05 Accessibility
└── 99 Archive
```

## Token

Gerarchia:

```text
global
→ semantic
→ component
```

Esempi:

```text
color/global/iron-900
color/semantic/panel-background
color/component/button-primary-hover

space/global/4
space/semantic/control-gap
size/component/hud-icon

motion/fast
motion/menu-transition
```

Non usare nomi come `blue-button` o `dark-panel`: i nomi devono descrivere funzione e
stato.

## Componenti minimi

- button;
- icon button;
- panel 9-slice;
- card tema;
- card scoperta;
- slot;
- tooltip;
- health segment;
- resource counter;
- progress/status message;
- tabs;
- modal;
- list row;
- slider;
- toggle;
- key/controller hint;
- focus ring;
- scrollbar;
- notification.

Ogni componente definisce:

- default;
- hover;
- pressed;
- focused;
- disabled;
- selected, quando applicabile;
- dimensioni minime;
- padding;
- testo lungo;
- controller;
- localizzazione.

## 9-slice

Pannelli e carte devono usare 9-slice quando possibile. Una singola cornice produce pannelli
di dimensioni differenti senza deformare gli angoli.

Manifest concettuale:

```json
{
  "id": "panel/bone/default",
  "texture": "panel_bone.png",
  "slice": {"left": 12, "top": 12, "right": 12, "bottom": 12},
  "padding": {"left": 16, "top": 14, "right": 16, "bottom": 14}
}
```

## Stable Diffusion per UI

LoRA proposta:

```text
worldsmelt-ui
```

Dataset separato da nemici e ambienti. Caption:

```text
wsmeltpx, single pixel-art UI frame, nine-slice compatible,
ornate forged metal border, empty center, no text, front view
```

Vincoli:

- nessun testo;
- un componente per immagine;
- simmetria controllata;
- margini;
- sfondo rimovibile;
- palette del kit;
- test di 9-slice;
- più stati derivati deterministicamente quando possibile.

## raygui

Uso consigliato:

- strumenti interni;
- asset browser;
- editor dei manifest;
- debugger;
- pannello degli esperimenti;
- configurazione modelli.

La UI finale del gioco resta un modulo raylib personalizzato, perché deve supportare
transizioni, carte, controller, pixel snapping, 9-slice e uno stile molto specifico.

## Moduli C proposti

```text
src/ui/
├── ui_context.h/.c
├── ui_layout.h/.c
├── ui_input.h/.c
├── ui_focus.h/.c
├── ui_draw.h/.c
├── ui_theme.h/.c
├── ui_nineslice.h/.c
├── ui_animation.h/.c
├── ui_accessibility.h/.c
└── screens/
```

Il renderer non legge Penpot. Legge:

```text
generated/ui/ui_theme.json
generated/ui/*.png
```

## Scaling

Definire una risoluzione logica e scale intere quando possibile. Il design deve gestire:

- 16:9;
- ultrawide;
- finestra ridotta;
- scaling UI;
- modalità fullscreen;
- testo più grande;
- safe area;
- mouse;
- tastiera;
- controller.

Le decisioni esatte sulla risoluzione logica e sul target minimo restano nella coda delle
domande finché non sono approvate.

## Accettazione

Una schermata è pronta quando:

- corrisponde alla KB;
- usa token;
- non contiene magic numbers non documentati;
- mouse/tastiera/controller funzionano;
- focus è visibile;
- testo lungo non rompe il layout;
- scala intera verificata;
- fallback grafico presente;
- screenshot test o verifica riproducibile disponibile.
