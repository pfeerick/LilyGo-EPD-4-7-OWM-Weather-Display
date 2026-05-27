// PC-patched version of epdiy.c — all types defined in epdiy_pc.h
#include "epdiy_pc.h"

// --------------- PC display dimensions ---------------
#define EPD_PC_WIDTH 960
#define EPD_PC_HEIGHT 540
#define EPD_PC_FB_SIZE (EPD_PC_WIDTH / 2 * EPD_PC_HEIGHT)

static uint8_t pc_framebuffer[EPD_PC_FB_SIZE];

static enum EpdRotation display_rotation = EPD_ROT_LANDSCAPE;

typedef struct {
  uint16_t x;
  uint16_t y;
} Coord_xy;

#ifndef _swap_int
#define _swap_int(a, b) \
  {                     \
    int t = a;          \
    a = b;              \
    b = t;              \
  }
#endif

// --------------- Hardware stubs ---------------

int epd_width() {
  return EPD_PC_WIDTH;
}
int epd_height() {
  return EPD_PC_HEIGHT;
}
float epd_ambient_temperature() {
  return 25.0f;
}
void epd_poweron() {}
void epd_poweroff() {}
void epd_init(const void* board, const void* disp, int options) {
  (void)board;
  (void)disp;
  (void)options;
}
void epd_deinit() {}
void epd_clear() {}
void epd_clear_area(struct {
  int x;
  int y;
  int width;
  int height;
} area) {
  (void)area;
}
void epd_clear_area_cycles(
    struct {
      int x;
      int y;
      int w;
      int h;
    } area,
    int c, int t) {
  (void)area;
  (void)c;
  (void)t;
}
void epd_set_rotation(enum EpdRotation rotation) {
  display_rotation = rotation;
}
enum EpdRotation epd_get_rotation(void) {
  return display_rotation;
}
int epd_rotated_display_width() {
  return (display_rotation == EPD_ROT_PORTRAIT || display_rotation == EPD_ROT_INVERTED_PORTRAIT) ? EPD_PC_HEIGHT
                                                                                                 : EPD_PC_WIDTH;
}
int epd_rotated_display_height() {
  return (display_rotation == EPD_ROT_PORTRAIT || display_rotation == EPD_ROT_INVERTED_PORTRAIT) ? EPD_PC_WIDTH
                                                                                                 : EPD_PC_HEIGHT;
}
void epd_set_vcom(uint16_t v) {
  (void)v;
}
void epd_set_lcd_pixel_clock_MHz(int f) {
  (void)f;
}
void epd_push_pixels(
    struct {
      int x;
      int y;
      int w;
      int h;
    } area,
    short t, int color) {
  (void)area;
  (void)t;
  (void)color;
}

// High-level API stubs (EpdiyHighlevelState defined in epdiy_pc.h)

EpdiyHighlevelState epd_hl_init(const EpdWaveform* waveform) {
  (void)waveform;
  memset(pc_framebuffer, 0xFF, EPD_PC_FB_SIZE);
  EpdiyHighlevelState state;
  state.front_fb = pc_framebuffer;
  state.back_fb = NULL;
  state.difference_fb = NULL;
  state.dirty_lines = NULL;
  state.waveform = NULL;
  return state;
}

uint8_t* epd_hl_get_framebuffer(EpdiyHighlevelState* state) {
  (void)state;
  return pc_framebuffer;
}

enum EpdDrawError epd_hl_update_screen(EpdiyHighlevelState* state, enum EpdDrawMode mode, int temperature) {
  (void)state;
  (void)mode;
  (void)temperature;
  return EPD_DRAW_SUCCESS;
}
enum EpdDrawError epd_hl_update_area(EpdiyHighlevelState* state, enum EpdDrawMode mode, int temperature, EpdRect area) {
  (void)state;
  (void)mode;
  (void)temperature;
  (void)area;
  return EPD_DRAW_SUCCESS;
}
void epd_hl_set_all_white(EpdiyHighlevelState* state) {
  (void)state;
  memset(pc_framebuffer, 0xFF, EPD_PC_FB_SIZE);
}
void epd_fullclear(EpdiyHighlevelState* state, int temperature) {
  (void)state;
  (void)temperature;
  memset(pc_framebuffer, 0xFF, EPD_PC_FB_SIZE);
}

// EpdRect is defined in epd_internals.h (included via epdiy_pc.h)

EpdRect epd_full_screen() {
  EpdRect area = {0, 0, EPD_PC_WIDTH, EPD_PC_HEIGHT};
  return area;
}

// --------------- Coordinate rotation ---------------
static Coord_xy _rotate(uint16_t x, uint16_t y) {
  switch (display_rotation) {
    case EPD_ROT_PORTRAIT:
      _swap_int(x, y);
      x = EPD_PC_WIDTH - x - 1;
      break;
    case EPD_ROT_INVERTED_LANDSCAPE:
      x = EPD_PC_WIDTH - x - 1;
      y = EPD_PC_HEIGHT - y - 1;
      break;
    case EPD_ROT_INVERTED_PORTRAIT:
      _swap_int(x, y);
      y = EPD_PC_HEIGHT - y - 1;
      break;
    case EPD_ROT_LANDSCAPE:
    default:
      break;
  }
  Coord_xy coord = {x, y};
  return coord;
}

// --------------- Drawing primitives (unchanged from upstream) ---------------

void epd_draw_pixel(int x, int y, uint8_t color, uint8_t* framebuffer) {
  Coord_xy coord = _rotate(x, y);
  x = coord.x;
  y = coord.y;
  if (x < 0 || x >= EPD_PC_WIDTH) return;
  if (y < 0 || y >= EPD_PC_HEIGHT) return;
  uint8_t* buf_ptr = &framebuffer[y * EPD_PC_WIDTH / 2 + x / 2];
  if (x % 2) {
    *buf_ptr = (*buf_ptr & 0x0F) | (color & 0xF0);
  } else {
    *buf_ptr = (*buf_ptr & 0xF0) | (color >> 4);
  }
}

void epd_draw_hline(int x, int y, int length, uint8_t color, uint8_t* framebuffer) {
  for (int i = 0; i < length; i++)
    epd_draw_pixel(x + i, y, color, framebuffer);
}

void epd_draw_vline(int x, int y, int length, uint8_t color, uint8_t* framebuffer) {
  for (int i = 0; i < length; i++)
    epd_draw_pixel(x, y + i, color, framebuffer);
}

void epd_draw_circle(int x0, int y0, int r, uint8_t color, uint8_t* framebuffer) {
  int f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  epd_draw_pixel(x0, y0 + r, color, framebuffer);
  epd_draw_pixel(x0, y0 - r, color, framebuffer);
  epd_draw_pixel(x0 + r, y0, color, framebuffer);
  epd_draw_pixel(x0 - r, y0, color, framebuffer);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    epd_draw_pixel(x0 + x, y0 + y, color, framebuffer);
    epd_draw_pixel(x0 - x, y0 + y, color, framebuffer);
    epd_draw_pixel(x0 + x, y0 - y, color, framebuffer);
    epd_draw_pixel(x0 - x, y0 - y, color, framebuffer);
    epd_draw_pixel(x0 + y, y0 + x, color, framebuffer);
    epd_draw_pixel(x0 - y, y0 + x, color, framebuffer);
    epd_draw_pixel(x0 + y, y0 - x, color, framebuffer);
    epd_draw_pixel(x0 - y, y0 - x, color, framebuffer);
  }
}

void epd_fill_circle_helper(int x0, int y0, int r, int corners, int delta, uint8_t color, uint8_t* framebuffer) {
  int f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r, px = x, py = y;
  delta++;
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < y + 1) {
      if (corners & 1) epd_draw_vline(x0 + x, y0 - y, 2 * y + delta, color, framebuffer);
      if (corners & 2) epd_draw_vline(x0 - x, y0 - y, 2 * y + delta, color, framebuffer);
    }
    if (y != py) {
      if (corners & 1) epd_draw_vline(x0 + py, y0 - px, 2 * px + delta, color, framebuffer);
      if (corners & 2) epd_draw_vline(x0 - py, y0 - px, 2 * px + delta, color, framebuffer);
      py = y;
    }
    px = x;
  }
}

void epd_fill_circle(int x0, int y0, int r, uint8_t color, uint8_t* framebuffer) {
  epd_draw_vline(x0, y0 - r, 2 * r + 1, color, framebuffer);
  epd_fill_circle_helper(x0, y0, r, 3, 0, color, framebuffer);
}

void epd_draw_rect(EpdRect rect, uint8_t color, uint8_t* framebuffer) {
  epd_draw_hline(rect.x, rect.y, rect.width, color, framebuffer);
  epd_draw_hline(rect.x, rect.y + rect.height - 1, rect.width, color, framebuffer);
  epd_draw_vline(rect.x, rect.y, rect.height, color, framebuffer);
  epd_draw_vline(rect.x + rect.width - 1, rect.y, rect.height, color, framebuffer);
}

void epd_fill_rect(EpdRect rect, uint8_t color, uint8_t* framebuffer) {
  for (int i = rect.y; i < rect.y + rect.height; i++)
    epd_draw_hline(rect.x, i, rect.width, color, framebuffer);
}

static void epd_write_line_geom(int x0, int y0, int x1, int y1, uint8_t color, uint8_t* framebuffer) {
  int steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int(x0, y0);
    _swap_int(x1, y1);
  }
  if (x0 > x1) {
    _swap_int(x0, x1);
    _swap_int(y0, y1);
  }
  int dx = x1 - x0, dy = abs(y1 - y0), err = dx / 2, ystep = (y0 < y1) ? 1 : -1;
  for (; x0 <= x1; x0++) {
    if (steep)
      epd_draw_pixel(y0, x0, color, framebuffer);
    else
      epd_draw_pixel(x0, y0, color, framebuffer);
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void epd_draw_line(int x0, int y0, int x1, int y1, uint8_t color, uint8_t* framebuffer) {
  if (x0 == x1) {
    if (y0 > y1) _swap_int(y0, y1);
    epd_draw_vline(x0, y0, y1 - y0 + 1, color, framebuffer);
  } else if (y0 == y1) {
    if (x0 > x1) _swap_int(x0, x1);
    epd_draw_hline(x0, y0, x1 - x0 + 1, color, framebuffer);
  } else {
    epd_write_line_geom(x0, y0, x1, y1, color, framebuffer);
  }
}

void epd_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t* framebuffer) {
  epd_draw_line(x0, y0, x1, y1, color, framebuffer);
  epd_draw_line(x1, y1, x2, y2, color, framebuffer);
  epd_draw_line(x2, y2, x0, y0, color, framebuffer);
}

void epd_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t* framebuffer) {
  int a, b, y, last;
  if (y0 > y1) {
    _swap_int(y0, y1);
    _swap_int(x0, x1);
  }
  if (y1 > y2) {
    _swap_int(y2, y1);
    _swap_int(x2, x1);
  }
  if (y0 > y1) {
    _swap_int(y0, y1);
    _swap_int(x0, x1);
  }
  if (y0 == y2) {
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    epd_draw_hline(a, y0, b - a + 1, color, framebuffer);
    return;
  }
  int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;
  last = (y1 == y2) ? y1 : y1 - 1;
  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b) _swap_int(a, b);
    epd_draw_hline(a, y, b - a + 1, color, framebuffer);
  }
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b) _swap_int(a, b);
    epd_draw_hline(a, y, b - a + 1, color, framebuffer);
  }
}

void epd_copy_to_framebuffer(EpdRect image_area, const uint8_t* image_data, uint8_t* framebuffer) {
  assert(framebuffer != NULL);
  for (uint32_t i = 0; i < (uint32_t)(image_area.width * image_area.height); i++) {
    uint32_t value_index = i;
    if (image_area.width % 2) value_index += i / image_area.width;
    uint8_t val = (value_index % 2) ? (image_data[value_index / 2] & 0xF0) >> 4 : image_data[value_index / 2] & 0x0F;
    int xx = image_area.x + i % image_area.width;
    if (xx < 0 || xx >= EPD_PC_WIDTH) continue;
    int yy = image_area.y + i / image_area.width;
    if (yy < 0 || yy >= EPD_PC_HEIGHT) continue;
    uint8_t* buf_ptr = &framebuffer[yy * EPD_PC_WIDTH / 2 + xx / 2];
    if (xx % 2)
      *buf_ptr = (*buf_ptr & 0x0F) | (val << 4);
    else
      *buf_ptr = (*buf_ptr & 0xF0) | val;
  }
}

uint8_t epd_get_pixel(int x, int y, int fb_width, int fb_height, const uint8_t* framebuffer) {
  if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return 0;
  int fb_width_bytes = fb_width / 2 + fb_width % 2;
  uint8_t buf_val = framebuffer[y * fb_width_bytes + x / 2];
  if (x % 2)
    buf_val = (buf_val & 0xF0) >> 4;
  else
    buf_val = (buf_val & 0x0F);
  return buf_val << 4;
}

static void draw_rotated_transparent_image_impl(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer,
                                                uint8_t* transparent_color) {
  for (uint16_t y = 0; y < (uint16_t)image_area.height; y++) {
    for (uint16_t x = 0; x < (uint16_t)image_area.width; x++) {
      uint16_t xo = image_area.x + x, yo = image_area.y + y;
      if (xo >= (uint16_t)epd_rotated_display_width() || yo >= (uint16_t)epd_rotated_display_height()) continue;
      uint8_t pixel_color = epd_get_pixel(x, y, image_area.width, image_area.height, image_buffer);
      if (transparent_color == NULL || pixel_color != *transparent_color)
        epd_draw_pixel(xo, yo, pixel_color, framebuffer);
    }
  }
}

void epd_draw_rotated_transparent_image(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer,
                                        uint8_t transparent_color) {
  draw_rotated_transparent_image_impl(image_area, image_buffer, framebuffer, &transparent_color);
}

void epd_draw_rotated_image(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer) {
  if (epd_get_rotation() != EPD_ROT_LANDSCAPE)
    draw_rotated_transparent_image_impl(image_area, image_buffer, framebuffer, NULL);
  else
    epd_copy_to_framebuffer(image_area, image_buffer, framebuffer);
}

// epd_draw_base — not called by rendering code
enum EpdDrawError epd_draw_base(EpdRect area, const uint8_t* data, EpdRect crop_to, enum EpdDrawMode mode,
                                int temperature, const bool* drawn_lines, const EpdWaveform* waveform) {
  (void)area;
  (void)data;
  (void)crop_to;
  (void)mode;
  (void)temperature;
  (void)drawn_lines;
  (void)waveform;
  return EPD_DRAW_SUCCESS;
}

EpdRect epd_difference_image(const uint8_t* to, const uint8_t* from, uint8_t* interlaced, bool* dirty_lines) {
  (void)to;
  (void)from;
  (void)interlaced;
  (void)dirty_lines;
  EpdRect r = {0, 0, 0, 0};
  return r;
}
EpdRect epd_difference_image_cropped(const uint8_t* to, const uint8_t* from, EpdRect crop_to, uint8_t* interlaced,
                                     bool* dirty_lines, bool* previously_white, bool* previously_black) {
  (void)to;
  (void)from;
  (void)crop_to;
  (void)interlaced;
  (void)dirty_lines;
  (void)previously_white;
  (void)previously_black;
  EpdRect r = {0, 0, 0, 0};
  return r;
}

// Expose framebuffer directly for main_pc.cpp
uint8_t* epd_pc_get_framebuffer(void) {
  return pc_framebuffer;
}
