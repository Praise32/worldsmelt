#!/usr/bin/env python3
"""runtime_bench_fusion_compose — composizione immagine per le tecniche (b)
"img2img" e (c) "controlnet" di scripts/runtime_bench_fusion.sh (R3 06/08).
Prende DUE sprite gia' generati e post-processati (pixel-64/<config>/
<reqid>__spec.png, prodotti da scripts/runtime-bench.sh +
scripts/teacher_bench_post.py) e produce UNA immagine 512x512 di ingresso
per sd-cli:

  img2img     -> i due sprite affiancati (con un margine di sovrapposizione
                 verso il centro, cosi' la "fusione" e' gia' visibile nella
                 sorgente e non solo nel prompt), su sfondo grigio piatto
                 (stesso registro di scripts/visualspec_template.py, "flat
                 solid neutral gray background") -- passata a sd-cli come
                 --init-img.
  controlnet  -> la maschera UNIONE dei due canali alpha (stesso
                 posizionamento di img2img, cosi' le due tecniche condizionano
                 sulla STESSA geometria) ridotta al solo contorno (filtro
                 "trova bordi" su una maschera binarizzata): un controllo
                 scribble vuole linee, non masse piene, e il contorno di
                 un'unione di silhouette E' comunque "la maschera unione dei
                 due alpha" richiesta dal task -- solo espressa come bordo
                 invece che come riempimento, la forma che
                 control_v11p_sd15_scribble si aspetta in ingresso.

Uso:
  python3 scripts/runtime_bench_fusion_compose.py img2img sprite_a.png sprite_b.png out.png
  python3 scripts/runtime_bench_fusion_compose.py controlnet sprite_a.png sprite_b.png out.png

Dipendenze: solo stdlib + Pillow.
"""
import argparse
import sys

from PIL import Image, ImageChops, ImageFilter

CANVAS_SIZE = 512
# MULTIPLO INTERO dei 64 px dello sprite sorgente (256 = 4x), mai un fattore
# frazionario: a 224 (3.5x) il nearest-neighbour spalma la stessa riga in 32
# blocchi da 3 px e 32 da 4 px -- una griglia di pixel irregolare proprio
# nell'immagine il cui unico compito e' portare la geometria pixel dentro
# --init-img e --control-image. 256 lascia comunque margine ai lati una volta
# applicato OVERLAP_FRACTION (vedi _placements).
SPRITE_UPSCALE = 256
BG_GRAY = (128, 128, 128)  # stesso "flat solid neutral gray background" del template (visualspec_template.py)
OVERLAP_FRACTION = 0.25   # quanto i due sprite si sovrappongono verso il centro (task: "affiancati/sovrapposti")


def _placements():
    """Posizioni (x, y) fisse dei due sprite sul canvas 512: A a sinistra, B
    a destra, sovrapposti di OVERLAP_FRACTION*SPRITE_UPSCALE verso il centro.
    Identiche per img2img e controlnet (stessa geometria, vedi docstring del
    file) -- una sola funzione cosi' non possono disallinearsi fra le due."""
    overlap = round(SPRITE_UPSCALE * OVERLAP_FRACTION)
    cx = CANVAS_SIZE // 2
    xa = cx - SPRITE_UPSCALE + overlap // 2
    xb = cx - overlap // 2
    y = (CANVAS_SIZE - SPRITE_UPSCALE) // 2
    return (xa, y), (xb, y)


def _load_upscaled(path):
    sprite = Image.open(path).convert("RGBA")
    return sprite.resize((SPRITE_UPSCALE, SPRITE_UPSCALE), resample=Image.Resampling.NEAREST)


def build_img2img_source(path_a, path_b, out_path):
    canvas = Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), BG_GRAY + (255,))
    (xa, ya), (xb, yb) = _placements()
    # B PRIMA di A: A resta "sopra" nella zona di sovrapposizione (ordine
    # arbitrario ma deterministico, documentato qui perche' altrimenti
    # sembrerebbe un caso).
    canvas.alpha_composite(_load_upscaled(path_b), (xb, yb))
    canvas.alpha_composite(_load_upscaled(path_a), (xa, ya))
    canvas.convert("RGB").save(out_path)


def build_controlnet_source(path_a, path_b, out_path):
    (xa, ya), (xb, yb) = _placements()
    union = Image.new("L", (CANVAS_SIZE, CANVAS_SIZE), 0)
    for path, (x, y) in ((path_a, (xa, ya)), (path_b, (xb, yb))):
        alpha = _load_upscaled(path).split()[-1]
        layer = Image.new("L", (CANVAS_SIZE, CANVAS_SIZE), 0)
        layer.paste(alpha, (x, y))
        # ImageChops.lighter = max pixel per pixel: la vera UNIONE dei due
        # alpha (non un OR binario approssimato via paste-con-maschera, che
        # nella fascia di sovrapposizione lascerebbe vincere solo l'ultimo
        # incollato).
        union = ImageChops.lighter(union, layer)
    binary = union.point(lambda v: 255 if v >= 128 else 0)
    edges = binary.filter(ImageFilter.FIND_EDGES)
    edges.convert("RGB").save(out_path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("technique", choices=("img2img", "controlnet"))
    ap.add_argument("sprite_a")
    ap.add_argument("sprite_b")
    ap.add_argument("out_path")
    args = ap.parse_args()

    if args.technique == "img2img":
        build_img2img_source(args.sprite_a, args.sprite_b, args.out_path)
    else:
        build_controlnet_source(args.sprite_a, args.sprite_b, args.out_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
