
//////hijiriyah voidku/////////////////////////////////////////////////
void islam() {
  now = RTC.now();
  static uint32_t sv=0;
 
  if(millis() - sv > 5000){

    JWS.Update(config.zonawaktu, config.latitude, config.longitude, config.altitude, now.year(), now.month(), now.day()); // Jalankan fungsi ini untuk update jadwal sholat
    JWS.setIkhtiSu = dataIhty[0];
    JWS.setIkhtiDzu = dataIhty[1];
    JWS.setIkhtiAs = dataIhty[2];
    JWS.setIkhtiMa = dataIhty[3];
    JWS.setIkhtiIs = dataIhty[4];

    sv = millis();
  }
}

//================= cek waktu sholat ===================//
void check() {
    now = RTC.now();
    uint8_t jam = now.hour();
    uint8_t menit = now.minute();
    uint8_t detik = now.second();
    uint8_t daynow = Time.getDoW();
    
    uint8_t hours, minutes;
    
    static uint8_t counter = 0;
    static uint32_t lsTmr;
    static bool adzanFlag[5] = {false, false, false, false, false};
    float sholatT[]={JWS.floatSubuh,JWS.floatDzuhur,JWS.floatAshar,JWS.floatMaghrib,JWS.floatIsya};
    uint32_t tmr = millis();
    
    
    if (tmr - lsTmr > 100 ) {
        lsTmr = tmr;
        
        float stime = sholatT[counter];
        uint8_t hours = floor(stime);
        uint8_t minutes = floor((stime - (float)hours) * 60);
 
        if (!adzanFlag[counter]) {
           if (jam == hours && menit == minutes && detik == 0)   {      
              if(daynow == 5 && counter == 1){
                return ;
              }else{
                config.sholatNow = counter;
                config.adzan = 1;
                adzanFlag[counter] = true;
                Serial.println("ADZAN RUN");
              }    
           }
        }
        if (jam != hours || menit != minutes) {
            adzanFlag[counter] = false;
        }    
//        Serial.println("jadwal sholat: " + String(hours) +":"+ String(minutes));
//        Serial.println("hari: " + String(daynow));
        counter = (counter + 1) % 5;
    }
}
