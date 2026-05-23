#pragma once

// ============================================================
// PC simulation stubs — replaces Arduino.h, ESP-IDF headers,
// WiFi/HTTP, and the EPDIY hardware layer.
// ============================================================

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
#include <string>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cmath>

// --------------- Arduino String type ---------------
// Thin wrapper so the rendering code's String usage compiles unchanged.
class String {
 public:
  std::string s;
  String() = default;
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& str) : s(str) {}
  String(int n) : s(std::to_string(n)) {}
  String(unsigned int n) : s(std::to_string(n)) {}
  String(long n) : s(std::to_string(n)) {}
  String(float f, int decimals = 2) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*f", decimals, (double)f);
    s = buf;
  }
  String(double f, int decimals = 2) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*f", decimals, f);
    s = buf;
  }
  String(char c) : s(1, c) {}

  size_t length() const {
    return s.size();
  }
  const char* c_str() const {
    return s.c_str();
  }
  bool isEmpty() const {
    return s.empty();
  }

  String substring(int from) const {
    return String(s.substr(from));
  }
  String substring(int from, int to) const {
    return String(s.substr(from, to - from));
  }
  int indexOf(const String& o, int from = 0) const {
    auto pos = s.find(o.s, from);
    return pos == std::string::npos ? -1 : (int)pos;
  }
  int indexOf(char c, int from = 0) const {
    auto pos = s.find(c, from);
    return pos == std::string::npos ? -1 : (int)pos;
  }
  void toUpperCase() {
    for (auto& c : s)
      c = (char)toupper((unsigned char)c);
  }
  void toLowerCase() {
    for (auto& c : s)
      c = (char)tolower((unsigned char)c);
  }
  float toFloat() const {
    return (float)std::stof(s);
  }
  int toInt() const {
    try {
      return std::stoi(s);
    } catch (...) {
      return 0;
    }
  }

  void replace(const String& from, const String& to) {
    size_t pos = 0;
    while ((pos = s.find(from.s, pos)) != std::string::npos) {
      s.replace(pos, from.s.size(), to.s);
      pos += to.s.size();
    }
  }
  void replace(char from, char to) {
    for (auto& c : s)
      if (c == from) c = to;
  }
  String& trim() {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
    return *this;
  }

  bool startsWith(const String& o) const {
    return s.rfind(o.s, 0) == 0;
  }
  bool endsWith(const String& o) const {
    if (o.s.size() > s.size()) return false;
    return s.compare(s.size() - o.s.size(), o.s.size(), o.s) == 0;
  }
  bool equals(const String& o) const {
    return s == o.s;
  }
  bool equalsIgnoreCase(const String& o) const {
    if (s.size() != o.s.size()) return false;
    for (size_t i = 0; i < s.size(); i++)
      if (tolower((unsigned char)s[i]) != tolower((unsigned char)o.s[i])) return false;
    return true;
  }

  String& operator+=(const String& o) {
    s += o.s;
    return *this;
  }
  String& operator+=(const char* c) {
    if (c) s += c;
    return *this;
  }
  String& operator+=(char c) {
    s += c;
    return *this;
  }
  String& operator+=(int n) {
    s += std::to_string(n);
    return *this;
  }

  bool operator==(const String& o) const {
    return s == o.s;
  }
  bool operator!=(const String& o) const {
    return s != o.s;
  }
  bool operator<(const String& o) const {
    return s < o.s;
  }
  char operator[](int i) const {
    return s[i];
  }
};

inline String operator+(const String& a, const String& b) {
  return String(a.s + b.s);
}
inline String operator+(const String& a, const char* b) {
  return String(a.s + (b ? b : ""));
}
inline String operator+(const char* a, const String& b) {
  return String(std::string(a ? a : "") + b.s);
}
inline String operator+(const String& a, char b) {
  return String(a.s + b);
}
inline String operator+(const String& a, int b) {
  return String(a.s + std::to_string(b));
}
inline String operator+(const String& a, float b) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.2f", b);
  return String(a.s + buf);
}
inline String operator+(const String& a, double b) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.2f", b);
  return String(a.s + buf);
}
inline String operator+(int a, const String& b) {
  return String(std::to_string(a) + b.s);
}

// Needed for Serial.println(&timeinfo, fmt) — we just format to stderr
inline String operator+(const String& a, const void*) {
  return a;
}

using boolean = bool;
using byte = uint8_t;
using word = uint16_t;

// --------------- Serial stub ---------------
struct SerialClass {
  void begin(int) {}
  template <typename T>
  void print(T v) {
    fprintf(stderr, "%s", String(v).c_str());
  }
  void print(const char* s) {
    fputs(s, stderr);
  }
  void print(const String& s) {
    fputs(s.c_str(), stderr);
  }
  // println with struct tm* (UpdateLocalTime calls Serial.println(&timeinfo, fmt))
  void println(const struct tm* t, const char* fmt) {
    char buf[64];
    strftime(buf, sizeof(buf), fmt, t);
    fprintf(stderr, "%s\n", buf);
  }
  template <typename T>
  void println(T v) {
    fprintf(stderr, "%s\n", String(v).c_str());
  }
  void println(const char* s) {
    fprintf(stderr, "%s\n", s);
  }
  void println(const String& s) {
    fprintf(stderr, "%s\n", s.c_str());
  }
  void println() {
    fputs("\n", stderr);
  }
  void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
  }
  void flush() {}
} Serial;

// --------------- Arduino helpers ---------------
inline unsigned long millis() {
  return (unsigned long)(clock() * 1000 / CLOCKS_PER_SEC);
}
inline void delay(int) {}
inline void pinMode(int, int) {}
inline int digitalRead(int) {
  return 0;
}
inline int analogRead(int) {
  return 0;
}

#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1

template <typename T>
inline T constrain(T x, T lo, T hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}
template <typename T>
inline T arduino_min(T a, T b) {
  return a < b ? a : b;
}
template <typename T>
inline T arduino_max(T a, T b) {
  return a > b ? a : b;
}
#define min(a, b) arduino_min((a), (b))
#define max(a, b) arduino_max((a), (b))

#ifndef PI
#define PI 3.14159265358979323846
#endif
#define TWO_PI (2.0 * PI)
#define sq(x) ((x) * (x))
#define radians(d) ((d) * PI / 180.0)
#define degrees(r) ((r) * 180.0 / PI)

// F() macro — no-op on PC
#define F(x) (x)

// --------------- ESP-IDF / FreeRTOS stubs ---------------
// These are only called from functions we skip (setup/loop, sleep, NTP)
inline void esp_deep_sleep_start() {
  exit(0);
}
inline void esp_sleep_enable_timer_wakeup(long long) {}
inline uint8_t esp_read_mac(uint8_t* mac, int) {
  memset(mac, 0, 6);
  return 0;
}
#define ESP_MAC_WIFI_STA 0

// getLocalTime — used in UpdateLocalTime(); we replace the whole function in main_pc.cpp
// but need the declaration to compile
struct tm;
inline bool getLocalTime(struct tm* info, int timeout = 5000) {
  (void)timeout;
  time_t now = time(nullptr);
#ifdef _WIN32
  localtime_s(info, &now);
#else
  localtime_r(&now, info);
#endif
  return true;
}

// ADC stubs (DrawBattery)
typedef int esp_adc_cal_characteristics_t;
typedef int esp_adc_cal_value_t;
typedef int adc_unit_t;
typedef int adc_atten_t;
typedef int adc_bits_width_t;
#define ADC_UNIT_1 0
#define ADC_ATTEN_DB_12 0
#define ADC_WIDTH_BIT_12 0
#define ESP_ADC_CAL_VAL_EFUSE_VREF 0
inline esp_adc_cal_value_t esp_adc_cal_characterize(adc_unit_t, adc_atten_t, adc_bits_width_t, uint32_t,
                                                    esp_adc_cal_characteristics_t*) {
  return 0;
}

// LittleFS stub (only called from config.cpp which we don't compile)
struct LittleFSClass {
  bool begin(bool = false) {
    return false;
  }
} LittleFS;
struct File {
  operator bool() const {
    return false;
  }
  void close() {}
};

// ArduinoJson — used by DecodeWeather, compile normally via the libdeps header
// WiFiClient — replaced by std::istream in the PC build
#include <istream>
using WiFiClient = std::istream;

// setup_portal stub
inline void enterSetupMode() {
  fprintf(stderr, "enterSetupMode() called\n");
  exit(1);
}
inline void DisplaySetupScreen(const char*) {}

// --------------- EPDIY hardware stubs ---------------
// Only the HIGH-LEVEL init/update/power functions need stubbing.
// The actual drawing primitives (epd_draw_pixel etc.) are compiled from epdiy_pc/epdiy.c

// Forward-declare types needed by main.cpp before epdiy.h is included
// (epdiy.h is included via the include path; these stubs extend it)

#endif  // __cplusplus
