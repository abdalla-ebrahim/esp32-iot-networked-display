// ===== Blynk credentials — replace with your own before use =====
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"      // from your Blynk template
#define BLYNK_TEMPLATE_NAME "LED Matrix ESP32 Project"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN_HERE"  // from Blynk device — keep private

#include <WiFi.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>

// ===== Wi-Fi credentials — replace with your own =====
char ssid[] = "YOUR_WIFI_NAME";       // your network name
char pass[] = "YOUR_WIFI_PASSWORD";   // your network password

// MAX7219 pins
#define DIN_PIN 23
#define CLK_PIN 18
#define CS_PIN 5

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
int currentMode = 0;
int currentDigit = 0;
int constantDigit = 0;

bool isRunning = false;

unsigned long lastUpdate = 0;

BlynkTimer timer;

// Digit patterns
byte digits[10][8] = {
  {0x3C,0x66,0x66,0x6E,0x76,0x66,0x66,0x3C},
  {0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x7E},
  {0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x7E},
  {0x3C,0x66,0x06,0x1C,0x06,0x06,0x66,0x3C},
  {0x0C,0x1C,0x2C,0x4C,0x66,0x7E,0x06,0x06},
  {0x7E,0x60,0x60,0x7C,0x06,0x06,0x66,0x3C},
  {0x3C,0x66,0x60,0x60,0x7C,0x66,0x66,0x3C},
  {0x7E,0x06,0x06,0x0C,0x18,0x30,0x30,0x30},
  {0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C},
  {0x3C,0x66,0x66,0x3E,0x06,0x06,0x66,0x3C}
};

// MAX7219 registers
#define MAX7219_REG_SHUTDOWN 0x0C
#define MAX7219_REG_DECODEMODE 0x09
#define MAX7219_REG_SCANLIMIT 0x0B
#define MAX7219_REG_INTENSITY 0x0A
#define MAX7219_REG_DISPLAYTEST 0x0F

void writeMAX7219(byte reg, byte data) {
  digitalWrite(CS_PIN, LOW);
  shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, reg);
  shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data);
  digitalWrite(CS_PIN, HIGH);
}

void clearDisplay() {
  for (int i = 1; i <= 8; i++) {
    writeMAX7219(i, 0x00);
  }
}

void initMAX7219() {
  writeMAX7219(MAX7219_REG_SHUTDOWN, 1);
  writeMAX7219(MAX7219_REG_DECODEMODE, 0);
  writeMAX7219(MAX7219_REG_SCANLIMIT, 7);
  writeMAX7219(MAX7219_REG_INTENSITY, 8);
  writeMAX7219(MAX7219_REG_DISPLAYTEST, 0);
  clearDisplay();
}

void displayDigit(int digit) {
  for (int row = 0; row < 8; row++) {
    writeMAX7219(row + 1, digits[digit][row]);
  }
}

void updateLCD() {

  lcd.clear();

  lcd.setCursor(0, 0);

  switch(currentMode) {
    case 0:
      lcd.print("Timer Mode");
      break;

    case 1:
      lcd.print("Up Counter");
      break;

    case 2:
      lcd.print("Down Counter");
      break;

    case 3:
      lcd.print("Constant");
      break;
  }

  lcd.setCursor(0, 1);
  lcd.print("Digit: ");
  lcd.print(currentDigit);
}

void updateBlynk() {

  Blynk.virtualWrite(V2, currentDigit);

  switch(currentMode) {

    case 0:
      Blynk.virtualWrite(V3, "Timer");
      break;

    case 1:
      Blynk.virtualWrite(V3, "Up Counter");
      break;

    case 2:
      Blynk.virtualWrite(V3, "Down Counter");
      break;

    case 3:
      Blynk.virtualWrite(V3, "Constant");
      break;
  }
}

void setMode(int mode) {

  currentMode = mode;

  switch(mode) {

    case 0:
      currentDigit = 0;
      isRunning = true;
      break;

    case 1:
      currentDigit = 0;
      isRunning = true;
      break;

    case 2:
      currentDigit = 9;
      isRunning = true;
      break;

    case 3:
      currentDigit = constantDigit;
      isRunning = false;
      break;
  }

  displayDigit(currentDigit);
  updateLCD();
  updateBlynk();
}

BLYNK_WRITE(V0) {

  int mode = param.asInt();

  if(mode >= 0 && mode <= 3) {
    setMode(mode);
  }
}

BLYNK_WRITE(V1) {

  constantDigit = param.asInt();

  if(currentMode == 3) {

    currentDigit = constantDigit;

    displayDigit(currentDigit);

    updateLCD();

    updateBlynk();
  }
}

void updateDisplay() {

  if(!isRunning) return;

  unsigned long currentTime = millis();

  if(currentTime - lastUpdate >= 1000) {

    lastUpdate = currentTime;

    switch(currentMode) {

      case 0:
        currentDigit = (currentDigit + 1) % 10;
        break;

      case 1:
        currentDigit = (currentDigit + 1) % 10;
        break;

      case 2:
        currentDigit = (currentDigit - 1 + 10) % 10;
        break;
    }

    displayDigit(currentDigit);

    updateLCD();

    updateBlynk();
  }
}

void handleKeypad(char key) {

  switch(key) {

    case '1':
      setMode(0);
      break;

    case '2':
      setMode(1);
      break;

    case '3':
      setMode(2);
      break;

    case '4':
      setMode(3);
      break;

    case '*':
      isRunning = !isRunning;
      break;

    case '0':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':

      if(currentMode == 3) {

        constantDigit = key - '0';

        currentDigit = constantDigit;

        displayDigit(currentDigit);

        updateLCD();

        updateBlynk();
      }

      break;
  }
}

void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  pinMode(CS_PIN, OUTPUT);
  pinMode(DIN_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);

  digitalWrite(CS_PIN, HIGH);

  initMAX7219();

  lcd.setCursor(0,0);
  lcd.print("Connecting...");

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");

  displayDigit(0);

  timer.setInterval(100L, updateDisplay);
}

void loop() {

  Blynk.run();

  timer.run();

  char key = keypad.getKey();

  if(key) {
    handleKeypad(key);
  }
}