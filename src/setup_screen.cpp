#include "setup_screen.h"
#include "build_info.h"
#include "display.h"
#include "qrcode.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"

static int qr_origin_x, qr_origin_y, qr_mod_size, qr_rendered_px;

static int DrawQR(const char* text, int originX, int originY, int moduleSize, int eccLevel = ESP_QRCODE_ECC_LOW) {
  qr_origin_x = originX;
  qr_origin_y = originY;
  qr_mod_size = moduleSize;
  qr_rendered_px = 0;

  esp_qrcode_config_t cfg = {
      .display_func =
          [](esp_qrcode_handle_t qrcode) {
            int size = esp_qrcode_get_size(qrcode);
            qr_rendered_px = size * qr_mod_size;
            int border = qr_mod_size * 2;
            FillRect(qr_origin_x - border, qr_origin_y - border, qr_rendered_px + border * 2,
                     qr_rendered_px + border * 2, kWhite);
            for (int y = 0; y < size; y++) {
              for (int x = 0; x < size; x++) {
                if (esp_qrcode_get_module(qrcode, x, y)) {
                  FillRect(qr_origin_x + x * qr_mod_size, qr_origin_y + y * qr_mod_size, qr_mod_size, qr_mod_size,
                           kBlack);
                }
              }
            }
          },
      .max_qrcode_version = 10,
      .qrcode_ecc_level = eccLevel,
  };

  esp_qrcode_generate(&cfg, text);
  return qr_rendered_px;
}

void DisplaySetupScreen(const char* ap_name) {
  epd_poweron();
  epd_fullclear(&epd_hl, (int)epd_ambient_temperature());

  int cx = epd_width() / 2;
  int w = epd_width();
  int ms = 6;
  int pad = 16;

  int qrPx = 33 * ms;
  int qrCxL = pad + qrPx / 2;
  int qrCxR = w - pad - qrPx / 2;

  char wifiQR[64];
  snprintf(wifiQR, sizeof(wifiQR), "WIFI:S:%s;T:nopass;;", ap_name);
  int wifiQRpx = DrawQR(wifiQR, pad, pad, ms, ESP_QRCODE_ECC_LOW);

  int rightQRx = w - pad - qrPx + (ms * 2);
  int portalQRpx = DrawQR("http://192.168.4.1", rightQRx, pad, ms, ESP_QRCODE_ECC_HIGH);

  SetFont(OpenSans12B);
  int labelGap = 24;
  int wifiGap = 34;
  int portalGap = 28;
  DrawString(qrCxL, pad + wifiQRpx + labelGap, "Scan to join", Alignment::kCenter);
  DrawString(qrCxL, pad + wifiQRpx + labelGap + wifiGap, "WiFi network", Alignment::kCenter);
  DrawString(qrCxR, pad + portalQRpx + labelGap, "Scan to open", Alignment::kCenter);
  DrawString(qrCxR, pad + portalQRpx + labelGap + portalGap, "config portal", Alignment::kCenter);

  int blockH = 270;
  int textY = (epd_height() - blockH) / 2;

  // Halve the whitespace above "SETUP MODE" to balance the block against the build-info footer
  int topMargin = textY - 48;
  textY -= topMargin / 2;

  SetFont(OpenSans18B);
  DrawString(cx, textY - 48, "SETUP MODE", Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 48, "Connect to WiFi network:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 80, String(ap_name), Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 140, "Then open in a browser:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 172, "http://192.168.4.1", Alignment::kCenter);
  SetFont(OpenSans12B);
  DrawString(cx, textY + 228, "To update firmware:", Alignment::kCenter);
  SetFont(OpenSans18B);
  DrawString(cx, textY + 260, "http://192.168.4.1/update", Alignment::kCenter);

  SetFont(OpenSans12B);
  DrawString(cx, epd_height() - 70, BUILD_INFO, Alignment::kCenter);
  DrawString(cx, epd_height() - 36, BUILD_TIME, Alignment::kCenter);

  EdpUpdate();
  epd_poweroff();
}
