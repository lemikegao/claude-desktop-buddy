# claude-desktop-buddy — PLUS2 fork

Personal hobby fork of [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy).
Low-stakes, iterate-as-we-go. No need to keep upstream parity, no production
discipline — favor "make it work on my hardware" over "make it general."

## Hardware target: M5StickC PLUS2 (NOT original PLUS)

Upstream targets the original PLUS. This fork targets the **PLUS2**, which is
a different board with no AXP192 power chip. Differences that matter:

- Library: **`m5stack/M5Unified`** (NOT the legacy `m5stack/M5StickCPlus`)
- PlatformIO board: **`m5stick-c-plus2`** (NOT `m5stick-c`)
- Power: `M5.Power.*` API (no `M5.Axp.*` — that chip doesn't exist on PLUS2)
- LED: GPIO **19**, active-low (PLUS uses GPIO10)
- Audio: real I2S speaker via `M5.Speaker.*` (PLUS has a passive buzzer / `M5.Beep`)
- **Power-hold quirk:** PLUS2 needs GPIO4 held HIGH at boot to stay on after
  the power button is released. `M5.begin()` in M5Unified handles this for the
  PLUS2 board target — do not regress this.
- Specs: 8MB flash, 2MB PSRAM, 200mAh battery (all upgrades vs PLUS)

## Why M5Unified (not M5StickCPlus)

M5Unified is M5Stack's modern unified API supporting all sticks/cores. Cleaner,
future-proof, and the legacy `M5StickCPlus` library doesn't recognize the
PLUS2 board variant at all. Don't reintroduce `<M5StickCPlus.h>` includes —
they will compile against the wrong hardware assumptions.

## Working agreement

- Branch: `plus2`. Don't push to `anthropics/claude-desktop-buddy` upstream.
- First milestone: get something visible on the PLUS2 LCD, then get one ASCII
  buddy rendering. BLE / Claude Desktop pairing is deferred until basic
  hardware works.
- It's fine to stub or disable upstream features (AXP-only stats, fancy power
  management) to get to first-pixel faster. Mark stubs with `// PLUS2 stub:`
  so they're easy to grep later.
- User flashes the device and reports back what shows up on screen. Expect
  multiple "try this build → here's what happened" round-trips.
