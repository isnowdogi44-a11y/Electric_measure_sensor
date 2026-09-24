#include "electric_measure.h"
#include <Arduino.h>
#include <math.h>

static unsigned long time_last_measure;
static float sum = 0.0f;
static float sumSquer = 0.0f;
static int count_of_measure = 0;

bool current_measure(float* current)
{
    static float sum = 0.0f;
    static float sumSquares = 0.0f;
    static uint16_t sampleCount = 0;
    static unsigned long lastSampleTime = 0;

    constexpr unsigned long SAMPLE_INTERVAL = 5; // мс

    // Ещё не пришло время нового ADC-отсчёта
    if (millis() - lastSampleTime < SAMPLE_INTERVAL) {
        return false;
    }

    lastSampleTime = millis();

    float adc = analogRead(ANALOG_IN);

    sum += adc;
    sumSquares += adc * adc;

    sampleCount++;

    // Окно измерения ещё не набрано
    if (sampleCount < SAMPLES) {
        return false;
    }

    // Средняя точка ADC
    float adcMean = sum / (float)sampleCount;

    // RMS только переменной составляющей:
    // RMS_AC² = E[x²] - E[x]²
    float variance = (sumSquares / (float)sampleCount) - (adcMean * adcMean);

    // Защита от отрицательного числа из-за погрешности float
    if (variance < 0.0f) {
        variance = 0.0f;
    }

    float adcRms = sqrt(variance);

    float voltage = adcRms * ESP_VOLTAGE / ADC_MAX;

    *current = (voltage / RESISTANCE) * 2500.0f;

    // Начинаем новое окно измерения
    sum = 0.0f;
    sumSquares = 0.0f;
    sampleCount = 0;

    return true;
}
/*
bool current_measure(float *current) {  
  //point: do a measure in time line with millis() func
  //measure shoulde be deal in period >= 5 ms.
  bool flag = false; //flag показывает сделано измерение или назодится в процессе.
  if (millis() - time_last_measure > SAMPLES_INTERVAL) {
    float val = analogRead(ANALOG_IN);
    float diff = val - 512;
    sum += val;
    sumSquer += diff * diff;
    count_of_measure += 1;
    time_last_measure = millis();
    flag = false;
  }

  if (count_of_measure == SAMPLES) {
    float adcMean = sum / (float)count_of_measure;
    float mean = sqrt(sum / (float)SAMPLES);
    float voltage = mean * ESP_VOLTAGE / ADC_MAX;
    *current = (voltage / RESISTANCE) * 2500.0f;
    sum = 0.0f;
    count_of_measure = 0;
    flag = true;
  }
  
  return flag;
}
*/