// PC-patched version of font.c — all types from epdiy_pc.h, no upstream headers needed.
#include "epdiy_pc.h"
#include "../miniz/miniz.h"
void epd_draw_hline(int x, int y, int length, uint8_t color, uint8_t* framebuffer);

// ---- Everything below is verbatim from upstream font.c ----

typedef struct {
  uint8_t mask;
  uint8_t lead;
  uint32_t beg;
  uint32_t end;
  int bits_stored;
} utf_t;

static utf_t* utf[] = {
    [0] = &(utf_t){0b00111111, 0b10000000, 0, 0, 6},
    [1] = &(utf_t){0b01111111, 0b00000000, 0000, 0177, 7},
    [2] = &(utf_t){0b00011111, 0b11000000, 0200, 03777, 5},
    [3] = &(utf_t){0b00001111, 0b11100000, 04000, 0177777, 4},
    [4] = &(utf_t){0b00000111, 0b11110000, 0200000, 04177777, 3},
    &(utf_t){0},
};

static inline int font_min(int x, int y) {
  return x < y ? x : y;
}
static inline int font_max(int x, int y) {
  return x > y ? x : y;
}

static int utf8_len(const uint8_t ch) {
  int len = 0;
  for (utf_t** u = utf; (*u)->mask; ++u) {
    if ((ch & ~(*u)->mask) == (*u)->lead) break;
    ++len;
  }
  assert(len <= 4);
  return len;
}

static uint32_t next_cp(const uint8_t** string) {
  if (**string == 0) return 0;
  int bytes = utf8_len(**string);
  const uint8_t* chr = *string;
  *string += bytes;
  int shift = utf[0]->bits_stored * (bytes - 1);
  uint32_t codep = (*chr++ & utf[bytes]->mask) << shift;
  for (int i = 1; i < bytes; ++i, ++chr) {
    shift -= utf[0]->bits_stored;
    codep |= ((const uint8_t)*chr & utf[0]->mask) << shift;
  }
  return codep;
}

EpdFontProperties epd_font_properties_default() {
  EpdFontProperties props = {.fg_color = 0, .bg_color = 15, .fallback_glyph = 0, .flags = EPD_DRAW_ALIGN_LEFT};
  return props;
}

const EpdGlyph* epd_get_glyph(const EpdFont* font, uint32_t code_point) {
  const EpdUnicodeInterval* intervals = font->intervals;
  for (int i = 0; i < (int)font->interval_count; i++) {
    const EpdUnicodeInterval* interval = &intervals[i];
    if (code_point >= interval->first && code_point <= interval->last)
      return &font->glyph[interval->offset + (code_point - interval->first)];
    if (code_point < interval->first) return NULL;
  }
  return NULL;
}

static int uncompress_font(uint8_t* dest, size_t uncompressed_size, const uint8_t* source, size_t source_size) {
  if (uncompressed_size == 0 || !dest || source_size == 0 || !source) return -1;
  tinfl_decompressor* decomp = malloc(sizeof(tinfl_decompressor));
  if (!decomp) return -1;
  tinfl_init(decomp);
  tinfl_status status = tinfl_decompress(decomp, source, &source_size, dest, dest, &uncompressed_size,
                                         TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  free(decomp);
  return (status == TINFL_STATUS_DONE) ? 0 : (int)status;
}

static enum EpdDrawError draw_char(const EpdFont* font, uint8_t* buffer, int* cursor_x, int cursor_y, uint32_t cp,
                                   const EpdFontProperties* props) {
  assert(props != NULL);
  const EpdGlyph* glyph = epd_get_glyph(font, cp);
  if (!glyph) glyph = epd_get_glyph(font, props->fallback_glyph);
  if (!glyph) return EPD_DRAW_GLYPH_FALLBACK_FAILED;

  uint32_t offset = glyph->data_offset;
  uint16_t width = glyph->width, height = glyph->height;
  int left = glyph->left;
  int byte_width = (width / 2 + width % 2);
  unsigned long bitmap_size = byte_width * height;
  const uint8_t* bitmap = NULL;

  if (bitmap_size > 0 && font->compressed) {
    uint8_t* tmp = (uint8_t*)malloc(bitmap_size);
    if (!tmp) return EPD_DRAW_FAILED_ALLOC;
    uncompress_font(tmp, bitmap_size, &font->bitmap[offset], glyph->compressed_size);
    bitmap = tmp;
  } else {
    bitmap = &font->bitmap[offset];
  }

  uint8_t color_lut[16];
  for (int c = 0; c < 16; c++) {
    int diff = (int)props->fg_color - (int)props->bg_color;
    color_lut[c] = (uint8_t)font_max(0, font_min(15, props->bg_color + c * diff / 15));
  }
  bool background_needed = (props->flags & EPD_DRAW_BACKGROUND) != 0;

  for (int y = 0; y < height; y++) {
    int yy = cursor_y - glyph->top + y;
    int start_pos = *cursor_x + left;
    int x = font_max(0, -start_pos);
    int max_x = start_pos + width;
    for (int xx = start_pos; xx < max_x; xx++) {
      uint8_t bm = bitmap[y * byte_width + x / 2];
      bm = (x & 1) ? (bm >> 4) : (bm & 0xF);
      if (background_needed || bm) epd_draw_pixel(xx, yy, color_lut[bm] << 4, buffer);
      x++;
    }
  }
  if (bitmap_size > 0 && font->compressed) free((uint8_t*)bitmap);
  *cursor_x += glyph->advance_x;
  return EPD_DRAW_SUCCESS;
}

static void get_char_bounds(const EpdFont* font, uint32_t cp, int* x, int* y, int* minx, int* miny, int* maxx,
                            int* maxy, const EpdFontProperties* props) {
  assert(props != NULL);
  const EpdGlyph* glyph = epd_get_glyph(font, cp);
  if (!glyph) glyph = epd_get_glyph(font, props->fallback_glyph);
  if (!glyph) return;
  int x1 = *x + glyph->left, y1 = *y + glyph->top - glyph->height, x2 = x1 + glyph->width, y2 = y1 + glyph->height;
  if (props->flags & EPD_DRAW_BACKGROUND) {
    *minx = font_min(*x, font_min(*minx, x1));
    *maxx = font_max(font_max(*x + glyph->advance_x, x2), *maxx);
    *miny = font_min(*y + font->descender, font_min(*miny, y1));
    *maxy = font_max(*y + font->ascender, font_max(*maxy, y2));
  } else {
    if (x1 < *minx) *minx = x1;
    if (y1 < *miny) *miny = y1;
    if (x2 > *maxx) *maxx = x2;
    if (y2 > *maxy) *maxy = y2;
  }
  *x += glyph->advance_x;
}

EpdRect epd_get_string_rect(const EpdFont* font, const char* string, int x, int y, int margin,
                            const EpdFontProperties* properties) {
  assert(properties != NULL);
  EpdFontProperties props = *properties;
  props.flags |= EPD_DRAW_BACKGROUND;
  EpdRect temp = {x, y, 0, 0};
  if (*string == '\0') return temp;
  int minx = 100000, miny = 100000, maxx = -1, maxy = -1;
  int tx = x, ty = y + font->ascender;
  uint32_t c;
  while ((c = next_cp((const uint8_t**)&string))) {
    if (c == 0x000A) {
      tx = x;
      ty += font->advance_y;
    } else
      get_char_bounds(font, c, &tx, &ty, &minx, &miny, &maxx, &maxy, &props);
  }
  temp.width = maxx - x + margin * 2;
  temp.height = maxy - miny + margin * 2;
  return temp;
}

void epd_get_text_bounds(const EpdFont* font, const char* string, const int* x, const int* y, int* x1, int* y1, int* w,
                         int* h, const EpdFontProperties* properties) {
  assert(properties != NULL);
  EpdFontProperties props = *properties;
  if (*string == '\0') {
    *w = 0;
    *h = 0;
    *y1 = *y;
    *x1 = *x;
    return;
  }
  int minx = 100000, miny = 100000, maxx = -1, maxy = -1;
  int original_x = *x, tx = *x, ty = *y;
  uint32_t c;
  while ((c = next_cp((const uint8_t**)&string)))
    get_char_bounds(font, c, &tx, &ty, &minx, &miny, &maxx, &maxy, &props);
  *x1 = font_min(original_x, minx);
  *w = maxx - *x1;
  *y1 = miny;
  *h = maxy - miny;
}

static enum EpdDrawError epd_write_line_font(const EpdFont* font, const char* string, int* cursor_x, int* cursor_y,
                                             uint8_t* framebuffer, const EpdFontProperties* properties) {
  assert(framebuffer != NULL);
  if (*string == '\0') return EPD_DRAW_SUCCESS;
  assert(properties != NULL);
  EpdFontProperties props = *properties;
  int alignment_mask = EPD_DRAW_ALIGN_LEFT | EPD_DRAW_ALIGN_RIGHT | EPD_DRAW_ALIGN_CENTER;
  int alignment = props.flags & alignment_mask;
  if ((alignment & (alignment - 1)) != 0) return EPD_DRAW_INVALID_FONT_FLAGS;

  int x1 = 0, y1 = 0, w = 0, h = 0;
  int tx = *cursor_x, ty = *cursor_y;
  epd_get_text_bounds(font, string, &tx, &ty, &x1, &y1, &w, &h, &props);
  if (w < 0 || h < 0) return EPD_DRAW_NO_DRAWABLE_CHARACTERS;

  int local_x = *cursor_x, local_y = *cursor_y;
  int cursor_x_init = local_x;
  switch (alignment) {
    case EPD_DRAW_ALIGN_CENTER:
      local_x -= w / 2;
      break;
    case EPD_DRAW_ALIGN_RIGHT:
      local_x -= w;
      break;
    default:
      break;
  }
  if (props.flags & EPD_DRAW_BACKGROUND) {
    for (int l = local_y - font->ascender; l < local_y - font->descender; l++)
      epd_draw_hline(local_x, l, w, (uint8_t)(props.bg_color << 4), framebuffer);
  }
  enum EpdDrawError err = EPD_DRAW_SUCCESS;
  uint32_t c;
  while ((c = next_cp((const uint8_t**)&string)))
    err = (enum EpdDrawError)(err | draw_char(font, framebuffer, &local_x, local_y, c, &props));
  *cursor_x += local_x - cursor_x_init;
  return err;
}

enum EpdDrawError epd_write_default(const EpdFont* font, const char* string, int* cursor_x, int* cursor_y,
                                    uint8_t* framebuffer) {
  const EpdFontProperties props = epd_font_properties_default();
  return epd_write_string(font, string, cursor_x, cursor_y, framebuffer, &props);
}

enum EpdDrawError epd_write_string(const EpdFont* font, const char* string, int* cursor_x, int* cursor_y,
                                   uint8_t* framebuffer, const EpdFontProperties* properties) {
  if (!string) {
    ESP_LOGE("font.c", "cannot draw a NULL string!");
    return EPD_DRAW_STRING_INVALID;
  }
  char* tofree = strdup(string);
  char* newstring = tofree;
  if (!newstring) {
    ESP_LOGE("font.c", "cannot allocate string copy!");
    return EPD_DRAW_FAILED_ALLOC;
  }
  enum EpdDrawError err = EPD_DRAW_SUCCESS;
  int line_start = *cursor_x;
  char* token;
  while ((token = strsep(&newstring, "\n")) != NULL) {
    *cursor_x = line_start;
    err = (enum EpdDrawError)(err | epd_write_line_font(font, token, cursor_x, cursor_y, framebuffer, properties));
    *cursor_y += font->advance_y;
  }
  free(tofree);
  return err;
}
