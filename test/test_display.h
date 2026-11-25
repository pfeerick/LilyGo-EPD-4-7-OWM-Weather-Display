#pragma once

namespace {
std::string S(const String& s) {
  return s.c_str();
}

void Wrap(const char* text, String& l1, String& l2) {
  WrapToTwoLines(String(text), 34, l1, l2);
}
}  // namespace

TEST_SUITE("display") {
  TEST_CASE("WrapToTwoLines") {
    String l1, l2;
    SUBCASE("short text stays on one line") {
      Wrap("light rain", l1, l2);
      CHECK(S(l1) == "light rain");
      CHECK(l2.length() == 0);
    }
    SUBCASE("text exactly at the width fits on one line") {
      Wrap("abcdefghij abcdefghij abcdefghij a", l1, l2);
      CHECK(l1.length() == 34);
      CHECK(l2.length() == 0);
    }
    SUBCASE("breaks at the last space that fits") {
      Wrap("moderate rain with occasional heavy showers", l1, l2);
      CHECK(S(l1) == "moderate rain with occasional");
      CHECK(S(l2) == "heavy showers");
    }
    SUBCASE("no space: hard break without losing characters") {
      Wrap("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789xyz", l1, l2);
      CHECK(S(l1) == "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567");
      CHECK(S(l2) == "89xyz");
    }
    SUBCASE("overflow past two lines is truncated with an ellipsis") {
      Wrap("aaaa bbbb cccc dddd eeee ffff gggg hhhh iiii jjjj kkkk llll mmmm nnnn oooo pppp", l1, l2);
      CHECK(l1.length() <= 34);
      CHECK(l2.length() <= 34);
      CHECK(S(l2).size() >= 3);
      CHECK(S(l2).substr(S(l2).size() - 3) == "...");
    }
    SUBCASE("width counts UTF-8 characters, not bytes") {
      std::string word;
      for (int i = 0; i < 20; i++)
        word += "\xC3\xA9";  // é, 2 bytes each
      Wrap((word + " " + word).c_str(), l1, l2);
      CHECK(S(l1) == word);
      CHECK(S(l2) == word);
    }
    SUBCASE("rainfall suffix is wrapped with the description") {
      Wrap("moderate rain with thunder storms (12.3mm)", l1, l2);
      CHECK(S(l1) == "moderate rain with thunder storms");
      CHECK(S(l2) == "(12.3mm)");
    }
  }

  TEST_CASE("calculateGraphYScale") {
    float yMin, yMax, d[16];
    SUBCASE("flat series never yields an empty range") {
      for (float& v : d)
        v = 0.6f;
      calculateGraphYScale(d, 16, yMin, yMax);
      CHECK(yMax > yMin);
      CHECK(yMin <= 0.6f);
      CHECK(yMax >= 0.6f);
    }
    SUBCASE("integer bounds around the data") {
      for (float& v : d)
        v = 20.7f;
      calculateGraphYScale(d, 16, yMin, yMax);
      CHECK(yMin == 20);
      CHECK(yMax == 21);
    }
    SUBCASE("first reading is included") {
      for (int i = 0; i < 16; i++)
        d[i] = 20 + i * 0.1f;
      d[0] = 30.0f;
      calculateGraphYScale(d, 16, yMin, yMax);
      CHECK(yMax >= 30.0f);
    }
    SUBCASE("negative minimum is not clipped") {
      for (int i = 0; i < 16; i++)
        d[i] = -3.2f + i * 0.1f;
      calculateGraphYScale(d, 16, yMin, yMax);
      CHECK(yMin <= -3.2f);
    }
    SUBCASE("inHg-range pressure") {
      for (int i = 0; i < 16; i++)
        d[i] = 29.6f + i * 0.02f;
      calculateGraphYScale(d, 16, yMin, yMax);
      CHECK(yMax > yMin);
      CHECK(yMin <= 29.6f);
      CHECK(yMax >= 29.9f);
    }
  }

  TEST_CASE("UVIndexLevel covers fractional values") {
    CHECK(std::string(UVIndexLevel(0.0f)) == " (L)");
    CHECK(std::string(UVIndexLevel(2.4f)) == " (L)");
    CHECK(std::string(UVIndexLevel(2.6f)) == " (M)");
    CHECK(std::string(UVIndexLevel(5.6f)) == " (H)");
    CHECK(std::string(UVIndexLevel(7.3f)) == " (H)");
    CHECK(std::string(UVIndexLevel(7.6f)) == " (VH)");
    CHECK(std::string(UVIndexLevel(10.4f)) == " (VH)");
    CHECK(std::string(UVIndexLevel(10.5f)) == " (EX)");
    CHECK(std::string(UVIndexLevel(13.0f)) == " (EX)");
  }

  TEST_CASE("WindDegToOrdinalDirection") {
    CHECK(S(WindDegToOrdinalDirection(0)) == S(TXT_N));
    CHECK(S(WindDegToOrdinalDirection(359)) == S(TXT_N));
    CHECK(S(WindDegToOrdinalDirection(90)) == S(TXT_E));
    CHECK(S(WindDegToOrdinalDirection(225)) == S(TXT_SW));
  }

  TEST_CASE("MoonPhase boundaries") {
    CHECK(S(MoonPhase(0.0f)) == S(TXT_MOON_NEW));
    CHECK(S(MoonPhase(0.99f)) == S(TXT_MOON_NEW));
    CHECK(S(MoonPhase(0.25f)) == S(TXT_MOON_FIRST_QUARTER));
    CHECK(S(MoonPhase(0.5f)) == S(TXT_MOON_FULL));
    CHECK(S(MoonPhase(0.75f)) == S(TXT_MOON_THIRD_QUARTER));
    CHECK(S(MoonPhase(0.1f)) == S(TXT_MOON_WAXING_CRESCENT));
  }

  TEST_CASE("full weather render smoke test") {
    wasm_init();
    std::string j = MakeOneCallJson(1010, 1015, 36000, 7.3f);
    CHECK(wasm_render(j.c_str(), (int)j.size()) == 0);
  }
}
