// Definitions.h — Sala1 (Nano ESP32)
#pragma once

// ===================== PIN =====================
const byte BUTTON1_PIN     = 7;
const byte BUTTON2_PIN     = 8;
const byte OPEN_W_PIN      = 2;
const byte CLOSE_W_PIN     = 3;
const byte LIGHT_ON_PIN    = 4;
const byte LIGHT_OFF_PIN   = 5;
const byte DHT_SENSOR_PIN  = A0;
const byte RAIN_SENSOR_PIN = A1;
const byte NTC_SENSOR_PIN  = A2;
const byte LIGHT_SENSOR_PIN = A3;
const byte HEATER_PIN      = 10;
const byte LED_PIN         = 9;
const byte LED_COUNT       = 16;

// ===================== SYSTEM STATE =====================
enum SysState : uint8_t {
  SYS_BOOT,
  SYS_RUNNING,
  SYS_DISCONNECTED
};

// ===================== WINDOW STATE MACHINE =====================
enum WinState : uint8_t {
  WIN_IDLE,
  WIN_RELAY_PAUSE,
  WIN_MOVING
};

enum WinPos : int8_t {
  WIN_CLOSED  = -1,
  WIN_TRANSIT =  0,
  WIN_OPEN    =  1
};

// ===================== LIGHT RELAY STATE =====================
enum LightRelayState : uint8_t {
  LRELAY_IDLE,
  LRELAY_PAUSE,
  LRELAY_PULSING
};

// ===================== TIMERS =====================
// System
const unsigned long DISCONNECT_RESET_MS     = 5UL * 60 * 1000;

// Time sync
const unsigned long SYNC_FAST_INTERVAL      = 10000;
const unsigned long SYNC_SLOW_INTERVAL      = 3600000UL;
const uint8_t       SYNC_FAIL_MAX           = 6;

// Windows
const unsigned long WINDOW_MOVE_MS          = 18000;
const unsigned long RELAY_SWITCH_PAUSE_MS   = 500;
const unsigned long SHORT_PRESS_MS          = 1500;
const unsigned long LONG_PRESS_MS           = 6000;
const unsigned long ALL_CMD_TIMEOUT_MS      = 30000UL;

// Light
const unsigned long LIGHT_RELAY_MS          = 2000;
const unsigned long LIGHT_RELAY_PAUSE_MS    = 500;
const int           LIGHT_THRESHOLD         = 10;
const int           LIGHT_OFFSET            = 5;
const unsigned long LIGHT_CHECK_INTERVAL    = 10000;
const unsigned long MANUAL_TIMER            = 60UL * 60 * 1000;

// Rain sensor
const unsigned long MIN_PULSE_INTERVAL      = 10;
const unsigned long REFERENCE_FREQUENCY     = 9227;
const float         REFERENCE_CAPACITANCE   = 100;
const float         WET_THRESHOLD_PERCENT   = 10.0;
const unsigned long RAIN_CHECK_INTERVAL     = 2000;
const unsigned long CALIBRATION_INTERVAL    = 30UL * 60 * 1000;
const unsigned long RECURSIVE_CALIBRATION_TIMEOUT = 8UL * 60 * 60 * 1000;

// Temperature sensors
const float VCC           = 3.13;
const float R_FIXED       = 10030.0;
const float R_NOMINAL     = 782.0;
const float T_NOMINAL     = 27;
const float B_COEFFICIENT = 3950.0;
const int   NUM_SAMPLES   = 10;
const unsigned long NTC_CHECK_INTERVAL = 10000;
const unsigned long DHT_CHECK_INTERVAL = 10000;

// Heater
const int SETPOINT = 24;
const int HYST     = 2;
const int HYST2    = 1;
