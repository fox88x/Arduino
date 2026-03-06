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
  }

  if (allWindows != 0) {
    if(now - allStartTime > allTimer) {
        allWindows = 0;
    }
  }

  if (allWindows == 0 && isSender) {
    isSender = false;
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
