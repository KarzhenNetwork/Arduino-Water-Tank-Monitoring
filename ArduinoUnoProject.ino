#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const int MAX_LEVEL = 98;    // مستوى المياه لتنشيط الإنذار | ڕێژەی ئاو بۆ ئاگادارکردنەوە
const int MIN_LEVEL = 80;

// pin connections
const int triggerPin = 5;    // trigger مكان الحساس | شوێنی حەساس
const int echoPin = 4;       // echo    مكان الحساس | شوێنی حەساس 
const int buzzer = 11;       //  مكان الجرس | شوێنی دەنگ
const int button = 8;        // زر | شوێکی دوگمە

int waterLevel = 0;
int additionalWaterLevel = 0;

bool ack = 0;
bool lcdIsOn = true;

unsigned long Timer = 0, highTimer = 0, buzzerTimer = 0;
unsigned long normalTimer = 0, lowTimer = 0;
unsigned long lastActivityTime = 0; 

float distance, duration;

void setup() {
  Serial.begin(9600);
  pinMode(triggerPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(button, INPUT_PULLUP);

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
                                             // 1000ms = 1s | 5000ms = 5s | 25000ms = 25s
  if (waterLevel >= 90 && waterLevel <= 100) {
    if (Timer > highTimer + 10000) { // كل 10 ثوانٍ عند الارتفاع | هەموو 10 چرکەیەک لەکاتی بەرزبوونەوەی ئاو
      workingSensor();
      highTimer = Timer;
    }
  } else if (waterLevel > 30 && waterLevel < 90) {
    if (Timer > normalTimer + 30000) { // هەموو 30 چرکەیەک لەکاتی ڕێضەی ئاسایی ئاو | كل 30 ثانية عند الحالة الطبيعية
      workingSensor();
      normalTimer = Timer;
    }
  } else if ( waterLevel <= 30) {
    if (Timer > lowTimer + 10000) { // هەموو 10 چرکەیەک لەکاتی نزمبوونەوەی ئاو | كل 10 ثانية عند انخفاض المستوى
      workingSensor();
      lowTimer = Timer;
    }
  }

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

  if (additionalWaterLevel > 100) { 
    lcd.setCursor(0, 1);
    lcd.print("Warning: ");
    lcd.setCursor(9, 1);
    lcd.print(additionalWaterLevel);
    lcd.setCursor(0, 1);
    lcd.print("            ");
  }

  // قم بإيقاف إضاءة شاشة ل س د بعد 10 ثوانٍ من عدم النشاط 
  // دوای 10 چرکە لە چالاک نەبوونی ل س د  ڕووناکی شاشەکە بکوژێنەوە
  if (millis() - lastActivityTime > 10000 && lcdIsOn) {
    //                              ^^^^^
    lcd.noBacklight();
    lcdIsOn = false;
  }
}

void workingSensor() {
  int minWaterLevel = 150;

  for (int i = 0; i < 10; i++) {
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000);
    distance = (duration * 0.034) / 2;
                         // قیاسی تەواوی درێژی تانکی ئاو بکە بە سانتیمەتر، وە قیاسی ئەو ژمارەیە بکەرە جێی 158
                         // قم بقياس المسافة الحقيقية لخزان المياه بالسنتيمترات، وضعه في المكان 158
    int currentWaterLevel = map(distance, 20, 158, 100, 0); // 
              //                              ^^^
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

  //// these are optional 
 /*
  Serial.print(waterLevel);
  Serial.print(", CM: ");
  Serial.print(distance);
  Serial.print(", EEPROM: ");
  Serial.println(ack);

  lcd.setCursor(13, 0);
  lcd.print("    ");
  lcd.setCursor(13, 0);
  lcd.print(waterLevel); 
  */
}
