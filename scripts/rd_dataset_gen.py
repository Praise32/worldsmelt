#!/usr/bin/env python3
"""rd_dataset_gen — client Retro Diffusion Cloud + orchestratore dataset (RD-PREP, 07/08/2026).

Contesto e fonti: censimento verificato il 07/08/2026 su
https://github.com/Retro-Diffusion/api-examples (in particolare
example-scripts/rd_client.py, 07_async_batch.py, 11_recover_async_tasks.py) e sul
ToS PDF del 19/08/2025 -- verdetto e note complete in
docs/ai-production/retro-diffusion-letter.md. La chiave arriva DOPO questa sessione
(niente credito oggi): tutto qui deve girare a vuoto con --mock/--check-cost SENZA
RD_API_KEY, e degradare con un errore leggibile (mai un traceback) quando la rete
serve davvero e la chiave manca. Nessuna chiave in questo file o in un altro file
del repo -- solo la variabile d'ambiente RD_API_KEY (regole ML, docs/ai-production/
regole-agenti-ml.md, divieto 4).

CONFINE DI PROVENIENZA (dataset/README.md, regola d'oro 3): gli output di questo
script NON entrano mai in docs/ai-production/dataset/ledger.jsonl (il registro del
corpus per la Worldsmelt LoRA) come sorgente di training finche' Astropulse non
conferma per iscritto il permesso (vedi la lettera). Qui restano nel loro registro
separato, rd-receipts.jsonl: e' un ledger di COSTO e provenienza per gli asset
CURATI (tier plus) e i provini (tier fast), non un lasciapassare per il training.

Politica qualita' del proprietario, applicata (non decisa qui) dal piano quote:
  fast -> dataset/training, provini economici
  plus -> asset curati che entrano nel gioco
  pro  -> SOLO reference-consistency (famiglie personaggi, parti boss, fusioni)
Il piano vive in docs/ai-production/dataset/rd-dataset-plan.json (quote per
categoria/tier, stili, temi a rotazione sulla palette Fucina); i soggetti vengono
dai VisualSpec di generated/visualspecs/batch.json (batch di bin/melting-gen
--visualspecs N). Ogni tier riparte dall'inizio della lista di soggetti del dominio:
la scala fast->plus->pro sullo stesso soggetto e' VOLUTA (provino, poi asset curato,
poi ancora di coerenza che aggancia gli output Plus della stessa famiglia). Quando un
dominio ha meno soggetti della SOMMA delle sue quote, quel riuso diventa credito speso
due o tre volte sullo stesso soggetto: expand_plan() lo AVVISA elencando quali e con
che comando generarne di distinti -- mai un riciclo silenzioso.

Sottocomandi via flag (nessun subcomando: un solo script, un solo scopo):
  --check-cost   stima il costo TOTALE del piano con le formule del censimento
                 (nessuna rete, nessuna chiave richiesta: serve a decidere PRIMA
                 che arrivi il credito).
  --mock         risposte finte offline (PNG sintetico via PIL, stesse formule di
                 costo dell'API reale) per testare l'intero giro senza rete/chiave.
  --guided       aggiunge input_image dalla maschera COLORATA di
                 artifacts/guided-bench/masks-color/ piu' vicina al body_plan del
                 VisualSpec (euristica a parole chiave, mai a caso: vedi
                 choose_mask), con --strength parametrica (default 0.65, lo stesso
                 valore del giro guided del 07/08 in
                 docs/design/governance/decision-log.md, addendum 2 del report
                 teacher-bench).
  --palette      aggiunge input_palette: la palette Fucina (assets/art-src/palette/
                 worldsmelt-fucina.gpl, 31 colori) resa come immagine a runtime
                 (build_palette_image_b64), mai un PNG duplicato da tenere
                 sincronizzato a mano col .gpl.

Ricevute: ogni chiamata scrive una riga JSON in rd-receipts.jsonl (default
docs/ai-production/dataset/rd-receipts.jsonl, versionato come ledger.jsonl -- e'
documentazione di provenienza, non output generato). Il file e' append-only e
serve anche da meccanismo di RIPRESA: un evento "submitted" con task_id scritto
PRIMA di iniziare il polling permette a una run successiva di ritrovare un task
sospeso e continuare a interrogarlo invece di sottomettere (e pagare) di nuovo la
stessa immagine -- vedi process_unit(). L'API reale non offre un modo affidabile
per correlare un job orfano al soggetto che lo ha generato (GET /inferences/tasks
restituisce solo task_id/status/created_at, mai il prompt): la ricevuta locale,
scritta PRIMA del poll, e' l'unica fonte di verita' per l'idempotenza, non un
ripiego.

Dipendenze: solo libreria standard + Pillow (PIL), come richiesto. HTTP via
urllib.request (niente `requests`: coerente col resto del repo, es.
scripts/dataset_ledger.py usa solo stdlib).

Uso:
    python3 scripts/rd_dataset_gen.py --check-cost
    python3 scripts/rd_dataset_gen.py --mock --limit 5
    RD_API_KEY=rdpk-... python3 scripts/rd_dataset_gen.py --guided --palette
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import io
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from remap_fucina import load_palette  # noqa: E402 -- riusa il parser .gpl, niente duplicazione

REPO_ROOT = Path(__file__).resolve().parent.parent
API_BASE_URL = "https://api.retrodiffusion.ai/v1"

DEFAULT_PLAN = REPO_ROOT / "docs" / "ai-production" / "dataset" / "rd-dataset-plan.json"
DEFAULT_VISUALSPECS = REPO_ROOT / "generated" / "visualspecs" / "batch.json"
DEFAULT_OUT = REPO_ROOT / "generated" / "rd-assets"
DEFAULT_RECEIPTS = REPO_ROOT / "docs" / "ai-production" / "dataset" / "rd-receipts.jsonl"
DEFAULT_MASKS_DIR = REPO_ROOT / "artifacts" / "guided-bench" / "masks-color"

RETRY_MAX = 5           # tentativi su 429 prima di rinunciare (Retry-After rispettato ogni volta)
POLL_INTERVAL_S = 2.0
POLL_TIMEOUT_S = 180.0  # oltre questo la unita' resta "pending" nella ricevuta: si ripete lo stesso comando, mai un secondo submit


# ---------------------------------------------------------------------------
# Costi -- nessuna chiamata di rete: stessa tabella del censimento RD-PREP
# (07/08/2026), cosi' --check-cost funziona anche prima che arrivi la chiave.
# ---------------------------------------------------------------------------

def cost_per_image(style_id: str, width: int, height: int) -> float:
    """Prezzo di UNA immagine. La formula la sceglie il NOME dello stile, non
    un parametro a parte: rd_pro__* e' flat, qualunque stile con "low_res" nel
    nome (Fast o Plus) usa la formula low-res, altrimenti rd_fast__*/rd_plus__*
    usano le rispettive formule base. Se lo stile non corrisponde a nessun
    prefisso noto e' un errore di configurazione del piano, non un default
    silenzioso (un costo sbagliato per difetto e' peggio di un crash qui)."""
    if style_id.startswith("rd_pro__"):
        return 0.18
    if "low_res" in style_id:
        return max(0.02, (width * height + 13700) / 6e5)
    if style_id.startswith("rd_fast__"):
        return max(0.015, (width * height + 100000) / 6e6)
    if style_id.startswith("rd_plus__"):
        return max(0.025, (width * height + 50000) / 2e6)
    raise ValueError(f"stile sconosciuto, nessuna formula di costo per {style_id!r}")


# ---------------------------------------------------------------------------
# Maschere guidate -- 8 sagome fisse (artifacts/guided-bench/masks-color/,
# generatore in scratchpad di sessione, pattern noto): non una per ogni
# VisualSpec, un'euristica a parole chiave dal body_plan/subtype verso la
# sagoma piu' vicina fra quelle disponibili per il dominio. Grezze per
# costruzione (vedi decision-log, addendum 2 del 07/08): bastano a dare la
# vista e la silhouette giuste, il dettaglio lo aggiunge lo stile RD.
# ---------------------------------------------------------------------------

MASK_CANDIDATES_BY_DOMAIN = {
    "character": ["char_smelter_mage", "char_ash_ranger"],
    "enemy": ["enemy_ash_beetle", "enemy_ember_blob", "enemy_walking_crucible"],
    "weapon": ["weapon_forge_hammer"],
    "item": ["item_fusion_catalyst"],
    "boss_part": ["boss_furnace_core"],
}
MASK_KEYWORDS = {
    "char_smelter_mage": ["robe", "cloth", "wide", "wrap", "hood", "priest", "mage", "vest"],
    "char_ash_ranger": ["slender", "lean", "narrow", "ranger", "agile", "scout", "cloak"],
    "enemy_ash_beetle": ["beetle", "quadruped", "carapace", "insect", "shell", "leg", "crawl"],
    "enemy_ember_blob": ["blob", "ooze", "gelatin", "amorphous", "slime", "mass", "puddle"],
    "enemy_walking_crucible": ["crucible", "cauldron", "pot", "vessel", "container", "squat", "urn"],
    "item_fusion_catalyst": ["crystal", "gem", "catalyst", "relic", "shard", "orb", "prism"],
    "weapon_forge_hammer": ["hammer", "blade", "sword", "axe", "spear", "mace", "hilt", "weapon"],
}
MASK_DEFAULT_BY_DOMAIN = {
    "character": "char_smelter_mage",
    "enemy": "enemy_ember_blob",
    "weapon": "weapon_forge_hammer",
    "item": "item_fusion_catalyst",
    "boss_part": "boss_furnace_core",
}


def choose_mask(domain: str, spec: dict) -> str | None:
    candidates = MASK_CANDIDATES_BY_DOMAIN.get(domain)
    if not candidates:
        return None
    text = " ".join([
        spec.get("body_plan", ""),
        spec.get("subtype", ""),
        " ".join(spec.get("materials", []) or []),
        spec.get("distinctive_feature", ""),
    ]).lower()
    best, best_score = candidates[0], -1
    for name in candidates:
        score = sum(1 for kw in MASK_KEYWORDS.get(name, []) if kw in text)
        if score > best_score:
            best, best_score = name, score
    if best_score <= 0:
        return MASK_DEFAULT_BY_DOMAIN.get(domain, candidates[0])
    return best


# ---------------------------------------------------------------------------
# Palette Fucina come immagine per input_palette (RD la vuole come "immagine
# palette" in base64, RGB, "well under 1MB" -- non una lista di colori: vedi
# lo schema verificato sull'esempio ufficiale). Costruita a runtime dal .gpl
# canonico via lo stesso parser di remap_fucina.py: mai un secondo file da
# tenere sincronizzato a mano con la palette ufficiale (DEC-173).
# ---------------------------------------------------------------------------

def build_palette_image_b64() -> str:
    palette = load_palette()  # [(r,g,b,nome), ...], ordine del .gpl
    cols = 8  # "Columns: 8" dichiarato in testa al .gpl stesso
    cell = 16
    rows = (len(palette) + cols - 1) // cols
    im = Image.new("RGB", (cols * cell, rows * cell), (0, 0, 0))
    draw = ImageDraw.Draw(im)
    for i, (r, g, b, _name) in enumerate(palette):
        x, y = (i % cols) * cell, (i // cols) * cell
        draw.rectangle([x, y, x + cell - 1, y + cell - 1], fill=(r, g, b))
    buf = io.BytesIO()
    im.save(buf, format="PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


# ---------------------------------------------------------------------------
# Prompt SOLO-soggetto. Diverso APPOSTA da scripts/visualspec_template.py:
# quello scrive per SD1.5 locale (ha bisogno di vista/inquadratura/sfondo nel
# testo perche' CLIP non li ottiene altrimenti -- vedi l'addendum 2 del
# report teacher-bench). RD invece li assegna con prompt_style, e la sua
# stessa guida dice esplicitamente di non scrivere "pixel art" ne'
# istruzioni di resa nel prompt: farlo confonde la STYLE del modello con la
# richiesta e produce risultati peggiori secondo la documentazione ufficiale.
# ---------------------------------------------------------------------------

def build_subject_prompt(spec: dict, theme: dict | None) -> str:
    subtype = (spec.get("subtype") or "").strip()
    body_plan = (spec.get("body_plan") or "").strip()
    materials = spec.get("materials") or []
    feature = (spec.get("distinctive_feature") or "").strip()
    size_class = (spec.get("size_class") or "").strip()

    parts = []
    if subtype:
        parts.append(subtype)
    if body_plan:
        parts.append(f"{body_plan} build")
    if materials:
        parts.append("made of " + ", ".join(materials))
    if feature:
        parts.append(feature)
    if size_class:
        parts.append(f"{size_class} size")
    if theme and theme.get("colors"):
        parts.append("accented with " + " and ".join(theme["colors"][:2]))
    prompt = ", ".join(p for p in parts if p)
    return prompt or subtype or "a game asset"


# ---------------------------------------------------------------------------
# Piano -> lista di unita' di lavoro
# ---------------------------------------------------------------------------

def load_visualspecs(path) -> dict:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    by_domain: dict[str, list] = {}
    for req in data.get("requests", []):
        by_domain.setdefault(req.get("domain"), []).append(req)
    return by_domain


def resolve_style(plan: dict, tier: str, category: str) -> str:
    styles = plan["styles"][tier]
    return styles.get(category, styles["default"])


def expand_plan(plan: dict, specs_by_domain: dict) -> tuple[list, list]:
    """Espande le quote in unita' di lavoro. Ogni tier riparte dall'inizio della
    lista di soggetti del dominio: la SCALA fast->plus->pro sullo stesso
    soggetto e' voluta (il provino economico anticipa l'asset curato, e il tier
    pro aggancia via reference_images gli output Plus GIA' fatti della stessa
    famiglia -- vedi rd-dataset-plan.json, tiers.pro). Il confronto che conta
    per capire se il dominio ha materiale a sufficienza e' quindi con la SOMMA
    delle quote del dominio, non con la singola quota: con 10 soggetti e quote
    6+4+2 nessuna singola quota sfora, ma due soggetti finiscono su tutti e tre
    i tier e vengono fatturati tre volte. Quel riuso resta lecito e voluto --
    non deve pero' essere SILENZIOSO, perche' e' credito speso."""
    units, warnings = [], []
    themes = plan.get("themes", [])
    theme_idx = 0
    quotas = plan.get("quotas", [])

    demand_by_domain: dict[str, int] = {}
    tiers_by_domain: dict[str, list] = {}
    for q in quotas:
        demand_by_domain[q["category"]] = demand_by_domain.get(q["category"], 0) + q["count"]
        tiers_by_domain.setdefault(q["category"], []).append(f"{q['tier']}:{q['count']}")

    for category in dict.fromkeys(q["category"] for q in quotas):
        available = specs_by_domain.get(category, [])
        demand = demand_by_domain[category]
        if demand <= len(available):
            continue
        missing = demand - len(available)
        breakdown = " + ".join(tiers_by_domain[category])
        tiers_per_subject: dict[str, list] = {}
        for q in quotas:
            if q["category"] != category:
                continue
            for req in available[:q["count"]]:
                tiers_per_subject.setdefault(req["id"], []).append(q["tier"])
        reused = sorted(sid for sid, tl in tiers_per_subject.items() if len(tl) > 1)
        detail = ", ".join(f"{sid} ({'+'.join(tiers_per_subject[sid])})" for sid in reused)
        warnings.append(
            f"dominio '{category}': le quote chiedono {breakdown} = {demand} soggetti in totale, "
            f"ma generated/visualspecs/batch.json ne ha {len(available)}. I tier ripartono "
            f"dall'inizio della lista, quindi {len(reused)} soggetti vengono generati su piu' "
            f"tier e fatturati piu' volte -- voluto (scala provino->curato->ancora), ma "
            f"dichiarato: {detail}. Per avere soggetti distinti su ogni tier: "
            f"bin/melting-gen --visualspecs {missing}  (poi rilancia questo script)."
        )

    for q in quotas:
        category, tier, count = q["category"], q["tier"], q["count"]
        available = specs_by_domain.get(category, [])
        if count > len(available):
            warnings.append(
                f"quota {category}/{tier}: chiesti {count} soggetti, disponibili "
                f"{len(available)} per il dominio '{category}' in generated/visualspecs/"
                f"batch.json -- questa singola quota si ferma a {len(available)}, "
                f"non inventa soggetti."
            )
        tier_cfg = plan["tiers"][tier]
        for spec_req in available[:count]:
            theme = themes[theme_idx % len(themes)] if themes else None
            theme_idx += 1
            units.append({
                "subject_id": spec_req["id"],
                "category": category,
                "tier": tier,
                "spec": spec_req.get("spec", {}),
                "theme": theme,
                "width": tier_cfg["width"],
                "height": tier_cfg["height"],
                "num_images": tier_cfg["num_images"],
                "remove_bg": tier_cfg.get("remove_bg", True),
            })
    return units, warnings


# ---------------------------------------------------------------------------
# Ricevute (rd-receipts.jsonl) -- append-only, stesso pattern di
# scripts/dataset_ledger.py: una riga = un evento, mai riscritto in place.
# ---------------------------------------------------------------------------

def now_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%S")


def load_receipts(path) -> list:
    path = Path(path)
    if not path.exists():
        return []
    out = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                out.append(json.loads(line))
            except json.JSONDecodeError:
                continue  # riga corrotta: ignorata, mai fatale per il resto del registro
    return out


def append_receipt(path, entry: dict) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(entry, ensure_ascii=False) + "\n")


def unit_key(unit: dict, guided: bool, mask_name, palette_used: bool, strength) -> str:
    """Chiave di idempotenza: stessi parametri -> stessa chiave -> stessa riga
    di ricevuta. NON include il seed (nessun seed fisso nel piano oggi: se un
    giorno lo si aggiunge, va incluso qui perche' cambia davvero l'output)."""
    payload = {
        "subject_id": unit["subject_id"], "tier": unit["tier"], "category": unit["category"],
        "width": unit["width"], "height": unit["height"], "num_images": unit["num_images"],
        "guided": guided, "mask": mask_name if guided else None,
        "strength": strength if guided else None,
        "palette": palette_used,
        "theme": unit["theme"]["id"] if unit["theme"] else None,
    }
    blob = json.dumps(payload, sort_keys=True).encode("utf-8")
    return hashlib.sha256(blob).hexdigest()[:16]


# ---------------------------------------------------------------------------
# Trasporto HTTP reale
# ---------------------------------------------------------------------------

class RDError(RuntimeError):
    pass


def get_api_key() -> str:
    key = os.environ.get("RD_API_KEY")
    if not key:
        raise SystemExit(
            "RD_API_KEY non impostata: il client Retro Diffusion non parte senza una "
            "chiave valida (mai chiavi nel repo, docs/ai-production/regole-agenti-ml.md "
            "divieto 4). Imposta la variabile d'ambiente prima di rilanciare "
            "(export RD_API_KEY=rdpk-...), oppure usa --mock per un giro offline o "
            "--check-cost per la sola stima costi (nessuno dei due richiede la chiave)."
        )
    return key


def _http_request(method: str, url: str, api_key: str, json_body=None, params=None, timeout=30):
    if params:
        url = f"{url}?{urllib.parse.urlencode(params)}"
    data = json.dumps(json_body).encode("utf-8") if json_body is not None else None
    attempt = 0
    while True:
        attempt += 1
        req = urllib.request.Request(url, data=data, method=method)
        req.add_header("X-RD-Token", api_key)
        if data is not None:
            req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            body = e.read().decode("utf-8", errors="replace")
            if e.code == 429 and attempt <= RETRY_MAX:
                retry_after = e.headers.get("Retry-After")
                wait_s = float(retry_after) if retry_after else min(2 ** attempt, 60)
                print(f"  429 rate limited su {method} {url}, attendo {wait_s:.0f}s (Retry-After)...",
                      file=sys.stderr)
                time.sleep(wait_s)
                continue
            raise RDError(f"HTTP {e.code} su {method} {url}: {body}") from e
        except urllib.error.URLError as e:
            raise RDError(f"errore di rete su {method} {url}: {e}") from e


class RealTransport:
    def __init__(self, api_key: str):
        self.api_key = api_key

    def submit(self, payload: dict) -> dict:
        resp = _http_request("POST", f"{API_BASE_URL}/inferences", self.api_key,
                              json_body={**payload, "async": True}, timeout=300)
        if "task_id" not in resp:
            raise RDError(f"risposta senza task_id per una richiesta async: {resp}")
        return resp

    def poll(self, task_id: str) -> dict:
        return _http_request("GET", f"{API_BASE_URL}/inferences/tasks/{task_id}",
                              self.api_key, timeout=30)


class MockTransport:
    """Risposte finte, offline, per --mock. Async simulato SINCRONO: il
    risultato si genera e si registra gia' a submit(), poll() lo ritrova
    subito "succeeded". Basta a esercitare la FORMA del giro
    submit->task_id->poll->result (compreso il salvataggio della ricevuta fra
    i due passi): il timing reale del servizio non serve offline, e
    aggiungere uno stato "pending" persistito fra processi separati
    servirebbe solo a testare urllib, non questo script."""

    def __init__(self):
        self.balance = 100.0
        self._tasks: dict[str, dict] = {}
        self._counter = 0

    def submit(self, payload: dict) -> dict:
        style = payload["prompt_style"]
        w, h = payload["width"], payload["height"]
        n = payload.get("num_images", 1)
        cost = round(cost_per_image(style, w, h) * n, 6)
        self.balance = round(self.balance - cost, 6)
        images = [self._fake_png(payload, i) for i in range(n)]
        self._counter += 1
        task_id = f"mock-{self._counter:04d}"
        self._tasks[task_id] = {
            "created_at": now_iso(),
            "balance_cost": cost,
            "remaining_balance": self.balance,
            "base64_images": images,
            "model": style,
        }
        return {"task_id": task_id}

    def poll(self, task_id: str) -> dict:
        result = self._tasks.get(task_id)
        if result is None:
            return {"task_id": task_id, "status": "failed", "error": "task mock sconosciuto"}
        return {"task_id": task_id, "status": "succeeded", "result": result}

    def _fake_png(self, payload: dict, index: int) -> str:
        # Colore deterministico dal prompt+indice via sha256, NON via hash():
        # hash() delle stringhe e' randomizzato per processo da PYTHONHASHSEED,
        # quindi due run identiche di --mock producevano PNG diversi e
        # 'output_sha256' nelle ricevute -- che e' provenienza -- non era
        # riproducibile. sha256 e' stabile fra processi, macchine e versioni.
        digest = hashlib.sha256(f"{payload['prompt']}|{index}".encode("utf-8")).digest()
        h = int.from_bytes(digest[:3], "big")
        color = ((h >> 0) % 200 + 30, (h >> 8) % 200 + 30, (h >> 16) % 200 + 30, 255)
        im = Image.new("RGBA", (payload["width"], payload["height"]), color)
        buf = io.BytesIO()
        im.save(buf, format="PNG")
        return base64.b64encode(buf.getvalue()).decode("ascii")


# ---------------------------------------------------------------------------
# Salvataggio immagini
# ---------------------------------------------------------------------------

def save_images(result: dict, out_dir: Path, unit: dict, key: str) -> tuple[list, list]:
    dest = Path(out_dir) / unit["tier"] / unit["category"]
    dest.mkdir(parents=True, exist_ok=True)
    images = result.get("base64_images") or []
    paths, hashes = [], []
    for i, b64 in enumerate(images):
        data = base64.b64decode(b64)
        suffix = f"_{i + 1}" if len(images) > 1 else ""
        path = dest / f"{unit['subject_id']}__{key}{suffix}.png"
        path.write_bytes(data)
        try:
            rel = path.relative_to(REPO_ROOT)
        except ValueError:
            rel = path
        paths.append(str(rel))
        hashes.append(hashlib.sha256(data).hexdigest())
    return paths, hashes


# ---------------------------------------------------------------------------
# Elaborazione di una unita'
# ---------------------------------------------------------------------------

def process_unit(unit: dict, plan: dict, args, transport, out_dir: Path,
                  receipts_path: Path, masks_dir: Path, palette_b64) -> str:
    style = resolve_style(plan, unit["tier"], unit["category"])
    guided = args.guided
    mask_name = choose_mask(unit["category"], unit["spec"]) if guided else None
    strength = args.strength if guided else None
    palette_used = bool(args.palette)

    key = unit_key(unit, guided, mask_name, palette_used, strength)
    receipts = [r for r in load_receipts(receipts_path) if r.get("unit_key") == key]
    last = receipts[-1] if receipts else None  # append-only: l'ultima riga per la chiave e' lo stato vero

    label = f"{unit['subject_id']}/{unit['tier']}"

    if last and last.get("event") == "succeeded":
        print(f"  [skip] {label}: gia' completato ({key})")
        return "skipped"

    prompt = build_subject_prompt(unit["spec"], unit["theme"])
    payload = {
        "prompt": prompt,
        "prompt_style": style,
        "width": unit["width"],
        "height": unit["height"],
        "num_images": unit["num_images"],
        "remove_bg": unit["remove_bg"],
    }
    if guided and mask_name:
        mask_path = masks_dir / f"{mask_name}__fill.png"
        if mask_path.exists():
            payload["input_image"] = base64.b64encode(mask_path.read_bytes()).decode("ascii")
            payload["strength"] = strength
        else:
            print(f"  ATTENZIONE: maschera mancante {mask_path}, {label} procede senza guided",
                  file=sys.stderr)
    if palette_used and palette_b64:
        payload["input_palette"] = palette_b64

    if last and last.get("event") == "submitted" and last.get("task_id"):
        task_id = last["task_id"]
        print(f"  [recover] {label}: riprendo il task {task_id} senza ri-sottomettere")
    else:
        try:
            submitted = transport.submit(payload)
        except RDError as e:
            print(f"  [failed] {label}: sottomissione fallita: {e}", file=sys.stderr)
            append_receipt(receipts_path, {
                "ts": now_iso(), "event": "failed", "unit_key": key, "subject_id": unit["subject_id"],
                "category": unit["category"], "tier": unit["tier"], "error": str(e), "mock": args.mock,
            })
            return "failed"
        task_id = submitted["task_id"]
        # Scritta SUBITO, prima del poll: se il processo muore da qui in poi,
        # la prossima run trova questa riga e riprende lo stesso task_id
        # invece di sottomettere (e pagare) di nuovo la stessa immagine.
        append_receipt(receipts_path, {
            "ts": now_iso(), "event": "submitted", "unit_key": key,
            "subject_id": unit["subject_id"], "category": unit["category"], "tier": unit["tier"],
            "style": style, "width": unit["width"], "height": unit["height"],
            "num_images": unit["num_images"], "guided": guided, "mask": mask_name,
            "strength": strength, "palette": palette_used,
            "theme": unit["theme"]["id"] if unit["theme"] else None,
            "task_id": task_id, "mock": args.mock,
        })

    deadline = time.time() + POLL_TIMEOUT_S
    status, result, error = None, None, None
    while time.time() < deadline:
        polled = transport.poll(task_id)
        status = polled.get("status")
        if status == "succeeded":
            result = polled.get("result")
            break
        if status == "failed":
            error = polled.get("error")
            break
        time.sleep(args.poll_interval)
    else:
        print(f"  [pending] {label}: timeout di polling, rilancia lo stesso comando "
              f"(la ricevuta 'submitted' fa riprendere il task, non un nuovo submit)",
              file=sys.stderr)
        return "pending"

    if status == "failed" or result is None:
        print(f"  [failed] {label}: {error}", file=sys.stderr)
        append_receipt(receipts_path, {
            "ts": now_iso(), "event": "failed", "unit_key": key, "subject_id": unit["subject_id"],
            "category": unit["category"], "tier": unit["tier"], "task_id": task_id,
            "error": error or "esito sconosciuto", "mock": args.mock,
        })
        return "failed"

    out_paths, out_hashes = save_images(result, out_dir, unit, key)
    append_receipt(receipts_path, {
        "ts": now_iso(), "event": "succeeded", "unit_key": key,
        "subject_id": unit["subject_id"], "category": unit["category"], "tier": unit["tier"],
        "style": style, "width": unit["width"], "height": unit["height"],
        "num_images": unit["num_images"], "guided": guided, "mask": mask_name,
        "strength": strength, "palette": palette_used,
        "theme": unit["theme"]["id"] if unit["theme"] else None,
        "task_id": task_id, "balance_cost": result.get("balance_cost"),
        "remaining_balance": result.get("remaining_balance"),
        "output_paths": out_paths, "output_sha256": out_hashes, "mock": args.mock,
    })
    print(f"  [ok] {label}: {len(out_paths)} immagini, costo {result.get('balance_cost')}, "
          f"saldo {result.get('remaining_balance')}")
    return "succeeded"


# ---------------------------------------------------------------------------
# --check-cost: nessuna rete, nessuna chiave
# ---------------------------------------------------------------------------

def cmd_check_cost(units: list, plan: dict) -> int:
    if not units:
        print("nessuna unita' nel piano (dopo i filtri): niente da stimare")
        return 0
    total = 0.0
    header = f"{'soggetto':28s} {'categoria':10s} {'tier':5s} {'stile':30s} {'dim':11s} {'n':>2s} {'$/img':>9s} {'subtot $':>9s}"
    print(header)
    print("-" * len(header))
    for u in units:
        style = resolve_style(plan, u["tier"], u["category"])
        cpi = cost_per_image(style, u["width"], u["height"])
        subtotal = cpi * u["num_images"]
        total += subtotal
        dims = f"{u['width']}x{u['height']}"
        print(f"{u['subject_id']:28s} {u['category']:10s} {u['tier']:5s} {style:30s} "
              f"{dims:11s} {u['num_images']:>2d} {cpi:>9.5f} {subtotal:>9.5f}")
    print("-" * len(header))
    print(f"TOTALE piano: {len(units)} richieste, ${total:.4f} USD stimati "
          f"(formule locali, equivalenti a check_cost=true lato server -- nessun addebito, nessuna chiamata)")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="rd_dataset_gen.py",
        description="Client + orchestratore dataset Retro Diffusion Cloud (docs/ai-production/dataset/rd-dataset-plan.json).")
    p.add_argument("--plan", default=str(DEFAULT_PLAN), help="piano quote JSON")
    p.add_argument("--visualspecs", default=str(DEFAULT_VISUALSPECS), help="batch VisualSpec")
    p.add_argument("--out", default=str(DEFAULT_OUT), help="cartella di output delle immagini")
    p.add_argument("--receipts", default=str(DEFAULT_RECEIPTS), help="ledger rd-receipts.jsonl")
    p.add_argument("--masks-dir", default=str(DEFAULT_MASKS_DIR), help="cartella maschere colorate")
    p.add_argument("--check-cost", action="store_true", help="stima costo totale del piano, nessuna rete")
    p.add_argument("--mock", action="store_true", help="risposte finte offline, nessuna chiave richiesta")
    p.add_argument("--guided", action="store_true", help="usa input_image dalla maschera colorata piu' vicina")
    p.add_argument("--strength", type=float, default=0.65, help="forza img2img per --guided (0-1, default 0.65)")
    p.add_argument("--palette", action="store_true", help="allega input_palette (palette Fucina)")
    p.add_argument("--category", default=None, help="filtra per categoria (character/enemy/weapon/item/boss_part)")
    p.add_argument("--tier", default=None, help="filtra per tier (fast/plus/pro)")
    p.add_argument("--limit", type=int, default=None, help="limita al primo N unita' del piano espanso")
    p.add_argument("--poll-interval", type=float, default=POLL_INTERVAL_S, help="secondi fra un poll e il successivo")
    return p


def load_and_filter_units(args) -> tuple[list, dict]:
    plan = json.loads(Path(args.plan).read_text(encoding="utf-8"))
    specs_by_domain = load_visualspecs(args.visualspecs)
    units, warnings = expand_plan(plan, specs_by_domain)
    for w in warnings:
        print(f"AVVISO: {w}", file=sys.stderr)
    if args.category:
        units = [u for u in units if u["category"] == args.category]
    if args.tier:
        units = [u for u in units if u["tier"] == args.tier]
    if args.limit is not None:
        units = units[:args.limit]
    return units, plan


def main(argv=None) -> int:
    args = build_arg_parser().parse_args(argv)

    # Validazione degli ARGOMENTI prima di qualunque altro controllo: stava dopo
    # get_api_key() e con la chiave assente un `--strength 5` riportava
    # "RD_API_KEY non impostata", cioe' il problema sbagliato.
    if args.strength < 0.0 or args.strength > 1.0:
        raise SystemExit(f"--strength deve stare in [0,1], ricevuto {args.strength}")

    if not Path(args.plan).exists():
        raise SystemExit(f"piano non trovato: {args.plan}")
    if not Path(args.visualspecs).exists():
        raise SystemExit(
            f"VisualSpec non trovati: {args.visualspecs} -- generane con "
            f"bin/melting-gen --visualspecs N prima di rilanciare."
        )

    units, plan = load_and_filter_units(args)

    if args.check_cost:
        return cmd_check_cost(units, plan)

    if not args.mock:
        get_api_key()  # errore chiaro e immediato, prima di qualunque lavoro

    if not units:
        print("nessuna unita' da generare (piano vuoto o filtri troppo stretti)")
        return 0

    transport = MockTransport() if args.mock else RealTransport(get_api_key())
    palette_b64 = build_palette_image_b64() if args.palette else None
    masks_dir = Path(args.masks_dir)
    out_dir = Path(args.out)
    receipts_path = Path(args.receipts)

    counts = {"succeeded": 0, "skipped": 0, "failed": 0, "pending": 0}
    for unit in units:
        outcome = process_unit(unit, plan, args, transport, out_dir, receipts_path, masks_dir, palette_b64)
        counts[outcome] = counts.get(outcome, 0) + 1

    print(f"-- fine: {counts} --")
    print(f"ricevute: {receipts_path}")
    return 0 if counts["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
