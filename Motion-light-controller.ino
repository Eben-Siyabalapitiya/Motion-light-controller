#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <time.h>

const char* WIFI_SSID = "BELL991";
const char* WIFI_PASS = "531D1374E947";

const char* MQTT_HOST = "0b4cfe2f4fae436596c1cb84b32aacd9.s1.eu.hivemq.cloud";
const int   MQTT_PORT = 8883;
const char* MQTT_USER = "motionlight";
const char* MQTT_PASS = "motionlight1523";

const char* T_CMD    = "eben/light/cmd";
const char* T_STATE  = "eben/light/state";
const char* T_ONLINE = "eben/light/online";
const char* T_LOG    = "eben/light/log";

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int PIN_GREEN = 5;
const int PIN_RED = 4;

const int ANGLE_DOWN = 65;
const int ANGLE_UP = 25;
const unsigned long BLINK_MS = 900UL;
const unsigned long SUPPRESS_MS = 15000UL;
const unsigned long RETRY_MS = 5000UL;
const unsigned long BEAT_MS = 5000UL;

const unsigned long LOG_COOLDOWN_MS = 900000UL;  
const time_t LOG_WINDOW_S = 172800;               
const int LOG_MAX = 300;                           
const char* TZ_STRING = "EST5EDT,M3.2.0,M11.1.0";  

WiFiClientSecure net;
PubSubClient mqtt(net);
Servo servo;

bool lightOn = false;
bool autoMode = true;
bool blinkState = false;
unsigned long holdMs = 300000UL;
unsigned long lastMotionMs = 0;
unsigned long lastBlinkMs = 0;
unsigned long suppressUntilMs = 0;
unsigned long lastRetryMs = 0;
unsigned long lastBeatMs = 0;

time_t motionLog[LOG_MAX];
int logCount = 0;
unsigned long lastLogMs = 0;
unsigned long lastPruneMs = 0;
bool timeSynced = false;

void setLight(bool on) {
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(on ? ANGLE_UP : ANGLE_DOWN);
  delay(600);
  servo.detach();
  lightOn = on;
}

void publishState() {
  unsigned long left = 0;
  if (autoMode && lightOn) {
    unsigned long gone = millis() - lastMotionMs;
    if (gone < holdMs) left = (holdMs - gone) / 1000UL;
  }
  String j = "{\"on\":";
  j += lightOn ? "true" : "false";
  j += ",\"auto\":";
  j += autoMode ? "true" : "false";
  j += ",\"hold\":";
  j += String(holdMs / 1000UL);
  j += ",\"left\":";
  j += String(left);
  j += "}";
  mqtt.publish(T_STATE, j.c_str(), true);
}

void pruneLog(time_t now) {
  int i = 0;
  while (i < logCount && (now - motionLog[i]) > LOG_WINDOW_S) i++;
  if (i > 0) {
    memmove(motionLog, motionLog + i, (logCount - i) * sizeof(time_t));
    logCount -= i;
  }
}

void addLogEntry(time_t now) {
  if (logCount >= LOG_MAX) {
    memmove(motionLog, motionLog + 1, (logCount - 1) * sizeof(time_t));
    logCount--;
  }
  motionLog[logCount++] = now;
}

void publishLog() {
  String j = "{\"log\":[";
  for (int i = 0; i < logCount; i++) {
    if (i) j += ",";
    j += String((unsigned long)motionLog[i]);
  }
  j += "]}";
  mqtt.publish(T_LOG, j.c_str(), true);
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  char buf[64];
  if (len > 63) len = 63;
  memcpy(buf, payload, len);
  buf[len] = '\0';
  String m = String(buf);
  Serial.print("cmd: ");
  Serial.println(m);

  if (m == "flip" || m == "on" || m == "off") {
    bool target = (m == "flip") ? !lightOn : (m == "on");
    if (target != lightOn) setLight(target);
    lastMotionMs = millis();
    if (!lightOn) suppressUntilMs = millis() + SUPPRESS_MS;
  } else if (m.startsWith("auto:")) {
    autoMode = m.substring(5).toInt() == 1;
    lastMotionMs = millis();
  } else if (m.startsWith("hold:")) {
    long s = m.substring(5).toInt();
    if (s < 10) s = 10;
    if (s > 7200) s = 7200;
    holdMs = (unsigned long)s * 1000UL;
  }
  publishState();
}

void mqttConnect() {
  String cid = "esp32light-" + WiFi.macAddress();
  Serial.print("mqtt connecting... ");
  if (mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASS, T_ONLINE, 1, true, "0")) {
    Serial.println("ok");
    mqtt.publish(T_ONLINE, "1", true);
    mqtt.subscribe(T_CMD);
    publishState();
    publishLog();
  } else {
    Serial.print("failed rc=");
    Serial.println(mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  digitalWrite(PIN_GREEN, LOW);
  digitalWrite(PIN_RED, LOW);

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(ANGLE_DOWN);
  delay(600);
  servo.detach();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("wifi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
    delay(400);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " ok" : " offline");

  configTzTime(TZ_STRING, "pool.ntp.org", "time.nist.gov");

  net.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(4096);
  mqtt.setKeepAlive(30);

  lastMotionMs = millis();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      if (millis() - lastRetryMs > RETRY_MS) {
        lastRetryMs = millis();
        mqttConnect();
      }
    } else {
      mqtt.loop();
    }
  }

  if (!timeSynced && time(nullptr) > 1700000000) {
    timeSynced = true;
  }

  bool motion = digitalRead(PIN_PIR) == HIGH;

  if (autoMode && motion && millis() > suppressUntilMs) {
    lastMotionMs = millis();
    if (!lightOn) {
      setLight(true);
      publishState();
    }
  }

  if (autoMode && lightOn && millis() - lastMotionMs > holdMs) {
    setLight(false);
    publishState();
  }

  if (motion && timeSynced &&
      (lastLogMs == 0 || millis() - lastLogMs >= LOG_COOLDOWN_MS)) {
    time_t now = time(nullptr);
    pruneLog(now);
    addLogEntry(now);
    lastLogMs = millis();
    if (mqtt.connected()) publishLog();
  }

  if (timeSynced && millis() - lastPruneMs > 3600000UL) {
    lastPruneMs = millis();
    int before = logCount;
    pruneLog(time(nullptr));
    if (logCount != before && mqtt.connected()) publishLog();
  }

  if (lightOn && millis() - lastBeatMs > BEAT_MS) {
    lastBeatMs = millis();
    publishState();
  }

  if (lightOn) {
    digitalWrite(PIN_GREEN, HIGH);
    digitalWrite(PIN_RED, LOW);
  } else {
    digitalWrite(PIN_GREEN, LOW);
    if (millis() - lastBlinkMs > BLINK_MS) {
      lastBlinkMs = millis();
      blinkState = !blinkState;
      digitalWrite(PIN_RED, blinkState);
    }
  }

  delay(10);
}
