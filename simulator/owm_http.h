#pragma once
#include <string>

// Fetch the OWM onecall JSON for the given lat/lon/apikey/server.
// Returns the raw JSON string on success, empty string on failure.
std::string owm_fetch(const char* server, const char* apikey, const char* latitude, const char* longitude,
                      const char* units, const char* language);
