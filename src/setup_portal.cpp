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

static String htmlEscape(const char* s) {
  String out;
  for (const char* p = s; *p; p++) {
    switch (*p) {
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
        out += *p;
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
  page.replace("__HEM_NORTH__", strcmp(cfg.hemisphere, "south") == 0 ? "" : "selected");
  page.replace("__HEM_SOUTH__", strcmp(cfg.hemisphere, "south") == 0 ? "selected" : "");
  page.replace("__UNITS_M__", strcmp(cfg.units, "I") == 0 ? "" : "selected");
  page.replace("__UNITS_I__", strcmp(cfg.units, "I") == 0 ? "selected" : "");

  // Language options
  const char* langs[] = {"EN", "AR", "CZ", "EL", "FA", "FR", "GL", "HU",
                         "JA", "KR", "LA", "LT", "MK", "SK", "SL", "VI"};
  for (const char* lang : langs) {
    String token = String("__LANG_") + lang + "__";
    page.replace(token, strcmp(cfg.language, lang) == 0 ? "selected" : "");
  }

  page.replace("__TIMEZONE__", htmlEscape(cfg.timezone));
  page.replace("__NTPSERVER__", htmlEscape(cfg.ntpServer));
  page.replace("__GMTOFFSET__", String(cfg.gmtOffset_sec));
  page.replace("__DSTOFFSET__", String(cfg.daylightOffset_sec));
  page.replace("__SLEEPDURATION__", String(cfg.sleepDuration));
  page.replace("__WAKEUPHOUR__", String(cfg.wakeupHour));
  page.replace("__SLEEPHOUR__", String(cfg.sleepHour));
  return page;
}

#ifdef ENABLE_CONFIG_DOWNLOAD
static void handleConfigJson() {
  JsonDocument doc;
  doc["ssid"] = cfg.ssid;
#ifndef REDACT_PASSWORD_IN_CONFIG_DOWNLOAD
  doc["password"] = cfg.password;
#endif
  doc["apikey"] = cfg.apikey;
  doc["server"] = cfg.server;
  doc["city"] = cfg.city;
  doc["latitude"] = cfg.latitude;
  doc["longitude"] = cfg.longitude;
  doc["language"] = cfg.language;
  doc["hemisphere"] = cfg.hemisphere;
  doc["units"] = cfg.units;
  doc["timezone"] = cfg.timezone;
  doc["ntpServer"] = cfg.ntpServer;
  doc["gmtOffset_sec"] = cfg.gmtOffset_sec;
  doc["daylightOffset_sec"] = cfg.daylightOffset_sec;
  doc["sleepDuration"] = cfg.sleepDuration;
  doc["wakeupHour"] = cfg.wakeupHour;
  doc["sleepHour"] = cfg.sleepHour;
  doc["debugDisplayUpdate"] = cfg.debugDisplayUpdate;
  String body;
  serializeJsonPretty(doc, body);
  httpServer.send(200, "application/json", body);
}
#endif

static void handleRoot() {
  httpServer.send(200, "text/html", buildPage());
}

static void handleSave() {
  auto arg = [](const char* n) { return httpServer.arg(n); };

  arg("ssid").toCharArray(cfg.ssid, sizeof(cfg.ssid));

  // Only overwrite password if user typed something
  String pw = arg("password");
  if (pw.length() > 0) pw.toCharArray(cfg.password, sizeof(cfg.password));

  arg("apikey").toCharArray(cfg.apikey, sizeof(cfg.apikey));
  arg("server").toCharArray(cfg.server, sizeof(cfg.server));
  arg("city").toCharArray(cfg.city, sizeof(cfg.city));
  arg("latitude").toCharArray(cfg.latitude, sizeof(cfg.latitude));
  arg("longitude").toCharArray(cfg.longitude, sizeof(cfg.longitude));
  arg("language").toCharArray(cfg.language, sizeof(cfg.language));
  arg("hemisphere").toCharArray(cfg.hemisphere, sizeof(cfg.hemisphere));
  arg("units").toCharArray(cfg.units, sizeof(cfg.units));
  arg("timezone").toCharArray(cfg.timezone, sizeof(cfg.timezone));
  arg("ntpServer").toCharArray(cfg.ntpServer, sizeof(cfg.ntpServer));
  cfg.gmtOffset_sec = arg("gmtOffset_sec").toInt();
  cfg.daylightOffset_sec = arg("daylightOffset_sec").toInt();
  cfg.sleepDuration = arg("sleepDuration").toInt();
  cfg.wakeupHour = arg("wakeupHour").toInt();
  cfg.sleepHour = arg("sleepHour").toInt();

  saveConfig();

  httpServer.send(200, "text/html",
                  "<html><body style='font-family:sans-serif;max-width:400px;margin:40px auto'>"
                  "<h2>Saved!</h2><p>Device is restarting&hellip;</p></body></html>");
  delay(1000);
  ESP.restart();
}

// ---------------------------------------------------------------------------
// OTA update handlers
// ---------------------------------------------------------------------------

static void handleOTAPage() {
  httpServer.send(200, "text/html", FPSTR(UPDATE_HTML));
}

static void handleOTAUpload() {
  HTTPUpload& upload = httpServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) Update.printError(Serial);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA success: %u bytes\n", upload.totalSize);
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
  httpServer.sendHeader("Location", "http://192.168.4.1/", true);
  httpServer.send(302, "text/plain", "");
}

// ---------------------------------------------------------------------------

void enterSetupMode() {
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
