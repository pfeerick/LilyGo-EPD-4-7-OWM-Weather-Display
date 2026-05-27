#pragma once
#include "forecast_record.h"
#include "config.h"

constexpr uint8_t kMaxReadings      = 24;
constexpr uint8_t kMaxGraphReadings = 16;

extern ForecastRecord WxConditions;
extern ForecastRecord WxForecast[];

#ifndef SIMULATOR_BUILD
#include <WiFiClient.h>
bool obtainWeatherData(WiFiClient& client, const String& RequestType);
bool DecodeWeather(WiFiClient& json, const String& Type);
#endif

String ConvertUnixTime(int unix_time);
float mm_to_inches(float value_mm);
float hPa_to_inHg(float value_hPa);
int JulianDate(int d, int m, int y);
float SumOfPrecip(float DataArray[], int readings);
String TitleCase(const String& text);
void Convert_Readings_to_Imperial(int count);
