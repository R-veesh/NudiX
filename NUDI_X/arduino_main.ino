// arduino_main.ino
#include <SoftwareSerial.h>

#define RX_PIN 8 // Arduino RX <- ESP32 TX
#define TX_PIN 9 // Arduino TX -> ESP32 RX

// Ultrasonic Sensor Pins (HC-SR04)
#define ULTRASONIC_TRIG 2
#define ULTRASONIC_ECHO 6

#define RELAY_PIN 7 // Heater / water relay
#define L298N_IN1 3 // L298N Input 1
#define L298N_IN2 4 // L298N Input 2

SoftwareSerial espSerial(RX_PIN, TX_PIN);

// State variables
bool waitingForDrop = false;
bool heatingInProgress = false;
bool emergencyStopFlag = false;

bool relayActive = false;
bool l298nMotorActive = false;

unsigned long relayStartTime = 0;
unsigned long l298nStartTime = 0;
unsigned long lastUltrasonicCheck = 0;

// Config
#define DISTANCE_THRESHOLD 23 // cm (adjustable)
#define RELAY_DURATION 600000UL // 10 minutes (ms)
#define L298N_DURATION 10000UL  // 10 seconds (ms)
#define DROP_CONFIRM_REQUIRED 3 // require 3 consecutive readings under threshold to confirm drop

int dropConfirmCount = 0;
unsigned long lastSerialTime = 0;

void setup() {
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);

  // Default OFF
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);

  Serial.begin(9600);
  espSerial.begin(9600);
  delay(2000);

  Serial.println("=== Arduino Noodle Vending Ready ===");
  espSerial.println("ARDUINO_READY");

  Serial.println("=== Arduino Configuration ===");
  Serial.println("RX Pin: 8");
  Serial.println("TX Pin: 9");
  Serial.println("Relay Pin: 7");
  Serial.println("Ultrasonic Trig: 2");
  Serial.println("Ultrasonic Echo: 6");
  Serial.println("L298N IN1: 3");
  Serial.println("L298N IN2: 4");
}

void loop() {
  // Handle incoming commands from ESP32
  if (espSerial.available()) {
    String command = espSerial.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      Serial.print("[");
      Serial.print(millis());
      Serial.print("] Received command: ");
      Serial.println(command);

      if (command == "DISPENSING") {
        if (!emergencyStopFlag) startDropDetection();
        else {
          Serial.println("Emergency active - ignoring DISPENSING");
          espSerial.println("EMERGENCY_ACTIVE");
        }
      }
      else if (command == "START_HEATING") {
        if (!emergencyStopFlag && !heatingInProgress) startHeatingProcess();
      }
      else if (command == "EMERGENCY_STOP") {
        emergencyStop();
      }
      else if (command == "RESET") {
        // clear emergency flag
        emergencyStopFlag = false;
        Serial.println("[RESET] Emergency flag cleared");
        espSerial.println("RESET_DONE");
      }
    }
  }

  // Drop detection loop (non-blocking)
  if (waitingForDrop && !emergencyStopFlag) {
    checkDropDetection();
  }

  // Heating/pump motor management (non-blocking)
  if (heatingInProgress && !emergencyStopFlag) {
    handleHeatingProcess();
  }

  // small idle delay
  delay(30);
}

void startDropDetection() {
  waitingForDrop = true;
  dropConfirmCount = 0;
  Serial.println("\n========== NOODLE DROP DETECTION STARTED ==========");
  espSerial.println("WAITING_FOR_DROP");
}

float getUltrasonicDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000); // 30 ms timeout
  if (duration == 0) return -1.0;
  return (duration * 0.0343) / 2.0;
}

void checkDropDetection() {
  unsigned long now = millis();

  float distance = getUltrasonicDistance();
  Serial.print("[");
  Serial.print(now);
  Serial.print("] Distance: ");
  if (distance < 0) {
    Serial.println("timeout");
    dropConfirmCount = 0; // reset confirmation on bad reading
    return;
  } else {
    Serial.println(distance);
  }

  // require N consecutive readings below threshold to confirm
  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    dropConfirmCount++;
    Serial.print("  dropConfirmCount: ");
    Serial.println(dropConfirmCount);
    if (dropConfirmCount >= DROP_CONFIRM_REQUIRED) {
      Serial.println("NOODLE DROP DETECTED");
      waitingForDrop = false;
      dropConfirmCount = 0;
      espSerial.println("DROP_DETECTED");
      // Immediately start heating when drop is detected
      startHeatingProcess();
    }
  } else {
    dropConfirmCount = 0;
  }
}

void startHeatingProcess() {
  heatingInProgress = true;
  relayActive = true;
  l298nMotorActive = true;
  relayStartTime = millis();
  l298nStartTime = millis();

  digitalWrite(RELAY_PIN, HIGH);

  // L298N forward
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);

  Serial.println("HEATING STARTED");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] Relay ON, L298N motor ON (10s motor / 10min relay)");
  espSerial.println("HEATING_STARTED");
}

void handleHeatingProcess() {
  unsigned long now = millis();
  unsigned long l298nElapsed = now - l298nStartTime;
  unsigned long relayElapsed = now - relayStartTime;

  if (l298nMotorActive && l298nElapsed >= L298N_DURATION) {
    l298nMotorActive = false;
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, LOW);
    Serial.print("[");
    Serial.print(now);
    Serial.print("] Pump stopped (L298N OFF) after ");
    Serial.print(l298nElapsed);
    Serial.println("ms");
  }

  if (relayActive && relayElapsed >= RELAY_DURATION) {
    relayActive = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.print("[");
    Serial.print(now);
    Serial.print("] Heater OFF (relay duration complete) after ");
    Serial.print(relayElapsed);
    Serial.println("ms");
  }

  if (!relayActive && !l298nMotorActive) {
    stopHeatingProcess();
  }
}

void stopHeatingProcess() {
  heatingInProgress = false;
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  Serial.println("HEATING COMPLETE");
  espSerial.println("HEATING_COMPLETE");
}

void emergencyStop() {
  emergencyStopFlag = true;
  waitingForDrop = false;
  heatingInProgress = false;
  relayActive = false;
  l298nMotorActive = false;

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);

  Serial.println("EMERGENCY STOP - all outputs OFF");
  espSerial.println("EMERGENCY_STOPPED");
  // emergencyStopFlag remains true until a RESET command is received
}
