from __future__ import annotations

import argparse
from collections.abc import Iterator
from pathlib import Path

from PIL import Image, ImageDraw, _webp


EXPECTED_FRAMES = 450
SOURCE_FPS = 15


def frame_paths(directory: Path) -> list[Path]:
    paths = sorted(directory.glob("frame-*.png"))
    expected_names = [f"frame-{index:04d}.png" for index in range(EXPECTED_FRAMES)]
    actual_names = [path.name for path in paths]
    if actual_names != expected_names:
        missing = sorted(set(expected_names) - set(actual_names))
        unexpected = sorted(set(actual_names) - set(expected_names))
        detail = []
        if missing:
            detail.append(f"mancanti: {', '.join(missing[:6])}")
        if unexpected:
            detail.append(f"inaspettati: {', '.join(unexpected[:6])}")
        suffix = f" ({'; '.join(detail)})" if detail else ""
        raise SystemExit(
            f"Attesi {EXPECTED_FRAMES} frame numerati da 0000 a 0449, "
            f"trovati {len(paths)} in {directory}{suffix}"
        )
    return paths


def processed_frame(path: Path, width: int, *, quantize: bool = False) -> Image.Image:
    with Image.open(path) as source:
        rgb = source.convert("RGB")
        height = round(rgb.height * width / rgb.width)
        resized = rgb.resize((width, height), Image.Resampling.LANCZOS)
        rgb.close()

    if not quantize:
        return resized

    result = resized.quantize(
        colors=112,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.FLOYDSTEINBERG,
    )
    resized.close()
    return result


def iter_processed(
    paths: list[Path], width: int, *, quantize: bool = False
) -> Iterator[Image.Image]:
    previous: Image.Image | None = None
    try:
        for path in paths:
            if previous is not None:
                previous.close()
            previous = processed_frame(path, width, quantize=quantize)
            yield previous
    finally:
        if previous is not None:
            previous.close()


def write_gif(paths: list[Path], destination: Path) -> None:
    # 15 fps sorgente -> 5 fps. La GIF resta apribile anche nei visualizzatori
    # meno efficienti; il WebP conserva invece tutti i 450 frame.
    sampled = paths[::3]
    first = processed_frame(sampled[0], 640, quantize=True)
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        first.save(
            destination,
            save_all=True,
            append_images=iter_processed(sampled[1:], 640, quantize=True),
            duration=200,
            loop=0,
            disposal=2,
            optimize=False,
        )
    finally:
        first.close()


def write_webp(paths: list[Path], destination: Path) -> None:
    # Pillow converte normalmente append_images in una lista prima di passare
    # i frame al codec WebP. Con 450 immagini 720p questo puo richiedere oltre
    # un gigabyte: usiamo direttamente lo stesso encoder animato di Pillow e
    # gli consegniamo un frame ridotto alla volta.
    width = 960
    duration_ms = round(1000 / SOURCE_FPS)
    encoder = _webp.WebPAnimEncoder(
        (width, 540),
        0xFF080C16,
        0,
        False,
        3,
        5,
        False,
        False,
    )
    timestamp = 0
    destination.parent.mkdir(parents=True, exist_ok=True)
    for path in paths:
        frame = processed_frame(path, width)
        try:
            if frame.size != (width, 540):
                raise SystemExit(
                    f"Formato frame inatteso {frame.size}; la cattura deve essere 16:9."
                )
            encoder.add(frame.getim(), timestamp, False, 86, 100, 6)
        finally:
            frame.close()
        timestamp += duration_ms

    encoder.add(None, timestamp, False, 86, 100, 0)
    data = encoder.assemble("", "", "")
    if data is None:
        raise SystemExit("Il codec WebP non ha prodotto un'animazione.")
    destination.write_bytes(data)


def write_contact_sheet(paths: list[Path], destination: Path) -> None:
    indexes = (37, 112, 187, 262, 337, 412)
    tile_width = 480
    tile_height = 270
    sheet = Image.new("RGB", (tile_width * 3, tile_height * 2), (8, 12, 22))
    draw = ImageDraw.Draw(sheet)

    for slot, frame_index in enumerate(indexes):
        image = processed_frame(paths[frame_index], tile_width)
        x = (slot % 3) * tile_width
        y = (slot // 3) * tile_height
        sheet.paste(image, (x, y))
        image.close()

        seconds = frame_index / SOURCE_FPS
        label = f"t = {seconds:04.1f} s   |   frame {frame_index:04d}"
        draw.rectangle((x + 8, y + tile_height - 30, x + 188, y + tile_height - 8), fill=(5, 9, 18))
        draw.text((x + 14, y + tile_height - 26), label, fill=(225, 235, 245))

    destination.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(destination, optimize=True)
    sheet.close()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Monta i 450 frame della demo procedurale Raylib."
    )
    parser.add_argument("--frames", type=Path, required=True)
    parser.add_argument("--gif", type=Path, required=True)
    parser.add_argument("--webp", type=Path, required=True)
    parser.add_argument("--contact-sheet", type=Path, required=True)
    args = parser.parse_args()

    paths = frame_paths(args.frames)
    write_gif(paths, args.gif)
    write_webp(paths, args.webp)
    write_contact_sheet(paths, args.contact_sheet)

    print(f"GIF: {args.gif}")
    print(f"WebP: {args.webp}")
    print(f"Contact sheet: {args.contact_sheet}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
