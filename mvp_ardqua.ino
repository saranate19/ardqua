
// --- Konfiguration ---
const int PIN_SOIL = A0;         // Analog-Pin für Feuchtigkeitssensor
const int PIN_PUMP = 8;          // Digital-Pin für Relais/MOSFET der Pumpe (anpassen)
const int THRESHOLD = 500;       // Schwellenwert (0..1023). Beispiel: >600 = zu trocken

// TODO: Temporär sind kurze Intervalle gewählt zu Testzwecken
const unsigned long SAMPLE_INTERVAL_MS = 15UL * 1000UL; // 10 Minuten
const unsigned long PUMP_RUNTIME_MS    = 3UL * 1000UL;        // 10 Sekunden
//const unsigned long SAMPLE_INTERVAL_MS = 10UL * 60UL * 1000UL; // 10 Minuten
//const unsigned long PUMP_RUNTIME_MS    = 10UL * 1000UL;        // 10 Sekunden

// Optional: einfache Hysterese, vermeidet Flattern um den Schwellwert
const int HYSTERESIS = 20;       // Messwerte müssen THRESHOLD+HYST überschreiten

// --- Zustandsvariablen ---
unsigned long lastSampleTs = 0;  // Zeitpunkt der letzten Messung
unsigned long pumpStartTs  = 0;  // Zeitpunkt, als die Pumpe gestartet wurde
bool pumpRunning           = false;

// Für geglättete Messung (Mittelwert aus n Stichproben)
const int N_SAMPLES = 10;
int readSoilAveraged() {
  long sum = 0;
  for (int i = 0; i < N_SAMPLES; ++i) {
    sum += analogRead(PIN_SOIL);
    delay(5); // kurze, unkritische Wartezeit innerhalb Messfunktion zur Beruhigung
  }
  return (int)(sum / N_SAMPLES);
}

void setup() {
  pinMode(PIN_PUMP, OUTPUT);
  // Annahme: HIGH = Pumpe EIN. Falls Relais LOW-aktiv ist, invertieren!
  digitalWrite(PIN_PUMP, LOW); // Pumpe sicher AUS beim Start
  // Optional: serielle Ausgabe zur Diagnose
  Serial.begin(9600);
  Serial.println(F("Start: Bodenfeuchte-Autobewaesserung"));
}

void loop() {
  // TODO: millis startet nach 70 Tagen wieder bei 0 -> Berücksichtigen
  unsigned long now = millis();

  // 1) Zeitgesteuerte Messung alle 10 Minuten
  if (now - lastSampleTs >= SAMPLE_INTERVAL_MS) {
    lastSampleTs = now;
    int moisture = readSoilAveraged();

    Serial.print(F("Feuchtigkeit (0..1023): "));
    Serial.println(moisture);

    // 2) Entscheid: zu trocken? (groesserer Wert = trockener bei vielen Sensoren)
    // Falls dein Sensor invertiert misst (niedriger Wert = trockener), passe die Bedingung an.
    if (!pumpRunning && moisture >= (THRESHOLD + HYSTERESIS)) {
      // Pumpe starten
      pumpRunning  = true;
      pumpStartTs  = now;
      digitalWrite(PIN_PUMP, HIGH); // EIN (bei LOW-aktiv: LOW)
      Serial.println(F("Pumpe START"));
    }
  }

  // 3) Pumpenlaufzeit verwalten (nicht-blockierend)
  if (pumpRunning && (now - pumpStartTs >= PUMP_RUNTIME_MS)) {
    // TODO: Eine eigene Funktion, sodass bei "Pumpbedarf" die Funktion aufgerufen wird
    //       Die Funktion soll kurz pumpen und zB nach 10 Sekunden prüfen, ob der angepeilte
    //       Wert unterschritten ist. Der Wert soll nach Pflanzenkategorie variieren.
    pumpRunning = false;
    digitalWrite(PIN_PUMP, LOW); // AUS (bei LOW-aktiv: HIGH)
    Serial.println(F("Pumpe STOP"));
  }

  // Optional: hier weitere Logik einfügen (LEDs, Tasten, Display, etc.)
}
