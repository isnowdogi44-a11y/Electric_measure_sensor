#include "MicroSD_header.h"
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

SdFat SD;
bool sdReady;

struct dir_name {
  char yearDir[6];
  char monthDir[9];
} microSD_dir;

struct csv_file {
  char day_file_csv[32];
  char data_line_csv[100];
} microSD_file;

bool sd_init(uint8_t CS_PIN) {
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(50);
  
  Serial.print("Initializing SD card...");
  if (SD.begin(CS_PIN, SD_SCK_MHZ(1))) {
    sdReady = true;
  } else {
    sdReady = false;
  }
  return sdReady;
}

bool sd_ready(uint8_t CS_PIN) {
  if (!sdReady) {
    Serial.print("Initializing SD card...");
    if (!SD.begin(CS_PIN, SD_SCK_MHZ(1))) {
      Serial.println("initialization failed!");
    } else {
      Serial.println("initialization done.");
      sdReady = true;
    }
  }
  return sdReady;
}

void sd_logData(const char* date, int hour, float RMS_current,
             float I_min, const char* time_min, float I_max, const char* time_max,
             float P_average, float P_kWh) {
  int year, month, day;

  if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) {
      Serial.println("Invalid date");
      return;
  }

  snprintf(microSD_dir.yearDir, sizeof(microSD_dir.yearDir), "/%04d", year);
  snprintf(microSD_dir.monthDir, sizeof(microSD_dir.monthDir), "/%04d/%02d", year, month);
  snprintf(microSD_file.day_file_csv, sizeof(microSD_file.day_file_csv),
           "/%04d/%02d/%02d.csv", year, month, day);

  if (!SD.exists(microSD_dir.yearDir)) {
    Serial.print("Creating year microSD_direction: ");
    Serial.println(microSD_dir.yearDir);
    SD.mkdir(microSD_dir.yearDir);
  }

  if (!SD.exists(microSD_dir.monthDir)) {
    Serial.print("Creating month microSD_direction: ");
    Serial.println(microSD_dir.monthDir);
    SD.mkdir(microSD_dir.monthDir);
  }

  FsFile dataFile = SD.open(microSD_file.day_file_csv, O_WRONLY | O_CREAT | O_AT_END);

  if (dataFile) {
    if (dataFile.size() == 0) {
      dataFile.println("Date,Hour,RMS_current,I_min,time_min,I_max,time_max,P_count,P_kWh");
      Serial.print("Created new microSD_file: ");
      Serial.println(microSD_file.day_file_csv);
    }

    snprintf(microSD_file.data_line_csv, sizeof(microSD_file.data_line_csv),
             "%s,%02d,%.2f,%.2f,%s,%.2f,%s,%.2f,%.2f",
             date, hour, RMS_current, I_min, time_min, I_max, time_max, P_average, P_kWh);

    dataFile.println(microSD_file.data_line_csv);
    dataFile.close();

    Serial.print("Data written to: ");
    Serial.println(microSD_file.day_file_csv);
  } else {
    Serial.print("Error opening microSD_file: ");
    Serial.println(microSD_file.day_file_csv);
  }
}
