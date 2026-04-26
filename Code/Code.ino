#include <Servo.h>

// --- Pin Definitions ---
const int TRIG_1 = 9;  const int ECHO_1 = 10; // Upper (Posture)
const int TRIG_2 = 7;  const int ECHO_2 = 8;  // Lower (Presence)
const int PIR_PIN = 2;
const int LDR_PIN = A0;
const int BUZZER  = 4;
const int RED_LED = 3; const int GREEN_LED = 5; const int BLUE_LED = 11;
const int SERVO_PIN = 6;

// --- Thresholds ---
const int POSTURE_THRESHOLD = 30; // Slouching if < 30cm
const int PRESENT_THRESHOLD = 70; // User present if Lower sensor < 70cm
const int LIGHT_THRESHOLD = 400; // Ambient light threshold (0-1023)
const unsigned long BREAK_INTERVAL = 2700000; 
const unsigned long STAY_AWAKE_TIMEOUT = 300000; 

// --- Global Variables ---
Servo headServo;
unsigned long studyStartTime = 0;       
unsigned long lastMotionDetectedTime = 0; 
bool userInChair = false;
int departureCounter = 0;

void setup() {
  pinMode(TRIG_1, OUTPUT); pinMode(ECHO_1, INPUT);
  pinMode(TRIG_2, OUTPUT); pinMode(ECHO_2, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT); pinMode(GREEN_LED, OUTPUT); pinMode(BLUE_LED, OUTPUT);
  
  headServo.attach(SERVO_PIN);
  headServo.write(90); 
  Serial.begin(9600);
  Serial.println("DUAL-SENSOR SYSTEM START");
}

void loop() {
  // 1. READ SENSORS
  bool pirMotion = digitalRead(PIR_PIN);
  long distUpper = getDistance(TRIG_1, ECHO_1);
  long distLower = getDistance(TRIG_2, ECHO_2);

  // 2. PIR TRACKING
  if (pirMotion == HIGH) {
    lastMotionDetectedTime = millis();
  }

  // 3. PHYSICAL PRESENCE (LOWER SENSOR)
  // We check if the lower sensor sees an object within range
  userInChair = (distLower > 0 && distLower < PRESENT_THRESHOLD);

  // 4. DEPARTURE LOGIC (The "Fix")
  // The user is only "Gone" if PIR is silent AND the distance sensor sees an empty chair
  bool pirTimedOut = (millis() - lastMotionDetectedTime > STAY_AWAKE_TIMEOUT);
  
  if (pirTimedOut && !userInChair) {
    departureCounter++; // Increment only if BOTH sensors agree you are gone
  } else {
    departureCounter = 0; // Reset if either sensor sees you
  }

  // 5. SYSTEM STATE MACHINE
  if (departureCounter < 10) {
    // --- USER IS PRESENT ---
    if (studyStartTime == 0) {
      studyStartTime = millis();
      Serial.println(">>> SESSION STARTED <<<");
    }
    
    // Posture Guard (Upper Sensor)
    if (distUpper < POSTURE_THRESHOLD && distUpper > 0) {
      handlePostureAlert();
    } else {
      handleEnvironmentLogic(); // Checks light and handles sound/LED
    }

    // Pomodoro Check
    if (millis() - studyStartTime > BREAK_INTERVAL) {
      handleBreakAlert();
    }

  } else {
    // --- USER IS NOT PRESENT (SYSTEM SLEEP) ---
    if (studyStartTime != 0) {
      Serial.println(">>> USER LEFT: RESETTING SESSION <<<");
    }
    studyStartTime = 0;
    setLED(0, 0, 0);
    digitalWrite(BUZZER, LOW);
  }

  printDebug(distUpper, distLower, pirMotion);
  delay(100);
}

// Fixed Environment Logic with Sound
void handleEnvironmentLogic() {
  int lightLevel = analogRead(LDR_PIN);
  if (lightLevel < LIGHT_THRESHOLD) {
    setLED(255, 255, 255); // White light
    // Environment chirp: Short and low frequency
    digitalWrite(BUZZER, HIGH);
    delay(20); 
    digitalWrite(BUZZER, LOW);
  } else {
    setLED(0, 255, 0); // Green: Good posture & Good light
    digitalWrite(BUZZER, LOW);
  }
}
// Helper to get distance from a specific sensor
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 25000);
  return (dur > 0) ? (dur * 0.034 / 2) : -1;
}

void printDebug(long d1, long d2, bool pir) {
  unsigned long elapsed = (studyStartTime == 0) ? 0 : (millis() - studyStartTime) / 1000;
  Serial.print("PIR: "); Serial.print(pir);
  Serial.print(" | UP: "); Serial.print(d1);
  Serial.print(" | LOW: "); Serial.print(d2);
  Serial.print(" | SESS: "); Serial.print(elapsed);
  Serial.print(" | DEP_CNT: "); Serial.println(departureCounter);
}

// [Include handlePostureAlert, handleEnvironmentLogic, handleBreakAlert, setLED from previous versions]
void handlePostureAlert() {
  setLED(255, 0, 0); 
  headServo.write(110); delay(200);
  digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW);
  headServo.write(90);
}


void handleBreakAlert() {
  setLED(0, 0, 255);
  digitalWrite(BUZZER, HIGH);
  for(int i=0; i<2; i++) {
    headServo.write(110); delay(300);
    headServo.write(70);  delay(300);
  }
  digitalWrite(BUZZER, LOW);
  headServo.write(90);
  studyStartTime = millis(); 
}

void setLED(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}
