#include <Servo.h>

// --- Pin Definitions ---
const int TRIG_PIN  = 9;
const int ECHO_PIN  = 10;
const int PIR_PIN   = 2;
const int LDR_PIN   = A0;
const int BUZZER    = 4; 
const int RED_LED   = 3;
const int GREEN_LED = 5;
const int BLUE_LED  = 11;
const int SERVO_PIN = 6;

// --- Thresholds & Timers ---
const int DIST_THRESHOLD = 30;         
const int AWAY_THRESHOLD = 80;         // User is considered "gone" if > 80cm
const int LIGHT_THRESHOLD = 400;       
const unsigned long BREAK_INTERVAL = 30000; 
const unsigned long STAY_AWAKE_TIMEOUT = 300000; 

// --- Global Variables ---
Servo headServo;
unsigned long studyStartTime = 0;       
unsigned long lastMotionDetectedTime = 0; 
int departureCounter = 0;              // Counter for consecutive "absent" reads

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  headServo.attach(SERVO_PIN);
  headServo.write(90); 
  
  Serial.begin(9600);
  Serial.println("System Initialized with Departure Detection...");
}

void loop() {
  bool motion = digitalRead(PIR_PIN);
  long distance = getAveragedDistance();

  // 1. UPDATE PRESENCE TIMER
  if (motion == HIGH) {
    lastMotionDetectedTime = millis();
  }

  // 2. DEPARTURE CHECK (The 10-Read Logic)
  // If distance is > 80cm OR sensor times out (-1), user is likely gone
  if (distance > AWAY_THRESHOLD || distance == -1) {
    departureCounter++;
  } else {
    departureCounter = 0; // Reset counter if we see the user even once
  }

  // 3. MAIN LOGIC CONTROLLER
  // System stays awake if PIR recently saw motion AND departure counter is low
  if ((millis() - lastMotionDetectedTime < STAY_AWAKE_TIMEOUT) && (departureCounter < 10)) {
    
    if (studyStartTime == 0) {
      studyStartTime = millis();
      Serial.println("--- Session Started ---");
    }

    // Posture Guard
    if (distance < DIST_THRESHOLD && distance > 0) {
      handlePostureAlert();
    } else {
      handleEnvironmentLogic();
    }

    // Pomodoro Timer
    if (millis() - studyStartTime > BREAK_INTERVAL) {
      handleBreakAlert();
    }

    debugToSerial(distance, motion);

  } else {
    // SYSTEM RESET (User left or 10 consecutive absent reads)
    if (studyStartTime != 0) {
      Serial.print("--- Resetting: ");
      if (departureCounter >= 10) Serial.println("User confirmed GONE by Distance Sensor ---");
      else Serial.println("Timeout: No motion for 5 mins ---");
    }
    
    studyStartTime = 0; 
    departureCounter = 0; // Reset counter for next time
    setLED(0, 0, 0); 
    digitalWrite(BUZZER, LOW);
  }
  
  delay(100); 
}

// --- Logic & Hardware Functions ---

void handlePostureAlert() {
  setLED(255, 0, 0); 
  moveServoSmooth(120); 
  digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW);
  delay(100);
  digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW);
  delay(1000); 
  moveServoSmooth(90); 
}

void handleEnvironmentLogic() {
  int lightLevel = analogRead(LDR_PIN);
  if (lightLevel < LIGHT_THRESHOLD) setLED(255, 255, 255); 
  else setLED(0, 255, 0); 
}

void handleBreakAlert() {
  setLED(0, 0, 255);
  digitalWrite(BUZZER, HIGH);
  for(int i=0; i<3; i++) {
    moveServoSmooth(110);
    moveServoSmooth(70);
  }
  digitalWrite(BUZZER, LOW);
  moveServoSmooth(90);
  studyStartTime = millis(); 
}

long getAveragedDistance() {
  long sum = 0;
  int validSamples = 0;
  for(int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
    if (duration > 0) {
      sum += duration * 0.034 / 2;
      validSamples++;
    }
    delay(10);
  }
  return (validSamples > 0) ? (sum / validSamples) : -1;
}

void moveServoSmooth(int targetAngle) {
  int currentAngle = headServo.read();
  int step = (currentAngle < targetAngle) ? 1 : -1;
  while(currentAngle != targetAngle){
    currentAngle += step;
    headServo.write(currentAngle);
    delay(15);
  }
}

void setLED(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}

void debugToSerial(long dist, bool pir) {
  unsigned long elapsed = (millis() - studyStartTime) / 1000;
  
  Serial.print("Dist: "); 
  Serial.print(dist);
  Serial.print("cm | Session: "); 
  Serial.print(elapsed);
  Serial.print("s | Absence Count: "); 
  Serial.print(departureCounter); // Changed println to print
  Serial.print(" | PIR_RAW: "); 
  Serial.println(pir ? "1 (MOV)" : "0 (---)"); // println goes at the very end
}
