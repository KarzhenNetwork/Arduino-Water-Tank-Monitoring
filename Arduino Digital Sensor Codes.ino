#include <EEPROM.h>

// Pins
const int SENSOR_PIN = 2;   // Digital water sensor DO (assumed LOW when wet)
const int BUTTON_PIN = 9;   // Acknowledge button (INPUT_PULLUP, press = LOW)
const int LED_PIN = 10;     // Indicator LED
const int BUZZER_PIN = 3;  // Buzzer

// EEPROM address to store acknowledge state
const int EEPROM_ADDR_ACK = 0;

// Runtime variables
bool ack = false;           // acknowledgment saved in EEPROM
bool waterPresent = false;  // current sensor reading
unsigned long lastBlinkTime = 0;
unsigned long lastBuzzerTime = 0;
const unsigned long BLINK_INTERVAL = 500;   // ms
const unsigned long BUZZER_INTERVAL = 900;  // ms
bool ledState = false;

void setup() {
  Serial.begin(9600);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Load acknowledgment from EEPROM (0 or 1)
  ack = (EEPROM.read(EEPROM_ADDR_ACK) == 1);

  // Ensure outputs off at start
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void loop() {
  // Read water sensor (assumed: LOW = wet). If your sensor is reversed, change == LOW to == HIGH.
  waterPresent = (digitalRead(SENSOR_PIN) == LOW);

  // If water no longer present, clear ack and outputs
  if (!waterPresent) {
    if (ack) {
      // clear stored ack when water is gone
      ack = false;
      EEPROM.write(EEPROM_ADDR_ACK, 0);
    }
    // ensure outputs off
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    // reset blink/buzzer timers
    lastBlinkTime = millis();
    lastBuzzerTime = millis();
    ledState = false;
    return; // nothing else to do until water appears
  }

  // If waterPresent == true from here on

  // If not acknowledged yet -> blinking LED + repeating buzzer pattern
  if (!ack) {
    unsigned long now = millis();

    // LED blinking (non-blocking)
    if (now - lastBlinkTime >= BLINK_INTERVAL) {
      lastBlinkTime = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }

    // Buzzer repeating short pattern (non-blocking)
    if (now - lastBuzzerTime >= BUZZER_INTERVAL) {
      // Two-tone short pattern using tone() with durations (non-blocking)
      tone(BUZZER_PIN, 1800, 180);            // high short beep
      delay(50);                              // very short gap to make pattern audible
      tone(BUZZER_PIN, 1200, 180);            // low short beep
      lastBuzzerTime = now;
    }

    // Check button press to acknowledge (simple debounce)
    if (!digitalRead(BUTTON_PIN)) { // pressed (LOW)
      delay(40); // tiny debounce
      if (!digitalRead(BUTTON_PIN)) {
        ack = true;
        EEPROM.write(EEPROM_ADDR_ACK, 1); // store ack
        noTone(BUZZER_PIN);
        // leave LED ON steady to show water present (stop blinking)
        digitalWrite(LED_PIN, HIGH);
      }
      // wait for release to avoid bouncing repeated triggers
      while (!digitalRead(BUTTON_PIN)) { delay(10); }
    }

  } else {
    // ack == true: water present and user acknowledged => LED steady ON, no buzzer
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, HIGH);

    // If user presses button while ack true, we can optionally toggle ack off — but per your request
    // we keep ack until water is removed. So we do nothing on button here.
  }

  // Optional: small delay to reduce serial spam (not necessary)
  // delay(10);
}
