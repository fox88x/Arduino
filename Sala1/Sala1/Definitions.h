//Definitions.h
const byte BUTTON1_PIN = 7;            // Pin del pulsante di apertura
const byte BUTTON2_PIN = 8;            // Pin del pulsante di chiusura

const byte OPEN_W_PIN = 2;             // Pin dell'attuatore apri 
const byte CLOSE_W_PIN = 3;            // Pin dell'attuatore chiudi       
const byte LIGHT_ON_PIN = 4;
const byte LIGHT_OFF_PIN = 5;

const byte DHT_SENSOR_PIN = A0;
const byte RAIN_SENSOR_PIN = A1;
const byte NTC_SENSOR_PIN = A2;  
const byte LIGHT_SENSOR_PIN = A3;
const byte HEATER_PIN = 10;

const byte LED_PIN = 9;                //NeoPixal Led
const byte LED_COUNT = 16;             //n led NeoPixal

///////////////////////////////////////////SYNC TIMER////////////////////////////////////////////////////
const unsigned long SYNC_FAST_INTERVAL =        10000;       // 10 secondi
const unsigned long SYNC_SLOW_INTERVAL =    3600000UL;       // 1 ora

//////////////////////////////////////////WINDOWS TIMER//////////////////////////////////////////////////
const int windowMovementTimer =                18000;          // tempo di movimentazione finestre
const int shortPressTimer =                     1500;          // tempo pressione breve del pulsante 
const unsigned long prolongedPressTimer =       6000;          // Pressione prolungata > 6s
const int allTimer =                         30*1000;          // durata comando allWindows
const unsigned long AUTOWINDOWS_INTERVAL =     30000;          // timer controllo movimentazione automatica finestre

///////////////////////////////////////////////LIGHT/////////////////////////////////////////////////////
const unsigned long MIN_PULSE_INTERVAL = 10;  // Filtro anti-rimbalzo (microseconds)
const int LIGHT_THRESHOLD = 10;                 //valore di thresold per la luminosità esterna
const int LIGHT_OFFSET = 5; 
const int lightRelayTimer =                        2000;       //tempo movimentazione relè luce
const int MANUAL_TIMER =                 60 * 60 * 1000;       //durata timer manuale luci
static const unsigned long LIGHT_CHECK_INTERVAL = 10000;       // timer controllo luci

////////////////////////////////////////////////RAIN/////////////////////////////////////////////////////
const unsigned long REFERENCE_FREQUENCY = 9227;
const float REFERENCE_CAPACITANCE = 100;
const float WET_THRESHOLD_PERCENT = 10.0;  // Soglia 10% per bagnato

static const unsigned long RAIN_CHECK_INTERVAL =        2000;       // timer controllo sensore pioggia
static const unsigned long CALIBRATION_INTERVAL =       30 * 60 * 1000;       //durata rutine calibrazione in caso di pioggia
static const unsigned long RECURSIVE_CALIBRATION_TIMEOUT = 8UL * 60UL * 60UL * 1000UL; // 8 ore timeout calibrazione ricorsiva

///////////////////////////////////////////////TEMP SENSOR///////////////////////////////////////////////
const float VCC = 3.13;              // Tensione reale misurata
const float R_FIXED = 10030.0;
const float R_NOMINAL = 782.0;     // Resistenza termistore calcolata dal parallelo
const float T_NOMINAL = 27;       // Temperatura ambiente misurata
const float B_COEFFICIENT = 3950.0;  // Coefficiente B per termistore ~10kΩ

const int NUM_SAMPLES = 10;

static const unsigned long NTC_CHECK_INTERVAL = 10000;
static const unsigned long DHT_CHECK_INTERVAL = 10000;

const int SETPOINT = 24;
const int HYST = 2;
const int HYST2 = 1;

//////////////////////////////////////////MAIN VARIABLES/////////////////////////////////////////////////
int oldSystemState = 0, oldAll = 0;
bool rst = true;
int windowState = 0;
bool syncState = false;
bool firstSyncDone = false;
char buf[64];

//////////////////////////////////////PREVIUS MAIN VARIABLES/////////////////////////////////////////////
bool prev_isRaining = false;
bool prev_w_STATE = false;
bool prev_manualLight = false;
int prev_allWindows = 0;
bool prev_lightState = false;
bool firstSync = true;