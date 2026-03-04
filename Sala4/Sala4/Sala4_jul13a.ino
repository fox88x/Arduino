#include "arduino_secrets.h"
#include <Bounce2.h>
#include "thingProperties.h"
#include "Definitions.h"
#include "Classes.h"
#include <time.h>
#include "customSequences.h"
//#include <Arduino_DebugUtils.h>

// --- Virtual Delay Timers ---
RetriggerableTimer timeSyncTimer;

// --- Altri oggetti ---
Calendar cal;
NP_Led Led1(LED_COUNT, LED_PIN);
Bounce2::Button button1, button2;


void setup() {
  Serial.begin(19200);
  delay(1500);

  // Setup I/O
  button1.attach(BUTTON1_PIN, INPUT_PULLUP);
  button2.attach(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(CLOSE_W_PIN, OUTPUT);
  pinMode(OPEN_W_PIN, OUTPUT);

  button1.interval(5);
  button2.interval(5);
  button1.setPressedState(LOW);
  button2.setPressedState(LOW);
  
  // Inizializza proprietà Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  //Debug.setDebugLevel(DBG_VERBOSE);
  //Debug.setDebugOutputStream(&Serial);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Callback Cloud
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);
  
  Led1.begin();
  Led1.setContinuousBrightness(20);  // più basso
  Led1.setSequenceBrightness(200);   // più alto
  Led1.show();
}

void loop() {
  ArduinoCloud.update();
  cloudTimeSync();
  checkWindows();
  
  Led1.update();

  checkVariableChanges();
  
  if (messager == "reset" || messager == "reset4") {
    NVIC_SystemReset(); 
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void sync_reset() {
  if (w_STATE) {
    windowState=1;
  }
  else
  {
    windowState=-1;
  }
  prev_w_STATE = w_STATE;
  prev_isRaining = isRaining;
  prev_manualLight = manualLight;
  prev_allWindows = allWindows;
}

// --- CALLBACKS della sincronizzazione e comunicazione Cloud ---
void doThisOnConnect(){
  Serial.println("Board connected to Arduino IoT Cloud");
  Led1.startContinuous(0x00FF00);
}

void doThisOnSync(){
  Serial.println(F(" [CLOUD]  ✅  Proprietà sincronizzate con il Cloud."));
  Led1.startContinuous(0x0000FF);
  snprintf(buf, sizeof(buf), "Sala4: RESET allW:%d wState:%d", allWindows, windowState);
  messager = buf;
  sync_reset();
  timeSyncTimer.start(SYNC_FAST_INTERVAL);
  onCloudReconnect();
}

void doThisOnDisconnect(){
  Serial.println("Board disconnected from Arduino IoT Cloud");
  Led1.startContinuous(0xFF0000);
}


///////////////////////////////////////////////////////////////////////////////////////////

void checkVariableChanges() {
  // Salta il controllo alla prima sincronizzazione per evitare falsi trigger
  if (firstSync) {
    firstSync = false;
    return;
  }

  if (w_STATE != prev_w_STATE) {
    onW_STATERealChange(w_STATE);
    prev_w_STATE = w_STATE;
  }
  
  if (isRaining != prev_isRaining) {
    onIsRainingRealChange(isRaining);
    prev_isRaining = isRaining;
  }
  
  if (manualLight != prev_manualLight) {
    onManualLightRealChange(manualLight);
    prev_manualLight = manualLight;
  }
  
  if (allWindows != prev_allWindows) {
    onAllWindowsRealChange(allWindows);
    prev_allWindows = allWindows;
  }
}

void onW_STATERealChange(bool newValue) {
  logWindowStateChange(newValue, lastWindowAction);
  lastWindowAction = WINDOW_UNKNOWN; // Reset dopo il log
}

void onIsRainingRealChange(bool newValue) {
  if (isRaining) {
    // Controlla se è un errore del sensore
  }
}

void onManualLightRealChange(bool newValue) {
 if (manualLight) {
        Led1.start(sequence1, numSteps3);
    } else {
        Led1.start(sequence2, numSteps4);
    }
}

void onAllWindowsRealChange(int newValue) {
}


///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SPAZZATURA////////////////////////////////////////////
void onIsRainingChange() {}
void onAutoWChange() {}
void onAllWindowsChange() {}
void onMessagerChange() {}
void onWSTATEChange() {}
void onManualLightChange()  {}
///////////////////////////////////////////////////////////////////////////////////////////


/*
void checkSync() {
  // Forza la sincronizzazione e verifica il risultato
  if (TimeService.sync()) {
      Serial.println("SYNC");
  } else {
      Serial.println("sync Faill");
  }
}
*/

