# Solution — RGB LED color cycling

Retroactive reference documentation for an exercise already marked
**passed** in `PROGRESS.md`. (Note: that log entry's own one-line
description says "toggles GPIO 7 HIGH/LOW on a 1s timer" — that's a
copy-paste leftover from the `pwr_pin` entry above it; the actual code, and
what's documented here, is RGB color cycling via `rgbLedWrite()`.)

## What you wrote

### High-level

The ESP32-S3 dev board's addressable RGB LED is driven through the
Arduino-ESP32 core's `rgbLedWrite(pin, r, g, b)` helper rather than raw
PWM — it cycles the LED through red, green, then blue, one second each,
logging each color change over Serial.

### Line-by-line

```cpp
// Define the pin for the RGB LED.
// Using a const int ensures the pin number cannot be accidentally modified.
const int RGB_LED_PIN = 48;

// Note: RGB_BRIGHTNESS is assumed to be a macro defined elsewhere (e.g., in the Arduino-ESP32 core).
// If not defined, this code will fail to compile. Consider defining it explicitly if needed.
#define RGB_BRIGHTNESS 50 // 255 is the Max brightness for 8-bit PWM.

void setup() {
  // Initialize serial communication at 115200 baud for debugging.
  Serial.begin(115200);
}

void loop() {
  // Set the RGB LED to red (full brightness, green and blue off).
  rgbLedWrite(RGB_LED_PIN, RGB_BRIGHTNESS, 0, 0);
  Serial.println("LED set to Red.");
  delay(1000);  // Wait for 1 second.

  // Set the RGB LED to green (full brightness, red and blue off).
  rgbLedWrite(RGB_LED_PIN, 0, RGB_BRIGHTNESS, 0);
  Serial.println("LED set to green.");
  delay(1000);  // Wait for 1 second.

  // Set the RGB LED to blue (full brightness, red and green off).
  rgbLedWrite(RGB_LED_PIN, 0, 0, RGB_BRIGHTNESS);
  Serial.println("LED set to Blue.");
  delay(1000);  // Wait for 1 second.
}
```

- **Line 3**: `RGB_LED_PIN = 48` — the ESP32-S3 dev board's built-in
  addressable RGB LED data pin (distinct from the CrowPanel's `BUSY` pin,
  which happens to also be GPIO 48 on that board — no conflict here since
  this sketch runs on the bare dev board, not the CrowPanel).
- **Line 7**: `RGB_BRIGHTNESS` set to 50 out of a possible 255 — capping
  brightness rather than defining a separate on/off state; the comment
  correctly flags that this relies on the Arduino-ESP32 core actually
  compiling against a board with a built-in addressable RGB LED.
- **`rgbLedWrite(pin, r, g, b)`**: a single core-provided call that drives
  the addressable LED directly — no manual PWM channel setup, unlike a
  plain RGB LED wired to three separate GPIOs.
- **Each color block**: sets one color at full `RGB_BRIGHTNESS` with the
  other two channels at 0, logs which color was just set, then blocks for
  a second — three near-identical blocks back to back, looping forever
  because `loop()` itself repeats.

## Another way to do it

### High-level

The three color blocks are structurally identical — same three lines,
different numbers and a different string, copy-pasted twice. This is the
same shape you already spotted and fixed yourself in `generative_art.ino`,
where 13 near-identical `drawLine()` calls became one `for` loop over an
array. The same move applies here: describe each color as one entry in an
array of `{name, r, g, b}` records, then loop over the array instead of
writing the block three times.

### Line-by-line

```cpp
const int RGB_LED_PIN = 48;
#define RGB_BRIGHTNESS 50

struct Color {
  const char* name;
  uint8_t r, g, b;
};

Color colors[] = {
  { "Red",   RGB_BRIGHTNESS, 0, 0 },
  { "green", 0, RGB_BRIGHTNESS, 0 },
  { "Blue",  0, 0, RGB_BRIGHTNESS },
};

void setup() {
  Serial.begin(115200);
}

void loop() {
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
    rgbLedWrite(RGB_LED_PIN, colors[i].r, colors[i].g, colors[i].b);
    Serial.print("LED set to ");
    Serial.print(colors[i].name);
    Serial.println(".");
    delay(1000);
  }
}
```

- **`struct Color`**: bundles a label with the three channel values one
  color needs — the same "describe it as data" move as `BadgeLine` in the
  desk-badge alternative solution.
- **`Color colors[] = { ... }`**: the actual red/green/blue sequence,
  expressed as three array entries instead of three code blocks. The
  slightly inconsistent capitalization ("Red", "green", "Blue") is kept
  as-is from your original `Serial.println` strings, just relocated into
  the array.
- **The `for` loop in `loop()`**: does exactly what the three original
  blocks did — write the color, print a label, wait a second — but the
  logic that stays fixed (call `rgbLedWrite`, print, delay) now exists
  once, and adding a fourth color (e.g. white, or a fade step) is a new
  array entry rather than a fourth copy-pasted block.

As with the badge alternative, the visible LED behavior is identical
either way — the value shows up when the color list needs to grow or
change, not in this exact three-color version.
