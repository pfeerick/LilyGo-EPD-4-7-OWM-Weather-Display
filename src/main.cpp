#ifndef SIMULATOR_BUILD
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <SPI.h>
#include <time.h>
#include <LittleFS.h>
#include "config.h"
#include "defaults.h"
#include "setup_portal.h"
#include "setup_screen.h"
#include "weather_api.h"
#include "display.h"
#include "icons.h"
#include "power.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"

//image "fonts"
#include "images/moon.h"
#include "images/sunrise.h"
#include "images/sunset.h"
#include "images/uvi.h"

#endif  // SIMULATOR_BUILD

#ifndef SIMULATOR_BUILD
void InitialiseSystem() {
  start_time = millis();
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.printf("%s\nStarting...\n", __FILE__);
  epd_init(&epd_board_lilygo_t5_47, &ED047TC2, EPD_LUT_64K);
  epd_set_vcom(1560);
  epd_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
  framebuffer = epd_hl_get_framebuffer(&epd_hl);
  epd_hl_set_all_white(&epd_hl);
}

// ---------------------------------------------------------------------------

void loop() {}

void setup() {
  InitialiseSystem();

  bool force_config = false;
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    delay(CONFIG_BUTTON_HOLD_MS);
    if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
      Serial.println("Config button held — entering setup mode");
      force_config = true;
    }
  }

  if (!LoadConfig() || !IsConfigValid()) {
    if (!SeedConfigFromHeader() || !IsConfigValid()) {
      Serial.println("No valid config found — entering setup mode");
      force_config = true;
    }
  }

  sleep_duration = cfg.sleep_duration;
  wakeup_hour = cfg.wakeup_hour;
  sleep_hour = cfg.sleep_hour;

  if (force_config) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char ap_name[24];
    snprintf(ap_name, sizeof(ap_name), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(ap_name);
    EnterSetupMode();
  }

  if (StartWiFi() != WL_CONNECTED) {
    Serial.println("WiFi failed — entering setup mode");
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char ap_name[24];
    snprintf(ap_name, sizeof(ap_name), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(ap_name);
    EnterSetupMode();
  }

  if (SetupTime()) {
    bool wake_up = false;
    if (wakeup_hour > sleep_hour)
      wake_up = (current_hour >= wakeup_hour || current_hour <= sleep_hour);
    else
      wake_up = (current_hour >= wakeup_hour && current_hour <= sleep_hour);
    if (wake_up) {
      uint8_t attempts = 1;
      bool rx_weather = false;
      WiFiClient client;
      while (!rx_weather && attempts <= 2) {
        rx_weather = ObtainWeatherData(client, "onecall");
        attempts++;
      }
      StopWiFi();
      if (rx_weather) {
        Serial.printf("Received all weather data, updating display...\n");
        epd_poweron();
        epd_fullclear(&epd_hl, (int)epd_ambient_temperature());
        DisplayWeather();
        EdpUpdate();
        epd_poweroff();
      } else {
        Serial.printf("Failed to receive weather data, skipping display update.\n");
      }
    }
  }
  BeginSleep();
}
#endif  // SIMULATOR_BUILD
