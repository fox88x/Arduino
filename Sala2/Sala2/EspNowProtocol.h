// EspNowProtocol.h — Protocollo di comunicazione ESP-NOW
// Condiviso tra tutte le stanze. Cambia solo THIS_SALA_ID.
#pragma once

#include <Arduino.h>

// ===================== IDENTIFICATIVO STANZA =====================
#define THIS_SALA_ID  2
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

// ===================== COSTANTI PROTOCOLLO =====================
const unsigned long PING_INTERVAL_MS     = 5000;
const unsigned long STATE_BROADCAST_MS   = 5000;
const unsigned long PEER_TIMEOUT_MS      = 15000;
const unsigned long WIFI_RECONNECT_MS    = 30000;
const uint8_t       RX_QUEUE_SIZE        = 16;
