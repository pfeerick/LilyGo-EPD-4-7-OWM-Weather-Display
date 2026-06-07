#ifndef FORECAST_RECORD_H_
#define FORECAST_RECORD_H_

struct ForecastRecord {  // For current Day and Day 1, 2, 3, etc
  int dt;
  char icon[8];        // OWM icon code e.g. "01d", "01n" (3 chars + null)
  char trend;          // Pressure trend: '+', '-', '0', '='
  char forecast0[64];  // Weather description e.g. "scattered clouds"
  float temperature;
  float feels_like;
  float dew_point;
  float humidity;
  float high;
  float low;
  float wind_dir;
  float wind_speed;
  float rainfall;
  float snowfall;
  float pressure;
  int cloud_cover;
  int visibility;
  int sunrise;
  int sunset;
  float uvi;
  float moon_phase;
};

#endif /* ifndef FORECAST_RECORD_H_ */
