#pragma once
// PC stub for ESP-IDF's qrcode.h — backs the esp_qrcode_* API used by
// DrawQR (src/setup_screen.cpp) with the vendored qrcodegen library, since
// the real component is a precompiled ESP-IDF library (libqrcode.a).
//
// Two API details make this a thin pass-through:
//  - esp_qrcode_handle_t is `const uint8_t*`, the same type qrcodegen's
//    qrcode buffers use, so esp_qrcode_get_size/get_module forward directly.
//  - ESP_QRCODE_ECC_{LOW,MED,QUART,HIGH} (0-3) match qrcodegen_Ecc_{LOW,
//    MEDIUM,QUARTILE,HIGH} (0-3) numerically, so the level casts directly.
#include "esp_err.h"
#include "qrcodegen.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const uint8_t* esp_qrcode_handle_t;

typedef struct {
  void (*display_func)(esp_qrcode_handle_t qrcode);
  int max_qrcode_version;
  int qrcode_ecc_level;
} esp_qrcode_config_t;

enum {
  ESP_QRCODE_ECC_LOW,
  ESP_QRCODE_ECC_MED,
  ESP_QRCODE_ECC_QUART,
  ESP_QRCODE_ECC_HIGH,
};

static inline int esp_qrcode_get_size(esp_qrcode_handle_t qrcode) {
  return qrcodegen_getSize(qrcode);
}

static inline bool esp_qrcode_get_module(esp_qrcode_handle_t qrcode, int x, int y) {
  return qrcodegen_getModule(qrcode, x, y);
}

static inline esp_err_t esp_qrcode_generate(esp_qrcode_config_t* cfg, const char* text) {
  uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
  uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
  bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, (enum qrcodegen_Ecc)cfg->qrcode_ecc_level,
                                 qrcodegen_VERSION_MIN, cfg->max_qrcode_version, qrcodegen_Mask_AUTO, true);
  if (!ok) return ESP_FAIL;
  if (cfg->display_func) cfg->display_func(qrcode);
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
