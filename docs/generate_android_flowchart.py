"""
Renders android-flowchart.png: a diagram of the Android app's two real
decision flows -- Connect (MainViewModel.OnConnectClick -> UsbSerialTransport
.connect()) and Send (OnSendTextClick/OnSendCheckerboardClick -> .sendImage())
-- merged into one top-to-bottom diagram, since Send only ever runs after
Connect has succeeded. Every node's wording matches the actual code/state
names in MainViewModel.kt/UsbSerialTransport.kt, so android-app.md and the
diagram can be read side by side with the source.

Same approach as firmwares/receive_image/generate_flowchart.py: no
diagramming library (graphviz/mermaid) is installed on this machine and
installing one needs a system package (sudo), so this hand-draws boxes,
diamonds, and connector arrows directly with Pillow. Text is kept
plain-ASCII on purpose: Pillow's bundled default font has no glyphs for
em-dashes/bullets/arrows and silently renders them as "missing glyph" boxes.
This script is a self-contained copy of that one's drawing primitives
(box/oval/diamond/arrows) rather than an import, matching this project's
convention of self-contained per-folder scripts (see config.h in the
firmware folders).

Usage:
    python3 generate_android_flowchart.py

Requires Pillow (`pip install pillow`).
"""

import os

from PIL import Image, ImageDraw, ImageFont

# Resolved against this script's own location, not the current working
# directory, so "run from the repo root" (see android-app.md) saves next to
# this script rather than into the repo root.
OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "android-flowchart.png")

MAIN_X = 560  # center x of the main (happy-path) column
MAIN_W = 560  # width of main-path boxes/diamonds -- slightly wider than
              # receive_image's chart since these labels mention Kotlin
              # class/method names, which run longer than the firmware's
GAP = 110  # vertical gap between stacked main-path nodes

BAIL_X = 1550  # center x of the shared "bail" boxes
BAIL_W = 460
BAIL_LANE_X = 950  # vertical trunk each "no" branch travels down/right on
                    # its way to a bail box

RETURN_X = 1800  # far-right lane every "try again" loop-back travels up

FONT_SIZE = 17
FONT = ImageFont.load_default(size=FONT_SIZE)

BG = (255, 255, 255)
BOX_FILL = (235, 244, 255)
DIAMOND_FILL = (255, 244, 214)
BAIL_FILL = (255, 227, 227)
START_FILL = (223, 245, 223)
OK_FILL = (223, 245, 223)
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


def main():
    y = 90
    nodes = {}

    def place(key, h):
        nonlocal y
        nodes[key] = y
        y += h + GAP
        return key

    place("start", 70)
    place("tap_connect", 90)
    place("d_driver", 110)
    place("d_perm", 130)
    place("open_port", 110)
    place("d_open_ok", 100)
    place("connected", 70)
    place("tap_send", 90)
    place("build_payload", 140)
    place("write_serial", 130)
    place("d_write_ok", 100)
    place("sent_ok", 90)

    def cy(key):
        return nodes[key]

    canvas_h = y + 60
    canvas_w = RETURN_X + 150
    img = Image.new("RGB", (canvas_w, canvas_h), BG)
    draw = ImageDraw.Draw(img)

    connect_bail_sources = [
        ("d_driver", "no: no USB serial\ndevice found"),
        ("d_perm", "no: permission denied"),
        ("d_open_ok", "no: could not open\ndevice/port"),
    ]
    connect_bail_cy = sum(cy(k) for k, _ in connect_bail_sources) / len(connect_bail_sources)
    connect_bail_h = 190

    # --- main-path nodes ---
    oval(draw, MAIN_X, cy("start"), 280, 70, "App launched\nMainScreen shown")
    box(draw, MAIN_X, cy("tap_connect"), MAIN_W, 90,
        "User taps Connect\n(transport picker: USB, only option)")
    diamond(draw, MAIN_X, cy("d_driver"), MAIN_W, 110,
            "UsbSerialTransport.connect():\nUsbSerialProber finds a\nUSB serial driver?")
    diamond(draw, MAIN_X, cy("d_perm"), MAIN_W, 130,
            "Has USB permission?\n(state = RequestingPermission;\nrequests async dialog via\nBroadcastReceiver if not)")
    box(draw, MAIN_X, cy("open_port"), MAIN_W, 110,
        "state = Connecting\nopenDevice() + open port\nset 115200 baud, 8N1")
    diamond(draw, MAIN_X, cy("d_open_ok"), MAIN_W, 100, "Opened successfully?")
    box(draw, MAIN_X, cy("connected"), MAIN_W, 70, "state = Connected", fill=OK_FILL)
    box(draw, MAIN_X, cy("tap_send"), MAIN_W, 90,
        "User taps Send Text\nor Send Checkerboard")
    box(draw, MAIN_X, cy("build_payload"), MAIN_W, 140,
        "Build payload:\nTextBitmap.generate() or\nCheckerboard.generate()\n"
        "-> WireFrame.build(payload)\n(magic + length + timestamp + checksum)")
    box(draw, MAIN_X, cy("write_serial"), MAIN_W, 130,
        "port.write(frame, timeout)\n"
        "Progress bar ticks 0 -> 1 over an\nestimated ~1.3s write + 4s panel\n"
        "refresh cooldown (fire-and-forget:\nno real completion signal exists)")
    diamond(draw, MAIN_X, cy("d_write_ok"), MAIN_W, 100, "Write succeeded?")
    box(draw, MAIN_X, cy("sent_ok"), MAIN_W, 90,
        "status = \"Sent successfully\"\n(every step logged to Diagnostics)",
        fill=OK_FILL)

    box(draw, BAIL_X, connect_bail_cy, BAIL_W, connect_bail_h,
        "state = ConnectFailed(reason)\nor PermissionDenied\n(see label at left; logged\nto Diagnostics)",
        fill=BAIL_FILL)
    box(draw, BAIL_X, cy("d_write_ok"), BAIL_W, 130,
        "status = \"Send failed: <reason>\"\n(logged to Diagnostics)",
        fill=BAIL_FILL)

    # --- main path connectors, top to bottom ---
    vline(draw, MAIN_X, cy("start") + 35, cy("tap_connect") - 45)
    vline(draw, MAIN_X, cy("tap_connect") + 45, cy("d_driver") - 55)
    vline(draw, MAIN_X, cy("d_driver") + 55, cy("d_perm") - 65, "yes")
    vline(draw, MAIN_X, cy("d_perm") + 65, cy("open_port") - 55, "yes (granted)")
    vline(draw, MAIN_X, cy("open_port") + 55, cy("d_open_ok") - 50, "")
    vline(draw, MAIN_X, cy("d_open_ok") + 50, cy("connected") - 35, "yes")
    vline(draw, MAIN_X, cy("connected") + 35, cy("tap_send") - 45)
    vline(draw, MAIN_X, cy("tap_send") + 45, cy("build_payload") - 70)
    vline(draw, MAIN_X, cy("build_payload") + 70, cy("write_serial") - 65)
    vline(draw, MAIN_X, cy("write_serial") + 65, cy("d_write_ok") - 50)
    vline(draw, MAIN_X, cy("d_write_ok") + 50, cy("sent_ok") - 45, "yes")

    # --- each connect-phase "no" edge: right to a shared trunk, then to its
    #     own dedicated entry point on the connect-bail box's left edge ---
    entry_ys = [
        connect_bail_cy - connect_bail_h / 2 + (i + 1) * connect_bail_h / (len(connect_bail_sources) + 1)
        for i in range(len(connect_bail_sources))
    ]
    for (key, label), entry_y in zip(connect_bail_sources, entry_ys):
        source_y = cy(key)
        draw.line([(MAIN_X + MAIN_W / 2, source_y), (BAIL_LANE_X, source_y)], fill=LINE, width=2)
        draw.text((MAIN_X + MAIN_W / 2 + 12, source_y - 26), label, font=FONT, fill=TEXT, anchor="lm")
        draw.line([(BAIL_LANE_X, source_y), (BAIL_LANE_X, entry_y)], fill=LINE, width=2)
        hline(draw, BAIL_LANE_X, BAIL_X - BAIL_W / 2, entry_y)

    # --- connect-bail box back up to "User taps Connect" (far-right lane) ---
    to_y = cy("tap_connect") - 20
    draw.line([(BAIL_X, connect_bail_cy + connect_bail_h / 2), (BAIL_X, connect_bail_cy + connect_bail_h / 2 + 30)],
              fill=LINE, width=2)
    draw.line([(BAIL_X, connect_bail_cy + connect_bail_h / 2 + 30), (RETURN_X, connect_bail_cy + connect_bail_h / 2 + 30)],
              fill=LINE, width=2)
    vline(draw, RETURN_X, connect_bail_cy + connect_bail_h / 2 + 30, to_y)
    hline(draw, RETURN_X, MAIN_X + MAIN_W / 2, to_y, label="user can tap Connect again")

    # --- d_write_ok "no" -> send-bail box (single source, direct line) ---
    draw.line([(MAIN_X + MAIN_W / 2, cy("d_write_ok")), (BAIL_X - BAIL_W / 2, cy("d_write_ok"))],
              fill=LINE, width=2)
    arrowhead(draw, BAIL_X - BAIL_W / 2, cy("d_write_ok"), "right")
    draw.text((MAIN_X + MAIN_W / 2 + 12, cy("d_write_ok") - 26), "no: port.write()\nthrew or timed out",
               font=FONT, fill=TEXT, anchor="lm")

    # --- send-bail and successful-send both loop back to "User taps Send"
    #     (two lanes on the far right so the lines don't overlap) ---
    send_bail_return_x = RETURN_X - 80
    to_y2 = cy("tap_send") - 20
    draw.line([(BAIL_X, cy("d_write_ok") + 65), (BAIL_X, cy("d_write_ok") + 90)], fill=LINE, width=2)
    draw.line([(BAIL_X, cy("d_write_ok") + 90), (send_bail_return_x, cy("d_write_ok") + 90)], fill=LINE, width=2)
    vline(draw, send_bail_return_x, cy("d_write_ok") + 90, to_y2)
    hline(draw, send_bail_return_x, MAIN_X + MAIN_W / 2, to_y2, label="user can tap Send again")

    ok_to_y = cy("tap_send") + 20
    draw.line([(MAIN_X, cy("sent_ok") + 45), (MAIN_X, cy("sent_ok") + 70)], fill=LINE, width=2)
    draw.line([(MAIN_X, cy("sent_ok") + 70), (RETURN_X, cy("sent_ok") + 70)], fill=LINE, width=2)
    vline(draw, RETURN_X, cy("sent_ok") + 70, ok_to_y)
    hline(draw, RETURN_X, MAIN_X + MAIN_W / 2, ok_to_y, label="ready to send again")

    img.save(OUT_PATH)
    print(f"--- saved {OUT_PATH} ({canvas_w}x{canvas_h}) ---")


if __name__ == "__main__":
    main()
