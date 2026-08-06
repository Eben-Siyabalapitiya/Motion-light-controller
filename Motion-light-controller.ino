#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;

const int ANGLE_HOME = 60;   // zero / resting position - servo sits here at boot, mount your arm at this angle
const int ANGLE_PUSH = 120;  // how far it travels from home - raise for more throw, lower for less

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
  servo.write(ANGLE_HOME);
  delay(1000);
  servo.detach();
  Serial.println("ready - servo parked at home");
}

void loop() {
  if (digitalRead(PIN_PIR) == HIGH && millis() - lastTriggerMs > COOLDOWN_MS) {
    lastTriggerMs = millis();
    Serial.println("motion");
    servo.attach(PIN_SERVO, 500, 2400);
    sweepTo(ANGLE_PUSH, ANGLE_HOME);
    sweepTo(ANGLE_HOME, ANGLE_PUSH);
    servo.detach();
  }
  delay(50);
}
