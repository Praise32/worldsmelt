#!/usr/bin/env python3
"""teacher_bench_review — genera artifacts/image-model-research/review.html:
pagina STATICA (nessun server, apribile con file://) per il giudizio umano
Stage A del benchmark teacher immagine (vedi scripts/teacher-bench.sh per il
contesto completo del mandato).

Per ogni immagine mostra raw 512 / pixel-64 / pixel-32 / preview chiara /
preview scura + i metadati di generazione e postproc, con i 9 criteri umani
della matrice benchmark come checkbox. I voti si salvano in localStorage del
browser (chiave per record, mai persi ricaricando la pagina) e un bottone
"Esporta voti" scarica un JSON con tutto quello che e' stato votato finora.
Il JSON esportato riporta ogni criterio anche per NOME accanto all'array
posizionale: l'ordine dei criteri e' un contratto, ma un file di voti deve
restare leggibile anche se un giorno quel contratto cambia.

Track (item 4 Track F): le card sono RAGGRUPPATE per track -- P (teacher,
contratto docs/.../teacher-bench-2026-08-prompts.json) o F (caccia libera,
contratto .../teacher-bench-2026-08-prompts-trackF.json, righe F*/S* di
scripts/teacher-bench.sh) -- dedotto dal campo "prompts_contract_path" che
teacher-bench.sh scrive in ogni manifest, non dal prefisso del config_id (il
manifest e' la fonte di verita', il nome della config e' solo una
convenzione umana). L'immagine PRINCIPALE (prima nella card) e' il 64 (BOX)
per Track F -- il judge_scale del suo contratto e' 64 -- e resta il 32 per
Track P (l'inverso). Le righe S* (asse velocita') portano un badge con
step/CFG/sampler in testa alla card: sono confrontate fra loro proprio su
quei tre numeri, non vanno cercati nella tabella metadati sotto.

Niente framework, niente build step: un solo file HTML con CSS/JS inline,
percorsi RELATIVI alla cartella dove vive review.html (root del benchmark),
cosi' le immagini si aprono anche via file:// -- fetch() di un JSON esterno
non lo farebbe (CORS su file://), per questo i metadati sono incorporati
come JSON dentro un <script> invece che caricati a parte.

Uso:
  python3 scripts/teacher_bench_review.py [--root artifacts/image-model-research] [--out review.html]
Scansiona manifests/*.json (esclude model-ledger.json) -- non richiede che
il postproc sia gia' girato: un record senza "postproc" mostra solo il raw
512 con una nota, non e' un errore.
"""
import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ROOT = REPO_ROOT / "artifacts" / "image-model-research"

# I 9 criteri umani della matrice benchmark (dossier ricerca, sezione
# "Metriche umane" di 03-MATRICE-BENCHMARK.md) -- ordine e testo INVARIATI,
# e' un contratto di giudizio, non prosa libera di questo script.
HUMAN_CRITERIA = [
    "categoria riconoscibile",
    "silhouette leggibile",
    "visuale corretta",
    "coerenza Worldsmelt",
    "materiale riconoscibile",
    "originalita'",
    "utilizzabile senza ritocco",
    "utilizzabile con meno di cinque minuti di ritocco",
    "leggibile nella scena 640x360",
]


# Marcatore del path contratto Track F (vedi TASK item 1): una sottostringa
# basta, non serve un confronto esatto -- e' l'unico contratto del repo che
# la contiene, e resta vero anche se PROMPTS_FILE viene passato con un path
# relativo diverso (es. "./docs/...").
TRACKF_CONTRACT_MARKER = "teacher-bench-2026-08-prompts-trackF.json"


def track_of(record):
    """'F' se il manifest dichiara il contratto Track F, 'P' altrimenti
    (default onesto: un manifest senza "prompts_contract_path" -- generato
    prima di questo campo, o dal fallback sintetico di teacher_bench_post.py
    per un PNG senza manifest -- e' Track P, il contratto storico)."""
    path = record.get("prompts_contract_path") or ""
    return "F" if TRACKF_CONTRACT_MARKER in path else "P"


def load_records(root):
    manifest_dir = root / "manifests"
    records = []
    if not manifest_dir.is_dir():
        return records
    for path in sorted(manifest_dir.glob("*.json")):
        if path.name == "model-ledger.json":
            continue
        try:
            data = json.loads(path.read_text())
        except json.JSONDecodeError:
            print(f"teacher_bench_review: WARN manifest illeggibile, saltato: {path}", file=sys.stderr)
            continue
        data["_record_id"] = path.stem
        data["_track"] = track_of(data)
        records.append(data)
    # Track prima di tutto: raggruppa P e F in due blocchi contigui (la
    # pagina inserisce un'intestazione quando il track cambia, vedi JS sotto)
    # -- dentro un track, stesso ordine di sempre. Rango esplicito (non
    # l'ordine alfabetico della lettera) cosi' Track P, lo storico, resta
    # sempre il primo blocco anche se un giorno un terzo track iniziasse
    # con una lettera prima di "P".
    track_rank = {"P": 0, "F": 1}
    records.sort(key=lambda r: (
        track_rank.get(r.get("_track"), 99),
        r.get("config_id", ""), r.get("subject_id", ""), r.get("seed", 0),
    ))
    return records


def rel_or_none(root, value):
    """I path nel manifest sono gia' relativi alla root (li scrive
    teacher-bench.sh/teacher_bench_post.py cosi'): qui si limita a
    normalizzarli per il browser (sempre '/', mai backslash) e a passare
    None se il campo manca -- niente <img> rotte senza spiegazione."""
    if not value:
        return None
    return str(value).replace("\\", "/")


def build_record_view(root, r):
    postproc = r.get("postproc") or {}
    canvases = postproc.get("canvases") or {}
    previews = postproc.get("previews") or {}
    c32 = canvases.get("32") or {}
    c64 = canvases.get("64") or {}
    p32 = previews.get("32") or {}
    p64 = previews.get("64") or {}
    model = r.get("model") or {}
    lora = r.get("lora") or {}
    extra_lora = r.get("extra_lora") or {}
    track = r.get("_track") or track_of(r)
    # domain/mode/request_id (R2/R3 06/08, scripts/runtime-bench.sh): assenti
    # da ogni manifest teacher-bench (Stage A/Track F), presenti SEMPRE nei
    # manifest runtime-bench -- is_runtime_bench() sotto usa "domain" come
    # unico discriminante, quindi qui basta leggerlo com'e', mai inventarlo.
    domain = r.get("domain")
    return {
        "id": r.get("_record_id"),
        "track": track,
        "config": r.get("config_id"),
        "subject": r.get("subject_id"),
        "category": r.get("category"),
        "seed": r.get("seed"),
        "domain": domain,
        "mode": r.get("mode"),
        "request_id": r.get("request_id"),
        "generation_ok": r.get("generation_ok"),
        "raw": rel_or_none(root, r.get("raw_image")),
        # judge_scale (item 4 Track F + R2/R3): a 64 la card mette il 64
        # PRIMA del 32 -- l'ordine qui e' gia' quello di visualizzazione,
        # renderCard() lo consuma cosi' com'e' invece di rideciderlo in JS.
        # runtime-bench (domain presente) segue lo STESSO stile Track F
        # (vedi scripts/visualspec_template.py, "stile Track F cioe' SENZA
        # divieto outline"): niente contratto proprio da cui leggere un
        # judge_scale, quindi il dominio da solo basta a decidere 64.
        "pixel_primary_scale": 64 if (track == "F" or domain) else 32,
        "pixel64": rel_or_none(root, c64.get("path")),
        "pixel64_nearest": rel_or_none(root, c64.get("path_nearest")),
        "pixel32": rel_or_none(root, c32.get("path")),
        "pixel32_nearest": rel_or_none(root, c32.get("path_nearest")),
        "preview_light_32": rel_or_none(root, p32.get("light")),
        "preview_dark_32": rel_or_none(root, p32.get("dark")),
        "preview_light_64": rel_or_none(root, p64.get("light")),
        "preview_dark_64": rel_or_none(root, p64.get("dark")),
        "postproc_status": postproc.get("status"),
        "postproc_error": postproc.get("error"),
        "meta": {
            "model": model.get("path"),
            "model_sha256": (model.get("sha256") or "")[:12] or None,
            "model_license": model.get("license"),
            "lora": lora.get("path"),
            "lora_weight": lora.get("weight"),
            "lora_trigger": lora.get("trigger"),
            # extra_lora (righe S*, asse velocita'): seconda LoRA sopra la
            # style LoRA -- vedi record["extra_lora"] scritto da run_one() in
            # scripts/teacher-bench.sh.
            "extra_lora": extra_lora.get("path"),
            "extra_lora_weight": extra_lora.get("weight"),
            "steps": r.get("steps"),
            "cfg_scale": r.get("cfg_scale"),
            "sampling_method": r.get("sampling_method"),
            "scheduler": r.get("scheduler"),
            "latency_ms": r.get("latency_ms"),
            "vram_note": r.get("vram_note"),
            "negative_prompt": r.get("negative_prompt"),
            "prompt_full": r.get("prompt_full"),
            # VisualSpec grezzo (R2/R3, solo mode "spec"): mostrare lo spec
            # ORIGINALE accanto al prompt_full assemblato lascia vedere cosa
            # ha scritto Gemma prima che scripts/visualspec_template.py lo
            # trasformasse -- e' la meta' "strutturata" del confronto
            # appaiato che questa pagina deve rendere ispezionabile.
            "spec_json": json.dumps(r.get("spec"), ensure_ascii=False) if r.get("spec") else None,
        },
        "metrics": {
            "foreground_pct_32": c32.get("foreground_pct"),
            "n_colors_32": c32.get("n_colors"),
            "colors_out_of_palette_after_32": c32.get("colors_out_of_palette_after"),
            "silhouette_connected_32": c32.get("silhouette_connected"),
            "contrast_light_32": c32.get("contrast_light"),
            "contrast_dark_32": c32.get("contrast_dark"),
            # equivalenti a 64: judge_scale di Track F, mostrati insieme ai
            # "_32" storici invece di sostituirli -- Track P continua a
            # leggersi sui "_32" come sempre.
            "foreground_pct_64": c64.get("foreground_pct"),
            "n_colors_64": c64.get("n_colors"),
            "colors_out_of_palette_after_64": c64.get("colors_out_of_palette_after"),
            "silhouette_connected_64": c64.get("silhouette_connected"),
            "contrast_light_64": c64.get("contrast_light"),
            "contrast_dark_64": c64.get("contrast_dark"),
            # metriche del RAW (non del canvas): dicono se il fondo era
            # davvero rimovibile, che e' un criterio della matrice a se'
            # stante e non si vede guardando lo sprite finito.
            "bg_removal_mode": postproc.get("bg_removal_mode"),
            "border_occupancy_raw512": postproc.get("border_occupancy_raw512"),
            "components_raw": postproc.get("components_raw"),
        },
    }


def is_runtime_bench(views):
    """True se QUESTA collezione di record e' un giro di scripts/
    runtime-bench.sh (harness R2/R3) e non Stage A/Track F: il discriminante
    e' "domain", un campo che i manifest runtime-bench valorizzano SEMPRE e
    che nessun manifest teacher-bench ha mai scritto. Richiede che TUTTI i
    record lo abbiano (non "almeno uno"): un root misto e' un caso che non
    dovrebbe succedere (le due pipeline scrivono radici diverse per
    costruzione), e se succedesse comunque e' piu' sicuro ricadere sulla
    vista Track P/F esistente (gia' testata) che su una vista nuova che non
    sa come mostrare un record senza domain."""
    return bool(views) and all(v.get("domain") for v in views)


def build_runtime_groups(views):
    """Raggruppa i record per (domain, config, request_id): item 4 R2/R3,
    "raggruppata per DOMINIO poi config, coppie spec/free AFFIANCATE per la
    stessa richiesta". Un gruppo porta SEMPRE due slot, "spec" e "free" (uno
    puo' restare None se una generazione e' mancante/fallita: il confronto
    resta visibile con un placeholder invece di sparire in silenzio).
    Ordine: domain, poi config, poi request_id -- stabile e riproducibile
    (mai l'ordine di iterazione di un dict), cosi' la pagina generata due
    volte sugli stessi manifest produce sempre la stessa sequenza di card."""
    groups = {}
    for v in views:
        key = (v.get("domain") or "", v.get("config") or "", v.get("request_id") or v.get("subject") or "")
        slot = groups.setdefault(key, {"domain": key[0], "config": key[1], "request_id": key[2], "spec": None, "free": None})
        mode = v.get("mode")
        if mode in ("spec", "free"):
            slot[mode] = v
        else:
            # mode ne' "spec" ne' "free" (manifest malformato/futuro): non lo
            # si perde silenziosamente, finisce comunque in uno slot dedicato
            # cosi' la card lo mostra invece di scartarlo.
            slot.setdefault("other", []).append(v)
    ordered_keys = sorted(groups.keys())
    return [groups[k] for k in ordered_keys]


PAGE_TEMPLATE = """<!doctype html>
<meta charset="utf-8">
<title>teacher-bench Stage A — review</title>
<style>
:root { color-scheme: light dark; }
body {
  font-family: -apple-system, "Segoe UI", sans-serif;
  margin: 0; padding: 1.5rem;
  background: #f4f2ec; color: #14100e;
}
@media (prefers-color-scheme: dark) {
  body { background: #1b1710; color: #f4f2ec; }
}
h1 { font-size: 1.3rem; margin: 0 0 .25rem; }
.sub { opacity: .7; font-size: .85rem; margin-bottom: 1rem; }
.toolbar {
  position: sticky; top: 0; z-index: 5;
  display: flex; gap: .75rem; flex-wrap: wrap; align-items: center;
  padding: .75rem; margin-bottom: 1rem; border-radius: 8px;
  background: rgba(127,127,127,.12); backdrop-filter: blur(4px);
}
select, button, input[type=text] {
  font: inherit; padding: .35rem .6rem; border-radius: 6px;
  border: 1px solid rgba(127,127,127,.4); background: transparent; color: inherit;
}
button { cursor: pointer; }
button.primary { background: #b13a1e; color: #fff; border-color: #b13a1e; }
#progress { font-size: .85rem; opacity: .8; }
.grid { display: flex; flex-direction: column; gap: 1rem; }
.card {
  border: 1px solid rgba(127,127,127,.35); border-radius: 10px; padding: .9rem;
  display: grid; grid-template-columns: minmax(280px, 420px) 1fr; gap: 1rem;
}
.card.voted { border-color: #3a7d63; }
.card.hidden, .track-header.hidden { display: none; }
.imgs { display: flex; flex-wrap: wrap; gap: .5rem; align-items: flex-start; }
.imgs figure { margin: 0; text-align: center; font-size: .7rem; opacity: .75; }
.imgs img { display: block; background: repeating-conic-gradient(#8883 0% 25%, transparent 0% 50%) 0 0/16px 16px; border-radius: 4px; }
.imgs img.px { image-rendering: pixelated; width: 128px; height: 128px; }
.imgs img.raw { width: 128px; height: 128px; object-fit: contain; }
.imgs img.scene { width: 200px; height: auto; }
.missing { width: 128px; height: 128px; display: flex; align-items: center; justify-content: center;
  border: 1px dashed rgba(127,127,127,.5); border-radius: 4px; font-size: .65rem; opacity: .6; text-align: center; }
.meta { font-size: .78rem; }
.meta table { border-collapse: collapse; width: 100%; }
.meta td { vertical-align: top; padding: 1px 4px 1px 0; word-break: break-word; }
.meta td.k { opacity: .65; white-space: nowrap; padding-right: .5rem; }
.title { font-weight: 600; margin-bottom: .35rem; }
.badge { display: inline-block; font-size: .7rem; padding: .05rem .4rem; border-radius: 4px; margin-left: .35rem; }
.badge.ok { background: #3a7d6333; }
.badge.fail { background: #7e221633; }
.criteria { margin-top: .6rem; display: grid; grid-template-columns: repeat(auto-fill, minmax(220px,1fr)); gap: .15rem .75rem; }
.criteria label { font-size: .78rem; display: flex; gap: .35rem; align-items: center; }
textarea.note { width: 100%; box-sizing: border-box; margin-top: .4rem; font: inherit; font-size: .78rem;
  background: transparent; color: inherit; border: 1px solid rgba(127,127,127,.4); border-radius: 6px; padding: .3rem; }
.track-header {
  font-size: 1.05rem; font-weight: 700; margin: 1.3rem 0 .3rem;
  padding-bottom: .3rem; border-bottom: 2px solid rgba(127,127,127,.35);
}
.track-header:first-child { margin-top: 0; }
.badge.track { background: rgba(127,127,127,.25); font-weight: 600; }
.badge.speed { background: #2b6cb033; }
.imgs img.px.secondary, .imgs img.scene.secondary { opacity: .55; }
/* pair-row (R2/R3, vista runtime-bench): le due card spec/free della STESSA
   richiesta affiancate -- "e' il confronto che il proprietario deve vedere"
   (mandato). Sotto una certa larghezza vanno in colonna (wrap): affiancate
   quando c'e' spazio, mai un layout che costringe a scorrere in orizzontale. */
.pair-row { margin-bottom: 1.1rem; }
.pair-row.hidden { display: none; }
.pair-label { font-size: .8rem; font-weight: 600; opacity: .75; margin-bottom: .3rem; }
.pair-cards { display: flex; gap: 1rem; flex-wrap: wrap; align-items: flex-start; }
.pair-cards .card { flex: 1 1 420px; margin-bottom: 0; }
</style>

<h1>teacher-bench Stage A — review</h1>
<div class="sub" id="subtitle"></div>

<div class="toolbar">
  <label id="lbl-track">Track: <select id="filter-track"><option value="">tutti</option></select></label>
  <label id="lbl-config">Config: <select id="filter-config"><option value="">tutte</option></select></label>
  <label id="lbl-category">Categoria: <select id="filter-category"><option value="">tutte</option></select></label>
  <label title="si applica quando lo cambi o ricaricando: una card gia' aperta non sparisce a meta' valutazione"><input type="checkbox" id="filter-unvoted"> solo non ancora votate</label>
  <span id="progress"></span>
  <button class="primary" id="export-btn">Esporta voti</button>
  <button id="clear-btn">Azzera voti (locali)</button>
</div>

<div class="grid" id="grid"></div>

<script>
const RECORDS = __RECORDS_JSON__;
const CRITERIA = __CRITERIA_JSON__;
// RUNTIME_GROUPS: vuoto per un root teacher-bench (Stage A/Track F, la vista
// di sempre via init()); popolato SOLO per un root runtime-bench (R2/R3,
// scripts/runtime-bench.sh) -- vedi is_runtime_bench()/build_runtime_groups()
// lato Python. E' il segnale che sceglie il ramo di rendering in fondo al
// file (initRuntime() vs init()).
const RUNTIME_GROUPS = __RUNTIME_GROUPS_JSON__;
const STORAGE_KEY = "teacher_bench_review_votes_v1";

function loadVotes() {
  try { return JSON.parse(localStorage.getItem(STORAGE_KEY)) || {}; }
  catch (e) { return {}; }
}
function saveVotes(votes) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(votes));
}
let VOTES = loadVotes();

function imgOrMissing(src, cls, label) {
  if (!src) {
    return `<div class="missing">${label}<br>assente</div>`;
  }
  return `<figure><img class="${cls}" src="${src}" loading="lazy" alt="${label}"><figcaption>${label}</figcaption></figure>`;
}

function metaRow(k, v) {
  if (v === null || v === undefined || v === "") return "";
  return `<tr><td class="k">${html_(k)}</td><td>${html_(String(v))}</td></tr>`;
}
function html_(s) {
  return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
}

function renderCard(r) {
  const v = VOTES[r.id] || { criteria: new Array(CRITERIA.length).fill(false), note: "" };
  const statusBadge = r.generation_ok === false
    ? '<span class="badge fail">generazione FALLITA</span>'
    : (r.postproc_status === "failed"
        ? `<span class="badge fail">postproc fallito: ${html_(r.postproc_error || "")}</span>`
        : (r.postproc_status === "ok" ? '<span class="badge ok">postproc ok</span>' : '<span class="badge">postproc non ancora eseguito</span>'));

  // Ordine PRIMARIO/secondario (item 4 Track F): Track F giudica a 64x64
  // nativo (judge_scale del suo contratto) quindi mostra il 64 per primo,
  // Track P resta sul 32 -- pixel_primary_scale arriva gia' deciso dal
  // Python (build_record_view), qui si applica soltanto.
  const primary64 = r.pixel_primary_scale === 64;
  const pixelImgs = primary64
    ? [imgOrMissing(r.pixel64, "px primary", "64x64"), imgOrMissing(r.pixel32, "px secondary", "32x32")]
    : [imgOrMissing(r.pixel32, "px primary", "32x32"), imgOrMissing(r.pixel64, "px secondary", "64x64")];
  const previewImgs = primary64
    ? [imgOrMissing(r.preview_light_64, "scene primary", "scena chiara 64"),
       imgOrMissing(r.preview_dark_64, "scene primary", "scena scura 64"),
       imgOrMissing(r.preview_light_32, "scene secondary", "scena chiara 32"),
       imgOrMissing(r.preview_dark_32, "scene secondary", "scena scura 32")]
    : [imgOrMissing(r.preview_light_32, "scene primary", "scena chiara 32"),
       imgOrMissing(r.preview_dark_32, "scene primary", "scena scura 32"),
       imgOrMissing(r.preview_light_64, "scene secondary", "scena chiara 64"),
       imgOrMissing(r.preview_dark_64, "scene secondary", "scena scura 64")];
  const imgs = [imgOrMissing(r.raw, "raw", "raw 512"), ...pixelImgs, ...previewImgs].join("");

  // Righe S* (asse velocita', Track F): badge dedicato con step/CFG/sampler
  // in testa alla card -- e' proprio su questi tre numeri che si confrontano
  // fra loro, non vanno cercati nella tabella metadati sotto.
  // Qui il nome della config E' l'unico segnale disponibile, a differenza del
  // track (che ha "prompts_contract_path" nel manifest): l'asse velocita' non
  // ha un campo suo in nessun artefatto. Se un giorno una riga dell'asse non
  // si chiamera' S*, il badge sparira' in silenzio e questa riga va rifatta
  // su un campo vero -- e' un ripiego dichiarato, non la stessa fonte di
  // verita' di track_of().
  const speedBadge = /^S/.test(r.config || "")
    ? `<span class="badge speed">asse velocita': ${html_(r.meta.steps ?? "?")} step / CFG ${html_(r.meta.cfg_scale ?? "?")} / ${html_(r.meta.sampling_method ?? "?")}</span>`
    : "";

  const metaRows = [
    metaRow("model", r.meta.model),
    metaRow("model sha256", r.meta.model_sha256),
    metaRow("model license", r.meta.model_license),
    metaRow("lora", r.meta.lora),
    metaRow("lora weight", r.meta.lora_weight),
    metaRow("lora trigger", r.meta.lora_trigger),
    metaRow("extra lora (asse velocita')", r.meta.extra_lora),
    metaRow("extra lora weight", r.meta.extra_lora_weight),
    metaRow("steps / cfg", (r.meta.steps ?? "") + " / " + (r.meta.cfg_scale ?? "")),
    metaRow("sampler / scheduler", (r.meta.sampling_method ?? "") + " / " + (r.meta.scheduler ?? "")),
    metaRow("latency", r.meta.latency_ms != null ? (r.meta.latency_ms + " ms") : ""),
    metaRow("vram note", r.meta.vram_note),
    metaRow("prompt", r.meta.prompt_full),
    metaRow("negative", r.meta.negative_prompt),
    metaRow("spec (VisualSpec grezzo)", r.meta.spec_json),
    metaRow("foreground % (32)", r.metrics.foreground_pct_32 != null ? (r.metrics.foreground_pct_32 * 100).toFixed(1) + "%" : ""),
    metaRow("n colori (32)", r.metrics.n_colors_32),
    metaRow("fuori palette dopo (32)", r.metrics.colors_out_of_palette_after_32),
    metaRow("silhouette connessa (32)", r.metrics.silhouette_connected_32),
    metaRow("contrasto chiaro/scuro (32)", r.metrics.contrast_light_32 != null
      ? r.metrics.contrast_light_32.toFixed(2) + " / " + r.metrics.contrast_dark_32.toFixed(2) : ""),
    metaRow("foreground % (64)", r.metrics.foreground_pct_64 != null ? (r.metrics.foreground_pct_64 * 100).toFixed(1) + "%" : ""),
    metaRow("n colori (64)", r.metrics.n_colors_64),
    metaRow("fuori palette dopo (64)", r.metrics.colors_out_of_palette_after_64),
    metaRow("silhouette connessa (64)", r.metrics.silhouette_connected_64),
    metaRow("contrasto chiaro/scuro (64)", r.metrics.contrast_light_64 != null
      ? r.metrics.contrast_light_64.toFixed(2) + " / " + r.metrics.contrast_dark_64.toFixed(2) : ""),
    metaRow("rimozione fondo", r.metrics.bg_removal_mode),
    metaRow("bordo occupato (raw 512)", r.metrics.border_occupancy_raw512 != null
      ? (r.metrics.border_occupancy_raw512 * 100).toFixed(1) + "%" : ""),
    metaRow("componenti nel raw", r.metrics.components_raw),
  ].join("");

  const criteriaHtml = CRITERIA.map((c, i) => `
    <label><input type="checkbox" data-idx="${i}" ${v.criteria[i] ? "checked" : ""}> ${html_(c)}</label>
  `).join("");

  // data-domain/data-mode (R2/R3): sempre presenti sull'attributo (stringa
  // vuota per un record teacher-bench che non li ha), applyRuntimeFilters()
  // sotto li legge -- applyFilters() legacy non li tocca mai, additivo puro.
  const modeBadge = r.mode ? `<span class="badge speed">mode: ${html_(r.mode)}</span>` : "";
  return `
  <div class="card" data-track="${html_(r.track||"")}" data-config="${html_(r.config||"")}" data-category="${html_(r.category||"")}" data-domain="${html_(r.domain||"")}" data-mode="${html_(r.mode||"")}" data-id="${r.id}">
    <div class="imgs">${imgs}</div>
    <div>
      <div class="title">${html_(r.config)} / ${html_(r.subject)} / seed ${html_(r.seed)}
        <span class="badge track">${html_(r.track||"")}</span>
        <span class="badge">${html_(r.category||"")}</span>${modeBadge}${speedBadge}${statusBadge}</div>
      <div class="meta"><table>${metaRows}</table></div>
      <div class="criteria">${criteriaHtml}</div>
      <textarea class="note" rows="2" placeholder="note libere...">${html_(v.note||"")}</textarea>
    </div>
  </div>`;
}

function isVoted(id) {
  const v = VOTES[id];
  return !!v && (v.criteria.some(Boolean) || (v.note && v.note.trim() !== ""));
}

function updateProgress() {
  const total = RECORDS.length;
  const voted = RECORDS.filter(r => isVoted(r.id)).length;
  document.getElementById("progress").textContent = `${voted}/${total} valutate`;
}

function applyFilters() {
  const trk = document.getElementById("filter-track").value;
  const cfg = document.getElementById("filter-config").value;
  const cat = document.getElementById("filter-category").value;
  const onlyUnvoted = document.getElementById("filter-unvoted").checked;
  document.querySelectorAll(".card").forEach(card => {
    const id = card.dataset.id;
    let show = true;
    if (trk && card.dataset.track !== trk) show = false;
    if (cfg && card.dataset.config !== cfg) show = false;
    if (cat && card.dataset.category !== cat) show = false;
    if (onlyUnvoted && isVoted(id)) show = false;
    card.classList.toggle("hidden", !show);
  });
  // Le intestazioni di track seguono le loro card: filtrando su una sola
  // config resterebbe a schermo il titolone dell'altro track con zero card
  // sotto. Si guarda i FRATELLI fino alla prossima intestazione invece di
  // contare le card per data-track: cosi' vale anche se un giorno lo stesso
  // track comparisse in piu' blocchi.
  document.querySelectorAll(".track-header").forEach(h => {
    let visible = false;
    for (let el = h.nextElementSibling; el && !el.classList.contains("track-header"); el = el.nextElementSibling) {
      if (el.classList.contains("card") && !el.classList.contains("hidden")) { visible = true; break; }
    }
    h.classList.toggle("hidden", !visible);
  });
}

// Etichette leggibili delle intestazioni di gruppo (item 4: "raggruppa per
// track"). RECORDS arriva gia' ordinato per track da load_records() in
// Python -- qui si inserisce solo l'intestazione quando il track cambia.
const TRACK_LABELS = {
  P: "Track P — teacher (contratto storico, giudizio a 32x32)",
  F: "Track F — caccia libera all'asset (contratto trackF, giudizio a 64x64 nativo)",
};

function init() {
  document.getElementById("subtitle").textContent =
    `${RECORDS.length} record (config x soggetto x seed) -- generato staticamente, ricarica la pagina dopo un nuovo giro di teacher-bench.sh/teacher_bench_post.py`;

  const tracks = [...new Set(RECORDS.map(r => r.track).filter(Boolean))].sort();
  const configs = [...new Set(RECORDS.map(r => r.config).filter(Boolean))].sort();
  const cats = [...new Set(RECORDS.map(r => r.category).filter(Boolean))].sort();
  const trkSel = document.getElementById("filter-track");
  const cSel = document.getElementById("filter-config");
  const catSel = document.getElementById("filter-category");
  tracks.forEach(t => trkSel.insertAdjacentHTML("beforeend", `<option value="${html_(t)}">${html_(t)}</option>`));
  configs.forEach(c => cSel.insertAdjacentHTML("beforeend", `<option value="${html_(c)}">${html_(c)}</option>`));
  cats.forEach(c => catSel.insertAdjacentHTML("beforeend", `<option value="${html_(c)}">${html_(c)}</option>`));

  const grid = document.getElementById("grid");
  let gridHtml = "";
  let lastTrack = null;
  RECORDS.forEach(r => {
    if (r.track !== lastTrack) {
      gridHtml += `<div class="track-header">${html_(TRACK_LABELS[r.track] || ("Track " + (r.track || "?")))}</div>`;
      lastTrack = r.track;
    }
    gridHtml += renderCard(r);
  });
  grid.innerHTML = gridHtml;
  wireCommon(grid, applyFilters, [trkSel, cSel, catSel]);
}

// wireCommon: voto (input su checkbox/note), progresso, esporta/azzera --
// IDENTICO per la vista legacy (init(), Track P/F) e per quella runtime-bench
// (initRuntime()): opera solo su RECORDS/VOTES/CRITERIA (mai sul layout della
// griglia), quindi non ha bisogno di sapere quale delle due l'ha chiamato.
// 'applyFn' e 'selects' parametrizzano SOLO quali controlli di filtro
// riascoltare "change": la logica di filtro vera e propria resta fuori di
// qui (applyFilters() o applyRuntimeFilters(), diverse fra le due viste).
function wireCommon(grid, applyFn, selects) {
  // NIENTE applyFn() qui: col filtro "solo non ancora votate" attivo la
  // card sparirebbe alla prima spunta, cioe' mentre la si sta ancora
  // valutando (nota non scritta, criteri a meta'). I filtri si applicano solo
  // quando li si cambia o ricaricando la pagina; il bordo verde dice subito
  // che quella card e' ormai votata.
  // "input" e non "change": la textarea con "change" salverebbe la nota solo
  // alla perdita di fuoco, e una nota scritta e mai salvata e' un voto perso.
  grid.addEventListener("input", (ev) => {
    const card = ev.target.closest(".card");
    if (!card) return;
    const id = card.dataset.id;
    const boxes = [...card.querySelectorAll('input[type=checkbox][data-idx]')];
    const note = card.querySelector("textarea.note").value;
    VOTES[id] = { criteria: boxes.map(b => b.checked), note };
    saveVotes(VOTES);
    card.classList.toggle("voted", isVoted(id));
    updateProgress();
  });

  selects.forEach(sel => sel && sel.addEventListener("change", applyFn));
  document.getElementById("filter-unvoted").addEventListener("change", applyFn);

  document.getElementById("export-btn").addEventListener("click", () => {
    const byId = Object.fromEntries(RECORDS.map(r => [r.id, r]));
    // ogni voto esce SIA come array posizionale (compatto, allineato a
    // "criteria") SIA come mappa nome->bool: se un giorno l'ordine dei
    // criteri cambia, un export vecchio resta comunque interpretabile.
    const votes = {};
    for (const [id, v] of Object.entries(VOTES)) {
      const rec = byId[id] || {};
      votes[id] = {
        config: rec.config ?? null,
        subject: rec.subject ?? null,
        seed: rec.seed ?? null,
        criteria: v.criteria,
        criteria_labeled: Object.fromEntries(CRITERIA.map((c, i) => [c, !!v.criteria[i]])),
        note: v.note || "",
      };
    }
    const payload = { exported_at: new Date().toISOString(), criteria: CRITERIA, votes };
    const blob = new Blob([JSON.stringify(payload, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = "teacher-bench-review-votes.json";
    // l'anchor deve stare NEL DOM prima del click (Firefox ignora il click su
    // un elemento non agganciato); la revoca dell'URL va rimandata di un giro
    // di eventi, revocarlo subito dopo il click annulla il download.
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  });
  document.getElementById("clear-btn").addEventListener("click", () => {
    if (!confirm("Azzerare tutti i voti salvati in questo browser?")) return;
    VOTES = {};
    saveVotes(VOTES);
    location.reload();
  });

  document.querySelectorAll(".card").forEach(card => card.classList.toggle("voted", isVoted(card.dataset.id)));
  updateProgress();
}

// ============================================================================
// Vista runtime-bench (R2/R3 06/08, item 4): raggruppa per DOMINIO poi
// CONFIG, coppie spec/free affiancate per la stessa richiesta -- "e' il
// confronto che il proprietario deve vedere" (mandato). Riusa renderCard()
// (un record runtime-bench e' la STESSA forma di view di un record
// teacher-bench, solo con domain/mode/request_id valorizzati) e wireCommon()
// (voto/progresso/esporta sono identici): l'unica cosa che cambia davvero e'
// COME si raggruppano le card in griglia e COME si filtrano.
// ============================================================================
function missingPairCard(domain, config, requestId, mode) {
  return `<div class="card" data-track="" data-config="${html_(config)}" data-category="${html_(domain)}" data-domain="${html_(domain)}" data-mode="${html_(mode)}" data-id="">
    <div class="imgs"><div class="missing">${html_(mode)}<br>nessuna immagine</div></div>
    <div><div class="title">${html_(config)} / ${html_(requestId)}
      <span class="badge fail">mode "${html_(mode)}" assente per questa richiesta</span></div></div>
  </div>`;
}

function applyRuntimeFilters() {
  const dom = document.getElementById("filter-track").value;   // riusato come filtro DOMINIO, vedi initRuntime()
  const cfg = document.getElementById("filter-config").value;
  const onlyUnvoted = document.getElementById("filter-unvoted").checked;
  document.querySelectorAll(".pair-row").forEach(pairRow => {
    let anyVisible = false;
    pairRow.querySelectorAll(".card").forEach(card => {
      const id = card.dataset.id;
      let show = true;
      if (dom && card.dataset.domain !== dom) show = false;
      if (cfg && card.dataset.config !== cfg) show = false;
      if (onlyUnvoted && id && isVoted(id)) show = false;
      card.classList.toggle("hidden", !show);
      if (show) anyVisible = true;
    });
    pairRow.classList.toggle("hidden", !anyVisible);
  });
  document.querySelectorAll(".track-header").forEach(h => {
    let visible = false;
    for (let el = h.nextElementSibling; el && !el.classList.contains("track-header"); el = el.nextElementSibling) {
      if (el.classList.contains("pair-row") && !el.classList.contains("hidden")) { visible = true; break; }
    }
    h.classList.toggle("hidden", !visible);
  });
}

function initRuntime() {
  const nPairs = RUNTIME_GROUPS.length;
  document.getElementById("subtitle").textContent =
    `${nPairs} richieste (dominio x config), ${RECORDS.length} immagini spec/free -- generato staticamente, ricarica la pagina dopo un nuovo giro di runtime-bench.sh/teacher_bench_post.py`;

  // Riuso i tre <select> gia' nel DOM invece di introdurne di nuovi: "Track"
  // diventa "Dominio" (il vero raggruppamento primario qui), "Config" resta
  // se stesso, "Categoria" e' ridondante con dominio e viene nascosto --
  // meno HTML duplicato, un solo layout di toolbar per le due viste.
  document.getElementById("lbl-track").firstChild.textContent = "Dominio: ";
  document.getElementById("lbl-category").style.display = "none";

  const domains = [...new Set(RUNTIME_GROUPS.map(g => g.domain).filter(Boolean))].sort();
  const configs = [...new Set(RUNTIME_GROUPS.map(g => g.config).filter(Boolean))].sort();
  const trkSel = document.getElementById("filter-track");
  const cSel = document.getElementById("filter-config");
  domains.forEach(d => trkSel.insertAdjacentHTML("beforeend", `<option value="${html_(d)}">${html_(d)}</option>`));
  configs.forEach(c => cSel.insertAdjacentHTML("beforeend", `<option value="${html_(c)}">${html_(c)}</option>`));

  const grid = document.getElementById("grid");
  let gridHtml = "";
  let lastDomain = null;
  RUNTIME_GROUPS.forEach(g => {
    if (g.domain !== lastDomain) {
      gridHtml += `<div class="track-header">Dominio: ${html_(g.domain)}</div>`;
      lastDomain = g.domain;
    }
    const specCard = g.spec ? renderCard(g.spec) : missingPairCard(g.domain, g.config, g.request_id, "spec");
    const freeCard = g.free ? renderCard(g.free) : missingPairCard(g.domain, g.config, g.request_id, "free");
    gridHtml += `<div class="pair-row"><div class="pair-label">${html_(g.config)} / ${html_(g.request_id)}</div><div class="pair-cards">${specCard}${freeCard}</div></div>`;
  });
  grid.innerHTML = gridHtml;
  wireCommon(grid, applyRuntimeFilters, [trkSel, cSel]);
}

if (RUNTIME_GROUPS.length) { initRuntime(); } else { init(); }
</script>
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=str(DEFAULT_ROOT), help="radice artifacts/image-model-research")
    ap.add_argument("--out", default="review.html", help="nome file di output, scritto dentro --root")
    args = ap.parse_args()

    root = Path(args.root)
    records = load_records(root)
    if not records:
        print(f"teacher_bench_review: nessun manifest trovato sotto {root}/manifests -- pagina vuota comunque scritta", file=sys.stderr)

    views = [build_record_view(root, r) for r in records]
    # RUNTIME_GROUPS (item 4 R2/R3): [] per un root teacher-bench -- il JS
    # sceglie init() legacy quando l'array e' vuoto, vedi fondo di
    # PAGE_TEMPLATE. is_runtime_bench() e' l'UNICO punto che decide quale
    # vista rendere: se un giorno cambia il discriminante, cambia solo li'.
    runtime_groups = build_runtime_groups(views) if is_runtime_bench(views) else []

    # "</" -> "<\/" : i metadati finiscono dentro un <script>, e un prompt che
    # per caso contenesse "</script>" chiuderebbe il blocco a meta' pagina.
    # Dentro una stringa JS "<\/" vale esattamente "</", quindi il dato non
    # cambia.
    def embed(value):
        return json.dumps(value, ensure_ascii=False).replace("</", "<\\/")

    page = PAGE_TEMPLATE.replace("__RECORDS_JSON__", embed(views))
    page = page.replace("__CRITERIA_JSON__", embed(HUMAN_CRITERIA))
    page = page.replace("__RUNTIME_GROUPS_JSON__", embed(runtime_groups))

    out_path = root / args.out
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(page, encoding="utf-8")
    print(f"teacher_bench_review: {len(views)} record -> {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
