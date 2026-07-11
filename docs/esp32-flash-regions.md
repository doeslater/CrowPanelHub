# ESP32-S3 flash regions, explained

What each region of the ESP32-S3's flash chip actually stores, and when it gets
rewritten — explained via a desktop-computer analogy. Written while designing the
in-app firmware-reflash idea (see `CLAUDE.md`'s "Open threads" — "Reset and
restore" as an actual in-app reflash), but useful background any time firmware
flashing on this board comes up.

## The regions

| Flash region | What it stores | When you'd rewrite it | Desktop equivalent |
|---|---|---|---|
| ROM bootloader (not flash — burned in at the factory) | Decides "enter recovery/download mode" vs. hand off to the next stage | Never — it can't be written, ever, under any circumstance | BIOS/UEFI firmware — always lets you get into a recovery mode no matter how bad the disk is |
| Second-stage bootloader (`0x0`) | Reads the partition table, jumps to the right app partition | Only if flash mode/speed/security settings change | Boot manager (GRUB / Windows Boot Manager) |
| Partition table (`0x8000`) | The "map" of how the rest of the flash is divided up — app size, OTA slots, storage regions | Only if the `PartitionScheme` FQBN option changes | The disk's MBR/GPT partition table |
| `otadata`/`boot_app0` (`0xe000`) | One flag: which app slot to boot from — only meaningful if there are two app slots, for OTA rollback | Only relevant for Wi-Fi OTA updates | The "which OS copy to boot" flag in a dual-boot/rollback setup |
| App partition (`0x10000`) | The actual compiled sketch — the `setup()`/`loop()` code | Every time a different sketch is flashed — this is the one region that meaningfully differs sketch-to-sketch | The OS files themselves (e.g. `C:\Windows`) |
| NVS | Persistent key-value settings (Wi-Fi credentials, saved preferences) — survives reboot, unlike RAM | Written at runtime by the sketch itself (e.g. `Preferences.putString(...)`), not by the flashing tool | A small settings file on the disk |

## Why this matters for this project

Every sketch in `workspace/sketches/` and `workspace/exercises/` is compiled with
the same FQBN (`docs/fqbn.txt`), so the bootloader, partition table, and
`otadata` region are identical across all of them — only the **app partition**
actually differs between sketches. A normal `arduino-cli upload` rewrites the
whole bundle (bootloader + partition table + app) every time regardless, even
though most of that bundle's content never changes here.

This project has no Wi-Fi/OTA update feature (see `CLAUDE.md`'s Transport & wire
protocol section), so `otadata`/`boot_app0` exists in the flash layout but is
never actually used. Likewise, nothing here saves persistent settings on the
board, so the NVS region is unused too.

## Full-chip vs. app-partition-only reflash

Using the same analogy: a **full-chip reflash** (bootloader + partition table +
app, all rewritten) is like formatting a drive and reinstalling the OS from
scratch — it rewrites the disk's own "map," which is riskier if interrupted
mid-write. An **app-partition-only reflash** is like reinstalling just the OS
onto an already-correctly-partitioned drive — the partition table and boot
manager are left completely alone, and only the OS files themselves (the app
code) get replaced. Since only the app partition ever actually differs between
sketches in this project, app-partition-only is sufficient to fix "the wrong
sketch is flashed" without ever touching the riskier regions.

## Flashing firmware vs. sending an image over the wire protocol

Worth not conflating these — they're unrelated operations with very different
risk profiles:

- **Sending an image** (the existing, working milestone-1 wire protocol):
  `test_card.ino`'s `payload_buffer` is a plain array in **RAM**. Incoming bytes
  are copied into it, drawn to the e-paper display, and that's it — nothing is
  ever written to flash. The image "persists" only because e-paper physically
  holds its pixels with no power, not because anything was saved on the chip. A
  botched image send is harmless — just resend it.
- **Flashing firmware**: the payload (a compiled `.bin`) must be written into
  the chip's **flash memory** so it survives reboots and is what actually runs.
  An interrupted flash write can leave the board unable to boot into anything
  until it's re-flashed — which is why it's a fundamentally riskier operation
  than sending an image ever was.
