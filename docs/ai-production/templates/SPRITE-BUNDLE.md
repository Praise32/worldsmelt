---
id: aiprod-sprite-bundle
title: SpriteBundle Template
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Schema JSON per bundle sprite: rig, canvas/pivot/hitbox, texture, sockets, animazioni, metadata di generazione (seed, steps, cfg, LoRA), validazione e fallback.
last_reviewed: 2026-07-22
topics: [template, sprite, JSON schema, generazione, fallback]
related: []
supersedes: []
source_files: [tools/melting-sprites]
---
# SpriteBundle Template

```json
{
  "version": 1,
  "id": "",
  "content_hash": "",
  "rig": "",
  "canvas": [128, 128],
  "pivot": [64, 100],
  "hitbox": [0, 0, 0, 0],
  "textures": {},
  "sockets": {},
  "animations": {},
  "generation": {
    "pipeline_version": "",
    "model_sha256": "",
    "loras": [],
    "prompt_sha256": "",
    "negative_prompt_sha256": "",
    "seed": 0,
    "steps": 0,
    "cfg": 0,
    "generation_size": 512,
    "postprocess_version": ""
  },
  "validation": {
    "status": "accepted|fallback|rejected",
    "checks": {},
    "reviewed": false
  },
  "fallback": {
    "atlas_cell": -1,
    "geometric_profile": ""
  }
}
```
