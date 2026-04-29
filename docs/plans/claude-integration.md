# Claude Integration — ambient agent-time buddy

Hobby-project plan. Tracks how much agent time I'm actually utilizing,
rendered as an ambient companion on the desk. Anti-streak, anti-FOMO —
the goal is "agents running as long as possible during the day," not
"hit a number."

## Goal

Make agent utilization legible. The buddy reflects how much of today my
Claude sessions have actually been working (running or waiting on me),
plus a lifetime total so I can see "how much agent time I've ever had."

## Final screen layout

```
┌─────────────────┐
│                 │
│    [ buddy ]    │  ← reflects today's agent active time
│                 │       (BtnA still cycles species)
│                 │
│                 │
│    Ryan         │  ← buddy's display name
│  ─────────      │
│   2h 34m today  │  ← agent active seconds today
│   47h total     │  ← cumulative since first paired
│                 │
└─────────────────┘
```

## Behaviors

- **Buddy mood = today's agent active time.** Three tiers when no session
  is currently doing anything:
    - 0s → SLEEP ("haven't started yet today")
    - 0 < today < ENERGETIC_THRESHOLD_S → IDLE (ordinary)
    - today ≥ ENERGETIC_THRESHOLD_S → HEART ("good day, buddy is pleased")
  Threshold starts at 2h. Tune in `main.cpp` if it fires too often or
  never. Never sad — only "hasn't worked yet today." Wind-down hours
  override HEART back to SLEEP (bedtime > reward).
- **What counts as "active":** `running > waiting` — at least one alive
  session that isn't blocked on a permission prompt. The desktop counts
  blocked sessions as both `running` AND `waiting`, so the actual
  "currently generating" count is `running - waiting`. A session stopped
  on a prompt doesn't tick the clock: that's the agent idle, waiting on
  me. Open-but-idle sessions don't count either. Active time accumulates
  between snapshots (~10s cadence) and is capped per-snapshot at 60s so
  a BLE drop doesn't credit a phantom hour.
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

- `src/daystats.h` — header-only, NVS-backed. Tracks today's agent active
  seconds + lifetime total. Daily reset at local midnight via the M5 RTC
  (set by the desktop's `time` message on connect). Loaded at boot so
  reboots don't wipe progress.

### Refactor

- `src/main.cpp` grows from ~50 lines to ~150. Adds: BLE init, BLE poll →
  daystats update, derive buddy state from Claude session state, render
  agent active time text below buddy, wind-down hour check.

## Implementation phases (one commit each)

| Phase | Adds | Verification |
|---|---|---|
| A | BLE bridge revived | Pair with desktop, stays connected |
| B | Buddy state from Claude (busy/idle) | Trigger Claude work → buddy goes busy |
| C | `daystats` + agent today + total | Numbers update live, persist across reboot |
| D | Buddy mood reflects daystats | Buddy sleeps if no agent time today |
| E | End-of-day wind-down | RTC says 9pm → buddy yawns |
| F | Drive metric off agent active time | Replaces tokens-based phase D |
| G | Energetic mood tier (HEART persona at ≥2h today) | Cross threshold → buddy gets floating hearts |

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
