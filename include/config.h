#pragma once

#include <Arduino.h>
#include <string>
#include "defaults.h"

// GPIO for the button that triggers config mode when held at boot.
// GPIO 34-39 have no internal pull-ups, so INPUT_PULLUP is a no-op here;
// the pin relies on the T5 4.7 board's external pull-up on this button.
#define CONFIG_BUTTON_PIN 39
#define CONFIG_BUTTON_HOLD_MS 2000

// Define to enable GET /config.json in the setup portal.
// Comment out to disable the endpoint entirely.
#define ENABLE_CONFIG_DOWNLOAD

// Define to redact the WiFi password / OWM API key from the /config.json response.
// The setup AP is open, so both are redacted by default; comment out to include them.
// Only has effect when ENABLE_CONFIG_DOWNLOAD is also defined.
#define REDACT_PASSWORD_IN_CONFIG_DOWNLOAD
#define REDACT_APIKEY_IN_CONFIG_DOWNLOAD

struct AppConfig {
  std::string ssid;
  std::string password;
  std::string apikey;
  std::string server;
  std::string city;
  std::string latitude;
  std::string longitude;
  std::string language;
  std::string units;
  std::string timezone;
  std::string ntp_server;
  int gmt_offset_sec;
  int daylight_offset_sec;
  int sleep_duration;
  int wakeup_hour;
  int sleep_hour;
  bool debug_display_update;
};

extern AppConfig cfg;

bool LoadConfig();
void SaveConfig();
bool IsConfigValid();
bool SeedConfigFromHeader();

// Returns a sleep duration (minutes) safe to use as a divisor: non-positive values fall back to
// kDefaultSleepDuration, anything else is clamped to [kMinSleepDuration, kMaxSleepDuration].
// Header-inline (no LittleFS dependency) so it can be unit tested natively.
inline int SanitizeSleepDuration(int minutes) {
  if (minutes <= 0) return kDefaultSleepDuration;
  if (minutes < kMinSleepDuration) return kMinSleepDuration;
  if (minutes > kMaxSleepDuration) return kMaxSleepDuration;
  return minutes;
}

// Clamps an hour-of-day setting (wakeup_hour / sleep_hour) to 0-23.
inline int SanitizeHour(int hour) {
  return hour < 0 ? 0 : (hour > 23 ? 23 : hour);
}
