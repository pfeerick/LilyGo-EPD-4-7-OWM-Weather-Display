#pragma once

#ifndef SIMULATOR_BUILD

extern long SleepDuration;
extern int WakeupHour;
extern int SleepHour;
extern long StartTime;

void BeginSleep();
bool SetupTime();
uint8_t StartWiFi();
void StopWiFi();
bool UpdateLocalTime();

#endif  // SIMULATOR_BUILD
