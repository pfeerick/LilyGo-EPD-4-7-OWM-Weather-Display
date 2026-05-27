#include "icons.h"
#include "fonts/opensans8b.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"
#include "fonts/opensans24b.h"

void AddCloud(int x, int y, int scale, int linesize) {
  FillCircle(x - scale * 3, y, scale, kBlack);
  FillCircle(x + scale * 3, y, scale, kBlack);
  FillCircle(x - scale, y - scale, scale * 1.4, kBlack);
  FillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, kBlack);
  FillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, kBlack);
  FillCircle(x - scale * 3, y, scale - linesize, kWhite);
  FillCircle(x + scale * 3, y, scale - linesize, kWhite);
  FillCircle(x - scale, y - scale, scale * 1.4 - linesize, kWhite);
  FillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, kWhite);
  FillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, kWhite);
}

void AddRain(int x, int y, int scale, bool icon_size) {
  if (icon_size == small_icon) {
    SetFont(OpenSans8B);
    DrawString(x - 25, y + 12, "///////", Alignment::kLeft);
  } else {
    SetFont(OpenSans18B);
    DrawString(x - 60, y + 25, "///////", Alignment::kLeft);
  }
}

void AddSnow(int x, int y, int scale, bool icon_size) {
  if (icon_size == small_icon) {
    SetFont(OpenSans8B);
    DrawString(x - 25, y + 15, "* * * *", Alignment::kLeft);
  } else {
    SetFont(OpenSans18B);
    DrawString(x - 60, y + 30, "* * * *", Alignment::kLeft);
  }
}

void AddTstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 1; i < 5; i++) {
    DrawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, kBlack);
    DrawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, kBlack);
    DrawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, kBlack);
    DrawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, kBlack);
    DrawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, kBlack);
    DrawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, kBlack);
    DrawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, kBlack);
    DrawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, kBlack);
    DrawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, kBlack);
  }
}

void AddSun(int x, int y, int scale, bool icon_size) {
  int linesize = 5;
  FillRect(x - scale * 2, y, scale * 4, linesize, kBlack);
  FillRect(x, y - scale * 2, linesize, scale * 4, kBlack);
  DrawAngledLine(x + scale * 1.4, y + scale * 1.4, (x - scale * 1.4), (y - scale * 1.4), linesize * 1.5, kBlack);
  DrawAngledLine(x - scale * 1.4, y + scale * 1.4, (x + scale * 1.4), (y - scale * 1.4), linesize * 1.5, kBlack);
  FillCircle(x, y, scale * 1.3, kWhite);
  FillCircle(x, y, scale, kBlack);
  FillCircle(x, y, scale - linesize, kWhite);
}

void AddFog(int x, int y, int scale, int linesize, bool icon_size) {
  if (icon_size == small_icon) linesize = 3;
  FillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, kBlack);
  FillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, kBlack);
  FillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, kBlack);
}

void DrawAngledLine(int x, int y, int x1, int y1, int size, int color) {
  int dx = (size / 2.0) * (x - x1) / sqrt(sq(x - x1) + sq(y - y1));
  int dy = (size / 2.0) * (y - y1) / sqrt(sq(x - x1) + sq(y - y1));
  FillTriangle(x + dx, y - dy, x - dx, y + dy, x1 + dx, y1 - dy, color);
  FillTriangle(x - dx, y + dy, x1 - dx, y1 + dy, x1 + dx, y1 - dy, color);
}

void ClearSky(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  if (icon_size == large_icon) scale = kLarge;
  y += (icon_size ? 0 : 10);
  AddSun(x, y, scale * (icon_size ? 1.7 : 1.2), icon_size);
}

void BrokenClouds(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  y += 15;
  if (icon_size == large_icon) scale = kLarge;
  AddSun(x - scale * 1.8, y - scale * 1.8, scale, icon_size);
  AddCloud(x, y, scale * (icon_size ? 1 : 0.75), linesize);
}

void FewClouds(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  y += 15;
  if (icon_size == large_icon) scale = kLarge;
  AddCloud(x + (icon_size ? 10 : 0), y, scale * (icon_size ? 0.9 : 0.8), linesize);
  AddSun((x + (icon_size ? 10 : 0)) - scale * 1.8, y - scale * 1.6, scale, icon_size);
}

void ScatteredClouds(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  y += 15;
  if (icon_size == large_icon) scale = kLarge;
  AddCloud(x - (icon_size ? 35 : 0), y - scale * 2, scale / 2, linesize);
  AddCloud(x, y, scale * 0.9, linesize);
}

void Rain(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  y += 15;
  if (icon_size == large_icon) scale = kLarge;
  AddCloud(x, y, scale * (icon_size ? 1 : 0.75), linesize);
  AddRain(x, y, scale, icon_size);
}

void ChanceRain(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  if (icon_size == large_icon) scale = kLarge;
  y += 15;
  AddSun(x - scale * 1.8, y - scale * 1.8, scale, icon_size);
  AddCloud(x, y, scale * (icon_size ? 1 : 0.65), linesize);
  AddRain(x, y, scale, icon_size);
}

void Thunderstorms(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  if (icon_size == large_icon) scale = kLarge;
  y += 5;
  AddCloud(x, y, scale * (icon_size ? 1 : 0.75), linesize);
  AddTstorm(x, y, scale);
}

void Snow(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  if (icon_size == large_icon) scale = kLarge;
  AddCloud(x, y, scale * (icon_size ? 1 : 0.75), linesize);
  AddSnow(x, y, scale, icon_size);
}

void Mist(int x, int y, bool icon_size, const char* icon_name) {
  int scale = kSmall, linesize = 5;
  if (icon_name[2] == 'n') AddMoon(x, y, icon_size);
  if (icon_size == large_icon) scale = kLarge;
  AddSun(x, y, scale * (icon_size ? 1 : 0.75), linesize);
  AddFog(x, y, scale, linesize, icon_size);
}

void CloudCover(int x, int y, int cloud_cover) {
  AddCloud(x - 9, y, kSmall * 0.3, 2);
  AddCloud(x + 3, y - 2, kSmall * 0.3, 2);
  AddCloud(x, y + 15, kSmall * 0.6, 2);
  DrawString(x + 30, y, String(cloud_cover) + "%", Alignment::kLeft);
}

void Visibility(int x, int y, const String& visibility) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 10;
  int r = 14;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    DrawPixel(x + r * cos(i), y - r / 2 + r * sin(i) + Offset, kBlack);
    DrawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i) + Offset, kBlack);
  }
  start_angle = 3.61;
  end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    DrawPixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, kBlack);
    DrawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, kBlack);
  }
  FillCircle(x, y + Offset, r / 4, kBlack);
  DrawString(x + 20, y, visibility, Alignment::kLeft);
}

void AddMoon(int x, int y, bool icon_size) {
  int xOffset = 65;
  int yOffset = 12;
  if (icon_size == large_icon) {
    xOffset = 102;
    yOffset = -13;
  }
  FillCircle(x - 28 + xOffset, y - 37 + yOffset, kSmall, kBlack);
  FillCircle(x - 16 + xOffset, y - 37 + yOffset, (int)(kSmall * 1.6), kWhite);
}

void Nodata(int x, int y, bool icon_size, const char* icon_name) {
  if (icon_size == large_icon)
    SetFont(OpenSans24B);
  else
    SetFont(OpenSans12B);
  DrawString(x - 3, y - 10, "?", Alignment::kCenter);
}
