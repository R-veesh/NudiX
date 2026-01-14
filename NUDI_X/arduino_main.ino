// arduino_main.ino
#include <SoftwareSerial.h>

#define RX_PIN 8   // Arduino RX <- ESP32 TX
#define TX_PIN 9   // Arduino TX -> ESP32 RX

// Ultrasonic Sensor Pins (HC-SR04)
#define ULTRASONIC_TRIG 2
#define ULTRASONIC_ECHO 6

#define RELAY_PIN 7        // Heater / water relay
#define L298N_IN1 3        // L298N Input 1
#define L298N_IN2 4        // L298N Input 2

#define BUZZER_PIN 10

SoftwareSerial espSerial(RX_PIN, TX_PIN);

// State variables
bool waitingForDrop = false;
bool heatingInProgress = false;
bool emergencyStopFlag = false;
bool waterPumpRunning = false;
bool dropDetected = false;
bool relayActive = false;
bool l298nMotorActive = false;

unsigned long heatingStartTime = 0;
unsigned long waterPumpStartTime = 0;
unsigned long relayStartTime = 0;
unsigned long l298nStartTime = 0;
unsigned long lastUltrasonicCheck = 0;

#define DISTANCE_THRESHOLD 23  // cm
#define ULTRASONIC_CHECK_INTERVAL 200  // Check ultrasonic every 200ms
#define RELAY_DURATION 600000  // 10 minutes
#define L298N_DURATION 20000  // 20 seconds
#define STEPPER_SPIN_INTERVAL 500  // Spin stepper every 500ms during drop detection

void setup() {
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(9600);
  espSerial.begin(9600);

  delay(2000);

  Serial.println("Arduino Noodle Vending Ready");
  espSerial.println("ARDUINO_READY");

  beep(100);

  Serial.println("=== Arduino Configuration ===");
  Serial.println("RX Pin: 8");
  Serial.println("TX Pin: 9");
  Serial.println("Relay Pin: 7");
  Serial.println("Ultrasonic Trig: 2");
  Serial.println("Ultrasonic Echo: 6");
  Serial.println("L298N IN1: 3");
  Serial.println("L298N IN2: 4");
  Serial.println("Buzzer Pin: 10");
}

void loop() {
  if (espSerial.available()) {
    String command = espSerial.readStringUntil('\n');
    command.trim();

    Serial.print("[");
    Serial.print(millis());
    Serial.print("] Received: ");
    Serial.println(command);

    if (command == "DISPENSING") startDropDetection();
    else if (command == "START_HEATING") startHeatingProcess();
    else if (command == "EMERGENCY_STOP") emergencyStop();
  }

  if (waitingForDrop && !emergencyStopFlag) {
    checkDropDetection();
  }

  if (dropDetected && !heatingInProgress) {
    // After drop is detected, start heating process automatically
    if (millis() > lastUltrasonicCheck + 1000) {
      startHeatingProcess();
      dropDetected = false;
    }
  }

  if (heatingInProgress && !emergencyStopFlag) {
    handleHeatingProcess();
  }

  delay(50);
}

void startDropDetection() {
  waitingForDrop = true;
  dropDetected = false;
  lastUltrasonicCheck = millis();

  Serial.println("\n========== NOODLE DROP DETECTION STARTED ==========");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] Waiting for noodle drop...");
  Serial.println("Stepper motor spinning continuously...");
  
  espSerial.println("WAITING_FOR_DROP");

  beep(100);
  delay(200);
  beep(100);
}

float getUltrasonicDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  return (duration * 0.0343) / 2;
}

void checkDropDetection() {
  unsigned long now = millis();
  
  // Check ultrasonic sensor at intervals
  if (now - lastUltrasonicCheck < ULTRASONIC_CHECK_INTERVAL) {
    return;
  }
  
  lastUltrasonicCheck = now;
  float distance = getUltrasonicDistance();

  Serial.print("[");
  Serial.print(now);
  Serial.print("] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    Serial.println("\n========== NOODLE DROP DETECTED! ==========");
    Serial.print("[");
    Serial.print(millis());
    Serial.println("] Drop confirmed by ultrasonic sensor!");
    
    waitingForDrop = false;
    dropDetected = true;

    espSerial.println("DROP_DETECTED");
    beep(300);
    delay(500);
  }
}

void startHeatingProcess() {
  heatingInProgress = true;
  relayActive = true;
  l298nMotorActive = true;
  
  heatingStartTime = millis();
  relayStartTime = millis();
  l298nStartTime = millis();

  Serial.println("\n========== HEATING PROCESS STARTED ==========");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] Activating heating components...");
  
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] ✓ RELAY (Heater) turned ON for 10 minutes");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] ✓ L298N Motor Driver turned ON for 20 seconds");

  espSerial.println("HEATING_STARTED");

  // Relay ON (Heater)
  digitalWrite(RELAY_PIN, HIGH);

  // L298N Motor ON (Pump)
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);

  beep(100);
  delay(100);
  beep(100);
}

void handleHeatingProcess() {
  unsigned long now = millis();
  unsigned long relayElapsed = now - relayStartTime;
  unsigned long l298nElapsed = now - l298nStartTime;

  // Check and display relay status every 10 seconds
  static unsigned long lastRelayStatus = 0;
  if (now - lastRelayStatus > 10000) {
    lastRelayStatus = now;
    unsigned long relayRemaining = RELAY_DURATION - relayElapsed;
    Serial.print("[");
    Serial.print(now);
    Serial.print("] RELAY Status: ON | Remaining: ");
    Serial.print(relayRemaining / 1000);
    Serial.println(" seconds");
  }

  // Check and display L298N motor status every 5 seconds
  static unsigned long lastL298nStatus = 0;
  if (now - lastL298nStatus > 5000) {
    lastL298nStatus = now;
    unsigned long l298nRemaining = L298N_DURATION - l298nElapsed;
    Serial.print("[");
    Serial.print(now);
    Serial.print("] L298N Motor Status: ON | Remaining: ");
    Serial.print(l298nRemaining / 1000);
    Serial.println(" seconds");
  }

  // Turn off L298N motor after 20 seconds
  if (l298nMotorActive && l298nElapsed > L298N_DURATION) {
    l298nMotorActive = false;
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, LOW);
    
    Serial.println("\n========== L298N MOTOR STOPPED ==========");
    Serial.print("[");
    Serial.print(millis());
    Serial.println("] ✓ L298N Motor turned OFF after 20 seconds");
  }

  // Turn off relay after 10 minutes
  if (relayActive && relayElapsed > RELAY_DURATION) {
    relayActive = false;
    digitalWrite(RELAY_PIN, LOW);
    
    Serial.println("\n========== RELAY STOPPED ==========");
    Serial.print("[");
    Serial.print(millis());
    Serial.println("] ✓ RELAY (Heater) turned OFF after 10 minutes");
  }

  // End heating process when both are done
  if (!relayActive && !l298nMotorActive) {
    stopHeatingProcess();
  }
}

void stopWaterPump() {
  waterPumpRunning = false;
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] Water pump stopped");
}

void stopHeatingProcess() {
  heatingInProgress = false;
  relayActive = false;
  l298nMotorActive = false;

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);

  Serial.println("\n========== HEATING PROCESS COMPLETED ==========");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] All heating components turned OFF");

  espSerial.println("HEATING_COMPLETE");

  for (int i = 0; i < 3; i++) {
    beep(100);
    delay(100);
  }
}

void emergencyStop() {
  Serial.println("\n========== EMERGENCY STOP ACTIVATED ==========");
  Serial.print("[");
  Serial.print(millis());
  Serial.println("] All operations halted!");

  emergencyStopFlag = true;
  waitingForDrop = false;
  heatingInProgress = false;
  relayActive = false;
  l298nMotorActive = false;

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);

  espSerial.println("EMERGENCY_STOPPED");

  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }

  emergencyStopFlag = false;
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
