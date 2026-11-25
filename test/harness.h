// Pulls the real firmware rendering/parsing code into the test binary via the simulator entry point, which
// supplies Arduino/ESP-IDF/EPDIY stand-ins (simulator/simulator_stubs.h). Must be included exactly once.
#pragma once

#include <cstring>
#include <ctime>

#ifdef _WIN32
// POSIX/BSD functions the firmware relies on that the Windows CRT lacks (Linux CI has them natively).
static inline size_t strlcpy(char* dst, const char* src, size_t size) {
  size_t len = strlen(src);
  if (size) {
    size_t n = len < size - 1 ? len : size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
  }
  return len;
}
static inline struct tm* localtime_r(const time_t* t, struct tm* out) {
  return localtime_s(out, t) == 0 ? out : nullptr;
}
#endif

#include "main_wasm.cpp"
