#include "config.h"
#include "defaults.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#if __has_include("owm_credentials.h")
#include "owm_credentials.h"
#define OWM_CREDENTIALS_AVAILABLE
#endif

AppConfig cfg = {
    "",                        // ssid
    "",                        // password
    "",                        // apikey
    "api.openweathermap.org",  // server
    "",                        // city
    "",                        // latitude
    "",                        // longitude
    "EN",                      // language
    "M",                       // units
    "",                        // timezone
    "pool.ntp.org",            // ntp_server
    0,                         // gmt_offset_sec
    0,                         // daylight_offset_sec
    60,                        // sleep_duration
    8,                         // wakeup_hour
    23,                        // sleep_hour
    false                      // debug_display_update
};

bool LoadConfig() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return false;
  }
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println("No config file found");
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("Config parse error: %s\n", err.c_str());
    return false;
  }
  cfg.ssid = doc["ssid"] | "";
  cfg.password = doc["password"] | "";
  cfg.apikey = doc["apikey"] | "";
  cfg.server = doc["server"] | kDefaultServer;
  cfg.city = doc["city"] | "";
  cfg.latitude = doc["latitude"] | "";
  cfg.longitude = doc["longitude"] | "";
  cfg.language = doc["language"] | kDefaultLanguage;
  cfg.units = doc["units"] | kDefaultUnits;
  cfg.timezone = doc["timezone"] | "";
  cfg.ntp_server = doc["ntp_server"] | kDefaultNtpServer;
  cfg.gmt_offset_sec = doc["gmt_offset_sec"] | 0;
  cfg.daylight_offset_sec = doc["daylight_offset_sec"] | 0;
  cfg.sleep_duration = doc["sleep_duration"] | kDefaultSleepDuration;
  cfg.wakeup_hour = doc["wakeup_hour"] | kDefaultWakeupHour;
  cfg.sleep_hour = doc["sleep_hour"] | kDefaultSleepHour;
  cfg.debug_display_update = doc["debug_display_update"] | false;
  Serial.println("Config loaded from LittleFS");
  return true;
}

void SaveConfig() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed — config not saved");
    return;
  }
  JsonDocument doc;
  doc["ssid"] = cfg.ssid;
  doc["password"] = cfg.password;
  doc["apikey"] = cfg.apikey;
  doc["server"] = cfg.server;
  doc["city"] = cfg.city;
  doc["latitude"] = cfg.latitude;
  doc["longitude"] = cfg.longitude;
  doc["language"] = cfg.language;
  doc["units"] = cfg.units;
  doc["timezone"] = cfg.timezone;
  doc["ntp_server"] = cfg.ntp_server;
  doc["gmt_offset_sec"] = cfg.gmt_offset_sec;
  doc["daylight_offset_sec"] = cfg.daylight_offset_sec;
  doc["sleep_duration"] = cfg.sleep_duration;
  doc["wakeup_hour"] = cfg.wakeup_hour;
  doc["sleep_hour"] = cfg.sleep_hour;
  doc["debug_display_update"] = cfg.debug_display_update;
  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    Serial.println("Failed to open config.json for writing");
    return;
  }
  serializeJson(doc, f);
  f.close();
  Serial.println("Config saved to LittleFS");
}

bool IsConfigValid() {
  return !cfg.ssid.empty() && !cfg.apikey.empty() && !cfg.latitude.empty() && !cfg.longitude.empty() &&
         !cfg.timezone.empty();
}

bool SeedConfigFromHeader() {
#ifdef OWM_CREDENTIALS_AVAILABLE
  cfg.ssid = ssid;
  cfg.password = password;
  cfg.apikey = apikey.c_str();
  cfg.server = server;
  cfg.city = City.c_str();
  cfg.latitude = Latitude.c_str();
  cfg.longitude = Longitude.c_str();
  cfg.language = Language.c_str();
  cfg.units = Units.c_str();
  cfg.timezone = Timezone;
  cfg.ntp_server = ntpServer;
  cfg.gmt_offset_sec = gmtOffset_sec;
  cfg.daylight_offset_sec = daylightOffset_sec;
  cfg.debug_display_update = DebugDisplayUpdate;
  Serial.println("Config seeded from owm_credentials.h");
  SaveConfig();
  return true;
#else
  return false;
#endif
}
