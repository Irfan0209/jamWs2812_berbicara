#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DS3231.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#define PinLed D5
#define LEDS_PER_SEG 5
#define LEDS_PER_DOT 4
#define LEDS_PER_DIGIT  LEDS_PER_SEG *7
#define LED   148

#ifndef STASSID
#define STASSID "Wifi"
#define STAPSK  "00000000"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "OTA-LEDS";

RTClib RTC;
DS3231 Time;
DateTime now;
WiFiManager wifi;
Adafruit_NeoPixel strip(LED, PinLed, NEO_GRB + NEO_KHZ800);

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
};


void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(50);

}

void loop() {
  // put your main code here, to run repeatedly:

}

void DisplayNumber(byte number, byte segment, uint32_t color) {
  // Hitung index awal untuk digit ke-0 s.d. ke-3
  byte startindex = segment * LEDS_PER_DIGIT;
  if (segment >= 2) startindex += LEDS_PER_DOT * 2;  // Lewati dot setelah digit 1

  uint8_t segBits = numberss[number];  // Bit pola untuk angka

  // Untuk setiap dari 7 segmen (a-g)
  for (byte i = 0; i < 7; i++) {
    bool isOn = segBits & (1 << i);  // Cek apakah segmen i aktif
    uint32_t col = isOn ? color : strip.Color(0, 0, 0);

    // Setiap LED dalam segmen
    for (byte j = 0; j < LEDS_PER_SEG; j++) {
      byte ledIndex = startindex + i * LEDS_PER_SEG + j;
      strip.setPixelColor(ledIndex, col);
    }
  }

  strip.show();  // Tampilkan setelah semua pixel diatur
}

void getClockRTC() 
{
  now = RTC.now();
  h1 = now.hour() / 10;
  h2 = now.hour() % 10;
  m1 = now.minute() / 10;
  m2 = now.minute() % 10;
}

void showClock(uint32_t color) {
  DisplayNumber(h1, 3, color);
  DisplayNumber(h2, 2, color);
  DisplayNumber(m1, 1, color);
  DisplayNumber(m2, 0, color);
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
  const uint8_t delayHue = 2;
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
