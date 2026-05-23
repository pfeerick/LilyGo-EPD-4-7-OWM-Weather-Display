// PC-side weather display simulator.
// Fetches live OWM data, runs the real rendering code, writes 259,200 bytes to stdout.
#define PC_SIMULATOR_BUILD 1

#include "pc_stubs.h"

// EPDIY types — self-contained PC header (no ESP-IDF/xtensa deps)
#include "epdiy_pc/epdiy_pc.h"

// ArduinoJson (header-only, compiles on PC).
// Must undef min/max macros from pc_stubs.h — they break .as<T>() template calls.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../.pio/libdeps/display/ArduinoJson/src/ArduinoJson.h"


// Project includes (all needed by rendering code and DecodeWeather)
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

#include "owm_http.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

// ---- Globals expected by the rendering code ----

boolean LargeIcon = true;
boolean SmallIcon = false;
#define Large 20
#define Small 10
String Time_str = "--:--:--";
String Date_str = "-- --- ----";
int wifi_signal = -55;
int CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, vref = 1100;

#define max_readings 24
#define max_graph_readings 16

Forecast_record_type WxConditions[1];
Forecast_record_type WxForecast[max_readings];

float pressure_readings[max_readings] = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings] = {0};
float rain_readings[max_readings] = {0};
float snow_readings[max_readings] = {0};

long SleepDuration = 30;
int WakeupHour = 7;
int SleepHour = 23;
long StartTime = 0;
long SleepTimer = 0;
long Delta = 30;

EpdFont currentFont;
uint8_t* framebuffer;
static EpdiyHighlevelState hl;

// ---- Config (extern AppConfig cfg declared in config.h) ----

AppConfig cfg = {"",                        // ssid
                 "",                        // password
                 "",                        // apikey
                 "api.openweathermap.org",  // server
                 "",                        // city
                 "",                        // latitude
                 "",                        // longitude
                 "EN",                      // language
                 "north",                   // hemisphere
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

// ---- PC implementations of hardware-dependent helpers ----

// UpdateLocalTime — uses system clock instead of FreeRTOS NTP
boolean UpdateLocalTime() {
  time_t now = time(nullptr);
  struct tm t;
#ifdef _WIN32
  localtime_s(&t, &now);
#else
  localtime_r(&now, &t);
#endif
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

// edp_update — no-op on PC (we read the framebuffer directly)
void edp_update() {}

// ---- Include the rendering code from main.cpp ----
// PC_SIMULATOR_BUILD gates out: includes, globals, BeginSleep, SetupTime,
// StartWiFi, StopWiFi, InitialiseSystem, loop, setup, DisplaySetupScreen.
// Everything else (Convert_Readings_to_Imperial, DecodeWeather, ConvertUnixTime,
// all DisplayWeather sub-functions, drawing wrappers) compiles unchanged.
#include "../src/main.cpp"

// DrawBattery — skip ADC, draw a "USB" indicator instead.
// Must come after #include "../src/main.cpp" so drawRect/fillRect/drawString are defined.
void DrawBattery(int x, int y) {
  drawRect(x + 25, y - 14, 40, 15, (uint16_t)0x00);
  fillRect(x + 65, y - 10, 4, 7, (uint16_t)0x00);
  fillRect(x + 27, y - 12, 36, 11, (uint16_t)0x00);
  drawString(x + 85, y - 14, String("USB"), LEFT);
}

// ---- OWM data loading ----
// PC replacement for DecodeWeather() from main.cpp (which is guarded out).
// Uses std::string directly — ArduinoJson natively supports it.
bool DecodeWeather(const std::string& json, String Type) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    fprintf(stderr, "deserializeJson() failed: %s\n", error.c_str());
    return false;
  }
  JsonObject root = doc.as<JsonObject>();
  fprintf(stderr, "Decoding %s data\n", Type.c_str());

  WxConditions[0].High = -50;
  WxConditions[0].Low = 50;

  JsonObject current = doc["current"];
  WxConditions[0].Sunrise = current["sunrise"];
  WxConditions[0].Sunset = current["sunset"];
  WxConditions[0].Temperature = current["temp"];
  WxConditions[0].FeelsLike = current["feels_like"];
  WxConditions[0].Pressure = current["pressure"];
  WxConditions[0].Humidity = current["humidity"];
  WxConditions[0].DewPoint = current["dew_point"];
  WxConditions[0].UVI = current["uvi"];
  WxConditions[0].Cloudcover = current["clouds"];
  WxConditions[0].Visibility = current["visibility"];
  WxConditions[0].Windspeed = current["wind_speed"];
  WxConditions[0].Winddir = current["wind_deg"];

  const char* desc = current["weather"][0]["description"];
  const char* icon = current["weather"][0]["icon"];
  WxConditions[0].Forecast0 = String(desc ? desc : "");
  WxConditions[0].Icon = String(icon ? icon : "");

  JsonArray daily = root["daily"];
  WxConditions[0].Low = daily[0]["temp"]["min"].as<float>();
  WxConditions[0].High = daily[0]["temp"]["max"].as<float>();

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
    const char* ic = list[r]["weather"][0]["icon"];
    WxForecast[wxIndex].Icon = String(ic ? ic : "");
    WxForecast[wxIndex].Rainfall = list[r]["rain"]["1h"].as<float>();
    WxForecast[wxIndex].Snowfall = list[r]["snow"]["1h"].as<float>();

    if (wxIndex >= 2) {
      float pt = WxForecast[0].Pressure - WxForecast[2].Pressure;
      pt = ((int)(pt * 10)) / 10.0f;
      WxConditions[0].Trend = (pt > 0) ? "+" : (pt < 0) ? "-" : "0";
    } else {
      WxConditions[0].Trend = "0";
    }
    wxIndex++;
  }
  if (strcmp(cfg.units, "I") == 0) Convert_Readings_to_Imperial(wxIndex);
  return true;
}

bool loadWeatherFromJson(const std::string& json) {
  return DecodeWeather(json, "onecall");
}

// ---- Entry point ----

int main() {
  // Config from environment variables
  auto env = [](const char* var, char* dest, size_t sz) {
    if (const char* v = getenv(var)) strncpy(dest, v, sz - 1);
  };
  env("OWM_APIKEY", cfg.apikey, sizeof(cfg.apikey));
  env("OWM_LAT", cfg.latitude, sizeof(cfg.latitude));
  env("OWM_LON", cfg.longitude, sizeof(cfg.longitude));
  env("OWM_CITY", cfg.city, sizeof(cfg.city));
  env("OWM_UNITS", cfg.units, sizeof(cfg.units));
  env("OWM_LANG", cfg.language, sizeof(cfg.language));
  env("OWM_HEMISPHERE", cfg.hemisphere, sizeof(cfg.hemisphere));

  if (!isConfigValid()) {
    fprintf(stderr,
            "ERROR: Missing OWM credentials.\n"
            "Set environment variables: OWM_APIKEY, OWM_LAT, OWM_LON\n"
            "Optionally: OWM_CITY, OWM_UNITS (M/I), OWM_LANG, OWM_HEMISPHERE (north/south)\n"
            "Or create include/owm_credentials.h and rebuild.\n");
    return 1;
  }

  UpdateLocalTime();

  hl = epd_hl_init(nullptr);
  framebuffer = epd_pc_get_framebuffer();
  epd_hl_set_all_white(&hl);

  std::string json = owm_fetch(cfg.server, cfg.apikey, cfg.latitude, cfg.longitude, cfg.units, cfg.language);
  if (json.empty()) {
    fprintf(stderr, "ERROR: OWM fetch failed\n");
    return 1;
  }

  if (!loadWeatherFromJson(json)) {
    fprintf(stderr, "ERROR: OWM JSON decode failed\n");
    return 1;
  }

  DisplayWeather();

#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  size_t fb_size = (size_t)(epd_width() / 2) * epd_height();
  fwrite(framebuffer, 1, fb_size, stdout);
  fflush(stdout);
  return 0;
}
