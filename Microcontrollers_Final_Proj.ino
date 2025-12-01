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
const int pumpin = 10; //Runs a relay that drives the WaterPump
const int axe = A1;      
const int spraydist = 20; // In centimeters
bool hasSprayed = false;   

// Changes throughout code
int Speed = 90;

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

  digitalWrite(pumpin, LOW);
  scanningServo.attach(servoPin);
  AXEServo.attach(axe);
  AXEServo.write(90);
  delay(1000);
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
  AXEServo.write(180);
  delay(100);
  AXEServo.write(90);
  delay(400);
  AXEServo.write(180);
  delay(100);

  MotorControl('R');
  delay(300);
  MotorControl('L');
  delay(300);
  MotorControl('R');
  delay(300);
  MotorControl('L');
  delay(300);

  AXEServo.write(90);
  delay(400);
  AXEServo.write(180);
  delay(100);
  AXEServo.write(90);
  delay(400);
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
  digitalWrite(pumpin, HIGH);
  delay(5000);
  digitalWrite(pumpin, LOW);
  delay(500);
}

// Chasing Fire (SO brave of FRED)

// Tune this after you look at Serial prints:
const int FIRE_THRESHOLD = 850;   // example; you MUST tune this
float ChaseFire() {
  // --------- Coarse scan ----------
  int bestAngle = 90;
  float bestReading = 1023.0;    // max ADC
  float worstReading = 0.0;

  for (int i = 0; i <= 180; i += 15) {
    scanningServo.write(i);
    if (i == 0) {
      delay(500);                // first move settle
    }
    delay(200);                  // sensor settle

    float d = 0;
    const int N = 7;             // more averaging
    for (int k = 0; k < N; k++) {
      d += analogRead(heatpin);
      delay(5);
    }
    d /= N;

    Serial.print("COARSE angle ");
    Serial.print(i);
    Serial.print("  reading = ");
    Serial.println(d);

    if (d < bestReading) {
      bestReading = d;
      bestAngle   = i;
    }
    if (d > worstReading) {
      worstReading = d;
    }
  }

  // Check if fire actually stands out: "best" must be significantly lower
  float contrast = worstReading - bestReading;  // bigger = more obvious fire
  Serial.print("Best coarse reading = ");
  Serial.println(bestReading);
  Serial.print("Worst coarse reading = ");
  Serial.println(worstReading);
  Serial.print("Contrast = ");
  Serial.println(contrast);

  // If there is no clear hot spot, skip movement this cycle
  const float MIN_CONTRAST = 40.0;  // tune experimentally
  if (contrast < MIN_CONTRAST || bestReading > FIRE_THRESHOLD) {
    Serial.println("No strong fire detected in coarse scan.");
    return bestReading;
  }

  // --------- Fine scan around best coarse angle ----------
  int fineStart = max(0,   bestAngle - 15);
  int fineEnd   = min(180, bestAngle + 15);

  float bestReadingFine = bestReading;
  int   bestAngleFine   = bestAngle;

  for (int i = fineStart; i <= fineEnd; i += 3) {
    scanningServo.write(i);
    delay(150);

    float d = 0;
    const int N2 = 7;
    for (int k = 0; k < N2; k++) {
      d += analogRead(heatpin);
      delay(5);
    }
    d /= N2;

    Serial.print("FINE angle ");
    Serial.print(i);
    Serial.print("  reading = ");
    Serial.println(d);

    // still minimizing reading (lower = closer)
    if (d < bestReadingFine) {
      bestReadingFine = d;
      bestAngleFine   = i;
    }
  }

  Serial.print("BEST ANGLE = ");
  Serial.println(bestAngleFine);
  Serial.print("BEST READING = ");
  Serial.println(bestReadingFine);

  // --------- Turn toward that best angle ----------
  int angleOffset = 90 - bestAngleFine;   // 90° is "straight ahead"

  Speed = 80;
  if (angleOffset > 8) {
    MotorControl('R');
    delay(abs(angleOffset) * 4);
    Serial.println("Turning Right");
  } else if (angleOffset < -8) {
    MotorControl('L');
    delay(abs(angleOffset) * 4);
    Serial.println("Turning Left");
  }
  MotorControl('K');

  // --------- Move forward a shorter step ----------
  Speed = 140;     // slower than 180 for better control
  MotorControl('F');
  delay(350);      // smaller hop so you can re-scan often
  MotorControl('K');

  // Return bestReading (lower = closer / hotter)
  return bestReadingFine;
}


// Aim FRED at the fire (keep turning until roughly centered at 90°)
void aimFred() {
  bool centered = false;

  while (!centered) {   // FIX: comparison, not assignment
    float bestReading = 1023.0;
    int   bestAngle   = 90;

    // single scan, tighter step for aiming
    for (int i = 0; i <= 180; i += 10) {
      scanningServo.write(i);
      if (i == 0) {
        delay(500);
      }
      delay(150);

      float d = 0;
      const int N = 7;
      for (int k = 0; k < N; k++) {
        d += analogRead(heatpin);
        delay(5);
      }
      d /= N;

      if (d < bestReading) {
        bestReading = d;
        bestAngle   = i;
      }
    }

    int angleOffset = 90 - bestAngle;

    Serial.print("AIM bestAngle = ");
    Serial.println(bestAngle);
    Serial.print("AIM angleOffset = ");
    Serial.println(angleOffset);

    // close enough to "straight ahead"?
    if (abs(angleOffset) <= 5) {
      centered = true;
      MotorControl('K');
      Serial.println("AIM: centered on fire.");
      break;
    }

    Speed = 70;
    if (angleOffset > 0) {
      MotorControl('R');
      delay(abs(angleOffset) * 4);
      Serial.println("AIM: Right");
    } else {
      MotorControl('L');
      delay(abs(angleOffset) * 4);
      Serial.println("AIM: Left");
    }
    MotorControl('K');
  }
}

void ThinkFREDThink() {
  // -------- QUICK EXIT OPTION --------
  // Take a small average of the IR reading
  float quick = 0;
  const int Nq = 5;
  for (int i = 0; i < Nq; i++) {
    quick += analogRead(heatpin);
    delay(5);
  }
  quick /= Nq;

  Serial.print("Quick IR check = ");
  Serial.println(quick);

  // If IR is very low (very hot / very close), skip the chase
  if (quick < 30) {
    Serial.println("Very close fire detected (quick check) -> AIM + SPRAY");
    aimFred();
    spray();
    AXEIT();
    return;   // exit early, no chasing this cycle
  }
  // -----------------------------------

  // -------- NORMAL BEHAVIOR YOU ALREADY HAD --------
  float dist = ChaseFire();        // your existing chase logic

  if (dist < spraydist) {          // your existing condition
    Serial.println("ChaseFire says we are close enough -> AIM + SPRAY");
    aimFred();
    spray();
    AXEIT();
  }
}


bool autoMode = false;

void loop() {

  if (IrReceiver.decode()) {
    char c = remoteDecoder(IrReceiver.decodedIRData.decodedRawData);
    IrReceiver.resume();

    if (c == 'U') {          // forward arrow
      autoMode = true;
      hasSprayed = false;    // new mission
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