#ifndef SIMULATOR_BUILD
#include <Arduino.h>
#include <WiFi.h>
#include "display.h"  // for time_str, date_str, current_hour/Min/Sec, current_hour extern
#include "defaults.h"
#include "power.h"

long sleep_duration;
int wakeup_hour;
int sleep_hour;
long start_time = 0;

void BeginSleep() {
  epd_poweroff();
  UpdateLocalTime();
  long delta = 30;  // ESP32 RTC speed compensation: prevents display at xx:59:yy then xx:00:yy one minute later
  long sleep_timer = (sleep_duration * 60 - ((current_min % sleep_duration) * 60 + current_sec)) + delta;
  esp_sleep_enable_timer_wakeup(sleep_timer * 1000000LL);
  Serial.printf("Awake for : %.3f-secs\n", (millis() - start_time) / 1000.0);
  Serial.printf("Entering %ld (secs) of sleep time\n", sleep_timer);
  Serial.printf("Starting deep-sleep period...\n");
  esp_deep_sleep_start();
}

bool SetupTime() {
  configTime(cfg.gmt_offset_sec, cfg.daylight_offset_sec, cfg.ntp_server, kDefaultNtpFallback);
  setenv("TZ", cfg.timezone, 1);
  tzset();
  delay(100);
  return UpdateLocalTime();
}

uint8_t StartWiFi() {
  Serial.printf("\r\nConnecting to: %s\n", cfg.ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid, cfg.password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(cfg.ssid, cfg.password);
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI();
    Serial.printf("WiFi connected at: %s\n", WiFi.localIP().toString().c_str());
  } else
    Serial.printf("WiFi connection *** FAILED ***\n");
  return WiFi.status();
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.printf("WiFi switched Off\n");
}
#endif  // SIMULATOR_BUILD
