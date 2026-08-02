#!/usr/bin/env python3
"""remap_fucina — bonifica dei PNG di gioco sulla palette ufficiale Worldsmelt
Fucina (DEC-173, assets/art-src/palette/worldsmelt-fucina.gpl, 31 colori).

Perche' questo script esiste: il verifier HUD ha misurato pixel CIANO nel
gameplay (ragno-di-cenere e altri) fuori dalla palette ufficiale. La bonifica
deve essere deterministica e riproducibile -- non un editor grafico aperto a
mano -- cosi' e' riverificabile e ripetibile se rientrano regressioni da
future generazioni AI o da modifiche manuali.

Cosa fa:
  --audit  (default) elenca, per ogni PNG sotto le cartelle indicate, i
            colori RGB non presenti nella palette (match esatto, alpha=0
            ignorato) con il conteggio pixel. Non scrive nulla.
  --apply   oltre all'audit, riscrive ogni file con almeno un colore fuori
            palette sostituendo ciascun colore con il piu' vicino della
            palette Fucina (distanza in spazio Lab, vedi rgb_to_lab). I file
            gia' interamente in palette restano byte-identici (non vengono
            nemmeno riaperti in scrittura). L'alpha non viene MAI toccato.

Casi speciali nel report (sezione "segnalati"):
  - un colore fuori palette ma quasi-identico al suo sostituto (delta <8 per
    canale R/G/B) viene rimappato senza essere segnalato: e' rumore di
    compressione/anti-aliasing, non una scelta cromatica da rivedere;
  - un colore LONTANO da tutta la palette (es. ciano puro) viene rimappato
    comunque (alla famiglia piu' vicina in Lab) ma SEGNALATO: e' il caso in
    cui vale la pena che un umano riguardi lo sprite dopo la bonifica.

Uso:
  python3 scripts/remap_fucina.py --audit [cartella ...]
  python3 scripts/remap_fucina.py --apply [cartella ...]

Senza cartelle posizionali tratta l'insieme standard di WP-ASSET-4:
enemies, bosses, items, shots, props, tiles, ui sotto assets/art/.
assets/art/character/ (congelato dal proprietario) e assets/art/equip/ (gia'
verificato a parte) restano fuori di proposito -- passali esplicitamente
come argomento se un giorno servono.
"""
import argparse
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
PALETTE_PATH = ROOT / "assets" / "art-src" / "palette" / "worldsmelt-fucina.gpl"
DEFAULT_DIRS = ["enemies", "bosses", "items", "shots", "props", "tiles", "ui"]

# Sotto questo delta per canale un colore fuori palette e' considerato rumore
# (anti-aliasing/compressione) e non compare fra i "casi segnalati" del
# report, anche se viene comunque rimappato come tutti gli altri.
NEAR_IDENTICAL_DELTA = 8

# Sopra questo delta MASSIMO per canale fra il colore originale e il
# sostituto scelto, il colore e' "lontano da tutto" (es. ciano puro) e va
# segnalato esplicitamente: la scelta automatica per famiglia potrebbe non
# bastare a un occhio umano.
FAR_FLAG_DELTA = 60


def load_palette(path=PALETTE_PATH):
    """Legge un .gpl GIMP minimale: righe 'R G B nome', righe '#'/intestazione
    ignorate. Ordine di apparizione preservato (usato solo per stampa)."""
    entries = []
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(None, 3)
        if len(parts) < 3 or not parts[0].lstrip("-").isdigit():
            continue  # intestazioni GIMP (Name/Columns) o righe malformate
        r, g, b = int(parts[0]), int(parts[1]), int(parts[2])
        name = parts[3] if len(parts) > 3 else f"rgb({r},{g},{b})"
        entries.append((r, g, b, name))
    if not entries:
        raise SystemExit(f"palette vuota o illeggibile: {path}")
    return entries


def rgb_to_lab(rgb):
    """sRGB (0-255) -> CIE Lab (D65), implementazione autonoma senza
    dipendenze extra (niente numpy/colormath nel repo): la stessa formula
    standard sRGB->XYZ->Lab a due passi, con la parte lineare a soglia."""
    def inv_gamma(c):
        c = c / 255.0
        return ((c + 0.055) / 1.055) ** 2.4 if c > 0.04045 else c / 12.92

    r, g, b = (inv_gamma(c) for c in rgb)
    x = r * 0.4124 + g * 0.3576 + b * 0.1805
    y = r * 0.2126 + g * 0.7152 + b * 0.0722
    z = r * 0.0193 + g * 0.1192 + b * 0.9505
    xn, yn, zn = 0.95047, 1.0, 1.08883

    def f(t):
        return t ** (1.0 / 3.0) if t > 0.008856 else (7.787 * t) + 16.0 / 116.0

    fx, fy, fz = f(x / xn), f(y / yn), f(z / zn)
    L = 116.0 * fy - 16.0
    a = 500.0 * (fx - fy)
    bb = 200.0 * (fy - fz)
    return (L, a, bb)


class Matcher:
    """Cache del colore Fucina piu' vicino (Lab) per ogni RGB gia' incontrato
    -- gli sprite sono piccoli ma la stessa tavolozza si ripete su molti
    file, la cache evita di ricalcolare Lab per gli stessi colori."""

    def __init__(self, palette):
        self.palette = palette
        self._palette_lab = [(r, g, b, name, rgb_to_lab((r, g, b))) for r, g, b, name in palette]
        self._cache = {}

    def nearest(self, rgb):
        cached = self._cache.get(rgb)
        if cached is not None:
            return cached
        lab = rgb_to_lab(rgb)
        best = None
        best_d = None
        for pr, pg, pb, name, plab in self._palette_lab:
            d = (lab[0] - plab[0]) ** 2 + (lab[1] - plab[1]) ** 2 + (lab[2] - plab[2]) ** 2
            if best_d is None or d < best_d:
                best_d = d
                best = (pr, pg, pb, name)
        self._cache[rgb] = best
        return best


def scan_colors(path):
    """Ritorna {(r,g,b): pixel_count} per i pixel con alpha>0 di un PNG.
    Accesso via load()/getpixel invece di getdata(): quest'ultima e'
    deprecata in Pillow 12+ e la sostituta (get_flattened_data) non e'
    ancora disponibile ovunque nel repo."""
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    px = img.load()
    counts = {}
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            key = (r, g, b)
            counts[key] = counts.get(key, 0) + 1
    return img, counts


def out_of_palette(counts, palette_rgb):
    return {c: n for c, n in counts.items() if c not in palette_rgb}


def resolve_dir(d):
    """Un nome nudo ('enemies') e' relativo a assets/art/; un percorso che
    esiste gia' cosi' com'e' (assoluto o relativo alla cwd) e' usato diretto
    -- serve a poter puntare anche fuori dall'albero standard (es. equip/)."""
    p = Path(d)
    return p if p.exists() else ROOT / "assets" / "art" / d


def iter_png_files(dirs):
    for d in dirs:
        base = resolve_dir(d)
        if not base.exists():
            raise SystemExit(f"cartella non trovata: {d} (provato {base})")
        for p in sorted(base.rglob("*.png")):
            yield p


def audit(dirs, palette):
    palette_rgb = {(r, g, b) for r, g, b, _ in palette}
    report = []  # (path, {color: count})
    for path in iter_png_files(dirs):
        _, counts = scan_colors(path)
        bad = out_of_palette(counts, palette_rgb)
        if bad:
            report.append((path, bad))
    return report


def apply_remap(dirs, palette):
    palette_rgb = {(r, g, b) for r, g, b, _ in palette}
    matcher = Matcher(palette)

    touched = []      # (path, {old: (new, name, flagged)})
    untouched = []     # path gia' in palette
    flagged_far = []   # (path, old, new, name, max_delta) per il rapporto

    for path in iter_png_files(dirs):
        img, counts = scan_colors(path)
        bad = out_of_palette(counts, palette_rgb)
        if not bad:
            untouched.append(path)
            continue

        substitutions = {}
        for old in bad:
            nr, ng, nb, name = matcher.nearest(old)
            max_delta = max(abs(old[0] - nr), abs(old[1] - ng), abs(old[2] - nb))
            flagged = max_delta >= FAR_FLAG_DELTA
            substitutions[old] = (nr, ng, nb, name, flagged, max_delta)
            if flagged:
                flagged_far.append((path, old, (nr, ng, nb), name, max_delta))

        rgba = img.convert("RGBA")
        pixels = rgba.load()
        w, h = rgba.size
        for y in range(h):
            for x in range(w):
                r, g, b, a = pixels[x, y]
                if a == 0:
                    continue
                sub = substitutions.get((r, g, b))
                if sub is not None:
                    nr, ng, nb, _name, _flag, _delta = sub
                    pixels[x, y] = (nr, ng, nb, a)

        rgba.save(path)
        touched.append((path, substitutions))

    return touched, untouched, flagged_far


def rel_display(path):
    """Percorso relativo alla radice del repo per il report; se il file e'
    fuori dall'albero del repo (es. un test puntato su /tmp) torna il
    percorso cosi' com'e' invece di far fallire la stampa."""
    try:
        return path.relative_to(ROOT)
    except ValueError:
        return path


def print_audit_report(report, dirs):
    if not report:
        print(f"Nessun colore fuori palette in {', '.join(dirs)}.")
        return
    total_files = len(report)
    print(f"{total_files} file con colori fuori palette Fucina:\n")
    for path, bad in report:
        rel = rel_display(path)
        print(f"  {rel}")
        for (r, g, b), n in sorted(bad.items(), key=lambda kv: -kv[1]):
            print(f"    rgb({r:3d},{g:3d},{b:3d})  {n:6d} px")
    print()


def print_apply_report(touched, untouched, flagged_far):
    print(f"File intatti (gia' in palette, byte-identici): {len(untouched)}")
    print(f"File riscritti: {len(touched)}\n")

    if touched:
        print("Tabella sostituzioni (per file):")
        for path, subs in touched:
            rel = rel_display(path)
            print(f"\n  {rel}")
            for (r, g, b), (nr, ng, nb, name, flagged, delta) in sorted(
                subs.items(), key=lambda kv: -kv[1][5]
            ):
                mark = "  [SEGNALATO: colore lontano]" if flagged else ""
                near = " (quasi-identico)" if delta < NEAR_IDENTICAL_DELTA else ""
                print(
                    f"    rgb({r:3d},{g:3d},{b:3d}) -> rgb({nr:3d},{ng:3d},{nb:3d}) {name}"
                    f"  delta_max={delta}{near}{mark}"
                )

    if flagged_far:
        print("\nCasi segnalati (colore lontano da tutta la palette, verifica visiva consigliata):")
        for path, old, new, name, delta in flagged_far:
            rel = rel_display(path)
            print(
                f"  {rel}: rgb{old} -> rgb{new} ({name}), delta_max={delta}"
            )
    else:
        print("\nNessun caso segnalato: tutte le sostituzioni erano entro la soglia normale.")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--audit", action="store_true", help="solo report, non scrive nulla (default)")
    mode.add_argument("--apply", action="store_true", help="riscrive i file fuori palette")
    ap.add_argument("dirs", nargs="*", default=DEFAULT_DIRS,
                     help="cartelle sotto assets/art/ (o percorsi assoluti/relativi espliciti)")
    args = ap.parse_args()

    palette = load_palette()

    if args.apply:
        touched, untouched, flagged_far = apply_remap(args.dirs, palette)
        print_apply_report(touched, untouched, flagged_far)
        return 0

    report = audit(args.dirs, palette)
    print_audit_report(report, args.dirs)
    return 1 if report else 0


if __name__ == "__main__":
    sys.exit(main())
