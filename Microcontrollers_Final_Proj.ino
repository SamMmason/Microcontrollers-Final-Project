#include <IRremote.h>
#include <Servo.h>

const int ReadPin = 3;
const int echoPin = 13;
const int trigPin = 12;
const int servoPin = 9;
const int heatpin = A2;

int Speed = 90;

const unsigned long Forward = 0xB946FF00;
const unsigned long Kill = 0xBF40FF00;

Servo scanningServo;

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(ReadPin, ENABLE_LED_FEEDBACK);

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  scanningServo.attach(servoPin);
}

float ultrasonic_read() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 20000); // timeout
  float d = (duration * 0.034) / 2;
  return d;
}

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

char remoteDecoder(unsigned long rawdata) {
  switch (rawdata) {
    case Forward: return 'U';
    case Kill: return '/';
    default: return '?';
  }
}

void spray(){

}

void acquireTargetAndChase() {

  float closestDist = 9999;
  int bestAngle = 90;

  // scan entire arc
  for (int i = 0; i <= 180; i += 15) {
    scanningServo.write(i);
    if(i == 0){
      delay(500);
    }
    delay(200);
    float d = 0;
    for (int k = 0; k < 3; k++) d += analogRead(heatpin);
    d /= 3.0;

    if (d > 2 && d < closestDist) {
      closestDist = d;
      bestAngle = i;
    }
  }

  Serial.print("BEST ANGLE = ");
  Serial.println(bestAngle);
  Serial.print("DIST = ");
  Serial.println(closestDist);

  // turn toward it
  int angleOffset = 90 - bestAngle;

  // positive offset = right
  // negative offset = left
  Speed = 80;
  if (angleOffset > 8) {
    MotorControl('R');
    delay(abs(angleOffset) * 4);
    Serial.println("Right");
  } else if (angleOffset < -8) {
    MotorControl('L');
    delay(abs(angleOffset) * 4);
    Serial.println("left");
  }
  MotorControl('K');

  // now drive forward
  Speed = 180;
  MotorControl('F');
  delay(600);
  MotorControl('K');
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
    acquireTargetAndChase(); // run hunt loop repeatedly
  }
}