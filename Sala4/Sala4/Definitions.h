// Definitions.h — Sala4 (Nano 33 IoT / SAMD21)
#pragma once

// ===================== PIN =====================
const int  BUTTON1_PIN =  4;       // Pulsante apertura
const int  BUTTON2_PIN =  5;       // Pulsante chiusura
const int  CLOSE_W_PIN =  6;       // Attuatore chiudi
const int  OPEN_W_PIN  =  9;       // Attuatore apri
const byte LED_PIN     = 10;       // NeoPixel
const byte LED_COUNT   = 16;       // Numero LED

// ===================== SYSTEM STATE =====================
enum SysState : uint8_t {
  SYS_BOOT,            // Hardware inizializzato, in attesa del cloud
  SYS_RUNNING,         // Cloud connesso e sincronizzato
  SYS_DISCONNECTED     // Connessione cloud persa
};

// ===================== WINDOW STATE MACHINE =====================
enum WinState : uint8_t {
  WIN_IDLE,            // Nessun relay attivo
  WIN_RELAY_PAUSE,     // Pausa 500ms dopo disattivazione relay opposto
  WIN_MOVING           // Relay attivo, finestra in movimento
};

enum WinPos : int8_t {
  WIN_CLOSED  = -1,
  WIN_TRANSIT =  0,
  WIN_OPEN    =  1
};

// ===================== TIMERS =====================
// System
const unsigned long DISCONNECT_RESET_MS     = 5UL * 60 * 1000;  // Reset dopo 5 min disconnesso

// Time sync
const unsigned long SYNC_FAST_INTERVAL      = 10000;             // 10 secondi
const unsigned long SYNC_SLOW_INTERVAL      = 3600000UL;         // 1 ora
const uint8_t       SYNC_FAIL_MAX           = 6;

// Windows
const unsigned long WINDOW_MOVE_MS          = 18000;             // Durata movimentazione finestre
const unsigned long RELAY_SWITCH_PAUSE_MS   = 500;               // Pausa tra relay opposti
const unsigned long SHORT_PRESS_MS          = 1500;              // Soglia pressione breve
const unsigned long LONG_PRESS_MS           = 6000;              // Soglia pressione lunga
const unsigned long ALL_CMD_TIMEOUT_MS      = 30000UL;           // Timeout comando allWindows
