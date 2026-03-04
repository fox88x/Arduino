//Classes.ino
// Static globals for ISR and counting
static uint16_t count_msw;
static uint32_t count_prev;
static volatile uint32_t count_output;
static volatile uint8_t count_ready;
static uint16_t gate_length;
static uint16_t gate_index;


// ------------------------------------
// ----- Time -----
// ------------------------------------
void getCurrentTime(struct tm &timeinfo) {
  time_t epoch = TimeService.getLocalTime();
  gmtime_r(&epoch, &timeinfo);
}


// ------------------------------------
// ---- Timers ----
// ------------------------------------
// RetriggerableTimer 
void RetriggerableTimer::start(unsigned long interval) {
  _interval = interval;
  _next     = millis() + interval;
  _active   = true;
}

bool RetriggerableTimer::elapsed() const {
  if (!_active) return false;
  return long(millis() - _next) >= 0;
}

unsigned long RetriggerableTimer::interval() const {
  return _interval;
}

void RetriggerableTimer::stop() {
  _active = false;
}

// OneShotTimer
void OneShotTimer::start(unsigned long interval) {
  _interval    = interval;
  _start       = millis();
  _running     = true;
  _everStarted = true;
}

bool OneShotTimer::elapsed() {
  if (!_everStarted || !_running) return false;
  if (long(millis() - _start) >= long(_interval)) {
    _running = false;
    return true;
  }
  return false;
}

bool OneShotTimer::running() const {
  return _running;
}

void OneShotTimer::stop() {
  _running = false;
}


// ------------------------------------
// ---- RainSensor ----
// ------------------------------------
FrequencySensor* FrequencySensor::instance = nullptr;

FrequencySensor::FrequencySensor(int pin) : sensorPin(pin), pulseCount(0), lastPulseTime(0) {
  instance = this;
}

void FrequencySensor::begin() {
  pinMode(sensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);
}

void FrequencySensor::startMeasurement() {
  pulseCount = 0;
}

unsigned long FrequencySensor::getFrequency(unsigned long measurementInterval) {
  detachInterrupt(digitalPinToInterrupt(sensorPin));
  unsigned long finalCount = pulseCount;
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);
  
  return (finalCount * 1000UL) / measurementInterval;
}

void IRAM_ATTR FrequencySensor::pulseISR() {
  if (instance) {
    unsigned long currentTime = micros();
    if (currentTime - instance->lastPulseTime > MIN_PULSE_INTERVAL) {
      instance->pulseCount++;
      instance->lastPulseTime = currentTime;
    }
  }
}

// RAIN DETECTOR IMPLEMENTATION
RainDetector::RainDetector() : referenceCapacitance(100.0), 
                               originalReferenceCapacitance(100.0),
                               previousCalibrationValue(100.0),
                               currentCapacitance(0), 
                               currentRainValue(0), 
                               isWet(false), 
                               errorReported(false), 
                               calibrating(false), 
                               recursiveCalibrationActive(false),
                               recursiveStartTime(0),
                               sensorErrorState(false),
                               firstErrorOccurred(false) {}

void RainDetector::updateFrequency(unsigned long freq) {
  if (freq > 0) {
    // Sensore funzionante - reset stato errore
    if (sensorErrorState) {
      sensorErrorState = false;
      firstErrorOccurred = false;
      setRainAction(RAIN_RECOVERY);
      logRainStateChange(currentCapacitance, RAIN_RECOVERY);
    }
    
    currentCapacitance = (float(REFERENCE_FREQUENCY) * REFERENCE_CAPACITANCE) / float(freq);
    
    if (calibrating) {
      referenceCapacitance = currentCapacitance;
      calibrating = false;
      setRainAction(CALIB_STOP);
      logRainStateChange(referenceCapacitance, CALIB_STOP);
    }
    
    // Calcola percentuale bagnato
    currentRainValue = ((currentCapacitance - referenceCapacitance) / 3000.0) * 100.0;
    
    // Limita tra 0 e 100%
    if (currentRainValue < 0) currentRainValue = 0;
    if (currentRainValue > 100) currentRainValue = 100;
    
    // Determina stato bagnato normale
    isWet = (currentRainValue > WET_THRESHOLD_PERCENT);
    
    errorReported = false;
  } 
  else {
    // ERRORE SENSORE
    if (!sensorErrorState) {
      // Prima volta che si verifica l'errore
      sensorErrorState = true;
      firstErrorOccurred = true;
      setRainAction(RAIN_ERROR);
      logRainStateChange(0.0, RAIN_ERROR);
    }
    
    // In caso di errore, forza sempre stato bagnato
    isWet = true;
    currentRainValue = 100.0; // Valore massimo per indicare errore
    errorReported = true;
  }
}

bool RainDetector::isSensorInError() {
  return sensorErrorState;
}

bool RainDetector::isFirstError() {
  return firstErrorOccurred;
}

void RainDetector::resetFirstErrorFlag() {
  firstErrorOccurred = false;
}

void RainDetector::startCalibration() {
  calibrating = true;
  setRainAction(CALIB_START);
  logRainStateChange(referenceCapacitance, CALIB_START);
}

void RainDetector::startRecursiveCalibration() {
  recursiveCalibrationActive = true;
  recursiveStartTime = millis();
  previousCalibrationValue = referenceCapacitance;
}

bool RainDetector::processRecursiveCalibration() {
  // Controlla timeout di 8 ore
  if (millis() - recursiveStartTime > RECURSIVE_CALIBRATION_TIMEOUT) {
    setRainAction(CALIB_TIMEOUT_STOP);
    logRainStateChange(referenceCapacitance, CALIB_TIMEOUT_STOP);
    stopRecursiveCalibration();
    return false;
  }
  
  float newValue = currentCapacitance;
  
  // Se il nuovo valore è maggiore del precedente, il sensore non si sta asciugando
  if (newValue >= previousCalibrationValue) {
    return true; // Continua ma non aggiorna il valore
  }
  
  // Se il nuovo valore è sceso sotto il valore originale, ripristina e ferma
  if (newValue <= originalReferenceCapacitance) {
    setRainAction(CALIB_STOP);
    logRainStateChange(originalReferenceCapacitance, CALIB_STOP);
    referenceCapacitance = originalReferenceCapacitance;
    stopRecursiveCalibration();
    return false;
  }
  
  // Il nuovo valore è minore del precedente e maggiore dell'originale - aggiorna
  previousCalibrationValue = referenceCapacitance;
  referenceCapacitance = newValue;
  setRainAction(NEW_VAL);
  logRainStateChange(referenceCapacitance, NEW_VAL);
  
  return true; // Continua la calibrazione
}

void RainDetector::stopRecursiveCalibration() {
  recursiveCalibrationActive = false;
  //Serial.println("[ Rain ] -- Calibrazione ricorsiva terminata");
}

float RainDetector::getRainValue() {
  return currentRainValue;
}

bool RainDetector::getIsWet() {
  return isWet;
}

// DEBUG: metodi per debug
float RainDetector::getCurrentCapacitance() {
  return currentCapacitance;
}

void RainDetector::printDebugInfo() {
    Serial.print("[ Rain  ] -- C: ");
    Serial.print(currentCapacitance, 1);
    Serial.print(" pF, Rain: ");
    Serial.print(currentRainValue, 1);
    Serial.print("%, Wet: ");
    Serial.print(isWet ? "YES" : "NO");
    if (recursiveCalibrationActive) {
      Serial.print(", RecCal: ACTIVE");
    }
    Serial.println();
}

// MAIN RAIN SENSOR IMPLEMENTATION
RainSensor::RainSensor() {
  sensor = new FrequencySensor(RAIN_SENSOR_PIN);
  detector = new RainDetector();
}

RainSensor::~RainSensor() {
  delete sensor;
  delete detector;
}

void RainSensor::begin() {
  sensor->begin();
  sensor->startMeasurement();
  measurementTimer.start(RAIN_CHECK_INTERVAL);
}

void RainSensor::update() {
  if (measurementTimer.elapsed()) {
    // Gestione calibrazione ricorsiva
    if(newCalTimer.elapsed() && newCal) {
      if (!detector->processRecursiveCalibration()) {
        newCal = false; // Interrompe il ciclo se processRecursiveCalibration ritorna false
      } else {
        newCalTimer.start(CALIBRATION_INTERVAL); // Riavvia timer per prossimo ciclo
      }
    }
    
    unsigned long frequency = sensor->getFrequency(measurementTimer.interval());
    
    detector->updateFrequency(frequency);
    
    // Aggiorna le variabili globali
    rainValue = detector->getCurrentCapacitance();
    isRaining = detector->getIsWet();
    
    sensor->startMeasurement();
    measurementTimer.start(RAIN_CHECK_INTERVAL);
  }
}

void RainSensor::calibrate() {
  // Prima esegue calibrazione immediata (come originale)
  detector->startCalibration();
  
  // Poi avvia il processo ricorsivo
  detector->startRecursiveCalibration();
  newCalTimer.start(CALIBRATION_INTERVAL);
}


// ------------------------------------
// ---- LightSensor ----
// ------------------------------------
LightSensor::LightSensor() {             
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    lightUpdateDelay.start(LIGHT_CHECK_INTERVAL); 
}

float LightSensor::getLight() { 
    return ((1.0 - (analogRead(LIGHT_SENSOR_PIN) / 4095.0)) * 100.0); 
}

void LightSensor::update() { 
  if (lightUpdateDelay.elapsed()) {
    light = light*(1-alfa) + getLight()*alfa;
    lightValue = light;

    if (light < LIGHT_THRESHOLD ) {
      night = true;
    }
    else if (light > LIGHT_THRESHOLD + LIGHT_OFFSET){
      night = false;
    }
    
    lightUpdateDelay.start(LIGHT_CHECK_INTERVAL);
  }
}

void LightSensor::init() { 
    light = getLight();
}


// ------------------------------------
// ---- NTCSensor ----
// ------------------------------------
NTC_Sensor::NTC_Sensor() : _pin(NTC_SENSOR_PIN) {
  pinMode(_pin, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  temp_temperature = 0.0;
  ntcUpdateDelay.start(NTC_CHECK_INTERVAL); 
}

float NTC_Sensor::getAverageADC() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(_pin);
    delayMicroseconds(100);
  }
  return (float)sum / NUM_SAMPLES;
}
    
float NTC_Sensor::calculateResistance() {
  float adcValue = getAverageADC();
  if (adcValue <= 0 || adcValue >= 4095) return -1;
        
  float voltage = (adcValue / 4095.0) * VCC;
  return R_FIXED * voltage / (VCC - voltage);
}
    
float NTC_Sensor::readTemperature() {
  float resistance = calculateResistance();
  if (resistance <= 0) return NAN;
        
  float steinhart = resistance / R_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (T_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;  
  return steinhart;
}
    
void NTC_Sensor::update() {      
  if (ntcUpdateDelay.elapsed()) {
    temp_temperature = readTemperature();
            
    if (!isnan(temp_temperature) && temp_temperature > -50 && temp_temperature < 150) {
      temperature = int(round(temp_temperature));
      int ntc_temp=temperature;
    } 
    ntcUpdateDelay.start(NTC_CHECK_INTERVAL);
  }
}
    
float NTC_Sensor::getTemperature() {
  return temp_temperature;
}


// ------------------------------------
// ---- DHTSensor ----
// ------------------------------------
DHT_Sensor::DHT_Sensor() : _pin(DHT_SENSOR_PIN)
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
  temp_temperature = 0.0;
  dhtUpdateDelay.start(DHT_CHECK_INTERVAL); 
}

int DHT_Sensor::readRawData(byte data[5])
{
  startSignal();
  unsigned long timeout_start = millis();

  while (digitalRead(_pin) == HIGH)
  {
    if (millis() - timeout_start > TIMEOUT_DURATION)
    {
      return DHT_Sensor::ERROR_TIMEOUT;
    }
  }

  if (digitalRead(_pin) == LOW)
  {
    delayMicroseconds(80);
    if (digitalRead(_pin) == HIGH)
    {
      delayMicroseconds(80);
      for (int i = 0; i < 5; i++)
      {
        data[i] = readByte();
        if (data[i] == DHT_Sensor::ERROR_TIMEOUT)
        {
          return DHT_Sensor::ERROR_TIMEOUT;
        }
      }

      if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
      {
        return 0; // Success
      }
      else
      {
        return DHT_Sensor::ERROR_CHECKSUM;
      }
    }
  }
  return DHT_Sensor::ERROR_TIMEOUT;
}

byte DHT_Sensor::readByte()
{
  byte value = 0;

  for (int i = 0; i < 8; i++)
  {
    while (digitalRead(_pin) == LOW)
      ;
    delayMicroseconds(30);
    if (digitalRead(_pin) == HIGH)
    {
      value |= (1 << (7 - i));
    }
    while (digitalRead(_pin) == HIGH)
      ;
  }
  return value;
}

void DHT_Sensor::startSignal()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delay(18);
  digitalWrite(_pin, HIGH);
  delayMicroseconds(40);
  pinMode(_pin, INPUT);
}

int DHT_Sensor::readTemperature()
{
  byte data[5];
  int error = readRawData(data);
  if (error != 0)
  {
    return error;
  }
  return data[2];
}

String DHT_Sensor::getErrorString(int errorCode)
{
  switch (errorCode)
  {
  case DHT_Sensor::ERROR_TIMEOUT:
    return "Error 253 Reading from DHT11 timed out.";
  case DHT_Sensor::ERROR_CHECKSUM:
    return "Error 254 Checksum mismatch while reading from DHT11.";
  default:
    return "Error Unknown.";
  }
}

void DHT_Sensor::update() {      
  if (dhtUpdateDelay.elapsed()) {   

    temp_temperature = readTemperature();
            
    if (!isnan(temp_temperature) && temp_temperature > -50 && temp_temperature < 150) {
      temperature = temp_temperature;
      int dht_temp=temperature;
    }
    dhtUpdateDelay.start(DHT_CHECK_INTERVAL);
  } 
}

// ------------------------------------
// ---- Heather ----
// ------------------------------------
Heather::Heather() { 
    pinMode(HEATER_PIN, OUTPUT);
}

void Heather::on() { 
    digitalWrite(HEATER_PIN, HIGH);
}

void Heather::onPWM() { 
    analogWrite(HEATER_PIN, 50); // se PWM disponibile
}

void Heather::on_duty_cycle() {
    if (isFirstCall) {
        heaterOffDelay.start(700);
        isFirstCall = false;
    }
    if (heaterOffDelay.elapsed()) {
        on();
        heaterOnDelay.start(300);
    }
    if (heaterOnDelay.elapsed()) {
        off();
        heaterOffDelay.start(700);
    }
}

void Heather::off() { 
    digitalWrite(HEATER_PIN, LOW);
}


// ------------------------------------
// ---- Calendar ----
// ------------------------------------
void Calendar::updateTime() {
  time_t epoch = TimeService.getLocalTime();
  gmtime_r(&epoch, &timeinfo);
}

bool Calendar::isWorkingDay() {   
  if (!syncState) {
    return false;
  }
  else {
    updateTime();
    hours = timeinfo.tm_hour;
    
    switch (timeinfo.tm_wday) {
        case 0: return false; // Domenica
        case 1: return true;
        case 2: return true;
        case 3: return false;
        case 4: return true; 
        case 5: return (hours <= 15); // Venerdì fino alle 15:00
        case 6: return (hours < 14);  // Sabato fino alle 14:
        default: return false;
    }
  }
}

bool Calendar::isWorkingTime() {
  updateTime();
  if (!syncState) {
    return false;
  }
  else {
    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    if ((hours < 20 || (hours == 20 && minutes < 30)) && (hours > 8 || (hours == 8 && minutes >= 30))) {
        return true;
    }
    return false; 
  }
}

void Calendar::printCurrentTime() {
  updateTime();
  
  char buffer[26];
  snprintf(buffer, sizeof(buffer)," %02d/%02d/%04d  %02d:%02d:%02d", timeinfo.tm_mday, timeinfo.tm_mon  + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.println(buffer);
  
  Serial.print("   Giorno lavorativo: ");
  Serial.print(isWorkingDay() ? "SÌ" : "NO");
  Serial.print("  |  Orario lavorativo: ");
  Serial.println(isWorkingTime() ? "SÌ" : "NO");
}


// ------------------------------------
// NP_Led
// ------------------------------------
NP_Led::NP_Led(uint16_t numPix, uint8_t pin)
  : strip(numPix, pin, NEO_GRB + NEO_KHZ800),
    numPixels(numPix),
    sequence(nullptr),
    sequenceLen(0),
    active(false),
    currentStep(0),
    lastChange(0),
    continuousBrightness(50),  // default dimming
    sequenceBrightness(255),   // default full
    continuousHex(0) {}

void NP_Led::begin() {
  strip.begin();
  strip.show();
}

void NP_Led::start(const SequenceStep* seq, uint8_t len) {
  if (active) return;
  active = true;
  sequence = seq;
  sequenceLen = len;
  currentStep = 0;
  lastChange = millis();

  // usa brightness impostata per sequenze
  strip.setBrightness(sequenceBrightness);
  strip.fill(sequence[currentStep].color, 0, numPixels);
  strip.show();
}

void NP_Led::startContinuous(uint32_t colorHex) {
  continuousHex = colorHex;
  active = false;

  // in modalità continua usiamo brightness ridotto
  strip.setBrightness(continuousBrightness);

  // unpack raw hex e mostra colore direttamente (no dimming manuale)
  uint8_t r = (continuousHex >> 16) & 0xFF;
  uint8_t g = (continuousHex >> 8) & 0xFF;
  uint8_t b = continuousHex & 0xFF;
  uint32_t encoded = strip.Color(r, g, b);

  strip.fill(encoded, 0, numPixels);
  strip.show();
}

void NP_Led::setContinuousBrightness(uint8_t b) {
  continuousBrightness = b;
  // aggiorna immediatamente se in continuo
  if (!active) {
    startContinuous(continuousHex);
  }
}

void NP_Led::setSequenceBrightness(uint8_t b) {
  sequenceBrightness = b;
}

bool NP_Led::update() {
  if (!active) return false;

  uint32_t now = millis();
  if ((uint32_t)(now - lastChange) >= sequence[currentStep].duration) {
    currentStep++;
    if (currentStep >= sequenceLen) {
      // fine sequenza: torna al continuo
      active = false;
      // ripristina stato continuo
      startContinuous(continuousHex);
      return false;
    }
    lastChange = now;
    strip.fill(sequence[currentStep].color, 0, numPixels);
    strip.show();
  }
  return true;
}

void NP_Led::setBrightness(uint8_t brightness) {
  strip.setBrightness(brightness);
}

void NP_Led::show() {
  strip.show();
}


// ------------------------------------
// Log
// ------------------------------------
// ===== DEFINIZIONE DELLE VARIABILI GLOBALI =====
WindowAction lastWindowAction = WINDOW_UNKNOWN;
LightAction lastLightAction = LIGHT_UNKNOWN;
RainAction lastRainAction = RAIN_UNKNOWN;

// ===== IMPLEMENTAZIONE DELLE FUNZIONI =====
void initLogging() {
  //Serial.println("[LOG] Sistema di logging inizializzato");
  lastWindowAction = WINDOW_UNKNOWN;
  lastLightAction = LIGHT_UNKNOWN;
  lastRainAction = RAIN_UNKNOWN;
}

void logWindowStateChange(bool newState, WindowAction action) {
  messager = String("Sala 1:\n[Windows] ");
  
  switch (action) {
    case WINDOW_MANUAL_OPEN:
      messager = messager + "Apertura MANUALE"; 
      break;
    case WINDOW_MANUAL_CLOSE:
      messager = messager + "Chiusura MANUALE";
      break;
    case WINDOW_AUTO_OPEN:
      messager = messager + "Apertura AUTOMATICA";
      break;
    case WINDOW_AUTO_CLOSE:
      messager = messager + "Chiusura AUTOMATICA";
      break;
    case WINDOW_RAIN_CLOSE:
      messager = messager + "Chiusura per PIOGGIA";
      break;
    case WINDOW_SENSOR_ERROR_CLOSE:
      messager = messager + "Chiusura di SICUREZZA (errore sensore pioggia)";
      break;
    case WINDOW_SEND_ALL_OPEN:
      messager = messager + "Apertura TOTALE (invio comando all)";
      break;
    case WINDOW_SEND_ALL_CLOSE:
      messager = messager + "Chiusura TOTALE (invio comando all)";
      break;
    case WINDOW_ALL_OPEN:
      messager = messager + "Apertura TOTALE (comando remoto)";
      break;
    case WINDOW_ALL_CLOSE:
      messager = messager + "Chiusura TOTALE (comando remoto)";
      break;
    default:
      messager = messager + "Causa SCONOSCIUTA";
      break;
  }
  
  messager = messager + "\nW_State: " + (newState ? "APERTE" : "CHIUSE") + "\n";
}

void logLightStateChange(bool newState, LightAction action) {
  messager = String("[Light] ") + (newState ? "ACCESA" : "SPENTA") + " - ";
  
 switch (action) {
    case LIGHT_AUTO_ON:
      messager = messager + "Accensione AUTOMATICA\n";
      break;
    case LIGHT_AUTO_OFF:
      messager = messager + "Spegnimento AUTOMATICO\n";
      break;
    case LIGHT_MANUAL_ON:
      messager = messager + "Accensione MANUALE\n";
      break;
    case LIGHT_MANUAL_OFF:
      messager = messager + "Spegnimento MANUALE (timeout)\n";
      break;
    default:
      messager = messager + "Causa SCONOSCIUTA\n";
      break;
  }
}

void logRainStateChange(float newVal, RainAction action) {
  messager = "[Rain Sensor]: ";
  
  switch (action) {
    case CALIB_START:
      messager = messager + "Calibrazione automatica avviata.\n";
      break;
    case NEW_VAL:
      messager = messager + "\n Nuova soglia aggiornata: " + String(newVal) + "\n";
      break;
    case CALIB_STOP:
      messager = messager + "Calibrazione automatica completata.\n";
      break;
    case CALIB_TIMEOUT_STOP:
      messager = messager + "Time-out scaduto. Calibrazione automatica arrestata.\n";
      break;
    case RAIN_ERROR:
      messager = messager + "Errore, sensore pioggia non riconosciuto\n";
      break;
    default:
      messager = messager + "Errore SCONOSCIUTO\n";
      break;
  }
}

void setWindowAction(WindowAction action) {
  lastWindowAction = action;
}

void setLightAction(LightAction action) {
  lastLightAction = action;
}

void setRainAction(RainAction action) {
  lastRainAction = action;
}