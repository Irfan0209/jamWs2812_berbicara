#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

//#include <WiFiManager.h>

#include <DS3231.h>
#include <SPI.h>
#include <Wire.h>

#include <EEPROM.h>

#include <ESP8266WebServer.h>

#include <ArduinoOTA.h>
#include "DFRobotDFPlayerMini.h"

#define PINLED D5
#define LEDS_PER_SEG 5
#define LEDS_PER_DOT 4
#define LEDS_PER_DIGIT  LEDS_PER_SEG *7
#define LED   148

char ssid[20]     = "JAM_WS2812";
char password[20] = "00000000";

const char* otaSsid = "KELUARGA02";
const char* otaPass = "suhartono";
const char* otaHost = "SERVER";

const long utcOffsetInSeconds = 25200;

RTClib RTC;
DS3231 Time;
DateTime now;
//WiFiManager wifi;
Adafruit_NeoPixel strip(LED, PINLED, NEO_GRB + NEO_KHZ800);
DFRobotDFPlayerMini myDFPlayer;
WiFiUDP ntpUDP;
NTPClient Clock(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);
ESP8266WebServer server(80);

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
bool modeOnline = false;
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

IPAddress local_IP(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

struct PanelSettings {
  bool modeWarnaOtomatis;
  bool modeOnline;
  bool modeSwitchTempp;
  uint8_t manualR, manualG, manualB;
  uint8_t alarm1Hour, alarm1Minute, alarm1Sound;
  uint8_t alarm2Hour, alarm2Minute, alarm2Sound;
  uint8_t kecerahan;
  uint8_t volumeDfplayer;
  
};

PanelSettings settings;

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



void handleRoot() {
  server.send(200, "text/plain", "Jam LED Siap!");
}

void handleCommand() {
  String cmd = server.arg("q");

  if (cmd == "PLAY:") {
    uint8_t track = cmd.substring(5, 7).toInt();
    myDFPlayer.play(track);
    isPlaying = true;

  } else if (cmd == "STOP") {
    stopDFPlayer();

  } else if (cmd == "BRIGHTNESS:") {
    brightness = cmd.substring(11, 14).toInt();
    uint8_t data = map(brightness,0,100,1,255);
    strip.setBrightness(data);
    settings.kecerahan = data;
    saveSettings();

  } else if (cmd == "VOLUME:") {
    volumeDfPlayer = cmd.substring(7, 9).toInt();
    uint8_t data = map(volumeDfPlayer,0,100,0,29);
    myDFPlayer.volume(data);
    settings.volumeDfplayer = data;
    saveSettings();

  } else if (cmd.startsWith("SET_TIME:")) {
    int h = cmd.substring(9, 11).toInt();
    int m = cmd.substring(12, 14).toInt();
    int s = cmd.substring(15, 17).toInt();
    Time.setHour(h);
    Time.setMinute(m);
    Time.setSecond(s);

  } else if (cmd.startsWith("COLOR:")) {
    modeWarnaOtomatis = false;
    manualR = cmd.substring(6, 9).toInt();
    manualG = cmd.substring(10, 13).toInt();
    manualB = cmd.substring(14, 17).toInt();

    settings.modeWarnaOtomatis = modeWarnaOtomatis;
    settings.manualR = manualR;
    settings.manualG = manualG;
    settings.manualB = manualB;
    saveSettings();

  } else if (cmd == "AUTO_COLOR") {
    modeWarnaOtomatis = true;

    settings.modeWarnaOtomatis = modeWarnaOtomatis;
    saveSettings();

  } else if (cmd == "MODE_ONLINE") {
    modeOnline = true;

    settings.modeOnline = modeOnline;
    saveSettings();
    delay(1000);
    ESP.restart();

  } else if (cmd == "MODE_OFFLINE") {
    modeOnline = false;

    settings.modeOnline = modeOnline;
    saveSettings();
    delay(1000);
    ESP.restart();

  } else if (cmd == "MODE_SWITCH:") {
    modeSwitchTemp = cmd.substring(12).toInt();

    settings.modeSwitchTempp = modeSwitchTemp;
    saveSettings();

  }else if (cmd.startsWith("ALARM1:")) {
    alarm1Hour = cmd.substring(7, 9).toInt();
    alarm1Minute = cmd.substring(10, 12).toInt();
    alarm1Sound = cmd.substring(13).toInt();

    settings.alarm1Hour = alarm1Hour;
    settings.alarm1Minute = alarm1Minute;
    settings.alarm1Sound = alarm1Sound;
    saveSettings();

  } else if (cmd.startsWith("ALARM2:")) {
    alarm2Hour = cmd.substring(7, 9).toInt();
    alarm2Minute = cmd.substring(10, 12).toInt();
    alarm2Sound = cmd.substring(13).toInt();

    settings.alarm2Hour = alarm2Hour;
    settings.alarm2Minute = alarm2Minute;
    settings.alarm2Sound = alarm2Sound;
    saveSettings();
  }

  server.send(200, "text/plain", "OK");
}


void AP_init() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  server.on("/set", handleCommand);
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
    //Serial.println("restart=1");
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
  EEPROM.get(0, settings);

  // Gunakan nilai hasil load
  modeWarnaOtomatis = settings.modeWarnaOtomatis;
  modeOnline = settings.modeOnline;
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
  modeSwitchTemp = settings. modeSwitchTempp;
}


void setup() {
  Serial.begin(115200);
  //myDFPlayer.begin(Serial);  // DFPlayer RX → TX ESP8266
  EEPROM.begin(512);  // Inisialisasi EEPROM
  strip.begin();
  Wire.begin();
  //loadSettings();     // Load data ke variabel
  
  strip.setBrightness(brightness);
  //myDFPlayer.volume(volumeDfPlayer);     // Volume dari 0 - 30
  modeOnline?ONLINE():AP_init();
  Serial.println();
  Serial.println("modeOnline: " + String(modeOnline));
  Serial.println("READY");
}

void loop() {
  if (modeOnline) {
    ArduinoOTA.handle();
    getClockNTP();
  } else {
    server.handleClient();
    getClockRTC(); // dari RTC
  

  timerHue();

  static uint32_t lastToggle = 0;
  static bool toggleState = false;

  if (modeSwitchTemp) {
    uint32_t now = millis();
    if (now - lastToggle > 5000) {
      toggleState = !toggleState;
      lastToggle = now;
    }

    if (toggleState) {
      showClock(getCurrentColor());
      showDots(0xFF0000); // Merah
    } else {
      showTemp();
      showDots(0x000000); // Mati
    }

  } else {
    showClock(getCurrentColor());
    showDots(0xFF0000); // Merah
  }

  // Aktifkan kembali jika perlu:
  // checkAlarm();
  // checkHourlyChime();
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

    myDFPlayer.playFolder(1,jam);   // Misalnya track 1-12 adalah suara jam
    isPlaying = true;
    lastHourlyPlay = now.hour();  // Simpan jam terakhir dimainkan
  }
}

void checkAlarm() {
  if (!isPlaying) {
    if (now.hour() == alarm1Hour && now.minute() == alarm1Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,alarm1Sound);
      isPlaying = true;
    }
    if (now.hour() == alarm2Hour && now.minute() == alarm2Minute && now.second() == 0) {
      myDFPlayer.playFolder(2,alarm2Sound);
      isPlaying = true;
    }
  }
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
  const uint8_t delayHue = 30;
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
