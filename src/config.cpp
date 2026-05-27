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
    60,                        // sleepDuration
    8,                         // wakeupHour
    23,                        // sleepHour
    false                      // debugDisplayUpdate
};

bool loadConfig() {
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
  strlcpy(cfg.ntpServer, doc["ntpServer"] | kDefaultNtpServer, sizeof(cfg.ntpServer));
  cfg.gmtOffset_sec = doc["gmtOffset_sec"] | 0;
  cfg.daylightOffset_sec = doc["daylightOffset_sec"] | 0;
  cfg.sleepDuration = doc["sleepDuration"] | kDefaultSleepDuration;
  cfg.wakeupHour = doc["wakeupHour"] | kDefaultWakeupHour;
  cfg.sleepHour = doc["sleepHour"] | kDefaultSleepHour;
  cfg.debugDisplayUpdate = doc["debugDisplayUpdate"] | false;
  Serial.println("Config loaded from LittleFS");
  return true;
}

void saveConfig() {
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
  doc["ntpServer"] = cfg.ntpServer;
  doc["gmtOffset_sec"] = cfg.gmtOffset_sec;
  doc["daylightOffset_sec"] = cfg.daylightOffset_sec;
  doc["sleepDuration"] = cfg.sleepDuration;
  doc["wakeupHour"] = cfg.wakeupHour;
  doc["sleepHour"] = cfg.sleepHour;
  doc["debugDisplayUpdate"] = cfg.debugDisplayUpdate;
  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    Serial.println("Failed to open config.json for writing");
    return;
  }
  serializeJson(doc, f);
  f.close();
  Serial.println("Config saved to LittleFS");
}

bool isConfigValid() {
  return cfg.ssid[0] != '\0' && cfg.apikey[0] != '\0' && cfg.latitude[0] != '\0' && cfg.longitude[0] != '\0' &&
         cfg.timezone[0] != '\0';
}

bool seedConfigFromHeader() {
#ifdef OWM_CREDENTIALS_AVAILABLE
  strlcpy(cfg.ssid, ssid, sizeof(cfg.ssid));
  strlcpy(cfg.password, password, sizeof(cfg.password));
  strlcpy(cfg.apikey, apikey.c_str(), sizeof(cfg.apikey));
  strlcpy(cfg.server, server, sizeof(cfg.server));
  strlcpy(cfg.city, City.c_str(), sizeof(cfg.city));
  strlcpy(cfg.latitude, Latitude.c_str(), sizeof(cfg.latitude));
  strlcpy(cfg.longitude, Longitude.c_str(), sizeof(cfg.longitude));
  strlcpy(cfg.language, Language.c_str(), sizeof(cfg.language));
  strlcpy(cfg.units, Units.c_str(), sizeof(cfg.units));
  strlcpy(cfg.timezone, Timezone, sizeof(cfg.timezone));
  strlcpy(cfg.ntpServer, ntpServer, sizeof(cfg.ntpServer));
  cfg.gmtOffset_sec = gmtOffset_sec;
  cfg.daylightOffset_sec = daylightOffset_sec;
  cfg.debugDisplayUpdate = DebugDisplayUpdate;
  Serial.println("Config seeded from owm_credentials.h");
  saveConfig();
  return true;
#else
  return false;
#endif
}
