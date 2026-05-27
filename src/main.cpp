#ifndef SIMULATOR_BUILD
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <SPI.h>
#include <time.h>
#include <LittleFS.h>
#include "qrcode.h"
#include "config.h"
#include "defaults.h"
#include "setup_portal.h"
#include "weather_api.h"
#include "display.h"
#include "icons.h"
#include "power.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"

// ---------------------------------------------------------------------------
// QR + setup screen (only used during first-boot config mode)
// ---------------------------------------------------------------------------

static int _ox, _oy, _ms, _renderedPx;

static int drawQR(const char* text, int originX, int originY, int moduleSize, int eccLevel = ESP_QRCODE_ECC_LOW) {
  _ox = originX;
  _oy = originY;
  _ms = moduleSize;
  _renderedPx = 0;

  esp_qrcode_config_t cfg = {
      .display_func =
          [](esp_qrcode_handle_t qrcode) {
            int size = esp_qrcode_get_size(qrcode);
            _renderedPx = size * _ms;
            int border = _ms * 2;
            fillRect(_ox - border, _oy - border, _renderedPx + border * 2, _renderedPx + border * 2, White);
            for (int y = 0; y < size; y++) {
              for (int x = 0; x < size; x++) {
                if (esp_qrcode_get_module(qrcode, x, y)) {
                  fillRect(_ox + x * _ms, _oy + y * _ms, _ms, _ms, Black);
                }
              }
            }
          },
      .max_qrcode_version = 10,
      .qrcode_ecc_level = eccLevel,
  };

  esp_qrcode_generate(&cfg, text);
  return _renderedPx;
}

void DisplaySetupScreen(const char* apName) {
  epd_poweron();
  epd_fullclear(&hl, (int)epd_ambient_temperature());

  int cx = epd_width() / 2;
  int w = epd_width();
  int ms = 6;
  int pad = 16;

  int qrPx = 33 * ms;
  int qrCxL = pad + qrPx / 2;
  int qrCxR = w - pad - qrPx / 2;

  char wifiQR[64];
  snprintf(wifiQR, sizeof(wifiQR), "WIFI:S:%s;T:nopass;;", apName);
  int wifiQRpx = drawQR(wifiQR, pad, pad, ms, ESP_QRCODE_ECC_LOW);

  int rightQRx = w - pad - qrPx + (ms * 2);
  int portalQRpx = drawQR("http://192.168.4.1", rightQRx, pad, ms, ESP_QRCODE_ECC_HIGH);

  setFont(OpenSans12B);
  int labelGap = 24;
  int wifiGap = 34;
  int portalGap = 28;
  drawString(qrCxL, pad + wifiQRpx + labelGap, "Scan to join", CENTER);
  drawString(qrCxL, pad + wifiQRpx + labelGap + wifiGap, "WiFi network", CENTER);
  drawString(qrCxR, pad + portalQRpx + labelGap, "Scan to open", CENTER);
  drawString(qrCxR, pad + portalQRpx + labelGap + portalGap, "config portal", CENTER);

  int blockH = 270;
  int textY = (epd_height() - blockH) / 2;

  setFont(OpenSans18B);
  drawString(cx, textY - 48, "SETUP MODE", CENTER);
  setFont(OpenSans12B);
  drawString(cx, textY + 48, "Connect to WiFi network:", CENTER);
  setFont(OpenSans18B);
  drawString(cx, textY + 80, String(apName), CENTER);
  setFont(OpenSans12B);
  drawString(cx, textY + 140, "Then open in a browser:", CENTER);
  setFont(OpenSans18B);
  drawString(cx, textY + 172, "http://192.168.4.1", CENTER);
  setFont(OpenSans12B);
  drawString(cx, textY + 228, "To update firmware:", CENTER);
  setFont(OpenSans18B);
  drawString(cx, textY + 260, "http://192.168.4.1/update", CENTER);

  edp_update();
  epd_poweroff();
}

// ---------------------------------------------------------------------------

void loop() {}

void setup() {
  InitialiseSystem();

  bool forceConfig = false;
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    delay(CONFIG_BUTTON_HOLD_MS);
    if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
      Serial.println("Config button held — entering setup mode");
      forceConfig = true;
    }
  }

  if (!loadConfig() || !isConfigValid()) {
    if (!seedConfigFromHeader() || !isConfigValid()) {
      Serial.println("No valid config found — entering setup mode");
      forceConfig = true;
    }
  }

  SleepDuration = cfg.sleepDuration;
  WakeupHour    = cfg.wakeupHour;
  SleepHour     = cfg.sleepHour;

  if (forceConfig) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char apName[24];
    snprintf(apName, sizeof(apName), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(apName);
    enterSetupMode();
  }

  if (StartWiFi() != WL_CONNECTED) {
    Serial.println("WiFi failed — entering setup mode");
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char apName[24];
    snprintf(apName, sizeof(apName), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(apName);
    enterSetupMode();
  }

  if (SetupTime()) {
    bool WakeUp = false;
    if (WakeupHour > SleepHour)
      WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
    else
      WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
    if (WakeUp) {
      byte Attempts = 1;
      bool RxWeather = false;
      WiFiClient client;
      while (!RxWeather && Attempts <= 2) {
        if (!RxWeather) RxWeather = obtainWeatherData(client, "onecall");
        Attempts++;
      }
      StopWiFi();
      if (RxWeather) {
        Serial.printf("Received all weather data, updating display...\n");
        epd_poweron();
        epd_fullclear(&hl, (int)epd_ambient_temperature());
        DisplayWeather();
        edp_update();
        epd_poweroff();
      } else {
        Serial.printf("Failed to receive weather data, skipping display update.\n");
      }
    }
  }
  BeginSleep();
}
#endif  // SIMULATOR_BUILD
