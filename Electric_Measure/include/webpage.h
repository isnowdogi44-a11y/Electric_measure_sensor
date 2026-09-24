#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

struct web_data {
  float html_current = 0;
  float html_power = 0;
  char html_time[9] = {0};
  char html_date[11] = {0};
};

void html_page_begin(struct web_data *data);
void makeStatusJson(char* json, size_t jsonSize, const struct web_data* data);
void handleRoot();
void html_page();

#endif
