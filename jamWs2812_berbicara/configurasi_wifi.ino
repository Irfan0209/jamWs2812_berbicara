
// Variabel yang bisa disesuaikan user (bisa dari EEPROM/WebUI)
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
}
