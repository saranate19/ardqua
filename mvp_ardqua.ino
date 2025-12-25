#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <Thread.h>
#include <ThreadController.h>

// ================= TFT ==================
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8
const int PIN_BL = 7;   // PWM-fähiger Pin für Backlight

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ================= PINS =================
const int PIN_SOIL = A0;
const int PIN_SWITCH = A1;
const int PIN_PUMP = 2;
const int PIN_BUTTON = 6;
const int PIN_WET = 4;
const int PIN_DRY = 5;

// ================= DISPLAY =================
const unsigned long DISPLAY_ON_MS = 10000;

// ================= DISPLAY MODUS =================
enum DisplayMode
{
  MODE_GRAPH,
  MODE_TEXT
};

// ================= PROFILE =================
// threshold of soil moisture measurement
static constexpr int thr[3] = {
    430, // feuchter Grenzwert
    520, // Mittlerer Grenzwert
    610  // Trockener Grenzwert
};

// pump run time in ms
static constexpr int prt[3] = {
    3000, // Pumpdauer feuchter Modus
    2000, // Pumpdauer mittlerer Modus
    1000  // Pumpdauer trockener Modus
};

const unsigned long SAMPLE_INTERVAL_MS = 30000;

const int HYSTERESIS = 20;

// ================= FEUCHTEVERLAUF =================
const int HISTORY_SIZE = 64;
int moistureHistory[HISTORY_SIZE];
int historyIndex = 0;
bool historyFilled = false;

// ================= STATUS =================
unsigned long lastSampleTs = 0;
const int N_SAMPLES = 10;

// =================================================
// ================= KLASSEN =======================
// =================================================

class Ardqua
{
private:
  // current mode (wet=0, medium=1, dry=2)
  int currentMode;
  // current pump run time in ms
  int pumpTime;
  // current threshold of moisture measurement
  int threshold;
  // current tft modus
  bool displayOn;
  // time of last button press
  unsigned long buttonLastPressed;
  // Mode of the TFT Display
  DisplayMode tftMode;

public:
  // constructor
  Ardqua(int start_mode)
  {
    this->currentMode = start_mode;
    this->pumpTime = prt[start_mode];
    this->threshold = thr[start_mode];
    this->displayOn = true;
    this->tftMode = MODE_GRAPH;
    this->buttonLastPressed = millis();
  }

  void changePumpMode(int newmode)
  {
    this->currentMode = newmode;

    Serial.println(F("Pump Modus: "));
    Serial.println(this->currentMode);
    this->pumpTime = prt[this->currentMode];
    this->threshold = thr[this->currentMode];
    delay(1000);
  }

  void checkPump(int moisture)
  {
    if (moisture >= (this->threshold + HYSTERESIS))
    {
      Serial.println(F("*** Pumpvorgang START ***"));

      digitalWrite(PIN_PUMP, HIGH);
      delay(this->pumpTime);
      digitalWrite(PIN_PUMP, LOW);

      Serial.println(F("*** Pumpvorgang STOP ***"));
    }
    else
    {
      Serial.println(F("*** Kein Pumpvorgang notwendig ***"));
    }
  }

  void changeDispMode()
  {
    if (this->tftMode == MODE_GRAPH)
    {
      this->tftMode = MODE_TEXT;
    }
    else
    {
      this->tftMode = MODE_GRAPH;
    }
  }

  void checkSchalt()
  {
    if (digitalRead(PIN_WET) == HIGH && this->currentMode != 0)
    {
      this->changePumpMode(0);
    }
    else if (digitalRead(PIN_DRY) == HIGH && this->currentMode != 2)
    {
      this->changePumpMode(2);
    }
    else if (this->currentMode != 1)
    {
      this->changePumpMode(1);
    }
  }

  void checkBut()
  {
    Serial.println(this->buttonLastPressed);
    Serial.println(this->displayOn);
    if (digitalRead(PIN_BUTTON) == HIGH &&
        (millis() - this->buttonLastPressed) < DISPLAY_ON_MS)
    {
      Serial.println("TFT Display Modus aendern");
      this->changeDispMode();
      this->updateScreen(moistureHistory[-1]);
      this->buttonLastPressed = millis();
    }
    else if (digitalRead(PIN_BUTTON) == HIGH
            && (millis() - this->buttonLastPressed) > DISPLAY_ON_MS)
    {
      this->displayWake();
      Serial.println("TFT Display Wake");
      this->buttonLastPressed = millis();
    }
    else if ((millis() - this->buttonLastPressed) > DISPLAY_ON_MS &&
             digitalRead(PIN_BUTTON == LOW))
    {
      Serial.println("TFT Display Sleep");
      this->displaySleep();
    }
  }

  int readSoilAveraged()
  {
    long sum = 0;
    int moisture;
    for (int i = 0; i < N_SAMPLES; i++)
    {
      sum += analogRead(PIN_SOIL);
      delay(5);
    }
    moisture = sum / N_SAMPLES;

    Serial.print(F("Profil: "));
    Serial.print(this->currentMode);
    Serial.print(F(" | Feuchte: "));
    Serial.print(moisture);
    Serial.print(F(" | Schwelle: "));
    Serial.println(this->threshold);

    return moisture;
  }

  void updateScreen(int moisture)
  {
    if (this->tftMode == MODE_GRAPH)
    {
      this->drawMoistureGraph();
    }
    else
    {
      this->drawTextScreen(moisture);
    }
  }

  void addMoistureToHistory(int value)
  {
    moistureHistory[historyIndex++] = value;
    if (historyIndex >= HISTORY_SIZE)
    {
      historyIndex = 0;
      historyFilled = true;
    }
  }

  void displayWake()
  {
    if (!this->displayOn)
    {
      this->displayOn = true;
      analogWrite(PIN_BL, 200);   // Backlight EIN
    }
  }

  void displaySleep()
  {
    if (this->displayOn)
    {
      this->displayOn = false;
      display.fillScreen(ST77XX_BLACK);
      analogWrite(PIN_BL, 0);     // Backlight AUS
    }
  }

  void drawMoistureGraph()
  {
    if (!this->displayOn)
      return;

    // Größere Fläche an TFT (128x160)
    const int gx = 0;
    const int gy = 20;
    const int gw = 128;
    const int gh = 128;

    // Hintergrund schwarz
    display.fillScreen(ST77XX_BLACK);

    // Titel
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.setTextColor(ST77XX_WHITE);
    display.print(F("Feuchteverlauf"));

    // Rahmen
    display.drawRect(gx, gy, gw, gh, ST77XX_WHITE);

    // Graph Linie
    int count = historyFilled ? HISTORY_SIZE : historyIndex;
    if (count < 2) return;

    for (int i = 1; i < count; i++)
    {
        int idx0 = (historyIndex + i - count - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        int idx1 = (historyIndex + i - count + HISTORY_SIZE) % HISTORY_SIZE;

        int v0 = moistureHistory[idx0];
        int v1 = moistureHistory[idx1];

        int y0 = map(v0, 0, 1023, gy + gh - 2, gy + 2);
        int y1 = map(v1, 0, 1023, gy + gh - 2, gy + 2);

        int x0 = map(i - 1, 0, count - 1, gx + 2, gx + gw - 2);
        int x1 = map(i, 0, count - 1, gx + 2, gx + gw - 2);

        // grüne Linie für Feuchteverlauf
        display.drawLine(x0, y0, x1, y1, ST77XX_GREEN);
    }

    // Schwellenlinie rot
    int yThresh = map(this->threshold, 0, 1023, gy + gh - 2, gy + 2);
    display.drawLine(gx + 2, yThresh, gx + gw - 2, yThresh, ST77XX_RED);
  }

  void drawTextScreen(int moisture)
  {
    if (!this->displayOn)
        return;

    // Hintergrund schwarz
    display.fillScreen(ST77XX_BLACK);

    // Text Einstellungen
    display.setTextSize(2);                 // etwas größer für TFT
    display.setTextColor(ST77XX_WHITE);     // Textfarbe
    display.setCursor(5, 10);

    display.print(F("Profil: "));
    display.println(this->currentMode);

    display.print(F("Feuchte: "));
    display.println(moisture);

    display.print(F("Schwelle: "));
    display.println(this->threshold);
  }

};

// ========== Objekte erstellen ===========

// Objekt a der Klasse Ardqua mit Modus "medium" instanzieren
Ardqua a = Ardqua(1);

// ThreadController that will controll all threads
ThreadController controll = ThreadController();

// Button pruefen
Thread *checkButton = new Thread();
// Feuchtigkeitssensor auslesen
Thread *checkMoisture = new Thread();
// Schalter pruefen
Thread *checkSchalter = new Thread();

// Hack: Funktionen erstellen, weil onRun() nicht mit Methoden funktioniert?
void CallbackButton()
{
  a.checkBut();
}

void CallbackSchalter()
{
  a.checkSchalt();
}

void CallbackMoisture()
{
  int moisture = a.readSoilAveraged();
  a.addMoistureToHistory(moisture);
  a.updateScreen(moisture);
  a.checkPump(moisture);
}

// ================= SETUP =================

void setup()
{
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);

  pinMode(PIN_BUTTON, INPUT);

  pinMode(PIN_BL, OUTPUT);
  analogWrite(PIN_BL, 200);   // Backlight AN (0–255)

  Serial.begin(9600);
  Serial.println(F("Start: Autobewaesserung mit TFT"));

  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_BLACK);

  checkButton->onRun(CallbackButton);
  checkButton->setInterval(200);

  checkSchalter->onRun(CallbackSchalter);
  checkSchalter->setInterval(500);

  checkMoisture->onRun(CallbackMoisture);
  checkMoisture->setInterval(SAMPLE_INTERVAL_MS);

  controll.add(checkButton);
  controll.add(checkSchalter);
  controll.add(checkMoisture);

  a.displayWake();
}

// ================= LOOP =================

void loop()
{
  controll.run();
}