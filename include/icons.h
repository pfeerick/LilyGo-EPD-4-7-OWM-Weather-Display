#pragma once
#include "display.h"

void addcloud(int x, int y, int scale, int linesize);
void addrain(int x, int y, int scale, bool IconSize);
void addsnow(int x, int y, int scale, bool IconSize);
void addtstorm(int x, int y, int scale);
void addsun(int x, int y, int scale, bool IconSize);
void addfog(int x, int y, int scale, int linesize, bool IconSize);
void DrawAngledLine(int x, int y, int x1, int y1, int size, int color);
void ClearSky(int x, int y, bool IconSize, const char* IconName);
void BrokenClouds(int x, int y, bool IconSize, const char* IconName);
void FewClouds(int x, int y, bool IconSize, const char* IconName);
void ScatteredClouds(int x, int y, bool IconSize, const char* IconName);
void Rain(int x, int y, bool IconSize, const char* IconName);
void ChanceRain(int x, int y, bool IconSize, const char* IconName);
void Thunderstorms(int x, int y, bool IconSize, const char* IconName);
void Snow(int x, int y, bool IconSize, const char* IconName);
void Mist(int x, int y, bool IconSize, const char* IconName);
void CloudCover(int x, int y, int CloudCover);
void Visibility(int x, int y, const String& Visibility);
void addmoon(int x, int y, bool IconSize);
void Nodata(int x, int y, bool IconSize, const char* IconName);
