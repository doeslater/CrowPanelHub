"""
Low-level helpers for talking to the board over USB serial: resetting it
(DTR/RTS pulse), building a wire-protocol frame around a 15,000-byte
payload, and sending one while printing whatever the board says back.
Stdlib-only (no pyserial) -- this was written during firmware bring-up,
before a proper serial library was worth adding.

Every script in this repo that sends a payload (send_checkerboard.py,
send_text.py, test_card/generate_test_pattern.py) imports send_payload()
from here rather than reimplementing the reset/frame/send logic.

This file is a byte-identical copy in sketches/test_card/ too (same
convention as config.h/install.sh) -- so scripts there don't need to reach
into this folder. If you change it, change both copies; `diff` should
always come back clean between them.
"""

import fcntl
import os
import struct
import termios
import time
import sys

PORT = "/dev/ttyUSB0"
BAUD = termios.B115200

TIOCMGET = 0x5415
TIOCMBIS = 0x5416
TIOCMBIC = 0x5417
TIOCM_DTR = 0x002
TIOCM_RTS = 0x004

DISPLAY_WIDTH = 400
DISPLAY_HEIGHT = 300
PAYLOAD_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) // 8  # 15000
FRAME_MAGIC = 0xA5


def open_port(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
    attrs = termios.tcgetattr(fd)
    # cfmakeraw equivalent
    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = attrs
    iflag = 0
    oflag = 0
    cflag &= ~(termios.CSIZE | termios.PARENB)
    cflag |= termios.CS8 | termios.CREAD | termios.CLOCAL
    lflag = 0
    cc[termios.VMIN] = 0
    cc[termios.VTIME] = 0
    termios.tcsetattr(
        fd, termios.TCSANOW, [iflag, oflag, cflag, lflag, BAUD, BAUD, cc]
    )
    return fd


def set_line(fd, dtr=None, rts=None):
    if dtr is True:
        fcntl.ioctl(fd, TIOCMBIS, struct.pack("I", TIOCM_DTR))
    elif dtr is False:
        fcntl.ioctl(fd, TIOCMBIC, struct.pack("I", TIOCM_DTR))
    if rts is True:
        fcntl.ioctl(fd, TIOCMBIS, struct.pack("I", TIOCM_RTS))
    elif rts is False:
        fcntl.ioctl(fd, TIOCMBIC, struct.pack("I", TIOCM_RTS))


def read_for(fd, seconds):
    end = time.time() + seconds
    data = b""
    while time.time() < end:
        try:
            chunk = os.read(fd, 4096)
            if chunk:
                data += chunk
        except BlockingIOError:
            pass
        time.sleep(0.05)
    return data


def build_frame(payload):
    import datetime

    assert len(payload) == PAYLOAD_SIZE
    now_utc = datetime.datetime.now(datetime.timezone.utc)
    epoch_seconds = int(now_utc.timestamp())
    print(f"--- embedding timestamp: {now_utc.isoformat()} (epoch={epoch_seconds}) ---")

    checksum = sum(payload) & 0xFF
    length_bytes = struct.pack("<I", len(payload))
    timestamp_bytes = struct.pack("<I", epoch_seconds & 0xFFFFFFFF)
    return (
        bytes([FRAME_MAGIC])
        + length_bytes
        + timestamp_bytes
        + payload
        + bytes([checksum])
    )


def send_payload(payload):
    """Resets the board, sends `payload` as a frame, and prints everything
    the board says over Serial. Shared by every script that builds a
    different 15,000-byte bitmap (checkerboard, text, the test card, ...)
    but talks the same wire protocol -- works against any sketch in this
    repo that implements it (receive_image.ino, test_card.ino)."""
    fd = open_port(PORT)
    try:
        print("--- resetting board (DTR/RTS pulse) ---")
        set_line(fd, dtr=False, rts=True)
        time.sleep(0.1)
        set_line(fd, dtr=False, rts=False)

        boot_output = read_for(fd, 3)
        print("--- boot output ---")
        sys.stdout.buffer.write(boot_output)
        print()

        frame = build_frame(payload)
        print(f"--- sending frame: {len(frame)} bytes total ---")
        os.write(fd, frame)

        response = read_for(fd, 6)
        print("--- response after frame ---")
        sys.stdout.buffer.write(response)
        print()
    finally:
        os.close(fd)
