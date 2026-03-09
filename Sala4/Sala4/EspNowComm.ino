// EspNowComm.ino — Layer comunicazione ESP-NOW
//
// - Inizializzazione ESP-NOW + WiFi (per NTP)
// - Invio/ricezione messaggi via coda FreeRTOS (thread-safe)
// - Gestione peer (ping/pong, online/offline)
// - Broadcast stato periodico
// - Comando globale con retry (30s per 5min)
// - Sicurezza offline (chiusura finestre dopo 5min senza peer)

#include <esp_now.h>
#include <esp_wifi.h>

// ===================== VARIABILI MODULO =====================
static QueueHandle_t rxQueue          = NULL;
PeerInfo peers[NUM_SALAS];
static const uint8_t broadcastMac[]   = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned long lastPingTime     = 0;
static unsigned long lastBroadcastTime = 0;
static unsigned long lastWifiReconnect = 0;
static bool wifiConnected             = false;
static bool stateChanged              = false;

// ===================== COMANDO GLOBALE PENDENTE =====================
PendingGlobalCmd pendingCmd = {};
static bool offlineSafetyDone = false;

// ===================== CALLBACK RICEZIONE =====================
#if ESP_IDF_VERSION_MAJOR >= 5
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
#else
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len < (int)sizeof(MsgHeader) || len > 250) return;

  ReceivedMsg msg;
  memcpy(msg.mac, mac, 6);
  memcpy(msg.data, data, len);
  msg.len = (uint8_t)len;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(rxQueue, &msg, &xHigherPriorityTaskWoken);
}

// ===================== CALLBACK INVIO =====================
void onDataSend(const uint8_t *mac, esp_now_send_status_t status) {
}

// ===================== INIT ESP-NOW =====================
bool initEspNow() {
  rxQueue = xQueueCreate(RX_QUEUE_SIZE, sizeof(ReceivedMsg));
  if (!rxQueue) {
    Serial.println(F("[ESP-NOW] Errore creazione coda"));
    return false;
  }

  for (int i = 0; i < NUM_SALAS; i++) {
    peers[i].salaId    = i + 1;
    peers[i].status    = (i + 1 == THIS_SALA_ID) ? PEER_ONLINE : PEER_UNKNOWN;
    peers[i].lastSeen  = 0;
    peers[i].stateValid = false;
    memset(&peers[i].lastState, 0, sizeof(StateMsg));
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[ESP-NOW] Errore inizializzazione"));
    return false;
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSend);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println(F("[ESP-NOW] Errore registrazione broadcast peer"));
    return false;
  }

  Serial.print(F("[ESP-NOW] Inizializzato. MAC: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("[ESP-NOW] Canale: "));
  Serial.println(WiFi.channel());

  return true;
}

// ===================== INVIO MESSAGGI =====================
static bool espNowSend(const uint8_t *data, uint8_t len) {
  return esp_now_send(broadcastMac, data, len) == ESP_OK;
}

void sendPing() {
  PingPongMsg msg;
  msg.hdr.type      = MSG_PING;
  msg.hdr.salaId    = THIS_SALA_ID;
  msg.hdr.timestamp = millis();
  msg.targetSala    = 0;
  espNowSend((uint8_t*)&msg, sizeof(msg));
}

void sendPong(uint8_t toSala) {
  PingPongMsg msg;
  msg.hdr.type      = MSG_PONG;
  msg.hdr.salaId    = THIS_SALA_ID;
  msg.hdr.timestamp = millis();
  msg.targetSala    = toSala;
  espNowSend((uint8_t*)&msg, sizeof(msg));
}

void sendState() {
  StateMsg msg;
  msg.hdr.type      = MSG_STATE;
  msg.hdr.salaId    = THIS_SALA_ID;
  msg.hdr.timestamp = millis();
  msg.winPos        = (int8_t)winPos;
  msg.sysState      = (uint8_t)sysState;
  msg.isRaining     = false;
  msg.lightState    = false;
  msg.manualLight   = manualLight;
  msg.rainValue     = 0;
  msg.lightValue    = 0;
  msg.temperature   = 0;
  espNowSend((uint8_t*)&msg, sizeof(msg));
}

void sendCommand(CmdType cmdType, uint8_t target, int32_t value) {
  CommandMsg msg;
  msg.hdr.type      = MSG_COMMAND;
  msg.hdr.salaId    = THIS_SALA_ID;
  msg.hdr.timestamp = millis();
  msg.targetSala    = target;
  msg.cmdType       = cmdType;
  msg.cmdValue      = value;
  espNowSend((uint8_t*)&msg, sizeof(msg));
}

void notifyStateChanged() {
  stateChanged = true;
}

// ===================== COMANDO GLOBALE CON RETRY =====================

// Posizione attesa in base al tipo di comando
static int8_t expectedPosForCmd(uint8_t cmdType) {
  if (cmdType == CMD_ALL_OPEN) return (int8_t)WIN_OPEN;
  if (cmdType == CMD_ALL_CLOSE) return (int8_t)WIN_CLOSED;
  return 0;
}

// Invia comando globale e inizia tracking conferme
void sendGlobalCommand(CmdType cmdType) {
  sendCommand(cmdType, 0, 0);

  pendingCmd.active      = true;
  pendingCmd.cmdType     = cmdType;
  pendingCmd.expectedPos = expectedPosForCmd(cmdType);
  pendingCmd.startTime   = millis();
  pendingCmd.lastRetry   = millis();

  // Self è già confermato, gli altri no
  for (int i = 0; i < NUM_SALAS; i++) {
    pendingCmd.confirmed[i] = (peers[i].salaId == THIS_SALA_ID);
  }

  Serial.print(F("[CMD] Comando globale inviato: "));
  Serial.println(cmdType == CMD_ALL_OPEN ? F("APERTURA") : F("CHIUSURA"));
}

// Controlla se un peer ha confermato il comando (chiamata da handleState)
static void checkPendingConfirmation(uint8_t salaId, int8_t peerWinPos) {
  if (!pendingCmd.active) return;
  if (salaId < 1 || salaId > NUM_SALAS) return;

  if (peerWinPos == pendingCmd.expectedPos) {
    if (!pendingCmd.confirmed[salaId - 1]) {
      pendingCmd.confirmed[salaId - 1] = true;
      char buf[40];
      snprintf(buf, sizeof(buf), "[CMD] Sala%d ha confermato", salaId);
      Serial.println(buf);
    }
  }
}

// Verifica se tutti hanno confermato
static bool allConfirmed() {
  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;
    // Solo peer che sono (o erano) online devono confermare
    if (!pendingCmd.confirmed[i] && peers[i].status != PEER_UNKNOWN) return false;
  }
  return true;
}

// Processa retry comando pendente — chiamare nel loop
void processPendingCommand() {
  if (!pendingCmd.active) return;

  unsigned long now = millis();

  // Timeout 5 minuti → abbandona
  if (now - pendingCmd.startTime >= CMD_RETRY_TIMEOUT_MS) {
    Serial.println(F("[CMD] Timeout comando globale (5min)"));
    pendingCmd.active = false;
    return;
  }

  // Tutti confermati → successo
  if (allConfirmed()) {
    Serial.println(F("[CMD] Tutti i peer hanno confermato"));
    pendingCmd.active = false;
    return;
  }

  // Retry ogni 30s ai peer non confermati
  if (now - pendingCmd.lastRetry >= CMD_RETRY_INTERVAL_MS) {
    pendingCmd.lastRetry = now;
    for (int i = 0; i < NUM_SALAS; i++) {
      if (peers[i].salaId == THIS_SALA_ID) continue;
      if (!pendingCmd.confirmed[i] && peers[i].status == PEER_ONLINE) {
        sendCommand((CmdType)pendingCmd.cmdType, peers[i].salaId, 0);
        char buf[40];
        snprintf(buf, sizeof(buf), "[CMD] Retry a Sala%d", peers[i].salaId);
        Serial.println(buf);
      }
    }
  }
}

// Query per LED status
bool hasPendingCommands() {
  return pendingCmd.active;
}

// ===================== SICUREZZA OFFLINE =====================
// Se nessun peer visibile per 5 minuti → chiudi finestre una volta

void checkOfflineSafety() {
  if (getOnlinePeerCount() > 0) {
    offlineSafetyDone = false;  // Reset quando torna online
    return;
  }
  if (offlineSafetyDone) return;
  if (!isWindowIdle()) return;

  // Trova il timestamp più recente tra tutti i peer
  unsigned long lastAnyPeer = 0;
  bool anyEverSeen = false;
  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;
    if (peers[i].lastSeen > 0) {
      anyEverSeen = true;
      if (peers[i].lastSeen > lastAnyPeer) lastAnyPeer = peers[i].lastSeen;
    }
  }

  // Non attivare se non abbiamo mai visto nessun peer (primo avvio)
  if (!anyEverSeen) return;

  if (millis() - lastAnyPeer >= OFFLINE_SAFETY_MS) {
    Serial.println(F("[SAFETY] Offline >5min — chiusura sicurezza"));
    if (winPos != WIN_CLOSED) {
      setWindowAction(WINDOW_RAIN_CLOSE);
      activateRelay(CLOSE_W_PIN);
    }
    offlineSafetyDone = true;
  }
}

// ===================== GESTIONE MESSAGGI RICEVUTI =====================
static void handlePing(const PingPongMsg *msg) {
  markPeerSeen(msg->hdr.salaId);
  sendPong(msg->hdr.salaId);
}

static void handlePong(const PingPongMsg *msg) {
  if (msg->targetSala == THIS_SALA_ID) {
    markPeerSeen(msg->hdr.salaId);
  }
}

static void handleState(const StateMsg *msg) {
  uint8_t id = msg->hdr.salaId;
  if (id < 1 || id > NUM_SALAS || id == THIS_SALA_ID) return;

  markPeerSeen(id);
  PeerInfo &p = peers[id - 1];
  p.lastState  = *msg;
  p.stateValid = true;

  // Verifica conferma comando pendente
  checkPendingConfirmation(id, msg->winPos);
}

static void handleCommand(const CommandMsg *msg) {
  uint8_t id = msg->hdr.salaId;
  if (id == THIS_SALA_ID) return;
  markPeerSeen(id);

  if (msg->targetSala != 0 && msg->targetSala != THIS_SALA_ID) return;

  char logBuf[48];
  switch ((CmdType)msg->cmdType) {
    case CMD_ALL_OPEN:
      snprintf(logBuf, sizeof(logBuf), "[CMD] All-open da Sala%d", id);
      Serial.println(logBuf);
      if (winPos != WIN_OPEN && isWindowIdle()) {
        setWindowAction(WINDOW_ALL_OPEN);
        activateRelay(OPEN_W_PIN);
      }
      break;

    case CMD_ALL_CLOSE:
      snprintf(logBuf, sizeof(logBuf), "[CMD] All-close da Sala%d", id);
      Serial.println(logBuf);
      if (winPos != WIN_CLOSED && isWindowIdle()) {
        setWindowAction(WINDOW_ALL_CLOSE);
        activateRelay(CLOSE_W_PIN);
      }
      break;

    case CMD_RESET:
      snprintf(logBuf, sizeof(logBuf), "[CMD] Reset da Sala%d", id);
      Serial.println(logBuf);
      delay(100);
      ESP.restart();
      break;

    case CMD_MANUAL_LIGHT:
      manualLight = !manualLight;
      saveState();
      notifyStateChanged();
      break;

    default:
      break;
  }
}

// ===================== PEER MANAGEMENT =====================
void markPeerSeen(uint8_t salaId) {
  if (salaId < 1 || salaId > NUM_SALAS || salaId == THIS_SALA_ID) return;

  PeerInfo &p = peers[salaId - 1];
  bool wasOffline = (p.status != PEER_ONLINE);
  p.status   = PEER_ONLINE;
  p.lastSeen = millis();

  if (wasOffline) {
    char buf[32];
    snprintf(buf, sizeof(buf), "[PEER] Sala%d ONLINE", salaId);
    Serial.println(buf);

    // Peer riconnesso: invia comando pendente se non confermato
    if (pendingCmd.active && !pendingCmd.confirmed[salaId - 1]) {
      sendCommand((CmdType)pendingCmd.cmdType, salaId, 0);
      snprintf(buf, sizeof(buf), "[CMD] Reinvio a Sala%d", salaId);
      Serial.println(buf);
    }
  }
}

static void updatePeers() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;

    if (peers[i].status == PEER_ONLINE &&
        now - peers[i].lastSeen > PEER_TIMEOUT_MS) {
      peers[i].status = PEER_OFFLINE;
      char buf[32];
      snprintf(buf, sizeof(buf), "[PEER] Sala%d OFFLINE", peers[i].salaId);
      Serial.println(buf);
    }
  }
}

uint8_t getOnlinePeerCount() {
  uint8_t count = 0;
  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;
    if (peers[i].status == PEER_ONLINE) count++;
  }
  return count;
}

PeerStatus getPeerStatus(uint8_t salaId) {
  if (salaId < 1 || salaId > NUM_SALAS) return PEER_UNKNOWN;
  return peers[salaId - 1].status;
}

const StateMsg* getPeerState(uint8_t salaId) {
  if (salaId < 1 || salaId > NUM_SALAS) return nullptr;
  PeerInfo &p = peers[salaId - 1];
  if (!p.stateValid) return nullptr;
  return &p.lastState;
}

// ===================== PROCESS — chiamare nel loop() =====================
void processEspNow() {
  // --- Processa coda messaggi ricevuti ---
  ReceivedMsg rx;
  while (xQueueReceive(rxQueue, &rx, 0) == pdTRUE) {
    if (rx.len < sizeof(MsgHeader)) continue;
    MsgHeader *hdr = (MsgHeader*)rx.data;

    if (hdr->salaId == THIS_SALA_ID) continue;

    switch ((MsgType)hdr->type) {
      case MSG_PING:
        if (rx.len >= sizeof(PingPongMsg))
          handlePing((PingPongMsg*)rx.data);
        break;
      case MSG_PONG:
        if (rx.len >= sizeof(PingPongMsg))
          handlePong((PingPongMsg*)rx.data);
        break;
      case MSG_STATE:
        if (rx.len >= sizeof(StateMsg))
          handleState((StateMsg*)rx.data);
        break;
      case MSG_COMMAND:
        if (rx.len >= sizeof(CommandMsg))
          handleCommand((CommandMsg*)rx.data);
        break;
    }
  }

  // --- Ping periodico ---
  unsigned long now = millis();
  if (now - lastPingTime >= PING_INTERVAL_MS) {
    sendPing();
    lastPingTime = now;
  }

  // --- Broadcast stato periodico (o immediato su cambio) ---
  if (stateChanged || now - lastBroadcastTime >= STATE_BROADCAST_MS) {
    sendState();
    lastBroadcastTime = now;
    stateChanged = false;
  }

  // --- Aggiorna timeout peer ---
  updatePeers();

  // --- Comando globale retry ---
  processPendingCommand();

  // --- Sicurezza offline ---
  checkOfflineSafety();
}

// ===================== WIFI MANAGEMENT (per NTP) =====================
void initWifi() {
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  Serial.print(F("[WiFi] Connessione"));

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
    delay(100);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.print(F("[WiFi] Connesso — IP: "));
    Serial.print(WiFi.localIP());
    Serial.print(F(" — Canale: "));
    Serial.println(WiFi.channel());
  } else {
    wifiConnected = false;
    Serial.println(F("[WiFi] Non connesso — solo ESP-NOW"));
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  }
}

void checkWifiReconnect() {
  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !wifiConnected) {
    wifiConnected = true;
    Serial.println(F("[WiFi] Riconnesso"));
  }
  else if (!connected && wifiConnected) {
    wifiConnected = false;
    Serial.println(F("[WiFi] Disconnesso"));
  }

  if (!connected) {
    unsigned long now = millis();
    if (now - lastWifiReconnect >= WIFI_RECONNECT_MS) {
      WiFi.begin(SECRET_SSID, SECRET_PASS);
      lastWifiReconnect = now;
    }
  }
}

bool isWifiConnected() {
  return wifiConnected;
}
