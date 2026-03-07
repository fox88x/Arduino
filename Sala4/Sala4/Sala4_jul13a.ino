// Sala4_jul13a.ino — Nano 33 IoT (SAMD21)
// Architettura a stati con auto-recovery
//
// SYS_BOOT → (cloud sync) → SYS_RUNNING ⇄ SYS_DISCONNECTED
//                                              ↓ (timeout)
//                                          NVIC_SystemReset()

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

// ===================== STATO SISTEMA =====================
SysState      sysState       = SYS_BOOT;
bool          syncState      = false;
bool          firstSyncDone  = false;
unsigned long disconnectTime = 0;    // Quando è iniziata la disconnessione
char          buf[64];

// ===================== TRACKING VARIABILI CLOUD =====================
bool prev_w_STATE     = false;
bool prev_isRaining   = false;
bool prev_manualLight = false;
int  prev_allWindows  = 0;
bool firstSync        = true;

// ===================== SETUP =====================
void setup() {
  Serial.begin(19200);
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
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, onCloudConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onCloudSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, onCloudDisconnect);

  // LED
  Led1.begin();
  Led1.setContinuousBrightness(20);
  Led1.setSequenceBrightness(200);
  Led1.show();

  Serial.println(F("[SYS] Sala4 avviata - SYS_BOOT"));
}

// ===================== LOOP — STATE MACHINE =====================
void loop() {
  ArduinoCloud.update();
  Led1.update();

  switch (sysState) {

    case SYS_BOOT:
      // In attesa della prima sincronizzazione cloud.
      // I pulsanti funzionano comunque per sicurezza.
      updateWindows();
      break;

    case SYS_RUNNING:
      // Operatività normale
      cloudTimeSync();
      updateWindows();
      checkVariableChanges();
      checkResetCommand();
      break;

    case SYS_DISCONNECTED:
      // Cloud perso — continua a far funzionare i pulsanti locali
      updateWindows();

      // Auto-reset dopo DISCONNECT_RESET_MS
      if (millis() - disconnectTime >= DISCONNECT_RESET_MS) {
        Serial.println(F("[SYS] Disconnesso troppo a lungo - RESET"));
        NVIC_SystemReset();
      }
      break;
  }
}

// ===================== CLOUD CALLBACKS =====================
void onCloudConnect() {
  Serial.println(F("[CLOUD] Connesso"));
  Led1.startContinuous(0x00FF00);  // Verde
}

void onCloudSync() {
  Serial.println(F("[CLOUD] Sincronizzato"));
  Led1.startContinuous(0x0000FF);  // Blu

  // Log stato al momento della sync
  snprintf(buf, sizeof(buf), "Sala4: SYNC allW:%d wPos:%d", allWindows, (int)winPos);
  messager = buf;

  // Allinea stato locale con cloud
  syncReset();

  // Avvia time sync
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
  onCloudReconnect();

  // Transizione → RUNNING
  sysState = SYS_RUNNING;
  Serial.println(F("[SYS] -> SYS_RUNNING"));
}

void onCloudDisconnect() {
  Serial.println(F("[CLOUD] Disconnesso"));
  Led1.startContinuous(0xFF0000);  // Rosso

  if (sysState == SYS_RUNNING) {
    sysState       = SYS_DISCONNECTED;
    disconnectTime = millis();
    Serial.println(F("[SYS] -> SYS_DISCONNECTED"));
  }
}

// ===================== SYNC RESET =====================
void syncReset() {
  // Allinea posizione finestra da cloud
  winPos = w_STATE ? WIN_OPEN : WIN_CLOSED;

  // Snapshot variabili per change detection
  prev_w_STATE     = w_STATE;
  prev_isRaining   = isRaining;
  prev_manualLight = manualLight;
  prev_allWindows  = allWindows;
  oldAll           = allWindows;
}

// ===================== CHECK RESET COMMAND =====================
void checkResetCommand() {
  if (messager == "reset" || messager == "reset4") {
    Serial.println(F("[SYS] Reset richiesto via cloud"));
    NVIC_SystemReset();
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
}

// ===================== CLOUD CALLBACK STUBS (richiesti dal cloud) =====================
void onIsRainingChange()   {}
void onAllWindowsChange()  {}
void onMessagerChange()    {}
void onWSTATEChange()      {}
void onManualLightChange() {}
