#ifndef SIMULATOR_BUILD
#include <Arduino.h>
#include <WiFi.h>
#include "display.h"  // for Time_str, Date_str, CurrentHour/Min/Sec, CurrentHour extern
#include "defaults.h"
#include "power.h"

long SleepDuration;
int WakeupHour;
int SleepHour;
long StartTime = 0;

void BeginSleep() {
  epd_poweroff();
  UpdateLocalTime();
  long Delta = 30;  // ESP32 RTC speed compensation: prevents display at xx:59:yy then xx:00:yy one minute later
  long SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta;
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL);
  Serial.printf("Awake for : %.3f-secs\n", (millis() - StartTime) / 1000.0);
  Serial.printf("Entering %ld (secs) of sleep time\n", SleepTimer);
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
