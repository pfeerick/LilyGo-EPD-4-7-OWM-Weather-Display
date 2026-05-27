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
    "pool.ntp.org",            // ntpServer
    0,                         // gmtOffset_sec
    0,                         // daylightOffset_sec
    60,                        // sleep_duration
    8,                         // wakeup_hour
    23,                        // sleep_hour
    false                      // debugDisplayUpdate
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
  strlcpy(cfg.ssid, doc["ssid"] | "", sizeof(cfg.ssid));
  strlcpy(cfg.password, doc["password"] | "", sizeof(cfg.password));
  strlcpy(cfg.apikey, doc["apikey"] | "", sizeof(cfg.apikey));
  strlcpy(cfg.server, doc["server"] | kDefaultServer, sizeof(cfg.server));
  strlcpy(cfg.city, doc["city"] | "", sizeof(cfg.city));
  strlcpy(cfg.latitude, doc["latitude"] | "", sizeof(cfg.latitude));
  strlcpy(cfg.longitude, doc["longitude"] | "", sizeof(cfg.longitude));
  strlcpy(cfg.language, doc["language"] | kDefaultLanguage, sizeof(cfg.language));
  strlcpy(cfg.units, doc["units"] | kDefaultUnits, sizeof(cfg.units));
  strlcpy(cfg.timezone, doc["timezone"] | "", sizeof(cfg.timezone));
  strlcpy(cfg.ntp_server, doc["ntp_server"] | kDefaultNtpServer, sizeof(cfg.ntp_server));
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
  return cfg.ssid[0] != '\0' && cfg.apikey[0] != '\0' && cfg.latitude[0] != '\0' && cfg.longitude[0] != '\0' &&
         cfg.timezone[0] != '\0';
}

bool SeedConfigFromHeader() {
#ifdef OWM_CREDENTIALS_AVAILABLE
  strlcpy(cfg.ssid, ssid, sizeof(cfg.ssid));
  strlcpy(cfg.password, password, sizeof(cfg.password));
  strlcpy(cfg.apikey, apikey.c_str(), sizeof(cfg.apikey));
  strlcpy(cfg.server, server, sizeof(cfg.server));
  strlcpy(cfg.city, city.c_str(), sizeof(cfg.city));
  strlcpy(cfg.latitude, latitude.c_str(), sizeof(cfg.latitude));
  strlcpy(cfg.longitude, longitude.c_str(), sizeof(cfg.longitude));
  strlcpy(cfg.language, language.c_str(), sizeof(cfg.language));
  strlcpy(cfg.units, units.c_str(), sizeof(cfg.units));
  strlcpy(cfg.timezone, timezone, sizeof(cfg.timezone));
  strlcpy(cfg.ntp_server, ntp_server, sizeof(cfg.ntp_server));
  cfg.gmt_offset_sec = gmt_offset_sec;
  cfg.daylight_offset_sec = daylight_offset_sec;
  cfg.debug_display_update = debug_display_update;
  Serial.println("Config seeded from owm_credentials.h");
  SaveConfig();
  return true;
#else
  return false;
#endif
}
