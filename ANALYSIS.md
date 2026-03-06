# Analisi del Progetto Arduino - Sistema Domotico Multi-Sala

## Panoramica

Sistema di automazione per 4 sale (stanze) di un edificio commerciale/ufficio. Ogni sala è controllata da una scheda Arduino indipendente collegata ad Arduino IoT Cloud per il controllo remoto e la sincronizzazione temporale.

---

## Architettura Generale

```
Arduino IoT Cloud
       │
       ├── Sala 1 (Completa: finestre, luci, sensori pioggia/temperatura/luminosità)
       ├── Sala 2 (Base: solo finestre)
       ├── Sala 3 (Base: solo finestre - implementazione incompleta)
       └── Sala 4 (Base: solo finestre)
```

Tutte le sale comunicano tramite variabili cloud condivise (`allWindows`, `isRaining`) per coordinare azioni broadcast come la chiusura simultanea di tutte le finestre.

---

## Riepilogo per Sala

### Sala 1 — Sala Principale (Completa)

**Sensori:**
| Sensore | Pin | Funzione |
|---------|-----|----------|
| Luminosità | A3 | Rilevamento luce ambientale con smoothing esponenziale (α=0.5) |
| Pioggia (capacitivo) | A1 | Misurazione frequenza con ISR, calibrazione ricorsiva, rilevamento errori |
| NTC (temperatura) | A2 | Termistore con equazione Steinhart-Hart |
| DHT (umidità) | A0 | Sensore digitale umidità/temperatura (commentato) |
| Pulsante Apri | 7 | Controllo finestra aperta (INPUT_PULLUP, debounce 5ms) |
| Pulsante Chiudi | 8 | Controllo finestra chiusa (INPUT_PULLUP, debounce 5ms) |

**Attuatori:**
| Attuatore | Pin | Funzione |
|-----------|-----|----------|
| Relè Finestra Apri | 2 | Motore apertura finestra |
| Relè Finestra Chiudi | 3 | Motore chiusura finestra |
| Relè Luce ON | 4 | Accensione luci |
| Relè Luce OFF | 5 | Spegnimento luci |
| Riscaldatore | 10 | Elemento riscaldante PWM |
| NeoPixel LED | 9 | Striscia LED 16 pixel per indicazione stato |

**Variabili Cloud:** `messager`, `allWindows`, `lightValue`, `rainValue`, `isRaining`, `lightState`, `manualLight`, `w_STATE` (8 totali)

**Logica Luci Automatica:**
- Attiva durante orario lavorativo (8:30-20:30) quando è notte (luce < 10%) e giorno lavorativo
- Modalità manuale con timeout 60 minuti

**Calendario Lavorativo:**
- Lunedì-Giovedì: tutto il giorno
- Venerdì: fino alle 15:00
- Sabato: fino alle 14:00
- Domenica: non lavorativo

**Sensore Pioggia:**
- Misurazione frequenza con conteggio impulsi ISR
- Calcolo capacitanza e percentuale
- Calibrazione automatica con timeout 8 ore
- Stato errore con rilevamento recupero
- In caso di pioggia → chiusura automatica finestre

**Timer Finestra:** 18 secondi

---

### Sala 2 — Solo Finestre

**Pin:**
| Componente | Pin |
|------------|-----|
| Pulsante Apri | 4 |
| Pulsante Chiudi | 5 |
| Relè Apri | 6 |
| Relè Chiudi | 9 |
| NeoPixel LED | 10 |

**Variabili Cloud:** `messager`, `allWindows`, `isRaining`, `manualLight`, `w_STATE` (5 totali)

**Timer Finestra:** 2 secondi | **Reset:** `NVIC_SystemReset()` (keyword: "reset2")

---

### Sala 3 — Solo Finestre (Incompleta)

**Pin:**
| Componente | Pin |
|------------|-----|
| Pulsante Apri | 5 |
| Pulsante Chiudi | 4 |
| Relè Apri | 6 |
| Relè Chiudi | 3 |
| NeoPixel LED | 9 |

**Variabili Cloud:** `messager`, `allWindows`, `isRaining`, `manualLight`, `w_STATE` (5 totali)

**Timer Finestra:** 2 secondi | **Reset:** `ESP.restart()` (keyword: "reset3")

> **⚠ ATTENZIONE:** Sala 3 è **incompleta** — mancano i file `customSequences.h`, `TimeSync.ino` e `WindowsManagment.ino`.

---

### Sala 4 — Solo Finestre

**Pin:**
| Componente | Pin |
|------------|-----|
| Pulsante Apri | 4 |
| Pulsante Chiudi | 5 |
| Relè Apri | 9 |
| Relè Chiudi | 6 |
| NeoPixel LED | 10 |

**Variabili Cloud:** `messager`, `allWindows`, `isRaining`, `manualLight`, `w_STATE` (5 totali)

**Timer Finestra:** 18 secondi | **Reset:** `NVIC_SystemReset()` (keyword: "reset4")

---

## Tabella Comparativa

| Caratteristica | Sala 1 | Sala 2 | Sala 3 | Sala 4 |
|----------------|--------|--------|--------|--------|
| Controllo Luci | ✅ | ❌ | ❌ | ❌ |
| Sensore Luminosità | ✅ | ❌ | ❌ | ❌ |
| Sensore Pioggia | ✅ | ❌ | ❌ | ❌ |
| Sensori Temperatura | ✅ (NTC+DHT) | ❌ | ❌ | ❌ |
| Riscaldatore | ✅ | ❌ | ❌ | ❌ |
| Timer Finestra | 18s | 2s | 2s | 18s |
| Baud Seriale | 9600 | 19200 | 19200 | 19200 |
| Variabili Cloud | 8 | 5 | 5 | 5 |
| Reset | ESP.restart() | NVIC_SystemReset() | ESP.restart() | NVIC_SystemReset() |
| Implementazione | Completa | Completa | **Incompleta** | Completa |

---

## Logica Comune Pulsanti (Tutte le Sale)

| Pressione | Durata | Azione |
|-----------|--------|--------|
| Breve | < 1.5s | Controllo finestra individuale |
| Media | 1.5–6s | Comando broadcast a tutte le sale |
| Lunga | > 6s | Toggle impostazioni (modalità manuale luce) |

---

## Sequenze LED NeoPixel (Indicazione Stato)

| Colore | Stato |
|--------|-------|
| Verde continuo | Cloud connesso |
| Blu continuo | Cloud sincronizzato |
| Rosso continuo | Cloud disconnesso |
| Giallo+Verde lampeggiante | Luce manuale ON |
| Giallo+Rosso lampeggiante | Luce manuale OFF |
| Ciano+Verde lampeggiante | Sincronizzazione ora OK |
| Ciano+Rosso lampeggiante | Sincronizzazione ora fallita |

---

## Struttura File

```
Sala{N}/Sala{N}/
├── Sala{N}_jul13a.ino      # Sketch principale
├── Classes.h                # Definizioni classi
├── Classes.ino              # Implementazioni classi
├── Definitions.h            # Pin e costanti
├── thingProperties.h        # Proprietà cloud (auto-generato)
├── customSequences.h        # Animazioni NeoPixel
├── TimeSync.ino             # Sincronizzazione oraria
├── WindowsManagment.ino     # Gestione finestre
├── LightManagment.ino       # Gestione luci (solo Sala 1)
└── arduino_secrets.h        # Credenziali (non incluso)
```

---

## Classi Principali

| Classe | Descrizione | Presente in |
|--------|-------------|-------------|
| `RetriggerableTimer` | Timer riavviabile ad ogni ciclo | Tutte |
| `OneShotTimer` | Timer singolo per eventi una tantum | Tutte |
| `Calendar` | Calcolo giorno/ora lavorativo | Tutte |
| `NP_Led` | Controllo NeoPixel con sequenze e modalità continue | Tutte |
| `SequenceStep` | Frame animazione LED | Tutte |
| `FrequencySensor` | Conteggio impulsi per sensore pioggia | Solo Sala 1 |
| `RainDetector` | Calcolo percentuale pioggia | Solo Sala 1 |
| `RainSensor` | Gestore sensore pioggia principale | Solo Sala 1 |
| `LightSensor` | Misurazione luce ambientale | Solo Sala 1 |
| `NTC_Sensor` | Lettura temperatura via termistore | Solo Sala 1 |
| `DHT_Sensor` | Temperatura/umidità digitale | Solo Sala 1 |
| `Heater` | Controllo riscaldatore PWM | Solo Sala 1 |

---

## Problemi e Raccomandazioni

1. **Sala 3 incompleta** — Mancano 3 file essenziali (`customSequences.h`, `TimeSync.ino`, `WindowsManagment.ino`). Copiare e adattare da Sala 2 o Sala 4.

2. **Incoerenza pin tra sale** — I pulsanti e relè usano pin diversi in ogni sala. Documentare chiaramente l'hardware di ogni sala per evitare errori durante la manutenzione.

3. **Timer finestra disomogenei** — Sala 1 e 4 usano 18s, Sala 2 e 3 usano 2s. Verificare se il timer di 2s è sufficiente per il movimento completo delle finestre.

4. **Metodo reset diverso** — `ESP.restart()` (Sala 1, 3) vs `NVIC_SystemReset()` (Sala 2, 4) suggerisce schede Arduino diverse. Verificare compatibilità firmware.

5. **Sensore DHT commentato** — In Sala 1, il sensore DHT su A0 è dichiarato ma commentato nel codice. Rimuovere se non utilizzato o riattivare se necessario.

6. **Credenziali nel codice** — Le credenziali WiFi e cloud sono in `arduino_secrets.h`. Assicurarsi che questi file non vengano mai committati nel repository.
