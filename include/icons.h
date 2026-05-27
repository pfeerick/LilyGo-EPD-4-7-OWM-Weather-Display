#pragma once
#include "display.h"

void AddCloud(int x, int y, int scale, int linesize);
void AddRain(int x, int y, int scale, bool icon_size);
void AddSnow(int x, int y, int scale, bool icon_size);
void AddTstorm(int x, int y, int scale);
void AddSun(int x, int y, int scale, bool icon_size);
void AddFog(int x, int y, int scale, int linesize, bool icon_size);
void DrawAngledLine(int x, int y, int x1, int y1, int size, int color);
void ClearSky(int x, int y, bool icon_size, const char* icon_name);
void BrokenClouds(int x, int y, bool icon_size, const char* icon_name);
void FewClouds(int x, int y, bool icon_size, const char* icon_name);
void ScatteredClouds(int x, int y, bool icon_size, const char* icon_name);
void Rain(int x, int y, bool icon_size, const char* icon_name);
void ChanceRain(int x, int y, bool icon_size, const char* icon_name);
void Thunderstorms(int x, int y, bool icon_size, const char* icon_name);
void Snow(int x, int y, bool icon_size, const char* icon_name);
void Mist(int x, int y, bool icon_size, const char* icon_name);
void CloudCover(int x, int y, int cloud_cover);
void Visibility(int x, int y, const String& visibility);
void AddMoon(int x, int y, bool icon_size);
void Nodata(int x, int y, bool icon_size, const char* icon_name);
