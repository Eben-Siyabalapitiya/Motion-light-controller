#include <ESP32Servo.h>

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int ANGLE_DOWN = 60;
const int ANGLE_UP = 0;

Servo servo;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGLE_DOWN);
  delay(500);
  servo.detach();
}

void loop() {
  if (digitalRead(PIN_PIR) == HIGH) {
    Serial.println("motion");
    servo.attach(PIN_SERVO, 500, 2400);
    servo.write(ANGLE_UP);
    delay(5000);
    servo.write(ANGLE_DOWN);
    delay(500);
    servo.detach();
  }
  delay(50);
}
