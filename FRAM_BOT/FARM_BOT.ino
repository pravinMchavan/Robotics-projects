#include <SoftwareSerial.h>
#include <Servo.h>

// -------------------- Bluetooth --------------------
SoftwareSerial BT(3, 2); // RX, TX for Bluetooth

// -------------------- DC Motor Pins (L298N) --------------------
const int IN1 = 8;
const int IN2 = 9;
const int IN3 = 10;
const int IN4 = 11;
const int ENA = 5;
const int ENB = 6;
const int SPEED = 255;

// -------------------- Servo Pins --------------------
Servo servo1; // Left/Right
Servo servo2; // Up/Down
Servo servo3; // Pick/Release

// Current servo angles
int angle1 = 90;
int angle2 = 90;
int angle3 = 90;

void setup() {
  // Serial setup
  Serial.begin(9600);  
  BT.begin(9600);

  // Motor setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Servo setup
  servo1.attach(4);    // Left/Right
  servo2.attach(A1);   // Up/Down
  servo3.attach(A2);   // Grabber

  // Set initial servo positions
  servo1.write(angle1);
  servo2.write(angle2);
  servo3.write(angle3);

  Serial.println("Ready. Send commands like bot_forward, arm_left, stop, etc.");
}

void loop() {
  if (BT.available()) {
    String cmd = BT.readStringUntil('\n');
    cmd.trim();

    Serial.print("Command received: ");
    Serial.println(cmd);

    // First check for bot movement commands
    if (cmd == "bot_forward") forward();
    else if (cmd == "bot_backward") backward();
    else if (cmd == "bot_left") turnLeft();
    else if (cmd == "bot_right") turnRight();
    else if (cmd == "stop") stopMotors();

    // Servo1 - Left/Right
    else if (cmd == "arm_left") {
      angle1 = constrain(angle1 - 20, 0, 180);
      servo1.write(angle1);
    }
    else if (cmd == "arm_right") {
      angle1 = constrain(angle1 + 20, 0, 180);
      servo1.write(angle1);
    }

    // Servo2 - Up/Down
    else if (cmd == "arm_up") {
      angle2 = constrain(angle2 + 20, 0, 180);
      servo2.write(angle2);
    }
    else if (cmd == "arm_down") {
      angle2 = constrain(angle2 - 20, 0, 180);
      servo2.write(angle2);
    }

    // Servo3 - Pick/Release
    else if (cmd == "arm_pick") {
      angle3 = constrain(angle3 + 20, 0, 180);
      servo3.write(angle3);
    }
    else if (cmd == "arm_release") {
      angle3 = constrain(angle3 - 20, 0, 180);
      servo3.write(angle3);
    }

    else {
      Serial.println("Unknown command");
    }
  }
}

// -------------------- DC Motor Functions --------------------
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
}

void backward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
}

void turnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
