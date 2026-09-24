#ifndef ELECTRIC_MEASURE

#define ELECTRIC_MEASURE

// Исходные данные для измерений
#define RESISTANCE 33.                //Ом
#define TRANS_COEFF 1.0 / 2500.0      //А
#define VOLTAGE_MAIN 230              // В
#define DELAY 10000                   // мс
#define COUNT_MEASURE 3600000 / DELAY // шт. - кол-во измерений считываемых с датчика тока за 1 час
#define SAMPLES 40                    // кол-во измряений тока в ед.времени
#define SAMPLES_INTERVAL 5            // минимальный интервал между измерениями. Ограничен возможностью ESP в связке с Wi-Fi и HTTP-server
#define MAX_CURRENT_VAL 19            // А - предельное значение тока для события тревоги
#define ESP_VOLTAGE  3.3              // Опорное напряжение
#define ADC_MAX  1023                 // Максимум АЦП
#define ANALOG_IN A0                  // Вход для измерения тока
#define CS_PIN D2                     // Контакт управления шиной SPI

struct electric_value {
  int Hour;             // Текущий час

  float RMS_sum;        // вsеличина среднеквадратичного тока за 1 час
  long RMS_total;      // кол-во точек измерения используемых для вычисления RMS_current

  float current_calc;   //VAR TESTER
  float current;        // Измеряемый ток

  float I_min;          // счётчик минимального тока за час
  char time_i_min[10];  // время минимальной нагрузки сети

  float I_max;          // счётчик максимального тока за час
  char time_i_max[10];  // время максимальной нагрузки сети

  double I_rms;         // среднеквадратичное значение тока час
  float P_count;        // суммарная мощность за время измерений
  float P_average;      // среняя потребляемая мощность за час
  float P_kWh;          // электроэнергия потреблённая за час

  int flag_alarm;       // флаг превышения по току

  char date_of_measure[11] = {0};

  int counter_measure;  //VAR TESTER
};


bool current_measure(float *current);

#endif