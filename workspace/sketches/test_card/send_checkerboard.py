"""
Sends a fixed 8x8-pixel checkerboard test pattern over serial -- the
simplest possible payload for a quick "is the wire protocol actually
working" smoke test, independent of any of the fancier generated patterns
(send_text.py's rendered text, test_card/generate_test_pattern.py's PM5544
card).

This file is a byte-identical copy in sketches/test_card/ too (same
convention as config.h/install.sh) -- so it works as a quick smoke test
against test_card.ino as well, without reaching into this folder. If you
change it, change both copies; `diff` should always come back clean
between them.

Usage:
    python3 send_checkerboard.py
"""

from serial_sender import DISPLAY_HEIGHT, DISPLAY_WIDTH, PAYLOAD_SIZE, send_payload


def checkerboard_payload():
    # 8x8 pixel checkerboard, packed 1bpp MSB-first, row-major, 400x300.
    row_bytes = DISPLAY_WIDTH // 8  # 50
    buf = bytearray(PAYLOAD_SIZE)
    for y in range(DISPLAY_HEIGHT):
        for byte_x in range(row_bytes):
            b = 0
            for bit in range(8):
                x = byte_x * 8 + bit
                on = ((x // 8) + (y // 8)) % 2 == 0
                if on:
                    b |= 1 << (7 - bit)
            buf[y * row_bytes + byte_x] = b
    return bytes(buf)


def main():
    send_payload(checkerboard_payload())


if __name__ == "__main__":
    main()
