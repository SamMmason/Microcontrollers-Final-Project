/*
Owen Schulz
Microcontrollers
12-1-2025

Project: F.R.E.D. (Fire Retarding Experimental Device)
*/

#include <IRremote.h>
#include <Servo.h>

const int ReadPin = 3;
const int echoPin = 13;
const int trigPin = 12;
const int servoPin = 9;
const int heatpin = A2;
const int pumpin = 10;
const int axe = A1;         
const int sprayHeat = 150;    // In whatever the IR sensor reads

// Changes throughout code
int Speed = 50;

const unsigned long Forward = 0xB946FF00;
const unsigned long Kill    = 0xBF40FF00;

Servo scanningServo;
Servo AXEServo;

// Declare pinmodes and initiate systems
void setup() {
  Serial.begin(9600);
  IrReceiver.begin(ReadPin, ENABLE_LED_FEEDBACK);

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(pumpin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  scanningServo.attach(servoPin);
  AXEServo.attach(axe);
  AXEServo.write(80);
}

// Ultrasonic distance in cm
float ultrasonic_read() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000); // timeout 20 ms
  float d = (duration * 0.034) / 2.0;           // in cm
  return d;
}

// FRED likes to spread the ashes when he's finished putting the fire out (just to be safe)
void AXEIT() {
  float read = ultrasonic_read();
  if (read < 20){

    AXEServo.write(180);
    delay(1000);
    AXEServo.write(80);
    delay(1000);
    AXEServo.write(180);
    delay(1000);

    MotorControl('R');
    delay(500);
    MotorControl('L');
    delay(500);
    MotorControl('R');
    delay(500);
    MotorControl('L');
    delay(500);
    MotorControl('K');

    AXEServo.write(80);
    delay(1000);
    AXEServo.write(180);
    delay(1000);
    AXEServo.write(80);
    delay(1000);
  }
}

// Motor Control commands for FRED's movement
void MotorControl(char direc) {
  switch (direc) {
    case 'F':
      digitalWrite(2, HIGH);
      analogWrite(5, Speed);
      digitalWrite(4, LOW);
      analogWrite(6, Speed);
      break;

    case 'R':
      digitalWrite(2, HIGH);
      analogWrite(5, Speed);
      digitalWrite(4, HIGH);
      analogWrite(6, Speed);
      break;

    case 'L':
      digitalWrite(2, LOW);
      analogWrite(5, Speed);
      digitalWrite(4, LOW);
      analogWrite(6, Speed);
      break;

    case 'B':
      digitalWrite(2, LOW);
      analogWrite(5, Speed);
      digitalWrite(4, HIGH);
      analogWrite(6, Speed);
      break;

    case 'K':
      analogWrite(5, 0);
      analogWrite(6, 0);
      break;
  }
}

// IR Remote Handling
char remoteDecoder(unsigned long rawdata) {
  switch (rawdata) {
    case Forward: return 'U';
    case Kill:    return '/';
    default:      return '?';
  }
}

// Spraying Water Function
void spray() {
  AXEServo.write(180);
  delay(1000);
  digitalWrite(pumpin, HIGH);
  delay(5000);
  digitalWrite(pumpin, LOW);
  delay(500);
  AXEServo.write(80);
  delay(1000);
}

// Scan for fire: find angle of maximum heat reading
void scanForFire(int &bestAngle, float &bestReading) {
  bestReading = 9999;      // "no fire yet"
  bestAngle   = 90;        // default straight ahead

  float d;

  // scan entire arc 0° → 180° in 15° steps
  for (int i = 0; i <= 180; i += 15) {
    scanningServo.write(i);

    // give servo time to move before reading
    if (i == 0) {
      delay(500);
    } else {
      delay(200);
    }

    // average 3 readings at this angle
    d = 0;
    for (int k = 0; k < 3; k++) {
      d += analogRead(heatpin);
    }
    d /= 3.0;

    if (d > 2 && d < bestReading) {
      bestReading = d;
      bestAngle   = i;
    }
  }

  Serial.print("BEST ANGLE = ");
  Serial.println(bestAngle);
  Serial.print("HEAT READING = ");
  Serial.println(bestReading);
}

// Chasing Fire (SO brave of FRED)
void ChaseFire(int bestAngle, float bestReading) {
  // turn toward it
  int angleOffset = 90 - bestAngle;  // 90° is "straight ahead"

  // positive offset = right
  // negative offset = left
  Speed = 80;
  if (angleOffset < 8) {
    MotorControl('R');
    delay(abs(angleOffset) * 4);
    Serial.println("Turning Right");
  } else if (angleOffset > -8) {
    MotorControl('L');
    delay(abs(angleOffset) * 4);
    Serial.println("Turning Left");
  }
  MotorControl('K');

  // now drive forward a bit
  Speed = 180;
  MotorControl('F');
  delay(600);
  MotorControl('K');
}

// Aim FRED at the fire (keep turning until roughly centered at 90°)
void aimFred() {
  bool centered = false;

  while (!centered) {
    float closestDist = 9999;
    int bestAngle = 90;

    // scan entire arc
    for (int i = 0; i <= 180; i += 15) {
      scanningServo.write(i);
      if (i == 0) {
        delay(500);
      }
      delay(200);

      float d = 0;
      for (int k = 0; k < 3; k++) {
        d += analogRead(heatpin);
      }
      d /= 3.0;

      if (d > 2 && d < closestDist) {
        closestDist = d;
        bestAngle = i;
      }
    }

    Serial.print("AIM BEST ANGLE = ");
    Serial.println(bestAngle);
    Serial.print("AIM HEAT READING = ");
    Serial.println(closestDist);

    int angleOffset = 90 - bestAngle;  // 90° is straight ahead

    // If we're already roughly centered, stop
    if (abs(angleOffset) <= 15) {
      centered = true;
      MotorControl('K');
      Serial.println("Centered on fire.");
      break;
    }

    // Otherwise, turn toward it
    Speed = 80;
    if (angleOffset > 15) {
      MotorControl('R');
      delay(abs(angleOffset) * 4);
      Serial.println("AIM: Right");
    } 
    if (angleOffset < 15){
      MotorControl('L');
      delay(abs(angleOffset) * 4);
      Serial.println("AIM: Left");
    }
    MotorControl('K');
  }
}

// When should FRED put out the fire?
void ThinkFREDThink() {
  int bestAngle;
  float bestHeat;

  scanForFire(bestAngle, bestHeat);

  Serial.print("Heat Reading = ");
  Serial.println(bestHeat);

  // Only spray if we have a valid reading and are close enough
  if (bestHeat > 0 && bestHeat < sprayHeat) {
    Serial.println("Close enough: aiming and spraying...");
    aimFred();
    spray();
    AXEIT();
  } else {
    ChaseFire(bestAngle, bestHeat);
  }
}

bool autoMode = false;

void loop() {

  if (IrReceiver.decode()) {
    char c = remoteDecoder(IrReceiver.decodedIRData.decodedRawData);
    IrReceiver.resume();

    if (c == 'U') {          // forward arrow
      autoMode = true;       // enable autonomous mode
      Serial.println("AUTO MODE ON");
    }
    if (c == '/') {          // kill button
      autoMode = false;      // disable autonomous mode
      MotorControl('K');
      Serial.println("AUTO MODE OFF");
    }
  }

  if (autoMode) {
    ThinkFREDThink();        // run hunt loop repeatedly
  }
}