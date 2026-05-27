#pragma once
#include "forecast_record.h"
#include "config.h"

constexpr uint8_t kMaxReadings      = 24;
constexpr uint8_t kMaxGraphReadings = 16;

extern ForecastRecord wx_conditions;
extern ForecastRecord wx_forecast[];

#ifndef SIMULATOR_BUILD
#include <WiFiClient.h>
bool ObtainWeatherData(WiFiClient& client, const String& request_type);
bool DecodeWeather(WiFiClient& json, const String& type);
#endif

String ConvertUnixTime(int unix_time);
float MmToInches(float value_mm);
float HpaToInhg(float value_hpa);
int JulianDate(int d, int m, int y);
float SumOfPrecip(float data_array[], int readings);
String TitleCase(const String& text);
void ConvertReadingsToImperial(int count);
