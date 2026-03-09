// EspNowProtocol.h — Protocollo di comunicazione ESP-NOW
// Condiviso tra tutte le stanze. Cambia solo THIS_SALA_ID.
#pragma once

#include <Arduino.h>

// ===================== IDENTIFICATIVO STANZA =====================
#define THIS_SALA_ID  4
#define NUM_SALAS     4

// ===================== TIPI MESSAGGIO =====================
enum MsgType : uint8_t {
  MSG_PING    = 0x01,
  MSG_PONG    = 0x02,
  MSG_STATE   = 0x10,
  MSG_COMMAND = 0x20
};

// ===================== TIPI COMANDO =====================
enum CmdType : uint8_t {
  CMD_NONE         = 0,
  CMD_ALL_OPEN     = 1,
  CMD_ALL_CLOSE    = 2,
  CMD_RESET        = 3,
  CMD_MANUAL_LIGHT = 4
};

// ===================== STATO PEER =====================
enum PeerStatus : uint8_t {
  PEER_UNKNOWN,
  PEER_ONLINE,
  PEER_OFFLINE
};

// ===================== MESSAGGI (packed, no padding) =====================

struct __attribute__((packed)) MsgHeader {
  uint8_t  type;
  uint8_t  salaId;
  uint32_t timestamp;
};

struct __attribute__((packed)) PingPongMsg {
  MsgHeader hdr;
  uint8_t   targetSala;
};

struct __attribute__((packed)) StateMsg {
  MsgHeader hdr;
  int8_t    winPos;
  uint8_t   sysState;
  bool      isRaining;
  bool      lightState;
  bool      manualLight;
  int16_t   rainValue;
  int16_t   lightValue;
  int16_t   temperature;
};

struct __attribute__((packed)) CommandMsg {
  MsgHeader hdr;
  uint8_t   targetSala;
  uint8_t   cmdType;
  int32_t   cmdValue;
};

// ===================== INFO PEER =====================
struct PeerInfo {
  uint8_t    salaId;
  PeerStatus status;
  uint32_t   lastSeen;
  StateMsg   lastState;
  bool       stateValid;
};

// ===================== MESSAGGIO RICEVUTO (coda FreeRTOS) =====================
struct ReceivedMsg {
  uint8_t mac[6];
  uint8_t data[250];
  uint8_t len;
};

// ===================== COMANDO GLOBALE PENDENTE =====================
struct PendingGlobalCmd {
  bool     active;
  uint8_t  cmdType;
  int8_t   expectedPos;
  uint32_t startTime;
  uint32_t lastRetry;
  bool     confirmed[NUM_SALAS];
};

// ===================== FRAM MB85RC256V =====================
const uint8_t  FRAM_I2C_ADDR   = 0x50;
const uint16_t FRAM_STATE_ADDR = 0x0000;
const uint8_t  FRAM_MAGIC      = 0xA5;

struct __attribute__((packed)) SavedState {
  uint8_t magic;
  int8_t  winPos;
  bool    manualLight;
  uint8_t checksum;
};

// ===================== COSTANTI PROTOCOLLO =====================
const unsigned long PING_INTERVAL_MS       = 5000;
const unsigned long STATE_BROADCAST_MS     = 5000;
const unsigned long PEER_TIMEOUT_MS        = 15000;
const unsigned long WIFI_RECONNECT_MS      = 30000;
const uint8_t       RX_QUEUE_SIZE          = 16;

// Retry comandi globali
const unsigned long CMD_RETRY_INTERVAL_MS  = 30000;
const unsigned long CMD_RETRY_TIMEOUT_MS   = 5UL * 60 * 1000;

// Chiusura sicurezza offline
const unsigned long OFFLINE_SAFETY_MS      = 5UL * 60 * 1000;
