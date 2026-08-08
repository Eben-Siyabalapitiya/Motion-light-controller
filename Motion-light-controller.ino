#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int PIN_LED = 2;
const int ANGLE_DOWN = 65;
const int ANGLE_UP = 25;
const unsigned long HOLD_MS = 5000UL;

Servo servo;
bool isUp = false;
unsigned long lastMotionMs = 0;

void blinkOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(110);
    digitalWrite(PIN_LED, LOW);
    delay(110);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
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
      digitalWrite(PIN_LED, HIGH);
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
    blinkOff();
    isUp = false;
  }

  delay(50);
}
