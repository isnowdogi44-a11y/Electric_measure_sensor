#ifndef MICROSD_H
#define MICROSD_H

#include <Arduino.h>
#include <SdFat.h>

bool sd_init(uint8_t CS_PIN);
bool sd_ready(uint8_t CS_PIN);
void sd_logData(const char* date, int hour, float RMS_current,
             float I_min, const char* time_min, float I_max, const char* time_max,
             float P_average, float P_kWh);

#endif
