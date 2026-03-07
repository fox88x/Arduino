// Definitions.h — Sala3 (Nano ESP32)
#pragma once

// ===================== PIN =====================
const int  BUTTON1_PIN = 5;
const int  BUTTON2_PIN = 4;
const int  CLOSE_W_PIN = 3;
const int  OPEN_W_PIN  = 6;
const byte LED_PIN     = 9;
const byte LED_COUNT   = 16;

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

// ===================== TIMERS =====================
// System
const unsigned long DISCONNECT_RESET_MS   = 5UL * 60 * 1000;

// Time sync
const unsigned long SYNC_FAST_INTERVAL    = 10000;
const unsigned long SYNC_SLOW_INTERVAL    = 3600000UL;
const uint8_t       SYNC_FAIL_MAX         = 6;

// Windows
const unsigned long WINDOW_MOVE_MS        = 2000;
const unsigned long RELAY_SWITCH_PAUSE_MS = 500;
const unsigned long SHORT_PRESS_MS        = 1500;
const unsigned long LONG_PRESS_MS         = 6000;
const unsigned long ALL_CMD_TIMEOUT_MS    = 30000UL;
