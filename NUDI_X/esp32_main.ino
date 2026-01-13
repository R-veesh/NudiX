// esp32_main.ino (FIXED VERSION)
#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

// Define LED_BUILTIN if not already defined
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Most ESP32 boards use GPIO 2 for built-in LED
#endif

// WiFi Credentials - CHANGE THESE!
const char* ssid = "Darshana";
const char* password = "0777802809";

// MQTT Configuration - Using HiveMQ public broker
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "noodle_vending/command";
const char* mqtt_status = "noodle_vending/status";
const char* mqtt_drop = "noodle_vending/drop_detected";

// Stepper Motor Pins (using ULN2003)
#define IN1_1 19
#define IN2_1 18
#define IN3_1 5
#define IN4_1 17

#define IN1_2 16
#define IN2_2 4
#define IN3_2 2
#define IN4_2 15

#define IN1_3 13
#define IN2_3 12
#define IN3_3 14
#define IN4_3 27

#define IN1_4 26
#define IN2_4 25
#define IN3_4 33
#define IN4_4 32

// Steps per revolution for 28BYJ-48
const int STEPS_PER_REV = 2038;
Stepper stepper1(STEPS_PER_REV, IN1_1, IN3_1, IN2_1, IN4_1);
Stepper stepper2(STEPS_PER_REV, IN1_2, IN3_2, IN2_2, IN4_2);
Stepper stepper3(STEPS_PER_REV, IN1_3, IN3_3, IN2_3, IN4_3);
Stepper stepper4(STEPS_PER_REV, IN1_4, IN3_4, IN2_4, IN4_4);

// Communication with Arduino
#define ARDUINO_RX 34  // ESP32 RX -> Connect to Arduino TX (pin 9)
#define ARDUINO_TX 35  // ESP32 TX -> Connect to Arduino RX (pin 8)

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
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { // Increased timeout
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
    // Still continue - maybe we can work offline
  }
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(30);
  mqttClient.setBufferSize(2048); // Increase buffer size for stability
  
  // Set last will message
  mqttClient.setWill(mqtt_status, "disconnected", 1, true);
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
  if (dispensing) {
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
  
  Serial.println("Dispensing Noodle " + String(noodleNumber));
  mqttPublish(mqtt_status, "dispensing_noodle_" + String(noodleNumber));
  mqttPublish("noodle_vending/log", "Dispensing noodle " + String(noodleNumber));
  
  // Activate corresponding stepper motor
  switch (noodleNumber) {
    case 1:
      stepper1.step(STEPS_PER_REV / 2); // Half revolution for dispensing
      break;
    case 2:
      stepper2.step(STEPS_PER_REV / 2);
      break;
    case 3:
      stepper3.step(STEPS_PER_REV / 2);
      break;
    case 4:
      stepper4.step(STEPS_PER_REV / 2);
      break;
  }
  
  delay(100);
  
  // Signal Arduino to prepare for drop detection
  Serial2.println("DISPENSING");
  Serial.println("Sent: DISPENSING to Arduino");
  
  // Wait for drop detection (timeout after 15 seconds)
  unsigned long startTime = millis();
  bool dropDetected = false;
  
  while (millis() - startTime < 15000) {
    // Check for response from Arduino
    if (Serial2.available()) {
      String response = Serial2.readStringUntil('\n');
      response.trim();
      Serial.println("Arduino: " + response);
      
      if (response == "DROP_DETECTED") {
        dropDetected = true;
        mqttPublish(mqtt_drop, "success_noodle_" + String(noodleNumber));
        mqttPublish("noodle_vending/log", "Drop detected for noodle " + String(noodleNumber));
        break;
      }
    }
    
    // Also check MQTT for emergency stop
    mqttClient.loop();
    delay(100);
  }
  
  if (dropDetected) {
    Serial.println("Drop detected successfully!");
    
    // Signal Arduino to start heating/water pump
    Serial2.println("START_HEATING");
    Serial.println("Sent: START_HEATING to Arduino");
    
    // Wait for completion signal from Arduino (11 minutes max)
    startTime = millis();
    bool heatingComplete = false;
    
    while (millis() - startTime < 660000) { // 11 minutes
      if (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        response.trim();
        Serial.println("Arduino: " + response);
        
        if (response == "HEATING_COMPLETE") {
          heatingComplete = true;
          break;
        }
      }
      
      // Publish heating status every 30 seconds
      static unsigned long lastStatus = 0;
      if (millis() - lastStatus > 30000 && !heatingComplete) {
        lastStatus = millis();
        long secondsLeft = (660000 - (millis() - startTime)) / 1000;
        mqttPublish("noodle_vending/log", "Heating... " + String(secondsLeft) + "s remaining");
      }
      
      mqttClient.loop();
      delay(1000);
    }
    
    if (heatingComplete) {
      Serial.println("Heating process completed!");
      mqttPublish("noodle_vending/log", "Noodle " + String(noodleNumber) + " ready!");
    } else {
      Serial.println("Heating process timeout!");
      mqttPublish("noodle_vending/log", "Heating timeout for noodle " + String(noodleNumber));
    }
    
  } else {
    Serial.println("Drop detection timeout!");
    mqttPublish(mqtt_drop, "timeout_noodle_" + String(noodleNumber));
    mqttPublish("noodle_vending/log", "Drop timeout for noodle " + String(noodleNumber));
  }
  
  dispensing = false;
  mqttPublish(mqtt_status, "ready");
}

void mqttPublish(const char* topic, String message) {
  // Try to reconnect if not connected
  if (!mqttClient.connected()) {
    Serial.println("⚠️ MQTT not connected, attempting to reconnect...");
    if (!reconnectMQTT()) {
      Serial.println("❌ Failed to reconnect MQTT for publish");
      // Still try to publish - sometimes it works even if not connected
    }
  }
  
  // Always try to publish, even if not connected
  // The client will handle reconnection internally
  bool success = mqttClient.publish(topic, message.c_str(), true); // retained = true
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
  
  // Create a client ID with MAC address for uniqueness
  String clientId = "ESP32-NoodleVending-";
  clientId += String(WiFi.macAddress());
  
  // Try to connect with 5 second timeout
  bool connected = mqttClient.connect(clientId.c_str());
  
  if (connected) {
    Serial.println("✅ connected");
    mqttConnectionAttempts = 0; // Reset counter on success
    mqttClient.subscribe(mqtt_topic);
    
    // Publish connection success
    mqttClient.publish(mqtt_status, "ready", true); // retained
    mqttClient.publish("noodle_vending/log", "MQTT connected successfully");
    
    // Blink LED to indicate connection
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
  // Check WiFi connection
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) { // Check WiFi every 10 seconds
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi disconnected, attempting to reconnect...");
      digitalWrite(LED_BUILTIN, LOW);
      
      // Try to reconnect WiFi
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
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Blink LED when MQTT disconnected
    reconnectMQTT();
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Solid LED when connected
    mqttClient.loop();
  }
  
  // Check for messages from Arduino
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    message.trim();
    Serial.println("From Arduino: " + message);
    
    if (message == "DROP_DETECTED") {
      dropDetectedFlag = true;
      mqttPublish(mqtt_drop, "detected");
    } else if (message == "HEATING_COMPLETE") {
      heatingCompleteFlag = true;
    } else if (message == "EMERGENCY_STOPPED") {
      mqttPublish("noodle_vending/log", "Arduino emergency stopped");
      dispensing = false;
      mqttPublish(mqtt_status, "ready");
    }
  }
  
  // Small delay to prevent watchdog timer issues
  delay(10);
}