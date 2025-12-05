// --- Konfiguration ---
const int PIN_SOIL   = A0;       // Analog-Pin für Feuchtigkeitssensor
const int PIN_SWITCH = A1;       // 3-Stufen-Drehschalter
const int PIN_PUMP   = 8;        // Digital-Pin für Relais/MOSFET

// --- Profil-Schwellwerte ---
// Bereich: 0..1023, abhängig von Sensor-Kalibrierung
const int THRESH_WET    = 430;   // feuchtigkeitsliebend
const int THRESH_NORMAL = 520;   // normal
const int THRESH_DRY    = 610;   // trockenheitsliebend

// --- Pumpenlaufzeiten je Profil ---
const unsigned long PUMP_WET_MS    = 2000;  // 2s
const unsigned long PUMP_NORMAL_MS = 3000;  // 3s
const unsigned long PUMP_DRY_MS    = 4000;  // 4s

// TODO: aktuell kurze Intervalle zum Testen
const unsigned long SAMPLE_INTERVAL_MS = 15UL * 1000UL;  // 15 Sekunden
const unsigned long DEFAULT_PUMP_TIME  = 3000UL;          // nur Fallback

// Optional: Hysterese
const int HYSTERESIS = 20;

// --- Zustandsvariablen ---
unsigned long lastSampleTs = 0;
unsigned long pumpStartTs  = 0;
bool pumpRunning           = false;

// Für geglättete Messung
const int N_SAMPLES = 10;

int readSoilAveraged() {
  long sum = 0;
  for (int i = 0; i < N_SAMPLES; ++i) {
    sum += analogRead(PIN_SOIL);
    delay(5);
  }
  return (int)(sum / N_SAMPLES);
}

// --- NEU: Funktion zur Auswertung des 3-Stufen-Schalters ---
int getWaterProfile() {
  int val = analogRead(PIN_SWITCH);

  if (val < 341)       return 1;   // feuchtigkeitsliebend
  else if (val < 682)  return 2;   // normal
  else                 return 3;   // trocken
}

void setup() {
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);
  
  Serial.begin(9600);
  Serial.println(F("Start: Bodenfeuchte-Autobewaesserung + Profilwahlschalter"));
}

void loop() {
  unsigned long now = millis();

  // 1) Messen im Intervall
  if (now - lastSampleTs >= SAMPLE_INTERVAL_MS) {
    lastSampleTs = now;

    int profile  = getWaterProfile();
    int moisture = readSoilAveraged();

    // --- Profilabhängige Parameter setzen ---
    int threshold;
    unsigned long pumpTime;

    switch (profile) {
      case 1: threshold = THRESH_WET;    pumpTime = PUMP_WET_MS;    break;
      case 2: threshold = THRESH_NORMAL; pumpTime = PUMP_NORMAL_MS; break;
      case 3: threshold = THRESH_DRY;    pumpTime = PUMP_DRY_MS;    break;
      default: threshold = THRESH_NORMAL; pumpTime = DEFAULT_PUMP_TIME;
    }

    // Debug-Ausgabe
    Serial.print(F("Profil: ")); Serial.print(profile);
    Serial.print(F(" | Feuchtigkeit: ")); Serial.print(moisture);
    Serial.print(F(" | Schwelle: ")); Serial.print(threshold);
    Serial.print(F(" | Pumpzeit: ")); Serial.println(pumpTime);

    // 2) Startet die Pumpe?
    if (!pumpRunning && moisture >= (threshold + HYSTERESIS)) {
      pumpRunning = true;
      pumpStartTs = now;
      digitalWrite(PIN_PUMP, HIGH);
      Serial.println(F("Pumpe START"));
    }
  }

  // 3) Pumpenlaufzeit nicht-blockierend verwalten
  if (pumpRunning && (now - pumpStartTs >= PUMP_WET_MS)) {
    pumpRunning = false;
    digitalWrite(PIN_PUMP, LOW);
    Serial.println(F("Pumpe STOP"));
  }
}
