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
}
