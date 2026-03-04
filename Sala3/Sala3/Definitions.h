const int BUTTON1_PIN = 5;       // Pin del pulsante di apertura
const int BUTTON2_PIN = 4;       // Pin del pulsante di chiusura
const int CLOSE_W_PIN = 3;       // Pin dell'attuatore chiudi 
const int OPEN_W_PIN = 6;        // Pin dell'attuatore apri 
const byte LED_PIN = 9;          // NeoPixal Led
const byte LED_COUNT = 16;       // n led NeoPixal

//////////////////////////////////////////////TIMER//////////////////////////////////////////////////////
const unsigned long AUTOWINDOWS_INTERVAL =        30000;       // timer controllo movimentazione automatica finestre
const unsigned long SYNC_FAST_INTERVAL =          10000;       // 10 secondi
const unsigned long SYNC_SLOW_INTERVAL =      3600000UL;       // 1 ora

//////////////////////////////////////////WINDOWS TIMER//////////////////////////////////////////////////
const int windowMovementTimer =            2000;          // tempo di movimentazione finestre
const int shortPressTimer =                1500;          // tempo pressione breve del pulsante 
const unsigned long prolongedPressTimer =  6000;          // Pressione prolungata > 8000ms (8s)
const int allTimer =                    30*1000;          // durata comando allWindows

//////////////////////////////////////////MAIN VARIABLES/////////////////////////////////////////////////
int oldSystemState = 0, oldAll = 0;
bool rst = true;
int windowState = 0;
bool syncState = false;
bool firstSyncDone = false;
char buf[64];
bool virtualAutoW = false;  // Indica se le finestre sono state aperte automaticamente

//////////////////////////////////////PREVIUS MAIN VARIABLES/////////////////////////////////////////////
bool prev_isRaining = false;
bool prev_w_STATE = false;
bool prev_manualLight = false;
int prev_allWindows = 0;
bool firstSync = true;