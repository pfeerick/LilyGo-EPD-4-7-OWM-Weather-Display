#include "weather_api.h"

#ifndef SIMULATOR_BUILD
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "defaults.h"

ForecastRecord wx_conditions;
ForecastRecord wx_forecast[kMaxReadings];
#endif

void ConvertReadingsToImperial(int count) {
  wx_conditions.pressure = HpaToInhg(wx_conditions.pressure);
  for (int i = 0; i < count; i++) {
    wx_forecast[i].rainfall = MmToInches(wx_forecast[i].rainfall);
    wx_forecast[i].snowfall = MmToInches(wx_forecast[i].snowfall);
  }
}

String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  const bool isMetric = (cfg.units == "M");
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

float MmToInches(float value_mm) {
  return 0.0393701 * value_mm;
}

float HpaToInhg(float value_hPa) {
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
  wx_conditions.high = -50;  // Sentinel: replaced by daily[0] max below
  wx_conditions.low = 50;    // Sentinel: replaced by daily[0] min below
  JsonObject current = doc["current"];
  wx_conditions.sunrise = current["sunrise"];
  Serial.printf("SRis: %d\n", wx_conditions.sunrise);
  wx_conditions.sunset = current["sunset"];
  Serial.printf("SSet: %d\n", wx_conditions.sunset);
  wx_conditions.temperature = current["temp"];
  Serial.printf("Temp: %f\n", wx_conditions.temperature);
  wx_conditions.feels_like = current["feels_like"];
  Serial.printf("FLik: %f\n", wx_conditions.feels_like);
  wx_conditions.pressure = current["pressure"];
  Serial.printf("Pres: %f\n", wx_conditions.pressure);
  wx_conditions.humidity = current["humidity"];
  Serial.printf("Humi: %f\n", wx_conditions.humidity);
  wx_conditions.dew_point = current["dew_point"];
  Serial.printf("DPoi: %f\n", wx_conditions.dew_point);
  wx_conditions.uvi = current["uvi"];
  Serial.printf("UVin: %f\n", wx_conditions.uvi);
  wx_conditions.cloud_cover = current["clouds"];
  Serial.printf("CCov: %d\n", wx_conditions.cloud_cover);
  wx_conditions.visibility = current["visibility"];
  Serial.printf("Visi: %d\n", wx_conditions.visibility);
  wx_conditions.wind_speed = current["wind_speed"];
  Serial.printf("WSpd: %f\n", wx_conditions.wind_speed);
  wx_conditions.wind_dir = current["wind_deg"];
  Serial.printf("WDir: %d\n", wx_conditions.wind_dir);
  JsonObject current_weather = current["weather"][0];
  strlcpy(wx_conditions.forecast0, current_weather["description"] | "", sizeof(wx_conditions.forecast0));
  Serial.printf("Fore: %s\n", wx_conditions.forecast0);
  strlcpy(wx_conditions.icon, current_weather["icon"] | "", sizeof(wx_conditions.icon));
  Serial.printf("Icon: %s\n", wx_conditions.icon);

  Serial.printf("\nReceiving Forecast period - ");

  JsonArray daily = doc["daily"];
  wx_conditions.low = daily[0]["temp"]["min"].as<float>();
  Serial.printf("TLow: %f\n", wx_conditions.low);
  wx_conditions.high = daily[0]["temp"]["max"].as<float>();
  Serial.printf("High: %f\n", wx_conditions.high);
  wx_conditions.moon_phase = daily[0]["moon_phase"].as<float>();
  Serial.printf("MPhs: %f\n", wx_conditions.moon_phase);

  //TODO: daily[1..7] has 7 more days of temp/icon/description data — add a weekly forecast row if screen space allows

  JsonArray list = doc["hourly"];
  byte wxIndex = 0;
  Serial.printf("hourly list size: %u\n", list.size());
  for (byte r = 0; r < 48 && wxIndex < 16; r += 3) {
    Serial.printf("\nPeriod-%u--------------\n", r);
    wx_forecast[wxIndex].dt = list[r]["dt"].as<int>();
    wx_forecast[wxIndex].temperature = list[r]["temp"].as<float>();
    Serial.printf("Temp: %f\n", wx_forecast[wxIndex].temperature);
    float t1 = (r + 1 < (int)list.size()) ? list[r + 1]["temp"].as<float>() : wx_forecast[wxIndex].temperature;
    float t2 = (r + 2 < (int)list.size()) ? list[r + 2]["temp"].as<float>() : wx_forecast[wxIndex].temperature;
    wx_forecast[wxIndex].high = max(max(wx_forecast[wxIndex].temperature, t1), t2);
    Serial.printf("High: %f\n", wx_forecast[wxIndex].high);
    wx_forecast[wxIndex].low = min(min(wx_forecast[wxIndex].temperature, t1), t2);
    Serial.printf("Low: %f\n", wx_forecast[wxIndex].low);
    wx_forecast[wxIndex].pressure = list[r]["pressure"].as<float>();
    Serial.printf("Pres: %f\n", wx_forecast[wxIndex].pressure);
    wx_forecast[wxIndex].humidity = list[r]["humidity"].as<float>();
    Serial.printf("Humi: %f\n", wx_forecast[wxIndex].humidity);
    strlcpy(wx_forecast[wxIndex].icon, list[r]["weather"][0]["icon"] | "", sizeof(wx_forecast[wxIndex].icon));
    Serial.printf("Icon: %s\n", wx_forecast[wxIndex].icon);
    wx_forecast[wxIndex].rainfall = list[r]["rain"]["1h"].as<float>();
    Serial.printf("Rain: %f\n", wx_forecast[wxIndex].rainfall);
    wx_forecast[wxIndex].snowfall = list[r]["snow"]["1h"].as<float>();
    Serial.printf("Snow: %f\n", wx_forecast[wxIndex].snowfall);
    wxIndex++;
  }
  if (wxIndex >= 3) {
    float pressure_trend = wx_forecast[0].pressure - wx_forecast[2].pressure;
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0;
    wx_conditions.trend = '=';
    if (pressure_trend > 0) wx_conditions.trend = '+';
    if (pressure_trend < 0) wx_conditions.trend = '-';
    if (pressure_trend == 0) wx_conditions.trend = '0';
  } else {
    wx_conditions.trend = '0';
  }
  if (cfg.units == "I") ConvertReadingsToImperial(wxIndex);
  return true;
}

bool ObtainWeatherData(WiFiClient& client, const String& RequestType) {
  const bool isMetric = (cfg.units == "M");
  const String units = (isMetric ? "metric" : "imperial");
  client.stop();
  HTTPClient http;
  String uri = "/data/3.0/" + RequestType + "?lat=" + cfg.latitude.c_str() + "&lon=" + cfg.longitude.c_str() +
               "&appid=" + cfg.apikey.c_str() + "&mode=json&units=" + units + "&lang=" + cfg.language.c_str();
  if (RequestType == "onecall") uri += "&exclude=minutely,alerts";
  http.begin(client, cfg.server.c_str(), 80, uri);
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
