// WASM entry point for the weather display simulator.
// OWM HTTP fetch is done in JavaScript; this module handles JSON decoding and rendering.
#define SIMULATOR_BUILD 1

#include "simulator_stubs.h"
#include "epdiy_pc/epdiy_pc.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../.pio/libdeps/display/ArduinoJson/src/ArduinoJson.h"

#include "../include/config.h"
#include "../include/forecast_record.h"
#include "../include/translations/lang_en.h"
#include "../include/fonts/opensans8b.h"
#include "../include/fonts/opensans10b.h"
#include "../include/fonts/opensans12b.h"
#include "../include/fonts/opensans18b.h"
#include "../include/fonts/opensans24b.h"
#include "../include/images/moon.h"
#include "../include/images/sunrise.h"
#include "../include/images/sunset.h"
#include "../include/images/uvi.h"

#include "../include/display.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
// Allow the file to compile under a native toolchain (e.g. for IDE checks).
// EMSCRIPTEN_KEEPALIVE becomes a no-op; the exported functions exist but are
// not linked as WASM exports.
#define EMSCRIPTEN_KEEPALIVE
#endif

// ---- Globals expected by the rendering code ----

boolean LargeIcon = true;
boolean SmallIcon = false;
String Time_str = "--:--:--";
String Date_str = "-- --- ----";
int wifi_signal = -55;
int CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;

Forecast_record_type WxConditions;
Forecast_record_type WxForecast[kMaxReadings];

long SleepDuration = 30;
int WakeupHour = 7;
int SleepHour = 23;
long StartTime = 0;
long SleepTimer = 0;
long Delta = 30;

EpdFont currentFont;
uint8_t* framebuffer;
static EpdiyHighlevelState hl;

// ---- Config ----

AppConfig cfg = {"",                        // ssid
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
                 0,
                 0,
                 30,
                 7,
                 23,
                 false};

bool loadConfig() {
  return false;
}
void saveConfig() {}
bool isConfigValid() {
  return cfg.apikey[0] != '\0' && cfg.latitude[0] != '\0' && cfg.longitude[0] != '\0';
}
bool seedConfigFromHeader() {
  return false;
}

boolean UpdateLocalTime() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  CurrentHour = t.tm_hour;
  CurrentMin = t.tm_min;
  CurrentSec = t.tm_sec;
  char day_buf[64], time_buf[32];
  if (strcmp(cfg.units, "M") == 0) {
    snprintf(day_buf, sizeof(day_buf), "%s, %02d %s %04d", weekday_D[t.tm_wday], t.tm_mday, month_M[t.tm_mon],
             t.tm_year + 1900);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &t);
  } else {
    strftime(day_buf, sizeof(day_buf), "%a %b-%d-%Y", &t);
    strftime(time_buf, sizeof(time_buf), "%r", &t);
  }
  Date_str = day_buf;
  Time_str = time_buf;
  return true;
}

void edp_update() {}

#include "../src/weather_api.cpp"
#include "../src/display.cpp"
#include "../src/icons.cpp"

void DrawBattery(int x, int y) {
  const uint8_t percentage = 85;
  const float voltage = 4.1f;
  drawRect(x + 25, y - 14, 40, 15, (uint16_t)0x00);
  fillRect(x + 65, y - 10, 4, 7, (uint16_t)0x00);
  fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, (uint16_t)0x00);
  drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", Alignment::kLeft);
}

bool DecodeWeather(const std::string& json, String Type) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    fprintf(stderr, "deserializeJson() failed: %s\n", error.c_str());
    return false;
  }
  JsonObject root = doc.as<JsonObject>();
  fprintf(stderr, "Decoding %s data\n", Type.c_str());

  WxConditions.High = -50;
  WxConditions.Low = 50;

  JsonObject current = doc["current"];
  WxConditions.Sunrise = current["sunrise"];
  WxConditions.Sunset = current["sunset"];
  WxConditions.Temperature = current["temp"];
  WxConditions.FeelsLike = current["feels_like"];
  WxConditions.Pressure = current["pressure"];
  WxConditions.Humidity = current["humidity"];
  WxConditions.DewPoint = current["dew_point"];
  WxConditions.UVI = current["uvi"];
  WxConditions.Cloudcover = current["clouds"];
  WxConditions.Visibility = current["visibility"];
  WxConditions.Windspeed = current["wind_speed"];
  WxConditions.Winddir = current["wind_deg"];

  strlcpy(WxConditions.Forecast0, current["weather"][0]["description"] | "", sizeof(WxConditions.Forecast0));
  strlcpy(WxConditions.Icon, current["weather"][0]["icon"] | "", sizeof(WxConditions.Icon));

  JsonArray daily = root["daily"];
  WxConditions.Low = daily[0]["temp"]["min"].as<float>();
  WxConditions.High = daily[0]["temp"]["max"].as<float>();

  JsonArray list = root["hourly"];
  byte wxIndex = 0;
  for (byte r = 0; r < 48 && wxIndex < 16; r += 3) {
    WxForecast[wxIndex].Dt = list[r]["dt"].as<int>();
    WxForecast[wxIndex].Temperature = list[r]["temp"].as<float>();
    float t1 = (r + 1 < (int)list.size()) ? list[r + 1]["temp"].as<float>() : WxForecast[wxIndex].Temperature;
    float t2 = (r + 2 < (int)list.size()) ? list[r + 2]["temp"].as<float>() : WxForecast[wxIndex].Temperature;
    float temps[3] = {WxForecast[wxIndex].Temperature, t1, t2};
    WxForecast[wxIndex].High = (temps[0] > temps[1] ? (temps[0] > temps[2] ? temps[0] : temps[2])
                                                    : (temps[1] > temps[2] ? temps[1] : temps[2]));
    WxForecast[wxIndex].Low = (temps[0] < temps[1] ? (temps[0] < temps[2] ? temps[0] : temps[2])
                                                   : (temps[1] < temps[2] ? temps[1] : temps[2]));
    WxForecast[wxIndex].Pressure = list[r]["pressure"].as<float>();
    WxForecast[wxIndex].Humidity = list[r]["humidity"].as<float>();
    strlcpy(WxForecast[wxIndex].Icon, list[r]["weather"][0]["icon"] | "", sizeof(WxForecast[wxIndex].Icon));
    WxForecast[wxIndex].Rainfall = list[r]["rain"]["1h"].as<float>();
    WxForecast[wxIndex].Snowfall = list[r]["snow"]["1h"].as<float>();

    wxIndex++;
  }
  if (wxIndex >= 3) {
    float pt = WxForecast[0].Pressure - WxForecast[2].Pressure;
    pt = ((int)(pt * 10)) / 10.0f;
    WxConditions.Trend = (pt > 0) ? '+' : (pt < 0) ? '-' : '0';
  } else {
    WxConditions.Trend = '0';
  }
  if (strcmp(cfg.units, "I") == 0) Convert_Readings_to_Imperial(wxIndex);
  return true;
}

// ---- Exported WASM API ----

extern "C" {

EMSCRIPTEN_KEEPALIVE void wasm_init() {
  hl = epd_hl_init(nullptr);
  framebuffer = epd_pc_get_framebuffer();
  epd_hl_set_all_white(&hl);
}

EMSCRIPTEN_KEEPALIVE void wasm_set_config(const char* city, const char* units, const char* lang, float latitude,
                                          int gmt_offset_sec, int dst_offset_sec) {
  if (city) strncpy(cfg.city, city, sizeof(cfg.city) - 1);
  if (units) strncpy(cfg.units, units, sizeof(cfg.units) - 1);
  if (lang) strncpy(cfg.language, lang, sizeof(cfg.language) - 1);
  snprintf(cfg.latitude, sizeof(cfg.latitude), "%.6f", latitude);
  cfg.gmtOffset_sec = gmt_offset_sec;
  cfg.daylightOffset_sec = dst_offset_sec;
}

EMSCRIPTEN_KEEPALIVE int wasm_render(const char* json, int len) {
  UpdateLocalTime();
  epd_hl_set_all_white(&hl);
  std::string json_str(json, len);
  if (!DecodeWeather(json_str, "onecall")) return -1;
  DisplayWeather();
  return 0;
}

EMSCRIPTEN_KEEPALIVE uint8_t* wasm_get_framebuffer() {
  return epd_pc_get_framebuffer();
}

EMSCRIPTEN_KEEPALIVE int wasm_fb_size() {
  return (epd_width() / 2) * epd_height();
}

}  // extern "C"
