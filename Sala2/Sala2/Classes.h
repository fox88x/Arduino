#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


// ------------------------------------
// Timers
// ------------------------------------
class RetriggerableTimer {
public:
  void start(unsigned long interval);
  bool elapsed() const;
  unsigned long interval() const;
  void stop();

private:
  unsigned long _next     = 0;
  unsigned long _interval = 0;
  bool _active            = false;
};

class OneShotTimer {
public:
  void start(unsigned long interval);
  bool elapsed();
  bool running() const;
  void stop();

private:
  unsigned long _start        = 0;
  unsigned long _interval     = 0;
  bool _running               = false;
  bool _everStarted           = false;
};


// ------------------------------------
// Calendar
// ------------------------------------
class Calendar {
public:
    struct tm timeinfo;
    bool isWorkingDay();
    bool isWorkingTime();
    void printCurrentTime();
    void updateTime();
private:
    int hours = 0;
    int minutes = 0;
};


// ------------------------------------
// Sequenza Step
// ------------------------------------
class SequenceStep {
public:
  uint32_t color;
  uint16_t duration;

  SequenceStep(uint32_t colorHex, uint16_t durationMs)
    : color(colorHex), duration(durationMs) {}

  SequenceStep(uint8_t r, uint8_t g, uint8_t b, uint16_t durationMs)
    : SequenceStep(((uint32_t)r << 16) | ((uint32_t)g << 8) | b, durationMs) {}
};

// ------------------------------------
// NP_Led
// ------------------------------------
class NP_Led {
public:
  NP_Led(uint16_t numPix, uint8_t pin);
  void begin();

  // Sequenza classica
  void start(const SequenceStep* seq, uint8_t len);
  // Modalità continua: riceve raw hex 0xRRGGBB
  void startContinuous(uint32_t colorHex);

  // Imposta livello di dimming per la modalità continua (0-255)
  void setContinuousBrightness(uint8_t b);
  // Imposta brightness globale per le sequenze
  void setSequenceBrightness(uint8_t b);

  bool update();
  void setBrightness(uint8_t brightness);
  void show();

private:
  Adafruit_NeoPixel strip;
  uint16_t        numPixels;
  const SequenceStep* sequence;
  uint8_t         sequenceLen;

  bool            active;             // true durante esecuzione sequenza
  uint8_t         currentStep;
  uint32_t        lastChange;

  uint8_t         continuousBrightness; // livello di dimming continuo
  uint8_t         sequenceBrightness;   // brightness per sequenze
  uint32_t        continuousHex;        // raw hex colore continuo
};


// ------------------------------------
// Log
// ------------------------------------
enum WindowAction {
  WINDOW_MANUAL_OPEN,
  WINDOW_MANUAL_CLOSE,
  WINDOW_AUTO_OPEN,
  WINDOW_AUTO_CLOSE,
  WINDOW_RAIN_CLOSE,
  WINDOW_SENSOR_ERROR_CLOSE,
  WINDOW_SEND_ALL_OPEN,
  WINDOW_SEND_ALL_CLOSE,
  WINDOW_ALL_OPEN,
  WINDOW_ALL_CLOSE,
  WINDOW_UNKNOWN
};


// ===== VARIABILI GLOBALI PER IL TRACKING =====
extern WindowAction lastWindowAction;


// ===== DICHIARAZIONI DELLE FUNZIONI =====
void initLogging();
void logWindowStateChange(bool newState, WindowAction action);
void logTFavorableChange(bool newValue, int tempIn, int tempOut);
void setWindowAction(WindowAction action);