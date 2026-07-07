# Task — Desk badge / name tag

See `lifecycle.md` for the theory (library/display object, and the
power/init/draw/display/hibernate/power-off sequence) before attempting
this.

## Checkpoint

Write a new sketch, from scratch (`desk_badge.ino`, in this folder), that:

1. Powers up the panel (`PWR` HIGH).
2. Initializes the display (`epd.init(...)`, full refresh, not partial).
3. Draws a name/badge message using the text API (`setRotation()`,
   `fillScreen()`, `setFont()`, `setTextColor()`, `setCursor(x, y)`,
   `print()`/`println()`).
4. Pushes it to the glass (`epd.display()`).
5. Hibernates the controller (`epd.hibernate()`) and cuts power
   (`PWR` LOW).

Result: a badge that stays up on the panel permanently, with the board's
panel-power rail fully unpowered afterward.

If you get stuck on *how the library API actually behaves* (not the
concept, just syntax), `workspace/sketches/display_text/display_text.ino`
is fair to peek at — that's what it's there for. Ask for one hint only
after you've made a real attempt.
