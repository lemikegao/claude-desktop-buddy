#pragma once
// Daystats: NVS-backed agent active-time counters.
//
// "Agent today (s)" = wall-clock seconds today during which Claude had at
//   least one session running or waiting on a permission. Reset at local
//   midnight via the M5 RTC (set by the desktop's `time` message on connect).
// "Agent total (s)" = device-observed cumulative agent active seconds.
//   Persisted to NVS, never resets. Pre-RTC-sync ticks still count toward
//   total; only the daily reset is gated on a valid RTC.
//
// Why time, not tokens: tokens were a holdover from upstream and read as
// an abstract number on the LCD. The user-facing question this fork wants
// to answer is "how much of my day is the agent actually working?" — i.e.
// utilization of agent time. Tokens correlate but don't communicate that.
// The desktop still sends `tokens`/`tokens_today` in heartbeats; we just
// ignore them.
//
// Header-only by design (per claude-integration.md). All state is
// file-static; daystatsInit() loads from NVS. Persistence is throttled to
// once per 30 s and triggered from the snapshot callback.

#include <M5Unified.h>
#include <Preferences.h>
#include <Arduino.h>
#include <time.h>
#include <stdint.h>

// ---- state ----------------------------------------------------------------
static Preferences _dsPrefs;
static uint64_t _dsAgentTotalS         = 0;   // lifetime active seconds
static uint32_t _dsAgentTodayS         = 0;   // today's active seconds
static int32_t  _dsTodayYday           = -1;  // tm_yday of "today"; -1 = unset
static uint32_t _dsLastSnapshotMs      = 0;
static bool     _dsLastActive          = false;
static bool     _dsSeen                = false;
static uint32_t _dsLastPersistMs       = 0;
static bool     _dsDirty               = false;

// ---- helpers --------------------------------------------------------------
static inline bool _dsRtcValid() {
  m5::rtc_datetime_t dt;
  if (!M5.Rtc.getDateTime(&dt)) return false;
  return dt.date.year >= 2025;
}

// Day-of-year for "today". -1 if RTC not yet sync'd. mktime fills in tm_yday
// from year/month/day; mid-day hour dodges DST edge cases.
static inline int32_t _dsTodayKey() {
  if (!_dsRtcValid()) return -1;
  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);
  struct tm t = {};
  t.tm_year = dt.date.year - 1900;
  t.tm_mon  = dt.date.month - 1;
  t.tm_mday = dt.date.date;
  t.tm_hour = 12;
  mktime(&t);
  // Combine year and yday so a New Year's rollover doesn't collide with
  // the previous year's day-of-year (e.g. day 365 -> day 0 looks "earlier"
  // by yday alone but is actually the next day).
  return t.tm_year * 1000 + t.tm_yday;
}

static inline void _dsPersistIfDirty(uint32_t now, bool force = false) {
  if (!_dsDirty) return;
  if (!force && (now - _dsLastPersistMs) < 30000) return;
  _dsPrefs.putULong64("totS",   _dsAgentTotalS);
  _dsPrefs.putUInt   ("todayS", _dsAgentTodayS);
  _dsPrefs.putInt    ("yday",   _dsTodayYday);
  _dsLastPersistMs = now;
  _dsDirty = false;
}

// ---- public ---------------------------------------------------------------
inline void daystatsInit() {
  _dsPrefs.begin("daystats", false);
  _dsAgentTotalS = _dsPrefs.getULong64("totS",   0);
  _dsAgentTodayS = _dsPrefs.getUInt   ("todayS", 0);
  _dsTodayYday   = _dsPrefs.getInt    ("yday",   -1);
}

// Reset today's bucket if we've crossed local midnight. No-op when the RTC
// isn't sync'd yet (we don't know what day it is). Total never resets.
inline void daystatsCheckRollover() {
  int32_t key = _dsTodayKey();
  if (key < 0) return;
  if (_dsTodayYday < 0) {
    _dsTodayYday = key;
    _dsDirty = true;
    return;
  }
  if (key != _dsTodayYday) {
    Serial.printf("[daystats] midnight: %u s archived from yesterday\n",
                  (unsigned)_dsAgentTodayS);
    _dsAgentTodayS = 0;
    _dsTodayYday = key;
    _dsDirty = true;
  }
}

// Called from the heartbeat handler on every snapshot. `active` is true
// only when a session is actually generating — `waiting > 0` alone doesn't
// count, because that's the agent stopped, blocked on a permission prompt
// I haven't answered. Counting "waiting" would inflate the metric in
// exactly the direction we want it to discourage (idle-while-prompted).
// Between snapshots we add the elapsed wall-clock seconds to today/total
// iff the *previous* snapshot was active — that's what "agent was running
// for the past N seconds" means. `waiting` is still a parameter so callers
// don't need to know the predicate; the buddy's ATTENTION persona keeps
// using `waiting` independently to flag prompts on the LCD.
inline void daystatsOnSnapshot(uint8_t running, uint8_t waiting) {
  (void)waiting;
  daystatsCheckRollover();
  uint32_t now = millis();
  bool active = (running > 0);
  if (_dsSeen && _dsLastActive) {
    uint32_t deltaMs = now - _dsLastSnapshotMs;
    // Cap at 60s: heartbeats arrive every ~10s, and the freshness window
    // is 30s. A bigger gap means we lost contact — don't credit the void.
    if (deltaMs > 60000) deltaMs = 0;
    if (deltaMs >= 1000) {
      uint32_t deltaS = deltaMs / 1000;
      _dsAgentTodayS += deltaS;
      _dsAgentTotalS += deltaS;
      _dsDirty = true;
    }
  }
  _dsLastSnapshotMs = now;
  _dsLastActive = active;
  _dsSeen = true;
  _dsPersistIfDirty(now);
}

inline uint32_t daystatsAgentTodayS() { return _dsAgentTodayS; }
inline uint64_t daystatsAgentTotalS() { return _dsAgentTotalS; }
inline bool     daystatsSeen()        { return _dsSeen; }

// Compact duration formatter for the LCD:
//   < 60s   → "0m"        (sub-minute reads as zero — the buddy isn't a stopwatch)
//   < 60m   → "47m"
//   < 24h   → "2h 34m"
//   ≥ 24h   → "47h"        (drop minutes once you're in the dozens of hours)
inline void daystatsFmtDuration(char* out, size_t cap, uint64_t s) {
  if (s < 60) {
    snprintf(out, cap, "0m");
  } else if (s < 3600) {
    snprintf(out, cap, "%llum", (unsigned long long)(s / 60));
  } else if (s < 24ULL * 3600) {
    uint64_t h = s / 3600;
    uint64_t m = (s % 3600) / 60;
    snprintf(out, cap, "%lluh %llum", (unsigned long long)h, (unsigned long long)m);
  } else {
    snprintf(out, cap, "%lluh", (unsigned long long)(s / 3600));
  }
}
