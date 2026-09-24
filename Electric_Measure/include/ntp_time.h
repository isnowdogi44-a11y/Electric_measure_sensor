#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <Arduino.h>

void ntp_init();
bool ntp_update();

void getFormattedDate(char* buffer, size_t bufferSize);
void getFormattedTime(char* buffer, size_t bufferSize);
bool ntp_synced();
unsigned long ntp_epoch();
int ntp_hour();

#endif