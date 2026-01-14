// esp32_main.ino (CORRECTED PIN ASSIGNMENTS)
#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

// Define LED_BUILTIN if not already defined
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Built-in LED on most ESP32 boards
#endif


// WiFi Credentials - CHANGE THESE!
const char* ssid = "darshana";
const char* password = "07405950";

// MQTT Configuration - Using HiveMQ public broker
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "noodle_vending/command";
const char* mqtt_status = "noodle_vending/status";
const char* mqtt_drop = "noodle_vending/drop_detected";

// Stepper Motor Pins (using ULN2003)
// FIXED: Removed GPIO6-11, GPIO1, GPIO3, using only safe GPIO pins
#define IN1_1 19
#define IN2_1 18
#define IN3_1 5
#define IN4_1 17

#define IN1_2 16
#define IN2_2 4
#define IN3_2 15
#define IN4_2 14

#define IN1_3 13
#define IN2_3 12
#define IN3_3 27
#define IN4_3 26

#define IN1_4 25
#define IN2_4 33
#define IN3_4 32
#define IN4_4 23

// Steps per revolution for 28BYJ-48
const int STEPS_PER_REV = 2038;
Stepper stepper1(STEPS_PER_REV, IN1_1, IN3_1, IN2_1, IN4_1);
Stepper stepper2(STEPS_PER_REV, IN1_2, IN3_2, IN2_2, IN4_2);
Stepper stepper3(STEPS_PER_REV, IN1_3, IN3_3, IN2_3, IN4_3);
Stepper stepper4(STEPS_PER_REV, IN1_4, IN3_4, IN2_4, IN4_4);

// Communication with Arduino
// Using Serial2 with proper RX/TX pins
#define ARDUINO_RX 22  // ESP32 RX2 <- Arduino TX
#define ARDUINO_TX 21  // ESP32 TX2 -> Arduino RX

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String currentCommand = "";
bool dispensing = false;
bool dropDetectedFlag = false;
bool heatingCompleteFlag = false;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;
bool mqttInitialized = false;
int mqttConnectionAttempts = 0;
const int MAX_MQTT_ATTEMPTS = 10;



void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);
  
  delay(500);
  Serial.println("\n\n=== ESP32 Noodle Vending Machine Starting ===");
  Serial.println("Serial2 initialized for Arduino communication");
  Serial.print("Serial2 Baud Rate: 9600");
  Serial.print(" | RX Pin: 22 | TX Pin: 21");
  Serial.println("\n");
  
  // Initialize stepper motor speeds (slower for better control)
  stepper1.setSpeed(8);
  stepper2.setSpeed(8);
  stepper3.setSpeed(8);
  stepper4.setSpeed(8);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  setupWiFi();
  setupMQTT();
  
  Serial.println("ESP32 Noodle Vending Machine Ready");
  Serial.println("Waiting for commands...");
  
  // Initial MQTT connection with retry
  bool mqttConnected = false;
  for (int i = 0; i < MAX_MQTT_ATTEMPTS; i++) {
    if (reconnectMQTT()) {
      mqttConnected = true;
      break;
    }
    delay(1000);
  }
  
  if (mqttConnected) {
    Serial.println("✅ MQTT connected successfully!");
  } else {
    Serial.println("⚠️ MQTT connection failed, continuing with limited functionality");
  }
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("✅ WiFi connected");
    Serial.print("📡 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.println("");
    Serial.println("❌ WiFi connection failed!");
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);
  mqttClient.setBufferSize(2048);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("📨 MQTT Message [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println(message);
  
  if (String(topic) == mqtt_topic) {
    handleCommand(message);
  }
}

void handleCommand(String cmd) {
  Serial.println("=== COMMAND RECEIVED ===");
  Serial.print("Timestamp: ");
  Serial.println(millis());
  Serial.print("Command: ");
  Serial.println(cmd);
  Serial.print("Device busy: ");
  Serial.println(dispensing ? "YES" : "NO");
  
  if (dispensing) {
    Serial.println("❌ Device is busy, rejecting command");
    mqttPublish(mqtt_status, "busy");
    mqttPublish("noodle_vending/log", "Rejected - Currently busy");
    return;
  }
  
  cmd.trim();
  Serial.println("Processing command: " + cmd);
  mqttPublish("noodle_vending/log", "Processing: " + cmd);
  
  if (cmd == "noodle_1") {
    dispenseNoodle(1);
  } else if (cmd == "noodle_2") {
    dispenseNoodle(2);
  } else if (cmd == "noodle_3") {
    dispenseNoodle(3);
  } else if (cmd == "noodle_4") {
    dispenseNoodle(4);
  } else if (cmd == "status") {
    mqttPublish(mqtt_status, dispensing ? "busy" : "ready");
  } else if (cmd == "emergency_stop") {
    emergencyStop();
  } else if (cmd == "test_motor_1") {
    testMotor(1);
  } else if (cmd == "test_motor_2") {
    testMotor(2);
  } else if (cmd == "test_motor_3") {
    testMotor(3);
  } else if (cmd == "test_motor_4") {
    testMotor(4);
  }
}

void testMotor(int motorNumber) {
  Serial.println("Testing motor " + String(motorNumber));
  mqttPublish("noodle_vending/log", "Testing motor " + String(motorNumber));
  
  switch (motorNumber) {
    case 1:
      stepper1.step(STEPS_PER_REV / 4);
      delay(1000);
      stepper1.step(-STEPS_PER_REV / 4);
      break;
    case 2:
      stepper2.step(STEPS_PER_REV / 4);
      delay(1000);
      stepper2.step(-STEPS_PER_REV / 4);
      break;
    case 3:
      stepper3.step(STEPS_PER_REV / 4);
      delay(1000);
      stepper3.step(-STEPS_PER_REV / 4);
      break;
    case 4:
      stepper4.step(STEPS_PER_REV / 4);
      delay(1000);
      stepper4.step(-STEPS_PER_REV / 4);
      break;
  }
  
  mqttPublish("noodle_vending/log", "Motor test complete");
  mqttPublish(mqtt_status, "ready");
}

void emergencyStop() {
  dispensing = false;
  mqttPublish("noodle_vending/log", "EMERGENCY STOP");
  mqttPublish(mqtt_status, "emergency_stop");
  
  // Send emergency stop to Arduino
  Serial2.println("EMERGENCY_STOP");
  Serial.println("[" + String(millis()) + "] Sent to Arduino: \"EMERGENCY_STOP\"");
  
  // Reset all steppers
  digitalWrite(IN1_1, LOW);
  digitalWrite(IN2_1, LOW);
  digitalWrite(IN3_1, LOW);
  digitalWrite(IN4_1, LOW);
  
  digitalWrite(IN1_2, LOW);
  digitalWrite(IN2_2, LOW);
  digitalWrite(IN3_2, LOW);
  digitalWrite(IN4_2, LOW);
  
  digitalWrite(IN1_3, LOW);
  digitalWrite(IN2_3, LOW);
  digitalWrite(IN3_3, LOW);
  digitalWrite(IN4_3, LOW);
  
  digitalWrite(IN1_4, LOW);
  digitalWrite(IN2_4, LOW);
  digitalWrite(IN3_4, LOW);
  digitalWrite(IN4_4, LOW);
  
  delay(1000);
  mqttPublish(mqtt_status, "ready");
}

void dispenseNoodle(int noodleNumber) {
  dispensing = true;
  dropDetectedFlag = false;
  heatingCompleteFlag = false;
  
  Serial.println("\n====== DISPENSING SEQUENCE START ======");
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] Noodle Number: ");
  Serial.println(noodleNumber);
  
  Serial.println("🍜 Dispensing Noodle " + String(noodleNumber));
  mqttPublish(mqtt_status, "dispensing_noodle_" + String(noodleNumber));
  mqttPublish("noodle_vending/log", "Dispensing noodle " + String(noodleNumber));
  
  // Signal Arduino to prepare for drop detection
  Serial2.println("DISPENSING");
  Serial.println("[" + String(millis()) + "] Sent to Arduino: \"DISPENSING\"");
  
  // Continuously spin stepper motor until drop is detected
  unsigned long startTime = millis();
  bool dropDetected = false;
  int spinCount = 0;
  
  Serial.println("[" + String(millis()) + "] Starting stepper motor continuous spin...");
  
  while (millis() - startTime < 15000 && !dropDetected) {  // 15 second timeout
    // Check if Arduino has detected drop
    if (Serial2.available()) {
      String receivedMessage = Serial2.readStringUntil('\n');
      receivedMessage.trim();
      Serial.println("[" + String(millis()) + "] Arduino: \"" + receivedMessage + "\"");
      
      if (receivedMessage == "DROP_DETECTED") {
        dropDetected = true;
        dropDetectedFlag = true;
        Serial.println("[" + String(millis()) + "] ✅ Noodle drop confirmed by Arduino!");
        mqttPublish(mqtt_drop, "success_noodle_" + String(noodleNumber));
        mqttPublish("noodle_vending/log", "Drop detected for noodle " + String(noodleNumber));
        break;
      }
    }
    
    // Spin stepper motor continuously
    spinCount++;
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] Stepper Motor Spin #");
    Serial.println(spinCount);
    
    switch (noodleNumber) {
      case 1:
        stepper1.step(100);  // Spin in small increments
        break;
      case 2:
        stepper2.step(100);
        break;
      case 3:
        stepper3.step(100);
        break;
      case 4:
        stepper4.step(100);
        break;
    }
    
    mqttClient.loop();
    delay(300);  // Small delay between spins
  }
  
  if (dropDetected) {
    Serial.println("\n========== DROP DETECTION SUCCESSFUL ==========");
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] Total spins before drop: ");
    Serial.println(spinCount);
    
    // Signal Arduino to start heating/water pump and L298N motor
    delay(500);
    Serial2.println("START_HEATING");
    Serial.println("[" + String(millis()) + "] Sent to Arduino: \"START_HEATING\"");
    Serial.println("[" + String(millis()) + "] ✓ RELAY (Heater) activated for 10 minutes");
    Serial.println("[" + String(millis()) + "] ✓ L298N Motor activated for 20 seconds");
    
    // Wait for heating to complete (Relay: 10 minutes, L298N Motor: 20 seconds)
    // Total wait: 10 minutes for relay to finish
    startTime = millis();
    bool heatingComplete = false;
    unsigned long statusUpdateInterval = millis();
    
    while (millis() - startTime < 660000 && !heatingComplete) {  // 11 minutes timeout
      if (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        response.trim();
        Serial.println("[" + String(millis()) + "] Arduino: \"" + response + "\"");
        
        if (response == "HEATING_COMPLETE") {
          heatingComplete = true;
          heatingCompleteFlag = true;
          break;
        }
      }
      
      // Publish heating status every 30 seconds
      if (millis() - statusUpdateInterval > 30000) {
        statusUpdateInterval = millis();
        long secondsLeft = (660000 - (millis() - startTime)) / 1000;
        Serial.println("[" + String(millis()) + "] Heating in progress... " + String(secondsLeft) + "s remaining");
        mqttPublish("noodle_vending/log", "Heating... " + String(secondsLeft) + "s remaining");
      }
      
      mqttClient.loop();
      delay(1000);
    }
    
    if (heatingComplete) {
      Serial.println("\n========== HEATING PROCESS COMPLETED ==========");
      Serial.print("[");
      Serial.print(millis());
      Serial.println("] Noodle " + String(noodleNumber) + " is ready!");
      mqttPublish("noodle_vending/log", "Noodle " + String(noodleNumber) + " ready!");
    } else {
      Serial.println("⚠️ Heating process timeout!");
      mqttPublish("noodle_vending/log", "Heating timeout for noodle " + String(noodleNumber));
    }
    
  } else {
    Serial.println("\n========== DROP DETECTION TIMEOUT ==========");
    Serial.print("[");
    Serial.print(millis());
    Serial.println("] Failed to detect noodle drop within 15 seconds!");
    mqttPublish(mqtt_drop, "timeout_noodle_" + String(noodleNumber));
    mqttPublish("noodle_vending/log", "Drop timeout for noodle " + String(noodleNumber));
  }
  
  dispensing = false;
  mqttPublish(mqtt_status, "ready");
}

void mqttPublish(const char* topic, String message) {
  if (!mqttClient.connected()) {
    Serial.println("⚠️ MQTT not connected, attempting to reconnect...");
    if (!reconnectMQTT()) {
      Serial.println("❌ Failed to reconnect MQTT for publish");
    }
  }
  
  bool success = mqttClient.publish(topic, message.c_str(), true);
  if (success) {
    Serial.println("✅ Published to " + String(topic) + ": " + message);
  } else {
    Serial.println("❌ Failed to publish to " + String(topic));
    Serial.print("MQTT state: ");
    Serial.println(mqttClient.state());
  }
}

bool reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected, cannot connect MQTT");
    return false;
  }
  
  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL) {
    return mqttClient.connected();
  }
  
  lastReconnectAttempt = now;
  
  Serial.print("🔗 Attempting MQTT connection...");
  
  String clientId = "ESP32-NoodleVending-";
  clientId += String(WiFi.macAddress());
  
  bool connected = false;
  
  connected = mqttClient.connect(
    clientId.c_str(),
    mqtt_status,
    1,
    true,
    "disconnected"
  );
  
  if (!connected) {
    Serial.println("First attempt failed, trying simple connection...");
    connected = mqttClient.connect(clientId.c_str());
  }
  
  if (connected) {
    Serial.println("✅ connected");
    mqttConnectionAttempts = 0;
    mqttClient.subscribe(mqtt_topic);
    
    mqttClient.publish(mqtt_status, "ready", true);
    mqttClient.publish("noodle_vending/log", "MQTT connected successfully");
    
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
    return true;
  } else {
    mqttConnectionAttempts++;
    Serial.print("❌ failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
    Serial.println("Attempt " + String(mqttConnectionAttempts) + " of " + String(MAX_MQTT_ATTEMPTS));
    return false;
  }
}

void loop() {
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi disconnected, attempting to reconnect...");
      digitalWrite(LED_BUILTIN, LOW);
      
      WiFi.disconnect();
      delay(100);
      WiFi.reconnect();
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi reconnected!");
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
  }
  
  if (!mqttClient.connected()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    reconnectMQTT();
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    mqttClient.loop();
  }
  
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    message.trim();
    
    // Debug output
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] Received from Arduino: \"");
    Serial.print(message);
    Serial.println("\"");
    
    if (message == "DROP_DETECTED") {
      dropDetectedFlag = true;
      mqttPublish(mqtt_drop, "detected");
    } else if (message == "HEATING_COMPLETE") {
      heatingCompleteFlag = true;
    } else if (message == "EMERGENCY_STOPPED") {
      mqttPublish("noodle_vending/log", "Arduino emergency stopped");
      dispensing = false;
      mqttPublish(mqtt_status, "ready");
    } else if (message == "ARDUINO_READY") {
      Serial.println("✅ Arduino is ready!");
      mqttPublish("noodle_vending/log", "Arduino connected and ready");
    }
  }
  
  delay(10);
}