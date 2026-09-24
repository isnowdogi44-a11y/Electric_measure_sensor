#include "wi-fi_service.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#define WiFi_SSID "RoVi"
#define WiFi_password "24june99"

/*
IPAddress local_IP(192, 168, 31, 42);
IPAddress gateway(192, 168, 31, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);
*/

static bool wi_fi_connect_stat;
static unsigned long wi_fi_time_connect;
static unsigned long wi_fi_time_reconnect;

void wi_fi_init() {
  unsigned long start = millis();
  WiFi.mode(WIFI_STA);
  //WiFi.config(local_IP, gateway, subnet, dns);
  //WiFi.config(0U, 0U, 0U);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WiFi_SSID, WiFi_password);

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP()); 
    wi_fi_connect_stat = true; 
  } else {
      Serial.println("STA FAILED");
      wi_fi_connect_stat = false;
  }
  
  /*
  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("GW: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  */
  return;
}

bool wi_fi_stat() {
  bool flag = false;
  if (WiFi.status() == WL_CONNECTED) {  
    wi_fi_time_connect = millis();
    flag = true;
  } else {
    flag = false;
    if (millis() - wi_fi_time_connect > 5000) {
      wi_fi_time_connect = millis();
      Serial.println("WiFi reconnect...");
      WiFi.begin(WiFi_SSID, WiFi_password);
    }
  }

  return flag;
}

/*
void wi_fi_reconect(bool wi_fi_stat) {
  if (millis() - wi_fi_time_connect > 5000 && ((wi_fi_stat != wi_fi_connect_stat) || !wi_fi_time_connect)) {
    WiFi.begin(WiFi_SSID, WiFi_password);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("STA IP: ");
      Serial.println(WiFi.localIP());
      wi_fi_connect_stat = true;
      wi_fi_time_connect = millis();
    } else {
        Serial.println("STA FAILED");
        wi_fi_time_connect = millis();
    }
  }
  return;
}
*/

void wi_fi_reconect()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    else if (millis() - wi_fi_time_connect < 5000) {
        return;
    }

}