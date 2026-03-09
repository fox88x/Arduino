// FramStorage.ino — Persistenza stato su FRAM MB85RC256V
//
// Il modulo MB85RC256V (32KB FRAM, I2C) salva lo stato delle
// finestre e variabili dopo ogni cambiamento. Al riavvio lo stato
// viene ripristinato per riflettere la posizione reale.
//
// Collegamento I2C:
//   SDA → A4 (Nano ESP32: GPIO11)
//   SCL → A5 (Nano ESP32: GPIO12)
//   VCC → 3.3V
//   GND → GND
//   A0/A1/A2 → GND (indirizzo 0x50)

#include <Wire.h>

static bool framPresent = false;

// ===================== INIT =====================
void initFram() {
  Wire.begin();
  Wire.beginTransmission(FRAM_I2C_ADDR);
  framPresent = (Wire.endTransmission() == 0);

  if (framPresent) {
    Serial.println(F("[FRAM] MB85RC256V rilevato"));
  } else {
    Serial.println(F("[FRAM] MB85RC256V non trovato — stato non persistente"));
  }
}

// ===================== I2C RAW =====================
static void framWriteRaw(uint16_t addr, const uint8_t* data, uint8_t len) {
  if (!framPresent) return;
  Wire.beginTransmission(FRAM_I2C_ADDR);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  for (uint8_t i = 0; i < len; i++) Wire.write(data[i]);
  Wire.endTransmission();
}

static bool framReadRaw(uint16_t addr, uint8_t* data, uint8_t len) {
  if (!framPresent) return false;
  Wire.beginTransmission(FRAM_I2C_ADDR);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  Wire.endTransmission(false);
  Wire.requestFrom(FRAM_I2C_ADDR, len);
  for (uint8_t i = 0; i < len && Wire.available(); i++) data[i] = Wire.read();
  return true;
}

// ===================== CHECKSUM =====================
static uint8_t calcChecksum(const SavedState& s) {
  return s.magic ^ (uint8_t)s.winPos ^ (uint8_t)s.manualLight;
}

// ===================== SAVE / LOAD =====================
void saveState() {
  SavedState s;
  s.magic       = FRAM_MAGIC;
  s.winPos      = (int8_t)winPos;
  s.manualLight = manualLight;
  s.checksum    = calcChecksum(s);
  framWriteRaw(FRAM_STATE_ADDR, (uint8_t*)&s, sizeof(s));
}

bool loadState() {
  SavedState s;
  if (!framReadRaw(FRAM_STATE_ADDR, (uint8_t*)&s, sizeof(s))) return false;

  if (s.magic != FRAM_MAGIC) {
    Serial.println(F("[FRAM] Nessuno stato salvato (primo avvio)"));
    return false;
  }
  if (s.checksum != calcChecksum(s)) {
    Serial.println(F("[FRAM] Checksum errato — stato ignorato"));
    return false;
  }

  winPos      = (WinPos)s.winPos;
  manualLight = s.manualLight;

  char buf[48];
  snprintf(buf, sizeof(buf), "[FRAM] Stato ripristinato: win=%d ml=%d",
    (int)winPos, (int)manualLight);
  Serial.println(buf);
  return true;
}
