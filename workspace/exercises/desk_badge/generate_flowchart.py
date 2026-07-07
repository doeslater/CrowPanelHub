"""
Renders flowchart.png: the desk-badge exercise's boot-to-sleep-forever
lifecycle -- power the panel, wake the controller, draw into the frame
buffer, push it to the glass, then put both the controller and the panel's
own power rail to sleep for good. No branches -- unlike test_card.ino's
receive loop, this sketch runs once and never revisits a step -- so this
is a straight top-to-bottom chain rather than test_card/generate_flowchart.py's
decision-diamond layout.

Same hand-drawn-with-Pillow approach as sketches/test_card/generate_flowchart.py
(see that file's docstring for why: no graphviz/mermaid installed). Kept as
its own self-contained script rather than a shared import, matching this
repo's per-folder self-containment convention.

Usage:
    python3 generate_flowchart.py

Requires Pillow (`pip install pillow`).
"""

import os

from PIL import Image, ImageDraw, ImageFont

OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "flowchart.png")

CX = 420          # center x of every node
W = 640           # node width
GAP = 70          # vertical gap between stacked nodes

FONT_SIZE = 17
FONT = ImageFont.load_default(size=FONT_SIZE)

BG = (255, 255, 255)
BOX_FILL = (235, 244, 255)
POWER_FILL = (255, 244, 214)
SLEEP_FILL = (223, 245, 223)
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


def oval(draw, cx, cy_, w, h, text, fill=SLEEP_FILL):
    x0, y0, x1, y1 = cx - w / 2, cy_ - h / 2, cx + w / 2, cy_ + h / 2
    draw.ellipse([x0, y0, x1, y1], fill=fill, outline=LINE, width=2)
    draw_centered_text(draw, cx, cy_, text)


def arrowhead(draw, x, y):
    size = 8
    draw.polygon([(x, y), (x - size, y - size), (x + size, y - size)], fill=LINE)


def vline(draw, x, y1, y2, label=None):
    draw.line([(x, y1), (x, y2)], fill=LINE, width=2)
    arrowhead(draw, x, y2)
    if label:
        draw.text((x + 12, (y1 + y2) / 2), label, font=FONT, fill=TEXT, anchor="lm")


def main():
    y = 80
    nodes = {}

    def place(key, h):
        nonlocal y
        nodes[key] = y
        y += h + GAP
        return key

    place("start", 70)
    place("power_on", 90)
    place("init", 100)
    place("draw", 160)
    place("display", 120)
    place("hibernate", 90)
    place("power_off", 90)
    place("end", 90)

    def cy(key):
        return nodes[key]

    canvas_h = y + 40
    canvas_w = CX + W / 2 + 60
    img = Image.new("RGB", (int(canvas_w), int(canvas_h)), BG)
    draw = ImageDraw.Draw(img)

    oval(draw, CX, cy("start"), 260, 70, "Board boot / reset\nsetup() begins", fill=BOX_FILL)
    box(draw, CX, cy("power_on"), W, 90,
        "PWR (GPIO 7) -> HIGH\nMOSFET conducts, panel's power rail energizes",
        fill=POWER_FILL)
    box(draw, CX, cy("init"), W, 100,
        "epd.init(...)\nwakes the SSD1683 controller, configures it\nfor a full (not partial) refresh")
    box(draw, CX, cy("draw"), W, 160,
        "Draw into the in-memory frame buffer only --\nnothing visible on the panel yet:\n"
        "setRotation(), fillScreen(), setFont(),\nsetTextColor(), setCursor(x, y), print(...)")
    box(draw, CX, cy("display"), W, 120,
        "epd.display()\npushes the frame buffer to the glass --\n"
        "the one call that actually redraws the panel\n(several seconds, blocks)")
    box(draw, CX, cy("hibernate"), W, 90,
        "epd.hibernate()\ncontroller chip itself goes to sleep")
    box(draw, CX, cy("power_off"), W, 90,
        "PWR (GPIO 7) -> LOW\nMOSFET cuts the panel's power rail entirely",
        fill=POWER_FILL)
    oval(draw, CX, cy("end"), 340, 90,
         "loop() idles forever --\nbadge stays on the glass with\nthe panel fully unpowered",
         fill=SLEEP_FILL)

    vline(draw, CX, cy("start") + 35, cy("power_on") - 45)
    vline(draw, CX, cy("power_on") + 45, cy("init") - 50)
    vline(draw, CX, cy("init") + 50, cy("draw") - 80)
    vline(draw, CX, cy("draw") + 80, cy("display") - 60)
    vline(draw, CX, cy("display") + 60, cy("hibernate") - 45)
    vline(draw, CX, cy("hibernate") + 45, cy("power_off") - 45)
    vline(draw, CX, cy("power_off") + 45, cy("end") - 45)

    img.save(OUT_PATH)
    print(f"--- saved {OUT_PATH} ({canvas_w}x{canvas_h}) ---")


if __name__ == "__main__":
    main()
