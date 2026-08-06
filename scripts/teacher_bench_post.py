#!/usr/bin/env python3
"""teacher_bench_post — postproc DETERMINISTICO Stage A (mandato ricerca
immagini agosto 2026, vedi commento in cima a scripts/teacher-bench.sh per
il contesto completo). Prende ogni raw-512/<config>/<subject>_<seed>.png
prodotto da teacher-bench.sh e produce:

  rimozione sfondo (flood fill dalla cornice, colore di fondo stimato per moda)
  -> crop al bounding box della silhouette principale
  -> centratura in canvas logico 32x32 E 64x64 (downscale BOX + variante NEAREST)
  -> palette Fucina (riusa scripts/remap_fucina.py, non duplica la tavolozza)
  -> pulizia alpha (binarizzazione) + rimozione componenti isolate
  -> validazione silhouette (connessa? quanti colori? quanto fuori palette?)

Scrive pixel-64/, pixel-32/, previews-640x360/ (sprite su scena mock
chiara/scura, in DUE varianti di scala per immagine -- "..._s32"/"..._s64":
Track F giudica a 64x64 nativo, quindi la scena deve mostrare anche
quell'ingombro a schermo raddoppiato, non solo lo storico 32; vedi
"judge_scale" nel contratto letto da teacher-bench.sh e riportato nel
manifest come "prompts_contract_path"), contact-sheets/ (una griglia per
config, sempre a 32: e' un indice visivo rapido, non lo strumento di
giudizio), metrics.csv, e AGGIORNA (non sovrascrive da zero) il campo
"postproc" del manifest JSON per immagine gia' scritto da teacher-bench.sh in
manifests/ -- se il manifest non esiste (es. PNG di test creato a mano, non
da teacher-bench.sh) ne scrive uno minimo, cosi' review.html ha sempre
qualcosa da leggere.

metrics.csv e' INCREMENTALE: viene ricostruito da TUTTI i manifest presenti
(che contengono gia' ogni metrica in "postproc"), non dalle sole immagini
processate in questa invocazione. Processare una sola config non cancella
quindi le righe delle altre -- serve perche' A3/A4 arrivano dopo A0/A1/A2.
Le righe di un metrics.csv preesistente la cui chiave (config, subject,
seed, canvas_size) non ha piu' un manifest vengono conservate (con le colonne
sconosciute vuote) e segnalate a video: sono misure che nessuno puo' piu'
ricalcolare da qui. Se una COLONNA sparisce dallo schema i suoi valori si
perdono nella riscrittura, e anche questo viene detto con un WARN invece che
in silenzio.

Uso:
  python3 scripts/teacher_bench_post.py [--root artifacts/image-model-research]
                                        [--skip-existing] [CONFIG_ID ...]
Senza CONFIG_ID processa tutte le sottocartelle trovate sotto raw-512/.
Default: RICALCOLA tutto (il postproc e' deterministico e costa millisecondi
contro i minuti di GPU della generazione). --skip-existing salta le immagini
gia' processate DALLA STESSA versione della pipeline (confronto sull'hash di
questo file, campo postproc.pipeline_version_sha256): un cambio di soglia o
di algoritmo qui invalida da solo il salto, cosi' non convivono mai nella
stessa cartella derivati prodotti da due pipeline diverse.

--root riusa questa stessa pipeline per un ALTRO harness (R2/R3 06/08,
scripts/runtime-bench.sh -> artifacts/runtime-bench): il default
(artifacts/image-model-research) resta invariato, e metrics.csv guadagna le
colonne opzionali "domain"/"mode" SOLO se i manifest processati le
dichiarano (vedi CSV_OPTIONAL_FIELDS) -- un root teacher-bench puro non le
vede mai, un root runtime-bench le vede sempre.

Dipendenze: solo stdlib + Pillow (verificare con `python3 -c "import PIL"`
prima di lanciare su una macchina nuova).
"""
import argparse
import csv
import hashlib
import json
import sys
import time
from collections import Counter, deque
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import remap_fucina  # noqa: E402  (stessa cartella) -- riusa load_palette()/Matcher, non duplica i 31 colori Fucina

DEFAULT_ROOT = REPO_ROOT / "artifacts" / "image-model-research"

# -- costanti della pipeline, DOCUMENTATE anche nel postproc di ogni immagine
# (matrice benchmark: "downscale controllato e documentato nel metadata") --
BG_FLOOD_TOLERANCE = 24     # delta max per canale rispetto al colore di fondo stimato
BG_BORDER_DOMINANCE_MIN = 0.5   # sotto questa frazione di cornice occupata dal colore
                                # dominante NON esiste un fondo piatto da rimuovere
BG_MAX_REMOVED_FRACTION = 0.99  # oltre questa frazione di pixel spenti il flood ha
                                # mangiato il soggetto: si annulla tutto (vedi sotto)
CONTENT_FRACTION = 0.8      # il contenuto ritagliato occupa questa frazione del lato del canvas
                             # (01-VINCOLI-WORLDSMELT.md del dossier: personaggi ~24-30px su 32 => ~0.75-0.94, 0.8 in mezzo)
ALPHA_BINARIZE_THRESHOLD = 128  # sotto: trasparente; sopra: opaco pieno (pixel art vuole alpha netto, non morbido)
CANVAS_SIZES = (128, 64, 32)  # ordine di generazione (dal piu' ricco: 128 = 512/4, il gradino fra raw e 64, chiesto dal proprietario il 06/08)
SCENE_SIZE = (640, 360)     # canvas logico di gioco (01-VINCOLI-WORLDSMELT.md)
CHECKER_CELL = 32           # stessa scala base sprite (DEC-177/208 del design canonico)

# Colori di scena presi DALLA palette Fucina (stessa fonte del quantizzatore,
# cosi' le due scene mock restano dentro la palette ufficiale) -- vedi
# assets/art-src/palette/worldsmelt-fucina.gpl. Nomi cercati per stringa
# nella tavolozza caricata a runtime (fallback numerico se il nome cambia).
SCENE_LIGHT_NAMES = ("bianco-caldo", "fumo")
SCENE_DARK_NAMES = ("cenere-nera", "ardesia-scura")

# Hash della pipeline (this file): permette di sapere con quale versione del
# postproc e' stata prodotta un'immagine, senza mantenere un changelog a
# mano -- e' l'"hash di pipeline" della lista di metriche automatiche, ed e'
# anche il criterio di invalidazione di --skip-existing.
_PIPELINE_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()[:12]


def log(msg):
    print(f"teacher_bench_post: {msg}")


# ============================================================================
# Passo 1: rimozione sfondo.
#
# Il colore di fondo NON e' quello del singolo angolo: e' la moda dell'intera
# cornice dell'immagine. Un angolo puo' appartenere al soggetto (soggetto
# ancorato a (0,0), ombra lunga, firma del modello) e in quel caso partire da
# li' significa floodare il soggetto invece del fondo. Tre difese, in ordine:
#   1. si stima il colore dominante di bordo e quanto della cornice occupa:
#      sotto BG_BORDER_DOMINANCE_MIN non c'e' nessun fondo piatto e il flood
#      non parte affatto (l'immagine passa intera al crop: la metrica dira'
#      foreground 100% e bordo occupato, che e' esattamente il difetto da
#      misurare, "sfondo facile da rimuovere" della matrice);
#   2. i semi sono i pixel di CORNICE col colore di fondo (tutti, non i soli
#      quattro angoli): un pixel che non corrisponde -- perche' li' c'e' il
#      soggetto -- non diventa mai un seme;
#   3. se il flood spegne piu' di BG_MAX_REMOVED_FRACTION dell'immagine ha
#      per forza attraversato il soggetto (tipico: soggetto e fondo separati
#      da meno della tolleranza): si ANNULLA il flood e si torna al fallback,
#      invece di lasciare un'immagine vuota e marcarla "failed".
# Il confronto e' sempre col colore di fondo stimato, mai a catena col pixel
# adiacente: un fondo con gradiente lieve trascinerebbe il flood dentro il
# soggetto un pixel alla volta.
# ============================================================================
def _border_coords(w, h):
    """Cornice larga 1 px, senza duplicati (gli angoli appartengono a una
    riga E a una colonna). Lista ORDINATA, non un set: l'ordine entra nel
    conteggio della moda (parita' fra due colori) e nell'ordine dei semi del
    flood, e questa pipeline deve dare lo stesso risultato bit per bit a ogni
    esecuzione."""
    seen = set()
    for x in range(w):
        seen.add((x, 0))
        seen.add((x, h - 1))
    for y in range(h):
        seen.add((0, y))
        seen.add((w - 1, y))
    return sorted(seen)


def estimate_background_color(px, border, tolerance):
    """Ritorna (colore_dominante_rgb, frazione_di_cornice_entro_tolleranza)."""
    counts = Counter(px[x, y][:3] for (x, y) in border)
    dominant, _n = counts.most_common(1)[0]
    within = sum(
        n for c, n in counts.items()
        if abs(c[0] - dominant[0]) <= tolerance
        and abs(c[1] - dominant[1]) <= tolerance
        and abs(c[2] - dominant[2]) <= tolerance
    )
    return dominant, within / len(border)


def flood_remove_background(img, tolerance=BG_FLOOD_TOLERANCE):
    """Ritorna (immagine RGBA col fondo spento, info dict). L'immagine di
    partenza non viene mai modificata: il fallback deve poter restituire
    l'originale intatto."""
    rgba = img.convert("RGBA")
    w, h = rgba.size
    border = _border_coords(w, h)
    probe = rgba.load()
    bg_color, dominance = estimate_background_color(probe, border, tolerance)

    info = {
        "bg_color": list(bg_color),
        "bg_border_dominance": dominance,
        "bg_flood_tolerance": tolerance,
        "bg_seeds_used": 0,
        "bg_corners_on_subject": 0,
        "bg_removed_fraction": 0.0,
    }

    if dominance < BG_BORDER_DOMINANCE_MIN:
        info["bg_removal_mode"] = "fallback-no-flat-bg"
        return rgba, info

    def matches(c):
        return (abs(c[0] - bg_color[0]) <= tolerance
                and abs(c[1] - bg_color[1]) <= tolerance
                and abs(c[2] - bg_color[2]) <= tolerance)

    # Semi: TUTTI i pixel di cornice che hanno il colore di fondo, non i soli
    # quattro angoli. Un angolo puo' appartenere al soggetto (e allora non
    # matcha e viene scartato da solo), ma puo' anche essere un pixel di
    # fondo INTRAPPOLATO in una sacca chiusa dal soggetto: partendo solo di
    # li' il flood spegne due pixel e lascia intatto tutto il resto del
    # fondo, con l'aggravante di dichiarare "riuscito" -- verificato su un
    # soggetto a X che tocca i quattro angoli. Seminare dall'intera cornice
    # costa un migliaio di push in coda e toglie di mezzo il caso.
    corners = [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]
    info["bg_corners_on_subject"] = sum(1 for c in corners if not matches(probe[c][:3]))
    seeds = [p for p in border if matches(probe[p][:3])]
    info["bg_seeds_used"] = len(seeds)

    work = rgba.copy()
    px = work.load()
    visited = bytearray(w * h)
    removed = 0
    dq = deque()
    for (sx, sy) in seeds:
        sidx = sy * w + sx
        if not visited[sidx]:
            visited[sidx] = 1
            dq.append((sx, sy))
    while dq:
        x, y = dq.popleft()
        r, g, b, _a = px[x, y]
        px[x, y] = (r, g, b, 0)
        removed += 1
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < w and 0 <= ny < h:
                nidx = ny * w + nx
                if visited[nidx]:
                    continue
                if matches(px[nx, ny][:3]):
                    visited[nidx] = 1
                    dq.append((nx, ny))

    info["bg_removed_fraction"] = removed / (w * h)
    if info["bg_removed_fraction"] > BG_MAX_REMOVED_FRACTION:
        info["bg_removal_mode"] = "fallback-overflow"
        return rgba, info

    info["bg_removal_mode"] = "flood"
    return work, info


# ============================================================================
# Componenti connesse (8-connessione: in pixel art due pixel che si toccano
# solo per lo spigolo restano la STESSA silhouette percepita, a differenza
# della 4-connessione classica -- rilevante soprattutto ai canvas 32/64px).
# Ritorna lista di dict {size, bbox, pixels}: "pixels" serve a chi deve
# spegnere le componenti non principali (rimozione isole), non solo contare.
# ============================================================================
NEI8 = ((-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1))


def connected_components(mask, w, h):
    visited = bytearray(w * h)
    comps = []
    for y in range(h):
        for x in range(w):
            idx = y * w + x
            if not mask[idx] or visited[idx]:
                continue
            stack = [(x, y)]
            visited[idx] = 1
            pixels = []
            minx = maxx = x
            miny = maxy = y
            while stack:
                cx, cy = stack.pop()
                pixels.append((cx, cy))
                if cx < minx: minx = cx
                if cx > maxx: maxx = cx
                if cy < miny: miny = cy
                if cy > maxy: maxy = cy
                for dx, dy in NEI8:
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h:
                        nidx = ny * w + nx
                        if mask[nidx] and not visited[nidx]:
                            visited[nidx] = 1
                            stack.append((nx, ny))
            comps.append({"size": len(pixels), "bbox": (minx, miny, maxx, maxy), "pixels": pixels})
    return comps


def alpha_mask(img):
    w, h = img.size
    px = img.load()
    mask = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            if px[x, y][3] > 0:
                mask[y * w + x] = 1
    return mask, w, h


def strip_non_main_components(img, comps, main):
    """Rende trasparenti i pixel di ogni componente diversa dalla principale
    -- usato sia prima del crop (pulisce residui di flood fill imperfetto)
    sia dopo la quantizzazione (la binarizzazione alpha puo' far riapparire
    scaglie isolate ai bordi)."""
    if len(comps) <= 1:
        return
    px = img.load()
    for comp in comps:
        if comp is main:
            continue
        for x, y in comp["pixels"]:
            px[x, y] = (0, 0, 0, 0)


def border_occupancy(mask, w, h):
    """Frazione dei pixel della cornice che sono opachi, sulla maschera del
    RAW 512 (non sul canvas finale). Sul canvas la centratura a
    CONTENT_FRACTION lascia sempre un margine, quindi li' la metrica sarebbe
    0.0 per costruzione e non direbbe nulla. Qui invece dice quello che
    interessa: il soggetto (o il fondo non rimosso) arriva al bordo del
    fotogramma generato, cioe' il ritaglio non e' affidabile. Stessa
    semantica del gate di scarto gia' in uso nel gioco,
    SpritesOpaqueTouchesBorder (tools/melting-sprites/sprite_atlas.c:58), che
    e' la versione booleana di questa frazione."""
    border = _border_coords(w, h)
    opaque = sum(1 for (x, y) in border if mask[y * w + x])
    return opaque / len(border)


# ============================================================================
# Passo 2: palette Fucina + pulizia alpha -- riusa remap_fucina.Matcher
# (stessa distanza Lab del bonificatore ufficiale degli sprite del gioco:
# un solo posto che decide "quale colore Fucina e' piu' vicino").
# ============================================================================
def quantize_and_clean(canvas, matcher, palette_rgb):
    px = canvas.load()
    w, h = canvas.size

    seen_before = set()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a > 0:
                seen_before.add((r, g, b))
    colors_out_before = len(seen_before - palette_rgb)

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < ALPHA_BINARIZE_THRESHOLD:
                px[x, y] = (0, 0, 0, 0)
                continue
            nr, ng, nb, _name = matcher.nearest((r, g, b))
            px[x, y] = (nr, ng, nb, 255)

    mask, mw, mh = alpha_mask(canvas)
    comps = connected_components(mask, mw, mh)
    silhouette_connected = len(comps) == 1
    components_before_cleanup = len(comps)
    if comps:
        main = max(comps, key=lambda c: c["size"])
        strip_non_main_components(canvas, comps, main)

    seen_after = set()
    opaque = 0
    alpha_valid = True
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                if (r, g, b) != (0, 0, 0):
                    alpha_valid = False  # pulizia alpha incompleta: colore residuo dietro un pixel trasparente
                continue
            if a != 255:
                alpha_valid = False  # binarizzazione fallita (non dovrebbe succedere, controllo onesto)
            opaque += 1
            seen_after.add((r, g, b))
    colors_out_after = len(seen_after - palette_rgb)

    metrics = {
        "foreground_pct": opaque / (w * h),
        "n_colors": len(seen_after),
        "colors_out_of_palette_before": colors_out_before,
        "colors_out_of_palette_after": colors_out_after,
        "components_before_cleanup": components_before_cleanup,
        "silhouette_connected": silhouette_connected,
        "alpha_valid": alpha_valid,
    }
    return canvas, metrics


def relative_luminance(rgb):
    """Luminanza relativa sRGB (formula WCAG), usata solo per un contrasto
    approssimato fondo/soggetto -- non serve precisione fotometrica qui."""
    def lin(c):
        c = c / 255.0
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (lin(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(rgb_a, rgb_b):
    la = relative_luminance(rgb_a) + 0.05
    lb = relative_luminance(rgb_b) + 0.05
    return max(la, lb) / min(la, lb)


def mean_foreground_color(canvas):
    px = canvas.load()
    w, h = canvas.size
    rs = gs = bs = n = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a > 0:
                rs += r; gs += g; bs += b; n += 1
    if n == 0:
        return (128, 128, 128)
    return (rs // n, gs // n, bs // n)


# ============================================================================
# Passo 3: crop + centratura in canvas logico + downscale (BOX + NEAREST)
# ============================================================================
def build_canvases(raw_img, matcher, palette_rgb):
    rgba, bg_info = flood_remove_background(raw_img)
    mask, w, h = alpha_mask(rgba)
    raw_meta = dict(bg_info)
    raw_meta["border_occupancy_raw512"] = border_occupancy(mask, w, h)
    raw_meta["touches_border_raw512"] = raw_meta["border_occupancy_raw512"] > 0.0

    comps_raw = connected_components(mask, w, h)
    raw_meta["components_raw"] = len(comps_raw)
    raw_meta["raw_bbox"] = None
    if not comps_raw:
        return None, raw_meta

    main_raw = max(comps_raw, key=lambda c: c["size"])
    strip_non_main_components(rgba, comps_raw, main_raw)  # scarta isole residue del flood fill prima del crop
    minx, miny, maxx, maxy = main_raw["bbox"]
    crop = rgba.crop((minx, miny, maxx + 1, maxy + 1))
    bw, bh = crop.size
    raw_meta["raw_bbox"] = [minx, miny, maxx, maxy]

    canvases = {}
    for canvas_size in CANVAS_SIZES:
        content_target = max(1, round(canvas_size * CONTENT_FRACTION))
        scale = min(content_target / bw, content_target / bh)
        new_w = max(1, round(bw * scale))
        new_h = max(1, round(bh * scale))
        ox = (canvas_size - new_w) // 2
        oy = (canvas_size - new_h) // 2

        variants = {}
        for method_name, resample in (("box", Image.Resampling.BOX), ("nearest", Image.Resampling.NEAREST)):
            resized = crop.resize((new_w, new_h), resample=resample)
            canvas = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
            canvas.paste(resized, (ox, oy), resized)
            canvas, qmetrics = quantize_and_clean(canvas, matcher, palette_rgb)
            variants[method_name] = (canvas, qmetrics)
        canvases[canvas_size] = variants

    return canvases, raw_meta


# ============================================================================
# Scene mock 640x360 (pavimento a scacchi, non e' il gioco vero: serve solo
# a giudicare la leggibilita' a scala reale, criterio umano 9 della matrice)
# ============================================================================
def resolve_scene_colors(palette):
    by_name = {name: (r, g, b) for r, g, b, name in palette}

    def pick(names, fallback_idx):
        for n in names:
            if n in by_name:
                return by_name[n]
        r, g, b, _n = palette[fallback_idx % len(palette)]
        return (r, g, b)

    light = (pick(SCENE_LIGHT_NAMES[:1], -1), pick(SCENE_LIGHT_NAMES[1:], -2))
    dark = (pick(SCENE_DARK_NAMES[:1], 0), pick(SCENE_DARK_NAMES[1:], 1))
    return light, dark


def make_checker_scene(color_a, color_b):
    scene = Image.new("RGB", SCENE_SIZE, color_a)
    draw = ImageDraw.Draw(scene)
    cols = SCENE_SIZE[0] // CHECKER_CELL + 1
    rows = SCENE_SIZE[1] // CHECKER_CELL + 1
    for cy in range(rows):
        for cx in range(cols):
            if (cx + cy) % 2 == 1:
                x0, y0 = cx * CHECKER_CELL, cy * CHECKER_CELL
                draw.rectangle([x0, y0, x0 + CHECKER_CELL - 1, y0 + CHECKER_CELL - 1], fill=color_b)
    return scene


def composite_on_scene(scene, sprite32):
    out = scene.convert("RGBA").copy()
    x = (SCENE_SIZE[0] - sprite32.width) // 2
    y = (SCENE_SIZE[1] - sprite32.height) // 2
    out.alpha_composite(sprite32, (x, y))
    return out.convert("RGB")


# ============================================================================
# Contact sheet per config: griglia soggetti (colonne) x seed (righe),
# pixel-32 BOX ingrandito 4x nearest per leggibilita', con etichetta.
# ============================================================================
def build_contact_sheet(entries):
    """entries: lista di (subject_id, seed, PIL.Image pixel-32 RGBA)."""
    subjects = sorted({e[0] for e in entries})
    seeds = sorted({str(e[1]) for e in entries})
    if not subjects or not seeds:
        return None
    cell_img = 32 * 4
    label_h = 16
    cell_w, cell_h = cell_img, cell_img + label_h
    sheet = Image.new("RGB", (cell_w * len(subjects), cell_h * len(seeds)), (40, 40, 46))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    by_key = {(sid, str(seed)): img for sid, seed, img in entries}
    for row, seed in enumerate(seeds):
        for col, sid in enumerate(subjects):
            ox, oy = col * cell_w, row * cell_h
            img = by_key.get((sid, seed))
            if img is not None:
                big = img.resize((cell_img, cell_img), resample=Image.Resampling.NEAREST)
                sheet.paste(big.convert("RGB"), (ox, oy))
            label = f"{sid[:14]} {seed}"
            draw.text((ox + 2, oy + cell_img + 2), label, fill=(230, 230, 230), font=font)
    return sheet


# ============================================================================
# Manifest per immagine: legge quello scritto da teacher-bench.sh e ne
# aggiorna SOLO "postproc" (mai gli altri campi, e' territorio dello script
# di generazione); se manca (PNG di test, non da teacher-bench.sh) ne scrive
# uno minimo cosi' review.html ha sempre qualcosa da mostrare.
# ============================================================================
def load_or_init_manifest(manifest_path, config_id, subject_id, seed, raw_rel):
    if manifest_path.exists():
        try:
            return json.loads(manifest_path.read_text())
        except json.JSONDecodeError:
            log(f"WARN manifest illeggibile, ricreato: {manifest_path}")
    return {
        "config_id": config_id,
        "subject_id": subject_id,
        "category": "unknown",
        "seed": seed,
        "model": None,
        "lora": None,
        "prompt_full": None,
        "negative_prompt": None,
        "generation_ok": True,
        "raw_image": raw_rel,
        "postproc": None,
        "_note": "manifest ricostruito da teacher_bench_post.py: nessun manifest di generazione trovato",
    }


def rel(root, path):
    return str(path.relative_to(root))


def write_manifest(manifest_path, manifest):
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")


def split_stem(stem):
    if "_" in stem:
        subject_id, seed_s = stem.rsplit("_", 1)
    else:
        subject_id, seed_s = stem, "0"
    return subject_id, (int(seed_s) if seed_s.isdigit() else seed_s)


def outputs_present(root, config_id, subject_id, seed, manifest):
    """True solo se il manifest dichiara un postproc riuscito DELLA STESSA
    versione di pipeline e tutti i file derivati esistono davvero: un PNG
    cancellato a mano deve far ripartire il lavoro, non essere ereditato."""
    pp = (manifest or {}).get("postproc") or {}
    if pp.get("status") != "ok" or pp.get("pipeline_version_sha256") != _PIPELINE_SHA256:
        return False
    paths = []
    for entry in (pp.get("canvases") or {}).values():
        paths += [entry.get("path"), entry.get("path_nearest")]
    # "previews" e' per-scala (item 3 Track F: 32 E 64, non solo 32) --
    # un manifest scritto dalla pipeline precedente (chiave piatta
    # "preview_light"/"preview_dark") non ha "previews" e fallisce comunque
    # il controllo pipeline_version_sha256 sopra, quindi non arriva mai qui.
    for entry in (pp.get("previews") or {}).values():
        paths += [entry.get("light"), entry.get("dark")]
    return all(p and (root / p).is_file() for p in paths)


def process_one(root, raw_path, matcher, palette_rgb, scenes, skip_existing):
    stem = raw_path.stem
    config_id = raw_path.parent.name
    subject_id, seed = split_stem(stem)

    manifest_path = root / "manifests" / f"{config_id}_{subject_id}_{seed}.json"
    manifest = load_or_init_manifest(manifest_path, config_id, subject_id, seed, rel(root, raw_path))

    if skip_existing and outputs_present(root, config_id, subject_id, seed, manifest):
        px32 = (manifest["postproc"].get("canvases") or {}).get("32", {}).get("path")
        if px32:
            # il contact sheet deve restare completo anche saltando: si
            # rilegge il PNG gia' prodotto invece di rigenerarlo.
            scenes["contact_entries"].setdefault(config_id, []).append(
                (subject_id, seed, Image.open(root / px32).convert("RGBA")))
        log(f"  salto {raw_path.name} (gia' processato dalla pipeline {_PIPELINE_SHA256})")
        return "skipped"

    t0 = time.time()
    try:
        raw_img = Image.open(raw_path)
    except Exception as exc:  # PNG corrotto/troncato: non deve fermare la suite (mai nascondere i fallimenti)
        manifest["postproc"] = {
            "status": "failed",
            "error": f"apertura PNG fallita: {exc}",
            "pipeline_version_sha256": _PIPELINE_SHA256,
        }
        write_manifest(manifest_path, manifest)
        log(f"  FALLITO {raw_path.name}: {exc}")
        return "failed"

    canvases, raw_meta = build_canvases(raw_img, matcher, palette_rgb)
    if canvases is None:
        # con il tetto BG_MAX_REMOVED_FRACTION questo caso resta possibile solo
        # per un PNG interamente trasparente in partenza.
        manifest["postproc"] = {
            "status": "failed",
            "error": "nessun pixel opaco nel raw dopo la rimozione dello sfondo",
            "pipeline_version_sha256": _PIPELINE_SHA256,
            **raw_meta,
        }
        write_manifest(manifest_path, manifest)
        log(f"  FALLITO {raw_path.name}: nessun foreground")
        return "failed"

    postproc = {
        "status": "ok",
        "error": None,
        "pipeline_version_sha256": _PIPELINE_SHA256,
        "content_fraction": CONTENT_FRACTION,
        "downscale_methods": ["box", "nearest"],
        "canvases": {},
        "previews": {},
        **raw_meta,
    }

    contact_sheet_img_32 = None
    preview_dir = root / "previews-640x360" / config_id
    for canvas_size, variants in canvases.items():
        out_dir = root / f"pixel-{canvas_size}" / config_id
        out_dir.mkdir(parents=True, exist_ok=True)
        box_img, box_metrics = variants["box"]
        nearest_img, _nearest_metrics = variants["nearest"]

        box_path = out_dir / f"{subject_id}_{seed}.png"
        nearest_path = out_dir / f"{subject_id}_{seed}__nearest.png"
        box_img.save(box_path)
        nearest_img.save(nearest_path)

        fg_rgb = mean_foreground_color(box_img)
        light_bg, dark_bg = scenes["colors"]
        entry = dict(box_metrics)
        entry["path"] = rel(root, box_path)
        entry["path_nearest"] = rel(root, nearest_path)
        entry["contrast_light"] = contrast_ratio(fg_rgb, light_bg[0])
        entry["contrast_dark"] = contrast_ratio(fg_rgb, dark_bg[0])
        postproc["canvases"][str(canvas_size)] = entry

        # Preview 640x360 PER OGNI scala del canvas (item 3 Track F: "_s32"
        # gia' esistente da prima, "_s64" nuova) -- composite_on_scene NON
        # ridimensiona lo sprite, lo incolla al suo pixel size nativo: e'
        # proprio quello che rende visibile a schermo il raddoppio
        # d'ingombro del canvas 64 rispetto al 32, il punto del giudizio
        # nativo di Track F (judge_scale nel contratto).
        preview_dir.mkdir(parents=True, exist_ok=True)
        light_path = preview_dir / f"{subject_id}_{seed}_light_s{canvas_size}.png"
        dark_path = preview_dir / f"{subject_id}_{seed}_dark_s{canvas_size}.png"
        composite_on_scene(scenes["light"], box_img).save(light_path)
        composite_on_scene(scenes["dark"], box_img).save(dark_path)
        postproc["previews"][str(canvas_size)] = {
            "light": rel(root, light_path),
            "dark": rel(root, dark_path),
        }

        if canvas_size == 32:
            contact_sheet_img_32 = box_img

    postproc["postproc_latency_ms"] = int((time.time() - t0) * 1000)
    manifest["postproc"] = postproc
    write_manifest(manifest_path, manifest)
    log(f"  ok {raw_path.name} ({postproc['postproc_latency_ms']} ms, fondo: {postproc['bg_removal_mode']})")

    if contact_sheet_img_32 is not None:
        scenes["contact_entries"].setdefault(config_id, []).append((subject_id, seed, contact_sheet_img_32))
    return "ok"


# ============================================================================
# metrics.csv -- ricostruito dai manifest (fonte di verita': ogni metrica sta
# gia' in postproc.canvases) e fuso con l'eventuale CSV preesistente.
# Colonne: le metriche automatiche della matrice benchmark. Quelle che NON
# nascono qui (latenza di generazione, nota VRAM, RSS, step, CFG) vengono
# copiate dalla parte di manifest scritta da teacher-bench.sh, cosi' il
# report non deve incrociare due sorgenti a mano. Una generazione fallita non
# ha derivati ma ha un manifest, e infatti compare lo stesso con stato
# gen-failed: e' la ragione per cui il CSV nasce dai manifest e non dalle
# immagini trovate su disco.
# "retries" resta 0: l'harness Stage A non ritenta una generazione fallita
# (resta in failures/), la colonna esiste perche' la matrice la chiede e per
# non cambiare schema il giorno che una politica di retry esistera'.
# "judge_scale" (item 3 Track F): a quale scala va giudicata questa immagine
# ad occhio -- letto dal contratto che l'ha generata (manifest.
# "prompts_contract_path", scritto da teacher-bench.sh), NON da un default
# hardcoded qui: e' il contratto, non questo script, la fonte di verita' su
# cosa si sta giudicando.
# ============================================================================
CSV_FIELDS = [
    "config", "subject", "seed", "canvas_size", "status", "error",
    "foreground_pct", "n_colors",
    "colors_out_of_palette_before", "colors_out_of_palette_after",
    "components_before_cleanup", "silhouette_connected",
    "border_occupancy_raw512", "touches_border_raw512",
    "alpha_valid", "contrast_light", "contrast_dark",
    "raw_bbox", "components_raw", "bg_removal_mode", "bg_border_dominance",
    "downscale_method", "retries",
    "gen_latency_ms", "postproc_latency_ms", "rss_kb", "vram_note",
    "steps", "cfg_scale", "judge_scale", "pipeline_sha256",
]
CSV_KEY = ("config", "subject", "seed", "canvas_size")

# Colonne OPZIONALI (R2/R3 06/08, harness scripts/runtime-bench.sh): "domain"
# e "mode" ("spec"|"free") vivono gia' come campi piatti nei manifest che
# quell'harness scrive, ma un manifest teacher-bench (Stage A/Track F) non li
# ha MAI. Non sono in CSV_FIELDS sopra apposta: "default invariato" (mandato
# R2/R3, "gli output teacher-bench NON devono cambiare") deve restare vero
# per COSTRUZIONE, non per promessa -- se le aggiungessi a CSV_FIELDS ogni
# corsa teacher-bench guadagnerebbe due colonne vuote in piu' e l'header
# cambierebbe SEMPRE, anche processando solo manifest che non le hanno mai
# viste. write_metrics_csv() le attiva (le aggiunge in coda a CSV_FIELDS per
# QUESTA scrittura) solo se compaiono davvero in almeno un manifest o in un
# metrics.csv preesistente -- cosi' un root teacher-bench puro produce un CSV
# byte-identico a prima di queste due righe di codice, e un root
# runtime-bench guadagna le colonne senza bisogno di un flag dedicato.
CSV_OPTIONAL_FIELDS = ["domain", "mode"]

# Cache path-contratto -> judge_scale: letto una volta per file, non una
# volta per manifest (metrics.csv puo' fondere centinaia di manifest che
# condividono lo stesso contratto).
_JUDGE_SCALE_CACHE = {}


def judge_scale_for_contract(contract_path):
    """32 di default (lo judge scale canonico Track P, DEC-177/208): i
    contratti che non dichiarano "judge_scale" -- incluso quello P, mai
    toccato da questo cambio -- restano al canone del gioco finche' non e'
    un contratto esplicito (trackF) a dire 64."""
    if not contract_path:
        return 32
    if contract_path not in _JUDGE_SCALE_CACHE:
        try:
            data = json.loads((REPO_ROOT / contract_path).read_text())
            _JUDGE_SCALE_CACHE[contract_path] = data.get("judge_scale", 32)
        except Exception as exc:  # contratto spostato/cancellato dopo la corsa: non deve rompere il CSV
            log(f"WARN judge_scale non leggibile da {contract_path} ({exc}), uso 32 di default")
            _JUDGE_SCALE_CACHE[contract_path] = 32
    return _JUDGE_SCALE_CACHE[contract_path]


def rows_from_manifest(manifest):
    """Una riga per canvas (32, 64) se il postproc e' andato; una riga sola
    con lo stato altrimenti -- un fallimento non sparisce dal CSV."""
    base = {
        "config": manifest.get("config_id", ""),
        "subject": manifest.get("subject_id", ""),
        "seed": manifest.get("seed", ""),
        "gen_latency_ms": manifest.get("latency_ms"),
        "rss_kb": manifest.get("rss_kb"),
        "vram_note": manifest.get("vram_note"),
        "steps": manifest.get("steps"),
        "cfg_scale": manifest.get("cfg_scale"),
        "judge_scale": judge_scale_for_contract(manifest.get("prompts_contract_path")),
        "retries": 0,
        # Sempre presenti nel dict Python (anche "", per i manifest
        # teacher-bench che non li dichiarano): CSV_OPTIONAL_FIELDS decide
        # SOLO se finiscono nell'header scritto, non se esistono qui --
        # write_metrics_csv() li filtra via row.get(k, "") comunque.
        "domain": manifest.get("domain", ""),
        "mode": manifest.get("mode", ""),
    }
    pp = manifest.get("postproc") or {}
    if manifest.get("generation_ok") is False:
        return [dict(base, canvas_size="", status="gen-failed",
                     error="generazione fallita (vedi failures/)")]
    if not pp:
        return [dict(base, canvas_size="", status="pending",
                     error="postproc non ancora eseguito")]

    shared = {
        "border_occupancy_raw512": pp.get("border_occupancy_raw512"),
        "touches_border_raw512": pp.get("touches_border_raw512"),
        "raw_bbox": " ".join(str(v) for v in pp["raw_bbox"]) if pp.get("raw_bbox") else "",
        "components_raw": pp.get("components_raw"),
        "bg_removal_mode": pp.get("bg_removal_mode"),
        "bg_border_dominance": pp.get("bg_border_dominance"),
        "postproc_latency_ms": pp.get("postproc_latency_ms"),
        "pipeline_sha256": pp.get("pipeline_version_sha256"),
    }
    if pp.get("status") != "ok":
        return [dict(base, **shared, canvas_size="", status=pp.get("status", "failed"),
                     error=pp.get("error") or "")]

    rows = []
    for canvas_size, m in sorted((pp.get("canvases") or {}).items(), key=lambda kv: int(kv[0])):
        rows.append(dict(
            base, **shared,
            canvas_size=canvas_size, status="ok", error="",
            foreground_pct=m.get("foreground_pct"), n_colors=m.get("n_colors"),
            colors_out_of_palette_before=m.get("colors_out_of_palette_before"),
            colors_out_of_palette_after=m.get("colors_out_of_palette_after"),
            components_before_cleanup=m.get("components_before_cleanup"),
            silhouette_connected=m.get("silhouette_connected"),
            alpha_valid=m.get("alpha_valid"),
            contrast_light=m.get("contrast_light"), contrast_dark=m.get("contrast_dark"),
            downscale_method="box",
        ))
    return rows


def row_key(row):
    return tuple(str(row.get(k, "")) for k in CSV_KEY)


def write_metrics_csv(root):
    """Fonde: righe ricostruite da manifests/*.json (autorevoli) + righe di un
    metrics.csv preesistente la cui chiave non ha piu' un manifest (per non
    perdere misure di immagini archiviate/spostate a mano).

    Le colonne effettive di QUESTA scrittura sono CSV_FIELDS + le sole
    CSV_OPTIONAL_FIELDS che risultano "attive" (vedi il commento sopra
    CSV_OPTIONAL_FIELDS): l'attivazione si decide leggendo prima TUTTE le
    righe (manifest correnti + CSV preesistente), mai a meta' scrittura, cosi'
    l'header e' deciso una volta sola e ogni riga lo rispetta."""
    metrics_path = root / "metrics.csv"

    legacy_rows_by_key = {}
    legacy_header = []
    if metrics_path.is_file():
        with open(metrics_path, newline="") as f:
            reader = csv.DictReader(f)
            legacy_header = reader.fieldnames or []
            for old in reader:
                legacy_rows_by_key[row_key(old)] = old

    fresh_rows_by_key = {}
    dropped_columns = set()
    manifest_dir = root / "manifests"
    n_manifests = 0
    if manifest_dir.is_dir():
        for path in sorted(manifest_dir.glob("*.json")):
            if path.name == "model-ledger.json":
                continue
            try:
                manifest = json.loads(path.read_text())
            except json.JSONDecodeError:
                log(f"WARN manifest illeggibile, escluso da metrics.csv: {path}")
                continue
            n_manifests += 1
            for row in rows_from_manifest(manifest):
                fresh_rows_by_key[row_key(row)] = row

    # Attivazione delle colonne opzionali: un manifest di QUESTA corsa le
    # valorizza, OPPURE un metrics.csv preesistente le aveva gia' in header
    # (corsa runtime-bench precedente sullo stesso root, poi rilanciata
    # processando magari solo alcune config) -- in entrambi i casi vanno
    # scritte, altrimenti si perderebbero dati gia' presenti su disco.
    active_optional = [
        f for f in CSV_OPTIONAL_FIELDS
        if f in legacy_header or any(row.get(f) for row in fresh_rows_by_key.values())
    ]
    fields = CSV_FIELDS + active_optional

    merged = {}
    legacy_keys = set(legacy_rows_by_key)
    for key, old in legacy_rows_by_key.items():
        merged[key] = {k: old.get(k, "") for k in fields}
        dropped_columns |= {k for k, v in old.items()
                            if k not in fields and v not in ("", None)}
    for key, row in fresh_rows_by_key.items():
        legacy_keys.discard(key)
        merged[key] = {k: row.get(k, "") for k in fields}

    # Righe rimaste senza manifest: si conservano, ma vanno dette. Sono
    # misure che nessuno puo' piu' ricalcolare da qui (raw o manifest
    # spostati a mano), quindi diventano dati orfani dentro un CSV che per
    # tutto il resto e' rigenerabile.
    if legacy_keys:
        log(f"WARN {len(legacy_keys)} righe di metrics.csv non hanno piu' un manifest e sono "
            f"state conservate cosi' come sono: {sorted(legacy_keys)[:5]}"
            + (" ..." if len(legacy_keys) > 5 else ""))
    if dropped_columns:
        log(f"WARN colonne non piu' nello schema, valori persi nella riscrittura: {sorted(dropped_columns)}")

    rows = sorted(merged.values(), key=lambda r: row_key(r))
    with open(metrics_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for row in rows:
            w.writerow(row)
    log(f"metrics.csv -> {metrics_path} ({len(rows)} righe da {n_manifests} manifest)"
        + (f" [colonne extra attive: {', '.join(active_optional)}]" if active_optional else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("configs", nargs="*", help="config da processare (default: tutte sotto raw-512/)")
    ap.add_argument("--root", default=str(DEFAULT_ROOT), help="radice artifacts/image-model-research")
    ap.add_argument("--skip-existing", action="store_true",
                    help="salta le immagini gia' processate dalla stessa versione di pipeline")
    args = ap.parse_args()

    root = Path(args.root)
    raw_root = root / "raw-512"
    if not raw_root.is_dir():
        print(f"teacher_bench_post: nessuna cartella raw-512 sotto {root}", file=sys.stderr)
        return 1

    configs = args.configs or sorted(p.name for p in raw_root.iterdir() if p.is_dir())
    if not configs:
        print(f"teacher_bench_post: nessuna config trovata sotto {raw_root}", file=sys.stderr)
        return 1

    palette = remap_fucina.load_palette()
    matcher = remap_fucina.Matcher(palette)
    palette_rgb = {(r, g, b) for r, g, b, _ in palette}
    light_colors, dark_colors = resolve_scene_colors(palette)

    (root / "previews-640x360").mkdir(parents=True, exist_ok=True)
    scenes = {
        "colors": (light_colors, dark_colors),
        "light": make_checker_scene(*light_colors),
        "dark": make_checker_scene(*dark_colors),
        "contact_entries": {},
    }
    scenes["light"].save(root / "previews-640x360" / "scene-light-640x360.png")
    scenes["dark"].save(root / "previews-640x360" / "scene-dark-640x360.png")

    tally = {"ok": 0, "failed": 0, "skipped": 0}
    for config_id in configs:
        cfg_dir = raw_root / config_id
        if not cfg_dir.is_dir():
            print(f"teacher_bench_post: config sconosciuta (nessuna cartella): {config_id}", file=sys.stderr)
            continue
        pngs = sorted(cfg_dir.glob("*.png"))
        log(f"[{config_id}] {len(pngs)} immagini raw")
        for raw_path in pngs:
            tally[process_one(root, raw_path, matcher, palette_rgb, scenes, args.skip_existing)] += 1

    contact_dir = root / "contact-sheets"
    contact_dir.mkdir(parents=True, exist_ok=True)
    for config_id, entries in scenes["contact_entries"].items():
        sheet = build_contact_sheet(entries)
        if sheet is not None:
            sheet.save(contact_dir / f"{config_id}.png")
            log(f"contact sheet -> contact-sheets/{config_id}.png ({len(entries)} celle)")

    write_metrics_csv(root)
    log(f"fatto: {tally['ok']} processate, {tally['skipped']} saltate, "
        f"{tally['failed']} fallite su {len(configs)} config")
    return 0


if __name__ == "__main__":
    sys.exit(main())
