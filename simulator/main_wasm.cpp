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

boolean large_icon = true;
boolean small_icon = false;
String time_str = "--:--:--";
String date_str = "-- --- ----";
int wifi_signal = -55;
int current_hour = 0, current_min = 0, current_sec = 0;

ForecastRecord wx_conditions;
ForecastRecord wx_forecast[kMaxReadings];

long sleep_duration = 30;
int wakeup_hour = 7;
int sleep_hour = 23;
long start_time = 0;
long sleep_timer = 0;
long delta = 30;

EpdFont current_font;
uint8_t* framebuffer;
static EpdiyHighlevelState epd_hl;

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

bool LoadConfig() {
  return false;
}
void SaveConfig() {}
bool IsConfigValid() {
  return cfg.apikey[0] != '\0' && cfg.latitude[0] != '\0' && cfg.longitude[0] != '\0';
}
bool SeedConfigFromHeader() {
  return false;
}

boolean UpdateLocalTime() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  current_hour = t.tm_hour;
  current_min = t.tm_min;
  current_sec = t.tm_sec;
  char day_buf[64], time_buf[32];
  if (strcmp(cfg.units, "M") == 0) {
    snprintf(day_buf, sizeof(day_buf), "%s, %02d %s %04d", weekday_D[t.tm_wday], t.tm_mday, month_M[t.tm_mon],
             t.tm_year + 1900);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &t);
  } else {
    strftime(day_buf, sizeof(day_buf), "%a %b-%d-%Y", &t);
    strftime(time_buf, sizeof(time_buf), "%r", &t);
  }
  date_str = day_buf;
  time_str = time_buf;
  return true;
}

void EdpUpdate() {}

#include "../src/weather_api.cpp"
#include "../src/display.cpp"
#include "../src/icons.cpp"

void DrawBattery(int x, int y) {
  const uint8_t percentage = 85;
  const float voltage = 4.1f;
  DrawRect(x + 25, y - 14, 40, 15, (uint16_t)0x00);
  FillRect(x + 65, y - 10, 4, 7, (uint16_t)0x00);
  FillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, (uint16_t)0x00);
  DrawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", Alignment::kLeft);
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

  wx_conditions.high = -50;
  wx_conditions.low = 50;

  JsonObject current = doc["current"];
  wx_conditions.sunrise = current["sunrise"];
  wx_conditions.sunset = current["sunset"];
  wx_conditions.temperature = current["temp"];
  wx_conditions.feels_like = current["feels_like"];
  wx_conditions.pressure = current["pressure"];
  wx_conditions.humidity = current["humidity"];
  wx_conditions.dew_point = current["dew_point"];
  wx_conditions.uvi = current["uvi"];
  wx_conditions.cloud_cover = current["clouds"];
  wx_conditions.visibility = current["visibility"];
  wx_conditions.wind_speed = current["wind_speed"];
  wx_conditions.wind_dir = current["wind_deg"];

  strlcpy(wx_conditions.forecast0, current["weather"][0]["description"] | "", sizeof(wx_conditions.forecast0));
  strlcpy(wx_conditions.icon, current["weather"][0]["icon"] | "", sizeof(wx_conditions.icon));

  JsonArray daily = root["daily"];
  wx_conditions.low = daily[0]["temp"]["min"].as<float>();
  wx_conditions.high = daily[0]["temp"]["max"].as<float>();

  JsonArray list = root["hourly"];
  byte wxIndex = 0;
  for (byte r = 0; r < 48 && wxIndex < 16; r += 3) {
    wx_forecast[wxIndex].dt = list[r]["dt"].as<int>();
    wx_forecast[wxIndex].temperature = list[r]["temp"].as<float>();
    float t1 = (r + 1 < (int)list.size()) ? list[r + 1]["temp"].as<float>() : wx_forecast[wxIndex].temperature;
    float t2 = (r + 2 < (int)list.size()) ? list[r + 2]["temp"].as<float>() : wx_forecast[wxIndex].temperature;
    float temps[3] = {wx_forecast[wxIndex].temperature, t1, t2};
    wx_forecast[wxIndex].high = (temps[0] > temps[1] ? (temps[0] > temps[2] ? temps[0] : temps[2])
                                                     : (temps[1] > temps[2] ? temps[1] : temps[2]));
    wx_forecast[wxIndex].low = (temps[0] < temps[1] ? (temps[0] < temps[2] ? temps[0] : temps[2])
                                                    : (temps[1] < temps[2] ? temps[1] : temps[2]));
    wx_forecast[wxIndex].pressure = list[r]["pressure"].as<float>();
    wx_forecast[wxIndex].humidity = list[r]["humidity"].as<float>();
    strlcpy(wx_forecast[wxIndex].icon, list[r]["weather"][0]["icon"] | "", sizeof(wx_forecast[wxIndex].icon));
    wx_forecast[wxIndex].rainfall = list[r]["rain"]["1h"].as<float>();
    wx_forecast[wxIndex].snowfall = list[r]["snow"]["1h"].as<float>();

    wxIndex++;
  }
  if (wxIndex >= 3) {
    float pt = wx_forecast[0].pressure - wx_forecast[2].pressure;
    pt = ((int)(pt * 10)) / 10.0f;
    wx_conditions.trend = (pt > 0) ? '+' : (pt < 0) ? '-' : '0';
  } else {
    wx_conditions.trend = '0';
  }
  if (strcmp(cfg.units, "I") == 0) ConvertReadingsToImperial(wxIndex);
  return true;
}

// ---- Exported WASM API ----

extern "C" {

EMSCRIPTEN_KEEPALIVE void wasm_init() {
  epd_hl = epd_hl_init(nullptr);
  framebuffer = epd_pc_get_framebuffer();
  epd_hl_set_all_white(&epd_hl);
}

EMSCRIPTEN_KEEPALIVE void wasm_set_config(const char* city, const char* units, const char* lang, float latitude,
                                          int gmt_offset_sec, int dst_offset_sec) {
  if (city) strncpy(cfg.city, city, sizeof(cfg.city) - 1);
  if (units) strncpy(cfg.units, units, sizeof(cfg.units) - 1);
  if (lang) strncpy(cfg.language, lang, sizeof(cfg.language) - 1);
  snprintf(cfg.latitude, sizeof(cfg.latitude), "%.6f", latitude);
  cfg.gmt_offset_sec = gmt_offset_sec;
  cfg.daylight_offset_sec = dst_offset_sec;
}

EMSCRIPTEN_KEEPALIVE int wasm_render(const char* json, int len) {
  UpdateLocalTime();
  epd_hl_set_all_white(&epd_hl);
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
