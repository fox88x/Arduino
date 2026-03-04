//TimeSync.ino
const uint8_t SYNC_FAIL_MAX = 6;

enum SyncMode { SYNC_FAST, SYNC_SLOW };
SyncMode syncMode = SYNC_FAST;

uint8_t syncFailCount = 0;
unsigned long epoch = 0;

// Variabili di stato esistenti
extern bool firstSyncDone;
extern bool syncState;

void onCloudReconnect() {
  syncMode = SYNC_FAST;
  syncFailCount = 0;
  firstSyncDone = false;
  syncState = false;
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
}

void cloudTimeSync() {
  if (!timeSyncTimer.elapsed()) return;

  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  epoch = ArduinoCloud.getInternalTime();

  if (epoch == 0 || epoch < 1609459200UL) {
    // Sincronizzazione fallita
    Led1.start(sequence6, numSteps6);
    Serial.println(F(" [SYNC]   ❌  Sincronizzazione fallita."));

    syncFailCount++;
    if (syncFailCount >= SYNC_FAIL_MAX) {
      Serial.println(F(" [SYNC]   ❌  Orologio interno non sincronizzato (troppi tentativi falliti)"));
      Led1.startContinuous(0x8F00FF);
      syncState = false;
    }
    // Riprova sempre ogni 10s finché non sincronizza
    timeSyncTimer.start(SYNC_FAST_INTERVAL);
    syncMode = SYNC_FAST;
    return;
  }

  // Sincronizzazione riuscita
  struct timeval tv = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  if (!firstSyncDone) {
    Serial.println(F(" [SYNC]   ✅  Sincronizzazione oraria completata."));
    //cal.updateTime();
    Led1.start(sequence5, numSteps5);
  } else {
    Serial.println(F(" [SYNC]   🔄  Sincronizzazione oraria aggiornata."));
    //cal.updateTime();
    //Led1.start(sequence9, numSteps9);
  }
  Serial.println("");
  Serial.print("RTC sincronizzato a: ");
  Serial.println(ctime((time_t*)&epoch));
  
  syncFailCount = 0;
  syncState = true;
  firstSyncDone = true;
  syncMode = SYNC_SLOW;
  timeSyncTimer.start(SYNC_SLOW_INTERVAL);
}


// ——————————————————————————————————————————————————————————  
/*void dataPrint() {
  //struct tm timeinfo;
  // getLocalTime() legge dal solo RTC (senza rete) :contentReference[oaicite:11]{index=11}
  if (getLocalTime(&timeinfo)) {
    Serial.printf(
      "%02d/%02d/%04d  %02d:%02d:%02d\n",
      timeinfo.tm_mday,
      timeinfo.tm_mon + 1,
      timeinfo.tm_year + 1900,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
  }
  else {
    Serial.println("Errore: impossibile leggere il RTC interno");
  }
  Serial.print("Is working day: ");
  Serial.println(cal.isWorkingDay() ? "Si" : "No");
  Serial.print("Is working time: ");
  Serial.println(cal.isWorkingTime() ? "Si" : "No");
  Serial.println(); 
}*/