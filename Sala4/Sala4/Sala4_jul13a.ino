// Sala4_jul13a.ino — Nano ESP32
// Comunicazione ESP-NOW peer-to-peer
//
// FRAM MB85RC256V per persistenza stato
// Comando globale con retry 30s/5min
// Sicurezza offline: chiusura dopo 5min senza peer
// LED semplificato: blu/arancione/rosso

#include "arduino_secrets.h"
#include <Bounce2.h>
#include <WiFi.h>
#include <Wire.h>
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
  Serial.println(F("  Sala4 — ESP-NOW Edition"));
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

  // LED — flash blu alta intensità al boot
  Led1.begin();
  Led1.setSequenceBrightness(200);
  Led1.setContinuousBrightness(200);
  Led1.startContinuous(0x0000FF);
  Led1.show();
  delay(500);
  Led1.setContinuousBrightness(20);
  Led1.startContinuous(0xFFAA00);
  Led1.show();

  // FRAM — persistenza stato
  initFram();
  if (loadState()) {
    Serial.println(F("[SYS] Stato ripristinato da FRAM"));
  }

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
  Serial.println(F("[SYS] Sala4 avviata — SYS_RUNNING (ESP-NOW)"));

  Serial.print(F("[SYS] MAC: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("[SYS] Peer attesi: "));
  Serial.println(NUM_SALAS - 1);
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
      Serial.println(F("[NTP] Prima sincronizzazione completata"));
      cal.updateTime();
      cal.printCurrentTime();
      firstSyncDone = true;
    }
    syncState     = true;
    syncFailCount = 0;
    timeSyncTimer.start(SYNC_SLOW_INTERVAL);
  } else {
    syncFailCount++;
    if (syncFailCount <= 3) {
      Serial.println(F("[NTP] Sincronizzazione fallita"));
    }
    if (syncFailCount >= SYNC_FAIL_MAX) {
      syncState = false;
    }
    timeSyncTimer.start(SYNC_FAST_INTERVAL);
  }
}

// ===================== LED STATUS =====================
// Blu:      tutti i peer online + nessun comando pendente
// Arancione: alcuni peer offline o comando pendente
// Rosso:    nessun peer online (isolata)
void updateLedStatus() {
  static uint32_t lastLedColor = 0;
  uint32_t color;

  uint8_t online = getOnlinePeerCount();
  if (online == 0) {
    color = 0xFF0000;       // Rosso: isolata
  } else if (online < NUM_SALAS - 1 || hasPendingCommands()) {
    color = 0xFFAA00;       // Arancione: parziale o pending
  } else {
    color = 0x0000FF;       // Blu: tutto ok
  }

  if (color != lastLedColor) {
    Led1.startContinuous(color);
    lastLedColor = color;
  }
}
