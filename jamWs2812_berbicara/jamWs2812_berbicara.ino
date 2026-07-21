#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <DS3231.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h> // Gunakan library standar EEPROM
#include <ESP8266WebServer.h>
#include "DFRobotDFPlayerMini.h"

// --- KONFIGURASI PIN ---
#define BUZZ D5 // DIPINDAH dari D7 ke D5 (D7 dipakai untuk DFPlayer RX)
#define PINLED D6
#define LEDS_PER_SEG 5
#define LEDS_PER_DOT 4
#define LEDS_PER_DIGIT (LEDS_PER_SEG * 7)
#define LED 148

// --- ALAMAT EEPROM ---
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
#define ADDR_HALF_HOUR_BASE   14

// --- WIFI OTA ---
const char* ssid     = "JAM_WS2812";
const char* password = "00000000";
const char* otaSsid  = "AUTOBACKUP";
const char* otaPass  = "IA051510";

const long utcOffsetInSeconds = 25200;

RTClib RTC;
DS3231 Time;
DateTime now;

Adafruit_NeoPixel strip(LED, PINLED, NEO_GRB + NEO_KHZ800);
DFRobotDFPlayerMini myDFPlayer;
WiFiUDP ntpUDP;
NTPClient Clock(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);
ESP8266WebServer server(80);

// --- VARIABEL GLOBAL ---
byte h1, h2, m1, m2;
uint16_t hue;
uint16_t pixelColor;
bool modeSetting = false;
bool isPlaying = false;
int8_t lastHourlyPlay = -1;

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
  byte kecerahan = 50;
  byte volumeDfplayer = 20;
};
PanelSettings settings;

// --- FONT ANGKA (DIMASUKKAN KE PROGMEM UNTUK HEMAT RAM) ---
const uint8_t numberss[] = {
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
  0b0111101,  // [14] n
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
  0b1011011   // [26] S
};

// ==========================================
// FUNGSI EEPROM
// ==========================================
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
  if (hour < 24) return EEPROM.read(ADDR_HALF_HOUR_BASE + hour);
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
}

// ==========================================
// FUNGSI WEB SERVER & JARINGAN
// ==========================================
void handleSetTime() {
  if (server.hasArg(F("PLAY"))) {
    uint8_t track = server.arg(F("PLAY")).toInt();
    if(isPlaying) stopDFPlayer();
    delay(100);
    myDFPlayer.playFolder(2, track);
    delay(150); // Tambahan delay keamanan
    isPlaying = true;
    server.send(200, F("text/plain"), F("OK"));
    return;
  }

  if (server.hasArg(F("STOP"))) {
    stopDFPlayer();
    server.send(200, F("text/plain"), F("OK"));
    return;
  }

  if (server.hasArg(F("MODE"))) {
    byte mode = server.arg(F("MODE")).toInt();
    settings.modeOnline = mode;
    saveByteToEEPROM(ADDR_MODE_ONLINE, mode);
    server.send(200, F("text/plain"), F("OK"));
    delay(1000);
    ESP.restart();
  }

  if (server.hasArg(F("BRIGHTNESS"))) {
    uint8_t brightnessInput = server.arg(F("BRIGHTNESS")).toInt();
    byte mapped = map(brightnessInput, 0, 100, 1, 255);
    settings.kecerahan = mapped;
    strip.setBrightness(mapped);
    strip.show();
    saveByteToEEPROM(ADDR_KECERAHAN, mapped);
    server.send(200, F("text/plain"), F("OK"));
  }

  if (server.hasArg(F("VOLUME"))) {
    uint8_t volInput = server.arg(F("VOLUME")).toInt();
    byte mapped = map(volInput, 0, 100, 0, 30);
    settings.volumeDfplayer = mapped;
    stopDFPlayer();
    myDFPlayer.volume(mapped);
    delay(100);
    saveByteToEEPROM(ADDR_VOLUME, mapped);
    server.send(200, F("text/plain"), F("OK"));
  }

  if (server.hasArg(F("COLOR"))) {
    String colorStr = server.arg(F("COLOR"));
    uint8_t idx1 = colorStr.indexOf(',');
    uint8_t idx2 = colorStr.indexOf(',', idx1 + 1);
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
      saveByteToEEPROM(ADDR_MODE_WARNA, 0); 
      server.send(200, F("text/plain"), F("OK"));
    }
  }

  if (server.hasArg(F("AUTO_COLOR"))) {
    settings.modeWarnaOtomatis = true;
    saveByteToEEPROM(ADDR_MODE_WARNA, 1);
    server.send(200, F("text/plain"), F("OK"));
  }

  if (server.hasArg(F("MODE_SWITCH"))) {
    byte mode = server.arg(F("MODE_SWITCH")).toInt();
    settings.modeSwitchTempp = mode;
    saveByteToEEPROM(ADDR_MODE_SWITCH, mode);
    server.send(200, F("text/plain"), F("OK"));
  }

  // ALARM 1
  if (server.hasArg(F("ALARM1"))) {
    String data = server.arg(F("ALARM1"));
    uint8_t colon1 = data.indexOf(':');
    uint8_t colon2 = data.indexOf(':', colon1 + 1);
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
      server.send(200, F("text/plain"), F("OK"));
    }
  }

  // ALARM 2
  if (server.hasArg(F("ALARM2"))) {
    String data = server.arg(F("ALARM2"));
    uint8_t colon1 = data.indexOf(':');
    uint8_t colon2 = data.indexOf(':', colon1 + 1);
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
      server.send(200, F("text/plain"), F("OK"));
    }
  }

  if (server.hasArg(F("SET_TIME"))) {
    String timeStr = server.arg(F("SET_TIME"));
    uint8_t tahun, bulan, tanggal, dow, jam, menit, detik;
    uint8_t idx1 = timeStr.indexOf('-');
    uint8_t idx2 = timeStr.indexOf('-', idx1 + 1);
    uint8_t idx3 = timeStr.indexOf('-', idx2 + 1);
    uint8_t idx4 = timeStr.indexOf('-', idx3 + 1);
    uint8_t idx5 = timeStr.indexOf(':', idx4 + 1);
    uint8_t idx6 = timeStr.indexOf(':', idx5 + 1);

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
      server.send(200, F("text/plain"), F("OK"));
    }
  }

  if (server.hasArg(F("HALFCHIME"))) {
    String data = server.arg(F("HALFCHIME"));
    uint8_t colon = data.indexOf(':');
    if (colon != -1) {
      byte h = data.substring(0, colon).toInt();
      byte f = data.substring(colon + 1).toInt();
      if (h < 24) {
        saveHalfHourChime(h, f);
        server.send(200, F("text/plain"), F("OK"));
      }
    }
  }
}

void handleGetData() {
  now = RTC.now();
  uint8_t jam = now.hour();
  uint8_t menit = now.minute();
  uint8_t detik = now.second();
  float suhu = getTempp(); 
  String response = "JAM=" + String(jam) + ":" + String(menit) + ":" + String(detik) + " SUHU=" + String(suhu, 1);
  server.send(200, F("text/plain"), response);
}

void AP_init() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  server.on(F("/set"), handleSetTime);       
  server.on(F("/getdata"), handleGetData);   
  server.begin();
}

void ONLINE() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(otaSsid, otaPass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  // 1. Inisialisasi Serial Debug (PC) pada pin D4 (Serial1)
  Serial1.begin(115200);
  Serial1.println(F("\n[SISTEM] Booting..."));

  // 2. Inisialisasi Hardware Serial untuk DFPlayer (Swap ke D7/D8)
  Serial.begin(9600);
  Serial.swap();

  EEPROM.begin(64);

  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, HIGH);

  // Inisiasi DFPlayer dengan ACK dimatikan (false) agar tidak hang!
  if (!myDFPlayer.begin(Serial, false, true)) {
    Serial1.println(F("[ERROR] DFPlayer Gagal"));
    while (true);
  }
  Serial1.println(F("[INFO] DFPlayer Ready."));
  myDFPlayer.setTimeOut(500);

  myDFPlayer.volume(30);  
  delay(100);
  myDFPlayer.playFolder(3, 1); 
  delay(2000); 

  loadSettings();
  strip.begin();
  Wire.begin();
  strip.setBrightness(settings.kecerahan);
  delay(500);
  
  myDFPlayer.playFolder(3, 2); 
  delay(3000);
  
  // Asumsi fungsi syncTimeNTP() ada di file/tab terpisah
   syncTimeNTP();
 
  delay(3000);
  myDFPlayer.playFolder(3, 3); 
  delay(3000);
  
  Serial1.println(F("[SISTEM] READY"));
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  checkClientConnected();  
  if (modeSetting) {
    server.handleClient();
    buzzerUpload(250);
    return;
  }

  timerHue();

  static unsigned long lastToggle = 0;
  static bool toggleState = false;
  unsigned long nowMillis = millis();

  if (settings.modeSwitchTempp) {
    if (nowMillis - lastToggle > 15000) {
      toggleState = !toggleState;
      lastToggle = nowMillis;
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

  // Penting: strip.show() HANYA dipanggil sekali di akhir untuk mencegah hang!
  strip.show(); 

  checkAlarm();
  checkHourlyChime();
}

// ==========================================
// FUNGSI PENDUKUNG
// ==========================================
void checkClientConnected() {
  static uint32_t lastCheck = 0;
  static uint8_t lastClientCount = 0;
  
  if (millis() - lastCheck > 2000) { 
    lastCheck = millis();
    uint8_t clientCount = WiFi.softAPgetStationNum();
 
    if (clientCount != lastClientCount) {
      modeSetting = (clientCount == 1);
      strip.clear();
      strip.show();
      digitalWrite(BUZZ, HIGH);
      lastClientCount = clientCount;
    }
  }
}

void showClock(uint32_t color) {
  getClockRTC();    
  DisplayNumber(h1, 3, color);
  DisplayNumber(h2, 2, color);
  DisplayNumber(m1, 1, color);
  DisplayNumber(m2, 0, color);
}

void showTemp(){
  DisplayNumber(getTempp() / 10, 3, strip.Color(0,255,0));
  DisplayNumber(getTempp() % 10, 2, strip.Color(0,255,0));
  DisplayNumber(11, 1, strip.Color(0,255,0));
  DisplayNumber(12, 0, strip.Color(255,0,0));
}

void checkHourlyChime() {
  now = RTC.now();
  checkScheduledSync(now.hour(),now.minute()); // Uncomment jika NTP sync dipakai
  
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastHourlyPlay) {
    uint8_t jam = now.hour() % 12;
    if (jam == 0) jam = 12; 
    myDFPlayer.volume(settings.volumeDfplayer);
    delay(100);
    myDFPlayer.playFolder(1, jam);  
    delay(150);
    isPlaying = true;
    lastHourlyPlay = now.hour();
  }
}

void checkAlarm() {
  now = RTC.now();
  if (now.hour() == settings.alarm1Hour && now.minute() == settings.alarm1Minute && now.second() == 0) {
    myDFPlayer.volume(settings.volumeDfplayer);
    delay(100);
    myDFPlayer.playFolder(2, settings.alarm1Sound);
    delay(150);
    isPlaying = true;
  }
  if (now.hour() == settings.alarm2Hour && now.minute() == settings.alarm2Minute && now.second() == 0) {
    myDFPlayer.volume(settings.volumeDfplayer);
    delay(100);
    myDFPlayer.playFolder(2, settings.alarm2Sound);
    delay(150);
    isPlaying = true;
  }
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
  if (segment >= 2) startindex += LEDS_PER_DOT * 2;  

  // Baca langsung dari memori biasa (SRAM), hapus pgm_read_byte
  uint8_t segBits = numberss[number];

  for (byte i = 0; i < 7; i++) {           
    for (byte j = 0; j < LEDS_PER_SEG; j++) {         
      strip.setPixelColor(i * LEDS_PER_SEG + j + startindex, (segBits & (1 << i)) ? color : strip.Color(0, 0, 0));
    }
  }
}

/*/ Dioptimasi: Membaca font dari Flash RAM (PROGMEM) dan TANPA strip.show()
void DisplayNumber(byte number, byte segment, uint32_t color) {
  byte startindex = segment * LEDS_PER_DIGIT;
  if (segment >= 2) startindex += LEDS_PER_DOT * 2;  

  uint8_t segBits = pgm_read_byte(&numberss[number]);

  for (byte i = 0; i < 7; i++) {           
    for (byte j = 0; j < LEDS_PER_SEG; j++) {         
      strip.setPixelColor(i * LEDS_PER_SEG + j + startindex, (segBits & (1 << i)) ? color : strip.Color(0, 0, 0));
    }
  }
}*/

void getClockRTC() {
  now = RTC.now();
  h1 = now.hour() / 10;
  h2 = now.hour() % 10;
  m1 = now.minute() / 10;
  m2 = now.minute() % 10;
}

void getClockNTP() {
  Clock.update();
  h1 = Clock.getHours();
  m2 = Clock.getMinutes();
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
    static bool state;
    static uint32_t save = 0;
    uint32_t tmr = millis();
    
    if(tmr - save > Delay ){
      save = tmr;
      state = !state;
      digitalWrite(BUZZ, state);
    }
}
