---
id: aiprod-ui-skin-spec
title: UISkinSpec Template
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Schema JSON per skin UI: risoluzione logica 640x360, integer scale, token, componenti (panel/button/card/slot/tooltip/focus), input supportati, fallback_skin.
last_reviewed: 2026-07-22
topics: [template, UI, JSON schema, design system, raylib]
related: []
supersedes: []
source_files: []
---
# UISkinSpec Template

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
