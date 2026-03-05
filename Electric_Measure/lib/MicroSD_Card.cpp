#include "MicroSD_hedder.h"  // твой заголовок
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <NTPClient.h>


//---------функция получения даты---------------------
void getFormattedDate(char* buffer, size_t bufferSize) 
{
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime(&epochTime);

  sprintf(buffer, "%04d-%02d-%02d",
    ptm->tm_year + 1900,
    ptm->tm_mon + 1,
    ptm->tm_mday);
}

//---------функция получения времени---------------------
void getFormattedTime(char* buffer, size_t bufferSize) {
  char temp[9];
  timeClient.getFormattedTime().toCharArray(temp, 9);
  strncpy(buffer, temp, bufferSize);
}

//-------функция создания файлов и записи данных------------------

void logData(const char* date, int hour, float RMS_current, float I_min, char* time_min, float I_max, char* time_max, float P_count)
{
	//----------------Создаём строки для хранения названий папок-------------------
    char yearDir[6] = {};    // /YYYY\0
    char monthDir[9] = {};   // /YYYY/MM\0
    char dayfile[20] = {};   // /YYYY/MM/DD\0
    char fileCSV[32];        // /YYYY/MM/DD.csv\0
    char dataLine[100] = {}; //данные записываемые в файл CSV

    int year, month, day;
	sscanf(date, "%d-%d-%d", &year, &month, &day);
	
	snprintf(yearDir, sizeof(yearDir), "/%04d", year);
	snprintf(monthDir, sizeof(monthDir), "/%02d", month);
	snprintf(fileCSV, sizeof(fileCSV), "/%04d/%02d/%02d.csv", year, month, day);
	
	//Create folders
	if(!SD.exists(yearDir)){
		Serial.print("Creating year direction: ");
		Serial.println(yearDir);
		SD.mkdir(yearDir);
	}
	if(!SD.exists(monthDir)){
		Serial.print("Creating month direction: ");
		Serial.println(monthDir);
		SD.mkdir(monthDir);
	}
	
	//Open file
	File dataFile = SD.open(fileCSV, FILE_WRITE);
	
	if(dataFile){
		if(dataFile.size() == 0){
			dataFile.println("Date,Hour,RMS_current,I_min,time_min,I_max,time_max,P_count");
			Serial.print("Created new file: ");
		    Serial.println(fileCSV);
		}
	
	
	//Формируем строку для записи в CSV
	  snprintf(dataLine, sizeof(dataLine)/sizeof(*dataLine), "%s,%02d,%.2f,%.2f,%s,%.2f,%s,%.2f", date, hour, RMS_current, I_min, time_min, I_max, time_max, P_count);
	
	  dataFile.println(dataLine);
	
	  dataFile.close();
	
    Serial.print("Data written to: ");
    Serial.println(fileCSV);
  } else {
    Serial.print("Error opening file: ");
    Serial.println(fileCSV);
  }
}


