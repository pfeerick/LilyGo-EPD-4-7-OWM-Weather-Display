#include "weather_api.h"

#ifndef SIMULATOR_BUILD
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "defaults.h"

Forecast_record_type WxConditions;
Forecast_record_type WxForecast[max_readings];
#endif

void Convert_Readings_to_Imperial(int count) {
  WxConditions.Pressure = hPa_to_inHg(WxConditions.Pressure);
  for (int i = 0; i < count; i++) {
    WxForecast[i].Rainfall = mm_to_inches(WxForecast[i].Rainfall);
    WxForecast[i].Snowfall = mm_to_inches(WxForecast[i].Snowfall);
  }
}

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
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3;
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
    return temp_text + text.substring(1);
  } else
    return text;
}

#ifndef SIMULATOR_BUILD
bool DecodeWeather(WiFiClient& json, const String& Type) {
  Serial.printf("\nCreating object...");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
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

  Serial.printf("\nReceiving Forecast period - ");

  JsonArray daily = doc["daily"];
  WxConditions.Low = daily[0]["temp"]["min"].as<float>();
  Serial.printf("TLow: %f\n", WxConditions.Low);
  WxConditions.High = daily[0]["temp"]["max"].as<float>();
  Serial.printf("High: %f\n", WxConditions.High);

  //TODO: daily[1..7] has 7 more days of temp/icon/description data — add a weekly forecast row if screen space allows

  JsonArray list = doc["hourly"];
  byte wxIndex = 0;
  Serial.printf("hourly list size: %u\n", list.size());
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
    wxIndex++;
  }
  if (wxIndex >= 3) {
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure;
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0;
    WxConditions.Trend = '=';
    if (pressure_trend > 0) WxConditions.Trend = '+';
    if (pressure_trend < 0) WxConditions.Trend = '-';
    if (pressure_trend == 0) WxConditions.Trend = '0';
  } else {
    WxConditions.Trend = '0';
  }
  if (strcmp(cfg.units, "I") == 0) Convert_Readings_to_Imperial(wxIndex);
  return true;
}

bool obtainWeatherData(WiFiClient& client, const String& RequestType) {
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  const String units = (isMetric ? "metric" : "imperial");
  client.stop();
  HTTPClient http;
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
