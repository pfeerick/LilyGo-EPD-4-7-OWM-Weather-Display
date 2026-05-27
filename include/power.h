#pragma once

#ifndef SIMULATOR_BUILD

extern long sleep_duration;
extern int wakeup_hour;
extern int sleep_hour;
extern long start_time;

void BeginSleep();
bool SetupTime();
uint8_t StartWiFi();
void StopWiFi();
bool UpdateLocalTime();

#endif  // SIMULATOR_BUILD
