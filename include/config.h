#pragma once

#include <Arduino.h>
#include <string>

// GPIO for the button that triggers config mode when held at boot.
#define CONFIG_BUTTON_PIN 39
#define CONFIG_BUTTON_HOLD_MS 2000

// Define to enable GET /config.json in the setup portal.
// Comment out to disable the endpoint entirely.
#define ENABLE_CONFIG_DOWNLOAD

// Define to redact the WiFi password from the /config.json response.
// Only has effect when ENABLE_CONFIG_DOWNLOAD is also defined.
// #define REDACT_PASSWORD_IN_CONFIG_DOWNLOAD

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
  std::string ntpServer;
  int gmtOffset_sec;
  int daylightOffset_sec;
  int sleepDuration;
  int wakeupHour;
  int sleepHour;
  bool debugDisplayUpdate;
};

extern AppConfig cfg;

bool LoadConfig();
void SaveConfig();
bool IsConfigValid();
bool SeedConfigFromHeader();
