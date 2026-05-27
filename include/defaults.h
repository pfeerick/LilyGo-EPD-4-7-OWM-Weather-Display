#ifndef DEFAULTS_H_
#define DEFAULTS_H_

// Default configuration values — single source of truth used by config.cpp
// and any code that needs fallback values.

constexpr const char* kDefaultServer      = "api.openweathermap.org";
constexpr const char* kDefaultNtpServer   = "pool.ntp.org";
constexpr const char* kDefaultNtpFallback = "time.nist.gov";
constexpr const char* kDefaultLanguage    = "EN";
constexpr const char* kDefaultUnits       = "M";

constexpr int kDefaultSleepDuration = 60;  // minutes
constexpr int kDefaultWakeupHour    = 8;   // 0-23
constexpr int kDefaultSleepHour     = 23;  // 0-23

#endif /* ifndef DEFAULTS_H_ */
