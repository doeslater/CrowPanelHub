"""
Renders a PNG approximating what test_card.ino puts on the physical panel,
without needing real hardware -- the card drawing (border, circle bands,
dithering, grid lines) is a line-for-line port of test_card.ino's C++, so
that part is pixel-accurate. The "Last updated: ..." label uses this
machine's own font instead of GxEPD2/Adafruit_GFX's actual built-in font,
so its exact letter shapes are an approximation -- but it's drawn at the
same setCursor()/setTextSize() coordinates the firmware uses, so this is
still the right tool for the actual question that matters: does the label
fit inside the reserved strip, or does it get clipped by the bottom edge
of the panel? Two guide lines mark both edges to make that obvious.

Usage:
    python3 render_preview.py                          # boot self-test, no label
    python3 render_preview.py "Last updated: 2026-07-04 15:35:31"

Saves to boot_preview.png (next to this script) rather than preview.png --
generate_test_pattern.py already uses that name for the full-canvas card it
sends to receive_image.ino, which is a different image (no reserved label
strip, circle centered differently); a shared filename would let one script
silently overwrite the other's output.
"""

import os
import sys

from PIL import Image, ImageDraw, ImageFont

from config_h import DISPLAY_HEIGHT, DISPLAY_WIDTH

BORDER_CELL = 20
GRID_CELL = 20
CIRCLE_RADIUS = 105
BACKGROUND_GRAY = 210
LABEL_HEIGHT = 20
CONTENT_HEIGHT = DISPLAY_HEIGHT - LABEL_HEIGHT
BAND_FRACTIONS = [0.17, 0.20, 0.25, 0.225, 0.155]
TOP_SPLIT_WHITE_FRACTION = 0.63
BOTTOM_SPLIT_BLACK_FRACTION = 0.565
CHECKERBOARD_BLOCK = 26

cx = DISPLAY_WIDTH // 2
cy = CONTENT_HEIGHT // 2


def compute_band_bounds():
    heights = [int(f * 2 * CIRCLE_RADIUS + 0.5) for f in BAND_FRACTIONS]
    heights[4] += 2 * CIRCLE_RADIUS - sum(heights)
    y_bounds = [cy - CIRCLE_RADIUS]
    for h in heights:
        y_bounds.append(y_bounds[-1] + h)
    return y_bounds


y_bounds = compute_band_bounds()


def inside_circle(x, y):
    dx, dy = x - cx, y - cy
    return dx * dx + dy * dy <= CIRCLE_RADIUS * CIRCLE_RADIUS


def band_top_split(x, y, y0, y1):
    if x < cx:
        return 0
    right_white_bottom = y0 + int((y1 - y0) * TOP_SPLIT_WHITE_FRACTION + 0.5)
    return 255 if y < right_white_bottom else 0


def band_grating(query_x, x0):
    widths = [8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 2, 2]
    cur, black = x0, True
    for i in range(200):
        w = widths[i if i < len(widths) else -1]
        if query_x < cur + w:
            return 0 if black else 255
        cur += w
        black = not black
    return 255


def band_checkerboard(x, y, x0, x1, y0, y1):
    cols = max(1, int((x1 - x0) / CHECKERBOARD_BLOCK + 0.5))
    rows = max(1, int((y1 - y0) / CHECKERBOARD_BLOCK + 0.5))
    col_w = (x1 - x0) / cols
    row_h = (y1 - y0) / rows
    col = int((x - x0) / col_w)
    row = int((y - y0) / row_h)
    return 0 if (row + col) % 2 == 0 else 255


def band_gradient(x, x0, x1):
    steps = [30, 90, 150, 210, 255]
    step_w = (x1 - x0) / len(steps)
    idx = int((x - x0) / step_w)
    return steps[max(0, min(idx, len(steps) - 1))]


def band_bottom_split(x, y, y0, y1):
    black_bottom = y0 + int((y1 - y0) * BOTTOM_SPLIT_BLACK_FRACTION + 0.5)
    if y < black_bottom:
        return 0
    return 255 if x < cx else 0


def circle_content_at(x, y):
    x0, x1 = cx - CIRCLE_RADIUS, cx + CIRCLE_RADIUS
    for i in range(5):
        if y_bounds[i] <= y < y_bounds[i + 1]:
            if i == 0:
                return band_top_split(x, y, y_bounds[0], y_bounds[1])
            if i == 1:
                return band_grating(x, x0)
            if i == 2:
                return band_checkerboard(x, y, x0, x1, y_bounds[2], y_bounds[3])
            if i == 3:
                return band_gradient(x, x0, x1)
            return band_bottom_split(x, y, y_bounds[4], y_bounds[5])
    return 255


def border_cell_color(x, y):
    col, row = x // BORDER_CELL, y // BORDER_CELL
    cols, rows = DISPLAY_WIDTH // BORDER_CELL, CONTENT_HEIGHT // BORDER_CELL
    if 0 < col < cols - 1 and 0 < row < rows - 1:
        return None
    return 0 if (col + row) % 2 == 0 else 255


def render_card():
    """Same as test_card.ino's renderPM5544Card() + drawGridLines()."""
    img = Image.new("1", (DISPLAY_WIDTH, DISPLAY_HEIGHT), 1)  # start all white
    px = img.load()

    err_curr = [0] * DISPLAY_WIDTH
    for y in range(CONTENT_HEIGHT):
        err_next = [0] * DISPLAY_WIDTH
        for x in range(DISPLAY_WIDTH):
            if inside_circle(x, y):
                ideal = circle_content_at(x, y)
            else:
                bc = border_cell_color(x, y)
                ideal = bc if bc is not None else BACKGROUND_GRAY

            value = max(0, min(255, ideal + err_curr[x]))
            chosen = 0 if value < 128 else 255
            err = value - chosen

            if x + 1 < DISPLAY_WIDTH:
                err_curr[x + 1] += (err * 7) // 16
            if x > 0:
                err_next[x - 1] += (err * 3) // 16
            err_next[x] += (err * 5) // 16
            if x + 1 < DISPLAY_WIDTH:
                err_next[x + 1] += (err * 1) // 16

            if chosen == 0:
                px[x, y] = 0
        err_curr = err_next

    left, top = BORDER_CELL, BORDER_CELL
    right, bottom = DISPLAY_WIDTH - BORDER_CELL, CONTENT_HEIGHT - BORDER_CELL
    for x in range(left, right + 1, GRID_CELL):
        for y in range(top, bottom + 1):
            for dx in range(2):
                px_x = x + dx
                if px_x <= right and not inside_circle(px_x, y):
                    px[px_x, y] = 1
    for y in range(top, bottom + 1, GRID_CELL):
        for x in range(left, right + 1):
            for dy in range(2):
                px_y = y + dy
                if px_y <= bottom and not inside_circle(x, px_y):
                    px[x, px_y] = 1

    return img


def render_payload(label):
    """Mirrors test_card.ino's renderPayload(): the card, the reserved
    strip cleared white, and (if given) the label at the same
    setCursor(4, CONTENT_HEIGHT + 4) / setTextSize(2) position."""
    img = render_card().convert("L")
    draw = ImageDraw.Draw(img)
    draw.rectangle([0, CONTENT_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT], fill=255)

    if label:
        # Real firmware uses Adafruit_GFX's built-in font at setTextSize(2)
        # (8px cell x2 = 16px tall). This uses a similarly-sized font from
        # this machine as a stand-in -- see this script's docstring.
        font = ImageFont.load_default(size=16)
        draw.text((4, CONTENT_HEIGHT + 4), label, fill=0, font=font)

    # Guide lines -- not part of the real output, just this script's own
    # markers for where the panel's bottom edge and reserved strip are.
    draw.line([(0, CONTENT_HEIGHT), (DISPLAY_WIDTH, CONTENT_HEIGHT)], fill=128, width=1)
    draw.line([(0, DISPLAY_HEIGHT - 1), (DISPLAY_WIDTH, DISPLAY_HEIGHT - 1)], fill=0, width=1)
    return img


OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "boot_preview.png")


def main():
    label = sys.argv[1] if len(sys.argv) > 1 else None
    img = render_payload(label)
    img.save(OUTPUT_PATH)
    print(f"--- saved {OUTPUT_PATH} ---")
    if label:
        print(f'--- label drawn: "{label}" ---')
        print("--- gray guide line = where the reserved strip starts (row 280) ---")
        print("--- black guide line at the very bottom = panel's last row (299) ---")
        print("--- if any text crosses below the black line, it's being clipped ---")


if __name__ == "__main__":
    main()
