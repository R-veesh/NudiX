// arduino_main.ino
#include <SoftwareSerial.h>

#define RX_PIN 8   // Arduino RX -> ESP32 TX (pin 35)
#define TX_PIN 9   // Arduino TX -> ESP32 RX (pin 34)

#define IR_SENSOR_1 A0  // First IR sensor (active LOW when object detected)
#define IR_SENSOR_2 A1  // Second IR sensor
#define RELAY_PIN 7     // For water pump/heating
#define L298N_ENA 5     // L298N Enable pin (PWM)
#define L298N_IN1 3     // L298N Input 1
#define L298N_IN2 4     // L298N Input 2

#define BUZZER_PIN 10   // Optional: for audio feedback

SoftwareSerial espSerial(RX_PIN, TX_PIN);

// State variables
bool waitingForDrop = false;
bool heatingInProgress = false;
bool emergencyStopFlag = false;
unsigned long heatingStartTime = 0;
unsigned long waterPumpStartTime = 0;
bool waterPumpRunning = false;
bool dropDetected = false;
int dropSensorCount = 0;

void setup() {
  // Initialize pins
  pinMode(IR_SENSOR_1, INPUT_PULLUP);  // Use internal pull-up resistor
  pinMode(IR_SENSOR_2, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Start with everything off
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 0);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize serial communication
  Serial.begin(9600);
  espSerial.begin(9600);
  
  // Wait for ESP32 to be ready
  delay(2000);
  
  Serial.println("Arduino Noodle Vending Ready");
  espSerial.println("ARDUINO_READY");
  
  // Beep to indicate ready
  beep(100);
}

void loop() {
  // Check for messages from ESP32
  if (espSerial.available()) {
    String command = espSerial.readStringUntil('\n');
    command.trim();
    Serial.println("From ESP32: " + command);
    
    if (command == "DISPENSING") {
      startDropDetection();
    } 
    else if (command == "START_HEATING") {
      startHeatingProcess();
    }
    else if (command == "EMERGENCY_STOP") {
      emergencyStop();
    }
  }
  
  // Check IR sensors if waiting for drop
  if (waitingForDrop && !emergencyStopFlag) {
    checkDropDetection();
  }
  
  // Handle heating process
  if (heatingInProgress && !emergencyStopFlag) {
    handleHeatingProcess();
  }
  
  // Monitor emergency stop via serial monitor (for debugging)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "stop") {
      emergencyStop();
    }
  }
  
  delay(50);
}

void startDropDetection() {
  waitingForDrop = true;
  dropDetected = false;
  dropSensorCount = 0;
  emergencyStopFlag = false;
  
  Serial.println("Waiting for noodle drop...");
  espSerial.println("WAITING_FOR_DROP");
  
  // Beep twice to indicate waiting
  beep(100);
  delay(200);
  beep(100);
}

void checkDropDetection() {
  int sensor1 = digitalRead(IR_SENSOR_1);
  int sensor2 = digitalRead(IR_SENSOR_2);
  
  // Debug sensor values
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 1000) {
    lastDebug = millis();
    Serial.print("IR Sensors: S1=");
    Serial.print(sensor1);
    Serial.print(" S2=");
    Serial.println(sensor2);
  }
  
  // If either sensor detects a drop (LOW means object detected)
  if (sensor1 == LOW || sensor2 == LOW) {
    dropSensorCount++;
    
    // Require multiple detections to avoid false positives
    if (dropSensorCount >= 3) {
      Serial.println("Drop detected by IR sensors!");
      waitingForDrop = false;
      dropDetected = true;
      
      // Send confirmation to ESP32
      espSerial.println("DROP_DETECTED");
      
      // Beep to indicate detection
      beep(300);
      
      // Also send to serial for debugging
      Serial.println("Sent: DROP_DETECTED");
      
      // Reset for next time
      delay(1000);
    }
  } else {
    dropSensorCount = 0;
  }
  
  // Timeout after 15 seconds
  static unsigned long dropStartTime = 0;
  if (waitingForDrop && dropStartTime == 0) {
    dropStartTime = millis();
  }
  
  if (waitingForDrop && (millis() - dropStartTime > 15000)) {
    Serial.println("Drop detection timeout!");
    waitingForDrop = false;
    espSerial.println("DROP_TIMEOUT");
    dropStartTime = 0;
  }
}

void startHeatingProcess() {
  if (emergencyStopFlag) return;
  
  heatingInProgress = true;
  heatingStartTime = millis();
  waterPumpStartTime = millis();
  waterPumpRunning = true;
  
  Serial.println("Starting heating process...");
  espSerial.println("HEATING_STARTED");
  
  // Turn on relay (for heater)
  digitalWrite(RELAY_PIN, HIGH);
  
  // Start water pump via L298N
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 200); // 200/255 speed
  
  // Beep pattern for heating start
  beep(100);
  delay(100);
  beep(100);
  delay(100);
  beep(100);
}

void handleHeatingProcess() {
  if (emergencyStopFlag) {
    stopHeatingProcess();
    return;
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - heatingStartTime;
  
  // Check if water pump should stop (after 20 seconds)
  if (waterPumpRunning && (currentTime - waterPumpStartTime > 20000)) {
    stopWaterPump();
  }
  
  // Check if heating should stop (after 10 minutes = 600000 ms)
  if (elapsed > 600000) {
    stopHeatingProcess();
  }
  
  // Send progress updates every 30 seconds
  static unsigned long lastUpdate = 0;
  if (currentTime - lastUpdate > 30000) {
    lastUpdate = currentTime;
    int minutesLeft = (600000 - elapsed) / 60000;
    Serial.print("Heating: ");
    Serial.print(minutesLeft);
    Serial.println(" minutes remaining");
    
    if (minutesLeft <= 1) {
      // Final minute beep
      beep(500);
    }
  }
}

void stopWaterPump() {
  if (waterPumpRunning) {
    waterPumpRunning = false;
    analogWrite(L298N_ENA, 0);
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, LOW);
    Serial.println("Water pump stopped");
  }
}

void stopHeatingProcess() {
  if (!heatingInProgress) return;
  
  heatingInProgress = false;
  
  // Turn off relay
  digitalWrite(RELAY_PIN, LOW);
  
  // Ensure water pump is off
  stopWaterPump();
  
  // Notify ESP32
  espSerial.println("HEATING_COMPLETE");
  Serial.println("Heating process completed");
  
  // Completion beep pattern
  for (int i = 0; i < 3; i++) {
    beep(100);
    delay(100);
  }
}

void emergencyStop() {
  Serial.println("EMERGENCY STOP ACTIVATED!");
  
  emergencyStopFlag = true;
  waitingForDrop = false;
  
  // Stop everything immediately
  digitalWrite(RELAY_PIN, LOW);
  analogWrite(L298N_ENA, 0);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  
  // Notify ESP32
  espSerial.println("EMERGENCY_STOPPED");
  
  // Emergency beep pattern
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
  
  // Reset state after emergency stop
  delay(1000);
  emergencyStopFlag = false;
  heatingInProgress = false;
  waterPumpRunning = false;
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}