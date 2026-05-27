#pragma once
#include "forecast_record.h"
#include "config.h"

constexpr uint8_t max_readings       = 24;
constexpr uint8_t max_graph_readings = 16;

extern Forecast_record_type WxConditions;
extern Forecast_record_type WxForecast[];

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
