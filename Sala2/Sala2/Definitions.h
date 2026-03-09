// Definitions.h — Sala2 (Nano ESP32 — ESP-NOW)
#pragma once

// ===================== PIN =====================
const int  BUTTON1_PIN =  4;
const int  BUTTON2_PIN =  5;
const int  CLOSE_W_PIN =  9;
const int  OPEN_W_PIN  =  6;
const byte LED_PIN     = 10;
const byte LED_COUNT   = 16;

// ===================== SYSTEM STATE =====================
enum SysState : uint8_t {
  SYS_BOOT,
  SYS_RUNNING
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

// ===================== TIMERS =====================
const unsigned long SYNC_FAST_INTERVAL    = 10000;
const unsigned long SYNC_SLOW_INTERVAL    = 3600000UL;
const uint8_t       SYNC_FAIL_MAX         = 6;

const unsigned long WINDOW_MOVE_MS        = 2000;
const unsigned long RELAY_SWITCH_PAUSE_MS = 500;
const unsigned long SHORT_PRESS_MS        = 1500;
const unsigned long LONG_PRESS_MS         = 6000;
