#include "display.h"
#include "icons.h"
#include "translations/lang_en.h"

#include "defaults.h"

#ifndef SIMULATOR_BUILD
#include "esp_adc_cal.h"
#include "power.h"  // for start_time extern (set in InitialiseSystem)

bool large_icon = true;
bool small_icon = false;
String time_str = "--:--:--";
String date_str = "-- --- ----";
int wifi_signal, current_hour = 0, current_min = 0, current_sec = 0;
EpdFont current_font;
uint8_t* framebuffer;
EpdiyHighlevelState epd_hl;

#include "fonts/opensans8b.h"
#include "fonts/opensans10b.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"
#include "fonts/opensans24b.h"

#include "images/moon.h"
#include "images/sunrise.h"
#include "images/sunset.h"
#include "images/uvi.h"
#endif  // SIMULATOR_BUILD

void DisplayWeather() {
  DisplayStatusSection(kLayoutStatusX, kLayoutStatusY, wifi_signal);
  DisplayGeneralInfoSection();
  DisplayWindSection(kLayoutWindX, kLayoutWindY, wx_conditions.wind_dir, wx_conditions.wind_speed, kLayoutWindRadius);
  DisplayAstronomySection(kLayoutAstronomyX, kLayoutAstronomyY);
  DisplayMainWeatherSection(kLayoutMainWeatherX, kLayoutMainWeatherY);
  DisplayWeatherIcon(kLayoutWeatherIconX, kLayoutWeatherIconY);
  DisplayForecastSection(kLayoutForecastX, kLayoutForecastY);
  DisplayGraphSection(kLayoutGraphX, kLayoutGraphY);
}

void DisplayGeneralInfoSection() {
  SetFont(OpenSans10B);
  DrawString(5, 2, cfg.city.c_str(), Alignment::kLeft);
  SetFont(OpenSans8B);
  DrawString(500, 2, date_str + "  @   " + time_str, Alignment::kLeft);
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, wx_conditions.icon, large_icon);
}

void DisplayMainWeatherSection(int x, int y) {
  SetFont(OpenSans8B);
  DisplayTempHumiPressSection(x, y - 60);
  DisplayForecastTextSection(x - 55, y + 45);
  DisplayVisiCCoverUVISection(x - 10, y + 95);
}

void DisplayWindSection(int x, int y, float angle, float windspeed, int radius) {
  DrawArrow(x, y, radius - 22, angle, 18, 33);
  SetFont(OpenSans8B);
  int dxo, dyo, dxi, dyi;
  DrawCircle(x, y, radius, kBlack);
  DrawCircle(x, y, radius + 1, kBlack);
  DrawCircle(x, y, radius * 0.7, kBlack);
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = radius * cos((a - 90) * PI / 180);
    dyo = radius * sin((a - 90) * PI / 180);
    if (a == 45) DrawString(dxo + x + 15, dyo + y - 18, TXT_NE, Alignment::kCenter);
    if (a == 135) DrawString(dxo + x + 20, dyo + y - 2, TXT_SE, Alignment::kCenter);
    if (a == 225) DrawString(dxo + x - 20, dyo + y - 2, TXT_SW, Alignment::kCenter);
    if (a == 315) DrawString(dxo + x - 15, dyo + y - 18, TXT_NW, Alignment::kCenter);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    DrawLine(dxo + x, dyo + y, dxi + x, dyi + y, kBlack);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    DrawLine(dxo + x, dyo + y, dxi + x, dyi + y, kBlack);
  }
  DrawString(x, y - radius - 20, TXT_N, Alignment::kCenter);
  DrawString(x, y + radius + 10, TXT_S, Alignment::kCenter);
  DrawString(x - radius - 15, y - 5, TXT_W, Alignment::kCenter);
  DrawString(x + radius + 10, y - 5, TXT_E, Alignment::kCenter);
  DrawString(x + 3, y + 50, String(angle, 0) + "°", Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(x, y - 50, WindDegToOrdinalDirection(angle), Alignment::kCenter);
  SetFont(OpenSans24B);
  DrawString(x + 3, y - 18, String(windspeed, 1), Alignment::kCenter);
  SetFont(OpenSans12B);
  const bool isMetric = (cfg.units == "M");
  DrawString(x, y + 25, (isMetric ? "m/s" : "mph"), Alignment::kCenter);
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
  SetFont(OpenSans18B);
  DrawString(x - 30, y, String(wx_conditions.temperature, 1) + "°   " + String(wx_conditions.humidity, 0) + "%",
             Alignment::kLeft);
  SetFont(OpenSans12B);
  DrawPressureAndTrend(x + 195, y + 15, wx_conditions.pressure, wx_conditions.trend);
  int Yoffset = 42;
  if (wx_conditions.wind_speed > 0) {
    DrawString(x - 30, y + Yoffset, String(wx_conditions.feels_like, 1) + "° FL", Alignment::kLeft);
    Yoffset += 30;
  }
  DrawString(x - 30, y + Yoffset, String(wx_conditions.high, 0) + "° | " + String(wx_conditions.low, 0) + "° Hi/Lo",
             Alignment::kLeft);
}

void DisplayForecastTextSection(int x, int y) {
  constexpr uint8_t lineWidth = 34;
  const bool isMetric = (cfg.units == "M");
  SetFont(OpenSans12B);
  String Wx_Description = wx_conditions.forecast0;
  Wx_Description.replace(".", "");
  int spaceRemaining = 0, p = 0, charCount = 0;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > lineWidth - 1) {
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (wx_forecast[0].rainfall > 0)
    Wx_Description += " (" + String(wx_forecast[0].rainfall, 1) + String((isMetric ? "mm" : "in")) + ")";
  int sep = Wx_Description.indexOf("~");
  String Line1 = (sep >= 0) ? Wx_Description.substring(0, sep) : Wx_Description;
  String Line2 = (sep >= 0) ? Wx_Description.substring(sep + 1) : "";
  DrawString(x + 30, y + 5, TitleCase(Line1), Alignment::kLeft);
  if (Line2.length() > 0) DrawString(x + 30, y + 30, Line2, Alignment::kLeft);
}

void DisplayVisiCCoverUVISection(int x, int y) {
  SetFont(OpenSans12B);
  Visibility(x + 5, y, String(wx_conditions.visibility) + "M");
  CloudCover(x + 155, y, wx_conditions.cloud_cover);
  DisplayUVIndexLevel(x + 265, y, wx_conditions.uvi);
}

void DisplayUVIndexLevel(int x, int y, float UVI) {
  const char* Level = "";
  if (UVI <= 2) Level = " (L)";
  if (UVI >= 3 && UVI <= 5) Level = " (M)";
  if (UVI >= 6 && UVI <= 7) Level = " (H)";
  if (UVI >= 8 && UVI <= 10) Level = " (VH)";
  if (UVI >= 11) Level = " (EX)";
  DrawString(x + 20, y - 5, String(UVI, (UVI < 0 ? 1 : 0)) + Level, Alignment::kLeft);
  DrawUVI(x - 10, y - 5);
}

void DisplayForecastWeather(int x, int y, int index, int fwidth) {
  x = x + fwidth * index;
  DisplayConditionsSection(x + fwidth / 2 - 5, y + 85, wx_forecast[index].icon, small_icon);
  SetFont(OpenSans10B);
  DrawString(x + fwidth / 2, y + 30, String(ConvertUnixTime(wx_forecast[index].dt).substring(0, 5)),
             Alignment::kCenter);
  DrawString(x + fwidth / 2, y + 130,
             String(wx_forecast[index].high, 0) + "°/" + String(wx_forecast[index].low, 0) + "°", Alignment::kCenter);
}

static inline bool isSouthernHemisphere() {
  return atof(cfg.latitude.c_str()) < 0.0;
}

void DisplayAstronomySection(int x, int y) {
  SetFont(OpenSans10B);
  time_t now = time(NULL);
  struct tm* now_utc = gmtime(&now);
  DrawString(x + 5, y + 102, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900),
             Alignment::kLeft);
  DrawMoonImage(x + 10, y + 23);
  DrawMoon(x - 28, y - 15, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, isSouthernHemisphere());
  DrawString(x + 115, y + 40, ConvertUnixTime(wx_conditions.sunrise).substring(0, 5), Alignment::kLeft);
  DrawString(x + 115, y + 80, ConvertUnixTime(wx_conditions.sunset).substring(0, 5), Alignment::kLeft);
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
  jd -= b;
  double Phase = jd;
  if (southernHemisphere) Phase = 1 - Phase;
  FillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, kDarkGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    } else {
      Xpos1 = -Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos) / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines) / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines) / number_of_lines * diameter + y;
    DrawLine(pW1x, pW1y, pW2x, pW2y, kWhite);
    DrawLine(pW3x, pW3y, pW4x, pW4y, kWhite);
  }
  DrawCircle(x + diameter - 1, y + diameter, diameter / 2, kBlack);
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
  jd = c + e + d - 694039.09;
  jd /= 29.53059;
  b = jd;
  jd -= b;
  b = jd * 8 + 0.5;
  b = b & 7;
  if (b == 0) return TXT_MOON_NEW;
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;
  if (b == 2) return TXT_MOON_FIRST_QUARTER;
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;
  if (b == 4) return TXT_MOON_FULL;
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;
  if (b == 6) return TXT_MOON_THIRD_QUARTER;
  if (b == 7) return TXT_MOON_WANING_CRESCENT;
  return "";
}

void DisplayForecastSection(int x, int y) {
  int f = 0;
  do {
    DisplayForecastWeather(x, y, f, 82);
    f++;
  } while (f < 8);
}

void DisplayGraphSection(int x, int y) {
  const bool isMetric = (cfg.units == "M");
  static float pressure_readings[kMaxReadings] = {0};
  static float temperature_readings[kMaxReadings] = {0};
  static float humidity_readings[kMaxReadings] = {0};
  static float rain_readings[kMaxReadings] = {0};
  static float snow_readings[kMaxReadings] = {0};
  int r = 0;
  do {
    pressure_readings[r] = wx_forecast[r].pressure;
    rain_readings[r] = wx_forecast[r].rainfall;
    snow_readings[r] = wx_forecast[r].snowfall;
    temperature_readings[r] = wx_forecast[r].temperature;
    humidity_readings[r] = wx_forecast[r].humidity;
    r++;
  } while (r < kMaxGraphReadings);
  int gwidth = 175, gheight = 100;
  int gx = (epd_width() - gwidth * 4) / 5 + 8;
  int gy = (epd_height() - gheight - 30);
  int gap = gwidth + gx;
  DrawGraph(
      {gx + 0 * gap, gy, gwidth, gheight, 900, 1050, isMetric ? TXT_PRESSURE_HPA.c_str() : TXT_PRESSURE_IN.c_str(),
       kAutoscaleOn, kBarchartOff, kMaxGraphReadings},
      pressure_readings);
  DrawGraph(
      {gx + 1 * gap, gy, gwidth, gheight, 10, 30, isMetric ? TXT_TEMPERATURE_C.c_str() : TXT_TEMPERATURE_F.c_str(),
       kAutoscaleOn, kBarchartOff, kMaxGraphReadings},
      temperature_readings);
  DrawGraph({gx + 2 * gap, gy, gwidth, gheight, 0, 100, TXT_HUMIDITY_PERCENT.c_str(), kAutoscaleOff, kBarchartOff,
             kMaxGraphReadings},
            humidity_readings);
  if (SumOfPrecip(rain_readings, kMaxGraphReadings) >= SumOfPrecip(snow_readings, kMaxGraphReadings))
    DrawGraph(
        {gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, isMetric ? TXT_RAINFALL_MM.c_str() : TXT_RAINFALL_IN.c_str(),
         kAutoscaleOn, kBarchartOn, kMaxGraphReadings},
        rain_readings);
  else
    DrawGraph(
        {gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30, isMetric ? TXT_SNOWFALL_MM.c_str() : TXT_SNOWFALL_IN.c_str(),
         kAutoscaleOn, kBarchartOn, kMaxGraphReadings},
        snow_readings);
}

void DisplayConditionsSection(int x, int y, const char* icon_name, bool icon_size) {
  Serial.printf("Icon name: %s\n", icon_name);
  if (strcmp(icon_name, "01d") == 0 || strcmp(icon_name, "01n") == 0)
    ClearSky(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "02d") == 0 || strcmp(icon_name, "02n") == 0)
    FewClouds(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "03d") == 0 || strcmp(icon_name, "03n") == 0)
    ScatteredClouds(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "04d") == 0 || strcmp(icon_name, "04n") == 0)
    BrokenClouds(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "09d") == 0 || strcmp(icon_name, "09n") == 0)
    ChanceRain(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "10d") == 0 || strcmp(icon_name, "10n") == 0)
    Rain(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "11d") == 0 || strcmp(icon_name, "11n") == 0)
    Thunderstorms(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "13d") == 0 || strcmp(icon_name, "13n") == 0)
    Snow(x, y, icon_size, icon_name);
  else if (strcmp(icon_name, "50d") == 0 || strcmp(icon_name, "50n") == 0)
    Mist(x, y, icon_size, icon_name);
  else
    Nodata(x, y, icon_size, icon_name);
}

void DrawArrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x;
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y;
  float x1 = 0, y1 = plength, x2 = pwidth / 2, y2 = pwidth / 2, x3 = -pwidth / 2, y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  FillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, kBlack);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  DrawLine(x + o1, y + o2, x + o3, y + o4, kBlack);
  DrawLine(x + o11, y + o12, x + o13, y + o14, kBlack);
}

void DrawPressureAndTrend(int x, int y, float pressure, char slope) {
  const bool isMetric = (cfg.units == "M");
  DrawString(x + 25, y - 10, String(pressure, isMetric ? 0 : 1) + (isMetric ? "hPa" : "in"), Alignment::kLeft);
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
  SetFont(OpenSans8B);
  DrawRSSI(x + 305, y + 15, rssi);
  DrawBattery(x + 150, y);
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20) WIFIsignal = 30;
    if (_rssi <= -40) WIFIsignal = 24;
    if (_rssi <= -60) WIFIsignal = 18;
    if (_rssi <= -80) WIFIsignal = 12;
    if (_rssi <= -100) WIFIsignal = 6;
    FillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, kBlack);
    xpos++;
  }
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

void DrawGraph(GraphConfig gcfg, float data_array[]) {
  if (gcfg.autoscale) calculateGraphYScale(data_array, gcfg.readings, gcfg.y_min, gcfg.y_max);
  SetFont(OpenSans10B);
  int last_x = gcfg.x + 1;
  int last_y =
      gcfg.y + (gcfg.y_max - constrain(data_array[0], gcfg.y_min, gcfg.y_max)) / (gcfg.y_max - gcfg.y_min) * gcfg.h;
  DrawRect(gcfg.x, gcfg.y, gcfg.w + 3, gcfg.h + 2, kGrey);
  DrawString(gcfg.x - 20 + gcfg.w / 2, gcfg.y - 28, gcfg.title, Alignment::kCenter);
  for (int i = 0; i < gcfg.readings; i++) {
    float x2 = gcfg.x + i * gcfg.w / (gcfg.readings - 1) - 1;
    float y2 = gcfg.y +
               (gcfg.y_max - constrain(data_array[i], gcfg.y_min, gcfg.y_max)) / (gcfg.y_max - gcfg.y_min) * gcfg.h + 1;
    if (gcfg.barchart) {
      FillRect(last_x + 2, y2, (gcfg.w / gcfg.readings) - 1, gcfg.y + gcfg.h - y2 + 2, kBlack);
    } else {
      DrawLine(last_x, last_y - 1, x2, y2 - 1, kBlack);
      DrawLine(last_x, last_y, x2, y2, kBlack);
    }
    last_x = x2;
    last_y = y2;
  }
  for (int spacing = 0; spacing <= kGraphYDivisions; spacing++) {
    for (int j = 0; j < kGraphDashes; j++) {
      if (spacing < kGraphYDivisions)
        DrawFastHLine((gcfg.x + 3 + j * gcfg.w / kGraphDashes), gcfg.y + (gcfg.h * spacing / kGraphYDivisions),
                      gcfg.w / (2 * kGraphDashes), kGrey);
    }
    float axisVal = gcfg.y_max - (float)(gcfg.y_max - gcfg.y_min) / kGraphYDivisions * spacing + 0.01f;
    int labelX, decimalPlaces;
    if (axisVal < 5 || strcmp(gcfg.title, TXT_PRESSURE_IN.c_str()) == 0) {
      labelX = gcfg.x - 10;
      decimalPlaces = 1;
    } else if (gcfg.y_min < 1 && gcfg.y_max < 10) {
      labelX = gcfg.x - 3;
      decimalPlaces = 1;
    } else {
      labelX = gcfg.x - 7;
      decimalPlaces = 0;
    }
    DrawString(labelX, gcfg.y + gcfg.h * spacing / kGraphYDivisions - 5, String(axisVal, decimalPlaces),
               Alignment::kRight);
  }
  for (int i = 0; i < kGraphDaySections; i++) {
    DrawString(20 + gcfg.x + gcfg.w / kGraphDaySections * i, gcfg.y + gcfg.h + 10, String(i) + "d", Alignment::kLeft);
    if (i < kGraphDaySections - 1)
      DrawFastVLine(gcfg.x + gcfg.w / kGraphDaySections * i + gcfg.w / kGraphDaySections, gcfg.y, gcfg.h, kLightGrey);
  }
}

void DrawString(int x, int y, String text, Alignment align) {
  char* data = const_cast<char*>(text.c_str());
  int x1, y1, w, h;
  int xx = x, yy = y;
  EpdFontProperties props = epd_font_properties_default();
  epd_get_text_bounds(&current_font, data, &xx, &yy, &x1, &y1, &w, &h, &props);
  if (align == Alignment::kRight) x = x - w;
  if (align == Alignment::kCenter) x = x - w / 2;
  int cursor_y = y + h;
  epd_write_default(&current_font, data, &x, &cursor_y, framebuffer);
}

void FillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void DrawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void DrawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_draw_line(x0, y0, x1, y1, color, framebuffer);
}

void DrawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect({x, y, w, h}, color, framebuffer);
}

void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect({x, y, w, h}, color, framebuffer);
}

void FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void DrawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void SetFont(EpdFont const& font) {
  current_font = font;
}

#ifndef SIMULATOR_BUILD
bool UpdateLocalTime() {
  const bool isMetric = (cfg.units == "M");
  struct tm timeinfo;
  char time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) {
    Serial.printf("Failed to obtain time\n");
    return false;
  }
  current_hour = timeinfo.tm_hour;
  current_min = timeinfo.tm_min;
  current_sec = timeinfo.tm_sec;
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");
  if (isMetric) {
    snprintf(day_output, sizeof(day_output), "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday,
             month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);
    snprintf(time_output, sizeof(time_output), "%s", update_time);
  } else {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo);
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);
    snprintf(time_output, sizeof(time_output), "%s", update_time);
  }
  date_str = day_output;
  time_str = time_output;
  return true;
}

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, kDefaultVref, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
  }
  float voltage = analogRead(kBatteryAdcPin) / (float)kBatteryAdcBits * kBatteryVoltageDiv * (adc_chars.vref / 1000.0);
  if (voltage > 1) {
    Serial.printf("\nVoltage = %.2f\n", voltage);
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) -
                 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.20) percentage = 0;
    DrawRect(x + 25, y - 14, 40, 15, kBlack);
    FillRect(x + 65, y - 10, 4, 7, kBlack);
    FillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, kBlack);
    DrawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", Alignment::kLeft);
  }
}

void EdpUpdate() {
  epd_hl_update_screen(&epd_hl, MODE_GL16, (int)epd_ambient_temperature());
}

#endif  // SIMULATOR_BUILD
