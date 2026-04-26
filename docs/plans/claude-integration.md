# Claude Integration — ambient productivity buddy

Hobby-project plan. Current state on `plus2` branch: capybara renders,
BtnA cycles species, no Claude integration yet.

## Goal

Ambient productivity coach on the desk. Reflects how much I've delegated
to Claude today without nagging or gamifying it. Explicitly anti-streak,
anti-FOMO.

## Final screen layout

```
┌─────────────────┐
│                 │
│    [ buddy ]    │  ← reflects today's delegation
│                 │       (BtnA still cycles species)
│                 │
│   1h 23m        │
│   today         │
│                 │
│  ─────────      │
│   47K tokens    │  ← today
│   3.2M lifetime │
│                 │
└─────────────────┘
```

## Behaviors

- **Buddy mood = today's delegation time.** <15min: just sleeping. 15min–1hr:
  content idle. 1hr+: energetic idle. Never sad — only "hasn't been used yet
  today." Idle animation amplitude scales.
- **BtnA cycles species** (current behavior, keep). Species sound flashes for
  ~3s in the stats area on press, then fades back to delegation/tokens.
- **End-of-day wind-down.** After 9pm (configurable): buddy yawns + screen
  dims to 60. After 10pm: buddy sleeps + screen dims to 30. Anti-addiction
  baked in — device tells me to stop.
- **Offline = current behavior.** When BLE not paired: buddy keeps cycling
  species + showing sound, no stats. Being unplugged doesn't make buddy sad.
- **No celebrations in v1.** Future: maybe all-time token milestones (1M,
  5M, 10M) — fires every few weeks/months, can't be gamed.

## Architecture

### Revive from upstream (parked behind `build_src_filter`)

- `src/ble_bridge.cpp/h` — Nordic UART, passkey pairing. Doesn't touch AXP,
  should drop in. Maybe minor newer-IDF tweaks.
- `src/data.h` — port `RTC_TimeTypeDef` → `m5::rtc_datetime_t`. JSON parsing
  untouched. Provides `TamaState` with sessions/tokens/prompt fields.

### New module

- `src/daystats.h` — header-only, NVS-backed. Tracks today's delegation
  seconds + tokens today + all-time tokens. Auto-resets when RTC date
  changes. Loaded at boot so reboots don't wipe progress.

### Refactor

- `src/main.cpp` grows from ~50 lines to ~150. Adds: BLE init, BLE poll →
  daystats update, derive buddy state from Claude session state, render
  delegation time + tokens text below buddy, wind-down hour check.

## Implementation phases (one commit each)

| Phase | Adds | Verification |
|---|---|---|
| A | BLE bridge revived | Pair with desktop, stays connected |
| B | Buddy state from Claude (busy/idle) | Trigger Claude work → buddy goes busy |
| C | `daystats` + delegation time + tokens today + all-time | Numbers update live, persist across reboot |
| D | Buddy mood reflects daystats | Idle amplitude scales with today's delegation |
| E | End-of-day wind-down | RTC says 9pm → buddy yawns |

Each phase is small enough to test in isolation. Adjust between phases.

## Out of scope (deliberately)

- Streaks, daily goals, "behind yesterday" comparisons
- Activity dot trail (FOMO — empty slots feel like judgment)
- Persistent species sound text (only flashes on BtnA press)
- Celebration on commit/plan-exit/permission (no celebrations in v1)
- Full upstream menu/settings UI
- Stats panel pages (mood/fed/energy meters), level-up confetti
- Charging clock face
- Daughter-mode features
