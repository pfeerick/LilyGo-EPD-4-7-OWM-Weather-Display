#pragma once

#include <Arduino.h>

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
  char ssid[64];
  char password[64];
  char apikey[64];
  char server[64];
  char city[64];
  char latitude[16];
  char longitude[16];
  char language[8];
  char units[4];
  char timezone[80];
  char ntp_server[64];
  int gmt_offset_sec;
  int daylight_offset_sec;
  int sleep_duration;
  int wakeup_hour;
  int sleep_hour;
  bool debug_display_update;
};

extern AppConfig cfg;

bool loadConfig();
void saveConfig();
bool isConfigValid();
bool seedConfigFromHeader();
