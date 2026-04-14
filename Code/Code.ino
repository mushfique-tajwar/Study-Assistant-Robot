#include <Servo.h>

// Pin Definitions based on the designed circuit
const int TRIG_PIN  = 9;
const int ECHO_PIN  = 10;
const int PIR_PIN   = 2;
const int LDR_PIN   = A0;
const int BUZZER    = 4;
const int RED_LED   = 3;
const int GREEN_LED = 5;
const int BLUE_LED  = 11;
const int SERVO_PIN = 6;

// Constants & Thresholds
const int DIST_THRESHOLD = 30;     // Slouching distance in cm [cite: 38]
const int LIGHT_THRESHOLD = 400;   // Ambient light threshold [cite: 41]
const unsigned long BREAK_INTERVAL = 2700000; // 45 mins in ms [cite: 44]

Servo headServo;
unsigned long studyStartTime = 0;
bool isUserPresent = false;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  headServo.attach(SERVO_PIN);
  headServo.write(90); // Start at neutral position
  Serial.begin(9600);
}

// Function for averaged ultrasonic readings 
long getAveragedDistance() {
  long sum = 0;
  for(int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    sum += pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;
    delay(10);
  }
  return sum / 5;
}

// Function for smooth servo movement 
void moveServoSmooth(int targetAngle) {
  int currentAngle = headServo.read();
  if (currentAngle < targetAngle) {
    for (int pos = currentAngle; pos <= targetAngle; pos++) {
      headServo.write(pos);
      delay(15);
    }
  } else {
    for (int pos = currentAngle; pos >= targetAngle; pos--) {
      headServo.write(pos);
      delay(15);
    }
  }
}

void loop() {
  // 1. Presence Tracking (PIR) 
  isUserPresent = digitalRead(PIR_PIN);
  
  if (isUserPresent) {
    if (studyStartTime == 0) studyStartTime = millis();
    
    // 2. Posture Guard [cite: 35, 38]
    long distance = getAveragedDistance();
    if (distance < DIST_THRESHOLD && distance > 0) {
      setLED(255, 0, 0); // Red alert
      moveServoSmooth(120); // Tilt head up to catch eye [cite: 38]
      delay(2000);
      moveServoSmooth(90);
    } else {
      setLED(0, 255, 0); // Green (Good posture)
    }

    // 3. Adaptive Lighting [cite: 39, 41]
    int lightLevel = analogRead(LDR_PIN);
    if (lightLevel < LIGHT_THRESHOLD) {
      setLED(255, 255, 255); // White auxiliary light
      tone(BUZZER, 500, 100); // Low frequency chirp [cite: 41, 51]
    }

    // 4. Sedentary Alert (Pomodoro) [cite: 43, 45]
    if (millis() - studyStartTime > BREAK_INTERVAL) {
      setLED(0, 0, 255); // Flash Blue
      for(int i=0; i<3; i++) { // Waving motion
        moveServoSmooth(110);
        moveServoSmooth(70);
      }
      studyStartTime = millis(); // Reset timer after alert
    }
  } else {
    studyStartTime = 0; // Reset timer if user leaves
    setLED(0, 0, 0);    // Turn off LEDs
  }
  
  delay(1000); 
}

void setLED(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}
