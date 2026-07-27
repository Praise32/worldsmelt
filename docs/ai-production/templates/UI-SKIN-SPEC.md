---
id: aiprod-ui-skin-spec
title: UISkinSpec Template
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Schema JSON per skin UI: risoluzione logica 640x360 (PROPOSTA, non approvata — DEC-156, open question 11), integer scale, token, componenti (panel/button/card/slot/tooltip/focus), input supportati, fallback_skin.
last_reviewed: 2026-07-27
topics: [template, UI, JSON schema, design system, raylib]
related: []
supersedes: []
source_files: []
---
# UISkinSpec Template

> **`logical_resolution` non è una decisione approvata.** Il valore `[640, 360]` è la
> proposta ricorrente negli appunti e nei template, ma nessuna decisione l'ha mai fissata
> come canone (DEC-156). La risoluzione logica canonica dell'interfaccia resta la
> **domanda aperta 11** in
> [`docs/design/governance/open-questions.md`](../../design/governance/open-questions.md#interfaccia).
> Fino a una decisione, questo valore va trattato come default provvisorio d'esempio, non
> da implementare come requisito definitivo.

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
