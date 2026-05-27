#pragma once
#include "epdiy.h"
#include "weather_api.h"

enum class Alignment { kLeft, kRight, kCenter };
constexpr uint8_t kWhite     = 0xFF;
constexpr uint8_t kLightGrey = 0xBB;
constexpr uint8_t kGrey      = 0x88;
constexpr uint8_t kDarkGrey  = 0x44;
constexpr uint8_t kBlack     = 0x00;

constexpr bool kAutoscaleOn  = true;
constexpr bool kAutoscaleOff = false;
constexpr bool kBarchartOn   = true;
constexpr bool kBarchartOff  = false;

constexpr uint8_t kGraphYDivisions  = 5;
constexpr uint8_t kGraphDashes      = 20;
constexpr uint8_t kGraphDaySections = 2;

constexpr uint8_t kLarge = 20;
constexpr uint8_t kSmall = 10;

struct GraphConfig {
  int x, y, w, h;
  float yMin, yMax;
  const char* title;
  bool autoscale;
  bool barchart;
  int readings;
};

extern bool LargeIcon;
extern bool SmallIcon;
extern String Time_str;
extern String Date_str;
extern int wifi_signal, CurrentHour, CurrentMin, CurrentSec;
extern EpdFont currentFont;
extern uint8_t* framebuffer;
#ifndef SIMULATOR_BUILD
extern EpdiyHighlevelState hl;
#endif

void InitialiseSystem();
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
void DrawBattery(int x, int y);
void DrawMoonImage(int x, int y);
void DrawSunriseImage(int x, int y);
void DrawSunsetImage(int x, int y);
void DrawUVI(int x, int y);
void DrawGraph(GraphConfig gcfg, float DataArray[]);

void drawString(int x, int y, String text, Alignment align);
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
