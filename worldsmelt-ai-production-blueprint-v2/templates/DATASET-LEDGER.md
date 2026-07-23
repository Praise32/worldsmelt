# Dataset Ledger Specification

Formato consigliato: JSONL, una riga per asset.

## Campi richiesti

```json
{
  "asset_id": "",
  "path": "",
  "original_sha256": "",
  "processed_sha256": "",
  "source_url": "",
  "author": "",
  "source_pack": "",
  "license": "",
  "license_url": "",
  "downloaded_at": "",
  "allowed_commercial": false,
  "allowed_derivatives": false,
  "training_allowed_explicit": false,
  "provenance_status": "verified|unknown|quarantine",
  "transformations": [],
  "caption": "",
  "body_plan": "",
  "role": "",
  "view": "",
  "split": "train|validation|test",
  "split_group": "",
  "reviewer": "",
  "notes": ""
}
```

## Regole

- `provenance_status=unknown` non entra in `commercial-clean`.
- `split_group` è obbligatorio per animazioni e identità.
- conservare sempre l'originale;
- non sovrascrivere hash;
- ogni trasformazione aggiunge un nuovo processed hash;
- licenza e source URL non possono essere inferiti dal nome del file.
