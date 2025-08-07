#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

#include <Prayer.h>

#include <DS3231.h>
#include <SPI.h>
#include <Wire.h>

#include <EEPROM.h>

#include <ESP8266WebServer.h>

#include <ArduinoOTA.h>
#include "DFRobotDFPlayerMini.h"

#include <SoftwareSerial.h>

#define BUZZ D7
#define PINLED D5
#define LEDS_PER_SEG 5
#define LEDS_PER_DOT 4
#define LEDS_PER_DIGIT  LEDS_PER_SEG *7
#define LED   148

char ssid[20]     = "JAM_WS2812";
char password[20] = "00000000";

const char* otaSsid = "KELUARGA02";
const char* otaPass = "suhartono";
const char* otaHost = "JAM-STRIP";

const long utcOffsetInSeconds = 25200;

RTClib RTC;
DS3231 Time;
DateTime now;
Prayer JWS;
Adafruit_NeoPixel strip(LED, PINLED, NEO_GRB + NEO_KHZ800);
DFRobotDFPlayerMini myDFPlayer;
WiFiUDP ntpUDP;
NTPClient Clock(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);
ESP8266WebServer server(80);
SoftwareSerial softSerial(D3, D4); // RX, TX
#define FPSerial softSerial

uint8_t h1;
uint8_t h2;
uint8_t m1;
uint8_t m2;
//int JW;
//int MW;
//int JR;
//int MR;

uint8_t dot1[]={70,71,72,73};
uint8_t dot2[]={74,75,76,77};

uint8_t hue;
uint8_t pixelColor;
uint8_t dotsOn = 0;

// MODE
bool modeWarnaOtomatis = true;
bool modeSetting = false;
bool modeSwitchTemp = true;

// MANUAL COLOR
uint8_t manualR = 255, manualG = 0, manualB = 0;

// ALARM
uint8_t alarm1Hour = 6, alarm1Minute = 0, alarm1Sound = 1;
uint8_t alarm2Hour = 12, alarm2Minute = 0, alarm2Sound = 2;

uint8_t brightness = 20;
uint8_t volumeDfPlayer;

// DFPlayer
bool isPlaying = false;

uint8_t lastHourlyPlay = 255;  // Nilai tidak valid awalnya

bool stateBuzzWar = false;

IPAddress local_IP(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

struct PanelSettings {
  bool modeWarnaOtomatis = true;
  bool modeSetting;
  bool modeSwitchTempp = true;
  bool modeOnline = false;
  uint8_t manualR, manualG, manualB;
  uint8_t alarm1Hour, alarm1Minute, alarm1Sound;
  uint8_t alarm2Hour, alarm2Minute, alarm2Sound;
  byte kecerahan;
  byte volumeDfplayer;
  
};

PanelSettings settings;

//=========================//
//==variabel alarm adzan===//
//=========================//
struct Config {
  uint8_t durasiadzan = 40;
  uint8_t altitude = 10;
  double latitude = -7.364057;
  double longitude = 112.646222;
  uint8_t zonawaktu = 7;
  bool       adzan         = 0;
  bool       reset_x       = 0; 
  uint8_t    sholatNow     = -1;
};

Config config;

uint8_t dataIhty[]      = {0,0,0,0,0,0};

long numberss[] = {
  //  7654321
  0b0111111,  // [0] 0
  0b0100001,  // [1] 1
  0b1110110,  // [2] 2
  0b1110011,  // [3] 3
  0b1101001,  // [4] 4
  0b1011011,  // [5] 5
  0b1011111,  // [6] 6
  0b0110001,  // [7] 7
  0b1111111,  // [8] 8
  0b1111011,  // [9] 9
  0b0000000,  // [10] off
  0b1111000,  // [11] degrees symbol
  0b0011110,  // [12] C(elsius)
  0b1011110,  // [13] E
  0b0111101,  // [14] n(N)
  0b1001110,  // [15] t
  0b1111110,  // [16] e
  0b1000101,  // [17] n
  0b1000100,  // [18] r
  0b1000111,  // [19] o
  0b1100111,  // [20] d
  0b0000001,  // [21] i
  0b1000110,  // [22] c
  0b1000000,  // [23] -
  0b1111101,  // [24] A
  0b1111100,  // [25] P
  0b1011011,  // [26] S
  //0b1111000   // [27] '
};

void handleSetTime() {
  //if (!server.hasArg("plain")) return server.send(400, "text/plain", "Bad Request");

  if (server.hasArg("PLAY")) {
    uint8_t track = server.arg("PLAY").toInt();
    Serial.println("[PLAY] Track: " + String(track));
    myDFPlayer.playFolder(2, track);
    isPlaying = true;

  } else if (server.hasArg("STOP")) {
    Serial.println("[STOP] DFPlayer stop");
    stopDFPlayer();

  } else if (server.hasArg("MODE")) {
    uint8_t data = server.arg("MODE").toInt();
    settings.modeOnline = data;
     Serial.println("[MODE ONLINE] Set to: " + String(data));
    saveSettings();
    delay(1000);
    ESP.restart();
    
  } else if (server.hasArg("BRIGHTNESS")) {
    brightness = server.arg("BRIGHTNESS").toInt();
    byte data = map(brightness, 0, 100, 1, 255);
    strip.setBrightness(data);
    Serial.println("[BRIGHTNESS] Set to: " + String(data));
    settings.kecerahan = data;
    saveSettings();

  } else if (server.hasArg("VOLUME")) {
    volumeDfPlayer = server.arg("VOLUME").toInt();
    byte data = map(volumeDfPlayer, 0, 100, 0, 29);
    myDFPlayer.volume(data);
    Serial.println("[VOLUME] Set to: " + String(data));
    settings.volumeDfplayer = data;
    saveSettings();

  } else if (server.hasArg("SET_TIME")) {
  String timeStr = server.arg("SET_TIME");
  Serial.println("[DEBUG] SET_TIME raw input: " + timeStr);

  int tahun, bulan, tanggal, dow, jam, menit, detik;

  // Parsing format: YYYY-MM-DD-DOW-HH:MM:SS
  int idx1 = timeStr.indexOf('-');
  int idx2 = timeStr.indexOf('-', idx1 + 1);
  int idx3 = timeStr.indexOf('-', idx2 + 1);
  int idx4 = timeStr.indexOf('-', idx3 + 1);
  int idx5 = timeStr.indexOf(':', idx4 + 1);
  int idx6 = timeStr.indexOf(':', idx5 + 1);

  if (idx1 > 0 && idx2 > idx1 && idx3 > idx2 && idx4 > idx3 && idx5 > idx4 && idx6 > idx5) {
    tahun   = timeStr.substring(0, idx1).toInt();
    bulan   = timeStr.substring(idx1 + 1, idx2).toInt();
    tanggal = timeStr.substring(idx2 + 1, idx3).toInt();
    dow     = timeStr.substring(idx3 + 1, idx4).toInt();
    jam     = timeStr.substring(idx4 + 1, idx5).toInt();
    menit   = timeStr.substring(idx5 + 1, idx6).toInt();
    detik   = timeStr.substring(idx6 + 1).toInt();

    // Set RTC atau waktu sistem Anda
    // Set ke waktu RTC/objek Time Anda
    Time.setYear(tahun);
    Time.setMonth(bulan);
    Time.setDate(tanggal);
    Time.setDoW(dow);
    Time.setHour(jam);
    Time.setMinute(menit);
    Time.setSecond(detik);

    // Tampilkan ke Serial Monitor
//    Serial.println("[SET_TIME] Waktu disetel:");
//    Serial.println("  Tanggal : " + String(tanggal) + "-" + String(bulan) + "-" + String(tahun));
//    Serial.println("  Hari (DoW): " + String(dow));
//    Serial.println("  Jam     : " + String(jam) + ":" + String(menit) + ":" + String(detik));
  } else {
    //Serial.println("[SET_TIME] Format salah! Gunakan: YYYY-MM-DD-DOW-HH:MM:SS");
  }
}

 else if (server.hasArg("COLOR")) {
  String colorStr = server.arg("COLOR");
  //Serial.println("[DEBUG] COLOR raw input: " + colorStr);

  // Parsing dengan format: COLOR=R,G,B
  int r = 0, g = 0, b = 0;
  int idx1 = colorStr.indexOf(',');
  int idx2 = colorStr.indexOf(',', idx1 + 1);

  if (idx1 > 0 && idx2 > idx1) {
    r = colorStr.substring(0, idx1).toInt();
    g = colorStr.substring(idx1 + 1, idx2).toInt();
    b = colorStr.substring(idx2 + 1).toInt();

    manualR = r;
    manualG = g;
    manualB = b;
    modeWarnaOtomatis = false;

    Serial.println("[COLOR] R: " + String(r) + " G: " + String(g) + " B: " + String(b));

    settings.modeWarnaOtomatis = modeWarnaOtomatis;
    settings.manualR = r;
    settings.manualG = g;
    settings.manualB = b;
    saveSettings();
  } else {
    Serial.println("[COLOR] Invalid format! Use R,G,B");
  }
}
 else if (server.hasArg("AUTO_COLOR")) {
    modeWarnaOtomatis = true;
    Serial.println("[AUTO_COLOR] Enabled");
    settings.modeWarnaOtomatis = modeWarnaOtomatis;
    saveSettings();

  } else if (server.hasArg("MODE_SWITCH")) {
    modeSwitchTemp = server.arg("MODE_SWITCH").toInt();
    Serial.println("[MODE_SWITCH] Mode: " + String(modeSwitchTemp));
    settings.modeSwitchTempp = modeSwitchTemp;
    saveSettings();

  } else if (server.hasArg("ALARM1")) {
  String data = server.arg("ALARM1");
  int colon1 = data.indexOf(':');
  int colon2 = data.indexOf(':', colon1 + 1);

  if (colon1 != -1 && colon2 != -1) {
    alarm1Hour = data.substring(0, colon1).toInt();
    alarm1Minute = data.substring(colon1 + 1, colon2).toInt();
    alarm1Sound = data.substring(colon2 + 1).toInt();

    Serial.println("[ALARM1] " + String(alarm1Hour) + ":" + String(alarm1Minute) + " Sound: " + String(alarm1Sound));

    settings.alarm1Hour = alarm1Hour;
    settings.alarm1Minute = alarm1Minute;
    settings.alarm1Sound = alarm1Sound;
    saveSettings();
  } else {
    Serial.println("[ALARM1] Format salah. Gunakan ALARM1=HH:MM:SOUND");
  }

} else if (server.hasArg("ALARM2")) {
  String data = server.arg("ALARM2");
  int colon1 = data.indexOf(':');
  int colon2 = data.indexOf(':', colon1 + 1);

  if (colon1 != -1 && colon2 != -1) {
    alarm2Hour = data.substring(0, colon1).toInt();
    alarm2Minute = data.substring(colon1 + 1, colon2).toInt();
    alarm2Sound = data.substring(colon2 + 1).toInt();

    Serial.println("[ALARM2] " + String(alarm2Hour) + ":" + String(alarm2Minute) + " Sound: " + String(alarm2Sound));

    settings.alarm2Hour = alarm2Hour;
    settings.alarm2Minute = alarm2Minute;
    settings.alarm2Sound = alarm2Sound;
    saveSettings();
  } else {
    Serial.println("[ALARM2] Format salah. Gunakan ALARM2=HH:MM:SOUND");
  }
}


  server.send(200, "text/plain", "OK");
}

void handleGetData() {
  now = RTC.now();
  uint8_t jam = now.hour();
  uint8_t menit = now.minute();
  uint8_t detik = now.second();

  // Ambil suhu (misalnya dari variabel global suhuSensor)
  float suhu = getTempp(); // Pastikan variabel ini diperbarui rutin dari sensor

  // Kirim data ke client
  String response = "JAM=" + String(jam) + ":" + String(menit) + ":" + String(detik);
  response += " SUHU=" + String(suhu, 1);
  server.send(200, "text/plain", response);
}


void AP_init() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  server.on("/set", handleSetTime);       // endpoint untuk set waktu
  server.on("/getdata", handleGetData);   // endpoint baru untuk ambil jam & suhu
  server.begin();
}


void ONLINE() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(otaSsid, otaPass);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("OTA WiFi gagal. Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(otaHost);
 
  ArduinoOTA.onEnd([]() {
    Serial.println("restart");
    settings.modeOnline = 0;
    saveSettings();
    delay(1000);
    ESP.restart();
  });
  
  ArduinoOTA.begin();
  //Serial.println("OTA Ready");
}

void saveSettings() {
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void loadSettings() {
  EEPROM.get(0, settings); // Load struct dari EEPROM address 0

  // Simpan ke variabel global
  modeWarnaOtomatis = settings.modeWarnaOtomatis;
  manualR = settings.manualR;
  manualG = settings.manualG;
  manualB = settings.manualB;
  alarm1Hour = settings.alarm1Hour;
  alarm1Minute = settings.alarm1Minute;
  alarm1Sound = settings.alarm1Sound;
  alarm2Hour = settings.alarm2Hour;
  alarm2Minute = settings.alarm2Minute;
  alarm2Sound = settings.alarm2Sound;
  brightness = settings.kecerahan;
  volumeDfPlayer = settings.volumeDfplayer;
  modeSwitchTemp = settings.modeSwitchTempp;

  // Tampilkan semua nilai ke Serial Monitor
//  Serial.println(F("=== Data dari EEPROM ==="));
//  Serial.println("modeOnline        : " + String(settings.modeOnline));
//  Serial.println("modeSetting       : " + String(settings.modeSetting));
//  Serial.println("modeWarnaOtomatis : " + String(settings.modeWarnaOtomatis));
//  Serial.println("manualR           : " + String(settings.manualR));
//  Serial.println("manualG           : " + String(settings.manualG));
//  Serial.println("manualB           : " + String(settings.manualB));
//  Serial.println("alarm1Hour        : " + String(settings.alarm1Hour));
//  Serial.println("alarm1Minute      : " + String(settings.alarm1Minute));
//  Serial.println("alarm1Sound       : " + String(settings.alarm1Sound));
//  Serial.println("alarm2Hour        : " + String(settings.alarm2Hour));
//  Serial.println("alarm2Minute      : " + String(settings.alarm2Minute));
//  Serial.println("alarm2Sound       : " + String(settings.alarm2Sound));
//  Serial.println("kecerahan         : " + String(settings.kecerahan));
//  Serial.println("volumeDfplayer    : " + String(settings.volumeDfplayer));
//  Serial.println("modeSwitchTemp    : " + String(settings.modeSwitchTempp));
//  Serial.println(F("========================\n"));
}

void setup() {
  Serial.begin(115200);
  FPSerial.begin(9600);
  EEPROM.begin(512);  // Inisialisasi EEPROM
  pinMode(BUZZ,OUTPUT);
  digitalWrite(BUZZ, HIGH);
  
 if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  
  
  strip.begin();
  Wire.begin();
  loadSettings();     // Load data ke variabel
  
  strip.setBrightness(brightness);
//  myDFPlayer.volume(volumeDfPlayer);     // Volume dari 0 - 30
  settings.modeOnline?ONLINE() : AP_init();
  Serial.println();
  Serial.println("READY");
}

void loop() {
  if (settings.modeOnline) {
    ArduinoOTA.handle();  // OTA update jika mode online
    buzzerUpload(1000);
    return;
  }

  // Mode Offline
  checkClientConnected();  // Cek koneksi client

  if (modeSetting) {
    server.handleClient();  // Web config aktif jika mode setting
    buzzerUpload(250);
    return;
  }

  // Mode normal (offline + tidak setting)
  getClockRTC();    // Ambil waktu dari RTC
  timerHue();       // Update warna
  islam();          // Fungsi terkait waktu sholat
  check();          // Cek parameter lainnya
  buzzerWarning(stateBuzzWar);

  static uint32_t lastToggle = 0;
  static bool toggleState = false;
  uint32_t now = millis();

  if (modeSwitchTemp) {
    // Tampilkan jam dan suhu bergantian setiap 10 detik
    if (now - lastToggle > 10000) {
      toggleState = !toggleState;
      lastToggle = now;
    }

    if (toggleState) {
      showClock(getCurrentColor());
      showDots(0xFF0000); // Titik merah
    } else {
      showTemp();
      showDots(0x000000); // Titik mati
    }
  } else {
    // Tampilkan jam saja
    showClock(getCurrentColor());
    showDots(0xFF0000); // Titik merah
  }

  // Pengecekan alarm dan bunyi jam
  checkAlarm();
  checkHourlyChime();
  
}


// =========================
// Cek apakah ada client di AP
// =========================
void checkClientConnected() {
  static uint32_t lastCheck = 0;
  static uint8_t lastClientCount = 0;
  
  if (millis() - lastCheck > 2000) { // Cek tiap 2 detik
    lastCheck = millis();

    uint8_t clientCount = WiFi.softAPgetStationNum();
 
    if (clientCount != lastClientCount) {
      
      clientCount==1?modeSetting = 1 : modeSetting = 0;
      //Serial.println("clientCount: " + String(clientCount));
      strip.clear();
      strip.show();
      digitalWrite(BUZZ, HIGH);
      lastClientCount = clientCount;
    }
    
  }
}

void showClock(uint32_t color) {
  DisplayNumber(h1, 3, color);
  DisplayNumber(h2, 2, color);
  DisplayNumber(m1, 1, color);
  DisplayNumber(m2, 0, color);
}

void showTemp(){
  DisplayNumber(getTempp() / 10,3,strip.Color(0,255,0));
  DisplayNumber(getTempp() % 10,2,strip.Color(0,255,0));
  DisplayNumber(11       ,1,strip.Color(0,255,0));
  DisplayNumber(12       ,0,strip.Color(255,0,0));
}

void checkHourlyChime() {
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastHourlyPlay) {
    uint8_t jam = now.hour() % 12;
    if (jam == 0) jam = 12;  // Ubah 0 jadi 12
    //Serial.println("update jam: " + String(jam));
    myDFPlayer.playFolder(1,jam);   // Misalnya track 1-12 adalah suara jam
   // isPlaying = true;
    lastHourlyPlay = now.hour();  // Simpan jam terakhir dimainkan
  }
}

void checkAlarm() {
  //if (!isPlaying) {
    if (now.hour() == alarm1Hour && now.minute() == alarm1Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,alarm1Sound);
     // isPlaying = true;
     // Serial.println("ALARM 1 RUN");
    }
    if (now.hour() == alarm2Hour && now.minute() == alarm2Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,alarm2Sound);
     // isPlaying = true;
      //Serial.println("ALARM 2 RUN");
    }
 // }
}

void stopDFPlayer() {
  myDFPlayer.stop();
  isPlaying = false;
}

uint32_t getCurrentColor() {
  if (modeWarnaOtomatis) {
    return Wheel((hue + pixelColor) & 255);
  } else {
    return strip.Color(manualR, manualG, manualB);
  }
}

void DisplayNumber(byte number, byte segment, uint32_t color) {
  byte startindex = segment * LEDS_PER_DIGIT;
  if (segment >= 2) startindex += LEDS_PER_DOT * 2;  // Lewati dot setelah digit 1

  uint8_t segBits = numberss[number];

  for (byte i = 0; i < 7; i++) {           // 7 segments
    for (byte j = 0; j < LEDS_PER_SEG; j++) {         // LEDs per segment
      strip.setPixelColor(i * LEDS_PER_SEG + j + startindex , (numberss[number] & 1 << i) == 1 << i ? color : strip.Color(0, 0, 0));
    }
  }
  strip.show();
}


/*
void DisplayNumber(byte number, byte segment, uint32_t color) {
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = LEDS_PER_DIGIT;
      break;
    case 2:
      startindex = LEDS_PER_DIGIT * 2 + LEDS_PER_DOT * 2;
      break;
    case 3:
      startindex = LEDS_PER_DIGIT * 3 + LEDS_PER_DOT * 2;
      break;
  }

  for (byte i = 0; i < 7; i++) {           // 7 segments
    for (byte j = 0; j < LEDS_PER_SEG; j++) {         // LEDs per segment
      strip.setPixelColor(i * LEDS_PER_SEG + j + startindex , (numberss[number] & 1 << i) == 1 << i ? color : strip.Color(0, 0, 0));
      strip.show();
    }
  }
}
*/

void getClockRTC() 
{
  now = RTC.now();
  h1 = now.hour() / 10;
  h2 = now.hour() % 10;
  m1 = now.minute() / 10;
  m2 = now.minute() % 10;
}

void getClockNTP()
{
  Clock.update();
  h1 = Clock.getHours() / 10;
  h2 = Clock.getHours() % 10;
  m1 = Clock.getMinutes() / 10;
  m2 = Clock.getMinutes() % 10;
}

int getTempp(){
  return Time.getTemperature();
}

void showDots(uint32_t color) {
  now = RTC.now();
  bool isOn = now.second() % 2;

  uint32_t col = isOn ? color : strip.Color(0, 0, 0);
  for (int i = 70; i <= 77; i++) {
    strip.setPixelColor(i, col);
  }

  strip.show();
}

void timerHue() {
  const uint8_t delayHue = 8;
  static uint32_t tmrsaveHue = 0;
  uint32_t tmr = millis();

  if (tmr - tmrsaveHue > delayHue) {
    tmrsaveHue = tmr;

    pixelColor++;
    if (pixelColor >= 255) pixelColor = 0;
    
  }
  hue++;
  if (hue >= strip.numPixels()) hue = 0;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void buzzerUpload(uint16_t Delay){
    if(config.adzan) return;
    
    static bool state;
    static uint32_t save = 0;
    static uint8_t  con = 0;
    uint32_t tmr = millis();
    
    if(tmr - save > Delay ){
      save = tmr;
      state = !state;
      digitalWrite(BUZZ, state);
      
    }
}

void buzzerWarning(int cek){
   if(!config.adzan) return;
   
   static bool state = false;
   static uint32_t save = 0;
   uint32_t tmr = millis();
   static uint8_t con = 0;
    
    if(tmr - save > 500 && cek == 1){//2500
      save = tmr;
      state = !state;
      digitalWrite(BUZZ, state);
      //Serial.println("active");
      if(con <= 60) { con++; }
      if(con == 61) { cek = 0; con = 0; state = true; stateBuzzWar = 0; config.adzan = 0;}
      //Serial.println("con:" + String(con));
    } 
    
}

/*
void Buzzer(uint8_t state)
  {
    if(!stateBuzzer) return;
    
    switch(state){
      case 0 :
        digitalWrite(BUZZ,HIGH);
      break;
      case 1 :
        digitalWrite(BUZZ,LOW);
      break;
    };
  }
*/
