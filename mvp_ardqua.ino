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
const int PIN_SOIL   = A0;
const int PIN_SWITCH = A1;
const int PIN_PUMP   = 8;
const int PIN_BUTTON = 7;
const int LED_WET    = 1;
const int LED_MED    = 2;
const int LED_DRY    = 3;

// ================= DISPLAY =================
const unsigned long DISPLAY_ON_MS = 5000;
bool displayOn = false;
unsigned long displayOnTs = 0;

// ================= DISPLAY MODUS =================
enum DisplayMode
{
  MODE_GRAPH,
  MODE_TEXT
};

DisplayMode currentMode = MODE_GRAPH;

// ================= PROFILE =================
// threshold of soil moisture measurement
static constexpr int thr[3] = {430, // feuchter Grenzwert
                               520, // Mittlerer Grenzwert
                               610  // Trockener Grenzwert
                              };

// pump run time in ms
static constexpr int prt[3] = {3000, // Pumpdauer feuchter Modus
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
// ================= FUNKTIONEN ====================
// =================================================

int readSoilAveraged()
{
  long sum = 0;
  for (int i = 0; i < N_SAMPLES; i++)
  {
    sum += analogRead(PIN_SOIL);
    delay(5);
  }
  return sum / N_SAMPLES;
}

int getWaterProfile()
{
  int val = analogRead(PIN_SWITCH);
  if (val < 341) return 1;
  if (val < 682) return 2;
  return 3;
}

void runPump(const unsigned long runTime, const int threshold)
{
  Serial.println(F("*** Pumpvorgang START ***"));

  while (true)
  {
    digitalWrite(PIN_PUMP, HIGH);
    delay(runTime);
    digitalWrite(PIN_PUMP, LOW);

    delay(15000);

    int moisture = readSoilAveraged();
    if (moisture < (threshold + HYSTERESIS))
      break;
  }

  Serial.println(F("*** Pumpvorgang STOP ***"));
}

// ================= DISPLAY CONTROL =================

void displayWake()
{
  if (!displayOn)
  {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayOn = true;
  }
  displayOnTs = millis();
}

void displaySleepIfTimeout()
{
  if (displayOn && millis() - displayOnTs >= DISPLAY_ON_MS)
  {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displayOn = false;
  }
}

// ================= FEUCHTEVERLAUF =================

void addMoistureToHistory(int value)
{
  moistureHistory[historyIndex++] = value;
  if (historyIndex >= HISTORY_SIZE)
  {
    historyIndex = 0;
    historyFilled = true;
  }
}

void drawMoistureGraph(int threshold)
{
  if (!displayOn) return;

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
  if (count < 2) return;

  for (int i = 1; i < count; i++)
  {
    int idx0 = (historyIndex + i - count - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    int idx1 = (historyIndex + i - count + HISTORY_SIZE) % HISTORY_SIZE;

    int v0 = moistureHistory[idx0];
    int v1 = moistureHistory[idx1];

    int y0 = map(v0, 0, 1023, gy + gh - 2, gy + 1);
    int y1 = map(v1, 0, 1023, gy + gh - 2, gy + 1);

    int x0 = map(i - 1, 0, count - 1, gx + 1, gx + gw - 2);
    int x1 = map(i,     0, count - 1, gx + 1, gx + gw - 2);

    display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
  }

  int yThresh = map(threshold, 0, 1023, gy + gh - 2, gy + 1);
  display.drawLine(gx + 1, yThresh, gx + gw - 2, yThresh, SSD1306_WHITE);

  display.display();
}

// ================= TEXTANZEIGE =================

void drawTextScreen(int profile, int moisture, int threshold)
{
  if (!displayOn) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print(F("Profil: "));
  display.println(profile);

  display.print(F("Feuchte: "));
  display.println(moisture);

  display.print(F("Schwelle: "));
  display.println(threshold);

  display.display();
}

// =================================================
// ================= KLASSEn =======================
// =================================================

class Ardqua
{
private:
  // current mode (wet=0, medium=1, dry=2)
  int currentMode;

  // current pump run time in ms
  int pumpTime;

  // current led pin
  int ledPin;

  // current OLED modus
  bool displayOn;

  // time of last button press
  int buttonLastPressed;

  // 

public:
  // constructor
  Ardqua(int start_mode)
  {
    this->currentMode = start_mode;
    this->pumpTime = prt[start_mode];
    this->ledPin = led[start_mode];
    this->displayOn = true;
    this->buttonLastPressed = millis();
  }

  void changeMode()
  {
    digitalWrite(this->ledPin, LOW);
    if (this->pumpTime == 2)
    {
      this->pumpTime = 0;
    }
    else
    {
      this->pumpTime++;
    }
    Serial.println(F("Pump Modus: "));
    Serial.println(this->currentMode);
    this->ledPin = led[this->currentMode];
    this->pumpTime = prt[this->currentMode];
    digitalWrite(this->ledPin, HIGH);
    delay(1000);
  }

  void runPump()
  {
    Serial.println(F("*** Pumpvorgang START ***"));

    digitalWrite(PIN_PUMP, HIGH);
    delay(this->pumpTime);
    digitalWrite(PIN_PUMP, LOW);

    Serial.println(F("*** Pumpvorgang STOP ***"));
  }

  void checkBut()
  {
    if (digitalRead(PIN_BUTTON) == HIGH &&
        (millis() - self->buttonLastPressed) < 5000)
    {
      self.changeMode();
    }
    if (digitalRead(PIN_BUTTON) == HIGH)
    {
      self->buttonLastPressed = millis();
      self.displayWake();
    }
    else if ((millis() - self->buttonLastPressed) > 5000 &&
              digitalRead(PIN_BUTTON == LOW))
    {
      this.displaySleep();
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

  int getCurrentMode()
  {
    return this->currentMode;
  }
};

// ========== Objekte erstellen ===========

Ardqua a = Ardqua(0);

// ThreadController that will controll all threads
ThreadController controll = ThreadController();

// Button pruefen
Thread* checkButton = new Thread();

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
    while (true);
  }

  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  // Configure myThread
	checkButton->onRun(a.checkBut());
	checkButton->setInterval(500);

	// Adds both threads to the controller
	controll.add(checkButton);
}

// ================= LOOP =================

void loop()
{
  unsigned long now = millis();

  controll.run();

  // ---------- Taster ----------
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(PIN_BUTTON);

  if (lastButtonState == HIGH && buttonState == LOW)
  {
    displayWake();

    // Modus wechseln
    currentMode = (currentMode == MODE_GRAPH) ? MODE_TEXT : MODE_GRAPH;

    delay(200); // Entprellen
  }
  lastButtonState = buttonState;

  displaySleepIfTimeout();

  // ---------- Messintervall ----------
  if (now < lastSampleTs)
    lastSampleTs = now;

  if (now - lastSampleTs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleTs = now;

    int profile = getWaterProfile();
    int moisture = readSoilAveraged();
    addMoistureToHistory(moisture);

    int threshold;
    unsigned long pumpTime;

    switch (profile)
    {
      case 1: threshold = THRESH_WET;    pumpTime = PUMP_WET_MS;    break;
      case 2: threshold = THRESH_NORMAL; pumpTime = PUMP_NORMAL_MS; break;
      case 3: threshold = THRESH_DRY;    pumpTime = PUMP_DRY_MS;    break;
      default: threshold = THRESH_NORMAL; pumpTime = DEFAULT_PUMP_TIME;
    }

    Serial.print(F("Profil: "));
    Serial.print(profile);
    Serial.print(F(" | Feuchte: "));
    Serial.print(moisture);
    Serial.print(F(" | Schwelle: "));
    Serial.println(threshold);

    // ---------- Anzeige ----------
    if (currentMode == MODE_GRAPH)
      drawMoistureGraph(threshold);
    else
      drawTextScreen(profile, moisture, threshold);

    // ---------- Bewässerung ----------
    if (moisture >= (threshold + HYSTERESIS))
      runPump(pumpTime, threshold);
  }
}
