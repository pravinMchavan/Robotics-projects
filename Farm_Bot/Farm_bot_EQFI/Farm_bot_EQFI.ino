#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/* ================= WIFI ================= */
const char* ssid = "FarmBot";
const char* password = "12345678";

/* ================= SERVER ================= */
WebServer server(80);

/* ================= PCA9685 ================= */
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();

#define SERVOMIN 120
#define SERVOMAX 520

#define SERVO_STEP   20   // degrees per button press
#define SERVO_SPEED  2    // smooth step (slow = safe)
#define SERVO_DELAY  30   // delay per step (ms)

int baseAngle = 90;
int armAngle  = 90;
int gripAngle = 90;

/* ================= L298N ================= */
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25
#define ENA 33
#define ENB 32

/* ================= TOOLS ================= */
#define pumpPin   18
#define cutterPin 19

/* ================= SENSOR ================= */
#define soilPin 34

/* ================= SERVO FUNCTIONS ================= */
void setServo(uint8_t ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pca.setPWM(ch, 0, pulse);
}

void moveServoSmooth(uint8_t ch, int &angle, int change) {
  int target = constrain(angle + change, 0, 180);
  if (target == angle) return;

  int step = (target > angle) ? SERVO_SPEED : -SERVO_SPEED;

  for (int a = angle; a != target; a += step) {
    setServo(ch, a);
    delay(SERVO_DELAY);
    if (abs(a - target) < SERVO_SPEED) break;
  }

  angle = target;
  setServo(ch, angle); // final stable position
}

/* ================= ARM / GRIPPER ================= */
void baseL(){ moveServoSmooth(0, baseAngle, -SERVO_STEP); server.send(200); }
void baseR(){ moveServoSmooth(0, baseAngle,  SERVO_STEP); server.send(200); }
void armU() { moveServoSmooth(1, armAngle,   SERVO_STEP); server.send(200); }
void armD() { moveServoSmooth(1, armAngle,  -SERVO_STEP); server.send(200); }
void gripO(){ moveServoSmooth(2, gripAngle,  SERVO_STEP); server.send(200); }
void gripC(){ moveServoSmooth(2, gripAngle, -SERVO_STEP); server.send(200); }

/* ================= MOVEMENT ================= */
void forward(){ digitalWrite(IN1,1); digitalWrite(IN2,0); digitalWrite(IN3,1); digitalWrite(IN4,0); server.send(200); }
void back()   { digitalWrite(IN1,0); digitalWrite(IN2,1); digitalWrite(IN3,0); digitalWrite(IN4,1); server.send(200); }
void left()   { digitalWrite(IN1,0); digitalWrite(IN2,1); digitalWrite(IN3,1); digitalWrite(IN4,0); server.send(200); }
void right()  { digitalWrite(IN1,1); digitalWrite(IN2,0); digitalWrite(IN3,0); digitalWrite(IN4,1); server.send(200); }
void stopBot(){ digitalWrite(IN1,0); digitalWrite(IN2,0); digitalWrite(IN3,0); digitalWrite(IN4,0); server.send(200); }

/* ================= TOOLS ================= */
void pumpOn()  { digitalWrite(pumpPin,1); server.send(200); }
void pumpOff() { digitalWrite(pumpPin,0); server.send(200); }
void cutOn()   { digitalWrite(cutterPin,1); server.send(200); }
void cutOff()  { digitalWrite(cutterPin,0); server.send(200); }

/* ================= SENSOR ================= */
void soil(){
  int val = analogRead(soilPin);
  int percent = map(val, 4095, 1500, 0, 100);
  percent = constrain(percent, 0, 100);
  server.send(200, "text/plain", String(percent));
}

/* ================= UI ================= */
void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FarmBot Controller</title>
<style>
body{margin:0;background:#0b3d0b;font-family:Arial;color:white}
h2{text-align:center;margin:10px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;padding:12px}
.card{background:white;color:black;border-radius:14px;padding:10px;text-align:center}
button{width:120px;height:55px;font-size:16px;font-weight:bold;border:none;border-radius:10px;margin:5px;background:#4CAF50;color:white}
button:active{background:#2e7d32}
.red{background:#e53935}
.blue{background:#1e88e5}
.orange{background:#fb8c00}
.value{font-size:26px;font-weight:bold}
</style>
</head>
<body>

<h2>FARMBOT CONTROLLER</h2>

<div class="grid">

<div class="card">
<h3>ROBOT MOVE</h3>
<button data-on="/f">FORWARD</button><br>
<button data-on="/l">LEFT</button>
<button class="red" data-on="/s">STOP</button>
<button data-on="/r">RIGHT</button><br>
<button data-on="/b">BACK</button>
</div>

<div class="card">
<h3>TOOLS</h3>
<button class="blue" onclick="fetch('/pumpOn')">PUMP ON</button>
<button class="red" onclick="fetch('/pumpOff')">PUMP OFF</button><br>
<button class="orange" onclick="fetch('/cutOn')">CUTTER ON</button>
<button class="red" onclick="fetch('/cutOff')">CUTTER OFF</button>
</div>

<div class="card">
<h3>ARM</h3>
<button data-on="/bl">BASE LEFT</button>
<button data-on="/br">BASE RIGHT</button><br>
<button data-on="/au">ARM UP</button>
<button data-on="/ad">ARM DOWN</button>
</div>

<div class="card">
<h3>GRIPPER</h3>
<button data-on="/gc">CLOSE</button><br>
<button data-on="/go">OPEN</button><br><br>
<h3>SOIL</h3>
<div class="value"><span id="soil">--</span>%</div>
</div>

</div>

<script>
function press(url){ fetch(url); }
function release(){ fetch('/s'); }

document.querySelectorAll("button[data-on]").forEach(btn=>{
  btn.addEventListener("mousedown",()=>press(btn.dataset.on));
  btn.addEventListener("mouseup",release);
  btn.addEventListener("mouseleave",release);

  btn.addEventListener("touchstart",(e)=>{e.preventDefault();press(btn.dataset.on);});
  btn.addEventListener("touchend",(e)=>{e.preventDefault();release();});
});

setInterval(()=>{
  fetch('/soil').then(r=>r.text()).then(v=>soil.innerHTML=v);
},1500);
</script>

</body></html>
)rawliteral");
}

/* ================= SETUP ================= */
void setup() {
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);

  pinMode(pumpPin,OUTPUT);
  pinMode(cutterPin,OUTPUT);
  pinMode(soilPin,INPUT);

  digitalWrite(ENA,HIGH);
  digitalWrite(ENB,HIGH);

  Wire.begin();
  pca.begin();
  pca.setPWMFreq(50);

  setServo(0, baseAngle);
  setServo(1, armAngle);
  setServo(2, gripAngle);

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);

  server.on("/f", forward);
  server.on("/b", back);
  server.on("/l", left);
  server.on("/r", right);
  server.on("/s", stopBot);

  server.on("/bl", baseL);
  server.on("/br", baseR);
  server.on("/au", armU);
  server.on("/ad", armD);
  server.on("/gc", gripC);
  server.on("/go", gripO);

  server.on("/pumpOn", pumpOn);
  server.on("/pumpOff", pumpOff);
  server.on("/cutOn", cutOn);
  server.on("/cutOff", cutOff);

  server.on("/soil", soil);

  server.begin();
}

void loop() {
  server.handleClient();
}
