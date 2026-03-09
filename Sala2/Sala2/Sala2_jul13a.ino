// Sala2_jul13a.ino — Nano ESP32
// Comunicazione ESP-NOW peer-to-peer

#include "arduino_secrets.h"
#include <Bounce2.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Definitions.h"
#include "EspNowProtocol.h"
#include "Classes.h"
#include <time.h>
#include "customSequences.h"

// ===================== OGGETTI =====================
RetriggerableTimer timeSyncTimer;
Calendar           cal;
NP_Led             Led1(LED_COUNT, LED_PIN);
Bounce2::Button    button1, button2;

// ===================== STATO SISTEMA =====================
SysState sysState      = SYS_BOOT;
WinPos   winPos        = WIN_CLOSED;
bool     manualLight   = false;
bool     syncState     = false;
bool     firstSyncDone = false;
uint8_t  syncFailCount = 0;

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1500);
  initLogging();

  Serial.println(F("========================================"));
  Serial.println(F("  Sala2 — ESP-NOW Edition"));
  Serial.println(F("========================================"));

  // I/O
  button1.attach(BUTTON1_PIN, INPUT_PULLUP);
  button2.attach(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(CLOSE_W_PIN, OUTPUT);
  pinMode(OPEN_W_PIN, OUTPUT);
  button1.interval(5);
  button2.interval(5);
  button1.setPressedState(LOW);
  button2.setPressedState(LOW);

  // LED
  Led1.begin();
  Led1.setContinuousBrightness(20);
  Led1.setSequenceBrightness(200);
  Led1.startContinuous(0xFFAA00);
  Led1.show();

  // WiFi + NTP
  WiFi.mode(WIFI_STA);
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  initWifi();

  // ESP-NOW
  if (!initEspNow()) {
    Serial.println(F("[SYS] ESP-NOW init fallita — RESET tra 5s"));
    Led1.startContinuous(0xFF0000);
    delay(5000);
    ESP.restart();
  }

  timeSyncTimer.start(SYNC_FAST_INTERVAL);
  sysState = SYS_RUNNING;
  Led1.startContinuous(0x0000FF);
  Serial.println(F("[SYS] Sala2 avviata — SYS_RUNNING (ESP-NOW)"));
}

// ===================== LOOP =====================
void loop() {
  processEspNow();
  updateWindows();
  ntpTimeSync();
  checkWifiReconnect();
  Led1.update();
  updateLedStatus();
}

// ===================== NTP TIME SYNC =====================
void ntpTimeSync() {
  if (!timeSyncTimer.elapsed()) return;

  struct tm ti;
  if (getLocalTime(&ti, 100)) {
    if (!firstSyncDone) {
      Serial.println(F("[NTP] Sincronizzato"));
      cal.updateTime();
      cal.printCurrentTime();
      Led1.start(sequence5, numSteps5);
      firstSyncDone = true;
    }
    syncState     = true;
    syncFailCount = 0;
    timeSyncTimer.start(SYNC_SLOW_INTERVAL);
  } else {
    syncFailCount++;
    if (syncFailCount >= SYNC_FAIL_MAX) {
      syncState = false;
    }
    timeSyncTimer.start(SYNC_FAST_INTERVAL);
  }
}

// ===================== LED STATUS =====================
void updateLedStatus() {
  static uint32_t lastLedColor = 0;
  uint32_t color;

  uint8_t online = getOnlinePeerCount();
  if (online == NUM_SALAS - 1) {
    color = 0x0000FF;
  } else if (online > 0) {
    color = 0x00FF00;
  } else {
    color = 0xFFAA00;
  }

  if (color != lastLedColor) {
    Led1.startContinuous(color);
    lastLedColor = color;
  }
}
