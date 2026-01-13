// arduino_main.ino
#include <SoftwareSerial.h>

#define RX_PIN 8   // Arduino RX -> ESP32 TX
#define TX_PIN 9   // Arduino TX -> ESP32 RX

#define IR_SENSOR_1 A0  // First IR sensor
#define IR_SENSOR_2 A1  // Second IR sensor
#define RELAY_PIN 7     // For water pump/heating
#define L298N_ENA 5     // L298N Enable pin
#define L298N_IN1 3     // L298N Input 1
#define L298N_IN2 4     // L298N Input 2

SoftwareSerial espSerial(RX_PIN, TX_PIN);

// State variables
bool waitingForDrop = false;
bool heatingInProgress = false;
unsigned long heatingStartTime = 0;
unsigned long waterPumpStartTime = 0;
bool waterPumpRunning = false;

void setup() {
  // Initialize pins
  pinMode(IR_SENSOR_1, INPUT);
  pinMode(IR_SENSOR_2, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  
  // Start with everything off
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 0);
  
  // Initialize serial communication
  Serial.begin(9600);
  espSerial.begin(9600);
  
  Serial.println("Arduino Noodle Vending Ready");
}

void loop() {
  // Check for messages from ESP32
  if (espSerial.available()) {
    String command = espSerial.readStringUntil('\n');
    command.trim();
    Serial.println("From ESP32: " + command);
    
    if (command == "DISPENSING") {
      waitingForDrop = true;
      Serial.println("Waiting for noodle drop...");
    } 
    else if (command == "START_HEATING") {
      startHeatingProcess();
    }
  }
  
  // Check IR sensors if waiting for drop
  if (waitingForDrop) {
    checkDropDetection();
  }
  
  // Handle heating process
  if (heatingInProgress) {
    handleHeatingProcess();
  }
  
  delay(50);
}

void checkDropDetection() {
  int sensor1 = digitalRead(IR_SENSOR_1);
  int sensor2 = digitalRead(IR_SENSOR_2);
  
  // If either sensor detects a drop (LOW means object detected)
  if (sensor1 == LOW || sensor2 == LOW) {
    Serial.println("Drop detected by IR sensors!");
    waitingForDrop = false;
    espSerial.println("DROP_DETECTED");
    
    // Send to serial for debugging
    Serial.println("Sent: DROP_DETECTED");
  }
}

void startHeatingProcess() {
  heatingInProgress = true;
  heatingStartTime = millis();
  waterPumpStartTime = millis();
  waterPumpRunning = true;
  
  Serial.println("Starting heating process...");
  
  // Turn on relay (for heater)
  digitalWrite(RELAY_PIN, HIGH);
  
  // Start water pump via L298N
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 255); // Full speed
}

void handleHeatingProcess() {
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
  heatingInProgress = false;
  
  // Turn off relay
  digitalWrite(RELAY_PIN, LOW);
  
  // Ensure water pump is off
  stopWaterPump();
  
  // Notify ESP32
  espSerial.println("HEATING_COMPLETE");
  Serial.println("Heating process completed");
}