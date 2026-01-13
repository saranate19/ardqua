#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <Thread.h>
#include <ThreadController.h>

// ============== TFT PINS ================
const int PIN_BL  = 7;
const int TFT_CS  = 8;
const int TFT_DC  = 9;
const int TFT_RST = 10;

// ================= PINS =================
const int PIN_SOIL   = A0;
const int PIN_BUTTON = 2;
const int PIN_PUMP   = 3;
const int PIN_WET    = 5;
const int PIN_DRY    = 6;


// ================= DISPLAY =================
const unsigned long DISPLAY_ON_MS = 10000;
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ================= DISPLAY MODUS =================
enum DisplayMode
{
  MODE_GRAPH,
  MODE_TEXT
};

// ================= PROFILE =================
// Maximaler und minimaler "messbarer" Wert (Wasser, Luft)
const int MAX_VALUE = 550;
const int MIN_VALUE = 210;

// threshold of soil moisture measurement
static constexpr int thr[3] = {
    5.5, // feuchter Grenzwert
    7, // Mittlerer Grenzwert
    8.5  // Trockener Grenzwert
};

// pump run time in ms
static constexpr int prt[3] = {
    3000, // Pumpdauer feuchter Modus
    2000, // Pumpdauer mittlerer Modus
    1000  // Pumpdauer trockener Modus
};

static String str[3] = {
    "Feucht", // Pumpdauer feuchter Modus
    "Mittel", // Pumpdauer mittlerer Modus
    "Trocken"  // Pumpdauer trockener Modus
};

const unsigned long SAMPLE_INTERVAL_MS = 10000;

// ================= FEUCHTEVERLAUF =================
const int HISTORY_SIZE = 64;
int moistureHistory[HISTORY_SIZE];
int historyIndex = 0;
bool historyFilled = false;

// ================= STATUS =================
unsigned long lastSampleTs = 0;
const int N_SAMPLES = 10;

// Float Map Funktion fuer Feuchteskala
float fmap(float x, float in_min, float in_max,
           float out_min, float out_max) 
{
  return (x - in_min) * (out_max - out_min)
         / (in_max - in_min) + out_min;
}

// =================================================
// ================= KLASSEN =======================
// =================================================

class Ardqua
{
private:
  // current mode (wet=0, medium=1, dry=2)
  int currentMode;
  String modeStr;
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
    this->modeStr = str[start_mode];
    this->pumpTime = prt[start_mode];
    this->threshold = thr[start_mode];
    this->displayOn = true;
    this->tftMode = MODE_GRAPH;
    this->buttonLastPressed = millis();
  }

  void changePumpMode(int newmode)
  {
    this->currentMode = newmode;
    this->modeStr = str[this->currentMode];
    this->pumpTime = prt[this->currentMode];
    this->threshold = thr[this->currentMode];
    Serial.println(F("Pump Modus: "));
    Serial.println(this->modeStr);
  }

  void checkPump(int moisture)
  {
    if (moisture >= (this->threshold))
    {
      Serial.println(F("*** Pumpvorgang START ***"));
      /* Randnotiz: Der Delay stoert die Funktionalitaet der Threadcontroller Bibliothek
         (Die anderen Threads werden nicht ausgefuehrt)
         Der Button und der Schalter sind aber waehrend dem Pumpvorgang irrelevant. */

      digitalWrite(PIN_PUMP, LOW);
      delay(this->pumpTime);
      digitalWrite(PIN_PUMP, HIGH);

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
    bool I_wet = digitalRead(PIN_WET) == LOW;
    bool II_dry = digitalRead(PIN_DRY) == LOW;
    if (I_wet && this->currentMode != 0)
    {
      this->changePumpMode(0);
    }
    else if (II_dry && this->currentMode != 2)
    {
      this->changePumpMode(2);
    }
    else if (!I_wet && !II_dry && this->currentMode != 1)
    {
      this->changePumpMode(1);
    }
  }

  void checkBut()
  {
    bool but_pressed = digitalRead(PIN_BUTTON) == HIGH;
    if (but_pressed && (millis() - this->buttonLastPressed) < DISPLAY_ON_MS && this->displayOn)
    {
      Serial.println("TFT Display Modus aendern");
      this->changeDispMode();
      this->updateScreen(moistureHistory[historyIndex-1]);
      this->buttonLastPressed = millis();
    }
    else if (but_pressed && (millis() - this->buttonLastPressed) > DISPLAY_ON_MS && !this->displayOn)
    {
      Serial.println("TFT Display aktiviert");
      this->displayOn = true;
      analogWrite(PIN_BL, 200);
      this->updateScreen(moistureHistory[-1]);
      this->buttonLastPressed = millis();
    }
    else if ((millis() - this->buttonLastPressed) > DISPLAY_ON_MS && !but_pressed && this->displayOn)
    {
      Serial.println("TFT Display deaktiviert");
      this->displayOn = false;
      display.fillScreen(ST77XX_BLACK);
      analogWrite(PIN_BL, 0);
    }
  }

  int readSoilAveraged()
  {
    long sum = 0;
    float moisture;
    for (int i = 0; i < N_SAMPLES; i++)
    {
      sum += analogRead(PIN_SOIL);
      delay(5);
    }
    moisture = sum / N_SAMPLES;
    moisture = fmap(moisture, MIN_VALUE, MAX_VALUE, 0, 10);

    Serial.print(F("Profil: "));
    Serial.print(this->modeStr);
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
    if (!historyFilled)
    {
      moistureHistory[historyIndex++] = value;

      if (historyIndex >= HISTORY_SIZE)
      {
        historyIndex = HISTORY_SIZE;
        historyFilled = true;
      }
      return;
    }

    // Array nach links schieben (aeltester Wert faellt raus)
    for (int i = 0; i < HISTORY_SIZE - 1; i++)
    {
      moistureHistory[i] = moistureHistory[i + 1];
    }
    moistureHistory[HISTORY_SIZE - 1] = value;
  }

  void drawMoistureGraph()
{
  if (!this->displayOn)
    return;

  const int gx = 20;
  const int gy = 20;
  const int gw = 100;
  const int gh = 100;

  display.fillScreen(ST77XX_BLACK);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(ST77XX_WHITE);
  display.print(F("Feuchteverlauf"));

  display.drawRect(gx, gy, gw, gh, ST77XX_WHITE);

  // ---------- Y-Achse ----------
  display.setCursor(0, gy);
  display.print(F("dry"));

  display.setCursor(0, gy + gh - 8);
  display.print(F("wet"));

  // ---------- Graph ----------
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

    display.drawLine(x0, y0, x1, y1, ST77XX_GREEN);
  }

  // ---------- Schwelle ----------
  int yThresh = map(this->threshold, 0, 1023, gy + gh - 2, gy + 2);
  display.drawLine(gx + 2, yThresh, gx + gw - 2, yThresh, ST77XX_RED);

  display.setTextColor(ST77XX_RED);
  display.setCursor(0, yThresh-2);
  display.print(this->threshold);

  // ---------- X-Achse ----------
  if ((historyIndex * SAMPLE_INTERVAL_MS / 60000) > 60)
  {
    float totalzeit = round(historyIndex * SAMPLE_INTERVAL_MS / 3600000 * 10) / 10;
    float halbzeit = round(totalzeit*5) / 10;
    display.setTextColor(ST77XX_WHITE);

    display.setCursor(gx, gy + gh + 4);
    display.print(totalzeit);
    display.print(F("h"));

    display.setCursor(gx + (gw/2) - 10, gy + gh + 4);
    display.print(halbzeit);
    display.print(F("h"));
  }
  else
  {
    long totalzeit = round(historyIndex * SAMPLE_INTERVAL_MS / 60000);
    long halbzeit = round(totalzeit*5) / 10;
    display.setTextColor(ST77XX_WHITE);

    display.setCursor(gx, gy + gh + 4);
    display.print(totalzeit);
    display.print(F("min"));

    display.setCursor(gx + (gw/2) -10, gy + gh + 4);
    display.print(halbzeit);
    display.print(F("min"));
  }
  display.setCursor(gx + gw - 18, gy + gh + 4);
  display.print(F("now"));
}


  void drawTextScreen(int moisture)
  {
    if (!this->displayOn)
        return;

    // Hintergrund schwarz
    display.fillScreen(ST77XX_BLACK);

    // Text Einstellungen
    display.setTextSize(1);                 // etwas größer für TFT
    display.setTextColor(ST77XX_WHITE);     // Textfarbe
    display.setCursor(0, 10);

    display.print(F("Profil: "));
    display.println(this->modeStr);

    display.print(F("Feuchte: "));
    display.println(moisture);

    display.print(F("Schwelle: "));
    display.println(this->threshold);

  }

};

// ========== Objekte erstellen ===========

// Objekt a der Klasse Ardqua mit Modus "medium" instanzieren
Ardqua a = Ardqua(1);

// ThreadController: kontrolliert alle Threads.
ThreadController controll = ThreadController();

// Button pruefen und falls gedruckt Display aktivieren/steuern
Thread *checkButton = new Thread();
// Feuchtigkeitssensor auslesen, Messwert speichern, allenfalls Pumpe starten
Thread *checkMoisture = new Thread();
// Schalter pruefen, falls veraendert: Modus anpassen
Thread *checkSchalter = new Thread();

// =============== Funktionen ===============
// Hack: Funktionen erstellen, weil onRun() nicht mit Methoden funktioniert (?)

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
  digitalWrite(PIN_PUMP, HIGH);

  pinMode(PIN_BUTTON, INPUT);

  pinMode(PIN_BL, OUTPUT);
  analogWrite(PIN_BL, 200);   // Backlight AN (0–255)

  pinMode(PIN_WET, INPUT_PULLUP);
  pinMode(PIN_DRY, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("Start: Autobewaesserung mit TFT"));

  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_BLACK);
  a.updateScreen(moistureHistory[0]);

  checkButton->onRun(CallbackButton);
  checkButton->setInterval(200);

  checkSchalter->onRun(CallbackSchalter);
  checkSchalter->setInterval(500);

  checkMoisture->onRun(CallbackMoisture);
  checkMoisture->setInterval(SAMPLE_INTERVAL_MS);

  controll.add(checkButton);
  controll.add(checkSchalter);
  controll.add(checkMoisture);
}

// ================= LOOP =================

void loop()
{
  controll.run();
}
