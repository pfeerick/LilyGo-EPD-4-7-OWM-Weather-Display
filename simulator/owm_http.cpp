#include "owm_http.h"
#include <string>
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

static std::wstring to_wide(const std::string& s) {
  if (s.empty()) return {};
  int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring w(len - 1, 0);
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
  return w;
}

std::string owm_fetch(const char* server, const char* apikey, const char* latitude, const char* longitude,
                      const char* units_str, const char* language) {
  const char* units_param = (strcmp(units_str, "M") == 0) ? "metric" : "imperial";

  char path[512];
  snprintf(path, sizeof(path),
           "/data/3.0/onecall?lat=%s&lon=%s&appid=%s&mode=json&units=%s&lang=%s&exclude=minutely,alerts", latitude,
           longitude, apikey, units_param, language);

  fprintf(stderr, "[owm] GET https://%s%s\n", server, path);

  HINTERNET hSession = WinHttpOpen(L"EPD-Simulator/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) {
    fprintf(stderr, "[owm] WinHttpOpen failed %lu\n", GetLastError());
    return {};
  }

  HINTERNET hConnect = WinHttpConnect(hSession, to_wide(server).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (!hConnect) {
    fprintf(stderr, "[owm] WinHttpConnect failed %lu\n", GetLastError());
    WinHttpCloseHandle(hSession);
    return {};
  }

  HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", to_wide(path).c_str(), nullptr, WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!hRequest) {
    fprintf(stderr, "[owm] WinHttpOpenRequest failed %lu\n", GetLastError());
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return {};
  }

  BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (!sent || !WinHttpReceiveResponse(hRequest, nullptr)) {
    fprintf(stderr, "[owm] Request failed %lu\n", GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return {};
  }

  DWORD status = 0;
  DWORD statusSize = sizeof(status);
  WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                      &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
  if (status != 200) {
    fprintf(stderr, "[owm] HTTP status %lu\n", status);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return {};
  }

  std::string body;
  DWORD bytesAvail = 0;
  while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
    std::string chunk(bytesAvail, '\0');
    DWORD bytesRead = 0;
    WinHttpReadData(hRequest, &chunk[0], bytesAvail, &bytesRead);
    body.append(chunk, 0, bytesRead);
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  fprintf(stderr, "[owm] Received %zu bytes\n", body.size());
  return body;
}

#else
// POSIX fallback using libcurl if needed
#include <curl/curl.h>
static size_t write_cb(char* ptr, size_t size, size_t nmemb, std::string* body) {
  body->append(ptr, size * nmemb);
  return size * nmemb;
}
std::string owm_fetch(const char* server, const char* apikey, const char* latitude, const char* longitude,
                      const char* units_str, const char* language) {
  const char* units_param = (strcmp(units_str, "M") == 0) ? "metric" : "imperial";
  char url[768];
  snprintf(url, sizeof(url),
           "https://%s/data/3.0/onecall?lat=%s&lon=%s&appid=%s&mode=json&units=%s&lang=%s&exclude=minutely,alerts",
           server, latitude, longitude, apikey, units_param, language);
  CURL* curl = curl_easy_init();
  if (!curl) return {};
  std::string body;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "[owm] curl: %s\n", curl_easy_strerror(res));
    return {};
  }
  return body;
}
#endif
