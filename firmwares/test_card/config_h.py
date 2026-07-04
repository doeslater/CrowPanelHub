"""
Reads firmwares/test_card/config.h so the Python scripts in this folder never
hardcode a value that's already defined there -- config.h is the single
source of truth for these constants, this module just parses it.

Only handles this one header's actual shape: `const <type> NAME = <expr>;`
lines, where <expr> is a numeric/hex literal or a plain arithmetic
expression referencing earlier constants in the same file (e.g.
PAYLOAD_SIZE's `(DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8`). This is not a
general C header parser -- just enough to cover config.h as it's actually
written.
"""

import os
import re

CONFIG_PATH = os.path.join(os.path.dirname(__file__), "config.h")

# Matches e.g. "const size_t PAYLOAD_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8;"
# Type is ".+?" (non-greedy) rather than "\w+" so multi-word types like
# "unsigned long" match too -- backtracking expands it just enough to leave
# a valid NAME = ... after it.
_CONST_RE = re.compile(r"const\s+(.+?)\s+(\w+)\s*=\s*([^;]+);")


def _parse(path):
    with open(path) as f:
        text = f.read()
    text = re.sub(r"//.*", "", text)  # strip line comments first -- this file's own
    # header comment about config_h.py literally contains the string
    # "const <type> NAME = <expr>;" as an example, which would otherwise match too.

    values = {}
    for match in _CONST_RE.finditer(text):
        c_type, name, expr = match.group(1), match.group(2), match.group(3).strip()
        # C truncates integer division; Python's "/" doesn't. None of this
        # header's constants are float/double, so "//" is always correct here.
        py_expr = expr if c_type in ("float", "double") else expr.replace("/", "//")
        values[name] = eval(py_expr, {"__builtins__": {}}, values)  # noqa: S307 (trusted local file)
    return values


_VALUES = _parse(CONFIG_PATH)

DISPLAY_WIDTH = _VALUES["DISPLAY_WIDTH"]
DISPLAY_HEIGHT = _VALUES["DISPLAY_HEIGHT"]
FRAME_MAGIC = _VALUES["FRAME_MAGIC"]
PAYLOAD_SIZE = _VALUES["PAYLOAD_SIZE"]
SERIAL_BAUD_RATE = _VALUES["SERIAL_BAUD_RATE"]
SERIAL_TIMEOUT_MS = _VALUES["SERIAL_TIMEOUT_MS"]
TIMESTAMP_SIZE = _VALUES["TIMESTAMP_SIZE"]
