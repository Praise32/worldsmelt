---
id: aiprod-audio-spec
title: AudioSpec Template
domain: ai-production
status: draft
authority: supporting
owner: ai-production
summary: >-
  Schema JSON per AudioSpec: musica, famiglie di suoni, generation_policy (curated/rfxgen/stable-audio/hybrid), fallback_pack, license_branch.
last_reviewed: 2026-07-22
topics: [template, audio, JSON schema, fallback, licenze]
related: []
supersedes: []
source_files: []
---
# AudioSpec Template

```json
{
  "version": 1,
  "id": "",
  "theme": "",
  "music": {
    "enabled": false,
    "mood": "",
    "bpm": 0,
    "duration_seconds": 0,
    "loop": false,
    "prompt_tags": []
  },
  "families": {
    "ui": "",
    "projectile": "",
    "impact": "",
    "enemy": "",
    "environment": ""
  },
  "generation_policy": {
    "mode": "curated|rfxgen|stable-audio|hybrid",
    "tier": 0,
    "timeout_seconds": 0
  },
  "fallback_pack": "",
  "license_branch": "commercial-clean"
}
```
