#include <Servo.h>

// --- Pin Definitions ---
const int TRIG_PIN = 9;  
const int ECHO_PIN = 10; 
const int PIR_PIN = 2;
const int LDR_PIN = A0;
const int BUZZER  = 4;
const int RED_LED = 3; 
const int GREEN_LED = 5; 
const int BLUE_LED = 11;
const int SERVO_PIN = 6;

// --- Thresholds ---
const int POSTURE_THRESHOLD = 35; 
const int AWAY_THRESHOLD    = 65; 
const int LIGHT_THRESHOLD   = 400; 
const unsigned long BREAK_INTERVAL = 30000; 
const unsigned long STAY_AWAKE_TIMEOUT = 40000; // 2 Minutes

// --- Global Variables ---
Servo headServo;
unsigned long studyStartTime = 0;        
unsigned long lastSeenTime = 0; 
unsigned long lastSerialPrint = 0; 

void setup() {
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT); pinMode(GREEN_LED, OUTPUT); pinMode(BLUE_LED, OUTPUT);
  
  headServo.attach(SERVO_PIN);
  headServo.write(90); 
  Serial.begin(9600);
  Serial.println("System: Ultrasonic-Primary Countdown Mode");
  lastSeenTime = millis(); 
}

void loop() {
  bool pirMotion = digitalRead(PIR_PIN);
  long distance = getDistance();
  int lightLevel = analogRead(LDR_PIN);

  // --- LOGIC: COUNTDOWN BASED ON ULTRASONIC ONLY ---
  // If user is within 80cm, reset the "Last Seen" timer.
  if (distance > 0 && distance < AWAY_THRESHOLD) {
    lastSeenTime = millis();
  }

  // --- DEBUGGING ---
  if (millis() - lastSerialPrint > 1000) {
    long elapsed = millis() - lastSeenTime;
    long secondsRemaining = (STAY_AWAKE_TIMEOUT > elapsed) ? (STAY_AWAKE_TIMEOUT - elapsed) / 1000 : 0;
    
    Serial.print("Dist: "); Serial.print(distance);
    Serial.print(" | PIR: "); Serial.print(pirMotion ? "MOV" : "---");
    Serial.print(" | Countdown: "); Serial.print(secondsRemaining); Serial.println("s");
    lastSerialPrint = millis();
  }

  // --- STATE CHECK ---
  if (millis() - lastSeenTime > STAY_AWAKE_TIMEOUT) {
    handleUserAway(); // Enter Blue Flash Mode
  } else {
    // Normal Operation
    if (studyStartTime == 0) studyStartTime = millis();

    if (distance > 0 && distance < POSTURE_THRESHOLD) {
      handlePostureAlert();
    } else {
      handleEnvironmentLogic(lightLevel);
    }

    if (millis() - studyStartTime > BREAK_INTERVAL) handleBreakAlert();
  }
  delay(50);
}

void handleUserAway() {
  studyStartTime = 0; 
  Serial.println(">>> LOCK: SEARCHING FOR USER (BOTH SENSORS REQUIRED) <<<");

  while (true) {
    // Flash Blue
    setLED(0, 0, 255); 
    delay(500);
    setLED(0, 0, 0);
    delay(500);

    bool motion = digitalRead(PIR_PIN);
    long dist = getDistance();

    // WAKE UP: Requires BOTH sensors to confirm user presence
    if (motion == HIGH && (dist > 0 && dist < AWAY_THRESHOLD)) {
      Serial.println(">>> USER VERIFIED: RETURNING TO NORMAL <<<");
      lastSeenTime = millis(); // Reset inactivity timer
      break; 
    }
  }
}

// --- Standard Handlers ---

void handlePostureAlert() {
  setLED(255, 0, 0); 
  moveServoSmooth(110); 
  digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW);
  moveServoSmooth(90);
}

void handleEnvironmentLogic(int lightLevel) {
  if (lightLevel < LIGHT_THRESHOLD) setLED(255, 255, 255); // White
  else setLED(0, 255, 0); // Green
}

void handleBreakAlert() {
  setLED(0, 0, 255); // Blue
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

void moveServoSmooth(int targetAngle) {
  int currentAngle = headServo.read();
  int step = (currentAngle < targetAngle) ? 1 : -1;
  while(currentAngle != targetAngle) {
    currentAngle += step;
    headServo.write(currentAngle);
    delay(15);
  }
}