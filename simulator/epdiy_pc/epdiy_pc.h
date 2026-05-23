#pragma once
#ifndef EPDIY_PC_H
#define EPDIY_PC_H
// Self-contained PC compatibility header for epdiy_pc/epdiy.c and epdiy_pc/font.c.
// Defines all EPDIY types needed by the drawing code without pulling in ESP-IDF headers.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

// ESP-IDF log macros → stderr
#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGD(tag, fmt, ...)
#define IRAM_ATTR

// heap_caps_malloc → regular malloc
#define MALLOC_CAP_SPIRAM 0
static inline void* heap_caps_malloc(size_t size, int caps) { (void)caps; return malloc(size); }

// strsep for Windows
#ifdef _WIN32
static inline char* strsep(char** stringp, const char* delim) {
  char* start = *stringp;
  if (!start) return NULL;
  char* p = start;
  while (*p && !strchr(delim, *p)) p++;
  if (*p) { *p = '\0'; *stringp = p + 1; }
  else     { *stringp = NULL; }
  return start;
}
#define strdup _strdup
#endif

// ---- EPDIY types (copied from epd_internals.h / epdiy.h — no ESP-IDF deps) ----

typedef struct {
  int x;
  int y;
  int width;
  int height;
} EpdRect;

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t advance_x;
  int16_t left;
  int16_t top;
  uint32_t compressed_size;
  uint32_t data_offset;
} EpdGlyph;

typedef struct {
  uint32_t first;
  uint32_t last;
  uint32_t offset;
} EpdUnicodeInterval;

typedef struct {
  const uint8_t *bitmap;
  const EpdGlyph *glyph;
  const EpdUnicodeInterval *intervals;
  uint32_t interval_count;
  bool compressed;
  uint16_t advance_y;
  int ascender;
  int descender;
} EpdFont;

enum EpdFontFlags {
  EPD_DRAW_BACKGROUND   = 0x1,
  EPD_DRAW_ALIGN_LEFT   = 0x2,
  EPD_DRAW_ALIGN_RIGHT  = 0x4,
  EPD_DRAW_ALIGN_CENTER = 0x8,
};

typedef struct {
  uint8_t fg_color : 4;
  uint8_t bg_color : 4;
  uint32_t fallback_glyph;
  enum EpdFontFlags flags;
} EpdFontProperties;

enum EpdDrawError {
  EPD_DRAW_SUCCESS                = 0x0,
  EPD_DRAW_INVALID_PACKING_MODE   = 0x1,
  EPD_DRAW_LOOKUP_NOT_IMPLEMENTED = 0x2,
  EPD_DRAW_STRING_INVALID         = 0x4,
  EPD_DRAW_NO_DRAWABLE_CHARACTERS = 0x8,
  EPD_DRAW_FAILED_ALLOC           = 0x10,
  EPD_DRAW_GLYPH_FALLBACK_FAILED  = 0x20,
  EPD_DRAW_INVALID_CROP           = 0x40,
  EPD_DRAW_MODE_NOT_FOUND         = 0x80,
  EPD_DRAW_NO_PHASES_AVAILABLE    = 0x100,
  EPD_DRAW_INVALID_FONT_FLAGS     = 0x200,
  EPD_DRAW_EMPTY_LINE_QUEUE       = 0x400,
};

enum EpdDrawMode {
  MODE_GL16 = 0x5,
  EPD_MODE_DEFAULT = MODE_GL16,
};

enum EpdRotation {
  EPD_ROT_LANDSCAPE          = 0,
  EPD_ROT_PORTRAIT           = 1,
  EPD_ROT_INVERTED_LANDSCAPE = 2,
  EPD_ROT_INVERTED_PORTRAIT  = 3,
};

// Waveform placeholder (not used in PC drawing code)
typedef struct { int dummy; } EpdWaveform;

// High-level state struct (mirrors epd_highlevel.h)
typedef struct {
  uint8_t* front_fb;
  uint8_t* back_fb;
  uint8_t* difference_fb;
  bool*    dirty_lines;
  const EpdWaveform* waveform;
} EpdiyHighlevelState;

#ifdef __cplusplus
extern "C" {
#endif

// Drawing primitives (implemented in epdiy_pc/epdiy.c)
void     epd_draw_pixel(int x, int y, uint8_t color, uint8_t *framebuffer);
void     epd_draw_hline(int x, int y, int length, uint8_t color, uint8_t *framebuffer);
void     epd_draw_vline(int x, int y, int length, uint8_t color, uint8_t *framebuffer);
void     epd_draw_line(int x0, int y0, int x1, int y1, uint8_t color, uint8_t *framebuffer);
void     epd_draw_circle(int x, int y, int r, uint8_t color, uint8_t *framebuffer);
void     epd_fill_circle(int x, int y, int r, uint8_t color, uint8_t *framebuffer);
void     epd_fill_circle_helper(int x0, int y0, int r, int corners, int delta, uint8_t color, uint8_t *framebuffer);
void     epd_draw_rect(EpdRect rect, uint8_t color, uint8_t *framebuffer);
void     epd_fill_rect(EpdRect rect, uint8_t color, uint8_t *framebuffer);
void     epd_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t *framebuffer);
void     epd_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t *framebuffer);
void     epd_copy_to_framebuffer(EpdRect image_area, const uint8_t *image_data, uint8_t *framebuffer);
uint8_t  epd_get_pixel(int x, int y, int fb_width, int fb_height, const uint8_t *framebuffer);
void     epd_draw_rotated_image(EpdRect image_area, const uint8_t *image_buffer, uint8_t *framebuffer);
void     epd_draw_rotated_transparent_image(EpdRect image_area, const uint8_t *image_buffer, uint8_t *framebuffer, uint8_t transparent_color);
int      epd_width(void);
int      epd_height(void);
float    epd_ambient_temperature(void);
enum EpdRotation epd_get_rotation(void);
void     epd_set_rotation(enum EpdRotation rotation);
int      epd_rotated_display_width(void);
int      epd_rotated_display_height(void);
EpdRect  epd_full_screen(void);
void     epd_poweron(void);
void     epd_poweroff(void);

// High-level API (implemented in epdiy_pc/epdiy.c)
EpdiyHighlevelState      epd_hl_init(const EpdWaveform *waveform);
uint8_t*                 epd_hl_get_framebuffer(EpdiyHighlevelState *state);
void                     epd_hl_set_all_white(EpdiyHighlevelState *state);
void                     epd_fullclear(EpdiyHighlevelState *state, int temperature);
enum EpdDrawError        epd_hl_update_screen(EpdiyHighlevelState *state, enum EpdDrawMode mode, int temperature);
uint8_t*                 epd_pc_get_framebuffer(void);

// Function declarations implemented in font.c
EpdFontProperties           epd_font_properties_default(void);
const EpdGlyph*             epd_get_glyph(const EpdFont *font, uint32_t code_point);
enum EpdDrawError           epd_write_default(const EpdFont *font, const char *string, int *cursor_x, int *cursor_y, uint8_t *framebuffer);
enum EpdDrawError           epd_write_string(const EpdFont *font, const char *string, int *cursor_x, int *cursor_y, uint8_t *framebuffer, const EpdFontProperties *properties);
void                        epd_get_text_bounds(const EpdFont *font, const char *string, const int *x, const int *y, int *x1, int *y1, int *w, int *h, const EpdFontProperties *properties);
EpdRect                     epd_get_string_rect(const EpdFont *font, const char *string, int x, int y, int margin, const EpdFontProperties *properties);

#ifdef __cplusplus
}
#endif

#endif // EPDIY_PC_H
