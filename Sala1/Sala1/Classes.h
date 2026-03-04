//Classes.h
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


// ===============================
// FREQUENCY SENSOR CLASS
// ===============================
class FrequencySensor {
private:
  volatile unsigned long pulseCount;
  volatile unsigned long lastPulseTime;
  int sensorPin;
  
public:
  FrequencySensor(int pin);
  void begin();
  void startMeasurement();
  unsigned long getFrequency(unsigned long measurementInterval);
  static void IRAM_ATTR pulseISR();
  
  static FrequencySensor* instance;
};

// ===============================
// RAIN DETECTOR CLASS
// ===============================
class RainDetector {
private:
  float referenceCapacitance;
  float originalReferenceCapacitance;
  float previousCalibrationValue;
  float currentCapacitance;
  float currentRainValue;
  bool isWet;
  bool errorReported;
  bool calibrating;
  bool recursiveCalibrationActive;
  unsigned long recursiveStartTime;
  
  // NUOVO: Gestione stato di errore
  bool sensorErrorState;           // Indica se il sensore è in errore
  bool firstErrorOccurred;         // Flag per tracciare la prima occorrenza dell'errore
  
public:
  RainDetector();
  void updateFrequency(unsigned long freq);
  void startCalibration();
  float getRainValue();
  bool getIsWet();
  float getCurrentCapacitance();
  
  // NUOVO: Metodi per gestione errore
  bool isSensorInError();
  bool isFirstError();
  void resetFirstErrorFlag();
  
  // Metodi per calibrazione ricorsiva
  void startRecursiveCalibration();
  bool processRecursiveCalibration();
  void stopRecursiveCalibration();
  
  void printDebugInfo();
};

// ===============================
// MAIN RAIN SENSOR CLASS
// ===============================
class RainSensor {
private:
  FrequencySensor* sensor;
  RetriggerableTimer measurementTimer, newCalTimer;
  
public:
  RainDetector* detector;

  RainSensor();
  ~RainSensor();
  void begin();
  void update();
  void calibrate();  // Metodo pubblico per calibrazione
  bool newCal = false;
};

// ------------------------------------
// LightSensor
// ------------------------------------
class LightSensor {
public:
    LightSensor();
    void update();
    void init();
    float light;
    bool night = false;
    
private:
    RetriggerableTimer lightUpdateDelay;
    float alfa = 0.5;
    float getLight();
};


// ------------------------------------
// NTC
// ------------------------------------
class NTC_Sensor {
private:
    int _pin;                     
    unsigned long lastReadTime;
    float temp_temperature;

    RetriggerableTimer ntcUpdateDelay;

    float getAverageADC();
    float calculateResistance();
    float readTemperature();

public:
    //Costruttore
    NTC_Sensor();

    void init();
    void update();
    float getTemperature();

    int temperature;
};


// ------------------------------------
// DHT
// ------------------------------------
class DHT_Sensor {
public:
  DHT_Sensor();
  void update();

  int temperature;

  // Constants to represent error codes.
  static const int ERROR_CHECKSUM = 254;    // Error code indicating checksum mismatch.
  static const int ERROR_TIMEOUT = 253;     // Error code indicating a timeout occurred during reading.
  static const int TIMEOUT_DURATION = 1000; // Duration (in milliseconds) to wait before timing out.
  static String getErrorString(int errorCode);

private:
  RetriggerableTimer dhtUpdateDelay;

  int _pin;  
  int temp_temperature;
  int readRawData(byte data[5]);
  byte readByte();
  void startSignal();
  int readTemperature();
};


// ------------------------------------
// Heather
// ------------------------------------
class Heather {
public:
    Heather();
    void on();
    void on_duty_cycle();
    void onPWM();
    void off();
private:
    bool isFirstCall = true;
    RetriggerableTimer heaterOffDelay;
    RetriggerableTimer heaterOnDelay;
    RetriggerableTimer heaterDurationDelay;
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
// ===== ENUM PER IDENTIFICARE LE CAUSE DEI CAMBIAMENTI =====
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

enum LightAction {
  LIGHT_AUTO_ON,
  LIGHT_AUTO_OFF,
  LIGHT_MANUAL_ON,
  LIGHT_MANUAL_OFF,
  LIGHT_UNKNOWN
};

enum RainAction {
  RAIN_ERROR,
  RAIN_RECOVERY,    // NUOVO: Recupero del sensore
  CALIB_START,
  CALIB_STOP,
  CALIB_TIMEOUT_STOP,
  NEW_VAL,
  RAIN_UNKNOWN
};

// ===== VARIABILI GLOBALI PER IL TRACKING =====
extern WindowAction lastWindowAction;
extern LightAction lastLightAction;
extern RainAction lastRainAction;

// ===== DICHIARAZIONI DELLE FUNZIONI =====
void initLogging();
void logWindowStateChange(bool newState, WindowAction action);
void logLightStateChange(bool newState, LightAction action);
void logRainStateChange(float newVal, RainAction action);
void logTFavorableChange(bool newValue, int tempIn, int tempOut);
void setWindowAction(WindowAction action);
void setLightAction(LightAction action);
void setRainAction(RainAction action);