#include <Servo.h>

// --- Pin Definitions ---
const int TRIG_PIN = 9;  
const int ECHO_PIN = 10; 
const int PIR_PIN = 2;
const int LDR_PIN = A0;
const int BUZZER  = 4;
const int RED_LED = 3; const int GREEN_LED = 5; const int BLUE_LED = 11;
const int SERVO_PIN = 6;

// --- Thresholds ---
const int POSTURE_THRESHOLD = 35; // Alert if closer than 35cm
const int AWAY_THRESHOLD    = 80; // "Empty chair" if further than 80cm
const int LIGHT_THRESHOLD   = 400; 
const unsigned long BREAK_INTERVAL = 2700000; 
const unsigned long STAY_AWAKE_TIMEOUT = 300000; 

// --- Global Variables ---
Servo headServo;
unsigned long studyStartTime = 0;       
unsigned long lastMotionDetectedTime = 0; 
int departureCounter = 0;

void setup() {
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT); pinMode(GREEN_LED, OUTPUT); pinMode(BLUE_LED, OUTPUT);
  
  headServo.attach(SERVO_PIN);
  headServo.write(90); 
  Serial.begin(9600);
  Serial.println("SINGLE SENSOR STABLE SYSTEM START");
}

void loop() {
  // 1. READ SENSORS
  bool pirMotion = digitalRead(PIR_PIN);
  long distance = getDistance();

  // 2. PIR TRACKING
  if (pirMotion == HIGH) lastMotionDetectedTime = millis();

  // 3. DEPARTURE LOGIC (1 Sensor + PIR)
  // If sensor sees nothing (<80cm) AND PIR is quiet for a while
  bool pirTimedOut = (millis() - lastMotionDetectedTime > STAY_AWAKE_TIMEOUT);
  bool sensorSeesEmpty = (distance > AWAY_THRESHOLD || distance <= 0);

  if (pirTimedOut && sensorSeesEmpty) {
    departureCounter++;
  } else {
    departureCounter = 0; 
  }

  // 4. STATE MACHINE
  if (departureCounter < 10) {
    if (studyStartTime == 0) studyStartTime = millis();

    // Posture Check
    if (distance > 0 && distance < POSTURE_THRESHOLD) {
      handlePostureAlert();
    } else {
      handleEnvironmentLogic();
    }

    // Pomodoro Check
    if (millis() - studyStartTime > BREAK_INTERVAL) handleBreakAlert();

  } else {
    // SYSTEM SLEEP
    studyStartTime = 0;
    setLED(0, 0, 0);
    digitalWrite(BUZZER, LOW);
  }

  printDebug(distance, pirMotion);
  delay(100);
}

// SMOOTH MOVEMENT LOGIC
void moveServoSmooth(int targetAngle) {
  int currentAngle = headServo.read();
  int step = (currentAngle < targetAngle) ? 1 : -1;
  while(currentAngle != targetAngle) {
    currentAngle += step;
    headServo.write(currentAngle);
    delay(15); // Controls speed
  }
}

void handlePostureAlert() {
  setLED(255, 0, 0); 
  moveServoSmooth(110); 
  digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW);
  moveServoSmooth(90);
}

void handleEnvironmentLogic() {
  int lightLevel = analogRead(LDR_PIN);
  if (lightLevel < LIGHT_THRESHOLD) {
    setLED(255, 255, 255);
    digitalWrite(BUZZER, HIGH); delay(20); digitalWrite(BUZZER, LOW);
  } else {
    setLED(0, 255, 0);
  }
}

void handleBreakAlert() {
  setLED(0, 0, 255);
  digitalWrite(BUZZER, HIGH);
  for(int i=0; i<2; i++) {
    moveServoSmooth(110); moveServoSmooth(70);
  }
  digitalWrite(BUZZER, LOW);
  moveServoSmooth(90);
  studyStartTime = millis(); 
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 25000);
  return (dur > 0) ? (dur * 0.034 / 2) : -1;
}

void setLED(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}

void printDebug(long d, bool pir) {
  unsigned long elapsed = (studyStartTime == 0) ? 0 : (millis() - studyStartTime) / 1000;
  Serial.print("DIST: "); Serial.print(d);
  Serial.print(" | PIR: "); Serial.print(pir);
  Serial.print(" | SESSION: "); Serial.print(elapsed);
  Serial.print("s | DEP_CNT: "); Serial.println(departureCounter);
}