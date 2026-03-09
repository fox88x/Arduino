// EspNowProtocol.h — Protocollo di comunicazione ESP-NOW
// Condiviso tra tutte le stanze. Ogni stanza cambia solo THIS_SALA_ID.
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
  uint8_t  type;        // MsgType
  uint8_t  salaId;      // ID stanza mittente (1-4)
  uint32_t timestamp;   // millis() del mittente
};

// Ping/Pong — heartbeat attivo
struct __attribute__((packed)) PingPongMsg {
  MsgHeader hdr;
  uint8_t   targetSala;  // PONG: sala che ha chiesto il ping. PING: 0
};

// Stato stanza — broadcast periodico + su cambio stato
struct __attribute__((packed)) StateMsg {
  MsgHeader hdr;
  int8_t    winPos;       // WIN_CLOSED(-1), WIN_TRANSIT(0), WIN_OPEN(1)
  uint8_t   sysState;     // SysState enum
  bool      isRaining;
  bool      lightState;
  bool      manualLight;
  int16_t   rainValue;
  int16_t   lightValue;
  int16_t   temperature;
};

// Comando — broadcast o diretto a una stanza
struct __attribute__((packed)) CommandMsg {
  MsgHeader hdr;
  uint8_t   targetSala;   // 0 = broadcast a tutte
  uint8_t   cmdType;      // CmdType
  int32_t   cmdValue;     // Valore opzionale
};

// ===================== INFO PEER =====================
struct PeerInfo {
  uint8_t    salaId;
  PeerStatus status;
  uint32_t   lastSeen;     // millis() ultima ricezione
  StateMsg   lastState;    // Ultimo stato ricevuto
  bool       stateValid;   // Almeno uno stato ricevuto
};

// ===================== MESSAGGIO RICEVUTO (coda FreeRTOS) =====================
struct ReceivedMsg {
  uint8_t mac[6];
  uint8_t data[250];      // ESP-NOW max payload
  uint8_t len;
};

// ===================== COSTANTI PROTOCOLLO =====================
const unsigned long PING_INTERVAL_MS     = 5000;
const unsigned long STATE_BROADCAST_MS   = 5000;
const unsigned long PEER_TIMEOUT_MS      = 15000;
const unsigned long WIFI_RECONNECT_MS    = 30000;
const uint8_t       RX_QUEUE_SIZE        = 16;
