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

  bool state = TimeService.sync();
    
  if (!state) {
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

  if (!firstSyncDone) {
    Serial.println(F(" [SYNC]   ✅  Sincronizzazione oraria completata."));
    cal.updateTime();
    Led1.start(sequence5, numSteps5);
  } else {
    Serial.println(F(" [SYNC]   🔄  Sincronizzazione oraria aggiornata."));
    cal.updateTime();
    Led1.start(sequence9, numSteps9);
  }

  Serial.print(" RTC sincronizzato a: ");
  cal.printCurrentTime();

  syncFailCount = 0;
  syncState = true;
  firstSyncDone = true;
  syncMode = SYNC_SLOW;
  timeSyncTimer.start(SYNC_SLOW_INTERVAL);
}