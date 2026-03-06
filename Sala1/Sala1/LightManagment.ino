//lightManagment.ino
unsigned long manualStartTime = 0, relay2StartTime=0;
int active2Relay = 0;

// Pausa non bloccante per relè luce
int pendingLightPin = 0;
unsigned long pendingLightTime = 0;
const unsigned long LIGHT_RELAY_PAUSE = 500;

void checkLight() {
  lightSensor.update();

  // Completamento attivazione relè luce dopo pausa non bloccante
  if (pendingLightPin != 0 && (millis() - pendingLightTime >= LIGHT_RELAY_PAUSE)) {
    digitalWrite(pendingLightPin, HIGH);
    relay2StartTime = millis();
    active2Relay = (pendingLightPin == LIGHT_ON_PIN) ? 1 : 2;
    if(pendingLightPin == LIGHT_ON_PIN) {
      Serial.println("luci accese");
    } else {
      Serial.println("luci spente");
    }
    pendingLightPin = 0;
  }

  //auto mode
  if (!manualLight) {
    if(cal.isWorkingDay() && cal.isWorkingTime() && lightSensor.night)  {
      if (!lightState) {
        setLightAction(LIGHT_AUTO_ON);
        activateLightRelay(LIGHT_ON_PIN);
        lightState = true;
      }
    }
    else
    {
      if (lightState) {
        setLightAction(LIGHT_AUTO_OFF);
        activateLightRelay(LIGHT_OFF_PIN);
        lightState = false;
      }
    }
  }
  // manual mode
  else {
    if(!lightState) {
      //messager = "luce accesa in modalita MANUAL";
      setLightAction(LIGHT_MANUAL_ON);
      activateLightRelay(LIGHT_ON_PIN);
      lightState = true;
      manualStartTime = millis();
    }

    if (millis() - manualStartTime >= MANUAL_TIMER) {
      manualLight = 0;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void activateLightRelay(int lightRelayPin) {
  // Determina il relè opposto:
  int oppositeRelay = (lightRelayPin == LIGHT_ON_PIN) ? LIGHT_OFF_PIN : LIGHT_ON_PIN;

  // Se il relè opposto è attivo, disattivalo e avvia pausa non bloccante
  if ((lightRelayPin == LIGHT_ON_PIN && active2Relay == 2) ||
      (lightRelayPin == LIGHT_OFF_PIN && active2Relay == 1)) {
    digitalWrite(oppositeRelay, LOW);
    active2Relay = 0;
    // Avvia pausa non bloccante
    pendingLightPin = lightRelayPin;
    pendingLightTime = millis();
    return;
  }

  // Nessun conflitto, attiva subito
  digitalWrite(lightRelayPin, HIGH);
  relay2StartTime = millis();
  active2Relay = (lightRelayPin == LIGHT_ON_PIN) ? 1 : 2;

  if(lightRelayPin == LIGHT_ON_PIN) {
    Serial.println("luci accese");
  }
  else {
    Serial.println("luci spente");
  }
}
