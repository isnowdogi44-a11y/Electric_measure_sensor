#ifndef MICROSD_H
#define MICROSD_H

#include <Arduino.h>
#include <SD.h>
#include <NTPClient.h>

extern NTPClient timeClient;  // ОБЯЗАТЕЛЬНО!

void getFormattedDate(char* buffer, size_t bufferSize);
void getFormattedTime(char* buffer, size_t bufferSize);
void logData(const char* date, int hour, float RMS_current, 
             float I_min, char* time_min, float I_max, char* time_max, float P_count);

#endif