// WindowManager.ino — Sala4 — State Machine
//
// Stati: WIN_IDLE → WIN_RELAY_PAUSE → WIN_MOVING → WIN_IDLE
//

// ===================== STATO FINESTRE =====================
WinState winState      = WIN_IDLE;
WinPos   winPos        = WIN_CLOSED;
int      pendingPin    = 0;           // Pin relay in attesa di attivazione
unsigned long winTimer = 0;           // Timer unificato per pausa e movimento

// ===================== ALLWINDOWS =====================
int  oldAll            = 0;
bool isSender          = false;
unsigned long allStart = 0;

// ===================== PULSANTI =====================
unsigned long btn1Start = 0;
unsigned long btn2Start = 0;

// ===================== BUTTON HANDLER =====================
void processButton(Bounce2::Button& btn, unsigned long& startTime, int relayPin, int cmdValue) {
  if (btn.fell()) {
    startTime = millis();
  }

  if (btn.rose()) {
    unsigned long dur = millis() - startTime;

    if (dur < SHORT_PRESS_MS) {
      // Pressione breve: comando finestra locale
      setWindowAction((relayPin == OPEN_W_PIN) ? WINDOW_MANUAL_OPEN : WINDOW_MANUAL_CLOSE);
      activateRelay(relayPin);
    }
    else if (dur < LONG_PRESS_MS) {
      // Pressione media: comando ALL
      allWindows = cmdValue;
      isSender = true;
      setWindowAction((relayPin == OPEN_W_PIN) ? WINDOW_SEND_ALL_OPEN : WINDOW_SEND_ALL_CLOSE);
      activateRelay(relayPin);
      Led1.start(sequence11, numSteps11);
    }
    else {
      // Pressione lunga: toggle manualLight
      if (cmdValue == 2) {
        manualLight = !manualLight;
      }
    }
  }
}

// ===================== ATTIVAZIONE RELAY =====================
void activateRelay(int relayPin) {
  // Se un relay è già attivo in movimento, ignora
  if (winState == WIN_MOVING) return;

  // Spegni relay opposto
  int opposite = (relayPin == OPEN_W_PIN) ? CLOSE_W_PIN : OPEN_W_PIN;
  digitalWrite(opposite, LOW);

  // Avvia pausa non bloccante prima di accendere il nuovo relay
  pendingPin = relayPin;
  winTimer   = millis();
  winState   = WIN_RELAY_PAUSE;
}

// ===================== STATE MACHINE FINESTRE =====================
void updateWindows() {
  unsigned long now = millis();
  button1.update();
  button2.update();

  // --- State machine relay ---
  switch (winState) {

    case WIN_IDLE:
      // Niente da fare, in attesa di comando
      break;

    case WIN_RELAY_PAUSE:
      // Attesa 500ms dopo spegnimento relay opposto
      if (now - winTimer >= RELAY_SWITCH_PAUSE_MS) {
        digitalWrite(pendingPin, HIGH);
        winTimer = now;
        winPos   = WIN_TRANSIT;
        winState = WIN_MOVING;
      }
      break;

    case WIN_MOVING:
      // Relay attivo, in attesa del completamento movimento
      if (now - winTimer >= WINDOW_MOVE_MS) {
        digitalWrite(pendingPin, LOW);
        if (pendingPin == OPEN_W_PIN) {
          winPos  = WIN_OPEN;
          w_STATE = true;
        } else {
          winPos  = WIN_CLOSED;
          w_STATE = false;
        }
        pendingPin = 0;
        winState   = WIN_IDLE;
      }
      break;
  }

  // --- Pulsanti (solo se non in pausa relay) ---
  processButton(button1, btn1Start, OPEN_W_PIN, 1);
  processButton(button2, btn2Start, CLOSE_W_PIN, 2);

  // --- Comando allWindows ricevuto da remoto ---
  if (oldAll != allWindows) {
    if (!isSender) {
      switch (allWindows) {
        case 1:
          setWindowAction(WINDOW_ALL_OPEN);
          activateRelay(OPEN_W_PIN);
          break;
        case 2:
          setWindowAction(WINDOW_ALL_CLOSE);
          activateRelay(CLOSE_W_PIN);
          break;
      }
    }
    allStart = now;
    oldAll   = allWindows;
    isSender = false;
  }

  // --- Timeout allWindows (reset a 0 dopo 30s) ---
  if (allWindows != 0 && (now - allStart > ALL_CMD_TIMEOUT_MS)) {
    allWindows = 0;
  }
}
