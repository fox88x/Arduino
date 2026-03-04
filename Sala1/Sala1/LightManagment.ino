//lightManagment.ino
unsigned long manualStartTime = 0, relay2StartTime=0;
int active2Relay = 0;

void checkLight() {
  lightSensor.update();
  
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
  
  // Se il relè opposto è attivo, disattivalo immediatamente.
  if ((lightRelayPin == LIGHT_ON_PIN && active2Relay == 2) ||
      (lightRelayPin == LIGHT_OFF_PIN && active2Relay == 1)) {
    digitalWrite(oppositeRelay, LOW);
    active2Relay = 0;
    delay(500); // Attende 500ms per una transizione stabile
  }
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