#include <ESP8266WebServer.h>
#include "webpage.h"
#include <stdio.h>

ESP8266WebServer server(80);

static const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Electric measure</title>

<style>
  body {
    margin: 0;
    background: #06090b;
    color: #c8ffde;
    font-family: Consolas, monospace;
    padding: 24px;
  }

  .card {
    max-width: 900px;
    margin: 0 auto;
    padding: 20px;
    border: 1px solid rgba(0,255,140,.25);
    border-radius: 16px;
    background: rgba(0,0,0,.45);
  }

  h2 {
    color: #6bffb6;
    letter-spacing: .12em;
    text-transform: uppercase;
    font-size: 16px;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    margin-top: 16px;
  }

  th, td {
    border-bottom: 1px solid rgba(0,255,140,.18);
    padding: 14px 10px;
    text-align: left;
  }

  th {
    color: #00ff8c;
    text-transform: uppercase;
    font-size: 13px;
  }

  td {
    font-size: 22px;
    font-weight: 700;
  }
</style>
</head>

<body>
  <div class="card">
    <h2>Текущие показания</h2>

    <table>
      <thead>
        <tr>
          <th>Ток</th>
          <th>Мощность</th>
          <th>Время</th>
          <th>Дата</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td><span id="current">0.00</span> A</td>
          <td><span id="power">0</span> W</td>
          <td id="time">--:--:--</td>
          <td id="date">--.--.----</td>
        </tr>
      </tbody>
    </table>
  </div>

<script>
async function updateData() {
  try {
    const res = await fetch('/api/status', { cache: 'no-store' });
    const data = await res.json();

    document.getElementById('current').textContent = Number(data.current).toFixed(2);
    document.getElementById('power').textContent = Number(data.power).toFixed(0);
    document.getElementById('time').textContent = data.time;
    document.getElementById('date').textContent = data.date;
  } catch (e) {
    console.log('Ошибка обновления данных', e);
  }
}

updateData();
setInterval(updateData, 2000);
</script>

</body>
</html>
)HTML";


void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}
void handleStatus(struct web_data *data) {
  char json[160];
  makeStatusJson(json, sizeof(json), data);
  server.send(200, "application/json", json);
}
void html_page() {
  server.handleClient();
}
void html_page_begin(struct web_data *data) {
  server.on("/", handleRoot);
  server.on("/api/status", [data]() {handleStatus(data);});
  server.begin();
}
void makeStatusJson(char* json, size_t jsonSize, const struct web_data* data) {
  snprintf(json, jsonSize,
           "{\"current\":%.2f,\"power\":%.0f,\"time\":\"%s\",\"date\":\"%s\"}",
           data->html_current,
           data->html_power,
           data->html_time,
           data->html_date);
}

