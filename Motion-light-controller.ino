#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int ANGLE_A = 60;
const int ANGLE_B = 120;
const unsigned long COOLDOWN_MS = 5000UL;

Servo servo;
unsigned long lastTriggerMs = 0;

void sweepTo(int target, int from) {
  int step = (target > from) ? 2 : -2;
  for (int a = from; a != target; a += step) {
    servo.write(a);
    delay(12);
  }
  servo.write(target);
  delay(300);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGLE_A);
  delay(500);
  servo.detach();
  Serial.println("ready");
}

void loop() {
  if (digitalRead(PIN_PIR) == HIGH && millis() - lastTriggerMs > COOLDOWN_MS) {
    lastTriggerMs = millis();
    Serial.println("motion");
    servo.attach(PIN_SERVO, 500, 2400);
    sweepTo(ANGLE_B, ANGLE_A);
    sweepTo(ANGLE_A, ANGLE_B);
    servo.detach();
  }
  delay(50);
}
