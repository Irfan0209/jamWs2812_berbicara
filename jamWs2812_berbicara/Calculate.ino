
//////hijiriyah voidku/////////////////////////////////////////////////
void islam() {
  now = RTC.now();

  checkScheduledSync(now.hour(),now.minute());
  
  static int8_t lastDayCalc = -1;
  if (now.minute() == 0 && now.second() == 0 && now.hour() != lastDayCalc){
    lastDayCalc = now.hour();
    butuhHitungJadwal = true; 
  }
 
  if(butuhHitungJadwal){
    for(uint8_t i = 0; i < 2; i++) {
      JWS.Update(config.zonawaktu, config.latitude, config.longitude, config.altitude, now.year(), now.month(), now.day());
      JWS.setIkhtiSu = dataIhty[0];
      JWS.setIkhtiDzu = dataIhty[1];
      JWS.setIkhtiAs = dataIhty[2];
      JWS.setIkhtiMa = dataIhty[3];
      JWS.setIkhtiIs = dataIhty[4];
      JWS.setIkhtiIm = dataIhty[5];
      //Hijir.Update(now.year(), now.month(), now.day(), config.Correction);
    }
    butuhHitungJadwal = false; 
  }
}


static float lastStime[5] = { -1, -1, -1, -1, -1 };
static uint8_t sholatHour[5];
static uint8_t sholatMinute[5];
static bool adzanFlag[5] = { 0 };

//================= cek waktu sholat (REFRAKTOR) ===================//
void check() {

  static uint8_t counter = 0;
  static uint32_t scanTmr = 0, sendTmr = 0;

  uint32_t nowMs = millis();

  // ================= CACHE UPDATE (float -> int) =================
  
  // Pilih waktu sholat sesuai list
  float stime;
  switch (counter) {
    case 0: stime = JWS.floatSubuh; break;
    case 1: stime = JWS.floatDzuhur; break;
    case 2: stime = JWS.floatAshar; break;
    case 3: stime = JWS.floatMaghrib; break;
    case 4: stime = JWS.floatIsya; break;
  }
  
  if (stime != lastStime[counter]) {

    lastStime[counter] = stime;

    uint8_t h = (uint8_t)stime;
    uint8_t m = (uint8_t)((stime - h) * 60);

    sholatHour[counter]   = h;
    sholatMinute[counter] = m;

    adzanFlag[counter] = false;   // reset jika jadwal berubah
  }

  // ================= CEK WAKTU ADZAN =================
  if (!stateSendSholat && nowMs - scanTmr >= 100) {
    scanTmr = nowMs;

    now = RTC.now();
    uint8_t jam   = now.hour();
    uint8_t menit = now.minute();
    uint8_t detik = now.second();
    uint8_t day ;//  = now.dayOfTheWeek();

    uint8_t h = sholatHour[counter];
    uint8_t m = sholatMinute[counter];

    if (!adzanFlag[counter]) {
      if (jam == h && menit == m && detik == 0) {
        if (!(day == 5 && counter == 1)) {
          sholatNow = counter;
          adzan = 1;
          adzanFlag[counter] = true;
        }
      }
    }
    
  
    // reset flag jika waktu sudah lewat
    if (jam != h || menit != m) {
      adzanFlag[counter] = false;
    }
    
    counter++;
    if (counter >= 5) counter = 0;
  }

  // ================= KIRIM DATA JWS =================
  if (stateSendSholat && nowMs - sendTmr >= 500) {
    sendTmr = nowMs;

    char buf[64];
    char *p = buf;
    p += sprintf(p, "JWS:");

    for (uint8_t i = 0; i < 5; i++) {
      p += sprintf(p, "%02d,%02d", sholatHour[i], sholatMinute[i]);
      if (i < 4) *p++ = '|';
    }

    *p = '\0';
    Serial.println(buf);

    stateSendSholat = false;
  }
}
