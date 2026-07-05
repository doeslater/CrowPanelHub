"""
Renders flowchart.png: a diagram of test_card.ino's boot-to-loop lifecycle,
from power-on through the built-in PM5544-style test-card self-test render
and then the same wire-protocol frame handling receive_image.ino uses.
Every node's wording matches the actual code/log messages in test_card.ino,
so README.md and the diagram can be read side by side with the source.

Same approach as sketches/receive_image/generate_flowchart.py (see that file's
docstring for why: no graphviz/mermaid installed, so this hand-draws boxes,
diamonds, and connector arrows directly with Pillow). Kept as a separate,
self-contained script rather than a shared import, matching this repo's
convention of each firmware folder being self-contained (e.g. config.h is
duplicated per folder rather than shared too).

Usage:
    python3 generate_flowchart.py

Requires Pillow (`pip install pillow`).
"""

import os

from PIL import Image, ImageDraw, ImageFont

# Resolved against this script's own location, not the current working
# directory, so "run from the repo root" (see README.md) saves next to this
# script rather than into the repo root.
OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "flowchart.png")

MAIN_X = 560  # center x of the main (happy-path) column -- kept well clear of
              # the canvas's left edge since a loop-back arc swings left of it
MAIN_W = 560  # width of main-path boxes/diamonds
GAP = 110     # vertical gap between stacked main-path nodes

BAIL_X = 1540  # center x of the shared "bail out" box
BAIL_W = 400
BAIL_LANE_X = 950  # vertical trunk each "no" branch travels down before
                    # turning right into the bail box

RETURN_BAIL_X = 1790   # far-right lane: bail box -> back to the top decision
RETURN_OK_X = 1690     # separate far-right lane: successful render -> back to the top

FONT_SIZE = 17
FONT = ImageFont.load_default(size=FONT_SIZE)

BG = (255, 255, 255)
BOX_FILL = (235, 244, 255)
DIAMOND_FILL = (255, 244, 214)
BAIL_FILL = (255, 227, 227)
START_FILL = (223, 245, 223)
LINE = (20, 20, 20)
TEXT = (10, 10, 10)


def draw_centered_text(draw, cx, cy_, text, fill=TEXT):
    lines = text.split("\n")
    line_h = FONT_SIZE + 6
    total_h = len(lines) * line_h
    y = cy_ - total_h / 2
    for line in lines:
        w = draw.textbbox((0, 0), line, font=FONT)[2]
        draw.text((cx - w / 2, y), line, font=FONT, fill=fill)
        y += line_h


def box(draw, cx, cy_, w, h, text, fill=BOX_FILL):
    x0, y0, x1, y1 = cx - w / 2, cy_ - h / 2, cx + w / 2, cy_ + h / 2
    draw.rounded_rectangle([x0, y0, x1, y1], radius=14, fill=fill, outline=LINE, width=2)
    draw_centered_text(draw, cx, cy_, text)


def oval(draw, cx, cy_, w, h, text, fill=START_FILL):
    x0, y0, x1, y1 = cx - w / 2, cy_ - h / 2, cx + w / 2, cy_ + h / 2
    draw.ellipse([x0, y0, x1, y1], fill=fill, outline=LINE, width=2)
    draw_centered_text(draw, cx, cy_, text)


def diamond(draw, cx, cy_, w, h, text):
    pts = [(cx, cy_ - h / 2), (cx + w / 2, cy_), (cx, cy_ + h / 2), (cx - w / 2, cy_)]
    draw.polygon(pts, fill=DIAMOND_FILL, outline=LINE, width=2)
    draw_centered_text(draw, cx, cy_, text)


def arrowhead(draw, x, y, direction):
    size = 8
    if direction == "down":
        pts = [(x, y), (x - size, y - size), (x + size, y - size)]
    elif direction == "up":
        pts = [(x, y), (x - size, y + size), (x + size, y + size)]
    elif direction == "right":
        pts = [(x, y), (x - size, y - size), (x - size, y + size)]
    else:  # left
        pts = [(x, y), (x + size, y - size), (x + size, y + size)]
    draw.polygon(pts, fill=LINE)


def vline(draw, x, y1, y2, label=None, label_x_offset=10):
    draw.line([(x, y1), (x, y2)], fill=LINE, width=2)
    arrowhead(draw, x, y2, "down" if y2 > y1 else "up")
    if label:
        draw.text((x + label_x_offset, (y1 + y2) / 2), label, font=FONT, fill=TEXT, anchor="lm")


def hline(draw, x1, x2, y, label=None):
    draw.line([(x1, y), (x2, y)], fill=LINE, width=2)
    arrowhead(draw, x2, y, "right" if x2 > x1 else "left")
    if label:
        draw.text(((x1 + x2) / 2, y - 22), label, font=FONT, fill=TEXT, anchor="mm")


def self_loop_left(draw, cx, cy_, w, h, label, radius=70):
    x = cx - w / 2
    draw.line([(x, cy_ - h / 4), (x - radius, cy_ - h / 4), (x - radius, cy_ + h / 4), (x, cy_ + h / 4)],
              fill=LINE, width=2)
    arrowhead(draw, x, cy_ + h / 4, "right")
    draw.text((x - radius - 4, cy_), label, font=FONT, fill=TEXT, anchor="rm")


def main():
    y = 90
    nodes = {}

    def place(key, h):
        nonlocal y
        nodes[key] = y
        y += h + GAP
        return key

    place("start", 70)
    place("setup", 170)
    place("d_avail", 100)
    place("d_sync", 120)
    place("d_header", 110)
    place("d_len", 110)
    place("d_payload", 120)
    place("d_checksum_read", 110)
    place("d_checksum_match", 120)
    place("format_label", 80)
    place("render", 220)
    place("log_ok", 70)

    def cy(key):
        return nodes[key]

    canvas_h = y + 60
    canvas_w = RETURN_BAIL_X + 150
    img = Image.new("RGB", (canvas_w, canvas_h), BG)
    draw = ImageDraw.Draw(img)

    bail_sources = [
        ("d_header", "no: header read timed out"),
        ("d_len", "no: bad length"),
        ("d_payload", "no: payload read timed out"),
        ("d_checksum_read", "no: checksum read timed out"),
        ("d_checksum_match", "no: checksum mismatch"),
    ]
    bail_cy = sum(cy(k) for k, _ in bail_sources) / len(bail_sources)
    bail_h = 190

    # --- main-path nodes ---
    oval(draw, MAIN_X, cy("start"), 260, 70, "Board boot / reset")
    box(draw, MAIN_X, cy("setup"), MAIN_W, 170,
        "setup()\nSerial.setRxBufferSize(15064) before Serial.begin() --\n"
        "large enough to survive the blocking self-test render below\n"
        "power on panel; display.init(), setFullWindow()\n"
        "renderPM5544Card() + renderPayload(..., BUILD_TIMESTAMP) --\n"
        "self-test, centered label; print \"test_card ready, waiting for frames\"")
    diamond(draw, MAIN_X, cy("d_avail"), MAIN_W, 100, "loop():\nbyte available on Serial?")
    diamond(draw, MAIN_X, cy("d_sync"), MAIN_W, 120,
            "receiveFrame(): drain available bytes\nlooking for FRAME_MAGIC (0xA5) -- found it?")
    diamond(draw, MAIN_X, cy("d_header"), MAIN_W, 110,
            "read 8-byte header\n(length + timestamp) within timeout?")
    diamond(draw, MAIN_X, cy("d_len"), MAIN_W, 110, "length == PAYLOAD_SIZE (15000)?")
    diamond(draw, MAIN_X, cy("d_payload"), MAIN_W, 120,
            "read 15,000-byte payload\ninto payload_buffer within timeout?")
    diamond(draw, MAIN_X, cy("d_checksum_read"), MAIN_W, 110, "read 1-byte checksum\nwithin timeout?")
    diamond(draw, MAIN_X, cy("d_checksum_match"), MAIN_W, 120,
            "computed checksum ==\nreceived checksum byte?")
    box(draw, MAIN_X, cy("format_label"), MAIN_W, 80,
        "format \"Last updated: ...\" from the\ntimestamp (localtime_r + strftime)")
    box(draw, MAIN_X, cy("render"), MAIN_W, 220,
        "renderPayload(payload_buffer, label):\n"
        "- fillScreen(GxEPD_WHITE)\n"
        "- drawBitmap(payload_buffer) -- single-color,\n"
        "  set bits draw black, unset bits untouched\n"
        "- draw the label text\n"
        "- display(false) -- physical refresh\n"
        "  (several seconds, blocks)")
    box(draw, MAIN_X, cy("log_ok"), MAIN_W, 70, "print \"frame ok, <label>\"")

    box(draw, BAIL_X, bail_cy, BAIL_W, bail_h,
        "Log the specific error via\nSerial.println (see labels\nat left); receiveFrame()\nreturns false, loop() exits\n(no render this pass)",
        fill=BAIL_FILL)

    # --- main path connectors ---
    vline(draw, MAIN_X, cy("start") + 35, cy("setup") - 85)
    vline(draw, MAIN_X, cy("setup") + 85, cy("d_avail") - 50)
    vline(draw, MAIN_X, cy("d_avail") + 50, cy("d_sync") - 60, "yes, read it")
    vline(draw, MAIN_X, cy("d_sync") + 60, cy("d_header") - 55, "yes (magic found)")
    vline(draw, MAIN_X, cy("d_header") + 55, cy("d_len") - 55, "yes")
    vline(draw, MAIN_X, cy("d_len") + 55, cy("d_payload") - 60, "yes")
    vline(draw, MAIN_X, cy("d_payload") + 60, cy("d_checksum_read") - 55, "yes")
    vline(draw, MAIN_X, cy("d_checksum_read") + 55, cy("d_checksum_match") - 60, "yes")
    vline(draw, MAIN_X, cy("d_checksum_match") + 60, cy("format_label") - 40, "yes, match")
    vline(draw, MAIN_X, cy("format_label") + 40, cy("render") - 110)
    vline(draw, MAIN_X, cy("render") + 110, cy("log_ok") - 35)

    # --- d_avail: nothing waiting yet -> keep polling itself ---
    self_loop_left(draw, MAIN_X, cy("d_avail"), MAIN_W, 100, "no: keep polling", radius=60)

    # --- d_sync: drained all available bytes, no magic found -> back to d_avail ---
    # Enters d_avail's left tip above center (cy - 30), not dead center --
    # the self-loop above enters at cy + 25 and its "no: keep polling" label
    # sits right at cy, so a dead-center entry here would draw a line
    # straight through that label's text.
    x_discard = MAIN_X - MAIN_W / 2 - 160
    discard_entry_y = cy("d_avail") - 30
    draw.line([(MAIN_X - MAIN_W / 2, cy("d_sync")), (x_discard, cy("d_sync"))], fill=LINE, width=2)
    draw.line([(x_discard, cy("d_sync")), (x_discard, discard_entry_y)], fill=LINE, width=2)
    hline(draw, x_discard, MAIN_X - MAIN_W / 2, discard_entry_y)
    draw.text((x_discard + 10, (cy("d_sync") + cy("d_avail")) / 2),
              "no: no magic byte in what\nwas available -- try again\nnext loop() pass",
              font=FONT, fill=TEXT, anchor="lm")

    # --- each validation diamond's "no" edge into the shared bail box ---
    entry_ys = [bail_cy - bail_h / 2 + (i + 1) * bail_h / (len(bail_sources) + 1)
                for i in range(len(bail_sources))]
    for (key, label), entry_y in zip(bail_sources, entry_ys):
        source_y = cy(key)
        draw.line([(MAIN_X + MAIN_W / 2, source_y), (BAIL_LANE_X, source_y)], fill=LINE, width=2)
        draw.text((MAIN_X + MAIN_W / 2 + 12, source_y - 22), label, font=FONT, fill=TEXT, anchor="lm")
        draw.line([(BAIL_LANE_X, source_y), (BAIL_LANE_X, entry_y)], fill=LINE, width=2)
        hline(draw, BAIL_LANE_X, BAIL_X - BAIL_W / 2, entry_y)

    # --- bail box back up to the top of the loop (far-right lane) ---
    to_y = cy("d_avail") - 20
    draw.line([(BAIL_X, bail_cy + bail_h / 2), (BAIL_X, bail_cy + bail_h / 2 + 30)], fill=LINE, width=2)
    draw.line([(BAIL_X, bail_cy + bail_h / 2 + 30), (RETURN_BAIL_X, bail_cy + bail_h / 2 + 30)],
              fill=LINE, width=2)
    vline(draw, RETURN_BAIL_X, bail_cy + bail_h / 2 + 30, to_y)
    hline(draw, RETURN_BAIL_X, MAIN_X + MAIN_W / 2, to_y, label="next loop() pass")

    # --- successful render back up to the top of the loop (separate far-right lane) ---
    ok_to_y = cy("d_avail") + 20
    draw.line([(MAIN_X, cy("log_ok") + 35), (MAIN_X, cy("log_ok") + 60)], fill=LINE, width=2)
    draw.line([(MAIN_X, cy("log_ok") + 60), (RETURN_OK_X, cy("log_ok") + 60)], fill=LINE, width=2)
    vline(draw, RETURN_OK_X, cy("log_ok") + 60, ok_to_y)
    hline(draw, RETURN_OK_X, MAIN_X + MAIN_W / 2, ok_to_y, label="next loop() pass")

    img.save(OUT_PATH)
    print(f"--- saved {OUT_PATH} ({canvas_w}x{canvas_h}) ---")


if __name__ == "__main__":
    main()
