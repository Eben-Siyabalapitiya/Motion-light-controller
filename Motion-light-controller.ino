#include <ESP32Servo.h>

Servo servo;

void setup() {
  Serial.begin(115200);
  servo.setPeriodHertz(50);
  servo.attach(18, 500, 2400);
}

void loop() {
  for (int a = 0; a <= 180; a += 2) {
    servo.write(a);
    delay(15);
  }
  delay(500);
  for (int a = 180; a >= 0; a -= 2) {
    servo.write(a);
    delay(15);
  }
  delay(500);
}