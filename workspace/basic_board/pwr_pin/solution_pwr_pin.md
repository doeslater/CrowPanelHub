# Solution — PWR pin warm-up

Retroactive reference documentation for an exercise already marked
**passed** in `PROGRESS.md` ("no hint needed"). See `CLAUDE.md`'s Teaching
mode section for why this file exists.

## What you wrote

### High-level

The CrowPanel-native version of "blink an LED": since this board has no
onboard LED, the exercise instead toggles `PWR` (GPIO 7, the MOSFET switch
that energizes the panel's power rail) HIGH and LOW on a one-second cadence,
logging each transition over Serial so it can be confirmed via
`arduino-cli monitor` without needing to look at the panel at all. This
passed on the first attempt, no hint needed.

### Line-by-line

```cpp
const int PWR_PIN = 7;  // switches panel power via a MOSFET; HIGH = on, LOW = off

void setup() {
  Serial.begin(115200);
  pinMode(PWR_PIN, OUTPUT);
}

void loop() {
  digitalWrite(PWR_PIN, HIGH);
  Serial.println("High");
  delay(1000);

  digitalWrite(PWR_PIN, LOW);
  Serial.println("Low");
  delay(1000);
}
```

- **Line 1**: `PWR_PIN` as a named constant rather than a bare `7`
  scattered through the file — the comment records *why* this pin matters
  (it's not a general-purpose GPIO, it's wired to a MOSFET gate).
- **Line 4**: `Serial.begin(115200)` — the standard baud rate used
  throughout this project (matches the wire protocol's baud rate too).
- **Line 5**: `pinMode(PWR_PIN, OUTPUT)` — must run once before the pin is
  ever written to.
- **Lines 8–10**: drive `PWR_PIN` HIGH, log it, then block for one second.
- **Lines 12–14**: drive it LOW, log it, block another second. `loop()`
  repeating this forever produces the 1Hz toggle.

## Another way to do it

### High-level

`delay(1000)` blocks the entire CPU for that full second — nothing else in
`loop()` can run during it. That's invisible here because there's nothing
else to do yet, but it's a real limitation the moment a sketch needs to
also poll a sensor, read Serial input, or otherwise stay responsive while
timing something. The standard fix (often called "blink without delay" in
Arduino material) is to track elapsed time with `millis()` instead of
blocking with `delay()` — `loop()` keeps spinning continuously, and the
pin only toggles when enough time has actually passed.

This one *is* a meaningfully different approach, not a cosmetic rename —
it changes what `loop()` is free to do in between toggles.

### Line-by-line

```cpp
const int PWR_PIN = 7;
const unsigned long INTERVAL_MS = 1000;

unsigned long lastToggle = 0;
bool pwrState = false;

void setup() {
  Serial.begin(115200);
  pinMode(PWR_PIN, OUTPUT);
}

void loop() {
  unsigned long now = millis();
  if (now - lastToggle >= INTERVAL_MS) {
    pwrState = !pwrState;
    digitalWrite(PWR_PIN, pwrState ? HIGH : LOW);
    Serial.println(pwrState ? "High" : "Low");
    lastToggle = now;
  }
}
```

- **`INTERVAL_MS`**: the toggle period, now a named constant instead of a
  magic `1000` baked into two `delay()` calls.
- **`lastToggle`**: remembers the `millis()` timestamp of the last
  transition, persisted across loop iterations (hence declared outside
  `loop()`, at file scope).
- **`pwrState`**: tracks which state the pin is currently in, since
  there's no longer a linear HIGH-block-LOW-block sequence to fall through
  — the loop has to know what to flip *to*.
- **`unsigned long now = millis()`**: `millis()` returns milliseconds
  since boot; comparing it against `lastToggle` (rather than sleeping) is
  what makes this non-blocking.
- **`if (now - lastToggle >= INTERVAL_MS)`**: the subtraction (not a
  direct `>=` comparison against a fixed target time) is deliberate — it's
  the idiom that survives `millis()` wrapping back to zero after about 49
  days of uptime, since unsigned subtraction still produces the correct
  elapsed duration even across the wraparound.
- **Inside the `if`**: flip `pwrState`, write it, log it, and record
  `now` as the new `lastToggle` — all in constant time, no blocking.

For this specific exercise the visible behavior is identical either way —
the value here is entirely in what `loop()` is *free to do* in the gaps,
which matters more once later exercises need to do more than one thing at
a time.
