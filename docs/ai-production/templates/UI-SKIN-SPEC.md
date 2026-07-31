---
id: aiprod-ui-skin-spec
title: UISkinSpec Template
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Schema JSON per skin UI: risoluzione logica canonica 640x360 (DEC-200, 2026-07-31), integer scale, token, componenti (panel/button/card/slot/tooltip/focus), input supportati, fallback_skin. Migrazione della GUI a questa risoluzione nella sessione CP4; la demo resta su 960x640 (DEC-174) fino ad allora.
last_reviewed: 2026-07-31
last_verified_commit: 4d7a410
topics: [template, UI, JSON schema, design system, raylib, DEC-200]
related: []
supersedes: []
source_files: []
---
# UISkinSpec Template

> **`logical_resolution` è ora una decisione approvata (DEC-200, 2026-07-31).** Il valore
> `[640, 360]`, proposta ricorrente negli appunti e nei template dal tempo di DEC-156, è
> la **risoluzione logica canonica** dell'interfaccia (16:9 nativo: scala intera ×3 =
> 1920×1080 esatto, ×2 = 720p, ×6 = 4K). La **migrazione** della GUI a questa risoluzione
> avviene nella **sessione CP4**: fino ad allora la demo resta sul canvas 960×640 (DEC-174),
> non toccato da questa decisione. Sul formato 21:9 (ultrawide): pillarbox a scala intera
> nella prima release, vista ultrawide nativa eventuale estensione futura non decisa. La
> domanda aperta 11 in
> [`docs/design/governance/open-questions.md`](../../design/governance/open-questions.md#interfaccia)
> è chiusa.

```json
{
  "version": 1,
  "id": "",
  "logical_resolution": [640, 360],
  "integer_scale": true,
  "font": "",
  "tokens": {},
  "components": {
    "panel": {},
    "button": {},
    "card": {},
    "slot": {},
    "tooltip": {},
    "focus": {}
  },
  "input": ["mouse", "keyboard", "controller"],
  "fallback_skin": "curated_ui_core_v1"
}
```
