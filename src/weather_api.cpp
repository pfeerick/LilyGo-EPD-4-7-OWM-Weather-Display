#include "weather_api.h"

#ifndef SIMULATOR_BUILD
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "defaults.h"

ForecastRecord WxConditions;
ForecastRecord WxForecast[kMaxReadings];
#endif

void Convert_Readings_to_Imperial(int count) {
  WxConditions.pressure = hPa_to_inHg(WxConditions.pressure);
  for (int i = 0; i < count; i++) {
    WxForecast[i].rainfall = mm_to_inches(WxForecast[i].rainfall);
    WxForecast[i].snowfall = mm_to_inches(WxForecast[i].snowfall);
  }
}

String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  const bool isMetric = (strcmp(cfg.units, "M") == 0);
  time_t tm = unix_time + cfg.gmt_offset_sec + cfg.daylight_offset_sec;
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
  WxConditions.high = -50;  // Sentinel: replaced by daily[0] max below
  WxConditions.low = 50;    // Sentinel: replaced by daily[0] min below
  JsonObject current = doc["current"];
  WxConditions.sunrise = current["sunrise"];
  Serial.printf("SRis: %d\n", WxConditions.sunrise);
  WxConditions.sunset = current["sunset"];
  Serial.printf("SSet: %d\n", WxConditions.sunset);
  WxConditions.temperature = current["temp"];
  Serial.printf("Temp: %f\n", WxConditions.temperature);
  WxConditions.feels_like = current["feels_like"];
  Serial.printf("FLik: %f\n", WxConditions.feels_like);
  WxConditions.pressure = current["pressure"];
  Serial.printf("Pres: %f\n", WxConditions.pressure);
  WxConditions.humidity = current["humidity"];
  Serial.printf("Humi: %f\n", WxConditions.humidity);
  WxConditions.dew_point = current["dew_point"];
  Serial.printf("DPoi: %f\n", WxConditions.dew_point);
  WxConditions.uvi = current["uvi"];
  Serial.printf("UVin: %f\n", WxConditions.uvi);
  WxConditions.cloud_cover = current["clouds"];
  Serial.printf("CCov: %d\n", WxConditions.cloud_cover);
  WxConditions.visibility = current["visibility"];
  Serial.printf("Visi: %d\n", WxConditions.visibility);
  WxConditions.wind_speed = current["wind_speed"];
  Serial.printf("WSpd: %f\n", WxConditions.wind_speed);
  WxConditions.wind_dir = current["wind_deg"];
  Serial.printf("WDir: %d\n", WxConditions.wind_dir);
  JsonObject current_weather = current["weather"][0];
  strlcpy(WxConditions.forecast0, current_weather["description"] | "", sizeof(WxConditions.forecast0));
  Serial.printf("Fore: %s\n", WxConditions.forecast0);
  strlcpy(WxConditions.icon, current_weather["icon"] | "", sizeof(WxConditions.icon));
  Serial.printf("Icon: %s\n", WxConditions.icon);

  Serial.printf("\nReceiving Forecast period - ");

  JsonArray daily = doc["daily"];
  WxConditions.low = daily[0]["temp"]["min"].as<float>();
  Serial.printf("TLow: %f\n", WxConditions.low);
  WxConditions.high = daily[0]["temp"]["max"].as<float>();
  Serial.printf("High: %f\n", WxConditions.high);

  //TODO: daily[1..7] has 7 more days of temp/icon/description data — add a weekly forecast row if screen space allows

  JsonArray list = doc["hourly"];
  byte wxIndex = 0;
  Serial.printf("hourly list size: %u\n", list.size());
  for (byte r = 0; r < 48 && wxIndex < 16; r += 3) {
    Serial.printf("\nPeriod-%u--------------\n", r);
    WxForecast[wxIndex].dt = list[r]["dt"].as<int>();
    WxForecast[wxIndex].temperature = list[r]["temp"].as<float>();
    Serial.printf("Temp: %f\n", WxForecast[wxIndex].temperature);
    float t1 = (r + 1 < (int)list.size()) ? list[r + 1]["temp"].as<float>() : WxForecast[wxIndex].temperature;
    float t2 = (r + 2 < (int)list.size()) ? list[r + 2]["temp"].as<float>() : WxForecast[wxIndex].temperature;
    WxForecast[wxIndex].high = max(max(WxForecast[wxIndex].temperature, t1), t2);
    Serial.printf("High: %f\n", WxForecast[wxIndex].high);
    WxForecast[wxIndex].low = min(min(WxForecast[wxIndex].temperature, t1), t2);
    Serial.printf("Low: %f\n", WxForecast[wxIndex].low);
    WxForecast[wxIndex].pressure = list[r]["pressure"].as<float>();
    Serial.printf("Pres: %f\n", WxForecast[wxIndex].pressure);
    WxForecast[wxIndex].humidity = list[r]["humidity"].as<float>();
    Serial.printf("Humi: %f\n", WxForecast[wxIndex].humidity);
    strlcpy(WxForecast[wxIndex].icon, list[r]["weather"][0]["icon"] | "", sizeof(WxForecast[wxIndex].icon));
    Serial.printf("Icon: %s\n", WxForecast[wxIndex].icon);
    WxForecast[wxIndex].rainfall = list[r]["rain"]["1h"].as<float>();
    Serial.printf("Rain: %f\n", WxForecast[wxIndex].rainfall);
    WxForecast[wxIndex].snowfall = list[r]["snow"]["1h"].as<float>();
    Serial.printf("Snow: %f\n", WxForecast[wxIndex].snowfall);
    wxIndex++;
  }
  if (wxIndex >= 3) {
    float pressure_trend = WxForecast[0].pressure - WxForecast[2].pressure;
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0;
    WxConditions.trend = '=';
    if (pressure_trend > 0) WxConditions.trend = '+';
    if (pressure_trend < 0) WxConditions.trend = '-';
    if (pressure_trend == 0) WxConditions.trend = '0';
  } else {
    WxConditions.trend = '0';
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
