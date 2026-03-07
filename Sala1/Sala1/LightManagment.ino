// LightManagment.ino — Sala1 — State Machine
//
// Stati relay luce: LRELAY_IDLE → LRELAY_PAUSE → LRELAY_PULSING → LRELAY_IDLE

// ===================== STATO RELAY LUCE =====================
LightRelayState lightRelayState = LRELAY_IDLE;
int      pendingLightPin  = 0;
unsigned long lightRelayTimer_ms = 0;
unsigned long manualStartTime = 0;

// ===================== UPDATE LIGHT =====================
void updateLight() {
  lightSensor.update();
  unsigned long now = millis();

  // --- State machine relay luce ---
  switch (lightRelayState) {

    case LRELAY_IDLE:
      break;

    case LRELAY_PAUSE:
      if (now - lightRelayTimer_ms >= LIGHT_RELAY_PAUSE_MS) {
        digitalWrite(pendingLightPin, HIGH);
        lightRelayTimer_ms = now;
        lightRelayState = LRELAY_PULSING;
      }
      break;

    case LRELAY_PULSING:
      if (now - lightRelayTimer_ms >= LIGHT_RELAY_MS) {
        digitalWrite(pendingLightPin, LOW);
        pendingLightPin = 0;
        lightRelayState = LRELAY_IDLE;
      }
      break;
  }

  // --- Logica auto/manual ---
  if (!manualLight) {
    // Auto mode
    if (cal.isWorkingDay() && cal.isWorkingTime() && lightSensor.night) {
      if (!lightState) {
        setLightAction(LIGHT_AUTO_ON);
        activateLightRelay(LIGHT_ON_PIN);
        lightState = true;
      }
    } else {
      if (lightState) {
        setLightAction(LIGHT_AUTO_OFF);
        activateLightRelay(LIGHT_OFF_PIN);
        lightState = false;
      }
    }
  } else {
    // Manual mode
    if (!lightState) {
      setLightAction(LIGHT_MANUAL_ON);
      activateLightRelay(LIGHT_ON_PIN);
      lightState = true;
      manualStartTime = now;
    }

    if (now - manualStartTime >= MANUAL_TIMER) {
      manualLight = false;
    }
  }
}

// ===================== ATTIVAZIONE RELAY LUCE =====================
void activateLightRelay(int lightRelayPin) {
  // Se un relay luce e' gia' in pulsing, ignora
  if (lightRelayState == LRELAY_PULSING) return;

  int opposite = (lightRelayPin == LIGHT_ON_PIN) ? LIGHT_OFF_PIN : LIGHT_ON_PIN;
  digitalWrite(opposite, LOW);

  pendingLightPin    = lightRelayPin;
  lightRelayTimer_ms = millis();
  lightRelayState    = LRELAY_PAUSE;
}
