#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;

const int ANGLE_DOWN = 60;
const int ANGLE_UP = 0;

const unsigned long HOLD_MS = 5000UL;

Servo servo;
bool isUp = false;
int currentAngle = ANGLE_DOWN;
unsigned long lastMotionMs = 0;

void sweepTo(int target) {
  int step = (target > currentAngle) ? 2 : -2;
  while (abs(target - currentAngle) > 1) {
    currentAngle += step;
    servo.write(currentAngle);
    delay(12);
  }
  currentAngle = target;
  servo.write(currentAngle);
  delay(200);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGLE_DOWN);
  currentAngle = ANGLE_DOWN;
  delay(1000);
  servo.detach();
  Serial.println("ready");
}

void loop() {
  if (digitalRead(PIN_PIR) == HIGH) {
    lastMotionMs = millis();
    if (!isUp) {
      Serial.println("up");
      servo.attach(PIN_SERVO, 500, 2400);
      sweepTo(ANGLE_UP);
      isUp = true;
    }
  }

  if (isUp && millis() - lastMotionMs > HOLD_MS) {
    Serial.println("down");
    sweepTo(ANGLE_DOWN);
    servo.detach();
    isUp = false;
  }

  delay(50);
}
