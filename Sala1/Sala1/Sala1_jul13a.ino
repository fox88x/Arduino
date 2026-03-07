// Sala1_jul13a.ino — Nano ESP32
// Architettura a stati con auto-recovery
//
// SYS_BOOT → (cloud sync) → SYS_RUNNING ⇄ SYS_DISCONNECTED
//                                              ↓ (timeout)
//                                          ESP.restart()

#include "arduino_secrets.h"
#include <Bounce2.h>
#include "thingProperties.h"
#include "Definitions.h"
#include "Classes.h"
#include <time.h>
#include "customSequences.h"

// ===================== OGGETTI =====================
RetriggerableTimer timeSyncTimer;
Calendar           cal;
NP_Led             Led1(LED_COUNT, LED_PIN);
Bounce2::Button    button1, button2;
LightSensor        lightSensor;
//NTC_Sensor       ntc;
//DHT_Sensor       dht;

// ===================== STATO SISTEMA =====================
SysState      sysState       = SYS_BOOT;
bool          syncState      = false;
bool          firstSyncDone  = false;
unsigned long disconnectTime = 0;
char          buf[64];

// ===================== TRACKING VARIABILI CLOUD =====================
bool prev_w_STATE     = false;
bool prev_isRaining   = false;
bool prev_manualLight = false;
int  prev_allWindows  = 0;
bool prev_lightState  = false;
bool firstSync        = true;

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);
  delay(1500);
  initLogging();

  // I/O
  button1.attach(BUTTON1_PIN, INPUT_PULLUP);
  button2.attach(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(CLOSE_W_PIN, OUTPUT);
  pinMode(OPEN_W_PIN, OUTPUT);
  button1.interval(5);
  button2.interval(5);
  button1.setPressedState(LOW);
  button2.setPressedState(LOW);

  // Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(0);
  ArduinoCloud.printDebugInfo();

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, onCloudConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onCloudSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, onCloudDisconnect);

  // Sensori
  lightSensor.init();

  // LED
  Led1.begin();
  Led1.setContinuousBrightness(20);
  Led1.setSequenceBrightness(200);
  Led1.show();

  Serial.println(F("[SYS] Sala1 avviata - SYS_BOOT"));
}

// ===================== LOOP — STATE MACHINE =====================
void loop() {
  ArduinoCloud.update();
  Led1.update();

  switch (sysState) {

    case SYS_BOOT:
      updateWindows();
      break;

    case SYS_RUNNING:
      cloudTimeSync();
      updateWindows();
      updateLight();
      //ntc.update();
      //dht.update();
      checkVariableChanges();
      checkResetCommand();
      break;

    case SYS_DISCONNECTED:
      updateWindows();
      if (millis() - disconnectTime >= DISCONNECT_RESET_MS) {
        Serial.println(F("[SYS] Disconnesso troppo a lungo - RESET"));
        ESP.restart();
      }
      break;
  }
}

// ===================== CLOUD CALLBACKS =====================
void onCloudConnect() {
  Serial.println(F("[CLOUD] Connesso"));
  Led1.startContinuous(0x00FF00);
}

void onCloudSync() {
  Serial.println(F("[CLOUD] Sincronizzato"));
  Led1.startContinuous(0x0000FF);

  snprintf(buf, sizeof(buf), "Sala1: SYNC allW:%d wPos:%d", allWindows, (int)winPos);
  messager = buf;

  syncReset();
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
  onCloudReconnect();

  sysState = SYS_RUNNING;
  Serial.println(F("[SYS] -> SYS_RUNNING"));
}

void onCloudDisconnect() {
  Serial.println(F("[CLOUD] Disconnesso"));
  Led1.startContinuous(0xFF0000);

  if (sysState == SYS_RUNNING) {
    sysState       = SYS_DISCONNECTED;
    disconnectTime = millis();
    Serial.println(F("[SYS] -> SYS_DISCONNECTED"));
  }
}

// ===================== SYNC RESET =====================
void syncReset() {
  winPos = w_STATE ? WIN_OPEN : WIN_CLOSED;

  prev_w_STATE     = w_STATE;
  prev_isRaining   = isRaining;
  prev_manualLight = manualLight;
  prev_allWindows  = allWindows;
  prev_lightState  = lightState;
  oldAll           = allWindows;
}

// ===================== CHECK RESET COMMAND =====================
void checkResetCommand() {
  if (messager == "reset" || messager == "reset1") {
    Serial.println(F("[SYS] Reset richiesto via cloud"));
    ESP.restart();
  }
}

// ===================== VARIABLE CHANGE DETECTION =====================
void checkVariableChanges() {
  if (firstSync) {
    firstSync = false;
    return;
  }

  if (w_STATE != prev_w_STATE) {
    logWindowStateChange(w_STATE, lastWindowAction);
    lastWindowAction = WINDOW_UNKNOWN;
    prev_w_STATE = w_STATE;
  }

  if (isRaining != prev_isRaining) {
    prev_isRaining = isRaining;
  }

  if (manualLight != prev_manualLight) {
    if (manualLight) {
      Led1.start(sequence1, numSteps1);
    } else {
      Led1.start(sequence2, numSteps2);
    }
    prev_manualLight = manualLight;
  }

  if (allWindows != prev_allWindows) {
    prev_allWindows = allWindows;
  }

  if (lightState != prev_lightState) {
    logLightStateChange(lightState, lastLightAction);
    lastLightAction = LIGHT_UNKNOWN;
    prev_lightState = lightState;
  }
}

// ===================== CLOUD CALLBACK STUBS =====================
void onIsRainingChange()   {}
void onAutoWChange()       {}
void onManualLightChange() {}
void onAllWindowsChange()  {}
void onMessagerChange()    {}
void onWSTATEChange()      {}
void onLightStateChange()  {}
void onRainValueChange()   {}
void onLightValueChange()  {}
