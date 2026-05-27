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

static int qr_origin_x, qr_origin_y, qr_mod_size, qr_rendered_px;

static int DrawQR(const char* text, int originX, int originY, int moduleSize, int eccLevel = ESP_QRCODE_ECC_LOW) {
  qr_origin_x = originX;
  qr_origin_y = originY;
  qr_mod_size = moduleSize;
  qr_rendered_px = 0;

  esp_qrcode_config_t cfg = {
      .display_func =
          [](esp_qrcode_handle_t qrcode) {
            int size = esp_qrcode_get_size(qrcode);
            qr_rendered_px = size * qr_mod_size;
            int border = qr_mod_size * 2;
            FillRect(qr_origin_x - border, qr_origin_y - border, qr_rendered_px + border * 2,
                     qr_rendered_px + border * 2, kWhite);
            for (int y = 0; y < size; y++) {
              for (int x = 0; x < size; x++) {
                if (esp_qrcode_get_module(qrcode, x, y)) {
                  FillRect(qr_origin_x + x * qr_mod_size, qr_origin_y + y * qr_mod_size, qr_mod_size, qr_mod_size,
                           kBlack);
                }
              }
            }
          },
      .max_qrcode_version = 10,
      .qrcode_ecc_level = eccLevel,
  };

  esp_qrcode_generate(&cfg, text);
  return qr_rendered_px;
}

void DisplaySetupScreen(const char* ap_name) {
  epd_poweron();
  epd_fullclear(&epd_hl, (int)epd_ambient_temperature());

  int cx = epd_width() / 2;
  int w = epd_width();
  int ms = 6;
  int pad = 16;

  int qrPx = 33 * ms;
  int qrCxL = pad + qrPx / 2;
  int qrCxR = w - pad - qrPx / 2;

  char wifiQR[64];
  snprintf(wifiQR, sizeof(wifiQR), "WIFI:S:%s;T:nopass;;", ap_name);
  int wifiQRpx = DrawQR(wifiQR, pad, pad, ms, ESP_QRCODE_ECC_LOW);

  int rightQRx = w - pad - qrPx + (ms * 2);
  int portalQRpx = DrawQR("http://192.168.4.1", rightQRx, pad, ms, ESP_QRCODE_ECC_HIGH);

  SetFont(OpenSans12B);
  int labelGap = 24;
  int wifiGap = 34;
  int portalGap = 28;
  DrawString(qrCxL, pad + wifiQRpx + labelGap, "Scan to join", Alignment::kCenter);
  DrawString(qrCxL, pad + wifiQRpx + labelGap + wifiGap, "WiFi network", Alignment::kCenter);
  DrawString(qrCxR, pad + portalQRpx + labelGap, "Scan to open", Alignment::kCenter);
  DrawString(qrCxR, pad + portalQRpx + labelGap + portalGap, "config portal", Alignment::kCenter);

  int blockH = 270;
  int textY = (epd_height() - blockH) / 2;

  SetFont(OpenSans18B);
  DrawString(cx, textY - 48, "SETUP MODE", Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 48, "Connect to WiFi network:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 80, String(ap_name), Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 140, "Then open in a browser:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 172, "http://192.168.4.1", Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 228, "To update firmware:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 260, "http://192.168.4.1/update", Alignment::kCenter);

  EdpUpdate();
  epd_poweroff();
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
