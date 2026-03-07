// Classes.h — Sala1
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
  bool sensorErrorState;
  bool firstErrorOccurred;

public:
  RainDetector();
  void updateFrequency(unsigned long freq);
  void startCalibration();
  float getRainValue();
  bool getIsWet();
  float getCurrentCapacitance();

  bool isSensorInError();
  bool isFirstError();
  void resetFirstErrorFlag();

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
  void calibrate();
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

  static const int ERROR_CHECKSUM = 254;
  static const int ERROR_TIMEOUT = 253;
  static const int TIMEOUT_DURATION = 1000;
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
// SequenceStep
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

  void start(const SequenceStep* seq, uint8_t len);
  void startContinuous(uint32_t colorHex);

  void setContinuousBrightness(uint8_t b);
  void setSequenceBrightness(uint8_t b);

  bool update();
  void setBrightness(uint8_t brightness);
  void show();

private:
  Adafruit_NeoPixel strip;
  uint16_t        numPixels;
  const SequenceStep* sequence;
  uint8_t         sequenceLen;

  bool            active;
  uint8_t         currentStep;
  uint32_t        lastChange;

  uint8_t         continuousBrightness;
  uint8_t         sequenceBrightness;
  uint32_t        continuousHex;
};

// ------------------------------------
// Log — Action Tracking
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

enum LightAction {
  LIGHT_AUTO_ON,
  LIGHT_AUTO_OFF,
  LIGHT_MANUAL_ON,
  LIGHT_MANUAL_OFF,
  LIGHT_UNKNOWN
};

enum RainAction {
  RAIN_ERROR,
  RAIN_RECOVERY,
  CALIB_START,
  CALIB_STOP,
  CALIB_TIMEOUT_STOP,
  NEW_VAL,
  RAIN_UNKNOWN
};

extern WindowAction lastWindowAction;
extern LightAction lastLightAction;
extern RainAction lastRainAction;

void initLogging();
void logWindowStateChange(bool newState, WindowAction action);
void logLightStateChange(bool newState, LightAction action);
void logRainStateChange(float newVal, RainAction action);
void setWindowAction(WindowAction action);
void setLightAction(LightAction action);
void setRainAction(RainAction action);
