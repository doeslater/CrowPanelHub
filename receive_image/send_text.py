"""
Renders text into a 400x300 monochrome bitmap with PIL and sends it to the
already-flashed receive_image.ino firmware over serial. No firmware change is
needed for this -- receive_image.ino only ever draws whatever bitmap it's
given, so this is the text-display equivalent of the checkerboard pattern in
send_test_frame.py: same wire protocol, same board, different payload.

Usage:
    python3 send_text.py "Hello World" "Second line" "Third line"
    python3 send_text.py                                # uses default demo lines

Requires Pillow (`pip install pillow`) -- unlike send_test_frame.py, this one
isn't stdlib-only, since rendering readable text needs real font metrics.
"""

import sys

from PIL import Image, ImageDraw, ImageFont

from send_test_frame import DISPLAY_HEIGHT, DISPLAY_WIDTH, PAYLOAD_SIZE, send_payload

DEFAULT_LINES = ["Hello World", "from Python", "over USB serial"]


def text_to_payload(lines, font_size=28, margin=10, line_gap=8):
    image = Image.new("1", (DISPLAY_WIDTH, DISPLAY_HEIGHT), color=1)  # 1 = white background
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default(size=font_size)

    y = margin
    for line in lines:
        draw.text((margin, y), line, fill=0, font=font)  # 0 = black text
        _, top, _, bottom = draw.textbbox((margin, y), line, font=font)
        y = bottom + line_gap

    packed = image.tobytes()  # PIL convention: bit=1 -> white, bit=0 -> black
    payload = bytes(b ^ 0xFF for b in packed)  # firmware convention: bit=1 -> black, bit=0 -> white
    assert len(payload) == PAYLOAD_SIZE, f"expected {PAYLOAD_SIZE} bytes, got {len(payload)}"
    return payload


def main():
    lines = sys.argv[1:] or DEFAULT_LINES
    send_payload(text_to_payload(lines))


if __name__ == "__main__":
    main()
