// timeSync.ino — Sala3

enum SyncMode : uint8_t { SYNC_FAST, SYNC_SLOW };

SyncMode syncMode      = SYNC_FAST;
uint8_t  syncFailCount = 0;

void onCloudReconnect() {
  syncMode      = SYNC_FAST;
  syncFailCount = 0;
  firstSyncDone = false;
  syncState     = false;
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
}

void cloudTimeSync() {
  if (!timeSyncTimer.elapsed()) return;

  bool ok = TimeService.sync();

  if (!ok) {
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
  if (!firstSyncDone) {
    Serial.println(F(" [SYNC] Sincronizzazione completata."));
    cal.updateTime();
    Led1.start(sequence5, numSteps5);
  } else {
    Serial.println(F(" [SYNC] Orologio aggiornato."));
    cal.updateTime();
  }

  Serial.print(F(" RTC: "));
  cal.printCurrentTime();

  syncFailCount = 0;
  syncState     = true;
  firstSyncDone = true;
  syncMode      = SYNC_SLOW;
  timeSyncTimer.start(SYNC_SLOW_INTERVAL);
}
