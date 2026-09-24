#include <Arduino.h>
#include <ArduinoOTA.h>
#include <math.h>
#include <stdbool.h>

#include "electric_measure.h"
#include "webpage.h"
#include "MicroSD_header.h"
#include "ntp_time.h"
#include "wi-fi_service.h"

// WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org");

struct electric_value el_val = {};
struct web_data web_data = {};

//void ensureWiFiConnected();
//unsigned long lastWifiReconnectAttempt = 0;


void setup() {
  pinMode(ANALOG_IN, INPUT);
  Serial.begin(115200);
  delay(5000);
  
  if (sd_init(CS_PIN)) {
    Serial.println("initialization done.");
  } else {
    Serial.println("initialization failed!");
  }
  
  wi_fi_init();
  
  ntp_init();
  if (ntp_synced()) {
    el_val.Hour = ntp_hour();
    getFormattedDate(el_val.date_of_measure, sizeof(el_val.date_of_measure));
  } else {
    el_val.Hour = -1;
  }    
  if (wi_fi_stat()) {
    ntp_update();
  }
  Serial.print("Epoch: ");
  Serial.println(ntp_epoch());
  char strDate[11] = {};
  char strTime[9] = {};
  getFormattedDate(strDate, sizeof(strDate));
  getFormattedTime(strTime, sizeof(strTime));

  html_page_begin(&web_data);

  el_val.I_min = 100.0f;
  getFormattedTime(el_val.time_i_min, sizeof(el_val.time_i_min));
  getFormattedTime(el_val.time_i_max, sizeof(el_val.time_i_max));

  ArduinoOTA.begin();

  Serial.println("OTA ready");
}

void loop() {
  ArduinoOTA.handle();
  
  char strDate[11] = {};
  char strTime[9] = {};

  if (wi_fi_stat()) {
    if(!ntp_synced()) {
      if (ntp_update()) {
        Serial.println("NTP synhronized!");
      } /*else {
        Serial.println("NTP not synhronized!");
      }
    */}
  } else {
    Serial.println("Wi-Fi conection is broke!");
  }
  
  html_page();
  //ensureWiFiConnected();


  if (ntp_synced() && el_val.Hour == -1) {
      el_val.Hour = ntp_hour();
      
      getFormattedDate(el_val.date_of_measure, sizeof(el_val.date_of_measure));

      Serial.print("Time initialized: ");
      Serial.print(el_val.date_of_measure);
      Serial.print(" ");
      Serial.println(el_val.Hour);
  }

  
  if (ntp_synced()) {
    getFormattedDate(strDate, sizeof(strDate));
    getFormattedTime(strTime, sizeof(strTime));
  } else {
    strncpy(strDate, "1970-01-01", sizeof(strDate));
    strncpy(strTime, "--:--:--", sizeof(strTime));
    strDate[sizeof(strDate) - 1] = '\0';
    strTime[sizeof(strTime) - 1] = '\0';
  }

  int CurrentHour = ntp_hour();

  if (ntp_synced() && CurrentHour != el_val.Hour) {
    Serial.print("Start write data on SD...");
    bool sd = sd_ready(CS_PIN);
    if (sd && el_val.RMS_total > 0) {
      el_val.I_rms = sqrt(el_val.RMS_sum / (float)el_val.RMS_total);
      float P_average = el_val.P_count / (float)el_val.RMS_total;
      float P_kWh = P_average * el_val.RMS_total / 3600.0 / 1000;
      sd_logData(el_val.date_of_measure, el_val.Hour, el_val.I_rms, el_val.I_min,
              el_val.time_i_min, el_val.I_max, el_val.time_i_max, P_average, P_kWh);
    } else if (!sd) {
      Serial.println(" SD is not ready, skip write.");
    } else {
      Serial.println("Not enougth measure for saving.");
    }

    el_val.RMS_sum = 0.0f;
    el_val.RMS_total = 0;
    el_val.I_min = 100.0f;
    el_val.I_max = 0.0f;
    el_val.I_rms = 0.0f;
    el_val.P_count = 0.0f;
    el_val.flag_alarm = 0;
    el_val.Hour = CurrentHour;
    strncpy(el_val.date_of_measure, strDate, sizeof(el_val.date_of_measure));
  }
  
  if (current_measure(&el_val.current)) {

    el_val.RMS_sum += el_val.current * el_val.current;
    el_val.RMS_total += 1;
    
    if (el_val.I_min > el_val.current) {
      el_val.I_min = el_val.current;
      if (ntp_synced()) {getFormattedTime(el_val.time_i_min, sizeof(el_val.time_i_min));}
    }

    if (el_val.I_max < el_val.current) {
      el_val.I_max = el_val.current;
      if (ntp_synced()) {getFormattedTime(el_val.time_i_max, sizeof(el_val.time_i_max));}
    }
    
    el_val.P_count += el_val.current * VOLTAGE_MAIN;

    if (el_val.I_max > MAX_CURRENT_VAL && el_val.flag_alarm == 0) {
      el_val.flag_alarm = 1;
      printf("ALARM: current is more than 19 A !!!");
    }
  }
  
  web_data.html_current = el_val.current;
  web_data.html_power = el_val.current * VOLTAGE_MAIN;
  
  snprintf(web_data.html_time, sizeof(web_data.html_time), "%s", strTime);
  snprintf(web_data.html_date, sizeof(web_data.html_date), "%s", strDate);
  
}


