
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const int MAX_LEVEL = 98;
const int MIN_LEVEL = 80;

// Ultrasonic sensor
const int triggerPin = 5;
const int echoPin    = 4;

// Gas sensor (DIGITAL)
const int gasPin = 2;     // DO pin from gas sensor

const int buzzer = 11;
const int button = 9;

int waterLevel = 0;
int additionalWaterLevel = 0;

bool ack = 0;
bool lcdIsOn = true;
bool isHigh = false;
bool gasAlert = false;

unsigned long Timer = 0, highTimer = 0, buzzerTimer = 0;
unsigned long normalTimer = 0, lowTimer = 0;
unsigned long lastActivityTime = 0;

float distance, duration;

void setup() {
  Serial.begin(9600);

  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(gasPin, INPUT);   // GAS SENSOR

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Water Level: ");

  ack = EEPROM.read(0);
  lastActivityTime = millis();

  workingSensor();
}

void loop() {
  Timer = millis();

  // ================= GAS SENSOR CHECK =================
  if (digitalRead(gasPin) == LOW) {   // GAS DETECTED
    gasAlert = true;
    lastActivityTime = millis();

    if (!lcdIsOn) {
      lcd.backlight();
      lcdIsOn = true;
    }

    lcd.setCursor(0, 1);
    lcd.print("!!! GAS ALERT !!!");
    tone(buzzer, 1000, 800);
  } else {
    gasAlert = false;
    lcd.setCursor(0, 1);
    lcd.print("                  ");
  }
  // ===================================================

  // Water level update timing
  if (waterLevel >= 90) {
    if (Timer > highTimer + 200000) {
      workingSensor();
      highTimer = Timer;
    }
  } else if (waterLevel > 30 && waterLevel < 90) {
    if (Timer > normalTimer + 300000) {
      workingSensor();
      normalTimer = Timer;
    }
  } else if (waterLevel <= 30) {
    if (Timer > lowTimer + 200000) {
      workingSensor();
      lowTimer = Timer;
    }
  }

  // Water level alarm
  if (waterLevel >= MAX_LEVEL && ack == false) {
    if (Timer > buzzerTimer + 1000) {
      lastActivityTime = millis();
      if (!lcdIsOn) {
        lcd.backlight();
        lcdIsOn = true;
      }
      tone(buzzer, 1000, 500);
      buzzerTimer = Timer;
    }
    if (!digitalRead(button)) {
      EEPROM.write(0, true);
      ack = true;
    }
  }

  if (waterLevel < MIN_LEVEL && ack == true) {
    EEPROM.write(0, false);
    ack = false;
  }

  // Manual refresh
  if (!digitalRead(button)) {
    workingSensor();
    lastActivityTime = millis();

    if (!lcdIsOn) {
      lcd.backlight();
      lcdIsOn = true;
    }

    lcd.setCursor(0, 1);
    lcd.print("Refreshed >*_*<");
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  // LCD auto off
  if (millis() - lastActivityTime > 10000 && lcdIsOn) {
    lcd.noBacklight();
    lcdIsOn = false;
  }

  isHigh = (waterLevel >= 50);
}

void workingSensor() {
  int minWaterLevel = 150;
  int numLoop = isHigh ? 10 : 23;

  for (int i = 0; i < numLoop; i++) {
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000);
    distance = (duration * 0.034) / 2;

    int currentWaterLevel = map(distance, 20, 158, 100, 0);

    if (currentWaterLevel < minWaterLevel) {
      additionalWaterLevel = currentWaterLevel;
      currentWaterLevel = constrain(currentWaterLevel, 0, 100);
      minWaterLevel = currentWaterLevel;
    }
    delay(50);
  }

  waterLevel = minWaterLevel;

  lcd.setCursor(13, 0);
  lcd.print("   ");
  lcd.setCursor(13, 0);
  lcd.print(waterLevel);
}
