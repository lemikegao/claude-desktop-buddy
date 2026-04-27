#pragma once
// Daystats: NVS-backed today/lifetime token counters from the desktop.
//
// "Tokens today" = whatever the desktop most recently sent in
//   tokens_today. Transient; not persisted (desktop owns it and resets
//   it at its own local midnight).
// "Lifetime tokens" = device-observed cumulative output tokens. Tracked
//   by deltaing the desktop's `tokens` field across snapshots. Survives
//   reboot. If the desktop restarts (tokens jumps backward) we just
//   re-baseline without decrementing.
//
// Header-only by design (per claude-integration.md). All state is
// file-static; daystatsInit() loads from NVS. Persistence is throttled
// to once per 30 s and triggered from the heartbeat callbacks (no
// per-loop tick required).
//
// Earlier iterations also tracked a delegation-seconds counter and did
// midnight rollover off the on-board RTC. Phase D dropped it: tokens_today
// is already the user-visible "today's activity" signal AND the desktop
// owns its reset, so the device doesn't need its own RTC-driven rollover
// or a parallel seconds counter.

#include <Preferences.h>
#include <Arduino.h>
#include <stdint.h>

// ---- state ----------------------------------------------------------------
static Preferences _dsPrefs;
static uint64_t _dsLifetimeTokens      = 0;
static uint32_t _dsBridgeBaseline      = 0;
static bool     _dsBridgeBaselineSet   = false;
static uint32_t _dsTokensToday         = 0;
static bool     _dsTokensTodaySeen     = false;  // true once a heartbeat arrives
static uint32_t _dsLastPersistMs       = 0;
static bool     _dsDirty               = false;

// ---- helpers --------------------------------------------------------------
static inline void _dsPersistIfDirty(uint32_t now, bool force = false) {
  if (!_dsDirty) return;
  if (!force && (now - _dsLastPersistMs) < 30000) return;
  _dsPrefs.putULong64("lifeT", _dsLifetimeTokens);
  _dsPrefs.putUInt("brgBL",   _dsBridgeBaseline);
  _dsPrefs.putBool("brgBLs",  _dsBridgeBaselineSet);
  _dsLastPersistMs = now;
  _dsDirty = false;
}

// ---- public ---------------------------------------------------------------
inline void daystatsInit() {
  _dsPrefs.begin("daystats", false);
  _dsLifetimeTokens      = _dsPrefs.getULong64("lifeT", 0);
  _dsBridgeBaseline      = _dsPrefs.getUInt("brgBL", 0);
  _dsBridgeBaselineSet   = _dsPrefs.getBool("brgBLs", false);
}

// Called from the heartbeat parser when `tokens` (cumulative since desktop
// start) is present. Track deltas, rebaseline on backward jumps.
inline void daystatsOnBridgeTokens(uint32_t bridgeTokens) {
  if (!_dsBridgeBaselineSet) {
    _dsBridgeBaseline = bridgeTokens;
    _dsBridgeBaselineSet = true;
    _dsDirty = true;
    _dsPersistIfDirty(millis());
    return;
  }
  if (bridgeTokens >= _dsBridgeBaseline) {
    uint32_t delta = bridgeTokens - _dsBridgeBaseline;
    if (delta) {
      _dsLifetimeTokens += delta;
      _dsDirty = true;
    }
  } else {
    Serial.printf("[daystats] bridge restart (%lu -> %lu), rebaseline\n",
                  (unsigned long)_dsBridgeBaseline, (unsigned long)bridgeTokens);
    _dsDirty = true;
  }
  _dsBridgeBaseline = bridgeTokens;
  _dsPersistIfDirty(millis());
}

inline void daystatsOnTokensToday(uint32_t tt) {
  _dsTokensToday = tt;
  _dsTokensTodaySeen = true;
  // Invariant: lifetime is a superset of today, so lifetime >= today must
  // always hold. On a fresh device this floor is what makes day-1 coherent
  // (otherwise we'd show "873K today / 0 lifetime" until the next bridge
  // delta trickled in). Lifetime never decreases, so post-midnight when
  // today resets, this is a no-op.
  if (tt > _dsLifetimeTokens) {
    _dsLifetimeTokens = tt;
    _dsDirty = true;
  }
  _dsPersistIfDirty(millis());
}

inline uint32_t daystatsTokensToday()         { return _dsTokensToday; }
inline bool     daystatsTokensTodaySeen()     { return _dsTokensTodaySeen; }
inline uint64_t daystatsLifetimeTokens()      { return _dsLifetimeTokens; }

// Pretty-formatter used by the renderer.
//
// fmtTokens:   "0" / "847" / "47K" / "3.2M" / "12.4M"
//   - Compact below 10K: 4521 -> "4.5K"
//   - 10K..1M: "47K"
//   - 1M+: "3.2M"
inline void daystatsFmtTokens(char* out, size_t cap, uint64_t n) {
  if (n < 1000) {
    snprintf(out, cap, "%llu", (unsigned long long)n);
  } else if (n < 10000) {
    // 4521 -> "4.5K"
    snprintf(out, cap, "%llu.%lluK",
             (unsigned long long)(n / 1000),
             (unsigned long long)((n % 1000) / 100));
  } else if (n < 1000000) {
    // 47200 -> "47K"
    snprintf(out, cap, "%lluK", (unsigned long long)(n / 1000));
  } else {
    // 3210000 -> "3.2M"
    snprintf(out, cap, "%llu.%lluM",
             (unsigned long long)(n / 1000000),
             (unsigned long long)((n % 1000000) / 100000));
  }
}
