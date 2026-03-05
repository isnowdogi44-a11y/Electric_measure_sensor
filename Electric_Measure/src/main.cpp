#include <Arduino.h>
#include <math.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <SdFat.h>
#include <stdbool.h>


String prepareHTML();
void getFormattedDate(char* buffer, size_t bufferSize);
void getFormattedTime(char* buffer, size_t bufferSize);
void logData(const char* date, int hour, float RMS_current, float I_min, char* time_min, float I_max, char* time_max, float P_count);




// ************************************************************************************

// *****************************ГЛОБАЛЬНЫЙ ПЕРЕМЕННЫЕ**********************************

// ************************************************************************************


//----------------------------Исходные данные для измерений---------------------------

// Исходные данные для измерений
#define RESISTANCE 33.                // Ом
#define TRANS_COEFF 1 / 2500          // А
#define VOLTAGE_MAIN 230              // В
#define DELAY 10000                   // мс
#define COUNT_MEASURE 3600000 / DELAY // шт. - кол-во измерений считываемых с датчика тока за 1 час
#define MAX_CURRENT_VAL 19            // А - предельное значение тока для события тревоги
#define CS_PIN D2                     // Контакт управления шиной SPI
#define ESP_VOLTAGE  3.3              // Опорное напряжение
#define ADC_MAX  1023                 // Максимум АЦП


//----------------------------Сетевые настройки---------------------------
// Wi-Fi client
#define WiFi_SSID "EVIL"
#define WiFi_password "24june99"
// Парметры HTTP сервера
ESP8266WebServer server(80);
//IPAddress local_IP(192, 168, 31, 42);
IPAddress gateway(192, 168, 31, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);



//----------------------------НАСТРОЙКИ NTCP---------------------------

WiFiUDP ntpUDP;
                    
NTPClient timeClient(ntpUDP, "pool.ntp.org");


const int ntp_packet_size = 48;   //данные о времени хранятся в первых 48 байтах пакета UDP NTCP

byte NTPBuffer[ntp_packet_size];  //массив для хранения данных о времени





//----------------------------НАСТРОЙКИ HTTP СЕРВЕРА И HTML---------------------------

volatile float g_currentA = 0.0f;      // текущий ток, A
volatile float g_powerW   = 0.0f;      // текущая мощность, W
volatile uint32_t g_unix  = 0;         // текущее время (unix), если есть

// ===== Hour live buffer (60 points, 1 per minute) =====
static const uint16_t HOUR_POINTS = 60;
static float hour_I[HOUR_POINTS] = {0};
static float hour_P[HOUR_POINTS] = {0};
static uint32_t hour_t[HOUR_POINTS] = {0}; // unix for each point (optional)

static uint16_t hour_idx = 0;
static uint32_t lastMinuteStamp = 0;

//******************************* HTTP Server *****************************************/

static const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1" />
<title>VoltTwin • Current Monitor</title>
<style>
  :root{
    --bg:#06090b;
    --panel:#0b1113;
    --grid:rgba(0,255,140,.08);
    --grid2:rgba(0,255,140,.04);
    --line:#00ff8c;
    --line2:#6bffb6;
    --text:#c8ffdE;
    --muted:#7de7b4;
    --shadow:0 0 0 1px rgba(0,255,140,.14), 0 12px 30px rgba(0,0,0,.55);
    --r:16px;
  }
  *{box-sizing:border-box}
  body{
    margin:0;
    font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
    color:var(--text);
    background:
      linear-gradient(180deg, rgba(0,255,140,.06), transparent 28%),
      radial-gradient(1200px 700px at 50% 0%, rgba(0,255,140,.10), transparent 65%),
      var(--bg);
  }
  .wrap{max-width:1100px;margin:18px auto;padding:14px;}
  .frame{
    position:relative;border-radius:22px;padding:18px;
    background:
      linear-gradient(180deg, rgba(0,255,140,.06), transparent 35%),
      repeating-linear-gradient(0deg, var(--grid) 0 1px, transparent 1px 20px),
      repeating-linear-gradient(90deg, var(--grid2) 0 1px, transparent 1px 20px),
      linear-gradient(180deg, rgba(0,0,0,.55), rgba(0,0,0,.55)),
      var(--panel);
    box-shadow:var(--shadow);
    overflow:hidden;
  }
  .hud{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:6px 6px 14px 6px;}
  .brand{display:flex;align-items:center;gap:10px;letter-spacing:.14em;text-transform:uppercase;font-weight:700;}
  .dot{width:10px;height:10px;border-radius:2px;background:var(--line);box-shadow:0 0 16px rgba(0,255,140,.65);}
  .clock{color:var(--muted);font-size:12px;letter-spacing:.08em;}

  .grid{
    display:grid;
    grid-template-columns: 1.15fr .85fr;
    grid-template-rows: auto auto auto;
    gap:14px;
  }
  @media (max-width: 860px){
    .grid{grid-template-columns:1fr}
    .yesterday{grid-row:auto;grid-column:auto;}
  }

  .card{
    border-radius:var(--r);
    padding:14px;
    background:linear-gradient(180deg, rgba(0,255,140,.05), transparent 40%), rgba(0,0,0,.55);
    box-shadow:0 0 0 1px rgba(0,255,140,.12) inset;
  }
  .title{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px;}
  .title h2{margin:0;font-size:13px;font-weight:800;letter-spacing:.14em;text-transform:uppercase;color:var(--line2);}
  .badge{font-size:11px;color:var(--muted);border:1px solid rgba(0,255,140,.22);padding:3px 8px;border-radius:999px;}

  .big{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin-top:8px;}
  .metric{border-radius:14px;padding:12px;background:rgba(0,0,0,.35);box-shadow:0 0 0 1px rgba(0,255,140,.10) inset;}
  .metric .k{font-size:11px;letter-spacing:.12em;text-transform:uppercase;color:var(--muted)}
  .metric .v{margin-top:6px;font-size:34px;font-weight:900;letter-spacing:.02em;color:var(--text);}
  .metric .u{font-size:12px;color:var(--muted);margin-left:6px}

  .foot{
    margin-top:12px;
    display:flex;justify-content:space-between;align-items:center;gap:10px;
    color:var(--muted);font-size:11px;
  }
  .btn{
    cursor:pointer;background:transparent;color:var(--line2);
    border:1px solid rgba(0,255,140,.22);
    border-radius:12px;padding:8px 10px;font:inherit;
  }
  .btn:active{transform:translateY(1px)}

  .chartBox{height:240px; width:100%; position:relative;}
  .hourBox{height:170px; width:100%; position:relative;}
  canvas{display:block; width:100%; height:100%;}

  .hourHint{
    margin-top:8px;
    text-align:center;
    color:rgba(125,231,180,0.85);
    font-size:12px;
    letter-spacing:.12em;
    text-transform:uppercase;
  }

  .yesterday{
    grid-column:2;
    grid-row:1 / span 2;
    display:flex;
    flex-direction:column;
  }
  .yesterday .chartBox{flex:1; height:auto; min-height:200px;}

  table{width:100%;border-collapse:collapse;font-size:12px;}
  th,td{padding:10px 8px;border-bottom:1px solid rgba(0,255,140,.14);text-align:left;color:var(--text);}
  th{color:var(--line2);font-size:11px;letter-spacing:.12em;text-transform:uppercase;}
  tr:hover td{background:rgba(0,255,140,.05)}
</style>
</head>

<body>
  <div class="wrap">
    <div class="frame">
      <div class="hud">
        <div class="brand"><span class="dot"></span> VOLTTWIN / SENSOR</div>
        <div class="clock" id="clock">--.--.---- --:--:--</div>
      </div>

      <div class="grid">
        <!-- ЛЕВО / ВЕРХ: текущие значения -->
        <div class="card">
          <div class="title">
            <h2>Текущие значения</h2>
            <span class="badge" id="status">OFFLINE</span>
          </div>

          <div class="big">
            <div class="metric">
              <div class="k">Ток</div>
              <div class="v"><span id="curA">0.00</span><span class="u">A</span></div>
            </div>
            <div class="metric">
              <div class="k">Мощность</div>
              <div class="v"><span id="powW">0</span><span class="u">W</span></div>
            </div>
          </div>

          <div class="foot">
            <div>Данные обновляются каждые <span id="poll">2</span> сек</div>
            <button class="btn" id="btnRefresh">Обновить</button>
          </div>
        </div>

        <!-- ПРАВО: график вчера -->
        <div class="card yesterday">
          <div class="title">
            <h2>График (вчера)</h2>
            <span class="badge">24 точки</span>
          </div>
          <div class="chartBox">
            <canvas id="dayChart"></canvas>
          </div>
          <div class="foot">
            <div>Ось X: часы • Ось Y: потребление</div>
          </div>
        </div>

        <!-- ЛЕВО / НИЗ: график 60 минут -->
        <div class="card">
          <div class="title">
            <h2>График (последние 60 мин)</h2>
            <span class="badge">реальное время</span>
          </div>

          <div class="hourBox">
            <canvas id="hourChart"></canvas>
          </div>

          <div class="hourHint">МИНУТЫ ВСЕГО 60 ДЕЛЕНИЙ</div>
        </div>

        <!-- НИЗ: таблица на всю ширину -->
        <div class="card" style="grid-column:1 / -1;">
          <div class="title">
            <h2>Сводка за 5 дней</h2>
            <span class="badge">таблица</span>
          </div>
          <table>
            <thead>
              <tr>
                <th>Дата</th>
                <th>кВт⋅ч</th>
                <th>Max W</th>
                <th>Avg W</th>
                <th>Min W</th>
              </tr>
            </thead>
            <tbody id="tblBody">
              <!-- Тестовые данные для проверки отображения -->
              <tr><td>01.03.2026</td><td>2.45</td><td>180</td><td>120</td><td>45</td></tr>
              <tr><td>02.03.2026</td><td>3.12</td><td>210</td><td>145</td><td>52</td></tr>
              <tr><td>03.03.2026</td><td>1.98</td><td>165</td><td>98</td><td>32</td></tr>
              <tr><td>04.03.2026</td><td>2.87</td><td>195</td><td>132</td><td>48</td></tr>
              <tr><td>05.03.2026</td><td>2.34</td><td>175</td><td>115</td><td>41</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  </div>

<script>
  const $ = (id)=>document.getElementById(id);

  function fmt2(x){ return (Math.round(x*100)/100).toFixed(2); }
  function fmt0(x){ return String(Math.round(x)); }

  async function fetchJSON(path){
    try {
      const res = await fetch(path, {cache:"no-store"});
      if(!res.ok) throw new Error("HTTP "+res.status);
      return await res.json();
    } catch(e) {
      console.log("Fetch error:", e);
      return null;
    }
  }

  function setClockFromunix(g_unix){
    const d = new Date(g_unix*1000);
    const dd = String(d.getDate()).padStart(2,"0");
    const mm = String(d.getMonth()+1).padStart(2,"0");
    const yy = String(d.getFullYear());
    const hh = String(d.getHours()).padStart(2,"0");
    const mi = String(d.getMinutes()).padStart(2,"0");
    const ss = String(d.getSeconds()).padStart(2,"0");
    $("clock").textContent = `${dd}.${mm}.${yy} ${hh}:${mi}:${ss}`;
  }

  // Тестовые данные для графиков (пока нет сервера) - МОЖНО УДАЛИТЬ
  // const testYesterday = [1.2, 1.8, 2.1, 2.4, 2.2, 1.9, 1.5, 1.3, 1.1, 1.4, 1.7, 2.0, 
  //                        2.3, 2.5, 2.7, 2.4, 2.1, 1.8, 1.6, 1.9, 2.2, 2.0, 1.7, 1.4];

  // Буфер для часового графика - начинаем с нулей
  let hourBuf = new Array(60).fill(0);

  // ======= ГРАФИК "ВЧЕРА" =======
  function drawDay(values){
    const canvas = $("dayChart");
    if(!canvas) return;
    
    const ctx = canvas.getContext("2d");
    const container = canvas.parentElement;
    const w = container.clientWidth || 300;
    const h = container.clientHeight || 200;
    
    canvas.width = w;
    canvas.height = h;
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';

    ctx.clearRect(0, 0, w, h);

    // сетка
    ctx.strokeStyle = "rgba(0,255,140,0.12)";
    ctx.lineWidth = 1;
    const gx = 8, gy = 6;
    for(let i=1;i<gx;i++){ 
      const x=(w/gx)*i; 
      ctx.beginPath(); 
      ctx.moveTo(x,0); 
      ctx.lineTo(x,h); 
      ctx.stroke(); 
    }
    for(let i=1;i<gy;i++){ 
      const y=(h/gy)*i; 
      ctx.beginPath(); 
      ctx.moveTo(0,y); 
      ctx.lineTo(w,y); 
      ctx.stroke(); 
    }

    const max = Math.max(...values, 0.1);
    const min = Math.min(...values, 0);
    const range = max - min || 1;
    const pad = 16;
    const innerW = w - pad*2;
    const innerH = h - pad*2;

    // Рисуем линию
    ctx.strokeStyle = "#00ff8c";
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    values.forEach((v, i)=>{
      const x = pad + innerW * (i / (values.length-1 || 1));
      const y = pad + innerH * (1 - (v - min) / range);
      
      if(i===0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.stroke();

    // Рисуем точки
    ctx.fillStyle = "#00ff8c";
    values.forEach((v, i)=>{
      const x = pad + innerW * (i / (values.length-1 || 1));
      const y = pad + innerH * (1 - (v - min) / range);
      ctx.beginPath();
      ctx.arc(x, y, 3, 0, 2*Math.PI);
      ctx.fill();
    });
  }

  // ======= ГРАФИК "60 МИН" =======
  function drawHour(values){
    const canvas = $("hourChart");
    if(!canvas) return;
    
    const ctx = canvas.getContext("2d");
    const container = canvas.parentElement;
    const w = container.clientWidth || 300;
    const h = container.clientHeight || 150;
    
    canvas.width = w;
    canvas.height = h;
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';

    ctx.clearRect(0, 0, w, h);

    // сетка
    ctx.strokeStyle = "rgba(0,255,140,0.10)";
    ctx.lineWidth = 1;
    for(let i=1;i<6;i++){
      const x = (w/6)*i;
      ctx.beginPath(); 
      ctx.moveTo(x,0); 
      ctx.lineTo(x,h); 
      ctx.stroke();
    }
    for(let i=1;i<4;i++){
      const y = (h/4)*i;
      ctx.beginPath(); 
      ctx.moveTo(0,y); 
      ctx.lineTo(w,y); 
      ctx.stroke();
    }

    const max = Math.max(...values, 0.1);
    const min = Math.min(...values, 0);
    const range = max - min || 1;
    const pad = 10;
    const innerW = w - pad*2;
    const innerH = h - pad*2;

    ctx.strokeStyle = "#00ff8c";
    ctx.lineWidth = 1.6;
    ctx.beginPath();
    
    values.forEach((v, i)=>{
      const x = pad + innerW * (i / (values.length-1 || 1));
      const y = pad + innerH * (1 - (v - min) / range);
      
      if(i===0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.stroke();
  }

  function pushHourPoint(val){
    hourBuf.push(val);
    if(hourBuf.length > 60) hourBuf.shift();
    drawHour(hourBuf);
  }

  function fillTable(rows){
    const tb = $("tblBody");
    if(!rows || rows.length === 0) return;
    
    tb.innerHTML = rows.map(r => `
      <tr>
        <td>${r.date || '--.--.----'}</td>
        <td>${(r.kwh || 0).toFixed(2)}</td>
        <td>${fmt0(r.maxW || 0)}</td>
        <td>${fmt0(r.avgW || 0)}</td>
        <td>${fmt0(r.minW || 0)}</td>
      </tr>
    `).join("");
  }

async function refresh(){
  try{
    // Пытаемся получить данные с сервера
    const s = await fetchJSON("/api/status");
    
    if(s) {
      // Данные получены с сервера
      $("curA").textContent = fmt2(s.currentA || 0);
      $("powW").textContent = fmt0(s.powerW || 0);
      $("status").textContent = "ONLINE";
      $("status").style.color = "#6bffb6";
      setClockFromUnix(s.unix || Math.floor(Date.now()/1000));

      // Рисуем графики с серверными данными
      if(s.yesterday24 && s.yesterday24.length > 0) {
        drawDay(s.yesterday24);
      }
      // else {
      //   drawDay(testYesterday); // ← закомментировано
      // }
      
      if(s.last5 && s.last5.length > 0) {
        fillTable(s.last5);
      }

      // Добавляем точку в часовой график
      pushHourPoint(Number(s.currentA) || 0); // убрал random
    }
    // else {
    //   // Режим офлайн - показываем тестовые данные (ЗАКОММЕНТИРОВАНО)
    //   $("status").textContent = "DEMO MODE";
    //   $("status").style.color = "#ffaa00";
    //   $("curA").textContent = fmt2(Math.random() * 5);
    //   $("powW").textContent = fmt0(Math.random() * 200 + 50);
    //   
    //   // Рисуем тестовые графики
    //   drawDay(testYesterday);
    //   pushHourPoint(Math.random() * 5);
    // }

  } catch(e){
    console.log("Refresh error:", e);
    $("status").textContent = "OFFLINE";
    $("status").style.color = "#ff6b6b";
    
    // Даже в офлайне показываем тестовые графики (ЗАКОММЕНТИРОВАНО)
    // drawDay(testYesterday);
    // pushHourPoint(Math.random() * 5);
  }
}

  // Обработчик кнопки обновления
  $("btnRefresh").addEventListener("click", (e) => {
    e.preventDefault();
    refresh();
  });

  const POLL_SEC = 2;
  $("poll").textContent = POLL_SEC;

  // Функция для безопасного ресайза
  let resizeTimeout;
  window.addEventListener("resize", ()=>{
    clearTimeout(resizeTimeout);
    resizeTimeout = setTimeout(() => {
      // Только перерисовываем графики, без запроса новых данных
      // drawDay(testYesterday); // ← закомментировать
      drawHour(hourBuf);
    }, 100);
  });

  // Старт
  setTimeout(() => {
    refresh();
    // Запускаем периодическое обновление
    setInterval(refresh, POLL_SEC*1000);
  }, 100);
</script>
</body>
</html>
)HTML";

static void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

// JSON: текущие значения
static void handleStatus() {
  char dt[24];
  // время берём из NTPClient
  // формат: YYYY-MM-DD HH:MM:SS
  char d[11];
  char t[9];
  // твои функции уже есть:
  getFormattedDate(d, sizeof(d));
  timeClient.getFormattedTime().toCharArray(t, sizeof(t));
  snprintf(dt, sizeof(dt), "%s %s", d, t);

  // компактный JSON без String (меньше фрагментации heap)
  char json[160];
  snprintf(json, sizeof(json),
    "{\"currentA\":%.3f,\"powerW\":%.1f,\"unix\":%lu,\"datetime\":\"%s\"}",
    (double)g_currentA,
    (double)g_powerW,
    (unsigned long)g_unix,
    dt
  );
  server.send(200, "application/json; charset=utf-8", json);
}

static void handleHour() {
  // отдаём 60 точек в правильном порядке (от старых к новым)
  String json;
  json.reserve(2500);

  json += "{\"idx\":";
  json += String(hour_idx);
  json += ",\"I\":[";
  for (uint16_t k = 0; k < HOUR_POINTS; k++) {
    uint16_t i = (hour_idx + k) % HOUR_POINTS;
    if (k) json += ",";
    json += String(hour_I[i], 3);
  }
  json += "],\"P\":[";
  for (uint16_t k = 0; k < HOUR_POINTS; k++) {
    uint16_t i = (hour_idx + k) % HOUR_POINTS;
    if (k) json += ",";
    json += String(hour_P[i], 1);
  }
  json += "]}";

  server.send(200, "application/json; charset=utf-8", json);
}

//----------------------------ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ---------------------------


int samples = 200;        //кол-во измряений тока в ед.времени
int Hour = -1;                 // Текущий час

float RMS_sum = 0;       //величина среднеквадратичного тока за 1 час
int RMS_total = 0;       //кол-во точек измерения используемых для вычисления RMS_current

float current;             // Измеряемый ток

float I_min = 100;         // счётчик минимального тока за час
char time_i_min[10] = {0}; // время минимальной нагрузки сети

float I_max = 0;           // счётчик максимального тока за час
char time_i_max[10] = {0}; // время максимальной нагрузки сети

double I_rms = 0;  // среднеквадратичное значение тока час
float P_count = 0; // суммарная потреблённая мощность за час

char flag_alarm = 0;                  // тревога превышаения нагрузки

//---------------- Переменные для работы с временем NTP----------------------

unsigned long intervalNTP =  DELAY; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;
uint32_t Last_hour = 0;

char strDate[20] = {};              //Строка для хранения даты 
char strTime[20] = {};              //Строка для хранения времени


//----------------Создаём строки для хранения названий папок-------------------
char yearDir[6] = {};    // /YYYY\0
char monthDir[9] = {};   // /YYYY/MM\0
char dayfile[20] = {};   // /YYYY/MM/DD\0
char fileCSV[32];        // /YYYY/MM/DD.csv\0
char dataLine[100] = {}; //данные записываемые в файл CSV



//-------Массивы которые хранят измерения за час и далее записываются на MicroSD-----------

float ar_i_min_day[24] = {0};
float ar_i_max_day[24] = {0};
float ar_i_rms_day[24] = {0};
float ar_P_count_day[24] = {0};
short ar_hour[24] = {0};
char time_min[24] = {0};
char time_max[24] = {0};
SdFat SD;


//------------------ФУНКЦИИ ----------------------------------

/*
void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, ntp_packet_size);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, ntp_packet_size);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}
*/


void setup()
{
  // включаем аналогвый порт А0
  pinMode(A0, INPUT);
  // настрои скорость обмена данными для мониторинга платы
  Serial.begin(115200);

  delay(2000);
  // -------------------Инициализация MIcroSD карты---------------------------
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(50);
  Serial.print("Initializing SD card...");
  if (!SD.begin(CS_PIN, SD_SCK_MHZ(1))) {
    Serial.println("initialization failed!");
  }
  else {
    Serial.println("initialization done.");
  }


  // ------------------- Подключение к Wi-Fi ---------------------------
  // Включаем Wi-Fi
  WiFi.mode(WIFI_STA);   // режим работы клиент_точка_доступа

  // Параметры клиента роутера
  //WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(WiFi_SSID, WiFi_password);

  // check wi-fi is connected to wi-fi network
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("STA FAILED");
  }

  Serial.print("STA IP: "); Serial.println(WiFi.localIP());
  Serial.print("GW: ");     Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");    Serial.println(WiFi.dnsIP());
  
  
  // **********    НАСТРОЙКА ДЛЯ СВЯЗИ С NTP       *************
  
  timeClient.begin();
  timeClient.setTimeOffset(10800); 
  uint32_t t0 = millis();
  while (millis() - t0 < 15000) {
    timeClient.update();
    if (timeClient.getEpochTime() > 1700000000UL) break;
    delay(200);
  }
  Serial.print("Epoch: ");
  Serial.println(timeClient.getEpochTime()); 
  /*
  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);  
 */


 
  // Создадим HTTP сервер
  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/hour", handleHour);
  server.begin();

}


void loop()
{

  server.handleClient();
  timeClient.update();

  //-------------------------- Обнуляем данные ---------------------

  getFormattedDate(strDate, sizeof(strDate));
  getFormattedTime(strTime, sizeof(strTime));

  int currentHour = timeClient.getHours();
  if(Hour == -1){
    Hour = currentHour;
  }
  else if(currentHour != Hour){
    Serial.print("Start write data on SD...");
    if (RMS_total > 0) {
      I_rms = sqrt(RMS_sum/(float)RMS_total);
    }
    Hour = currentHour;
    char date[11] = {};
    getFormattedDate(date, sizeof(date)/sizeof(char));
    logData(date, Hour, I_rms, I_min, time_min, I_max, time_max, (P_count/RMS_total));
    RMS_sum = 0;
    RMS_total = 0;
    I_min = 100;
    I_max = 0;
    I_rms = 0;
    P_count = 0;
    flag_alarm = 0;
  }

  // --------------------- Измерения ---------------------------
  float sum = 0;                           // Сумма измеренные токов за samples раз
  // измеряем величину тока
  for(int i = 0; i < samples; ++i){
    int val = analogRead(A0);
    int diff = val - (ADC_MAX / 2);
    sum += (float)(diff * diff);
    delay(1);
  }

  float mean = sqrt(sum / samples);
  float voltage = mean * ESP_VOLTAGE  / ADC_MAX;
  current = (voltage / RESISTANCE) * 2500;

  // Параметры для передачи на HTTP
  g_currentA = current;
  g_powerW   = current * VOLTAGE_MAIN;
  g_unix     = timeClient.getEpochTime();

  // ===== write 1 point per minute to hour buffer =====
  uint32_t now = g_unix;
  uint32_t minuteStamp = now / 60;   // номер минуты от эпохи
  if (minuteStamp != lastMinuteStamp) {
    lastMinuteStamp = minuteStamp;

    hour_I[hour_idx] = g_currentA;
    hour_P[hour_idx] = g_powerW;
    hour_t[hour_idx] = now;

    hour_idx = (hour_idx + 1) % HOUR_POINTS;
  }

  // Расчёт параметров для расчёта среднеквадратичного тока
  RMS_sum += current*current;
  ++RMS_total;

  // Минимум тока за прошедший час
  if (I_min > current){
    I_min = current;
    getFormattedTime(time_min, sizeof(time_min));
  }
  // Максимум тока за прошедший час
  if (I_max < current){
    I_max = current;
    getFormattedTime(time_max, sizeof(time_max));
  }

  // Мощность потреблённая за час
  P_count += current * VOLTAGE_MAIN;

  // Alarm по превышению тока
  if (I_max > MAX_CURRENT_VAL && flag_alarm == 0){
    flag_alarm = 1;
    printf("ALARM: current is more then 19 A !!!");
    // в идеале сделать что бы это как то сигнализировалось на лампочку или ПУШ на телефон или сообщение в HTTP интерфейсе
  }

}
  


/********************************************************         FUNCTIONS           ********************************************* */
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

  int year, month, day;
	sscanf(date, "%d-%d-%d", &year, &month, &day);
	
	snprintf(yearDir, sizeof(yearDir), "/%04d", year);
	snprintf(monthDir, sizeof(monthDir), "/%04d/%02d", year, month);
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

  auto dataFile = SD.open(fileCSV, O_WRONLY | O_CREAT | O_AT_END);
	
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