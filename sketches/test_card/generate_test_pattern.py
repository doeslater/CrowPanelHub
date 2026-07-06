"""
Recreates (in code, not by loading a reference photo) the classic Philips
PM5544-style B/W test card -- checkered border, gray grid background, and a
center circle divided into the five bands a real test card uses to check
geometry, bandwidth/resolution, and grayscale tracking -- as a 400x300
monochrome bitmap, sent over serial to whichever sketch in this repo is
currently flashed and implements the wire protocol (receive_image.ino,
test_card.ino). No firmware change is needed for this -- both sketches only
ever draw whatever bitmap they're given (and stamp the "last updated"
timestamp themselves), so this is the test-card equivalent of the
checkerboard pattern in this folder's own send_checkerboard.py (a vendored
copy of receive_image/send_checkerboard.py -- see serial_sender.py for why):
same wire protocol, same board, different payload.

Band proportions and the border/grid cell size were measured from
CompletePattern.jpg (a reference photo of the real card, kept alongside this
script for comparison) using pixel-profile sampling -- see the band height
fractions below. The one genuinely grayscale element (the step gradient) is
drawn with real gray values and left for Image.convert("1")'s default
Floyd-Steinberg dithering to turn into a halftone; every other element is
drawn pure black/white so dithering has no effect on it (zero quantization
error on 0/255 pixels).

Usage:
    python3 generate_test_pattern.py             # save a PNG preview only
    python3 generate_test_pattern.py --send      # also send to the board

Requires Pillow (`pip install pillow`).
"""

import argparse
import datetime
import os

from PIL import Image, ImageDraw, ImageFont, ImageOps

from config_h import DISPLAY_HEIGHT, DISPLAY_WIDTH, PAYLOAD_SIZE
from serial_sender import send_payload

PREVIEW_PATH = os.path.join(os.path.dirname(__file__), "preview.png")

LABEL_HEIGHT = 20  # must match receive_image.ino's drawUpdatedLabel()

BORDER_CELL = 20  # checkerboard border thickness and cell size (square cells)
GRID_CELL = 20  # background grid line pitch, inside the border
CIRCLE_RADIUS = 105
BACKGROUND_GRAY = 210

# Circle band heights, as fractions of the diameter -- measured from
# CompletePattern.jpg by sampling per-row pixel brightness to find where each
# band starts/ends: top split, grating, checkerboard, gradient, bottom split.
# Must sum to 1.0.
BAND_FRACTIONS = [0.17, 0.20, 0.25, 0.225, 0.155]

# Also measured from CompletePattern.jpg: how far down the top split's white
# portion (top-right) and the bottom split's full-black portion (top of that
# band) extend, as a fraction of their band's height.
TOP_SPLIT_WHITE_FRACTION = 0.63
BOTTOM_SPLIT_BLACK_FRACTION = 0.565

CHECKERBOARD_BLOCK = 26  # target size (px) for the checkerboard band's squares


def draw_checkerboard_border(draw):
    """Only the outermost ring of cells is ever visible -- the interior ones
    would just get painted over by the background gray fill next -- so this
    skips straight to drawing the ring instead of the full grid."""
    cols, rows = DISPLAY_WIDTH // BORDER_CELL, DISPLAY_HEIGHT // BORDER_CELL
    for col in range(cols):
        for row in range(rows):
            if 0 < col < cols - 1 and 0 < row < rows - 1:
                continue
            color = 0 if (col + row) % 2 == 0 else 255
            x, y = col * BORDER_CELL, row * BORDER_CELL
            draw.rectangle([x, y, x + BORDER_CELL, y + BORDER_CELL], fill=color)


def draw_background_grid_lines(draw):
    """Drawn AFTER dithering, directly onto the 1-bit image -- a thin pure-white
    line drawn before dithering would still convert cleanly, but visually it
    gets lost among the dithered gray fill's own white speckle. Drawing on top
    of the final image guarantees the grid reads clearly regardless."""
    left, top = BORDER_CELL, BORDER_CELL
    right, bottom = DISPLAY_WIDTH - BORDER_CELL, DISPLAY_HEIGHT - BORDER_CELL
    for x in range(left, right + 1, GRID_CELL):
        draw.line([(x, top), (x, bottom)], fill=255, width=2)
    for y in range(top, bottom + 1, GRID_CELL):
        draw.line([(left, y), (right, y)], fill=255, width=2)


def draw_top_split(draw, x0, x1, cx, y0, y1):
    """Left half black for the whole band; right half white, then black --
    matches the reference photo's top-of-circle quadrant split."""
    draw.rectangle([x0, y0, cx, y1], fill=0)
    right_white_bottom = y0 + round((y1 - y0) * TOP_SPLIT_WHITE_FRACTION)
    draw.rectangle([cx, y0, x1, right_white_bottom], fill=255)
    draw.rectangle([cx, right_white_bottom, x1, y1], fill=0)


def draw_bottom_split(draw, x0, x1, cx, y0, y1):
    """Mirror of draw_top_split (black-full first, then white-left/black-right)
    -- the reference card has 180-degree rotational symmetry, not a simple
    vertical mirror, so left/right are swapped relative to the top split."""
    black_bottom = y0 + round((y1 - y0) * BOTTOM_SPLIT_BLACK_FRACTION)
    draw.rectangle([x0, y0, x1, black_bottom], fill=0)
    draw.rectangle([x0, black_bottom, cx, y1], fill=255)
    draw.rectangle([cx, black_bottom, x1, y1], fill=0)


def draw_grating(draw, x0, x1, y0, y1):
    """Vertical black/white stripes, coarse to fine left-to-right -- a
    simplified stand-in for the real card's bandwidth-sweep grating."""
    widths = [8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 2, 2]
    x, black, i = x0, True, 0
    while x < x1:
        w = widths[min(i, len(widths) - 1)]
        draw.rectangle([x, y0, min(x + w, x1), y1], fill=0 if black else 255)
        x += w
        black = not black
        i += 1


def draw_checkerboard_band(draw, x0, x1, y0, y1, block=CHECKERBOARD_BLOCK):
    rows = max(1, round((y1 - y0) / block))
    cols = max(1, round((x1 - x0) / block))
    row_h = (y1 - y0) / rows
    col_w = (x1 - x0) / cols
    for r in range(rows):
        for c in range(cols):
            color = 0 if (r + c) % 2 == 0 else 255
            draw.rectangle(
                [x0 + c * col_w, y0 + r * row_h, x0 + (c + 1) * col_w, y0 + (r + 1) * row_h],
                fill=color,
            )


def draw_gradient(draw, x0, x1, y0, y1):
    """Dark-to-light steps, left to right -- the one band with real grayscale,
    left for the final convert("1") to dither into a halftone."""
    steps = [30, 90, 150, 210, 255]
    step_w = (x1 - x0) / len(steps)
    for i, value in enumerate(steps):
        draw.rectangle([x0 + i * step_w, y0, x0 + (i + 1) * step_w, y1], fill=value)


def build_test_pattern():
    cx, cy = DISPLAY_WIDTH // 2, DISPLAY_HEIGHT // 2
    r = CIRCLE_RADIUS

    image = Image.new("L", (DISPLAY_WIDTH, DISPLAY_HEIGHT), color=BACKGROUND_GRAY)
    draw = ImageDraw.Draw(image)
    draw_checkerboard_border(draw)

    content = Image.new("L", (DISPLAY_WIDTH, DISPLAY_HEIGHT), color=255)
    cdraw = ImageDraw.Draw(content)
    x0, x1 = cx - r, cx + r
    y = cy - r
    band_heights = [round(f * 2 * r) for f in BAND_FRACTIONS]
    band_heights[-1] += 2 * r - sum(band_heights)  # absorb rounding into the last band
    y_bounds = [y]
    for h in band_heights:
        y += h
        y_bounds.append(y)

    draw_top_split(cdraw, x0, x1, cx, y_bounds[0], y_bounds[1])
    draw_grating(cdraw, x0, x1, y_bounds[1], y_bounds[2])
    draw_checkerboard_band(cdraw, x0, x1, y_bounds[2], y_bounds[3])
    draw_gradient(cdraw, x0, x1, y_bounds[3], y_bounds[4])
    draw_bottom_split(cdraw, x0, x1, cx, y_bounds[4], y_bounds[5])

    mask = Image.new("L", (DISPLAY_WIDTH, DISPLAY_HEIGHT), 0)
    ImageDraw.Draw(mask).ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)
    image.paste(content, (0, 0), mask)

    dithered = image.convert("1")

    # Grid lines are composited on top of the dithered image (see
    # draw_background_grid_lines) so they stay crisp, but that would otherwise
    # draw them straight over the circle -- paste them through the inverse of
    # the circle mask so they only ever land in the background.
    grid_layer = dithered.copy()
    draw_background_grid_lines(ImageDraw.Draw(grid_layer))
    dithered.paste(grid_layer, (0, 0), ImageOps.invert(mask))

    packed = dithered.tobytes()  # PIL convention: bit=1 -> white, bit=0 -> black
    payload = bytes(b ^ 0xFF for b in packed)  # firmware convention: bit=1 -> black, bit=0 -> white
    assert len(payload) == PAYLOAD_SIZE, f"expected {PAYLOAD_SIZE} bytes, got {len(payload)}"
    return payload, dithered


def with_simulated_timestamp_label(image, epoch_seconds):
    """Overlays a bottom label matching receive_image.ino's drawUpdatedLabel(),
    purely so the saved preview shows what the panel will actually display --
    the real label is drawn by the firmware itself, not sent in the payload."""
    preview = image.convert("L")
    draw = ImageDraw.Draw(preview)
    label_y = DISPLAY_HEIGHT - LABEL_HEIGHT
    draw.rectangle([0, label_y, DISPLAY_WIDTH, DISPLAY_HEIGHT], fill=255)
    timestamp = datetime.datetime.fromtimestamp(epoch_seconds, tz=datetime.timezone.utc)
    label = timestamp.strftime("%Y-%m-%d %H:%M:%S")
    font = ImageFont.load_default(size=16)
    draw.text((5, label_y + 2), label, fill=0, font=font)
    return preview


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--send", action="store_true", help="also send the pattern to the board")
    args = parser.parse_args()

    payload, image = build_test_pattern()

    now_utc = datetime.datetime.now(datetime.timezone.utc)
    preview = with_simulated_timestamp_label(image, int(now_utc.timestamp()))
    preview.save(PREVIEW_PATH)
    print(f"--- saved preview to {PREVIEW_PATH} ---")

    if args.send:
        send_payload(payload)


if __name__ == "__main__":
    main()
