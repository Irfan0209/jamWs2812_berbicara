// Variabel yang bisa disesuaikan user (bisa dari EEPROM/WebUI)
uint8_t syncHour = 5;   // Contoh: Jam 05:00 pagi
uint8_t syncMinute = 55; // Menit ke-55
bool hasSynced = false; // Flag agar tidak looping sync di menit yang sama

// Fungsi utama untuk sinkronisasi
void syncTimeNTP() {
  Serial1.println(F("[NTP] Memulai sinkronisasi..."));
  
  myDFPlayer.volume(30);  
  myDFPlayer.playFolder(3, 5); // 003_menghubungkan wifi.wav
  
  // Jeda audio 2,5 detik tapi layar tetap hidup
  for (uint8_t i = 0; i < 5; i++) {
    showClock(getCurrentColor());
    strip.show();
    delay(500);
  }

  // 1. Ubah ke mode Station
  WiFi.disconnect(true); // Putus koneksi lama agar state bersih
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(otaSsid, otaPass);
  
  Serial1.println(F("[NTP] Menyambung WiFi..."));

  // 2. Tunggu koneksi dengan timeout (maksimal ~10 detik) -> 20 * 500ms
  uint8_t wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    // Tetap update tampilan jam agar LED tidak freeze!
    showClock(getCurrentColor());
    strip.show();
    
    delay(500);
    Serial1.print(F("."));
    wifiTimeout++;
  }
  Serial1.println();

  // 3. Jika berhasil konek, lakukan sinkronisasi NTP
  if (WiFi.status() == WL_CONNECTED) {
      Serial1.println(F("[NTP] Berhasil konek! Meminta waktu..."));
      
      Clock.begin(); // Pastikan library NTP Client menyala
      if (Clock.forceUpdate()) { // forceUpdate = paksa minta data ke server internet
        Time.setDoW(Clock.getDay());
        Time.setHour(Clock.getHours());
        Time.setMinute(Clock.getMinutes());
        Time.setSecond(Clock.getSeconds());
        Serial1.println(F("[NTP] Berhasil sinkron waktu RTC!"));
      } else {
        Serial1.println(F("[NTP] Gagal tarik data dari Server NTP."));
      }

      myDFPlayer.playFolder(3, 4); // 004_wifi connect.wav
      
      // Menggantikan delay(5000); agar jam tetap berjalan selama suara dimainkan
      for (uint8_t i = 0; i < 10; i++) {
        showClock(getCurrentColor());
        strip.show();
        delay(500);
      }

  } else {
      Serial1.println(F("[NTP] Gagal konek WiFi (Timeout)."));
  }
  
  // 4. Memutus WiFi dan kembali ke AP Mode
  WiFi.disconnect(true); 
  delay(100); 
  myDFPlayer.playFolder(3, 7); // 006_mulai mode AP.wav
  
  // Bebaskan jam dari freeze sesaat sambil menunggu audio
  for (int i = 0; i < 4; i++) {
    showClock(getCurrentColor());
    strip.show();
    delay(500);
  }
  
  AP_init();
  Serial1.println(F("[NTP] Sistem kembali ke Mode AP."));
}

// Checker di dalam loop()
void checkScheduledSync(uint8_t currentHour, uint8_t currentMinute) {
  if (currentHour == syncHour && currentMinute == syncMinute) {
    if (!hasSynced) {
      syncTimeNTP();
      hasSynced = true;
    }
  } else {
    // Reset flag saat waktu sudah beranjak dari menit sinkronisasi
    hasSynced = false; 
  }
}

/*/ Variabel yang bisa disesuaikan user (bisa dari EEPROM/WebUI)
uint8_t syncHour = 5;   // Contoh: Jam 03:00 pagi
uint8_t syncMinute = 55; // Menit ke-0
bool hasSynced = false; // Flag agar tidak looping sync di menit yang sama

// Fungsi utama untuk sinkronisasi
void syncTimeNTP() {
  // 1. Ubah ke mode Station dan mulai koneksi
  myDFPlayer.volume(30);  // volume awal sementara, nanti diganti dari EEPROM
  myDFPlayer.playFolder(3, 5); // 003_menghubungkan wifi.wav
  delay(2500);
  WiFi.mode(WIFI_STA);
  WiFi.begin(otaSsid, otaPass);
  Serial.println(F("menyambung wifi"));
 
  // 2. Tunggu koneksi dengan timeout (maksimal ~10 detik)
 uint8_t wifiTimeout = 0;
 while (WiFi.status() != WL_CONNECTED && wifiTimeout <= 150) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  

  // 3. Jika berhasil konek, lakukan sinkronisasi NTP
  if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("berhasil konek"));
      Clock.update();
      Time.setDoW(Clock.getDay());
      Time.setHour(Clock.getHours());
      Time.setMinute(Clock.getMinutes());
      Time.setSecond(Clock.getSeconds());
      myDFPlayer.playFolder(3, 4); // 004_wifi connect.wav
      delay(5000);

      WiFi.disconnect(true); 
      delay(100); // Beri jeda sebentar agar hardware radio stabil
      myDFPlayer.playFolder(3, 7); // 006_mulai mode AP.wav
      AP_init();
      Serial.println("berhasil sinkron");
   }else if(WiFi.status() != WL_CONNECTED) {
      Serial.println(F("gagal konek"));
      WiFi.disconnect(true); 
      myDFPlayer.playFolder(3, 7); // 006_mulai mode AP.wav
      //delay(1000); // Beri jeda sebentar agar hardware radio stabil
      AP_init();
      
   }
  
}

// Masukkan fungsi ini di dalam loop() Anda
void checkScheduledSync(uint8_t currentHour, uint8_t currentMinute) {
  if (currentHour == syncHour && currentMinute == syncMinute) {
    if (!hasSynced) {
      syncTimeNTP();
      hasSynced = true;
    }
  } else {
    // Reset flag saat waktu sudah beranjak dari menit sinkronisasi
    hasSynced = false; 
  }
}*/
