# EnemySpec Template

```json
{
  "version": 1,
  "id": "",
  "name": "",
  "origin": "curato|composto|variato|nuovo",
  "role": "chaser|shooter|charger|space_controller|support|summoner|obstacle|splitter|bomber|phased_boss",
  "body_plan": "biped|quadruped|mounted|crawler|blob|serpentine|tentacled|flying|turret|swarm|multipart_boss",
  "rig": "",
  "size": "small|medium|large|boss",
  "locomotion": "",
  "material": [],
  "palette_tags": [],
  "parts": {},
  "stats_band": {},
  "attack": {
    "pattern": "",
    "telegraph": "",
    "cooldown_band": ""
  },
  "required_assets": [],
  "fallback": "",
  "generation": {
    "prompt_tags": [],
    "negative_tags": [],
    "candidate_count": 1
  }
}
```

## Validation checklist

- [ ] ID univoco
- [ ] enum validi
- [ ] rig nel registry
- [ ] ruolo compatibile
- [ ] parti entro budget
- [ ] telegraph presente
- [ ] fallback esistente
- [ ] asset richiesti supportati
