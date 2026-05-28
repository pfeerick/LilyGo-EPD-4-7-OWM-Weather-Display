#ifndef SIMULATOR_BUILD
#include <Arduino.h>            // In-built
#include <esp_task_wdt.h>       // In-built
#include "freertos/FreeRTOS.h"  // In-built
#include "freertos/task.h"      // In-built
#include "epdiy.h"              // https://github.com/vroland/epdiy
#include "esp_adc_cal.h"        // In-built
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built
#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include <time.h>               // In-built

#include <LittleFS.h>
#include "qrcode.h"
#include "config.h"
#include "defaults.h"
#include "setup_portal.h"
#include "forecast_record.h"
#include "translations/lang_en.h"
#else
// Simulator build: includes provided by simulator/main_wasm.cpp before including this file
#endif

enum alignment { LEFT, RIGHT, CENTER };
constexpr uint8_t White = 0xFF;
constexpr uint8_t LightGrey = 0xBB;
constexpr uint8_t Grey = 0x88;
constexpr uint8_t DarkGrey = 0x44;
constexpr uint8_t Black = 0x00;

constexpr bool autoscale_on = true;
constexpr bool autoscale_off = false;
constexpr bool barchart_on = true;
constexpr bool barchart_off = false;

constexpr uint8_t kGraphYDivisions = 5;   // Number of y-axis division markers
constexpr uint8_t kGraphDashes = 20;      // Dashes per horizontal grid line
constexpr uint8_t kGraphDaySections = 2;  // Day-boundary vertical lines

struct GraphConfig {
  int x, y, w, h;
  float yMin, yMax;
  const char* title;
  bool autoscale;
  bool barchart;
  int readings;
};

#ifndef SIMULATOR_BUILD
bool LargeIcon = true;
bool SmallIcon = false;
constexpr uint8_t Large = 20;  // For icon drawing
constexpr uint8_t Small = 10;  // For icon drawing
String Time_str = "--:--:--";
String Date_str = "-- --- ----";
int wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, vref = kDefaultVref;
//################ PROGRAM VARIABLES and OBJECTS ##########################################
constexpr uint8_t max_readings = 24;  // Limited to 3-days here, but could go to 5-days = 40 as the data is issued
constexpr uint8_t max_graph_readings = 16;

Forecast_record_type WxConditions;
Forecast_record_type WxForecast[max_readings];

float pressure_readings[max_readings] = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings] = {0};
float rain_readings[max_readings] = {0};
float snow_readings[max_readings] = {0};

long SleepDuration;
int WakeupHour;
int SleepHour;
long StartTime = 0;
long SleepTimer = 0;
long Delta =
    30;  // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power

//fonts
#include "fonts/opensans8b.h"
#include "fonts/opensans10b.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"
#include "fonts/opensans24b.h"

//image "fonts"
#include "images/moon.h"
#include "images/sunrise.h"
#include "images/sunset.h"
#include "images/uvi.h"

EpdFont currentFont;
uint8_t* framebuffer;
static EpdiyHighlevelState hl;
#endif  // SIMULATOR_BUILD

#pragma region Function Prototypes
void BeginSleep();
bool SetupTime();
uint8_t StartWiFi();
void StopWiFi();
void InitialiseSystem();
void Convert_Readings_to_Imperial(int count);
bool DecodeWeather(WiFiClient& json, const String& Type);
String ConvertUnixTime(int unix_time);
bool obtainWeatherData(WiFiClient& client, const String& RequestType);
float mm_to_inches(float value_mm);
float hPa_to_inHg(float value_hPa);
int JulianDate(int d, int m, int y);
float SumOfPrecip(float DataArray[], int readings);
String TitleCase(const String& text);
void DisplayWeather();
void DisplayGeneralInfoSection();
void DisplayWeatherIcon(int x, int y);
void DisplayMainWeatherSection(int x, int y);
void DisplayWindSection(int x, int y, float angle, float windspeed, int Cradius);
String WindDegToOrdinalDirection(float winddirection);
void DisplayTempHumiPressSection(int x, int y);
void DisplayForecastTextSection(int x, int y);
void DisplayVisiCCoverUVISection(int x, int y);
void Display_UVIndexLevel(int x, int y, float UVI);
void DisplayForecastWeather(int x, int y, int index, int fwidth);
void DisplayAstronomySection(int x, int y);
void DrawMoon(int x, int y, int diameter, int dd, int mm, int yy, bool southernHemisphere);
String MoonPhase(int d, int m, int y);
void DisplayForecastSection(int x, int y);
void DisplayGraphSection(int x, int y);
void DisplayConditionsSection(int x, int y, const char* IconName, bool IconSize);
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength);
void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14);
void DrawPressureAndTrend(int x, int y, float pressure, char slope);
void DisplayStatusSection(int x, int y, int rssi);
void DrawRSSI(int x, int y, int rssi);
bool UpdateLocalTime();
void DrawBattery(int x, int y);
void addcloud(int x, int y, int scale, int linesize);
void addrain(int x, int y, int scale, bool IconSize);
void addsnow(int x, int y, int scale, bool IconSize);
void addtstorm(int x, int y, int scale);
void addsun(int x, int y, int scale, bool IconSize);
void addfog(int x, int y, int scale, int linesize, bool IconSize);
void DrawAngledLine(int x, int y, int x1, int y1, int size, int color);
void ClearSky(int x, int y, bool IconSize, const char* IconName);
void BrokenClouds(int x, int y, bool IconSize, const char* IconName);
void FewClouds(int x, int y, bool IconSize, const char* IconName);
void ScatteredClouds(int x, int y, bool IconSize, const char* IconName);
void Rain(int x, int y, bool IconSize, const char* IconName);
void ChanceRain(int x, int y, bool IconSize, const char* IconName);
void Thunderstorms(int x, int y, bool IconSize, const char* IconName);
void Snow(int x, int y, bool IconSize, const char* IconName);
void Mist(int x, int y, bool IconSize, const char* IconName);
void CloudCover(int x, int y, int CloudCover);
void Visibility(int x, int y, const String& Visibility);
void addmoon(int x, int y, bool IconSize);
void Nodata(int x, int y, bool IconSize, const char* IconName);
void DrawMoonImage(int x, int y);
void DrawSunriseImage(int x, int y);
void DrawSunsetImage(int x, int y);
void DrawUVI(int x, int y);
void DrawGraph(GraphConfig gcfg, float DataArray[]);
void drawString(int x, int y, String text, alignment align);
void fillCircle(int x, int y, int r, uint8_t color);
void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color);
void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void drawCircle(int x0, int y0, int r, uint8_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void drawPixel(int x, int y, uint8_t color);
void setFont(EpdFont const& font);
void edp_update();
#pragma endregion

#ifndef SIMULATOR_BUILD
void BeginSleep() {
  epd_poweroff();
  UpdateLocalTime();
  SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) +
               Delta;  //Some ESP32 have a RTC that is too fast to maintain accurate time, so add an offset
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL);  // in Secs, 1000000LL converts to Secs as unit = 1uSec
  Serial.printf("Awake for : %.3f-secs\n", (millis() - StartTime) / 1000.0);
  Serial.printf("Entering %ld (secs) of sleep time\n", SleepTimer);
  Serial.printf("Starting deep-sleep period...\n");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

bool SetupTime() {
  configTime(cfg.gmtOffset_sec, cfg.daylightOffset_sec, cfg.ntpServer, kDefaultNtpFallback);
  setenv("TZ", cfg.timezone, 1);
  tzset();  // Set the TZ environment variable
  delay(100);
  return UpdateLocalTime();
}

uint8_t StartWiFi() {
  Serial.printf("\r\nConnecting to: %s\n", cfg.ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);  // switch off AP
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
    wifi_signal = WiFi.RSSI();  // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
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

void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.printf("%s\nStarting...\n", __FILE__);
  epd_init(&epd_board_lilygo_t5_47, &ED047TC2, EPD_LUT_64K);
  epd_set_vcom(1560);
  hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
  framebuffer = epd_hl_get_framebuffer(&hl);
  epd_hl_set_all_white(&hl);
}

void loop() {
  // Nothing to do here
}

void DisplaySetupScreen(const char* apName);

void setup() {
  InitialiseSystem();

  // Check for button-held config mode before loading config
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

  // Populate schedule globals from config
  SleepDuration = cfg.sleepDuration;
  WakeupHour = cfg.wakeupHour;
  SleepHour = cfg.sleepHour;

  if (forceConfig) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char apName[24];
    snprintf(apName, sizeof(apName), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(apName);
    enterSetupMode();  // never returns
  }

  if (StartWiFi() != WL_CONNECTED) {
    Serial.println("WiFi failed — entering setup mode");
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char apName[24];
    snprintf(apName, sizeof(apName), "WeatherSetup-%02X%02X", mac[4], mac[5]);
    DisplaySetupScreen(apName);
    enterSetupMode();  // never returns
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

static int _ox, _oy, _ms, _renderedPx;

// Returns the rendered pixel size (modules * moduleSize) of the QR code
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

  int cx = epd_width() / 2;  // 480
  int w = epd_width();       // 960
  int ms = 6;
  int pad = 16;

  // WiFi QR (ECC Low): WIFI:S:WeatherSetup-XXXX;T:nopass;; → version 4 = 33 modules = 198px
  // Portal QR (ECC High): http://192.168.4.1 → forced to version 4 = 33 modules = 198px
  int qrPx = 33 * ms;              // 198px — both QRs render at this size
  int qrCxL = pad + qrPx / 2;      // left QR centre x
  int qrCxR = w - pad - qrPx / 2;  // right QR centre x

  // Top-left: WiFi join QR — label positioned from actual rendered size
  char wifiQR[64];
  snprintf(wifiQR, sizeof(wifiQR), "WIFI:S:%s;T:nopass;;", apName);
  int wifiQRpx = drawQR(wifiQR, pad, pad, ms, ESP_QRCODE_ECC_LOW);

  // Top-right: portal QR — ECC High forces a larger version to better match WiFi QR size
  int rightQRx = w - pad - qrPx + (ms * 2);
  int portalQRpx = drawQR("http://192.168.4.1", rightQRx, pad, ms, ESP_QRCODE_ECC_HIGH);

  // Two-line labels under each QR, each positioned from its own actual rendered height
  setFont(OpenSans12B);
  int labelGap = 24;
  int wifiGap = 34;
  int portalGap = 28;
  drawString(qrCxL, pad + wifiQRpx + labelGap, "Scan to join", CENTER);
  drawString(qrCxL, pad + wifiQRpx + labelGap + wifiGap, "WiFi network", CENTER);
  drawString(qrCxR, pad + portalQRpx + labelGap, "Scan to open", CENTER);
  drawString(qrCxR, pad + portalQRpx + labelGap + portalGap, "config portal", CENTER);

  // Middle text block — sits between the QRs, starts near the top
  // Vertically centre the block in the display height
  // Block height: heading(~22) + gap(20) + 6 lines × ~38px = ~270px
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

#endif  // SIMULATOR_BUILD

void Convert_Readings_to_Imperial(int count) {
  WxConditions.Pressure = hPa_to_inHg(WxConditions.Pressure);
  for (int i = 0; i < count; i++) {
    WxForecast[i].Rainfall = mm_to_inches(WxForecast[i].Rainfall);
    WxForecast[i].Snowfall = mm_to_inches(WxForecast[i].Snowfall);
  }
}

#ifndef SIMULATOR_BUILD
bool DecodeWeather(WiFiClient& json, const String& Type) {
  Serial.printf("\nCreating object...");
  JsonDocument doc;                                         // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json);  // Deserialize the JSON document
  if (error) {                                              // Test if parsing succeeds.
    Serial.printf("deserializeJson() failed: %s\n", error.c_str());
    return false;
  }
  Serial.printf(" Decoding %s data\n", Type.c_str());
  WxConditions.High = -50;  // Sentinel: replaced by daily[0] max below
  WxConditions.Low = 50;    // Sentinel: replaced by daily[0] min below
  JsonObject current = doc["current"];
  WxConditions.Sunrise = current["sunrise"];
  Serial.printf("SRis: %d\n", WxConditions.Sunrise);
  WxConditions.Sunset = current["sunset"];
  Serial.printf("SSet: %d\n", WxConditions.Sunset);
  WxConditions.Temperature = current["temp"];
  Serial.printf("Temp: %f\n", WxConditions.Temperature);
  WxConditions.FeelsLike = current["feels_like"];
  Serial.printf("FLik: %f\n", WxConditions.FeelsLike);
  WxConditions.Pressure = current["pressure"];
  Serial.printf("Pres: %f\n", WxConditions.Pressure);
  WxConditions.Humidity = current["humidity"];
  Serial.printf("Humi: %f\n", WxConditions.Humidity);
  WxConditions.DewPoint = current["dew_point"];
  Serial.printf("DPoi: %f\n", WxConditions.DewPoint);
  WxConditions.UVI = current["uvi"];
  Serial.printf("UVin: %f\n", WxConditions.UVI);
  WxConditions.Cloudcover = current["clouds"];
  Serial.printf("CCov: %d\n", WxConditions.Cloudcover);
  WxConditions.Visibility = current["visibility"];
  Serial.printf("Visi: %d\n", WxConditions.Visibility);
  WxConditions.Windspeed = current["wind_speed"];
  Serial.printf("WSpd: %f\n", WxConditions.Windspeed);
  WxConditions.Winddir = current["wind_deg"];
  Serial.printf("WDir: %d\n", WxConditions.Winddir);
  JsonObject current_weather = current["weather"][0];
  strlcpy(WxConditions.Forecast0, current_weather["description"] | "", sizeof(WxConditions.Forecast0));
  Serial.printf("Fore: %s\n", WxConditions.Forecast0);
  strlcpy(WxConditions.Icon, current_weather["icon"] | "", sizeof(WxConditions.Icon));
  Serial.printf("Icon: %s\n", WxConditions.Icon);

  Serial.printf("\nReceiving Forecast period - ");  //------------------------------------------------

  // Daily
  JsonArray daily = doc["daily"];
  WxConditions.Low = daily[0]["temp"]["min"].as<float>();  // Get Lowest temperature for next 24Hrs
  Serial.printf("TLow: %f\n", WxConditions.Low);
  WxConditions.High = daily[0]["temp"]["max"].as<float>();  // Get Highest temperature for next 24Hrs
  Serial.printf("High: %f\n", WxConditions.High);

  //TODO: daily[1..7] has 7 more days of temp/icon/description data — add a weekly forecast row if screen space allows

  JsonArray list = doc["hourly"];
  byte wxIndex = 0;                                      // Index to populate WxForecast sequentially
  Serial.printf("hourly list size: %u\n", list.size());  // 48 hours of hourly data is returned by the API
  for (byte r = 0; r < 48 && wxIndex < 16; r += 3) {
    Serial.printf("\nPeriod-%u--------------\n", r);

    WxForecast[wxIndex].Dt = list[r]["dt"].as<int>();
    WxForecast[wxIndex].Temperature = list[r]["temp"].as<float>();
    Serial.printf("Temp: %f\n", WxForecast[wxIndex].Temperature);
    float t1 = (r + 1 < (int)list.size()) ? list[r + 1]["temp"].as<float>() : WxForecast[wxIndex].Temperature;
    float t2 = (r + 2 < (int)list.size()) ? list[r + 2]["temp"].as<float>() : WxForecast[wxIndex].Temperature;
    WxForecast[wxIndex].High = max(max(WxForecast[wxIndex].Temperature, t1), t2);
    Serial.printf("High: %f\n", WxForecast[wxIndex].High);
    WxForecast[wxIndex].Low = min(min(WxForecast[wxIndex].Temperature, t1), t2);
    Serial.printf("Low: %f\n", WxForecast[wxIndex].Low);
    WxForecast[wxIndex].Pressure = list[r]["pressure"].as<float>();
    Serial.printf("Pres: %f\n", WxForecast[wxIndex].Pressure);
    WxForecast[wxIndex].Humidity = list[r]["humidity"].as<float>();
    Serial.printf("Humi: %f\n", WxForecast[wxIndex].Humidity);
    strlcpy(WxForecast[wxIndex].Icon, list[r]["weather"][0]["icon"] | "", sizeof(WxForecast[wxIndex].Icon));
    Serial.printf("Icon: %s\n", WxForecast[wxIndex].Icon);
    WxForecast[wxIndex].Rainfall = list[r]["rain"]["1h"].as<float>();
    Serial.printf("Rain: %f\n", WxForecast[wxIndex].Rainfall);
    WxForecast[wxIndex].Snowfall = list[r]["snow"]["1h"].as<float>();
    Serial.printf("Snow: %f\n", WxForecast[wxIndex].Snowfall);

    wxIndex++;  // Increment WxForecast index for sequential population
  }
  if (wxIndex >= 3) {
    float pressure_trend =
        WxForecast[0].Pressure - WxForecast[2].Pressure;   // Measure pressure slope between ~now and later
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0;  // Remove any small variations less than 0.1
    WxConditions.Trend = '=';
    if (pressure_trend > 0) WxConditions.Trend = '+';
    if (pressure_trend < 0) WxConditions.Trend = '-';
    if (pressure_trend == 0) WxConditions.Trend = '0';
  } else {
    WxConditions.Trend = '0';  // Default if insufficient data
  }
  if (strcmp(cfg.units, "I") == 0) Convert_Readings_to_Imperial(wxIndex);
  return true;
}
#endif  // SIMULATOR_BUILD

//#########################################################################################
String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  time_t tm = unix_time + cfg.gmtOffset_sec + cfg.daylightOffset_sec;
  struct tm* now_tm = gmtime(&tm);
  char output[40];
  if (isMetric) {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  } else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}
//#########################################################################################
#ifndef SIMULATOR_BUILD
bool obtainWeatherData(WiFiClient& client, const String& RequestType) {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  const String units = (isMetric ? "metric" : "imperial");
  client.stop();  // close connection before sending a new request
  HTTPClient http;
  //api.openweathermap.org/data/3.0/onecall?lat={lat}&lon={lon}&appid={API key}
  String uri = "/data/3.0/" + RequestType + "?lat=" + cfg.latitude + "&lon=" + cfg.longitude + "&appid=" + cfg.apikey +
               "&mode=json&units=" + units + "&lang=" + cfg.language;
  if (RequestType == "onecall") uri += "&exclude=minutely,alerts";
  http.begin(client, cfg.server, 80, uri);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
  } else {
    Serial.printf("connection failed, error: %s\n", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}
#endif  // SIMULATOR_BUILD

float mm_to_inches(float value_mm) {
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa) {
  return 0.02953 * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3;  // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i < readings; i++)
    sum += DataArray[i];
  return sum;
}

String TitleCase(const String& text) {
  if (text.length() > 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1);  // Title-case the string
  } else
    return text;
}

void DisplayWeather() {  // 4.7" e-paper display is 960x540 resolution
  DisplayStatusSection(kLayoutStatusX, kLayoutStatusY, wifi_signal);
  DisplayGeneralInfoSection();
  DisplayWindSection(kLayoutWindX, kLayoutWindY, WxConditions.Winddir, WxConditions.Windspeed, kLayoutWindRadius);
  DisplayAstronomySection(kLayoutAstronomyX, kLayoutAstronomyY);
  DisplayMainWeatherSection(kLayoutMainWeatherX, kLayoutMainWeatherY);
  DisplayWeatherIcon(kLayoutWeatherIconX, kLayoutWeatherIconY);
  DisplayForecastSection(kLayoutForecastX, kLayoutForecastY);
  DisplayGraphSection(kLayoutGraphX, kLayoutGraphY);
}

void DisplayGeneralInfoSection() {
  setFont(OpenSans10B);
  drawString(5, 2, String(cfg.city), LEFT);
  setFont(OpenSans8B);
  drawString(500, 2, Date_str + "  @   " + Time_str, LEFT);
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions.Icon, LargeIcon);
}

void DisplayMainWeatherSection(int x, int y) {
  setFont(OpenSans8B);
  DisplayTempHumiPressSection(x, y - 60);
  DisplayForecastTextSection(x - 55, y + 45);
  DisplayVisiCCoverUVISection(x - 10, y + 95);
}

void DisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, Cradius - 22, angle, 18, 33);  // Show wind direction on outer circle of width and length
  setFont(OpenSans8B);
  int dxo, dyo, dxi, dyi;
  drawCircle(x, y, Cradius, Black);        // Draw compass circle
  drawCircle(x, y, Cradius + 1, Black);    // Draw compass circle
  drawCircle(x, y, Cradius * 0.7, Black);  // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45) drawString(dxo + x + 15, dyo + y - 18, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 20, dyo + y - 2, TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x - 20, dyo + y - 2, TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 18, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
  }
  drawString(x, y - Cradius - 20, TXT_N, CENTER);
  drawString(x, y + Cradius + 10, TXT_S, CENTER);
  drawString(x - Cradius - 15, y - 5, TXT_W, CENTER);
  drawString(x + Cradius + 10, y - 5, TXT_E, CENTER);
  drawString(x + 3, y + 50, String(angle, 0) + "°", CENTER);
  setFont(OpenSans12B);
  drawString(x, y - 50, WindDegToOrdinalDirection(angle), CENTER);
  setFont(OpenSans24B);
  drawString(x + 3, y - 18, String(windspeed, 1), CENTER);
  setFont(OpenSans12B);
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  drawString(x, y + 25, (isMetric ? "m/s" : "mph"), CENTER);
}

String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25) return TXT_N;
  if (winddirection >= 11.25 && winddirection < 33.75) return TXT_NNE;
  if (winddirection >= 33.75 && winddirection < 56.25) return TXT_NE;
  if (winddirection >= 56.25 && winddirection < 78.75) return TXT_ENE;
  if (winddirection >= 78.75 && winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
  return "?";
}

void DisplayTempHumiPressSection(int x, int y) {
  setFont(OpenSans18B);
  drawString(x - 30, y, String(WxConditions.Temperature, 1) + "°   " + String(WxConditions.Humidity, 0) + "%", LEFT);
  setFont(OpenSans12B);
  DrawPressureAndTrend(x + 195, y + 15, WxConditions.Pressure, WxConditions.Trend);
  int Yoffset = 42;
  if (WxConditions.Windspeed > 0) {
    drawString(x - 30, y + Yoffset, String(WxConditions.FeelsLike, 1) + "° FL",
               LEFT);  // Show FeelsLike temperature if windspeed > 0
    Yoffset += 30;
  }
  drawString(x - 30, y + Yoffset, String(WxConditions.High, 0) + "° | " + String(WxConditions.Low, 0) + "° Hi/Lo",
             LEFT);  // Show forecast high and Low
}

void DisplayForecastTextSection(int x, int y) {
  constexpr uint8_t lineWidth = 34;
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  setFont(OpenSans12B);
  String Wx_Description = WxConditions.Forecast0;
  Wx_Description.replace(".", "");  // remove any '.'
  int spaceRemaining = 0, p = 0, charCount = 0;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > lineWidth - 1) {  // '~' is the end of line marker
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (WxForecast[0].Rainfall > 0)
    Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((isMetric ? "mm" : "in")) + ")";
  int sep = Wx_Description.indexOf("~");
  String Line1 = (sep >= 0) ? Wx_Description.substring(0, sep) : Wx_Description;
  String Line2 = (sep >= 0) ? Wx_Description.substring(sep + 1) : "";
  drawString(x + 30, y + 5, TitleCase(Line1), LEFT);
  if (Line2.length() > 0) drawString(x + 30, y + 30, Line2, LEFT);
}

void DisplayVisiCCoverUVISection(int x, int y) {
  setFont(OpenSans12B);
  Visibility(x + 5, y, String(WxConditions.Visibility) + "M");
  CloudCover(x + 155, y, WxConditions.Cloudcover);
  Display_UVIndexLevel(x + 265, y, WxConditions.UVI);
}

void Display_UVIndexLevel(int x, int y, float UVI) {
  const char* Level = "";
  if (UVI <= 2) Level = " (L)";
  if (UVI >= 3 && UVI <= 5) Level = " (M)";
  if (UVI >= 6 && UVI <= 7) Level = " (H)";
  if (UVI >= 8 && UVI <= 10) Level = " (VH)";
  if (UVI >= 11) Level = " (EX)";
  drawString(x + 20, y - 5, String(UVI, (UVI < 0 ? 1 : 0)) + Level, LEFT);
  DrawUVI(x - 10, y - 5);
}

void DisplayForecastWeather(int x, int y, int index, int fwidth) {
  x = x + fwidth * index;
  DisplayConditionsSection(x + fwidth / 2 - 5, y + 85, WxForecast[index].Icon, SmallIcon);
  setFont(OpenSans10B);
  drawString(x + fwidth / 2, y + 30, String(ConvertUnixTime(WxForecast[index].Dt).substring(0, 5)), CENTER);
  drawString(x + fwidth / 2, y + 130, String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°",
             CENTER);
}


static inline bool isSouthernHemisphere() {
  return atof(cfg.latitude) < 0.0;
}

void DisplayAstronomySection(int x, int y) {
  setFont(OpenSans10B);
  time_t now = time(NULL);
  struct tm* now_utc = gmtime(&now);
  drawString(x + 5, y + 102, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900), LEFT);
  DrawMoonImage(x + 10, y + 23);  // Different references!
  DrawMoon(x - 28, y - 15, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900,
           isSouthernHemisphere());  // Spaced at 1/2 moon size, so 10 - 75/2 = -28
  drawString(x + 115, y + 40, ConvertUnixTime(WxConditions.Sunrise).substring(0, 5), LEFT);  // Sunrise
  drawString(x + 115, y + 80, ConvertUnixTime(WxConditions.Sunset).substring(0, 5), LEFT);   // Sunset
  DrawSunriseImage(x + 180, y + 20);
  DrawSunsetImage(x + 180, y + 60);
}

void DrawMoon(int x, int y, int diameter, int dd, int mm, int yy, bool southernHemisphere) {
  int c, e;
  double jd;
  if (mm < 3) {
    yy--;
    mm += 12;
  }
  mm++;
  c = 365.25 * yy;
  e = 30.6 * mm;
  jd = c + e + dd - 694039.09;
  jd /= 29.53059;
  int b = jd;
  jd -= b;  // fractional part 0.0–1.0
  double Phase = jd;
  b = (int)(Phase * 8 + 0.5) & 7;
  if (southernHemisphere) Phase = 1 - Phase;
  int octant = (int)(Phase * 8 + 0.5) & 7;
  // Draw dark part of moon
  fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, DarkGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (octant < 5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    } else {
      Xpos1 = Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines) / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines) / number_of_lines * diameter + y;
    drawLine(pW1x, pW1y, pW2x, pW2y, White);
    drawLine(pW3x, pW3y, pW4x, pW4y, White);
  }
  drawCircle(x + diameter - 1, y + diameter, diameter / 2, Black);
}

String MoonPhase(int d, int m, int y) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c = 365.25 * y;
  e = 30.6 * m;
  jd = c + e + d - 694039.09;                   /* jd is total days elapsed */
  jd /= 29.53059;                               /* divide by the moon cycle (29.53 days) */
  b = jd;                                       /* int(jd) -> b, take integer part of jd */
  jd -= b;                                      /* subtract integer part to leave fractional part of original jd */
  b = jd * 8 + 0.5;                             /* scale fraction from 0-8 and round by adding 0.5 */
  b = b & 7;                                    /* 0 and 8 are the same phase so modulo 8 for 0 */
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}

void DisplayForecastSection(int x, int y) {
  int f = 0;
  do {
    DisplayForecastWeather(x, y, f, 82);  // x,y cordinates, forecatsr number, spacing width
    f++;
  } while (f < 8);
}

void DisplayGraphSection(int x, int y) {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  int r = 0;
  do {  // Pre-load temporary arrays with data — values already in display units after DecodeWeather
    pressure_readings[r] = WxForecast[r].Pressure;
    rain_readings[r] = WxForecast[r].Rainfall;
    snow_readings[r] = WxForecast[r].Snowfall;
    temperature_readings[r] = WxForecast[r].Temperature;
    humidity_readings[r] = WxForecast[r].Humidity;
    r++;
  } while (r < max_graph_readings);
  int gwidth = 175, gheight = 100;
  int gx = (epd_width() - gwidth * 4) / 5 + 8;
  int gy = (epd_height() - gheight - 30);
  int gap = gwidth + gx;
  DrawGraph(
      {gx + 0 * gap, gy, gwidth, gheight, 900, 1050, isMetric ? TXT_PRESSURE_HPA.c_str() : TXT_PRESSURE_IN.c_str(),
       autoscale_on, barchart_off, max_graph_readings},
      pressure_readings);
  DrawGraph(
      {gx + 1 * gap, gy, gwidth, gheight, 10, 30, isMetric ? TXT_TEMPERATURE_C.c_str() : TXT_TEMPERATURE_F.c_str(),
       autoscale_on, barchart_off, max_graph_readings},
      temperature_readings);
  DrawGraph({gx + 2 * gap, gy, gwidth, gheight, 0, 100, TXT_HUMIDITY_PERCENT.c_str(), autoscale_off, barchart_off,
             max_graph_readings},
            humidity_readings);
  if (SumOfPrecip(rain_readings, max_graph_readings) >= SumOfPrecip(snow_readings, max_graph_readings))
    DrawGraph(
        {gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, isMetric ? TXT_RAINFALL_MM.c_str() : TXT_RAINFALL_IN.c_str(),
         autoscale_on, barchart_on, max_graph_readings},
        rain_readings);
  else
    DrawGraph(
        {gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, isMetric ? TXT_SNOWFALL_MM.c_str() : TXT_SNOWFALL_IN.c_str(),
         autoscale_on, barchart_on, max_graph_readings},
        snow_readings);
}

void DisplayConditionsSection(int x, int y, const char* IconName, bool IconSize) {
  Serial.printf("Icon name: %s\n", IconName);
  if (strcmp(IconName, "01d") == 0 || strcmp(IconName, "01n") == 0)
    ClearSky(x, y, IconSize, IconName);
  else if (strcmp(IconName, "02d") == 0 || strcmp(IconName, "02n") == 0)
    FewClouds(x, y, IconSize, IconName);
  else if (strcmp(IconName, "03d") == 0 || strcmp(IconName, "03n") == 0)
    ScatteredClouds(x, y, IconSize, IconName);
  else if (strcmp(IconName, "04d") == 0 || strcmp(IconName, "04n") == 0)
    BrokenClouds(x, y, IconSize, IconName);
  else if (strcmp(IconName, "09d") == 0 || strcmp(IconName, "09n") == 0)
    ChanceRain(x, y, IconSize, IconName);
  else if (strcmp(IconName, "10d") == 0 || strcmp(IconName, "10n") == 0)
    Rain(x, y, IconSize, IconName);
  else if (strcmp(IconName, "11d") == 0 || strcmp(IconName, "11n") == 0)
    Thunderstorms(x, y, IconSize, IconName);
  else if (strcmp(IconName, "13d") == 0 || strcmp(IconName, "13n") == 0)
    Snow(x, y, IconSize, IconName);
  else if (strcmp(IconName, "50d") == 0 || strcmp(IconName, "50n") == 0)
    Mist(x, y, IconSize, IconName);
  else
    Nodata(x, y, IconSize, IconName);
}

void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x;  // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y;  // calculate Y position
  float x1 = 0;
  float y1 = plength;
  float x2 = pwidth / 2;
  float y2 = pwidth / 2;
  float x3 = -pwidth / 2;
  float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, Black);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  drawLine(x + o1, y + o2, x + o3, y + o4, Black);
  drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}

void DrawPressureAndTrend(int x, int y, float pressure, char slope) {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  drawString(x + 25, y - 10, String(pressure, isMetric ? 0 : 1) + (isMetric ? "hPa" : "in"), LEFT);
  if (slope == '+') {
    DrawSegment(x, y, 0, 0, 8, -8, 8, -8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, -8, 8, -8, 16, 0);
  } else if (slope == '0') {
    DrawSegment(x, y, 8, -8, 16, 0, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 8, -8, 16, 0, 8, 8, 16, 0);
  } else if (slope == '-') {
    DrawSegment(x, y, 0, 0, 8, 8, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, 8, 8, 8, 16, 0);
  }
}

void DisplayStatusSection(int x, int y, int rssi) {
  setFont(OpenSans8B);
  DrawRSSI(x + 305, y + 15, rssi);
  DrawBattery(x + 150, y);
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20) WIFIsignal = 30;  //            <-20dbm displays 5-bars
    if (_rssi <= -40) WIFIsignal = 24;  //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60) WIFIsignal = 18;  //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80) WIFIsignal = 12;  //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 6;  // -100dbm to  -81dbm displays 1-bar
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
}

#ifndef SIMULATOR_BUILD
bool UpdateLocalTime() {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  struct tm timeinfo;
  char time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) {  // Wait for 5-sec for time to synchronise
    Serial.printf("Failed to obtain time\n");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin = timeinfo.tm_min;
  CurrentSec = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");  // Displays: Saturday, June 24 2017 14:05:49
  if (isMetric) {
    snprintf(day_output, sizeof(day_output), "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday,
             month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '14:05:49'
    snprintf(time_output, sizeof(time_output), "%s", update_time);
  } else {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo);  // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);         // Creates: '@ 02:05:49pm'
    snprintf(time_output, sizeof(time_output), "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  return true;
}
#endif  // SIMULATOR_BUILD

#ifndef SIMULATOR_BUILD
void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, kDefaultVref, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  // kBatteryVoltageDiv is the resistor divider ratio on ADC pin kBatteryAdcPin
  float voltage = analogRead(kBatteryAdcPin) / (float)kBatteryAdcBits * kBatteryVoltageDiv * (vref / 1000.0);
  if (voltage > 1) {  // Only display if there is a valid reading
    Serial.printf("\nVoltage = %.2f\n", voltage);
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) -
                 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.20) percentage = 0;  // orig 3.5
    drawRect(x + 25, y - 14, 40, 15, Black);
    fillRect(x + 65, y - 10, 4, 7, Black);
    fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
    drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
  }
}
#endif  // SIMULATOR_BUILD — DrawBattery uses ADC; stub defined in simulator/main_wasm.cpp

// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  fillCircle(x - scale * 3, y, scale, Black);                                    // Left most circle
  fillCircle(x + scale * 3, y, scale, Black);                                    // Right most circle
  fillCircle(x - scale, y - scale, scale * 1.4, Black);                          // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, Black);             // Right middle upper circle
  fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, Black);       // Upper and lower lines
  fillCircle(x - scale * 3, y, scale - linesize, White);                         // Clear left most circle
  fillCircle(x + scale * 3, y, scale - linesize, White);                         // Clear right most circle
  fillCircle(x - scale, y - scale, scale * 1.4 - linesize, White);               // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, White);  // Right middle upper circle
  fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2,
           White);  // Upper and lower lines
}

void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 12, "///////", LEFT);
  } else {
    setFont(OpenSans18B);
    drawString(x - 60, y + 25, "///////", LEFT);
  }
}

void addsnow(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 15, "* * * *", LEFT);
  } else {
    setFont(OpenSans18B);
    drawString(x - 60, y + 30, "* * * *", LEFT);
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 1; i < 5; i++) {
    drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale,
             Black);
    drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale,
             Black);
    drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale,
             Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0,
             y + scale * 1.5 + 0, Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0,
             y + scale * 1.5 + 1, Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0,
             y + scale * 1.5 + 2, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0,
             y + scale * 1.5, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1,
             y + scale * 1.5, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2,
             y + scale * 1.5, Black);
  }
}

void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 5;
  fillRect(x - scale * 2, y, scale * 4, linesize, Black);
  fillRect(x, y - scale * 2, linesize, scale * 4, Black);
  DrawAngledLine(x + scale * 1.4, y + scale * 1.4, (x - scale * 1.4), (y - scale * 1.4), linesize * 1.5,
                 Black);  // Actually sqrt(2) but 1.4 is good enough
  DrawAngledLine(x - scale * 1.4, y + scale * 1.4, (x + scale * 1.4), (y - scale * 1.4), linesize * 1.5, Black);
  fillCircle(x, y, scale * 1.3, White);
  fillCircle(x, y, scale, Black);
  fillCircle(x, y, scale - linesize, White);
}

void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) linesize = 3;
  fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, Black);
  fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, Black);
  fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, Black);
}

void DrawAngledLine(int x, int y, int x1, int y1, int size, int color) {
  int dx = (size / 2.0) * (x - x1) / sqrt(sq(x - x1) + sq(y - y1));
  int dy = (size / 2.0) * (y - y1) / sqrt(sq(x - x1) + sq(y - y1));
  fillTriangle(x + dx, y - dy, x - dx, y + dy, x1 + dx, y1 - dy, color);
  fillTriangle(x - dx, y + dy, x1 - dx, y1 + dy, x1 + dx, y1 - dy, color);
}

void ClearSky(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += (IconSize ? 0 : 10);
  addsun(x, y, scale * (IconSize ? 1.7 : 1.2), IconSize);
}

void BrokenClouds(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void FewClouds(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x + (IconSize ? 10 : 0), y, scale * (IconSize ? 0.9 : 0.8), linesize);
  addsun((x + (IconSize ? 10 : 0)) - scale * 1.8, y - scale * 1.6, scale, IconSize);
}

void ScatteredClouds(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x - (IconSize ? 35 : 0), y - scale * 2, scale / 2, linesize);  // Cloud top left
  addcloud(x, y, scale * 0.9, linesize);                                  // Main cloud
}

void Rain(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addrain(x, y, scale, IconSize);
}

void ChanceRain(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 15;
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.65), linesize);
  addrain(x, y, scale, IconSize);
}

void Thunderstorms(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 5;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addtstorm(x, y, scale);
}

void Snow(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addsnow(x, y, scale, IconSize);
}

void Mist(int x, int y, bool IconSize, const char* IconName) {
  int scale = Small, linesize = 5;
  if (IconName[2] == 'n') addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addsun(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addfog(x, y, scale, linesize, IconSize);
}

void CloudCover(int x, int y, int CloudCover) {
  addcloud(x - 9, y, Small * 0.3, 2);      // Cloud top left
  addcloud(x + 3, y - 2, Small * 0.3, 2);  // Cloud top right
  addcloud(x, y + 15, Small * 0.6, 2);     // Main cloud
  drawString(x + 30, y, String(CloudCover) + "%", LEFT);
}

void Visibility(int x, int y, const String& Visibility) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 10;
  int r = 14;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y - r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i) + Offset, Black);
  }
  start_angle = 3.61;
  end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, Black);
  }
  fillCircle(x, y + Offset, r / 4, Black);
  drawString(x + 20, y, Visibility, LEFT);
}

void addmoon(int x, int y, bool IconSize) {
  int xOffset = 65;
  int yOffset = 12;
  if (IconSize == LargeIcon) {
    xOffset = 102;
    yOffset = -13;
  }
  fillCircle(x - 28 + xOffset, y - 37 + yOffset, Small, Black);
  fillCircle(x - 16 + xOffset, y - 37 + yOffset, (int)(Small * 1.6), White);
}

void Nodata(int x, int y, bool IconSize, const char* IconName) {
  if (IconSize == LargeIcon)
    setFont(OpenSans24B);
  else
    setFont(OpenSans12B);
  drawString(x - 3, y - 10, "?", CENTER);
}

void DrawMoonImage(int x, int y) {
  EpdRect area = {x, y, moon_width, moon_height};
  epd_copy_to_framebuffer(area, (uint8_t*)moon_data, framebuffer);
}

void DrawSunriseImage(int x, int y) {
  EpdRect area = {x, y, sunrise_width, sunrise_height};
  epd_copy_to_framebuffer(area, (uint8_t*)sunrise_data, framebuffer);
}

void DrawSunsetImage(int x, int y) {
  EpdRect area = {x, y, sunset_width, sunset_height};
  epd_copy_to_framebuffer(area, (uint8_t*)sunset_data, framebuffer);
}

void DrawUVI(int x, int y) {
  EpdRect area = {x, y, uvi_width, uvi_height};
  epd_copy_to_framebuffer(area, (uint8_t*)uvi_data, framebuffer);
}

/* (C) D L BIRD
    Draw a graph on the ePaper display from a float data array.
    x_pos, y_pos  - top-left corner of the graph in pixels
    gwidth, gheight - dimensions of the plot area in pixels
    Y1Min, Y1Max  - default Y-axis range (overridden when auto_scale is true)
    title         - label drawn above the graph
    DataArray     - float array of data points
    readings      - number of elements in DataArray to plot
    auto_scale    - true: adjust Y axis to fit the data; false: use Y1Min/Y1Max
    barchart_mode - true: draw filled bars; false: draw a line graph
*/
static void calculateGraphYScale(const float* data, int count, float& yMin, float& yMax) {
  yMax = -10000;
  yMin = 10000;
  for (int i = 1; i < count; i++) {
    if (data[i] >= yMax) yMax = data[i];
    if (data[i] <= yMin) yMin = data[i];
  }
  yMax = round(yMax + 0.5f);
  if (yMin != 0) yMin = round(yMin);
}

void DrawGraph(GraphConfig gcfg, float DataArray[]) {
  if (gcfg.autoscale) calculateGraphYScale(DataArray, gcfg.readings, gcfg.yMin, gcfg.yMax);
  setFont(OpenSans10B);
  int last_x = gcfg.x + 1;
  int last_y = gcfg.y + (gcfg.yMax - constrain(DataArray[0], gcfg.yMin, gcfg.yMax)) / (gcfg.yMax - gcfg.yMin) * gcfg.h;
  drawRect(gcfg.x, gcfg.y, gcfg.w + 3, gcfg.h + 2, Grey);
  drawString(gcfg.x - 20 + gcfg.w / 2, gcfg.y - 28, gcfg.title, CENTER);
  for (int i = 0; i < gcfg.readings; i++) {
    float x2 = gcfg.x + i * gcfg.w / (gcfg.readings - 1) - 1;
    float y2 =
        gcfg.y + (gcfg.yMax - constrain(DataArray[i], gcfg.yMin, gcfg.yMax)) / (gcfg.yMax - gcfg.yMin) * gcfg.h + 1;
    if (gcfg.barchart) {
      fillRect(last_x + 2, y2, (gcfg.w / gcfg.readings) - 1, gcfg.y + gcfg.h - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black);  // Two lines for hi-res display
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }
  // Draw the Y-axis scale
  for (int spacing = 0; spacing <= kGraphYDivisions; spacing++) {
    for (int j = 0; j < kGraphDashes; j++) {  // Draw dashed graph grid lines
      if (spacing < kGraphYDivisions)
        drawFastHLine((gcfg.x + 3 + j * gcfg.w / kGraphDashes), gcfg.y + (gcfg.h * spacing / kGraphYDivisions),
                      gcfg.w / (2 * kGraphDashes), Grey);
    }
    float axisVal = gcfg.yMax - (float)(gcfg.yMax - gcfg.yMin) / kGraphYDivisions * spacing + 0.01f;
    int labelX;
    int decimalPlaces;
    if (axisVal < 5 || strcmp(gcfg.title, TXT_PRESSURE_IN.c_str()) == 0) {
      labelX = gcfg.x - 10;
      decimalPlaces = 1;
    } else if (gcfg.yMin < 1 && gcfg.yMax < 10) {
      labelX = gcfg.x - 3;
      decimalPlaces = 1;
    } else {
      labelX = gcfg.x - 7;
      decimalPlaces = 0;
    }
    drawString(labelX, gcfg.y + gcfg.h * spacing / kGraphYDivisions - 5, String(axisVal, decimalPlaces), RIGHT);
  }
  for (int i = 0; i < kGraphDaySections; i++) {
    drawString(20 + gcfg.x + gcfg.w / kGraphDaySections * i, gcfg.y + gcfg.h + 10, String(i) + "d", LEFT);
    if (i < 2)
      drawFastVLine(gcfg.x + gcfg.w / kGraphDaySections * i + gcfg.w / kGraphDaySections, gcfg.y, gcfg.h, LightGrey);
  }
}

void drawString(int x, int y, String text, alignment align) {
  char* data = const_cast<char*>(text.c_str());
  int x1, y1;  //the bounds of x,y and w and h of the variable 'text' in pixels.
  int w, h;
  int xx = x, yy = y;
  EpdFontProperties props = epd_font_properties_default();
  epd_get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, &props);
  if (align == RIGHT) x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  epd_write_default(&currentFont, data, &x, &cursor_y, framebuffer);
}

void fillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_draw_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect({x, y, w, h}, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect({x, y, w, h}, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void setFont(EpdFont const& font) {
  currentFont = font;
}

#ifndef SIMULATOR_BUILD
void edp_update() {
  epd_hl_update_screen(&hl, MODE_GL16, (int)epd_ambient_temperature());
}
#endif
