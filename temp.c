// --- Konfiguration ---
const int PIN_SOIL = A0;   // Analog-Pin für Feuchtigkeitssensor
const int PIN_SWITCH = A1; // Schalter
const int PIN_PUMP = 8;    // Digital-Pin für Relais/MOSFET
const int PIN_BUT = 9;     // Button fuer Modus-Umschaltung
const int LED_WET = 1;
const int LED_MED = 2;
const int LED_DRY = 3;

// Für geglättete Messung
constexpr int N_SAMPLES = 10;

// TODO: aktuell kurze Intervalle zum Testen
constexpr int SAMPLE_INTERVAL_MS = 30000;

// Hysteresis for moisture value
constexpr int HYSTERESIS = 20;

// threshold of soil moisture measurement
static constexpr int thr[3] = {430, 520, 610};

// pump run time in ms
static constexpr int prt[3] = {3000, 2000, 1000};

// pins for LED configuration
static constexpr int led[3] = {LED_WET, LED_MED, LED_DRY};

class Pump
{
private:
  // current mode of the pump
  int pump_mode;

  // current pump run time in ms
  int pump_time;

  // current led pin
  int led_pin;

public:
  // constructor
  Pump(int start_mode)
  {
    this->pump_mode = start_mode;
    this->pump_time = prt[start_mode];
    this->led_pin = led[start_mode];
  }

  void change_mode()
  {
    digitalWrite(this->led_pin, LOW);
    if (this->pump_mode == 2)
    {
      this->pump_mode = 0;
    }
    else
    {
      this->pump_mode++;
    }
    Serial.println(F("Pump Modus: "));
    Serial.println(this->pump_mode);
    this->led_pin = led[this->pump_mode];
    this->pump_time = prt[this->pump_mode];
    digitalWrite(this->led_pin, HIGH);
    delay(1000);
  }

  void run_pump()
  {
    Serial.println(F("*** Pumpvorgang START ***"));

    digitalWrite(PIN_PUMP, HIGH);
    delay(this->pump_time);
    digitalWrite(PIN_PUMP, LOW);

    Serial.println(F("*** Pumpvorgang STOP ***"));
  }

  int get_pump_mode()
  {
    return this->pump_mode;
  }
};

class Button
{
  private:
    // Status: ist der Button mit erstem Klick aktiviert?
    bool state;
    unsigned long last_pressed;

  public:
    // Constructor
    Button()
    {
      this->state = false;
      this->last_pressed = -10000;
    }

    void pressed()
    {
      if ((millis() - this->last_pressed) > 5000)
      {
        this->last_pressed = millis();
        // TODO: das aktive LED auf AN stellen für 5 Sekunden
      }
      else
      {
        ardqua_pump.change_mode();
        // TODO: LED steuerung in der change_mode ausbauen, dass soll via button klasse oder via neue LED klasse funktionieren.
      }
    }
};

int readSoilAveraged()
{
  long sum = 0;
  for (int i = 0; i < N_SAMPLES; ++i)
  {
    sum += analogRead(PIN_SOIL);
    delay(5);
  }
  return (int)(sum / N_SAMPLES);
}

unsigned long lastSampleTs = 0;
Pump ardqua_pump = Pump(0);


void setup()
{
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);
  pinMode(PIN_BUT, INPUT);

  Serial.begin(9600);
  Serial.println(F("Start: Bodenfeuchte-Autobewaesserung + Profilwahlschalter"));
}

void loop()
{
  if (digitalRead(PIN_BUT) == HIGH)
  {
    // Bei Knopfdruck wird der Modus eins hochgestellt
    ardqua_pump.change_mode();
  }

  unsigned long now = millis();

  // Wenn millis von vorne beginnt, muss letzter Samplezeitpunkt reseted werden
  if (now < lastSampleTs)
  {
    lastSampleTs = now;
  }

  // 1) Messen im Intervall
  if (now - lastSampleTs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleTs = now;

    int moisture = readSoilAveraged();

    // Debug-Ausgabe
    Serial.print(F("Profil: "));
    Serial.print(ardqua_pump.get_pump_mode());
    Serial.print(F(" | Feuchtigkeit: "));
    Serial.print(moisture);
    Serial.print(F(" | Schwelle: "));
    Serial.print(thr[ardqua_pump.get_pump_mode()]);

    // 2) Startet die Pumpe?
    if (moisture >= (thr[ardqua_pump.get_pump_mode()] + HYSTERESIS))
    {
      while (1)
      {
        ardqua_pump.run_pump();

        delay(15000);

        int moisture = readSoilAveraged();
        if (moisture < (thr[ardqua_pump.get_pump_mode()] + HYSTERESIS))
        {
          break;
        }
      }
    }
  }
}
