#pragma once

#include <string>
#include <cstdio>

// Minimal OWM One Call 3.0 response: `hours` hourly entries whose pressure is `p_now` for the first six hours
// and `p_later` afterwards. Pass tz_offset < -86400 to omit `timezone_offset`.
inline std::string MakeOneCallJson(float p_now = 1013, float p_later = 1013, int tz_offset = 0, float uvi = 2.4f,
                                   int hours = 48) {
  std::string j = "{";
  if (tz_offset >= -86400) j += "\"timezone_offset\":" + std::to_string(tz_offset) + ",";
  char cur[256];
  snprintf(cur, sizeof(cur),
           "\"current\":{\"sunrise\":1700000000,\"sunset\":1700040000,\"temp\":12.5,\"pressure\":%.1f,"
           "\"uvi\":%.2f,\"weather\":[{\"description\":\"light rain\",\"icon\":\"10d\"}]},",
           p_now, uvi);
  j += cur;
  j += "\"daily\":[{\"temp\":{\"min\":8,\"max\":15},\"moon_phase\":0.25}],\"hourly\":[";
  for (int h = 0; h < hours; h++) {
    char buf[160];
    snprintf(buf, sizeof(buf), "%s{\"dt\":%d,\"temp\":%d,\"pressure\":%.1f,\"humidity\":60,\"rain\":{\"1h\":0.6}}",
             h ? "," : "", 1700000000 + h * 3600, 10 + (h % 5), h < 6 ? p_now : p_later);
    j += buf;
  }
  return j + "]}";
}

constexpr int kNoTimezoneOffset = -100000;
