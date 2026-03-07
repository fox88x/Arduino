// TimeSync.ino — Sala1

enum SyncMode : uint8_t { SYNC_FAST, SYNC_SLOW };

SyncMode syncMode      = SYNC_FAST;
uint8_t  syncFailCount = 0;
unsigned long epoch     = 0;

void onCloudReconnect() {
  syncMode      = SYNC_FAST;
  syncFailCount = 0;
  firstSyncDone = false;
  syncState     = false;
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
}

void cloudTimeSync() {
  if (!timeSyncTimer.elapsed()) return;

  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  epoch = ArduinoCloud.getInternalTime();

  if (epoch == 0 || epoch < 1609459200UL) {
    Led1.start(sequence6, numSteps6);
    Serial.println(F(" [SYNC] Sincronizzazione fallita."));

    syncFailCount++;
    if (syncFailCount >= SYNC_FAIL_MAX) {
      Serial.println(F(" [SYNC] Troppi tentativi falliti."));
      Led1.startContinuous(0x8F00FF);
      syncState = false;
    }
    timeSyncTimer.start(SYNC_FAST_INTERVAL);
    syncMode = SYNC_FAST;
    return;
  }

  // Sync riuscita
  struct timeval tv = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  if (!firstSyncDone) {
    Serial.println(F(" [SYNC] Sincronizzazione completata."));
    Led1.start(sequence5, numSteps5);
  } else {
    Serial.println(F(" [SYNC] Orologio aggiornato."));
  }

  Serial.print(F(" RTC: "));
  Serial.println(ctime((time_t*)&epoch));

  syncFailCount = 0;
  syncState     = true;
  firstSyncDone = true;
  syncMode      = SYNC_SLOW;
  timeSyncTimer.start(SYNC_SLOW_INTERVAL);
}
