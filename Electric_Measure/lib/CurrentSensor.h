#pragma once

class CurrentSensor {
public:
    float current;             // Измеряемый ток
    float I_min = 100;         // счётчик минимального тока за час
    char time_i_min[10] = {}; // время минимальной нагрузки сети

    float I_max = 0;           // счётчик максимального тока за час
    char time_i_max[10] = {}; // время максимальной нагрузки сети

    double I_rms = 0;  // среднеквадратичное значение тока час
    float P_count = 0; // суммарная потреблённая мощность за час    
    
    //Выбор входа для измерения
    float read_current();


}