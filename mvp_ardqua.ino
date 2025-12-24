#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Thread.h>
#include <ThreadController.h>

// ================= OLED =================
static constexpr int SCREEN_WIDTH = 128;
static constexpr int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= PINS =================
const int PIN_SOIL = A0;
const int PIN_SWITCH = A1;
const int PIN_PUMP = 8;
const int PIN_BUTTON = 7;
const int LED_WET = 1;
const int LED_MED = 2;
const int LED_DRY = 3;
const int PIN_WET = 4;
const int PIN_DRY = 5;

// ================= DISPLAY =================
const unsigned long DISPLAY_ON_MS = 5000;

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

// pins for LED configuration
static constexpr int led[3] = {LED_WET, LED_MED, LED_DRY};

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
  // current led pin
  int ledPin;
  // current OLED modus
  bool displayOn;
  // time of last button press
  int buttonLastPressed;
  // Mode of the OLED Display
  DisplayMode oledMode;

public:
  // constructor
  Ardqua(int start_mode)
  {
    this->currentMode = start_mode;
    this->pumpTime = prt[start_mode];
    this->ledPin = led[start_mode];
    this->threshold = thr[start_mode];
    this->displayOn = true;
    this->oledMode = MODE_GRAPH;
    this->buttonLastPressed = millis();
  }

  void changePumpMode(int newmode)
  {
    digitalWrite(this->ledPin, LOW);

    this->currentMode = newmode;

    Serial.println(F("Pump Modus: "));
    Serial.println(this->currentMode);
    this->ledPin = led[this->currentMode];
    this->pumpTime = prt[this->currentMode];
    this->threshold = thr[this->currentMode];
    digitalWrite(this->ledPin, HIGH);
    delay(1000);
  }

  void checkPump()
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
    if (this->oledMode == MODE_GRAPH)
    {
      this->oledMode = MODE_TEXT;
    }
    else
    {
      this->oledMode = MODE_GRAPH;
    }
  }

  void checkSchalter()
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
      this->changePumpMode(1)
    }
  }

  void checkBut()
  {
    if (digitalRead(PIN_BUTTON) == HIGH &&
        (millis() - this->buttonLastPressed) < DISPLAY_ON_MS)
    {
      this->changeDispMode();
      this->buttonLastPressed = millis();
    }
    if (digitalRead(PIN_BUTTON) == HIGH)
    {
      this->buttonLastPressed = millis();
      this->displayWake();
    }
    else if ((millis() - this->buttonLastPressed) > DISPLAY_ON_MS &&
             digitalRead(PIN_BUTTON == LOW))
    {
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

  void updateScreen()
  {
    if (this->oledMode == MODE_GRAPH)
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
      display.ssd1306_command(SSD1306_DISPLAYON);
      this->displayOn = true;
    }
  }

  void displaySleep()
  {
    if (this->displayOn)
    {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      this->displayOn = false;
    }
  }

  void drawMoistureGraph()
  {
    if (!this->displayOn)
      return;

    const int gx = 0;
    const int gy = 16;
    const int gw = 128;
    const int gh = 48;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Feuchteverlauf"));

    display.drawRect(gx, gy, gw, gh, SSD1306_WHITE);

    int count = historyFilled ? HISTORY_SIZE : historyIndex;
    if (count < 2)
      return;

    for (int i = 1; i < count; i++)
    {
      int idx0 = (historyIndex + i - count - 1 + HISTORY_SIZE) % HISTORY_SIZE;
      int idx1 = (historyIndex + i - count + HISTORY_SIZE) % HISTORY_SIZE;

      int v0 = moistureHistory[idx0];
      int v1 = moistureHistory[idx1];

      int y0 = map(v0, 0, 1023, gy + gh - 2, gy + 1);
      int y1 = map(v1, 0, 1023, gy + gh - 2, gy + 1);

      int x0 = map(i - 1, 0, count - 1, gx + 1, gx + gw - 2);
      int x1 = map(i, 0, count - 1, gx + 1, gx + gw - 2);

      display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }

    int yThresh = map(this->threshold, 0, 1023, gy + gh - 2, gy + 1);
    display.drawLine(gx + 1, yThresh, gx + gw - 2, yThresh, SSD1306_WHITE);

    display.display();
  }

  void drawTextScreen(int moisture)
  {
    if (!this->displayOn)
      return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    display.print(F("Profil: "));
    display.println(this->currentMode);

    display.print(F("Feuchte: "));
    display.println(moisture);

    display.print(F("Schwelle: "));
    display.println(this->threshold);

    display.display();
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

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("Start: Autobewaesserung mit OLED"));

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("OLED nicht gefunden"));
    while (true)
      ;
  }

  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  checkButton->onRun(CallbackButton);
  checkButton->setInterval(200);

  checkSchalter->onRun(CallbackSchalter);
  checkSchalter->setInterval(500);

  checkMoisture->onRun(CallbackMoisture);
  checkMoisture->setInterval(10000); // Temporaer 10 Sekunden

  controll.add(checkButton);
  controll.add(checkSchalter);
  controll.add(checkMoisture);
}

// ================= LOOP =================

void loop()
{
  unsigned long now = millis();

  controll.run();
}