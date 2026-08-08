#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "my wifi name";
const char* WIFI_PASS = "my wifi pass";

const int PIN_SERVO = 18;
const int PIN_PIR = 27;
const int PIN_GREEN = 5;
const int PIN_RED = 4;

const int ANGLE_DOWN = 65;
const int ANGLE_UP = 25;
const unsigned long BLINK_MS = 900UL;
const unsigned long SUPPRESS_MS = 15000UL;

WebServer server(80);
Servo servo;

bool lightOn = false;
bool autoMode = true;
bool blinkState = false;
unsigned long holdMs = 300000UL;
unsigned long lastMotionMs = 0;
unsigned long lastBlinkMs = 0;
unsigned long suppressUntilMs = 0;

const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#191c1f">
<title>Light switch</title>
<style>
:root{
  --bg:#191c1f; --panel:#22262a; --raise:#282d32; --edge:#343a41;
  --ink:#e2e6e9; --mute:#868e96; --amber:#f0a02c; --amber-soft:#f0a02c33;
  --mono:ui-monospace,"SF Mono",SFMono-Regular,Menlo,Consolas,monospace;
  --sans:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  background:var(--bg);color:var(--ink);font-family:var(--sans);
  min-height:100vh;display:flex;justify-content:center;
  padding:28px 18px 44px;-webkit-font-smoothing:antialiased;
}
.wrap{width:100%;max-width:400px}

.top{display:flex;align-items:baseline;justify-content:space-between;margin-bottom:22px}
.eyebrow{font-family:var(--mono);font-size:10px;letter-spacing:.22em;
  text-transform:uppercase;color:var(--mute)}
.link{display:flex;align-items:center;gap:6px;font-family:var(--mono);
  font-size:10px;letter-spacing:.14em;text-transform:uppercase;color:var(--mute)}
.dot{width:6px;height:6px;border-radius:50%;background:#4e5661;transition:background .3s}
.dot.live{background:#5ec97a}

.stage{
  background:var(--panel);border:1px solid var(--edge);border-radius:16px;
  padding:30px 24px 24px;text-align:center;position:relative;overflow:hidden;
}
.halo{position:absolute;inset:0;opacity:0;transition:opacity .5s;
  background:radial-gradient(circle at 50% 34%,var(--amber-soft),transparent 62%);
  pointer-events:none}
.stage.on .halo{opacity:1}

svg.plate{width:104px;height:auto;display:block;margin:0 auto 20px;position:relative}
.pl-body{fill:var(--raise);stroke:var(--edge);stroke-width:1.5}
.pl-slot{fill:#15181b}
.pl-screw{fill:none;stroke:#454c54;stroke-width:1.6;stroke-linecap:round}
.pl-lever{fill:#c6ccd2;transform-origin:70px 104px;transform:rotate(180deg);
  transition:transform .42s cubic-bezier(.34,1.4,.5,1)}
.stage.on .pl-lever{fill:#f4f6f7;transform:rotate(0deg)}
.pl-arm{stroke:#5b636c;stroke-width:5;stroke-linecap:round;transform-origin:70px 104px;
  transition:transform .42s cubic-bezier(.34,1.4,.5,1)}
.stage.on .pl-arm{stroke:var(--amber);transform:rotate(-34deg)}

.state{font-size:26px;font-weight:600;letter-spacing:-.02em}
.stage.on .state{color:var(--amber)}
.sub{margin-top:7px;font-family:var(--mono);font-size:11px;letter-spacing:.1em;
  text-transform:uppercase;color:var(--mute);font-variant-numeric:tabular-nums}

.act{width:100%;margin-top:22px;padding:15px;border:0;border-radius:11px;
  background:var(--raise);color:var(--ink);font-family:inherit;font-size:15px;
  font-weight:600;cursor:pointer;transition:background .18s,transform .08s}
.act:hover{background:#30363c}
.act:active{transform:scale(.985)}
.act:focus-visible{outline:2px solid var(--amber);outline-offset:2px}

.row{display:flex;align-items:center;justify-content:space-between;gap:14px;
  padding:17px 18px;background:var(--panel);border:1px solid var(--edge);
  border-radius:13px;margin-top:12px}
.row.col{flex-direction:column;align-items:stretch;gap:13px}
.lbl{font-size:14px;font-weight:500}
.hint{margin-top:3px;font-size:12px;color:var(--mute)}

.sw{position:relative;width:46px;height:27px;flex:none;border-radius:14px;
  background:#3a4148;border:0;cursor:pointer;transition:background .22s}
.sw[aria-checked="true"]{background:var(--amber)}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:21px;height:21px;
  border-radius:50%;background:#e8ebed;transition:transform .22s}
.sw[aria-checked="true"]::after{transform:translateX(19px)}
.sw:focus-visible{outline:2px solid var(--amber);outline-offset:3px}

.chips{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.chip{padding:11px 0;border:1px solid var(--edge);border-radius:9px;background:transparent;
  color:var(--mute);font-family:var(--mono);font-size:12px;letter-spacing:.04em;
  cursor:pointer;transition:all .18s}
.chip:hover{color:var(--ink);border-color:#454c54}
.chip.sel{background:var(--amber);border-color:var(--amber);color:#1a1204;font-weight:600}
.chip:focus-visible{outline:2px solid var(--amber);outline-offset:2px}

.foot{margin-top:20px;text-align:center;font-family:var(--mono);font-size:10px;
  letter-spacing:.16em;text-transform:uppercase;color:#5c646d}
.dim{opacity:.42;pointer-events:none;transition:opacity .25s}
@media(prefers-reduced-motion:reduce){*{transition:none!important}}
</style>
</head>
<body>
<div class="wrap">

  <div class="top">
    <span class="eyebrow">Room light</span>
    <span class="link"><span class="dot" id="dot"></span><span id="linktxt">connecting</span></span>
  </div>

  <div class="stage" id="stage">
    <div class="halo"></div>
    <svg class="plate" viewBox="0 0 140 210" aria-hidden="true">
      <rect class="pl-body" x="21" y="8" width="98" height="194" rx="11"/>
      <path class="pl-screw" d="M70 24v8M66 28h8"/>
      <path class="pl-screw" d="M70 178v8M66 182h8"/>
      <rect class="pl-slot" x="55" y="74" width="30" height="60" rx="7"/>
      <path class="pl-lever" d="M57 104h26l-4-38q-9-6-18 0z"/>
      <line class="pl-arm" x1="70" y1="104" x2="70" y2="66"/>
    </svg>
    <div class="state" id="state">Off</div>
    <div class="sub" id="sub">&mdash;</div>
    <button class="act" id="act">Turn on</button>
  </div>

  <div class="row">
    <div>
      <div class="lbl">Motion sensing</div>
      <div class="hint">Turn the light on when someone walks in</div>
    </div>
    <button class="sw" id="auto" role="switch" aria-checked="true" aria-label="Motion sensing"></button>
  </div>

  <div class="row col" id="holdrow">
    <div>
      <div class="lbl">Turn off after</div>
      <div class="hint">Time with no motion before the light goes off</div>
    </div>
    <div class="chips" id="chips">
      <button class="chip" data-s="60">1 min</button>
      <button class="chip" data-s="300">5 min</button>
      <button class="chip" data-s="600">10 min</button>
      <button class="chip" data-s="1800">30 min</button>
    </div>
  </div>

  <div class="foot">ESP32 &middot; servo actuated</div>
</div>

<script>
const $=i=>document.getElementById(i);
let hold=300;

function fmt(s){
  if(s<=0)return "0:00";
  const m=Math.floor(s/60),r=s%60;
  return m+":"+String(r).padStart(2,"0");
}

function paint(d){
  $("stage").classList.toggle("on",d.on);
  $("state").textContent=d.on?"On":"Off";
  $("act").textContent=d.on?"Turn off":"Turn on";
  $("auto").setAttribute("aria-checked",d.auto?"true":"false");
  $("holdrow").classList.toggle("dim",!d.auto);
  hold=d.hold;
  [...document.querySelectorAll(".chip")].forEach(c=>
    c.classList.toggle("sel",+c.dataset.s===d.hold));
  if(!d.auto) $("sub").textContent="Manual control";
  else if(d.on) $("sub").textContent="Off in "+fmt(d.left);
  else $("sub").textContent="Waiting for motion";
}

async function poll(url){
  try{
    const r=await fetch(url||"/api/state");
    paint(await r.json());
    $("dot").classList.add("live");
    $("linktxt").textContent="connected";
  }catch(e){
    $("dot").classList.remove("live");
    $("linktxt").textContent="no link";
  }
}

$("act").onclick=()=>poll("/api/flip");
$("auto").onclick=()=>poll("/api/auto?v="+($("auto").getAttribute("aria-checked")==="true"?0:1));
document.querySelectorAll(".chip").forEach(c=>
  c.onclick=()=>poll("/api/hold?sec="+c.dataset.s));

poll();
setInterval(poll,1000);
</script>
</body>
</html>)HTML";

void setLight(bool on) {
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(on ? ANGLE_UP : ANGLE_DOWN);
  delay(600);
  servo.detach();
  lightOn = on;
}

void sendState() {
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
  server.send(200, "application/json", j);
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
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("open http://");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("motionlight")) Serial.println("or http://motionlight.local");

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", PAGE);
  });

  server.on("/api/state", HTTP_GET, sendState);

  server.on("/api/flip", HTTP_GET, []() {
    setLight(!lightOn);
    lastMotionMs = millis();
    if (!lightOn) suppressUntilMs = millis() + SUPPRESS_MS;
    sendState();
  });

  server.on("/api/auto", HTTP_GET, []() {
    autoMode = server.arg("v").toInt() == 1;
    lastMotionMs = millis();
    sendState();
  });

  server.on("/api/hold", HTTP_GET, []() {
    long s = server.arg("sec").toInt();
    if (s < 10) s = 10;
    if (s > 7200) s = 7200;
    holdMs = (unsigned long)s * 1000UL;
    sendState();
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "not found");
  });

  server.begin();
  lastMotionMs = millis();
}

void loop() {
  server.handleClient();

  bool motion = digitalRead(PIN_PIR) == HIGH;

  if (autoMode && motion && millis() > suppressUntilMs) {
    lastMotionMs = millis();
    if (!lightOn) setLight(true);
  }

  if (autoMode && lightOn && millis() - lastMotionMs > holdMs) {
    setLight(false);
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
