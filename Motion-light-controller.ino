#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int PIN_BLUE = 2;
const int PIN_RED = 4;
const int ANGLE_DOWN = 65;
const int ANGLE_UP = 25;
const unsigned long HOLD_MS = 300000UL;
const unsigned long BLINK_MS = 900UL;

Servo servo;
bool isUp = false;
bool blinkState = false;
unsigned long lastMotionMs = 0;
unsigned long lastBlinkMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  digitalWrite(PIN_BLUE, LOW);
  digitalWrite(PIN_RED, LOW);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGLE_DOWN);
  delay(500);
  servo.detach();
  Serial.println("ready");
}

void loop() {
  if (digitalRead(PIN_PIR) == HIGH) {
    lastMotionMs = millis();
    if (!isUp) {
      Serial.println("up");
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_BLUE, HIGH);
      servo.attach(PIN_SERVO, 500, 2400);
      servo.write(ANGLE_UP);
      isUp = true;
    }
  }

  if (isUp && millis() - lastMotionMs > HOLD_MS) {
    Serial.println("down");
    servo.write(ANGLE_DOWN);
    delay(500);
    servo.detach();
    digitalWrite(PIN_BLUE, LOW);
    isUp = false;
    lastBlinkMs = millis();
  }

  if (!isUp && millis() - lastBlinkMs > BLINK_MS) {
    lastBlinkMs = millis();
    blinkState = !blinkState;
    digitalWrite(PIN_RED, blinkState);
  }

  delay(50);
}
