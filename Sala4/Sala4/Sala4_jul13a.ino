// Sala4_jul13a.ino — Nano ESP32
// Comunicazione ESP-NOW peer-to-peer (senza Arduino Cloud)
//
// Vantaggi ESP-NOW:
//   - Comunicazione diretta, latenza ~1-5ms
//   - Funziona senza router WiFi
//   - Dati strutturati (struct packed)
//   - Nessun limite variabili cloud
//   - WiFi usato SOLO per NTP (time sync)

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

  // LED — giallo durante il boot
  Led1.begin();
  Led1.setContinuousBrightness(20);
  Led1.setSequenceBrightness(200);
  Led1.startContinuous(0xFFAA00);
  Led1.show();

  // WiFi in modalità STA (richiesto per ESP-NOW)
  WiFi.mode(WIFI_STA);

  // NTP — configura timezone e server (sincronizza in background quando WiFi disponibile)
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");

  // Connessione WiFi (per NTP, timeout 5s)
  initWifi();

  // ESP-NOW — inizializza comunicazione peer-to-peer
  if (!initEspNow()) {
    Serial.println(F("[SYS] ESP-NOW init fallita — RESET tra 5s"));
    Led1.startContinuous(0xFF0000);
    delay(5000);
    ESP.restart();
  }

  // Timer sync NTP
  timeSyncTimer.start(SYNC_FAST_INTERVAL);

  // Sistema pronto
  sysState = SYS_RUNNING;
  Led1.startContinuous(0x0000FF);
  Serial.println(F("[SYS] Sala4 avviata — SYS_RUNNING (ESP-NOW)"));

  // Stampa riepilogo configurazione
  Serial.print(F("[SYS] MAC: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("[SYS] Peer attesi: "));
  Serial.println(NUM_SALAS - 1);
}

// ===================== LOOP =====================
void loop() {
  // 1. Comunicazione ESP-NOW (ricevi, processa, ping, broadcast stato)
  processEspNow();

  // 2. Finestre (pulsanti + relay state machine)
  updateWindows();

  // 3. NTP time sync
  ntpTimeSync();

  // 4. WiFi reconnect (per NTP)
  checkWifiReconnect();

  // 5. LED status basato su peer
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
      Led1.start(sequence5, numSteps5);
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
void updateLedStatus() {
  // Non sovrascrivere sequenze in corso
  static uint32_t lastLedColor = 0;
  uint32_t color;

  uint8_t online = getOnlinePeerCount();
  if (online == NUM_SALAS - 1) {
    color = 0x0000FF;   // Blu:      tutti i peer connessi
  } else if (online > 0) {
    color = 0x00FF00;   // Verde:    alcuni peer connessi
  } else {
    color = 0xFFAA00;   // Arancione: nessun peer
  }

  if (color != lastLedColor) {
    Led1.startContinuous(color);
    lastLedColor = color;
  }
}
