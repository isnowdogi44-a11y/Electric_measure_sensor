#include "ntp_time.h"

#include <WiFiUdp.h>
#include <NTPClient.h>

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, "pool.ntp.org");

static unsigned long lastNtpSyncAttempt = 0;

static const unsigned long NTP_retry_interval = 30000;  //30 sec.
static const unsigned long  NTP_sync_interval = 600000; //10 min.

void ntp_init() {
  timeClient.begin();
  timeClient.setTimeOffset(10800);
  return;
}

int ntp_hour(){
  return timeClient.getHours();
}

unsigned long ntp_epoch(){
  
  return timeClient.getEpochTime();
}

bool ntp_synced() {
  return timeClient.getEpochTime() > 1700000000UL;
}

bool ntp_update() {
  unsigned long now = millis();
  unsigned long update_interval;
  bool ntp_state;

  update_interval = ntp_synced() ? NTP_sync_interval : NTP_retry_interval;

  if (lastNtpSyncAttempt == 0 || (now - lastNtpSyncAttempt > update_interval)) {
    if (timeClient.update()) {
      ntp_state = true;
    } else {
      
      ntp_state = false;
    }
    lastNtpSyncAttempt = millis();
  } else {
    ntp_state = update_interval == NTP_sync_interval ? true : false;
  }

  return ntp_state;
}

void getFormattedDate(char* buffer, size_t bufferSize) {
  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime(&epochTime);

  snprintf(buffer, bufferSize, "%04d-%02d-%02d",
           ptm->tm_year + 1900,
           ptm->tm_mon + 1,
           ptm->tm_mday);
}

void getFormattedTime(char* buffer, size_t bufferSize) {
  char temp[9];
  timeClient.getFormattedTime().toCharArray(temp, sizeof(temp));
  strncpy(buffer, temp, bufferSize);
  buffer[bufferSize - 1] = '\0';
}