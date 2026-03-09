// timeSync.ino — Sala4 — NTP Time Sync
//
// Usa il client SNTP integrato nell'ESP32.
// configTzTime() chiamato nel setup() configura timezone e NTP server.
// getLocalTime() controlla se il tempo è sincronizzato.
//
// A differenza della versione cloud:
//   - Non usa TimeService (Arduino Cloud)
//   - Non usa ArduinoCloud.getInternalTime()
//   - NTP funziona in background quando WiFi è disponibile
//   - Se WiFi non disponibile, ESP-NOW continua a funzionare (solo no NTP)

// Questo file è mantenuto per compatibilità strutturale con le altre stanze.
// La logica NTP è in ntpTimeSync() nel main sketch.
// Qui ci sono funzioni helper.

void printNetworkStatus() {
  char buf[80];

  Serial.println(F("--- Network Status ---"));

  snprintf(buf, sizeof(buf), "WiFi: %s", isWifiConnected() ? "Connesso" : "Disconnesso");
  Serial.println(buf);

  if (isWifiConnected()) {
    Serial.print(F("  IP: "));
    Serial.println(WiFi.localIP());
    snprintf(buf, sizeof(buf), "  Canale: %d", WiFi.channel());
    Serial.println(buf);
  }

  snprintf(buf, sizeof(buf), "NTP:  %s", syncState ? "Sincronizzato" : "Non sincronizzato");
  Serial.println(buf);

  if (syncState) {
    cal.printCurrentTime();
  }

  snprintf(buf, sizeof(buf), "Peer: %d/%d online", getOnlinePeerCount(), NUM_SALAS - 1);
  Serial.println(buf);

  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;
    const char* status;
    switch (peers[i].status) {
      case PEER_ONLINE:  status = "ONLINE";  break;
      case PEER_OFFLINE: status = "OFFLINE"; break;
      default:           status = "UNKNOWN"; break;
    }
    snprintf(buf, sizeof(buf), "  Sala%d: %s", peers[i].salaId, status);
    Serial.println(buf);

    if (peers[i].stateValid) {
      snprintf(buf, sizeof(buf), "    Win: %d, Rain: %s, Light: %s",
        (int)peers[i].lastState.winPos,
        peers[i].lastState.isRaining ? "SI" : "NO",
        peers[i].lastState.lightState ? "ON" : "OFF");
      Serial.println(buf);
    }
  }

  Serial.println(F("----------------------"));
}
