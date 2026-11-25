#pragma once

TEST_SUITE("config") {
  TEST_CASE("SanitizeSleepDuration never returns a value unsafe as a divisor") {
    CHECK(SanitizeSleepDuration(0) == kDefaultSleepDuration);
    CHECK(SanitizeSleepDuration(-5) == kDefaultSleepDuration);
    CHECK(SanitizeSleepDuration(1) == kMinSleepDuration);
    CHECK(SanitizeSleepDuration(kMinSleepDuration) == kMinSleepDuration);
    CHECK(SanitizeSleepDuration(30) == 30);
    CHECK(SanitizeSleepDuration(kMaxSleepDuration) == kMaxSleepDuration);
    CHECK(SanitizeSleepDuration(100000) == kMaxSleepDuration);
  }

  TEST_CASE("SanitizeHour clamps to 0-23") {
    CHECK(SanitizeHour(-1) == 0);
    CHECK(SanitizeHour(0) == 0);
    CHECK(SanitizeHour(8) == 8);
    CHECK(SanitizeHour(23) == 23);
    CHECK(SanitizeHour(24) == 23);
    CHECK(SanitizeHour(1000) == 23);
  }
}
