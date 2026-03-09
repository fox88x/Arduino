// EspNowComm.ino — Sala4 — Layer comunicazione ESP-NOW
//
// - Inizializzazione ESP-NOW + WiFi (per NTP)
// - Invio/ricezione messaggi via coda FreeRTOS (thread-safe)
// - Gestione peer (ping/pong, online/offline)
// - Broadcast stato periodico

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
static bool stateChanged              = false;  // Flag per broadcast immediato

// ===================== CALLBACK RICEZIONE =====================
// Eseguita nel WiFi task — non fare operazioni pesanti qui!
// Copia il messaggio nella coda e processa nel loop principale.
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

  // Usa FromISR per sicurezza (callback in contesto WiFi task)
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(rxQueue, &msg, &xHigherPriorityTaskWoken);
}

// ===================== CALLBACK INVIO =====================
void onDataSend(const uint8_t *mac, esp_now_send_status_t status) {
  // Per broadcast il delivery status non è significativo.
  // Utile per debug:
  // if (status != ESP_NOW_SEND_SUCCESS) Serial.println(F("[ESP-NOW] Send failed"));
}

// ===================== INIT ESP-NOW =====================
bool initEspNow() {
  // Coda ricezione FreeRTOS
  rxQueue = xQueueCreate(RX_QUEUE_SIZE, sizeof(ReceivedMsg));
  if (!rxQueue) {
    Serial.println(F("[ESP-NOW] Errore creazione coda"));
    return false;
  }

  // Init array peer
  for (int i = 0; i < NUM_SALAS; i++) {
    peers[i].salaId    = i + 1;
    peers[i].status    = (i + 1 == THIS_SALA_ID) ? PEER_ONLINE : PEER_UNKNOWN;
    peers[i].lastSeen  = 0;
    peers[i].stateValid = false;
    memset(&peers[i].lastState, 0, sizeof(StateMsg));
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[ESP-NOW] Errore inizializzazione"));
    return false;
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSend);

  // Registra peer broadcast (unico peer necessario per comunicazione broadcast)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;   // Canale corrente
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
  msg.isRaining     = false;   // Sala4 non ha sensore pioggia
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
  msg.targetSala    = target;   // 0 = broadcast a tutte
  msg.cmdType       = cmdType;
  msg.cmdValue      = value;
  espNowSend((uint8_t*)&msg, sizeof(msg));
}

// Notifica che lo stato locale è cambiato → broadcast immediato
void notifyStateChanged() {
  stateChanged = true;
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
}

static void handleCommand(const CommandMsg *msg) {
  uint8_t id = msg->hdr.salaId;
  if (id == THIS_SALA_ID) return;   // Ignora comandi propri
  markPeerSeen(id);

  // Verifica se il comando è per noi
  if (msg->targetSala != 0 && msg->targetSala != THIS_SALA_ID) return;

  char logBuf[48];
  switch ((CmdType)msg->cmdType) {
    case CMD_ALL_OPEN:
      snprintf(logBuf, sizeof(logBuf), "[CMD] All-open da Sala%d", id);
      Serial.println(logBuf);
      setWindowAction(WINDOW_ALL_OPEN);
      activateRelay(OPEN_W_PIN);
      break;

    case CMD_ALL_CLOSE:
      snprintf(logBuf, sizeof(logBuf), "[CMD] All-close da Sala%d", id);
      Serial.println(logBuf);
      setWindowAction(WINDOW_ALL_CLOSE);
      activateRelay(CLOSE_W_PIN);
      break;

    case CMD_RESET:
      snprintf(logBuf, sizeof(logBuf), "[CMD] Reset da Sala%d", id);
      Serial.println(logBuf);
      delay(100);
      ESP.restart();
      break;

    case CMD_MANUAL_LIGHT:
      manualLight = !manualLight;
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
    char msg[32];
    snprintf(msg, sizeof(msg), "[PEER] Sala%d ONLINE", salaId);
    Serial.println(msg);
    Led1.start(sequence5, numSteps5);   // Verde: peer connesso
  }
}

static void updatePeers() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_SALAS; i++) {
    if (peers[i].salaId == THIS_SALA_ID) continue;

    if (peers[i].status == PEER_ONLINE &&
        now - peers[i].lastSeen > PEER_TIMEOUT_MS) {
      peers[i].status = PEER_OFFLINE;
      char msg[32];
      snprintf(msg, sizeof(msg), "[PEER] Sala%d OFFLINE", peers[i].salaId);
      Serial.println(msg);
      Led1.start(sequence6, numSteps6);   // Rosso: peer perso
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

// Restituisce lo stato di una stanza remota (se disponibile)
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

    // Ignora messaggi propri (riflessi dal broadcast)
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
    // Imposta canale fisso per ESP-NOW se WiFi non disponibile
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
