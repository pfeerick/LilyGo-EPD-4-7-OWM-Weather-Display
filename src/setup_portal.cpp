#include "setup_portal.h"
#include "config.h"
#include "config_html.h"
#include "update_html.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Update.h>

static WebServer httpServer(80);
static DNSServer dnsServer;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static String htmlEscape(const std::string& s) {
  String out;
  for (char c : s) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out += c;
    }
  }
  return out;
}

static String buildPage() {
  String page = FPSTR(CONFIG_HTML);
  page.replace("__SSID__", htmlEscape(cfg.ssid));
  page.replace("__PASSWORD__", "");  // never pre-fill password
  page.replace("__APIKEY__", htmlEscape(cfg.apikey));
  page.replace("__SERVER__", htmlEscape(cfg.server));
  page.replace("__CITY__", htmlEscape(cfg.city));
  page.replace("__LATITUDE__", htmlEscape(cfg.latitude));
  page.replace("__LONGITUDE__", htmlEscape(cfg.longitude));
  page.replace("__UNITS_M__", cfg.units == "I" ? "" : "selected");
  page.replace("__UNITS_I__", cfg.units == "I" ? "selected" : "");

  // Language options
  const char* langs[] = {"EN", "AR", "CZ", "EL", "FA", "FR", "GL", "HU",
                         "JA", "KR", "LA", "LT", "MK", "SK", "SL", "VI"};
  for (const char* lang : langs) {
    String token = String("__LANG_") + lang + "__";
    page.replace(token, cfg.language == lang ? "selected" : "");
  }

  page.replace("__TIMEZONE__", htmlEscape(cfg.timezone));
  page.replace("__NTPSERVER__", htmlEscape(cfg.ntp_server));
  page.replace("__GMTOFFSET__", String(cfg.gmt_offset_sec));
  page.replace("__DSTOFFSET__", String(cfg.daylight_offset_sec));
  page.replace("__SLEEPDURATION__", String(cfg.sleep_duration));
  page.replace("__WAKEUPHOUR__", String(cfg.wakeup_hour));
  page.replace("__SLEEPHOUR__", String(cfg.sleep_hour));
  return page;
}

#ifdef ENABLE_CONFIG_DOWNLOAD
static void handleConfigJson() {
  JsonDocument doc;
  doc["ssid"] = cfg.ssid;
#ifndef REDACT_PASSWORD_IN_CONFIG_DOWNLOAD
  doc["password"] = cfg.password;
#endif
#ifndef REDACT_APIKEY_IN_CONFIG_DOWNLOAD
  doc["apikey"] = cfg.apikey;
#endif
  doc["server"] = cfg.server;
  doc["city"] = cfg.city;
  doc["latitude"] = cfg.latitude;
  doc["longitude"] = cfg.longitude;
  doc["language"] = cfg.language;
  doc["units"] = cfg.units;
  doc["timezone"] = cfg.timezone;
  doc["ntp_server"] = cfg.ntp_server;
  doc["gmt_offset_sec"] = cfg.gmt_offset_sec;
  doc["daylight_offset_sec"] = cfg.daylight_offset_sec;
  doc["sleep_duration"] = cfg.sleep_duration;
  doc["wakeup_hour"] = cfg.wakeup_hour;
  doc["sleep_hour"] = cfg.sleep_hour;
  doc["debug_display_update"] = cfg.debug_display_update;
  String body;
  serializeJsonPretty(doc, body);
  httpServer.send(200, "application/json", body);
}
#endif

static void handleRoot() {
  httpServer.send(200, "text/html", buildPage());
}

// Reject and respond 400 if the named form field exceeds a reasonable max length.
#define CHECK_FIELD_LEN(name, maxlen)                            \
  if (httpServer.arg(name).length() > maxlen) {                  \
    httpServer.send(400, "text/plain", "Field too long: " name); \
    return;                                                      \
  }

static void handleSave() {
  auto arg = [](const char* n) { return httpServer.arg(n); };

  CHECK_FIELD_LEN("ssid", 63)
  CHECK_FIELD_LEN("apikey", 63)
  CHECK_FIELD_LEN("server", 63)
  CHECK_FIELD_LEN("city", 63)
  CHECK_FIELD_LEN("latitude", 15)
  CHECK_FIELD_LEN("longitude", 15)
  CHECK_FIELD_LEN("language", 7)
  CHECK_FIELD_LEN("units", 3)
  CHECK_FIELD_LEN("timezone", 79)
  CHECK_FIELD_LEN("ntpServer", 63)

  cfg.ssid = arg("ssid").c_str();

  // Only overwrite password if user typed something
  String pw = arg("password");
  if (pw.length() > 0) {
    if (pw.length() > 63) {
      httpServer.send(400, "text/plain", "Field too long: password");
      return;
    }
    cfg.password = pw.c_str();
  }

  cfg.apikey = arg("apikey").c_str();
  cfg.server = arg("server").c_str();
  cfg.city = arg("city").c_str();
  cfg.latitude = arg("latitude").c_str();
  cfg.longitude = arg("longitude").c_str();
  cfg.language = arg("language").c_str();
  cfg.units = arg("units").c_str();
  cfg.timezone = arg("timezone").c_str();
  cfg.ntpServer = arg("ntpServer").c_str();
  cfg.gmtOffset_sec = arg("gmtOffset_sec").toInt();
  cfg.daylightOffset_sec = arg("daylightOffset_sec").toInt();
  cfg.sleepDuration = arg("sleepDuration").toInt();
  cfg.wakeupHour = arg("wakeupHour").toInt();
  cfg.sleepHour = arg("sleepHour").toInt();

  SaveConfig();

  httpServer.send(200, "text/html",
                  "<html><body style='font-family:sans-serif;max-width:400px;margin:40px auto'>"
                  "<h2>Saved!</h2><p>Device is restarting&hellip;</p></body></html>");
  delay(1000);
  ESP.restart();
}

#undef CHECK_FIELD_LEN

// ---------------------------------------------------------------------------
// OTA update handlers
// ---------------------------------------------------------------------------

static void handleOTAPage() {
  httpServer.send(200, "text/html", FPSTR(UPDATE_HTML));
}

static void handleOTAUpload() {
  static uint32_t lastLogged = 0;
  HTTPUpload& upload = httpServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    lastLogged = 0;
    Serial.printf("OTA upload start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) Update.printError(Serial);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    } else if (upload.totalSize - lastLogged >= 65536) {
      Serial.printf("OTA progress: %u bytes\n", upload.totalSize);
      lastLogged = upload.totalSize;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA success: %u bytes written\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

static void handleOTADone() {
  if (Update.hasError()) {
    httpServer.send(500, "text/plain", Update.errorString());
  } else {
    httpServer.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  }
}

// ---------------------------------------------------------------------------

// Redirect all unknown URLs to root (captive portal behaviour)
static void handleNotFound() {
  Serial.printf("404: %s %s\n", httpServer.method() == HTTP_GET ? "GET" : "POST", httpServer.uri().c_str());
  if (httpServer.method() == HTTP_POST) {
    httpServer.send(404, "text/plain", "Not Found");
    return;
  }
  httpServer.sendHeader("Location", "http://192.168.4.1/", true);
  httpServer.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------

void EnterSetupMode() {
  Serial.println("Entering setup mode");

  // Derive a unique AP name from the last 4 hex digits of MAC
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char apName[24];
  snprintf(apName, sizeof(apName), "WeatherSetup-%02X%02X", mac[4], mac[5]);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);
  Serial.printf("AP started: %s  IP: %s\n", apName, WiFi.softAPIP().toString().c_str());

  // Wildcard DNS — send every hostname to our IP so phones auto-open the portal
  dnsServer.start(53, "*", WiFi.softAPIP());

  httpServer.on("/", HTTP_GET, handleRoot);
  httpServer.on("/save", HTTP_POST, handleSave);
  httpServer.on("/update", HTTP_GET, handleOTAPage);
  httpServer.on("/update", HTTP_POST, handleOTADone, handleOTAUpload);
#ifdef ENABLE_CONFIG_DOWNLOAD
  httpServer.on("/config.json", HTTP_GET, handleConfigJson);
#endif
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();
  Serial.println("Web server started on http://192.168.4.1");

  ArduinoOTA.setHostname("WeatherDisplay");
  ArduinoOTA.onStart([]() { Serial.println("ArduinoOTA start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nArduinoOTA complete"); });
  ArduinoOTA.onProgress(
      [](unsigned int progress, unsigned int total) { Serial.printf("OTA: %u%%\r", progress / (total / 100)); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("ArduinoOTA error[%u]\n", error); });
  ArduinoOTA.begin();
  Serial.println("ArduinoOTA listening (hostname: WeatherDisplay)");

  while (true) {
    ArduinoOTA.handle();
    dnsServer.processNextRequest();
    httpServer.handleClient();
    yield();
  }
}
