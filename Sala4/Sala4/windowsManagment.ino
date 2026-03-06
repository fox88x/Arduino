// WindowsManagment.ino
unsigned long allStartTime = 0;
unsigned long relayStartTime = 0;   // Timer unificato per il relè attivo
int activeRelay = 0;                // 0 = nessuno, 1 = apertura, 2 = chiusura

unsigned long now = 0;
unsigned long button1StartTime = 0;
unsigned long button2StartTime = 0;

bool isSender = false;

// Pausa non bloccante tra spegnimento relè opposto e accensione nuovo
int pendingRelayPin = 0;            // 0 = nessuna attivazione in attesa
unsigned long pendingRelayTime = 0;
const unsigned long RELAY_SWITCH_PAUSE = 500;


void processButton(Bounce2::Button& button, unsigned long& startTime, int buttonPin, int relayPin, int commandValue) {
  if (button.fell()) {
    startTime = millis();
  }

  if (button.rose()) {
    unsigned long duration = millis() - startTime;

    if (duration < shortPressTimer) {

      WindowAction action = (relayPin == OPEN_W_PIN) ? WINDOW_MANUAL_OPEN : WINDOW_MANUAL_CLOSE;
      setWindowAction(action);

      activateWindowsRelay(relayPin);
    }
    else if (duration >= shortPressTimer && duration < prolongedPressTimer) {
      // Comando ALL sempre permesso
      allWindows = commandValue;
      isSender = true;
      activateWindowsRelay(relayPin);

      WindowAction action = (relayPin == OPEN_W_PIN) ? WINDOW_SEND_ALL_OPEN : WINDOW_SEND_ALL_CLOSE;
      setWindowAction(action);

      Led1.start(sequence11, numSteps11);
    }
    else {
      // Modifica impostazioni
      if (commandValue == 1) {
          //auto_w = !auto_w;
      }
      else {
        manualLight = !manualLight;
      }
    }
  }
}

// GESTIONE DEI PULSANTI E controllo dei relè
void checkWindows() {
  now = millis();
  button1.update();
  button2.update();

  // Completamento attivazione relè dopo pausa non bloccante
  if (pendingRelayPin != 0 && (now - pendingRelayTime >= RELAY_SWITCH_PAUSE)) {
    digitalWrite(pendingRelayPin, HIGH);
    activeRelay = (pendingRelayPin == OPEN_W_PIN) ? 1 : 2;
    relayStartTime = now;
    windowState = 0; // in transizione
    pendingRelayPin = 0;
  }

  // Gestione dei pulsanti
  processButton(button1, button1StartTime, BUTTON1_PIN, OPEN_W_PIN, 1);
  processButton(button2, button2StartTime, BUTTON2_PIN, CLOSE_W_PIN, 2);


  if (oldAll != allWindows) {
    if(!isSender) {
      switch (allWindows) {
        case 1:
          setWindowAction(WINDOW_ALL_OPEN);
          activateWindowsRelay(OPEN_W_PIN);
          break;
        case 2:
          setWindowAction(WINDOW_ALL_CLOSE);
          activateWindowsRelay(CLOSE_W_PIN);
          break;
      }
    }

    allStartTime = now;
    oldAll = allWindows;
    isSender = false;
  }

  if (allWindows != 0) {
    if(now - allStartTime > allTimer) {
        allWindows = 0;
    }
  }

  // Disattivazione del relè attivo dopo il tempo prestabilito
  if (activeRelay != 0 && (now - relayStartTime >= windowMovementTimer)) {
    if (activeRelay == 1) {
      digitalWrite(OPEN_W_PIN, LOW);
      windowState = 1; // finestra aperta
      w_STATE = true;
    }
    else if (activeRelay == 2) {
      digitalWrite(CLOSE_W_PIN, LOW);
      windowState = -1; // finestra chiusa
      w_STATE = false;
    }
    unsigned long elapsed = now - relayStartTime;
    activeRelay = 0;
  }
}

void activateWindowsRelay(int windowsRelayPin) {
  // Determina il relè opposto:
  int oppositeRelay = (windowsRelayPin == OPEN_W_PIN) ? CLOSE_W_PIN : OPEN_W_PIN;
  digitalWrite(oppositeRelay, LOW);

  // Avvia pausa non bloccante prima di accendere il nuovo relè
  pendingRelayPin = windowsRelayPin;
  pendingRelayTime = now;
}


/*
  struct TimeInterval {
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;

  bool isActive() const {
    int current = cal.timeinfo.tm_hour * 60 + cal.timeinfo.tm_min;
    int start   = startHour * 60 + startMinute;
    int end     = endHour * 60 + endMinute;

    if (start <= end) {
      // Intervallo normale (es. 08:00–17:00)
      return (current >= start) && (current <= end);
    } else {
      // Intervallo che attraversa la mezzanotte (es. 22:00–06:00)
      return (current >= start) || (current <= end);
    }
  }
};

// Intervallo apertura finestre;
static const TimeInterval intervalDayFav     { 13, 30, 15, 00 };  // 13:30–15:00
static const TimeInterval intervalNightFav   { 21, 00, 06, 30 };  // 21:00–06:30
static const TimeInterval intervalDayUnfav   { 13, 30, 13, 50 };  // 13:30–13:50
static const TimeInterval intervalNightUnfav { 20, 30, 20, 50 };  // 20:30–20:50

// Gestione automatica delle finestre
void manageAutoWindows() {
  if (!autoWDelay.elapsed()) return;
  autoWDelay.start(AUTOWINDOWS_INTERVAL);

  // Condizioni generali per automazione (solo se sensore OK)
  if (!(auto_w && syncState && !isRaining)) {
    if (virtualAutoW) {
      setWindowAction(WINDOW_AUTO_CLOSE);
      activateWindowsRelay(CLOSE_W_PIN);
      virtualAutoW = false;
    }
    return;
  }

  cal.updateTime();
  int nowMin = cal.timeinfo.tm_hour * 60 + cal.timeinfo.tm_min;
  bool isEvening = (nowMin >= 20*60+30) || (nowMin < 7*60);
  const TimeInterval& favInt   = isEvening ? intervalNightFav   : intervalDayFav;
  const TimeInterval& unfavInt = isEvening ? intervalNightUnfav : intervalDayUnfav;
  const TimeInterval& currentInt = tFavorable ? favInt : unfavInt;

  bool isActiveNow = currentInt.isActive();

  if (isActiveNow) {
    if (!virtualAutoW) {
      if (windowState == -1) {
        setWindowAction(WINDOW_AUTO_OPEN);
        activateWindowsRelay(OPEN_W_PIN);
      }
      virtualAutoW = true;
      syncVirtualAutoW = virtualAutoW;
    }
  }
  else {
    if (virtualAutoW) {
      setWindowAction(WINDOW_AUTO_CLOSE);
      activateWindowsRelay(CLOSE_W_PIN);
      virtualAutoW = false;
    }
  }
}
*/
