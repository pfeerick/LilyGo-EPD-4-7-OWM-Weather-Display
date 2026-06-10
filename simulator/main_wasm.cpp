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
#include "../include/defaults.h"
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
                 "pool.ntp.org",            // ntp_server
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
  return !cfg.apikey.empty() && !cfg.latitude.empty() && !cfg.longitude.empty();
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
  if (cfg.units == "M") {
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
#include "../src/setup_screen.cpp"

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
    Serial.printf("deserializeJson() failed: %s\n", error.c_str());
    return false;
  }
  return ParseWeatherDoc(doc, Type);
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
  if (city) cfg.city = city;
  if (units) cfg.units = units;
  if (lang) cfg.language = lang;
  char lat_buf[32];
  snprintf(lat_buf, sizeof(lat_buf), "%.6f", latitude);
  cfg.latitude = lat_buf;
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

EMSCRIPTEN_KEEPALIVE void wasm_render_setup(const char* ap_name) {
  epd_hl_set_all_white(&epd_hl);
  DisplaySetupScreen(ap_name);
}

EMSCRIPTEN_KEEPALIVE uint8_t* wasm_get_framebuffer() {
  return epd_pc_get_framebuffer();
}

EMSCRIPTEN_KEEPALIVE int wasm_fb_size() {
  return (epd_width() / 2) * epd_height();
}

}  // extern "C"
