# Warm-up — GPIO output via `PWR`

## Theory

Every GPIO pin on the ESP32 can be told, in software, to sit at one of two
voltage states: HIGH (~3.3V) or LOW (0V). You set this up with
`pinMode(pin, OUTPUT)` once, then flip it with `digitalWrite(pin, HIGH)` /
`digitalWrite(pin, LOW)` whenever you want — the exact same mechanism your
earlier blink exercise used.

The difference here: `PWR` (GPIO 7) isn't wired straight to an LED. It's
wired to the gate of a **MOSFET** — a transistor acting as an electronic
switch for a circuit the GPIO pin itself couldn't power directly (the
whole e-paper panel draws more current than a GPIO pin can safely
supply). When `PWR` is HIGH, the MOSFET conducts and lets power flow
through to the panel; when LOW, it cuts the panel's power rail entirely.
So toggling this pin doesn't light anything up — it turns the **entire
panel's power supply** on and off, at 3.3V logic level, indirectly,
through the transistor.

One consequence worth knowing up front: e-paper holds its image with
**zero power** and only redraws when you explicitly call a refresh
function — so toggling `PWR` alone won't produce any visible change on
the panel. That's fine; this warm-up isn't about the panel, it's about
the GPIO mechanic itself.

## Checkpoint

Write a sketch that sets `PWR` as an output in `setup()`, then in
`loop()` toggles it HIGH/LOW on a timer (e.g. every second), printing
which state you just set to `Serial` each time — so you can watch it
happen live in the serial monitor once you flash and open it yourself.
