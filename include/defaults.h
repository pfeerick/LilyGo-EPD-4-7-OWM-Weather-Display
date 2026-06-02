#ifndef DEFAULTS_H_
#define DEFAULTS_H_

// ---- Display layout anchor points (960x540 canvas) ----
constexpr int kLayoutStatusX = 600;
constexpr int kLayoutStatusY = 20;
constexpr int kLayoutWindX = 137;
constexpr int kLayoutWindY = 150;
constexpr int kLayoutWindRadius = 100;
constexpr int kLayoutAstronomyX = 5;
constexpr int kLayoutAstronomyY = 252;
constexpr int kLayoutMainWeatherX = 320;
constexpr int kLayoutMainWeatherY = 110;
constexpr int kLayoutWeatherIconX = 835;
constexpr int kLayoutWeatherIconY = 140;
constexpr int kLayoutForecastX = 285;
constexpr int kLayoutForecastY = 220;
constexpr int kLayoutGraphX = 320;
constexpr int kLayoutGraphY = 220;

// ---- Hardware ----
constexpr int kBatteryAdcPin = 36;
constexpr float kBatteryVoltageDiv = 6.566f;  // voltage divider ratio for ADC pin 36
constexpr int kBatteryAdcBits = 4096;         // 12-bit ADC
constexpr int kDefaultVref = 1100;            // mV, overridden by eFuse calibration if available

// ---- Default configuration values ----
// Single source of truth used by config.cpp and any code that needs fallback values.

constexpr const char* kDefaultServer = "api.openweathermap.org";
constexpr const char* kDefaultNtpServer = "pool.ntp.org";
constexpr const char* kDefaultNtpFallback = "time.nist.gov";
constexpr const char* kDefaultLanguage = "EN";
constexpr const char* kDefaultUnits = "M";

constexpr int kDefaultSleepDuration = 60;  // minutes
constexpr int kDefaultWakeupHour = 8;      // 0-23
constexpr int kDefaultSleepHour = 23;      // 0-23

#endif /* ifndef DEFAULTS_H_ */
