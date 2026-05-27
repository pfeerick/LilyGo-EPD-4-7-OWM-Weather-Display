#include "display.h"
#include "icons.h"
#include "translations/lang_en.h"

#include "defaults.h"

#ifndef SIMULATOR_BUILD
#include "esp_adc_cal.h"
#include "power.h"  // for StartTime extern (set in InitialiseSystem)

bool LargeIcon = true;
bool SmallIcon = false;
String Time_str = "--:--:--";
String Date_str = "-- --- ----";
int wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
EpdFont currentFont;
uint8_t* framebuffer;
EpdiyHighlevelState hl;

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
  arrow(x, y, Cradius - 22, angle, 18, 33);
  setFont(OpenSans8B);
  int dxo, dyo, dxi, dyi;
  drawCircle(x, y, Cradius, Black);
  drawCircle(x, y, Cradius + 1, Black);
  drawCircle(x, y, Cradius * 0.7, Black);
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
    drawString(x - 30, y + Yoffset, String(WxConditions.FeelsLike, 1) + "° FL", LEFT);
    Yoffset += 30;
  }
  drawString(x - 30, y + Yoffset, String(WxConditions.High, 0) + "° | " + String(WxConditions.Low, 0) + "° Hi/Lo", LEFT);
}

void DisplayForecastTextSection(int x, int y) {
  constexpr uint8_t lineWidth = 34;
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  setFont(OpenSans12B);
  String Wx_Description = WxConditions.Forecast0;
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
  drawString(x + fwidth / 2, y + 130,
             String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°", CENTER);
}

static inline bool isSouthernHemisphere() {
  return atof(cfg.latitude) < 0.0;
}

void DisplayAstronomySection(int x, int y) {
  setFont(OpenSans10B);
  time_t now = time(NULL);
  struct tm* now_utc = gmtime(&now);
  drawString(x + 5, y + 102, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900), LEFT);
  DrawMoonImage(x + 10, y + 23);
  DrawMoon(x - 28, y - 15, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900,
           isSouthernHemisphere());
  drawString(x + 115, y + 40, ConvertUnixTime(WxConditions.Sunrise).substring(0, 5), LEFT);
  drawString(x + 115, y + 80, ConvertUnixTime(WxConditions.Sunset).substring(0, 5), LEFT);
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
  b = (int)(Phase * 8 + 0.5) & 7;
  if (southernHemisphere) Phase = 1 - Phase;
  int octant = (int)(Phase * 8 + 0.5) & 7;
  fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, DarkGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (octant < 5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    } else {
      Xpos1 = Xpos;
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
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  static float pressure_readings[max_readings]    = {0};
  static float temperature_readings[max_readings] = {0};
  static float humidity_readings[max_readings]    = {0};
  static float rain_readings[max_readings]         = {0};
  static float snow_readings[max_readings]         = {0};
  int r = 0;
  do {
    pressure_readings[r]    = WxForecast[r].Pressure;
    rain_readings[r]        = WxForecast[r].Rainfall;
    snow_readings[r]        = WxForecast[r].Snowfall;
    temperature_readings[r] = WxForecast[r].Temperature;
    humidity_readings[r]    = WxForecast[r].Humidity;
    r++;
  } while (r < max_graph_readings);
  int gwidth = 175, gheight = 100;
  int gx = (epd_width() - gwidth * 4) / 5 + 8;
  int gy = (epd_height() - gheight - 30);
  int gap = gwidth + gx;
  DrawGraph({gx + 0 * gap, gy, gwidth, gheight, 900, 1050,
             isMetric ? TXT_PRESSURE_HPA.c_str() : TXT_PRESSURE_IN.c_str(), autoscale_on, barchart_off, max_graph_readings},
            pressure_readings);
  DrawGraph({gx + 1 * gap, gy, gwidth, gheight, 10, 30,
             isMetric ? TXT_TEMPERATURE_C.c_str() : TXT_TEMPERATURE_F.c_str(), autoscale_on, barchart_off, max_graph_readings},
            temperature_readings);
  DrawGraph({gx + 2 * gap, gy, gwidth, gheight, 0, 100,
             TXT_HUMIDITY_PERCENT.c_str(), autoscale_off, barchart_off, max_graph_readings},
            humidity_readings);
  if (SumOfPrecip(rain_readings, max_graph_readings) >= SumOfPrecip(snow_readings, max_graph_readings))
    DrawGraph({gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30,
               isMetric ? TXT_RAINFALL_MM.c_str() : TXT_RAINFALL_IN.c_str(), autoscale_on, barchart_on, max_graph_readings},
              rain_readings);
  else
    DrawGraph({gx + 3 * gap + 5, gy, gwidth, gheight, 0, 30,
               isMetric ? TXT_SNOWFALL_MM.c_str() : TXT_SNOWFALL_IN.c_str(), autoscale_on, barchart_on, max_graph_readings},
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
    if (_rssi <= -20) WIFIsignal = 30;
    if (_rssi <= -40) WIFIsignal = 24;
    if (_rssi <= -60) WIFIsignal = 18;
    if (_rssi <= -80) WIFIsignal = 12;
    if (_rssi <= -100) WIFIsignal = 6;
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
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

void DrawGraph(GraphConfig gcfg, float DataArray[]) {
  if (gcfg.autoscale) calculateGraphYScale(DataArray, gcfg.readings, gcfg.yMin, gcfg.yMax);
  setFont(OpenSans10B);
  int last_x = gcfg.x + 1;
  int last_y = gcfg.y + (gcfg.yMax - constrain(DataArray[0], gcfg.yMin, gcfg.yMax)) / (gcfg.yMax - gcfg.yMin) * gcfg.h;
  drawRect(gcfg.x, gcfg.y, gcfg.w + 3, gcfg.h + 2, Grey);
  drawString(gcfg.x - 20 + gcfg.w / 2, gcfg.y - 28, gcfg.title, CENTER);
  for (int i = 0; i < gcfg.readings; i++) {
    float x2 = gcfg.x + i * gcfg.w / (gcfg.readings - 1) - 1;
    float y2 = gcfg.y + (gcfg.yMax - constrain(DataArray[i], gcfg.yMin, gcfg.yMax)) / (gcfg.yMax - gcfg.yMin) * gcfg.h + 1;
    if (gcfg.barchart) {
      fillRect(last_x + 2, y2, (gcfg.w / gcfg.readings) - 1, gcfg.y + gcfg.h - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black);
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }
  for (int spacing = 0; spacing <= kGraphYDivisions; spacing++) {
    for (int j = 0; j < kGraphDashes; j++) {
      if (spacing < kGraphYDivisions)
        drawFastHLine((gcfg.x + 3 + j * gcfg.w / kGraphDashes), gcfg.y + (gcfg.h * spacing / kGraphYDivisions),
                      gcfg.w / (2 * kGraphDashes), Grey);
    }
    float axisVal = gcfg.yMax - (float)(gcfg.yMax - gcfg.yMin) / kGraphYDivisions * spacing + 0.01f;
    int labelX, decimalPlaces;
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
  int x1, y1, w, h;
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
bool UpdateLocalTime() {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  struct tm timeinfo;
  char time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) {
    Serial.printf("Failed to obtain time\n");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
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
  Date_str = day_output;
  Time_str = time_output;
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
    drawRect(x + 25, y - 14, 40, 15, Black);
    fillRect(x + 65, y - 10, 4, 7, Black);
    fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
    drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
  }
}

void edp_update() {
  epd_hl_update_screen(&hl, MODE_GL16, (int)epd_ambient_temperature());
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
#endif  // SIMULATOR_BUILD
