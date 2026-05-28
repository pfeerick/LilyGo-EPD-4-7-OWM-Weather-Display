#ifndef FORECAST_RECORD_H_
#define FORECAST_RECORD_H_

struct Forecast_record_type {  // For current Day and Day 1, 2, 3, etc
  int Dt;
  char Icon[8];        // OWM icon code e.g. "01d", "01n" (3 chars + null)
  char Trend;          // Pressure trend: '+', '-', '0', '='
  char Forecast0[64];  // Weather description e.g. "scattered clouds"
  float Temperature;
  float FeelsLike;
  float DewPoint;
  float Humidity;
  float High;
  float Low;
  float Winddir;
  float Windspeed;
  float Rainfall;
  float Snowfall;
  float Pressure;
  int Cloudcover;
  int Visibility;
  int Sunrise;
  int Sunset;
  float UVI;
};

#endif /* ifndef FORECAST_RECORD_H_ */
