"""
Renders flowchart.png: a diagram of receive_image.ino's full one-frame
lifecycle, from the board waiting for a byte on Serial through validating and
rendering a frame. Every node's wording matches the actual code/log messages
in receive_image.ino, so README.md and the diagram can be read side by side
with the source.

No diagramming library (graphviz/mermaid) is installed on this machine and
installing one needs a system package (sudo), so this hand-draws boxes,
diamonds, and connector arrows directly with Pillow -- the same approach
sketches/test_card/generate_test_pattern.py uses for its bitmap. Text is kept
plain-ASCII on purpose: Pillow's bundled default font has no glyphs for
em-dashes/bullets/arrows and silently renders them as "missing glyph" boxes.

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
              # the canvas's left edge since two loop-back arcs swing left of it
MAIN_W = 520  # width of main-path boxes/diamonds
GAP = 110  # vertical gap between stacked main-path nodes -- generous so a
           # two-line "no" label next to a diamond never collides with the
           # "yes" label on the connector above/below it

BAIL_X = 1500  # center x of the shared "bail out" box
BAIL_W = 400
BAIL_LANE_X = 900  # the vertical trunk each "no" branch travels down before
                    # turning right into the bail box, kept well clear of the
                    # main column's own text and of the bail box's labels

RETURN_BAIL_X = 1750   # far-right lane: bail box -> back to the top decision
RETURN_OK_X = 1650     # a separate far-right lane: successful render -> back to the top

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
    """direction: 'down', 'up', 'left', 'right' -- which way the arrow travels."""
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
    """A loop from a node's left edge back into itself, used for 'stay here
    and keep polling' cases that don't go anywhere else."""
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
    place("setup", 100)
    place("d_avail", 100)
    place("d_magic", 110)
    place("d_len_to", 110)
    place("d_len_val", 110)
    place("d_ts_to", 110)
    place("d_payload_to", 120)
    place("d_checksum_to", 110)
    place("d_checksum_match", 120)
    place("log_received", 70)
    place("render", 300)
    place("log_complete", 70)

    def cy(key):
        return nodes[key]

    canvas_h = y + 60
    canvas_w = RETURN_BAIL_X + 150
    img = Image.new("RGB", (canvas_w, canvas_h), BG)
    draw = ImageDraw.Draw(img)

    bail_sources = [
        ("d_len_to", "no: timed out reading length"),
        ("d_len_val", "no: unexpected payload length"),
        ("d_ts_to", "no: timed out reading timestamp"),
        ("d_payload_to", "no: timed out reading payload"),
        ("d_checksum_to", "no: timed out reading checksum"),
        ("d_checksum_match", "no: checksum mismatch"),
    ]
    bail_cy = sum(cy(k) for k, _ in bail_sources) / len(bail_sources)
    bail_h = 190

    # --- main-path nodes ---
    oval(draw, MAIN_X, cy("start"), 260, 70, "Board boot / reset")
    box(draw, MAIN_X, cy("setup"), MAIN_W, 100,
        "setup()\nSerial.begin(115200); setTimeout(5000 ms)\nprint \"Receive Image Controller Starting...\"")
    diamond(draw, MAIN_X, cy("d_avail"), MAIN_W, 100, "loop():\nbyte available on Serial?")
    diamond(draw, MAIN_X, cy("d_magic"), MAIN_W, 110, "read one byte -\nis it FRAME_MAGIC (0xA5)?")
    diamond(draw, MAIN_X, cy("d_len_to"), MAIN_W, 110, "handleFrame():\nread 4-byte length within timeout?")
    diamond(draw, MAIN_X, cy("d_len_val"), MAIN_W, 110, "length == PAYLOAD_SIZE (15000)?")
    diamond(draw, MAIN_X, cy("d_ts_to"), MAIN_W, 110, "read 4-byte timestamp\nwithin timeout?")
    diamond(draw, MAIN_X, cy("d_payload_to"), MAIN_W, 120,
            "read 15,000-byte payload\ninto frameBuffer within timeout?")
    diamond(draw, MAIN_X, cy("d_checksum_to"), MAIN_W, 110, "read 1-byte checksum\nwithin timeout?")
    diamond(draw, MAIN_X, cy("d_checksum_match"), MAIN_W, 120,
            "checksumOf(frameBuffer) ==\nreceived checksum byte?")
    box(draw, MAIN_X, cy("log_received"), MAIN_W, 70, "print \"Frame received, rendering...\"")
    box(draw, MAIN_X, cy("render"), MAIN_W, 300,
        "renderFrame(epochSeconds):\n"
        "- epdPower(HIGH)\n"
        "- epd.init(...) - hardware-reset panel\n"
        "- setRotation(0); setFullWindow()\n"
        "- drawBitmap(frameBuffer)\n"
        "- drawUpdatedLabel(epochSeconds)\n"
        "- epd.display() - physical refresh\n"
        "  (several seconds, blocks)\n"
        "- epd.hibernate()\n"
        "- epdPower(LOW)")
    box(draw, MAIN_X, cy("log_complete"), MAIN_W, 70, "print \"Render complete\"")

    box(draw, BAIL_X, bail_cy, BAIL_W, bail_h,
        "Log the specific error via\nSerial.println (see labels\nat left) and return from\nhandleFrame()",
        fill=BAIL_FILL)

    # --- main path connectors, top to bottom ---
    vline(draw, MAIN_X, cy("start") + 35, cy("setup") - 50)
    vline(draw, MAIN_X, cy("setup") + 50, cy("d_avail") - 50)
    vline(draw, MAIN_X, cy("d_avail") + 50, cy("d_magic") - 55, "yes, read it")
    vline(draw, MAIN_X, cy("d_magic") + 55, cy("d_len_to") - 55, "yes (magic matched)")
    vline(draw, MAIN_X, cy("d_len_to") + 55, cy("d_len_val") - 55, "yes")
    vline(draw, MAIN_X, cy("d_len_val") + 55, cy("d_ts_to") - 55, "yes")
    vline(draw, MAIN_X, cy("d_ts_to") + 55, cy("d_payload_to") - 60, "yes")
    vline(draw, MAIN_X, cy("d_payload_to") + 60, cy("d_checksum_to") - 55, "yes")
    vline(draw, MAIN_X, cy("d_checksum_to") + 55, cy("d_checksum_match") - 60, "yes")
    vline(draw, MAIN_X, cy("d_checksum_match") + 60, cy("log_received") - 35, "yes, match")
    vline(draw, MAIN_X, cy("log_received") + 35, cy("render") - 150)
    vline(draw, MAIN_X, cy("render") + 150, cy("log_complete") - 35)

    # --- d_avail: no byte waiting yet -> keep polling itself ---
    self_loop_left(draw, MAIN_X, cy("d_avail"), MAIN_W, 100, "no: keep polling", radius=60)

    # --- d_magic: byte wasn't the sync byte -> back to d_avail, own left-side loop ---
    x_discard = MAIN_X - MAIN_W / 2 - 150
    draw.line([(MAIN_X - MAIN_W / 2, cy("d_magic")), (x_discard, cy("d_magic"))], fill=LINE, width=2)
    draw.line([(x_discard, cy("d_magic")), (x_discard, cy("d_avail"))], fill=LINE, width=2)
    hline(draw, x_discard, MAIN_X - MAIN_W / 2, cy("d_avail"))
    # Label sits to the *right* of this trunk line (between it and the main
    # column), not to the left -- the trunk is already close to the canvas's
    # left edge, and a right-anchored label there would run off the canvas.
    draw.text((x_discard + 10, (cy("d_magic") + cy("d_avail")) / 2), "no: discard byte, keep scanning",
              font=FONT, fill=TEXT, anchor="lm")

    # --- each validation diamond's "no" edge: right to a shared trunk, then
    #     down/up that trunk to its own dedicated entry point on the bail
    #     box's left edge (spread out so no two lines share an endpoint) ---
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
    hline(draw, RETURN_BAIL_X, MAIN_X + MAIN_W / 2, to_y, label="wait for next frame")

    # --- successful render back up to the top of the loop (separate far-right lane) ---
    ok_to_y = cy("d_avail") + 20
    draw.line([(MAIN_X, cy("log_complete") + 35), (MAIN_X, cy("log_complete") + 60)], fill=LINE, width=2)
    draw.line([(MAIN_X, cy("log_complete") + 60), (RETURN_OK_X, cy("log_complete") + 60)], fill=LINE, width=2)
    vline(draw, RETURN_OK_X, cy("log_complete") + 60, ok_to_y)
    hline(draw, RETURN_OK_X, MAIN_X + MAIN_W / 2, ok_to_y, label="wait for next frame")

    img.save(OUT_PATH)
    print(f"--- saved {OUT_PATH} ({canvas_w}x{canvas_h}) ---")


if __name__ == "__main__":
    main()
