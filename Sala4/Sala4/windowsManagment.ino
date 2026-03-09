// windowsManagment.ino — State Machine Finestre (ESP-NOW)
//
// Stati: WIN_IDLE → WIN_RELAY_PAUSE → WIN_MOVING → WIN_IDLE
// Salva stato su FRAM dopo ogni movimentazione completata.

// ===================== STATO FINESTRE =====================
WinState      winState   = WIN_IDLE;
int           pendingPin = 0;
unsigned long winTimer   = 0;

// ===================== PULSANTI =====================
unsigned long btn1Start  = 0;
unsigned long btn2Start  = 0;

// ===================== HELPER =====================
bool isWindowIdle() {
  return winState == WIN_IDLE;
}

// ===================== BUTTON HANDLER =====================
void processButton(Bounce2::Button& btn, unsigned long& startTime, int relayPin, int btnId) {
  if (btn.fell()) {
    startTime = millis();
  }

  if (btn.rose()) {
    unsigned long dur = millis() - startTime;

    if (dur < SHORT_PRESS_MS) {
      // Pressione corta: muovi finestra locale
      setWindowAction((relayPin == OPEN_W_PIN) ? WINDOW_MANUAL_OPEN : WINDOW_MANUAL_CLOSE);
      activateRelay(relayPin);
    }
    else if (dur < LONG_PRESS_MS) {
      // Pressione lunga: comando apertura/chiusura TOTALE via ESP-NOW
      CmdType cmd = (relayPin == OPEN_W_PIN) ? CMD_ALL_OPEN : CMD_ALL_CLOSE;
      sendGlobalCommand(cmd);

      setWindowAction((relayPin == OPEN_W_PIN) ? WINDOW_SEND_ALL_OPEN : WINDOW_SEND_ALL_CLOSE);
      activateRelay(relayPin);

      Serial.print(F("[WIN] Comando totale inviato: "));
      Serial.println(cmd == CMD_ALL_OPEN ? F("APERTURA") : F("CHIUSURA"));
    }
    else {
      // Pressione molto lunga: toggle manualLight (solo pulsante 2)
      if (btnId == 2) {
        manualLight = !manualLight;
        saveState();
        notifyStateChanged();
        Serial.print(F("[WIN] ManualLight: "));
        Serial.println(manualLight ? F("ON") : F("OFF"));
      }
    }
  }
}

// ===================== ATTIVAZIONE RELAY =====================
void activateRelay(int relayPin) {
  if (winState != WIN_IDLE) return;

  // Disattiva relay opposto prima di attivare quello richiesto
  int opposite = (relayPin == OPEN_W_PIN) ? CLOSE_W_PIN : OPEN_W_PIN;
  digitalWrite(opposite, LOW);

  pendingPin = relayPin;
  winTimer   = millis();
  winState   = WIN_RELAY_PAUSE;
}

// ===================== STATE MACHINE FINESTRE =====================
void updateWindows() {
  unsigned long now = millis();
  button1.update();
  button2.update();

  switch (winState) {

    case WIN_IDLE:
      break;

    case WIN_RELAY_PAUSE:
      if (now - winTimer >= RELAY_SWITCH_PAUSE_MS) {
        digitalWrite(pendingPin, HIGH);
        winTimer = now;
        winPos   = WIN_TRANSIT;
        winState = WIN_MOVING;
        notifyStateChanged();
      }
      break;

    case WIN_MOVING:
      if (now - winTimer >= WINDOW_MOVE_MS) {
        digitalWrite(pendingPin, LOW);

        if (pendingPin == OPEN_W_PIN) {
          winPos = WIN_OPEN;
        } else {
          winPos = WIN_CLOSED;
        }

        pendingPin = 0;
        winState   = WIN_IDLE;
        saveState();
        notifyStateChanged();

        logWindowStateChange(winPos == WIN_OPEN, lastWindowAction);
        lastWindowAction = WINDOW_UNKNOWN;
      }
      break;
  }

  // Processa pulsanti
  processButton(button1, btn1Start, OPEN_W_PIN, 1);
  processButton(button2, btn2Start, CLOSE_W_PIN, 2);
}
