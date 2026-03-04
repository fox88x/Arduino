#include "arduino_secrets.h"
//Sketch.ino
#include <Bounce2.h>
#include "thingProperties.h"
#include "Definitions.h"
#include "Classes.h"
#include <time.h>
#include "customSequences.h"

RetriggerableTimer timeSyncTimer, lightUpdateDelay;
//RetriggerableTimer timeSyncTimer, lightUpdateDelay, autoWDelay;

Bounce2::Button button1, button2;

LightSensor lightSensor;
//NTC_Sensor ntc;
//DHT_Sensor dht;
Calendar cal;
NP_Led Led1(LED_COUNT, LED_PIN);


void setup() {
  Serial.begin(9600);
  delay(1500);
  initLogging();

  // Setup I/O
  button1.attach(BUTTON1_PIN, INPUT_PULLUP);
  button2.attach(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(CLOSE_W_PIN, OUTPUT);
  pinMode(OPEN_W_PIN, OUTPUT);

  button1.interval(5);
  button2.interval(5);
  button1.setPressedState(LOW);
  button2.setPressedState(LOW);
  
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(0);    //2
  ArduinoCloud.printDebugInfo();

  // Callback Cloud
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  lightSensor.init(); 

  Led1.begin();
  Led1.setContinuousBrightness(20);  
  Led1.setSequenceBrightness(200);   
  Led1.show();
}

void loop() {
  ArduinoCloud.update();
  cloudTimeSync();
  checkWindows();
  checkLight();
  
  //ntc.update();
  //dht.update();
  Led1.update();

  checkVariableChanges();

  if (messager == "reset" || messager == "reset1") {
    ESP.restart(); 
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
  prev_lightState = lightState;
}

// --- CALLBACKS della sincronizzazione e comunicazione Cloud ---
void doThisOnConnect(){
  Serial.println("Board connected to Arduino IoT Cloud");
  Led1.startContinuous(0x00FF00);
}

void doThisOnSync(){
  Serial.println(F(" [CLOUD]  ✅  Proprietà sincronizzate con il Cloud."));
  Led1.startContinuous(0x0000FF);
  snprintf(buf, sizeof(buf), "Sala1: RESET allW:%d wState:%d", allWindows, windowState);
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
  
  if (lightState != prev_lightState) {
    onLightStateRealChange(lightState);
    prev_lightState = lightState;
  }
}


// Nuove funzioni che si attivano solo al cambio reale

void onW_STATERealChange(bool newValue) {
  logWindowStateChange(newValue, lastWindowAction);
  lastWindowAction = WINDOW_UNKNOWN; // Reset dopo il log
}

void onIsRainingRealChange(bool newValue) {
  if (isRaining) {
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

void onLightStateRealChange(bool newValue) {
  logLightStateChange(newValue, lastLightAction);
  lastLightAction = LIGHT_UNKNOWN; // Reset dopo il log
}


///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////SPAZZATURA////////////////////////////////////////////
void onIsRainingChange() {}
void onAutoWChange() {}
void onManualLightChange()  {}
void onAllWindowsChange() {}
void onMessagerChange() {}
void onWSTATEChange() {}
void onLightStateChange()  {}
void onRainValueChange()  {}
void onLightValueChange() {}
//void onTFavorableChange()  {}
///////////////////////////////////////////////////////////////////////////////////////////