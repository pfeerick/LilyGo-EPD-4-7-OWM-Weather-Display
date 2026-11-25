#pragma once

namespace {
// Restores the global config fields the tests touch.
struct ConfigGuard {
  std::string units = cfg.units;
  int gmt = cfg.gmt_offset_sec, dst = cfg.daylight_offset_sec;
  ~ConfigGuard() {
    cfg.units = units;
    cfg.gmt_offset_sec = gmt;
    cfg.daylight_offset_sec = dst;
  }
};
}  // namespace

TEST_SUITE("weather_api") {
  TEST_CASE("unit conversions") {
    CHECK(MmToInches(25.4f) == doctest::Approx(1.0f).epsilon(0.001));
    CHECK(HpaToInhg(1013.25f) == doctest::Approx(29.92f).epsilon(0.001));
  }

  TEST_CASE("TitleCase capitalises only the first character") {
    CHECK(std::string(TitleCase("light rain").c_str()) == "Light rain");
    CHECK(TitleCase("").length() == 0);
  }

  TEST_CASE("SumOfPrecip") {
    float d[] = {0.5f, 1.0f, 0.0f, 2.5f};
    CHECK(SumOfPrecip(d, 4) == doctest::Approx(4.0f));
  }

  TEST_CASE("ParseWeatherDoc pressure trend direction") {
    ConfigGuard guard;
    cfg.units = "M";
    SUBCASE("rising") {
      REQUIRE(DecodeWeather(MakeOneCallJson(1010, 1015), "onecall"));
      CHECK(wx_conditions.trend == '+');
    }
    SUBCASE("falling") {
      REQUIRE(DecodeWeather(MakeOneCallJson(1015, 1010), "onecall"));
      CHECK(wx_conditions.trend == '-');
    }
    SUBCASE("steady") {
      REQUIRE(DecodeWeather(MakeOneCallJson(1012, 1012), "onecall"));
      CHECK(wx_conditions.trend == '0');
    }
  }

  TEST_CASE("ParseWeatherDoc samples every third hour") {
    ConfigGuard guard;
    cfg.units = "M";
    REQUIRE(DecodeWeather(MakeOneCallJson(), "onecall"));
    for (int i = 0; i < 16; i++)
      CHECK(wx_forecast[i].dt == 1700000000 + i * 3 * 3600);
  }

  TEST_CASE("imperial mode converts current and forecast pressure and precipitation") {
    ConfigGuard guard;
    cfg.units = "I";
    REQUIRE(DecodeWeather(MakeOneCallJson(1013, 1013), "onecall"));
    CHECK(wx_conditions.pressure == doctest::Approx(29.91f).epsilon(0.001));
    for (int i = 0; i < 16; i++) {
      CHECK(wx_forecast[i].pressure == doctest::Approx(29.91f).epsilon(0.001));
      CHECK(wx_forecast[i].rainfall == doctest::Approx(0.0236f).epsilon(0.01));
    }
  }

  TEST_CASE("ConvertUnixTime uses OWM timezone_offset, falling back to configured offsets") {
    ConfigGuard guard;
    cfg.units = "M";
    cfg.gmt_offset_sec = 3600;
    cfg.daylight_offset_sec = 0;
    // 1700000000 = 2023-11-14 22:13:20 UTC
    SUBCASE("timezone_offset present") {
      REQUIRE(DecodeWeather(MakeOneCallJson(1013, 1013, 36000), "onecall"));
      CHECK(wx_conditions.timezone_offset == 36000);
      CHECK(std::string(ConvertUnixTime(1700000000).c_str()) == "08:13 15/11/23");
    }
    SUBCASE("timezone_offset absent") {
      REQUIRE(DecodeWeather(MakeOneCallJson(1013, 1013, kNoTimezoneOffset), "onecall"));
      CHECK(std::string(ConvertUnixTime(1700000000).c_str()) == "23:13 14/11/23");
    }
#ifndef _WIN32  // "%P" (lowercase am/pm) is a glibc/newlib extension; the Windows CRT aborts on it
    SUBCASE("imperial format") {
      cfg.units = "I";
      REQUIRE(DecodeWeather(MakeOneCallJson(1013, 1013, 0), "onecall"));
      CHECK(std::string(ConvertUnixTime(1700000000).c_str()) == "10:13pm 11/14/23");
    }
#endif
  }

  TEST_CASE("DecodeWeather rejects malformed JSON") {
    CHECK_FALSE(DecodeWeather(std::string("{\"current\":"), "onecall"));
  }
}
