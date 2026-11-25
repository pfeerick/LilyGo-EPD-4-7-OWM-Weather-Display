// Native unit tests. Build with CMake (see CONTRIBUTING.md); everything is compiled as a single translation
// unit so file-static helpers in src/*.cpp are reachable from the tests.
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "harness.h"
#include "fixtures.h"

#include "test_config.h"
#include "test_weather_api.h"
#include "test_display.h"

#include <cstdlib>

int main(int argc, char** argv) {
  // The firmware's Serial logging goes to stderr via the simulator stubs; doctest reports on stdout.
  // Keep test output readable unless TEST_VERBOSE is set.
  if (!std::getenv("TEST_VERBOSE")) {
#ifdef _WIN32
    freopen("NUL", "w", stderr);
#else
    freopen("/dev/null", "w", stderr);
#endif
  }
  return doctest::Context(argc, argv).run();
}
