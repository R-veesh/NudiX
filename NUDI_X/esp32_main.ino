// esp32_main.ino (FIXED - Motor Driver Overheating Issue)
#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

// Define LED_BUILTIN if not already defined
#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Built-in LED on most ESP32 boards
#endif

// WiFi Credentials - CHANGE THESE!
const char* ssid = "darshana";
const char* password = "07405950";

// MQTT Configuration - Using HiveMQ public broker (example)
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "noodle_vending/command";
const char* mqtt_status = "noodle_vending/status";
const char* mqtt_drop = "noodle_vending/drop_detected";

// Stepper Motor Pins (use safe GPIOs)
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

const int STEPS_PER_REV = 2038;
Stepper stepper1(STEPS_PER_REV, IN1_1, IN3_1, IN2_1, IN4_1);
Stepper stepper2(STEPS_PER_REV, IN1_2, IN3_2, IN2_2, IN4_2);
Stepper stepper3(STEPS_PER_REV, IN1_3, IN3_3, IN2_3, IN4_3);
Stepper stepper4(STEPS_PER_REV, IN1_4, IN3_4, IN2_4, IN4_4);

// Communication with Arduino via Serial2
#define ARDUINO_RX 22 // ESP32 RX2 <- Arduino TX
#define ARDUINO_TX 21 // ESP32 TX2 -> Arduino RX

WiFiClient espClient;
PubSubClient mqttClient(espClient);

String currentCommand = "";
volatile bool dispensing = false;

unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;
bool mqttInitialized = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);
  delay(500);

  Serial.println("\n\n=== ESP32 Noodle Vending Machine Starting ===");
  Serial.println("Serial2 initialized for Arduino communication");
  Serial.print("Serial2 Baud Rate: 9600 | RX Pin: ");
  Serial.print(ARDUINO_RX);
  Serial.print(" | TX Pin: ");
  Serial.println(ARDUINO_TX);
  Serial.println();

  stepper1.setSpeed(8);
  stepper2.setSpeed(8);
  stepper3.setSpeed(8);
  stepper4.setSpeed(8);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // De-energize all steppers on startup to prevent heating
  deEnergizeAllSteppers();

  setupWiFi();
  setupMQTT();

  Serial.println("ESP32 Noodle Vending Machine Ready");
  Serial.println("Waiting for commands...");

  // Try connecting MQTT briefly
  bool mqttConnected = false;
  for (int i = 0; i < 5; i++) {
    if (reconnectMQTT()) {
      mqttConnected = true;
      break;
    }
    delay(500);
  }
  if (mqttConnected) Serial.println("✅ MQTT connected successfully!");
  else Serial.println("⚠️ MQTT connection failed, continuing with limited functionality");
}

void setupWiFi() {
  delay(10);
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
    Serial.println("\n✅ WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.println("\n❌ WiFi connection failed!");
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
  String message = "";
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();
  Serial.print("MQTT Message [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  if (String(topic) == mqtt_topic) {
    handleCommand(message);
  }
}

void handleCommand(String cmd) {
  cmd.trim();
  Serial.println("=== COMMAND RECEIVED ===");
  Serial.print("Command: ");
  Serial.println(cmd);
  Serial.print("Device busy: ");
  Serial.println(dispensing ? "YES" : "NO");

  if (dispensing && cmd.startsWith("noodle_")) {
    Serial.println("Device busy, rejecting new dispense command");
    mqttPublish(mqtt_status, "busy");
    mqttPublish("noodle_vending/log", "Rejected - Currently busy");
    return;
  }

  if (cmd == "status") {
    mqttPublish(mqtt_status, dispensing ? "busy" : "ready");
  } else if (cmd == "emergency_stop") {
    emergencyStop();
  } else if (cmd.startsWith("test_motor_")) {
    int motorNum = cmd.substring(cmd.lastIndexOf('_') + 1).toInt();
    testMotor(motorNum);
  } else if (cmd.startsWith("noodle_")) {
    int noodleNumber = cmd.substring(cmd.indexOf('_') + 1).toInt();
    dispenseNoodle(noodleNumber);
  }
}

void deEnergizeStepper(int motorNumber) {
  Serial.println("De-energizing stepper motor " + String(motorNumber));
  switch (motorNumber) {
    case 1:
      digitalWrite(IN1_1, LOW);
      digitalWrite(IN2_1, LOW);
      digitalWrite(IN3_1, LOW);
      digitalWrite(IN4_1, LOW);
      break;
    case 2:
      digitalWrite(IN1_2, LOW);
      digitalWrite(IN2_2, LOW);
      digitalWrite(IN3_2, LOW);
      digitalWrite(IN4_2, LOW);
      break;
    case 3:
      digitalWrite(IN1_3, LOW);
      digitalWrite(IN2_3, LOW);
      digitalWrite(IN3_3, LOW);
      digitalWrite(IN4_3, LOW);
      break;
    case 4:
      digitalWrite(IN1_4, LOW);
      digitalWrite(IN2_4, LOW);
      digitalWrite(IN3_4, LOW);
      digitalWrite(IN4_4, LOW);
      break;
  }
}

void deEnergizeAllSteppers() {
  Serial.println("De-energizing all stepper motors");
  for (int i = 1; i <= 4; i++) {
    deEnergizeStepper(i);
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
  
  // CRITICAL FIX: De-energize the motor after testing to prevent overheating
  deEnergizeStepper(motorNumber);
  
  mqttPublish("noodle_vending/log", "Motor test complete");
  mqttPublish(mqtt_status, "ready");
}

void emergencyStop() {
  dispensing = false;
  mqttPublish("noodle_vending/log", "EMERGENCY STOP");
  mqttPublish(mqtt_status, "emergency_stop");

  // Send emergency stop to Arduino
  Serial2.println("EMERGENCY_STOP");
  Serial.println("Sent to Arduino: \"EMERGENCY_STOP\"");

  // CRITICAL FIX: De-energize all steppers to prevent heating
  deEnergizeAllSteppers();
  
  delay(200);
  mqttPublish(mqtt_status, "ready");
}

void dispenseNoodle(int noodleNumber) {
  dispensing = true;
  bool dropDetected = false;
  bool heatingComplete = false;

  Serial.println("\n====== DISPENSING SEQUENCE START ======");
  Serial.print("Noodle Number: ");
  Serial.println(noodleNumber);

  mqttPublish(mqtt_status, "dispensing_noodle_" + String(noodleNumber));
  mqttPublish("noodle_vending/log", "Dispensing noodle " + String(noodleNumber));

  // Tell Arduino to start drop detection
  Serial2.println("DISPENSING");
  Serial.println("Sent to Arduino: \"DISPENSING\"");

  unsigned long startTime = millis();
  unsigned long spinTimeout = 15000UL; // 15 seconds to detect drop
  unsigned long spinCount = 0;

  while (millis() - startTime < spinTimeout && !dropDetected && !emergencyStopFlag()) {
    // read Arduino messages
    if (Serial2.available()) {
      String receivedMessage = Serial2.readStringUntil('\n');
      receivedMessage.trim();
      if (receivedMessage.length() > 0) {
        Serial.print("Arduino: \"");
        Serial.print(receivedMessage);
        Serial.println("\"");
        if (receivedMessage == "DROP_DETECTED") {
          dropDetected = true;
          // CRITICAL FIX: De-energize motor immediately when drop detected
          deEnergizeStepper(noodleNumber);
          mqttPublish(mqtt_drop, "success_noodle_" + String(noodleNumber));
          mqttPublish("noodle_vending/log", "Drop detected for noodle " + String(noodleNumber));
          break;
        } else if (receivedMessage == "EMERGENCY_STOPPED") {
          Serial.println("Arduino reported emergency stop");
          mqttPublish("noodle_vending/log", "Arduino emergency stopped");
          deEnergizeAllSteppers(); // De-energize on emergency
          dispensing = false;
          mqttPublish(mqtt_status, "ready");
          return;
        }
      }
    }

    // Spin the appropriate stepper a bit
    spinCount++;
    Serial.print("Spin #");
    Serial.println(spinCount);
    switch (noodleNumber) {
      case 1: stepper1.step(100); break;
      case 2: stepper2.step(100); break;
      case 3: stepper3.step(-100); break;
      case 4: stepper4.step(100); break;
      default: break;
    }
    mqttClient.loop();
    delay(300); // small pause between spins
  }

  if (!dropDetected) {
    Serial.println("DROP DETECTION TIMEOUT - NO L298N/RELAY ACTIVATION (Ultrasonic sensor did NOT detect drop)");
    mqttPublish(mqtt_drop, "timeout_noodle_" + String(noodleNumber));
    mqttPublish("noodle_vending/log", "Drop timeout for noodle " + String(noodleNumber) + " - L298N & Relay NOT activated");
    // De-energize motor on timeout
    deEnergizeStepper(noodleNumber);
    dispensing = false;
    mqttPublish(mqtt_status, "ready");
    return;
  }

  // ONLY if drop detected by ultrasonic sensor, tell Arduino to start heating/pump
  // L298N and Relay will ONLY turn ON here, not on timeout
  delay(200);
  Serial2.println("START_HEATING");
  Serial.println("Sent to Arduino: \"START_HEATING\" - L298N & Relay will now activate (drop confirmed by ultrasonic)");
  mqttPublish("noodle_vending/log", "START_HEATING sent to Arduino - L298N & Relay activating for noodle " + String(noodleNumber));

  // Wait for HEATING_COMPLETE (Arduino will send it when both relay and L298N are finished)
  unsigned long heatStart = millis();
  unsigned long heatTimeout = 660000UL; // 11 minutes timeout
  while (millis() - heatStart < heatTimeout && !heatingComplete && !emergencyStopFlag()) {
    if (Serial2.available()) {
      String response = Serial2.readStringUntil('\n');
      response.trim();
      if (response.length() > 0) {
        Serial.print("Arduino: \"");
        Serial.print(response);
        Serial.println("\"");
        if (response == "HEATING_COMPLETE") {
          heatingComplete = true;
          break;
        } else if (response == "EMERGENCY_STOPPED") {
          Serial.println("Arduino reported emergency stop during heating");
          mqttPublish("noodle_vending/log", "Emergency during heating");
          deEnergizeAllSteppers(); // De-energize on emergency
          dispensing = false;
          mqttPublish(mqtt_status, "ready");
          return;
        }
      }
    }
    // send periodic log updates
    mqttClient.loop();
    delay(1000);
  }

  if (heatingComplete) {
    Serial.println("HEATING PROCESS COMPLETED - noodle ready");
    mqttPublish("noodle_vending/log", "Noodle " + String(noodleNumber) + " ready!");
  } else {
    Serial.println("HEATING PROCESS TIMEOUT");
    mqttPublish("noodle_vending/log", "Heating timeout for noodle " + String(noodleNumber));
  }

  dispensing = false;
  mqttPublish(mqtt_status, "ready");
}

void mqttPublish(const char* topic, String message) {
  if (!mqttClient.connected()) {
    Serial.println("MQTT not connected, trying to reconnect...");
    reconnectMQTT();
  }
  bool success = mqttClient.publish(topic, message.c_str(), true);
  if (success) {
    Serial.println("Published to " + String(topic) + ": " + message);
  } else {
    Serial.print("Failed publish to ");
    Serial.print(topic);
    Serial.print(" state=");
    Serial.println(mqttClient.state());
  }
}

bool reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot connect MQTT");
    return false;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL) return mqttClient.connected();
  lastReconnectAttempt = now;

  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP32-NoodleVending-";
  clientId += String(WiFi.macAddress());

  bool connected = mqttClient.connect(clientId.c_str());
  if (!connected) {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }

  Serial.println("connected to MQTT");
  mqttClient.subscribe(mqtt_topic);
  mqttClient.publish(mqtt_status, "ready", true);
  mqttClient.publish("noodle_vending/log", "MQTT connected successfully");
  return true;
}

bool emergencyStopFlag() {
  // simple helper to quickly check if Arduino reported emergency
  // We also check Serial2 for EMERGENCY_STOPPED messages
  if (Serial2.available()) {
    String m = Serial2.readStringUntil('\n');
    m.trim();
    if (m == "EMERGENCY_STOPPED") {
      Serial.println("Detected EMERGENCY_STOPPED from Arduino");
      return true;
    }
    if (m.length() > 0) {
      Serial.print("Arduino: ");
      Serial.println(m);
    }
  }
  return false;
}

void loop() {
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting to reconnect...");
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
        Serial.println("WiFi reconnected!");
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

  // Read any asynchronous messages from Arduino and forward to logs if it's not handled elsewhere
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    message.trim();
    if (message.length() > 0) {
      Serial.print("[");
      Serial.print(millis());
      Serial.print("] Received from Arduino: \"");
      Serial.print(message);
      Serial.println("\"");
      if (message == "STOP_STEPPERS") {
        Serial.println("Arduino requested stepper motors to stop");
        deEnergizeAllSteppers();
        mqttPublish("noodle_vending/log", "Steppers stopped - drop detected");
      } else if (message == "DROP_DETECTED") {
        mqttPublish(mqtt_drop, "detected");
      } else if (message == "HEATING_COMPLETE") {
        mqttPublish("noodle_vending/log", "Arduino heating complete");
      } else if (message == "ARDUINO_READY") {
        mqttPublish("noodle_vending/log", "Arduino connected and ready");
      } else if (message == "EMERGENCY_STOPPED") {
        mqttPublish("noodle_vending/log", "Arduino emergency stopped");
        deEnergizeAllSteppers(); // De-energize on emergency
        dispensing = false;
        mqttPublish(mqtt_status, "ready");
      }
    }
  }

  delay(10);
}