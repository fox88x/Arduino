#include "Classes.h"

// ----- Time -----
void getCurrentTime(struct tm &timeinfo) {
  time_t epoch = TimeService.getLocalTime();
  gmtime_r(&epoch, &timeinfo);
}

// ---- Timers ----
// RetriggerableTimer 
void RetriggerableTimer::start(unsigned long interval) {
  _interval = interval;
  _next     = millis() + interval;
  _active   = true;
}

bool RetriggerableTimer::elapsed() const {
  if (!_active) return false;
  return long(millis() - _next) >= 0;
}

unsigned long RetriggerableTimer::interval() const {
  return _interval;
}

void RetriggerableTimer::stop() {
  _active = false;
}

// OneShotTimer
void OneShotTimer::start(unsigned long interval) {
  _interval    = interval;
  _start       = millis();
  _running     = true;
  _everStarted = true;
}

bool OneShotTimer::elapsed() {
  if (!_everStarted || !_running) return false;
  if (long(millis() - _start) >= long(_interval)) {
    _running = false;
    return true;
  }
  return false;
}

bool OneShotTimer::running() const {
  return _running;
}

void OneShotTimer::stop() {
  _running = false;
}


// ---- Calendar ----
void Calendar::updateTime() {
  time_t epoch = TimeService.getLocalTime();
  gmtime_r(&epoch, &timeinfo);
}

bool Calendar::isWorkingDay() {   
  if (!syncState) {
    return false;
  }
  else {
    updateTime();
    hours = timeinfo.tm_hour;
    
    switch (timeinfo.tm_wday) {
        case 0: return false; // Domenica
        case 1:
        case 2:
        case 3:
        case 4: return true; // Lun- Gio
        case 5: return (hours <= 15); // Venerdì fino alle 15:00
        case 6: return (hours < 14);  // Sabato fino alle 13:59
        default: return false;
    }
  }
}

bool Calendar::isWorkingTime() {
  updateTime();
  if (!syncState) {
    return false;
  }
  else {
    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    if ((hours < 20 || (hours == 20 && minutes < 30)) && (hours > 8 || (hours == 8 && minutes >= 30))) {
        return true;
    }
    return false; 
  }
}

void Calendar::printCurrentTime() {
  updateTime();

  char buffer[26];
  snprintf(buffer, sizeof(buffer),
    " %02d/%02d/%04d  %02d:%02d:%02d",
    timeinfo.tm_mday,
    timeinfo.tm_mon  + 1,
    timeinfo.tm_year + 1900,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );
  Serial.println(buffer);
}


// ------------------------------------
// NP_Led
// ------------------------------------
NP_Led::NP_Led(uint16_t numPix, uint8_t pin)
  : strip(numPix, pin, NEO_GRB + NEO_KHZ800),
    numPixels(numPix),
    sequence(nullptr),
    sequenceLen(0),
    active(false),
    currentStep(0),
    lastChange(0),
    continuousBrightness(50),  // default dimming
    sequenceBrightness(255),   // default full
    continuousHex(0) {}

void NP_Led::begin() {
  strip.begin();
  strip.show();
}

void NP_Led::start(const SequenceStep* seq, uint8_t len) {
  if (active) return;
  active = true;
  sequence = seq;
  sequenceLen = len;
  currentStep = 0;
  lastChange = millis();

  // usa brightness impostata per sequenze
  strip.setBrightness(sequenceBrightness);
  strip.fill(sequence[currentStep].color, 0, numPixels);
  strip.show();
}

void NP_Led::startContinuous(uint32_t colorHex) {
  continuousHex = colorHex;
  active = false;

  // in modalità continua usiamo brightness ridotto
  strip.setBrightness(continuousBrightness);

  // unpack raw hex e mostra colore direttamente (no dimming manuale)
  uint8_t r = (continuousHex >> 16) & 0xFF;
  uint8_t g = (continuousHex >> 8) & 0xFF;
  uint8_t b = continuousHex & 0xFF;
  uint32_t encoded = strip.Color(r, g, b);

  strip.fill(encoded, 0, numPixels);
  strip.show();
}

void NP_Led::setContinuousBrightness(uint8_t b) {
  continuousBrightness = b;
  // aggiorna immediatamente se in continuo
  if (!active) {
    startContinuous(continuousHex);
  }
}

void NP_Led::setSequenceBrightness(uint8_t b) {
  sequenceBrightness = b;
}

bool NP_Led::update() {
  if (!active) return false;

  uint32_t now = millis();
  if ((uint32_t)(now - lastChange) >= sequence[currentStep].duration) {
    currentStep++;
    if (currentStep >= sequenceLen) {
      // fine sequenza: torna al continuo
      active = false;
      // ripristina stato continuo
      startContinuous(continuousHex);
      return false;
    }
    lastChange = now;
    strip.fill(sequence[currentStep].color, 0, numPixels);
    strip.show();
  }
  return true;
}

void NP_Led::setBrightness(uint8_t brightness) {
  strip.setBrightness(brightness);
}

void NP_Led::show() {
  strip.show();
}


// ------------------------------------
// Log
// ------------------------------------
// ===== DEFINIZIONE DELLE VARIABILI GLOBALI =====
WindowAction lastWindowAction = WINDOW_UNKNOWN;

// ===== IMPLEMENTAZIONE DELLE FUNZIONI =====
void initLogging() {
  //Serial.println("[LOG] Sistema di logging inizializzato");
  lastWindowAction = WINDOW_UNKNOWN;
}

void logWindowStateChange(bool newState, WindowAction action) {
  messager = String("Sala 4:\n[Windows] ");
  
  switch (action) {
    case WINDOW_MANUAL_OPEN:
      messager = messager + "Apertura MANUALE"; 
      break;
    case WINDOW_MANUAL_CLOSE:
      messager = messager + "Chiusura MANUALE";
      break;
    case WINDOW_AUTO_OPEN:
      messager = messager + "Apertura AUTOMATICA";
      break;
    case WINDOW_AUTO_CLOSE:
      messager = messager + "Chiusura AUTOMATICA";
      break;
    case WINDOW_RAIN_CLOSE:
      messager = messager + "Chiusura per PIOGGIA";
      break;
    case WINDOW_SENSOR_ERROR_CLOSE:
      messager = messager + "Chiusura di SICUREZZA (errore sensore pioggia)";
      break;
    case WINDOW_SEND_ALL_OPEN:
      messager = messager + "Apertura TOTALE (invio comando all)";
      break;
    case WINDOW_SEND_ALL_CLOSE:
      messager = messager + "Chiusura TOTALE (invio comando all)";
      break;
    case WINDOW_ALL_OPEN:
      messager = messager + "Apertura TOTALE (comando remoto)";
      break;
    case WINDOW_ALL_CLOSE:
      messager = messager + "Chiusura TOTALE (comando remoto)";
      break;
    default:
      messager = messager + "Causa SCONOSCIUTA";
      break;
  }
  
  messager = messager + "\nW_State: " + (newState ? "APERTE" : "CHIUSE") + "\n";
}

/*
  void logTFavorableChange(bool newValue, int tempIn, int tempOut) {
  messager = String("Sala 4:\n[Thermal condition] ");
  
  if (newValue) {
    messager = messager + "FAVOREVOLI ";
  } else {
    messager = messager + "SFAVOREVOLI ";
  }
  messager = messager + "(Tin= )" + String(tempIn) + "°C)\n";
  messager = messager + "(Tout= )" + String(tempOut) + "°C)\n";
}*/

void setWindowAction(WindowAction action) {
  lastWindowAction = action;
}