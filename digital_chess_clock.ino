#include <LiquidCrystal.h>
#include <EEPROM.h>

// LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Butoane meniu
const int buttonNext = 36;
const int buttonSelect = 34;

// Butoane jucatori
const int buttonP1 = 32;
const int buttonP2 = 38;

// Buzzer
const int buzzerPin = 41;

// Moduri
const char* gameModes[] = {"Blitz", "Rapid", "Classic"};
const int numModes = 3;
int currentMode;
bool modeSelected = false;

int timeP1 = 0;
int timeP2 = 0;
bool player1Turn = true;
bool gameRunning = false;
bool alarmTriggered = false;

unsigned long lastButtonPress = 0;
unsigned long lastTick = 0;
const unsigned long debounceDelay = 200;

// Display 1 - Player 1
const int segmentPins1[8] = {6, 7, 8, 9, 10, 13, A0, A1};
const int digitPins1[4] = {A2, A3, A4, A5};

// Display 2 - Player 2
const int segmentPins2[8] = {49, 51, 43, 26, 22, 31, 28, 24};
const int digitPins2[4] = {53, 47, 45, 30};

const byte digitSegments[10] = {
  0b00111111, 0b00001100, 0b01110110, 0b01011110, 0b01001101,
  0b01011011, 0b01111011, 0b00001110, 0b01111111, 0b01011111
};

void setup() {
  // Initializare LCD
  lcd.begin(16, 2);

  // Citire mod preferat salvat in EEPROM
  currentMode = EEPROM.read(0);
  if (currentMode < 0 || currentMode >= numModes) {
    currentMode = 0;
  }

  // Afisare meniu cu modul preselectat
  lcd.print("Select Mode:");
  lcd.setCursor(0, 1);
  lcd.print(gameModes[currentMode]);

  // Configurare pini
  pinMode(buttonNext, INPUT);
  pinMode(buttonSelect, INPUT);
  pinMode(buttonP1, INPUT);
  pinMode(buttonP2, INPUT);
  pinMode(buzzerPin, OUTPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(segmentPins1[i], OUTPUT);
    pinMode(segmentPins2[i], OUTPUT);
  }

  for (int i = 0; i < 4; i++) {
    pinMode(digitPins1[i], OUTPUT);
    pinMode(digitPins2[i], OUTPUT);
  }
}

void loop() {
  if (!modeSelected) {
    if (millis() - lastButtonPress > debounceDelay) {
      if (digitalRead(buttonNext) == HIGH) {
        currentMode = (currentMode + 1) % numModes;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(gameModes[currentMode]);
        lastButtonPress = millis();
      }

      if (digitalRead(buttonSelect) == HIGH) {
        modeSelected = true;
        EEPROM.update(0, currentMode);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: ");
        lcd.print(gameModes[currentMode]);
        lcd.setCursor(0, 1);
        lcd.print("Player 1's turn");

        int startTime = 0;
        switch (currentMode) {
          case 0: startTime = 5 * 60; break;
          case 1: startTime = 10 * 60; break;
          case 2: startTime = 30 * 60; break;
        }

        timeP1 = startTime;
        timeP2 = startTime;
        player1Turn = true;
        gameRunning = true;
        alarmTriggered = false;
        lastTick = millis();
        lastButtonPress = millis();

        updateDisplay(timeP1, digitPins1, segmentPins1);
        updateDisplay(timeP2, digitPins2, segmentPins2);
      }
    }
  }

  if (gameRunning) {
    // reset joc daca se apasa din nou butonul select
    if (digitalRead(buttonSelect) == HIGH && millis() - lastButtonPress > debounceDelay) {
      gameRunning = false;
      modeSelected = false;
      alarmTriggered = false;
      timeP1 = 0;
      timeP2 = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Mode:");
      lcd.setCursor(0, 1);
      lcd.print(gameModes[currentMode]);
      updateDisplay(timeP1, digitPins1, segmentPins1);
      updateDisplay(timeP2, digitPins2, segmentPins2);
      lastButtonPress = millis();
    }

    if (millis() - lastTick >= 1000) {
      if (player1Turn && timeP1 > 0) timeP1--;
      if (!player1Turn && timeP2 > 0) timeP2--;
      lastTick = millis();

      if ((timeP1 == 0 || timeP2 == 0) && !alarmTriggered) {
        alarmTriggered = true;
        gameRunning = false;
        lcd.setCursor(0, 1);
        lcd.print("   TIME OUT!    ");
        playAlarm();
      }
    }

    if (millis() - lastButtonPress > debounceDelay) {
      if (player1Turn && digitalRead(buttonP1) == HIGH) {
        player1Turn = false;
        playSwitchBeep();
        lcd.setCursor(0, 1);
        lcd.print("Player 2's turn ");
        lastButtonPress = millis();
      } else if (!player1Turn && digitalRead(buttonP2) == HIGH) {
        player1Turn = true;
        playSwitchBeep();
        lcd.setCursor(0, 1);
        lcd.print("Player 1's turn ");
        lastButtonPress = millis();
      }
    }
  }

  updateDisplay(timeP1, digitPins1, segmentPins1);
  updateDisplay(timeP2, digitPins2, segmentPins2);
}

// ========== Display Functions ==========

void updateDisplay(int seconds, const int* digitPins, const int* segmentPins) {
  int mins = seconds / 60;
  int secs = seconds % 60;

  int digits[4] = {
    mins / 10,
    mins % 10,
    secs / 10,
    secs % 10
  };

  for (int i = 0; i < 4; i++) {
    showDigit(digitPins, segmentPins, i, digits[i], i == 1);
    delay(2);
  }
}

void showDigit(const int* digitPins, const int* segmentPins, int digitIndex, int number, bool dot) {
  for (int i = 0; i < 4; i++)
    digitalWrite(digitPins[i], HIGH);

  byte seg = digitSegments[number];
  for (int i = 0; i < 7; i++)
    digitalWrite(segmentPins[i], (seg >> i) & 1);

  digitalWrite(segmentPins[7], dot ? HIGH : LOW);
  digitalWrite(digitPins[digitIndex], LOW);
}

// ========== Buzzer Functions ==========

void playSwitchBeep() {
  tone(buzzerPin, 1000, 100);
  delay(120);
  noTone(buzzerPin);
}

void playAlarm() {
  for (int i = 0; i < 10; i++) {
    tone(buzzerPin, 1500);
    delay(200);
    noTone(buzzerPin);
    delay(100);
  }
}
