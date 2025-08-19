#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

#include <Prayer.h>

#include <DS3231.h>
#include <SPI.h>
#include <Wire.h>

#include <ESP_EEPROM.h>

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

#define ADDR_MODE_ONLINE      0
#define ADDR_KECERAHAN        1
#define ADDR_VOLUME           2
#define ADDR_MODE_WARNA       3
#define ADDR_MANUAL_R         4
#define ADDR_MANUAL_G         5
#define ADDR_MANUAL_B         6
#define ADDR_MODE_SWITCH      7
#define ADDR_ALARM1_HOUR      8
#define ADDR_ALARM1_MINUTE    9
#define ADDR_ALARM1_SOUND     10
#define ADDR_ALARM2_HOUR      11
#define ADDR_ALARM2_MINUTE    12
#define ADDR_ALARM2_SOUND     13

// Tambahan untuk setengah jam
#define ADDR_HALF_HOUR_BASE   14   // alamat awal untuk chime setengah jam
#define HALF_HOUR_COUNT       24   // jumlah data (1 data = 1 jam, otomatis 2x main tiap jam)

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

byte h1;
byte h2;
byte m1;
byte m2;

byte dot1[]={70,71,72,73};
byte dot2[]={74,75,76,77};

uint16_t hue;
uint16_t pixelColor;
byte dotsOn = 0;

// MODE
//bool modeWarnaOtomatis = true;
bool modeSetting = false;
//bool modeSwitchTemp = true;

// Untuk kontrol urutan suara startup
byte startupStage = 0;
unsigned long lastVoiceMillis = 0;
bool startupSelesai = false;

// DFPlayer
bool isPlaying = false;

int currentVolume = -1;  // simpan volume terakhir yg dikirim ke DFPlayer

// --- Variabel global ---
int lastHourlyPlay = -1;
int lastHalfPlay = -1;

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
  byte kecerahan=50;
  byte volumeDfplayer=20;
  
};

PanelSettings settings;
#define EEPROM_START 0

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
  if (server.hasArg("PLAY")) {
    uint8_t track = server.arg("PLAY").toInt();
    //Serial.println("[PLAY] Track: " + String(track));
    if(isPlaying) stopDFPlayer();
    delay(100);
    myDFPlayer.playFolder(2, track);
    isPlaying = true;
    server.send(200, "text/plain", "OK");
    return;
  }

  if (server.hasArg("STOP")) {
    Serial.println("[STOP] DFPlayer stop");
    stopDFPlayer();
    server.send(200, "text/plain", "OK");
    return;
  }

  if (server.hasArg("MODE")) {
    byte mode = server.arg("MODE").toInt();
    settings.modeOnline = mode;
    saveByteToEEPROM(ADDR_MODE_ONLINE, mode);
    //Serial.println("[MODE ONLINE] Set to: " + String(mode));
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  }

  if (server.hasArg("BRIGHTNESS")) {
    int brightnessInput = server.arg("BRIGHTNESS").toInt();
    byte mapped = map(brightnessInput, 0, 100, 1, 255);
    settings.kecerahan = mapped;
    strip.setBrightness(mapped);
    delay(100);
    saveByteToEEPROM(ADDR_KECERAHAN, mapped);
    //Serial.println("[BRIGHTNESS] Set to: " + String(mapped));
    server.send(200, "text/plain", "OK");
  }

  if (server.hasArg("VOLUME")) {
    int volInput = server.arg("VOLUME").toInt();
    byte mapped = map(volInput, 0, 100, 0, 29);
    stopDFPlayer();
    myDFPlayer.volume(mapped);
    delay(100);
    saveByteToEEPROM(ADDR_VOLUME, mapped);
    //Serial.println("[VOLUME] Set to: " + String(volInput));
    server.send(200, "text/plain", "OK");
  }
 

  if (server.hasArg("COLOR")) {
    String colorStr = server.arg("COLOR");
    int idx1 = colorStr.indexOf(',');
    int idx2 = colorStr.indexOf(',', idx1 + 1);
    if (idx1 > 0 && idx2 > idx1) {
      byte r = colorStr.substring(0, idx1).toInt();
      byte g = colorStr.substring(idx1 + 1, idx2).toInt();
      byte b = colorStr.substring(idx2 + 1).toInt();

      settings.manualR = r;
      settings.manualG = g;
      settings.manualB = b;
      settings.modeWarnaOtomatis = false;

      saveByteToEEPROM(ADDR_MANUAL_R, r);
      saveByteToEEPROM(ADDR_MANUAL_G, g);
      saveByteToEEPROM(ADDR_MANUAL_B, b);
      saveByteToEEPROM(ADDR_MODE_WARNA, 0); // 0 = manual

      //Serial.printf("[COLOR] R:%d G:%d B:%d\n", r, g, b);
      server.send(200, "text/plain", "OK");
    } else {
      Serial.println("[COLOR] Format salah. Gunakan R,G,B");
    }
  }

  if (server.hasArg("AUTO_COLOR")) {
    settings.modeWarnaOtomatis = true;
    saveByteToEEPROM(ADDR_MODE_WARNA, 1); // 1 = otomatis
    //Serial.println("[AUTO_COLOR] Enabled");
    server.send(200, "text/plain", "OK");
  }

  if (server.hasArg("MODE_SWITCH")) {
    byte mode = server.arg("MODE_SWITCH").toInt();
    settings.modeSwitchTempp = mode;
    saveByteToEEPROM(ADDR_MODE_SWITCH, mode);
    //Serial.println("[MODE_SWITCH] Mode: " + String(mode));
    server.send(200, "text/plain", "OK");
  }

  if (server.hasArg("ALARM1")) {
    String data = server.arg("ALARM1");
    int colon1 = data.indexOf(':');
    int colon2 = data.indexOf(':', colon1 + 1);
    if (colon1 != -1 && colon2 != -1) {
      byte h = data.substring(0, colon1).toInt();
      byte m = data.substring(colon1 + 1, colon2).toInt();
      byte s = data.substring(colon2 + 1).toInt();

      settings.alarm1Hour = h;
      settings.alarm1Minute = m;
      settings.alarm1Sound = s;

      saveByteToEEPROM(ADDR_ALARM1_HOUR, h);
      saveByteToEEPROM(ADDR_ALARM1_MINUTE, m);
      saveByteToEEPROM(ADDR_ALARM1_SOUND, s);

      //Serial.printf("[ALARM1] %02d:%02d Sound:%d\n", h, m, s);
      server.send(200, "text/plain", "OK");
    } else {
      Serial.println("[ALARM1] Format salah. Gunakan HH:MM:SOUND");
    }
  }

  if (server.hasArg("ALARM2")) {
    String data = server.arg("ALARM2");
    int colon1 = data.indexOf(':');
    int colon2 = data.indexOf(':', colon1 + 1);
    if (colon1 != -1 && colon2 != -1) {
      byte h = data.substring(0, colon1).toInt();
      byte m = data.substring(colon1 + 1, colon2).toInt();
      byte s = data.substring(colon2 + 1).toInt();

      settings.alarm2Hour = h;
      settings.alarm2Minute = m;
      settings.alarm2Sound = s;

      saveByteToEEPROM(ADDR_ALARM2_HOUR, h);
      saveByteToEEPROM(ADDR_ALARM2_MINUTE, m);
      saveByteToEEPROM(ADDR_ALARM2_SOUND, s);

      //Serial.printf("[ALARM2] %02d:%02d Sound:%d\n", h, m, s);
      server.send(200, "text/plain", "OK");
    } else {
      Serial.println("[ALARM2] Format salah. Gunakan HH:MM:SOUND");
    }
  }

  if (server.hasArg("SET_TIME")) {
    String timeStr = server.arg("SET_TIME");
    int tahun, bulan, tanggal, dow, jam, menit, detik;

    int idx1 = timeStr.indexOf('-');
    int idx2 = timeStr.indexOf('-', idx1 + 1);
    int idx3 = timeStr.indexOf('-', idx2 + 1);
    int idx4 = timeStr.indexOf('-', idx3 + 1);
    int idx5 = timeStr.indexOf(':', idx4 + 1);
    int idx6 = timeStr.indexOf(':', idx5 + 1);

    if (idx6 != -1) {
      tahun   = timeStr.substring(0, idx1).toInt();
      bulan   = timeStr.substring(idx1 + 1, idx2).toInt();
      tanggal = timeStr.substring(idx2 + 1, idx3).toInt();
      dow     = timeStr.substring(idx3 + 1, idx4).toInt();
      jam     = timeStr.substring(idx4 + 1, idx5).toInt();
      menit   = timeStr.substring(idx5 + 1, idx6).toInt();
      detik   = timeStr.substring(idx6 + 1).toInt();

      Time.setYear(tahun);
      Time.setMonth(bulan);
      Time.setDate(tanggal);
      Time.setDoW(dow);
      Time.setHour(jam);
      Time.setMinute(menit);
      Time.setSecond(detik);

      //Serial.println("[SET_TIME] OK");
      server.send(200, "text/plain", "OK");
    } else {
      Serial.println("[SET_TIME] Format salah!");
    }
  }

    if (server.hasArg("HALFCHIME")) {
    String data = server.arg("HALFCHIME");
    int colon = data.indexOf(':');
    if (colon != -1) {
      byte h = data.substring(0, colon).toInt();
      byte f = data.substring(colon + 1).toInt();

      if (h < 24) {
        saveHalfHourChime(h, f);
        server.send(200, "text/plain", "OK");
        Serial.printf("[HALFCHIME] Hour:%d File:%d\n", h, f);
      } else {
        server.send(400, "text/plain", "Hour invalid (0-23)");
      }
    } else {
      server.send(400, "text/plain", "Format salah. Gunakan HOUR:FILE");
    }
  }

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
    saveByteToEEPROM(ADDR_MODE_ONLINE, 0);
    delay(1000);
    ESP.restart();
  });
  
  ArduinoOTA.begin();
  //Serial.println("OTA Ready");
}

void saveByteToEEPROM(int address, byte value) {
  EEPROM.write(address, value);
  EEPROM.commit();
}

void saveHalfHourChime(uint8_t hour, uint8_t fileIndex) {
  if (hour < 24) {
    EEPROM.write(ADDR_HALF_HOUR_BASE + hour, fileIndex);
    EEPROM.commit();
  }
}

uint8_t loadHalfHourChime(uint8_t hour) {
  if (hour < 24) {
    return EEPROM.read(ADDR_HALF_HOUR_BASE + hour);
  }
  return 0;
}


void loadSettings() {
  settings.modeOnline        = EEPROM.read(ADDR_MODE_ONLINE);
  settings.kecerahan         = EEPROM.read(ADDR_KECERAHAN);
  settings.volumeDfplayer    = EEPROM.read(ADDR_VOLUME);
  settings.modeWarnaOtomatis = EEPROM.read(ADDR_MODE_WARNA);
  settings.manualR           = EEPROM.read(ADDR_MANUAL_R);
  settings.manualG           = EEPROM.read(ADDR_MANUAL_G);
  settings.manualB           = EEPROM.read(ADDR_MANUAL_B);
  settings.modeSwitchTempp   = EEPROM.read(ADDR_MODE_SWITCH);
  settings.alarm1Hour        = EEPROM.read(ADDR_ALARM1_HOUR);
  settings.alarm1Minute      = EEPROM.read(ADDR_ALARM1_MINUTE);
  settings.alarm1Sound       = EEPROM.read(ADDR_ALARM1_SOUND);
  settings.alarm2Hour        = EEPROM.read(ADDR_ALARM2_HOUR);
  settings.alarm2Minute      = EEPROM.read(ADDR_ALARM2_MINUTE);
  settings.alarm2Sound       = EEPROM.read(ADDR_ALARM2_SOUND);

  Serial.println(F("\n[EEPROM] === SETTINGS LOADED ==="));
  Serial.println("[EEPROM] modeOnline        : " + String(settings.modeOnline));
  Serial.println("[EEPROM] kecerahan         : " + String(settings.kecerahan));
  Serial.println("[EEPROM] volumeDfplayer    : " + String(settings.volumeDfplayer));
  Serial.println("[EEPROM] modeWarnaOtomatis : " + String(settings.modeWarnaOtomatis));
  Serial.println("[EEPROM] manualR           : " + String(settings.manualR));
  Serial.println("[EEPROM] manualG           : " + String(settings.manualG));
  Serial.println("[EEPROM] manualB           : " + String(settings.manualB));
  Serial.println("[EEPROM] modeSwitchTempp   : " + String(settings.modeSwitchTempp));
  Serial.println("[EEPROM] alarm1Hour        : " + String(settings.alarm1Hour));
  Serial.println("[EEPROM] alarm1Minute      : " + String(settings.alarm1Minute));
  Serial.println("[EEPROM] alarm1Sound       : " + String(settings.alarm1Sound));
  Serial.println("[EEPROM] alarm2Hour        : " + String(settings.alarm2Hour));
  Serial.println("[EEPROM] alarm2Minute      : " + String(settings.alarm2Minute));
  Serial.println("[EEPROM] alarm2Sound       : " + String(settings.alarm2Sound));
  Serial.println(F("[EEPROM] ========================\n"));
}
void setup() {
  Serial.begin(115200);
  FPSerial.begin(9600);
  EEPROM.begin(64); // atau sesuai kebutuhan

  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, HIGH);

  if (!myDFPlayer.begin(FPSerial, true, true)) {
    Serial.println(F("Unable to begin DFPlayer."));
    while (true);
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.setTimeOut(500);

  // === SYSTEM STARTUP SOUND SEQUENCE ===
  myDFPlayer.volume(30);  // volume awal sementara, nanti diganti dari EEPROM
  delay(500);
  myDFPlayer.playFolder(3, 1); // 001_menyiapkan system.wav
  delay(5000); // sesuaikan dengan durasi file
  myDFPlayer.playFolder(3, 2); // 002_penyiapan selesai.wav
  delay(5000);

  // === LOAD SETTINGS ===
  loadSettings();
  strip.begin();
  Wire.begin();
  strip.setBrightness(settings.kecerahan);
  

  // === MODE CONNECTION ===
  if (settings.modeOnline) {
    myDFPlayer.playFolder(3, 5); // 003_menghubungkan wifi.wav
    delay(2500);
    ONLINE();
    delay(500);
    myDFPlayer.playFolder(3, 4); // 004_wifi connect.wav
  } else {
    myDFPlayer.playFolder(3, 7); // 006_mulai mode AP.wav
    delay(4000);
    AP_init();
  }

  delay(2000);
  myDFPlayer.playFolder(3, 3); // 007_jam menyala.wav
  delay(3000);
  myDFPlayer.volume(settings.volumeDfplayer);
  delay(1000);
  Serial.println();
  Serial.println("READY");
}

void loop() {
  // --- Mode Online ---
  if (settings.modeOnline) {
    ArduinoOTA.handle();  // OTA update
    buzzerUpload(1000);
    return;
  }

  // --- Mode Offline + Setting ---
  checkClientConnected();  
  if (modeSetting) {
    server.handleClient();
    buzzerUpload(250);
    return;
  }

  // --- Mode Normal ---
  timerHue();
  buzzerWarning(stateBuzzWar);

  // Jika sedang memutar file DFPlayer
  /*if (isPlaying) {
    // Cek apakah DFPlayer sudah selesai memutar file
    if (myDFPlayer.available()) {
      uint8_t type = myDFPlayer.readType();
      if (type == DFPlayerPlayFinished) {
        isPlaying = false;
        Serial.println("DFPlayer: Playback finished.");
      }
    }
//    // Saat isPlaying true, kita bisa tetap tampilkan jam atau animasi tertentu
//    showClock(getCurrentColor());
//    showDots(0xFF0000);
//    return; // Skip sisa proses normal
  }*/

  // --- Tampilan Normal (tidak sedang memutar audio) ---
  static unsigned long lastToggle = 0;
  static bool toggleState = false;
  unsigned long now = millis();

  if (settings.modeSwitchTempp) {
    if (now - lastToggle > 15000) {
      toggleState = !toggleState;
      lastToggle = now;
    }

    if (toggleState) {
      showClock(getCurrentColor());
      showDots(0xFF0000);
    } else {
      showTemp();
      showDots(0x000000);
    }
  } else {
    showClock(getCurrentColor());
    showDots(0xFF0000);
  }

  checkAlarm();
  checkHourlyChime();
}


void updateVolumeByTime(uint8_t hour) {
  int targetVolume;

  if (hour >= 22 || hour < 4) {
    targetVolume = 5; // volume malam
  } else {
    targetVolume = settings.volumeDfplayer; // volume normal dari aplikasi
  }

  // hanya update kalau berbeda dengan volume terakhir
  if (currentVolume != targetVolume) {
    myDFPlayer.volume(targetVolume);
    currentVolume = targetVolume;
    Serial.println("Volume update: " + String(targetVolume));
  }
}

/*
void loop() {
  // --- kode loop utama programmu mulai jalan di sini ---

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

  timerHue();       // Update warna
  //islam();          // Fungsi terkait waktu sholat
  //check();          // Cek parameter lainnya
  buzzerWarning(stateBuzzWar);

  static unsigned long lastToggle = 0;
  static bool toggleState = false;
  unsigned long now = millis();
 
  if (settings.modeSwitchTempp) {
    // Tampilkan jam dan suhu bergantian setiap 10 detik
    if (now - lastToggle > 15000) {
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
  
}*/


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
  getClockRTC();    // Ambil waktu dari RTC
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

// --- Fungsi cek bunyi jam & setengah jam ---
void checkHourlyChime() {
  now = RTC.now();
  updateVolumeByTime(now.hour());
  // Bunyi jam tepat
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastHourlyPlay) {
    uint8_t jam = now.hour() % 12;
    if (jam == 0) { jam = 12; }
    myDFPlayer.playFolder(1, jam);  // Folder 1 = suara jam 1-12
    delay(50);
    isPlaying = true;
    lastHourlyPlay = now.hour();
  }

  // Bunyi setengah jam
  if (now.minute() == 30 && now.second() == 0 && now.hour() != lastHalfPlay) {
    uint8_t fileIndex = loadHalfHourChime(now.hour()); // ambil setting user dari EEPROM
    if (fileIndex > 0) {
      myDFPlayer.playFolder(2, fileIndex);  // Misal folder 2 = koleksi bunyi setengah jam
      delay(50);
      isPlaying = true;
    }
    lastHalfPlay = now.hour();
  }
}

/*void checkHourlyChime() {
  now = RTC.now();
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastHourlyPlay ) {//&& !isPlaying
    uint8_t jam = now.hour() % 12;
    if (jam == 0) {jam = 12; } // Ubah 0 jadi 12
    //Serial.println("update jam: " + String(jam));
    myDFPlayer.playFolder(1,jam);   // Misalnya track 1-12 adalah suara jam
    delay(50);
    isPlaying = true;
    lastHourlyPlay = now.hour();  // Simpan jam terakhir dimainkan
  }
}*/

void checkAlarm() {
    now = RTC.now();
  //if (!isPlaying) {
    if (now.hour() == settings.alarm1Hour && now.minute() == settings.alarm1Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,settings.alarm1Sound);
      delay(50);
      isPlaying = true;
     // Serial.println("ALARM 1 RUN");
    }
    if (now.hour() == settings.alarm2Hour && now.minute() == settings.alarm2Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,settings.alarm2Sound);
      delay(50);
      isPlaying = true;
      //Serial.println("ALARM 2 RUN");
    }
 // }
}

void stopDFPlayer() {
  myDFPlayer.stop();
  isPlaying = false;
}

uint32_t getCurrentColor() {
  if (settings.modeWarnaOtomatis) {
    return Wheel((hue + pixelColor) & 255);
  } else {
    return strip.Color(settings.manualR, settings.manualG, settings.manualB);
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
  now = RTC.now();
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
  const uint8_t delayHue = 20;
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
